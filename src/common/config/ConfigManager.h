#pragma once

#include <filesystem>
#include <memory>
#include <string>
#include <yaml-cpp/yaml.h>

namespace service::common {
    struct ApiConfig {
        std::string api;
        std::string server_address;

        void validate() const;
    };

    struct CoreConfig {
        std::string camera;

        void validate() const;
    };

    struct AppConfig {
        ApiConfig api_config;
        CoreConfig core_config;
        std::string log_level;
        std::string name;

        void validate() const;
    };

    class ConfigManager {
    public:
        ConfigManager() = delete;
        explicit ConfigManager(const std::string& filename);

        const ApiConfig& getApiConfig() const;
        const CoreConfig& getCoreConfig() const;
        const std::string& getLogLevel() const;
        const std::string& getAppName() const;

    private:
        void loadFromFile(const std::filesystem::path& filename) const;
        void validateConfiguration() const;
        void loadApiConfig(const YAML::Node& app_node) const;
        void loadCoreConfig(const YAML::Node& app_node) const;
        void loadAppConfig(const YAML::Node& app_node) const;

        std::unique_ptr<AppConfig> app_config_;
    };
}
