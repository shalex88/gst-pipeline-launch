#include "PipelineParser.h"

#include <utility>

PipelineParser::PipelineParser(const std::string& file_name) : file_(std::make_unique<File>(file_name)) {
    LOG_TRACE("PipelineParser constructor");
}

PipelineParser::~PipelineParser() {
    LOG_TRACE("PipelineParser destructor");
}

PipelineElement PipelineParser::deserializeElement(const YAML::detail::iterator_value& element, std::string branch) {
    static unsigned int id = 0;
    auto name = element["name"].as<std::string>();
    auto type = element["type"].IsDefined() ? element["type"].as<std::string>() : "unknown";
    auto properties = element["properties"].IsDefined() ? element["properties"].as<std::map<std::string, std::string>>() : std::map<std::string, std::string>();
    auto is_optional = element["optional"].IsDefined() ? element["optional"].as<bool>() : false;

    return {id++, name, type, std::move(branch), properties, is_optional, false, nullptr};
}

std::vector<PipelineElement> PipelineParser::getAllElements() const {
    auto yaml_data = YAML::Load(file_->getContent());

    std::vector<PipelineElement> all_elements;

    for (const auto& branch : yaml_data["pipeline"]["branches"]) {
        for (const auto& element : branch["elements"]) {
            all_elements.emplace_back(deserializeElement(element, branch["name"].as<std::string>()));
        }
    }

    return all_elements;
}
