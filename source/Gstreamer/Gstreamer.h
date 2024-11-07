#ifndef GSTREAMER_H
#define GSTREAMER_H

#include <vector>
#include <mutex>
#include <gst/gst.h>
#include "Gstreamer/PipelineElement.h"

class Gstreamer {
public:
    explicit Gstreamer(std::string pipeline_file);
    ~Gstreamer();
    void play();
    void stop();
    int enable_optional_pipeline_elements();
    int disable_optional_pipeline_elements();

private:
    GMainLoop* gst_loop_ = nullptr;
    void link_all_gst_elements();
    void create_gst_element(PipelineElement& element);
    GstElement* gst_pipeline_ = nullptr;
    void create_gst_pipeline(std::vector<PipelineElement>& pipeline);
    std::vector<PipelineElement> create_elements_list(const std::string& file_path);
    void print_element(const PipelineElement& element);
    PipelineElement get_previous_enabled_element(const PipelineElement& element) const;
    PipelineElement get_next_enabled_element(const PipelineElement& element) const;
    void enable_optional_element(PipelineElement& element) const;
    void disable_optional_element(PipelineElement& element) const;
    std::string pipeline_file_;
    std::vector<PipelineElement> pipeline_elements_;
    mutable std::mutex mutex_;
};

#include "Logger/Logger.h"

#endif //GSTREAMER_H
