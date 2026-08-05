#include "ImageFlow/devices/devices.h"
#include "ImageFlow/devices/img_loader.h"
#include "ImageFlow/io/image.h"
#include "fastest/custom_tests.h"
#include "fastest/tests.h"
#include "IF_tests/error.h"
#include <string.h>

#define TESTS_DIM_X 10
#define TESTS_DIM_Y 10
#define TESTS_CHANNELS 3

int seed = 0;

static void round_trip(IF_DevType_t dev, FASTEST_TestOutput_t *out) {
    IF_image_t img;
    IF_image_t img_out;
    IF_image_t *imgs[_IF_DEV_LEN] = {0};

    IF_FASTEST_CHECK(IF_genRandomImage(&img, TESTS_DIM_X, TESTS_DIM_Y, TESTS_CHANNELS, seed++));

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
}

FASTEST_CUSTOMTEST_INLINE("cpu_round_trip", FASTEST_TIME_NS | FASTEST_FAIL_ERROR, NULL,
    {
        round_trip(IF_DEV_CPU, out);
    }
)
