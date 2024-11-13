#include "Gstreamer.h"
#include <utility>
#include <sstream>
#include "Gstreamer/PipelineHandler.h"

Gstreamer::Gstreamer(std::string pipeline_file) : pipeline_file_(std::move(pipeline_file)) {
    LOG_TRACE("Gstreamer constructor");
    LOG_DEBUG("Init gstreamer");

    gst_init(nullptr, nullptr);

    gst_pipeline_ = std::shared_ptr<GstElement>(gst_pipeline_new("runtime-control-pipeline"), gst_object_unref);
    if (!GST_IS_ELEMENT(gst_pipeline_.get())) {
        LOG_ERROR("Failed to create pipeline");
    }

    pipeline_elements_ = createElementsList(pipeline_file_);
    createGstPipeline(pipeline_elements_);
}

Gstreamer::~Gstreamer() {
    LOG_TRACE("Gstreamer destructor");
}

int Gstreamer::linkAllGstElements() {
    for (const auto& element: pipeline_elements_) {
        if (element.enabled) {
            auto next_element = getNextEnabledElement(element);
            if (next_element.gst_element.get()) {
                if (!gst_element_link(element.gst_element.get(), next_element.gst_element.get())) {
                    LOG_ERROR("Failed to link element {} to {}", element.name, next_element.name);
                    return EXIT_FAILURE;
                }
            }
        }
    }

    return EXIT_SUCCESS;
}

int Gstreamer::createGstElement(PipelineElement& element) {
    element.gst_element = std::shared_ptr<GstElement>(
        gst_element_factory_make(element.name.c_str(), element.name.c_str()));
    if (!element.gst_element) {
        LOG_ERROR("Failed to create pipeline element {}", element.name);
        return EXIT_FAILURE;
    } else if (!GST_IS_ELEMENT(element.gst_element.get())) {
        LOG_ERROR("Non valid pipeline element {}", element.name);
        return EXIT_FAILURE;
    }

    for (const auto& [key, value]: element.properties) {
        gst_util_set_object_arg(G_OBJECT(element.gst_element.get()), key.c_str(), value.c_str());
    }

    if (!gst_bin_add(GST_BIN(gst_pipeline_.get()), element.gst_element.get())) {
        LOG_ERROR("Failed to add element {} to pipeline", gst_element_get_name(element.gst_element.get()));
        return EXIT_FAILURE;
    }

    element.enabled = true;
    printElement(element);

    return EXIT_SUCCESS;
}

