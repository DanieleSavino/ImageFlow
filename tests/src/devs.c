#include "ImageFlow/devices/devices.h"
#include "ImageFlow/devices/img_loader.h"
#include "ImageFlow/error.h"
#include "ImageFlow/io/image.h"
#include "fastest/custom_tests.h"
#include "fastest/tests.h"
#include "IF_tests/error.h"
#include "IF_tests/utils.h"
#include "IF_tests/vars.h"
#include <string.h>

static int seed = 0;

static void round_trip(IF_DevType_t dev, FASTEST_TestOutput_t *out) {
    PROBE_DEV(out, dev);

    int DIM_X;
    IF_GET_TEST_VAR(IF_TESTS_DIM_X, &DIM_X);
    int DIM_Y;
    IF_GET_TEST_VAR(IF_TESTS_DIM_Y, &DIM_Y);
    int CHANNELS;
    IF_GET_TEST_VAR(IF_TESTS_CHANNELS, &CHANNELS);

    IF_image_t img;
    IF_image_t img_out;
    IF_image_t *imgs[_IF_DEV_LEN] = {0};

    IF_FASTEST_CHECK(IF_genRandomImage(&img, DIM_X, DIM_Y, CHANNELS, seed++));

    imgs[IF_DEV_CPU] = &img;

    IF_FASTEST_CHECK(IF_host2dev(dev, imgs));
    IF_FASTEST_CHECK(IF_dev2host(dev, imgs));

    IF_FASTEST_CHECK(IF_storeImageQ(&img, "/tmp/if_test.png", 100));
    IF_FASTEST_CHECK(IF_loadImage(&img_out, "/tmp/if_test.png"));

    int equal = 0;
    IF_FASTEST_CHECK(IF_imageCompare(&img, &img_out, 1.0f / 255.0f, &equal));
    out->test_flags  |= FASTEST_ASSERT_EQ;
    out->exit_status |= equal ? FASTEST_SUCCESS : FASTEST_ERROR_ASSERT;

    IF_FASTEST_CHECK(IF_freeImage(&img));
    IF_FASTEST_CHECK(IF_freeImage(&img_out));
    IF_FASTEST_CHECK(IF_img_free(dev, imgs));
}

static void double_free(IF_DevType_t dev, FASTEST_TestOutput_t *out) {
    PROBE_DEV(out, dev);

    int DIM_X;
    IF_GET_TEST_VAR(IF_TESTS_DIM_X, &DIM_X);
    int DIM_Y;
    IF_GET_TEST_VAR(IF_TESTS_DIM_Y, &DIM_Y);
    int CHANNELS;
    IF_GET_TEST_VAR(IF_TESTS_CHANNELS, &CHANNELS);

    IF_image_t img;
    IF_image_t *imgs[_IF_DEV_LEN] = {0};

    IF_FASTEST_CHECK(IF_genRandomImage(&img, DIM_X, DIM_Y, CHANNELS, seed++));

    imgs[IF_DEV_CPU] = &img;

    IF_FASTEST_CHECK(IF_host2dev(dev, imgs));

    IF_error_t err[2];

    err[0] = IF_img_free(dev, imgs);
    err[1] = IF_img_free(dev, imgs);

    out->test_flags |= FASTEST_ASSERT_EQ;
    out->exit_status |= (err[0] == IF_SUCCESS && (err[1] == IF_NULL_POINTER || dev == IF_DEV_CPU)) ? FASTEST_SUCCESS : FASTEST_ERROR_ASSERT;

    IF_FASTEST_CHECK(IF_freeImage(&img));
}

#define IF_DEV_DEF(name) \
\
FASTEST_CUSTOMTEST_INLINE("devs/" #name "/round_trip", FASTEST_TIME_NS | FASTEST_FAIL_ERROR, NULL, \
    { \
        round_trip(IF_DEV_##name, out); \
    } \
) \
\
FASTEST_CUSTOMTEST_INLINE("devs/" #name "/double_free", FASTEST_TIME_NS | FASTEST_FAIL_ERROR, NULL, \
    { \
        double_free(IF_DEV_##name, out); \
    } \
)

#include "ImageFlow/devices/devices.def"

#undef IF_DEV_DEF
