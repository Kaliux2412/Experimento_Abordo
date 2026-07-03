# Procedimiento de bring-up — IGNIS-1 (protoboard)

> Guía paso a paso para verificar el hardware de IGNIS-1 en protoboard, con lo que el
> equipo tiene a la fecha. *Bring-up* = encender y validar cada subsistema **uno por uno**
> antes de juntarlos. Si algo falla, lo aíslas rápido en vez de depurar todo el sistema a
> ciegas.
>
> Firmware de prueba: `Firmware/Test_Protoboard/Test_Protoboard.ino` (NO es el firmware de
> vuelo; solo imprime PASS/FAIL por subsistema y se activa/desactiva por flags).

---

## 0. Inventario

### Disponible
ESP32 DevKit · BME280 · MPU6050 · MQ-135 · GPS NEO-6M (GY-GPS6MV2) · buzzer activo ·
2× 2N3904 · 4× 1N4148 · LiPo 1S 1000mAh · TP4056 (cargador) · **TPS63020** (buck-boost 3V3,
**salida fija**) · **MT3608** (boost 5V, **ajustable**) · resistencias/caps/LEDs/cables del
kit · protoboards.

### Falta (no testeable aún)
Módulo **microSD** · módulo **LoRa RFM95W**. Por eso `TEST_SD` y `TEST_LORA` van en `0`.

### Herramientas
Cautín ~370°C (estaño sin plomo 0.6mm SAC0307) · lana de bronce · **multímetro
(imprescindible para el paso 4)**.

---

## Pin-map de referencia (ESP32 DevKit)
| Señal | GPIO | Señal | GPIO |
|---|---|---|---|
| I²C SDA | 33 | SPI SCK | 18 |
| I²C SCL | 32 | SPI MISO | 19 |
| Buzzer | 25 | SPI MOSI | 23 |
| LED status | 4 | GPS RX2 (← GPS TX) | 16 |
| MQ-135 AOUT (ADC1) | 35 | GPS TX2 (→ GPS RX) | 17 |
| MPU_INT | 27 | — | — |

---

## ⚠️ Reglas de seguridad (leer antes de conectar nada)
1. **El ESP32 es 3.3V.** Ningún pin GPIO tolera 5V. El MQ-135 entrega 0–5V → **siempre**
   pasa por el divisor 10k/15k antes de GPIO35. Sin divisor, quemas la entrada.
2. **Mide los reguladores SIN carga antes de conectarles algo** (paso 4).
3. **GND común:** todos los módulos, reguladores, batería y ESP comparten el mismo GND.
   Sin GND común, nada funciona o da lecturas erráticas.
4. Alimentación: ESP y sensores I²C a **3.3V**; MQ-135 y GPS a **5V**.

---

## Paso 1 — ESP32 solo, por USB

**Objetivo:** confirmar placa + toolchain. Sin reguladores todavía.

1. Conecta solo el ESP32 por USB-C.
2. Arduino IDE → instala librerías (Library Manager): **Adafruit BME280**, **Adafruit
   MPU6050**, **Adafruit Unified Sensor**, **TinyGPSPlus**.
3. Abre `Test_Protoboard.ino`. Verifica que `TEST_SD` y `TEST_LORA` están en `0`.
4. Sube. Abre Serial Monitor a **115200 baud**.

**Esperado (PASS):**
- Beep corto del buzzer al boot (si ya lo conectaste; si no, ignora).
- LED en GPIO4 parpadea (latido).
- Serial imprime cabecera + `[I2C] scan:` (sin sensores aún → 0 dispositivos, normal).

**Si falla:** revisa puerto/driver USB, baud rate, que la placa seleccionada sea el ESP32
correcto.

---

## Paso 2 — Buzzer

**Objetivo:** validar el driver del buzzer (réplica THT del que va en el PCB).

Montaje:
```
GPIO25 ──[R 1k]── base (B) del 2N3904
                  colector (C) ── buzzer(−)
                  buzzer(+) ── +5V (o 3V3 si aún no hay 5V)
                  emisor (E) ── GND
1N4148 en paralelo al buzzer: cátodo (raya) → buzzer(+), ánodo → buzzer(−)  [flyback]
```
2N3904 visto de frente (cara plana al frente, patas abajo): **E – B – C** de izq. a der.

**Esperado:** beep al boot del ESP. Si quieres forzarlo, en `setup()` ya hay
`digitalWrite(BUZZER,HIGH)`.

**Si falla:** revisa orientación del transistor (E/B/C), el flyback (polaridad del diodo),
y que la base lleve la resistencia de 1k.

---

## Paso 3 — Bus I²C (BME280 + MPU6050)

**Objetivo:** validar el bus de sensores y leer datos.

Montaje (ambos sensores en paralelo, alimentados del pin **3V3** del ESP):
```
ESP 3V3 ── VCC de BME280 y MPU6050
ESP GND ── GND de ambos
GPIO33 (SDA) ── SDA de ambos
GPIO32 (SCL) ── SCL de ambos
MPU6050 AD0 ── GND   (fija dirección 0x68)
```
Los módulos ya traen pull-ups → no agregues más.

