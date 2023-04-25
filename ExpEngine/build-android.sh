export PATH=$PATH:/home/ttammear/src/android-toolchain/bin

set -e

echo 'compiling platform...'

arm-linux-androideabi-clang ./AikePlatform/android_platform.c -pthread -std=c11 -fPIC -ffast-math -O0 -DAIKE_ANDROID -DAIKE_DEBUG -DAIKE_AIO -DAIKE_NO_OPENGL -c -o ./obj/androidplatform.o

arm-linux-androideabi-clang -I./AikePlatform redscreen.c -pthread -std=c11 -fPIC -ffast-math -O0 -c -D_DEBUG2 -DAIKE_AIO -DAIKE_NO_OPENGL -DAIKE_ANDROID -o ./obj/androidengine.o 

#arm-linux-androideabi-clang -c -O2 -fPIC libs_static.c -o ./obj/androidlibs.o

#arm-linux-androideabi-clang ./obj/androidengine.o ./obj/androidlibs.o ./androidplatform.o -shared -lm -fPIC -g -o ../ExpEngineAndroid/libAike.so
arm-linux-androideabi-ar -rcs ../ExpEngineAndroid/libEngine.a ./obj/androidengine.o ./obj/androidlibs.o ./obj/androidplatform.o
