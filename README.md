# hypreact

`hypreact` is a Hyprland layout plugin that combines:

- `QuickJS` for runtime TSX layout and component evaluation
- `Yoga` for layout computation
- `libcss` for in-process CSS parsing into `hypreact` domain stylesheet types

The project keeps the `spiders-wm` structural layout model while targeting Hyprland as the compositor host instead of implementing a compositor from scratch.

Development setup notes live in `docs/development.md`.

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
- authored source can live as `js`, `ts`, `jsx`, or `tsx`
- reusable components: `~/.config/hypreact/components/**/*.{js,ts,jsx,tsx}`
- component stylesheet convention: sibling `*.css` next to component `*.tsx`

Compiled runtime artifacts should live in a cache root such as `${XDG_CACHE_HOME:-~/.cache}/hypreact`.

The intended split is:

- config root: authored source and styles
- cache root: compiled JavaScript modules consumed by `QuickJS`

Current compiler behavior:

- authored `js`, `ts`, `jsx`, and `tsx` layout entrypoints are bundled with `esbuild`
- compiled output is written into the cache root and consumed from there by `QuickJS`
- `hypreact` uses the built-in virtual runtime module `hypreact/jsx-runtime` during bundling/runtime execution

The plugin should load global CSS first, then component CSS discovered through module loading, and finally the selected layout CSS.

## Hyprland Plugin Config

`hypreact` is configured primarily through the Hyprland config under `plugin:hypreact`.

Supported keys:

- `config_path` - root directory containing `index.css`, `layouts/`, and `components/`
- `cache_path` - root directory containing compiled runtime JS artifacts
- `layout` - optional hard override for the selected layout
- `default_layout` - fallback layout when no override or mapping matches
- `workspace_layout = <workspace>,<layout>` - repeatable mapping
- `monitor_layout = <monitor>,<layout>` - repeatable mapping

Example:

```ini
plugin {
  hypreact {
    config_path = ~/.config/hypreact
    layout =
    default_layout = first

    workspace_layout = 1,master-stack
    workspace_layout = 2,columns
    workspace_layout = special:scratch,floating

    monitor_layout = DP-1,wide
    monitor_layout = HDMI-A-1,stack
  }
}
```

Selection precedence is intended to be:

1. `layout` if explicitly set
2. matching `monitor_layout`
3. matching `workspace_layout`
4. `default_layout`
5. fallback `first`

## Initial Scope

- plugin bootstrap and Hyprland layout registration
- `QuickJS` module runtime for layout TSX
- `libcss` parser integration into `hypreact` stylesheet types
- selector matching for node types, ids, classes, attributes, and selected pseudoclasses
- Yoga-backed flex layout for resolved node trees

See `docs/implementation-plan.md` and `docs/css.md` for the working spec.
