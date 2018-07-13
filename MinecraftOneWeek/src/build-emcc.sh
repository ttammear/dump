emcc main.cpp application.cpp Renderer/OpenGL/renderer-gles.cpp game.cpp Renderer/mesh.cpp Renderer/material.cpp transform.cpp camera.cpp Renderer/texture.cpp GameWorld/chunk.cpp Renderer/renderevents.cpp GameWorld/world.cpp GameWorld/chunkmanager.cpp GameWorld/worldgenerator.cpp Maths/perlin.cpp Player/player.cpp Player/inventory.cpp Player/gui.cpp -O2 -lGLEW -lGL -std=c++11 -s WASM=1 -s USE_SDL=2 -s USE_WEBGL2=1 -s ALLOW_MEMORY_GROWTH=1 --embed-file ../Resources/mcatlas.png@Resources/mcatlas.png --shell-file ./shell_minimal.html -o ../game.html

if [ $1 == 'run' ]
then
sh -c 'cd .. && ./game.out'
fi
