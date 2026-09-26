#include "dxmt_perf.hpp"
#include "log/log.hpp"
#include "util_env.hpp"
#include <atomic>

#ifdef _WIN32
#include "windows.h"
#endif

namespace dxmt {

namespace {

struct PerfState {
  std::atomic<uint32_t> flags;
  bool hotkeys;

  PerfState() {
    uint32_t initial = 0;
    if (env::getEnvVar("DXMT_REORDER_BLITS") == "1")
      initial |= 1u << uint32_t(PerfFlag::ReorderBlits);
    if (env::getEnvVar("DXMT_FRAME_LIMITER") == "1")
      initial |= 1u << uint32_t(PerfFlag::FrameLimiter);
    if (env::getEnvVar("DXMT_DISPLAY_SYNC_OFF") == "1")
      initial |= 1u << uint32_t(PerfFlag::DisplaySyncOff);
    flags.store(initial, std::memory_order_relaxed);
    hotkeys = env::getEnvVar("DXMT_PERF_HOTKEYS") == "1";
    if (hotkeys || initial)
      WARN("Perf flags: ", perfFlagsString(initial), hotkeys ? " (hotkeys F6/F8/F11 enabled)" : "");
  }
};

PerfState &
state() {
  static PerfState instance;
  return instance;
}

} // namespace

uint32_t
perfFlags() {
  return state().flags.load(std::memory_order_relaxed);
}

bool
perfFlag(PerfFlag flag) {
  return perfFlags() & (1u << uint32_t(flag));
}

std::string
perfFlagsString(uint32_t flags) {
  auto bit = [&](PerfFlag f) { return (flags & (1u << uint32_t(f))) ? '+' : '-'; };
  std::string s;
  s += "RB";
  s += bit(PerfFlag::ReorderBlits);
  s += " FL";
  s += bit(PerfFlag::FrameLimiter);
  s += " VS";
  s += bit(PerfFlag::DisplaySyncOff);
  return s;
}

void
pollPerfHotkeys() {
#ifdef _WIN32
  auto &s = state();
  if (!s.hotkeys)
    return;
  static const struct {
    int vk;
    PerfFlag flag;
  } keys[] = {
      {VK_F6, PerfFlag::ReorderBlits},
      {VK_F8, PerfFlag::FrameLimiter},
      {VK_F11, PerfFlag::DisplaySyncOff},
  };
  static bool pressed[std::size(keys)] = {};
  for (unsigned i = 0; i < std::size(keys); i++) {
    bool down = GetAsyncKeyState(keys[i].vk) & 0x8000;
    if (down && !pressed[i]) {
      uint32_t flags = s.flags.fetch_xor(1u << uint32_t(keys[i].flag), std::memory_order_relaxed) ^
                       (1u << uint32_t(keys[i].flag));
      WARN("Perf flags: ", perfFlagsString(flags));
    }
    pressed[i] = down;
  }
#endif
}

} // namespace dxmt
