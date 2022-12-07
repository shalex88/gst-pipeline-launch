#include <sstream>
#include "Logger/Logger.h"
#include "PipelineElement.h"

PipelineElement::PipelineElement(const unsigned int id, std::string name, std::string type, std::string branch,
                                 std::map<std::string, std::string> properties, std::string sink_pad_name,
                                 const bool optional, GstElement* gst_element)
    : id(id), name(std::move(name)), type(std::move(type)), branch(std::move(branch)),
      properties(std::move(properties)), sink_pad_name(std::move(sink_pad_name)), is_optional(optional), gst_element(gst_element) {
}

std::string PipelineElement::toString() const {
    std::ostringstream oss;
    oss << "'" << name;
    for (const auto& [key, value] : properties) {
        oss << " " << key << "=" << value;
    }
    oss << " (" << branch;
    if (name == "tee") {
        oss << " -> " << type;
    }
    oss << ")'";
    return oss.str();
}
