#pragma once
#include "dxmt_statistics.hpp"
#include <fstream>

namespace dxmt {

/**
Per-frame CSV log of FrameStatistics, enabled with DXMT_STATS_LOG=<directory>.
A Unix path (starting with '/') is mapped to Wine's Z: drive. One row is written
per presented frame, a few frames late so the encode thread and GPU numbers of
that frame are complete.
*/
class StatsLog {
public:
  StatsLog();
  ~StatsLog();

  /* Call from the app thread right after PresentBoundary() advanced to `frame_count`. */
  void onPresent(const FrameStatisticsContainer &statistics, uint64_t frame_count, uint32_t max_latency);

private:
  std::ofstream stream_;
  bool enabled_ = false;
  clock::time_point origin_{};
  clock::time_point previous_present_{};
  uint64_t rows_ = 0;
};

} // namespace dxmt
