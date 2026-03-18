#pragma once

#include <string>
#include <vector>

#include "stylesheet.hpp"

namespace hypreact::style {

struct LibcssBridgeStatus {
    bool available;
    std::string detail;
};

struct LibcssSelectionProbe {
    bool parsed = false;
    bool selected = false;
    bool authoredMatch = false;
    std::vector<std::string> diagnostics;
    std::optional<std::string> display;
    std::optional<std::string> position;
    std::optional<std::string> overflow;
    std::optional<std::string> boxSizing;
    std::optional<float> width;
    std::optional<float> height;
    std::optional<float> minWidth;
    std::optional<float> maxHeight;
    std::optional<float> maxWidth;
    std::optional<float> minHeight;
    std::optional<float> marginLeft;
    std::optional<float> paddingRight;
    std::optional<float> left;
    std::optional<float> top;
    std::optional<float> right;
    std::optional<float> bottom;
    std::vector<std::string> trace;
    std::vector<std::string> warnings;
};

struct StylesheetParseWarning {
    std::string message;
};

struct StylesheetParseResult {
    Stylesheet stylesheet;
    std::vector<StylesheetParseWarning> warnings;
};

LibcssBridgeStatus libcssBridgeStatus();
LibcssSelectionProbe probeLibcssSelection(const std::string& source, const StyleNodeContext& node);
StylesheetParseResult parseStylesheetDetailed(const std::string& source);
Stylesheet parseStylesheet(const std::string& source);

} // namespace hypreact::style
