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
in-game by Apple's Metal performance HUD. Other branches
(`vsync-updates`, `dlssperf-pass2-notexview`, `pagefault-pass1-psoguard`,
`release-on-upstream`) are in-progress work; only `release` is meant to be
built from.

### Fixes on top of upstream

All authored by `elseform`, in commit order:

- **Winemetal resource lifetime/argument-buffer hardening**. Tracks Metal hazard mode for D3D11 resource allocations and
  moves the winemetal argument-buffer allocator to a Metal-owned allocation,
  closing a use-after-free/aliasing window implicated in a mid-gameplay GPU
  page fault. One of several changes aimed at that fault; see
  "DynamicBuffer suballocation" and "PSO-failure skip-draw guard" below.
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

Everything else in `release`'s history versus upstream is the rebase carrying
these same fixes forward onto newer upstream commits (`vsync-updates` was
rebuilt on top of `3Shain/dxmt` `7c8dee1`), not new work.
