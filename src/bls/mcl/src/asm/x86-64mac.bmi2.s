	.section	__TEXT,__text,regular,pure_instructions
	.build_version macos, 11, 0
	.globl	_makeNIST_P192Lbmi2             ## -- Begin function makeNIST_P192Lbmi2
	.p2align	4, 0x90
_makeNIST_P192Lbmi2:                    ## @makeNIST_P192Lbmi2
## %bb.0:
	movq	$-1, %rax
	movq	$-2, %rdx
	movq	$-1, %rcx
	retq
                                        ## -- End function
	.globl	_mcl_fpDbl_mod_NIST_P192Lbmi2   ## -- Begin function mcl_fpDbl_mod_NIST_P192Lbmi2
	.p2align	4, 0x90
_mcl_fpDbl_mod_NIST_P192Lbmi2:          ## @mcl_fpDbl_mod_NIST_P192Lbmi2
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
	.globl	_mcl_fp_sqr_NIST_P192Lbmi2      ## -- Begin function mcl_fp_sqr_NIST_P192Lbmi2
	.p2align	4, 0x90
_mcl_fp_sqr_NIST_P192Lbmi2:             ## @mcl_fp_sqr_NIST_P192Lbmi2
## %bb.0:
	pushq	%rbp
	pushq	%r15
	pushq	%r14
	pushq	%r13
	pushq	%r12
	pushq	%rbx
	movq	16(%rsi), %r9
	movq	(%rsi), %rcx
	movq	8(%rsi), %rsi
	movq	%r9, %rdx
	mulxq	%rsi, %r11, %r10
	movq	%rsi, %rdx
	mulxq	%rsi, %r12, %r14
	mulxq	%rcx, %r15, %rsi
	addq	%rsi, %r12
	adcq	%r11, %r14
	movq	%r10, %rbx
	adcq	$0, %rbx
	movq	%rcx, %rdx
	mulxq	%rcx, %r8, %rax
	addq	%r15, %rax
	movq	%r9, %rdx
	mulxq	%rcx, %r13, %rcx
	adcq	%r13, %rsi
	movq	%rcx, %rbp
	adcq	$0, %rbp
	addq	%r15, %rax
	adcq	%r12, %rsi
	adcq	%r14, %rbp
	adcq	$0, %rbx
	movq	%r9, %rdx
	mulxq	%r9, %r9, %rdx
	addq	%r11, %rcx
	adcq	%r10, %r9
	adcq	$0, %rdx
	addq	%r13, %rsi
	adcq	%rbp, %rcx
	adcq	%rbx, %r9
	adcq	$0, %rdx
	addq	%rdx, %rax
	adcq	$0, %rsi
	setb	%bl
	movzbl	%bl, %ebx
	addq	%rcx, %r8
	adcq	%r9, %rax
	adcq	%rdx, %rsi
	adcq	$0, %rbx
	addq	%rdx, %r8
	adcq	%rcx, %rax
	adcq	%r9, %rsi
	setb	%cl
	movq	%rbx, %rbp
	adcq	$0, %rbp
	addb	$255, %cl
	adcq	%rbx, %r8
	adcq	%rax, %rbp
	adcq	$0, %rsi
	setb	%al
	movzbl	%al, %eax
	movq	%r8, %rcx
	addq	$1, %rcx
	movq	%rbp, %rdx
	adcq	$1, %rdx
	movq	%rsi, %rbx
	adcq	$0, %rbx
	adcq	$-1, %rax
	testb	$1, %al
	cmovneq	%rsi, %rbx
	movq	%rbx, 16(%rdi)
	cmovneq	%rbp, %rdx
	movq	%rdx, 8(%rdi)
	cmovneq	%r8, %rcx
	movq	%rcx, (%rdi)
	popq	%rbx
	popq	%r12
	popq	%r13
	popq	%r14
	popq	%r15
	popq	%rbp
	retq
                                        ## -- End function
	.globl	_mcl_fp_mulNIST_P192Lbmi2       ## -- Begin function mcl_fp_mulNIST_P192Lbmi2
	.p2align	4, 0x90
_mcl_fp_mulNIST_P192Lbmi2:              ## @mcl_fp_mulNIST_P192Lbmi2
## %bb.0:
	pushq	%r14
	pushq	%rbx
	subq	$56, %rsp
	movq	%rdi, %r14
	leaq	8(%rsp), %rdi
	callq	_mcl_fpDbl_mulPre3Lbmi2
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
	.globl	_mcl_fpDbl_mod_NIST_P521Lbmi2   ## -- Begin function mcl_fpDbl_mod_NIST_P521Lbmi2
	.p2align	4, 0x90
_mcl_fpDbl_mod_NIST_P521Lbmi2:          ## @mcl_fpDbl_mod_NIST_P521Lbmi2
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
	.globl	_mulPv192x64bmi2                ## -- Begin function mulPv192x64bmi2
	.p2align	4, 0x90
_mulPv192x64bmi2:                       ## @mulPv192x64bmi2
## %bb.0:
	movq	%rdi, %rax
	mulxq	(%rsi), %rdi, %rcx
	movq	%rdi, (%rax)
	mulxq	8(%rsi), %rdi, %r8
	addq	%rcx, %rdi
	movq	%rdi, 8(%rax)
	mulxq	16(%rsi), %rcx, %rdx
	adcq	%r8, %rcx
	movq	%rcx, 16(%rax)
	adcq	$0, %rdx
	movq	%rdx, 24(%rax)
	retq
                                        ## -- End function
	.globl	_mcl_fp_mulUnitPre3Lbmi2        ## -- Begin function mcl_fp_mulUnitPre3Lbmi2
	.p2align	4, 0x90
_mcl_fp_mulUnitPre3Lbmi2:               ## @mcl_fp_mulUnitPre3Lbmi2
## %bb.0:
	mulxq	16(%rsi), %r8, %rcx
	mulxq	8(%rsi), %r9, %rax
	mulxq	(%rsi), %rdx, %rsi
	movq	%rdx, (%rdi)
	addq	%r9, %rsi
	movq	%rsi, 8(%rdi)
	adcq	%r8, %rax
	movq	%rax, 16(%rdi)
	adcq	$0, %rcx
	movq	%rcx, 24(%rdi)
	retq
                                        ## -- End function
	.globl	_mcl_fpDbl_mulPre3Lbmi2         ## -- Begin function mcl_fpDbl_mulPre3Lbmi2
	.p2align	4, 0x90
_mcl_fpDbl_mulPre3Lbmi2:                ## @mcl_fpDbl_mulPre3Lbmi2
## %bb.0:
	pushq	%rbp
	pushq	%r15
	pushq	%r14
	pushq	%r13
	pushq	%r12
	pushq	%rbx
	movq	%rdx, %r10
	movq	(%rsi), %r8
	movq	8(%rsi), %r9
	movq	(%rdx), %r13
	movq	%r8, %rdx
	mulxq	%r13, %rdx, %rax
	movq	16(%rsi), %r12
	movq	%rdx, (%rdi)
	movq	8(%r10), %rdx
	mulxq	%r9, %rsi, %r15
	mulxq	%r8, %r14, %rbp
	addq	%rsi, %rbp
	mulxq	%r12, %r11, %rsi
	adcq	%r15, %r11
	adcq	$0, %rsi
	movq	%r9, %rdx
	mulxq	%r13, %rcx, %r15
	addq	%rax, %rcx
	movq	%r12, %rdx
	mulxq	%r13, %rbx, %r13
	adcq	%r15, %rbx
	adcq	$0, %r13
	addq	%r14, %rcx
	movq	%rcx, 8(%rdi)
	adcq	%rbp, %rbx
	adcq	%r11, %r13
	adcq	$0, %rsi
	movq	16(%r10), %rdx
	mulxq	%r12, %r10, %rbp
	mulxq	%r9, %r9, %rcx
	mulxq	%r8, %rdx, %rax
	addq	%r9, %rax
	adcq	%r10, %rcx
	adcq	$0, %rbp
	addq	%rbx, %rdx
	movq	%rdx, 16(%rdi)
	adcq	%r13, %rax
	movq	%rax, 24(%rdi)
	adcq	%rsi, %rcx
	movq	%rcx, 32(%rdi)
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
	.globl	_mcl_fpDbl_sqrPre3Lbmi2         ## -- Begin function mcl_fpDbl_sqrPre3Lbmi2
	.p2align	4, 0x90
_mcl_fpDbl_sqrPre3Lbmi2:                ## @mcl_fpDbl_sqrPre3Lbmi2
## %bb.0:
	pushq	%r15
	pushq	%r14
	pushq	%r13
	pushq	%r12
	pushq	%rbx
	movq	16(%rsi), %r8
	movq	(%rsi), %rcx
	movq	8(%rsi), %rsi
	movq	%rcx, %rdx
	mulxq	%rcx, %rdx, %rax
	movq	%rdx, (%rdi)
	movq	%r8, %rdx
	mulxq	%rsi, %r10, %r9
	movq	%rsi, %rdx
	mulxq	%rsi, %r11, %r15
	mulxq	%rcx, %r14, %rsi
	addq	%rsi, %r11
	adcq	%r10, %r15
	movq	%r9, %r13
	adcq	$0, %r13
	addq	%r14, %rax
	movq	%r8, %rdx
	mulxq	%rcx, %r12, %rcx
	adcq	%r12, %rsi
	movq	%rcx, %rbx
	adcq	$0, %rbx
	addq	%r14, %rax
	movq	%rax, 8(%rdi)
	adcq	%r11, %rsi
	adcq	%r15, %rbx
	adcq	$0, %r13
	movq	%r8, %rdx
	mulxq	%r8, %rax, %rdx
	addq	%r10, %rcx
	adcq	%r9, %rax
	adcq	$0, %rdx
	addq	%r12, %rsi
	movq	%rsi, 16(%rdi)
	adcq	%rbx, %rcx
	movq	%rcx, 24(%rdi)
	adcq	%r13, %rax
	movq	%rax, 32(%rdi)
	adcq	$0, %rdx
	movq	%rdx, 40(%rdi)
	popq	%rbx
	popq	%r12
	popq	%r13
	popq	%r14
	popq	%r15
	retq
                                        ## -- End function
	.globl	_mcl_fp_mont3Lbmi2              ## -- Begin function mcl_fp_mont3Lbmi2
	.p2align	4, 0x90
_mcl_fp_mont3Lbmi2:                     ## @mcl_fp_mont3Lbmi2
## %bb.0:
	pushq	%rbp
	pushq	%r15
	pushq	%r14
	pushq	%r13
	pushq	%r12
	pushq	%rbx
	movq	%rdx, %r14
	movq	%rdi, -8(%rsp)                  ## 8-byte Spill
	movq	16(%rsi), %rdi
	movq	%rdi, -48(%rsp)                 ## 8-byte Spill
	movq	(%rdx), %rax
	movq	%rdx, -16(%rsp)                 ## 8-byte Spill
	movq	%rdi, %rdx
	mulxq	%rax, %r11, %rbx
	movq	(%rsi), %rdi
	movq	%rdi, -56(%rsp)                 ## 8-byte Spill
	movq	8(%rsi), %rdx
	movq	%rdx, -64(%rsp)                 ## 8-byte Spill
	mulxq	%rax, %r15, %rbp
	movq	%rdi, %rdx
	mulxq	%rax, %r9, %r8
	addq	%r15, %r8
	adcq	%r11, %rbp
	adcq	$0, %rbx
	movq	-8(%rcx), %r13
	movq	%r13, %rdx
	imulq	%r9, %rdx
	movq	8(%rcx), %rax
	movq	%rax, -32(%rsp)                 ## 8-byte Spill
	mulxq	%rax, %r12, %r10
	movq	(%rcx), %rax
	movq	%rax, -40(%rsp)                 ## 8-byte Spill
	mulxq	%rax, %r11, %rax
	addq	%r12, %rax
	movq	16(%rcx), %rdi
	mulxq	%rdi, %rcx, %rsi
	movq	%rdi, %r15
	movq	%rdi, -24(%rsp)                 ## 8-byte Spill
	adcq	%r10, %rcx
	adcq	$0, %rsi
	addq	%r9, %r11
	adcq	%r8, %rax
	movq	8(%r14), %rdx
	adcq	%rbp, %rcx
	adcq	%rbx, %rsi
	movq	-48(%rsp), %r14                 ## 8-byte Reload
	mulxq	%r14, %r9, %r8
	mulxq	-64(%rsp), %rbp, %rbx           ## 8-byte Folded Reload
	mulxq	-56(%rsp), %r10, %rdi           ## 8-byte Folded Reload
	setb	%dl
	addq	%rbp, %rdi
	adcq	%r9, %rbx
	adcq	$0, %r8
	addq	%rax, %r10
	adcq	%rcx, %rdi
	movzbl	%dl, %eax
	adcq	%rsi, %rbx
	adcq	%rax, %r8
	setb	%r11b
	movq	%r13, %rdx
	imulq	%r10, %rdx
	mulxq	%r15, %r9, %rcx
	movq	-32(%rsp), %r12                 ## 8-byte Reload
	mulxq	%r12, %rsi, %rbp
	movq	-40(%rsp), %r15                 ## 8-byte Reload
	mulxq	%r15, %rdx, %rax
	addq	%rsi, %rax
	adcq	%r9, %rbp
	adcq	$0, %rcx
	addq	%r10, %rdx
	adcq	%rdi, %rax
	movzbl	%r11b, %r9d
	adcq	%rbx, %rbp
	adcq	%r8, %rcx
	adcq	$0, %r9
	movq	-16(%rsp), %rdx                 ## 8-byte Reload
	movq	16(%rdx), %rdx
	mulxq	%r14, %r8, %rsi
	mulxq	-64(%rsp), %r10, %r14           ## 8-byte Folded Reload
	mulxq	-56(%rsp), %r11, %rdi           ## 8-byte Folded Reload
	addq	%r10, %rdi
	adcq	%r8, %r14
	adcq	$0, %rsi
	addq	%rax, %r11
	adcq	%rbp, %rdi
	adcq	%rcx, %r14
	adcq	%r9, %rsi
	setb	%r8b
	imulq	%r11, %r13
	movq	%r13, %rdx
	mulxq	%r15, %rax, %rbp
	movq	%r12, %r10
	mulxq	%r12, %rcx, %r9
	addq	%rbp, %rcx
	movq	-24(%rsp), %r12                 ## 8-byte Reload
	mulxq	%r12, %rdx, %rbx
	adcq	%r9, %rdx
	adcq	$0, %rbx
	addq	%r11, %rax
	adcq	%rdi, %rcx
	adcq	%r14, %rdx
	movzbl	%r8b, %eax
	adcq	%rsi, %rbx
	adcq	$0, %rax
	movq	%rcx, %rsi
	subq	%r15, %rsi
	movq	%rdx, %rdi
	sbbq	%r10, %rdi
	movq	%rbx, %rbp
	sbbq	%r12, %rbp
	sbbq	$0, %rax
	testb	$1, %al
	cmovneq	%rbx, %rbp
	movq	-8(%rsp), %rax                  ## 8-byte Reload
	movq	%rbp, 16(%rax)
	cmovneq	%rdx, %rdi
	movq	%rdi, 8(%rax)
	cmovneq	%rcx, %rsi
	movq	%rsi, (%rax)
	popq	%rbx
	popq	%r12
	popq	%r13
	popq	%r14
	popq	%r15
	popq	%rbp
	retq
                                        ## -- End function
	.globl	_mcl_fp_montNF3Lbmi2            ## -- Begin function mcl_fp_montNF3Lbmi2
	.p2align	4, 0x90
_mcl_fp_montNF3Lbmi2:                   ## @mcl_fp_montNF3Lbmi2
## %bb.0:
	pushq	%rbp
	pushq	%r15
	pushq	%r14
	pushq	%r13
	pushq	%r12
	pushq	%rbx
	movq	%rdx, %r10
	movq	%rdi, -8(%rsp)                  ## 8-byte Spill
	movq	(%rsi), %r11
	movq	8(%rsi), %rbp
	movq	%rbp, -32(%rsp)                 ## 8-byte Spill
	movq	(%rdx), %rax
	movq	%rdx, -16(%rsp)                 ## 8-byte Spill
	movq	%rbp, %rdx
	mulxq	%rax, %rbx, %r14
	movq	%r11, %rdx
	movq	%r11, -24(%rsp)                 ## 8-byte Spill
	mulxq	%rax, %r15, %r12
	movq	16(%rsi), %rdx
	movq	%rdx, -40(%rsp)                 ## 8-byte Spill
	addq	%rbx, %r12
	mulxq	%rax, %rsi, %rbx
	adcq	%r14, %rsi
	adcq	$0, %rbx
	movq	-8(%rcx), %r13
	movq	(%rcx), %r14
	movq	%r13, %rax
	imulq	%r15, %rax
	movq	%r14, %rdx
	mulxq	%rax, %rdx, %rbp
	addq	%r15, %rdx
	movq	8(%rcx), %r15
	movq	%r15, %rdx
	mulxq	%rax, %rdi, %r9
	adcq	%r12, %rdi
	movq	16(%rcx), %r12
	movq	%r12, %rdx
	mulxq	%rax, %r8, %rax
	adcq	%rsi, %r8
	adcq	$0, %rbx
	addq	%rbp, %rdi
	movq	8(%r10), %rcx
	adcq	%r9, %r8
	adcq	%rax, %rbx
	movq	-32(%rsp), %r10                 ## 8-byte Reload
	movq	%r10, %rdx
	mulxq	%rcx, %rsi, %r9
	movq	%r11, %rdx
	mulxq	%rcx, %rbp, %rax
	addq	%rsi, %rax
	movq	-40(%rsp), %r11                 ## 8-byte Reload
	movq	%r11, %rdx
	mulxq	%rcx, %rsi, %rcx
	adcq	%r9, %rsi
	adcq	$0, %rcx
	addq	%rdi, %rbp
	adcq	%r8, %rax
	adcq	%rbx, %rsi
	adcq	$0, %rcx
	movq	%r13, %rdx
	imulq	%rbp, %rdx
	mulxq	%r14, %rbx, %r8
	addq	%rbp, %rbx
	mulxq	%r15, %rdi, %rbx
	adcq	%rax, %rdi
	mulxq	%r12, %rbp, %rax
	adcq	%rsi, %rbp
	adcq	$0, %rcx
	addq	%r8, %rdi
	adcq	%rbx, %rbp
	adcq	%rax, %rcx
	movq	-16(%rsp), %rax                 ## 8-byte Reload
	movq	16(%rax), %rdx
	mulxq	%r10, %rbx, %r8
	mulxq	-24(%rsp), %r9, %rsi            ## 8-byte Folded Reload
	addq	%rbx, %rsi
	mulxq	%r11, %rax, %rbx
	adcq	%r8, %rax
	adcq	$0, %rbx
	addq	%rdi, %r9
	adcq	%rbp, %rsi
	adcq	%rcx, %rax
	adcq	$0, %rbx
	imulq	%r9, %r13
	movq	%r14, %rdx
	mulxq	%r13, %rdx, %r8
	addq	%r9, %rdx
	movq	%r12, %rdx
	mulxq	%r13, %rbp, %rdi
	movq	%r15, %rdx
	mulxq	%r13, %rcx, %rdx
	adcq	%rsi, %rcx
	adcq	%rax, %rbp
	adcq	$0, %rbx
	addq	%r8, %rcx
	adcq	%rdx, %rbp
	adcq	%rdi, %rbx
	movq	%rcx, %rax
	subq	%r14, %rax
	movq	%rbp, %rdx
	sbbq	%r15, %rdx
	movq	%rbx, %rsi
	sbbq	%r12, %rsi
	movq	%rsi, %rdi
	sarq	$63, %rdi
	cmovsq	%rbx, %rsi
	movq	-8(%rsp), %rdi                  ## 8-byte Reload
	movq	%rsi, 16(%rdi)
	cmovsq	%rbp, %rdx
	movq	%rdx, 8(%rdi)
	cmovsq	%rcx, %rax
	movq	%rax, (%rdi)
	popq	%rbx
	popq	%r12
	popq	%r13
	popq	%r14
	popq	%r15
	popq	%rbp
	retq
                                        ## -- End function
	.globl	_mcl_fp_montRed3Lbmi2           ## -- Begin function mcl_fp_montRed3Lbmi2
	.p2align	4, 0x90
