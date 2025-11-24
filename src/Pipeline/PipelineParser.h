#ifndef PIPELINEPARSER_H
#define PIPELINEPARSER_H

#include <vector>
#include "Pipeline/PipelineElement.h"
#include <File/File.h>
#include <yaml-cpp/yaml.h>

class PipelineParser {
public:
    explicit PipelineParser(const std::string& file_name);
    ~PipelineParser();
    std::vector<PipelineElement> getAllElements() const;
private:
    std::unique_ptr<File> file_;
    static PipelineElement deserializeElement(const YAML::detail::iterator_value& element, std::string branch, const bool branch_is_optional);
};

#endif //PIPELINEPARSER_H
