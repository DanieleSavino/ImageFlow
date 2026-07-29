/**
 * @file operations.h
 * @brief Operation descriptors and dispatch table for ImageFlow.
 *
 * Defines the IF_Operation_t descriptor struct, supporting enumerations,
 * and the static dispatch table IF_OpImpls that maps each supported
 * operation to its implementation function.
 *
 * Each operation is described by:
 * - a supported op identifier (IF_SupportedOp_t)
 * - a traversal class (IF_OpType_t) characterizing its memory access pattern
 * - a preferred device (IF_DevType_t)
 * - a tagged-union of operation arguments (IF_OpArgs_t)
 *
 * Execution is routed through IF_op_execute, which indexes into
 * IF_OpImpls and delegates to the per-op implementation.
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
#include "ImageFlow/devices/devices.h"
#include "ImageFlow/operations/op_args.h"

#ifdef __cplusplus
extern "C" {
#endif


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
 * @brief Identifies a supported image operation.
 *
 * _IF_OP_LEN is a sentinel used to size the dispatch table; do not use it
 * as a valid operation identifier.
 */
#define IF_OP_DEF(op, name, type, args) IF_OP_##op,

typedef enum {
    #include "operations.def"
    _IF_OP_LEN
} IF_SupportedOp_t;

#undef IF_OP_DEF


/* String names, built from the same list */
#define IF_OP_DEF(op, name, type, args) #name,

static const char *IF_OpNames[_IF_OP_LEN] = {
    #include "operations.def"
};

#undef IF_OP_DEF

static inline const char* IF_strop(IF_SupportedOp_t op) {
    return IF_OpNames[op];
}


#define IF_OP_DEF(op, name, type, args) IF_TRAVERSAL_##type,

static const IF_OpType_t IF_OpTypes[_IF_OP_LEN] = {
    #include "operations.def"
};

#undef IF_OP_DEF

static inline IF_OpType_t IF_op_type(IF_SupportedOp_t op) {
    return IF_OpTypes[op];
}

/**
 * @brief Fully describes a single image operation to be executed.
 */
typedef struct {
    IF_SupportedOp_t supp_op; /**< Which operation to perform. */
    IF_DevType_t     pref_dev;/**< Preferred execution device. */
    IF_OpArgs_t      op_args; /**< Operation-specific arguments. */
} IF_Operation_t;


/**
 * @brief Function pointer type for operation implementations.
 *
 * @param args Operation-specific arguments.
 * @param imgs Array of length _IF_DEV_LEN, one slot per device; imgs[dev]
 *             holds the image for that device, or NULL if unused. The
 *             implementation is responsible for knowing which slot(s)
 *             it expects to be populated.
 * @return IF_error_t
 */
typedef IF_error_t (*IF_op_func_ptr_t)(IF_OpArgs_t args, IF_image_t **imgs);

typedef struct {
    IF_op_func_ptr_t func;
    int flops_per_pixel;
    int bytes_per_pixel;
} IF_OpImpl_t;

/**
 * @brief Static dispatch table: [op][device] -> implementation, or NULL if
 *        that (op, device) pair is not implemented.
 *
 * Declared extern here and defined exactly once in operations.c to avoid
 * each translation unit getting its own private (and independently
 * zero-initialized) copy.
 */
extern IF_OpImpl_t *IF_OpImpls[_IF_OP_LEN][_IF_DEV_LEN];

static inline int IF_op_supported(IF_SupportedOp_t supp_op, IF_DevType_t dev) {
    if (supp_op < 0 || supp_op >= _IF_OP_LEN || dev < 0 || dev >= _IF_DEV_LEN)
        return 0;

    IF_OpImpl_t *op_impl = IF_OpImpls[supp_op][dev];

    if (op_impl != NULL && op_impl->func != NULL)
        return 1;
    return 0;
}

static inline IF_OpImpl_t *IF_op_get_impl(IF_SupportedOp_t op, IF_DevType_t dev) {
    if (op < 0 || op >= _IF_OP_LEN || dev < 0 || dev >= _IF_DEV_LEN)
        return NULL;

    IF_OpImpl_t *op_impl = IF_OpImpls[op][dev];

    if (op_impl != NULL && op_impl->func != NULL)
        return op_impl;

    return NULL;
}

/**
 * @brief Prints the current state of the IF_OpImpls dispatch table.
 *
 * For every (op, dev) pair, prints whether an implementation is
 * registered, and if so, its flops/bytes-per-pixel metadata. Intended
 * as a debug/diagnostic helper (e.g. to sanity-check that constructor
 * registration actually ran for the ops/devices you expect).
 */
static inline void IF_print_op_impls(void) {
    printf("%-14s", "op \\ dev");
    for (int d = 0; d < _IF_DEV_LEN; d++) {
        printf("%-10s", IF_strdev((IF_DevType_t)d));
    }
    printf("\n");

    for (int op = 0; op < _IF_OP_LEN; op++) {
        printf("%-14s", IF_strop((IF_SupportedOp_t)op));

        for (int dev = 0; dev < _IF_DEV_LEN; dev++) {
            IF_OpImpl_t *impl = IF_OpImpls[op][dev];

            if (impl == NULL || impl->func == NULL) {
                printf("%-10s", "-");
            } else {
                char buf[16];
                snprintf(buf, sizeof(buf), "%dF/%dB", impl->flops_per_pixel, impl->bytes_per_pixel);
                printf("%-10s", buf);
            }
        }
        printf("\n");
    }
}


/**
 * @brief Executes an operation by indexing into the static dispatch table.
 *
 * Looks up the implementation registered for (op->supp_op, op->pref_dev)
 * via IF_op_get_impl and delegates to it.
 *
 * @param op   Operation descriptor. Must not be NULL.
 * @param imgs Array of length _IF_DEV_LEN, one slot per device; imgs[dev]
 *             holds the image for that device, or NULL if unused.
 * @return IF_SUCCESS on success.
 * @return IF_INVALID_ARGS if op->supp_op/op->pref_dev is out of range, or
 *         if no implementation is registered for that (op, device) pair.
 * @return Any error code returned by the delegated operation.
 */
NODISCARD IF_error_t IF_op_execute(IF_Operation_t *op, IF_image_t **imgs);

#ifdef __cplusplus
}
#endif
