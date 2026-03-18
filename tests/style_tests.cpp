#include <cmath>
#include <cstdlib>
#include <iostream>
#include <optional>
#include <string>

#include "domain/layout_node.hpp"
#include "domain/runtime_snapshot.hpp"
#include "layout/resolver.hpp"
#include "style/libcss_bridge.hpp"
#include "style/debug_dump.hpp"
#include "style/libcss_selector_adapter.hpp"
#include "style/stylesheet.hpp"
#include "style_test_utils.hpp"

namespace {

using hypreact::domain::LayoutNode;
using hypreact::domain::LayoutNodeType;
using hypreact::style::LengthUnit;

void expect(bool condition, const std::string& message) {
    if (!condition) {
        std::cerr << "FAIL: " << message << '\n';
        std::exit(1);
    }
}

bool hasOnlyLibcssAvailabilityWarnings(const std::vector<hypreact::style::StylesheetParseWarning>& warnings) {
    if (warnings.empty()) {
        return true;
    }

    for (const auto& warning : warnings) {
        if (warning.message.find("libcss ") != 0) {
            return false;
        }
    }

    return true;
}

void expectNear(float actual, float expected, const std::string& message) {
    if (std::fabs(actual - expected) > 0.001f) {
        std::cerr << "FAIL: " << message << " (expected " << expected << ", got " << actual << ")\n";
        std::exit(1);
    }
}

void expectLength(
    const std::optional<hypreact::style::LengthValue>& value,
    LengthUnit unit,
    float number,
    const std::string& message
) {
    expect(value.has_value(), message + " should exist");
    expect(value->unit == unit, message + " should have expected unit");
    expectNear(value->value, number, message + " should have expected value");
}

void testParseLengthValue() {
    const auto points = hypreact::style::parseLengthValue("12px");
    expectLength(points, LengthUnit::Points, 12.0f, "12px");

    const auto percent = hypreact::style::parseLengthValue("40%");
    expectLength(percent, LengthUnit::Percent, 40.0f, "40%");

    const auto autoValue = hypreact::style::parseLengthValue("auto");
    expect(autoValue.has_value(), "auto should parse");
    expect(autoValue->unit == LengthUnit::Auto, "auto should use auto unit");
}

void testParseStylesheetDetailed() {
    const auto parsed = hypreact::style::parseStylesheetDetailed(R"CSS(
      /* comment */
      workspace.main#root {
        width: 75%;
        height: auto;
        margin-left: 12px;
        padding: 5%;
        flex-basis: auto;
      }

      group > window {
        gap: 8px;
      }
    )CSS");

    expect(parsed.stylesheet.rules.size() == 1, "only supported selector rule should be kept");
    expect(parsed.warnings.size() >= 1, "unsupported selector should produce at least one warning");

    const auto& rule = parsed.stylesheet.rules.front();
    expect(rule.selector.target.type.has_value() && *rule.selector.target.type == "workspace", "type selector parsed");
    expect(rule.selector.target.id.has_value() && *rule.selector.target.id == "root", "id selector parsed");
    expect(rule.selector.target.classes.size() == 1 && rule.selector.target.classes.front() == "main", "class selector parsed");
    expect(rule.declarations.size() == 8, "supported declarations should be preserved after shorthand expansion");
}

void testComputeStyle() {
    const auto parsed = hypreact::style::parseStylesheetDetailed(R"CSS(
      workspace { width: 10px; }
      .main { width: 25%; justify-content: center; }
      #root { width: auto; margin: auto; padding-right: 8px; flex-basis: 50%; }
    )CSS");

    LayoutNode node {
        .type = LayoutNodeType::Workspace,
        .id = "root",
        .classes = {"main"},
    };

    const auto computed = hypreact::style::computeStyle(parsed.stylesheet, node);
    expect(computed.justifyContent.has_value() && *computed.justifyContent == "center", "justify-content should cascade");
    expectLength(computed.width, LengthUnit::Auto, 0.0f, "width auto override");
    expectLength(computed.marginTop, LengthUnit::Auto, 0.0f, "margin-top auto");
    expectLength(computed.marginRight, LengthUnit::Auto, 0.0f, "margin-right auto");
    expectLength(computed.marginBottom, LengthUnit::Auto, 0.0f, "margin-bottom auto");
    expectLength(computed.marginLeft, LengthUnit::Auto, 0.0f, "margin-left auto");
    expectLength(computed.paddingRight, LengthUnit::Points, 8.0f, "padding-right points");
    expectLength(computed.flexBasis, LengthUnit::Percent, 50.0f, "flex-basis percent");
}

void testSpecificityAndExtendedProperties() {
    const auto parsed = hypreact::style::parseStylesheetDetailed(R"CSS(
      workspace { position: relative; min-width: 10px; top: 1px; }
      .main { min-width: 25%; max-height: 90%; }
      #root { min-width: 40px; top: auto; right: 5%; position: absolute; }
      workspace.main { min-width: 60px; left: 12px; }
    )CSS");

    LayoutNode node {
        .type = LayoutNodeType::Workspace,
        .id = "root",
        .classes = {"main"},
    };

    const auto computed = hypreact::style::computeStyle(parsed.stylesheet, node);
    expect(computed.position.has_value() && *computed.position == "absolute", "id selector should win for position");
    expectLength(computed.minWidth, LengthUnit::Points, 40.0f, "id selector should win for min-width");
    expectLength(computed.maxHeight, LengthUnit::Percent, 90.0f, "class selector max-height");
    expectLength(computed.top, LengthUnit::Auto, 0.0f, "top auto override");
    expectLength(computed.right, LengthUnit::Percent, 5.0f, "right percent");
    expectLength(computed.left, LengthUnit::Points, 12.0f, "compound selector left");
}

void testShorthandAndAdditionalFlexProperties() {
    const auto parsed = hypreact::style::parseStylesheetDetailed(R"CSS(
      workspace.main {
        margin: 1px 2px 3px 4px;
        padding: 5% 6%;
        flex-wrap: wrap-reverse;
        flex-shrink: 2;
        align-self: flex-end;
        align-content: space-around;
        overflow: hidden;
      }
    )CSS");

    LayoutNode node {
        .type = LayoutNodeType::Workspace,
        .id = "root",
        .classes = {"main"},
    };

    const auto computed = hypreact::style::computeStyle(parsed.stylesheet, node);
    expect(hasOnlyLibcssAvailabilityWarnings(parsed.warnings), "supported shorthand should only emit bridge availability warnings");
    expectLength(computed.marginTop, LengthUnit::Points, 1.0f, "margin-top from 4-value shorthand");
    expectLength(computed.marginRight, LengthUnit::Points, 2.0f, "margin-right from 4-value shorthand");
    expectLength(computed.marginBottom, LengthUnit::Points, 3.0f, "margin-bottom from 4-value shorthand");
    expectLength(computed.marginLeft, LengthUnit::Points, 4.0f, "margin-left from 4-value shorthand");
    expectLength(computed.paddingTop, LengthUnit::Percent, 5.0f, "padding-top from 2-value shorthand");
    expectLength(computed.paddingRight, LengthUnit::Percent, 6.0f, "padding-right from 2-value shorthand");
    expectLength(computed.paddingBottom, LengthUnit::Percent, 5.0f, "padding-bottom from 2-value shorthand");
    expectLength(computed.paddingLeft, LengthUnit::Percent, 6.0f, "padding-left from 2-value shorthand");
    expect(computed.flexWrap.has_value() && *computed.flexWrap == "wrap-reverse", "flex-wrap parsed");
    expect(computed.flexShrink.has_value(), "flex-shrink parsed");
    expectNear(*computed.flexShrink, 2.0f, "flex-shrink value");
    expect(computed.alignSelf.has_value() && *computed.alignSelf == "flex-end", "align-self parsed");
    expect(computed.alignContent.has_value() && *computed.alignContent == "space-around", "align-content parsed");
    expect(computed.overflow.has_value() && *computed.overflow == "hidden", "overflow parsed");
}

