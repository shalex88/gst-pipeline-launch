#pragma once
#include <atomic>
#include <memory>
#include <thread>

#include "api/IRequestHandler.h"
#include "common/types/Result.h"

namespace service::core {
    class ICore;
}

namespace service::api {
    class RequestHandler final : public IRequestHandler {
    public:
        explicit RequestHandler(std::unique_ptr<core::ICore> core);
        ~RequestHandler() override;

        Result<void> start() override;
        Result<void> stop() override;
        bool isRunning() const override;

        Result<void> enableOptionalElement(std::string_view element) const override;
        Result<void> disableOptionalElement(std::string_view element) const override;

    private:
        void monitorCore();

        std::unique_ptr<core::ICore> core_;
        std::atomic<bool> running_{false};
        std::jthread monitor_thread_;
    };
}
