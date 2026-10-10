#!/usr/bin/env python3
"""One-off port of MAME's Model 1 TGP simulation (mame0210) and rasterizer (current) into
src/machine. Kept so the port can be redone against a newer MAME. Both sources are
BSD-3-Clause (copyright-holders: Olivier Galibert); their headers are kept."""
import re, sys
ROOT = sys.argv[1] if len(sys.argv) > 1 else '.'

def common(s):
    s = s.replace('model1_state::', 'model1::')
    s = re.sub(r'READ16_MEMBER\(\s*model1::(\w+)\s*\)', r'u16 model1::\1(offs_t offset, u16 mem_mask)', s)
    s = re.sub(r'WRITE16_MEMBER\(\s*model1::(\w+)\s*\)', r'void model1::\1(offs_t offset, u16 data, u16 mem_mask)', s)
    s = re.sub(r'READ32_MEMBER\(\s*model1::(\w+)\s*\)', r'u32 model1::\1(offs_t offset, u32 mem_mask)', s)
    s = re.sub(r'WRITE32_MEMBER\(\s*model1::(\w+)\s*\)', r'void model1::\1(offs_t offset, u32 data, u32 mem_mask)', s)
    s = s.replace('#include "emu.h"\n', '')
    s = re.sub(r'#include "(debugger|cpu/[^"]+|includes/model1|model1|corefloat)\.h"\n', '', s)
    s = s.replace('#include "logmacro.h"\n', '')
    s = s.replace('ARRAY_LENGTH', 'std::size')
    s = s.replace('machine().rand()', 'm_rand()')
    return '#include "model1.h"\n' + s

# --- TGP simulation (mame0210 machine/model1.cpp) ---
src = open(f'{ROOT}/refs/hle/tgp_hle_0210.cpp').read()
# keep: functions + ftab_vf + copro_hle_vf; v60-side copro handlers; table readers
a = src.index('const struct model1_state::function model1_state::ftab_swa[]')
b = src.index('void model1_state::copro_hle_vf()')
c = src.index('void model1_state::copro_hle_swa()')
d = src.index('READ16_MEMBER(model1_state::v60_copro_ram_adr_r)')
e = src.index('/* Coprocessor TGP memory map */')
f = src.index('WRITE32_MEMBER(model1_state::copro_ramadr_w)')
tgp = src[:a] + src[b:c] + src[d:e] + src[f:]
tgp = common(tgp)
tgp = tgp.replace('m_maincpu->pc()', 'm_maincpu.pc()')
tgp = tgp.replace('#define TGP_FUNCTION(name) void name()\n', '')
tgp = re.sub(r'logerror\(', 'M1_LOG(0, ', tgp)
open(f'{ROOT}/src/machine/model1_tgp.cpp', 'w').write(tgp)

# --- rasterizer (current src/mame/sega/model1_v.cpp) ---
v = open(f'{ROOT}/refs/mame-src/src/mame/sega/model1_v.cpp').read()
v = common(v)
v = re.sub(r'LOGMASKED\(', 'M1_LOG(', v)
v = v.replace('machine().pens[', 'm_pens[')
v = v.replace('m_poly_rom.target()', 'm_poly_rom')
# our screen_update replaces MAME's (the tile chip and screen device differ)
a = v.index('uint32_t model1::screen_update_model1(')
b = v.index('void model1::screen_vblank_model1(')
v = v[:a] + v[b:]
v = re.sub(r'\tsave_(pointer|item)\(NAME\([^;]*;\n', '', v)
v = v.replace('m_view = std::make_unique<model1::view_t>();', 'm_view = std::make_unique<view_t>();')
open(f'{ROOT}/src/machine/model1_video.cpp', 'w').write(v)
print('ported')
