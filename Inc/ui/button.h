#pragma once

#include "clock.h"
#include "drivers/pin.h"

#include <stdint.h>
#include <assert.h>

class Button {
public:
    enum class State {
        UNPRESSED,
        DEBOUNCING,
        PRESSED,
        HOLDING,
        RELEASED,
    };

    struct Event {
        State state;
        uint32_t held_time_ms;
    };

    Button(Pin *p, uint32_t debounce_time_ms = 10, bool active_low = false)
        : pin(p),
          active_low(active_low),
          debounce_time_ms(debounce_time_ms),
          state(State::UNPRESSED),
          pressed_time_ms(0) {}

    Event update() {
        switch (state) {
            case State::UNPRESSED:
                if (read_pin()) {
                    pressed_time_ms = millis();
                    state = State::DEBOUNCING;
                }
                break;
            case State::DEBOUNCING:
                if (!read_pin()) {
                    state = State::UNPRESSED;
                } else if (elapsed_time_ms() >= debounce_time_ms) {
                    state = State::PRESSED;
                }
                break;
            case State::PRESSED:
                if (!read_pin()) {
                    state = State::RELEASED;
                } else {
                    state = State::HOLDING;
                }
                break;
            case State::HOLDING:
                if (!read_pin()) {
                    state = State::RELEASED;
                }
                break;
            case State::RELEASED:
                state = State::UNPRESSED;
                break;
            default:
                assert(false);  // This should never happen
                break;
        }

        uint32_t held_time = (state == State::UNPRESSED) ? 0 : elapsed_time_ms();
        return {state, held_time};
    }

    inline uint32_t time_pressed_ms() {
        return pressed_time_ms;
    }

    inline uint32_t elapsed_time_ms() {
        return time_since_ms(pressed_time_ms);
    }

    inline bool read_pin() {
        return pin->read() ^ active_low;
    }

private:
    // Configuration variables
    Pin *pin;
    bool active_low;
    uint32_t debounce_time_ms;

    // state variables
    State state;
    uint32_t pressed_time_ms;
};
