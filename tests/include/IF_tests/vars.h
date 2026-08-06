#pragma once

#include <string.h>

#ifdef __cplusplus
extern "C" {
#endif

#define IF_TESTS_DIM_X "IF_TESTS_DIM_X"
#define _DEFAULT_IF_TESTS_DIM_X 1920

#define IF_TESTS_DIM_Y "IF_TESTS_DIM_Y"
#define _DEFAULT_IF_TESTS_DIM_Y 1080

#define IF_TESTS_CHANNELS "IF_TESTS_CHANNELS"
#define _DEFAULT_IF_TESTS_CHANNELS 3

#define IF_TESTS_VERBOSE "IF_TESTS_VERBOSE"
#define _DEFAULT_IF_TESTS_VERBOSE 0

#define IF_GET_TEST_VAR(var, val) \
    do { \
        if (getenv(var) == NULL) { \
            if(getenv(IF_TESTS_VERBOSE) != NULL && strcmp(getenv(IF_TESTS_VERBOSE), "1") == 0) { \
                fprintf(stderr, "[get_test_var] %s not set, using default %d\n", var, _DEFAULT_##var); \
            } \
            *val = _DEFAULT_##var; \
        } \
        else \
            *val = atoi(getenv(var)); \
    } while (0)

#ifdef __cplusplus
}
#endif
