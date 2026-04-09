/**
 * @file scheduler.h
 * @brief Pipeline scheduler interface for ImageFlow.
 *
 * Provides IF_flow_run and IF_flow_run_sched, the top-level entry points
 * for executing an IF_Flow_t against an image.
 *
 * The scheduler is responsible for:
 * - copying the input image to produce a mutable working buffer
 * - probing GPU availability and falling back to CPU if needed (execution_plan)
 * - managing H2D and D2H transfers at device boundary crossings between operations
 * - invoking IF_op_execute for each operation in pipeline order
 *
 * This is where the double-wrapper design (IF_op_execute -> IF_grayscale/etc ->
 * IFACC_* / IFOMP_*) pays off: the scheduler only calls IF_op_execute and lets
 * the per-op functions own backend selection, keeping scheduling logic
 * decoupled from execution details.
 *
 * IF_flow_run defaults to IF_SCHEDULER_LINEAR. Additional schedulers can be
 * added to IF_Scheduler_t and handled in IF_flow_run_sched without changing
 * the public API.
 *
 * @author Daniele Savino <daniele.savino0@gmail.com>
 *
 * SPDX-License-Identifier: MIT
 * Copyright (c) 2026 Daniele Savino
 */
#pragma once

#include "ImageFlow/error.h"
#include "ImageFlow/io/image.h"
#include "ImageFlow/pipeline.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Validates the three mandatory parameters shared by all scheduler entry points.
 *
 * Returns IF_NULL_POINTER from the enclosing function if any of the three
 * pointers is NULL.
 *
 * @param flow    IF_Flow_t pipeline handle.
 * @param img_in  Input image pointer.
 * @param img_out Output image pointer.
 */
#define IF_CHECK_SCHED_PARAMS(flow, img_in, img_out)        \
    do {                                                     \
        if(flow == NULL || img_in == NULL || img_out == NULL)\
            return IF_NULL_POINTER;                          \
    } while(0)

/**
 * @brief Identifies the scheduling strategy used to execute a pipeline.
 */
typedef enum {
    IF_SCHEDULER_LINEAR /**< Naive sequential scheduler. Executes operations in
                             pipeline order, managing device transfers at
                             CPU<->GPU boundary crossings. */
} IF_Scheduler_t;

/**
 * @brief Executes a pipeline against an input image using the specified scheduler.
 *
 * Copies @p img_in into @p img_out, then runs each operation in the pipeline
 * in order. Device transfers (H2D / D2H) are issued automatically at
 * CPU<->GPU boundary crossings between consecutive operations.
 *
 * @param flow    Pipeline to execute. Must not be NULL.
 * @param img_in  Source image. Must not be NULL. Not modified.
 * @param sched   Scheduler strategy to use.
 * @param img_out Output image. Must not be NULL. Receives the result.
 *                Any pre-existing data pointer will be overwritten without
 *                being freed — caller must ensure @p img_out is uninitialized
 *                or already freed.
 * @return IF_SUCCESS on success.
 * @return IF_NULL_POINTER if any pointer argument is NULL.
 * @return IF_INVALID_ARGS if @p sched is not a recognized scheduler.
 * @return Any error code propagated from the scheduler or operations.
 */
NODISCARD IF_error_t IF_flow_run_sched(IF_Flow_t flow, const IF_image_t *img_in, IF_Scheduler_t sched, IF_image_t *img_out);

/**
 * @brief Executes a pipeline using the default scheduler (IF_SCHEDULER_LINEAR).
 *
 * Convenience wrapper around IF_flow_run_sched.
 *
 * @param flow    Pipeline to execute. Must not be NULL.
 * @param img_in  Source image. Must not be NULL. Not modified.
 * @param img_out Output image. Must not be NULL. Receives the result.
 * @return IF_SUCCESS on success.
 * @return Any error code propagated from IF_flow_run_sched.
 */
NODISCARD IF_error_t IF_flow_run(IF_Flow_t flow, const IF_image_t *img_in, IF_image_t *img_out);

#ifdef __cplusplus
}
#endif
