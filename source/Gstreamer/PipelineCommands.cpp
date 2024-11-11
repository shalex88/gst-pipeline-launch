#include "PipelineCommands.h"

void EnableOptionalElementCommand::execute(std::shared_ptr<InputInterface::Requester> requester) {
    component_->enable_optional_pipeline_element(element_name_);
    requester->source->sendResponse(requester, "Ack");
}

void DisableOptionalElementCommand::execute(std::shared_ptr<InputInterface::Requester> requester) {
    component_->disable_optional_pipeline_element(element_name_);
    requester->source->sendResponse(requester, "Ack");
}

void EnableAllOptionalElementsCommand::execute(std::shared_ptr<InputInterface::Requester> requester) {
    component_->enable_all_optional_pipeline_elements();
    requester->source->sendResponse(requester, "Ack");
}

void DisableAllOptionalElementsCommand::execute(std::shared_ptr<InputInterface::Requester> requester) {
    component_->disable_all_optional_pipeline_elements();
    requester->source->sendResponse(requester, "Ack");
}

void StopPipelineCommand::execute(std::shared_ptr<InputInterface::Requester> requester) {
    component_->stop();
    requester->source->sendResponse(requester, "Ack");
}