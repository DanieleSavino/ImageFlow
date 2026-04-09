#include "ImageFlow/omp/backend.h"
#include "ImageFlow/error.h"
#include "ImageFlow/io/image.h"
#include <math.h>
#include <stdlib.h>
#include <string.h>

static inline IF_error_t IFOMP_checkImg(IF_image_t *img) {
    if(img == NULL || img->data == NULL) {
        return IF_NULL_POINTER;
    }
    if(img->channels < 3) {
        return IF_INVALID_CHANNELS;
    }

    return IF_SUCCESS;
}

IF_error_t IFOMP_grayScale(IF_image_t *img) {
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

IF_error_t IFOMP_invert(IF_image_t *img) {
    IF_CHECK(IFOMP_checkImg(img));

    size_t n = img->width * img->height * img->channels;

    #pragma omp parallel for simd schedule(static)
    for (size_t i = 0; i < n; i++) {
        img->data[i] = 1.0f - img->data[i];
    }
    return IF_SUCCESS;
}

IF_error_t IFOMP_brightness(IF_image_t *img, float factor) {
    IF_CHECK(IFOMP_checkImg(img));

    size_t n = img->width * img->height * img->channels;

    #pragma omp parallel for simd schedule(static)
    for (size_t i = 0; i < n; i++) {
        img->data[i] = fminf(img->data[i] * factor, 1.0f);
    }

    return IF_SUCCESS;
}
