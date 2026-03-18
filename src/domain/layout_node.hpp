#pragma once

#include <string>
#include <vector>

namespace hypreact::domain {

enum class LayoutNodeType {
    Workspace,
    Group,
    Window,
    Slot,
};

struct LayoutNode {
    LayoutNodeType type;
    std::string id;
    std::vector<std::string> classes;
};

} // namespace hypreact::domain
