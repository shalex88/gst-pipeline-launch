#include <utility>
#include "File/File.h"
#include "Gstreamer.h"

Gstreamer::Gstreamer(std::string pipeline_file) : pipeline_file_(std::move(pipeline_file)){
    LOG_TRACE("Gstreamer constructor");
    LOG_DEBUG("Init gstreamer");
    gst_init(nullptr, nullptr);

    // Create pipeline elements
    pipeline_ = gst_pipeline_new("runtime-control-pipeline");
    pipeline_elements_ = get_pipeline_elements(pipeline_file_);

    gst_elements.reserve(pipeline_elements_.size());

    for (const auto& element : pipeline_elements_) {
        LOG_INFO("Enable element: {}", element);
        gst_elements.emplace_back(gst_element_factory_make(element.c_str(), element.c_str()));
    }

    if (!pipeline_) {
        LOG_ERROR("Failed to create pipeline");
    }

    for (const auto& element : gst_elements) {
        if (!element) {
            LOG_ERROR("Failed to create pipeline element {}", gst_element_get_name(element));
        }
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

int Gstreamer::enable_element(const std::string& element_name) {
    LOG_INFO("Enable element: {}", element_name);

    if (!optional_element_) {
        // Create the optional_element_ element
        optional_element_ = gst_element_factory_make(element_name.c_str(), element_name.c_str());

        if (!optional_element_) {
            LOG_ERROR("Failed to create pipeline element {}", element_name);
            return EXIT_FAILURE;
        }

        // TODO: these properties depend on the element
        g_object_set(optional_element_, "method", 1, NULL); // videoflip. Flip horizontally
        // g_object_set(optional_element_, "hue", 0.5, NULL); // Adjust hue

        // Add the optional_element_ element to the pipeline
        gst_bin_add(GST_BIN(pipeline_), optional_element_);

        // Unlink timeoverlay from sink, insert optional_element_
        gst_element_unlink(gst_elements[gst_elements.size() - 2], gst_elements.back());
        gst_element_link_many(gst_elements[gst_elements.size() - 2], optional_element_ , gst_elements.back(), NULL);

        gst_element_sync_state_with_parent(optional_element_);
    } else {
        LOG_WARN("Element {} is already enabled", element_name);
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}

int Gstreamer::disable_element(const std::string& element_name) {
    LOG_INFO("Disable element: {}", element_name);

    if (optional_element_) {
        gst_element_set_state(optional_element_, GST_STATE_NULL);

        GstPad *src_pad = gst_element_get_static_pad(optional_element_, "src");
        GstPad *sink_pad = gst_element_get_static_pad(optional_element_, "sink");

        // Block pads to unlink the optional_element_ safely
        gst_pad_add_probe(sink_pad, GST_PAD_PROBE_TYPE_BLOCK_DOWNSTREAM, nullptr, nullptr, nullptr);
        gst_pad_add_probe(src_pad, GST_PAD_PROBE_TYPE_BLOCK_DOWNSTREAM, nullptr, nullptr, nullptr);

        // Unlink and remove the optional_element_ element
        gst_element_unlink(gst_elements[gst_elements.size() - 2], optional_element_);
        gst_element_unlink(optional_element_, gst_elements.back());
        gst_bin_remove(GST_BIN(pipeline_), optional_element_);
        optional_element_ = nullptr;

        // Re-link timeoverlay directly to sink
        gst_element_link(gst_elements[gst_elements.size() - 2], gst_elements.back());

        // Release the pads
        gst_object_unref(sink_pad);
        gst_object_unref(src_pad);
    } else {
        LOG_WARN("Element {} is not enabled", element_name);
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}

std::vector<std::string> Gstreamer::get_pipeline_elements(const std::string& file_path) { // TODO: make agnostic to how elements are stored
    const auto file = std::make_unique<File>(file_path);
    LOG_DEBUG("Use pipeline from: {}", file_path);

    return file->get_vector_of_lines();
}