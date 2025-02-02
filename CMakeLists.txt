cmake_minimum_required(VERSION 3.31)

project(chip-8 LANGUAGES CXX)

add_executable(chip-8 src/main.cpp src/chip8.cpp)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)


target_compile_features(chip-8 PUBLIC cxx_std_20)
