#pragma once
#include <stdint.h>

namespace rce {

void time_init(uint64_t start_ms);
float time_update(uint64_t now_ms); // returns dt in seconds

}
