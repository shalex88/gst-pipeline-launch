#include "Gstreamer.h"
#include <utility>
#include <sstream>
#include "Gstreamer/PipelineHandler.h"

Gstreamer::Gstreamer(std::string pipeline_file) : pipeline_file_(std::move(pipeline_file)) {
    LOG_TRACE("Gstreamer constructor");
    LOG_DEBUG("Init gstreamer");

    gst_init(nullptr, nullptr);

    gst_pipeline_ = gst_pipeline_new("runtime-control-pipeline");
    if (!gst_pipeline_) {
        LOG_ERROR("Failed to create pipeline");
    }

    pipeline_elements_ = createElementsList(pipeline_file_);
    createGstPipeline(pipeline_elements_);
}

Gstreamer::~Gstreamer() {
    LOG_TRACE("Gstreamer destructor");
}

void Gstreamer::linkAllGstElements() {
    for (const auto& element: pipeline_elements_) {
        if (element.enabled) {
            auto next_element = getNextEnabledElement(element);
            if (next_element.gst_element) {
                if (!gst_element_link(element.gst_element, next_element.gst_element)) {
                    LOG_ERROR("Failed to link element {} to {}", element.name, next_element.name);
                }
            }
        }
    }
}

void Gstreamer::createGstElement(PipelineElement& element) {
    element.gst_element = gst_element_factory_make(element.name.c_str(), element.name.c_str());
    if (!element.gst_element) {
        LOG_ERROR("Failed to create pipeline element {}", element.name);
    }

    for (const auto& [key, value]: element.properties) {
        gst_util_set_object_arg(G_OBJECT(element.gst_element), key.c_str(), value.c_str());
    }

    if (!gst_bin_add(GST_BIN(gst_pipeline_), element.gst_element)) {
        LOG_ERROR("Failed to add element {} to pipeline", gst_element_get_name(element.gst_element));
    }

    element.enabled = true;

    printElement(element);
}

void Gstreamer::createGstPipeline(std::vector<PipelineElement>& pipeline) {
    for (auto& element: pipeline) {
        if (!element.optional) {
            createGstElement(element);
        }
    }

    linkAllGstElements();
}

void Gstreamer::printElement(const PipelineElement& element) {
    std::ostringstream oss;
    oss << element;
    LOG_INFO("Enable element: {}", oss.str());
}

void Gstreamer::play() {
    LOG_INFO("Start playing");
    gst_element_set_state(gst_pipeline_, GST_STATE_PLAYING);

    // Add a bus watch to listen for EOS
    GstBus* bus = gst_element_get_bus(gst_pipeline_);
    gst_bus_add_watch(bus, [](GstBus*, GstMessage* message, const gpointer data) -> gboolean {
        if (GST_MESSAGE_TYPE(message) == GST_MESSAGE_EOS) {
            static_cast<Gstreamer*>(data)->stop();
        }
        return TRUE;
    }, this);
    gst_object_unref(bus);

    // Start the main loop in a new thread
    gst_loop_ = g_main_loop_new(nullptr, FALSE);

    g_main_loop_run(gst_loop_);
}

void Gstreamer::stop() {
    if (gst_pipeline_ && gst_loop_) {
        LOG_INFO("Stop playing");
        g_main_loop_quit(gst_loop_);

        gst_element_set_state(gst_pipeline_, GST_STATE_NULL);
        gst_object_unref(gst_pipeline_);
        g_main_loop_unref(gst_loop_);
        gst_loop_ = nullptr;
        gst_pipeline_ = nullptr;
    } else {
        LOG_WARN("No stream playing");
    }
}

PipelineElement Gstreamer::getPreviousEnabledElement(const PipelineElement& element) const {
    for (auto i = element.id - 1; i != pipeline_elements_.size(); --i) {
        if (pipeline_elements_.at(i).enabled) {
            return pipeline_elements_.at(i);
        }
    }

    return {};
}

PipelineElement Gstreamer::getNextEnabledElement(const PipelineElement& element) const {
    for (auto i = element.id + 1; i != pipeline_elements_.size(); ++i) {
        if (pipeline_elements_.at(i).enabled) {
            return pipeline_elements_.at(i);
        }
    }

    return {};
}

int Gstreamer::enableOptionalElement(PipelineElement& element) const {
    std::lock_guard lock_guard(mutex_);

    LOG_INFO("Enable element: {}", element.name);

    element.gst_element = gst_element_factory_make(element.name.c_str(), element.name.c_str());
    if (!element.gst_element) {
        LOG_ERROR("Failed to create pipeline element {}", element.name);
        return EXIT_FAILURE;
    }

    const auto previous_gst_element = getPreviousEnabledElement(element).gst_element;
    if (!previous_gst_element) {
        LOG_ERROR("Could not find previous element");
        return EXIT_FAILURE;
    }

    const auto next_gst_element = getNextEnabledElement(element).gst_element;
    if (!next_gst_element) {
        LOG_ERROR("Could not find next element");
        return EXIT_FAILURE;
    }

    gst_bin_add(GST_BIN(gst_pipeline_), element.gst_element);

    gst_element_unlink(previous_gst_element, next_gst_element);
    gst_element_link_many(previous_gst_element, element.gst_element, next_gst_element, NULL);

    gst_element_sync_state_with_parent(element.gst_element);

    element.enabled = true;

    return EXIT_SUCCESS;
}

