#pragma once

#include <string>

#include "hyprland_compat.hpp"

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
};

} // namespace hypreact::plugin
