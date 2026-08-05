/**
 * @file error.h
 * @brief IF_error_t propagation for fastest test bodies.
 *
 * Mirrors ImageFlow's own IF_CHECK (file/line-annotated propagate-on-error)
 * but targets a fastest FASTEST_TestOutput_t *out instead of a C return,
 * so ImageFlow calls can be checked directly inside FASTEST_CUSTOMTEST /
 * _INLINE / _DINLINE bodies without hand-unwrapping IF_error_t each time.
 *
 * @author Daniele Savino <daniele.savino0@gmail.com>
 *
 * SPDX-License-Identifier: MIT
 * Copyright (c) 2026 Daniele Savino
 */
#pragma once

#include "ImageFlow/error.h"
#include "fastest/tests.h"
#include <stdio.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Maps an IF_error_t to the closest matching FASTEST exit code.
 *
 * Not a 1:1 mapping — IF_error_t is finer-grained than fastest's exit
 * status set, so several IF_error_t values collapse onto the same
 * FASTEST_ERROR_* bucket. Falls back to FASTEST_ERROR_UNEXPECTED for
 * anything not explicitly handled.
 */
static inline int IF_toFastestExit(IF_error_t err) {
    switch (err) {
        case IF_SUCCESS:            return FASTEST_SUCCESS;
        case IF_OUT_OF_MEMORY:      return FASTEST_ERROR_MEMORY;
        case IF_FILE_READ_ERROR:
        case IF_FILE_WRITE_ERROR:
        case IF_UNSUPPORTED_FORMAT: return FASTEST_ERROR_RESOURCE;
        case IF_NULL_POINTER:
        default:                    return FASTEST_ERROR_UNEXPECTED;
    }
}

#ifdef __cplusplus
}
#endif

/**
 * @brief Evaluates an IF_error_t-returning expression inside a fastest test.
 *
 * On failure: logs the error via IF_logError (with file/line context),
 * sets out->exit_status to the mapped FASTEST_ERROR_* code, and returns
 * from the enclosing test function. Requires a FASTEST_TestOutput_t *out
 * in scope, matching the signature fastest hands to CUSTOMTEST bodies.
 *
 * @param expr An expression evaluating to IF_error_t.
 */
#define IF_FASTEST_CHECK(expr)                                             \
    do {                                                                   \
        IF_error_t _if_fc_err = (expr);                                    \
        if (_if_fc_err != IF_SUCCESS) {                                    \
            fprintf(stderr, "[IF_FASTEST_CHECK] %s:%d: %s -> %s\n",        \
                    __FILE__, __LINE__, #expr, IF_strerror(_if_fc_err));   \
            IF_logError(stderr, _if_fc_err);                               \
            out->exit_status |= IF_toFastestExit(_if_fc_err);              \
            out->test_flags  |= FASTEST_FAIL_ERROR;                        \
            return;                                                       \
        }                                                                  \
    } while (0)
