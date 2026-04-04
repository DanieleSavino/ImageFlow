#include "ImageFlow/error.h"

#include "ImageFlow/io/image.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * WARN: I'm not interested in making this multi-gpu for now.
 */
typedef enum {
    UNDEF = 0,
    NO_ACC,
    CUDA_ACC,
    HIP_ACC
} IF_acc_t;

IF_error_t IFACC_check_system();
IF_error_t IFACC_available();

IF_error_t IFACC_load(const IF_image_t *img, IF_image_t **cuda_img);
IF_error_t IFACC_retrieve(IF_image_t **cuda_img, IF_image_t *img_out);

IF_error_t IFACC_grayScale(IF_image_t *img);
IF_error_t IFACC_invert(IF_image_t *img);
IF_error_t IFACC_brightness(IF_image_t *img, float factor);

IF_error_t IFACC_loaded_grayScale(const IF_image_t *img, IF_image_t *cuda_img);
IF_error_t IFACC_loaded_invert(const IF_image_t *img, IF_image_t *cuda_img);
IF_error_t IFACC_loaded_brightness(const IF_image_t *img, IF_image_t *cuda_img, float factor);

#ifdef __cplusplus
}
#endif
