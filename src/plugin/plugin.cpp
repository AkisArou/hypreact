#include "plugin.hpp"

#include <stdexcept>

namespace hypreact::plugin {

Plugin& Plugin::instance() {
    static Plugin plugin;
    return plugin;
}

std::string Plugin::apiVersion() const {
    return HYPRLAND_API_VERSION;
}

PluginDescription Plugin::init(HANDLE handle) {
    const std::string compositorHash = __hyprland_api_get_hash();
    const std::string clientHash = __hyprland_api_get_client_hash();

    if (compositorHash != clientHash) {
        HyprlandAPI::addNotification(
            handle,
            "[hypreact] built for a different Hyprland version; refusing to load.",
            CHyprColor {1.0, 0.2, 0.2, 1.0},
            10000
        );
        throw std::runtime_error("[hypreact] target Hyprland version mismatch");
    }

    return {
        "hypreact",
        "QuickJS + Yoga layout plugin for Hyprland",
        "AkisArou",
        "0.1.0"
    };
}

void Plugin::exit() {}

} // namespace hypreact::plugin
