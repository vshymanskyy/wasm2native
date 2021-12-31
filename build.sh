export CC

#rm -rf ./build
#rm -rf ./src/wasm

#wasm2c "$1" -o wasi-app.c
#mv wasi-app.* ./src

JOBS=$((`nproc`+1))

mkdir -p ./src/wasm
./deps/w2c2/w2c2 -j $JOBS -f 250 -o ./src/wasm/ "$1"

mkdir -p build
cd build
cmake ..
cmake --build . -j $JOBS
cd ..

fn_out=$(basename -- "$1")
fn_out="${fn_out%%.*}.elf"

rm -f ./${fn_out}
cp ./build/app.out ./${fn_out}
