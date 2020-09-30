#include <caf/all.hpp>
#include <cdcf/actor_system.h>
#include <cdcf/logger.h>
#include "blockable_count_actor.h"
#include "count_down_latch.h"
#include "mini_actor.h"

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

void handleDequeueing(caf::actor_system &system, std::vector<std::string> &args) {
    int n = std::atoi(args[1].c_str());
    CountDownLatch start_latch(1);
    CountDownLatch finish_latch(1);
    auto actor = system.spawn(blockableCountActorFun, &start_latch, &finish_latch, n);

    EmptyMessage empty_message;
    for (int i = 0; i < n; i++) {
        caf::anon_send(actor, empty_message);
    }

//  std::cout << "finish send message" << std::endl;
    auto start = std::chrono::high_resolution_clock::now();
    start_latch.countDown();
    finish_latch.await();
    auto spent_time = std::chrono::duration_cast<std::chrono::nanoseconds>(
            std::chrono::high_resolution_clock::now() - start).count();

    std::cout << "Dequeueing:" << std::endl;
    std::cout << "\t" << n << " ops" << std::endl;
    std::cout << "\t" << spent_time << " ns" << std::endl;
    std::cout << "\t" << (n * 1000'000'000L / spent_time) << " ops/s" << std::endl;
}

void handleInitiation(caf::actor_system &system, std::vector<std::string> &args) {
    int n = std::atoi(args[1].c_str());

    std::vector<caf::actor> actor_list;
    auto start = std::chrono::high_resolution_clock::now();
    for(int i=0; i<n; i++) {
        auto actor = system.spawn(miniFun, n);
        actor_list.push_back(actor);
    }
    auto spent_time = std::chrono::duration_cast<std::chrono::nanoseconds>(
            std::chrono::high_resolution_clock::now() - start).count();


    for (auto actor : actor_list) {
        // std::cout << "actor id: " << actor->id() << std::endl;
        // std::cout << "actor address: " << caf::to_string(actor->address()) << std::endl;
        caf::actor_cast<caf::event_based_actor*>(actor)->quit();
    }

    std::cout << "Initiation:" << std::endl;
    std::cout << "\t" << n << " ops" << std::endl;
    std::cout << "\t" << spent_time << " ns" << std::endl;
    std::cout << "\t" << (n * 1000'000'000L / spent_time) << " ops/s" << std::endl;
}

void handleSingleProducerSending(caf::actor_system &system, std::vector<std::string> &args) {
    int n = std::atoi(args[1].c_str());

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

        if (command == "dequeueing") {
            handleDequeueing(system, args);
            continue;
        }

        if (command == "initiation") {
            handleInitiation(system, args);
            continue;
        }

        if (command == "single-producer-sending") {
            handleSingleProducerSending(system, args);
            continue;
        }

    }
}

CAF_MAIN(caf::io::middleman)
