#include "ImageFlow/error.h"
#include "ImageFlow/io/image.h"
#include <ImageFlow/accelerated/acc_wrapper.h>

#ifdef _CUDA_ACC
#include "ImageFlow/accelerated/cuda/backend.h"
#endif
#ifdef _HIP_ACC
#include "ImageFlow/accelerated/hip/backend.h"
#endif

/* ---------- dispatch table ---------- */

typedef struct {
    IF_error_t (*getDevices)(int *);
    IF_error_t (*load)(const IF_image_t *, IF_image_t **);
    IF_error_t (*retrieve)(IF_image_t **, IF_image_t *);
    IF_error_t (*grayScale)(IF_image_t *);
    IF_error_t (*invert)(IF_image_t *);
    IF_error_t (*brightness)(IF_image_t *, float);
    IF_error_t (*loaded_grayScale)(IF_image_t *, IF_image_t *);
    IF_error_t (*loaded_invert)(IF_image_t *, IF_image_t *);
    IF_error_t (*loaded_brightness)(IF_image_t *, IF_image_t *, float);
} IF_backend_t;

static IF_acc_t acc_comp = UNDEF;
static IF_backend_t backend;

/* ---------- system check ---------- */

IF_error_t IFACC_check_system() {
    acc_comp = NO_ACC;

#ifdef _CUDA_ACC
    {
        int n = 0;
        IF_CHECK(IFCU_getDevices(&n));
        if (n > 0) {
            acc_comp = CUDA_ACC;
            backend = (IF_backend_t){
                .load             = IFCU_load,
                .retrieve         = IFCU_retrieve,
                .grayScale        = IFCU_grayScale,
                .invert           = IFCU_invert,
                .brightness       = IFCU_brightness,
                .loaded_grayScale = IFCU_loaded_grayScale,
                .loaded_invert    = IFCU_loaded_invert,
                .loaded_brightness= IFCU_loaded_brightness,
            };
            return IF_SUCCESS;
        }
    }
#endif

#ifdef _HIP_ACC
    {
        int n = 0;
        IFHIP_getDevices(&n);
        if (n > 0) {
            acc_comp = HIP_ACC;
            backend = (IF_backend_t){
                .load             = IFHIP_load,
                .retrieve         = IFHIP_retrieve,
                .grayScale        = IFHIP_grayScale,
                .invert           = IFHIP_invert,
                .brightness       = IFHIP_brightness,
                .loaded_grayScale = IFHIP_loaded_grayScale,
                .loaded_invert    = IFHIP_loaded_invert,
                .loaded_brightness= IFHIP_loaded_brightness,
            };
            return IF_SUCCESS;
        }
    }
#endif

    return IF_SUCCESS;
}

NODISCARD static inline IF_error_t check_comp() {
    if (acc_comp == UNDEF)       IF_CHECK(IFACC_check_system());
    if (acc_comp == NO_ACC)      return IF_DEVICE_ERROR;
    return IF_SUCCESS;
}

int IFACC_available() {
    IF_error_t err = check_comp();
    return err == IF_SUCCESS;
}

/* ---------- public API ---------- */

IF_error_t IFACC_load(const IF_image_t *img, IF_image_t **out) {
    IF_CHECK(check_comp()); return backend.load(img, out);
}
IF_error_t IFACC_retrieve(IF_image_t **dev, IF_image_t *out) {
    IF_CHECK(check_comp()); return backend.retrieve(dev, out);
}
IF_error_t IFACC_grayScale(IF_image_t *img) {
    IF_CHECK(check_comp()); return backend.grayScale(img);
}
IF_error_t IFACC_invert(IF_image_t *img) {
    IF_CHECK(check_comp()); return backend.invert(img);
}
IF_error_t IFACC_brightness(IF_image_t *img, float f) {
    IF_CHECK(check_comp()); return backend.brightness(img, f);
}
IF_error_t IFACC_loaded_grayScale(IF_image_t *img, IF_image_t *dev) {
    IF_CHECK(check_comp()); return backend.loaded_grayScale(img, dev);
}
IF_error_t IFACC_loaded_invert(IF_image_t *img, IF_image_t *dev) {
    IF_CHECK(check_comp()); return backend.loaded_invert(img, dev);
}
IF_error_t IFACC_loaded_brightness(IF_image_t *img, IF_image_t *dev, float f) {
    IF_CHECK(check_comp()); return backend.loaded_brightness(img, dev, f);
}
