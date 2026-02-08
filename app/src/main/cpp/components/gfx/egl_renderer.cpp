// EGL creates and manages the bridge between an OS window and a GPU rendering context. Consider it the power outlet that OpenGL plugs into.

#include "gfx/egl_renderer.h"
#include "app/log.h"

#include <EGL/egl.h>
#include <GLES3/gl3.h>

EglRenderer::~EglRenderer() {
    shutdown();
}

bool EglRenderer::init(void* native_window) {
    if (ready_) return true;
    if (!native_window) {
		LOGE("EglRenderer::init: native_window is null");
		return false;
	}

    EGLDisplay dpy = eglGetDisplay(EGL_DEFAULT_DISPLAY);
    if (dpy == EGL_NO_DISPLAY) {
        LOGE("eglGetDisplay failed");
        return false;
    }
    if (!eglInitialize(dpy, nullptr, nullptr)) {
        LOGE("eglInitialize failed");
        return false;
    }

    // Request an ES3 config
    const EGLint configAttribs[] = {
        EGL_RENDERABLE_TYPE, EGL_OPENGL_ES3_BIT,
        EGL_SURFACE_TYPE,    EGL_WINDOW_BIT,
        EGL_RED_SIZE,        8,
        EGL_GREEN_SIZE,      8,
        EGL_BLUE_SIZE,       8,
        EGL_ALPHA_SIZE,      8,
        EGL_DEPTH_SIZE,      16,
        EGL_NONE
    };

    EGLConfig cfg = nullptr;
    EGLint num = 0;
    if (!eglChooseConfig(dpy, configAttribs, &cfg, 1, &num) || num < 1) {
        LOGE("eglChooseConfig failed (no matching configs)");
        eglTerminate(dpy);
        return false;
    }

	EGLSurface surf = eglCreateWindowSurface(
		dpy, cfg,
		(EGLNativeWindowType)native_window,
		nullptr
	);

    const EGLint ctxAttribs[] = {
        EGL_CONTEXT_CLIENT_VERSION, 3,
        EGL_NONE
    };

    EGLContext ctx = eglCreateContext(dpy, cfg, EGL_NO_CONTEXT, ctxAttribs);
    if (ctx == EGL_NO_CONTEXT) {
        LOGE("eglCreateContext failed");
        eglDestroySurface(dpy, surf);
        eglTerminate(dpy);
        return false;
    }

    if (!eglMakeCurrent(dpy, surf, surf, ctx)) {
        LOGE("eglMakeCurrent failed");
        eglDestroyContext(dpy, ctx);
        eglDestroySurface(dpy, surf);
        eglTerminate(dpy);
        return false;
    }

    EGLint w = 0, h = 0;
    eglQuerySurface(dpy, surf, EGL_WIDTH, &w);
    eglQuerySurface(dpy, surf, EGL_HEIGHT, &h);

    display_ = (void*)dpy;
    surface_ = (void*)surf;
    context_ = (void*)ctx;
    width_ = (int)w;
    height_ = (int)h;

    glViewport(0, 0, width_, height_);

    ready_ = true;
    LOGI("EGL ready: %dx%d", width_, height_);
    return true;
}

void EglRenderer::shutdown() {
    if (!display_) return;

    EGLDisplay dpy = (EGLDisplay)display_;
    EGLSurface surf = (EGLSurface)surface_;
    EGLContext ctx = (EGLContext)context_;

    // Detach
    eglMakeCurrent(dpy, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);

    if (ctx && ctx != EGL_NO_CONTEXT) eglDestroyContext(dpy, ctx);
    if (surf && surf != EGL_NO_SURFACE) eglDestroySurface(dpy, surf);

    eglTerminate(dpy);

    display_ = nullptr;
    surface_ = nullptr;
    context_ = nullptr;
    width_ = 0;
    height_ = 0;
    ready_ = false;

    LOGI("EGL shutdown complete");
}

void EglRenderer::render_frame(float r, float g, float b, float a) {
    if (!ready_) return;

    glViewport(0, 0, width_, height_);
    glClearColor(r, g, b, a);
    glClear(GL_COLOR_BUFFER_BIT);

    eglSwapBuffers(display_, surface_);
}

