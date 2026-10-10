#include <Arduino.h>

constexpr uint32_t SERIAL_BAUD_RATE = 115200;

constexpr uint8_t PIN_SERVO = 13;
constexpr uint32_t SERVO_PWM_FREQUENCY_HZ = 50;
constexpr uint8_t SERVO_PWM_RESOLUTION_BITS = 14;
constexpr float SERVO_PERIOD_US = 1000000.0f / SERVO_PWM_FREQUENCY_HZ;
constexpr float SERVO_MIN_PULSE_US = 500.0f;
constexpr float SERVO_MAX_PULSE_US = 2500.0f;
constexpr float SERVO_RANGE_DEG = 180.0f;
constexpr float SERVO_CENTER_DEG = 90.0f;
constexpr uint32_t SERVO_MAX_DUTY = (1u << SERVO_PWM_RESOLUTION_BITS) - 1;

constexpr int STEER_STEP_DEG = 5;
constexpr int MIN_MAX_STEER_DEG = 5;
constexpr int LIMIT_MAX_STEER_DEG = 60;
constexpr int INITIAL_MAX_STEER_DEG = 30;
constexpr int MAX_TRIM_DEG = 15;
constexpr uint32_t SWEEP_STEP_PERIOD_MS = 20;

int steerDeg = 0;
int trimDeg = 0;
int maxSteerDeg = INITIAL_MAX_STEER_DEG;
bool inverted = false;
bool sweeping = false;
int sweepDirection = 1;
uint32_t lastSweepStepMs = 0;

void setup() {
  Serial.begin(SERIAL_BAUD_RATE);
  if (!ledcAttach(PIN_SERVO, SERVO_PWM_FREQUENCY_HZ, SERVO_PWM_RESOLUTION_BITS)) {
    Serial.println("Error: no se pudo asignar el PWM al pin del servo");
  }
  applyAngle();
  printHelp();
}

void loop() {
  handleSerialInput();
  advanceSweep(millis());
}

void handleSerialInput() {
  while (Serial.available() > 0) {
    char key = (char)Serial.read();
    if (key == '\r' || key == '\n' || key == ' ') {
      continue;
    }
    if (key != 'b') {
      sweeping = false;
    }
    switch (tolower(key)) {
      case 'a':
        moveSteer(-STEER_STEP_DEG);
        break;
      case 'd':
        moveSteer(STEER_STEP_DEG);
        break;
      case 'c':
        steerDeg = 0;
        applyAngle();
        reportState();
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
        inverted = !inverted;
        applyAngle();
        Serial.printf("Sentido invertido: %s -> en DriveConfig.h: SERVO_INVERTED = %s\n", inverted ? "si" : "no",
                      inverted ? "true" : "false");
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
  applyAngle();
  reportState();
}

void changeTrim(int deltaDeg) {
  trimDeg = constrain(trimDeg + deltaDeg, -MAX_TRIM_DEG, MAX_TRIM_DEG);
  applyAngle();
  Serial.printf("Centro: %.0f grados (trim %+d) -> en DriveConfig.h: SERVO_CENTER_DEG = %.1f\n",
                SERVO_CENTER_DEG + trimDeg, trimDeg, SERVO_CENTER_DEG + trimDeg);
}

void changeMaxSteer(int deltaDeg) {
  maxSteerDeg = constrain(maxSteerDeg + deltaDeg, MIN_MAX_STEER_DEG, LIMIT_MAX_STEER_DEG);
  steerDeg = constrain(steerDeg, -maxSteerDeg, maxSteerDeg);
  applyAngle();
  Serial.printf("Angulo maximo: %d grados -> en DriveConfig.h: SERVO_MAX_STEER_DEG = %d\n", maxSteerDeg,
                maxSteerDeg);
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
  applyAngle();
}

void applyAngle() {
  int direction = inverted ? -1 : 1;
  float degrees = SERVO_CENTER_DEG + trimDeg + direction * steerDeg;
  float clampedDegrees = constrain(degrees, 0.0f, SERVO_RANGE_DEG);
  float pulseUs = SERVO_MIN_PULSE_US + (SERVO_MAX_PULSE_US - SERVO_MIN_PULSE_US) * clampedDegrees / SERVO_RANGE_DEG;
  ledcWrite(PIN_SERVO, (uint32_t)(pulseUs * SERVO_MAX_DUTY / SERVO_PERIOD_US));
}

void reportState() {
  Serial.printf("Giro: %+d grados (maximo +/-%d, centro %.0f)\n", steerDeg, maxSteerDeg, SERVO_CENTER_DEG + trimDeg);
}

void printHelp() {
  Serial.println("Prueba del servo. Envia: a izquierda | d derecha | c centrar | b barrido | [ ] centro | + - angulo maximo | i invertir | ? ayuda");
  reportState();
}
