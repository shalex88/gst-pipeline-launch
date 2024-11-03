#include <utility>
#include "Gstreamer/PipelineHandler.h"
#include "Gstreamer.h"

Gstreamer::Gstreamer(std::string pipeline_file) : pipeline_file_(std::move(pipeline_file)){
    LOG_TRACE("Gstreamer constructor");
    LOG_DEBUG("Init gstreamer");
    gst_init(nullptr, nullptr);

    pipeline_ = gst_pipeline_new("runtime-control-pipeline");
    if (!pipeline_) {
        LOG_ERROR("Failed to create pipeline");
    }

    create_element_objects(pipeline_file_);

    default_pipeline_elements_ = get_default_pipeline_elements();
    optional_pipeline_elements_ = get_optional_pipeline_elements();

    gst_elements.reserve(default_pipeline_elements_.size());

    for (const auto& element : default_pipeline_elements_) {
        LOG_INFO("Enable element: {}", element.name);
        auto element_ptr = gst_element_factory_make(element.name.c_str(), element.name.c_str());
        if (!element_ptr) {
            LOG_ERROR("Failed to create pipeline element {}", element.name);
        }
        gst_elements.emplace_back(element_ptr);
    }

    for (const auto& element : gst_elements) {
        if (!gst_bin_add(GST_BIN(pipeline_), element)) {
            LOG_ERROR("Failed to add element {} to pipeline", gst_element_get_name(element));
        }
    }

    for (auto it = gst_elements.begin(); it != gst_elements.end() - 1; ++it) {
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

PipelineElement* Gstreamer::find_element(const std::string& element_name) {
    auto it = std::find_if(all_pipeline_elements_.begin(), all_pipeline_elements_.end(),
                           [&element_name](const PipelineElement& element) {
                               return element.name == element_name;
                           });
    if (it != all_pipeline_elements_.end()) {
        return &(*it);
    }

    return nullptr;
}

void Gstreamer::enable_element(const PipelineElement& element) {
    LOG_INFO("Enable element: {}", element.name);

    const auto current_element = gst_element_factory_make(element.name.c_str(), element.name.c_str());
    if (!current_element) {
        LOG_ERROR("Failed to create pipeline element {}", element.name);
    }

    gst_bin_add(GST_BIN(pipeline_), current_element);

    auto priveous_element = gst_bin_get_by_name(GST_BIN(pipeline_), all_pipeline_elements_[element.id - 1].name.c_str());
    auto next_element = gst_bin_get_by_name(GST_BIN(pipeline_), all_pipeline_elements_[element.id + 1].name.c_str());

    gst_element_unlink(priveous_element, next_element);
    gst_element_link_many(priveous_element, current_element , next_element, NULL);

    gst_element_sync_state_with_parent(current_element);
}

void Gstreamer::disable_element(const PipelineElement& element) {
    LOG_INFO("Disable element: {}", element.name);

    auto current_element = gst_bin_get_by_name(GST_BIN(pipeline_), all_pipeline_elements_[element.id].name.c_str());

    gst_element_set_state(current_element, GST_STATE_NULL);

    GstPad *src_pad = gst_element_get_static_pad(current_element, "src");
    GstPad *sink_pad = gst_element_get_static_pad(current_element, "sink");

    // Block pads to unlink the optional_element_ safely
    gst_pad_add_probe(sink_pad, GST_PAD_PROBE_TYPE_BLOCK_DOWNSTREAM, nullptr, nullptr, nullptr);
    gst_pad_add_probe(src_pad, GST_PAD_PROBE_TYPE_BLOCK_DOWNSTREAM, nullptr, nullptr, nullptr);

    auto priveous_element = gst_bin_get_by_name(GST_BIN(pipeline_), all_pipeline_elements_[element.id - 1].name.c_str());
    auto next_element = gst_bin_get_by_name(GST_BIN(pipeline_), all_pipeline_elements_[element.id + 1].name.c_str());

    gst_element_unlink(priveous_element, current_element);
    gst_element_unlink(current_element, next_element);
    gst_bin_remove(GST_BIN(pipeline_), current_element);

    gst_element_link(priveous_element, next_element);

    // Release the pads
    gst_object_unref(sink_pad);
    gst_object_unref(src_pad);
}

int Gstreamer::enable_optional_pipeline_elements() {
    for (const auto& element : optional_pipeline_elements_) {
        enable_element(*find_element(element.name));
    }

    return EXIT_SUCCESS;
}

int Gstreamer::disable_optional_pipeline_elements() {
    for (const auto& element : optional_pipeline_elements_) {
        disable_element(*find_element(element.name));
    }

    return EXIT_SUCCESS;
}

void Gstreamer::create_element_objects(const std::string& file_path) {
    const auto pipeline_handler = std::make_unique<PipelineHandler>(file_path);
    LOG_DEBUG("Use pipeline from: {}", file_path);

    all_pipeline_elements_ = pipeline_handler->get_all_elements();
}

std::vector<PipelineElement> Gstreamer::get_default_pipeline_elements() {
    for (const auto& element: all_pipeline_elements_) {
        if (element.optional == false) {
            default_pipeline_elements_.emplace_back(element);
        }
    }

    return default_pipeline_elements_;
}

std::vector<PipelineElement> Gstreamer::get_optional_pipeline_elements() {
    for (const auto& element: all_pipeline_elements_) {
        if (element.optional == true) {
            optional_pipeline_elements_.emplace_back(element);
        }
    }

    return optional_pipeline_elements_;
}
