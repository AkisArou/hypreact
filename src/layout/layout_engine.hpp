#pragma once

#include <string>

#include "domain/runtime_snapshot.hpp"
#include "geometry.hpp"
#include "resolver.hpp"

namespace hypreact::style {
struct Stylesheet;
}

namespace hypreact::layout {

class LayoutEngine {
  public:
    [[nodiscard]] static std::string dependencySummary();
    [[nodiscard]] static GeometryResult compute(
        const ResolutionResult& resolved,
        const domain::RuntimeSnapshot& snapshot,
        const style::Stylesheet& stylesheet
    );
};

} // namespace hypreact::layout
