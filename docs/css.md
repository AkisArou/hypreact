# hypreact CSS Spec

`hypreact` CSS is parsed in-process with `libcss`, then converted into `hypreact` domain stylesheet types. Runtime code should not depend directly on `libcss` types after parsing.

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

## Supported Selectors

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

Unsupported selector forms should fail clearly.

## Supported Pseudoclasses

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

## Supported Property Groups

## Layout properties (Yoga-backed)

- `display`
- `position`
- `top`, `right`, `bottom`, `left`
- `width`, `height`
- `min-width`, `min-height`
- `max-width`, `max-height`
- `margin`
- `margin-top`, `margin-right`, `margin-bottom`, `margin-left`
- `padding`
- `padding-top`, `padding-right`, `padding-bottom`, `padding-left`
- `border-width`
- `border-top-width`, `border-right-width`, `border-bottom-width`, `border-left-width`
- `flex-direction`
- `flex-wrap`
- `flex-grow`
- `flex-shrink`
- `flex-basis`
- `justify-content`
- `align-items`
- `align-self`
- `align-content`
- `gap`
- `row-gap`
- `column-gap`
- `aspect-ratio`
- `overflow`

`hypreact` v1 is flex-first. Grid properties are intentionally out of scope until there is a clear design on top of Yoga.

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
