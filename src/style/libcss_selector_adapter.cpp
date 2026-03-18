#include "libcss_selector_adapter.hpp"

#include <libcss/libcss.h>
#include <libcss/computed.h>
#include <cstring>



namespace hypreact::style {

namespace {

struct TraceCollector {
    std::vector<std::string>* trace = nullptr;
};

void pushTrace(void* pw, const std::string& event) {
    auto* collector = static_cast<TraceCollector*>(pw);
    if (collector != nullptr && collector->trace != nullptr) {
        collector->trace->push_back(event);
    }
}

std::string nodeSummary(const LibcssAdapterNode* node) {
    if (node == nullptr || node->context == nullptr) {
        return "<null>";
    }

    return std::string(domain::toString(node->context->type)) + "#" + node->context->id;
}

LibcssAdapterNode* adapterParent(LibcssAdapterNode* node, const css_qname* qname) {
    if (node == nullptr || node->context == nullptr || node->context->parent == nullptr) {
        return nullptr;
    }

    auto* parent = const_cast<LibcssAdapterNode*>(node - 1);
    if (qname == nullptr || qname->name == nullptr) {
        return parent;
    }

    if (domain::toString(parent->context->type) == lwc_string_data(qname->name)) {
        return parent;
    }

    return nullptr;
}

css_error nodeName(void* pw, void* node, css_qname* qname) {
    pushTrace(pw, "node_name " + nodeSummary(static_cast<LibcssAdapterNode*>(node)));
    (void)pw;
    const auto* adapter = static_cast<LibcssAdapterNode*>(node);
    qname->ns = nullptr;
    const auto name = internLibcssString(domain::toString(adapter->context->type));
    if (!name.has_value()) {
        return CSS_NOMEM;
    }
    qname->name = *name;
    return CSS_OK;
}

css_error nodeClasses(void* pw, void* node, lwc_string*** classes, uint32_t* n_classes) {
    pushTrace(pw, "node_classes " + nodeSummary(static_cast<LibcssAdapterNode*>(node)));
    (void)pw;
    const auto* adapter = static_cast<LibcssAdapterNode*>(node);
    *n_classes = static_cast<uint32_t>(adapter->context->classes.size());
    if (*n_classes == 0) {
        *classes = nullptr;
        return CSS_OK;
    }

    auto** values = static_cast<lwc_string**>(calloc(*n_classes, sizeof(lwc_string*)));
    for (uint32_t index = 0; index < *n_classes; ++index) {
        const auto value = internLibcssString(adapter->context->classes[index]);
        if (!value.has_value()) {
            return CSS_NOMEM;
        }
        values[index] = *value;
    }
    *classes = values;
    return CSS_OK;
}

css_error nodeId(void* pw, void* node, lwc_string** id) {
    pushTrace(pw, "node_id " + nodeSummary(static_cast<LibcssAdapterNode*>(node)));
    (void)pw;
    const auto* adapter = static_cast<LibcssAdapterNode*>(node);
    const auto value = internLibcssString(adapter->context->id);
    if (!value.has_value()) {
        return CSS_NOMEM;
    }
    *id = *value;
    return CSS_OK;
}

css_error parentNode(void* pw, void* node, void** parent) {
    pushTrace(pw, "parent_node " + nodeSummary(static_cast<LibcssAdapterNode*>(node)));
    (void)pw;
    *parent = adapterParent(static_cast<LibcssAdapterNode*>(node), nullptr);
    return CSS_OK;
}

css_error namedParentNode(void* pw, void* node, const css_qname* qname, void** parent) {
    pushTrace(pw, "named_parent_node " + nodeSummary(static_cast<LibcssAdapterNode*>(node)));
    (void)pw;
    *parent = adapterParent(static_cast<LibcssAdapterNode*>(node), qname);
    return CSS_OK;
}

css_error namedAncestorNode(void* pw, void* node, const css_qname* qname, void** ancestor) {
    pushTrace(pw, "named_ancestor_node " + nodeSummary(static_cast<LibcssAdapterNode*>(node)));
    (void)pw;
    auto* current = static_cast<LibcssAdapterNode*>(node);
    while (true) {
        current = adapterParent(current, nullptr);
        if (current == nullptr) {
            *ancestor = nullptr;
            return CSS_OK;
        }
        if (qname == nullptr || qname->name == nullptr || domain::toString(current->context->type) == lwc_string_data(qname->name)) {
            *ancestor = current;
            return CSS_OK;
        }
    }
}

css_error unsupportedNodeOp(void* pw, void* node, void** out) {
    (void)pw;
    (void)node;
    *out = nullptr;
    return CSS_OK;
}

css_error unsupportedNamedNodeOp(void* pw, void* node, const css_qname* qname, void** out) {
    (void)qname;
    return unsupportedNodeOp(pw, node, out);
}

css_error nodeHasName(void* pw, void* node, const css_qname* qname, bool* match) {
    pushTrace(pw, "node_has_name " + nodeSummary(static_cast<LibcssAdapterNode*>(node)));
    (void)pw;
    const auto* adapter = static_cast<LibcssAdapterNode*>(node);
    *match = qname->name != nullptr && std::strcmp(lwc_string_data(qname->name), domain::toString(adapter->context->type).c_str()) == 0;
    return CSS_OK;
}

css_error nodeHasClass(void* pw, void* node, lwc_string* name, bool* match) {
    pushTrace(pw, "node_has_class " + nodeSummary(static_cast<LibcssAdapterNode*>(node)));
    (void)pw;
    const auto* adapter = static_cast<LibcssAdapterNode*>(node);
    *match = false;
    for (const auto& className : adapter->context->classes) {
        if (className == lwc_string_data(name)) {
            *match = true;
            break;
        }
    }
    return CSS_OK;
}

css_error nodeHasId(void* pw, void* node, lwc_string* name, bool* match) {
    pushTrace(pw, "node_has_id " + nodeSummary(static_cast<LibcssAdapterNode*>(node)));
    (void)pw;
    const auto* adapter = static_cast<LibcssAdapterNode*>(node);
    *match = adapter->context->id == lwc_string_data(name);
    return CSS_OK;
}

css_error nodeHasAttribute(void* pw, void* node, const css_qname* qname, bool* match) {
    pushTrace(pw, "node_has_attribute " + nodeSummary(static_cast<LibcssAdapterNode*>(node)));
    (void)pw;
    const auto* adapter = static_cast<LibcssAdapterNode*>(node);
    *match = adapter->context->window != nullptr && (
        std::strcmp(lwc_string_data(qname->name), "class") == 0
        || std::strcmp(lwc_string_data(qname->name), "title") == 0
        || std::strcmp(lwc_string_data(qname->name), "app-id") == 0
        || std::strcmp(lwc_string_data(qname->name), "floating") == 0
        || std::strcmp(lwc_string_data(qname->name), "fullscreen") == 0
    );
    return CSS_OK;
}

css_error nodeHasAttributeEqual(void* pw, void* node, const css_qname* qname, lwc_string* value, bool* match) {
    pushTrace(pw, "node_has_attribute_equal " + nodeSummary(static_cast<LibcssAdapterNode*>(node)));
    (void)pw;
    const auto* adapter = static_cast<LibcssAdapterNode*>(node);
    *match = false;
    if (adapter->context->window == nullptr) {
        return CSS_OK;
    }

    const std::string_view key = lwc_string_data(qname->name);
    const std::string_view expected = lwc_string_data(value);
    if (key == "class") *match = adapter->context->window->klass == expected;
    else if (key == "title") *match = adapter->context->window->title == expected;
    else if (key == "app-id") *match = adapter->context->window->initialClass == expected;
    else if (key == "floating") *match = (adapter->context->window->floating ? "true" : "false") == expected;
    else if (key == "fullscreen") *match = (adapter->context->window->fullscreen ? "true" : "false") == expected;
    return CSS_OK;
}

css_error nodeIsRoot(void* pw, void* node, bool* match) {
    pushTrace(pw, "node_is_root " + nodeSummary(static_cast<LibcssAdapterNode*>(node)));
    (void)pw;
    const auto* adapter = static_cast<LibcssAdapterNode*>(node);
    *match = adapter->context->parent == nullptr;
    return CSS_OK;
}

css_error nodeIsFocus(void* pw, void* node, bool* match) {
    pushTrace(pw, "node_is_focus " + nodeSummary(static_cast<LibcssAdapterNode*>(node)));
    (void)pw;
    const auto* adapter = static_cast<LibcssAdapterNode*>(node);
    *match = adapter->context->focused;
    return CSS_OK;
}

css_error nodeIsTarget(void* pw, void* node, bool* match) {
    pushTrace(pw, "node_is_target " + nodeSummary(static_cast<LibcssAdapterNode*>(node)));
    (void)pw;
    const auto* adapter = static_cast<LibcssAdapterNode*>(node);
    *match = adapter->context->visible;
    return CSS_OK;
}

css_error noBool(void* pw, void* node, bool* match) {
    (void)pw;
    (void)node;
    *match = false;
    return CSS_OK;
}

css_error nodeCountSiblings(void* pw, void* node, bool same_name, bool after, int32_t* count) {
    pushTrace(pw, "node_count_siblings " + nodeSummary(static_cast<LibcssAdapterNode*>(node)));
    (void)pw;
    (void)node;
    (void)same_name;
    (void)after;
    *count = 0;
    return CSS_OK;
}

css_error nodePresentationalHint(void* pw, void* node, uint32_t* nhints, css_hint** hints) {
    pushTrace(pw, "node_presentational_hint " + nodeSummary(static_cast<LibcssAdapterNode*>(node)));
    (void)pw;
    (void)node;
    *nhints = 0;
    *hints = nullptr;
    return CSS_OK;
}

css_error uaDefaultForProperty(void* pw, uint32_t property, css_hint* hint) {
    pushTrace(pw, "ua_default_for_property");
    (void)pw;
    (void)property;
    (void)hint;
    return CSS_INVALID;
}

css_error nodeDataSet(void* pw, void* node, void* libcss_node_data) {
    pushTrace(pw, "set_libcss_node_data " + nodeSummary(static_cast<LibcssAdapterNode*>(node)));
    (void)pw;
    (void)node;
    (void)libcss_node_data;
    return CSS_OK;
}

css_error nodeDataGet(void* pw, void* node, void** libcss_node_data) {
    pushTrace(pw, "get_libcss_node_data " + nodeSummary(static_cast<LibcssAdapterNode*>(node)));
    (void)pw;
    (void)node;
    *libcss_node_data = nullptr;
    return CSS_OK;
}

} // namespace

std::vector<LibcssAdapterNode> buildLibcssAdapterChain(const StyleNodeContext& node) {
    std::vector<const StyleNodeContext*> reversed;
    const StyleNodeContext* current = &node;
    while (current != nullptr) {
        reversed.push_back(current);
        current = current->parent;
    }

    std::vector<LibcssAdapterNode> chain;
    chain.reserve(reversed.size());
    for (auto it = reversed.rbegin(); it != reversed.rend(); ++it) {
        chain.push_back(LibcssAdapterNode {.context = *it});
    }
    return chain;
}

std::optional<lwc_string*> internLibcssString(std::string_view text) {
    lwc_string* result = nullptr;
    if (lwc_intern_string(text.data(), text.size(), &result) != lwc_error_ok || result == nullptr) {
        return std::nullopt;
    }
    return result;
}

css_select_handler makeLibcssSelectHandler() {
    return css_select_handler {
        .handler_version = CSS_SELECT_HANDLER_VERSION_1,
        .node_name = nodeName,
        .node_classes = nodeClasses,
        .node_id = nodeId,
        .named_ancestor_node = namedAncestorNode,
        .named_parent_node = namedParentNode,
        .named_sibling_node = unsupportedNamedNodeOp,
        .named_generic_sibling_node = unsupportedNamedNodeOp,
        .parent_node = parentNode,
        .sibling_node = unsupportedNodeOp,
        .node_has_name = nodeHasName,
        .node_has_class = nodeHasClass,
        .node_has_id = nodeHasId,
        .node_has_attribute = nodeHasAttribute,
        .node_has_attribute_equal = nodeHasAttributeEqual,
        .node_has_attribute_dashmatch = nullptr,
        .node_has_attribute_includes = nullptr,
        .node_has_attribute_prefix = nullptr,
        .node_has_attribute_suffix = nullptr,
        .node_has_attribute_substring = nullptr,
        .node_is_root = nodeIsRoot,
        .node_count_siblings = nodeCountSiblings,
        .node_is_empty = noBool,
        .node_is_link = noBool,
        .node_is_visited = noBool,
        .node_is_hover = noBool,
        .node_is_active = noBool,
        .node_is_focus = nodeIsFocus,
        .node_is_enabled = noBool,
        .node_is_disabled = noBool,
        .node_is_checked = noBool,
        .node_is_target = nodeIsTarget,
        .node_is_lang = nullptr,
        .node_presentational_hint = nodePresentationalHint,
        .ua_default_for_property = uaDefaultForProperty,
        .set_libcss_node_data = nodeDataSet,
        .get_libcss_node_data = nodeDataGet,
    };
}

LibcssSelectorMatchResult libcssMatchSelector(const std::string& selector, const StyleNodeContext& node) {
    LibcssSelectorMatchResult result;
    const std::string stylesheet = selector + " { display: none; }";

    css_stylesheet* sheet = nullptr;
    css_select_ctx* ctx = nullptr;
    const css_stylesheet_params params {
        .params_version = CSS_STYLESHEET_PARAMS_VERSION_1,
        .level = CSS_LEVEL_DEFAULT,
        .charset = "UTF-8",
        .url = "hypreact://selector-match",
        .title = "selector-match",
        .allow_quirks = false,
        .inline_style = false,
        .resolve = nullptr,
        .resolve_pw = nullptr,
        .import = nullptr,
        .import_pw = nullptr,
        .color = nullptr,
        .color_pw = nullptr,
        .font = nullptr,
        .font_pw = nullptr,
    };

    if (css_stylesheet_create(&params, &sheet) != CSS_OK || sheet == nullptr) {
        result.diagnostics.push_back("failed to create libcss stylesheet");
        return result;
    }

    const auto append = css_stylesheet_append_data(sheet, reinterpret_cast<const uint8_t*>(stylesheet.data()), stylesheet.size());
    const auto done = append == CSS_OK ? css_stylesheet_data_done(sheet) : append;
    if (append != CSS_OK || done != CSS_OK) {
        css_stylesheet_destroy(sheet);
        result.diagnostics.push_back("selector stylesheet parse failed");
        return result;
    }
    result.parsed = true;

    if (css_select_ctx_create(&ctx) != CSS_OK || ctx == nullptr) {
        css_stylesheet_destroy(sheet);
        result.diagnostics.push_back("failed to create libcss select context");
        return result;
    }

    if (css_select_ctx_append_sheet(ctx, sheet, CSS_ORIGIN_AUTHOR, "screen") != CSS_OK) {
        css_select_ctx_destroy(ctx);
        css_stylesheet_destroy(sheet);
        result.diagnostics.push_back("failed to append selector stylesheet");
        return result;
    }

    TraceCollector collector {.trace = &result.trace};
    auto handler = makeLibcssSelectHandler();
    auto chain = buildLibcssAdapterChain(node);
    const css_media media {.type = CSS_MEDIA_SCREEN};
    const css_unit_ctx unitCtx {
        .viewport_width = 1920,
        .viewport_height = 1080,
        .font_size_default = 16,
        .font_size_minimum = 0,
        .device_dpi = 96,
        .root_style = nullptr,
        .pw = nullptr,
        .measure = nullptr,
    };

    css_select_results* results = nullptr;
    const auto selected = css_select_style(ctx, &chain.back(), &unitCtx, &media, nullptr, &handler, &collector, &results) == CSS_OK && results != nullptr;
    if (selected) {
        result.matched = css_computed_display(results->styles[CSS_PSEUDO_ELEMENT_NONE], false) == CSS_DISPLAY_NONE;
        if (!result.matched) {
            result.diagnostics.push_back("selector resolved but sentinel display property was not applied");
            result.diagnostics.push_back("selector='" + selector + "' nodeType='" + std::string(domain::toString(node.type)) + "' id='" + node.id + "'");
        }
        css_select_results_destroy(results);
    } else {
        result.diagnostics.push_back("libcss selection did not return computed results");
        result.diagnostics.push_back("selector='" + selector + "' nodeType='" + std::string(domain::toString(node.type)) + "' id='" + node.id + "'");
    }

    css_select_ctx_destroy(ctx);
    css_stylesheet_destroy(sheet);

    return result;
}

bool libcssSelectorMatches(const std::string& selector, const StyleNodeContext& node) {
    return libcssMatchSelector(selector, node).matched;
}

} // namespace hypreact::style
