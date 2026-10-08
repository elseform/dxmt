/*
 * Native (non-Wine) wsi monitor queries, answered from the state the host
 * application publishes (winemetal_host.h). A monitor handle is the 1-based index
 * into the host's monitor list. The display modes are the native mode plus the
 * 16:9 ladder at or below it: the host renders 16:9 images only.
 */
#include "wsi_monitor.hpp"
#include "winemetal_host.h"
#include <cstdio>
#include <vector>

namespace dxmt::wsi {

namespace {

const uint32_t kLadder[] = {3840, 3200, 2560, 1920, 1600, 1280};

int monitorIndex(HMONITOR hMonitor, const WMTHostWsiState &state) {
  int index = (int)(uintptr_t)hMonitor - 1;
  return (index >= 0 && index < state.monitor_count) ? index : -1;
}

std::vector<WsiMode> modesFor(const WMTHostMonitor &monitor) {
  std::vector<WsiMode> modes;
  WsiRational refresh = {(uint32_t)(monitor.refresh_hz > 0.f ? monitor.refresh_hz : 60.f), 1};
  modes.push_back({(uint32_t)monitor.width, (uint32_t)monitor.height, refresh, 32, false});
  for (uint32_t width : kLadder) {
    if (width >= (uint32_t)monitor.width)
      continue;
    modes.push_back({width, width * 9 / 16, refresh, 32, false});
  }
  return modes;
}

} // namespace

HMONITOR getDefaultMonitor() {
  WMTHostWsiState state;
  WMTHostWsiCopy(&state);
  for (int i = 0; i < state.monitor_count; ++i)
    if (state.monitors[i].primary)
      return (HMONITOR)(uintptr_t)(i + 1);
  return state.monitor_count ? (HMONITOR)(uintptr_t)1 : nullptr;
}

HMONITOR enumMonitors(uint32_t index) {
  WMTHostWsiState state;
  WMTHostWsiCopy(&state);
  return index < (uint32_t)state.monitor_count ? (HMONITOR)(uintptr_t)(index + 1) : nullptr;
}

bool getDisplayName(HMONITOR hMonitor, WCHAR (&Name)[32]) {
  WMTHostWsiState state;
  WMTHostWsiCopy(&state);
  int index = monitorIndex(hMonitor, state);
  if (index < 0)
    return false;
  char narrow[32];
  snprintf(narrow, sizeof(narrow), "\\\\.\\DISPLAY%d", index + 1);
  int i = 0;
  for (; narrow[i] && i < 31; ++i)
    Name[i] = (WCHAR)narrow[i];
  Name[i] = 0;
  return true;
}

bool getDesktopCoordinates(HMONITOR hMonitor, RECT *pRect) {
  WMTHostWsiState state;
  WMTHostWsiCopy(&state);
  int index = monitorIndex(hMonitor, state);
  if (index < 0)
    return false;
  const WMTHostMonitor &m = state.monitors[index];
  pRect->left = m.x;
  pRect->top = m.y;
  pRect->right = m.x + m.width;
  pRect->bottom = m.y + m.height;
  return true;
}

bool getDisplayMode(HMONITOR hMonitor, uint32_t modeNumber, WsiMode *pMode) {
  WMTHostWsiState state;
  WMTHostWsiCopy(&state);
  int index = monitorIndex(hMonitor, state);
  if (index < 0)
    return false;
  std::vector<WsiMode> modes = modesFor(state.monitors[index]);
  if (modeNumber >= modes.size())
    return false;
  *pMode = modes[modeNumber];
  return true;
}

bool getCurrentDisplayMode(HMONITOR hMonitor, WsiMode *pMode) {
  return getDisplayMode(hMonitor, 0, pMode);
}

bool getDesktopDisplayMode(HMONITOR hMonitor, WsiMode *pMode) {
  return getDisplayMode(hMonitor, 0, pMode);
}

} // namespace dxmt::wsi
