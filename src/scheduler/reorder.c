/**
 * @file reorder.c
 * @brief Pipeline reordering optimizer implementation.
 *
 * @author Daniele Savino <daniele.savino0@gmail.com>
 *
 * SPDX-License-Identifier: MIT
 * Copyright (c) 2026 Daniele Savino
 */
#include "ImageFlow/scheduler/reorder.h"
#include "ImageFlow/error.h"
#include "ImageFlow/operations.h"
#include "ImageFlow/pipeline.h"

#include <stdlib.h>
#include <string.h>

/* -------------------------------------------------------------------------
 * Internal helpers
 * ---------------------------------------------------------------------- */

/**
 * @brief Returns true if this op type is a hard barrier at the given aggression.
 *
 * A barrier is hard when POINT ops may not cross it.
 * METADATA is never a barrier (POINT commutes with dimension changes).
 * Each aggression level unlocks one additional barrier type for POINT crossing.
 */
static int _is_hard_barrier(IF_OpType_t t, IF_ReorderLevel_t aggression) {
    switch (t) {
        case IF_TRAVERSAL_METADATA:
            return 0;
        case IF_TRAVERSAL_POINT:
            return 0;
        case IF_TRAVERSAL_STENCIL:
            return aggression < IF_REORDER_STENCIL;
        case IF_TRAVERSAL_REDUCTION:
            return aggression < IF_REORDER_REDUCTION;
        case IF_TRAVERSAL_MORPH:
            return aggression < IF_REORDER_MORPH;
        default:
            return 1;
    }
}

/**
 * @brief Primary sort key: op-type priority within a segment.
 *
 * METADATA (0) before POINT (1) before unlocked barrier types (2).
 * In practice at SAFE level segments only contain METADATA and POINT.
 * At higher aggression levels a segment may also contain the unlocked
 * barrier types (STENCIL, REDUCTION, MORPH), which get priority 2 and
 * stay after POINT.
 */
static int _priority(IF_OpType_t t) {
    switch (t) {
        case IF_TRAVERSAL_METADATA:  return 0;
        case IF_TRAVERSAL_POINT:     return 1;
        default:                     return 2;
    }
}

/**
 * @brief Composite sort key for a pipeline operation.
 *
 * Primary key  : op-type priority (METADATA=0, POINT=1, other=2).
 * Secondary key: preferred device, with polarity flipped by priority parity.
 *
 * The parity flip ensures that at every priority tier boundary the same
 * device is attracted to both sides, minimising host<->device transitions:
 *
 *   even priority (METADATA=0, other=2): GPU=0, CPU=1  (GPU sorts first)
 *   odd  priority (POINT=1):             GPU=1, CPU=0  (CPU sorts first)
 *
 * Concretely, the right edge of an even tier and the left edge of the
 * following odd tier both carry the same device, and vice versa.
 *
 * Example — input: point/cpu, point/gpu, stencil/gpu, stencil/cpu
 *
 *   op           pri  dev  flipped  key
 *   point/cpu     1    1      0      2
 *   point/gpu     1    0      1      3
 *   stencil/gpu   2    0      0      4
 *   stencil/cpu   2    1      1      5
 *
 * Keys are already monotone so the order is unchanged and the
 * point/gpu -> stencil/gpu boundary is transition-free.
 */
static int _sort_key(const IF_Operation_t *op) {
    int pri         = _priority(op->op_type);
    int dev         = (op->pref_dev == IF_DEV_GPU) ? 0 : 1;
    int dev_flipped = (pri % 2 == 0) ? dev : (1 - dev);
    return pri * 2 + dev_flipped;
}

/**
 * @brief Stable insertion sort a subarray of IF_Operation_t by _sort_key.
 *
 * Insertion sort is O(n²) but segments are almost always short (< 16 ops),
 * so the constant factor beats qsort's overhead and the stability guarantee
 * is free.
 */
static void _sort_segment(IF_Operation_t *seg, size_t len) {
    for (size_t j = 1; j < len; j++) {
        IF_Operation_t key = seg[j];
        int key_k = _sort_key(&key);
        size_t k = j;
        while (k > 0 && _sort_key(&seg[k - 1]) > key_k) {
            seg[k] = seg[k - 1];
            k--;
        }
        seg[k] = key;
    }
}

/* -------------------------------------------------------------------------
 * IF_reorder
 * ---------------------------------------------------------------------- */

IF_error_t IF_reorder(IF_Flow_t flow, IF_ReorderLevel_t aggression) {
    if (!flow || !flow->buff) return IF_NULL_POINTER;
    if (aggression < IF_REORDER_SAFE || aggression > IF_REORDER_MORPH)
        return IF_INVALID_ARGS;
    if (flow->len == 0) return IF_SUCCESS;

    size_t n = flow->len;
    IF_Operation_t *buf = flow->buff;

    /*
     * Walk the pipeline collecting runs of non-hard-barrier ops into
     * segments, then stable-sort each segment by _sort_key.
     * Hard barrier ops themselves are left in place and not included
     * in any segment.
     */
    size_t seg_start = 0;

    for (size_t i = 0; i <= n; i++) {
        int hit_barrier = (i == n) || _is_hard_barrier(buf[i].op_type, aggression);

        if (hit_barrier) {
            size_t seg_len = i - seg_start;
            if (seg_len > 1) {
                _sort_segment(buf + seg_start, seg_len);
            }
            seg_start = i + 1;
        }
    }

    return IF_SUCCESS;
}
