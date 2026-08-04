

# tilescript

`tilescript` es un runtime de disposiciones diseñado para compositores Wayland y gestores de ventanas en mosaico.

Te permite definir disposiciones de espacios de trabajo en JSX/TSX, Lua o Fennel junto con CSS, luego evalúa esas disposiciones en Rust y entrega las decisiones de colocación resultantes a un adaptador de gestor de ventanas o compositor.

> Adaptador objetivo actual: `Hyprland`

> Entorno de pruebas: <https://akisarou.github.io/tilescript/>

## Características

- Disposiciones en JSX/TSX, Lua y Fennel
- Disposiciones impulsadas por CSS
- Selección de disposición en tiempo de ejecución por índice de espacio de trabajo, nombre de espacio de trabajo y monitor
- Comportamiento de redimensionamiento inferido por flex con paso de redimensionamiento y tamaño mínimo de panel configurables
- Integración con plugins de Hyprland
- Crates de servidor de lenguaje CSS y un paquete cliente para VS Code

## Ejemplos

<details open>
<summary><code>layouts/master-stack/index.tsx</code></summary>

```tsx
import type { LayoutContext } from "@tilescript/sdk/layout";

import "./index.css";

export default function layout(ctx: LayoutContext) {
  return (
    <workspace>
      <slot take={1} class="master-slot" />

      {ctx.windows.length > 1 ? (
        <group class="stack-group">
          <slot class="stack-slot" />
        </group>
      ) : null}
    </workspace>
  );
}
```

</details>

<details>
<summary><code>layouts/master-stack/index.lua</code></summary>

```lua
local h = require("tilescript")

---@param ctx Tilescript.LayoutContext
return function(ctx)
  return h.workspace() {
    h.slot({
      take = 1,
      class = "master-slot",
    }),

    h.when(#ctx.windows > 1) {
      h.group({ class = "stack-group" }) {
        h.slot({
          class = "stack-slot",
        }),
      },
    },
  }
end
```

</details>

<details>
<summary><code>layouts/master-stack/index.fnl</code></summary>

```fennel
(local h (require "tilescript"))

(fn [ctx]
  ((h.workspace)
   [(h.slot {:take 1
             :class "master-slot"})
    ((h.when (> (# ctx.windows) 1))
     [((h.group {:class "stack-group"})
       [(h.slot {:class "stack-slot"})])])]))
```

</details>

`layouts/master-stack/index.css`

```css
workspace {
  display: flex;
  flex-direction: row;
  gap: 6px;
  padding: 6px;
  width: 100%;
  height: 100%;

  .master-slot {
    flex-basis: 0;
    flex-grow: 3;
    min-width: 0;
    min-height: 0;
  }

  .stack-group {
    display: flex;
    flex-direction: column;
    gap: 6px;
    flex-basis: 0;
    flex-grow: 2;
    min-width: 0;

    .stack-slot {
      flex-basis: 0;
      flex-grow: 1;
      min-height: 0;
    }
  }
}
```

## Estructura de Configuración

`tilescript` carga un directorio raíz de configuración.

Si se omite `config_path`, el directorio raíz de configuración predeterminado es `~/.config/tilescript`.

Como mínimo, ese directorio contiene:

- una entrada de configuración como `config.{ts, tsx, js, jsx, lua, fnl}`.
- uno o más disposiciones bajo `layouts/<name>/`
- un `index.css` raíz opcional para reglas de hoja de estilo compartidas

Para disposiciones iniciales y esqueletos de configuración, consulta **`examples/`**.

Estructura típica de proyecto:

```text
config.ts
index.css
layouts/
  master-stack/
    index.tsx
    index.css
  dwindle/
    index.lua
    index.css
```

## Hyprland

Para usar `tilescript` con Hyprland:

1. Compila el plugin con `make hypr-plugin`.
2. Ejecuta `make hypr-plugin-snippet` y pega el bloque `plugin` impreso en tu configuración de Hyprland.
3. Establece `config_path` en tu directorio raíz de configuración.
4. Recarga Hyprland o recarga el plugin después de cambiar las disposiciones o la configuración.

Ejemplo de configuración de Hyprland:

```ini
plugin = /absolute/path/to/tilescript-hypr.so

plugin {
  tilescript-hypr {
    config_path = /absolute/path/to/your/tilescript-config/
  }
}


general {
    # disable gaps. those should be better configured via CSS
    gaps_in = 0
    gaps_out = 0

    layout = tilescript
}
```

Combinaciones de teclas relacionadas:

