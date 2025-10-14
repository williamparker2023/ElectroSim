cmake_minimum_required(VERSION 3.20)

# Project name and language standard
project(ElectroSim LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Default to Release if nothing is specified
if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type" FORCE)
endif()

# Output folders (nice and tidy)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

# Warnings (per compiler)
if(MSVC)
  add_compile_options(/W4 /permissive- /Zc:__cplusplus)
else()
  add_compile_options(-Wall -Wextra -Wpedantic)
endif()

# --- Executable ---
# Put your sources in src/ (start with src/main.cpp)
file(GLOB_RECURSE ELECTROSIM_SOURCES CONFIGURE_DEPENDS
     src/*.cpp)
add_executable(ElectroSim ${ELECTROSIM_SOURCES})

target_include_directories(ElectroSim PRIVATE include)

# (Optional) Enable debug symbols on non-MSVC in Release for easier profiling
if(NOT MSVC AND CMAKE_BUILD_TYPE STREQUAL "Release")
  target_compile_options(ElectroSim PRIVATE -g)
endif()

# ---------- OPTIONAL: SFML (commented for now) ----------
# Later, when you add visualization, uncomment these lines and make sure SFML is installed.
# find_package(SFML 2.5 COMPONENTS system window graphics REQUIRED)
# target_link_libraries(ElectroSim PRIVATE sfml-system sfml-window sfml-graphics)