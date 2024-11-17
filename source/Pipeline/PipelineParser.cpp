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
    for (const auto& element: yaml_data["pipeline"]["elements"]) {
        static unsigned int id = 0;
        all_elements.emplace_back(
            id++,
            element["name"].as<std::string>(),
            element["type"].IsDefined()
                ? element["type"].as<std::string>()
                : "unknown",
            element["properties"].IsDefined()
                ? element["properties"].as<std::map<std::string, std::string> >()
                : std::map<std::string, std::string>(),
            element["optional"].IsDefined() ? element["optional"].as<bool>() : false,
            false,
            nullptr
        );
    }

    return all_elements;
}
