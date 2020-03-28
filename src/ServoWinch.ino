/* Servo Winch Control for BVM squeezer
 *  by James Lott and Mike Maluk
 *  25 MAR 2020
 */

#include <Servo.h> 
#include <Arduino.h>
#include "winch.h"
#include "circular_buffer.h"

enum class Modes {
  CALIBRATION,
  SERVO,
};

int timer = 150;
int pos;
int pot;
int fwd = 2000;
int rev = 1000;

const uint8_t servo_pin = 2;
const uint8_t pot_pin = PIN_A0;
const uint8_t current_pin = PIN_A1;

const uint8_t green_led_pin = 3;
const uint8_t red_led_pin = 4;
const Modes mode = Modes::SERVO;
struct WinchSettings {
  int32_t closed_us;
  int32_t open_us;
  int32_t idle_us;
};

WinchSettings settings = {
  .closed_us = 1166,
  .open_us = 1300 ,
  .idle_us = 1400

};

const int32_t num_rates = 8;

static int32_t rates_lookups[8][2]= {
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
int32_t dir = -1;

Winch winch(servo_pin, 50);

uint32_t last_time_current_ms = 0;

CircularBuffer<uint32_t, 400, true> fir_buffer;

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

void calibrate() {
  pos = map(analogRead(pot_pin), 0, 1023, 1000, 2000);      // scales values   
  Serial.print(pos, DEC);
  Serial.write("\n");
  winch.writeMicroseconds(pos);

}

int32_t counter = 0;
uint32_t average_current = 0;

void loop() {
  if (mode == Modes::CALIBRATION) {
    calibrate();
  } else {
      uint32_t now = millis();
    if (abs(now - last_time_current_ms) >= 1) {
      uint32_t current = 1000*3.3*analogRead(current_pin)/1024/2.5;
      fir_buffer.push(current);

      last_time_current_ms = now;
    }
    if (abs(now - last_time_ms) >= 20) {
      if (fir_buffer.count() == fir_buffer.max_size()) {
        float avg = 0;
        for (size_t i = 0; i < fir_buffer.count(); i++) {
          avg += *fir_buffer.peek(i) * (1.0/fir_buffer.max_size());
        }
        Serial.println(avg, DEC);
      } else {
                Serial.println(fir_buffer.count(), DEC);

      }

      uint32_t rate_idx = map(analogRead(pot_pin), 0, 1023, 0, num_rates);      // scales values   

      if (rate_idx == 0) { // If speed is 0 set to idle position
        winch.writeMicroseconds(settings.idle_us);
      } else { 
        
        if (++counter > rates_lookups[rate_idx-1][dir > 0 ? 1 : 0]) {
          dir *= -1;
          counter = 0;
        }

        pos = (dir > 0) ? settings.open_us : settings.closed_us;
        winch.writeMicroseconds(pos);
      }
        last_time_ms = now;

    }
  }
//  for(pos = rev; pos <= fwd; pos += pot){  // increments pos 
//   pot = analogRead(1);                  // reads pot position
//   pot = map(pot, 0, 1023, 1, 8);      // scales values   
//   Serial.print(pot, DEC);
//   Serial.write("\n");

//   Serial.print(pos, DEC);
//   Serial.write("\n");

//   winch.writeMicroseconds(pos);   // set servo to left endpoint
 
//  delay(timer);                         // wait for it to get there
//  }
//  for(pos = fwd; pos >= rev; pos -= pot){  // increments pos

//   pot = analogRead(1);                  // reads pot position
//   pot = map(pot, 0, 1023, 1, 8);      // scales values   

//  winch.writeMicroseconds(pos);   // set servo to right endpoint

//  delay(timer);                         // wait for it to get there
//  }
} 
