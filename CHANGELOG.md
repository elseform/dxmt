# Changelog

Changes to the `release` branch of `elseform/dxmt`, newest first. Releases
are tagged `gamma-YYYY.MM.DD`, with a `.N` suffix for a further release on the
same day. The full list of fixes this fork carries over upstream `3Shain/dxmt`
is in the README, "Fixes introduced".

## gamma-2026.09.27.1 (2026-09-27)

Built for macOS 26 (`MACOSX_DEPLOYMENT_TARGET=26.0`) instead of 15. `winemetal.so` declares macOS 26
and the Metal command shaders in `d3d11.dll` and `d3d12.dll` target
`macosx26.0`.

### Added

- **Frame limiter**, opt-in with `DXMT_FRAME_LIMITER=1`. Paces the game's own
  thread, not only the display, to `d3d11.preferredMaxFrameRate`, or to half
  the display refresh rate when that is not set (30 on a 60 Hz display).
  Game timing, input and presentation then follow one steady rhythm; frames
  no longer run ahead of what the display can show. `d3d11.frameLimit`
  overrides the target; `DXMT_DISPLAY_SYNC_OFF=1` presents without display
  sync.
- **Per-frame stats log**, with `DXMT_STATS_LOG=<dir>`: one CSV row per frame
  with the frame interval, waits, encode times, GPU time and pass counts.

### Changed

- The Metal HUD line is now just the feature level and the release tag (e.g.
  `FL_11_1 gamma-2026.09.27.1`). The `DXMT D3D11` prefix made it long enough
  for the macOS 26+ HUD to cut off the end of the tag.
- Blit encoder merging (`DXMT_REORDER_BLITS=1`) reads its switch once at
  startup, as before, through the same switch set as the limiter.

## gamma-2026.09.27 (2026-09-27)

Upstream base unchanged since `0.80-gamma`: `3Shain/dxmt` `7c8dee1`
(`v0.80` plus 244 upstream commits).

### Fixed

- **Startup crash.** The game could crash during startup with
  `unrecognized selector sent to instance 0xf` in
  `newComputePipelineStateWithDescriptor` when it released its first
  Direct3D 11 device; clicking the splash screen repeatedly sometimes got past
  it. The shader compile worker threads were still running when the tasks they
  worked on were freed. They are now stopped first.
- **Memory growth and GPU timeout while loading textures created with their
  data.** Textures created with initial data were staged for upload all at once: the staging
  area grew by the size of the whole level load, stayed allocated for the rest
  of the session, and the first in-game frame timed out on the GPU. Uploads now
  go out in batches of at most 64 MB.

### Changed

- **Shader IR release** (`d3d11.releaseShaderIR`, default on). After a
  shader's pipelines are built, its parsed intermediate representation is
  freed and rebuilt from the kept DXBC only if a new pipeline variant needs it,
  instead of staying in memory for the whole session. Also fixes a leak of each
  shader's argument-info buffer. Ported from NerRobDog/dxmt.

### Opt-in

- **Blit encoder merging** with `DXMT_REORDER_BLITS=1`. Moves independent
  copy and upload work together so fewer encoders and render passes are split
  per frame. Off by default.

### Diagnostics

- **Memory accounting log** at `DXMT_LOG_LEVEL=debug`: every 5 s, the Metal
  device's allocated size next to DXMT's buffer allocations, parked rename
  copies and staging blocks.

## 0.80-gamma (2026-09-24)

First published release of the fork. Its fixes are listed in the README,
"Fixes on top of upstream".
