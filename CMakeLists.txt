cmake_minimum_required(VERSION 3.16)
project(Tetris)

set(CMAKE_CXX_STANDARD 20)

# 不指定类型时默认 Debug（带 -g 调试符号），方便 CLion / cmake debugging
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Debug)
endif()

# Debug 下关优化，断点/单步才准确
if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    add_compile_options(-g -O0)
endif()

if (WIN32)
    link_libraries(winmm)   # PlaySound 背景音乐
endif()

add_executable(Tetris main.cpp
        Game.cpp
        Game.h
        Console.cpp
        Console.h
        Cube.cpp
        Cube.h
        Board.cpp
        Board.h
        Hud.cpp
        Hud.h
        GameConfig.h
        Sound.cpp
        Sound.h)
