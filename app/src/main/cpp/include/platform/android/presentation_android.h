#pragma once
#include <vector>
#include <string>
#include "gfx/presentation_types.h"

struct android_app;

namespace platform::android_presentation {

    // Returns platform-defined mode IDs (strings).
    std::vector<std::string> get_modes();

    // Apply OS-level UI behavior for a mode (immersive, decor fits, etc.)(if desired)
    void apply_mode(android_app* app, const std::string& mode_id);

    // Compute output/safe rect primitives for a mode from SurfaceMetrics.
    PresentationResult compute_result(const std::string& mode_id, const SurfaceMetrics& m);

}

