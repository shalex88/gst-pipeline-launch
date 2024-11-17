#include "PipelineElement.h"

PipelineElement::PipelineElement(const unsigned int idx, const unsigned int idy, std::string name, std::string type,std::map<std::string, std::string> properties,
                                 const bool optional, const bool enabled, GstElement* gst_element)
    : id(idx,idy), name(std::move(name)), type(std::move(type)), properties(std::move(properties)),
      optional(optional), enabled(enabled),
      gst_element(gst_element) {
}
