#include "File.h"
#include <nlohmann/json.hpp>

File::File(const std::string& file_name) : file_(file_name) {
    LOG_TRACE("File constructor");
    if (!file_.is_open()) {
        LOG_ERROR("Failed to open file: {}", file_name);
        return;
    }
}

File::~File() {
    LOG_TRACE("File destructor");
    file_.close();
}

std::string File::get_line() {
    std::string line;
    std::getline(file_, line);
    return line;
}

std::vector<std::string> File::get_vector_of_lines() {
    std::vector<std::string> lines;
    std::string line;
    while (std::getline(file_, line)) {
        lines.push_back(line);
    }
    return lines;
}

std::vector<std::string> File::get_non_optional_elements() {
    nlohmann::json json_data;
    file_ >> json_data;

    std::vector<std::string> non_optional_elements;
    for (const auto& element : json_data["pipeline"]["elements"]) {
        if (!element["optional"].get<bool>()) {
            non_optional_elements.push_back(element["name"].get<std::string>());
        }
    }

    return non_optional_elements;
}

std::vector<std::string> File::get_optional_elements() {
    nlohmann::json json_data;
    file_ >> json_data;

    std::vector<std::string> optional_elements;
    for (const auto& element : json_data["pipeline"]["elements"]) {
        if (element["optional"].get<bool>()) {
            optional_elements.push_back(element["name"].get<std::string>());
        }
    }

    return optional_elements;
}
