#ifndef PERIPHERY_MANAGER_PIPELINECOMMANDS_H
#define PERIPHERY_MANAGER_PIPELINECOMMANDS_H

#include "TasksManager/CommandInterface.h"
#include "Gstreamer.h"

class StartOptionalElementsCommand : public CommandInterface {
public:
    explicit StartOptionalElementsCommand(std::shared_ptr<Gstreamer> sensor) : component_(std::move(sensor)) {}
    void execute(std::shared_ptr<InputInterface::Requester> requester) override;
    ~StartOptionalElementsCommand() override = default;

private:
    std::shared_ptr<Gstreamer> component_;
};

class StopOptionalElementsCommand : public CommandInterface {
public:
    explicit StopOptionalElementsCommand(std::shared_ptr<Gstreamer> sensor) : component_(std::move(sensor)) {}
    void execute(std::shared_ptr<InputInterface::Requester> requester) override;
    ~StopOptionalElementsCommand() override = default;

private:
    std::shared_ptr<Gstreamer> component_;
};

class StopPipelineCommand : public CommandInterface {
public:
    explicit StopPipelineCommand(std::shared_ptr<Gstreamer> sensor) : component_(std::move(sensor)) {}
    void execute(std::shared_ptr<InputInterface::Requester> requester) override;
    ~StopPipelineCommand() override = default;

private:
    std::shared_ptr<Gstreamer> component_;
};

#endif //PERIPHERY_MANAGER_PIPELINECOMMANDS_H
