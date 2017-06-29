set -e # break on error
gcc main.cpp nes.cpp cpu.cpp ppu.cpp apu.cpp mapper.cpp platform.c -lGL -lSDL2 -lGLU -lGLEW -lm
if [ $1 == 'run' ]
then
./a.out
fi
