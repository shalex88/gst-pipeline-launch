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
    std::error_code play();
    std::error_code stop() const;
    std::error_code enableOptionalPipelineElement(const std::string& element_name);
    std::error_code disableOptionalPipelineElement(const std::string& element_name);
    std::error_code enableOptionalPipelineBranch(const std::string& branch_name);
    std::error_code disableOptionalPipelineBranch(const std::string& branch_name);
    std::error_code enableAllOptionalPipelineElements();
    std::error_code disableAllOptionalPipelineElements();
    std::error_code enableAllOptionalPipelineBranches();
    std::error_code disableAllOptionalPipelineBranches();
    std::vector<std::string> getOptionalPipelineElementsNames() const;
    std::vector<std::string> getOptionalPipelineBranchesNames() const;

private:
    PipelineElement& findTeeElementForBranch(const std::string& branch_name);
    PipelineElement& findFirstElementInBranch(const std::string& branch_name);
    static GstPad* findLinkedSrcPad(GstElement* upstream_element, GstElement* downstream_element);
    std::error_code createGstElement(PipelineElement& element) const;
    std::error_code linkGstElement(PipelineElement& current_element);
    std::error_code createGstPipeline(std::vector<PipelineElement>& pipeline);
    void createElementsList(const std::string& file_path);
    PipelineElement* getPreviousEnabledElement(const PipelineElement& element);
    PipelineElement* getNextEnabledElement(const PipelineElement& element);
    static std::error_code linkElements(PipelineElement& source, PipelineElement& destination);
    std::error_code enableOptionalElement(PipelineElement& element);
    std::error_code disableOptionalElement(PipelineElement& element) const;
    static gint busCallback(GstBus* bus, GstMessage* message, gpointer data);
    static GstPadProbeReturn disconnectGstElementProbeCallback(GstPad* src_peer, GstPadProbeInfo* info, gpointer data);
    static GstPadProbeReturn connectGstElementProbeCallback(GstPad* pad, GstPadProbeInfo* info, gpointer data);
    static GstPadProbeReturn disconnectBranchProbeCallback(GstPad* src_peer, GstPadProbeInfo* info, gpointer data);
    static GstPadTemplate* findSuitablePadTemplate(PipelineElement& element, GstPadDirection direction);
    static std::string generateDynamicPadName(const GstPadTemplate* pad_template);
    static GstPad* requestExplicitPadName(PipelineElement& element, GstPadDirection direction);
    static GstPad* allocatePad(PipelineElement& element, GstPadDirection direction);
    std::string generateGstElementUniqueName(const PipelineElement& element) const;
    void validateGstElementProperties(PipelineElement& element) const;
    void setGstElementProperty(PipelineElement& element) const;
    std::error_code retrieveMuxGstElement(PipelineElement& element, const std::string unique_element_name) const;
    bool isGstElementInPipeline(const std::string& element_name) const;
    std::vector<GstPad*> getLinkedSinkPads(GstElement* element) const;
    std::shared_ptr<GMainLoop> gst_loop_;
    std::shared_ptr<GstElement> gst_pipeline_;
    std::string pipeline_file_;
    std::vector<PipelineElement> pipeline_elements_;
    mutable std::mutex mutex_;
};

#endif //PIPELINEMANAGER_H