void testSelectorListsAndBorderProperties() {
    const auto parsed = hypreact::style::parseStylesheetDetailed(R"CSS(
      workspace, .main {
        border-width: 1px 2px 3px 4px;
        box-sizing: border-box;
        aspect-ratio: 1.5;
      }

      #root {
        border-left-width: 9px;
      }
    )CSS");

    LayoutNode node {
        .type = LayoutNodeType::Workspace,
        .id = "root",
        .classes = {"main"},
    };

    expect(parsed.stylesheet.rules.size() == 3, "selector list should create one rule per selector");

    const auto computed = hypreact::style::computeStyle(parsed.stylesheet, node);
    expect(computed.boxSizing.has_value() && *computed.boxSizing == "border-box", "box-sizing parsed");
    expect(computed.aspectRatio.has_value(), "aspect-ratio parsed");
    expectNear(*computed.aspectRatio, 1.5f, "aspect-ratio value");
    expect(computed.borderTopWidth.has_value(), "border-top-width parsed");
    expectNear(*computed.borderTopWidth, 1.0f, "border-top-width value");
    expect(computed.borderRightWidth.has_value(), "border-right-width parsed");
    expectNear(*computed.borderRightWidth, 2.0f, "border-right-width value");
    expect(computed.borderBottomWidth.has_value(), "border-bottom-width parsed");
    expectNear(*computed.borderBottomWidth, 3.0f, "border-bottom-width value");
    expect(computed.borderLeftWidth.has_value(), "border-left-width parsed");
    expectNear(*computed.borderLeftWidth, 9.0f, "id override border-left-width");
}

void testChildCombinatorAndWindowAttributes() {
    const auto parsed = hypreact::style::parseStylesheetDetailed(R"CSS(
      group>window[class="Alacritty"][floating="false"] {
        width: 66%;
      }

      workspace > window {
        width: 10px;
      }
    )CSS");

    const hypreact::domain::WindowSnapshot window {
        .id = "1",
        .klass = "Alacritty",
        .title = "term",
        .initialClass = "Alacritty",
        .floating = false,
    };

    const hypreact::style::StyleNodeContext parent {
        .type = LayoutNodeType::Group,
        .id = "group-1",
    };
    const hypreact::style::StyleNodeContext node {
        .type = LayoutNodeType::Window,
        .id = "win-1",
        .parent = &parent,
        .window = &window,
    };

    const auto computed = hypreact::style::computeStyle(parsed.stylesheet, node);
    expectLength(computed.width, LengthUnit::Percent, 66.0f, "child combinator and attributes should match");
}

void testDescendantCombinator() {
    const auto parsed = hypreact::style::parseStylesheetDetailed(R"CSS(
      workspace window[title="term"] {
        height: 33%;
      }
    )CSS");

    const hypreact::domain::WindowSnapshot window {
        .id = "1",
        .klass = "Alacritty",
        .title = "term",
    };
    const hypreact::style::StyleNodeContext grandparent {
        .type = LayoutNodeType::Workspace,
        .id = "ws-1",
    };
    const hypreact::style::StyleNodeContext parent {
        .type = LayoutNodeType::Group,
        .id = "group-1",
        .parent = &grandparent,
    };
    const hypreact::style::StyleNodeContext node {
        .type = LayoutNodeType::Window,
        .id = "win-1",
        .parent = &parent,
        .window = &window,
    };

    const auto computed = hypreact::style::computeStyle(parsed.stylesheet, node);
    expectLength(computed.height, LengthUnit::Percent, 33.0f, "descendant combinator should match ancestors");
}

void testLibcssFallbackWarning() {
    const auto parsed = hypreact::style::parseStylesheetDetailed("workspace { width: ; }");
    expect(!parsed.warnings.empty(), "libcss bridge status warning should be recorded");
}

void testPseudoclasses() {
    const auto parsed = hypreact::style::parseStylesheetDetailed(R"CSS(
      window:focused { width: 70%; }
      window:fullscreen { height: 80%; }
      window:floating { min-width: 11px; }
      window:urgent { min-height: 12px; }
      workspace:special-workspace { max-width: 13px; }
      workspace:focused-within { max-height: 14px; }
    )CSS");

    const hypreact::domain::WindowSnapshot window {
        .id = "1",
        .fullscreen = true,
        .focused = true,
    };
    const hypreact::style::StyleNodeContext node {
        .type = LayoutNodeType::Window,
        .id = "win-1",
        .window = &window,
        .focused = true,
        .focusedWithin = true,
        .fullscreen = true,
        .floating = true,
        .urgent = true,
    };

    const hypreact::style::StyleNodeContext workspace {
        .type = LayoutNodeType::Workspace,
        .id = "ws-1",
        .focusedWithin = true,
        .specialWorkspace = true,
    };

    const auto computed = hypreact::style::computeStyle(parsed.stylesheet, node);
    const auto workspaceComputed = hypreact::style::computeStyle(parsed.stylesheet, workspace);
    expectLength(computed.width, LengthUnit::Percent, 70.0f, "focused pseudoclass should match");
    expectLength(computed.height, LengthUnit::Percent, 80.0f, "fullscreen pseudoclass should match");
    expectLength(computed.minWidth, LengthUnit::Points, 11.0f, "floating pseudoclass should match");
    expectLength(computed.minHeight, LengthUnit::Points, 12.0f, "urgent pseudoclass should match");
    expectLength(workspaceComputed.maxWidth, LengthUnit::Points, 13.0f, "special workspace pseudoclass should match");
    expectLength(workspaceComputed.maxHeight, LengthUnit::Points, 14.0f, "focused-within pseudoclass should match");
}

void testResolvedTreeSelectors() {
    hypreact::domain::RuntimeSnapshot snapshot;
    snapshot.workspace.name = "1";
    snapshot.workspace.special = true;

    snapshot.windows.push_back({
        .id = "win-1",
        .klass = "Alacritty",
        .title = "term",
        .workspace = "1",
        .floating = true,
        .fullscreen = false,
        .focused = true,
        .urgent = true,
    });

    hypreact::domain::LayoutNode root {
        .type = LayoutNodeType::Workspace,
        .id = "root",
        .children = {
            hypreact::domain::LayoutNode {
                .type = LayoutNodeType::Group,
                .id = "group",
                .children = {
                    hypreact::domain::LayoutNode {
                        .type = LayoutNodeType::Window,
                        .id = "window",
                        .children = {},
                    },
                },
            },
        },
    };

    const auto resolved = hypreact::layout::LayoutResolver::resolve(root, snapshot);
    expect(resolved.root.focusedWithin, "resolved root should track focused-within");
    expect(resolved.root.specialWorkspace, "resolved root should track special workspace");
    expect(!resolved.root.children.empty(), "resolved tree should have group child");
    expect(resolved.root.children.front().focusedWithin, "resolved group should track focused-within");
    expect(!resolved.root.children.front().children.empty(), "resolved group should have window child");
    expect(resolved.root.children.front().children.front().primaryWindow != nullptr, "resolved window should claim a window");

    std::vector<hypreact::style::StyleNodeContext> storage;
    const auto context = hypreact::test::styleContextFromResolvedPath(resolved.root, {0, 0}, storage);
    expect(context.focused, "resolved style context should carry focused state");
    expect(context.floating, "resolved style context should carry floating state");
    expect(context.urgent, "resolved style context should carry urgent state");
    expect(context.visible, "resolved style context should carry visible state");
}

