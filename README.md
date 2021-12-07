# wasm2native

Turn WASI apps into native executables

## Usage examples

```sh
# Single step:
CC="clang-12" ./build-full.sh ./examples/coremark.wasm

# Cross-compile with Zig CC: x86_64-windows-gnu, x86_64-linux-gnu, x86_64-macos-gnu
CC="zig cc -Dtarget=x86_64-linux-musl" ./build-full.sh ./examples/coremark.wasm

# Two-step:
./build-libs.sh
./build.sh ./examples/coremark.wasm

CC=gcc ./build.sh ./examples/coremark.wasm
CC=clang-12 ./build.sh ./examples/coremark.wasm
```

## TODO

- [ ] Use Zig to build `libuv`, `uvwasi`
- [ ] Convert main script to Zig
- [ ] Ship `wasm2c` as a default app, build it
- [ ] Improve build speed for big apps. Split `wasm2c` output?
