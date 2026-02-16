#include "Application.h"

#include <chrono>
#include <csignal>
#include <iostream>
#include <thread>

#include <CLI/CLI.hpp>

#include "api/ApiController.h"
#include "api/ApiControllerFactory.h"
#include "common/config/ConfigManager.h"
#include "common/logger/Logger.h"
#include "core/CoreFactory.h"
#include "core/ICore.h"

namespace service::app {
    static Application* g_application_instance = nullptr;

    void signalHandler(int signal) {
        if (signal == SIGTERM || signal == SIGINT) {
            if (g_application_instance != nullptr) {
                g_application_instance->requestShutdown();
            }
        }
    }

    Application::Application(int argc, char* argv[]) {
        parseArguments(argc, argv);
        setupSignalHandlers();
    }

    Application::~Application() = default;

    void Application::parseArguments(int argc, char* argv[]) {
        CLI::App app{"A video streaming service", APP_NAME};

        bool show_version = false;

        app.add_flag("-v,--version", show_version, "Show version information");
        app.add_option("-c,--config", config_file_, "Configuration file path")->check(CLI::ExistingFile);

        try {
            app.parse(argc, argv);
        } catch (const CLI::ParseError& e) {
            std::exit(app.exit(e));
        }

        if (show_version) {
            std::cout << APP_NAME << " v" << APP_VERSION_MAJOR << "." << APP_VERSION_MINOR << "." << APP_VERSION_PATCH
                << APP_VERSION_DIRTY << "\n";
            std::exit(EXIT_SUCCESS);
        }
    }

    void Application::setupSignalHandlers() {
        g_application_instance = this;
        std::signal(SIGTERM, signalHandler);
        std::signal(SIGINT, signalHandler);
    }

    Result<void> Application::initialize() {
        try {
            config_ = std::make_unique<common::ConfigManager>(config_file_);

            CONFIGURE_LOGGER(config_->getAppName(), config_->getLogLevel());

            LOG_INFO("{} v{}.{}.{}{}", APP_NAME, APP_VERSION_MAJOR, APP_VERSION_MINOR, APP_VERSION_PATCH,
                     APP_VERSION_DIRTY);

            auto core = core::CoreFactory::createCore(config_->getCoreConfig());
            api_controller_ = api::ApiControllerFactory::createController(std::move(core), config_->getApiConfig());

            return Result<void>::success();
        } catch (const std::exception& e) {
            return Result<void>::error(e.what());
        }
    }

    Result<void> Application::start() const {
        if (api_controller_ == nullptr) {
            return Result<void>::error("Application not initialized");
        }

        if (const auto result = api_controller_->startAsync(); result.isError()) {
            return Result<void>::error(result.error());
        }

        LOG_INFO("Running...");
        return Result<void>::success();
    }

    void Application::run() const {
        while (api_controller_ != nullptr && api_controller_->isRunning() && !shutdown_requested_.load()) {
            std::this_thread::sleep_for(std::chrono::milliseconds(500));
        }

        if (shutdown_requested_.load()) {
            LOG_INFO("Stopping...");
        }
    }

    Result<void> Application::stop() const {
        if (api_controller_ == nullptr) {
            return Result<void>::success();
        }

        if (const auto result = api_controller_->stop(); result.isError()) {
            return Result<void>::error(result.error());
        }

        return Result<void>::success();
    }

    void Application::requestShutdown() {
        shutdown_requested_.store(true);
    }
}