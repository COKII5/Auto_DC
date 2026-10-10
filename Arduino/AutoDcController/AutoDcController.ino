#include <WiFi.h>
#include <WiFiUdp.h>

#include "CarProtocol.h"
#include "DistanceSensor.h"
#include "DriveConfig.h"
#include "DriveTrain.h"

WiFiUDP udp;
DriveTrain driveTrain;
DistanceSensor distanceSensor;
DriveCommand lastCommand = {};
IPAddress clientAddress;
uint16_t clientPort = 0;
uint32_t lastCommandMs = 0;
uint32_t lastTelemetryMs = 0;
uint32_t lastUpdateMs = 0;
bool failsafeActive = true;

void setup() {
  Serial.begin(SERIAL_BAUD_RATE);
  if (!driveTrain.begin()) {
    Serial.println("Error: no se pudo asignar el PWM a los pines de DriveConfig.h");
  }
  distanceSensor.begin();
  WiFi.mode(WIFI_AP);
  if (!WiFi.softAP(WIFI_SSID, WIFI_PASSWORD)) {
    Serial.println("Error: no se pudo crear la red Wi-Fi del carro");
    return;
  }
  udp.begin(UDP_PORT);
  Serial.printf("Red %s lista. Unity debe enviar a %s:%u\n", WIFI_SSID,
                WiFi.softAPIP().toString().c_str(), UDP_PORT);
}

void loop() {
  uint32_t nowMs = millis();
  receiveCommands(nowMs);
  updateFailsafe(nowMs);
  distanceSensor.update(micros());
  driveTrain.update(nowMs - lastUpdateMs);
  lastUpdateMs = nowMs;
  sendTelemetryIfDue(nowMs);
}

void receiveCommands(uint32_t nowMs) {
  while (udp.parsePacket() > 0) {
    uint8_t buffer[DRIVE_PACKET_SIZE + 1];
    int length = udp.read(buffer, sizeof(buffer));
    DriveCommand command;
    if (length <= 0 || !parseDriveCommand(buffer, (size_t)length, command)) {
      continue;
    }
    if (!failsafeActive && !isNewerSequence(command.sequence, lastCommand.sequence)) {
      continue;
    }
    lastCommand = command;
    lastCommandMs = nowMs;
    failsafeActive = false;
    clientAddress = udp.remoteIP();
    clientPort = udp.remotePort();
    driveTrain.apply(command);
  }
}

void updateFailsafe(uint32_t nowMs) {
  if (!failsafeActive && nowMs - lastCommandMs > FAILSAFE_TIMEOUT_MS) {
    failsafeActive = true;
    driveTrain.stopAll();
  }
}

void sendTelemetryIfDue(uint32_t nowMs) {
  if (clientPort == 0 || nowMs - lastTelemetryMs < TELEMETRY_PERIOD_MS) {
    return;
  }
  lastTelemetryMs = nowMs;
  CarTelemetry telemetry = {};
  telemetry.ackSequence = lastCommand.sequence;
  telemetry.appliedThrottle = failsafeActive || lastCommand.brake ? 0 : lastCommand.throttle;
  telemetry.appliedSteering = failsafeActive ? 0 : lastCommand.steering;
  telemetry.steeringMode = (uint8_t)STEERING_MODE;
  telemetry.maxSteerDeg = SERVO_MAX_STEER_DEG;
  telemetry.ackAgeMs = (uint8_t)min(nowMs - lastCommandMs, (uint32_t)UINT8_MAX);
  telemetry.uptimeMs = nowMs;
  telemetry.distanceMm = distanceSensor.distanceMm();
  telemetry.failsafe = failsafeActive;
  uint8_t bytes[TELEMETRY_PACKET_SIZE];
  encodeTelemetry(telemetry, bytes);
  udp.beginPacket(clientAddress, clientPort);
  udp.write(bytes, sizeof(bytes));
  udp.endPacket();
}
