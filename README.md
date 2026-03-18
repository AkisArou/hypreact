# hypreact

`hypreact` is a Hyprland layout plugin that combines:

- `QuickJS` for runtime TSX layout and component evaluation
- `Yoga` for layout computation
- `libcss` for in-process CSS parsing into `hypreact` domain stylesheet types

The project keeps the `spiders-wm` structural layout model while targeting Hyprland as the compositor host instead of implementing a compositor from scratch.

## Goals

- use `workspace`, `group`, `window`, and `slot` nodes as the authored layout model
- evaluate layouts from TSX at runtime with `QuickJS`
- support CSS for layout and plugin-owned presentation using our own stylesheet domain types
- let Hyprland keep responsibility for compositor-level rendering, animations, and built-in decorations

## Non-Goals

- no standalone compositor
- no full browser CSS model
- no `@keyframes`, CSS transitions, or standalone animation engine
- no config-runtime surface like `spiders-wm` config/actions

## Planned Repository Layout

- `src/` - plugin entrypoint, layout engine, runtime integration, CSS/style system
- `sdk/` - authoring SDK for TSX layouts and CSS module typing
- `docs/` - architecture, CSS spec, runtime model, and implementation plan

## Config Conventions

Given a config root such as `~/.config/hypreact`:

- global stylesheet: `~/.config/hypreact/index.css`
- layout entrypoint: `~/.config/hypreact/layouts/<name>/index.tsx`
- layout stylesheet: `~/.config/hypreact/layouts/<name>/index.css`
- reusable components: `~/.config/hypreact/components/**/*.tsx`
- component stylesheet convention: sibling `*.css` next to component `*.tsx`

The plugin should load global CSS first, then component CSS discovered through module loading, and finally the selected layout CSS.

## Initial Scope

- plugin bootstrap and Hyprland layout registration
- `QuickJS` module runtime for layout TSX
- `libcss` parser integration into `hypreact` stylesheet types
- selector matching for node types, ids, classes, attributes, and selected pseudoclasses
- Yoga-backed flex layout for resolved node trees

See `docs/implementation-plan.md` and `docs/css.md` for the working spec.
