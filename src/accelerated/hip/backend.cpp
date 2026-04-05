#include "ImageFlow/accelerated/hip/backend.h"
#include <hip/hip_runtime.h>

#include "ImageFlow/error.h"
#include "ImageFlow/io/image.h"
#include <cstddef>

IF_error_t IFHIP_getDevices(int *hip_dev) {
    hipError_t _ = hipGetDeviceCount(hip_dev);
    return IF_SUCCESS;
}

static inline IF_error_t IFHIP_checkImg(const IF_image_t *img) {
    if(img == NULL || img->data == NULL) {
        return IF_NULL_POINTER;
    }
    if(img->channels < 3) {
        return IF_INVALID_CHANNELS;
    }

    return IF_SUCCESS;
}

static __global__ void IFHIPK_grayScale(IF_image_t *img) {
    int x = blockIdx.x * blockDim.x + threadIdx.x;
    int y = blockIdx.y * blockDim.y + threadIdx.y;

    int w = img->width;
    int h = img->height;
    int c = img->channels;

    if (x >= w || y >= h) return;

    int idx = (y * w + x) * c;
    float *pixel = img->data + idx;

    float gray = 0.299f * pixel[0] + 0.587f * pixel[1] + 0.114f * pixel[2];
    pixel[0] = gray;
    pixel[1] = gray;
    pixel[2] = gray;
}

static __global__ void IFHIPK_invert(IF_image_t *img) {
    int x = blockIdx.x * blockDim.x + threadIdx.x;
    int y = blockIdx.y * blockDim.y + threadIdx.y;

    int w = img->width;
    int h = img->height;
    int c = img->channels;

    if (x >= w || y >= h) return;

    int idx = (y * w + x) * c;
    float *pixel = img->data + idx;

    pixel[0] = 1.0f - pixel[0];
    pixel[1] = 1.0f - pixel[1];
    pixel[2] = 1.0f - pixel[2];
}

static __global__ void IFHIPK_brightness(IF_image_t *img, float factor) {
    int x = blockIdx.x * blockDim.x + threadIdx.x;
    int y = blockIdx.y * blockDim.y + threadIdx.y;

    int w = img->width;
    int h = img->height;
    int c = img->channels;

    if (x >= w || y >= h) return;

    int idx = (y * w + x) * c;
    float *pixel = img->data + idx;

    pixel[0] *= factor;
    pixel[1] *= factor;
    pixel[2] *= factor;
}

IF_error_t IFHIP_load(const IF_image_t *img, IF_image_t **hip_img) {
    IF_CHECK(IFHIP_checkImg(img));

    int w = img->width;
    int h = img->height;
    int c = img->channels;

    IF_HIP_CHECK(hipMalloc(hip_img, sizeof(IF_image_t)));

    float *hip_data;
    IF_HIP_CHECK(hipMalloc(&hip_data, w * h * c * sizeof(float)));
    IF_HIP_CHECK(hipMemcpy(hip_data, img->data, w * h * c * sizeof(float), hipMemcpyHostToDevice));

    IF_image_t host_img = *img;
    host_img.data = hip_data;
    IF_HIP_CHECK(hipMemcpy(*hip_img, &host_img, sizeof(IF_image_t), hipMemcpyHostToDevice));

    return IF_SUCCESS;
}

IF_error_t IFHIP_retrieve(IF_image_t **hip_img, IF_image_t *img_out) {
    IF_image_t *d_hip_img = *hip_img;

    IF_HIP_CHECK(hipMemcpy(img_out, d_hip_img, sizeof(IF_image_t), hipMemcpyDeviceToHost));

    int w = img_out->width;
    int h = img_out->height;
    int c = img_out->channels;

    float *img_data = (float*)malloc(w * h * c * sizeof(float));
    IF_HIP_CHECK(hipMemcpy(img_data, img_out->data, w * h * c * sizeof(float), hipMemcpyDeviceToHost));

    IF_HIP_CHECK(hipFree(img_out->data));

    img_out->data = img_data;

    IF_HIP_CHECK(hipFree(d_hip_img));
    *hip_img = NULL;

    return IF_SUCCESS;
}

IF_error_t IFHIP_loaded_grayScale(const IF_image_t *img, IF_image_t *hip_img) {
    if(hip_img == NULL) {
        return IF_NULL_POINTER;
    }

    int w = img->width;
    int h = img->height;

    dim3 block(16, 16);
    dim3 grid((w + block.x - 1)/block.x,
              (h + block.y - 1)/block.y);

    hipLaunchKernelGGL(IFHIPK_grayScale, grid, block, 0, 0, hip_img);
    IF_HIP_KERNEL_CHECK();

    IF_HIP_CHECK(hipDeviceSynchronize());

    return IF_SUCCESS;
}

IF_error_t IFHIP_grayScale(IF_image_t *img) {
    IF_image_t *hip_img;
    IFHIP_load(img, &hip_img);

    IF_CHECK(IFHIP_loaded_grayScale(img, hip_img));

    IFHIP_retrieve(&hip_img, img);

    return IF_SUCCESS;
}

IF_error_t IFHIP_loaded_invert(const IF_image_t *img, IF_image_t *hip_img) {
    if(hip_img == NULL) {
        return IF_NULL_POINTER;
    }

    int w = img->width;
    int h = img->height;

    dim3 block(16, 16);
    dim3 grid((w + block.x - 1)/block.x,
              (h + block.y - 1)/block.y);

    hipLaunchKernelGGL(IFHIPK_invert, grid, block, 0, 0, hip_img);
    IF_HIP_KERNEL_CHECK();

    IF_HIP_CHECK(hipDeviceSynchronize());

    return IF_SUCCESS;
}

IF_error_t IFHIP_invert(IF_image_t *img) {
    IF_image_t *hip_img;
    IFHIP_load(img, &hip_img);

    IF_CHECK(IFHIP_loaded_invert(img, hip_img));

    IFHIP_retrieve(&hip_img, img);

    return IF_SUCCESS;
}

IF_error_t IFHIP_loaded_brightness(const IF_image_t *img, IF_image_t *hip_img, float factor) {
    if(hip_img == NULL) {
        return IF_NULL_POINTER;
    }

    int w = img->width;
    int h = img->height;

    dim3 block(16, 16);
    dim3 grid((w + block.x - 1)/block.x,
              (h + block.y - 1)/block.y);

    hipLaunchKernelGGL(IFHIPK_brightness, grid, block, 0, 0, hip_img, factor);
    IF_HIP_KERNEL_CHECK();

    IF_HIP_CHECK(hipDeviceSynchronize());

    return IF_SUCCESS;
}

IF_error_t IFHIP_brightness(IF_image_t *img, float factor) {
    IF_image_t *hip_img;
    IFHIP_load(img, &hip_img);

    IF_CHECK(IFHIP_loaded_brightness(img, hip_img, factor));

    IFHIP_retrieve(&hip_img, img);

    return IF_SUCCESS;
}
