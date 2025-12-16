#include "Core.h"

#include "common/logger/Logger.h"
#include "core/pipeline/PipelineManager.h"

namespace service::core {
    Core::Core() : pipeline_manager_(nullptr) {
    }

    Core::~Core() {
        if (stop().isError()) {
            LOG_ERROR("Failed to shut down Core properly");
        }
    }

    Result<void> Core::start() {
        LOG_DEBUG("Starting...");

        is_running_ = true;
        pipeline_manager_ = std::make_unique<PipelineManager>("/home/shalex/dev/video-player/config/pipeline.yaml"); //FIXME: hardcoded path
        pipeline_thread_ = std::jthread([this] {
            runPipelineThread();
        });

        LOG_DEBUG("Running");
        return Result<void>::success();
    }

    Result<void> Core::stop() {
        if (!isRunning()) {
            return Result<void>::success();
        }

        is_running_ = false;

        LOG_DEBUG("Stopped");
        return Result<void>::success();
    }

    bool Core::isRunning() const {
        return is_running_;
    }

    Result<void> Core::enableOptionalElement(std::string_view element) const {
        if (!isRunning()) {
            return Result<void>::error("Core is not running");
        }

        if (const auto ec = pipeline_manager_->enableOptionalPipelineElement(std::string(element))) {
            LOG_ERROR("Failed to enable optional element {}: {}", element, ec.message());
            return Result<void>::error("Failed to enable optional element: " + ec.message());
        }

        return Result<void>::success();
    }

    Result<void> Core::disableOptionalElement(std::string_view element) const {
        if (!isRunning()) {
            return Result<void>::error("Core is not running");
        }

        if (const auto ec = pipeline_manager_->disableOptionalPipelineElement(std::string(element))) {
            LOG_ERROR("Failed to disable optional element {}: {}", element, ec.message());
            return Result<void>::error("Failed to disable optional element: " + ec.message());
        }

        return Result<void>::success();
    }

    void Core::runPipelineThread() {
        if (pipeline_manager_) {
            if (const auto ec = pipeline_manager_->play()) {
                LOG_ERROR("Failed to play pipeline {}", ec.message());
            }
            if (const auto result = stop(); result.isError()) {
                LOG_ERROR("Failed to stop Core: {}", result.error());
            }
        }
    }
}
