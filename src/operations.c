#include "ImageFlow/operations.h"
#include "ImageFlow/accelerated/acc_wrapper.h"
#include "ImageFlow/error.h"
#include "ImageFlow/omp/backend.h"

/**
 * FIXME: For now we just do "if there's a gpu then use it else fallback"
 * the goal of the project is to have a builder pattern like api that actively
 * schedules the requests, using the loaded api when possible, that's why there are 2 wrappers,
 * so for example the cpu might handle simple jobs like resizing, trimming and easy jobs on small images.
 *
 * WARN: The direct IF_operationX() api will probably be deprecated after the transition to the
 * builder pattern api is complete.
 */

IF_error_t IF_grayScale(IF_image_t *img) {
    if(IFACC_available()) {
        return IFACC_grayScale(img);
    }
    else {
        return IFOMP_grayScale(img);
    }
}

IF_error_t IF_invert(IF_image_t *img) {
    if(IFACC_available()) {
        return IFACC_invert(img);
    }
    else {
        return IFOMP_invert(img);
    }
}

IF_error_t IF_brightness(IF_image_t *img, float factor) {
    if(IFACC_available()) {
        return IFACC_brightness(img, factor);
    }
    else {
        return IFOMP_brightness(img, factor);
    }
}
