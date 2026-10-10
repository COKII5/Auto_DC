#pragma once

#include <Arduino.h>

#include "DriveConfig.h"

class SteeringServo {
 public:
  explicit SteeringServo(uint8_t signalPin) : signalPin(signalPin) {}

  bool begin() {
    bool attached = ledcAttach(signalPin, SERVO_PWM_FREQUENCY_HZ, SERVO_PWM_RESOLUTION_BITS);
    center();
    return attached;
  }

  void setTrim(int8_t trimDeg) {
    trimDegrees = constrain((float)trimDeg, -SERVO_MAX_TRIM_DEG, SERVO_MAX_TRIM_DEG);
  }

  void setSteering(int8_t percent) {
    float direction = SERVO_INVERTED ? -1.0f : 1.0f;
    float steerDegrees = direction * percent * SERVO_MAX_STEER_DEG / 100.0f;
    writeAngle(SERVO_CENTER_DEG + trimDegrees + steerDegrees);
  }

  void center() {
    setSteering(0);
  }

 private:
  static constexpr uint32_t MAX_DUTY = (1u << SERVO_PWM_RESOLUTION_BITS) - 1;

  void writeAngle(float degrees) {
    float clampedDegrees = constrain(degrees, 0.0f, SERVO_RANGE_DEG);
    float pulseUs = SERVO_MIN_PULSE_US +
                    (SERVO_MAX_PULSE_US - SERVO_MIN_PULSE_US) * clampedDegrees / SERVO_RANGE_DEG;
    uint32_t duty = (uint32_t)(pulseUs * MAX_DUTY / SERVO_PERIOD_US);
    if (duty != lastDuty) {
      ledcWrite(signalPin, duty);
      lastDuty = duty;
    }
  }

  const uint8_t signalPin;
  float trimDegrees = 0.0f;
  uint32_t lastDuty = UINT32_MAX;
};
