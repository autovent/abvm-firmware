#ifndef CONFIG_H_
#define CONFIG_H_
#include "motor.h"
#include "pid.h"

enum class Modes {
    CALIBRATION,
    VENTILATOR,
};

struct IERatio {
    float inspiration;
    float expiration;

    inline const float getInspirationPercent() const
    {
        return inspiration / (inspiration + expiration);
    }
    inline const float getExpirationPercent() const
    {
        return expiration / (inspiration + expiration);
    }
};

constexpr MotorParameters kGoBilda_YellowJacket_5202_0002_0188_30RPM = {188, 28};

constexpr MotorParameters kRobotZone_638312_16RPM = {515.63, 48};

#if defined(MOTOR_GOBILDA_30RPM)
constexpr MotorParameters kMotorParams = kGoBilda_YellowJacket_5202_0002_0188_30RPM;
constexpr PID::Params kMotorSpeedPidParams = {3, .2, 0};
constexpr PID::Params kMotorPosPidParams = {16, .4, 0};
#elif defined(MOTOR_ROBOTZONE_16RPM)
constexpr MotorParameters kMotorParams = kRobotZone_638312_16RPM;
constexpr PID::Params kMotorSpeedPidParams = {3.8, .2, 0};
constexpr PID::Params kMotorPosPidParams = {12, .4, .0};
#endif



constexpr Modes mode = Modes::VENTILATOR;

constexpr uint32_t kCurrentUpdatePeriod_ms = 2;
constexpr uint32_t kPositionUpdate_ms = 20;          // Match the 50 freq of the servo motors
constexpr uint32_t kMeasurementUpdatePeriod_ms = 10; // Match the 50 freq of the servo motors

// Configuration parameters
// ! EDIT theses as needed
constexpr int32_t kSlowestBreathTime_ms = 6250;
constexpr int32_t kFastestBreathTime_ms = 2400;
constexpr float kIdlePositiong_deg = 1;
constexpr float kOpenPosition_deg = 35; // Change this to a value where the servo is just barely compressing the bag
constexpr float kMinClosedPosition_deg
    = 40; // Change this to a value where the servo has displaced the appropriate amount.
constexpr float kMaxClosedPosition_deg
    = 89; // Change this to a value where the servo has displaced the appropriate amount.

// I : E Inspiration to Expiration Ratio
constexpr IERatio kIERatio = {1, 2};
constexpr bool kInvertMotion = true; // Change this is the motor is inverted. This will reflect it 180

#endif