# Reproducible iOS dependencies

Run `sh ios/Scripts/prepare-dependencies.sh` on a macOS cloud runner with Xcode,
CMake 3.24+ and Git. The script fetches **source code only**, at the exact commits
in `versions.env` (SDL 3.4.2 and FreeType 2.13.3), and builds static arm64 libraries
for iPhoneOS and iPhoneSimulator. It does not build this app or fetch game data.

Generated `SDL3.xcframework` and `FreeType.xcframework` stay ignored. Static
XCFrameworks are linked, never embedded as dynamic frameworks. The SDL headers
use `Headers/SDL3/`; FreeType exposes `ft2build.h` and `freetype/` at `Headers/`.
SDK-specific header search paths are recorded in `Config/Debug.xcconfig`.

FreeType's optional compressed-font/image/shaping dependencies are disabled;
the bundled TTF menu fonts need none of them. The app links SDK zlib for its
existing ZIP reader. SDL OpenGL ES and HIDAPI are disabled; Metal, UIKit and
GameController are retained. Source licenses are copied to `ios/build/licenses/`.
Include those, FreeType's referenced `docs/FTL.TXT`/`docs/GPLv2.TXT`, and the
repository's third-party notices in any later personal packaging.

No ROM, extracted data, generated game code, IPA, signing key, or provisioning
profile belongs in this directory. Dependencies may be rebuilt without the
user's computer or signing credentials. Their macOS/iOS build is unverified
until the cloud compilation check completes.
