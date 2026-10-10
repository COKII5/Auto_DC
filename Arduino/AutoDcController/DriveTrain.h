#pragma once

#include <Arduino.h>

#include "CarProtocol.h"
#include "DriveConfig.h"
#include "MotorDriver.h"
#include "SteeringServo.h"

class DriveTrain {
 public:
  bool begin() {
    bool leftReady = leftMotor.begin();
    bool rightReady = rightMotor.begin();
    bool servoReady = STEERING_MODE != SteeringMode::SERVO || steeringServo.begin();
    return leftReady && rightReady && servoReady;
  }

  void apply(const DriveCommand& command) {
    if (STEERING_MODE == SteeringMode::SERVO) {
      steeringServo.setTrim(command.steeringTrimDeg);
      steeringServo.setSteering(command.steering);
    }
    if (command.brake) {
      leftMotor.brake();
      rightMotor.brake();
      return;
    }
    float leftPercent = command.throttle;
    float rightPercent = command.throttle;
    if (STEERING_MODE == SteeringMode::DIFFERENTIAL) {
      mixDifferential(command.throttle, command.steering, leftPercent, rightPercent);
    }
    leftMotor.setTarget(leftPercent);
    rightMotor.setTarget(rightPercent);
  }

  void stopAll() {
    leftMotor.stop();
    rightMotor.stop();
    if (STEERING_MODE == SteeringMode::SERVO) {
      steeringServo.center();
    }
  }

  void update(uint32_t elapsedMs) {
    leftMotor.update(elapsedMs);
    rightMotor.update(elapsedMs);
  }

 private:
  static void mixDifferential(float throttle, float steering, float& leftPercent, float& rightPercent) {
    leftPercent = throttle + steering;
    rightPercent = throttle - steering;
    float largest = max(fabsf(leftPercent), fabsf(rightPercent));
    if (largest > 100.0f) {
      leftPercent *= 100.0f / largest;
      rightPercent *= 100.0f / largest;
    }
  }

  MotorDriver leftMotor{PIN_LEFT_ENABLE, PIN_LEFT_FORWARD, PIN_LEFT_REVERSE, LEFT_MOTOR_INVERTED};
  MotorDriver rightMotor{PIN_RIGHT_ENABLE, PIN_RIGHT_FORWARD, PIN_RIGHT_REVERSE, RIGHT_MOTOR_INVERTED};
  SteeringServo steeringServo{PIN_SERVO};
};
