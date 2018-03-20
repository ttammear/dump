cd "$(dirname "$0")"
set -e

git submodule update --recursive

STARTTIME=$(date +%s%N)
clang ./AikePlatform/linux_main.cpp -D AIKE_DEBUG -fno-exceptions -std=c++11 -ggdb -lGL -lm -lX11 -ldl -linput -ludev -rdynamic -pthread -Wshadow -o ../ExpEngineBuild/Engine.out

echo 'compiling...'
clang -I./AikePlatform unitybuild.c -shared -Wno-unused-parameter -Wshadow -std=c11 -gdwarf-4 -fPIC -o ../ExpEngineBuild/libAike.so -O0 -ffast-math -lm -lGL -pthread -D_DEBUG --no-undefined

echo 'done... deploying...'

if [ $1 == 'run' ]
then
gnome-terminal -- DRI_PRIME=1 ../ExpEngineBuild/Engine.out
fi
