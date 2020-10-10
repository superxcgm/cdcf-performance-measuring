#include <caf/all.hpp>
#include <cdcf/actor_system.h>
#include <cdcf/logger.h>
#include "blockable_count_actor.h"
#include "count_down_latch.h"
#include "ping_throughput_actor.h"

std::vector<std::string> split(const std::string &input,
                               char delim) {
    std::vector<std::string> result;
    std::stringstream ss(input);
    std::string token;
    while (std::getline(ss, token, delim)) {
        if (token.empty()) {
            continue;
        }
        result.push_back(token);
    }
    return result;
}

std::string toLower(std::string &str) {
    std::string result;
    std::transform(str.begin(), str.end(), std::back_inserter(result), ::tolower);
    return result;
}

void handleEnqueueing(caf::actor_system &system, std::vector<std::string> &args) {
    int n = std::atoi(args[1].c_str());
    CountDownLatch start_latch(1);
    CountDownLatch finish_latch(1);
    auto actor = system.spawn(blockableCountActorFun, &start_latch, &finish_latch, n);

    auto start = std::chrono::high_resolution_clock::now();
    EmptyMessage empty_message;
    for (int i = 0; i < n; i++) {
        caf::anon_send(actor, empty_message);
    }
    auto spent_time = std::chrono::duration_cast<std::chrono::nanoseconds>(
            std::chrono::high_resolution_clock::now() - start).count();

//            std::cout << "finish send message" << std::endl;
    start_latch.countDown();
    finish_latch.await();

    std::cout << "Enqueueing:" << std::endl;
    std::cout << "\t" << n << " ops" << std::endl;
    std::cout << "\t" << spent_time << " ns" << std::endl;
    std::cout << "\t" << (n * 1000'000'000L / spent_time) << " ops/s" << std::endl;
}

int roundToParallelism(int n, int parallelism) {
    return (n / parallelism) * parallelism;
}

int roundToEven(int x) {
    if ((x & 1) == 1) {
        x--;
    }
    return x;
}

void handlePingThroughput(caf::actor_system &system, std::vector<std::string> &args, int pair_count) {
    int n = std::atoi(args[1].c_str());

    int p = roundToEven(pair_count);
    n = roundToParallelism(n, p);
    CountDownLatch finish_latch(p * 2);
    std::vector<caf::actor> actors(p * 2);

    auto start = std::chrono::high_resolution_clock::now();
    for (int i = 0; i < p; i++){
        auto actor1 = system.spawn(pingThroughputActorFun, &finish_latch, n/p/2);
        auto actor2 = system.spawn(pingThroughputActorFun, &finish_latch, n/p/2);

        actors.push_back(actor1);
        actors.push_back(actor2);
    }

    for (int i = 0; i < actors.size(); i += 2) {
        caf::anon_send(actors[i], PingThroughputMessage{actors[i + 1]});
    }
    finish_latch.countDown();

    auto spent_time = std::chrono::duration_cast<std::chrono::nanoseconds>(
            std::chrono::high_resolution_clock::now() - start).count();
    finish_latch.await();

    std::cout << "Ping throughput:" << std::endl;
    std::cout << "\t" << n << " ops" << std::endl;
    std::cout << "\t" << p << " pairs" << std::endl;
    std::cout << "\t" << spent_time << " ns" << std::endl;
    std::cout << "\t" << (n * 1000'000'000L / spent_time) << " ops/s" << std::endl;
}

[[noreturn]] void caf_main(caf::actor_system &system, const cdcf::actor_system::Config &config) {
    cdcf::Logger::Init(config);

    while (true) {
        std::cout << "> ";
        std::string line;
        std::getline(std::cin, line);
        auto args = split(line, ' ');
        if (args.empty()) {
            continue;
        }
        // Todo: string trim
        std::string command = toLower(args[0]);

        if (command == "h" || command == "help") {
            // print help message
            continue;
        }

        if (command == "q" || command == "quit") {
            exit(0);
        }

        if (command == "n") {
            std::cout << "hello world" << std::endl;
            continue;
        }

        if (command == "enqueueing") {
            handleEnqueueing(system, args);
            continue;
        }

        if (command == "ping-throughput-10k") {
            int pair_count = 10'000;
            handlePingThroughput(system, args, pair_count);
            continue;
        }
    }
}

CAF_MAIN(caf::io::middleman)
