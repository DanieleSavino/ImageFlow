#include "ImageFlow/devices/devices.h"
#include "ImageFlow/devices/img_loader.h"

IF_DevStatus_t IF_devices_enabled[_IF_DEV_LEN];
IF_load_img_func_ptr_t    IF_LoadImgFuncs[_IF_DEV_LEN];
IF_retrieve_img_func_ptr_t IF_RetrieveImgFuncs[_IF_DEV_LEN];
IF_free_img_func_ptr_t IF_FreeImgFuncs[_IF_DEV_LEN];
