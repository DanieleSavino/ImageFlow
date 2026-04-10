#include "ImageFlow/scheduler/cpu.h"
#include "ImageFlow/error.h"
#include "ImageFlow/io/image.h"
#include "ImageFlow/operations.h"
#include "ImageFlow/pipeline.h"

// WARN: The initial copy is unnecessary, we could just do the 1st operation non in-place
NODISCARD IF_error_t IF_cpu_execute(IF_Flow_t flow, const IF_image_t *img_in, IF_image_t *img_out) {
    IF_CHECK(IF_copyImage(img_in, img_out));

    IF_Operation_t *op;
    for(int i = 0; i < flow->len; i++) {
        IF_CHECK(IF_flow_get(flow, i, &op));
        op->pref_dev = IF_DEV_CPU;

        IF_CHECK(IF_op_execute(op, img_out, NULL));
    }

    return IF_SUCCESS;
}
