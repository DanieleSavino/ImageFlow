#include "ImageFlow/error.h"
#include <stdio.h>

const char *IF_strerror(IF_error_t e) {
    switch(e) {
        case IF_SUCCESS:              return "Success";
        case IF_INVALID_ARGS:         return "Invalid argument";
        case IF_FILE_NOT_FOUND:       return "File not found";
        case IF_FILE_READ_ERROR:      return "File read error";
        case IF_FILE_WRITE_ERROR:     return "File write error";
        case IF_UNSUPPORTED_FORMAT:   return "Unsupported image format";
        case IF_INVALID_IMAGE:        return "Invalid or corrupted image";
        case IF_OUT_OF_MEMORY:        return "Out of memory";
        case IF_NULL_POINTER:         return "Null pointer passed to function";
        case IF_INVALID_DIMENSIONS:   return "Invalid image dimensions";
        case IF_INVALID_CHANNELS:     return "Invalid number of channels";
        case IF_INVALID_STRIDE:       return "Invalid row stride";
        case IF_ALIGNMENT_ERROR:      return "Memory alignment error";
        case IF_KERNEL_FAILURE:       return "Kernel execution failed";
        case IF_BACKEND_UNAVAILABLE:  return "Requested backend unavailable";
        case IF_THREAD_FAILURE:       return "Thread execution failure";
        case IF_DEVICE_ERROR:         return "Device / GPU error";
        case IF_CONVERSION_ERROR:     return "Pixel format conversion failed";
        case IF_NORMALIZATION_ERROR:  return "Normalization error";
        case IF_RESIZE_ERROR:         return "Image resize failed";
        case IF_FILTER_ERROR:         return "Image filter failed";
        case IF_MPI_ERROR:            return "MPI error";
        case IF_OMP_ERROR:            return "OpenMP error";
        case IF_CUDA_ERROR:           return "CUDA error";
        case IF_HIP_ERROR:            return "HIP/ROCm error";
        case IF_UNKNOWN_ERROR:        return "Unknown error";
        default:                      return "Unrecognized IF_error code";
    }
}

void IF_logError(FILE *stream, IF_error_t e) {
    fprintf(stream, "ImageFlow: %s\n", IF_strerror(e));
}
