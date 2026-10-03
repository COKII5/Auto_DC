# Proyecto Carro DC

Carro con tracción por motorreductores DC, dirección por servo, modo manual desde Unity 6 con mando y modo autónomo para esquivar obstáculos.

## Estado actual

**Última sesión (2026-10-03):** se creó el proyecto Unity en `unity/CarroDC` (6000.0.82f1, 3D URP + Input System), sin el tutorial de la plantilla ni Unity Version Control. Está subido a `desarrollo`. Aún no hay código del carro.
**Próximos pasos:**
- En Unity Hub, quitar la entrada `Auto_DC` y añadir `unity/CarroDC`. Definir el formato de mensajes UDP en `docs/`.
- Confirmar: voltaje de los N20 y si traen encoder, modelo del servo, mando y cargador.
- Confirmar la consigna: qué debe hacer el modo autónomo y que no exija el brushless.
- Conseguir el chasis y los materiales 🛒 de la checklist.

## Decisiones tomadas

### Arquitectura
- **Una sola ESP32** controla todo: motores, servo, sensores y comunicación con Unity.
- La **segunda ESP32 queda de repuesto**.
- El **motor brushless A2212 y el ESC A30 quedan fuera del proyecto**. No tienen marcha atrás y funcionan mal a baja velocidad.

### Tracción
- **Dos motorreductores N20 de 500 rpm** en las ruedas traseras, uno por rueda.
- Se controlan con un **driver L298N** (puente H), que da marcha adelante, marcha atrás y freno.
- **Diferencial electrónico:** en las curvas, la rueda interior gira más lenta que la exterior.

### Dirección
- **Servo** en las ruedas delanteras.
- El giro se **suaviza por software**: el servo avanza poco a poco hacia el ángulo objetivo, sin saltos.

### Modos de funcionamiento
- **Manual:** se conduce desde **Unity 6** con un mando (PlayStation o Xbox) a través del Input System.
- **Autónomo:** **esquivar obstáculos**. Se ejecuta **en la ESP32**, no en Unity, para que siga funcionando si se pierde la conexión.
- Desde el mando se puede cambiar entre manual y autónomo, y hay un botón de **parada de emergencia**.
- **Orden de prioridades:** parada de emergencia > failsafe (sin comunicación) > obstáculo muy cerca > órdenes del modo activo.

### Comunicación con Unity 6
- **Wi-Fi por UDP**, entre la ESP32 y el PC con Unity.
- La ESP32 envía los datos de los sensores y el estado del modo autónomo, **20–50 veces por segundo**.
- Unity envía las órdenes de conducción y de cambio de modo.
- Se usa una red Wi-Fi de **2,4 GHz**.

### Proyecto Unity
- **Unity 6000.0.82f1** (Unity 6 LTS) con la plantilla **3D URP**.
- Solo el **Input System** nuevo como sistema de entrada. Las acciones del proyecto están en `Assets/InputSystem_Actions.inputactions`.
- El proyecto tiene su propio `.gitignore` en `unity/CarroDC/`. `Library/`, `Logs/`, `UserSettings/` y los archivos del IDE no se suben.
- El control de versiones es **solo git**. Se quitó el paquete Unity Version Control (`com.unity.collab-proxy`) y el modo queda en **Visible Meta Files**.
- En Unity Hub se añade la carpeta `unity/CarroDC`, no la raíz del repo.

### Sensores
- **3 sensores ToF VL53L0X** (frente, izquierda y derecha) para detectar obstáculos.
- **IMU MPU6050** para la orientación del carro (opcional, pero recomendada).

### Alimentación
- **2 celdas 18650 en serie (2S)**: 7,4 V nominales y 8,4 V con carga completa.
- Las celdas van con un **BMS 2S** de protección.
- Un **regulador step-down a 5 V** alimenta la ESP32 y el servo.
- **GND común** entre todos los componentes.
- **Condensadores** en los motores y en la alimentación para reducir el ruido eléctrico.

## Checklist de materiales

Leyenda: ✅ ya lo tengo · ❓ falta confirmar · 🛒 hay que conseguirlo · ⭐ opcional pero recomendado.

