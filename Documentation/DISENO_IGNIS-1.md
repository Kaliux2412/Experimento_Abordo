# Diseño electrónico IGNIS-1 (rediseño desde cero)

> Proyecto KiCad nuevo y separado: `KiCad/IGNIS-1/`. NO continúa `KiCad/Project/Schema_Experimento_Abordo`.
> Fuentes: `Informe_Alineacion_IGNIS-1.md` (requisitos/entregables) + `AUDITORIA_HARDWARE.md` (hallazgos/errores a no repetir).
> Inicio: 2026-06-23. KiCad 10.0.4 (formato sch 20260306, lib 20251024).

## Decisiones cerradas (usuario, 2026-06-23)
| # | Tema | Decisión | Implicación |
|---|---|---|---|
| D1 | Sensor de gas | **MQ-135** (spec de misión) | Analógico → ADC1; calentador ~150 mA a **5 V**. Obliga riel +5V. |
| D2 | Material particulado | **REMOVIDO** (2026-06-23) | Experimento auto-propuesto (no exigido por LASC) → PM no obligatorio. Quitado por dato pobre en caída libre + complejidad I²C-5V + stack/masa. PMS5003/HM3301 descartados. |
| D3 | ESP32 | **DevKit 38p completo (USB-C)** | Programación por USB-C (sin FTDI/USB-UART/boot extra). Alimentar por pin `3V3` (NO VIN). Footprint custom 2×20 (40 pines). Riesgo masa/volumen. |
| D4 | MLX90614 | **Eliminado** | No está en spec; ahorra masa/área. |
| D5 | Buzzer | **Incluido** | Localización post-aterrizaje. GPIO + driver. |
| D6 | Nº de placas | **3 placas apiladas** (2026-06-23) | Cerebro (power+mcu) / Sensores / RF (comms_storage) + backplane. Hojas jerárquicas mapean 1:1 a placas. |
| — | Energía | **LiPo 1S → buck-boost 3.3V + boost 5V** | AP2112K LDO descartado (no sirve 1S). Confirmado por AUDITORIA. |

## Variables de misión → sensor (trazabilidad)
| Variable (spec) | Sensor | Bus |
|---|---|---|
| Temperatura / Humedad / Presión | BME280 | I²C |
| Gas de combustión | MQ-135 | ADC1 (+5V heater) |
| ~~Material particulado~~ | — (removido) | experimento auto-propuesto; PM descartado |
| Posición (recuperación, extra) | GPS NEO-6M | UART2 |
| Orientación vertical (extra) | MPU6050 | I²C + INT |
| RSSI + pérdida de paquetes | LoRa RFM95W + firmware (nº secuencia) | SPI |

## Componentes descartados del diseño viejo
SGP30 (VOC interior, débil para combustión) · MLX90614 (no spec) · AP2112K LDO (dropout mata 1S) · MCP1700 1.8V (riel huérfano) · TXS0102 shifter (bus 3.3V puro) · riel +1V8 · PCB viejo (desincronizado).

## Componentes reutilizados (justificados)
ESP32 (forma DevKit) · BME280 (cubre 3 variables) · LoRa RFM95W · microSD · GPS NEO-6M · MPU6050. Símbolos custom reutilizables: GPS `ZC323200`, headers.

## Estructura del proyecto KiCad
```
KiCad/IGNIS-1/
├── IGNIS-1.kicad_pro
├── IGNIS-1.kicad_sch              root jerárquico (5 sheet symbols)
├── sheets/
│   ├── power.kicad_sch            ✅ POBLADA (ver abajo)
│   ├── mcu.kicad_sch              ✅ POBLADA (ver abajo)
│   ├── sensors.kicad_sch          ✅ POBLADA (ver abajo)
│   ├── comms_storage.kicad_sch    ✅ POBLADA (ver abajo)
│   └── connectors.kicad_sch       vacía — backplane inter-placa (pendiente)
├── libs/IGNIS1_Symbols.kicad_sym  símbolos custom
├── footprints/IGNIS1.pretty/      footprints custom (ESP32_DevKit_38p, LoRa_RFM95_adapter)
├── sym-lib-table (nick "IGNIS1")  /  fp-lib-table (IGNIS1 + ZC323200 reuso)
```
Convención de nets: `+BATT +5V +3V3 GND SDA SCL SCK MOSI MISO CS_SD NSS_LORA DIO0 LORA_RST GPS_TX GPS_RX MQ_AOUT MPU_INT BUZZER`.

