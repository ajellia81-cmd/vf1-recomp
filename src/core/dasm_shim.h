// MAME's disassembler interface, reduced to what v60d.cpp uses.
#pragma once
#include "mame_shim.h"
#include <ostream>
#include <string>

namespace util {
template <typename... Args>
void stream_format(std::ostream &s, const char *fmt, Args... args)
{
	char buf[512];
	snprintf(buf, sizeof buf, fmt, args...);
	s << buf;
}
inline void stream_format(std::ostream &s, const char *fmt) { s << fmt; }

class disasm_interface
{
public:
	virtual ~disasm_interface() = default;
	enum : u32 {
		FLAGS_MASK = 0xf0000000, LENGTHMASK = 0x0000ffff,
		STEP_OUT = 0x80000000, STEP_OVER = 0x40000000, STEP_COND = 0x20000000,
		SUPPORTED = 0x10000000,
	};
	class data_buffer
	{
	public:
		std::function<u8(offs_t)> rd;
		u8 r8(offs_t a) const { return rd(a); }
		u16 r16(offs_t a) const { return rd(a) | (rd(a + 1) << 8); }
		u32 r32(offs_t a) const { return r16(a) | (u32(r16(a + 2)) << 16); }
	};
	virtual u32 opcode_alignment() const = 0;
	virtual offs_t disassemble(std::ostream &stream, offs_t pc, const data_buffer &opcodes, const data_buffer &params) = 0;
};
}
