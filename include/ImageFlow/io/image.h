#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include "ImageFlow/error/error.h"
#include "ImageFlow/stb/stb_image.h"
#include "ImageFlow/stb/stb_image_write.h"

typedef struct {
    int width;
    int height;
    int channels;
    float *data;
} IF_image_t;

IF_error_t IF_loadImage(IF_image_t *img, const char *path);

IF_error_t IF_storeImage(IF_image_t *img, const char *path);

IF_error_t IF_freeImage(IF_image_t *img);

#ifdef __cplusplus
}
#endif
