#pragma once

/**
 * FIXME: For now we just do "if there's a gpu then use it else fallback"
 * the goal of the project is to have a builder pattern like api that actively
 * schedules the requests, using the loaded api when possible, that's why there are 2 wrappers,
 * so for example the cpu might handle simple jobs like resizing, trimming and easy jobs on small images.
 *
 * WARN: The direct IF_operationX() api will probably be deprecated after the transition to the
 * builder pattern api is complete.
 */

#ifdef __cplusplus
extern "C" {
#endif

#include "ImageFlow/error.h"
#include "ImageFlow/io/image.h"

IF_error_t IF_grayScale(IF_image_t *img);
IF_error_t IF_invert(IF_image_t *img);
IF_error_t IF_brightness(IF_image_t *img, float factor);

#ifdef __cplusplus
}
#endif
