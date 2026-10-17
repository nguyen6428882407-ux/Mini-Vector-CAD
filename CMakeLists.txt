cmake_minimum_required(VERSION 3.15)

project(MiniVectorCAD LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

file(GLOB_RECURSE MINIVECTORCAD_CORE_SOURCES CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/src/auth/*.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/canvas/*.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/commands/*.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/controller/*.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/io/*.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/shapes/*.cpp"
)

add_library(MiniVectorCADCore STATIC ${MINIVECTORCAD_CORE_SOURCES})
target_include_directories(MiniVectorCADCore PUBLIC
    "${CMAKE_CURRENT_SOURCE_DIR}/include"
)

if(MSVC)
    target_compile_options(MiniVectorCADCore PRIVATE /W4)
else()
    target_compile_options(MiniVectorCADCore PRIVATE -Wall -Wextra -pedantic)
endif()

add_executable(MiniVectorCAD src/main.cpp)
target_link_libraries(MiniVectorCAD PRIVATE MiniVectorCADCore)

add_executable(MiniVectorCADServer
    src/web/server_main.cpp
    src/web/ProjectServer.cpp
)
target_link_libraries(MiniVectorCADServer PRIVATE MiniVectorCADCore)
if(WIN32)
    target_link_libraries(MiniVectorCADServer PRIVATE ws2_32)
endif()

include(CTest)
if(BUILD_TESTING)
    foreach(test_name IN ITEMS test_shapes test_task9 test_undo_redo test_auth)
        add_executable(${test_name} "tests/${test_name}.cpp")
        target_link_libraries(${test_name} PRIVATE MiniVectorCADCore)
        add_test(NAME ${test_name} COMMAND ${test_name})
    endforeach()
endif()


