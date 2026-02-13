#include "app/engine.h"
#include "app/event_dispatcher.h"   // if you added it
#include "app/event_pipe.h"
#include "app/log.h"

#include "app/timer.h"

namespace rce {

void engine_init() {
    timers_init();

    // quick proof:
    timers_every(1.0f, [](rce::TimerId) {
        LOGI("timer: 1 second tick");
    });
}

void engine_tick(float dt) {

    // 1. Dispatch platform -> engine events
    ep_dispatch_all_p2e();

    // 2. Update game logic (future)
    // 3. Update timers
	timers_update(dt);
    // 4. Submit rendering (future)
    // ...
    (void)dt;
}

}
