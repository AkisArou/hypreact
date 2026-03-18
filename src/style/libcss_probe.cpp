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
        probe.warnings.push_back("failed to create libcss stylesheet");
        return probe;
    }

    const auto append = css_stylesheet_append_data(sheet, reinterpret_cast<const uint8_t*>(source.data()), source.size());
    const auto done = append == CSS_OK ? css_stylesheet_data_done(sheet) : append;
    probe.parsed = append == CSS_OK && done == CSS_OK;
    if (!probe.parsed) {
        probe.warnings.push_back("libcss stylesheet parse failed");
        css_stylesheet_destroy(sheet);
        return probe;
    }

    if (css_select_ctx_create(&ctx) != CSS_OK || ctx == nullptr) {
        probe.warnings.push_back("failed to create libcss select context");
        css_stylesheet_destroy(sheet);
        return probe;
    }

    if (css_select_ctx_append_sheet(ctx, sheet, CSS_ORIGIN_AUTHOR, "screen") != CSS_OK) {
        probe.warnings.push_back("failed to append stylesheet to libcss context");
        css_select_ctx_destroy(ctx);
        css_stylesheet_destroy(sheet);
        return probe;
    }

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

    css_select_results* results = nullptr;
    const auto selectResult = css_select_style(ctx, &probeChain.back(), &unitCtx, &media, nullptr, &handler, nullptr, &results);
    if (selectResult == CSS_OK && results != nullptr) {
        probe.selected = true;
        const auto* style = results->styles[CSS_PSEUDO_ELEMENT_NONE];
        probe.display = displayFromComputed(style);
        probe.position = positionFromComputed(style);
        probe.overflow = overflowFromComputed(style);
        probe.boxSizing = boxSizingFromComputed(style);

        css_fixed length = 0;
        css_unit unit = CSS_UNIT_PX;
        css_computed_width(style, &length, &unit);
        probe.width = cssLengthToFloat(style, unitCtx, length, unit);
        css_computed_height(style, &length, &unit);
        probe.height = cssLengthToFloat(style, unitCtx, length, unit);
        css_computed_min_width(style, &length, &unit);
        probe.minWidth = cssLengthToFloat(style, unitCtx, length, unit);
        css_computed_max_width(style, &length, &unit);
        probe.maxWidth = cssLengthToFloat(style, unitCtx, length, unit);
        css_computed_min_height(style, &length, &unit);
        probe.minHeight = cssLengthToFloat(style, unitCtx, length, unit);
        css_computed_max_height(style, &length, &unit);
        probe.maxHeight = cssLengthToFloat(style, unitCtx, length, unit);
        css_computed_margin_left(style, &length, &unit);
        probe.marginLeft = cssLengthToFloat(style, unitCtx, length, unit);
        css_computed_padding_right(style, &length, &unit);
        probe.paddingRight = cssLengthToFloat(style, unitCtx, length, unit);
        css_computed_left(style, &length, &unit);
        probe.left = cssLengthToFloat(style, unitCtx, length, unit);
        css_computed_top(style, &length, &unit);
        probe.top = cssLengthToFloat(style, unitCtx, length, unit);
        css_computed_right(style, &length, &unit);
        probe.right = cssLengthToFloat(style, unitCtx, length, unit);
        css_computed_bottom(style, &length, &unit);
        probe.bottom = cssLengthToFloat(style, unitCtx, length, unit);

        css_select_results_destroy(results);
    } else {
        probe.warnings.push_back("libcss selection failed");
    }

    css_select_ctx_destroy(ctx);
    css_stylesheet_destroy(sheet);
    return probe;
}

} // namespace hypreact::style
