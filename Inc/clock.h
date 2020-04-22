#ifndef ABVM_CLOCK_H_
#define ABVM_CLOCK_H_
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Get the current system time in milliseconds
 * 
 * IMPORTANT: This function assumes that millis() will count up to the full extent of a uint32_t and
 *            then wrap around from 0.
 */
uint32_t millis();

/**
 * Get the current system time in microseconds
 * 
 * IMPORTANT: This function assumes that micros() will count up to the full extent of a uint64_t and
 *            then wrap around from 0.
 * 
 * TODO(cw): Verify that micros() is reentrant
 */
uint64_t micros();

inline uint32_t time_since_ms(uint32_t start_time) { return millis() - start_time; }
inline uint64_t time_since_us(uint64_t start_time) { return micros() - start_time; }

#ifdef __cplusplus
}
#endif

#endif