**Esperado (PASS):**
- `[I2C] scan:` lista **0x76 (BME280)** y **0x68 (MPU6050)**.
- `[BME280] PASS` y `[MPU6050] PASS`.
- **Doble beep** (sensores I²C OK).
- En loop: `BME280: T=.. RH=.. P=..` y `MPU6050: ax/ay/az/gz`.

**Si falla:** revisa SDA/SCL no cruzados, GND común, alimentación 3V3. Si el scan no
muestra nada, casi siempre es GND o pull-ups.

---

## Paso 4 — Ajustar/medir reguladores ⚠️ (sin carga, con multímetro)

**Objetivo:** dejar los rieles en su voltaje correcto ANTES de conectarles cargas.

> **TPS63020 = salida FIJA 3.3V, no tiene pote.** Solo se mide para confirmar.
> **MT3608 = ajustable, tiene pote multivuelta.** Hay que ajustarlo.

1. **TPS63020 (riel 3.3V):**
   - Alimenta VIN desde la batería (o fuente 3.0–4.2V). GND común.
   - Mide VOUT con multímetro **sin nada conectado**: debe leer **~3.3V**.
   - Confirmado → este riel alimenta el ESP (pin 3V3) y los sensores I²C.
2. **MT3608 (riel 5V):**
   - Alimenta VIN desde la batería. GND común.
   - Mide VOUT y **gira el pote** hasta **5.0V exactos**, sin carga.
   - Confirmado → este riel alimenta MQ-135 y GPS.

**Regla:** conectar la carga **solo después** de leer el voltaje correcto. Conectar el ESP
a un riel sin medir = arriesgar el chip.

---

## Paso 5 — MQ-135 (gas)

**Objetivo:** leer el sensor de gas por el ADC, protegiendo la entrada.

Montaje:
```
MQ-135 VCC ── +5V (MT3608 ya ajustado)
MQ-135 GND ── GND
MQ-135 A0  ──[10k]──┬──[15k]── GND
                    └── GPIO35   (el nodo del divisor; ~3V máx)
(opcional) 100nF del nodo a GND = filtro de ruido
MQ-135 D0  ── sin conectar
```
El divisor baja A0 (0–5V) a ~0–3V seguros para el ADC. **Obligatorio.**

**Esperado:** `[MQ-135] ADC listo`. En loop: `MQ-135: ADC=.. mV A0~=.. V`. El heater
calienta (se siente tibio); los valores **se estabilizan tras varios minutos** de warmup.
Acerca alcohol/encendedor (sin llama) → el valor debe moverse.

**Si falla:** revisa el divisor (sin él la lectura satura o dañas el pin), 5V real en VCC,
y que uses GPIO35 (input-only, ADC1).

---

## Paso 6 — GPS NEO-6M

**Objetivo:** confirmar recepción NMEA y, con suerte, fix de posición.

Montaje (UART **cruzado**, GPS a 5V):
```
GPS VCC ── +5V        GPS GND ── GND
GPS TX  ── GPIO16 (ESP RX2)
GPS RX  ── GPIO17 (ESP TX2)
```

**Esperado:**
- `[GPS] UART2 abierto`. En loop, al principio: `GPS: sin fix (chars=.., sats=..)`.
- **`chars` subiendo = el GPS habla y el ESP lo escucha** (UART OK), aunque no haya fix.
- Cerca de ventana o en exterior, tras 1–5 min: `GPS: lat, lng sats=.. alt=..`.

**Si falla:** si `chars=0`, el UART está mal → revisa TX/RX cruzados y GND. El fix necesita
cielo despejado; en interior puede no llegar nunca (eso no es falla del cableado).

---

## Paso 7 — Integración energía + MCU

**Objetivo:** correr todo desde batería, como en vuelo.

1. **Carga la batería con el TP4056** (USB-micro in; B+/B− a la celda). LED rojo =
   cargando, azul/verde = lleno. *No* alimentes el resto desde el TP4056; es solo cargador.
2. Batería → **TPS63020** (3.3V) → pin **3V3** del ESP y sensores I²C.
3. Batería → **MT3608** (5V) → MQ-135 y GPS.
4. Caps de bulk: 220µF en el riel 5V, 100µF/22µF en el 3.3V (cerca de las cargas).
5. Corre el sketch completo (BME+MPU+MQ+GPS+buzzer) **sin USB**, solo batería.
6. Mide consumo total con el multímetro en serie (opcional, útil para el presupuesto de
   energía de la misión).

**Esperado:** mismo comportamiento que con USB, ahora autónomo. Riel 3.3V estable aunque la
batería baje (el TPS63020 lo mantiene de 3.0 a 4.2V de entrada).

---

