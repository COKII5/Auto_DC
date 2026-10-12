#include <Arduino.h>

#include "../../AutoDcController/DistanceSensor.h"
#include "../../AutoDcController/DriveConfig.h"

constexpr uint32_t PRINT_PERIOD_MS = 250;
constexpr uint16_t OBSTACLE_WARNING_MM = 250;
constexpr float MILLIMETERS_PER_METER = 1000.0f;

DistanceSensor distanceSensor;

uint32_t validReadings = 0;
uint32_t missedReadings = 0;
bool printing = true;
uint32_t lastPrintMs = 0;

void setup() {
  Serial.begin(SERIAL_BAUD_RATE);
  distanceSensor.begin();
  printHelp();
}

void loop() {
  handleSerialInput();
  if (distanceSensor.update(micros())) {
    countReading();
  }
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

void countReading() {
  if (distanceSensor.distanceMm() == DISTANCE_UNKNOWN_MM) {
    missedReadings++;
  } else {
    validReadings++;
  }
}

void printIfDue(uint32_t nowMs) {
  if (!printing || nowMs - lastPrintMs < PRINT_PERIOD_MS) {
    return;
  }
  lastPrintMs = nowMs;
  uint16_t distanceMm = distanceSensor.distanceMm();
  if (distanceMm == DISTANCE_UNKNOWN_MM) {
    Serial.printf("Sin lectura (sin eco o fuera de %u a %u mm)\n", DISTANCE_MIN_MM, DISTANCE_MAX_MM);
    return;
  }
  Serial.printf("Distancia: %4u mm (%.2f m, eco %lu us)%s\n", distanceMm, distanceMm / MILLIMETERS_PER_METER,
                (unsigned long)distanceSensor.lastEchoWidthUs(),
                distanceMm <= OBSTACLE_WARNING_MM ? "  <- OBSTACULO" : "");
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
  uint32_t edges = DistanceSensor::takeEchoEdgeCount();
  Serial.printf("Cambios de nivel en el eco desde el ultimo 'd': %lu | ECHO ahora: %s | ultimo ancho: %lu us\n",
                (unsigned long)edges, digitalRead(PIN_DISTANCE_ECHO) == HIGH ? "ALTO" : "BAJO",
                (unsigned long)distanceSensor.lastEchoWidthUs());
  if (edges == 0) {
    Serial.printf("-> El eco no llega al GPIO %u: revisa VCC (5 V), GND comun, TRIG en GPIO %u y el divisor\n",
                  PIN_DISTANCE_ECHO, PIN_DISTANCE_TRIGGER);
  }
}

void printHelp() {
  Serial.println("Prueba del HC-SR04 frontal. Envia: p pausar/reanudar | e estadisticas | d diagnostico | ? ayuda");
  Serial.printf("Aviso de obstaculo a %u mm o menos\n", OBSTACLE_WARNING_MM);
}
