#pragma once

#include <optional>
#include <string>
#include <vector>

#include "domain/layout_node.hpp"

namespace hypreact::layout {

struct ValidationIssue {
    std::string path;
    std::string message;
};

struct ValidationResult {
    bool ok;
    std::vector<ValidationIssue> issues;
};

class LayoutValidator {
  public:
    [[nodiscard]] static ValidationResult validate(const domain::LayoutNode& root);

  private:
    static void validateNode(
        const domain::LayoutNode& node,
        const std::string& path,
        bool isRoot,
        std::vector<ValidationIssue>& issues
    );
};

} // namespace hypreact::layout
