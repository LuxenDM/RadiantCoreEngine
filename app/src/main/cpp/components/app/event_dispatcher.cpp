#include "app/event_dispatcher.h"

namespace rce {

static std::unordered_map<EPType, std::vector<EPCallback>> g_listeners;

void ep_subscribe(EPType type, EPCallback cb) {
    g_listeners[type].push_back(std::move(cb));
}

void ep_dispatch_all_p2e() {
    EPMsg msg;
    while (ep_poll_p2e(&msg)) {
        auto it = g_listeners.find(msg.type);
        if (it != g_listeners.end()) {
            for (auto& fn : it->second) {
                fn(msg);
            }
        }
    }
}

}
