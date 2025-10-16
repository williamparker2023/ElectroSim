cmake_minimum_required(VERSION 3.20)

# -----------------------------
# Project & C++ standard
# -----------------------------
project(ElectroSim LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Default to Release if nothing set (you can still pick Debug/RelWithDebInfo in VS Code)
if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type" FORCE)
endif()

# Put executables under build/bin
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

# Warnings
if(MSVC)
  add_compile_options(/W4 /permissive- /Zc:__cplusplus)
else()
  add_compile_options(-Wall -Wextra -Wpedantic)
endif()

# -----------------------------
# Sources & target
# -----------------------------
file(GLOB_RECURSE ELECTROSIM_SOURCES CONFIGURE_DEPENDS
     src/*.cpp)
add_executable(ElectroSim ${ELECTROSIM_SOURCES})
target_include_directories(ElectroSim PRIVATE include)

# (Optional) keep symbols on non-MSVC even in Release for profiling
if(NOT MSVC AND CMAKE_BUILD_TYPE STREQUAL "Release")
  target_compile_options(ElectroSim PRIVATE -g)
endif()

# -----------------------------
# SFML (use SFML_DIR passed in from VS Code settings.json)
# -----------------------------
# Print what CMake thinks SFML_DIR is (helps debugging)
message(STATUS "SFML_DIR from cache: ${SFML_DIR}")

if(NOT DEFINED SFML_DIR)
  message(FATAL_ERROR "SFML_DIR is not set. Point it to the folder that CONTAINS SFMLConfig.cmake, e.g.
    D:/SFML-2.6.2-windows-vc17-64-bit/SFML-2.6.2/lib/cmake/SFML")
endif()

find_package(SFML 2.5 CONFIG REQUIRED COMPONENTS system window graphics)

target_link_libraries(ElectroSim PRIVATE sfml-system sfml-window sfml-graphics)

get_target_property(SFML_INC sfml-graphics INTERFACE_INCLUDE_DIRECTORIES)

target_include_directories(ElectroSim PRIVATE ${SFML_INC})


# Auto-copy SFML runtime DLLs next to the executable after build (Windows only)
if(WIN32)
  add_custom_command(TARGET ElectroSim POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
            "D:/SFML-2.6.2-windows-vc17-64-bit/SFML-2.6.2/bin"
            "$<TARGET_FILE_DIR:ElectroSim>"
    COMMENT "Copying SFML DLLs to output folder")
endif()
