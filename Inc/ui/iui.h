#pragma once

#include <stdint.h>
#include <stdlib.h>

// UI Interface
class IUI {
public:
    enum class State { STOPPED, STARTED, ALARMED };

    enum class Event {
        NONE = 0,               /// No event to emit
        START,                  /// Start event: HOLD the start button
        STOP,                   /// Stop : PRESS  the  stop button
        SILENCE_ALARM,          /// Silence the alarm: PRESS AND HOLD the stop button
        TIDAL_VOLUME_UP,        /// Increment Tidal Volume
        TIDAL_VOLUME_DOWN,      /// Decrement Tidal Volume
        RESPIRATORY_RATE_UP,    /// Increment Respiratory Rate
        RESPIRATORY_RATE_DOWN,  /// Decrement Respiratory Rate
        PRESSURE_LIMIT_UP,      /// Increment Pressure Limit
        PRESSURE_LIMIT_DOWN,    /// Decrement Pressure Limit
        GO_TO_BOOTLOADER,
        NUM_EVENTS //! KEEP AT END OF  LIST
    };

    enum class DisplayValue {
        TIDAL_VOLUME,
        RESPIRATORY_RATE,
        PLATEAU_PRESSURE,
        PEAK_PRESSURE,
        PEAK_PRESSURE_ALARM,
        NUM_DISPLAY_VALUES //! KEEP AT END OF  LIST
    };

    enum class Alarm {
        NUM_ALARMS,
    };

    static constexpr size_t kNumDisplayValues = (size_t)DisplayValue::NUM_DISPLAY_VALUES;
    static constexpr size_t kNumEvents = (size_t)Event::NUM_EVENTS;
    static constexpr size_t kNumAlarms = (size_t)Alarm::NUM_ALARMS;

    virtual ~IUI() = default;

    virtual void init() = 0;
    virtual Event update() = 0;

    virtual void set_state(State s) = 0;

    virtual void set_value(DisplayValue param, float value) = 0;

    virtual void set_alarm(Alarm a) = 0;
};