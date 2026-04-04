cmake_minimum_required(VERSION 3.20)
project(ImgFlow LANGUAGES C CXX)

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
# CUDA — optional
# =========================
include(CheckLanguage)
check_language(CUDA)
if(CMAKE_CUDA_COMPILER)
    enable_language(CUDA)
    set(HAVE_CUDA TRUE)
    message(STATUS "CUDA found: ${CMAKE_CUDA_COMPILER}")
else()
    set(HAVE_CUDA FALSE)
    message(STATUS "CUDA not found — skipping CUDA sources")
endif()

# =========================
# HIP — optional
# =========================
if(NOT DEFINED HIP_PATH)
    if(DEFINED ENV{HIP_PATH})
        set(HIP_PATH $ENV{HIP_PATH} CACHE PATH "Path to HIP installation")
    else()
        set(HIP_PATH "/opt/rocm" CACHE PATH "Path to HIP installation")
    endif()
endif()

find_package(hip CONFIG QUIET)
if(hip_FOUND)
    set(HAVE_HIP TRUE)
    message(STATUS "HIP found")
else()
    set(HAVE_HIP FALSE)
    message(STATUS "HIP not found — skipping HIP sources")
endif()

# =========================
# Include dirs
# =========================
include_directories(${PROJECT_SOURCE_DIR}/include)

# =========================
# Sources
# =========================
file(GLOB_RECURSE SRC_C "${PROJECT_SOURCE_DIR}/src/*.c")

set(SRC_CUDA "")
if(HAVE_CUDA)
    file(GLOB_RECURSE SRC_CUDA "${PROJECT_SOURCE_DIR}/src/accelerated/cuda/*.cu")
    set_source_files_properties(${SRC_CUDA} PROPERTIES LANGUAGE CUDA)
endif()

set(SRC_HIP "")
if(HAVE_HIP)
    file(GLOB_RECURSE SRC_HIP "${PROJECT_SOURCE_DIR}/src/accelerated/hip/*.cpp")
    set_source_files_properties(${SRC_HIP} PROPERTIES LANGUAGE HIP)
endif()

set_source_files_properties(${SRC_C} PROPERTIES LANGUAGE C)

# =========================
# Executable
# =========================
add_executable(imgflow
    ${SRC_C}
    ${SRC_CUDA}
    ${SRC_HIP}
)

# =========================
# Link libraries
# =========================
target_link_libraries(imgflow PRIVATE m)

if(HAVE_HIP)
    target_link_libraries(imgflow PRIVATE hip::device)
endif()

# =========================
# Compile definitions & flags
# =========================
set(_acc_defs "")
if(HAVE_CUDA)
    list(APPEND _acc_defs _CUDA_ACC)
endif()
if(HAVE_HIP)
    list(APPEND _acc_defs _HIP_ACC)
endif()

if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    target_compile_definitions(imgflow PRIVATE DEBUG NDIST ${_acc_defs})
    target_compile_options(imgflow PRIVATE
        $<$<COMPILE_LANGUAGE:C>:-O0 -g>
        $<$<COMPILE_LANGUAGE:CXX>:-O0 -g>
        $<$<AND:$<BOOL:${HAVE_CUDA}>,$<COMPILE_LANGUAGE:CUDA>>:-O0 -g>
        $<$<AND:$<BOOL:${HAVE_HIP}>,$<COMPILE_LANGUAGE:HIP>>:-O0 -g>
    )
elseif(CMAKE_BUILD_TYPE STREQUAL "Release")
    target_compile_definitions(imgflow PRIVATE NDEBUG DIST ${_acc_defs})
    target_compile_options(imgflow PRIVATE
        $<$<COMPILE_LANGUAGE:C>:-O3 -march=native>
        $<$<COMPILE_LANGUAGE:CXX>:-O3 -march=native>
        $<$<AND:$<BOOL:${HAVE_CUDA}>,$<COMPILE_LANGUAGE:CUDA>>:-O3>
        $<$<AND:$<BOOL:${HAVE_HIP}>,$<COMPILE_LANGUAGE:HIP>>:-O3>
    )
endif()

message(STATUS "Accelerators enabled — CUDA: ${HAVE_CUDA}, HIP: ${HAVE_HIP}")
