/**
 * @file linear.h
 * @brief Naive sequential scheduler implementation for ImageFlow.
 *
 * Executes an IF_Pipeline_t operation-by-operation in pipeline order.
 * Before execution, runs an execution_plan pass that downgrades all
 * operations to IF_DEV_CPU if no GPU backend is available.
 *
 * Device transfers are managed lazily at CPU<->GPU boundary crossings:
 * - CPU -> GPU: IFACC_load is called before the first GPU operation.
 * - GPU -> CPU: IFACC_retrieve is called before the first CPU operation
 *   following a GPU sequence, and at the end of the pipeline if the
 *   last operation ran on GPU.
 *
 * @note The initial IF_copyImage call produces an unnecessary full copy
 *       when the first operation could be run non-in-place. This is a
 *       known limitation of the current implementation.
 *
 * @warning This scheduler is a naive reference implementation intended
 *          to validate the pipeline and dispatch infrastructure.
 *          It is not optimized for performance.
 *
 * @author Daniele Savino <daniele.savino0@gmail.com>
 *
 * SPDX-License-Identifier: MIT
 * Copyright (c) 2026 Daniele Savino
 */
#pragma once

#include "ImageFlow/error.h"
#include "ImageFlow/pipeline.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Executes a pipeline sequentially against an image.
 *
 * Copies @p img_in into @p img_out, then iterates over each operation
 * in @p pipe in order, managing device transfers at CPU<->GPU boundaries.
 *
 * @param pipe    Pipeline to execute. Must not be NULL and must have a
 *                valid buffer.
 * @param img_in  Source image. Must not be NULL. Not modified.
 * @param img_out Output image. Must not be NULL. Receives the result.
 *                Any pre-existing data pointer will be overwritten without
 *                being freed.
 * @return IF_SUCCESS on success.
 * @return IF_NULL_POINTER if any pointer argument is NULL.
 * @return Any error code propagated from IF_copyImage, IFACC_load,
 *         IFACC_retrieve, or IF_op_execute.
 */
NODISCARD IF_error_t IF_linear_execute(IF_Flow_t flow, const IF_image_t *img_in, IF_image_t *img_out);

#ifdef __cplusplus
}
#endif
