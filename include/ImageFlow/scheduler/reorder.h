/**
 * @file reorder.h
 * @brief Pipeline reordering optimizer for ImageFlow.
 *
 * Reorders ops within an IF_Flow_t to minimize total pixel work and
 * host<->device transfer overhead, with an aggression level controlling
 * the correctness/performance tradeoff.
 *
 * Correctness model
 * -----------------
 * Ops are classified by their IF_OpType_t:
 *
 *   METADATA   — changes image dimensions (e.g. resize). Moving earlier
 *                means all subsequent ops process fewer pixels. Reordering
 *                METADATA with POINT is always safe: point ops are 1:1
 *                per-pixel and commute with dimension changes.
 *
 *   POINT      — per-pixel, 1:1, no neighborhood read. Commutes freely
 *                with other POINT and with METADATA.
 *
 *   STENCIL    — reads a pixel neighborhood (e.g. blur, Sobel). Does NOT
 *                commute with POINT in general: brightness-then-blur ≠
 *                blur-then-brightness. Crossing a POINT past a STENCIL
 *                produces different (though often perceptually acceptable)
 *                results. Rationale for allowing it at higher aggression:
 *                consecutive POINT ops can be fused into a single kernel
 *                pass; grouping a POINT next to other POINTs and away from
 *                STENCILs enables that fusion.
 *
 *   REDUCTION  — reads the entire image (e.g. min, max, histogram).
 *                Similar non-commutativity with POINT; higher risk than
 *                STENCIL because the scalar result changes.
 *
 *   MORPH      — geometric transform (e.g. rotate). Hardest barrier:
 *                moving a POINT past a MORPH changes which pixels get
 *                transformed, not just their values.
 *
 * Aggression levels
 * -----------------
 * IF_REORDER_SAFE (0)
 *   Reorder METADATA and POINT freely among themselves.
 *   STENCIL, REDUCTION, MORPH are hard barriers.
 *   No correctness risk.
 *
 * IF_REORDER_STENCIL (1)
 *   Additionally allows POINT ops to cross STENCIL barriers.
 *   Rationale: enables point-kernel fusion across stencil boundaries.
 *   Risk: floating-point results differ from sequential order.
 *
 * IF_REORDER_REDUCTION (2)
 *   Additionally allows POINT ops to cross REDUCTION barriers.
 *   Risk: scalar reduction results change (e.g. max-after-brightness ≠
 *   max-before-brightness).
 *
 * IF_REORDER_MORPH (3)
 *   Additionally allows POINT ops to cross MORPH barriers.
 *   Highest risk: which pixels are transformed changes.
 *   Only appropriate when point ops are pure value transforms with no
 *   spatial meaning (e.g. color space conversion before a rotate).
 *
 * Algorithm
 * ---------
 * The pipeline is walked once, collecting runs of non-barrier ops into
 * segments. Each segment is stable-sorted by a composite key:
 *
 *   primary   : op-type priority — METADATA(0) < POINT(1) < other(2)
 *   secondary : preferred device, with polarity flipped by priority parity
 *
 * The parity flip on the device key ensures that at every tier boundary
 * the same device is attracted to both sides, minimising host<->device
 * transitions without any extra allocation or candidate enumeration.
 * Hard barrier ops are left in place and never included in a segment.
 * The sort is stable within each class, preserving relative order of ops
 * of the same type and device.
 *
 * @author Daniele Savino <daniele.savino0@gmail.com>
 *
 * SPDX-License-Identifier: MIT
 * Copyright (c) 2026 Daniele Savino
 */
#pragma once

#include "ImageFlow/error.h"
#include "ImageFlow/pipeline.h"
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Aggression level for IF_reorder.
 *
 * Higher levels allow more reordering at the cost of result fidelity.
 * See header documentation for a full description of each level.
 */
typedef enum {
    IF_REORDER_SAFE      = 0, /**< Reorder METADATA/POINT only. No risk. */
    IF_REORDER_STENCIL   = 1, /**< POINT may cross STENCIL barriers.    */
    IF_REORDER_REDUCTION = 2, /**< POINT may cross REDUCTION barriers.  */
    IF_REORDER_MORPH     = 3  /**< POINT may cross MORPH barriers.      */
} IF_ReorderLevel_t;

/**
 * @brief Reorders ops in @p flow in-place to minimize pixel work and
 *        host<->device transfer overhead.
 *
 * See header-level documentation for the correctness model and algorithm.
 *
 * @param flow       Pipeline to reorder. Modified in place.
 * @param aggression Reordering aggression level (IF_ReorderLevel_t).
 * @return IF_SUCCESS on success.
 * @return IF_NULL_POINTER if flow or its buffer is NULL.
 * @return IF_INVALID_ARGS if aggression is out of range.
 */
NODISCARD IF_error_t IF_reorder(IF_Flow_t flow, IF_ReorderLevel_t aggression);

#ifdef __cplusplus
}
#endif
