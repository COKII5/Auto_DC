#include <Arduino.h>

#include "../../AutoDcController/DistanceSensor.h"
#include "../../AutoDcController/DriveConfig.h"
#include "../../AutoDcController/MotorDriver.h"
#include "../../AutoDcController/SteeringServo.h"

constexpr uint32_t RUN_TIMEOUT_MS = 2000;
constexpr uint32_t STATUS_PERIOD_MS = 500;
constexpr uint16_t OBSTACLE_STOP_MM = 250;
constexpr int DEFAULT_SPEED_PERCENT = 60;
constexpr int MIN_SPEED_PERCENT = 10;
constexpr int MAX_SPEED_PERCENT = 100;
constexpr int SPEED_STEP_PERCENT = 10;
constexpr int STEER_STEP_PERCENT = 20;

enum class Motion : int8_t { REVERSE = -1, STOPPED = 0, FORWARD = 1 };

MotorDriver leftMotor{PIN_LEFT_ENABLE, PIN_LEFT_FORWARD, PIN_LEFT_REVERSE, LEFT_MOTOR_INVERTED};
MotorDriver rightMotor{PIN_RIGHT_ENABLE, PIN_RIGHT_FORWARD, PIN_RIGHT_REVERSE, RIGHT_MOTOR_INVERTED};
SteeringServo steeringServo{PIN_SERVO};
DistanceSensor distanceSensor;

Motion motion = Motion::STOPPED;
int speedPercent = DEFAULT_SPEED_PERCENT;
int steeringPercent = 0;
bool continuousMode = false;
bool obstacleBrakeEnabled = true;
bool blockedByObstacle = false;
uint32_t lastMotionCommandMs = 0;
uint32_t lastUpdateMs = 0;
uint32_t lastStatusMs = 0;

void setup() {
  Serial.begin(SERIAL_BAUD_RATE);
  if (!leftMotor.begin() || !rightMotor.begin() || !steeringServo.begin()) {
    Serial.println("Error: no se pudo asignar el PWM a los pines de DriveConfig.h");
  }
  distanceSensor.begin();
  printHelp();
}

void loop() {
  uint32_t nowMs = millis();
  handleSerialInput(nowMs);
  distanceSensor.update(micros());
  enforceObstacleBrake();
  stopIfTimedOut(nowMs);
  leftMotor.update(nowMs - lastUpdateMs);
  rightMotor.update(nowMs - lastUpdateMs);
  lastUpdateMs = nowMs;
  printStatusIfDue(nowMs);
}

void handleSerialInput(uint32_t nowMs) {
  while (Serial.available() > 0) {
    switch (tolower(Serial.read())) {
      case 'w':
        startMotion(Motion::FORWARD, nowMs);
        break;
      case 's':
        startMotion(Motion::REVERSE, nowMs);
        break;
      case 'x':
        stopMotion("Detenido");
        break;
      case 'm':
        continuousMode = !continuousMode;
        Serial.println(continuousMode ? "Modo continuo activo (detener con 'x')" : "Modo continuo desactivado");
        break;
      case 'a':
        changeSteering(-STEER_STEP_PERCENT);
        break;
      case 'd':
        changeSteering(STEER_STEP_PERCENT);
        break;
      case 'c':
        changeSteering(-steeringPercent);
        break;
      case '+':
        changeSpeed(SPEED_STEP_PERCENT);
        break;
      case '-':
        changeSpeed(-SPEED_STEP_PERCENT);
        break;
      case 'o':
        obstacleBrakeEnabled = !obstacleBrakeEnabled;
        Serial.printf("Freno por obstaculo: %s\n", obstacleBrakeEnabled ? "activo" : "desactivado");
        break;
      case '?':
        printHelp();
        break;
      default:
        break;
    }
  }
}

void startMotion(Motion requested, uint32_t nowMs) {
  if (requested == Motion::FORWARD && isObstacleAhead()) {
    Serial.println("No avanza: obstaculo al frente");
    return;
  }
  motion = requested;
  lastMotionCommandMs = nowMs;
  applyMotorTargets();
}

void stopMotion(const char* reason) {
  motion = Motion::STOPPED;
  leftMotor.brake();
  rightMotor.brake();
  Serial.println(reason);
}

void stopIfTimedOut(uint32_t nowMs) {
  if (motion != Motion::STOPPED && !continuousMode && nowMs - lastMotionCommandMs > RUN_TIMEOUT_MS) {
    stopMotion("Sin nuevas ordenes en 2 s: detenido");
  }
}

void enforceObstacleBrake() {
  bool obstacleAhead = isObstacleAhead();
  if (motion == Motion::FORWARD && obstacleAhead) {
    stopMotion("Frenado por obstaculo al frente");
  }
  blockedByObstacle = obstacleAhead;
}

bool isObstacleAhead() {
  uint16_t distanceMm = distanceSensor.distanceMm();
  return obstacleBrakeEnabled && distanceMm != DISTANCE_UNKNOWN_MM && distanceMm <= OBSTACLE_STOP_MM;
}

void changeSpeed(int deltaPercent) {
  speedPercent = constrain(speedPercent + deltaPercent, MIN_SPEED_PERCENT, MAX_SPEED_PERCENT);
  Serial.printf("Potencia: %d %%\n", speedPercent);
  applyMotorTargets();
}

void changeSteering(int deltaPercent) {
  steeringPercent = constrain(steeringPercent + deltaPercent, -COMMAND_LIMIT, (int)COMMAND_LIMIT);
  steeringServo.setSteering((int8_t)steeringPercent);
  Serial.printf("Direccion: %+d %%\n", steeringPercent);
}

void applyMotorTargets() {
  if (motion == Motion::STOPPED) {
    return;
  }
  float target = (float)speedPercent * (int)motion;
  leftMotor.setTarget(target);
  rightMotor.setTarget(target);
}

void printStatusIfDue(uint32_t nowMs) {
  if (nowMs - lastStatusMs < STATUS_PERIOD_MS) {
    return;
  }
  lastStatusMs = nowMs;
  uint16_t distanceMm = distanceSensor.distanceMm();
  char distanceText[24];
  if (distanceMm == DISTANCE_UNKNOWN_MM) {
    snprintf(distanceText, sizeof(distanceText), "sin lectura");
  } else {
    snprintf(distanceText, sizeof(distanceText), "%u mm", distanceMm);
  }
  Serial.printf("[%s] potencia %d %% | direccion %+d %% | distancia %s%s\n", motionName(), speedPercent,
                steeringPercent, distanceText, blockedByObstacle ? " | OBSTACULO" : "");
}

const char* motionName() {
  switch (motion) {
    case Motion::FORWARD:
      return continuousMode ? "ADELANTE continuo" : "ADELANTE";
    case Motion::REVERSE:
      return continuousMode ? "REVERSA continua" : "REVERSA";
    default:
      return "DETENIDO";
  }
}

void printHelp() {
  Serial.println("Prueba integral. Envia: w adelante | s reversa | x detener | m continuo | a d girar | c centrar");
  Serial.println("                        + - potencia | o freno por obstaculo | ? ayuda");
  Serial.printf("Potencia inicial %d %%, freno por obstaculo a %u mm o menos\n", speedPercent, OBSTACLE_STOP_MM);
}
