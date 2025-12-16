#pragma once

#include "common/types/Result.h"

namespace service::api {
    class IRequestHandler {
    public:
        virtual ~IRequestHandler() = default;

        virtual Result<void> start() = 0;
        virtual Result<void> stop() = 0;
        virtual bool isRunning() const = 0;

        virtual Result<void> enableOptionalElement(std::string_view element) const = 0;
        virtual Result<void> disableOptionalElement(std::string_view element) const = 0;
    };
}
