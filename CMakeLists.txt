cmake_minimum_required(VERSION 3.0)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")

project(game)

add_executable(game
    src/main.cpp)

target_link_libraries(game glew glfw /System/Library/Frameworks/OpenGL.framework)
