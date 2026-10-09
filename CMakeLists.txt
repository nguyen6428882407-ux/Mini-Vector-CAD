cmake_minimum_required(VERSION 3.15)
project(MiniVectorCAD CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include_directories(include)

file(GLOB_RECURSE SOURCES
    "src/core/*.cpp"
    "src/shapes/*.cpp"
    "src/canvas/*.cpp"
    "src/commands/*.cpp"
    "src/io/*.cpp"
    "src/controller/*.cpp"
    "src/main.cpp"
)

add_executable(MiniVectorCAD ${SOURCES})

if (MSVC)
    target_compile_options(MiniVectorCAD PRIVATE /W4)
else()
    target_compile_options(MiniVectorCAD PRIVATE -Wall -Wextra -pedantic)
endif()
