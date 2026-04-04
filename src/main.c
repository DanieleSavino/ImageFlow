#include "ImageFlow/error.h"
#include "ImageFlow/io/image.h"
#include "ImageFlow/operations.h"

int main(void)
{
    IF_image_t img;
    IF_CHECK(IF_loadImage(&img, "img.jpg"));

    IF_CHECK(IF_grayScale(&img));

    IF_CHECK(IF_storeImage(&img, "img_gs.jpg"));
}
