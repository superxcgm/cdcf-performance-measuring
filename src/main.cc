#include <caf/all.hpp>
#include <cdcf/actor_system.h>
#include <cdcf/logger.h>
#include "blockable_count_actor.h"
#include "count_down_latch.h"
#include "mini_actor.h"
#include "stateful_actor.h"
#include "circle_barrier.h"
#include <unistd.h>
#include <hdr_histogram.h>
#include <hdr_histogram_log.h>
#include "ping_throughput_actor.h"
#include "ping_latency_actor.h"
#include "latency_histogram.h"

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

void handlePingLatency(caf::actor_system &system, std::vector<std::string> &args) {
    int n = std::atoi(args[1].c_str());
    n = roundToEven(n);
    CountDownLatch finish_latch(2);

    LatencyHistogram latency_histogram;
    auto actor1 = system.spawn(pingLatencyActorFun, &finish_latch, n / 2, &latency_histogram);
    auto actor2 = system.spawn(pingLatencyActorFun, &finish_latch, n / 2, &latency_histogram);
    auto start = std::chrono::high_resolution_clock::now();
    caf::anon_send(actor1, PingLatencyMessage{actor2});
    finish_latch.await();
    auto spent_time = std::chrono::duration_cast<std::chrono::nanoseconds>(
            std::chrono::high_resolution_clock::now() - start).count();

    std::cout << "Ping latency:" << std::endl;
    std::cout << "\t" << n << " ops" << std::endl;
    std::cout << "\t" << spent_time << " ns" << std::endl;
    std::vector<double> list = {0.0, 0.5, 0.9, 0.99, 0.999, 0.9999, 1.0};
    for (auto &x: list){
        auto temp_value = hdr_value_at_percentile(latency_histogram.latency_histogram_, x * 100);
        printf("\tp(%.5f) = %8lld ns/op\n", x, temp_value);
    }
}

void handlePingThroughput(caf::actor_system &system, std::vector<std::string> &args, int pair_count) {
    int n = std::atoi(args[1].c_str());

    int p = roundToEven(pair_count);
    n = roundToParallelism(n, p);
    CountDownLatch finish_latch(p * 2);
    std::vector<caf::actor> actors(p * 2);

    for (int i = 0; i < p; i++) {
        auto actor1 = system.spawn(pingThroughputActorFun, &finish_latch, n / p / 2);
        auto actor2 = system.spawn(pingThroughputActorFun, &finish_latch, n / p / 2);

        actors.push_back(actor1);
        actors.push_back(actor2);
    }

    auto start = std::chrono::high_resolution_clock::now();

    for (int i = 0; i < actors.size(); i += 2) {
        caf::anon_send(actors[i], PingThroughputMessage{actors[i + 1]});
    }
    finish_latch.await();

    auto spent_time = std::chrono::duration_cast<std::chrono::nanoseconds>(
            std::chrono::high_resolution_clock::now() - start).count();

    std::cout << "Ping throughput:" << std::endl;
    std::cout << "\t" << n << " ops" << std::endl;
    std::cout << "\t" << p << " pairs" << std::endl;
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
    for (int i = 0; i < n; i++) {
        auto actor = system.spawn(miniFun, n);
        actor_list.push_back(actor);
    }
    auto spent_time = std::chrono::duration_cast<std::chrono::nanoseconds>(
            std::chrono::high_resolution_clock::now() - start).count();


    for (auto actor : actor_list) {
        // std::cout << "actor id: " << actor->id() << std::endl;
        // std::cout << "actor address: " << caf::to_string(actor->address()) << std::endl;
        caf::actor_cast<caf::event_based_actor *>(actor)->quit();
    }

    std::cout << "Initiation:" << std::endl;
    std::cout << "\t" << n << " ops" << std::endl;
    std::cout << "\t" << spent_time << " ns" << std::endl;
    std::cout << "\t" << (n * 1000'000'000L / spent_time) << " ops/s" << std::endl;
}

