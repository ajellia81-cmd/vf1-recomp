#include "coro.h"
#include <cstdint>
#include <cstring>

#if defined(__aarch64__)
extern "C" void coro_start();

void coro_init(coro_ctx *ctx, void *stack, size_t size, void (*fn)(void *), void *arg)
{
	uintptr_t top = (reinterpret_cast<uintptr_t>(stack) + size) & ~uintptr_t(15);
	uint64_t *frame = reinterpret_cast<uint64_t *>(top - 160);
	memset(frame, 0, 160);
	frame[0] = reinterpret_cast<uint64_t>(fn);    // x19
	frame[1] = reinterpret_cast<uint64_t>(arg);   // x20
	frame[11] = reinterpret_cast<uint64_t>(&coro_start);  // x30: where coro_switch returns
	ctx->sp = frame;
}
#elif defined(__x86_64__) && defined(_WIN32)
extern "C" void coro_start();

void coro_init(coro_ctx *ctx, void *stack, size_t size, void (*fn)(void *), void *arg)
{
	// frame, low to high (coro_win64.S): xmm6-xmm15, MXCSR + x87 control word (8), StackLimit,
	// StackBase, r15, r14, r13, r12, rsi, rdi, rbx, rbp, return address: 256 bytes. The stack top is
	// 16-aligned, so coro_start runs with rsp aligned for its call.
	uintptr_t base = reinterpret_cast<uintptr_t>(stack);
	uintptr_t top = (base + size) & ~uintptr_t(15);
	uint64_t *frame = reinterpret_cast<uint64_t *>(top - 256);
	memset(frame, 0, 256);
	uint32_t mxcsr = 0x1f80;     // defaults: all exceptions masked, round to nearest
	uint16_t fpucw = 0x037f;
	memcpy(reinterpret_cast<char *>(frame) + 160, &mxcsr, 4);
	memcpy(reinterpret_cast<char *>(frame) + 164, &fpucw, 2);
	frame[21] = base;            // StackLimit (168)
	frame[22] = top;             // StackBase (176)
	frame[25] = reinterpret_cast<uint64_t>(arg);   // r13 (200)
	frame[26] = reinterpret_cast<uint64_t>(fn);    // r12 (208)
	frame[31] = reinterpret_cast<uint64_t>(&coro_start);   // (248)
	ctx->sp = frame;
}
#elif defined(__x86_64__)
extern "C" void coro_start();

void coro_init(coro_ctx *ctx, void *stack, size_t size, void (*fn)(void *), void *arg)
{
	// frame, low to high: MXCSR + x87 control word, r15, r14, r13, r12, rbx, rbp, return address.
	// The stack top is 16-aligned, so coro_start runs with rsp aligned for its call.
	uintptr_t top = (reinterpret_cast<uintptr_t>(stack) + size) & ~uintptr_t(15);
	uint64_t *frame = reinterpret_cast<uint64_t *>(top - 64);
	memset(frame, 0, 64);
	uint32_t mxcsr = 0x1f80;     // defaults: all exceptions masked, round to nearest
	uint16_t fpucw = 0x037f;
	memcpy(reinterpret_cast<char *>(frame), &mxcsr, 4);
	memcpy(reinterpret_cast<char *>(frame) + 4, &fpucw, 2);
	frame[3] = reinterpret_cast<uint64_t>(arg);   // r13
	frame[4] = reinterpret_cast<uint64_t>(fn);    // r12
	frame[7] = reinterpret_cast<uint64_t>(&coro_start);
	ctx->sp = frame;
}
#else
#error "coroutines: arm64 and x86-64 only"
#endif
