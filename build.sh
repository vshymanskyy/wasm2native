export CC=${CC:="zig cc"}

JOBS=$((`nproc`+1))

rm -rf ./src/wasm/

#wasm-opt -Os --converge --strip-debug
#wasm2c --no-debug-names "$1" -o wasi-app.c
#mv wasi-app.* ./src

mkdir -p ./src/wasm/
./deps/w2c2/w2c2 -j $JOBS -f 250 -o ./src/wasm/ "$1"

OPT_FLAGS="-O3 -flto=thin -fomit-frame-pointer -fno-stack-protector -march=native"
SRCS="$(ls ./src/wasm/*.c) src/wasi-main.c"
DEPS="-Ideps/w2c2/ -Ibuild/_deps/uvwasi-src/include -Lbuild/_deps/libuv-build -Lbuild/_deps/uvwasi-build -luvwasi_a -luv_a -lpthread -ldl -lm"



fn_out=$(basename -- "$1")
fn_out="${fn_out%%.*}.elf"

rm -f ./${fn_out}
$CC $OPT_FLAGS $SRCS $DEPS -o ./${fn_out}
