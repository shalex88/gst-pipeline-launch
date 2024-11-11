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
    int enable_all_optional_pipeline_elements();
    int disable_all_optional_pipeline_elements();
    int enable_optional_pipeline_element(const std::string& element_name);
    int disable_optional_pipeline_element(const std::string& element_name);
    std::vector<std::string> get_optional_pipeline_elements_names() const;

private:
    void link_all_gst_elements();
    void create_gst_element(PipelineElement& element);
    void create_gst_pipeline(std::vector<PipelineElement>& pipeline);
    std::vector<PipelineElement> create_elements_list(const std::string& file_path);
    void print_element(const PipelineElement& element);
    PipelineElement get_previous_enabled_element(const PipelineElement& element) const;
    PipelineElement get_next_enabled_element(const PipelineElement& element) const;
    PipelineElement find_element(const std::string& element_name);
    int enable_optional_element(PipelineElement& element) const;
    int disable_optional_element(PipelineElement& element) const;
    GMainLoop* gst_loop_ = nullptr;
    GstElement* gst_pipeline_ = nullptr;
    std::string pipeline_file_;
    std::vector<PipelineElement> pipeline_elements_;
    mutable std::mutex mutex_;
};


#endif //GSTREAMER_H