void testLibcssSelectionProbe() {
    const hypreact::style::StyleNodeContext node {
        .type = LayoutNodeType::Window,
        .id = "win-1",
        .classes = {"main"},
        .focused = true,
    };

    const auto probe = hypreact::style::probeLibcssSelection(
        "window.main:focus { display: none; }",
        node
    );

    expect(probe.parsed, "libcss probe should parse supported focus selector rule");
    expect(probe.selected, "libcss probe should select supported focus selector rule");
    expect(probe.display.has_value() && *probe.display == "none", "libcss probe should resolve display");
    expect(probe.authoredMatch, "libcss probe should report authored match for display rule");
}

void testLibcssCrossCheck() {
    const hypreact::domain::WindowSnapshot window {
        .id = "1",
        .klass = "Alacritty",
        .focused = true,
    };
    const hypreact::style::StyleNodeContext parent {
        .type = LayoutNodeType::Group,
        .id = "group-1",
    };
    const hypreact::style::StyleNodeContext node {
        .type = LayoutNodeType::Window,
        .id = "win-1",
        .classes = {"main"},
        .parent = &parent,
        .window = &window,
        .focused = true,
    };

    const std::string fallbackStylesheetSource = hypreact::test::readFixture("libcss-display.css");
    const std::string probeStylesheetSource = hypreact::test::readFixture("libcss-display-focus.css");

    const auto parsed = hypreact::style::parseStylesheetDetailed(fallbackStylesheetSource);
    const auto computed = hypreact::style::computeStyle(parsed.stylesheet, node);
    const auto probe = hypreact::style::probeLibcssSelection(probeStylesheetSource, node);

    expect(computed.display.has_value() && *computed.display == "none", "fallback parser should match cross-check rule");
    expect(probe.parsed, "libcss cross-check should parse supported display fixture");
    expect(probe.selected, "libcss cross-check should select supported display fixture");
    expect(probe.authoredMatch, "libcss cross-check should report authored match for display fixture");
    if (probe.display.has_value()) {
        expect(*probe.display == "none", "libcss cross-check should agree on display when display is exposed in the overlap subset");
    }
}

void testLibcssCrossCheckWithAttribute() {
    const hypreact::domain::WindowSnapshot window {
        .id = "1",
        .klass = "Alacritty",
        .floating = true,
        .fullscreen = false,
    };
    const hypreact::style::StyleNodeContext parent {
        .type = LayoutNodeType::Group,
        .id = "group-1",
    };
    const hypreact::style::StyleNodeContext node {
        .type = LayoutNodeType::Window,
        .id = "win-1",
        .parent = &parent,
        .window = &window,
    };

    const std::string stylesheetSource = hypreact::test::readFixture("libcss-attribute.css");

    const auto parsed = hypreact::style::parseStylesheetDetailed(stylesheetSource);
    const auto computed = hypreact::style::computeStyle(parsed.stylesheet, node);
    const auto probe = hypreact::style::probeLibcssSelection(stylesheetSource, node);

    expect(computed.display.has_value() && *computed.display == "none", "fallback parser should match attribute cross-check rule");
    expect(probe.selected || !probe.warnings.empty(), "libcss attribute cross-check should select or explain failure");
}

void testLibcssCrossCheckPositionFixture() {
    const hypreact::style::StyleNodeContext node {
        .type = LayoutNodeType::Window,
        .id = "win-1",
        .classes = {"main"},
    };

    const std::string stylesheetSource = hypreact::test::readFixture("libcss-position.css");
    const auto parsed = hypreact::style::parseStylesheetDetailed(stylesheetSource);
    const auto computed = hypreact::style::computeStyle(parsed.stylesheet, node);
    const auto probe = hypreact::style::probeLibcssSelection(stylesheetSource, node);

    expect(computed.position.has_value() && *computed.position == "absolute", "fallback parser should match fixture position");
    expectLength(computed.width, LengthUnit::Points, 24.0f, "fallback parser should match fixture width");
    expectLength(computed.minWidth, LengthUnit::Points, 12.0f, "fallback parser should match fixture min-width");
    expect(probe.selected || !probe.warnings.empty(), "libcss position fixture should select or explain failure");
    if (probe.selected) {
        if (probe.position.has_value()) {
            expect(*probe.position == "absolute", "libcss fixture should agree on position");
        }
        if (probe.width.has_value()) {
            expectNear(*probe.width, 24.0f, "libcss fixture should agree on width");
        }
        if (probe.minWidth.has_value()) {
            expectNear(*probe.minWidth, 12.0f, "libcss fixture should agree on min-width");
        }
    }
}

void testLibcssCrossCheckDescendantFixture() {
    const hypreact::domain::WindowSnapshot window {
        .id = "1",
        .focused = true,
    };
    const hypreact::style::StyleNodeContext workspace {
        .type = LayoutNodeType::Workspace,
        .id = "ws-1",
    };
    const hypreact::style::StyleNodeContext group {
        .type = LayoutNodeType::Group,
        .id = "group-1",
        .parent = &workspace,
    };
    const hypreact::style::StyleNodeContext node {
        .type = LayoutNodeType::Window,
        .id = "win-1",
        .classes = {"main"},
        .parent = &group,
        .window = &window,
        .focused = true,
    };

    const std::string fallbackStylesheetSource = hypreact::test::readFixture("libcss-descendant.css");
    const std::string probeStylesheetSource = hypreact::test::readFixture("libcss-descendant-focus.css");
    const auto parsed = hypreact::style::parseStylesheetDetailed(fallbackStylesheetSource);
    const auto computed = hypreact::style::computeStyle(parsed.stylesheet, node);
    const auto probe = hypreact::style::probeLibcssSelection(probeStylesheetSource, node);

    expect(computed.display.has_value() && *computed.display == "none", "fallback parser should match descendant fixture");
    expect(probe.selected || !probe.warnings.empty(), "libcss descendant fixture should select or explain failure");
}

