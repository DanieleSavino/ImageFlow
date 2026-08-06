# TODO

## 1. Per (op, dev) loss estimate
Model expected cost of running a given `IF_SupportedOp_t` on a given `IF_DevType_t`,
using the `flops_per_pixel` / `bytes_per_pixel` metadata already present in
`IF_OpImpl_t` (currently populated but unused downstream).
- [ ] Define a cost/loss function combining compute (flops) and memory
      (bytes) cost, weighted per device (e.g. CPU vs GPU throughput/bandwidth
      profiles).
- [ ] Decide where device throughput/bandwidth constants live (static table?
      runtime-probed? q-table based corrections?).
- [ ] Expose via something like `IF_op_estimate_loss(op, dev, width, height)`.
- Used for both scheduling decisions (2) and as ground truth for tests (3).

## 2. Per-scheduler loss estimate
*(depends on 1)*
Aggregate per-(op, dev) loss across a full pipeline to score a given
scheduling/device-assignment plan.
- [ ] Sum/accumulate loss across the operation sequence for a candidate
      device assignment.
- [ ] Account for transfer cost between devices (host2dev/dev2host) when an
      op sequence crosses device boundaries — not just per-op compute cost.
- [ ] Produce a single comparable score so different schedules can be ranked.

## 3. Tests for scheduler
*(depends on 2)*
- [ ] Validate loss-estimate correctness against measured runtime (regression
      test against real timings, not just internal consistency).
- [ ] Validate that `scheduler/reorder.c` produces a valid topological order
      w.r.t. op dependencies.
- [ ] Validate device-assignment decisions once (4) exists — i.e. that the
      scheduler actually picks the lower-loss device when both are available.

## 4. Device-selecting scheduler
*(depends on 1)*
Currently `scheduler/reorder.c` only reorders ops on a fixed device;
device selection itself is static. Extend it to exploit the (op, dev) loss
model to choose devices, not just op order.
- [ ] Given a pipeline and available devices, assign each op to the device
      minimizing total pipeline loss (compute + transfer).
- [ ] Handle cases where reordering *and* redevicing interact (an op moved
      earlier/later may change which device is optimal due to adjacent
      transfers).
- [ ] Fallback behavior when an op has no implementation on the "optimal"
      device (must degrade to CPU or next-best, per `IF_op_supported`).

## 5. Benchmarking
- [ ] Baseline timings per (op, dev) to calibrate/validate the loss model
      (feeds back into 1).
- [ ] End-to-end pipeline benchmarks: naive fixed-device vs. reorder-only
      vs. reorder+redevice scheduler.
- [ ] Establish a repeatable benchmark harness (fixed image sizes/seeds,
      similar to the `devs/`/`ops/` test convention) so numbers are
      comparable across runs and CI.
