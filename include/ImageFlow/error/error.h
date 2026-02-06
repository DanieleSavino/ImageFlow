#pragma once

#include <stdio.h>

#define IF_CHECK(call)                              \
    do {                                                 \
        IF_error_t __err__ = (call);                      \
        if (__err__ != IF_SUCCESS) {                      \
            fprintf(stderr, "ImageFlow ERROR: %s | In file: %s, at line %d\n",    \
                    IF_strerror(__err__), __FILE__, __LINE__);               \
            return __err__;                               \
        }                                               \
    } while(0)

typedef enum {
    // Success
    IF_SUCCESS = 0x0,

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
    IF_MPI_ERROR,
    IF_OMP_ERROR,
    IF_CUDA_ERROR,

    // General / unknown
    IF_UNKNOWN_ERROR
} IF_error_t;

const char *IF_strerror(IF_error_t e);

void IF_logError(FILE *stream, IF_error_t e);
