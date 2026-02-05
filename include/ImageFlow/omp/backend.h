#pragma once

#include "ImageFlow/error/error.h"
#include "ImageFlow/io/image.h"

IF_error_t IFOMP_grayScale(IF_image_t *img);
IF_error_t IFOMP_upscale(IF_image_t *img, int scale);
IF_error_t IFOMP_invert(IF_image_t *img);
IF_error_t IFOMP_brightness(IF_image_t *img, float factor);
