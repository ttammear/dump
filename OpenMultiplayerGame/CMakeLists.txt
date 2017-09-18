cmake_minimum_required (VERSION 2.6)
project (OpenMultiplayerGame)

file(GLOB_RECURSE sources src/*.cpp src/*.h)

add_executable(server ${sources})

target_compile_options(server PUBLIC -std=c++11 -ggdb)

target_include_directories(server PUBLIC src)

target_link_libraries(server sfml-window sfml-system GLEW GL dl enet)

install(TARGETS server DESTINATION server_destination)

set(CPACK_PACKAGE_NAME "OMG Server")
set(CPACK_PACKAGE_VERSION "0.0.1")

set(CPACK_MONOLITHIC_INSTALL 1)

include(CPack)
