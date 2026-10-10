// Minimal coroutines: the game's code runs on its own stack and gives control back to the host
// at the end of each scanline's worth of cycles, from however deep in its call chain it is.
#pragma once
#include <cstddef>

struct coro_ctx { void *sp = nullptr; };

extern "C" void coro_switch(coro_ctx *from, coro_ctx *to);
// Prepares `ctx` so that the first switch to it calls fn(arg) on the given stack.
void coro_init(coro_ctx *ctx, void *stack, size_t size, void (*fn)(void *), void *arg);
