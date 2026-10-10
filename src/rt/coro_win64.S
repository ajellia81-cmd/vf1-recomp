// coro_switch(from, to) for x86-64 Windows (MinGW): saves what the Windows x64 ABI keeps across a
// call (rbx, rbp, rdi, rsi, r12-r15, xmm6-xmm15, MXCSR and the x87 control word) and the thread's
// stack bounds in its TEB (StackBase gs:8, StackLimit gs:16, which unwinding checks rsp against) on
// the current stack, stores rsp in *from, loads it from *to and restores. coro_start is the first
// return of a new coroutine: fn in r12, arg in r13.
#if defined(__x86_64__) && defined(_WIN32)
	.text
	.p2align 4
	.globl coro_switch
coro_switch:
	pushq %rbp
	pushq %rbx
	pushq %rdi
	pushq %rsi
	pushq %r12
	pushq %r13
	pushq %r14
	pushq %r15
	pushq %gs:8
	pushq %gs:16
	subq $168, %rsp
	movaps %xmm6, 0(%rsp)
	movaps %xmm7, 16(%rsp)
	movaps %xmm8, 32(%rsp)
	movaps %xmm9, 48(%rsp)
	movaps %xmm10, 64(%rsp)
	movaps %xmm11, 80(%rsp)
	movaps %xmm12, 96(%rsp)
	movaps %xmm13, 112(%rsp)
	movaps %xmm14, 128(%rsp)
	movaps %xmm15, 144(%rsp)
	stmxcsr 160(%rsp)
	fnstcw 164(%rsp)
	movq %rsp, (%rcx)
	movq (%rdx), %rsp
	ldmxcsr 160(%rsp)
	fldcw 164(%rsp)
	movaps 0(%rsp), %xmm6
	movaps 16(%rsp), %xmm7
	movaps 32(%rsp), %xmm8
	movaps 48(%rsp), %xmm9
	movaps 64(%rsp), %xmm10
	movaps 80(%rsp), %xmm11
	movaps 96(%rsp), %xmm12
	movaps 112(%rsp), %xmm13
	movaps 128(%rsp), %xmm14
	movaps 144(%rsp), %xmm15
	addq $168, %rsp
	popq %gs:16
	popq %gs:8
	popq %r15
	popq %r14
	popq %r13
	popq %r12
	popq %rsi
	popq %rdi
	popq %rbx
	popq %rbp
	ret

	.globl coro_start
coro_start:
	movq %r13, %rcx
	subq $32, %rsp          // the callee's shadow space; rsp stays 16-aligned for the call
	callq *%r12
	ud2
#endif
