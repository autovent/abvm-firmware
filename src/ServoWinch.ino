/* Servo Winch Control for BVM squeezer
 *  by James Lott and Mike Maluk
 *  25 MAR 2020
 */

#include <Servo.h>
#include <Arduino.h>
#include "winch.h"
#include "circular_buffer.h"
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
constexpr int32_t slowest_breath_length_ms = 6250;
constexpr int32_t fastest_breath_length_ms = 2400;
constexpr float idle_deg = 5;  
constexpr float open_deg = 50;    // Change this to a value where the servo is just barely compressing the bag
constexpr float closed_deg = 140; // Change this to a value where the servo has displaced the appropriate amount.
constexpr float expiration_percent = .66666;
constexpr float inspiration_percent = .33333;
constexpr bool invert_motion = false; // Change this is the motor is inverted. This will reflect it 180

uint32_t last_time_ms = 0;
uint32_t last_time_current_ms = 0;


float m_pos_degrees = 0;

BreathState breath_state = BreathState::IDLE;
Winch winch(servo_pin);

CircularBuffer<float, 60, true> current_buffer;

void setup()
{
  Serial.begin(9600);

  pinMode(red_led_pin, OUTPUT);
  pinMode(green_led_pin, OUTPUT);
  digitalWrite(red_led_pin, LOW);
  digitalWrite(green_led_pin, LOW);

  winch.invert = invert_motion;
  winch.init();

  
  m_pos_degrees = idle_deg;
  winch.writeDegrees(m_pos_degrees);
  
  last_time_ms = millis();
  last_time_current_ms = millis();

  delay(1);
}

void calibrate()
{
  m_pos_degrees = map(analogRead(pot_pin), 0, 1023, 600, 2400); // scales values
  Serial.print(m_pos_degrees, DEC);
  Serial.write("\n");
  winch.writeMicroseconds(m_pos_degrees);
}

int32_t counter = 0;

float filter_current()
{
  float avg = 0;
  for (size_t i = 0; i < current_buffer.count(); i++)
  {
    avg += *current_buffer.peek(i) * (1.0 / current_buffer.max_size());
  }
  return avg;
}

void update_current()
{
  constexpr float kVDC_V = 3.3;
  constexpr float kK_A_per_V = 2.5;
  constexpr float kAdcResolution = 1024;
  float current = kVDC_V * analogRead(current_pin) / kAdcResolution / kK_A_per_V;
  current_buffer.push(current);
}

void loop()
{
  if (mode == Modes::CALIBRATION)
  {
    calibrate();
  }
  else
  {
    uint32_t now = millis();
    // Once a millisecond
    if (abs(now - last_time_current_ms) >= kCurrentUpdatePeriod_ms)
    {
      update_current();
      last_time_current_ms = now;
    }

    if (abs(now - last_time_ms) >= kServoUpdatePeriod_ms)
    {
      // Print the currents for now.
      // TODO(cw): Use the currents as feedback? Set desired depth by torque/pressure?
      // Serial.println(filter_current(), DEC);

      // Read the potentiometer and determine the total length of breath
      float breath_length_ms = map(analogRead(pot_pin), 0, 1023, slowest_breath_length_ms + 100, fastest_breath_length_ms); // scales values

      if ((breath_state != BreathState::IDLE && breath_length_ms > slowest_breath_length_ms + 50) || breath_length_ms > slowest_breath_length_ms)
      {
        breath_state = BreathState::IDLE;
      }
      else if ((breath_state != BreathState::INHALATION) && m_pos_degrees <= open_deg)
      {
        Serial.write("Starting inhalation at: ");
        Serial.println(now, DEC);
        breath_state = BreathState::INHALATION;
      }
      else if ((breath_state != BreathState::EXHALATION) && (m_pos_degrees >= closed_deg))
      {
        Serial.write("Starting exhalation at: ");
        Serial.println(now, DEC);
        breath_state = BreathState::EXHALATION;
      }

      switch (breath_state)
      {
      case BreathState::IDLE:
        if (m_pos_degrees >= idle_deg)
        {
          m_pos_degrees += (idle_deg - closed_deg) * ((float)kServoUpdatePeriod_ms / (expiration_percent * breath_length_ms));
        }
        break;
      case BreathState::INHALATION:
        m_pos_degrees += (closed_deg - open_deg) * ((float)kServoUpdatePeriod_ms / (inspiration_percent * breath_length_ms));
        break;
      case BreathState::EXHALATION:
        m_pos_degrees += (open_deg - closed_deg) * ((float)kServoUpdatePeriod_ms / (expiration_percent * breath_length_ms));
        break;
      default:
        break;
      }

      winch.writeDegrees(m_pos_degrees);
      last_time_ms = now;
    }
  }
}