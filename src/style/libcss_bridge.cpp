#include "libcss_bridge.hpp"

#if defined(HYPREACT_HAS_LIBCSS)
#include <libcss/libcss.h>
#endif

namespace hypreact::style {

LibcssBridgeStatus libcssBridgeStatus() {
#if defined(HYPREACT_HAS_LIBCSS)
    return LibcssBridgeStatus {
        true,
        "libcss bridge available"
    };
#else
    return LibcssBridgeStatus {
        false,
        "libcss bridge unavailable: install libparserutils and libwapcaplet"
    };
#endif
}

} // namespace hypreact::style