## Pruebas que quedan pendientes (falta hardware)
- **microSD** (`TEST_SD`): cuando llegue el módulo, SPI SCK18/MOSI23/MISO19, CS=13.
- **LoRa RFM95W** (`TEST_LORA`): SPI compartido, NSS=5, RST=14, DIO0=26. Mientras tanto, el
  envío solo se verifica por prints en Serial o con analizador lógico en SCK/MOSI/NSS.

---

## Tips de uso del sketch
- Para probar **un solo** subsistema, pon los demás `TEST_*` en `0` y vuelve a subir.
- Doble beep al boot = BME280 + MPU6050 OK.
- Todo a 115200 baud en el Serial Monitor.

---

## Bitácora de bring-up

Registro cronológico de lo hecho en protoboard real, hallazgos y fixes. El procedimiento de
arriba es la guía; esto es el récord de qué pasó de verdad.

### 2026-07-01/02 — Primera sesión completa

**Sensores probados (Pasos 1-6):** BME280 ✓, MPU6050 ✓, MQ-135 ✓ (responde, warmup normal),
GPS NEO-6M ✓ recibe NMEA (UART OK, sin fix por falta de antena — esperado en interior).

**Bug de pinout ESP32 corregido.** Símbolo `ESP32_DevKit_40` en KiCad tenía un pin de más
corrido (le faltaba un 3V3, el resto de la fila se recorría). Pin18 decía "5V" (debía ser
3V3) y pin20 decía "GND" (debía ser 5V) — y el esquemático amarraba GND justo a ese pin20,
es decir tenía un GND cableado sobre lo que físicamente es el pin 5V del DevKit real. Se
corrigió símbolo + caché del `.kicad_sch` + wiring (se quitó el GND mal puesto, se dejó
no-connect). ERC 0/0 después del fix. Verificado contra pinout oficial Freenove.
Detalle técnico completo: sección 8 de `Guia_Educativa_Diseno_PCB.md`.

**Calibración de MPU6050 agregada al sketch.** `az` leía 13.1 m/s² en reposo (debía ser
9.81 — offset de fábrica sin calibrar). Se agregó rutina `calibrarMPU()` que promedia 1000
muestras al boot (placa quieta) y resta el bias de los 3 ejes de accel y gyro. Resultado
tras fix: az≈9.81, gx/gy/gz≈0.000. **La placa debe estar plana y quieta al arrancar** — la
calibración se recalcula cada boot.

**Investigación de carga de batería (TP4056).** Dejamos la batería cargando toda la noche
(ESP32 conectado y corriendo el sketch, alimentado por el mismo circuito) para que el
MQ-135 hiciera warmup largo. LED del TP4056 se quedó en rojo (cargando) sin cambiar en 8+
horas.

- *Diagnóstico:* `chars` del GPS (contador acumulado desde boot) daba solo ~90 min de
  uptime, no 8h — el ESP se había reseteado en algún punto de la noche. Indicio de
  brownout (caída de voltaje) mientras cargaba.
- *Causa raíz confirmada:* el ESP32 estaba alimentado desde OUT+ del TP4056 **al mismo
  tiempo** que cargaba. La corriente que consume el ESP se suma a la corriente de carga
  vista por el chip 4056D, así que la corriente total nunca bajaba al umbral de
  "terminado" (~C/10) aunque la batería ya estuviera casi llena — la carga nunca se
  declaraba completa mientras hubiera carga conectada.
- *Prueba:* se desconectó el ESP32 (solo batería + TP4056). El LED cambió de rojo a azul
  **casi instantáneo** — sospechoso de un corte abrupto por desaparición de carga, no de
  una transición orgánica de fin de carga. Se midió voltaje real en B+/B- con multímetro:
  **4.21V** → confirmado, sí estaba genuinamente cargada.
- **Regla operativa (ya estaba en el Paso 7 pero se violó en la práctica):** NUNCA
  alimentar el ESP32/carga desde el TP4056 mientras carga. Cargar la batería sola,
  desconectada de todo consumo, y solo después conectarla al sistema.

**Pendiente — monitoreo de batería (propuesto, no implementado aún):**
- *Voltaje/SOC:* divisor 100k/100k desde `+BATT` (antes del TPS63020, no después — el
  buck-boost siempre da 3.3V fijo sin importar la carga) a GPIO34 (ADC1, libre, input-only).
  4.2V→2.1V, 3.0V→1.5V en el ADC, dentro de rango seguro. Curva LiPo no es lineal — sirve
  más para detectar "batería baja" (umbral) que % preciso sin calibración.
- *Flag digital de "carga completa" real:* tapear el pin que maneja el LED de "done" del
  TP4056 (activo-bajo típico) a un GPIO con pull-up, para leer el estado del chip 4056D
  directo en vez de inferirlo por voltaje. Pendiente confirmar con multímetro cuál pin es
  cuál en el módulo HW-373 específico (no hay datasheet confiable del clon).

---

*IGNIS-1 — equipo Ignitia. Procedimiento de bring-up en protoboard. Reglas de seguridad y
divisor del MQ-135 son obligatorias, no opcionales.*
