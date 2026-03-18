#pragma once

#include <optional>
#include <string>
#include <vector>

namespace hypreact::domain {

struct MonitorSnapshot {
    std::string name;
    int width = 0;
    int height = 0;
    int x = 0;
    int y = 0;
    double scale = 1.0;
    bool focused = false;
};

struct WorkspaceSnapshot {
    std::string name;
    int id = -1;
    bool special = false;
    std::vector<std::string> visible;
    int windowCount = 0;
};

struct WindowSnapshot {
    std::string id;
    std::string address;
    std::string klass;
    std::string title;
    std::string initialClass;
    std::string initialTitle;
    std::string workspace;
    std::string monitor;
    bool floating = false;
    bool fullscreen = false;
    bool focused = false;
    bool pinned = false;
    bool urgent = false;
    bool xwayland = false;
};

struct RuntimeSnapshot {
    MonitorSnapshot monitor;
    WorkspaceSnapshot workspace;
    std::vector<WindowSnapshot> windows;
    std::optional<WindowSnapshot> focusedWindow;
};

} // namespace hypreact::domain
