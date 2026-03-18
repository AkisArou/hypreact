#include "validator.hpp"

namespace hypreact::layout {

namespace {

bool isContainer(domain::LayoutNodeType type) {
    return type == domain::LayoutNodeType::Workspace || type == domain::LayoutNodeType::Group;
}

bool isAllowedChild(domain::LayoutNodeType parent, domain::LayoutNodeType child) {
    switch (parent) {
        case domain::LayoutNodeType::Workspace:
        case domain::LayoutNodeType::Group:
            return child == domain::LayoutNodeType::Group || child == domain::LayoutNodeType::Window || child == domain::LayoutNodeType::Slot;
        case domain::LayoutNodeType::Window:
            return false;
        case domain::LayoutNodeType::Slot:
            return false;
    }

    return false;
}

std::string childPath(const std::string& path, const domain::LayoutNode& node, std::size_t index) {
    std::string next = path + "/" + domain::toString(node.type);
    if (!node.id.empty()) {
        next += "#" + node.id;
    } else {
        next += "[" + std::to_string(index) + "]";
    }
    return next;
}

} // namespace

ValidationResult LayoutValidator::validate(const domain::LayoutNode& root) {
    std::vector<ValidationIssue> issues;
    validateNode(root, "root", true, issues);
    return {
        issues.empty(),
        std::move(issues),
    };
}

void LayoutValidator::validateNode(
    const domain::LayoutNode& node,
    const std::string& path,
    bool isRoot,
    std::vector<ValidationIssue>& issues
) {
    if (isRoot && node.type != domain::LayoutNodeType::Workspace) {
        issues.push_back({path, "root element must be a workspace"});
    }

    if (!isRoot && node.type == domain::LayoutNodeType::Workspace) {
        issues.push_back({path, "workspace nodes may only appear at the root"});
    }

    if ((node.type == domain::LayoutNodeType::Window || node.type == domain::LayoutNodeType::Slot) && !node.children.empty()) {
        issues.push_back({path, std::string(domain::toString(node.type)) + " nodes cannot have children"});
    }

    if (node.type != domain::LayoutNodeType::Window && node.match.has_value()) {
        issues.push_back({path, "only window nodes may define match"});
    }

    if (node.type != domain::LayoutNodeType::Slot && node.take.has_value()) {
        issues.push_back({path, "only slot nodes may define take"});
    }

    if (node.type == domain::LayoutNodeType::Slot && node.take.has_value() && *node.take <= 0) {
        issues.push_back({path, "slot take must be a positive integer"});
    }

    if (!isContainer(node.type) && !node.children.empty()) {
        return;
    }

    for (std::size_t index = 0; index < node.children.size(); ++index) {
        const auto& child = node.children[index];
        const std::string nextPath = childPath(path, child, index);

        if (!isAllowedChild(node.type, child.type)) {
            issues.push_back({
                nextPath,
                std::string(domain::toString(node.type)) + " cannot contain " + domain::toString(child.type),
            });
        }

        validateNode(child, nextPath, false, issues);
    }
}

} // namespace hypreact::layout
