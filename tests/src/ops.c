#include "IF_tests/error.h"
#include "IF_tests/utils.h"
#include "ImageFlow/devices/devices.h"
#include "ImageFlow/devices/img_loader.h"
#include "ImageFlow/io/image.h"
#include "ImageFlow/operations/operations.h"
#include "fastest/tests.h"
#include "fastest/custom_tests.h"
#include <stdlib.h>

static int seed = 0;

#define TESTS_DIM_X 5
#define TESTS_DIM_Y 5
#define TESTS_CHANNELS 3

static void check_against_cpu(IF_DevType_t dev, IF_SupportedOp_t op, IF_OpArgs_t args, FASTEST_TestOutput_t *out) {
    PROBE_DEV(out, dev);

    IF_OpImpl_t *dev_impl = IF_op_get_impl(op, dev);
    IF_OpImpl_t *host_impl = IF_op_get_impl(op, IF_DEV_CPU);
    CHECK_IMPL(host_impl, IF_DEV_CPU);
    CHECK_IMPL(dev_impl, dev);

    IF_image_t host_img;
    IF_image_t dev_img;
    IF_image_t *imgs[_IF_DEV_LEN] = {0};

    IF_FASTEST_CHECK(IF_genRandomImage(&host_img, TESTS_DIM_X, TESTS_DIM_Y, TESTS_CHANNELS, seed++));
    IF_FASTEST_CHECK(IF_copyImage(&host_img, &dev_img));     // independent copy for the dev path

    imgs[IF_DEV_CPU] = &host_img;
    IF_FASTEST_CHECK(host_impl->func(args, imgs));   // host_img now holds the reference result

    imgs[IF_DEV_CPU] = &dev_img;
    IF_FASTEST_CHECK(IF_host2dev(dev, imgs));
    IF_FASTEST_CHECK(dev_impl->func(args, imgs));
    IF_FASTEST_CHECK(IF_dev2host(dev, imgs));

    int ok = 0;
    IF_FASTEST_CHECK(IF_imageCompare(&host_img, &dev_img, 1.0f / 255.0f, &ok));

    out->test_flags  |= FASTEST_ASSERT_EQ;
    out->exit_status |= ok ? FASTEST_SUCCESS : FASTEST_ERROR_ASSERT;

    IF_FASTEST_CHECK(IF_storeImageQ(&host_img, "/tmp/if_ops.png", 100));
    IF_FASTEST_CHECK(IF_storeImageQ(&dev_img, "/tmp/if_ops2.png", 100));

    IF_FASTEST_CHECK(IF_freeImage(&host_img));
    IF_FASTEST_CHECK(IF_freeImage(&dev_img));
}

#define IF_OP_DEF(op, name, type, args_type) \
\
FASTEST_CUSTOMTEST_INLINE("ops/" #name "/check_against_cpu", FASTEST_TIME_NS | FASTEST_FAIL_ERROR, NULL, \
    { \
        IF_OpArgs_t args; \
        default_##op##_args(&args); \
        for(IF_DevType_t dev = 0; dev < _IF_DEV_LEN; dev++) { \
            check_against_cpu(dev, IF_OP_##op, args, out); \
        } \
    } \
)

#include "ImageFlow/operations/operations.def"

#undef IF_OP_DEF
