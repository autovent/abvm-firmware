/* Servo Winch Control for BVM squeezer
 *  by James Lott and Mike Maluk
 *  25 MAR 2020
 */

#include <Servo.h>
#include <Arduino.h>
#include "winch.h"
#include "current_sensor.h"
#include "pins.h"

enum class Modes
{
  CALIBRATION,
  RATE,
};

enum class BreathState
{
  IDLE,
  INHALATION,
  PLATEAU,
  EXHALATION,
};

constexpr Modes mode = Modes::RATE;

constexpr uint32_t kCurrentUpdatePeriod_ms = 1;
constexpr uint32_t kServoUpdatePeriod_ms = 20; // Match the 50 freq of the servo motors

// Configuration parameters
// ! EDIT theses as needed
constexpr int32_t kSlowestBreathTime_ms = 6250;
constexpr int32_t kFastestBreathTime_ms = 2400;
constexpr float kIdlePositiong_deg = 2;
constexpr float kOpenPosition_deg = 10;    // Change this to a value where the servo is just barely compressing the bag
constexpr float kMinClosedPosition_deg = 90; // Change this to a value where the servo has displaced the appropriate amount.
constexpr float kMaxClosedPosition_deg = 180; // Change this to a value where the servo has displaced the appropriate amount.
constexpr float kExpiration_percents = .66666;
constexpr float kInspiration_percent = .33333;
constexpr bool kInvertMotion = true; // Change this is the motor is inverted. This will reflect it 180

BreathState breath_state = BreathState::IDLE;
Winch winch(servo_pin);
CurrentSensor current_sensor(current_pin);

void setup()
{
  Serial.begin(9600);

  pinMode(red_led_pin, OUTPUT);
  pinMode(green_led_pin, OUTPUT);
  digitalWrite(red_led_pin, LOW);
  digitalWrite(green_led_pin, HIGH);

  winch.invert = kInvertMotion;
  winch.init();

  winch.writeDegrees(kIdlePositiong_deg);

  delay(1);
}

void calibrate()
{
  float pos_deg = map(analogRead(rate_in_pin), 0, 1023, 600, 2400); // scales values
  Serial.print(pos_deg, DEC);
  Serial.write("\n");
  winch.writeMicroseconds(pos_deg);
}

void squeeze()
{
  static uint32_t last_time_ms = millis();
  static uint32_t last_time_current_ms = millis();
  static float m_pos_degrees = kIdlePositiong_deg;
  static float closed_position_deg = kMinClosedPosition_deg;
  uint32_t now = millis();
  // Once a millisecond
  if (abs(now - last_time_current_ms) >= kCurrentUpdatePeriod_ms)
  {
    current_sensor.update();
    last_time_current_ms = now;
  }

  if (abs(now - last_time_ms) >= kServoUpdatePeriod_ms)
  {
    // Print the currents for now.
    Serial.print(analogRead(depth_in_pin), DEC);
        Serial.write(", ");

    Serial.print(current_sensor.get(), DEC);
    Serial.write(", ");
    Serial.println(m_pos_degrees, DEC);

    // Read the potentiometer and determine the total length of breath
    float breath_length_ms = map(analogRead(rate_in_pin), 0, 1023, kSlowestBreathTime_ms + 100, kFastestBreathTime_ms); // scales values
    float next_closed_position_deg = map(analogRead(depth_in_pin), 0, 1023, kMinClosedPosition_deg, kMaxClosedPosition_deg); // scales values

    if ((breath_state != BreathState::IDLE && breath_length_ms > kSlowestBreathTime_ms + 50) || breath_length_ms > kSlowestBreathTime_ms)
    {
      breath_state = BreathState::IDLE;
    }
    else if ((breath_state != BreathState::INHALATION) && m_pos_degrees <= kOpenPosition_deg)
    {
      Serial.write("Starting inhalation at: ");
      Serial.println(now, DEC);
      breath_state = BreathState::INHALATION;
      closed_position_deg = next_closed_position_deg;
    }
    else if ((breath_state != BreathState::EXHALATION) && (m_pos_degrees >= closed_position_deg))
    {
      Serial.write("Starting exhalation at: ");
      Serial.println(now, DEC);
      breath_state = BreathState::EXHALATION;
    }

    switch (breath_state)
    {
    case BreathState::IDLE:
      if (m_pos_degrees >= kIdlePositiong_deg)
      {
        m_pos_degrees += (kIdlePositiong_deg - closed_position_deg) * ((float)kServoUpdatePeriod_ms / (kExpiration_percents * breath_length_ms));
      }
      break;
    case BreathState::INHALATION:
      m_pos_degrees += (closed_position_deg - kOpenPosition_deg) * ((float)kServoUpdatePeriod_ms / (kInspiration_percent * breath_length_ms));
      break;
    case BreathState::EXHALATION:
      m_pos_degrees += (kOpenPosition_deg - closed_position_deg) * ((float)kServoUpdatePeriod_ms / (kExpiration_percents * breath_length_ms));
      break;
    default:
      break;
    }

    winch.writeDegrees(m_pos_degrees);
    last_time_ms = now;
  }
}

void loop()
{
  if (mode == Modes::CALIBRATION)
  {
    calibrate();
  }
  else
  {
    squeeze();
  }
}