void handleSingleProducerSending(caf::actor_system &system, std::vector<std::string> &args) {
    int n = std::atoi(args[1].c_str());

    CountDownLatch finish_latch(1);
    auto actor = system.spawn(countActorFun, &finish_latch, n);

    EmptyMessage empty_message;
    auto start = std::chrono::high_resolution_clock::now();
    for (int i = 0; i < n; i++) {
        caf::anon_send(actor, empty_message);
    }
    finish_latch.await();
    auto spent_time = std::chrono::duration_cast<std::chrono::nanoseconds>(
            std::chrono::high_resolution_clock::now() - start).count();

    std::cout << "Single-producer sending:" << std::endl;
    std::cout << "\t" << n << " ops" << std::endl;
    std::cout << "\t" << spent_time << " ns" << std::endl;
    std::cout << "\t" << (n * 1000'000'000L / spent_time) << " ops/s" << std::endl;
}

void handleMultiProducerSending(caf::actor_system &system, std::vector<std::string> &args) {
    int parallelism = 10;
    if (args.size() > 2) {
        parallelism = std::atoi(args[2].c_str());;
    }
    int n = std::atoi(args[1].c_str()) / parallelism * parallelism;
    int num_of_each_message = n / parallelism;

    CyclicBarrier barrier(parallelism + 1);
    CountDownLatch finish_latch(1);
    std::vector<std::thread *> thread_vector;
    EmptyMessage empty_message;

    auto actor = system.spawn(countActorFun, &finish_latch, n);
    for (int i = 0; i < parallelism; i++) {
        std::thread *th =
                new std::thread([&]() {
                    // print_out("[INFO] thread #", " starts", th->get_id());
                    try {
                        barrier.await();

                        for (int j = 0; j < num_of_each_message; j++) {
                            caf::anon_send(actor, empty_message);
                        }
                    } catch (const std::exception &e) {
                        std::cout << e.what() << std::endl;
                    }
                });
        //TODO： GC new 在堆上
        thread_vector.emplace_back(th);
    }

    auto start = std::chrono::high_resolution_clock::now();
    barrier.await();
    finish_latch.await();
    auto spent_time = std::chrono::duration_cast<std::chrono::nanoseconds>(
            std::chrono::high_resolution_clock::now() - start).count();

    std::cout << "Multi-producer sending:" << std::endl;
    std::cout << "\t" << n << " ops" << std::endl;
    std::cout << "\t" << spent_time << " ns" << std::endl;
    std::cout << "\t" << (n * 1000'000'000L / spent_time) << " ops/s" << std::endl;
}

void handleMaxThroughput(caf::actor_system &system, std::vector<std::string> &args) {
    int parallelism = 10;
    if (args.size() > 2) {
        parallelism = std::atoi(args[2].c_str());;
    }
    int n = std::atoi(args[1].c_str()) / parallelism * parallelism;
    int num_of_each_message = n / parallelism;

    CyclicBarrier barrier(parallelism + 1);
    CountDownLatch finish_latch(parallelism);
    std::vector<std::thread *> thread_vector;
    EmptyMessage empty_message;

    for (int i = 0; i < parallelism; i++) {
        std::thread *th =
                new std::thread([&]() {
                    // print_out("[INFO] thread #", " starts", th->get_id());
                    try {
                        auto actor = system.spawn(countActorFun, &finish_latch, num_of_each_message);
                        barrier.await();

                        for (int j = 0; j < num_of_each_message; j++) {
                            caf::anon_send(actor, empty_message);
                        }
                    } catch (const std::exception &e) {
                        std::cout << e.what() << std::endl;
                    }
                });
        // TODO：
        thread_vector.emplace_back(th);
    }

    auto start = std::chrono::high_resolution_clock::now();
    barrier.await();
    finish_latch.await();
    auto spent_time = std::chrono::duration_cast<std::chrono::nanoseconds>(
            std::chrono::high_resolution_clock::now() - start).count();

    std::cout << "Max throughput:" << std::endl;
    std::cout << "\t" << n << " ops" << std::endl;
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

        if (command == "multi-producer-sending") {
            //if (command == "multi-producer-sending") {
            handleMultiProducerSending(system, args);
            continue;
        }

        if (command == "max-throughput") {
            //if (command == "multi-producer-sending") {
            handleMaxThroughput(system, args);
            continue;
        }

        if (command == "ping-latency") {
            handlePingLatency(system, args);
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
