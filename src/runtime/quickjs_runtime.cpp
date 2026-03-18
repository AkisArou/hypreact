#include "quickjs_runtime.hpp"

#include <filesystem>
#include <fstream>
#include <sstream>
#include <utility>

namespace hypreact::runtime {

namespace fs = std::filesystem;

namespace {

constexpr std::string_view HYPREACT_JSX_RUNTIME_MODULE = "hypreact/jsx-runtime";
constexpr std::string_view HYPREACT_JSX_RUNTIME_SOURCE = R"JS(
const flattenChildren = (input, out) => {
  for (const child of input) {
    if (Array.isArray(child)) {
      flattenChildren(child, out);
      continue;
    }
    if (child === false || child === null || child === undefined) {
      continue;
    }
    out.push(child);
  }
};

export const Fragment = Symbol("hypreact.fragment");

function createNode(type, props, ...children) {
  const normalizedChildren = [];
  const nextProps = props || {};

  flattenChildren(children, normalizedChildren);
  if (type === Fragment) {
    return normalizedChildren;
  }
  if (typeof type === "function") {
    return type({
      ...nextProps,
      children: normalizedChildren,
    });
  }
  return {
    type,
    props: nextProps,
    children: normalizedChildren,
  };
}

export function hr(type, props, ...children) {
  return createNode(type, props, ...children);
}

export function jsx(type, props, key) {
  const nextProps = props || {};
  const children = Object.prototype.hasOwnProperty.call(nextProps, "children")
    ? [nextProps.children]
    : [];
  const runtimeProps = { ...nextProps };

  void key;
  delete runtimeProps.children;
  return createNode(type, runtimeProps, ...children);
}

export function jsxs(type, props, key) {
  return jsx(type, props, key);
}
)JS";

} // namespace

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

