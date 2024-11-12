#include "PipelineElement.h"

PipelineElement::PipelineElement() : gst_element(nullptr) {
}

PipelineElement::PipelineElement(const unsigned int id, std::string name, std::map<std::string, std::string> properties,
                                 const bool optional, const bool enabled, std::shared_ptr<GstElement> gst_element)
    : id(id), name(std::move(name)), properties(std::move(properties)),
      optional(optional), enabled(enabled),
      gst_element(std::move(gst_element).get()) {
}
