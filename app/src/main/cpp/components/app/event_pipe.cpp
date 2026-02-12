#include "app/event_pipe.h"
#include <mutex>

namespace rce {

static constexpr uint32_t CAP = 512;

struct Ring {
    EPMsg buf[CAP]{};
    uint32_t head = 0; // next write
    uint32_t tail = 0; // next read
    uint32_t count = 0;
    std::mutex m;

    uint64_t pushed = 0;
    uint64_t popped = 0;
    uint64_t dropped = 0;
    uint64_t highwater = 0;
};

static Ring g_p2e;
static Ring g_e2p;
static bool g_inited = false;

static bool push(Ring& r, const EPMsg& msg) {
    std::lock_guard<std::mutex> lock(r.m);

    if (r.count >= CAP) {
        r.dropped++;
        return false;
    }

    r.buf[r.head] = msg;
    r.head = (r.head + 1) % CAP;
    r.count++;
    r.pushed++;
    if (r.count > r.highwater) r.highwater = r.count;
    return true;
}

static bool pop(Ring& r, EPMsg* out) {
    if (!out) return false;

    std::lock_guard<std::mutex> lock(r.m);

    if (r.count == 0) return false;

    *out = r.buf[r.tail];
    r.tail = (r.tail + 1) % CAP;
    r.count--;
    r.popped++;
    return true;
}

void ep_init() {
    if (g_inited) return;
    g_inited = true;
}

bool ep_post_p2e(const EPMsg& msg) { ep_init(); return push(g_p2e, msg); }
bool ep_poll_p2e(EPMsg* out)       { ep_init(); return pop(g_p2e, out);  }

bool ep_post_e2p(const EPMsg& msg) { ep_init(); return push(g_e2p, msg); }
bool ep_poll_e2p(EPMsg* out)       { ep_init(); return pop(g_e2p, out);  }

EPCounters ep_get_counters() {
    EPCounters c{};
    {
        std::lock_guard<std::mutex> lock(g_p2e.m);
        c.p2e_pushed = g_p2e.pushed;
        c.p2e_popped = g_p2e.popped;
        c.p2e_dropped = g_p2e.dropped;
        c.p2e_highwater = g_p2e.highwater;
    }
    {
        std::lock_guard<std::mutex> lock(g_e2p.m);
        c.e2p_pushed = g_e2p.pushed;
        c.e2p_popped = g_e2p.popped;
        c.e2p_dropped = g_e2p.dropped;
        c.e2p_highwater = g_e2p.highwater;
    }
    return c;
}

} // namespace rce
