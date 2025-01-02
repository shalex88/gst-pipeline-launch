#include "App.h"
#include <csignal>
#include <filesystem>
#include "Pipeline/PipelineManager.h"
#include "Pipeline/PipelineCommands.h"
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
    }

    LOG_INFO("Provided pipeline file: {}", pipeline_file.string());

    return pipeline_file;
}

int App::run(const AppConfig& config) {
    // SignalHandler::setupSignalHandling(); //FIXME:

    if (config.verbose) {
        // gst_debug_set_default_threshold(GST_LEVEL_INFO);
    }

    auto scheduler = std::make_shared<Scheduler>();
    scheduler->init();

    auto pipeline_file = get_pipeline_file_path(config.input_file);
    auto pipeline_manager = std::make_shared<PipelineManager>(pipeline_file);

    auto dispatcher = std::make_shared<CommandDispatcher>(scheduler);


    // TODO: Fix cretain commands for multiple elements with the same name
    dispatcher->registerCommand("test",
                                std::make_shared<CommandFake>());
    dispatcher->registerCommand("enable_elements",
                                std::make_shared<EnableAllOptionalElementsCommand>(pipeline_manager));
    dispatcher->registerCommand("disable_elements",
                                std::make_shared<DisableAllOptionalElementsCommand>(pipeline_manager));
    dispatcher->registerCommand("enable_branches",
                                std::make_shared<EnableAllOptionalBranchesCommand>(pipeline_manager));
    dispatcher->registerCommand("disable_branches",
                                std::make_shared<DisableAllOptionalBranchesCommand>(pipeline_manager));
    dispatcher->registerCommand("stop",
                                std::make_shared<StopPipelineCommand>(pipeline_manager));

    for (const auto& optional_element_name: pipeline_manager->getOptionalPipelineElementsNames()) {
        std::string enable_command_name = "enable_" + optional_element_name;
        std::string disable_command_name = "disable_" + optional_element_name;
        dispatcher->registerCommand(enable_command_name,
                                    std::make_shared<EnableOptionalElementCommand>(
                                        pipeline_manager, optional_element_name));
        dispatcher->registerCommand(disable_command_name,
                                    std::make_shared<DisableOptionalElementCommand>(
                                        pipeline_manager, optional_element_name));
    }

    for (const auto& optional_branch_name: pipeline_manager->getOptionalPipelineBranchesNames()) {
        std::string enable_command_name = "enable_" + optional_branch_name;
        std::string disable_command_name = "disable_" + optional_branch_name;
        dispatcher->registerCommand(enable_command_name,
                                    std::make_shared<EnableOptionalBranchCommand>(
                                        pipeline_manager, optional_branch_name));
        dispatcher->registerCommand(disable_command_name,
                                    std::make_shared<DisableOptionalBranchCommand>(
                                        pipeline_manager, optional_branch_name));
    }

    auto network_manager = std::make_shared<TcpNetworkManager>(config.port);

    const auto tcp_server = std::make_shared<MessageServer>(dispatcher, network_manager);
    tcp_server->init();

    if (auto ec = pipeline_manager->play()) { // Blocking call
        LOG_ERROR("Failed to play pipeline");
        return EXIT_FAILURE;
    }

    LOG_TRACE("Main thread stopped");
    return EXIT_SUCCESS;
}
