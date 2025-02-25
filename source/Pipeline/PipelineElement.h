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
                    std::map<std::string, std::string> properties, std::string sink_pad_name,
                    bool optional, GstElement* gst_element);
    unsigned int id {std::numeric_limits<unsigned int>::max()};
    std::string name {};
    std::string type {};
    std::string branch {};
    std::map<std::string, std::string> properties;
    std::string sink_pad_name {};
    bool is_optional {false};
    bool is_initialized {false};
    bool is_linked {false};
    GstElement* gst_element {nullptr};

    std::string toString() const;
};


#endif //PIPELINEELEMENT_H