void testLibcssCrossCheckPseudostateFixture() {
    const hypreact::domain::WindowSnapshot window {
        .id = "1",
        .focused = true,
    };
    const hypreact::style::StyleNodeContext node {
        .type = LayoutNodeType::Window,
        .id = "win-1",
        .classes = {"main"},
        .window = &window,
        .focused = true,
    };

    const std::string fallbackStylesheetSource = hypreact::test::readFixture("libcss-pseudostate.css");
    const std::string probeStylesheetSource = hypreact::test::readFixture("libcss-pseudostate-focus.css");
    const auto parsed = hypreact::style::parseStylesheetDetailed(fallbackStylesheetSource);
    const auto computed = hypreact::style::computeStyle(parsed.stylesheet, node);
    const auto probe = hypreact::style::probeLibcssSelection(probeStylesheetSource, node);

    expect(computed.position.has_value() && *computed.position == "absolute", "fallback parser should match pseudostate position");
    expectLength(computed.width, LengthUnit::Points, 18.0f, "fallback parser should match pseudostate width");
    expectLength(computed.minWidth, LengthUnit::Points, 9.0f, "fallback parser should match pseudostate min-width");
    expect(computed.overflow.has_value() && *computed.overflow == "hidden", "fallback parser should match pseudostate overflow");
    expect(computed.boxSizing.has_value() && *computed.boxSizing == "border-box", "fallback parser should match pseudostate box-sizing");
    expectLength(computed.marginLeft, LengthUnit::Points, 7.0f, "fallback parser should match pseudostate margin-left");
    expectLength(computed.paddingRight, LengthUnit::Points, 3.0f, "fallback parser should match pseudostate padding-right");
    expect(probe.selected || !probe.warnings.empty(), "libcss pseudostate fixture should select or explain failure");
    if (probe.selected) {
        if (probe.position.has_value()) expect(*probe.position == "absolute", "libcss pseudostate should agree on position");
        if (probe.width.has_value()) expectNear(*probe.width, 18.0f, "libcss pseudostate should agree on width");
        if (probe.minWidth.has_value()) expectNear(*probe.minWidth, 9.0f, "libcss pseudostate should agree on min-width");
        if (probe.overflow.has_value()) expect(*probe.overflow == "hidden", "libcss pseudostate should agree on overflow");
        if (probe.boxSizing.has_value()) expect(*probe.boxSizing == "border-box", "libcss pseudostate should agree on box-sizing");
        if (probe.marginLeft.has_value()) expectNear(*probe.marginLeft, 7.0f, "libcss pseudostate should agree on margin-left");
        if (probe.paddingRight.has_value()) expectNear(*probe.paddingRight, 3.0f, "libcss pseudostate should agree on padding-right");
    }
}

void testLibcssCrossCheckPercentFixture() {
    const hypreact::domain::WindowSnapshot window {
        .id = "1",
        .focused = true,
    };
    const hypreact::style::StyleNodeContext node {
        .type = LayoutNodeType::Window,
        .id = "win-1",
        .classes = {"main"},
        .window = &window,
        .focused = true,
        .visible = true,
    };

    const std::string stylesheetSource = hypreact::test::readFixture("libcss-percent.css");
    const auto parsed = hypreact::style::parseStylesheetDetailed(stylesheetSource);
    const auto computed = hypreact::style::computeStyle(parsed.stylesheet, node);
    const auto probe = hypreact::style::probeLibcssSelection(stylesheetSource, node);

    expectLength(computed.width, LengthUnit::Percent, 50.0f, "fallback parser should match percent width");
    expectLength(computed.marginLeft, LengthUnit::Percent, 10.0f, "fallback parser should match percent margin-left");
    expectLength(computed.paddingRight, LengthUnit::Percent, 5.0f, "fallback parser should match percent padding-right");
    expect(probe.selected || !probe.warnings.empty(), "libcss percent fixture should select or explain failure");
    if (probe.selected) {
        if (probe.width.has_value()) expectNear(*probe.width, 50.0f, "libcss percent fixture should agree on width");
        if (probe.marginLeft.has_value()) expectNear(*probe.marginLeft, 10.0f, "libcss percent fixture should agree on margin-left");
        if (probe.paddingRight.has_value()) expectNear(*probe.paddingRight, 5.0f, "libcss percent fixture should agree on padding-right");
    }
}

void testLibcssSelectorAdapter() {
    const hypreact::domain::WindowSnapshot window {
        .id = "1",
        .focused = true,
    };
    const hypreact::style::StyleNodeContext workspace {
        .type = LayoutNodeType::Workspace,
        .id = "ws-1",
    };
    const hypreact::style::StyleNodeContext node {
        .type = LayoutNodeType::Window,
        .id = "win-1",
        .classes = {"main"},
        .parent = &workspace,
        .window = &window,
        .focused = true,
    };

    const auto match = hypreact::style::libcssMatchSelector("window", node);
    expect(match.matched || !match.diagnostics.empty(), "selector adapter should match or explain failure");
}

void testLibcssSelectorAdapterDiagnostics() {
    const hypreact::style::StyleNodeContext node {
        .type = LayoutNodeType::Window,
        .id = "win-1",
    };

    const auto match = hypreact::style::libcssMatchSelector("group.main", node);
    expect(!match.matched, "non-matching selector should not match");
    expect(!match.diagnostics.empty(), "non-matching selector should provide diagnostics");
    if (match.parsed) {
        expect(!match.trace.empty(), "parsed non-matching selector should capture adapter trace");
        expect(match.trace.front().find(" ") != std::string::npos, "trace entries should include payload details");
    }
}

void testSelectorSerializationForLibcss() {
    hypreact::style::Selector selector;
    selector.ancestor = hypreact::style::SimpleSelector {
        .type = std::string("workspace"),
    };
    selector.target = hypreact::style::SimpleSelector {
        .type = std::string("window"),
        .classes = {"main"},
        .pseudoclasses = {hypreact::style::PseudoClass::Focused},
    };

    const auto serialized = hypreact::style::serializeSelectorForLibcss(selector);
    expect(serialized.has_value(), "serializable selector should produce libcss selector string");
    expect(*serialized == "workspace window.main:focus", "serialized selector should preserve structure");
}

void testSelectorSerializationForLoweredPseudostates() {
    hypreact::style::Selector selector;
    selector.target = hypreact::style::SimpleSelector {
        .type = std::string("window"),
        .classes = {"main"},
        .pseudoclasses = {
            hypreact::style::PseudoClass::Floating,
        },
    };

    const auto serialized = hypreact::style::serializeSelectorForLibcss(selector);
    expect(!serialized.has_value(), "floating selector should remain fallback-only until libcss path supports it");
}

void testProductionComputeStyleUsesLibcssSelectorPath() {
    const hypreact::domain::WindowSnapshot window {
        .id = "1",
        .focused = true,
    };
    const hypreact::style::StyleNodeContext node {
        .type = LayoutNodeType::Window,
        .id = "win-1",
        .classes = {"main"},
        .window = &window,
        .focused = true,
    };

    const auto stylesheet = hypreact::style::parseStylesheetDetailed("window.main:focused { height: 22px; }");
    const auto computed = hypreact::style::computeStyle(stylesheet.stylesheet, node);
    const auto probe = hypreact::style::probeLibcssSelection("window.main:focused { height: 22px; }", node);

    expectLength(computed.height, LengthUnit::Points, 22.0f, "production computeStyle should still apply height through libcss-backed selector matching");
    expect(probe.parsed, "production selector path test should have libcss parse coverage");
    expect(probe.selected, "production selector path test should have libcss selection coverage");
}

void testProductionComputeStyleUsesLibcssCombinatorPath() {
    const hypreact::domain::WindowSnapshot window {
        .id = "1",
        .focused = true,
    };
    const hypreact::style::StyleNodeContext workspace {
        .type = LayoutNodeType::Workspace,
        .id = "ws-1",
    };
    const hypreact::style::StyleNodeContext group {
        .type = LayoutNodeType::Group,
        .id = "group-1",
        .parent = &workspace,
    };
    const hypreact::style::StyleNodeContext node {
        .type = LayoutNodeType::Window,
        .id = "win-1",
        .classes = {"main"},
        .parent = &group,
        .window = &window,
        .focused = true,
    };

    const auto stylesheet = hypreact::style::parseStylesheetDetailed("workspace window.main:focused { width: 21px; }");
    hypreact::style::ComputeStyleDiagnostics diagnostics;
    const auto computed = hypreact::style::computeStyle(stylesheet.stylesheet, node, &diagnostics);
    const auto serialized = hypreact::style::serializeSelectorForLibcss(stylesheet.stylesheet.rules.front().selector);

    expect(serialized.has_value(), "combinator selector should serialize for libcss production path");
    expect(*serialized == "workspace window.main:focus", "combinator selector should serialize descendant structure with :focus lowering");
    expectLength(computed.width, LengthUnit::Points, 21.0f, "production computeStyle should apply width for libcss-backed combinator selector");
    expect(!diagnostics.selectorMismatches.empty(), "guarded combinator selector path should record current libcss/fallback disagreement");
    expect(diagnostics.selectorMismatches.front().selector == "workspace window.main:focus", "combinator selector mismatch should preserve serialized selector");
}

