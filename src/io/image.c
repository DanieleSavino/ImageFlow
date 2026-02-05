#include "ImageFlow/io/image.h"
#include "ImageFlow/error/error.h"
#include <string.h>

IF_error_t IF_loadImageFallback(IF_image_t *img, const char *path) {
    int w, h, c;
    unsigned char *data_u8 = stbi_load(path, &w, &h, &c, 0);
    if (!data_u8) return IF_FILE_READ_ERROR;

    // allocate float buffer
    float *data = malloc(w * h * c * sizeof(float));
    if(!data) {
        return IF_OUT_OF_MEMORY;
    }

    #pragma omp parallel for simd schedule(static)
    for (int i = 0; i < w * h * c; i++)
        data[i] = data_u8[i] / 255.0f;

    stbi_image_free(data_u8);

    img->width = w;
    img->height = h;
    img->channels = c;
    img->data = data;

    return IF_SUCCESS;
}

IF_error_t IF_loadImage(IF_image_t *img, const char *path) {
    if (!img || !path) return IF_NULL_POINTER;

    int w, h, c;
    float *data = stbi_loadf(path, &w, &h, &c, 0);
    if (!data) return IF_loadImageFallback(img, path);

    img->width = w;
    img->height = h;
    img->channels = c;
    img->data = data;

    return IF_SUCCESS;
}

static inline unsigned char *float_to_bytes(const float *data, int n) {
    unsigned char *bytes = malloc(n);
    if (!bytes) return NULL;

    #pragma omp parallel for simd schedule(static)
    for (int i = 0; i < n; ++i) {
        float v = data[i];
        if (v < 0.0f) v = 0.0f;
        if (v > 1.0f) v = 1.0f;
        bytes[i] = (unsigned char)(v * 255.0f + 0.5f);
    }
    return bytes;
}

IF_error_t IF_storeImage(IF_image_t *img, const char *path) {
    if (!img || !img->data || !path) return IF_NULL_POINTER;

    const char *ext = strrchr(path, '.');
    if (!ext) return IF_UNSUPPORTED_FORMAT;

    unsigned char *bytes = float_to_bytes(img->data, img->width * img->height * img->channels);
    if (!bytes) return IF_OUT_OF_MEMORY;

    int success = 0;
    if (strcasecmp(ext, ".png") == 0) {
        success = stbi_write_png(path, img->width, img->height, img->channels, bytes, img->width * img->channels);
    } else if (strcasecmp(ext, ".bmp") == 0) {
        success = stbi_write_bmp(path, img->width, img->height, img->channels, bytes);
    } else if (strcasecmp(ext, ".jpg") == 0 || strcasecmp(ext, ".jpeg") == 0) {
        success = stbi_write_jpg(path, img->width, img->height, img->channels, bytes, 90);
    } else {
        free(bytes);
        return IF_UNSUPPORTED_FORMAT;
    }

    free(bytes);
    if (!success) return IF_FILE_WRITE_ERROR;

    return IF_SUCCESS;
}

IF_error_t IF_freeImage(IF_image_t *img) {
    if (!img) return IF_NULL_POINTER;

    if (img->data) {
        free(img->data);
        img->data = NULL;
    }

    img->width = 0;
    img->height = 0;
    img->channels = 0;

    return IF_SUCCESS;
}
