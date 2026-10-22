#!/bin/sh
# Read-only preflight for a Mac before generating or archiving. It intentionally does not build.
set -eu

root=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)
repo=$(CDPATH= cd -- "$root/.." && pwd)
missing=0

need() {
	if [ ! -e "$1" ]; then
		printf 'missing: %s\n' "$2" >&2
		missing=1
	fi
}

command -v xcodegen >/dev/null 2>&1 || { printf 'missing: XcodeGen\n' >&2; missing=1; }
command -v xcodebuild >/dev/null 2>&1 || { printf 'missing: Xcode command-line tools\n' >&2; missing=1; }
need "$root/Dependencies/SDL3.xcframework" "ios/Dependencies/SDL3.xcframework"
need "$root/Dependencies/FreeType.xcframework" "ios/Dependencies/FreeType.xcframework"
need "$repo/src/rt/rec_table_none.cpp" "interpreter-only native stub"

if [ "$missing" -ne 0 ]; then
	printf '%s\n' 'Preflight failed; no project was generated and no build was run.' >&2
	exit 1
fi

xcrun --sdk iphoneos --show-sdk-path >/dev/null
xcrun --sdk iphonesimulator --show-sdk-path >/dev/null
python3 - "$root" <<'PYDEPS'
import pathlib, plistlib, sys
root = pathlib.Path(sys.argv[1])
for name, header in [('SDL3', 'SDL3/SDL.h'), ('FreeType', 'ft2build.h')]:
    package = root / 'Dependencies' / (name + '.xcframework')
    info = plistlib.loads((package / 'Info.plist').read_bytes())
    slices = {item['LibraryIdentifier']: item for item in info['AvailableLibraries']}
    for identifier in ['ios-arm64', 'ios-arm64-simulator']:
        item = slices[identifier]
        assert item['SupportedPlatform'] == 'ios'
        assert item['SupportedArchitectures'] == ['arm64']
        assert (package / identifier / item['LibraryPath']).is_file()
        assert (package / identifier / item['HeadersPath'] / header).is_file()
print('arm64 iPhone/simulator static libraries and public headers: OK')
PYDEPS

printf '%s\n' 'Preflight passed. Next: set PRODUCT_BUNDLE_IDENTIFIER and DEVELOPMENT_TEAM, then run xcodegen generate.'
