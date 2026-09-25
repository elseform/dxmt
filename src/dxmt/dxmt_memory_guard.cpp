#include "dxmt_memory_guard.hpp"
#include "log/log.hpp"
#include "util_env.hpp"
#include <algorithm>
#include <string>

namespace dxmt {

static std::atomic<MemoryPressureLevel> g_memory_pressure = MemoryPressureLevel::Normal;

MemoryPressureLevel
CurrentMemoryPressure() {
  return g_memory_pressure.load(std::memory_order_relaxed);
}

static uint64_t
ReadPercent(const char *name, uint64_t fallback) {
  auto value = env::getEnvVar(name);
  if (value.empty())
    return fallback;
  try {
    return std::clamp<uint64_t>(std::stoull(value), 10, 100);
  } catch (const std::exception &) {
    return fallback;
  }
}

static const char *
LevelName(MemoryPressureLevel level) {
  switch (level) {
  case MemoryPressureLevel::Normal:
    return "normal";
  case MemoryPressureLevel::Throttle:
    return "throttle";
  case MemoryPressureLevel::Split:
    return "split";
  }
  return "unknown";
}

MemoryGuard::MemoryGuard(WMT::Device device) : device_(device) {
  enabled_ = env::getEnvVar("DXMT_MEMORY_GUARD") != "0";
  budget_ = device_.recommendedMaxWorkingSetSize();
  if (!enabled_ || !budget_) {
    enabled_ = false;
    WARN("Memory guard disabled");
    return;
  }
  auto throttle = ReadPercent("DXMT_MEMORY_GUARD_THROTTLE", 85);
  auto split = std::max(ReadPercent("DXMT_MEMORY_GUARD_SPLIT", 95), throttle);
  throttle_enter_ = budget_ / 100 * throttle;
  throttle_exit_ = budget_ / 100 * (throttle - 5);
  split_enter_ = budget_ / 100 * split;
  split_exit_ = budget_ / 100 * (split - 5);
  WARN(
      "Memory guard enabled: working set budget ", budget_ >> 20, " MB, throttle at ", throttle, "%, split at ",
      split, "%"
  );
}

MemoryPressureLevel
MemoryGuard::sample() {
  if (!enabled_)
    return MemoryPressureLevel::Normal;

  uint64_t allocated = device_.currentAllocatedSize();
  auto previous = level_.load(std::memory_order_relaxed);
  auto next = previous;

  switch (previous) {
  case MemoryPressureLevel::Normal:
    if (allocated >= split_enter_)
      next = MemoryPressureLevel::Split;
    else if (allocated >= throttle_enter_)
      next = MemoryPressureLevel::Throttle;
    break;
  case MemoryPressureLevel::Throttle:
    if (allocated >= split_enter_)
      next = MemoryPressureLevel::Split;
    else if (allocated < throttle_exit_)
      next = MemoryPressureLevel::Normal;
    break;
  case MemoryPressureLevel::Split:
    if (allocated < throttle_exit_)
      next = MemoryPressureLevel::Normal;
    else if (allocated < split_exit_)
      next = MemoryPressureLevel::Throttle;
    break;
  }

  if (next != previous) {
    level_.store(next, std::memory_order_relaxed);
    g_memory_pressure.store(next, std::memory_order_relaxed);
    WARN(
        "Memory guard: ", LevelName(previous), " -> ", LevelName(next), " (allocated ", allocated >> 20, " MB of ",
        budget_ >> 20, " MB)"
    );
  }
  return next;
}

} // namespace dxmt
