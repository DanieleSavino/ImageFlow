#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include <stdio.h>

#if defined(__GNUC__) || defined(__clang__)
#define NODISCARD __attribute__((warn_unused_result))
// WARN: MSC is not supported.
#elif defined(_MSC_VER)
#define NODISCARD _Check_return_
#else
#define NODISCARD
#endif

#define IF_CHECK(call)                              \
    do {                                                 \
        IF_error_t __err__ = (call);                      \
        if (__err__ != IF_SUCCESS) {                      \
            fprintf(stderr, "ImageFlow ERROR: %s | In file: %s, at line %d\n",    \
                    IF_strerror(__err__), __FILE__, __LINE__);               \
            return __err__;                               \
        }                                               \
    } while(0)

#define IF_MALLOC(ptr, size)                         \
    do {                                             \
        (ptr) = malloc(size);                        \
        if ((ptr) == NULL && (size) > 0) {           \
            return IF_OUT_OF_MEMORY;                 \
        }                                            \
    } while (0)

#define IF_MALLOC_LABEL(ptr, size, label)            \
    do {                                             \
        (ptr) = malloc(size);                        \
        if ((ptr) == NULL && (size) > 0) {           \
            goto label;                              \
        }                                            \
    } while (0)

#define IF_REALLOC(ptr, size)                    \
    do {                                         \
        void *tmp_realloc_ptr = realloc(ptr, size); \
        if (tmp_realloc_ptr == NULL && (size) > 0) { \
            return IF_OUT_OF_MEMORY;                          \
        }                                        \
        (ptr) = tmp_realloc_ptr;                 \
    } while (0)

#define IF_REALLOC_LABEL(ptr, size, label)             \
    do {                                         \
        void *tmp_realloc_ptr = realloc(ptr, size); \
        if (tmp_realloc_ptr == NULL && (size) > 0) { \
            goto label;                          \
        }                                        \
        (ptr) = tmp_realloc_ptr;                 \
    } while (0)



typedef enum {
    // Success
    IF_SUCCESS = 0x0,

    // Invalid args
    IF_INVALID_ARGS,

    // File / I/O errors
    IF_FILE_NOT_FOUND,
    IF_FILE_READ_ERROR,
    IF_FILE_WRITE_ERROR,
    IF_UNSUPPORTED_FORMAT,
    IF_INVALID_IMAGE,

    // Memory errors
    IF_OUT_OF_MEMORY,
    IF_NULL_POINTER,

    // Image / data validation errors
    IF_INVALID_DIMENSIONS,
    IF_INVALID_CHANNELS,
    IF_INVALID_STRIDE,
    IF_ALIGNMENT_ERROR,

    // Runtime / kernel errors
    IF_KERNEL_FAILURE,
    IF_BACKEND_UNAVAILABLE,
    IF_THREAD_FAILURE,
    IF_DEVICE_ERROR,

    // Conversion / processing errors
    IF_CONVERSION_ERROR,
    IF_NORMALIZATION_ERROR,
    IF_RESIZE_ERROR,
    IF_FILTER_ERROR,

    // Parallel / MPI errors
    IF_MPI_ERROR, // WARN: MPI support is not planned.
    IF_OMP_ERROR,
    IF_CUDA_ERROR,
    IF_HIP_ERROR,

    // General / unknown
    IF_UNKNOWN_ERROR
} IF_error_t;

const char *IF_strerror(IF_error_t e);

void IF_logError(FILE *stream, IF_error_t e);

#ifdef __cplusplus
}
#endif
