cmake_minimum_required(VERSION 3.20)
project(ImgFlow LANGUAGES C CUDA)

# =========================
# Use C11 standard
# =========================
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

# =========================
# Include directories
# =========================
include_directories(
    ${PROJECT_SOURCE_DIR}/include
)

# =========================
# Gather all sources
# =========================
file(GLOB_RECURSE SRC_C
    "${PROJECT_SOURCE_DIR}/src/*.c"
)
file(GLOB_RECURSE SRC_CUDA
    "${PROJECT_SOURCE_DIR}/src/*.cu"
)

# =========================
# Mark C files as pure C
# =========================
set_source_files_properties(${SRC_C} PROPERTIES LANGUAGE C)
set_source_files_properties(${SRC_CUDA} PROPERTIES LANGUAGE CUDA)

# =========================
# Add executable
# =========================
add_executable(imgflow
    ${SRC_C}
    ${SRC_CUDA}
)

# =========================
# Link math library
# =========================
target_link_libraries(imgflow m)

# =========================
# Set build type and flags
# =========================
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Debug)
endif()

message(STATUS "Build type: ${CMAKE_BUILD_TYPE}")

if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    target_compile_definitions(imgflow PRIVATE DEBUG NDIST)
    target_compile_options(imgflow PRIVATE -O0 -g)
elseif(CMAKE_BUILD_TYPE STREQUAL "Release")
    target_compile_definitions(imgflow PRIVATE NDEBUG DIST)
    target_compile_options(imgflow PRIVATE -O3 -march=native)
endif()

# =========================
# Ensure proper C linkage for C headers when compiling CUDA
# =========================
# In your .cu files, wrap C includes like this:
#
# #ifdef __cplusplus
# extern "C" {
# #endif
# #include "ImageFlow/error/error.h"
# #include "ImageFlow/io/image.h"
# #ifdef __cplusplus
# }
# #endif
#
# This prevents undefined references for C functions like IF_strerror
