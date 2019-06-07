cd "$(dirname "$0")"

STARTTIME=$(date +%s%N)


cd AikePlatform
git fetch
git reset --hard origin/master
git submodule update --recursive
cd ..

set -e

CFLAGS='-Wno-multichar -pthread -std=c11 -ffast-math -O0 -g3 -DAIKE_X86'
LFLAGS=''
COMFLAGS=''
CC='gcc'
LINKER='ld'

# compile platform
if [ "$1" == 'platform' ]
then
PLATTIME=$(date +%s%N)
$CC ./AikePlatform/linux_main.c $CFLAGS $COMFLAGS -D AIKE_DEBUG -D AIKE_AIO -lm -lX11 -ldl -lGL -linput -ludev -rdynamic -lrt -o ../ExpEngineBuild/Engine.out
echo "compiling platform $(($(($(date +%s%N) - $PLATTIME))/1000000))ms"
fi

# compile engine
ENGTIME=$(date +%s%N)
$CC -I./AikePlatform -I./libs/libcoro unitybuild.c $CFLAGS $COMFLAGS -fPIC -c -o ./obj/engine.o -D_DEBUG -D AIKE_AIO
echo "compiling engine $(($(($(date +%s%N) - $ENGTIME))/1000000))ms"

#compile libs
if [ "$1" == 'libs' ]
then
LIBTIME=$(date +%s%N)
#clang -c -O0 -gdwarf-4 -fPIC libs_static.c -o ./obj/libs.o
#precompile headers
#clang -cc1 -std=c11 -pthread -target-cpu x86-64 -pic-level 2 -ffp-contract=fast -ffast-math -ffinite-math-only -menable-unsafe-fp-math -I/usr/lib/llvm-5.0/lib/clang/5.0.2/include -I/usr/include/x86_64-linux-gnu -I/usr/include libs.h -emit-pch -o libs.h.pch
$CC -shared -O2 -gdwarf-4 -fPIC libs_static.c -o ../ExpEngineBuild/libAikeDeps.so
echo "compiling libs $(($(($(date +%s%N) - $LIBTIME))/1000000))ms"
fi

LINKTIME=$(date +%s%N)
$LINKER ./obj/engine.o ../ExpEngineBuild/libAikeDeps.so ./libs/libcoro/libcoro.a -shared -lenet -lm -fPIC $LFLAGS $COMFLAGS -g -o ../ExpEngineBuild/libAike.so
echo "linking engine $(($(($(date +%s%N) - $LINKTIME))/1000000))ms"

CURTIME=$(date +%s%N)
echo "total $(($(($CURTIME - $STARTTIME))/1000000))ms"

cp ./Managed/bin/Debug/netstandard2.0/Managed.{dll,pdb} ../ExpEngineBuild/

#run if run set
if [ "$1" == 'run' ]
then
#vblank_mode=0

gnome-terminal -- ../ExpEngineBuild/Engine.out

fi
