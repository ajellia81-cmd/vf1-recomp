// coro_switch(from, to) for x86-64 System V (Linux, SteamOS): saves the callee-saved registers,
// MXCSR and the x87 control word on the current stack, stores rsp in *from, loads it from *to
// and restores. coro_start is the first return of a new coroutine: fn in r12, arg in r13.
#if defined(__x86_64__)
#if defined(__APPLE__)
#define SYM(x) _##x
#else
#define SYM(x) x
#endif
	.text
	.p2align 4
	.globl SYM(coro_switch)
SYM(coro_switch):
	pushq %rbp
	pushq %rbx
	pushq %r12
	pushq %r13
	pushq %r14
	pushq %r15
	subq $8, %rsp
	stmxcsr (%rsp)
	fnstcw 4(%rsp)
	movq %rsp, (%rdi)
	movq (%rsi), %rsp
	ldmxcsr (%rsp)
	fldcw 4(%rsp)
	addq $8, %rsp
	popq %r15
	popq %r14
	popq %r13
	popq %r12
	popq %rbx
	popq %rbp
	ret

	.globl SYM(coro_start)
SYM(coro_start):
	movq %r13, %rdi
	callq *%r12
	ud2
#endif
#if defined(__linux__) && defined(__ELF__)
	.section .note.GNU-stack,"",@progbits
#endif
