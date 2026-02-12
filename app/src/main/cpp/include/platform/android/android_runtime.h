#pragma once
#include <android_native_app_glue.h>

namespace platform::android_runtime {

void init(android_app* app);   // binds activity, caches, etc.
void shutdown();               // optional for later
void pump_engine_commands();   // drains e2p pipe and executes
void pump_platform_events();   // optional: drain p2e? usually engine does that
void tick();                   // convenience: calls pump_engine_commands()

} // namespace rce::android
