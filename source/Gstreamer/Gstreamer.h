#ifndef GSTREAMER_H
#define GSTREAMER_H

#include <gst/gst.h>
#include "Gstreamer/PipelineElement.h"

class Gstreamer {
public:
    explicit Gstreamer(std::string  pipeline_file);
    ~Gstreamer();
    void play();
    void stop();
    int enable_optional_pipeline_elements();
    int disable_optional_pipeline_elements();
    bool is_running() const;

private:
    bool is_finished_ {false};
    GstElement *pipeline_ = nullptr;
    GMainLoop *loop_ = nullptr;
    std::vector<PipelineElement> create_element_objects(const std::string& file_path);
    std::vector<PipelineElement> init_default_pipeline_elements();
    GstElement* get_previous_running_gst_element(const PipelineElement& element) const;
    GstElement* get_next_gst_element(const PipelineElement& element) const;
    void enable_element(PipelineElement& element);
    void disable_element(PipelineElement& element);
    std::string pipeline_file_;
    std::vector<PipelineElement> user_pipeline_elements_;
    std::vector<PipelineElement> running_pipeline_elements_;
    std::vector<GstElement*> running_gst_elements;
    mutable std::mutex mutex_;
};

#include "Logger/Logger.h"

#endif //GSTREAMER_H
