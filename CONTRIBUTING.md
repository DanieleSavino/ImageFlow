# Contributing to ImageFlow

The dispatch layer for devices, operations, and schedulers is table-driven, so adding any of the three usually means adding a `.def` entry plus one self-registering implementation file — no changes to the scheduler, pipeline, or dispatch code required. Registering a device or an operation also gets you generated test coverage for free — see [Adding a test](#adding-a-test).

## Adding a device

1. **Register it in `include/ImageFlow/devices/devices.def`**:

   ```c
   IF_DEV_DEF(CPU)

   // Gpus
   IF_DEV_DEF(CUDA)
   IF_DEV_DEF(HIP)
   ```

   Add a new `IF_DEV_DEF(YOUR_DEV)` line. This drives the `IF_DevType_t` enum, `IF_strdev`, and the size of every per-device dispatch table — nothing else needs to know the device exists. It also drives `tests/src/devs.c`: as soon as the line is added, `devs/YOUR_DEV/round_trip` and `devs/YOUR_DEV/double_free` tests exist and will run (skipped, until step 2 below makes the device report itself enabled). See [Adding a test](#adding-a-test).

2. **Implement the backend.** Create `include/ImageFlow/backends/your_dev.h` (+ a matching `src/backends/your_dev.{c,cpp,cu}` that just `#include`s it, following the existing backends) and provide:

   - **A runtime availability check**, registered via `IF_CONSTRUCTOR` so it runs before `main()` and calls `IF_enable_device` / `IF_disable_device`.
   - **A load-image implementation**, via `IF_LOAD_IMG_IMPL(YOUR_DEV) { ... }` — moves a host image onto the device, populating `imgs[IF_DEV_YOUR_DEV]`.
   - **A retrieve-image implementation**, via `IF_RETRIEVE_IMG_IMPL(YOUR_DEV) { ... }` — moves `imgs[IF_DEV_YOUR_DEV]` back into the host image.
   - **A free-image implementation**, via `IF_FREE_IMG_IMPL(YOUR_DEV) { ... }` — releases whatever the device-side allocation is and clears the slot back to `NULL`. Should return `IF_NULL_POINTER` (not crash) if the slot is already `NULL` — the generated `devs/YOUR_DEV/double_free` test exercises exactly this by freeing twice in a row.
   - **An implementation of every operation** you want available on this device (see [Adding an operation](#adding-an-operation) below) — a device with no registered op implementations is still a valid device, it just falls back to CPU for everything.

   All four of `IF_CONSTRUCTOR`, `IF_LOAD_IMG_IMPL`, `IF_RETRIEVE_IMG_IMPL`, and `IF_FREE_IMG_IMPL` self-register into their respective dispatch tables at load time — you only need to write the function bodies.

   ### Example: the CUDA backend

   `include/ImageFlow/backends/cuda.h` is the fullest reference implementation and worth reading end to end. The pieces it contributes, in order:

   - `IF_register_cuda_availability`, an `IF_CONSTRUCTOR` that calls `cudaGetDeviceCount` and enables/disables `IF_DEV_CUDA` accordingly:

     ```c
     IF_CONSTRUCTOR(IF_register_cuda_availability) {
         int count = 0;
         IF_error_t err = IFCU_getDevices(&count);
         if (err == IF_SUCCESS && count > 0) {
             IF_enable_device(IF_DEV_CUDA);
         } else {
             IF_disable_device(IF_DEV_CUDA);
         }
     }
     ```

   - `IF_LOAD_IMG_IMPL(CUDA)`, which allocates the device-side image on first use and reuses the allocation on subsequent calls, copying fresh host pixel data down each time.
   - `IF_RETRIEVE_IMG_IMPL(CUDA)`, which copies device pixel data back into the host `IF_image_t`.
   - `IF_FREE_IMG_IMPL(CUDA)`, which frees the device buffer and struct, returns `IF_NULL_POINTER` if the slot is already `NULL`, and resets `imgs[IF_DEV_CUDA]` to `NULL`.
   - One `IF_OP_IMPL(CUDA, OP, flops_per_pixel, bytes_per_pixel) { ... }` per operation (`GRAYSCALE`, `INVERT`, `BRIGHTNESS`), each launching a `__global__` kernel over the device image and synchronizing before returning.

   A new backend should mirror this shape: availability check → load/retrieve/free → one `IF_OP_IMPL` per supported operation.

3. **Wire it into `CMakeLists.txt`** so its source file(s) get compiled and linked only when the relevant toolchain (CUDA/HIP/etc.) is actually available, following the existing `HAVE_CUDA` / `HAVE_HIP` pattern.

4. **Build and run the generated tests** to confirm the backend behaves correctly — see [Adding a test](#adding-a-test) for how. No test code needs to be written for a new device beyond what `devs.c` already generates, unless you want additional device-specific coverage.

## Adding an operation

1. **Register it in `include/ImageFlow/operations/operations.def`**:

   ```c
   IF_OP_DEF(GRAYSCALE,  Grayscale,  POINT, EMPTY,        )
   IF_OP_DEF(INVERT,     Invert,     POINT, EMPTY,        )
   IF_OP_DEF(BRIGHTNESS, Brightness, POINT, FLOAT_FACTOR, float factor = 1.5f)
   ```

   Each line is `IF_OP_DEF(id, name, traversal_type, args, def)`:

   - `id` — the bare identifier pasted into `IF_OP_##id` (e.g. `IF_OP_GRAYSCALE`).
   - `name` — used for the generated pipeline-builder function (`IF_flow_##name`, e.g. `IF_flow_Grayscale`) and for `IF_strop`.
   - `traversal_type` — one of `METADATA`, `POINT`, `STENCIL`, `REDUCTION`, `MORPH`. This characterizes the operation's memory access pattern and controls how aggressively the reorder scheduler is allowed to move it across device boundaries.
   - `args` — the suffix of the `IF_OP_ARGS_*` / `IF_OP_INIT_*` macros for this operation's parameters (see below). Use `EMPTY` if the operation takes no parameters.
   - `def` — a declaration (or nothing, for `EMPTY` ops) providing the default value(s) that `IF_OP_INIT_<argtype>` reads when building this op's `default_<id>_args()`. Leave it blank (just a trailing comma) for `EMPTY` ops; for parameterized ops it should declare exactly the variable(s) your `IF_OP_INIT_<argtype>` macro references — e.g. `float factor = 1.5f` for `FLOAT_FACTOR`.

   Adding this line is enough to get `IF_flow_{name}(flow, ...)` generated automatically for you — you don't write the pipeline builder function yourself. It also drives `include/ImageFlow/operations/op_args.h`, which generates a `default_<id>_args()` helper per op using each op's own `def`, and `tests/src/ops.c`, which generates an `ops/<name>/check_against_cpu` test that exercises the new op on every registered device once you complete step 3. See [Adding a test](#adding-a-test).

2. **If the operation needs parameters**, extend `include/ImageFlow/operations/op_args.h`:

   ```c
   /**
    * @brief Tagged union of operation-specific arguments.
    *
    * Extend this union when adding new operations that require parameters.
    * TODO: Clean this
    *
    * @warning Adding pointer members here would complicate any future
    *          distributed (MPI) execution model significantly.
    *          Plus MPI datatypes would be a pain.
    */
   typedef union {
       struct { char _unused; } empty;        /**< Placeholder for zero-argument operations. */
       struct { float factor; } float_factor; /**< Single float parameter (e.g. brightness factor). */
   } IF_OpArgs_t;

   #define IF_OP_ARGS_EMPTY
   #define IF_OP_ARGS_FLOAT_FACTOR , float factor

   #define IF_OP_INIT_EMPTY \
       (IF_OpArgs_t){ .empty = {} }

   #define IF_OP_INIT_FLOAT_FACTOR \
       (IF_OpArgs_t){ .float_factor = { .factor = factor } }
   ```

   For a new arg shape `YOUR_ARGS`, add:

   - a new member to the `IF_OpArgs_t` union holding whatever fields you need,
   - `#define IF_OP_ARGS_YOUR_ARGS , <parameter list>` — appended after `flow` in the generated builder's signature,
   - `#define IF_OP_INIT_YOUR_ARGS (IF_OpArgs_t){ .your_member = { ... } }` — how the builder packs its parameters into the union, reading whatever variable(s) your op's `def` field declares.

   Keep members flat (no pointers) — see the warning in `op_args.h` about why pointer members would complicate any future MPI/distributed execution model.

   `op_args.h` itself then X-macros over `operations.def` a second time to generate a `default_<id>_args()` for every registered op, splicing in each op's own `def` before packing it via `IF_OP_INIT_<argtype>`:

   ```c
   #define IF_OP_DEF(op, name, type, argtype, def) \
   static inline void default_##op##_args(IF_OpArgs_t *args) { \
       def; \
       *args = IF_OP_INIT_##argtype; \
   }
   #include "ImageFlow/operations/operations.def"
   #undef IF_OP_DEF
   ```

   Unlike an earlier version of this macro (which declared a single fixed `float factor = 1.5f` in scope for every op and silenced the resulting unused-variable warning with `(void)factor`), each op now supplies its own `def` directly in `operations.def` — so `EMPTY` ops declare nothing and `FLOAT_FACTOR` ops declare exactly the `factor` their `IF_OP_INIT_FLOAT_FACTOR` consumes. This is what `default_<id>_args()` uses to construct a representative `IF_OpArgs_t` for the generated `ops/<name>/check_against_cpu` test (see [Adding a test](#adding-a-test)) — so once your `IF_OP_INIT_YOUR_ARGS` exists and you've supplied a `def`, the test harness can build valid args for it automatically, with no extra step on your part.

   > **Note:** threading a raw declaration through `operations.def` as a bare macro argument is a bit of a blunt instrument — it works because `def` is always a single, simple declaration today, but it doesn't scale gracefully if an op ever needs a more elaborate default (multiple statements, a computed value, conditional logic, etc.). If operation argument signatures grow significantly more complex, this is worth revisiting — likely by splitting into `IF_OP_LOCALS_<argtype>`-style tables like `IF_OP_ARGS_*` and `IF_OP_INIT_*` above. For the current, fairly narrow set of argument shapes (`EMPTY`, `FLOAT_FACTOR`), it's not worth the extra indirection yet.

3. **Implement the operation for each device you want to support it on**, via `IF_OP_IMPL(dev, OP, flops_per_pixel, bytes_per_pixel) { ... }` in that device's backend file. Look at the CUDA implementations in `include/ImageFlow/backends/cuda.h` for the expected shape:

   ```c
   IF_OP_IMPL(CUDA, BRIGHTNESS, 1, 1) {
       IF_image_t *img = imgs[IF_DEV_CPU];
       IF_image_t *cuda_img = imgs[IF_DEV_CUDA];
       IF_CHECK(IFCU_checkImg(img));

       int w = img->width, h = img->height;
       dim3 block(16, 16);
       dim3 grid((w + block.x - 1) / block.x, (h + block.y - 1) / block.y);

       IFCUK_brightness<<<grid, block>>>(cuda_img, args.float_factor.factor);
       IF_CUDA_KERNEL_CHECK();
       IF_CUDA_CHECK(cudaDeviceSynchronize());
       return IF_SUCCESS;
   }
   ```

   Notes:

   - The `args` parameter is the `IF_OpArgs_t` you defined in step 2 — pull your fields out via the union member (`args.float_factor.factor` above).
   - `imgs` is the array of length `_IF_DEV_LEN` handed down by the scheduler; index it by whichever device slot(s) your implementation actually needs.
   - `flops_per_pixel` / `bytes_per_pixel` are cost-model metadata used for diagnostics (`IF_print_op_impls`) — pass reasonable estimates. These are also the fields the planned per-(op, device) loss estimate in [TODO.md](TODO.md) will consume.
   - An operation doesn't need an implementation on every device. If a device has no registered implementation for an operation, the scheduler falls back to CPU for that operation automatically, and the generated `check_against_cpu` test skips that (op, device) pair via `CHECK_IMPL` rather than failing.

That's it — no changes are needed to `pipeline.c`, `operations.c`, or any of the schedulers. The `.def` list is the single source of truth they all read from.

## Adding a scheduler

Schedulers follow the same registration-table pattern as devices and operations now — adding one means adding a `.def` entry plus one self-registering implementation, no changes to `scheduler.c`'s dispatch code or to `IF_flow_run_sched` required.

1. **Register it in `include/ImageFlow/scheduler/schedulers.def`**:

   ```c
   IF_SCHED_DEF(CPU)
   IF_SCHED_DEF(LINEAR)
   IF_SCHED_DEF(REORDER)
   ```

   Add a new `IF_SCHED_DEF(YOUR_SCHED)` line. This drives the `IF_Scheduler_t` enum, `IF_strsched`, and the size of `IF_sched_impls[]` — nothing else needs to know the scheduler exists.

2. **Implement it**, following the shape of `IF_cpu_execute` / `IF_linear_execute` / `IF_reorder_execute` — an `IF_error_t your_execute(IF_Flow_t flow, const IF_image_t *img_in, IF_image_t *img_out)` function matching `IF_SchedImpl_t`, declared in its own `scheduler/your_sched.h` and defined in `scheduler/your_sched.c`.

3. **Self-register it** via `IF_SCHED_IMPL(YOUR_SCHED) { ... }` in `your_sched.h`, following e.g. `linear.h`:

   ```c
   IF_SCHED_IMPL(LINEAR) {
       return IF_linear_execute(flow, img_in, img_out);
   }
   ```

   `IF_SCHED_IMPL` declares the implementation and self-registers it into `IF_sched_impls[IF_SCHEDULER_YOUR_SCHED]` via a load-time `IF_CONSTRUCTOR` (see `scheduler/sched_constructor.h`) — you only need to write the function body, matching the signature `(IF_Flow_t flow, const IF_image_t *img_in, IF_image_t *img_out)`.

   Most existing schedulers are thin: `IF_reorder_execute` mutates the flow in place (reordering ops for device-transfer locality via `IF_reorder`) and then delegates to `IF_flow_run_sched(flow, img_in, IF_SCHEDULER_LINEAR, img_out)` to actually run it. A new scheduler can follow the same pattern — do some pipeline-level transformation, then hand off to `IF_SCHEDULER_LINEAR` — or execute the flow directly, whichever fits.

4. **If it should be the default**, update `_DEFAULT_SCHED` at the top of `scheduler.c` (currently `IF_SCHEDULER_REORDER`). This is the fallback `IF_getenv_sched()` returns when the `IF_SCHED` environment variable is unset or doesn't match a registered scheduler name, and it's therefore what `IF_flow_run` (the no-scheduler-argument entry point) uses by default.

5. **Add tests by hand.** Unlike devices and operations, schedulers aren't currently X-macro'd into `tests/src/`, so there's no automatic `IF_SCHED_DEF` → generated test yet — write one under `tests/src/` following the pattern in [Adding a test](#adding-a-test). Scheduler-correctness tests (e.g. validating that a new scheduler produces the same output as `IF_SCHEDULER_LINEAR`) are also tracked in [TODO.md](TODO.md).

That's it — no changes are needed to `scheduler.c`'s dispatch table lookup, `IF_flow_run_sched`, or any hand-written switch statement. The `.def` list is the single source of truth all three registration tables (devices, operations, schedulers) read from.

## Adding a test

Tests live under `tests/src/*.c` and are picked up automatically — `tests/CMakeLists.txt` globs that directory and compiles whatever it finds into the `imageflow_tests_static` target, so a new `.c` file dropped in there is enough. No `CMakeLists.txt` edits required.

Tests are written using [fastest](https://github.com/danielesavino/fastest) (vendored as a submodule at `tests/vendor/fastest`; `git submodule update --init --recursive` before your first build). Two headers under `tests/include/IF_tests/` exist specifically to make writing ImageFlow tests against fastest less verbose:

- `IF_tests/error.h` gives you `IF_FASTEST_CHECK(expr)` — wrap any `IF_error_t`-returning ImageFlow call in it and it will fail the enclosing fastest test (with file/line-annotated logging) on error, instead of you having to unwrap `IF_error_t` by hand.
- `IF_tests/utils.h` gives you `PROBE_DEV(out, dev)` (skip — not fail — if `dev` isn't enabled at runtime) and `CHECK_IMPL(impl, dev)` (skip if an op has no implementation for `dev`).

### You probably don't need to write devs/ops tests by hand

`tests/src/devs.c` and `tests/src/ops.c` already X-macro over `devices.def` and `operations.def` respectively, so simply adding a `IF_DEV_DEF(...)` or `IF_OP_DEF(...)` line (see [Adding a device](#adding-a-device) / [Adding an operation](#adding-an-operation) above) is enough to get:

- `devs/<name>/round_trip` and `devs/<name>/double_free`, per device
- `ops/<name>/check_against_cpu`, per operation, run against every enabled device

with no test code of your own. Write a new file under `tests/src/` when you need coverage that doesn't fit this per-device/per-op shape — e.g. pipeline-level behavior, a specific scheduler, or a regression test for a bug that isn't captured by the generic round-trip/compare-to-CPU pattern.

### Building and running

```bash
cmake -B build/debug -DCMAKE_BUILD_TYPE=Debug
cmake --build build/debug --target imageflow_static imageflow_tests_static

./scripts/build_wheel.sh debug
python3 tests/main.py
```

`imageflow_tests_static` only compiles your test sources into a static archive — it isn't a runnable binary by itself. `scripts/build_wheel.sh` links `libImageFlow_tests.a` together with `libImageFlow.a` into a pybind11 Python extension (module name `IF_tests`) via fastest's own build flow under `tests/vendor/fastest/bindings`. `tests/main.py` then imports `fastest` and `IF_tests`, points fastest at it as the backend, and runs everything with `fastest.run_log_all()`.

See [TODO.md](TODO.md) for planned additions to the test suite (loss-model regression tests, scheduler topological-order validation, a benchmarking harness) that build on this same convention.
