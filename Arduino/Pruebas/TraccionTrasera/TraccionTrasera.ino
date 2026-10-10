#include <Arduino.h>

constexpr uint32_t SERIAL_BAUD_RATE = 115200;

constexpr uint8_t PIN_LEFT_ENABLE = 25;
constexpr uint8_t PIN_LEFT_FORWARD = 27;
constexpr uint8_t PIN_LEFT_REVERSE = 26;
constexpr uint8_t PIN_RIGHT_ENABLE = 14;
constexpr uint8_t PIN_RIGHT_FORWARD = 32;
constexpr uint8_t PIN_RIGHT_REVERSE = 33;

constexpr uint32_t MOTOR_PWM_FREQUENCY_HZ = 1000;
constexpr uint8_t MOTOR_PWM_RESOLUTION_BITS = 8;
constexpr float MOTOR_MAX_DUTY_RATIO = 0.675f;
constexpr uint32_t MAX_DUTY = (uint32_t)(((1u << MOTOR_PWM_RESOLUTION_BITS) - 1) * MOTOR_MAX_DUTY_RATIO);

constexpr uint32_t RUN_TIMEOUT_MS = 2000;
constexpr uint8_t DEFAULT_SPEED_PERCENT = 40;
constexpr uint8_t MIN_SPEED_PERCENT = 10;
constexpr uint8_t MAX_SPEED_PERCENT = 100;
constexpr uint8_t SPEED_STEP_PERCENT = 10;

bool leftInverted = false;
bool rightInverted = false;
uint8_t speedPercent = DEFAULT_SPEED_PERCENT;
bool running = false;
bool continuousMode = false;
uint32_t lastForwardCommandMs = 0;

void setup() {
  Serial.begin(SERIAL_BAUD_RATE);
  pinMode(PIN_LEFT_FORWARD, OUTPUT);
  pinMode(PIN_LEFT_REVERSE, OUTPUT);
  pinMode(PIN_RIGHT_FORWARD, OUTPUT);
  pinMode(PIN_RIGHT_REVERSE, OUTPUT);
  if (!ledcAttach(PIN_LEFT_ENABLE, MOTOR_PWM_FREQUENCY_HZ, MOTOR_PWM_RESOLUTION_BITS) ||
      !ledcAttach(PIN_RIGHT_ENABLE, MOTOR_PWM_FREQUENCY_HZ, MOTOR_PWM_RESOLUTION_BITS)) {
    Serial.println("Error: no se pudo asignar el PWM a ENA/ENB");
  }
  stopMotors();
  printHelp();
}

void loop() {
  handleSerialInput();
  stopIfTimedOut(millis());
}

void handleSerialInput() {
  while (Serial.available() > 0) {
    char key = (char)tolower(Serial.read());
    switch (key) {
      case 'w':
        continuousMode = false;
        driveForward(millis());
        break;
      case 'c':
        continuousMode = true;
        driveForward(millis());
        Serial.println("Modo continuo: los motores siguen girando hasta enviar 's'");
        break;
      case 's':
        stopMotors();
        Serial.println("Motores detenidos");
        break;
      case '+':
        changeSpeed((int)SPEED_STEP_PERCENT);
        break;
      case '-':
        changeSpeed(-(int)SPEED_STEP_PERCENT);
        break;
      case 'l':
        leftInverted = !leftInverted;
        reportInversion("izquierdo", "LEFT_MOTOR_INVERTED", leftInverted);
        break;
      case 'r':
        rightInverted = !rightInverted;
        reportInversion("derecho", "RIGHT_MOTOR_INVERTED", rightInverted);
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
  applyMotors();
  Serial.printf("Adelante al %u %% (la potencia maxima es el %.1f %% del PWM)\n", speedPercent,
                MOTOR_MAX_DUTY_RATIO * 100.0f);
}

void stopIfTimedOut(uint32_t nowMs) {
  if (running && !continuousMode && nowMs - lastForwardCommandMs > RUN_TIMEOUT_MS) {
    stopMotors();
    Serial.println("Sin nuevas 'w' en 2 s: motores detenidos");
  }
}

void changeSpeed(int deltaPercent) {
  int updated = constrain((int)speedPercent + deltaPercent, (int)MIN_SPEED_PERCENT, (int)MAX_SPEED_PERCENT);
  speedPercent = (uint8_t)updated;
  Serial.printf("Potencia: %u %%\n", speedPercent);
  if (running) {
    applyMotors();
  }
}

void reportInversion(const char* side, const char* constantName, bool inverted) {
  Serial.printf("Motor %s invertido: %s -> en DriveConfig.h: %s = %s\n", side, inverted ? "si" : "no",
                constantName, inverted ? "true" : "false");
  if (running) {
    applyMotors();
  }
}

void applyMotors() {
  uint32_t duty = MAX_DUTY * speedPercent / 100;
  setMotorForward(PIN_LEFT_ENABLE, PIN_LEFT_FORWARD, PIN_LEFT_REVERSE, leftInverted, duty);
  setMotorForward(PIN_RIGHT_ENABLE, PIN_RIGHT_FORWARD, PIN_RIGHT_REVERSE, rightInverted, duty);
}

void setMotorForward(uint8_t enablePin, uint8_t forwardPin, uint8_t reversePin, bool inverted, uint32_t duty) {
  digitalWrite(forwardPin, inverted ? LOW : HIGH);
  digitalWrite(reversePin, inverted ? HIGH : LOW);
  ledcWrite(enablePin, duty);
}

void stopMotors() {
  running = false;
  continuousMode = false;
  digitalWrite(PIN_LEFT_FORWARD, LOW);
  digitalWrite(PIN_LEFT_REVERSE, LOW);
  digitalWrite(PIN_RIGHT_FORWARD, LOW);
  digitalWrite(PIN_RIGHT_REVERSE, LOW);
  ledcWrite(PIN_LEFT_ENABLE, 0);
  ledcWrite(PIN_RIGHT_ENABLE, 0);
}

void printHelp() {
  Serial.println("Prueba de traccion trasera. Envia: w adelante | c continuo | s detener | + / - potencia | l / r invertir motor | ? ayuda");
  Serial.printf("Potencia inicial: %u %%\n", speedPercent);
}
