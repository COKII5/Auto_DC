# Bitácora del carro Auto DC

Registro de lo hecho en el carro: cambios, pruebas, mediciones, problemas y decisiones. Las entradas van de la más antigua a la más reciente.

## 2026-10-09

### Informe del primer corte
- Se hizo una copia corregida del informe (`Informe/informe_carro.tex`) sin tocar el PDF original (`INFORME_CARRO_D_C_.pdf`).
- Se corrigieron los errores de cálculo del original:
  - El informe mezclaba dos motores. Se unificó en el **N20 de 6 V, relación 1:50, 500 RPM sin carga** (fila de la hoja de datos de la Fig. 6).
  - La conversión de torque de la Ec. 14 tenía un error de ×10 (0,055 kgf·cm en lugar de 5,5×10⁻³).
  - El radio de rueda aparecía con tres valores distintos.
  - La Ec. 11 usaba 5 V en un motor de 6 V; se justificó el 67,5 % de PWM con las celdas cargadas (8,4 V).
- Los números del informe salen de `Informe/compute_values.py`, que los escribe dentro del `.tex`.
- Se agregó la estructura que faltaba (resumen y sección de Resultados) y se dejó listo para Overleaf, con las imágenes en la carpeta `Imagenes/`.

### Sensor de distancia HC-SR04 (frontal)
- Se integró al firmware principal: `Arduino/AutoDcController/DistanceSensor.h`, medición cada 60 ms sin bloquear el `loop()` (el eco se lee con una interrupción).
- Pines: **TRIG en GPIO 18** y **ECHO en GPIO 34** a través de un divisor (1 kΩ en serie y 2 kΩ a GND), porque el eco sale a 5 V y el ESP32 solo tolera 3,3 V.
- La telemetría pasó de 15 a 17 bytes para llevar la distancia en mm (`0xFFFF` = sin lectura). Se actualizaron el firmware, Unity (`CarTelemetry.cs`, `PacketFormat.cs`, `DriveHud.cs`) y `Tools/fake_car.py`.
- Se evaluó usar cuatro sensores (frontal, laterales y trasero). Es viable, pero **se decidió dejar solo el frontal por ahora**.

### Pruebas por partes
Se crearon sketches independientes en `Arduino/Pruebas/`, manejados por el Monitor Serie (115200):

| Prueba | Resultado |
|---|---|
| `TraccionTrasera` | Funciona. Se agregó el comando `c` (giro continuo). |
| `ServoDireccion` | Funciona. |
| `SensorFrontal` | Funciona. Se agregó el comando `d` (diagnóstico del eco). |

### Problemas encontrados y su solución
- **Las ruedas no giraban:** al 40 % de potencia el motor recibe menos de 2 V por la caída del L298N; la prueba aislada del puente H confirmó que el L298N, las baterías y el motor izquierdo estaban bien.
- **Error al depurar en Arduino IDE (`unable to open ftdi device`):** era el botón *Debug*, que necesita un depurador JTAG externo. Se usa **Subir** y el Monitor Serie.
- **El sensor siempre daba "Sin lectura":** faltaba el cable del punto medio del divisor al **GPIO 34 (`D34`)**. El GND del sensor también debe ir a la tierra común.
- **Error de compilación en `SensorFrontal.ino`:** una cadena de texto quedó partida en dos líneas al agregar el diagnóstico. Se corrigió y se compilaron todos los sketches con `arduino-cli`.
- Se probaron los pines 16 y 17 para `IN3`/`IN4`, pero se **volvió a los originales (33 y 32)**.

### Mediciones
| Magnitud | Valor medido |
|---|---|
| Radio de la rueda | 2,4 cm |
| Masa del vehículo | 317 g |
| Celdas sin carga | 7,5 V |
| Celdas con los motores en marcha | 6,9 V |
| Voltaje en cada motor | 5,7 V (falta confirmar en qué condición de PWM) |
| Sensor a 10 / 20 / 25 cm | 14 / 23 / 26 cm |

- Con estos datos se recalculó el informe: velocidad teórica de 1,257 m/s y factor de seguridad de torque de 66.
- El gemelo de Unity (`VehicleTwin.cs`) se actualizó con el radio de 0,024 m y la velocidad máxima de 1,257 m/s.
- Se decidió **no medir la corriente**. La rúbrica pide al menos dos magnitudes entre torque, velocidad y corriente, así que la validación experimental queda corta si solo se mide la velocidad.

## 2026-10-10

- Se comprobó que hay dos ESP32: la DevKit V1 usada (más grande) y otra más pequeña sin modelo identificado. En la de 30 pines el pin de 5 V se llama `VIN`. El código funciona en ambas.
- Se creó `Arduino/Pruebas/PruebaIntegral`: motores, servo y sensor funcionando juntos, con **freno automático por obstáculo a 250 mm** (la reversa siempre está permitida). Reutiliza las clases del firmware principal en lugar de copiarlas. Compila sin errores; falta probarla en el carro.

### Conexión con Unity y repositorio
- Se probó `AutoDcController`: el ESP32 crea la red `AutoDC` y la PC la detecta. El error de Unity "red no accesible" aparece si la PC sigue conectada a otra red Wi-Fi.
- Se revisó el código contra la tabla de penalizaciones y se **eliminaron todos los comentarios** (157 líneas en 37 archivos de Arduino, Unity y Python), sin cambiar ninguna línea de código. Se comprobó que todo compila sin errores ni avisos (los 5 sketches de Arduino con todos los avisos activados y los scripts de Unity) y que no hay `using`, `#include` ni `import` innecesarios.
- Se subió el proyecto completo al repositorio público `COKII5/Auto_DC`, que solo tenía el sketch antiguo para Arduino Uno (`Auto_DC.ino`). Ese archivo se eliminó porque lo reemplaza el firmware del ESP32; sigue disponible en el historial.

## Pendientes
- [ ] Probar `PruebaIntegral` con las ruedas al aire y luego en el piso, y medir a qué distancia se detiene de verdad.
- [ ] Medir la velocidad lineal: 1 m cronometrado, 5 repeticiones.
- [ ] Confirmar en qué condición se midieron los 5,7 V en los motores.
- [ ] Calibrar el sensor a 50 y 100 cm, con la regla desde el frente de los cilindros.
- [ ] Completar en el informe: velocidad medida, falla detectada, modelo del multímetro y apellido del cuarto autor.
- [ ] Definir cómo se alimenta el ESP32 cuando el carro funcione suelto, sin USB.
- [ ] Probar todo junto con `AutoDcController` por Wi-Fi desde Unity.
