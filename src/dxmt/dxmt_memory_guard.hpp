#pragma once

#include "Metal.hpp"
#include <atomic>
#include <cstdint>

namespace dxmt {

enum class MemoryPressureLevel : uint32_t {
  Normal = 0,
  /* Serialize command chunk submission and release idle pool blocks */
  Throttle = 1,
  /* Additionally commit at every pass boundary */
  Split = 2,
};

/**
 * Trades throughput for a lower peak Metal working set when the device's
 * allocated size approaches `recommendedMaxWorkingSetSize`.
 *
 * Sampled on every command chunk commit. Enabled by default; set
 * DXMT_MEMORY_GUARD=0 to disable. Thresholds are percentages of the
 * recommended working set:
 * - DXMT_MEMORY_GUARD_THROTTLE (default 85)
 * - DXMT_MEMORY_GUARD_SPLIT (default 95)
 * Each level is left again 5 percentage points below its entry threshold.
 */
class MemoryGuard {
public:
  explicit MemoryGuard(WMT::Device device);

  MemoryPressureLevel sample();

  MemoryPressureLevel
  level() const {
    return level_.load(std::memory_order_relaxed);
  }

private:
  WMT::Device device_;
  bool enabled_ = true;
  uint64_t budget_ = 0;
  uint64_t throttle_enter_ = 0;
  uint64_t throttle_exit_ = 0;
  uint64_t split_enter_ = 0;
  uint64_t split_exit_ = 0;
  std::atomic<MemoryPressureLevel> level_ = MemoryPressureLevel::Normal;
};

/**
 * Process-wide pressure level of the most recently sampled guard, for code
 * that has no access to the command queue (dynamic buffer pools, contexts).
 */
MemoryPressureLevel CurrentMemoryPressure();

} // namespace dxmt
