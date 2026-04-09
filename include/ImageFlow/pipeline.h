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

NODISCARD IF_error_t IF_pipe_init_size(IF_Pipeline_t **pipe, int initial_size);
NODISCARD IF_error_t IF_pipe_init(IF_Pipeline_t **pipe);

// WARN: This model trades memory usage for simplicity and 0-copy, this list is not expected to be huge so should be fine.
NODISCARD IF_error_t IF_pipe_push(IF_Pipeline_t *pipe, IF_SupportedOp_t supp_op, IF_OpType_t op_type, IF_DevType_t pref_dev, IF_OpArgs_t op_args);
NODISCARD IF_error_t IF_pipe_get(IF_Pipeline_t *pipe, size_t i, IF_Operation_t **operation);

NODISCARD IF_error_t IF_pipe_free(IF_Pipeline_t *pipe);

NODISCARD IF_error_t IF_pipe_push_grayscale(IF_Pipeline_t *pipe);
NODISCARD IF_error_t IF_pipe_push_invert(IF_Pipeline_t *pipe);
NODISCARD IF_error_t IF_pipe_push_brightness(IF_Pipeline_t *pipe, float factor);

#ifdef __cplusplus
}
#endif
