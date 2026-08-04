#pragma once

#include "ImageFlow/error.h"
#include "ImageFlow/pipeline.h"
#include "ImageFlow/scheduler/scheduler.h"
#include "ImageFlow/scheduler/sched_constructor.h"

#ifdef __cplusplus
extern "C" {
#endif

NODISCARD IF_error_t IF_cpu_execute(IF_Flow_t flow, const IF_image_t *img_in, IF_image_t *img_out);

/**
 * @brief Self-registers IF_cpu_execute as the IF_SCHEDULER_CPU
 *        implementation in IF_sched_impls[].
 */
IF_SCHED_IMPL(CPU) {
    return IF_cpu_execute(flow, img_in, img_out);
}

#ifdef __cplusplus
}
#endif
