#pragma once
#include <string>
#include "common/types/Result.h"

namespace service::api {
    class ITransport {
    public:
        virtual ~ITransport() = default;

        virtual Result<void> start(const std::string& server_address) = 0;
        virtual Result<void> stop() = 0;
        virtual Result<void> runLoop() = 0;
    };
}