void testProductionComputeStyleUsesLoweredPseudostates() {
    const hypreact::domain::WindowSnapshot window {
        .id = "1",
        .floating = true,
    };
    const hypreact::style::StyleNodeContext node {
        .type = LayoutNodeType::Window,
        .id = "win-1",
        .classes = {"main"},
        .window = &window,
        .floating = true,
    };

    const auto stylesheet = hypreact::style::parseStylesheetDetailed("window.main:floating { height: 33px; }");
    hypreact::style::ComputeStyleDiagnostics diagnostics;
    const auto computed = hypreact::style::computeStyle(stylesheet.stylesheet, node, &diagnostics);
    const auto serialized = hypreact::style::serializeSelectorForLibcss(stylesheet.stylesheet.rules.front().selector);

    expectLength(computed.height, LengthUnit::Points, 33.0f, "production computeStyle should still apply height for fallback-only floating pseudostate");
    expect(!serialized.has_value(), "floating pseudostate should stay off libcss production path for now");
    expect(diagnostics.selectorMismatches.empty(), "fallback-only floating pseudostate should not produce mismatch noise");
}

void testProductionComputeStyleFallsBackForNonRepresentableSelector() {
    hypreact::style::Selector selector;
    selector.target.universal = false;
    selector.target.pseudoclasses = {};

    const auto serialized = hypreact::style::serializeSelectorForLibcss(selector);
    expect(!serialized.has_value(), "empty selector should not be serialized for libcss");
}

void testComputeStyleSelectorMismatchDiagnostics() {
    const hypreact::style::StyleNodeContext node {
        .type = LayoutNodeType::Workspace,
        .id = "ws-1",
        .specialWorkspace = true,
    };

    const auto parsed = hypreact::style::parseStylesheetDetailed("workspace:special-workspace { height: 22px; }");
    hypreact::style::ComputeStyleDiagnostics diagnostics;
    const auto computed = hypreact::style::computeStyle(parsed.stylesheet, node, &diagnostics);

    expectLength(computed.height, LengthUnit::Points, 22.0f, "mismatch diagnostic fixture should still apply fallback height");
    expect(diagnostics.selectorMismatches.empty(), "unsupported pseudostate should stay on fallback path without mismatch noise");
}

void testFallbackAutoAndLonghandPrecedenceFixtures() {
    const hypreact::style::StyleNodeContext node {
        .type = LayoutNodeType::Window,
        .id = "win-1",
        .classes = {"main"},
    };

    const auto autoParsed = hypreact::style::parseStylesheetDetailed(hypreact::test::readFixture("libcss-auto.css"));
    const auto autoComputed = hypreact::style::computeStyle(autoParsed.stylesheet, node);
    expectLength(autoComputed.width, LengthUnit::Auto, 0.0f, "fallback parser should preserve auto width");

    const auto precedenceParsed = hypreact::style::parseStylesheetDetailed(hypreact::test::readFixture("libcss-longhand-precedence.css"));
    const auto precedenceComputed = hypreact::style::computeStyle(precedenceParsed.stylesheet, node);
    expectLength(precedenceComputed.marginLeft, LengthUnit::Points, 8.0f, "fallback parser should apply longhand precedence after shorthand");
}

void testFallbackMaxMinInsetFixture() {
    const hypreact::domain::WindowSnapshot window {
        .id = "1",
        .focused = true,
    };
    const hypreact::style::StyleNodeContext node {
        .type = LayoutNodeType::Window,
        .id = "win-1",
        .classes = {"main"},
        .window = &window,
        .focused = true,
    };

    const auto parsed = hypreact::style::parseStylesheetDetailed(hypreact::test::readFixture("libcss-maxmin-inset.css"));
    const auto computed = hypreact::style::computeStyle(parsed.stylesheet, node);
    const auto probe = hypreact::style::probeLibcssSelection(hypreact::test::readFixture("libcss-maxmin-inset.css"), node);
    expectLength(computed.maxWidth, LengthUnit::Points, 40.0f, "fallback parser should match max-width fixture");
    expectLength(computed.minHeight, LengthUnit::Points, 15.0f, "fallback parser should match min-height fixture");
    expectLength(computed.left, LengthUnit::Points, 6.0f, "fallback parser should match left inset fixture");
    expectLength(computed.top, LengthUnit::Points, 2.0f, "fallback parser should match top inset fixture");
    expect(probe.selected || !probe.warnings.empty(), "libcss max/min inset fixture should select or explain failure");
    if (probe.selected) {
        if (probe.maxWidth.has_value()) expectNear(*probe.maxWidth, 40.0f, "libcss fixture should agree on max-width");
        if (probe.minHeight.has_value()) expectNear(*probe.minHeight, 15.0f, "libcss fixture should agree on min-height");
        if (probe.left.has_value()) expectNear(*probe.left, 6.0f, "libcss fixture should agree on left");
        if (probe.top.has_value()) expectNear(*probe.top, 2.0f, "libcss fixture should agree on top");
    }
}

void testLibcssGeometryFixture() {
    const hypreact::domain::WindowSnapshot window {
        .id = "1",
        .focused = true,
    };
    const hypreact::style::StyleNodeContext node {
        .type = LayoutNodeType::Window,
        .id = "win-1",
        .classes = {"main"},
        .window = &window,
        .focused = true,
    };

    const auto stylesheet = hypreact::test::readFixture("libcss-geometry.css");
    const auto parsed = hypreact::style::parseStylesheetDetailed(stylesheet);
    const auto computed = hypreact::style::computeStyle(parsed.stylesheet, node);
    const auto probe = hypreact::style::probeLibcssSelection(stylesheet, node);
    expectLength(computed.height, LengthUnit::Points, 22.0f, "fallback parser should match height fixture");
    expectLength(computed.maxHeight, LengthUnit::Points, 44.0f, "fallback parser should match max-height fixture");
    expectLength(computed.right, LengthUnit::Percent, 10.0f, "fallback parser should match right fixture");
    expectLength(computed.bottom, LengthUnit::Percent, 20.0f, "fallback parser should match bottom fixture");
    expect(probe.selected || !probe.warnings.empty(), "libcss geometry fixture should select or explain failure");
    expect(probe.selected || !probe.diagnostics.empty(), "probe should provide structured diagnostics when needed");
    if (probe.parsed) {
        expect(!probe.trace.empty(), "parsed probe should capture trace entries");
    }
    if (probe.selected) {
        if (probe.height.has_value()) expectNear(*probe.height, 22.0f, "libcss fixture should agree on height");
        if (probe.maxHeight.has_value()) expectNear(*probe.maxHeight, 44.0f, "libcss fixture should agree on max-height");
        if (probe.right.has_value()) expectNear(*probe.right, 10.0f, "libcss fixture should agree on right");
        if (probe.bottom.has_value()) expectNear(*probe.bottom, 20.0f, "libcss fixture should agree on bottom");
    }
}

