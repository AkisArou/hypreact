#include <iostream>
#include <string>

#include "domain/runtime_snapshot.hpp"
#include "style/libcss_probe.hpp"

int main() {
    const hypreact::style::StyleNodeContext simpleNode {
        .type = hypreact::domain::LayoutNodeType::Window,
        .id = "",
    };

    const hypreact::style::StyleNodeContext focusNode {
        .type = hypreact::domain::LayoutNodeType::Window,
        .id = "win-1",
        .focused = true,
    };

    const hypreact::style::StyleNodeContext classFocusNode {
        .type = hypreact::domain::LayoutNodeType::Window,
        .id = "win-1",
        .classes = {"main"},
        .focused = true,
    };

    const hypreact::style::StyleNodeContext groupNode {
        .type = hypreact::domain::LayoutNodeType::Group,
        .id = "group-1",
    };

    const hypreact::style::StyleNodeContext childCombinatorNode {
        .type = hypreact::domain::LayoutNodeType::Window,
        .id = "win-1",
        .classes = {"main"},
        .parent = &groupNode,
        .focused = true,
    };

    const hypreact::domain::WindowSnapshot crossCheckWindow {
        .id = "1",
        .klass = "Alacritty",
        .focused = true,
    };

    const hypreact::style::StyleNodeContext crossCheckNode {
        .type = hypreact::domain::LayoutNodeType::Window,
        .id = "win-1",
        .classes = {"main"},
        .parent = &groupNode,
        .window = &crossCheckWindow,
        .focused = true,
    };

    const auto run = [](const std::string& label, const std::string& css, const hypreact::style::StyleNodeContext& node) {
        std::cout << "case=" << label << '\n';
        const auto probe = hypreact::style::probeLibcssSelection(css, node);
        std::cout << "  parsed=" << (probe.parsed ? "true" : "false")
                  << " selected=" << (probe.selected ? "true" : "false")
                  << " authoredMatch=" << (probe.authoredMatch ? "true" : "false")
                  << '\n';
        for (const auto& diagnostic : probe.diagnostics) {
            std::cout << "  diagnostic: " << diagnostic << '\n';
        }
        for (const auto& warning : probe.warnings) {
            std::cout << "  warning: " << warning << '\n';
        }
    };

    run("name-only", "window { display: none; }", simpleNode);
    run("name-focus", "window:focus { display: none; }", focusNode);
    run("name-class-focus", "window.main:focus { display: none; }", classFocusNode);
    run("group-child-window-focus", "group > window.main:focus { display: none; }", childCombinatorNode);
    run("cross-check-display-fixture", "group>window.main:focus { display: none; }", crossCheckNode);

    return 0;
}
