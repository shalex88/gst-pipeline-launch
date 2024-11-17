#ifndef PIPELINEPARSER_H
#define PIPELINEPARSER_H

#include <vector>
#include "Pipeline/PipelineElement.h"
#include <File/File.h>

class PipelineParser {
public:
    explicit PipelineParser(const std::string& file_name);
    ~PipelineParser();
    std::vector<PipelineElement> getAllElements() const;
private:
    std::unique_ptr<File> file_;
};

#endif //PIPELINEPARSER_H