## Arquitectura física: 3 placas apiladas (Ø62)
| Placa | Hoja(s) | Contenido |
|---|---|---|
| **Cerebro** | power + mcu | ESP32 DevKit, **buck-boost 3V3 (U2)**, protección 1S, batería, RBF, switch, LEDs, buzzer |
| **Sensores** | sensors | BME280, MPU6050, MQ-135, GPS, **boost 5V (U3) local + C4** |
| **RF** | comms_storage | LoRa RFM95W (SMA), microSD |

### 3 proyectos KiCad para fabricación (`boards/`)
Divididos desde el diseño jerárquico (2026-06-23). Cada placa = proyecto independiente (sch + lib-tables propias → PCB):
```
KiCad/IGNIS-1/boards/
├── cerebro/cerebro.kicad_pro    (power + mcu + J40 backplane) — 22 comp.
├── sensores/sensores.kicad_pro  (sensores + J41 backplane)    — 11 comp.
└── rf/rf.kicad_pro              (comms_storage + J42 backplane) — 6 comp.
```
Comparten `libs/IGNIS1_Symbols.kicad_sym` y `footprints/IGNIS1.pretty` (rutas absolutas en lib-tables). El proyecto jerárquico `IGNIS-1.kicad_sch` queda como **referencia integrada** (ERC del sistema completo). **ERC por placa: 0/0/0.**

### Backplane de apilado (conector 2×10, J40/J41/J42, pinout idéntico)
| Pin | Net | Pin | Net |
|---|---|---|---|
| 1 | +3V3 | 11 | GND |
| 2 | GND | 12 | SCK |
| 3 | **SPARE** (+5V eliminado) | 13 | MOSI |
| 4 | GND | 14 | MISO |
| 5 | SDA | 15 | CS_SD |
| 6 | SCL | 16 | NSS_LORA |
| 7 | MPU_INT | 17 | DIO0 |
| 8 | GPS_TX | 18 | LORA_RST |
| 9 | GPS_RX | 19 | SPARE1 |
| 10 | MQ_AOUT | 20 | SPARE2 |

Cerebro = hub (todos los pines). Sensores usa 1-11 (resto NC). RF usa 1,2,4,11-18 (resto NC). En placas receptoras (Sensores/RF) los rieles +3V3/+5V/GND entran por backplane con PWR_FLAG. Footprint conector: `PinHeader_2x10_P2.54mm_Vertical` (provisional — definir tipo de apilado: hembra/macho, altura, retención anti-vibración).

> **Cuidado:** los 3 proyectos de placa son ahora independientes; un cambio de netlist debe re-propagarse (regenerar con el script o editar las 3 + el jerárquico). El generador `scratchpad/gen.py` rehace todo de forma consistente.

## Plantilla mecánica común (3 placas, 2026-06-23)
Idéntica en cerebro/sensores/rf para que el stack apile alineado:
- **Centro placa: (100,100) mm. Contorno Ø62** (Edge.Cuts, radio 31).
- **4 agujeros M2.5** (`MountingHole:MountingHole_2.7mm_M2.5`, PCB-only) en bolt-circle Ø54 (r=27), diagonales: H1(119.1,119.1) H2(80.9,119.1) H3(80.9,80.9) H4(119.1,80.9).
- **Backplane** J40/J41/J42 en **(79.46, 88.84) rot 0**.
- Update PCB: dejar **"Delete extra footprints" OFF** (agujeros sin símbolo).
- Cerebro (denso por el ESP): correr C10/U2/U3/J1 fuera de los agujeros.

## Footprints (asignados 2026-06-23)
| Componente | Footprint | Estado |
|---|---|---|
| Pasivos R/LED/D/Q | SMD (0805, SOT-23, SOD-123) | ✅ |
| Caps 100nF / 22µF | C_0805 | ✅ |
| **Caps bulk 100µF/220µF** | **C_1210 (MLCC SMD)** | ✅ (2026-06-23: THT→SMD; MLCC cerámico, evita tantalio cerca de LiPo; considerar DC-bias derating) |
| BME280, MQ-135 | PinHeader_1x04 | ✅ (⚠️ verificar silk) |
| MPU6050 / microSD | PinHeader_1x08 / 1x06 | ✅ |
| GPS NEO-6M | ZC323200:XCVR_ZC323200 (reuso) | ✅ |
| SMA / Buzzer | SMA_Amphenol_901-144_Vertical / Buzzer_12x9.5 | ✅ (verificar modelo) |
| J1/J2/SW1 | PinHeader_1x02 | ✅ provisional |
| **ESP32 DevKit 40p (U10)** | IGNIS1:ESP32_DevKit_40p (2×20) | ✅ filas 22.86mm CONFIRMADO (PDF Freenove) |
| **LoRa adapter (U30)** | IGNIS1:LoRa_RFM95_adapter | ⚠️ **2×8 nominal — medir pitch/row + orden silk P1/P2** |
| **U1 protección, U2/U3 DCDC** | PinHeader_1x04 (provisional) | ⏳ placeholder — cambiar al footprint real (módulo buck-boost/boost, protección 1S) |

