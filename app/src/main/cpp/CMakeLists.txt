cmake_minimum_required(VERSION 3.22.1)
project(mylua_android)

# =========================
# Lua 5.1.4 (static library)
# =========================
add_library(lua STATIC
    third_party/lua/lapi.c
    third_party/lua/lauxlib.c
    third_party/lua/lbaselib.c
    third_party/lua/lcode.c
    third_party/lua/ldblib.c
    third_party/lua/ldebug.c
    third_party/lua/ldo.c
    third_party/lua/ldump.c
    third_party/lua/lfunc.c
    third_party/lua/lgc.c
    third_party/lua/linit.c
    third_party/lua/liolib.c
    third_party/lua/llex.c
    third_party/lua/lmathlib.c
    third_party/lua/lmem.c
    third_party/lua/loadlib.c
    third_party/lua/lobject.c
    third_party/lua/lopcodes.c
    third_party/lua/loslib.c
    third_party/lua/lparser.c
    third_party/lua/lstate.c
    third_party/lua/lstring.c
    third_party/lua/lstrlib.c
    third_party/lua/ltable.c
    third_party/lua/ltablib.c
    third_party/lua/ltm.c
    third_party/lua/lundump.c
    third_party/lua/lvm.c
    third_party/lua/lzio.c
)

target_include_directories(lua PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/third_party/lua
)

# IMPORTANT: Lua 5.1 + Android bionic fortified libc can crash in luaopen_os
# without disabling FORTIFY for this target.
target_compile_definitions(lua PUBLIC LUA_USE_LINUX LUA_USE_POSIX)
target_compile_options(lua PRIVATE -U_FORTIFY_SOURCE -D_FORTIFY_SOURCE=0)

# =================================
# Core (portable static library)
# =================================
add_library(mylua_core STATIC
    components/luax/lua_runtime.cpp
    components/gfx/egl_renderer.cpp
    
	components/app/paths.cpp
	components/app/event_pipe.cpp
	components/app/event_dispatcher.cpp
	components/app/engine.cpp
	
	components/input/input.cpp
)

target_include_directories(mylua_core PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}/third_party/lua
)

target_link_libraries(mylua_core PUBLIC lua)

# Optional: keep the core strict so Android headers don't leak in later.
# (You can comment this out if it’s annoying early on.)
# target_compile_options(mylua_core PRIVATE -Werror)

# =================================
# Android shared library (.so)
# =================================
add_library(mylua_android SHARED
    platform/android/app_init.cpp
	platform/android/android_runtime.cpp
    platform/android/native_app_glue_include.c
    platform/android/paths_android.cpp
	platform/android/input_android.cpp
    platform/android/ui_insets_jni.cpp
	platform/android/presentation_android.cpp
)

target_include_directories(mylua_android PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${ANDROID_NDK}/sources/android/native_app_glue
)

find_library(log-lib log)
find_library(android-lib android)
find_library(egl-lib EGL)
find_library(gles-lib GLESv3)

target_link_libraries(mylua_android
    mylua_core
    ${log-lib}
    ${android-lib}
    ${egl-lib}
    ${gles-lib}
)
