#include <Arduino.h>

constexpr uint32_t SERIAL_BAUD_RATE = 115200;

constexpr uint8_t PIN_DISTANCE_TRIGGER = 18;
constexpr uint8_t PIN_DISTANCE_ECHO = 34;

constexpr uint32_t DISTANCE_MEASURE_PERIOD_US = 60000;
constexpr uint32_t DISTANCE_TRIGGER_PULSE_US = 10;
constexpr uint32_t DISTANCE_ECHO_TIMEOUT_US = 30000;
constexpr uint16_t DISTANCE_MIN_MM = 20;
constexpr uint16_t DISTANCE_MAX_MM = 4000;
constexpr float SPEED_OF_SOUND_MM_PER_US = 0.343f;

constexpr uint32_t PRINT_PERIOD_MS = 250;
constexpr uint16_t OBSTACLE_WARNING_MM = 250;

enum class MeasureState : uint8_t { IDLE, TRIGGERING, WAITING_ECHO };

volatile uint32_t echoRiseUs = 0;
volatile uint32_t echoWidthUs = 0;
volatile bool echoCaptured = false;
volatile uint32_t echoEdgeCount = 0;

MeasureState measureState = MeasureState::IDLE;
uint32_t cycleStartUs = 0;
bool hasReading = false;
uint16_t lastDistanceMm = 0;
uint32_t lastEchoWidthUs = 0;
uint32_t validReadings = 0;
uint32_t missedReadings = 0;
bool printing = true;
uint32_t lastPrintMs = 0;

void IRAM_ATTR onEchoChange() {
  uint32_t nowUs = micros();
  echoEdgeCount = echoEdgeCount + 1;
  if (digitalRead(PIN_DISTANCE_ECHO) == HIGH) {
    echoRiseUs = nowUs;
  } else {
    echoWidthUs = nowUs - echoRiseUs;
    echoCaptured = true;
  }
}

void setup() {
  Serial.begin(SERIAL_BAUD_RATE);
  pinMode(PIN_DISTANCE_TRIGGER, OUTPUT);
  digitalWrite(PIN_DISTANCE_TRIGGER, LOW);
  pinMode(PIN_DISTANCE_ECHO, INPUT);
  attachInterrupt(digitalPinToInterrupt(PIN_DISTANCE_ECHO), onEchoChange, CHANGE);
  printHelp();
}

void loop() {
  handleSerialInput();
  updateMeasurement(micros());
  printIfDue(millis());
}

void handleSerialInput() {
  while (Serial.available() > 0) {
    switch (tolower(Serial.read())) {
      case 'p':
        printing = !printing;
        Serial.println(printing ? "Impresion reanudada" : "Impresion en pausa (envia 'p' para reanudar)");
        break;
      case 'e':
        reportStatistics();
        break;
      case 'd':
        reportDiagnostics();
        break;
      case '?':
        printHelp();
        break;
      default:
        break;
    }
  }
}

void updateMeasurement(uint32_t nowUs) {
  uint32_t elapsedUs = nowUs - cycleStartUs;
  switch (measureState) {
    case MeasureState::IDLE:
      if (elapsedUs >= DISTANCE_MEASURE_PERIOD_US) {
        echoCaptured = false;
        digitalWrite(PIN_DISTANCE_TRIGGER, HIGH);
        cycleStartUs = nowUs;
        measureState = MeasureState::TRIGGERING;
      }
      break;
    case MeasureState::TRIGGERING:
      if (elapsedUs >= DISTANCE_TRIGGER_PULSE_US) {
        digitalWrite(PIN_DISTANCE_TRIGGER, LOW);
        measureState = MeasureState::WAITING_ECHO;
      }
      break;
    case MeasureState::WAITING_ECHO:
      if (echoCaptured) {
        recordEcho(echoWidthUs);
        measureState = MeasureState::IDLE;
      } else if (elapsedUs >= DISTANCE_ECHO_TIMEOUT_US) {
        recordMissedEcho();
        measureState = MeasureState::IDLE;
      }
      break;
  }
}

void recordEcho(uint32_t widthUs) {
  lastEchoWidthUs = widthUs;
  float millimeters = widthUs * SPEED_OF_SOUND_MM_PER_US / 2.0f;
  if (millimeters < DISTANCE_MIN_MM || millimeters > DISTANCE_MAX_MM) {
    recordMissedEcho();
    return;
  }
  lastDistanceMm = (uint16_t)millimeters;
  hasReading = true;
  validReadings++;
}

void recordMissedEcho() {
  hasReading = false;
  missedReadings++;
}

void printIfDue(uint32_t nowMs) {
  if (!printing || nowMs - lastPrintMs < PRINT_PERIOD_MS) {
    return;
  }
  lastPrintMs = nowMs;
  if (!hasReading) {
    Serial.println("Sin lectura (sin eco o fuera de 20 a 4000 mm)");
    return;
  }
  Serial.printf("Distancia: %4u mm (%.2f m, eco %lu us)%s\n", lastDistanceMm, lastDistanceMm / 1000.0f,
                (unsigned long)lastEchoWidthUs, lastDistanceMm <= OBSTACLE_WARNING_MM ? "  <- OBSTACULO" : "");
}

void reportStatistics() {
  uint32_t total = validReadings + missedReadings;
  float validPercent = total == 0 ? 0.0f : 100.0f * validReadings / total;
  Serial.printf("Lecturas validas: %lu de %lu (%.1f %%). Estadisticas reiniciadas.\n", (unsigned long)validReadings,
                (unsigned long)total, validPercent);
  validReadings = 0;
  missedReadings = 0;
}

void reportDiagnostics() {
  uint32_t edges = echoEdgeCount;
  echoEdgeCount = 0;
  Serial.printf("Cambios de nivel en el eco desde el ultimo 'd': %lu | ECHO ahora: %s | ultimo ancho: %lu us\n",
                (unsigned long)edges, digitalRead(PIN_DISTANCE_ECHO) == HIGH ? "ALTO" : "BAJO",
                (unsigned long)lastEchoWidthUs);
  if (edges == 0) {
    Serial.println("-> El eco no llega al GPIO 34: revisa VCC (5 V), GND comun, TRIG en D18 y el divisor");
  }
}

void printHelp() {
  Serial.println("Prueba del HC-SR04 frontal. Envia: p pausar/reanudar | e estadisticas | d diagnostico | ? ayuda");
  Serial.printf("Aviso de obstaculo a %u mm o menos\n", OBSTACLE_WARNING_MM);
}
