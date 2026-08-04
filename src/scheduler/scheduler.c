#include "ImageFlow/scheduler/scheduler.h"
#include "ImageFlow/vars.h"
#include "ImageFlow/error.h"
#include "ImageFlow/pipeline.h"
#include <stdlib.h>
#include <string.h>

IF_SchedImpl_t IF_sched_impls[_IF_SCHEDULER_LEN];

NODISCARD IF_error_t IF_flow_run_sched(IF_Flow_t flow, const IF_image_t *img_in, IF_Scheduler_t sched, IF_image_t *img_out) {
    IF_CHECK_SCHED_PARAMS(flow, img_in, img_out);

    if(sched >= _IF_SCHEDULER_LEN)
        return IF_INVALID_ARGS;

    return IF_sched_impls[sched](flow, img_in, img_out);
}

#define IF_SCHED_DEF(name) #name,

const char* IF_SchedNames[_IF_SCHEDULER_LEN] = {
    #include "ImageFlow/scheduler/schedulers.def"
};

#undef IF_SCHED_DEF

#define _DEFAULT_SCHED IF_SCHEDULER_REORDER

IF_Scheduler_t IF_getenv_sched() {
    const char *env_sched = getenv(IF_SCHED_PARAM);

    if(env_sched == NULL) return _DEFAULT_SCHED;

    for(IF_Scheduler_t sched = 0; sched < _IF_SCHEDULER_LEN; sched++) {
        if(!strcmp(IF_strsched(sched), env_sched))
            return sched;
    }

    return _DEFAULT_SCHED;
}

NODISCARD IF_error_t IF_flow_run(IF_Flow_t flow, const IF_image_t *img_in, IF_image_t *img_out) {
    return IF_flow_run_sched(flow, img_in, IF_getenv_sched(), img_out);
}
