#include "PipelineCommands.h"

void EnableOptionalElementCommand::execute(const std::shared_ptr<InputInterface::Requester> requester) {
    std::string response = "Ack";
    if (component_->enableOptionalPipelineElement(element_name_) != EXIT_SUCCESS) {
        response = "Nack";
    }
    requester->source->sendResponse(requester, response);
}

void DisableOptionalElementCommand::execute(const std::shared_ptr<InputInterface::Requester> requester) {
    std::string response = "Ack";
    if (component_->disableOptionalPipelineElement(element_name_) != EXIT_SUCCESS) {
        response = "Nack";
    }
    requester->source->sendResponse(requester, response);
}

void EnableOptionalBranchCommand::execute(const std::shared_ptr<InputInterface::Requester> requester) {
    std::string response = "Ack";
    if (component_->enableOptionalPipelineBranch(branch_name_) != EXIT_SUCCESS) {
        response = "Nack";
    }
    requester->source->sendResponse(requester, response);
}

void DisableOptionalBranchCommand::execute(const std::shared_ptr<InputInterface::Requester> requester) {
    std::string response = "Ack";
    if (component_->disableOptionalPipelineBranch(branch_name_) != EXIT_SUCCESS) {
        response = "Nack";
    }
    requester->source->sendResponse(requester, response);
}

void EnableAllOptionalElementsCommand::execute(const std::shared_ptr<InputInterface::Requester> requester) {
    std::string response = "Ack";
    if (component_->enableAllOptionalPipelineElements() != EXIT_SUCCESS) {
        response = "Nack";
    }
    requester->source->sendResponse(requester, response);
}

void DisableAllOptionalElementsCommand::execute(const std::shared_ptr<InputInterface::Requester> requester) {
    std::string response = "Ack";
    if (component_->disableAllOptionalPipelineElements() != EXIT_SUCCESS) {
        response = "Nack";
    }
    requester->source->sendResponse(requester, response);
}

void EnableAllOptionalBranchesCommand::execute(const std::shared_ptr<InputInterface::Requester> requester) {
    std::string response = "Ack";
    if (component_->enableAllOptionalPipelineBranches() != EXIT_SUCCESS) {
        response = "Nack";
    }
    requester->source->sendResponse(requester, response);
}

void DisableAllOptionalBranchesCommand::execute(const std::shared_ptr<InputInterface::Requester> requester) {
    std::string response = "Ack";
    if (component_->disableAllOptionalPipelineBranches() != EXIT_SUCCESS) {
        response = "Nack";
    }
    requester->source->sendResponse(requester, response);
}

void StopPipelineCommand::execute(const std::shared_ptr<InputInterface::Requester> requester) {
    requester->source->sendResponse(requester, "Ack");
    component_->stop();
}
