#include "ImageFlow/accelerated/cuda/backend.h"
#include <cuda.h>

#include "ImageFlow/error.h"
#include "ImageFlow/io/image.h"
#include <cstddef>

IF_error_t IFCU_getDevices(int *cuda_dev) {
    cudaGetDeviceCount(cuda_dev);
    return IF_SUCCESS;
}

static inline IF_error_t IFCU_checkImg(const IF_image_t *img) {
    if(img == NULL || img->data == NULL) {
        return IF_NULL_POINTER;
    }
    if(img->channels < 3) {
        return IF_INVALID_CHANNELS;
    }

    return IF_SUCCESS;
}

static __global__ void IFCUK_grayScale(IF_image_t *img) {
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

static __global__ void IFCUK_invert(IF_image_t *img) {
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

static __global__ void IFCUK_brightness(IF_image_t *img, float factor) {
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

IF_error_t IFCU_load(const IF_image_t *img, IF_image_t **cuda_img) {
    IF_CHECK(IFCU_checkImg(img));

    int w = img->width;
    int h = img->height;
    int c = img->channels;

    IF_CUDA_CHECK(cudaMalloc(cuda_img, sizeof(IF_image_t)));

    float *cuda_data;
    IF_CUDA_CHECK(cudaMalloc(&cuda_data, w * h * c * sizeof(float)));
    IF_CUDA_CHECK(cudaMemcpy(cuda_data, img->data, w * h * c * sizeof(float), cudaMemcpyHostToDevice));

    IF_image_t host_img = *img;
    host_img.data = cuda_data;
    IF_CUDA_CHECK(cudaMemcpy(*cuda_img, &host_img, sizeof(IF_image_t), cudaMemcpyHostToDevice));

    return IF_SUCCESS;
}

IF_error_t IFCU_retrieve(IF_image_t **cuda_img, IF_image_t *img_out) {
    IF_image_t *d_cuda_img = *cuda_img;

    IF_CUDA_CHECK(cudaMemcpy(img_out, d_cuda_img, sizeof(IF_image_t), cudaMemcpyDeviceToHost));

    int w = img_out->width;
    int h = img_out->height;
    int c = img_out->channels;

    float *img_data = (float*)malloc(w * h * c * sizeof(float));
    IF_CUDA_CHECK(cudaMemcpy(img_data, img_out->data, w * h * c * sizeof(float), cudaMemcpyDeviceToHost));

    IF_CUDA_CHECK(cudaFree(img_out->data));

    img_out->data = img_data;

    IF_CUDA_CHECK(cudaFree(d_cuda_img));
    *cuda_img = NULL;

    return IF_SUCCESS;
}

IF_error_t IFCU_loaded_grayScale(IF_image_t *img, IF_image_t *cuda_img) {
    if(cuda_img == NULL) {
        return IF_NULL_POINTER;
    }

    int w = img->width;
    int h = img->height;

    dim3 block(16, 16);
    dim3 grid((w + block.x - 1)/block.x,
            (h + block.y - 1)/block.y);

    IFCUK_grayScale<<<grid, block>>>(cuda_img);
    IF_CUDA_KERNEL_CHECK();

    IF_CUDA_CHECK(cudaDeviceSynchronize());

    return IF_SUCCESS;
}

IF_error_t IFCU_grayScale(IF_image_t *img) {
    IF_image_t *cuda_img;
    IF_CHECK(IFCU_load(img, &cuda_img));

    IF_CHECK(IFCU_loaded_grayScale(img, cuda_img));

    IF_CHECK(IFCU_retrieve(&cuda_img, img));

    return IF_SUCCESS;
}

IF_error_t IFCU_loaded_invert(IF_image_t *img, IF_image_t *cuda_img) {
    if(cuda_img == NULL) {
        return IF_NULL_POINTER;
    }

    int w = img->width;
    int h = img->height;

    dim3 block(16, 16);
    dim3 grid((w + block.x - 1)/block.x,
            (h + block.y - 1)/block.y);

    IFCUK_invert<<<grid, block>>>(cuda_img);
    IF_CUDA_KERNEL_CHECK();

    IF_CUDA_CHECK(cudaDeviceSynchronize());

    return IF_SUCCESS;
}

IF_error_t IFCU_invert(IF_image_t *img) {
    IF_image_t *cuda_img;
    IF_CHECK(IFCU_load(img, &cuda_img));

    IF_CHECK(IFCU_loaded_invert(img, cuda_img));

    IF_CHECK(IFCU_retrieve(&cuda_img, img));

    return IF_SUCCESS;
}

IF_error_t IFCU_loaded_brightness(IF_image_t *img, IF_image_t *cuda_img, float factor) {
    if(cuda_img == NULL) {
        return IF_NULL_POINTER;
    }

    int w = img->width;
    int h = img->height;

    dim3 block(16, 16);
    dim3 grid((w + block.x - 1)/block.x,
            (h + block.y - 1)/block.y);

    IFCUK_brightness<<<grid, block>>>(cuda_img, factor);
    IF_CUDA_KERNEL_CHECK();

    IF_CUDA_CHECK(cudaDeviceSynchronize());

    return IF_SUCCESS;
}

IF_error_t IFCU_brightness(IF_image_t *img, float factor) {
    IF_image_t *cuda_img;
    IF_CHECK(IFCU_load(img, &cuda_img));

    IF_CHECK(IFCU_loaded_brightness(img, cuda_img, factor));

    IF_CHECK(IFCU_retrieve(&cuda_img, img));

    return IF_SUCCESS;
}
