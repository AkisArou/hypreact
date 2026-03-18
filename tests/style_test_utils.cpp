#include "style_test_utils.hpp"

#include <fstream>
#include <sstream>

namespace hypreact::test {

style::StyleNodeContext styleContextFromResolvedPath(
    const layout::ResolvedNode& root,
    const std::vector<std::size_t>& path,
    std::vector<style::StyleNodeContext>& storage
) {
    storage.clear();
    const layout::ResolvedNode* current = &root;

    auto pushNode = [&](const layout::ResolvedNode& node) {
        storage.push_back(style::StyleNodeContext {
            .type = node.type,
            .id = node.id,
            .classes = node.classes,
            .parent = storage.empty() ? nullptr : &storage.back(),
            .window = node.primaryWindow,
            .focused = node.primaryWindow != nullptr && node.primaryWindow->focused,
            .focusedWithin = node.focusedWithin,
            .fullscreen = node.primaryWindow != nullptr && node.primaryWindow->fullscreen,
            .floating = node.primaryWindow != nullptr && node.primaryWindow->floating,
            .urgent = node.primaryWindow != nullptr && node.primaryWindow->urgent,
            .visible = node.visible,
            .specialWorkspace = node.specialWorkspace,
        });
    };

    pushNode(*current);
    for (const auto index : path) {
        current = &current->children[index];
        pushNode(*current);
    }

    return storage.back();
}

std::string readFixture(const std::string& relativePath) {
    std::ifstream input("/home/akisarou/projects/hypreact/tests/fixtures/" + relativePath);
    std::stringstream buffer;
    buffer << input.rdbuf();
    return buffer.str();
}

std::optional<nlohmann::json> parseJson(const std::string& json) {
    try {
        return nlohmann::json::parse(json);
    } catch (const nlohmann::json::parse_error&) {
        return std::nullopt;
    }
}

} // namespace hypreact::test
