#include <utility>
#include <sstream>
#include "Pipeline/PipelineHandler.h"
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
    createGstPipeline(pipeline_elements_);
}

PipelineManager::~PipelineManager() {
    LOG_TRACE("Pipeline destructor");
}

int PipelineManager::linkAllGstElements() const {
    for (const auto& element: pipeline_elements_) {
        if (element->enabled) {
            auto next_element = getNextEnabledElement(*element);
            if (next_element.lock() && next_element.lock()->gst_element.get()) {
                if (!gst_element_link(element->gst_element.get(), next_element.lock()->gst_element.get())) {
                    LOG_ERROR("Failed to link element {} to {}", element->name, next_element.lock()->name);
                    return EXIT_FAILURE;
                }
            }
        }
    }

    return EXIT_SUCCESS;
}

int PipelineManager::createGstElement(PipelineElement& element) const {
    element.gst_element = std::shared_ptr<GstElement>(
    gst_element_factory_make(element.name.c_str(), element.name.c_str()),
        [](GstElement*) {
        });
    if (!element.gst_element) {
        LOG_ERROR("Failed to create pipeline element {}", element.name);
        return EXIT_FAILURE;
    }
    if (!GST_IS_ELEMENT(element.gst_element.get())) {
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

int PipelineManager::createGstPipeline(std::vector<std::shared_ptr<PipelineElement>>& pipeline) const {
    for (auto& element: pipeline) {
        if (!element->optional) {
            if (createGstElement(*element) != EXIT_SUCCESS) {
                return EXIT_FAILURE;
            }
        }
    }

    if (linkAllGstElements() != EXIT_SUCCESS) {
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}

void PipelineManager::printElement(const PipelineElement& element) {
    std::ostringstream oss;
    oss << element;
    LOG_INFO("Enable element: {}", oss.str());
}

int PipelineManager::play() {
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

std::weak_ptr<PipelineElement> PipelineManager::getPreviousEnabledElement(const PipelineElement& element) const {
    for (auto i = element.id - 1; i < pipeline_elements_.size(); --i) {
        if (pipeline_elements_.at(i) && pipeline_elements_.at(i)->enabled) {
            return pipeline_elements_.at(i);
        }
    }
    return {};
}

std::weak_ptr<PipelineElement> PipelineManager::getNextEnabledElement(const PipelineElement& element) const {
    for (auto i = element.id + 1; i != pipeline_elements_.size(); ++i) {
        if (pipeline_elements_.at(i) && pipeline_elements_.at(i)->enabled) {
            return pipeline_elements_.at(i);
        }
    }
    return {};
}

int PipelineManager::enableElement(PipelineElement& element) const {
    std::lock_guard lock_guard(mutex_);

    LOG_INFO("Enable element: {}", element.name);

    element.gst_element = std::shared_ptr<GstElement>(
        gst_element_factory_make(element.name.c_str(), element.name.c_str()),
        [](GstElement*) {
        });
    if (!element.gst_element) {
        LOG_ERROR("Failed to create pipeline element {}", element.name);
        return EXIT_FAILURE;
    }
    if (!GST_IS_ELEMENT(element.gst_element.get())) {
        LOG_ERROR("Non valid pipeline element {}", element.name);
        return EXIT_FAILURE;
    }

    const auto previous_enabled_element = getPreviousEnabledElement(element);
    if (!previous_enabled_element.lock() || !GST_IS_ELEMENT(previous_enabled_element.lock()->gst_element.get())) {
        LOG_ERROR("Could not find previous element");
        return EXIT_FAILURE;
    }

    const auto next_enabled_element = getNextEnabledElement(element);
    if (!next_enabled_element.lock() || !GST_IS_ELEMENT(next_enabled_element.lock()->gst_element.get())) {
        LOG_ERROR("Could not find next element");
        return EXIT_FAILURE;
    }

    gst_bin_add(GST_BIN(gst_pipeline_.get()), element.gst_element.get());

    gst_element_unlink(previous_enabled_element.lock()->gst_element.get(), next_enabled_element.lock()->gst_element.get());
    gst_element_link_many(previous_enabled_element.lock()->gst_element.get(), element.gst_element.get(),
                          next_enabled_element.lock()->gst_element.get(), NULL);

    gst_element_sync_state_with_parent(element.gst_element.get());
    element.enabled = true;

    return EXIT_SUCCESS;
}

int PipelineManager::disableElement(PipelineElement& element) const {
    std::lock_guard lock_guard(mutex_);

    LOG_INFO("Disable element: {}", element.name);

    gst_element_set_state(element.gst_element.get(), GST_STATE_NULL);

    const auto src_pad = std::shared_ptr<GstPad>(gst_element_get_static_pad(element.gst_element.get(), "src"),
                                           gst_object_unref);
    if (!GST_IS_PAD(src_pad.get())) {
        LOG_ERROR("Failed to get src pad for element {}", element.name);
        return EXIT_FAILURE;
    }

    const auto sink_pad = std::shared_ptr<GstPad>(gst_element_get_static_pad(element.gst_element.get(), "video_sink"),
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
    if (!previous_enabled_element.lock() || !GST_IS_ELEMENT(previous_enabled_element.lock()->gst_element.get())) {
        LOG_ERROR("Could not find previous element");
        return EXIT_FAILURE;
    }

    const auto next_enabled_element = getNextEnabledElement(element);
    if (!next_enabled_element.lock() || !GST_IS_ELEMENT(next_enabled_element.lock()->gst_element.get())) {
        LOG_ERROR("Could not find next element");
        return EXIT_FAILURE;
    }

    gst_element_unlink(previous_enabled_element.lock()->gst_element.get(), element.gst_element.get());
    gst_element_unlink(element.gst_element.get(), next_enabled_element.lock()->gst_element.get());

    if (!gst_bin_remove(GST_BIN(gst_pipeline_.get()), element.gst_element.get())) {
        LOG_ERROR("Failed to remove element {} from pipeline", element.name);
        return EXIT_FAILURE;
    }

    if (!gst_element_link(previous_enabled_element.lock()->gst_element.get(), next_enabled_element.lock()->gst_element.get())) {
        LOG_ERROR("Failed to link previous element to next element");
        return EXIT_FAILURE;
    }

    element.enabled = false;

    return EXIT_SUCCESS;
}

int PipelineManager::enableAllOptionalPipelineElements() {
    for (auto& element: pipeline_elements_) {
        if (element->optional && !element->enabled) {
            enableElement(*element);
        } else if (element->optional) {
            LOG_WARN("Element {} is already enabled", element->name);
        }
    }
    return EXIT_SUCCESS;
}

int PipelineManager::disableAllOptionalPipelineElements() {
    for (auto& element: pipeline_elements_) {
        if (element->optional && element->enabled) {
            disableElement(*element);
        } else if (element->optional) {
            LOG_WARN("Element {} is already disabled", element->name);
        }
    }
    return EXIT_SUCCESS;
}

int PipelineManager::enableOptionalPipelineElement(const std::string& element_name) {
    for (const auto& element: pipeline_elements_) {
        if (element->name == element_name) {
            if (element->enabled) {
                LOG_WARN("Element {} is already enabled", element->name);
                return EXIT_FAILURE;
            }
            return enableElement(*element);
        }
    }
    return EXIT_FAILURE;
}

int PipelineManager::disableOptionalPipelineElement(const std::string& element_name) {
    for (const auto& element: pipeline_elements_) {
        if (element->name == element_name) {
            if (!element->enabled) {
                LOG_WARN("Element {} is already disabled", element->name);
                return EXIT_FAILURE;
            }
            return disableElement(*element);
        }
    }
    return EXIT_FAILURE;
}

std::vector<std::string> PipelineManager::getOptionalPipelineElementsNames() const {
    std::vector<std::string> elements_names;
    for (const auto& element: pipeline_elements_) {
        if (element->optional) {
            elements_names.push_back(element->name);
        }
    }
    return elements_names;
}

void PipelineManager::createElementsList(const std::string& file_path) {
    const auto pipeline_handler = std::make_unique<PipelineHandler>(file_path);
    LOG_DEBUG("Use pipeline from: {}", file_path);
    for (auto& element : pipeline_handler->getAllElements()) {
        pipeline_elements_.push_back(std::make_shared<PipelineElement>(std::move(element)));
    }
}