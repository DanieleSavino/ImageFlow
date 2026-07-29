#pragma once

#include "ImageFlow/error.h"
#include "ImageFlow/io/image.h"
#include "ImageFlow/devices/img_loader.h"
#include "ImageFlow/devices/devices.h"
#include "ImageFlow/operations/operations.h"
#include "ImageFlow/operations/op_constructor.h"
#include <math.h>

#ifdef __cplusplus
extern "C" {
#endif

IF_CONSTRUCTOR(IF_register_cpu_availability) {
    IF_enable_device(IF_DEV_CPU);
}

static inline IF_error_t IFOMP_checkImg(IF_image_t *img) {
    if(img == NULL || img->data == NULL) {
        return IF_NULL_POINTER;
    }
    if(img->channels < 3) {
        return IF_INVALID_CHANNELS;
    }

    return IF_SUCCESS;
}

IF_LOAD_IMG_IMPL(CPU) {
    //no-op
    return IF_SUCCESS;
}

IF_RETRIEVE_IMG_IMPL(CPU) {
    //no-op
    return IF_SUCCESS;
}

IF_FREE_IMG_IMPL(CPU) {
    /* host buffer is caller-owned; nothing to free here */
    return IF_SUCCESS;
}

IF_OP_IMPL(CPU, GRAYSCALE, 1, 1) {
    IF_image_t *img = imgs[0];
    IF_CHECK(IFOMP_checkImg(img));

    int n_pixels = img->height * img->width;
    float *pixel;

    #pragma omp parallel for simd schedule(static) private(pixel)
    for(int p = 0; p < n_pixels; p++) {
        pixel = img->data + (p * img->channels);

        float gray = 0.299f * pixel[0] + 0.587f * pixel[1] + 0.114f * pixel[2];
        pixel[0] = gray;
        pixel[1] = gray;
        pixel[2] = gray;
    }

    return IF_SUCCESS;
}

IF_OP_IMPL(CPU, INVERT, 1, 1) {
    IF_image_t *img = imgs[0];
    IF_CHECK(IFOMP_checkImg(img));

    size_t n = img->width * img->height * img->channels;

    #pragma omp parallel for simd schedule(static)
    for (size_t i = 0; i < n; i++) {
        img->data[i] = 1.0f - img->data[i];
    }
    return IF_SUCCESS;
}

IF_OP_IMPL(CPU, BRIGHTNESS, 1, 1) {
    IF_image_t *img = imgs[0];
    IF_CHECK(IFOMP_checkImg(img));

    size_t n = img->width * img->height * img->channels;

    #pragma omp parallel for simd schedule(static)
    for (size_t i = 0; i < n; i++) {
        img->data[i] = fminf(img->data[i] * args.float_factor.factor, 1.0f);
    }

    return IF_SUCCESS;
}

#ifdef __cplusplus
}
#endif
