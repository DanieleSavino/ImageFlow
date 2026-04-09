/**
 * @file backend.h
 * @brief HIP GPU backend for ImageFlow image processing operations (AMD ROCm).
 *
 * Mirrors the CUDA backend API with HIP equivalents, targeting AMD GPUs via ROCm.
 * Kernels are launched through hipLaunchKernelGGL with a 16x16 thread block grid.
 *
 * Provides two usage patterns for each operation:
 *
 * @verbatim
 * 1. Single-shot (IFHIP_grayScale / IFHIP_invert / IFHIP_brightness):
 *    Internally calls IFHIP_load -> kernel -> IFHIP_retrieve.
 *    Convenient but pays the H2D + D2H transfer cost every call.
 *
 * 2. GPU-resident (IFHIP_load -> IFHIP_loaded_* -> IFHIP_retrieve):
 *    Upload once, run multiple kernels, download once.
 *    Preferred when chaining operations on the same image.
 * @endverbatim
 *
 * All pixel operations apply to the first three channels (RGB); additional
 * channels are left untouched. Pixel values are normalized to [0.0, 1.0].
 *
 * @author Daniele Savino <daniele.savino0@gmail.com>
 *
 * SPDX-License-Identifier: MIT
 * Copyright (c) 2026 Daniele Savino
 */
#pragma once

#include "ImageFlow/error.h"
#include "ImageFlow/io/image.h"

/**
 * @brief Checks a HIP API call and returns IF_HIP_ERROR on failure.
 *
 * Prints the HIP error string, file, and line to stderr before returning.
 * Intended for use around any hipXxx() API call inside a function returning
 * IF_error_t.
 *
 * @param call A HIP runtime API call expression returning hipError_t.
 */
#define IF_HIP_CHECK(call)                                                       \
    do {                                                                         \
        hipError_t __err__ = call;                                               \
        if (__err__ != hipSuccess) {                                             \
            fprintf(stderr,                                                      \
                    "HIP Error: %s in file %s at line %d\n",                    \
                    hipGetErrorString(__err__),                                  \
                    __FILE__,                                                    \
                    __LINE__);                                                   \
            return IF_HIP_ERROR;                                                 \
        }                                                                        \
    } while (0)

/**
 * @brief Checks the last HIP kernel launch for errors, returning IF_HIP_ERROR on failure.
 *
 * Calls hipGetLastError() to detect launch configuration errors.
 * Must be called immediately after hipLaunchKernelGGL before any other HIP call
 * that could clear the error state.
 *
 * @note Unlike the CUDA backend, this macro does not conditionally synchronize
 *       in DEBUG builds. Execution errors are caught by the explicit
 *       hipDeviceSynchronize() that follows each kernel launch.
 */
#define IF_HIP_KERNEL_CHECK()                                                    \
    do {                                                                         \
        hipError_t __err__ = hipGetLastError();                                  \
        if (__err__ != hipSuccess) {                                             \
            fprintf(stderr,                                                      \
                    "HIP Kernel Launch Error: %s in file %s at line %d\n",      \
                    hipGetErrorString(__err__),                                  \
                    __FILE__,                                                    \
                    __LINE__);                                                   \
            return IF_HIP_ERROR;                                                 \
        }                                                                        \
    } while (0)

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Queries the number of available HIP devices.
 *
 * @param hip_dev Output: number of HIP-capable devices on the system.
 * @return IF_SUCCESS always.
 */
NODISCARD IF_error_t IFHIP_getDevices(int *hip_dev);

/**
 * @brief Uploads a host IF_image_t to device memory.
 *
 * Allocates a device-side IF_image_t struct and a separate device float buffer,
 * copies the pixel data host-to-device, then patches the device struct's data
 * pointer to the device buffer.
 *
 * @param img     Source host image. Must not be NULL, must have valid data
 *                and at least 3 channels.
 * @param hip_img Output: pointer to the newly allocated device IF_image_t.
 *                The caller is responsible for releasing it via IFHIP_retrieve.
 * @return IF_SUCCESS on success.
 * @return IF_NULL_POINTER if @p img or @p img->data is NULL.
 * @return IF_INVALID_CHANNELS if @p img->channels < 3.
 * @return IF_HIP_ERROR on any HIP allocation or transfer failure.
 *
 * @note The host @p img->data pointer is not freed; only a copy is uploaded.
 */
NODISCARD IF_error_t IFHIP_load(const IF_image_t *img, IF_image_t **hip_img);

