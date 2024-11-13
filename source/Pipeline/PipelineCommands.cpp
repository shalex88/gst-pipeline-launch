#include "PipelineCommands.h"

void EnableOptionalElementCommand::execute(std::shared_ptr<InputInterface::Requester> requester) {
    std::string response = "Ack";
    if (component_->enableOptionalPipelineElement(element_name_) != EXIT_SUCCESS) {
        response = "Nack";
    }
    requester->source->sendResponse(requester, response);
}

void DisableOptionalElementCommand::execute(std::shared_ptr<InputInterface::Requester> requester) {
    std::string response = "Ack";
    if (component_->disableOptionalPipelineElement(element_name_) != EXIT_SUCCESS) {
        response = "Nack";
    }
    requester->source->sendResponse(requester, response);
}

void EnableAllOptionalElementsCommand::execute(std::shared_ptr<InputInterface::Requester> requester) {
    std::string response = "Ack";
    if (component_->enableAllOptionalPipelineElements() != EXIT_SUCCESS) {
        response = "Nack";
    }
    requester->source->sendResponse(requester, response);
}

void DisableAllOptionalElementsCommand::execute(std::shared_ptr<InputInterface::Requester> requester) {
    std::string response = "Ack";
    if (component_->disableAllOptionalPipelineElements() != EXIT_SUCCESS) {
        response = "Nack";
    }
    requester->source->sendResponse(requester, response);
}

void StopPipelineCommand::execute(std::shared_ptr<InputInterface::Requester> requester) {
    requester->source->sendResponse(requester, "Ack");
    component_->stop();
}
