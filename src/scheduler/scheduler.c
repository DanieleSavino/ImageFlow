#include "ImageFlow/scheduler/scheduler.h"
#include "ImageFlow/error.h"
#include "ImageFlow/pipeline.h"
#include "ImageFlow/scheduler/cpu.h"
#include "ImageFlow/scheduler/linear.h"

NODISCARD IF_error_t IF_flow_run_sched(IF_Flow_t flow, const IF_image_t *img_in, IF_Scheduler_t sched, IF_image_t *img_out) {
    IF_CHECK_SCHED_PARAMS(flow, img_in, img_out);

    switch (sched) {
        case IF_SCHEDULER_CPU:
            return IF_cpu_execute(flow, img_in, img_out);
        case IF_SCHEDULER_LINEAR:
            return IF_linear_execute(flow, img_in, img_out);
        default:
            return IF_INVALID_ARGS;
    }
}

NODISCARD IF_error_t IF_flow_run(IF_Flow_t flow, const IF_image_t *img_in, IF_image_t *img_out) {
    return IF_flow_run_sched(flow, img_in, IF_SCHEDULER_LINEAR, img_out);
}
