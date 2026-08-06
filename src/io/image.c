#include "ImageFlow/io/image.h"
#include "ImageFlow/error.h"
#include <stdatomic.h>
#include <stddef.h>
#include <string.h>

IF_error_t IF_loadImageFallback(IF_image_t *img, const char *path) {
    int w, h, c;
    unsigned char *data_u8 = stbi_load(path, &w, &h, &c, 0);
    if (!data_u8) return IF_FILE_READ_ERROR;

    // allocate float buffer
    float *data = malloc(w * h * c * sizeof(float));
    if(!data) {
        stbi_image_free(data_u8);
        return IF_OUT_OF_MEMORY;
    }

    #pragma omp parallel for simd schedule(static)
    for (size_t i = 0; i < w * h * c; i++)
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

    stbi_ldr_to_hdr_gamma(1.0f);
    stbi_ldr_to_hdr_scale(1.0f);

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
    unsigned char *bytes = malloc(n * sizeof(float));
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

IF_error_t IF_storeImageQ(IF_image_t *img, const char *path, int quality) {
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
        success = stbi_write_jpg(path, img->width, img->height, img->channels, bytes, quality);
    } else {
        free(bytes);
        return IF_UNSUPPORTED_FORMAT;
    }

    free(bytes);
    if (!success) return IF_FILE_WRITE_ERROR;

    return IF_SUCCESS;
}

IF_error_t IF_storeImage(IF_image_t *img, const char *path) {
    return IF_storeImageQ(img, path, IF_STD_QUALITY);
}

NODISCARD IF_error_t IF_copyImage(const IF_image_t *img_in, IF_image_t *img_out) {
    if (!img_in || !img_out || !img_in->data)
        return IF_NULL_POINTER;

    img_out->width = img_in->width;
    img_out->height = img_in->height;
    img_out->channels = img_in->channels;

    size_t total_elements = (size_t)img_out->width * img_out->height * img_out->channels;
    size_t total_size = total_elements * sizeof(float);

    img_out->data = malloc(total_size);
    if (!img_out->data) return IF_OUT_OF_MEMORY;

    memcpy(img_out->data, img_in->data, total_elements * sizeof(float));

    return IF_SUCCESS;
}

/* splitmix64 — used only to seed/spread the xorshift64* state from a
 * single user seed; avoids weak initial states (e.g. seed=0) propagating
 * into visibly patterned output. */
static inline uint64_t splitmix64_next(uint64_t *state) {
    uint64_t z = (*state += 0x9E3779B97F4A7C15ULL);
    z = (z ^ (z >> 30)) * 0xBF58476D1CE4E5B9ULL;
    z = (z ^ (z >> 27)) * 0x94D049BB133111EBULL;
    return z ^ (z >> 31);
}

static inline uint64_t xorshift64star_next(uint64_t *state) {
    uint64_t x = *state;
    x ^= x >> 12;
    x ^= x << 25;
    x ^= x >> 27;
    *state = x;
    return x * 0x2545F4914F6CDD1DULL;
}

NODISCARD IF_error_t IF_genRandomImage(IF_image_t *img, int width, int height,
                                        int channels, uint64_t seed) {
    if (!img) return IF_NULL_POINTER;
    if (width <= 0 || height <= 0 || channels <= 0) return IF_NULL_POINTER;

    size_t total_elements = (size_t)width * height * channels;
    float *data = malloc(total_elements * sizeof(float));
    if (!data) return IF_OUT_OF_MEMORY;

    /* spread the raw seed through splitmix64 before feeding xorshift64*,
     * so adjacent/small seeds (0, 1, 2, ...) don't produce correlated
     * initial states. */
    uint64_t sm_state = seed;
    uint64_t state = splitmix64_next(&sm_state);
    if (state == 0) state = 0x9E3779B97F4A7C15ULL; /* xorshift64* requires nonzero state */

    for (size_t i = 0; i < total_elements; ++i) {
        uint64_t r = xorshift64star_next(&state);
        /* top 24 bits -> float in [0.0, 1.0] */
        data[i] = (float)(r >> 40) / (float)(1u << 24);
    }

    img->width = width;
    img->height = height;
    img->channels = channels;
    img->data = data;
    return IF_SUCCESS;
}

NODISCARD IF_error_t IF_imageCompare(const IF_image_t *a, const IF_image_t *b,
                                      float tolerance, int *out_equal) {
    if (!a || !a->data || !b || !b->data || !out_equal) return IF_NULL_POINTER;
    if (tolerance < 0.0f) return IF_NULL_POINTER;

    *out_equal = 0;

    if (a->width != b->width || a->height != b->height || a->channels != b->channels)
        return IF_SUCCESS; /* dimension mismatch -> not equal, not an error */

    size_t n = (size_t)a->width * a->height * a->channels;
    int mismatch = 0;

    #pragma omp parallel for simd schedule(static) reduction(||:mismatch)
    for (size_t i = 0; i < n; ++i) {
        float diff = a->data[i] - b->data[i];
        if (diff < 0.0f) diff = -diff;
        if (diff > tolerance) mismatch = 1;
    }

    *out_equal = !mismatch;
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
