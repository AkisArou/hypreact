#include "layout_node.hpp"

namespace hypreact::domain {

std::string toString(LayoutNodeType type) {
    switch (type) {
        case LayoutNodeType::Workspace: return "workspace";
        case LayoutNodeType::Group: return "group";
        case LayoutNodeType::Window: return "window";
        case LayoutNodeType::Slot: return "slot";
    }

    return "unknown";
}

} // namespace hypreact::domain
