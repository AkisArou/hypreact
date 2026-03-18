#pragma once

#include <optional>
#include <string_view>
#include <vector>

#if defined(__cplusplus) && !defined(restrict)
#define restrict __restrict
#endif

#include <libcss/select.h>

#include "stylesheet.hpp"

namespace hypreact::style {

struct LibcssAdapterNode {
    const StyleNodeContext* context = nullptr;
    lwc_string* typeName = nullptr;
};

struct LibcssSelectorMatchResult {
    bool parsed = false;
    bool matched = false;
    std::vector<std::string> diagnostics;
    std::vector<std::string> trace;
};

std::vector<LibcssAdapterNode> buildLibcssAdapterChain(const StyleNodeContext& node);
css_select_handler makeLibcssSelectHandler();
std::optional<lwc_string*> internLibcssString(std::string_view text);
bool libcssSelectorMatches(const std::string& selector, const StyleNodeContext& node);
LibcssSelectorMatchResult libcssMatchSelector(const std::string& selector, const StyleNodeContext& node);

} // namespace hypreact::style
