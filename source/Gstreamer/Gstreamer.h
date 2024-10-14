#ifndef GSTREAMER_H
#define GSTREAMER_H

#include <gst/gst.h>

class Gstreamer {
public:
    explicit Gstreamer(std::string  pipeline_file);
    ~Gstreamer();
    void play();
    void stop() const;
    int enable_optional_pipeline_elements();
    int disable_optional_pipeline_elements();

private:
    GstElement *pipeline_;
    GMainLoop *loop_ = nullptr;
    GstElement* optional_element_ = nullptr;
    std::vector<std::string> get_pipeline_elements(const std::string& file_path);
    std::vector<std::string> get_optional_pipeline_elements(const std::string& file_path);
    int enable_element(const std::string& element_name);
    int disable_element(const std::string& element_name);
    std::string pipeline_file_;
    std::vector<std::string> pipeline_elements_;
    std::vector<std::string> optional_pipeline_elements_;
    std::vector<GstElement*> gst_elements;
};

#include "Logger/Logger.h"

#endif //GSTREAMER_H
