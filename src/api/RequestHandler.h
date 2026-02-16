#pragma once
#include <atomic>
#include <memory>
#include <thread>
#include <vector>

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

        Result<void> setVideoCapability(std::string_view capability, bool enable) const override;
        Result<std::vector<std::string>> getVideoCapabilities() const override;
        Result<bool> getVideoCapabilityState(std::string_view capability) const override;

    private:
        void monitorCore();

        std::unique_ptr<core::ICore> core_;
        std::atomic<bool> running_{false};
        std::jthread monitor_thread_;
    };
}
