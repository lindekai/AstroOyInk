#!/usr/bin/env bash
# Prueft das Tischgehaeuse auf Ueberschneidungen: jedes Paar aus Druckteilen,
# Display und Elektronik wird geschnitten (teil="kollision"). Flaechenkontakt
# (0 mm³, z. B. Platine liegt auf Steg) ist gewollt, alles darueber nicht.
# Aufruf: hardware/gehaeuse/pruefe_kollision.sh   (OpenSCAD-Pfad per $OPENSCAD)
set -euo pipefail
cd "$(dirname "$0")"
OPENSCAD="${OPENSCAD:-/Applications/OpenSCAD-2021.01.app/Contents/MacOS/OpenSCAD}"
TMP="$(mktemp -d)"
trap 'rm -rf "$TMP"' EXIT

GRUPPEN=(sockel boden panel drv lipo ada tc buck bme)
fehler=0
for ((i = 0; i < ${#GRUPPEN[@]}; i++)); do
  for ((j = i + 1; j < ${#GRUPPEN[@]}; j++)); do
    a=${GRUPPEN[i]}; b=${GRUPPEN[j]}
    out=$("$OPENSCAD" -D "teil=\"kollision\"" -D "gruppe=\"$a\"" -D "gruppe2=\"$b\"" \
          -o "$TMP/k.stl" gehaeuse_tisch.scad 2>&1 || true)
    if echo "$out" | grep -qi "empty"; then continue; fi
    erg=$(python3 - "$TMP/k.stl" <<'PY'
import re, sys
v = [tuple(map(float, m)) for m in re.findall(r"vertex\s+(\S+)\s+(\S+)\s+(\S+)", open(sys.argv[1]).read())]
vol = 0.0
for i in range(0, len(v), 3):
    a, b, c = v[i:i + 3]
    vol += (a[0] * (b[1] * c[2] - b[2] * c[1]) - a[1] * (b[0] * c[2] - b[2] * c[0])
            + a[2] * (b[0] * c[1] - b[1] * c[0])) / 6
xs, ys, zs = zip(*v)
print(f"{abs(vol):.1f} x {min(xs):.1f}..{max(xs):.1f} y {min(ys):.1f}..{max(ys):.1f} z {min(zs):.1f}..{max(zs):.1f}")
PY
)
    vol=${erg%% *}
    if [ "$vol" = "0.0" ]; then continue; fi
    echo "[KOLLISION] $a / $b: ${vol} mm³ bei ${erg#* }"
    fehler=1
  done
done
if [ $fehler -eq 0 ]; then echo "[OK] keine Ueberschneidungen"; else exit 1; fi
