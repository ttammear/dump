cd "$(dirname "$0")"

STARTTIME=$(date +%s%N)

cd AikePlatform
git fetch
git reset --hard origin/master
git submodule update --recursive
cd ..

set -e

echo 'compiling platform...'
clang ./AikePlatform/linux_main.cpp -D AIKE_DEBUG -fno-exceptions -std=c++11 -ggdb -lGL -lm -lX11 -ldl -linput -ludev -rdynamic -pthread -Wshadow -o ../ExpEngineBuild/Engine.out

echo 'compiling engine...'
clang -I./AikePlatform unitybuild.c -Wshadow -std=c11 -ggdb3 -fPIC -c -o ./obj/engine.o -O0 -ffast-math -pthread -D_DEBUG2

#clang -c -O2 -fPIC libs_static.c -o ./obj/libs.o

echo 'linking engine...'
gold ./obj/engine.o ./obj/libs.o -shared -lm -lGL -fPIC -g -o ../ExpEngineBuild/libAike.so

CURTIME=$(date +%s%N)
echo "done in $(($(($CURTIME - $STARTTIME))/1000000))ms!"

if [ $1 == 'run' ]
then
gnome-terminal -- DRI_PRIME=1 ../ExpEngineBuild/Engine.out
fi
