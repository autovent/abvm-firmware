#include "ui/button.h"

#include <assert.h>
#include <stdint.h>

#include "drivers/pin.h"

Button::Button(Pin *p, uint32_t debounce_time_ms, bool active_low)
    : pin(p), active_low(active_low), debounce_time_ms(debounce_time_ms), state(State::UNPRESSED), pressed_time_ms(0) {}

Button::State Button::update() {
    switch (state) {
        case State::UNPRESSED:
            long_press_triggered = false;

            if (read_pin()) {
                pressed_time_ms = millis();
                state = State::DEBOUNCING;
            }
            break;
        case State::DEBOUNCING:
            if (!read_pin()) {
                state = State::UNPRESSED;
            } else if (held_for_more_than(debounce_time_ms)) {
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
            } else if (!long_press_triggered && held_for_more_than(long_press_time_ms)) {
                state = State::LONG_PRESS;
                long_press_triggered = true;
            }
            break;
        case State::LONG_PRESS:
            state = read_pin() ? State::HOLDING : State::RELEASED;
            break;
        case State::RELEASED:
            state = State::UNPRESSED;
            break;
        default:
            assert(false);  // This should never happen
            break;
    }

    return state;
}
