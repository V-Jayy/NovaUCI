cmake_minimum_required(VERSION 3.10)
project(Nova LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Shorten object paths on Windows to avoid long file name errors
if(MSVC)
    set(CMAKE_OBJECT_PATH_MAX 240)
endif()

add_executable(nova
    src/main.cpp
    src/uci.cpp
    src/board.cpp
    src/search.cpp
)
