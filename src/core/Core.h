#pragma once

#include "common/types/Result.h"
#include "core/ICore.h"

namespace service::core {
    class Core final : public ICore {
    public:
        Core();
        ~Core() override;

        // ICore implementation
        Result<void> start() override;
        Result<void> stop() override;

    private:
        bool isRunning() const;
        bool is_running_;
    };
}
