#pragma once

#include <vector>

#include "common/types/Result.h"

namespace service::core {
    class ICore {
    public:
        virtual ~ICore() = default;

        virtual Result<void> start() = 0;
        virtual Result<void> stop() = 0;
        virtual bool isRunning() const = 0;

        virtual Result<void> enableOptionalElement(std::string_view element) const = 0;
        virtual Result<void> disableOptionalElement(std::string_view element) const = 0;
        virtual Result<std::vector<std::string>> getVideoCapabilities() const = 0;
        virtual Result<bool> getVideoCapabilityState(std::string_view capability) const = 0;
    };
}