_mcl_fp_montRed3Lbmi2:                  ## @mcl_fp_montRed3Lbmi2
## %bb.0:
	pushq	%rbp
	pushq	%r15
	pushq	%r14
	pushq	%r13
	pushq	%r12
	pushq	%rbx
	movq	%rdx, %rcx
	movq	%rdi, -8(%rsp)                  ## 8-byte Spill
	movq	-8(%rdx), %r14
	movq	(%rdx), %r8
	movq	(%rsi), %rax
	movq	%rax, %rdx
	imulq	%r14, %rdx
	movq	16(%rcx), %r9
	mulxq	%r9, %r15, %r10
	movq	8(%rcx), %r11
	mulxq	%r11, %rbx, %r12
	mulxq	%r8, %rdx, %rcx
	addq	%rbx, %rcx
	adcq	%r15, %r12
	adcq	$0, %r10
	addq	%rax, %rdx
	adcq	8(%rsi), %rcx
	adcq	16(%rsi), %r12
	adcq	24(%rsi), %r10
	setb	%r13b
	movq	%r14, %rdx
	imulq	%rcx, %rdx
	mulxq	%r8, %rbp, %rax
	mulxq	%r11, %rbx, %rdi
	addq	%rax, %rbx
	mulxq	%r9, %r15, %rdx
	adcq	%rdi, %r15
	movzbl	%r13b, %edi
	adcq	%rdx, %rdi
	addq	%rcx, %rbp
	adcq	%r12, %rbx
	adcq	%r10, %r15
	adcq	32(%rsi), %rdi
	setb	%r10b
	imulq	%rbx, %r14
	movq	%r14, %rdx
	mulxq	%r8, %r13, %rbp
	mulxq	%r11, %rcx, %r12
	addq	%rbp, %rcx
	mulxq	%r9, %rbp, %r14
	adcq	%r12, %rbp
	movzbl	%r10b, %eax
	adcq	%r14, %rax
	addq	%rbx, %r13
	adcq	%r15, %rcx
	adcq	%rdi, %rbp
	adcq	40(%rsi), %rax
	xorl	%ebx, %ebx
	movq	%rcx, %rsi
	subq	%r8, %rsi
	movq	%rbp, %rdi
	sbbq	%r11, %rdi
	movq	%rax, %rdx
	sbbq	%r9, %rdx
	sbbq	%rbx, %rbx
	testb	$1, %bl
	cmovneq	%rax, %rdx
	movq	-8(%rsp), %rax                  ## 8-byte Reload
	movq	%rdx, 16(%rax)
	cmovneq	%rbp, %rdi
	movq	%rdi, 8(%rax)
	cmovneq	%rcx, %rsi
	movq	%rsi, (%rax)
	popq	%rbx
	popq	%r12
	popq	%r13
	popq	%r14
	popq	%r15
	popq	%rbp
	retq
                                        ## -- End function
	.globl	_mcl_fp_montRedNF3Lbmi2         ## -- Begin function mcl_fp_montRedNF3Lbmi2
	.p2align	4, 0x90
_mcl_fp_montRedNF3Lbmi2:                ## @mcl_fp_montRedNF3Lbmi2
## %bb.0:
	pushq	%rbp
	pushq	%r15
	pushq	%r14
	pushq	%r13
	pushq	%r12
	pushq	%rbx
	movq	%rdx, %rcx
	movq	%rdi, -8(%rsp)                  ## 8-byte Spill
	movq	-8(%rdx), %r14
	movq	(%rdx), %r8
	movq	(%rsi), %rbx
	movq	%rbx, %rdx
	imulq	%r14, %rdx
	movq	16(%rcx), %r9
	mulxq	%r9, %r12, %r10
	movq	8(%rcx), %r11
	mulxq	%r11, %rcx, %r15
	mulxq	%r8, %rdx, %rax
	addq	%rcx, %rax
	adcq	%r12, %r15
	adcq	$0, %r10
	addq	%rbx, %rdx
	adcq	8(%rsi), %rax
	adcq	16(%rsi), %r15
	adcq	24(%rsi), %r10
	setb	%r13b
	movq	%r14, %rdx
	imulq	%rax, %rdx
	mulxq	%r8, %rbp, %rcx
	mulxq	%r11, %rbx, %rdi
	addq	%rcx, %rbx
	mulxq	%r9, %r12, %rdx
	adcq	%rdi, %r12
	movzbl	%r13b, %ecx
	adcq	%rdx, %rcx
	addq	%rax, %rbp
	adcq	%r15, %rbx
	adcq	%r10, %r12
	adcq	32(%rsi), %rcx
	setb	%r10b
	imulq	%rbx, %r14
	movq	%r14, %rdx
	mulxq	%r8, %r13, %rdi
	mulxq	%r11, %rax, %r15
	addq	%rdi, %rax
	mulxq	%r9, %rdi, %r14
	adcq	%r15, %rdi
	movzbl	%r10b, %r10d
	adcq	%r14, %r10
	addq	%rbx, %r13
	adcq	%r12, %rax
	adcq	%rcx, %rdi
	adcq	40(%rsi), %r10
	movq	%rax, %rcx
	subq	%r8, %rcx
	movq	%rdi, %rsi
	sbbq	%r11, %rsi
	movq	%r10, %rbp
	sbbq	%r9, %rbp
	movq	%rbp, %rdx
	sarq	$63, %rdx
	cmovsq	%r10, %rbp
	movq	-8(%rsp), %rdx                  ## 8-byte Reload
	movq	%rbp, 16(%rdx)
	cmovsq	%rdi, %rsi
	movq	%rsi, 8(%rdx)
	cmovsq	%rax, %rcx
	movq	%rcx, (%rdx)
	popq	%rbx
	popq	%r12
	popq	%r13
	popq	%r14
	popq	%r15
	popq	%rbp
	retq
                                        ## -- End function
	.globl	_mcl_fp_addPre3Lbmi2            ## -- Begin function mcl_fp_addPre3Lbmi2
	.p2align	4, 0x90
_mcl_fp_addPre3Lbmi2:                   ## @mcl_fp_addPre3Lbmi2
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
	.globl	_mcl_fp_subPre3Lbmi2            ## -- Begin function mcl_fp_subPre3Lbmi2
	.p2align	4, 0x90
_mcl_fp_subPre3Lbmi2:                   ## @mcl_fp_subPre3Lbmi2
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
	.globl	_mcl_fp_shr1_3Lbmi2             ## -- Begin function mcl_fp_shr1_3Lbmi2
	.p2align	4, 0x90
_mcl_fp_shr1_3Lbmi2:                    ## @mcl_fp_shr1_3Lbmi2
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
	.globl	_mcl_fp_add3Lbmi2               ## -- Begin function mcl_fp_add3Lbmi2
	.p2align	4, 0x90
_mcl_fp_add3Lbmi2:                      ## @mcl_fp_add3Lbmi2
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
	.globl	_mcl_fp_addNF3Lbmi2             ## -- Begin function mcl_fp_addNF3Lbmi2
	.p2align	4, 0x90
_mcl_fp_addNF3Lbmi2:                    ## @mcl_fp_addNF3Lbmi2
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
	.globl	_mcl_fp_sub3Lbmi2               ## -- Begin function mcl_fp_sub3Lbmi2
	.p2align	4, 0x90
_mcl_fp_sub3Lbmi2:                      ## @mcl_fp_sub3Lbmi2
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
	.globl	_mcl_fp_subNF3Lbmi2             ## -- Begin function mcl_fp_subNF3Lbmi2
	.p2align	4, 0x90
_mcl_fp_subNF3Lbmi2:                    ## @mcl_fp_subNF3Lbmi2
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
	.globl	_mcl_fpDbl_add3Lbmi2            ## -- Begin function mcl_fpDbl_add3Lbmi2
	.p2align	4, 0x90
_mcl_fpDbl_add3Lbmi2:                   ## @mcl_fpDbl_add3Lbmi2
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
	.globl	_mcl_fpDbl_sub3Lbmi2            ## -- Begin function mcl_fpDbl_sub3Lbmi2
	.p2align	4, 0x90
_mcl_fpDbl_sub3Lbmi2:                   ## @mcl_fpDbl_sub3Lbmi2
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
	.globl	_mulPv256x64bmi2                ## -- Begin function mulPv256x64bmi2
	.p2align	4, 0x90
_mulPv256x64bmi2:                       ## @mulPv256x64bmi2
## %bb.0:
	movq	%rdi, %rax
	mulxq	(%rsi), %rdi, %rcx
	movq	%rdi, (%rax)
	mulxq	8(%rsi), %rdi, %r8
	addq	%rcx, %rdi
	movq	%rdi, 8(%rax)
	mulxq	16(%rsi), %rdi, %rcx
	adcq	%r8, %rdi
	movq	%rdi, 16(%rax)
	mulxq	24(%rsi), %rdx, %rsi
	adcq	%rcx, %rdx
	movq	%rdx, 24(%rax)
	adcq	$0, %rsi
	movq	%rsi, 32(%rax)
	retq
                                        ## -- End function
	.globl	_mcl_fp_mulUnitPre4Lbmi2        ## -- Begin function mcl_fp_mulUnitPre4Lbmi2
	.p2align	4, 0x90
_mcl_fp_mulUnitPre4Lbmi2:               ## @mcl_fp_mulUnitPre4Lbmi2
## %bb.0:
	mulxq	24(%rsi), %r8, %r11
	mulxq	16(%rsi), %r9, %rax
	mulxq	8(%rsi), %r10, %rcx
	mulxq	(%rsi), %rdx, %rsi
	movq	%rdx, (%rdi)
	addq	%r10, %rsi
	movq	%rsi, 8(%rdi)
	adcq	%r9, %rcx
	movq	%rcx, 16(%rdi)
	adcq	%r8, %rax
	movq	%rax, 24(%rdi)
	adcq	$0, %r11
	movq	%r11, 32(%rdi)
	retq
                                        ## -- End function
	.globl	_mcl_fpDbl_mulPre4Lbmi2         ## -- Begin function mcl_fpDbl_mulPre4Lbmi2
	.p2align	4, 0x90
_mcl_fpDbl_mulPre4Lbmi2:                ## @mcl_fpDbl_mulPre4Lbmi2
## %bb.0:
	pushq	%rbp
	pushq	%r15
	pushq	%r14
	pushq	%r13
	pushq	%r12
	pushq	%rbx
	movq	%rdi, %r9
	movq	(%rsi), %r14
	movq	8(%rsi), %rbp
	movq	(%rdx), %rax
	movq	%rdx, %rbx
	movq	%rdx, -16(%rsp)                 ## 8-byte Spill
	movq	%r14, %rdx
	mulxq	%rax, %rcx, %r10
	movq	16(%rsi), %rdi
	movq	24(%rsi), %r11
	movq	%rcx, (%r9)
	movq	%r11, %rdx
	mulxq	%rax, %r12, %r15
	movq	%rbp, %rdx
	mulxq	%rax, %rsi, %r8
	addq	%r10, %rsi
	movq	%rdi, %rdx
	movq	%rdi, %r10
	mulxq	%rax, %rax, %rcx
	adcq	%r8, %rax
	adcq	%r12, %rcx
	adcq	$0, %r15
	movq	8(%rbx), %rdx
	mulxq	%r14, %r13, %r8
	movq	%r14, -8(%rsp)                  ## 8-byte Spill
	addq	%rsi, %r13
	mulxq	%rbp, %rbx, %r12
	adcq	%rax, %rbx
	mulxq	%rdi, %rsi, %rax
	adcq	%rcx, %rsi
	mulxq	%r11, %rcx, %rdx
	adcq	%r15, %rcx
	setb	%r15b
	addq	%r8, %rbx
	adcq	%r12, %rsi
	movq	%r13, 8(%r9)
	movzbl	%r15b, %r8d
	adcq	%rax, %rcx
	adcq	%rdx, %r8
	movq	-16(%rsp), %rax                 ## 8-byte Reload
	movq	16(%rax), %rdx
	mulxq	%rbp, %rdi, %r15
	mulxq	%r14, %rax, %r12
	addq	%rdi, %r12
	mulxq	%r10, %r13, %r14
	adcq	%r15, %r13
	mulxq	%r11, %rdi, %r15
	adcq	%r14, %rdi
	adcq	$0, %r15
	addq	%rbx, %rax
	adcq	%rsi, %r12
	movq	%rax, 16(%r9)
	adcq	%rcx, %r13
	adcq	%r8, %rdi
	adcq	$0, %r15
	movq	-16(%rsp), %rax                 ## 8-byte Reload
	movq	24(%rax), %rdx
	mulxq	%rbp, %rcx, %r8
	mulxq	-8(%rsp), %rsi, %rbp            ## 8-byte Folded Reload
	addq	%rcx, %rbp
	mulxq	%r11, %rcx, %rbx
	mulxq	%r10, %rdx, %rax
	adcq	%r8, %rdx
	adcq	%rcx, %rax
	adcq	$0, %rbx
	addq	%r12, %rsi
	movq	%rsi, 24(%r9)
	adcq	%r13, %rbp
	movq	%rbp, 32(%r9)
	adcq	%rdi, %rdx
	movq	%rdx, 40(%r9)
	adcq	%r15, %rax
	movq	%rax, 48(%r9)
	adcq	$0, %rbx
	movq	%rbx, 56(%r9)
	popq	%rbx
	popq	%r12
	popq	%r13
	popq	%r14
	popq	%r15
	popq	%rbp
	retq
                                        ## -- End function
	.globl	_mcl_fpDbl_sqrPre4Lbmi2         ## -- Begin function mcl_fpDbl_sqrPre4Lbmi2
	.p2align	4, 0x90
_mcl_fpDbl_sqrPre4Lbmi2:                ## @mcl_fpDbl_sqrPre4Lbmi2
## %bb.0:
	pushq	%rbp
	pushq	%r15
	pushq	%r14
	pushq	%r13
	pushq	%r12
	pushq	%rbx
	movq	24(%rsi), %r8
	movq	(%rsi), %rax
	movq	8(%rsi), %rcx
	movq	%r8, %rdx
	movq	%r8, -64(%rsp)                  ## 8-byte Spill
	mulxq	%rcx, %r14, %r9
	movq	%r14, -8(%rsp)                  ## 8-byte Spill
	movq	16(%rsi), %r12
	movq	%r12, %rdx
	mulxq	%rcx, %rbp, %rsi
	movq	%rbp, -40(%rsp)                 ## 8-byte Spill
	movq	%rsi, -24(%rsp)                 ## 8-byte Spill
	movq	%rcx, %rdx
	mulxq	%rcx, %r10, %r11
	mulxq	%rax, %r15, %rbx
	movq	%r15, -56(%rsp)                 ## 8-byte Spill
	addq	%rbx, %r10
	adcq	%rbp, %r11
	movq	%rsi, %rbp
	adcq	%r14, %rbp
	movq	%r9, %r14
	adcq	$0, %r14
	movq	%rax, %rdx
	mulxq	%rax, %rcx, %rsi
	movq	%rcx, -48(%rsp)                 ## 8-byte Spill
	addq	%r15, %rsi
	movq	%r12, %rdx
	mulxq	%rax, %rdx, %rcx
	movq	%rdx, -32(%rsp)                 ## 8-byte Spill
	adcq	%rdx, %rbx
	movq	%r8, %rdx
	mulxq	%rax, %rax, %r15
	movq	%rax, -16(%rsp)                 ## 8-byte Spill
	movq	%rcx, %r8
	adcq	%rax, %r8
	movq	%r15, %r13
	adcq	$0, %r13
	addq	-56(%rsp), %rsi                 ## 8-byte Folded Reload
	adcq	%r10, %rbx
	adcq	%r11, %r8
	adcq	%rbp, %r13
	adcq	$0, %r14
	addq	-40(%rsp), %rcx                 ## 8-byte Folded Reload
	movq	%r12, %rdx
	mulxq	%r12, %rbp, %r11
	adcq	-24(%rsp), %rbp                 ## 8-byte Folded Reload
	movq	-48(%rsp), %rax                 ## 8-byte Reload
	movq	%rax, (%rdi)
	movq	-64(%rsp), %rdx                 ## 8-byte Reload
	mulxq	%r12, %rdx, %r10
	adcq	%rdx, %r11
	movq	%r10, %rax
	adcq	$0, %rax
	addq	-32(%rsp), %rbx                 ## 8-byte Folded Reload
	movq	%rsi, 8(%rdi)
	adcq	%r8, %rcx
	movq	%rbx, 16(%rdi)
	adcq	%r13, %rbp
	adcq	%r14, %r11
	adcq	$0, %rax
	addq	-8(%rsp), %r15                  ## 8-byte Folded Reload
	adcq	%rdx, %r9
	movq	-64(%rsp), %rdx                 ## 8-byte Reload
	mulxq	%rdx, %rdx, %rsi
	adcq	%r10, %rdx
	adcq	$0, %rsi
	addq	-16(%rsp), %rcx                 ## 8-byte Folded Reload
	movq	%rcx, 24(%rdi)
	adcq	%rbp, %r15
	movq	%r15, 32(%rdi)
	adcq	%r11, %r9
	movq	%r9, 40(%rdi)
	adcq	%rax, %rdx
	movq	%rdx, 48(%rdi)
	adcq	$0, %rsi
	movq	%rsi, 56(%rdi)
	popq	%rbx
	popq	%r12
	popq	%r13
	popq	%r14
	popq	%r15
	popq	%rbp
	retq
                                        ## -- End function
	.globl	_mcl_fp_mont4Lbmi2              ## -- Begin function mcl_fp_mont4Lbmi2
	.p2align	4, 0x90
