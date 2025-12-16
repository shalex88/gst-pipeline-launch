#include "ApiController.h"

#include <utility>

#include "api/IRequestHandler.h"
#include "api/ITransport.h"
#include "common/logger/Logger.h"

namespace service::api {
    ApiController::ApiController(std::unique_ptr<IRequestHandler> request_handler,
                                 std::unique_ptr<ITransport> transport, std::string server_address)
        : request_handler_(std::move(request_handler)), transport_(std::move(transport)),
          server_address_(std::move(server_address)), is_running_(false) {
        if (!request_handler_) {
            throw std::invalid_argument("Request Handler cannot be null");
        }
        if (!transport_) {
            throw std::invalid_argument("Transport cannot be null");
        }
        if (server_address_.empty()) {
            throw std::invalid_argument("Server address cannot be empty");
        }
    }

    ApiController::~ApiController() {
        if (stop().isError()) {
            LOG_ERROR("ApiController failed to stop gracefully");
        }
    }

    Result<void> ApiController::startAsync() {
        LOG_DEBUG("Starting ApiController...");

        if (const auto request_handler_result = request_handler_->start(); request_handler_result.isError()) {
            return Result<void>::error(request_handler_result.error());
        }

        if (const auto transport_result = transport_->start(server_address_); transport_result.isError()) {
            if (const auto result = request_handler_->stop(); result.isError()) {
                return Result<void>::error(result.error());
            }
            return Result<void>::error(transport_result.error());
        }

        is_running_ = true;

        service_thread_ = std::jthread([this] {
            if (transport_->runLoop().isError()) {
                LOG_ERROR("Transport run loop failed");
                is_running_ = false;
            }
        });

        LOG_DEBUG("ApiController started");

        return Result<void>::success();
    }

    Result<void> ApiController::stop() {
        if (!isRunning()) {
            return Result<void>::success();
        }

        LOG_DEBUG("Stopping ApiController...");
        is_running_ = false;

        if (const auto transport_result = transport_->stop(); transport_result.isError()) {
            return Result<void>::error("Error stopping transport: " + transport_result.error());
        }

        if (const auto stop_result = request_handler_->stop(); stop_result.isError()) {
            return Result<void>::error("Failed to stop request handler: " + stop_result.error());
        }

        LOG_DEBUG("ApiController stopped");
        return Result<void>::success();
    }

    bool ApiController::isRunning() const {
        return is_running_;
    }
}
