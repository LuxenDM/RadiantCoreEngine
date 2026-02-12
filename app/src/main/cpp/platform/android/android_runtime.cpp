#include "app/event_pipe.h"
#include "platform/android/android_runtime.h"
#include "platform/android/rce_platform.h" // for rce_android_set_allowed_rotations etc.

namespace platform::android_runtime {

static android_app* g_app = nullptr;

void init(android_app* app) {
    g_app = app;
    // Bind activity for JNI bridges you already have
    rce_android_bind_activity(app->activity);
}

void pump_engine_commands() {
    rce::EPMsg msg;
    while (rce::ep_poll_e2p(&msg)) {
        switch (msg.type) {
            case rce::EPType::SetAllowedRotations:
                rce_android_set_allowed_rotations(msg.a);
                break;

            case rce::EPType::SetUiMode:
                // If you later expose rce_android_set_ui_mode(...)
                // rce_android_set_ui_mode(msg.a);
                break;

            default:
                // For now: ignore
                // LOGI("Unhandled e2p msg type=%d", (int)msg.type);
                break;
        }
    }
}

void tick() {
    pump_engine_commands();
}

} // namespace rce::android
