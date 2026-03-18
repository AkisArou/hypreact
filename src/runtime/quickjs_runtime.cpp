#include "quickjs_runtime.hpp"

#include "quickjs.h"

#include <filesystem>
#include <fstream>
#include <sstream>
#include <utility>

namespace hypreact::runtime {

namespace fs = std::filesystem;

void QuickJsRuntime::freeRuntime(JSRuntime* runtime) {
    if (runtime != nullptr) {
        JS_FreeRuntime(runtime);
    }
}

void QuickJsRuntime::freeContext(JSContext* context) {
    if (context != nullptr) {
        JS_FreeContext(context);
    }
}

QuickJsRuntime::QuickJsRuntime(std::string configRoot)
    : runtime_(JS_NewRuntime(), &QuickJsRuntime::freeRuntime),
      context_(nullptr, &QuickJsRuntime::freeContext),
      configRoot_(std::move(configRoot)) {
    if (runtime_ != nullptr) {
        context_.reset(JS_NewContext(runtime_.get()));
    }

    if (isReady()) {
        JS_SetModuleLoaderFunc(runtime_.get(), &QuickJsRuntime::normalizeModule, &QuickJsRuntime::loadModule, this);
    }
}

QuickJsRuntime::~QuickJsRuntime() = default;

QuickJsRuntime::QuickJsRuntime(QuickJsRuntime&& other) noexcept
    : runtime_(std::move(other.runtime_)),
      context_(std::move(other.context_)) {}

QuickJsRuntime& QuickJsRuntime::operator=(QuickJsRuntime&& other) noexcept {
    if (this != &other) {
        context_ = std::move(other.context_);
        runtime_ = std::move(other.runtime_);
    }

    return *this;
}

bool QuickJsRuntime::isReady() const {
    return runtime_ != nullptr && context_ != nullptr;
}

std::string QuickJsRuntime::dependencySummary() const {
    return isReady() ? "QuickJS runtime ready" : "QuickJS runtime unavailable";
}

const std::string& QuickJsRuntime::configRoot() const {
    return configRoot_;
}

std::string QuickJsRuntime::resolveConfigPath(std::string_view specifier) const {
    if (specifier.empty()) {
        return {};
    }

    const fs::path candidate = fs::path(configRoot_.c_str()) / std::string(specifier);
    std::error_code error;
    const fs::path normalized = fs::weakly_canonical(candidate, error);
    return (error ? candidate.lexically_normal() : normalized).string();
}

char* QuickJsRuntime::normalizeModule(
    JSContext* context,
    const char* moduleBaseName,
    const char* moduleName,
    void* opaque
) {
    auto* runtime = static_cast<QuickJsRuntime*>(opaque);
    fs::path resolvedPath;

    if (runtime == nullptr || moduleName == nullptr) {
        return nullptr;
    }

    if (fs::path(moduleName).is_absolute()) {
        resolvedPath = fs::path(moduleName);
    } else if (moduleBaseName != nullptr && moduleBaseName[0] != '\0') {
        resolvedPath = fs::path(moduleBaseName).parent_path() / fs::path(moduleName);
    } else {
        resolvedPath = fs::path(runtime->configRoot_.c_str()) / fs::path(moduleName);
    }

    std::error_code error;
    const fs::path normalized = fs::weakly_canonical(resolvedPath, error);
    const auto normalizedString = (error ? resolvedPath.lexically_normal() : normalized).string();

    return js_strdup(context, normalizedString.c_str());
}

JSModuleDef* QuickJsRuntime::loadModule(JSContext* context, const char* moduleName, void* opaque) {
    auto* runtime = static_cast<QuickJsRuntime*>(opaque);

    if (runtime == nullptr || moduleName == nullptr) {
        JS_ThrowInternalError(context, "invalid module loader state");
        return nullptr;
    }

    std::ifstream input(moduleName);
    if (!input.is_open()) {
        JS_ThrowReferenceError(context, "unable to open module: %s", moduleName);
        return nullptr;
    }

    std::stringstream buffer;
    buffer << input.rdbuf();
    const std::string source = buffer.str();

    JSValue value = JS_Eval(
        context,
        source.c_str(),
        source.size(),
        moduleName,
        JS_EVAL_TYPE_MODULE | JS_EVAL_FLAG_COMPILE_ONLY
    );

    if (JS_IsException(value)) {
        return nullptr;
    }

    return static_cast<JSModuleDef*>(JS_VALUE_GET_PTR(value));
}

QuickJsEvalResult QuickJsRuntime::evalInternal(std::string_view source, std::string_view filename, int flags) const {
    if (!isReady()) {
        return {
            false,
            {},
            std::string("QuickJS runtime unavailable")
        };
    }

    const std::string filenameString(filename);

    JSValue result = JS_Eval(
        context_.get(),
        source.data(),
        source.size(),
        filenameString.c_str(),
        flags
    );

    if (JS_IsException(result)) {
        JSValue exception = JS_GetException(context_.get());
        const char* errorString = JS_ToCString(context_.get(), exception);
        std::string error = errorString != nullptr ? errorString : "unknown QuickJS exception";

        if (errorString != nullptr) {
            JS_FreeCString(context_.get(), errorString);
        }

        JS_FreeValue(context_.get(), exception);
        JS_FreeValue(context_.get(), result);

        return {
            false,
            {},
            error
        };
    }

    const char* valueString = JS_ToCString(context_.get(), result);
    std::string value = valueString != nullptr ? valueString : std::string {};

    if (valueString != nullptr) {
        JS_FreeCString(context_.get(), valueString);
    }

    JS_FreeValue(context_.get(), result);

    return {
        true,
        value,
        std::nullopt
    };
}

QuickJsEvalResult QuickJsRuntime::eval(std::string_view source, std::string_view filename) const {
    return evalInternal(source, filename, JS_EVAL_TYPE_GLOBAL);
}

QuickJsSmokeTestResult QuickJsRuntime::smokeTest() const {
    const auto result = eval("1 + 2", "<smoke-test>");
    if (!result.ok) {
        return {
            false,
            result.error.value_or("QuickJS smoke test failed")
        };
    }

    return {
        result.value == "3",
        "QuickJS eval(\"1 + 2\") => " + result.value
    };
}

} // namespace hypreact::runtime
