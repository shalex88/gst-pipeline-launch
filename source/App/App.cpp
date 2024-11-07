#include "App.h"
// #include "AppInputs/MessageServer.h"
#include "App/SignalHandler.h"
// #include "Network/TcpNetworkManager.h"
// #include "TemperatureSensor/TemperatureSensorProtocol.h"
// #include "TemperatureSensor/TemperatureSensorCommands.h"
#include <csignal>
#include <filesystem>
#include "Gstreamer/Gstreamer.h"

std::atomic<bool> App::keep_running_ = true;

void App::shutdown() {
    keep_running_ = false;
    LOG_INFO("Terminating...");
}


std::filesystem::path get_pipeline_file_path(const std::filesystem::path& file_path) {
    std::filesystem::path pipeline_file = std::filesystem::current_path() / file_path;
    if (!exists(pipeline_file)) {
        LOG_ERROR("Pipeline file not found at: {}", pipeline_file.string());
        throw std::runtime_error("Pipeline file not found");
    } else {
        LOG_INFO("Provided pipeline file: {}", pipeline_file.string());
    }

    return pipeline_file;
}

void user_input_thread(const std::shared_ptr<Gstreamer>& gstreamer, std::atomic<bool>& keep_running) {
    LOG_INFO("Enter 'e' to enable optional element, 'd' to disable optional element, and 'q' to quit:");

    while (keep_running) {
        fd_set read_fds;
        FD_ZERO(&read_fds);
        FD_SET(STDIN_FILENO, &read_fds);

        struct timeval timeout {};
        timeout.tv_sec = 0;
        timeout.tv_usec = 100000; // 100 milliseconds

        int result = select(STDIN_FILENO + 1, &read_fds, nullptr, nullptr, &timeout);

        if (result > 0 && FD_ISSET(STDIN_FILENO, &read_fds)) {
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
                LOG_INFO("Stopped pipeline and exiting.");
                return;
                default:
                    LOG_WARN("Invalid command. Use 'e', 'd', or 'q'.");
                break;
            }
        }
    }
}

void App::run(const AppConfig& config) {
//     SignalHandler::setupSignalHandling();
//
//     auto hw_interface = std::make_shared<HwFake>();
    // auto hw_interface = std::make_shared<TcpClient>("127.0.0.1", 12345);
//     auto protocol_interface = std::make_shared<TemperatureSensorProtocol>();
//     auto temp_sensor = std::make_shared<TemperatureSensor>(hw_interface, protocol_interface);
//     temp_sensor->init();
//
//     const uint32_t cores_num = sysconf(_SC_NPROCESSORS_ONLN);
//     LOG_INFO("CPU cores number is {}", cores_num);
//     auto scheduler = std::make_shared<Scheduler>(cores_num);
//     scheduler->init();
//
//     auto dispatcher = std::make_shared<CommandDispatcher>(scheduler);
//     dispatcher->registerCommand("stop", std::make_shared<StopCommand>());
//     dispatcher->registerCommand("temp", std::make_shared<GetTempCommand>(temp_sensor));
//     dispatcher->registerCommand("test", std::make_shared<CommandFake>());
//
//     const int tcp_server_port = 12345;
//     auto network_manager = std::make_shared<TcpNetworkManager>(tcp_server_port);
//
//     auto tcp_server = std::make_shared<MessageServer>(dispatcher, network_manager);
//     tcp_server->init();
//
    auto pipeline_file = get_pipeline_file_path(config.input_file);
    auto gstreamer = std::make_shared<Gstreamer>(pipeline_file);
    std::atomic<bool> keep_running{true};

    std::thread input_thread(user_input_thread, gstreamer, std::ref(keep_running));

    gstreamer->play(); // Blocking call

    keep_running = false; // Signal the input thread to stop once play() returns
    if (input_thread.joinable()) {
        input_thread.join();
    }

    LOG_DEBUG("Main thread stopped");
}
