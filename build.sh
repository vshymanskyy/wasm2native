export CC=${CC:="zig cc"}

rm -f wasi-app.*

wasm2c "$1" -o wasi-app.c

OPT_FLAGS="-O3 -flto -fomit-frame-pointer -fno-stack-protector -march=native"
SRCS="wasi-app.c wasi-main.c wasm-rt-impl.c"
DEPS="-Ideps/uvwasi/include -Lbuild/_deps/libuv-build -Lbuild/_deps/uvwasi-build -luvwasi_a -luv_a -lpthread -ldl -lm"



fn_out=$(basename -- "$1")
fn_out="${fn_out%%.*}.elf"

rm -f ./${fn_out}
$CC $OPT_FLAGS $SRCS $DEPS -o ./${fn_out}
