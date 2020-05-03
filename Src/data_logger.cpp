#include "data_logger.h"
#include "clock.h"
#include "math/conversions.h"

DataLogger::DataLogger(uint8_t id, ADS1231 *pressure_sensor, Servo *motor,
    DRV8873 *motor_driver, VentilatorController *vent) :
        CommEndpoint(id, &log, sizeof(dataLog), true),
        pressure_sensor(pressure_sensor),
        motor(motor),
        motor_driver(motor_driver),
        vent(vent) {}

uint8_t DataLogger::read(void *data, size_t size) {
    log.time = msec_to_sec(millis());
    log.pressure = psi_to_cmH2O(pressure_sensor->read());
    log.motor_velocity = motor->velocity;
    log.motor_target_vel = motor->target_velocity;
    log.motor_pos = motor->position;
    log.motor_target_pos = motor->target_pos;
    log.motor_current = motor_driver->get_current();
    log.vent_rate = vent->get_rate();
    log.vent_closed_pos = vent->get_closed_pos();
    log.vent_open_pos = vent->get_open_pos();

    return CommEndpoint::read(data, size);
}