_mcl_fp_mont4Lbmi2:                     ## @mcl_fp_mont4Lbmi2
## %bb.0:
	pushq	%rbp
	pushq	%r15
	pushq	%r14
	pushq	%r13
	pushq	%r12
	pushq	%rbx
	movq	%rdx, -32(%rsp)                 ## 8-byte Spill
	movq	%rdi, -8(%rsp)                  ## 8-byte Spill
	movq	24(%rsi), %rdi
	movq	%rdi, -48(%rsp)                 ## 8-byte Spill
	movq	(%rdx), %rax
	movq	%rdi, %rdx
	mulxq	%rax, %r14, %r11
	movq	16(%rsi), %rdx
	movq	%rdx, -88(%rsp)                 ## 8-byte Spill
	mulxq	%rax, %rbx, %r10
	movq	(%rsi), %r12
	movq	8(%rsi), %rdx
	movq	%rdx, -40(%rsp)                 ## 8-byte Spill
	mulxq	%rax, %rdi, %r8
	movq	%r12, %rdx
	movq	%r12, -16(%rsp)                 ## 8-byte Spill
	mulxq	%rax, %r15, %r13
	addq	%rdi, %r13
	adcq	%rbx, %r8
	adcq	%r14, %r10
	adcq	$0, %r11
	movq	-8(%rcx), %rdx
	movq	%rdx, -64(%rsp)                 ## 8-byte Spill
	imulq	%r15, %rdx
	movq	24(%rcx), %rax
	movq	%rax, -24(%rsp)                 ## 8-byte Spill
	mulxq	%rax, %r14, %rbx
	movq	16(%rcx), %rax
	movq	%rax, -80(%rsp)                 ## 8-byte Spill
	mulxq	%rax, %r9, %rdi
	movq	(%rcx), %rbp
	movq	%rbp, -72(%rsp)                 ## 8-byte Spill
	movq	8(%rcx), %rax
	movq	%rax, -56(%rsp)                 ## 8-byte Spill
	mulxq	%rax, %rsi, %rcx
	mulxq	%rbp, %rdx, %rax
	addq	%rsi, %rax
	adcq	%r9, %rcx
	adcq	%r14, %rdi
	adcq	$0, %rbx
	addq	%r15, %rdx
	adcq	%r13, %rax
	adcq	%r8, %rcx
	adcq	%r10, %rdi
	adcq	%r11, %rbx
	movq	-32(%rsp), %r13                 ## 8-byte Reload
	movq	8(%r13), %rdx
	mulxq	-48(%rsp), %r11, %r10           ## 8-byte Folded Reload
	mulxq	-88(%rsp), %r14, %rbp           ## 8-byte Folded Reload
	mulxq	-40(%rsp), %r15, %r8            ## 8-byte Folded Reload
	mulxq	%r12, %r9, %rsi
	setb	%dl
	addq	%r15, %rsi
	adcq	%r14, %r8
	adcq	%r11, %rbp
	adcq	$0, %r10
	addq	%rax, %r9
	adcq	%rcx, %rsi
	adcq	%rdi, %r8
	adcq	%rbx, %rbp
	movzbl	%dl, %eax
	adcq	%rax, %r10
	setb	-89(%rsp)                       ## 1-byte Folded Spill
	movq	-64(%rsp), %rdx                 ## 8-byte Reload
	imulq	%r9, %rdx
	movq	-24(%rsp), %r12                 ## 8-byte Reload
	mulxq	%r12, %r14, %rbx
	mulxq	-80(%rsp), %r15, %rcx           ## 8-byte Folded Reload
	mulxq	-56(%rsp), %r11, %rdi           ## 8-byte Folded Reload
	mulxq	-72(%rsp), %rdx, %rax           ## 8-byte Folded Reload
	addq	%r11, %rax
	adcq	%r15, %rdi
	adcq	%r14, %rcx
	adcq	$0, %rbx
	addq	%r9, %rdx
	adcq	%rsi, %rax
	adcq	%r8, %rdi
	adcq	%rbp, %rcx
	adcq	%r10, %rbx
	movzbl	-89(%rsp), %r11d                ## 1-byte Folded Reload
	adcq	$0, %r11
	movq	16(%r13), %rdx
	mulxq	-48(%rsp), %r14, %r8            ## 8-byte Folded Reload
	mulxq	-88(%rsp), %r15, %r10           ## 8-byte Folded Reload
	mulxq	-40(%rsp), %r13, %rbp           ## 8-byte Folded Reload
	mulxq	-16(%rsp), %r9, %rsi            ## 8-byte Folded Reload
	addq	%r13, %rsi
	adcq	%r15, %rbp
	adcq	%r14, %r10
	adcq	$0, %r8
	addq	%rax, %r9
	adcq	%rdi, %rsi
	adcq	%rcx, %rbp
	adcq	%rbx, %r10
	adcq	%r11, %r8
	setb	%r11b
	movq	-64(%rsp), %rdx                 ## 8-byte Reload
	imulq	%r9, %rdx
	mulxq	%r12, %r14, %rbx
	mulxq	-80(%rsp), %r15, %rcx           ## 8-byte Folded Reload
	movq	-56(%rsp), %r12                 ## 8-byte Reload
	mulxq	%r12, %r13, %rdi
	mulxq	-72(%rsp), %rdx, %rax           ## 8-byte Folded Reload
	addq	%r13, %rax
	adcq	%r15, %rdi
	adcq	%r14, %rcx
	adcq	$0, %rbx
	addq	%r9, %rdx
	adcq	%rsi, %rax
	adcq	%rbp, %rdi
	adcq	%r10, %rcx
	adcq	%r8, %rbx
	movzbl	%r11b, %r11d
	adcq	$0, %r11
	movq	-32(%rsp), %rdx                 ## 8-byte Reload
	movq	24(%rdx), %rdx
	mulxq	-48(%rsp), %r14, %r8            ## 8-byte Folded Reload
	mulxq	-88(%rsp), %r15, %r9            ## 8-byte Folded Reload
	mulxq	-40(%rsp), %r13, %rbp           ## 8-byte Folded Reload
	mulxq	-16(%rsp), %r10, %rsi           ## 8-byte Folded Reload
	addq	%r13, %rsi
	adcq	%r15, %rbp
	adcq	%r14, %r9
	adcq	$0, %r8
	addq	%rax, %r10
	adcq	%rdi, %rsi
	adcq	%rcx, %rbp
	adcq	%rbx, %r9
	adcq	%r11, %r8
	setb	-88(%rsp)                       ## 1-byte Folded Spill
	movq	-64(%rsp), %rdx                 ## 8-byte Reload
	imulq	%r10, %rdx
	movq	-72(%rsp), %rcx                 ## 8-byte Reload
	mulxq	%rcx, %rdi, %rax
	mulxq	%r12, %r13, %r14
	addq	%rax, %r13
	mulxq	-80(%rsp), %rbx, %r15           ## 8-byte Folded Reload
	adcq	%r14, %rbx
	movq	-24(%rsp), %r11                 ## 8-byte Reload
	mulxq	%r11, %r14, %r12
	adcq	%r15, %r14
	adcq	$0, %r12
	addq	%r10, %rdi
	adcq	%rsi, %r13
	adcq	%rbp, %rbx
	adcq	%r9, %r14
	movzbl	-88(%rsp), %esi                 ## 1-byte Folded Reload
	adcq	%r8, %r12
	adcq	$0, %rsi
	movq	%r13, %rdi
	subq	%rcx, %rdi
	movq	%rbx, %rcx
	sbbq	-56(%rsp), %rcx                 ## 8-byte Folded Reload
	movq	%r14, %rax
	sbbq	-80(%rsp), %rax                 ## 8-byte Folded Reload
	movq	%r12, %rdx
	sbbq	%r11, %rdx
	sbbq	$0, %rsi
	testb	$1, %sil
	cmovneq	%r12, %rdx
	movq	-8(%rsp), %rsi                  ## 8-byte Reload
	movq	%rdx, 24(%rsi)
	cmovneq	%r14, %rax
	movq	%rax, 16(%rsi)
	cmovneq	%rbx, %rcx
	movq	%rcx, 8(%rsi)
	cmovneq	%r13, %rdi
	movq	%rdi, (%rsi)
	popq	%rbx
	popq	%r12
	popq	%r13
	popq	%r14
	popq	%r15
	popq	%rbp
	retq
                                        ## -- End function
	.globl	_mcl_fp_montNF4Lbmi2            ## -- Begin function mcl_fp_montNF4Lbmi2
	.p2align	4, 0x90
_mcl_fp_montNF4Lbmi2:                   ## @mcl_fp_montNF4Lbmi2
## %bb.0:
	pushq	%rbp
	pushq	%r15
	pushq	%r14
	pushq	%r13
	pushq	%r12
	pushq	%rbx
	movq	%rdi, -8(%rsp)                  ## 8-byte Spill
	movq	(%rsi), %rdi
	movq	%rdi, -56(%rsp)                 ## 8-byte Spill
	movq	8(%rsi), %rbp
	movq	%rbp, -64(%rsp)                 ## 8-byte Spill
	movq	(%rdx), %rax
	movq	%rdx, %r15
	movq	%rdx, -24(%rsp)                 ## 8-byte Spill
	movq	%rbp, %rdx
	mulxq	%rax, %rbp, %r9
	movq	%rdi, %rdx
	mulxq	%rax, %r12, %rbx
	movq	16(%rsi), %rdx
	movq	%rdx, -40(%rsp)                 ## 8-byte Spill
	addq	%rbp, %rbx
	mulxq	%rax, %r14, %rbp
	adcq	%r9, %r14
	movq	24(%rsi), %rdx
	movq	%rdx, -80(%rsp)                 ## 8-byte Spill
	mulxq	%rax, %r8, %rdi
	adcq	%rbp, %r8
	adcq	$0, %rdi
	movq	-8(%rcx), %r13
	movq	(%rcx), %rax
	movq	%rax, -48(%rsp)                 ## 8-byte Spill
	movq	%r13, %rdx
	imulq	%r12, %rdx
	mulxq	%rax, %rax, %r11
	addq	%r12, %rax
	movq	8(%rcx), %rax
	movq	%rax, -16(%rsp)                 ## 8-byte Spill
	mulxq	%rax, %rbp, %r10
	adcq	%rbx, %rbp
	movq	16(%rcx), %rax
	movq	%rax, -32(%rsp)                 ## 8-byte Spill
	mulxq	%rax, %rsi, %rbx
	adcq	%r14, %rsi
	movq	24(%rcx), %rax
	movq	%rax, -72(%rsp)                 ## 8-byte Spill
	mulxq	%rax, %rcx, %rdx
	adcq	%r8, %rcx
	adcq	$0, %rdi
	addq	%r11, %rbp
	adcq	%r10, %rsi
	adcq	%rbx, %rcx
	adcq	%rdx, %rdi
	movq	8(%r15), %rdx
	movq	-64(%rsp), %r12                 ## 8-byte Reload
	mulxq	%r12, %rbx, %r9
	movq	-56(%rsp), %r15                 ## 8-byte Reload
	mulxq	%r15, %r10, %r11
	addq	%rbx, %r11
	mulxq	-40(%rsp), %rax, %r8            ## 8-byte Folded Reload
	adcq	%r9, %rax
	mulxq	-80(%rsp), %r9, %rbx            ## 8-byte Folded Reload
	adcq	%r8, %r9
	adcq	$0, %rbx
	addq	%rbp, %r10
	adcq	%rsi, %r11
	adcq	%rcx, %rax
	adcq	%rdi, %r9
	adcq	$0, %rbx
	movq	%r13, %rdx
	imulq	%r10, %rdx
	movq	-48(%rsp), %r14                 ## 8-byte Reload
	mulxq	%r14, %rcx, %r8
	addq	%r10, %rcx
	mulxq	-16(%rsp), %r10, %rdi           ## 8-byte Folded Reload
	adcq	%r11, %r10
	mulxq	-32(%rsp), %rcx, %rsi           ## 8-byte Folded Reload
	adcq	%rax, %rcx
	mulxq	-72(%rsp), %rax, %rdx           ## 8-byte Folded Reload
	adcq	%r9, %rax
	adcq	$0, %rbx
	addq	%r8, %r10
	adcq	%rdi, %rcx
	adcq	%rsi, %rax
	adcq	%rdx, %rbx
	movq	-24(%rsp), %rdx                 ## 8-byte Reload
	movq	16(%rdx), %rdx
	mulxq	%r12, %rsi, %r8
	mulxq	%r15, %r11, %rbp
	addq	%rsi, %rbp
	movq	-40(%rsp), %r12                 ## 8-byte Reload
	mulxq	%r12, %rdi, %r9
	adcq	%r8, %rdi
	mulxq	-80(%rsp), %r8, %rsi            ## 8-byte Folded Reload
	adcq	%r9, %r8
	adcq	$0, %rsi
	addq	%r10, %r11
	adcq	%rcx, %rbp
	adcq	%rax, %rdi
	adcq	%rbx, %r8
	adcq	$0, %rsi
	movq	%r13, %rdx
	imulq	%r11, %rdx
	mulxq	%r14, %rax, %r10
	addq	%r11, %rax
	movq	-16(%rsp), %r14                 ## 8-byte Reload
	mulxq	%r14, %r9, %rbx
	adcq	%rbp, %r9
	movq	-32(%rsp), %r15                 ## 8-byte Reload
	mulxq	%r15, %rax, %rbp
	adcq	%rdi, %rax
	mulxq	-72(%rsp), %rcx, %rdx           ## 8-byte Folded Reload
	adcq	%r8, %rcx
	adcq	$0, %rsi
	addq	%r10, %r9
	adcq	%rbx, %rax
	adcq	%rbp, %rcx
	adcq	%rdx, %rsi
	movq	-24(%rsp), %rdx                 ## 8-byte Reload
	movq	24(%rdx), %rdx
	mulxq	-64(%rsp), %rbx, %r8            ## 8-byte Folded Reload
	mulxq	-56(%rsp), %r11, %rbp           ## 8-byte Folded Reload
	addq	%rbx, %rbp
	mulxq	%r12, %rdi, %rbx
	adcq	%r8, %rdi
	mulxq	-80(%rsp), %r8, %r10            ## 8-byte Folded Reload
	adcq	%rbx, %r8
	adcq	$0, %r10
	addq	%r9, %r11
	adcq	%rax, %rbp
	adcq	%rcx, %rdi
	adcq	%rsi, %r8
	adcq	$0, %r10
	imulq	%r11, %r13
	movq	%r13, %rdx
	movq	-48(%rsp), %rbx                 ## 8-byte Reload
	mulxq	%rbx, %rcx, %r9
	addq	%r11, %rcx
	mulxq	%r14, %r11, %r12
	adcq	%rbp, %r11
	mulxq	%r15, %rax, %rcx
	adcq	%rdi, %rax
	movq	-72(%rsp), %rsi                 ## 8-byte Reload
	mulxq	%rsi, %rbp, %rdx
	adcq	%r8, %rbp
	adcq	$0, %r10
	addq	%r9, %r11
	adcq	%r12, %rax
	adcq	%rcx, %rbp
	adcq	%rdx, %r10
	movq	%r11, %rcx
	subq	%rbx, %rcx
	movq	%rax, %rdx
	sbbq	%r14, %rdx
	movq	%rbp, %rdi
	sbbq	%r15, %rdi
	movq	%r10, %rbx
	sbbq	%rsi, %rbx
	cmovsq	%r10, %rbx
	movq	-8(%rsp), %rsi                  ## 8-byte Reload
	movq	%rbx, 24(%rsi)
	cmovsq	%rbp, %rdi
	movq	%rdi, 16(%rsi)
	cmovsq	%rax, %rdx
	movq	%rdx, 8(%rsi)
	cmovsq	%r11, %rcx
	movq	%rcx, (%rsi)
	popq	%rbx
	popq	%r12
	popq	%r13
	popq	%r14
	popq	%r15
	popq	%rbp
	retq
                                        ## -- End function
	.globl	_mcl_fp_montRed4Lbmi2           ## -- Begin function mcl_fp_montRed4Lbmi2
	.p2align	4, 0x90
_mcl_fp_montRed4Lbmi2:                  ## @mcl_fp_montRed4Lbmi2
## %bb.0:
	pushq	%rbp
	pushq	%r15
	pushq	%r14
	pushq	%r13
	pushq	%r12
	pushq	%rbx
	movq	%rdx, %rcx
	movq	%rdi, -8(%rsp)                  ## 8-byte Spill
	movq	-8(%rdx), %r15
	movq	(%rdx), %rdi
	movq	%rdi, -64(%rsp)                 ## 8-byte Spill
	movq	(%rsi), %rax
	movq	%rax, %rdx
	imulq	%r15, %rdx
	movq	24(%rcx), %rbp
	mulxq	%rbp, %r12, %r11
	movq	%rbp, %r8
	movq	%rbp, -40(%rsp)                 ## 8-byte Spill
	movq	16(%rcx), %r9
	mulxq	%r9, %r10, %r13
	movq	8(%rcx), %rcx
	movq	%rcx, -56(%rsp)                 ## 8-byte Spill
	mulxq	%rcx, %rcx, %rbx
	mulxq	%rdi, %rdx, %rbp
	addq	%rcx, %rbp
	adcq	%r10, %rbx
	adcq	%r12, %r13
	adcq	$0, %r11
	addq	%rax, %rdx
	movq	%rsi, -48(%rsp)                 ## 8-byte Spill
	adcq	8(%rsi), %rbp
	adcq	16(%rsi), %rbx
	adcq	24(%rsi), %r13
	adcq	32(%rsi), %r11
	setb	-65(%rsp)                       ## 1-byte Folded Spill
	movq	%r15, %rdx
	imulq	%rbp, %rdx
	mulxq	%r8, %r14, %r12
	movq	%r9, -16(%rsp)                  ## 8-byte Spill
	mulxq	%r9, %r10, %rsi
	mulxq	-64(%rsp), %rdi, %r8            ## 8-byte Folded Reload
	mulxq	-56(%rsp), %rax, %rcx           ## 8-byte Folded Reload
	addq	%r8, %rax
	adcq	%r10, %rcx
	adcq	%r14, %rsi
	movzbl	-65(%rsp), %edx                 ## 1-byte Folded Reload
	adcq	%rdx, %r12
	addq	%rbp, %rdi
	adcq	%rbx, %rax
	adcq	%r13, %rcx
	adcq	%r11, %rsi
	movq	-48(%rsp), %r10                 ## 8-byte Reload
	adcq	40(%r10), %r12
	setb	-65(%rsp)                       ## 1-byte Folded Spill
	movq	%r15, %rdx
	imulq	%rax, %rdx
	mulxq	-40(%rsp), %rdi, %r11           ## 8-byte Folded Reload
	movq	%rdi, -24(%rsp)                 ## 8-byte Spill
	mulxq	%r9, %rdi, %r13
	movq	%rdi, -32(%rsp)                 ## 8-byte Spill
	movq	-64(%rsp), %r8                  ## 8-byte Reload
	mulxq	%r8, %rdi, %r14
	movq	-56(%rsp), %r9                  ## 8-byte Reload
	mulxq	%r9, %rbp, %rbx
	addq	%r14, %rbp
	adcq	-32(%rsp), %rbx                 ## 8-byte Folded Reload
	adcq	-24(%rsp), %r13                 ## 8-byte Folded Reload
	movzbl	-65(%rsp), %edx                 ## 1-byte Folded Reload
	adcq	%rdx, %r11
	addq	%rax, %rdi
	adcq	%rcx, %rbp
	adcq	%rsi, %rbx
	adcq	%r12, %r13
	adcq	48(%r10), %r11
	setb	%dil
	imulq	%rbp, %r15
	movq	%r15, %rdx
	mulxq	%r8, %rcx, %rax
	mulxq	%r9, %r12, %rsi
	addq	%rax, %r12
	movq	-16(%rsp), %r8                  ## 8-byte Reload
	mulxq	%r8, %rax, %r9
	adcq	%rsi, %rax
	movq	-40(%rsp), %r10                 ## 8-byte Reload
	mulxq	%r10, %r15, %r14
	adcq	%r9, %r15
	movzbl	%dil, %edi
	adcq	%r14, %rdi
	addq	%rbp, %rcx
	adcq	%rbx, %r12
	adcq	%r13, %rax
	adcq	%r11, %r15
	movq	-48(%rsp), %rcx                 ## 8-byte Reload
	adcq	56(%rcx), %rdi
	xorl	%ebx, %ebx
	movq	%r12, %rcx
	subq	-64(%rsp), %rcx                 ## 8-byte Folded Reload
	movq	%rax, %rbp
	sbbq	-56(%rsp), %rbp                 ## 8-byte Folded Reload
	movq	%r15, %rdx
	sbbq	%r8, %rdx
	movq	%rdi, %rsi
	sbbq	%r10, %rsi
	sbbq	%rbx, %rbx
	testb	$1, %bl
	cmovneq	%rdi, %rsi
	movq	-8(%rsp), %rdi                  ## 8-byte Reload
	movq	%rsi, 24(%rdi)
	cmovneq	%r15, %rdx
	movq	%rdx, 16(%rdi)
	cmovneq	%rax, %rbp
	movq	%rbp, 8(%rdi)
	cmovneq	%r12, %rcx
	movq	%rcx, (%rdi)
	popq	%rbx
	popq	%r12
	popq	%r13
	popq	%r14
	popq	%r15
	popq	%rbp
	retq
                                        ## -- End function
	.globl	_mcl_fp_montRedNF4Lbmi2         ## -- Begin function mcl_fp_montRedNF4Lbmi2
	.p2align	4, 0x90
