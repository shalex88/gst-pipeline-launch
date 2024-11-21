#include <utility>
#include <sstream>
#include "Pipeline/PipelineParser.h"
#include "PipelineElement.h"
#include "PipelineManager.h"

PipelineManager::PipelineManager(std::string pipeline_file) : pipeline_file_(std::move(pipeline_file)) {
    LOG_TRACE("Pipeline constructor");
    LOG_DEBUG("Init gstreamer");

    gst_init(nullptr, nullptr);

    gst_pipeline_ = std::shared_ptr<GstElement>(gst_pipeline_new("runtime-control-pipeline"), gst_object_unref);
    if (!GST_IS_ELEMENT(gst_pipeline_.get())) {
        LOG_ERROR("Failed to create pipeline");
    }

    createElementsList(pipeline_file_);
}

int PipelineManager::linkElements(PipelineElement& source, PipelineElement& destination) {
    GstPad* tee_src_pad = nullptr;
    if (source.name == "tee") {
        tee_src_pad = gst_element_request_pad_simple(source.gst_element, "src_%u");
        if (!(tee_src_pad && GST_PAD_IS_SRC(tee_src_pad))) {
            LOG_ERROR("Failed to request a new pad from {}", source.name);
            return EXIT_FAILURE;
        }
    } else {
        tee_src_pad = gst_element_get_static_pad(source.gst_element, "src");
        if (!(tee_src_pad && GST_PAD_IS_SRC(tee_src_pad))) {
            LOG_ERROR("Failed to get the src pad of the {}", source.name);
            return EXIT_FAILURE;
        }
    }

    auto sink_pad = gst_element_get_static_pad(destination.gst_element, "sink");
    if (!sink_pad) {
        sink_pad = gst_element_get_static_pad(destination.gst_element, "video_sink");
        if (!(sink_pad && GST_PAD_IS_SINK(sink_pad))) {
            LOG_ERROR("Failed to get the sink pad of the {}", destination.name);
            gst_object_unref(tee_src_pad);
            return EXIT_FAILURE;
        }
    }

    if (gst_pad_link(tee_src_pad, sink_pad) != GST_PAD_LINK_OK) {
        LOG_ERROR("Failed to link tee to element");
        gst_object_unref(tee_src_pad);
        gst_object_unref(sink_pad);
        return EXIT_FAILURE;
    }

    // Unref pads to release references
    gst_object_unref(tee_src_pad);
    gst_object_unref(sink_pad);

    return EXIT_SUCCESS;
}

PipelineManager::~PipelineManager() {
    LOG_TRACE("Pipeline destructor");
}

int PipelineManager::enableAllOptionalPipelineBranches() {
    return EXIT_SUCCESS;
}

int PipelineManager::disableAllOptionalPipelineBranches() {
    return EXIT_SUCCESS;
}

int PipelineManager::linkGstElement(PipelineElement& current_element) {
    auto next_element = getNextEnabledElement(current_element);
    auto prev_element = getPreviousEnabledElement(current_element);

    if (next_element && current_element.branch == next_element->branch && !next_element->is_enabled.second) {
        LOG_TRACE("Link first or middle element");
        if (linkElements(current_element, *next_element) == EXIT_SUCCESS) {
            LOG_DEBUG("Linked {} to {}", current_element.toString(), next_element->toString());
        } else {
            LOG_ERROR("Failed to link {} to {}", current_element.toString(), next_element->toString());
            return EXIT_FAILURE;
        }
    } else if (prev_element && next_element && prev_element->branch == next_element->branch) {
        LOG_TRACE("Link between elements");
        gst_element_unlink(prev_element->gst_element, next_element->gst_element);
        if (gst_element_link_many(prev_element->gst_element, current_element.gst_element, next_element->gst_element, NULL)) {
            LOG_DEBUG("Linked {} to {} to {}", prev_element->toString(), current_element.toString(), next_element->toString());
        } else {
            LOG_ERROR("Failed to link {} to {} to {}", prev_element->toString(), current_element.toString(), next_element->toString());
            return EXIT_FAILURE;
        }
        if (!gst_element_sync_state_with_parent(current_element.gst_element)) {
            LOG_ERROR("Failed to sync {} state with parent", current_element.toString());
        }
    }

    if (prev_element && prev_element->branch != current_element.branch){
        LOG_TRACE("Link first in branch");
        auto tee = findTeeElementForBranch(current_element.branch);
        if (linkElements(tee, current_element) == EXIT_SUCCESS) {
            LOG_DEBUG("Linked {} to {}", tee.toString(), current_element.toString());
        } else {
            LOG_ERROR("Failed to link {} to {}", tee.toString(), current_element.toString());
            return EXIT_FAILURE;
        }
    }

    current_element.is_enabled.second = true;

    return EXIT_SUCCESS;
}

