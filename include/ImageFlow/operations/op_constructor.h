#pragma once

#include "ImageFlow/constructor.h"
#include "ImageFlow/operations/operations.h"

#ifdef __cplusplus
extern "C" {
#endif


/**
 * @brief Declares an operation implementation function and registers it
 *        into IF_OpImpls[op][dev] via a load-time constructor.
 *
 * Unlike a single-macro body-as-argument approach, this only declares the
 * function signature and emits the registration constructor; the caller
 * supplies the function body directly afterward as an ordinary braced
 * function definition. This lets the body contain #pragma directives
 * (e.g. #pragma omp), multi-statement macros, or anything else that
 * doesn't survive being passed as a macro argument.
 *
 * @param dev     Bare device suffix (e.g. CPU) — pasted into IF_DEV_##dev.
 * @param supp_op Bare op suffix (e.g. GRAYSCALE) — pasted into IF_OP_##supp_op.
 * @param fpp     Flops per pixel.
 * @param bpp     Bytes per pixel.
 *
 * @note Must be immediately followed by a `{ ... }` function body that
 *       returns IF_error_t.
 */
#define IF_OP_IMPL(dev, supp_op, fpp, bpp) \
static IF_error_t IF_##dev##_##supp_op(IF_OpArgs_t args, IF_image_t **imgs); \
\
IF_CONSTRUCTOR(IF_register_##dev##_##supp_op) \
{ \
    static IF_OpImpl_t impl = { \
        .func = IF_##dev##_##supp_op, \
        .flops_per_pixel = (fpp), \
        .bytes_per_pixel = (bpp) \
    }; \
    IF_OpImpls[IF_OP_##supp_op][IF_DEV_##dev] = &impl; \
} \
\
static IF_error_t IF_##dev##_##supp_op(IF_OpArgs_t args, IF_image_t **imgs)

#ifdef __cplusplus
}
#endif