```ini
$mainMod = ALT

bind = $mainMod, h, tilescript-hypr, focus, l
bind = $mainMod, j, tilescript-hypr, focus, d
bind = $mainMod, k, tilescript-hypr, focus, u
bind = $mainMod, l, tilescript-hypr, focus, r

bind = $mainMod CTRL, h, tilescript-hypr, resize, l
bind = $mainMod CTRL, j, tilescript-hypr, resize, d
bind = $mainMod CTRL, k, tilescript-hypr, resize, u
bind = $mainMod CTRL, l, tilescript-hypr, resize, r

bind = $mainMod SHIFT, h, tilescript-hypr, move, l
bind = $mainMod SHIFT, j, tilescript-hypr, move, d
bind = $mainMod SHIFT, k, tilescript-hypr, move, u
bind = $mainMod SHIFT, l, tilescript-hypr, move, r

bind = $mainMod, 1, tilescript-hypr, workspace, 1
bind = $mainMod, 2, tilescript-hypr, workspace, 2
bind = $mainMod, 3, tilescript-hypr, workspace, 3
bind = $mainMod, 4, tilescript-hypr, workspace, 4
bind = $mainMod, 5, tilescript-hypr, workspace, 5
bind = $mainMod, 6, tilescript-hypr, workspace, 6
bind = $mainMod, 7, tilescript-hypr, workspace, 7
bind = $mainMod, 8, tilescript-hypr, workspace, 8
bind = $mainMod, 9, tilescript-hypr, workspace, 9
bind = $mainMod, 0, tilescript-hypr, workspace, 10

bind = $mainMod SHIFT, 1, tilescript-hypr, movetoworkspace, 1
bind = $mainMod SHIFT, 2, tilescript-hypr, movetoworkspace, 2
bind = $mainMod SHIFT, 3, tilescript-hypr, movetoworkspace, 3
bind = $mainMod SHIFT, 4, tilescript-hypr, movetoworkspace, 4
bind = $mainMod SHIFT, 5, tilescript-hypr, movetoworkspace, 5
bind = $mainMod SHIFT, 6, tilescript-hypr, movetoworkspace, 6
bind = $mainMod SHIFT, 7, tilescript-hypr, movetoworkspace, 7
bind = $mainMod SHIFT, 8, tilescript-hypr, movetoworkspace, 8
bind = $mainMod SHIFT, 9, tilescript-hypr, movetoworkspace, 9
bind = $mainMod SHIFT, 0, tilescript-hypr, movetoworkspace, 10

bind = $mainMod, q, tilescript-hypr, closewindow
bind = $mainMod, f, tilescript-hypr, fullscreen
```

## Documentación

- [`docs/config.md`](docs/config.md)
- [`docs/hyprland.md`](docs/hyprland.md)
- [`docs/jsx.md`](docs/jsx.md)
- [`docs/css.md`](docs/css.md)
- [`docs/css-lsp.md`](docs/css-lsp.md)
- [`docs/development.md`](docs/development.md)
- [`docs/playground.md`](docs/playground.md)

## Dependencias Principales

`tilescript` se construye sobre un pequeño conjunto de bibliotecas fundamentales:

- [`stylo`](https://crates.io/crates/stylo), [`cssparser`](https://crates.io/crates/cssparser) y [`selectors`](https://crates.io/crates/selectors) para el análisis de CSS y el mecanismo de selectores
- [`taffy`](https://crates.io/crates/taffy) para el cálculo de disposiciones
- [`oxc`](https://crates.io/crates/oxc) y [`oxc_resolver`](https://crates.io/crates/oxc_resolver) para el análisis de JS/TS/TSX y el análisis del grafo de módulos
- [`rquickjs`](https://crates.io/crates/rquickjs) para el runtime nativo de JS
- [`mlua`](https://crates.io/crates/mlua) para el runtime nativo de Lua
- [`leptos`](https://crates.io/crates/leptos) y [`leptos_router`](https://crates.io/crates/leptos_router) para la interfaz de usuario del entorno de pruebas en el navegador
- [`wasm-bindgen`](https://crates.io/crates/wasm-bindgen) y [`web-sys`](https://crates.io/crates/web-sys) para la interoperabilidad con el navegador/WASM
- [`monaco-editor`](https://www.npmjs.com/package/monaco-editor), [`monaco-vim`](https://www.npmjs.com/package/monaco-vim) y [`wasmoon`](https://www.npmjs.com/package/wasmoon) para el editor del entorno de pruebas y la ejecución de Lua en el navegador
