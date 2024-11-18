#include "PipelineElement.h"

PipelineElement::PipelineElement(const unsigned int id, std::string name, std::string type, std::string branch,
                                 std::map<std::string, std::string> properties,
                                 const bool optional, const bool enabled, GstElement* gst_element)
    : id(id), name(std::move(name)), type(std::move(type)), branch(std::move(branch)),
      properties(std::move(properties)), optional(optional), enabled(enabled), gst_element(gst_element) {
}
