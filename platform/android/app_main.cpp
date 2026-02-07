#include <android_native_app_glue.h>

#include <string>

#include "app/log.h"
#include "app/paths.h"
#include "gfx/egl_renderer.h"
#include "luax/lua_runtime.h"

// from platform/android/paths_android.cpp
namespace app::paths { void init_from_android_app(android_app* app); }

struct AppState {
    EglRenderer renderer;
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

void android_main(struct android_app* app) {
    LOGI("android_main() start");

    AppState state;
    app->userData = &state;
    app->onAppCmd = handle_cmd;

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
        const int timeout_ms = state.animating ? 16 : -1;

        while (ALooper_pollAll(timeout_ms, nullptr, &events, (void**)&source) >= 0) {
            if (source) source->process(app, source);

            if (app->destroyRequested) {
                state.renderer.shutdown();
                return;
            }

            if (timeout_ms == -1) break;
        }

        if (state.renderer.is_ready() && state.animating) {
            state.renderer.render_frame();
        }
    }
}
