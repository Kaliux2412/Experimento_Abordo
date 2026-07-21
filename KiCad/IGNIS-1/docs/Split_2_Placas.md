# IGNIS-1 — Split en 2 placas (poder+cerebro / sensores)

**Fecha:** 2026-07-20
**Estado:** esquemas cerrados y validados (ERC 0 / DRC 0). Falta poblar y rutear los PCB en la GUI.

Sustituye al backplane split de 5 boards (ver `Recableo_Backplane_Split.md`, **obsoleto**) y a
`placa_unificada/` (un solo board 100×60 que no cupo).

---

## 1. Arquitectura

Dos placas **58 × 100 mm** apiladas en sandwich, unidas por un header macho/hembra — sin cables.

| Placa | Carpeta | Contenido | Comps | Área usada |
|---|---|---|---|---|
| **A** | `placa_poder_cerebro/` | ESP32, LoRa, microSD, TPS63020, buzzer | 22 | ~3480 mm² (60 %) |
| **B** | `placa_sensores/` | BME280, MPU6050, MQ-135, GPS, MT3608, TP4056, batería | 17 | ~3000 mm² (52 %) |

**Regla de ocupación:** con módulos THT grandes, no pasar de **60–65 %** de courtyard sobre área de
placa, o no queda por dónde rutear.

### Reparto de la energía

- **TPS63020 (U2, 3V3) se queda en la placa A** → el 3V3 se regula junto a sus consumidores con
  transitorios (ESP32 + LoRa): regulación en el punto de carga.
- **MT3608 (U3, boost 5V) y el bloque carga/batería (TP4056, JST, switch) van en la placa B** →
  el 5V se genera junto a su único consumidor (MQ-135).
- Consecuencia: **`VSW` (batería tras el switch) cruza el bus** y `+5V` ya no lo cruza.

> **Gotcha:** el MT3608 no se alimenta de +3V3 sino de VSW, igual que el TPS63020. Por eso moverlo
> a la placa B no liberó ningún pin del bus: salió `+5V`, entró `VSW`.

---

## 2. Geometría dentro del cohete

Tubo de **66 mm de diámetro interior** (R = 33). Componentes en la cara **externa** de cada placa
(hacia la pared); las caras internas del sandwich solo llevan cobre y soldaduras.

Con ese arreglo no hay compromiso: bajar la separación `d` ensancha las placas **y** agranda el
espacio para componentes a la vez.

- **Separación entre placas: `d` = 11 mm**, fijada por el par de conectores
  (PinSocket 8.5 mm de cuerpo + 2.5 mm de plástico del PinHeader).
- Los pines THT sobresalen ~2 mm hacia dentro en ambas placas → quedan **~7 mm de aire**.
  **Cortar los pines al ras (≤1.5 mm).**
- **Ancho 58 mm**, que deja 3.1 mm de holgura en la esquina para guías y tolerancia de corte.

### Perfil de altura disponible (cara externa)

```
x del centro:   0     5    10    15    20    25    29  mm
alto libre:   25.9  25.5  24.3  22.3  19.1  14.4   8.6 mm
```

**Los componentes altos van al centro, los bajos al borde.** En los últimos 4 mm de cada borde,
solo SMD.

| Componente | Alto | Zona permitida |
|---|---|---|
| MQ-135 | 18 mm | x ≤ ±21 mm |
| ESP32 DevKit | 13 mm | x ≤ ±26 mm |
| MT3608 / TP4056 / GPS | 10–12 mm | x ≤ ±27 mm |
| SMD 0805/1206 | < 2 mm | cualquiera |

Fórmula del ancho máximo para otro diámetro:
`W = 2·√((D/2 − margen)² − (d/2)²)`

---

## 3. Bus inter-placa — J50

**9 nets** en un conector **2×06 de 2.54 mm**:
`VSW, +3V3, GND, SDA, SCL, GPS_TX, GPS_RX, MPU_INT, MQ_AOUT`

- Placa A: `Connector_PinHeader_2.54mm:PinHeader_2x06_P2.54mm_Vertical` (macho)
- Placa B: `Connector_PinSocket_2.54mm:PinSocket_2x06_P2.54mm_Vertical` (hembra)

