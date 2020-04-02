/* Servo Winch Control for BVM squeezer
 *  by James Lott and Mike Maluk
 *  25 MAR 2020
 */

#include <Servo.h>
#include <Arduino.h>
#include "winch.h"
#include "current_sensor.h"
#include "pins.h"
#include "ad7780.h"
#include "motor.h"

#define MOTION_TYPE_MOTOR 1
#define MOTION_TYPE_SERVO 0
#define MOTION_TYPE       MOTION_TYPE_MOTOR

enum class Modes {
    CALIBRATION,
    RATE,
    MOTOR_TEST,
};

enum class BreathState {
    IDLE,
    INHALATION,
    PLATEAU,
    EXHALATION,
};

constexpr Modes mode = Modes::RATE;

constexpr uint32_t kCurrentUpdatePeriod_ms = 1;
constexpr uint32_t kPositionUpdate_ms = 20;           // Match the 50 freq of the servo motors
constexpr uint32_t kMeasurementUpdatePeriod_ms = 100; // Match the 50 freq of the servo motors

// Configuration parameters
// ! EDIT theses as needed
constexpr int32_t kSlowestBreathTime_ms = 6250;
constexpr int32_t kFastestBreathTime_ms = 2400;
constexpr float kIdlePositiong_deg = 0;
constexpr float kOpenPosition_deg = 0; // Change this to a value where the servo is just barely compressing the bag
constexpr float kMinClosedPosition_deg
    = 40; // Change this to a value where the servo has displaced the appropriate amount.
constexpr float kMaxClosedPosition_deg
    = 80; // Change this to a value where the servo has displaced the appropriate amount.

struct IERatio {
    float inspiration;
    float expiration;

    const float getInspirationPercent()
    {
        return inspiration / (inspiration + expiration);
    }
    float getExpirationPercent()
    {
        return expiration / (inspiration + expiration);
    }
};

// I : E Inspiration to Expiration Ratio
static IERatio kIERatio = {1, 2};
constexpr bool kInvertMotion = true; // Change this is the motor is inverted. This will reflect it 180

BreathState breath_state = BreathState::IDLE;

CurrentSensor current_sensor(current_pin);
AD7780 pressure(9);
AD7780 load_cell(8);

#if MOTION_TYPE == MOTION_TYPE_MOTOR
Motor motor(2, 1, 6, 7);
#else
Winch winch(servo_pin, 1);
#endif

void setup_motor()
{
    motor.set_velocity(0);
    pinMode(0, INPUT_PULLUP);
}

void setup_servo()
{
#if MOTION_TYPE == MOTION_TYPE_SERVO

    winch.invert = kInvertMotion;
    winch.init();

    winch.writeDegrees(kIdlePositiong_deg);
#endif
}

bool is_homing = true;
void setup()
{
    Serial.begin(115200);

    pinMode(red_led_pin, OUTPUT);
    pinMode(green_led_pin, OUTPUT);
    digitalWrite(red_led_pin, HIGH);
    digitalWrite(green_led_pin, HIGH);

    load_cell.init();
    pressure.init();
    pressure.measure();

    if (MOTION_TYPE == MOTION_TYPE_MOTOR) {
        is_homing = true;
        motor.is_pos_enabled = false;
        setup_motor();
    } else {
        setup_servo();
        delay(1);
    }
}

void calibrate()
{
    float pos_deg = map(analogRead(rate_in_pin), 0, 1023, 600, 2400); // scales values
    Serial.print(pos_deg, DEC);
    Serial.write("\n");
    // winch.writeMicroseconds(pos_deg);
}
static float m_pos_degrees = 0;

float update_rate()
{
    static float closed_position_deg = kMinClosedPosition_deg;
    // Read the potentiometer and determine the total length of breath
    float breath_length_ms
        = map(analogRead(rate_in_pin), 0, 1023, kSlowestBreathTime_ms + 100, kFastestBreathTime_ms); // scales values
    float next_closed_position_deg
        = map(analogRead(depth_in_pin), 0, 1023, kMinClosedPosition_deg, kMaxClosedPosition_deg); // scales values

    if ((breath_state != BreathState::IDLE && breath_length_ms > kSlowestBreathTime_ms + 50)
        || breath_length_ms > kSlowestBreathTime_ms) {
        breath_state = BreathState::IDLE;
    } else if ((breath_state != BreathState::INHALATION) && m_pos_degrees <= kOpenPosition_deg) {
        breath_state = BreathState::INHALATION;
        closed_position_deg = next_closed_position_deg;
    } else if ((breath_state != BreathState::EXHALATION) && (m_pos_degrees >= closed_position_deg)) {
        breath_state = BreathState::EXHALATION;
    }

    switch (breath_state) {
    case BreathState::IDLE:
        if (m_pos_degrees >= kIdlePositiong_deg) {
            m_pos_degrees += (kIdlePositiong_deg - closed_position_deg)
                * ((float)kPositionUpdate_ms / (kIERatio.getExpirationPercent() * breath_length_ms));
        }
        break;
    case BreathState::INHALATION:
        m_pos_degrees += (closed_position_deg - kOpenPosition_deg)
            * ((float)kPositionUpdate_ms / (kIERatio.getInspirationPercent() * breath_length_ms));
        break;
    case BreathState::EXHALATION:
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
    static float pressure_psi = 0;

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
                load_kg = (load_cell.read() * 10 / .0033) - .611; // 10kg fullscale at 3.3mV with a .611 kg offset

                pressure.measure();
            }
        }

        if (pressure.is_measuring) {
            if (pressure.update()) {
                // 14.85mV/psi
                pressure_psi = pressure.read() / .01485;
                load_cell.measure();
            }
        }

        // Print the currents for the current cycle now.
        // Format: time, closed pos, current, position
        Serial.print(now / 1000.0, DEC);
        Serial.write(",");
        Serial.print(load_kg, DEC);
        Serial.write(",");
        Serial.print(pressure_psi, DEC);
        Serial.write(",");
        Serial.print(motor.velocity, DEC);
        Serial.write(",");
        Serial.print(motor.position, DEC);
        Serial.write(",");
        Serial.print(current_sensor.get(), DEC);

        Serial.write("\r\n");
        last_time_meas_ms = now;
    }

    if (abs(now - last_time_ms) >= kPositionUpdate_ms) {

        if (is_homing) {
            // TODO breakout homing into its own routine?
            motor.set_velocity(-.5);
            if (!digitalRead(0)) {
                motor.zero();
                motor.set_pos(.0001);
                motor.is_pos_enabled = true;
                is_homing = false;
                m_pos_degrees = 0;
            }
        } else {
            if (!digitalRead(0)) {
                motor.zero();
                motor.set_pos(0.01);
            }

            motor.set_pos(2 * PI * update_rate() / 360);
        }

        last_time_ms = now;
    }
}