int PipelineManager::createGstElement(PipelineElement& element) const {
    auto unique_gst_element_name = element.name + std::to_string(element.id);

    element.gst_element = gst_element_factory_make(element.name.c_str(), unique_gst_element_name.c_str());
    if (!element.gst_element) {
        LOG_ERROR("Failed to create pipeline element {}", element.toString());
        return EXIT_FAILURE;
    }
    if (!GST_IS_ELEMENT(element.gst_element)) {
        LOG_ERROR("Non valid pipeline element {}", element.toString());
        return EXIT_FAILURE;
    }

    for (const auto& [key, value]: element.properties) {
        gst_util_set_object_arg(G_OBJECT(element.gst_element), key.c_str(), value.c_str());
    }

    if (!gst_bin_add(GST_BIN(gst_pipeline_.get()), element.gst_element)) {
        LOG_ERROR("Failed to add element {} to pipeline", element.toString());
        return EXIT_FAILURE;
    }

    element.is_enabled.first = true;
    LOG_DEBUG("Created element: {}", element.toString());

    return EXIT_SUCCESS;
}

int PipelineManager::createGstPipeline(std::vector<PipelineElement>& pipeline) {
    for (auto& element: pipeline) {
        if (!element.is_optional) {
            if (createGstElement(element) != EXIT_SUCCESS) {
                return EXIT_FAILURE;
            }
        }
    }

    for (auto& element: pipeline) {
        if (element.is_enabled.first) {
            if (linkGstElement(element) != EXIT_SUCCESS) {
                return EXIT_FAILURE;
            }
        }
    }

    return EXIT_SUCCESS;
}

int PipelineManager::play() {
    if (createGstPipeline(pipeline_elements_) != EXIT_SUCCESS) {
        throw std::runtime_error("Failed to create pipeline");
    }

    LOG_DEBUG("Start playing");
    gst_element_set_state(gst_pipeline_.get(), GST_STATE_PLAYING);

    const auto bus = std::shared_ptr<GstBus>(gst_element_get_bus(gst_pipeline_.get()), gst_object_unref);
    gst_bus_add_watch(bus.get(), busCallback, this);

    gst_loop_ = std::shared_ptr<GMainLoop>(g_main_loop_new(nullptr, FALSE), g_main_loop_unref);
    if (!gst_loop_) {
        LOG_ERROR("Failed to create gstreamer main loop");
        return EXIT_FAILURE;
    }
    g_main_loop_run(gst_loop_.get());

    return EXIT_SUCCESS;
}

int PipelineManager::stop() const {
    if (gst_pipeline_ && gst_loop_) {
        g_main_loop_quit(gst_loop_.get());
        gst_element_set_state(gst_pipeline_.get(), GST_STATE_NULL);
        LOG_DEBUG("Stop playing");
        return EXIT_SUCCESS;
    }

    LOG_WARN("No stream playing");
    return EXIT_FAILURE;
}

gboolean PipelineManager::busCallback(GstBus*, GstMessage* message, gpointer data) {
    const auto pipeline_manager = static_cast<PipelineManager*>(data);
    switch (GST_MESSAGE_TYPE(message)) {
        case GST_MESSAGE_EOS:
            LOG_DEBUG("End of stream");
            return pipeline_manager->stop() == EXIT_SUCCESS ? TRUE : FALSE;
        case GST_MESSAGE_ERROR:
            GError* err;
            gchar* debug;
            gst_message_parse_error(message, &err, &debug);
            LOG_ERROR("{}", err->message);
            g_error_free(err);
            g_free(debug);
            return pipeline_manager->stop() == EXIT_SUCCESS ? TRUE : FALSE;
        default:
            break;
    }
    return TRUE;
}

PipelineElement* PipelineManager::getPreviousEnabledElement(const PipelineElement& element) {
    for (auto i = element.id - 1; i < pipeline_elements_.size(); --i) {
        if (pipeline_elements_.at(i).is_enabled.first) {
            return &pipeline_elements_.at(i);
        }
    }
    return {};
}

PipelineElement* PipelineManager::getNextEnabledElement(const PipelineElement& element) {
    for (auto i = element.id + 1; i != pipeline_elements_.size(); ++i) {
        if (pipeline_elements_.at(i).is_enabled.first) {
            return &pipeline_elements_.at(i);
        }
    }
    return {};
}

