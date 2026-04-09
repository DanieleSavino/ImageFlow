#include "ImageFlow/operations.h"
#include "ImageFlow/accelerated/acc_wrapper.h"
#include "ImageFlow/error.h"
#include "ImageFlow/io/image.h"
#include "ImageFlow/omp/backend.h"
#include <math.h>

/**
 * FIXME: Explain why double wrapper
 */

NODISCARD IF_error_t IF_op_execute(IF_Operation_t *op, IF_image_t *img, IF_image_t *cuda_img) {
    // FIXME: Check out of bounds (would be pedantic)
    return IF_op_dispatcher[op->supp_op](op, img, cuda_img);
}

NODISCARD IF_error_t IF_grayscale(IF_Operation_t *op, IF_image_t *img, IF_image_t *cuda_img) {
    if(op->supp_op != IF_OP_GRAYSCALE) {
        return IF_INVALID_ARGS;
    }

    switch (op->pref_dev) {
        case IF_DEV_GPU:
            return IFACC_loaded_grayScale(img, cuda_img);
        case IF_DEV_CPU:
            return IFOMP_grayScale(img);
        default:
            return IF_INVALID_ARGS;
    }
}

NODISCARD IF_error_t IF_invert(IF_Operation_t *op, IF_image_t *img, IF_image_t *cuda_img) {
    if(op->supp_op != IF_OP_INVERT) {
        return IF_INVALID_ARGS;
    }

    switch (op->pref_dev) {
        case IF_DEV_GPU:
            return IFACC_loaded_invert(img, cuda_img);
        case IF_DEV_CPU:
            return IFOMP_invert(img);
        default:
            return IF_INVALID_ARGS;
    }
}

NODISCARD IF_error_t IF_brightness(IF_Operation_t *op, IF_image_t *img, IF_image_t *cuda_img) {
    if(op->supp_op != IF_OP_BRIGHTNESS) {
        return IF_INVALID_ARGS;
    }

    float factor = op->op_args.float_factor.factor;
    if(!isfinite(factor)) {
        return IF_INVALID_ARGS;
    }

    switch (op->pref_dev) {
        case IF_DEV_GPU:
            return IFACC_loaded_brightness(img, cuda_img, factor);
        case IF_DEV_CPU:
            return IFOMP_brightness(img, factor);
        default:
            return IF_INVALID_ARGS;
    }

    return IF_SUCCESS;
}
