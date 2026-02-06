#include "ImageFlow/accelerated/cuda/backend.h"
#include "ImageFlow/accelerated/hip/backend.h"
#include "ImageFlow/error/error.h"
#include <ImageFlow/accelerated/acc_wrapper.h>

IF_acc_t acc_comp = UNDEF;

IF_error_t IFACC_check_system() {
    acc_comp = NO_ACC;

    #ifdef _CUDA_ACC
        int cuda_devices = 0;
        IFCU_getDevices(&cuda_devices);
        if (cuda_devices > 0) {
            acc_comp = CUDA_ACC;
            return IF_SUCCESS;
        }
    #endif

    #ifdef _HIP_ACC
        int hip_devices = 0;
        IFHIP_getDevices(&hip_devices);
        if (hip_devices > 0) {
            acc_comp = HIP_ACC;
            return IF_SUCCESS;
        }
    #endif

    // No accelerator found
    acc_comp = NO_ACC;
    return IF_SUCCESS;
}

static inline IF_error_t IFACC_check_comp() {
    if(acc_comp == UNDEF) {
        IF_CHECK(IFACC_check_system());
    }

    if(acc_comp == UNDEF || acc_comp == NO_ACC) {
        return IF_DEVICE_ERROR;
    }

    return IF_SUCCESS;
}

IF_error_t IFACC_load(const IF_image_t *img, IF_image_t **cuda_img) {
    IF_CHECK(IFACC_check_comp());

    if(acc_comp == CUDA_ACC) {
        return IFCU_load(img, cuda_img);
    }
    else {
        return IFHIP_load(img, cuda_img);
    }
}

IF_error_t IFACC_retrieve(IF_image_t **cuda_img, IF_image_t *img_out) {
    IF_CHECK(IFACC_check_comp());

    if(acc_comp == CUDA_ACC) {
        return IFCU_retrieve(cuda_img, img_out);
    }
    else {
        return IFHIP_retrieve(cuda_img, img_out);
    }
}

IF_error_t IFACC_grayScale(IF_image_t *img) {
    IF_CHECK(IFACC_check_comp());

    if(acc_comp == CUDA_ACC) {
        return IFCU_grayScale(img);
    }
    else {
        return IFHIP_grayScale(img);
    }
}

IF_error_t IFACC_invert(IF_image_t *img) {
    IF_CHECK(IFACC_check_comp());

    if(acc_comp == CUDA_ACC) {
        return IFCU_invert(img);
    }
    else {
        return IFHIP_invert(img);
    }
}

IF_error_t IFACC_brightness(IF_image_t *img, float factor) {
    IF_CHECK(IFACC_check_comp());

    if(acc_comp == CUDA_ACC) {
        return IFCU_brightness(img, factor);
    }
    else {
        return IFHIP_brightness(img, factor);
    }
}

IF_error_t IFACC_loaded_grayScale(const IF_image_t *img, IF_image_t *cuda_img) {
    IF_CHECK(IFACC_check_comp());

    if(acc_comp == CUDA_ACC) {
        return IFCU_loaded_grayScale(img, cuda_img);
    }
    else {
        return IFHIP_loaded_grayScale(img, cuda_img);
    }
}

IF_error_t IFACC_loaded_invert(const IF_image_t *img, IF_image_t *cuda_img) {
    IF_CHECK(IFACC_check_comp());

    if(acc_comp == CUDA_ACC) {
        return IFCU_loaded_invert(img, cuda_img);
    }
    else {
        return IFHIP_loaded_invert(img, cuda_img);
    }
}

IF_error_t IFACC_loaded_brightness(const IF_image_t *img, IF_image_t *cuda_img, float factor) {
    IF_CHECK(IFACC_check_comp());

    if(acc_comp == CUDA_ACC) {
        return IFCU_loaded_brightness(img, cuda_img, factor);
    }
    else {
        return IFHIP_loaded_brightness(img, cuda_img, factor);
    }
}
