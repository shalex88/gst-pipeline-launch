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

std::error_code PipelineManager::linkElements(PipelineElement& source, PipelineElement& destination) {
    auto src_pad = allocatePad(source, GST_PAD_SRC);
    if(!src_pad){
        return {errno, std::generic_category()};
    }
    
    auto sink_pad = allocatePad(destination, GST_PAD_SINK);
    if(!sink_pad){
        return {errno, std::generic_category()};
    }
    
    if (gst_pad_link(src_pad, sink_pad) != GST_PAD_LINK_OK) {
        LOG_ERROR("Failed to link tee to element");
        gst_object_unref(src_pad);
        gst_object_unref(sink_pad);
        return {errno, std::generic_category()};
    }

    // Unref pads to release references
    gst_object_unref(src_pad);
    gst_object_unref(sink_pad);

    return {};
}

PipelineManager::~PipelineManager() {
    LOG_TRACE("Pipeline destructor");
}

std::error_code PipelineManager::enableAllOptionalPipelineBranches() {
    return {};
}

std::error_code PipelineManager::disableAllOptionalPipelineBranches() {
    return {};
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

std::error_code PipelineManager::linkGstElement(PipelineElement& current_element) {
    auto next_element = getNextEnabledElement(current_element);
    auto prev_element = getPreviousEnabledElement(current_element);

    if (next_element && current_element.branch == next_element->branch && !next_element->is_linked) {
        LOG_TRACE("Link first or middle element");
        if (auto ec = linkElements(current_element, *next_element)) {
            LOG_ERROR("Failed to link {} to {}", current_element.toString(), next_element->toString());
            return {errno, std::generic_category()};
        } else {
            LOG_DEBUG("Linked {} to {}", current_element.toString(), next_element->toString());
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
            return {errno, std::generic_category()};
        }
        if (!gst_element_sync_state_with_parent(current_element.gst_element)) {
            LOG_ERROR("Failed to sync {} state with parent", current_element.toString());
        }
    }

    if (prev_element && prev_element->branch != current_element.branch) {
        LOG_TRACE("Link first in branch");
        auto tee = findTeeElementForBranch(current_element.branch);
        if (auto ec = linkElements(tee, current_element)) {
            LOG_ERROR("Failed to link {} to {}", tee.toString(), current_element.toString());
            return {errno, std::generic_category()};
        } else {
            LOG_DEBUG("Linked {} to {}", tee.toString(), current_element.toString());
        }
    }

    current_element.is_linked = true;

    return {};
}

std::string PipelineManager::generateGstElementUniqueName(const PipelineElement& element) const {
    LOG_DEBUG("Generating unique name for element: {}", element.toString());
    std::string unique_name;

    auto unique_name_it = element.properties.find("name");
    if (unique_name_it != element.properties.end()) {
        unique_name = unique_name_it->second;
    }
    else {
        unique_name = element.name + std::to_string(element.id);
    }
    return {unique_name};
}

void PipelineManager::validateGstElementProperties(PipelineElement& element) const {
        for (auto property_it = element.properties.begin(); property_it != element.properties.end();) {
            const auto& [key, value] = *property_it;
            if(!g_object_class_find_property(G_OBJECT_GET_CLASS(element.gst_element), key.c_str())) {
                element.properties.erase(property_it++);
                LOG_WARN("Property {} not found for gst element {}. Earsing property from element", key, element.name.c_str());
            }
            else {
                ++property_it;
            }
        }
}

void PipelineManager::setGstElementProperty(PipelineElement& element) const {
    for (const auto& [key, value] : element.properties) {
        gst_util_set_object_arg(G_OBJECT(element.gst_element), key.c_str(), value.c_str());
        LOG_TRACE("Set property {} with value {} for element {}", key, value, element.name.c_str());
    }
}

std::error_code PipelineManager::retrieveMuxGstElement(PipelineElement& element, const std::string unique_element_name) const {
    element.gst_element = gst_bin_get_by_name(GST_BIN(gst_pipeline_.get()), unique_element_name.c_str());
    if (!element.gst_element) {
        LOG_ERROR("Failed to retrieve mux element {}", element.toString());
        return {errno, std::generic_category()};
    } 
        LOG_DEBUG("Mux element retrieved from pipeline: {}", element.toString());
        return {};
}

bool PipelineManager::isGstElementInPipeline(const std::string& element_name) const {
    auto element = gst_bin_get_by_name(GST_BIN(gst_pipeline_.get()), element_name.c_str());
    return {element != nullptr};
}

std::error_code PipelineManager::createGstElement(PipelineElement& element) const {
    auto unique_gst_element_name = generateGstElementUniqueName(element);
    
    if(!isGstElementInPipeline(unique_gst_element_name)) {
        element.gst_element = gst_element_factory_make(element.name.c_str(), unique_gst_element_name.c_str());
        if (!element.gst_element) {
            LOG_ERROR("Failed to create pipeline element {}", element.toString());
            return {errno, std::generic_category()};
        }
        if (!GST_IS_ELEMENT(element.gst_element)) {
            LOG_ERROR("Non valid pipeline element {}", element.toString());
            return {errno, std::generic_category()};
        }

        validateGstElementProperties(element);
        setGstElementProperty(element);

        if (!gst_bin_add(GST_BIN(gst_pipeline_.get()), element.gst_element)) {
            LOG_ERROR("Failed to add element {} to pipeline", element.toString());
            return {errno, std::generic_category()};
        }

        gst_element_sync_state_with_parent(element.gst_element);
        
        LOG_DEBUG("Created element: {} with name: {}", element.toString(), unique_gst_element_name);
    }
    else {
        if (element.type == "mux") {
            retrieveMuxGstElement(element, unique_gst_element_name);
        }
        else {
            LOG_ERROR("Element {} already exists in pipeline", element.toString());
            return {errno, std::generic_category()};
        }
    }

    element.is_initialized = true;

    return {};
}

std::error_code PipelineManager::createGstPipeline(std::vector<PipelineElement>& pipeline) {
    for (auto& element: pipeline) {
        if (!element.is_optional) {
            if (auto ec = createGstElement(element)) {
                return {errno, std::generic_category()};
            }
        }
    }

    for (auto& element: pipeline) {
        if (element.is_initialized) {
            if (auto ec = linkGstElement(element)) {
                return {errno, std::generic_category()};
            }
        }
    }

    return {};
}

std::error_code PipelineManager::play() {
    if (auto ec = createGstPipeline(pipeline_elements_)) {
        throw std::runtime_error("Failed to create pipeline");
    }

    LOG_DEBUG("Start playing");
    gst_element_set_state(gst_pipeline_.get(), GST_STATE_PLAYING);

    const auto bus = std::shared_ptr<GstBus>(gst_element_get_bus(gst_pipeline_.get()), gst_object_unref);
    gst_bus_add_watch(bus.get(), busCallback, this);

    gst_loop_ = std::shared_ptr<GMainLoop>(g_main_loop_new(nullptr, FALSE), g_main_loop_unref);
    if (!gst_loop_) {
        LOG_ERROR("Failed to create gstreamer main loop");
        return {errno, std::generic_category()};
    }
    
    // GST_DEBUG_BIN_TO_DOT_FILE(GST_BIN(gst_pipeline_.get()), GST_DEBUG_GRAPH_SHOW_ALL, "custom_pipeline");
    g_main_loop_run(gst_loop_.get());

    return {};
}

std::error_code PipelineManager::stop() const {
    if (gst_pipeline_ && gst_loop_) {
        g_main_loop_quit(gst_loop_.get());
        gst_element_set_state(gst_pipeline_.get(), GST_STATE_NULL);
        LOG_DEBUG("Stop playing");
        return {};
    }

    LOG_WARN("No stream playing");
    return {errno, std::generic_category()};
}

gboolean PipelineManager::busCallback(GstBus*, GstMessage* message, gpointer data) {
    const auto pipeline_manager = static_cast<PipelineManager*>(data);
    switch (GST_MESSAGE_TYPE(message)) {
        case GST_MESSAGE_EOS:
            LOG_DEBUG("End of stream");
            return pipeline_manager->stop() ? FALSE : TRUE;
        case GST_MESSAGE_ERROR:
            GError* err;
            gchar* debug;
            gst_message_parse_error(message, &err, &debug);
            LOG_ERROR("{}", err->message);
            g_error_free(err);
            g_free(debug);
            return pipeline_manager->stop() ? FALSE : TRUE;
        default:
            break;
    }
    return TRUE;
}

PipelineElement* PipelineManager::getPreviousEnabledElement(const PipelineElement& element) {
    for (auto i = element.id - 1; i < pipeline_elements_.size(); --i) {
        if (pipeline_elements_.at(i).is_initialized) {
            return &pipeline_elements_.at(i);
        }
    }
    return {};
}

PipelineElement* PipelineManager::getNextEnabledElement(const PipelineElement& element) {
    for (auto i = element.id + 1; i != pipeline_elements_.size(); ++i) {
        if (pipeline_elements_.at(i).is_initialized) {
            return &pipeline_elements_.at(i);
        }
    }
    return {};
}

std::error_code PipelineManager::enableOptionalElement(PipelineElement& element) {
    std::lock_guard lock_guard(mutex_);

    if (auto ec = createGstElement(element)) {
        return {errno, std::generic_category()};
    }

    if (auto ec = linkGstElement(element)) {
        return {errno, std::generic_category()};
    }

    return {};
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

    LOG_DEBUG("Element {} disconnected", gst_element_get_name(gst_element.get()));
    return GST_PAD_PROBE_REMOVE;
}

GstPadProbeReturn PipelineManager::disconnectBranchProbeCallback(GstPad* src_peer, GstPadProbeInfo* info, gpointer data) {
    const auto pipeline_manager = static_cast<PipelineManager*>(data);

    // Get the peer pad to determine the first element of the branch
    GstPad* peer_pad = gst_pad_get_peer(src_peer);
    if (!peer_pad) {
        LOG_ERROR("Failed to get peer pad for pad: {}", gst_pad_get_name(src_peer));
        return GST_PAD_PROBE_REMOVE;
    }

    GstElement* first_element = gst_pad_get_parent_element(peer_pad);
    gst_object_unref(peer_pad);

    if (!first_element) {
        LOG_ERROR("Failed to get first element in the branch for pad: {}", gst_pad_get_name(src_peer));
        return GST_PAD_PROBE_REMOVE;
    }

    LOG_DEBUG("Unlinking pad: {} from first element: {}", gst_pad_get_name(src_peer), gst_element_get_name(first_element));

    // Unlink the pad from the first element
    if (!gst_pad_unlink(src_peer, peer_pad)) {
        spdlog::error("Failed to unlink pad: {}", gst_pad_get_name(src_peer));
        gst_object_unref(first_element);
        return GST_PAD_PROBE_REMOVE;
    }

    // Iterate over all pipeline elements and remove those in the branch
    for (const auto& element : pipeline_manager->pipeline_elements_) {
        if (!element.gst_element) {
            continue;
        }

        // Find the source pad connecting to the first element
        GstPad* src_pad = findLinkedSrcPad(first_element, element.gst_element);
        if (src_pad) {
            spdlog::debug("Removing element: {}", element.name);

            // Set the element to NULL state and remove it from the pipeline
            gst_element_set_state(element.gst_element, GST_STATE_NULL);
            GstObject* parent = gst_element_get_parent(element.gst_element);
            if (parent) {
                gst_bin_remove(GST_BIN(parent), element.gst_element);
                gst_object_unref(parent);
            }
            gst_object_unref(src_pad);
        }
    }

    gst_object_unref(first_element);
    return GST_PAD_PROBE_REMOVE;
}

std::vector<GstPad*> PipelineManager::getLinkedSinkPads(GstElement* element) const {
    std::vector<GstPad*> sink_pads;
    GstIterator* it = gst_element_iterate_sink_pads(element);

    if (it) {
        GValue item = G_VALUE_INIT;
        while (gst_iterator_next(it, &item) == GST_ITERATOR_OK) {
            GstPad* pad = static_cast<GstPad*>(g_value_get_object(&item));
            if (pad && gst_pad_is_linked(pad)) {
                sink_pads.push_back(pad);
            }
            g_value_unset(&item);
        }

        gst_iterator_free(it);
    }
    return sink_pads;
}

std::error_code PipelineManager::disableOptionalElement(PipelineElement& element) const {
    std::lock_guard lock_guard(mutex_);
    LOG_DEBUG("Disabling element: {}", element.toString());

    auto sink_pads = getLinkedSinkPads(element.gst_element);
    
    if (sink_pads.size() == 0) {
        LOG_ERROR("Failed to get sink pad for element {}", element.toString());
        return {errno, std::generic_category()};
    }
    else if (sink_pads.size() > 1) {
        LOG_ERROR("Element {} has multiple sink pads ({}). This might have unexpected behaivior.", element.toString(), sink_pads.size());
        return std::make_error_code(std::errc::invalid_argument);
    }

    GstPad* sink_pad = sink_pads.front();
    GstPad* peer_pad = gst_pad_get_peer(sink_pad);

    if (!peer_pad) {
        LOG_ERROR("Failed to get src peer for element {}", element.toString());
        return {errno, std::generic_category()};
    }

    // Add probe to disconnect element safely when idle
    gst_pad_add_probe(peer_pad, GST_PAD_PROBE_TYPE_IDLE, disconnectGstElementProbeCallback, 
                        const_cast<PipelineManager*>(this), nullptr);

    gst_object_unref(peer_pad);

    element.is_initialized = false;
    element.is_linked = false;

    return {};
}

std::error_code PipelineManager::enableAllOptionalPipelineElements() {
    for (auto& element: pipeline_elements_) {
        if (element.is_optional && !element.is_initialized && !element.is_linked) {
            if (auto ec = createGstElement(element)) {
                return {errno, std::generic_category()};
            }
        }
    }

    for (auto& element: pipeline_elements_) {
        if (element.is_optional && element.is_initialized && !element.is_linked) {
            if (auto ec = linkGstElement(element)) {
                return {errno, std::generic_category()};
            }
        }
    }

    return {};
}

std::error_code PipelineManager::disableAllOptionalPipelineElements() {
    for (auto& element: pipeline_elements_) {
        if (element.is_optional && element.is_linked) {
            disableOptionalElement(element);
        } else if (element.is_optional) {
            LOG_WARN("Element {} is already disabled", element.toString());
        }
    }
    return {};
}

std::error_code PipelineManager::enableOptionalPipelineElement(const std::string& element_name) {
    for (auto& element: pipeline_elements_) {
        if (element.name == element_name) {
            if (element.is_initialized && element.is_linked) {
                LOG_WARN("Element {} is already enabled", element.toString());
                return {errno, std::generic_category()};
            }
            return enableOptionalElement(element);
        }
    }
    return {errno, std::generic_category()};
}

std::error_code PipelineManager::disableOptionalPipelineElement(const std::string& element_name) {
    for (auto& element: pipeline_elements_) {
        if (element.name == element_name) {
            if (!element.is_initialized && !element.is_linked) {
                LOG_WARN("Element {} is already disabled", element.toString());
                return {errno, std::generic_category()};
            }
            return disableOptionalElement(element);
        }
    }
    return {errno, std::generic_category()};
}

std::error_code PipelineManager::enableOptionalPipelineBranch(const std::string& branch_name) {
    for (auto& element: pipeline_elements_) {
        if (element.is_optional && !element.is_initialized && !element.is_linked && element.branch == branch_name) {
            if (auto ec = createGstElement(element)) {
                return {errno, std::generic_category()};
            }
        }
    }

    for (auto& element: pipeline_elements_) {
        if (element.is_optional && element.is_initialized && !element.is_linked) {
            if (auto ec = linkGstElement(element)) {
                return {errno, std::generic_category()};
            }
        }
    }

    return {};
}

std::error_code PipelineManager::disableOptionalPipelineBranch(const std::string& branch_name) {
    for (auto& element: pipeline_elements_) {
        if (element.is_optional && element.is_linked && element.branch == branch_name) {
            disableOptionalElement(element);
        } else if (element.is_optional && element.branch == branch_name) {
            LOG_WARN("Element {} is already disabled", element.toString()); //FIXME: handle to branch, not element
        }
    }
    return {};
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
        if (element.name == "tee" && element.is_initialized && element.type == branch_name) {
            return element;
        }
    }

    throw std::runtime_error("Tee element not found");
}

