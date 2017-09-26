cmake_minimum_required (VERSION 2.6)
project (OpenMultiplayerGame)

add_subdirectory(bullet3)

file(GLOB_RECURSE sources src/*.cpp src/*.h)

add_executable(server ${sources})
add_executable(client ${sources})

target_compile_options(server PUBLIC -std=c++11 -ggdb -D SERVER)
target_link_libraries(server SDL2 GLEW GL dl enet pthread)
target_link_libraries(server BulletDynamics BulletInverseDynamics BulletCollision LinearMath)
target_include_directories(server PUBLIC src)
target_include_directories(server PUBLIC src/imgui)
target_include_directories(server PUBLIC bullet3/src)
install(TARGETS server DESTINATION server_destination)

target_compile_options(client PUBLIC -std=c++11 -ggdb -D CLIENT)
target_link_libraries(client SDL2 GLEW GL dl enet pthread)
target_link_libraries(client BulletDynamics BulletInverseDynamics BulletCollision LinearMath)
target_include_directories(client PUBLIC src)
target_include_directories(client PUBLIC src/imgui)
target_include_directories(client PUBLIC bullet3/src)
install(TARGETS client DESTINATION client_destination)

set(CPACK_PACKAGE_NAME "OMG Server")
set(CPACK_PACKAGE_VERSION "0.0.1")

set(CPACK_MONOLITHIC_INSTALL 1)

include(CPack)
