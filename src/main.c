#include "ImageFlow/error/error.h"
#include "ImageFlow/io/image_io.h"
#include <stdlib.h>

int main(void)
{
    IF_image_t *img = malloc(sizeof(IF_image_t));
    if(!img) {
        IF_logError(stderr, IF_OUT_OF_MEMORY);
    }

    IF_CHECK(IF_loadImage(img, "img.png"));
    IF_CHECK(IF_storeImage(img, "img_store.png"));

    IF_CHECK(IF_freeImage(img));
    free(img);
}
