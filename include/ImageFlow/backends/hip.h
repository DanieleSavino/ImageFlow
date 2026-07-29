#include <cmath>
#include <cstddef>
#include <cstdio>
#include <hip/hip_runtime.h>
#include "ImageFlow/constructor.h"
#include "ImageFlow/devices/devices.h"
#include "ImageFlow/operations/operations.h"
#include "ImageFlow/operations/op_constructor.h"
#include "ImageFlow/devices/img_loader.h"
#include "ImageFlow/error.h"
#include "ImageFlow/io/image.h"

/**
 * @brief Checks a HIP API call and returns IF_HIP_ERROR on failure.
 *
 * Prints the HIP error string, file, and line to stderr before returning.
 * Intended for use around any hipXxx() runtime API call inside a function
 * returning IF_error_t.
 *
 * @param call A HIP runtime API call expression returning hipError_t.
 */
#define IF_HIP_CHECK(call)                                                     \
    do {                                                                       \
        hipError_t __err__ = call;                                             \
        if (__err__ != hipSuccess) {                                           \
            fprintf(stderr,                                                    \
                    "HIP Error: %s in file %s at line %d\n",                   \
                    hipGetErrorString(__err__),                                 \
                    __FILE__,                                                  \
                    __LINE__);                                                  \
            return IF_HIP_ERROR;                                               \
        }                                                                      \
    } while (0)

/**
 * @brief Checks the last HIP kernel launch for errors, returning IF_KERNEL_FAILURE on failure.
 *
 * In DEBUG builds: calls hipGetLastError() followed by hipDeviceSynchronize(),
 * catching both launch configuration errors and runtime execution errors.
 * In release builds: calls hipGetLastError() only; execution errors are
 * caught by the explicit hipDeviceSynchronize() that follows each kernel launch.
 *
 * @note Must be called immediately after a kernel launch (<<<...>>>) before
 *       any other HIP call that could clear the error state.
 */
#ifdef DEBUG
    #define IF_HIP_KERNEL_CHECK()                                              \
        do {                                                                   \
            hipError_t __err = hipGetLastError();                              \
            if (__err != hipSuccess) {                                         \
                fprintf(stderr, "HIP Launch Error [%s]: %s\nFile: %s | Line: %d\n", \
                        hipGetErrorName(__err), hipGetErrorString(__err),      \
                        __FILE__, __LINE__);                                   \
                return IF_KERNEL_FAILURE;                                      \
            }                                                                  \
            __err = hipDeviceSynchronize();                                    \
            if (__err != hipSuccess) {                                         \
                fprintf(stderr, "HIP Execution Error [%s]: %s\nFile: %s | Line: %d\n", \
                        hipGetErrorName(__err), hipGetErrorString(__err),      \
                        __FILE__, __LINE__);                                   \
                return IF_KERNEL_FAILURE;                                      \
            }                                                                  \
        } while (0)
#else
    #define IF_HIP_KERNEL_CHECK()                                              \
        do {                                                                   \
            hipError_t __err = hipGetLastError();                              \
            if (__err != hipSuccess) {                                         \
                fprintf(stderr, "HIP Launch Error [%s]: %s\nFile: %s | Line: %d\n", \
                        hipGetErrorName(__err), hipGetErrorString(__err),      \
                        __FILE__, __LINE__);                                   \
                return IF_KERNEL_FAILURE;                                      \
            }                                                                  \
        } while (0)
#endif

static IF_error_t IFHIP_getDevices(int *hip_dev) {
    hipError_t err = hipGetDeviceCount(hip_dev);
    if (err != hipSuccess)
        return IF_DEVICE_ERROR;
    return IF_SUCCESS;
}

IF_CONSTRUCTOR(IF_register_hip_availability) {
    int count = 0;
    IF_error_t err = IFHIP_getDevices(&count);
    if (err == IF_SUCCESS && count > 0) {
        IF_enable_device(IF_DEV_HIP);
    } else {
        IF_disable_device(IF_DEV_HIP);
    }
}

static inline IF_error_t IFHIP_checkAvail(void) {
    if (!IF_device_enabled(IF_DEV_HIP))
        return IF_BACKEND_UNAVAILABLE;
    return IF_SUCCESS;
}

static inline IF_error_t IFHIP_checkImg(IF_image_t *img) {
    IF_CHECK(IFHIP_checkAvail());
    if (img == NULL || img->data == NULL)
        return IF_NULL_POINTER;
    if (img->channels < 3)
        return IF_INVALID_CHANNELS;
    return IF_SUCCESS;
}

__global__ void IFHIPK_grayScale(IF_image_t *img);
__global__ void IFHIPK_invert(IF_image_t *img);
__global__ void IFHIPK_brightness(IF_image_t *img, float factor);

/**
 * @brief Updates the device image (imgs[IF_DEV_HIP]) in place with the
 *        current host pixel data. If imgs[IF_DEV_HIP] is NULL (first use,
 *        or after a teardown), allocates the device-side struct and its
 *        data buffer sized for the current image; otherwise reuses the
 *        existing allocation, assuming it's already sized correctly.
 *
 * @warning If a later call passes an img with a larger width/height/channels
 *          than whatever imgs[IF_DEV_HIP] was originally allocated for,
 *          this does NOT re-allocate/resize and will overflow the device
 *          buffer. Only safe if image dimensions are constant across the
 *          lifetime of a given imgs[IF_DEV_HIP] allocation.
 */
