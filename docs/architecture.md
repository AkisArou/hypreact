# Architecture

`hypreact` is a Hyprland plugin, not a compositor. Hyprland owns the compositor lifecycle, rendering pipeline, built-in animation system, and most decoration semantics. `hypreact` owns layout authoring, structural node resolution, CSS matching, Yoga layout computation, and placement policy.

## Core Layers

1. `plugin bootstrap`
   - version check
   - layout registration with Hyprland
   - config root discovery
   - reload hooks
   - initialize pinned third-party runtimes from `third_party/`

2. `runtime snapshot`
   - collect monitor, workspace, and window state from Hyprland
   - convert it into `LayoutContext`

3. `QuickJS runtime`
   - load the selected `layouts/<name>/index.tsx` bundle
   - provide the `hypreact` JSX runtime and SDK types
   - evaluate `layout(ctx)`
   - built from vendored `third_party/quickjs`

4. `resolved node tree`
   - normalize returned JSX output
   - keep `workspace`, `group`, `window`, and `slot` nodes
   - resolve window matching and slot occupancy

5. `style system`
   - load global, component, and layout CSS files
   - parse them with `libcss`
   - convert into `hypreact` stylesheet domain types
   - match selectors and compute cascaded styles
   - parser source vendored in `third_party/libcss`
   - current build still depends on `libparserutils` and `libwapcaplet`

6. `Yoga bridge`
   - create Yoga nodes for structural layout nodes
   - apply layout properties
   - compute rectangles
   - built from vendored `third_party/yoga`

7. `Hyprland placement`
   - assign computed boxes to tiled windows
   - recompute on window, workspace, focus, monitor, or stylesheet changes

## Structural Model

The node model intentionally matches the `spiders-wm` layout mental model:

- `workspace`
- `group`
- `window`
- `slot`

This keeps authored layouts portable in spirit while adapting the runtime context to Hyprland.

## Style Model

Authors write one CSS surface, but internally declarations are categorized by target:

- `Yoga` target for layout geometry
- `NodeMeta` target for runtime/layout metadata
- `Decoration` target for plugin-owned visual styling

Hyprland-owned visuals such as compositor animations remain host-controlled.
