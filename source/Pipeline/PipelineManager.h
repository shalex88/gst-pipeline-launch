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
    int enableOptionalPipelineElement(const std::string& element_name);
    int disableOptionalPipelineElement(const std::string& element_name);
    int enableOptionalPipelineBranch(const std::string& branch_name);
    int disableOptionalPipelineBranch(const std::string& branch_name);
    int enableAllOptionalPipelineElements();
    int disableAllOptionalPipelineElements();
    int enableAllOptionalPipelineBranches();
    int disableAllOptionalPipelineBranches();
    std::vector<std::string> getOptionalPipelineElementsNames() const;
    std::vector<std::string> getOptionalPipelineBranchesNames() const;

private:
    PipelineElement& findTeeElementForBranch(const std::string& branch_name);
    int createGstElement(PipelineElement& element) const;
    int linkGstElement(PipelineElement& current_element);
    int createGstPipeline(std::vector<PipelineElement>& pipeline);
    void createElementsList(const std::string& file_path);
    PipelineElement* getPreviousEnabledElement(const PipelineElement& element);
    PipelineElement* getNextEnabledElement(const PipelineElement& element);
    static int linkElements(const PipelineElement& source, const PipelineElement& destination);
    int enableOptionalElement(PipelineElement& element);
    int disableOptionalElement(PipelineElement& element) const;
    static gint busCallback(GstBus* bus, GstMessage* message, gpointer data);
    static GstPadProbeReturn disconnectGstElementProbeCallback(GstPad* src_peer, GstPadProbeInfo* info, gpointer data);
    static GstPadProbeReturn connectGstElementProbeCallback(GstPad* pad, GstPadProbeInfo* info, gpointer data);
    std::shared_ptr<GMainLoop> gst_loop_;
    std::shared_ptr<GstElement> gst_pipeline_;
    std::string pipeline_file_;
    std::vector<PipelineElement> pipeline_elements_;
    mutable std::mutex mutex_;
};

#endif //PIPELINEMANAGER_H