set -e # break on error
gcc src/main.c src/nes.c src/cpu.c src/ppu.c src/apu.c src/mapper.c src/platform.c -lGL -lSDL2 -lGLU -lGLEW -lm -lstdc++ -ggdb -Iinclude
if [ $1 == 'run' ]
then
./a.out
fi
