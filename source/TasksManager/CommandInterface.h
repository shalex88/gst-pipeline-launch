#ifndef PERIPHERY_MANAGER_COMMANDINTERFACE_H
#define PERIPHERY_MANAGER_COMMANDINTERFACE_H

#include <utility>
#include <csignal>
#include "Logger/Logger.h"
#include "AppInputs/InputInterface.h"

class CommandInterface {
public:
    virtual ~CommandInterface() = default;
    virtual void execute(std::shared_ptr<InputInterface::Requester> requester) = 0;
};

class CommandFake : public CommandInterface {
public:
    ~CommandFake() override = default;
    void execute(std::shared_ptr<InputInterface::Requester> requester) override {
        requester->source->sendResponse(requester, "Ack");
    }
};

#endif //PERIPHERY_MANAGER_COMMANDINTERFACE_H
