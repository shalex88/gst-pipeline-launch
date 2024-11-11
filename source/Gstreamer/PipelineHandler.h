#ifndef PIPELINEHANDLER_H
#define PIPELINEHANDLER_H

#include <vector>
#include "Gstreamer/PipelineElement.h"
#include <File/File.h>
#include "PipelineHandler.h"

class PipelineHandler {
public:
    explicit PipelineHandler(const std::string& file_name);
    ~PipelineHandler();
    std::vector<PipelineElement> getAllElements() const;
private:
    std::unique_ptr<File> file_;
};

#endif //PIPELINEHANDLER_H
