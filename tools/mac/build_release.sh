#!/bin/sh
# The Mac release: vf-native for Apple silicon and Intel, macOS 11 and later, with SDL3 and FreeType
# built in (static, from build/deps: see tools/mac/deps.sh), joined into one universal program.
#   tools/mac/build_release.sh      ->  build/rel/vf-native (universal)
set -e
cd "$(dirname "$0")/../.."
DEPS=build/deps/mac
[ -f $DEPS/lib/libSDL3.a ] || tools/mac/deps.sh
FW="-Wl,-framework,CoreMedia -Wl,-framework,CoreVideo -Wl,-framework,Cocoa -Wl,-weak_framework,UniformTypeIdentifiers \
-Wl,-framework,IOKit -Wl,-framework,ForceFeedback -Wl,-framework,Carbon -Wl,-framework,CoreAudio -Wl,-framework,AudioToolbox \
-Wl,-framework,AVFoundation -Wl,-framework,Foundation -Wl,-framework,GameController -Wl,-framework,Metal \
-Wl,-framework,QuartzCore -Wl,-weak_framework,CoreHaptics"
for A in arm64 x86_64; do
	mkdir -p build/rel/$A/bin
	# the opcode-table generator runs here, on this Mac: use ours rather than one built for $A
	make -s build/bin/m68kmake
	cp -p build/bin/m68kmake build/rel/$A/bin/m68kmake
	make -s -j10 ARCH=$A OBJ=build/rel/$A/obj BIN=build/rel/$A/bin \
		CC="clang -arch $A -mmacosx-version-min=11.0" CXX="clang++ -arch $A -mmacosx-version-min=11.0" \
		SDL_CFLAGS="-I$DEPS/include" SDL_LIBS="$DEPS/lib/libSDL3.a $FW -liconv" \
		FT_CFLAGS="-I$DEPS/include/freetype2" FT_LIBS="$DEPS/lib/libfreetype.a" \
		build/rel/$A/bin/vf-native
done
lipo -create build/rel/arm64/bin/vf-native build/rel/x86_64/bin/vf-native -output build/rel/vf-native
lipo -archs build/rel/vf-native
