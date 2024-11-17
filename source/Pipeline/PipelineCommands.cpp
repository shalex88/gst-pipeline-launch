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

void StopPipelineCommand::execute(const std::shared_ptr<InputInterface::Requester> requester) {
    requester->source->sendResponse(requester, "Ack");
    component_->stop();
}
