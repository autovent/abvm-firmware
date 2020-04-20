#pragma once

#include <assert.h>

#include "clock.h"
#include "control_panel.h"
#include "math/dsp.h"
#include "ui/iui.h"

class UI_V1 : public IUI {
public:
    enum class View { ADJUST, PRESSURE, PRESSURE_LIMIT_ADJUST};
    enum class AudioAlert {
        DONE_HOMING,
        STARTING,
        STOPPING,
        ALARM_1,
        ALARM_2,
        ALARM_3,
    };

    UI_V1(ControlPanel *controls);

    virtual ~UI_V1() = default;

    void init();
    void set_audio_alert(AudioAlert alert);

    void handle_buzzer();
    Event update();
    void set_state(State s);
    void set_view(View v);
    void toggle_view();
    void set_value(DisplayValue param, float value);
    void set_alarm(Alarm a);

private:

    ControlPanel *controls;
    bool is_bootloader_issued = false;
    uint32_t bootloader_time_ms = 5000;
    bool disallow_start = false;
    View view;

    AudioAlert current_alert;
    bool audio_alert_in_progress;
    uint32_t audio_alert_start_time_ms;

    float display_values[kNumDisplayValues];

    bool is_bootloader_event();
    void enter_pressure_limit_view();

};