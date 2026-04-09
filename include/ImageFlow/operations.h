#pragma once

/**
 * FIXME: For now we just do "if there's a gpu then use it else fallback"
 * the goal of the project is to have a builder pattern like api that actively
 * schedules the requests, using the loaded api when possible, that's why there are 2 wrappers,
 * so for example the cpu might handle simple jobs like resizing, trimming and easy jobs on small images.
 *
 * WARN: The direct IF_operationX() api will probably be deprecated after the transition to the
 * builder pattern api is complete.
 */

#ifdef __cplusplus
extern "C" {
#endif

#include "ImageFlow/error.h"
#include "ImageFlow/io/image.h"

typedef enum {
    IF_DEV_CPU,
    IF_DEV_GPU
} IF_DevType_t;

typedef enum {
    IF_TRAVERSAL_METADATA,   // metadata only operation, like vertical resize
    IF_TRAVERSAL_POINT,      // 1:1 mapping (Grayscale, Brightness)
    IF_TRAVERSAL_STENCIL,    // Neighborhood (Blur, Sobel)
    IF_TRAVERSAL_REDUCTION,  // Global to Scalar (Min/Max/Avg)
    IF_TRAVERSAL_MORPH       // Geometric (Rotate, horizontal resize)
} IF_OpType_t;

typedef union {
    struct {  } empty;                   // For no args operations
    struct { float factor; } brightness; // For brightness

    /** INFO: Add others for new operations */
    /** WARN: MPI support is not planned, but this would make it a nightmare */

} IF_OpArgs_t;

typedef enum {
    IF_OP_GRAYSCALE,
    IF_OP_INVERT,
    IF_OP_BRIGHTNESS
} IF_SupportedOp_t;

typedef struct {
    IF_SupportedOp_t supp_op;
    IF_OpType_t op_type;
    IF_DevType_t pref_dev;
    IF_OpArgs_t op_args;
} IF_Operation_t;

NODISCARD IF_error_t IF_grayScale(IF_image_t *img);
NODISCARD IF_error_t IF_invert(IF_image_t *img);
NODISCARD IF_error_t IF_brightness(IF_image_t *img, float factor);

#ifdef __cplusplus
}
#endif
