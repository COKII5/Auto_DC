# Auto DC: control por Wi-Fi y gemelo digital en Unity

Carro DC con **ESP32 + L298N + 2 motorreductores N20 + servo de dirección + sensor de distancia HC-SR04**, que se maneja desde **Unity 6** con teclado o gamepad. Unity muestra un **gemelo digital** 3D que se mueve como el carro real, usando los valores que el propio carro reporta que aplicó.

## Estructura

```
Arduino/AutoDcController/   firmware del ESP32 (Arduino IDE, core ESP32 3.x)
  DriveConfig.h             pines, modo de dirección, límites de PWM y del servo, red Wi-Fi
  CarProtocol.h             formato de paquetes y CRC-8
  MotorDriver.h             un canal del L298N con rampa de arranque
  SteeringServo.h           servo por LEDC a 50 Hz (sin librerías externas)
  DriveTrain.h              mezcla acelerador/giro según el modo de dirección
  DistanceSensor.h          HC-SR04 sin bloqueo (interrupción en el eco)
Arduino/Pruebas/            pruebas por partes, manejadas por el Monitor Serie (115200)
  TraccionTrasera/          motores hacia adelante, potencia e inversión de cada motor
  ServoDireccion/           giro, centro (trim), ángulo máximo e inversión del servo
  SensorFrontal/            distancia del HC-SR04 y diagnóstico del eco
  PruebaIntegral/           todo junto, con freno por obstáculo (usa las clases del firmware)
Unity/                      proyecto Unity 6 (6000.0.82f1)
  Assets/Scripts/Protocol       paquetes y CRC (sin dependencias de Unity)
  Assets/Scripts/Communication  enlace UDP, sesión con el carro, RTT y estado
  Assets/Scripts/Control        lectura de teclado y gamepad (Input System)
  Assets/Scripts/Twin           cinemática y modelo 3D del gemelo
  Assets/Scripts/World          piso, luz y cámara
  Assets/Scripts/UI             paneles de conexión y conducción
  Assets/Editor                 crea la escena automáticamente
Tools/fake_car.py           carro simulado para probar Unity sin hardware
Informe/                    copia corregida del informe del primer corte (LaTeX, IEEE)
  compute_values.py         calcula los valores del informe y los escribe en informe_carro.tex
BITACORA.md                 registro de cambios, pruebas, mediciones y decisiones
```

## Conexiones

| Elemento | Conexión |
|---|---|
| Baterías 2 × 3,7 V (7,4 V) | `+12V` y `GND` del L298N |
| L298N `ENA` / `IN1` / `IN2` (motor izquierdo) | GPIO 25 / 26 / 27 |
| L298N `ENB` / `IN3` / `IN4` (motor derecho) | GPIO 14 / 33 / 32 |
| Servo: señal (naranja o amarillo) | GPIO 13 |
| Servo: rojo / café o negro | `+5V` / `GND` del L298N |
| HC-SR04 `VCC` / `GND` | `+5V` / `GND` del L298N |
| HC-SR04 `TRIG` | GPIO 18 |
| HC-SR04 `ECHO` | Divisor de tensión (1 kΩ en serie y 2 kΩ a `GND`) → GPIO 34 |
| ESP32 `GND` | `GND` del L298N (tierra común) |

- Hay que **quitar los jumpers de ENA y ENB** del L298N para que el ESP32 controle la velocidad por PWM.
- El servo se alimenta del regulador de 5 V del L298N, **no** del pin de 3,3 V del ESP32. La señal de 3,3 V del ESP32 suele bastar para servos de 5 V.
- Si el ESP32 se reinicia cuando arrancan los motores o se mueve el servo (caída de voltaje), el panel de Unity lo muestra: "Carro encendido hace N s" vuelve a 0. Un condensador de 470–1000 µF entre `+5V` y `GND` cerca del servo ayuda.
- **El eco del HC-SR04 sale a 5 V y los GPIO del ESP32 solo toleran 3,3 V.** Sin el divisor se puede dañar el pin. Con 1 kΩ entre `ECHO` y el GPIO, y 2 kΩ entre el GPIO y `GND`, el GPIO ve 3,3 V. El GPIO 34 es solo de entrada, ideal para el eco. La señal `TRIG` de 3,3 V basta para disparar el sensor.
- Los pines se cambian en `DriveConfig.h`. Se evitan los *strapping pins* del ESP32 (0, 2, 5, 12, 15).

