#include <jni.h>
#include <atomic>
#include <android/native_activity.h> // ANativeActivity

#include "app/event_pipe.h"

// -------------------------
// Insets (existing behavior)
// -------------------------
static std::atomic<int> g_inset_l{0}, g_inset_t{0}, g_inset_r{0}, g_inset_b{0};

extern "C" JNIEXPORT void JNICALL
Java_com_example_myapplication_MyNativeActivity_nativeOnInsetsChanged(
    JNIEnv* env, jclass clazz, jint l, jint t, jint r, jint b)
{
    (void)env; (void)clazz;
    g_inset_l.store((int)l);
    g_inset_t.store((int)t);
    g_inset_r.store((int)r);
    g_inset_b.store((int)b);
	
	rce::EPMsg m{};
	m.type = rce::EPType::InsetsChanged;
	m.a = (uint32_t)l;
	m.b = (uint32_t)t;
	m.c = (uint32_t)r;
	m.d = (uint32_t)b;
	rce::ep_post_p2e(m);
}

int ui_inset_left()   { return g_inset_l.load(); }
int ui_inset_top()    { return g_inset_t.load(); }
int ui_inset_right()  { return g_inset_r.load(); }
int ui_inset_bottom() { return g_inset_b.load(); }

// -------------------------
// Rotation policy bridge
// -------------------------
static JavaVM*   g_vm = nullptr;
static jobject   g_activity = nullptr;   // GlobalRef to MyNativeActivity instance
static jmethodID g_mid_setAllowedRotations = nullptr;

static JNIEnv* get_env() {
    if (!g_vm) return nullptr;

    JNIEnv* env = nullptr;
    const jint r = g_vm->GetEnv((void**)&env, JNI_VERSION_1_6);
    if (r == JNI_OK) return env;

    // Attach if needed (safe for threads created in native)
    if (g_vm->AttachCurrentThread(&env, nullptr) != JNI_OK) return nullptr;
    return env;
}

// Call this once early (android_main) to bind activity + cache method id.
extern "C" void rce_android_bind_activity(ANativeActivity* activity) {
    if (!activity || !activity->vm || !activity->clazz) return;

    g_vm = activity->vm;

    JNIEnv* env = get_env();
    if (!env) return;

    // Store Activity instance
    if (!g_activity) {
        g_activity = env->NewGlobalRef(activity->clazz);
    }

    // Cache method id
    if (g_activity && !g_mid_setAllowedRotations) {
        jclass cls = env->GetObjectClass(g_activity);
        g_mid_setAllowedRotations =
            env->GetMethodID(cls, "setAllowedRotations", "(I)V");
        env->DeleteLocalRef(cls);
    }
}

extern "C" void rce_android_set_allowed_rotations(uint32_t allowed_mask) {
    if (!g_vm || !g_activity || !g_mid_setAllowedRotations) return;

    JNIEnv* env = get_env();
    if (!env) return;

    env->CallVoidMethod(g_activity, g_mid_setAllowedRotations, (jint)allowed_mask);
    // Optional: if you want to notice failures during dev:
    // if (env->ExceptionCheck()) { env->ExceptionDescribe(); env->ExceptionClear(); }
}
