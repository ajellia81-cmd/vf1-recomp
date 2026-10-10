#!/usr/bin/env python3
"""Build the Model 1 memory regions for Virtua Fighter from the user's own MAME `vf` set.

Reads vf.zip, checks every file's CRC32 and writes the regions MAME's driver builds
(src/mame/sega/model1.cpp, ROM_START(vf)) to build/vf/<region>.bin. The output is ROM data:
it stays in build/, which git ignores.

    tools/romset.py roms/vf.zip [build/vf] [roms/model1io.zip ...]

Optional, from a current MAME set: 315-5724.bin (the TGP's own program, CRC 4b4f330e) is
written to build/vf/tgp_program.bin and the runtime then runs the real TGP code.
"""
import os, sys, zipfile, zlib

# region name -> (size, fill)
REGIONS = {
    'maincpu':      (0x2000000, 0xff),
    'copro_tables': (0x40000, 0x00),
    'other_data':   (0x80000, 0x00),
    'other_other_data': (0x20000, 0x00),
    'sndcpu':       (0xc0000, 0x00),
    'pcm1':         (0x400000, 0x00),
    'pcm2':         (0x400000, 0x00),
    'polygons':     (0x1000000, 0x00),
}

# (file, crc, region, offset, kind). kind: 'raw', 'byte16' (every other byte),
# 'word32' (16-bit words into every other 32-bit slot), 'wordswap' (16-bit byteswapped)
ROMS = [
    ('opr14742.bin', 0x446a1085, 'copro_tables', 0x0, 'word32'),
    ('opr14743.bin', 0xe8953554, 'copro_tables', 0x2, 'word32'),
    ('opr14744.bin', 0x730ea9e0, 'other_data', 0x0, 'word32'),
    ('opr14745.bin', 0x4c934d96, 'other_data', 0x2, 'word32'),
    ('opr14746.bin', 0x2a266cbd, 'other_data', 0x40000, 'word32'),
    ('opr14747.bin', 0xa4ad5e19, 'other_data', 0x40002, 'word32'),
    ('opr14748.bin', 0x4a532cb8, 'other_other_data', 0x0, 'raw'),
    ('epr-16082.14', 0xb23f22ee, 'maincpu', 0x200000, 'byte16'),
    ('epr-16083.15', 0xd12c77f8, 'maincpu', 0x200001, 'byte16'),
    ('epr-16080.4',  0x3662e1a5, 'maincpu', 0xfc0000, 'raw'),
    ('epr-16081.5',  0x6dec06ce, 'maincpu', 0xfe0000, 'raw'),
    ('mpr-16084.6',  0x483f453b, 'maincpu', 0x1000000, 'byte16'),
    ('mpr-16085.7',  0x5fa01277, 'maincpu', 0x1000001, 'byte16'),
    ('mpr-16086.8',  0xdeac47a1, 'maincpu', 0x1100000, 'byte16'),
    ('mpr-16087.9',  0x7a64daac, 'maincpu', 0x1100001, 'byte16'),
    ('mpr-16088.10', 0xfcda2d1e, 'maincpu', 0x1200000, 'byte16'),
    ('mpr-16089.11', 0x39befbe0, 'maincpu', 0x1200001, 'byte16'),
    ('mpr-16090.12', 0x90c76831, 'maincpu', 0x1300000, 'byte16'),
    ('mpr-16091.13', 0x53115448, 'maincpu', 0x1300001, 'byte16'),
    ('epr-16120.7',  0x2bff8378, 'sndcpu', 0x00000, 'wordswap'),
    ('epr-16121.8',  0xff6723f9, 'sndcpu', 0x20000, 'wordswap'),
    ('epr-16121.8',  0xff6723f9, 'sndcpu', 0x80000, 'wordswap'),   # ROM_RELOAD
    ('mpr-16122.32', 0x568bc64e, 'pcm1', 0x000000, 'raw'),
    ('mpr-16123.33', 0x15d78844, 'pcm1', 0x200000, 'raw'),
    ('mpr-16124.4',  0x45520ba1, 'pcm2', 0x000000, 'raw'),
    ('mpr-16125.5',  0x9b4998b6, 'pcm2', 0x200000, 'raw'),
    ('mpr-16096.26', 0xa92b0bf3, 'polygons', 0x000000, 'word32'),
    ('mpr-16097.27', 0x0232955a, 'polygons', 0x000002, 'word32'),
    ('mpr-16098.28', 0xcf2e1b84, 'polygons', 0x400000, 'word32'),
    ('mpr-16099.29', 0x20e46854, 'polygons', 0x400002, 'word32'),
    ('mpr-16100.30', 0xe13e983d, 'polygons', 0x800000, 'word32'),
    ('mpr-16101.31', 0x0dbed94d, 'polygons', 0x800002, 'word32'),
    ('mpr-16102.32', 0x4cb41fb6, 'polygons', 0xc00000, 'word32'),
    ('mpr-16103.33', 0x526d1c76, 'polygons', 0xc00002, 'word32'),
]

