#include <memory>
#include "Logger/Logger.h"
#include "Gstreamer/Gstreamer.h"
#include <filesystem>
#include "cxxopts.hpp"

int main(const int argc, char* argv[]) {
    cxxopts::Options options("gstreamer-core-app", "Gstreamer runner");

    options.add_options()
        ("i,input", "Input pipeline file", cxxopts::value<std::filesystem::path>()->default_value("../resources/pipeline.json"))
        ("v,verbose", "Enable verbose logging", cxxopts::value<bool>()->default_value("false"))
        ("h,help", "Print usage");

    auto result = options.parse(argc, argv);

    if (result.count("help")) {
        std::cout << options.help() << std::endl;

        return EXIT_SUCCESS;
    }

    if (result["verbose"].as<bool>()) {
        Logger::SET_LOG_LEVEL(LoggerInterface::LogLevel::Trace);
    } else {
        Logger::SET_LOG_LEVEL(LoggerInterface::LogLevel::Info);
    }

    std::filesystem::path pipeline_file = std::filesystem::current_path() / result["input"].as<std::filesystem::path>();
    auto gstreamer = std::make_shared<Gstreamer>(pipeline_file);

    std::thread user_input_thread([gstreamer]()  {
        LOG_INFO("Enter 'e' to enable optional element, 'd' to disable optional element, and 'q' to quit:");
        char command;
        while (true) {
            std::cin >> command;
            if (command == 'e') {
                gstreamer->enable_optional_pipeline_elements();
            } else if (command == 'd') {
                gstreamer->disable_optional_pipeline_elements();
            } else if (command == 'q') {
                gstreamer->stop();
                break;
            }
        }
    });

    gstreamer->play();

    user_input_thread.join();

    return EXIT_SUCCESS;
}
