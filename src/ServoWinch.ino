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

struct WinchSettings
{
  int32_t closed_us;
  int32_t open_us;
  int32_t idle_us;
};

WinchSettings settings = {
    .closed_us = 1000,
    .open_us = 1872,
    .idle_us = 2000
};


const int32_t num_rates = 8;

static int32_t pos_hold_table_ms[8][2] = {
    {400, 400},
    {360, 360},
    {330, 330},
    {300, 300},
    {260, 260},
    {230, 230},
    {200, 200},
    {180, 190},

};

uint32_t last_time_ms = 0;
bool is_open = true;
uint32_t pos = 0;
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
      Serial.println(filter_current(), DEC);

      uint32_t rate_idx = map(analogRead(pot_pin), 0, 1023, 0, num_rates); // scales values

      if (rate_idx == 0)
      { // If speed is 0 set to idle position
        winch.writeMicroseconds(settings.idle_us);
      }
      else
      {
        uint32_t counter_max = pos_hold_table_ms[rate_idx - 1][is_open ? 1 : 0];
        // Lookup the amount of time we want to stay in a specific position
        if (++counter > counter_max)
        {
          is_open = !is_open;
          counter = 0;
        }


        winch.writeMicroseconds(is_open ? settings.open_us : settings.closed_us);
      }
      last_time_ms = now;
    }
  }
}
