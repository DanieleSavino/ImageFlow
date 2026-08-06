#pragma once

#include "ImageFlow/devices/devices.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Tagged union of operation-specific arguments.
 *
 * Extend this union when adding new operations that require parameters.
 * TODO: Clean this
 *
 * @warning Adding pointer members here would complicate any future
 *          distributed (MPI) execution model significantly.
 *          Plus MPI datatypes would be a pain.
 */
typedef union {
    struct { char _unused; } empty;        /**< Placeholder for zero-argument operations. */
    struct { float factor; } float_factor; /**< Single float parameter (e.g. brightness factor). */
} IF_OpArgs_t;

#define IF_OP_ARGS_EMPTY
#define IF_OP_ARGS_FLOAT_FACTOR , float factor

#define IF_OP_INIT_EMPTY \
    (IF_OpArgs_t){ .empty = {} }

#define IF_OP_INIT_FLOAT_FACTOR \
    (IF_OpArgs_t){ .float_factor = { .factor = factor } }

#define IF_OP_DEF(op, name, type, argtype) \
static inline void default_##op##_args(IF_OpArgs_t *args) { \
    float factor = 1.5f; \
    (void)factor; \
    *args = IF_OP_INIT_##argtype; \
}
#include "ImageFlow/operations/operations.def"
#undef IF_OP_DEF

#ifdef __cplusplus
}
#endif
