/* Servo Winch Control for BVM squeezer
 *  by James Lott and Mike Maluk
 *  25 MAR 2020
 */

#include <Servo.h>
#include <Arduino.h>
#include "winch.h"
#include "circular_buffer.h"

enum class Modes
{
  CALIBRATION,
  RATE,
};
const Modes mode = Modes::RATE;

const uint8_t servo_pin = 2;
const uint8_t pot_pin = PIN_A0;
const uint8_t current_pin = PIN_A1;
const uint8_t green_led_pin = 3;
const uint8_t red_led_pin = 4;

const uint32_t kCurrentUpdatePeriod_ms = 1;
const uint32_t kServoUpdatePeriod_ms = 20; // Match the 50 freq of the servo motors



// Configuration parameters 
// ! EDIT theses as needed
uint32_t slowest_breath_length_ms = 6250;
uint32_t fastest_breath_length_ms = 2400;
uint32_t slowest_closed_us = 950;
uint32_t fastest_closed_us = 1050;
float     expiration_percent = .66666;
float     inspiration_percent = .33333;

struct WinchSettings
{
  int32_t closed_us;
  float closed_slope;
  int32_t open_us;
  int32_t idle_us;
};
WinchSettings settings = {
    // 950 at the low end 1050 at the top end
    .closed_us = slowest_closed_us,
    .closed_slope = (fastest_closed_us - slowest_closed_us) / ((float)fastest_breath_length_ms- slowest_breath_length_ms), // Good at low end

    .open_us = 1850,
    .idle_us = 2000};

uint32_t last_time_ms = 0;
bool is_open = true;
int32_t pos = 0;
enum class BreathState
{
  IDLE,
  INHALATION,
  PLATEAU,
  EXHALATION,
};

BreathState breath_state = BreathState::IDLE;
Winch winch(servo_pin, 50);

uint32_t last_time_current_ms = 0;

CircularBuffer<float, 60, true> current_buffer;

void setup()
{
  Serial.begin(9600);
  pos = settings.idle_us;

  pinMode(red_led_pin, OUTPUT);
  pinMode(green_led_pin, OUTPUT);
  digitalWrite(red_led_pin, LOW);
  digitalWrite(green_led_pin, LOW);

  winch.init();
  winch.writeMicroseconds(pos);

  last_time_ms = millis();
  last_time_current_ms = millis();
  delay(1);
}

void calibrate()
{
  pos = map(analogRead(pot_pin), 0, 1023, 600, 2400); // scales values
  Serial.print(pos, DEC);
  Serial.write("\n");
  winch.writeMicroseconds(pos);
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

      float breath_length_ms = map(analogRead(pot_pin), 0, 1023, slowest_breath_length_ms + 100, fastest_breath_length_ms); // scales values

      uint32_t closed_ms = settings.closed_us + settings.closed_slope * (breath_length_ms - slowest_breath_length_ms);


      if ((breath_state != BreathState::IDLE  && breath_length_ms > slowest_breath_length_ms + 50) || breath_length_ms > slowest_breath_length_ms)
      {
        breath_state = BreathState::IDLE;
      }
      else if ((breath_state != BreathState::INHALATION) && pos >= settings.open_us)
      {
        Serial.write("Starting inhalation at: ");
        Serial.println(now, DEC);
        Serial.println(closed_ms, DEC);

        breath_state = BreathState::INHALATION;
      }
      else if ((breath_state != BreathState::EXHALATION) && (pos <= closed_ms))
      {
        Serial.write("Starting exhalation at: ");
        Serial.println(now, DEC);
        Serial.println(closed_ms, DEC);

        breath_state = BreathState::EXHALATION;
      }


      switch (breath_state)
      {
      case BreathState::IDLE:
        if (pos <= settings.idle_us) {
          pos += (settings.idle_us - closed_ms) * ((float)kServoUpdatePeriod_ms / (expiration_percent * breath_length_ms));
        }
      break;
      case BreathState::INHALATION:
        pos -= (settings.open_us - closed_ms) * ((float)kServoUpdatePeriod_ms / (inspiration_percent * breath_length_ms));
        break;
      case BreathState::EXHALATION:
        pos += (settings.open_us - closed_ms) * ((float)kServoUpdatePeriod_ms / (expiration_percent * breath_length_ms));
        break;
      default:
        break;
      }

      winch.writeMicroseconds(pos);
      last_time_ms = now;
    }
  }
}