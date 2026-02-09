#include "platform/android/presentation_android.h"

#include <android_native_app_glue.h>
#include <jni.h>

#include "app/log.h"

// from platform/android/ui_insets_jni.cpp
int ui_inset_left();
int ui_inset_top();
int ui_inset_right();
int ui_inset_bottom();

namespace platform::android_presentation {

static int mode_id_to_java(const std::string& mode_id) {
    if (mode_id == "fit_classic") return 0;
    if (mode_id == "fit_modern")  return 1;
    if (mode_id == "immersive")   return 2;
    return 1; // default: modern
}

std::vector<std::string> get_modes() {
    return { "fit_modern", "fit_classic", "immersive" };
}

// Android-only: call activity.setUiMode(int)
static void set_ui_mode(android_app* app, int mode) {
    if (!app || !app->activity) return;

    ANativeActivity* act = app->activity;
    JavaVM* vm = act->vm;
    JNIEnv* env = nullptr;

    if (vm->AttachCurrentThread(&env, nullptr) != JNI_OK || !env) {
        LOGE("set_ui_mode: AttachCurrentThread failed");
        return;
    }

    jobject activity = act->clazz;
    jclass cls = env->GetObjectClass(activity);
    if (!cls) {
        LOGE("set_ui_mode: GetObjectClass failed");
        return;
    }

    jmethodID mid = env->GetMethodID(cls, "setUiMode", "(I)V");
    if (!mid) {
        LOGE("set_ui_mode: GetMethodID(setUiMode) failed");
        env->DeleteLocalRef(cls);
        return;
    }

    env->CallVoidMethod(activity, mid, (jint)mode);

    env->DeleteLocalRef(cls);
    // Optional detach; native_app_glue thread is long-lived, so leaving attached is OK.
}

void apply_mode(android_app* app, const std::string& mode_id) {
    // Even if Android ignores this, our engine simulation still works via output_rect.
    const int java_mode = mode_id_to_java(mode_id);
    set_ui_mode(app, java_mode);
}

static RectI clamp_rect_to_surface(const RectI& r, int sw, int sh) {
    RectI out = r;
    if (out.x < 0) out.x = 0;
    if (out.y < 0) out.y = 0;
    if (out.w < 1) out.w = 1;
    if (out.h < 1) out.h = 1;

    if (out.x + out.w > sw) out.w = sw - out.x;
    if (out.y + out.h > sh) out.h = sh - out.y;

    if (out.w < 1) out.w = 1;
    if (out.h < 1) out.h = 1;
    return out;
}

PresentationResult compute_result(const std::string& mode_id, const SurfaceMetrics& m) {
    PresentationResult res{};

    const RectI full = { 0, 0, m.surface_w, m.surface_h };

    // GL viewport is bottom-left origin.
    // Android insets arrive as left/top/right/bottom, where "top" is the top edge.
    // For viewport offset Y, we use inset_b (bottom).
    const RectI classic = {
        m.inset_l,
        m.inset_b,
        m.surface_w - (m.inset_l + m.inset_r),
        m.surface_h - (m.inset_t + m.inset_b)
    };

    if (mode_id == "fit_classic") {
        res.output_rect = clamp_rect_to_surface(classic, m.surface_w, m.surface_h);
        res.safe_rect   = res.output_rect;
        res.request_immersive = false;
    } else if (mode_id == "immersive") {
        res.output_rect = full;
        res.safe_rect   = full;
        res.request_immersive = true;
    } else { // fit_modern default
        res.output_rect = full;

        // Safe rect avoids bars/cutouts, but output is still edge-to-edge.
        RectI safe = classic;
        res.safe_rect = clamp_rect_to_surface(safe, m.surface_w, m.surface_h);
        res.request_immersive = false;
    }

    return res;
}

// Convenience to build SurfaceMetrics on Android (optional helper)
// If you prefer, keep this logic in app_main instead.
static SurfaceMetrics read_surface_metrics(int sw, int sh) {
    SurfaceMetrics m{};
    m.surface_w = sw;
    m.surface_h = sh;
    m.inset_l = ui_inset_left();
    m.inset_t = ui_inset_top();
    m.inset_r = ui_inset_right();
    m.inset_b = ui_inset_bottom();
    return m;
}

} // namespace platform::android_presentation
