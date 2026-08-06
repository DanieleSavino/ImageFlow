/**
 * @file load_img.h
 * @brief Per-device image transfer dispatch: host <-> device.
 *
 * IF_LoadImgFuncs[dev]    moves a host image onto device dev, populating
 *                         imgs[dev].
 * IF_RetrieveImgFuncs[dev] moves imgs[dev] back into a host image.
 *
 * Both tables are populated at load time via IF_LOAD_IMG_IMPL /
 * IF_RETRIEVE_IMG_IMPL constructor-registered definitions, the same
 * pattern used for IF_OpImpls in operations.h.
 *
 * @author Daniele Savino <daniele.savino0@gmail.com>
 *
 * SPDX-License-Identifier: MIT
 * Copyright (c) 2026 Daniele Savino
 */
#pragma once

#include "ImageFlow/error.h"
#include "ImageFlow/io/image.h"
#include "ImageFlow/devices/devices.h"
#include <stdio.h>
#include <stdlib.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Loads a host image onto a device, writing the result into imgs[dev].
 *
 * @param imgs     Array of length _IF_DEV_LEN; imgs[dev] is populated
 *                 by this call. Other slots are untouched.
 * @return IF_error_t
 */
typedef IF_error_t (*IF_load_img_func_ptr_t)(IF_image_t **imgs);

/**
 * @brief Retrieves a device image back into a host image.
 *
 * @param imgs     Array of length _IF_DEV_LEN; imgs[dev] is read by this call.
 * @return IF_error_t
 */
typedef IF_error_t (*IF_retrieve_img_func_ptr_t)(IF_image_t **imgs);

/**
 * @brief Dispatch tables: [dev] -> transfer function, or NULL if that
 *        device has no registered load/retrieve implementation.
 *
 * Declared extern here and defined exactly once in load_img.c to avoid
 * each translation unit getting its own private zero-initialized copy.
 */
extern IF_load_img_func_ptr_t    IF_LoadImgFuncs[_IF_DEV_LEN];
extern IF_retrieve_img_func_ptr_t IF_RetrieveImgFuncs[_IF_DEV_LEN];

static IF_load_img_func_ptr_t IF_get_load_img(IF_DevType_t dev) {
    if (dev < 0 || dev >= _IF_DEV_LEN)
        return NULL;
    return IF_LoadImgFuncs[dev];
}

static inline IF_error_t IF_host2dev(IF_DevType_t dev, IF_image_t **imgs) {
    IF_load_img_func_ptr_t func = IF_get_load_img(dev);
    if(func == NULL) {
        printf("FUNC: %p\n", func);
        return IF_INVALID_ARGS;
    }

    func(imgs);
    return IF_SUCCESS;
}

static IF_retrieve_img_func_ptr_t IF_get_retrieve_img(IF_DevType_t dev) {
    if (dev < 0 || dev >= _IF_DEV_LEN)
        return NULL;
    return IF_RetrieveImgFuncs[dev];
}

static inline IF_error_t IF_dev2host(IF_DevType_t dev, IF_image_t **imgs) {
    IF_retrieve_img_func_ptr_t func = IF_get_retrieve_img(dev);
    if(func == NULL)
        return IF_INVALID_ARGS;

    func(imgs);
    return IF_SUCCESS;
}

/**
 * @brief Frees the device-side allocation at imgs[dev] and clears the slot
 *        to NULL. A no-op for CPU (the host buffer is owned by the caller
 *        of IF_linear_execute, not by the scheduler).
 */
typedef IF_error_t (*IF_free_img_func_ptr_t)(IF_image_t **imgs);

extern IF_free_img_func_ptr_t IF_FreeImgFuncs[_IF_DEV_LEN];

static inline IF_free_img_func_ptr_t IF_get_free_img(IF_DevType_t dev) {
    if (dev < 0 || dev >= _IF_DEV_LEN)
        return NULL;
    return IF_FreeImgFuncs[dev];
}

/**
 * @brief Frees imgs[dev] via whatever free implementation is registered
 *        for dev, and clears the slot. Safe to call on an already-NULL
 *        slot (no-op).
 */
static inline IF_error_t IF_img_free(IF_DevType_t dev, IF_image_t **imgs) {
    if (imgs[dev] == NULL)
        return IF_SUCCESS;

    IF_free_img_func_ptr_t free_fn = IF_get_free_img(dev);
    if (free_fn == NULL)
        return IF_BACKEND_UNAVAILABLE;

    return free_fn(imgs);
}

/**
 * @brief Declares + registers a load_img implementation for device dev.
 * @note Must be immediately followed by a `{ ... }` body returning IF_error_t.
 */
#define IF_LOAD_IMG_IMPL(dev) \
static IF_error_t IF_load_img_##dev(IF_image_t **imgs); \
\
IF_CONSTRUCTOR(IF_register_load_img_##dev) \
{ \
    IF_LoadImgFuncs[IF_DEV_##dev] = IF_load_img_##dev; \
} \
\
static IF_error_t IF_load_img_##dev(IF_image_t **imgs)

/**
 * @brief Declares + registers a retrieve_img implementation for device dev.
 * @note Must be immediately followed by a `{ ... }` body returning IF_error_t.
 */
#define IF_RETRIEVE_IMG_IMPL(dev) \
static IF_error_t IF_retrieve_img_##dev(IF_image_t **imgs); \
\
IF_CONSTRUCTOR(IF_register_retrieve_img_##dev) \
{ \
    IF_RetrieveImgFuncs[IF_DEV_##dev] = IF_retrieve_img_##dev; \
} \
\
static IF_error_t IF_retrieve_img_##dev(IF_image_t **imgs)

#ifdef __cplusplus
}
#endif

/**
 * @brief Declares + registers a free_img implementation for device dev.
 * @note Must be immediately followed by a `{ ... }` body returning IF_error_t.
 */
#define IF_FREE_IMG_IMPL(dev) \
static IF_error_t IF_free_img_##dev(IF_image_t **imgs); \
\
IF_CONSTRUCTOR(IF_register_free_img_##dev) \
{ \
    IF_FreeImgFuncs[IF_DEV_##dev] = IF_free_img_##dev; \
} \
\
static IF_error_t IF_free_img_##dev(IF_image_t **imgs)
