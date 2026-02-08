#pragma once
#include <android_native_app_glue.h>

namespace input { class InputState; }

namespace platform::android_input {

// Installs an input callback that translates Android events into InputState.
void install(android_app* app, input::InputState* input_state);

} // namespace platform::android_input
