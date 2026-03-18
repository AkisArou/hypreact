#pragma once

#include <cstddef>
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

} // namespace hypreact::test
