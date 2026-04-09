/**
 * @file image.h
 * @brief Core image type and I/O interface for ImageFlow.
 *
 * Defines IF_image_t, the central image representation used throughout
 * the library. Pixel data is stored as a packed, row-major float buffer
 * with values normalized to [0.0, 1.0] per channel.
 *
 * Provides the public API for loading from disk, storing to disk,
 * deep-copying, and freeing images. Loading dispatches through stb_image
 * with an automatic 8-bit fallback; storing dispatches on file extension
 * via stb_image_write. Supported formats: PNG, BMP, JPG/JPEG.
 *
 * @author Daniele Savino <daniele.savino0@gmail.com>
 *
 * SPDX-License-Identifier: MIT
 * Copyright (c) 2026 Daniele Savino
 */
#pragma once

#include "ImageFlow/error.h"
#include "ImageFlow/stb/stb_image.h"
#include "ImageFlow/stb/stb_image_write.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Represents a decoded image with a normalized float pixel buffer.
 *
 * Pixel data is stored in row-major order as a contiguous float array of
 * length width * height * channels, with each sample in [0.0, 1.0].
 *
 * @note Instances are typically stack-allocated; only the @p data buffer
 *       is heap-allocated. Use IF_freeImage to release it.
 */
typedef struct {
    int width;    /**< Image width in pixels. */
    int height;   /**< Image height in pixels. */
    int channels; /**< Number of channels per pixel (e.g. 1=grey, 3=RGB, 4=RGBA). */
    float *data;  /**< Heap-allocated pixel buffer of length width * height * channels. */
} IF_image_t;

/**
 * @brief Loads an image from disk into an IF_image_t.
 *
 * Attempts native float loading via stbi_loadf; falls back to 8-bit
 * loading with float conversion for LDR-only formats.
 *
 * @param img  Output struct to populate. Must not be NULL.
 * @param path Null-terminated path to the source file. Must not be NULL.
 * @return IF_SUCCESS on success.
 * @return IF_NULL_POINTER if @p img or @p path is NULL.
 * @return IF_FILE_READ_ERROR if the file cannot be decoded.
 * @return IF_OUT_OF_MEMORY if buffer allocation fails.
 *
 * @note The caller is responsible for releasing @p img via IF_freeImage.
 */
NODISCARD IF_error_t IF_loadImage(IF_image_t *img, const char *path);

/**
 * @brief Stores an IF_image_t to disk, dispatching on file extension.
 *
 * Converts the internal float buffer to 8-bit and writes via stb_image_write.
 * Supported extensions (case-insensitive): .png, .bmp, .jpg, .jpeg.
 * JPEG quality is fixed at 90.
 *
 * @param img  Source image. Must not be NULL and must have valid data.
 * @param path Null-terminated output path including extension. Must not be NULL.
 * @return IF_SUCCESS on success.
 * @return IF_NULL_POINTER if @p img, @p img->data, or @p path is NULL.
 * @return IF_UNSUPPORTED_FORMAT if the extension is missing or unrecognized.
 * @return IF_OUT_OF_MEMORY if the intermediate byte buffer allocation fails.
 * @return IF_FILE_WRITE_ERROR if the underlying write call fails.
 */
NODISCARD IF_error_t IF_storeImage(IF_image_t *img, const char *path);

/**
 * @brief Deep-copies an IF_image_t, allocating a new float buffer.
 *
 * Copies all metadata fields and duplicates the pixel buffer via memcpy.
 *
 * @param img_in  Source image. Must not be NULL and must have valid data.
 * @param img_out Destination struct to populate. Must not be NULL.
 * @return IF_SUCCESS on success.
 * @return IF_NULL_POINTER if @p img_in, @p img_out, or @p img_in->data is NULL.
 * @return IF_OUT_OF_MEMORY if the destination buffer allocation fails.
 *
 * @note The caller is responsible for releasing @p img_out via IF_freeImage.
 */
NODISCARD IF_error_t IF_copyImage(const IF_image_t *img_in, IF_image_t *img_out);

/**
 * @brief Frees the pixel buffer of an IF_image_t and zeroes its fields.
 *
 * Releases @p img->data and sets all fields to zero/NULL.
 * The IF_image_t struct itself is not freed.
 *
 * @param img Image to free. Must not be NULL.
 * @return IF_SUCCESS on success.
 * @return IF_NULL_POINTER if @p img is NULL.
 *
 * @note Safe to call when @p img->data is already NULL.
 */
NODISCARD IF_error_t IF_freeImage(IF_image_t *img);

#ifdef __cplusplus
}
#endif
