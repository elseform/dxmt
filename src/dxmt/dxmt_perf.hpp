#pragma once
#include <cstdint>
#include <string>

namespace dxmt {

/**
Performance switches, read once from the environment at startup:

- ReorderBlits:   DXMT_REORDER_BLITS=1
- FrameLimiter:   DXMT_FRAME_LIMITER=1
- DisplaySyncOff: DXMT_DISPLAY_SYNC_OFF=1
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

} // namespace dxmt
