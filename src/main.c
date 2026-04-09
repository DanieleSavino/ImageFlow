#include "ImageFlow/error.h"
#include "ImageFlow/io/image.h"
#include "ImageFlow/accelerated/acc_wrapper.h"
#include "ImageFlow/pipeline.h"
#include "ImageFlow/scheduler/scheduler.h"

int main(void)
{
    IF_image_t img, img_out;
    IF_CHECK(IF_loadImage(&img, "img.jpg"));

    IF_Flow_t flow;
    IF_flow_init(&flow);

    IF_flow_invert(flow);
    IF_flow_grayscale(flow);
    IF_flow_brightness(flow, 0.5);

    IF_CHECK(IF_flow_run(flow, &img, &img_out));

    IF_CHECK(IF_storeImage(&img_out, "out.jpg"));

    IF_CHECK(IF_flow_free(flow));
    IF_CHECK(IF_freeImage(&img));
    IF_CHECK(IF_freeImage(&img_out));
}
