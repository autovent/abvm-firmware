/* Servo Winch Control for BVM squeezer
 *  by James Lott and Mike Maluk
 *  25 MAR 2020
 */

#include <Arduino.h>
#include <Servo.h>

#include "ad7780.h"
#include "current_sensor.h"
#include "motor.h"
#include "pins.h"
#include "sensirion_sfm4100.h"
#include "trajectory/trap_percent_time.h"

#define MOTOR_GOBILDA_30RPM
#define CONFIG_LONG_SPIRIT_FINGERS
// #define CONFIG_USE_FLOW_METER
#include "config.h"

enum class BreathState {
  IDLE,
  TO_START,
  INSPIRATION,
  PLATEAU,
  EXPIRATION,
};

BreathState breath_state = BreathState::IDLE;

#ifdef CONFIG_USE_FLOW_METER
Sensirion_SFM4100 flow_meter(.01);
#endif

CurrentSensor current_sensor(current_pin);
AD7780 pressure(pressure_sense_pwdn_pin);
AD7780 load_cell(load_cell_sense_pwdn_pin);

Motor motor(.001f * kCurrentUpdatePeriod_ms, motor_pwm_pin, motor_dir_pin,
            motor_enca_pin, motor_encb_pin, kMotorParams, kMotorVelPidParams,
            kMotorVelLimits, kMotorPosPidParams, kMotorPosLimits);

static float m_pos_degrees = 0;

static float breath_length_ms = kSlowestBreathTime_ms;
static float closed_position_deg = kMinClosedPosition_deg;
static bool is_homing = true;

static float next_closed_position_deg = kMinClosedPosition_deg;
void setup_motor() {
  is_homing = true;
  motor.is_pos_enabled = false;
  motor.set_velocity(0);
  motor.set_pwm(0);
  motor.reset();
  pinMode(homing_switch_pin, INPUT_PULLUP);
}

void setup() {
  setup_motor();

  Serial.begin(115200);

  pinMode(red_led_pin, OUTPUT);
  pinMode(green_led_pin, OUTPUT);
  digitalWrite(red_led_pin, HIGH);
  digitalWrite(green_led_pin, HIGH);

  pinMode(pos_out_pwm_pin, OUTPUT);
  pinMode(breathstate_out_pin, OUTPUT);
  load_cell.init();
  pressure.init();
  load_cell.measure();
  current_sensor.zero();
}

Trap_PercentTime planner({.3, .5}, kPositionUpdate_ms);

float update_position_trap() {
  bool go_to_idle = (breath_length_ms > (kSlowestBreathTime_ms + 50)) ||
                    (breath_length_ms > kSlowestBreathTime_ms);

  if (planner.is_idle()) {
    if (go_to_idle) {
      breath_state = BreathState::IDLE;
      planner.set_next({kIdlePositiong_deg, kTimeToIdle_ms});
    } else {
      switch (breath_state) {
        case BreathState::IDLE:
          breath_state = BreathState::TO_START;
          planner.set_next({kOpenPosition_deg, kTimeToIdle_ms});
          break;
        case BreathState::TO_START:
          breath_state = BreathState::INSPIRATION;
          planner.set_next({kOpenPosition_deg, kTimeToIdle_ms});
          break;
        case BreathState::INSPIRATION:
          breath_state = BreathState::PLATEAU;
          closed_position_deg = next_closed_position_deg;
#ifdef CONFIG_USE_FLOW_METER
          flow_meter.reset();
#endif
          planner.set_next(
              {closed_position_deg,
               kIERatio.getInspirationPercent() * breath_length_ms});
          break;
        case BreathState::PLATEAU:
          breath_state = BreathState::EXPIRATION;
          planner.set_next({closed_position_deg, kPlateauTime_ms});
          break;
        case BreathState::EXPIRATION:
          breath_state = BreathState::INSPIRATION;

          planner.set_next(
              {kOpenPosition_deg,
               (kIERatio.getExpirationPercent() * breath_length_ms) -
                   kPlateauTime_ms});
          break;

        default:
          break;
      }
    }
  }

  digitalWrite(breathstate_out_pin, breath_state == BreathState::INSPIRATION);
  digitalWrite(plateau_out_pin, breath_state == BreathState::PLATEAU);

  m_pos_degrees = planner.run(m_pos_degrees);
  return m_pos_degrees;
}

