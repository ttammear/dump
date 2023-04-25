cd "$(dirname "$0")"

STARTTIME=$(date +%s%N)


cd AikePlatform
git fetch
git reset --hard origin/master
git submodule update --recursive
cd ..

set -e

CFLAGS='-Wno-multichar -pthread -std=c11 -ffast-math -O0 -ggdb3 -DAIKE_X86 -DLINUX_USE_KERNEL_AIO'
LFLAGS=''
COMFLAGS=''
CC='clang'
CXX='clang++-7'
LINKER='clang'
AR='ar'

cd GameserverMsg
python3 ./codegen.py
cd ..

# compile platform
if [ "$1" == 'platform' ]
then
PLATTIME=$(date +%s%N)
$CC ./AikePlatform/linux_main.c $CFLAGS $COMFLAGS -D AIKE_DEBUG -D AIKE_AIO -lm -lX11 -ldl -lGL -linput -ludev -rdynamic -lrt -o ../ExpEngineBuild/Engine.out
echo "compiling platform $(($(($(date +%s%N) - $PLATTIME))/1000000))ms"
fi

# compile engine (game client)
CLIENTTIME=$(date +%s%N)
$CC -I./AikePlatform -I./libs/libcoro client_unitybuild.c $CFLAGS $COMFLAGS -fPIC -c -o ./obj/engine.o -D_DEBUG -D AIKE_AIO -D TESS_CLIENT 

$CXX -c -O0 -fPIC -stdlib=libc++ -IPhysX/physx/include -IPhysX/pxshared/include physics_system_physx.cpp -IPhysX/physx/source/foundation/include  -D_DEBUG -DTESS_CLIENT -o ./obj/physics.o

$AR rcs ./obj/libtessphysics.a ./obj/physics.o

echo "compiling game client $(($(($(date +%s%N) - $CLIENTTIME))/1000000))ms"

# compile server
SERVERTIME=$(date +%s%N)
$CC -I./AikePlatform -I./libs/libcoro server_unitybuild.c $CFLAGS $COMFLAGS -fPIC -c -o ./obj/gameserver.o -D_DEBUG -D AIKE_AIO -D TESS_SERVER
echo "compiling game server $(($(($(date +%s%N) - $SERVERTIME))/1000000))ms"

#compile libs
if [ "$1" == 'libs' ]
then
LIBTIME=$(date +%s%N)
#clang -c -O0 -gdwarf-4 -fPIC libs_static.c -o ./obj/libs.o
#precompile headers
#clang -cc1 -std=c11 -pthread -target-cpu x86-64 -pic-level 2 -ffp-contract=fast -ffast-math -ffinite-math-only -menable-unsafe-fp-math -I/usr/lib/llvm-5.0/lib/clang/5.0.2/include -I/usr/include/x86_64-linux-gnu -I/usr/include libs.h -emit-pch -o libs.h.pch
$CC -shared -O2 -ggdb3 -fPIC libs_static.c -o ../ExpEngineBuild/libAikeDeps.so
echo "compiling libs $(($(($(date +%s%N) - $LIBTIME))/1000000))ms"
fi

PHYSXLIBS='-LPhysX/physx/bin/linux.clang/release -lPhysX_static_64 -lSnippetUtils_static_64 -lPhysXCooking_static_64 -lPhysXPvdSDK_static_64 -lPhysXCharacterKinematic_static_64 -lPhysXExtensions_static_64 -lPhysXVehicle_static_64 -lPhysXCommon_static_64 -lPhysXFoundation_static_64'

LINKTIME=$(date +%s%N)
$LINKER ./obj/engine.o ./obj/physics.o ../ExpEngineBuild/libAikeDeps.so ./libs/libcoro/libcoro.a -shared $PHYSXLIBS -lenet -lm  -lpthread -fno-threadsafe-statics -fPIC $LFLAGS $COMFLAGS -g -o ../ExpEngineBuild/libAike.so
echo "linking engine $(($(($(date +%s%N) - $LINKTIME))/1000000))ms"

SERVERLINKTIME=$(date +%s%N)
$LINKER ./obj/gameserver.o ./obj/physics.o ../ExpEngineBuild/libAikeDeps.so ./libs/libcoro/libcoro.a -shared $PHYSXLIBS -lenet -lm -lpthread -fno-threadsafe-statics -fPIC $LFLAGS $COMFLAGS -g -o ../ExpEngineBuild/libAikeServer.so
echo "linking server $(($(($(date +%s%N) - $SERVERLINKTIME))/1000000))ms"

CURTIME=$(date +%s%N)
echo "total $(($(($CURTIME - $STARTTIME))/1000000))ms"

cp ./Managed/bin/Debug/netstandard2.0/Managed.{dll,pdb} ../ExpEngineBuild/

#run if run set
if [ "$1" == 'run' ]
then
#vblank_mode=0

gnome-terminal -- ../ExpEngineBuild/Engine.out

fi
