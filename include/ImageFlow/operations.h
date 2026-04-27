/**
 * @file operations.h
 * @brief Operation descriptors and dispatch table for ImageFlow.
 *
 * Defines the IF_Operation_t descriptor struct, supporting enumerations,
 * and the static dispatch table IF_op_dispatcher that maps each supported
 * operation to its implementation function.
 *
 * Each operation is described by:
 * - a supported op identifier (IF_SupportedOp_t)
 * - a traversal class (IF_OpType_t) characterizing its memory access pattern
 * - a preferred device (IF_DevType_t)
 * - a tagged-union of operation arguments (IF_OpArgs_t)
 *
 * Execution is routed through IF_op_execute, which indexes into
 * IF_op_dispatcher and delegates to the per-op implementation.
 * Each implementation then dispatches further to the appropriate
 * backend (IFACC_* for GPU, IFOMP_* for CPU).
 *
 * @note MPI support is not planned. Adding distributed execution would
 *       require significant changes to IF_OpArgs_t and the dispatch model.
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
 * @brief Preferred execution device for an operation.
 */
typedef enum {
    IF_DEV_CPU, /**< Execute on CPU via the OpenMP backend. */
    IF_DEV_GPU  /**< Execute on GPU via the accelerator abstraction layer. */
} IF_DevType_t;

/**
 * @brief Characterizes the memory access pattern of an operation.
 *
 * Used to classify operations for scheduling or future optimization purposes.
 */
typedef enum {
    IF_TRAVERSAL_METADATA, /**< Metadata-only; no pixel traversal (e.g. vertical resize). */
    IF_TRAVERSAL_POINT,    /**< Per-pixel 1:1 mapping (e.g. grayscale, brightness). */
    IF_TRAVERSAL_STENCIL,  /**< Neighborhood access (e.g. blur, Sobel). */
    IF_TRAVERSAL_REDUCTION,/**< Global reduction to a scalar (e.g. min, max, average). */
    IF_TRAVERSAL_MORPH     /**< Geometric transformation (e.g. rotate, horizontal resize). */
} IF_OpType_t;

/**
 * @brief Tagged union of operation-specific arguments.
 *
 * Extend this union when adding new operations that require parameters.
 *
 * @warning Adding pointer members here would complicate any future
 *          distributed (MPI) execution model significantly.
 */
typedef union {
    struct { } empty;            /**< Placeholder for zero-argument operations. */
    struct { float factor; } float_factor; /**< Single float parameter (e.g. brightness factor). */
} IF_OpArgs_t;

/**
 * @brief Identifies a supported image operation.
 *
 * _IF_OP_LEN is a sentinel used to size the dispatch table; do not use it
 * as a valid operation identifier.
 */
typedef enum {
    IF_OP_GRAYSCALE, /**< Convert image to grayscale. */
    IF_OP_INVERT,    /**< Invert all RGB channel values. */
    IF_OP_BRIGHTNESS,/**< Scale RGB channel values by a factor. */
    _IF_OP_LEN       /**< Sentinel: number of supported operations. */
} IF_SupportedOp_t;

/**
 * @brief Fully describes a single image operation to be executed.
 */
typedef struct {
    IF_SupportedOp_t supp_op; /**< Which operation to perform. */
    IF_OpType_t      op_type; /**< Memory access pattern classification. */
    IF_DevType_t     pref_dev;/**< Preferred execution device. */
    IF_OpArgs_t      op_args; /**< Operation-specific arguments. */
} IF_Operation_t;

static const char* IF_strop(IF_SupportedOp_t op) {
    switch (op) {
        case IF_OP_GRAYSCALE:
            return "Grayscale";
        case IF_OP_INVERT:
            return "Invert";
        case IF_OP_BRIGHTNESS:
            return "Brightness";
        default:
            return "UNKNOWN";
    }
}

