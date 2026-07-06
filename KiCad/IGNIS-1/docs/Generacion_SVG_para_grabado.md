# IGNIS-1 — Generación de SVGs para grabado/trazado de cobre

**Fecha:** 2026-07-05
**Objetivo:** exportar las capas de cobre de los 5 boards a SVG 1:1 para alimentar la máquina que traza/graba el cobre en las placas.
**Estado:** SVGs generados en `fab_svg/` (10 archivos = 5 boards × 2 caras). **Falta confirmar con el operador** los puntos marcados ⚠️ antes de mandar a producción.
**Herramienta:** `kicad-cli` 10.0.4 (viene con KiCad). Inkscape solo para verificar/ajustar — ver abajo.

---

## Comandos usados

Un par de archivos por board: cara superior (F.Cu) + cara inferior (B.Cu espejada).

### Cara superior (F.Cu)
```bash
kicad-cli pcb export svg --layers F.Cu,Edge.Cuts --mode-single \
  --black-and-white --exclude-drawing-sheet --page-size-mode 2 \
  --scale 1 --check-zones --drill-shape-opt 2 \
  -o fab_svg/<board>_top.svg  boards/<board>/<board>.kicad_pcb
```

### Cara inferior (B.Cu) — ESPEJADA
```bash
kicad-cli pcb export svg --layers B.Cu,Edge.Cuts --mode-single --mirror \
  --black-and-white --exclude-drawing-sheet --page-size-mode 2 \
  --scale 1 --check-zones --drill-shape-opt 2 \
  -o fab_svg/<board>_bot_mirror.svg  boards/<board>/<board>.kicad_pcb
```

### Loop para los 5 boards
```bash
cd KiCad/IGNIS-1
mkdir -p fab_svg
for b in cerebro rf carga sensores energia; do
  kicad-cli pcb export svg --layers F.Cu,Edge.Cuts --mode-single \
    --black-and-white --exclude-drawing-sheet --page-size-mode 2 \
    --scale 1 --check-zones --drill-shape-opt 2 \
    -o "fab_svg/${b}_top.svg" "boards/$b/$b.kicad_pcb"
  kicad-cli pcb export svg --layers B.Cu,Edge.Cuts --mode-single --mirror \
    --black-and-white --exclude-drawing-sheet --page-size-mode 2 \
    --scale 1 --check-zones --drill-shape-opt 2 \
    -o "fab_svg/${b}_bot_mirror.svg" "boards/$b/$b.kicad_pcb"
done
```

---

## Flags — qué hace cada uno y por qué

| flag | efecto | por qué |
|------|--------|---------|
| `--layers F.Cu,Edge.Cuts` | qué capas van al SVG | cobre + contorno de placa. Una cara por archivo. |
| `--mode-single` | 1 archivo = la ruta `-o` exacta | sin esto genera nombres automáticos en un directorio. |
| `--mirror` | espeja el dibujo | **solo bottom**, para que alinee al voltear la placa. |
| `--scale 1` | escala 1:1 | **crítico.** El SVG queda en mm reales. `0` = autoescala (NO usar). |
| `--black-and-white` | negro/blanco puro | sin grises que confundan a la máquina. |
| `--check-zones` | rellena los pours antes de exportar | los `.kicad_pcb` son generados por script; la zona GND podría no estar rellena. Esto la rellena. |
| `--exclude-drawing-sheet` | sin marco/carátula | solo la placa. |
| `--page-size-mode 2` | recorta al área de la placa | sin margen de hoja A4. |
| `--drill-shape-opt 2` | dibuja el agujero real en pads/vías | deja ver la posición de taladros (anillo de cobre). |
| `--negative` | **NO usado** | invierte: cobre = blanco. Ver punto ⚠️2. |

---

## Archivos generados (`fab_svg/`)

```
cerebro_top.svg    cerebro_bot_mirror.svg
rf_top.svg         rf_bot_mirror.svg
carga_top.svg      carga_bot_mirror.svg
sensores_top.svg   sensores_bot_mirror.svg
energia_top.svg    energia_bot_mirror.svg
```

