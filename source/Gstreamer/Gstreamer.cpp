#include <utility>
#include "Gstreamer/PipelineHandler.h"
#include "Gstreamer.h"

Gstreamer::Gstreamer(std::string pipeline_file) : pipeline_file_(std::move(pipeline_file)) {
    LOG_TRACE("Gstreamer constructor");
    LOG_DEBUG("Init gstreamer");

    gst_init(nullptr, nullptr);

    pipeline_ = gst_pipeline_new("runtime-control-pipeline");
    if (!pipeline_) {
        LOG_ERROR("Failed to create pipeline");
    }

    user_pipeline_elements_ = create_element_objects(pipeline_file_);
    running_pipeline_elements_ = init_default_pipeline_elements();

    running_gst_elements.reserve(running_pipeline_elements_.size());

    for (const auto& element: running_pipeline_elements_) {
        if(element.enabled) {
            LOG_INFO("Enable element: {}", element.name);
            auto element_ptr = gst_element_factory_make(element.name.c_str(), element.name.c_str());
            if (!element_ptr) {
                LOG_ERROR("Failed to create pipeline element {}", element.name);
            }
            running_gst_elements.emplace_back(element_ptr);
        }
    }

    for (const auto& element: running_gst_elements) {
        if (!gst_bin_add(GST_BIN(pipeline_), element)) {
            LOG_ERROR("Failed to add element {} to pipeline", gst_element_get_name(element));
        }
    }

    for (auto it = running_gst_elements.begin(); it != running_gst_elements.end() - 1; ++it) {
        if (!gst_element_link(*it, *(it + 1))) {
            LOG_ERROR("Failed to link element {} to {}", gst_element_get_name(*it), gst_element_get_name(*(it + 1)));
        }
    }
}

Gstreamer::~Gstreamer() {
    LOG_TRACE("Gstreamer destructor");
}

void Gstreamer::play() {
    LOG_INFO("Start playing");
    gst_element_set_state(pipeline_, GST_STATE_PLAYING);

    // Start the main loop in a new thread
    loop_ = g_main_loop_new(nullptr, FALSE);

    g_main_loop_run(loop_);
}

void Gstreamer::stop() const {
    LOG_INFO("Stop playing");
    g_main_loop_quit(loop_);

    gst_element_set_state(pipeline_, GST_STATE_NULL);
    gst_object_unref(pipeline_);
    g_main_loop_unref(loop_);
}

GstElement* Gstreamer::get_previous_running_gst_element(const PipelineElement& element) {
    for (auto i = element.id - 1; i != running_pipeline_elements_.size(); --i) {
        if (running_pipeline_elements_.at(i).enabled) {
            return gst_bin_get_by_name(GST_BIN(pipeline_), running_pipeline_elements_.at(i).name.c_str());
        }
    }

    return nullptr;
}

GstElement* Gstreamer::get_next_gst_element(const PipelineElement& element) {
    for (auto i = element.id + 1; i != running_pipeline_elements_.size(); ++i) {
        if (running_pipeline_elements_.at(i).enabled) {
            return gst_bin_get_by_name(GST_BIN(pipeline_), running_pipeline_elements_.at(i).name.c_str());
        }
    }

        return nullptr;
}

void Gstreamer::enable_element(PipelineElement& element) {
    std::lock_guard<std::mutex> lock_guard(mutex_);

    LOG_INFO("Enable element: {}", element.name);

    auto current_gst_element = gst_element_factory_make(element.name.c_str(), element.name.c_str());
    if (!current_gst_element) {
        LOG_ERROR("Failed to create pipeline element {}", element.name);
    }

    auto priveous_gst_element = get_previous_running_gst_element(element);
    if (!priveous_gst_element) {
        LOG_ERROR("Could not find previous element");
    }

    auto next_gst_element = get_next_gst_element(element);
    if (!next_gst_element) {
        LOG_ERROR("Could not find next element");
    }

    gst_bin_add(GST_BIN(pipeline_), current_gst_element);

    gst_element_unlink(priveous_gst_element, next_gst_element);
    gst_element_link_many(priveous_gst_element, current_gst_element, next_gst_element, NULL);

    gst_element_sync_state_with_parent(current_gst_element);

    element.enabled = true;
}

void Gstreamer::disable_element(PipelineElement& element) {
    std::lock_guard<std::mutex> lock_guard(mutex_);

    LOG_INFO("Disable element: {}", element.name);

    auto current_element = gst_bin_get_by_name(GST_BIN(pipeline_), user_pipeline_elements_[element.id].name.c_str());

    gst_element_set_state(current_element, GST_STATE_NULL);

    GstPad* src_pad = gst_element_get_static_pad(current_element, "src");
    GstPad* sink_pad = gst_element_get_static_pad(current_element, "sink");

    // Block pads to unlink the optional_element_ safely
    gst_pad_add_probe(sink_pad, GST_PAD_PROBE_TYPE_BLOCK_DOWNSTREAM, nullptr, nullptr, nullptr);
    gst_pad_add_probe(src_pad, GST_PAD_PROBE_TYPE_BLOCK_DOWNSTREAM, nullptr, nullptr, nullptr);

    auto priveous_gst_element = get_previous_running_gst_element(element);
    if (!priveous_gst_element) {
        LOG_ERROR("Could not find previous element");
    }

    auto next_gst_element = get_next_gst_element(element);
    if (!next_gst_element) {
        LOG_ERROR("Could not find next element");
    }

    gst_element_unlink(priveous_gst_element, current_element);
    gst_element_unlink(current_element, next_gst_element);
    gst_bin_remove(GST_BIN(pipeline_), current_element);

    gst_element_link(priveous_gst_element, next_gst_element);

    // Release the pads
    gst_object_unref(sink_pad);
    gst_object_unref(src_pad);

    element.enabled = false;
}

int Gstreamer::enable_optional_pipeline_elements() {
    for (auto& element: running_pipeline_elements_) {
        if (element.optional) {
            if (!element.enabled) {
                enable_element(element);
            } else {
                LOG_WARN("Element {} is already enabled", element.name);
            }
        }
    }

    return EXIT_SUCCESS;
}

int Gstreamer::disable_optional_pipeline_elements() {
    for (auto& element: running_pipeline_elements_) {
        if (element.optional) {
            if (element.enabled) {
                disable_element(element);
            } else {
                LOG_WARN("Element {} is already disabled", element.name);
            }
        }
    }

    return EXIT_SUCCESS;
}

std::vector<PipelineElement> Gstreamer::create_element_objects(const std::string& file_path) {
    const auto pipeline_handler = std::make_unique<PipelineHandler>(file_path);
    LOG_DEBUG("Use pipeline from: {}", file_path);

    return pipeline_handler->get_all_elements();
}

std::vector<PipelineElement> Gstreamer::init_default_pipeline_elements() {
    for (const auto& element: user_pipeline_elements_) {
        if (!element.optional) {
            running_pipeline_elements_.emplace_back(PipelineElement{
                    .id = element.id,
                    .name = element.name,
                    .type = element.type,
                    .caps = element.caps,
                    .optional = element.optional,
                    .enabled = true
            });
        } else {
            running_pipeline_elements_.emplace_back(PipelineElement{
                    .id = element.id,
                    .name = element.name,
                    .type = element.type,
                    .caps = element.caps,
                    .optional = element.optional,
                    .enabled = false
            });
        }
    }

    return running_pipeline_elements_;
}
