#pragma once

#include "ImageFlow/error.h"
#include "ImageFlow/operations.h"
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    IF_Operation_t *buff;
    size_t len;
    size_t _size;
} IF_Pipeline_t;

typedef IF_Pipeline_t* IF_Flow_t;

NODISCARD IF_error_t IF_flow_init_size(IF_Flow_t *flow, int initial_size);
IF_error_t IF_flow_init(IF_Flow_t *flow);

NODISCARD IF_error_t IF_flow_push(IF_Flow_t flow, IF_SupportedOp_t supp_op, IF_OpType_t op_type, IF_DevType_t pref_dev, IF_OpArgs_t op_args);
NODISCARD IF_error_t IF_flow_get(IF_Flow_t flow, size_t i, IF_Operation_t **operation);

IF_error_t IF_flow_free(IF_Flow_t flow);

IF_error_t IF_flow_grayscale(IF_Flow_t flow);
IF_error_t IF_flow_invert(IF_Flow_t flow);
IF_error_t IF_flow_brightness(IF_Flow_t flow, float factor);

#ifdef __cplusplus
}
#endif