- **Positivo:** cobre = NEGRO, resto = blanco.
- **Bottom espejado** (`_bot_mirror`): asume que volteas la placa y grabas desde arriba.
- **1:1 en mm reales.** Placa circular ⌀ ≈ 62 mm (verificado: `width="62.0014mm"` en el SVG de cerebro).
- Los 4 agujeros de montaje (H1–H4) salen en el SVG → sirven de fiduciales para registrar las 2 caras.

---

## ⚠️ Confirmar con el operador ANTES de mandar

Un SVG mal interpretado arruina la placa. Cinco puntos:

### 1. Escala / unidades — el fallo #1
El SVG trae mm reales (`width="62mm"`). Pero muchos programas lo importan como px @96 o @72 dpi → sale a escala equivocada.
- **Preguntar:** ¿lee mm del SVG, o asume DPI?
- **Verificar tú:** abrir en Inkscape → seleccionar todo → la caja W/H debe marcar el tamaño real (cerebro = 62 mm). Si marca otra cosa, ahí está el problema.

### 2. Positivo o negativo
Ahora: **cobre = negro**.
- **Preguntar:** ¿lo negro es cobre que se QUEDA, o zona que se REMUEVE?
- Si es al revés → regenerar con `--negative`.

### 3. Trazo (centerline) vs relleno — gotcha grande
El SVG de KiCad da el cobre como **áreas rellenas** (contornos de pistas/pads).
- **Preguntar:** ¿la máquina sigue el CENTRO del trazo (pluma/plotter) o RELLENA áreas (láser/fresa/UV)?
- Si es **centerline**, el relleno está MAL: necesita paths de línea. Convertir en Inkscape (Path → Trace/Centerline) o cambiar de flujo.

### 4. Espejo de la cara inferior
Te dimos `_bot_mirror.svg` espejado (asume voltear la placa y grabar desde arriba).
- **Preguntar:** ¿graba la cara de cobre mirándola directo, o volteando la placa?
- Si graba directo → el bottom va **SIN** espejo (regenerar sin `--mirror`).

### 5. Registro entre caras
Para 2 caras: ¿cómo alinea una cara con la otra?
- Normalmente con los agujeros de montaje H1–H4 como fiduciales (ya están en el SVG).

---

## Rol de Inkscape

**No es web service.** Es editor vectorial de escritorio, libre (Linux/Win/Mac).

```bash
sudo pacman -S inkscape     # correr en otra terminal (no sudo desde Claude)
inkscape fab_svg/cerebro_top.svg
```

Para qué sirve aquí:
- **Verificar escala** (punto ⚠️1) — abrir, medir, confirmar mm reales.
- **Revisar** que se ve completo, sin capas raras.
- **Convertir a centerline** si la máquina lo pide (punto ⚠️3).
- **Generar G-code** si la máquina es CNC → extensión **Gcodetools**.

No es obligatorio para *generar* los SVG (eso lo hace kicad-cli). Solo para verificar/ajustar/convertir.

---

## Flujos alternativos según la máquina

| La máquina come… | Flujo recomendado |
|------------------|-------------------|
| **SVG directo** (láser / plotter / UV) | Lo de este doc. Ajustar positivo/negativo y espejo según operador. |
| **G-code** (CNC fresadora de aislamiento) | **Estándar:** Gerber → **FlatCAM** → G-code (más robusto, menos errores de escala). SVG + Inkscape + Gcodetools es plan B, más manual. |
| **Gerber** (fotoplotter / fab profesional) | Exportar Gerbers + Excellon estándar (`kicad-cli pcb export gerbers` + `export drill`), NO SVG. |

**Aviso clave:** el SVG lleva el cobre como formas rellenas (las pistas). Para *fresado de aislamiento* (la fresa corta ALREDEDOR de las pistas) la máquina necesita el **toolpath de aislamiento**, que NO está en el SVG — lo calcula FlatCAM/Gcodetools a partir del cobre. Por eso, si es CNC, el flujo Gerber→FlatCAM suele ser mejor que SVG.

---

## Estado actual y siguiente paso

- ✅ SVGs generados, positivos, bottom espejado, 1:1 → `fab_svg/`.
- ⏳ **Pendiente:** respuestas del operador a los puntos ⚠️1, 2 y 3.
- Según respuestas: regenerar con `--negative` y/o sin `--mirror`, o convertir a centerline / cambiar a flujo Gerber.
