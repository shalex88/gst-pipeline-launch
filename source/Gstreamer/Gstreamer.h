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
    int enableAllOptionalPipelineElements();
    int disableAllOptionalPipelineElements();
    int enableOptionalPipelineElement(const std::string& element_name);
    int disableOptionalPipelineElement(const std::string& element_name);
    std::vector<std::string> getOptionalPipelineElementsNames() const;

private:
    void linkAllGstElements();
    void createGstElement(PipelineElement& element);
    void createGstPipeline(std::vector<PipelineElement>& pipeline);
    std::vector<PipelineElement> createElementsList(const std::string& file_path);
    void printElement(const PipelineElement& element);
    PipelineElement getPreviousEnabledElement(const PipelineElement& element) const;
    PipelineElement getNextEnabledElement(const PipelineElement& element) const;
    PipelineElement& findElement(const std::string& element_name);
    int enableOptionalElement(PipelineElement& element) const;
    int disableOptionalElement(PipelineElement& element) const;
    GMainLoop* gst_loop_ = nullptr;
    GstElement* gst_pipeline_ = nullptr;
    std::string pipeline_file_;
    std::vector<PipelineElement> pipeline_elements_;
    mutable std::mutex mutex_;
};


#endif //GSTREAMER_H