PipelineElement& PipelineManager::findFirstElementInBranch(const std::string& branch_name) {
    for (auto& element: pipeline_elements_) {
        if (element.branch == branch_name) {
            return element;
        }
    }

    throw std::runtime_error("Branch not found");
}

GstPad* PipelineManager::findLinkedSrcPad(GstElement* upstream_element, GstElement* downstream_element) {
    GstPad* source_pad = nullptr;

    GstIterator* it = gst_element_iterate_sink_pads(downstream_element);
    GValue item = G_VALUE_INIT;

    while (gst_iterator_next(it, &item) == GST_ITERATOR_OK) {
        GstPad* sink_pad = static_cast<GstPad*>(g_value_get_object(&item));
        
        // Get the peer pad of the sink pad
        GstPad* peer_pad = gst_pad_get_peer(sink_pad);
        if (peer_pad) {
            GstElement* parent_element = gst_pad_get_parent_element(peer_pad);
            
            // Check if the peer pad belongs to the upstream element
            if (parent_element == upstream_element) {
                source_pad = peer_pad;  // Found the source pad
                gst_object_unref(parent_element);
                break;
            }
            
            gst_object_unref(parent_element);
            gst_object_unref(peer_pad);
        }
        g_value_unset(&item);
    }
    
    gst_iterator_free(it);
    return source_pad;
}


