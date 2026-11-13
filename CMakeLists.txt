cmake_minimum_required(VERSION 3.16)
project(bminor LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_executable(bminor src/main.cpp)
if(MSVC)
    target_compile_options(bminor PRIVATE /W4)
else()
    target_compile_options(bminor PRIVATE -Wall -Wextra -Wpedantic)
endif()
