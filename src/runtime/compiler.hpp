#pragma once

#include <optional>
#include <string>
#include <string_view>

namespace hypreact::runtime {

struct CompileResult {
    bool ok;
    std::string entryPath;
    std::optional<std::string> error;
};

class RuntimeCompiler {
  public:
    RuntimeCompiler(std::string configRoot, std::string cacheRoot);

    [[nodiscard]] const std::string& configRoot() const;
    [[nodiscard]] const std::string& cacheRoot() const;
    [[nodiscard]] CompileResult prepareLayout(std::string_view layoutName) const;

  private:
    std::string configRoot_;
    std::string cacheRoot_;

    [[nodiscard]] std::string authoredLayoutBase(std::string_view layoutName) const;
    [[nodiscard]] std::string compiledLayoutEntry(std::string_view layoutName) const;
};

} // namespace hypreact::runtime
