/*
 * Host window-system state for native (non-Wine) builds.
 *
 * In a native build the host application (the X-Ray engine's AppKit layer) owns the
 * window, the screens and the activation state. It publishes them here and DXMT's
 * wsi layer (wsi_window_native.cpp, wsi_monitor_native.cpp) reads them from any
 * thread, so no AppKit call is ever made from the present path.
 *
 * Writer: the host's main thread only. Wrap every change in WMTHostWsiBeginUpdate /
 * WMTHostWsiEndUpdate; readers retry while a write is in progress. The foreground and
 * minimized flags are plain atomics that may be stored at any time.
 *
 * Plain C so the host can include it next to AppKit headers. The functions live in
 * winemetal.dylib; a host that may run without DXMT looks them up with dlsym.
 */
#ifndef WINEMETAL_HOST_H
#define WINEMETAL_HOST_H

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define WMT_HOST_WSI_VERSION 1
#define WMT_HOST_MAX_MONITORS 8

struct WMTHostMonitor {
  char name[64];
  int32_t x, y;          /* origin in pixels, top-left based */
  int32_t width, height; /* native pixel size */
  float refresh_hz;
  float backing_scale;
  int32_t primary;
};

struct WMTHostWindow {
  void *view;            /* the NSView* given to CreateSwapChainForHwnd; NULL when no window */
  int32_t width, height; /* size of the 16:9 image in pixels */
  float backing_scale;
  int32_t monitor_index; /* index into monitors[] of the screen the window is on */
};

struct WMTHostWsiState {
  uint32_t version;
  int32_t monitor_count;
  struct WMTHostMonitor monitors[WMT_HOST_MAX_MONITORS];
  struct WMTHostWindow window;
};

/* Seqlock-protected block (writer: host main thread). */
void WMTHostWsiBeginUpdate(void);
struct WMTHostWsiState *WMTHostWsiMutableState(void); /* write between Begin and End */
void WMTHostWsiEndUpdate(void);

/* Flags the host may store at any time. */
void WMTHostWsiSetForeground(bool foreground);
void WMTHostWsiSetMinimized(bool minimized);

/* Reader side (DXMT). Copy returns a consistent snapshot. */
void WMTHostWsiCopy(struct WMTHostWsiState *out);
bool WMTHostWsiIsForeground(void);
bool WMTHostWsiIsMinimized(void);

#ifdef __cplusplus
}
#endif

#endif /* WINEMETAL_HOST_H */
