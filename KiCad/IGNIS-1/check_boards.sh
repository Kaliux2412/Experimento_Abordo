#!/usr/bin/env bash
# check_boards.sh — monitoreo de los 3 boards IGNIS-1 (ERC + DRC + netlist) con kicad-cli.
# Uso:  ./check_boards.sh            # los 3 boards
#       ./check_boards.sh cerebro    # solo uno
# Salida: resumen por board. Reportes completos en reports/.
# Exit 0 = todo limpio (0 errores ERC, 0 violaciones DRC). Exit 1 = hay errores.

set -uo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
BOARDS=("${@:-cerebro rf sensores}")
read -r -a BOARDS <<< "${BOARDS[*]}"
OUT="$ROOT/reports"
mkdir -p "$OUT"

if ! command -v kicad-cli >/dev/null 2>&1; then
  echo "ERROR: kicad-cli no encontrado en PATH." >&2; exit 2
fi

# colores (solo si TTY)
if [ -t 1 ]; then R=$'\e[31m'; G=$'\e[32m'; Y=$'\e[33m'; B=$'\e[1m'; Z=$'\e[0m'; else R= G= Y= B= Z=; fi

num() { grep -oE "$1" "$2" 2>/dev/null | grep -oE '[0-9]+' | head -1; }

fail=0
printf "%s%-10s %-18s %-26s%s\n" "$B" "BOARD" "ERC" "DRC" "$Z"
printf -- "---------- ------------------ --------------------------\n"

for b in "${BOARDS[@]}"; do
  sch="$ROOT/boards/$b/$b.kicad_sch"
  pcb="$ROOT/boards/$b/$b.kicad_pcb"
  if [ ! -f "$sch" ]; then printf "%s%-10s board no existe%s\n" "$R" "$b" "$Z"; fail=1; continue; fi

  # --- ERC ---
  ercrep="$OUT/$b.erc.rpt"
  kicad-cli sch erc -o "$ercrep" "$sch" >/dev/null 2>"$OUT/$b.erc.err"
  e_err=$(num 'Errors [0-9]+' "$ercrep");  e_err=${e_err:-?}
  e_warn=$(num 'Warnings [0-9]+' "$ercrep"); e_warn=${e_warn:-?}

  # --- netlist (verifica que el sch construye + guarda conectividad) ---
  kicad-cli sch export netlist --format kicadsexpr -o "$OUT/$b.net" "$sch" >/dev/null 2>"$OUT/$b.net.err" \
    && net_ok="ok" || net_ok="FALLO"

  # --- DRC ---
  drctxt="-"
  if [ -f "$pcb" ]; then
    drcrep="$OUT/$b.drc.rpt"
    kicad-cli pcb drc --exit-code-violations -o "$drcrep" "$pcb" >/dev/null 2>"$OUT/$b.drc.err"
    d_viol=$(num 'Found [0-9]+ DRC violations' "$drcrep");      d_viol=${d_viol:-0}
    d_unc=$(num 'Found [0-9]+ unconnected pads' "$drcrep");      d_unc=${d_unc:-0}
    drctxt="$d_viol viol / $d_unc sin conectar"
  else
    d_viol=0; d_unc=0; drctxt="(sin .kicad_pcb)"
  fi

  # color por estado
  if [ "$e_err" = "0" ]; then ec="$G$e_err err$Z"; else ec="$R$e_err err$Z"; fail=1; fi
   wc="$([ "${e_warn:-0}" = "0" ] && echo "$e_warn warn" || echo "$Y$e_warn warn$Z")"
  if [ "${d_viol:-0}" = "0" ] && [ "${d_unc:-0}" = "0" ]; then dc="$G$drctxt$Z"; else dc="$Y$drctxt$Z"; fi
  [ "$net_ok" = "FALLO" ] && { dc="$dc ${R}netlist FALLO$Z"; fail=1; }

  printf "%-10s %-27s %s\n" "$b" "$ec, $wc" "$dc"
done

echo
if [ "$fail" -eq 0 ]; then
  echo "${G}${B}ERC OK${Z} — 0 errores ERC y netlist construye en todos."
  echo "(DRC: ver columna arriba; viol/sin-conectar son esperados hasta rutear, no cuentan como fallo.)"
else
  echo "${R}${B}HAY FALLOS${Z} — errores ERC y/o netlist roto. Revisa reports/ (*.erc.rpt, *.err).${Z}"
fi
echo "Reportes: $OUT/"
echo "Nota: 'sin conectar' y violaciones DRC son normales hasta correr Update-from-Schematic (F8) + rutear."
exit $fail