int Gstreamer::disableOptionalElement(PipelineElement& element) const {
    std::lock_guard lock_guard(mutex_);

    LOG_INFO("Disable element: {}", element.name);

    element.gst_element = gst_bin_get_by_name(GST_BIN(gst_pipeline_), pipeline_elements_[element.id].name.c_str());

    gst_element_set_state(element.gst_element, GST_STATE_NULL);

    GstPad* src_pad = gst_element_get_static_pad(element.gst_element, "src");
    GstPad* sink_pad = gst_element_get_static_pad(element.gst_element, "sink");

    gst_pad_add_probe(sink_pad, GST_PAD_PROBE_TYPE_BLOCK_DOWNSTREAM,
                      [](GstPad*, GstPadProbeInfo*, gpointer) -> GstPadProbeReturn {
                          return GST_PAD_PROBE_OK;
                      }, nullptr, nullptr);

    gst_pad_add_probe(src_pad, GST_PAD_PROBE_TYPE_BLOCK_DOWNSTREAM,
                      [](GstPad*, GstPadProbeInfo*, gpointer) -> GstPadProbeReturn {
                          return GST_PAD_PROBE_OK;
                      }, nullptr, nullptr);

    const auto previous_gst_element = getPreviousEnabledElement(element).gst_element;
    if (!previous_gst_element) {
        LOG_ERROR("Could not find previous element");
        return EXIT_FAILURE;
    }

    const auto next_gst_element = getNextEnabledElement(element).gst_element;
    if (!next_gst_element) {
        LOG_ERROR("Could not find next element");
        return EXIT_FAILURE;
    }

    gst_element_unlink(previous_gst_element, element.gst_element);
    gst_element_unlink(element.gst_element, next_gst_element);
    gst_bin_remove(GST_BIN(gst_pipeline_), element.gst_element);

    gst_element_link(previous_gst_element, next_gst_element);

    gst_object_unref(sink_pad);
    gst_object_unref(src_pad);

    element.enabled = false;
    element.gst_element = nullptr;

    return EXIT_SUCCESS;
}

int Gstreamer::enableAllOptionalPipelineElements() {
    for (auto& element: pipeline_elements_) {
        if (element.optional) {
            if (!element.enabled) {
                enableOptionalElement(element);
            } else {
                LOG_WARN("Element {} is already enabled", element.name);
            }
        }
    }

    return EXIT_SUCCESS;
}

int Gstreamer::disableAllOptionalPipelineElements() {
    for (auto& element: pipeline_elements_) {
        if (element.optional) {
            if (element.enabled) {
                disableOptionalElement(element);
            } else {
                LOG_WARN("Element {} is already disabled", element.name);
            }
        }
    }

    return EXIT_SUCCESS;
}

int Gstreamer::enableOptionalPipelineElement(const std::string& element_name) {
    auto element = findElement(element_name);
    if (element.name.empty()) {
        LOG_ERROR("Element {} not found", element_name);
        return EXIT_FAILURE;
    }

    if (element.enabled) {
        LOG_WARN("Element {} is already enabled", element_name);
        return EXIT_SUCCESS;
    }

    return enableOptionalElement(element);
}

int Gstreamer::disableOptionalPipelineElement(const std::string& element_name) {
    auto element = findElement(element_name);
    if (element.name.empty()) {
        LOG_ERROR("Element {} not found", element_name);
        return EXIT_FAILURE;
    }

    if (!element.enabled) {
        LOG_WARN("Element {} is already disabled", element_name);
        return EXIT_SUCCESS;
    }

    return disableOptionalElement(element);
}

std::vector<std::string> Gstreamer::getOptionalPipelineElementsNames() const {
    std::vector<std::string> elements_names;
    for (const auto& element: pipeline_elements_) {
        if (element.optional) {
            elements_names.push_back(element.name);
        }
    }

    return elements_names;
}

PipelineElement Gstreamer::findElement(const std::string& element_name) {
    for (const auto& element: pipeline_elements_) {
        if (element.name == element_name) {
            return element;
        }
    }

    return {};
}

std::vector<PipelineElement> Gstreamer::createElementsList(const std::string& file_path) {
    const auto pipeline_handler = std::make_unique<PipelineHandler>(file_path);
    LOG_DEBUG("Use pipeline from: {}", file_path);

    return pipeline_handler->getAllElements();
}
