#pragma once
#include <functional>
#include <vector>
#include <unordered_map>
#include "app/event_pipe.h"

namespace rce {

using EPCallback = std::function<void(const EPMsg&)>;

void ep_subscribe(EPType type, EPCallback cb);
void ep_dispatch_all_p2e();  // drain pipe + notify subscribers

}
