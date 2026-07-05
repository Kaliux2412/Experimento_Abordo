# IGNIS-1 — Recableo: Split de Backplane (potencia + señal)

**Fecha:** 2026-07-05
**Objetivo:** partir el backplane único 2×10 (saturado, 19/20 pines usados) en dos buses:
- **J_PWR** — potencia (`IGNIS1:J_PWR_2x4`, footprint `J_PWR_2x4_Keyed`, 2×4 con pin7 = llave anti-inversión).
- **J_DAT** — señal (reusa el 2×10 actual; conserva refdes J40/J41/J42; llave = pin3).

**Parts ya creados y validados** (kicad-cli export svg OK). Recableo = manual en GUI.

**Simplificación:** el J_DAT reusa el 2×10 tal cual. Señales y GND se quedan en su lugar; solo se le quitan los rieles (+3V3/+5V/VSW) que pasan al J_PWR nuevo (ref **J50**).

---

## Pinouts de referencia

### J_PWR (fijo en los 5 boards)
```
1 = +3V3    2 = GND
3 = +5V     4 = GND
5 = VSW     6 = GND
7 = KEY     8 = GND     (pin7 = llave, sin pad)
```

### J_DAT (2×10, pinout actual — NO cambia)
```
1 = +3V3*   2 = GND     3 = KEY     4 = GND      (*+3V3 se ELIMINA aquí, va a J_PWR)
5 = SDA     6 = SCL     7 = MPU_INT 8 = GPS_TX
9 = GPS_RX  10 = MQ_AOUT 11 = GND
12 = SCK    13 = MOSI   14 = MISO   15 = CS_SD
16 = NSS_LORA 17 = DIO0 18 = LORA_RST
19 = +5V*   20 = VSW*                            (*rieles se ELIMINAN, van a J_PWR)
```

---

## cerebro  (señal + potencia → 2 conectores)

**Borrar**
- [ ] Wire `+3V3 → J40 p1`.
- [ ] *(rebalanceo opcional)* Componentes **D2 + R10** (LED power-on) con sus wires → van a energia.

**Agregar**
- [ ] Símbolo `IGNIS1:J_PWR_2x4`, ref **J50**.

**Recablear**
- [ ] J50: `p1=+3V3`, `p2/4/6/8=GND`, `p3(+5V)=NC`, `p5(VSW)=NC`.
- [ ] J40 (ahora J_DAT): **sin cambios**. Solo quedó libre p1.

---

## sensores  (señal + potencia → 2 conectores)

**Borrar**
- [ ] Wires `+3V3 → J41 p1` y `+5V → J41 p19`.
- [ ] *(rebalanceo opcional)* Componente **C4 220µF** con sus wires → va a energia (renómbralo a C6 allá).

**Agregar**
- [ ] Símbolo `IGNIS1:J_PWR_2x4`, ref **J50**.

**Recablear**
- [ ] J50: `p1=+3V3`, `p3=+5V`, `p2/4/6/8=GND`, `p5(VSW)=NC`.
- [ ] J41 (J_DAT): **sin cambios**. Libres p1, p19.

---

## rf  (señal + potencia → 2 conectores)

**Borrar**
- [ ] Wire `+3V3 → J42 p1`.

**Agregar**
- [ ] Símbolo `IGNIS1:J_PWR_2x4`, ref **J50**.

**Recablear**
- [ ] J50: `p1=+3V3`, `p2/4/6/8=GND`, `p3(+5V)=NC`, `p5(VSW)=NC`.
- [ ] J42 (J_DAT): **sin cambios**. Libre p1.

---

## carga  (solo potencia → 1 conector)

**Borrar**
- [ ] Conector 2×10 **J40 completo** (wires +3V3 p1, VSW p20, GND).
- [ ] **U2 (TPS63020) + C1 + C2** → se mueven a energia. carga ya no regula 3V3, solo carga batería + saca VSW.

**Agregar**
- [ ] Símbolo `IGNIS1:J_PWR_2x4`, ref **J50**.

**Recablear**
- [ ] J50: `p5=VSW` (salida de switch/batería), `p2/4/6/8=GND`, `p1(+3V3)=NC`, `p3(+5V)=NC`.

---

## energia  (solo potencia → 1 conector)

**Borrar**
- [ ] Conector 2×10 **J40 completo** (wires +5V p19, VSW p20, GND).

**Agregar**
- [ ] Símbolo `IGNIS1:J_PWR_2x4`, ref **J50**.
- [ ] **U2 (TPS63020) + C1 + C2** (desde carga): `VSW → U2 → +3V3`; C1 en VSW, C2 en +3V3.
- [ ] *(rebalanceo)* **D2 + R10** (LED power-on desde cerebro): `+3V3 → R10 → D2 → GND`.
- [ ] *(rebalanceo)* **C6 220µF** (era C4 de sensores): `+5V → C6 → GND`.

**Recablear**
- [ ] J50: `p1=+3V3` (salida de U2), `p3=+5V` (salida de U3), `p5=VSW` (entrada de U2 y U3), `p2/4/6/8=GND`.

---

## Cierre (todos los boards)
- [ ] F8 (Update PCB from Schematic) en los 5 boards.
- [ ] Re-ubicar / rutear conectores nuevos.
- [ ] `./check_boards.sh` (ERC + DRC + netlist).

**Nota:** las líneas *(rebalanceo)/(opcional)* solo sirven para llenar el board energia. Si solo quieres el split de backplane, ignóralas.
