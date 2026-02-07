#include "luax/lua_runtime.h"

#include "app/log.h"
#include <string>

extern "C" {
#include "lua.h"
#include "lauxlib.h"
#include "lualib.h"
}

static int l_android_log(lua_State* L) {
    int n = lua_gettop(L);

    std::string out;
    out.reserve(256);

    // tostring is expected, but keep it robust.
    lua_getglobal(L, "tostring");
    bool have_tostring = lua_isfunction(L, -1);

    for (int i = 1; i <= n; i++) {
        if (i > 1) out += "\t";

        if (have_tostring) {
            lua_pushvalue(L, -1); // tostring
            lua_pushvalue(L, i);  // arg
            lua_call(L, 1, 1);    // tostring(arg)

            size_t len = 0;
            const char* s = lua_tolstring(L, -1, &len);
            if (s && len) out.append(s, len);
            else out += "(value)";

            lua_pop(L, 1); // pop tostring result
        } else {
            // Fallback: best-effort type name
            out += luaL_typename(L, i);
        }
    }

    lua_pop(L, 1); // pop tostring / non-function

    // Keep everything under your app tag so your existing filter catches it.
    LOGI("[lua] %s", out.c_str());
    return 0;
}

bool luax_run_file(const char* path) {
    if (!path || !*path) {
        LOGE("luax_run_file: invalid path");
        return false;
    }

    lua_State* L = luaL_newstate();
    if (!L) {
        LOGE("Lua: luaL_newstate failed");
        return false;
    }

    luaL_openlibs(L);

    // You chose console_print() instead of overriding print().
    lua_pushcfunction(L, l_android_log);
    lua_setglobal(L, "console_print");

    int rc = luaL_dofile(L, path);
    if (rc != 0) {
        const char* err = lua_tostring(L, -1);
        LOGE("Lua error: %s", err ? err : "(unknown)");
        lua_pop(L, 1);
        lua_close(L);
        return false;
    }

    LOGI("Lua executed OK: %s", path);
    lua_close(L);
    return true;
}
