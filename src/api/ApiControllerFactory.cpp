#include "ApiControllerFactory.h"

#include "api/ApiController.h"
#include "api/GrpcTransport.h"
#include "api/RequestHandler.h"
#include "common/config/ConfigManager.h"
#include "core/ICore.h"

namespace service::api {
    std::unique_ptr<ApiController> ApiControllerFactory::createController(
        std::unique_ptr<core::ICore> core, const common::ApiConfig& config) {
        if (!core) {
            throw std::invalid_argument("Core cannot be null");
        }

        auto request_handler = std::make_unique<RequestHandler>(std::move(core));
        auto transport = std::make_unique<GrpcTransport>(*request_handler);
        return std::make_unique<ApiController>(std::move(request_handler), std::move(transport), config.server_address);
    }
}
