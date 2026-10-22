#!/bin/sh
set -eu
cd "$(dirname "$0")/../.."
mkdir -p build/musashi ios/Generated
cc -O1 -w third_party/musashi/m68kmake.c -o build/musashi/m68kmake
build/musashi/m68kmake build/musashi third_party/musashi/m68k_in.c >/dev/null
printf '#define VF_VERSION "ios-port"\n#define VF_DATE ""\n' > ios/Generated/version.h
