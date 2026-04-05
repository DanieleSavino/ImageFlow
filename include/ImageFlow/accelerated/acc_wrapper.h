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

NODISCARD IF_error_t IFACC_check_system();
int IFACC_available();

NODISCARD IF_error_t IFACC_load(const IF_image_t *img, IF_image_t **cuda_img);
NODISCARD IF_error_t IFACC_retrieve(IF_image_t **cuda_img, IF_image_t *img_out);

NODISCARD IF_error_t IFACC_grayScale(IF_image_t *img);
NODISCARD IF_error_t IFACC_invert(IF_image_t *img);
NODISCARD IF_error_t IFACC_brightness(IF_image_t *img, float factor);

NODISCARD IF_error_t IFACC_loaded_grayScale(const IF_image_t *img, IF_image_t *cuda_img);
NODISCARD IF_error_t IFACC_loaded_invert(const IF_image_t *img, IF_image_t *cuda_img);
NODISCARD IF_error_t IFACC_loaded_brightness(const IF_image_t *img, IF_image_t *cuda_img, float factor);

#ifdef __cplusplus
}
#endif
