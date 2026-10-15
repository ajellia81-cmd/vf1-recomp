#!/bin/sh
# Saves VF2's textures as PNGs, from your own vf2.zip: runs the game headless through attract mode
# (the stages and fighters it shows), saving each texture the first time it's drawn.
#   tools/dump_textures.sh path/to/vf2.zip [outdir] [frames]
# outdir defaults to build/textures, frames to 12000 (attract mode's 3 1/2 minutes, about two to run).
# For everything, play instead: VF2_DUMP_TEXTURES=outdir build/bin/vf2 path/to/vf2.zip
# (src/machine/texdump.h says what's in the folder).
set -e
cd "$(dirname "$0")/.."
[ -n "$1" ] || { echo "usage: tools/dump_textures.sh path/to/vf2.zip [outdir] [frames]"; exit 2; }
ZIP=$(cd "$(dirname "$1")" && pwd)/$(basename "$1")
OUT=${2:-build/textures}
make -s build/bin/vf2run
VF2_DUMP_TEXTURES="$OUT" build/bin/vf2run "$ZIP" "${3:-12000}" >/dev/null
echo "$(ls "$OUT/color" | wc -l | tr -d ' ') textures in $OUT/color (as coloured in the game), $(ls "$OUT/raw" | wc -l | tr -d ' ') in $OUT/raw (grey), listed in $OUT/index.csv"
