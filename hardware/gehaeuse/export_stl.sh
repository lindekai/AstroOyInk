#!/usr/bin/env bash
# Exportiert alle Druckteile beider Gehaeuse als STL nach hardware/gehaeuse/stl/
# Aufruf: hardware/gehaeuse/export_stl.sh   (OpenSCAD-Pfad per $OPENSCAD)
set -euo pipefail
cd "$(dirname "$0")"
OPENSCAD="${OPENSCAD:-/Applications/OpenSCAD-2021.01.app/Contents/MacOS/OpenSCAD}"
mkdir -p stl

teil() {  # $1 = scad-Datei, $2 = Teil, $3 = Ausgabename
  echo "[STL] $3"
  "$OPENSCAD" -q -D "teil=\"$2\"" -o "stl/$3.stl" "$1"
}

teil gehaeuse_tisch.scad sockel tisch_sockel
teil gehaeuse_tisch.scad boden  tisch_boden
teil gehaeuse_wand.scad  rahmen wand_rahmen
teil gehaeuse_wand.scad  deckel wand_deckel
echo "[STL] fertig: $(ls stl/*.stl | wc -l | tr -d ' ') Dateien"
