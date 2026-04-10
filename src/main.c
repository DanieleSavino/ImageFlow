#include "ImageFlow/error.h"
#include "ImageFlow/io/image.h"
#include "ImageFlow/pipeline.h"
#include "ImageFlow/scheduler/scheduler.h"
#include <time.h>
#include <stdio.h>

#define TIME_BLOCK(name, code) do {                          \
    struct timespec _start, _end;                            \
    clock_gettime(CLOCK_MONOTONIC, &_start);                 \
                                                            \
    code                                                     \
                                                            \
    clock_gettime(CLOCK_MONOTONIC, &_end);                   \
    double _elapsed = (_end.tv_sec - _start.tv_sec) +        \
                      (_end.tv_nsec - _start.tv_nsec)/1e9;   \
    printf("[%s] %f s\n", name, _elapsed);                   \
} while(0)

int main(void)
{
    IF_image_t img, img_out;
    IF_CHECK(IF_loadImage(&img, "img.jpg"));

    IF_Flow_t flow;
    IF_flow_init(&flow);

    IF_flow_invert(flow);
    IF_flow_grayscale(flow);
    IF_flow_brightness(flow, 0.5);

    IF_CHECK(IF_flow_run_sched(flow, &img, IF_SCHEDULER_LINEAR, &img_out));

    TIME_BLOCK(
        "GPU",
        IF_CHECK(IF_flow_run(flow, &img, &img_out));
    );

    IF_CHECK(IF_storeImage(&img_out, "out_gpu.jpg"));

    TIME_BLOCK(
        "CPU",
        IF_CHECK(IF_flow_run_sched(flow, &img, IF_SCHEDULER_CPU, &img_out));
    );

    IF_CHECK(IF_storeImage(&img_out, "out_cpu.jpg"));

    IF_CHECK(IF_flow_free(flow));
    IF_CHECK(IF_freeImage(&img));
    IF_CHECK(IF_freeImage(&img_out));
}
