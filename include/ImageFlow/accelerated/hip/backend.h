#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include "ImageFlow/error.h"
#include "ImageFlow/io/image.h"

#define IF_HIP_CHECK(call)                                                       \
    do {                                                                         \
        hipError_t __err__ = call;                                               \
        if (__err__ != hipSuccess) {                                             \
            fprintf(stderr,                                                      \
                    "HIP Error: %s in file %s at line %d\n",                    \
                    hipGetErrorString(__err__),                                  \
                    __FILE__,                                                   \
                    __LINE__);                                                  \
            return IF_HIP_ERROR; /* define this in error.h */                    \
        }                                                                        \
    } while (0)

#define IF_HIP_KERNEL_CHECK()                                                    \
    do {                                                                         \
        hipError_t __err__ = hipGetLastError();                                  \
        if (__err__ != hipSuccess) {                                             \
            fprintf(stderr,                                                      \
                    "HIP Kernel Launch Error: %s in file %s at line %d\n",      \
                    hipGetErrorString(__err__),                                  \
                    __FILE__,                                                   \
                    __LINE__);                                                  \
            return IF_HIP_ERROR;                                                \
        }                                                                        \
    } while (0)

NODISCARD IF_error_t IFHIP_getDevices(int *hip_dev);

NODISCARD IF_error_t IFHIP_load(const IF_image_t *img, IF_image_t **hip_img);
NODISCARD IF_error_t IFHIP_retrieve(IF_image_t **hip_img, IF_image_t *img_out);

NODISCARD IF_error_t IFHIP_grayScale(IF_image_t *img);
NODISCARD IF_error_t IFHIP_invert(IF_image_t *img);
NODISCARD IF_error_t IFHIP_brightness(IF_image_t *img, float factor);

NODISCARD IF_error_t IFHIP_loaded_grayScale(const IF_image_t *img, IF_image_t *hip_img);
NODISCARD IF_error_t IFHIP_loaded_invert(const IF_image_t *img, IF_image_t *hip_img);
NODISCARD IF_error_t IFHIP_loaded_brightness(const IF_image_t *img, IF_image_t *hip_img, float factor);

#ifdef __cplusplus
}
#endif
