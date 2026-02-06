#include "ImageFlow/accelerated/acc_wrapper.h"
#include "ImageFlow/error/error.h"
#include "ImageFlow/io/image.h"
#include "ImageFlow/omp/backend.h"
#include <stdlib.h>
#include <stdio.h>
#include <time.h>

static double timespec_diff(struct timespec *start, struct timespec *end) {
    return (end->tv_sec - start->tv_sec) + 
           (end->tv_nsec - start->tv_nsec) / 1e9;
}

int main(void)
{
    struct timespec start, end;
    double elapsed;
    
    IF_image_t *img = malloc(sizeof(IF_image_t));
    if(!img) {
        IF_logError(stderr, IF_OUT_OF_MEMORY);
        return 1;
    }
    
    // Load image
    printf("Loading image...\n");
    IF_CHECK(IF_loadImage(img, "img.jpg"));
    printf("Image loaded: %dx%d, %d channels\n", img->width, img->height, img->channels);
    
    // OpenMP grayscale
    printf("\n=== OpenMP Grayscale ===\n");
    clock_gettime(CLOCK_MONOTONIC, &start);
    IF_CHECK(IFOMP_grayScale(img));
    IF_CHECK(IFOMP_brightness(img, 0.3));
    IF_CHECK(IFOMP_invert(img));
    clock_gettime(CLOCK_MONOTONIC, &end);
    elapsed = timespec_diff(&start, &end);
    printf("OpenMP Time: %.6f seconds (%.3f ms)\n", elapsed, elapsed * 1000.0);
    
    IF_CHECK(IF_storeImage(img, "img_omp.jpg"));
    printf("Saved to img_omp.jpg\n");
    
    // Reload image for GPU test
    printf("\nReloading image...\n");
    IF_CHECK(IF_loadImage(img, "img.jpg"));
    
    // GPU (CUDA/HIP) grayscale
    printf("\n=== GPU Accelerated Grayscale ===\n");
    IF_image_t *cuda_img;
    IF_CHECK(IFACC_load(img, &cuda_img));
    clock_gettime(CLOCK_MONOTONIC, &start);
    IF_CHECK(IFACC_loaded_grayScale(img, cuda_img));
    IF_CHECK(IFACC_loaded_brightness(img, cuda_img, 0.3));
    IF_CHECK(IFACC_loaded_invert(img, cuda_img));
    clock_gettime(CLOCK_MONOTONIC, &end);
    elapsed = timespec_diff(&start, &end);
    printf("GPU Time: %.6f seconds (%.3f ms)\n", elapsed, elapsed * 1000.0);

    IF_CHECK(IFACC_retrieve(&cuda_img, img));
    
    IF_CHECK(IF_storeImage(img, "img_cuda.jpg"));
    printf("Saved to img_cuda.jpg\n");
    
    // Cleanup
    IF_CHECK(IF_freeImage(img));
    free(img);
    
    printf("\nDone!\n");
    
    return 0;
}