## Hoja de alimentación (`sheets/power.kicad_sch`) — implementada
Árbol verificado por netlist:
```
J1(LiPo 1S) ─+BATT─ U1 BattProtect_1S ─VSYS─ J2(RBF) ─VRBF─ SW1(main) ─VSW─┬─ U2 BuckBoost → +3V3 ─ C2,C3 (100µF)
                                                                          └─ U3 Boost     → +5V  ─ C4 (220µF)
                                                                          C1 (22µF) bulk en VSW
EN de U2/U3 atados a VSW (siempre habilitados). PWR_FLAG en +BATT/GND/VSW.
```
- **U1 BattProtect_1S**: símbolo genérico de protección/load-switch 1S (UVLO+OCP). Parte real por definir (celda protegida vs IC tipo load-switch).
- **U2/U3 DCDC_Module**: símbolos genéricos VIN/GND/EN/VOUT. U2 = buck-boost 3.3V (clase TPS63020/63060). U3 = boost 5V.
- **VSW**: riel conmutado tras RBF+switch; entrada de ambos convertidores.
- **Bulk distribuido (regla: cerca de la carga, en su placa):** C2 100µF (salida buck-boost) + C10 100µF (ESP) en Cerebro; **C32 100µF en RF junto al LoRa**. (C3 100µF eliminado de Cerebro 2026-06-23: era bulk de LoRa varado, inútil cruzando el backplane.)
- **Footprints**: caps `CP_Radial_D6.3mm_P2.50mm`, headers `PinHeader_1x02` — provisionales.
- **ERC: 0 errores / 0 warnings.** (validado con `kicad-cli sch erc`).

## Hoja MCU (`sheets/mcu.kicad_sch`) — implementada
**U10 = Freenove ESP32-WROOM-32E DevKit 40p** (símbolo custom `ESP32_DevKit_40`, 2×20, pinout oficial Freenove). Alimentado por pin `3V3` (NO VIN/5V). Programación USB-C onboard.

Pin-map (consistente con `Firmware/Firmware.ino`):
| Señal | GPIO | Señal | GPIO |
|---|---|---|---|
| SDA | 33 | SCK | 18 |
| SCL | 32 | MISO | 19 |
| NSS_LoRa | 5 | MOSI | 23 |
| LoRa RST | 14 | CS microSD | 13 |
| DIO0 | 26 | MPU_INT | 27 |
| Buzzer | 25 | LED status | 4 |
| GPS_TX (ESP RX2) | 16 | GPS_RX (ESP TX2) | 17 |
| MQ-135 AOUT (ADC1) | 35 | (GPIO21/22 libres) | — |

- **Buses inter-hoja = etiquetas GLOBALES** (SDA, SCL, SCK, MOSI, MISO, CS_SD, NSS_LORA, DIO0, LORA_RST, GPS_TX/RX, MQ_AOUT, MPU_INT). Conectan al poblar sensors/comms.
- **No-connect** en flash (GPIO6-11), strapping (0/2/12/15), TX0/RX0 (USB), 5V, EN, spares (34/36/39, **21/22 libres**).
- Periféricos de placa: C10 100µF + C11 100nF (desacoplo 3V3); D2+R10 (LED power, siempre on); D3+R11 (LED status en GPIO4); driver buzzer Q1(MMBT3904)+R12(1k base)+BZ1+D4(flyback) en GPIO25.
- **ERC: 0 errores.** 15 warnings = etiquetas globales que aún tocan un solo pin (cierran al poblar sensors/comms).

> **Nota firmware:** GPIO35 (MQ_AOUT, input-only ADC1) NO está aún en el firmware esqueleto — agregar lectura ADC del MQ-135. Sin sensor PM. GPIO21/22 libres.