int PipelineManager::enableOptionalElement(PipelineElement& element) {
    std::lock_guard lock_guard(mutex_);

    if (createGstElement(element) == EXIT_FAILURE) {
        return EXIT_FAILURE;
    }

    if (linkGstElement(element) == EXIT_FAILURE) {
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}

int PipelineManager::disableOptionalElement(PipelineElement& element) {
    std::lock_guard lock_guard(mutex_);

    gst_element_set_state(element.gst_element, GST_STATE_NULL);

    const auto src_pad = std::shared_ptr<GstPad>(gst_element_get_static_pad(element.gst_element, "src"),
                                           gst_object_unref);
    if (!GST_IS_PAD(src_pad.get())) {
        LOG_ERROR("Failed to get src pad for element {}", element.toString());
        return EXIT_FAILURE;
    }

    const auto sink_pad = std::shared_ptr<GstPad>(gst_element_get_static_pad(element.gst_element, "video_sink"),
                                            gst_object_unref);
    if (!GST_IS_PAD(sink_pad.get())) {
        LOG_ERROR("Failed to get sink pad for element {}", element.toString());
        return EXIT_FAILURE;
    }

    gst_pad_add_probe(sink_pad.get(), GST_PAD_PROBE_TYPE_BLOCK_DOWNSTREAM,
                      [](GstPad*, GstPadProbeInfo*, gpointer) -> GstPadProbeReturn {
                          return GST_PAD_PROBE_OK;
                      }, nullptr, nullptr);

    gst_pad_add_probe(src_pad.get(), GST_PAD_PROBE_TYPE_BLOCK_DOWNSTREAM,
                      [](GstPad*, GstPadProbeInfo*, gpointer) -> GstPadProbeReturn {
                          return GST_PAD_PROBE_OK;
                      }, nullptr, nullptr);

    const auto previous_enabled_element = getPreviousEnabledElement(element);
    if (!previous_enabled_element || !GST_IS_ELEMENT(previous_enabled_element->gst_element)) {
        LOG_ERROR("Could not find previous element");
        return EXIT_FAILURE;
    }

    const auto next_enabled_element = getNextEnabledElement(element);
    if (!next_enabled_element || !GST_IS_ELEMENT(next_enabled_element->gst_element)) {
        LOG_ERROR("Could not find next element");
        return EXIT_FAILURE;
    }

    gst_element_unlink(previous_enabled_element->gst_element, element.gst_element);
    gst_element_unlink(element.gst_element, next_enabled_element->gst_element);
    LOG_DEBUG("Unlinked {} from {}", previous_enabled_element->toString(), element.toString());
    LOG_DEBUG("Unlinked {} from {}", element.toString(), next_enabled_element->toString());

    if (!gst_bin_remove(GST_BIN(gst_pipeline_.get()), element.gst_element)) {
        LOG_ERROR("Failed to remove element {} from pipeline", element.toString());
        return EXIT_FAILURE;
    }
    LOG_DEBUG("Removed element {}", element.toString());

    if (!gst_element_link(previous_enabled_element->gst_element, next_enabled_element->gst_element)) {
        LOG_ERROR("Failed to link previous element to next element");
        return EXIT_FAILURE;
    }
    LOG_DEBUG("Linked {} to {}", previous_enabled_element->toString(), next_enabled_element->toString());

    element.is_enabled.first = false;
    element.is_enabled.second = false;

    return EXIT_SUCCESS;
}

int PipelineManager::enableAllOptionalPipelineElements() {
    for (auto& element: pipeline_elements_) {
        if (element.is_optional && !element.is_enabled.first && !element.is_enabled.second) {
            if (createGstElement(element) == EXIT_FAILURE) {
                return EXIT_FAILURE;
            }
        }
    }

    for (auto& element: pipeline_elements_) {
        if (element.is_optional && element.is_enabled.first && !element.is_enabled.second) {
            if (linkGstElement(element) == EXIT_FAILURE) {
                return EXIT_FAILURE;
            }
        }
    }

    return EXIT_SUCCESS;
}

int PipelineManager::disableAllOptionalPipelineElements() {
    for (auto& element: pipeline_elements_) {
        if (element.is_optional && element.is_enabled.second) {
            disableOptionalElement(element);
        } else if (element.is_optional) {
            LOG_WARN("Element {} is already disabled", element.toString());
        }
    }
    return EXIT_SUCCESS;
}

int PipelineManager::enableOptionalPipelineElement(const std::string& element_name) {
    for (auto& element: pipeline_elements_) {
        if (element.name == element_name) {
            if (element.is_enabled.first && element.is_enabled.second) {
                LOG_WARN("Element {} is already enabled", element.toString());
                return EXIT_FAILURE;
            }
            return enableOptionalElement(element);
        }
    }
    return EXIT_FAILURE;
}

int PipelineManager::disableOptionalPipelineElement(const std::string& element_name) {
    for (auto& element: pipeline_elements_) {
        if (element.name == element_name) {
            if (!element.is_enabled.first && !element.is_enabled.second) {
                LOG_WARN("Element {} is already disabled", element.toString());
                return EXIT_FAILURE;
            }
            return disableOptionalElement(element);
        }
    }
    return EXIT_FAILURE;
}

std::vector<std::string> PipelineManager::getOptionalPipelineElementsNames() const {
    std::vector<std::string> elements_names;
    for (const auto& element: pipeline_elements_) {
        if (element.is_optional) {
            elements_names.push_back(element.name);
        }
    }
    return elements_names;
}

void PipelineManager::createElementsList(const std::string& file_path) {
    const auto pipeline_handler = std::make_unique<PipelineParser>(file_path);
    LOG_DEBUG("Use pipeline from: {}", file_path);
    pipeline_elements_ = pipeline_handler->getAllElements();
}

PipelineElement& PipelineManager::findTeeElementForBranch(const std::string& branch_name) {
    for (auto& element: pipeline_elements_) {
        if (element.name == "tee" && element.is_enabled.first && element.type == branch_name) {
            return element;
        }
    }

    throw std::runtime_error("Tee element not found");
}
