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

int roundToParallelism(int n, int parallelism) {
    return (n / parallelism) * parallelism;
}

int roundToEven(int x) {
    if ((x & 1) == 1) {
        x--;
    }
    return x;
}

long long handleEnqueueing(caf::actor_system &system, std::vector<std::string> &args) {
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
    return n * 1000'000'000L / spent_time;
}

long long handleDequeueing(caf::actor_system &system, std::vector<std::string> &args) {
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
    return n * 1000'000'000L / spent_time;
}

long long handleInitiation(caf::actor_system &system, std::vector<std::string> &args) {
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
        // caf::actor_cast<caf::event_based_actor *>(actor)->quit();
        caf::anon_send(actor, caf::exit_reason::kill);
    }

    std::cout << "Initiation:" << std::endl;
    std::cout << "\t" << n << " ops" << std::endl;
    std::cout << "\t" << spent_time << " ns" << std::endl;
    std::cout << "\t" << (n * 1000'000'000L / spent_time) << " ops/s" << std::endl;
    return n * 1000'000'000L / spent_time;
}

long long handleSingleProducerSending(caf::actor_system &system, std::vector<std::string> &args) {
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
    return n * 1000'000'000L / spent_time;
}

long long handleMultiProducerSending(caf::actor_system &system, std::vector<std::string> &args) {
    int parallelism = 1;
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
                    return;
                });
        //TODO： GC new 在堆上 shared point
        thread_vector.emplace_back(th);
        // th->detach();
    }

    auto start = std::chrono::high_resolution_clock::now();
    barrier.await();
    finish_latch.await();
    auto spent_time = std::chrono::duration_cast<std::chrono::nanoseconds>(
            std::chrono::high_resolution_clock::now() - start).count();

    //TODO： 这个解决方案能用，但不好，可选智能指针
    for( auto th : thread_vector) {
        th->join();
    }

    for( auto th : thread_vector) {
        delete th;
    }

    std::cout << "Multi-producer sending:" << std::endl;
    std::cout << "\t" << n << " ops" << std::endl;
    std::cout << "\t" << spent_time << " ns" << std::endl;
    std::cout << "\t" << (n * 1000'000'000L / spent_time) << " ops/s" << std::endl;
    return n * 1000'000'000L / spent_time;
}

long long handleMaxThroughput(caf::actor_system &system, std::vector<std::string> &args) {
    int parallelism = 1;
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
        // th->detach();
    }

    auto start = std::chrono::high_resolution_clock::now();
    barrier.await();
    finish_latch.await();
    auto spent_time = std::chrono::duration_cast<std::chrono::nanoseconds>(
            std::chrono::high_resolution_clock::now() - start).count();

    for( auto th : thread_vector) {
        th->join();
    }

    for( auto th : thread_vector) {
        delete th;
    }

    std::cout << "Max throughput:" << std::endl;
    std::cout << "\t" << n << " ops" << std::endl;
    std::cout << "\t" << spent_time << " ns" << std::endl;
    std::cout << "\t" << (n * 1000'000'000L / spent_time) << " ops/s" << std::endl;
    return n * 1000'000'000L / spent_time;
}

long long handlePingLatency(caf::actor_system &system, std::vector<std::string> &args) {
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
//        printf("\tp(%1.5f) = %8lld ns/op\n", x, temp_value);
        std::cout << "\tp(" << x << ") = " << temp_value << " ns/op" << std::endl;
    }
    return n* 1L;
}

long long handlePingThroughput(caf::actor_system &system, std::vector<std::string> &args) {
    int n = std::atoi(args[1].c_str());
    int pair_count;

    if( args.size() > 2) {
        pair_count = std::atoi(args[2].c_str());
    } else {
        pair_count = 10000;
    }

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
    return n * 1000'000'000L / spent_time;
}

[[noreturn]] void caf_main(caf::actor_system &system, const cdcf::actor_system::Config &config) {
    cdcf::Logger::Init(config);

//    long long (*ptr[8])(caf::actor_system &system, std::vector<std::string> &args){
//            handleEnqueueing, handleDequeueing, handleInitiation, handleSingleProducerSending,
//            handleMultiProducerSending, handleMaxThroughput, handlePingLatency, handlePingThroughput
//    };
//
//    std::vector<std::string> csv_context{
//            "Enqueueing, ", "Dequeueing, ", "Initiation, ", "SingleproducerSending, ", "MultiproducerSending, ",
//            "MaxThroughput, ", "PingLatency, ", "PingThroughput, "
//    };
//
//    for (int j = 0; j < 8; ++j) {
//        long long round_ouput = 0;
//        for (int i = 0; i < 10; ++i) {
//            std::string line = "NULL 100000";
//            auto args = split(line, ' ');
//            round_ouput += ptr[j](system, args);
//        }
//        csv_context[j] += std::to_string(round_ouput);
//    }
//
//    for (const auto &item : csv_context) {
//        std::cout << item << std::endl;
//    }

    std::streambuf* coutBuf = std::cout.rdbuf();
    std::ofstream of("out.txt");
    std::streambuf* fileBuf = of.rdbuf();
    std::cout.rdbuf(fileBuf);

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
            std::cout.rdbuf(coutBuf);
            std::cout << "exit" << std::endl;
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
            int pair_count = 10'000;
            handlePingLatency(system, args);
            continue;
        }

        if (command == "ping-throughput-10k") {
            args.push_back("10000");
            handlePingThroughput(system, args);
            continue;
        }
    }
}

CAF_MAIN(caf::io::middleman)
