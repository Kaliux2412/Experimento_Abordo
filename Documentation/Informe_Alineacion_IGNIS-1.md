---
title: "Informe de Alineación Técnica — CanSat IGNIS-1"
subtitle: "Revisión del diseño electrónico frente a los objetivos de misión (LASC 2026)"
author: "Área de Electrónica / Aviónica — Equipo IGNITIA"
date: "23 de junio de 2026"
lang: es
geometry: margin=2.5cm
fontsize: 11pt
toc: true
toc-depth: 2
---

\newpage

# Resumen ejecutivo

Esta revisión compara el **estado actual del diseño electrónico** del CanSat IGNIS-1 contra los **objetivos oficiales de la misión** descritos en los *Progress Updates* de LASC. El esquemático en KiCad alcanzó un estado **eléctricamente limpio** (0 errores de ERC) tras corregir múltiples desconexiones críticas. Sin embargo, se detectó una **desviación importante entre el hardware construido y la lista de componentes declarada en la misión**, principalmente en los sensores que sustentan el objetivo científico (detección de incendios).

Se requieren **tres decisiones de dirección** antes de avanzar a la fabricación final:

1. **Sensores de gas y material particulado** — el hardware actual no incluye el sensor de material particulado (humo) que exige la misión, y usa un sensor de gas distinto al especificado.
2. **Presupuesto de masa y forma** — verificar el cumplimiento de los **300 g** y el formato CanSat "1C".
3. **Índice de calidad de comunicación** — aún no implementado en el firmware (entregable de la misión secundaria).

El tiempo es crítico: la competencia es del **2 al 5 de septiembre de 2026** (~10 semanas).

\newpage

# 1. Contexto de la misión

**IGNIS-1** es un CanSat de la **Latin American Space Challenge (LASC) 2026**, vinculado a la misión de cohete ID 22, con apogeo objetivo de **1 000 m AGL**. Tras la eyección se recupera de forma independiente mediante paracaídas.

| Parámetro | Valor |
|---|---|
| Misión primaria | Detectar condiciones que llevan a incendios forestales |
| Variables requeridas | Temperatura, humedad, presión, **concentración de gas**, **material particulado** |
| Misión secundaria | Calidad del enlace de radio (**RSSI + pérdida de paquetes**) vs altitud |
| Entregables | Índice de riesgo de incendio + índice de calidad de comunicación |
| Masa objetivo | ~300 g |
| Dimensiones | "1C" (~ 66 mm de diámetro) |
| Energía | LiPo, con **RBF pins** (corte total) y protección de batería; sin paneles solares |
| Recuperación | Paracaídas pasivo; orientación vertical crítica para datos consistentes |

# 2. Estado actual del diseño electrónico

El esquemático en KiCad fue auditado y corregido a fondo. Se partió de **18 errores de ERC** y múltiples buses **desconectados** (no detectados a simple vista) hasta llegar a **0 errores**.

Correcciones aplicadas y verificadas:

- **UART del GPS**: las líneas RX/TX no estaban cruzadas ni conectadas; corregido.
- **Bus I²C**: el microcontrolador estaba **eléctricamente desconectado** de los sensores (BME280, MPU6050, MLX90614, SGP30); corregido y unificado.
- **Bus SPI**: el módulo microSD no estaba conectado al bus; corregido (LoRa estaba correcto).
- **Alimentación del SGP30**: estaba a 3.3 V (destructivo para el chip desnudo); resuelto al confirmarse que es un módulo con regulador propio.
- **Tipos de pin, banderas de alimentación (PWR_FLAG), blindaje del GPS a tierra, tablas de librerías**: corregidos.

**Conclusión:** el esquemático es funcional y verificable. El netlist (conexiones) es correcto. Lo que sigue depende de decisiones de componentes y de mecánica, no de errores de cableado.

# 3. Hallazgo crítico: desviación de hardware frente al spec

La misión declara una lista de sensores específica. El hardware construido **difiere** en los componentes que sustentan el objetivo científico.

| Componente (spec oficial) | Hardware construido | Estado |
|---|---|---|
| Sensor de gas **MQ-2 / MQ-135** | **SGP30** | Sustituido por uno más débil |
| Sensor de **material particulado** (humo) | **No existe** | **FALTANTE — sensor núcleo de la misión** |
| BME280 (T/H/P) | BME280 | Correcto |
| (no listados) | MLX90614, MPU6050, GPS | Extras no especificados |

**Interpretación:**

