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
#include "ImageFlow/operations/operations.h"
#include "ImageFlow/pipeline.h"
#include "ImageFlow/scheduler/linear.h"
#include "ImageFlow/scheduler/scheduler.h"
#include "ImageFlow/vars.h"

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
 * Primary key  : op-type priority (METADATA=0, POINT=1, STENCIL=2,
 *                REDUCTION=3, MORPH=4).
 * Secondary key: preferred device, with device ordering reversed on
 *                odd-priority tiers.
 *
 * The reversal makes the device at the right edge of one tier match the
 * device at the left edge of the next tier, minimizing transitions at
 * every boundary — generalizing the 2-device flip trick to N devices by
 * reversing the whole device ordering instead of a single bit.
 *
 * Example (2 devices, GPU=0 CPU=1): stencil(pri2,even) sorts GPU,CPU;
 * morph(pri3,odd) sorts reversed, i.e. CPU,GPU — giving
 * stencil_gpu, stencil_cpu, morph_cpu, morph_gpu, exactly matching at
 * the stencil/morph boundary (both CPU).
 */
static int _sort_key(const IF_Operation_t *op) {
    int pri = _priority(IF_op_type(op->supp_op));
    int dev = (int)op->pref_dev;
    int dev_ordered = (pri % 2 == 0) ? dev : (_IF_DEV_LEN - 1 - dev);
    return pri * _IF_DEV_LEN + dev_ordered;
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
        int hit_barrier = (i == n) || _is_hard_barrier(IF_op_type(buf[i].supp_op), aggression);

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

#define _DEFAULT_AGGRESSION IF_REORDER_SAFE

IF_ReorderLevel_t IF_getenv_aggression() {
    const char *env_sched = getenv(IF_AGGRESSION_PARAM);

    if(env_sched == NULL) return _DEFAULT_AGGRESSION;

    if(!strcmp(env_sched, "O0") || !strcmp(env_sched, "SAFE"))
        return IF_REORDER_SAFE;
    if(!strcmp(env_sched, "O1") || !strcmp(env_sched, "STENCIL"))
        return IF_REORDER_STENCIL;
    if(!strcmp(env_sched, "O2") || !strcmp(env_sched, "REDUCTION"))
        return IF_REORDER_REDUCTION;
    if(!strcmp(env_sched, "O3") || !strcmp(env_sched, "MORPH"))
        return IF_REORDER_MORPH;

    return _DEFAULT_AGGRESSION;
}

NODISCARD IF_error_t IF_reorder_execute(IF_Flow_t flow, const IF_image_t *img_in, IF_image_t *img_out) {
    IF_CHECK(IF_reorder(flow, IF_getenv_aggression()));
    IF_CHECK(IF_flow_run_sched(flow, img_in, IF_SCHEDULER_LINEAR, img_out));

    return IF_SUCCESS;
}
