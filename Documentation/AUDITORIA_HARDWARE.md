# Auditoría de Hardware — Experimento de Vuelo (Ignitia / Experimento_Abordo)

> Documento de referencia para sesiones de Claude y para el equipo.
> **Última actualización:** 2026-06-22.
> Marca de confianza por ítem: ✅ verificado contra archivo/datasheet · ⚠️ requiere verificación en KiCad · 📄 requiere verificación en datasheet · 🔧 cambio aplicado esta sesión.

## 0. Contexto del proyecto
- Experimento electrónico dentro de un cohete universitario. Sube ~1 km, se desprende el experimento. El circuito va en el experimento.
- Prioridad: diseño **seguro, confiable, revisable y resistente** a errores de conexión, alimentación, vibración, comunicación y ensamblaje.
- Proyecto KiCad **activo**: `Experimento_Abordo/KiCad/Project/Schema_Experimento_Abordo.kicad_sch` + `.kicad_pcb`.
- ⚠️ El repo/carpeta top-level `Schema_Experimento_Abordo/` **NO** es el proyecto activo. No editar ahí.
- Migración Sonnet → Opus por posibles errores técnicos de Sonnet + compacting. No asumir que Sonnet tenía razón.

## 1. ESTADO GLOBAL — lo más importante
1. ✅ **El esquemático y el PCB están DESINCRONIZADOS.** El PCB es una foto vieja. Evidencia dura:
   - `+1V8`: **0 pads** en el PCB (riel huérfano).
   - Labels `SDA_1V8` / `SCL_1V8`: **0** en el PCB.
   - C8/C9 (decoupling LoRa): **sin net** en el PCB (flotantes).
   - SGP30 (U7) en el PCB sigue en config vieja (3.3V + GPIO D32/D33).
   - El cruce GPS RX/TX (🔧 hecho esta sesión) está **solo en el .sch**.
2. ✅ **`ERC.rpt` es del 2026-06-21 22:22 → OBSOLETO.** Regenerar tras reconciliar sch↔PCB.
3. ⚠️ **NO correr "Update PCB from Schematic" todavía**: propagaría la cadena 1.8V huérfana y edits a medias. Primero decidir destino del SGP30 (ver §3).

## 2. Componentes / subsistemas (del esquemático) ✅
| Ref | Componente | Bus | Nota |
|---|---|---|---|
| U1 | ESP32-S3-WROOM-1 | host | VIN ya en +3.3V |
| U2 | BME280 | I2C | bus compartido |
| U3 | MPU6050 | I2C | bus compartido |
| U4 | LoRa — símbolo `LoRa_RFM65W` ⚠️ | SPI | ¿es RFM95W real? confirmar |
| U5 | microSD | SPI | |
| U6 | GPS GY-GPS6MV2 (símbolo ZC323200) | UART | |
| U7 | **SGP30** | I2C | ver §3 (módulo, no chip) |
| U8 | MLX90614 | I2C | bus compartido |
| U9 | AP2112K-3.3 (LDO 3.3V) | power | |
| U10 | MCP1700-1.8 (LDO 1.8V) | power | **huérfano** |
| U11 | TXS0102 (level shifter) | — | **huérfano** |

## 3. SGP30 (U7) — SUBSISTEMA A REDISEÑAR ⚠️ CRÍTICO
### Hardware real comprado
- ✅ Se compró un **MÓDULO breakout SGP30** (tipo CJMCU-30 / GY-SGP30) con **header de pines**, NO el chip DFN-6 desnudo. (Foto del vendedor: PCB con silk "SGP", 4 agujeros de montaje, header.)
- Estos módulos suelen traer **regulador a 1.8V y level shifter I2C onboard**, y se alimentan a **3.3–5V** con I2C a 3.3V.

### Datasheet del chip SGP30-25K (📄✅ verificado, v1.0 May 2020)
- VDD y VDDH: typ **1.8V**, **abs max +2.16V**. → 3.3V **destruye el chip desnudo**.
- Recomiendan **cortocircuitar VDD y VDDH**. Si no: VDD siempre alimentado cuando VDDH lo esté, o se daña.
- I2C **open-drain a 1.8V**, pull-ups ~10k al riel 1.8V. 100nF de desacoplo pegado a VDD. Pin4 "R" → GND. Die pad = GND interno (soldar por mecánica).
- Pinout (Table 4, top view): 1=VDD, 2=VSS, 3=SDA, 4=R, 5=VDDH, 6=SCL. ✅ Coincide con símbolo y footprint custom DFN.