QuickJsRuntime::QuickJsRuntime(std::string configRoot, std::string cacheRoot)
    : runtime_(JS_NewRuntime(), &QuickJsRuntime::freeRuntime),
      context_(nullptr, &QuickJsRuntime::freeContext),
      compiler_(configRoot, cacheRoot),
      configRoot_(std::move(configRoot)),
      cacheRoot_(std::move(cacheRoot)) {
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
      context_(std::move(other.context_)),
      compiler_(std::move(other.compiler_)),
      configRoot_(std::move(other.configRoot_)),
      cacheRoot_(std::move(other.cacheRoot_)) {}

QuickJsRuntime& QuickJsRuntime::operator=(QuickJsRuntime&& other) noexcept {
    if (this != &other) {
        context_ = std::move(other.context_);
        runtime_ = std::move(other.runtime_);
        compiler_ = std::move(other.compiler_);
        configRoot_ = std::move(other.configRoot_);
        cacheRoot_ = std::move(other.cacheRoot_);
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

const std::string& QuickJsRuntime::cacheRoot() const {
    return cacheRoot_;
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

std::string QuickJsRuntime::resolveCachePath(std::string_view specifier) const {
    if (specifier.empty()) {
        return {};
    }

    const fs::path candidate = fs::path(cacheRoot_.c_str()) / std::string(specifier);
    std::error_code error;
    const fs::path normalized = fs::weakly_canonical(candidate, error);
    return (error ? candidate.lexically_normal() : normalized).string();
}

std::string QuickJsRuntime::resolveLayoutEntryPath(std::string_view layoutName) const {
    const auto prepared = compiler_.prepareLayout(layoutName);
    return prepared.ok ? prepared.entryPath : std::string {};
}

bool QuickJsRuntime::isVirtualModule(std::string_view moduleName) {
    return moduleName == HYPREACT_JSX_RUNTIME_MODULE;
}

std::string_view QuickJsRuntime::virtualModuleSource(std::string_view moduleName) {
    if (moduleName == HYPREACT_JSX_RUNTIME_MODULE) {
        return HYPREACT_JSX_RUNTIME_SOURCE;
    }

    return {};
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

    if (isVirtualModule(moduleName)) {
        return js_strdup(context, std::string(moduleName).c_str());
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

    if (isVirtualModule(moduleName)) {
        const auto source = virtualModuleSource(moduleName);
        JSValue value = JS_Eval(
            context,
            source.data(),
            source.size(),
            moduleName,
            JS_EVAL_TYPE_MODULE | JS_EVAL_FLAG_COMPILE_ONLY
        );

        if (JS_IsException(value)) {
            return nullptr;
        }

        return static_cast<JSModuleDef*>(JS_VALUE_GET_PTR(value));
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

QuickJsEvalResult QuickJsRuntime::loadLayoutModule(std::string_view layoutName) const {
    if (layoutName.empty()) {
        return {
            false,
            {},
            std::string("layout name is empty")
        };
    }

    const auto prepared = compiler_.prepareLayout(layoutName);
    if (!prepared.ok) {
        return {
            false,
            {},
            prepared.error
        };
    }

    const std::string& entryPath = prepared.entryPath;

    std::ifstream input(entryPath);
    if (!input.is_open()) {
        return {
            false,
            {},
            std::string("could not open ") + entryPath
        };
    }

    std::stringstream buffer;
    buffer << input.rdbuf();
    const std::string source = buffer.str();
    const auto compiled = evalInternal(source, entryPath, JS_EVAL_TYPE_MODULE | JS_EVAL_FLAG_COMPILE_ONLY);

    if (!compiled.ok) {
        return compiled;
    }

    return {
        true,
        entryPath,
        std::nullopt
    };
}

JSValue QuickJsRuntime::runtimeSnapshotToJs(JSContext* context, const domain::RuntimeSnapshot& snapshot) {
    JSValue root = JS_NewObject(context);

    JSValue monitor = JS_NewObject(context);
    JS_SetPropertyStr(context, monitor, "name", JS_NewString(context, snapshot.monitor.name.c_str()));
    JS_SetPropertyStr(context, monitor, "width", JS_NewInt32(context, snapshot.monitor.width));
    JS_SetPropertyStr(context, monitor, "height", JS_NewInt32(context, snapshot.monitor.height));
    JS_SetPropertyStr(context, monitor, "x", JS_NewInt32(context, snapshot.monitor.x));
    JS_SetPropertyStr(context, monitor, "y", JS_NewInt32(context, snapshot.monitor.y));
    JS_SetPropertyStr(context, monitor, "scale", JS_NewFloat64(context, snapshot.monitor.scale));
    JS_SetPropertyStr(context, monitor, "focused", JS_NewBool(context, snapshot.monitor.focused));
    JS_SetPropertyStr(context, root, "monitor", monitor);

    JSValue workspace = JS_NewObject(context);
    JS_SetPropertyStr(context, workspace, "name", JS_NewString(context, snapshot.workspace.name.c_str()));
    JS_SetPropertyStr(context, workspace, "id", JS_NewInt32(context, snapshot.workspace.id));
    JS_SetPropertyStr(context, workspace, "special", JS_NewBool(context, snapshot.workspace.special));
    JS_SetPropertyStr(context, workspace, "windowCount", JS_NewInt32(context, snapshot.workspace.windowCount));

    JSValue visible = JS_NewArray(context);
    for (uint32_t index = 0; index < snapshot.workspace.visible.size(); ++index) {
        JS_SetPropertyUint32(context, visible, index, JS_NewString(context, snapshot.workspace.visible[index].c_str()));
    }
    JS_SetPropertyStr(context, workspace, "visible", visible);
    JS_SetPropertyStr(context, root, "workspace", workspace);

    JSValue windows = JS_NewArray(context);
    for (uint32_t index = 0; index < snapshot.windows.size(); ++index) {
        const auto& windowSnapshot = snapshot.windows[index];
        JSValue window = JS_NewObject(context);
        JS_SetPropertyStr(context, window, "id", JS_NewString(context, windowSnapshot.id.c_str()));
        JS_SetPropertyStr(context, window, "address", JS_NewString(context, windowSnapshot.address.c_str()));
        JS_SetPropertyStr(context, window, "class", JS_NewString(context, windowSnapshot.klass.c_str()));
        JS_SetPropertyStr(context, window, "title", JS_NewString(context, windowSnapshot.title.c_str()));
        JS_SetPropertyStr(context, window, "initialClass", JS_NewString(context, windowSnapshot.initialClass.c_str()));
        JS_SetPropertyStr(context, window, "initialTitle", JS_NewString(context, windowSnapshot.initialTitle.c_str()));
        JS_SetPropertyStr(context, window, "workspace", JS_NewString(context, windowSnapshot.workspace.c_str()));
        JS_SetPropertyStr(context, window, "monitor", JS_NewString(context, windowSnapshot.monitor.c_str()));
        JS_SetPropertyStr(context, window, "floating", JS_NewBool(context, windowSnapshot.floating));
        JS_SetPropertyStr(context, window, "fullscreen", JS_NewBool(context, windowSnapshot.fullscreen));
        JS_SetPropertyStr(context, window, "focused", JS_NewBool(context, windowSnapshot.focused));
        JS_SetPropertyStr(context, window, "pinned", JS_NewBool(context, windowSnapshot.pinned));
        JS_SetPropertyStr(context, window, "urgent", JS_NewBool(context, windowSnapshot.urgent));
        JS_SetPropertyStr(context, window, "xwayland", JS_NewBool(context, windowSnapshot.xwayland));
        JS_SetPropertyUint32(context, windows, index, window);
    }
    JS_SetPropertyStr(context, root, "windows", windows);

    if (snapshot.focusedWindow.has_value()) {
        JSValue focusedWindow = JS_NewObject(context);
        JS_SetPropertyStr(context, focusedWindow, "id", JS_NewString(context, snapshot.focusedWindow->id.c_str()));
        JS_SetPropertyStr(context, focusedWindow, "address", JS_NewString(context, snapshot.focusedWindow->address.c_str()));
        JS_SetPropertyStr(context, focusedWindow, "class", JS_NewString(context, snapshot.focusedWindow->klass.c_str()));
        JS_SetPropertyStr(context, focusedWindow, "title", JS_NewString(context, snapshot.focusedWindow->title.c_str()));
        JS_SetPropertyStr(context, root, "focusedWindow", focusedWindow);
    } else {
        JS_SetPropertyStr(context, root, "focusedWindow", JS_NULL);
    }

    return root;
}

std::optional<domain::LayoutNodeType> QuickJsRuntime::parseNodeType(JSContext* context, JSValueConst value) {
    const char* typeString = JS_ToCString(context, value);
    if (typeString == nullptr) {
        return std::nullopt;
    }

    const std::string type(typeString);
    JS_FreeCString(context, typeString);

    if (type == "workspace") return domain::LayoutNodeType::Workspace;
    if (type == "group") return domain::LayoutNodeType::Group;
    if (type == "window") return domain::LayoutNodeType::Window;
    if (type == "slot") return domain::LayoutNodeType::Slot;
    return std::nullopt;
}

std::optional<std::vector<std::string>> QuickJsRuntime::parseClasses(JSContext* context, JSValueConst value) {
    if (JS_IsUndefined(value) || JS_IsNull(value)) {
        return std::vector<std::string> {};
    }

    const char* classString = JS_ToCString(context, value);
    if (classString == nullptr) {
        return std::nullopt;
    }

    std::vector<std::string> classes;
    std::stringstream stream(classString);
    std::string item;
    while (stream >> item) {
        classes.push_back(item);
    }

    JS_FreeCString(context, classString);
    return classes;
}

std::optional<std::vector<domain::LayoutNode>> QuickJsRuntime::parseChildren(JSContext* context, JSValueConst value) {
    std::vector<domain::LayoutNode> children;

    if (JS_IsUndefined(value) || JS_IsNull(value)) {
        return children;
    }

    if (JS_IsArray(context, value)) {
        JSValue lengthValue = JS_GetPropertyStr(context, value, "length");
        int32_t length = 0;
        JS_ToInt32(context, &length, lengthValue);
        JS_FreeValue(context, lengthValue);

        for (uint32_t index = 0; index < static_cast<uint32_t>(length); ++index) {
            JSValue childValue = JS_GetPropertyUint32(context, value, index);
            auto childNode = parseLayoutNode(context, childValue);
            JS_FreeValue(context, childValue);
            if (childNode.has_value()) {
                children.push_back(std::move(*childNode));
            }
        }

        return children;
    }

    auto childNode = parseLayoutNode(context, value);
    if (childNode.has_value()) {
        children.push_back(std::move(*childNode));
    }

    return children;
}

std::optional<domain::LayoutNode> QuickJsRuntime::parseLayoutNode(JSContext* context, JSValueConst value) {
    if (JS_IsUndefined(value) || JS_IsNull(value) || !JS_IsObject(value)) {
        return std::nullopt;
    }

    JSValue typeValue = JS_GetPropertyStr(context, value, "type");
    const auto type = parseNodeType(context, typeValue);
    JS_FreeValue(context, typeValue);
    if (!type.has_value()) {
        return std::nullopt;
    }

    domain::LayoutNode node {
        .type = *type,
    };

    JSValue propsValue = JS_GetPropertyStr(context, value, "props");
    if (JS_IsObject(propsValue)) {
        JSValue idValue = JS_GetPropertyStr(context, propsValue, "id");
        if (!JS_IsUndefined(idValue) && !JS_IsNull(idValue)) {
            const char* idString = JS_ToCString(context, idValue);
            if (idString != nullptr) {
                node.id = idString;
                JS_FreeCString(context, idString);
            }
        }
        JS_FreeValue(context, idValue);

        JSValue classValue = JS_GetPropertyStr(context, propsValue, "class");
        const auto classes = parseClasses(context, classValue);
        JS_FreeValue(context, classValue);
        if (classes.has_value()) {
            node.classes = *classes;
        }

        JSValue matchValue = JS_GetPropertyStr(context, propsValue, "match");
        if (!JS_IsUndefined(matchValue) && !JS_IsNull(matchValue)) {
            const char* matchString = JS_ToCString(context, matchValue);
            if (matchString != nullptr) {
                node.match = std::string(matchString);
                JS_FreeCString(context, matchString);
            }
        }
        JS_FreeValue(context, matchValue);

        JSValue takeValue = JS_GetPropertyStr(context, propsValue, "take");
        if (!JS_IsUndefined(takeValue) && !JS_IsNull(takeValue)) {
            int32_t take = 0;
            if (JS_ToInt32(context, &take, takeValue) == 0) {
                node.take = take;
            }
        }
        JS_FreeValue(context, takeValue);
    }
    JS_FreeValue(context, propsValue);

    JSValue childrenValue = JS_GetPropertyStr(context, value, "children");
    const auto children = parseChildren(context, childrenValue);
    JS_FreeValue(context, childrenValue);
    if (children.has_value()) {
        node.children = *children;
    }

    return node;
}

QuickJsLayoutResult QuickJsRuntime::invokeLayout(std::string_view layoutName, const domain::RuntimeSnapshot& snapshot) const {
    if (!isReady()) {
        return {
            false,
            std::nullopt,
            std::nullopt,
            std::string("QuickJS runtime unavailable")
        };
    }

    const auto prepared = compiler_.prepareLayout(layoutName);
    if (!prepared.ok) {
        return {
            false,
            std::nullopt,
            std::nullopt,
            prepared.error
        };
    }

    const std::string& entryPath = prepared.entryPath;
    std::ifstream input(entryPath);
    if (!input.is_open()) {
        return {
            false,
            std::nullopt,
            std::nullopt,
            std::string("could not open ") + entryPath
        };
    }

    std::stringstream buffer;
    buffer << input.rdbuf();
    const std::string source = buffer.str();

    JSValue compiled = JS_Eval(
        context_.get(),
        source.c_str(),
        source.size(),
        entryPath.c_str(),
        JS_EVAL_TYPE_MODULE | JS_EVAL_FLAG_COMPILE_ONLY
    );

    if (JS_IsException(compiled)) {
        JS_FreeValue(context_.get(), compiled);
        const auto error = evalInternal("", "<module-compile-error>", JS_EVAL_TYPE_GLOBAL);
        return {false, std::nullopt, std::nullopt, error.error};
    }

    JSModuleDef* module = static_cast<JSModuleDef*>(JS_VALUE_GET_PTR(compiled));
    JSValue executed = JS_EvalFunction(context_.get(), compiled);
    if (JS_IsException(executed)) {
        JS_FreeValue(context_.get(), executed);
        const auto error = evalInternal("", "<module-exec-error>", JS_EVAL_TYPE_GLOBAL);
        return {false, std::nullopt, std::nullopt, error.error};
    }
    JS_FreeValue(context_.get(), executed);

    JSValue namespaceObject = JS_GetModuleNamespace(context_.get(), module);
    JSValue layoutFunction = JS_GetPropertyStr(context_.get(), namespaceObject, "default");

    if (!JS_IsFunction(context_.get(), layoutFunction)) {
        JS_FreeValue(context_.get(), layoutFunction);
        JS_FreeValue(context_.get(), namespaceObject);
        return {
            false,
            std::nullopt,
            std::nullopt,
            std::string("layout module default export is not a function")
        };
    }

    JSValue contextValue = runtimeSnapshotToJs(context_.get(), snapshot);
    JSValue result = JS_Call(context_.get(), layoutFunction, JS_UNDEFINED, 1, &contextValue);
    JS_FreeValue(context_.get(), contextValue);
    JS_FreeValue(context_.get(), layoutFunction);
    JS_FreeValue(context_.get(), namespaceObject);

    if (JS_IsException(result)) {
        JS_FreeValue(context_.get(), result);
        const auto error = evalInternal("", "<layout-call-error>", JS_EVAL_TYPE_GLOBAL);
        return {false, std::nullopt, std::nullopt, error.error};
    }

    const auto parsedRoot = parseLayoutNode(context_.get(), result);
    const char* valueString = JS_ToCString(context_.get(), result);
    std::string value = valueString != nullptr ? valueString : std::string {};

    if (valueString != nullptr) {
        JS_FreeCString(context_.get(), valueString);
    }

    JS_FreeValue(context_.get(), result);

    if (!parsedRoot.has_value()) {
        return {
            false,
            std::nullopt,
            value,
            std::string("layout returned an unsupported or invalid root node")
        };
    }

    return {
        true,
        parsedRoot,
        value,
        std::nullopt
    };
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
