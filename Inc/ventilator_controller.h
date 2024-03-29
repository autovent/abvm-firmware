#pragma once

#include "config.h"
#include "controls/motion_planner.h"
#include "drivers/sensor.h"
#include "math/conversions.h"
#include "math/dsp.h"
#include "servo.h"

class VentilatorController {
public:
    enum class State {
        IDLE = 0,
        GO_TO_START,
        INSPIRATION,
        INSPIRATORY_HOLD,
        EXPIRATION,
        GO_TO_IDLE,
        NUM_STATES,
    };

    VentilatorController(IMotionPlanner *motion, Servo *motor, ISensor *pressure, float tv_settings[],
                         uint32_t num_tv_settings, float bpm_settings[], uint32_t num_bpm_settings);
    void start();
    void stop();
    float update();

    void reset();

    bool is_running() {
        return is_operational;
    }

    inline void bump_tv(int i) {
        next_tv_idx = saturate(next_tv_idx + i, 0, num_tv_settings - 1);
    }

    inline uint8_t get_tv_idx() {
        return next_tv_idx;
    }

    inline void bump_rate(int i) {
        next_rate_idx = saturate(next_rate_idx + i, 0, num_rate_settings - 1);
    }
    inline float get_closed_pos() {
        return tidal_volume_settings[get_tv_idx()];
    }

    inline float get_open_pos() {
        return kVentMotionConfig.open_pos_deg;
    }

    inline uint8_t get_rate_idx() {
        return next_rate_idx;
    }

    inline float get_rate() {
        return rate_settings[get_rate_idx()];
    }

    float get_peak_pressure_cmH2O();
    float get_plateau_pressure_cmH2O();
    inline float get_peak_pressure_limit_cmH2O() const {
        return peak_pressure_limit_cmH2O;
    }

    float get_pressure_cmH2O() {
        return pressure_cmH2O;
    }

    void increment_peak_pressure_limit_cmH2O(float x) {
        peak_pressure_limit_cmH2O =
              saturate(peak_pressure_limit_cmH2O + x, kVentRespirationConfig.peak_pressure_display_min,
                       kVentRespirationConfig.peak_pressure_display_max);
    }

private:
    static constexpr bool INSPIRATION_RATIO = false;
    static constexpr bool EXPIRATION_RATIO = true;

    IMotionPlanner *motion;
    Servo *motor;
    ISensor *pressure_sensor;

    State state;

    bool is_measure_plateau_cycle;

    uint8_t num_tv_settings = 6;
    float *tidal_volume_settings;

    uint8_t num_rate_settings = 6;
    float *rate_settings;

    uint8_t current_tv_idx;
    uint8_t next_tv_idx;

    uint8_t current_rate_idx;
    uint8_t next_rate_idx;

    bool is_operational;

    bool is_fast_open = false;

    float current_peak_pressure_cmH2O;
    float last_peak_pressure_cmH2O;

    float last_plateau_pressure;
    float current_plateau_pressure;

    float pressure_cmH2O = 0;
    float peak_pressure_limit_cmH2O;

    uint32_t plateau_time;

    float inspiration_time() const;

    float expiration_time() const;
};