## Puesta en marcha

1. **Firmware.** Abre `Arduino/AutoDcController/AutoDcController.ino` en Arduino IDE, elige la placa *ESP32 Dev Module* y cárgalo. En el Monitor Serial (115200) debe aparecer `Red AutoDC lista. Unity debe enviar a 192.168.4.1:4210`.
2. **Red.** Conecta el PC a la red Wi-Fi `AutoDC` (clave `autodc1234`, se cambia en `DriveConfig.h`).
3. **Unity.** En Unity Hub: *Add → Add project from disk* → carpeta `Unity/`. Al abrirse, `Assets/Editor/ProjectSetup.cs` crea y abre la escena `Assets/Scenes/AutoDC.unity` (también existe el menú **AutoDC → Create Scene**). Presiona **Play**.
4. **Conectar.** En el panel *CONEXIÓN* deja `192.168.4.1` y `4210` y presiona **Conectar**. El estado pasa a *Conectado* y aparecen el RTT y la frecuencia de telemetría.

Sin conexión el gemelo simula localmente con los mismos controles, así que se puede practicar sin el carro.

### Probar sin el carro

```bash
python Tools/fake_car.py
```

En Unity usa la IP `127.0.0.1`. El script valida el CRC de cada comando, responde la telemetría y simula el failsafe. Con `--mode differential` simula el futuro modo diferencial.

## Sensor de distancia HC-SR04

El firmware mide cada 60 ms sin bloquear el `loop()`: `TRIG` se sube 10 µs con `micros()` y el ancho del eco se captura en una interrupción. La distancia es `ancho × 0,343 mm/µs ÷ 2`, con la velocidad del sonido a 20 °C (`SPEED_OF_SOUND_MM_PER_US` en `DriveConfig.h`). Los pines y los límites de medición también están ahí.

El sensor solo informa: no frena el carro. Para probar Unity sin el carro, `python Tools/fake_car.py --distance-mm 150` simula un obstáculo cercano, y `--distance-mm 65535` simula "sin lectura".

## Primera prueba con el carro (ruedas al aire)

1. Pon el carro sobre un soporte con las ruedas sin tocar el piso.
2. Baja la **Potencia máxima** al 30 % en el panel *AJUSTES*.
3. **Centro del servo:** sin girar, mueve el **Trim del servo** hasta que las ruedas delanteras queden rectas. El trim se envía con cada comando; cuando encuentres el valor, cópialo en `SERVO_CENTER_DEG` (90 + trim) para que quede fijo.
4. **Ángulo máximo:** gira a fondo a cada lado. Si el servo zumba o el mecanismo llega a su tope, baja `SERVO_MAX_STEER_DEG` en `DriveConfig.h`. Si gira al revés, cambia `SERVO_INVERTED`.
5. **Motores:** acelera. Si un motor gira al revés, cambia `LEFT_MOTOR_INVERTED` o `RIGHT_MOTOR_INVERTED`.
6. **Failsafe:** con el carro acelerando, presiona *Desconectar* o cierra Unity: los motores deben detenerse y el servo centrarse en menos de 300 ms.
7. **Gemelo:** mide la distancia entre ejes y el ancho de vía del carro y ponlos en el componente `VehicleTwin` (objeto `CarTwin`, en Play desde el Inspector; los valores por defecto son 0,12 m y 0,10 m).

## Controles

