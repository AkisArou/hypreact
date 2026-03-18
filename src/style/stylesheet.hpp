#pragma once

#include <optional>
#include <string>
#include <vector>

#include "domain/layout_node.hpp"
#include "domain/runtime_snapshot.hpp"

namespace hypreact::style {

enum class PropertyId {
    Display,
    Position,
    Top,
    Right,
    Bottom,
    Left,
    FlexDirection,
    FlexWrap,
    FlexGrow,
    FlexShrink,
    FlexBasis,
    Gap,
    RowGap,
    ColumnGap,
    JustifyContent,
    AlignItems,
    AlignSelf,
    AlignContent,
    Overflow,
    Width,
    Height,
    MinWidth,
    MinHeight,
    MaxWidth,
    MaxHeight,
    AspectRatio,
    BoxSizing,
    BorderWidth,
    BorderTopWidth,
    BorderRightWidth,
    BorderBottomWidth,
    BorderLeftWidth,
    Margin,
    MarginTop,
    MarginRight,
    MarginBottom,
    MarginLeft,
    Padding,
    PaddingTop,
    PaddingRight,
    PaddingBottom,
    PaddingLeft,
};

enum class LengthUnit {
    Points,
    Percent,
    Auto,
};

struct LengthValue {
    LengthUnit unit = LengthUnit::Points;
    float value = 0.0f;
};

enum class WindowAttribute {
    AppId,
    Class,
    Title,
    Floating,
    Fullscreen,
};

struct AttributeSelector {
    WindowAttribute attribute;
    std::string value;
};

enum class PseudoClass {
    Focused,
    FocusedWithin,
    Fullscreen,
    Floating,
    Urgent,
    Visible,
    SpecialWorkspace,
};

struct SimpleSelector {
    bool universal = false;
    std::optional<std::string> type;
    std::optional<std::string> id;
    std::vector<std::string> classes;
    std::vector<AttributeSelector> attributes;
    std::vector<PseudoClass> pseudoclasses;
};

struct Selector {
    SimpleSelector target;
    std::optional<SimpleSelector> directParent;
    std::optional<SimpleSelector> ancestor;
};

struct Declaration {
    PropertyId property;
    std::string value;
};

struct Rule {
    Selector selector;
    int specificity = 0;
    std::size_t sourceOrder = 0;
    std::vector<Declaration> declarations;
};

struct Stylesheet {
    std::vector<Rule> rules;
};

struct ComputedStyle {
    std::optional<std::string> display;
    std::optional<std::string> position;
    std::optional<LengthValue> top;
    std::optional<LengthValue> right;
    std::optional<LengthValue> bottom;
    std::optional<LengthValue> left;
    std::optional<std::string> flexDirection;
    std::optional<std::string> flexWrap;
    std::optional<float> flexGrow;
    std::optional<float> flexShrink;
    std::optional<LengthValue> flexBasis;
    std::optional<float> gap;
    std::optional<float> rowGap;
    std::optional<float> columnGap;
    std::optional<std::string> justifyContent;
    std::optional<std::string> alignItems;
    std::optional<std::string> alignSelf;
    std::optional<std::string> alignContent;
    std::optional<std::string> overflow;
    std::optional<LengthValue> width;
    std::optional<LengthValue> height;
    std::optional<LengthValue> minWidth;
    std::optional<LengthValue> minHeight;
    std::optional<LengthValue> maxWidth;
    std::optional<LengthValue> maxHeight;
    std::optional<float> aspectRatio;
    std::optional<std::string> boxSizing;
    std::optional<float> borderWidth;
    std::optional<float> borderTopWidth;
    std::optional<float> borderRightWidth;
    std::optional<float> borderBottomWidth;
    std::optional<float> borderLeftWidth;
    std::optional<LengthValue> margin;
    std::optional<LengthValue> marginTop;
    std::optional<LengthValue> marginRight;
    std::optional<LengthValue> marginBottom;
    std::optional<LengthValue> marginLeft;
    std::optional<LengthValue> padding;
    std::optional<LengthValue> paddingTop;
    std::optional<LengthValue> paddingRight;
    std::optional<LengthValue> paddingBottom;
    std::optional<LengthValue> paddingLeft;
};

struct StyleNodeContext {
    domain::LayoutNodeType type;
    std::string id;
    std::vector<std::string> classes;
    const StyleNodeContext* parent = nullptr;
    const domain::WindowSnapshot* window = nullptr;
    bool focused = false;
    bool focusedWithin = false;
    bool fullscreen = false;
    bool floating = false;
    bool urgent = false;
    bool visible = false;
    bool specialWorkspace = false;
};

int selectorSpecificity(const SimpleSelector& selector);
int selectorSpecificity(const Selector& selector);

bool matchesSimpleSelector(const SimpleSelector& selector, const StyleNodeContext& node);
bool matchesSelector(const Selector& selector, const StyleNodeContext& node);
bool matchesSelector(const Selector& selector, const domain::LayoutNode& node);
ComputedStyle computeStyle(const Stylesheet& stylesheet, const StyleNodeContext& node);
ComputedStyle computeStyle(const Stylesheet& stylesheet, const domain::LayoutNode& node);
std::optional<float> parseNumber(const std::string& value);
std::optional<LengthValue> parseLengthValue(const std::string& value);

} // namespace hypreact::style
