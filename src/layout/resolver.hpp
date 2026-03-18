#pragma once

#include <optional>
#include <string>
#include <unordered_set>
#include <vector>

#include "domain/layout_node.hpp"
#include "domain/runtime_snapshot.hpp"

namespace hypreact::layout {

struct ResolvedNode {
    domain::LayoutNodeType type;
    std::string id;
    std::vector<std::string> classes;
    const domain::WindowSnapshot* primaryWindow = nullptr;
    bool focusedWithin = false;
    bool visible = false;
    bool specialWorkspace = false;
    std::vector<const domain::WindowSnapshot*> claimedWindows;
    std::vector<ResolvedNode> children;
};

struct ResolutionResult {
    ResolvedNode root;
    std::vector<const domain::WindowSnapshot*> unclaimedWindows;
};

class LayoutResolver {
  public:
    [[nodiscard]] static ResolutionResult resolve(const domain::LayoutNode& root, const domain::RuntimeSnapshot& snapshot);

  private:
    static ResolvedNode resolveNode(
        const domain::LayoutNode& node,
        const domain::RuntimeSnapshot& snapshot,
        std::unordered_set<std::string>& claimed
    );

    static std::vector<const domain::WindowSnapshot*> claimMatches(
        const domain::LayoutNode& node,
        const domain::RuntimeSnapshot& snapshot,
        std::unordered_set<std::string>& claimed
    );

    static bool windowMatches(const domain::WindowSnapshot& window, const std::optional<std::string>& match);
};

} // namespace hypreact::layout
