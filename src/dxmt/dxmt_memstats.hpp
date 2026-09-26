#pragma once
#include <atomic>
#include <cstdint>

namespace dxmt::memstats {

/* Live byte counters, logged by CommandQueue at DXMT_LOG_LEVEL=debug. */
inline std::atomic<int64_t> buffer_bytes{0};
inline std::atomic<int64_t> buffer_count{0};
/* Rename copies parked in DynamicBuffer FIFOs, waiting for reuse. */
inline std::atomic<int64_t> idle_rename_bytes{0};
inline std::atomic<int64_t> idle_rename_count{0};
/* Staging ring blocks (upload heap, argument buffers, staging uploads). */
inline std::atomic<int64_t> staging_block_bytes{0};
inline std::atomic<int64_t> staging_block_count{0};

} // namespace dxmt::memstats
