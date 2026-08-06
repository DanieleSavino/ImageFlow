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
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define IF_STD_QUALITY 90

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
 * @brief Stores an IF_image_t to disk, dispatching on file extension.
 *
 * Converts the internal float buffer to 8-bit and writes via stb_image_write.
 * Supported extensions (case-insensitive): .png, .bmp, .jpg, .jpeg.
 *
 * @param img  Source image. Must not be NULL and must have valid data.
 * @param path Null-terminated output path including extension. Must not be NULL.
 * @param path quality factor for JPEG export.
 * @return IF_SUCCESS on success.
 * @return IF_NULL_POINTER if @p img, @p img->data, or @p path is NULL.
 * @return IF_UNSUPPORTED_FORMAT if the extension is missing or unrecognized.
 * @return IF_OUT_OF_MEMORY if the intermediate byte buffer allocation fails.
 * @return IF_FILE_WRITE_ERROR if the underlying write call fails.
 */
NODISCARD IF_error_t IF_storeImageQ(IF_image_t *img, const char *path, int quality);

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
 * @brief Fills img with pseudo-random pixel values in [0.0, 1.0], seeded.
 *
 * Deterministic: identical (width, height, channels, seed) always produces
 * identical pixel data, regardless of platform or libc.
 *
 * @param img      Output struct to populate. Must not be NULL.
 * @param width    Image width in pixels. Must be > 0.
 * @param height   Image height in pixels. Must be > 0.
 * @param channels Channels per pixel. Must be > 0.
 * @param seed     PRNG seed. Any 64-bit value; 0 is valid.
 * @return IF_SUCCESS on success.
 * @return IF_NULL_POINTER if @p img is NULL.
 * @return IF_OUT_OF_MEMORY if buffer allocation fails.
 *
 * @note The caller is responsible for releasing @p img via IF_freeImage.
 */
NODISCARD IF_error_t IF_genRandomImage(IF_image_t *img, int width, int height,
                                        int channels, uint64_t seed);

/**
 * @brief Compares two images for equality within a per-channel tolerance.
 *
 * Images are considered equal if they have matching dimensions and every
 * corresponding pixel/channel value differs by no more than @p tolerance.
 * Useful for validating lossy round trips (e.g. float -> 8-bit -> float)
 * where exact bitwise equality is not expected.
 *
 * @param a         First image. Must not be NULL and must have valid data.
 * @param b         Second image. Must not be NULL and must have valid data.
 * @param tolerance Maximum allowed absolute per-value difference. Must be >= 0.
 * @param out_equal Output: set to 1 if images are equal within tolerance,
 *                  0 otherwise. Must not be NULL.
 * @return IF_SUCCESS on success (regardless of comparison result).
 * @return IF_NULL_POINTER if @p a, @p a->data, @p b, @p b->data, or
 *         @p out_equal is NULL.
 * @return IF_ERROR_ASSERT if dimensions (width/height/channels) differ;
 *         @p out_equal is set to 0 in this case rather than treated as
 *         a hard failure.
 *
 * @note This performs an early-exit scan; on mismatch it stops at the
 *       first differing value rather than always scanning the full buffer.
 */
NODISCARD IF_error_t IF_imageCompare(const IF_image_t *a, const IF_image_t *b,
                                      float tolerance, int *out_equal);

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
