#pragma once

#include <stddef.h>
#include <stdint.h>

constexpr uint8_t PACKET_MAGIC = 0xAD;
constexpr uint8_t PACKET_TYPE_DRIVE = 0x01;
constexpr uint8_t PACKET_TYPE_TELEMETRY = 0x81;
constexpr size_t DRIVE_PACKET_SIZE = 9;
constexpr size_t TELEMETRY_PACKET_SIZE = 17;
constexpr uint16_t DISTANCE_UNKNOWN_MM = 0xFFFF;
constexpr uint8_t DRIVE_FLAG_BRAKE = 0x01;
constexpr uint8_t TELEMETRY_FLAG_FAILSAFE = 0x01;
constexpr int8_t COMMAND_LIMIT = 100;
constexpr uint8_t CRC8_POLYNOMIAL = 0x07;
constexpr uint8_t CRC8_HIGH_BIT = 0x80;
constexpr uint8_t BITS_PER_BYTE = 8;

struct DriveCommand {
  uint16_t sequence;
  int8_t throttle;
  int8_t steering;
  int8_t steeringTrimDeg;
  bool brake;
};

struct CarTelemetry {
  uint16_t ackSequence;
  int8_t appliedThrottle;
  int8_t appliedSteering;
  uint8_t steeringMode;
  uint8_t maxSteerDeg;
  uint8_t ackAgeMs;
  uint32_t uptimeMs;
  uint16_t distanceMm;
  bool failsafe;
};

inline uint8_t crc8(const uint8_t* bytes, size_t length) {
  uint8_t crc = 0x00;
  for (size_t index = 0; index < length; index++) {
    crc ^= bytes[index];
    for (uint8_t bit = 0; bit < BITS_PER_BYTE; bit++) {
      crc = (crc & CRC8_HIGH_BIT) ? (uint8_t)((crc << 1) ^ CRC8_POLYNOMIAL) : (uint8_t)(crc << 1);
    }
  }
  return crc;
}

inline bool isWithinCommandLimit(int8_t value) {
  return value >= -COMMAND_LIMIT && value <= COMMAND_LIMIT;
}

inline bool parseDriveCommand(const uint8_t* bytes, size_t length, DriveCommand& command) {
  if (length != DRIVE_PACKET_SIZE || bytes[0] != PACKET_MAGIC || bytes[1] != PACKET_TYPE_DRIVE) {
    return false;
  }
  if (crc8(bytes, length - 1) != bytes[length - 1]) {
    return false;
  }
  command.sequence = (uint16_t)(bytes[2] | (bytes[3] << 8));
  command.throttle = (int8_t)bytes[4];
  command.steering = (int8_t)bytes[5];
  command.steeringTrimDeg = (int8_t)bytes[6];
  command.brake = (bytes[7] & DRIVE_FLAG_BRAKE) != 0;
  return isWithinCommandLimit(command.throttle) && isWithinCommandLimit(command.steering);
}

inline bool isNewerSequence(uint16_t candidate, uint16_t reference) {
  return (int16_t)(candidate - reference) > 0;
}

inline void encodeTelemetry(const CarTelemetry& telemetry, uint8_t* bytes) {
  bytes[0] = PACKET_MAGIC;
  bytes[1] = PACKET_TYPE_TELEMETRY;
  bytes[2] = (uint8_t)(telemetry.ackSequence & 0xFF);
  bytes[3] = (uint8_t)(telemetry.ackSequence >> 8);
  bytes[4] = (uint8_t)telemetry.appliedThrottle;
  bytes[5] = (uint8_t)telemetry.appliedSteering;
  bytes[6] = telemetry.steeringMode;
  bytes[7] = telemetry.maxSteerDeg;
  bytes[8] = telemetry.ackAgeMs;
  for (uint8_t index = 0; index < 4; index++) {
    bytes[9 + index] = (uint8_t)(telemetry.uptimeMs >> (8 * index));
  }
  bytes[13] = (uint8_t)(telemetry.distanceMm & 0xFF);
  bytes[14] = (uint8_t)(telemetry.distanceMm >> 8);
  bytes[15] = telemetry.failsafe ? TELEMETRY_FLAG_FAILSAFE : 0x00;
  bytes[16] = crc8(bytes, TELEMETRY_PACKET_SIZE - 1);
}
