#ifndef PERIPHERY_MANAGER_COMMANDDISPATCHER_H
#define PERIPHERY_MANAGER_COMMANDDISPATCHER_H

#include <unordered_map>
#include <string>
#include <memory>
#include "TasksManager/CommandInterface.h"
#include "TasksManager/Scheduler.h"
#include "AppInputs/InputInterface.h"

class CommandDispatcher {
public:
    explicit CommandDispatcher(std::shared_ptr<Scheduler> scheduler);
    ~CommandDispatcher() = default;
    void registerCommand(const std::string& command_name, const std::shared_ptr<CommandInterface>& command);
    void dispatchCommand(const std::string& command_name);
    void dispatchCommand(std::shared_ptr<InputInterface::Requester> requester, const std::string& command_name);
private:
    std::unordered_map<std::string, std::shared_ptr<CommandInterface>> command_map_;
    std::shared_ptr<Scheduler> scheduler_;
    std::mutex map_mutex_;
};

#endif //PERIPHERY_MANAGER_COMMANDDISPATCHER_H
