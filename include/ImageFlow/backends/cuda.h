#include <cmath>
#include <cstddef>
#include <cstdio>
#include <cuda_runtime.h>
#include "ImageFlow/constructor.h"
#include "ImageFlow/devices/devices.h"
#include "ImageFlow/operations/operations.h"
#include "ImageFlow/operations/op_constructor.h"
#include "ImageFlow/devices/img_loader.h"
#include "ImageFlow/error.h"
#include "ImageFlow/io/image.h"

#define IF_CUDA_CHECK(call) call
#define IF_CUDA_KERNEL_CHECK() ;

static IF_error_t IFCU_getDevices(int *cuda_dev) {
    cudaError_t err = cudaGetDeviceCount(cuda_dev);
    if (err != cudaSuccess)
        return IF_DEVICE_ERROR;
    return IF_SUCCESS;
}

IF_CONSTRUCTOR(IF_register_cuda_availability) {
    int count = 0;
    IF_error_t err = IFCU_getDevices(&count);
    if (err == IF_SUCCESS && count > 0) {
        IF_enable_device(IF_DEV_CUDA);
    } else {
        IF_disable_device(IF_DEV_CUDA);
    }
}

static inline IF_error_t IFCU_checkAvail(void) {
    if (!IF_device_enabled(IF_DEV_CUDA))
        return IF_BACKEND_UNAVAILABLE;
    return IF_SUCCESS;
}

static inline IF_error_t IFCU_checkImg(IF_image_t *img) {
    IF_CHECK(IFCU_checkAvail());
    if (img == NULL || img->data == NULL)
        return IF_NULL_POINTER;
    if (img->channels < 3)
        return IF_INVALID_CHANNELS;
    return IF_SUCCESS;
}

__global__ void IFCUK_grayScale(IF_image_t *img);
__global__ void IFCUK_invert(IF_image_t *img);
__global__ void IFCUK_brightness(IF_image_t *img, float factor);

/**
 * @brief Updates the device image (imgs[IF_DEV_CUDA]) in place with the
 *        current host pixel data. If imgs[IF_DEV_CUDA] is NULL (first use,
 *        or after a teardown), allocates the device-side struct and its
 *        data buffer sized for the current image; otherwise reuses the
 *        existing allocation, assuming it's already sized correctly.
 *
 * @warning If a later call passes an img with a larger width/height/channels
 *          than whatever imgs[IF_DEV_CUDA] was originally allocated for,
 *          this does NOT re-allocate/resize and will overflow the device
 *          buffer. Only safe if image dimensions are constant across the
 *          lifetime of a given imgs[IF_DEV_CUDA] allocation.
 */
IF_LOAD_IMG_IMPL(CUDA) {
    IF_image_t *img = imgs[IF_DEV_CPU];
    IF_image_t *cuda_img = imgs[IF_DEV_CUDA];
    IF_CHECK(IFCU_checkImg(img));

    size_t n_bytes = (size_t)img->width * img->height * img->channels * sizeof(float);

    if (cuda_img == NULL) {
        float *cuda_data;
        IF_CUDA_CHECK(cudaMalloc(&cuda_data, n_bytes));
        IF_CUDA_CHECK(cudaMemcpy(cuda_data, img->data, n_bytes, cudaMemcpyHostToDevice));

        IF_image_t desc = *img;
        desc.data = cuda_data;

        IF_CUDA_CHECK(cudaMalloc((void**)&cuda_img, sizeof(IF_image_t)));
        IF_CUDA_CHECK(cudaMemcpy(cuda_img, &desc, sizeof(IF_image_t), cudaMemcpyHostToDevice));

        imgs[IF_DEV_CUDA] = cuda_img;
        return IF_SUCCESS;
    }

    /* Pull the struct back to host to recover the existing device data
     * pointer — cuda_img itself lives in device memory, so its fields
     * can't be read directly from host code. */
    IF_image_t desc;
    IF_CUDA_CHECK(cudaMemcpy(&desc, cuda_img, sizeof(IF_image_t), cudaMemcpyDeviceToHost));

    IF_CUDA_CHECK(cudaMemcpy(desc.data, img->data, n_bytes, cudaMemcpyHostToDevice));

    /* Refresh metadata in case width/height/channels changed, reusing
     * the same device data pointer (desc.data is untouched above). */
    desc.width = img->width;
    desc.height = img->height;
    desc.channels = img->channels;
    IF_CUDA_CHECK(cudaMemcpy(cuda_img, &desc, sizeof(IF_image_t), cudaMemcpyHostToDevice));

    return IF_SUCCESS;
}

