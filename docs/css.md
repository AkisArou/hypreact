# hypreact CSS Spec

`hypreact` CSS is intended to be parsed via a `libcss` bridge into `hypreact` domain stylesheet types. The current implementation still uses a temporary internal parser behind that bridge, so the supported surface is narrower than the long-term target.

## Current Implementation Status

Implemented today:

- stylesheet loading from config-root `index.css` and selected-layout `index.css`
- simple selectors: `*`, type, `#id`, `.class`, type/id/class compounds without combinators such as `workspace.main`, `group#left`, and `window.primary.tiled`, plus comma-separated selector lists of those supported simple selectors
- limited child combinators with simple selectors on both sides, such as `group > window`
- limited descendant combinators with simple selectors on both sides, such as `workspace window`
- exact-match window attribute selectors on simple `window` selectors: `[app-id="..."]`, `[class="..."]`, `[title="..."]`, `[floating="true|false"]`, `[fullscreen="true|false"]`
- layout properties: `display`, `position`, `top`, `right`, `bottom`, `left`, `flex-direction`, `flex-wrap`, `flex-grow`, `flex-shrink`, `flex-basis`, `gap`, `row-gap`, `column-gap`, `justify-content`, `align-items`, `align-self`, `align-content`, `overflow`, `width`, `height`, `min-width`, `min-height`, `max-width`, `max-height`, `aspect-ratio`, `box-sizing`, `border-width`, `border-top-width`, `border-right-width`, `border-bottom-width`, `border-left-width`, `margin`, `margin-top`, `margin-right`, `margin-bottom`, `margin-left`, `padding`, `padding-top`, `padding-right`, `padding-bottom`, `padding-left`
- typed values currently support point values (`8`, `8px`), percent values (`50%`), and `auto` where Yoga supports it (`width`, `height`, `flex-basis`, `margin*`)
- unsupported selectors and declarations now produce parse warnings at the bridge boundary; the plugin surfaces the first warning as a Hyprland notification
- matching rules now cascade by selector specificity first, then source order

Not implemented yet:

- real `libcss` selector parsing and error reporting
- sibling combinators and richer attribute forms

The current `libcss` bridge now performs a syntax-only parse check before falling back to the temporary domain parser. Selector/property support is still governed by the fallback parser, but syntax problems can now produce an explicit bridge warning.

There is also a small `libcss` selection probe used in tests to cross-check overlapping selector/display behavior against a minimal `StyleNodeContext` adapter. It is not yet the production selection path.

The test suite now includes fixture-driven probe cross-checks for a small overlapping subset, including `display`, `position`, `width`, `min-width`, `overflow`, `box-sizing`, `margin-left`, and `padding-right`, including percent-valued overlap cases when the minimal probe handler can resolve them.

The probe-side node adapter is now split into a reusable libcss selector adapter module so future production-side selector integration can build on the same `StyleNodeContext` bridge.

There is now also a small production-facing selector utility around that adapter for direct selector-match checks against `StyleNodeContext`, including a structured diagnostic result used in tests while the full production selection path is still in transition.

Plugin-side selector diagnostic notifications can now be gated with `plugin:hypreact:debug_selector_diagnostics = 1`.

Plugin-side selector dumps comparing selector-match state and probe output can be enabled with `plugin:hypreact:debug_selector_dump = 1`.

When enabled, the current dump includes match state plus a compact subset of probe fields such as `display`, `position`, `width`, `maxWidth`, `left`, and `right`.

If `plugin:hypreact:debug_selector_dump_path` is set, the plugin also appends the dump plus selector/probe trace lines to that file.

The file dump now uses JSON lines and includes selector-match state, probe state, selector/probe traces, and a richer fallback snapshot including parsed rule count plus computed fallback fields such as width/height, max constraints, and inset values.

The test suite now validates key JSON dump fields, arrays, nested fallback fields, and matching/non-matching/traced golden dump fixtures, including a richer populated success fixture, to keep the debug record shape stable while the dump grows.

There is also a small `hypreact_debug_dump_preview` test utility target that emits one formatted dump record for quick manual inspection.

