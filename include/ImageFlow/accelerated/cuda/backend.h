#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include "ImageFlow/error/error.h"
#include "ImageFlow/io/image.h"

#define IF_CUDA_CHECK(call)                                                       \
    do {                                                                       \
        cudaError_t __err__ = call;                                                \
        if (__err__ != cudaSuccess) {                                              \
            fprintf(stderr,                                                     \
                    "CUDA Error: %s in file %s at line %d\n",                   \
                    cudaGetErrorString(__err__),                                    \
                    __FILE__,                                                   \
                    __LINE__);                                                  \
            return IF_CUDA_ERROR;                                              \
        }                                                                      \
    } while (0)

#define IF_CUDA_KERNEL_CHECK()                                                    \
    do {                                                                       \
        cudaError_t __err__ = cudaGetLastError();                                  \
        if (__err__ != cudaSuccess) {                                              \
            fprintf(stderr,                                                    \
                    "CUDA Kernel Launch Error: %s in file %s at line %d\n",    \
                    cudaGetErrorString(__err__),                                   \
                    __FILE__,                                                  \
                    __LINE__);                                                 \
            return IF_CUDA_ERROR;                                               \
        }                                                                      \
    } while (0)

IF_error_t IFCU_getDevices(int *cuda_dev);

IF_error_t IFCU_load(const IF_image_t *img, IF_image_t **cuda_img);
IF_error_t IFCU_retrieve(IF_image_t **cuda_img, IF_image_t *img_out);

IF_error_t IFCU_grayScale(IF_image_t *img);
IF_error_t IFCU_invert(IF_image_t *img);
IF_error_t IFCU_brightness(IF_image_t *img, float factor);

IF_error_t IFCU_loaded_grayScale(const IF_image_t *img, IF_image_t *cuda_img);
IF_error_t IFCU_loaded_invert(const IF_image_t *img, IF_image_t *cuda_img);
IF_error_t IFCU_loaded_brightness(const IF_image_t *img, IF_image_t *cuda_img, float factor);

#ifdef __cplusplus
}
#endif
