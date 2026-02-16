#include "Core.h"

#include "common/logger/Logger.h"
#include "core/pipeline/PipelineManager.h"

namespace service::core {
    Core::Core(std::string_view pipeline_path)
        : pipeline_path_(pipeline_path) {
    }

    Core::~Core() {
        if (stop().isError()) {
            LOG_ERROR("Failed to shut down Core properly");
        }
    }

    Result<void> Core::start() {
        LOG_DEBUG("Starting...");

        is_running_ = true;
        pipeline_manager_ = std::make_unique<PipelineManager>(pipeline_path_);
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

        if (pipeline_manager_) {
            if (const auto ec = pipeline_manager_->stop()) {
                LOG_ERROR("Failed to stop pipeline manager: {}", ec.message());
                return Result<void>::error("Failed to stop pipeline manager: " + ec.message());
            }
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

    Result<std::vector<std::string>> Core::getVideoCapabilities() const {
        if (!isRunning()) {
            return Result<std::vector<std::string>>::error("Core is not running");
        }

        if (!pipeline_manager_) {
            return Result<std::vector<std::string>>::error("Pipeline manager is not initialized");
        }

        auto capabilities = pipeline_manager_->getOptionalPipelineElementsNames();
        return Result<std::vector<std::string>>::success(std::move(capabilities));
    }

    Result<bool> Core::getVideoCapabilityState(std::string_view capability) const {
        if (!isRunning()) {
            return Result<bool>::error("Core is not running");
        }

        if (!pipeline_manager_) {
            return Result<bool>::error("Pipeline manager is not initialized");
        }

        const auto enabled = pipeline_manager_->isOptionalPipelineElementEnabled(capability);
        return Result<bool>::success(enabled);
    }

    void Core::runPipelineThread() {
        if (pipeline_manager_) {
            if (const auto ec = pipeline_manager_->play()) { // Blocking call
                LOG_ERROR("Failed to play pipeline {}", ec.message());
            }
            is_running_ = false;
        }
    }
}
