# DXMT

A Metal-based translation layer for Direct3D 11 and 10 which allows running 3D applications on macOS using Wine.

For the current status of the project, please refer to the [project wiki](https://github.com/3Shain/dxmt/wiki).

The most recent development builds can be found [here](https://github.com/3Shain/dxmt/actions).


## Build

See [DEVELOPMENT.md](docs/DEVELOPMENT.md)

## This fork

`elseform/dxmt` maintains local fixes over upstream. Build instructions are
in [DEVELOPMENT.md](docs/DEVELOPMENT.md).

The `release` branch is what actually ships; tags on it (e.g. `0.80-gamma`)
mark the exact commit a published payload was compiled
from — `git describe --always` embeds the tag name into `DXMT_VERSION`, shown
in-game by Apple's Metal performance HUD. Other branches are in-progress
work or experiments; only `release` is meant to be built from.

### Fixes on top of upstream

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
- **Blit encoder merging, opt-in**. With `DXMT_REORDER_BLITS=1`,
  a blit encoder is moved past independent encoders into the next blit encoder,
  so fewer encoders and render passes are split per frame. Off by default.
- **Shader IR release**. After a shader's pipelines are compiled,
  its parsed intermediate representation is freed and rebuilt from the
  retained DXBC only if a new pipeline variant needs it
  (`d3d11.releaseShaderIR`, default on). Ported from NerRobDog/dxmt. Also frees
  a shader's argument-info buffer, which was leaked.
- **Pipeline cache teardown order**. The compile worker threads
  are joined before the shader and pipeline tasks they run are freed, fixing a
  startup crash (`unrecognized selector ... 0xf` in
  `newComputePipelineStateWithDescriptor`) when the first D3D11 device is
  released. Runtime-verified.
- **Resource-initializer upload cap**. Textures created with
  initial data are submitted in batches of at most 64 MB, waiting for the
  previous batch, instead of staging a whole level load in the upload heap,
  which grew to gigabytes, kept them for the rest of the session, and ended in
  a GPU timeout on the first frame.
- **Memory accounting log**. With `DXMT_LOG_LEVEL=debug`, every
  5 s the log shows the Metal device's allocated size next to DXMT's own
  buffer allocations, parked rename copies and staging ring blocks. A
  diagnostic; at the default log level it costs one comparison per frame.

Everything else in `release`'s history versus upstream is the rebase carrying
these same fixes forward onto newer upstream commits (the fixes were rebased
onto `3Shain/dxmt` `7c8dee1`), not new work.
