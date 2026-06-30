# IGNIS-1 — estructura KiCad

## Fuente canónica = `boards/`

El diseño real y la fabricación viven en **3 proyectos KiCad independientes**:

- `boards/cerebro/` — power + MCU (ESP32, buck-boost 3V3, batería/RBF/switch)
- `boards/sensores/` — sensores + boost 5V (BME280, MPU6050, MQ-135, GPS)
- `boards/rf/` — comms + storage (LoRa RFM95W, microSD)

Cada uno es `*.kicad_pro` con su `*.kicad_sch` + `*.kicad_pcb`. **Abrir cada proyecto por separado** para editar/rutear. Se conectan por backplane 2×10 (J40/J41/J42, pinout idéntico).

## `IGNIS-1.kicad_sch` = NO CANÓNICO

El jerárquico raíz (`IGNIS-1.kicad_sch` + `sheets/*.kicad_sch`) es solo **referencia integrada histórica** previa al split en 3 placas. **NO refleja** los cambios hechos después en `boards/`:

- footprints `*_HandSolder` (C/R/LED SMD),
- caps bulk polarizados THT (`C_Polarized`: C2, C10, C32, C4),
- eliminación de U1 (protección 1S incluida en la celda LiPo) + re-cableado de la batería.

No editar ni fabricar desde el jerárquico.