| Control | Acción |
|---|---|
| `W` / `S` o flechas | Acelerar / reversa |
| `A` / `D` o flechas | Girar |
| `Espacio` | Freno activo |
| Gamepad: gatillo derecho / izquierdo o stick izquierdo | Acelerar / reversa y girar |
| Gamepad: botón sur (A / X) | Freno |
| `R` o Select | Reiniciar la posición del gemelo |
| `C` | Cambiar entre cámara de persecución y cenital |

## Protocolo

UDP entre Unity y el ESP32, que funciona como Access Point (`192.168.4.1:4210`). Enteros multibyte en little-endian. Cada paquete lleva un byte mágico, el tipo, una longitud fija y un **CRC-8/SMBUS** (polinomio `0x07`). El receptor descarta el paquete si alguno no coincide.

| Paquete | Bytes | Campos |
|---|---|---|
| Comando (Unity → carro, 50 Hz) | 9 | `AD 01` · seq (2) · acelerador · giro · trim (°) · flags (bit 0 = freno) · CRC |
| Telemetría (carro → Unity, 10 Hz) | 17 | `AD 81` · ackSeq (2) · acelerador aplicado · giro aplicado · modo · ángulo máx. · ackAgeMs · uptime ms (4) · distancia mm (2) · flags (bit 0 = failsafe) · CRC |

- **Distancia:** `0xFFFF` significa "sin lectura" (sin eco, o fuera de 20 a 4000 mm). Unity la muestra en el panel *CONDUCCIÓN* y la pinta de amarillo a 200 mm o menos.
- **Datagramas completos:** UDP entrega cada paquete entero o no lo entrega, así que no hace falta delimitar tramas ni hacer *byte stuffing*. La integridad la dan el byte mágico, la longitud y el CRC.
- **Orden:** el carro ignora comandos con `seq` más viejo que el último aceptado (comparación de 16 bits con desborde, RFC 1982), así un paquete atrasado no revierte una orden más nueva.
- **Failsafe:** si pasan 300 ms sin un comando válido, el carro detiene los motores y centra el servo.
- **Latencia:** Unity guarda la hora de envío de cada `seq`. Cuando llega la telemetría con `ackSeq`, el RTT es `llegada − envío − ackAgeMs`; así se descuenta el tiempo que la telemetría esperó en el carro antes de salir.
- **PWM:** acelerador y giro van de −100 a 100. El 100 % equivale a un ciclo de trabajo de 67,5 % (Ec. 11 del informe: 5 V / 7,4 V), con una rampa de 250 ms de 0 a 100 % para limitar la corriente de arranque.

## Cambiar el servo por dos N20 (giro diferencial)

1. En `DriveConfig.h`: `STEERING_MODE = SteeringMode::DIFFERENTIAL`.
2. Carga el firmware. El servo deja de usarse y el giro sale de la diferencia de velocidad entre las ruedas: izquierda = acelerador + giro, derecha = acelerador − giro.
3. Unity no necesita cambios. La telemetría informa el modo y el gemelo cambia solo al modelo diferencial. Para la simulación sin conexión, cambia *Offline Steering Mode* en `VehicleTwin`.

## Modelo del gemelo

- **Servo (bicicleta):** ω = v · tan(δ) / L, con δ = giro × ángulo máximo y L = distancia entre ejes.
- **Diferencial:** v = (vᵢ + v_d) / 2 y ω = (vᵢ − v_d) / ancho de vía.
- **Velocidad máxima:** v = 1,257 m/s (Ec. 18 del informe, motor de 500 RPM y rueda de 2,4 cm), escalada por el acelerador aplicado.
- **Respuesta:** el motor y el servo se modelan con un retardo de primer orden (τ = 0,15 s y 0,08 s, ajustables en el Inspector).
- La pose se integra desde el centro del eje trasero, que es el punto de referencia de ambos modelos.

## Informe del Primer Corte (Entregable 1)

Estas son las peticiones del **primer corte**: el informe del vehículo (`INFORME_CARRO_D_C_.pdf`). No son las del **segundo corte**, que es el laboratorio *Experiencia Físico-Digital* (protocolos seriales Arduino–Unity, informe de máximo 8 páginas). Esas están en `../CLAUDE.md`, `../README.md` y `../Lab/`.

