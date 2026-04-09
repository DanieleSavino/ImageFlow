/**
 * @file backend.h
 * @brief OpenMP CPU backend for ImageFlow image processing operations.
 *
 * Provides SIMD-accelerated, parallelized image filters operating directly
 * on the float pixel buffer of an IF_image_t. All operations are in-place.
 * Pixel values are assumed to be normalized to [0.0, 1.0] per channel.
 *
 * All functions require at least 3 channels (RGB); images with fewer
 * channels will return IF_INVALID_CHANNELS.
 *
 * @author Daniele Savino <daniele.savino0@gmail.com>
 *
 * SPDX-License-Identifier: MIT
 * Copyright (c) 2026 Daniele Savino
 */
#pragma once

#include "ImageFlow/error.h"
#include "ImageFlow/io/image.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Converts an image to grayscale in-place using Rec. 601 luminance coefficients.
 *
 * Computes Y = 0.299·R + 0.587·G + 0.114·B for each pixel and writes the
 * result back to all three channels. Alpha and any additional channels beyond
 * index 2 are left untouched.
 *
 * @param img Image to convert. Must not be NULL, must have valid data, and
 *            must have at least 3 channels.
 * @return IF_SUCCESS on success.
 * @return IF_NULL_POINTER if @p img or @p img->data is NULL.
 * @return IF_INVALID_CHANNELS if @p img->channels < 3.
 */
NODISCARD IF_error_t IFOMP_grayScale(IF_image_t *img);

/**
 * @brief Inverts all channel values of an image in-place.
 *
 * Applies v' = 1.0 - v to every element of the pixel buffer.
 * Values are not clamped; inputs outside [0.0, 1.0] will produce
 * correspondingly out-of-range results.
 *
 * @param img Image to invert. Must not be NULL and must have valid data
 *            and at least 3 channels.
 * @return IF_SUCCESS on success.
 * @return IF_NULL_POINTER if @p img or @p img->data is NULL.
 * @return IF_INVALID_CHANNELS if @p img->channels < 3.
 */
NODISCARD IF_error_t IFOMP_invert(IF_image_t *img);

/**
 * @brief Scales all channel values of an image by a constant factor in-place.
 *
 * Applies v' = v * factor to every element of the pixel buffer.
 * Values are not clamped after multiplication; callers should ensure
 * @p factor keeps values within [0.0, 1.0] if normalized output is required.
 *
 * @param img    Image to adjust. Must not be NULL and must have valid data
 *               and at least 3 channels.
 * @param factor Multiplicative brightness factor. Values > 1.0 brighten,
 *               values in (0.0, 1.0) darken, negative values invert sign.
 * @return IF_SUCCESS on success.
 * @return IF_NULL_POINTER if @p img or @p img->data is NULL.
 * @return IF_INVALID_CHANNELS if @p img->channels < 3.
 */
NODISCARD IF_error_t IFOMP_brightness(IF_image_t *img, float factor);

#ifdef __cplusplus
}
#endif
