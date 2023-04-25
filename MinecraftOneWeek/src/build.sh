clang++ main.cpp application.cpp Renderer/OpenGL/renderer.cpp game.cpp Renderer/mesh.cpp Renderer/material.cpp transform.cpp camera.cpp Renderer/texture.cpp GameWorld/chunk.cpp Renderer/renderevents.cpp GameWorld/world.cpp GameWorld/chunkmanager.cpp GameWorld/worldgenerator.cpp Maths/perlin.cpp Player/player.cpp Player/inventory.cpp Player/gui.cpp -O3 -lSDL2 -lGLEW -lGL -ggdb -std=c++11 -o ../game.out

if [ $1 == 'run' ]
then
sh -c 'cd .. && ./game.out'
fi
