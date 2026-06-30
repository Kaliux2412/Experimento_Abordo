# Lista de compra — IGNIS-1 (2026-06-23)

Para pruebas en protoboard + armado. Pasivos en **THT** (protoboard).

## Módulos

### Ya deberías tener (build anterior — verificar)
| Módulo | Uso |
|---|---|
| ESP32 DevKit 38p | MCU (✅ confirmado) |
| BME280 (T/HR/P) | clima |
| MPU6050 GY-521 | IMU |
| GPS NEO-6M (GY-GPS6MV2) | posición |
| LoRa RFM95W 433MHz + antena SMA | radio |
| microSD module (SPI) + tarjeta | log |

### Nuevos — pedir
| Módulo | Uso | Notas |
|---|---|---|
| MQ-135 | gas combustión | nuevo |
| Buck-boost 3.3V (U2) | riel 3.3V de 1S | TPS63020 o buck-boost ajustable |
| Boost 5V (U3) | riel 5V (MQ + GPS) | MT3608 ajustable → 5V |
| LiPo 1S 3.7V protegida | batería | UNIT 503450, 1000mAh/3.7Wh, **con PCM integrado** + JST |

> **U1 (protección 1S) eliminado del esquemático y PCB:** la celda LiPo 503450 ya trae circuito de protección (PCM: sobre/baja descarga, sobre-corriente, corto) bajo la cinta. No se compra módulo aparte. La protección de batería va **incluida en la celda**. (Recargar sigue requiriendo cargador 1S externo, ej. TP4056.)

## Pasivos THT (protoboard)
Resistencias 1/4W: 4.7k ×2, 10k ×1, 15k ×1, 1k ×2, 330Ω ×1  → kit surtido.
Capacitores: 100nF cerámico ×4, 22µF elec ×1, 100µF elec ×3, 220µF elec ×1  → kit.
Discretos: LED ×2 (verde/azul), 1N4148 ×1, 2N3904 NPN ×1, buzzer activo 3–5V ×1.

## Mecánica / conectores
- Headers 2.54mm macho/hembra (módulos + backplane 2×10)
- Standoffs M2.5 + tornillos + tuercas (stack 3 placas, ≥12mm)
- JST batería (si la LiPo no lo trae)

## Nota para protoboard (primeras pruebas)
- Podés empezar sin los reguladores: ESP por USB, sensores 3.3V del pin 3V3 del ESP, y **MQ-135/GPS desde un 5V de banco/USB**. Probás I²C/SPI/UART y lecturas.
- MQ-135 calienta ~150mA @5V — fuente con corriente suficiente.
- El divisor del MQ (10k/15k) baja su A0 (0–5V) a ~3V antes del ADC del ESP (GPIO35).
- Cuando llegue el buck-boost/boost, probás el árbol de energía aparte antes de juntar todo.
