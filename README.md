# [WIP] wasm2native

Turn WASI apps into native executables

## How it works

The approach is similar to [`WasmBoxC`](https://kripken.github.io/blog/wasm/2020/07/27/wasmboxc.html) or [`RLBox`](https://hacks.mozilla.org/2020/02/securing-firefox-with-webassembly/):

```log
app.wasm + wasm2c + compiler (Zig) + uvwasi + libuv = native app
```

## Prerequisites

- CMake
- Native toolchain (GCC/Clang/MSVC). Optional: [Zig](https://github.com/ziglang/zig/releases/latest)
- [WABT](https://github.com/WebAssembly/wabt/releases/latest)

## Usage examples

```sh
# Single step:
export CC="clang-12"
export LDFLAGS="-fuse-ld=lld"
./build.sh ./examples/coremark.wasm

# Cross-compile with Zig CC: x86_64-windows-gnu, x86_64-linux-gnu, x86_64-macos-gnu
CC="zig cc -target x86_64-linux-musl" ./build.sh ./examples/coremark.wasm

# Cross-compilation to other architectures
# For full targets list: zig targets | jq .libc
CC="zig cc -target aarch64-linux-musl" ./build.sh ./examples/hello.wasm
qemu-aarch64-static hello.elf
Hello from WebAssembly!
```

**Note:** this tool can be used for building `WASI` apps, not `emscripten`-generated `wasm+js` output.

## Coremark 1.0 results

Intel(R) Core(TM) i5-10400 CPU @ 2.90GHz, single-thread:
- Native: **32475**
- wasm2c: **27400** (84% of native)

## TODO

- [x] Bind all WASI imports
    - [x] `wasi_snapshot_preview1`
    - [x] `wasi_unstable` compatibility layer
- [ ] Convert main script to `zig`
    - [ ] Allow setting `CFLAGS`, overriding optimization flags
- [ ] Windows host support
- [ ] CI via Github Actions
- [ ] `Zig` as the primary toolchain
    - [ ] Use Zig to build `libuv`, `uvwasi`
    - [x] Ship `wasm2c.wasm` as an example app ;)
    - [x] Fix cross-compilation
    - [ ] Big-endian target support
- [ ] Get rid of mandatory CMake dependency (write build script for `libuv` and `uvwasi` for major platforms)
- [ ] Improve build speed for big apps. Split `wasm2c` output?

### License
This project is released under The MIT License (MIT)

<!-- wasm2exe wasm2elf compiler toolchain -->