The probe overlap surface now also includes `max-width`, `min-height`, `left`, `top`, `height`, `max-height`, `right`, and `bottom` for fixture-driven comparisons where the current adapter can resolve them.

Currently implemented pseudoclasses are `:focused`, `:focused-within`, `:fullscreen`, `:floating`, `:urgent`, and `:special-workspace` where the runtime style context carries the needed state.
- shorthand expansion beyond `margin`, `padding`, and `border-width`
- plugin-owned visual properties

## File Conventions

Given a config root like `~/.config/hypreact`:

- global stylesheet: `~/.config/hypreact/index.css`
- selected layout stylesheet: `~/.config/hypreact/layouts/<name>/index.css`
- component stylesheet: sibling `*.css` next to component `*.tsx`

Cascade order:

1. global stylesheet
2. component stylesheets in module discovery order
3. selected layout stylesheet

## Node Types

Supported node selectors:

- `workspace`
- `group`
- `window`
- `slot`

Each node may also expose:

- `#id`
- `.class`

## Target Selector Surface

### Basic selectors

- type selectors
- `#id`
- `.class`
- compound selectors such as `window.master:focused`

### Combinators

- descendant combinator
- child combinator (`>`)

### Attributes

Supported only on `window` selectors in v1:

- `[app-id="..."]`
- `[class="..."]`
- `[title="..."]`
- `[floating="true" | "false"]`
- `[fullscreen="true" | "false"]`

Unsupported selector forms should fail clearly once the real parser is in place. For now they are ignored by the temporary parser.

## Target Pseudoclasses

### Structural

- `:root`
- `:empty`
- `:first-child`
- `:last-child`
- `:only-child`

### Layout/content

- `:occupied`
- `:vacant`
- `:master`
- `:stacked`
- `:single-window`

### Window/workspace state

- `:focused`
- `:focused-within`
- `:floating`
- `:fullscreen`
- `:urgent`
- `:visible`
- `:active-monitor`
- `:inactive-monitor`
- `:special-workspace`

## Property Groups

### Currently implemented Yoga-backed properties

- `display`
- `position`
- `top`, `right`, `bottom`, `left`
- `flex-direction`
- `flex-wrap`
- `flex-grow`
- `flex-shrink`
- `flex-basis`
- `gap`
- `row-gap`
- `column-gap`
- `justify-content`
- `align-items`
- `align-self`
- `align-content`
- `overflow`
- `width`
- `height`
- `min-width`, `min-height`
- `max-width`, `max-height`
- `aspect-ratio`
- `box-sizing`
- `border-width`
- `border-top-width`, `border-right-width`, `border-bottom-width`, `border-left-width`
- `margin`
- `margin-top`, `margin-right`, `margin-bottom`, `margin-left`
- `padding`
- `padding-top`, `padding-right`, `padding-bottom`, `padding-left`

### Planned Yoga-backed properties

`hypreact` v1 is flex-first. Grid properties are intentionally out of scope until there is a clear design on top of Yoga.

Current shorthand support is limited to `margin`, `padding`, and `border-width` with 1, 2, 3, or 4 whitespace-separated values.

## Plugin-owned visual properties

These are only applied where `hypreact` owns a visual/decorative surface:

- `opacity`
- `background-color`
- `border-color`
- `border-radius`
- `color`
- `font-family`
- `font-size`
- `font-weight`
- `text-align`
- `blur`

## Host-controlled Hyprland visuals

These remain Hyprland-owned and should not be silently redefined as `hypreact` CSS semantics:

- global gaps and border size
- active/inactive border gradients
- default rounding
- built-in blur and shadow configuration
- built-in window opacity rules
- compositor animation timing and interpolation

Relevant host variables include Hyprland values such as `general:gaps_in`, `general:gaps_out`, `general:border_size`, `general:col.*`, and `decoration:*`.

## Explicitly Unsupported In V1

- `@keyframes`
- CSS transitions
- pseudo-elements
- sibling combinators
- `:not()` / `:is()`
- browser-only CSS concepts that do not map to layout or plugin-owned visuals
