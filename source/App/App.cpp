#include "App.h"
#include <csignal>
#include <filesystem>
#include "Gstreamer/Gstreamer.h"
#include "Gstreamer/PipelineCommands.h"
#include "AppInputs/MessageServer.h"
#include "Network/TcpNetworkManager.h"
#include "TasksManager/CommandDispatcher.h"
#include "TasksManager/Scheduler.h"
#include "App/SignalHandler.h"

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

void App::run(const AppConfig& config) {
     SignalHandler::setupSignalHandling();

    auto scheduler = std::make_shared<Scheduler>();
    scheduler->init();

    auto pipeline_file = get_pipeline_file_path(config.input_file);
    auto gstreamer = std::make_shared<Gstreamer>(pipeline_file);

     auto dispatcher = std::make_shared<CommandDispatcher>(scheduler);

     dispatcher->registerCommand("enable", std::make_shared<StartOptionalElementsCommand>(gstreamer));
     dispatcher->registerCommand("disable", std::make_shared<StopOptionalElementsCommand>(gstreamer));
     dispatcher->registerCommand("stop", std::make_shared<StopPipelineCommand>(gstreamer));

     constexpr int tcp_server_port = 12345;
     auto network_manager = std::make_shared<TcpNetworkManager>(tcp_server_port);

     auto tcp_server = std::make_shared<MessageServer>(dispatcher, network_manager);
     tcp_server->init();


    gstreamer->play(); // Blocking call

    LOG_TRACE("Main thread stopped");
}
