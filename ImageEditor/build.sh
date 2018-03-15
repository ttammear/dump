set -e
STARTTIME=$(date +%s%N)
clang ./AikePlatform/linux_main.cpp -D AIKE_DEBUG -fno-exceptions -std=c++11 -ggdb -lGL -lm -lX11 -ldl -linput -ludev -rdynamic -pthread -o ../ImageEditorBuild/ImageEditor.out

clang -I./AikePlatform unitybuild.cpp -shared -std=c++11 -O0 -gdwarf-4 -fPIC -lm -lGL -pthread -D AIKE_DEBUG -fno-exceptions --no-undefined -o ../ImageEditorBuild/libAike.so
CURTIME=$(date +%s%N)
echo "Compiled in $(($(($CURTIME - $STARTTIME))/1000000))ms"

if [ $1 == 'run' ]
then
../ImageEditorBuild/ImageEditor.out
fi
