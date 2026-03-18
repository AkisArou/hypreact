#pragma once

#include <memory>
#include <string>
#include <unordered_map>

#include "hyprland_compat.hpp"

namespace hypreact::runtime {
class QuickJsRuntime;
}

namespace hypreact::plugin {

struct PluginDescription {
    std::string name;
    std::string description;
    std::string author;
    std::string version;
};

class Plugin {
  public:
    static Plugin& instance();

    std::string apiVersion() const;
    PluginDescription init(HANDLE handle);
    void exit();

  private:
    Plugin() = default;

    std::unique_ptr<runtime::QuickJsRuntime> runtime_;
    std::unordered_map<std::string, std::string> workspaceLayouts_;
    std::unordered_map<std::string, std::string> monitorLayouts_;
    bool debugSelectorDiagnostics_ = false;
    bool debugSelectorDump_ = false;
    std::string debugDumpPath_;

    [[nodiscard]] static std::string detectConfigRoot(HANDLE handle);
    [[nodiscard]] static std::string detectCacheRoot(HANDLE handle);
    [[nodiscard]] static std::string detectSelectedLayout(HANDLE handle);
    [[nodiscard]] std::string resolveLayoutForSelection(const std::string& monitor, const std::string& workspace) const;

    static Hyprlang::CParseResult onWorkspaceLayoutKeyword(const char* command, const char* value);
    static Hyprlang::CParseResult onMonitorLayoutKeyword(const char* command, const char* value);
};

} // namespace hypreact::plugin
