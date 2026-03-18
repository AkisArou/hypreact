#pragma once

#include <string>

#if defined(HYPREACT_USE_REAL_HYPRLAND_HEADERS) && __has_include(<hyprland/src/plugins/PluginAPI.hpp>) && __has_include(<hyprland/src/version.h>)
#include <hyprland/src/plugins/PluginAPI.hpp>
#include <hyprland/src/version.h>
#include <hyprlang.hpp>
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

namespace Hyprlang {
using INT = long long;
using FLOAT = double;
using STRING = const char*;

class CConfigValue {
  public:
    CConfigValue() = default;
    explicit CConfigValue(INT value)
        : stringValue_(std::to_string(value)) {}
    explicit CConfigValue(FLOAT value)
        : stringValue_(std::to_string(value)) {}
    explicit CConfigValue(STRING value)
        : stringValue_(value != nullptr ? value : "") {}

    void* const* getDataStaticPtr() const {
        storage_ = stringValue_.c_str();
        return reinterpret_cast<void* const*>(&storage_);
    }

  private:
    mutable const char* storage_ = nullptr;
    std::string stringValue_;
};
} // namespace Hyprlang

namespace HyprlandAPI {
inline void addNotification(HANDLE, const std::string&, CHyprColor, int) {}
inline bool addConfigValue(HANDLE, const std::string&, const Hyprlang::CConfigValue&) {
    return true;
}
inline Hyprlang::CConfigValue* getConfigValue(HANDLE, const std::string&) {
    return nullptr;
}
}
#endif