GstPad* PipelineManager::requestExplicitPadName(PipelineElement& element, GstPadDirection direction) {
    GstPad *pad = nullptr;
    std::string padName = (direction == GST_PAD_SINK) ? element.sink_pad_name : ""; /*element.src_pad_name;*/

    if (!padName.empty()) {
        LOG_DEBUG("Requesting explicit pad name: {} for element {}",padName, element.toString());
        pad = gst_element_request_pad_simple(element.gst_element, padName.c_str());
    }
    return pad;
}

GstPadTemplate* PipelineManager::findSuitablePadTemplate(PipelineElement& element, GstPadDirection direction) {
    auto *pad_templates = gst_element_get_pad_template_list(element.gst_element);
    for (const auto *l = pad_templates; l; l = l->next) {
        auto *pad_template = static_cast<GstPadTemplate*>(l->data);
        if (GST_PAD_TEMPLATE_DIRECTION(pad_template) == direction) {
            return pad_template;
        }
    }
    return nullptr;
}

std::string PipelineManager::generateDynamicPadName(const GstPadTemplate* pad_template) {
    LOG_DEBUG("Generating dynamic pad name from template: {}", GST_PAD_TEMPLATE_NAME_TEMPLATE(pad_template));
    std::string generatedPadName = GST_PAD_TEMPLATE_NAME_TEMPLATE(pad_template);
    if (generatedPadName.find('%') != std::string::npos) {
        static unsigned int pad_counter = 0;  // Unique counter for naming
        generatedPadName.replace(generatedPadName.find('%'), 2, std::to_string(pad_counter++));
    }
    return generatedPadName;
}

