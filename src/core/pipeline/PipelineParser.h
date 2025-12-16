#pragma once

#include <vector>
#include <yaml-cpp/yaml.h>
#include "common/file/File.h"

#include "core/pipeline/PipelineElement.h"

class PipelineParser {
public:
    explicit PipelineParser(std::string_view file_name);
    ~PipelineParser();
    std::vector<PipelineElement> getAllElements() const;
private:
    std::unique_ptr<File> file_;
    static PipelineElement deserializeElement(const YAML::detail::iterator_value& element, std::string branch, const bool branch_is_optional);
};