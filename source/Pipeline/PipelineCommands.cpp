#include "PipelineCommands.h"

void EnableOptionalElementCommand::execute(const std::shared_ptr<InputInterface::Requester> requester) {
    std::string response = "Ack";
    if (auto ec = component_->enableOptionalPipelineElement(element_name_)) {
        response = "Nack";
    }
    requester->source->sendResponse(requester, response);
}

void DisableOptionalElementCommand::execute(const std::shared_ptr<InputInterface::Requester> requester) {
    std::string response = "Ack";
    if (auto ec = component_->disableOptionalPipelineElement(element_name_)) {
        response = "Nack";
    }
    requester->source->sendResponse(requester, response);
}

void EnableOptionalBranchCommand::execute(const std::shared_ptr<InputInterface::Requester> requester) {
    std::string response = "Ack";
    if (auto ec = component_->enableOptionalPipelineBranch(branch_name_)) {
        response = "Nack";
    }
    requester->source->sendResponse(requester, response);
}

void DisableOptionalBranchCommand::execute(const std::shared_ptr<InputInterface::Requester> requester) {
    std::string response = "Ack";
    if (auto ec = component_->disableOptionalPipelineBranch(branch_name_)) {
        response = "Nack";
    }
    requester->source->sendResponse(requester, response);
}

void EnableAllOptionalElementsCommand::execute(const std::shared_ptr<InputInterface::Requester> requester) {
    std::string response = "Ack";
    if (auto ec = component_->enableAllOptionalPipelineElements()) {
        response = "Nack";
    }
    requester->source->sendResponse(requester, response);
}

void DisableAllOptionalElementsCommand::execute(const std::shared_ptr<InputInterface::Requester> requester) {
    std::string response = "Ack";
    if (auto ec = component_->disableAllOptionalPipelineElements()) {
        response = "Nack";
    }
    requester->source->sendResponse(requester, response);
}

void EnableAllOptionalBranchesCommand::execute(const std::shared_ptr<InputInterface::Requester> requester) {
    std::string response = "Ack";
    if (auto ec = component_->enableAllOptionalPipelineBranches()) {
        response = "Nack";
    }
    requester->source->sendResponse(requester, response);
}

void DisableAllOptionalBranchesCommand::execute(const std::shared_ptr<InputInterface::Requester> requester) {
    std::string response = "Ack";
    if (auto ec = component_->disableAllOptionalPipelineBranches()) {
        response = "Nack";
    }
    requester->source->sendResponse(requester, response);
}

void StopPipelineCommand::execute(const std::shared_ptr<InputInterface::Requester> requester) {
    requester->source->sendResponse(requester, "Ack");
    component_->stop();
}