### Estado actual en el PCB (config vieja, para chip desnudo) ✅
| Pad | Función | Net actual | Veredicto |
|---|---|---|---|
| 1 VDD | aliment. | +3.3V | ❌ destructivo SI fuera chip desnudo / OK si módulo |
| 5 VDDH | hotplate | +3.3V | ❌ / OK módulo |
| 2 VSS | GND | GND | ✅ |
| 4 R | →GND | GND | ✅ |
| 3 SDA | I2C | /D33 (3.3V) | I2C por SW, fuera del bus compartido |
| 6 SCL | I2C | /D32 (3.3V) | idem |
| DIEPAD | GND | GND | ✅ |

- ✅ Footprint asignado a U7 = `SGP30:SGP30` (**DFN-6**) → **incorrecto** para el módulo con header.
- ✅ MCP1700 (U10) y TXS0102 (U11) están **desconectados** del SGP30 (riel +1V8 sin pads). Probablemente **sobran** con el módulo.

### Acciones SGP30 (pendientes, NO aplicadas)
1. ⚠️ **Conseguir foto nítida del módulo (frente + reverso)** con etiquetas de pines y componentes (¿regulador SOT-23? ¿level shifter?). BLOQUEA el rediseño.
2. Reemplazar footprint DFN-6 por **footprint de header** según pinout real del módulo (probable VIN/VCC, GND, SCL, SDA; header de la foto parece de 5 pines → leer silk).
3. Si el módulo regula+traslada onboard: **eliminar U10 (MCP1700), U11 (TXS0102), riel +1V8**; alimentar VIN a 3.3V; I2C a 3.3V.
4. Decidir si SGP30 va al **bus I2C compartido** (dir 0x58, no colisiona) o se queda en D32/D33.

## 4. Bus I2C ⚠️
- Bus compartido: **U2 BME280, U3 MPU6050, U8 MLX90614** (nets `/SDA I2C`, `/SCL I2C`).
- SGP30 (U7) hoy **fuera** del bus (en D32/D33).
- 📄 Direcciones esperadas (verificar variantes): BME280 0x76/0x77, MPU6050 0x68/0x69, MLX90614 0x5A, SGP30 0x58. Sin colisión esperada.
- ⚠️ **Pull-ups del bus: no se encontraron resistencias en las nets I2C.** Sin pull-ups el bus no opera. Verificar en KiCad; si faltan, añadir un solo par 4.7k–10k a 3.3V.

## 5. LoRa (U4) ⚠️
- ⚠️ Símbolo `LoRa_RFM65W`. El proyecto habla de **RFM95W (SX1276)**. RFM95 ≠ RFM65 (pinout/RF distintos). **Confirmar parte real.**
- ✅ **C8/C9 sin net en el PCB** (flotantes). No verificable el "paralelo" desde PCB. ⚠️ Verificar en .sch que C8 y C9 estén entre 3.3V y GND (decoupling), no en serie.
- ERC: MOSI/SCK "Output↔Output" = **falso positivo** (tipos de pin). MISO not driven y DIO0 not driven = **posibles reales** → verificar NSS/SCK/MOSI/MISO/DIO0 al ESP32.
- Pendiente: layout RF/antena (50Ω, plano de tierra, conector).

## 6. ESP32 (U1) ⚠️
- ✅ **VIN_16 → +3.3V** en el PCB (ERC viejo lo marcaba flotante; ya conectado). Implica bypass del regulador interno del WROOM → OK solo con 3.3V regulados (U9 AP2112K-3.3). ⚠️ Asegurar que NO entren 5V a VIN.
- ⚠️ GND pin17 y EP (pin41) marcados sin conectar en ERC viejo → verificar que el **exposed pad** esté a GND (crítico vibración/térmica).
- EN/reset: pull-up + 100nF a GND + botón a GND = estándar correcto. ⚠️ **Usar pull-up 10k** (no 1k). Verificar valor real.
- ⚠️ Boot/strapping pins (IO0, etc.): no auditados → verificar arranque confiable.
- TX2 tipado como **Input** en el símbolo → falso positivo de ERC tras el cruce GPS. Conviene corregir tipo de pin.