## Hoja Sensores (`sheets/sensors.kicad_sch`) — implementada
| Ref | Componente | Bus / pin | Alimentación |
|---|---|---|---|
| U20 | BME280 (T/H/P) | I²C (SDA/SCL) | +3V3 |
| U21 | MPU6050 (IMU) | I²C + INT→MPU_INT; AD0→GND (0x68) | +3V3 |
| U22 | MQ-135 (gas) | A0→divisor→MQ_AOUT(GPIO35); D0 NC | **+5V** (heater) |
| U24 | GPS NEO-6M | UART2 cruzado (TX→GPS_TX, RX→GPS_RX) | **+5V** |
| R20/R21 | Pull-ups I²C 4.7k | SDA/SCL → +3V3 | — |
| R22/R23 | Divisor MQ-135 10k/15k | A0(0–5V) → ~3.0V | — |
| C20 | Filtro ADC 100nF | MQ_AOUT → GND | — |

- **I²C compartido** BME280 (0x76) + MPU6050 (0x68) con **un solo par de pull-ups** (módulos traen los suyos; al integrar dejar uno). Sin colisión.
- **MQ-135 A0 (0–5V) → divisor 10k/15k → ~3.0V** antes del ADC (GPIO35 input-only). Protege la entrada del ESP.
- **MQ-135 y GPS alimentados a +5V.** GPS a 5V por recomendación de auditoría (LDO onboard marginal a 3.3V); lógica UART sigue a 3.3V.
- MPU6050 XDA/XCL = NC; MQ-135 D0 = NC.
- Símbolos: BME280 = módulo custom nuevo; MQ-135/MPU6050/GPS = reusados del lib viejo (GND saneado `power_out`→`power_in`). GPS footprint `ZC323200` pendiente de re-vincular.
- **ERC: 0 errores.** Quedan 7 warnings = buses SPI/LoRa (SCK/MOSI/MISO/CS_SD/NSS_LORA/DIO0/LORA_RST) que cierran al poblar `comms_storage`.

## Hoja Comunicación + Almacenamiento (`sheets/comms_storage.kicad_sch`) — implementada
| Ref | Componente | Conexión |
|---|---|---|
| U30 | LoRa RFM95W (SX1276, 433 MHz) | SPI; NSS_LORA, LORA_RST, DIO0; ANT→SMA; +3V3 |
| U31 | microSD | SPI; CS_SD; +3V3 |
| J30 | SMA antena | señal→ANT, shield→GND |
| C30/C31 | Desacoplo 100nF | LoRa / microSD |
| C32 | **Bulk 100µF** | +3V3 junto al LoRa (U30.12) — reserva para picos de TX |

- **SPI compartido** ESP↔LoRa↔microSD: SCK/MOSI/MISO comunes; **MISO tri-state** en ambos esclavos (sin conflicto de salidas). **CS dedicados**: NSS_LORA (GPIO5) y CS_SD (GPIO13).
- LoRa: DIO0→GPIO26, RESET→GPIO14, DIO1-5 = NC, ANT→SMA. Bulk 100µF de LoRa = C3 (hoja power).
- Símbolos reusados del lib viejo: `LoRa_RFM65W` (= RFM95W confirmado por auditoría) y `Micro_SD`.

## ESTADO: esquemático de cadena de señal COMPLETO
4 de 5 hojas pobladas (power, mcu, sensors, comms_storage). **ERC global: 0 errores, 0 warnings.** Toda la electrónica de vuelo (alimentación + MCU + sensores + radio + almacenamiento) está conectada y verificada. Falta solo `connectors` (backplane), que depende de la decisión 1 vs 3 placas.

## Pendientes (no bloquean seguir con el esquemático)
2. Parte real de U1 (protección 1S) y de U2/U3 (módulo o IC + inductor/caps).
3. Medidas footprint custom: DevKit 38p (separación filas, pitch), LoRa verde.
4. Decidir 1 vs 3 placas tras presupuesto de masa (300 g).
5. Poblar hojas mcu / sensors / comms_storage / connectors.

## Cómo validar
```
cd KiCad/IGNIS-1
kicad-cli sch erc IGNIS-1.kicad_sch -o /tmp/erc.rpt
kicad-cli sch export pdf IGNIS-1.kicad_sch -o /tmp/IGNIS-1.pdf
```
