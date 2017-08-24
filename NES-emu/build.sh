set -e # break on error
gcc main.c nes.c cpu.c ppu.c apu.c mapper.c platform.c -lGL -lSDL2 -lGLU -lGLEW -lm -lstdc++ -ggdb
if [ $1 == 'run' ]
then
./a.out
fi
