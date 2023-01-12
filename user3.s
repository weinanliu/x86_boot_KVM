	.file	"user3.c"
	.text
	.p2align 4
	.globl	puts
	.type	puts, @function
puts:
.LFB1:
	.cfi_startproc
	endbr64
	pushq	%rbx
	.cfi_def_cfa_offset 16
	.cfi_offset 3, -16
	movq	%rdi, %rbx
	movsbl	(%rdi), %edi
	testb	%dil, %dil
	je	.L2
	.p2align 4,,10
	.p2align 3
.L3:
	call	putc@PLT
	movsbl	1(%rbx), %edi
	addq	$1, %rbx
	testb	%dil, %dil
	jne	.L3
.L2:
	movl	$10, %edi
	popq	%rbx
	.cfi_def_cfa_offset 8
	jmp	putc@PLT
	.cfi_endproc
.LFE1:
	.size	puts, .-puts
	.section	.rodata.str1.1,"aMS",@progbits,1
.LC0:
	.string	"Here is user mode!!"
	.section	.rodata.str1.8,"aMS",@progbits,1
	.align 8
.LC1:
	.string	"Let's call the function of PAGE2"
	.section	.rodata.str1.1
.LC2:
	.string	"user EXIT!"
	.section	.text.startup,"ax",@progbits
	.p2align 4
	.globl	main
	.type	main, @function
main:
.LFB2:
	.cfi_startproc
	endbr64
	subq	$8, %rsp
	.cfi_def_cfa_offset 16
	leaq	.LC0(%rip), %rdi
	call	puts
	leaq	.LC1(%rip), %rdi
	call	puts
	xorl	%eax, %eax
	call	page2_fun@PLT
	leaq	.LC2(%rip), %rdi
	call	puts
	xorl	%eax, %eax
	call	hlt@PLT
	xorl	%eax, %eax
	addq	$8, %rsp
	.cfi_def_cfa_offset 8
	ret
	.cfi_endproc
.LFE2:
	.size	main, .-main
	.ident	"GCC: (Ubuntu 11.3.0-1ubuntu1~22.04) 11.3.0"
	.section	.note.GNU-stack,"",@progbits
	.section	.note.gnu.property,"a"
	.align 8
	.long	1f - 0f
	.long	4f - 1f
	.long	5
0:
	.string	"GNU"
1:
	.align 8
	.long	0xc0000002
	.long	3f - 2f
2:
	.long	0x3
3:
	.align 8
4:
