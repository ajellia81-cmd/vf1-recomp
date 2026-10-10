#!/usr/bin/env python3
"""Turns v60recomp's listing into C++: one member function of v60_gen per V60 function.

    emit.py <maincpu.bin> <listing.txt> <out.cpp> [--exec-only]

Each instruction becomes MAME's handler body for its opcode, pasted verbatim from
third_party/mame/v60/*.hxx, with the operand decoding (which only depends on the instruction's
bytes) worked out here instead of at run time. The pasted body runs inside a lambda so its
`return` statements keep their meaning: the instruction's length, or 0 when it set PC itself.
The control flow around it is then the same as for the interpreter fallback (EXEC), so the two
mix freely and the lockstep check covers both.

Instructions whose handlers decode in ways this doesn't model (the bit-string and string
formats, 64-bit operands, error modes) fall back to EXEC: MAME's handler, run at that address.

The output is derived from the ROM: it goes to build/ and is never committed.
"""
import os, re, sys
from collections import Counter

ROOT = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
MAME = os.path.join(ROOT, 'third_party', 'mame', 'v60')

# ---------------------------------------------------------------- ROM bytes

mem = b''
def u8(a): return mem[a & 0xffffff]
def u16(a): return u8(a) | (u8(a + 1) << 8)
def u32(a): return u16(a) | (u16(a + 2) << 16)
def s8(a): v = u8(a); return v - 0x100 if v & 0x80 else v
def s16(a): v = u16(a); return v - 0x10000 if v & 0x8000 else v
def s32(a): v = u32(a); return v - 0x100000000 if v & 0x80000000 else v

# ---------------------------------------------------------------- MAME handler bodies

def load_handlers():
    bodies = {}
    for fn in sorted(os.listdir(MAME)):
        if not fn.endswith('.hxx'):
            continue
        src = open(os.path.join(MAME, fn)).read()
        for m in re.finditer(r'uint32_t v60_device::(op\w+)\(([^)]*)\)[^{\n]*\n?\{', src):
            depth, i = 1, m.end()
            while depth:
                c = src[i]
                if c == '{': depth += 1
                elif c == '}': depth -= 1
                i += 1
            bodies[m.group(1)] = (m.group(2).strip(), src[m.end():i - 1])
    return bodies

def load_tables():
    src = open(os.path.join(MAME, 'optable.hxx')).read()
    src += open(os.path.join(MAME, 'op7a.hxx')).read() + open(os.path.join(MAME, 'op2.hxx')).read()
    src += open(os.path.join(MAME, 'op6.hxx')).read()
    tables = {}
    for m in re.finditer(r'v60_device::(s_\w+)\[\d+\]\s*=\s*(/\*[^*]*\*/)?\s*\{(.*?)\};', src, re.S):
        tables[m.group(1)] = re.findall(r'&v60_device::(\w+)', m.group(3))
    return tables

def load_macros():
    """The #defines MAME's opcode files make for their handlers (F12LOADOPWORD and the like)."""
    out = []
    for fn in sorted(os.listdir(MAME)):
        if not fn.endswith('.hxx') or fn.startswith('am'):
            continue
        lines = open(os.path.join(MAME, fn)).read().split('\n')
        i = 0
        while i < len(lines):
            if lines[i].startswith('#define'):
                block = [lines[i]]
                while block[-1].rstrip().endswith('\\'):
                    i += 1
                    block.append(lines[i])
                out.append('\n'.join(block))
            i += 1
    return out

HANDLERS = load_handlers()
MACROS = load_macros()
TABLES = load_tables()

# ---------------------------------------------------------------- addressing modes

def R(n): return f'm_reg[{n}]'
RD = {0: 'm_program->read_byte', 1: 'm_program->read_word_unaligned', 2: 'm_program->read_dword_unaligned'}
WR = {0: ('m_program->write_byte', 'm_modwritevalb'), 1: ('m_program->write_word_unaligned', 'm_modwritevalh'),
      2: ('m_program->write_dword_unaligned', 'm_modwritevalw')}
CAST = {0: '(uint8_t)', 1: '(uint16_t)', 2: ''}
SCALE = {0: 1, 1: 2, 2: 4, 3: 8}

