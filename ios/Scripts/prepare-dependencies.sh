#!/bin/sh
# Build only redistributable static libraries, never the app or an IPA.
set -eu
root=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)
. "$root/Dependencies/versions.env"
[ "$(uname -s)" = Darwin ] || { echo 'Requires a macOS runner with Xcode.' >&2; exit 1; }
for tool in git cmake xcodebuild xcrun; do command -v "$tool" >/dev/null; done
work="$root/build/dependencies"
mkdir -p "$work"
checkout() {
    name=$1 url=$2 revision=$3
    if [ ! -d "$work/$name/.git" ]; then git init "$work/$name"; fi
    git -C "$work/$name" fetch --depth 1 "$url" "$revision"
    git -C "$work/$name" checkout --detach FETCH_HEAD
    [ "$(git -C "$work/$name" rev-parse HEAD)" = "$revision" ]
}
checkout SDL https://github.com/libsdl-org/SDL.git "$SDL_REVISION"
checkout FreeType https://github.com/freetype/freetype.git "$FREETYPE_REVISION"
for sdk in iphoneos iphonesimulator; do
    prefix="$work/install-$sdk"
    sysroot=$(xcrun --sdk "$sdk" --show-sdk-path)
    cmake -S "$work/SDL" -B "$work/sdl-$sdk" -G Xcode \
        -DCMAKE_SYSTEM_NAME=iOS -DCMAKE_OSX_SYSROOT="$sysroot" \
        -DCMAKE_OSX_ARCHITECTURES=arm64 -DCMAKE_OSX_DEPLOYMENT_TARGET=15.0 \
        -DCMAKE_INSTALL_PREFIX="$prefix" -DSDL_SHARED=OFF -DSDL_STATIC=ON \
        -DSDL_FRAMEWORK=OFF -DSDL_TESTS=OFF -DSDL_EXAMPLES=OFF \
        -DSDL_OPENGLES=OFF -DSDL_HIDAPI=OFF -DSDL_INSTALL_TESTS=OFF
    cmake --build "$work/sdl-$sdk" --config Release --parallel 3 -- CODE_SIGNING_ALLOWED=NO
    cmake --install "$work/sdl-$sdk" --config Release
    cmake -S "$work/FreeType" -B "$work/ft-$sdk" -G Xcode \
        -DCMAKE_SYSTEM_NAME=iOS -DCMAKE_OSX_SYSROOT="$sysroot" \
        -DCMAKE_OSX_ARCHITECTURES=arm64 -DCMAKE_OSX_DEPLOYMENT_TARGET=15.0 \
        -DCMAKE_INSTALL_PREFIX="$prefix" -DBUILD_SHARED_LIBS=OFF \
        -DFT_DISABLE_ZLIB=ON -DFT_DISABLE_BZIP2=ON -DFT_DISABLE_PNG=ON \
        -DFT_DISABLE_HARFBUZZ=ON -DFT_DISABLE_BROTLI=ON
    cmake --build "$work/ft-$sdk" --config Release --parallel 3 -- CODE_SIGNING_ALLOWED=NO
    cmake --install "$work/ft-$sdk" --config Release
 done
# Replace only generated dependency packages, never any player data.
rm -rf "$root/Dependencies/SDL3.xcframework" "$root/Dependencies/FreeType.xcframework"
xcodebuild -create-xcframework \
    -library "$work/install-iphoneos/lib/libSDL3.a" -headers "$work/install-iphoneos/include" \
    -library "$work/install-iphonesimulator/lib/libSDL3.a" -headers "$work/install-iphonesimulator/include" \
    -output "$root/Dependencies/SDL3.xcframework"
xcodebuild -create-xcframework \
    -library "$work/install-iphoneos/lib/libfreetype.a" -headers "$work/install-iphoneos/include/freetype2" \
    -library "$work/install-iphonesimulator/lib/libfreetype.a" -headers "$work/install-iphonesimulator/include/freetype2" \
    -output "$root/Dependencies/FreeType.xcframework"
# Preserve the exact licenses beside the generated libraries for later personal packaging.
mkdir -p "$root/build/licenses"
cp "$work/SDL/LICENSE.txt" "$root/build/licenses/SDL.txt"
cp "$work/FreeType/LICENSE.TXT" "$root/build/licenses/FreeType.txt"
cp "$work/FreeType/docs/FTL.TXT" "$work/FreeType/docs/GPLv2.TXT" "$root/build/licenses/"
printf '%s\n' 'Static XCFramework dependencies prepared. No app or IPA was built.'
