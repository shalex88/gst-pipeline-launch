#ifndef PIPELINEELEMENT_H
#define PIPELINEELEMENT_H

#include <string>

struct {
    unsigned int id;
    std::string name;
    std::string type;
    std::string caps;
    bool optional;
    bool enabled;
} typedef PipelineElement;

#endif //PIPELINEELEMENT_H
