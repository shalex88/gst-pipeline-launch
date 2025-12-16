#pragma once
#include <memory>

namespace service::common {
    struct CoreConfig;
}

namespace service::core {
    class ICore;

    class CoreFactory {
    public:
        static std::unique_ptr<ICore> createCore(const common::CoreConfig& config);
    };
}
