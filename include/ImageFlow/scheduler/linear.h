#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include "ImageFlow/error.h"
#include "ImageFlow/pipeline.h"

NODISCARD IF_error_t IF_linear_execute(IF_Pipeline_t *pipe, const IF_image_t *img_in, IF_image_t *img_out);

#ifdef __cplusplus
}
#endif