_mcl_fp_montRedNF4Lbmi2:                ## @mcl_fp_montRedNF4Lbmi2
## %bb.0:
	pushq	%rbp
	pushq	%r15
	pushq	%r14
	pushq	%r13
	pushq	%r12
	pushq	%rbx
	movq	%rdx, %rcx
	movq	%rdi, -8(%rsp)                  ## 8-byte Spill
	movq	-8(%rdx), %r15
	movq	(%rdx), %rdi
	movq	(%rsi), %rax
	movq	%rax, %rdx
	imulq	%r15, %rdx
	movq	24(%rcx), %rbp
	mulxq	%rbp, %r12, %r11
	movq	%rbp, %r14
	movq	%rbp, -32(%rsp)                 ## 8-byte Spill
	movq	16(%rcx), %r8
	mulxq	%r8, %r9, %r13
	movq	%r8, -40(%rsp)                  ## 8-byte Spill
	movq	8(%rcx), %rcx
	movq	%rcx, -64(%rsp)                 ## 8-byte Spill
	mulxq	%rcx, %rbp, %rbx
	mulxq	%rdi, %rdx, %rcx
	movq	%rdi, -56(%rsp)                 ## 8-byte Spill
	addq	%rbp, %rcx
	adcq	%r9, %rbx
	adcq	%r12, %r13
	adcq	$0, %r11
	addq	%rax, %rdx
	movq	%rsi, -48(%rsp)                 ## 8-byte Spill
	adcq	8(%rsi), %rcx
	adcq	16(%rsi), %rbx
	adcq	24(%rsi), %r13
	adcq	32(%rsi), %r11
	setb	%r10b
	movq	%r15, %rdx
	imulq	%rcx, %rdx
	mulxq	%r14, %r14, %r12
	mulxq	%r8, %r9, %rbp
	mulxq	%rdi, %rdi, %r8
	mulxq	-64(%rsp), %rax, %rsi           ## 8-byte Folded Reload
	addq	%r8, %rax
	adcq	%r9, %rsi
	adcq	%r14, %rbp
	movzbl	%r10b, %edx
	adcq	%rdx, %r12
	addq	%rcx, %rdi
	adcq	%rbx, %rax
	adcq	%r13, %rsi
	adcq	%r11, %rbp
	movq	-48(%rsp), %r10                 ## 8-byte Reload
	adcq	40(%r10), %r12
	setb	-65(%rsp)                       ## 1-byte Folded Spill
	movq	%r15, %rdx
	imulq	%rax, %rdx
	mulxq	-32(%rsp), %rcx, %r11           ## 8-byte Folded Reload
	movq	%rcx, -16(%rsp)                 ## 8-byte Spill
	mulxq	-40(%rsp), %rcx, %r13           ## 8-byte Folded Reload
	movq	%rcx, -24(%rsp)                 ## 8-byte Spill
	movq	-56(%rsp), %r9                  ## 8-byte Reload
	mulxq	%r9, %rdi, %r14
	movq	-64(%rsp), %r8                  ## 8-byte Reload
	mulxq	%r8, %rbx, %rcx
	addq	%r14, %rbx
	adcq	-24(%rsp), %rcx                 ## 8-byte Folded Reload
	adcq	-16(%rsp), %r13                 ## 8-byte Folded Reload
	movzbl	-65(%rsp), %edx                 ## 1-byte Folded Reload
	adcq	%rdx, %r11
	addq	%rax, %rdi
	adcq	%rsi, %rbx
	adcq	%rbp, %rcx
	adcq	%r12, %r13
	adcq	48(%r10), %r11
	setb	%al
	imulq	%rbx, %r15
	movq	%r15, %rdx
	mulxq	%r9, %rsi, %rbp
	mulxq	%r8, %r12, %rdi
	addq	%rbp, %r12
	movq	-40(%rsp), %r8                  ## 8-byte Reload
	mulxq	%r8, %rbp, %r9
	adcq	%rdi, %rbp
	movq	-32(%rsp), %r10                 ## 8-byte Reload
	mulxq	%r10, %r15, %r14
	adcq	%r9, %r15
	movzbl	%al, %eax
	adcq	%r14, %rax
	addq	%rbx, %rsi
	adcq	%rcx, %r12
	adcq	%r13, %rbp
	adcq	%r11, %r15
	movq	-48(%rsp), %rcx                 ## 8-byte Reload
	adcq	56(%rcx), %rax
	movq	%r12, %rcx
	subq	-56(%rsp), %rcx                 ## 8-byte Folded Reload
	movq	%rbp, %rsi
	sbbq	-64(%rsp), %rsi                 ## 8-byte Folded Reload
	movq	%r15, %rdi
	sbbq	%r8, %rdi
	movq	%rax, %rdx
	sbbq	%r10, %rdx
	cmovsq	%rax, %rdx
	movq	-8(%rsp), %rax                  ## 8-byte Reload
	movq	%rdx, 24(%rax)
	cmovsq	%r15, %rdi
	movq	%rdi, 16(%rax)
	cmovsq	%rbp, %rsi
	movq	%rsi, 8(%rax)
	cmovsq	%r12, %rcx
	movq	%rcx, (%rax)
	popq	%rbx
	popq	%r12
	popq	%r13
	popq	%r14
	popq	%r15
	popq	%rbp
	retq
                                        ## -- End function
	.globl	_mcl_fp_addPre4Lbmi2            ## -- Begin function mcl_fp_addPre4Lbmi2
	.p2align	4, 0x90
_mcl_fp_addPre4Lbmi2:                   ## @mcl_fp_addPre4Lbmi2
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
	.globl	_mcl_fp_subPre4Lbmi2            ## -- Begin function mcl_fp_subPre4Lbmi2
	.p2align	4, 0x90
_mcl_fp_subPre4Lbmi2:                   ## @mcl_fp_subPre4Lbmi2
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
	.globl	_mcl_fp_shr1_4Lbmi2             ## -- Begin function mcl_fp_shr1_4Lbmi2
	.p2align	4, 0x90
_mcl_fp_shr1_4Lbmi2:                    ## @mcl_fp_shr1_4Lbmi2
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
	.globl	_mcl_fp_add4Lbmi2               ## -- Begin function mcl_fp_add4Lbmi2
	.p2align	4, 0x90
_mcl_fp_add4Lbmi2:                      ## @mcl_fp_add4Lbmi2
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
	.globl	_mcl_fp_addNF4Lbmi2             ## -- Begin function mcl_fp_addNF4Lbmi2
	.p2align	4, 0x90
_mcl_fp_addNF4Lbmi2:                    ## @mcl_fp_addNF4Lbmi2
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
	.globl	_mcl_fp_sub4Lbmi2               ## -- Begin function mcl_fp_sub4Lbmi2
	.p2align	4, 0x90
_mcl_fp_sub4Lbmi2:                      ## @mcl_fp_sub4Lbmi2
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
	.globl	_mcl_fp_subNF4Lbmi2             ## -- Begin function mcl_fp_subNF4Lbmi2
	.p2align	4, 0x90
_mcl_fp_subNF4Lbmi2:                    ## @mcl_fp_subNF4Lbmi2
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
	.globl	_mcl_fpDbl_add4Lbmi2            ## -- Begin function mcl_fpDbl_add4Lbmi2
	.p2align	4, 0x90
_mcl_fpDbl_add4Lbmi2:                   ## @mcl_fpDbl_add4Lbmi2
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
	.globl	_mcl_fpDbl_sub4Lbmi2            ## -- Begin function mcl_fpDbl_sub4Lbmi2
	.p2align	4, 0x90
_mcl_fpDbl_sub4Lbmi2:                   ## @mcl_fpDbl_sub4Lbmi2
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
	.globl	_mulPv384x64bmi2                ## -- Begin function mulPv384x64bmi2
	.p2align	4, 0x90
_mulPv384x64bmi2:                       ## @mulPv384x64bmi2
## %bb.0:
	movq	%rdi, %rax
	mulxq	(%rsi), %rdi, %rcx
	movq	%rdi, (%rax)
	mulxq	8(%rsi), %rdi, %r8
	addq	%rcx, %rdi
	movq	%rdi, 8(%rax)
	mulxq	16(%rsi), %rdi, %r9
	adcq	%r8, %rdi
	movq	%rdi, 16(%rax)
	mulxq	24(%rsi), %rcx, %rdi
	adcq	%r9, %rcx
	movq	%rcx, 24(%rax)
	mulxq	32(%rsi), %rcx, %r8
	adcq	%rdi, %rcx
	movq	%rcx, 32(%rax)
	mulxq	40(%rsi), %rcx, %rdx
	adcq	%r8, %rcx
	movq	%rcx, 40(%rax)
	adcq	$0, %rdx
	movq	%rdx, 48(%rax)
	retq
                                        ## -- End function
	.globl	_mcl_fp_mulUnitPre6Lbmi2        ## -- Begin function mcl_fp_mulUnitPre6Lbmi2
	.p2align	4, 0x90
_mcl_fp_mulUnitPre6Lbmi2:               ## @mcl_fp_mulUnitPre6Lbmi2
## %bb.0:
	pushq	%r15
	pushq	%r14
	pushq	%r12
	pushq	%rbx
	mulxq	40(%rsi), %r8, %r11
	mulxq	32(%rsi), %r9, %r12
	mulxq	24(%rsi), %r10, %rcx
	mulxq	16(%rsi), %r14, %rbx
	mulxq	8(%rsi), %r15, %rax
	mulxq	(%rsi), %rdx, %rsi
	movq	%rdx, (%rdi)
	addq	%r15, %rsi
	movq	%rsi, 8(%rdi)
	adcq	%r14, %rax
	movq	%rax, 16(%rdi)
	adcq	%r10, %rbx
	movq	%rbx, 24(%rdi)
	adcq	%r9, %rcx
	movq	%rcx, 32(%rdi)
	adcq	%r8, %r12
	movq	%r12, 40(%rdi)
	adcq	$0, %r11
	movq	%r11, 48(%rdi)
	popq	%rbx
	popq	%r12
	popq	%r14
	popq	%r15
	retq
                                        ## -- End function
	.globl	_mcl_fpDbl_mulPre6Lbmi2         ## -- Begin function mcl_fpDbl_mulPre6Lbmi2
	.p2align	4, 0x90
_mcl_fpDbl_mulPre6Lbmi2:                ## @mcl_fpDbl_mulPre6Lbmi2
## %bb.0:
	pushq	%rbp
	pushq	%r15
	pushq	%r14
	pushq	%r13
	pushq	%r12
	pushq	%rbx
	movq	(%rsi), %r9
	movq	8(%rsi), %r13
	movq	(%rdx), %rcx
	movq	%rdx, %r12
	movq	%rdx, -40(%rsp)                 ## 8-byte Spill
	movq	%r9, %rdx
	movq	%r9, -24(%rsp)                  ## 8-byte Spill
	mulxq	%rcx, %r8, %rax
	movq	%rax, -112(%rsp)                ## 8-byte Spill
	movq	16(%rsi), %rax
	movq	%rax, -32(%rsp)                 ## 8-byte Spill
	movq	24(%rsi), %rbx
	movq	%rbx, -80(%rsp)                 ## 8-byte Spill
	movq	32(%rsi), %rbp
	movq	%rbp, -72(%rsp)                 ## 8-byte Spill
	movq	40(%rsi), %rdx
	movq	%r8, (%rdi)
	movq	%rdi, %r15
	movq	%rdi, -16(%rsp)                 ## 8-byte Spill
	movq	%rdx, %r8
	movq	%rdx, -48(%rsp)                 ## 8-byte Spill
	mulxq	%rcx, %rdx, %rsi
	movq	%rdx, -104(%rsp)                ## 8-byte Spill
	movq	%rbp, %rdx
	mulxq	%rcx, %r10, %r14
	movq	%rbx, %rdx
	mulxq	%rcx, %r11, %rdi
	movq	%rax, %rdx
	mulxq	%rcx, %rbx, %rax
	movq	%r13, %rdx
	movq	%r13, -64(%rsp)                 ## 8-byte Spill
	mulxq	%rcx, %rcx, %rbp
	addq	-112(%rsp), %rcx                ## 8-byte Folded Reload
	adcq	%rbx, %rbp
	adcq	%r11, %rax
	adcq	%r10, %rdi
	adcq	-104(%rsp), %r14                ## 8-byte Folded Reload
	adcq	$0, %rsi
	movq	%rsi, -96(%rsp)                 ## 8-byte Spill
	movq	8(%r12), %rdx
	mulxq	%r9, %rbx, %rsi
	movq	%rsi, -88(%rsp)                 ## 8-byte Spill
	addq	%rcx, %rbx
	movq	%rbx, 8(%r15)
	mulxq	%r8, %r10, %rcx
	movq	%rcx, -104(%rsp)                ## 8-byte Spill
	movq	-72(%rsp), %rcx                 ## 8-byte Reload
	mulxq	%rcx, %r9, %rbx
	movq	%rbx, -112(%rsp)                ## 8-byte Spill
	movq	-80(%rsp), %r12                 ## 8-byte Reload
	mulxq	%r12, %r11, %rsi
	mulxq	-32(%rsp), %r8, %r15            ## 8-byte Folded Reload
	mulxq	%r13, %rbx, %rdx
	adcq	%rbp, %rbx
	adcq	%rax, %r8
	adcq	%rdi, %r11
	adcq	%r14, %r9
	adcq	-96(%rsp), %r10                 ## 8-byte Folded Reload
	setb	%al
	addq	-88(%rsp), %rbx                 ## 8-byte Folded Reload
	adcq	%rdx, %r8
	adcq	%r15, %r11
	adcq	%rsi, %r9
	adcq	-112(%rsp), %r10                ## 8-byte Folded Reload
	movzbl	%al, %r13d
	adcq	-104(%rsp), %r13                ## 8-byte Folded Reload
	movq	-40(%rsp), %r15                 ## 8-byte Reload
	movq	16(%r15), %rdx
	mulxq	-48(%rsp), %rsi, %rax           ## 8-byte Folded Reload
	movq	%rsi, -104(%rsp)                ## 8-byte Spill
	movq	%rax, -88(%rsp)                 ## 8-byte Spill
	mulxq	%rcx, %rax, %r14
	movq	%rax, -112(%rsp)                ## 8-byte Spill
	mulxq	%r12, %rax, %rbp
	movq	%rax, -96(%rsp)                 ## 8-byte Spill
	mulxq	-64(%rsp), %rcx, %r12           ## 8-byte Folded Reload
	mulxq	-24(%rsp), %rax, %rsi           ## 8-byte Folded Reload
	addq	%rcx, %rsi
	mulxq	-32(%rsp), %rcx, %rdi           ## 8-byte Folded Reload
	adcq	%r12, %rcx
	adcq	-96(%rsp), %rdi                 ## 8-byte Folded Reload
	adcq	-112(%rsp), %rbp                ## 8-byte Folded Reload
	adcq	-104(%rsp), %r14                ## 8-byte Folded Reload
	movq	-88(%rsp), %r12                 ## 8-byte Reload
	adcq	$0, %r12
	addq	%rbx, %rax
	movq	-16(%rsp), %rdx                 ## 8-byte Reload
	movq	%rax, 16(%rdx)
	adcq	%r8, %rsi
	adcq	%r11, %rcx
	adcq	%r9, %rdi
	adcq	%r10, %rbp
	adcq	%r13, %r14
	adcq	$0, %r12
	movq	%r12, -88(%rsp)                 ## 8-byte Spill
	movq	24(%r15), %rdx
	movq	-48(%rsp), %r15                 ## 8-byte Reload
	mulxq	%r15, %rbx, %rax
	movq	%rbx, -96(%rsp)                 ## 8-byte Spill
	movq	%rax, -104(%rsp)                ## 8-byte Spill
	mulxq	-72(%rsp), %rbx, %rax           ## 8-byte Folded Reload
	movq	%rbx, -56(%rsp)                 ## 8-byte Spill
	movq	%rax, -112(%rsp)                ## 8-byte Spill
	mulxq	-80(%rsp), %rax, %r11           ## 8-byte Folded Reload
	movq	%rax, -8(%rsp)                  ## 8-byte Spill
	mulxq	-64(%rsp), %r8, %r12            ## 8-byte Folded Reload
	mulxq	-24(%rsp), %rax, %rbx           ## 8-byte Folded Reload
	addq	%r8, %rbx
	movq	-32(%rsp), %r13                 ## 8-byte Reload
	mulxq	%r13, %r9, %r10
	adcq	%r12, %r9
	adcq	-8(%rsp), %r10                  ## 8-byte Folded Reload
	adcq	-56(%rsp), %r11                 ## 8-byte Folded Reload
	movq	-112(%rsp), %r8                 ## 8-byte Reload
	adcq	-96(%rsp), %r8                  ## 8-byte Folded Reload
	movq	-104(%rsp), %r12                ## 8-byte Reload
	adcq	$0, %r12
	addq	%rsi, %rax
	movq	-16(%rsp), %rdx                 ## 8-byte Reload
	movq	%rax, 24(%rdx)
	adcq	%rcx, %rbx
	adcq	%rdi, %r9
	adcq	%rbp, %r10
	adcq	%r14, %r11
	adcq	-88(%rsp), %r8                  ## 8-byte Folded Reload
	movq	%r8, -112(%rsp)                 ## 8-byte Spill
	adcq	$0, %r12
	movq	%r12, -104(%rsp)                ## 8-byte Spill
	movq	-40(%rsp), %rax                 ## 8-byte Reload
	movq	32(%rax), %rdx
	mulxq	%r15, %rcx, %rax
	movq	%rcx, -88(%rsp)                 ## 8-byte Spill
	mulxq	-72(%rsp), %rcx, %r14           ## 8-byte Folded Reload
	movq	%rcx, -96(%rsp)                 ## 8-byte Spill
	mulxq	-80(%rsp), %rcx, %rbp           ## 8-byte Folded Reload
	movq	%rcx, -56(%rsp)                 ## 8-byte Spill
	mulxq	-64(%rsp), %rdi, %r15           ## 8-byte Folded Reload
	movq	-24(%rsp), %r12                 ## 8-byte Reload
	mulxq	%r12, %rcx, %rsi
	addq	%rdi, %rsi
	mulxq	%r13, %rdi, %r8
	adcq	%r15, %rdi
	adcq	-56(%rsp), %r8                  ## 8-byte Folded Reload
	adcq	-96(%rsp), %rbp                 ## 8-byte Folded Reload
	adcq	-88(%rsp), %r14                 ## 8-byte Folded Reload
	adcq	$0, %rax
	addq	%rbx, %rcx
	movq	-16(%rsp), %r15                 ## 8-byte Reload
	movq	%rcx, 32(%r15)
	adcq	%r9, %rsi
	adcq	%r10, %rdi
	adcq	%r11, %r8
	adcq	-112(%rsp), %rbp                ## 8-byte Folded Reload
	movq	-40(%rsp), %rcx                 ## 8-byte Reload
	movq	40(%rcx), %rdx
	adcq	-104(%rsp), %r14                ## 8-byte Folded Reload
	mulxq	-64(%rsp), %rbx, %r9            ## 8-byte Folded Reload
	mulxq	%r12, %rcx, %r11
	adcq	$0, %rax
	addq	%rbx, %r11
	mulxq	%r13, %r12, %r10
	adcq	%r9, %r12
	mulxq	-80(%rsp), %r13, %r9            ## 8-byte Folded Reload
	adcq	%r10, %r13
	mulxq	-72(%rsp), %rbx, %r10           ## 8-byte Folded Reload
	adcq	%r9, %rbx
	mulxq	-48(%rsp), %rdx, %r9            ## 8-byte Folded Reload
	adcq	%r10, %rdx
	adcq	$0, %r9
	addq	%rcx, %rsi
	movq	%rsi, 40(%r15)
	adcq	%rdi, %r11
	movq	%r11, 48(%r15)
	adcq	%r8, %r12
	movq	%r12, 56(%r15)
	adcq	%rbp, %r13
	movq	%r13, 64(%r15)
	adcq	%r14, %rbx
	movq	%rbx, 72(%r15)
	adcq	%rax, %rdx
	movq	%rdx, 80(%r15)
	adcq	$0, %r9
	movq	%r9, 88(%r15)
	popq	%rbx
	popq	%r12
	popq	%r13
	popq	%r14
	popq	%r15
	popq	%rbp
	retq
                                        ## -- End function
	.globl	_mcl_fpDbl_sqrPre6Lbmi2         ## -- Begin function mcl_fpDbl_sqrPre6Lbmi2
	.p2align	4, 0x90
