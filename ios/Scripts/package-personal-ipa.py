#!/usr/bin/env python3
"""Package an unsigned device build for personal re-signing. Never accepts ROMs."""
import hashlib
import json
import plistlib
import shutil
import subprocess
import zipfile
from pathlib import Path

root = Path(__file__).resolve().parents[2]
build = root / 'ios/build'
app = build / 'device/Build/Products/Release-iphoneos/Virtua Fighter Recomp.app'
with (app / 'Info.plist').open('rb') as f:
    info = plistlib.load(f)
assert info['CFBundleIdentifier'] == 'com.ajellia81.vf1recomp'
assert info['CFBundleSupportedPlatforms'] == ['iPhoneOS']
assert info.get('CADisableMinimumFrameDurationOnPhone') is True
assert info['CFBundleDisplayName'] == 'Virtua Fighter'
assert info['CFBundleVersion'] == '7'
assert info['CFBundleIcons']['CFBundlePrimaryIcon']['CFBundleIconName'] == 'AppIcon'
assert (app / 'Assets.car').is_file()
exe = app / info['CFBundleExecutable']
assert subprocess.check_output(['xcrun', 'lipo', '-archs', str(exe)], text=True).strip() == 'arm64'
symbols = subprocess.check_output(['xcrun', 'nm', str(exe)], text=True)
assert 'sljit_' not in symbols, 'JIT symbols must never enter the iOS package'

# Ensure the shipped executable has no diagnostic UI, CSV export or recorders.
binary = exe.read_bytes()
for marker in (b'Save hit diagnostics', b'Record sound diagnostics', b'Test vibration',
               b'haptics-hit-diagnostics.csv', b'haptics-sound-diagnostics.csv'):
    assert marker not in binary, f'Diagnostic feature remains: {marker!r}'
assert 'combat_probe' not in symbols and 'combat_sound_probe' not in symbols

for file in app.rglob('*'):
    assert file.suffix.lower() not in {'.zip', '.bin', '.rom', '.ipa'}, f'Unexpected game/archive data: {file}'
    assert file.name != 'embedded.mobileprovision', 'Personal signing happens after download'

out = build / 'personal'
# Only replace this script's generated staging directory.
if out.exists():
    shutil.rmtree(out)
staged = out / 'Payload' / app.name
shutil.copytree(app, staged, symlinks=True)
for signature in staged.rglob('_CodeSignature'):
    shutil.rmtree(signature)
notices = []
def include(title, text):
    notices.append('\n\n' + '=' * 72 + '\n' + title + '\n' + '=' * 72 + '\n' + text)
include('Virtua Fighter Recomp (MIT)', (root / 'LICENSE').read_text())
# Reuse the maintained MAME notice without executing the desktop dependency downloader.
release_licenses = (root / 'tools/release/licenses.sh').read_text()
include('ymfm', (root / 'third_party/ymfm/LICENSE').read_text())
include('Z80', (root / 'third_party/z80/LICENSE').read_text())
include('GLM', (root / 'third_party/glm/copying.txt').read_text())
musashi = (root / 'third_party/musashi/readme.txt').read_text()
include('Musashi', musashi[musashi.index('LICENSE AND COPYRIGHT'):])
include('MAME (BSD-3-Clause)', release_licenses.split("cat <<'BSD'\n", 1)[1].split('\nBSD\n', 1)[0])
for title, name in [('RmlUi (MIT)', 'third_party/rmlui/LICENSE.txt'),
                    ('robin_hood and itlib (MIT)', 'third_party/rmlui/Include/RmlUi/Core/Containers/LICENSE.txt'),
                    ('Barlow Condensed (SIL OFL 1.1)', 'assets/fonts/OFL.txt')]:
    include(title, (root / name).read_text())
stb = (root / 'third_party/stb/stb_image.h').read_text()
include('stb_image (MIT option)', stb.split('ALTERNATIVE A - MIT License', 1)[1].split('ALTERNATIVE B', 1)[0])
for name in ('SDL.txt', 'FreeType.txt', 'FTL.TXT'):
    include(name, (build / 'licenses' / name).read_text())
include('FreeType attribution', 'Portions of this software are copyright (c) The FreeType Project (www.freetype.org). All rights reserved.')
sdk = Path(subprocess.check_output(['xcrun', '--sdk', 'iphoneos', '--show-sdk-path'], text=True).strip())
zlib_header = (sdk / 'usr/include/zlib.h').read_text()
include('System zlib', zlib_header.split('/*', 1)[1].split('*/', 1)[0])
(staged / 'LICENSES.txt').write_text('Third-party notices for the personal iOS build.\n' + ''.join(notices))
sha = subprocess.check_output(['git', 'rev-parse', 'HEAD'], cwd=root, text=True).strip()
(staged / 'BUILD-INFO.json').write_text(json.dumps({
    'source_commit': sha, 'execution': 'interpreter-only', 'presentation_target': [2868,1320], 'native_output_geometry_scale': 4, 'selectable_integer_scales': [1, 2, 3, 4, 5, 6], 'brightness_range_percent': [50, 200],
    'signed': False, 'roms_included': False, 'purpose': 'personal use; re-sign before installation',
    'gameplay_device_tested': False, 'diagnostic_features': False,
}, indent=2) + '\n')
ipa = out / 'VirtuaFighter-build7-clean-unsigned.ipa'
subprocess.run(['ditto', '-c', '-k', '--sequesterRsrc', '--keepParent', 'Payload', ipa.name], cwd=out, check=True)
with zipfile.ZipFile(ipa) as z:
    assert z.testzip() is None
    assert f'Payload/{app.name}/Info.plist' in z.namelist()
    assert f'Payload/{app.name}/{exe.name}' in z.namelist()
print(f'Personal unsigned IPA: {ipa.name}; source: {sha}')
print(f'SHA-256: {hashlib.sha256(ipa.read_bytes()).hexdigest()}')
