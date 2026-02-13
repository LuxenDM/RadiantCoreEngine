#include "app/timer.h"
#include <vector>

namespace rce {

struct TimerEntry {
    TimerId id = 0;
    bool active = false;

    float remaining_s = 0.0f;
    float repeat_s = 0.0f;

    TimerCallback cb;
};

static std::vector<TimerEntry> g_timers;
static TimerId g_next_id = 1;

void timers_init() {
    g_timers.clear();
    g_next_id = 1;
}

static TimerId add_impl(float delay_s, float repeat_s, TimerCallback cb) {
    if (!cb) return 0;

    TimerEntry e;
    e.id = g_next_id++;
    if (e.id == 0) e.id = g_next_id++; // avoid 0
    e.active = true;
    e.remaining_s = (delay_s < 0.0f) ? 0.0f : delay_s;
    e.repeat_s = repeat_s;
    e.cb = std::move(cb);

    g_timers.push_back(std::move(e));
    return g_timers.back().id;
}

TimerId timers_add(const TimerSpec& spec, TimerCallback cb) {
    return add_impl(spec.delay_s, spec.repeat_s, std::move(cb));
}

TimerId timers_after(float delay_s, TimerCallback cb) {
    return add_impl(delay_s, 0.0f, std::move(cb));
}

TimerId timers_every(float interval_s, TimerCallback cb) {
    if (interval_s < 0.0f) interval_s = 0.0f;
    // first fire after interval by default
    return add_impl(interval_s, interval_s, std::move(cb));
}

void timers_cancel(TimerId id) {
    if (id == 0) return;
    for (auto& t : g_timers) {
        if (t.active && t.id == id) {
            t.active = false;
            t.cb = nullptr;
            return;
        }
    }
}

void timers_update(float dt_s) {
    if (dt_s < 0.0f) dt_s = 0.0f;

    // We allow callbacks to cancel timers (including themselves).
    // Avoid iterator invalidation by not erasing during iteration.
    for (auto& t : g_timers) {
        if (!t.active) continue;

        t.remaining_s -= dt_s;
        if (t.remaining_s > 0.0f) continue;

        // Fire
        TimerId id = t.id;
        TimerCallback cb = t.cb; // copy std::function (safe)
        if (cb) cb(id);

        // Timer might have been canceled during cb
        if (!t.active) continue;

        if (t.repeat_s > 0.0f) {
            // Repeat: carry over overshoot to keep cadence stable-ish
            // Example: remaining_s = -0.010 and repeat=1.0 => next in 0.990
            float overshoot = -t.remaining_s;
            float next = t.repeat_s - overshoot;
            if (next < 0.0f) next = 0.0f; // if dt was huge, don't go negative
            t.remaining_s = next;
        } else {
            // One-shot: deactivate
            t.active = false;
            t.cb = nullptr;
        }
    }

    // Optional cleanup pass to keep vector from growing forever
    // (cheap and safe). You can do this less often if desired.
    if (!g_timers.empty()) {
        size_t write = 0;
        for (size_t read = 0; read < g_timers.size(); ++read) {
            if (g_timers[read].active) {
                if (write != read) g_timers[write] = std::move(g_timers[read]);
                ++write;
            }
        }
        g_timers.resize(write);
    }
}

} // namespace rce
