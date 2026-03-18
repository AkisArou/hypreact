#pragma once

#include <string>

#include "libcss_bridge.hpp"

namespace hypreact::style {

LibcssSelectionProbe probeLibcssSelection(const std::string& source, const StyleNodeContext& node);

} // namespace hypreact::style
