#include "ImageFlow/pipeline.h"
#include "ImageFlow/error.h"
#include "ImageFlow/operations.h"
#include <stddef.h>

#define CHECK_PIPE_REF(pipe) \
    do { \
        if(pipe == NULL || pipe->buff == NULL)  \
            return IF_NULL_POINTER; \
        if(pipe->_size <= 0) \
            return  IF_INVALID_ARGS; \
    } while(0)


NODISCARD IF_error_t IF_pipe_init_size(IF_Pipeline_t **pipe, int initial_size) {
    if(pipe == NULL)
        return IF_NULL_POINTER;

    IF_Pipeline_t *tmp_pipe = NULL;
    IF_MALLOC(tmp_pipe, (size_t)sizeof(IF_Pipeline_t));

    IF_MALLOC_LABEL(tmp_pipe->buff, initial_size * sizeof(IF_Operation_t), oom);

    tmp_pipe->_size = initial_size;
    tmp_pipe->len = 0;

    *pipe = tmp_pipe;

    return IF_SUCCESS;

    oom:
        free(tmp_pipe);
        return IF_OUT_OF_MEMORY;
}

#define INIT_SIZE 10

NODISCARD IF_error_t IF_pipe_init(IF_Pipeline_t **pipe) {
    return IF_pipe_init_size(pipe, INIT_SIZE);
}

NODISCARD IF_error_t IF_pipe_push(IF_Pipeline_t *pipe, IF_SupportedOp_t supp_op, IF_OpType_t op_type, IF_DevType_t pref_dev, IF_OpArgs_t op_args) {
    CHECK_PIPE_REF(pipe);

    if(pipe->len == pipe->_size) {
        IF_REALLOC(pipe->buff, pipe->_size * 2);
        pipe->_size *= 2;
    }

    IF_Operation_t *op = pipe->buff + pipe->len;

    op->supp_op = supp_op;
    op->op_args = op_args;
    op->op_type = op_type;

    // WARN: This makes a copy, the union should be pretty small.
    op->op_args = op_args;

    pipe->len++;

    return IF_SUCCESS;
}

NODISCARD IF_error_t IF_pipe_get(IF_Pipeline_t *pipe, size_t i, IF_Operation_t **operation) {
    CHECK_PIPE_REF(pipe);

    if(pipe->len == 0) {
        return IF_INVALID_ARGS;
    }

    *operation = pipe->buff + i;

    return IF_SUCCESS;
}

NODISCARD IF_error_t IF_pipe_free(IF_Pipeline_t *pipe) {
    CHECK_PIPE_REF(pipe);

    free(pipe->buff);
    free(pipe);

    return IF_SUCCESS;
}

NODISCARD IF_error_t IF_pipe_push_grayscale(IF_Pipeline_t *pipe) {
    CHECK_PIPE_REF(pipe);

    IF_OpArgs_t empty = { .empty = {} };
    IF_CHECK(IF_pipe_push(pipe, IF_OP_GRAYSCALE, IF_TRAVERSAL_POINT, IF_DEV_GPU, empty));

    return IF_SUCCESS;
}

NODISCARD IF_error_t IF_pipe_push_invert(IF_Pipeline_t *pipe) {
    CHECK_PIPE_REF(pipe);

    IF_OpArgs_t empty = { .empty = {} };
    IF_CHECK(IF_pipe_push(pipe, IF_OP_INVERT, IF_TRAVERSAL_POINT, IF_DEV_GPU, empty));

    return IF_SUCCESS;
}

NODISCARD IF_error_t IF_pipe_push_brightness(IF_Pipeline_t *pipe, float factor) {
    CHECK_PIPE_REF(pipe);

    IF_OpArgs_t args = { .brightness = { .factor = factor } };
    IF_CHECK(IF_pipe_push(pipe, IF_OP_BRIGHTNESS, IF_TRAVERSAL_POINT, IF_DEV_GPU, args));

    return IF_SUCCESS;
}
