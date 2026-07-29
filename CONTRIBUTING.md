# Contributing to ImageFlow

This is a toy project, but PRs are welcome. The dispatch layer for both devices and operations is table-driven, so adding either usually means adding a `.def` entry plus one self-registering implementation file — no changes to the scheduler, pipeline, or dispatch code required.

## Adding a device

1. **Register it in `include/ImageFlow/devices/devices.def`**:

   ```c
   IF_DEV_DEF(CPU, "CPU")

   // Gpus
   IF_DEV_DEF(CUDA, "CUDA")
   IF_DEV_DEF(HIP, "HIP")
   ```

   Add a new `IF_DEV_DEF(YOUR_DEV, "YourDev")` line. This drives the `IF_DevType_t` enum, `IF_strdev`, and the size of every per-device dispatch table — nothing else needs to know the device exists.

2. **Implement the backend.** Create `include/ImageFlow/backends/your_dev.h` (+ a matching `src/backends/your_dev.{c,cpp,cu}` that just `#include`s it, following the existing backends) and provide:

   - **A runtime availability check**, registered via `IF_CONSTRUCTOR` so it runs before `main()` and calls `IF_enable_device` / `IF_disable_device`.
   - **A load-image implementation**, via `IF_LOAD_IMG_IMPL(YOUR_DEV) { ... }` — moves a host image onto the device, populating `imgs[IF_DEV_YOUR_DEV]`.
   - **A retrieve-image implementation**, via `IF_RETRIEVE_IMG_IMPL(YOUR_DEV) { ... }` — moves `imgs[IF_DEV_YOUR_DEV]` back into the host image.
   - **A free-image implementation**, via `IF_FREE_IMG_IMPL(YOUR_DEV) { ... }` — releases whatever the device-side allocation is and clears the slot back to `NULL`.
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
   - `IF_FREE_IMG_IMPL(CUDA)`, which frees the device buffer and struct and resets `imgs[IF_DEV_CUDA]` to `NULL`.
   - One `IF_OP_IMPL(CUDA, OP, flops_per_pixel, bytes_per_pixel) { ... }` per operation (`GRAYSCALE`, `INVERT`, `BRIGHTNESS`), each launching a `__global__` kernel over the device image and synchronizing before returning.

   A new backend should mirror this shape: availability check → load/retrieve/free → one `IF_OP_IMPL` per supported operation.

3. **Wire it into `CMakeLists.txt`** so its source file(s) get compiled and linked only when the relevant toolchain (CUDA/HIP/etc.) is actually available, following the existing `HAVE_CUDA` / `HAVE_HIP` pattern.

## Adding an operation

1. **Register it in `include/ImageFlow/operations/operations.def`**:

   ```c
   IF_OP_DEF(GRAYSCALE,  Grayscale,  POINT, EMPTY)
   IF_OP_DEF(INVERT,     Invert,     POINT, EMPTY)
   IF_OP_DEF(BRIGHTNESS, Brightness, POINT, FLOAT_FACTOR)
   ```

   Each line is `IF_OP_DEF(id, name, traversal_type, args)`:

   - `id` — the bare identifier pasted into `IF_OP_##id` (e.g. `IF_OP_GRAYSCALE`).
   - `name` — used for the generated pipeline-builder function (`IF_flow_##name`, e.g. `IF_flow_Grayscale`) and for `IF_strop`.
   - `traversal_type` — one of `METADATA`, `POINT`, `STENCIL`, `REDUCTION`, `MORPH`. This characterizes the operation's memory access pattern and controls how aggressively the reorder scheduler is allowed to move it across device boundaries.
   - `args` — the suffix of the `IF_OP_ARGS_*` / `IF_OP_INIT_*` macros for this operation's parameters (see below). Use `EMPTY` if the operation takes no parameters.

   Adding this line is enough to get `IF_flow_{name}(flow, ...)` generated automatically for you — you don't write the pipeline builder function yourself.

2. **If the operation needs parameters**, extend `include/ImageFlow/operations/op_args.h`:

   ```c
   /**
    * @brief Tagged union of operation-specific arguments.
    *
    * Extend this union when adding new operations that require parameters.
    *
    * @warning Adding pointer members here would complicate any future
    *          distributed (MPI) execution model significantly.
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
   - `#define IF_OP_INIT_YOUR_ARGS (IF_OpArgs_t){ .your_member = { ... } }` — how the builder packs its parameters into the union.

   Keep members flat (no pointers) — see the warning in `op_args.h` about why pointer members would complicate any future MPI/distributed execution model.

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
   - `flops_per_pixel` / `bytes_per_pixel` are cost-model metadata used for diagnostics (`IF_print_op_impls`) — pass reasonable estimates.
   - An operation doesn't need an implementation on every device. If a device has no registered implementation for an operation, the scheduler falls back to CPU for that operation automatically.

That's it — no changes are needed to `pipeline.c`, `operations.c`, or any of the schedulers. The `.def` list is the single source of truth they all read from.

## Adding a scheduler

Unlike devices and operations, schedulers aren't table-driven yet — there's no `.def` list or self-registration macro for them. Adding one means:

1. Add a new enumerator to `IF_Scheduler_t` in `include/ImageFlow/scheduler/scheduler.h`.
2. Implement it, following the shape of `IF_cpu_execute` / `IF_linear_execute` — an `IF_error_t your_execute(IF_Flow_t flow, const IF_image_t *img_in, IF_image_t *img_out)` function, declared in its own `scheduler/your_sched.h` and defined in `scheduler/your_sched.c`.
3. Add a `case` for it by hand in the switch in `src/scheduler/scheduler.c`:

   ```c
   NODISCARD IF_error_t IF_flow_run_sched(IF_Flow_t flow, const IF_image_t *img_in, IF_Scheduler_t sched, IF_image_t *img_out) {
       IF_CHECK_SCHED_PARAMS(flow, img_in, img_out);
       switch (sched) {
           case IF_SCHEDULER_CPU:
               return IF_cpu_execute(flow, img_in, img_out);
           case IF_SCHEDULER_LINEAR:
               return IF_linear_execute(flow, img_in, img_out);
           case IF_SCHEDULER_REORDER_O0:
               IF_CHECK(IF_reorder(flow, IF_REORDER_SAFE));
               return IF_linear_execute(flow, img_in, img_out);
           case IF_SCHEDULER_REORDER_O1:
               IF_CHECK(IF_reorder(flow, IF_REORDER_STENCIL));
               return IF_linear_execute(flow, img_in, img_out);
           case IF_SCHEDULER_REORDER_O2:
               IF_CHECK(IF_reorder(flow, IF_REORDER_REDUCTION));
               return IF_linear_execute(flow, img_in, img_out);
           case IF_SCHEDULER_REORDER_O3:
               IF_CHECK(IF_reorder(flow, IF_REORDER_MORPH));
               return IF_linear_execute(flow, img_in, img_out);
           // add your case here
           default:
               return IF_INVALID_ARGS;
       }
   }
   ```

   Most of the existing schedulers are thin: `IF_reorder` mutates the flow in place (reordering ops for device-transfer locality) and then delegates to `IF_linear_execute` to actually run it. A new scheduler can follow the same pattern — do some pipeline-level transformation, then hand off to `IF_linear_execute` — or execute the flow directly, whichever fits.
4. If it should be the default, update `_DEFAULT_SCHED` at the top of `scheduler.c` (currently `IF_SCHEDULER_REORDER_O0`), which is what `IF_flow_run` (the no-scheduler-argument entry point) uses.

If schedulers keep growing, this switch is the obvious next candidate for the same registration-table treatment devices and operations already got — but for now it's just a manual `case`.
