#include "platform/android/input_android.h"

#include <android/input.h>
#include "input/input.h"

namespace platform::android_input {

static input::InputState* g_input = nullptr;

static int32_t handle_input(android_app* app, AInputEvent* event) {
    (void)app;
    if (!g_input) return 0;

    if (AInputEvent_getType(event) != AINPUT_EVENT_TYPE_MOTION) return 0;

    const int32_t source = AInputEvent_getSource(event);
    if ((source & AINPUT_SOURCE_TOUCHSCREEN) == 0 &&
        (source & AINPUT_SOURCE_MOUSE) == 0 &&
        (source & AINPUT_SOURCE_STYLUS) == 0) {
        return 0;
    }

    const int32_t action = AMotionEvent_getAction(event);
    const int32_t action_mask = action & AMOTION_EVENT_ACTION_MASK;

    const int32_t pointer_index =
        (action & AMOTION_EVENT_ACTION_POINTER_INDEX_MASK) >>
        AMOTION_EVENT_ACTION_POINTER_INDEX_SHIFT;

    const int32_t pointer_id = AMotionEvent_getPointerId(event, pointer_index);
    const float x = AMotionEvent_getX(event, pointer_index);
    const float y = AMotionEvent_getY(event, pointer_index);

    switch (action_mask) {
        case AMOTION_EVENT_ACTION_DOWN:
        case AMOTION_EVENT_ACTION_POINTER_DOWN:
            g_input->push_pointer_down(pointer_id, x, y);
            return 1;

        case AMOTION_EVENT_ACTION_UP:
        case AMOTION_EVENT_ACTION_POINTER_UP:
        case AMOTION_EVENT_ACTION_CANCEL:
            g_input->push_pointer_up(pointer_id, x, y);
            return 1;

        case AMOTION_EVENT_ACTION_MOVE: {
            // Primary pointer only for now.
            const int32_t count = AMotionEvent_getPointerCount(event);
            if (count > 0) {
                const int32_t id0 = AMotionEvent_getPointerId(event, 0);
                const float x0 = AMotionEvent_getX(event, 0);
                const float y0 = AMotionEvent_getY(event, 0);
                g_input->push_pointer_move(id0, x0, y0);
            } else {
                g_input->push_pointer_move(pointer_id, x, y);
            }
            return 1;
        }

        default:
            return 0;
    }
}

void install(android_app* app, input::InputState* input_state) {
    g_input = input_state;
    app->onInputEvent = handle_input;
}

} // namespace platform::android_input