void testLibcssGeometryNoMatchFixture() {
    const hypreact::style::StyleNodeContext node {
        .type = LayoutNodeType::Window,
        .id = "win-1",
        .classes = {"main"},
    };

    const auto stylesheet = hypreact::test::readFixture("libcss-geometry-nomatch.css");
    const auto parsed = hypreact::style::parseStylesheetDetailed(stylesheet);
    const auto computed = hypreact::style::computeStyle(parsed.stylesheet, node);
    const auto match = hypreact::style::libcssMatchSelector("group.main", node);
    expect(!computed.height.has_value(), "non-matching fallback fixture should not apply height");
    expect(!computed.right.has_value(), "non-matching fallback fixture should not apply right");
    expect(!match.matched, "non-matching geometry selector should not match");
    expect(!match.diagnostics.empty(), "non-matching geometry selector should explain failure");
}

void testDebugDumpJsonFormat() {
    const hypreact::domain::WindowSnapshot window {
        .id = "1",
        .focused = true,
    };
    const hypreact::style::StyleNodeContext node {
        .type = LayoutNodeType::Window,
        .id = "win-1",
        .classes = {"main"},
        .window = &window,
        .focused = true,
    };

    const auto stylesheetSource = hypreact::test::readFixture("libcss-geometry.css");
    const auto parsed = hypreact::style::parseStylesheetDetailed(stylesheetSource);
    const auto fallback = hypreact::style::computeStyle(parsed.stylesheet, node);
    const auto match = hypreact::style::libcssMatchSelector("window.main:focus", node);
    const auto probe = hypreact::style::probeLibcssSelection(stylesheetSource, node);
    const auto json = hypreact::style::formatSelectorDebugDumpJson(match, probe, fallback, parsed.stylesheet.rules.size());
    const auto parsedJson = hypreact::test::parseJson(json);

    expect(parsedJson.has_value(), "debug dump should be valid JSON");
    expect(parsedJson->contains("matched"), "debug dump should include matched field");
    expect(parsedJson->contains("probeParsed"), "debug dump should include probeParsed field");
    expect(parsedJson->contains("fallback"), "debug dump should include fallback object");
    expect(parsedJson->contains("probeAuthoredMatch"), "debug dump should include probeAuthoredMatch field");
    expect(parsedJson->contains("position"), "debug dump should include position field");
    expect(parsedJson->contains("height"), "debug dump should include height field");
    expect(parsedJson->contains("minWidth"), "debug dump should include minWidth field");
    expect(parsedJson->contains("maxHeight"), "debug dump should include maxHeight field");
    expect(parsedJson->contains("minHeight"), "debug dump should include minHeight field");
    expect(parsedJson->contains("marginLeft"), "debug dump should include marginLeft field");
    expect(parsedJson->contains("paddingRight"), "debug dump should include paddingRight field");
    expect(parsedJson->contains("bottom"), "debug dump should include bottom field");
    expect(parsedJson->contains("trace"), "debug dump should include trace array");
    expect(parsedJson->contains("probeTrace"), "debug dump should include probeTrace array");
    expect(parsedJson->contains("diagnostics"), "debug dump should include diagnostics array");
    expect(parsedJson->contains("probeDiagnostics"), "debug dump should include probeDiagnostics array");
    expect(parsedJson->contains("probeWarnings"), "debug dump should include probeWarnings array");

    const auto& fallbackField = (*parsedJson)["fallback"];
    expect(fallbackField.is_object(), "debug dump fallback should be a parsed object");
    const auto& heightField = fallbackField["height"];
    expect(heightField.is_object(), "debug dump fallback should include parsed height object");
    expect(heightField["unit"].is_string() && heightField["unit"].get<std::string>() == "points", "debug dump fallback height unit should parse");
    expect(heightField["value"].is_number(), "debug dump fallback height value should parse as number");
    expectNear(heightField["value"].get<float>(), 22.0f, "debug dump fallback height value should match fixture");
}

void testDebugDumpGoldenFixture() {
    const hypreact::domain::WindowSnapshot window {
        .id = "1",
        .focused = true,
    };
    const hypreact::style::StyleNodeContext node {
        .type = LayoutNodeType::Window,
        .id = "win-1",
        .classes = {"main"},
        .window = &window,
        .focused = true,
    };

    const auto stylesheetSource = hypreact::test::readFixture("libcss-geometry.css");
    const auto parsed = hypreact::style::parseStylesheetDetailed(stylesheetSource);
    const auto fallback = hypreact::style::computeStyle(parsed.stylesheet, node);
    hypreact::style::LibcssSelectorMatchResult match;
    hypreact::style::LibcssSelectionProbe probe;
    probe.parsed = true;
    probe.selected = true;
    probe.height = 22.0f;
    probe.maxHeight = 44.0f;
    probe.right = 10.0f;
    probe.bottom = 20.0f;

    const auto json = hypreact::style::formatSelectorDebugDumpJson(match, probe, fallback, parsed.stylesheet.rules.size());
    const auto golden = hypreact::test::readFixture("debug-dump-golden.jsonl");
    const auto parsedJson = hypreact::test::parseJson(json);
    const auto parsedGolden = hypreact::test::parseJson(golden);

    expect(parsedJson.has_value(), "golden dump output should parse as JSON");
    expect(parsedGolden.has_value(), "golden dump fixture should parse as JSON");
    expect(parsedJson->contains("matched"), "golden dump should include matched field");
    expect(parsedGolden->contains("matched"), "golden fixture should include matched field");
    expect((*parsedJson)["matched"] == (*parsedGolden)["matched"], "golden dump should preserve matched field");
    expect((*parsedJson)["probeParsed"] == (*parsedGolden)["probeParsed"], "golden dump should preserve probeParsed field");
    expect((*parsedJson)["probeAuthoredMatch"] == (*parsedGolden)["probeAuthoredMatch"], "golden dump should preserve probeAuthoredMatch field");
    expect((*parsedJson)["fallback"] == (*parsedGolden)["fallback"], "golden dump should preserve fallback object shape");
    expect((*parsedJson)["trace"] == (*parsedGolden)["trace"], "golden dump should preserve trace array shape");
    expect((*parsedJson)["probeTrace"] == (*parsedGolden)["probeTrace"], "golden dump should preserve probeTrace array shape");
    expect((*parsedJson)["fallback"]["right"] == (*parsedGolden)["fallback"]["right"], "golden fallback right field should match exactly");
}

void testDebugDumpGoldenRichFixture() {
    hypreact::style::LibcssSelectorMatchResult match;
    match.matched = true;
    match.trace.push_back("node_name window#win-1");

    hypreact::style::LibcssSelectionProbe probe;
    probe.parsed = true;
    probe.selected = true;
    probe.display = "none";
    probe.position = "absolute";
    probe.width = 24.0f;
    probe.right = 10.0f;
    probe.trace.push_back("probe_select_start");

    hypreact::style::ComputedStyle fallback;
    fallback.display = "none";
    fallback.position = "absolute";
    fallback.width = hypreact::style::LengthValue {.unit = hypreact::style::LengthUnit::Points, .value = 24.0f};
    fallback.right = hypreact::style::LengthValue {.unit = hypreact::style::LengthUnit::Percent, .value = 10.0f};
    fallback.bottom = hypreact::style::LengthValue {.unit = hypreact::style::LengthUnit::Percent, .value = 20.0f};

    const auto json = hypreact::style::formatSelectorDebugDumpJson(match, probe, fallback, 1);
    const auto golden = hypreact::test::readFixture("debug-dump-golden-rich.jsonl");
    const auto parsedJson = hypreact::test::parseJson(json);
    const auto parsedGolden = hypreact::test::parseJson(golden);

    expect(parsedJson.has_value(), "rich golden output should parse as JSON");
    expect(parsedGolden.has_value(), "rich golden fixture should parse as JSON");
    expect((*parsedJson)["matched"] == (*parsedGolden)["matched"], "rich golden should preserve matched field");
    expect((*parsedJson)["probeAuthoredMatch"] == (*parsedGolden)["probeAuthoredMatch"], "rich golden should preserve probeAuthoredMatch field");
    expect((*parsedJson)["display"] == (*parsedGolden)["display"], "rich golden should preserve display field");
    expect((*parsedJson)["trace"].is_array(), "rich golden trace should be an array");
    expect((*parsedJson)["fallback"].is_object(), "rich golden fallback should be an object");
    expect((*parsedJson)["fallback"].contains("width"), "fallback object should include width");
    expect((*parsedJson)["fallback"].contains("right"), "fallback object should include right");
    expect((*parsedJson)["fallback"].contains("bottom"), "fallback object should include bottom");
    expect((*parsedJson)["trace"].size() == 1, "rich golden trace should contain one entry");
    expect((*parsedJson)["trace"][0].get<std::string>() == "node_name window#win-1", "rich golden trace should preserve string payload");
}

