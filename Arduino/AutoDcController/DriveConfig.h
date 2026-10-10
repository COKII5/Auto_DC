#pragma once

#include <stdint.h>

enum class SteeringMode : uint8_t {
  SERVO = 0,
  DIFFERENTIAL = 1,
};

constexpr const char* WIFI_SSID = "AutoDC";
constexpr const char* WIFI_PASSWORD = "autodc1234";
constexpr uint16_t UDP_PORT = 4210;
constexpr uint32_t SERIAL_BAUD_RATE = 115200;

constexpr SteeringMode STEERING_MODE = SteeringMode::SERVO;

constexpr uint8_t PIN_LEFT_ENABLE = 25;
constexpr uint8_t PIN_LEFT_FORWARD = 26;
constexpr uint8_t PIN_LEFT_REVERSE = 27;
constexpr uint8_t PIN_RIGHT_ENABLE = 14;
constexpr uint8_t PIN_RIGHT_FORWARD = 33;
constexpr uint8_t PIN_RIGHT_REVERSE = 32;
constexpr uint8_t PIN_SERVO = 13;

constexpr uint8_t PIN_DISTANCE_TRIGGER = 18;
constexpr uint8_t PIN_DISTANCE_ECHO = 34;

constexpr bool LEFT_MOTOR_INVERTED = false;
constexpr bool RIGHT_MOTOR_INVERTED = false;

constexpr uint32_t MOTOR_PWM_FREQUENCY_HZ = 1000;
constexpr uint8_t MOTOR_PWM_RESOLUTION_BITS = 8;
constexpr float MOTOR_MAX_DUTY_RATIO = 0.675f;
constexpr float MOTOR_RAMP_PERCENT_PER_SECOND = 400.0f;

constexpr uint32_t SERVO_PWM_FREQUENCY_HZ = 50;
constexpr uint8_t SERVO_PWM_RESOLUTION_BITS = 14;
constexpr float SERVO_PERIOD_US = 1000000.0f / SERVO_PWM_FREQUENCY_HZ;
constexpr float SERVO_MIN_PULSE_US = 500.0f;
constexpr float SERVO_MAX_PULSE_US = 2500.0f;
constexpr float SERVO_RANGE_DEG = 180.0f;
constexpr float SERVO_CENTER_DEG = 90.0f;
constexpr uint8_t SERVO_MAX_STEER_DEG = 30;
constexpr float SERVO_MAX_TRIM_DEG = 15.0f;
constexpr bool SERVO_INVERTED = false;

constexpr uint32_t DISTANCE_MEASURE_PERIOD_US = 60000;
constexpr uint32_t DISTANCE_TRIGGER_PULSE_US = 10;
constexpr uint32_t DISTANCE_ECHO_TIMEOUT_US = 30000;
constexpr uint16_t DISTANCE_MIN_MM = 20;
constexpr uint16_t DISTANCE_MAX_MM = 4000;
constexpr float SPEED_OF_SOUND_MM_PER_US = 0.343f;

constexpr uint32_t FAILSAFE_TIMEOUT_MS = 300;
constexpr uint32_t TELEMETRY_PERIOD_MS = 100;
