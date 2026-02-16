#include "app/Application.h"
#include "common/logger/Logger.h"

// void custom_log_handler(GstDebugCategory* category, GstDebugLevel level, const gchar* file, const gchar* function, gint line, GObject* object, GstDebugMessage* message, gpointer user_data) {
//     const gchar* log_message = gst_debug_message_get(message);
//     const gchar* object_name = "";
//
//     if (object && GST_IS_PAD(object)) {
//         const gchar* parent_name = GST_OBJECT_PARENT(object) ? GST_OBJECT_NAME(GST_OBJECT_PARENT(object)) : "unknown";
//         const gchar* pad_name = GST_OBJECT_NAME(object);
//         object_name = g_strdup_printf("%s:%s", parent_name, pad_name);
//     }
//
//     std::string log_print = fmt::format("[{}()<{}>] {}", function, object_name, log_message);
//
//     switch (level) {
//         case GST_LEVEL_ERROR:
//             LOG_ERROR("{}", log_print);
//             break;
//         case GST_LEVEL_WARNING:
//             LOG_WARN("{}", log_print);
//             break;
//         case GST_LEVEL_INFO:
//             LOG_INFO("{}", log_print);
//             break;
//         case GST_LEVEL_DEBUG:
//             LOG_DEBUG("{}", log_print);
//             break;
//         case GST_LEVEL_TRACE:
//             LOG_TRACE("{}", log_print);
//             break;
//         default:
//             break;
//     }
// }
//
// // TODO: place gst debug configuration in a separate file
// void configure_logger(const bool verbose) {
//     gst_debug_add_log_function(custom_log_handler, nullptr, nullptr);
//     gst_debug_remove_log_function(gst_debug_log_default);
//
//     if (verbose) {
//         SET_LOG_LEVEL(LoggerInterface::LogLevel::Debug);
//         // gst_debug_set_default_threshold(GST_LEVEL_INFO);
//         // gst_debug_set_threshold_from_string("nvmsgconv:5,GST_CAPS:4", TRUE);
//     } else {
//         SET_LOG_LEVEL(LoggerInterface::LogLevel::Info);
//     }
// }

int main(int argc, char* argv[]) {
    service::app::Application app(argc, argv);

    if (const auto result = app.initialize(); result.isError()) {
        LOG_ERROR("Initialization failed: {}", result.error());
        return EXIT_FAILURE;
    }

    if (const auto result = app.start(); result.isError()) {
        LOG_ERROR("Failed to start: {}", result.error());
        return EXIT_FAILURE;
    }

    app.run(); // Blocking call

    if (const auto result = app.stop(); result.isError()) {
        LOG_ERROR("Shutdown error: {}", result.error());
        return EXIT_FAILURE;
    }

    LOG_INFO("Stopped");
    return EXIT_SUCCESS;
}