#pragma once

#include <atomic>
#include <memory>
#include <string>

#include "common/types/Result.h"

namespace service::common {
    class ConfigManager;
}

namespace service::infrastructure {
    class ICamera;
}

namespace service::core {
    class ICore;
}

namespace service::api {
    class ApiController;
}

namespace service::app {

    class Application final {
    public:
        Application(int argc, char* argv[]);
        ~Application();

        Application(const Application&) = delete;
        Application& operator=(const Application&) = delete;
        Application(Application&&) = delete;
        Application& operator=(Application&&) = delete;

        Result<void> initialize();
        Result<void> start() const;
        void run() const;
        Result<void> stop() const;

        void requestShutdown();

    private:
        void parseArguments(int argc, char* argv[]);
        void setupSignalHandlers();

        std::atomic<bool> shutdown_requested_{false};
        std::string config_file_{"../config/config.yaml"};

        std::unique_ptr<common::ConfigManager> config_{};
        std::unique_ptr<api::ApiController> api_controller_{};
    };

}
