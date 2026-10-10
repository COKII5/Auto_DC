#pragma once

#include <Arduino.h>

#include "CarProtocol.h"
#include "DriveConfig.h"

namespace distance_sensor_internal {
volatile uint32_t echoRiseUs = 0;
volatile uint32_t echoWidthUs = 0;
volatile bool echoCaptured = false;

void IRAM_ATTR onEchoChange() {
  uint32_t nowUs = micros();
  if (digitalRead(PIN_DISTANCE_ECHO) == HIGH) {
    echoRiseUs = nowUs;
  } else {
    echoWidthUs = nowUs - echoRiseUs;
    echoCaptured = true;
  }
}
}

class DistanceSensor {
 public:
  void begin() {
    pinMode(PIN_DISTANCE_TRIGGER, OUTPUT);
    digitalWrite(PIN_DISTANCE_TRIGGER, LOW);
    pinMode(PIN_DISTANCE_ECHO, INPUT);
    attachInterrupt(digitalPinToInterrupt(PIN_DISTANCE_ECHO), distance_sensor_internal::onEchoChange, CHANGE);
  }

  void update(uint32_t nowUs) {
    uint32_t elapsedUs = nowUs - cycleStartUs;
    switch (state) {
      case State::IDLE:
        if (elapsedUs >= DISTANCE_MEASURE_PERIOD_US) {
          distance_sensor_internal::echoCaptured = false;
          digitalWrite(PIN_DISTANCE_TRIGGER, HIGH);
          cycleStartUs = nowUs;
          state = State::TRIGGERING;
        }
        break;
      case State::TRIGGERING:
        if (elapsedUs >= DISTANCE_TRIGGER_PULSE_US) {
          digitalWrite(PIN_DISTANCE_TRIGGER, LOW);
          state = State::WAITING_ECHO;
        }
        break;
      case State::WAITING_ECHO:
        if (distance_sensor_internal::echoCaptured) {
          latestDistanceMm = echoWidthToMillimeters(distance_sensor_internal::echoWidthUs);
          state = State::IDLE;
        } else if (elapsedUs >= DISTANCE_ECHO_TIMEOUT_US) {
          latestDistanceMm = DISTANCE_UNKNOWN_MM;
          state = State::IDLE;
        }
        break;
    }
  }

  uint16_t distanceMm() const { return latestDistanceMm; }

 private:
  enum class State : uint8_t { IDLE, TRIGGERING, WAITING_ECHO };

  static uint16_t echoWidthToMillimeters(uint32_t widthUs) {
    float millimeters = widthUs * SPEED_OF_SOUND_MM_PER_US / 2.0f;
    if (millimeters < DISTANCE_MIN_MM || millimeters > DISTANCE_MAX_MM) {
      return DISTANCE_UNKNOWN_MM;
    }
    return (uint16_t)millimeters;
  }

  State state = State::IDLE;
  uint32_t cycleStartUs = 0;
  uint16_t latestDistanceMm = DISTANCE_UNKNOWN_MM;
};