_mcl_fpDbl_sqrPre6Lbmi2:                ## @mcl_fpDbl_sqrPre6Lbmi2
## %bb.0:
	pushq	%rbp
	pushq	%r15
	pushq	%r14
	pushq	%r13
	pushq	%r12
	pushq	%rbx
	subq	$168, %rsp
	movq	%rdi, -48(%rsp)                 ## 8-byte Spill
	movq	40(%rsi), %rdx
	movq	32(%rsi), %rcx
	mulxq	%rcx, %rax, %rdi
	movq	%rdi, -104(%rsp)                ## 8-byte Spill
	movq	%rax, -128(%rsp)                ## 8-byte Spill
	movq	24(%rsi), %rax
	mulxq	%rax, %r14, %r13
	movq	%r14, -112(%rsp)                ## 8-byte Spill
	movq	%r13, -64(%rsp)                 ## 8-byte Spill
	movq	16(%rsi), %r10
	mulxq	%r10, %r8, %r11
	movq	%r8, 24(%rsp)                   ## 8-byte Spill
	movq	%r11, -88(%rsp)                 ## 8-byte Spill
	movq	(%rsi), %rdi
	movq	%rdi, -96(%rsp)                 ## 8-byte Spill
	movq	8(%rsi), %r15
	mulxq	%r15, %r9, %r12
	movq	%r9, 40(%rsp)                   ## 8-byte Spill
	mulxq	%rdi, %rsi, %rbx
	movq	%rsi, -56(%rsp)                 ## 8-byte Spill
	mulxq	%rdx, %rbp, %rdx
	movq	%rbx, %rdi
	addq	%r9, %rdi
	movq	%rdi, 120(%rsp)                 ## 8-byte Spill
	movq	%r12, %rdi
	adcq	%r8, %rdi
	movq	%rdi, 128(%rsp)                 ## 8-byte Spill
	movq	%r11, %rdi
	adcq	%r14, %rdi
	movq	%rdi, 136(%rsp)                 ## 8-byte Spill
	adcq	-128(%rsp), %r13                ## 8-byte Folded Reload
	movq	%r13, 144(%rsp)                 ## 8-byte Spill
	movq	-104(%rsp), %r9                 ## 8-byte Reload
	adcq	%r9, %rbp
	movq	%rbp, 152(%rsp)                 ## 8-byte Spill
	adcq	$0, %rdx
	movq	%rdx, 160(%rsp)                 ## 8-byte Spill
	movq	%rcx, %rdx
	mulxq	%rax, %rdx, %r14
	movq	%rdx, -120(%rsp)                ## 8-byte Spill
	movq	%rcx, %rdx
	mulxq	%r10, %r13, %r11
	movq	%r13, -16(%rsp)                 ## 8-byte Spill
	movq	%r11, -80(%rsp)                 ## 8-byte Spill
	mulxq	%r15, %rsi, %rdi
	movq	%rsi, 16(%rsp)                  ## 8-byte Spill
	movq	%rdi, -72(%rsp)                 ## 8-byte Spill
	mulxq	-96(%rsp), %rdx, %r8            ## 8-byte Folded Reload
	movq	%rdx, 32(%rsp)                  ## 8-byte Spill
	movq	%rcx, %rdx
	mulxq	%rcx, %rdx, %rcx
	movq	%r8, %rbp
	addq	%rsi, %rbp
	movq	%rbp, 96(%rsp)                  ## 8-byte Spill
	adcq	%r13, %rdi
	movq	%rdi, 88(%rsp)                  ## 8-byte Spill
	adcq	-120(%rsp), %r11                ## 8-byte Folded Reload
	movq	%r11, 80(%rsp)                  ## 8-byte Spill
	adcq	%r14, %rdx
	movq	%rdx, 104(%rsp)                 ## 8-byte Spill
	adcq	-128(%rsp), %rcx                ## 8-byte Folded Reload
	movq	%rcx, 112(%rsp)                 ## 8-byte Spill
	adcq	$0, %r9
	movq	%r9, -104(%rsp)                 ## 8-byte Spill
	movq	%rax, %rdx
	mulxq	%r10, %rdi, %r13
	mulxq	%r15, %rbp, %rcx
	movq	%rbp, -24(%rsp)                 ## 8-byte Spill
	movq	%rcx, -128(%rsp)                ## 8-byte Spill
	movq	-96(%rsp), %r11                 ## 8-byte Reload
	mulxq	%r11, %rdx, %r9
	movq	%rdx, -8(%rsp)                  ## 8-byte Spill
	movq	%rax, %rdx
	mulxq	%rax, %rdx, %rax
	movq	%r9, %rsi
	addq	%rbp, %rsi
	movq	%rsi, 56(%rsp)                  ## 8-byte Spill
	adcq	%rdi, %rcx
	movq	%rcx, 48(%rsp)                  ## 8-byte Spill
	adcq	%r13, %rdx
	movq	%rdx, 64(%rsp)                  ## 8-byte Spill
	movq	%r13, %rbp
	adcq	-120(%rsp), %rax                ## 8-byte Folded Reload
	movq	%rax, 72(%rsp)                  ## 8-byte Spill
	adcq	-112(%rsp), %r14                ## 8-byte Folded Reload
	movq	%r14, -120(%rsp)                ## 8-byte Spill
	adcq	$0, -64(%rsp)                   ## 8-byte Folded Spill
	movq	%r10, %rdx
	mulxq	%r15, %r13, %rsi
	mulxq	%r11, %rcx, %rax
	movq	%rcx, -32(%rsp)                 ## 8-byte Spill
	mulxq	%r10, %rcx, %r10
	movq	%rax, %rdx
	addq	%r13, %rdx
	movq	%rdx, (%rsp)                    ## 8-byte Spill
	adcq	%rsi, %rcx
	movq	%rcx, -40(%rsp)                 ## 8-byte Spill
	adcq	%rdi, %r10
	movq	%r10, 8(%rsp)                   ## 8-byte Spill
	adcq	-16(%rsp), %rbp                 ## 8-byte Folded Reload
	movq	%rbp, -112(%rsp)                ## 8-byte Spill
	movq	24(%rsp), %rcx                  ## 8-byte Reload
	adcq	%rcx, -80(%rsp)                 ## 8-byte Folded Spill
	adcq	$0, -88(%rsp)                   ## 8-byte Folded Spill
	movq	%r15, %rdx
	mulxq	%r15, %r14, %rdi
	mulxq	%r11, %r10, %rcx
	addq	%rcx, %r14
	adcq	%r13, %rdi
	adcq	-24(%rsp), %rsi                 ## 8-byte Folded Reload
	movq	16(%rsp), %rdx                  ## 8-byte Reload
	adcq	%rdx, -128(%rsp)                ## 8-byte Folded Spill
	movq	40(%rsp), %rdx                  ## 8-byte Reload
	adcq	%rdx, -72(%rsp)                 ## 8-byte Folded Spill
	movq	%r11, %rdx
	mulxq	%r11, %rdx, %r11
	movq	-48(%rsp), %rbp                 ## 8-byte Reload
	movq	%rdx, (%rbp)
	adcq	$0, %r12
	addq	%r10, %r11
	movq	-32(%rsp), %rdx                 ## 8-byte Reload
	adcq	%rdx, %rcx
	movq	-8(%rsp), %r15                  ## 8-byte Reload
	adcq	%r15, %rax
	movq	32(%rsp), %rbp                  ## 8-byte Reload
	adcq	%rbp, %r9
	adcq	-56(%rsp), %r8                  ## 8-byte Folded Reload
	adcq	$0, %rbx
	addq	%r10, %r11
	adcq	%r14, %rcx
	adcq	%rdi, %rax
	adcq	%rsi, %r9
	adcq	-128(%rsp), %r8                 ## 8-byte Folded Reload
	adcq	-72(%rsp), %rbx                 ## 8-byte Folded Reload
	adcq	$0, %r12
	addq	%rdx, %rcx
	adcq	(%rsp), %rax                    ## 8-byte Folded Reload
	adcq	-40(%rsp), %r9                  ## 8-byte Folded Reload
	adcq	8(%rsp), %r8                    ## 8-byte Folded Reload
	adcq	-112(%rsp), %rbx                ## 8-byte Folded Reload
	adcq	-80(%rsp), %r12                 ## 8-byte Folded Reload
	movq	-88(%rsp), %rsi                 ## 8-byte Reload
	adcq	$0, %rsi
	addq	%r15, %rax
	adcq	56(%rsp), %r9                   ## 8-byte Folded Reload
	adcq	48(%rsp), %r8                   ## 8-byte Folded Reload
	adcq	64(%rsp), %rbx                  ## 8-byte Folded Reload
	adcq	72(%rsp), %r12                  ## 8-byte Folded Reload
	adcq	-120(%rsp), %rsi                ## 8-byte Folded Reload
	movq	-64(%rsp), %rdi                 ## 8-byte Reload
	adcq	$0, %rdi
	addq	%rbp, %r9
	adcq	96(%rsp), %r8                   ## 8-byte Folded Reload
	adcq	88(%rsp), %rbx                  ## 8-byte Folded Reload
	adcq	80(%rsp), %r12                  ## 8-byte Folded Reload
	adcq	104(%rsp), %rsi                 ## 8-byte Folded Reload
	adcq	112(%rsp), %rdi                 ## 8-byte Folded Reload
	movq	-104(%rsp), %rdx                ## 8-byte Reload
	adcq	$0, %rdx
	addq	-56(%rsp), %r8                  ## 8-byte Folded Reload
	movq	-48(%rsp), %rbp                 ## 8-byte Reload
	movq	%r11, 8(%rbp)
	movq	%rcx, 16(%rbp)
	movq	%rax, 24(%rbp)
	movq	%r9, 32(%rbp)
	movq	%r8, 40(%rbp)
	adcq	120(%rsp), %rbx                 ## 8-byte Folded Reload
	movq	%rbx, 48(%rbp)
	adcq	128(%rsp), %r12                 ## 8-byte Folded Reload
	movq	%r12, 56(%rbp)
	movq	%rsi, %rax
	adcq	136(%rsp), %rax                 ## 8-byte Folded Reload
	movq	%rax, 64(%rbp)
	movq	%rdi, %rax
	adcq	144(%rsp), %rax                 ## 8-byte Folded Reload
	movq	%rax, 72(%rbp)
	movq	%rdx, %rax
	adcq	152(%rsp), %rax                 ## 8-byte Folded Reload
	movq	%rax, 80(%rbp)
	movq	160(%rsp), %rax                 ## 8-byte Reload
	adcq	$0, %rax
	movq	%rax, 88(%rbp)
	addq	$168, %rsp
	popq	%rbx
	popq	%r12
	popq	%r13
	popq	%r14
	popq	%r15
	popq	%rbp
	retq
                                        ## -- End function
	.globl	_mcl_fp_mont6Lbmi2              ## -- Begin function mcl_fp_mont6Lbmi2
	.p2align	4, 0x90
