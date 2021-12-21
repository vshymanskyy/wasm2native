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
CC="clang-12" ./build-full.sh ./examples/coremark.wasm

# Cross-compile with Zig CC: x86_64-windows-gnu, x86_64-linux-gnu, x86_64-macos-gnu
CC="zig cc -Dtarget=x86_64-linux-musl" ./build-full.sh ./examples/coremark.wasm

# Cross-compilation to other architectures:
CC="zig cc -Dtarget=mips-linux-musl" ./build-full.sh ./examples/hello.wasm
qemu-mips-static hello.elf
Hello from WebAssembly!

### Two-step mode (faster)

# Build libs (needed only once):
./build-libs.sh

# Build app (defaults to Zig):
./build.sh ./examples/coremark.wasm

# Specify another compiler:
CC=gcc ./build.sh ./examples/coremark.wasm
CC=clang-12 ./build.sh ./examples/coremark.wasm
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
    - [ ] Fix cross-compilation
    - [ ] Big-endian target support
- [ ] Get rid of mandatory CMake dependency (write build script for `libuv` and `uvwasi` for major platforms)
- [ ] Improve build speed for big apps. Split `wasm2c` output?

### License
This project is released under The MIT License (MIT)

<!-- wasm2exe wasm2elf compiler toolchain -->
