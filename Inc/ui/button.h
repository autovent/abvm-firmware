#pragma once

#include <stdint.h>

#include "clock.h"
#include "drivers/pin.h"

class Button {
public:
    enum class State {
        UNPRESSED,
        DEBOUNCING,
        PRESSED,
        HOLDING,
        LONG_PRESS,
        RELEASED,
    };

    Button(Pin *p, uint32_t debounce_time_ms = 10, bool active_low = false);

    State update();

    inline uint32_t time_pressed_ms() const {
        return pressed_time_ms;
    }

    inline uint32_t elapsed_time_ms() const {
        return time_since_ms(pressed_time_ms);
    }

    inline bool read_pin() const {
        return pin->read() ^ active_low;
    }

    inline bool is_pressed() const {
        return state == Button::State::PRESSED;
    }

    inline bool is_unpressed() const {
        return state == Button::State::UNPRESSED || state == Button::State::DEBOUNCING;
    }

    inline bool is_released() const {
        return state == Button::State::RELEASED;
    }

    inline bool is_holding() const {
        return state == Button::State::HOLDING || state == State::LONG_PRESS;
    }

    inline bool is_long_press() const {
        return state == State::LONG_PRESS;
    }

    inline bool is_long_press_triggered() const {
        return long_press_triggered;
    }

    inline bool held_for_more_than(uint32_t time_ms) const {
        return (state != Button::State::UNPRESSED) && (elapsed_time_ms() >= time_ms);
    }

    inline void set_long_press_time_ms(uint32_t time_ms) {
        long_press_time_ms = time_ms;
    }

private:
    // Configuration variables
    Pin *pin;
    bool active_low;
    uint32_t debounce_time_ms;
    uint32_t long_press_time_ms;

    // state variables
    State state;
    uint32_t pressed_time_ms;
    bool long_press_triggered;
};
