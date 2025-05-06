#include <filesystem>
#include "Logger/Logger.h"
#include "cxxopts.hpp"
#include <gst/gst.h>
#include "App/App.h"

AppConfig parse_command_line_arguments(const int argc, const char* argv[]) {
    cxxopts::Options options(argv[0], "Gstreamer runner");
    options.add_options()
        ("i,input", "Input YAML pipeline file", cxxopts::value<std::filesystem::path>()->default_value("../resources/pipeline.yaml"))
        ("p,port", "Port for TCP socket", cxxopts::value<unsigned int>()->default_value("12345"))
        ("v,verbose", "Enable verbose logging", cxxopts::value<bool>()->default_value("false"))
        ("h,help", "Print usage");

    const auto result = options.parse(argc, argv);

    if (result.count("help")) {
        std::cout << options.help() << std::endl;
        exit(EXIT_SUCCESS);
    }

    AppConfig config {
        .input_file = result["input"].as<std::filesystem::path>(),
        .port = result["port"].as<unsigned int>(),
        .verbose = result["verbose"].as<bool>()
    };

    return config;
}

// TODO: place this function in a appropriate separate file
void custom_log_handler(GstDebugCategory* category, GstDebugLevel level, const gchar* file, const gchar* function, gint line, GObject* object, GstDebugMessage* message, gpointer user_data) {
    const gchar* log_message = gst_debug_message_get(message);
    const gchar* object_name = "";
  
    if (object && GST_IS_PAD(object)) {
        const gchar* parent_name = GST_OBJECT_PARENT(object) ? GST_OBJECT_NAME(GST_OBJECT_PARENT(object)) : "unknown";
        const gchar* pad_name = GST_OBJECT_NAME(object);
        object_name = g_strdup_printf("%s:%s", parent_name, pad_name);
    }
    
    std::string log_print = fmt::format("[{}()<{}>] {}", function, object_name, log_message);

    switch (level) {
        case GST_LEVEL_ERROR:
            LOG_ERROR("{}", log_print);
            break;
        case GST_LEVEL_WARNING:
            LOG_WARN("{}", log_print);
            break;
        case GST_LEVEL_INFO:
            LOG_INFO("{}", log_print);
            break;
        case GST_LEVEL_DEBUG:
            LOG_DEBUG("{}", log_print);
            break;
        case GST_LEVEL_TRACE:
            LOG_TRACE("{}", log_print);
            break;
        default:
            break;
    }
}

// TODO: place gst debug configuration in a separate file
void configure_logger(const bool verbose) {
    gst_debug_add_log_function(custom_log_handler, nullptr, nullptr);
    gst_debug_remove_log_function(gst_debug_log_default);

    if (verbose) {
        SET_LOG_LEVEL(LoggerInterface::LogLevel::Trace);
        // gst_debug_set_threshold_from_string("GST_SCHEDULING:5", TRUE);
        gst_debug_set_default_threshold(GST_LEVEL_INFO);
    } else {
        SET_LOG_LEVEL(LoggerInterface::LogLevel::Info);
    }
}

int main(const int argc, const char* argv[]) {
    const AppConfig config = parse_command_line_arguments(argc, argv);
    configure_logger(config.verbose);

    LOG_TRACE("{} {}.{}.{}", APP_NAME, APP_VERSION_MAJOR, APP_VERSION_MINOR, APP_VERSION_PATCH);

    try {
        if (App::run(config) != EXIT_SUCCESS) {
            return EXIT_FAILURE;
        }
    } catch (const std::exception& e) {
        LOG_CRITICAL("{}", e.what());
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}