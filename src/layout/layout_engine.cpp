#include "layout_engine.hpp"

#include "style/stylesheet.hpp"

#include <optional>

#include <yoga/YGNode.h>
#include <yoga/YGNodeLayout.h>
#include <yoga/YGNodeStyle.h>

namespace {

using hypreact::domain::LayoutNodeType;
using hypreact::domain::WindowSnapshot;
using hypreact::layout::ComputedNode;
using hypreact::layout::GeometryResult;
using hypreact::layout::ResolvedNode;
using hypreact::style::LengthUnit;
using hypreact::style::LengthValue;

std::optional<YGJustify> parseJustify(const std::string& value) {
    if (value == "flex-start") return YGJustifyFlexStart;
    if (value == "center") return YGJustifyCenter;
    if (value == "flex-end") return YGJustifyFlexEnd;
    if (value == "space-between") return YGJustifySpaceBetween;
    if (value == "space-around") return YGJustifySpaceAround;
    if (value == "space-evenly") return YGJustifySpaceEvenly;
    if (value == "stretch") return YGJustifyStretch;
    if (value == "start") return YGJustifyStart;
    if (value == "end") return YGJustifyEnd;
    return std::nullopt;
}

std::optional<YGAlign> parseAlign(const std::string& value) {
    if (value == "auto") return YGAlignAuto;
    if (value == "flex-start") return YGAlignFlexStart;
    if (value == "center") return YGAlignCenter;
    if (value == "flex-end") return YGAlignFlexEnd;
    if (value == "stretch") return YGAlignStretch;
    if (value == "baseline") return YGAlignBaseline;
    if (value == "space-between") return YGAlignSpaceBetween;
    if (value == "space-around") return YGAlignSpaceAround;
    if (value == "space-evenly") return YGAlignSpaceEvenly;
    if (value == "start") return YGAlignStart;
    if (value == "end") return YGAlignEnd;
    return std::nullopt;
}

std::optional<YGWrap> parseWrap(const std::string& value) {
    if (value == "nowrap") return YGWrapNoWrap;
    if (value == "wrap") return YGWrapWrap;
    if (value == "wrap-reverse") return YGWrapWrapReverse;
    return std::nullopt;
}

std::optional<YGOverflow> parseOverflow(const std::string& value) {
    if (value == "visible") return YGOverflowVisible;
    if (value == "hidden") return YGOverflowHidden;
    if (value == "scroll") return YGOverflowScroll;
    return std::nullopt;
}

std::optional<YGBoxSizing> parseBoxSizing(const std::string& value) {
    if (value == "border-box") return YGBoxSizingBorderBox;
    if (value == "content-box") return YGBoxSizingContentBox;
    return std::nullopt;
}

std::optional<YGPositionType> parsePositionType(const std::string& value) {
    if (value == "static") return YGPositionTypeStatic;
    if (value == "relative") return YGPositionTypeRelative;
    if (value == "absolute") return YGPositionTypeAbsolute;
    return std::nullopt;
}

void applyInsetEdges(
    YGNodeRef yogaNode,
    const std::optional<LengthValue>& all,
    const std::optional<LengthValue>& top,
    const std::optional<LengthValue>& right,
    const std::optional<LengthValue>& bottom,
    const std::optional<LengthValue>& left,
    void (*pointSetter)(YGNodeRef, YGEdge, float),
    void (*percentSetter)(YGNodeRef, YGEdge, float),
    void (*autoSetter)(YGNodeRef, YGEdge)
) {
    auto apply = [&](YGEdge edge, const std::optional<LengthValue>& value) {
        if (!value.has_value()) {
            return;
        }

        switch (value->unit) {
            case LengthUnit::Points:
                pointSetter(yogaNode, edge, value->value);
                break;
            case LengthUnit::Percent:
                percentSetter(yogaNode, edge, value->value);
                break;
            case LengthUnit::Auto:
                if (autoSetter != nullptr) {
                    autoSetter(yogaNode, edge);
                }
                break;
        }
    };

    apply(YGEdgeAll, all);
    apply(YGEdgeTop, top);
    apply(YGEdgeRight, right);
    apply(YGEdgeBottom, bottom);
    apply(YGEdgeLeft, left);
}

void applyDimension(
    YGNodeRef yogaNode,
    const std::optional<LengthValue>& value,
    void (*pointSetter)(YGNodeRef, float),
    void (*percentSetter)(YGNodeRef, float),
    void (*autoSetter)(YGNodeRef)
) {
    if (!value.has_value()) {
        return;
    }

    switch (value->unit) {
        case LengthUnit::Points:
            pointSetter(yogaNode, value->value);
            break;
        case LengthUnit::Percent:
            percentSetter(yogaNode, value->value);
            break;
        case LengthUnit::Auto:
            if (autoSetter != nullptr) {
                autoSetter(yogaNode);
            }
            break;
    }
}

void applyPositionEdges(
    YGNodeRef yogaNode,
    const std::optional<LengthValue>& top,
    const std::optional<LengthValue>& right,
    const std::optional<LengthValue>& bottom,
    const std::optional<LengthValue>& left
) {
    auto apply = [&](YGEdge edge, const std::optional<LengthValue>& value) {
        if (!value.has_value()) {
            return;
        }

        switch (value->unit) {
            case LengthUnit::Points:
                YGNodeStyleSetPosition(yogaNode, edge, value->value);
                break;
            case LengthUnit::Percent:
                YGNodeStyleSetPositionPercent(yogaNode, edge, value->value);
                break;
            case LengthUnit::Auto:
                YGNodeStyleSetPositionAuto(yogaNode, edge);
                break;
        }
    };

    apply(YGEdgeTop, top);
    apply(YGEdgeRight, right);
    apply(YGEdgeBottom, bottom);
    apply(YGEdgeLeft, left);
}

YGNodeRef buildYogaTree(
    const ResolvedNode& node,
    const hypreact::style::Stylesheet& stylesheet,
    std::vector<YGNodeRef>& allocated,
    const hypreact::style::StyleNodeContext* parentContext = nullptr
) {
    YGNodeRef yogaNode = YGNodeNew();
    allocated.push_back(yogaNode);

    const hypreact::style::StyleNodeContext styleContext {
        .type = node.type,
        .id = node.id,
        .classes = node.classes,
        .parent = parentContext,
        .window = node.primaryWindow,
        .focused = node.primaryWindow != nullptr && node.primaryWindow->focused,
        .focusedWithin = node.focusedWithin,
        .fullscreen = node.primaryWindow != nullptr && node.primaryWindow->fullscreen,
        .floating = node.primaryWindow != nullptr && node.primaryWindow->floating,
        .urgent = node.primaryWindow != nullptr && node.primaryWindow->urgent,
        .visible = node.visible,
        .specialWorkspace = node.specialWorkspace,
    };
    const auto style = hypreact::style::computeStyle(stylesheet, styleContext);

    if (style.display.has_value() && *style.display == "none") {
        YGNodeStyleSetDisplay(yogaNode, YGDisplayNone);
    } else {
        YGNodeStyleSetDisplay(yogaNode, YGDisplayFlex);
    }

    if (style.position.has_value()) {
        if (const auto position = parsePositionType(*style.position); position.has_value()) {
            YGNodeStyleSetPositionType(yogaNode, *position);
        }
    }

    if (style.flexDirection.has_value() && *style.flexDirection == "column") {
        YGNodeStyleSetFlexDirection(yogaNode, YGFlexDirectionColumn);
    } else {
        YGNodeStyleSetFlexDirection(yogaNode, YGFlexDirectionRow);
    }

    if (style.flexWrap.has_value()) {
        if (const auto wrap = parseWrap(*style.flexWrap); wrap.has_value()) {
            YGNodeStyleSetFlexWrap(yogaNode, *wrap);
        }
    }

    if (style.flexGrow.has_value()) {
        YGNodeStyleSetFlexGrow(yogaNode, *style.flexGrow);
    } else if (node.type == LayoutNodeType::Slot) {
        const float claimCount = static_cast<float>(node.claimedWindows.empty() ? 1 : node.claimedWindows.size());
        YGNodeStyleSetFlexGrow(yogaNode, claimCount);
    } else {
        YGNodeStyleSetFlexGrow(yogaNode, 1.0f);
    }

    if (style.flexShrink.has_value()) {
        YGNodeStyleSetFlexShrink(yogaNode, *style.flexShrink);
    }

    applyDimension(
        yogaNode,
        style.flexBasis,
        YGNodeStyleSetFlexBasis,
        YGNodeStyleSetFlexBasisPercent,
        YGNodeStyleSetFlexBasisAuto
    );

    if (style.gap.has_value()) {
        YGNodeStyleSetGap(yogaNode, YGGutterAll, *style.gap);
    }

    if (style.rowGap.has_value()) {
        YGNodeStyleSetGap(yogaNode, YGGutterRow, *style.rowGap);
    }

    if (style.columnGap.has_value()) {
        YGNodeStyleSetGap(yogaNode, YGGutterColumn, *style.columnGap);
    }

    if (style.justifyContent.has_value()) {
        if (const auto justify = parseJustify(*style.justifyContent); justify.has_value()) {
            YGNodeStyleSetJustifyContent(yogaNode, *justify);
        }
    }

    if (style.alignItems.has_value()) {
        if (const auto align = parseAlign(*style.alignItems); align.has_value()) {
            YGNodeStyleSetAlignItems(yogaNode, *align);
        }
    }

    if (style.alignSelf.has_value()) {
        if (const auto align = parseAlign(*style.alignSelf); align.has_value()) {
            YGNodeStyleSetAlignSelf(yogaNode, *align);
        }
    }

    if (style.alignContent.has_value()) {
        if (const auto align = parseAlign(*style.alignContent); align.has_value()) {
            YGNodeStyleSetAlignContent(yogaNode, *align);
        }
    }

    if (style.overflow.has_value()) {
        if (const auto overflow = parseOverflow(*style.overflow); overflow.has_value()) {
            YGNodeStyleSetOverflow(yogaNode, *overflow);
        }
    }

    if (style.boxSizing.has_value()) {
        if (const auto boxSizing = parseBoxSizing(*style.boxSizing); boxSizing.has_value()) {
            YGNodeStyleSetBoxSizing(yogaNode, *boxSizing);
        }
    }

    applyDimension(yogaNode, style.width, YGNodeStyleSetWidth, YGNodeStyleSetWidthPercent, YGNodeStyleSetWidthAuto);
    applyDimension(yogaNode, style.height, YGNodeStyleSetHeight, YGNodeStyleSetHeightPercent, YGNodeStyleSetHeightAuto);
    applyDimension(yogaNode, style.minWidth, YGNodeStyleSetMinWidth, YGNodeStyleSetMinWidthPercent, nullptr);
    applyDimension(yogaNode, style.minHeight, YGNodeStyleSetMinHeight, YGNodeStyleSetMinHeightPercent, nullptr);
    applyDimension(yogaNode, style.maxWidth, YGNodeStyleSetMaxWidth, YGNodeStyleSetMaxWidthPercent, nullptr);
    applyDimension(yogaNode, style.maxHeight, YGNodeStyleSetMaxHeight, YGNodeStyleSetMaxHeightPercent, nullptr);
    if (style.aspectRatio.has_value()) {
        YGNodeStyleSetAspectRatio(yogaNode, *style.aspectRatio);
    }
    applyPositionEdges(yogaNode, style.top, style.right, style.bottom, style.left);

    if (style.borderWidth.has_value()) {
        YGNodeStyleSetBorder(yogaNode, YGEdgeAll, *style.borderWidth);
    }
    if (style.borderTopWidth.has_value()) {
        YGNodeStyleSetBorder(yogaNode, YGEdgeTop, *style.borderTopWidth);
    }
    if (style.borderRightWidth.has_value()) {
        YGNodeStyleSetBorder(yogaNode, YGEdgeRight, *style.borderRightWidth);
    }
    if (style.borderBottomWidth.has_value()) {
        YGNodeStyleSetBorder(yogaNode, YGEdgeBottom, *style.borderBottomWidth);
    }
    if (style.borderLeftWidth.has_value()) {
        YGNodeStyleSetBorder(yogaNode, YGEdgeLeft, *style.borderLeftWidth);
    }

    applyInsetEdges(
        yogaNode,
        style.margin,
        style.marginTop,
        style.marginRight,
        style.marginBottom,
        style.marginLeft,
        YGNodeStyleSetMargin,
        YGNodeStyleSetMarginPercent,
        YGNodeStyleSetMarginAuto
    );
    applyInsetEdges(
        yogaNode,
        style.padding,
        style.paddingTop,
        style.paddingRight,
        style.paddingBottom,
        style.paddingLeft,
        YGNodeStyleSetPadding,
        YGNodeStyleSetPaddingPercent,
        nullptr
    );

    for (const auto& child : node.children) {
        YGNodeRef childNode = buildYogaTree(child, stylesheet, allocated, &styleContext);
        YGNodeInsertChild(yogaNode, childNode, YGNodeGetChildCount(yogaNode));
    }

    return yogaNode;
}

ComputedNode collectComputedTree(const ResolvedNode& node, YGNodeRef yogaNode) {
    ComputedNode computed {
        .node = node,
        .box = {
            .x = YGNodeLayoutGetLeft(yogaNode),
            .y = YGNodeLayoutGetTop(yogaNode),
            .width = YGNodeLayoutGetWidth(yogaNode),
            .height = YGNodeLayoutGetHeight(yogaNode),
        },
    };

    for (std::size_t index = 0; index < node.children.size(); ++index) {
        computed.children.push_back(collectComputedTree(node.children[index], YGNodeGetChild(yogaNode, index)));
    }

    return computed;
}

} // namespace

namespace hypreact::layout {

std::string LayoutEngine::dependencySummary() {
    return "Yoga-backed layout engine ready";
}

GeometryResult LayoutEngine::compute(const ResolutionResult& resolved, const domain::RuntimeSnapshot& snapshot, const style::Stylesheet& stylesheet) {
    std::vector<YGNodeRef> allocated;
    YGNodeRef root = buildYogaTree(resolved.root, stylesheet, allocated, nullptr);

    YGNodeCalculateLayout(
        root,
        static_cast<float>(snapshot.monitor.width),
        static_cast<float>(snapshot.monitor.height),
        YGDirectionLTR
    );

    GeometryResult result {
        .root = collectComputedTree(resolved.root, root),
    };

    for (auto it = allocated.rbegin(); it != allocated.rend(); ++it) {
        YGNodeFree(*it);
    }

    return result;
}

} // namespace hypreact::layout
