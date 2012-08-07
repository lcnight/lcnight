	.file	"atomic.c"
	.section	.rodata
.LC0:
	.string	"%d\n"
	.text
.globl main
	.type	main, @function
main:
.LFB12:
	pushq	%rbp
.LCFI0:
	movq	%rsp, %rbp
.LCFI1:
	subq	$32, %rsp
.LCFI2:
	movl	%edi, -20(%rbp)
	movq	%rsi, -32(%rbp)
	movl	$123, -16(%rbp)
	leaq	-16(%rbp), %rdi
	call	atomic_inc
	leaq	-16(%rbp), %rsi
	movl	$1, %edi
	call	atomic_add_return
	movl	%eax, %esi
	movl	$.LC0, %edi
	movl	$0, %eax
	call	printf
	movl	-16(%rbp), %esi
	movl	$.LC0, %edi
	movl	$0, %eax
	call	printf
	movl	$0, %eax
	leave
	ret
.LFE12:
	.size	main, .-main
	.type	atomic_inc, @function
atomic_inc:
.LFB5:
	pushq	%rbp
.LCFI3:
	movq	%rsp, %rbp
.LCFI4:
	movq	%rdi, -8(%rbp)
	movq	-8(%rbp), %rdx
	movq	-8(%rbp), %rax
#APP
# 89 "atomic.h" 1
	lock ; incl (%rdx)
# 0 "" 2
#NO_APP
	leave
	ret
.LFE5:
	.size	atomic_inc, .-atomic_inc
	.type	atomic_add_return, @function
atomic_add_return:
.LFB10:
	pushq	%rbp
.LCFI5:
	movq	%rsp, %rbp
.LCFI6:
	movl	%edi, -20(%rbp)
	movq	%rsi, -32(%rbp)
	movl	-20(%rbp), %eax
	movl	%eax, -4(%rbp)
	movq	-32(%rbp), %rdx
	movl	-20(%rbp), %eax
#APP
# 169 "atomic.h" 1
	lock ; xaddl %eax, (%rdx);
# 0 "" 2
#NO_APP
	movl	%eax, -20(%rbp)
	movl	-4(%rbp), %edx
	movl	-20(%rbp), %eax
	addl	%edx, %eax
	leave
	ret
.LFE10:
	.size	atomic_add_return, .-atomic_add_return
	.section	.eh_frame,"a",@progbits
.Lframe1:
	.long	.LECIE1-.LSCIE1
.LSCIE1:
	.long	0x0
	.byte	0x1
	.string	"zR"
	.uleb128 0x1
	.sleb128 -8
	.byte	0x10
	.uleb128 0x1
	.byte	0x3
	.byte	0xc
	.uleb128 0x7
	.uleb128 0x8
	.byte	0x90
	.uleb128 0x1
	.align 8
.LECIE1:
.LSFDE1:
	.long	.LEFDE1-.LASFDE1
.LASFDE1:
	.long	.LASFDE1-.Lframe1
	.long	.LFB12
	.long	.LFE12-.LFB12
	.uleb128 0x0
	.byte	0x4
	.long	.LCFI0-.LFB12
	.byte	0xe
	.uleb128 0x10
	.byte	0x86
	.uleb128 0x2
	.byte	0x4
	.long	.LCFI1-.LCFI0
	.byte	0xd
	.uleb128 0x6
	.align 8
.LEFDE1:
.LSFDE3:
	.long	.LEFDE3-.LASFDE3
.LASFDE3:
	.long	.LASFDE3-.Lframe1
	.long	.LFB5
	.long	.LFE5-.LFB5
	.uleb128 0x0
	.byte	0x4
	.long	.LCFI3-.LFB5
	.byte	0xe
	.uleb128 0x10
	.byte	0x86
	.uleb128 0x2
	.byte	0x4
	.long	.LCFI4-.LCFI3
	.byte	0xd
	.uleb128 0x6
	.align 8
.LEFDE3:
.LSFDE5:
	.long	.LEFDE5-.LASFDE5
.LASFDE5:
	.long	.LASFDE5-.Lframe1
	.long	.LFB10
	.long	.LFE10-.LFB10
	.uleb128 0x0
	.byte	0x4
	.long	.LCFI5-.LFB10
	.byte	0xe
	.uleb128 0x10
	.byte	0x86
	.uleb128 0x2
	.byte	0x4
	.long	.LCFI6-.LCFI5
	.byte	0xd
	.uleb128 0x6
	.align 8
.LEFDE5:
	.ident	"GCC: (Debian 4.3.2-1.1) 4.3.2"
	.section	.note.GNU-stack,"",@progbits
