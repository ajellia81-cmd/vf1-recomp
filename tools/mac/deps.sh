#!/bin/sh
# SDL3 and FreeType for the Mac release, from their release tarballs (checked against the SHA-256
# Homebrew uses), built static for Apple silicon and Intel, macOS 11 and later, into build/deps/mac.
set -e
cd "$(dirname "$0")/../.."
mkdir -p build/deps && cd build/deps
fetch() { [ -f "$2" ] || curl -sSL -o "$2" "$1"; echo "$3  $2" | shasum -a 256 -c -; }
fetch https://github.com/libsdl-org/SDL/releases/download/release-3.4.18/SDL3-3.4.18.tar.gz SDL3.tar.gz \
	9c75cf16330322c217dedd2e0609f1124f1b54b8633e763467b4684d0f4334a3
fetch https://downloads.sourceforge.net/project/freetype/freetype2/2.14.3/freetype-2.14.3.tar.xz freetype.tar.xz \
	36bc4f1cc413335368ee656c42afca65c5a3987e8768cc28cf11ba775e785a5f
tar xzf SDL3.tar.gz && tar xJf freetype.tar.xz
P=$PWD/mac
cmake -S SDL3-3.4.18 -B sdl-build -DCMAKE_OSX_ARCHITECTURES="arm64;x86_64" -DCMAKE_OSX_DEPLOYMENT_TARGET=11.0 \
	-DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX="$P" -DSDL_SHARED=OFF -DSDL_STATIC=ON -DSDL_TEST_LIBRARY=OFF -DSDL_TESTS=OFF -DSDL_EXAMPLES=OFF
cmake --build sdl-build -j10 && cmake --install sdl-build
cmake -S freetype-2.14.3 -B ft-build -DCMAKE_OSX_ARCHITECTURES="arm64;x86_64" -DCMAKE_OSX_DEPLOYMENT_TARGET=11.0 \
	-DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX="$P" -DBUILD_SHARED_LIBS=OFF \
	-DFT_DISABLE_ZLIB=ON -DFT_DISABLE_BZIP2=ON -DFT_DISABLE_PNG=ON -DFT_DISABLE_HARFBUZZ=ON -DFT_DISABLE_BROTLI=ON
cmake --build ft-build -j10 && cmake --install ft-build
