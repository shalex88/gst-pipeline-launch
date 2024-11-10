#include "PipelineCommands.h"

void StartOptionalElementsCommand::execute(std::shared_ptr<InputInterface::Requester> requester) {
    component_->enable_optional_pipeline_elements();
    requester->source->sendResponse(requester, "Ack");
}

void StopOptionalElementsCommand::execute(std::shared_ptr<InputInterface::Requester> requester) {
    component_->disable_optional_pipeline_elements();
    requester->source->sendResponse(requester, "Ack");
}

void StopPipelineCommand::execute(std::shared_ptr<InputInterface::Requester> requester) {
    component_->stop();
    requester->source->sendResponse(requester, "Ack");
}