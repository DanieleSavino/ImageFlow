/**
 * @file backend.h
 * @brief CUDA GPU backend for ImageFlow image processing operations.
 *
 * Provides two usage patterns for each operation:
 *
 * @verbatim
 * 1. Single-shot (IFCU_grayScale / IFCU_invert / IFCU_brightness):
 *    Internally calls IFCU_load -> kernel -> IFCU_retrieve.
 *    Convenient but pays the H2D + D2H transfer cost every call.
 *
 * 2. GPU-resident (IFCU_load -> IFCU_loaded_* -> IFCU_retrieve):
 *    Upload once, run multiple kernels, download once.
 *    Preferred when chaining operations on the same image.
 * @endverbatim
 *
 * All pixel operations use a 16x16 thread block grid covering the full image.
 * Kernels operate on the first three channels (RGB); additional channels are
 * left untouched. Pixel values are normalized to [0.0, 1.0].
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
 * @brief Checks a CUDA API call and returns IF_CUDA_ERROR on failure.
 *
 * Prints the CUDA error string, file, and line to stderr before returning.
 * Intended for use around any cudaXxx() API call inside a function returning
 * IF_error_t.
 *
 * @param call A CUDA runtime API call expression returning cudaError_t.
 */
#define IF_CUDA_CHECK(call)                                                    \
    do {                                                                       \
        cudaError_t __err__ = call;                                            \
        if (__err__ != cudaSuccess) {                                          \
            fprintf(stderr,                                                    \
                    "CUDA Error: %s in file %s at line %d\n",                  \
                    cudaGetErrorString(__err__),                                \
                    __FILE__,                                                  \
                    __LINE__);                                                  \
            return IF_CUDA_ERROR;                                              \
        }                                                                      \
    } while (0)

/**
 * @brief Checks the last CUDA kernel launch for errors, returning IF_KERNEL_FAILURE on failure.
 *
 * In DEBUG builds: calls cudaGetLastError() followed by cudaDeviceSynchronize(),
 * catching both launch configuration errors and runtime execution errors.
 * In release builds: calls cudaGetLastError() only; execution errors are
 * caught by the explicit cudaDeviceSynchronize() that follows each kernel launch.
 *
 * @note Must be called immediately after a kernel launch (<<<...>>>) before
 *       any other CUDA call that could clear the error state.
 */
#ifdef DEBUG
    #define IF_CUDA_KERNEL_CHECK()                                             \
        do {                                                                   \
            cudaError_t __err = cudaGetLastError();                            \
            if (__err != cudaSuccess) {                                        \
                fprintf(stderr, "CUDA Launch Error [%s]: %s\nFile: %s | Line: %d\n", \
                        cudaGetErrorName(__err), cudaGetErrorString(__err),    \
                        __FILE__, __LINE__);                                   \
                return IF_KERNEL_FAILURE;                                      \
            }                                                                  \
            __err = cudaDeviceSynchronize();                                   \
            if (__err != cudaSuccess) {                                        \
                fprintf(stderr, "CUDA Execution Error [%s]: %s\nFile: %s | Line: %d\n", \
                        cudaGetErrorName(__err), cudaGetErrorString(__err),    \
                        __FILE__, __LINE__);                                   \
                return IF_KERNEL_FAILURE;                                      \
            }                                                                  \
        } while (0)
#else
    #define IF_CUDA_KERNEL_CHECK()                                             \
        do {                                                                   \
            cudaError_t __err = cudaGetLastError();                            \
            if (__err != cudaSuccess) {                                        \
                fprintf(stderr, "CUDA Launch Error [%s]: %s\nFile: %s | Line: %d\n", \
                        cudaGetErrorName(__err), cudaGetErrorString(__err),    \
                        __FILE__, __LINE__);                                   \
                return IF_KERNEL_FAILURE;                                      \
            }                                                                  \
        } while (0)
#endif

/**
 * @brief Queries the number of available CUDA devices.
 *
 * @param cuda_dev Output: number of CUDA-capable devices on the system.
 * @return IF_SUCCESS always.
 */
NODISCARD IF_error_t IFCU_getDevices(int *cuda_dev);

/**
 * @brief Uploads a host IF_image_t to device memory.
 *
 * Allocates a device-side IF_image_t struct and a separate device float buffer,
 * copies the pixel data host-to-device, then patches the device struct's data
 * pointer to the device buffer.
 *
 * @param img      Source host image. Must not be NULL, must have valid data
 *                 and at least 3 channels.
 * @param cuda_img Output: pointer to the newly allocated device IF_image_t.
 *                 Must not be NULL. The caller is responsible for releasing it
 *                 via IFCU_retrieve.
 * @return IF_SUCCESS on success.
 * @return IF_NULL_POINTER if @p img or @p img->data is NULL.
 * @return IF_INVALID_CHANNELS if @p img->channels < 3.
 * @return IF_CUDA_ERROR on any CUDA allocation or transfer failure.
 *
 * @note The host @p img->data pointer is not freed; only a copy is uploaded.
 */
