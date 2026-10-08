/* Storage and seqlock for the native host window-system state; see winemetal_host.h. */
#include "../winemetal_host.h"
#include <string.h>

#define WMT_EXPORT __attribute__((visibility("default")))

static struct WMTHostWsiState g_state = {WMT_HOST_WSI_VERSION};
static uint32_t g_sequence;   /* odd while the host is writing */
static int32_t g_foreground;
static int32_t g_minimized;

WMT_EXPORT void WMTHostWsiBeginUpdate(void) {
  __atomic_add_fetch(&g_sequence, 1, __ATOMIC_ACQ_REL);
}

WMT_EXPORT struct WMTHostWsiState *WMTHostWsiMutableState(void) { return &g_state; }

WMT_EXPORT void WMTHostWsiEndUpdate(void) {
  g_state.version = WMT_HOST_WSI_VERSION;
  __atomic_add_fetch(&g_sequence, 1, __ATOMIC_ACQ_REL);
}

WMT_EXPORT void WMTHostWsiSetForeground(bool foreground) {
  __atomic_store_n(&g_foreground, foreground ? 1 : 0, __ATOMIC_RELEASE);
}

WMT_EXPORT void WMTHostWsiSetMinimized(bool minimized) {
  __atomic_store_n(&g_minimized, minimized ? 1 : 0, __ATOMIC_RELEASE);
}

WMT_EXPORT void WMTHostWsiCopy(struct WMTHostWsiState *out) {
  for (;;) {
    uint32_t before = __atomic_load_n(&g_sequence, __ATOMIC_ACQUIRE);
    if (before & 1)
      continue;
    memcpy(out, &g_state, sizeof(*out));
    __atomic_thread_fence(__ATOMIC_ACQUIRE);
    if (__atomic_load_n(&g_sequence, __ATOMIC_ACQUIRE) == before)
      return;
  }
}

WMT_EXPORT bool WMTHostWsiIsForeground(void) {
  return __atomic_load_n(&g_foreground, __ATOMIC_ACQUIRE) != 0;
}

WMT_EXPORT bool WMTHostWsiIsMinimized(void) {
  return __atomic_load_n(&g_minimized, __ATOMIC_ACQUIRE) != 0;
}
