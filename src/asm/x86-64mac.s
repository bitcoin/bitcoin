	.section	__TEXT,__text,regular,pure_instructions
	.build_version macos, 11, 0
	.globl	_makeNIST_P192L                 ## -- Begin function makeNIST_P192L
	.p2align	4, 0x90
_makeNIST_P192L:                        ## @makeNIST_P192L
## %bb.0:
	movq	$-1, %rax
	movq	$-2, %rdx
	movq	$-1, %rcx
	retq
                                        ## -- End function
	.globl	_mcl_fpDbl_mod_NIST_P192L       ## -- Begin function mcl_fpDbl_mod_NIST_P192L
	.p2align	4, 0x90
_mcl_fpDbl_mod_NIST_P192L:              ## @mcl_fpDbl_mod_NIST_P192L
## %bb.0:
	pushq	%r14
	pushq	%rbx
	movq	16(%rsi), %rbx
	movq	24(%rsi), %r8
	movq	40(%rsi), %r9
	movq	8(%rsi), %rdx
	addq	%r9, %rdx
	adcq	$0, %rbx
	setb	%cl
	movzbl	%cl, %r10d
	movq	32(%rsi), %r11
	movq	(%rsi), %r14
	addq	%r8, %r14
	adcq	%r11, %rdx
	adcq	%r9, %rbx
	adcq	$0, %r10
	addq	%r9, %r14
	adcq	%r8, %rdx
	adcq	%r11, %rbx
	setb	%r8b
	movq	%r10, %r9
	adcq	$0, %r9
	addb	$255, %r8b
	adcq	%r10, %r14
	adcq	%rdx, %r9
	adcq	$0, %rbx
	setb	%dl
	movzbl	%dl, %edx
	movq	%r14, %rcx
	addq	$1, %rcx
	movq	%r9, %rsi
	adcq	$1, %rsi
	movq	%rbx, %rax
	adcq	$0, %rax
	adcq	$-1, %rdx
	testb	$1, %dl
	cmovneq	%rbx, %rax
	movq	%rax, 16(%rdi)
	cmovneq	%r9, %rsi
	movq	%rsi, 8(%rdi)
	cmovneq	%r14, %rcx
	movq	%rcx, (%rdi)
	popq	%rbx
	popq	%r14
	retq
                                        ## -- End function
	.globl	_mcl_fp_sqr_NIST_P192L          ## -- Begin function mcl_fp_sqr_NIST_P192L
	.p2align	4, 0x90
_mcl_fp_sqr_NIST_P192L:                 ## @mcl_fp_sqr_NIST_P192L
## %bb.0:
	pushq	%rbp
	pushq	%r15
	pushq	%r14
	pushq	%r13
	pushq	%r12
	pushq	%rbx
	movq	%rdi, -8(%rsp)                  ## 8-byte Spill
	movq	16(%rsi), %r11
	movq	(%rsi), %rbx
	movq	8(%rsi), %rcx
	movq	%r11, %rax
	mulq	%rcx
	movq	%rdx, %rdi
	movq	%rax, %r14
	movq	%rcx, %rax
	mulq	%rcx
	movq	%rdx, %r15
	movq	%rax, %r12
	movq	%rcx, %rax
	mulq	%rbx
	movq	%rax, %r13
	movq	%rdx, %rcx
	addq	%rdx, %r12
	adcq	%r14, %r15
	movq	%rdi, %r10
	adcq	$0, %r10
	movq	%r11, %rax
	mulq	%rbx
	movq	%rdx, %r9
	movq	%rax, %rbp
	movq	%rbx, %rax
	mulq	%rbx
	movq	%rax, %r8
	movq	%rdx, %rsi
	addq	%r13, %rsi
	adcq	%rbp, %rcx
	movq	%r9, %rbx
	adcq	$0, %rbx
	addq	%r13, %rsi
	adcq	%r12, %rcx
	adcq	%r15, %rbx
	adcq	$0, %r10
	movq	%r11, %rax
	mulq	%r11
	addq	%r14, %r9
	adcq	%rdi, %rax
	adcq	$0, %rdx
	addq	%rbp, %rcx
	adcq	%rbx, %r9
	adcq	%r10, %rax
	adcq	$0, %rdx
	addq	%rdx, %rsi
	adcq	$0, %rcx
	setb	%bl
	movzbl	%bl, %edi
	addq	%r9, %r8
	adcq	%rax, %rsi
	adcq	%rdx, %rcx
	adcq	$0, %rdi
	addq	%rdx, %r8
	adcq	%r9, %rsi
	adcq	%rax, %rcx
	setb	%al
	movq	%rdi, %rdx
	adcq	$0, %rdx
	addb	$255, %al
	adcq	%rdi, %r8
	adcq	%rsi, %rdx
	adcq	$0, %rcx
	setb	%al
	movzbl	%al, %eax
	movq	%r8, %rsi
	addq	$1, %rsi
	movq	%rdx, %rdi
	adcq	$1, %rdi
	movq	%rcx, %rbp
	adcq	$0, %rbp
	adcq	$-1, %rax
	testb	$1, %al
	cmovneq	%rcx, %rbp
	movq	-8(%rsp), %rax                  ## 8-byte Reload
	movq	%rbp, 16(%rax)
	cmovneq	%rdx, %rdi
	movq	%rdi, 8(%rax)
	cmovneq	%r8, %rsi
	movq	%rsi, (%rax)
	popq	%rbx
	popq	%r12
	popq	%r13
	popq	%r14
	popq	%r15
	popq	%rbp
	retq
                                        ## -- End function
	.globl	_mcl_fp_mulNIST_P192L           ## -- Begin function mcl_fp_mulNIST_P192L
	.p2align	4, 0x90
_mcl_fp_mulNIST_P192L:                  ## @mcl_fp_mulNIST_P192L
## %bb.0:
	pushq	%r14
	pushq	%rbx
	subq	$56, %rsp
	movq	%rdi, %r14
	leaq	8(%rsp), %rdi
	callq	_mcl_fpDbl_mulPre3L
	movq	24(%rsp), %rbx
	movq	32(%rsp), %r8
	movq	48(%rsp), %rax
	movq	16(%rsp), %rdi
	addq	%rax, %rdi
	adcq	$0, %rbx
	setb	%cl
	movzbl	%cl, %esi
	movq	40(%rsp), %rdx
	movq	8(%rsp), %r9
	addq	%r8, %r9
	adcq	%rdx, %rdi
	adcq	%rax, %rbx
	adcq	$0, %rsi
	addq	%rax, %r9
	adcq	%r8, %rdi
	adcq	%rdx, %rbx
	setb	%dl
	movq	%rsi, %rcx
	adcq	$0, %rcx
	addb	$255, %dl
	adcq	%rsi, %r9
	adcq	%rdi, %rcx
	adcq	$0, %rbx
	setb	%dl
	movzbl	%dl, %edx
	movq	%r9, %rdi
	addq	$1, %rdi
	movq	%rcx, %rsi
	adcq	$1, %rsi
	movq	%rbx, %rax
	adcq	$0, %rax
	adcq	$-1, %rdx
	testb	$1, %dl
	cmovneq	%rbx, %rax
	movq	%rax, 16(%r14)
	cmovneq	%rcx, %rsi
	movq	%rsi, 8(%r14)
	cmovneq	%r9, %rdi
	movq	%rdi, (%r14)
	addq	$56, %rsp
	popq	%rbx
	popq	%r14
	retq
                                        ## -- End function
	.globl	_mcl_fpDbl_mod_NIST_P521L       ## -- Begin function mcl_fpDbl_mod_NIST_P521L
	.p2align	4, 0x90
_mcl_fpDbl_mod_NIST_P521L:              ## @mcl_fpDbl_mod_NIST_P521L
## %bb.0:
	pushq	%r15
	pushq	%r14
	pushq	%r12
	pushq	%rbx
	movq	120(%rsi), %r9
	movq	128(%rsi), %r14
	movq	%r14, %r8
	shldq	$55, %r9, %r8
	movq	112(%rsi), %r10
	shldq	$55, %r10, %r9
	movq	104(%rsi), %r11
	shldq	$55, %r11, %r10
	movq	96(%rsi), %r15
	shldq	$55, %r15, %r11
	movq	88(%rsi), %r12
	shldq	$55, %r12, %r15
	movq	80(%rsi), %rcx
	shldq	$55, %rcx, %r12
	movq	64(%rsi), %rbx
	movq	72(%rsi), %rax
	shldq	$55, %rax, %rcx
	shrq	$9, %r14
	shldq	$55, %rbx, %rax
	movl	%ebx, %edx
	andl	$511, %edx                      ## imm = 0x1FF
	addq	(%rsi), %rax
	adcq	8(%rsi), %rcx
	adcq	16(%rsi), %r12
	adcq	24(%rsi), %r15
	adcq	32(%rsi), %r11
	adcq	40(%rsi), %r10
	adcq	48(%rsi), %r9
	adcq	56(%rsi), %r8
	adcq	%r14, %rdx
	movl	%edx, %esi
	shrl	$9, %esi
	andl	$1, %esi
	addq	%rax, %rsi
	adcq	$0, %rcx
	adcq	$0, %r12
	adcq	$0, %r15
	adcq	$0, %r11
	adcq	$0, %r10
	adcq	$0, %r9
	adcq	$0, %r8
	adcq	$0, %rdx
	movq	%rsi, %rax
	andq	%r12, %rax
	andq	%r15, %rax
	andq	%r11, %rax
	andq	%r10, %rax
	andq	%r9, %rax
	andq	%r8, %rax
	movq	%rdx, %rbx
	orq	$-512, %rbx                     ## imm = 0xFE00
	andq	%rax, %rbx
	andq	%rcx, %rbx
	cmpq	$-1, %rbx
	je	LBB4_1
## %bb.3:                               ## %nonzero
	movq	%r8, 56(%rdi)
	movq	%r9, 48(%rdi)
	movq	%r10, 40(%rdi)
	movq	%r11, 32(%rdi)
	movq	%r15, 24(%rdi)
	movq	%r12, 16(%rdi)
	movq	%rcx, 8(%rdi)
	movq	%rsi, (%rdi)
	andl	$511, %edx                      ## imm = 0x1FF
	movq	%rdx, 64(%rdi)
	jmp	LBB4_2
LBB4_1:                                 ## %zero
	movq	$0, 64(%rdi)
	movq	$0, 56(%rdi)
	movq	$0, 48(%rdi)
	movq	$0, 40(%rdi)
	movq	$0, 32(%rdi)
	movq	$0, 24(%rdi)
	movq	$0, 16(%rdi)
	movq	$0, 8(%rdi)
	movq	$0, (%rdi)
LBB4_2:                                 ## %zero
	popq	%rbx
	popq	%r12
	popq	%r14
	popq	%r15
	retq
                                        ## -- End function
	.globl	_mulPv192x64                    ## -- Begin function mulPv192x64
	.p2align	4, 0x90
_mulPv192x64:                           ## @mulPv192x64
## %bb.0:
	movq	%rdx, %rcx
	movq	%rdx, %rax
	mulq	(%rsi)
	movq	%rdx, %r8
	movq	%rax, (%rdi)
	movq	%rcx, %rax
	mulq	16(%rsi)
	movq	%rdx, %r9
	movq	%rax, %r10
	movq	%rcx, %rax
	mulq	8(%rsi)
	addq	%r8, %rax
	movq	%rax, 8(%rdi)
	adcq	%r10, %rdx
	movq	%rdx, 16(%rdi)
	adcq	$0, %r9
	movq	%r9, 24(%rdi)
	movq	%rdi, %rax
	retq
                                        ## -- End function
	.globl	_mcl_fp_mulUnitPre3L            ## -- Begin function mcl_fp_mulUnitPre3L
	.p2align	4, 0x90
_mcl_fp_mulUnitPre3L:                   ## @mcl_fp_mulUnitPre3L
## %bb.0:
	movq	%rdx, %rcx
	movq	%rdx, %rax
	mulq	16(%rsi)
	movq	%rdx, %r8
	movq	%rax, %r9
	movq	%rcx, %rax
	mulq	8(%rsi)
	movq	%rdx, %r10
	movq	%rax, %r11
	movq	%rcx, %rax
	mulq	(%rsi)
	movq	%rax, (%rdi)
	addq	%r11, %rdx
	movq	%rdx, 8(%rdi)
	adcq	%r9, %r10
	movq	%r10, 16(%rdi)
	adcq	$0, %r8
	movq	%r8, 24(%rdi)
	retq
                                        ## -- End function
	.globl	_mcl_fpDbl_mulPre3L             ## -- Begin function mcl_fpDbl_mulPre3L
	.p2align	4, 0x90
_mcl_fpDbl_mulPre3L:                    ## @mcl_fpDbl_mulPre3L
## %bb.0:
	pushq	%rbp
	pushq	%r15
	pushq	%r14
	pushq	%r13
	pushq	%r12
	pushq	%rbx
	movq	%rdx, %r11
	movq	(%rsi), %r8
	movq	8(%rsi), %r10
	movq	(%rdx), %rcx
	movq	%r8, %rax
	mulq	%rcx
	movq	%rdx, -8(%rsp)                  ## 8-byte Spill
	movq	16(%rsi), %r12
	movq	%rax, (%rdi)
	movq	%r12, %rax
	mulq	%rcx
	movq	%rdx, %r9
	movq	%rax, -16(%rsp)                 ## 8-byte Spill
	movq	%r10, %rax
	mulq	%rcx
	movq	%rax, %rbx
	movq	%rdx, %rcx
	movq	8(%r11), %rsi
	movq	%rsi, %rax
	mulq	%r12
	movq	%rdx, %r13
	movq	%rax, %rbp
	movq	%rsi, %rax
	mulq	%r10
	movq	%rdx, %r14
	movq	%rax, %r15
	movq	%rsi, %rax
	mulq	%r8
	addq	%r15, %rdx
	adcq	%rbp, %r14
	adcq	$0, %r13
	addq	-8(%rsp), %rbx                  ## 8-byte Folded Reload
	adcq	-16(%rsp), %rcx                 ## 8-byte Folded Reload
	adcq	$0, %r9
	addq	%rax, %rbx
	movq	%rbx, 8(%rdi)
	adcq	%rdx, %rcx
	adcq	%r14, %r9
	adcq	$0, %r13
	movq	16(%r11), %rsi
	movq	%rsi, %rax
	mulq	%r12
	movq	%rdx, %rbp
	movq	%rax, %r11
	movq	%rsi, %rax
	mulq	%r10
	movq	%rdx, %rbx
	movq	%rax, %r10
	movq	%rsi, %rax
	mulq	%r8
	addq	%r10, %rdx
	adcq	%r11, %rbx
	adcq	$0, %rbp
	addq	%rcx, %rax
	movq	%rax, 16(%rdi)
	adcq	%r9, %rdx
	movq	%rdx, 24(%rdi)
	adcq	%r13, %rbx
	movq	%rbx, 32(%rdi)
	adcq	$0, %rbp
	movq	%rbp, 40(%rdi)
	popq	%rbx
	popq	%r12
	popq	%r13
	popq	%r14
	popq	%r15
	popq	%rbp
	retq
                                        ## -- End function
	.globl	_mcl_fpDbl_sqrPre3L             ## -- Begin function mcl_fpDbl_sqrPre3L
	.p2align	4, 0x90
_mcl_fpDbl_sqrPre3L:                    ## @mcl_fpDbl_sqrPre3L
## %bb.0:
	pushq	%rbp
	pushq	%r15
	pushq	%r14
	pushq	%r13
	pushq	%r12
	pushq	%rbx
	movq	16(%rsi), %r10
	movq	(%rsi), %r11
	movq	8(%rsi), %rsi
	movq	%r11, %rax
	mulq	%r11
	movq	%rdx, %rcx
	movq	%rax, (%rdi)
	movq	%r10, %rax
	mulq	%rsi
	movq	%rdx, %r8
	movq	%rax, %r9
	movq	%rsi, %rax
	mulq	%rsi
	movq	%rdx, %r14
	movq	%rax, %r12
	movq	%rsi, %rax
	mulq	%r11
	movq	%rax, %r15
	movq	%rdx, %rsi
	addq	%rdx, %r12
	adcq	%r9, %r14
	movq	%r8, %r13
	adcq	$0, %r13
	movq	%r10, %rax
	mulq	%r11
	movq	%rax, %r11
	movq	%rdx, %rbx
	addq	%r15, %rcx
	adcq	%rax, %rsi
	movq	%rdx, %rbp
	adcq	$0, %rbp
	addq	%r15, %rcx
	movq	%rcx, 8(%rdi)
	adcq	%r12, %rsi
	adcq	%r14, %rbp
	adcq	$0, %r13
	movq	%r10, %rax
	mulq	%r10
	addq	%r9, %rbx
	adcq	%r8, %rax
	adcq	$0, %rdx
	addq	%r11, %rsi
	movq	%rsi, 16(%rdi)
	adcq	%rbp, %rbx
	movq	%rbx, 24(%rdi)
	adcq	%r13, %rax
	movq	%rax, 32(%rdi)
	adcq	$0, %rdx
	movq	%rdx, 40(%rdi)
	popq	%rbx
	popq	%r12
	popq	%r13
	popq	%r14
	popq	%r15
	popq	%rbp
	retq
                                        ## -- End function
	.globl	_mcl_fp_mont3L                  ## -- Begin function mcl_fp_mont3L
	.p2align	4, 0x90
_mcl_fp_mont3L:                         ## @mcl_fp_mont3L
## %bb.0:
	pushq	%rbp
	pushq	%r15
	pushq	%r14
	pushq	%r13
	pushq	%r12
	pushq	%rbx
	movq	%rdi, -8(%rsp)                  ## 8-byte Spill
	movq	16(%rsi), %r10
	movq	(%rdx), %rdi
	movq	%rdx, %r11
	movq	%rdx, -64(%rsp)                 ## 8-byte Spill
	movq	%r10, %rax
	mulq	%rdi
	movq	%rax, %rbp
	movq	%rdx, %r15
	movq	(%rsi), %rbx
	movq	%rbx, -16(%rsp)                 ## 8-byte Spill
	movq	8(%rsi), %r14
	movq	%r14, %rax
	movq	%r14, -72(%rsp)                 ## 8-byte Spill
	mulq	%rdi
	movq	%rdx, %r8
	movq	%rax, %rsi
	movq	%rbx, %rax
	mulq	%rdi
	movq	%rax, %r12
	movq	%rdx, %rdi
	addq	%rsi, %rdi
	adcq	%rbp, %r8
	adcq	$0, %r15
	movq	-8(%rcx), %rbp
	movq	%rbp, -32(%rsp)                 ## 8-byte Spill
	imulq	%rax, %rbp
	movq	16(%rcx), %rdx
	movq	%rdx, -56(%rsp)                 ## 8-byte Spill
	movq	%rbp, %rax
	mulq	%rdx
	movq	%rax, %r13
	movq	%rdx, %r9
	movq	(%rcx), %rbx
	movq	%rbx, -48(%rsp)                 ## 8-byte Spill
	movq	8(%rcx), %rcx
	movq	%rcx, -40(%rsp)                 ## 8-byte Spill
	movq	%rbp, %rax
	mulq	%rcx
	movq	%rdx, %rsi
	movq	%rax, %rcx
	movq	%rbp, %rax
	mulq	%rbx
	movq	%rdx, %rbp
	addq	%rcx, %rbp
	adcq	%r13, %rsi
	adcq	$0, %r9
	addq	%r12, %rax
	adcq	%rdi, %rbp
	movq	8(%r11), %rcx
	adcq	%r8, %rsi
	adcq	%r15, %r9
	setb	%r11b
	movq	%rcx, %rax
	mulq	%r10
	movq	%rdx, %r15
	movq	%rax, %rdi
	movq	%rcx, %rax
	mulq	%r14
	movq	%rdx, %r13
	movq	%rax, %r8
	movq	%rcx, %rax
	movq	-16(%rsp), %rcx                 ## 8-byte Reload
	mulq	%rcx
	movq	%rax, %r12
	movq	%rdx, %rbx
	addq	%r8, %rbx
	adcq	%rdi, %r13
	adcq	$0, %r15
	addq	%rbp, %r12
	adcq	%rsi, %rbx
	movzbl	%r11b, %eax
	adcq	%r9, %r13
	adcq	%rax, %r15
	setb	-73(%rsp)                       ## 1-byte Folded Spill
	movq	-32(%rsp), %rsi                 ## 8-byte Reload
	imulq	%r12, %rsi
	movq	%rsi, %rax
	mulq	-56(%rsp)                       ## 8-byte Folded Reload
	movq	%rdx, %r14
	movq	%rax, -24(%rsp)                 ## 8-byte Spill
	movq	%rsi, %rax
	mulq	-48(%rsp)                       ## 8-byte Folded Reload
	movq	%rdx, %rdi
	movq	%rax, %r11
	movq	%rsi, %rax
	mulq	-40(%rsp)                       ## 8-byte Folded Reload
	movq	%rax, %rbp
	movq	%rdx, %rsi
	movq	-64(%rsp), %rax                 ## 8-byte Reload
	movq	16(%rax), %r9
	movq	%r9, %rax
	mulq	%r10
	movq	%rdx, %r8
	movq	%rax, -64(%rsp)                 ## 8-byte Spill
	movq	%r9, %rax
	mulq	-72(%rsp)                       ## 8-byte Folded Reload
	movq	%rax, -72(%rsp)                 ## 8-byte Spill
	movq	%rdx, %r10
	addq	%rdi, %rbp
	adcq	-24(%rsp), %rsi                 ## 8-byte Folded Reload
	adcq	$0, %r14
	addq	%r12, %r11
	adcq	%rbx, %rbp
	adcq	%r13, %rsi
	adcq	%r15, %r14
	movzbl	-73(%rsp), %edi                 ## 1-byte Folded Reload
	adcq	$0, %rdi
	movq	%r9, %rax
	mulq	%rcx
	movq	%rax, %r9
	movq	%rdx, %rcx
	addq	-72(%rsp), %rcx                 ## 8-byte Folded Reload
	adcq	-64(%rsp), %r10                 ## 8-byte Folded Reload
	adcq	$0, %r8
	addq	%rbp, %r9
	adcq	%rsi, %rcx
	adcq	%r14, %r10
	adcq	%rdi, %r8
	setb	%r11b
	movq	-32(%rsp), %rsi                 ## 8-byte Reload
	imulq	%r9, %rsi
	movq	%rsi, %rax
	movq	-56(%rsp), %r14                 ## 8-byte Reload
	mulq	%r14
	movq	%rdx, %rbx
	movq	%rax, %r12
	movq	%rsi, %rax
	movq	-40(%rsp), %r15                 ## 8-byte Reload
	mulq	%r15
	movq	%rdx, %rbp
	movq	%rax, %rdi
	movq	%rsi, %rax
	movq	-48(%rsp), %rsi                 ## 8-byte Reload
	mulq	%rsi
	addq	%rdi, %rdx
	adcq	%r12, %rbp
	adcq	$0, %rbx
	addq	%r9, %rax
	adcq	%rcx, %rdx
	adcq	%r10, %rbp
	movzbl	%r11b, %eax
	adcq	%r8, %rbx
	adcq	$0, %rax
	movq	%rdx, %rdi
	subq	%rsi, %rdi
	movq	%rbp, %rsi
	sbbq	%r15, %rsi
	movq	%rbx, %rcx
	sbbq	%r14, %rcx
	sbbq	$0, %rax
	testb	$1, %al
	cmovneq	%rbx, %rcx
	movq	-8(%rsp), %rax                  ## 8-byte Reload
	movq	%rcx, 16(%rax)
	cmovneq	%rbp, %rsi
	movq	%rsi, 8(%rax)
	cmovneq	%rdx, %rdi
	movq	%rdi, (%rax)
	popq	%rbx
	popq	%r12
	popq	%r13
	popq	%r14
	popq	%r15
	popq	%rbp
	retq
                                        ## -- End function
	.globl	_mcl_fp_montNF3L                ## -- Begin function mcl_fp_montNF3L
	.p2align	4, 0x90
_mcl_fp_montNF3L:                       ## @mcl_fp_montNF3L
## %bb.0:
	pushq	%rbp
	pushq	%r15
	pushq	%r14
	pushq	%r13
	pushq	%r12
	pushq	%rbx
	movq	%rcx, %r8
	movq	%rdx, %r15
	movq	%rdi, -8(%rsp)                  ## 8-byte Spill
	movq	16(%rsi), %rax
	movq	%rax, -48(%rsp)                 ## 8-byte Spill
	movq	(%rdx), %rdi
	movq	%rdx, -16(%rsp)                 ## 8-byte Spill
	mulq	%rdi
	movq	%rax, %rcx
	movq	%rdx, %r13
	movq	(%rsi), %r12
	movq	8(%rsi), %rax
	movq	%rax, -64(%rsp)                 ## 8-byte Spill
	mulq	%rdi
	movq	%rdx, %rbx
	movq	%rax, %rsi
	movq	%r12, %rax
	movq	%r12, -24(%rsp)                 ## 8-byte Spill
	mulq	%rdi
	movq	%rax, %r10
	movq	%rdx, %rdi
	addq	%rsi, %rdi
	adcq	%rcx, %rbx
	adcq	$0, %r13
	movq	-8(%r8), %r11
	movq	%r11, %rbp
	imulq	%rax, %rbp
	movq	16(%r8), %rax
	movq	%rax, -56(%rsp)                 ## 8-byte Spill
	mulq	%rbp
	movq	%rax, %r9
	movq	%rdx, %r14
	movq	(%r8), %rcx
	movq	%rcx, -40(%rsp)                 ## 8-byte Spill
	movq	8(%r8), %rax
	movq	%rax, -32(%rsp)                 ## 8-byte Spill
	mulq	%rbp
	movq	%rdx, %r8
	movq	%rax, %rsi
	movq	%rcx, %rax
	mulq	%rbp
	addq	%r10, %rax
	adcq	%rdi, %rsi
	adcq	%rbx, %r9
	adcq	$0, %r13
	addq	%rdx, %rsi
	movq	8(%r15), %rdi
	adcq	%r8, %r9
	adcq	%r14, %r13
	movq	-48(%rsp), %r14                 ## 8-byte Reload
	movq	%r14, %rax
	mulq	%rdi
	movq	%rdx, %rbx
	movq	%rax, %r8
	movq	-64(%rsp), %rax                 ## 8-byte Reload
	mulq	%rdi
	movq	%rdx, %rbp
	movq	%rax, %rcx
	movq	%r12, %rax
	mulq	%rdi
	movq	%rax, %rdi
	movq	%rdx, %r10
	addq	%rcx, %r10
	adcq	%r8, %rbp
	adcq	$0, %rbx
	addq	%rsi, %rdi
	adcq	%r9, %r10
	adcq	%r13, %rbp
	adcq	$0, %rbx
	movq	%r11, %rsi
	imulq	%rdi, %rsi
	movq	%rsi, %rax
	mulq	-56(%rsp)                       ## 8-byte Folded Reload
	movq	%rdx, %r8
	movq	%rax, %r13
	movq	%rsi, %rax
	movq	-32(%rsp), %r15                 ## 8-byte Reload
	mulq	%r15
	movq	%rdx, %r9
	movq	%rax, %rcx
	movq	%rsi, %rax
	movq	-40(%rsp), %r12                 ## 8-byte Reload
	mulq	%r12
	addq	%rdi, %rax
	adcq	%r10, %rcx
	adcq	%rbp, %r13
	adcq	$0, %rbx
	addq	%rdx, %rcx
	adcq	%r9, %r13
	adcq	%r8, %rbx
	movq	-16(%rsp), %rax                 ## 8-byte Reload
	movq	16(%rax), %rdi
	movq	%rdi, %rax
	mulq	%r14
	movq	%rdx, %rsi
	movq	%rax, %r8
	movq	%rdi, %rax
	mulq	-64(%rsp)                       ## 8-byte Folded Reload
	movq	%rdx, %rbp
	movq	%rax, %r9
	movq	%rdi, %rax
	mulq	-24(%rsp)                       ## 8-byte Folded Reload
	movq	%rax, %r10
	movq	%rdx, %rdi
	addq	%r9, %rdi
	adcq	%r8, %rbp
	adcq	$0, %rsi
	addq	%rcx, %r10
	adcq	%r13, %rdi
	adcq	%rbx, %rbp
	adcq	$0, %rsi
	imulq	%r10, %r11
	movq	-56(%rsp), %r14                 ## 8-byte Reload
	movq	%r14, %rax
	mulq	%r11
	movq	%rdx, %r8
	movq	%rax, %rcx
	movq	%r15, %rax
	mulq	%r11
	movq	%rdx, %r9
	movq	%rax, %rbx
	movq	%r12, %rax
	mulq	%r11
	addq	%r10, %rax
	adcq	%rdi, %rbx
	adcq	%rbp, %rcx
	adcq	$0, %rsi
	addq	%rdx, %rbx
	adcq	%r9, %rcx
	adcq	%r8, %rsi
	movq	%rbx, %rax
	subq	%r12, %rax
	movq	%rcx, %rdx
	sbbq	%r15, %rdx
	movq	%rsi, %rbp
	sbbq	%r14, %rbp
	movq	%rbp, %rdi
	sarq	$63, %rdi
	cmovsq	%rsi, %rbp
	movq	-8(%rsp), %rsi                  ## 8-byte Reload
	movq	%rbp, 16(%rsi)
	cmovsq	%rcx, %rdx
	movq	%rdx, 8(%rsi)
	cmovsq	%rbx, %rax
	movq	%rax, (%rsi)
	popq	%rbx
	popq	%r12
	popq	%r13
	popq	%r14
	popq	%r15
	popq	%rbp
	retq
                                        ## -- End function
	.globl	_mcl_fp_montRed3L               ## -- Begin function mcl_fp_montRed3L
	.p2align	4, 0x90
_mcl_fp_montRed3L:                      ## @mcl_fp_montRed3L
## %bb.0:
	pushq	%rbp
	pushq	%r15
	pushq	%r14
	pushq	%r13
	pushq	%r12
	pushq	%rbx
	movq	%rdx, %rcx
	movq	%rdi, -8(%rsp)                  ## 8-byte Spill
	movq	-8(%rdx), %r9
	movq	(%rdx), %rdi
	movq	(%rsi), %r14
	movq	%r14, %rbx
	imulq	%r9, %rbx
	movq	16(%rdx), %rbp
	movq	%rbx, %rax
	mulq	%rbp
	movq	%rbp, -16(%rsp)                 ## 8-byte Spill
	movq	%rax, %r11
	movq	%rdx, %r8
	movq	8(%rcx), %rcx
	movq	%rcx, -32(%rsp)                 ## 8-byte Spill
	movq	%rbx, %rax
	mulq	%rcx
	movq	%rdx, %r10
	movq	%rax, %rcx
	movq	%rbx, %rax
	mulq	%rdi
	movq	%rdi, -24(%rsp)                 ## 8-byte Spill
	movq	%rdx, %rbx
	addq	%rcx, %rbx
	adcq	%r11, %r10
	adcq	$0, %r8
	addq	%r14, %rax
	adcq	8(%rsi), %rbx
	adcq	16(%rsi), %r10
	adcq	24(%rsi), %r8
	setb	-33(%rsp)                       ## 1-byte Folded Spill
	movq	%r9, %rcx
	imulq	%rbx, %rcx
	movq	%rcx, %rax
	mulq	%rbp
	movq	%rdx, %r14
	movq	%rax, %r15
	movq	%rcx, %rax
	mulq	%rdi
	movq	%rdx, %r12
	movq	%rax, %r13
	movq	%rcx, %rax
	movq	-32(%rsp), %rbp                 ## 8-byte Reload
	mulq	%rbp
	movq	%rdx, %r11
	movq	%rax, %rcx
	addq	%r12, %rcx
	adcq	%r15, %r11
	movzbl	-33(%rsp), %r15d                ## 1-byte Folded Reload
	adcq	%r14, %r15
	addq	%rbx, %r13
	adcq	%r10, %rcx
	adcq	%r8, %r11
	adcq	32(%rsi), %r15
	setb	%dil
	imulq	%rcx, %r9
	movq	%r9, %rax
	movq	-16(%rsp), %r13                 ## 8-byte Reload
	mulq	%r13
	movq	%rdx, %r12
	movq	%rax, %r8
	movq	%r9, %rax
	movq	-24(%rsp), %rbx                 ## 8-byte Reload
	mulq	%rbx
	movq	%rdx, %r10
	movq	%rax, %r14
	movq	%r9, %rax
	mulq	%rbp
	addq	%r10, %rax
	adcq	%r8, %rdx
	movzbl	%dil, %edi
	adcq	%rdi, %r12
	addq	%rcx, %r14
	adcq	%r11, %rax
	adcq	%r15, %rdx
	adcq	40(%rsi), %r12
	xorl	%ecx, %ecx
	movq	%rax, %rsi
	subq	%rbx, %rsi
	movq	%rdx, %rdi
	sbbq	%rbp, %rdi
	movq	%r12, %rbx
	sbbq	%r13, %rbx
	sbbq	%rcx, %rcx
	testb	$1, %cl
	cmovneq	%r12, %rbx
	movq	-8(%rsp), %rcx                  ## 8-byte Reload
	movq	%rbx, 16(%rcx)
	cmovneq	%rdx, %rdi
	movq	%rdi, 8(%rcx)
	cmovneq	%rax, %rsi
	movq	%rsi, (%rcx)
	popq	%rbx
	popq	%r12
	popq	%r13
	popq	%r14
	popq	%r15
	popq	%rbp
	retq
                                        ## -- End function
	.globl	_mcl_fp_montRedNF3L             ## -- Begin function mcl_fp_montRedNF3L
	.p2align	4, 0x90
_mcl_fp_montRedNF3L:                    ## @mcl_fp_montRedNF3L
## %bb.0:
	pushq	%rbp
	pushq	%r15
	pushq	%r14
	pushq	%r13
	pushq	%r12
	pushq	%rbx
	movq	%rdx, %rcx
	movq	%rdi, -8(%rsp)                  ## 8-byte Spill
	movq	-8(%rdx), %r9
	movq	(%rdx), %rbp
	movq	(%rsi), %r14
	movq	%r14, %rbx
	imulq	%r9, %rbx
	movq	16(%rdx), %rdi
	movq	%rbx, %rax
	mulq	%rdi
	movq	%rdi, %r15
	movq	%rdi, -16(%rsp)                 ## 8-byte Spill
	movq	%rax, %r11
	movq	%rdx, %r8
	movq	8(%rcx), %rdi
	movq	%rbx, %rax
	mulq	%rdi
	movq	%rdx, %r10
	movq	%rax, %rcx
	movq	%rbx, %rax
	mulq	%rbp
	movq	%rbp, -24(%rsp)                 ## 8-byte Spill
	movq	%rdx, %rbx
	addq	%rcx, %rbx
	adcq	%r11, %r10
	adcq	$0, %r8
	addq	%r14, %rax
	adcq	8(%rsi), %rbx
	adcq	16(%rsi), %r10
	adcq	24(%rsi), %r8
	setb	-25(%rsp)                       ## 1-byte Folded Spill
	movq	%r9, %rcx
	imulq	%rbx, %rcx
	movq	%rcx, %rax
	mulq	%r15
	movq	%rdx, %r14
	movq	%rax, %r15
	movq	%rcx, %rax
	mulq	%rbp
	movq	%rdx, %r12
	movq	%rax, %r13
	movq	%rcx, %rax
	mulq	%rdi
	movq	%rdx, %r11
	movq	%rax, %rcx
	addq	%r12, %rcx
	adcq	%r15, %r11
	movzbl	-25(%rsp), %r15d                ## 1-byte Folded Reload
	adcq	%r14, %r15
	addq	%rbx, %r13
	adcq	%r10, %rcx
	adcq	%r8, %r11
	adcq	32(%rsi), %r15
	setb	%bpl
	imulq	%rcx, %r9
	movq	%r9, %rax
	movq	-16(%rsp), %r13                 ## 8-byte Reload
	mulq	%r13
	movq	%rdx, %r12
	movq	%rax, %r8
	movq	%r9, %rax
	movq	-24(%rsp), %rbx                 ## 8-byte Reload
	mulq	%rbx
	movq	%rdx, %r10
	movq	%rax, %r14
	movq	%r9, %rax
	mulq	%rdi
	addq	%r10, %rax
	adcq	%r8, %rdx
	movzbl	%bpl, %ebp
	adcq	%rbp, %r12
	addq	%rcx, %r14
	adcq	%r11, %rax
	adcq	%r15, %rdx
	adcq	40(%rsi), %r12
	movq	%rax, %rcx
	subq	%rbx, %rcx
	movq	%rdx, %rsi
	sbbq	%rdi, %rsi
	movq	%r12, %rbx
	sbbq	%r13, %rbx
	movq	%rbx, %rdi
	sarq	$63, %rdi
	cmovsq	%r12, %rbx
	movq	-8(%rsp), %rdi                  ## 8-byte Reload
	movq	%rbx, 16(%rdi)
	cmovsq	%rdx, %rsi
	movq	%rsi, 8(%rdi)
	cmovsq	%rax, %rcx
	movq	%rcx, (%rdi)
	popq	%rbx
	popq	%r12
	popq	%r13
	popq	%r14
	popq	%r15
	popq	%rbp
	retq
                                        ## -- End function
	.globl	_mcl_fp_addPre3L                ## -- Begin function mcl_fp_addPre3L
	.p2align	4, 0x90
_mcl_fp_addPre3L:                       ## @mcl_fp_addPre3L
## %bb.0:
	movq	16(%rsi), %rax
	movq	(%rsi), %rcx
	movq	8(%rsi), %rsi
	addq	(%rdx), %rcx
	adcq	8(%rdx), %rsi
	adcq	16(%rdx), %rax
	movq	%rax, 16(%rdi)
	movq	%rsi, 8(%rdi)
	movq	%rcx, (%rdi)
	setb	%al
	movzbl	%al, %eax
	retq
                                        ## -- End function
	.globl	_mcl_fp_subPre3L                ## -- Begin function mcl_fp_subPre3L
	.p2align	4, 0x90
_mcl_fp_subPre3L:                       ## @mcl_fp_subPre3L
## %bb.0:
	movq	16(%rsi), %rcx
	movq	(%rsi), %r8
	movq	8(%rsi), %rsi
	xorl	%eax, %eax
	subq	(%rdx), %r8
	sbbq	8(%rdx), %rsi
	sbbq	16(%rdx), %rcx
	movq	%rcx, 16(%rdi)
	movq	%rsi, 8(%rdi)
	movq	%r8, (%rdi)
	sbbq	%rax, %rax
	andl	$1, %eax
	retq
                                        ## -- End function
	.globl	_mcl_fp_shr1_3L                 ## -- Begin function mcl_fp_shr1_3L
	.p2align	4, 0x90
_mcl_fp_shr1_3L:                        ## @mcl_fp_shr1_3L
## %bb.0:
	movq	(%rsi), %rax
	movq	8(%rsi), %rcx
	movq	16(%rsi), %rdx
	movq	%rdx, %rsi
	shrq	%rsi
	movq	%rsi, 16(%rdi)
	shldq	$63, %rcx, %rdx
	movq	%rdx, 8(%rdi)
	shrdq	$1, %rcx, %rax
	movq	%rax, (%rdi)
	retq
                                        ## -- End function
	.globl	_mcl_fp_add3L                   ## -- Begin function mcl_fp_add3L
	.p2align	4, 0x90
_mcl_fp_add3L:                          ## @mcl_fp_add3L
## %bb.0:
	movq	16(%rsi), %r8
	movq	(%rsi), %rax
	movq	8(%rsi), %rsi
	addq	(%rdx), %rax
	adcq	8(%rdx), %rsi
	adcq	16(%rdx), %r8
	movq	%r8, 16(%rdi)
	movq	%rsi, 8(%rdi)
	movq	%rax, (%rdi)
	setb	%dl
	movzbl	%dl, %edx
	subq	(%rcx), %rax
	sbbq	8(%rcx), %rsi
	sbbq	16(%rcx), %r8
	sbbq	$0, %rdx
	testb	$1, %dl
	jne	LBB16_2
## %bb.1:                               ## %nocarry
	movq	%rax, (%rdi)
	movq	%rsi, 8(%rdi)
	movq	%r8, 16(%rdi)
LBB16_2:                                ## %carry
	retq
                                        ## -- End function
	.globl	_mcl_fp_addNF3L                 ## -- Begin function mcl_fp_addNF3L
	.p2align	4, 0x90
_mcl_fp_addNF3L:                        ## @mcl_fp_addNF3L
## %bb.0:
	movq	16(%rdx), %r10
	movq	(%rdx), %r8
	movq	8(%rdx), %r9
	addq	(%rsi), %r8
	adcq	8(%rsi), %r9
	adcq	16(%rsi), %r10
	movq	%r8, %rsi
	subq	(%rcx), %rsi
	movq	%r9, %rdx
	sbbq	8(%rcx), %rdx
	movq	%r10, %rax
	sbbq	16(%rcx), %rax
	movq	%rax, %rcx
	sarq	$63, %rcx
	cmovsq	%r10, %rax
	movq	%rax, 16(%rdi)
	cmovsq	%r9, %rdx
	movq	%rdx, 8(%rdi)
	cmovsq	%r8, %rsi
	movq	%rsi, (%rdi)
	retq
                                        ## -- End function
	.globl	_mcl_fp_sub3L                   ## -- Begin function mcl_fp_sub3L
	.p2align	4, 0x90
_mcl_fp_sub3L:                          ## @mcl_fp_sub3L
## %bb.0:
	movq	16(%rsi), %rax
	movq	(%rsi), %r8
	movq	8(%rsi), %rsi
	xorl	%r9d, %r9d
	subq	(%rdx), %r8
	sbbq	8(%rdx), %rsi
	sbbq	16(%rdx), %rax
	movq	%rax, 16(%rdi)
	movq	%rsi, 8(%rdi)
	movq	%r8, (%rdi)
	sbbq	%r9, %r9
	testb	$1, %r9b
	jne	LBB18_2
## %bb.1:                               ## %nocarry
	retq
LBB18_2:                                ## %carry
	addq	(%rcx), %r8
	adcq	8(%rcx), %rsi
	adcq	16(%rcx), %rax
	movq	%rax, 16(%rdi)
	movq	%rsi, 8(%rdi)
	movq	%r8, (%rdi)
	retq
                                        ## -- End function
	.globl	_mcl_fp_subNF3L                 ## -- Begin function mcl_fp_subNF3L
	.p2align	4, 0x90
_mcl_fp_subNF3L:                        ## @mcl_fp_subNF3L
## %bb.0:
	movq	16(%rsi), %r10
	movq	(%rsi), %r8
	movq	8(%rsi), %r9
	subq	(%rdx), %r8
	sbbq	8(%rdx), %r9
	sbbq	16(%rdx), %r10
	movq	%r10, %rdx
	sarq	$63, %rdx
	movq	%rdx, %rsi
	shldq	$1, %r10, %rsi
	andq	(%rcx), %rsi
	movq	16(%rcx), %rax
	andq	%rdx, %rax
	andq	8(%rcx), %rdx
	addq	%r8, %rsi
	movq	%rsi, (%rdi)
	adcq	%r9, %rdx
	movq	%rdx, 8(%rdi)
	adcq	%r10, %rax
	movq	%rax, 16(%rdi)
	retq
                                        ## -- End function
	.globl	_mcl_fpDbl_add3L                ## -- Begin function mcl_fpDbl_add3L
	.p2align	4, 0x90
_mcl_fpDbl_add3L:                       ## @mcl_fpDbl_add3L
## %bb.0:
	movq	40(%rsi), %r10
	movq	32(%rsi), %r9
	movq	24(%rsi), %r8
	movq	16(%rsi), %rax
	movq	(%rsi), %r11
	movq	8(%rsi), %rsi
	addq	(%rdx), %r11
	adcq	8(%rdx), %rsi
	adcq	16(%rdx), %rax
	adcq	24(%rdx), %r8
	adcq	32(%rdx), %r9
	adcq	40(%rdx), %r10
	movq	%rax, 16(%rdi)
	movq	%rsi, 8(%rdi)
	movq	%r11, (%rdi)
	setb	%al
	movzbl	%al, %r11d
	movq	%r8, %rdx
	subq	(%rcx), %rdx
	movq	%r9, %rsi
	sbbq	8(%rcx), %rsi
	movq	%r10, %rax
	sbbq	16(%rcx), %rax
	sbbq	$0, %r11
	testb	$1, %r11b
	cmovneq	%r10, %rax
	movq	%rax, 40(%rdi)
	cmovneq	%r9, %rsi
	movq	%rsi, 32(%rdi)
	cmovneq	%r8, %rdx
	movq	%rdx, 24(%rdi)
	retq
                                        ## -- End function
	.globl	_mcl_fpDbl_sub3L                ## -- Begin function mcl_fpDbl_sub3L
	.p2align	4, 0x90
_mcl_fpDbl_sub3L:                       ## @mcl_fpDbl_sub3L
## %bb.0:
	pushq	%rbx
	movq	40(%rsi), %r8
	movq	32(%rsi), %r9
	movq	24(%rsi), %r10
	movq	16(%rsi), %rax
	movq	(%rsi), %r11
	movq	8(%rsi), %rbx
	xorl	%esi, %esi
	subq	(%rdx), %r11
	sbbq	8(%rdx), %rbx
	sbbq	16(%rdx), %rax
	sbbq	24(%rdx), %r10
	sbbq	32(%rdx), %r9
	sbbq	40(%rdx), %r8
	movq	%rax, 16(%rdi)
	movq	%rbx, 8(%rdi)
	movq	%r11, (%rdi)
	sbbq	%rsi, %rsi
	andl	$1, %esi
	negq	%rsi
	movq	16(%rcx), %rax
	andq	%rsi, %rax
	movq	8(%rcx), %rdx
	andq	%rsi, %rdx
	andq	(%rcx), %rsi
	addq	%r10, %rsi
	movq	%rsi, 24(%rdi)
	adcq	%r9, %rdx
	movq	%rdx, 32(%rdi)
	adcq	%r8, %rax
	movq	%rax, 40(%rdi)
	popq	%rbx
	retq
                                        ## -- End function
	.globl	_mulPv256x64                    ## -- Begin function mulPv256x64
	.p2align	4, 0x90
_mulPv256x64:                           ## @mulPv256x64
## %bb.0:
	pushq	%rbx
	movq	%rdx, %rcx
	movq	%rdx, %rax
	mulq	(%rsi)
	movq	%rdx, %r8
	movq	%rax, (%rdi)
	movq	%rcx, %rax
	mulq	24(%rsi)
	movq	%rdx, %r9
	movq	%rax, %r10
	movq	%rcx, %rax
	mulq	16(%rsi)
	movq	%rdx, %r11
	movq	%rax, %rbx
	movq	%rcx, %rax
	mulq	8(%rsi)
	addq	%r8, %rax
	movq	%rax, 8(%rdi)
	adcq	%rbx, %rdx
	movq	%rdx, 16(%rdi)
	adcq	%r10, %r11
	movq	%r11, 24(%rdi)
	adcq	$0, %r9
	movq	%r9, 32(%rdi)
	movq	%rdi, %rax
	popq	%rbx
	retq
                                        ## -- End function
	.globl	_mcl_fp_mulUnitPre4L            ## -- Begin function mcl_fp_mulUnitPre4L
	.p2align	4, 0x90
_mcl_fp_mulUnitPre4L:                   ## @mcl_fp_mulUnitPre4L
## %bb.0:
	pushq	%r14
	pushq	%rbx
	movq	%rdx, %rcx
	movq	%rdx, %rax
	mulq	24(%rsi)
	movq	%rdx, %r8
	movq	%rax, %r9
	movq	%rcx, %rax
	mulq	16(%rsi)
	movq	%rdx, %r10
	movq	%rax, %r11
	movq	%rcx, %rax
	mulq	8(%rsi)
	movq	%rdx, %rbx
	movq	%rax, %r14
	movq	%rcx, %rax
	mulq	(%rsi)
	movq	%rax, (%rdi)
	addq	%r14, %rdx
	movq	%rdx, 8(%rdi)
	adcq	%r11, %rbx
	movq	%rbx, 16(%rdi)
	adcq	%r9, %r10
	movq	%r10, 24(%rdi)
	adcq	$0, %r8
	movq	%r8, 32(%rdi)
	popq	%rbx
	popq	%r14
	retq
                                        ## -- End function
	.globl	_mcl_fpDbl_mulPre4L             ## -- Begin function mcl_fpDbl_mulPre4L
	.p2align	4, 0x90
_mcl_fpDbl_mulPre4L:                    ## @mcl_fpDbl_mulPre4L
## %bb.0:
	pushq	%rbp
	pushq	%r15
	pushq	%r14
	pushq	%r13
	pushq	%r12
	pushq	%rbx
	movq	%rdx, %rbp
	movq	(%rsi), %rax
	movq	8(%rsi), %r9
	movq	(%rdx), %rbx
	movq	%rax, %r8
	movq	%rax, -8(%rsp)                  ## 8-byte Spill
	mulq	%rbx
	movq	%rdx, -80(%rsp)                 ## 8-byte Spill
	movq	16(%rsi), %r10
	movq	24(%rsi), %r13
	movq	%rax, (%rdi)
	movq	8(%rbp), %rcx
	movq	%rbp, %r11
	movq	%rbp, -48(%rsp)                 ## 8-byte Spill
	movq	%rcx, %rax
	mulq	%r13
	movq	%rdx, -96(%rsp)                 ## 8-byte Spill
	movq	%rax, -24(%rsp)                 ## 8-byte Spill
	movq	%rcx, %rax
	mulq	%r10
	movq	%rdx, -16(%rsp)                 ## 8-byte Spill
	movq	%rax, -40(%rsp)                 ## 8-byte Spill
	movq	%rcx, %rax
	mulq	%r9
	movq	%rdx, -32(%rsp)                 ## 8-byte Spill
	movq	%rax, %r14
	movq	%rcx, %rax
	mulq	%r8
	movq	%rdx, -88(%rsp)                 ## 8-byte Spill
	movq	%rax, %r15
	movq	%r13, %rax
	movq	%r13, -72(%rsp)                 ## 8-byte Spill
	mulq	%rbx
	movq	%rdx, %rsi
	movq	%rax, %r12
	movq	%r10, %rax
	movq	%r10, %r8
	movq	%r10, -56(%rsp)                 ## 8-byte Spill
	mulq	%rbx
	movq	%rdx, %rcx
	movq	%rax, %rbp
	movq	%r9, %rax
	movq	%r9, %r10
	movq	%r9, -64(%rsp)                  ## 8-byte Spill
	mulq	%rbx
	movq	%rdx, %rbx
	addq	-80(%rsp), %rax                 ## 8-byte Folded Reload
	adcq	%rbp, %rbx
	adcq	%r12, %rcx
	adcq	$0, %rsi
	addq	%r15, %rax
	movq	%rax, 8(%rdi)
	adcq	%r14, %rbx
	adcq	-40(%rsp), %rcx                 ## 8-byte Folded Reload
	adcq	-24(%rsp), %rsi                 ## 8-byte Folded Reload
	setb	%al
	addq	-88(%rsp), %rbx                 ## 8-byte Folded Reload
	adcq	-32(%rsp), %rcx                 ## 8-byte Folded Reload
	movzbl	%al, %r14d
	adcq	-16(%rsp), %rsi                 ## 8-byte Folded Reload
	adcq	-96(%rsp), %r14                 ## 8-byte Folded Reload
	movq	16(%r11), %rbp
	movq	%rbp, %rax
	mulq	%r13
	movq	%rdx, %r15
	movq	%rax, -96(%rsp)                 ## 8-byte Spill
	movq	%rbp, %rax
	mulq	%r8
	movq	%rdx, %r12
	movq	%rax, %r9
	movq	%rbp, %rax
	mulq	%r10
	movq	%rdx, %r13
	movq	%rax, %r10
	movq	%rbp, %rax
	movq	-8(%rsp), %r8                   ## 8-byte Reload
	mulq	%r8
	movq	%rdx, %r11
	addq	%r10, %r11
	adcq	%r9, %r13
	adcq	-96(%rsp), %r12                 ## 8-byte Folded Reload
	adcq	$0, %r15
	addq	%rbx, %rax
	adcq	%rcx, %r11
	movq	%rax, 16(%rdi)
	adcq	%rsi, %r13
	adcq	%r14, %r12
	adcq	$0, %r15
	movq	-48(%rsp), %rax                 ## 8-byte Reload
	movq	24(%rax), %rsi
	movq	%rsi, %rax
	mulq	-72(%rsp)                       ## 8-byte Folded Reload
	movq	%rdx, %rcx
	movq	%rax, %r14
	movq	%rsi, %rax
	mulq	-56(%rsp)                       ## 8-byte Folded Reload
	movq	%rdx, %rbx
	movq	%rax, %r9
	movq	%rsi, %rax
	mulq	-64(%rsp)                       ## 8-byte Folded Reload
	movq	%rdx, %rbp
	movq	%rax, %r10
	movq	%rsi, %rax
	mulq	%r8
	addq	%r10, %rdx
	adcq	%r9, %rbp
	adcq	%r14, %rbx
	adcq	$0, %rcx
	addq	%r11, %rax
	movq	%rax, 24(%rdi)
	adcq	%r13, %rdx
	movq	%rdx, 32(%rdi)
	adcq	%r12, %rbp
	movq	%rbp, 40(%rdi)
	adcq	%r15, %rbx
	movq	%rbx, 48(%rdi)
	adcq	$0, %rcx
	movq	%rcx, 56(%rdi)
	popq	%rbx
	popq	%r12
	popq	%r13
	popq	%r14
	popq	%r15
	popq	%rbp
	retq
                                        ## -- End function
	.globl	_mcl_fpDbl_sqrPre4L             ## -- Begin function mcl_fpDbl_sqrPre4L
	.p2align	4, 0x90
_mcl_fpDbl_sqrPre4L:                    ## @mcl_fpDbl_sqrPre4L
## %bb.0:
	pushq	%rbp
	pushq	%r15
	pushq	%r14
	pushq	%r13
	pushq	%r12
	pushq	%rbx
	movq	%rdi, %r10
	movq	24(%rsi), %rbx
	movq	16(%rsi), %rcx
	movq	(%rsi), %r11
	movq	8(%rsi), %r12
	movq	%r11, %rax
	mulq	%r11
	movq	%rdx, %rbp
	movq	%rax, (%rdi)
	movq	%rbx, %rax
	mulq	%rcx
	movq	%rdx, -16(%rsp)                 ## 8-byte Spill
	movq	%rax, -24(%rsp)                 ## 8-byte Spill
	movq	%rbx, %rax
	movq	%rbx, -8(%rsp)                  ## 8-byte Spill
	mulq	%r11
	movq	%rdx, -48(%rsp)                 ## 8-byte Spill
	movq	%rax, -72(%rsp)                 ## 8-byte Spill
	movq	%rcx, %rax
	mulq	%r11
	movq	%rdx, %rsi
	movq	%rax, %r15
	movq	%rax, -56(%rsp)                 ## 8-byte Spill
	movq	%rbx, %rax
	mulq	%r12
	movq	%rdx, %r14
	movq	%rax, %rbx
	movq	%rax, -32(%rsp)                 ## 8-byte Spill
	movq	%rcx, %rax
	mulq	%r12
	movq	%rdx, %r9
	movq	%rax, %rdi
	movq	%rax, -64(%rsp)                 ## 8-byte Spill
	movq	%rcx, %rax
	mulq	%rcx
	movq	%rdx, -40(%rsp)                 ## 8-byte Spill
	movq	%rax, %rcx
	movq	%r12, %rax
	mulq	%r12
	movq	%rdx, %r13
	movq	%rax, %r8
	movq	%r12, %rax
	mulq	%r11
	addq	%rdx, %r8
	adcq	%rdi, %r13
	movq	%r9, %r12
	adcq	%rbx, %r12
	movq	%r14, %r11
	adcq	$0, %r11
	addq	%rax, %rbp
	adcq	%r15, %rdx
	movq	%rsi, %rbx
	adcq	-72(%rsp), %rbx                 ## 8-byte Folded Reload
	movq	-48(%rsp), %rdi                 ## 8-byte Reload
	movq	%rdi, %r15
	adcq	$0, %r15
	addq	%rax, %rbp
	adcq	%r8, %rdx
	movq	%rbp, 8(%r10)
	adcq	%r13, %rbx
	adcq	%r12, %r15
	adcq	$0, %r11
	addq	-64(%rsp), %rsi                 ## 8-byte Folded Reload
	adcq	%r9, %rcx
	movq	-24(%rsp), %r12                 ## 8-byte Reload
	movq	-40(%rsp), %rax                 ## 8-byte Reload
	adcq	%r12, %rax
	movq	-16(%rsp), %r8                  ## 8-byte Reload
	movq	%r8, %rbp
	adcq	$0, %rbp
	addq	-56(%rsp), %rdx                 ## 8-byte Folded Reload
	movq	%rdx, 16(%r10)
	adcq	%rbx, %rsi
	adcq	%r15, %rcx
	adcq	%r11, %rax
	movq	%rax, %r9
	adcq	$0, %rbp
	movq	-8(%rsp), %rax                  ## 8-byte Reload
	mulq	%rax
	addq	-32(%rsp), %rdi                 ## 8-byte Folded Reload
	adcq	%r12, %r14
	adcq	%r8, %rax
	adcq	$0, %rdx
	addq	-72(%rsp), %rsi                 ## 8-byte Folded Reload
	movq	%rsi, 24(%r10)
	adcq	%rcx, %rdi
	movq	%rdi, 32(%r10)
	adcq	%r9, %r14
	movq	%r14, 40(%r10)
	adcq	%rbp, %rax
	movq	%rax, 48(%r10)
	adcq	$0, %rdx
	movq	%rdx, 56(%r10)
	popq	%rbx
	popq	%r12
	popq	%r13
	popq	%r14
	popq	%r15
	popq	%rbp
	retq
                                        ## -- End function
	.globl	_mcl_fp_mont4L                  ## -- Begin function mcl_fp_mont4L
	.p2align	4, 0x90
_mcl_fp_mont4L:                         ## @mcl_fp_mont4L
## %bb.0:
	pushq	%rbp
	pushq	%r15
	pushq	%r14
	pushq	%r13
	pushq	%r12
	pushq	%rbx
	movq	%rdx, -88(%rsp)                 ## 8-byte Spill
	movq	%rdi, -8(%rsp)                  ## 8-byte Spill
	movq	24(%rsi), %rax
	movq	%rax, -40(%rsp)                 ## 8-byte Spill
	movq	(%rdx), %rdi
	mulq	%rdi
	movq	%rax, %r14
	movq	%rdx, %r8
	movq	16(%rsi), %rax
	movq	%rax, -48(%rsp)                 ## 8-byte Spill
	mulq	%rdi
	movq	%rax, %r12
	movq	%rdx, %r9
	movq	(%rsi), %rbx
	movq	%rbx, -56(%rsp)                 ## 8-byte Spill
	movq	8(%rsi), %rax
	movq	%rax, -64(%rsp)                 ## 8-byte Spill
	mulq	%rdi
	movq	%rdx, %r10
	movq	%rax, %rbp
	movq	%rbx, %rax
	mulq	%rdi
	movq	%rax, %r11
	movq	%rdx, %r15
	addq	%rbp, %r15
	adcq	%r12, %r10
	adcq	%r14, %r9
	adcq	$0, %r8
	movq	-8(%rcx), %rdi
	movq	%rdi, -80(%rsp)                 ## 8-byte Spill
	imulq	%rax, %rdi
	movq	24(%rcx), %rdx
	movq	%rdx, -72(%rsp)                 ## 8-byte Spill
	movq	%rdi, %rax
	mulq	%rdx
	movq	%rax, %r12
	movq	%rdx, %r13
	movq	16(%rcx), %rdx
	movq	%rdx, -16(%rsp)                 ## 8-byte Spill
	movq	%rdi, %rax
	mulq	%rdx
	movq	%rax, %r14
	movq	%rdx, %rbx
	movq	(%rcx), %rsi
	movq	%rsi, -24(%rsp)                 ## 8-byte Spill
	movq	8(%rcx), %rcx
	movq	%rcx, -32(%rsp)                 ## 8-byte Spill
	movq	%rdi, %rax
	mulq	%rcx
	movq	%rdx, %rbp
	movq	%rax, %rcx
	movq	%rdi, %rax
	mulq	%rsi
	movq	%rdx, %rdi
	addq	%rcx, %rdi
	adcq	%r14, %rbp
	adcq	%r12, %rbx
	adcq	$0, %r13
	addq	%r11, %rax
	adcq	%r15, %rdi
	adcq	%r10, %rbp
	adcq	%r9, %rbx
	adcq	%r8, %r13
	setb	-96(%rsp)                       ## 1-byte Folded Spill
	movq	-88(%rsp), %rax                 ## 8-byte Reload
	movq	8(%rax), %rcx
	movq	%rcx, %rax
	mulq	-40(%rsp)                       ## 8-byte Folded Reload
	movq	%rdx, %r12
	movq	%rax, %r8
	movq	%rcx, %rax
	mulq	-48(%rsp)                       ## 8-byte Folded Reload
	movq	%rdx, %r14
	movq	%rax, %r11
	movq	%rcx, %rax
	mulq	-64(%rsp)                       ## 8-byte Folded Reload
	movq	%rdx, %rsi
	movq	%rax, %r15
	movq	%rcx, %rax
	mulq	-56(%rsp)                       ## 8-byte Folded Reload
	movq	%rax, %r10
	movq	%rdx, %r9
	addq	%r15, %r9
	adcq	%r11, %rsi
	adcq	%r8, %r14
	adcq	$0, %r12
	addq	%rdi, %r10
	adcq	%rbp, %r9
	adcq	%rbx, %rsi
	adcq	%r13, %r14
	movzbl	-96(%rsp), %eax                 ## 1-byte Folded Reload
	adcq	%rax, %r12
	setb	-96(%rsp)                       ## 1-byte Folded Spill
	movq	-80(%rsp), %rcx                 ## 8-byte Reload
	imulq	%r10, %rcx
	movq	%rcx, %rax
	mulq	-72(%rsp)                       ## 8-byte Folded Reload
	movq	%rdx, %r13
	movq	%rax, %r15
	movq	%rcx, %rax
	mulq	-16(%rsp)                       ## 8-byte Folded Reload
	movq	%rdx, %rbx
	movq	%rax, %rbp
	movq	%rcx, %rax
	mulq	-32(%rsp)                       ## 8-byte Folded Reload
	movq	%rdx, %r8
	movq	%rax, %rdi
	movq	%rcx, %rax
	mulq	-24(%rsp)                       ## 8-byte Folded Reload
	movq	%rdx, %r11
	addq	%rdi, %r11
	adcq	%rbp, %r8
	adcq	%r15, %rbx
	adcq	$0, %r13
	addq	%r10, %rax
	adcq	%r9, %r11
	adcq	%rsi, %r8
	adcq	%r14, %rbx
	adcq	%r12, %r13
	movzbl	-96(%rsp), %r14d                ## 1-byte Folded Reload
	adcq	$0, %r14
	movq	-88(%rsp), %rax                 ## 8-byte Reload
	movq	16(%rax), %rcx
	movq	%rcx, %rax
	mulq	-40(%rsp)                       ## 8-byte Folded Reload
	movq	%rdx, %r12
	movq	%rax, -96(%rsp)                 ## 8-byte Spill
	movq	%rcx, %rax
	mulq	-48(%rsp)                       ## 8-byte Folded Reload
	movq	%rdx, %r10
	movq	%rax, %r15
	movq	%rcx, %rax
	mulq	-64(%rsp)                       ## 8-byte Folded Reload
	movq	%rdx, %rbp
	movq	%rax, %rsi
	movq	%rcx, %rax
	mulq	-56(%rsp)                       ## 8-byte Folded Reload
	movq	%rax, %r9
	movq	%rdx, %rdi
	addq	%rsi, %rdi
	adcq	%r15, %rbp
	adcq	-96(%rsp), %r10                 ## 8-byte Folded Reload
	adcq	$0, %r12
	addq	%r11, %r9
	adcq	%r8, %rdi
	adcq	%rbx, %rbp
	adcq	%r13, %r10
	adcq	%r14, %r12
	setb	%r15b
	movq	-80(%rsp), %rcx                 ## 8-byte Reload
	imulq	%r9, %rcx
	movq	%rcx, %rax
	mulq	-72(%rsp)                       ## 8-byte Folded Reload
	movq	%rdx, %r13
	movq	%rax, %r8
	movq	%rcx, %rax
	mulq	-16(%rsp)                       ## 8-byte Folded Reload
	movq	%rdx, %rbx
	movq	%rax, %r11
	movq	%rcx, %rax
	mulq	-32(%rsp)                       ## 8-byte Folded Reload
	movq	%rdx, %rsi
	movq	%rax, %r14
	movq	%rcx, %rax
	mulq	-24(%rsp)                       ## 8-byte Folded Reload
	movq	%rdx, %rcx
	addq	%r14, %rcx
	adcq	%r11, %rsi
	adcq	%r8, %rbx
	adcq	$0, %r13
	addq	%r9, %rax
	adcq	%rdi, %rcx
	adcq	%rbp, %rsi
	adcq	%r10, %rbx
	adcq	%r12, %r13
	movzbl	%r15b, %r12d
	adcq	$0, %r12
	movq	-88(%rsp), %rax                 ## 8-byte Reload
	movq	24(%rax), %rdi
	movq	%rdi, %rax
	mulq	-40(%rsp)                       ## 8-byte Folded Reload
	movq	%rdx, %r8
	movq	%rax, %r11
	movq	%rdi, %rax
	mulq	-48(%rsp)                       ## 8-byte Folded Reload
	movq	%rdx, %r9
	movq	%rax, %r14
	movq	%rdi, %rax
	mulq	-64(%rsp)                       ## 8-byte Folded Reload
	movq	%rdx, %rbp
	movq	%rax, %r15
	movq	%rdi, %rax
	mulq	-56(%rsp)                       ## 8-byte Folded Reload
	movq	%rax, %r10
	movq	%rdx, %rdi
	addq	%r15, %rdi
	adcq	%r14, %rbp
	adcq	%r11, %r9
	adcq	$0, %r8
	addq	%rcx, %r10
	adcq	%rsi, %rdi
	adcq	%rbx, %rbp
	adcq	%r13, %r9
	adcq	%r12, %r8
	setb	-88(%rsp)                       ## 1-byte Folded Spill
	movq	-80(%rsp), %rcx                 ## 8-byte Reload
	imulq	%r10, %rcx
	movq	%rcx, %rax
	mulq	-72(%rsp)                       ## 8-byte Folded Reload
	movq	%rdx, %r13
	movq	%rax, -80(%rsp)                 ## 8-byte Spill
	movq	%rcx, %rax
	movq	-16(%rsp), %r12                 ## 8-byte Reload
	mulq	%r12
	movq	%rdx, %rbx
	movq	%rax, %r14
	movq	%rcx, %rax
	movq	-32(%rsp), %r11                 ## 8-byte Reload
	mulq	%r11
	movq	%rdx, %rsi
	movq	%rax, %r15
	movq	%rcx, %rax
	movq	-24(%rsp), %rcx                 ## 8-byte Reload
	mulq	%rcx
	addq	%r15, %rdx
	adcq	%r14, %rsi
	adcq	-80(%rsp), %rbx                 ## 8-byte Folded Reload
	adcq	$0, %r13
	addq	%r10, %rax
	adcq	%rdi, %rdx
	adcq	%rbp, %rsi
	adcq	%r9, %rbx
	movzbl	-88(%rsp), %eax                 ## 1-byte Folded Reload
	adcq	%r8, %r13
	adcq	$0, %rax
	movq	%rdx, %r8
	subq	%rcx, %r8
	movq	%rsi, %rcx
	sbbq	%r11, %rcx
	movq	%rbx, %rbp
	sbbq	%r12, %rbp
	movq	%r13, %rdi
	sbbq	-72(%rsp), %rdi                 ## 8-byte Folded Reload
	sbbq	$0, %rax
	testb	$1, %al
	cmovneq	%r13, %rdi
	movq	-8(%rsp), %rax                  ## 8-byte Reload
	movq	%rdi, 24(%rax)
	cmovneq	%rbx, %rbp
	movq	%rbp, 16(%rax)
	cmovneq	%rsi, %rcx
	movq	%rcx, 8(%rax)
	cmovneq	%rdx, %r8
	movq	%r8, (%rax)
	popq	%rbx
	popq	%r12
	popq	%r13
	popq	%r14
	popq	%r15
	popq	%rbp
	retq
                                        ## -- End function
	.globl	_mcl_fp_montNF4L                ## -- Begin function mcl_fp_montNF4L
	.p2align	4, 0x90
_mcl_fp_montNF4L:                       ## @mcl_fp_montNF4L
## %bb.0:
	pushq	%rbp
	pushq	%r15
	pushq	%r14
	pushq	%r13
	pushq	%r12
	pushq	%rbx
	movq	%rdx, -88(%rsp)                 ## 8-byte Spill
	movq	%rdi, -8(%rsp)                  ## 8-byte Spill
	movq	24(%rsi), %rax
	movq	%rax, -48(%rsp)                 ## 8-byte Spill
	movq	(%rdx), %rdi
	mulq	%rdi
	movq	%rax, %r8
	movq	%rdx, %r12
	movq	16(%rsi), %rax
	movq	%rax, -56(%rsp)                 ## 8-byte Spill
	mulq	%rdi
	movq	%rax, %rbp
	movq	%rdx, %r9
	movq	(%rsi), %rbx
	movq	%rbx, -64(%rsp)                 ## 8-byte Spill
	movq	8(%rsi), %rax
	movq	%rax, -72(%rsp)                 ## 8-byte Spill
	mulq	%rdi
	movq	%rdx, %r15
	movq	%rax, %rsi
	movq	%rbx, %rax
	mulq	%rdi
	movq	%rax, %r10
	movq	%rdx, %rdi
	addq	%rsi, %rdi
	adcq	%rbp, %r15
	adcq	%r8, %r9
	adcq	$0, %r12
	movq	-8(%rcx), %rsi
	movq	%rsi, -80(%rsp)                 ## 8-byte Spill
	imulq	%rax, %rsi
	movq	24(%rcx), %rdx
	movq	%rdx, -32(%rsp)                 ## 8-byte Spill
	movq	%rsi, %rax
	mulq	%rdx
	movq	%rax, %r13
	movq	%rdx, %r11
	movq	16(%rcx), %rdx
	movq	%rdx, -40(%rsp)                 ## 8-byte Spill
	movq	%rsi, %rax
	mulq	%rdx
	movq	%rax, %r8
	movq	%rdx, %r14
	movq	(%rcx), %rbx
	movq	%rbx, -16(%rsp)                 ## 8-byte Spill
	movq	8(%rcx), %rcx
	movq	%rcx, -24(%rsp)                 ## 8-byte Spill
	movq	%rsi, %rax
	mulq	%rcx
	movq	%rdx, %rcx
	movq	%rax, %rbp
	movq	%rsi, %rax
	mulq	%rbx
	addq	%r10, %rax
	adcq	%rdi, %rbp
	adcq	%r15, %r8
	adcq	%r9, %r13
	adcq	$0, %r12
	addq	%rdx, %rbp
	adcq	%rcx, %r8
	adcq	%r14, %r13
	adcq	%r11, %r12
	movq	-88(%rsp), %rax                 ## 8-byte Reload
	movq	8(%rax), %rdi
	movq	%rdi, %rax
	mulq	-48(%rsp)                       ## 8-byte Folded Reload
	movq	%rdx, %rbx
	movq	%rax, %rsi
	movq	%rdi, %rax
	mulq	-56(%rsp)                       ## 8-byte Folded Reload
	movq	%rdx, %r10
	movq	%rax, %r11
	movq	%rdi, %rax
	mulq	-72(%rsp)                       ## 8-byte Folded Reload
	movq	%rdx, %rcx
	movq	%rax, %r14
	movq	%rdi, %rax
	mulq	-64(%rsp)                       ## 8-byte Folded Reload
	movq	%rax, %rdi
	movq	%rdx, %r9
	addq	%r14, %r9
	adcq	%r11, %rcx
	adcq	%rsi, %r10
	adcq	$0, %rbx
	addq	%rbp, %rdi
	adcq	%r8, %r9
	adcq	%r13, %rcx
	adcq	%r12, %r10
	adcq	$0, %rbx
	movq	-80(%rsp), %rsi                 ## 8-byte Reload
	imulq	%rdi, %rsi
	movq	%rsi, %rax
	mulq	-32(%rsp)                       ## 8-byte Folded Reload
	movq	%rdx, %r8
	movq	%rax, %r12
	movq	%rsi, %rax
	mulq	-40(%rsp)                       ## 8-byte Folded Reload
	movq	%rdx, %r11
	movq	%rax, %r13
	movq	%rsi, %rax
	movq	-24(%rsp), %r15                 ## 8-byte Reload
	mulq	%r15
	movq	%rdx, %r14
	movq	%rax, %rbp
	movq	%rsi, %rax
	mulq	-16(%rsp)                       ## 8-byte Folded Reload
	addq	%rdi, %rax
	adcq	%r9, %rbp
	adcq	%rcx, %r13
	adcq	%r10, %r12
	adcq	$0, %rbx
	addq	%rdx, %rbp
	adcq	%r14, %r13
	adcq	%r11, %r12
	adcq	%r8, %rbx
	movq	-88(%rsp), %rax                 ## 8-byte Reload
	movq	16(%rax), %rdi
	movq	%rdi, %rax
	mulq	-48(%rsp)                       ## 8-byte Folded Reload
	movq	%rdx, %rsi
	movq	%rax, %r10
	movq	%rdi, %rax
	mulq	-56(%rsp)                       ## 8-byte Folded Reload
	movq	%rdx, %r8
	movq	%rax, %r11
	movq	%rdi, %rax
	mulq	-72(%rsp)                       ## 8-byte Folded Reload
	movq	%rdx, %rcx
	movq	%rax, %r14
	movq	%rdi, %rax
	mulq	-64(%rsp)                       ## 8-byte Folded Reload
	movq	%rax, %r9
	movq	%rdx, %rdi
	addq	%r14, %rdi
	adcq	%r11, %rcx
	adcq	%r10, %r8
	adcq	$0, %rsi
	addq	%rbp, %r9
	adcq	%r13, %rdi
	adcq	%r12, %rcx
	adcq	%rbx, %r8
	adcq	$0, %rsi
	movq	-80(%rsp), %rbx                 ## 8-byte Reload
	imulq	%r9, %rbx
	movq	%rbx, %rax
	mulq	-32(%rsp)                       ## 8-byte Folded Reload
	movq	%rdx, %r10
	movq	%rax, %r12
	movq	%rbx, %rax
	mulq	-40(%rsp)                       ## 8-byte Folded Reload
	movq	%rdx, %r11
	movq	%rax, %r13
	movq	%rbx, %rax
	mulq	%r15
	movq	%rdx, %r14
	movq	%rax, %rbp
	movq	%rbx, %rax
	movq	-16(%rsp), %r15                 ## 8-byte Reload
	mulq	%r15
	addq	%r9, %rax
	adcq	%rdi, %rbp
	adcq	%rcx, %r13
	adcq	%r8, %r12
	adcq	$0, %rsi
	addq	%rdx, %rbp
	adcq	%r14, %r13
	adcq	%r11, %r12
	adcq	%r10, %rsi
	movq	-88(%rsp), %rax                 ## 8-byte Reload
	movq	24(%rax), %rdi
	movq	%rdi, %rax
	mulq	-48(%rsp)                       ## 8-byte Folded Reload
	movq	%rdx, %r11
	movq	%rax, %rcx
	movq	%rdi, %rax
	mulq	-56(%rsp)                       ## 8-byte Folded Reload
	movq	%rdx, %r8
	movq	%rax, %rbx
	movq	%rdi, %rax
	mulq	-72(%rsp)                       ## 8-byte Folded Reload
	movq	%rdx, %r10
	movq	%rax, %r14
	movq	%rdi, %rax
	mulq	-64(%rsp)                       ## 8-byte Folded Reload
	movq	%rax, %r9
	movq	%rdx, %rdi
	addq	%r14, %rdi
	adcq	%rbx, %r10
	adcq	%rcx, %r8
	adcq	$0, %r11
	addq	%rbp, %r9
	adcq	%r13, %rdi
	adcq	%r12, %r10
	adcq	%rsi, %r8
	adcq	$0, %r11
	movq	-80(%rsp), %rsi                 ## 8-byte Reload
	imulq	%r9, %rsi
	movq	%rsi, %rax
	movq	-32(%rsp), %r12                 ## 8-byte Reload
	mulq	%r12
	movq	%rdx, -80(%rsp)                 ## 8-byte Spill
	movq	%rax, %r13
	movq	%rsi, %rax
	movq	-40(%rsp), %r14                 ## 8-byte Reload
	mulq	%r14
	movq	%rdx, -88(%rsp)                 ## 8-byte Spill
	movq	%rax, %rbp
	movq	%rsi, %rax
	movq	%r15, %rbx
	mulq	%r15
	movq	%rdx, %r15
	movq	%rax, %rcx
	movq	%rsi, %rax
	movq	-24(%rsp), %rsi                 ## 8-byte Reload
	mulq	%rsi
	addq	%r9, %rcx
	adcq	%rdi, %rax
	adcq	%r10, %rbp
	adcq	%r8, %r13
	adcq	$0, %r11
	addq	%r15, %rax
	adcq	%rdx, %rbp
	adcq	-88(%rsp), %r13                 ## 8-byte Folded Reload
	adcq	-80(%rsp), %r11                 ## 8-byte Folded Reload
	movq	%rax, %rcx
	subq	%rbx, %rcx
	movq	%rbp, %rdx
	sbbq	%rsi, %rdx
	movq	%r13, %rdi
	sbbq	%r14, %rdi
	movq	%r11, %rbx
	sbbq	%r12, %rbx
	cmovsq	%r11, %rbx
	movq	-8(%rsp), %rsi                  ## 8-byte Reload
	movq	%rbx, 24(%rsi)
	cmovsq	%r13, %rdi
	movq	%rdi, 16(%rsi)
	cmovsq	%rbp, %rdx
	movq	%rdx, 8(%rsi)
	cmovsq	%rax, %rcx
	movq	%rcx, (%rsi)
	popq	%rbx
	popq	%r12
	popq	%r13
	popq	%r14
	popq	%r15
	popq	%rbp
	retq
                                        ## -- End function
	.globl	_mcl_fp_montRed4L               ## -- Begin function mcl_fp_montRed4L
	.p2align	4, 0x90
_mcl_fp_montRed4L:                      ## @mcl_fp_montRed4L
## %bb.0:
	pushq	%rbp
	pushq	%r15
	pushq	%r14
	pushq	%r13
	pushq	%r12
	pushq	%rbx
	movq	%rdx, %rcx
	movq	%rdi, -8(%rsp)                  ## 8-byte Spill
	movq	-8(%rdx), %r8
	movq	(%rdx), %r13
	movq	(%rsi), %r15
	movq	%r15, %rbx
	imulq	%r8, %rbx
	movq	24(%rdx), %rdi
	movq	%rbx, %rax
	mulq	%rdi
	movq	%rdi, -40(%rsp)                 ## 8-byte Spill
	movq	%rax, %r10
	movq	%rdx, %r9
	movq	16(%rcx), %rbp
	movq	%rbx, %rax
	mulq	%rbp
	movq	%rbp, -24(%rsp)                 ## 8-byte Spill
	movq	%rax, %r14
	movq	%rdx, %r11
	movq	8(%rcx), %rcx
	movq	%rcx, -48(%rsp)                 ## 8-byte Spill
	movq	%rbx, %rax
	mulq	%rcx
	movq	%rdx, %r12
	movq	%rax, %rcx
	movq	%rbx, %rax
	mulq	%r13
	movq	%r13, -32(%rsp)                 ## 8-byte Spill
	movq	%rdx, %rbx
	addq	%rcx, %rbx
	adcq	%r14, %r12
	adcq	%r10, %r11
	adcq	$0, %r9
	addq	%r15, %rax
	adcq	8(%rsi), %rbx
	adcq	16(%rsi), %r12
	adcq	24(%rsi), %r11
	adcq	32(%rsi), %r9
	movq	%rsi, -16(%rsp)                 ## 8-byte Spill
	setb	-65(%rsp)                       ## 1-byte Folded Spill
	movq	%r8, %rcx
	imulq	%rbx, %rcx
	movq	%rcx, %rax
	mulq	%rdi
	movq	%rdx, %r10
	movq	%rax, -64(%rsp)                 ## 8-byte Spill
	movq	%rcx, %rax
	mulq	%rbp
	movq	%rdx, %r14
	movq	%rax, %rbp
	movq	%rcx, %rax
	mulq	%r13
	movq	%rdx, %r13
	movq	%rax, %rdi
	movq	%rcx, %rax
	mulq	-48(%rsp)                       ## 8-byte Folded Reload
	movq	%rdx, %r15
	movq	%rax, %rcx
	addq	%r13, %rcx
	adcq	%rbp, %r15
	adcq	-64(%rsp), %r14                 ## 8-byte Folded Reload
	movzbl	-65(%rsp), %eax                 ## 1-byte Folded Reload
	adcq	%rax, %r10
	addq	%rbx, %rdi
	adcq	%r12, %rcx
	adcq	%r11, %r15
	adcq	%r9, %r14
	adcq	40(%rsi), %r10
	setb	-65(%rsp)                       ## 1-byte Folded Spill
	movq	%r8, %rdi
	imulq	%rcx, %rdi
	movq	%rdi, %rax
	movq	-40(%rsp), %rsi                 ## 8-byte Reload
	mulq	%rsi
	movq	%rdx, %r9
	movq	%rax, -64(%rsp)                 ## 8-byte Spill
	movq	%rdi, %rax
	mulq	-24(%rsp)                       ## 8-byte Folded Reload
	movq	%rdx, %r11
	movq	%rax, -56(%rsp)                 ## 8-byte Spill
	movq	%rdi, %rax
	mulq	-32(%rsp)                       ## 8-byte Folded Reload
	movq	%rdx, %r13
	movq	%rax, %rbp
	movq	%rdi, %rax
	movq	-48(%rsp), %rdi                 ## 8-byte Reload
	mulq	%rdi
	movq	%rdx, %r12
	movq	%rax, %rbx
	addq	%r13, %rbx
	adcq	-56(%rsp), %r12                 ## 8-byte Folded Reload
	adcq	-64(%rsp), %r11                 ## 8-byte Folded Reload
	movzbl	-65(%rsp), %eax                 ## 1-byte Folded Reload
	adcq	%rax, %r9
	addq	%rcx, %rbp
	adcq	%r15, %rbx
	adcq	%r14, %r12
	adcq	%r10, %r11
	movq	-16(%rsp), %r15                 ## 8-byte Reload
	adcq	48(%r15), %r9
	setb	-65(%rsp)                       ## 1-byte Folded Spill
	imulq	%rbx, %r8
	movq	%r8, %rax
	mulq	%rsi
	movq	%rdx, -64(%rsp)                 ## 8-byte Spill
	movq	%rax, -56(%rsp)                 ## 8-byte Spill
	movq	%r8, %rax
	movq	-24(%rsp), %r14                 ## 8-byte Reload
	mulq	%r14
	movq	%rdx, %r13
	movq	%rax, %rbp
	movq	%r8, %rax
	movq	-32(%rsp), %r10                 ## 8-byte Reload
	mulq	%r10
	movq	%rdx, %rsi
	movq	%rax, %rcx
	movq	%r8, %rax
	mulq	%rdi
	addq	%rsi, %rax
	adcq	%rbp, %rdx
	adcq	-56(%rsp), %r13                 ## 8-byte Folded Reload
	movzbl	-65(%rsp), %edi                 ## 1-byte Folded Reload
	adcq	-64(%rsp), %rdi                 ## 8-byte Folded Reload
	addq	%rbx, %rcx
	adcq	%r12, %rax
	adcq	%r11, %rdx
	adcq	%r9, %r13
	adcq	56(%r15), %rdi
	xorl	%r8d, %r8d
	movq	%rax, %rbp
	subq	%r10, %rbp
	movq	%rdx, %rbx
	sbbq	-48(%rsp), %rbx                 ## 8-byte Folded Reload
	movq	%r13, %rcx
	sbbq	%r14, %rcx
	movq	%rdi, %rsi
	sbbq	-40(%rsp), %rsi                 ## 8-byte Folded Reload
	sbbq	%r8, %r8
	testb	$1, %r8b
	cmovneq	%rdi, %rsi
	movq	-8(%rsp), %rdi                  ## 8-byte Reload
	movq	%rsi, 24(%rdi)
	cmovneq	%r13, %rcx
	movq	%rcx, 16(%rdi)
	cmovneq	%rdx, %rbx
	movq	%rbx, 8(%rdi)
	cmovneq	%rax, %rbp
	movq	%rbp, (%rdi)
	popq	%rbx
	popq	%r12
	popq	%r13
	popq	%r14
	popq	%r15
	popq	%rbp
	retq
                                        ## -- End function
	.globl	_mcl_fp_montRedNF4L             ## -- Begin function mcl_fp_montRedNF4L
	.p2align	4, 0x90
_mcl_fp_montRedNF4L:                    ## @mcl_fp_montRedNF4L
## %bb.0:
	pushq	%rbp
	pushq	%r15
	pushq	%r14
	pushq	%r13
	pushq	%r12
	pushq	%rbx
	movq	%rdx, %rcx
	movq	%rdi, -8(%rsp)                  ## 8-byte Spill
	movq	-8(%rdx), %r8
	movq	(%rdx), %r13
	movq	(%rsi), %r15
	movq	%r15, %rbx
	imulq	%r8, %rbx
	movq	24(%rdx), %rdi
	movq	%rbx, %rax
	mulq	%rdi
	movq	%rdi, -48(%rsp)                 ## 8-byte Spill
	movq	%rax, %r10
	movq	%rdx, %r9
	movq	16(%rcx), %rbp
	movq	%rbx, %rax
	mulq	%rbp
	movq	%rbp, -32(%rsp)                 ## 8-byte Spill
	movq	%rax, %r14
	movq	%rdx, %r11
	movq	8(%rcx), %rcx
	movq	%rcx, -24(%rsp)                 ## 8-byte Spill
	movq	%rbx, %rax
	mulq	%rcx
	movq	%rdx, %r12
	movq	%rax, %rcx
	movq	%rbx, %rax
	mulq	%r13
	movq	%r13, -40(%rsp)                 ## 8-byte Spill
	movq	%rdx, %rbx
	addq	%rcx, %rbx
	adcq	%r14, %r12
	adcq	%r10, %r11
	adcq	$0, %r9
	addq	%r15, %rax
	adcq	8(%rsi), %rbx
	adcq	16(%rsi), %r12
	adcq	24(%rsi), %r11
	adcq	32(%rsi), %r9
	movq	%rsi, -16(%rsp)                 ## 8-byte Spill
	setb	-65(%rsp)                       ## 1-byte Folded Spill
	movq	%r8, %rcx
	imulq	%rbx, %rcx
	movq	%rcx, %rax
	mulq	%rdi
	movq	%rdx, %r10
	movq	%rax, -64(%rsp)                 ## 8-byte Spill
	movq	%rcx, %rax
	mulq	%rbp
	movq	%rdx, %r14
	movq	%rax, %rbp
	movq	%rcx, %rax
	mulq	%r13
	movq	%rdx, %r13
	movq	%rax, %rdi
	movq	%rcx, %rax
	mulq	-24(%rsp)                       ## 8-byte Folded Reload
	movq	%rdx, %r15
	movq	%rax, %rcx
	addq	%r13, %rcx
	adcq	%rbp, %r15
	adcq	-64(%rsp), %r14                 ## 8-byte Folded Reload
	movzbl	-65(%rsp), %eax                 ## 1-byte Folded Reload
	adcq	%rax, %r10
	addq	%rbx, %rdi
	adcq	%r12, %rcx
	adcq	%r11, %r15
	adcq	%r9, %r14
	adcq	40(%rsi), %r10
	setb	-65(%rsp)                       ## 1-byte Folded Spill
	movq	%r8, %rdi
	imulq	%rcx, %rdi
	movq	%rdi, %rax
	movq	-48(%rsp), %rsi                 ## 8-byte Reload
	mulq	%rsi
	movq	%rdx, %r9
	movq	%rax, -64(%rsp)                 ## 8-byte Spill
	movq	%rdi, %rax
	mulq	-32(%rsp)                       ## 8-byte Folded Reload
	movq	%rdx, %r11
	movq	%rax, -56(%rsp)                 ## 8-byte Spill
	movq	%rdi, %rax
	mulq	-40(%rsp)                       ## 8-byte Folded Reload
	movq	%rdx, %r13
	movq	%rax, %rbp
	movq	%rdi, %rax
	movq	-24(%rsp), %rdi                 ## 8-byte Reload
	mulq	%rdi
	movq	%rdx, %r12
	movq	%rax, %rbx
	addq	%r13, %rbx
	adcq	-56(%rsp), %r12                 ## 8-byte Folded Reload
	adcq	-64(%rsp), %r11                 ## 8-byte Folded Reload
	movzbl	-65(%rsp), %eax                 ## 1-byte Folded Reload
	adcq	%rax, %r9
	addq	%rcx, %rbp
	adcq	%r15, %rbx
	adcq	%r14, %r12
	adcq	%r10, %r11
	movq	-16(%rsp), %r15                 ## 8-byte Reload
	adcq	48(%r15), %r9
	setb	-65(%rsp)                       ## 1-byte Folded Spill
	imulq	%rbx, %r8
	movq	%r8, %rax
	mulq	%rsi
	movq	%rdx, -64(%rsp)                 ## 8-byte Spill
	movq	%rax, -56(%rsp)                 ## 8-byte Spill
	movq	%r8, %rax
	movq	-32(%rsp), %r14                 ## 8-byte Reload
	mulq	%r14
	movq	%rdx, %r13
	movq	%rax, %rbp
	movq	%r8, %rax
	movq	-40(%rsp), %r10                 ## 8-byte Reload
	mulq	%r10
	movq	%rdx, %rsi
	movq	%rax, %rcx
	movq	%r8, %rax
	mulq	%rdi
	movq	%rdi, %r8
	addq	%rsi, %rax
	adcq	%rbp, %rdx
	adcq	-56(%rsp), %r13                 ## 8-byte Folded Reload
	movzbl	-65(%rsp), %edi                 ## 1-byte Folded Reload
	adcq	-64(%rsp), %rdi                 ## 8-byte Folded Reload
	addq	%rbx, %rcx
	adcq	%r12, %rax
	adcq	%r11, %rdx
	adcq	%r9, %r13
	adcq	56(%r15), %rdi
	movq	%rax, %rbx
	subq	%r10, %rbx
	movq	%rdx, %rbp
	sbbq	%r8, %rbp
	movq	%r13, %rcx
	sbbq	%r14, %rcx
	movq	%rdi, %rsi
	sbbq	-48(%rsp), %rsi                 ## 8-byte Folded Reload
	cmovsq	%rdi, %rsi
	movq	-8(%rsp), %rdi                  ## 8-byte Reload
	movq	%rsi, 24(%rdi)
	cmovsq	%r13, %rcx
	movq	%rcx, 16(%rdi)
	cmovsq	%rdx, %rbp
	movq	%rbp, 8(%rdi)
	cmovsq	%rax, %rbx
	movq	%rbx, (%rdi)
	popq	%rbx
	popq	%r12
	popq	%r13
	popq	%r14
	popq	%r15
	popq	%rbp
	retq
                                        ## -- End function
	.globl	_mcl_fp_addPre4L                ## -- Begin function mcl_fp_addPre4L
	.p2align	4, 0x90
_mcl_fp_addPre4L:                       ## @mcl_fp_addPre4L
## %bb.0:
	movq	24(%rsi), %rax
	movq	16(%rsi), %rcx
	movq	(%rsi), %r8
	movq	8(%rsi), %rsi
	addq	(%rdx), %r8
	adcq	8(%rdx), %rsi
	adcq	16(%rdx), %rcx
	adcq	24(%rdx), %rax
	movq	%rax, 24(%rdi)
	movq	%rcx, 16(%rdi)
	movq	%rsi, 8(%rdi)
	movq	%r8, (%rdi)
	setb	%al
	movzbl	%al, %eax
	retq
                                        ## -- End function
	.globl	_mcl_fp_subPre4L                ## -- Begin function mcl_fp_subPre4L
	.p2align	4, 0x90
_mcl_fp_subPre4L:                       ## @mcl_fp_subPre4L
## %bb.0:
	movq	24(%rsi), %rcx
	movq	16(%rsi), %r8
	movq	(%rsi), %r9
	movq	8(%rsi), %rsi
	xorl	%eax, %eax
	subq	(%rdx), %r9
	sbbq	8(%rdx), %rsi
	sbbq	16(%rdx), %r8
	sbbq	24(%rdx), %rcx
	movq	%rcx, 24(%rdi)
	movq	%r8, 16(%rdi)
	movq	%rsi, 8(%rdi)
	movq	%r9, (%rdi)
	sbbq	%rax, %rax
	andl	$1, %eax
	retq
                                        ## -- End function
	.globl	_mcl_fp_shr1_4L                 ## -- Begin function mcl_fp_shr1_4L
	.p2align	4, 0x90
_mcl_fp_shr1_4L:                        ## @mcl_fp_shr1_4L
## %bb.0:
	movq	(%rsi), %rax
	movq	8(%rsi), %r8
	movq	16(%rsi), %rdx
	movq	24(%rsi), %rcx
	movq	%rcx, %rsi
	shrq	%rsi
	movq	%rsi, 24(%rdi)
	shldq	$63, %rdx, %rcx
	movq	%rcx, 16(%rdi)
	shldq	$63, %r8, %rdx
	movq	%rdx, 8(%rdi)
	shrdq	$1, %r8, %rax
	movq	%rax, (%rdi)
	retq
                                        ## -- End function
	.globl	_mcl_fp_add4L                   ## -- Begin function mcl_fp_add4L
	.p2align	4, 0x90
_mcl_fp_add4L:                          ## @mcl_fp_add4L
## %bb.0:
	movq	24(%rsi), %r8
	movq	16(%rsi), %r9
	movq	(%rsi), %rax
	movq	8(%rsi), %rsi
	addq	(%rdx), %rax
	adcq	8(%rdx), %rsi
	adcq	16(%rdx), %r9
	adcq	24(%rdx), %r8
	movq	%r8, 24(%rdi)
	movq	%r9, 16(%rdi)
	movq	%rsi, 8(%rdi)
	movq	%rax, (%rdi)
	setb	%dl
	movzbl	%dl, %edx
	subq	(%rcx), %rax
	sbbq	8(%rcx), %rsi
	sbbq	16(%rcx), %r9
	sbbq	24(%rcx), %r8
	sbbq	$0, %rdx
	testb	$1, %dl
	jne	LBB33_2
## %bb.1:                               ## %nocarry
	movq	%rax, (%rdi)
	movq	%rsi, 8(%rdi)
	movq	%r9, 16(%rdi)
	movq	%r8, 24(%rdi)
LBB33_2:                                ## %carry
	retq
                                        ## -- End function
	.globl	_mcl_fp_addNF4L                 ## -- Begin function mcl_fp_addNF4L
	.p2align	4, 0x90
_mcl_fp_addNF4L:                        ## @mcl_fp_addNF4L
## %bb.0:
	pushq	%rbx
	movq	24(%rdx), %r11
	movq	16(%rdx), %r8
	movq	(%rdx), %r9
	movq	8(%rdx), %r10
	addq	(%rsi), %r9
	adcq	8(%rsi), %r10
	adcq	16(%rsi), %r8
	adcq	24(%rsi), %r11
	movq	%r9, %rsi
	subq	(%rcx), %rsi
	movq	%r10, %rdx
	sbbq	8(%rcx), %rdx
	movq	%r8, %rax
	sbbq	16(%rcx), %rax
	movq	%r11, %rbx
	sbbq	24(%rcx), %rbx
	cmovsq	%r11, %rbx
	movq	%rbx, 24(%rdi)
	cmovsq	%r8, %rax
	movq	%rax, 16(%rdi)
	cmovsq	%r10, %rdx
	movq	%rdx, 8(%rdi)
	cmovsq	%r9, %rsi
	movq	%rsi, (%rdi)
	popq	%rbx
	retq
                                        ## -- End function
	.globl	_mcl_fp_sub4L                   ## -- Begin function mcl_fp_sub4L
	.p2align	4, 0x90
_mcl_fp_sub4L:                          ## @mcl_fp_sub4L
## %bb.0:
	movq	24(%rsi), %r9
	movq	16(%rsi), %r10
	movq	(%rsi), %r8
	movq	8(%rsi), %rsi
	xorl	%eax, %eax
	subq	(%rdx), %r8
	sbbq	8(%rdx), %rsi
	sbbq	16(%rdx), %r10
	sbbq	24(%rdx), %r9
	movq	%r9, 24(%rdi)
	movq	%r10, 16(%rdi)
	movq	%rsi, 8(%rdi)
	movq	%r8, (%rdi)
	sbbq	%rax, %rax
	testb	$1, %al
	jne	LBB35_2
## %bb.1:                               ## %nocarry
	retq
LBB35_2:                                ## %carry
	addq	(%rcx), %r8
	adcq	8(%rcx), %rsi
	adcq	16(%rcx), %r10
	adcq	24(%rcx), %r9
	movq	%r9, 24(%rdi)
	movq	%r10, 16(%rdi)
	movq	%rsi, 8(%rdi)
	movq	%r8, (%rdi)
	retq
                                        ## -- End function
	.globl	_mcl_fp_subNF4L                 ## -- Begin function mcl_fp_subNF4L
	.p2align	4, 0x90
_mcl_fp_subNF4L:                        ## @mcl_fp_subNF4L
## %bb.0:
	pushq	%rbx
	movq	24(%rsi), %r11
	movq	16(%rsi), %r8
	movq	(%rsi), %r9
	movq	8(%rsi), %r10
	subq	(%rdx), %r9
	sbbq	8(%rdx), %r10
	sbbq	16(%rdx), %r8
	sbbq	24(%rdx), %r11
	movq	%r11, %rdx
	sarq	$63, %rdx
	movq	24(%rcx), %rsi
	andq	%rdx, %rsi
	movq	16(%rcx), %rax
	andq	%rdx, %rax
	movq	8(%rcx), %rbx
	andq	%rdx, %rbx
	andq	(%rcx), %rdx
	addq	%r9, %rdx
	movq	%rdx, (%rdi)
	adcq	%r10, %rbx
	movq	%rbx, 8(%rdi)
	adcq	%r8, %rax
	movq	%rax, 16(%rdi)
	adcq	%r11, %rsi
	movq	%rsi, 24(%rdi)
	popq	%rbx
	retq
                                        ## -- End function
	.globl	_mcl_fpDbl_add4L                ## -- Begin function mcl_fpDbl_add4L
	.p2align	4, 0x90
_mcl_fpDbl_add4L:                       ## @mcl_fpDbl_add4L
## %bb.0:
	pushq	%r14
	pushq	%rbx
	movq	56(%rsi), %r11
	movq	48(%rsi), %r10
	movq	40(%rsi), %r9
	movq	32(%rsi), %r8
	movq	24(%rsi), %rax
	movq	16(%rsi), %rbx
	movq	(%rsi), %r14
	movq	8(%rsi), %rsi
	addq	(%rdx), %r14
	adcq	8(%rdx), %rsi
	adcq	16(%rdx), %rbx
	adcq	24(%rdx), %rax
	adcq	32(%rdx), %r8
	adcq	40(%rdx), %r9
	adcq	48(%rdx), %r10
	adcq	56(%rdx), %r11
	movq	%rax, 24(%rdi)
	movq	%rbx, 16(%rdi)
	movq	%rsi, 8(%rdi)
	movq	%r14, (%rdi)
	setb	%al
	movzbl	%al, %r14d
	movq	%r8, %rdx
	subq	(%rcx), %rdx
	movq	%r9, %rsi
	sbbq	8(%rcx), %rsi
	movq	%r10, %rbx
	sbbq	16(%rcx), %rbx
	movq	%r11, %rax
	sbbq	24(%rcx), %rax
	sbbq	$0, %r14
	testb	$1, %r14b
	cmovneq	%r11, %rax
	movq	%rax, 56(%rdi)
	cmovneq	%r10, %rbx
	movq	%rbx, 48(%rdi)
	cmovneq	%r9, %rsi
	movq	%rsi, 40(%rdi)
	cmovneq	%r8, %rdx
	movq	%rdx, 32(%rdi)
	popq	%rbx
	popq	%r14
	retq
                                        ## -- End function
	.globl	_mcl_fpDbl_sub4L                ## -- Begin function mcl_fpDbl_sub4L
	.p2align	4, 0x90
_mcl_fpDbl_sub4L:                       ## @mcl_fpDbl_sub4L
## %bb.0:
	pushq	%r15
	pushq	%r14
	pushq	%rbx
	movq	56(%rsi), %r8
	movq	48(%rsi), %r9
	movq	40(%rsi), %r10
	movq	32(%rsi), %r11
	movq	24(%rsi), %r15
	movq	16(%rsi), %rbx
	movq	(%rsi), %r14
	movq	8(%rsi), %rax
	xorl	%esi, %esi
	subq	(%rdx), %r14
	sbbq	8(%rdx), %rax
	sbbq	16(%rdx), %rbx
	sbbq	24(%rdx), %r15
	sbbq	32(%rdx), %r11
	sbbq	40(%rdx), %r10
	sbbq	48(%rdx), %r9
	sbbq	56(%rdx), %r8
	movq	%r15, 24(%rdi)
	movq	%rbx, 16(%rdi)
	movq	%rax, 8(%rdi)
	movq	%r14, (%rdi)
	sbbq	%rsi, %rsi
	andl	$1, %esi
	negq	%rsi
	movq	24(%rcx), %rax
	andq	%rsi, %rax
	movq	16(%rcx), %rdx
	andq	%rsi, %rdx
	movq	8(%rcx), %rbx
	andq	%rsi, %rbx
	andq	(%rcx), %rsi
	addq	%r11, %rsi
	movq	%rsi, 32(%rdi)
	adcq	%r10, %rbx
	movq	%rbx, 40(%rdi)
	adcq	%r9, %rdx
	movq	%rdx, 48(%rdi)
	adcq	%r8, %rax
	movq	%rax, 56(%rdi)
	popq	%rbx
	popq	%r14
	popq	%r15
	retq
                                        ## -- End function
	.globl	_mulPv384x64                    ## -- Begin function mulPv384x64
	.p2align	4, 0x90
_mulPv384x64:                           ## @mulPv384x64
## %bb.0:
	pushq	%r15
	pushq	%r14
	pushq	%r13
	pushq	%r12
	pushq	%rbx
	movq	%rdx, %rcx
	movq	%rdx, %rax
	mulq	(%rsi)
	movq	%rdx, %r9
	movq	%rax, (%rdi)
	movq	%rcx, %rax
	mulq	40(%rsi)
	movq	%rdx, %r8
	movq	%rax, %r10
	movq	%rcx, %rax
	mulq	32(%rsi)
	movq	%rdx, %r11
	movq	%rax, %r14
	movq	%rcx, %rax
	mulq	24(%rsi)
	movq	%rdx, %r12
	movq	%rax, %r15
	movq	%rcx, %rax
	mulq	16(%rsi)
	movq	%rdx, %rbx
	movq	%rax, %r13
	movq	%rcx, %rax
	mulq	8(%rsi)
	addq	%r9, %rax
	movq	%rax, 8(%rdi)
	adcq	%r13, %rdx
	movq	%rdx, 16(%rdi)
	adcq	%r15, %rbx
	movq	%rbx, 24(%rdi)
	adcq	%r14, %r12
	movq	%r12, 32(%rdi)
	adcq	%r10, %r11
	movq	%r11, 40(%rdi)
	adcq	$0, %r8
	movq	%r8, 48(%rdi)
	movq	%rdi, %rax
	popq	%rbx
	popq	%r12
	popq	%r13
	popq	%r14
	popq	%r15
	retq
                                        ## -- End function
	.globl	_mcl_fp_mulUnitPre6L            ## -- Begin function mcl_fp_mulUnitPre6L
	.p2align	4, 0x90
_mcl_fp_mulUnitPre6L:                   ## @mcl_fp_mulUnitPre6L
## %bb.0:
	pushq	%rbp
	pushq	%r15
	pushq	%r14
	pushq	%r13
	pushq	%r12
	pushq	%rbx
	movq	%rdx, %rcx
	movq	%rdx, %rax
	mulq	40(%rsi)
	movq	%rdx, %r9
	movq	%rax, %r8
	movq	%rcx, %rax
	mulq	32(%rsi)
	movq	%rdx, %r10
	movq	%rax, %r11
	movq	%rcx, %rax
	mulq	24(%rsi)
	movq	%rdx, %r15
	movq	%rax, %r14
	movq	%rcx, %rax
	mulq	16(%rsi)
	movq	%rdx, %r13
	movq	%rax, %r12
	movq	%rcx, %rax
	mulq	8(%rsi)
	movq	%rdx, %rbx
	movq	%rax, %rbp
	movq	%rcx, %rax
	mulq	(%rsi)
	movq	%rax, (%rdi)
	addq	%rbp, %rdx
	movq	%rdx, 8(%rdi)
	adcq	%r12, %rbx
	movq	%rbx, 16(%rdi)
	adcq	%r14, %r13
	movq	%r13, 24(%rdi)
	adcq	%r11, %r15
	movq	%r15, 32(%rdi)
	adcq	%r8, %r10
	movq	%r10, 40(%rdi)
	adcq	$0, %r9
	movq	%r9, 48(%rdi)
	popq	%rbx
	popq	%r12
	popq	%r13
	popq	%r14
	popq	%r15
	popq	%rbp
	retq
                                        ## -- End function
	.globl	_mcl_fpDbl_mulPre6L             ## -- Begin function mcl_fpDbl_mulPre6L
	.p2align	4, 0x90
_mcl_fpDbl_mulPre6L:                    ## @mcl_fpDbl_mulPre6L
## %bb.0:
	pushq	%rbp
	pushq	%r15
	pushq	%r14
	pushq	%r13
	pushq	%r12
	pushq	%rbx
	movq	%rdx, -64(%rsp)                 ## 8-byte Spill
	movq	%rdi, -48(%rsp)                 ## 8-byte Spill
	movq	(%rsi), %rax
	movq	%rax, -56(%rsp)                 ## 8-byte Spill
	movq	8(%rsi), %r14
	movq	(%rdx), %rbx
	mulq	%rbx
	movq	%rdx, %r12
	movq	16(%rsi), %r13
	movq	24(%rsi), %r8
	movq	32(%rsi), %r10
	movq	40(%rsi), %rdx
	movq	%rdx, -72(%rsp)                 ## 8-byte Spill
	movq	%rax, (%rdi)
	movq	%rdx, %rax
	mulq	%rbx
	movq	%rdx, %rcx
	movq	%rax, -104(%rsp)                ## 8-byte Spill
	movq	%r10, %rax
	mulq	%rbx
	movq	%rdx, %rbp
	movq	%rax, -112(%rsp)                ## 8-byte Spill
	movq	%r8, %rax
	mulq	%rbx
	movq	%rdx, %r11
	movq	%rax, -88(%rsp)                 ## 8-byte Spill
	movq	%r13, %rax
	movq	%r13, %r9
	movq	%r13, -32(%rsp)                 ## 8-byte Spill
	mulq	%rbx
	movq	%rdx, %r13
	movq	%rax, %r15
	movq	%r14, %rax
	movq	%r14, -40(%rsp)                 ## 8-byte Spill
	mulq	%rbx
	movq	%rdx, %rsi
	movq	%rax, %rdi
	addq	%r12, %rdi
	adcq	%r15, %rsi
	adcq	-88(%rsp), %r13                 ## 8-byte Folded Reload
	adcq	-112(%rsp), %r11                ## 8-byte Folded Reload
	adcq	-104(%rsp), %rbp                ## 8-byte Folded Reload
	movq	%rbp, -24(%rsp)                 ## 8-byte Spill
	adcq	$0, %rcx
	movq	%rcx, -80(%rsp)                 ## 8-byte Spill
	movq	-64(%rsp), %rax                 ## 8-byte Reload
	movq	8(%rax), %r15
	movq	%r15, %rax
	mulq	-72(%rsp)                       ## 8-byte Folded Reload
	movq	%rdx, -104(%rsp)                ## 8-byte Spill
	movq	%rax, -112(%rsp)                ## 8-byte Spill
	movq	%r15, %rax
	mulq	%r10
	movq	%r10, -16(%rsp)                 ## 8-byte Spill
	movq	%rdx, -88(%rsp)                 ## 8-byte Spill
	movq	%rax, %r12
	movq	%r15, %rax
	mulq	%r8
	movq	%r8, -8(%rsp)                   ## 8-byte Spill
	movq	%rdx, -96(%rsp)                 ## 8-byte Spill
	movq	%rax, %rbp
	movq	%r15, %rax
	mulq	%r9
	movq	%rdx, %r9
	movq	%rax, %rcx
	movq	%r15, %rax
	mulq	%r14
	movq	%rdx, %r14
	movq	%rax, %rbx
	movq	%r15, %rax
	mulq	-56(%rsp)                       ## 8-byte Folded Reload
	addq	%rdi, %rax
	movq	-48(%rsp), %rdi                 ## 8-byte Reload
	movq	%rax, 8(%rdi)
	adcq	%rsi, %rbx
	adcq	%r13, %rcx
	adcq	%r11, %rbp
	adcq	-24(%rsp), %r12                 ## 8-byte Folded Reload
	movq	-112(%rsp), %rsi                ## 8-byte Reload
	adcq	-80(%rsp), %rsi                 ## 8-byte Folded Reload
	setb	%al
	addq	%rdx, %rbx
	adcq	%r14, %rcx
	adcq	%r9, %rbp
	adcq	-96(%rsp), %r12                 ## 8-byte Folded Reload
	adcq	-88(%rsp), %rsi                 ## 8-byte Folded Reload
	movq	%rsi, -112(%rsp)                ## 8-byte Spill
	movzbl	%al, %r9d
	adcq	-104(%rsp), %r9                 ## 8-byte Folded Reload
	movq	-64(%rsp), %rax                 ## 8-byte Reload
	movq	16(%rax), %rsi
	movq	%rsi, %rax
	mulq	-72(%rsp)                       ## 8-byte Folded Reload
	movq	%rdx, -104(%rsp)                ## 8-byte Spill
	movq	%rax, -88(%rsp)                 ## 8-byte Spill
	movq	%rsi, %rax
	mulq	%r10
	movq	%rdx, %r10
	movq	%rax, -96(%rsp)                 ## 8-byte Spill
	movq	%rsi, %rax
	mulq	%r8
	movq	%rdx, %r8
	movq	%rax, -80(%rsp)                 ## 8-byte Spill
	movq	%rsi, %rax
	mulq	-32(%rsp)                       ## 8-byte Folded Reload
	movq	%rdx, %r11
	movq	%rax, %r13
	movq	%rsi, %rax
	mulq	-40(%rsp)                       ## 8-byte Folded Reload
	movq	%rdx, %r15
	movq	%rax, %r14
	movq	%rsi, %rax
	mulq	-56(%rsp)                       ## 8-byte Folded Reload
	movq	%rdx, %rdi
	addq	%r14, %rdi
	adcq	%r13, %r15
	adcq	-80(%rsp), %r11                 ## 8-byte Folded Reload
	adcq	-96(%rsp), %r8                  ## 8-byte Folded Reload
	adcq	-88(%rsp), %r10                 ## 8-byte Folded Reload
	movq	-104(%rsp), %rsi                ## 8-byte Reload
	adcq	$0, %rsi
	addq	%rbx, %rax
	movq	-48(%rsp), %rdx                 ## 8-byte Reload
	movq	%rax, 16(%rdx)
	adcq	%rcx, %rdi
	adcq	%rbp, %r15
	adcq	%r12, %r11
	adcq	-112(%rsp), %r8                 ## 8-byte Folded Reload
	adcq	%r9, %r10
	adcq	$0, %rsi
	movq	%rsi, -104(%rsp)                ## 8-byte Spill
	movq	-64(%rsp), %rax                 ## 8-byte Reload
	movq	24(%rax), %rsi
	movq	%rsi, %rax
	mulq	-72(%rsp)                       ## 8-byte Folded Reload
	movq	%rdx, -88(%rsp)                 ## 8-byte Spill
	movq	%rax, -112(%rsp)                ## 8-byte Spill
	movq	%rsi, %rax
	mulq	-16(%rsp)                       ## 8-byte Folded Reload
	movq	%rdx, %rcx
	movq	%rax, -96(%rsp)                 ## 8-byte Spill
	movq	%rsi, %rax
	mulq	-8(%rsp)                        ## 8-byte Folded Reload
	movq	%rdx, %r9
	movq	%rax, -80(%rsp)                 ## 8-byte Spill
	movq	%rsi, %rax
	mulq	-32(%rsp)                       ## 8-byte Folded Reload
	movq	%rdx, %r14
	movq	%rax, %rbp
	movq	%rsi, %rax
	mulq	-40(%rsp)                       ## 8-byte Folded Reload
	movq	%rdx, %r12
	movq	%rax, %rbx
	movq	%rsi, %rax
	mulq	-56(%rsp)                       ## 8-byte Folded Reload
	movq	%rdx, %r13
	addq	%rbx, %r13
	adcq	%rbp, %r12
	adcq	-80(%rsp), %r14                 ## 8-byte Folded Reload
	adcq	-96(%rsp), %r9                  ## 8-byte Folded Reload
	adcq	-112(%rsp), %rcx                ## 8-byte Folded Reload
	movq	-88(%rsp), %rdx                 ## 8-byte Reload
	adcq	$0, %rdx
	addq	%rdi, %rax
	movq	-48(%rsp), %rdi                 ## 8-byte Reload
	movq	%rax, 24(%rdi)
	adcq	%r15, %r13
	adcq	%r11, %r12
	adcq	%r8, %r14
	adcq	%r10, %r9
	adcq	-104(%rsp), %rcx                ## 8-byte Folded Reload
	adcq	$0, %rdx
	movq	%rdx, -88(%rsp)                 ## 8-byte Spill
	movq	-64(%rsp), %rax                 ## 8-byte Reload
	movq	32(%rax), %rsi
	movq	%rsi, %rax
	mulq	-72(%rsp)                       ## 8-byte Folded Reload
	movq	%rdx, -104(%rsp)                ## 8-byte Spill
	movq	%rax, -96(%rsp)                 ## 8-byte Spill
	movq	%rsi, %rax
	mulq	-16(%rsp)                       ## 8-byte Folded Reload
	movq	%rdx, -112(%rsp)                ## 8-byte Spill
	movq	%rax, -80(%rsp)                 ## 8-byte Spill
	movq	%rsi, %rax
	mulq	-8(%rsp)                        ## 8-byte Folded Reload
	movq	%rdx, %rbx
	movq	%rax, -24(%rsp)                 ## 8-byte Spill
	movq	%rsi, %rax
	mulq	-32(%rsp)                       ## 8-byte Folded Reload
	movq	%rdx, %r15
	movq	%rax, %rbp
	movq	%rsi, %rax
	mulq	-40(%rsp)                       ## 8-byte Folded Reload
	movq	%rdx, %r10
	movq	%rax, %r8
	movq	%rsi, %rax
	mulq	-56(%rsp)                       ## 8-byte Folded Reload
	movq	%rdx, %r11
	addq	%r8, %r11
	adcq	%rbp, %r10
	adcq	-24(%rsp), %r15                 ## 8-byte Folded Reload
	adcq	-80(%rsp), %rbx                 ## 8-byte Folded Reload
	movq	-112(%rsp), %rsi                ## 8-byte Reload
	adcq	-96(%rsp), %rsi                 ## 8-byte Folded Reload
	movq	-104(%rsp), %rdx                ## 8-byte Reload
	adcq	$0, %rdx
	addq	%r13, %rax
	movq	%rax, 32(%rdi)
	adcq	%r12, %r11
	adcq	%r14, %r10
	adcq	%r9, %r15
	adcq	%rcx, %rbx
	movq	%rbx, -96(%rsp)                 ## 8-byte Spill
	adcq	-88(%rsp), %rsi                 ## 8-byte Folded Reload
	movq	%rsi, -112(%rsp)                ## 8-byte Spill
	adcq	$0, %rdx
	movq	%rdx, -104(%rsp)                ## 8-byte Spill
	movq	-64(%rsp), %rax                 ## 8-byte Reload
	movq	40(%rax), %rbx
	movq	%rbx, %rax
	mulq	-72(%rsp)                       ## 8-byte Folded Reload
	movq	%rdx, %rcx
	movq	%rax, -72(%rsp)                 ## 8-byte Spill
	movq	%rbx, %rax
	mulq	-16(%rsp)                       ## 8-byte Folded Reload
	movq	%rdx, %rbp
	movq	%rax, %r9
	movq	%rbx, %rax
	mulq	-8(%rsp)                        ## 8-byte Folded Reload
	movq	%rdx, %rsi
	movq	%rax, %r14
	movq	%rbx, %rax
	mulq	-56(%rsp)                       ## 8-byte Folded Reload
	movq	%rdx, %r12
	movq	%rax, %rdi
	movq	%rbx, %rax
	mulq	-40(%rsp)                       ## 8-byte Folded Reload
	movq	%rdx, %r13
	movq	%rax, %r8
	movq	%rbx, %rax
	mulq	-32(%rsp)                       ## 8-byte Folded Reload
	addq	%r12, %r8
	adcq	%r13, %rax
	adcq	%r14, %rdx
	adcq	%r9, %rsi
	adcq	-72(%rsp), %rbp                 ## 8-byte Folded Reload
	adcq	$0, %rcx
	addq	%r11, %rdi
	movq	-48(%rsp), %rbx                 ## 8-byte Reload
	movq	%rdi, 40(%rbx)
	adcq	%r10, %r8
	movq	%r8, 48(%rbx)
	adcq	%r15, %rax
	movq	%rax, 56(%rbx)
	adcq	-96(%rsp), %rdx                 ## 8-byte Folded Reload
	movq	%rdx, 64(%rbx)
	adcq	-112(%rsp), %rsi                ## 8-byte Folded Reload
	movq	%rsi, 72(%rbx)
	adcq	-104(%rsp), %rbp                ## 8-byte Folded Reload
	movq	%rbp, 80(%rbx)
	adcq	$0, %rcx
	movq	%rcx, 88(%rbx)
	popq	%rbx
	popq	%r12
	popq	%r13
	popq	%r14
	popq	%r15
	popq	%rbp
	retq
                                        ## -- End function
	.globl	_mcl_fpDbl_sqrPre6L             ## -- Begin function mcl_fpDbl_sqrPre6L
	.p2align	4, 0x90
_mcl_fpDbl_sqrPre6L:                    ## @mcl_fpDbl_sqrPre6L
## %bb.0:
	pushq	%rbp
	pushq	%r15
	pushq	%r14
	pushq	%r13
	pushq	%r12
	pushq	%rbx
	subq	$168, %rsp
	movq	%rdi, -128(%rsp)                ## 8-byte Spill
	movq	40(%rsi), %r9
	movq	(%rsi), %r10
	movq	8(%rsi), %rcx
	movq	%r9, %rax
	mulq	%r10
	movq	%rax, 24(%rsp)                  ## 8-byte Spill
	movq	%rdx, 16(%rsp)                  ## 8-byte Spill
	movq	32(%rsi), %r8
	movq	%r8, %rax
	mulq	%r10
	movq	%rax, 8(%rsp)                   ## 8-byte Spill
	movq	%rdx, (%rsp)                    ## 8-byte Spill
	movq	24(%rsi), %r11
	movq	%r11, %rax
	mulq	%r10
	movq	%rax, -16(%rsp)                 ## 8-byte Spill
	movq	%rdx, -80(%rsp)                 ## 8-byte Spill
	movq	16(%rsi), %r14
	movq	%r14, %rax
	mulq	%r10
	movq	%rdx, 144(%rsp)                 ## 8-byte Spill
	movq	%rax, -72(%rsp)                 ## 8-byte Spill
	movq	%r9, %rax
	mulq	%rcx
	movq	%rdx, -8(%rsp)                  ## 8-byte Spill
	movq	%rax, -24(%rsp)                 ## 8-byte Spill
	movq	%r8, %rax
	mulq	%rcx
	movq	%rdx, -32(%rsp)                 ## 8-byte Spill
	movq	%rax, -88(%rsp)                 ## 8-byte Spill
	movq	%r11, %rax
	mulq	%rcx
	movq	%rdx, -96(%rsp)                 ## 8-byte Spill
	movq	%rax, -104(%rsp)                ## 8-byte Spill
	movq	%r14, %rax
	mulq	%rcx
	movq	%rdx, %rsi
	movq	%rdx, 40(%rsp)                  ## 8-byte Spill
	movq	%rax, %r13
	movq	%rcx, %rax
	mulq	%rcx
	movq	%rdx, 112(%rsp)                 ## 8-byte Spill
	movq	%rax, %rbp
	movq	%rcx, %rax
	mulq	%r10
	movq	%rdx, %rbx
	movq	%rax, %r15
	movq	%r10, %rax
	mulq	%r10
	movq	%rdx, %rcx
	movq	%rax, (%rdi)
	movq	%r9, %rax
	mulq	%r8
	movq	%rdx, 136(%rsp)                 ## 8-byte Spill
	movq	%rax, 128(%rsp)                 ## 8-byte Spill
	movq	%r9, %rax
	mulq	%r11
	movq	%rdx, -40(%rsp)                 ## 8-byte Spill
	movq	%rax, -48(%rsp)                 ## 8-byte Spill
	movq	%r9, %rax
	mulq	%r14
	movq	%rdx, -56(%rsp)                 ## 8-byte Spill
	movq	%rax, -64(%rsp)                 ## 8-byte Spill
	movq	%r9, %rax
	mulq	%r9
	movq	%rdx, 160(%rsp)                 ## 8-byte Spill
	movq	%rax, 152(%rsp)                 ## 8-byte Spill
	movq	%r8, %rax
	mulq	%r11
	movq	%rdx, 96(%rsp)                  ## 8-byte Spill
	movq	%rax, 88(%rsp)                  ## 8-byte Spill
	movq	%r8, %rax
	mulq	%r14
	movq	%rdx, -112(%rsp)                ## 8-byte Spill
	movq	%rax, -120(%rsp)                ## 8-byte Spill
	movq	%r8, %rax
	mulq	%r8
	movq	%rdx, 120(%rsp)                 ## 8-byte Spill
	movq	%rax, 104(%rsp)                 ## 8-byte Spill
	movq	%r11, %rax
	mulq	%r14
	movq	%rdx, 64(%rsp)                  ## 8-byte Spill
	movq	%rax, 56(%rsp)                  ## 8-byte Spill
	movq	%r11, %rax
	mulq	%r11
	movq	%rdx, 80(%rsp)                  ## 8-byte Spill
	movq	%rax, 72(%rsp)                  ## 8-byte Spill
	movq	%r14, %rax
	mulq	%r14
	movq	%rax, %r12
	movq	%rdx, 48(%rsp)                  ## 8-byte Spill
	addq	%rbx, %rbp
	movq	%rbp, 32(%rsp)                  ## 8-byte Spill
	movq	112(%rsp), %r11                 ## 8-byte Reload
	adcq	%r13, %r11
	movq	%rsi, %r10
	adcq	-104(%rsp), %r10                ## 8-byte Folded Reload
	movq	-96(%rsp), %r14                 ## 8-byte Reload
	adcq	-88(%rsp), %r14                 ## 8-byte Folded Reload
	movq	-32(%rsp), %r9                  ## 8-byte Reload
	adcq	-24(%rsp), %r9                  ## 8-byte Folded Reload
	movq	-8(%rsp), %r8                   ## 8-byte Reload
	adcq	$0, %r8
	movq	%r15, %rdi
	addq	%r15, %rcx
	adcq	-72(%rsp), %rbx                 ## 8-byte Folded Reload
	movq	144(%rsp), %r15                 ## 8-byte Reload
	movq	%r15, %rbp
	adcq	-16(%rsp), %rbp                 ## 8-byte Folded Reload
	movq	-80(%rsp), %rax                 ## 8-byte Reload
	adcq	8(%rsp), %rax                   ## 8-byte Folded Reload
	movq	(%rsp), %rdx                    ## 8-byte Reload
	adcq	24(%rsp), %rdx                  ## 8-byte Folded Reload
	movq	16(%rsp), %rsi                  ## 8-byte Reload
	adcq	$0, %rsi
	addq	%rdi, %rcx
	adcq	32(%rsp), %rbx                  ## 8-byte Folded Reload
	movq	-128(%rsp), %rdi                ## 8-byte Reload
	movq	%rcx, 8(%rdi)
	adcq	%r11, %rbp
	adcq	%r10, %rax
	adcq	%r14, %rdx
	adcq	%r9, %rsi
	adcq	$0, %r8
	movq	%r15, %r9
	addq	%r13, %r9
	adcq	40(%rsp), %r12                  ## 8-byte Folded Reload
	movq	48(%rsp), %rcx                  ## 8-byte Reload
	movq	56(%rsp), %rdi                  ## 8-byte Reload
	adcq	%rdi, %rcx
	movq	64(%rsp), %r15                  ## 8-byte Reload
	movq	%r15, %r10
	adcq	-120(%rsp), %r10                ## 8-byte Folded Reload
	movq	-112(%rsp), %r11                ## 8-byte Reload
	adcq	-64(%rsp), %r11                 ## 8-byte Folded Reload
	movq	-56(%rsp), %r13                 ## 8-byte Reload
	adcq	$0, %r13
	addq	-72(%rsp), %rbx                 ## 8-byte Folded Reload
	adcq	%rbp, %r9
	movq	-128(%rsp), %rbp                ## 8-byte Reload
	movq	%rbx, 16(%rbp)
	adcq	%rax, %r12
	adcq	%rdx, %rcx
	movq	%rcx, %rbx
	adcq	%rsi, %r10
	adcq	%r8, %r11
	adcq	$0, %r13
	movq	-80(%rsp), %rsi                 ## 8-byte Reload
	addq	-104(%rsp), %rsi                ## 8-byte Folded Reload
	movq	-96(%rsp), %rax                 ## 8-byte Reload
	adcq	%rdi, %rax
	movq	72(%rsp), %rdi                  ## 8-byte Reload
	adcq	%r15, %rdi
	movq	80(%rsp), %rdx                  ## 8-byte Reload
	movq	88(%rsp), %r15                  ## 8-byte Reload
	adcq	%r15, %rdx
	movq	96(%rsp), %r8                   ## 8-byte Reload
	movq	%r8, %r14
	adcq	-48(%rsp), %r14                 ## 8-byte Folded Reload
	movq	-40(%rsp), %rcx                 ## 8-byte Reload
	adcq	$0, %rcx
	addq	-16(%rsp), %r9                  ## 8-byte Folded Reload
	adcq	%r12, %rsi
	movq	%r9, 24(%rbp)
	adcq	%rbx, %rax
	adcq	%r10, %rdi
	movq	%rdi, %r9
	adcq	%r11, %rdx
	movq	%rdx, %r12
	adcq	%r13, %r14
	adcq	$0, %rcx
	movq	(%rsp), %rdi                    ## 8-byte Reload
	addq	-88(%rsp), %rdi                 ## 8-byte Folded Reload
	movq	-32(%rsp), %rdx                 ## 8-byte Reload
	adcq	-120(%rsp), %rdx                ## 8-byte Folded Reload
	movq	-112(%rsp), %rbx                ## 8-byte Reload
	adcq	%r15, %rbx
	movq	104(%rsp), %r13                 ## 8-byte Reload
	adcq	%r8, %r13
	movq	120(%rsp), %rbp                 ## 8-byte Reload
	movq	128(%rsp), %r11                 ## 8-byte Reload
	adcq	%r11, %rbp
	movq	136(%rsp), %r15                 ## 8-byte Reload
	movq	%r15, %r10
	adcq	$0, %r10
	addq	8(%rsp), %rsi                   ## 8-byte Folded Reload
	adcq	%rax, %rdi
	movq	-128(%rsp), %r8                 ## 8-byte Reload
	movq	%rsi, 32(%r8)
	adcq	%r9, %rdx
	movq	%rdx, %r9
	adcq	%r12, %rbx
	movq	%rbx, %r12
	adcq	%r14, %r13
	adcq	%rcx, %rbp
	movq	%rbp, %r14
	adcq	$0, %r10
	movq	16(%rsp), %rsi                  ## 8-byte Reload
	addq	-24(%rsp), %rsi                 ## 8-byte Folded Reload
	movq	-8(%rsp), %rdx                  ## 8-byte Reload
	adcq	-64(%rsp), %rdx                 ## 8-byte Folded Reload
	movq	-56(%rsp), %rbp                 ## 8-byte Reload
	adcq	-48(%rsp), %rbp                 ## 8-byte Folded Reload
	movq	-40(%rsp), %rbx                 ## 8-byte Reload
	adcq	%r11, %rbx
	movq	152(%rsp), %r11                 ## 8-byte Reload
	adcq	%r15, %r11
	movq	160(%rsp), %rax                 ## 8-byte Reload
	adcq	$0, %rax
	addq	24(%rsp), %rdi                  ## 8-byte Folded Reload
	movq	%rdi, 40(%r8)
	adcq	%r9, %rsi
	movq	%rsi, 48(%r8)
	adcq	%r12, %rdx
	movq	%rdx, 56(%r8)
	movq	%rbp, %rdx
	adcq	%r13, %rdx
	movq	%rdx, 64(%r8)
	movq	%rbx, %rdx
	adcq	%r14, %rdx
	movq	%rdx, 72(%r8)
	movq	%r11, %rdx
	adcq	%r10, %rdx
	movq	%rdx, 80(%r8)
	adcq	$0, %rax
	movq	%rax, 88(%r8)
	addq	$168, %rsp
	popq	%rbx
	popq	%r12
	popq	%r13
	popq	%r14
	popq	%r15
	popq	%rbp
	retq
                                        ## -- End function
	.globl	_mcl_fp_mont6L                  ## -- Begin function mcl_fp_mont6L
	.p2align	4, 0x90
_mcl_fp_mont6L:                         ## @mcl_fp_mont6L
## %bb.0:
	pushq	%rbp
	pushq	%r15
	pushq	%r14
	pushq	%r13
	pushq	%r12
	pushq	%rbx
	subq	$48, %rsp
	movq	%rdx, -48(%rsp)                 ## 8-byte Spill
	movq	%rdi, 40(%rsp)                  ## 8-byte Spill
	movq	40(%rsi), %rax
	movq	%rax, -56(%rsp)                 ## 8-byte Spill
	movq	(%rdx), %rbp
	mulq	%rbp
	movq	%rax, %r8
	movq	%rdx, %r10
	movq	32(%rsi), %rax
	movq	%rax, -64(%rsp)                 ## 8-byte Spill
	mulq	%rbp
	movq	%rax, %r11
	movq	%rdx, %r13
	movq	24(%rsi), %rax
	movq	%rax, -72(%rsp)                 ## 8-byte Spill
	mulq	%rbp
	movq	%rax, %r15
	movq	%rdx, %rdi
	movq	16(%rsi), %rax
	movq	%rax, -40(%rsp)                 ## 8-byte Spill
	mulq	%rbp
	movq	%rax, %r9
	movq	%rdx, %r14
	movq	(%rsi), %rbx
	movq	%rbx, 24(%rsp)                  ## 8-byte Spill
	movq	8(%rsi), %rax
	movq	%rax, 16(%rsp)                  ## 8-byte Spill
	mulq	%rbp
	movq	%rdx, %r12
	movq	%rax, %rsi
	movq	%rbx, %rax
	mulq	%rbp
	movq	%rax, -96(%rsp)                 ## 8-byte Spill
	movq	%rdx, %rbp
	addq	%rsi, %rbp
	adcq	%r9, %r12
	adcq	%r15, %r14
	adcq	%r11, %rdi
	movq	%rdi, -88(%rsp)                 ## 8-byte Spill
	adcq	%r8, %r13
	movq	%r13, -128(%rsp)                ## 8-byte Spill
	adcq	$0, %r10
	movq	%r10, -112(%rsp)                ## 8-byte Spill
	movq	-8(%rcx), %r8
	movq	%r8, -32(%rsp)                  ## 8-byte Spill
	imulq	%rax, %r8
	movq	40(%rcx), %rdx
	movq	%rdx, 8(%rsp)                   ## 8-byte Spill
	movq	%r8, %rax
	mulq	%rdx
	movq	%rax, -104(%rsp)                ## 8-byte Spill
	movq	%rdx, -120(%rsp)                ## 8-byte Spill
	movq	32(%rcx), %rdx
	movq	%rdx, (%rsp)                    ## 8-byte Spill
	movq	%r8, %rax
	mulq	%rdx
	movq	%rax, -80(%rsp)                 ## 8-byte Spill
	movq	%rdx, %r11
	movq	24(%rcx), %rdx
	movq	%rdx, -8(%rsp)                  ## 8-byte Spill
	movq	%r8, %rax
	mulq	%rdx
	movq	%rax, %r13
	movq	%rdx, %r15
	movq	16(%rcx), %rdx
	movq	%rdx, -16(%rsp)                 ## 8-byte Spill
	movq	%r8, %rax
	mulq	%rdx
	movq	%rax, %r9
	movq	%rdx, %rsi
	movq	(%rcx), %rbx
	movq	%rbx, -24(%rsp)                 ## 8-byte Spill
	movq	8(%rcx), %rcx
	movq	%rcx, 32(%rsp)                  ## 8-byte Spill
	movq	%r8, %rax
	mulq	%rcx
	movq	%rdx, %rdi
	movq	%rax, %r10
	movq	%r8, %rax
	mulq	%rbx
	movq	%rdx, %rcx
	addq	%r10, %rcx
	adcq	%r9, %rdi
	adcq	%r13, %rsi
	adcq	-80(%rsp), %r15                 ## 8-byte Folded Reload
	adcq	-104(%rsp), %r11                ## 8-byte Folded Reload
	movq	-120(%rsp), %rdx                ## 8-byte Reload
	adcq	$0, %rdx
	addq	-96(%rsp), %rax                 ## 8-byte Folded Reload
	adcq	%rbp, %rcx
	adcq	%r12, %rdi
	adcq	%r14, %rsi
	adcq	-88(%rsp), %r15                 ## 8-byte Folded Reload
	adcq	-128(%rsp), %r11                ## 8-byte Folded Reload
	adcq	-112(%rsp), %rdx                ## 8-byte Folded Reload
	movq	%rdx, -120(%rsp)                ## 8-byte Spill
	setb	-128(%rsp)                      ## 1-byte Folded Spill
	movq	-48(%rsp), %rax                 ## 8-byte Reload
	movq	8(%rax), %rbx
	movq	%rbx, %rax
	mulq	-56(%rsp)                       ## 8-byte Folded Reload
	movq	%rdx, -112(%rsp)                ## 8-byte Spill
	movq	%rax, -88(%rsp)                 ## 8-byte Spill
	movq	%rbx, %rax
	mulq	-64(%rsp)                       ## 8-byte Folded Reload
	movq	%rdx, %r10
	movq	%rax, -96(%rsp)                 ## 8-byte Spill
	movq	%rbx, %rax
	mulq	-72(%rsp)                       ## 8-byte Folded Reload
	movq	%rdx, %r14
	movq	%rax, -104(%rsp)                ## 8-byte Spill
	movq	%rbx, %rax
	mulq	-40(%rsp)                       ## 8-byte Folded Reload
	movq	%rdx, %r8
	movq	%rax, %r13
	movq	%rbx, %rax
	mulq	16(%rsp)                        ## 8-byte Folded Reload
	movq	%rdx, %rbp
	movq	%rax, %r12
	movq	%rbx, %rax
	mulq	24(%rsp)                        ## 8-byte Folded Reload
	movq	%rax, %r9
	movq	%rdx, %rbx
	addq	%r12, %rbx
	adcq	%r13, %rbp
	adcq	-104(%rsp), %r8                 ## 8-byte Folded Reload
	adcq	-96(%rsp), %r14                 ## 8-byte Folded Reload
	adcq	-88(%rsp), %r10                 ## 8-byte Folded Reload
	movq	-112(%rsp), %rdx                ## 8-byte Reload
	adcq	$0, %rdx
	addq	%rcx, %r9
	adcq	%rdi, %rbx
	adcq	%rsi, %rbp
	adcq	%r15, %r8
	adcq	%r11, %r14
	adcq	-120(%rsp), %r10                ## 8-byte Folded Reload
	movzbl	-128(%rsp), %eax                ## 1-byte Folded Reload
	adcq	%rax, %rdx
	movq	%rdx, -112(%rsp)                ## 8-byte Spill
	setb	-120(%rsp)                      ## 1-byte Folded Spill
	movq	-32(%rsp), %rsi                 ## 8-byte Reload
	imulq	%r9, %rsi
	movq	%rsi, %rax
	mulq	8(%rsp)                         ## 8-byte Folded Reload
	movq	%rdx, -128(%rsp)                ## 8-byte Spill
	movq	%rax, -96(%rsp)                 ## 8-byte Spill
	movq	%rsi, %rax
	mulq	(%rsp)                          ## 8-byte Folded Reload
	movq	%rdx, -88(%rsp)                 ## 8-byte Spill
	movq	%rax, -104(%rsp)                ## 8-byte Spill
	movq	%rsi, %rax
	mulq	-8(%rsp)                        ## 8-byte Folded Reload
	movq	%rdx, %r13
	movq	%rax, -80(%rsp)                 ## 8-byte Spill
	movq	%rsi, %rax
	mulq	-16(%rsp)                       ## 8-byte Folded Reload
	movq	%rdx, %rcx
	movq	%rax, %rdi
	movq	%rsi, %rax
	mulq	32(%rsp)                        ## 8-byte Folded Reload
	movq	%rdx, %r12
	movq	%rax, %r15
	movq	%rsi, %rax
	mulq	-24(%rsp)                       ## 8-byte Folded Reload
	movq	%rdx, %r11
	addq	%r15, %r11
	adcq	%rdi, %r12
	adcq	-80(%rsp), %rcx                 ## 8-byte Folded Reload
	adcq	-104(%rsp), %r13                ## 8-byte Folded Reload
	movq	-88(%rsp), %rsi                 ## 8-byte Reload
	adcq	-96(%rsp), %rsi                 ## 8-byte Folded Reload
	movq	-128(%rsp), %rdx                ## 8-byte Reload
	adcq	$0, %rdx
	addq	%r9, %rax
	adcq	%rbx, %r11
	adcq	%rbp, %r12
	adcq	%r8, %rcx
	adcq	%r14, %r13
	adcq	%r10, %rsi
	movq	%rsi, -88(%rsp)                 ## 8-byte Spill
	adcq	-112(%rsp), %rdx                ## 8-byte Folded Reload
	movq	%rdx, -128(%rsp)                ## 8-byte Spill
	movzbl	-120(%rsp), %ebx                ## 1-byte Folded Reload
	adcq	$0, %rbx
	movq	-48(%rsp), %rax                 ## 8-byte Reload
	movq	16(%rax), %rsi
	movq	%rsi, %rax
	mulq	-56(%rsp)                       ## 8-byte Folded Reload
	movq	%rdx, -112(%rsp)                ## 8-byte Spill
	movq	%rax, -96(%rsp)                 ## 8-byte Spill
	movq	%rsi, %rax
	mulq	-64(%rsp)                       ## 8-byte Folded Reload
	movq	%rdx, -120(%rsp)                ## 8-byte Spill
	movq	%rax, -104(%rsp)                ## 8-byte Spill
	movq	%rsi, %rax
	mulq	-72(%rsp)                       ## 8-byte Folded Reload
	movq	%rdx, %rdi
	movq	%rax, %r9
	movq	%rsi, %rax
	mulq	-40(%rsp)                       ## 8-byte Folded Reload
	movq	%rdx, %r10
	movq	%rax, %r14
	movq	%rsi, %rax
	mulq	16(%rsp)                        ## 8-byte Folded Reload
	movq	%rdx, %r15
	movq	%rax, %rbp
	movq	%rsi, %rax
	mulq	24(%rsp)                        ## 8-byte Folded Reload
	movq	%rax, %rsi
	movq	%rdx, %r8
	addq	%rbp, %r8
	adcq	%r14, %r15
	adcq	%r9, %r10
	adcq	-104(%rsp), %rdi                ## 8-byte Folded Reload
	movq	-120(%rsp), %rdx                ## 8-byte Reload
	adcq	-96(%rsp), %rdx                 ## 8-byte Folded Reload
	movq	-112(%rsp), %rax                ## 8-byte Reload
	adcq	$0, %rax
	addq	%r11, %rsi
	adcq	%r12, %r8
	adcq	%rcx, %r15
	adcq	%r13, %r10
	adcq	-88(%rsp), %rdi                 ## 8-byte Folded Reload
	adcq	-128(%rsp), %rdx                ## 8-byte Folded Reload
	movq	%rdx, -120(%rsp)                ## 8-byte Spill
	adcq	%rbx, %rax
	movq	%rax, -112(%rsp)                ## 8-byte Spill
	setb	-88(%rsp)                       ## 1-byte Folded Spill
	movq	-32(%rsp), %rcx                 ## 8-byte Reload
	imulq	%rsi, %rcx
	movq	%rcx, %rax
	mulq	8(%rsp)                         ## 8-byte Folded Reload
	movq	%rdx, -128(%rsp)                ## 8-byte Spill
	movq	%rax, -96(%rsp)                 ## 8-byte Spill
	movq	%rcx, %rax
	mulq	(%rsp)                          ## 8-byte Folded Reload
	movq	%rdx, %r12
	movq	%rax, -104(%rsp)                ## 8-byte Spill
	movq	%rcx, %rax
	mulq	-8(%rsp)                        ## 8-byte Folded Reload
	movq	%rdx, %rbx
	movq	%rax, -80(%rsp)                 ## 8-byte Spill
	movq	%rcx, %rax
	mulq	-16(%rsp)                       ## 8-byte Folded Reload
	movq	%rdx, %r14
	movq	%rax, %r13
	movq	%rcx, %rax
	mulq	32(%rsp)                        ## 8-byte Folded Reload
	movq	%rdx, %rbp
	movq	%rax, %r9
	movq	%rcx, %rax
	mulq	-24(%rsp)                       ## 8-byte Folded Reload
	movq	%rdx, %r11
	addq	%r9, %r11
	adcq	%r13, %rbp
	adcq	-80(%rsp), %r14                 ## 8-byte Folded Reload
	adcq	-104(%rsp), %rbx                ## 8-byte Folded Reload
	adcq	-96(%rsp), %r12                 ## 8-byte Folded Reload
	movq	-128(%rsp), %rcx                ## 8-byte Reload
	adcq	$0, %rcx
	addq	%rsi, %rax
	adcq	%r8, %r11
	adcq	%r15, %rbp
	adcq	%r10, %r14
	adcq	%rdi, %rbx
	adcq	-120(%rsp), %r12                ## 8-byte Folded Reload
	adcq	-112(%rsp), %rcx                ## 8-byte Folded Reload
	movq	%rcx, -128(%rsp)                ## 8-byte Spill
	movzbl	-88(%rsp), %esi                 ## 1-byte Folded Reload
	adcq	$0, %rsi
	movq	-48(%rsp), %rax                 ## 8-byte Reload
	movq	24(%rax), %rdi
	movq	%rdi, %rax
	mulq	-56(%rsp)                       ## 8-byte Folded Reload
	movq	%rdx, -112(%rsp)                ## 8-byte Spill
	movq	%rax, -88(%rsp)                 ## 8-byte Spill
	movq	%rdi, %rax
	mulq	-64(%rsp)                       ## 8-byte Folded Reload
	movq	%rdx, -120(%rsp)                ## 8-byte Spill
	movq	%rax, -96(%rsp)                 ## 8-byte Spill
	movq	%rdi, %rax
	mulq	-72(%rsp)                       ## 8-byte Folded Reload
	movq	%rdx, %r13
	movq	%rax, -104(%rsp)                ## 8-byte Spill
	movq	%rdi, %rax
	mulq	-40(%rsp)                       ## 8-byte Folded Reload
	movq	%rdx, %r15
	movq	%rax, %r10
	movq	%rdi, %rax
	mulq	16(%rsp)                        ## 8-byte Folded Reload
	movq	%rdx, %rcx
	movq	%rax, %r8
	movq	%rdi, %rax
	mulq	24(%rsp)                        ## 8-byte Folded Reload
	movq	%rax, %r9
	movq	%rdx, %rdi
	addq	%r8, %rdi
	adcq	%r10, %rcx
	adcq	-104(%rsp), %r15                ## 8-byte Folded Reload
	adcq	-96(%rsp), %r13                 ## 8-byte Folded Reload
	movq	-120(%rsp), %rdx                ## 8-byte Reload
	adcq	-88(%rsp), %rdx                 ## 8-byte Folded Reload
	movq	-112(%rsp), %rax                ## 8-byte Reload
	adcq	$0, %rax
	addq	%r11, %r9
	adcq	%rbp, %rdi
	adcq	%r14, %rcx
	adcq	%rbx, %r15
	adcq	%r12, %r13
	adcq	-128(%rsp), %rdx                ## 8-byte Folded Reload
	movq	%rdx, -120(%rsp)                ## 8-byte Spill
	adcq	%rsi, %rax
	movq	%rax, -112(%rsp)                ## 8-byte Spill
	setb	-88(%rsp)                       ## 1-byte Folded Spill
	movq	-32(%rsp), %rsi                 ## 8-byte Reload
	imulq	%r9, %rsi
	movq	%rsi, %rax
	mulq	8(%rsp)                         ## 8-byte Folded Reload
	movq	%rdx, -128(%rsp)                ## 8-byte Spill
	movq	%rax, -96(%rsp)                 ## 8-byte Spill
	movq	%rsi, %rax
	mulq	(%rsp)                          ## 8-byte Folded Reload
	movq	%rdx, %r8
	movq	%rax, -104(%rsp)                ## 8-byte Spill
	movq	%rsi, %rax
	mulq	-8(%rsp)                        ## 8-byte Folded Reload
	movq	%rdx, %r12
	movq	%rax, -80(%rsp)                 ## 8-byte Spill
	movq	%rsi, %rax
	mulq	-16(%rsp)                       ## 8-byte Folded Reload
	movq	%rdx, %rbp
	movq	%rax, %r14
	movq	%rsi, %rax
	mulq	32(%rsp)                        ## 8-byte Folded Reload
	movq	%rdx, %rbx
	movq	%rax, %r11
	movq	%rsi, %rax
	mulq	-24(%rsp)                       ## 8-byte Folded Reload
	movq	%rdx, %r10
	addq	%r11, %r10
	adcq	%r14, %rbx
	adcq	-80(%rsp), %rbp                 ## 8-byte Folded Reload
	adcq	-104(%rsp), %r12                ## 8-byte Folded Reload
	adcq	-96(%rsp), %r8                  ## 8-byte Folded Reload
	movq	-128(%rsp), %rdx                ## 8-byte Reload
	adcq	$0, %rdx
	addq	%r9, %rax
	adcq	%rdi, %r10
	adcq	%rcx, %rbx
	adcq	%r15, %rbp
	adcq	%r13, %r12
	adcq	-120(%rsp), %r8                 ## 8-byte Folded Reload
	adcq	-112(%rsp), %rdx                ## 8-byte Folded Reload
	movq	%rdx, -128(%rsp)                ## 8-byte Spill
	movzbl	-88(%rsp), %r11d                ## 1-byte Folded Reload
	adcq	$0, %r11
	movq	-48(%rsp), %rax                 ## 8-byte Reload
	movq	32(%rax), %rcx
	movq	%rcx, %rax
	mulq	-56(%rsp)                       ## 8-byte Folded Reload
	movq	%rdx, -112(%rsp)                ## 8-byte Spill
	movq	%rax, -88(%rsp)                 ## 8-byte Spill
	movq	%rcx, %rax
	mulq	-64(%rsp)                       ## 8-byte Folded Reload
	movq	%rdx, -120(%rsp)                ## 8-byte Spill
	movq	%rax, -96(%rsp)                 ## 8-byte Spill
	movq	%rcx, %rax
	mulq	-72(%rsp)                       ## 8-byte Folded Reload
	movq	%rdx, %r14
	movq	%rax, -104(%rsp)                ## 8-byte Spill
	movq	%rcx, %rax
	mulq	-40(%rsp)                       ## 8-byte Folded Reload
	movq	%rdx, %r13
	movq	%rax, %rsi
	movq	%rcx, %rax
	mulq	16(%rsp)                        ## 8-byte Folded Reload
	movq	%rdx, %rdi
	movq	%rax, %r15
	movq	%rcx, %rax
	mulq	24(%rsp)                        ## 8-byte Folded Reload
	movq	%rax, %r9
	movq	%rdx, %rcx
	addq	%r15, %rcx
	adcq	%rsi, %rdi
	adcq	-104(%rsp), %r13                ## 8-byte Folded Reload
	adcq	-96(%rsp), %r14                 ## 8-byte Folded Reload
	movq	-120(%rsp), %rax                ## 8-byte Reload
	adcq	-88(%rsp), %rax                 ## 8-byte Folded Reload
	movq	-112(%rsp), %rdx                ## 8-byte Reload
	adcq	$0, %rdx
	addq	%r10, %r9
	adcq	%rbx, %rcx
	adcq	%rbp, %rdi
	adcq	%r12, %r13
	adcq	%r8, %r14
	adcq	-128(%rsp), %rax                ## 8-byte Folded Reload
	movq	%rax, -120(%rsp)                ## 8-byte Spill
	adcq	%r11, %rdx
	movq	%rdx, -112(%rsp)                ## 8-byte Spill
	setb	-88(%rsp)                       ## 1-byte Folded Spill
	movq	-32(%rsp), %rbx                 ## 8-byte Reload
	imulq	%r9, %rbx
	movq	%rbx, %rax
	mulq	8(%rsp)                         ## 8-byte Folded Reload
	movq	%rdx, -128(%rsp)                ## 8-byte Spill
	movq	%rax, -96(%rsp)                 ## 8-byte Spill
	movq	%rbx, %rax
	mulq	(%rsp)                          ## 8-byte Folded Reload
	movq	%rdx, %r15
	movq	%rax, -104(%rsp)                ## 8-byte Spill
	movq	%rbx, %rax
	mulq	-8(%rsp)                        ## 8-byte Folded Reload
	movq	%rdx, %r12
	movq	%rax, -80(%rsp)                 ## 8-byte Spill
	movq	%rbx, %rax
	mulq	-16(%rsp)                       ## 8-byte Folded Reload
	movq	%rdx, %rbp
	movq	%rax, %r10
	movq	%rbx, %rax
	mulq	32(%rsp)                        ## 8-byte Folded Reload
	movq	%rdx, %rsi
	movq	%rax, %r11
	movq	%rbx, %rax
	mulq	-24(%rsp)                       ## 8-byte Folded Reload
	movq	%rdx, %r8
	addq	%r11, %r8
	adcq	%r10, %rsi
	adcq	-80(%rsp), %rbp                 ## 8-byte Folded Reload
	adcq	-104(%rsp), %r12                ## 8-byte Folded Reload
	adcq	-96(%rsp), %r15                 ## 8-byte Folded Reload
	movq	-128(%rsp), %rbx                ## 8-byte Reload
	adcq	$0, %rbx
	addq	%r9, %rax
	adcq	%rcx, %r8
	adcq	%rdi, %rsi
	adcq	%r13, %rbp
	adcq	%r14, %r12
	adcq	-120(%rsp), %r15                ## 8-byte Folded Reload
	movq	%r15, -120(%rsp)                ## 8-byte Spill
	adcq	-112(%rsp), %rbx                ## 8-byte Folded Reload
	movq	%rbx, -128(%rsp)                ## 8-byte Spill
	movzbl	-88(%rsp), %edi                 ## 1-byte Folded Reload
	adcq	$0, %rdi
	movq	-48(%rsp), %rax                 ## 8-byte Reload
	movq	40(%rax), %rcx
	movq	%rcx, %rax
	mulq	-56(%rsp)                       ## 8-byte Folded Reload
	movq	%rdx, -48(%rsp)                 ## 8-byte Spill
	movq	%rax, -112(%rsp)                ## 8-byte Spill
	movq	%rcx, %rax
	mulq	-64(%rsp)                       ## 8-byte Folded Reload
	movq	%rdx, -56(%rsp)                 ## 8-byte Spill
	movq	%rax, -64(%rsp)                 ## 8-byte Spill
	movq	%rcx, %rax
	mulq	-72(%rsp)                       ## 8-byte Folded Reload
	movq	%rdx, %r11
	movq	%rax, -72(%rsp)                 ## 8-byte Spill
	movq	%rcx, %rax
	mulq	-40(%rsp)                       ## 8-byte Folded Reload
	movq	%rdx, %r13
	movq	%rax, %rbx
	movq	%rcx, %rax
	mulq	16(%rsp)                        ## 8-byte Folded Reload
	movq	%rdx, %r10
	movq	%rax, %r15
	movq	%rcx, %rax
	mulq	24(%rsp)                        ## 8-byte Folded Reload
	movq	%rax, %r14
	movq	%rdx, %r9
	addq	%r15, %r9
	adcq	%rbx, %r10
	adcq	-72(%rsp), %r13                 ## 8-byte Folded Reload
	adcq	-64(%rsp), %r11                 ## 8-byte Folded Reload
	movq	-56(%rsp), %rcx                 ## 8-byte Reload
	adcq	-112(%rsp), %rcx                ## 8-byte Folded Reload
	movq	-48(%rsp), %rax                 ## 8-byte Reload
	adcq	$0, %rax
	addq	%r8, %r14
	adcq	%rsi, %r9
	adcq	%rbp, %r10
	adcq	%r12, %r13
	adcq	-120(%rsp), %r11                ## 8-byte Folded Reload
	adcq	-128(%rsp), %rcx                ## 8-byte Folded Reload
	movq	%rcx, -56(%rsp)                 ## 8-byte Spill
	adcq	%rdi, %rax
	movq	%rax, -48(%rsp)                 ## 8-byte Spill
	setb	-64(%rsp)                       ## 1-byte Folded Spill
	movq	-32(%rsp), %r12                 ## 8-byte Reload
	imulq	%r14, %r12
	movq	%r12, %rax
	mulq	8(%rsp)                         ## 8-byte Folded Reload
	movq	%rdx, %rsi
	movq	%rax, -32(%rsp)                 ## 8-byte Spill
	movq	%r12, %rax
	mulq	(%rsp)                          ## 8-byte Folded Reload
	movq	%rdx, %rbp
	movq	%rax, -72(%rsp)                 ## 8-byte Spill
	movq	%r12, %rax
	mulq	-8(%rsp)                        ## 8-byte Folded Reload
	movq	%rdx, %rcx
	movq	%rax, -40(%rsp)                 ## 8-byte Spill
	movq	%r12, %rax
	mulq	-16(%rsp)                       ## 8-byte Folded Reload
	movq	%rdx, %rbx
	movq	%rax, %r15
	movq	%r12, %rax
	mulq	-24(%rsp)                       ## 8-byte Folded Reload
	movq	%rdx, %r8
	movq	%rax, %rdi
	movq	%r12, %rax
	movq	32(%rsp), %r12                  ## 8-byte Reload
	mulq	%r12
	addq	%r8, %rax
	adcq	%r15, %rdx
	adcq	-40(%rsp), %rbx                 ## 8-byte Folded Reload
	adcq	-72(%rsp), %rcx                 ## 8-byte Folded Reload
	adcq	-32(%rsp), %rbp                 ## 8-byte Folded Reload
	adcq	$0, %rsi
	addq	%r14, %rdi
	adcq	%r9, %rax
	adcq	%r10, %rdx
	adcq	%r13, %rbx
	adcq	%r11, %rcx
	adcq	-56(%rsp), %rbp                 ## 8-byte Folded Reload
	adcq	-48(%rsp), %rsi                 ## 8-byte Folded Reload
	movzbl	-64(%rsp), %r11d                ## 1-byte Folded Reload
	adcq	$0, %r11
	movq	%rax, %r8
	subq	-24(%rsp), %r8                  ## 8-byte Folded Reload
	movq	%rdx, %r9
	sbbq	%r12, %r9
	movq	%rbx, %r10
	sbbq	-16(%rsp), %r10                 ## 8-byte Folded Reload
	movq	%rcx, %r14
	sbbq	-8(%rsp), %r14                  ## 8-byte Folded Reload
	movq	%rbp, %r15
	sbbq	(%rsp), %r15                    ## 8-byte Folded Reload
	movq	%rsi, %rdi
	sbbq	8(%rsp), %rdi                   ## 8-byte Folded Reload
	sbbq	$0, %r11
	testb	$1, %r11b
	cmovneq	%rsi, %rdi
	movq	40(%rsp), %rsi                  ## 8-byte Reload
	movq	%rdi, 40(%rsi)
	cmovneq	%rbp, %r15
	movq	%r15, 32(%rsi)
	cmovneq	%rcx, %r14
	movq	%r14, 24(%rsi)
	cmovneq	%rbx, %r10
	movq	%r10, 16(%rsi)
	cmovneq	%rdx, %r9
	movq	%r9, 8(%rsi)
	cmovneq	%rax, %r8
	movq	%r8, (%rsi)
	addq	$48, %rsp
	popq	%rbx
	popq	%r12
	popq	%r13
	popq	%r14
	popq	%r15
	popq	%rbp
	retq
                                        ## -- End function
	.globl	_mcl_fp_montNF6L                ## -- Begin function mcl_fp_montNF6L
	.p2align	4, 0x90
_mcl_fp_montNF6L:                       ## @mcl_fp_montNF6L
## %bb.0:
	pushq	%rbp
	pushq	%r15
	pushq	%r14
	pushq	%r13
	pushq	%r12
	pushq	%rbx
	subq	$40, %rsp
	movq	%rdx, -72(%rsp)                 ## 8-byte Spill
	movq	%rdi, 32(%rsp)                  ## 8-byte Spill
	movq	40(%rsi), %rax
	movq	%rax, -80(%rsp)                 ## 8-byte Spill
	movq	(%rdx), %rdi
	mulq	%rdi
	movq	%rax, -64(%rsp)                 ## 8-byte Spill
	movq	%rdx, %r12
	movq	32(%rsi), %rax
	movq	%rax, -88(%rsp)                 ## 8-byte Spill
	mulq	%rdi
	movq	%rax, %r14
	movq	%rdx, %r10
	movq	24(%rsi), %rax
	movq	%rax, -56(%rsp)                 ## 8-byte Spill
	mulq	%rdi
	movq	%rax, %r15
	movq	%rdx, %r9
	movq	16(%rsi), %rax
	movq	%rax, 8(%rsp)                   ## 8-byte Spill
	mulq	%rdi
	movq	%rax, %r11
	movq	%rdx, %r8
	movq	(%rsi), %rbx
	movq	%rbx, (%rsp)                    ## 8-byte Spill
	movq	8(%rsi), %rax
	movq	%rax, -8(%rsp)                  ## 8-byte Spill
	mulq	%rdi
	movq	%rdx, %rbp
	movq	%rax, %rsi
	movq	%rbx, %rax
	mulq	%rdi
	movq	%rax, %r13
	movq	%rdx, %rdi
	addq	%rsi, %rdi
	adcq	%r11, %rbp
	adcq	%r15, %r8
	adcq	%r14, %r9
	adcq	-64(%rsp), %r10                 ## 8-byte Folded Reload
	movq	%r10, -128(%rsp)                ## 8-byte Spill
	adcq	$0, %r12
	movq	%r12, -112(%rsp)                ## 8-byte Spill
	movq	-8(%rcx), %rbx
	movq	%rbx, -48(%rsp)                 ## 8-byte Spill
	imulq	%rax, %rbx
	movq	40(%rcx), %rdx
	movq	%rdx, -64(%rsp)                 ## 8-byte Spill
	movq	%rbx, %rax
	mulq	%rdx
	movq	%rax, %r14
	movq	%rdx, -120(%rsp)                ## 8-byte Spill
	movq	32(%rcx), %rdx
	movq	%rdx, -16(%rsp)                 ## 8-byte Spill
	movq	%rbx, %rax
	mulq	%rdx
	movq	%rax, %r15
	movq	%rdx, -96(%rsp)                 ## 8-byte Spill
	movq	24(%rcx), %rdx
	movq	%rdx, -24(%rsp)                 ## 8-byte Spill
	movq	%rbx, %rax
	mulq	%rdx
	movq	%rax, %r12
	movq	%rdx, -104(%rsp)                ## 8-byte Spill
	movq	16(%rcx), %rdx
	movq	%rdx, -40(%rsp)                 ## 8-byte Spill
	movq	%rbx, %rax
	mulq	%rdx
	movq	%rax, %r10
	movq	%rdx, 24(%rsp)                  ## 8-byte Spill
	movq	(%rcx), %rsi
	movq	%rsi, -32(%rsp)                 ## 8-byte Spill
	movq	8(%rcx), %rcx
	movq	%rcx, 16(%rsp)                  ## 8-byte Spill
	movq	%rbx, %rax
	mulq	%rcx
	movq	%rdx, %r11
	movq	%rax, %rcx
	movq	%rbx, %rax
	mulq	%rsi
	addq	%r13, %rax
	adcq	%rdi, %rcx
	adcq	%rbp, %r10
	adcq	%r8, %r12
	adcq	%r9, %r15
	adcq	-128(%rsp), %r14                ## 8-byte Folded Reload
	movq	-112(%rsp), %rax                ## 8-byte Reload
	adcq	$0, %rax
	addq	%rdx, %rcx
	adcq	%r11, %r10
	adcq	24(%rsp), %r12                  ## 8-byte Folded Reload
	adcq	-104(%rsp), %r15                ## 8-byte Folded Reload
	adcq	-96(%rsp), %r14                 ## 8-byte Folded Reload
	movq	%r14, -128(%rsp)                ## 8-byte Spill
	adcq	-120(%rsp), %rax                ## 8-byte Folded Reload
	movq	%rax, -112(%rsp)                ## 8-byte Spill
	movq	-72(%rsp), %rax                 ## 8-byte Reload
	movq	8(%rax), %rdi
	movq	%rdi, %rax
	mulq	-80(%rsp)                       ## 8-byte Folded Reload
	movq	%rdx, %r8
	movq	%rax, -120(%rsp)                ## 8-byte Spill
	movq	%rdi, %rax
	mulq	-88(%rsp)                       ## 8-byte Folded Reload
	movq	%rdx, %r9
	movq	%rax, -96(%rsp)                 ## 8-byte Spill
	movq	%rdi, %rax
	mulq	-56(%rsp)                       ## 8-byte Folded Reload
	movq	%rdx, %r13
	movq	%rax, -104(%rsp)                ## 8-byte Spill
	movq	%rdi, %rax
	mulq	8(%rsp)                         ## 8-byte Folded Reload
	movq	%rdx, %rsi
	movq	%rax, %r14
	movq	%rdi, %rax
	mulq	-8(%rsp)                        ## 8-byte Folded Reload
	movq	%rdx, %rbx
	movq	%rax, %r11
	movq	%rdi, %rax
	mulq	(%rsp)                          ## 8-byte Folded Reload
	movq	%rax, %rdi
	movq	%rdx, %rbp
	addq	%r11, %rbp
	adcq	%r14, %rbx
	adcq	-104(%rsp), %rsi                ## 8-byte Folded Reload
	adcq	-96(%rsp), %r13                 ## 8-byte Folded Reload
	adcq	-120(%rsp), %r9                 ## 8-byte Folded Reload
	adcq	$0, %r8
	addq	%rcx, %rdi
	adcq	%r10, %rbp
	adcq	%r12, %rbx
	adcq	%r15, %rsi
	adcq	-128(%rsp), %r13                ## 8-byte Folded Reload
	adcq	-112(%rsp), %r9                 ## 8-byte Folded Reload
	adcq	$0, %r8
	movq	-48(%rsp), %r11                 ## 8-byte Reload
	imulq	%rdi, %r11
	movq	%r11, %rax
	mulq	-64(%rsp)                       ## 8-byte Folded Reload
	movq	%rdx, -128(%rsp)                ## 8-byte Spill
	movq	%rax, -112(%rsp)                ## 8-byte Spill
	movq	%r11, %rax
	mulq	-16(%rsp)                       ## 8-byte Folded Reload
	movq	%rdx, -96(%rsp)                 ## 8-byte Spill
	movq	%rax, %r15
	movq	%r11, %rax
	mulq	-24(%rsp)                       ## 8-byte Folded Reload
	movq	%rdx, -120(%rsp)                ## 8-byte Spill
	movq	%rax, %rcx
	movq	%r11, %rax
	mulq	-40(%rsp)                       ## 8-byte Folded Reload
	movq	%rdx, -104(%rsp)                ## 8-byte Spill
	movq	%rax, %r10
	movq	%r11, %rax
	mulq	16(%rsp)                        ## 8-byte Folded Reload
	movq	%rdx, %r12
	movq	%rax, %r14
	movq	%r11, %rax
	mulq	-32(%rsp)                       ## 8-byte Folded Reload
	addq	%rdi, %rax
	adcq	%rbp, %r14
	adcq	%rbx, %r10
	adcq	%rsi, %rcx
	adcq	%r13, %r15
	movq	-112(%rsp), %rax                ## 8-byte Reload
	adcq	%r9, %rax
	adcq	$0, %r8
	addq	%rdx, %r14
	adcq	%r12, %r10
	adcq	-104(%rsp), %rcx                ## 8-byte Folded Reload
	adcq	-120(%rsp), %r15                ## 8-byte Folded Reload
	movq	%r15, -120(%rsp)                ## 8-byte Spill
	adcq	-96(%rsp), %rax                 ## 8-byte Folded Reload
	movq	%rax, -112(%rsp)                ## 8-byte Spill
	adcq	-128(%rsp), %r8                 ## 8-byte Folded Reload
	movq	-72(%rsp), %rax                 ## 8-byte Reload
	movq	16(%rax), %rdi
	movq	%rdi, %rax
	mulq	-80(%rsp)                       ## 8-byte Folded Reload
	movq	%rdx, %r13
	movq	%rax, -128(%rsp)                ## 8-byte Spill
	movq	%rdi, %rax
	mulq	-88(%rsp)                       ## 8-byte Folded Reload
	movq	%rdx, %r15
	movq	%rax, -96(%rsp)                 ## 8-byte Spill
	movq	%rdi, %rax
	mulq	-56(%rsp)                       ## 8-byte Folded Reload
	movq	%rdx, %r11
	movq	%rax, -104(%rsp)                ## 8-byte Spill
	movq	%rdi, %rax
	mulq	8(%rsp)                         ## 8-byte Folded Reload
	movq	%rdx, %r12
	movq	%rax, 24(%rsp)                  ## 8-byte Spill
	movq	%rdi, %rax
	mulq	-8(%rsp)                        ## 8-byte Folded Reload
	movq	%rdx, %rsi
	movq	%rax, %r9
	movq	%rdi, %rax
	mulq	(%rsp)                          ## 8-byte Folded Reload
	movq	%rax, %rbp
	movq	%rdx, %rbx
	addq	%r9, %rbx
	adcq	24(%rsp), %rsi                  ## 8-byte Folded Reload
	adcq	-104(%rsp), %r12                ## 8-byte Folded Reload
	adcq	-96(%rsp), %r11                 ## 8-byte Folded Reload
	adcq	-128(%rsp), %r15                ## 8-byte Folded Reload
	adcq	$0, %r13
	addq	%r14, %rbp
	adcq	%r10, %rbx
	adcq	%rcx, %rsi
	adcq	-120(%rsp), %r12                ## 8-byte Folded Reload
	adcq	-112(%rsp), %r11                ## 8-byte Folded Reload
	adcq	%r8, %r15
	adcq	$0, %r13
	movq	-48(%rsp), %rcx                 ## 8-byte Reload
	imulq	%rbp, %rcx
	movq	%rcx, %rax
	mulq	-64(%rsp)                       ## 8-byte Folded Reload
	movq	%rdx, -120(%rsp)                ## 8-byte Spill
	movq	%rax, -112(%rsp)                ## 8-byte Spill
	movq	%rcx, %rax
	mulq	-16(%rsp)                       ## 8-byte Folded Reload
	movq	%rdx, -128(%rsp)                ## 8-byte Spill
	movq	%rax, %r9
	movq	%rcx, %rax
	mulq	-24(%rsp)                       ## 8-byte Folded Reload
	movq	%rdx, -96(%rsp)                 ## 8-byte Spill
	movq	%rax, %r10
	movq	%rcx, %rax
	mulq	-40(%rsp)                       ## 8-byte Folded Reload
	movq	%rdx, -104(%rsp)                ## 8-byte Spill
	movq	%rax, %r14
	movq	%rcx, %rax
	mulq	16(%rsp)                        ## 8-byte Folded Reload
	movq	%rdx, %r8
	movq	%rax, %rdi
	movq	%rcx, %rax
	mulq	-32(%rsp)                       ## 8-byte Folded Reload
	addq	%rbp, %rax
	adcq	%rbx, %rdi
	adcq	%rsi, %r14
	adcq	%r12, %r10
	adcq	%r11, %r9
	movq	-112(%rsp), %rax                ## 8-byte Reload
	adcq	%r15, %rax
	adcq	$0, %r13
	addq	%rdx, %rdi
	adcq	%r8, %r14
	adcq	-104(%rsp), %r10                ## 8-byte Folded Reload
	adcq	-96(%rsp), %r9                  ## 8-byte Folded Reload
	adcq	-128(%rsp), %rax                ## 8-byte Folded Reload
	movq	%rax, -112(%rsp)                ## 8-byte Spill
	adcq	-120(%rsp), %r13                ## 8-byte Folded Reload
	movq	-72(%rsp), %rax                 ## 8-byte Reload
	movq	24(%rax), %rbp
	movq	%rbp, %rax
	mulq	-80(%rsp)                       ## 8-byte Folded Reload
	movq	%rdx, %r15
	movq	%rax, -120(%rsp)                ## 8-byte Spill
	movq	%rbp, %rax
	mulq	-88(%rsp)                       ## 8-byte Folded Reload
	movq	%rdx, %r11
	movq	%rax, -128(%rsp)                ## 8-byte Spill
	movq	%rbp, %rax
	mulq	-56(%rsp)                       ## 8-byte Folded Reload
	movq	%rdx, %rcx
	movq	%rax, -96(%rsp)                 ## 8-byte Spill
	movq	%rbp, %rax
	mulq	8(%rsp)                         ## 8-byte Folded Reload
	movq	%rdx, %rsi
	movq	%rax, -104(%rsp)                ## 8-byte Spill
	movq	%rbp, %rax
	mulq	-8(%rsp)                        ## 8-byte Folded Reload
	movq	%rdx, %rbx
	movq	%rax, %r12
	movq	%rbp, %rax
	mulq	(%rsp)                          ## 8-byte Folded Reload
	movq	%rax, %r8
	movq	%rdx, %rbp
	addq	%r12, %rbp
	adcq	-104(%rsp), %rbx                ## 8-byte Folded Reload
	adcq	-96(%rsp), %rsi                 ## 8-byte Folded Reload
	adcq	-128(%rsp), %rcx                ## 8-byte Folded Reload
	adcq	-120(%rsp), %r11                ## 8-byte Folded Reload
	adcq	$0, %r15
	addq	%rdi, %r8
	adcq	%r14, %rbp
	adcq	%r10, %rbx
	adcq	%r9, %rsi
	adcq	-112(%rsp), %rcx                ## 8-byte Folded Reload
	adcq	%r13, %r11
	adcq	$0, %r15
	movq	-48(%rsp), %r13                 ## 8-byte Reload
	imulq	%r8, %r13
	movq	%r13, %rax
	mulq	-64(%rsp)                       ## 8-byte Folded Reload
	movq	%rdx, -120(%rsp)                ## 8-byte Spill
	movq	%rax, -96(%rsp)                 ## 8-byte Spill
	movq	%r13, %rax
	mulq	-16(%rsp)                       ## 8-byte Folded Reload
	movq	%rdx, -128(%rsp)                ## 8-byte Spill
	movq	%rax, %r9
	movq	%r13, %rax
	mulq	-24(%rsp)                       ## 8-byte Folded Reload
	movq	%rdx, -112(%rsp)                ## 8-byte Spill
	movq	%rax, %r10
	movq	%r13, %rax
	mulq	-40(%rsp)                       ## 8-byte Folded Reload
	movq	%rdx, -104(%rsp)                ## 8-byte Spill
	movq	%rax, %r12
	movq	%r13, %rax
	mulq	16(%rsp)                        ## 8-byte Folded Reload
	movq	%rdx, %r14
	movq	%rax, %rdi
	movq	%r13, %rax
	mulq	-32(%rsp)                       ## 8-byte Folded Reload
	addq	%r8, %rax
	adcq	%rbp, %rdi
	adcq	%rbx, %r12
	adcq	%rsi, %r10
	movq	%r9, %rax
	adcq	%rcx, %rax
	movq	-96(%rsp), %r9                  ## 8-byte Reload
	adcq	%r11, %r9
	adcq	$0, %r15
	addq	%rdx, %rdi
	adcq	%r14, %r12
	adcq	-104(%rsp), %r10                ## 8-byte Folded Reload
	adcq	-112(%rsp), %rax                ## 8-byte Folded Reload
	movq	%rax, -112(%rsp)                ## 8-byte Spill
	adcq	-128(%rsp), %r9                 ## 8-byte Folded Reload
	movq	%r9, %rcx
	adcq	-120(%rsp), %r15                ## 8-byte Folded Reload
	movq	-72(%rsp), %rax                 ## 8-byte Reload
	movq	32(%rax), %rbp
	movq	%rbp, %rax
	mulq	-80(%rsp)                       ## 8-byte Folded Reload
	movq	%rdx, %r14
	movq	%rax, -120(%rsp)                ## 8-byte Spill
	movq	%rbp, %rax
	mulq	-88(%rsp)                       ## 8-byte Folded Reload
	movq	%rdx, %r11
	movq	%rax, -128(%rsp)                ## 8-byte Spill
	movq	%rbp, %rax
	mulq	-56(%rsp)                       ## 8-byte Folded Reload
	movq	%rdx, %r9
	movq	%rax, -96(%rsp)                 ## 8-byte Spill
	movq	%rbp, %rax
	mulq	8(%rsp)                         ## 8-byte Folded Reload
	movq	%rdx, %rsi
	movq	%rax, -104(%rsp)                ## 8-byte Spill
	movq	%rbp, %rax
	mulq	-8(%rsp)                        ## 8-byte Folded Reload
	movq	%rdx, %rbx
	movq	%rax, %r8
	movq	%rbp, %rax
	mulq	(%rsp)                          ## 8-byte Folded Reload
	movq	%rax, %r13
	movq	%rdx, %rbp
	addq	%r8, %rbp
	adcq	-104(%rsp), %rbx                ## 8-byte Folded Reload
	adcq	-96(%rsp), %rsi                 ## 8-byte Folded Reload
	adcq	-128(%rsp), %r9                 ## 8-byte Folded Reload
	adcq	-120(%rsp), %r11                ## 8-byte Folded Reload
	adcq	$0, %r14
	addq	%rdi, %r13
	adcq	%r12, %rbp
	adcq	%r10, %rbx
	adcq	-112(%rsp), %rsi                ## 8-byte Folded Reload
	adcq	%rcx, %r9
	adcq	%r15, %r11
	adcq	$0, %r14
	movq	-48(%rsp), %rcx                 ## 8-byte Reload
	imulq	%r13, %rcx
	movq	%rcx, %rax
	mulq	-64(%rsp)                       ## 8-byte Folded Reload
	movq	%rdx, -96(%rsp)                 ## 8-byte Spill
	movq	%rax, -112(%rsp)                ## 8-byte Spill
	movq	%rcx, %rax
	mulq	-16(%rsp)                       ## 8-byte Folded Reload
	movq	%rdx, -104(%rsp)                ## 8-byte Spill
	movq	%rax, %r15
	movq	%rcx, %rax
	mulq	-24(%rsp)                       ## 8-byte Folded Reload
	movq	%rdx, -120(%rsp)                ## 8-byte Spill
	movq	%rax, %r10
	movq	%rcx, %rax
	mulq	-40(%rsp)                       ## 8-byte Folded Reload
	movq	%rdx, -128(%rsp)                ## 8-byte Spill
	movq	%rax, %r8
	movq	%rcx, %rax
	mulq	16(%rsp)                        ## 8-byte Folded Reload
	movq	%rdx, %r12
	movq	%rax, %rdi
	movq	%rcx, %rax
	mulq	-32(%rsp)                       ## 8-byte Folded Reload
	addq	%r13, %rax
	adcq	%rbp, %rdi
	adcq	%rbx, %r8
	adcq	%rsi, %r10
	adcq	%r9, %r15
	movq	-112(%rsp), %rcx                ## 8-byte Reload
	adcq	%r11, %rcx
	adcq	$0, %r14
	addq	%rdx, %rdi
	adcq	%r12, %r8
	adcq	-128(%rsp), %r10                ## 8-byte Folded Reload
	movq	%r10, -128(%rsp)                ## 8-byte Spill
	adcq	-120(%rsp), %r15                ## 8-byte Folded Reload
	movq	%r15, -120(%rsp)                ## 8-byte Spill
	adcq	-104(%rsp), %rcx                ## 8-byte Folded Reload
	movq	%rcx, -112(%rsp)                ## 8-byte Spill
	adcq	-96(%rsp), %r14                 ## 8-byte Folded Reload
	movq	-72(%rsp), %rax                 ## 8-byte Reload
	movq	40(%rax), %rcx
	movq	%rcx, %rax
	mulq	-80(%rsp)                       ## 8-byte Folded Reload
	movq	%rdx, %rbx
	movq	%rax, -72(%rsp)                 ## 8-byte Spill
	movq	%rcx, %rax
	mulq	-88(%rsp)                       ## 8-byte Folded Reload
	movq	%rdx, %r11
	movq	%rax, -80(%rsp)                 ## 8-byte Spill
	movq	%rcx, %rax
	mulq	-56(%rsp)                       ## 8-byte Folded Reload
	movq	%rdx, %r15
	movq	%rax, -88(%rsp)                 ## 8-byte Spill
	movq	%rcx, %rax
	mulq	8(%rsp)                         ## 8-byte Folded Reload
	movq	%rdx, %r12
	movq	%rax, %rbp
	movq	%rcx, %rax
	mulq	-8(%rsp)                        ## 8-byte Folded Reload
	movq	%rdx, %r13
	movq	%rax, %rsi
	movq	%rcx, %rax
	mulq	(%rsp)                          ## 8-byte Folded Reload
	movq	%rax, %r10
	movq	%rdx, %r9
	addq	%rsi, %r9
	adcq	%rbp, %r13
	adcq	-88(%rsp), %r12                 ## 8-byte Folded Reload
	adcq	-80(%rsp), %r15                 ## 8-byte Folded Reload
	adcq	-72(%rsp), %r11                 ## 8-byte Folded Reload
	adcq	$0, %rbx
	addq	%rdi, %r10
	adcq	%r8, %r9
	adcq	-128(%rsp), %r13                ## 8-byte Folded Reload
	adcq	-120(%rsp), %r12                ## 8-byte Folded Reload
	adcq	-112(%rsp), %r15                ## 8-byte Folded Reload
	adcq	%r14, %r11
	adcq	$0, %rbx
	movq	-48(%rsp), %r14                 ## 8-byte Reload
	imulq	%r10, %r14
	movq	%r14, %rax
	mulq	-64(%rsp)                       ## 8-byte Folded Reload
	movq	%rdx, -48(%rsp)                 ## 8-byte Spill
	movq	%rax, %rdi
	movq	%r14, %rax
	mulq	-16(%rsp)                       ## 8-byte Folded Reload
	movq	%rdx, -72(%rsp)                 ## 8-byte Spill
	movq	%rax, %rbp
	movq	%r14, %rax
	mulq	-24(%rsp)                       ## 8-byte Folded Reload
	movq	%rdx, -80(%rsp)                 ## 8-byte Spill
	movq	%rax, %rcx
	movq	%r14, %rax
	mulq	-32(%rsp)                       ## 8-byte Folded Reload
	movq	%rdx, -88(%rsp)                 ## 8-byte Spill
	movq	%rax, %r8
	movq	%r14, %rax
	mulq	-40(%rsp)                       ## 8-byte Folded Reload
	movq	%rdx, -56(%rsp)                 ## 8-byte Spill
	movq	%rax, %rsi
	movq	%r14, %rax
	movq	16(%rsp), %r14                  ## 8-byte Reload
	mulq	%r14
	addq	%r10, %r8
	adcq	%r9, %rax
	adcq	%r13, %rsi
	adcq	%r12, %rcx
	adcq	%r15, %rbp
	adcq	%r11, %rdi
	adcq	$0, %rbx
	addq	-88(%rsp), %rax                 ## 8-byte Folded Reload
	adcq	%rdx, %rsi
	adcq	-56(%rsp), %rcx                 ## 8-byte Folded Reload
	adcq	-80(%rsp), %rbp                 ## 8-byte Folded Reload
	adcq	-72(%rsp), %rdi                 ## 8-byte Folded Reload
	adcq	-48(%rsp), %rbx                 ## 8-byte Folded Reload
	movq	%rax, %r8
	subq	-32(%rsp), %r8                  ## 8-byte Folded Reload
	movq	%rsi, %r9
	sbbq	%r14, %r9
	movq	%rcx, %r10
	sbbq	-40(%rsp), %r10                 ## 8-byte Folded Reload
	movq	%rbp, %r11
	sbbq	-24(%rsp), %r11                 ## 8-byte Folded Reload
	movq	%rdi, %r14
	sbbq	-16(%rsp), %r14                 ## 8-byte Folded Reload
	movq	%rbx, %r15
	sbbq	-64(%rsp), %r15                 ## 8-byte Folded Reload
	movq	%r15, %rdx
	sarq	$63, %rdx
	cmovsq	%rbx, %r15
	movq	32(%rsp), %rdx                  ## 8-byte Reload
	movq	%r15, 40(%rdx)
	cmovsq	%rdi, %r14
	movq	%r14, 32(%rdx)
	cmovsq	%rbp, %r11
	movq	%r11, 24(%rdx)
	cmovsq	%rcx, %r10
	movq	%r10, 16(%rdx)
	cmovsq	%rsi, %r9
	movq	%r9, 8(%rdx)
	cmovsq	%rax, %r8
	movq	%r8, (%rdx)
	addq	$40, %rsp
	popq	%rbx
	popq	%r12
	popq	%r13
	popq	%r14
	popq	%r15
	popq	%rbp
	retq
                                        ## -- End function
	.globl	_mcl_fp_montRed6L               ## -- Begin function mcl_fp_montRed6L
	.p2align	4, 0x90
_mcl_fp_montRed6L:                      ## @mcl_fp_montRed6L
## %bb.0:
	pushq	%rbp
	pushq	%r15
	pushq	%r14
	pushq	%r13
	pushq	%r12
	pushq	%rbx
	pushq	%rax
	movq	%rdx, %rcx
	movq	%rdi, (%rsp)                    ## 8-byte Spill
	movq	-8(%rdx), %rax
	movq	%rax, -80(%rsp)                 ## 8-byte Spill
	movq	(%rsi), %r9
	movq	%r9, %rdi
	imulq	%rax, %rdi
	movq	40(%rdx), %rdx
	movq	%rdx, -40(%rsp)                 ## 8-byte Spill
	movq	%rdi, %rax
	mulq	%rdx
	movq	%rax, -128(%rsp)                ## 8-byte Spill
	movq	%rdx, -120(%rsp)                ## 8-byte Spill
	movq	32(%rcx), %rdx
	movq	%rdx, -64(%rsp)                 ## 8-byte Spill
	movq	%rdi, %rax
	mulq	%rdx
	movq	%rax, %r10
	movq	%rdx, %r12
	movq	24(%rcx), %rdx
	movq	%rdx, -72(%rsp)                 ## 8-byte Spill
	movq	%rdi, %rax
	mulq	%rdx
	movq	%rax, %r14
	movq	%rdx, %r15
	movq	16(%rcx), %rdx
	movq	%rdx, -48(%rsp)                 ## 8-byte Spill
	movq	%rdi, %rax
	mulq	%rdx
	movq	%rax, %r11
	movq	%rdx, %r13
	movq	(%rcx), %r8
	movq	8(%rcx), %rcx
	movq	%rcx, -24(%rsp)                 ## 8-byte Spill
	movq	%rdi, %rax
	mulq	%rcx
	movq	%rdx, %rbx
	movq	%rax, %rbp
	movq	%rdi, %rax
	mulq	%r8
	movq	%r8, %rdi
	movq	%r8, -16(%rsp)                  ## 8-byte Spill
	movq	%rdx, %rcx
	addq	%rbp, %rcx
	adcq	%r11, %rbx
	adcq	%r14, %r13
	adcq	%r10, %r15
	adcq	-128(%rsp), %r12                ## 8-byte Folded Reload
	movq	-120(%rsp), %rdx                ## 8-byte Reload
	adcq	$0, %rdx
	addq	%r9, %rax
	movq	%rsi, -32(%rsp)                 ## 8-byte Spill
	adcq	8(%rsi), %rcx
	adcq	16(%rsi), %rbx
	adcq	24(%rsi), %r13
	adcq	32(%rsi), %r15
	adcq	40(%rsi), %r12
	movq	%r12, -88(%rsp)                 ## 8-byte Spill
	adcq	48(%rsi), %rdx
	movq	%rdx, -120(%rsp)                ## 8-byte Spill
	setb	-96(%rsp)                       ## 1-byte Folded Spill
	movq	-80(%rsp), %rsi                 ## 8-byte Reload
	imulq	%rcx, %rsi
	movq	%rsi, %rax
	mulq	-40(%rsp)                       ## 8-byte Folded Reload
	movq	%rdx, -128(%rsp)                ## 8-byte Spill
	movq	%rax, -104(%rsp)                ## 8-byte Spill
	movq	%rsi, %rax
	mulq	-64(%rsp)                       ## 8-byte Folded Reload
	movq	%rdx, %r14
	movq	%rax, -112(%rsp)                ## 8-byte Spill
	movq	%rsi, %rax
	mulq	-72(%rsp)                       ## 8-byte Folded Reload
	movq	%rdx, %r12
	movq	%rax, -56(%rsp)                 ## 8-byte Spill
	movq	%rsi, %rax
	mulq	-48(%rsp)                       ## 8-byte Folded Reload
	movq	%rdx, %r8
	movq	%rax, %r9
	movq	%rsi, %rax
	mulq	%rdi
	movq	%rdx, %r10
	movq	%rax, %r11
	movq	%rsi, %rax
	movq	-24(%rsp), %rsi                 ## 8-byte Reload
	mulq	%rsi
	movq	%rdx, %rbp
	movq	%rax, %rdi
	addq	%r10, %rdi
	adcq	%r9, %rbp
	adcq	-56(%rsp), %r8                  ## 8-byte Folded Reload
	adcq	-112(%rsp), %r12                ## 8-byte Folded Reload
	adcq	-104(%rsp), %r14                ## 8-byte Folded Reload
	movzbl	-96(%rsp), %eax                 ## 1-byte Folded Reload
	movq	-128(%rsp), %rdx                ## 8-byte Reload
	adcq	%rax, %rdx
	addq	%rcx, %r11
	adcq	%rbx, %rdi
	adcq	%r13, %rbp
	adcq	%r15, %r8
	adcq	-88(%rsp), %r12                 ## 8-byte Folded Reload
	adcq	-120(%rsp), %r14                ## 8-byte Folded Reload
	movq	-32(%rsp), %rax                 ## 8-byte Reload
	adcq	56(%rax), %rdx
	movq	%rdx, -128(%rsp)                ## 8-byte Spill
	setb	-120(%rsp)                      ## 1-byte Folded Spill
	movq	-80(%rsp), %rcx                 ## 8-byte Reload
	imulq	%rdi, %rcx
	movq	%rcx, %rax
	mulq	-40(%rsp)                       ## 8-byte Folded Reload
	movq	%rdx, -88(%rsp)                 ## 8-byte Spill
	movq	%rax, -96(%rsp)                 ## 8-byte Spill
	movq	%rcx, %rax
	mulq	-64(%rsp)                       ## 8-byte Folded Reload
	movq	%rdx, %r11
	movq	%rax, -104(%rsp)                ## 8-byte Spill
	movq	%rcx, %rax
	mulq	-72(%rsp)                       ## 8-byte Folded Reload
	movq	%rdx, %r15
	movq	%rax, -112(%rsp)                ## 8-byte Spill
	movq	%rcx, %rax
	mulq	-48(%rsp)                       ## 8-byte Folded Reload
	movq	%rdx, %r13
	movq	%rax, %rbx
	movq	%rcx, %rax
	mulq	-16(%rsp)                       ## 8-byte Folded Reload
	movq	%rdx, %r10
	movq	%rax, %r9
	movq	%rcx, %rax
	mulq	%rsi
	movq	%rdx, %rcx
	movq	%rax, %rsi
	addq	%r10, %rsi
	adcq	%rbx, %rcx
	adcq	-112(%rsp), %r13                ## 8-byte Folded Reload
	adcq	-104(%rsp), %r15                ## 8-byte Folded Reload
	adcq	-96(%rsp), %r11                 ## 8-byte Folded Reload
	movzbl	-120(%rsp), %eax                ## 1-byte Folded Reload
	movq	-88(%rsp), %rdx                 ## 8-byte Reload
	adcq	%rax, %rdx
	addq	%rdi, %r9
	adcq	%rbp, %rsi
	adcq	%r8, %rcx
	adcq	%r12, %r13
	adcq	%r14, %r15
	adcq	-128(%rsp), %r11                ## 8-byte Folded Reload
	movq	-32(%rsp), %rax                 ## 8-byte Reload
	adcq	64(%rax), %rdx
	movq	%rdx, -88(%rsp)                 ## 8-byte Spill
	setb	-128(%rsp)                      ## 1-byte Folded Spill
	movq	-80(%rsp), %rdi                 ## 8-byte Reload
	imulq	%rsi, %rdi
	movq	%rdi, %rax
	mulq	-40(%rsp)                       ## 8-byte Folded Reload
	movq	%rdx, -96(%rsp)                 ## 8-byte Spill
	movq	%rax, -104(%rsp)                ## 8-byte Spill
	movq	%rdi, %rax
	mulq	-64(%rsp)                       ## 8-byte Folded Reload
	movq	%rdx, -120(%rsp)                ## 8-byte Spill
	movq	%rax, -112(%rsp)                ## 8-byte Spill
	movq	%rdi, %rax
	mulq	-72(%rsp)                       ## 8-byte Folded Reload
	movq	%rdx, %r12
	movq	%rax, -56(%rsp)                 ## 8-byte Spill
	movq	%rdi, %rax
	movq	-48(%rsp), %r14                 ## 8-byte Reload
	mulq	%r14
	movq	%rdx, %rbp
	movq	%rax, %r9
	movq	%rdi, %rax
	mulq	-16(%rsp)                       ## 8-byte Folded Reload
	movq	%rdx, %r8
	movq	%rax, %r10
	movq	%rdi, %rax
	mulq	-24(%rsp)                       ## 8-byte Folded Reload
	movq	%rdx, %rdi
	movq	%rax, %rbx
	addq	%r8, %rbx
	adcq	%r9, %rdi
	adcq	-56(%rsp), %rbp                 ## 8-byte Folded Reload
	adcq	-112(%rsp), %r12                ## 8-byte Folded Reload
	movq	-120(%rsp), %rdx                ## 8-byte Reload
	adcq	-104(%rsp), %rdx                ## 8-byte Folded Reload
	movzbl	-128(%rsp), %eax                ## 1-byte Folded Reload
	movq	-96(%rsp), %r8                  ## 8-byte Reload
	adcq	%rax, %r8
	addq	%rsi, %r10
	adcq	%rcx, %rbx
	adcq	%r13, %rdi
	adcq	%r15, %rbp
	adcq	%r11, %r12
	adcq	-88(%rsp), %rdx                 ## 8-byte Folded Reload
	movq	%rdx, -120(%rsp)                ## 8-byte Spill
	movq	-32(%rsp), %rax                 ## 8-byte Reload
	adcq	72(%rax), %r8
	movq	%r8, -96(%rsp)                  ## 8-byte Spill
	setb	-104(%rsp)                      ## 1-byte Folded Spill
	movq	-80(%rsp), %rcx                 ## 8-byte Reload
	imulq	%rbx, %rcx
	movq	%rcx, %rax
	movq	-40(%rsp), %r9                  ## 8-byte Reload
	mulq	%r9
	movq	%rdx, -88(%rsp)                 ## 8-byte Spill
	movq	%rax, -112(%rsp)                ## 8-byte Spill
	movq	%rcx, %rax
	mulq	-64(%rsp)                       ## 8-byte Folded Reload
	movq	%rdx, -128(%rsp)                ## 8-byte Spill
	movq	%rax, -56(%rsp)                 ## 8-byte Spill
	movq	%rcx, %rax
	mulq	-72(%rsp)                       ## 8-byte Folded Reload
	movq	%rdx, %r15
	movq	%rax, -8(%rsp)                  ## 8-byte Spill
	movq	%rcx, %rax
	mulq	%r14
	movq	%rdx, %r11
	movq	%rax, %r13
	movq	%rcx, %rax
	mulq	-16(%rsp)                       ## 8-byte Folded Reload
	movq	%rdx, %r10
	movq	%rax, %r14
	movq	%rcx, %rax
	mulq	-24(%rsp)                       ## 8-byte Folded Reload
	movq	%rdx, %r8
	movq	%rax, %rsi
	addq	%r10, %rsi
	adcq	%r13, %r8
	adcq	-8(%rsp), %r11                  ## 8-byte Folded Reload
	adcq	-56(%rsp), %r15                 ## 8-byte Folded Reload
	movq	-128(%rsp), %rdx                ## 8-byte Reload
	adcq	-112(%rsp), %rdx                ## 8-byte Folded Reload
	movzbl	-104(%rsp), %eax                ## 1-byte Folded Reload
	movq	-88(%rsp), %rcx                 ## 8-byte Reload
	adcq	%rax, %rcx
	addq	%rbx, %r14
	adcq	%rdi, %rsi
	adcq	%rbp, %r8
	adcq	%r12, %r11
	adcq	-120(%rsp), %r15                ## 8-byte Folded Reload
	adcq	-96(%rsp), %rdx                 ## 8-byte Folded Reload
	movq	%rdx, -128(%rsp)                ## 8-byte Spill
	movq	-32(%rsp), %rax                 ## 8-byte Reload
	adcq	80(%rax), %rcx
	movq	%rcx, -88(%rsp)                 ## 8-byte Spill
	setb	-120(%rsp)                      ## 1-byte Folded Spill
	movq	-80(%rsp), %rcx                 ## 8-byte Reload
	imulq	%rsi, %rcx
	movq	%rcx, %rax
	mulq	%r9
	movq	%rdx, -80(%rsp)                 ## 8-byte Spill
	movq	%rax, -96(%rsp)                 ## 8-byte Spill
	movq	%rcx, %rax
	mulq	-64(%rsp)                       ## 8-byte Folded Reload
	movq	%rdx, %rdi
	movq	%rax, -104(%rsp)                ## 8-byte Spill
	movq	%rcx, %rax
	mulq	-72(%rsp)                       ## 8-byte Folded Reload
	movq	%rdx, %rbp
	movq	%rax, -112(%rsp)                ## 8-byte Spill
	movq	%rcx, %rax
	movq	-16(%rsp), %r13                 ## 8-byte Reload
	mulq	%r13
	movq	%rdx, %r14
	movq	%rax, %r9
	movq	%rcx, %rax
	mulq	-48(%rsp)                       ## 8-byte Folded Reload
	movq	%rdx, %rbx
	movq	%rax, %r10
	movq	%rcx, %rax
	movq	-24(%rsp), %r12                 ## 8-byte Reload
	mulq	%r12
	addq	%r14, %rax
	adcq	%r10, %rdx
	adcq	-112(%rsp), %rbx                ## 8-byte Folded Reload
	adcq	-104(%rsp), %rbp                ## 8-byte Folded Reload
	adcq	-96(%rsp), %rdi                 ## 8-byte Folded Reload
	movzbl	-120(%rsp), %r10d               ## 1-byte Folded Reload
	adcq	-80(%rsp), %r10                 ## 8-byte Folded Reload
	addq	%rsi, %r9
	adcq	%r8, %rax
	adcq	%r11, %rdx
	adcq	%r15, %rbx
	adcq	-128(%rsp), %rbp                ## 8-byte Folded Reload
	adcq	-88(%rsp), %rdi                 ## 8-byte Folded Reload
	movq	-32(%rsp), %rcx                 ## 8-byte Reload
	adcq	88(%rcx), %r10
	xorl	%r8d, %r8d
	movq	%rax, %r9
	subq	%r13, %r9
	movq	%rdx, %r11
	sbbq	%r12, %r11
	movq	%rbx, %r14
	sbbq	-48(%rsp), %r14                 ## 8-byte Folded Reload
	movq	%rbp, %r15
	sbbq	-72(%rsp), %r15                 ## 8-byte Folded Reload
	movq	%rdi, %r12
	sbbq	-64(%rsp), %r12                 ## 8-byte Folded Reload
	movq	%r10, %rcx
	sbbq	-40(%rsp), %rcx                 ## 8-byte Folded Reload
	sbbq	%r8, %r8
	testb	$1, %r8b
	cmovneq	%r10, %rcx
	movq	(%rsp), %rsi                    ## 8-byte Reload
	movq	%rcx, 40(%rsi)
	cmovneq	%rdi, %r12
	movq	%r12, 32(%rsi)
	cmovneq	%rbp, %r15
	movq	%r15, 24(%rsi)
	cmovneq	%rbx, %r14
	movq	%r14, 16(%rsi)
	cmovneq	%rdx, %r11
	movq	%r11, 8(%rsi)
	cmovneq	%rax, %r9
	movq	%r9, (%rsi)
	addq	$8, %rsp
	popq	%rbx
	popq	%r12
	popq	%r13
	popq	%r14
	popq	%r15
	popq	%rbp
	retq
                                        ## -- End function
	.globl	_mcl_fp_montRedNF6L             ## -- Begin function mcl_fp_montRedNF6L
	.p2align	4, 0x90
_mcl_fp_montRedNF6L:                    ## @mcl_fp_montRedNF6L
## %bb.0:
	pushq	%rbp
	pushq	%r15
	pushq	%r14
	pushq	%r13
	pushq	%r12
	pushq	%rbx
	pushq	%rax
	movq	%rdx, %rcx
	movq	%rdi, (%rsp)                    ## 8-byte Spill
	movq	-8(%rdx), %rax
	movq	%rax, -80(%rsp)                 ## 8-byte Spill
	movq	(%rsi), %r9
	movq	%r9, %rdi
	imulq	%rax, %rdi
	movq	40(%rdx), %rdx
	movq	%rdx, -40(%rsp)                 ## 8-byte Spill
	movq	%rdi, %rax
	mulq	%rdx
	movq	%rax, -128(%rsp)                ## 8-byte Spill
	movq	%rdx, -120(%rsp)                ## 8-byte Spill
	movq	32(%rcx), %rdx
	movq	%rdx, -64(%rsp)                 ## 8-byte Spill
	movq	%rdi, %rax
	mulq	%rdx
	movq	%rax, %r10
	movq	%rdx, %r12
	movq	24(%rcx), %rdx
	movq	%rdx, -72(%rsp)                 ## 8-byte Spill
	movq	%rdi, %rax
	mulq	%rdx
	movq	%rax, %r14
	movq	%rdx, %r15
	movq	16(%rcx), %rdx
	movq	%rdx, -48(%rsp)                 ## 8-byte Spill
	movq	%rdi, %rax
	mulq	%rdx
	movq	%rax, %r11
	movq	%rdx, %r13
	movq	(%rcx), %r8
	movq	8(%rcx), %rcx
	movq	%rcx, -24(%rsp)                 ## 8-byte Spill
	movq	%rdi, %rax
	mulq	%rcx
	movq	%rdx, %rbx
	movq	%rax, %rbp
	movq	%rdi, %rax
	mulq	%r8
	movq	%r8, %rdi
	movq	%r8, -16(%rsp)                  ## 8-byte Spill
	movq	%rdx, %rcx
	addq	%rbp, %rcx
	adcq	%r11, %rbx
	adcq	%r14, %r13
	adcq	%r10, %r15
	adcq	-128(%rsp), %r12                ## 8-byte Folded Reload
	movq	-120(%rsp), %rdx                ## 8-byte Reload
	adcq	$0, %rdx
	addq	%r9, %rax
	movq	%rsi, -32(%rsp)                 ## 8-byte Spill
	adcq	8(%rsi), %rcx
	adcq	16(%rsi), %rbx
	adcq	24(%rsi), %r13
	adcq	32(%rsi), %r15
	adcq	40(%rsi), %r12
	movq	%r12, -88(%rsp)                 ## 8-byte Spill
	adcq	48(%rsi), %rdx
	movq	%rdx, -120(%rsp)                ## 8-byte Spill
	setb	-96(%rsp)                       ## 1-byte Folded Spill
	movq	-80(%rsp), %rsi                 ## 8-byte Reload
	imulq	%rcx, %rsi
	movq	%rsi, %rax
	mulq	-40(%rsp)                       ## 8-byte Folded Reload
	movq	%rdx, -128(%rsp)                ## 8-byte Spill
	movq	%rax, -104(%rsp)                ## 8-byte Spill
	movq	%rsi, %rax
	mulq	-64(%rsp)                       ## 8-byte Folded Reload
	movq	%rdx, %r14
	movq	%rax, -112(%rsp)                ## 8-byte Spill
	movq	%rsi, %rax
	mulq	-72(%rsp)                       ## 8-byte Folded Reload
	movq	%rdx, %r12
	movq	%rax, -56(%rsp)                 ## 8-byte Spill
	movq	%rsi, %rax
	mulq	-48(%rsp)                       ## 8-byte Folded Reload
	movq	%rdx, %r8
	movq	%rax, %r9
	movq	%rsi, %rax
	mulq	%rdi
	movq	%rdx, %r10
	movq	%rax, %r11
	movq	%rsi, %rax
	movq	-24(%rsp), %rsi                 ## 8-byte Reload
	mulq	%rsi
	movq	%rdx, %rbp
	movq	%rax, %rdi
	addq	%r10, %rdi
	adcq	%r9, %rbp
	adcq	-56(%rsp), %r8                  ## 8-byte Folded Reload
	adcq	-112(%rsp), %r12                ## 8-byte Folded Reload
	adcq	-104(%rsp), %r14                ## 8-byte Folded Reload
	movzbl	-96(%rsp), %eax                 ## 1-byte Folded Reload
	movq	-128(%rsp), %rdx                ## 8-byte Reload
	adcq	%rax, %rdx
	addq	%rcx, %r11
	adcq	%rbx, %rdi
	adcq	%r13, %rbp
	adcq	%r15, %r8
	adcq	-88(%rsp), %r12                 ## 8-byte Folded Reload
	adcq	-120(%rsp), %r14                ## 8-byte Folded Reload
	movq	-32(%rsp), %rax                 ## 8-byte Reload
	adcq	56(%rax), %rdx
	movq	%rdx, -128(%rsp)                ## 8-byte Spill
	setb	-120(%rsp)                      ## 1-byte Folded Spill
	movq	-80(%rsp), %rcx                 ## 8-byte Reload
	imulq	%rdi, %rcx
	movq	%rcx, %rax
	mulq	-40(%rsp)                       ## 8-byte Folded Reload
	movq	%rdx, -88(%rsp)                 ## 8-byte Spill
	movq	%rax, -96(%rsp)                 ## 8-byte Spill
	movq	%rcx, %rax
	mulq	-64(%rsp)                       ## 8-byte Folded Reload
	movq	%rdx, %r11
	movq	%rax, -104(%rsp)                ## 8-byte Spill
	movq	%rcx, %rax
	mulq	-72(%rsp)                       ## 8-byte Folded Reload
	movq	%rdx, %r15
	movq	%rax, -112(%rsp)                ## 8-byte Spill
	movq	%rcx, %rax
	mulq	-48(%rsp)                       ## 8-byte Folded Reload
	movq	%rdx, %r13
	movq	%rax, %rbx
	movq	%rcx, %rax
	mulq	-16(%rsp)                       ## 8-byte Folded Reload
	movq	%rdx, %r10
	movq	%rax, %r9
	movq	%rcx, %rax
	mulq	%rsi
	movq	%rdx, %rcx
	movq	%rax, %rsi
	addq	%r10, %rsi
	adcq	%rbx, %rcx
	adcq	-112(%rsp), %r13                ## 8-byte Folded Reload
	adcq	-104(%rsp), %r15                ## 8-byte Folded Reload
	adcq	-96(%rsp), %r11                 ## 8-byte Folded Reload
	movzbl	-120(%rsp), %eax                ## 1-byte Folded Reload
	movq	-88(%rsp), %rdx                 ## 8-byte Reload
	adcq	%rax, %rdx
	addq	%rdi, %r9
	adcq	%rbp, %rsi
	adcq	%r8, %rcx
	adcq	%r12, %r13
	adcq	%r14, %r15
	adcq	-128(%rsp), %r11                ## 8-byte Folded Reload
	movq	-32(%rsp), %rax                 ## 8-byte Reload
	adcq	64(%rax), %rdx
	movq	%rdx, -88(%rsp)                 ## 8-byte Spill
	setb	-128(%rsp)                      ## 1-byte Folded Spill
	movq	-80(%rsp), %rdi                 ## 8-byte Reload
	imulq	%rsi, %rdi
	movq	%rdi, %rax
	mulq	-40(%rsp)                       ## 8-byte Folded Reload
	movq	%rdx, -96(%rsp)                 ## 8-byte Spill
	movq	%rax, -104(%rsp)                ## 8-byte Spill
	movq	%rdi, %rax
	mulq	-64(%rsp)                       ## 8-byte Folded Reload
	movq	%rdx, -120(%rsp)                ## 8-byte Spill
	movq	%rax, -112(%rsp)                ## 8-byte Spill
	movq	%rdi, %rax
	mulq	-72(%rsp)                       ## 8-byte Folded Reload
	movq	%rdx, %r12
	movq	%rax, -56(%rsp)                 ## 8-byte Spill
	movq	%rdi, %rax
	movq	-48(%rsp), %r14                 ## 8-byte Reload
	mulq	%r14
	movq	%rdx, %rbp
	movq	%rax, %r9
	movq	%rdi, %rax
	mulq	-16(%rsp)                       ## 8-byte Folded Reload
	movq	%rdx, %r8
	movq	%rax, %r10
	movq	%rdi, %rax
	mulq	-24(%rsp)                       ## 8-byte Folded Reload
	movq	%rdx, %rdi
	movq	%rax, %rbx
	addq	%r8, %rbx
	adcq	%r9, %rdi
	adcq	-56(%rsp), %rbp                 ## 8-byte Folded Reload
	adcq	-112(%rsp), %r12                ## 8-byte Folded Reload
	movq	-120(%rsp), %rdx                ## 8-byte Reload
	adcq	-104(%rsp), %rdx                ## 8-byte Folded Reload
	movzbl	-128(%rsp), %eax                ## 1-byte Folded Reload
	movq	-96(%rsp), %r8                  ## 8-byte Reload
	adcq	%rax, %r8
	addq	%rsi, %r10
	adcq	%rcx, %rbx
	adcq	%r13, %rdi
	adcq	%r15, %rbp
	adcq	%r11, %r12
	adcq	-88(%rsp), %rdx                 ## 8-byte Folded Reload
	movq	%rdx, -120(%rsp)                ## 8-byte Spill
	movq	-32(%rsp), %rax                 ## 8-byte Reload
	adcq	72(%rax), %r8
	movq	%r8, -96(%rsp)                  ## 8-byte Spill
	setb	-104(%rsp)                      ## 1-byte Folded Spill
	movq	-80(%rsp), %rcx                 ## 8-byte Reload
	imulq	%rbx, %rcx
	movq	%rcx, %rax
	movq	-40(%rsp), %r9                  ## 8-byte Reload
	mulq	%r9
	movq	%rdx, -88(%rsp)                 ## 8-byte Spill
	movq	%rax, -112(%rsp)                ## 8-byte Spill
	movq	%rcx, %rax
	mulq	-64(%rsp)                       ## 8-byte Folded Reload
	movq	%rdx, -128(%rsp)                ## 8-byte Spill
	movq	%rax, -56(%rsp)                 ## 8-byte Spill
	movq	%rcx, %rax
	mulq	-72(%rsp)                       ## 8-byte Folded Reload
	movq	%rdx, %r15
	movq	%rax, -8(%rsp)                  ## 8-byte Spill
	movq	%rcx, %rax
	mulq	%r14
	movq	%rdx, %r11
	movq	%rax, %r13
	movq	%rcx, %rax
	mulq	-16(%rsp)                       ## 8-byte Folded Reload
	movq	%rdx, %r10
	movq	%rax, %r14
	movq	%rcx, %rax
	mulq	-24(%rsp)                       ## 8-byte Folded Reload
	movq	%rdx, %r8
	movq	%rax, %rsi
	addq	%r10, %rsi
	adcq	%r13, %r8
	adcq	-8(%rsp), %r11                  ## 8-byte Folded Reload
	adcq	-56(%rsp), %r15                 ## 8-byte Folded Reload
	movq	-128(%rsp), %rdx                ## 8-byte Reload
	adcq	-112(%rsp), %rdx                ## 8-byte Folded Reload
	movzbl	-104(%rsp), %eax                ## 1-byte Folded Reload
	movq	-88(%rsp), %rcx                 ## 8-byte Reload
	adcq	%rax, %rcx
	addq	%rbx, %r14
	adcq	%rdi, %rsi
	adcq	%rbp, %r8
	adcq	%r12, %r11
	adcq	-120(%rsp), %r15                ## 8-byte Folded Reload
	adcq	-96(%rsp), %rdx                 ## 8-byte Folded Reload
	movq	%rdx, -128(%rsp)                ## 8-byte Spill
	movq	-32(%rsp), %rax                 ## 8-byte Reload
	adcq	80(%rax), %rcx
	movq	%rcx, -88(%rsp)                 ## 8-byte Spill
	setb	-120(%rsp)                      ## 1-byte Folded Spill
	movq	-80(%rsp), %rcx                 ## 8-byte Reload
	imulq	%rsi, %rcx
	movq	%rcx, %rax
	mulq	%r9
	movq	%rdx, -80(%rsp)                 ## 8-byte Spill
	movq	%rax, -96(%rsp)                 ## 8-byte Spill
	movq	%rcx, %rax
	mulq	-64(%rsp)                       ## 8-byte Folded Reload
	movq	%rdx, %rdi
	movq	%rax, -104(%rsp)                ## 8-byte Spill
	movq	%rcx, %rax
	mulq	-72(%rsp)                       ## 8-byte Folded Reload
	movq	%rdx, %rbp
	movq	%rax, -112(%rsp)                ## 8-byte Spill
	movq	%rcx, %rax
	movq	-16(%rsp), %r13                 ## 8-byte Reload
	mulq	%r13
	movq	%rdx, %r14
	movq	%rax, %r9
	movq	%rcx, %rax
	mulq	-48(%rsp)                       ## 8-byte Folded Reload
	movq	%rdx, %rbx
	movq	%rax, %r10
	movq	%rcx, %rax
	movq	-24(%rsp), %r12                 ## 8-byte Reload
	mulq	%r12
	addq	%r14, %rax
	adcq	%r10, %rdx
	adcq	-112(%rsp), %rbx                ## 8-byte Folded Reload
	adcq	-104(%rsp), %rbp                ## 8-byte Folded Reload
	adcq	-96(%rsp), %rdi                 ## 8-byte Folded Reload
	movzbl	-120(%rsp), %r10d               ## 1-byte Folded Reload
	adcq	-80(%rsp), %r10                 ## 8-byte Folded Reload
	addq	%rsi, %r9
	adcq	%r8, %rax
	adcq	%r11, %rdx
	adcq	%r15, %rbx
	adcq	-128(%rsp), %rbp                ## 8-byte Folded Reload
	adcq	-88(%rsp), %rdi                 ## 8-byte Folded Reload
	movq	-32(%rsp), %rcx                 ## 8-byte Reload
	adcq	88(%rcx), %r10
	movq	%rax, %r8
	subq	%r13, %r8
	movq	%rdx, %r9
	sbbq	%r12, %r9
	movq	%rbx, %r11
	sbbq	-48(%rsp), %r11                 ## 8-byte Folded Reload
	movq	%rbp, %r14
	sbbq	-72(%rsp), %r14                 ## 8-byte Folded Reload
	movq	%rdi, %r15
	sbbq	-64(%rsp), %r15                 ## 8-byte Folded Reload
	movq	%r10, %rcx
	sbbq	-40(%rsp), %rcx                 ## 8-byte Folded Reload
	movq	%rcx, %rsi
	sarq	$63, %rsi
	cmovsq	%r10, %rcx
	movq	(%rsp), %rsi                    ## 8-byte Reload
	movq	%rcx, 40(%rsi)
	cmovsq	%rdi, %r15
	movq	%r15, 32(%rsi)
	cmovsq	%rbp, %r14
	movq	%r14, 24(%rsi)
	cmovsq	%rbx, %r11
	movq	%r11, 16(%rsi)
	cmovsq	%rdx, %r9
	movq	%r9, 8(%rsi)
	cmovsq	%rax, %r8
	movq	%r8, (%rsi)
	addq	$8, %rsp
	popq	%rbx
	popq	%r12
	popq	%r13
	popq	%r14
	popq	%r15
	popq	%rbp
	retq
                                        ## -- End function
	.globl	_mcl_fp_addPre6L                ## -- Begin function mcl_fp_addPre6L
	.p2align	4, 0x90
_mcl_fp_addPre6L:                       ## @mcl_fp_addPre6L
## %bb.0:
	movq	40(%rsi), %rax
	movq	32(%rsi), %rcx
	movq	24(%rsi), %r8
	movq	16(%rsi), %r9
	movq	(%rsi), %r10
	movq	8(%rsi), %rsi
	addq	(%rdx), %r10
	adcq	8(%rdx), %rsi
	adcq	16(%rdx), %r9
	adcq	24(%rdx), %r8
	adcq	32(%rdx), %rcx
	adcq	40(%rdx), %rax
	movq	%rax, 40(%rdi)
	movq	%rcx, 32(%rdi)
	movq	%r8, 24(%rdi)
	movq	%r9, 16(%rdi)
	movq	%rsi, 8(%rdi)
	movq	%r10, (%rdi)
	setb	%al
	movzbl	%al, %eax
	retq
                                        ## -- End function
	.globl	_mcl_fp_subPre6L                ## -- Begin function mcl_fp_subPre6L
	.p2align	4, 0x90
_mcl_fp_subPre6L:                       ## @mcl_fp_subPre6L
## %bb.0:
	movq	40(%rsi), %rcx
	movq	32(%rsi), %r8
	movq	24(%rsi), %r9
	movq	16(%rsi), %r10
	movq	(%rsi), %r11
	movq	8(%rsi), %rsi
	xorl	%eax, %eax
	subq	(%rdx), %r11
	sbbq	8(%rdx), %rsi
	sbbq	16(%rdx), %r10
	sbbq	24(%rdx), %r9
	sbbq	32(%rdx), %r8
	sbbq	40(%rdx), %rcx
	movq	%rcx, 40(%rdi)
	movq	%r8, 32(%rdi)
	movq	%r9, 24(%rdi)
	movq	%r10, 16(%rdi)
	movq	%rsi, 8(%rdi)
	movq	%r11, (%rdi)
	sbbq	%rax, %rax
	andl	$1, %eax
	retq
                                        ## -- End function
	.globl	_mcl_fp_shr1_6L                 ## -- Begin function mcl_fp_shr1_6L
	.p2align	4, 0x90
_mcl_fp_shr1_6L:                        ## @mcl_fp_shr1_6L
## %bb.0:
	movq	(%rsi), %r9
	movq	8(%rsi), %r8
	movq	16(%rsi), %r10
	movq	24(%rsi), %rcx
	movq	32(%rsi), %rax
	movq	40(%rsi), %rdx
	movq	%rdx, %rsi
	shrq	%rsi
	movq	%rsi, 40(%rdi)
	shldq	$63, %rax, %rdx
	movq	%rdx, 32(%rdi)
	shldq	$63, %rcx, %rax
	movq	%rax, 24(%rdi)
	shldq	$63, %r10, %rcx
	movq	%rcx, 16(%rdi)
	shldq	$63, %r8, %r10
	movq	%r10, 8(%rdi)
	shrdq	$1, %r8, %r9
	movq	%r9, (%rdi)
	retq
                                        ## -- End function
	.globl	_mcl_fp_add6L                   ## -- Begin function mcl_fp_add6L
	.p2align	4, 0x90
_mcl_fp_add6L:                          ## @mcl_fp_add6L
## %bb.0:
	movq	40(%rsi), %r8
	movq	32(%rsi), %r9
	movq	24(%rsi), %r10
	movq	16(%rsi), %r11
	movq	(%rsi), %rax
	movq	8(%rsi), %rsi
	addq	(%rdx), %rax
	adcq	8(%rdx), %rsi
	adcq	16(%rdx), %r11
	adcq	24(%rdx), %r10
	adcq	32(%rdx), %r9
	adcq	40(%rdx), %r8
	movq	%r8, 40(%rdi)
	movq	%r9, 32(%rdi)
	movq	%r10, 24(%rdi)
	movq	%r11, 16(%rdi)
	movq	%rsi, 8(%rdi)
	movq	%rax, (%rdi)
	setb	%dl
	movzbl	%dl, %edx
	subq	(%rcx), %rax
	sbbq	8(%rcx), %rsi
	sbbq	16(%rcx), %r11
	sbbq	24(%rcx), %r10
	sbbq	32(%rcx), %r9
	sbbq	40(%rcx), %r8
	sbbq	$0, %rdx
	testb	$1, %dl
	jne	LBB50_2
## %bb.1:                               ## %nocarry
	movq	%rax, (%rdi)
	movq	%rsi, 8(%rdi)
	movq	%r11, 16(%rdi)
	movq	%r10, 24(%rdi)
	movq	%r9, 32(%rdi)
	movq	%r8, 40(%rdi)
LBB50_2:                                ## %carry
	retq
                                        ## -- End function
	.globl	_mcl_fp_addNF6L                 ## -- Begin function mcl_fp_addNF6L
	.p2align	4, 0x90
_mcl_fp_addNF6L:                        ## @mcl_fp_addNF6L
## %bb.0:
	pushq	%r15
	pushq	%r14
	pushq	%r13
	pushq	%r12
	pushq	%rbx
	movq	40(%rdx), %r15
	movq	32(%rdx), %r11
	movq	24(%rdx), %r10
	movq	16(%rdx), %r9
	movq	(%rdx), %r8
	movq	8(%rdx), %r14
	addq	(%rsi), %r8
	adcq	8(%rsi), %r14
	adcq	16(%rsi), %r9
	adcq	24(%rsi), %r10
	adcq	32(%rsi), %r11
	adcq	40(%rsi), %r15
	movq	%r8, %r12
	subq	(%rcx), %r12
	movq	%r14, %r13
	sbbq	8(%rcx), %r13
	movq	%r9, %rdx
	sbbq	16(%rcx), %rdx
	movq	%r10, %rax
	sbbq	24(%rcx), %rax
	movq	%r11, %rsi
	sbbq	32(%rcx), %rsi
	movq	%r15, %rbx
	sbbq	40(%rcx), %rbx
	movq	%rbx, %rcx
	sarq	$63, %rcx
	cmovsq	%r15, %rbx
	movq	%rbx, 40(%rdi)
	cmovsq	%r11, %rsi
	movq	%rsi, 32(%rdi)
	cmovsq	%r10, %rax
	movq	%rax, 24(%rdi)
	cmovsq	%r9, %rdx
	movq	%rdx, 16(%rdi)
	cmovsq	%r14, %r13
	movq	%r13, 8(%rdi)
	cmovsq	%r8, %r12
	movq	%r12, (%rdi)
	popq	%rbx
	popq	%r12
	popq	%r13
	popq	%r14
	popq	%r15
	retq
                                        ## -- End function
	.globl	_mcl_fp_sub6L                   ## -- Begin function mcl_fp_sub6L
	.p2align	4, 0x90
_mcl_fp_sub6L:                          ## @mcl_fp_sub6L
## %bb.0:
	pushq	%rbx
	movq	40(%rsi), %r11
	movq	32(%rsi), %r10
	movq	24(%rsi), %r9
	movq	16(%rsi), %rax
	movq	(%rsi), %r8
	movq	8(%rsi), %rsi
	xorl	%ebx, %ebx
	subq	(%rdx), %r8
	sbbq	8(%rdx), %rsi
	sbbq	16(%rdx), %rax
	sbbq	24(%rdx), %r9
	sbbq	32(%rdx), %r10
	sbbq	40(%rdx), %r11
	movq	%r11, 40(%rdi)
	movq	%r10, 32(%rdi)
	movq	%r9, 24(%rdi)
	movq	%rax, 16(%rdi)
	movq	%rsi, 8(%rdi)
	movq	%r8, (%rdi)
	sbbq	%rbx, %rbx
	testb	$1, %bl
	jne	LBB52_2
## %bb.1:                               ## %nocarry
	popq	%rbx
	retq
LBB52_2:                                ## %carry
	addq	(%rcx), %r8
	adcq	8(%rcx), %rsi
	adcq	16(%rcx), %rax
	adcq	24(%rcx), %r9
	adcq	32(%rcx), %r10
	adcq	40(%rcx), %r11
	movq	%r11, 40(%rdi)
	movq	%r10, 32(%rdi)
	movq	%r9, 24(%rdi)
	movq	%rax, 16(%rdi)
	movq	%rsi, 8(%rdi)
	movq	%r8, (%rdi)
	popq	%rbx
	retq
                                        ## -- End function
	.globl	_mcl_fp_subNF6L                 ## -- Begin function mcl_fp_subNF6L
	.p2align	4, 0x90
_mcl_fp_subNF6L:                        ## @mcl_fp_subNF6L
## %bb.0:
	pushq	%r15
	pushq	%r14
	pushq	%r13
	pushq	%r12
	pushq	%rbx
	movq	40(%rsi), %r15
	movq	32(%rsi), %r8
	movq	24(%rsi), %r9
	movq	16(%rsi), %r10
	movq	(%rsi), %r11
	movq	8(%rsi), %r14
	subq	(%rdx), %r11
	sbbq	8(%rdx), %r14
	sbbq	16(%rdx), %r10
	sbbq	24(%rdx), %r9
	sbbq	32(%rdx), %r8
	sbbq	40(%rdx), %r15
	movq	%r15, %rdx
	sarq	$63, %rdx
	movq	%rdx, %rbx
	shldq	$1, %r15, %rbx
	andq	(%rcx), %rbx
	movq	40(%rcx), %r12
	andq	%rdx, %r12
	movq	32(%rcx), %r13
	andq	%rdx, %r13
	movq	24(%rcx), %rsi
	andq	%rdx, %rsi
	movq	16(%rcx), %rax
	andq	%rdx, %rax
	andq	8(%rcx), %rdx
	addq	%r11, %rbx
	movq	%rbx, (%rdi)
	adcq	%r14, %rdx
	movq	%rdx, 8(%rdi)
	adcq	%r10, %rax
	movq	%rax, 16(%rdi)
	adcq	%r9, %rsi
	movq	%rsi, 24(%rdi)
	adcq	%r8, %r13
	movq	%r13, 32(%rdi)
	adcq	%r15, %r12
	movq	%r12, 40(%rdi)
	popq	%rbx
	popq	%r12
	popq	%r13
	popq	%r14
	popq	%r15
	retq
                                        ## -- End function
	.globl	_mcl_fpDbl_add6L                ## -- Begin function mcl_fpDbl_add6L
	.p2align	4, 0x90
_mcl_fpDbl_add6L:                       ## @mcl_fpDbl_add6L
## %bb.0:
	pushq	%rbp
	pushq	%r15
	pushq	%r14
	pushq	%r13
	pushq	%r12
	pushq	%rbx
	movq	88(%rsi), %r15
	movq	80(%rsi), %r14
	movq	72(%rsi), %r11
	movq	64(%rsi), %r10
	movq	56(%rsi), %r9
	movq	48(%rsi), %r8
	movq	40(%rsi), %rax
	movq	(%rsi), %r12
	movq	8(%rsi), %r13
	addq	(%rdx), %r12
	adcq	8(%rdx), %r13
	movq	32(%rsi), %rbx
	movq	24(%rsi), %rbp
	movq	16(%rsi), %rsi
	adcq	16(%rdx), %rsi
	adcq	24(%rdx), %rbp
	adcq	32(%rdx), %rbx
	adcq	40(%rdx), %rax
	adcq	48(%rdx), %r8
	adcq	56(%rdx), %r9
	adcq	64(%rdx), %r10
	adcq	72(%rdx), %r11
	adcq	80(%rdx), %r14
	adcq	88(%rdx), %r15
	movq	%rax, 40(%rdi)
	movq	%rbx, 32(%rdi)
	movq	%rbp, 24(%rdi)
	movq	%rsi, 16(%rdi)
	movq	%r13, 8(%rdi)
	movq	%r12, (%rdi)
	setb	%al
	movzbl	%al, %r12d
	movq	%r8, %r13
	subq	(%rcx), %r13
	movq	%r9, %rsi
	sbbq	8(%rcx), %rsi
	movq	%r10, %rbx
	sbbq	16(%rcx), %rbx
	movq	%r11, %rbp
	sbbq	24(%rcx), %rbp
	movq	%r14, %rax
	sbbq	32(%rcx), %rax
	movq	%r15, %rdx
	sbbq	40(%rcx), %rdx
	sbbq	$0, %r12
	testb	$1, %r12b
	cmovneq	%r15, %rdx
	movq	%rdx, 88(%rdi)
	cmovneq	%r14, %rax
	movq	%rax, 80(%rdi)
	cmovneq	%r11, %rbp
	movq	%rbp, 72(%rdi)
	cmovneq	%r10, %rbx
	movq	%rbx, 64(%rdi)
	cmovneq	%r9, %rsi
	movq	%rsi, 56(%rdi)
	cmovneq	%r8, %r13
	movq	%r13, 48(%rdi)
	popq	%rbx
	popq	%r12
	popq	%r13
	popq	%r14
	popq	%r15
	popq	%rbp
	retq
                                        ## -- End function
	.globl	_mcl_fpDbl_sub6L                ## -- Begin function mcl_fpDbl_sub6L
	.p2align	4, 0x90
_mcl_fpDbl_sub6L:                       ## @mcl_fpDbl_sub6L
## %bb.0:
	pushq	%rbp
	pushq	%r15
	pushq	%r14
	pushq	%r13
	pushq	%r12
	pushq	%rbx
	movq	%rcx, %r10
	movq	88(%rsi), %r15
	movq	80(%rsi), %r14
	movq	72(%rsi), %r11
	movq	64(%rsi), %r9
	movq	56(%rsi), %r8
	movq	48(%rsi), %rax
	movq	%rax, -16(%rsp)                 ## 8-byte Spill
	movq	(%rsi), %rcx
	movq	8(%rsi), %r13
	xorl	%eax, %eax
	subq	(%rdx), %rcx
	movq	%rcx, -8(%rsp)                  ## 8-byte Spill
	sbbq	8(%rdx), %r13
	movq	40(%rsi), %rbx
	movq	32(%rsi), %rbp
	movq	24(%rsi), %rcx
	movq	16(%rsi), %rsi
	sbbq	16(%rdx), %rsi
	sbbq	24(%rdx), %rcx
	sbbq	32(%rdx), %rbp
	sbbq	40(%rdx), %rbx
	movq	-16(%rsp), %r12                 ## 8-byte Reload
	sbbq	48(%rdx), %r12
	movq	%r12, -16(%rsp)                 ## 8-byte Spill
	sbbq	56(%rdx), %r8
	sbbq	64(%rdx), %r9
	sbbq	72(%rdx), %r11
	sbbq	80(%rdx), %r14
	sbbq	88(%rdx), %r15
	movq	%rbx, 40(%rdi)
	movq	%rbp, 32(%rdi)
	movq	%rcx, 24(%rdi)
	movq	%rsi, 16(%rdi)
	movq	%r13, 8(%rdi)
	movq	-8(%rsp), %rcx                  ## 8-byte Reload
	movq	%rcx, (%rdi)
	sbbq	%rax, %rax
	andl	$1, %eax
	negq	%rax
	movq	40(%r10), %rcx
	andq	%rax, %rcx
	movq	32(%r10), %rdx
	andq	%rax, %rdx
	movq	24(%r10), %rsi
	andq	%rax, %rsi
	movq	16(%r10), %rbx
	andq	%rax, %rbx
	movq	8(%r10), %rbp
	andq	%rax, %rbp
	andq	(%r10), %rax
	addq	-16(%rsp), %rax                 ## 8-byte Folded Reload
	movq	%rax, 48(%rdi)
	adcq	%r8, %rbp
	movq	%rbp, 56(%rdi)
	adcq	%r9, %rbx
	movq	%rbx, 64(%rdi)
	adcq	%r11, %rsi
	movq	%rsi, 72(%rdi)
	adcq	%r14, %rdx
	movq	%rdx, 80(%rdi)
	adcq	%r15, %rcx
	movq	%rcx, 88(%rdi)
	popq	%rbx
	popq	%r12
	popq	%r13
	popq	%r14
	popq	%r15
	popq	%rbp
	retq
                                        ## -- End function
	.globl	_mulPv512x64                    ## -- Begin function mulPv512x64
	.p2align	4, 0x90
_mulPv512x64:                           ## @mulPv512x64
## %bb.0:
	pushq	%rbp
	pushq	%r15
	pushq	%r14
	pushq	%r13
	pushq	%r12
	pushq	%rbx
	movq	%rdx, %rcx
	movq	%rdx, %rax
	mulq	(%rsi)
	movq	%rdx, -24(%rsp)                 ## 8-byte Spill
	movq	%rax, (%rdi)
	movq	%rcx, %rax
	mulq	56(%rsi)
	movq	%rdx, %r10
	movq	%rax, -8(%rsp)                  ## 8-byte Spill
	movq	%rcx, %rax
	mulq	48(%rsi)
	movq	%rdx, %r11
	movq	%rax, -16(%rsp)                 ## 8-byte Spill
	movq	%rcx, %rax
	mulq	40(%rsi)
	movq	%rdx, %r12
	movq	%rax, %r15
	movq	%rcx, %rax
	mulq	32(%rsi)
	movq	%rdx, %rbx
	movq	%rax, %r13
	movq	%rcx, %rax
	mulq	24(%rsi)
	movq	%rdx, %rbp
	movq	%rax, %r8
	movq	%rcx, %rax
	mulq	16(%rsi)
	movq	%rdx, %r9
	movq	%rax, %r14
	movq	%rcx, %rax
	mulq	8(%rsi)
	addq	-24(%rsp), %rax                 ## 8-byte Folded Reload
	movq	%rax, 8(%rdi)
	adcq	%r14, %rdx
	movq	%rdx, 16(%rdi)
	adcq	%r8, %r9
	movq	%r9, 24(%rdi)
	adcq	%r13, %rbp
	movq	%rbp, 32(%rdi)
	adcq	%r15, %rbx
	movq	%rbx, 40(%rdi)
	adcq	-16(%rsp), %r12                 ## 8-byte Folded Reload
	movq	%r12, 48(%rdi)
	adcq	-8(%rsp), %r11                  ## 8-byte Folded Reload
	movq	%r11, 56(%rdi)
	adcq	$0, %r10
	movq	%r10, 64(%rdi)
	movq	%rdi, %rax
	popq	%rbx
	popq	%r12
	popq	%r13
	popq	%r14
	popq	%r15
	popq	%rbp
	retq
                                        ## -- End function
	.globl	_mcl_fp_mulUnitPre8L            ## -- Begin function mcl_fp_mulUnitPre8L
	.p2align	4, 0x90
_mcl_fp_mulUnitPre8L:                   ## @mcl_fp_mulUnitPre8L
## %bb.0:
	pushq	%rbx
	subq	$80, %rsp
	movq	%rdi, %rbx
	leaq	8(%rsp), %rdi
	callq	_mulPv512x64
	movq	8(%rsp), %r8
	movq	16(%rsp), %r9
	movq	24(%rsp), %r10
	movq	32(%rsp), %r11
	movq	40(%rsp), %rdi
	movq	48(%rsp), %rax
	movq	56(%rsp), %rcx
	movq	64(%rsp), %rdx
	movq	72(%rsp), %rsi
	movq	%rsi, 64(%rbx)
	movq	%rdx, 56(%rbx)
	movq	%rcx, 48(%rbx)
	movq	%rax, 40(%rbx)
	movq	%rdi, 32(%rbx)
	movq	%r11, 24(%rbx)
	movq	%r10, 16(%rbx)
	movq	%r9, 8(%rbx)
	movq	%r8, (%rbx)
	addq	$80, %rsp
	popq	%rbx
	retq
                                        ## -- End function
	.globl	_mcl_fpDbl_mulPre8L             ## -- Begin function mcl_fpDbl_mulPre8L
	.p2align	4, 0x90
_mcl_fpDbl_mulPre8L:                    ## @mcl_fpDbl_mulPre8L
## %bb.0:
	pushq	%rbp
	pushq	%r15
	pushq	%r14
	pushq	%r13
	pushq	%r12
	pushq	%rbx
	subq	$648, %rsp                      ## imm = 0x288
	movq	%rdx, %rax
	movq	%rdi, 32(%rsp)                  ## 8-byte Spill
	movq	(%rdx), %rdx
	movq	%rax, %r12
	movq	%rax, 40(%rsp)                  ## 8-byte Spill
	leaq	576(%rsp), %rdi
	movq	%rsi, %r15
	callq	_mulPv512x64
	movq	640(%rsp), %rax
	movq	%rax, (%rsp)                    ## 8-byte Spill
	movq	632(%rsp), %rax
	movq	%rax, 8(%rsp)                   ## 8-byte Spill
	movq	624(%rsp), %rax
	movq	%rax, 16(%rsp)                  ## 8-byte Spill
	movq	616(%rsp), %r13
	movq	608(%rsp), %rax
	movq	%rax, 48(%rsp)                  ## 8-byte Spill
	movq	600(%rsp), %rbp
	movq	592(%rsp), %rbx
	movq	576(%rsp), %rax
	movq	584(%rsp), %r14
	movq	32(%rsp), %rcx                  ## 8-byte Reload
	movq	%rax, (%rcx)
	movq	8(%r12), %rdx
	leaq	504(%rsp), %rdi
	movq	%r15, %rsi
	movq	%r15, 56(%rsp)                  ## 8-byte Spill
	callq	_mulPv512x64
	movq	568(%rsp), %r12
	addq	504(%rsp), %r14
	adcq	512(%rsp), %rbx
	movq	%rbx, 24(%rsp)                  ## 8-byte Spill
	adcq	520(%rsp), %rbp
	movq	%rbp, 64(%rsp)                  ## 8-byte Spill
	movq	48(%rsp), %rax                  ## 8-byte Reload
	adcq	528(%rsp), %rax
	movq	%rax, 48(%rsp)                  ## 8-byte Spill
	adcq	536(%rsp), %r13
	movq	16(%rsp), %rbp                  ## 8-byte Reload
	adcq	544(%rsp), %rbp
	movq	8(%rsp), %rax                   ## 8-byte Reload
	adcq	552(%rsp), %rax
	movq	%rax, 8(%rsp)                   ## 8-byte Spill
	movq	(%rsp), %rax                    ## 8-byte Reload
	adcq	560(%rsp), %rax
	movq	%rax, (%rsp)                    ## 8-byte Spill
	movq	32(%rsp), %rax                  ## 8-byte Reload
	movq	%r14, 8(%rax)
	adcq	$0, %r12
	movq	40(%rsp), %rax                  ## 8-byte Reload
	movq	16(%rax), %rdx
	leaq	432(%rsp), %rdi
	movq	%r15, %rsi
	callq	_mulPv512x64
	movq	496(%rsp), %r15
	movq	24(%rsp), %rcx                  ## 8-byte Reload
	addq	432(%rsp), %rcx
	movq	64(%rsp), %rax                  ## 8-byte Reload
	adcq	440(%rsp), %rax
	movq	%rax, 64(%rsp)                  ## 8-byte Spill
	movq	48(%rsp), %rbx                  ## 8-byte Reload
	adcq	448(%rsp), %rbx
	adcq	456(%rsp), %r13
	movq	%r13, 24(%rsp)                  ## 8-byte Spill
	adcq	464(%rsp), %rbp
	movq	%rbp, 16(%rsp)                  ## 8-byte Spill
	movq	8(%rsp), %rbp                   ## 8-byte Reload
	adcq	472(%rsp), %rbp
	movq	(%rsp), %rax                    ## 8-byte Reload
	adcq	480(%rsp), %rax
	movq	%rax, (%rsp)                    ## 8-byte Spill
	adcq	488(%rsp), %r12
	movq	32(%rsp), %r14                  ## 8-byte Reload
	movq	%rcx, 16(%r14)
	adcq	$0, %r15
	movq	40(%rsp), %rax                  ## 8-byte Reload
	movq	24(%rax), %rdx
	leaq	360(%rsp), %rdi
	movq	56(%rsp), %rsi                  ## 8-byte Reload
	callq	_mulPv512x64
	movq	424(%rsp), %r13
	movq	64(%rsp), %rcx                  ## 8-byte Reload
	addq	360(%rsp), %rcx
	adcq	368(%rsp), %rbx
	movq	%rbx, 48(%rsp)                  ## 8-byte Spill
	movq	24(%rsp), %rax                  ## 8-byte Reload
	adcq	376(%rsp), %rax
	movq	%rax, 24(%rsp)                  ## 8-byte Spill
	movq	16(%rsp), %rax                  ## 8-byte Reload
	adcq	384(%rsp), %rax
	movq	%rax, 16(%rsp)                  ## 8-byte Spill
	adcq	392(%rsp), %rbp
	movq	%rbp, 8(%rsp)                   ## 8-byte Spill
	movq	(%rsp), %rbx                    ## 8-byte Reload
	adcq	400(%rsp), %rbx
	adcq	408(%rsp), %r12
	adcq	416(%rsp), %r15
	movq	%rcx, 24(%r14)
	adcq	$0, %r13
	movq	40(%rsp), %rax                  ## 8-byte Reload
	movq	32(%rax), %rdx
	leaq	288(%rsp), %rdi
	movq	56(%rsp), %rsi                  ## 8-byte Reload
	callq	_mulPv512x64
	movq	352(%rsp), %r14
	movq	48(%rsp), %rcx                  ## 8-byte Reload
	addq	288(%rsp), %rcx
	movq	24(%rsp), %rax                  ## 8-byte Reload
	adcq	296(%rsp), %rax
	movq	%rax, 24(%rsp)                  ## 8-byte Spill
	movq	16(%rsp), %rax                  ## 8-byte Reload
	adcq	304(%rsp), %rax
	movq	%rax, 16(%rsp)                  ## 8-byte Spill
	movq	8(%rsp), %rbp                   ## 8-byte Reload
	adcq	312(%rsp), %rbp
	adcq	320(%rsp), %rbx
	movq	%rbx, (%rsp)                    ## 8-byte Spill
	adcq	328(%rsp), %r12
	adcq	336(%rsp), %r15
	adcq	344(%rsp), %r13
	movq	32(%rsp), %rax                  ## 8-byte Reload
	movq	%rcx, 32(%rax)
	adcq	$0, %r14
	movq	40(%rsp), %rax                  ## 8-byte Reload
	movq	40(%rax), %rdx
	leaq	216(%rsp), %rdi
	movq	56(%rsp), %rsi                  ## 8-byte Reload
	callq	_mulPv512x64
	movq	280(%rsp), %rbx
	movq	24(%rsp), %rax                  ## 8-byte Reload
	addq	216(%rsp), %rax
	movq	16(%rsp), %rcx                  ## 8-byte Reload
	adcq	224(%rsp), %rcx
	movq	%rcx, 16(%rsp)                  ## 8-byte Spill
	adcq	232(%rsp), %rbp
	movq	%rbp, 8(%rsp)                   ## 8-byte Spill
	movq	(%rsp), %rcx                    ## 8-byte Reload
	adcq	240(%rsp), %rcx
	movq	%rcx, (%rsp)                    ## 8-byte Spill
	adcq	248(%rsp), %r12
	adcq	256(%rsp), %r15
	adcq	264(%rsp), %r13
	adcq	272(%rsp), %r14
	movq	32(%rsp), %rcx                  ## 8-byte Reload
	movq	%rax, 40(%rcx)
	adcq	$0, %rbx
	movq	40(%rsp), %rax                  ## 8-byte Reload
	movq	48(%rax), %rdx
	leaq	144(%rsp), %rdi
	movq	56(%rsp), %rsi                  ## 8-byte Reload
	callq	_mulPv512x64
	movq	208(%rsp), %rbp
	movq	16(%rsp), %rax                  ## 8-byte Reload
	addq	144(%rsp), %rax
	movq	8(%rsp), %rcx                   ## 8-byte Reload
	adcq	152(%rsp), %rcx
	movq	%rcx, 8(%rsp)                   ## 8-byte Spill
	movq	(%rsp), %rcx                    ## 8-byte Reload
	adcq	160(%rsp), %rcx
	movq	%rcx, (%rsp)                    ## 8-byte Spill
	adcq	168(%rsp), %r12
	adcq	176(%rsp), %r15
	adcq	184(%rsp), %r13
	adcq	192(%rsp), %r14
	adcq	200(%rsp), %rbx
	movq	32(%rsp), %rcx                  ## 8-byte Reload
	movq	%rax, 48(%rcx)
	adcq	$0, %rbp
	movq	40(%rsp), %rax                  ## 8-byte Reload
	movq	56(%rax), %rdx
	leaq	72(%rsp), %rdi
	movq	56(%rsp), %rsi                  ## 8-byte Reload
	callq	_mulPv512x64
	movq	136(%rsp), %rax
	movq	8(%rsp), %rsi                   ## 8-byte Reload
	addq	72(%rsp), %rsi
	movq	(%rsp), %rdx                    ## 8-byte Reload
	adcq	80(%rsp), %rdx
	adcq	88(%rsp), %r12
	adcq	96(%rsp), %r15
	adcq	104(%rsp), %r13
	adcq	112(%rsp), %r14
	adcq	120(%rsp), %rbx
	adcq	128(%rsp), %rbp
	movq	32(%rsp), %rcx                  ## 8-byte Reload
	movq	%rbp, 112(%rcx)
	movq	%rbx, 104(%rcx)
	movq	%r14, 96(%rcx)
	movq	%r13, 88(%rcx)
	movq	%r15, 80(%rcx)
	movq	%r12, 72(%rcx)
	movq	%rdx, 64(%rcx)
	movq	%rsi, 56(%rcx)
	adcq	$0, %rax
	movq	%rax, 120(%rcx)
	addq	$648, %rsp                      ## imm = 0x288
	popq	%rbx
	popq	%r12
	popq	%r13
	popq	%r14
	popq	%r15
	popq	%rbp
	retq
                                        ## -- End function
	.globl	_mcl_fpDbl_sqrPre8L             ## -- Begin function mcl_fpDbl_sqrPre8L
	.p2align	4, 0x90
_mcl_fpDbl_sqrPre8L:                    ## @mcl_fpDbl_sqrPre8L
## %bb.0:
	pushq	%rbp
	pushq	%r15
	pushq	%r14
	pushq	%r13
	pushq	%r12
	pushq	%rbx
	subq	$648, %rsp                      ## imm = 0x288
	movq	%rsi, %r15
	movq	%rdi, %r12
	movq	%rdi, 56(%rsp)                  ## 8-byte Spill
	movq	(%rsi), %rdx
	leaq	576(%rsp), %rdi
	callq	_mulPv512x64
	movq	640(%rsp), %rax
	movq	%rax, 24(%rsp)                  ## 8-byte Spill
	movq	632(%rsp), %rax
	movq	%rax, 32(%rsp)                  ## 8-byte Spill
	movq	624(%rsp), %rax
	movq	%rax, 48(%rsp)                  ## 8-byte Spill
	movq	616(%rsp), %rax
	movq	%rax, 40(%rsp)                  ## 8-byte Spill
	movq	608(%rsp), %r13
	movq	600(%rsp), %rbp
	movq	592(%rsp), %rbx
	movq	576(%rsp), %rax
	movq	584(%rsp), %r14
	movq	%rax, (%r12)
	movq	8(%r15), %rdx
	leaq	504(%rsp), %rdi
	movq	%r15, %rsi
	callq	_mulPv512x64
	movq	568(%rsp), %rax
	addq	504(%rsp), %r14
	adcq	512(%rsp), %rbx
	movq	%rbx, 8(%rsp)                   ## 8-byte Spill
	adcq	520(%rsp), %rbp
	movq	%rbp, 64(%rsp)                  ## 8-byte Spill
	adcq	528(%rsp), %r13
	movq	%r13, %rbx
	movq	40(%rsp), %r13                  ## 8-byte Reload
	adcq	536(%rsp), %r13
	movq	48(%rsp), %rcx                  ## 8-byte Reload
	adcq	544(%rsp), %rcx
	movq	%rcx, 48(%rsp)                  ## 8-byte Spill
	movq	32(%rsp), %rcx                  ## 8-byte Reload
	adcq	552(%rsp), %rcx
	movq	%rcx, 32(%rsp)                  ## 8-byte Spill
	movq	24(%rsp), %r12                  ## 8-byte Reload
	adcq	560(%rsp), %r12
	movq	56(%rsp), %rcx                  ## 8-byte Reload
	movq	%r14, 8(%rcx)
	adcq	$0, %rax
	movq	%rax, 16(%rsp)                  ## 8-byte Spill
	movq	16(%r15), %rdx
	leaq	432(%rsp), %rdi
	movq	%r15, %rsi
	callq	_mulPv512x64
	movq	496(%rsp), %rax
	movq	8(%rsp), %rdx                   ## 8-byte Reload
	addq	432(%rsp), %rdx
	movq	64(%rsp), %rcx                  ## 8-byte Reload
	adcq	440(%rsp), %rcx
	movq	%rcx, 64(%rsp)                  ## 8-byte Spill
	adcq	448(%rsp), %rbx
	movq	%rbx, 40(%rsp)                  ## 8-byte Spill
	adcq	456(%rsp), %r13
	movq	48(%rsp), %rbx                  ## 8-byte Reload
	adcq	464(%rsp), %rbx
	movq	32(%rsp), %rbp                  ## 8-byte Reload
	adcq	472(%rsp), %rbp
	adcq	480(%rsp), %r12
	movq	%r12, 24(%rsp)                  ## 8-byte Spill
	movq	16(%rsp), %rcx                  ## 8-byte Reload
	adcq	488(%rsp), %rcx
	movq	%rcx, 16(%rsp)                  ## 8-byte Spill
	movq	56(%rsp), %r12                  ## 8-byte Reload
	movq	%rdx, 16(%r12)
	adcq	$0, %rax
	movq	%rax, 8(%rsp)                   ## 8-byte Spill
	movq	24(%r15), %rdx
	leaq	360(%rsp), %rdi
	movq	%r15, %rsi
	callq	_mulPv512x64
	movq	424(%rsp), %r14
	movq	64(%rsp), %rax                  ## 8-byte Reload
	addq	360(%rsp), %rax
	movq	40(%rsp), %rcx                  ## 8-byte Reload
	adcq	368(%rsp), %rcx
	movq	%rcx, 40(%rsp)                  ## 8-byte Spill
	adcq	376(%rsp), %r13
	adcq	384(%rsp), %rbx
	movq	%rbx, 48(%rsp)                  ## 8-byte Spill
	adcq	392(%rsp), %rbp
	movq	%rbp, %rbx
	movq	24(%rsp), %rbp                  ## 8-byte Reload
	adcq	400(%rsp), %rbp
	movq	16(%rsp), %rcx                  ## 8-byte Reload
	adcq	408(%rsp), %rcx
	movq	%rcx, 16(%rsp)                  ## 8-byte Spill
	movq	8(%rsp), %rcx                   ## 8-byte Reload
	adcq	416(%rsp), %rcx
	movq	%rcx, 8(%rsp)                   ## 8-byte Spill
	movq	%rax, 24(%r12)
	adcq	$0, %r14
	movq	32(%r15), %rdx
	leaq	288(%rsp), %rdi
	movq	%r15, %rsi
	callq	_mulPv512x64
	movq	352(%rsp), %r12
	movq	40(%rsp), %rax                  ## 8-byte Reload
	addq	288(%rsp), %rax
	adcq	296(%rsp), %r13
	movq	%r13, 40(%rsp)                  ## 8-byte Spill
	movq	48(%rsp), %r13                  ## 8-byte Reload
	adcq	304(%rsp), %r13
	adcq	312(%rsp), %rbx
	movq	%rbx, 32(%rsp)                  ## 8-byte Spill
	adcq	320(%rsp), %rbp
	movq	%rbp, 24(%rsp)                  ## 8-byte Spill
	movq	16(%rsp), %rbx                  ## 8-byte Reload
	adcq	328(%rsp), %rbx
	movq	8(%rsp), %rcx                   ## 8-byte Reload
	adcq	336(%rsp), %rcx
	movq	%rcx, 8(%rsp)                   ## 8-byte Spill
	adcq	344(%rsp), %r14
	movq	56(%rsp), %rcx                  ## 8-byte Reload
	movq	%rax, 32(%rcx)
	adcq	$0, %r12
	movq	40(%r15), %rdx
	leaq	216(%rsp), %rdi
	movq	%r15, %rsi
	callq	_mulPv512x64
	movq	280(%rsp), %rbp
	movq	40(%rsp), %rax                  ## 8-byte Reload
	addq	216(%rsp), %rax
	adcq	224(%rsp), %r13
	movq	%r13, 48(%rsp)                  ## 8-byte Spill
	movq	32(%rsp), %rcx                  ## 8-byte Reload
	adcq	232(%rsp), %rcx
	movq	%rcx, 32(%rsp)                  ## 8-byte Spill
	movq	24(%rsp), %rcx                  ## 8-byte Reload
	adcq	240(%rsp), %rcx
	movq	%rcx, 24(%rsp)                  ## 8-byte Spill
	adcq	248(%rsp), %rbx
	movq	%rbx, 16(%rsp)                  ## 8-byte Spill
	movq	8(%rsp), %rbx                   ## 8-byte Reload
	adcq	256(%rsp), %rbx
	adcq	264(%rsp), %r14
	adcq	272(%rsp), %r12
	movq	56(%rsp), %rcx                  ## 8-byte Reload
	movq	%rax, 40(%rcx)
	adcq	$0, %rbp
	movq	48(%r15), %rdx
	leaq	144(%rsp), %rdi
	movq	%r15, %rsi
	callq	_mulPv512x64
	movq	208(%rsp), %r13
	movq	48(%rsp), %rcx                  ## 8-byte Reload
	addq	144(%rsp), %rcx
	movq	32(%rsp), %rax                  ## 8-byte Reload
	adcq	152(%rsp), %rax
	movq	%rax, 32(%rsp)                  ## 8-byte Spill
	movq	24(%rsp), %rax                  ## 8-byte Reload
	adcq	160(%rsp), %rax
	movq	%rax, 24(%rsp)                  ## 8-byte Spill
	movq	16(%rsp), %rax                  ## 8-byte Reload
	adcq	168(%rsp), %rax
	movq	%rax, 16(%rsp)                  ## 8-byte Spill
	adcq	176(%rsp), %rbx
	movq	%rbx, 8(%rsp)                   ## 8-byte Spill
	adcq	184(%rsp), %r14
	adcq	192(%rsp), %r12
	adcq	200(%rsp), %rbp
	movq	56(%rsp), %rax                  ## 8-byte Reload
	movq	%rcx, 48(%rax)
	adcq	$0, %r13
	movq	56(%r15), %rdx
	leaq	72(%rsp), %rdi
	movq	%r15, %rsi
	callq	_mulPv512x64
	movq	136(%rsp), %rax
	movq	32(%rsp), %rsi                  ## 8-byte Reload
	addq	72(%rsp), %rsi
	movq	24(%rsp), %rdi                  ## 8-byte Reload
	adcq	80(%rsp), %rdi
	movq	16(%rsp), %rbx                  ## 8-byte Reload
	adcq	88(%rsp), %rbx
	movq	8(%rsp), %rdx                   ## 8-byte Reload
	adcq	96(%rsp), %rdx
	adcq	104(%rsp), %r14
	adcq	112(%rsp), %r12
	adcq	120(%rsp), %rbp
	adcq	128(%rsp), %r13
	movq	56(%rsp), %rcx                  ## 8-byte Reload
	movq	%r13, 112(%rcx)
	movq	%rbp, 104(%rcx)
	movq	%r12, 96(%rcx)
	movq	%r14, 88(%rcx)
	movq	%rdx, 80(%rcx)
	movq	%rbx, 72(%rcx)
	movq	%rdi, 64(%rcx)
	movq	%rsi, 56(%rcx)
	adcq	$0, %rax
	movq	%rax, 120(%rcx)
	addq	$648, %rsp                      ## imm = 0x288
	popq	%rbx
	popq	%r12
	popq	%r13
	popq	%r14
	popq	%r15
	popq	%rbp
	retq
                                        ## -- End function
	.globl	_mcl_fp_mont8L                  ## -- Begin function mcl_fp_mont8L
	.p2align	4, 0x90
_mcl_fp_mont8L:                         ## @mcl_fp_mont8L
## %bb.0:
	pushq	%rbp
	pushq	%r15
	pushq	%r14
	pushq	%r13
	pushq	%r12
	pushq	%rbx
	subq	$1256, %rsp                     ## imm = 0x4E8
	movq	%rcx, %r13
	movq	%rdx, 80(%rsp)                  ## 8-byte Spill
	movq	%rsi, 88(%rsp)                  ## 8-byte Spill
	movq	%rdi, 96(%rsp)                  ## 8-byte Spill
	movq	-8(%rcx), %rbx
	movq	%rbx, 72(%rsp)                  ## 8-byte Spill
	movq	%rcx, 56(%rsp)                  ## 8-byte Spill
	movq	(%rdx), %rdx
	leaq	1184(%rsp), %rdi
	callq	_mulPv512x64
	movq	1184(%rsp), %r15
	movq	1192(%rsp), %r12
	movq	%rbx, %rdx
	imulq	%r15, %rdx
	movq	1248(%rsp), %rax
	movq	%rax, 8(%rsp)                   ## 8-byte Spill
	movq	1240(%rsp), %rax
	movq	%rax, 16(%rsp)                  ## 8-byte Spill
	movq	1232(%rsp), %r14
	movq	1224(%rsp), %rax
	movq	%rax, (%rsp)                    ## 8-byte Spill
	movq	1216(%rsp), %rax
	movq	%rax, 24(%rsp)                  ## 8-byte Spill
	movq	1208(%rsp), %rbx
	movq	1200(%rsp), %rbp
	leaq	1112(%rsp), %rdi
	movq	%r13, %rsi
	callq	_mulPv512x64
	addq	1112(%rsp), %r15
	adcq	1120(%rsp), %r12
	adcq	1128(%rsp), %rbp
	movq	%rbp, 64(%rsp)                  ## 8-byte Spill
	adcq	1136(%rsp), %rbx
	movq	%rbx, 40(%rsp)                  ## 8-byte Spill
	movq	24(%rsp), %rbp                  ## 8-byte Reload
	adcq	1144(%rsp), %rbp
	movq	(%rsp), %r15                    ## 8-byte Reload
	adcq	1152(%rsp), %r15
	adcq	1160(%rsp), %r14
	movq	%r14, 48(%rsp)                  ## 8-byte Spill
	movq	16(%rsp), %r13                  ## 8-byte Reload
	adcq	1168(%rsp), %r13
	movq	8(%rsp), %rbx                   ## 8-byte Reload
	adcq	1176(%rsp), %rbx
	setb	%r14b
	movq	80(%rsp), %rax                  ## 8-byte Reload
	movq	8(%rax), %rdx
	leaq	1040(%rsp), %rdi
	movq	88(%rsp), %rsi                  ## 8-byte Reload
	callq	_mulPv512x64
	movzbl	%r14b, %ecx
	addq	1040(%rsp), %r12
	movq	64(%rsp), %r14                  ## 8-byte Reload
	adcq	1048(%rsp), %r14
	movq	40(%rsp), %rax                  ## 8-byte Reload
	adcq	1056(%rsp), %rax
	movq	%rax, 40(%rsp)                  ## 8-byte Spill
	adcq	1064(%rsp), %rbp
	adcq	1072(%rsp), %r15
	movq	%r15, (%rsp)                    ## 8-byte Spill
	movq	48(%rsp), %rax                  ## 8-byte Reload
	adcq	1080(%rsp), %rax
	movq	%rax, 48(%rsp)                  ## 8-byte Spill
	adcq	1088(%rsp), %r13
	movq	%r13, 16(%rsp)                  ## 8-byte Spill
	adcq	1096(%rsp), %rbx
	movq	%rbx, 8(%rsp)                   ## 8-byte Spill
	adcq	1104(%rsp), %rcx
	movq	%rcx, 32(%rsp)                  ## 8-byte Spill
	setb	%r15b
	movq	72(%rsp), %rdx                  ## 8-byte Reload
	imulq	%r12, %rdx
	leaq	968(%rsp), %rdi
	movq	56(%rsp), %rsi                  ## 8-byte Reload
	callq	_mulPv512x64
	movzbl	%r15b, %r15d
	addq	968(%rsp), %r12
	adcq	976(%rsp), %r14
	movq	%r14, 64(%rsp)                  ## 8-byte Spill
	movq	40(%rsp), %r13                  ## 8-byte Reload
	adcq	984(%rsp), %r13
	adcq	992(%rsp), %rbp
	movq	%rbp, 24(%rsp)                  ## 8-byte Spill
	movq	(%rsp), %r12                    ## 8-byte Reload
	adcq	1000(%rsp), %r12
	movq	48(%rsp), %r14                  ## 8-byte Reload
	adcq	1008(%rsp), %r14
	movq	16(%rsp), %rbx                  ## 8-byte Reload
	adcq	1016(%rsp), %rbx
	movq	8(%rsp), %rax                   ## 8-byte Reload
	adcq	1024(%rsp), %rax
	movq	%rax, 8(%rsp)                   ## 8-byte Spill
	movq	32(%rsp), %rbp                  ## 8-byte Reload
	adcq	1032(%rsp), %rbp
	adcq	$0, %r15
	movq	80(%rsp), %rax                  ## 8-byte Reload
	movq	16(%rax), %rdx
	leaq	896(%rsp), %rdi
	movq	88(%rsp), %rsi                  ## 8-byte Reload
	callq	_mulPv512x64
	movq	64(%rsp), %rax                  ## 8-byte Reload
	addq	896(%rsp), %rax
	adcq	904(%rsp), %r13
	movq	%r13, 40(%rsp)                  ## 8-byte Spill
	movq	24(%rsp), %r13                  ## 8-byte Reload
	adcq	912(%rsp), %r13
	adcq	920(%rsp), %r12
	adcq	928(%rsp), %r14
	movq	%r14, 48(%rsp)                  ## 8-byte Spill
	adcq	936(%rsp), %rbx
	movq	%rbx, 16(%rsp)                  ## 8-byte Spill
	movq	8(%rsp), %rbx                   ## 8-byte Reload
	adcq	944(%rsp), %rbx
	adcq	952(%rsp), %rbp
	movq	%rbp, 32(%rsp)                  ## 8-byte Spill
	adcq	960(%rsp), %r15
	setb	%r14b
	movq	72(%rsp), %rdx                  ## 8-byte Reload
	imulq	%rax, %rdx
	movq	%rax, %rbp
	leaq	824(%rsp), %rdi
	movq	56(%rsp), %rsi                  ## 8-byte Reload
	callq	_mulPv512x64
	movzbl	%r14b, %eax
	addq	824(%rsp), %rbp
	movq	40(%rsp), %r14                  ## 8-byte Reload
	adcq	832(%rsp), %r14
	adcq	840(%rsp), %r13
	movq	%r13, 24(%rsp)                  ## 8-byte Spill
	adcq	848(%rsp), %r12
	movq	%r12, (%rsp)                    ## 8-byte Spill
	movq	48(%rsp), %r12                  ## 8-byte Reload
	adcq	856(%rsp), %r12
	movq	16(%rsp), %rcx                  ## 8-byte Reload
	adcq	864(%rsp), %rcx
	movq	%rcx, 16(%rsp)                  ## 8-byte Spill
	movq	%rbx, %rbp
	adcq	872(%rsp), %rbp
	movq	32(%rsp), %r13                  ## 8-byte Reload
	adcq	880(%rsp), %r13
	adcq	888(%rsp), %r15
	movq	%rax, %rbx
	adcq	$0, %rbx
	movq	80(%rsp), %rax                  ## 8-byte Reload
	movq	24(%rax), %rdx
	leaq	752(%rsp), %rdi
	movq	88(%rsp), %rsi                  ## 8-byte Reload
	callq	_mulPv512x64
	movq	%r14, %rax
	addq	752(%rsp), %rax
	movq	24(%rsp), %rcx                  ## 8-byte Reload
	adcq	760(%rsp), %rcx
	movq	%rcx, 24(%rsp)                  ## 8-byte Spill
	movq	(%rsp), %r14                    ## 8-byte Reload
	adcq	768(%rsp), %r14
	adcq	776(%rsp), %r12
	movq	16(%rsp), %rcx                  ## 8-byte Reload
	adcq	784(%rsp), %rcx
	movq	%rcx, 16(%rsp)                  ## 8-byte Spill
	adcq	792(%rsp), %rbp
	movq	%rbp, 8(%rsp)                   ## 8-byte Spill
	adcq	800(%rsp), %r13
	movq	%r13, 32(%rsp)                  ## 8-byte Spill
	adcq	808(%rsp), %r15
	movq	%r15, %r13
	adcq	816(%rsp), %rbx
	setb	%r15b
	movq	72(%rsp), %rdx                  ## 8-byte Reload
	imulq	%rax, %rdx
	movq	%rax, %rbp
	leaq	680(%rsp), %rdi
	movq	56(%rsp), %rsi                  ## 8-byte Reload
	callq	_mulPv512x64
	movzbl	%r15b, %eax
	addq	680(%rsp), %rbp
	movq	24(%rsp), %rcx                  ## 8-byte Reload
	adcq	688(%rsp), %rcx
	movq	%rcx, 24(%rsp)                  ## 8-byte Spill
	adcq	696(%rsp), %r14
	movq	%r14, (%rsp)                    ## 8-byte Spill
	adcq	704(%rsp), %r12
	movq	16(%rsp), %rbp                  ## 8-byte Reload
	adcq	712(%rsp), %rbp
	movq	8(%rsp), %r14                   ## 8-byte Reload
	adcq	720(%rsp), %r14
	movq	32(%rsp), %r15                  ## 8-byte Reload
	adcq	728(%rsp), %r15
	adcq	736(%rsp), %r13
	movq	%r13, 40(%rsp)                  ## 8-byte Spill
	adcq	744(%rsp), %rbx
	adcq	$0, %rax
	movq	%rax, %r13
	movq	80(%rsp), %rax                  ## 8-byte Reload
	movq	32(%rax), %rdx
	leaq	608(%rsp), %rdi
	movq	88(%rsp), %rsi                  ## 8-byte Reload
	callq	_mulPv512x64
	movq	24(%rsp), %rax                  ## 8-byte Reload
	addq	608(%rsp), %rax
	movq	(%rsp), %rcx                    ## 8-byte Reload
	adcq	616(%rsp), %rcx
	movq	%rcx, (%rsp)                    ## 8-byte Spill
	adcq	624(%rsp), %r12
	adcq	632(%rsp), %rbp
	movq	%rbp, 16(%rsp)                  ## 8-byte Spill
	adcq	640(%rsp), %r14
	movq	%r14, 8(%rsp)                   ## 8-byte Spill
	adcq	648(%rsp), %r15
	movq	%r15, 32(%rsp)                  ## 8-byte Spill
	movq	40(%rsp), %rbp                  ## 8-byte Reload
	adcq	656(%rsp), %rbp
	adcq	664(%rsp), %rbx
	movq	%rbx, %r15
	adcq	672(%rsp), %r13
	setb	%r14b
	movq	72(%rsp), %rdx                  ## 8-byte Reload
	imulq	%rax, %rdx
	movq	%rax, %rbx
	leaq	536(%rsp), %rdi
	movq	56(%rsp), %rsi                  ## 8-byte Reload
	callq	_mulPv512x64
	movzbl	%r14b, %eax
	addq	536(%rsp), %rbx
	movq	(%rsp), %rcx                    ## 8-byte Reload
	adcq	544(%rsp), %rcx
	movq	%rcx, (%rsp)                    ## 8-byte Spill
	adcq	552(%rsp), %r12
	movq	%r12, 48(%rsp)                  ## 8-byte Spill
	movq	16(%rsp), %r12                  ## 8-byte Reload
	adcq	560(%rsp), %r12
	movq	8(%rsp), %rbx                   ## 8-byte Reload
	adcq	568(%rsp), %rbx
	movq	32(%rsp), %r14                  ## 8-byte Reload
	adcq	576(%rsp), %r14
	adcq	584(%rsp), %rbp
	adcq	592(%rsp), %r15
	movq	%r15, 64(%rsp)                  ## 8-byte Spill
	adcq	600(%rsp), %r13
	movq	%r13, 24(%rsp)                  ## 8-byte Spill
	adcq	$0, %rax
	movq	%rax, %r13
	movq	80(%rsp), %rax                  ## 8-byte Reload
	movq	40(%rax), %rdx
	leaq	464(%rsp), %rdi
	movq	88(%rsp), %rsi                  ## 8-byte Reload
	callq	_mulPv512x64
	movq	(%rsp), %rax                    ## 8-byte Reload
	addq	464(%rsp), %rax
	movq	48(%rsp), %r15                  ## 8-byte Reload
	adcq	472(%rsp), %r15
	adcq	480(%rsp), %r12
	movq	%r12, 16(%rsp)                  ## 8-byte Spill
	adcq	488(%rsp), %rbx
	movq	%rbx, 8(%rsp)                   ## 8-byte Spill
	adcq	496(%rsp), %r14
	movq	%r14, %r12
	adcq	504(%rsp), %rbp
	movq	64(%rsp), %rcx                  ## 8-byte Reload
	adcq	512(%rsp), %rcx
	movq	%rcx, 64(%rsp)                  ## 8-byte Spill
	movq	24(%rsp), %rcx                  ## 8-byte Reload
	adcq	520(%rsp), %rcx
	movq	%rcx, 24(%rsp)                  ## 8-byte Spill
	adcq	528(%rsp), %r13
	movq	%r13, (%rsp)                    ## 8-byte Spill
	setb	%r14b
	movq	72(%rsp), %rdx                  ## 8-byte Reload
	imulq	%rax, %rdx
	movq	%rax, %rbx
	leaq	392(%rsp), %rdi
	movq	56(%rsp), %rsi                  ## 8-byte Reload
	callq	_mulPv512x64
	movzbl	%r14b, %eax
	addq	392(%rsp), %rbx
	adcq	400(%rsp), %r15
	movq	%r15, 48(%rsp)                  ## 8-byte Spill
	movq	16(%rsp), %rbx                  ## 8-byte Reload
	adcq	408(%rsp), %rbx
	movq	8(%rsp), %r14                   ## 8-byte Reload
	adcq	416(%rsp), %r14
	adcq	424(%rsp), %r12
	movq	%r12, 32(%rsp)                  ## 8-byte Spill
	adcq	432(%rsp), %rbp
	movq	%rbp, 40(%rsp)                  ## 8-byte Spill
	movq	64(%rsp), %rbp                  ## 8-byte Reload
	adcq	440(%rsp), %rbp
	movq	24(%rsp), %r13                  ## 8-byte Reload
	adcq	448(%rsp), %r13
	movq	(%rsp), %r12                    ## 8-byte Reload
	adcq	456(%rsp), %r12
	movq	%rax, %r15
	adcq	$0, %r15
	movq	80(%rsp), %rax                  ## 8-byte Reload
	movq	48(%rax), %rdx
	leaq	320(%rsp), %rdi
	movq	88(%rsp), %rsi                  ## 8-byte Reload
	callq	_mulPv512x64
	leaq	248(%rsp), %rdi
	movq	48(%rsp), %rax                  ## 8-byte Reload
	addq	320(%rsp), %rax
	adcq	328(%rsp), %rbx
	movq	%rbx, 16(%rsp)                  ## 8-byte Spill
	adcq	336(%rsp), %r14
	movq	%r14, 8(%rsp)                   ## 8-byte Spill
	movq	32(%rsp), %rbx                  ## 8-byte Reload
	adcq	344(%rsp), %rbx
	movq	40(%rsp), %rcx                  ## 8-byte Reload
	adcq	352(%rsp), %rcx
	movq	%rcx, 40(%rsp)                  ## 8-byte Spill
	adcq	360(%rsp), %rbp
	adcq	368(%rsp), %r13
	adcq	376(%rsp), %r12
	movq	%r12, (%rsp)                    ## 8-byte Spill
	adcq	384(%rsp), %r15
	movq	%r15, 48(%rsp)                  ## 8-byte Spill
	setb	%r12b
	movq	72(%rsp), %rdx                  ## 8-byte Reload
	imulq	%rax, %rdx
	movq	%rax, %r14
	movq	56(%rsp), %rsi                  ## 8-byte Reload
	callq	_mulPv512x64
	movzbl	%r12b, %r12d
	addq	248(%rsp), %r14
	movq	16(%rsp), %rax                  ## 8-byte Reload
	adcq	256(%rsp), %rax
	movq	%rax, 16(%rsp)                  ## 8-byte Spill
	movq	8(%rsp), %r15                   ## 8-byte Reload
	adcq	264(%rsp), %r15
	adcq	272(%rsp), %rbx
	movq	%rbx, 32(%rsp)                  ## 8-byte Spill
	movq	40(%rsp), %rbx                  ## 8-byte Reload
	adcq	280(%rsp), %rbx
	adcq	288(%rsp), %rbp
	adcq	296(%rsp), %r13
	movq	(%rsp), %r14                    ## 8-byte Reload
	adcq	304(%rsp), %r14
	movq	48(%rsp), %rax                  ## 8-byte Reload
	adcq	312(%rsp), %rax
	movq	%rax, 48(%rsp)                  ## 8-byte Spill
	adcq	$0, %r12
	movq	80(%rsp), %rax                  ## 8-byte Reload
	movq	56(%rax), %rdx
	leaq	176(%rsp), %rdi
	movq	88(%rsp), %rsi                  ## 8-byte Reload
	callq	_mulPv512x64
	movq	16(%rsp), %rax                  ## 8-byte Reload
	addq	176(%rsp), %rax
	adcq	184(%rsp), %r15
	movq	%r15, 8(%rsp)                   ## 8-byte Spill
	movq	32(%rsp), %rcx                  ## 8-byte Reload
	adcq	192(%rsp), %rcx
	movq	%rcx, 32(%rsp)                  ## 8-byte Spill
	adcq	200(%rsp), %rbx
	adcq	208(%rsp), %rbp
	adcq	216(%rsp), %r13
	movq	%r13, 24(%rsp)                  ## 8-byte Spill
	adcq	224(%rsp), %r14
	movq	%r14, (%rsp)                    ## 8-byte Spill
	movq	48(%rsp), %r15                  ## 8-byte Reload
	adcq	232(%rsp), %r15
	adcq	240(%rsp), %r12
	setb	%r14b
	movq	72(%rsp), %rdx                  ## 8-byte Reload
	imulq	%rax, %rdx
	movq	%rax, %r13
	leaq	104(%rsp), %rdi
	movq	56(%rsp), %rsi                  ## 8-byte Reload
	callq	_mulPv512x64
	movzbl	%r14b, %r9d
	addq	104(%rsp), %r13
	movq	8(%rsp), %r11                   ## 8-byte Reload
	adcq	112(%rsp), %r11
	movq	%r11, 8(%rsp)                   ## 8-byte Spill
	movq	32(%rsp), %r10                  ## 8-byte Reload
	adcq	120(%rsp), %r10
	movq	%r10, 32(%rsp)                  ## 8-byte Spill
	movq	%rbx, %r8
	adcq	128(%rsp), %r8
	movq	%r8, 40(%rsp)                   ## 8-byte Spill
	movq	%rbp, %r13
	adcq	136(%rsp), %r13
	movq	24(%rsp), %r14                  ## 8-byte Reload
	adcq	144(%rsp), %r14
	movq	(%rsp), %rsi                    ## 8-byte Reload
	adcq	152(%rsp), %rsi
	adcq	160(%rsp), %r15
	adcq	168(%rsp), %r12
	adcq	$0, %r9
	movq	56(%rsp), %rcx                  ## 8-byte Reload
	subq	(%rcx), %r11
	sbbq	8(%rcx), %r10
	sbbq	16(%rcx), %r8
	movq	%r13, %rdi
	sbbq	24(%rcx), %rdi
	movq	%r14, %rbx
	sbbq	32(%rcx), %rbx
	movq	%rsi, %rbp
	sbbq	40(%rcx), %rbp
	movq	%r15, %rax
	sbbq	48(%rcx), %rax
	movq	%rcx, %rdx
	movq	%r12, %rcx
	sbbq	56(%rdx), %rcx
	sbbq	$0, %r9
	testb	$1, %r9b
	cmovneq	%r12, %rcx
	movq	96(%rsp), %rdx                  ## 8-byte Reload
	movq	%rcx, 56(%rdx)
	cmovneq	%r15, %rax
	movq	%rax, 48(%rdx)
	cmovneq	%rsi, %rbp
	movq	%rbp, 40(%rdx)
	cmovneq	%r14, %rbx
	movq	%rbx, 32(%rdx)
	cmovneq	%r13, %rdi
	movq	%rdi, 24(%rdx)
	cmovneq	40(%rsp), %r8                   ## 8-byte Folded Reload
	movq	%r8, 16(%rdx)
	cmovneq	32(%rsp), %r10                  ## 8-byte Folded Reload
	movq	%r10, 8(%rdx)
	cmovneq	8(%rsp), %r11                   ## 8-byte Folded Reload
	movq	%r11, (%rdx)
	addq	$1256, %rsp                     ## imm = 0x4E8
	popq	%rbx
	popq	%r12
	popq	%r13
	popq	%r14
	popq	%r15
	popq	%rbp
	retq
                                        ## -- End function
	.globl	_mcl_fp_montNF8L                ## -- Begin function mcl_fp_montNF8L
	.p2align	4, 0x90
_mcl_fp_montNF8L:                       ## @mcl_fp_montNF8L
## %bb.0:
	pushq	%rbp
	pushq	%r15
	pushq	%r14
	pushq	%r13
	pushq	%r12
	pushq	%rbx
	subq	$1256, %rsp                     ## imm = 0x4E8
	movq	%rcx, %rbp
	movq	%rdx, 88(%rsp)                  ## 8-byte Spill
	movq	%rsi, 80(%rsp)                  ## 8-byte Spill
	movq	%rdi, 96(%rsp)                  ## 8-byte Spill
	movq	-8(%rcx), %rbx
	movq	%rbx, 64(%rsp)                  ## 8-byte Spill
	movq	%rcx, 72(%rsp)                  ## 8-byte Spill
	movq	(%rdx), %rdx
	leaq	1184(%rsp), %rdi
	callq	_mulPv512x64
	movq	1184(%rsp), %r15
	movq	1192(%rsp), %r12
	movq	%rbx, %rdx
	imulq	%r15, %rdx
	movq	1248(%rsp), %rax
	movq	%rax, 8(%rsp)                   ## 8-byte Spill
	movq	1240(%rsp), %rax
	movq	%rax, 24(%rsp)                  ## 8-byte Spill
	movq	1232(%rsp), %rax
	movq	%rax, 32(%rsp)                  ## 8-byte Spill
	movq	1224(%rsp), %rax
	movq	%rax, 16(%rsp)                  ## 8-byte Spill
	movq	1216(%rsp), %r14
	movq	1208(%rsp), %rbx
	movq	1200(%rsp), %r13
	leaq	1112(%rsp), %rdi
	movq	%rbp, %rsi
	callq	_mulPv512x64
	addq	1112(%rsp), %r15
	adcq	1120(%rsp), %r12
	adcq	1128(%rsp), %r13
	adcq	1136(%rsp), %rbx
	movq	%rbx, 40(%rsp)                  ## 8-byte Spill
	movq	%r14, %r15
	adcq	1144(%rsp), %r15
	movq	16(%rsp), %rbx                  ## 8-byte Reload
	adcq	1152(%rsp), %rbx
	movq	32(%rsp), %r14                  ## 8-byte Reload
	adcq	1160(%rsp), %r14
	movq	24(%rsp), %rbp                  ## 8-byte Reload
	adcq	1168(%rsp), %rbp
	movq	8(%rsp), %rax                   ## 8-byte Reload
	adcq	1176(%rsp), %rax
	movq	%rax, 8(%rsp)                   ## 8-byte Spill
	movq	88(%rsp), %rax                  ## 8-byte Reload
	movq	8(%rax), %rdx
	leaq	1040(%rsp), %rdi
	movq	80(%rsp), %rsi                  ## 8-byte Reload
	callq	_mulPv512x64
	movq	1104(%rsp), %rcx
	addq	1040(%rsp), %r12
	adcq	1048(%rsp), %r13
	movq	40(%rsp), %rax                  ## 8-byte Reload
	adcq	1056(%rsp), %rax
	movq	%rax, 40(%rsp)                  ## 8-byte Spill
	adcq	1064(%rsp), %r15
	adcq	1072(%rsp), %rbx
	adcq	1080(%rsp), %r14
	movq	%r14, 32(%rsp)                  ## 8-byte Spill
	adcq	1088(%rsp), %rbp
	movq	%rbp, 24(%rsp)                  ## 8-byte Spill
	movq	8(%rsp), %r14                   ## 8-byte Reload
	adcq	1096(%rsp), %r14
	adcq	$0, %rcx
	movq	%rcx, 48(%rsp)                  ## 8-byte Spill
	movq	64(%rsp), %rdx                  ## 8-byte Reload
	imulq	%r12, %rdx
	leaq	968(%rsp), %rdi
	movq	72(%rsp), %rsi                  ## 8-byte Reload
	callq	_mulPv512x64
	addq	968(%rsp), %r12
	adcq	976(%rsp), %r13
	movq	40(%rsp), %rbp                  ## 8-byte Reload
	adcq	984(%rsp), %rbp
	adcq	992(%rsp), %r15
	movq	%r15, 56(%rsp)                  ## 8-byte Spill
	adcq	1000(%rsp), %rbx
	movq	%rbx, 16(%rsp)                  ## 8-byte Spill
	movq	32(%rsp), %r15                  ## 8-byte Reload
	adcq	1008(%rsp), %r15
	movq	24(%rsp), %rax                  ## 8-byte Reload
	adcq	1016(%rsp), %rax
	movq	%rax, 24(%rsp)                  ## 8-byte Spill
	adcq	1024(%rsp), %r14
	movq	%r14, 8(%rsp)                   ## 8-byte Spill
	movq	48(%rsp), %rbx                  ## 8-byte Reload
	adcq	1032(%rsp), %rbx
	movq	88(%rsp), %rax                  ## 8-byte Reload
	movq	16(%rax), %rdx
	leaq	896(%rsp), %rdi
	movq	80(%rsp), %rsi                  ## 8-byte Reload
	callq	_mulPv512x64
	movq	960(%rsp), %r12
	addq	896(%rsp), %r13
	movq	%rbp, %r14
	adcq	904(%rsp), %r14
	movq	56(%rsp), %rax                  ## 8-byte Reload
	adcq	912(%rsp), %rax
	movq	%rax, 56(%rsp)                  ## 8-byte Spill
	movq	16(%rsp), %rax                  ## 8-byte Reload
	adcq	920(%rsp), %rax
	movq	%rax, 16(%rsp)                  ## 8-byte Spill
	adcq	928(%rsp), %r15
	movq	%r15, 32(%rsp)                  ## 8-byte Spill
	movq	24(%rsp), %rbp                  ## 8-byte Reload
	adcq	936(%rsp), %rbp
	movq	8(%rsp), %rax                   ## 8-byte Reload
	adcq	944(%rsp), %rax
	movq	%rax, 8(%rsp)                   ## 8-byte Spill
	adcq	952(%rsp), %rbx
	adcq	$0, %r12
	movq	64(%rsp), %rdx                  ## 8-byte Reload
	imulq	%r13, %rdx
	leaq	824(%rsp), %rdi
	movq	72(%rsp), %rsi                  ## 8-byte Reload
	callq	_mulPv512x64
	addq	824(%rsp), %r13
	adcq	832(%rsp), %r14
	movq	%r14, 40(%rsp)                  ## 8-byte Spill
	movq	56(%rsp), %r13                  ## 8-byte Reload
	adcq	840(%rsp), %r13
	movq	16(%rsp), %r15                  ## 8-byte Reload
	adcq	848(%rsp), %r15
	movq	32(%rsp), %r14                  ## 8-byte Reload
	adcq	856(%rsp), %r14
	adcq	864(%rsp), %rbp
	movq	%rbp, 24(%rsp)                  ## 8-byte Spill
	movq	8(%rsp), %rbp                   ## 8-byte Reload
	adcq	872(%rsp), %rbp
	adcq	880(%rsp), %rbx
	adcq	888(%rsp), %r12
	movq	88(%rsp), %rax                  ## 8-byte Reload
	movq	24(%rax), %rdx
	leaq	752(%rsp), %rdi
	movq	80(%rsp), %rsi                  ## 8-byte Reload
	callq	_mulPv512x64
	movq	816(%rsp), %rcx
	movq	40(%rsp), %rax                  ## 8-byte Reload
	addq	752(%rsp), %rax
	adcq	760(%rsp), %r13
	adcq	768(%rsp), %r15
	movq	%r15, 16(%rsp)                  ## 8-byte Spill
	movq	%r14, %r15
	adcq	776(%rsp), %r15
	movq	24(%rsp), %rdx                  ## 8-byte Reload
	adcq	784(%rsp), %rdx
	movq	%rdx, 24(%rsp)                  ## 8-byte Spill
	adcq	792(%rsp), %rbp
	movq	%rbp, 8(%rsp)                   ## 8-byte Spill
	adcq	800(%rsp), %rbx
	adcq	808(%rsp), %r12
	adcq	$0, %rcx
	movq	%rcx, 40(%rsp)                  ## 8-byte Spill
	movq	64(%rsp), %rdx                  ## 8-byte Reload
	imulq	%rax, %rdx
	movq	%rax, %rbp
	leaq	680(%rsp), %rdi
	movq	72(%rsp), %rsi                  ## 8-byte Reload
	callq	_mulPv512x64
	addq	680(%rsp), %rbp
	adcq	688(%rsp), %r13
	movq	16(%rsp), %r14                  ## 8-byte Reload
	adcq	696(%rsp), %r14
	adcq	704(%rsp), %r15
	movq	%r15, 32(%rsp)                  ## 8-byte Spill
	movq	24(%rsp), %r15                  ## 8-byte Reload
	adcq	712(%rsp), %r15
	movq	8(%rsp), %rbp                   ## 8-byte Reload
	adcq	720(%rsp), %rbp
	adcq	728(%rsp), %rbx
	adcq	736(%rsp), %r12
	movq	40(%rsp), %rax                  ## 8-byte Reload
	adcq	744(%rsp), %rax
	movq	%rax, 40(%rsp)                  ## 8-byte Spill
	movq	88(%rsp), %rax                  ## 8-byte Reload
	movq	32(%rax), %rdx
	leaq	608(%rsp), %rdi
	movq	80(%rsp), %rsi                  ## 8-byte Reload
	callq	_mulPv512x64
	movq	672(%rsp), %rcx
	movq	%r13, %rax
	addq	608(%rsp), %rax
	adcq	616(%rsp), %r14
	movq	%r14, 16(%rsp)                  ## 8-byte Spill
	movq	32(%rsp), %r13                  ## 8-byte Reload
	adcq	624(%rsp), %r13
	adcq	632(%rsp), %r15
	movq	%r15, 24(%rsp)                  ## 8-byte Spill
	adcq	640(%rsp), %rbp
	movq	%rbp, 8(%rsp)                   ## 8-byte Spill
	adcq	648(%rsp), %rbx
	movq	%rbx, %r15
	adcq	656(%rsp), %r12
	movq	40(%rsp), %r14                  ## 8-byte Reload
	adcq	664(%rsp), %r14
	movq	%rcx, %rbp
	adcq	$0, %rbp
	movq	64(%rsp), %rdx                  ## 8-byte Reload
	imulq	%rax, %rdx
	movq	%rax, %rbx
	leaq	536(%rsp), %rdi
	movq	72(%rsp), %rsi                  ## 8-byte Reload
	callq	_mulPv512x64
	addq	536(%rsp), %rbx
	movq	16(%rsp), %rax                  ## 8-byte Reload
	adcq	544(%rsp), %rax
	movq	%rax, 16(%rsp)                  ## 8-byte Spill
	movq	%r13, %rbx
	adcq	552(%rsp), %rbx
	movq	24(%rsp), %r13                  ## 8-byte Reload
	adcq	560(%rsp), %r13
	movq	8(%rsp), %rax                   ## 8-byte Reload
	adcq	568(%rsp), %rax
	movq	%rax, 8(%rsp)                   ## 8-byte Spill
	adcq	576(%rsp), %r15
	movq	%r15, 48(%rsp)                  ## 8-byte Spill
	adcq	584(%rsp), %r12
	movq	%r14, %r15
	adcq	592(%rsp), %r15
	adcq	600(%rsp), %rbp
	movq	88(%rsp), %rax                  ## 8-byte Reload
	movq	40(%rax), %rdx
	leaq	464(%rsp), %rdi
	movq	80(%rsp), %rsi                  ## 8-byte Reload
	callq	_mulPv512x64
	movq	528(%rsp), %rcx
	movq	16(%rsp), %rax                  ## 8-byte Reload
	addq	464(%rsp), %rax
	adcq	472(%rsp), %rbx
	movq	%rbx, 32(%rsp)                  ## 8-byte Spill
	adcq	480(%rsp), %r13
	movq	%r13, 24(%rsp)                  ## 8-byte Spill
	movq	8(%rsp), %r14                   ## 8-byte Reload
	adcq	488(%rsp), %r14
	movq	48(%rsp), %r13                  ## 8-byte Reload
	adcq	496(%rsp), %r13
	adcq	504(%rsp), %r12
	movq	%r12, 16(%rsp)                  ## 8-byte Spill
	adcq	512(%rsp), %r15
	movq	%r15, %r12
	adcq	520(%rsp), %rbp
	movq	%rcx, %r15
	adcq	$0, %r15
	movq	64(%rsp), %rdx                  ## 8-byte Reload
	imulq	%rax, %rdx
	movq	%rax, %rbx
	leaq	392(%rsp), %rdi
	movq	72(%rsp), %rsi                  ## 8-byte Reload
	callq	_mulPv512x64
	addq	392(%rsp), %rbx
	movq	32(%rsp), %rax                  ## 8-byte Reload
	adcq	400(%rsp), %rax
	movq	%rax, 32(%rsp)                  ## 8-byte Spill
	movq	24(%rsp), %rax                  ## 8-byte Reload
	adcq	408(%rsp), %rax
	movq	%rax, 24(%rsp)                  ## 8-byte Spill
	movq	%r14, %rbx
	adcq	416(%rsp), %rbx
	adcq	424(%rsp), %r13
	movq	%r13, 48(%rsp)                  ## 8-byte Spill
	movq	16(%rsp), %r14                  ## 8-byte Reload
	adcq	432(%rsp), %r14
	adcq	440(%rsp), %r12
	adcq	448(%rsp), %rbp
	movq	%rbp, 56(%rsp)                  ## 8-byte Spill
	adcq	456(%rsp), %r15
	movq	88(%rsp), %rax                  ## 8-byte Reload
	movq	48(%rax), %rdx
	leaq	320(%rsp), %rdi
	movq	80(%rsp), %rsi                  ## 8-byte Reload
	callq	_mulPv512x64
	leaq	248(%rsp), %rdi
	movq	384(%rsp), %r13
	movq	32(%rsp), %rax                  ## 8-byte Reload
	addq	320(%rsp), %rax
	movq	24(%rsp), %rbp                  ## 8-byte Reload
	adcq	328(%rsp), %rbp
	adcq	336(%rsp), %rbx
	movq	%rbx, 8(%rsp)                   ## 8-byte Spill
	movq	48(%rsp), %rcx                  ## 8-byte Reload
	adcq	344(%rsp), %rcx
	movq	%rcx, 48(%rsp)                  ## 8-byte Spill
	adcq	352(%rsp), %r14
	movq	%r14, 16(%rsp)                  ## 8-byte Spill
	adcq	360(%rsp), %r12
	movq	%r12, 40(%rsp)                  ## 8-byte Spill
	movq	56(%rsp), %r12                  ## 8-byte Reload
	adcq	368(%rsp), %r12
	adcq	376(%rsp), %r15
	movq	%r15, 32(%rsp)                  ## 8-byte Spill
	adcq	$0, %r13
	movq	64(%rsp), %rdx                  ## 8-byte Reload
	imulq	%rax, %rdx
	movq	%rax, %rbx
	movq	72(%rsp), %rsi                  ## 8-byte Reload
	callq	_mulPv512x64
	leaq	176(%rsp), %rdi
	addq	248(%rsp), %rbx
	adcq	256(%rsp), %rbp
	movq	%rbp, 24(%rsp)                  ## 8-byte Spill
	movq	8(%rsp), %r14                   ## 8-byte Reload
	adcq	264(%rsp), %r14
	movq	48(%rsp), %rbp                  ## 8-byte Reload
	adcq	272(%rsp), %rbp
	movq	16(%rsp), %r15                  ## 8-byte Reload
	adcq	280(%rsp), %r15
	movq	40(%rsp), %rbx                  ## 8-byte Reload
	adcq	288(%rsp), %rbx
	adcq	296(%rsp), %r12
	movq	%r12, 56(%rsp)                  ## 8-byte Spill
	movq	32(%rsp), %rax                  ## 8-byte Reload
	adcq	304(%rsp), %rax
	movq	%rax, 32(%rsp)                  ## 8-byte Spill
	adcq	312(%rsp), %r13
	movq	88(%rsp), %rax                  ## 8-byte Reload
	movq	56(%rax), %rdx
	movq	80(%rsp), %rsi                  ## 8-byte Reload
	callq	_mulPv512x64
	leaq	104(%rsp), %rdi
	movq	240(%rsp), %r12
	movq	24(%rsp), %rax                  ## 8-byte Reload
	addq	176(%rsp), %rax
	adcq	184(%rsp), %r14
	movq	%r14, 8(%rsp)                   ## 8-byte Spill
	adcq	192(%rsp), %rbp
	movq	%rbp, 48(%rsp)                  ## 8-byte Spill
	adcq	200(%rsp), %r15
	movq	%r15, 16(%rsp)                  ## 8-byte Spill
	adcq	208(%rsp), %rbx
	movq	%rbx, 40(%rsp)                  ## 8-byte Spill
	movq	56(%rsp), %rbp                  ## 8-byte Reload
	adcq	216(%rsp), %rbp
	movq	32(%rsp), %r15                  ## 8-byte Reload
	adcq	224(%rsp), %r15
	adcq	232(%rsp), %r13
	adcq	$0, %r12
	movq	64(%rsp), %rdx                  ## 8-byte Reload
	imulq	%rax, %rdx
	movq	%rax, %rbx
	movq	72(%rsp), %r14                  ## 8-byte Reload
	movq	%r14, %rsi
	callq	_mulPv512x64
	addq	104(%rsp), %rbx
	movq	8(%rsp), %r8                    ## 8-byte Reload
	adcq	112(%rsp), %r8
	movq	%r8, 8(%rsp)                    ## 8-byte Spill
	movq	48(%rsp), %r9                   ## 8-byte Reload
	adcq	120(%rsp), %r9
	movq	%r9, 48(%rsp)                   ## 8-byte Spill
	movq	16(%rsp), %rsi                  ## 8-byte Reload
	adcq	128(%rsp), %rsi
	movq	40(%rsp), %r11                  ## 8-byte Reload
	adcq	136(%rsp), %r11
	movq	%rbp, %r10
	adcq	144(%rsp), %r10
	adcq	152(%rsp), %r15
	adcq	160(%rsp), %r13
	adcq	168(%rsp), %r12
	movq	%r14, %rax
	subq	(%r14), %r8
	sbbq	8(%r14), %r9
	movq	%rsi, %rdx
	movq	%rsi, %r14
	sbbq	16(%rax), %rdx
	movq	%r11, %rsi
	sbbq	24(%rax), %rsi
	movq	%r10, %rdi
	sbbq	32(%rax), %rdi
	movq	%r15, %rbp
	sbbq	40(%rax), %rbp
	movq	%r13, %rbx
	sbbq	48(%rax), %rbx
	movq	%rax, %rcx
	movq	%r12, %rax
	sbbq	56(%rcx), %rax
	cmovsq	%r12, %rax
	movq	96(%rsp), %rcx                  ## 8-byte Reload
	movq	%rax, 56(%rcx)
	cmovsq	%r13, %rbx
	movq	%rbx, 48(%rcx)
	cmovsq	%r15, %rbp
	movq	%rbp, 40(%rcx)
	cmovsq	%r10, %rdi
	movq	%rdi, 32(%rcx)
	cmovsq	%r11, %rsi
	movq	%rsi, 24(%rcx)
	cmovsq	%r14, %rdx
	movq	%rdx, 16(%rcx)
	cmovsq	48(%rsp), %r9                   ## 8-byte Folded Reload
	movq	%r9, 8(%rcx)
	cmovsq	8(%rsp), %r8                    ## 8-byte Folded Reload
	movq	%r8, (%rcx)
	addq	$1256, %rsp                     ## imm = 0x4E8
	popq	%rbx
	popq	%r12
	popq	%r13
	popq	%r14
	popq	%r15
	popq	%rbp
	retq
                                        ## -- End function
	.globl	_mcl_fp_montRed8L               ## -- Begin function mcl_fp_montRed8L
	.p2align	4, 0x90
_mcl_fp_montRed8L:                      ## @mcl_fp_montRed8L
## %bb.0:
	pushq	%rbp
	pushq	%r15
	pushq	%r14
	pushq	%r13
	pushq	%r12
	pushq	%rbx
	subq	$728, %rsp                      ## imm = 0x2D8
	movq	%rdi, 144(%rsp)                 ## 8-byte Spill
	movq	56(%rdx), %rax
	movq	%rax, 136(%rsp)                 ## 8-byte Spill
	movq	48(%rdx), %rax
	movq	%rax, 128(%rsp)                 ## 8-byte Spill
	movq	40(%rdx), %rax
	movq	%rax, 120(%rsp)                 ## 8-byte Spill
	movq	32(%rdx), %rax
	movq	%rax, 112(%rsp)                 ## 8-byte Spill
	movq	24(%rdx), %rax
	movq	%rax, 104(%rsp)                 ## 8-byte Spill
	movq	16(%rdx), %rax
	movq	%rax, 96(%rsp)                  ## 8-byte Spill
	movq	8(%rdx), %rax
	movq	%rax, 88(%rsp)                  ## 8-byte Spill
	movq	%rsi, 72(%rsp)                  ## 8-byte Spill
	movq	56(%rsi), %r12
	movq	48(%rsi), %rax
	movq	%rax, 16(%rsp)                  ## 8-byte Spill
	movq	40(%rsi), %rax
	movq	%rax, 32(%rsp)                  ## 8-byte Spill
	movq	32(%rsi), %r15
	movq	24(%rsi), %r14
	movq	16(%rsi), %r13
	movq	(%rsi), %rbp
	movq	8(%rsi), %rbx
	movq	-8(%rdx), %rcx
	movq	%rcx, 56(%rsp)                  ## 8-byte Spill
	movq	(%rdx), %rax
	movq	%rdx, %rsi
	movq	%rdx, 64(%rsp)                  ## 8-byte Spill
	movq	%rax, 80(%rsp)                  ## 8-byte Spill
	movq	%rbp, %rdx
	imulq	%rcx, %rdx
	leaq	656(%rsp), %rdi
	callq	_mulPv512x64
	addq	656(%rsp), %rbp
	adcq	664(%rsp), %rbx
	adcq	672(%rsp), %r13
	adcq	680(%rsp), %r14
	adcq	688(%rsp), %r15
	movq	32(%rsp), %rbp                  ## 8-byte Reload
	adcq	696(%rsp), %rbp
	movq	16(%rsp), %rax                  ## 8-byte Reload
	adcq	704(%rsp), %rax
	movq	%rax, 16(%rsp)                  ## 8-byte Spill
	adcq	712(%rsp), %r12
	movq	%r12, 24(%rsp)                  ## 8-byte Spill
	movq	72(%rsp), %rax                  ## 8-byte Reload
	movq	64(%rax), %rax
	adcq	720(%rsp), %rax
	movq	%rax, (%rsp)                    ## 8-byte Spill
	setb	%r12b
	movq	56(%rsp), %rdx                  ## 8-byte Reload
	imulq	%rbx, %rdx
	leaq	584(%rsp), %rdi
	movq	64(%rsp), %rsi                  ## 8-byte Reload
	callq	_mulPv512x64
	movq	648(%rsp), %rax
	addb	$255, %r12b
	adcq	$0, %rax
	movq	%rax, %rcx
	addq	584(%rsp), %rbx
	adcq	592(%rsp), %r13
	adcq	600(%rsp), %r14
	adcq	608(%rsp), %r15
	adcq	616(%rsp), %rbp
	movq	%rbp, 32(%rsp)                  ## 8-byte Spill
	movq	16(%rsp), %rax                  ## 8-byte Reload
	adcq	624(%rsp), %rax
	movq	%rax, 16(%rsp)                  ## 8-byte Spill
	movq	24(%rsp), %rbp                  ## 8-byte Reload
	adcq	632(%rsp), %rbp
	movq	(%rsp), %rax                    ## 8-byte Reload
	adcq	640(%rsp), %rax
	movq	%rax, (%rsp)                    ## 8-byte Spill
	movq	72(%rsp), %r12                  ## 8-byte Reload
	adcq	72(%r12), %rcx
	movq	%rcx, 8(%rsp)                   ## 8-byte Spill
	setb	%bl
	movq	56(%rsp), %rdx                  ## 8-byte Reload
	imulq	%r13, %rdx
	leaq	512(%rsp), %rdi
	movq	64(%rsp), %rsi                  ## 8-byte Reload
	callq	_mulPv512x64
	movq	576(%rsp), %rax
	addb	$255, %bl
	adcq	$0, %rax
	movq	%rax, %rcx
	addq	512(%rsp), %r13
	adcq	520(%rsp), %r14
	adcq	528(%rsp), %r15
	movq	32(%rsp), %rax                  ## 8-byte Reload
	adcq	536(%rsp), %rax
	movq	%rax, 32(%rsp)                  ## 8-byte Spill
	movq	16(%rsp), %rax                  ## 8-byte Reload
	adcq	544(%rsp), %rax
	movq	%rax, 16(%rsp)                  ## 8-byte Spill
	adcq	552(%rsp), %rbp
	movq	%rbp, 24(%rsp)                  ## 8-byte Spill
	movq	(%rsp), %rbp                    ## 8-byte Reload
	adcq	560(%rsp), %rbp
	movq	8(%rsp), %rbx                   ## 8-byte Reload
	adcq	568(%rsp), %rbx
	adcq	80(%r12), %rcx
	movq	%rcx, 40(%rsp)                  ## 8-byte Spill
	setb	%r13b
	movq	56(%rsp), %rdx                  ## 8-byte Reload
	imulq	%r14, %rdx
	leaq	440(%rsp), %rdi
	movq	64(%rsp), %r12                  ## 8-byte Reload
	movq	%r12, %rsi
	callq	_mulPv512x64
	movq	504(%rsp), %rax
	addb	$255, %r13b
	adcq	$0, %rax
	addq	440(%rsp), %r14
	adcq	448(%rsp), %r15
	movq	32(%rsp), %rcx                  ## 8-byte Reload
	adcq	456(%rsp), %rcx
	movq	%rcx, 32(%rsp)                  ## 8-byte Spill
	movq	16(%rsp), %r13                  ## 8-byte Reload
	adcq	464(%rsp), %r13
	movq	24(%rsp), %rcx                  ## 8-byte Reload
	adcq	472(%rsp), %rcx
	movq	%rcx, 24(%rsp)                  ## 8-byte Spill
	adcq	480(%rsp), %rbp
	movq	%rbp, (%rsp)                    ## 8-byte Spill
	adcq	488(%rsp), %rbx
	movq	%rbx, 8(%rsp)                   ## 8-byte Spill
	movq	40(%rsp), %rbp                  ## 8-byte Reload
	adcq	496(%rsp), %rbp
	movq	72(%rsp), %rcx                  ## 8-byte Reload
	adcq	88(%rcx), %rax
	movq	%rax, 48(%rsp)                  ## 8-byte Spill
	setb	%bl
	movq	56(%rsp), %rdx                  ## 8-byte Reload
	imulq	%r15, %rdx
	leaq	368(%rsp), %rdi
	movq	%r12, %rsi
	callq	_mulPv512x64
	movq	432(%rsp), %r14
	addb	$255, %bl
	adcq	$0, %r14
	addq	368(%rsp), %r15
	movq	32(%rsp), %rax                  ## 8-byte Reload
	adcq	376(%rsp), %rax
	adcq	384(%rsp), %r13
	movq	%r13, 16(%rsp)                  ## 8-byte Spill
	movq	24(%rsp), %rbx                  ## 8-byte Reload
	adcq	392(%rsp), %rbx
	movq	(%rsp), %rcx                    ## 8-byte Reload
	adcq	400(%rsp), %rcx
	movq	%rcx, (%rsp)                    ## 8-byte Spill
	movq	8(%rsp), %rcx                   ## 8-byte Reload
	adcq	408(%rsp), %rcx
	movq	%rcx, 8(%rsp)                   ## 8-byte Spill
	adcq	416(%rsp), %rbp
	movq	%rbp, 40(%rsp)                  ## 8-byte Spill
	movq	48(%rsp), %rcx                  ## 8-byte Reload
	adcq	424(%rsp), %rcx
	movq	%rcx, 48(%rsp)                  ## 8-byte Spill
	movq	72(%rsp), %rcx                  ## 8-byte Reload
	adcq	96(%rcx), %r14
	setb	%r15b
	movq	56(%rsp), %r13                  ## 8-byte Reload
	movq	%r13, %rdx
	imulq	%rax, %rdx
	movq	%rax, %rbp
	leaq	296(%rsp), %rdi
	movq	%r12, %rsi
	callq	_mulPv512x64
	movq	360(%rsp), %r12
	addb	$255, %r15b
	adcq	$0, %r12
	addq	296(%rsp), %rbp
	movq	16(%rsp), %rax                  ## 8-byte Reload
	adcq	304(%rsp), %rax
	adcq	312(%rsp), %rbx
	movq	%rbx, 24(%rsp)                  ## 8-byte Spill
	movq	(%rsp), %rcx                    ## 8-byte Reload
	adcq	320(%rsp), %rcx
	movq	%rcx, (%rsp)                    ## 8-byte Spill
	movq	8(%rsp), %rcx                   ## 8-byte Reload
	adcq	328(%rsp), %rcx
	movq	%rcx, 8(%rsp)                   ## 8-byte Spill
	movq	40(%rsp), %rcx                  ## 8-byte Reload
	adcq	336(%rsp), %rcx
	movq	%rcx, 40(%rsp)                  ## 8-byte Spill
	movq	48(%rsp), %rcx                  ## 8-byte Reload
	adcq	344(%rsp), %rcx
	movq	%rcx, 48(%rsp)                  ## 8-byte Spill
	adcq	352(%rsp), %r14
	movq	72(%rsp), %rbp                  ## 8-byte Reload
	adcq	104(%rbp), %r12
	setb	%r15b
	movq	%r13, %rdx
	imulq	%rax, %rdx
	movq	%rax, %rbx
	leaq	224(%rsp), %rdi
	movq	64(%rsp), %rsi                  ## 8-byte Reload
	callq	_mulPv512x64
	movq	288(%rsp), %r13
	addb	$255, %r15b
	adcq	$0, %r13
	addq	224(%rsp), %rbx
	movq	24(%rsp), %rax                  ## 8-byte Reload
	adcq	232(%rsp), %rax
	movq	(%rsp), %rcx                    ## 8-byte Reload
	adcq	240(%rsp), %rcx
	movq	%rcx, (%rsp)                    ## 8-byte Spill
	movq	8(%rsp), %rcx                   ## 8-byte Reload
	adcq	248(%rsp), %rcx
	movq	%rcx, 8(%rsp)                   ## 8-byte Spill
	movq	40(%rsp), %rcx                  ## 8-byte Reload
	adcq	256(%rsp), %rcx
	movq	%rcx, 40(%rsp)                  ## 8-byte Spill
	movq	48(%rsp), %rcx                  ## 8-byte Reload
	adcq	264(%rsp), %rcx
	movq	%rcx, 48(%rsp)                  ## 8-byte Spill
	adcq	272(%rsp), %r14
	adcq	280(%rsp), %r12
	adcq	112(%rbp), %r13
	setb	%r15b
	movq	56(%rsp), %rdx                  ## 8-byte Reload
	imulq	%rax, %rdx
	movq	%rax, %rbx
	leaq	152(%rsp), %rdi
	movq	64(%rsp), %rsi                  ## 8-byte Reload
	callq	_mulPv512x64
	addb	$255, %r15b
	movq	216(%rsp), %rdx
	adcq	$0, %rdx
	addq	152(%rsp), %rbx
	movq	(%rsp), %r9                     ## 8-byte Reload
	adcq	160(%rsp), %r9
	movq	%r9, (%rsp)                     ## 8-byte Spill
	movq	8(%rsp), %r10                   ## 8-byte Reload
	adcq	168(%rsp), %r10
	movq	%r10, 8(%rsp)                   ## 8-byte Spill
	movq	40(%rsp), %r15                  ## 8-byte Reload
	adcq	176(%rsp), %r15
	movq	48(%rsp), %r11                  ## 8-byte Reload
	adcq	184(%rsp), %r11
	adcq	192(%rsp), %r14
	adcq	200(%rsp), %r12
	adcq	208(%rsp), %r13
	adcq	120(%rbp), %rdx
	xorl	%r8d, %r8d
	subq	80(%rsp), %r9                   ## 8-byte Folded Reload
	sbbq	88(%rsp), %r10                  ## 8-byte Folded Reload
	movq	%r15, %rdi
	sbbq	96(%rsp), %rdi                  ## 8-byte Folded Reload
	movq	%r11, %rbp
	sbbq	104(%rsp), %rbp                 ## 8-byte Folded Reload
	movq	%r14, %rbx
	sbbq	112(%rsp), %rbx                 ## 8-byte Folded Reload
	movq	%r12, %rsi
	sbbq	120(%rsp), %rsi                 ## 8-byte Folded Reload
	movq	%r13, %rax
	sbbq	128(%rsp), %rax                 ## 8-byte Folded Reload
	movq	%rdx, %rcx
	sbbq	136(%rsp), %rcx                 ## 8-byte Folded Reload
	sbbq	%r8, %r8
	testb	$1, %r8b
	cmovneq	%rdx, %rcx
	movq	144(%rsp), %rdx                 ## 8-byte Reload
	movq	%rcx, 56(%rdx)
	cmovneq	%r13, %rax
	movq	%rax, 48(%rdx)
	cmovneq	%r12, %rsi
	movq	%rsi, 40(%rdx)
	cmovneq	%r14, %rbx
	movq	%rbx, 32(%rdx)
	cmovneq	%r11, %rbp
	movq	%rbp, 24(%rdx)
	cmovneq	%r15, %rdi
	movq	%rdi, 16(%rdx)
	cmovneq	8(%rsp), %r10                   ## 8-byte Folded Reload
	movq	%r10, 8(%rdx)
	cmovneq	(%rsp), %r9                     ## 8-byte Folded Reload
	movq	%r9, (%rdx)
	addq	$728, %rsp                      ## imm = 0x2D8
	popq	%rbx
	popq	%r12
	popq	%r13
	popq	%r14
	popq	%r15
	popq	%rbp
	retq
                                        ## -- End function
	.globl	_mcl_fp_montRedNF8L             ## -- Begin function mcl_fp_montRedNF8L
	.p2align	4, 0x90
_mcl_fp_montRedNF8L:                    ## @mcl_fp_montRedNF8L
## %bb.0:
	pushq	%rbp
	pushq	%r15
	pushq	%r14
	pushq	%r13
	pushq	%r12
	pushq	%rbx
	subq	$728, %rsp                      ## imm = 0x2D8
	movq	%rdi, 144(%rsp)                 ## 8-byte Spill
	movq	56(%rdx), %rax
	movq	%rax, 136(%rsp)                 ## 8-byte Spill
	movq	48(%rdx), %rax
	movq	%rax, 128(%rsp)                 ## 8-byte Spill
	movq	40(%rdx), %rax
	movq	%rax, 120(%rsp)                 ## 8-byte Spill
	movq	32(%rdx), %rax
	movq	%rax, 112(%rsp)                 ## 8-byte Spill
	movq	24(%rdx), %rax
	movq	%rax, 104(%rsp)                 ## 8-byte Spill
	movq	16(%rdx), %rax
	movq	%rax, 96(%rsp)                  ## 8-byte Spill
	movq	8(%rdx), %rax
	movq	%rax, 88(%rsp)                  ## 8-byte Spill
	movq	%rsi, 72(%rsp)                  ## 8-byte Spill
	movq	56(%rsi), %rax
	movq	%rax, 8(%rsp)                   ## 8-byte Spill
	movq	48(%rsi), %rax
	movq	%rax, 24(%rsp)                  ## 8-byte Spill
	movq	40(%rsi), %r12
	movq	32(%rsi), %r13
	movq	24(%rsi), %r15
	movq	16(%rsi), %r14
	movq	(%rsi), %rbx
	movq	8(%rsi), %rbp
	movq	-8(%rdx), %rcx
	movq	%rcx, 56(%rsp)                  ## 8-byte Spill
	movq	(%rdx), %rax
	movq	%rdx, %rsi
	movq	%rdx, 64(%rsp)                  ## 8-byte Spill
	movq	%rax, 80(%rsp)                  ## 8-byte Spill
	movq	%rbx, %rdx
	imulq	%rcx, %rdx
	leaq	656(%rsp), %rdi
	callq	_mulPv512x64
	addq	656(%rsp), %rbx
	adcq	664(%rsp), %rbp
	adcq	672(%rsp), %r14
	adcq	680(%rsp), %r15
	adcq	688(%rsp), %r13
	adcq	696(%rsp), %r12
	movq	%r12, 48(%rsp)                  ## 8-byte Spill
	movq	24(%rsp), %rax                  ## 8-byte Reload
	adcq	704(%rsp), %rax
	movq	%rax, 24(%rsp)                  ## 8-byte Spill
	movq	8(%rsp), %rbx                   ## 8-byte Reload
	adcq	712(%rsp), %rbx
	movq	72(%rsp), %rax                  ## 8-byte Reload
	movq	64(%rax), %rax
	adcq	720(%rsp), %rax
	movq	%rax, (%rsp)                    ## 8-byte Spill
	setb	%r12b
	movq	56(%rsp), %rdx                  ## 8-byte Reload
	imulq	%rbp, %rdx
	leaq	584(%rsp), %rdi
	movq	64(%rsp), %rsi                  ## 8-byte Reload
	callq	_mulPv512x64
	movq	648(%rsp), %rax
	addb	$255, %r12b
	adcq	$0, %rax
	movq	%rax, %rcx
	addq	584(%rsp), %rbp
	adcq	592(%rsp), %r14
	adcq	600(%rsp), %r15
	adcq	608(%rsp), %r13
	movq	48(%rsp), %rax                  ## 8-byte Reload
	adcq	616(%rsp), %rax
	movq	%rax, 48(%rsp)                  ## 8-byte Spill
	movq	24(%rsp), %rax                  ## 8-byte Reload
	adcq	624(%rsp), %rax
	movq	%rax, 24(%rsp)                  ## 8-byte Spill
	adcq	632(%rsp), %rbx
	movq	%rbx, 8(%rsp)                   ## 8-byte Spill
	movq	(%rsp), %rax                    ## 8-byte Reload
	adcq	640(%rsp), %rax
	movq	%rax, (%rsp)                    ## 8-byte Spill
	movq	72(%rsp), %rax                  ## 8-byte Reload
	adcq	72(%rax), %rcx
	movq	%rcx, 16(%rsp)                  ## 8-byte Spill
	setb	%bl
	movq	56(%rsp), %rbp                  ## 8-byte Reload
	movq	%rbp, %rdx
	imulq	%r14, %rdx
	leaq	512(%rsp), %rdi
	movq	64(%rsp), %r12                  ## 8-byte Reload
	movq	%r12, %rsi
	callq	_mulPv512x64
	movq	576(%rsp), %rax
	addb	$255, %bl
	adcq	$0, %rax
	addq	512(%rsp), %r14
	adcq	520(%rsp), %r15
	adcq	528(%rsp), %r13
	movq	48(%rsp), %rcx                  ## 8-byte Reload
	adcq	536(%rsp), %rcx
	movq	%rcx, 48(%rsp)                  ## 8-byte Spill
	movq	24(%rsp), %rcx                  ## 8-byte Reload
	adcq	544(%rsp), %rcx
	movq	%rcx, 24(%rsp)                  ## 8-byte Spill
	movq	8(%rsp), %rcx                   ## 8-byte Reload
	adcq	552(%rsp), %rcx
	movq	%rcx, 8(%rsp)                   ## 8-byte Spill
	movq	(%rsp), %rcx                    ## 8-byte Reload
	adcq	560(%rsp), %rcx
	movq	%rcx, (%rsp)                    ## 8-byte Spill
	movq	16(%rsp), %rcx                  ## 8-byte Reload
	adcq	568(%rsp), %rcx
	movq	%rcx, 16(%rsp)                  ## 8-byte Spill
	movq	%rax, %r14
	movq	72(%rsp), %rax                  ## 8-byte Reload
	adcq	80(%rax), %r14
	setb	%bl
	movq	%rbp, %rdx
	imulq	%r15, %rdx
	leaq	440(%rsp), %rdi
	movq	%r12, %rsi
	callq	_mulPv512x64
	movq	504(%rsp), %rax
	addb	$255, %bl
	adcq	$0, %rax
	addq	440(%rsp), %r15
	adcq	448(%rsp), %r13
	movq	48(%rsp), %rcx                  ## 8-byte Reload
	adcq	456(%rsp), %rcx
	movq	%rcx, 48(%rsp)                  ## 8-byte Spill
	movq	24(%rsp), %rbx                  ## 8-byte Reload
	adcq	464(%rsp), %rbx
	movq	8(%rsp), %rbp                   ## 8-byte Reload
	adcq	472(%rsp), %rbp
	movq	(%rsp), %rcx                    ## 8-byte Reload
	adcq	480(%rsp), %rcx
	movq	%rcx, (%rsp)                    ## 8-byte Spill
	movq	16(%rsp), %rcx                  ## 8-byte Reload
	adcq	488(%rsp), %rcx
	movq	%rcx, 16(%rsp)                  ## 8-byte Spill
	adcq	496(%rsp), %r14
	movq	%r14, 40(%rsp)                  ## 8-byte Spill
	movq	72(%rsp), %r14                  ## 8-byte Reload
	adcq	88(%r14), %rax
	movq	%rax, 32(%rsp)                  ## 8-byte Spill
	setb	%r12b
	movq	56(%rsp), %rdx                  ## 8-byte Reload
	imulq	%r13, %rdx
	leaq	368(%rsp), %rdi
	movq	64(%rsp), %rsi                  ## 8-byte Reload
	callq	_mulPv512x64
	movq	432(%rsp), %r15
	addb	$255, %r12b
	adcq	$0, %r15
	addq	368(%rsp), %r13
	movq	48(%rsp), %r13                  ## 8-byte Reload
	adcq	376(%rsp), %r13
	adcq	384(%rsp), %rbx
	movq	%rbx, 24(%rsp)                  ## 8-byte Spill
	adcq	392(%rsp), %rbp
	movq	%rbp, 8(%rsp)                   ## 8-byte Spill
	movq	(%rsp), %rbx                    ## 8-byte Reload
	adcq	400(%rsp), %rbx
	movq	16(%rsp), %rbp                  ## 8-byte Reload
	adcq	408(%rsp), %rbp
	movq	40(%rsp), %rcx                  ## 8-byte Reload
	adcq	416(%rsp), %rcx
	movq	%rcx, 40(%rsp)                  ## 8-byte Spill
	movq	32(%rsp), %rcx                  ## 8-byte Reload
	adcq	424(%rsp), %rcx
	movq	%rcx, 32(%rsp)                  ## 8-byte Spill
	adcq	96(%r14), %r15
	setb	%r14b
	movq	56(%rsp), %rdx                  ## 8-byte Reload
	imulq	%r13, %rdx
	leaq	296(%rsp), %rdi
	movq	64(%rsp), %rsi                  ## 8-byte Reload
	callq	_mulPv512x64
	movq	360(%rsp), %r12
	addb	$255, %r14b
	adcq	$0, %r12
	addq	296(%rsp), %r13
	movq	24(%rsp), %rax                  ## 8-byte Reload
	adcq	304(%rsp), %rax
	movq	8(%rsp), %rcx                   ## 8-byte Reload
	adcq	312(%rsp), %rcx
	movq	%rcx, 8(%rsp)                   ## 8-byte Spill
	adcq	320(%rsp), %rbx
	movq	%rbx, (%rsp)                    ## 8-byte Spill
	adcq	328(%rsp), %rbp
	movq	%rbp, 16(%rsp)                  ## 8-byte Spill
	movq	40(%rsp), %rcx                  ## 8-byte Reload
	adcq	336(%rsp), %rcx
	movq	%rcx, 40(%rsp)                  ## 8-byte Spill
	movq	32(%rsp), %rcx                  ## 8-byte Reload
	adcq	344(%rsp), %rcx
	movq	%rcx, 32(%rsp)                  ## 8-byte Spill
	adcq	352(%rsp), %r15
	movq	72(%rsp), %rbx                  ## 8-byte Reload
	adcq	104(%rbx), %r12
	setb	%r13b
	movq	56(%rsp), %rdx                  ## 8-byte Reload
	imulq	%rax, %rdx
	movq	%rax, %rbp
	leaq	224(%rsp), %rdi
	movq	64(%rsp), %rsi                  ## 8-byte Reload
	callq	_mulPv512x64
	movq	288(%rsp), %r14
	addb	$255, %r13b
	adcq	$0, %r14
	addq	224(%rsp), %rbp
	movq	8(%rsp), %rax                   ## 8-byte Reload
	adcq	232(%rsp), %rax
	movq	(%rsp), %rcx                    ## 8-byte Reload
	adcq	240(%rsp), %rcx
	movq	%rcx, (%rsp)                    ## 8-byte Spill
	movq	16(%rsp), %rcx                  ## 8-byte Reload
	adcq	248(%rsp), %rcx
	movq	%rcx, 16(%rsp)                  ## 8-byte Spill
	movq	40(%rsp), %rcx                  ## 8-byte Reload
	adcq	256(%rsp), %rcx
	movq	%rcx, 40(%rsp)                  ## 8-byte Spill
	movq	32(%rsp), %rcx                  ## 8-byte Reload
	adcq	264(%rsp), %rcx
	movq	%rcx, 32(%rsp)                  ## 8-byte Spill
	adcq	272(%rsp), %r15
	adcq	280(%rsp), %r12
	adcq	112(%rbx), %r14
	setb	%r13b
	movq	56(%rsp), %rdx                  ## 8-byte Reload
	imulq	%rax, %rdx
	movq	%rax, %rbp
	leaq	152(%rsp), %rdi
	movq	64(%rsp), %rsi                  ## 8-byte Reload
	callq	_mulPv512x64
	addb	$255, %r13b
	movq	216(%rsp), %rdx
	adcq	$0, %rdx
	addq	152(%rsp), %rbp
	movq	(%rsp), %r8                     ## 8-byte Reload
	adcq	160(%rsp), %r8
	movq	%r8, (%rsp)                     ## 8-byte Spill
	movq	16(%rsp), %rcx                  ## 8-byte Reload
	adcq	168(%rsp), %rcx
	movq	40(%rsp), %rdi                  ## 8-byte Reload
	adcq	176(%rsp), %rdi
	movq	32(%rsp), %r10                  ## 8-byte Reload
	adcq	184(%rsp), %r10
	adcq	192(%rsp), %r15
	adcq	200(%rsp), %r12
	adcq	208(%rsp), %r14
	adcq	120(%rbx), %rdx
	subq	80(%rsp), %r8                   ## 8-byte Folded Reload
	movq	%rcx, %r9
	movq	%rcx, %r11
	sbbq	88(%rsp), %r9                   ## 8-byte Folded Reload
	movq	%rdi, %rsi
	movq	%rdi, %r13
	sbbq	96(%rsp), %rsi                  ## 8-byte Folded Reload
	movq	%r10, %rdi
	sbbq	104(%rsp), %rdi                 ## 8-byte Folded Reload
	movq	%r15, %rbx
	sbbq	112(%rsp), %rbx                 ## 8-byte Folded Reload
	movq	%r12, %rbp
	sbbq	120(%rsp), %rbp                 ## 8-byte Folded Reload
	movq	%r14, %rax
	sbbq	128(%rsp), %rax                 ## 8-byte Folded Reload
	movq	%rdx, %rcx
	sbbq	136(%rsp), %rcx                 ## 8-byte Folded Reload
	cmovsq	%rdx, %rcx
	movq	144(%rsp), %rdx                 ## 8-byte Reload
	movq	%rcx, 56(%rdx)
	cmovsq	%r14, %rax
	movq	%rax, 48(%rdx)
	cmovsq	%r12, %rbp
	movq	%rbp, 40(%rdx)
	cmovsq	%r15, %rbx
	movq	%rbx, 32(%rdx)
	cmovsq	%r10, %rdi
	movq	%rdi, 24(%rdx)
	cmovsq	%r13, %rsi
	movq	%rsi, 16(%rdx)
	cmovsq	%r11, %r9
	movq	%r9, 8(%rdx)
	cmovsq	(%rsp), %r8                     ## 8-byte Folded Reload
	movq	%r8, (%rdx)
	addq	$728, %rsp                      ## imm = 0x2D8
	popq	%rbx
	popq	%r12
	popq	%r13
	popq	%r14
	popq	%r15
	popq	%rbp
	retq
                                        ## -- End function
	.globl	_mcl_fp_addPre8L                ## -- Begin function mcl_fp_addPre8L
	.p2align	4, 0x90
_mcl_fp_addPre8L:                       ## @mcl_fp_addPre8L
## %bb.0:
	pushq	%rbx
	movq	56(%rsi), %rax
	movq	48(%rsi), %rcx
	movq	40(%rsi), %r8
	movq	32(%rsi), %r9
	movq	24(%rsi), %r10
	movq	16(%rsi), %r11
	movq	(%rsi), %rbx
	movq	8(%rsi), %rsi
	addq	(%rdx), %rbx
	adcq	8(%rdx), %rsi
	adcq	16(%rdx), %r11
	adcq	24(%rdx), %r10
	adcq	32(%rdx), %r9
	adcq	40(%rdx), %r8
	adcq	48(%rdx), %rcx
	adcq	56(%rdx), %rax
	movq	%rax, 56(%rdi)
	movq	%rcx, 48(%rdi)
	movq	%r8, 40(%rdi)
	movq	%r9, 32(%rdi)
	movq	%r10, 24(%rdi)
	movq	%r11, 16(%rdi)
	movq	%rsi, 8(%rdi)
	movq	%rbx, (%rdi)
	setb	%al
	movzbl	%al, %eax
	popq	%rbx
	retq
                                        ## -- End function
	.globl	_mcl_fp_subPre8L                ## -- Begin function mcl_fp_subPre8L
	.p2align	4, 0x90
_mcl_fp_subPre8L:                       ## @mcl_fp_subPre8L
## %bb.0:
	pushq	%r14
	pushq	%rbx
	movq	56(%rsi), %rcx
	movq	48(%rsi), %r8
	movq	40(%rsi), %r9
	movq	32(%rsi), %r10
	movq	24(%rsi), %r11
	movq	16(%rsi), %rbx
	movq	(%rsi), %r14
	movq	8(%rsi), %rsi
	xorl	%eax, %eax
	subq	(%rdx), %r14
	sbbq	8(%rdx), %rsi
	sbbq	16(%rdx), %rbx
	sbbq	24(%rdx), %r11
	sbbq	32(%rdx), %r10
	sbbq	40(%rdx), %r9
	sbbq	48(%rdx), %r8
	sbbq	56(%rdx), %rcx
	movq	%rcx, 56(%rdi)
	movq	%r8, 48(%rdi)
	movq	%r9, 40(%rdi)
	movq	%r10, 32(%rdi)
	movq	%r11, 24(%rdi)
	movq	%rbx, 16(%rdi)
	movq	%rsi, 8(%rdi)
	movq	%r14, (%rdi)
	sbbq	%rax, %rax
	andl	$1, %eax
	popq	%rbx
	popq	%r14
	retq
                                        ## -- End function
	.globl	_mcl_fp_shr1_8L                 ## -- Begin function mcl_fp_shr1_8L
	.p2align	4, 0x90
_mcl_fp_shr1_8L:                        ## @mcl_fp_shr1_8L
## %bb.0:
	pushq	%rbx
	movq	(%rsi), %r9
	movq	8(%rsi), %r8
	movq	16(%rsi), %r10
	movq	24(%rsi), %r11
	movq	32(%rsi), %rax
	movq	40(%rsi), %rdx
	movq	48(%rsi), %rcx
	movq	56(%rsi), %rsi
	movq	%rsi, %rbx
	shrq	%rbx
	movq	%rbx, 56(%rdi)
	shldq	$63, %rcx, %rsi
	movq	%rsi, 48(%rdi)
	shldq	$63, %rdx, %rcx
	movq	%rcx, 40(%rdi)
	shldq	$63, %rax, %rdx
	movq	%rdx, 32(%rdi)
	shldq	$63, %r11, %rax
	movq	%rax, 24(%rdi)
	shldq	$63, %r10, %r11
	movq	%r11, 16(%rdi)
	shldq	$63, %r8, %r10
	movq	%r10, 8(%rdi)
	shrdq	$1, %r8, %r9
	movq	%r9, (%rdi)
	popq	%rbx
	retq
                                        ## -- End function
	.globl	_mcl_fp_add8L                   ## -- Begin function mcl_fp_add8L
	.p2align	4, 0x90
_mcl_fp_add8L:                          ## @mcl_fp_add8L
## %bb.0:
	pushq	%r14
	pushq	%rbx
	movq	56(%rsi), %r8
	movq	48(%rsi), %r9
	movq	40(%rsi), %r10
	movq	32(%rsi), %r11
	movq	24(%rsi), %r14
	movq	16(%rsi), %rbx
	movq	(%rsi), %rax
	movq	8(%rsi), %rsi
	addq	(%rdx), %rax
	adcq	8(%rdx), %rsi
	adcq	16(%rdx), %rbx
	adcq	24(%rdx), %r14
	adcq	32(%rdx), %r11
	adcq	40(%rdx), %r10
	adcq	48(%rdx), %r9
	adcq	56(%rdx), %r8
	movq	%r8, 56(%rdi)
	movq	%r9, 48(%rdi)
	movq	%r10, 40(%rdi)
	movq	%r11, 32(%rdi)
	movq	%r14, 24(%rdi)
	movq	%rbx, 16(%rdi)
	movq	%rsi, 8(%rdi)
	movq	%rax, (%rdi)
	setb	%dl
	movzbl	%dl, %edx
	subq	(%rcx), %rax
	sbbq	8(%rcx), %rsi
	sbbq	16(%rcx), %rbx
	sbbq	24(%rcx), %r14
	sbbq	32(%rcx), %r11
	sbbq	40(%rcx), %r10
	sbbq	48(%rcx), %r9
	sbbq	56(%rcx), %r8
	sbbq	$0, %rdx
	testb	$1, %dl
	jne	LBB67_2
## %bb.1:                               ## %nocarry
	movq	%rax, (%rdi)
	movq	%rsi, 8(%rdi)
	movq	%rbx, 16(%rdi)
	movq	%r14, 24(%rdi)
	movq	%r11, 32(%rdi)
	movq	%r10, 40(%rdi)
	movq	%r9, 48(%rdi)
	movq	%r8, 56(%rdi)
LBB67_2:                                ## %carry
	popq	%rbx
	popq	%r14
	retq
                                        ## -- End function
	.globl	_mcl_fp_addNF8L                 ## -- Begin function mcl_fp_addNF8L
	.p2align	4, 0x90
_mcl_fp_addNF8L:                        ## @mcl_fp_addNF8L
## %bb.0:
	pushq	%rbp
	pushq	%r15
	pushq	%r14
	pushq	%r13
	pushq	%r12
	pushq	%rbx
	movq	56(%rdx), %r8
	movq	48(%rdx), %r9
	movq	40(%rdx), %r10
	movq	32(%rdx), %r11
	movq	24(%rdx), %r15
	movq	16(%rdx), %rbx
	movq	(%rdx), %rax
	movq	8(%rdx), %rdx
	addq	(%rsi), %rax
	movq	%rax, -8(%rsp)                  ## 8-byte Spill
	adcq	8(%rsi), %rdx
	movq	%rdx, -16(%rsp)                 ## 8-byte Spill
	adcq	16(%rsi), %rbx
	movq	%rbx, -24(%rsp)                 ## 8-byte Spill
	adcq	24(%rsi), %r15
	adcq	32(%rsi), %r11
	adcq	40(%rsi), %r10
	adcq	48(%rsi), %r9
	adcq	56(%rsi), %r8
	movq	%rax, %rsi
	subq	(%rcx), %rsi
	sbbq	8(%rcx), %rdx
	sbbq	16(%rcx), %rbx
	movq	%r15, %rax
	sbbq	24(%rcx), %rax
	movq	%r11, %rbp
	sbbq	32(%rcx), %rbp
	movq	%r10, %r14
	sbbq	40(%rcx), %r14
	movq	%r9, %r12
	sbbq	48(%rcx), %r12
	movq	%r8, %r13
	sbbq	56(%rcx), %r13
	cmovsq	%r8, %r13
	movq	%r13, 56(%rdi)
	cmovsq	%r9, %r12
	movq	%r12, 48(%rdi)
	cmovsq	%r10, %r14
	movq	%r14, 40(%rdi)
	cmovsq	%r11, %rbp
	movq	%rbp, 32(%rdi)
	cmovsq	%r15, %rax
	movq	%rax, 24(%rdi)
	cmovsq	-24(%rsp), %rbx                 ## 8-byte Folded Reload
	movq	%rbx, 16(%rdi)
	cmovsq	-16(%rsp), %rdx                 ## 8-byte Folded Reload
	movq	%rdx, 8(%rdi)
	cmovsq	-8(%rsp), %rsi                  ## 8-byte Folded Reload
	movq	%rsi, (%rdi)
	popq	%rbx
	popq	%r12
	popq	%r13
	popq	%r14
	popq	%r15
	popq	%rbp
	retq
                                        ## -- End function
	.globl	_mcl_fp_sub8L                   ## -- Begin function mcl_fp_sub8L
	.p2align	4, 0x90
_mcl_fp_sub8L:                          ## @mcl_fp_sub8L
## %bb.0:
	pushq	%r15
	pushq	%r14
	pushq	%rbx
	movq	56(%rsi), %r14
	movq	48(%rsi), %rbx
	movq	40(%rsi), %r11
	movq	32(%rsi), %r10
	movq	24(%rsi), %r9
	movq	16(%rsi), %r15
	movq	(%rsi), %r8
	movq	8(%rsi), %rsi
	xorl	%eax, %eax
	subq	(%rdx), %r8
	sbbq	8(%rdx), %rsi
	sbbq	16(%rdx), %r15
	sbbq	24(%rdx), %r9
	sbbq	32(%rdx), %r10
	sbbq	40(%rdx), %r11
	sbbq	48(%rdx), %rbx
	sbbq	56(%rdx), %r14
	movq	%r14, 56(%rdi)
	movq	%rbx, 48(%rdi)
	movq	%r11, 40(%rdi)
	movq	%r10, 32(%rdi)
	movq	%r9, 24(%rdi)
	movq	%r15, 16(%rdi)
	movq	%rsi, 8(%rdi)
	movq	%r8, (%rdi)
	sbbq	%rax, %rax
	testb	$1, %al
	je	LBB69_2
## %bb.1:                               ## %carry
	addq	(%rcx), %r8
	adcq	8(%rcx), %rsi
	adcq	16(%rcx), %r15
	adcq	24(%rcx), %r9
	adcq	32(%rcx), %r10
	adcq	40(%rcx), %r11
	adcq	48(%rcx), %rbx
	adcq	56(%rcx), %r14
	movq	%r14, 56(%rdi)
	movq	%rbx, 48(%rdi)
	movq	%r11, 40(%rdi)
	movq	%r10, 32(%rdi)
	movq	%r9, 24(%rdi)
	movq	%r15, 16(%rdi)
	movq	%rsi, 8(%rdi)
	movq	%r8, (%rdi)
LBB69_2:                                ## %nocarry
	popq	%rbx
	popq	%r14
	popq	%r15
	retq
                                        ## -- End function
	.globl	_mcl_fp_subNF8L                 ## -- Begin function mcl_fp_subNF8L
	.p2align	4, 0x90
_mcl_fp_subNF8L:                        ## @mcl_fp_subNF8L
## %bb.0:
	pushq	%rbp
	pushq	%r15
	pushq	%r14
	pushq	%r13
	pushq	%r12
	pushq	%rbx
	movq	%rcx, %r8
	movq	%rdi, %r9
	movq	56(%rsi), %r14
	movq	48(%rsi), %rax
	movq	40(%rsi), %rcx
	movq	32(%rsi), %rdi
	movq	24(%rsi), %r11
	movq	16(%rsi), %r15
	movq	(%rsi), %r13
	movq	8(%rsi), %r12
	subq	(%rdx), %r13
	sbbq	8(%rdx), %r12
	sbbq	16(%rdx), %r15
	sbbq	24(%rdx), %r11
	sbbq	32(%rdx), %rdi
	movq	%rdi, -24(%rsp)                 ## 8-byte Spill
	sbbq	40(%rdx), %rcx
	movq	%rcx, -16(%rsp)                 ## 8-byte Spill
	sbbq	48(%rdx), %rax
	movq	%rax, -8(%rsp)                  ## 8-byte Spill
	sbbq	56(%rdx), %r14
	movq	%r14, %rsi
	sarq	$63, %rsi
	movq	56(%r8), %r10
	andq	%rsi, %r10
	movq	48(%r8), %rbx
	andq	%rsi, %rbx
	movq	40(%r8), %rdi
	andq	%rsi, %rdi
	movq	32(%r8), %rbp
	andq	%rsi, %rbp
	movq	24(%r8), %rdx
	andq	%rsi, %rdx
	movq	16(%r8), %rcx
	andq	%rsi, %rcx
	movq	8(%r8), %rax
	andq	%rsi, %rax
	andq	(%r8), %rsi
	addq	%r13, %rsi
	adcq	%r12, %rax
	movq	%rsi, (%r9)
	adcq	%r15, %rcx
	movq	%rax, 8(%r9)
	movq	%rcx, 16(%r9)
	adcq	%r11, %rdx
	movq	%rdx, 24(%r9)
	adcq	-24(%rsp), %rbp                 ## 8-byte Folded Reload
	movq	%rbp, 32(%r9)
	adcq	-16(%rsp), %rdi                 ## 8-byte Folded Reload
	movq	%rdi, 40(%r9)
	adcq	-8(%rsp), %rbx                  ## 8-byte Folded Reload
	movq	%rbx, 48(%r9)
	adcq	%r14, %r10
	movq	%r10, 56(%r9)
	popq	%rbx
	popq	%r12
	popq	%r13
	popq	%r14
	popq	%r15
	popq	%rbp
	retq
                                        ## -- End function
	.globl	_mcl_fpDbl_add8L                ## -- Begin function mcl_fpDbl_add8L
	.p2align	4, 0x90
_mcl_fpDbl_add8L:                       ## @mcl_fpDbl_add8L
## %bb.0:
	pushq	%rbp
	pushq	%r15
	pushq	%r14
	pushq	%r13
	pushq	%r12
	pushq	%rbx
	movq	%rcx, %r15
	movq	120(%rsi), %rax
	movq	%rax, -72(%rsp)                 ## 8-byte Spill
	movq	112(%rsi), %rax
	movq	%rax, -64(%rsp)                 ## 8-byte Spill
	movq	104(%rsi), %rax
	movq	%rax, -56(%rsp)                 ## 8-byte Spill
	movq	96(%rsi), %rbx
	movq	88(%rsi), %rcx
	movq	80(%rsi), %r8
	movq	72(%rsi), %r10
	movq	(%rsi), %rax
	movq	8(%rsi), %rbp
	addq	(%rdx), %rax
	movq	%rax, -8(%rsp)                  ## 8-byte Spill
	adcq	8(%rdx), %rbp
	movq	%rbp, -16(%rsp)                 ## 8-byte Spill
	movq	64(%rsi), %rax
	movq	56(%rsi), %rbp
	movq	48(%rsi), %r13
	movq	40(%rsi), %r14
	movq	32(%rsi), %r9
	movq	24(%rsi), %r11
	movq	16(%rsi), %r12
	adcq	16(%rdx), %r12
	adcq	24(%rdx), %r11
	adcq	32(%rdx), %r9
	adcq	40(%rdx), %r14
	adcq	48(%rdx), %r13
	adcq	56(%rdx), %rbp
	adcq	64(%rdx), %rax
	movq	%rax, -48(%rsp)                 ## 8-byte Spill
	adcq	72(%rdx), %r10
	movq	%r8, %rax
	adcq	80(%rdx), %rax
	movq	%rax, -24(%rsp)                 ## 8-byte Spill
	adcq	88(%rdx), %rcx
	movq	%rcx, -32(%rsp)                 ## 8-byte Spill
	movq	%rbx, %rsi
	adcq	96(%rdx), %rsi
	movq	%rsi, -40(%rsp)                 ## 8-byte Spill
	movq	-56(%rsp), %r8                  ## 8-byte Reload
	adcq	104(%rdx), %r8
	movq	%r8, -56(%rsp)                  ## 8-byte Spill
	movq	-64(%rsp), %rbx                 ## 8-byte Reload
	adcq	112(%rdx), %rbx
	movq	%rbx, -64(%rsp)                 ## 8-byte Spill
	movq	-72(%rsp), %r8                  ## 8-byte Reload
	adcq	120(%rdx), %r8
	movq	%rbp, 56(%rdi)
	movq	%r13, 48(%rdi)
	movq	%r14, 40(%rdi)
	movq	%r9, 32(%rdi)
	movq	%r11, 24(%rdi)
	movq	%r12, 16(%rdi)
	movq	-16(%rsp), %rdx                 ## 8-byte Reload
	movq	%rdx, 8(%rdi)
	movq	-8(%rsp), %rdx                  ## 8-byte Reload
	movq	%rdx, (%rdi)
	setb	-72(%rsp)                       ## 1-byte Folded Spill
	movq	-48(%rsp), %r14                 ## 8-byte Reload
	subq	(%r15), %r14
	movq	%r10, %r9
	movq	%r10, %r13
	sbbq	8(%r15), %r9
	movq	%rax, %r11
	sbbq	16(%r15), %r11
	movq	%rcx, %rbp
	sbbq	24(%r15), %rbp
	movq	%rsi, %rbx
	sbbq	32(%r15), %rbx
	movq	-56(%rsp), %r12                 ## 8-byte Reload
	movq	%r12, %rax
	sbbq	40(%r15), %rax
	movq	-64(%rsp), %r10                 ## 8-byte Reload
	movq	%r10, %rdx
	sbbq	48(%r15), %rdx
	movq	%r8, %rsi
	sbbq	56(%r15), %rsi
	movzbl	-72(%rsp), %ecx                 ## 1-byte Folded Reload
	sbbq	$0, %rcx
	testb	$1, %cl
	cmovneq	%r8, %rsi
	movq	%rsi, 120(%rdi)
	cmovneq	%r10, %rdx
	movq	%rdx, 112(%rdi)
	cmovneq	%r12, %rax
	movq	%rax, 104(%rdi)
	cmovneq	-40(%rsp), %rbx                 ## 8-byte Folded Reload
	movq	%rbx, 96(%rdi)
	cmovneq	-32(%rsp), %rbp                 ## 8-byte Folded Reload
	movq	%rbp, 88(%rdi)
	cmovneq	-24(%rsp), %r11                 ## 8-byte Folded Reload
	movq	%r11, 80(%rdi)
	cmovneq	%r13, %r9
	movq	%r9, 72(%rdi)
	cmovneq	-48(%rsp), %r14                 ## 8-byte Folded Reload
	movq	%r14, 64(%rdi)
	popq	%rbx
	popq	%r12
	popq	%r13
	popq	%r14
	popq	%r15
	popq	%rbp
	retq
                                        ## -- End function
	.globl	_mcl_fpDbl_sub8L                ## -- Begin function mcl_fpDbl_sub8L
	.p2align	4, 0x90
_mcl_fpDbl_sub8L:                       ## @mcl_fpDbl_sub8L
## %bb.0:
	pushq	%rbp
	pushq	%r15
	pushq	%r14
	pushq	%r13
	pushq	%r12
	pushq	%rbx
	movq	%rcx, %r11
	movq	120(%rsi), %rax
	movq	%rax, -64(%rsp)                 ## 8-byte Spill
	movq	112(%rsi), %r12
	movq	104(%rsi), %r15
	movq	96(%rsi), %rax
	movq	%rax, -48(%rsp)                 ## 8-byte Spill
	movq	88(%rsi), %r13
	movq	80(%rsi), %rax
	movq	%rax, -56(%rsp)                 ## 8-byte Spill
	movq	(%rsi), %rcx
	movq	8(%rsi), %rbp
	xorl	%eax, %eax
	subq	(%rdx), %rcx
	movq	%rcx, -32(%rsp)                 ## 8-byte Spill
	sbbq	8(%rdx), %rbp
	movq	%rbp, -40(%rsp)                 ## 8-byte Spill
	movq	72(%rsi), %rbp
	movq	64(%rsi), %rcx
	movq	56(%rsi), %r8
	movq	48(%rsi), %r9
	movq	40(%rsi), %r10
	movq	32(%rsi), %r14
	movq	24(%rsi), %rbx
	movq	16(%rsi), %rsi
	sbbq	16(%rdx), %rsi
	sbbq	24(%rdx), %rbx
	sbbq	32(%rdx), %r14
	sbbq	40(%rdx), %r10
	sbbq	48(%rdx), %r9
	sbbq	56(%rdx), %r8
	sbbq	64(%rdx), %rcx
	movq	%rcx, -24(%rsp)                 ## 8-byte Spill
	sbbq	72(%rdx), %rbp
	movq	%rbp, -16(%rsp)                 ## 8-byte Spill
	movq	-56(%rsp), %rbp                 ## 8-byte Reload
	sbbq	80(%rdx), %rbp
	movq	%rbp, -56(%rsp)                 ## 8-byte Spill
	sbbq	88(%rdx), %r13
	movq	%r13, -8(%rsp)                  ## 8-byte Spill
	movq	-48(%rsp), %r13                 ## 8-byte Reload
	sbbq	96(%rdx), %r13
	movq	%r13, -48(%rsp)                 ## 8-byte Spill
	sbbq	104(%rdx), %r15
	sbbq	112(%rdx), %r12
	movq	-64(%rsp), %rcx                 ## 8-byte Reload
	sbbq	120(%rdx), %rcx
	movq	%rcx, -64(%rsp)                 ## 8-byte Spill
	movq	%r8, 56(%rdi)
	movq	%r9, 48(%rdi)
	movq	%r10, 40(%rdi)
	movq	%r14, 32(%rdi)
	movq	%rbx, 24(%rdi)
	movq	%rsi, 16(%rdi)
	movq	-40(%rsp), %rcx                 ## 8-byte Reload
	movq	%rcx, 8(%rdi)
	movq	-32(%rsp), %rcx                 ## 8-byte Reload
	movq	%rcx, (%rdi)
	sbbq	%rax, %rax
	andl	$1, %eax
	negq	%rax
	movq	56(%r11), %r8
	andq	%rax, %r8
	movq	48(%r11), %r9
	andq	%rax, %r9
	movq	40(%r11), %r10
	andq	%rax, %r10
	movq	32(%r11), %rbx
	andq	%rax, %rbx
	movq	24(%r11), %rdx
	andq	%rax, %rdx
	movq	16(%r11), %rsi
	andq	%rax, %rsi
	movq	8(%r11), %rbp
	andq	%rax, %rbp
	andq	(%r11), %rax
	addq	-24(%rsp), %rax                 ## 8-byte Folded Reload
	adcq	-16(%rsp), %rbp                 ## 8-byte Folded Reload
	movq	%rax, 64(%rdi)
	adcq	-56(%rsp), %rsi                 ## 8-byte Folded Reload
	movq	%rbp, 72(%rdi)
	movq	%rsi, 80(%rdi)
	adcq	-8(%rsp), %rdx                  ## 8-byte Folded Reload
	movq	%rdx, 88(%rdi)
	adcq	-48(%rsp), %rbx                 ## 8-byte Folded Reload
	movq	%rbx, 96(%rdi)
	adcq	%r15, %r10
	movq	%r10, 104(%rdi)
	adcq	%r12, %r9
	movq	%r9, 112(%rdi)
	adcq	-64(%rsp), %r8                  ## 8-byte Folded Reload
	movq	%r8, 120(%rdi)
	popq	%rbx
	popq	%r12
	popq	%r13
	popq	%r14
	popq	%r15
	popq	%rbp
	retq
                                        ## -- End function
.subsections_via_symbols
