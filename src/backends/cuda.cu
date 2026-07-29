#include "ImageFlow/backends/cuda.h"

__global__ void IFCUK_grayScale(IF_image_t *img) {
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

__global__ void IFCUK_invert(IF_image_t *img) {
    int x = blockIdx.x * blockDim.x + threadIdx.x;
    int y = blockIdx.y * blockDim.y + threadIdx.y;

    int w = img->width;
    int h = img->height;
    int c = img->channels;

    if (x >= w || y >= h) return;

    int idx = (y * w + x) * c;
    float *pixel = img->data + idx;

    pixel[0] = 1 / pixel[0];
    pixel[1] = 1 / pixel[1];
    pixel[2] = 1 / pixel[2];
}

__global__ void IFCUK_brightness(IF_image_t *img, float factor) {
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
