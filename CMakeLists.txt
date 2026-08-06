# ==============================================================================
# ImageFlow — top-level CMakeLists.txt
# ==============================================================================
cmake_minimum_required(VERSION 3.20)
project(ImgFlow LANGUAGES C CXX)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Debug)
endif()
string(TOLOWER "${CMAKE_BUILD_TYPE}" BUILD_TYPE_LOWER)
message(STATUS "Build type: ${CMAKE_BUILD_TYPE}")

option(BUILD_TESTS "Build the ImageFlow test suite"    ON)

# ==============================================================================
# Whole-archive helper
#
# Static libraries whose translation units self-register via load-time
# constructors (IF_CONSTRUCTOR) must be linked with --whole-archive, or the
# linker will silently drop any .o file that nothing else references by name
# — which is every registration TU, since nothing calls them directly.
# This is a static-archive-only concern; .so consumers don't need it.
# ==============================================================================
function(target_link_whole_archive target archive)
    add_dependencies(${target} ${archive})
    if(APPLE)
        target_link_options(${target} PRIVATE "LINKER:-force_load,$<TARGET_FILE:${archive}>")
        target_link_libraries(${target} PRIVATE ${archive})
    elseif(MSVC)
        target_link_libraries(${target} PRIVATE ${archive})
        target_link_options(${target} PRIVATE "/WHOLEARCHIVE:$<TARGET_FILE:${archive}>")
    else()
        target_link_options(${target} PRIVATE
            "LINKER:--whole-archive,$<TARGET_FILE:${archive}>,--no-whole-archive"
        )
        target_link_libraries(${target} PRIVATE ${archive})
    endif()
endfunction()

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
# Library sources (everything except main.c)
# =========================
file(GLOB_RECURSE SRC_C "${PROJECT_SOURCE_DIR}/src/*.c")
list(FILTER SRC_C EXCLUDE REGEX ".*/src/main\\.c$")
set_source_files_properties(${SRC_C} PROPERTIES LANGUAGE C)

set(SRC_CUDA "")
if(HAVE_CUDA)
    file(GLOB_RECURSE SRC_CUDA "${PROJECT_SOURCE_DIR}/src/backends/*.cu")
    set_source_files_properties(${SRC_CUDA} PROPERTIES LANGUAGE CUDA)
endif()

set(SRC_HIP "")
if(HAVE_HIP)
    file(GLOB_RECURSE SRC_HIP "${PROJECT_SOURCE_DIR}/src/backends/*.cpp")
    set_source_files_properties(${SRC_HIP} PROPERTIES LANGUAGE HIP)
endif()

set(IMAGEFLOW_ALL_SRC ${SRC_C} ${SRC_CUDA} ${SRC_HIP})

set(_acc_defs "")
if(HAVE_CUDA)
    list(APPEND _acc_defs _CUDA_ACC)
endif()
if(HAVE_HIP)
    list(APPEND _acc_defs _HIP_ACC)
endif()

# =========================
# Shared compile settings applied identically to both variants,
# so debug/release behave the same regardless of which one gets linked.
# =========================
function(imageflow_apply_build_settings tgt)
    target_include_directories(${tgt} PUBLIC
        $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
    )
    target_link_libraries(${tgt} PUBLIC m)
    if(HAVE_HIP)
        target_link_libraries(${tgt} PUBLIC hip::device)
    endif()

    # Never hide symbols and never strip unreferenced sections/functions —
    # both would risk eliminating constructor-registration TUs or symbols
    # that dispatch tables rely on being present at load time.
    set_target_properties(${tgt} PROPERTIES
        C_VISIBILITY_PRESET default
        CXX_VISIBILITY_PRESET default
        VISIBILITY_INLINES_HIDDEN OFF
    )

    if(CMAKE_BUILD_TYPE STREQUAL "Debug")
        target_compile_definitions(${tgt} PUBLIC DEBUG NDIST ${_acc_defs})
        target_compile_options(${tgt} PRIVATE
            $<$<COMPILE_LANGUAGE:C>:-O0 -g>
            $<$<COMPILE_LANGUAGE:CXX>:-O0 -g>
            $<$<AND:$<BOOL:${HAVE_CUDA}>,$<COMPILE_LANGUAGE:CUDA>>:-O0 -g>
            $<$<AND:$<BOOL:${HAVE_HIP}>,$<COMPILE_LANGUAGE:HIP>>:-O0 -g>
        )
    else()
        target_compile_definitions(${tgt} PUBLIC NDEBUG DIST ${_acc_defs})
        target_compile_options(${tgt} PRIVATE
            $<$<COMPILE_LANGUAGE:C>:-O3 -march=native>
            $<$<COMPILE_LANGUAGE:CXX>:-O3 -march=native>
            $<$<AND:$<BOOL:${HAVE_CUDA}>,$<COMPILE_LANGUAGE:CUDA>>:-O3>
            $<$<AND:$<BOOL:${HAVE_HIP}>,$<COMPILE_LANGUAGE:HIP>>:-O3>
        )
    endif()
endfunction()

# =========================
# libImageFlow.a  (static, PIC — must be linkable into a .so later, e.g. pybind)
# =========================
add_library(imageflow_static STATIC ${IMAGEFLOW_ALL_SRC})
add_library(ImageFlow::static ALIAS imageflow_static)
imageflow_apply_build_settings(imageflow_static)
set_target_properties(imageflow_static PROPERTIES
    POSITION_INDEPENDENT_CODE ON
    OUTPUT_NAME ImageFlow
    ARCHIVE_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}/build/${BUILD_TYPE_LOWER}
)

# =========================
# libImageFlow.so  (shared — every object file always included, no whole-archive needed downstream)
# =========================
add_library(imageflow_shared SHARED ${IMAGEFLOW_ALL_SRC})
add_library(ImageFlow::shared ALIAS imageflow_shared)
imageflow_apply_build_settings(imageflow_shared)
set_target_properties(imageflow_shared PROPERTIES
    OUTPUT_NAME ImageFlow
    LIBRARY_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}/build/${BUILD_TYPE_LOWER}
    RUNTIME_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}/build/${BUILD_TYPE_LOWER}
)

message(STATUS "Accelerators enabled — CUDA: ${HAVE_CUDA}, HIP: ${HAVE_HIP}")

# =========================
# Tests
# =========================
if(BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()

# =========================
# Install
# =========================
include(GNUInstallDirs)
install(TARGETS imageflow_static imageflow_shared
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)
install(DIRECTORY include/ImageFlow DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})