IF_LOAD_IMG_IMPL(HIP) {
    IF_image_t *img = imgs[IF_DEV_CPU];
    IF_image_t *hip_img = imgs[IF_DEV_HIP];
    IF_CHECK(IFHIP_checkImg(img));

    size_t n_bytes = (size_t)img->width * img->height * img->channels * sizeof(float);

    if (hip_img == NULL) {
        float *hip_data;
        IF_HIP_CHECK(hipMalloc(&hip_data, n_bytes));
        IF_HIP_CHECK(hipMemcpy(hip_data, img->data, n_bytes, hipMemcpyHostToDevice));

        IF_image_t desc = *img;
        desc.data = hip_data;

        IF_HIP_CHECK(hipMalloc((void**)&hip_img, sizeof(IF_image_t)));
        IF_HIP_CHECK(hipMemcpy(hip_img, &desc, sizeof(IF_image_t), hipMemcpyHostToDevice));

        imgs[IF_DEV_HIP] = hip_img;
        return IF_SUCCESS;
    }

    /* Pull the struct back to host to recover the existing device data
     * pointer — hip_img itself lives in device memory, so its fields
     * can't be read directly from host code. */
    IF_image_t desc;
    IF_HIP_CHECK(hipMemcpy(&desc, hip_img, sizeof(IF_image_t), hipMemcpyDeviceToHost));

    IF_HIP_CHECK(hipMemcpy(desc.data, img->data, n_bytes, hipMemcpyHostToDevice));

    /* Refresh metadata in case width/height/channels changed, reusing
     * the same device data pointer (desc.data is untouched above). */
    desc.width = img->width;
    desc.height = img->height;
    desc.channels = img->channels;
    IF_HIP_CHECK(hipMemcpy(hip_img, &desc, sizeof(IF_image_t), hipMemcpyHostToDevice));

    return IF_SUCCESS;
}

IF_RETRIEVE_IMG_IMPL(HIP) {
    IF_CHECK(IFHIP_checkAvail());
    IF_image_t *img = imgs[IF_DEV_CPU];
    IF_image_t *hip_img = imgs[IF_DEV_HIP];

    IF_image_t desc;
    IF_HIP_CHECK(hipMemcpy(&desc, hip_img, sizeof(IF_image_t), hipMemcpyDeviceToHost));

    size_t n = (size_t)desc.width * desc.height * desc.channels;
    IF_HIP_CHECK(hipMemcpy(img->data, desc.data, n * sizeof(float), hipMemcpyDeviceToHost));

    return IF_SUCCESS;
}

IF_FREE_IMG_IMPL(HIP) {
    IF_image_t *hip_img = imgs[IF_DEV_HIP];

    IF_image_t desc;
    IF_HIP_CHECK(hipMemcpy(&desc, hip_img, sizeof(IF_image_t), hipMemcpyDeviceToHost));

    IF_HIP_CHECK(hipFree(desc.data));
    IF_HIP_CHECK(hipFree(hip_img));

    imgs[IF_DEV_HIP] = NULL;
    return IF_SUCCESS;
}

IF_OP_IMPL(HIP, GRAYSCALE, 1, 1) {
    IF_image_t *img = imgs[IF_DEV_CPU];
    IF_image_t *hip_img = imgs[IF_DEV_HIP];
    IF_CHECK(IFHIP_checkImg(img));

    int w = img->width, h = img->height;
    dim3 block(16, 16);
    dim3 grid((w + block.x - 1) / block.x, (h + block.y - 1) / block.y);

    hipLaunchKernelGGL(IFHIPK_grayScale, grid, block, 0, 0, hip_img);
    IF_HIP_KERNEL_CHECK();
    IF_HIP_CHECK(hipDeviceSynchronize());
    return IF_SUCCESS;
}

IF_OP_IMPL(HIP, INVERT, 1, 1) {
    IF_image_t *img = imgs[IF_DEV_CPU];
    IF_image_t *hip_img = imgs[IF_DEV_HIP];
    IF_CHECK(IFHIP_checkImg(img));

    int w = img->width, h = img->height;
    dim3 block(16, 16);
    dim3 grid((w + block.x - 1) / block.x, (h + block.y - 1) / block.y);

    hipLaunchKernelGGL(IFHIPK_invert, grid, block, 0, 0, hip_img);
    IF_HIP_KERNEL_CHECK();
    IF_HIP_CHECK(hipDeviceSynchronize());
    return IF_SUCCESS;
}

IF_OP_IMPL(HIP, BRIGHTNESS, 1, 1) {
    IF_image_t *img = imgs[IF_DEV_CPU];
    IF_image_t *hip_img = imgs[IF_DEV_HIP];
    IF_CHECK(IFHIP_checkImg(img));

    int w = img->width, h = img->height;
    dim3 block(16, 16);
    dim3 grid((w + block.x - 1) / block.x, (h + block.y - 1) / block.y);

    hipLaunchKernelGGL(IFHIPK_brightness, grid, block, 0, 0, hip_img, args.float_factor.factor);
    IF_HIP_KERNEL_CHECK();
    IF_HIP_CHECK(hipDeviceSynchronize());
    return IF_SUCCESS;
}
