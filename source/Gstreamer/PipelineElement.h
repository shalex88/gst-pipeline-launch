#ifndef PIPELINEELEMENT_H
#define PIPELINEELEMENT_H

#include <map>
#include <ostream>
#include <string>
#include <gst/gstelement.h>

struct PipelineElement {
    unsigned int id;
    std::string name;
    std::map<std::string, std::string> properties;
    bool optional;
    bool enabled;
    GstElement* gst_element;

    friend std::ostream& operator<<(std::ostream& os, const PipelineElement& element);
} typedef PipelineElement;

inline std::ostream& operator<<(std::ostream& os, const PipelineElement& element) {
    os << element.name;

    for (const auto& [key, value] : element.properties) {
        os << " " << key << "=" << value;
    }

    return os;
}

#endif //PIPELINEELEMENT_H