IF_RETRIEVE_IMG_IMPL(CUDA) {
    IF_CHECK(IFCU_checkAvail());
    IF_image_t *img = imgs[IF_DEV_CPU];
    IF_image_t *cuda_img = imgs[IF_DEV_CUDA];

    IF_image_t desc;
    IF_CUDA_CHECK(cudaMemcpy(&desc, cuda_img, sizeof(IF_image_t), cudaMemcpyDeviceToHost));

    size_t n = (size_t)desc.width * desc.height * desc.channels;
    IF_CUDA_CHECK(cudaMemcpy(img->data, desc.data, n * sizeof(float), cudaMemcpyDeviceToHost));

    return IF_SUCCESS;
}

IF_FREE_IMG_IMPL(CUDA) {
    IF_image_t *cuda_img = imgs[IF_DEV_CUDA];

    IF_image_t desc;
    IF_CUDA_CHECK(cudaMemcpy(&desc, cuda_img, sizeof(IF_image_t), cudaMemcpyDeviceToHost));

    IF_CUDA_CHECK(cudaFree(desc.data));
    IF_CUDA_CHECK(cudaFree(cuda_img));

    imgs[IF_DEV_CUDA] = NULL;
    return IF_SUCCESS;
}

IF_OP_IMPL(CUDA, GRAYSCALE, 1, 1) {
    IF_image_t *img = imgs[IF_DEV_CPU];
    IF_image_t *cuda_img = imgs[IF_DEV_CUDA];
    IF_CHECK(IFCU_checkImg(img));

    int w = img->width, h = img->height;
    dim3 block(16, 16);
    dim3 grid((w + block.x - 1) / block.x, (h + block.y - 1) / block.y);

    IFCUK_grayScale<<<grid, block>>>(cuda_img);
    IF_CUDA_KERNEL_CHECK();
    IF_CUDA_CHECK(cudaDeviceSynchronize());
    return IF_SUCCESS;
}

IF_OP_IMPL(CUDA, INVERT, 1, 1) {
    IF_image_t *img = imgs[IF_DEV_CPU];
    IF_image_t *cuda_img = imgs[IF_DEV_CUDA];
    IF_CHECK(IFCU_checkImg(img));

    int w = img->width, h = img->height;
    dim3 block(16, 16);
    dim3 grid((w + block.x - 1) / block.x, (h + block.y - 1) / block.y);

    IFCUK_invert<<<grid, block>>>(cuda_img);
    IF_CUDA_KERNEL_CHECK();
    IF_CUDA_CHECK(cudaDeviceSynchronize());
    return IF_SUCCESS;
}

IF_OP_IMPL(CUDA, BRIGHTNESS, 1, 1) {
    IF_image_t *img = imgs[IF_DEV_CPU];
    IF_image_t *cuda_img = imgs[IF_DEV_CUDA];
    IF_CHECK(IFCU_checkImg(img));

    int w = img->width, h = img->height;
    dim3 block(16, 16);
    dim3 grid((w + block.x - 1) / block.x, (h + block.y - 1) / block.y);

    IFCUK_brightness<<<grid, block>>>(cuda_img, args.float_factor.factor);
    IF_CUDA_KERNEL_CHECK();
    IF_CUDA_CHECK(cudaDeviceSynchronize());
    return IF_SUCCESS;
}
