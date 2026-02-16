#pragma once

#include <vector>

#include "common/types/Result.h"

namespace service::api {
    class IRequestHandler {
    public:
        virtual ~IRequestHandler() = default;

        virtual Result<void> start() = 0;
        virtual Result<void> stop() = 0;
        virtual bool isRunning() const = 0;

        virtual Result<void> setVideoCapability(std::string_view capability, bool enable) const = 0;
        virtual Result<std::vector<std::string>> getVideoCapabilities() const = 0;
        virtual Result<bool> getVideoCapabilityState(std::string_view capability) const = 0;
    };
}
