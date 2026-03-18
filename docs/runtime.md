# Runtime Model

`hypreact` uses `QuickJS` to evaluate compiled JavaScript layout modules at runtime.

## Entry Contract

Each layout entrypoint exports:

```ts
export default function layout(ctx: LayoutContext) {
  return <workspace />;
}
```

Author-facing layouts can be written in `js`, `ts`, `jsx`, or `tsx`. `hypreact` compiles them into `layouts/<name>/index.js` in the cache root before loading them with `QuickJS`.

The runtime should treat layout evaluation as a pure function of `LayoutContext`.

## Layout Selection

The selected layout should be driven by Hyprland plugin config values:

- `plugin:hypreact:layout`
- `plugin:hypreact:default_layout`
- `plugin:hypreact:workspace_layout = <workspace>,<layout>`
- `plugin:hypreact:monitor_layout = <monitor>,<layout>`

The runtime currently loads `layouts/<name>/index.js` from the configured cache root, while authored source remains in the config root.

Current compiler behavior:

- `hypreact` uses `esbuild` to bundle authored `js`, `ts`, `jsx`, and `tsx` entrypoints into cache-local JS modules
- the runtime then loads only the compiled cache artifacts

Intended precedence:

1. explicit `layout`
2. `monitor_layout`
3. `workspace_layout`
4. `default_layout`
5. fallback `first`

## JSX Runtime Scope

Initial support:

- function components
- `Fragment`
- `props.children`
- intrinsic elements: `workspace`, `group`, `window`, `slot`
- built-in virtual runtime module: `hypreact/jsx-runtime`

Not planned for v1:

- React hooks
- effects
- asynchronous rendering semantics
- arbitrary host object access

## Context Shape

`LayoutContext` should be adapted to Hyprland and include:

- monitor name and geometry
- active workspace information
- visible windows on the current workspace
- focused window metadata
- optional plugin-owned transient state later

The JS environment should remain deterministic and capability-limited.
