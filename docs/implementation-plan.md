# Implementation Plan

## Phase 1 - Repository bootstrap

- create plugin skeleton
- add docs and SDK
- establish config/file conventions
- vendor `QuickJS`, `Yoga`, and `libcss` under `third_party/`
- keep CMake setup usable even before every Hyprland native dependency is present locally

## Phase 2 - Domain types

- define runtime window/monitor/workspace snapshot types
- define resolved layout node types
- define stylesheet domain types
- define selector, pseudoclass, and property enums

## Phase 3 - QuickJS runtime

- embed `QuickJS`
- add module loader rooted at the hypreact config directory
- expose the `hypreact` JSX runtime
- evaluate `layout(ctx)` for a selected layout

## Phase 4 - CSS parser boundary

- embed `libcss`
- parse source stylesheets in-process
- normalize them into `hypreact` stylesheet types
- keep all later code independent from `libcss` internals
- either vendor or bridge the remaining `libparserutils` and `libwapcaplet` dependency chain for fully self-contained builds

## Phase 5 - Selector matching and cascade

- implement matching for node types, ids, classes, attributes, and pseudoclasses
- compute specificity and source order
- produce computed style objects per node

## Phase 6 - Yoga integration

- create Yoga nodes for resolved layout nodes
- apply computed layout declarations
- compute geometry

## Phase 7 - Hyprland placement integration

- apply computed geometry to tiled windows
- skip or specially handle floating/fullscreen windows as needed
- recompute on relevant Hyprland events

## Phase 8 - Visual/deco extensions

- add plugin-owned decorative surfaces only if necessary
- keep Hyprland host-owned animation and decoration semantics intact by default

## First Milestone

The first meaningful milestone is:

- load one layout from `layouts/<name>/index.tsx`
- parse `index.css`
- evaluate one workspace layout against current Hyprland windows
- compute Yoga geometry
- place windows in a simple master-stack layout
