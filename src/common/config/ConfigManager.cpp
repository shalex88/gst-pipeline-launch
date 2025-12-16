#include "ConfigManager.h"

#include <set>
#include <yaml-cpp/yaml.h>

namespace service::common {
    void ApiConfig::validate() const {
        static const std::set<std::string> valid_apis{"grpc"};

        if (api.empty()) {
            throw std::runtime_error("API type cannot be empty");
        }
        if (!valid_apis.contains(api)) {
            throw std::runtime_error("Invalid API type: " + api);
        }
        if (server_address.empty()) {
            throw std::runtime_error("Server address cannot be empty");
        }
        if (server_address.find(':') == std::string::npos) {
            throw std::runtime_error("Server address must include port (format: host:port)");
        }
    }

    void CoreConfig::validate() const {
        static const std::set<std::string> valid_cameras{"core"};

        if (camera.empty()) {
            throw std::runtime_error("Camera type cannot be empty");
        }
    }

    void AppConfig::validate() const {
        static const std::set<std::string> valid_log_levels{"trace", "debug", "info", "warn", "error", "critical"};

        api_config.validate();
        core_config.validate();

        if (log_level.empty()) {
            throw std::runtime_error("Log level cannot be empty");
        }
        if (!valid_log_levels.contains(log_level)) {
            throw std::runtime_error("Invalid log level: " + log_level);
        }
        if (name.empty()) {
            throw std::runtime_error("App name cannot be empty");
        }

        if (api_config.server_address == core_config.camera) {
            throw std::runtime_error("API server address cannot be the same as camera type");
        }
    }

    ConfigManager::ConfigManager(const std::string& filename) : app_config_(std::make_unique<AppConfig>()) {
        if (!std::filesystem::exists(filename)) {
            throw std::runtime_error("Configuration file does not exist: " + filename);
        }
        loadFromFile(filename);
        validateConfiguration();
    }

    void ConfigManager::loadFromFile(const std::filesystem::path& filename) const {
        try {
            if (const YAML::Node config = YAML::LoadFile(filename); config["app"]) {
                const auto& app_node = config["app"];
                loadApiConfig(app_node);
                loadCoreConfig(app_node);
                loadAppConfig(app_node);
            }
        }
        catch (const YAML::Exception& e) {
            throw std::runtime_error("YAML parsing error: " + std::string(e.what()));
        }
    }

    void ConfigManager::loadApiConfig(const YAML::Node& app_node) const {
        if (app_node["api"]) {
            const auto& api_node = app_node["api"];
            if (api_node["api_type"]) {
                app_config_->api_config.api = api_node["api_type"].as<std::string>();
            }
            if (api_node["server_address"]) {
                app_config_->api_config.server_address = api_node["server_address"].as<std::string>();
            }
        }
    }

    void ConfigManager::loadCoreConfig(const YAML::Node& app_node) const {
        if (app_node["core"]) {
            if (const auto& core_node = app_node["core"]; core_node["camera"]) {
                app_config_->core_config.camera = core_node["camera"].as<std::string>();
            }
        }
    }

    void ConfigManager::loadAppConfig(const YAML::Node& app_node) const {
        if (app_node["log_level"]) {
            app_config_->log_level = app_node["log_level"].as<std::string>();
        }
        if (app_node["name"]) {
            app_config_->name = app_node["name"].as<std::string>();
        }
    }

    const ApiConfig& ConfigManager::getApiConfig() const {
        return app_config_->api_config;
    }

    const CoreConfig& ConfigManager::getCoreConfig() const {
        return app_config_->core_config;
    }

    const std::string& ConfigManager::getLogLevel() const {
        return app_config_->log_level;
    }

    const std::string& ConfigManager::getAppName() const {
        return app_config_->name;
    }

    void ConfigManager::validateConfiguration() const {
        if (!app_config_) {
            throw std::runtime_error("Configuration not initialized");
        }
        app_config_->validate();
    }
}
