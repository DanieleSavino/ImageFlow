/**
 * @file pipeline.h
 * @brief Operation pipeline (flow) builder for ImageFlow.
 *
 * Provides IF_Pipeline_t, a dynamic array of IF_Operation_t descriptors,
 * and IF_Flow_t, an opaque handle to it. Pipelines are built by appending
 * operations via the IF_flow_* builder functions, then executed externally
 * against an image using IF_op_execute on each entry.
 *
 * The buffer doubles in capacity when full (see IF_flow_push).
 * Default initial capacity is 10 operations (see IF_flow_init).
 *
 * All builder functions (IF_flow_grayscale, IF_flow_invert, IF_flow_brightness)
 * default to IF_DEV_GPU. Device preference can be set explicitly via IF_flow_push.
 *
 * @author Daniele Savino <daniele.savino0@gmail.com>
 *
 * SPDX-License-Identifier: MIT
 * Copyright (c) 2026 Daniele Savino
 */
#pragma once

#include "ImageFlow/error.h"
#include "ImageFlow/operations/operations.h"
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Dynamic array of IF_Operation_t descriptors representing a processing pipeline.
 *
 * Not intended to be manipulated directly; use IF_Flow_t and the IF_flow_* API.
 */
typedef struct {
    IF_Operation_t *buff; /**< Heap-allocated array of operation descriptors. */
    size_t len;           /**< Number of operations currently in the pipeline. */
    size_t _size;         /**< Current allocated capacity of buff. */
} IF_Pipeline_t;

/**
 * @brief Opaque handle to a heap-allocated IF_Pipeline_t.
 *
 * All pipeline functions operate through this handle.
 * Must be released via IF_flow_free when no longer needed.
 */
typedef IF_Pipeline_t* IF_Flow_t;

/**
 * @brief Allocates and initializes an empty pipeline with a given initial capacity.
 *
 * @param flow         Output: pointer to the newly allocated IF_Flow_t handle.
 *                     Must not be NULL.
 * @param initial_size Initial buffer capacity in number of operations.
 * @return IF_SUCCESS on success.
 * @return IF_NULL_POINTER if @p flow is NULL.
 * @return IF_OUT_OF_MEMORY if allocation fails.
 */
NODISCARD IF_error_t IF_flow_init_size(IF_Flow_t *flow, int initial_size);

/**
 * @brief Allocates and initializes an empty pipeline with the default capacity (10).
 *
 * @param flow Output: pointer to the newly allocated IF_Flow_t handle.
 * @return IF_SUCCESS on success.
 * @return IF_OUT_OF_MEMORY if allocation fails.
 */
IF_error_t IF_flow_init(IF_Flow_t *flow);

/**
 * @brief Appends an operation descriptor to the pipeline.
 *
 * Doubles the buffer capacity if the pipeline is full.
 * op_args is copied by value; the union is expected to remain small.
 *
 * @param flow     Target pipeline. Must not be NULL and must be initialized.
 * @param supp_op  Operation identifier.
 * @param op_type  Memory access pattern classification.
 * @param pref_dev Preferred execution device.
 * @param op_args  Operation-specific arguments (copied by value).
 * @return IF_SUCCESS on success.
 * @return IF_NULL_POINTER if @p flow or its buffer is NULL.
 * @return IF_INVALID_ARGS if the pipeline capacity is invalid.
 * @return IF_OUT_OF_MEMORY if buffer reallocation fails.
 */
NODISCARD IF_error_t IF_flow_push(IF_Flow_t flow, IF_SupportedOp_t supp_op, IF_DevType_t pref_dev, IF_OpArgs_t op_args);

/**
 * @brief Returns a pointer to the operation descriptor at index @p i.
 *
 * @param flow      Pipeline to index into.
 * @param i         Zero-based index.
 * @param operation Output: pointer to the descriptor at @p i.
 * @return IF_SUCCESS on success.
 * @return IF_NULL_POINTER if @p flow or its buffer is NULL.
 * @return IF_INVALID_ARGS if the pipeline is empty.
 *
 * @warning No bounds check is performed on @p i beyond verifying the pipeline
 *          is non-empty. Caller is responsible for ensuring @p i < flow->len.
 */
NODISCARD IF_error_t IF_flow_get(IF_Flow_t flow, size_t i, IF_Operation_t **operation);

/**
 * @brief Frees a pipeline and its internal buffer.
 *
 * @param flow Pipeline to free.
 * @return IF_SUCCESS on success.
 * @return IF_NULL_POINTER if @p flow or its buffer is NULL.
 */
IF_error_t IF_flow_free(IF_Flow_t flow);

#define CHECK_FLOW(flow) \
    do { \
        if(flow == NULL || flow->buff == NULL)  \
            return IF_NULL_POINTER; \
        if(flow->_size <= 0) \
            return  IF_INVALID_ARGS; \
    } while(0)

#define IF_OP_DEF(op, name, type, args) \
    IF_error_t IF_flow_##name(IF_Flow_t flow IF_OP_ARGS_##args);

#include "ImageFlow/operations/operations.def"

#undef IF_OP_DEF

#ifdef __cplusplus
}
#endif
