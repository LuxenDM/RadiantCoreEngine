#pragma once

struct RectI {
    int x = 0;
    int y = 0;
    int w = 0;
    int h = 0;
};

struct SurfaceMetrics {
    int surface_w = 0;
    int surface_h = 0;

    // Insets in surface pixel coordinates (Android system bars/cutouts)
    int inset_l = 0;
    int inset_t = 0;
    int inset_r = 0;
    int inset_b = 0;
};

// Portable primitives every platform mode must output.
struct PresentationResult {
    RectI output_rect;         // GL viewport region inside surface (bottom-left origin for GL)
    RectI safe_rect;           // UI/layout safe region (same coords as output_rect or smaller)
    bool request_immersive = false; // platform may attempt OS UI changes (Android only)
};
