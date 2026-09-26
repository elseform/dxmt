#include "dxmt_stats_log.hpp"
#include "log/log.hpp"
#include "util_env.hpp"
#include "util_string.hpp"
#include <algorithm>
#include <ctime>
#include <string>

namespace dxmt {

namespace {

double
ms(clock::duration d) {
  return std::chrono::duration<double, std::milli>(d).count();
}

std::string
toWindowsPath(std::string path) {
  if (!path.empty() && path.front() == '/') {
    std::replace(path.begin(), path.end(), '/', '\\');
    path = "Z:" + path;
  }
  if (!path.empty() && path.back() != '\\' && path.back() != '/')
    path += '\\';
  return path;
}

} // namespace

StatsLog::StatsLog() {
  std::string dir = env::getEnvVar("DXMT_STATS_LOG");
  if (dir.empty())
    return;
  char stamp[32];
  std::time_t now = std::time(nullptr);
  std::strftime(stamp, sizeof(stamp), "%Y%m%d-%H%M%S", std::localtime(&now));
  std::string path = toWindowsPath(dir) + "dxmt-stats-" + env::getExeBaseName() + "-" + stamp + ".csv";
  stream_ = std::ofstream(str::topath(path.c_str()).c_str());
  if (!stream_) {
    WARN("Stats log: cannot open ", path);
    return;
  }
  enabled_ = true;
  WARN("Stats log: ", path);
  stream_ << "frame,t_ms,frame_ms,flags,latency_wait_ms,limiter_wait_ms,commit_wait_ms,sync_count,sync_ms,"
             "event_stall,encode_prepare_ms,encode_flush_ms,drawable_wait_ms,gpu_ms,command_buffers,"
             "render_passes,render_merged,clear_passes,clear_merged,compute_passes,blit_passes,blit_merged,"
             "max_latency\n";
}

StatsLog::~StatsLog() {
  if (enabled_)
    stream_.flush();
}

void
StatsLog::onPresent(const FrameStatisticsContainer &statistics, uint64_t frame_count, uint32_t max_latency) {
  if (!enabled_)
    return;
  /*
  Log a frame whose GPU work is known to be finished: PresentBoundary() has just
  waited for frame (frame_count - max_latency), and the ring holds
  kFrameStatisticsCount frames.
  */
  uint64_t lag = std::clamp<uint64_t>(max_latency + 2, 8, kFrameStatisticsCount - 2);
  if (frame_count <= lag)
    return;
  uint64_t frame = frame_count - lag;
  auto &f = statistics.at(frame);
  if (f.present_time == clock::time_point{})
    return;
  if (origin_ == clock::time_point{})
    origin_ = f.present_time;
  double frame_ms = previous_present_ == clock::time_point{} ? 0.0 : ms(f.present_time - previous_present_);
  previous_present_ = f.present_time;

  char row[512];
  snprintf(
      row, sizeof(row),
      "%llu,%.3f,%.3f,%u,%.3f,%.3f,%.3f,%u,%.3f,%u,%.3f,%.3f,%.3f,%.3f,%u,%u,%u,%u,%u,%u,%u,%u,%u\n",
      (unsigned long long)frame, ms(f.present_time - origin_), frame_ms, f.perf_flags, ms(f.present_latency_interval),
      ms(f.limiter_interval), ms(f.commit_interval), f.sync_count, ms(f.sync_interval), f.event_stall,
      ms(f.encode_prepare_interval), ms(f.encode_flush_interval), ms(f.drawable_blocking_interval),
      f.gpu_time_ns / 1e6, f.command_buffer_count, f.render_pass_count, f.render_pass_optimized, f.clear_pass_count,
      f.clear_pass_optimized, f.compute_pass_count, f.blit_pass_count, f.blit_pass_optimized, f.latency
  );
  stream_ << row;
  if (++rows_ % 120 == 0)
    stream_.flush();
}

} // namespace dxmt
