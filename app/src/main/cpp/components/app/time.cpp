#include "app/time.h"

namespace rce {

static uint64_t g_last_ms = 0;

void time_init(uint64_t start_ms) {
    g_last_ms = start_ms;
}

float time_update(uint64_t now_ms) {
    uint64_t delta = now_ms - g_last_ms;
    g_last_ms = now_ms;
    return delta / 1000.0f;
}

}