class Unsupported(Exception):
    pass

def h(v):
    v &= 0xffffffff
    return f'0x{v:x}'

def disp(a, size):
    return {1: s8, 2: s16, 4: s32}[size](a)

def mode(modadd, modm, dim, pc):
    """Static decode of one operand. Returns (kind, length, data):
       ('reg', len, r) / ('imm', len, value) / ('mem', len, (pre, addr_expr, post))
       pre/post: statements before/after the access (autodecrement / autoincrement)."""
    modval = u8(modadd)
    r = modval & 0x1f
    sizes = {0: 1, 1: 2, 2: 4}
    if not modm:
        g = modval >> 5
        if g in (0, 1, 2):
            sz = (1, 2, 4)[g]
            return 'mem', 1 + sz, ([], f'{R(r)} + {h(disp(modadd + 1, sz))}', [])
        if g == 3:
            return 'mem', 1, ([], R(r), [])
        if g in (4, 5, 6):
            sz = (1, 2, 4)[g - 4]
            return 'mem', 1 + sz, ([], f'm_program->read_dword_unaligned({R(r)} + {h(disp(modadd + 1, sz))})', [])
        # group 7
        if r < 16:
            return 'imm', 1, r
        if r in (16, 17, 18):
            sz = (1, 2, 4)[r - 16]
            return 'mem', 1 + sz, ([], h(pc + disp(modadd + 1, sz)), [])
        if r == 19:
            return 'mem', 5, ([], h(u32(modadd + 1)), [])
        if r == 20:
            if dim == 0: return 'imm', 2, u8(modadd + 1)
            if dim == 1: return 'imm', 3, u16(modadd + 1)
            if dim == 2: return 'imm', 5, u32(modadd + 1)
            raise Unsupported('immediate dim')
        if r in (24, 25, 26):
            sz = (1, 2, 4)[r - 24]
            return 'mem', 1 + sz, ([], f'm_program->read_dword_unaligned({h(pc + disp(modadd + 1, sz))})', [])
        if r == 27:
            return 'mem', 5, ([], f'm_program->read_dword_unaligned({h(u32(modadd + 1))})', [])
        if r in (28, 29, 30):
            sz = (1, 2, 4)[r - 28]
            return 'mem', 1 + 2 * sz, ([], f'm_program->read_dword_unaligned({h(pc + disp(modadd + 1, sz))}) + {h(disp(modadd + 1 + sz, sz))}', [])
        raise Unsupported('group 7 error mode')
    g = modval >> 5
    if g in (0, 1, 2):
        sz = (1, 2, 4)[g]
        return 'mem', 1 + 2 * sz, ([], f'm_program->read_dword_unaligned({R(r)} + {h(disp(modadd + 1, sz))}) + {h(disp(modadd + 1 + sz, sz))}', [])
    if g == 3:
        return 'reg', 1, r
    if g == 4:
        return 'mem', 1, ([], R(r), [f'{R(r)} += {SCALE[dim]};'])
    if g == 5:
        return 'mem', 1, ([f'{R(r)} -= {SCALE[dim]};'], R(r), [])
    if g == 6:
        m2 = u8(modadd + 1)
        b = m2 & 0x1f
        idx = f'{R(r)} * {SCALE[dim]}' if SCALE[dim] != 1 else R(r)
        g2 = m2 >> 5
        if g2 in (0, 1, 2):
            sz = (1, 2, 4)[g2]
            return 'mem', 2 + sz, ([], f'{R(b)} + {h(disp(modadd + 2, sz))} + {idx}', [])
        if g2 == 3:
            return 'mem', 2, ([], f'{R(b)} + {idx}', [])
        if g2 in (4, 5, 6):
            sz = (1, 2, 4)[g2 - 4]
            return 'mem', 2 + sz, ([], f'm_program->read_dword_unaligned({R(b)} + {h(disp(modadd + 2, sz))}) + {idx}', [])
        # group 7a
        if not (m2 & 0x10):
            raise Unsupported('group 7a error')
        k = m2 & 0xf
        if k in (0, 1, 2):
            sz = (1, 2, 4)[k]
            return 'mem', 2 + sz, ([], f'{h(pc + disp(modadd + 2, sz))} + {idx}', [])
        if k == 3:
            return 'mem', 6, ([], f'{h(u32(modadd + 2))} + {idx}', [])
        if k in (8, 9, 10):
            sz = (1, 2, 4)[k - 8]
            return 'mem', 2 + sz, ([], f'm_program->read_dword_unaligned({h(pc + disp(modadd + 2, sz))}) + {idx}', [])
        if k == 11:
            return 'mem', 6, ([], f'm_program->read_dword_unaligned({h(u32(modadd + 2))}) + {idx}', [])
        raise Unsupported('group 7a error')
    raise Unsupported('error mode')

