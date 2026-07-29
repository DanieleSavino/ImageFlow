#include "ImageFlow/pipeline.h"
#include "ImageFlow/devices/devices.h"
#include "ImageFlow/error.h"
#include "ImageFlow/operations/operations.h"
#include <stddef.h>
#include <stdio.h>

#define CHECK_FLOW(flow) \
    do { \
        if(flow == NULL || flow->buff == NULL)  \
            return IF_NULL_POINTER; \
        if(flow->_size <= 0) \
            return  IF_INVALID_ARGS; \
    } while(0)


NODISCARD IF_error_t IF_flow_init_size(IF_Flow_t *flow, int initial_size) {
    if(flow == NULL)
        return IF_NULL_POINTER;

    IF_Pipeline_t *tmp_pipe = NULL;
    IF_MALLOC(tmp_pipe, (size_t)sizeof(IF_Pipeline_t));

    IF_MALLOC_LABEL(tmp_pipe->buff, initial_size * sizeof(IF_Operation_t), oom);

    tmp_pipe->_size = initial_size;
    tmp_pipe->len = 0;

    *flow = tmp_pipe;

    return IF_SUCCESS;

    oom:
        free(tmp_pipe);
        return IF_OUT_OF_MEMORY;
}

#define INIT_SIZE 10

IF_error_t IF_flow_init(IF_Flow_t *flow) {
    IF_CHECK(IF_flow_init_size(flow, INIT_SIZE));

    return IF_SUCCESS;
}

NODISCARD IF_error_t IF_flow_push(IF_Flow_t flow, IF_SupportedOp_t supp_op, IF_DevType_t pref_dev, IF_OpArgs_t op_args) {
    CHECK_FLOW(flow);

    if(flow->len == flow->_size) {
        IF_REALLOC(flow->buff, flow->_size * 2 * sizeof(IF_Operation_t));
        flow->_size *= 2;
    }

    IF_Operation_t *op = flow->buff + flow->len;

    op->supp_op = supp_op;
    op->op_args = op_args;
    op->pref_dev = pref_dev;

    // WARN: This makes a copy, the union should be pretty small.
    op->op_args = op_args;

    flow->len++;

    return IF_SUCCESS;
}

NODISCARD IF_error_t IF_flow_get(IF_Flow_t flow, size_t i, IF_Operation_t **operation) {
    CHECK_FLOW(flow);

    if(flow->len == 0) {
        return IF_INVALID_ARGS;
    }

    *operation = flow->buff + i;

    return IF_SUCCESS;
}

IF_error_t IF_flow_free(IF_Flow_t flow) {
    CHECK_FLOW(flow);

    free(flow->buff);
    free(flow);

    return IF_SUCCESS;
}

IF_error_t IF_flow_grayscale(IF_Flow_t flow) {
    CHECK_FLOW(flow);

    IF_OpArgs_t empty = { .empty = {} };
    IF_CHECK(IF_flow_push(flow, IF_OP_GRAYSCALE, IF_enabled_gpu(), empty));

    return IF_SUCCESS;
}

IF_error_t IF_flow_invert(IF_Flow_t flow) {
    CHECK_FLOW(flow);

    IF_OpArgs_t empty = { .empty = {} };
    IF_CHECK(IF_flow_push(flow, IF_OP_INVERT, IF_enabled_gpu(), empty));

    return IF_SUCCESS;
}

IF_error_t IF_flow_brightness(IF_Flow_t flow, float factor) {
    CHECK_FLOW(flow);

    IF_OpArgs_t args = { .float_factor = { .factor = factor } };
    IF_CHECK(IF_flow_push(flow, IF_OP_BRIGHTNESS, IF_enabled_gpu(), args));

    return IF_SUCCESS;
}
