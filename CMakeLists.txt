cmake_minimum_required(VERSION 3.20)
project(ImgFlow LANGUAGES C CXX CUDA HIP)

# =========================
# Standards
# =========================
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# =========================
# Build type
# =========================
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Debug)
endif()

message(STATUS "Build type: ${CMAKE_BUILD_TYPE}")

# =========================
# HIP Configuration
# =========================
if(NOT DEFINED HIP_PATH)
    if(NOT DEFINED ENV{HIP_PATH})
        set(HIP_PATH "/opt/rocm" CACHE PATH "Path to HIP installation")
    else()
        set(HIP_PATH $ENV{HIP_PATH} CACHE PATH "Path to HIP installation")
    endif()
endif()

# Set HIP architectures for your 5700 XT (gfx1010)
set(CMAKE_HIP_ARCHITECTURES gfx1010)

# =========================
# Include dirs
# =========================
include_directories(
    ${PROJECT_SOURCE_DIR}/include
)

# =========================
# Sources
# =========================
file(GLOB_RECURSE SRC_C
    "${PROJECT_SOURCE_DIR}/src/*.c"
)

file(GLOB_RECURSE SRC_CUDA
    "${PROJECT_SOURCE_DIR}/src/accelerated/cuda/*.cu"
)

file(GLOB_RECURSE SRC_HIP
    "${PROJECT_SOURCE_DIR}/src/accelerated/hip/*.cpp"
)

# =========================
# Languages
# =========================
set_source_files_properties(${SRC_C} PROPERTIES LANGUAGE C)
set_source_files_properties(${SRC_CUDA} PROPERTIES LANGUAGE CUDA)
set_source_files_properties(${SRC_HIP} PROPERTIES LANGUAGE HIP)

# =========================
# Executable
# =========================
add_executable(imgflow
    ${SRC_C}
    ${SRC_CUDA}
    ${SRC_HIP}
)

# =========================
# CUDA
# =========================
if(SRC_CUDA)
    enable_language(CUDA)
endif()

# =========================
# HIP
# =========================
find_package(hip REQUIRED CONFIG)

target_link_libraries(imgflow PRIVATE
    m
    hip::device
)

# =========================
# Compile flags
# =========================
if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    target_compile_definitions(imgflow PRIVATE DEBUG NDIST _CUDA_ACC _HIP_ACC)
    target_compile_options(imgflow PRIVATE 
        $<$<COMPILE_LANGUAGE:C>:-O0 -g>
        $<$<COMPILE_LANGUAGE:CXX>:-O0 -g>
        $<$<COMPILE_LANGUAGE:CUDA>:-O0 -g>
        $<$<COMPILE_LANGUAGE:HIP>:-O0 -g>
    )
elseif(CMAKE_BUILD_TYPE STREQUAL "Release")
    target_compile_definitions(imgflow PRIVATE NDEBUG DIST _CUDA_ACC _HIP_ACC)
    target_compile_options(imgflow PRIVATE 
        $<$<COMPILE_LANGUAGE:C>:-O3 -march=native>
        $<$<COMPILE_LANGUAGE:CXX>:-O3 -march=native>
        $<$<COMPILE_LANGUAGE:CUDA>:-O3>
        $<$<COMPILE_LANGUAGE:HIP>:-O3>
    )
endif()