def read_am(modadd, modm, dim, pc):
    """MAME's ReadAM: statements setting m_amout; returns (stmts, length)."""
    if dim not in (0, 1, 2):
        raise Unsupported('ReadAM dim')
    kind, ln, d = mode(modadd, modm, dim, pc)
    if kind == 'reg':
        return [f'm_amout = {CAST[dim]}{R(d)};'], ln
    if kind == 'imm':
        return [f'm_amout = {h(d)};'], ln
    pre, addr, post = d
    return pre + [f'm_amout = {RD[dim]}({addr});'] + post, ln

def read_am_address(modadd, modm, dim, pc):
    """MAME's ReadAMAddress: statements setting m_amflag and m_amout."""
    if dim not in (0, 1, 2, 3):
        raise Unsupported('ReadAMAddress dim')
    kind, ln, d = mode(modadd, modm, dim, pc)
    if kind == 'reg':
        return ['m_amflag = 1;', f'm_amout = {d};'], ln
    if kind == 'imm':
        return ['m_amflag = 0;', f'm_amout = {h(d)};'], ln
    pre, addr, post = d
    return pre + ['m_amflag = 0;', f'm_amout = {addr};'] + post, ln

def write_am(modadd, modm, dim, pc):
    """MAME's WriteAM: stores m_modwritevalb/h/w."""
    if dim not in (0, 1, 2):
        raise Unsupported('WriteAM dim')
    kind, ln, d = mode(modadd, modm, dim, pc)
    if kind == 'reg':
        if dim == 0: return [f'SETREG8({R(d)}, m_modwritevalb);'], ln
        if dim == 1: return [f'SETREG16({R(d)}, m_modwritevalh);'], ln
        return [f'{R(d)} = m_modwritevalw;'], ln
    if kind == 'imm':
        raise Unsupported('WriteAM immediate')
    pre, addr, post = d
    fn, val = WR[dim]
    return pre + [f'{fn}({addr}, {val});'] + post, ln

DECODERS = {'ReadAM': read_am, 'ReadAMAddress': read_am_address}

def decode_operand(fn, modadd, modm, dim, pc, n):
    """Decode via MAME's am_func `fn` and copy into m_op<n>/m_flag<n>, as F12DecodeOperands does."""
    if fn not in DECODERS:
        raise Unsupported(fn)
    stmts, ln = DECODERS[fn](modadd, modm, dim, pc)
    return stmts + [f'm_op{n} = m_amout;', f'm_flag{n} = m_amflag;'], ln

# ---------------------------------------------------------------- the operand formats

def f12_decode_operands(pc, fn1, d1, fn2, d2):
    if12 = u8(pc + 1)
    out = []
    if if12 & 0x80:
        s1, l1 = decode_operand(fn1, pc + 2, if12 & 0x40, d1, pc, 1)
        s2, l2 = decode_operand(fn2, pc + 2 + l1, if12 & 0x20, d2, pc, 2)
        out = s1 + [f'm_amlength1 = {l1};'] + s2 + [f'm_amlength2 = {l2};']
    elif if12 & 0x20:
        reg = if12 & 0x1f
        if fn2 == 'ReadAMAddress':
            out = [f'm_op2 = {reg};', 'm_flag2 = 1;']
        else:
            if d2 not in (0, 1, 2): raise Unsupported('f12 reg dim')
            out = [f'm_op2 = {CAST[d2]}{R(reg)};']
        out.append('m_amlength2 = 0;')
        s1, l1 = decode_operand(fn1, pc + 2, if12 & 0x40, d1, pc, 1)
        out += s1 + [f'm_amlength1 = {l1};']
    else:
        reg = if12 & 0x1f
        if fn1 == 'ReadAMAddress':
            out = [f'm_op1 = {reg};', 'm_flag1 = 1;']
        else:
            if d1 not in (0, 1, 2): raise Unsupported('f12 reg dim')
            out = [f'm_op1 = {CAST[d1]}{R(reg)};']
        out.append('m_amlength1 = 0;')
        s2, l2 = decode_operand(fn2, pc + 2, if12 & 0x40, d2, pc, 2)
        out += s2 + [f'm_amlength2 = {l2};']
    return out, None

