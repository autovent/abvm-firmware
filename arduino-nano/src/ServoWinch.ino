/* Servo Winch Control for BVM squeezer
 *  by James Lott and Mike Maluk
 *  25 MAR 2020
 */

#include <Servo.h> 
#include <Arduino.h>
#include "winch.h"
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
  {500,300},
  {350, 350},
  {300, 300},
  {250, 250},
  {200, 200},
  {150, 150},
  {100, 100},
  {50, 50},
};

uint32_t control_rate_ms = 1e3/100.0; // 100Hz control rate
uint32_t last_time_ms = 0;
int32_t dir = -1;

Winch winch(servo_pin);
void setup() 
{ 
  Serial.begin(9600);
  // winch.attach(servo_pin, 18000, 19000); 
  pos = settings.idle_us;

  winch.init();
  winch.writeMicroseconds(pos);
  delay(1);
} 

void calibrate() {
  pos = map(analogRead(pot_pin), 0, 1023, 1000, 2000);      // scales values   
  Serial.print(pos, DEC);
  Serial.write("\n");
  winch.writeMicroseconds(pos);
  last_time_ms = millis();

}

int32_t counter = 0;
void loop() {
  if (mode == Modes::CALIBRATION) {
    calibrate();
  } else {
      uint32_t now = millis();

    if (abs(now - last_time_ms) >= 20) {
      uint32_t rate_idx = map(analogRead(pot_pin), 0, 1023, 0, num_rates);      // scales values   
      Serial.println(pos, DEC);
      Serial.println(rate_idx, DEC);

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
