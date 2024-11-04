#include <memory>
#include <iostream>
#include <filesystem>
#include <thread>
#include <chrono>
#include "Logger/Logger.h"
#include "Gstreamer/Gstreamer.h"
#include "cxxopts.hpp"

void configureLogger(bool verbose) {
    if (verbose) {
        Logger::SET_LOG_LEVEL(LoggerInterface::LogLevel::Trace);
        LOG_INFO("Verbose logging enabled.");
    } else {
        Logger::SET_LOG_LEVEL(LoggerInterface::LogLevel::Info);
    }
}

std::filesystem::path getPipelineFilePath(const cxxopts::ParseResult& result) {
    std::filesystem::path pipeline_file = std::filesystem::current_path() / result["input"].as<std::filesystem::path>();
    if (!exists(pipeline_file)) {
        LOG_ERROR("Pipeline file not found at: {}", pipeline_file.string());
        throw std::runtime_error("Pipeline file not found");
    }
    return pipeline_file;
}

void userInputThread(std::shared_ptr<Gstreamer> gstreamer, std::atomic<bool>& keep_running) {
    LOG_INFO("Enter 'e' to enable optional element, 'd' to disable optional element, and 'q' to quit:");

    while (keep_running) {
        if (std::cin.rdbuf()->in_avail() > 0) {
            char command = 0;
            std::cin >> command;

            switch (command) {
                case 'e':
                    gstreamer->enable_optional_pipeline_elements();
                    LOG_INFO("Enabled optional pipeline elements.");
                    break;
                case 'd':
                    gstreamer->disable_optional_pipeline_elements();
                    LOG_INFO("Disabled optional pipeline elements.");
                    break;
                case 'q':
                    gstreamer->stop();
                    keep_running = false;  // Signal to stop the thread
                    LOG_INFO("Stopping pipeline and exiting.");
                    return;
                default:
                    LOG_WARN("Invalid command. Use 'e', 'd', or 'q'.");
                    break;
            }
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(100)); // Prevent busy-waiting
    }
}

int main(const int argc, char* argv[]) {
    // Parse command-line options
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

    try {
        configureLogger(result["verbose"].as<bool>());

        auto pipeline_file = getPipelineFilePath(result);
        auto gstreamer = std::make_shared<Gstreamer>(pipeline_file);
        std::atomic<bool> keep_running{true};

        std::thread input_thread(userInputThread, gstreamer, std::ref(keep_running));

        gstreamer->play();

        keep_running = false; // Signal the input thread to stop once play() returns
        input_thread.join();
    } catch (const std::exception& e) {
        LOG_ERROR("Error: {}", e.what());
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
