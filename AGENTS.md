# AGENTS

This repository is for building `hypreact`, a Hyprland layout plugin.

## Mission

Build `hypreact` as a Hyprland-native layout plugin with:

- `QuickJS` for runtime TSX layout/component evaluation
- `Yoga` for layout computation
- `libcss` for in-process CSS parsing
- custom `hypreact` stylesheet domain types used by runtime/layout logic

## Hard Decisions

- This is a Hyprland plugin, not a standalone compositor.
- Layout authoring uses TSX and runs at runtime through `QuickJS`.
- CSS is parsed with `libcss`, but runtime code must use `hypreact` domain stylesheet types.
- The structural node model remains `workspace`, `group`, `window`, `slot`.
- Hyprland keeps ownership of compositor animations and most built-in decoration behavior.
- `hypreact` v1 is flex-first and Yoga-backed; grid is out of scope unless explicitly added.
- `QuickJS` and `Yoga` are vendored as pinned git submodules under `third_party/`.
- `libcss`, `libparserutils`, and `libwapcaplet` are expected from the system package manager.

## Priority Order

1. Keep the runtime boundary small and deterministic.
2. Preserve a clean authored model for TSX layouts and component-local CSS.
3. Keep CSS semantics explicit and fail clearly on unsupported syntax.
4. Do not let parser-library types leak into core runtime/layout code.
5. Prefer direct, testable domain types over convenience wrappers.

## Implementation Rules

- Treat JS layout functions as pure functions of `LayoutContext`.
- Keep `QuickJS` host bindings minimal and capability-limited.
- Parse CSS in-process, then convert immediately to `hypreact` stylesheet types.
- Support pseudoclasses that reflect compositor/layout state rather than browser DOM state.
- Distinguish clearly between:
  - Yoga-backed layout properties
  - plugin-owned visual properties
  - Hyprland host-controlled visuals
- Use `hy3` and official Hyprland plugins as architectural references for plugin lifecycle and integration.

## File Conventions

Assume a config root such as `~/.config/hypreact`:

- `index.css` - global stylesheet
- `layouts/<name>/index.tsx` - layout entrypoint
- `layouts/<name>/index.css` - layout stylesheet
- `components/**/*.tsx` - reusable components
- sibling `*.css` files for component-local styling

## Expected Deliverables

When implementing a subsystem, update the relevant docs in `docs/` if the implementation refines the design.

Key docs:

- `docs/architecture.md`
- `docs/css.md`
- `docs/runtime.md`
- `docs/implementation-plan.md`
