#include <android_native_app_glue.h>

#include <string>
#include <android/input.h>
#include <jni.h>

// project headers
#include "app/log.h"
#include "app/paths.h"

#include "platform/android/input_android.h"
#include "input/input.h"

#include "gfx/egl_renderer.h"
#include "luax/lua_runtime.h"

// from platform/android/paths_android.cpp
namespace app::paths { void init_from_android_app(android_app* app); }

struct AppState {
    EglRenderer renderer;
    input::InputState input;
    bool animating = false;
};

static void handle_cmd(android_app* app, int32_t cmd) {
    auto* st = (AppState*)app->userData;

    switch (cmd) {
    case APP_CMD_INIT_WINDOW:
        if (app->window) {
            LOGI("APP_CMD_INIT_WINDOW");
            if (st->renderer.init((void*)app->window)) {
                st->animating = true;
            }
        }
        break;

    case APP_CMD_TERM_WINDOW:
        LOGI("APP_CMD_TERM_WINDOW");
        st->renderer.shutdown();
        st->animating = false;
        break;

    default:
        break;
    }
}

static int32_t handle_input(android_app* app, AInputEvent* event) {
    auto* st = (AppState*)app->userData;
    if (!st) return 0;

    if (AInputEvent_getType(event) != AINPUT_EVENT_TYPE_MOTION) return 0;

    const int32_t source = AInputEvent_getSource(event);
    if ((source & AINPUT_SOURCE_TOUCHSCREEN) == 0 &&
        (source & AINPUT_SOURCE_MOUSE) == 0 &&
        (source & AINPUT_SOURCE_STYLUS) == 0) {
        return 0;
    }

    const int32_t action = AMotionEvent_getAction(event);
    const int32_t action_mask = action & AMOTION_EVENT_ACTION_MASK;

    const int32_t pointer_index = (action & AMOTION_EVENT_ACTION_POINTER_INDEX_MASK)
                                >> AMOTION_EVENT_ACTION_POINTER_INDEX_SHIFT;

    const int32_t pointer_id = AMotionEvent_getPointerId(event, pointer_index);
    const float x = AMotionEvent_getX(event, pointer_index);
    const float y = AMotionEvent_getY(event, pointer_index);

    switch (action_mask) {
        case AMOTION_EVENT_ACTION_DOWN:
        case AMOTION_EVENT_ACTION_POINTER_DOWN:
            st->input.push_pointer_down(pointer_id, x, y);
            return 1;

        case AMOTION_EVENT_ACTION_UP:
        case AMOTION_EVENT_ACTION_POINTER_UP:
        case AMOTION_EVENT_ACTION_CANCEL:
            st->input.push_pointer_up(pointer_id, x, y);
            return 1;

        case AMOTION_EVENT_ACTION_MOVE: {
            // Use pointer 0 as your "primary" for now
            const int32_t count = AMotionEvent_getPointerCount(event);
            if (count > 0) {
                const int32_t id0 = AMotionEvent_getPointerId(event, 0);
                const float x0 = AMotionEvent_getX(event, 0);
                const float y0 = AMotionEvent_getY(event, 0);
                st->input.push_pointer_move(id0, x0, y0);
            } else {
                st->input.push_pointer_move(pointer_id, x, y);
            }
            return 1;
        }

        default:
            return 0;
    }
}

/*
	set_ui_mode(app, 0) -> fit inside (classic, viewport shrinks inside bars)
	set_ui_mode(app, 1) -> fit inside (modern, app must handle layout) (default)
	set_ui_mode(app, 2) -> fullscreen
	
	can use UiMode::<type>
*/
enum class UiMode { FitClassic=0, FitModern=1, Immersive=2 };
static void set_ui_mode(android_app* app, int mode) {
    ANativeActivity* act = app->activity;
    JavaVM* vm = act->vm;
    JNIEnv* env = nullptr;

    vm->AttachCurrentThread(&env, nullptr);

    jobject activity = act->clazz; // your MyNativeActivity instance
    jclass cls = env->GetObjectClass(activity);
    jmethodID mid = env->GetMethodID(cls, "setUiMode", "(I)V");
    env->CallVoidMethod(activity, mid, (jint)mode);

    env->DeleteLocalRef(cls);
    // (No Detach here if you stay on the same thread for app lifetime; optional.)
}


void android_main(struct android_app* app) {
    LOGI("android_main() start");

    AppState state;
    app->userData = &state;
    app->onAppCmd = handle_cmd;
	
	set_ui_mode(app, 0);
	
	platform::android_input::install(app, &state.input);


    // Init platform paths once.
    app::paths::init_from_android_app(app);

    // Build script path in POSIX style
    const auto& p = app::paths::get();
    if (!p.data.empty()) {
        std::string script = app::paths::join(p.data, "scripts/main.lua");
        LOGI("Trying Lua script: %s", script.c_str());

        luax_run_file(script.c_str());

        LOGI("Returned from luax_run_file()");
    } else {
        LOGE("No valid data directory (paths.data is empty)");
        return;
    }

    int events = 0;
    android_poll_source* source = nullptr;

    while (true) {
		state.input.begin_frame();
        int timeout_ms = state.animating ? 16 : -1;

        for (;;) {
			int ident = ALooper_pollOnce(timeout_ms, nullptr, &events, (void**)&source);

			if (ident == ALOOPER_POLL_TIMEOUT) {
				break; // no events ready
			}
			if (ident == ALOOPER_POLL_ERROR) {
				break; // looper error
			}

			if (source) source->process(app, source);

			if (app->destroyRequested) {
				state.renderer.shutdown();
				return;
			}

			// If we were in "block forever" mode (-1), don't keep draining;
			// do one event and then go back to blocking.
			if (timeout_ms == -1) break;

			// Otherwise: after the first wait, drain without blocking.
			timeout_ms = 0;
		}


        if (state.renderer.is_ready() && state.animating) {
			const float w = (float)state.renderer.width();
			const float h = (float)state.renderer.height();

			float r = 0.0f;
			float b = 0.0f;

			if (w > 0.0f && h > 0.0f) {
				r = state.input.pointer_x() / w;
				b = state.input.pointer_y() / h;

				// clamp (just in case)
				if (r < 0.0f) r = 0.0f; else if (r > 1.0f) r = 1.0f;
				if (b < 0.0f) b = 0.0f; else if (b > 1.0f) b = 1.0f;
			}

			state.renderer.render_frame(r, 1.0f, b);
		}
    }
}

