#include <iostream>

#include "style/debug_dump.hpp"

int main() {
    hypreact::style::LibcssSelectorMatchResult match;
    match.matched = true;
    match.diagnostics.push_back("selector matched with fallback warning");

    hypreact::style::LibcssSelectionProbe probe;
    probe.parsed = true;
    probe.selected = true;
    probe.position = "absolute";
    probe.width = 24.0f;
    probe.height = 18.0f;
    probe.top = 3.0f;
    probe.right = 11.0f;
    probe.bottom = 7.0f;
    probe.diagnostics.push_back("libcss selected subset only");
    probe.warnings.push_back("probe warning detail");
    probe.trace.push_back("probe_select_start");
    probe.trace.push_back("probe_select_success");

    hypreact::style::ComputedStyle fallback;
    fallback.position = "absolute";
    fallback.width = hypreact::style::LengthValue {.unit = hypreact::style::LengthUnit::Points, .value = 24.0f};

    std::cout << hypreact::style::formatSelectorDebugDumpJson(match, probe, fallback, 1) << '\n';
    return 0;
}
