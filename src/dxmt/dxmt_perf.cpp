#include "dxmt_perf.hpp"
#include "log/log.hpp"
#include "util_env.hpp"
#include <atomic>

namespace dxmt {

namespace {

struct PerfState {
  std::atomic<uint32_t> flags;

  PerfState() {
    uint32_t initial = 0;
    if (env::getEnvVar("DXMT_REORDER_BLITS") == "1")
      initial |= 1u << uint32_t(PerfFlag::ReorderBlits);
    if (env::getEnvVar("DXMT_FRAME_LIMITER") == "1")
      initial |= 1u << uint32_t(PerfFlag::FrameLimiter);
    if (env::getEnvVar("DXMT_DISPLAY_SYNC_OFF") == "1")
      initial |= 1u << uint32_t(PerfFlag::DisplaySyncOff);
    flags.store(initial, std::memory_order_relaxed);
    if (initial)
      WARN("Perf flags: ", perfFlagsString(initial));
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

} // namespace dxmt
