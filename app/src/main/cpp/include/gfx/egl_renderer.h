#pragma once

#include "gfx/presentation_types.h"

class EglRenderer {
public:
    EglRenderer() = default;
    ~EglRenderer();

    // Non-copyable
    EglRenderer(const EglRenderer&) = delete;
    EglRenderer& operator=(const EglRenderer&) = delete;
	
	void set_viewport(int x, int y, int w, int h);
    void reset_viewport();
    void render_frame(float r, float g, float b, float a = 1.0f);
	bool recalc_surface_size();
	
	// 
    void set_output_rect(const RectI& r);
    void clear_output_rect(); // revert to full surface
	
    void set_scissor_rect(const RectI& r);
    void clear_scissor();

    RectI surface_rect() const { return {0, 0, width_, height_}; }

    // Platform passes a native window handle as an opaque pointer.
    // Android: ANativeWindow* (app->window)
    bool init(void* native_window);
    void shutdown();

    bool is_ready() const { return ready_; }
	
	int width() const { return width_; }
	int height() const { return height_; }
	
private:
    bool ready_ = false;
	
    bool use_custom_viewport_ = false;
	
    bool has_output_rect_ = false;
    RectI output_rect_;

    bool has_scissor_ = false;
    RectI scissor_rect_;

    void* display_ = nullptr; // EGLDisplay
    void* surface_ = nullptr; // EGLSurface
    void* context_ = nullptr; // EGLContext

    int width_ = 0;
    int height_ = 0;
    
	int vp_x_ = 0;
    int vp_y_ = 0;
    int vp_w_ = 0;
    int vp_h_ = 0;
};