def f12_decode_first(pc, fn1, d1):
    flags = u8(pc + 1)
    out = [f'm_instflags = {h(flags)};']
    if flags & 0x80 or flags & 0x20:
        s1, l1 = decode_operand(fn1, pc + 2, flags & 0x40, d1, pc, 1)
        return out + s1 + [f'm_amlength1 = {l1};'], l1
    reg = flags & 0x1f
    if fn1 == 'ReadAM':
        if d1 not in (0, 1, 2): raise Unsupported('f12 reg dim')
        out += [f'm_op1 = {CAST[d1]}{R(reg)};', 'm_flag1 = 0;']
    elif fn1 == 'ReadAMAddress':
        out += ['m_flag1 = 1;', f'm_op1 = {reg};']
    else:
        raise Unsupported(fn1)
    return out + ['m_amlength1 = 0;'], 0

def f12_write_second(pc, d2, l1):
    flags = u8(pc + 1)
    if flags & 0x80:
        s, l2 = write_am(pc + 2 + l1, flags & 0x20, d2, pc)
        return s + [f'm_amlength2 = {l2};']
    if flags & 0x20:
        reg = flags & 0x1f
        if d2 == 0: s = [f'SETREG8({R(reg)}, m_modwritevalb);']
        elif d2 == 1: s = [f'SETREG16({R(reg)}, m_modwritevalh);']
        elif d2 == 2: s = [f'{R(reg)} = m_modwritevalw;']
        else: raise Unsupported('f12 write dim')
        return s + ['m_amlength2 = 0;']
    s, l2 = write_am(pc + 2, flags & 0x40, d2, pc)
    return s + [f'm_amlength2 = {l2};']

# ---------------------------------------------------------------- handler specialisation

RE_F12OPS = re.compile(r'F12DecodeOperands\(&v60_device::(\w+),\s*(\d+),\s*&v60_device::(\w+),\s*(\d+)\);')
RE_F12FIRST = re.compile(r'F12DecodeFirstOperand\(&v60_device::(\w+),\s*(\d+)\);')
RE_F12W2 = re.compile(r'F12WriteSecondOperand\((\d+)\);')
RE_F2FIRST = re.compile(r'F2DecodeFirstOperand\(&v60_device::(\w+),\s*(\d+)\);')
RE_F2SECOND = re.compile(r'F2DecodeSecondOperand\(&v60_device::(\w+),\s*(\d+)\);')
RE_F2W2 = re.compile(r'F2WriteSecondOperand\((\d+)\);')
RE_F3 = re.compile(r'm_modadd = PC \+ 1;\s*m_moddim = (\d+);\s*(?://[^\n]*\n\s*)*(?:(m_amlength1) = )?(ReadAM|ReadAMAddress|WriteAM)\(\);')

