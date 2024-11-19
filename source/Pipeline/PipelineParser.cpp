#include "PipelineParser.h"

#include <utility>

PipelineParser::PipelineParser(const std::string& file_name) : file_(std::make_unique<File>(file_name)) {
    LOG_TRACE("PipelineParser constructor");
}

PipelineParser::~PipelineParser() {
    LOG_TRACE("PipelineParser destructor");
}

PipelineElement PipelineParser::deserializeElement(const YAML::detail::iterator_value& element, std::string branch, const bool branch_is_optional) {
    static unsigned int id = 0;
    auto name = element["name"].as<std::string>();
    auto type = element["type"].IsDefined() ? element["type"].as<std::string>() : "unknown";
    auto properties = element["properties"].IsDefined() ? element["properties"].as<std::map<std::string, std::string>>() : std::map<std::string, std::string>();
    auto is_optional = element["optional"].IsDefined() ? element["optional"].as<bool>() : false;
    if (branch_is_optional) {
        is_optional = true;
    }

    return {id++, name, type, std::move(branch), properties, is_optional, false, nullptr};
}

std::vector<PipelineElement> PipelineParser::getAllElements() const {
    auto yaml_data = YAML::Load(file_->getContent());

    std::vector<PipelineElement> all_elements;

    for (const auto& branch : yaml_data["pipeline"]["branches"]) {
        const auto branch_name = branch["name"].as<std::string>();
        const auto branch_is_optional = branch["optional"].IsDefined() ? branch["optional"].as<bool>() : false;
        if (!branch_is_optional) {
            for (const auto& element : branch["elements"]) {
                all_elements.emplace_back(deserializeElement(element, branch_name, branch_is_optional));
            }
        }
    }

    return all_elements;
}
