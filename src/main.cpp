#include <stdexcept>
#include <string>

#if __has_include(<hyprland/src/plugins/PluginAPI.hpp>) && __has_include(<hyprland/src/version.h>)
#include <hyprland/src/plugins/PluginAPI.hpp>
#include <hyprland/src/version.h>
#else
#define APICALL
#define EXPORT

using HANDLE = void*;

struct CHyprColor {
    float r;
    float g;
    float b;
    float a;
};

struct PLUGIN_DESCRIPTION_INFO {
    std::string name;
    std::string description;
    std::string author;
    std::string version;
};

inline constexpr const char* HYPRLAND_API_VERSION = "unknown";

inline std::string __hyprland_api_get_hash() {
    return {};
}

inline std::string __hyprland_api_get_client_hash() {
    return {};
}

namespace HyprlandAPI {
inline void addNotification(HANDLE, const std::string&, CHyprColor, int) {}
}
#endif

APICALL EXPORT std::string PLUGIN_API_VERSION() { return HYPRLAND_API_VERSION; }

APICALL EXPORT PLUGIN_DESCRIPTION_INFO PLUGIN_INIT(HANDLE handle) {
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

APICALL EXPORT void PLUGIN_EXIT() {}
