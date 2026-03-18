# Development

## Editor / LSP Setup

`hypreact` uses CMake, vendored dependencies, and `pkg-config`-discovered system headers. C/C++ language servers such as `clangd` need `compile_commands.json` to see the real include paths, compile definitions, and flags.

Generate it with:

```sh
cmake -B build -DCMAKE_EXPORT_COMPILE_COMMANDS=ON
ln -sf build/compile_commands.json compile_commands.json
```

If your editor already had the repository open, restart the language server after generating the file.

## Local Build

Configure and build with:

```sh
cmake -B build -DCMAKE_EXPORT_COMPILE_COMMANDS=ON
cmake --build build -j
```

The plugin shared object is produced at:

```sh
build/libhypreact.so
```

For local runtime experiments, `examples/config/` contains a minimal config tree and example Hyprland plugin block.

## Dependencies

`hypreact` currently expects:

- vendored `QuickJS` under `third_party/quickjs`
- vendored `Yoga` under `third_party/yoga`
- system-installed `libcss`, `libparserutils`, and `libwapcaplet`
- system-installed Hyprland development headers and their transitive native dependencies
- `esbuild` available on `PATH` for layout/module compilation

Example Arch packages:

```sh
sudo pacman -S hyprland pixman libcss libparserutils libwapcaplet pkgconf cmake clang esbuild
```