_mcl_fp_mont6Lbmi2:                     ## @mcl_fp_mont6Lbmi2
## %bb.0:
	pushq	%rbp
	pushq	%r15
	pushq	%r14
	pushq	%r13
	pushq	%r12
	pushq	%rbx
	subq	$32, %rsp
	movq	%rdx, -80(%rsp)                 ## 8-byte Spill
	movq	%rdi, 24(%rsp)                  ## 8-byte Spill
	movq	40(%rsi), %rdi
	movq	%rdi, -88(%rsp)                 ## 8-byte Spill
	movq	(%rdx), %rax
	movq	%rdi, %rdx
	mulxq	%rax, %r8, %rbx
	movq	32(%rsi), %rdx
	movq	%rdx, -96(%rsp)                 ## 8-byte Spill
	mulxq	%rax, %r11, %rdi
	movq	24(%rsi), %rdx
	movq	%rdx, -72(%rsp)                 ## 8-byte Spill
	mulxq	%rax, %r13, %r12
	movq	16(%rsi), %rdx
	movq	%rdx, -8(%rsp)                  ## 8-byte Spill
	mulxq	%rax, %r14, %r15
	movq	(%rsi), %rbp
	movq	%rbp, -16(%rsp)                 ## 8-byte Spill
	movq	8(%rsi), %rdx
	movq	%rdx, -24(%rsp)                 ## 8-byte Spill
	mulxq	%rax, %rsi, %r10
	movq	%rbp, %rdx
	mulxq	%rax, %rax, %r9
	movq	%rax, -120(%rsp)                ## 8-byte Spill
	addq	%rsi, %r9
	adcq	%r14, %r10
	adcq	%r13, %r15
	adcq	%r11, %r12
	adcq	%r8, %rdi
	movq	%rdi, -112(%rsp)                ## 8-byte Spill
	adcq	$0, %rbx
	movq	%rbx, -128(%rsp)                ## 8-byte Spill
	movq	-8(%rcx), %rdx
	movq	%rdx, 8(%rsp)                   ## 8-byte Spill
	imulq	%rax, %rdx
	movq	40(%rcx), %rax
	movq	%rax, -32(%rsp)                 ## 8-byte Spill
	mulxq	%rax, %r13, %rbp
	movq	16(%rcx), %rax
	movq	%rax, -40(%rsp)                 ## 8-byte Spill
	mulxq	%rax, %r8, %r14
	movq	8(%rcx), %rax
	movq	%rax, (%rsp)                    ## 8-byte Spill
	mulxq	%rax, %rax, %r11
	movq	(%rcx), %rsi
	movq	%rsi, -48(%rsp)                 ## 8-byte Spill
	mulxq	%rsi, %rsi, %rdi
	addq	%rax, %rdi
	adcq	%r8, %r11
	movq	24(%rcx), %rax
	movq	%rax, -56(%rsp)                 ## 8-byte Spill
	mulxq	%rax, %rbx, %r8
	adcq	%r14, %rbx
	movq	32(%rcx), %rax
	movq	%rax, -64(%rsp)                 ## 8-byte Spill
	mulxq	%rax, %rcx, %rax
	adcq	%r8, %rcx
	adcq	%r13, %rax
	adcq	$0, %rbp
	addq	-120(%rsp), %rsi                ## 8-byte Folded Reload
	adcq	%r9, %rdi
	adcq	%r10, %r11
	adcq	%r15, %rbx
	adcq	%r12, %rcx
	adcq	-112(%rsp), %rax                ## 8-byte Folded Reload
	adcq	-128(%rsp), %rbp                ## 8-byte Folded Reload
	movq	%rbp, -104(%rsp)                ## 8-byte Spill
	movq	-80(%rsp), %rdx                 ## 8-byte Reload
	movq	8(%rdx), %rdx
	mulxq	-88(%rsp), %rbp, %rsi           ## 8-byte Folded Reload
	movq	%rbp, -120(%rsp)                ## 8-byte Spill
	movq	%rsi, -128(%rsp)                ## 8-byte Spill
	mulxq	-96(%rsp), %rbp, %r15           ## 8-byte Folded Reload
	mulxq	-72(%rsp), %rsi, %r14           ## 8-byte Folded Reload
	movq	%rsi, 16(%rsp)                  ## 8-byte Spill
	mulxq	-24(%rsp), %rsi, %r8            ## 8-byte Folded Reload
	mulxq	-16(%rsp), %r12, %r10           ## 8-byte Folded Reload
	setb	-112(%rsp)                      ## 1-byte Folded Spill
	addq	%rsi, %r10
	mulxq	-8(%rsp), %r9, %r13             ## 8-byte Folded Reload
	adcq	%r8, %r9
	adcq	16(%rsp), %r13                  ## 8-byte Folded Reload
	adcq	%rbp, %r14
	adcq	-120(%rsp), %r15                ## 8-byte Folded Reload
	movq	-128(%rsp), %rdx                ## 8-byte Reload
	adcq	$0, %rdx
	addq	%rdi, %r12
	adcq	%r11, %r10
	adcq	%rbx, %r9
	adcq	%rcx, %r13
	adcq	%rax, %r14
	adcq	-104(%rsp), %r15                ## 8-byte Folded Reload
	movzbl	-112(%rsp), %eax                ## 1-byte Folded Reload
	adcq	%rax, %rdx
	movq	%rdx, -128(%rsp)                ## 8-byte Spill
	setb	-112(%rsp)                      ## 1-byte Folded Spill
	movq	8(%rsp), %rdx                   ## 8-byte Reload
	imulq	%r12, %rdx
	mulxq	-32(%rsp), %rax, %rbp           ## 8-byte Folded Reload
	movq	%rax, -120(%rsp)                ## 8-byte Spill
	mulxq	-64(%rsp), %rax, %r11           ## 8-byte Folded Reload
	movq	%rax, -104(%rsp)                ## 8-byte Spill
	mulxq	(%rsp), %rdi, %rsi              ## 8-byte Folded Reload
	mulxq	-48(%rsp), %rcx, %r8            ## 8-byte Folded Reload
	addq	%rdi, %r8
	mulxq	-40(%rsp), %rbx, %rax           ## 8-byte Folded Reload
	adcq	%rsi, %rbx
	mulxq	-56(%rsp), %rsi, %rdi           ## 8-byte Folded Reload
	adcq	%rax, %rsi
	adcq	-104(%rsp), %rdi                ## 8-byte Folded Reload
	adcq	-120(%rsp), %r11                ## 8-byte Folded Reload
	adcq	$0, %rbp
	addq	%r12, %rcx
	adcq	%r10, %r8
	adcq	%r9, %rbx
	adcq	%r13, %rsi
	adcq	%r14, %rdi
	adcq	%r15, %r11
	adcq	-128(%rsp), %rbp                ## 8-byte Folded Reload
	movzbl	-112(%rsp), %r10d               ## 1-byte Folded Reload
	adcq	$0, %r10
	movq	-80(%rsp), %rcx                 ## 8-byte Reload
	movq	16(%rcx), %rdx
	mulxq	-88(%rsp), %rcx, %rax           ## 8-byte Folded Reload
	movq	%rcx, -112(%rsp)                ## 8-byte Spill
	movq	%rax, -128(%rsp)                ## 8-byte Spill
	mulxq	-96(%rsp), %rax, %r13           ## 8-byte Folded Reload
	movq	%rax, -120(%rsp)                ## 8-byte Spill
	mulxq	-72(%rsp), %rax, %r15           ## 8-byte Folded Reload
	movq	%rax, -104(%rsp)                ## 8-byte Spill
	mulxq	-24(%rsp), %rcx, %r14           ## 8-byte Folded Reload
	mulxq	-16(%rsp), %rax, %r9            ## 8-byte Folded Reload
	addq	%rcx, %r9
	mulxq	-8(%rsp), %rcx, %r12            ## 8-byte Folded Reload
	adcq	%r14, %rcx
	adcq	-104(%rsp), %r12                ## 8-byte Folded Reload
	adcq	-120(%rsp), %r15                ## 8-byte Folded Reload
	adcq	-112(%rsp), %r13                ## 8-byte Folded Reload
	movq	-128(%rsp), %rdx                ## 8-byte Reload
	adcq	$0, %rdx
	addq	%r8, %rax
	movq	%rax, %r14
	adcq	%rbx, %r9
	adcq	%rsi, %rcx
	adcq	%rdi, %r12
	adcq	%r11, %r15
	adcq	%rbp, %r13
	movq	%r13, -120(%rsp)                ## 8-byte Spill
	adcq	%r10, %rdx
	movq	%rdx, -128(%rsp)                ## 8-byte Spill
	setb	-112(%rsp)                      ## 1-byte Folded Spill
	movq	8(%rsp), %rdx                   ## 8-byte Reload
	imulq	%rax, %rdx
	mulxq	-32(%rsp), %rax, %r13           ## 8-byte Folded Reload
	movq	%rax, -104(%rsp)                ## 8-byte Spill
	mulxq	-64(%rsp), %r10, %r11           ## 8-byte Folded Reload
	mulxq	(%rsp), %rbx, %rdi              ## 8-byte Folded Reload
	mulxq	-48(%rsp), %r8, %rsi            ## 8-byte Folded Reload
	addq	%rbx, %rsi
	mulxq	-40(%rsp), %rbx, %rax           ## 8-byte Folded Reload
	adcq	%rdi, %rbx
	mulxq	-56(%rsp), %rbp, %rdi           ## 8-byte Folded Reload
	adcq	%rax, %rbp
	adcq	%r10, %rdi
	adcq	-104(%rsp), %r11                ## 8-byte Folded Reload
	adcq	$0, %r13
	addq	%r14, %r8
	adcq	%r9, %rsi
	adcq	%rcx, %rbx
	adcq	%r12, %rbp
	adcq	%r15, %rdi
	adcq	-120(%rsp), %r11                ## 8-byte Folded Reload
	adcq	-128(%rsp), %r13                ## 8-byte Folded Reload
	movzbl	-112(%rsp), %r9d                ## 1-byte Folded Reload
	adcq	$0, %r9
	movq	-80(%rsp), %rcx                 ## 8-byte Reload
	movq	24(%rcx), %rdx
	mulxq	-88(%rsp), %rcx, %rax           ## 8-byte Folded Reload
	movq	%rcx, -112(%rsp)                ## 8-byte Spill
	movq	%rax, -128(%rsp)                ## 8-byte Spill
	mulxq	-96(%rsp), %rax, %r14           ## 8-byte Folded Reload
	movq	%rax, -120(%rsp)                ## 8-byte Spill
	mulxq	-72(%rsp), %rax, %r15           ## 8-byte Folded Reload
	movq	%rax, -104(%rsp)                ## 8-byte Spill
	mulxq	-24(%rsp), %rcx, %r10           ## 8-byte Folded Reload
	mulxq	-16(%rsp), %rax, %r8            ## 8-byte Folded Reload
	addq	%rcx, %r8
	mulxq	-8(%rsp), %rcx, %r12            ## 8-byte Folded Reload
	adcq	%r10, %rcx
	adcq	-104(%rsp), %r12                ## 8-byte Folded Reload
	adcq	-120(%rsp), %r15                ## 8-byte Folded Reload
	adcq	-112(%rsp), %r14                ## 8-byte Folded Reload
	movq	-128(%rsp), %rdx                ## 8-byte Reload
	adcq	$0, %rdx
	addq	%rsi, %rax
	movq	%rax, %r10
	adcq	%rbx, %r8
	adcq	%rbp, %rcx
	adcq	%rdi, %r12
	adcq	%r11, %r15
	adcq	%r13, %r14
	movq	%r14, -120(%rsp)                ## 8-byte Spill
	adcq	%r9, %rdx
	movq	%rdx, -128(%rsp)                ## 8-byte Spill
	setb	-112(%rsp)                      ## 1-byte Folded Spill
	movq	8(%rsp), %rdx                   ## 8-byte Reload
	imulq	%rax, %rdx
	mulxq	-32(%rsp), %rax, %r14           ## 8-byte Folded Reload
	movq	%rax, -104(%rsp)                ## 8-byte Spill
	mulxq	-64(%rsp), %r13, %r11           ## 8-byte Folded Reload
	mulxq	(%rsp), %rbx, %rsi              ## 8-byte Folded Reload
	mulxq	-48(%rsp), %rax, %rdi           ## 8-byte Folded Reload
	addq	%rbx, %rdi
	mulxq	-40(%rsp), %rbx, %r9            ## 8-byte Folded Reload
	adcq	%rsi, %rbx
	mulxq	-56(%rsp), %rbp, %rsi           ## 8-byte Folded Reload
	adcq	%r9, %rbp
	adcq	%r13, %rsi
	adcq	-104(%rsp), %r11                ## 8-byte Folded Reload
	adcq	$0, %r14
	addq	%r10, %rax
	adcq	%r8, %rdi
	adcq	%rcx, %rbx
	adcq	%r12, %rbp
	adcq	%r15, %rsi
	adcq	-120(%rsp), %r11                ## 8-byte Folded Reload
	adcq	-128(%rsp), %r14                ## 8-byte Folded Reload
	movzbl	-112(%rsp), %r9d                ## 1-byte Folded Reload
	adcq	$0, %r9
	movq	-80(%rsp), %rcx                 ## 8-byte Reload
	movq	32(%rcx), %rdx
	mulxq	-88(%rsp), %rcx, %rax           ## 8-byte Folded Reload
	movq	%rcx, -112(%rsp)                ## 8-byte Spill
	movq	%rax, -128(%rsp)                ## 8-byte Spill
	mulxq	-96(%rsp), %rax, %r15           ## 8-byte Folded Reload
	movq	%rax, -120(%rsp)                ## 8-byte Spill
	mulxq	-72(%rsp), %rax, %r12           ## 8-byte Folded Reload
	movq	%rax, -104(%rsp)                ## 8-byte Spill
	mulxq	-24(%rsp), %rcx, %r10           ## 8-byte Folded Reload
	mulxq	-16(%rsp), %rax, %r13           ## 8-byte Folded Reload
	addq	%rcx, %r13
	mulxq	-8(%rsp), %rcx, %r8             ## 8-byte Folded Reload
	adcq	%r10, %rcx
	adcq	-104(%rsp), %r8                 ## 8-byte Folded Reload
	adcq	-120(%rsp), %r12                ## 8-byte Folded Reload
	adcq	-112(%rsp), %r15                ## 8-byte Folded Reload
	movq	-128(%rsp), %rdx                ## 8-byte Reload
	adcq	$0, %rdx
	addq	%rdi, %rax
	movq	%rax, -120(%rsp)                ## 8-byte Spill
	adcq	%rbx, %r13
	adcq	%rbp, %rcx
	adcq	%rsi, %r8
	adcq	%r11, %r12
	adcq	%r14, %r15
	adcq	%r9, %rdx
	movq	%rdx, -128(%rsp)                ## 8-byte Spill
	setb	-112(%rsp)                      ## 1-byte Folded Spill
	movq	8(%rsp), %rdx                   ## 8-byte Reload
	imulq	%rax, %rdx
	mulxq	-32(%rsp), %rax, %r14           ## 8-byte Folded Reload
	movq	%rax, -104(%rsp)                ## 8-byte Spill
	mulxq	-64(%rsp), %r9, %r10            ## 8-byte Folded Reload
	mulxq	(%rsp), %rbx, %rsi              ## 8-byte Folded Reload
	mulxq	-48(%rsp), %rax, %r11           ## 8-byte Folded Reload
	addq	%rbx, %r11
	mulxq	-40(%rsp), %rbx, %rdi           ## 8-byte Folded Reload
	adcq	%rsi, %rbx
	mulxq	-56(%rsp), %rbp, %rsi           ## 8-byte Folded Reload
	adcq	%rdi, %rbp
	adcq	%r9, %rsi
	adcq	-104(%rsp), %r10                ## 8-byte Folded Reload
	adcq	$0, %r14
	addq	-120(%rsp), %rax                ## 8-byte Folded Reload
	adcq	%r13, %r11
	adcq	%rcx, %rbx
	adcq	%r8, %rbp
	adcq	%r12, %rsi
	adcq	%r15, %r10
	adcq	-128(%rsp), %r14                ## 8-byte Folded Reload
	movq	%r14, -128(%rsp)                ## 8-byte Spill
	movzbl	-112(%rsp), %edi                ## 1-byte Folded Reload
	adcq	$0, %rdi
	movq	-80(%rsp), %rax                 ## 8-byte Reload
	movq	40(%rax), %rdx
	mulxq	-88(%rsp), %rcx, %rax           ## 8-byte Folded Reload
	movq	%rcx, -88(%rsp)                 ## 8-byte Spill
	movq	%rax, -80(%rsp)                 ## 8-byte Spill
	mulxq	-96(%rsp), %rax, %r8            ## 8-byte Folded Reload
	movq	%rax, -96(%rsp)                 ## 8-byte Spill
	mulxq	-72(%rsp), %rax, %r15           ## 8-byte Folded Reload
	movq	%rax, -72(%rsp)                 ## 8-byte Spill
	mulxq	-8(%rsp), %r14, %r12            ## 8-byte Folded Reload
	mulxq	-24(%rsp), %rcx, %r13           ## 8-byte Folded Reload
	mulxq	-16(%rsp), %r9, %rax            ## 8-byte Folded Reload
	addq	%rcx, %rax
	adcq	%r14, %r13
	adcq	-72(%rsp), %r12                 ## 8-byte Folded Reload
	adcq	-96(%rsp), %r15                 ## 8-byte Folded Reload
	adcq	-88(%rsp), %r8                  ## 8-byte Folded Reload
	movq	-80(%rsp), %rcx                 ## 8-byte Reload
	adcq	$0, %rcx
	addq	%r11, %r9
	adcq	%rbx, %rax
	adcq	%rbp, %r13
	adcq	%rsi, %r12
	adcq	%r10, %r15
	adcq	-128(%rsp), %r8                 ## 8-byte Folded Reload
	movq	%r8, -96(%rsp)                  ## 8-byte Spill
	adcq	%rdi, %rcx
	movq	%rcx, -80(%rsp)                 ## 8-byte Spill
	setb	-88(%rsp)                       ## 1-byte Folded Spill
	movq	8(%rsp), %rdx                   ## 8-byte Reload
	imulq	%r9, %rdx
	mulxq	-48(%rsp), %r11, %rsi           ## 8-byte Folded Reload
	movq	(%rsp), %r10                    ## 8-byte Reload
	mulxq	%r10, %rcx, %rbx
	addq	%rsi, %rcx
	mulxq	-40(%rsp), %rdi, %rbp           ## 8-byte Folded Reload
	adcq	%rbx, %rdi
	mulxq	-56(%rsp), %rsi, %rbx           ## 8-byte Folded Reload
	adcq	%rbp, %rsi
	mulxq	-64(%rsp), %rbp, %r14           ## 8-byte Folded Reload
	adcq	%rbx, %rbp
	mulxq	-32(%rsp), %rdx, %rbx           ## 8-byte Folded Reload
	adcq	%r14, %rdx
	adcq	$0, %rbx
	addq	%r9, %r11
	adcq	%rax, %rcx
	adcq	%r13, %rdi
	adcq	%r12, %rsi
	adcq	%r15, %rbp
	adcq	-96(%rsp), %rdx                 ## 8-byte Folded Reload
	adcq	-80(%rsp), %rbx                 ## 8-byte Folded Reload
	movzbl	-88(%rsp), %r11d                ## 1-byte Folded Reload
	adcq	$0, %r11
	movq	%rcx, %r8
	subq	-48(%rsp), %r8                  ## 8-byte Folded Reload
	movq	%rdi, %r9
	sbbq	%r10, %r9
	movq	%rsi, %r10
	sbbq	-40(%rsp), %r10                 ## 8-byte Folded Reload
	movq	%rbp, %r14
	sbbq	-56(%rsp), %r14                 ## 8-byte Folded Reload
	movq	%rdx, %r15
	sbbq	-64(%rsp), %r15                 ## 8-byte Folded Reload
	movq	%rbx, %rax
	sbbq	-32(%rsp), %rax                 ## 8-byte Folded Reload
	sbbq	$0, %r11
	testb	$1, %r11b
	cmovneq	%rbx, %rax
	movq	24(%rsp), %rbx                  ## 8-byte Reload
	movq	%rax, 40(%rbx)
	cmovneq	%rdx, %r15
	movq	%r15, 32(%rbx)
	cmovneq	%rbp, %r14
	movq	%r14, 24(%rbx)
	cmovneq	%rsi, %r10
	movq	%r10, 16(%rbx)
	cmovneq	%rdi, %r9
	movq	%r9, 8(%rbx)
	cmovneq	%rcx, %r8
	movq	%r8, (%rbx)
	addq	$32, %rsp
	popq	%rbx
	popq	%r12
	popq	%r13
	popq	%r14
	popq	%r15
	popq	%rbp
	retq
                                        ## -- End function
	.globl	_mcl_fp_montNF6Lbmi2            ## -- Begin function mcl_fp_montNF6Lbmi2
	.p2align	4, 0x90
_mcl_fp_montNF6Lbmi2:                   ## @mcl_fp_montNF6Lbmi2
## %bb.0:
	pushq	%rbp
	pushq	%r15
	pushq	%r14
	pushq	%r13
	pushq	%r12
	pushq	%rbx
	movq	%rdx, -120(%rsp)                ## 8-byte Spill
	movq	%rdi, -8(%rsp)                  ## 8-byte Spill
	movq	(%rsi), %rax
	movq	%rax, -112(%rsp)                ## 8-byte Spill
	movq	8(%rsi), %rdi
	movq	%rdi, -128(%rsp)                ## 8-byte Spill
	movq	(%rdx), %rbp
	movq	%rdi, %rdx
	mulxq	%rbp, %rdi, %rbx
	movq	%rax, %rdx
	mulxq	%rbp, %r9, %r14
	movq	16(%rsi), %rdx
	movq	%rdx, -64(%rsp)                 ## 8-byte Spill
	addq	%rdi, %r14
	mulxq	%rbp, %rdi, %r8
	adcq	%rbx, %rdi
	movq	24(%rsi), %rdx
	movq	%rdx, -72(%rsp)                 ## 8-byte Spill
	mulxq	%rbp, %rbx, %r10
	adcq	%r8, %rbx
	movq	32(%rsi), %rdx
	movq	%rdx, -80(%rsp)                 ## 8-byte Spill
	mulxq	%rbp, %r8, %r11
	adcq	%r10, %r8
	movq	40(%rsi), %rdx
	movq	%rdx, -88(%rsp)                 ## 8-byte Spill
	mulxq	%rbp, %rsi, %r15
	adcq	%r11, %rsi
	adcq	$0, %r15
	movq	-8(%rcx), %rdx
	movq	%rdx, -104(%rsp)                ## 8-byte Spill
	imulq	%r9, %rdx
	movq	(%rcx), %rax
	movq	%rax, -16(%rsp)                 ## 8-byte Spill
	mulxq	%rax, %rbp, %rax
	movq	%rax, -96(%rsp)                 ## 8-byte Spill
	addq	%r9, %rbp
	movq	8(%rcx), %rax
	movq	%rax, -24(%rsp)                 ## 8-byte Spill
	mulxq	%rax, %r13, %r9
	adcq	%r14, %r13
	movq	16(%rcx), %rax
	movq	%rax, -32(%rsp)                 ## 8-byte Spill
	mulxq	%rax, %r12, %rax
	adcq	%rdi, %r12
	movq	24(%rcx), %rdi
	movq	%rdi, -40(%rsp)                 ## 8-byte Spill
	mulxq	%rdi, %r14, %rdi
	adcq	%rbx, %r14
	movq	32(%rcx), %rbp
	movq	%rbp, -48(%rsp)                 ## 8-byte Spill
	mulxq	%rbp, %r11, %rbx
	adcq	%r8, %r11
	movq	40(%rcx), %rcx
	movq	%rcx, -56(%rsp)                 ## 8-byte Spill
	mulxq	%rcx, %r10, %rcx
	adcq	%rsi, %r10
	adcq	$0, %r15
	addq	-96(%rsp), %r13                 ## 8-byte Folded Reload
	adcq	%r9, %r12
	adcq	%rax, %r14
	adcq	%rdi, %r11
	adcq	%rbx, %r10
	adcq	%rcx, %r15
	movq	-120(%rsp), %rax                ## 8-byte Reload
	movq	8(%rax), %rdx
	mulxq	-128(%rsp), %rcx, %rsi          ## 8-byte Folded Reload
	mulxq	-112(%rsp), %rbx, %rax          ## 8-byte Folded Reload
	addq	%rcx, %rax
	mulxq	-64(%rsp), %rcx, %rdi           ## 8-byte Folded Reload
	adcq	%rsi, %rcx
	mulxq	-72(%rsp), %rsi, %r8            ## 8-byte Folded Reload
	adcq	%rdi, %rsi
	mulxq	-80(%rsp), %rdi, %rbp           ## 8-byte Folded Reload
	movq	%rbp, -96(%rsp)                 ## 8-byte Spill
	adcq	%r8, %rdi
	mulxq	-88(%rsp), %r8, %r9             ## 8-byte Folded Reload
	adcq	-96(%rsp), %r8                  ## 8-byte Folded Reload
	adcq	$0, %r9
	addq	%r13, %rbx
	adcq	%r12, %rax
	adcq	%r14, %rcx
	adcq	%r11, %rsi
	adcq	%r10, %rdi
	adcq	%r15, %r8
	adcq	$0, %r9
	movq	-104(%rsp), %rdx                ## 8-byte Reload
	imulq	%rbx, %rdx
	mulxq	-16(%rsp), %rbp, %r13           ## 8-byte Folded Reload
	addq	%rbx, %rbp
	mulxq	-24(%rsp), %r11, %rbx           ## 8-byte Folded Reload
	adcq	%rax, %r11
	mulxq	-32(%rsp), %r14, %rax           ## 8-byte Folded Reload
	adcq	%rcx, %r14
	mulxq	-40(%rsp), %r10, %rcx           ## 8-byte Folded Reload
	adcq	%rsi, %r10
	mulxq	-48(%rsp), %r15, %rsi           ## 8-byte Folded Reload
	adcq	%rdi, %r15
	mulxq	-56(%rsp), %r12, %rdx           ## 8-byte Folded Reload
	adcq	%r8, %r12
	adcq	$0, %r9
	addq	%r13, %r11
	adcq	%rbx, %r14
	adcq	%rax, %r10
	adcq	%rcx, %r15
	adcq	%rsi, %r12
	adcq	%rdx, %r9
	movq	-120(%rsp), %rax                ## 8-byte Reload
	movq	16(%rax), %rdx
	mulxq	-128(%rsp), %rcx, %rax          ## 8-byte Folded Reload
	mulxq	-112(%rsp), %r13, %rdi          ## 8-byte Folded Reload
	addq	%rcx, %rdi
	mulxq	-64(%rsp), %rbx, %rcx           ## 8-byte Folded Reload
	adcq	%rax, %rbx
	mulxq	-72(%rsp), %rsi, %rbp           ## 8-byte Folded Reload
	adcq	%rcx, %rsi
	mulxq	-80(%rsp), %rax, %rcx           ## 8-byte Folded Reload
	movq	%rcx, -96(%rsp)                 ## 8-byte Spill
	adcq	%rbp, %rax
	mulxq	-88(%rsp), %r8, %rcx            ## 8-byte Folded Reload
	adcq	-96(%rsp), %r8                  ## 8-byte Folded Reload
	adcq	$0, %rcx
	addq	%r11, %r13
	adcq	%r14, %rdi
	adcq	%r10, %rbx
	adcq	%r15, %rsi
	adcq	%r12, %rax
	adcq	%r9, %r8
	adcq	$0, %rcx
	movq	-104(%rsp), %rdx                ## 8-byte Reload
	imulq	%r13, %rdx
	mulxq	-16(%rsp), %rbp, %r12           ## 8-byte Folded Reload
	addq	%r13, %rbp
	mulxq	-24(%rsp), %r11, %rbp           ## 8-byte Folded Reload
	adcq	%rdi, %r11
	mulxq	-32(%rsp), %r9, %rdi            ## 8-byte Folded Reload
	adcq	%rbx, %r9
	mulxq	-40(%rsp), %r10, %rbx           ## 8-byte Folded Reload
	adcq	%rsi, %r10
	mulxq	-48(%rsp), %r14, %rsi           ## 8-byte Folded Reload
	adcq	%rax, %r14
	mulxq	-56(%rsp), %r15, %rax           ## 8-byte Folded Reload
	adcq	%r8, %r15
	adcq	$0, %rcx
	addq	%r12, %r11
	adcq	%rbp, %r9
	adcq	%rdi, %r10
	adcq	%rbx, %r14
	adcq	%rsi, %r15
	adcq	%rax, %rcx
	movq	-120(%rsp), %rax                ## 8-byte Reload
	movq	24(%rax), %rdx
	mulxq	-128(%rsp), %rsi, %rax          ## 8-byte Folded Reload
	mulxq	-112(%rsp), %r13, %rbx          ## 8-byte Folded Reload
	addq	%rsi, %rbx
	mulxq	-64(%rsp), %rdi, %rbp           ## 8-byte Folded Reload
	adcq	%rax, %rdi
	mulxq	-72(%rsp), %rsi, %r8            ## 8-byte Folded Reload
	adcq	%rbp, %rsi
	mulxq	-80(%rsp), %rax, %rbp           ## 8-byte Folded Reload
	adcq	%r8, %rax
	mulxq	-88(%rsp), %r8, %r12            ## 8-byte Folded Reload
	adcq	%rbp, %r8
	adcq	$0, %r12
	addq	%r11, %r13
	adcq	%r9, %rbx
	adcq	%r10, %rdi
	adcq	%r14, %rsi
	adcq	%r15, %rax
	adcq	%rcx, %r8
	adcq	$0, %r12
	movq	-104(%rsp), %rdx                ## 8-byte Reload
	imulq	%r13, %rdx
	mulxq	-16(%rsp), %rbp, %rcx           ## 8-byte Folded Reload
	addq	%r13, %rbp
	mulxq	-24(%rsp), %r11, %rbp           ## 8-byte Folded Reload
	adcq	%rbx, %r11
	mulxq	-32(%rsp), %r9, %rbx            ## 8-byte Folded Reload
	adcq	%rdi, %r9
	mulxq	-40(%rsp), %r10, %rdi           ## 8-byte Folded Reload
	adcq	%rsi, %r10
	mulxq	-48(%rsp), %r14, %rsi           ## 8-byte Folded Reload
	adcq	%rax, %r14
	mulxq	-56(%rsp), %r15, %rax           ## 8-byte Folded Reload
	adcq	%r8, %r15
	adcq	$0, %r12
	addq	%rcx, %r11
	adcq	%rbp, %r9
	adcq	%rbx, %r10
	adcq	%rdi, %r14
	adcq	%rsi, %r15
	adcq	%rax, %r12
	movq	-120(%rsp), %rax                ## 8-byte Reload
	movq	32(%rax), %rdx
	mulxq	-128(%rsp), %rsi, %rcx          ## 8-byte Folded Reload
	mulxq	-112(%rsp), %r13, %rax          ## 8-byte Folded Reload
	addq	%rsi, %rax
	mulxq	-64(%rsp), %rbx, %rsi           ## 8-byte Folded Reload
	adcq	%rcx, %rbx
	mulxq	-72(%rsp), %rdi, %rcx           ## 8-byte Folded Reload
	adcq	%rsi, %rdi
	mulxq	-80(%rsp), %rsi, %rbp           ## 8-byte Folded Reload
	adcq	%rcx, %rsi
	mulxq	-88(%rsp), %r8, %rcx            ## 8-byte Folded Reload
	adcq	%rbp, %r8
	adcq	$0, %rcx
	addq	%r11, %r13
	adcq	%r9, %rax
	adcq	%r10, %rbx
	adcq	%r14, %rdi
	adcq	%r15, %rsi
	adcq	%r12, %r8
	adcq	$0, %rcx
	movq	-104(%rsp), %rdx                ## 8-byte Reload
	imulq	%r13, %rdx
	mulxq	-16(%rsp), %rbp, %r15           ## 8-byte Folded Reload
	addq	%r13, %rbp
	mulxq	-24(%rsp), %r11, %rbp           ## 8-byte Folded Reload
	adcq	%rax, %r11
	mulxq	-32(%rsp), %r9, %rax            ## 8-byte Folded Reload
	adcq	%rbx, %r9
	mulxq	-40(%rsp), %r10, %rbx           ## 8-byte Folded Reload
	adcq	%rdi, %r10
	mulxq	-48(%rsp), %r14, %rdi           ## 8-byte Folded Reload
	adcq	%rsi, %r14
	mulxq	-56(%rsp), %rsi, %rdx           ## 8-byte Folded Reload
	adcq	%r8, %rsi
	adcq	$0, %rcx
	addq	%r15, %r11
	adcq	%rbp, %r9
	adcq	%rax, %r10
	adcq	%rbx, %r14
	adcq	%rdi, %rsi
	adcq	%rdx, %rcx
	movq	-120(%rsp), %rax                ## 8-byte Reload
	movq	40(%rax), %rdx
	mulxq	-128(%rsp), %rdi, %rax          ## 8-byte Folded Reload
	mulxq	-112(%rsp), %r13, %rbx          ## 8-byte Folded Reload
	addq	%rdi, %rbx
	mulxq	-64(%rsp), %rdi, %rbp           ## 8-byte Folded Reload
	adcq	%rax, %rdi
	mulxq	-72(%rsp), %r8, %rax            ## 8-byte Folded Reload
	adcq	%rbp, %r8
	mulxq	-80(%rsp), %r15, %rbp           ## 8-byte Folded Reload
	adcq	%rax, %r15
	mulxq	-88(%rsp), %r12, %rax           ## 8-byte Folded Reload
	adcq	%rbp, %r12
	adcq	$0, %rax
	addq	%r11, %r13
	adcq	%r9, %rbx
	adcq	%r10, %rdi
	adcq	%r14, %r8
	adcq	%rsi, %r15
	adcq	%rcx, %r12
	adcq	$0, %rax
	movq	-104(%rsp), %rdx                ## 8-byte Reload
	imulq	%r13, %rdx
	movq	-16(%rsp), %r9                  ## 8-byte Reload
	mulxq	%r9, %rcx, %rsi
	movq	%rsi, -104(%rsp)                ## 8-byte Spill
	addq	%r13, %rcx
	movq	-24(%rsp), %r10                 ## 8-byte Reload
	mulxq	%r10, %r13, %rcx
	movq	%rcx, -112(%rsp)                ## 8-byte Spill
	adcq	%rbx, %r13
	movq	-32(%rsp), %r11                 ## 8-byte Reload
	mulxq	%r11, %rbp, %rcx
	movq	%rcx, -120(%rsp)                ## 8-byte Spill
	adcq	%rdi, %rbp
	movq	%rdx, %rcx
	movq	-40(%rsp), %rsi                 ## 8-byte Reload
	mulxq	%rsi, %rdi, %rdx
	movq	%rdx, -128(%rsp)                ## 8-byte Spill
	adcq	%r8, %rdi
	movq	%rcx, %rdx
	movq	-48(%rsp), %r14                 ## 8-byte Reload
	mulxq	%r14, %rbx, %r8
	adcq	%r15, %rbx
	movq	-56(%rsp), %rcx                 ## 8-byte Reload
	mulxq	%rcx, %r15, %rdx
	adcq	%r12, %r15
	adcq	$0, %rax
	addq	-104(%rsp), %r13                ## 8-byte Folded Reload
	adcq	-112(%rsp), %rbp                ## 8-byte Folded Reload
	adcq	-120(%rsp), %rdi                ## 8-byte Folded Reload
	adcq	-128(%rsp), %rbx                ## 8-byte Folded Reload
	adcq	%r8, %r15
	adcq	%rdx, %rax
	movq	%r13, %r8
	subq	%r9, %r8
	movq	%rbp, %r9
	sbbq	%r10, %r9
	movq	%rdi, %r10
	sbbq	%r11, %r10
	movq	%rbx, %r11
	sbbq	%rsi, %r11
	movq	%r15, %rsi
	sbbq	%r14, %rsi
	movq	%rax, %rdx
	sbbq	%rcx, %rdx
	movq	%rdx, %rcx
	sarq	$63, %rcx
	cmovsq	%rax, %rdx
	movq	-8(%rsp), %rax                  ## 8-byte Reload
	movq	%rdx, 40(%rax)
	cmovsq	%r15, %rsi
	movq	%rsi, 32(%rax)
	cmovsq	%rbx, %r11
	movq	%r11, 24(%rax)
	cmovsq	%rdi, %r10
	movq	%r10, 16(%rax)
	cmovsq	%rbp, %r9
	movq	%r9, 8(%rax)
	cmovsq	%r13, %r8
	movq	%r8, (%rax)
	popq	%rbx
	popq	%r12
	popq	%r13
	popq	%r14
	popq	%r15
	popq	%rbp
	retq
                                        ## -- End function
	.globl	_mcl_fp_montRed6Lbmi2           ## -- Begin function mcl_fp_montRed6Lbmi2
	.p2align	4, 0x90
