#include <utility>
#include <sstream>
#include <unordered_set>
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

int PipelineManager::linkElements(const PipelineElement& source, const PipelineElement& destination) {
    auto src_pad = gst_element_request_pad_simple(source.gst_element, "src_%u");
    if (!(src_pad && GST_PAD_IS_SRC(src_pad))) {
        src_pad = gst_element_get_static_pad(source.gst_element, "src");
        if (!(src_pad && GST_PAD_IS_SRC(src_pad))) {
            LOG_ERROR("Failed to get the src pad of the {}", source.name);
            return EXIT_FAILURE;
        }
    }

    auto sink_pad = gst_element_get_static_pad(destination.gst_element, "sink");
    if (!sink_pad) {
        sink_pad = gst_element_get_static_pad(destination.gst_element, "video_sink");
        if (!(sink_pad && GST_PAD_IS_SINK(sink_pad))) {
            LOG_ERROR("Failed to get the sink pad of the {}", destination.name);
            gst_object_unref(src_pad);
            return EXIT_FAILURE;
        }
    }

    if (gst_pad_link(src_pad, sink_pad) != GST_PAD_LINK_OK) {
        LOG_ERROR("Failed to link tee to element");
        gst_object_unref(src_pad);
        gst_object_unref(sink_pad);
        return EXIT_FAILURE;
    }

    // Unref pads to release references
    gst_object_unref(src_pad);
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

GstPadProbeReturn PipelineManager::connectGstElementProbeCallback(GstPad* pad, GstPadProbeInfo* info, gpointer data) {
    const auto element = static_cast<PipelineElement*>(data);

    auto peer = std::shared_ptr<GstPad>(gst_pad_get_peer(pad), gst_object_unref);
    if (peer == nullptr) {
        LOG_ERROR("Failed to get peer pad");
        return GST_PAD_PROBE_REMOVE;
    }

    /* Unlink pads */
    gst_pad_unlink(pad, peer.get());
    gst_element_sync_state_with_parent(element->gst_element);

    /* Connect new element */
    gst_element_link_pads(GST_ELEMENT(GST_OBJECT_PARENT(pad)), GST_OBJECT_NAME(pad),
                          element->gst_element, nullptr);
    gst_element_link_pads(element->gst_element, nullptr, GST_ELEMENT(GST_OBJECT_PARENT (peer.get())),
                          GST_OBJECT_NAME(peer.get()));

    return GST_PAD_PROBE_REMOVE;
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
        // TODO: finish implementing linking a new element between two elements with probe
        // const auto src_pad = std::shared_ptr<GstPad>(gst_element_get_static_pad(current_element.gst_element, "src"), gst_object_unref);
        // gst_pad_add_probe(src_pad.get(), GST_PAD_PROBE_TYPE_IDLE, connectGstElementProbeCallback, &current_element, nullptr);
        gst_element_unlink(prev_element->gst_element, next_element->gst_element);
        if (gst_element_link_many(prev_element->gst_element, current_element.gst_element, next_element->gst_element,
                                  NULL)) {
            LOG_DEBUG("Linked {} to {} to {}", prev_element->toString(), current_element.toString(),
                      next_element->toString());
        } else {
            LOG_ERROR("Failed to link {} to {} to {}", prev_element->toString(), current_element.toString(),
                      next_element->toString());
            return EXIT_FAILURE;
        }
        if (!gst_element_sync_state_with_parent(current_element.gst_element)) {
            LOG_ERROR("Failed to sync {} state with parent", current_element.toString());
        }
    }

    if (prev_element && prev_element->branch != current_element.branch) {
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

    gst_element_sync_state_with_parent(element.gst_element);

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

GstPadProbeReturn PipelineManager::disconnectGstElementProbeCallback(GstPad* src_peer, GstPadProbeInfo* info, gpointer data) {
    const auto pipeline_manager = static_cast<PipelineManager*>(data);

    const auto sink_pad = std::shared_ptr<GstPad>(gst_pad_get_peer(src_peer), gst_object_unref);
    if (!(sink_pad && GST_PAD_IS_SINK(sink_pad.get()))) {
        LOG_ERROR("Failed to get the sink_pad pad");
        return GST_PAD_PROBE_REMOVE;
    }

    const auto gst_element = std::shared_ptr<GstElement>(gst_pad_get_parent_element(sink_pad.get()), gst_object_unref);
    if (!(gst_element && GST_ELEMENT(gst_element.get()))) {
        LOG_ERROR("Failed to get element");
        return GST_PAD_PROBE_REMOVE;
    }

    const auto src_pad = std::shared_ptr<GstPad>(gst_element_get_static_pad(gst_element.get(), "src"), gst_object_unref);
    if (!src_pad) {
        LOG_TRACE("Element is a sink, no src pad to unlink");
        gst_element_set_state(gst_element.get(), GST_STATE_NULL);
        gst_bin_remove(GST_BIN(pipeline_manager->gst_pipeline_.get()), gst_element.get());
        return GST_PAD_PROBE_REMOVE;
    }

    if (!(src_pad && GST_PAD_IS_SRC(src_pad.get()))) {
        LOG_ERROR("Failed to get the src pad");
        return GST_PAD_PROBE_REMOVE;
    }

    const auto sink_peer = std::shared_ptr<GstPad>(gst_pad_get_peer(src_pad.get()), gst_object_unref);
    if (!(sink_peer && GST_PAD_IS_SINK(sink_peer.get()))) {
        LOG_ERROR("Failed to get the sink peer");
        return GST_PAD_PROBE_REMOVE;
    }

    gst_pad_unlink(src_peer, sink_pad.get());
    gst_pad_unlink(src_pad.get(), sink_peer.get());

    gst_pad_link(src_peer, sink_peer.get());

    gst_element_set_state(gst_element.get(), GST_STATE_NULL);
    gst_bin_remove(GST_BIN(pipeline_manager->gst_pipeline_.get()), gst_element.get());

    return GST_PAD_PROBE_REMOVE;
}

int PipelineManager::disableOptionalElement(PipelineElement& element) const {
    std::lock_guard lock_guard(mutex_);

    LOG_DEBUG("Disable element: {}", element.toString());

    auto sink_pad = std::shared_ptr<GstPad>(gst_element_get_static_pad(element.gst_element, "video_sink"), gst_object_unref);
    if (!sink_pad || !GST_IS_PAD(sink_pad.get())) {
        sink_pad = std::shared_ptr<GstPad>(gst_element_get_static_pad(element.gst_element, "sink"), gst_object_unref);
        if (!sink_pad || !GST_IS_PAD(sink_pad.get())) {
            LOG_ERROR("Failed to get sink pad for element {}", element.toString());
            return EXIT_FAILURE;
        }
    }

    const auto src_peer = std::shared_ptr<GstPad>(gst_pad_get_peer(sink_pad.get()), gst_object_unref);
    if (!src_peer) {
        LOG_ERROR("Failed to get src peer for element {}", element.toString());
        return EXIT_FAILURE;
    }

    gst_pad_add_probe(src_peer.get(), GST_PAD_PROBE_TYPE_IDLE, disconnectGstElementProbeCallback,
                      const_cast<PipelineManager*>(this), nullptr);

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

int PipelineManager::enableOptionalPipelineBranch(const std::string& branch_name) {
    for (auto& element: pipeline_elements_) {
        if (element.is_optional && !element.is_enabled.first && !element.is_enabled.second && element.branch == branch_name) {
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

int PipelineManager::disableOptionalPipelineBranch(const std::string& branch_name) {
    for (auto& element: pipeline_elements_) {
        if (element.is_optional && element.is_enabled.second && element.branch == branch_name) {
            disableOptionalElement(element);
        } else if (element.is_optional && element.branch == branch_name) {
            LOG_WARN("Element {} is already disabled", element.toString()); //FIXME: handle to branch, not element
        }
    }
    return EXIT_SUCCESS;
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

std::vector<std::string> PipelineManager::getOptionalPipelineBranchesNames() const {
    std::vector<std::string> branches_names;
    std::unordered_set<std::string> unique_branches;
    for (const auto& element: pipeline_elements_) {
        if (element.is_optional && unique_branches.insert(element.branch).second) {
            branches_names.push_back(element.branch);
        }
    }
    return branches_names;
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