int Gstreamer::createGstPipeline(std::vector<PipelineElement>& pipeline) {
    for (auto& element: pipeline) {
        if (!element.optional) {
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

void Gstreamer::printElement(const PipelineElement& element) {
    std::ostringstream oss;
    oss << element;
    LOG_INFO("Enable element: {}", oss.str());
}

void Gstreamer::play() {
    LOG_INFO("Start playing");
    gst_element_set_state(gst_pipeline_.get(), GST_STATE_PLAYING);

    auto bus = std::shared_ptr<GstBus>(gst_element_get_bus(gst_pipeline_.get()), gst_object_unref);
    gst_bus_add_watch(bus.get(), [](GstBus*, GstMessage* message, const gpointer data) -> gboolean {
        if (GST_MESSAGE_TYPE(message) == GST_MESSAGE_EOS) {
            static_cast<Gstreamer*>(data)->stop();
        }
        return TRUE;
    }, this);

    gst_loop_ = std::shared_ptr<GMainLoop>(g_main_loop_new(nullptr, FALSE), g_main_loop_unref);
    g_main_loop_run(gst_loop_.get());
}

void Gstreamer::stop() {
    if (gst_pipeline_.get() && gst_loop_.get()) {
        LOG_INFO("Stop playing");
        g_main_loop_quit(gst_loop_.get());

        gst_element_set_state(gst_pipeline_.get(), GST_STATE_NULL);
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

    // gst_bin_remove() unrefs the element so no need to unref it in custom deleter
    element.gst_element = std::shared_ptr<GstElement>(
        gst_element_factory_make(element.name.c_str(), element.name.c_str()),
        [](GstElement* ptr) {
        });
    if (!element.gst_element) {
        LOG_ERROR("Failed to create pipeline element {}", element.name);
        return EXIT_FAILURE;
    } else if (!GST_IS_ELEMENT(element.gst_element.get())) {
        LOG_ERROR("Non valid pipeline element {}", element.name);
        return EXIT_FAILURE;
    }

    const auto previous_enabled_element = getPreviousEnabledElement(element);
    if (!GST_IS_ELEMENT(previous_enabled_element.gst_element.get())) {
        LOG_ERROR("Could not find previous element");
        return EXIT_FAILURE;
    }

    const auto next_enabled_element = getNextEnabledElement(element);
    if (!GST_IS_ELEMENT(next_enabled_element.gst_element.get())) {
        LOG_ERROR("Could not find next element");
        return EXIT_FAILURE;
    }

    gst_bin_add(GST_BIN(gst_pipeline_.get()), element.gst_element.get());

    gst_element_unlink(previous_enabled_element.gst_element.get(), next_enabled_element.gst_element.get());
    gst_element_link_many(previous_enabled_element.gst_element.get(), element.gst_element.get(),
                          next_enabled_element.gst_element.get(), NULL);

    gst_element_sync_state_with_parent(element.gst_element.get());
    element.enabled = true;

    return EXIT_SUCCESS;
}

int Gstreamer::disableOptionalElement(PipelineElement& element) const {
    std::lock_guard lock_guard(mutex_);

    LOG_INFO("Disable element: {}", element.name);

    gst_element_set_state(element.gst_element.get(), GST_STATE_NULL);

    auto src_pad = std::shared_ptr<GstPad>(gst_element_get_static_pad(element.gst_element.get(), "src"),
                                           gst_object_unref);
    if (!GST_IS_PAD(src_pad.get())) {
        LOG_ERROR("Failed to get src pad for element {}", element.name);
        return EXIT_FAILURE;
    }

    auto sink_pad = std::shared_ptr<GstPad>(gst_element_get_static_pad(element.gst_element.get(), "video_sink"),
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
    if (!GST_IS_ELEMENT(previous_enabled_element.gst_element.get())) {
        LOG_ERROR("Could not find previous element");
        return EXIT_FAILURE;
    }

    const auto next_enabled_element = getNextEnabledElement(element);
    if (!GST_IS_ELEMENT(next_enabled_element.gst_element.get())) {
        LOG_ERROR("Could not find next element");
        return EXIT_FAILURE;
    }

    gst_element_unlink(previous_enabled_element.gst_element.get(), element.gst_element.get());
    gst_element_unlink(element.gst_element.get(), next_enabled_element.gst_element.get());

    if (!gst_bin_remove(GST_BIN(gst_pipeline_.get()), element.gst_element.get())) {
        LOG_ERROR("Failed to remove element {} from pipeline", element.name);
        return EXIT_FAILURE;
    }

    if (!gst_element_link(previous_enabled_element.gst_element.get(), next_enabled_element.gst_element.get())) {
        LOG_ERROR("Failed to link previous element to next element");
        return EXIT_FAILURE;
    }

    element.enabled = false;

    return EXIT_SUCCESS;
}

int Gstreamer::enableAllOptionalPipelineElements() {
    for (auto& element: pipeline_elements_) {
        if (element.optional && !element.enabled) {
            enableOptionalElement(element);
        } else if (element.optional) {
            LOG_WARN("Element {} is already enabled", element.name);
        }
    }
    return EXIT_SUCCESS;
}

int Gstreamer::disableAllOptionalPipelineElements() {
    for (auto& element: pipeline_elements_) {
        if (element.optional && element.enabled) {
            disableOptionalElement(element);
        } else if (element.optional) {
            LOG_WARN("Element {} is already disabled", element.name);
        }
    }
    return EXIT_SUCCESS;
}

int Gstreamer::enableOptionalPipelineElement(const std::string& element_name) {
    for (auto& element: pipeline_elements_) {
        if (element.name == element_name) {
            if (element.enabled) {
                LOG_WARN("Element {} is already enabled", element.name);
                return EXIT_FAILURE;
            }
            return enableOptionalElement(element);
        }
    }
    return EXIT_FAILURE;
}

int Gstreamer::disableOptionalPipelineElement(const std::string& element_name) {
    for (auto& element: pipeline_elements_) {
        if (element.name == element_name) {
            if (!element.enabled) {
                LOG_WARN("Element {} is already disabled", element.name);
                return EXIT_FAILURE;
            }
            return disableOptionalElement(element);
        }
    }
    return EXIT_FAILURE;
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

std::vector<PipelineElement> Gstreamer::createElementsList(const std::string& file_path) {
    auto pipeline_handler = std::make_unique<PipelineHandler>(file_path);
    LOG_DEBUG("Use pipeline from: {}", file_path);
    return pipeline_handler->getAllElements();
}
