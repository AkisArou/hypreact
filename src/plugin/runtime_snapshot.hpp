#pragma once

#include "domain/runtime_snapshot.hpp"

namespace hypreact::plugin {

class RuntimeSnapshotProvider {
  public:
    static hypreact::domain::RuntimeSnapshot current();
};

} // namespace hypreact::plugin
