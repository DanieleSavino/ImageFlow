#include "ImageFlow/scheduler/linear.h"
#include "ImageFlow/error.h"
#include "ImageFlow/io/image.h"
#include "ImageFlow/operations.h"
#include "ImageFlow/pipeline.h"
#include "ImageFlow/accelerated/acc_wrapper.h"
#include "ImageFlow/scheduler/scheduler.h"

static NODISCARD IF_error_t execution_plan(IF_Flow_t flow) {
    if(flow == NULL || flow->buff == NULL) {
        return IF_NULL_POINTER;
    }

    if(IFACC_available()) {
        return IF_SUCCESS;
    }

    IF_Operation_t *op;
    for(int i = 0; i < flow->len; i++) {
        IF_CHECK(IF_flow_get(flow, i, &op));
        op->pref_dev = IF_DEV_CPU;
    }

    return IF_SUCCESS;

}

// WARN: The initial copy is unnecessary, we could just do the 1st operation non in-place
NODISCARD IF_error_t IF_linear_execute(IF_Flow_t flow, const IF_image_t *img_in, IF_image_t *img_out) {
    IF_CHECK_SCHED_PARAMS(flow, img_in, img_out);

    IF_CHECK(IF_copyImage(img_in, img_out));
    IF_CHECK(execution_plan(flow));

    IF_image_t *cuda_img;

    IF_DevType_t prev_dev = IF_DEV_CPU;
    IF_DevType_t curr_dev = IF_DEV_CPU;
    IF_Operation_t *op;

    for(int i = 0; i < flow->len; i++) {
        IF_CHECK(IF_flow_get(flow, i, &op));

        curr_dev = op->pref_dev;
        if(prev_dev == IF_DEV_CPU && curr_dev == IF_DEV_GPU) {
            IF_CHECK(IFACC_load(img_out, &cuda_img));
        }
        if(prev_dev == IF_DEV_GPU && curr_dev == IF_DEV_CPU) {
            IF_CHECK(IF_freeImage(img_out));
            IF_CHECK(IFACC_retrieve(&cuda_img, img_out));
        }

        IF_CHECK(IF_op_execute(op, img_out, cuda_img));

        prev_dev = curr_dev;
    }

    if(curr_dev == IF_DEV_GPU) {
        IF_CHECK(IF_freeImage(img_out));
        IF_CHECK(IFACC_retrieve(&cuda_img, img_out));
    }

    return IF_SUCCESS;
}
