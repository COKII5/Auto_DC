#include <Arduino.h>

#include "../../AutoDcController/DriveConfig.h"
#include "../../AutoDcController/SteeringServo.h"

constexpr int STEER_STEP_DEG = 5;
constexpr int MIN_MAX_STEER_DEG = 5;
constexpr int LIMIT_MAX_STEER_DEG = 60;
constexpr int MAX_TRIM_DEG = (int)SERVO_MAX_TRIM_DEG;
constexpr uint32_t SWEEP_STEP_PERIOD_MS = 20;

SteeringServo steeringServo{PIN_SERVO};

int steerDeg = 0;
int trimDeg = 0;
int maxSteerDeg = SERVO_MAX_STEER_DEG;
bool sweeping = false;
int sweepDirection = 1;
uint32_t lastSweepStepMs = 0;

void setup() {
  Serial.begin(SERIAL_BAUD_RATE);
  if (!steeringServo.begin()) {
    Serial.println("Error: no se pudo asignar el PWM al pin del servo");
  }
  printHelp();
}

void loop() {
  handleSerialInput();
  advanceSweep(millis());
}

void handleSerialInput() {
  while (Serial.available() > 0) {
    char key = (char)tolower(Serial.read());
    if (key == '\r' || key == '\n' || key == ' ') {
      continue;
    }
    if (key != 'b') {
      sweeping = false;
    }
    switch (key) {
      case 'a':
        moveSteer(-STEER_STEP_DEG);
        break;
      case 'd':
        moveSteer(STEER_STEP_DEG);
        break;
      case 'c':
        moveSteer(-steerDeg);
        break;
      case 'b':
        sweeping = true;
        Serial.println("Barrido automatico: envia cualquier otra tecla para detenerlo");
        break;
      case '[':
        changeTrim(-1);
        break;
      case ']':
        changeTrim(1);
        break;
      case '+':
        changeMaxSteer(STEER_STEP_DEG);
        break;
      case '-':
        changeMaxSteer(-STEER_STEP_DEG);
        break;
      case 'i':
        toggleInversion();
        break;
      case '?':
        printHelp();
        break;
      default:
        break;
    }
  }
}

void moveSteer(int deltaDeg) {
  steerDeg = constrain(steerDeg + deltaDeg, -maxSteerDeg, maxSteerDeg);
  steeringServo.setSteeringDegrees(steerDeg);
  reportState();
}

void changeTrim(int deltaDeg) {
  trimDeg = constrain(trimDeg + deltaDeg, -MAX_TRIM_DEG, MAX_TRIM_DEG);
  steeringServo.setTrim(trimDeg);
  steeringServo.setSteeringDegrees(steerDeg);
  Serial.printf("Centro: %.0f grados (trim %+d) -> en DriveConfig.h: SERVO_CENTER_DEG = %.1f\n",
                SERVO_CENTER_DEG + trimDeg, trimDeg, SERVO_CENTER_DEG + trimDeg);
}

void changeMaxSteer(int deltaDeg) {
  maxSteerDeg = constrain(maxSteerDeg + deltaDeg, MIN_MAX_STEER_DEG, LIMIT_MAX_STEER_DEG);
  steerDeg = constrain(steerDeg, -maxSteerDeg, maxSteerDeg);
  steeringServo.setSteeringDegrees(steerDeg);
  Serial.printf("Angulo maximo: %d grados -> en DriveConfig.h: SERVO_MAX_STEER_DEG = %d\n", maxSteerDeg,
                maxSteerDeg);
}

void toggleInversion() {
  steeringServo.setInverted(!steeringServo.isInverted());
  steeringServo.setSteeringDegrees(steerDeg);
  Serial.printf("Sentido invertido: %s -> en DriveConfig.h: SERVO_INVERTED = %s\n",
                steeringServo.isInverted() ? "si" : "no", steeringServo.isInverted() ? "true" : "false");
}

void advanceSweep(uint32_t nowMs) {
  if (!sweeping || nowMs - lastSweepStepMs < SWEEP_STEP_PERIOD_MS) {
    return;
  }
  lastSweepStepMs = nowMs;
  steerDeg += sweepDirection;
  if (steerDeg >= maxSteerDeg || steerDeg <= -maxSteerDeg) {
    steerDeg = constrain(steerDeg, -maxSteerDeg, maxSteerDeg);
    sweepDirection = -sweepDirection;
  }
  steeringServo.setSteeringDegrees(steerDeg);
}

void reportState() {
  Serial.printf("Giro: %+d grados (maximo +/-%d, centro %.0f)\n", steerDeg, maxSteerDeg, SERVO_CENTER_DEG + trimDeg);
}

void printHelp() {
  Serial.println("Prueba del servo. Envia: a izquierda | d derecha | c centrar | b barrido | [ ] centro | + - angulo maximo | i invertir | ? ayuda");
  reportState();
}
