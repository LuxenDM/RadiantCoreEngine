#pragma once
#include <stdint.h>
#include <android/native_activity.h>

enum RCE_RotationBits : uint32_t {
  RCE_ROT_0   = 1u << 0,
  RCE_ROT_90  = 1u << 1,
  RCE_ROT_180 = 1u << 2,
  RCE_ROT_270 = 1u << 3,
};

#ifdef __cplusplus
extern "C" {
#endif

void rce_android_bind_activity(ANativeActivity* activity);
void rce_android_set_allowed_rotations(uint32_t allowed_mask);

#ifdef __cplusplus
}
#endif
