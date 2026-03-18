#include "resolver.hpp"

#include <sstream>

namespace hypreact::layout {

namespace {

std::vector<std::string> splitClauses(const std::string& match) {
    std::vector<std::string> clauses;
    std::stringstream stream(match);
    std::string clause;
    while (stream >> clause) {
        clauses.push_back(clause);
    }
    return clauses;
}

std::optional<std::pair<std::string, std::string>> parseClause(const std::string& clause) {
    const auto equals = clause.find('=');
    if (equals == std::string::npos) {
        return std::nullopt;
    }

    std::string key = clause.substr(0, equals);
    std::string value = clause.substr(equals + 1);
    if (value.size() >= 2 && value.front() == '"' && value.back() == '"') {
        value = value.substr(1, value.size() - 2);
    }
    return std::pair {key, value};
}

std::string_view windowField(const domain::WindowSnapshot& window, const std::string& key) {
    if (key == "class") return window.klass;
    if (key == "title") return window.title;
    if (key == "app_id") return window.initialClass;
    if (key == "instance") return window.address;
    if (key == "shell") return window.xwayland ? "xwayland" : "wayland";
    if (key == "window_type") return window.floating ? "floating" : "tiled";
    return {};
}

} // namespace

ResolutionResult LayoutResolver::resolve(const domain::LayoutNode& root, const domain::RuntimeSnapshot& snapshot) {
    std::unordered_set<std::string> claimed;
    ResolvedNode resolvedRoot = resolveNode(root, snapshot, claimed);

    std::vector<const domain::WindowSnapshot*> unclaimed;
    for (const auto& window : snapshot.windows) {
        if (!claimed.contains(window.id)) {
            unclaimed.push_back(&window);
        }
    }

    return {
        std::move(resolvedRoot),
        std::move(unclaimed),
    };
}

ResolvedNode LayoutResolver::resolveNode(
    const domain::LayoutNode& node,
    const domain::RuntimeSnapshot& snapshot,
    std::unordered_set<std::string>& claimed
) {
    ResolvedNode resolved {
        .type = node.type,
        .id = node.id,
        .classes = node.classes,
        .visible = node.type != domain::LayoutNodeType::Window,
        .specialWorkspace = snapshot.workspace.special,
    };

    if (node.type == domain::LayoutNodeType::Window || node.type == domain::LayoutNodeType::Slot) {
        resolved.claimedWindows = claimMatches(node, snapshot, claimed);
        if (!resolved.claimedWindows.empty()) {
            resolved.primaryWindow = resolved.claimedWindows.front();
            resolved.visible = resolved.primaryWindow->workspace == snapshot.workspace.name;
        }
    }

    for (const auto& child : node.children) {
        resolved.children.push_back(resolveNode(child, snapshot, claimed));
    }

    resolved.focusedWithin = resolved.primaryWindow != nullptr && resolved.primaryWindow->focused;
    for (const auto& child : resolved.children) {
        if (child.focusedWithin) {
            resolved.focusedWithin = true;
            break;
        }
    }

    return resolved;
}

std::vector<const domain::WindowSnapshot*> LayoutResolver::claimMatches(
    const domain::LayoutNode& node,
    const domain::RuntimeSnapshot& snapshot,
    std::unordered_set<std::string>& claimed
) {
    std::vector<const domain::WindowSnapshot*> matches;
    const int limit = node.type == domain::LayoutNodeType::Window ? 1 : node.take.value_or(-1);

    for (const auto& window : snapshot.windows) {
        if (claimed.contains(window.id)) {
            continue;
        }

        if (!windowMatches(window, node.match)) {
            continue;
        }

        matches.push_back(&window);
        claimed.insert(window.id);

        if (limit > 0 && static_cast<int>(matches.size()) >= limit) {
            break;
        }
    }

    return matches;
}

bool LayoutResolver::windowMatches(const domain::WindowSnapshot& window, const std::optional<std::string>& match) {
    if (!match.has_value() || match->empty()) {
        return true;
    }

    for (const auto& clause : splitClauses(*match)) {
        const auto parsed = parseClause(clause);
        if (!parsed.has_value()) {
            return false;
        }

        const auto& [key, value] = *parsed;
        if (windowField(window, key) != value) {
            return false;
        }
    }

    return true;
}

} // namespace hypreact::layout