void testDebugDumpGoldenNoMatchFixture() {
    hypreact::style::LibcssSelectorMatchResult match;
    match.diagnostics.push_back("selector did not match");
    hypreact::style::LibcssSelectionProbe probe;
    probe.diagnostics.push_back("libcss selection failed");
    const hypreact::style::ComputedStyle fallback;

    const auto json = hypreact::style::formatSelectorDebugDumpJson(match, probe, fallback, 0);
    const auto golden = hypreact::test::readFixture("debug-dump-golden-nomatch.jsonl");
    const auto parsedJson = hypreact::test::parseJson(json);
    const auto parsedGolden = hypreact::test::parseJson(golden);
    expect(parsedJson.has_value() && parsedGolden.has_value(), "non-match dumps should parse as JSON");
    expect((*parsedJson)["matched"] == (*parsedGolden)["matched"], "non-match golden should preserve matched field");
    expect((*parsedJson)["probeAuthoredMatch"] == (*parsedGolden)["probeAuthoredMatch"], "non-match golden should preserve probeAuthoredMatch field");
    expect((*parsedJson)["diagnostics"] == (*parsedGolden)["diagnostics"], "non-match golden should preserve diagnostics array");
    expect((*parsedJson)["probeDiagnostics"] == (*parsedGolden)["probeDiagnostics"], "non-match golden should preserve probe diagnostics array");
    expect((*parsedJson)["probeWarnings"] == (*parsedGolden)["probeWarnings"], "non-match golden should preserve probe warnings array");
}

void testDebugDumpGoldenTracedFixture() {
    hypreact::style::LibcssSelectorMatchResult match;
    match.trace.push_back("node_name window#win-1");
    match.trace.push_back("node_has_name window#win-1");
    match.diagnostics.push_back("selector did not match");

    hypreact::style::LibcssSelectionProbe probe;
    probe.parsed = true;
    probe.trace.push_back("probe_select_start");
    probe.trace.push_back("probe_select_failure");
    probe.diagnostics.push_back("libcss selection failed");

    const hypreact::style::ComputedStyle fallback;
    const auto json = hypreact::style::formatSelectorDebugDumpJson(match, probe, fallback, 0);
    const auto golden = hypreact::test::readFixture("debug-dump-golden-traced.jsonl");
    const auto parsedJson = hypreact::test::parseJson(json);
    const auto parsedGolden = hypreact::test::parseJson(golden);

    expect(parsedJson.has_value(), "traced golden output should parse as JSON");
    expect(parsedGolden.has_value(), "traced golden fixture should parse as JSON");
    expect((*parsedJson)["trace"] == (*parsedGolden)["trace"], "traced golden should preserve selector trace array");
    expect((*parsedJson)["probeAuthoredMatch"] == (*parsedGolden)["probeAuthoredMatch"], "traced golden should preserve probeAuthoredMatch field");
    expect((*parsedJson)["probeTrace"] == (*parsedGolden)["probeTrace"], "traced golden should preserve probe trace array");
    expect((*parsedJson)["diagnostics"] == (*parsedGolden)["diagnostics"], "traced golden should preserve diagnostics array");
    expect((*parsedJson)["probeWarnings"] == (*parsedGolden)["probeWarnings"], "traced golden should preserve probe warnings array");
    expect((*parsedJson)["probeTrace"].is_array(), "traced golden probeTrace should parse as array");
    expect((*parsedJson)["probeTrace"].size() == 2, "traced golden probeTrace should preserve both entries");
    expect((*parsedJson)["probeTrace"][0].get<std::string>() == "probe_select_start", "traced golden should preserve first probe trace entry");
    expect((*parsedJson)["probeTrace"][1].get<std::string>() == "probe_select_failure", "traced golden should preserve second probe trace entry");
}

void testDebugDumpGoldenDiagnosticsFixture() {
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

    const auto json = hypreact::style::formatSelectorDebugDumpJson(match, probe, fallback, 1);
    const auto golden = hypreact::test::readFixture("debug-dump-golden-diagnostics.jsonl");
    const auto parsedJson = hypreact::test::parseJson(json);
    const auto parsedGolden = hypreact::test::parseJson(golden);

    expect(parsedJson.has_value() && parsedGolden.has_value(), "diagnostics dump should parse as JSON");
    expect(*parsedJson == *parsedGolden, "diagnostics golden should match full JSON shape");
    expect((*parsedJson)["probeDiagnostics"].is_array(), "diagnostics golden should include probeDiagnostics array");
    expect((*parsedJson)["probeDiagnostics"].size() == 1, "diagnostics golden should include one probe diagnostic");
    expect((*parsedJson)["probeDiagnostics"][0].get<std::string>() == "libcss selected subset only", "diagnostics golden should preserve probe diagnostic payload");
    expect((*parsedJson)["probeAuthoredMatch"] == false, "diagnostics golden should preserve authored match flag");
    expect((*parsedJson)["probeWarnings"].is_array(), "diagnostics golden should include probeWarnings array");
    expect((*parsedJson)["probeWarnings"].size() == 1, "diagnostics golden should include one probe warning");
    expect((*parsedJson)["probeWarnings"][0].get<std::string>() == "probe warning detail", "diagnostics golden should preserve probe warning payload");
    expectNear((*parsedJson)["height"].get<float>(), 18.0f, "diagnostics golden should preserve top-level height");
    expectNear((*parsedJson)["top"].get<float>(), 3.0f, "diagnostics golden should preserve top-level top");
    expectNear((*parsedJson)["right"].get<float>(), 11.0f, "diagnostics golden should preserve top-level right");
    expectNear((*parsedJson)["bottom"].get<float>(), 7.0f, "diagnostics golden should preserve top-level bottom");
}

