#include "stylesheet.hpp"

#include <algorithm>
#include <charconv>
#include <sstream>
#include <string_view>
#include <tuple>

#include "libcss_selector_adapter.hpp"

namespace hypreact::style {

namespace {

std::string_view windowAttributeValue(const domain::WindowSnapshot& window, WindowAttribute attribute) {
    switch (attribute) {
        case WindowAttribute::AppId:
            return window.initialClass;
        case WindowAttribute::Class:
            return window.klass;
        case WindowAttribute::Title:
            return window.title;
        case WindowAttribute::Floating:
            return window.floating ? "true" : "false";
        case WindowAttribute::Fullscreen:
            return window.fullscreen ? "true" : "false";
    }

    return {};
}

std::optional<std::string> pseudoClassName(PseudoClass pseudoclass) {
    switch (pseudoclass) {
        case PseudoClass::Focused: return std::string("focus");
        case PseudoClass::Visible: return std::string("target");
        case PseudoClass::FocusedWithin:
        case PseudoClass::Fullscreen:
        case PseudoClass::Floating:
        case PseudoClass::Urgent:
        case PseudoClass::SpecialWorkspace:
            return std::nullopt;
    }

    return std::nullopt;
}

std::string attributeName(WindowAttribute attribute) {
    switch (attribute) {
        case WindowAttribute::AppId: return "app-id";
        case WindowAttribute::Class: return "class";
        case WindowAttribute::Title: return "title";
        case WindowAttribute::Floating: return "floating";
        case WindowAttribute::Fullscreen: return "fullscreen";
    }

    return {};
}

std::optional<std::string> serializeSimpleSelector(const SimpleSelector& selector) {
    std::string result;
    if (selector.universal && !selector.type.has_value()) {
        result += "*";
    } else if (selector.type.has_value()) {
        result += *selector.type;
    }

    if (selector.id.has_value()) {
        result += "#" + *selector.id;
    }

    for (const auto& className : selector.classes) {
        result += "." + className;
    }

    for (const auto& attribute : selector.attributes) {
        result += "[" + attributeName(attribute.attribute) + "=\"" + attribute.value + "\"]";
    }

    for (const auto pseudoclass : selector.pseudoclasses) {
        const auto serialized = pseudoClassName(pseudoclass);
        if (!serialized.has_value()) {
            return std::nullopt;
        }
        result += ":" + *serialized;
    }

    return result;
}

bool shouldUseLibcssSelector(const Selector& selector) {
    const auto hasRepresentableSimpleSelector = [](const SimpleSelector& simple) {
        return simple.universal || simple.type.has_value() || simple.id.has_value() || !simple.classes.empty()
            || !simple.attributes.empty() || !simple.pseudoclasses.empty();
    };

    if (!hasRepresentableSimpleSelector(selector.target)) {
        return false;
    }

    if (selector.directParent.has_value() && !hasRepresentableSimpleSelector(*selector.directParent)) {
        return false;
    }

    if (selector.ancestor.has_value() && !hasRepresentableSimpleSelector(*selector.ancestor)) {
        return false;
    }

    return true;
}

bool productionMatchesSelector(const Selector& selector, const StyleNodeContext& node, ComputeStyleDiagnostics* diagnostics) {
    const bool fallbackMatched = matchesSelector(selector, node);
    (void)diagnostics;
    return fallbackMatched;
}

} // namespace

std::optional<float> parseNumber(const std::string& value) {
    float result = 0.0f;
    const auto* begin = value.data();
    const auto* end = value.data() + value.size();
    auto parsed = std::from_chars(begin, end, result);
    if (parsed.ec == std::errc {} && parsed.ptr != begin) {
        return result;
    }
    return std::nullopt;
}

std::optional<LengthValue> parseLengthValue(const std::string& value) {
    if (value == "auto") {
        return LengthValue {
            .unit = LengthUnit::Auto,
            .value = 0.0f,
        };
    }

    if (value.size() > 1 && value.back() == '%') {
        if (const auto parsed = parseNumber(value.substr(0, value.size() - 1)); parsed.has_value()) {
            return LengthValue {
                .unit = LengthUnit::Percent,
                .value = *parsed,
            };
        }
        return std::nullopt;
    }

    constexpr std::string_view pxSuffix = "px";
    if (value.size() > pxSuffix.size() && value.ends_with(pxSuffix)) {
        if (const auto parsed = parseNumber(value.substr(0, value.size() - pxSuffix.size())); parsed.has_value()) {
            return LengthValue {
                .unit = LengthUnit::Points,
                .value = *parsed,
            };
        }
        return std::nullopt;
    }

    if (const auto parsed = parseNumber(value); parsed.has_value()) {
        return LengthValue {
            .unit = LengthUnit::Points,
            .value = *parsed,
        };
    }

    return std::nullopt;
}

bool matchesSimpleSelector(const SimpleSelector& selector, const StyleNodeContext& node) {
    if (
        selector.universal && !selector.type.has_value() && !selector.id.has_value() && selector.classes.empty()
        && selector.attributes.empty() && selector.pseudoclasses.empty()
    ) {
        return true;
    }

    if (selector.type.has_value() && *selector.type != domain::toString(node.type)) {
        return false;
    }

    if (selector.id.has_value() && *selector.id != node.id) {
        return false;
    }

    for (const auto& className : selector.classes) {
        if (std::find(node.classes.begin(), node.classes.end(), className) == node.classes.end()) {
            return false;
        }
    }

    for (const auto& attribute : selector.attributes) {
        if (node.window == nullptr || windowAttributeValue(*node.window, attribute.attribute) != attribute.value) {
            return false;
        }
    }

    for (const auto pseudoclass : selector.pseudoclasses) {
        switch (pseudoclass) {
            case PseudoClass::Focused:
                if (!node.focused) {
                    return false;
                }
                break;
            case PseudoClass::FocusedWithin:
                if (!node.focusedWithin) {
                    return false;
                }
                break;
            case PseudoClass::Fullscreen:
                if (!node.fullscreen) {
                    return false;
                }
                break;
            case PseudoClass::Floating:
                if (!node.floating) {
                    return false;
                }
                break;
            case PseudoClass::Urgent:
                if (!node.urgent) {
                    return false;
                }
                break;
            case PseudoClass::Visible:
                if (!node.visible) {
                    return false;
                }
                break;
            case PseudoClass::SpecialWorkspace:
                if (!node.specialWorkspace) {
                    return false;
                }
                break;
        }
    }

    return selector.universal || selector.type.has_value() || selector.id.has_value() || !selector.classes.empty()
        || !selector.attributes.empty() || !selector.pseudoclasses.empty();
}

bool matchesSelector(const Selector& selector, const StyleNodeContext& node) {
    if (!matchesSimpleSelector(selector.target, node)) {
        return false;
    }

    if (selector.directParent.has_value()) {
        if (node.parent == nullptr) {
            return false;
        }

        return matchesSimpleSelector(*selector.directParent, *node.parent);
    }

    if (selector.ancestor.has_value()) {
        const StyleNodeContext* current = node.parent;
        while (current != nullptr) {
            if (matchesSimpleSelector(*selector.ancestor, *current)) {
                return true;
            }
            current = current->parent;
        }

        return false;
    }

    return true;
}

bool matchesSelector(const Selector& selector, const domain::LayoutNode& node) {
    const StyleNodeContext context {
        .type = node.type,
        .id = node.id,
        .classes = node.classes,
    };
    return matchesSelector(selector, context);
}

std::optional<std::string> serializeSelectorForLibcss(const Selector& selector) {
    if (!shouldUseLibcssSelector(selector)) {
        return std::nullopt;
    }

    std::ostringstream output;
    if (selector.ancestor.has_value()) {
        const auto ancestor = serializeSimpleSelector(*selector.ancestor);
        if (!ancestor.has_value()) {
            return std::nullopt;
        }
        output << *ancestor << " ";
    }
    if (selector.directParent.has_value()) {
        const auto parent = serializeSimpleSelector(*selector.directParent);
        if (!parent.has_value()) {
            return std::nullopt;
        }
        output << *parent << " > ";
    }
    const auto target = serializeSimpleSelector(selector.target);
    if (!target.has_value()) {
        return std::nullopt;
    }
    output << *target;
    return output.str();
}

int selectorSpecificity(const SimpleSelector& selector) {
    const int idScore = selector.id.has_value() ? 100 : 0;
    const int classScore = static_cast<int>(selector.classes.size() + selector.attributes.size() + selector.pseudoclasses.size()) * 10;
    const int typeScore = selector.type.has_value() ? 1 : 0;
    return idScore + classScore + typeScore;
}

int selectorSpecificity(const Selector& selector) {
    int specificity = selectorSpecificity(selector.target);
    if (selector.directParent.has_value()) {
        specificity += selectorSpecificity(*selector.directParent);
    }
    if (selector.ancestor.has_value()) {
        specificity += selectorSpecificity(*selector.ancestor);
    }
    return specificity;
}

ComputedStyle computeStyle(const Stylesheet& stylesheet, const StyleNodeContext& node) {
    return computeStyle(stylesheet, node, nullptr);
}

ComputedStyle computeStyle(const Stylesheet& stylesheet, const StyleNodeContext& node, ComputeStyleDiagnostics* diagnostics) {
    ComputedStyle style;

    std::vector<std::tuple<int, std::size_t, const Declaration*>> matchedDeclarations;

    for (const auto& rule : stylesheet.rules) {
        if (!productionMatchesSelector(rule.selector, node, diagnostics)) {
            continue;
        }

        for (const auto& declaration : rule.declarations) {
            matchedDeclarations.emplace_back(rule.specificity, rule.sourceOrder, &declaration);
        }
    }

    std::stable_sort(
        matchedDeclarations.begin(),
        matchedDeclarations.end(),
        [](const auto& left, const auto& right) {
            if (std::get<0>(left) != std::get<0>(right)) {
                return std::get<0>(left) < std::get<0>(right);
            }
            return std::get<1>(left) < std::get<1>(right);
        }
    );

    for (const auto& [specificity, sourceOrder, declaration] : matchedDeclarations) {
        (void)specificity;
        (void)sourceOrder;

        switch (declaration->property) {
            case PropertyId::Display: style.display = declaration->value; break;
            case PropertyId::Position: style.position = declaration->value; break;
            case PropertyId::Top: style.top = parseLengthValue(declaration->value); break;
            case PropertyId::Right: style.right = parseLengthValue(declaration->value); break;
            case PropertyId::Bottom: style.bottom = parseLengthValue(declaration->value); break;
            case PropertyId::Left: style.left = parseLengthValue(declaration->value); break;
            case PropertyId::FlexDirection: style.flexDirection = declaration->value; break;
            case PropertyId::FlexWrap: style.flexWrap = declaration->value; break;
            case PropertyId::FlexGrow: style.flexGrow = parseNumber(declaration->value); break;
            case PropertyId::FlexShrink: style.flexShrink = parseNumber(declaration->value); break;
            case PropertyId::FlexBasis: style.flexBasis = parseLengthValue(declaration->value); break;
            case PropertyId::Gap: style.gap = parseNumber(declaration->value); break;
            case PropertyId::RowGap: style.rowGap = parseNumber(declaration->value); break;
            case PropertyId::ColumnGap: style.columnGap = parseNumber(declaration->value); break;
            case PropertyId::JustifyContent: style.justifyContent = declaration->value; break;
            case PropertyId::AlignItems: style.alignItems = declaration->value; break;
            case PropertyId::AlignSelf: style.alignSelf = declaration->value; break;
            case PropertyId::AlignContent: style.alignContent = declaration->value; break;
            case PropertyId::Overflow: style.overflow = declaration->value; break;
            case PropertyId::Width: style.width = parseLengthValue(declaration->value); break;
            case PropertyId::Height: style.height = parseLengthValue(declaration->value); break;
            case PropertyId::MinWidth: style.minWidth = parseLengthValue(declaration->value); break;
            case PropertyId::MinHeight: style.minHeight = parseLengthValue(declaration->value); break;
            case PropertyId::MaxWidth: style.maxWidth = parseLengthValue(declaration->value); break;
            case PropertyId::MaxHeight: style.maxHeight = parseLengthValue(declaration->value); break;
            case PropertyId::AspectRatio: style.aspectRatio = parseNumber(declaration->value); break;
            case PropertyId::BoxSizing: style.boxSizing = declaration->value; break;
            case PropertyId::BorderWidth: style.borderWidth = parseNumber(declaration->value); break;
            case PropertyId::BorderTopWidth: style.borderTopWidth = parseNumber(declaration->value); break;
            case PropertyId::BorderRightWidth: style.borderRightWidth = parseNumber(declaration->value); break;
            case PropertyId::BorderBottomWidth: style.borderBottomWidth = parseNumber(declaration->value); break;
            case PropertyId::BorderLeftWidth: style.borderLeftWidth = parseNumber(declaration->value); break;
            case PropertyId::Margin: style.margin = parseLengthValue(declaration->value); break;
            case PropertyId::MarginTop: style.marginTop = parseLengthValue(declaration->value); break;
            case PropertyId::MarginRight: style.marginRight = parseLengthValue(declaration->value); break;
            case PropertyId::MarginBottom: style.marginBottom = parseLengthValue(declaration->value); break;
            case PropertyId::MarginLeft: style.marginLeft = parseLengthValue(declaration->value); break;
            case PropertyId::Padding: style.padding = parseLengthValue(declaration->value); break;
            case PropertyId::PaddingTop: style.paddingTop = parseLengthValue(declaration->value); break;
            case PropertyId::PaddingRight: style.paddingRight = parseLengthValue(declaration->value); break;
            case PropertyId::PaddingBottom: style.paddingBottom = parseLengthValue(declaration->value); break;
            case PropertyId::PaddingLeft: style.paddingLeft = parseLengthValue(declaration->value); break;
        }
    }

    return style;
}

ComputedStyle computeStyle(const Stylesheet& stylesheet, const domain::LayoutNode& node) {
    const StyleNodeContext context {
        .type = node.type,
        .id = node.id,
        .classes = node.classes,
    };
    return computeStyle(stylesheet, context);
}

} // namespace hypreact::style