GstPad* PipelineManager::allocatePad(PipelineElement& element, GstPadDirection direction) {
    GstPad *pad = requestExplicitPadName(element, direction);
    
    if(!pad)
    {
        auto pad_template = findSuitablePadTemplate(element, direction);
        
        if (pad_template) {
            switch (GST_PAD_TEMPLATE_PRESENCE(pad_template))
            {
            case GST_PAD_ALWAYS:
                LOG_DEBUG("Found a static pad template {} for element {}", GST_PAD_TEMPLATE_NAME_TEMPLATE(pad_template), element.toString());
                pad = gst_element_get_static_pad(element.gst_element, GST_PAD_TEMPLATE_NAME_TEMPLATE(pad_template));
                break;
            case GST_PAD_REQUEST:
            {
                LOG_DEBUG("Found a dynamic pad template {} for element {}", GST_PAD_TEMPLATE_NAME_TEMPLATE(pad_template), element.toString());
                auto generatedPadName = generateDynamicPadName(pad_template);
                LOG_DEBUG("Requesting pad: {} from template: {}", generatedPadName, GST_PAD_TEMPLATE_NAME_TEMPLATE(pad_template));
                pad = gst_element_request_pad_simple(element.gst_element, generatedPadName.c_str());
            }
                break;
            default:
                LOG_WARN("Pad template {} presence is not supported", GST_PAD_TEMPLATE_NAME_TEMPLATE(pad_template));
                break;
            }
        }
        else {
            LOG_ERROR("No compatible pad template found for direction: {}", (direction == GST_PAD_SINK ? "SINK" : "SRC"));
        }
    }

    return pad;
}