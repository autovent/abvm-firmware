#pragma once
#include <array>

class Alarms {
public:
    enum Name {
        LOSS_OF_POWER,  //
        OVER_PRESSURE,
        UNDER_PRESSURE,
        OVER_CURRENT,
        MOTION_FAULT,
        NUM_ALARMS
    };

    Alarms() : alarms_{} {}

    void set(Name n, bool s) {
        State &alarm_state = alarms_[n];
        if (alarm_state.is_alarmed != s) {
            alarm_state.is_alarmed = s;
            alarm_state.timestamp_ms = millis();
        }
    }

    uint32_t time_in_state(Name n) {
        return time_since_ms(alarms_[n].timestamp_ms);
    }

    void clear_all() {
        for (State &alarm : alarms_) {
            alarm.is_alarmed = false;
        }
    }

    bool is_alarmed(Name n) const {
        return alarms_[n].is_alarmed;
    }

    bool is_cleared(Name n) const {
        return !alarms_[n].is_alarmed;
    }

    bool is_any_alarmed() const {
        for (State const &alarm : alarms_) {
            if (alarm.is_alarmed) {
                return true;
            }
        }
        return false;
    }

    bool is_all_clear() const {
        return !is_any_alarmed();
    }

    Name get_highest_priority_alarm() const {
        for (uint8_t i = 0; i < NUM_ALARMS; i++) {
            if (alarms_[i].is_alarmed) {
                return (Name)i;
            }
        }
    }

private:
    struct State {
        bool is_alarmed;
        uint32_t timestamp_ms;
    };

    std::array<State, NUM_ALARMS> alarms_;
};