#pragma once
#include <stdint.h>
#include <stdbool.h>

namespace rce {

// Keep this POD and fixed-size.
enum class EPType : uint16_t {
    // Platform -> Engine
    InsetsChanged = 1,
    SurfaceResized = 2,

    // Engine -> Platform
    SetAllowedRotations = 100,
    SetUiMode = 101,
};

struct EPMsg {
    EPType type;
    uint16_t flags = 0; // reserved
    uint32_t a = 0;
    uint32_t b = 0;
    uint32_t c = 0;
    uint32_t d = 0;
};

// Init once at startup (safe to call multiple times; no-op after first).
void ep_init();

// Platform -> Engine
bool ep_post_p2e(const EPMsg& msg);
bool ep_poll_p2e(EPMsg* out); // engine drains

// Engine -> Platform
bool ep_post_e2p(const EPMsg& msg);
bool ep_poll_e2p(EPMsg* out); // platform drains

// Optional debug counters (handy early)
struct EPCounters {
    uint64_t p2e_pushed, p2e_popped, p2e_dropped, p2e_highwater;
    uint64_t e2p_pushed, e2p_popped, e2p_dropped, e2p_highwater;
};
EPCounters ep_get_counters();

} // namespace rce