/**
 * @brief Downloads a device IF_image_t back to host memory and frees device resources.
 *
 * Copies the device struct and pixel buffer to host, frees the device pixel
 * buffer and device struct, and sets *hip_img to NULL.
 *
 * @param hip_img Pointer to the device IF_image_t pointer produced by IFHIP_load.
 *                Set to NULL on return.
 * @param img_out Output host image struct to populate. The pixel buffer is
 *                heap-allocated; the caller is responsible for freeing it
 *                via IF_freeImage.
 * @return IF_SUCCESS on success.
 * @return IF_HIP_ERROR on any HIP transfer or free failure.
 *
 * @warning @p img_out must not have an existing live data pointer; no prior
 *          buffer is freed before overwriting img_out->data.
 */
NODISCARD IF_error_t IFHIP_retrieve(IF_image_t **hip_img, IF_image_t *img_out);

/**
 * @brief Converts an image to grayscale using Rec. 601 luminance coefficients (single-shot).
 *
 * Uploads the image, runs the grayscale kernel, and retrieves the result.
 * Prefer IFHIP_loaded_grayScale when chaining multiple operations.
 *
 * @param img Image to convert in-place. Must not be NULL and must have valid
 *            data and at least 3 channels.
 * @return IF_SUCCESS on success.
 * @return IF_NULL_POINTER, IF_INVALID_CHANNELS, or IF_HIP_ERROR on failure.
 */
NODISCARD IF_error_t IFHIP_grayScale(IF_image_t *img);

/**
 * @brief Inverts all RGB channel values of an image (single-shot).
 *
 * Uploads the image, runs the invert kernel (v' = 1.0 - v), and retrieves the result.
 * Prefer IFHIP_loaded_invert when chaining multiple operations.
 *
 * @param img Image to invert in-place. Must not be NULL and must have valid
 *            data and at least 3 channels.
 * @return IF_SUCCESS on success.
 * @return IF_NULL_POINTER, IF_INVALID_CHANNELS, or IF_HIP_ERROR on failure.
 */
NODISCARD IF_error_t IFHIP_invert(IF_image_t *img);

/**
 * @brief Scales RGB channel values by a constant factor, clamping to 1.0 (single-shot).
 *
 * Uploads the image, runs the brightness kernel (v' = fminf(v * factor, 1.0f)),
 * and retrieves the result.
 * Prefer IFHIP_loaded_brightness when chaining multiple operations.
 *
 * @param img    Image to adjust in-place. Must not be NULL and must have valid
 *               data and at least 3 channels.
 * @param factor Multiplicative brightness factor. Output is clamped to [0.0, 1.0].
 * @return IF_SUCCESS on success.
 * @return IF_NULL_POINTER, IF_INVALID_CHANNELS, or IF_HIP_ERROR on failure.
 */
NODISCARD IF_error_t IFHIP_brightness(IF_image_t *img, float factor);

/**
 * @brief Runs the grayscale kernel on an already-loaded device image.
 *
 * @param img     Host image, used only to read width and height for grid sizing.
 * @param hip_img Device IF_image_t produced by IFHIP_load. Must not be NULL.
 * @return IF_SUCCESS on success.
 * @return IF_NULL_POINTER if @p hip_img is NULL.
 * @return IF_HIP_ERROR on kernel launch or sync failure.
 */
NODISCARD IF_error_t IFHIP_loaded_grayScale(IF_image_t *img, IF_image_t *hip_img);

/**
 * @brief Runs the invert kernel on an already-loaded device image.
 *
 * @param img     Host image, used only to read width and height for grid sizing.
 * @param hip_img Device IF_image_t produced by IFHIP_load. Must not be NULL.
 * @return IF_SUCCESS on success.
 * @return IF_NULL_POINTER if @p hip_img is NULL.
 * @return IF_HIP_ERROR on kernel launch or sync failure.
 */
NODISCARD IF_error_t IFHIP_loaded_invert(IF_image_t *img, IF_image_t *hip_img);

/**
 * @brief Runs the brightness kernel on an already-loaded device image.
 *
 * @param img     Host image, used only to read width and height for grid sizing.
 * @param hip_img Device IF_image_t produced by IFHIP_load. Must not be NULL.
 * @param factor  Multiplicative brightness factor. Output is clamped to [0.0, 1.0].
 * @return IF_SUCCESS on success.
 * @return IF_NULL_POINTER if @p hip_img is NULL.
 * @return IF_HIP_ERROR on kernel launch or sync failure.
 */
NODISCARD IF_error_t IFHIP_loaded_brightness(IF_image_t *img, IF_image_t *hip_img, float factor);

#ifdef __cplusplus
}
#endif
