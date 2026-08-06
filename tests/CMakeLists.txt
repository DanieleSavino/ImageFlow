# ==============================================================================
# tests/CMakeLists.txt
# ==============================================================================
add_subdirectory(vendor/fastest)

file(GLOB_RECURSE TEST_SRC "${CMAKE_CURRENT_SOURCE_DIR}/src/*.c")

if(NOT TEST_SRC)
    message(WARNING "tests/src/*.c is empty — imageflow_tests_static will not be built yet")
else()
    # ------------------------------------------------------------------------------
    # libImageFlow_tests.a — static archive of the test sources. Consumed by
    # fastest's own pybind11 wheel-build flow (FASTEST_USER_LIB expects a .a);
    # fastest's build does the final link into a runnable Python extension, so
    # this target only needs headers to compile, not a link against
    # imageflow/fastest itself.
    # ------------------------------------------------------------------------------
    add_library(imageflow_tests_static STATIC ${TEST_SRC})
    add_library(ImageFlow::tests_static ALIAS imageflow_tests_static)

    target_include_directories(imageflow_tests_static PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/include
        $<TARGET_PROPERTY:imageflow_static,INTERFACE_INCLUDE_DIRECTORIES>
        $<TARGET_PROPERTY:fastest_static,INTERFACE_INCLUDE_DIRECTORIES>
    )

    set_target_properties(imageflow_tests_static PROPERTIES
        POSITION_INDEPENDENT_CODE ON
        C_VISIBILITY_PRESET default
        VISIBILITY_INLINES_HIDDEN OFF
        OUTPUT_NAME ImageFlow_tests
        ARCHIVE_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}/build/${BUILD_TYPE_LOWER}
    )

    if(CMAKE_BUILD_TYPE STREQUAL "Debug")
        target_compile_definitions(imageflow_tests_static PRIVATE DEBUG)
        target_compile_options(imageflow_tests_static PRIVATE -g -O0)
    else()
        target_compile_definitions(imageflow_tests_static PRIVATE NDEBUG)
        target_compile_options(imageflow_tests_static PRIVATE -O2)
    endif()
endif()
