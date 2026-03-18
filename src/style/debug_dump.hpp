#pragma once

#include <string>

#include "libcss_bridge.hpp"
#include "libcss_selector_adapter.hpp"
#include "stylesheet.hpp"

namespace hypreact::style {

std::string formatSelectorDebugDumpJson(
    const LibcssSelectorMatchResult& selectorMatch,
    const LibcssSelectionProbe& probe,
    const ComputedStyle& fallbackStyle,
    std::size_t ruleCount
);

} // namespace hypreact::style
