cmake_minimum_required(VERSION 3.20)
project(HairSimulation LANGUAGES C CXX)   # C is needed for glad's gl.c

# ---- Compiler settings -------------------------------------------------------
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)             # use -std=c++20, not -std=gnu++20
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)     # writes compile_commands.json for clangd

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Debug)
endif()

# ---- Third-party libraries ---------------------------------------------------
find_package(glfw3 3.3 REQUIRED)          # installed via Homebrew

add_library(glad STATIC extern/glad/src/gl.c)
target_include_directories(glad PUBLIC extern/glad/include)

# ---- Renderer (provided, independent of the sim code) ------------------------
add_library(renderer STATIC render/Renderer.cpp)
target_include_directories(renderer PUBLIC render)
target_link_libraries(renderer PUBLIC glfw PRIVATE glad)
target_compile_options(renderer PRIVATE -Wall -Wextra -Wpedantic)

# ---- Your program ------------------------------------------------------------
add_executable(hair_sim
    main.cpp
)

target_include_directories(hair_sim PRIVATE render)   # lets you #include "Solver.h"

target_link_libraries(hair_sim PRIVATE renderer glad glfw)
target_compile_options(hair_sim PRIVATE -Wall -Wextra -Wpedantic)
