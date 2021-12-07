# [WIP] wasm2native

Turn WASI apps into native executables

## How it works

```log
app.wasm + wasm2c + compiler (Zig) + uvwasi + libuv = native app
```

## Prerequisites

- CMake
- Native toolchain (GCC/Clang/MSVC)
- [WABT](https://github.com/WebAssembly/wabt/releases/latest)
- Optional: [Zig](https://github.com/ziglang/zig/releases/latest)

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

# Build app:
./build.sh ./examples/coremark.wasm

CC=gcc ./build.sh ./examples/coremark.wasm
CC=clang-12 ./build.sh ./examples/coremark.wasm
```

## TODO

- [ ] Bind all WASI imports (currently it's a bare minimum)
- [ ] Convert main script to Zig
- [ ] Windows support
- [ ] `CMake+Zig` as the primary toolchain
    - [ ] Use Zig to build `libuv`, `uvwasi`
    - [ ] Ship `wasm2c` as a default app, build it
- [ ] Fix cross-compilation
- [ ] Improve build speed for big apps. Split `wasm2c` output?