| # | Componente | Cant. | Estado | Notas |
|---|---|---|---|---|
| **Control** |||||
| 1 | ESP32 DevKit | 2 | ✅ | Una en el carro y otra de repuesto |
| 2 | Cable USB de datos | 1 | ❓ | Que transmita datos, no solo carga |
| **Tracción** |||||
| 3 | Motorreductor N20 500 rpm | 2 | ✅❓ | Confirmar **voltaje** (3, 6 o 12 V) y si traen **encoder** |
| 4 | Soporte (bracket) para N20 | 2 | 🛒 | Para fijarlos al chasis |
| 5 | Ruedas traseras para eje N20 (3 mm, en D) | 2 | 🛒 | De unos 40–45 mm salen ~1 m/s |
| 6 | Driver L298N | 1 | ✅ | Pierde ~2 V (con 7,4 V los N20 reciben ~5,4 V como máximo). Quitar los jumpers ENA/ENB para controlar la velocidad por PWM |
| 7 | N20 con encoder | 2 | ⭐ | Solo si los actuales no lo traen. Mide la velocidad real |
| **Dirección** |||||
| 8 | Servo MG90S | 1 | ❓ | Confirmar modelo |
| 9 | Mecanismo de dirección (manguetas + bieleta) | 1 | 🛒 | Depende del chasis |
| 10 | Ruedas delanteras | 2 | 🛒 | Iguales o similares a las traseras |
| **Sensores** |||||
| 11 | Sensor ToF VL53L0X (o VL53L1X) | 3 | 🛒 | Frente, izquierda y derecha. Usan 3 pines XSHUT para cambiar la dirección I2C |
| 12 | IMU MPU6050 | 1 | ⭐ | Orientación del carro en Unity y giros precisos |
| **Alimentación** |||||
| 13 | Celdas 18650 3,7 V 3500 mAh (SGS Power INR) | 2 | ✅ | En serie: 7,4 V (8,4 V con carga completa) |
| 14 | Portapilas 2S para 18650 | 1 | 🛒 | |
| 15 | BMS 2S (placa de protección) | 1 | 🛒 | Protege de sobredescarga y cortocircuito |
| 16 | Cargador de 18650 | 1 | ❓ | Uno para celdas sueltas sirve |
| 17 | Interruptor general | 1 | 🛒 | |
| 18 | Regulador step-down a 5 V, mínimo 3 A | 1 | 🛒 | Alimenta la ESP32 (pin 5V) y el servo |
| 19 | Condensadores cerámicos 100 nF | 4–6 | 🛒 | Soldados en los bornes de los motores |
| 20 | Condensadores electrolíticos 470 µF 16 V | 2 | 🛒 | A la salida del regulador y junto al servo |
| 21 | Resistencias 100 kΩ + 47 kΩ | 1 de cada | ⭐ | Divisor para medir la batería y verla en Unity |
| **Chasis y montaje** |||||
| 22 | Chasis | 1 | 🛒 | Ligero (menos de 600–800 g en total), porque los N20 tienen poco par |
| 23 | Protoboard mini o placa perforada | 1 | 🛒 | La placa perforada soldada aguanta mejor las vibraciones |
| 24 | Cables dupont, tornillería M2/M3, bridas, termorretráctil | — | 🛒 | |
| **PC / Unity** |||||
| 25 | PC con Unity 6 | 1 | ✅ | |
| 26 | Red Wi-Fi de 2,4 GHz | 1 | ❓ | La ESP32 no funciona en 5 GHz. Sirve el router o un punto de acceso del móvil o del PC |
| 27 | Mando PS4, PS5 o Xbox | 1 | ❓ | Cualquiera funciona con el Input System de Unity 6 |

## Estructura

```
Auto_DC/
├── firmware/carro_dc/   Sketch principal de la ESP32
├── pruebas/             Un sketch por prueba: pruebas/<nombre>/<nombre>.ino
├── unity/CarroDC/       Proyecto Unity 6 (Assets, Packages, ProjectSettings)
├── docs/                Consigna, datasheets, fotos y formato de mensajes UDP
└── hardware/            Esquema de conexiones y archivos del chasis (STL/CAD)
```

## Código

| Archivo | Estado | Qué hace |
|---|---|---|
| `pruebas/prueba_brushless_a2212/prueba_brushless_a2212.ino` | Archivado | Prueba inicial del A2212 con ESC: arma, rampa de 1,5 s, gira 5 s y para. No forma parte del carro final |
| `pruebas/prueba_motores_l298n/prueba_motores_l298n.ino` | Archivado | Prueba inicial para Arduino Uno: dos motores DC con L298N (IN1–IN4 = 11, 10, 6, 5), adelante 2 s, para y atrás. Sin PWM |
| `unity/CarroDC/` | Creado | Proyecto vacío de la plantilla 3D URP: escena `Assets/Scenes/SampleScene.unity`, ajustes URP en `Assets/Settings/` y acciones por defecto del Input System. Sin Unity Version Control. Sin scripts propios |

*Sketch principal del carro: aún no creado.*
