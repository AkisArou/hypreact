#include <cmath>
#include <cstdlib>
#include <iostream>
#include <optional>
#include <string>

#include "domain/layout_node.hpp"
#include "domain/runtime_snapshot.hpp"
#include "layout/resolver.hpp"
#include "style/libcss_bridge.hpp"
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
    expect(parsed.warnings.empty(), "supported shorthand should not warn");
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

    expect(probe.selected || !probe.warnings.empty(), "libcss probe should either select or explain failure");
    if (probe.selected) {
        expect(probe.display.has_value() && *probe.display == "none", "libcss probe should resolve display");
    }
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

    const std::string stylesheetSource = hypreact::test::readFixture("libcss-display.css");

    const auto parsed = hypreact::style::parseStylesheetDetailed(stylesheetSource);
    const auto computed = hypreact::style::computeStyle(parsed.stylesheet, node);
    const auto probe = hypreact::style::probeLibcssSelection(stylesheetSource, node);

    expect(computed.display.has_value() && *computed.display == "none", "fallback parser should match cross-check rule");
    expect(probe.selected || !probe.warnings.empty(), "libcss cross-check should select or explain failure");
    if (probe.selected && probe.display.has_value()) {
        expect(*probe.display == "none", "libcss cross-check should agree on display");
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

    const std::string stylesheetSource = hypreact::test::readFixture("libcss-descendant.css");
    const auto parsed = hypreact::style::parseStylesheetDetailed(stylesheetSource);
    const auto computed = hypreact::style::computeStyle(parsed.stylesheet, node);
    const auto probe = hypreact::style::probeLibcssSelection(stylesheetSource, node);

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

    const std::string stylesheetSource = hypreact::test::readFixture("libcss-pseudostate.css");
    const auto parsed = hypreact::style::parseStylesheetDetailed(stylesheetSource);
    const auto computed = hypreact::style::computeStyle(parsed.stylesheet, node);
    const auto probe = hypreact::style::probeLibcssSelection(stylesheetSource, node);

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
    expectLength(computed.right, LengthUnit::Points, 5.0f, "fallback parser should match right fixture");
    expectLength(computed.bottom, LengthUnit::Points, 7.0f, "fallback parser should match bottom fixture");
    expect(probe.selected || !probe.warnings.empty(), "libcss geometry fixture should select or explain failure");
    if (probe.selected) {
        if (probe.height.has_value()) expectNear(*probe.height, 22.0f, "libcss fixture should agree on height");
        if (probe.maxHeight.has_value()) expectNear(*probe.maxHeight, 44.0f, "libcss fixture should agree on max-height");
        if (probe.right.has_value()) expectNear(*probe.right, 5.0f, "libcss fixture should agree on right");
        if (probe.bottom.has_value()) expectNear(*probe.bottom, 7.0f, "libcss fixture should agree on bottom");
    }
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
    testFallbackAutoAndLonghandPrecedenceFixtures();
    testFallbackMaxMinInsetFixture();
    testLibcssGeometryFixture();
    std::cout << "hypreact_style_tests: ok\n";
    return 0;
}
