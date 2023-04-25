set -e

CC='wasm-clang'
LINKER='wasm-lld'

$CC --target=wasm32-unknown-unkown-wasm physics.c bind_js.c -c -I/keep/Projects/wasmtest/libc/include -Wextra -Wno-unused-parameter -Wno-incompatible-library-redeclaration -g

$LINKER -flavor wasm mylibc.a physics.o bind_js.o -o physics.wasm --no-entry --allow-undefined --export-all

#source ~/emsdk/emsdk_env.sh
#emcc -Os ./physics.c -s SIDE_MODULE=1 -s WASM=1 -o physics.wasm -s EXPORTED_FUNCTIONS="['_malloc','_calloc']" -s LINKABLE=1 -s EXPORT_ALL=1 r/mylibc.a

echo 'deploying...'

cp ./index.html /var/www/html/physics/
cp ./physics.wasm /var/www/html/physics/
