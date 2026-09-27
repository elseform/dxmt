# Changelog

Changes to the `release` branch of `elseform/dxmt`, newest first. Releases
are tagged `gamma-YYYY.MM.DD`, with a `.N` suffix for a further release on the
same day. The full list of fixes this fork carries over upstream `3Shain/dxmt`
is in the README, "Fixes introduced".

## gamma-2026.09.27.1 (2026-09-27)

Same code as `gamma-2026.09.27`, built for macOS 26
(`MACOSX_DEPLOYMENT_TARGET=26.0`) instead of 15: GAMMA's minimum is now
macOS 26 on Apple Silicon. `winemetal.so` declares macOS 26 and the Metal
command shaders in `d3d11.dll` and `d3d12.dll` target `macosx26.0`.

## gamma-2026.09.27 (2026-09-27)

Upstream base unchanged since `0.80-gamma`: `3Shain/dxmt` `7c8dee1`
(`v0.80` plus 244 upstream commits).

### Fixed

- **Startup crash.** The game could crash during startup with
  `unrecognized selector sent to instance 0xf` in
  `newComputePipelineStateWithDescriptor` when it released its first
  Direct3D 11 device; clicking the splash screen repeatedly sometimes got past
  it. The shader compile worker threads were still running when the tasks they
  worked on were freed. They are now stopped first. (`c3a5d2c`)
- **Memory growth and GPU timeout while loading textures created with their
  data.** Textures created with initial data (in GAMMA, with
  `r__no_ram_textures on`) were staged for upload all at once: the staging
  area grew by the size of the whole level load, stayed allocated for the rest
  of the session, and the first in-game frame timed out on the GPU. Uploads now
  go out in batches of at most 64 MB. Dead City on a 16 GB M1 Pro: Metal
  allocation peak 22.5 GB before, 17.5 GB after, swap 12.2 GB before, 9.3 GB
  after, and the level runs instead of freezing. (`c73e813`)

### Changed

- **Shader IR release** (`d3d11.releaseShaderIR`, default on). After a
  shader's pipelines are built, its parsed intermediate representation is
  freed and rebuilt from the kept DXBC only if a new pipeline variant needs it,
  instead of staying in memory for the whole session. Also fixes a leak of each
  shader's argument-info buffer. Ported from NerRobDog/dxmt. (`2d18458`)

### Opt-in

- **Blit encoder merging** with `DXMT_REORDER_BLITS=1`. Moves independent
  copy and upload work together so fewer encoders and render passes are split
  per frame. Off by default. (`05ba201`)

### Diagnostics

- **Memory accounting log** at `DXMT_LOG_LEVEL=debug`: every 5 s, the Metal
  device's allocated size next to DXMT's buffer allocations, parked rename
  copies and staging blocks. (`05cfcf7`)

## 0.80-gamma (2026-09-24)

First published release of the fork. Its fixes are listed in the README,
"Fixes on top of upstream".
