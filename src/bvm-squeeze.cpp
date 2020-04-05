/* Servo Winch Control for BVM squeezer
 *  by James Lott and Mike Maluk
 *  25 MAR 2020
 */

#include <Servo.h>
#include <Arduino.h>
#include "current_sensor.h"
#include "pins.h"
#include "ad7780.h"
#include "motor.h"
#define MOTOR_ROBOTZONE_16RPM
#include "config.h"

enum class BreathState {
    IDLE,
    INSPIRATION,
    EXPIRATION,
};

BreathState breath_state = BreathState::IDLE;

CurrentSensor current_sensor(current_pin);
AD7780 pressure(pressure_sense_pwdn_pin);
AD7780 load_cell(load_cell_sense_pwdn_pin);

Motor motor(.001f * kCurrentUpdatePeriod_ms,
            motor_pwm_pin,
            motor_dir_pin,
            motor_enca_pin,
            motor_encb_pin,
            kMotorParams,
            kMotorSpeedPidParams,
            kMotorPosPidParams);

static float m_pos_degrees = 0;

static float breath_length_ms = 0;
static float closed_position_deg = kMinClosedPosition_deg;
static bool is_homing = true;

void setup_motor()
{
    is_homing = true;
    motor.is_pos_enabled = false;
    motor.set_velocity(0);
    pinMode(homing_switch_pin, INPUT_PULLUP);
}

void setup()
{
    Serial.begin(115200);

pinMode(red_led_pin, OUTPUT);
    pinMode(green_led_pin, OUTPUT);
    digitalWrite(red_led_pin, HIGH);
    digitalWrite(green_led_pin, HIGH);

    pinMode(pos_out_pwm_pin, OUTPUT);
    pinMode(breathstate_out_pin, OUTPUT);
    load_cell.init();
    pressure.init();
    pressure.measure();

    setup_motor();
}

void calibrate()
{
    float pos_deg = map(analogRead(rate_in_pin), 0, 1023, 600, 2400); // scales values
    Serial.print(pos_deg, DEC);
    Serial.write("\n");
}
float update_position()
{
    // Read the potentiometer and determine the total length of breath
    breath_length_ms
        = map(analogRead(rate_in_pin), 0, 1 << 12, kSlowestBreathTime_ms + 100, kFastestBreathTime_ms); // scales values
    float next_closed_position_deg
        = map(analogRead(depth_in_pin), 0, 1 << 12, kMinClosedPosition_deg, kMaxClosedPosition_deg); // scales values

    if ((breath_state != BreathState::IDLE && breath_length_ms > kSlowestBreathTime_ms + 50)
        || breath_length_ms > kSlowestBreathTime_ms) {
        breath_state = BreathState::IDLE;
    } else if ((breath_state != BreathState::INSPIRATION) && m_pos_degrees <= kOpenPosition_deg) {
        breath_state = BreathState::INSPIRATION;
        closed_position_deg = next_closed_position_deg;
    } else if ((breath_state != BreathState::EXPIRATION) && (m_pos_degrees >= closed_position_deg)) {
        breath_state = BreathState::EXPIRATION;
    }

    switch (breath_state) {
    case BreathState::IDLE:
        if (m_pos_degrees >= kIdlePositiong_deg) {
            m_pos_degrees += (kIdlePositiong_deg - closed_position_deg)
                * ((float)kPositionUpdate_ms / (kIERatio.getExpirationPercent() * breath_length_ms));
        }
        break;
    case BreathState::INSPIRATION:
        m_pos_degrees += (closed_position_deg - kOpenPosition_deg)
            * ((float)kPositionUpdate_ms / (kIERatio.getInspirationPercent() * breath_length_ms));
        break;
    case BreathState::EXPIRATION:
        m_pos_degrees += (kOpenPosition_deg - closed_position_deg)
            * ((float)kPositionUpdate_ms / (kIERatio.getExpirationPercent() * breath_length_ms));
        break;
    default:
        break;
    }
    return m_pos_degrees;
}

void loop()
{
    static uint32_t last_time_ms = millis();
    static uint32_t last_time_current_ms = millis();
    static uint32_t last_time_meas_ms = millis();

    static float load_kg = 0;
    static float pressure_cmH2O = 0;

    uint32_t now = millis();
    // Once a millisecond
    if (abs(now - last_time_current_ms) >= kCurrentUpdatePeriod_ms) {
        current_sensor.update();
        motor.update();
        last_time_current_ms = now;
    }

    if (abs(now - last_time_meas_ms) >= kMeasurementUpdatePeriod_ms) {
        // TODO: cleanup this measurement code
        if (load_cell.is_measuring) {
            if (load_cell.update()) {
                load_kg
                    = (load_cell.read() * 10 / .0033) - .611 + .528; // 10kg fullscale at 3.3mV with a .611 kg offset

                pressure.measure();
            }
        }

        if (pressure.is_measuring) {
            if (pressure.update()) {
                // 14.85mV/psi
                pressure_cmH2O = (0.24 + pressure.read() / .01485) * 70.306957829636;
                load_cell.measure();
            }
        }

        // Print the currents for the current cycle now.
        // Format: time, closed pos, current, position
        Serial.print(now / 1000.0, DEC);
        Serial.write(",");
        Serial.print(load_kg, DEC);
        Serial.write(",");
        Serial.print(pressure_cmH2O, DEC);
        Serial.write(",");
        Serial.print(motor.velocity, DEC);
        Serial.write(",");
        Serial.print(motor.target_velocity, DEC);
        Serial.write(",");
        Serial.print(motor.position, DEC);
        Serial.write(",");
        Serial.print(motor.target_pos, DEC);
        Serial.write(",");
        Serial.print(current_sensor.get(), DEC);
        Serial.write(",");
        Serial.print(breath_length_ms, DEC);
        Serial.write(",");
        Serial.print(closed_position_deg, DEC);
        Serial.write(",");
        Serial.print(kOpenPosition_deg, DEC);
        Serial.write("\r\n");
        last_time_meas_ms = now;
    }

    if (abs(now - last_time_ms) >= kPositionUpdate_ms) {
        if (!digitalRead(vdc_in_pin)) {
            is_homing = true;
            motor.set_velocity(0);
            motor.set_pwm(0);
            motor.reset();            
            breath_state = BreathState::IDLE;
        } else {

            if (is_homing) {
                motor.is_pos_enabled = false;

                // TODO breakout homing into its own routine?
                motor.set_velocity(-.14);
                static uint8_t home_counter = 0;
                if ((current_sensor.get() > .2f /*A*/)) {
                    home_counter++;
                } else {
                    home_counter = 0;
                }

                if (home_counter > 20) {
                    home_counter = 0;
                    motor.reset();
                    motor.zero();
                    motor.set_pos(0);
                    motor.is_pos_enabled = true;
                    is_homing = false;
                    m_pos_degrees = 0;
                    breath_state = BreathState::IDLE;
                }
            } else {
                motor.set_pos(2 * PI * update_position() / 360);

                analogWrite(pos_out_pwm_pin, (motor.maxPwmValue()+1) * motor.position / (PI/2));
                digitalWrite(breathstate_out_pin, breath_state == BreathState::INSPIRATION);
            }
        }
        

        last_time_ms = now;
    }
}