_mcl_fp_montRed6Lbmi2:                  ## @mcl_fp_montRed6Lbmi2
## %bb.0:
	pushq	%rbp
	pushq	%r15
	pushq	%r14
	pushq	%r13
	pushq	%r12
	pushq	%rbx
	movq	%rdx, %rcx
	movq	%rsi, %r11
	movq	%rdi, -8(%rsp)                  ## 8-byte Spill
	movq	-8(%rdx), %rax
	movq	%rax, -80(%rsp)                 ## 8-byte Spill
	movq	(%rsi), %rdi
	movq	%rdi, %rdx
	imulq	%rax, %rdx
	movq	40(%rcx), %rsi
	movq	%rsi, -56(%rsp)                 ## 8-byte Spill
	mulxq	%rsi, %rax, %r12
	movq	%rax, -88(%rsp)                 ## 8-byte Spill
	movq	32(%rcx), %rsi
	movq	%rsi, -64(%rsp)                 ## 8-byte Spill
	mulxq	%rsi, %rax, %r13
	movq	%rax, -48(%rsp)                 ## 8-byte Spill
	movq	24(%rcx), %rsi
	mulxq	%rsi, %r8, %r15
	movq	%rsi, %r14
	movq	%rsi, -16(%rsp)                 ## 8-byte Spill
	movq	16(%rcx), %rsi
	movq	%rsi, -72(%rsp)                 ## 8-byte Spill
	mulxq	%rsi, %rbp, %r9
	movq	(%rcx), %rax
	movq	8(%rcx), %r10
	mulxq	%r10, %rcx, %rsi
	movq	%r10, -32(%rsp)                 ## 8-byte Spill
	mulxq	%rax, %rdx, %rbx
	movq	%rax, -40(%rsp)                 ## 8-byte Spill
	addq	%rcx, %rbx
	adcq	%rbp, %rsi
	adcq	%r8, %r9
	adcq	-48(%rsp), %r15                 ## 8-byte Folded Reload
	adcq	-88(%rsp), %r13                 ## 8-byte Folded Reload
	adcq	$0, %r12
	addq	%rdi, %rdx
	movq	%r11, -24(%rsp)                 ## 8-byte Spill
	adcq	8(%r11), %rbx
	adcq	16(%r11), %rsi
	adcq	24(%r11), %r9
	adcq	32(%r11), %r15
	adcq	40(%r11), %r13
	adcq	48(%r11), %r12
	setb	-88(%rsp)                       ## 1-byte Folded Spill
	movq	-80(%rsp), %rdx                 ## 8-byte Reload
	imulq	%rbx, %rdx
	mulxq	%r14, %rcx, %rdi
	movq	%rdi, -48(%rsp)                 ## 8-byte Spill
	mulxq	%rax, %r14, %rdi
	mulxq	%r10, %rbp, %rax
	addq	%rdi, %rbp
	mulxq	-72(%rsp), %r8, %r10            ## 8-byte Folded Reload
	adcq	%rax, %r8
	adcq	%rcx, %r10
	mulxq	-64(%rsp), %rdi, %r11           ## 8-byte Folded Reload
	adcq	-48(%rsp), %rdi                 ## 8-byte Folded Reload
	mulxq	-56(%rsp), %rax, %rcx           ## 8-byte Folded Reload
	adcq	%r11, %rax
	movzbl	-88(%rsp), %edx                 ## 1-byte Folded Reload
	adcq	%rdx, %rcx
	addq	%rbx, %r14
	adcq	%rsi, %rbp
	adcq	%r9, %r8
	adcq	%r15, %r10
	adcq	%r13, %rdi
	adcq	%r12, %rax
	movq	-24(%rsp), %rdx                 ## 8-byte Reload
	adcq	56(%rdx), %rcx
	setb	-88(%rsp)                       ## 1-byte Folded Spill
	movq	-80(%rsp), %rdx                 ## 8-byte Reload
	imulq	%rbp, %rdx
	mulxq	-16(%rsp), %r11, %rsi           ## 8-byte Folded Reload
	movq	%rsi, -48(%rsp)                 ## 8-byte Spill
	mulxq	-40(%rsp), %r15, %rbx           ## 8-byte Folded Reload
	mulxq	-32(%rsp), %rsi, %r13           ## 8-byte Folded Reload
	addq	%rbx, %rsi
	mulxq	-72(%rsp), %r9, %r12            ## 8-byte Folded Reload
	adcq	%r13, %r9
	adcq	%r11, %r12
	mulxq	-64(%rsp), %r11, %r14           ## 8-byte Folded Reload
	adcq	-48(%rsp), %r11                 ## 8-byte Folded Reload
	mulxq	-56(%rsp), %rbx, %r13           ## 8-byte Folded Reload
	adcq	%r14, %rbx
	movzbl	-88(%rsp), %edx                 ## 1-byte Folded Reload
	adcq	%rdx, %r13
	addq	%rbp, %r15
	adcq	%r8, %rsi
	adcq	%r10, %r9
	adcq	%rdi, %r12
	adcq	%rax, %r11
	adcq	%rcx, %rbx
	movq	-24(%rsp), %rax                 ## 8-byte Reload
	adcq	64(%rax), %r13
	setb	-88(%rsp)                       ## 1-byte Folded Spill
	movq	-80(%rsp), %rdx                 ## 8-byte Reload
	imulq	%rsi, %rdx
	mulxq	-16(%rsp), %rbp, %r8            ## 8-byte Folded Reload
	mulxq	-40(%rsp), %r15, %rdi           ## 8-byte Folded Reload
	mulxq	-32(%rsp), %rax, %rcx           ## 8-byte Folded Reload
	addq	%rdi, %rax
	mulxq	-72(%rsp), %r10, %r14           ## 8-byte Folded Reload
	adcq	%rcx, %r10
	adcq	%rbp, %r14
	mulxq	-64(%rsp), %rbp, %rdi           ## 8-byte Folded Reload
	adcq	%r8, %rbp
	mulxq	-56(%rsp), %rcx, %r8            ## 8-byte Folded Reload
	adcq	%rdi, %rcx
	movzbl	-88(%rsp), %edx                 ## 1-byte Folded Reload
	adcq	%rdx, %r8
	addq	%rsi, %r15
	adcq	%r9, %rax
	adcq	%r12, %r10
	adcq	%r11, %r14
	adcq	%rbx, %rbp
	adcq	%r13, %rcx
	movq	-24(%rsp), %rdx                 ## 8-byte Reload
	adcq	72(%rdx), %r8
	setb	-88(%rsp)                       ## 1-byte Folded Spill
	movq	-80(%rsp), %rdx                 ## 8-byte Reload
	imulq	%rax, %rdx
	mulxq	-16(%rsp), %r15, %r13           ## 8-byte Folded Reload
	mulxq	-40(%rsp), %rbx, %rdi           ## 8-byte Folded Reload
	mulxq	-32(%rsp), %rsi, %r11           ## 8-byte Folded Reload
	addq	%rdi, %rsi
	mulxq	-72(%rsp), %r9, %r12            ## 8-byte Folded Reload
	adcq	%r11, %r9
	adcq	%r15, %r12
	mulxq	-64(%rsp), %r11, %r15           ## 8-byte Folded Reload
	adcq	%r13, %r11
	mulxq	-56(%rsp), %rdi, %r13           ## 8-byte Folded Reload
	adcq	%r15, %rdi
	movzbl	-88(%rsp), %edx                 ## 1-byte Folded Reload
	adcq	%rdx, %r13
	addq	%rax, %rbx
	adcq	%r10, %rsi
	adcq	%r14, %r9
	adcq	%rbp, %r12
	adcq	%rcx, %r11
	adcq	%r8, %rdi
	movq	-24(%rsp), %rax                 ## 8-byte Reload
	adcq	80(%rax), %r13
	setb	%r14b
	movq	-80(%rsp), %rdx                 ## 8-byte Reload
	imulq	%rsi, %rdx
	mulxq	-40(%rsp), %rax, %rcx           ## 8-byte Folded Reload
	movq	%rax, -80(%rsp)                 ## 8-byte Spill
	mulxq	-32(%rsp), %r8, %rbp            ## 8-byte Folded Reload
	addq	%rcx, %r8
	mulxq	-72(%rsp), %rbx, %r10           ## 8-byte Folded Reload
	adcq	%rbp, %rbx
	mulxq	-16(%rsp), %rcx, %r15           ## 8-byte Folded Reload
	adcq	%r10, %rcx
	mulxq	-64(%rsp), %rbp, %r10           ## 8-byte Folded Reload
	adcq	%r15, %rbp
	mulxq	-56(%rsp), %rdx, %r15           ## 8-byte Folded Reload
	adcq	%r10, %rdx
	movzbl	%r14b, %r14d
	adcq	%r15, %r14
	addq	%rsi, -80(%rsp)                 ## 8-byte Folded Spill
	adcq	%r9, %r8
	adcq	%r12, %rbx
	adcq	%r11, %rcx
	adcq	%rdi, %rbp
	adcq	%r13, %rdx
	movq	-24(%rsp), %rax                 ## 8-byte Reload
	adcq	88(%rax), %r14
	xorl	%r9d, %r9d
	movq	%r8, %r10
	subq	-40(%rsp), %r10                 ## 8-byte Folded Reload
	movq	%rbx, %r11
	sbbq	-32(%rsp), %r11                 ## 8-byte Folded Reload
	movq	%rcx, %r15
	sbbq	-72(%rsp), %r15                 ## 8-byte Folded Reload
	movq	%rbp, %r12
	sbbq	-16(%rsp), %r12                 ## 8-byte Folded Reload
	movq	%rdx, %rsi
	sbbq	-64(%rsp), %rsi                 ## 8-byte Folded Reload
	movq	%r14, %rdi
	sbbq	-56(%rsp), %rdi                 ## 8-byte Folded Reload
	sbbq	%r9, %r9
	testb	$1, %r9b
	cmovneq	%r14, %rdi
	movq	-8(%rsp), %rax                  ## 8-byte Reload
	movq	%rdi, 40(%rax)
	cmovneq	%rdx, %rsi
	movq	%rsi, 32(%rax)
	cmovneq	%rbp, %r12
	movq	%r12, 24(%rax)
	cmovneq	%rcx, %r15
	movq	%r15, 16(%rax)
	cmovneq	%rbx, %r11
	movq	%r11, 8(%rax)
	cmovneq	%r8, %r10
	movq	%r10, (%rax)
	popq	%rbx
	popq	%r12
	popq	%r13
	popq	%r14
	popq	%r15
	popq	%rbp
	retq
                                        ## -- End function
	.globl	_mcl_fp_montRedNF6Lbmi2         ## -- Begin function mcl_fp_montRedNF6Lbmi2
	.p2align	4, 0x90
