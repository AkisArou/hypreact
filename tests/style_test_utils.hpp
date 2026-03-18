#pragma once

#include <cstddef>
#include <nlohmann/json.hpp>
#include <optional>
#include <string>
#include <vector>

#include "layout/resolver.hpp"
#include "style/stylesheet.hpp"

namespace hypreact::test {

style::StyleNodeContext styleContextFromResolvedPath(
    const layout::ResolvedNode& root,
    const std::vector<std::size_t>& path,
    std::vector<style::StyleNodeContext>& storage
);

std::string readFixture(const std::string& relativePath);

using Json = nlohmann::json;

std::optional<Json> parseJson(const std::string& json);

} // namespace hypreact::test
