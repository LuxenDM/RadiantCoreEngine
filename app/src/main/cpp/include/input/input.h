#pragma once
#include <cstdint>
#include <vector>

namespace input {

enum class EventType : uint8_t {
    PointerDown,
    PointerUp,
    PointerMove,
};

struct PointerEvent {
    EventType type;
    int32_t pointer_id;
    float x;
    float y;
};

class InputState {
public:
    InputState();
    ~InputState();

    void begin_frame();

    bool pointer_is_down() const;
    float pointer_x() const;
    float pointer_y() const;

    const std::vector<PointerEvent>& events() const;

    // These are what platform layers call.
    void push_pointer_down(int32_t id, float x, float y);
    void push_pointer_up(int32_t id, float x, float y);
    void push_pointer_move(int32_t id, float x, float y);

private:
    bool pointer_down_;
    float pointer_x_;
    float pointer_y_;
    std::vector<PointerEvent> events_;
};

} // namespace input
