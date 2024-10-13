#ifndef GSTREAMER_H
#define GSTREAMER_H

#include <gst/gst.h>
#include "Logger/Logger.h"

class Gstreamer {
public:
    explicit Gstreamer(std::string  pipeline_file);
    ~Gstreamer();
    void play();
    void stop() const;

    int enable_element(const std::string& element_name);
    int disable_element(const std::string& element_name);
private:
    GstElement *pipeline_;
    GMainLoop *loop_ = nullptr;
    GstElement* optional_element_ = nullptr;
    std::vector<std::string> get_pipeline_elements(const std::string& file_path);
    std::string pipeline_file_;
    std::vector<std::string> pipeline_elements_;
    std::vector<GstElement*> gst_elements;
};

#endif //GSTREAMER_H
