#pragma once

#ifndef _WIN32

#include <atomic>
#include <windows.h>

#include <sched.h>
#include <unistd.h>

#include "log/log.hpp"

#define THREAD_PRIORITY_TIME_CRITICAL 15

inline HANDLE GetCurrentProcess() {
  dxmt::Logger::warn("GetCurrentProcess not implemented.");
  return nullptr;
}

inline HANDLE GetCurrentThread() {
  dxmt::Logger::warn("GetCurrentThread not implemented.");
  return nullptr;
}

inline DWORD GetCurrentProcessId() {
  dxmt::Logger::warn("GetCurrentProcessId not implemented.");
  return 0;
}

inline BOOL ProcessIdToSessionId(DWORD pid, DWORD *id) {
  dxmt::Logger::warn("ProcessIdToSessionId not implemented.");
  *id = 0;
  return FALSE;
}

inline BOOL SetThreadPriority(HANDLE hThread, int nPriority) {
  dxmt::Logger::warn("SetThreadPriority not implemented.");
  return FALSE;
}

inline HANDLE GetModuleHandle(LPCSTR lpModuleName) {
  dxmt::Logger::warn("GetModuleHandle not implemented.");
  return nullptr;
}

inline void ExitThread(DWORD dwExitCode) {
  dxmt::Logger::warn("ExitThread not implemented.");
}

#define WM_SIZE 0

inline LONG SendMessage(HWND hWnd, UINT Msg, UINT_PTR wParam, LONG_PTR lParam) {
  dxmt::Logger::warn("SendMessage not implemented.");
  return 0;
}

inline DWORD GetProcessId(HANDLE Process) {
  dxmt::Logger::warn("GetProcessId not implemented.");
  return 0;
}

inline DWORD GetWindowThreadProcessId(HWND hWnd, LPDWORD lpdwProcessId) {
  dxmt::Logger::warn("GetWindowThreadProcessId not implemented.");
  *lpdwProcessId = 0;
  return 0;
}

inline DWORD MAKELONG(WORD a, WORD b) {
  return (((DWORD)a) << 16) | ((DWORD)b);
}

typedef SECURITY_ATTRIBUTES *LPSECURITY_ATTRIBUTES;

// A counting semaphore as a plain object: the swapchain only counts present slots with it
// (the frame-latency waitable object is not exposed natively, so nothing waits on it).
struct NativeSemaphore {
  static constexpr uint32_t kMagic = 0x53454d41; // "SEMA"
  uint32_t magic = kMagic;
  std::atomic<LONG> count;
  LONG maximum;
};

inline HANDLE CreateSemaphore(LPSECURITY_ATTRIBUTES lpSemaphoreAttributes,
                              LONG                  lInitialCount,
                              LONG                  lMaximumCount,
                              LPCSTR                lpName) {
  auto *semaphore = new NativeSemaphore();
  semaphore->count = lInitialCount;
  semaphore->maximum = lMaximumCount;
  return (HANDLE)semaphore;
}

inline BOOL ReleaseSemaphore(HANDLE hSemaphore,
                             LONG   lReleaseCount,
                             LPLONG lpPreviousCount) {
  auto *semaphore = (NativeSemaphore *)hSemaphore;
  if (!semaphore || semaphore->magic != NativeSemaphore::kMagic)
    return FALSE;
  LONG previous = semaphore->count.load();
  LONG next;
  do {
    next = previous + lReleaseCount;
    if (next > semaphore->maximum)
      return FALSE;
  } while (!semaphore->count.compare_exchange_weak(previous, next));
  if (lpPreviousCount)
    *lpPreviousCount = previous;
  return TRUE;
}

inline BOOL CloseHandle(HANDLE hObject) {
  auto *semaphore = (NativeSemaphore *)hObject;
  if (!semaphore || semaphore->magic != NativeSemaphore::kMagic) {
    dxmt::Logger::warn("CloseHandle: not a handle this layer created.");
    return FALSE;
  }
  semaphore->magic = 0;
  delete semaphore;
  return TRUE;
}

inline BOOL DuplicateHandle(HANDLE   hSourceProcessHandle,
                            HANDLE   hSourceHandle,
                            HANDLE   hTargetProcessHandle,
                            LPHANDLE lpTargetHandle,
                            DWORD    dwDesiredAccess,
                            BOOL     bInheritHandle,
                            DWORD    dwOptions)
{
  dxmt::Logger::warn("DuplicateHandle not implemented.");
  return FALSE;
}


inline VOID Sleep(DWORD dwMilliseconds) {
  usleep(useconds_t(dwMilliseconds) * 1000);
}

inline BOOL SwitchToThread() {
  return sched_yield() == 0;
}

#define ARRAYSIZE(a) (sizeof(a)/sizeof(*(a)))

#endif