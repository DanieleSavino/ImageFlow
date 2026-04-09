#pragma once

#include "ImageFlow/io/image.h"
#include "ImageFlow/pipeline.h"
#ifdef __cplusplus
extern "C" {
#endif

#include "ImageFlow/error.h"

#define IF_CHECK_SCHED_PARAMS(flow, img_in, img_out) \
    do { \
    if(flow == NULL || img_in == NULL || img_out == NULL) \
        return IF_NULL_POINTER; \
    } while(0)

typedef enum {
    IF_SCHEDULER_LINEAR // Naive implementation
} IF_Scheduler_t;

NODISCARD IF_error_t IF_flow_run_sched(IF_Flow_t flow, const IF_image_t *img_in, IF_Scheduler_t sched, IF_image_t *img_out);
NODISCARD IF_error_t IF_flow_run(IF_Flow_t flow, const IF_image_t *img_in, IF_image_t *img_out);

#ifdef __cplusplus
}
#endif
