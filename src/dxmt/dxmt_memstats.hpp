#pragma once
#include <atomic>
#include <cstdint>

namespace dxmt::memstats {

/* Live byte counters for DXMT_MEMORY_STATS=1 (logged by CommandQueue). */
inline std::atomic<int64_t> buffer_bytes{0};
inline std::atomic<int64_t> buffer_count{0};
/* Rename copies parked in DynamicBuffer FIFOs, waiting for reuse. */
inline std::atomic<int64_t> idle_rename_bytes{0};
inline std::atomic<int64_t> idle_rename_count{0};

} // namespace dxmt::memstats
