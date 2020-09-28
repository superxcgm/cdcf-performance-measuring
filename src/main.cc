#include <caf/all.hpp>
#include <cdcf/actor_system.h>
#include <cdcf/logger.h>

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
    std::transform(str.begin(), str.end(), std::back_inserter(result), ::toLower);
    return result;
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
        std::string command = args[0];

        if (command == "h" || command == "help") {
            // print help message
            continue;
        }

        if (command == "q" || command == "quit") {
            exit(0);
        }

        if (command == "enqueueing") {
            // Todo:
        }
    }
}

CAF_MAIN(caf::io::middleman)
