#include "RequestHandler.h"

#include "common/logger/Logger.h"
#include "core/ICore.h"

namespace service::api {
    RequestHandler::RequestHandler(std::unique_ptr<core::ICore> core)
        : core_(std::move(core)), running_(false) {
        if (!core_) {
            throw std::invalid_argument("Core cannot be null");
        }
    }

    RequestHandler::~RequestHandler() {
        if (stop().isError()) {
            LOG_ERROR("RequestHandler failed to stop gracefully");
        }
    }

    Result<void> RequestHandler::start() {
        LOG_DEBUG("Starting RequestHandler...");
        if (const auto init_result = core_->start(); init_result.isError()) {
            return Result<void>::error(init_result.error());
        }

        running_ = true;
        LOG_DEBUG("RequestHandler started");
        return Result<void>::success();
    }

    Result<void> RequestHandler::stop() {
        if (!isRunning()) {
            return Result<void>::success();
        }

        LOG_DEBUG("Stopping RequestHandler...");
        running_ = false;

        if (core_) {
            if (const auto shutdown_result = core_->stop(); shutdown_result.isError()) {
                LOG_ERROR("Error stopping core: {}", shutdown_result.error());
                return Result<void>::error("Failed to shut down core: " + shutdown_result.error());
            }
        }
        LOG_DEBUG("RequestHandler stopped");
        return Result<void>::success();
    }

    bool RequestHandler::isRunning() const {
        return running_;
    }
}
