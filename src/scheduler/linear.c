#include "ImageFlow/scheduler/linear.h"
#include "ImageFlow/devices/devices.h"
#include "ImageFlow/devices/img_loader.h"
#include "ImageFlow/error.h"
#include "ImageFlow/io/image.h"
#include "ImageFlow/pipeline.h"
#include "ImageFlow/scheduler/scheduler.h"
#include "ImageFlow/operations/operations.h"

// WARN: The initial copy is unnecessary, we could just do the 1st operation non in-place
NODISCARD IF_error_t IF_linear_execute(IF_Flow_t flow, const IF_image_t *img_in, IF_image_t *img_out) {
    IF_CHECK_SCHED_PARAMS(flow, img_in, img_out);
    IF_CHECK(IF_copyImage(img_in, img_out));

    IF_image_t *imgs[_IF_DEV_LEN] = { 0 };
    imgs[IF_DEV_CPU] = img_out;

    IF_DevType_t prev_dev = IF_DEV_CPU;
    IF_DevType_t curr_dev = IF_DEV_CPU;
    IF_Operation_t *op;

    for (int i = 0; i < flow->len; i++) {
        IF_CHECK(IF_flow_get(flow, i, &op));

        if (!IF_device_enabled(op->pref_dev) || !IF_op_supported(op->supp_op, op->pref_dev)) {
            op->pref_dev = IF_DEV_CPU;
        }
        curr_dev = op->pref_dev;

        if (curr_dev != prev_dev) {
            IF_CHECK(IF_dev2host(prev_dev, imgs));
            IF_CHECK(IF_img_free(prev_dev, imgs));
            IF_CHECK(IF_host2dev(curr_dev, imgs));
        }

        IF_CHECK(IF_op_execute(op, imgs));
        prev_dev = curr_dev;
    }

    IF_CHECK(IF_dev2host(prev_dev, imgs));
    IF_CHECK(IF_img_free(prev_dev, imgs));

    return IF_SUCCESS;
}
