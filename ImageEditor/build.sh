set -e
STARTTIME=$(date +%s%N)
clang linux_main.cpp -D AIKE_DEBUG -fno-exceptions -std=c++11 -ggdb -lGL -lm -lX11 -ldl -lstdc++ -rdynamic -pthread -o ../ImageEditorBuild/ImageEditor.out
#clang unitybuild.cpp -D AIKE_DEBUG -fno-exceptions -std=c++11 -ggdb -lGL -lm -lstdc++ -o ../ImageEditorBuild/ImageEditor.out

clang unitybuild.cpp -shared -std=c++11 -ggdb -fPIC -lm -lGL -lstdc++ -pthread -D AIKE_DEBUG -fno-exceptions --no-undefined -o ../ImageEditorBuild/libAike.so
CURTIME=$(date +%s%N)
echo "Compiled in $(($(($CURTIME - $STARTTIME))/1000000))ms"

if [ $1 == 'run' ]
then
../ImageEditorBuild/ImageEditor.out
fi
