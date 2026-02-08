#pragma once

class EglRenderer {
public:
    EglRenderer() = default;
    ~EglRenderer();

    // Non-copyable
    EglRenderer(const EglRenderer&) = delete;
    EglRenderer& operator=(const EglRenderer&) = delete;

    // Platform passes a native window handle as an opaque pointer.
    // Android: ANativeWindow* (app->window)
    bool init(void* native_window);
    void shutdown();

    bool is_ready() const { return ready_; }

    // For now: clear to a color and swap buffers
    void render_frame(float r, float g, float b, float a = 1.0f);
	
	int width() const { return width_; }
	int height() const { return height_; }
	
private:
    bool ready_ = false;

    void* display_ = nullptr; // EGLDisplay
    void* surface_ = nullptr; // EGLSurface
    void* context_ = nullptr; // EGLContext

    int width_ = 0;
    int height_ = 0;
};
