#ifndef PIPELINEELEMENT_H
#define PIPELINEELEMENT_H

#include <map>
#include <memory>
#include <limits>
#include <ostream>
#include <string>
#include <gst/gstelement.h>

class PipelineElement {
public:
    PipelineElement() = delete;
    PipelineElement(const unsigned int id, std::string name, std::string type, std::string branch, std::map<std::string, std::string> properties,
                    bool optional, bool enabled, GstElement* gst_element);
    unsigned int id {std::numeric_limits<unsigned int>::max()};
    std::string name {};
    std::string type {};
    std::string branch {};
    std::map<std::string, std::string> properties;
    bool optional {false};
    bool enabled {false};
    GstElement* gst_element {nullptr};

    friend std::ostream& operator<<(std::ostream& os, const PipelineElement& element);
};

inline std::ostream& operator<<(std::ostream& os, const PipelineElement& element) {
    os << element.name;

    for (const auto& [key, value] : element.properties) {
        os << " " << key << "=" << value;
    }

    return os;
}

#endif //PIPELINEELEMENT_H
