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

int PipelineManager::linkTeeToElement(const PipelineElement* tee, const PipelineElement* sink) {
    if (!GST_IS_ELEMENT(tee->gst_element) || !GST_IS_ELEMENT(sink->gst_element)) {
        LOG_ERROR("One or more elements are invalid");
        return EXIT_FAILURE;
    }

    // Request a new source pad from the tee
    auto tee_src_pad = gst_element_request_pad_simple(tee->gst_element, "src_%u");
    if (!(tee_src_pad && GST_PAD_IS_SRC(tee_src_pad))) {
        LOG_ERROR("Failed to request a new pad from {}", tee->name);
        return EXIT_FAILURE;
    }

    // GList* free_sink_pads = nullptr;
    // gst_element_foreach_pad(sink->gst_element, [](GstElement* element, GstPad* pad, gpointer user_data) -> gboolean {
    //     if (GST_PAD_IS_SINK(pad) && !gst_pad_is_linked(pad)) {
    //         GList** free_pads = static_cast<GList**>(user_data);
    //         *free_pads = g_list_append(*free_pads, g_strdup(gst_pad_get_name(pad)));
    //     }
    //     return TRUE;
    // }, &free_sink_pads);
    //
    // for (GList* l = free_sink_pads; l != nullptr; l = l->next) {
    //     gchar* pad_name = static_cast<gchar*>(l->data);
    //     LOG_INFO("Free sink pad: {}", pad_name);
    //     g_free(pad_name);
    // }
    // g_list_free(free_sink_pads);

    // Get the sink pad of the downstream element
    auto sink_pad = gst_element_get_static_pad(sink->gst_element, "sink");
    if (!sink_pad) {
        sink_pad = gst_element_get_static_pad(sink->gst_element, "video_sink");
        if (!(sink_pad && GST_PAD_IS_SINK(sink_pad))) {
            LOG_ERROR("Failed to get the sink pad of the {}", sink->name);
            return EXIT_FAILURE;
        }
    }

    // Link the tee's src pad to the sink pad
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

int PipelineManager::linkAllGstElements() {
    LOG_TRACE("Link all default elements");

    for (auto& current_element: pipeline_elements_) {
        if (current_element.is_enabled) {
            auto next_element = getNextEnabledElement(current_element);
            auto prev_element = getPreviousEnabledElement(current_element);

            if (next_element && next_element->gst_element) {
                if (current_element.name == "tee") { // link tee to next element
                    if (linkTeeToElement(&current_element, next_element) == EXIT_SUCCESS) {
                        LOG_INFO("Linked {}({}) to {}({})", current_element.name, current_element.branch,
                                 next_element->name, next_element->branch);
                    } else {
                        LOG_ERROR("Failed to link {}({}) to {}({})", current_element.name, current_element.branch,
                                 next_element->name, next_element->branch);
                        return EXIT_FAILURE;
                    }
                } else if (current_element.branch == next_element->branch) { // any element inside the branch that is not sink
                    if (gst_element_link(current_element.gst_element, next_element->gst_element)) {
                        LOG_INFO("Linked {} to {}", current_element.name, next_element->name);
                    } else {
                        LOG_ERROR("Failed to link element {} to {}", current_element.name, next_element->name);
                        return EXIT_FAILURE;
                    }
                }
            }

            if (prev_element && prev_element->gst_element) { //first element in branch
                if (current_element.branch != prev_element->branch) {
                    auto tee = findTeeElementForBranch(current_element.branch);
                    if (linkTeeToElement(&tee, &current_element) == EXIT_SUCCESS) {
                        LOG_INFO("Linked {}({}) to {}({})", tee.name, tee.branch, current_element.name,
                                 current_element.branch);
                    } else {
                        LOG_ERROR("Failed to link {}({}) to {}({})", tee.name, tee.branch, current_element.name,
                                  current_element.branch);
                        return EXIT_FAILURE;
                    }
                }
            }

            //next, when current element is sink
        }
    }

    return EXIT_SUCCESS;
}

int PipelineManager::createGstElement(PipelineElement& element) const {
    element.print();
    // TODO: handle the case when there are multiple elements with the same name in the same branch
    auto unique_gst_element_name = element.name + std::to_string(element.id);

    element.gst_element = gst_element_factory_make(element.name.c_str(), unique_gst_element_name.c_str());
    if (!element.gst_element) {
        LOG_ERROR("Failed to create pipeline element {}", element.name);
        return EXIT_FAILURE;
    }
    if (!GST_IS_ELEMENT(element.gst_element)) {
        LOG_ERROR("Non valid pipeline element {}", element.name);
        return EXIT_FAILURE;
    }

    for (const auto& [key, value]: element.properties) {
        gst_util_set_object_arg(G_OBJECT(element.gst_element), key.c_str(), value.c_str());
    }

    if (!gst_bin_add(GST_BIN(gst_pipeline_.get()), element.gst_element)) {
        LOG_ERROR("Failed to add element {} to pipeline", element.name);
        return EXIT_FAILURE;
    }

    element.is_enabled = true;

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

    if (linkAllGstElements() != EXIT_SUCCESS) {
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}

int PipelineManager::play() {
    if (createGstPipeline(pipeline_elements_) != EXIT_SUCCESS) {
        throw std::runtime_error("Failed to create pipeline");
    }

    LOG_INFO("Start playing");
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
        LOG_INFO("Stop playing");
        return EXIT_SUCCESS;
    }

    LOG_WARN("No stream playing");
    return EXIT_FAILURE;
}

gboolean PipelineManager::busCallback(GstBus*, GstMessage* message, gpointer data) {
    const auto pipeline_manager = static_cast<PipelineManager*>(data);
    switch (GST_MESSAGE_TYPE(message)) {
        case GST_MESSAGE_EOS:
            LOG_INFO("End of stream");
            return pipeline_manager->stop() == EXIT_SUCCESS ? TRUE : FALSE;
        case GST_MESSAGE_ERROR:
            GError* err;
            gchar* debug;
            gst_message_parse_error(message, &err, &debug);
            LOG_ERROR("Error: {}", err->message);
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
        if (pipeline_elements_.at(i).is_enabled) {
            return &pipeline_elements_.at(i);
        }
    }
    return {};
}

PipelineElement* PipelineManager::getNextEnabledElement(const PipelineElement& element) {
    for (auto i = element.id + 1; i != pipeline_elements_.size(); ++i) {
        if (pipeline_elements_.at(i).is_enabled) {
            return &pipeline_elements_.at(i);
        }
    }
    return {};
}

int PipelineManager::enableElement(PipelineElement& element) {
    std::lock_guard lock_guard(mutex_);

    LOG_INFO("Enable element: {}", element.name);

    element.gst_element = gst_element_factory_make(element.name.c_str(), element.name.c_str());
    if (!element.gst_element) {
        LOG_ERROR("Failed to create pipeline element {}", element.name);
        return EXIT_FAILURE;
    }
    if (!GST_IS_ELEMENT(element.gst_element)) {
        LOG_ERROR("Non valid pipeline element {}", element.name);
        return EXIT_FAILURE;
    }

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

    gst_bin_add(GST_BIN(gst_pipeline_.get()), element.gst_element);

    gst_element_unlink(previous_enabled_element->gst_element, next_enabled_element->gst_element);
    gst_element_link_many(previous_enabled_element->gst_element, element.gst_element,
                          next_enabled_element->gst_element, NULL);

    gst_element_sync_state_with_parent(element.gst_element);
    element.is_enabled = true;

    return EXIT_SUCCESS;
}

int PipelineManager::disableElement(PipelineElement& element) {
    std::lock_guard lock_guard(mutex_);

    LOG_INFO("Disable element: {}", element.name);

    gst_element_set_state(element.gst_element, GST_STATE_NULL);

    const auto src_pad = std::shared_ptr<GstPad>(gst_element_get_static_pad(element.gst_element, "src"),
                                           gst_object_unref);
    if (!GST_IS_PAD(src_pad.get())) {
        LOG_ERROR("Failed to get src pad for element {}", element.name);
        return EXIT_FAILURE;
    }

    const auto sink_pad = std::shared_ptr<GstPad>(gst_element_get_static_pad(element.gst_element, "video_sink"),
                                            gst_object_unref);
    if (!GST_IS_PAD(sink_pad.get())) {
        LOG_ERROR("Failed to get sink pad for element {}", element.name);
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

    if (!gst_bin_remove(GST_BIN(gst_pipeline_.get()), element.gst_element)) {
        LOG_ERROR("Failed to remove element {} from pipeline", element.name);
        return EXIT_FAILURE;
    }

    if (!gst_element_link(previous_enabled_element->gst_element, next_enabled_element->gst_element)) {
        LOG_ERROR("Failed to link previous element to next element");
        return EXIT_FAILURE;
    }

    element.is_enabled = false;

    return EXIT_SUCCESS;
}

int PipelineManager::enableAllOptionalPipelineElements() {
    for (auto& element: pipeline_elements_) {
        if (element.is_optional && !element.is_enabled) {
            enableElement(element);
        } else if (element.is_optional) {
            LOG_WARN("Element {} is already enabled", element.name);
        }
    }
    return EXIT_SUCCESS;
}

int PipelineManager::disableAllOptionalPipelineElements() {
    for (auto& element: pipeline_elements_) {
        if (element.is_optional && element.is_enabled) {
            disableElement(element);
        } else if (element.is_optional) {
            LOG_WARN("Element {} is already disabled", element.name);
        }
    }
    return EXIT_SUCCESS;
}

int PipelineManager::enableOptionalPipelineElement(const std::string& element_name) {
    for (auto& element: pipeline_elements_) {
        if (element.name == element_name) {
            if (element.is_enabled) {
                LOG_WARN("Element {} is already enabled", element.name);
                return EXIT_FAILURE;
            }
            return enableElement(element);
        }
    }
    return EXIT_FAILURE;
}

int PipelineManager::disableOptionalPipelineElement(const std::string& element_name) {
    for (auto& element: pipeline_elements_) {
        if (element.name == element_name) {
            if (!element.is_enabled) {
                LOG_WARN("Element {} is already disabled", element.name);
                return EXIT_FAILURE;
            }
            return disableElement(element);
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
        if (element.name == "tee" && element.is_enabled && element.type == branch_name) {
            return element;
        }
    }

    LOG_CRITICAL("Proper tee element for {} not found", branch_name);
}
