#include <android_native_app_glue.h>
#include <string>

#include "app/paths.h"
#include "app/log.h"

namespace app::paths {

// Detect + set platform paths for Android.
// All stored paths are POSIX style, no trailing slash.
void init_from_android_app(android_app* app) {
    AppPaths p;

    const char* ext = app && app->activity ? app->activity->externalDataPath : nullptr;
    const char* inl = app && app->activity ? app->activity->internalDataPath : nullptr;

    // Prefer externalDataPath because that's what you're using now:
    // /sdcard/Android/data/<pkg>/files
    if (ext && *ext) {
        p.data = ext;
    } else if (inl && *inl) {
        // Typically: /data/data/<pkg>/files
        p.data = inl;
    } else {
        // Worst case: empty -> you’ll see it quickly via logs.
        p.data = "";
    }

    // Cache/logs convention: subfolders under data for now.
    // Later you can point cache to a true cache dir per platform.
    p.cache = join(p.data, "cache");
    p.logs  = join(p.data, "logs");

    set(std::move(p));

    const auto& gp = get();
    LOGI("Paths: data=%s",  gp.data.c_str());
    LOGI("Paths: cache=%s", gp.cache.c_str());
    LOGI("Paths: logs=%s",  gp.logs.c_str());
}

} // namespace app::paths
