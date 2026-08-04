/**
 * @file sched_constructor.h
 * @brief Self-registration macro for scheduler implementations.
 *
 * Mirrors the IF_OP_IMPL registration pattern used for operations and
 * devices elsewhere in ImageFlow. IF_SCHED_IMPL(name):
 *
 *   1. Forward-declares a static `IF_schedImpl_##name` function matching
 *      IF_SchedImpl_t's signature.
 *   2. Defines an IF_CONSTRUCTOR that runs before main() and installs
 *      that function into `IF_sched_impls[IF_SCHEDULER_##name]`.
 *   3. Opens the definition of `IF_schedImpl_##name`, leaving the caller
 *      to supply the function body (`{ ... }`).
 *
 * Usage:
 * @code
 * IF_SCHED_IMPL(MY_SCHED)
 * {
 *     return IF_my_sched_execute(flow, img_in, img_out);
 * }
 * @endcode
 *
 * @note `name` must correspond to an existing IF_SCHEDULER_##name
 *       enumerator — see schedulers.def. Adding a scheduler here without
 *       a matching entry in schedulers.def will fail to compile
 *       (undefined `IF_SCHEDULER_##name`); adding it to schedulers.def
 *       without a matching IF_SCHED_IMPL leaves that dispatch table slot
 *       NULL, which will crash on dispatch.
 *
 * @author Daniele Savino <daniele.savino0@gmail.com>
 *
 * SPDX-License-Identifier: MIT
 * Copyright (c) 2026 Daniele Savino
 */
#pragma once

#include "ImageFlow/constructor.h"
#include "ImageFlow/scheduler/scheduler.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Declares, self-registers, and opens the definition of a
 *        scheduler's execute implementation.
 *
 * Expands to a forward declaration, an IF_CONSTRUCTOR that registers the
 * function into IF_sched_impls[], and the opening signature of the
 * function itself — the caller supplies the trailing `{ ... }` body.
 *
 * @param sched Bare scheduler identifier (e.g. `CPU`), matching the
 *              `name` used in schedulers.def and pasted into both
 *              `IF_SCHEDULER_##sched` and the generated function name.
 */
#define IF_SCHED_IMPL(sched)                                                 \
static IF_error_t IF_schedImpl_##sched(IF_Flow_t flow,                       \
                                        const IF_image_t *img_in,            \
                                        IF_image_t *img_out);                \
                                                                              \
IF_CONSTRUCTOR(IF_register_sched_##sched)                                    \
{                                                                            \
    IF_sched_impls[IF_SCHEDULER_##sched] = IF_schedImpl_##sched;             \
}                                                                            \
                                                                              \
static IF_error_t IF_schedImpl_##sched(IF_Flow_t flow,                       \
                                        const IF_image_t *img_in,            \
                                        IF_image_t *img_out)

#ifdef __cplusplus
}
#endif
