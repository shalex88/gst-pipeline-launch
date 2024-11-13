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
    int linkAllGstElements() const;
    int createGstElement(PipelineElement& element) const;
    int createGstPipeline(std::vector<std::shared_ptr<PipelineElement>>& pipeline) const;
    void createElementsList(const std::string& file_path);
    static void printElement(const PipelineElement& element);
    std::weak_ptr<PipelineElement> getPreviousEnabledElement(const PipelineElement& element) const;
    std::weak_ptr<PipelineElement> getNextEnabledElement(const PipelineElement& element) const;
    int enableElement(PipelineElement& element) const;
    int disableElement(PipelineElement& element) const;
    std::shared_ptr<GMainLoop> gst_loop_;
    std::shared_ptr<GstElement> gst_pipeline_;
    std::string pipeline_file_;
    std::vector<std::shared_ptr<PipelineElement>> pipeline_elements_;
    mutable std::mutex mutex_;
};

#endif //PIPELINEMANAGER_H