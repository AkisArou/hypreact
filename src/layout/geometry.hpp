#pragma once

#include <vector>

#include "resolver.hpp"

namespace hypreact::layout {

struct Box {
    float x = 0.0f;
    float y = 0.0f;
    float width = 0.0f;
    float height = 0.0f;
};

struct ComputedNode {
    ResolvedNode node;
    Box box;
    std::vector<ComputedNode> children;
};

struct GeometryResult {
    ComputedNode root;
};

} // namespace hypreact::layout