def specialise(name, body, pc, modm=None):
    """MAME handler body with its decode replaced by straight-line code for the bytes at pc."""
    b = body
    l1_holder = {}

    def sub_f12ops(m):
        s, _ = f12_decode_operands(pc, m.group(1), int(m.group(2)), m.group(3), int(m.group(4)))
        return ' '.join(s)
    def sub_f12first(m):
        s, l1 = f12_decode_first(pc, m.group(1), int(m.group(2)))
        l1_holder['l1'] = l1
        return ' '.join(s)
    def sub_f12w2(m):
        if 'l1' not in l1_holder: raise Unsupported('write before decode')
        return ' '.join(f12_write_second(pc, int(m.group(1)), l1_holder['l1']))
    def sub_f2first(m):
        flags = u8(pc + 1)
        s, l1 = decode_operand(m.group(1), pc + 2, flags & 0x40, int(m.group(2)), pc, 1)
        l1_holder['l1'] = l1
        return ' '.join(s + [f'm_amlength1 = {l1};'])
    def sub_f2second(m):
        flags = u8(pc + 1)
        if 'l1' not in l1_holder: raise Unsupported('second before first')
        s, l2 = decode_operand(m.group(1), pc + 2 + l1_holder['l1'], flags & 0x20, int(m.group(2)), pc, 2)
        return ' '.join(s + [f'm_amlength2 = {l2};'])
    def sub_f2w2(m):
        flags = u8(pc + 1)
        if 'l1' not in l1_holder: raise Unsupported('write before decode')
        s, l2 = write_am(pc + 2 + l1_holder['l1'], flags & 0x20, int(m.group(1)), pc)
        return ' '.join(s + [f'm_amlength2 = {l2};'])
    def sub_f3(m):
        if modm is None: raise Unsupported('F3 without m')
        dim = int(m.group(1))
        fn = m.group(3)
        if fn == 'WriteAM':
            s, ln = write_am(pc + 1, modm, dim, pc)
        else:
            s, ln = DECODERS[fn](pc + 1, modm, dim, pc)
        if m.group(2):
            s = s + [f'm_amlength1 = {ln};']
        return ' '.join(s)

    b = RE_F12OPS.sub(sub_f12ops, b)
    b = RE_F12FIRST.sub(sub_f12first, b)
    b = RE_F12W2.sub(sub_f12w2, b)
    b = RE_F2FIRST.sub(sub_f2first, b)
    b = RE_F2SECOND.sub(sub_f2second, b)
    b = RE_F2W2.sub(sub_f2w2, b)
    b = RE_F3.sub(sub_f3, b)
    b = b.replace('F12END();', 'return m_amlength1 + m_amlength2 + 2;')
    b = b.replace('F2END();', 'return 2 + m_amlength1 + m_amlength2;')
    # anything still decoding at run time, or a format this doesn't model: leave it to MAME
    for bad in ('ReadAM', 'WriteAM', 'Decode', 'm_modadd', 'm_moddim', 'F7', 'm_bamoffset', 'm_lenop',
                'opUNHANDLED', 'fatalerror'):
        if bad in b:
            raise Unsupported(bad)
    return b

def handler_for(pc):
    """(handler name, body, prelude) for the instruction at pc."""
    op = u8(pc)
    name = TABLES['s_OpCodeTable'][op]
    pre = []
    sub = {'op58': 's_Op58Table', 'op59': 's_Op59Table', 'op5A': 's_Op5ATable', 'op5B': 's_Op5BTable',
           'op5D': 's_Op5DTable', 'op5C': 's_Op5CTable', 'op5F': 's_Op5FTable'}
    if name in sub:
        b1 = u8(pc + 1)
        if name in ('op5C', 'op5F'):
            pre.append(f'm_instflags = {h(b1)};')
        else:
            pre.append(f'm_subop = {h(b1)};')
        name = TABLES[sub[name]][b1 & 0x1f]
    modm = None
    if name.endswith('_0') or name.endswith('_1'):
        # opINCB_0() { m_modm = 0; return opINCB(); }
        modm = int(name[-1])
        pre.append(f'm_modm = {modm};')
        name = name[:-2]
    if name in ('opC6', 'opC7'):
        b1 = u8(pc + 1)
        name = TABLES['s_OpC6Table' if name == 'opC6' else 's_OpC7Table'][b1 >> 5]
        pre.append(f'const int reg = {b1 & 0x1f};')
    if name not in HANDLERS:
        raise Unsupported('no body ' + name)
    args, body = HANDLERS[name]
    if args and args != 'int reg':
        raise Unsupported('args')
    return name, specialise(name, body, pc, modm), pre

# ---------------------------------------------------------------- functions

