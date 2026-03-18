#include "plugin.hpp"

#include <cstdlib>
#include <filesystem>
#include <string_view>
#include <stdexcept>

#include "layout/layout_engine.hpp"
#include "layout/validator.hpp"
#include "layout/resolver.hpp"
#include "runtime_snapshot.hpp"
#include "runtime/quickjs_runtime.hpp"
#include "style/libcss_bridge.hpp"

#include <fstream>
#include <sstream>

namespace hypreact::plugin {

namespace {

const char* stringConfigValueOrNull(HANDLE handle, const std::string& name) {
    auto* value = HyprlandAPI::getConfigValue(handle, name);
    if (value == nullptr) {
        return nullptr;
    }

    const void* const* staticPtr = value->getDataStaticPtr();
    if (staticPtr == nullptr || *staticPtr == nullptr) {
        return nullptr;
    }

    return *reinterpret_cast<const char* const*>(staticPtr);
}

void registerConfigValue(HANDLE handle, const char* name, const Hyprlang::CConfigValue& value) {
    HyprlandAPI::addConfigValue(handle, std::string("plugin:hypreact:") + name, value);
}

Hyprlang::CParseResult parseLayoutAssignment(const char* value, std::string& key, std::string& layout) {
    Hyprlang::CParseResult result;

    if (value == nullptr) {
        result.setError("missing layout assignment");
        return result;
    }

    std::string_view input(value);
    const auto comma = input.find(',');
    if (comma == std::string_view::npos) {
        result.setError("expected '<target>,<layout>'");
        return result;
    }

    key = std::string(input.substr(0, comma));
    layout = std::string(input.substr(comma + 1));

    auto trim = [](std::string& text) {
        const auto start = text.find_first_not_of(" \t");
        const auto end = text.find_last_not_of(" \t");
        if (start == std::string::npos || end == std::string::npos) {
            text.clear();
            return;
        }
        text = text.substr(start, end - start + 1);
    };

    trim(key);
    trim(layout);

    if (key.empty() || layout.empty()) {
        result.setError("target and layout must both be non-empty");
        return result;
    }

    return result;
}

} // namespace

Plugin& Plugin::instance() {
    static Plugin plugin;
    return plugin;
}

std::string Plugin::detectConfigRoot(HANDLE handle) {
    if (const char* configured = stringConfigValueOrNull(handle, "plugin:hypreact:config_path"); configured != nullptr
        && configured[0] != '\0') {
        return configured;
    }

    if (const char* configured = std::getenv("HYPREACT_CONFIG"); configured != nullptr && configured[0] != '\0') {
        return configured;
    }

    if (const char* xdgConfigHome = std::getenv("XDG_CONFIG_HOME"); xdgConfigHome != nullptr && xdgConfigHome[0] != '\0') {
        return (std::filesystem::path(xdgConfigHome) / "hypreact").string();
    }

    if (const char* home = std::getenv("HOME"); home != nullptr && home[0] != '\0') {
        return (std::filesystem::path(home) / ".config" / "hypreact").string();
    }

    return "./.hypreact";
}

std::string Plugin::detectCacheRoot(HANDLE handle) {
    if (const char* configured = stringConfigValueOrNull(handle, "plugin:hypreact:cache_path"); configured != nullptr
        && configured[0] != '\0') {
        return configured;
    }

    if (const char* configured = std::getenv("HYPREACT_CACHE"); configured != nullptr && configured[0] != '\0') {
        return configured;
    }

    if (const char* xdgCacheHome = std::getenv("XDG_CACHE_HOME"); xdgCacheHome != nullptr && xdgCacheHome[0] != '\0') {
        return (std::filesystem::path(xdgCacheHome) / "hypreact").string();
    }

    if (const char* home = std::getenv("HOME"); home != nullptr && home[0] != '\0') {
        return (std::filesystem::path(home) / ".cache" / "hypreact").string();
    }

    return "./.hypreact-cache";
}

std::string Plugin::detectSelectedLayout(HANDLE handle) {
    if (const char* configured = stringConfigValueOrNull(handle, "plugin:hypreact:layout"); configured != nullptr
        && configured[0] != '\0') {
        return configured;
    }

    if (const char* configured = std::getenv("HYPREACT_LAYOUT"); configured != nullptr && configured[0] != '\0') {
        return configured;
    }

    return "first";
}

std::string Plugin::resolveLayoutForSelection(const std::string& monitor, const std::string& workspace) const {
    if (!monitor.empty()) {
        const auto monitorIt = monitorLayouts_.find(monitor);
        if (monitorIt != monitorLayouts_.end()) {
            return monitorIt->second;
        }
    }

    if (!workspace.empty()) {
        const auto workspaceIt = workspaceLayouts_.find(workspace);
        if (workspaceIt != workspaceLayouts_.end()) {
            return workspaceIt->second;
        }
    }

    return {};
}

Hyprlang::CParseResult Plugin::onWorkspaceLayoutKeyword(const char*, const char* value) {
    std::string workspace;
    std::string layout;
    auto result = parseLayoutAssignment(value, workspace, layout);
    if (result.error) {
        return result;
    }

    Plugin::instance().workspaceLayouts_[workspace] = layout;
    return result;
}

Hyprlang::CParseResult Plugin::onMonitorLayoutKeyword(const char*, const char* value) {
    std::string monitor;
    std::string layout;
    auto result = parseLayoutAssignment(value, monitor, layout);
    if (result.error) {
        return result;
    }

    Plugin::instance().monitorLayouts_[monitor] = layout;
    return result;
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

    workspaceLayouts_.clear();
    monitorLayouts_.clear();

    registerConfigValue(handle, "config_path", Hyprlang::CConfigValue(Hyprlang::STRING("")));
    registerConfigValue(handle, "cache_path", Hyprlang::CConfigValue(Hyprlang::STRING("")));
    registerConfigValue(handle, "layout", Hyprlang::CConfigValue(Hyprlang::STRING("first")));
    registerConfigValue(handle, "default_layout", Hyprlang::CConfigValue(Hyprlang::STRING("first")));
    HyprlandAPI::addConfigKeyword(handle, "plugin:hypreact:workspace_layout", &Plugin::onWorkspaceLayoutKeyword, Hyprlang::SHandlerOptions {});
    HyprlandAPI::addConfigKeyword(handle, "plugin:hypreact:monitor_layout", &Plugin::onMonitorLayoutKeyword, Hyprlang::SHandlerOptions {});

    const std::string configRoot = detectConfigRoot(handle);
    const std::string cacheRoot = detectCacheRoot(handle);
    const auto snapshot = RuntimeSnapshotProvider::current();
    std::string selectedLayout = detectSelectedLayout(handle);

    const char* forcedLayout = stringConfigValueOrNull(handle, "plugin:hypreact:layout");
    if (forcedLayout == nullptr || forcedLayout[0] == '\0') {
        const std::string mappedLayout = resolveLayoutForSelection(snapshot.monitor.name, snapshot.workspace.name);
        if (!mappedLayout.empty()) {
            selectedLayout = mappedLayout;
        } else if (const char* configuredDefault = stringConfigValueOrNull(handle, "plugin:hypreact:default_layout");
                   configuredDefault != nullptr && configuredDefault[0] != '\0') {
            selectedLayout = configuredDefault;
        }
    }

    runtime_ = std::make_unique<runtime::QuickJsRuntime>(configRoot, cacheRoot);
    const auto smokeTest = runtime_->smokeTest();

    if (!smokeTest.ok) {
        HyprlandAPI::addNotification(
            handle,
            "[hypreact] QuickJS smoke test failed: " + smokeTest.summary,
            CHyprColor {1.0, 0.5, 0.1, 1.0},
            10000
        );
        throw std::runtime_error("[hypreact] QuickJS smoke test failed");
    }

    HyprlandAPI::addNotification(
        handle,
        "[hypreact] " + smokeTest.summary,
        CHyprColor {0.2, 0.8, 0.4, 1.0},
        5000
    );

    if (!snapshot.monitor.name.empty() || !snapshot.workspace.name.empty()) {
        HyprlandAPI::addNotification(
            handle,
            "[hypreact] selection snapshot monitor='" + snapshot.monitor.name + "' workspace='" + snapshot.workspace.name + "'",
            CHyprColor {0.4, 0.7, 1.0, 1.0},
            4000
        );
    }

    HyprlandAPI::addNotification(
        handle,
        "[hypreact] config='" + configRoot + "' cache='" + cacheRoot + "'",
        CHyprColor {0.7, 0.7, 1.0, 1.0},
        4000
    );

    const auto layoutModule = runtime_->loadLayoutModule(selectedLayout);
    if (!layoutModule.ok) {
        HyprlandAPI::addNotification(
            handle,
            "[hypreact] failed to load layout module: " + layoutModule.error.value_or("unknown error"),
            CHyprColor {1.0, 0.5, 0.1, 1.0},
            10000
        );
    } else {
        HyprlandAPI::addNotification(
            handle,
            "[hypreact] loaded layout module: " + layoutModule.value,
            CHyprColor {0.2, 0.6, 1.0, 1.0},
            5000
        );
    }

    const auto layoutInvocation = runtime_->invokeLayout(selectedLayout, snapshot);
    if (!layoutInvocation.ok) {
        HyprlandAPI::addNotification(
            handle,
            "[hypreact] layout invocation failed: " + layoutInvocation.error.value_or("unknown error"),
            CHyprColor {1.0, 0.5, 0.1, 1.0},
            10000
        );
    } else {
        const auto& root = *layoutInvocation.root;
        const auto validation = hypreact::layout::LayoutValidator::validate(root);
        if (!validation.ok) {
            const auto& issue = validation.issues.front();
            HyprlandAPI::addNotification(
                handle,
                "[hypreact] layout validation failed at " + issue.path + ": " + issue.message,
                CHyprColor {1.0, 0.5, 0.1, 1.0},
                10000
            );
        } else {
            const auto resolved = hypreact::layout::LayoutResolver::resolve(root, snapshot);
            std::stringstream cssBuffer;
            if (std::ifstream globalCss(configRoot + "/index.css"); globalCss.is_open()) {
                cssBuffer << globalCss.rdbuf();
            }
            if (std::ifstream layoutCss(configRoot + "/layouts/" + selectedLayout + "/index.css"); layoutCss.is_open()) {
                cssBuffer << "\n" << layoutCss.rdbuf();
            }

            const auto parsedStylesheet = hypreact::style::parseStylesheetDetailed(cssBuffer.str());
            if (!parsedStylesheet.warnings.empty()) {
                HyprlandAPI::addNotification(
                    handle,
                    "[hypreact] stylesheet warning: " + parsedStylesheet.warnings.front().message,
                    CHyprColor {1.0, 0.8, 0.2, 1.0},
                    5000
                );
            }

            const hypreact::style::StyleNodeContext debugSelectorContext {
                .type = resolved.root.type,
                .id = resolved.root.id,
                .classes = resolved.root.classes,
                .window = resolved.root.primaryWindow,
                .focused = resolved.root.primaryWindow != nullptr && resolved.root.primaryWindow->focused,
                .focusedWithin = resolved.root.focusedWithin,
                .fullscreen = resolved.root.primaryWindow != nullptr && resolved.root.primaryWindow->fullscreen,
                .floating = resolved.root.primaryWindow != nullptr && resolved.root.primaryWindow->floating,
                .urgent = resolved.root.primaryWindow != nullptr && resolved.root.primaryWindow->urgent,
                .visible = resolved.root.visible,
                .specialWorkspace = resolved.root.specialWorkspace,
            };
            const auto selectorDebug = hypreact::style::libcssMatchSelector(hypreact::domain::toString(resolved.root.type), debugSelectorContext);
            if (!selectorDebug.matched && !selectorDebug.diagnostics.empty()) {
                HyprlandAPI::addNotification(
                    handle,
                    "[hypreact] selector diagnostic: " + selectorDebug.diagnostics.front(),
                    CHyprColor {0.9, 0.7, 0.2, 1.0},
                    3000
                );
            }

            const auto geometry = hypreact::layout::LayoutEngine::compute(resolved, snapshot, parsedStylesheet.stylesheet);
            HyprlandAPI::addNotification(
                handle,
                "[hypreact] layout root type='" + hypreact::domain::toString(root.type) + "' id='" + root.id + "' children="
                    + std::to_string(root.children.size()) + " unclaimed=" + std::to_string(resolved.unclaimedWindows.size())
                    + " box=" + std::to_string(static_cast<int>(geometry.root.box.width)) + "x"
                    + std::to_string(static_cast<int>(geometry.root.box.height)),
                CHyprColor {0.6, 0.8, 1.0, 1.0},
                4000
            );
        }
    }

    return {
        "hypreact",
        "QuickJS + Yoga layout plugin for Hyprland",
        "AkisArou",
        "0.1.0"
    };
}

void Plugin::exit() {
    runtime_.reset();
}

} // namespace hypreact::plugin