/**
 * @brief Function pointer type for operation implementations.
 *
 * @param op       Operation descriptor.
 * @param img      Host image (always valid; used directly for CPU ops,
 *                 used for grid sizing on GPU ops).
 * @param cuda_img Device image for GPU execution, or NULL for CPU ops.
 * @return IF_error_t
 */
typedef IF_error_t (*IF_op_func_ptr_t)(IF_Operation_t*, IF_image_t*, IF_image_t*);

/**
 * @brief Executes an operation by indexing into the static dispatch table.
 *
 * Routes @p op to the corresponding implementation in IF_op_dispatcher
 * based on op->supp_op. The double-wrapper pattern (IF_op_execute ->
 * IF_grayscale/IF_invert/IF_brightness -> IFACC_* / IFOMP_*) keeps the
 * dispatch table decoupled from backend selection logic.
 *
 * @param op       Operation descriptor. Must not be NULL.
 * @param img      Host image.
 * @param cuda_img Device image, or NULL if executing on CPU.
 * @return IF_SUCCESS on success.
 * @return IF_INVALID_ARGS if op->supp_op is out of range.
 * @return Any error code returned by the delegated operation.
 */
NODISCARD IF_error_t IF_op_execute(IF_Operation_t *op, IF_image_t *img, IF_image_t *cuda_img);

/**
 * @brief Grayscale operation implementation.
 *
 * Validates that op->supp_op == IF_OP_GRAYSCALE, then dispatches to
 * IFACC_loaded_grayScale (GPU) or IFOMP_grayScale (CPU).
 *
 * @param op       Must have supp_op == IF_OP_GRAYSCALE.
 * @param img      Host image.
 * @param cuda_img Device image for GPU path; ignored on CPU path.
 * @return IF_SUCCESS on success.
 * @return IF_INVALID_ARGS if supp_op or pref_dev is invalid.
 * @return Backend error codes on failure.
 */
NODISCARD IF_error_t IF_grayscale(IF_Operation_t *op, IF_image_t *img, IF_image_t *cuda_img);

/**
 * @brief Invert operation implementation.
 *
 * Validates that op->supp_op == IF_OP_INVERT, then dispatches to
 * IFACC_loaded_invert (GPU) or IFOMP_invert (CPU).
 *
 * @param op       Must have supp_op == IF_OP_INVERT.
 * @param img      Host image.
 * @param cuda_img Device image for GPU path; ignored on CPU path.
 * @return IF_SUCCESS on success.
 * @return IF_INVALID_ARGS if supp_op or pref_dev is invalid.
 * @return Backend error codes on failure.
 */
NODISCARD IF_error_t IF_invert(IF_Operation_t *op, IF_image_t *img, IF_image_t *cuda_img);

/**
 * @brief Brightness operation implementation.
 *
 * Validates that op->supp_op == IF_OP_BRIGHTNESS and that
 * op->op_args.float_factor.factor is finite, then dispatches to
 * IFACC_loaded_brightness (GPU) or IFOMP_brightness (CPU).
 *
 * @param op       Must have supp_op == IF_OP_BRIGHTNESS. factor must be finite.
 * @param img      Host image.
 * @param cuda_img Device image for GPU path; ignored on CPU path.
 * @return IF_SUCCESS on success.
 * @return IF_INVALID_ARGS if supp_op is wrong, pref_dev is invalid,
 *         or factor is NaN or infinite.
 * @return Backend error codes on failure.
 */
NODISCARD IF_error_t IF_brightness(IF_Operation_t *op, IF_image_t *img, IF_image_t *cuda_img);

/**
 * @brief Static dispatch table mapping IF_SupportedOp_t values to implementations.
 *
 * Indexed by IF_SupportedOp_t. Designated initializers ensure the mapping
 * is explicit and order-independent.
 */
static const IF_op_func_ptr_t IF_op_dispatcher[_IF_OP_LEN] = {
    [IF_OP_GRAYSCALE]  = IF_grayscale,
    [IF_OP_INVERT]     = IF_invert,
    [IF_OP_BRIGHTNESS] = IF_brightness
};

#ifdef __cplusplus
}
#endif