## 7. GPS6MV2 (U6)
- 🔧 **Cruce UART RX/TX corregido esta sesión (solo .sch):** labels lado GPS renombradas `TX2→GPS_TX` (U6 pin3) y `RX2→GPS_RX` (U6 pin2).
  - Resultado: net `GPS_TX` = GPS TX + ESP32 RX2; net `GPS_RX` = GPS RX + ESP32 TX2. Cruce correcto. Falta propagar al PCB.
- ✅ Footprint `ZC323200` mecánicamente compatible con GY-GPS6MV2 (26×35.6mm, header 2.54, 4 holes M3). Pads = espejo de la vista superior → correcto para módulo apilado componentes-arriba. Pinmap 1=VCC,2=RX,3=TX,4=GND.
- ⚠️ VCC del GPS a 3.3V: verificar arranque (LDO onboard marginal a 3.3V; quizá quiere 5V).
- SH4 SHIELD sin conectar → atar shield a GND.

## 8. ERC — clasificación ✅ (reporte obsoleto, 18 errores)
- **Falsos positivos (tipos de pin):** GND "Power output↔Power output", SCL/SDA "Output↔Output", MOSI/SCK "Output↔Output". Limpiar corrigiendo tipos de pin en símbolos.
- **Reales / a verificar:** VIN U1 (parece resuelto), GND pin17 y EP pin41 de U1, #PWR01 sin fuente, DIO0 U4, MISO U4, SH4 U6.
- **Resuelto por mí:** U6 RX / U1 TX2 "not driven" (cruce GPS); ERC seguirá quejándose de TX2 por tipo de pin Input.

## 9. PCB / layout para vuelo ⚠️
- Caps de desacoplo flotantes (C8/C9 y otros sin net) → sin filtrado real. Crítico bajo vibración/ruido.
- EP/GND de ESP32 y SGP30 deben soldar a plano de tierra (mecánico + térmico).
- Si 2 PCBs: conectores entre placas con pinout explícito, GND/alimentación robustos, bloqueo mecánico anti-vibración.
- Separar RF (LoRa/GPS) de digital ruidoso; plano de tierra continuo.

## 10. Multi-PCB — ARQUITECTURA (actualizada 2026-06-22, **3 placas**)
- KiCad: **1 proyecto por PCB**; conexiones entre placas = **conectores físicos**. Nets no cruzan solas entre proyectos.
- **Forma:** placas **circulares Ø62mm** (tubo de cohete), apiladas (pancake vertical).
- **3 placas** (el ESP DevKit solo ya llena una Ø62 → 2 no alcanzaban):
  - **Placa 1 "Cerebro":** ESP32 DevKit 38p + regulación (buck-boost 1S) + batería. Hub.
  - **Placa 2 "Sensores":** GPS (NEO-6M, UART), MLX90614, BME280, MPU6050, SGP30 — I2C (+ GPS UART + MPU_INT). MLX mira afuera (IR); GPS u.FL al cielo.
  - **Placa 3 "SPI/RF":** microSD + LoRa RFM95W (SMA). RF aislado.
- **Bus inter-placa (backplane de apilado ~16p, 2×8):** `+3.3V, GND, GND, SDA, SCL, MPU_INT, GPS_TX, GPS_RX, SCK, MOSI, MISO, CS_SD, NSS_LoRa, DIO0, +2 spare`. ESP = hub. Orden stack sugerido: Sensores / ESP / RF.
- **Pull-ups I2C:** R2/R3 (un par a 3.3V) ya existen. Cada módulo breakout trae los suyos → al integrar, dejar UN solo par y levantar los demás.
- **Mecánica:** standoffs M2.5 en bolt-circle (~Ø54, 3–4 pts), separación ≥12mm; módulos pesados (GPS, batería) centrados y bajos; conector de apilado con retención; conformal coating a considerar.
- **Pendiente confirmar:** ubicación SGP30 (asumido Placa 2), ubicación batería, backplane único vs 2 conectores.

## 9b. Hardware CONFIRMADO por foto (2026-06-22)
- **U1 ESP32 = DevKit 38-pin (NodeMCU/WROOM-32)**, NO bare WROOM-1. Footprint actual `RF_Module:ESP32-S3-WROOM-1` = **incorrecto** → usar footprint DevKit 38p (2 filas, 2.54). El símbolo (pinout DevKit clásico) sí es el correcto.
- **U4 LoRa = RFM95W** (433MHz, SX1276), placa verde con SMA. Bandera "RFM65W" **resuelta**: es RFM95.
- **U5 microSD**: header 6p (3v3, CS, MOSI, CLK, MISO, GND).
- Sensores: GPS NEO-6M, MLX90614 (4p), BME/BMP280 (morada), MPU6050 GY-521 (1x08). SGP30 módulo aparte.
- Patrón confirmado: TODO son módulos con header → footprints de header, no chip desnudo (corrige U1 WROOM-1, U2 LGA-8, U7 DFN-6).

