#pragma once

#include <memory>
#include <optional>
#include <string_view>
#include <string>

struct JSContext;
struct JSModuleDef;
struct JSRuntime;

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

class QuickJsRuntime {
  public:
    explicit QuickJsRuntime(std::string configRoot = {});
    ~QuickJsRuntime();

    QuickJsRuntime(const QuickJsRuntime&) = delete;
    QuickJsRuntime& operator=(const QuickJsRuntime&) = delete;

    QuickJsRuntime(QuickJsRuntime&& other) noexcept;
    QuickJsRuntime& operator=(QuickJsRuntime&& other) noexcept;

    [[nodiscard]] bool isReady() const;
    [[nodiscard]] std::string dependencySummary() const;
    [[nodiscard]] const std::string& configRoot() const;
    [[nodiscard]] std::string resolveConfigPath(std::string_view specifier) const;
    [[nodiscard]] QuickJsEvalResult eval(std::string_view source, std::string_view filename = "<eval>") const;
    [[nodiscard]] QuickJsSmokeTestResult smokeTest() const;

  private:
    std::unique_ptr<JSRuntime, void (*)(JSRuntime*)> runtime_;
    std::unique_ptr<JSContext, void (*)(JSContext*)> context_;
    std::string configRoot_;

    static void freeRuntime(JSRuntime* runtime);
    static void freeContext(JSContext* context);
    static char* normalizeModule(JSContext* context, const char* moduleBaseName, const char* moduleName, void* opaque);
    static JSModuleDef* loadModule(JSContext* context, const char* moduleName, void* opaque);
    [[nodiscard]] QuickJsEvalResult evalInternal(std::string_view source, std::string_view filename, int flags) const;
};

} // namespace hypreact::runtime
