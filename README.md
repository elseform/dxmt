# DXMT

A Metal-based translation layer for Direct3D 11 and 10 which allows running 3D applications on macOS using Wine.

For the current status of the project, please refer to the [project wiki](https://github.com/3Shain/dxmt/wiki).

The most recent development builds can be found [here](https://github.com/3Shain/dxmt/actions).


## Build

See [DEVELOPMENT.md](docs/DEVELOPMENT.md)

## This fork

`elseform/dxmt` is a maintained fork of the above, built and shipped as the
Direct3D 11 backend for [GAMMA Setup Tool](https://github.com/elseform/gamma-setup-tool)
and [gamma-wine-engine](https://github.com/elseform/gamma-wine-engine), which
wrap S.T.A.L.K.E.R. Anomaly / G.A.M.M.A. for macOS. Build instructions for a
shippable payload from this fork:
[gamma-project's DXMT build procedure](https://github.com/elseform/gamma-project)
(private; ask if you need it mirrored here).

The `release` branch is what actually ships; tags on it (e.g. `0.80-gamma`)
mark the exact commit a published `gamma-wine-engine` build was compiled
from — `git describe --always` embeds the tag name into `DXMT_VERSION`, shown
in-game by Apple's Metal performance HUD. Other branches
(`vsync-updates`, `dlssperf-pass2-notexview`, `pagefault-pass1-psoguard`,
`release-on-upstream`) are in-progress work; only `release` is meant to be
built from.

### Fixes on top of upstream

In commit order:

- **Winemetal resource lifetime/argument-buffer hardening**
  (`059cd88`). Tracks Metal hazard mode for D3D11 resource allocations and
  moves the winemetal argument-buffer allocator to a Metal-owned allocation,
  closing a use-after-free/aliasing window implicated in a mid-gameplay GPU
  page fault. One of several changes aimed at that fault; see
  "DynamicBuffer suballocation" and "PSO-failure skip-draw guard" below.
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
  Runtime-verified: toggling v-sync in game and changing resolution both
  behave correctly.

Everything else in `release`'s history versus upstream is the rebase carrying
these same fixes forward onto newer upstream commits (`vsync-updates` was
rebuilt on top of `3Shain/dxmt` `7c8dee1`), not new work.
