#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include "ImageFlow/devices/devices.h"
#include "fastest/tests.h"
#include "IF_tests/vars.h"

#define PROBE_DEV(test, dev) \
    int on = IF_device_enabled(dev); \
    if(!on) { \
        test->exit_status |= FASTEST_SKIPPED; \
        return; \
    }

#define PROBE_DEV_LOG(dev) \
    do { \
        if (!IF_device_enabled(dev)) { \
            if(getenv(IF_TESTS_VERBOSE) != NULL && strcmp(getenv(IF_TESTS_VERBOSE), "1") == 0) \
                fprintf(stderr, "[PROBE_CHECK_LOG] %s:%d: device %s not enabled, skipping\n", \
                    __FILE__, __LINE__, IF_strdev(dev)); \
            return; \
        } \
    } while (0)

#define CHECK_IMPL(impl, dev) \
    do { \
        if (impl == NULL) { \
            fprintf(stderr, "[check_impl] op %s not implemented for device %s, skipping\n", \
                    IF_strop(op), IF_strdev(dev)); \
            return; \
        } \
    } while (0)

#ifdef __cplusplus
}
#endif