_mcl_fp_montRedNF6Lbmi2:                ## @mcl_fp_montRedNF6Lbmi2
## %bb.0:
	pushq	%rbp
	pushq	%r15
	pushq	%r14
	pushq	%r13
	pushq	%r12
	pushq	%rbx
	movq	%rdx, %rcx
	movq	%rsi, %r11
	movq	%rdi, -8(%rsp)                  ## 8-byte Spill
	movq	-8(%rdx), %rax
	movq	%rax, -72(%rsp)                 ## 8-byte Spill
	movq	(%rsi), %rdi
	movq	%rdi, %rdx
	imulq	%rax, %rdx
	movq	40(%rcx), %rsi
	movq	%rsi, -48(%rsp)                 ## 8-byte Spill
	mulxq	%rsi, %rax, %r12
	movq	%rax, -88(%rsp)                 ## 8-byte Spill
	movq	32(%rcx), %rsi
	movq	%rsi, -56(%rsp)                 ## 8-byte Spill
	mulxq	%rsi, %rax, %r13
	movq	%rax, -80(%rsp)                 ## 8-byte Spill
	movq	24(%rcx), %rsi
	mulxq	%rsi, %r8, %r15
	movq	%rsi, %r14
	movq	%rsi, -16(%rsp)                 ## 8-byte Spill
	movq	16(%rcx), %rsi
	movq	%rsi, -64(%rsp)                 ## 8-byte Spill
	mulxq	%rsi, %rbp, %r9
	movq	(%rcx), %rax
	movq	8(%rcx), %r10
	mulxq	%r10, %rcx, %rsi
	movq	%r10, -32(%rsp)                 ## 8-byte Spill
	mulxq	%rax, %rdx, %rbx
	movq	%rax, -40(%rsp)                 ## 8-byte Spill
	addq	%rcx, %rbx
	adcq	%rbp, %rsi
	adcq	%r8, %r9
	adcq	-80(%rsp), %r15                 ## 8-byte Folded Reload
	adcq	-88(%rsp), %r13                 ## 8-byte Folded Reload
	adcq	$0, %r12
	addq	%rdi, %rdx
	movq	%r11, -24(%rsp)                 ## 8-byte Spill
	adcq	8(%r11), %rbx
	adcq	16(%r11), %rsi
	adcq	24(%r11), %r9
	adcq	32(%r11), %r15
	adcq	40(%r11), %r13
	adcq	48(%r11), %r12
	setb	-88(%rsp)                       ## 1-byte Folded Spill
	movq	-72(%rsp), %rdx                 ## 8-byte Reload
	imulq	%rbx, %rdx
	mulxq	%r14, %rcx, %rdi
	movq	%rdi, -80(%rsp)                 ## 8-byte Spill
	mulxq	%rax, %r14, %rdi
	mulxq	%r10, %rbp, %rax
	addq	%rdi, %rbp
	mulxq	-64(%rsp), %r8, %r10            ## 8-byte Folded Reload
	adcq	%rax, %r8
	adcq	%rcx, %r10
	mulxq	-56(%rsp), %rdi, %r11           ## 8-byte Folded Reload
	adcq	-80(%rsp), %rdi                 ## 8-byte Folded Reload
	mulxq	-48(%rsp), %rax, %rcx           ## 8-byte Folded Reload
	adcq	%r11, %rax
	movzbl	-88(%rsp), %edx                 ## 1-byte Folded Reload
	adcq	%rdx, %rcx
	addq	%rbx, %r14
	adcq	%rsi, %rbp
	adcq	%r9, %r8
	adcq	%r15, %r10
	adcq	%r13, %rdi
	adcq	%r12, %rax
	movq	-24(%rsp), %rdx                 ## 8-byte Reload
	adcq	56(%rdx), %rcx
	setb	-88(%rsp)                       ## 1-byte Folded Spill
	movq	-72(%rsp), %rdx                 ## 8-byte Reload
	imulq	%rbp, %rdx
	mulxq	-16(%rsp), %r11, %rsi           ## 8-byte Folded Reload
	movq	%rsi, -80(%rsp)                 ## 8-byte Spill
	mulxq	-40(%rsp), %r15, %rbx           ## 8-byte Folded Reload
	mulxq	-32(%rsp), %rsi, %r13           ## 8-byte Folded Reload
	addq	%rbx, %rsi
	mulxq	-64(%rsp), %r9, %r12            ## 8-byte Folded Reload
	adcq	%r13, %r9
	adcq	%r11, %r12
	mulxq	-56(%rsp), %r11, %r14           ## 8-byte Folded Reload
	adcq	-80(%rsp), %r11                 ## 8-byte Folded Reload
	mulxq	-48(%rsp), %rbx, %r13           ## 8-byte Folded Reload
	adcq	%r14, %rbx
	movzbl	-88(%rsp), %edx                 ## 1-byte Folded Reload
	adcq	%rdx, %r13
	addq	%rbp, %r15
	adcq	%r8, %rsi
	adcq	%r10, %r9
	adcq	%rdi, %r12
	adcq	%rax, %r11
	adcq	%rcx, %rbx
	movq	-24(%rsp), %rax                 ## 8-byte Reload
	adcq	64(%rax), %r13
	setb	-88(%rsp)                       ## 1-byte Folded Spill
	movq	-72(%rsp), %rdx                 ## 8-byte Reload
	imulq	%rsi, %rdx
	mulxq	-16(%rsp), %rbp, %r8            ## 8-byte Folded Reload
	mulxq	-40(%rsp), %r15, %rdi           ## 8-byte Folded Reload
	mulxq	-32(%rsp), %rax, %rcx           ## 8-byte Folded Reload
	addq	%rdi, %rax
	mulxq	-64(%rsp), %r10, %r14           ## 8-byte Folded Reload
	adcq	%rcx, %r10
	adcq	%rbp, %r14
	mulxq	-56(%rsp), %rbp, %rdi           ## 8-byte Folded Reload
	adcq	%r8, %rbp
	mulxq	-48(%rsp), %rcx, %r8            ## 8-byte Folded Reload
	adcq	%rdi, %rcx
	movzbl	-88(%rsp), %edx                 ## 1-byte Folded Reload
	adcq	%rdx, %r8
	addq	%rsi, %r15
	adcq	%r9, %rax
	adcq	%r12, %r10
	adcq	%r11, %r14
	adcq	%rbx, %rbp
	adcq	%r13, %rcx
	movq	-24(%rsp), %rdx                 ## 8-byte Reload
	adcq	72(%rdx), %r8
	setb	-88(%rsp)                       ## 1-byte Folded Spill
	movq	-72(%rsp), %rdx                 ## 8-byte Reload
	imulq	%rax, %rdx
	mulxq	-16(%rsp), %r13, %rsi           ## 8-byte Folded Reload
	movq	%rsi, -80(%rsp)                 ## 8-byte Spill
	mulxq	-40(%rsp), %r15, %rdi           ## 8-byte Folded Reload
	mulxq	-32(%rsp), %rsi, %r11           ## 8-byte Folded Reload
	addq	%rdi, %rsi
	mulxq	-64(%rsp), %r12, %r9            ## 8-byte Folded Reload
	adcq	%r11, %r12
	adcq	%r13, %r9
	mulxq	-56(%rsp), %r13, %rbx           ## 8-byte Folded Reload
	adcq	-80(%rsp), %r13                 ## 8-byte Folded Reload
	mulxq	-48(%rsp), %rdi, %r11           ## 8-byte Folded Reload
	adcq	%rbx, %rdi
	movzbl	-88(%rsp), %edx                 ## 1-byte Folded Reload
	adcq	%rdx, %r11
	addq	%rax, %r15
	adcq	%r10, %rsi
	adcq	%r14, %r12
	adcq	%rbp, %r9
	adcq	%rcx, %r13
	adcq	%r8, %rdi
	movq	-24(%rsp), %rax                 ## 8-byte Reload
	adcq	80(%rax), %r11
	setb	%r14b
	movq	-72(%rsp), %rdx                 ## 8-byte Reload
	imulq	%rsi, %rdx
	mulxq	-40(%rsp), %rax, %rcx           ## 8-byte Folded Reload
	movq	%rax, -72(%rsp)                 ## 8-byte Spill
	mulxq	-32(%rsp), %r8, %rbx            ## 8-byte Folded Reload
	addq	%rcx, %r8
	mulxq	-64(%rsp), %rcx, %r10           ## 8-byte Folded Reload
	adcq	%rbx, %rcx
	mulxq	-16(%rsp), %rbp, %r15           ## 8-byte Folded Reload
	adcq	%r10, %rbp
	mulxq	-56(%rsp), %rbx, %r10           ## 8-byte Folded Reload
	adcq	%r15, %rbx
	mulxq	-48(%rsp), %rdx, %r15           ## 8-byte Folded Reload
	adcq	%r10, %rdx
	movzbl	%r14b, %r14d
	adcq	%r15, %r14
	addq	%rsi, -72(%rsp)                 ## 8-byte Folded Spill
	adcq	%r12, %r8
	adcq	%r9, %rcx
	adcq	%r13, %rbp
	adcq	%rdi, %rbx
	adcq	%r11, %rdx
	movq	-24(%rsp), %rax                 ## 8-byte Reload
	adcq	88(%rax), %r14
	movq	%r8, %r9
	subq	-40(%rsp), %r9                  ## 8-byte Folded Reload
	movq	%rcx, %r10
	sbbq	-32(%rsp), %r10                 ## 8-byte Folded Reload
	movq	%rbp, %r11
	sbbq	-64(%rsp), %r11                 ## 8-byte Folded Reload
	movq	%rbx, %r15
	sbbq	-16(%rsp), %r15                 ## 8-byte Folded Reload
	movq	%rdx, %rax
	sbbq	-56(%rsp), %rax                 ## 8-byte Folded Reload
	movq	%r14, %rdi
	sbbq	-48(%rsp), %rdi                 ## 8-byte Folded Reload
	movq	%rdi, %rsi
	sarq	$63, %rsi
	cmovsq	%r14, %rdi
	movq	-8(%rsp), %rsi                  ## 8-byte Reload
	movq	%rdi, 40(%rsi)
	cmovsq	%rdx, %rax
	movq	%rax, 32(%rsi)
	cmovsq	%rbx, %r15
	movq	%r15, 24(%rsi)
	cmovsq	%rbp, %r11
	movq	%r11, 16(%rsi)
	cmovsq	%rcx, %r10
	movq	%r10, 8(%rsi)
	cmovsq	%r8, %r9
	movq	%r9, (%rsi)
	popq	%rbx
	popq	%r12
	popq	%r13
	popq	%r14
	popq	%r15
	popq	%rbp
	retq
                                        ## -- End function
	.globl	_mcl_fp_addPre6Lbmi2            ## -- Begin function mcl_fp_addPre6Lbmi2
	.p2align	4, 0x90
_mcl_fp_addPre6Lbmi2:                   ## @mcl_fp_addPre6Lbmi2
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
	.globl	_mcl_fp_subPre6Lbmi2            ## -- Begin function mcl_fp_subPre6Lbmi2
	.p2align	4, 0x90
_mcl_fp_subPre6Lbmi2:                   ## @mcl_fp_subPre6Lbmi2
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
	.globl	_mcl_fp_shr1_6Lbmi2             ## -- Begin function mcl_fp_shr1_6Lbmi2
	.p2align	4, 0x90
_mcl_fp_shr1_6Lbmi2:                    ## @mcl_fp_shr1_6Lbmi2
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
	.globl	_mcl_fp_add6Lbmi2               ## -- Begin function mcl_fp_add6Lbmi2
	.p2align	4, 0x90
_mcl_fp_add6Lbmi2:                      ## @mcl_fp_add6Lbmi2
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
	.globl	_mcl_fp_addNF6Lbmi2             ## -- Begin function mcl_fp_addNF6Lbmi2
	.p2align	4, 0x90
_mcl_fp_addNF6Lbmi2:                    ## @mcl_fp_addNF6Lbmi2
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
	.globl	_mcl_fp_sub6Lbmi2               ## -- Begin function mcl_fp_sub6Lbmi2
	.p2align	4, 0x90
_mcl_fp_sub6Lbmi2:                      ## @mcl_fp_sub6Lbmi2
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
	.globl	_mcl_fp_subNF6Lbmi2             ## -- Begin function mcl_fp_subNF6Lbmi2
	.p2align	4, 0x90
_mcl_fp_subNF6Lbmi2:                    ## @mcl_fp_subNF6Lbmi2
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
	.globl	_mcl_fpDbl_add6Lbmi2            ## -- Begin function mcl_fpDbl_add6Lbmi2
	.p2align	4, 0x90
_mcl_fpDbl_add6Lbmi2:                   ## @mcl_fpDbl_add6Lbmi2
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
	.globl	_mcl_fpDbl_sub6Lbmi2            ## -- Begin function mcl_fpDbl_sub6Lbmi2
	.p2align	4, 0x90
_mcl_fpDbl_sub6Lbmi2:                   ## @mcl_fpDbl_sub6Lbmi2
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
	.globl	_mulPv512x64bmi2                ## -- Begin function mulPv512x64bmi2
	.p2align	4, 0x90
_mulPv512x64bmi2:                       ## @mulPv512x64bmi2
## %bb.0:
	movq	%rdi, %rax
	mulxq	(%rsi), %rdi, %rcx
	movq	%rdi, (%rax)
	mulxq	8(%rsi), %rdi, %r8
	addq	%rcx, %rdi
	movq	%rdi, 8(%rax)
	mulxq	16(%rsi), %rdi, %r9
	adcq	%r8, %rdi
	movq	%rdi, 16(%rax)
	mulxq	24(%rsi), %rcx, %rdi
	adcq	%r9, %rcx
	movq	%rcx, 24(%rax)
	mulxq	32(%rsi), %rcx, %r8
	adcq	%rdi, %rcx
	movq	%rcx, 32(%rax)
	mulxq	40(%rsi), %rdi, %r9
	adcq	%r8, %rdi
	movq	%rdi, 40(%rax)
	mulxq	48(%rsi), %rcx, %rdi
	adcq	%r9, %rcx
	movq	%rcx, 48(%rax)
	mulxq	56(%rsi), %rcx, %rdx
	adcq	%rdi, %rcx
	movq	%rcx, 56(%rax)
	adcq	$0, %rdx
	movq	%rdx, 64(%rax)
	retq
                                        ## -- End function
	.globl	_mcl_fp_mulUnitPre8Lbmi2        ## -- Begin function mcl_fp_mulUnitPre8Lbmi2
	.p2align	4, 0x90
_mcl_fp_mulUnitPre8Lbmi2:               ## @mcl_fp_mulUnitPre8Lbmi2
## %bb.0:
	pushq	%rbx
	subq	$80, %rsp
	movq	%rdi, %rbx
	leaq	8(%rsp), %rdi
	callq	_mulPv512x64bmi2
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
	.globl	_mcl_fpDbl_mulPre8Lbmi2         ## -- Begin function mcl_fpDbl_mulPre8Lbmi2
	.p2align	4, 0x90
_mcl_fpDbl_mulPre8Lbmi2:                ## @mcl_fpDbl_mulPre8Lbmi2
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
	callq	_mulPv512x64bmi2
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
	callq	_mulPv512x64bmi2
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
	callq	_mulPv512x64bmi2
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
	callq	_mulPv512x64bmi2
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
	callq	_mulPv512x64bmi2
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
	callq	_mulPv512x64bmi2
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
	callq	_mulPv512x64bmi2
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
	callq	_mulPv512x64bmi2
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
	.globl	_mcl_fpDbl_sqrPre8Lbmi2         ## -- Begin function mcl_fpDbl_sqrPre8Lbmi2
	.p2align	4, 0x90
_mcl_fpDbl_sqrPre8Lbmi2:                ## @mcl_fpDbl_sqrPre8Lbmi2
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
	callq	_mulPv512x64bmi2
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
	callq	_mulPv512x64bmi2
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
	callq	_mulPv512x64bmi2
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
	callq	_mulPv512x64bmi2
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
	callq	_mulPv512x64bmi2
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
	callq	_mulPv512x64bmi2
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
	callq	_mulPv512x64bmi2
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
	callq	_mulPv512x64bmi2
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
	.globl	_mcl_fp_mont8Lbmi2              ## -- Begin function mcl_fp_mont8Lbmi2
	.p2align	4, 0x90
_mcl_fp_mont8Lbmi2:                     ## @mcl_fp_mont8Lbmi2
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
	callq	_mulPv512x64bmi2
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
	callq	_mulPv512x64bmi2
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
	callq	_mulPv512x64bmi2
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
	callq	_mulPv512x64bmi2
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
	callq	_mulPv512x64bmi2
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
	callq	_mulPv512x64bmi2
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
	callq	_mulPv512x64bmi2
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
	callq	_mulPv512x64bmi2
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
	callq	_mulPv512x64bmi2
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
	callq	_mulPv512x64bmi2
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
	callq	_mulPv512x64bmi2
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
	callq	_mulPv512x64bmi2
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
	callq	_mulPv512x64bmi2
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
	callq	_mulPv512x64bmi2
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
	callq	_mulPv512x64bmi2
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
	callq	_mulPv512x64bmi2
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
	.globl	_mcl_fp_montNF8Lbmi2            ## -- Begin function mcl_fp_montNF8Lbmi2
	.p2align	4, 0x90
_mcl_fp_montNF8Lbmi2:                   ## @mcl_fp_montNF8Lbmi2
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
	callq	_mulPv512x64bmi2
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
	callq	_mulPv512x64bmi2
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
	callq	_mulPv512x64bmi2
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
	callq	_mulPv512x64bmi2
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
	callq	_mulPv512x64bmi2
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
	callq	_mulPv512x64bmi2
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
	callq	_mulPv512x64bmi2
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
	callq	_mulPv512x64bmi2
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
	callq	_mulPv512x64bmi2
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
	callq	_mulPv512x64bmi2
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
	callq	_mulPv512x64bmi2
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
	callq	_mulPv512x64bmi2
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
	callq	_mulPv512x64bmi2
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
	callq	_mulPv512x64bmi2
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
	callq	_mulPv512x64bmi2
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
	callq	_mulPv512x64bmi2
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
	.globl	_mcl_fp_montRed8Lbmi2           ## -- Begin function mcl_fp_montRed8Lbmi2
	.p2align	4, 0x90
_mcl_fp_montRed8Lbmi2:                  ## @mcl_fp_montRed8Lbmi2
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
	callq	_mulPv512x64bmi2
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
	callq	_mulPv512x64bmi2
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
	callq	_mulPv512x64bmi2
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
	callq	_mulPv512x64bmi2
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
	callq	_mulPv512x64bmi2
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
	callq	_mulPv512x64bmi2
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
	callq	_mulPv512x64bmi2
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
	callq	_mulPv512x64bmi2
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
	.globl	_mcl_fp_montRedNF8Lbmi2         ## -- Begin function mcl_fp_montRedNF8Lbmi2
	.p2align	4, 0x90
_mcl_fp_montRedNF8Lbmi2:                ## @mcl_fp_montRedNF8Lbmi2
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
	callq	_mulPv512x64bmi2
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
	callq	_mulPv512x64bmi2
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
	callq	_mulPv512x64bmi2
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
	callq	_mulPv512x64bmi2
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
	callq	_mulPv512x64bmi2
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
	callq	_mulPv512x64bmi2
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
	callq	_mulPv512x64bmi2
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
	callq	_mulPv512x64bmi2
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
	.globl	_mcl_fp_addPre8Lbmi2            ## -- Begin function mcl_fp_addPre8Lbmi2
	.p2align	4, 0x90
_mcl_fp_addPre8Lbmi2:                   ## @mcl_fp_addPre8Lbmi2
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
	.globl	_mcl_fp_subPre8Lbmi2            ## -- Begin function mcl_fp_subPre8Lbmi2
	.p2align	4, 0x90
_mcl_fp_subPre8Lbmi2:                   ## @mcl_fp_subPre8Lbmi2
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
	.globl	_mcl_fp_shr1_8Lbmi2             ## -- Begin function mcl_fp_shr1_8Lbmi2
	.p2align	4, 0x90
_mcl_fp_shr1_8Lbmi2:                    ## @mcl_fp_shr1_8Lbmi2
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
	.globl	_mcl_fp_add8Lbmi2               ## -- Begin function mcl_fp_add8Lbmi2
	.p2align	4, 0x90
_mcl_fp_add8Lbmi2:                      ## @mcl_fp_add8Lbmi2
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
	.globl	_mcl_fp_addNF8Lbmi2             ## -- Begin function mcl_fp_addNF8Lbmi2
	.p2align	4, 0x90
_mcl_fp_addNF8Lbmi2:                    ## @mcl_fp_addNF8Lbmi2
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
	.globl	_mcl_fp_sub8Lbmi2               ## -- Begin function mcl_fp_sub8Lbmi2
	.p2align	4, 0x90
_mcl_fp_sub8Lbmi2:                      ## @mcl_fp_sub8Lbmi2
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
	.globl	_mcl_fp_subNF8Lbmi2             ## -- Begin function mcl_fp_subNF8Lbmi2
	.p2align	4, 0x90
_mcl_fp_subNF8Lbmi2:                    ## @mcl_fp_subNF8Lbmi2
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
	.globl	_mcl_fpDbl_add8Lbmi2            ## -- Begin function mcl_fpDbl_add8Lbmi2
	.p2align	4, 0x90
_mcl_fpDbl_add8Lbmi2:                   ## @mcl_fpDbl_add8Lbmi2
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
	.globl	_mcl_fpDbl_sub8Lbmi2            ## -- Begin function mcl_fpDbl_sub8Lbmi2
	.p2align	4, 0x90
_mcl_fpDbl_sub8Lbmi2:                   ## @mcl_fpDbl_sub8Lbmi2
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
