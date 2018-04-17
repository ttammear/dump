cd "$(dirname "$0")"

STARTTIME=$(date +%s%N)

cd AikePlatform
git fetch
git reset --hard origin/master
git submodule update --recursive
cd ..

set -e

CFLAGS='-Wshadow -pthread -std=c11 -ffast-math -O0 -ggdb3 -DAIKE_X86'
LFLAGS=''
COMFLAGS=''

echo 'compiling platform...'
clang ./AikePlatform/linux_main.c $CFLAGS $COMFLAGS -D AIKE_DEBUG -D AIKE_AIO -lGL -lm -lX11 -ldl -linput -ludev -rdynamic -o ../ExpEngineBuild/Engine.out

echo 'compiling engine...'
clang -I./AikePlatform unitybuild.c $CFLAGS $COMFLAGS -fPIC -c -o ./obj/engine.o -D_DEBUG -D AIKE_AIO

#clang -c -O2 -fPIC libs_static.c -o ./obj/libs.o

echo 'linking engine...'
clang ./obj/engine.o ./obj/libs.o -shared -lm -lGL -fPIC $LFLAGS $COMFLAGS -g -o ../ExpEngineBuild/libAike.so

CURTIME=$(date +%s%N)
echo "done in $(($(($CURTIME - $STARTTIME))/1000000))ms!"

if [ $1 == 'run' ]
then
#vblank_mode=0
gnome-terminal -- ../ExpEngineBuild/Engine.out
fi
