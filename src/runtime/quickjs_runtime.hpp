#pragma once

#include <memory>
#include <optional>
#include <string_view>
#include <string>

#include "compiler.hpp"
#include "domain/layout_node.hpp"
#include "domain/runtime_snapshot.hpp"

#include "quickjs.h"

namespace hypreact::runtime {

struct QuickJsEvalResult {
    bool ok;
    std::string value;
    std::optional<std::string> error;
};

struct QuickJsSmokeTestResult {
    bool ok;
    std::string summary;
};

struct QuickJsLayoutResult {
    bool ok;
    std::optional<domain::LayoutNode> root;
    std::optional<std::string> debug;
    std::optional<std::string> error;
};

class QuickJsRuntime {
  public:
    explicit QuickJsRuntime(std::string configRoot = {}, std::string cacheRoot = {});
    ~QuickJsRuntime();

    QuickJsRuntime(const QuickJsRuntime&) = delete;
    QuickJsRuntime& operator=(const QuickJsRuntime&) = delete;

    QuickJsRuntime(QuickJsRuntime&& other) noexcept;
    QuickJsRuntime& operator=(QuickJsRuntime&& other) noexcept;

    [[nodiscard]] bool isReady() const;
    [[nodiscard]] std::string dependencySummary() const;
    [[nodiscard]] const std::string& configRoot() const;
    [[nodiscard]] const std::string& cacheRoot() const;
    [[nodiscard]] std::string resolveConfigPath(std::string_view specifier) const;
    [[nodiscard]] std::string resolveCachePath(std::string_view specifier) const;
    [[nodiscard]] QuickJsEvalResult eval(std::string_view source, std::string_view filename = "<eval>") const;
    [[nodiscard]] QuickJsEvalResult loadLayoutModule(std::string_view layoutName) const;
    [[nodiscard]] QuickJsLayoutResult invokeLayout(std::string_view layoutName, const hypreact::domain::RuntimeSnapshot& snapshot) const;
    [[nodiscard]] QuickJsSmokeTestResult smokeTest() const;

  private:
    std::unique_ptr<JSRuntime, void (*)(JSRuntime*)> runtime_;
    std::unique_ptr<JSContext, void (*)(JSContext*)> context_;
    RuntimeCompiler compiler_;
    std::string configRoot_;
    std::string cacheRoot_;

    static void freeRuntime(JSRuntime* runtime);
    static void freeContext(JSContext* context);
    static char* normalizeModule(JSContext* context, const char* moduleBaseName, const char* moduleName, void* opaque);
    static JSModuleDef* loadModule(JSContext* context, const char* moduleName, void* opaque);
    static JSValue runtimeSnapshotToJs(JSContext* context, const domain::RuntimeSnapshot& snapshot);
    static std::optional<domain::LayoutNodeType> parseNodeType(JSContext* context, JSValueConst value);
    static std::optional<std::vector<std::string>> parseClasses(JSContext* context, JSValueConst value);
    static std::optional<std::vector<domain::LayoutNode>> parseChildren(JSContext* context, JSValueConst value);
    static std::optional<domain::LayoutNode> parseLayoutNode(JSContext* context, JSValueConst value);
    [[nodiscard]] std::string resolveLayoutEntryPath(std::string_view layoutName) const;
    [[nodiscard]] static bool isVirtualModule(std::string_view moduleName);
    [[nodiscard]] static std::string_view virtualModuleSource(std::string_view moduleName);
    [[nodiscard]] QuickJsEvalResult evalInternal(std::string_view source, std::string_view filename, int flags) const;
};

} // namespace hypreact::runtime
