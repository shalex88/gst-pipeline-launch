#include "PipelineParser.h"
#include <yaml-cpp/yaml.h>

PipelineParser::PipelineParser(const std::string& file_name) : file_(std::make_unique<File>(file_name)) {
    LOG_TRACE("PipelineParser constructor");
}

PipelineParser::~PipelineParser() {
    LOG_TRACE("PipelineParser destructor");
}

std::vector<PipelineElement> PipelineParser::getAllElements() const {
    auto yaml_data = YAML::Load(file_->getContent());

    std::vector<PipelineElement> all_elements;
    unsigned int idy = 0;
    static unsigned int last_tee_idx = 0;

    for (const auto& branch : yaml_data["pipeline"]["branches"]) {
        unsigned int idx = last_tee_idx;

        for (const auto& element : branch["elements"]) {
            auto name = element["name"].as<std::string>();
            auto type = element["type"].IsDefined() ? element["type"].as<std::string>() : "unknown";
            if (type == "tee") {
                name = type;
            }

            all_elements.emplace_back(
                idx,
                idy,
                name,
                type,
                element["properties"].IsDefined() ? element["properties"].as<std::map<std::string, std::string>>() : std::map<std::string, std::string>(),
                element["optional"].IsDefined() ? element["optional"].as<bool>() : false,
                false,
                nullptr
            );

            if (type == "tee") {
                last_tee_idx = idx;
            } else if (type == "sink") {
                idx = last_tee_idx;
            }

            idx++;
        }
        idy++;
    }

    return all_elements;
}
