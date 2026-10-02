/*
  Motor brushless A2212 1000KV con ESC: gira 5 segundos y se detiene

  Conexiones:
    ESC cable de señal (naranja/amarillo/blanco) -> pin 9
    ESC cable café/negro fino (GND)              -> GND del Arduino
    ESC cable rojo fino (BEC 5V)                 -> SIN CONECTAR si el Arduino va por USB
    ESC cables gruesos                           -> batería
    3 cables del ESC                             -> 3 cables del motor

  Al encender: arma el ESC, sube suavemente la velocidad (para reducir el pico
  de corriente del arranque), mantiene el giro hasta completar TIEMPO_GIRO_MS
  y se detiene. Para repetirlo, pulsa el botón de reset del Arduino.
*/

#include <Servo.h>

const uint8_t  PIN_ESC         = 9;
const uint16_t PULSO_PARADO_US = 1000;   // motor parado
const uint16_t PULSO_INICIO_US = 1100;   // potencia con la que empieza a girar
const uint16_t PULSO_GIRO_US   = 2000;   // velocidad de giro (2000 = 100 %)
const uint32_t TIEMPO_ARMADO   = 3000;   // ms enviando "parado" para armar el ESC
const uint32_t TIEMPO_RAMPA_MS = 1500;   // ms de subida suave (incluidos en el giro)
const uint32_t TIEMPO_GIRO_MS  = 5000;   // ms de giro en total

Servo esc;

void setup() {
  esc.attach(PIN_ESC, PULSO_PARADO_US, PULSO_GIRO_US);

  // Armar el ESC
  esc.writeMicroseconds(PULSO_PARADO_US);
  delay(TIEMPO_ARMADO);

  // Girar: subida suave y después velocidad constante hasta cumplir el tiempo
  uint32_t inicio = millis();
  uint32_t pasado;
  while ((pasado = millis() - inicio) < TIEMPO_GIRO_MS) {
    if (pasado < TIEMPO_RAMPA_MS) {
      esc.writeMicroseconds(map(pasado, 0, TIEMPO_RAMPA_MS, PULSO_INICIO_US, PULSO_GIRO_US));
    } else {
      esc.writeMicroseconds(PULSO_GIRO_US);
    }
    delay(20);
  }

  // Detener
  esc.writeMicroseconds(PULSO_PARADO_US);
}

void loop() {
  // El motor queda detenido. Pulsa reset para volver a girar.
}
