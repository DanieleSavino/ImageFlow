cmake_minimum_required(VERSION 3.20)
project(ImgFlow C)

# Use C11
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

# Include directories
include_directories(
    ${PROJECT_SOURCE_DIR}/include
)

# Recursively gather all .c files in src/
file(GLOB_RECURSE SRC
    "${PROJECT_SOURCE_DIR}/src/*.c"
)

# Executable
add_executable(imgflow ${SRC})

# Link math library
target_link_libraries(imgflow m)

# =========================
# Build types: Debug / Release
# =========================

# Default build type
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Debug)
endif()

message(STATUS "Build type: ${CMAKE_BUILD_TYPE}")

# Set flags and defines per build type
if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    target_compile_definitions(imgflow PRIVATE DEBUG NDIST)
    target_compile_options(imgflow PRIVATE -O0 -g)
elseif(CMAKE_BUILD_TYPE STREQUAL "Release")
    target_compile_definitions(imgflow PRIVATE NDEBUG DIST)
    target_compile_options(imgflow PRIVATE -O3 -march=native)
endif()
