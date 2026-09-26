#pragma once
#include <cstdint>
#include <string>

namespace dxmt {

/**
Runtime-switchable performance experiments, so A/B comparisons can be made in
the same session and scene instead of across launches. Initial state comes from
the environment; with DXMT_PERF_HOTKEYS=1 each flag is toggled in game with a
function key (Fn+key on Mac keyboards):

- F6:  ReorderBlits   (initial: DXMT_REORDER_BLITS=1)
- F8:  FrameLimiter   (initial: DXMT_FRAME_LIMITER=1)
- F11: DisplaySyncOff (initial: DXMT_DISPLAY_SYNC_OFF=1)
*/
enum class PerfFlag : uint32_t {
  ReorderBlits = 0,
  FrameLimiter = 1,
  DisplaySyncOff = 2,
};

uint32_t perfFlags();

bool perfFlag(PerfFlag flag);

/* Short human-readable form, e.g. "RB+ FL- VS-". */
std::string perfFlagsString(uint32_t flags);

/* Poll the toggle hotkeys; call once per presented frame from the app thread. */
void pollPerfHotkeys();

} // namespace dxmt