# Some sets name the board ROMs with a dash and the IC number.
ALIASES = {'opr14744.bin': 'opr-14744.58', 'opr14745.bin': 'opr-14745.59',
           'opr14746.bin': 'opr-14746.62', 'opr14747.bin': 'opr-14747.63'}

def find(z, name, crc):
    by_crc = {i.CRC: i.filename for i in z.infolist()}
    for n in (name, ALIASES.get(name)):
        if n and n in z.namelist():
            return n
    if crc in by_crc:
        return by_crc[crc]
    sys.exit(f'{name}: not in the zip')

OPTIONAL = [('315-5724.bin', 0x4b4f330e, 'tgp_program.bin'),
            ('epr-14869b.25', 0x2d093304, 'ioboard.bin')]   # from model1io.zip

def find_optional(sources, name, crc):
    for s in sources:
        if s.endswith('.zip'):
            with zipfile.ZipFile(s) as z:
                for i in z.infolist():
                    if i.CRC == crc or os.path.basename(i.filename) == name:
                        d = z.read(i)
                        if zlib.crc32(d) == crc:
                            return d
        elif os.path.isfile(s):
            d = open(s, 'rb').read()
            if zlib.crc32(d) == crc:
                return d
    return None

def main():
    src = sys.argv[1] if len(sys.argv) > 1 else 'roms/vf.zip'
    out = sys.argv[2] if len(sys.argv) > 2 else 'build/vf'
    extra = sys.argv[3:]
    os.makedirs(out, exist_ok=True)
    regions = {k: bytearray([fill]) * size for k, (size, fill) in REGIONS.items()}
    with zipfile.ZipFile(src) as z:
        for name, crc, region, off, kind in ROMS:
            data = z.read(find(z, name, crc))
            if zlib.crc32(data) != crc:
                sys.exit(f'{name}: CRC {zlib.crc32(data):08x}, expected {crc:08x}')
            r = regions[region]
            if kind == 'raw':
                r[off:off + len(data)] = data
            elif kind == 'byte16':
                r[off:off + 2 * len(data):2] = data
            elif kind == 'wordswap':
                sw = bytearray(len(data)); sw[0::2] = data[1::2]; sw[1::2] = data[0::2]
                r[off:off + len(data)] = sw
            elif kind == 'word32':
                for i in range(0, len(data), 2):
                    r[off + 2 * i:off + 2 * i + 2] = data[i:i + 2]
    for k, r in regions.items():
        with open(os.path.join(out, k + '.bin'), 'wb') as f:
            f.write(r)
    print(f'{len(ROMS)} ROMs checked, {len(regions)} regions written to {out}/')
    for name, crc, dst in OPTIONAL:
        d = find_optional([src] + extra, name, crc)
        if d:
            open(os.path.join(out, dst), 'wb').write(d)
            print(f'{name}: found, written to {out}/{dst}')
        else:
            print(f'{name}: not found (falls back to a stand-in)')

if __name__ == '__main__':
    main()
