#g++ main.cpp application.cpp Renderer/OpenGL/renderer.cpp game.cpp Renderer/mesh.cpp Renderer/material.cpp transform.cpp camera.cpp Renderer/texture.cpp GameWorld/chunk.cpp Renderer/renderevents.cpp GameWorld/world.cpp GameWorld/chunkmanager.cpp GameWorld/worldgenerator.cpp Maths/perlin.cpp Player/player.cpp Player/inventory.cpp Player/gui.cpp Networking/client.cpp Networking/server.cpp Dotnet/dotnet.cpp -lsfml-window -lsfml-system -lGLEW -lGL -ggdb -ldl -lenet -std=gnu++0x -o ../game.out
cmake .
make -j5

if [ $1 == 'run' ]
then
gnome-terminal -e './server' --working-directory '/keep/Projects/OpenMultiplayerGame'
fi
