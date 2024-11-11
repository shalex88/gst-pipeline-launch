#include "PipelineCommands.h"

void EnableOptionalElementCommand::execute(std::shared_ptr<InputInterface::Requester> requester) {
    component_->enableOptionalPipelineElement(element_name_);
    requester->source->sendResponse(requester, "Ack");
}

void DisableOptionalElementCommand::execute(std::shared_ptr<InputInterface::Requester> requester) {
    component_->disableOptionalPipelineElement(element_name_);
    requester->source->sendResponse(requester, "Ack");
}

void EnableAllOptionalElementsCommand::execute(std::shared_ptr<InputInterface::Requester> requester) {
    component_->enableAllOptionalPipelineElements();
    requester->source->sendResponse(requester, "Ack");
}

void DisableAllOptionalElementsCommand::execute(std::shared_ptr<InputInterface::Requester> requester) {
    component_->disableAllOptionalPipelineElements();
    requester->source->sendResponse(requester, "Ack");
}

void StopPipelineCommand::execute(std::shared_ptr<InputInterface::Requester> requester) {
    component_->stop();
    requester->source->sendResponse(requester, "Ack");
}