#include "ImageFlow/io/image_io.h"
#include <string.h>

IF_error_t IF_loadImage(IF_image_t *img, const char *path) {
    if (!img || !path) return IF_NULL_POINTER;

    int w, h, c;
    float *data = stbi_loadf(path, &w, &h, &c, 0);
    if (!data) return IF_FILE_READ_ERROR;

    img->width = w;
    img->height = h;
    img->channels = c;
    img->data = data;

    return IF_SUCCESS;
}

IF_error_t IF_storeImage(IF_image_t *img, const char *path) {
    if (!img || !img->data || !path) return IF_NULL_POINTER;

    int success = 0;
    // Decide format from file extension
    const char *ext = strrchr(path, '.');
    if (!ext) return IF_UNSUPPORTED_FORMAT;

    if (strcasecmp(ext, ".png") == 0) {
        // stb_image_write expects unsigned char -> convert float to byte
        unsigned char *bytes = malloc(img->width * img->height * img->channels);
        if (!bytes) return IF_OUT_OF_MEMORY;

        for (int i = 0; i < img->width * img->height * img->channels; ++i) {
            float v = img->data[i];
            if (v < 0.0f) v = 0.0f;
            if (v > 1.0f) v = 1.0f;
            bytes[i] = (unsigned char)(v * 255.0f);
        }

        success = stbi_write_png(path, img->width, img->height, img->channels, bytes, img->width * img->channels);
        free(bytes);
        if (!success) return IF_FILE_WRITE_ERROR;

    } else if (strcasecmp(ext, ".bmp") == 0) {
        unsigned char *bytes = malloc(img->width * img->height * img->channels);
        if (!bytes) return IF_OUT_OF_MEMORY;

        for (int i = 0; i < img->width * img->height * img->channels; ++i) {
            float v = img->data[i];
            if (v < 0.0f) v = 0.0f;
            if (v > 1.0f) v = 1.0f;
            bytes[i] = (unsigned char)(v * 255.0f);
        }

        success = stbi_write_bmp(path, img->width, img->height, img->channels, bytes);
        free(bytes);
        if (!success) return IF_FILE_WRITE_ERROR;

    } else {
        return IF_UNSUPPORTED_FORMAT;
    }

    return IF_SUCCESS;
}

IF_error_t IF_freeImage(IF_image_t *img) {
    if (!img) return IF_NULL_POINTER;

    if (img->data) {
        stbi_image_free(img->data);
        img->data = NULL;
    }

    img->width = 0;
    img->height = 0;
    img->channels = 0;

    return IF_SUCCESS;
}