```
1  VSW      2  VSW        <- VSW lleva la corriente de todo el sistema: 2 pines
3  GND      4  GND
5  MQ_AOUT  6  +3V3       <- MQ_AOUT es analógico: rodeado de GND (pines 3 y 7)
7  GND      8  SDA
9  SCL     10  MPU_INT
11 GPS_TX  12  GPS_RX
```

El SPI (`SCK/MOSI/MISO/CS_SD`) **no cruza el bus**: LoRa y microSD están ambos en la placa A.
Moverlo costaría +4 nets y obligaría a un 2×07.

---

## 4. Barrenos

4 × M2.5 (`MountingHole:MountingHole_2.7mm_M2.5`), a 4.5 mm de cada borde. Coinciden en posición
entre ambas placas → los mismos espárragos atraviesan las dos y fijan la separación de 11 mm con
separadores.

> **Gotcha:** H1–H4 no tienen símbolo en el esquema. Al hacer *Update PCB from Schematic* (F8),
> **desmarcar "Delete footprints with no symbol"** o KiCad los borra.

---

## 5. Colocación de capacitores

Junto a qué componente va cada cap. La netlist no lo distingue (todos los de +3V3 comparten net),
así que esto es decisión de diseño.

### Placa A

| Cap | Valor | Footprint | Va pegado a | Función |
|---|---|---|---|---|
| C1 | 22 µF | 1206 | **U2** TPS63020, pin VIN | entrada del DC-DC |
| C5 | 100 µF | 1210 | **U2** TPS63020, pin VIN | bulk de entrada |
| C2 | 100 µF | 1210 | **U2** TPS63020, pin VOUT | salida del DC-DC |
| C10 | 100 µF | 1210 | **U10** ESP32, pin 3V3 | bulk del riel |
| C11 | 100 nF | 0805 | **U10** ESP32, pin 3V3 | desacoplo — el más pegado de todos |
| C32 | 100 µF | 1210 | **U30** LoRa RFM95 | bulk para picos de TX (~120 mA) |
| C30 | 100 nF | 0805 | **U30** LoRa, pin VCC | desacoplo |
| C31 | 100 nF | 0805 | **U31** microSD, pin VDD | desacoplo |

### Placa B

| Cap | Valor | Footprint | Va pegado a | Función |
|---|---|---|---|---|
| C4 | 220 µF | tantalio EIA-7343 | **U3** MT3608, salida OUT+ | salida del boost |
| C40 | 220 µF | tantalio EIA-7343 | **U22** MQ-135 | bulk en el consumidor (el heater tira fuerte) |
| C20 | 100 nF | 0805 | **R22 / R23** | ⚠️ **no es desacoplo** |

**C20** está en `MQ_AOUT + GND`, no en un riel: es el filtro RC del ruido de la salida analógica
del MQ-135 antes del ADC. Va junto al divisor, **no** pegado a un chip ni a un riel.

### Reglas al colocar

1. **El pequeño gana la carrera al pin.** Donde hay par (C11+C10 en el ESP32, C30+C32 en el LoRa),
   el 100 nF va más cerca del pin que el electrolítico.
2. **Vía a GND corta.** Un cap bien elegido con retorno largo no sirve.
3. **C1/C5/C2 alrededor de U2**, entrada de un lado y salida del otro, sin cruzar corrientes.

### Pendiente

La placa B **no tiene ningún cap en +3V3**. Ese riel cruza el bus desde la placa A y alimenta
BME280 + MPU6050. Los módulos traen su desacoplo propio, pero convendría un **10–100 µF junto a
J50 en la placa B** como bulk local. Ese cap todavía no existe en el esquema.

---

## 6. Footprints propios usados

Todos en `footprints/IGNIS1.pretty/`, montados cara-arriba con pines abajo (mezzanine),
vista superior 1:1 sin espejo.

