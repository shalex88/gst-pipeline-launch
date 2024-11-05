#include <utility>
#include <sstream>
#include "Gstreamer/PipelineHandler.h"
#include "Gstreamer.h"

Gstreamer::Gstreamer(std::string pipeline_file) : pipeline_file_(std::move(pipeline_file)) {
    LOG_TRACE("Gstreamer constructor");
    LOG_DEBUG("Init gstreamer");

    gst_init(nullptr, nullptr);

    gst_pipeline_ = gst_pipeline_new("runtime-control-pipeline");
    if (!gst_pipeline_) {
        LOG_ERROR("Failed to create pipeline");
    }

    pipeline_elements_ = create_elements_list(pipeline_file_);
    create_gst_pipeline(pipeline_elements_);
}

Gstreamer::~Gstreamer() {
    LOG_TRACE("Gstreamer destructor");
}

void Gstreamer::link_all_gst_elements() {
    for (const auto& element : pipeline_elements_) {
        if (element.enabled) {
            auto next_element = get_next_enabled_element(element);
            if (next_element.gst_element) {
                if (!gst_element_link(element.gst_element, next_element.gst_element)) {
                    LOG_ERROR("Failed to link element {} to {}", element.name, next_element.name);
                }
            }
        }
    }
}

void Gstreamer::create_gst_element(PipelineElement& element) {
    element.gst_element = gst_element_factory_make(element.name.c_str(), element.name.c_str());
    if (!element.gst_element) {
        LOG_ERROR("Failed to create pipeline element {}", element.name);
    }

    for (const auto& [key, value] : element.properties) {
        gst_util_set_object_arg(G_OBJECT(element.gst_element), key.c_str(),  value.c_str());
    }

    if (!gst_bin_add(GST_BIN(gst_pipeline_), element.gst_element)) {
        LOG_ERROR("Failed to add element {} to pipeline", gst_element_get_name(element.gst_element));
    }

    element.enabled = true;

    print_element(element);
}

void Gstreamer::create_gst_pipeline(std::vector<PipelineElement>& pipeline) {
    for (auto& element: pipeline) {
        if(!element.optional) {
            create_gst_element(element);
        }
    }

    link_all_gst_elements();
}

void Gstreamer::print_element(const PipelineElement& element) {
    std::ostringstream oss;
    oss << element;
    LOG_INFO("Enable: {}", oss.str());
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
    LOG_INFO("Stop playing");
    g_main_loop_quit(gst_loop_);

    gst_element_set_state(gst_pipeline_, GST_STATE_NULL);
    gst_object_unref(gst_pipeline_);
    g_main_loop_unref(gst_loop_);
}

PipelineElement Gstreamer::get_previous_enabled_element(const PipelineElement& element) const {
    for (auto i = element.id - 1; i != pipeline_elements_.size(); --i) {
        if (pipeline_elements_.at(i).enabled) {
            return pipeline_elements_.at(i);
        }
    }

    return {};
}

PipelineElement Gstreamer::get_next_enabled_element(const PipelineElement& element) const {
    for (auto i = element.id + 1; i != pipeline_elements_.size(); ++i) {
        if (pipeline_elements_.at(i).enabled) {
            return pipeline_elements_.at(i);
        }
    }

    return {};
}

void Gstreamer::enable_optional_element(PipelineElement& element) const {
    std::lock_guard lock_guard(mutex_);

    LOG_INFO("Enable element: {}", element.name);

    element.gst_element = gst_element_factory_make(element.name.c_str(), element.name.c_str());
    if (!element.gst_element) {
        LOG_ERROR("Failed to create pipeline element {}", element.name);
    }

    const auto previous_gst_element = get_previous_enabled_element(element).gst_element;
    if (!previous_gst_element) {
        LOG_ERROR("Could not find previous element");
    }

    const auto next_gst_element = get_next_enabled_element(element).gst_element;
    if (!next_gst_element) {
        LOG_ERROR("Could not find next element");
    }

    gst_bin_add(GST_BIN(gst_pipeline_), element.gst_element);

    gst_element_unlink(previous_gst_element, next_gst_element);
    gst_element_link_many(previous_gst_element, element.gst_element, next_gst_element, NULL);

    gst_element_sync_state_with_parent(element.gst_element);

    element.enabled = true;
}

void Gstreamer::disable_optional_element(PipelineElement& element) const {
    std::lock_guard lock_guard(mutex_);

    LOG_INFO("Disable element: {}", element.name);

    element.gst_element = gst_bin_get_by_name(GST_BIN(gst_pipeline_), pipeline_elements_[element.id].name.c_str());

    gst_element_set_state(element.gst_element, GST_STATE_NULL);

    GstPad* src_pad = gst_element_get_static_pad(element.gst_element, "src");
    GstPad* sink_pad = gst_element_get_static_pad(element.gst_element, "sink");

    gst_pad_add_probe(sink_pad, GST_PAD_PROBE_TYPE_BLOCK_DOWNSTREAM, [](GstPad*, GstPadProbeInfo*, gpointer) -> GstPadProbeReturn {
        return GST_PAD_PROBE_OK;
    }, nullptr, nullptr);

    gst_pad_add_probe(src_pad, GST_PAD_PROBE_TYPE_BLOCK_DOWNSTREAM, [](GstPad*, GstPadProbeInfo*, gpointer) -> GstPadProbeReturn {
        return GST_PAD_PROBE_OK;
    }, nullptr, nullptr);

    const auto previous_gst_element = get_previous_enabled_element(element).gst_element;
    if (!previous_gst_element) {
        LOG_ERROR("Could not find previous element");
    }

    const auto next_gst_element = get_next_enabled_element(element).gst_element;
    if (!next_gst_element) {
        LOG_ERROR("Could not find next element");
    }

    gst_element_unlink(previous_gst_element, element.gst_element);
    gst_element_unlink(element.gst_element, next_gst_element);
    gst_bin_remove(GST_BIN(gst_pipeline_), element.gst_element);

    gst_element_link(previous_gst_element, next_gst_element);

    gst_object_unref(sink_pad);
    gst_object_unref(src_pad);

    element.enabled = false;
    element.gst_element = nullptr;
}

int Gstreamer::enable_optional_pipeline_elements() {
    for (auto& element: pipeline_elements_) {
        if (element.optional) {
            if (!element.enabled) {
                enable_optional_element(element);
            } else {
                LOG_WARN("Element {} is already enabled", element.name);
            }
        }
    }

    return EXIT_SUCCESS;
}

int Gstreamer::disable_optional_pipeline_elements() {
    for (auto& element: pipeline_elements_) {
        if (element.optional) {
            if (element.enabled) {
                disable_optional_element(element);
            } else {
                LOG_WARN("Element {} is already disabled", element.name);
            }
        }
    }

    return EXIT_SUCCESS;
}

std::vector<PipelineElement> Gstreamer::create_elements_list(const std::string& file_path) {
    const auto pipeline_handler = std::make_unique<PipelineHandler>(file_path);
    LOG_DEBUG("Use pipeline from: {}", file_path);

    return pipeline_handler->get_all_elements();
}