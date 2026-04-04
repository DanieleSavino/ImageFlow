#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include "ImageFlow/error.h"
#include "ImageFlow/io/image.h"

IF_error_t IFOMP_grayScale(IF_image_t *img);
IF_error_t IFOMP_invert(IF_image_t *img);
IF_error_t IFOMP_brightness(IF_image_t *img, float factor);

#ifdef __cplusplus
}
#endif
