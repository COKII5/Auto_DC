#pragma once

#include <Arduino.h>

#include "DriveConfig.h"

class MotorDriver {
 public:
  MotorDriver(uint8_t enablePin, uint8_t forwardPin, uint8_t reversePin, bool inverted)
      : enablePin(enablePin), forwardPin(forwardPin), reversePin(reversePin), inverted(inverted) {}

  bool begin() {
    pinMode(forwardPin, OUTPUT);
    pinMode(reversePin, OUTPUT);
    bool attached = ledcAttach(enablePin, MOTOR_PWM_FREQUENCY_HZ, MOTOR_PWM_RESOLUTION_BITS);
    stop();
    return attached;
  }

  void setTarget(float percent) {
    targetPercent = constrain(percent, -100.0f, 100.0f);
    braking = false;
  }

  void setInverted(bool value) {
    inverted = value;
  }

  bool isInverted() const {
    return inverted;
  }

  void stop() {
    targetPercent = 0.0f;
    currentPercent = 0.0f;
    braking = false;
    writeOutput();
  }

  void brake() {
    targetPercent = 0.0f;
    currentPercent = 0.0f;
    braking = true;
    digitalWrite(forwardPin, HIGH);
    digitalWrite(reversePin, HIGH);
    ledcWrite(enablePin, MAX_DUTY);
  }

  void update(uint32_t elapsedMs) {
    if (braking) {
      return;
    }
    float maxStep = MOTOR_RAMP_PERCENT_PER_SECOND * elapsedMs / 1000.0f;
    float difference = targetPercent - currentPercent;
    currentPercent += constrain(difference, -maxStep, maxStep);
    writeOutput();
  }

 private:
  static constexpr uint32_t MAX_DUTY =
      (uint32_t)(((1u << MOTOR_PWM_RESOLUTION_BITS) - 1) * MOTOR_MAX_DUTY_RATIO);

  void writeOutput() {
    float signedPercent = inverted ? -currentPercent : currentPercent;
    digitalWrite(forwardPin, signedPercent > 0.0f ? HIGH : LOW);
    digitalWrite(reversePin, signedPercent < 0.0f ? HIGH : LOW);
    ledcWrite(enablePin, (uint32_t)(fabsf(signedPercent) * MAX_DUTY / 100.0f));
  }

  const uint8_t enablePin;
  const uint8_t forwardPin;
  const uint8_t reversePin;
  bool inverted;
  float targetPercent = 0.0f;
  float currentPercent = 0.0f;
  bool braking = false;
};
