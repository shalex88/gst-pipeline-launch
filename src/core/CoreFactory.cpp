#include "CoreFactory.h"

#include "common/config/ConfigManager.h"
#include "core/Core.h"

namespace service::core {
    std::unique_ptr<ICore> CoreFactory::createCore(const common::CoreConfig& config) {
        return std::make_unique<Core>(config.pipeline_path);

        throw std::invalid_argument("Unknown core type");
    }
}
