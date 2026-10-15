cmake_minimum_required(VERSION 3.15)
project(MiniVectorCAD CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include_directories(include)

file(GLOB SOURCES
    "src/shapes/*.cpp"
    "src/shapes/core/*.cpp"
    "src/canvas/*.cpp"
    "src/commands/*.cpp"
    "src/io/*.cpp"
    "src/controller/*.cpp"
)

add_library(MiniVectorCADCore ${SOURCES})
target_include_directories(MiniVectorCADCore PUBLIC include)

if (MSVC)
    target_compile_options(MiniVectorCADCore PRIVATE /W4)
else()
    target_compile_options(MiniVectorCADCore PRIVATE -Wall -Wextra -pedantic)
endif()

add_executable(MiniVectorCAD src/main.cpp)
target_link_libraries(MiniVectorCAD PRIVATE MiniVectorCADCore)

include(CTest)
if (BUILD_TESTING)
    foreach(test_name IN ITEMS test_shapes test_task9 test_undo_redo)
        add_executable(${test_name} tests/${test_name}.cpp)
        target_link_libraries(${test_name} PRIVATE MiniVectorCADCore)
        add_test(NAME ${test_name} COMMAND ${test_name})
    endforeach()
endif()

