#ifndef PERIPHERY_MANAGER_PIPELINECOMMANDS_H
#define PERIPHERY_MANAGER_PIPELINECOMMANDS_H

#include <utility>

#include "TasksManager/CommandInterface.h"
#include "Gstreamer.h"

class EnableOptionalElementCommand : public CommandInterface {
public:
    explicit EnableOptionalElementCommand(std::shared_ptr<Gstreamer> sensor, std::string& element_name)
        : component_(std::move(sensor)), element_name_(std::move(element_name)) {}
    void execute(std::shared_ptr<InputInterface::Requester> requester) override;
    ~EnableOptionalElementCommand() override = default;

private:
    std::shared_ptr<Gstreamer> component_;
    std::string element_name_;
};

class DisableOptionalElementCommand : public CommandInterface {
public:
    explicit DisableOptionalElementCommand(std::shared_ptr<Gstreamer> sensor, std::string& element_name)
        : component_(std::move(sensor)), element_name_(std::move(element_name)) {}
    void execute(std::shared_ptr<InputInterface::Requester> requester) override;
    ~DisableOptionalElementCommand() override = default;

private:
    std::shared_ptr<Gstreamer> component_;
    std::string element_name_;
};

class EnableAllOptionalElementsCommand : public CommandInterface {
public:
    explicit EnableAllOptionalElementsCommand(std::shared_ptr<Gstreamer> sensor) : component_(std::move(sensor)) {}
    void execute(std::shared_ptr<InputInterface::Requester> requester) override;
    ~EnableAllOptionalElementsCommand() override = default;

private:
    std::shared_ptr<Gstreamer> component_;
};

class DisableAllOptionalElementsCommand : public CommandInterface {
public:
    explicit DisableAllOptionalElementsCommand(std::shared_ptr<Gstreamer> sensor) : component_(std::move(sensor)) {}
    void execute(std::shared_ptr<InputInterface::Requester> requester) override;
    ~DisableAllOptionalElementsCommand() override = default;

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
