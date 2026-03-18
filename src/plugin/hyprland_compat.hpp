#pragma once

#include <string>

#if defined(HYPREACT_USE_REAL_HYPRLAND_HEADERS) && __has_include(<hyprland/src/plugins/PluginAPI.hpp>) && __has_include(<hyprland/src/version.h>)
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