## 10b. Energía — DECISIÓN (2026-06-22)
- **Fuente: LiPo 1S (3.0–4.2V).**
- **AP2112K (U9, LDO) NO sirve para 1S** (dropout ~250mV → riel 3.3V colapsa a baja carga, brownout en pico LoRa). **Reemplazar por buck-boost a 3.3V** (clase TPS63020/TPS63060 o módulo) → 3.3V estable en todo el rango.
- **ESP DevKit:** alimentar por pin **3V3** (NO VIN; su AMS1117 onboard necesita >4.4V, imposible con 1S).
- **Obligatorio vuelo:** bulk ~100µF en supply LoRa + 100µF en 3V3 ESP; protección 1S (celda protegida / load-switch con UVLO); switch principal.
- F.Cu/B.Cu son capas de ruteo, no "declaran" placas; sí se pueden poner componentes en ambas caras.

## 11. Cambios aplicados hasta ahora (esta sesión)
- 🔧 `Schema_Experimento_Abordo.kicad_sch`: cruce GPS RX/TX (2 labels renombradas).
- 🔧 MPU6050 (U3): pin `AD0` tenía `(number "")` vacío → asignado **7**. Bloqueaba "Update PCB from Schematic" (footprint `PinHeader_1x08`). Corregido en el `.sch` cacheado y en `Libraries&Components/Ex_Abordo_Components.kicad_sym` (CRLF). Mapa header 1x08: 1=VCC,2=GND,3=SCL,4=SDA,5=XDA,6=XCL,7=AD0,8=INT (orden estándar GY-521; ⚠️ verificar silk físico).
- 🔧 Bus I2C: el MCU estaba **desconectado de los sensores** (nets `/SDA I2C`,`/SCL I2C` del ESP separadas de `/D33`,`/D32` de los sensores — choque de labels, mismo bug que el GPS). Fix: renombradas las 10 labels a `SDA`/`SCL` (limpio). Ahora ESP+BME+MPU+MLX+shifter en un bus `/SDA`,`/SCL` con pull-ups R2/R3. ✅ Verificado en Update PCB (reconnects + 0 errores).
- 🔧 Footprints: **U2 BME280** (`Bosch_LGA-8`→`PinHeader_1x04_P2.54mm_Vertical`) y **U8 MLX90614** (`TO-39-4`→`PinHeader_1x04_P2.54mm_Vertical`). Chip desnudo → header de módulo. ⚠️ verificar silk físico orden VIN,GND,SCL,SDA. Limpia 7 de los 13 warnings.
- 🔧 PWR_FLAG en **+BATT**: el ERC marcaba `#PWR01 (+BATT) power input not driven` (la batería de entrada no tenía fuente modelada). NO era el 3.3V — U9 VOUT (`power_out`) sí maneja +3.3V. Agregado `power:PWR_FLAG` (lib def + instancia `#FLG01`) coincidente con +BATT @ (209.55,88.9). Validado: paréntesis balanceados, archivo íntegro. Falta recargar + ERC (15→14 esperado). El stage de entrada cambia con el buck-boost 1S, pero el flag sigue válido.
- 🔧 Pasada de tipos de pin (KiCad cerrado): 27 cambios en el `.sch` cache (BME280, MPU6050, LoRa, microSD, MLX90614, ESP32). GND→`power_in` (ocultos 40/41 del ESP auto-conectan), I2C→`bidirectional`, SPI esclavo MOSI/SCK/CLK/CS/NSS→`input` + MISO/SDO→`output` + DIO0→`output` + RESET→`input`, ESP TX2→`bidirectional`. Añadido **PWR_FLAG #FLG02 en GND** (driver del net GND). Librería custom `Ex_Abordo_Components` actualizada igual (20 cambios). Esperado ERC: 13→~1-2 (queda U6 SH2 shield).
  - ⚠️ BME280/ESP32 son libs STOCK: solo se editó el cache del `.sch`, NO la lib global → NO hacer "Update Symbols from Library" sobre U1/U2 (revertiría tipos). Micro_SD: nombres de pin divergen entre cache (MOSI/CLK/MISO) y lib (SCK/SDI/SDO); nets se mapean por número.
