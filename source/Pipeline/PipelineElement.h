#ifndef PIPELINEELEMENT_H
#define PIPELINEELEMENT_H

#include <map>
#include <limits>
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
    std::pair<bool,bool> is_enabled {false, false};
    unsigned int dynamic_pad_unique_index {0};
    GstElement* gst_element {nullptr};

    std::string toString() const;
};


#endif //PIPELINEELEMENT_H
