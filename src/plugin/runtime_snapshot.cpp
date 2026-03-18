#include "runtime_snapshot.hpp"

#include "hyprland_compat.hpp"

#include <format>

#if defined(HYPREACT_USE_REAL_HYPRLAND_HEADERS)
#include <hyprland/src/Compositor.hpp>
#include <hyprland/src/helpers/Monitor.hpp>
#include <hyprland/src/desktop/view/Window.hpp>
#endif

namespace hypreact::plugin {

hypreact::domain::RuntimeSnapshot RuntimeSnapshotProvider::current() {
    hypreact::domain::RuntimeSnapshot snapshot;

#if defined(HYPREACT_USE_REAL_HYPRLAND_HEADERS)
    if (!g_pCompositor) {
        return snapshot;
    }

    const auto monitor = g_pCompositor->getMonitorFromCursor();
    if (!monitor) {
        return snapshot;
    }

    snapshot.monitor.name = monitor->m_name;
    snapshot.monitor.width = static_cast<int>(monitor->m_size.x);
    snapshot.monitor.height = static_cast<int>(monitor->m_size.y);
    snapshot.monitor.x = static_cast<int>(monitor->m_position.x);
    snapshot.monitor.y = static_cast<int>(monitor->m_position.y);
    snapshot.monitor.scale = monitor->m_scale;
    snapshot.monitor.focused = true;

    const auto workspace = monitor->m_activeWorkspace;
    if (workspace) {
        snapshot.workspace.name = workspace->m_name;
        snapshot.workspace.id = workspace->m_id;
        snapshot.workspace.special = workspace->m_isSpecialWorkspace;
        snapshot.workspace.windowCount = workspace->getWindows();
        snapshot.workspace.visible.push_back(workspace->m_name);
    }

    for (const auto& window : g_pCompositor->m_windows) {
        if (!window || !window->m_workspace) {
            continue;
        }

        hypreact::domain::WindowSnapshot windowSnapshot;
        windowSnapshot.address = std::format("{:p}", static_cast<const void*>(window.get()));
        windowSnapshot.id = windowSnapshot.address;
        windowSnapshot.klass = window->m_class;
        windowSnapshot.title = window->m_title;
        windowSnapshot.initialClass = window->m_initialClass;
        windowSnapshot.initialTitle = window->m_initialTitle;
        windowSnapshot.workspace = window->m_workspace->m_name;
        windowSnapshot.monitor = window->m_workspace->m_monitor ? window->m_workspace->m_monitor->m_name : std::string {};
        windowSnapshot.floating = window->m_isFloating;
        windowSnapshot.fullscreen = window->isFullscreen();
        windowSnapshot.focused = g_pCompositor->isWindowActive(window);
        windowSnapshot.pinned = window->m_pinned;
        windowSnapshot.urgent = window->m_isUrgent;
        windowSnapshot.xwayland = window->m_isX11;

        snapshot.windows.push_back(windowSnapshot);

        if (windowSnapshot.focused) {
            snapshot.focusedWindow = windowSnapshot;
        }
    }
#endif

    return snapshot;
}

} // namespace hypreact::plugin
