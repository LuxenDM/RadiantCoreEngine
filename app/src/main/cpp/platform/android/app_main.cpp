#include <android_native_app_glue.h>

#include <string>
#include <android/input.h>

// project headers
//// generics
#include "app/log.h"
#include "app/paths.h"

//// input and device management
#include "platform/android/input_android.h"
#include "input/input.h"

//// android os interactions
#include "platform/android/presentation_android.h"
#include "platform/android/rce_platform.h"

//// graphical output
#include "gfx/egl_renderer.h"
#include "gfx/presentation_types.h"

//// lua subsystem
#include "luax/lua_runtime.h"


// from platform/android/paths_android.cpp
namespace app::paths { void init_from_android_app(android_app* app); }

// from platform/android/ui_insets_jni.cpp
int ui_inset_left();
int ui_inset_top();
int ui_inset_right();
int ui_inset_bottom();

struct AppState {
    EglRenderer renderer;
    input::InputState input;
    bool animating = false;

    std::string presentation_mode = "fit_classic"; // default
	
	int pending_resize_frames = 0;
};

static void handle_cmd(android_app* app, int32_t cmd) {
    auto* st = (AppState*)app->userData;

    switch (cmd) {
    case APP_CMD_INIT_WINDOW:
        if (app->window) {
            LOGI("APP_CMD_INIT_WINDOW");
            if (st->renderer.init((void*)app->window)) {
                st->animating = true;

                // Optional OS/UI request; simulation happens regardless.
                platform::android_presentation::apply_mode(app, st->presentation_mode);
            }
        }
        break;

    case APP_CMD_TERM_WINDOW:
        LOGI("APP_CMD_TERM_WINDOW");
        st->renderer.shutdown();
        st->animating = false;
        break;
	
	case APP_CMD_GAINED_FOCUS:
		LOGI("APP_CMD_GAINED_FOCUS");
		platform::android_presentation::apply_mode(app, st->presentation_mode);
		break;
	
	case APP_CMD_CONTENT_RECT_CHANGED:
		LOGI("APP_CMD_CONTENT_RECT_CHANGED");
		st->pending_resize_frames = 3;
		break;
	
	case APP_CMD_WINDOW_RESIZED:
		LOGI("APP_CMD_WINDOW_RESIZED");
		st->pending_resize_frames = 3;
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

static SurfaceMetrics build_surface_metrics(const AppState& st) {
    SurfaceMetrics m{};
    m.surface_w = st.renderer.width();
    m.surface_h = st.renderer.height();
    m.inset_l = ui_inset_left();
    m.inset_t = ui_inset_top();
    m.inset_r = ui_inset_right();
    m.inset_b = ui_inset_bottom();
    return m;
}

void android_main(struct android_app* app) {
    LOGI("android_main() start");

    AppState state;
    app->userData = &state;
    app->onAppCmd = handle_cmd;
    app->onInputEvent = handle_input;
	
	rce_android_bind_activity(app->activity);
	rce_android_set_allowed_rotations(RCE_ROT_90 | RCE_ROT_270);

    // Install Android input bridge
    platform::android_input::install(app, &state.input);

    // Init platform paths once.
    app::paths::init_from_android_app(app);

    // Run Lua script (current behavior)
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

            if (ident == ALOOPER_POLL_TIMEOUT) break;
            if (ident == ALOOPER_POLL_ERROR) break;

            if (source) source->process(app, source);

            if (app->destroyRequested) {
                state.renderer.shutdown();
                return;
            }

            if (timeout_ms == -1) break;
            timeout_ms = 0;
        }

        if (state.renderer.is_ready() && state.animating) {
			if (state.pending_resize_frames > 0) {
				if (state.renderer.recalc_surface_size()) {
					// size changed again; keep checking a couple more frames
					state.pending_resize_frames = 3;
				} else {
					state.pending_resize_frames--;
				}
			}
			
            // Compute and apply presentation primitives each frame for now.
            // (Later: only recompute when insets/mode changes.)
            SurfaceMetrics m = build_surface_metrics(state);
            PresentationResult pr = platform::android_presentation::compute_result(state.presentation_mode, m);

            // Apply output rect (viewport)
            state.renderer.set_viewport(pr.output_rect.x, pr.output_rect.y, pr.output_rect.w, pr.output_rect.h);
			// New: scissor restricts clears/draws to output rect
			state.renderer.set_output_rect(pr.output_rect);
			
			if (state.presentation_mode == "fit_classic") {
				state.renderer.set_scissor_rect(pr.output_rect); // **this is the key**
			} else {
				state.renderer.clear_scissor();
			}

			/*
			// Existing color wipe based on pointer position (still uses full surface for now)
            const float w = (float)state.renderer.width();
            const float h = (float)state.renderer.height();

            float r = 0.0f;
            float b = 0.0f;

            if (w > 0.0f && h > 0.0f) {
                r = state.input.pointer_x() / w;
                b = state.input.pointer_y() / h;

                if (r < 0.0f) r = 0.0f; else if (r > 1.0f) r = 1.0f;
                if (b < 0.0f) b = 0.0f; else if (b > 1.0f) b = 1.0f;
            }
			LOGI("mode=%s surface=%dx%d insets LTRB=%d,%d,%d,%d output=%d,%d %dx%d",
				state.presentation_mode.c_str(),
				m.surface_w, m.surface_h,
				m.inset_l, m.inset_t, m.inset_r, m.inset_b,
				pr.output_rect.x, pr.output_rect.y, pr.output_rect.w, pr.output_rect.h);


            state.renderer.render_frame(r, 1.0f, b);
			*/
            // Existing color wipe based on pointer position (still uses full surface for now)
            const float w = (float)state.renderer.width();
            const float h = (float)state.renderer.height();

            float hue = 0.0f;
            float saturation = 0.0f;

            if (w > 0.0f && h > 0.0f) {
                hue = state.input.pointer_x() / w;
                saturation = state.input.pointer_y() / h;

                if (hue < 0.0f) hue = 0.0f; else if (hue > 1.0f) hue = 1.0f;
                if (saturation < 0.0f) saturation = 0.0f; else if (saturation > 1.0f) saturation = 1.0f;
            }
			LOGI("mode=%s surface=%dx%d insets LTRB=%d,%d,%d,%d output=%d,%d %dx%d",
				state.presentation_mode.c_str(),
				m.surface_w, m.surface_h,
				m.inset_l, m.inset_t, m.inset_r, m.inset_b,
				pr.output_rect.x, pr.output_rect.y, pr.output_rect.w, pr.output_rect.h);


            // Map x -> hue and y -> saturation with value fixed at 1.0.
            const float value = 1.0f;
            const float h6 = hue * 6.0f;
            const int i = (int)h6;
            const float f = h6 - (float)i;
            const float p = value * (1.0f - saturation);
            const float q = value * (1.0f - saturation * f);
            const float t = value * (1.0f - saturation * (1.0f - f));

            float r = 0.0f, g = 0.0f, b = 0.0f;
            switch (i % 6) {
                case 0: r = value; g = t;     b = p;     break;
                case 1: r = q;     g = value; b = p;     break;
                case 2: r = p;     g = value; b = t;     break;
                case 3: r = p;     g = q;     b = value; break;
                case 4: r = t;     g = p;     b = value; break;
                case 5: r = value; g = p;     b = q;     break;
            }

            state.renderer.render_frame(r, g, b);
        }
    }
}

