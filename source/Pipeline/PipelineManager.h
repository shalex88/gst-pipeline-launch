#ifndef PIPELINEMANAGER_H
#define PIPELINEMANAGER_H

#include <memory>
#include <vector>
#include <mutex>
#include <gst/gst.h>
#include "Pipeline/PipelineElement.h"

class PipelineManager {
public:
    explicit PipelineManager(std::string pipeline_file);
    ~PipelineManager();
    int play();
    int stop() const;
    int enableAllOptionalPipelineElements();
    int disableAllOptionalPipelineElements();
    int enableOptionalPipelineElement(const std::string& element_name);
    int disableOptionalPipelineElement(const std::string& element_name);
    std::vector<std::string> getOptionalPipelineElementsNames() const;

private:
    int linkAllGstElements();
    int createGstElement(PipelineElement& element) const;
    int createGstPipeline(std::vector<PipelineElement>& pipeline);
    void createElementsList(const std::string& file_path);
    static void printElement(const PipelineElement& element);
    PipelineElement* getPreviousEnabledElement(const PipelineElement& element);
    PipelineElement* getNextEnabledElement(const PipelineElement& element);
    int enableElement(PipelineElement& element);
    int disableElement(PipelineElement& element);
    static gint busCallback(GstBus* bus, GstMessage* message, gpointer data);
    std::shared_ptr<GMainLoop> gst_loop_;
    std::shared_ptr<GstElement> gst_pipeline_;
    std::string pipeline_file_;
    std::vector<PipelineElement> pipeline_elements_;
    mutable std::mutex mutex_;
};

#endif //PIPELINEMANAGER_H