- El **material particulado (humo)** es central para "detectar incendios" y **no está en el diseño**. Es el hallazgo más relevante.
- El **SGP30** es un sensor de compuestos orgánicos volátiles (VOC) calibrado para interiores; responde a gases de combustión, pero **no es confiable como medidor de calidad de aire o humo** en exterior y en caída libre. El **MQ-135** especificado es más apropiado para gases de combustión.
- Los sensores **extra** (no en el spec) pueden justificarse dentro de la narrativa de la misión:
  - **GPS**: justificado — la estación terrena rastrea el CanSat en mapa y apoya la recuperación.
  - **MPU6050 (IMU)**: justificable — verifica la orientación vertical (que el documento marca como crítica) y la dinámica del descenso.
  - **MLX90614 (IR)**: el más cuestionable; defendible solo como "temperatura de superficie remota" (suelo seco/caliente). Candidato a eliminarse para ahorrar masa.

# 4. Restricciones CanSat aún no verificadas

1. **Masa (300 g)**: con DevKit ESP32 + 4–5 módulos de sensor + GPS + LoRa + 3 PCBs + LiPo + estructura, el conjunto **probablemente excede los 300 g**. Se requiere un **presupuesto de masa** formal. El **DevKit es pesado y voluminoso**; un módulo ESP32 "desnudo" (WROOM) ahorraría masa y volumen.
2. **Forma "1C" (~ 66 mm Ø)**: las placas diseñadas (Ø62 mm) caben, pero el **apilado de 3 placas + batería** debe entrar en la altura de una lata CanSat (~115 mm). Margen ajustado.
3. **RBF pin (Remove Before Flight)**: requisito de seguridad para cortar toda la energía. Existe un interruptor (SW1) pero **falta un RBF pin/conector dedicado** en el diseño de potencia.
4. **Protección de batería**: requerida; alineada con la recomendación previa de protección para LiPo 1S.

# 5. Firmware e índices de misión

- Existe un **esqueleto de firmware** con el mapa de pines verificado, lectura de sensores, cálculo preliminar del **índice de fuego** (VPD y *Chandler Burning Index* a partir de T/HR), registro en microSD y telemetría LoRa.
- **Falta el índice de calidad de comunicación** (misión secundaria): el firmware debe enviar **número de secuencia** por paquete, y la **estación terrena** debe medir **RSSI** y **pérdida de paquetes** contra altitud. Es un **entregable obligatorio** aún no implementado.

# 6. Riesgos y cronograma

- **Hoy: 23 de junio de 2026.** El *full flight test* estaba programado para el 22 de junio. Competencia: **2–5 de septiembre** — **~10 semanas**.
- Riesgo principal: el margen de tiempo **no permite rediseños grandes**. La decisión de sensores (sección 3) debe cerrarse de inmediato para no comprometer la fabricación de la(s) PCB(s).
- Riesgo secundario: si la masa excede 300 g, podría requerir recortar sensores tarde en el ciclo.

# 7. Recomendaciones priorizadas

| # | Decisión / acción | Responsable sugerido | Urgencia |
|---|---|---|---|
| 1 | **Definir sensores de gas y humo**: ¿agregar sensor de **material particulado** (p. ej. PMS5003) + **MQ-135**, o defender formalmente SGP30/MLX como sustitutos? | Tecnología + Liderazgo | **Inmediata** |
| 2 | **Presupuesto de masa** vs 300 g; evaluar cambiar DevKit por módulo WROOM y/o eliminar MLX90614 | Tecnología | Alta |
| 3 | **Implementar RSSI + pérdida de paquetes** en firmware y estación terrena | Software / Comunicaciones | Alta |
| 4 | **Diseñar el RBF pin** y confirmar protección de batería | Electrónica / Seguridad | Alta |
| 5 | **Alinear el documento oficial de misión con el hardware real** (o viceversa) antes de la competencia | Liderazgo / Documentación | Media |

# 8. Conclusión

El diseño electrónico está **eléctricamente sano y verificado**. El riesgo ya no está en el cableado, sino en la **alineación con los objetivos de la misión**: faltan o se sustituyeron los sensores que dan sentido al objetivo de "detección de incendios", no se ha verificado la masa contra el límite de 300 g, y falta el índice de calidad de comunicación.

La decisión más urgente es la **#1 (sensores de gas y material particulado)**, porque define si el hardware cumple su misión y porque cualquier cambio afecta la fabricación de la PCB con poco tiempo disponible.

---

*Documento técnico interno — Equipo IGNITIA, CanSat IGNIS-1 (LASC 2026). Preparado para revisión de dirección.*
