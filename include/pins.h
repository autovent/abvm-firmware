#ifndef PINS_H
#define PINS_H

#include "Arduino.h"

const uint8_t servo_pin = 2;
const uint8_t rate_in_pin = PIN_A0;
const uint8_t current_pin = PIN_A1;
const uint8_t green_led_pin = 3;
const uint8_t red_led_pin = 4;
const uint8_t depth_in_pin = PIN_A2;

const uint8_t motor_pwm_pin = 2;
const uint8_t motor_dir_pin = 1;
const uint8_t motor_enca_pin = 6;
const uint8_t motor_encb_pin = 7;

const uint8_t pressure_sense_pwdn_pin = 9;
const uint8_t load_cell_sense_pwdn_pin = 8;

const uint8_t homing_switch_pin = 0;

const uint8_t pos_out_pwm_pin = 23;
const uint8_t vel_out_pwm_pin = 22;
const uint8_t breathstate_out_pin = 21;

const uint8_t vdc_in_pin= 17;

#endif // PINS_H
