#include "util_string.hpp"
#include <version.h>

namespace dxmt {

std::string
GetVersionDescriptionText([[maybe_unused]] uint32_t ApiVersion, uint32_t FeatureLevel) {
  std::string feature_level_text = "UNKNOWN";
  switch (FeatureLevel) {
  case 0x9100:
    feature_level_text = "9_1";
    break;
  case 0x9200:
    feature_level_text = "9_2";
    break;
  case 0x9300:
    feature_level_text = "9_3";
    break;
  case 0xa000:
    feature_level_text = "10_0";
    break;
  case 0xa100:
    feature_level_text = "10_1";
    break;
  case 0xb000:
    feature_level_text = "11_0";
    break;
  case 0xb100:
    feature_level_text = "11_1";
    break;
  case 0xc000:
    feature_level_text = "12_0";
    break;
  case 0xc100:
    feature_level_text = "12_1";
    break;
  }
  // The Metal HUD shows this line; macOS 26+ cuts long metric values, so keep it to the
  // feature level and the release tag.
  return str::format("FL_", feature_level_text, " ", DXMT_VERSION);
}

} // namespace dxmt
