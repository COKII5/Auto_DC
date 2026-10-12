#pragma once

#include <Arduino.h>

#include "CarProtocol.h"
#include "DriveConfig.h"

namespace echoCapture {
volatile uint32_t riseUs = 0;
volatile uint32_t widthUs = 0;
volatile bool captured = false;
volatile uint32_t edgeCount = 0;

void IRAM_ATTR onEchoChange() {
  uint32_t nowUs = micros();
  edgeCount = edgeCount + 1;
  if (digitalRead(PIN_DISTANCE_ECHO) == HIGH) {
    riseUs = nowUs;
  } else {
    widthUs = nowUs - riseUs;
    captured = true;
  }
}
}

class DistanceSensor {
 public:
  void begin() {
    pinMode(PIN_DISTANCE_TRIGGER, OUTPUT);
    digitalWrite(PIN_DISTANCE_TRIGGER, LOW);
    pinMode(PIN_DISTANCE_ECHO, INPUT);
    attachInterrupt(digitalPinToInterrupt(PIN_DISTANCE_ECHO), echoCapture::onEchoChange, CHANGE);
  }

  bool update(uint32_t nowUs) {
    uint32_t elapsedUs = nowUs - cycleStartUs;
    switch (state) {
      case State::IDLE:
        if (elapsedUs >= DISTANCE_MEASURE_PERIOD_US) {
          echoCapture::captured = false;
          digitalWrite(PIN_DISTANCE_TRIGGER, HIGH);
          cycleStartUs = nowUs;
          state = State::TRIGGERING;
        }
        return false;
      case State::TRIGGERING:
        if (elapsedUs >= DISTANCE_TRIGGER_PULSE_US) {
          digitalWrite(PIN_DISTANCE_TRIGGER, LOW);
          state = State::WAITING_ECHO;
        }
        return false;
      case State::WAITING_ECHO:
        if (echoCapture::captured) {
          latestEchoWidthUs = echoCapture::widthUs;
          latestDistanceMm = echoWidthToMillimeters(latestEchoWidthUs);
        } else if (elapsedUs >= DISTANCE_ECHO_TIMEOUT_US) {
          latestDistanceMm = DISTANCE_UNKNOWN_MM;
        } else {
          return false;
        }
        state = State::IDLE;
        return true;
    }
    return false;
  }

  uint16_t distanceMm() const { return latestDistanceMm; }

  uint32_t lastEchoWidthUs() const { return latestEchoWidthUs; }

  static uint32_t takeEchoEdgeCount() {
    uint32_t edges = echoCapture::edgeCount;
    echoCapture::edgeCount = 0;
    return edges;
  }

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
  uint32_t latestEchoWidthUs = 0;
  uint16_t latestDistanceMm = DISTANCE_UNKNOWN_MM;
};
