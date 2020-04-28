#ifndef DATA_LOGGER_H
#define DATA_LOGGER_H

#include "ads1231.h"
#include "serial_comm.h"
#include "servo.h"
#include "ventilator_controller.h"

class DataLogger : public CommEndpoint {
public:
    DataLogger(uint8_t id, ADS1231 *pressure_sensor, Servo *motor, DRV8873 *motor_driver, VentilatorController *vent);

    uint8_t read(void *data, size_t size) override;

private:
    ADS1231 *pressure_sensor;
    Servo *motor;
    DRV8873 *motor_driver;
    VentilatorController *vent;

    struct __attribute__((__packed__)) dataLog {
        float time;
        float pressure;
        float motor_velocity;
        float motor_target_vel;
        float motor_pos;
        float motor_target_pos;
        float motor_current;
        float vent_rate;
        float vent_closed_pos;
        float vent_open_pos;
        uint32_t motor_faults;
    };

    dataLog log;
};

#endif
