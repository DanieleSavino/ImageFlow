#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include "ImageFlow/devices/devices.h"
#include "fastest/tests.h"

#define PROBE_DEV(test, dev) \
    int on = IF_device_enabled(dev); \
    if(!on) \
        test->exit_status |= FASTEST_SKIPPED;

#ifdef __cplusplus
}
#endif
