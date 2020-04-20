#ifndef CONFIG_H_
#define CONFIG_H_
#include "controls/pid.h"
#include "math/conversions.h"
#include "servo.h"

#define MOTOR_GOBILDA_30RPM
#define CONFIG_LONG_SPIRIT_FINGERS

enum class Modes {
    CALIBRATION,
    VENTILATOR,
};

struct IERatio {
    float inspiration;
    float expiration;

    inline const float inspiration_percent() const {
        return inspiration / (inspiration + expiration);
    }
    inline const float expiration_percent() const {
        return expiration / (inspiration + expiration);
    }
};

constexpr Servo::Config kGoBilda_YellowJacket_5202_0002_0188_30RPM = {188, 28};

constexpr Servo::Config kRobotZone_638312_16RPM = {515.63, 48};

#if defined(MOTOR_GOBILDA_30RPM)
constexpr Servo::Config kMotorParams = kGoBilda_YellowJacket_5202_0002_0188_30RPM;
constexpr PID::Params kMotorVelPidParams = {3, .0, .0};
constexpr PID::Params kMotorPosPidParams = {15, .1, .0};                         //.02};
constexpr Range<float> kMotorVelLimits = {-30 * 0.104719755, 30 * 0.104719755};  // RPM to rads/sec
constexpr Range<float> kMotorPosLimits = {0, deg_to_rad(95)};

#elif defined(MOTOR_ROBOTZONE_16RPM)
constexpr Servo::Config kMotorParams = kRobotZone_638312_16RPM;
constexpr PID::Params kMotorVelPidParams = {6.5, .05, 0};
constexpr PID::Params kMotorPosPidParams = {16, 0, 0.0};
constexpr Range kMotorVelLimits = {-15 * 0.104719755, 15 * 0.104719755};  // RPM to rads/sec
constexpr Range kMotorPosLimits = {0, deg_to_rad(89)};
#endif

constexpr Modes mode = Modes::VENTILATOR;

constexpr uint32_t kCurrentUpdatePeriod_ms = 2;
constexpr uint32_t kPositionUpdate_ms = 10;  // Match the 50 freq of the servo motors
constexpr uint32_t kMeasurementUpdatePeriod_ms = 100;

// Configuration parameters
// ! EDIT theses as needed
constexpr uint32_t kTimeToIdle_ms = 500;
constexpr int32_t kMinBPM = 8;
constexpr int32_t kMaxBPM = 18;
constexpr int32_t kSlowestBreathTime_ms = 1000 * 60 / kMinBPM;
constexpr int32_t kFastestBreathTime_ms = 1000 * 60 / kMaxBPM;
constexpr int32_t kPlateauTime_ms = 200;
constexpr int32_t kFastOpenTime_ms = 100;
constexpr float kDefaultPeakPressureLimit = 40;
constexpr float kPeakPressureDisplayMin = 25;
constexpr float kPeakPressureDisplayMax = 50;
constexpr float kPlateauPressureDisplayMin = 15;
constexpr float kPlateauPressureDisplayMax = 40;
constexpr float kPeakPressureLimitIncrement = 5;

constexpr float kIdlePositiong_deg = 10;
constexpr float kOpenPosition_deg = 25;       // Change this to a value where the servo is just barely compressing
                                              // the bag
constexpr float kMinClosedPosition_deg = 40;  // Change this to a value where the servo has displaced the appropriate
                                              // amount.
constexpr float kMaxClosedPosition_deg = 90;  // Change this to a value where the servo has displaced the appropriate
                                              // amount.
// I : E Inspiration to Expiration Ratio
constexpr IERatio kIERatio = {1, 2};
constexpr bool kInvertMotion = true;  // Change this is the motor is inverted. This will reflect it 180

#endif