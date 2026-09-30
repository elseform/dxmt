# Changelog

Changes to the `release` branch of `elseform/dxmt`, newest first. Releases
are tagged `anomaly-YYYY.MM.DD`, with a `.N` suffix for a further release on
the same day. Releases up to `gamma-2026.09.27.1` were tagged with the `gamma-`
prefix before the rebrand to Anomaly; those tags and their entries below keep
their original names. The full list of fixes this fork carries over upstream
`3Shain/dxmt` is in the README, "Fixes introduced".

## anomaly-2026.09.30 (2026-09-30)

Upstream base unchanged since `0.80-gamma`: `3Shain/dxmt` `7c8dee1`
(`v0.80` plus 244 upstream commits).

### Changed

- **Release name.** Releases are now tagged `anomaly-YYYY.MM.DD` instead of
  `gamma-YYYY.MM.DD`, following the rebrand of GAMMA's supporting tools to
  Anomaly. The Metal HUD version line shows the new tag.

### Fixed

- **Startup crash.** The game could abort during startup, while it created its
  first Direct3D 11 device, with `unrecognized selector sent to instance ...`
  in `newComputePipelineStateWithDescriptor`; the receiver was `0xf`, `0x6d`
  or a fragment of asset-path text. The command queue builds its clear
  pipelines through a Metal device handle, but the member holding that handle
  was declared after the objects that use it and so was still uninitialized
  when they ran. Depending on what the new memory held, the pipelines were
  silently missing or the call went to a garbage handle. The handle is now
  initialized first. This is the actual cause of the crash listed under
  gamma-2026.09.27; the change listed there did not fix it. (`35c6ed4`)
- **Possible hang when a device is released.** The pipeline compile worker
  threads could miss their shutdown signal and leave the device release
  waiting for them forever. (`4660ac1`)

## gamma-2026.09.27.1 (2026-09-27)

Built for macOS 26 (`MACOSX_DEPLOYMENT_TARGET=26.0`) instead of 15: GAMMA's
minimum is now macOS 26 on Apple Silicon. `winemetal.so` declares macOS 26
and the Metal command shaders in `d3d11.dll` and `d3d12.dll` target
`macosx26.0`.

### Added

- **Frame limiter**, opt-in with `DXMT_FRAME_LIMITER=1`. Paces the game's own
  thread, not only the display, to `d3d11.preferredMaxFrameRate`, or to half
  the display refresh rate when that is not set (30 on a 60 Hz display).
  Game timing, input and presentation then follow one steady rhythm; frames
  no longer run ahead of what the display can show. `d3d11.frameLimit`
  overrides the target; `DXMT_DISPLAY_SYNC_OFF=1` presents without display
  sync. (`c41cb78`)
- **Per-frame stats log**, with `DXMT_STATS_LOG=<dir>`: one CSV row per frame
  with the frame interval, waits, encode times, GPU time and pass counts.
  (`c41cb78`)

### Changed

- The Metal HUD line is now just the feature level and the release tag (e.g.
  `FL_11_1 gamma-2026.09.27.1`). The `DXMT D3D11` prefix made it long enough
  for the macOS 26+ HUD to cut off the end of the tag. (`1820ffc`)
- Blit encoder merging (`DXMT_REORDER_BLITS=1`) reads its switch once at
  startup, as before, through the same switch set as the limiter.

## gamma-2026.09.27 (2026-09-27)

Upstream base unchanged since `0.80-gamma`: `3Shain/dxmt` `7c8dee1`
(`v0.80` plus 244 upstream commits).

### Fixed

- **Compile worker shutdown order.** The shader compile worker threads could
  still be running when the tasks they worked on were freed as a device was
  released. They are now stopped first. This was listed as the fix for the
  startup crash; it was not, and that crash is fixed in anomaly-2026.09.30.
  (`c3a5d2c`)
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
