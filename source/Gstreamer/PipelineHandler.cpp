#include <nlohmann/json.hpp>
#include "PipelineHandler.h"

PipelineHandler::PipelineHandler(const std::string& file_name) : file_(std::make_unique<File>(file_name)) {
    LOG_TRACE("PipelineHandler constructor");
}

PipelineHandler::~PipelineHandler() {
    LOG_TRACE("PipelineHandler destructor");
}

std::vector<PipelineElement> PipelineHandler::get_all_elements() const {
    nlohmann::json json_data;
    file_->get_content() >> json_data;

    std::vector<PipelineElement> all_elements;
    for (const auto& element : json_data["pipeline"]["elements"]) {
        static unsigned int id = 0;
        all_elements.emplace_back(PipelineElement{
            id++,
            element.at("name").get<std::string>(),
            element.at("type").get<std::string>(),
            element.at("properties").get<std::map<std::string, std::string>>(),
            element.at("optional").get<bool>(),
            false,
            nullptr
            }
            );
    }

    return all_elements;
}
