#include <jni.h>
#include <atomic>

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
}

// Call these from your renderer loop or platform service
int ui_inset_left()   { return g_inset_l.load(); }
int ui_inset_top()    { return g_inset_t.load(); }
int ui_inset_right()  { return g_inset_r.load(); }
int ui_inset_bottom() { return g_inset_b.load(); }

/*
	To adjust the viewport to 'classic' mode, where you do not draw under nav and status bars, use this:
	glViewport(insetL, insetB, width - insetL - insetR, height - insetT - insetB);
	
	insets are given in screen coords
*/

