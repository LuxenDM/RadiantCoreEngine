#include "app/engine.h"
#include "app/event_dispatcher.h"   // if you added it
#include "app/event_pipe.h"
#include "app/log.h"

namespace rce {

void engine_init() {
    // Later: subscribe events here
}

void engine_tick(float dt) {

    // 1. Dispatch platform -> engine events
    ep_dispatch_all_p2e();

    // 2. Update game logic (future)
    // 3. Update timers (future)
    // 4. Submit rendering (future)

    (void)dt;
}

}
