#pragma once

#include <memory>

namespace service::common {
    struct ApiConfig;
}

namespace service::core {
    class ICore;
}

namespace service::api {
    class ApiController;

    class ApiControllerFactory {
    public:
        static std::unique_ptr<ApiController> createController(
            std::unique_ptr<core::ICore> core, const common::ApiConfig& config);
    };
}