- 🔧 PWR_FLAG en +BATT (#FLG01): agregado por el usuario en GUI (mi versión previa por texto se sobreescribió al guardar con KiCad abierto — lección: edits por texto requieren KiCad cerrado).
- 🔧 Bus SPI: el **microSD estaba desconectado del bus** (labels aislados D13/D23/D18/D19 = mismo bug que GPS/I2C; el LoRa sí estaba bien). Fix: renombrados → CS:`D13`→`CS MicroSD`, MOSI:`D23`→`MOSI SPI`, CLK:`D18`→`SCK SPI`, MISO:`D19`→`MISO SPI`. Mapeo verificado por posición de pines de U5. MISO compartido (multi-esclavo): LoRa+microSD MISO→`tri_state`, ESP D19→`input` (resuelve choque D19↔MISO). Lib actualizada. Esperado ERC: 5→~1 (queda U6 SH3 shield GPS).
  - Pendiente revisar (warnings, no errores): labels aislados `MPU6050 INT` (¿INT del MPU llega al ESP?), `Buzzer`, `D27`, `ANTENA` (normal). Config: agregar `Ex_Abordo_Components` y footprint `ZC323200` al lib-table del proyecto.
- 🔧 GPS shield (U6 SH1–4): cambiados a `power_in` + nombre `GND` + ocultos → auto-conectan a GND (agujeros de montaje a tierra). Cierra el último error ERC.
- 🔧 MPU6050 INT ↔ D27: conectados por el usuario (eran labels aislados).
- 🔧 Lib tables: `sym-lib-table` += `Ex_Abordo_Components` (limpia U3/U4/U5/U8); creado `fp-lib-table` con `ZC323200` (limpia footprint U6). `ZC323200.kicad_sym` es formato viejo (2 espacios), NO modificado, NO agregado al sym-lib-table (evita mismatch; el cache ya tiene shield→GND).
- Estado ERC: **0 errores** esperados. Warnings restantes = cosméticos: `lib_symbol_mismatch` BME280/ESP32 (cache editado vs lib stock — NO hacer Update-from-Library en U1/U2), posible mismatch Micro_SD (nombres pin cache vs lib), `ANTENA` (normal), `Buzzer` (sin cablear). Historial errores: 18→15→13→5→1→0.
- ✅ **HITO (2026-06-23): ERC 0 errores + Update PCB 0 errores.** Shield GPS→GND propagado, MPU INT→/MPU6050 INT, footprints 1x04/1x06/1x08 aplicados. 7 warnings ERC = cosméticos (`lib_symbol_mismatch` cache vs lib; `ANTENA`/`Buzzer` aislados). 9 warnings PCB = pads 31-39 del ESP (footprint WROOM-1, se van con DevKit). Esquemático CERRADO.
- Nada más editado. Otros footprints/símbolos/PCB: sin cambios.

## Estado al cierre del esquemático (2026-06-23)
Esquemático ERC-limpio (0 err). Buses conectados y verificados: I2C (BME/MPU/MLX/SGP30 + ESP), SPI (LoRa+microSD compartido + CS dedicados), UART GPS (cruzado), power (U9→3.3V con PWR_FLAG +BATT/GND). **PENDIENTE (bloquea layout/3 placas):**
1. Footprints custom: ESP DevKit 38p (medir separación filas + pitch), LoRa verde breakout (pitch 2.0/2.54?, separación, silk P1/P2). Footprint LoRa actual `HOPERF_RFM9XW_SMD` = incorrecto.
2. Buck-boost 1S→3.3V reemplaza U9 (AP2112K LDO no sirve para 1S).
3. Decidir Buzzer (cablear o quitar label).
4. Luego: 3 proyectos KiCad + backplane 2×8 + layout circular Ø62.

## Firmware (esqueleto, 2026-06-23)
`Experimento_Abordo/Firmware/Firmware.ino` — sketch ESP32 con pines VERIFICADOS del esquemático.
Pin map: I2C 33/32; SPI 18/19/23; LoRa CS5/RST14/DIO0-26; SD CS13; MPU_INT 27; Buzzer 25; GPS UART2 16/17.
Lee sensores → calcula VPD + CBI (índice de fuego desde T/HR) → log SD 5Hz → telemetría LoRa 1Hz. Falta: watchdog, recuperación I2C, detección de fases por accel, baseline SGP30, objetivo científico final.

## 11b. Footprints — estado y decisiones (2026-06-22)
Conteo de pines por símbolo (verificado): BME280=4 (VIN/GND/SCL/SDA), MLX90614=4 (VIN/GND/SCL/SDA), microSD=6 (VCC/CS/MOSI/CLK/MISO/GND), MPU6050=8, LoRa(símbolo `RFM65W`)=**16 pines** (RFM9x pelón: GND/MISO/MOSI/SCK/NSS/RESET/DIO5/GND/DIO2/DIO1/DIO0/3.3V/DIO4/DIO3/GND/ANT).

| Ref | Footprint | Estado |
|---|---|---|
| U2 BME280 | `PinHeader_1x04_P2.54mm_Vertical` | ✅ asignado (verificar silk) |
| U8 MLX90614 | `PinHeader_1x04_P2.54mm_Vertical` | ✅ asignado (verificar silk) |
| U5 microSD | `PinHeader_1x06_P2.54mm_Vertical` | ✅ ya estaba |
| U3 MPU6050 | `PinHeader_1x08_P2.54mm_Vertical` | ✅ |
| U6 GPS | `ZC323200:XCVR_ZC323200` | ✅ |
| U11 TXS0102 | `SSOP-8` | ✅ chip real |
| U1 ESP DevKit 38p | 2×19 header | ❌ CUSTOM — faltan medidas |
| U4 LoRa adapter (verde P1/P2) | 2 filas | ❌ CUSTOM — faltan medidas+silk |
| U7 SGP30 módulo | `PinHeader_1x04` | ⏳ símbolo nuevo creado, falta swap en GUI |
| U9 AP2112K | `SOT-23-5` | 🔁 → buck-boost (1S) |
| U10 MCP1700 1.8V | `SOT-23` | 🔁 quitar (módulo no necesita 1.8V) |

### Medidas pendientes (usuario, con calibrador)
**ESP32 DevKit 38p:** (1) confirmar 19 pines/fila; (2) **separación centro-a-centro entre filas (mm)** ← crítico; (3) pitch (¿2.54?).
**LoRa verde P1/P2:** (4) # pines P1 y P2; (5) **pitch (¿2.0 o 2.54mm?)**; (6) separación filas P1↔P2 (mm); (7) silk/orden de P1 y P2.
**SGP30 módulo:** ✅ CONFIRMADO = **4 pines, silk VIN/GND/SCL/SDA, 1.8–5V (regulador onboard)**. Footprint `PinHeader_1x04`, pad 1=VIN,2=GND,3=SCL,4=SDA. Símbolo nuevo creado: `Libraries&Components/SGP30_Module.kicad_sym`.
  - Rework pendiente (GUI): agregar librería; borrar **U10 (MCP1700)**, **U11 (TXS0102)**, power **+1V8**, labels **SDA_1V8/SCL_1V8**; *Change Symbol* de U7 → `SGP30_Module`; recablear VIN→+3.3V, GND→GND, SCL→SCL, SDA→SDA; ERC + Update PCB. Resultado: bus de sensores 3.3V puro, sin shifter.

## 12. Próximos pasos (orden de seguridad)
1. **Medidas físicas** (ESP DevKit, LoRa, SGP30) → crear footprints custom DevKit 2×19 y LoRa.
2. SGP30: rework símbolo a módulo 4-pin + quitar U10/U11/+1V8.
3. Update PCB → limpiar los 13 warnings → ERC/DRC.
4. Crear los **3 proyectos KiCad** (Placa 1 Cerebro / Placa 2 Sensores / Placa 3 SPI-RF) + símbolo de backplane 2×8 común.
5. Regulación: buck-boost 1S→3.3V (reemplaza U9); bulk caps; protección 1S.
6. Layout circular Ø62 por placa; rutear; ERC/DRC limpios por proyecto.

### Decisiones del usuario (cerradas 2026-06-22)
- SGP30 → Placa 2. Batería → **capa propia**. Conector → **backplane único** (~16p, 2×8). Footprints **primero**.

## Reglas de trabajo
- No asumir que Sonnet tenía razón. No inventar pines/nets/valores/footprints.
- Marcar dependencias de datasheet (📄) y de KiCad (⚠️).
- Priorizar: alimentación, GND, niveles lógicos, conectores entre placas, desacoplos, pines flotantes.
- No editar el .sch sin confirmar que no quedó en estado parcial.
