#pragma once

#include "config/config.hpp"

namespace dxmt {

/**
 * \brief DXGI options
 *
 * Per-app options that control the
 * behaviour of some DXGI classes.
 */
struct DxgiOptions {
  DxgiOptions(const Config &config);

  /// Override PCI vendor and device IDs reported to the
  /// application. This may make apps think they are running
  /// on a different GPU than they do and behave differently.
  int32_t customVendorId;
  int32_t customDeviceId;
  std::string customDeviceDesc;
  bool forceSDR;

  /// Upper bound, in bytes, for the video memory size and budget reported
  /// to the application. 0 keeps the value derived from the Metal device.
  uint64_t maxDeviceMemory;
};

} // namespace dxmt