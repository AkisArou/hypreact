#include "plugin/plugin.hpp"

APICALL EXPORT std::string PLUGIN_API_VERSION() {
    return hypreact::plugin::Plugin::instance().apiVersion();
}

APICALL EXPORT PLUGIN_DESCRIPTION_INFO PLUGIN_INIT(HANDLE handle) {
    const auto description = hypreact::plugin::Plugin::instance().init(handle);
    return {
        description.name,
        description.description,
        description.author,
        description.version,
    };
}

APICALL EXPORT void PLUGIN_EXIT() {
    hypreact::plugin::Plugin::instance().exit();
}
