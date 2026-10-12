#include <Arduino.h>

#include "../../AutoDcController/DriveConfig.h"
#include "../../AutoDcController/MotorDriver.h"

constexpr uint32_t RUN_TIMEOUT_MS = 2000;
constexpr int DEFAULT_SPEED_PERCENT = 40;
constexpr int MIN_SPEED_PERCENT = 10;
constexpr int MAX_SPEED_PERCENT = 100;
constexpr int SPEED_STEP_PERCENT = 10;

MotorDriver leftMotor{PIN_LEFT_ENABLE, PIN_LEFT_FORWARD, PIN_LEFT_REVERSE, LEFT_MOTOR_INVERTED};
MotorDriver rightMotor{PIN_RIGHT_ENABLE, PIN_RIGHT_FORWARD, PIN_RIGHT_REVERSE, RIGHT_MOTOR_INVERTED};

int speedPercent = DEFAULT_SPEED_PERCENT;
bool running = false;
bool continuousMode = false;
uint32_t lastForwardCommandMs = 0;
uint32_t lastUpdateMs = 0;

void setup() {
  Serial.begin(SERIAL_BAUD_RATE);
  bool leftReady = leftMotor.begin();
  bool rightReady = rightMotor.begin();
  if (!leftReady || !rightReady) {
    Serial.println("Error: no se pudo asignar el PWM a ENA/ENB");
  }
  printHelp();
}

void loop() {
  uint32_t nowMs = millis();
  handleSerialInput(nowMs);
  stopIfTimedOut(nowMs);
  leftMotor.update(nowMs - lastUpdateMs);
  rightMotor.update(nowMs - lastUpdateMs);
  lastUpdateMs = nowMs;
}

void handleSerialInput(uint32_t nowMs) {
  while (Serial.available() > 0) {
    switch (tolower(Serial.read())) {
      case 'w':
        continuousMode = false;
        driveForward(nowMs);
        break;
      case 'c':
        continuousMode = true;
        driveForward(nowMs);
        Serial.println("Modo continuo: los motores siguen girando hasta enviar 's'");
        break;
      case 's':
        stopMotors();
        Serial.println("Motores detenidos");
        break;
      case '+':
        changeSpeed(SPEED_STEP_PERCENT);
        break;
      case '-':
        changeSpeed(-SPEED_STEP_PERCENT);
        break;
      case 'l':
        toggleInversion(leftMotor, "izquierdo", "LEFT_MOTOR_INVERTED");
        break;
      case 'r':
        toggleInversion(rightMotor, "derecho", "RIGHT_MOTOR_INVERTED");
        break;
      case '?':
        printHelp();
        break;
      default:
        break;
    }
  }
}

void driveForward(uint32_t nowMs) {
  running = true;
  lastForwardCommandMs = nowMs;
  applySpeed();
  Serial.printf("Adelante al %d %% (la potencia maxima es el %.1f %% del PWM)\n", speedPercent,
                MOTOR_MAX_DUTY_RATIO * 100.0f);
}

void stopIfTimedOut(uint32_t nowMs) {
  if (running && !continuousMode && nowMs - lastForwardCommandMs > RUN_TIMEOUT_MS) {
    stopMotors();
    Serial.printf("Sin nuevas 'w' en %lu ms: motores detenidos\n", (unsigned long)RUN_TIMEOUT_MS);
  }
}

void changeSpeed(int deltaPercent) {
  speedPercent = constrain(speedPercent + deltaPercent, MIN_SPEED_PERCENT, MAX_SPEED_PERCENT);
  Serial.printf("Potencia: %d %%\n", speedPercent);
  if (running) {
    applySpeed();
  }
}

void toggleInversion(MotorDriver& motor, const char* side, const char* constantName) {
  motor.setInverted(!motor.isInverted());
  Serial.printf("Motor %s invertido: %s -> en DriveConfig.h: %s = %s\n", side, motor.isInverted() ? "si" : "no",
                constantName, motor.isInverted() ? "true" : "false");
}

void applySpeed() {
  leftMotor.setTarget(speedPercent);
  rightMotor.setTarget(speedPercent);
}

void stopMotors() {
  running = false;
  continuousMode = false;
  leftMotor.stop();
  rightMotor.stop();
}

void printHelp() {
  Serial.println("Prueba de traccion trasera. Envia: w adelante | c continuo | s detener | + / - potencia | l / r invertir motor | ? ayuda");
  Serial.printf("Potencia inicial: %d %%\n", speedPercent);
}