NODISCARD IF_error_t IFCU_load(const IF_image_t *img, IF_image_t **cuda_img);

/**
 * @brief Downloads a device IF_image_t back to host memory and frees device resources.
 *
 * Copies the device struct and pixel buffer to host, frees the device pixel
 * buffer and device struct, and sets *cuda_img to NULL.
 *
 * @param cuda_img Pointer to the device IF_image_t pointer produced by IFCU_load.
 *                 Set to NULL on return.
 * @param img_out  Output host image struct to populate. The pixel buffer is
 *                 heap-allocated; the caller is responsible for freeing it
 *                 via IF_freeImage.
 * @return IF_SUCCESS on success.
 * @return IF_CUDA_ERROR on any CUDA transfer or free failure.
 */
NODISCARD IF_error_t IFCU_retrieve(IF_image_t **cuda_img, IF_image_t *img_out);

/**
 * @brief Converts an image to grayscale using Rec. 601 luminance coefficients (single-shot).
 *
 * Uploads the image, runs the grayscale kernel, and retrieves the result.
 * Prefer IFCU_loaded_grayScale when chaining multiple operations.
 *
 * @param img Image to convert in-place. Must not be NULL and must have valid
 *            data and at least 3 channels.
 * @return IF_SUCCESS on success.
 * @return IF_NULL_POINTER, IF_INVALID_CHANNELS, IF_CUDA_ERROR, or IF_KERNEL_FAILURE on failure.
 */
NODISCARD IF_error_t IFCU_grayScale(IF_image_t *img);

/**
 * @brief Inverts all RGB channel values of an image (single-shot).
 *
 * Uploads the image, runs the invert kernel (v' = 1.0 - v), and retrieves the result.
 * Prefer IFCU_loaded_invert when chaining multiple operations.
 *
 * @param img Image to invert in-place. Must not be NULL and must have valid
 *            data and at least 3 channels.
 * @return IF_SUCCESS on success.
 * @return IF_NULL_POINTER, IF_INVALID_CHANNELS, IF_CUDA_ERROR, or IF_KERNEL_FAILURE on failure.
 */
NODISCARD IF_error_t IFCU_invert(IF_image_t *img);

/**
 * @brief Scales RGB channel values by a constant factor, clamping to 1.0 (single-shot).
 *
 * Uploads the image, runs the brightness kernel (v' = fminf(v * factor, 1.0f)),
 * and retrieves the result.
 * Prefer IFCU_loaded_brightness when chaining multiple operations.
 *
 * @param img    Image to adjust in-place. Must not be NULL and must have valid
 *               data and at least 3 channels.
 * @param factor Multiplicative brightness factor. Output is clamped to [0.0, 1.0].
 * @return IF_SUCCESS on success.
 * @return IF_NULL_POINTER, IF_INVALID_CHANNELS, IF_CUDA_ERROR, or IF_KERNEL_FAILURE on failure.
 */
NODISCARD IF_error_t IFCU_brightness(IF_image_t *img, float factor);

/**
 * @brief Runs the grayscale kernel on an already-loaded device image.
 *
 * @param img      Host image, used only to read width and height for grid sizing.
 * @param cuda_img Device IF_image_t produced by IFCU_load. Must not be NULL.
 * @return IF_SUCCESS on success.
 * @return IF_NULL_POINTER if @p cuda_img is NULL.
 * @return IF_CUDA_ERROR or IF_KERNEL_FAILURE on kernel or sync failure.
 */
NODISCARD IF_error_t IFCU_loaded_grayScale(IF_image_t *img, IF_image_t *cuda_img);

/**
 * @brief Runs the invert kernel on an already-loaded device image.
 *
 * @param img      Host image, used only to read width and height for grid sizing.
 * @param cuda_img Device IF_image_t produced by IFCU_load. Must not be NULL.
 * @return IF_SUCCESS on success.
 * @return IF_NULL_POINTER if @p cuda_img is NULL.
 * @return IF_CUDA_ERROR or IF_KERNEL_FAILURE on kernel or sync failure.
 */
NODISCARD IF_error_t IFCU_loaded_invert(IF_image_t *img, IF_image_t *cuda_img);

/**
 * @brief Runs the brightness kernel on an already-loaded device image.
 *
 * @param img      Host image, used only to read width and height for grid sizing.
 * @param cuda_img Device IF_image_t produced by IFCU_load. Must not be NULL.
 * @param factor   Multiplicative brightness factor. Output is clamped to [0.0, 1.0].
 * @return IF_SUCCESS on success.
 * @return IF_NULL_POINTER if @p cuda_img is NULL.
 * @return IF_CUDA_ERROR or IF_KERNEL_FAILURE on kernel or sync failure.
 */
NODISCARD IF_error_t IFCU_loaded_brightness(IF_image_t *img, IF_image_t *cuda_img, float factor);

#ifdef __cplusplus
}
#endif
