#pragma once

#include <memory>
#include <thread>

#include "common/types/Result.h"
#include "core/ICore.h"
#include "core/pipeline/PipelineManager.h"

namespace service::core {
    class Core final : public ICore {
    public:
        Core();
        ~Core() override;

        // ICore implementation
        Result<void> start() override;
        Result<void> stop() override;
        bool isRunning() const;

    private:
        void runPipelineThread();

        std::unique_ptr<PipelineManager> pipeline_manager_;
        std::jthread pipeline_thread_;
        bool is_running_{false};
    };
}
