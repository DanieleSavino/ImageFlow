#include "ImageFlow/error/error.h"
#include "ImageFlow/io/image.h"
#include "ImageFlow/omp/backend.h"
#include <stdlib.h>

int main(void)
{
    IF_image_t *img = malloc(sizeof(IF_image_t));
    if(!img) {
        IF_logError(stderr, IF_OUT_OF_MEMORY);
    }

    IF_CHECK(IF_loadImage(img, "img.jpg"));
    IF_CHECK(IFOMP_grayScale(img));
    IF_CHECK(IF_storeImage(img, "img_store.jpg"));

    IF_CHECK(IF_freeImage(img));
    free(img);
}
