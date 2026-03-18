#include "libcss_probe.hpp"

#if defined(__cplusplus) && !defined(restrict)
#define restrict __restrict
#endif

#include <libcss/libcss.h>
#include <libcss/computed.h>
#include <libcss/fpmath.h>
#include <libcss/properties.h>
#include <libcss/unit.h>

#include <optional>

#include "libcss_selector_adapter.hpp"

namespace hypreact::style {

namespace {

struct ProbeTraceCollector {
    std::vector<std::string>* trace = nullptr;
};

struct ProbeComputedSubset {
    std::optional<std::string> display;
    std::optional<std::string> position;
    std::optional<std::string> overflow;
    std::optional<std::string> boxSizing;
    std::optional<float> width;
    std::optional<float> height;
    std::optional<float> minWidth;
    std::optional<float> maxWidth;
    std::optional<float> minHeight;
    std::optional<float> maxHeight;
    std::optional<float> marginLeft;
    std::optional<float> paddingRight;
    std::optional<float> left;
    std::optional<float> top;
    std::optional<float> right;
    std::optional<float> bottom;
};

void pushProbeTrace(void* pw, const std::string& event) {
    auto* collector = static_cast<ProbeTraceCollector*>(pw);
    if (collector != nullptr && collector->trace != nullptr) {
        collector->trace->push_back(event);
    }
}

std::optional<float> cssLengthToFloat(const css_computed_style* style, const css_unit_ctx& unitCtx, css_fixed length, css_unit unit) {
    if (unit == CSS_UNIT_PX || unit == CSS_UNIT_PCT) {
        return FIXTOFLT(length);
    }

    return FIXTOFLT(css_unit_len2css_px(style, &unitCtx, length, unit));
}

std::optional<std::string> displayFromComputed(const css_computed_style* style) {
    if (style == nullptr) return std::nullopt;
    switch (css_computed_display(style, false)) {
        case CSS_DISPLAY_NONE: return std::string("none");
        case CSS_DISPLAY_FLEX: return std::string("flex");
        default: return std::nullopt;
    }
}

std::optional<std::string> positionFromComputed(const css_computed_style* style) {
    if (style == nullptr) return std::nullopt;
    switch (css_computed_position(style)) {
        case CSS_POSITION_STATIC: return std::string("static");
        case CSS_POSITION_RELATIVE: return std::string("relative");
        case CSS_POSITION_ABSOLUTE: return std::string("absolute");
        case CSS_POSITION_FIXED: return std::string("fixed");
        default: return std::nullopt;
    }
}

std::optional<std::string> overflowFromComputed(const css_computed_style* style) {
    if (style == nullptr) return std::nullopt;
    switch (css_computed_overflow_x(style)) {
        case CSS_OVERFLOW_VISIBLE: return std::string("visible");
        case CSS_OVERFLOW_HIDDEN: return std::string("hidden");
        case CSS_OVERFLOW_SCROLL: return std::string("scroll");
        case CSS_OVERFLOW_AUTO: return std::string("auto");
        default: return std::nullopt;
    }
}

std::optional<std::string> boxSizingFromComputed(const css_computed_style* style) {
    if (style == nullptr) return std::nullopt;
    switch (css_computed_box_sizing(style)) {
        case CSS_BOX_SIZING_BORDER_BOX: return std::string("border-box");
        case CSS_BOX_SIZING_CONTENT_BOX: return std::string("content-box");
        default: return std::nullopt;
    }
}

ProbeComputedSubset readComputedSubset(const css_computed_style* style, const css_unit_ctx& unitCtx) {
    ProbeComputedSubset subset;
    subset.display = displayFromComputed(style);
    subset.position = positionFromComputed(style);
    subset.overflow = overflowFromComputed(style);
    subset.boxSizing = boxSizingFromComputed(style);

    css_fixed length = 0;
    css_unit unit = CSS_UNIT_PX;
    css_computed_width(style, &length, &unit);
    subset.width = cssLengthToFloat(style, unitCtx, length, unit);
    css_computed_height(style, &length, &unit);
    subset.height = cssLengthToFloat(style, unitCtx, length, unit);
    css_computed_min_width(style, &length, &unit);
    subset.minWidth = cssLengthToFloat(style, unitCtx, length, unit);
    css_computed_max_width(style, &length, &unit);
    subset.maxWidth = cssLengthToFloat(style, unitCtx, length, unit);
    css_computed_min_height(style, &length, &unit);
    subset.minHeight = cssLengthToFloat(style, unitCtx, length, unit);
    css_computed_max_height(style, &length, &unit);
    subset.maxHeight = cssLengthToFloat(style, unitCtx, length, unit);
    css_computed_margin_left(style, &length, &unit);
    subset.marginLeft = cssLengthToFloat(style, unitCtx, length, unit);
    css_computed_padding_right(style, &length, &unit);
    subset.paddingRight = cssLengthToFloat(style, unitCtx, length, unit);
    css_computed_left(style, &length, &unit);
    subset.left = cssLengthToFloat(style, unitCtx, length, unit);
    css_computed_top(style, &length, &unit);
    subset.top = cssLengthToFloat(style, unitCtx, length, unit);
    css_computed_right(style, &length, &unit);
    subset.right = cssLengthToFloat(style, unitCtx, length, unit);
    css_computed_bottom(style, &length, &unit);
    subset.bottom = cssLengthToFloat(style, unitCtx, length, unit);
    return subset;
}

bool computedSubsetDiffers(const ProbeComputedSubset& lhs, const ProbeComputedSubset& rhs) {
    return lhs.display != rhs.display
        || lhs.position != rhs.position
        || lhs.overflow != rhs.overflow
        || lhs.boxSizing != rhs.boxSizing
        || lhs.width != rhs.width
        || lhs.height != rhs.height
        || lhs.minWidth != rhs.minWidth
        || lhs.maxWidth != rhs.maxWidth
        || lhs.minHeight != rhs.minHeight
        || lhs.maxHeight != rhs.maxHeight
        || lhs.marginLeft != rhs.marginLeft
        || lhs.paddingRight != rhs.paddingRight
        || lhs.left != rhs.left
        || lhs.top != rhs.top
        || lhs.right != rhs.right
        || lhs.bottom != rhs.bottom;
}

void copySubsetToProbe(const ProbeComputedSubset& subset, LibcssSelectionProbe& probe) {
    probe.display = subset.display;
    probe.position = subset.position;
    probe.overflow = subset.overflow;
    probe.boxSizing = subset.boxSizing;
    probe.width = subset.width;
    probe.height = subset.height;
    probe.minWidth = subset.minWidth;
    probe.maxWidth = subset.maxWidth;
    probe.minHeight = subset.minHeight;
    probe.maxHeight = subset.maxHeight;
    probe.marginLeft = subset.marginLeft;
    probe.paddingRight = subset.paddingRight;
    probe.left = subset.left;
    probe.top = subset.top;
    probe.right = subset.right;
    probe.bottom = subset.bottom;
}

std::optional<ProbeComputedSubset> probeComputedSubset(
    css_select_ctx* ctx,
    const css_unit_ctx& unitCtx,
    const css_media& media,
    css_select_handler& handler,
    ProbeTraceCollector* collector,
    const std::vector<LibcssAdapterNode>& probeChain,
    bool traceSelection,
    bool* selectionSucceeded = nullptr
) {
    css_select_results* results = nullptr;
    if (traceSelection) {
        pushProbeTrace(collector, "probe_select_start");
    }

    const auto selectResult = css_select_style(ctx, const_cast<LibcssAdapterNode*>(&probeChain.back()), &unitCtx, &media, nullptr, &handler, collector, &results);
    const bool ok = selectResult == CSS_OK && results != nullptr;
    if (selectionSucceeded != nullptr) {
        *selectionSucceeded = ok;
    }

    if (!ok) {
        if (traceSelection) {
            pushProbeTrace(collector, "probe_select_failure");
        }
        return std::nullopt;
    }

    if (traceSelection) {
        pushProbeTrace(collector, "probe_select_success");
    }

    const auto subset = readComputedSubset(results->styles[CSS_PSEUDO_ELEMENT_NONE], unitCtx);
    css_select_results_destroy(results);
    return subset;
}

} // namespace

LibcssSelectionProbe probeLibcssSelection(const std::string& source, const StyleNodeContext& node) {
    LibcssSelectionProbe probe;
    css_stylesheet* sheet = nullptr;
    css_select_ctx* ctx = nullptr;

    const css_stylesheet_params params {
        .params_version = CSS_STYLESHEET_PARAMS_VERSION_1,
        .level = CSS_LEVEL_DEFAULT,
        .charset = "UTF-8",
        .url = "hypreact://probe",
        .title = "probe",
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
        probe.diagnostics.push_back("failed to create libcss stylesheet");
        probe.warnings.push_back("failed to create libcss stylesheet");
        return probe;
    }

    const auto append = css_stylesheet_append_data(sheet, reinterpret_cast<const uint8_t*>(source.data()), source.size());
    const auto done = append == CSS_OK ? css_stylesheet_data_done(sheet) : append;
    probe.parsed = append == CSS_OK && done == CSS_OK;
    if (!probe.parsed) {
        probe.diagnostics.push_back("libcss stylesheet parse failed");
        probe.warnings.push_back("libcss stylesheet parse failed");
        css_stylesheet_destroy(sheet);
        return probe;
    }

    if (css_select_ctx_create(&ctx) != CSS_OK || ctx == nullptr) {
        probe.diagnostics.push_back("failed to create libcss select context");
        probe.warnings.push_back("failed to create libcss select context");
        css_stylesheet_destroy(sheet);
        return probe;
    }

    if (css_select_ctx_append_sheet(ctx, sheet, CSS_ORIGIN_AUTHOR, "screen") != CSS_OK) {
        probe.diagnostics.push_back("failed to append stylesheet to libcss context");
        probe.warnings.push_back("failed to append stylesheet to libcss context");
        css_select_ctx_destroy(ctx);
        css_stylesheet_destroy(sheet);
        return probe;
    }

    ProbeTraceCollector collector {.trace = &probe.trace};
    auto handler = makeLibcssSelectHandler();
    auto probeChain = buildLibcssAdapterChain(node);
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

    bool selected = false;
    const auto selectedSubset = probeComputedSubset(ctx, unitCtx, media, handler, &collector, probeChain, true, &selected);
    if (selectedSubset.has_value()) {
        probe.selected = true;
        copySubsetToProbe(*selectedSubset, probe);

        css_stylesheet* baselineSheet = nullptr;
        css_select_ctx* baselineCtx = nullptr;
        if (css_stylesheet_create(&params, &baselineSheet) == CSS_OK && baselineSheet != nullptr
            && css_stylesheet_data_done(baselineSheet) == CSS_OK
            && css_select_ctx_create(&baselineCtx) == CSS_OK && baselineCtx != nullptr
            && css_select_ctx_append_sheet(baselineCtx, baselineSheet, CSS_ORIGIN_AUTHOR, "screen") == CSS_OK) {
            bool baselineSelected = false;
            ProbeTraceCollector baselineCollector;
            if (const auto baselineSubset = probeComputedSubset(baselineCtx, unitCtx, media, handler, &baselineCollector, probeChain, false, &baselineSelected);
                baselineSubset.has_value() && baselineSelected) {
                probe.authoredMatch = computedSubsetDiffers(*selectedSubset, *baselineSubset);
            }
        }

        if (baselineCtx != nullptr) {
            css_select_ctx_destroy(baselineCtx);
        }
        if (baselineSheet != nullptr) {
            css_stylesheet_destroy(baselineSheet);
        }
    } else {
        probe.diagnostics.push_back("libcss selection failed");
        probe.warnings.push_back("libcss selection failed");
    }

    css_select_ctx_destroy(ctx);
    css_stylesheet_destroy(sheet);
    return probe;
}

} // namespace hypreact::style