static uint32_t error_overcurrent = 0;
void loop() {
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
    analogWrite(pos_out_pwm_pin,
                (motor.maxPwmValue() + 1) * motor.position / (PI / 2));
  }

  if (abs(now - last_time_meas_ms) >= kMeasurementUpdatePeriod_ms) {
    // // TODO: cleanup this measurement code
    if (load_cell.is_measuring) {
      if (load_cell.update()) {
        load_kg = (load_cell.read() * 10 / .0033) -
                  1;  // 10kg fullscale at 3.3mV with a .611 kg offset

        pressure.measure();
      }
    }

    if (pressure.is_measuring) {
      if (pressure.update()) {
        // 14.85mV/psi
        pressure_cmH2O = (pressure.read() / .01485) * -70.306957829636 -12;
        load_cell.measure();
      }
    }

#ifdef CONFIG_USE_FLOW_METER
    flow_meter.recv();
    flow_meter.start();
#endif
    // Print the currents for the current cycle now.
    // Format: time, closed pos, current, position
    Serial.print(now / 1000.0, 3);
    Serial.write(",");
    Serial.print(load_kg, 3);
    Serial.write(",");
    Serial.print(pressure_cmH2O, 3);
    Serial.write(",");
    Serial.print(motor.velocity, 6);
    Serial.write(",");
    Serial.print(motor.target_velocity, 6);
    Serial.write(",");
    Serial.print(motor.position, 2);
    Serial.write(",");
    Serial.print(motor.target_pos, 2);
    Serial.write(",");
    Serial.print(current_sensor.get(), 3);
    Serial.write(",");
    Serial.print(breath_length_ms, 0);
    Serial.write(",");
    Serial.print(closed_position_deg, 0);
    Serial.write(",");
    Serial.print(kOpenPosition_deg, 0);
    Serial.write(",");
    Serial.print(motor.faults.to_int(), DEC);
    Serial.write(",");
#ifdef CONFIG_USE_FLOW_METER
    Serial.print(flow_meter.get_mass_flow(), 2);
    Serial.write(",");
    Serial.print(flow_meter.get_cc_per_sec(), 2);
#else
    Serial.write("0,0");
#endif

    Serial.write("\r\n");
    last_time_meas_ms = now;
  }

  if (abs(now - last_time_ms) >= kPositionUpdate_ms) {
    // Read the potentiometer and determine the total length of breath
    breath_length_ms =
        map(analogRead(rate_in_pin), 0, 1 << 12, kSlowestBreathTime_ms + 100,
            kFastestBreathTime_ms);  // scales values
    next_closed_position_deg =
        map(analogRead(depth_in_pin), 0, 1 << 12, kMinClosedPosition_deg,
            kMaxClosedPosition_deg);  // scales values

    if (!digitalRead(vdc_in_pin)) {
      is_homing = true;
      is_homing = true;
      motor.is_pos_enabled = false;
      motor.set_velocity(0);
      motor.set_pwm(0);
      motor.reset();
      current_sensor.zero();
      breath_state = BreathState::IDLE;
    } else {
      if (current_sensor.get() > 9.5) {
        if (++error_overcurrent > 10) {
          motor.faults.overcurrent = true;
        }
      } else {
        error_overcurrent = 0;
      }
      if (is_homing) {
        motor.is_pos_enabled = false;

        // TODO breakout homing into its own routine?
        motor.set_velocity(-.1);
        if (current_sensor.get() > .35) {
          if (++error_overcurrent > 10) {
            motor.faults.overcurrent = true;
          }
        } else {
          error_overcurrent = 0;
        }
        if (!digitalRead(homing_switch_pin)) {
          motor.reset();
          motor.zero();
          planner.reset();
          motor.set_pos(0);
          motor.set_pwm(0);
          motor.pos_pid.reset();
          motor.is_pos_enabled = true;
          is_homing = false;
          breath_state = BreathState::IDLE;
          m_pos_degrees = motor.position;
          planner.set_next({kIdlePositiong_deg, kTimeToIdle_ms});
          // TODO: zero the loadcell
          motor.set_pos(2 * PI * update_position_trap() / 360);
        }
        // }
      } else {
        motor.set_pos(2 * PI * update_position_trap() / 360);

        digitalWrite(breathstate_out_pin,
                     breath_state == BreathState::INSPIRATION);
      }
    }

    last_time_ms = now;
  }
}