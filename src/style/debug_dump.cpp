#include "debug_dump.hpp"

namespace hypreact::style {

namespace {

std::string jsonEscape(const std::string& value) {
    std::string escaped;
    escaped.reserve(value.size());
    for (const char ch : value) {
        switch (ch) {
            case '\\': escaped += "\\\\"; break;
            case '"': escaped += "\\\""; break;
            case '\n': escaped += "\\n"; break;
            case '\r': escaped += "\\r"; break;
            case '\t': escaped += "\\t"; break;
            default: escaped += ch; break;
        }
    }
    return escaped;
}

std::string optionalString(const std::optional<std::string>& value) {
    return value.has_value() ? "\"" + jsonEscape(*value) + "\"" : "null";
}

std::string optionalFloat(const std::optional<float>& value) {
    return value.has_value() ? std::to_string(*value) : "null";
}

std::string optionalLength(const std::optional<LengthValue>& value) {
    if (!value.has_value()) {
        return "null";
    }
    const char* unit = value->unit == LengthUnit::Points ? "points" : value->unit == LengthUnit::Percent ? "percent" : "auto";
    return std::string("{\"unit\":\"") + unit + "\",\"value\":" + std::to_string(value->value) + "}";
}

void appendStringArray(std::string& json, const char* key, const std::vector<std::string>& values) {
    json += "\"";
    json += key;
    json += "\":[";
    for (std::size_t index = 0; index < values.size(); ++index) {
        if (index > 0) json += ",";
        json += "\"" + jsonEscape(values[index]) + "\"";
    }
    json += "]";
}

} // namespace

std::string formatSelectorDebugDumpJson(
    const LibcssSelectorMatchResult& selectorMatch,
    const LibcssSelectionProbe& probe,
    const ComputedStyle& fallbackStyle,
    std::size_t ruleCount
) {
    std::string json = "{";
    json += "\"matched\":" + std::string(selectorMatch.matched ? "true" : "false");
    json += ",\"probeParsed\":" + std::string(probe.parsed ? "true" : "false");
    json += ",\"probeSelected\":" + std::string(probe.selected ? "true" : "false");
    json += ",\"probeAuthoredMatch\":" + std::string(probe.authoredMatch ? "true" : "false");
    json += ",\"display\":" + optionalString(probe.display);
    json += ",\"position\":" + optionalString(probe.position);
    json += ",\"width\":" + optionalFloat(probe.width);
    json += ",\"height\":" + optionalFloat(probe.height);
    json += ",\"minWidth\":" + optionalFloat(probe.minWidth);
    json += ",\"maxWidth\":" + optionalFloat(probe.maxWidth);
    json += ",\"minHeight\":" + optionalFloat(probe.minHeight);
    json += ",\"maxHeight\":" + optionalFloat(probe.maxHeight);
    json += ",\"marginLeft\":" + optionalFloat(probe.marginLeft);
    json += ",\"paddingRight\":" + optionalFloat(probe.paddingRight);
    json += ",\"left\":" + optionalFloat(probe.left);
    json += ",\"top\":" + optionalFloat(probe.top);
    json += ",\"right\":" + optionalFloat(probe.right);
    json += ",\"bottom\":" + optionalFloat(probe.bottom);
    json += ",\"fallback\":{" + std::string("\"ruleCount\":") + std::to_string(ruleCount)
            + ",\"display\":" + optionalString(fallbackStyle.display)
            + ",\"position\":" + optionalString(fallbackStyle.position)
            + ",\"width\":" + optionalLength(fallbackStyle.width)
            + ",\"height\":" + optionalLength(fallbackStyle.height)
            + ",\"maxWidth\":" + optionalLength(fallbackStyle.maxWidth)
            + ",\"maxHeight\":" + optionalLength(fallbackStyle.maxHeight)
            + ",\"left\":" + optionalLength(fallbackStyle.left)
            + ",\"top\":" + optionalLength(fallbackStyle.top)
            + ",\"right\":" + optionalLength(fallbackStyle.right)
            + ",\"bottom\":" + optionalLength(fallbackStyle.bottom)
            + "}";
    json += ",";
    appendStringArray(json, "trace", selectorMatch.trace);
    json += ",";
    appendStringArray(json, "probeTrace", probe.trace);
    json += ",";
    appendStringArray(json, "diagnostics", selectorMatch.diagnostics);
    json += ",";
    appendStringArray(json, "probeDiagnostics", probe.diagnostics);
    json += ",";
    appendStringArray(json, "probeWarnings", probe.warnings);
    json += "}";
    return json;
}

} // namespace hypreact::style
