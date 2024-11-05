#ifndef PIPELINEELEMENT_H
#define PIPELINEELEMENT_H

#include <map>
#include <string>

struct {
    unsigned int id;
    std::string name;
    std::string type;
    std::string caps;
    std::map<std::string, std::string> properties;
    bool optional;
    bool enabled;
} typedef PipelineElement;

#endif //PIPELINEELEMENT_H
