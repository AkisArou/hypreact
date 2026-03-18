#include "libcss_bridge.hpp"

#if defined(__cplusplus) && !defined(restrict)
#define restrict __restrict
#endif

#include <libcss/libcss.h>

namespace hypreact::style {

LibcssBridgeStatus libcssBridgeStatus() {
    return LibcssBridgeStatus {
        true,
        "libcss bridge available"
    };
}

} // namespace hypreact::style
