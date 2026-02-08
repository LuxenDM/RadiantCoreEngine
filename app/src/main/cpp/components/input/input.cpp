#include "input/input.h"

namespace input {

InputState::InputState()
: pointer_down_(false), pointer_x_(0.0f), pointer_y_(0.0f) {}

InputState::~InputState() = default;

void InputState::begin_frame() {
    events_.clear();
}

bool InputState::pointer_is_down() const { return pointer_down_; }
float InputState::pointer_x() const { return pointer_x_; }
float InputState::pointer_y() const { return pointer_y_; }

const std::vector<PointerEvent>& InputState::events() const {
    return events_;
}

void InputState::push_pointer_down(int32_t id, float x, float y) {
    pointer_down_ = true;
    pointer_x_ = x; pointer_y_ = y;
    events_.push_back({EventType::PointerDown, id, x, y});
}

void InputState::push_pointer_up(int32_t id, float x, float y) {
    pointer_down_ = false;
    pointer_x_ = x; pointer_y_ = y;
    events_.push_back({EventType::PointerUp, id, x, y});
}

void InputState::push_pointer_move(int32_t id, float x, float y) {
    pointer_x_ = x; pointer_y_ = y;
    events_.push_back({EventType::PointerMove, id, x, y});
}

} // namespace input
