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
- `third_party/` - pinned upstream sources for `QuickJS` and `Yoga`

## Dependencies

Core embedded dependencies are vendored as git submodules:

- `third_party/quickjs`
- `third_party/yoga`

Clone with:

```sh
git clone --recursive git@github.com:AkisArou/hypreact.git
```

If you already cloned without submodules:

```sh
git submodule update --init --recursive
```

These are intentionally pinned for reproducible plugin builds.

System CSS dependencies:

- `libcss`
- `libparserutils`
- `libwapcaplet`

## Build Notes

- `QuickJS` is compiled from vendored source in `third_party/quickjs`.
- `Yoga` is compiled from vendored source in `third_party/yoga`.
- `libcss` is resolved from the system via `pkg-config`, along with `libparserutils` and `libwapcaplet`.

Current CMake behavior:

- `pkg-config`, `libcss`, `libparserutils`, and `libwapcaplet` are required at configure time
- `QuickJS` and `Yoga` build from vendored source successfully under CMake
- the final plugin target still depends on Hyprland's native header/runtime stack, including transitive dependencies such as `pixman`

Example distro package notes:

```sh
# Arch
sudo pacman -S libcss libparserutils libwapcaplet pkgconf

# Fedora
sudo dnf install libcss-devel libparserutils-devel libwapcaplet-devel pkgconf-pkg-config
```

Other ecosystems such as Nixpkgs, openSUSE, Gentoo, Void, and FreeBSD Ports also package `libcss`.

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
