#ifndef PIPELINEELEMENT_H
#define PIPELINEELEMENT_H

#include <map>
#include <limits>
#include <ostream>
#include <string>
#include <gst/gstelement.h>

class PipelineElement {
public:
    PipelineElement() = delete;
    PipelineElement(const unsigned int id, std::string name, std::string type, std::string branch,
                    std::map<std::string, std::string> properties,
                    bool optional, bool enabled, GstElement* gst_element);
    unsigned int id {std::numeric_limits<unsigned int>::max()};
    std::string name {};
    std::string type {};
    std::string branch {};
    std::map<std::string, std::string> properties;
    bool is_optional {false};
    bool is_enabled {false};
    GstElement* gst_element {nullptr};
    std::string print() const;
};


#endif //PIPELINEELEMENT_H
