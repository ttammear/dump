set -e # break on error

CC="wasm-clang"
LINKER="wasm-lld"

$CC -O2 --target=wasm32-unknown-unknown-wasm src/main_webgl.c src/nes.c src/cpu.c src/ppu.c src/apu.c src/mapper.c -Iinclude -I../wasmtest/libc/include -Wno-incompatible-library-redeclaration -D__WASM__ -DUSE_WEBGL_2 -c

$LINKER -flavor wasm main_webgl.o nes.o cpu.o ppu.o apu.o mapper.o mylibc.a -o nes.wasm -entry main --allow-undefined -export callFunction 

cp nes.wasm /var/www/html/nesemu/

#gcc src/main.c src/nes.c src/cpu.c src/ppu.c src/apu.c src/mapper.c src/platform.c -lGL -lSDL2 -lGLU -lGLEW -lm -lstdc++ -ggdb -Iinclude
if [ $1 == 'run' ]
then
./a.out
fi
