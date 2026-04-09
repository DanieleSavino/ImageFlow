/**
 * @file error.h
 * @brief Error codes, utility macros, and compiler annotations for ImageFlow.
 *
 * Defines IF_error_t, the unified return type used throughout the library,
 * along with helper macros for propagating errors, checked allocation, and
 * the NODISCARD annotation.
 *
 * @author Daniele Savino <daniele.savino0@gmail.com>
 *
 * SPDX-License-Identifier: MIT
 * Copyright (c) 2026 Daniele Savino
 */
#pragma once

#include <stdio.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Marks a function's return value as not safely discardable.
 *
 * Emits a compiler warning if the caller ignores the return value.
 * Supported on GCC, Clang, and MSVC. No-op on other compilers.
 *
 * @warning MSVC support is not tested.
 */
#if defined(__GNUC__) || defined(__clang__)
#define NODISCARD __attribute__((warn_unused_result))
#elif defined(_MSC_VER)
#define NODISCARD _Check_return_
#else
#define NODISCARD
#endif

/**
 * @brief Evaluates @p call, logs and propagates the error if it is not IF_SUCCESS.
 *
 * Prints file and line information to stderr via IF_strerror, then returns
 * the error code from the enclosing function.
 *
 * @param call An expression returning IF_error_t.
 */
#define IF_CHECK(call)                                                          \
    do {                                                                        \
        IF_error_t __err__ = (call);                                            \
        if (__err__ != IF_SUCCESS) {                                            \
            fprintf(stderr, "ImageFlow ERROR: %s | In file: %s, at line %d\n", \
                    IF_strerror(__err__), __FILE__, __LINE__);                  \
            return __err__;                                                     \
        }                                                                       \
    } while(0)

/**
 * @brief Calls malloc and returns IF_OUT_OF_MEMORY if allocation fails.
 *
 * @param ptr  lvalue to receive the allocated pointer.
 * @param size Number of bytes to allocate.
 */
#define IF_MALLOC(ptr, size)                        \
    do {                                            \
        (ptr) = malloc(size);                       \
        if ((ptr) == NULL && (size) > 0) {          \
            return IF_OUT_OF_MEMORY;                \
        }                                           \
    } while (0)

/**
 * @brief Calls malloc and jumps to @p label if allocation fails.
 *
 * @param ptr   lvalue to receive the allocated pointer.
 * @param size  Number of bytes to allocate.
 * @param label goto target on failure.
 */
#define IF_MALLOC_LABEL(ptr, size, label)           \
    do {                                            \
        (ptr) = malloc(size);                       \
        if ((ptr) == NULL && (size) > 0) {          \
            goto label;                             \
        }                                           \
    } while (0)

/**
 * @brief Calls realloc and returns IF_OUT_OF_MEMORY if reallocation fails.
 *
 * The original pointer is left unchanged on failure (realloc semantics).
 *
 * @param ptr  lvalue holding the existing pointer; updated on success.
 * @param size New size in bytes.
 */
#define IF_REALLOC(ptr, size)                           \
    do {                                                \
        void *tmp_realloc_ptr = realloc(ptr, size);     \
        if (tmp_realloc_ptr == NULL && (size) > 0) {    \
            return IF_OUT_OF_MEMORY;                    \
        }                                               \
        (ptr) = tmp_realloc_ptr;                        \
    } while (0)

/**
 * @brief Calls realloc and jumps to @p label if reallocation fails.
 *
 * The original pointer is left unchanged on failure (realloc semantics).
 *
 * @param ptr   lvalue holding the existing pointer; updated on success.
 * @param size  New size in bytes.
 * @param label goto target on failure.
 */
#define IF_REALLOC_LABEL(ptr, size, label)              \
    do {                                                \
        void *tmp_realloc_ptr = realloc(ptr, size);     \
        if (tmp_realloc_ptr == NULL && (size) > 0) {    \
            goto label;                                 \
        }                                               \
        (ptr) = tmp_realloc_ptr;                        \
    } while (0)

/**
 * @brief Unified error code enumeration for all ImageFlow operations.
 */
typedef enum {
    /* Success */
    IF_SUCCESS = 0x0,           /**< Operation completed successfully. */

    /* Invalid arguments */
    IF_INVALID_ARGS,            /**< One or more arguments are out of valid range. */

    /* File / I/O errors */
    IF_FILE_NOT_FOUND,          /**< File does not exist or is not accessible. */
    IF_FILE_READ_ERROR,         /**< Failed to read or decode the file. */
    IF_FILE_WRITE_ERROR,        /**< Failed to write or encode the file. */
    IF_UNSUPPORTED_FORMAT,      /**< File format is not supported. */
    IF_INVALID_IMAGE,           /**< Image data is invalid or corrupted. */

    /* Memory errors */
    IF_OUT_OF_MEMORY,           /**< Heap allocation failed. */
    IF_NULL_POINTER,            /**< A required pointer argument was NULL. */

    /* Image / data validation errors */
    IF_INVALID_DIMENSIONS,      /**< Image width or height is zero or negative. */
    IF_INVALID_CHANNELS,        /**< Channel count is unsupported (e.g. < 3 for RGB ops). */
    IF_INVALID_STRIDE,          /**< Row stride is inconsistent with width and channels. */
    IF_ALIGNMENT_ERROR,         /**< Buffer does not meet required memory alignment. */

    /* Runtime / kernel errors */
    IF_KERNEL_FAILURE,          /**< GPU kernel execution failed. */
    IF_BACKEND_UNAVAILABLE,     /**< Requested compute backend is not compiled in or not found. */
    IF_THREAD_FAILURE,          /**< CPU thread creation or execution failed. */
    IF_DEVICE_ERROR,            /**< Generic GPU device error (no device found, lost context, etc.). */

    /* Conversion / processing errors */
    IF_CONVERSION_ERROR,        /**< Pixel format or color space conversion failed. */
    IF_NORMALIZATION_ERROR,     /**< Value normalization produced out-of-range results. */
    IF_RESIZE_ERROR,            /**< Image resize operation failed. */
    IF_FILTER_ERROR,            /**< Image filter application failed. */

    /* Parallel / backend errors */
    IF_MPI_ERROR,               /**< MPI operation failed. @warning MPI support is not planned. */
    IF_OMP_ERROR,               /**< OpenMP runtime error. */
    IF_CUDA_ERROR,              /**< CUDA API call failed. */
    IF_HIP_ERROR,               /**< HIP/ROCm API call failed. */

    /* General */
    IF_UNKNOWN_ERROR            /**< Unclassified error. */
} IF_error_t;

/**
 * @brief Returns a human-readable string describing the given error code.
 *
 * @param e Error code to describe.
 * @return Null-terminated string literal. Never NULL.
 */
const char *IF_strerror(IF_error_t e);

/**
 * @brief Writes a formatted error message to @p stream.
 *
 * Equivalent to: fprintf(stream, "ImageFlow: %s\n", IF_strerror(e)).
 *
 * @param stream Output stream (e.g. stderr).
 * @param e      Error code to log.
 */
void IF_logError(FILE *stream, IF_error_t e);

#ifdef __cplusplus
}
#endif