| Footprint | Medidas | Notas |
|---|---|---|
| `ESP32_DevKit_40p` | 28 × 52.3 mm | keep-out de antena del lado del pin 1 |
| `GPS_NEO6M_module` | 35.7 × 26 mm | 4 barrenos NPTH 3 mm |
| `TPS63020_module` | 34 × 24 mm | pads con número compartido por neto |
| `MT3608_module` | 31 × 17.5 mm | 4 postes THT |
| `TP4056_HW373_module` | 28 × 17.5 mm | |
| `LoRa_RFM95_adapter` | 20 × 21.8 mm | |
| `MicroSD_module` | 17.76 × 17.49 mm | ver nota de numeración abajo |
| `MPU6050_module` | 15 × 20 mm | 2 barrenos NPTH 3 mm |
| `BME280_module` | 13.6 × 10.4 mm | barreno 3 mm arriba-derecha, a 3 mm del borde |

### MicroSD — numeración de pads no consecutiva

El orden físico del módulo es `3v3, CS, MOSI, CLK, MISO, GND`, pero el símbolo `IGNIS1:Micro_SD`
numera `1=VCC 2=CS 3=SCK 4=SDI 5=SDO 6=GND`. MOSI y CLK están intercambiados entre uno y otro,
así que **la secuencia de pads es 1, 2, 4, 3, 5, 6**:

| Posición | Señal | Pad |
|---|---|---|
| 1 (arriba) | 3v3 | 1 |
| 2 | CS | 2 |
| 3 | MOSI | **4** |
| 4 | CLK | **3** |
| 5 | MISO | 5 |
| 6 (abajo) | GND | 6 |

La ranura de la tarjeta queda en el borde derecho (marcada `SD >` en la serigrafía):
**dejar espacio libre para insertar y extraer.**

---

## 7. Gotchas de KiCad encontrados

**Footprint por defecto de la librería vs. instancia.** Poner el footprint por defecto en el
símbolo de la librería solo afecta a símbolos colocados *después*. Las instancias ya existentes
conservan su propiedad `Footprint` hasta *Tools → Update Symbols from Library* o cambio manual.
Esto tuvo a U20 (BME280) dibujándose como conector de 4 pines pese a que el footprint correcto
existía desde el 6 de julio. **U22 (MQ-135) sigue igual**: tiene `PinHeader_1x04` y aún no tiene
footprint propio.

**PWR_FLAG duplicados.** Al fusionar los 5 boards originales quedaron 13 PWR_FLAG, varios sobre la
misma net → error "Power output and Power output are connected". Regla: **un PWR_FLAG por net, y
solo si esa net no tiene ya un pin `power_out` real que la maneje.** Se borraron 6 (los de +3V3 en
A porque U2.VOUT es driver; los de GND y +5V en B porque U40.OUT− y U3.OUT+ lo son). El PWR_FLAG
va siempre en grupo aislado de 3 elementos: `power:<NET>` + wire vertical de 5.08 mm + `PWR_FLAG`
— hay que borrar los tres o quedan dangling.

**Parseo por script del `.kicad_sch`:**
- Los PWR_FLAG **no aparecen en la netlist exportada**; hay que trazarlos por geometría.
- El opener de los wires es `\t(wire\n` (multilínea), **no** `\t(wire ` — buscar con espacio no
  encuentra nada y falla en silencio.
- Al mover una hoja jerárquica entre proyectos hay que reescribir, dentro del `.kicad_sch` hijo,
  el `(project "...")` y los instance-path: `/<ROOT_uuid>/<sheet_symbol_uuid>` en los símbolos y
  `/<sheet_symbol_uuid>` en `sheet_instances`.

---

## 8. Estado y siguientes pasos

Validado por `kicad-cli`: **ERC 0 errores y DRC 0 violaciones en ambas placas.**

1. Abrir cada `.kicad_pro` → PCB → *Update PCB from Schematic* (F8),
   **desmarcando "Delete footprints with no symbol"** (barrenos H1–H4).
2. Colocar: altos al centro, SMD en los bordes, caps según la sección 5.
3. Rutear + GND pour.
4. Pendientes abiertos:
   - footprint propio del **MQ-135** (U22 sigue con `PinHeader_1x04`)
   - cap de bulk +3V3 junto a J50 en la placa B
   - limpieza cosmética de wires colgantes en `sub_energia` (donde estaba el boost)

`boards/` y `boards_backup_2/` conservan los 5 PCB ruteados originales y **no se han tocado**.