**Objetivo:** documentar y sustentar técnicamente el diseño, el cálculo y la construcción del vehículo. No es una bitácora descriptiva: se exige justificación cuantitativa de las decisiones de diseño y validación experimental de los cálculos frente al sistema construido.

**Naturaleza parcial:** es la primera etapa del reporte de desarrollo y **no incluye conclusiones**. El proyecto sigue en el Corte 2 con la integración del vehículo a Unity por transmisión serial. El documento se consolida como reporte único al final del Corte 2, y ahí se redactan las conclusiones del desarrollo completo.

**Estructura (IEEE, dos columnas):**

1. Resumen (Abstract) y palabras clave
2. Introducción
3. Marco teórico (dinámica de motores DC, torque, relaciones de transmisión, funcionamiento del puente H)
4. Diseño y metodología
5. Resultados (mediciones del sistema construido y comparación contra los valores calculados mediante errores absolutos y relativos)
6. Referencias en formato IEEE

**Cálculos obligatorios:**

- Torque requerido para el desplazamiento (masa, radio de rueda y condiciones de rodadura), contrastado contra el torque del motor seleccionado.
- Velocidad angular del motor y velocidad lineal resultante del vehículo.
- Corriente demandada por los motores en arranque y en régimen, contrastada explícitamente contra la corriente máxima soportada por el puente H.
- Medición experimental de **al menos dos** de las magnitudes anteriores sobre el vehículo construido, con comparación explícita frente al valor calculado y análisis de la discrepancia.

**Formato y entrega:** plantilla IEEE de doble columna, extensión sugerida de 4 a 6 páginas, PDF por Moodle.

**Rúbrica (100 puntos):**

| Criterio | Peso | 0 puntos | Nivel bajo | Nivel medio | Nivel máximo |
|---|---:|---|---|---|---|
| Rigor y corrección de los cálculos | 35 | Cálculos ausentes, incorrectos o solo el resultado final sin procedimiento | **14**: falta algún cálculo obligatorio o hay errores conceptuales que afectan la validez | **21**: presentes y en su mayoría correctos, con errores menores aislados (unidad o redondeo) que no invalidan la conclusión | **35**: todos presentes, fórmulas bien aplicadas, unidades consistentes y resultados numéricamente correctos |
| Validación experimental | 20 | Ninguna medición del sistema construido | **8**: mediciones sin comparación explícita contra lo calculado, o comparación sin evidencia numérica | **12**: se comparan, pero el análisis de discrepancias es superficial o incompleto | **20**: mediciones reales del vehículo, comparación explícita y discrepancias justificadas técnicamente (fricción, tolerancias, pérdidas no modeladas) |
| Cumplimiento del formato IEEE | 15 | No sigue IEEE | **6**: estructura similar con desviaciones notorias (columna única, secciones fuera de orden) | **9**: plantilla IEEE con desviaciones menores que no afectan la legibilidad | **15**: plantilla estricta de doble columna (tipografía, numeración de ecuaciones y figuras, secciones) sin desviaciones |
| Redacción técnica y estructura | 15 | Dificulta la comprensión o carece de estructura argumentativa | **6**: comprensible, pero con problemas recurrentes de claridad, terminología imprecisa o estructura poco lógica | **9**: clara en general, con argumentación débil o transiciones abruptas ocasionales | **15**: clara, lenguaje técnico apropiado, argumentación lógica entre secciones, sin ambigüedad |
| Referencias y citación | 15 | Sin referencias o no verificables | **6**: formato IEEE inconsistente o varias afirmaciones técnicas sin respaldo | **9**: formato IEEE con errores menores (numeración, orden); la mayoría de las afirmaciones respaldadas | **15**: todas las fuentes citadas y en formato IEEE correcto; toda afirmación técnica no trivial respaldada |
