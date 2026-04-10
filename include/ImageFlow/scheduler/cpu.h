#pragma once

#include "ImageFlow/pipeline.h"

#ifdef __cplusplus
extern "C" {
#endif

NODISCARD IF_error_t IF_cpu_execute(IF_Flow_t flow, const IF_image_t *img_in, IF_image_t *img_out);

#ifdef __cplusplus
}
#endif
