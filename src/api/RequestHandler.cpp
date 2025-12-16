#include "RequestHandler.h"

#include <chrono>

#include "common/logger/Logger.h"
#include "core/ICore.h"

namespace service::api {
    RequestHandler::RequestHandler(std::unique_ptr<core::ICore> core) : core_(std::move(core)) {
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
        monitor_thread_ = std::jthread([this] {
            monitorCore();
        });
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
        return running_.load();
    }

    Result<void> RequestHandler::enableOptionalElement(std::string_view element) const {
        if (!isRunning()) {
            return Result<void>::error("RequestHandler is not running");
        }

        LOG_INFO("Request: {} {}", __func__, element);

        auto operation = core_->enableOptionalElement(element);

        if (operation.isError()) {
            LOG_ERROR("Response: {}", operation.error());
        } else {
            LOG_INFO("Response: Success");
        }

        return operation;
    }

    Result<void> RequestHandler::disableOptionalElement(std::string_view element) const {
        if (!isRunning()) {
            return Result<void>::error("RequestHandler is not running");
        }

        LOG_INFO("Request: {} {}", __func__, element);

        auto operation = core_->disableOptionalElement(element);

        if (operation.isError()) {
            LOG_ERROR("Response: {}", operation.error());
        } else {
            LOG_INFO("Response: Success");
        }

        return operation;
    }

    void RequestHandler::monitorCore() {
        while (running_.load()) {
            if (!core_ || !core_->isRunning()) {
                if (const auto result = stop(); result.isError()) {
                    LOG_ERROR("Failed to stop RequestHandler: {}", result.error());
                }
                break;
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
    }
}
