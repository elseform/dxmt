/*
 * Native (non-Wine) wsi window queries, answered from the state the host application
 * publishes (winemetal_host.h). The window handle is the host's NSView*. The host
 * owns full screen, so the fullscreen and display-mode functions only report that
 * nothing was changed; the engine's native life-cycle never calls them.
 */
#include "wsi_window.hpp"
#include "wsi_monitor.hpp"
#include "winemetal_host.h"

namespace dxmt::wsi {

namespace {
bool isHostWindow(HWND hWindow, const WMTHostWsiState &state) {
  return hWindow && (void *)hWindow == state.window.view;
}
} // namespace

void getWindowSize(HWND hWindow, uint32_t *pWidth, uint32_t *pHeight) {
  WMTHostWsiState state;
  WMTHostWsiCopy(&state);
  bool known = isHostWindow(hWindow, state);
  if (pWidth)
    *pWidth = known ? (uint32_t)state.window.width : 0;
  if (pHeight)
    *pHeight = known ? (uint32_t)state.window.height : 0;
}

void resizeWindow(HWND, DXMTWindowState *, uint32_t, uint32_t) {}

bool setWindowMode(HMONITOR, HWND, const WsiMode &) { return false; }

bool enterFullscreenMode(HMONITOR, HWND, DXMTWindowState *, [[maybe_unused]] bool) {
  return false;
}

bool leaveFullscreenMode(HWND, DXMTWindowState *, bool) { return false; }

bool restoreDisplayMode(HMONITOR) { return false; }

HMONITOR getWindowMonitor(HWND hWindow) {
  WMTHostWsiState state;
  WMTHostWsiCopy(&state);
  if (isHostWindow(hWindow, state) && state.window.monitor_index >= 0 &&
      state.window.monitor_index < state.monitor_count)
    return (HMONITOR)(uintptr_t)(state.window.monitor_index + 1);
  return getDefaultMonitor();
}

bool isWindow(HWND hWindow) {
  WMTHostWsiState state;
  WMTHostWsiCopy(&state);
  return isHostWindow(hWindow, state);
}

void updateFullscreenWindow(HMONITOR, HWND, bool) {}

bool isForeground(HWND) { return WMTHostWsiIsForeground(); }

bool isMinimized(HWND) { return WMTHostWsiIsMinimized(); }

} // namespace dxmt::wsi