def emit(listing, out, exec_only):
    funcs = []
    cur = None
    for line in open(listing):
        if line.startswith('F '):
            cur = (int(line.split()[1], 16), [])
            funcs.append(cur)
        elif line.startswith('I '):
            head, _, text = line.rstrip('\n').partition(' ; ')
            p = head.split()
            cur[1].append((int(p[1], 16), int(p[2]), p[3], [int(t, 16) for t in p[4:]], text))
    native = Counter()
    fallback = Counter()
    o = open(out, 'w')
    o.write('// Generated by tools/recomp/emit.py from the user\'s own ROM. Do not commit.\n')
    o.write('#include "rec_gen.h"\n\n')
    o.write('// helper macros from MAME\'s opcode files\n' + '\n'.join(MACROS) + '\n\n')
    o.write('struct v60_gen : v60_device {\n')
    for entry, _ in funcs:
        o.write(f'\tvoid f_{entry:06x}();\n')
    o.write('};\n')
    for entry, insns in funcs:
        body = {a for a, *_ in insns}
        o.write(f'\nvoid v60_gen::f_{entry:06x}()\n{{\n\tuint32_t n;\n\t(void)n;\n')
        o.write(f'\tif (PC != 0x{entry:06x}) goto dispatch;\n')
        for i, (a, ln, kind, targets, text) in enumerate(insns):
            nxt = a + ln
            text = text.replace('\\', '/').replace('*/', '* /')
            o.write(f'L_{a:06x}: // {text}\n')
            code = None
            if not exec_only and kind != 'bad':
                try:
                    name, b, pre = handler_for(a)
                    code = (name, b, pre)
                except Unsupported as e:
                    fallback[f'{u8(a):02x} {e}'] += 1
            if code:
                name, b, pre = code
                native[name] += 1
                lam = '\n\t\t'.join(x.strip() for x in b.strip().split('\n') if x.strip())
                o.write(f'\tn = [&]() -> uint32_t {{ {" ".join(pre)}\n\t\t{lam}\n\t}}();\n')
            else:
                o.write('\tn = EXEC();\n')
            o.write('\tif (n) PC += n; TICK();\n')
            if kind == 'normal':
                o.write(f'\tif (!n) goto dispatch;\n')
            elif kind in ('branch', 'jump'):
                gotos = ''.join(f' if (PC == 0x{t:06x}) goto L_{t:06x};' for t in targets if t in body)
                o.write(f'\tif (!n) {{{gotos} goto dispatch; }}\n')
            elif kind == 'call':
                o.write(f'\tif (!n) {{ CALL(PC); if (PC != 0x{nxt:06x}) goto dispatch; }}\n')
            elif kind == 'ret':
                o.write('\treturn;\n')
            else:
                o.write(f'\tBAD(0x{a:06x});\n')
            falls = kind not in ('jump', 'ret', 'bad')
            if falls:
                written_next = insns[i + 1][0] if i + 1 < len(insns) else None
                if written_next != nxt:
                    if nxt in body:
                        o.write(f'\tgoto L_{nxt:06x};\n')
                    else:
                        o.write(f'\tJUMP(0x{nxt:06x}); return;\n')
        o.write('dispatch:\n\tswitch (PC) {\n')
        for a in sorted(body):
            o.write(f'\tcase 0x{a:06x}: goto L_{a:06x};\n')
        o.write(f'\t}}\n\tLEAVE(0x{entry:06x}); JUMP(PC);\n}}\n')
    o.write('\n')
    for entry, _ in funcs:
        o.write(f'static void t_{entry:06x}(v60_device &c) {{ static_cast<v60_gen &>(c).f_{entry:06x}(); }}\n')
    o.write('\nconst rec_entry rec_table[] = {\n')
    for entry, _ in sorted(funcs):
        o.write(f'\t{{ 0x{entry:06x}, t_{entry:06x} }},\n')
    o.write(f'}};\nconst size_t rec_table_size = {len(funcs)};\n')
    o.close()
    total_native = sum(native.values())
    total_fb = sum(fallback.values())
    print(f'{len(funcs)} functions: {total_native} instructions native, {total_fb} via MAME\'s handler')
    for k, v in fallback.most_common(15):
        print(f'  fallback {v:5d}  {k}')

def main():
    global mem
    args = [a for a in sys.argv[1:] if not a.startswith('--')]
    mem = open(args[0], 'rb').read()
    emit(args[1], args[2], '--exec-only' in sys.argv)

if __name__ == '__main__':
    main()
