#pragma once
#include <stdint.h>
#include <functional>

namespace rce {

using TimerId = uint32_t;

struct TimerSpec {
    // Delay until first fire.
    float delay_s = 0.0f;

    // If repeat_s > 0, timer repeats at this interval after firing.
    // If repeat_s <= 0, timer is one-shot.
    float repeat_s = 0.0f;

    // Optional: fire immediately on the next update (after delay handling).
    // Usually you just set delay_s = 0, so this is here mainly for clarity/future.
};

using TimerCallback = std::function<void(TimerId)>;

// Initialize/clear timer system (call once at engine init; safe to call multiple times).
void timers_init();

// Schedule a timer. Returns 0 on failure (0 is reserved as "invalid").
TimerId timers_add(const TimerSpec& spec, TimerCallback cb);

// Convenience: one-shot after delay.
TimerId timers_after(float delay_s, TimerCallback cb);

// Convenience: repeating interval (first fire after interval by default).
TimerId timers_every(float interval_s, TimerCallback cb);

// Cancel a timer by id (safe to call multiple times).
void timers_cancel(TimerId id);

// Called once per frame from engine_tick(dt).
void timers_update(float dt_s);

} // namespace rce
