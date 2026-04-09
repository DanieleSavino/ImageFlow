/**
 * @file acc_wrapper.h
 * @brief Hardware accelerator abstraction layer for ImageFlow.
 *
 * Provides a unified API that dispatches image operations to whichever GPU
 * backend is available at runtime (CUDA or HIP). Backend selection is performed
 * once by IFACC_check_system() via a static dispatch table; subsequent calls
 * go through function pointers with no further probing.
 *
 * Backends are compiled in conditionally:
 * @verbatim
 *   -D_CUDA_ACC   enables the CUDA backend (IFCU_*)
 *   -D_HIP_ACC    enables the HIP/ROCm backend (IFHIP_*)
 * @endverbatim
 *
 * CUDA is probed before HIP; the first backend that reports at least one device
 * wins. If neither reports a device, IFACC_available() returns 0 and all
 * operation calls return IF_DEVICE_ERROR.
 *
 * @note Multi-GPU support is intentionally out of scope.
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
 * @brief Identifies the active hardware accelerator backend.
 *
 * Set by IFACC_check_system(). UNDEF means the system has not been probed yet.
 */
typedef enum {
    UNDEF    = 0, /**< System not yet probed. */
    NO_ACC,       /**< No supported GPU device found. */
    CUDA_ACC,     /**< CUDA backend selected. */
    HIP_ACC       /**< HIP/ROCm backend selected. */
} IF_acc_t;

/**
 * @brief Probes the system for a supported GPU and initializes the dispatch table.
 *
 * Queries available devices for each compiled-in backend in priority order
 * (CUDA before HIP). Selects the first backend that reports at least one device
 * and populates the internal function-pointer table accordingly.
 *
 * This function is called lazily on the first operation if not called explicitly.
 * Calling it more than once is safe but redundant.
 *
 * @return IF_SUCCESS whether or not a GPU was found (NO_ACC is a valid outcome).
 * @return IF_CUDA_ERROR or IF_HIP_ERROR if device enumeration itself fails.
 */
NODISCARD IF_error_t IFACC_check_system();

/**
 * @brief Returns whether a supported GPU backend is available.
 *
 * Triggers IFACC_check_system() lazily if not already called.
 *
 * @return 1 if a GPU backend is active, 0 otherwise.
 */
int IFACC_available();

/**
 * @brief Uploads a host IF_image_t to device memory via the active backend.
 *
 * @param img      Source host image. Must not be NULL and must have valid data
 *                 and at least 3 channels.
 * @param cuda_img Output: pointer to the newly allocated device IF_image_t.
 *                 Must be released via IFACC_retrieve.
 * @return IF_SUCCESS on success.
 * @return IF_DEVICE_ERROR if no GPU backend is available.
 * @return Backend-specific error codes on failure.
 */
NODISCARD IF_error_t IFACC_load(const IF_image_t *img, IF_image_t **cuda_img);

/**
 * @brief Downloads a device IF_image_t back to host memory via the active backend.
 *
 * Frees the device image and sets *cuda_img to NULL on success.
 *
 * @param cuda_img Pointer to the device IF_image_t pointer from IFACC_load.
 * @param img_out  Output host image struct. Caller must free via IF_freeImage.
 * @return IF_SUCCESS on success.
 * @return IF_DEVICE_ERROR if no GPU backend is available.
 * @return Backend-specific error codes on failure.
 */
NODISCARD IF_error_t IFACC_retrieve(IF_image_t **cuda_img, IF_image_t *img_out);

/**
 * @brief Converts an image to grayscale via the active backend (single-shot).
 *
 * @param img Image to convert in-place.
 * @return IF_SUCCESS on success.
 * @return IF_DEVICE_ERROR if no GPU backend is available.
 * @return Backend-specific error codes on failure.
 */
NODISCARD IF_error_t IFACC_grayScale(IF_image_t *img);

/**
 * @brief Inverts all RGB channel values via the active backend (single-shot).
 *
 * @param img Image to invert in-place.
 * @return IF_SUCCESS on success.
 * @return IF_DEVICE_ERROR if no GPU backend is available.
 * @return Backend-specific error codes on failure.
 */
NODISCARD IF_error_t IFACC_invert(IF_image_t *img);

/**
 * @brief Scales RGB channel values by @p factor, clamping to 1.0 (single-shot).
 *
 * @param img    Image to adjust in-place.
 * @param factor Multiplicative brightness factor. Output is clamped to [0.0, 1.0].
 * @return IF_SUCCESS on success.
 * @return IF_DEVICE_ERROR if no GPU backend is available.
 * @return Backend-specific error codes on failure.
 */
NODISCARD IF_error_t IFACC_brightness(IF_image_t *img, float factor);

/**
 * @brief Runs the grayscale kernel on an already-loaded device image.
 *
 * @param img      Host image, used for grid sizing.
 * @param cuda_img Device IF_image_t produced by IFACC_load. Must not be NULL.
 * @return IF_SUCCESS on success.
 * @return IF_DEVICE_ERROR if no GPU backend is available.
 * @return Backend-specific error codes on failure.
 */
NODISCARD IF_error_t IFACC_loaded_grayScale(IF_image_t *img, IF_image_t *cuda_img);

/**
 * @brief Runs the invert kernel on an already-loaded device image.
 *
 * @param img      Host image, used for grid sizing.
 * @param cuda_img Device IF_image_t produced by IFACC_load. Must not be NULL.
 * @return IF_SUCCESS on success.
 * @return IF_DEVICE_ERROR if no GPU backend is available.
 * @return Backend-specific error codes on failure.
 */
NODISCARD IF_error_t IFACC_loaded_invert(IF_image_t *img, IF_image_t *cuda_img);

/**
 * @brief Runs the brightness kernel on an already-loaded device image.
 *
 * @param img      Host image, used for grid sizing.
 * @param cuda_img Device IF_image_t produced by IFACC_load. Must not be NULL.
 * @param factor   Multiplicative brightness factor. Output is clamped to [0.0, 1.0].
 * @return IF_SUCCESS on success.
 * @return IF_DEVICE_ERROR if no GPU backend is available.
 * @return Backend-specific error codes on failure.
 */
NODISCARD IF_error_t IFACC_loaded_brightness(IF_image_t *img, IF_image_t *cuda_img, float factor);

#ifdef __cplusplus
}
#endif
