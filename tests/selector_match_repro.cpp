#include <iostream>
#include <string>
#include <vector>

#include "domain/runtime_snapshot.hpp"
#include "style/libcss_bridge.hpp"
#include "style/libcss_selector_adapter.hpp"

int main() {
    const auto warmed = hypreact::style::parseStylesheetDetailed("window.main:focused { width: 10px; }");
    std::cout << "warmup rules=" << warmed.stylesheet.rules.size() << " warnings=" << warmed.warnings.size() << '\n';

    const hypreact::domain::WindowSnapshot window {
        .id = "1",
        .focused = true,
    };
    const hypreact::style::StyleNodeContext workspace {
        .type = hypreact::domain::LayoutNodeType::Workspace,
        .id = "ws-1",
    };
    const hypreact::style::StyleNodeContext group {
        .type = hypreact::domain::LayoutNodeType::Group,
        .id = "group-1",
        .parent = &workspace,
    };
    const hypreact::style::StyleNodeContext node {
        .type = hypreact::domain::LayoutNodeType::Window,
        .id = "win-1",
        .classes = {"main"},
        .parent = &group,
        .window = &window,
        .focused = true,
    };

    const std::vector<std::string> selectors {
        "window.main:focus",
        "group > window.main:focus",
        "workspace window.main:focus",
    };

    for (const auto& selector : selectors) {
        const auto result = hypreact::style::libcssMatchSelector(selector, node);
        std::cout << "selector=" << selector
                  << " parsed=" << (result.parsed ? "true" : "false")
                  << " matched=" << (result.matched ? "true" : "false")
                  << '\n';
        for (const auto& diagnostic : result.diagnostics) {
            std::cout << "  diagnostic: " << diagnostic << '\n';
        }
        for (const auto& trace : result.trace) {
            std::cout << "  trace: " << trace << '\n';
        }

        const auto probe = hypreact::style::probeLibcssSelection(selector + " { max-width: 37px; }", node);
        std::cout << "  probe parsed=" << (probe.parsed ? "true" : "false")
                  << " selected=" << (probe.selected ? "true" : "false")
                  << " authoredMatch=" << (probe.authoredMatch ? "true" : "false")
                  << '\n';
        for (const auto& diagnostic : probe.diagnostics) {
            std::cout << "    probe diagnostic: " << diagnostic << '\n';
        }
        for (const auto& warning : probe.warnings) {
            std::cout << "    probe warning: " << warning << '\n';
        }
    }

    return 0;
}
