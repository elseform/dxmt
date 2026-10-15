# DXMT

A Metal-based translation layer for Direct3D applications on macOS, with
separate Wine and native Darwin build configurations.

## Build

See [DEVELOPMENT.md](docs/DEVELOPMENT.md)

The Metal HUD identifies builds from `release` with the existing Git tag
description. Other named branches show their branch name. Detached builds
retain the tag/hash fallback. This identity is embedded at build time in both
Wine and native configurations.

### Fixes introduced

In commit order:

- **Winemetal resource lifetime/argument-buffer hardening**. Takes each binding's GPU resource ID, exact native Metal
  resource and owning allocation from one snapshot, retains every native
  resource a command buffer uses until that command buffer completes, and
  moves the argument-buffer allocator from `malloc`'d memory to Metal-owned
  storage, closing a use-after-free/aliasing window implicated in a
  mid-gameplay GPU page fault. Hazard tracking stays untracked. One of
  several changes aimed at that fault; see "DynamicBuffer suballocation" and
  "PSO-failure skip-draw guard" below.
- **DynamicBuffer suballocation from one page regardless of Usage**. `Map`/`Discard` renames of a `D3D11_USAGE_DYNAMIC` buffer now
  suballocate from a single page unconditionally, instead of only for some
  usage patterns, removing another source of the same fault class.
- **PSO-failure skip-draw guard, with named shaders in the failure log**. A draw whose pipeline state object failed to
  compile is now skipped instead of proceeding with an invalid PSO, and the
  failure log names the shader by digest alongside the Metal error.
- **DLSS/NGX fixes**. Fixes a
  DXMT-side NGX parameter-store type mismatch that made every
  `NVSDK_NGX_D3D11_EvaluateFeature` call fail silently, so DLSS upscaling
  never actually ran despite being reported available; bridges `void*`
  resource parameters through NGX to the right D3D11/D3D12 resource type;
  crops an oversized depth input and defaults a missing pre-exposure value;
  folds the DLSS depth-crop copy into the existing scaler blit encoder
  instead of an extra pass; and stops a `GetParameters` leak while logging
  `EvaluateFeature` only once instead of every frame.
- **`d3d11.displaySync` v-sync control**. Makes
  `CAMetalLayer.displaySyncEnabled` follow the D3D11 `Present` sync interval
  (`d3d11.displaySync` config key: `auto`/`true`/`false`) instead of being
  hardcoded off, and stops `changeDisplaySync()` from pushing layer
  properties to the Metal layer while a pixel-format/size/sample-count/
  color-space change from `changeLayerProperties()` is still deferred.
- **Motion-vector write tracking**. The downscale dispatch tracks
  the destination's full texture view before the temporal scaler reads it;
  the write previously used an empty view key and escaped fence tracking.
- **Blit encoder merging, opt-in**. With `DXMT_REORDER_BLITS=1`,
  a blit encoder is moved past independent encoders into the next blit encoder,
  so fewer encoders and render passes are split per frame. Off by default.
- **Shader IR release**. After a shader's pipelines are compiled,
  its parsed intermediate representation is freed and rebuilt from the
  retained DXBC only if a new pipeline variant needs it
  (`d3d11.releaseShaderIR`, default on). Ported from NerRobDog/dxmt. Also frees
  a shader's argument-info buffer, which was leaked.
- **Pipeline cache teardown order**. The compile worker threads
  are joined before the shader and pipeline tasks they run are freed when a
  device is released.
- **Startup crash: uninitialized device handle**. The command
  queue's clear pipelines were built through a Metal device handle that was
  still uninitialized, because its member was declared after the objects that
  use it. The game aborted at startup with `unrecognized selector ... 0xf`
  (or another stray value) in `newComputePipelineStateWithDescriptor`. The
  handle is now initialized first.
- **Scheduler shutdown wake-up**. The compile scheduler sets its
  shutdown flag under the workers' mutex, so a worker cannot miss the signal
  and leave a device release waiting for it forever.
- **Resource-initializer upload cap**. Textures created with
  initial data are submitted in batches of at most 64 MB, waiting for the
  previous batch, instead of staging a whole level load in the upload heap,
  which grew to gigabytes, kept them for the rest of the session, and ended in
  a GPU timeout during the first frame after a large upload workload.
- **Memory accounting log**. With `DXMT_LOG_LEVEL=debug`, every
  5 s the log shows the Metal device's allocated size next to DXMT's own
  buffer allocations, parked rename copies and staging ring blocks. A
  diagnostic; at the default log level it costs one comparison per frame.
- **Frame limiter, opt-in**. With `DXMT_FRAME_LIMITER=1`, the
  game's own thread is paced after each `Present` to a steady interval
  (`d3d11.frameLimit`, else `d3d11.preferredMaxFrameRate`, else half the
  display refresh rate), and presents are held to the same interval, so game
  timing, input and the display share one cadence. After falling a whole
  interval behind it resynchronizes instead of catching up.
  `DXMT_DISPLAY_SYNC_OFF=1` presents without display sync.
- **Per-frame stats log**. With `DXMT_STATS_LOG=<dir>`, one CSV
  row per presented frame: frame interval, latency and limiter waits, commit
  and sync waits, encode times, drawable wait, GPU time and pass counts.

Everything else in `release`'s history versus upstream is merging newer
`3Shain/dxmt` commits into `release`, not new work. The upstream base of each
release is stated in [CHANGELOG.md](CHANGELOG.md).

## Synthetic tests

Configure with `-Denable_tests=true`, build the test targets, then run
`meson test -C <build-dir> --no-rebuild --print-errorlogs`. Tests use the
checked-in synthetic HLSL/DXBC fixtures and the libraries built in this checkout;
no game assets, engine checkout, mod installation, or external compiler harness
is required to execute them.

The converter test runs without a Metal device. In a Windows cross build it
runs against the native Unix-side converter configuration; it is not a PE
executable and needs no Wine runner. The existing native Metal ownership test
requires an accessible Metal device.

Converter coverage includes read and unread omitted vertex inputs, unwritten
pixel inputs, and fully supplied or fully absent packed varying registers.
Partially supplied packed registers, live stage linkage, application loading,
and rendering are not covered by these tests.
