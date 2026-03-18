# Runtime Model

`hypreact` uses `QuickJS` to evaluate TSX layouts at runtime.

## Entry Contract

Each layout entrypoint exports:

```ts
export default function layout(ctx: LayoutContext) {
  return <workspace />;
}
```

The runtime should treat layout evaluation as a pure function of `LayoutContext`.

## JSX Runtime Scope

Initial support:

- function components
- `Fragment`
- `props.children`
- intrinsic elements: `workspace`, `group`, `window`, `slot`

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
