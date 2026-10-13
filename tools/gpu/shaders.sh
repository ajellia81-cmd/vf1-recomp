#!/bin/bash
# The GPU's 3D shaders for SDL_GPU (src/gpu/gpu3d.hlsl), compiled with Microsoft's DXC into SPIR-V
# (Vulkan: Linux, the Steam Deck, Windows without Direct3D 12) and DXIL (Direct3D 12: Windows),
# into src/gpu/gpu3d_shaders.h, which is committed: the builds don't need DXC. Run after changing
# gpu3d.hlsl.
#   DXC=/path/to/dxc tools/gpu/shaders.sh     (DXC's Linux release has libdxil.so next to it, which
#                                             signs the DXIL as Direct3D 12 wants it)
# Without DXC here (a Mac): DXC_LINUX=dir/with/bin/dxc runs it in an x86-64 Linux container.
set -e
cd "$(dirname "$0")/../.."
OUT=build/shaders
mkdir -p $OUT
KERNELS="clear rows solid shadow pixels"
FLAGS="-T cs_6_0 -E main -HV 2021 -O3"   # (-Gis, IEEE strictness, for DXIL: SPIR-V's would need Vulkan 1.2; `precise` does it there)
if [ -z "$DXC" ] && [ -n "$DXC_LINUX" ]; then
	docker run --rm --platform linux/amd64 -v "$PWD":/src -v "$DXC_LINUX":/dxc -w /src -e LD_LIBRARY_PATH=/dxc/lib ubuntu:24.04 \
		bash -c "for k in $KERNELS; do K=\$(echo \$k | tr a-z A-Z); \
			/dxc/bin/dxc $FLAGS -D K_\$K -spirv -fspv-target-env=vulkan1.0 -Fo $OUT/\$k.spv src/gpu/gpu3d.hlsl || exit 1; \
			/dxc/bin/dxc $FLAGS -D K_\$K -Gis -Fo $OUT/\$k.dxil src/gpu/gpu3d.hlsl || exit 1; done"
else
	DXC=${DXC:-dxc}
	for k in $KERNELS; do
		K=$(echo $k | tr a-z A-Z)
		$DXC $FLAGS -D K_$K -spirv -fspv-target-env=vulkan1.0 -Fo $OUT/$k.spv src/gpu/gpu3d.hlsl
		$DXC $FLAGS -D K_$K -Gis -Fo $OUT/$k.dxil src/gpu/gpu3d.hlsl
	done
fi
python3 - "$OUT" $KERNELS > src/gpu/gpu3d_shaders.h <<'PY'
import sys, hashlib
out, kernels = sys.argv[1], sys.argv[2:]
print("// Made by tools/gpu/shaders.sh from src/gpu/gpu3d.hlsl (DXC): don't edit.")
print("#pragma once")
print("#include <cstddef>")
print("#include <cstdint>")
for k in kernels:
    for ext in ("spv", "dxil"):
        data = open(f"{out}/{k}.{ext}", "rb").read()
        print(f"alignas(4) static const uint8_t gpu3d_{k}_{ext}[{len(data)}] = {{")
        for i in range(0, len(data), 24):
            print("\t" + ",".join(str(b) for b in data[i:i + 24]) + ",")
        print("};")
src = open("src/gpu/gpu3d.hlsl", "rb").read()
print(f'static const char gpu3d_shaders_source_sha1[] = "{hashlib.sha1(src).hexdigest()}";')
PY
ls -la src/gpu/gpu3d_shaders.h
