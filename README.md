# DXMT

A Metal-based translation layer which allows running 3D applications on macOS using Wine.
This fork focuses on achieving best possible performance for GAMMA on SSS24

## Build

See [DEVELOPMENT.md](docs/DEVELOPMENT.md)

### Fixes introduced

In commit order:

- **Winemetal resource lifetime/argument-buffer hardening**
  (`059cd88`). Takes each binding's GPU resource ID, exact native Metal
  resource and owning allocation from one snapshot, retains every native
  resource a command buffer uses until that command buffer completes, and
  moves the argument-buffer allocator from `malloc`'d memory to Metal-owned
  storage, closing a use-after-free/aliasing window implicated in a
  mid-gameplay GPU page fault. Hazard tracking stays untracked. One of
  several changes aimed at that fault; see "DynamicBuffer suballocation" and
  "PSO-failure skip-draw guard" below.
- **DynamicBuffer suballocation from one page regardless of Usage**
  (`3004e88`). `Map`/`Discard` renames of a `D3D11_USAGE_DYNAMIC` buffer now
  suballocate from a single page unconditionally, instead of only for some
  usage patterns, removing another source of the same fault class.
- **PSO-failure skip-draw guard, with named shaders in the failure log**
  (`5d274b4`, `d13d7ed`). A draw whose pipeline state object failed to
  compile is now skipped instead of proceeding with an invalid PSO, and the
  failure log names the shader (by digest) instead of only the Metal error —
  see `gamma-project`'s PSO failure register for the digests this triage
  produced.
- **DLSS/NGX fixes** (`b4f5afc`, `0861c6a`, `7026b8e`, `4f4c6a2`). Fixes a
  DXMT-side NGX parameter-store type mismatch that made every
  `NVSDK_NGX_D3D11_EvaluateFeature` call fail silently, so DLSS upscaling
  never actually ran despite being reported available; bridges `void*`
  resource parameters through NGX to the right D3D11/D3D12 resource type;
  crops an oversized depth input and defaults a missing pre-exposure value;
  folds the DLSS depth-crop copy into the existing scaler blit encoder
  instead of an extra pass; and stops a `GetParameters` leak while logging
  `EvaluateFeature` only once instead of every frame. Confirmed working
  in-game.
- **`d3d11.displaySync` v-sync control** (`9f8b1aa`, `b7f6717`). Makes
  `CAMetalLayer.displaySyncEnabled` follow the D3D11 `Present` sync interval
  (`d3d11.displaySync` config key: `auto`/`true`/`false`) instead of being
  hardcoded off, and stops `changeDisplaySync()` from pushing layer
  properties to the Metal layer while a pixel-format/size/sample-count/
  color-space change from `changeLayerProperties()` is still deferred.
- **Blit encoder merging, opt-in** (`05ba201`). With `DXMT_REORDER_BLITS=1`,
  a blit encoder is moved past independent encoders into the next blit encoder,
  so fewer encoders and render passes are split per frame. Off by default.
- **Shader IR release** (`2d18458`). After a shader's pipelines are compiled,
  its parsed intermediate representation is freed and rebuilt from the
  retained DXBC only if a new pipeline variant needs it
  (`d3d11.releaseShaderIR`, default on). Ported from NerRobDog/dxmt. Also frees
  a shader's argument-info buffer, which was leaked.
- **Pipeline cache teardown order** (`c3a5d2c`). The compile worker threads
  are joined before the shader and pipeline tasks they run are freed, fixing a
  startup crash (`unrecognized selector ... 0xf` in
  `newComputePipelineStateWithDescriptor`) when the first D3D11 device is
  released.
- **Resource-initializer upload cap** (`c73e813`). Textures created with
  initial data are submitted in batches of at most 64 MB, waiting for the
  previous batch, instead of staging a whole level load in the upload heap,
  which grew to gigabytes, kept them for the rest of the session, and ended in
  a GPU timeout on the first frame (X-Ray with `r__no_ram_textures on`).
- **Memory accounting log** (`05cfcf7`). With `DXMT_LOG_LEVEL=debug`, every
  5 s the log shows the Metal device's allocated size next to DXMT's own
  buffer allocations, parked rename copies and staging ring blocks. A
  diagnostic; at the default log level it costs one comparison per frame.

Everything else in `release`'s history versus upstream is the rebase carrying
these same fixes forward onto newer upstream commits (the fixes were rebased
onto `3Shain/dxmt` `7c8dee1`), not new work.
