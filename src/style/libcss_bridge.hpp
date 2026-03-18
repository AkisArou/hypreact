#pragma once

#include <string>

namespace hypreact::style {

struct LibcssBridgeStatus {
    bool available;
    std::string detail;
};

LibcssBridgeStatus libcssBridgeStatus();

} // namespace hypreact::style