void testDebugDumpGoldenNumericProbeFixture() {
    hypreact::style::LibcssSelectionProbe probe;
    probe.parsed = true;
    probe.selected = true;
    probe.minWidth = 12.0f;
    probe.minHeight = 15.0f;
    probe.marginLeft = 9.0f;
    probe.paddingRight = 4.0f;
    probe.left = 1.0f;
    probe.top = 2.0f;
    probe.right = 3.0f;
    probe.bottom = 4.0f;
    probe.warnings.push_back("numeric subset only");

    const hypreact::style::LibcssSelectorMatchResult match;
    const hypreact::style::ComputedStyle fallback;

    const auto json = hypreact::style::formatSelectorDebugDumpJson(match, probe, fallback, 0);
    const auto golden = hypreact::test::readFixture("debug-dump-golden-numeric-probe.jsonl");
    const auto parsedJson = hypreact::test::parseJson(json);
    const auto parsedGolden = hypreact::test::parseJson(golden);

    expect(parsedJson.has_value() && parsedGolden.has_value(), "numeric probe dump should parse as JSON");
    expect(*parsedJson == *parsedGolden, "numeric probe golden should match full JSON shape");
    expect((*parsedJson)["probeAuthoredMatch"] == false, "numeric probe golden should default authored match to false");
    expectNear((*parsedJson)["minWidth"].get<float>(), 12.0f, "numeric probe golden should preserve minWidth");
    expectNear((*parsedJson)["minHeight"].get<float>(), 15.0f, "numeric probe golden should preserve minHeight");
    expectNear((*parsedJson)["marginLeft"].get<float>(), 9.0f, "numeric probe golden should preserve marginLeft");
    expectNear((*parsedJson)["paddingRight"].get<float>(), 4.0f, "numeric probe golden should preserve paddingRight");
    expect((*parsedJson)["probeWarnings"].size() == 1, "numeric probe golden should preserve warning count");
    expect((*parsedJson)["probeWarnings"][0].get<std::string>() == "numeric subset only", "numeric probe golden should preserve warning payload");
}

void testDebugDumpRealFailureFixture() {
    const hypreact::style::StyleNodeContext node {
        .type = LayoutNodeType::Window,
        .id = "win-1",
        .classes = {"main"},
        .focused = true,
    };

    const auto stylesheet = hypreact::test::readFixture("libcss-invalid.css");
    const auto parsed = hypreact::style::parseStylesheetDetailed(stylesheet);
    const auto fallback = hypreact::style::computeStyle(parsed.stylesheet, node);
    const auto selector = hypreact::style::libcssMatchSelector("group.main", node);
    const auto probe = hypreact::style::probeLibcssSelection(stylesheet, node);
    const auto json = hypreact::style::formatSelectorDebugDumpJson(selector, probe, fallback, parsed.stylesheet.rules.size());
    const auto parsedJson = hypreact::test::parseJson(json);

    expect(parsedJson.has_value(), "real failure dump should parse as JSON");
    expect(!selector.matched, "real failure selector should not match");
    expect(!selector.diagnostics.empty(), "real failure selector should have diagnostics");
    expect(probe.parsed == false, "real failure probe should report parse failure");
    expect(!probe.diagnostics.empty(), "real failure probe should have diagnostics");
    expect(!probe.warnings.empty(), "real failure probe should have warnings");
    expect((*parsedJson)["probeParsed"] == false, "real failure dump should report probeParsed false");
    expect((*parsedJson)["probeAuthoredMatch"] == false, "real failure dump should report probeAuthoredMatch false");
    expect((*parsedJson)["diagnostics"].is_array() && !(*parsedJson)["diagnostics"].empty(), "real failure dump should include selector diagnostics");
    expect((*parsedJson)["probeDiagnostics"].is_array() && !(*parsedJson)["probeDiagnostics"].empty(), "real failure dump should include probe diagnostics");
    expect((*parsedJson)["probeWarnings"].is_array() && !(*parsedJson)["probeWarnings"].empty(), "real failure dump should include probe warnings");
    expect((*parsedJson)["trace"].is_array(), "real failure dump should include selector trace array");
    expect((*parsedJson)["probeTrace"].is_array() && (*parsedJson)["probeTrace"].empty(), "real failure parse failure should not include probe trace entries");
}

void testDebugDumpParsedNoMatchFixture() {
    const hypreact::style::StyleNodeContext node {
        .type = LayoutNodeType::Window,
        .id = "win-1",
        .classes = {"main"},
    };

    const auto stylesheet = hypreact::test::readFixture("libcss-geometry-nomatch.css");
    const auto parsed = hypreact::style::parseStylesheetDetailed(stylesheet);
    const auto fallback = hypreact::style::computeStyle(parsed.stylesheet, node);
    const auto selector = hypreact::style::libcssMatchSelector("group.main", node);
    const auto probe = hypreact::style::probeLibcssSelection(stylesheet, node);
    const auto json = hypreact::style::formatSelectorDebugDumpJson(selector, probe, fallback, parsed.stylesheet.rules.size());
    const auto parsedJson = hypreact::test::parseJson(json);

    expect(parsedJson.has_value(), "parsed no-match dump should parse as JSON");
    expect(parsed.stylesheet.rules.size() == 1, "parsed no-match fixture should still parse a rule");
    expect(!selector.matched, "parsed no-match selector should not match");
    expect(!fallback.height.has_value(), "parsed no-match fallback should not apply height");
    expect(!fallback.right.has_value(), "parsed no-match fallback should not apply right");
    expect((*parsedJson)["probeParsed"].is_boolean(), "parsed no-match dump should expose probeParsed state");
    expect((*parsedJson)["probeSelected"].is_boolean(), "parsed no-match dump should expose probeSelected state");
    expect((*parsedJson)["probeAuthoredMatch"].is_boolean(), "parsed no-match dump should expose probeAuthoredMatch state");
    expect((*parsedJson)["probeTrace"].is_array(), "parsed no-match dump should include probe trace array");
    expect((*parsedJson)["diagnostics"].is_array() && !(*parsedJson)["diagnostics"].empty(), "parsed no-match dump should include selector diagnostics");
    expect((*parsedJson)["probeDiagnostics"].is_array(), "parsed no-match dump should include probe diagnostics array");
    expect((*parsedJson)["probeWarnings"].is_array(), "parsed no-match dump should include probe warnings array");
}

} // namespace

int main() {
    testParseLengthValue();
    testParseStylesheetDetailed();
    testComputeStyle();
    testSpecificityAndExtendedProperties();
    testShorthandAndAdditionalFlexProperties();
    testSelectorListsAndBorderProperties();
    testChildCombinatorAndWindowAttributes();
    testDescendantCombinator();
    testLibcssFallbackWarning();
    testPseudoclasses();
    testResolvedTreeSelectors();
    testLibcssSelectionProbe();
    testLibcssCrossCheck();
    testLibcssCrossCheckWithAttribute();
    testLibcssCrossCheckPositionFixture();
    testLibcssCrossCheckDescendantFixture();
    testLibcssCrossCheckPseudostateFixture();
    testLibcssCrossCheckPercentFixture();
    testLibcssSelectorAdapter();
    testLibcssSelectorAdapterDiagnostics();
    testSelectorSerializationForLibcss();
    testSelectorSerializationForLoweredPseudostates();
    testProductionComputeStyleUsesLibcssSelectorPath();
    testProductionComputeStyleUsesLibcssCombinatorPath();
    testProductionComputeStyleUsesLoweredPseudostates();
    testProductionComputeStyleFallsBackForNonRepresentableSelector();
    testComputeStyleSelectorMismatchDiagnostics();
    testFallbackAutoAndLonghandPrecedenceFixtures();
    testFallbackMaxMinInsetFixture();
    testLibcssGeometryFixture();
    testLibcssGeometryNoMatchFixture();
    testDebugDumpJsonFormat();
    testDebugDumpGoldenFixture();
    testDebugDumpGoldenRichFixture();
    testDebugDumpGoldenNoMatchFixture();
    testDebugDumpGoldenTracedFixture();
    testDebugDumpGoldenDiagnosticsFixture();
    testDebugDumpGoldenNumericProbeFixture();
    testDebugDumpRealFailureFixture();
    testDebugDumpParsedNoMatchFixture();
    std::cout << "hypreact_style_tests: ok\n";
    return 0;
}
