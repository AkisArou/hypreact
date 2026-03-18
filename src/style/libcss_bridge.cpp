#include "libcss_bridge.hpp"

#if defined(__cplusplus) && !defined(restrict)
#define restrict __restrict
#endif

#include <libcss/libcss.h>

#include <cctype>
#include <optional>
#include <string>
#include <string_view>

#include "libcss_probe.hpp"

namespace hypreact::style {

namespace {

css_error resolveUrl(void* pw, const char* base, lwc_string* rel, lwc_string** abs) {
    (void)pw;
    (void)base;
    if (rel == nullptr || abs == nullptr) {
        return CSS_BADPARM;
    }

    *abs = lwc_string_ref(rel);
    return CSS_OK;
}

struct LibcssSyntaxCheck {
    bool accepted = false;
    std::string diagnostic;
};

LibcssSyntaxCheck libcssSyntaxAccepts(const std::string& source) {
    LibcssSyntaxCheck check;
    css_stylesheet* sheet = nullptr;
    const css_stylesheet_params params {
        .params_version = CSS_STYLESHEET_PARAMS_VERSION_1,
        .level = CSS_LEVEL_DEFAULT,
        .charset = "UTF-8",
        .url = "hypreact://stylesheet",
        .title = "hypreact",
        .allow_quirks = false,
        .inline_style = false,
        .resolve = resolveUrl,
        .resolve_pw = nullptr,
        .import = nullptr,
        .import_pw = nullptr,
        .color = nullptr,
        .color_pw = nullptr,
        .font = nullptr,
        .font_pw = nullptr,
    };

    const auto create = css_stylesheet_create(&params, &sheet);
    if (create != CSS_OK || sheet == nullptr) {
        check.diagnostic = "libcss stylesheet create failed: " + std::string(css_error_to_string(create))
            + " (" + std::to_string(static_cast<int>(create)) + ")";
        return check;
    }

    const auto append = css_stylesheet_append_data(sheet, reinterpret_cast<const uint8_t*>(source.data()), source.size());
    const bool appendAccepted = append == CSS_OK || append == CSS_NEEDDATA;
    const auto done = appendAccepted ? css_stylesheet_data_done(sheet) : append;
    css_stylesheet_destroy(sheet);
    check.accepted = appendAccepted && done == CSS_OK;
    if (!check.accepted) {
        check.diagnostic = "libcss stylesheet parse failed: append=" + std::string(css_error_to_string(append))
            + " (" + std::to_string(static_cast<int>(append)) + ") done="
            + std::string(css_error_to_string(done)) + " (" + std::to_string(static_cast<int>(done)) + ")";
    }
    return check;
}

std::string trimCopy(std::string_view text) {
    const auto start = text.find_first_not_of(" \t\n\r");
    if (start == std::string_view::npos) {
        return {};
    }

    const auto end = text.find_last_not_of(" \t\n\r");
    return std::string(text.substr(start, end - start + 1));
}

bool isNameChar(char ch) {
    const unsigned char value = static_cast<unsigned char>(ch);
    return std::isalnum(value) != 0 || ch == '-' || ch == '_';
}

std::string stripComments(const std::string& source) {
    std::string result;
    result.reserve(source.size());

    std::size_t cursor = 0;
    while (cursor < source.size()) {
        const auto comment = source.find("/*", cursor);
        if (comment == std::string::npos) {
            result.append(source, cursor, std::string::npos);
            break;
        }

        result.append(source, cursor, comment - cursor);
        const auto endComment = source.find("*/", comment + 2);
        if (endComment == std::string::npos) {
            break;
        }

        cursor = endComment + 2;
    }

    return result;
}

std::optional<Selector> parseSelector(std::string_view selectorText, std::vector<StylesheetParseWarning>& warnings) {
    const std::string selector = trimCopy(selectorText);
    if (selector.empty()) {
        return std::nullopt;
    }

    auto parseSimpleSelector = [&](std::string_view text) -> std::optional<SimpleSelector> {
        const std::string simple = trimCopy(text);
        if (simple.empty()) {
            return std::nullopt;
        }

        SimpleSelector parsed;
        std::size_t cursor = 0;

        if (simple == "*") {
            parsed.universal = true;
            return parsed;
        }

        if (simple[cursor] != '#' && simple[cursor] != '.' && simple[cursor] != '[' && simple[cursor] != ':') {
            const auto start = cursor;
            while (cursor < simple.size() && isNameChar(simple[cursor])) {
                ++cursor;
            }

            if (cursor == start) {
                return std::nullopt;
            }

            parsed.type = simple.substr(start, cursor - start);
        }

        while (cursor < simple.size()) {
            const char prefix = simple[cursor];
            if (prefix == '#' || prefix == '.') {
                ++cursor;
                const auto start = cursor;
                while (cursor < simple.size() && isNameChar(simple[cursor])) {
                    ++cursor;
                }

                if (cursor == start) {
                    return std::nullopt;
                }

                const auto value = simple.substr(start, cursor - start);
                if (prefix == '#') {
                    parsed.id = value;
                } else {
                    parsed.classes.push_back(value);
                }
                continue;
            }

            if (prefix == '[') {
                const auto close = simple.find(']', cursor + 1);
                if (close == std::string::npos) {
                    return std::nullopt;
                }

                const auto content = trimCopy(std::string_view(simple).substr(cursor + 1, close - cursor - 1));
                const auto equals = content.find('=');
                if (equals == std::string::npos) {
                    return std::nullopt;
                }

                const auto name = trimCopy(content.substr(0, equals));
                auto value = trimCopy(content.substr(equals + 1));
                if (value.size() >= 2 && value.front() == '"' && value.back() == '"') {
                    value = value.substr(1, value.size() - 2);
                }

                std::optional<WindowAttribute> attribute;
                if (name == "app-id") attribute = WindowAttribute::AppId;
                else if (name == "class") attribute = WindowAttribute::Class;
                else if (name == "title") attribute = WindowAttribute::Title;
                else if (name == "floating") attribute = WindowAttribute::Floating;
                else if (name == "fullscreen") attribute = WindowAttribute::Fullscreen;
                else return std::nullopt;

                parsed.attributes.push_back({*attribute, value});
                cursor = close + 1;
                continue;
            }

            if (prefix == ':') {
                ++cursor;
                const auto start = cursor;
                while (cursor < simple.size() && isNameChar(simple[cursor])) {
                    ++cursor;
                }

                if (cursor == start) {
                    return std::nullopt;
                }

                const auto name = simple.substr(start, cursor - start);
                if (name == "focused") parsed.pseudoclasses.push_back(PseudoClass::Focused);
                else if (name == "focused-within") parsed.pseudoclasses.push_back(PseudoClass::FocusedWithin);
                else if (name == "fullscreen") parsed.pseudoclasses.push_back(PseudoClass::Fullscreen);
                else if (name == "floating") parsed.pseudoclasses.push_back(PseudoClass::Floating);
                else if (name == "urgent") parsed.pseudoclasses.push_back(PseudoClass::Urgent);
                else if (name == "visible") parsed.pseudoclasses.push_back(PseudoClass::Visible);
                else if (name == "special-workspace") parsed.pseudoclasses.push_back(PseudoClass::SpecialWorkspace);
                else return std::nullopt;
                continue;
            }

            return std::nullopt;
        }

        return parsed;
    };

    Selector parsed;
    const auto child = selector.find('>');
    if (child != std::string::npos) {
        if (selector.find_first_of(" \t\n\r", child + 1) != std::string::npos || selector.find('>', child + 1) != std::string::npos) {
            warnings.push_back({"unsupported selector ignored: '" + selector + "'"});
            return std::nullopt;
        }

        const auto parent = parseSimpleSelector(selector.substr(0, child));
        const auto target = parseSimpleSelector(selector.substr(child + 1));
        if (!parent.has_value() || !target.has_value()) {
            warnings.push_back({"invalid selector ignored: '" + selector + "'"});
            return std::nullopt;
        }

        parsed.target = *target;
        parsed.directParent = *parent;
        return parsed;
    }

    const auto descendant = selector.find(' ');
    if (descendant != std::string::npos) {
        if (selector.find_first_not_of(' ', descendant) == std::string::npos) {
            warnings.push_back({"invalid selector ignored: '" + selector + "'"});
            return std::nullopt;
        }

        if (selector.find_first_of("\t\n\r", descendant) != std::string::npos) {
            warnings.push_back({"unsupported selector ignored: '" + selector + "'"});
            return std::nullopt;
        }

        const auto ancestor = parseSimpleSelector(selector.substr(0, descendant));
        const auto target = parseSimpleSelector(selector.substr(descendant + 1));
        if (!ancestor.has_value() || !target.has_value()) {
            warnings.push_back({"invalid selector ignored: '" + selector + "'"});
            return std::nullopt;
        }

        parsed.target = *target;
        parsed.ancestor = *ancestor;
        return parsed;
    }

    if (selector.find_first_of("\t\n\r+~") != std::string::npos) {
        warnings.push_back({"unsupported selector ignored: '" + selector + "'"});
        return std::nullopt;
    }

    const auto simple = parseSimpleSelector(selector);
    if (!simple.has_value()) {
        warnings.push_back({"invalid selector ignored: '" + selector + "'"});
        return std::nullopt;
    }

    parsed.target = *simple;
    return parsed;
}

std::vector<std::string> splitSelectorList(std::string_view text) {
    std::vector<std::string> selectors;
    std::size_t cursor = 0;

    while (cursor <= text.size()) {
        const auto comma = text.find(',', cursor);
        const auto length = comma == std::string_view::npos ? text.size() - cursor : comma - cursor;
        const auto selector = trimCopy(text.substr(cursor, length));
        if (!selector.empty()) {
            selectors.push_back(selector);
        }

        if (comma == std::string_view::npos) {
            break;
        }

        cursor = comma + 1;
    }

    return selectors;
}

std::optional<PropertyId> parsePropertyId(std::string_view property) {
    const auto name = trimCopy(property);
    if (name == "display") return PropertyId::Display;
    if (name == "position") return PropertyId::Position;
    if (name == "top") return PropertyId::Top;
    if (name == "right") return PropertyId::Right;
    if (name == "bottom") return PropertyId::Bottom;
    if (name == "left") return PropertyId::Left;
    if (name == "flex-direction") return PropertyId::FlexDirection;
    if (name == "flex-wrap") return PropertyId::FlexWrap;
    if (name == "flex-grow") return PropertyId::FlexGrow;
    if (name == "flex-shrink") return PropertyId::FlexShrink;
    if (name == "flex-basis") return PropertyId::FlexBasis;
    if (name == "gap") return PropertyId::Gap;
    if (name == "row-gap") return PropertyId::RowGap;
    if (name == "column-gap") return PropertyId::ColumnGap;
    if (name == "justify-content") return PropertyId::JustifyContent;
    if (name == "align-items") return PropertyId::AlignItems;
    if (name == "align-self") return PropertyId::AlignSelf;
    if (name == "align-content") return PropertyId::AlignContent;
    if (name == "overflow") return PropertyId::Overflow;
    if (name == "width") return PropertyId::Width;
    if (name == "height") return PropertyId::Height;
    if (name == "min-width") return PropertyId::MinWidth;
    if (name == "min-height") return PropertyId::MinHeight;
    if (name == "max-width") return PropertyId::MaxWidth;
    if (name == "max-height") return PropertyId::MaxHeight;
    if (name == "aspect-ratio") return PropertyId::AspectRatio;
    if (name == "box-sizing") return PropertyId::BoxSizing;
    if (name == "border-width") return PropertyId::BorderWidth;
    if (name == "border-top-width") return PropertyId::BorderTopWidth;
    if (name == "border-right-width") return PropertyId::BorderRightWidth;
    if (name == "border-bottom-width") return PropertyId::BorderBottomWidth;
    if (name == "border-left-width") return PropertyId::BorderLeftWidth;
    if (name == "margin") return PropertyId::Margin;
    if (name == "margin-top") return PropertyId::MarginTop;
    if (name == "margin-right") return PropertyId::MarginRight;
    if (name == "margin-bottom") return PropertyId::MarginBottom;
    if (name == "margin-left") return PropertyId::MarginLeft;
    if (name == "padding") return PropertyId::Padding;
    if (name == "padding-top") return PropertyId::PaddingTop;
    if (name == "padding-right") return PropertyId::PaddingRight;
    if (name == "padding-bottom") return PropertyId::PaddingBottom;
    if (name == "padding-left") return PropertyId::PaddingLeft;
    return std::nullopt;
}

std::vector<std::string> splitWhitespaceTokens(std::string_view text) {
    std::vector<std::string> tokens;
    std::size_t cursor = 0;

    while (cursor < text.size()) {
        while (cursor < text.size() && std::isspace(static_cast<unsigned char>(text[cursor])) != 0) {
            ++cursor;
        }

        const auto start = cursor;
        while (cursor < text.size() && std::isspace(static_cast<unsigned char>(text[cursor])) == 0) {
            ++cursor;
        }

        if (start < cursor) {
            tokens.emplace_back(text.substr(start, cursor - start));
        }
    }

    return tokens;
}

bool appendExpandedBoxShorthand(
    Rule& rule,
    PropertyId top,
    PropertyId right,
    PropertyId bottom,
    PropertyId left,
    const std::string& value,
    std::vector<StylesheetParseWarning>& warnings,
    const std::string& selectorText,
    const std::string& propertyName
) {
    const auto parts = splitWhitespaceTokens(value);
    if (parts.empty() || parts.size() > 4) {
        warnings.push_back({
            "unsupported declaration ignored in selector '" + selectorText + "': '" + propertyName + ": " + value + "'"
        });
        return false;
    }

    std::string topValue;
    std::string rightValue;
    std::string bottomValue;
    std::string leftValue;

    if (parts.size() == 1) {
        topValue = rightValue = bottomValue = leftValue = parts[0];
    } else if (parts.size() == 2) {
        topValue = bottomValue = parts[0];
        rightValue = leftValue = parts[1];
    } else if (parts.size() == 3) {
        topValue = parts[0];
        rightValue = leftValue = parts[1];
        bottomValue = parts[2];
    } else {
        topValue = parts[0];
        rightValue = parts[1];
        bottomValue = parts[2];
        leftValue = parts[3];
    }

    rule.declarations.push_back({top, topValue});
    rule.declarations.push_back({right, rightValue});
    rule.declarations.push_back({bottom, bottomValue});
    rule.declarations.push_back({left, leftValue});
    return true;
}

void parseDeclarations(
    std::string_view block,
    Rule& rule,
    std::vector<StylesheetParseWarning>& warnings,
    const std::string& selectorText
) {
    std::size_t cursor = 0;
    while (cursor < block.size()) {
        const auto semicolon = block.find(';', cursor);
        const auto length = semicolon == std::string_view::npos ? block.size() - cursor : semicolon - cursor;
        const auto declaration = block.substr(cursor, length);
        const auto colon = declaration.find(':');

        if (colon != std::string_view::npos) {
            const auto property = declaration.substr(0, colon);
            const auto propertyName = trimCopy(property);
            const auto value = trimCopy(declaration.substr(colon + 1));
            if (propertyName == "margin" && !value.empty()) {
                appendExpandedBoxShorthand(rule, PropertyId::MarginTop, PropertyId::MarginRight, PropertyId::MarginBottom, PropertyId::MarginLeft, value, warnings, selectorText, propertyName);
            } else if (propertyName == "padding" && !value.empty()) {
                appendExpandedBoxShorthand(rule, PropertyId::PaddingTop, PropertyId::PaddingRight, PropertyId::PaddingBottom, PropertyId::PaddingLeft, value, warnings, selectorText, propertyName);
            } else if (propertyName == "border-width" && !value.empty()) {
                appendExpandedBoxShorthand(rule, PropertyId::BorderTopWidth, PropertyId::BorderRightWidth, PropertyId::BorderBottomWidth, PropertyId::BorderLeftWidth, value, warnings, selectorText, propertyName);
            } else if (const auto propertyId = parsePropertyId(property); propertyId.has_value() && !value.empty()) {
                rule.declarations.push_back({*propertyId, value});
            } else if (!propertyName.empty()) {
                warnings.push_back({"unsupported declaration ignored in selector '" + selectorText + "': '" + trimCopy(declaration) + "'"});
            }
        }

        if (semicolon == std::string_view::npos) {
            break;
        }

        cursor = semicolon + 1;
    }
}

} // namespace

LibcssBridgeStatus libcssBridgeStatus() {
    return LibcssBridgeStatus {true, "libcss bridge available"};
}

Stylesheet parseStylesheet(const std::string& source) {
    return parseStylesheetDetailed(source).stylesheet;
}

StylesheetParseResult parseStylesheetDetailed(const std::string& source) {
    StylesheetParseResult result;
    const std::string cleanedSource = stripComments(source);
    const auto libcssCheck = libcssSyntaxAccepts(source);

    std::size_t cursor = 0;
    while (cursor < cleanedSource.size()) {
        const auto openBrace = cleanedSource.find('{', cursor);
        if (openBrace == std::string::npos) {
            break;
        }

        const auto closeBrace = cleanedSource.find('}', openBrace + 1);
        if (closeBrace == std::string::npos) {
            break;
        }

        const auto selectorText = trimCopy(std::string_view(cleanedSource).substr(cursor, openBrace - cursor));
        if (!selectorText.empty() && selectorText[0] == '@') {
            result.warnings.push_back({"unsupported at-rule ignored: '" + selectorText + "'"});
            cursor = closeBrace + 1;
            continue;
        }

        const auto selectorTexts = splitSelectorList(selectorText);
        for (const auto& selectorPart : selectorTexts) {
            const auto selector = parseSelector(selectorPart, result.warnings);
            if (selector.has_value()) {
                Rule rule;
                rule.selector = *selector;
                rule.specificity = selectorSpecificity(*selector);
                rule.sourceOrder = result.stylesheet.rules.size();
                parseDeclarations(std::string_view(cleanedSource).substr(openBrace + 1, closeBrace - openBrace - 1), rule, result.warnings, selectorPart);
                if (!rule.declarations.empty()) {
                    result.stylesheet.rules.push_back(std::move(rule));
                }
            }
        }

        cursor = closeBrace + 1;
    }

    if (!libcssCheck.accepted) {
        result.warnings.push_back({libcssCheck.diagnostic + "; fallback parser result may diverge from runtime libcss behavior"});
    }

    if (!libcssCheck.accepted && result.stylesheet.rules.empty() && result.warnings.size() == 1) {
        result.warnings.push_back({"libcss reported syntax issues; fallback parser produced no supported rules"});
    }

    return result;
}

} // namespace hypreact::style
