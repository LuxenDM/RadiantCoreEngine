// EGL creates and manages the bridge between an OS window and a GPU rendering context.

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
        dpy, cfg, (EGLNativeWindowType)native_window, nullptr
    );
    if (surf == EGL_NO_SURFACE) {
        LOGE("eglCreateWindowSurface failed");
        eglTerminate(dpy);
        return false;
    }

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

    // default viewport (full surface)
    use_custom_viewport_ = false;
    vp_x_ = vp_y_ = 0;
    vp_w_ = width_;
    vp_h_ = height_;

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

    use_custom_viewport_ = false;

    LOGI("EGL shutdown complete");
}

void EglRenderer::set_viewport(int x, int y, int w, int h) {
    // Clamp to sane values; we will clamp again against surface bounds in render_frame.
    if (w < 1) w = 1;
    if (h < 1) h = 1;

    vp_x_ = x;
    vp_y_ = y;
    vp_w_ = w;
    vp_h_ = h;
    use_custom_viewport_ = true;
}

bool EglRenderer::recalc_surface_size() {
    if (!display_ || !surface_) return false;

    EGLint w = 0, h = 0;
    eglQuerySurface((EGLDisplay)display_, (EGLSurface)surface_, EGL_WIDTH, &w);
    eglQuerySurface((EGLDisplay)display_, (EGLSurface)surface_, EGL_HEIGHT, &h);

    if (w <= 0 || h <= 0) return false;

    const int new_w = (int)w;
    const int new_h = (int)h;

    if (new_w == width_ && new_h == height_) return false;

    width_ = new_w;
    height_ = new_h;
    return true;
}

void EglRenderer::reset_viewport() {
    use_custom_viewport_ = false;
}

void EglRenderer::set_output_rect(const RectI& r) {
    output_rect_ = r;
    has_output_rect_ = true;

    // Also mirror into the viewport fields so existing viewport logic still works.
    set_viewport(r.x, r.y, r.w, r.h);
}

void EglRenderer::clear_output_rect() {
    has_output_rect_ = false;
    reset_viewport();
}

void EglRenderer::set_scissor_rect(const RectI& r) {
    scissor_rect_ = r;
    has_scissor_ = true;
}

void EglRenderer::clear_scissor() {
    has_scissor_ = false;
}


void EglRenderer::render_frame(float r, float g, float b, float a) {
    if (!ready_) return;

    // ----- PASS 1: clear the full surface to a border color (black)
    glDisable(GL_SCISSOR_TEST);
    glViewport(0, 0, width_, height_);
    glClearColor(1.f, 0.f, 1.f, 1.f);
    glClear(GL_COLOR_BUFFER_BIT);

    // ----- Compute the content viewport (from your existing viewport logic)
    int vx = 0, vy = 0, vw = width_, vh = height_;

    if (use_custom_viewport_) {
        vx = vp_x_;
        vy = vp_y_;
        vw = vp_w_;
        vh = vp_h_;

        // Clamp viewport to surface bounds
        if (vx < 0) vx = 0;
        if (vy < 0) vy = 0;
        if (vw < 1) vw = 1;
        if (vh < 1) vh = 1;

        if (vx + vw > width_)  vw = width_  - vx;
        if (vy + vh > height_) vh = height_ - vy;

        if (vw < 1) vw = 1;
        if (vh < 1) vh = 1;
    }

    glViewport(vx, vy, vw, vh);

    // ----- PASS 2: optionally restrict to the scissor rect, then clear content color
    if (has_scissor_) {
        // Clamp scissor rect too (defensive)
        int sx = scissor_rect_.x;
        int sy = scissor_rect_.y;
        int sw = scissor_rect_.w;
        int sh = scissor_rect_.h;

        if (sx < 0) sx = 0;
        if (sy < 0) sy = 0;
        if (sw < 1) sw = 1;
        if (sh < 1) sh = 1;

        if (sx + sw > width_)  sw = width_  - sx;
        if (sy + sh > height_) sh = height_ - sy;

        if (sw < 1) sw = 1;
        if (sh < 1) sh = 1;

        glEnable(GL_SCISSOR_TEST);
        glScissor(sx, sy, sw, sh);
    } else {
        glDisable(GL_SCISSOR_TEST);
    }

    glClearColor(r, g, b, a);
    glClear(GL_COLOR_BUFFER_BIT);

    eglSwapBuffers((EGLDisplay)display_, (EGLSurface)surface_);
}

