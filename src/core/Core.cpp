#include "Core.h"

#include "common/logger/Logger.h"
#include "core/pipeline/PipelineManager.h"

namespace service::core {
    Core::Core()
        : is_running_(false) {
    }

    Core::~Core() {
        if (stop().isError()) {
            LOG_ERROR("Failed to shut down Core properly");
        }
    }

    Result<void> Core::start() {
        LOG_DEBUG("Starting...");

        const auto pipeline_manager = std::make_shared<PipelineManager>("/home/shalex/dev/video-player/config/pipeline.yaml"); //FIXME: hardcoded path
        if (const auto ec = pipeline_manager->play()) { // Blocking call
            LOG_ERROR("Failed to play pipeline {}", ec.message());
        }

        is_running_ = true;
        LOG_DEBUG("Running");
        return Result<void>::success();
    }

    Result<void> Core::stop() {
        if (!isRunning()) {
            return Result<void>::success();
        }

        is_running_ = false;

        LOG_DEBUG("Stopping...");

        LOG_DEBUG("Stopped");
        return Result<void>::success();
    }

    bool Core::isRunning() const {
        return is_running_;
    }
}
