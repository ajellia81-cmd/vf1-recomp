#!/usr/bin/env python3
"""Inspect XcodeGen output, without compiling, signing or accessing any ROM."""
import json
import pathlib
import subprocess

root = pathlib.Path(__file__).resolve().parents[1]
project = root / 'VF1Recomp.xcodeproj' / 'project.pbxproj'
data = json.loads(subprocess.check_output(['plutil', '-convert', 'json', '-o', '-', str(project)]))
objects = data['objects']
app = next(v for v in objects.values() if v.get('isa') == 'PBXNativeTarget' and v['name'] == 'VF1Recomp')
phases = [objects[key] for key in app['buildPhases']]
def files(kind):
    result = []
    for phase in phases:
        if phase['isa'] == kind:
            for key in phase.get('files', []):
                ref = objects[objects[key]['fileRef']]
                result.append(pathlib.PurePosixPath(ref.get('path', ref.get('name', ''))).name)
    return result
sources = files('PBXSourcesBuildPhase')
for required in ['main.cpp', 'v60_device.cpp', 'rec_table_none.cpp', 'm68kops.c', 'coro_arm64.S',
                 'VF1Platform.mm', 'VF1ROMImport.mm', 'VF1InputBridge.mm', 'gpu3d_metal.mm']:
    assert sources.count(required) == 1, (required, sources.count(required))
for forbidden in ['native.cpp', 'sljitLir.c', 'analyse.cpp', 'coro_x86_64.S', 'coro_win64.S', 'gpu3d_sdlgpu.cpp', 'gpu3d.hlsl', 'CMakeLists.txt']:
    assert forbidden not in sources, forbidden
assert 'assets' in files('PBXResourcesBuildPhase'), 'Missing bundled menu/fonts/backgrounds'
for name in ['Debug.xcconfig', 'Release.xcconfig']:
    assert any(v.get('path', '').endswith(name) for v in objects.values()), name
assert 'VF1_DISABLE_JIT=1' in (root / 'Config/Debug.xcconfig').read_text()
assert 'Debug.xcconfig' in (root / 'Config/Release.xcconfig').read_text()
print('Generated source selection, resource copy and interpreter configuration: OK')

for config in (root / 'Config').glob('*.xcconfig'):
    for number, line in enumerate(config.read_text().splitlines(), 1):
        assert not line.lstrip().startswith('#') or line.lstrip().startswith('#include '), (config.name, number)
