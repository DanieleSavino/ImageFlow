#pragma once
#ifdef __cplusplus
extern "C" {
#endif

#define IF_DEV_DEF(dev) IF_DEV_##dev,

typedef enum {
    #include "devices.def"
    _IF_DEV_LEN
} IF_DevType_t;

#undef IF_DEV_DEF


#define IF_DEV_DEF(dev) #dev,

static const char *IF_DevNames[_IF_DEV_LEN] = {
    #include "devices.def"
};

#undef IF_DEV_DEF

static inline const char* IF_strdev(IF_DevType_t dev) {
    return IF_DevNames[dev];
}


typedef enum {
    IF_STATUS_UNKNOWN = 0,
    IF_STATUS_OFF,
    IF_STATUS_ON
} IF_DevStatus_t;

extern IF_DevStatus_t IF_devices_enabled[_IF_DEV_LEN];

static inline void IF_enable_device(IF_DevType_t dev) {
    IF_devices_enabled[dev] = IF_STATUS_ON;
}

static inline void IF_disable_device(IF_DevType_t dev) {
    IF_devices_enabled[dev] = IF_STATUS_OFF;
}

static inline int IF_device_enabled(IF_DevType_t dev) {
    return IF_devices_enabled[dev] == IF_STATUS_ON;
}

IF_DevType_t IF_enabled_gpu();

#ifdef __cplusplus
}
#endif
