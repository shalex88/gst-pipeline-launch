#include "CommandDispatcher.h"
#include <utility>

CommandDispatcher::CommandDispatcher(std::shared_ptr<Scheduler> scheduler) : scheduler_(std::move(scheduler)) {}

void CommandDispatcher::registerCommand(const std::string& command_name, const std::shared_ptr<CommandInterface>& command) {
    std::lock_guard<std::mutex> lock(map_mutex_);
    if (command_map_.find(command_name) == command_map_.end()) {
        command_map_[command_name] = command;
    } else {
        LOG_ERROR("Command '{}' is already registered", command_name);
    }
}

void CommandDispatcher::dispatchCommand(std::shared_ptr<InputInterface::Requester> requester, const std::string& command_name) {
    {
        std::lock_guard<std::mutex> lock(map_mutex_);
        auto it = command_map_.find(command_name);
        if (it != command_map_.end()) {
            scheduler_->enqueueTask(std::move(requester), it->second);
        } else {
            requester->source->sendResponse(requester, "Nack");
            LOG_ERROR("Unknown command");
        }
    }
}

void CommandDispatcher::dispatchCommand(const std::string& command_name) {
    {
        std::lock_guard<std::mutex> lock(map_mutex_);
        auto it = command_map_.find(command_name);
        if (it != command_map_.end()) {
            scheduler_->enqueueTask(it->second);
        } else {
            LOG_ERROR("Unknown command");
        }
    }
}
