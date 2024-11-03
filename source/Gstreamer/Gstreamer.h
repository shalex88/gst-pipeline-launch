#ifndef GSTREAMER_H
#define GSTREAMER_H

#include <gst/gst.h>
#include "Gstreamer/PipelineElement.h"

class Gstreamer {
public:
    explicit Gstreamer(std::string  pipeline_file);
    ~Gstreamer();
    void play();
    void stop() const;
    int enable_optional_pipeline_elements();
    int disable_optional_pipeline_elements();

private:
    GstElement *pipeline_ = nullptr;
    GMainLoop *loop_ = nullptr;
    void create_element_objects(const std::string& file_path);
    PipelineElement* find_element(const std::string& element_name);
    std::vector<PipelineElement> get_default_pipeline_elements();
    std::vector<PipelineElement> get_optional_pipeline_elements();
    void enable_element(const PipelineElement& element);
    void disable_element(const PipelineElement& element);
    std::string pipeline_file_;
    std::vector<PipelineElement> all_pipeline_elements_;
    std::vector<PipelineElement> default_pipeline_elements_;
    std::vector<PipelineElement> optional_pipeline_elements_;
    std::vector<GstElement*> gst_elements;
};

#include "Logger/Logger.h"

#endif //GSTREAMER_H
