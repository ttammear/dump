emcc -O2 src/main.c src/nes.c src/cpu.c src/ppu.c src/apu.c src/mapper.c src/platform.c -s USE_SDL=2 -s WASM=1 -Iinclude -o emu.html --preload-file Dr.\ Mario\ \(Japan\,\ USA\).nes --shell-file ./shell_minimal.html --profiling
