	.text
	.file	"base32.ll"
	.globl	makeNIST_P192L                  # -- Begin function makeNIST_P192L
	.p2align	4, 0x90
	.type	makeNIST_P192L,@function
makeNIST_P192L:                         # @makeNIST_P192L
# %bb.0:
	movl	4(%esp), %eax
	movl	$-1, 20(%eax)
	movl	$-1, 16(%eax)
	movl	$-1, 12(%eax)
	movl	$-2, 8(%eax)
	movl	$-1, 4(%eax)
	movl	$-1, (%eax)
	retl	$4
.Lfunc_end0:
	.size	makeNIST_P192L, .Lfunc_end0-makeNIST_P192L
                                        # -- End function
	.globl	mcl_fpDbl_mod_NIST_P192L        # -- Begin function mcl_fpDbl_mod_NIST_P192L
	.p2align	4, 0x90
	.type	mcl_fpDbl_mod_NIST_P192L,@function
mcl_fpDbl_mod_NIST_P192L:               # @mcl_fpDbl_mod_NIST_P192L
# %bb.0:
	pushl	%ebp
	pushl	%ebx
	pushl	%edi
	pushl	%esi
	subl	$36, %esp
	movl	60(%esp), %esi
	movl	32(%esi), %ecx
	movl	%ecx, 12(%esp)                  # 4-byte Spill
	movl	24(%esi), %edi
	movl	%edi, 20(%esp)                  # 4-byte Spill
	movl	28(%esi), %edx
	movl	%edx, 16(%esp)                  # 4-byte Spill
	movl	(%esi), %eax
	addl	%edi, %eax
	movl	%eax, %ebx
	movl	4(%esi), %eax
	adcl	%edx, %eax
	movl	%eax, %edx
	movl	8(%esi), %ebp
	adcl	%ecx, %ebp
	movl	36(%esi), %eax
	movl	%eax, 8(%esp)                   # 4-byte Spill
	movl	12(%esi), %ecx
	adcl	%eax, %ecx
	movl	40(%esi), %edi
	movl	%edi, 4(%esp)                   # 4-byte Spill
	movl	16(%esi), %eax
	adcl	%edi, %eax
	movl	44(%esi), %edi
	movl	%edi, 32(%esp)                  # 4-byte Spill
	movl	20(%esi), %edi
	movl	32(%esp), %esi                  # 4-byte Reload
	adcl	%esi, %edi
	setb	3(%esp)                         # 1-byte Folded Spill
	addl	4(%esp), %ebx                   # 4-byte Folded Reload
	movl	%ebx, 24(%esp)                  # 4-byte Spill
	adcl	%esi, %edx
	movl	%edx, 28(%esp)                  # 4-byte Spill
	adcl	20(%esp), %ebp                  # 4-byte Folded Reload
	adcl	16(%esp), %ecx                  # 4-byte Folded Reload
	adcl	12(%esp), %eax                  # 4-byte Folded Reload
	adcl	8(%esp), %edi                   # 4-byte Folded Reload
	movzbl	3(%esp), %ebx                   # 1-byte Folded Reload
	adcl	$0, %ebx
	setb	%dl
	addl	4(%esp), %ebp                   # 4-byte Folded Reload
	adcl	%esi, %ecx
	adcl	$0, %eax
	adcl	$0, %edi
	setb	4(%esp)                         # 1-byte Folded Spill
	movl	%ebx, %esi
	adcl	$0, %esi
	movzbl	%dl, %edx
	adcl	$0, %edx
	addb	$255, 4(%esp)                   # 1-byte Folded Spill
	adcl	24(%esp), %ebx                  # 4-byte Folded Reload
	adcl	%edx, 28(%esp)                  # 4-byte Folded Spill
	adcl	%ebp, %esi
	adcl	%ecx, %edx
	adcl	$0, %eax
	adcl	$0, %edi
	setb	24(%esp)                        # 1-byte Folded Spill
	movl	%ebx, 20(%esp)                  # 4-byte Spill
	addl	$1, %ebx
	movl	28(%esp), %ecx                  # 4-byte Reload
	adcl	$0, %ecx
	movl	%ecx, 4(%esp)                   # 4-byte Spill
	movl	%esi, 16(%esp)                  # 4-byte Spill
	movl	%esi, %ebp
	adcl	$1, %ebp
	movl	%edx, 12(%esp)                  # 4-byte Spill
	adcl	$0, %edx
	movl	%eax, 8(%esp)                   # 4-byte Spill
	adcl	$0, %eax
	movl	%edi, %esi
	adcl	$0, %esi
	movzbl	24(%esp), %ecx                  # 1-byte Folded Reload
	adcl	$-1, %ecx
	testb	$1, %cl
	jne	.LBB1_1
# %bb.2:
	movl	56(%esp), %edi
	movl	%esi, 20(%edi)
	jne	.LBB1_3
.LBB1_4:
	movl	%eax, 16(%edi)
	jne	.LBB1_5
.LBB1_6:
	movl	%edx, 12(%edi)
	movl	4(%esp), %eax                   # 4-byte Reload
	jne	.LBB1_7
.LBB1_8:
	movl	%ebp, 8(%edi)
	jne	.LBB1_9
.LBB1_10:
	movl	%eax, 4(%edi)
	je	.LBB1_12
.LBB1_11:
	movl	20(%esp), %ebx                  # 4-byte Reload
.LBB1_12:
	movl	%ebx, (%edi)
	addl	$36, %esp
	popl	%esi
	popl	%edi
	popl	%ebx
	popl	%ebp
	retl
.LBB1_1:
	movl	%edi, %esi
	movl	56(%esp), %edi
	movl	%esi, 20(%edi)
	je	.LBB1_4
.LBB1_3:
	movl	8(%esp), %eax                   # 4-byte Reload
	movl	%eax, 16(%edi)
	je	.LBB1_6
.LBB1_5:
	movl	12(%esp), %edx                  # 4-byte Reload
	movl	%edx, 12(%edi)
	movl	4(%esp), %eax                   # 4-byte Reload
	je	.LBB1_8
.LBB1_7:
	movl	16(%esp), %ebp                  # 4-byte Reload
	movl	%ebp, 8(%edi)
	je	.LBB1_10
.LBB1_9:
	movl	28(%esp), %eax                  # 4-byte Reload
	movl	%eax, 4(%edi)
	jne	.LBB1_11
	jmp	.LBB1_12
.Lfunc_end1:
	.size	mcl_fpDbl_mod_NIST_P192L, .Lfunc_end1-mcl_fpDbl_mod_NIST_P192L
                                        # -- End function
	.globl	mcl_fp_sqr_NIST_P192L           # -- Begin function mcl_fp_sqr_NIST_P192L
	.p2align	4, 0x90
	.type	mcl_fp_sqr_NIST_P192L,@function
mcl_fp_sqr_NIST_P192L:                  # @mcl_fp_sqr_NIST_P192L
# %bb.0:
	pushl	%ebp
	pushl	%ebx
	pushl	%edi
	pushl	%esi
	subl	$92, %esp
	calll	.L2$pb
.L2$pb:
	popl	%ebx
.Ltmp0:
	addl	$_GLOBAL_OFFSET_TABLE_+(.Ltmp0-.L2$pb), %ebx
	subl	$8, %esp
	movl	124(%esp), %eax
	leal	52(%esp), %ecx
	pushl	%eax
	pushl	%ecx
	calll	mcl_fpDbl_sqrPre6L@PLT
	addl	$16, %esp
	movl	80(%esp), %ecx
	movl	%ecx, 24(%esp)                  # 4-byte Spill
	movl	76(%esp), %esi
	movl	%esi, 28(%esp)                  # 4-byte Spill
	movl	68(%esp), %edi
	movl	%edi, 16(%esp)                  # 4-byte Spill
	movl	72(%esp), %edx
	movl	%edx, 32(%esp)                  # 4-byte Spill
	movl	44(%esp), %eax
	addl	%edi, %eax
	movl	%eax, %ebx
	movl	48(%esp), %eax
	adcl	%edx, %eax
	movl	%eax, %edx
	movl	52(%esp), %eax
	adcl	%esi, %eax
	movl	%eax, 20(%esp)                  # 4-byte Spill
	movl	56(%esp), %ebp
	adcl	%ecx, %ebp
	movl	84(%esp), %esi
	movl	60(%esp), %ecx
	adcl	%esi, %ecx
	movl	88(%esp), %eax
	movl	64(%esp), %edi
	adcl	%eax, %edi
	setb	15(%esp)                        # 1-byte Folded Spill
	addl	%esi, %ebx
	movl	%ebx, 36(%esp)                  # 4-byte Spill
	adcl	%eax, %edx
	movl	%edx, 40(%esp)                  # 4-byte Spill
	movl	20(%esp), %edx                  # 4-byte Reload
	adcl	16(%esp), %edx                  # 4-byte Folded Reload
	adcl	32(%esp), %ebp                  # 4-byte Folded Reload
	adcl	28(%esp), %ecx                  # 4-byte Folded Reload
	adcl	24(%esp), %edi                  # 4-byte Folded Reload
	movzbl	15(%esp), %ebx                  # 1-byte Folded Reload
	adcl	$0, %ebx
	setb	16(%esp)                        # 1-byte Folded Spill
	addl	%esi, %edx
	movl	%edx, 20(%esp)                  # 4-byte Spill
	adcl	%eax, %ebp
	adcl	$0, %ecx
	adcl	$0, %edi
	setb	%al
	movl	%ebx, %esi
	adcl	$0, %esi
	movzbl	16(%esp), %edx                  # 1-byte Folded Reload
	adcl	$0, %edx
	addb	$255, %al
	adcl	36(%esp), %ebx                  # 4-byte Folded Reload
	adcl	%edx, 40(%esp)                  # 4-byte Folded Spill
	adcl	20(%esp), %esi                  # 4-byte Folded Reload
	adcl	%ebp, %edx
	adcl	$0, %ecx
	adcl	$0, %edi
	setb	36(%esp)                        # 1-byte Folded Spill
	movl	%ebx, 16(%esp)                  # 4-byte Spill
	addl	$1, %ebx
	movl	%ebx, 20(%esp)                  # 4-byte Spill
	movl	40(%esp), %ebp                  # 4-byte Reload
	adcl	$0, %ebp
	movl	%esi, 24(%esp)                  # 4-byte Spill
	movl	%esi, %ebx
	adcl	$1, %ebx
	movl	%edx, 32(%esp)                  # 4-byte Spill
	adcl	$0, %edx
	movl	%ecx, 28(%esp)                  # 4-byte Spill
	movl	%ecx, %eax
	adcl	$0, %eax
	movl	%edi, %esi
	adcl	$0, %esi
	movzbl	36(%esp), %ecx                  # 1-byte Folded Reload
	adcl	$-1, %ecx
	testb	$1, %cl
	jne	.LBB2_1
# %bb.2:
	movl	112(%esp), %edi
	movl	%esi, 20(%edi)
	jne	.LBB2_3
.LBB2_4:
	movl	%eax, 16(%edi)
	jne	.LBB2_5
.LBB2_6:
	movl	%edx, 12(%edi)
	movl	20(%esp), %eax                  # 4-byte Reload
	jne	.LBB2_7
.LBB2_8:
	movl	%ebx, 8(%edi)
	jne	.LBB2_9
.LBB2_10:
	movl	%ebp, 4(%edi)
	je	.LBB2_12
.LBB2_11:
	movl	16(%esp), %eax                  # 4-byte Reload
.LBB2_12:
	movl	%eax, (%edi)
	addl	$92, %esp
	popl	%esi
	popl	%edi
	popl	%ebx
	popl	%ebp
	retl
.LBB2_1:
	movl	%edi, %esi
	movl	112(%esp), %edi
	movl	%esi, 20(%edi)
	je	.LBB2_4
.LBB2_3:
	movl	28(%esp), %eax                  # 4-byte Reload
	movl	%eax, 16(%edi)
	je	.LBB2_6
.LBB2_5:
	movl	32(%esp), %edx                  # 4-byte Reload
	movl	%edx, 12(%edi)
	movl	20(%esp), %eax                  # 4-byte Reload
	je	.LBB2_8
.LBB2_7:
	movl	24(%esp), %ebx                  # 4-byte Reload
	movl	%ebx, 8(%edi)
	je	.LBB2_10
.LBB2_9:
	movl	40(%esp), %ebp                  # 4-byte Reload
	movl	%ebp, 4(%edi)
	jne	.LBB2_11
	jmp	.LBB2_12
.Lfunc_end2:
	.size	mcl_fp_sqr_NIST_P192L, .Lfunc_end2-mcl_fp_sqr_NIST_P192L
                                        # -- End function
	.globl	mcl_fp_mulNIST_P192L            # -- Begin function mcl_fp_mulNIST_P192L
	.p2align	4, 0x90
	.type	mcl_fp_mulNIST_P192L,@function
mcl_fp_mulNIST_P192L:                   # @mcl_fp_mulNIST_P192L
# %bb.0:
	pushl	%ebp
	pushl	%ebx
	pushl	%edi
	pushl	%esi
	subl	$92, %esp
	calll	.L3$pb
.L3$pb:
	popl	%ebx
.Ltmp1:
	addl	$_GLOBAL_OFFSET_TABLE_+(.Ltmp1-.L3$pb), %ebx
	subl	$4, %esp
	movl	124(%esp), %eax
	movl	120(%esp), %ecx
	leal	48(%esp), %edx
	pushl	%eax
	pushl	%ecx
	pushl	%edx
	calll	mcl_fpDbl_mulPre6L@PLT
	addl	$16, %esp
	movl	80(%esp), %ecx
	movl	%ecx, 24(%esp)                  # 4-byte Spill
	movl	76(%esp), %esi
	movl	%esi, 28(%esp)                  # 4-byte Spill
	movl	68(%esp), %edi
	movl	%edi, 16(%esp)                  # 4-byte Spill
	movl	72(%esp), %edx
	movl	%edx, 32(%esp)                  # 4-byte Spill
	movl	44(%esp), %eax
	addl	%edi, %eax
	movl	%eax, %ebx
	movl	48(%esp), %eax
	adcl	%edx, %eax
	movl	%eax, %edx
	movl	52(%esp), %eax
	adcl	%esi, %eax
	movl	%eax, 20(%esp)                  # 4-byte Spill
	movl	56(%esp), %ebp
	adcl	%ecx, %ebp
	movl	84(%esp), %esi
	movl	60(%esp), %ecx
	adcl	%esi, %ecx
	movl	88(%esp), %eax
	movl	64(%esp), %edi
	adcl	%eax, %edi
	setb	15(%esp)                        # 1-byte Folded Spill
	addl	%esi, %ebx
	movl	%ebx, 36(%esp)                  # 4-byte Spill
	adcl	%eax, %edx
	movl	%edx, 40(%esp)                  # 4-byte Spill
	movl	20(%esp), %edx                  # 4-byte Reload
	adcl	16(%esp), %edx                  # 4-byte Folded Reload
	adcl	32(%esp), %ebp                  # 4-byte Folded Reload
	adcl	28(%esp), %ecx                  # 4-byte Folded Reload
	adcl	24(%esp), %edi                  # 4-byte Folded Reload
	movzbl	15(%esp), %ebx                  # 1-byte Folded Reload
	adcl	$0, %ebx
	setb	16(%esp)                        # 1-byte Folded Spill
	addl	%esi, %edx
	movl	%edx, 20(%esp)                  # 4-byte Spill
	adcl	%eax, %ebp
	adcl	$0, %ecx
	adcl	$0, %edi
	setb	%al
	movl	%ebx, %esi
	adcl	$0, %esi
	movzbl	16(%esp), %edx                  # 1-byte Folded Reload
	adcl	$0, %edx
	addb	$255, %al
	adcl	36(%esp), %ebx                  # 4-byte Folded Reload
	adcl	%edx, 40(%esp)                  # 4-byte Folded Spill
	adcl	20(%esp), %esi                  # 4-byte Folded Reload
	adcl	%ebp, %edx
	adcl	$0, %ecx
	adcl	$0, %edi
	setb	36(%esp)                        # 1-byte Folded Spill
	movl	%ebx, 16(%esp)                  # 4-byte Spill
	addl	$1, %ebx
	movl	%ebx, 20(%esp)                  # 4-byte Spill
	movl	40(%esp), %ebp                  # 4-byte Reload
	adcl	$0, %ebp
	movl	%esi, 24(%esp)                  # 4-byte Spill
	movl	%esi, %ebx
	adcl	$1, %ebx
	movl	%edx, 32(%esp)                  # 4-byte Spill
	adcl	$0, %edx
	movl	%ecx, 28(%esp)                  # 4-byte Spill
	movl	%ecx, %eax
	adcl	$0, %eax
	movl	%edi, %esi
	adcl	$0, %esi
	movzbl	36(%esp), %ecx                  # 1-byte Folded Reload
	adcl	$-1, %ecx
	testb	$1, %cl
	jne	.LBB3_1
# %bb.2:
	movl	112(%esp), %edi
	movl	%esi, 20(%edi)
	jne	.LBB3_3
.LBB3_4:
	movl	%eax, 16(%edi)
	jne	.LBB3_5
.LBB3_6:
	movl	%edx, 12(%edi)
	movl	20(%esp), %eax                  # 4-byte Reload
	jne	.LBB3_7
.LBB3_8:
	movl	%ebx, 8(%edi)
	jne	.LBB3_9
.LBB3_10:
	movl	%ebp, 4(%edi)
	je	.LBB3_12
.LBB3_11:
	movl	16(%esp), %eax                  # 4-byte Reload
.LBB3_12:
	movl	%eax, (%edi)
	addl	$92, %esp
	popl	%esi
	popl	%edi
	popl	%ebx
	popl	%ebp
	retl
.LBB3_1:
	movl	%edi, %esi
	movl	112(%esp), %edi
	movl	%esi, 20(%edi)
	je	.LBB3_4
.LBB3_3:
	movl	28(%esp), %eax                  # 4-byte Reload
	movl	%eax, 16(%edi)
	je	.LBB3_6
.LBB3_5:
	movl	32(%esp), %edx                  # 4-byte Reload
	movl	%edx, 12(%edi)
	movl	20(%esp), %eax                  # 4-byte Reload
	je	.LBB3_8
.LBB3_7:
	movl	24(%esp), %ebx                  # 4-byte Reload
	movl	%ebx, 8(%edi)
	je	.LBB3_10
.LBB3_9:
	movl	40(%esp), %ebp                  # 4-byte Reload
	movl	%ebp, 4(%edi)
	jne	.LBB3_11
	jmp	.LBB3_12
.Lfunc_end3:
	.size	mcl_fp_mulNIST_P192L, .Lfunc_end3-mcl_fp_mulNIST_P192L
                                        # -- End function
	.globl	mcl_fpDbl_mod_NIST_P521L        # -- Begin function mcl_fpDbl_mod_NIST_P521L
	.p2align	4, 0x90
	.type	mcl_fpDbl_mod_NIST_P521L,@function
mcl_fpDbl_mod_NIST_P521L:               # @mcl_fpDbl_mod_NIST_P521L
# %bb.0:
	pushl	%ebp
	pushl	%ebx
	pushl	%edi
	pushl	%esi
	subl	$60, %esp
	movl	84(%esp), %edi
	movl	124(%edi), %eax
	movl	128(%edi), %ecx
	movl	%ecx, 44(%esp)                  # 4-byte Spill
	shldl	$23, %eax, %ecx
	movl	%ecx, (%esp)                    # 4-byte Spill
	movl	120(%edi), %ecx
	shldl	$23, %ecx, %eax
	movl	%eax, 40(%esp)                  # 4-byte Spill
	movl	116(%edi), %eax
	shldl	$23, %eax, %ecx
	movl	%ecx, 36(%esp)                  # 4-byte Spill
	movl	112(%edi), %ecx
	shldl	$23, %ecx, %eax
	movl	%eax, 32(%esp)                  # 4-byte Spill
	movl	108(%edi), %eax
	shldl	$23, %eax, %ecx
	movl	%ecx, 28(%esp)                  # 4-byte Spill
	movl	104(%edi), %ecx
	shldl	$23, %ecx, %eax
	movl	%eax, 24(%esp)                  # 4-byte Spill
	movl	100(%edi), %eax
	shldl	$23, %eax, %ecx
	movl	%ecx, 20(%esp)                  # 4-byte Spill
	movl	96(%edi), %ecx
	shldl	$23, %ecx, %eax
	movl	%eax, 16(%esp)                  # 4-byte Spill
	movl	92(%edi), %eax
	shldl	$23, %eax, %ecx
	movl	%ecx, 12(%esp)                  # 4-byte Spill
	movl	88(%edi), %ecx
	shldl	$23, %ecx, %eax
	movl	%eax, 8(%esp)                   # 4-byte Spill
	movl	84(%edi), %esi
	shldl	$23, %esi, %ecx
	movl	%ecx, 4(%esp)                   # 4-byte Spill
	movl	80(%edi), %ecx
	shldl	$23, %ecx, %esi
	movl	76(%edi), %eax
	shldl	$23, %eax, %ecx
	movl	72(%edi), %edx
	shldl	$23, %edx, %eax
	movl	68(%edi), %ebp
	shldl	$23, %ebp, %edx
	movl	%edx, %ebx
	shrl	$9, 44(%esp)                    # 4-byte Folded Spill
	movl	64(%edi), %edx
	shldl	$23, %edx, %ebp
	andl	$511, %edx                      # imm = 0x1FF
	addl	(%edi), %ebp
	adcl	4(%edi), %ebx
	movl	%ebx, 48(%esp)                  # 4-byte Spill
	adcl	8(%edi), %eax
	adcl	12(%edi), %ecx
	adcl	16(%edi), %esi
	movl	4(%esp), %ebx                   # 4-byte Reload
	adcl	20(%edi), %ebx
	movl	%ebx, 4(%esp)                   # 4-byte Spill
	movl	8(%esp), %ebx                   # 4-byte Reload
	adcl	24(%edi), %ebx
	movl	%ebx, 8(%esp)                   # 4-byte Spill
	movl	12(%esp), %ebx                  # 4-byte Reload
	adcl	28(%edi), %ebx
	movl	%ebx, 12(%esp)                  # 4-byte Spill
	movl	16(%esp), %ebx                  # 4-byte Reload
	adcl	32(%edi), %ebx
	movl	%ebx, 16(%esp)                  # 4-byte Spill
	movl	20(%esp), %ebx                  # 4-byte Reload
	adcl	36(%edi), %ebx
	movl	%ebx, 20(%esp)                  # 4-byte Spill
	movl	24(%esp), %ebx                  # 4-byte Reload
	adcl	40(%edi), %ebx
	movl	%ebx, 24(%esp)                  # 4-byte Spill
	movl	28(%esp), %ebx                  # 4-byte Reload
	adcl	44(%edi), %ebx
	movl	%ebx, 28(%esp)                  # 4-byte Spill
	movl	32(%esp), %ebx                  # 4-byte Reload
	adcl	48(%edi), %ebx
	movl	%ebx, 32(%esp)                  # 4-byte Spill
	movl	36(%esp), %ebx                  # 4-byte Reload
	adcl	52(%edi), %ebx
	movl	%ebx, 36(%esp)                  # 4-byte Spill
	movl	40(%esp), %ebx                  # 4-byte Reload
	adcl	56(%edi), %ebx
	movl	%ebx, 40(%esp)                  # 4-byte Spill
	movl	(%esp), %ebx                    # 4-byte Reload
	adcl	60(%edi), %ebx
	movl	%ebx, (%esp)                    # 4-byte Spill
	adcl	44(%esp), %edx                  # 4-byte Folded Reload
	movl	%edx, %edi
	shrl	$9, %edi
	andl	$1, %edi
	addl	%ebp, %edi
	adcl	$0, 48(%esp)                    # 4-byte Folded Spill
	adcl	$0, %eax
	adcl	$0, %ecx
	adcl	$0, %esi
	adcl	$0, 4(%esp)                     # 4-byte Folded Spill
	adcl	$0, 8(%esp)                     # 4-byte Folded Spill
	adcl	$0, 12(%esp)                    # 4-byte Folded Spill
	adcl	$0, 16(%esp)                    # 4-byte Folded Spill
	adcl	$0, 20(%esp)                    # 4-byte Folded Spill
	adcl	$0, 24(%esp)                    # 4-byte Folded Spill
	adcl	$0, 28(%esp)                    # 4-byte Folded Spill
	adcl	$0, 32(%esp)                    # 4-byte Folded Spill
	movl	36(%esp), %ebx                  # 4-byte Reload
	adcl	$0, %ebx
	adcl	$0, 40(%esp)                    # 4-byte Folded Spill
	movl	(%esp), %ebp                    # 4-byte Reload
	adcl	$0, %ebp
	adcl	$0, %edx
	movl	%edi, 52(%esp)                  # 4-byte Spill
	movl	%eax, 56(%esp)                  # 4-byte Spill
	andl	%eax, %edi
	movl	%ecx, 44(%esp)                  # 4-byte Spill
	andl	%ecx, %edi
	movl	%esi, (%esp)                    # 4-byte Spill
	andl	%esi, %edi
	movl	48(%esp), %esi                  # 4-byte Reload
	andl	4(%esp), %edi                   # 4-byte Folded Reload
	andl	8(%esp), %edi                   # 4-byte Folded Reload
	andl	12(%esp), %edi                  # 4-byte Folded Reload
	andl	16(%esp), %edi                  # 4-byte Folded Reload
	andl	20(%esp), %edi                  # 4-byte Folded Reload
	andl	24(%esp), %edi                  # 4-byte Folded Reload
	andl	28(%esp), %edi                  # 4-byte Folded Reload
	andl	32(%esp), %edi                  # 4-byte Folded Reload
	movl	%ebx, %eax
	movl	%ebx, 36(%esp)                  # 4-byte Spill
	andl	%ebx, %edi
	andl	40(%esp), %edi                  # 4-byte Folded Reload
	movl	%ebp, %ecx
	andl	%ebp, %edi
	movl	%edx, %ebp
	orl	$-512, %ebp                     # imm = 0xFE00
	andl	%edi, %ebp
	andl	%esi, %ebp
	cmpl	$-1, %ebp
	movl	80(%esp), %edi
	je	.LBB4_1
# %bb.3:                                # %nonzero
	movl	%ecx, 60(%edi)
	movl	40(%esp), %ecx                  # 4-byte Reload
	movl	%ecx, 56(%edi)
	movl	36(%esp), %eax                  # 4-byte Reload
	movl	%eax, 52(%edi)
	movl	32(%esp), %eax                  # 4-byte Reload
	movl	%eax, 48(%edi)
	movl	28(%esp), %eax                  # 4-byte Reload
	movl	%eax, 44(%edi)
	movl	24(%esp), %eax                  # 4-byte Reload
	movl	%eax, 40(%edi)
	movl	20(%esp), %eax                  # 4-byte Reload
	movl	%eax, 36(%edi)
	movl	16(%esp), %eax                  # 4-byte Reload
	movl	%eax, 32(%edi)
	movl	12(%esp), %eax                  # 4-byte Reload
	movl	%eax, 28(%edi)
	movl	8(%esp), %eax                   # 4-byte Reload
	movl	%eax, 24(%edi)
	movl	4(%esp), %eax                   # 4-byte Reload
	movl	%eax, 20(%edi)
	movl	(%esp), %eax                    # 4-byte Reload
	movl	%eax, 16(%edi)
	movl	44(%esp), %eax                  # 4-byte Reload
	movl	%eax, 12(%edi)
	movl	56(%esp), %eax                  # 4-byte Reload
	movl	%eax, 8(%edi)
	movl	%esi, 4(%edi)
	movl	52(%esp), %eax                  # 4-byte Reload
	movl	%eax, (%edi)
	andl	$511, %edx                      # imm = 0x1FF
	movl	%edx, 64(%edi)
	jmp	.LBB4_2
.LBB4_1:                                # %zero
	xorl	%eax, %eax
	movl	$17, %ecx
	rep;stosl %eax, %es:(%edi)
.LBB4_2:                                # %zero
	addl	$60, %esp
	popl	%esi
	popl	%edi
	popl	%ebx
	popl	%ebp
	retl
.Lfunc_end4:
	.size	mcl_fpDbl_mod_NIST_P521L, .Lfunc_end4-mcl_fpDbl_mod_NIST_P521L
                                        # -- End function
	.globl	mulPv192x32                     # -- Begin function mulPv192x32
	.p2align	4, 0x90
	.type	mulPv192x32,@function
mulPv192x32:                            # @mulPv192x32
# %bb.0:
	pushl	%ebp
	pushl	%ebx
	pushl	%edi
	pushl	%esi
	subl	$28, %esp
	movl	56(%esp), %ebx
	movl	52(%esp), %edi
	movl	%ebx, %eax
	mull	20(%edi)
	movl	%edx, 24(%esp)                  # 4-byte Spill
	movl	%eax, 20(%esp)                  # 4-byte Spill
	movl	%ebx, %eax
	mull	16(%edi)
	movl	%edx, 16(%esp)                  # 4-byte Spill
	movl	%eax, 12(%esp)                  # 4-byte Spill
	movl	%ebx, %eax
	mull	12(%edi)
	movl	%edx, %esi
	movl	%eax, 8(%esp)                   # 4-byte Spill
	movl	%ebx, %eax
	mull	8(%edi)
	movl	%edx, %ebp
	movl	%eax, 4(%esp)                   # 4-byte Spill
	movl	%ebx, %eax
	mull	4(%edi)
	movl	%edx, %ecx
	movl	%eax, (%esp)                    # 4-byte Spill
	movl	%ebx, %eax
	mull	(%edi)
	movl	48(%esp), %ebx
	movl	%eax, (%ebx)
	addl	(%esp), %edx                    # 4-byte Folded Reload
	movl	%edx, 4(%ebx)
	adcl	4(%esp), %ecx                   # 4-byte Folded Reload
	movl	%ecx, 8(%ebx)
	adcl	8(%esp), %ebp                   # 4-byte Folded Reload
	movl	%ebp, 12(%ebx)
	adcl	12(%esp), %esi                  # 4-byte Folded Reload
	movl	%esi, 16(%ebx)
	movl	16(%esp), %eax                  # 4-byte Reload
	adcl	20(%esp), %eax                  # 4-byte Folded Reload
	movl	%eax, 20(%ebx)
	movl	24(%esp), %eax                  # 4-byte Reload
	adcl	$0, %eax
	movl	%eax, 24(%ebx)
	movl	%ebx, %eax
	addl	$28, %esp
	popl	%esi
	popl	%edi
	popl	%ebx
	popl	%ebp
	retl	$4
.Lfunc_end5:
	.size	mulPv192x32, .Lfunc_end5-mulPv192x32
                                        # -- End function
	.globl	mcl_fp_mulUnitPre6L             # -- Begin function mcl_fp_mulUnitPre6L
	.p2align	4, 0x90
	.type	mcl_fp_mulUnitPre6L,@function
mcl_fp_mulUnitPre6L:                    # @mcl_fp_mulUnitPre6L
# %bb.0:
	pushl	%ebp
	pushl	%ebx
	pushl	%edi
	pushl	%esi
	subl	$28, %esp
	movl	56(%esp), %ebx
	movl	52(%esp), %edi
	movl	%ebx, %eax
	mull	20(%edi)
	movl	%edx, 24(%esp)                  # 4-byte Spill
	movl	%eax, 20(%esp)                  # 4-byte Spill
	movl	%ebx, %eax
	mull	16(%edi)
	movl	%edx, 16(%esp)                  # 4-byte Spill
	movl	%eax, 12(%esp)                  # 4-byte Spill
	movl	%ebx, %eax
	mull	12(%edi)
	movl	%edx, %esi
	movl	%eax, 8(%esp)                   # 4-byte Spill
	movl	%ebx, %eax
	mull	8(%edi)
	movl	%edx, %ebp
	movl	%eax, 4(%esp)                   # 4-byte Spill
	movl	%ebx, %eax
	mull	4(%edi)
	movl	%edx, %ecx
	movl	%eax, (%esp)                    # 4-byte Spill
	movl	%ebx, %eax
	mull	(%edi)
	movl	48(%esp), %edi
	movl	%eax, (%edi)
	addl	(%esp), %edx                    # 4-byte Folded Reload
	movl	%edx, 4(%edi)
	adcl	4(%esp), %ecx                   # 4-byte Folded Reload
	movl	%ecx, 8(%edi)
	adcl	8(%esp), %ebp                   # 4-byte Folded Reload
	movl	%ebp, 12(%edi)
	adcl	12(%esp), %esi                  # 4-byte Folded Reload
	movl	%esi, 16(%edi)
	movl	16(%esp), %eax                  # 4-byte Reload
	adcl	20(%esp), %eax                  # 4-byte Folded Reload
	movl	%eax, 20(%edi)
	movl	24(%esp), %eax                  # 4-byte Reload
	adcl	$0, %eax
	movl	%eax, 24(%edi)
	addl	$28, %esp
	popl	%esi
	popl	%edi
	popl	%ebx
	popl	%ebp
	retl
.Lfunc_end6:
	.size	mcl_fp_mulUnitPre6L, .Lfunc_end6-mcl_fp_mulUnitPre6L
                                        # -- End function
	.globl	mcl_fpDbl_mulPre6L              # -- Begin function mcl_fpDbl_mulPre6L
	.p2align	4, 0x90
	.type	mcl_fpDbl_mulPre6L,@function
mcl_fpDbl_mulPre6L:                     # @mcl_fpDbl_mulPre6L
# %bb.0:
	pushl	%ebp
	pushl	%ebx
	pushl	%edi
	pushl	%esi
	subl	$96, %esp
	movl	120(%esp), %ecx
	movl	(%ecx), %eax
	movl	%eax, 72(%esp)                  # 4-byte Spill
	movl	124(%esp), %edx
	movl	(%edx), %esi
	mull	%esi
	movl	%edx, 48(%esp)                  # 4-byte Spill
	movl	116(%esp), %edx
	movl	%eax, (%edx)
	movl	4(%ecx), %eax
	movl	%eax, 56(%esp)                  # 4-byte Spill
	movl	8(%ecx), %edi
	movl	12(%ecx), %ebp
	movl	%ebp, 60(%esp)                  # 4-byte Spill
	movl	16(%ecx), %ebx
	movl	20(%ecx), %edx
	movl	%edx, 32(%esp)                  # 4-byte Spill
	movl	124(%esp), %eax
	movl	4(%eax), %ecx
	movl	%ecx, %eax
	mull	%edx
	movl	%edx, 4(%esp)                   # 4-byte Spill
	movl	%eax, 16(%esp)                  # 4-byte Spill
	movl	%ecx, %eax
	mull	%ebx
	movl	%ebx, 64(%esp)                  # 4-byte Spill
	movl	%edx, 28(%esp)                  # 4-byte Spill
	movl	%eax, 12(%esp)                  # 4-byte Spill
	movl	%ecx, %eax
	mull	%ebp
	movl	%edx, 24(%esp)                  # 4-byte Spill
	movl	%eax, 8(%esp)                   # 4-byte Spill
	movl	%ecx, %eax
	mull	%edi
	movl	%edi, 76(%esp)                  # 4-byte Spill
	movl	%edi, %ebp
	movl	%edx, 52(%esp)                  # 4-byte Spill
	movl	%eax, 40(%esp)                  # 4-byte Spill
	movl	%ecx, %eax
	movl	56(%esp), %edi                  # 4-byte Reload
	mull	%edi
	movl	%edx, (%esp)                    # 4-byte Spill
	movl	%eax, 36(%esp)                  # 4-byte Spill
	movl	%ecx, %eax
	mull	72(%esp)                        # 4-byte Folded Reload
	movl	%edx, 44(%esp)                  # 4-byte Spill
	movl	%eax, 68(%esp)                  # 4-byte Spill
	movl	32(%esp), %eax                  # 4-byte Reload
	mull	%esi
	movl	%edx, %ecx
	movl	%eax, 92(%esp)                  # 4-byte Spill
	movl	%ebx, %eax
	mull	%esi
	movl	%edx, 20(%esp)                  # 4-byte Spill
	movl	%eax, 88(%esp)                  # 4-byte Spill
	movl	60(%esp), %eax                  # 4-byte Reload
	mull	%esi
	movl	%edx, %ebx
	movl	%eax, 84(%esp)                  # 4-byte Spill
	movl	%ebp, %eax
	mull	%esi
	movl	%edx, %ebp
	movl	%eax, 80(%esp)                  # 4-byte Spill
	movl	%edi, %eax
	mull	%esi
	movl	%edx, %edi
	addl	48(%esp), %eax                  # 4-byte Folded Reload
	adcl	80(%esp), %edi                  # 4-byte Folded Reload
	adcl	84(%esp), %ebp                  # 4-byte Folded Reload
	adcl	88(%esp), %ebx                  # 4-byte Folded Reload
	movl	20(%esp), %esi                  # 4-byte Reload
	adcl	92(%esp), %esi                  # 4-byte Folded Reload
	adcl	$0, %ecx
	addl	68(%esp), %eax                  # 4-byte Folded Reload
	movl	116(%esp), %edx
	movl	%eax, 4(%edx)
	adcl	36(%esp), %edi                  # 4-byte Folded Reload
	adcl	40(%esp), %ebp                  # 4-byte Folded Reload
	adcl	8(%esp), %ebx                   # 4-byte Folded Reload
	adcl	12(%esp), %esi                  # 4-byte Folded Reload
	movl	%ecx, %edx
	adcl	16(%esp), %edx                  # 4-byte Folded Reload
	setb	%al
	addl	44(%esp), %edi                  # 4-byte Folded Reload
	movl	%edi, %ecx
	adcl	(%esp), %ebp                    # 4-byte Folded Reload
	movl	%ebp, 16(%esp)                  # 4-byte Spill
	adcl	52(%esp), %ebx                  # 4-byte Folded Reload
	movl	%ebx, 48(%esp)                  # 4-byte Spill
	adcl	24(%esp), %esi                  # 4-byte Folded Reload
	movl	%esi, 20(%esp)                  # 4-byte Spill
	adcl	28(%esp), %edx                  # 4-byte Folded Reload
	movl	%edx, 52(%esp)                  # 4-byte Spill
	movzbl	%al, %eax
	adcl	4(%esp), %eax                   # 4-byte Folded Reload
	movl	%eax, 8(%esp)                   # 4-byte Spill
	movl	124(%esp), %eax
	movl	8(%eax), %esi
	movl	%esi, %eax
	mull	32(%esp)                        # 4-byte Folded Reload
	movl	%edx, 4(%esp)                   # 4-byte Spill
	movl	%eax, 44(%esp)                  # 4-byte Spill
	movl	%esi, %eax
	mull	64(%esp)                        # 4-byte Folded Reload
	movl	%edx, 28(%esp)                  # 4-byte Spill
	movl	%eax, 40(%esp)                  # 4-byte Spill
	movl	%esi, %eax
	mull	60(%esp)                        # 4-byte Folded Reload
	movl	%edx, 24(%esp)                  # 4-byte Spill
	movl	%eax, 36(%esp)                  # 4-byte Spill
	movl	%esi, %eax
	mull	76(%esp)                        # 4-byte Folded Reload
	movl	%edx, 12(%esp)                  # 4-byte Spill
	movl	%eax, %ebx
	movl	%esi, %eax
	mull	56(%esp)                        # 4-byte Folded Reload
	movl	%edx, %edi
	movl	%eax, (%esp)                    # 4-byte Spill
	movl	%esi, %eax
	mull	72(%esp)                        # 4-byte Folded Reload
	movl	%eax, %ebp
	addl	(%esp), %edx                    # 4-byte Folded Reload
	movl	%edx, (%esp)                    # 4-byte Spill
	adcl	%ebx, %edi
	movl	%edi, %ebx
	movl	12(%esp), %edi                  # 4-byte Reload
	adcl	36(%esp), %edi                  # 4-byte Folded Reload
	movl	24(%esp), %esi                  # 4-byte Reload
	adcl	40(%esp), %esi                  # 4-byte Folded Reload
	movl	28(%esp), %eax                  # 4-byte Reload
	adcl	44(%esp), %eax                  # 4-byte Folded Reload
	movl	4(%esp), %edx                   # 4-byte Reload
	adcl	$0, %edx
	addl	%ecx, %ebp
	movl	116(%esp), %ecx
	movl	%ebp, 8(%ecx)
	movl	16(%esp), %ecx                  # 4-byte Reload
	adcl	%ecx, (%esp)                    # 4-byte Folded Spill
	adcl	48(%esp), %ebx                  # 4-byte Folded Reload
	movl	%ebx, 48(%esp)                  # 4-byte Spill
	adcl	20(%esp), %edi                  # 4-byte Folded Reload
	movl	%edi, 12(%esp)                  # 4-byte Spill
	adcl	52(%esp), %esi                  # 4-byte Folded Reload
	movl	%esi, 24(%esp)                  # 4-byte Spill
	adcl	8(%esp), %eax                   # 4-byte Folded Reload
	movl	%eax, 28(%esp)                  # 4-byte Spill
	adcl	$0, %edx
	movl	%edx, 4(%esp)                   # 4-byte Spill
	movl	124(%esp), %eax
	movl	12(%eax), %ecx
	movl	%ecx, %eax
	mull	32(%esp)                        # 4-byte Folded Reload
	movl	%edx, 20(%esp)                  # 4-byte Spill
	movl	%eax, 44(%esp)                  # 4-byte Spill
	movl	%ecx, %eax
	mull	64(%esp)                        # 4-byte Folded Reload
	movl	%edx, 16(%esp)                  # 4-byte Spill
	movl	%eax, 40(%esp)                  # 4-byte Spill
	movl	%ecx, %eax
	mull	60(%esp)                        # 4-byte Folded Reload
	movl	%edx, 52(%esp)                  # 4-byte Spill
	movl	%eax, 36(%esp)                  # 4-byte Spill
	movl	%ecx, %eax
	mull	76(%esp)                        # 4-byte Folded Reload
	movl	%edx, %esi
	movl	%eax, 8(%esp)                   # 4-byte Spill
	movl	%ecx, %eax
	mull	56(%esp)                        # 4-byte Folded Reload
	movl	%edx, %ebx
	movl	%eax, 68(%esp)                  # 4-byte Spill
	movl	%ecx, %eax
	movl	72(%esp), %ecx                  # 4-byte Reload
	mull	%ecx
	movl	%ecx, %ebp
	movl	%eax, %edi
	addl	68(%esp), %edx                  # 4-byte Folded Reload
	movl	%edx, %ecx
	adcl	8(%esp), %ebx                   # 4-byte Folded Reload
	movl	%ebx, 8(%esp)                   # 4-byte Spill
	adcl	36(%esp), %esi                  # 4-byte Folded Reload
	movl	%esi, %eax
	movl	52(%esp), %ebx                  # 4-byte Reload
	adcl	40(%esp), %ebx                  # 4-byte Folded Reload
	movl	16(%esp), %esi                  # 4-byte Reload
	adcl	44(%esp), %esi                  # 4-byte Folded Reload
	movl	20(%esp), %edx                  # 4-byte Reload
	adcl	$0, %edx
	addl	(%esp), %edi                    # 4-byte Folded Reload
	adcl	48(%esp), %ecx                  # 4-byte Folded Reload
	movl	%ecx, 44(%esp)                  # 4-byte Spill
	movl	116(%esp), %ecx
	movl	%edi, 12(%ecx)
	movl	12(%esp), %ecx                  # 4-byte Reload
	adcl	%ecx, 8(%esp)                   # 4-byte Folded Spill
	adcl	24(%esp), %eax                  # 4-byte Folded Reload
	movl	%eax, 48(%esp)                  # 4-byte Spill
	adcl	28(%esp), %ebx                  # 4-byte Folded Reload
	movl	%ebx, 52(%esp)                  # 4-byte Spill
	adcl	4(%esp), %esi                   # 4-byte Folded Reload
	movl	%esi, 16(%esp)                  # 4-byte Spill
	adcl	$0, %edx
	movl	%edx, 20(%esp)                  # 4-byte Spill
	movl	124(%esp), %eax
	movl	16(%eax), %esi
	movl	%esi, %eax
	mull	32(%esp)                        # 4-byte Folded Reload
	movl	%edx, 4(%esp)                   # 4-byte Spill
	movl	%eax, 40(%esp)                  # 4-byte Spill
	movl	%esi, %eax
	mull	64(%esp)                        # 4-byte Folded Reload
	movl	%edx, 28(%esp)                  # 4-byte Spill
	movl	%eax, 36(%esp)                  # 4-byte Spill
	movl	%esi, %eax
	mull	60(%esp)                        # 4-byte Folded Reload
	movl	%edx, 24(%esp)                  # 4-byte Spill
	movl	%eax, 68(%esp)                  # 4-byte Spill
	movl	%esi, %eax
	mull	76(%esp)                        # 4-byte Folded Reload
	movl	%edx, 12(%esp)                  # 4-byte Spill
	movl	%eax, (%esp)                    # 4-byte Spill
	movl	%esi, %eax
	mull	56(%esp)                        # 4-byte Folded Reload
	movl	%edx, %ecx
	movl	%eax, %ebx
	movl	%esi, %eax
	mull	%ebp
	movl	%eax, %ebp
	movl	%edx, %edi
	addl	%ebx, %edi
	adcl	(%esp), %ecx                    # 4-byte Folded Reload
	movl	%ecx, (%esp)                    # 4-byte Spill
	movl	12(%esp), %eax                  # 4-byte Reload
	adcl	68(%esp), %eax                  # 4-byte Folded Reload
	movl	24(%esp), %ecx                  # 4-byte Reload
	adcl	36(%esp), %ecx                  # 4-byte Folded Reload
	movl	28(%esp), %ebx                  # 4-byte Reload
	adcl	40(%esp), %ebx                  # 4-byte Folded Reload
	movl	4(%esp), %edx                   # 4-byte Reload
	adcl	$0, %edx
	addl	44(%esp), %ebp                  # 4-byte Folded Reload
	movl	%ebp, 4(%esp)                   # 4-byte Spill
	movl	116(%esp), %ebp
	movl	4(%esp), %esi                   # 4-byte Reload
	movl	%esi, 16(%ebp)
	adcl	8(%esp), %edi                   # 4-byte Folded Reload
	movl	%edi, 8(%esp)                   # 4-byte Spill
	movl	(%esp), %esi                    # 4-byte Reload
	adcl	48(%esp), %esi                  # 4-byte Folded Reload
	movl	%esi, (%esp)                    # 4-byte Spill
	adcl	52(%esp), %eax                  # 4-byte Folded Reload
	movl	%eax, 12(%esp)                  # 4-byte Spill
	adcl	16(%esp), %ecx                  # 4-byte Folded Reload
	movl	%ecx, 24(%esp)                  # 4-byte Spill
	adcl	20(%esp), %ebx                  # 4-byte Folded Reload
	movl	%ebx, 28(%esp)                  # 4-byte Spill
	adcl	$0, %edx
	movl	%edx, 4(%esp)                   # 4-byte Spill
	movl	124(%esp), %eax
	movl	20(%eax), %ebx
	movl	%ebx, %eax
	mull	32(%esp)                        # 4-byte Folded Reload
	movl	%edx, 32(%esp)                  # 4-byte Spill
	movl	%eax, 20(%esp)                  # 4-byte Spill
	movl	%ebx, %eax
	mull	64(%esp)                        # 4-byte Folded Reload
	movl	%edx, %ebp
	movl	%eax, 64(%esp)                  # 4-byte Spill
	movl	%ebx, %eax
	mull	56(%esp)                        # 4-byte Folded Reload
	movl	%edx, 56(%esp)                  # 4-byte Spill
	movl	%eax, 16(%esp)                  # 4-byte Spill
	movl	%ebx, %eax
	mull	72(%esp)                        # 4-byte Folded Reload
	movl	%edx, %ecx
	movl	%eax, %esi
	movl	%ebx, %eax
	mull	60(%esp)                        # 4-byte Folded Reload
	movl	%edx, %edi
	movl	%eax, 60(%esp)                  # 4-byte Spill
	movl	%ebx, %eax
	mull	76(%esp)                        # 4-byte Folded Reload
	addl	16(%esp), %ecx                  # 4-byte Folded Reload
	adcl	56(%esp), %eax                  # 4-byte Folded Reload
	adcl	60(%esp), %edx                  # 4-byte Folded Reload
	adcl	64(%esp), %edi                  # 4-byte Folded Reload
	adcl	20(%esp), %ebp                  # 4-byte Folded Reload
	adcl	$0, 32(%esp)                    # 4-byte Folded Spill
	addl	8(%esp), %esi                   # 4-byte Folded Reload
	adcl	(%esp), %ecx                    # 4-byte Folded Reload
	movl	116(%esp), %ebx
	movl	%esi, 20(%ebx)
	adcl	12(%esp), %eax                  # 4-byte Folded Reload
	movl	%ecx, 24(%ebx)
	adcl	24(%esp), %edx                  # 4-byte Folded Reload
	movl	%eax, 28(%ebx)
	adcl	28(%esp), %edi                  # 4-byte Folded Reload
	movl	%edx, 32(%ebx)
	movl	%edi, 36(%ebx)
	adcl	4(%esp), %ebp                   # 4-byte Folded Reload
	movl	%ebp, 40(%ebx)
	movl	32(%esp), %eax                  # 4-byte Reload
	adcl	$0, %eax
	movl	%eax, 44(%ebx)
	addl	$96, %esp
	popl	%esi
	popl	%edi
	popl	%ebx
	popl	%ebp
	retl
.Lfunc_end7:
	.size	mcl_fpDbl_mulPre6L, .Lfunc_end7-mcl_fpDbl_mulPre6L
                                        # -- End function
	.globl	mcl_fpDbl_sqrPre6L              # -- Begin function mcl_fpDbl_sqrPre6L
	.p2align	4, 0x90
	.type	mcl_fpDbl_sqrPre6L,@function
mcl_fpDbl_sqrPre6L:                     # @mcl_fpDbl_sqrPre6L
# %bb.0:
	pushl	%ebp
	pushl	%ebx
	pushl	%edi
	pushl	%esi
	subl	$172, %esp
	movl	196(%esp), %edi
	movl	20(%edi), %eax
	movl	%eax, 8(%esp)                   # 4-byte Spill
	movl	(%edi), %esi
	mull	%esi
	movl	%eax, 148(%esp)                 # 4-byte Spill
	movl	%edx, 144(%esp)                 # 4-byte Spill
	movl	16(%edi), %ebx
	movl	%ebx, %eax
	movl	%ebx, (%esp)                    # 4-byte Spill
	mull	%esi
	movl	%eax, 132(%esp)                 # 4-byte Spill
	movl	%edx, 44(%esp)                  # 4-byte Spill
	movl	12(%edi), %ecx
	movl	%ecx, %eax
	movl	%ecx, 28(%esp)                  # 4-byte Spill
	mull	%esi
	movl	%eax, 120(%esp)                 # 4-byte Spill
	movl	%edx, 16(%esp)                  # 4-byte Spill
	movl	4(%edi), %ebp
	movl	8(%edi), %edi
	movl	%edi, %eax
	mull	%esi
	movl	%edx, 12(%esp)                  # 4-byte Spill
	movl	%eax, 116(%esp)                 # 4-byte Spill
	movl	8(%esp), %eax                   # 4-byte Reload
	mull	%ebp
	movl	%edx, 140(%esp)                 # 4-byte Spill
	movl	%eax, 136(%esp)                 # 4-byte Spill
	movl	%ebx, %eax
	mull	%ebp
	movl	%edx, 128(%esp)                 # 4-byte Spill
	movl	%eax, 124(%esp)                 # 4-byte Spill
	movl	%ecx, %eax
	mull	%ebp
	movl	%edx, 40(%esp)                  # 4-byte Spill
	movl	%eax, 112(%esp)                 # 4-byte Spill
	movl	%edi, %eax
	mull	%ebp
	movl	%edx, 24(%esp)                  # 4-byte Spill
	movl	%eax, 20(%esp)                  # 4-byte Spill
	movl	%ebp, %eax
	mull	%ebp
	movl	%edx, 4(%esp)                   # 4-byte Spill
	movl	%eax, 36(%esp)                  # 4-byte Spill
	movl	%ebp, %eax
	mull	%esi
	movl	%edx, %ebp
	movl	%eax, 88(%esp)                  # 4-byte Spill
	movl	%esi, %eax
	mull	%esi
	movl	%edx, 32(%esp)                  # 4-byte Spill
	movl	192(%esp), %edx
	movl	%eax, (%edx)
	movl	8(%esp), %ebx                   # 4-byte Reload
	movl	%ebx, %eax
	movl	(%esp), %ecx                    # 4-byte Reload
	mull	%ecx
	movl	%edx, 92(%esp)                  # 4-byte Spill
	movl	%eax, 108(%esp)                 # 4-byte Spill
	movl	%ebx, %eax
	movl	28(%esp), %esi                  # 4-byte Reload
	mull	%esi
	movl	%edx, 104(%esp)                 # 4-byte Spill
	movl	%eax, 100(%esp)                 # 4-byte Spill
	movl	%ebx, %eax
	mull	%edi
	movl	%edx, 64(%esp)                  # 4-byte Spill
	movl	%eax, 96(%esp)                  # 4-byte Spill
	movl	%ebx, %eax
	mull	%ebx
	movl	%edx, 168(%esp)                 # 4-byte Spill
	movl	%eax, 164(%esp)                 # 4-byte Spill
	movl	%ecx, %eax
	mull	%esi
	movl	%edx, 80(%esp)                  # 4-byte Spill
	movl	%eax, 76(%esp)                  # 4-byte Spill
	movl	%ecx, %eax
	mull	%edi
	movl	%edx, 8(%esp)                   # 4-byte Spill
	movl	%eax, 84(%esp)                  # 4-byte Spill
	movl	%ecx, %eax
	mull	%ecx
	movl	%edx, 60(%esp)                  # 4-byte Spill
	movl	%eax, 56(%esp)                  # 4-byte Spill
	movl	%esi, %eax
	mull	%edi
	movl	%edx, 72(%esp)                  # 4-byte Spill
	movl	%eax, 68(%esp)                  # 4-byte Spill
	movl	%esi, %eax
	mull	%esi
	movl	%edx, 28(%esp)                  # 4-byte Spill
	movl	%eax, 52(%esp)                  # 4-byte Spill
	movl	%edi, %eax
	mull	%edi
	movl	%eax, (%esp)                    # 4-byte Spill
	movl	%edx, 48(%esp)                  # 4-byte Spill
	movl	%ebp, %edi
	addl	%ebp, 36(%esp)                  # 4-byte Folded Spill
	movl	4(%esp), %eax                   # 4-byte Reload
	adcl	20(%esp), %eax                  # 4-byte Folded Reload
	movl	%eax, 4(%esp)                   # 4-byte Spill
	movl	24(%esp), %eax                  # 4-byte Reload
	adcl	112(%esp), %eax                 # 4-byte Folded Reload
	movl	%eax, 160(%esp)                 # 4-byte Spill
	movl	40(%esp), %eax                  # 4-byte Reload
	adcl	124(%esp), %eax                 # 4-byte Folded Reload
	movl	%eax, 156(%esp)                 # 4-byte Spill
	movl	128(%esp), %eax                 # 4-byte Reload
	adcl	136(%esp), %eax                 # 4-byte Folded Reload
	movl	%eax, 152(%esp)                 # 4-byte Spill
	movl	140(%esp), %eax                 # 4-byte Reload
	adcl	$0, %eax
	movl	%eax, %ebx
	movl	32(%esp), %eax                  # 4-byte Reload
	addl	88(%esp), %eax                  # 4-byte Folded Reload
	adcl	116(%esp), %edi                 # 4-byte Folded Reload
	movl	12(%esp), %ecx                  # 4-byte Reload
	adcl	120(%esp), %ecx                 # 4-byte Folded Reload
	movl	16(%esp), %esi                  # 4-byte Reload
	adcl	132(%esp), %esi                 # 4-byte Folded Reload
	movl	44(%esp), %edx                  # 4-byte Reload
	adcl	148(%esp), %edx                 # 4-byte Folded Reload
	movl	144(%esp), %ebp                 # 4-byte Reload
	adcl	$0, %ebp
	movl	%ebp, 32(%esp)                  # 4-byte Spill
	addl	88(%esp), %eax                  # 4-byte Folded Reload
	adcl	36(%esp), %edi                  # 4-byte Folded Reload
	movl	192(%esp), %ebp
	movl	%eax, 4(%ebp)
	adcl	4(%esp), %ecx                   # 4-byte Folded Reload
	adcl	160(%esp), %esi                 # 4-byte Folded Reload
	adcl	156(%esp), %edx                 # 4-byte Folded Reload
	movl	152(%esp), %eax                 # 4-byte Reload
	adcl	%eax, 32(%esp)                  # 4-byte Folded Spill
	adcl	$0, %ebx
	movl	%ebx, 36(%esp)                  # 4-byte Spill
	movl	12(%esp), %eax                  # 4-byte Reload
	addl	20(%esp), %eax                  # 4-byte Folded Reload
	movl	(%esp), %ebp                    # 4-byte Reload
	adcl	24(%esp), %ebp                  # 4-byte Folded Reload
	movl	%ebp, (%esp)                    # 4-byte Spill
	movl	48(%esp), %ebp                  # 4-byte Reload
	adcl	68(%esp), %ebp                  # 4-byte Folded Reload
	movl	72(%esp), %ebx                  # 4-byte Reload
	adcl	84(%esp), %ebx                  # 4-byte Folded Reload
	movl	%ebx, 20(%esp)                  # 4-byte Spill
	movl	8(%esp), %ebx                   # 4-byte Reload
	adcl	96(%esp), %ebx                  # 4-byte Folded Reload
	movl	%ebx, 24(%esp)                  # 4-byte Spill
	movl	64(%esp), %ebx                  # 4-byte Reload
	movl	%ebx, 4(%esp)                   # 4-byte Spill
	adcl	$0, 4(%esp)                     # 4-byte Folded Spill
	addl	116(%esp), %edi                 # 4-byte Folded Reload
	adcl	%ecx, %eax
	movl	%eax, 12(%esp)                  # 4-byte Spill
	movl	192(%esp), %eax
	movl	%edi, 8(%eax)
	adcl	%esi, (%esp)                    # 4-byte Folded Spill
	adcl	%edx, %ebp
	movl	%ebp, 48(%esp)                  # 4-byte Spill
	movl	32(%esp), %eax                  # 4-byte Reload
	adcl	%eax, 20(%esp)                  # 4-byte Folded Spill
	movl	36(%esp), %eax                  # 4-byte Reload
	adcl	%eax, 24(%esp)                  # 4-byte Folded Spill
	adcl	$0, 4(%esp)                     # 4-byte Folded Spill
	movl	16(%esp), %eax                  # 4-byte Reload
	addl	112(%esp), %eax                 # 4-byte Folded Reload
	movl	40(%esp), %esi                  # 4-byte Reload
	adcl	68(%esp), %esi                  # 4-byte Folded Reload
	movl	52(%esp), %edx                  # 4-byte Reload
	adcl	72(%esp), %edx                  # 4-byte Folded Reload
	movl	28(%esp), %ecx                  # 4-byte Reload
	adcl	76(%esp), %ecx                  # 4-byte Folded Reload
	movl	80(%esp), %ebx                  # 4-byte Reload
	adcl	100(%esp), %ebx                 # 4-byte Folded Reload
	movl	104(%esp), %ebp                 # 4-byte Reload
	adcl	$0, %ebp
	movl	12(%esp), %edi                  # 4-byte Reload
	addl	120(%esp), %edi                 # 4-byte Folded Reload
	adcl	(%esp), %eax                    # 4-byte Folded Reload
	movl	%eax, 16(%esp)                  # 4-byte Spill
	movl	192(%esp), %eax
	movl	%edi, 12(%eax)
	adcl	48(%esp), %esi                  # 4-byte Folded Reload
	movl	%esi, 40(%esp)                  # 4-byte Spill
	adcl	20(%esp), %edx                  # 4-byte Folded Reload
	movl	%edx, 52(%esp)                  # 4-byte Spill
	adcl	24(%esp), %ecx                  # 4-byte Folded Reload
	movl	%ecx, 28(%esp)                  # 4-byte Spill
	adcl	4(%esp), %ebx                   # 4-byte Folded Reload
	movl	%ebx, 12(%esp)                  # 4-byte Spill
	adcl	$0, %ebp
	movl	%ebp, (%esp)                    # 4-byte Spill
	movl	44(%esp), %esi                  # 4-byte Reload
	addl	124(%esp), %esi                 # 4-byte Folded Reload
	movl	128(%esp), %edi                 # 4-byte Reload
	adcl	84(%esp), %edi                  # 4-byte Folded Reload
	movl	8(%esp), %ecx                   # 4-byte Reload
	adcl	76(%esp), %ecx                  # 4-byte Folded Reload
	movl	56(%esp), %edx                  # 4-byte Reload
	adcl	80(%esp), %edx                  # 4-byte Folded Reload
	movl	60(%esp), %eax                  # 4-byte Reload
	adcl	108(%esp), %eax                 # 4-byte Folded Reload
	movl	92(%esp), %ebp                  # 4-byte Reload
	adcl	$0, %ebp
	movl	16(%esp), %ebx                  # 4-byte Reload
	addl	132(%esp), %ebx                 # 4-byte Folded Reload
	adcl	40(%esp), %esi                  # 4-byte Folded Reload
	movl	%esi, 44(%esp)                  # 4-byte Spill
	movl	192(%esp), %esi
	movl	%ebx, 16(%esi)
	adcl	52(%esp), %edi                  # 4-byte Folded Reload
	movl	%edi, %esi
	adcl	28(%esp), %ecx                  # 4-byte Folded Reload
	movl	%ecx, 8(%esp)                   # 4-byte Spill
	adcl	12(%esp), %edx                  # 4-byte Folded Reload
	movl	%edx, 56(%esp)                  # 4-byte Spill
	adcl	(%esp), %eax                    # 4-byte Folded Reload
	movl	%eax, 60(%esp)                  # 4-byte Spill
	adcl	$0, %ebp
	movl	%ebp, 16(%esp)                  # 4-byte Spill
	movl	144(%esp), %ecx                 # 4-byte Reload
	addl	136(%esp), %ecx                 # 4-byte Folded Reload
	movl	140(%esp), %edi                 # 4-byte Reload
	adcl	96(%esp), %edi                  # 4-byte Folded Reload
	movl	100(%esp), %eax                 # 4-byte Reload
	adcl	%eax, 64(%esp)                  # 4-byte Folded Spill
	movl	104(%esp), %eax                 # 4-byte Reload
	adcl	108(%esp), %eax                 # 4-byte Folded Reload
	movl	164(%esp), %ebx                 # 4-byte Reload
	adcl	92(%esp), %ebx                  # 4-byte Folded Reload
	movl	168(%esp), %ebp                 # 4-byte Reload
	adcl	$0, %ebp
	movl	44(%esp), %edx                  # 4-byte Reload
	addl	148(%esp), %edx                 # 4-byte Folded Reload
	adcl	%esi, %ecx
	movl	192(%esp), %esi
	movl	%edx, 20(%esi)
	movl	%edi, %edx
	adcl	8(%esp), %edx                   # 4-byte Folded Reload
	movl	%ecx, 24(%esi)
	movl	64(%esp), %edi                  # 4-byte Reload
	adcl	56(%esp), %edi                  # 4-byte Folded Reload
	movl	%edx, 28(%esi)
	adcl	60(%esp), %eax                  # 4-byte Folded Reload
	movl	%edi, 32(%esi)
	movl	%eax, 36(%esi)
	adcl	16(%esp), %ebx                  # 4-byte Folded Reload
	movl	%ebx, 40(%esi)
	movl	%ebp, %eax
	adcl	$0, %eax
	movl	%eax, 44(%esi)
	addl	$172, %esp
	popl	%esi
	popl	%edi
	popl	%ebx
	popl	%ebp
	retl
.Lfunc_end8:
	.size	mcl_fpDbl_sqrPre6L, .Lfunc_end8-mcl_fpDbl_sqrPre6L
                                        # -- End function
	.globl	mcl_fp_mont6L                   # -- Begin function mcl_fp_mont6L
	.p2align	4, 0x90
	.type	mcl_fp_mont6L,@function
mcl_fp_mont6L:                          # @mcl_fp_mont6L
# %bb.0:
	pushl	%ebp
	pushl	%ebx
	pushl	%edi
	pushl	%esi
	subl	$128, %esp
	movl	152(%esp), %ebx
	movl	(%ebx), %eax
	movl	%eax, 76(%esp)                  # 4-byte Spill
	movl	156(%esp), %ecx
	movl	(%ecx), %esi
	mull	%esi
	movl	%eax, 12(%esp)                  # 4-byte Spill
	movl	%edx, 32(%esp)                  # 4-byte Spill
	movl	160(%esp), %edi
	movl	-4(%edi), %ecx
	movl	%ecx, 80(%esp)                  # 4-byte Spill
	imull	%eax, %ecx
	movl	20(%edi), %edx
	movl	%edx, 112(%esp)                 # 4-byte Spill
	movl	%ecx, %eax
	mull	%edx
	movl	%eax, 8(%esp)                   # 4-byte Spill
	movl	%edx, 24(%esp)                  # 4-byte Spill
	movl	16(%edi), %edx
	movl	%edx, 108(%esp)                 # 4-byte Spill
	movl	%ecx, %eax
	mull	%edx
	movl	%eax, 52(%esp)                  # 4-byte Spill
	movl	%edx, 20(%esp)                  # 4-byte Spill
	movl	12(%edi), %edx
	movl	%edx, 92(%esp)                  # 4-byte Spill
	movl	%ecx, %eax
	mull	%edx
	movl	%eax, 28(%esp)                  # 4-byte Spill
	movl	%edx, 16(%esp)                  # 4-byte Spill
	movl	8(%edi), %edx
	movl	%edx, 88(%esp)                  # 4-byte Spill
	movl	%ecx, %eax
	mull	%edx
	movl	%eax, 44(%esp)                  # 4-byte Spill
	movl	%edx, 4(%esp)                   # 4-byte Spill
	movl	(%edi), %ebp
	movl	%ebp, 84(%esp)                  # 4-byte Spill
	movl	4(%edi), %edx
	movl	%edx, 96(%esp)                  # 4-byte Spill
	movl	%ecx, %eax
	mull	%edx
	movl	%edx, 40(%esp)                  # 4-byte Spill
	movl	%eax, 56(%esp)                  # 4-byte Spill
	movl	%ecx, %eax
	mull	%ebp
	movl	%eax, 48(%esp)                  # 4-byte Spill
	movl	%edx, 36(%esp)                  # 4-byte Spill
	movl	%ebx, %ecx
	movl	20(%ebx), %eax
	movl	%eax, 72(%esp)                  # 4-byte Spill
	mull	%esi
	movl	%eax, 100(%esp)                 # 4-byte Spill
	movl	%edx, (%esp)                    # 4-byte Spill
	movl	16(%ebx), %eax
	movl	%eax, 68(%esp)                  # 4-byte Spill
	mull	%esi
	movl	%eax, 124(%esp)                 # 4-byte Spill
	movl	%edx, %ebp
	movl	12(%ebx), %eax
	movl	%eax, 64(%esp)                  # 4-byte Spill
	mull	%esi
	movl	%eax, 120(%esp)                 # 4-byte Spill
	movl	%edx, %edi
	movl	4(%ebx), %ecx
	movl	%ecx, 104(%esp)                 # 4-byte Spill
	movl	8(%ebx), %eax
	movl	%eax, 60(%esp)                  # 4-byte Spill
	mull	%esi
	movl	%edx, %ebx
	movl	%eax, 116(%esp)                 # 4-byte Spill
	movl	%ecx, %eax
	mull	%esi
	addl	32(%esp), %eax                  # 4-byte Folded Reload
	movl	%eax, 32(%esp)                  # 4-byte Spill
	adcl	116(%esp), %edx                 # 4-byte Folded Reload
	movl	%edx, 116(%esp)                 # 4-byte Spill
	adcl	120(%esp), %ebx                 # 4-byte Folded Reload
	movl	%ebx, 120(%esp)                 # 4-byte Spill
	adcl	124(%esp), %edi                 # 4-byte Folded Reload
	movl	%edi, 124(%esp)                 # 4-byte Spill
	adcl	100(%esp), %ebp                 # 4-byte Folded Reload
	movl	%ebp, 100(%esp)                 # 4-byte Spill
	adcl	$0, (%esp)                      # 4-byte Folded Spill
	movl	36(%esp), %edi                  # 4-byte Reload
	addl	56(%esp), %edi                  # 4-byte Folded Reload
	movl	40(%esp), %edx                  # 4-byte Reload
	adcl	44(%esp), %edx                  # 4-byte Folded Reload
	movl	4(%esp), %eax                   # 4-byte Reload
	adcl	28(%esp), %eax                  # 4-byte Folded Reload
	movl	16(%esp), %ebx                  # 4-byte Reload
	adcl	52(%esp), %ebx                  # 4-byte Folded Reload
	movl	20(%esp), %esi                  # 4-byte Reload
	adcl	8(%esp), %esi                   # 4-byte Folded Reload
	movl	24(%esp), %ecx                  # 4-byte Reload
	adcl	$0, %ecx
	movl	48(%esp), %ebp                  # 4-byte Reload
	addl	12(%esp), %ebp                  # 4-byte Folded Reload
	adcl	32(%esp), %edi                  # 4-byte Folded Reload
	movl	%edi, 36(%esp)                  # 4-byte Spill
	adcl	116(%esp), %edx                 # 4-byte Folded Reload
	movl	%edx, 40(%esp)                  # 4-byte Spill
	adcl	120(%esp), %eax                 # 4-byte Folded Reload
	movl	%eax, 4(%esp)                   # 4-byte Spill
	adcl	124(%esp), %ebx                 # 4-byte Folded Reload
	movl	%ebx, 16(%esp)                  # 4-byte Spill
	adcl	100(%esp), %esi                 # 4-byte Folded Reload
	movl	%esi, 20(%esp)                  # 4-byte Spill
	adcl	(%esp), %ecx                    # 4-byte Folded Reload
	movl	%ecx, 24(%esp)                  # 4-byte Spill
	setb	52(%esp)                        # 1-byte Folded Spill
	movl	156(%esp), %eax
	movl	4(%eax), %ecx
	movl	%ecx, %eax
	mull	72(%esp)                        # 4-byte Folded Reload
	movl	%edx, 12(%esp)                  # 4-byte Spill
	movl	%eax, 28(%esp)                  # 4-byte Spill
	movl	%ecx, %eax
	mull	68(%esp)                        # 4-byte Folded Reload
	movl	%edx, (%esp)                    # 4-byte Spill
	movl	%eax, 48(%esp)                  # 4-byte Spill
	movl	%ecx, %eax
	mull	64(%esp)                        # 4-byte Folded Reload
	movl	%edx, 8(%esp)                   # 4-byte Spill
	movl	%eax, 44(%esp)                  # 4-byte Spill
	movl	%ecx, %eax
	mull	60(%esp)                        # 4-byte Folded Reload
	movl	%edx, %ebp
	movl	%eax, 56(%esp)                  # 4-byte Spill
	movl	%ecx, %eax
	mull	104(%esp)                       # 4-byte Folded Reload
	movl	%edx, %esi
	movl	%eax, %ebx
	movl	%ecx, %eax
	mull	76(%esp)                        # 4-byte Folded Reload
	movl	%eax, 32(%esp)                  # 4-byte Spill
	addl	%ebx, %edx
	movl	%edx, %edi
	adcl	56(%esp), %esi                  # 4-byte Folded Reload
	movl	%esi, %ebx
	movl	%ebp, %edx
	adcl	44(%esp), %edx                  # 4-byte Folded Reload
	movl	8(%esp), %ecx                   # 4-byte Reload
	adcl	48(%esp), %ecx                  # 4-byte Folded Reload
	movl	(%esp), %eax                    # 4-byte Reload
	adcl	28(%esp), %eax                  # 4-byte Folded Reload
	movl	12(%esp), %ebp                  # 4-byte Reload
	adcl	$0, %ebp
	movl	32(%esp), %esi                  # 4-byte Reload
	addl	36(%esp), %esi                  # 4-byte Folded Reload
	movl	%esi, 32(%esp)                  # 4-byte Spill
	adcl	40(%esp), %edi                  # 4-byte Folded Reload
	movl	%edi, 48(%esp)                  # 4-byte Spill
	adcl	4(%esp), %ebx                   # 4-byte Folded Reload
	movl	%ebx, 28(%esp)                  # 4-byte Spill
	adcl	16(%esp), %edx                  # 4-byte Folded Reload
	movl	%edx, 16(%esp)                  # 4-byte Spill
	adcl	20(%esp), %ecx                  # 4-byte Folded Reload
	movl	%ecx, 8(%esp)                   # 4-byte Spill
	adcl	24(%esp), %eax                  # 4-byte Folded Reload
	movl	%eax, (%esp)                    # 4-byte Spill
	movzbl	52(%esp), %eax                  # 1-byte Folded Reload
	adcl	%eax, %ebp
	movl	%ebp, 12(%esp)                  # 4-byte Spill
	setb	24(%esp)                        # 1-byte Folded Spill
	movl	80(%esp), %ebx                  # 4-byte Reload
	imull	%esi, %ebx
	movl	%ebx, %eax
	mull	112(%esp)                       # 4-byte Folded Reload
	movl	%edx, 4(%esp)                   # 4-byte Spill
	movl	%eax, 20(%esp)                  # 4-byte Spill
	movl	%ebx, %eax
	mull	108(%esp)                       # 4-byte Folded Reload
	movl	%edx, 40(%esp)                  # 4-byte Spill
	movl	%eax, 44(%esp)                  # 4-byte Spill
	movl	%ebx, %eax
	mull	92(%esp)                        # 4-byte Folded Reload
	movl	%edx, 36(%esp)                  # 4-byte Spill
	movl	%eax, 56(%esp)                  # 4-byte Spill
	movl	%ebx, %eax
	mull	88(%esp)                        # 4-byte Folded Reload
	movl	%edx, 52(%esp)                  # 4-byte Spill
	movl	%eax, 100(%esp)                 # 4-byte Spill
	movl	%ebx, %eax
	mull	96(%esp)                        # 4-byte Folded Reload
	movl	%edx, %ecx
	movl	%eax, %ebp
	movl	%ebx, %eax
	mull	84(%esp)                        # 4-byte Folded Reload
	movl	%eax, %edi
	movl	%edx, %esi
	addl	%ebp, %esi
	adcl	100(%esp), %ecx                 # 4-byte Folded Reload
	movl	%ecx, %ebx
	movl	52(%esp), %ecx                  # 4-byte Reload
	adcl	56(%esp), %ecx                  # 4-byte Folded Reload
	movl	36(%esp), %eax                  # 4-byte Reload
	adcl	44(%esp), %eax                  # 4-byte Folded Reload
	movl	40(%esp), %ebp                  # 4-byte Reload
	adcl	20(%esp), %ebp                  # 4-byte Folded Reload
	movl	4(%esp), %edx                   # 4-byte Reload
	adcl	$0, %edx
	addl	32(%esp), %edi                  # 4-byte Folded Reload
	adcl	48(%esp), %esi                  # 4-byte Folded Reload
	movl	%esi, 48(%esp)                  # 4-byte Spill
	adcl	28(%esp), %ebx                  # 4-byte Folded Reload
	movl	%ebx, 32(%esp)                  # 4-byte Spill
	adcl	16(%esp), %ecx                  # 4-byte Folded Reload
	movl	%ecx, 52(%esp)                  # 4-byte Spill
	adcl	8(%esp), %eax                   # 4-byte Folded Reload
	movl	%eax, 36(%esp)                  # 4-byte Spill
	adcl	(%esp), %ebp                    # 4-byte Folded Reload
	movl	%ebp, 40(%esp)                  # 4-byte Spill
	adcl	12(%esp), %edx                  # 4-byte Folded Reload
	movl	%edx, 4(%esp)                   # 4-byte Spill
	movzbl	24(%esp), %eax                  # 1-byte Folded Reload
	adcl	$0, %eax
	movl	%eax, 12(%esp)                  # 4-byte Spill
	movl	156(%esp), %eax
	movl	8(%eax), %esi
	movl	%esi, %eax
	mull	72(%esp)                        # 4-byte Folded Reload
	movl	%edx, 24(%esp)                  # 4-byte Spill
	movl	%eax, 8(%esp)                   # 4-byte Spill
	movl	%esi, %eax
	mull	68(%esp)                        # 4-byte Folded Reload
	movl	%edx, 20(%esp)                  # 4-byte Spill
	movl	%eax, 28(%esp)                  # 4-byte Spill
	movl	%esi, %eax
	mull	64(%esp)                        # 4-byte Folded Reload
	movl	%edx, 16(%esp)                  # 4-byte Spill
	movl	%eax, 44(%esp)                  # 4-byte Spill
	movl	%esi, %eax
	mull	60(%esp)                        # 4-byte Folded Reload
	movl	%edx, %ecx
	movl	%eax, 56(%esp)                  # 4-byte Spill
	movl	%esi, %eax
	mull	104(%esp)                       # 4-byte Folded Reload
	movl	%edx, (%esp)                    # 4-byte Spill
	movl	%eax, %edi
	movl	%esi, %eax
	mull	76(%esp)                        # 4-byte Folded Reload
	movl	%eax, %ebx
	addl	%edi, %edx
	movl	%edx, %ebp
	movl	(%esp), %esi                    # 4-byte Reload
	adcl	56(%esp), %esi                  # 4-byte Folded Reload
	adcl	44(%esp), %ecx                  # 4-byte Folded Reload
	movl	16(%esp), %edi                  # 4-byte Reload
	adcl	28(%esp), %edi                  # 4-byte Folded Reload
	movl	20(%esp), %edx                  # 4-byte Reload
	adcl	8(%esp), %edx                   # 4-byte Folded Reload
	movl	24(%esp), %eax                  # 4-byte Reload
	adcl	$0, %eax
	addl	48(%esp), %ebx                  # 4-byte Folded Reload
	movl	%ebx, 28(%esp)                  # 4-byte Spill
	adcl	32(%esp), %ebp                  # 4-byte Folded Reload
	movl	%ebp, 32(%esp)                  # 4-byte Spill
	adcl	52(%esp), %esi                  # 4-byte Folded Reload
	movl	%esi, (%esp)                    # 4-byte Spill
	adcl	36(%esp), %ecx                  # 4-byte Folded Reload
	movl	%ecx, 52(%esp)                  # 4-byte Spill
	adcl	40(%esp), %edi                  # 4-byte Folded Reload
	movl	%edi, 16(%esp)                  # 4-byte Spill
	adcl	4(%esp), %edx                   # 4-byte Folded Reload
	movl	%edx, 20(%esp)                  # 4-byte Spill
	adcl	12(%esp), %eax                  # 4-byte Folded Reload
	movl	%eax, 24(%esp)                  # 4-byte Spill
	setb	36(%esp)                        # 1-byte Folded Spill
	movl	80(%esp), %edi                  # 4-byte Reload
	imull	%ebx, %edi
	movl	%edi, %eax
	mull	112(%esp)                       # 4-byte Folded Reload
	movl	%edx, 12(%esp)                  # 4-byte Spill
	movl	%eax, 48(%esp)                  # 4-byte Spill
	movl	%edi, %eax
	mull	108(%esp)                       # 4-byte Folded Reload
	movl	%edx, 8(%esp)                   # 4-byte Spill
	movl	%eax, 44(%esp)                  # 4-byte Spill
	movl	%edi, %eax
	mull	92(%esp)                        # 4-byte Folded Reload
	movl	%edx, 4(%esp)                   # 4-byte Spill
	movl	%eax, 56(%esp)                  # 4-byte Spill
	movl	%edi, %eax
	mull	88(%esp)                        # 4-byte Folded Reload
	movl	%edx, 40(%esp)                  # 4-byte Spill
	movl	%eax, 100(%esp)                 # 4-byte Spill
	movl	%edi, %eax
	mull	96(%esp)                        # 4-byte Folded Reload
	movl	%edx, %ecx
	movl	%eax, %ebx
	movl	%edi, %eax
	mull	84(%esp)                        # 4-byte Folded Reload
	movl	%eax, %ebp
	movl	%edx, %esi
	addl	%ebx, %esi
	adcl	100(%esp), %ecx                 # 4-byte Folded Reload
	movl	%ecx, %edi
	movl	40(%esp), %ecx                  # 4-byte Reload
	adcl	56(%esp), %ecx                  # 4-byte Folded Reload
	movl	4(%esp), %eax                   # 4-byte Reload
	adcl	44(%esp), %eax                  # 4-byte Folded Reload
	movl	8(%esp), %ebx                   # 4-byte Reload
	adcl	48(%esp), %ebx                  # 4-byte Folded Reload
	movl	12(%esp), %edx                  # 4-byte Reload
	adcl	$0, %edx
	addl	28(%esp), %ebp                  # 4-byte Folded Reload
	adcl	32(%esp), %esi                  # 4-byte Folded Reload
	movl	%esi, 32(%esp)                  # 4-byte Spill
	adcl	(%esp), %edi                    # 4-byte Folded Reload
	movl	%edi, (%esp)                    # 4-byte Spill
	adcl	52(%esp), %ecx                  # 4-byte Folded Reload
	movl	%ecx, 40(%esp)                  # 4-byte Spill
	adcl	16(%esp), %eax                  # 4-byte Folded Reload
	movl	%eax, 4(%esp)                   # 4-byte Spill
	adcl	20(%esp), %ebx                  # 4-byte Folded Reload
	movl	%ebx, 8(%esp)                   # 4-byte Spill
	adcl	24(%esp), %edx                  # 4-byte Folded Reload
	movl	%edx, 12(%esp)                  # 4-byte Spill
	movzbl	36(%esp), %eax                  # 1-byte Folded Reload
	adcl	$0, %eax
	movl	%eax, 28(%esp)                  # 4-byte Spill
	movl	156(%esp), %eax
	movl	12(%eax), %esi
	movl	%esi, %eax
	mull	72(%esp)                        # 4-byte Folded Reload
	movl	%edx, 24(%esp)                  # 4-byte Spill
	movl	%eax, 36(%esp)                  # 4-byte Spill
	movl	%esi, %eax
	mull	68(%esp)                        # 4-byte Folded Reload
	movl	%edx, 20(%esp)                  # 4-byte Spill
	movl	%eax, 52(%esp)                  # 4-byte Spill
	movl	%esi, %eax
	mull	64(%esp)                        # 4-byte Folded Reload
	movl	%edx, 16(%esp)                  # 4-byte Spill
	movl	%eax, 48(%esp)                  # 4-byte Spill
	movl	%esi, %eax
	mull	60(%esp)                        # 4-byte Folded Reload
	movl	%edx, %ecx
	movl	%eax, 44(%esp)                  # 4-byte Spill
	movl	%esi, %eax
	mull	104(%esp)                       # 4-byte Folded Reload
	movl	%edx, %ebp
	movl	%eax, %ebx
	movl	%esi, %eax
	mull	76(%esp)                        # 4-byte Folded Reload
	movl	%edx, %edi
	addl	%ebx, %edi
	adcl	44(%esp), %ebp                  # 4-byte Folded Reload
	adcl	48(%esp), %ecx                  # 4-byte Folded Reload
	movl	16(%esp), %ebx                  # 4-byte Reload
	adcl	52(%esp), %ebx                  # 4-byte Folded Reload
	movl	20(%esp), %edx                  # 4-byte Reload
	adcl	36(%esp), %edx                  # 4-byte Folded Reload
	movl	24(%esp), %esi                  # 4-byte Reload
	adcl	$0, %esi
	addl	32(%esp), %eax                  # 4-byte Folded Reload
	movl	%eax, 32(%esp)                  # 4-byte Spill
	adcl	(%esp), %edi                    # 4-byte Folded Reload
	movl	%edi, 52(%esp)                  # 4-byte Spill
	adcl	40(%esp), %ebp                  # 4-byte Folded Reload
	movl	%ebp, 36(%esp)                  # 4-byte Spill
	adcl	4(%esp), %ecx                   # 4-byte Folded Reload
	movl	%ecx, 40(%esp)                  # 4-byte Spill
	adcl	8(%esp), %ebx                   # 4-byte Folded Reload
	movl	%ebx, 16(%esp)                  # 4-byte Spill
	adcl	12(%esp), %edx                  # 4-byte Folded Reload
	movl	%edx, 20(%esp)                  # 4-byte Spill
	adcl	28(%esp), %esi                  # 4-byte Folded Reload
	movl	%esi, 24(%esp)                  # 4-byte Spill
	setb	4(%esp)                         # 1-byte Folded Spill
	movl	80(%esp), %edi                  # 4-byte Reload
	imull	%eax, %edi
	movl	%edi, %eax
	mull	112(%esp)                       # 4-byte Folded Reload
	movl	%edx, 12(%esp)                  # 4-byte Spill
	movl	%eax, 28(%esp)                  # 4-byte Spill
	movl	%edi, %eax
	mull	108(%esp)                       # 4-byte Folded Reload
	movl	%edx, 8(%esp)                   # 4-byte Spill
	movl	%eax, 48(%esp)                  # 4-byte Spill
	movl	%edi, %eax
	mull	92(%esp)                        # 4-byte Folded Reload
	movl	%edx, %ebp
	movl	%eax, 44(%esp)                  # 4-byte Spill
	movl	%edi, %eax
	mull	88(%esp)                        # 4-byte Folded Reload
	movl	%edx, %ecx
	movl	%eax, 56(%esp)                  # 4-byte Spill
	movl	%edi, %eax
	mull	96(%esp)                        # 4-byte Folded Reload
	movl	%edx, (%esp)                    # 4-byte Spill
	movl	%eax, %ebx
	movl	%edi, %eax
	mull	84(%esp)                        # 4-byte Folded Reload
	movl	%eax, %esi
	addl	%ebx, %edx
	movl	%edx, %edi
	movl	(%esp), %eax                    # 4-byte Reload
	adcl	56(%esp), %eax                  # 4-byte Folded Reload
	adcl	44(%esp), %ecx                  # 4-byte Folded Reload
	adcl	48(%esp), %ebp                  # 4-byte Folded Reload
	movl	8(%esp), %ebx                   # 4-byte Reload
	adcl	28(%esp), %ebx                  # 4-byte Folded Reload
	movl	12(%esp), %edx                  # 4-byte Reload
	adcl	$0, %edx
	addl	32(%esp), %esi                  # 4-byte Folded Reload
	adcl	52(%esp), %edi                  # 4-byte Folded Reload
	movl	%edi, 52(%esp)                  # 4-byte Spill
	adcl	36(%esp), %eax                  # 4-byte Folded Reload
	movl	%eax, (%esp)                    # 4-byte Spill
	adcl	40(%esp), %ecx                  # 4-byte Folded Reload
	movl	%ecx, 32(%esp)                  # 4-byte Spill
	adcl	16(%esp), %ebp                  # 4-byte Folded Reload
	movl	%ebp, 36(%esp)                  # 4-byte Spill
	adcl	20(%esp), %ebx                  # 4-byte Folded Reload
	movl	%ebx, 8(%esp)                   # 4-byte Spill
	adcl	24(%esp), %edx                  # 4-byte Folded Reload
	movl	%edx, 12(%esp)                  # 4-byte Spill
	movzbl	4(%esp), %eax                   # 1-byte Folded Reload
	adcl	$0, %eax
	movl	%eax, 4(%esp)                   # 4-byte Spill
	movl	156(%esp), %eax
	movl	16(%eax), %esi
	movl	%esi, %eax
	mull	72(%esp)                        # 4-byte Folded Reload
	movl	%edx, 24(%esp)                  # 4-byte Spill
	movl	%eax, 28(%esp)                  # 4-byte Spill
	movl	%esi, %eax
	mull	68(%esp)                        # 4-byte Folded Reload
	movl	%edx, 20(%esp)                  # 4-byte Spill
	movl	%eax, 48(%esp)                  # 4-byte Spill
	movl	%esi, %eax
	mull	64(%esp)                        # 4-byte Folded Reload
	movl	%edx, 16(%esp)                  # 4-byte Spill
	movl	%eax, 44(%esp)                  # 4-byte Spill
	movl	%esi, %eax
	mull	60(%esp)                        # 4-byte Folded Reload
	movl	%edx, %ecx
	movl	%eax, %edi
	movl	%esi, %eax
	mull	104(%esp)                       # 4-byte Folded Reload
	movl	%edx, %ebx
	movl	%eax, %ebp
	movl	%esi, %eax
	mull	76(%esp)                        # 4-byte Folded Reload
	movl	%eax, 40(%esp)                  # 4-byte Spill
	addl	%ebp, %edx
	movl	%edx, %ebp
	adcl	%edi, %ebx
	movl	%ebx, %edi
	adcl	44(%esp), %ecx                  # 4-byte Folded Reload
	movl	16(%esp), %esi                  # 4-byte Reload
	adcl	48(%esp), %esi                  # 4-byte Folded Reload
	movl	20(%esp), %edx                  # 4-byte Reload
	adcl	28(%esp), %edx                  # 4-byte Folded Reload
	movl	24(%esp), %eax                  # 4-byte Reload
	adcl	$0, %eax
	movl	40(%esp), %ebx                  # 4-byte Reload
	addl	52(%esp), %ebx                  # 4-byte Folded Reload
	movl	%ebx, 40(%esp)                  # 4-byte Spill
	adcl	(%esp), %ebp                    # 4-byte Folded Reload
	movl	%ebp, 28(%esp)                  # 4-byte Spill
	adcl	32(%esp), %edi                  # 4-byte Folded Reload
	movl	%edi, 32(%esp)                  # 4-byte Spill
	adcl	36(%esp), %ecx                  # 4-byte Folded Reload
	movl	%ecx, 52(%esp)                  # 4-byte Spill
	adcl	8(%esp), %esi                   # 4-byte Folded Reload
	movl	%esi, 16(%esp)                  # 4-byte Spill
	adcl	12(%esp), %edx                  # 4-byte Folded Reload
	movl	%edx, 20(%esp)                  # 4-byte Spill
	adcl	4(%esp), %eax                   # 4-byte Folded Reload
	movl	%eax, 24(%esp)                  # 4-byte Spill
	setb	36(%esp)                        # 1-byte Folded Spill
	movl	80(%esp), %ecx                  # 4-byte Reload
	imull	%ebx, %ecx
	movl	%ecx, %eax
	mull	112(%esp)                       # 4-byte Folded Reload
	movl	%edx, 12(%esp)                  # 4-byte Spill
	movl	%eax, 48(%esp)                  # 4-byte Spill
	movl	%ecx, %eax
	mull	108(%esp)                       # 4-byte Folded Reload
	movl	%edx, 8(%esp)                   # 4-byte Spill
	movl	%eax, 44(%esp)                  # 4-byte Spill
	movl	%ecx, %eax
	mull	92(%esp)                        # 4-byte Folded Reload
	movl	%edx, 4(%esp)                   # 4-byte Spill
	movl	%eax, 56(%esp)                  # 4-byte Spill
	movl	%ecx, %eax
	mull	88(%esp)                        # 4-byte Folded Reload
	movl	%edx, (%esp)                    # 4-byte Spill
	movl	%eax, %edi
	movl	%ecx, %eax
	mull	96(%esp)                        # 4-byte Folded Reload
	movl	%edx, %ebx
	movl	%eax, %esi
	movl	%ecx, %eax
	mull	84(%esp)                        # 4-byte Folded Reload
	movl	%eax, %ebp
	addl	%esi, %edx
	movl	%edx, %esi
	adcl	%edi, %ebx
	movl	%ebx, %edi
	movl	(%esp), %eax                    # 4-byte Reload
	adcl	56(%esp), %eax                  # 4-byte Folded Reload
	movl	4(%esp), %ebx                   # 4-byte Reload
	adcl	44(%esp), %ebx                  # 4-byte Folded Reload
	movl	8(%esp), %edx                   # 4-byte Reload
	adcl	48(%esp), %edx                  # 4-byte Folded Reload
	movl	12(%esp), %ecx                  # 4-byte Reload
	adcl	$0, %ecx
	addl	40(%esp), %ebp                  # 4-byte Folded Reload
	adcl	28(%esp), %esi                  # 4-byte Folded Reload
	movl	%esi, 28(%esp)                  # 4-byte Spill
	adcl	32(%esp), %edi                  # 4-byte Folded Reload
	movl	%edi, 40(%esp)                  # 4-byte Spill
	adcl	52(%esp), %eax                  # 4-byte Folded Reload
	movl	%eax, (%esp)                    # 4-byte Spill
	adcl	16(%esp), %ebx                  # 4-byte Folded Reload
	movl	%ebx, 4(%esp)                   # 4-byte Spill
	adcl	20(%esp), %edx                  # 4-byte Folded Reload
	movl	%edx, 8(%esp)                   # 4-byte Spill
	adcl	24(%esp), %ecx                  # 4-byte Folded Reload
	movl	%ecx, 12(%esp)                  # 4-byte Spill
	movzbl	36(%esp), %eax                  # 1-byte Folded Reload
	adcl	$0, %eax
	movl	%eax, 20(%esp)                  # 4-byte Spill
	movl	156(%esp), %eax
	movl	20(%eax), %ecx
	movl	%ecx, %eax
	mull	72(%esp)                        # 4-byte Folded Reload
	movl	%edx, 72(%esp)                  # 4-byte Spill
	movl	%eax, 24(%esp)                  # 4-byte Spill
	movl	%ecx, %eax
	mull	68(%esp)                        # 4-byte Folded Reload
	movl	%edx, 68(%esp)                  # 4-byte Spill
	movl	%eax, 16(%esp)                  # 4-byte Spill
	movl	%ecx, %eax
	mull	64(%esp)                        # 4-byte Folded Reload
	movl	%edx, 64(%esp)                  # 4-byte Spill
	movl	%eax, 36(%esp)                  # 4-byte Spill
	movl	%ecx, %eax
	mull	60(%esp)                        # 4-byte Folded Reload
	movl	%edx, 60(%esp)                  # 4-byte Spill
	movl	%eax, %ebp
	movl	%ecx, %eax
	mull	104(%esp)                       # 4-byte Folded Reload
	movl	%edx, %esi
	movl	%eax, %ebx
	movl	%ecx, %eax
	mull	76(%esp)                        # 4-byte Folded Reload
	movl	%eax, %edi
	movl	%edx, %eax
	addl	%ebx, %eax
	adcl	%ebp, %esi
	movl	60(%esp), %ebp                  # 4-byte Reload
	adcl	36(%esp), %ebp                  # 4-byte Folded Reload
	movl	64(%esp), %ebx                  # 4-byte Reload
	adcl	16(%esp), %ebx                  # 4-byte Folded Reload
	movl	68(%esp), %edx                  # 4-byte Reload
	adcl	24(%esp), %edx                  # 4-byte Folded Reload
	movl	72(%esp), %ecx                  # 4-byte Reload
	adcl	$0, %ecx
	addl	28(%esp), %edi                  # 4-byte Folded Reload
	movl	%edi, 16(%esp)                  # 4-byte Spill
	adcl	40(%esp), %eax                  # 4-byte Folded Reload
	movl	%eax, 24(%esp)                  # 4-byte Spill
	adcl	(%esp), %esi                    # 4-byte Folded Reload
	movl	%esi, 104(%esp)                 # 4-byte Spill
	adcl	4(%esp), %ebp                   # 4-byte Folded Reload
	movl	%ebp, 60(%esp)                  # 4-byte Spill
	adcl	8(%esp), %ebx                   # 4-byte Folded Reload
	movl	%ebx, 64(%esp)                  # 4-byte Spill
	adcl	12(%esp), %edx                  # 4-byte Folded Reload
	movl	%edx, 68(%esp)                  # 4-byte Spill
	adcl	20(%esp), %ecx                  # 4-byte Folded Reload
	movl	%ecx, 72(%esp)                  # 4-byte Spill
	setb	76(%esp)                        # 1-byte Folded Spill
	movl	80(%esp), %esi                  # 4-byte Reload
	imull	%edi, %esi
	movl	%esi, %eax
	mull	112(%esp)                       # 4-byte Folded Reload
	movl	%edx, 80(%esp)                  # 4-byte Spill
	movl	%eax, 20(%esp)                  # 4-byte Spill
	movl	%esi, %eax
	mull	108(%esp)                       # 4-byte Folded Reload
	movl	%edx, %ebp
	movl	%eax, (%esp)                    # 4-byte Spill
	movl	%esi, %eax
	mull	92(%esp)                        # 4-byte Folded Reload
	movl	%edx, %edi
	movl	%eax, 12(%esp)                  # 4-byte Spill
	movl	%esi, %eax
	mull	88(%esp)                        # 4-byte Folded Reload
	movl	%edx, %ecx
	movl	%eax, 8(%esp)                   # 4-byte Spill
	movl	%esi, %eax
	mull	84(%esp)                        # 4-byte Folded Reload
	movl	%edx, 4(%esp)                   # 4-byte Spill
	movl	%eax, %ebx
	movl	%esi, %eax
	mull	96(%esp)                        # 4-byte Folded Reload
	addl	4(%esp), %eax                   # 4-byte Folded Reload
	adcl	8(%esp), %edx                   # 4-byte Folded Reload
	adcl	12(%esp), %ecx                  # 4-byte Folded Reload
	adcl	(%esp), %edi                    # 4-byte Folded Reload
	adcl	20(%esp), %ebp                  # 4-byte Folded Reload
	movl	80(%esp), %esi                  # 4-byte Reload
	adcl	$0, %esi
	addl	16(%esp), %ebx                  # 4-byte Folded Reload
	adcl	24(%esp), %eax                  # 4-byte Folded Reload
	adcl	104(%esp), %edx                 # 4-byte Folded Reload
	adcl	60(%esp), %ecx                  # 4-byte Folded Reload
	adcl	64(%esp), %edi                  # 4-byte Folded Reload
	adcl	68(%esp), %ebp                  # 4-byte Folded Reload
	adcl	72(%esp), %esi                  # 4-byte Folded Reload
	movzbl	76(%esp), %ebx                  # 1-byte Folded Reload
	adcl	$0, %ebx
	movl	%eax, 76(%esp)                  # 4-byte Spill
	subl	84(%esp), %eax                  # 4-byte Folded Reload
	movl	%eax, 84(%esp)                  # 4-byte Spill
	movl	%edx, %eax
	sbbl	96(%esp), %eax                  # 4-byte Folded Reload
	movl	%ecx, 96(%esp)                  # 4-byte Spill
	sbbl	88(%esp), %ecx                  # 4-byte Folded Reload
	movl	%edi, 88(%esp)                  # 4-byte Spill
	sbbl	92(%esp), %edi                  # 4-byte Folded Reload
	movl	%ebp, 92(%esp)                  # 4-byte Spill
	sbbl	108(%esp), %ebp                 # 4-byte Folded Reload
	movl	%esi, 80(%esp)                  # 4-byte Spill
	sbbl	112(%esp), %esi                 # 4-byte Folded Reload
	sbbl	$0, %ebx
	testb	$1, %bl
	jne	.LBB9_1
# %bb.2:
	movl	148(%esp), %ebx
	movl	%esi, 20(%ebx)
	jne	.LBB9_3
.LBB9_4:
	movl	%ebp, 16(%ebx)
	jne	.LBB9_5
.LBB9_6:
	movl	%edi, 12(%ebx)
	jne	.LBB9_7
.LBB9_8:
	movl	%ecx, 8(%ebx)
	jne	.LBB9_9
.LBB9_10:
	movl	%eax, 4(%ebx)
	movl	84(%esp), %eax                  # 4-byte Reload
	je	.LBB9_12
.LBB9_11:
	movl	76(%esp), %eax                  # 4-byte Reload
.LBB9_12:
	movl	%eax, (%ebx)
	addl	$128, %esp
	popl	%esi
	popl	%edi
	popl	%ebx
	popl	%ebp
	retl
.LBB9_1:
	movl	80(%esp), %esi                  # 4-byte Reload
	movl	148(%esp), %ebx
	movl	%esi, 20(%ebx)
	je	.LBB9_4
.LBB9_3:
	movl	92(%esp), %ebp                  # 4-byte Reload
	movl	%ebp, 16(%ebx)
	je	.LBB9_6
.LBB9_5:
	movl	88(%esp), %edi                  # 4-byte Reload
	movl	%edi, 12(%ebx)
	je	.LBB9_8
.LBB9_7:
	movl	96(%esp), %ecx                  # 4-byte Reload
	movl	%ecx, 8(%ebx)
	je	.LBB9_10
.LBB9_9:
	movl	%edx, %eax
	movl	%eax, 4(%ebx)
	movl	84(%esp), %eax                  # 4-byte Reload
	jne	.LBB9_11
	jmp	.LBB9_12
.Lfunc_end9:
	.size	mcl_fp_mont6L, .Lfunc_end9-mcl_fp_mont6L
                                        # -- End function
	.globl	mcl_fp_montNF6L                 # -- Begin function mcl_fp_montNF6L
	.p2align	4, 0x90
	.type	mcl_fp_montNF6L,@function
mcl_fp_montNF6L:                        # @mcl_fp_montNF6L
# %bb.0:
	pushl	%ebp
	pushl	%ebx
	pushl	%edi
	pushl	%esi
	subl	$128, %esp
	movl	152(%esp), %esi
	movl	(%esi), %eax
	movl	%eax, 60(%esp)                  # 4-byte Spill
	movl	156(%esp), %ecx
	movl	(%ecx), %ebx
	mull	%ebx
	movl	%eax, 24(%esp)                  # 4-byte Spill
	movl	%edx, 32(%esp)                  # 4-byte Spill
	movl	160(%esp), %edi
	movl	-4(%edi), %ecx
	movl	%ecx, 76(%esp)                  # 4-byte Spill
	imull	%eax, %ecx
	movl	20(%edi), %edx
	movl	%edx, 104(%esp)                 # 4-byte Spill
	movl	%ecx, %eax
	mull	%edx
	movl	%eax, 4(%esp)                   # 4-byte Spill
	movl	%edx, 12(%esp)                  # 4-byte Spill
	movl	16(%edi), %edx
	movl	%edx, 100(%esp)                 # 4-byte Spill
	movl	%ecx, %eax
	mull	%edx
	movl	%eax, 28(%esp)                  # 4-byte Spill
	movl	%edx, 8(%esp)                   # 4-byte Spill
	movl	12(%edi), %edx
	movl	%edx, 96(%esp)                  # 4-byte Spill
	movl	%ecx, %eax
	mull	%edx
	movl	%eax, 44(%esp)                  # 4-byte Spill
	movl	%edx, (%esp)                    # 4-byte Spill
	movl	8(%edi), %edx
	movl	%edx, 84(%esp)                  # 4-byte Spill
	movl	%ecx, %eax
	mull	%edx
	movl	%eax, 52(%esp)                  # 4-byte Spill
	movl	%edx, 36(%esp)                  # 4-byte Spill
	movl	(%edi), %ebp
	movl	%ebp, 88(%esp)                  # 4-byte Spill
	movl	4(%edi), %edx
	movl	%edx, 80(%esp)                  # 4-byte Spill
	movl	%ecx, %eax
	mull	%edx
	movl	%edx, 48(%esp)                  # 4-byte Spill
	movl	%eax, 124(%esp)                 # 4-byte Spill
	movl	%ecx, %eax
	mull	%ebp
	movl	%eax, 120(%esp)                 # 4-byte Spill
	movl	%edx, 40(%esp)                  # 4-byte Spill
	movl	20(%esi), %eax
	movl	%eax, 56(%esp)                  # 4-byte Spill
	mull	%ebx
	movl	%eax, 116(%esp)                 # 4-byte Spill
	movl	%edx, 20(%esp)                  # 4-byte Spill
	movl	16(%esi), %eax
	movl	%eax, 72(%esp)                  # 4-byte Spill
	mull	%ebx
	movl	%eax, 112(%esp)                 # 4-byte Spill
	movl	%edx, 16(%esp)                  # 4-byte Spill
	movl	12(%esi), %eax
	movl	%eax, 68(%esp)                  # 4-byte Spill
	mull	%ebx
	movl	%eax, 108(%esp)                 # 4-byte Spill
	movl	%edx, %ecx
	movl	4(%esi), %ebp
	movl	%ebp, 92(%esp)                  # 4-byte Spill
	movl	8(%esi), %eax
	movl	%eax, 64(%esp)                  # 4-byte Spill
	mull	%ebx
	movl	%edx, %edi
	movl	%eax, %esi
	movl	%ebp, %eax
	mull	%ebx
	movl	%edx, %ebx
	movl	%eax, %ebp
	addl	32(%esp), %ebp                  # 4-byte Folded Reload
	adcl	%esi, %ebx
	adcl	108(%esp), %edi                 # 4-byte Folded Reload
	adcl	112(%esp), %ecx                 # 4-byte Folded Reload
	movl	16(%esp), %edx                  # 4-byte Reload
	adcl	116(%esp), %edx                 # 4-byte Folded Reload
	movl	20(%esp), %eax                  # 4-byte Reload
	adcl	$0, %eax
	movl	120(%esp), %esi                 # 4-byte Reload
	addl	24(%esp), %esi                  # 4-byte Folded Reload
	adcl	124(%esp), %ebp                 # 4-byte Folded Reload
	adcl	52(%esp), %ebx                  # 4-byte Folded Reload
	adcl	44(%esp), %edi                  # 4-byte Folded Reload
	adcl	28(%esp), %ecx                  # 4-byte Folded Reload
	adcl	4(%esp), %edx                   # 4-byte Folded Reload
	adcl	$0, %eax
	addl	40(%esp), %ebp                  # 4-byte Folded Reload
	adcl	48(%esp), %ebx                  # 4-byte Folded Reload
	adcl	36(%esp), %edi                  # 4-byte Folded Reload
	movl	%edi, 48(%esp)                  # 4-byte Spill
	adcl	(%esp), %ecx                    # 4-byte Folded Reload
	movl	%ecx, 4(%esp)                   # 4-byte Spill
	adcl	8(%esp), %edx                   # 4-byte Folded Reload
	movl	%edx, 16(%esp)                  # 4-byte Spill
	adcl	12(%esp), %eax                  # 4-byte Folded Reload
	movl	%eax, 20(%esp)                  # 4-byte Spill
	movl	156(%esp), %eax
	movl	4(%eax), %esi
	movl	%esi, %eax
	mull	56(%esp)                        # 4-byte Folded Reload
	movl	%edx, %edi
	movl	%eax, 12(%esp)                  # 4-byte Spill
	movl	%esi, %eax
	mull	72(%esp)                        # 4-byte Folded Reload
	movl	%edx, (%esp)                    # 4-byte Spill
	movl	%eax, 8(%esp)                   # 4-byte Spill
	movl	%esi, %eax
	mull	68(%esp)                        # 4-byte Folded Reload
	movl	%edx, 28(%esp)                  # 4-byte Spill
	movl	%eax, 36(%esp)                  # 4-byte Spill
	movl	%esi, %eax
	mull	64(%esp)                        # 4-byte Folded Reload
	movl	%edx, %ecx
	movl	%eax, 40(%esp)                  # 4-byte Spill
	movl	%esi, %eax
	mull	92(%esp)                        # 4-byte Folded Reload
	movl	%edx, 24(%esp)                  # 4-byte Spill
	movl	%eax, 32(%esp)                  # 4-byte Spill
	movl	%esi, %eax
	mull	60(%esp)                        # 4-byte Folded Reload
	movl	%eax, 44(%esp)                  # 4-byte Spill
	addl	32(%esp), %edx                  # 4-byte Folded Reload
	movl	%edx, 32(%esp)                  # 4-byte Spill
	movl	24(%esp), %esi                  # 4-byte Reload
	adcl	40(%esp), %esi                  # 4-byte Folded Reload
	adcl	36(%esp), %ecx                  # 4-byte Folded Reload
	movl	%ecx, 36(%esp)                  # 4-byte Spill
	movl	28(%esp), %ecx                  # 4-byte Reload
	adcl	8(%esp), %ecx                   # 4-byte Folded Reload
	movl	(%esp), %edx                    # 4-byte Reload
	adcl	12(%esp), %edx                  # 4-byte Folded Reload
	movl	%edi, %eax
	adcl	$0, %eax
	movl	44(%esp), %edi                  # 4-byte Reload
	addl	%ebp, %edi
	adcl	%ebx, 32(%esp)                  # 4-byte Folded Spill
	adcl	48(%esp), %esi                  # 4-byte Folded Reload
	movl	%esi, 24(%esp)                  # 4-byte Spill
	movl	4(%esp), %esi                   # 4-byte Reload
	adcl	%esi, 36(%esp)                  # 4-byte Folded Spill
	adcl	16(%esp), %ecx                  # 4-byte Folded Reload
	movl	%ecx, 28(%esp)                  # 4-byte Spill
	adcl	20(%esp), %edx                  # 4-byte Folded Reload
	movl	%edx, (%esp)                    # 4-byte Spill
	adcl	$0, %eax
	movl	%eax, 16(%esp)                  # 4-byte Spill
	movl	76(%esp), %ecx                  # 4-byte Reload
	imull	%edi, %ecx
	movl	%ecx, %eax
	mull	104(%esp)                       # 4-byte Folded Reload
	movl	%edx, 20(%esp)                  # 4-byte Spill
	movl	%eax, 12(%esp)                  # 4-byte Spill
	movl	%ecx, %eax
	mull	100(%esp)                       # 4-byte Folded Reload
	movl	%edx, 48(%esp)                  # 4-byte Spill
	movl	%eax, 8(%esp)                   # 4-byte Spill
	movl	%ecx, %eax
	mull	96(%esp)                        # 4-byte Folded Reload
	movl	%edx, 44(%esp)                  # 4-byte Spill
	movl	%eax, 4(%esp)                   # 4-byte Spill
	movl	%ecx, %eax
	mull	84(%esp)                        # 4-byte Folded Reload
	movl	%edx, 40(%esp)                  # 4-byte Spill
	movl	%eax, %ebx
	movl	%ecx, %eax
	mull	80(%esp)                        # 4-byte Folded Reload
	movl	%edx, 52(%esp)                  # 4-byte Spill
	movl	%eax, %esi
	movl	%ecx, %eax
	mull	88(%esp)                        # 4-byte Folded Reload
	movl	%edx, %ebp
	addl	%edi, %eax
	adcl	32(%esp), %esi                  # 4-byte Folded Reload
	movl	%esi, %edi
	movl	%ebx, %edx
	adcl	24(%esp), %edx                  # 4-byte Folded Reload
	movl	4(%esp), %esi                   # 4-byte Reload
	adcl	36(%esp), %esi                  # 4-byte Folded Reload
	movl	8(%esp), %ebx                   # 4-byte Reload
	adcl	28(%esp), %ebx                  # 4-byte Folded Reload
	movl	12(%esp), %ecx                  # 4-byte Reload
	adcl	(%esp), %ecx                    # 4-byte Folded Reload
	movl	16(%esp), %eax                  # 4-byte Reload
	adcl	$0, %eax
	addl	%ebp, %edi
	movl	%edi, 36(%esp)                  # 4-byte Spill
	adcl	52(%esp), %edx                  # 4-byte Folded Reload
	movl	%edx, 24(%esp)                  # 4-byte Spill
	adcl	40(%esp), %esi                  # 4-byte Folded Reload
	movl	%esi, 4(%esp)                   # 4-byte Spill
	adcl	44(%esp), %ebx                  # 4-byte Folded Reload
	movl	%ebx, 8(%esp)                   # 4-byte Spill
	adcl	48(%esp), %ecx                  # 4-byte Folded Reload
	movl	%ecx, 12(%esp)                  # 4-byte Spill
	adcl	20(%esp), %eax                  # 4-byte Folded Reload
	movl	%eax, 16(%esp)                  # 4-byte Spill
	movl	156(%esp), %eax
	movl	8(%eax), %ecx
	movl	%ecx, %eax
	mull	56(%esp)                        # 4-byte Folded Reload
	movl	%edx, 20(%esp)                  # 4-byte Spill
	movl	%eax, 32(%esp)                  # 4-byte Spill
	movl	%ecx, %eax
	mull	72(%esp)                        # 4-byte Folded Reload
	movl	%edx, (%esp)                    # 4-byte Spill
	movl	%eax, 48(%esp)                  # 4-byte Spill
	movl	%ecx, %eax
	mull	68(%esp)                        # 4-byte Folded Reload
	movl	%edx, 28(%esp)                  # 4-byte Spill
	movl	%eax, 44(%esp)                  # 4-byte Spill
	movl	%ecx, %eax
	mull	64(%esp)                        # 4-byte Folded Reload
	movl	%edx, %edi
	movl	%eax, 40(%esp)                  # 4-byte Spill
	movl	%ecx, %eax
	mull	92(%esp)                        # 4-byte Folded Reload
	movl	%edx, %ebp
	movl	%eax, %esi
	movl	%ecx, %eax
	mull	60(%esp)                        # 4-byte Folded Reload
	movl	%eax, %ecx
	movl	%edx, %ebx
	addl	%esi, %ebx
	adcl	40(%esp), %ebp                  # 4-byte Folded Reload
	adcl	44(%esp), %edi                  # 4-byte Folded Reload
	movl	28(%esp), %esi                  # 4-byte Reload
	adcl	48(%esp), %esi                  # 4-byte Folded Reload
	movl	(%esp), %edx                    # 4-byte Reload
	adcl	32(%esp), %edx                  # 4-byte Folded Reload
	movl	20(%esp), %eax                  # 4-byte Reload
	adcl	$0, %eax
	addl	36(%esp), %ecx                  # 4-byte Folded Reload
	adcl	24(%esp), %ebx                  # 4-byte Folded Reload
	adcl	4(%esp), %ebp                   # 4-byte Folded Reload
	adcl	8(%esp), %edi                   # 4-byte Folded Reload
	movl	%edi, 32(%esp)                  # 4-byte Spill
	adcl	12(%esp), %esi                  # 4-byte Folded Reload
	movl	%esi, 28(%esp)                  # 4-byte Spill
	adcl	16(%esp), %edx                  # 4-byte Folded Reload
	movl	%edx, (%esp)                    # 4-byte Spill
	adcl	$0, %eax
	movl	%eax, 20(%esp)                  # 4-byte Spill
	movl	76(%esp), %edi                  # 4-byte Reload
	imull	%ecx, %edi
	movl	%edi, %eax
	mull	104(%esp)                       # 4-byte Folded Reload
	movl	%edx, 24(%esp)                  # 4-byte Spill
	movl	%eax, 16(%esp)                  # 4-byte Spill
	movl	%edi, %eax
	mull	100(%esp)                       # 4-byte Folded Reload
	movl	%edx, 36(%esp)                  # 4-byte Spill
	movl	%eax, 12(%esp)                  # 4-byte Spill
	movl	%edi, %eax
	mull	96(%esp)                        # 4-byte Folded Reload
	movl	%edx, 48(%esp)                  # 4-byte Spill
	movl	%eax, 8(%esp)                   # 4-byte Spill
	movl	%edi, %eax
	mull	84(%esp)                        # 4-byte Folded Reload
	movl	%edx, 44(%esp)                  # 4-byte Spill
	movl	%eax, 4(%esp)                   # 4-byte Spill
	movl	%edi, %eax
	mull	80(%esp)                        # 4-byte Folded Reload
	movl	%edx, 40(%esp)                  # 4-byte Spill
	movl	%eax, %esi
	movl	%edi, %eax
	mull	88(%esp)                        # 4-byte Folded Reload
	movl	%edx, 52(%esp)                  # 4-byte Spill
	addl	%ecx, %eax
	adcl	%ebx, %esi
	movl	4(%esp), %edx                   # 4-byte Reload
	adcl	%ebp, %edx
	movl	8(%esp), %ebx                   # 4-byte Reload
	adcl	32(%esp), %ebx                  # 4-byte Folded Reload
	movl	12(%esp), %edi                  # 4-byte Reload
	adcl	28(%esp), %edi                  # 4-byte Folded Reload
	movl	16(%esp), %ecx                  # 4-byte Reload
	adcl	(%esp), %ecx                    # 4-byte Folded Reload
	movl	20(%esp), %eax                  # 4-byte Reload
	adcl	$0, %eax
	addl	52(%esp), %esi                  # 4-byte Folded Reload
	movl	%esi, (%esp)                    # 4-byte Spill
	adcl	40(%esp), %edx                  # 4-byte Folded Reload
	movl	%edx, 4(%esp)                   # 4-byte Spill
	adcl	44(%esp), %ebx                  # 4-byte Folded Reload
	movl	%ebx, 8(%esp)                   # 4-byte Spill
	adcl	48(%esp), %edi                  # 4-byte Folded Reload
	movl	%edi, 12(%esp)                  # 4-byte Spill
	adcl	36(%esp), %ecx                  # 4-byte Folded Reload
	movl	%ecx, 16(%esp)                  # 4-byte Spill
	adcl	24(%esp), %eax                  # 4-byte Folded Reload
	movl	%eax, 20(%esp)                  # 4-byte Spill
	movl	156(%esp), %eax
	movl	12(%eax), %ecx
	movl	%ecx, %eax
	mull	56(%esp)                        # 4-byte Folded Reload
	movl	%edx, %esi
	movl	%eax, 32(%esp)                  # 4-byte Spill
	movl	%ecx, %eax
	mull	72(%esp)                        # 4-byte Folded Reload
	movl	%edx, 28(%esp)                  # 4-byte Spill
	movl	%eax, 48(%esp)                  # 4-byte Spill
	movl	%ecx, %eax
	mull	68(%esp)                        # 4-byte Folded Reload
	movl	%edx, 24(%esp)                  # 4-byte Spill
	movl	%eax, 44(%esp)                  # 4-byte Spill
	movl	%ecx, %eax
	mull	64(%esp)                        # 4-byte Folded Reload
	movl	%edx, %ebp
	movl	%eax, 40(%esp)                  # 4-byte Spill
	movl	%ecx, %eax
	mull	92(%esp)                        # 4-byte Folded Reload
	movl	%edx, %ebx
	movl	%eax, 52(%esp)                  # 4-byte Spill
	movl	%ecx, %eax
	mull	60(%esp)                        # 4-byte Folded Reload
	movl	%eax, 36(%esp)                  # 4-byte Spill
	movl	%edx, %edi
	addl	52(%esp), %edi                  # 4-byte Folded Reload
	adcl	40(%esp), %ebx                  # 4-byte Folded Reload
	adcl	44(%esp), %ebp                  # 4-byte Folded Reload
	movl	24(%esp), %edx                  # 4-byte Reload
	adcl	48(%esp), %edx                  # 4-byte Folded Reload
	movl	28(%esp), %ecx                  # 4-byte Reload
	adcl	32(%esp), %ecx                  # 4-byte Folded Reload
	movl	%esi, %eax
	adcl	$0, %eax
	movl	36(%esp), %esi                  # 4-byte Reload
	addl	(%esp), %esi                    # 4-byte Folded Reload
	movl	%esi, 36(%esp)                  # 4-byte Spill
	adcl	4(%esp), %edi                   # 4-byte Folded Reload
	adcl	8(%esp), %ebx                   # 4-byte Folded Reload
	adcl	12(%esp), %ebp                  # 4-byte Folded Reload
	adcl	16(%esp), %edx                  # 4-byte Folded Reload
	movl	%edx, 24(%esp)                  # 4-byte Spill
	adcl	20(%esp), %ecx                  # 4-byte Folded Reload
	movl	%ecx, 28(%esp)                  # 4-byte Spill
	adcl	$0, %eax
	movl	%eax, 16(%esp)                  # 4-byte Spill
	movl	76(%esp), %ecx                  # 4-byte Reload
	imull	%esi, %ecx
	movl	%ecx, %eax
	mull	104(%esp)                       # 4-byte Folded Reload
	movl	%edx, 20(%esp)                  # 4-byte Spill
	movl	%eax, 12(%esp)                  # 4-byte Spill
	movl	%ecx, %eax
	mull	100(%esp)                       # 4-byte Folded Reload
	movl	%edx, 32(%esp)                  # 4-byte Spill
	movl	%eax, 8(%esp)                   # 4-byte Spill
	movl	%ecx, %eax
	mull	96(%esp)                        # 4-byte Folded Reload
	movl	%edx, 48(%esp)                  # 4-byte Spill
	movl	%eax, 4(%esp)                   # 4-byte Spill
	movl	%ecx, %eax
	mull	84(%esp)                        # 4-byte Folded Reload
	movl	%edx, 44(%esp)                  # 4-byte Spill
	movl	%eax, (%esp)                    # 4-byte Spill
	movl	%ecx, %eax
	mull	80(%esp)                        # 4-byte Folded Reload
	movl	%edx, 40(%esp)                  # 4-byte Spill
	movl	%eax, %esi
	movl	%ecx, %eax
	mull	88(%esp)                        # 4-byte Folded Reload
	movl	%edx, 52(%esp)                  # 4-byte Spill
	addl	36(%esp), %eax                  # 4-byte Folded Reload
	adcl	%edi, %esi
	movl	(%esp), %edx                    # 4-byte Reload
	adcl	%ebx, %edx
	movl	4(%esp), %ebx                   # 4-byte Reload
	adcl	%ebp, %ebx
	movl	8(%esp), %edi                   # 4-byte Reload
	adcl	24(%esp), %edi                  # 4-byte Folded Reload
	movl	12(%esp), %ecx                  # 4-byte Reload
	adcl	28(%esp), %ecx                  # 4-byte Folded Reload
	movl	16(%esp), %eax                  # 4-byte Reload
	adcl	$0, %eax
	addl	52(%esp), %esi                  # 4-byte Folded Reload
	movl	%esi, 36(%esp)                  # 4-byte Spill
	adcl	40(%esp), %edx                  # 4-byte Folded Reload
	movl	%edx, (%esp)                    # 4-byte Spill
	adcl	44(%esp), %ebx                  # 4-byte Folded Reload
	movl	%ebx, 4(%esp)                   # 4-byte Spill
	adcl	48(%esp), %edi                  # 4-byte Folded Reload
	movl	%edi, 8(%esp)                   # 4-byte Spill
	adcl	32(%esp), %ecx                  # 4-byte Folded Reload
	movl	%ecx, 12(%esp)                  # 4-byte Spill
	adcl	20(%esp), %eax                  # 4-byte Folded Reload
	movl	%eax, 16(%esp)                  # 4-byte Spill
	movl	156(%esp), %eax
	movl	16(%eax), %ecx
	movl	%ecx, %eax
	mull	56(%esp)                        # 4-byte Folded Reload
	movl	%edx, 20(%esp)                  # 4-byte Spill
	movl	%eax, 32(%esp)                  # 4-byte Spill
	movl	%ecx, %eax
	mull	72(%esp)                        # 4-byte Folded Reload
	movl	%edx, 28(%esp)                  # 4-byte Spill
	movl	%eax, 48(%esp)                  # 4-byte Spill
	movl	%ecx, %eax
	mull	68(%esp)                        # 4-byte Folded Reload
	movl	%edx, %ebx
	movl	%eax, 44(%esp)                  # 4-byte Spill
	movl	%ecx, %eax
	mull	64(%esp)                        # 4-byte Folded Reload
	movl	%edx, %esi
	movl	%eax, 40(%esp)                  # 4-byte Spill
	movl	%ecx, %eax
	mull	92(%esp)                        # 4-byte Folded Reload
	movl	%edx, %ebp
	movl	%eax, 52(%esp)                  # 4-byte Spill
	movl	%ecx, %eax
	mull	60(%esp)                        # 4-byte Folded Reload
	movl	%eax, 24(%esp)                  # 4-byte Spill
	movl	%edx, %edi
	addl	52(%esp), %edi                  # 4-byte Folded Reload
	adcl	40(%esp), %ebp                  # 4-byte Folded Reload
	adcl	44(%esp), %esi                  # 4-byte Folded Reload
	adcl	48(%esp), %ebx                  # 4-byte Folded Reload
	movl	28(%esp), %ecx                  # 4-byte Reload
	adcl	32(%esp), %ecx                  # 4-byte Folded Reload
	movl	20(%esp), %eax                  # 4-byte Reload
	adcl	$0, %eax
	movl	24(%esp), %edx                  # 4-byte Reload
	addl	36(%esp), %edx                  # 4-byte Folded Reload
	movl	%edx, 24(%esp)                  # 4-byte Spill
	adcl	(%esp), %edi                    # 4-byte Folded Reload
	adcl	4(%esp), %ebp                   # 4-byte Folded Reload
	adcl	8(%esp), %esi                   # 4-byte Folded Reload
	adcl	12(%esp), %ebx                  # 4-byte Folded Reload
	movl	%ebx, (%esp)                    # 4-byte Spill
	adcl	16(%esp), %ecx                  # 4-byte Folded Reload
	movl	%ecx, 28(%esp)                  # 4-byte Spill
	adcl	$0, %eax
	movl	%eax, 20(%esp)                  # 4-byte Spill
	movl	76(%esp), %ecx                  # 4-byte Reload
	imull	%edx, %ecx
	movl	%ecx, %eax
	mull	104(%esp)                       # 4-byte Folded Reload
	movl	%edx, 36(%esp)                  # 4-byte Spill
	movl	%eax, 16(%esp)                  # 4-byte Spill
	movl	%ecx, %eax
	mull	100(%esp)                       # 4-byte Folded Reload
	movl	%edx, 32(%esp)                  # 4-byte Spill
	movl	%eax, 12(%esp)                  # 4-byte Spill
	movl	%ecx, %eax
	mull	96(%esp)                        # 4-byte Folded Reload
	movl	%edx, 48(%esp)                  # 4-byte Spill
	movl	%eax, 8(%esp)                   # 4-byte Spill
	movl	%ecx, %eax
	mull	84(%esp)                        # 4-byte Folded Reload
	movl	%edx, 44(%esp)                  # 4-byte Spill
	movl	%eax, 4(%esp)                   # 4-byte Spill
	movl	%ecx, %eax
	mull	80(%esp)                        # 4-byte Folded Reload
	movl	%edx, 40(%esp)                  # 4-byte Spill
	movl	%eax, %ebx
	movl	%ecx, %eax
	mull	88(%esp)                        # 4-byte Folded Reload
	movl	%edx, 52(%esp)                  # 4-byte Spill
	addl	24(%esp), %eax                  # 4-byte Folded Reload
	adcl	%edi, %ebx
	movl	%ebx, %edx
	movl	4(%esp), %ebx                   # 4-byte Reload
	adcl	%ebp, %ebx
	movl	8(%esp), %edi                   # 4-byte Reload
	adcl	%esi, %edi
	movl	12(%esp), %ecx                  # 4-byte Reload
	adcl	(%esp), %ecx                    # 4-byte Folded Reload
	movl	16(%esp), %eax                  # 4-byte Reload
	adcl	28(%esp), %eax                  # 4-byte Folded Reload
	movl	20(%esp), %esi                  # 4-byte Reload
	adcl	$0, %esi
	addl	52(%esp), %edx                  # 4-byte Folded Reload
	movl	%edx, (%esp)                    # 4-byte Spill
	adcl	40(%esp), %ebx                  # 4-byte Folded Reload
	movl	%ebx, 4(%esp)                   # 4-byte Spill
	adcl	44(%esp), %edi                  # 4-byte Folded Reload
	movl	%edi, 8(%esp)                   # 4-byte Spill
	adcl	48(%esp), %ecx                  # 4-byte Folded Reload
	movl	%ecx, 12(%esp)                  # 4-byte Spill
	adcl	32(%esp), %eax                  # 4-byte Folded Reload
	movl	%eax, 16(%esp)                  # 4-byte Spill
	adcl	36(%esp), %esi                  # 4-byte Folded Reload
	movl	%esi, 20(%esp)                  # 4-byte Spill
	movl	156(%esp), %eax
	movl	20(%eax), %ecx
	movl	%ecx, %eax
	mull	56(%esp)                        # 4-byte Folded Reload
	movl	%edx, 56(%esp)                  # 4-byte Spill
	movl	%eax, 28(%esp)                  # 4-byte Spill
	movl	%ecx, %eax
	mull	72(%esp)                        # 4-byte Folded Reload
	movl	%edx, 72(%esp)                  # 4-byte Spill
	movl	%eax, 24(%esp)                  # 4-byte Spill
	movl	%ecx, %eax
	mull	68(%esp)                        # 4-byte Folded Reload
	movl	%edx, %esi
	movl	%eax, 68(%esp)                  # 4-byte Spill
	movl	%ecx, %eax
	mull	64(%esp)                        # 4-byte Folded Reload
	movl	%edx, %edi
	movl	%eax, 64(%esp)                  # 4-byte Spill
	movl	%ecx, %eax
	mull	92(%esp)                        # 4-byte Folded Reload
	movl	%edx, %ebx
	movl	%eax, %ebp
	movl	%ecx, %eax
	mull	60(%esp)                        # 4-byte Folded Reload
	movl	%eax, 60(%esp)                  # 4-byte Spill
	addl	%ebp, %edx
	movl	%edx, %ebp
	adcl	64(%esp), %ebx                  # 4-byte Folded Reload
	movl	%edi, %ecx
	adcl	68(%esp), %ecx                  # 4-byte Folded Reload
	adcl	24(%esp), %esi                  # 4-byte Folded Reload
	movl	72(%esp), %eax                  # 4-byte Reload
	adcl	28(%esp), %eax                  # 4-byte Folded Reload
	movl	56(%esp), %edi                  # 4-byte Reload
	adcl	$0, %edi
	movl	60(%esp), %edx                  # 4-byte Reload
	addl	(%esp), %edx                    # 4-byte Folded Reload
	movl	%edx, 60(%esp)                  # 4-byte Spill
	adcl	4(%esp), %ebp                   # 4-byte Folded Reload
	movl	%ebp, 4(%esp)                   # 4-byte Spill
	adcl	8(%esp), %ebx                   # 4-byte Folded Reload
	movl	%ebx, 8(%esp)                   # 4-byte Spill
	adcl	12(%esp), %ecx                  # 4-byte Folded Reload
	movl	%ecx, 12(%esp)                  # 4-byte Spill
	adcl	16(%esp), %esi                  # 4-byte Folded Reload
	movl	%esi, 64(%esp)                  # 4-byte Spill
	adcl	20(%esp), %eax                  # 4-byte Folded Reload
	movl	%eax, 72(%esp)                  # 4-byte Spill
	adcl	$0, %edi
	movl	%edi, 56(%esp)                  # 4-byte Spill
	movl	76(%esp), %esi                  # 4-byte Reload
	imull	%edx, %esi
	movl	%esi, %eax
	mull	104(%esp)                       # 4-byte Folded Reload
	movl	%edx, 92(%esp)                  # 4-byte Spill
	movl	%eax, 76(%esp)                  # 4-byte Spill
	movl	%esi, %eax
	mull	100(%esp)                       # 4-byte Folded Reload
	movl	%edx, 68(%esp)                  # 4-byte Spill
	movl	%eax, %ebp
	movl	%esi, %eax
	mull	96(%esp)                        # 4-byte Folded Reload
	movl	%edx, 20(%esp)                  # 4-byte Spill
	movl	%eax, %edi
	movl	%esi, %eax
	mull	88(%esp)                        # 4-byte Folded Reload
	movl	%edx, 16(%esp)                  # 4-byte Spill
	movl	%eax, %ebx
	movl	%esi, %eax
	mull	84(%esp)                        # 4-byte Folded Reload
	movl	%edx, (%esp)                    # 4-byte Spill
	movl	%eax, %ecx
	movl	%esi, %eax
	mull	80(%esp)                        # 4-byte Folded Reload
	addl	60(%esp), %ebx                  # 4-byte Folded Reload
	adcl	4(%esp), %eax                   # 4-byte Folded Reload
	adcl	8(%esp), %ecx                   # 4-byte Folded Reload
	adcl	12(%esp), %edi                  # 4-byte Folded Reload
	adcl	64(%esp), %ebp                  # 4-byte Folded Reload
	movl	76(%esp), %esi                  # 4-byte Reload
	adcl	72(%esp), %esi                  # 4-byte Folded Reload
	movl	56(%esp), %ebx                  # 4-byte Reload
	adcl	$0, %ebx
	addl	16(%esp), %eax                  # 4-byte Folded Reload
	adcl	%edx, %ecx
	adcl	(%esp), %edi                    # 4-byte Folded Reload
	adcl	20(%esp), %ebp                  # 4-byte Folded Reload
	adcl	68(%esp), %esi                  # 4-byte Folded Reload
	adcl	92(%esp), %ebx                  # 4-byte Folded Reload
	movl	%eax, %edx
	subl	88(%esp), %edx                  # 4-byte Folded Reload
	movl	%edx, 88(%esp)                  # 4-byte Spill
	movl	%ecx, 60(%esp)                  # 4-byte Spill
	movl	%ecx, %edx
	sbbl	80(%esp), %edx                  # 4-byte Folded Reload
	movl	%edi, 80(%esp)                  # 4-byte Spill
	movl	%edi, %ecx
	sbbl	84(%esp), %ecx                  # 4-byte Folded Reload
	movl	%ebp, 84(%esp)                  # 4-byte Spill
	sbbl	96(%esp), %ebp                  # 4-byte Folded Reload
	movl	%esi, 76(%esp)                  # 4-byte Spill
	sbbl	100(%esp), %esi                 # 4-byte Folded Reload
	movl	%ebx, 56(%esp)                  # 4-byte Spill
	movl	%ebx, %edi
	sbbl	104(%esp), %edi                 # 4-byte Folded Reload
	movl	%edi, %ebx
	sarl	$31, %ebx
	testl	%ebx, %ebx
	js	.LBB10_1
# %bb.2:
	movl	148(%esp), %ebx
	movl	%edi, 20(%ebx)
	js	.LBB10_3
.LBB10_4:
	movl	%esi, 16(%ebx)
	js	.LBB10_5
.LBB10_6:
	movl	%ebp, 12(%ebx)
	js	.LBB10_7
.LBB10_8:
	movl	%ecx, 8(%ebx)
	js	.LBB10_9
.LBB10_10:
	movl	%edx, 4(%ebx)
	movl	88(%esp), %ecx                  # 4-byte Reload
	jns	.LBB10_12
.LBB10_11:
	movl	%eax, %ecx
.LBB10_12:
	movl	%ecx, (%ebx)
	addl	$128, %esp
	popl	%esi
	popl	%edi
	popl	%ebx
	popl	%ebp
	retl
.LBB10_1:
	movl	56(%esp), %edi                  # 4-byte Reload
	movl	148(%esp), %ebx
	movl	%edi, 20(%ebx)
	jns	.LBB10_4
.LBB10_3:
	movl	76(%esp), %esi                  # 4-byte Reload
	movl	%esi, 16(%ebx)
	jns	.LBB10_6
.LBB10_5:
	movl	84(%esp), %ebp                  # 4-byte Reload
	movl	%ebp, 12(%ebx)
	jns	.LBB10_8
.LBB10_7:
	movl	80(%esp), %ecx                  # 4-byte Reload
	movl	%ecx, 8(%ebx)
	jns	.LBB10_10
.LBB10_9:
	movl	60(%esp), %edx                  # 4-byte Reload
	movl	%edx, 4(%ebx)
	movl	88(%esp), %ecx                  # 4-byte Reload
	js	.LBB10_11
	jmp	.LBB10_12
.Lfunc_end10:
	.size	mcl_fp_montNF6L, .Lfunc_end10-mcl_fp_montNF6L
                                        # -- End function
	.globl	mcl_fp_montRed6L                # -- Begin function mcl_fp_montRed6L
	.p2align	4, 0x90
	.type	mcl_fp_montRed6L,@function
mcl_fp_montRed6L:                       # @mcl_fp_montRed6L
# %bb.0:
	pushl	%ebp
	pushl	%ebx
	pushl	%edi
	pushl	%esi
	subl	$84, %esp
	movl	112(%esp), %ecx
	movl	-4(%ecx), %eax
	movl	%eax, 52(%esp)                  # 4-byte Spill
	movl	108(%esp), %edx
	movl	(%edx), %edi
	movl	%edi, 32(%esp)                  # 4-byte Spill
	imull	%eax, %edi
	movl	20(%ecx), %edx
	movl	%edx, 72(%esp)                  # 4-byte Spill
	movl	%edi, %eax
	mull	%edx
	movl	%eax, 20(%esp)                  # 4-byte Spill
	movl	%edx, (%esp)                    # 4-byte Spill
	movl	16(%ecx), %edx
	movl	%edx, 76(%esp)                  # 4-byte Spill
	movl	%edi, %eax
	mull	%edx
	movl	%eax, 16(%esp)                  # 4-byte Spill
	movl	%edx, 28(%esp)                  # 4-byte Spill
	movl	12(%ecx), %edx
	movl	%edx, 68(%esp)                  # 4-byte Spill
	movl	%edi, %eax
	mull	%edx
	movl	%eax, 12(%esp)                  # 4-byte Spill
	movl	%edx, 24(%esp)                  # 4-byte Spill
	movl	8(%ecx), %edx
	movl	%edx, 64(%esp)                  # 4-byte Spill
	movl	%edi, %eax
	mull	%edx
	movl	%eax, 8(%esp)                   # 4-byte Spill
	movl	%edx, %ebp
	movl	(%ecx), %esi
	movl	%esi, 56(%esp)                  # 4-byte Spill
	movl	4(%ecx), %ecx
	movl	%ecx, 60(%esp)                  # 4-byte Spill
	movl	%edi, %eax
	mull	%ecx
	movl	%edx, %ebx
	movl	%eax, %ecx
	movl	%edi, %eax
	mull	%esi
	movl	%eax, %esi
	movl	%edx, %edi
	addl	%ecx, %edi
	adcl	8(%esp), %ebx                   # 4-byte Folded Reload
	movl	%ebp, %eax
	adcl	12(%esp), %eax                  # 4-byte Folded Reload
	movl	24(%esp), %ebp                  # 4-byte Reload
	adcl	16(%esp), %ebp                  # 4-byte Folded Reload
	movl	28(%esp), %edx                  # 4-byte Reload
	adcl	20(%esp), %edx                  # 4-byte Folded Reload
	movl	(%esp), %ecx                    # 4-byte Reload
	adcl	$0, %ecx
	addl	32(%esp), %esi                  # 4-byte Folded Reload
	movl	108(%esp), %esi
	adcl	4(%esi), %edi
	adcl	8(%esi), %ebx
	movl	%ebx, 8(%esp)                   # 4-byte Spill
	adcl	12(%esi), %eax
	movl	%eax, 12(%esp)                  # 4-byte Spill
	adcl	16(%esi), %ebp
	movl	%ebp, 24(%esp)                  # 4-byte Spill
	adcl	20(%esi), %edx
	movl	%edx, 28(%esp)                  # 4-byte Spill
	adcl	24(%esi), %ecx
	movl	%ecx, (%esp)                    # 4-byte Spill
	setb	%bl
	movl	52(%esp), %ecx                  # 4-byte Reload
	imull	%edi, %ecx
	movl	%ecx, %eax
	mull	72(%esp)                        # 4-byte Folded Reload
	movl	%edx, 48(%esp)                  # 4-byte Spill
	movl	%eax, 44(%esp)                  # 4-byte Spill
	movl	%ecx, %eax
	mull	76(%esp)                        # 4-byte Folded Reload
	movl	%edx, 32(%esp)                  # 4-byte Spill
	movl	%eax, 40(%esp)                  # 4-byte Spill
	movl	%ecx, %eax
	mull	68(%esp)                        # 4-byte Folded Reload
	movl	%edx, 20(%esp)                  # 4-byte Spill
	movl	%eax, 36(%esp)                  # 4-byte Spill
	movl	%ecx, %eax
	mull	64(%esp)                        # 4-byte Folded Reload
	movl	%edx, 16(%esp)                  # 4-byte Spill
	movl	%eax, %esi
	movl	%ecx, %eax
	mull	56(%esp)                        # 4-byte Folded Reload
	movl	%edx, 80(%esp)                  # 4-byte Spill
	movl	%eax, 4(%esp)                   # 4-byte Spill
	movl	%ecx, %eax
	mull	60(%esp)                        # 4-byte Folded Reload
	movl	%eax, %ebp
	addl	80(%esp), %ebp                  # 4-byte Folded Reload
	adcl	%esi, %edx
	movl	%edx, %esi
	movl	16(%esp), %edx                  # 4-byte Reload
	adcl	36(%esp), %edx                  # 4-byte Folded Reload
	movl	20(%esp), %ecx                  # 4-byte Reload
	adcl	40(%esp), %ecx                  # 4-byte Folded Reload
	movl	32(%esp), %eax                  # 4-byte Reload
	adcl	44(%esp), %eax                  # 4-byte Folded Reload
	movzbl	%bl, %ebx
	adcl	48(%esp), %ebx                  # 4-byte Folded Reload
	addl	%edi, 4(%esp)                   # 4-byte Folded Spill
	adcl	8(%esp), %ebp                   # 4-byte Folded Reload
	adcl	12(%esp), %esi                  # 4-byte Folded Reload
	movl	%esi, 8(%esp)                   # 4-byte Spill
	adcl	24(%esp), %edx                  # 4-byte Folded Reload
	movl	%edx, 16(%esp)                  # 4-byte Spill
	adcl	28(%esp), %ecx                  # 4-byte Folded Reload
	movl	%ecx, 20(%esp)                  # 4-byte Spill
	adcl	(%esp), %eax                    # 4-byte Folded Reload
	movl	%eax, 32(%esp)                  # 4-byte Spill
	movl	108(%esp), %eax
	adcl	28(%eax), %ebx
	movl	%ebx, (%esp)                    # 4-byte Spill
	setb	48(%esp)                        # 1-byte Folded Spill
	movl	52(%esp), %esi                  # 4-byte Reload
	imull	%ebp, %esi
	movl	%esi, %eax
	mull	72(%esp)                        # 4-byte Folded Reload
	movl	%edx, 24(%esp)                  # 4-byte Spill
	movl	%eax, 44(%esp)                  # 4-byte Spill
	movl	%esi, %eax
	mull	76(%esp)                        # 4-byte Folded Reload
	movl	%edx, 28(%esp)                  # 4-byte Spill
	movl	%eax, %edi
	movl	%esi, %eax
	mull	68(%esp)                        # 4-byte Folded Reload
	movl	%edx, %ecx
	movl	%eax, 36(%esp)                  # 4-byte Spill
	movl	%esi, %eax
	mull	64(%esp)                        # 4-byte Folded Reload
	movl	%edx, 12(%esp)                  # 4-byte Spill
	movl	%eax, %ebx
	movl	%esi, %eax
	mull	56(%esp)                        # 4-byte Folded Reload
	movl	%edx, 4(%esp)                   # 4-byte Spill
	movl	%eax, 40(%esp)                  # 4-byte Spill
	movl	%esi, %eax
	mull	60(%esp)                        # 4-byte Folded Reload
	addl	4(%esp), %eax                   # 4-byte Folded Reload
	movl	%eax, 4(%esp)                   # 4-byte Spill
	adcl	%ebx, %edx
	movl	%edx, %ebx
	movl	12(%esp), %esi                  # 4-byte Reload
	adcl	36(%esp), %esi                  # 4-byte Folded Reload
	adcl	%edi, %ecx
	movl	%ecx, %edi
	movl	28(%esp), %ecx                  # 4-byte Reload
	adcl	44(%esp), %ecx                  # 4-byte Folded Reload
	movzbl	48(%esp), %eax                  # 1-byte Folded Reload
	movl	24(%esp), %edx                  # 4-byte Reload
	adcl	%eax, %edx
	addl	%ebp, 40(%esp)                  # 4-byte Folded Spill
	movl	4(%esp), %ebp                   # 4-byte Reload
	adcl	8(%esp), %ebp                   # 4-byte Folded Reload
	adcl	16(%esp), %ebx                  # 4-byte Folded Reload
	movl	%ebx, 16(%esp)                  # 4-byte Spill
	adcl	20(%esp), %esi                  # 4-byte Folded Reload
	movl	%esi, 12(%esp)                  # 4-byte Spill
	adcl	32(%esp), %edi                  # 4-byte Folded Reload
	movl	%edi, 32(%esp)                  # 4-byte Spill
	adcl	(%esp), %ecx                    # 4-byte Folded Reload
	movl	%ecx, 28(%esp)                  # 4-byte Spill
	movl	108(%esp), %eax
	adcl	32(%eax), %edx
	movl	%edx, 24(%esp)                  # 4-byte Spill
	setb	8(%esp)                         # 1-byte Folded Spill
	movl	52(%esp), %edi                  # 4-byte Reload
	imull	%ebp, %edi
	movl	%ebp, %ebx
	movl	%edi, %eax
	mull	72(%esp)                        # 4-byte Folded Reload
	movl	%edx, 48(%esp)                  # 4-byte Spill
	movl	%eax, %esi
	movl	%edi, %eax
	mull	76(%esp)                        # 4-byte Folded Reload
	movl	%edx, (%esp)                    # 4-byte Spill
	movl	%eax, 44(%esp)                  # 4-byte Spill
	movl	%edi, %eax
	mull	68(%esp)                        # 4-byte Folded Reload
	movl	%edx, 20(%esp)                  # 4-byte Spill
	movl	%eax, 36(%esp)                  # 4-byte Spill
	movl	%edi, %eax
	mull	64(%esp)                        # 4-byte Folded Reload
	movl	%edx, %ebp
	movl	%eax, %ecx
	movl	%edi, %eax
	mull	56(%esp)                        # 4-byte Folded Reload
	movl	%edx, 4(%esp)                   # 4-byte Spill
	movl	%eax, 40(%esp)                  # 4-byte Spill
	movl	%edi, %eax
	mull	60(%esp)                        # 4-byte Folded Reload
	addl	4(%esp), %eax                   # 4-byte Folded Reload
	movl	%eax, %edi
	adcl	%ecx, %edx
	movl	%edx, %ecx
	adcl	36(%esp), %ebp                  # 4-byte Folded Reload
	movl	20(%esp), %edx                  # 4-byte Reload
	adcl	44(%esp), %edx                  # 4-byte Folded Reload
	movl	(%esp), %eax                    # 4-byte Reload
	adcl	%esi, %eax
	movzbl	8(%esp), %esi                   # 1-byte Folded Reload
	adcl	48(%esp), %esi                  # 4-byte Folded Reload
	addl	%ebx, 40(%esp)                  # 4-byte Folded Spill
	adcl	16(%esp), %edi                  # 4-byte Folded Reload
	movl	%edi, 44(%esp)                  # 4-byte Spill
	adcl	12(%esp), %ecx                  # 4-byte Folded Reload
	movl	%ecx, 12(%esp)                  # 4-byte Spill
	adcl	32(%esp), %ebp                  # 4-byte Folded Reload
	movl	%ebp, 16(%esp)                  # 4-byte Spill
	adcl	28(%esp), %edx                  # 4-byte Folded Reload
	movl	%edx, 20(%esp)                  # 4-byte Spill
	adcl	24(%esp), %eax                  # 4-byte Folded Reload
	movl	%eax, (%esp)                    # 4-byte Spill
	movl	108(%esp), %eax
	adcl	36(%eax), %esi
	movl	%esi, 8(%esp)                   # 4-byte Spill
	setb	48(%esp)                        # 1-byte Folded Spill
	movl	52(%esp), %ecx                  # 4-byte Reload
	imull	%edi, %ecx
	movl	%ecx, %eax
	mull	72(%esp)                        # 4-byte Folded Reload
	movl	%edx, 28(%esp)                  # 4-byte Spill
	movl	%eax, 40(%esp)                  # 4-byte Spill
	movl	%ecx, %eax
	mull	76(%esp)                        # 4-byte Folded Reload
	movl	%edx, 32(%esp)                  # 4-byte Spill
	movl	%eax, 36(%esp)                  # 4-byte Spill
	movl	%ecx, %eax
	mull	68(%esp)                        # 4-byte Folded Reload
	movl	%edx, 24(%esp)                  # 4-byte Spill
	movl	%eax, %edi
	movl	%ecx, %eax
	mull	64(%esp)                        # 4-byte Folded Reload
	movl	%edx, %esi
	movl	%eax, %ebx
	movl	%ecx, %eax
	mull	56(%esp)                        # 4-byte Folded Reload
	movl	%edx, %ebp
	movl	%eax, 4(%esp)                   # 4-byte Spill
	movl	%ecx, %eax
	mull	60(%esp)                        # 4-byte Folded Reload
	addl	%ebp, %eax
	movl	%eax, %ebp
	adcl	%ebx, %edx
	movl	%edx, %ebx
	adcl	%edi, %esi
	movl	%esi, %edi
	movl	24(%esp), %esi                  # 4-byte Reload
	adcl	36(%esp), %esi                  # 4-byte Folded Reload
	movl	32(%esp), %ecx                  # 4-byte Reload
	adcl	40(%esp), %ecx                  # 4-byte Folded Reload
	movzbl	48(%esp), %eax                  # 1-byte Folded Reload
	movl	28(%esp), %edx                  # 4-byte Reload
	adcl	%eax, %edx
	movl	4(%esp), %eax                   # 4-byte Reload
	addl	44(%esp), %eax                  # 4-byte Folded Reload
	adcl	12(%esp), %ebp                  # 4-byte Folded Reload
	movl	%ebp, 12(%esp)                  # 4-byte Spill
	adcl	16(%esp), %ebx                  # 4-byte Folded Reload
	movl	%ebx, 16(%esp)                  # 4-byte Spill
	adcl	20(%esp), %edi                  # 4-byte Folded Reload
	movl	%edi, 20(%esp)                  # 4-byte Spill
	adcl	(%esp), %esi                    # 4-byte Folded Reload
	movl	%esi, 24(%esp)                  # 4-byte Spill
	adcl	8(%esp), %ecx                   # 4-byte Folded Reload
	movl	%ecx, 32(%esp)                  # 4-byte Spill
	movl	108(%esp), %eax
	adcl	40(%eax), %edx
	movl	%edx, 28(%esp)                  # 4-byte Spill
	setb	(%esp)                          # 1-byte Folded Spill
	movl	52(%esp), %edi                  # 4-byte Reload
	imull	%ebp, %edi
	movl	%edi, %eax
	mull	72(%esp)                        # 4-byte Folded Reload
	movl	%edx, 52(%esp)                  # 4-byte Spill
	movl	%eax, 8(%esp)                   # 4-byte Spill
	movl	%edi, %eax
	mull	76(%esp)                        # 4-byte Folded Reload
	movl	%edx, %esi
	movl	%eax, 44(%esp)                  # 4-byte Spill
	movl	%edi, %eax
	mull	56(%esp)                        # 4-byte Folded Reload
	movl	%edx, 40(%esp)                  # 4-byte Spill
	movl	%eax, 48(%esp)                  # 4-byte Spill
	movl	%edi, %eax
	mull	68(%esp)                        # 4-byte Folded Reload
	movl	%edx, %ebx
	movl	%eax, 36(%esp)                  # 4-byte Spill
	movl	%edi, %eax
	mull	64(%esp)                        # 4-byte Folded Reload
	movl	%edx, %ebp
	movl	%eax, 4(%esp)                   # 4-byte Spill
	movl	%edi, %eax
	mull	60(%esp)                        # 4-byte Folded Reload
	movl	%eax, %ecx
	addl	40(%esp), %ecx                  # 4-byte Folded Reload
	adcl	4(%esp), %edx                   # 4-byte Folded Reload
	adcl	36(%esp), %ebp                  # 4-byte Folded Reload
	adcl	44(%esp), %ebx                  # 4-byte Folded Reload
	adcl	8(%esp), %esi                   # 4-byte Folded Reload
	movzbl	(%esp), %edi                    # 1-byte Folded Reload
	adcl	52(%esp), %edi                  # 4-byte Folded Reload
	movl	48(%esp), %eax                  # 4-byte Reload
	addl	12(%esp), %eax                  # 4-byte Folded Reload
	adcl	16(%esp), %ecx                  # 4-byte Folded Reload
	adcl	20(%esp), %edx                  # 4-byte Folded Reload
	adcl	24(%esp), %ebp                  # 4-byte Folded Reload
	adcl	32(%esp), %ebx                  # 4-byte Folded Reload
	adcl	28(%esp), %esi                  # 4-byte Folded Reload
	movl	108(%esp), %eax
	adcl	44(%eax), %edi
	xorl	%eax, %eax
	movl	%ecx, 52(%esp)                  # 4-byte Spill
	subl	56(%esp), %ecx                  # 4-byte Folded Reload
	movl	%ecx, 56(%esp)                  # 4-byte Spill
	movl	%edx, %ecx
	sbbl	60(%esp), %ecx                  # 4-byte Folded Reload
	movl	%ecx, 60(%esp)                  # 4-byte Spill
	movl	%ebp, (%esp)                    # 4-byte Spill
	sbbl	64(%esp), %ebp                  # 4-byte Folded Reload
	movl	%ebx, 64(%esp)                  # 4-byte Spill
	sbbl	68(%esp), %ebx                  # 4-byte Folded Reload
	movl	%esi, 68(%esp)                  # 4-byte Spill
	movl	%esi, %ecx
	sbbl	76(%esp), %ecx                  # 4-byte Folded Reload
	movl	%edi, %esi
	sbbl	72(%esp), %esi                  # 4-byte Folded Reload
	sbbl	%eax, %eax
	testb	$1, %al
	jne	.LBB11_1
# %bb.2:
	movl	104(%esp), %eax
	movl	%esi, 20(%eax)
	jne	.LBB11_3
.LBB11_4:
	movl	%ecx, 16(%eax)
	movl	60(%esp), %esi                  # 4-byte Reload
	jne	.LBB11_5
.LBB11_6:
	movl	%ebx, 12(%eax)
	movl	56(%esp), %ecx                  # 4-byte Reload
	jne	.LBB11_7
.LBB11_8:
	movl	%ebp, 8(%eax)
	jne	.LBB11_9
.LBB11_10:
	movl	%esi, 4(%eax)
	je	.LBB11_12
.LBB11_11:
	movl	52(%esp), %ecx                  # 4-byte Reload
.LBB11_12:
	movl	%ecx, (%eax)
	addl	$84, %esp
	popl	%esi
	popl	%edi
	popl	%ebx
	popl	%ebp
	retl
.LBB11_1:
	movl	%edi, %esi
	movl	104(%esp), %eax
	movl	%esi, 20(%eax)
	je	.LBB11_4
.LBB11_3:
	movl	68(%esp), %ecx                  # 4-byte Reload
	movl	%ecx, 16(%eax)
	movl	60(%esp), %esi                  # 4-byte Reload
	je	.LBB11_6
.LBB11_5:
	movl	64(%esp), %ebx                  # 4-byte Reload
	movl	%ebx, 12(%eax)
	movl	56(%esp), %ecx                  # 4-byte Reload
	je	.LBB11_8
.LBB11_7:
	movl	(%esp), %ebp                    # 4-byte Reload
	movl	%ebp, 8(%eax)
	je	.LBB11_10
.LBB11_9:
	movl	%edx, %esi
	movl	%esi, 4(%eax)
	jne	.LBB11_11
	jmp	.LBB11_12
.Lfunc_end11:
	.size	mcl_fp_montRed6L, .Lfunc_end11-mcl_fp_montRed6L
                                        # -- End function
	.globl	mcl_fp_montRedNF6L              # -- Begin function mcl_fp_montRedNF6L
	.p2align	4, 0x90
	.type	mcl_fp_montRedNF6L,@function
mcl_fp_montRedNF6L:                     # @mcl_fp_montRedNF6L
# %bb.0:
	pushl	%ebp
	pushl	%ebx
	pushl	%edi
	pushl	%esi
	subl	$84, %esp
	movl	112(%esp), %ecx
	movl	-4(%ecx), %eax
	movl	%eax, 48(%esp)                  # 4-byte Spill
	movl	108(%esp), %edx
	movl	(%edx), %edi
	movl	%edi, 28(%esp)                  # 4-byte Spill
	imull	%eax, %edi
	movl	20(%ecx), %edx
	movl	%edx, 72(%esp)                  # 4-byte Spill
	movl	%edi, %eax
	mull	%edx
	movl	%eax, 16(%esp)                  # 4-byte Spill
	movl	%edx, 8(%esp)                   # 4-byte Spill
	movl	16(%ecx), %edx
	movl	%edx, 76(%esp)                  # 4-byte Spill
	movl	%edi, %eax
	mull	%edx
	movl	%eax, 12(%esp)                  # 4-byte Spill
	movl	%edx, 24(%esp)                  # 4-byte Spill
	movl	12(%ecx), %edx
	movl	%edx, 64(%esp)                  # 4-byte Spill
	movl	%edi, %eax
	mull	%edx
	movl	%eax, 4(%esp)                   # 4-byte Spill
	movl	%edx, %ebx
	movl	8(%ecx), %edx
	movl	%edx, 60(%esp)                  # 4-byte Spill
	movl	%edi, %eax
	mull	%edx
	movl	%eax, (%esp)                    # 4-byte Spill
	movl	%edx, 20(%esp)                  # 4-byte Spill
	movl	(%ecx), %esi
	movl	%esi, 52(%esp)                  # 4-byte Spill
	movl	4(%ecx), %ecx
	movl	%ecx, 56(%esp)                  # 4-byte Spill
	movl	%edi, %eax
	mull	%ecx
	movl	%edx, %ebp
	movl	%eax, %ecx
	movl	%edi, %eax
	mull	%esi
	movl	%eax, %esi
	movl	%edx, %edi
	addl	%ecx, %edi
	adcl	(%esp), %ebp                    # 4-byte Folded Reload
	movl	20(%esp), %eax                  # 4-byte Reload
	adcl	4(%esp), %eax                   # 4-byte Folded Reload
	adcl	12(%esp), %ebx                  # 4-byte Folded Reload
	movl	24(%esp), %edx                  # 4-byte Reload
	adcl	16(%esp), %edx                  # 4-byte Folded Reload
	movl	8(%esp), %ecx                   # 4-byte Reload
	adcl	$0, %ecx
	addl	28(%esp), %esi                  # 4-byte Folded Reload
	movl	108(%esp), %esi
	adcl	4(%esi), %edi
	adcl	8(%esi), %ebp
	movl	%ebp, 4(%esp)                   # 4-byte Spill
	adcl	12(%esi), %eax
	movl	%eax, 20(%esp)                  # 4-byte Spill
	adcl	16(%esi), %ebx
	movl	%ebx, 12(%esp)                  # 4-byte Spill
	adcl	20(%esi), %edx
	movl	%edx, 24(%esp)                  # 4-byte Spill
	adcl	24(%esi), %ecx
	movl	%ecx, 8(%esp)                   # 4-byte Spill
	setb	(%esp)                          # 1-byte Folded Spill
	movl	48(%esp), %ecx                  # 4-byte Reload
	imull	%edi, %ecx
	movl	%ecx, %eax
	mull	72(%esp)                        # 4-byte Folded Reload
	movl	%edx, 44(%esp)                  # 4-byte Spill
	movl	%eax, 40(%esp)                  # 4-byte Spill
	movl	%ecx, %eax
	mull	76(%esp)                        # 4-byte Folded Reload
	movl	%edx, 28(%esp)                  # 4-byte Spill
	movl	%eax, 36(%esp)                  # 4-byte Spill
	movl	%ecx, %eax
	mull	64(%esp)                        # 4-byte Folded Reload
	movl	%edx, 16(%esp)                  # 4-byte Spill
	movl	%eax, 32(%esp)                  # 4-byte Spill
	movl	%ecx, %eax
	mull	60(%esp)                        # 4-byte Folded Reload
	movl	%edx, %ebp
	movl	%eax, %esi
	movl	%ecx, %eax
	mull	52(%esp)                        # 4-byte Folded Reload
	movl	%edx, %ebx
	movl	%eax, 68(%esp)                  # 4-byte Spill
	movl	%ecx, %eax
	mull	56(%esp)                        # 4-byte Folded Reload
	addl	%ebx, %eax
	movl	%eax, %ebx
	adcl	%esi, %edx
	movl	%edx, %esi
	movl	%ebp, %ecx
	adcl	32(%esp), %ecx                  # 4-byte Folded Reload
	movl	16(%esp), %ebp                  # 4-byte Reload
	adcl	36(%esp), %ebp                  # 4-byte Folded Reload
	movl	28(%esp), %eax                  # 4-byte Reload
	adcl	40(%esp), %eax                  # 4-byte Folded Reload
	movzbl	(%esp), %edx                    # 1-byte Folded Reload
	adcl	44(%esp), %edx                  # 4-byte Folded Reload
	addl	%edi, 68(%esp)                  # 4-byte Folded Spill
	adcl	4(%esp), %ebx                   # 4-byte Folded Reload
	movl	%ebx, 40(%esp)                  # 4-byte Spill
	adcl	20(%esp), %esi                  # 4-byte Folded Reload
	movl	%esi, (%esp)                    # 4-byte Spill
	adcl	12(%esp), %ecx                  # 4-byte Folded Reload
	movl	%ecx, 12(%esp)                  # 4-byte Spill
	adcl	24(%esp), %ebp                  # 4-byte Folded Reload
	movl	%ebp, 16(%esp)                  # 4-byte Spill
	adcl	8(%esp), %eax                   # 4-byte Folded Reload
	movl	%eax, 28(%esp)                  # 4-byte Spill
	movl	108(%esp), %eax
	adcl	28(%eax), %edx
	movl	%edx, 8(%esp)                   # 4-byte Spill
	setb	44(%esp)                        # 1-byte Folded Spill
	movl	48(%esp), %esi                  # 4-byte Reload
	imull	%ebx, %esi
	movl	%esi, %eax
	mull	72(%esp)                        # 4-byte Folded Reload
	movl	%edx, 20(%esp)                  # 4-byte Spill
	movl	%eax, 36(%esp)                  # 4-byte Spill
	movl	%esi, %eax
	mull	76(%esp)                        # 4-byte Folded Reload
	movl	%edx, 24(%esp)                  # 4-byte Spill
	movl	%eax, %edi
	movl	%esi, %eax
	mull	64(%esp)                        # 4-byte Folded Reload
	movl	%edx, %ecx
	movl	%eax, 68(%esp)                  # 4-byte Spill
	movl	%esi, %eax
	mull	60(%esp)                        # 4-byte Folded Reload
	movl	%edx, 4(%esp)                   # 4-byte Spill
	movl	%eax, %ebx
	movl	%esi, %eax
	mull	52(%esp)                        # 4-byte Folded Reload
	movl	%edx, 80(%esp)                  # 4-byte Spill
	movl	%eax, 32(%esp)                  # 4-byte Spill
	movl	%esi, %eax
	mull	56(%esp)                        # 4-byte Folded Reload
	movl	%eax, %ebp
	addl	80(%esp), %ebp                  # 4-byte Folded Reload
	adcl	%ebx, %edx
	movl	%edx, %ebx
	movl	4(%esp), %esi                   # 4-byte Reload
	adcl	68(%esp), %esi                  # 4-byte Folded Reload
	adcl	%edi, %ecx
	movl	%ecx, %edi
	movl	24(%esp), %ecx                  # 4-byte Reload
	adcl	36(%esp), %ecx                  # 4-byte Folded Reload
	movzbl	44(%esp), %eax                  # 1-byte Folded Reload
	movl	20(%esp), %edx                  # 4-byte Reload
	adcl	%eax, %edx
	movl	32(%esp), %eax                  # 4-byte Reload
	addl	40(%esp), %eax                  # 4-byte Folded Reload
	adcl	(%esp), %ebp                    # 4-byte Folded Reload
	adcl	12(%esp), %ebx                  # 4-byte Folded Reload
	movl	%ebx, 44(%esp)                  # 4-byte Spill
	adcl	16(%esp), %esi                  # 4-byte Folded Reload
	movl	%esi, 4(%esp)                   # 4-byte Spill
	adcl	28(%esp), %edi                  # 4-byte Folded Reload
	movl	%edi, 28(%esp)                  # 4-byte Spill
	adcl	8(%esp), %ecx                   # 4-byte Folded Reload
	movl	%ecx, 24(%esp)                  # 4-byte Spill
	movl	108(%esp), %eax
	adcl	32(%eax), %edx
	movl	%edx, 20(%esp)                  # 4-byte Spill
	setb	40(%esp)                        # 1-byte Folded Spill
	movl	48(%esp), %ebx                  # 4-byte Reload
	imull	%ebp, %ebx
	movl	%ebx, %eax
	mull	72(%esp)                        # 4-byte Folded Reload
	movl	%edx, 36(%esp)                  # 4-byte Spill
	movl	%eax, %esi
	movl	%ebx, %eax
	mull	76(%esp)                        # 4-byte Folded Reload
	movl	%edx, 8(%esp)                   # 4-byte Spill
	movl	%eax, 32(%esp)                  # 4-byte Spill
	movl	%ebx, %eax
	mull	64(%esp)                        # 4-byte Folded Reload
	movl	%edx, 16(%esp)                  # 4-byte Spill
	movl	%eax, 68(%esp)                  # 4-byte Spill
	movl	%ebx, %eax
	mull	60(%esp)                        # 4-byte Folded Reload
	movl	%edx, 12(%esp)                  # 4-byte Spill
	movl	%eax, %ecx
	movl	%ebx, %eax
	mull	52(%esp)                        # 4-byte Folded Reload
	movl	%edx, (%esp)                    # 4-byte Spill
	movl	%eax, %edi
	movl	%ebx, %eax
	mull	56(%esp)                        # 4-byte Folded Reload
	addl	(%esp), %eax                    # 4-byte Folded Reload
	movl	%eax, (%esp)                    # 4-byte Spill
	adcl	%ecx, %edx
	movl	%edx, %ecx
	movl	12(%esp), %ebx                  # 4-byte Reload
	adcl	68(%esp), %ebx                  # 4-byte Folded Reload
	movl	16(%esp), %edx                  # 4-byte Reload
	adcl	32(%esp), %edx                  # 4-byte Folded Reload
	movl	8(%esp), %eax                   # 4-byte Reload
	adcl	%esi, %eax
	movzbl	40(%esp), %esi                  # 1-byte Folded Reload
	adcl	36(%esp), %esi                  # 4-byte Folded Reload
	addl	%ebp, %edi
	movl	(%esp), %edi                    # 4-byte Reload
	adcl	44(%esp), %edi                  # 4-byte Folded Reload
	movl	%edi, (%esp)                    # 4-byte Spill
	adcl	4(%esp), %ecx                   # 4-byte Folded Reload
	movl	%ecx, 44(%esp)                  # 4-byte Spill
	adcl	28(%esp), %ebx                  # 4-byte Folded Reload
	movl	%ebx, 12(%esp)                  # 4-byte Spill
	adcl	24(%esp), %edx                  # 4-byte Folded Reload
	movl	%edx, 16(%esp)                  # 4-byte Spill
	adcl	20(%esp), %eax                  # 4-byte Folded Reload
	movl	%eax, 8(%esp)                   # 4-byte Spill
	movl	108(%esp), %eax
	adcl	36(%eax), %esi
	movl	%esi, 40(%esp)                  # 4-byte Spill
	setb	36(%esp)                        # 1-byte Folded Spill
	movl	48(%esp), %ecx                  # 4-byte Reload
	imull	%edi, %ecx
	movl	%ecx, %eax
	mull	72(%esp)                        # 4-byte Folded Reload
	movl	%edx, 24(%esp)                  # 4-byte Spill
	movl	%eax, 32(%esp)                  # 4-byte Spill
	movl	%ecx, %eax
	mull	76(%esp)                        # 4-byte Folded Reload
	movl	%edx, 28(%esp)                  # 4-byte Spill
	movl	%eax, 68(%esp)                  # 4-byte Spill
	movl	%ecx, %eax
	mull	64(%esp)                        # 4-byte Folded Reload
	movl	%edx, 20(%esp)                  # 4-byte Spill
	movl	%eax, %edi
	movl	%ecx, %eax
	mull	60(%esp)                        # 4-byte Folded Reload
	movl	%edx, %esi
	movl	%eax, %ebp
	movl	%ecx, %eax
	mull	52(%esp)                        # 4-byte Folded Reload
	movl	%edx, 4(%esp)                   # 4-byte Spill
	movl	%eax, %ebx
	movl	%ecx, %eax
	mull	56(%esp)                        # 4-byte Folded Reload
	addl	4(%esp), %eax                   # 4-byte Folded Reload
	movl	%eax, 4(%esp)                   # 4-byte Spill
	adcl	%ebp, %edx
	movl	%edx, %ebp
	adcl	%edi, %esi
	movl	%esi, %edi
	movl	20(%esp), %esi                  # 4-byte Reload
	adcl	68(%esp), %esi                  # 4-byte Folded Reload
	movl	28(%esp), %ecx                  # 4-byte Reload
	adcl	32(%esp), %ecx                  # 4-byte Folded Reload
	movzbl	36(%esp), %eax                  # 1-byte Folded Reload
	movl	24(%esp), %edx                  # 4-byte Reload
	adcl	%eax, %edx
	addl	(%esp), %ebx                    # 4-byte Folded Reload
	movl	4(%esp), %ebx                   # 4-byte Reload
	adcl	44(%esp), %ebx                  # 4-byte Folded Reload
	movl	%ebx, 4(%esp)                   # 4-byte Spill
	adcl	12(%esp), %ebp                  # 4-byte Folded Reload
	movl	%ebp, 12(%esp)                  # 4-byte Spill
	adcl	16(%esp), %edi                  # 4-byte Folded Reload
	movl	%edi, 16(%esp)                  # 4-byte Spill
	adcl	8(%esp), %esi                   # 4-byte Folded Reload
	movl	%esi, 20(%esp)                  # 4-byte Spill
	adcl	40(%esp), %ecx                  # 4-byte Folded Reload
	movl	%ecx, 28(%esp)                  # 4-byte Spill
	movl	108(%esp), %eax
	adcl	40(%eax), %edx
	movl	%edx, 24(%esp)                  # 4-byte Spill
	setb	8(%esp)                         # 1-byte Folded Spill
	movl	48(%esp), %ecx                  # 4-byte Reload
	imull	%ebx, %ecx
	movl	%ecx, %eax
	mull	72(%esp)                        # 4-byte Folded Reload
	movl	%edx, 48(%esp)                  # 4-byte Spill
	movl	%eax, %ebp
	movl	%ecx, %eax
	mull	76(%esp)                        # 4-byte Folded Reload
	movl	%edx, %edi
	movl	%eax, 44(%esp)                  # 4-byte Spill
	movl	%ecx, %eax
	mull	52(%esp)                        # 4-byte Folded Reload
	movl	%edx, 40(%esp)                  # 4-byte Spill
	movl	%eax, (%esp)                    # 4-byte Spill
	movl	%ecx, %eax
	mull	64(%esp)                        # 4-byte Folded Reload
	movl	%edx, %ebx
	movl	%eax, 36(%esp)                  # 4-byte Spill
	movl	%ecx, %eax
	mull	60(%esp)                        # 4-byte Folded Reload
	movl	%edx, %esi
	movl	%eax, 32(%esp)                  # 4-byte Spill
	movl	%ecx, %eax
	mull	56(%esp)                        # 4-byte Folded Reload
	movl	%eax, %ecx
	addl	40(%esp), %ecx                  # 4-byte Folded Reload
	adcl	32(%esp), %edx                  # 4-byte Folded Reload
	adcl	36(%esp), %esi                  # 4-byte Folded Reload
	adcl	44(%esp), %ebx                  # 4-byte Folded Reload
	adcl	%ebp, %edi
	movzbl	8(%esp), %ebp                   # 1-byte Folded Reload
	adcl	48(%esp), %ebp                  # 4-byte Folded Reload
	movl	(%esp), %eax                    # 4-byte Reload
	addl	4(%esp), %eax                   # 4-byte Folded Reload
	adcl	12(%esp), %ecx                  # 4-byte Folded Reload
	adcl	16(%esp), %edx                  # 4-byte Folded Reload
	adcl	20(%esp), %esi                  # 4-byte Folded Reload
	adcl	28(%esp), %ebx                  # 4-byte Folded Reload
	adcl	24(%esp), %edi                  # 4-byte Folded Reload
	movl	108(%esp), %eax
	adcl	44(%eax), %ebp
	movl	%ecx, 48(%esp)                  # 4-byte Spill
	subl	52(%esp), %ecx                  # 4-byte Folded Reload
	movl	%ecx, 52(%esp)                  # 4-byte Spill
	movl	%edx, %eax
	sbbl	56(%esp), %eax                  # 4-byte Folded Reload
	movl	%eax, 56(%esp)                  # 4-byte Spill
	movl	%esi, 8(%esp)                   # 4-byte Spill
	sbbl	60(%esp), %esi                  # 4-byte Folded Reload
	movl	%ebx, 60(%esp)                  # 4-byte Spill
	sbbl	64(%esp), %ebx                  # 4-byte Folded Reload
	movl	%edi, 64(%esp)                  # 4-byte Spill
	movl	%edi, %ecx
	sbbl	76(%esp), %ecx                  # 4-byte Folded Reload
	movl	%ebp, %eax
	sbbl	72(%esp), %eax                  # 4-byte Folded Reload
	movl	%eax, %edi
	sarl	$31, %edi
	testl	%edi, %edi
	js	.LBB12_1
# %bb.2:
	movl	104(%esp), %edi
	movl	%eax, 20(%edi)
	js	.LBB12_3
.LBB12_4:
	movl	%ecx, 16(%edi)
	movl	52(%esp), %eax                  # 4-byte Reload
	js	.LBB12_5
.LBB12_6:
	movl	%ebx, 12(%edi)
	movl	56(%esp), %ecx                  # 4-byte Reload
	js	.LBB12_7
.LBB12_8:
	movl	%esi, 8(%edi)
	js	.LBB12_9
.LBB12_10:
	movl	%ecx, 4(%edi)
	jns	.LBB12_12
.LBB12_11:
	movl	48(%esp), %eax                  # 4-byte Reload
.LBB12_12:
	movl	%eax, (%edi)
	addl	$84, %esp
	popl	%esi
	popl	%edi
	popl	%ebx
	popl	%ebp
	retl
.LBB12_1:
	movl	%ebp, %eax
	movl	104(%esp), %edi
	movl	%eax, 20(%edi)
	jns	.LBB12_4
.LBB12_3:
	movl	64(%esp), %ecx                  # 4-byte Reload
	movl	%ecx, 16(%edi)
	movl	52(%esp), %eax                  # 4-byte Reload
	jns	.LBB12_6
.LBB12_5:
	movl	60(%esp), %ebx                  # 4-byte Reload
	movl	%ebx, 12(%edi)
	movl	56(%esp), %ecx                  # 4-byte Reload
	jns	.LBB12_8
.LBB12_7:
	movl	8(%esp), %esi                   # 4-byte Reload
	movl	%esi, 8(%edi)
	jns	.LBB12_10
.LBB12_9:
	movl	%edx, %ecx
	movl	%ecx, 4(%edi)
	js	.LBB12_11
	jmp	.LBB12_12
.Lfunc_end12:
	.size	mcl_fp_montRedNF6L, .Lfunc_end12-mcl_fp_montRedNF6L
                                        # -- End function
	.globl	mcl_fp_addPre6L                 # -- Begin function mcl_fp_addPre6L
	.p2align	4, 0x90
	.type	mcl_fp_addPre6L,@function
mcl_fp_addPre6L:                        # @mcl_fp_addPre6L
# %bb.0:
	pushl	%ebp
	pushl	%ebx
	pushl	%edi
	pushl	%esi
	movl	24(%esp), %eax
	movl	(%eax), %ecx
	movl	4(%eax), %edx
	movl	28(%esp), %esi
	addl	(%esi), %ecx
	adcl	4(%esi), %edx
	movl	20(%eax), %edi
	movl	16(%eax), %ebx
	movl	12(%eax), %ebp
	movl	8(%eax), %eax
	adcl	8(%esi), %eax
	adcl	12(%esi), %ebp
	adcl	16(%esi), %ebx
	adcl	20(%esi), %edi
	movl	20(%esp), %esi
	movl	%ebx, 16(%esi)
	movl	%ebp, 12(%esi)
	movl	%eax, 8(%esi)
	movl	%edi, 20(%esi)
	movl	%edx, 4(%esi)
	movl	%ecx, (%esi)
	setb	%al
	movzbl	%al, %eax
	popl	%esi
	popl	%edi
	popl	%ebx
	popl	%ebp
	retl
.Lfunc_end13:
	.size	mcl_fp_addPre6L, .Lfunc_end13-mcl_fp_addPre6L
                                        # -- End function
	.globl	mcl_fp_subPre6L                 # -- Begin function mcl_fp_subPre6L
	.p2align	4, 0x90
	.type	mcl_fp_subPre6L,@function
mcl_fp_subPre6L:                        # @mcl_fp_subPre6L
# %bb.0:
	pushl	%ebp
	pushl	%ebx
	pushl	%edi
	pushl	%esi
	pushl	%eax
	movl	28(%esp), %edx
	movl	(%edx), %ecx
	movl	4(%edx), %esi
	xorl	%eax, %eax
	movl	32(%esp), %edi
	subl	(%edi), %ecx
	movl	%ecx, (%esp)                    # 4-byte Spill
	sbbl	4(%edi), %esi
	movl	20(%edx), %ebx
	movl	16(%edx), %ebp
	movl	12(%edx), %ecx
	movl	8(%edx), %edx
	sbbl	8(%edi), %edx
	sbbl	12(%edi), %ecx
	sbbl	16(%edi), %ebp
	sbbl	20(%edi), %ebx
	movl	24(%esp), %edi
	movl	%ebp, 16(%edi)
	movl	%ecx, 12(%edi)
	movl	%edx, 8(%edi)
	movl	%esi, 4(%edi)
	movl	%ebx, 20(%edi)
	movl	(%esp), %ecx                    # 4-byte Reload
	movl	%ecx, (%edi)
	sbbl	%eax, %eax
	andl	$1, %eax
	addl	$4, %esp
	popl	%esi
	popl	%edi
	popl	%ebx
	popl	%ebp
	retl
.Lfunc_end14:
	.size	mcl_fp_subPre6L, .Lfunc_end14-mcl_fp_subPre6L
                                        # -- End function
	.globl	mcl_fp_shr1_6L                  # -- Begin function mcl_fp_shr1_6L
	.p2align	4, 0x90
	.type	mcl_fp_shr1_6L,@function
mcl_fp_shr1_6L:                         # @mcl_fp_shr1_6L
# %bb.0:
	pushl	%esi
	movl	12(%esp), %eax
	movl	20(%eax), %ecx
	movl	%ecx, %edx
	shrl	%edx
	movl	8(%esp), %esi
	movl	%edx, 20(%esi)
	movl	16(%eax), %edx
	shldl	$31, %edx, %ecx
	movl	%ecx, 16(%esi)
	movl	12(%eax), %ecx
	shldl	$31, %ecx, %edx
	movl	%edx, 12(%esi)
	movl	8(%eax), %edx
	shldl	$31, %edx, %ecx
	movl	%ecx, 8(%esi)
	movl	4(%eax), %ecx
	shldl	$31, %ecx, %edx
	movl	%edx, 4(%esi)
	movl	(%eax), %eax
	shrdl	$1, %ecx, %eax
	movl	%eax, (%esi)
	popl	%esi
	retl
.Lfunc_end15:
	.size	mcl_fp_shr1_6L, .Lfunc_end15-mcl_fp_shr1_6L
                                        # -- End function
	.globl	mcl_fp_add6L                    # -- Begin function mcl_fp_add6L
	.p2align	4, 0x90
	.type	mcl_fp_add6L,@function
mcl_fp_add6L:                           # @mcl_fp_add6L
# %bb.0:
	pushl	%ebp
	pushl	%ebx
	pushl	%edi
	pushl	%esi
	subl	$8, %esp
	movl	32(%esp), %ebx
	movl	(%ebx), %eax
	movl	4(%ebx), %ecx
	movl	36(%esp), %ebp
	addl	(%ebp), %eax
	adcl	4(%ebp), %ecx
	movl	20(%ebx), %edx
	movl	16(%ebx), %esi
	movl	12(%ebx), %edi
	movl	8(%ebx), %ebx
	adcl	8(%ebp), %ebx
	adcl	12(%ebp), %edi
	adcl	16(%ebp), %esi
	adcl	20(%ebp), %edx
	movl	28(%esp), %ebp
	movl	%edx, 20(%ebp)
	movl	%esi, 16(%ebp)
	movl	%edi, 12(%ebp)
	movl	%ebx, 8(%ebp)
	movl	%ecx, 4(%ebp)
	movl	%eax, (%ebp)
	setb	3(%esp)                         # 1-byte Folded Spill
	movl	40(%esp), %ebp
	subl	(%ebp), %eax
	movl	%eax, 4(%esp)                   # 4-byte Spill
	sbbl	4(%ebp), %ecx
	sbbl	8(%ebp), %ebx
	sbbl	12(%ebp), %edi
	sbbl	16(%ebp), %esi
	sbbl	20(%ebp), %edx
	movzbl	3(%esp), %eax                   # 1-byte Folded Reload
	sbbl	$0, %eax
	testb	$1, %al
	jne	.LBB16_2
# %bb.1:                                # %nocarry
	movl	4(%esp), %eax                   # 4-byte Reload
	movl	28(%esp), %ebp
	movl	%eax, (%ebp)
	movl	%ecx, 4(%ebp)
	movl	%ebx, 8(%ebp)
	movl	%edi, 12(%ebp)
	movl	%esi, 16(%ebp)
	movl	%edx, 20(%ebp)
.LBB16_2:                               # %carry
	addl	$8, %esp
	popl	%esi
	popl	%edi
	popl	%ebx
	popl	%ebp
	retl
.Lfunc_end16:
	.size	mcl_fp_add6L, .Lfunc_end16-mcl_fp_add6L
                                        # -- End function
	.globl	mcl_fp_addNF6L                  # -- Begin function mcl_fp_addNF6L
	.p2align	4, 0x90
	.type	mcl_fp_addNF6L,@function
mcl_fp_addNF6L:                         # @mcl_fp_addNF6L
# %bb.0:
	pushl	%ebp
	pushl	%ebx
	pushl	%edi
	pushl	%esi
	subl	$28, %esp
	movl	56(%esp), %ecx
	movl	(%ecx), %ebx
	movl	4(%ecx), %ebp
	movl	52(%esp), %edx
	addl	(%edx), %ebx
	adcl	4(%edx), %ebp
	movl	20(%ecx), %esi
	movl	16(%ecx), %edi
	movl	12(%ecx), %eax
	movl	8(%ecx), %ecx
	adcl	8(%edx), %ecx
	adcl	12(%edx), %eax
	adcl	16(%edx), %edi
	adcl	20(%edx), %esi
	movl	60(%esp), %edx
	movl	%ebx, 12(%esp)                  # 4-byte Spill
	subl	(%edx), %ebx
	movl	%ebx, 24(%esp)                  # 4-byte Spill
	movl	%ebp, 8(%esp)                   # 4-byte Spill
	sbbl	4(%edx), %ebp
	movl	%ebp, 20(%esp)                  # 4-byte Spill
	movl	%ecx, 4(%esp)                   # 4-byte Spill
	sbbl	8(%edx), %ecx
	movl	%ecx, 16(%esp)                  # 4-byte Spill
	movl	%eax, (%esp)                    # 4-byte Spill
	movl	%eax, %ebx
	movl	%edi, %eax
	sbbl	12(%edx), %ebx
	movl	%edi, %ebp
	sbbl	16(%edx), %ebp
	movl	%esi, %ecx
	sbbl	20(%edx), %ecx
	movl	%ecx, %edx
	sarl	$31, %edx
	testl	%edx, %edx
	js	.LBB17_1
# %bb.2:
	movl	48(%esp), %edi
	movl	%ecx, 20(%edi)
	js	.LBB17_3
.LBB17_4:
	movl	%ebp, 16(%edi)
	movl	20(%esp), %ecx                  # 4-byte Reload
	movl	16(%esp), %edx                  # 4-byte Reload
	js	.LBB17_5
.LBB17_6:
	movl	%ebx, 12(%edi)
	movl	24(%esp), %eax                  # 4-byte Reload
	js	.LBB17_7
.LBB17_8:
	movl	%edx, 8(%edi)
	js	.LBB17_9
.LBB17_10:
	movl	%ecx, 4(%edi)
	jns	.LBB17_12
.LBB17_11:
	movl	12(%esp), %eax                  # 4-byte Reload
.LBB17_12:
	movl	%eax, (%edi)
	addl	$28, %esp
	popl	%esi
	popl	%edi
	popl	%ebx
	popl	%ebp
	retl
.LBB17_1:
	movl	%esi, %ecx
	movl	48(%esp), %edi
	movl	%ecx, 20(%edi)
	jns	.LBB17_4
.LBB17_3:
	movl	%eax, %ebp
	movl	%ebp, 16(%edi)
	movl	20(%esp), %ecx                  # 4-byte Reload
	movl	16(%esp), %edx                  # 4-byte Reload
	jns	.LBB17_6
.LBB17_5:
	movl	(%esp), %ebx                    # 4-byte Reload
	movl	%ebx, 12(%edi)
	movl	24(%esp), %eax                  # 4-byte Reload
	jns	.LBB17_8
.LBB17_7:
	movl	4(%esp), %edx                   # 4-byte Reload
	movl	%edx, 8(%edi)
	jns	.LBB17_10
.LBB17_9:
	movl	8(%esp), %ecx                   # 4-byte Reload
	movl	%ecx, 4(%edi)
	js	.LBB17_11
	jmp	.LBB17_12
.Lfunc_end17:
	.size	mcl_fp_addNF6L, .Lfunc_end17-mcl_fp_addNF6L
                                        # -- End function
	.globl	mcl_fp_sub6L                    # -- Begin function mcl_fp_sub6L
	.p2align	4, 0x90
	.type	mcl_fp_sub6L,@function
mcl_fp_sub6L:                           # @mcl_fp_sub6L
# %bb.0:
	pushl	%ebp
	pushl	%ebx
	pushl	%edi
	pushl	%esi
	subl	$20, %esp
	movl	44(%esp), %edx
	movl	(%edx), %ecx
	movl	4(%edx), %esi
	xorl	%eax, %eax
	movl	48(%esp), %ebp
	subl	(%ebp), %ecx
	movl	%ecx, (%esp)                    # 4-byte Spill
	sbbl	4(%ebp), %esi
	movl	20(%edx), %ecx
	movl	16(%edx), %ebx
	movl	12(%edx), %edi
	movl	8(%edx), %edx
	sbbl	8(%ebp), %edx
	sbbl	12(%ebp), %edi
	sbbl	16(%ebp), %ebx
	sbbl	20(%ebp), %ecx
	sbbl	%eax, %eax
	testb	$1, %al
	movl	40(%esp), %eax
	movl	%ecx, 4(%esp)                   # 4-byte Spill
	movl	%ecx, 20(%eax)
	movl	%ebx, 8(%esp)                   # 4-byte Spill
	movl	%ebx, 16(%eax)
	movl	%edi, 12(%esp)                  # 4-byte Spill
	movl	%edi, 12(%eax)
	movl	%edx, 8(%eax)
	movl	%esi, 16(%esp)                  # 4-byte Spill
	movl	%esi, 4(%eax)
	movl	(%esp), %ecx                    # 4-byte Reload
	movl	%ecx, (%eax)
	je	.LBB18_2
# %bb.1:                                # %carry
	movl	%ecx, %ebp
	movl	52(%esp), %ecx
	addl	(%ecx), %ebp
	movl	%ebp, (%esp)                    # 4-byte Spill
	movl	16(%esp), %ebp                  # 4-byte Reload
	adcl	4(%ecx), %ebp
	adcl	8(%ecx), %edx
	movl	12(%esp), %edi                  # 4-byte Reload
	adcl	12(%ecx), %edi
	movl	8(%esp), %ebx                   # 4-byte Reload
	adcl	16(%ecx), %ebx
	movl	4(%esp), %ecx                   # 4-byte Reload
	movl	52(%esp), %esi
	adcl	20(%esi), %ecx
	movl	%ecx, 20(%eax)
	movl	%ebx, 16(%eax)
	movl	%edi, 12(%eax)
	movl	%edx, 8(%eax)
	movl	%ebp, 4(%eax)
	movl	(%esp), %ecx                    # 4-byte Reload
	movl	%ecx, (%eax)
.LBB18_2:                               # %nocarry
	addl	$20, %esp
	popl	%esi
	popl	%edi
	popl	%ebx
	popl	%ebp
	retl
.Lfunc_end18:
	.size	mcl_fp_sub6L, .Lfunc_end18-mcl_fp_sub6L
                                        # -- End function
	.globl	mcl_fp_subNF6L                  # -- Begin function mcl_fp_subNF6L
	.p2align	4, 0x90
	.type	mcl_fp_subNF6L,@function
mcl_fp_subNF6L:                         # @mcl_fp_subNF6L
# %bb.0:
	pushl	%ebp
	pushl	%ebx
	pushl	%edi
	pushl	%esi
	subl	$24, %esp
	movl	48(%esp), %ebx
	movl	(%ebx), %ecx
	movl	4(%ebx), %eax
	movl	52(%esp), %ebp
	subl	(%ebp), %ecx
	movl	%ecx, 12(%esp)                  # 4-byte Spill
	sbbl	4(%ebp), %eax
	movl	%eax, 16(%esp)                  # 4-byte Spill
	movl	20(%ebx), %ecx
	movl	16(%ebx), %eax
	movl	12(%ebx), %edx
	movl	8(%ebx), %esi
	sbbl	8(%ebp), %esi
	movl	%esi, (%esp)                    # 4-byte Spill
	sbbl	12(%ebp), %edx
	movl	%edx, 8(%esp)                   # 4-byte Spill
	sbbl	16(%ebp), %eax
	movl	%eax, 20(%esp)                  # 4-byte Spill
	sbbl	20(%ebp), %ecx
	movl	%ecx, 4(%esp)                   # 4-byte Spill
	movl	%ecx, %ebp
	sarl	$31, %ebp
	movl	%ebp, %eax
	shldl	$1, %ecx, %eax
	movl	56(%esp), %ebx
	andl	(%ebx), %eax
	movl	20(%ebx), %edi
	andl	%ebp, %edi
	movl	16(%ebx), %esi
	andl	%ebp, %esi
	movl	12(%ebx), %edx
	andl	%ebp, %edx
	movl	8(%ebx), %ecx
	andl	%ebp, %ecx
	andl	4(%ebx), %ebp
	addl	12(%esp), %eax                  # 4-byte Folded Reload
	adcl	16(%esp), %ebp                  # 4-byte Folded Reload
	movl	44(%esp), %ebx
	movl	%eax, (%ebx)
	adcl	(%esp), %ecx                    # 4-byte Folded Reload
	movl	%ebp, 4(%ebx)
	adcl	8(%esp), %edx                   # 4-byte Folded Reload
	movl	%ecx, 8(%ebx)
	adcl	20(%esp), %esi                  # 4-byte Folded Reload
	movl	%edx, 12(%ebx)
	movl	%esi, 16(%ebx)
	adcl	4(%esp), %edi                   # 4-byte Folded Reload
	movl	%edi, 20(%ebx)
	addl	$24, %esp
	popl	%esi
	popl	%edi
	popl	%ebx
	popl	%ebp
	retl
.Lfunc_end19:
	.size	mcl_fp_subNF6L, .Lfunc_end19-mcl_fp_subNF6L
                                        # -- End function
	.globl	mcl_fpDbl_add6L                 # -- Begin function mcl_fpDbl_add6L
	.p2align	4, 0x90
	.type	mcl_fpDbl_add6L,@function
mcl_fpDbl_add6L:                        # @mcl_fpDbl_add6L
# %bb.0:
	pushl	%ebp
	pushl	%ebx
	pushl	%edi
	pushl	%esi
	subl	$32, %esp
	movl	56(%esp), %esi
	movl	(%esi), %eax
	movl	4(%esi), %ecx
	movl	60(%esp), %edx
	addl	(%edx), %eax
	movl	%eax, 20(%esp)                  # 4-byte Spill
	adcl	4(%edx), %ecx
	movl	%ecx, 16(%esp)                  # 4-byte Spill
	movl	44(%esi), %eax
	movl	%eax, 24(%esp)                  # 4-byte Spill
	movl	40(%esi), %eax
	movl	%eax, 4(%esp)                   # 4-byte Spill
	movl	36(%esi), %eax
	movl	%eax, 8(%esp)                   # 4-byte Spill
	movl	32(%esi), %ecx
	movl	28(%esi), %ebx
	movl	24(%esi), %edi
	movl	20(%esi), %eax
	movl	%eax, (%esp)                    # 4-byte Spill
	movl	16(%esi), %ebp
	movl	12(%esi), %eax
	movl	8(%esi), %esi
	adcl	8(%edx), %esi
	adcl	12(%edx), %eax
	movl	%eax, 12(%esp)                  # 4-byte Spill
	adcl	16(%edx), %ebp
	movl	(%esp), %eax                    # 4-byte Reload
	adcl	20(%edx), %eax
	movl	%eax, (%esp)                    # 4-byte Spill
	adcl	24(%edx), %edi
	adcl	28(%edx), %ebx
	adcl	32(%edx), %ecx
	movl	%ecx, 28(%esp)                  # 4-byte Spill
	movl	8(%esp), %eax                   # 4-byte Reload
	adcl	36(%edx), %eax
	movl	%eax, 8(%esp)                   # 4-byte Spill
	movl	4(%esp), %eax                   # 4-byte Reload
	adcl	40(%edx), %eax
	movl	%eax, 4(%esp)                   # 4-byte Spill
	movl	24(%esp), %eax                  # 4-byte Reload
	adcl	44(%edx), %eax
	movl	%eax, 24(%esp)                  # 4-byte Spill
	movl	52(%esp), %edx
	movl	(%esp), %eax                    # 4-byte Reload
	movl	%eax, 20(%edx)
	movl	%ebp, 16(%edx)
	movl	12(%esp), %eax                  # 4-byte Reload
	movl	%eax, 12(%edx)
	movl	%esi, 8(%edx)
	movl	16(%esp), %eax                  # 4-byte Reload
	movl	%eax, 4(%edx)
	movl	20(%esp), %eax                  # 4-byte Reload
	movl	%eax, (%edx)
	setb	20(%esp)                        # 1-byte Folded Spill
	movl	64(%esp), %eax
	movl	%edi, 16(%esp)                  # 4-byte Spill
	subl	(%eax), %edi
	movl	%edi, (%esp)                    # 4-byte Spill
	movl	%ebx, 12(%esp)                  # 4-byte Spill
	movl	%ebx, %ecx
	sbbl	4(%eax), %ecx
	movl	28(%esp), %edx                  # 4-byte Reload
	sbbl	8(%eax), %edx
	movl	8(%esp), %esi                   # 4-byte Reload
	sbbl	12(%eax), %esi
	movl	4(%esp), %ebx                   # 4-byte Reload
	sbbl	16(%eax), %ebx
	movl	24(%esp), %ebp                  # 4-byte Reload
	movl	%ebp, %edi
	sbbl	20(%eax), %ebp
	movzbl	20(%esp), %eax                  # 1-byte Folded Reload
	sbbl	$0, %eax
	testb	$1, %al
	jne	.LBB20_1
# %bb.2:
	movl	52(%esp), %eax
	movl	%ebp, 44(%eax)
	jne	.LBB20_3
.LBB20_4:
	movl	%ebx, 40(%eax)
	jne	.LBB20_5
.LBB20_6:
	movl	%esi, 36(%eax)
	jne	.LBB20_7
.LBB20_8:
	movl	%edx, 32(%eax)
	jne	.LBB20_9
.LBB20_10:
	movl	%ecx, 28(%eax)
	movl	(%esp), %ecx                    # 4-byte Reload
	je	.LBB20_12
.LBB20_11:
	movl	16(%esp), %ecx                  # 4-byte Reload
.LBB20_12:
	movl	%ecx, 24(%eax)
	addl	$32, %esp
	popl	%esi
	popl	%edi
	popl	%ebx
	popl	%ebp
	retl
.LBB20_1:
	movl	%edi, %ebp
	movl	52(%esp), %eax
	movl	%ebp, 44(%eax)
	je	.LBB20_4
.LBB20_3:
	movl	4(%esp), %ebx                   # 4-byte Reload
	movl	%ebx, 40(%eax)
	je	.LBB20_6
.LBB20_5:
	movl	8(%esp), %esi                   # 4-byte Reload
	movl	%esi, 36(%eax)
	je	.LBB20_8
.LBB20_7:
	movl	28(%esp), %edx                  # 4-byte Reload
	movl	%edx, 32(%eax)
	je	.LBB20_10
.LBB20_9:
	movl	12(%esp), %ecx                  # 4-byte Reload
	movl	%ecx, 28(%eax)
	movl	(%esp), %ecx                    # 4-byte Reload
	jne	.LBB20_11
	jmp	.LBB20_12
.Lfunc_end20:
	.size	mcl_fpDbl_add6L, .Lfunc_end20-mcl_fpDbl_add6L
                                        # -- End function
	.globl	mcl_fpDbl_sub6L                 # -- Begin function mcl_fpDbl_sub6L
	.p2align	4, 0x90
	.type	mcl_fpDbl_sub6L,@function
mcl_fpDbl_sub6L:                        # @mcl_fpDbl_sub6L
# %bb.0:
	pushl	%ebp
	pushl	%ebx
	pushl	%edi
	pushl	%esi
	subl	$36, %esp
	movl	60(%esp), %eax
	movl	(%eax), %esi
	movl	4(%eax), %edi
	xorl	%edx, %edx
	movl	64(%esp), %ebx
	subl	(%ebx), %esi
	movl	%esi, 32(%esp)                  # 4-byte Spill
	sbbl	4(%ebx), %edi
	movl	%edi, 28(%esp)                  # 4-byte Spill
	movl	44(%eax), %ecx
	movl	%ecx, (%esp)                    # 4-byte Spill
	movl	40(%eax), %edi
	movl	%edi, 20(%esp)                  # 4-byte Spill
	movl	36(%eax), %edi
	movl	%edi, 16(%esp)                  # 4-byte Spill
	movl	32(%eax), %edi
	movl	%edi, 12(%esp)                  # 4-byte Spill
	movl	28(%eax), %edi
	movl	%edi, 8(%esp)                   # 4-byte Spill
	movl	24(%eax), %edi
	movl	%edi, 4(%esp)                   # 4-byte Spill
	movl	20(%eax), %esi
	movl	16(%eax), %edi
	movl	12(%eax), %ecx
	movl	8(%eax), %ebp
	sbbl	8(%ebx), %ebp
	sbbl	12(%ebx), %ecx
	movl	%ecx, 24(%esp)                  # 4-byte Spill
	sbbl	16(%ebx), %edi
	sbbl	20(%ebx), %esi
	movl	%esi, %ecx
	movl	4(%esp), %eax                   # 4-byte Reload
	sbbl	24(%ebx), %eax
	movl	%eax, 4(%esp)                   # 4-byte Spill
	movl	8(%esp), %eax                   # 4-byte Reload
	sbbl	28(%ebx), %eax
	movl	%eax, 8(%esp)                   # 4-byte Spill
	movl	12(%esp), %eax                  # 4-byte Reload
	sbbl	32(%ebx), %eax
	movl	%eax, 12(%esp)                  # 4-byte Spill
	movl	16(%esp), %eax                  # 4-byte Reload
	sbbl	36(%ebx), %eax
	movl	%eax, 16(%esp)                  # 4-byte Spill
	movl	20(%esp), %eax                  # 4-byte Reload
	sbbl	40(%ebx), %eax
	movl	%eax, 20(%esp)                  # 4-byte Spill
	movl	(%esp), %eax                    # 4-byte Reload
	sbbl	44(%ebx), %eax
	movl	%eax, (%esp)                    # 4-byte Spill
	movl	56(%esp), %esi
	movl	%ecx, 20(%esi)
	movl	%edi, 16(%esi)
	movl	24(%esp), %eax                  # 4-byte Reload
	movl	%eax, 12(%esi)
	movl	%ebp, 8(%esi)
	movl	28(%esp), %eax                  # 4-byte Reload
	movl	%eax, 4(%esi)
	movl	32(%esp), %eax                  # 4-byte Reload
	movl	%eax, (%esi)
	sbbl	%edx, %edx
	andl	$1, %edx
	negl	%edx
	movl	68(%esp), %eax
	movl	20(%eax), %ecx
	andl	%edx, %ecx
	movl	16(%eax), %edi
	andl	%edx, %edi
	movl	12(%eax), %ebx
	andl	%edx, %ebx
	movl	8(%eax), %ebp
	andl	%edx, %ebp
	movl	68(%esp), %eax
	movl	4(%eax), %eax
	andl	%edx, %eax
	movl	68(%esp), %esi
	andl	(%esi), %edx
	addl	4(%esp), %edx                   # 4-byte Folded Reload
	adcl	8(%esp), %eax                   # 4-byte Folded Reload
	movl	56(%esp), %esi
	movl	%edx, 24(%esi)
	adcl	12(%esp), %ebp                  # 4-byte Folded Reload
	movl	%eax, 28(%esi)
	adcl	16(%esp), %ebx                  # 4-byte Folded Reload
	movl	%ebp, 32(%esi)
	adcl	20(%esp), %edi                  # 4-byte Folded Reload
	movl	%ebx, 36(%esi)
	movl	%edi, 40(%esi)
	adcl	(%esp), %ecx                    # 4-byte Folded Reload
	movl	%ecx, 44(%esi)
	addl	$36, %esp
	popl	%esi
	popl	%edi
	popl	%ebx
	popl	%ebp
	retl
.Lfunc_end21:
	.size	mcl_fpDbl_sub6L, .Lfunc_end21-mcl_fpDbl_sub6L
                                        # -- End function
	.globl	mulPv224x32                     # -- Begin function mulPv224x32
	.p2align	4, 0x90
	.type	mulPv224x32,@function
mulPv224x32:                            # @mulPv224x32
# %bb.0:
	pushl	%ebp
	pushl	%ebx
	pushl	%edi
	pushl	%esi
	subl	$36, %esp
	movl	64(%esp), %esi
	movl	60(%esp), %ebx
	movl	%esi, %eax
	mull	24(%ebx)
	movl	%edx, 32(%esp)                  # 4-byte Spill
	movl	%eax, 28(%esp)                  # 4-byte Spill
	movl	%esi, %eax
	mull	20(%ebx)
	movl	%edx, 24(%esp)                  # 4-byte Spill
	movl	%eax, 20(%esp)                  # 4-byte Spill
	movl	%esi, %eax
	mull	16(%ebx)
	movl	%edx, 16(%esp)                  # 4-byte Spill
	movl	%eax, 12(%esp)                  # 4-byte Spill
	movl	%esi, %eax
	mull	12(%ebx)
	movl	%edx, %ebp
	movl	%eax, 8(%esp)                   # 4-byte Spill
	movl	%esi, %eax
	mull	8(%ebx)
	movl	%edx, %ecx
	movl	%eax, 4(%esp)                   # 4-byte Spill
	movl	%esi, %eax
	mull	4(%ebx)
	movl	%edx, %edi
	movl	%eax, (%esp)                    # 4-byte Spill
	movl	%esi, %eax
	mull	(%ebx)
	movl	56(%esp), %esi
	movl	%eax, (%esi)
	addl	(%esp), %edx                    # 4-byte Folded Reload
	movl	%edx, 4(%esi)
	adcl	4(%esp), %edi                   # 4-byte Folded Reload
	movl	%edi, 8(%esi)
	adcl	8(%esp), %ecx                   # 4-byte Folded Reload
	movl	%ecx, 12(%esi)
	adcl	12(%esp), %ebp                  # 4-byte Folded Reload
	movl	%ebp, 16(%esi)
	movl	16(%esp), %eax                  # 4-byte Reload
	adcl	20(%esp), %eax                  # 4-byte Folded Reload
	movl	%eax, 20(%esi)
	movl	24(%esp), %eax                  # 4-byte Reload
	adcl	28(%esp), %eax                  # 4-byte Folded Reload
	movl	%eax, 24(%esi)
	movl	32(%esp), %eax                  # 4-byte Reload
	adcl	$0, %eax
	movl	%eax, 28(%esi)
	movl	%esi, %eax
	addl	$36, %esp
	popl	%esi
	popl	%edi
	popl	%ebx
	popl	%ebp
	retl	$4
.Lfunc_end22:
	.size	mulPv224x32, .Lfunc_end22-mulPv224x32
                                        # -- End function
	.globl	mcl_fp_mulUnitPre7L             # -- Begin function mcl_fp_mulUnitPre7L
	.p2align	4, 0x90
	.type	mcl_fp_mulUnitPre7L,@function
mcl_fp_mulUnitPre7L:                    # @mcl_fp_mulUnitPre7L
# %bb.0:
	pushl	%ebp
	pushl	%ebx
	pushl	%edi
	pushl	%esi
	subl	$36, %esp
	movl	64(%esp), %esi
	movl	60(%esp), %ebx
	movl	%esi, %eax
	mull	24(%ebx)
	movl	%edx, 32(%esp)                  # 4-byte Spill
	movl	%eax, 28(%esp)                  # 4-byte Spill
	movl	%esi, %eax
	mull	20(%ebx)
	movl	%edx, 24(%esp)                  # 4-byte Spill
	movl	%eax, 20(%esp)                  # 4-byte Spill
	movl	%esi, %eax
	mull	16(%ebx)
	movl	%edx, 16(%esp)                  # 4-byte Spill
	movl	%eax, 12(%esp)                  # 4-byte Spill
	movl	%esi, %eax
	mull	12(%ebx)
	movl	%edx, %ebp
	movl	%eax, 8(%esp)                   # 4-byte Spill
	movl	%esi, %eax
	mull	8(%ebx)
	movl	%edx, %ecx
	movl	%eax, 4(%esp)                   # 4-byte Spill
	movl	%esi, %eax
	mull	4(%ebx)
	movl	%edx, %edi
	movl	%eax, (%esp)                    # 4-byte Spill
	movl	%esi, %eax
	mull	(%ebx)
	movl	56(%esp), %esi
	movl	%eax, (%esi)
	addl	(%esp), %edx                    # 4-byte Folded Reload
	movl	%edx, 4(%esi)
	adcl	4(%esp), %edi                   # 4-byte Folded Reload
	movl	%edi, 8(%esi)
	adcl	8(%esp), %ecx                   # 4-byte Folded Reload
	movl	%ecx, 12(%esi)
	adcl	12(%esp), %ebp                  # 4-byte Folded Reload
	movl	%ebp, 16(%esi)
	movl	16(%esp), %eax                  # 4-byte Reload
	adcl	20(%esp), %eax                  # 4-byte Folded Reload
	movl	%eax, 20(%esi)
	movl	24(%esp), %eax                  # 4-byte Reload
	adcl	28(%esp), %eax                  # 4-byte Folded Reload
	movl	%eax, 24(%esi)
	movl	32(%esp), %eax                  # 4-byte Reload
	adcl	$0, %eax
	movl	%eax, 28(%esi)
	addl	$36, %esp
	popl	%esi
	popl	%edi
	popl	%ebx
	popl	%ebp
	retl
.Lfunc_end23:
	.size	mcl_fp_mulUnitPre7L, .Lfunc_end23-mcl_fp_mulUnitPre7L
                                        # -- End function
	.globl	mcl_fpDbl_mulPre7L              # -- Begin function mcl_fpDbl_mulPre7L
	.p2align	4, 0x90
	.type	mcl_fpDbl_mulPre7L,@function
mcl_fpDbl_mulPre7L:                     # @mcl_fpDbl_mulPre7L
# %bb.0:
	pushl	%ebp
	pushl	%ebx
	pushl	%edi
	pushl	%esi
	subl	$112, %esp
	movl	136(%esp), %ecx
	movl	(%ecx), %eax
	movl	%eax, 88(%esp)                  # 4-byte Spill
	movl	140(%esp), %edx
	movl	(%edx), %edi
	mull	%edi
	movl	%edx, 8(%esp)                   # 4-byte Spill
	movl	132(%esp), %edx
	movl	%eax, (%edx)
	movl	4(%ecx), %eax
	movl	%eax, 64(%esp)                  # 4-byte Spill
	movl	8(%ecx), %eax
	movl	%eax, 84(%esp)                  # 4-byte Spill
	movl	12(%ecx), %eax
	movl	%eax, 68(%esp)                  # 4-byte Spill
	movl	16(%ecx), %esi
	movl	20(%ecx), %ebx
	movl	%ebx, 60(%esp)                  # 4-byte Spill
	movl	24(%ecx), %ebp
	movl	140(%esp), %eax
	movl	4(%eax), %ecx
	movl	%ecx, %eax
	mull	%ebp
	movl	%ebp, 72(%esp)                  # 4-byte Spill
	movl	%edx, 4(%esp)                   # 4-byte Spill
	movl	%eax, 28(%esp)                  # 4-byte Spill
	movl	%ecx, %eax
	mull	%ebx
	movl	%edx, 36(%esp)                  # 4-byte Spill
	movl	%eax, 24(%esp)                  # 4-byte Spill
	movl	%ecx, %eax
	mull	%esi
	movl	%esi, 80(%esp)                  # 4-byte Spill
	movl	%edx, (%esp)                    # 4-byte Spill
	movl	%eax, 12(%esp)                  # 4-byte Spill
	movl	%ecx, %eax
	mull	68(%esp)                        # 4-byte Folded Reload
	movl	%edx, 32(%esp)                  # 4-byte Spill
	movl	%eax, 56(%esp)                  # 4-byte Spill
	movl	%ecx, %eax
	mull	84(%esp)                        # 4-byte Folded Reload
	movl	%edx, 16(%esp)                  # 4-byte Spill
	movl	%eax, 48(%esp)                  # 4-byte Spill
	movl	%ecx, %eax
	mull	64(%esp)                        # 4-byte Folded Reload
	movl	%edx, 44(%esp)                  # 4-byte Spill
	movl	%eax, 76(%esp)                  # 4-byte Spill
	movl	%ecx, %eax
	mull	88(%esp)                        # 4-byte Folded Reload
	movl	%edx, 52(%esp)                  # 4-byte Spill
	movl	%eax, 108(%esp)                 # 4-byte Spill
	movl	%ebp, %eax
	mull	%edi
	movl	%edx, %ebp
	movl	%eax, 104(%esp)                 # 4-byte Spill
	movl	60(%esp), %eax                  # 4-byte Reload
	mull	%edi
	movl	%edx, 20(%esp)                  # 4-byte Spill
	movl	%eax, 100(%esp)                 # 4-byte Spill
	movl	%esi, %eax
	mull	%edi
	movl	%edx, %ebx
	movl	%eax, 96(%esp)                  # 4-byte Spill
	movl	68(%esp), %eax                  # 4-byte Reload
	mull	%edi
	movl	%edx, %esi
	movl	%eax, 92(%esp)                  # 4-byte Spill
	movl	84(%esp), %eax                  # 4-byte Reload
	mull	%edi
	movl	%edx, 40(%esp)                  # 4-byte Spill
	movl	%eax, %ecx
	movl	64(%esp), %eax                  # 4-byte Reload
	mull	%edi
	addl	8(%esp), %eax                   # 4-byte Folded Reload
	adcl	%ecx, %edx
	movl	%edx, 8(%esp)                   # 4-byte Spill
	movl	40(%esp), %edi                  # 4-byte Reload
	adcl	92(%esp), %edi                  # 4-byte Folded Reload
	adcl	96(%esp), %esi                  # 4-byte Folded Reload
	adcl	100(%esp), %ebx                 # 4-byte Folded Reload
	movl	104(%esp), %ecx                 # 4-byte Reload
	adcl	%ecx, 20(%esp)                  # 4-byte Folded Spill
	movl	%ebp, %ecx
	adcl	$0, %ecx
	addl	108(%esp), %eax                 # 4-byte Folded Reload
	movl	132(%esp), %edx
	movl	%eax, 4(%edx)
	movl	8(%esp), %ebp                   # 4-byte Reload
	adcl	76(%esp), %ebp                  # 4-byte Folded Reload
	adcl	48(%esp), %edi                  # 4-byte Folded Reload
	adcl	56(%esp), %esi                  # 4-byte Folded Reload
	adcl	12(%esp), %ebx                  # 4-byte Folded Reload
	movl	20(%esp), %edx                  # 4-byte Reload
	adcl	24(%esp), %edx                  # 4-byte Folded Reload
	adcl	28(%esp), %ecx                  # 4-byte Folded Reload
	setb	%al
	addl	52(%esp), %ebp                  # 4-byte Folded Reload
	adcl	44(%esp), %edi                  # 4-byte Folded Reload
	movl	%edi, 40(%esp)                  # 4-byte Spill
	adcl	16(%esp), %esi                  # 4-byte Folded Reload
	movl	%esi, 24(%esp)                  # 4-byte Spill
	adcl	32(%esp), %ebx                  # 4-byte Folded Reload
	movl	%ebx, 56(%esp)                  # 4-byte Spill
	adcl	(%esp), %edx                    # 4-byte Folded Reload
	movl	%edx, 20(%esp)                  # 4-byte Spill
	adcl	36(%esp), %ecx                  # 4-byte Folded Reload
	movl	%ecx, (%esp)                    # 4-byte Spill
	movzbl	%al, %eax
	adcl	4(%esp), %eax                   # 4-byte Folded Reload
	movl	%eax, 8(%esp)                   # 4-byte Spill
	movl	140(%esp), %eax
	movl	8(%eax), %ebx
	movl	%ebx, %eax
	mull	72(%esp)                        # 4-byte Folded Reload
	movl	%edx, 4(%esp)                   # 4-byte Spill
	movl	%eax, 44(%esp)                  # 4-byte Spill
	movl	%ebx, %eax
	mull	60(%esp)                        # 4-byte Folded Reload
	movl	%edx, 36(%esp)                  # 4-byte Spill
	movl	%eax, 52(%esp)                  # 4-byte Spill
	movl	%ebx, %eax
	mull	80(%esp)                        # 4-byte Folded Reload
	movl	%edx, 32(%esp)                  # 4-byte Spill
	movl	%eax, 16(%esp)                  # 4-byte Spill
	movl	%ebx, %eax
	mull	68(%esp)                        # 4-byte Folded Reload
	movl	%edx, %esi
	movl	%eax, 48(%esp)                  # 4-byte Spill
	movl	%ebx, %eax
	mull	84(%esp)                        # 4-byte Folded Reload
	movl	%edx, 28(%esp)                  # 4-byte Spill
	movl	%eax, 12(%esp)                  # 4-byte Spill
	movl	%ebx, %eax
	mull	64(%esp)                        # 4-byte Folded Reload
	movl	%edx, %edi
	movl	%eax, %ecx
	movl	%ebx, %eax
	mull	88(%esp)                        # 4-byte Folded Reload
	movl	%eax, %ebx
	addl	%ecx, %edx
	movl	%edx, %ecx
	adcl	12(%esp), %edi                  # 4-byte Folded Reload
	movl	%edi, 12(%esp)                  # 4-byte Spill
	movl	28(%esp), %edi                  # 4-byte Reload
	adcl	48(%esp), %edi                  # 4-byte Folded Reload
	adcl	16(%esp), %esi                  # 4-byte Folded Reload
	movl	%esi, 16(%esp)                  # 4-byte Spill
	movl	32(%esp), %esi                  # 4-byte Reload
	adcl	52(%esp), %esi                  # 4-byte Folded Reload
	movl	36(%esp), %eax                  # 4-byte Reload
	adcl	44(%esp), %eax                  # 4-byte Folded Reload
	movl	4(%esp), %edx                   # 4-byte Reload
	adcl	$0, %edx
	addl	%ebp, %ebx
	adcl	40(%esp), %ecx                  # 4-byte Folded Reload
	movl	%ecx, 44(%esp)                  # 4-byte Spill
	movl	132(%esp), %ecx
	movl	%ebx, 8(%ecx)
	movl	24(%esp), %ecx                  # 4-byte Reload
	adcl	%ecx, 12(%esp)                  # 4-byte Folded Spill
	adcl	56(%esp), %edi                  # 4-byte Folded Reload
	movl	%edi, 28(%esp)                  # 4-byte Spill
	movl	20(%esp), %ecx                  # 4-byte Reload
	adcl	%ecx, 16(%esp)                  # 4-byte Folded Spill
	adcl	(%esp), %esi                    # 4-byte Folded Reload
	movl	%esi, 32(%esp)                  # 4-byte Spill
	adcl	8(%esp), %eax                   # 4-byte Folded Reload
	movl	%eax, 36(%esp)                  # 4-byte Spill
	adcl	$0, %edx
	movl	%edx, 4(%esp)                   # 4-byte Spill
	movl	140(%esp), %eax
	movl	12(%eax), %ecx
	movl	%ecx, %eax
	mull	72(%esp)                        # 4-byte Folded Reload
	movl	%edx, 40(%esp)                  # 4-byte Spill
	movl	%eax, 56(%esp)                  # 4-byte Spill
	movl	%ecx, %eax
	mull	60(%esp)                        # 4-byte Folded Reload
	movl	%edx, (%esp)                    # 4-byte Spill
	movl	%eax, 52(%esp)                  # 4-byte Spill
	movl	%ecx, %eax
	mull	80(%esp)                        # 4-byte Folded Reload
	movl	%edx, 24(%esp)                  # 4-byte Spill
	movl	%eax, 48(%esp)                  # 4-byte Spill
	movl	%ecx, %eax
	mull	68(%esp)                        # 4-byte Folded Reload
	movl	%edx, %esi
	movl	%eax, 76(%esp)                  # 4-byte Spill
	movl	%ecx, %eax
	mull	84(%esp)                        # 4-byte Folded Reload
	movl	%edx, %ebx
	movl	%eax, 8(%esp)                   # 4-byte Spill
	movl	%ecx, %eax
	mull	64(%esp)                        # 4-byte Folded Reload
	movl	%edx, %edi
	movl	%eax, %ebp
	movl	%ecx, %eax
	mull	88(%esp)                        # 4-byte Folded Reload
	addl	%ebp, %edx
	movl	%edx, 20(%esp)                  # 4-byte Spill
	adcl	8(%esp), %edi                   # 4-byte Folded Reload
	movl	%edi, 8(%esp)                   # 4-byte Spill
	adcl	76(%esp), %ebx                  # 4-byte Folded Reload
	movl	%ebx, %ebp
	adcl	48(%esp), %esi                  # 4-byte Folded Reload
	movl	%esi, %ebx
	movl	24(%esp), %ecx                  # 4-byte Reload
	adcl	52(%esp), %ecx                  # 4-byte Folded Reload
	movl	(%esp), %edi                    # 4-byte Reload
	adcl	56(%esp), %edi                  # 4-byte Folded Reload
	movl	40(%esp), %esi                  # 4-byte Reload
	adcl	$0, %esi
	addl	44(%esp), %eax                  # 4-byte Folded Reload
	movl	132(%esp), %edx
	movl	%eax, 12(%edx)
	movl	20(%esp), %eax                  # 4-byte Reload
	adcl	12(%esp), %eax                  # 4-byte Folded Reload
	movl	%eax, 20(%esp)                  # 4-byte Spill
	movl	28(%esp), %eax                  # 4-byte Reload
	adcl	%eax, 8(%esp)                   # 4-byte Folded Spill
	adcl	16(%esp), %ebp                  # 4-byte Folded Reload
	movl	%ebp, 52(%esp)                  # 4-byte Spill
	adcl	32(%esp), %ebx                  # 4-byte Folded Reload
	movl	%ebx, 56(%esp)                  # 4-byte Spill
	adcl	36(%esp), %ecx                  # 4-byte Folded Reload
	movl	%ecx, 24(%esp)                  # 4-byte Spill
	adcl	4(%esp), %edi                   # 4-byte Folded Reload
	movl	%edi, (%esp)                    # 4-byte Spill
	adcl	$0, %esi
	movl	%esi, 40(%esp)                  # 4-byte Spill
	movl	140(%esp), %eax
	movl	16(%eax), %ebx
	movl	%ebx, %eax
	mull	72(%esp)                        # 4-byte Folded Reload
	movl	%edx, 32(%esp)                  # 4-byte Spill
	movl	%eax, 4(%esp)                   # 4-byte Spill
	movl	%ebx, %eax
	mull	60(%esp)                        # 4-byte Folded Reload
	movl	%edx, 28(%esp)                  # 4-byte Spill
	movl	%eax, 36(%esp)                  # 4-byte Spill
	movl	%ebx, %eax
	mull	80(%esp)                        # 4-byte Folded Reload
	movl	%edx, 16(%esp)                  # 4-byte Spill
	movl	%eax, 48(%esp)                  # 4-byte Spill
	movl	%ebx, %eax
	mull	68(%esp)                        # 4-byte Folded Reload
	movl	%edx, %ecx
	movl	%eax, 76(%esp)                  # 4-byte Spill
	movl	%ebx, %eax
	mull	84(%esp)                        # 4-byte Folded Reload
	movl	%edx, 12(%esp)                  # 4-byte Spill
	movl	%eax, 44(%esp)                  # 4-byte Spill
	movl	%ebx, %eax
	mull	64(%esp)                        # 4-byte Folded Reload
	movl	%edx, %esi
	movl	%eax, %edi
	movl	%ebx, %eax
	mull	88(%esp)                        # 4-byte Folded Reload
	movl	%eax, %ebp
	addl	%edi, %edx
	movl	%edx, %edi
	adcl	44(%esp), %esi                  # 4-byte Folded Reload
	movl	%esi, 44(%esp)                  # 4-byte Spill
	movl	12(%esp), %esi                  # 4-byte Reload
	adcl	76(%esp), %esi                  # 4-byte Folded Reload
	adcl	48(%esp), %ecx                  # 4-byte Folded Reload
	movl	%ecx, %ebx
	movl	16(%esp), %ecx                  # 4-byte Reload
	adcl	36(%esp), %ecx                  # 4-byte Folded Reload
	movl	28(%esp), %eax                  # 4-byte Reload
	adcl	4(%esp), %eax                   # 4-byte Folded Reload
	movl	32(%esp), %edx                  # 4-byte Reload
	adcl	$0, %edx
	addl	20(%esp), %ebp                  # 4-byte Folded Reload
	adcl	8(%esp), %edi                   # 4-byte Folded Reload
	movl	%edi, 8(%esp)                   # 4-byte Spill
	movl	132(%esp), %edi
	movl	%ebp, 16(%edi)
	movl	52(%esp), %edi                  # 4-byte Reload
	adcl	%edi, 44(%esp)                  # 4-byte Folded Spill
	adcl	56(%esp), %esi                  # 4-byte Folded Reload
	movl	%esi, 12(%esp)                  # 4-byte Spill
	adcl	24(%esp), %ebx                  # 4-byte Folded Reload
	movl	%ebx, 24(%esp)                  # 4-byte Spill
	adcl	(%esp), %ecx                    # 4-byte Folded Reload
	movl	%ecx, 16(%esp)                  # 4-byte Spill
	adcl	40(%esp), %eax                  # 4-byte Folded Reload
	movl	%eax, 28(%esp)                  # 4-byte Spill
	adcl	$0, %edx
	movl	%edx, 32(%esp)                  # 4-byte Spill
	movl	140(%esp), %eax
	movl	20(%eax), %ecx
	movl	%ecx, %eax
	mull	72(%esp)                        # 4-byte Folded Reload
	movl	%edx, 4(%esp)                   # 4-byte Spill
	movl	%eax, 56(%esp)                  # 4-byte Spill
	movl	%ecx, %eax
	mull	60(%esp)                        # 4-byte Folded Reload
	movl	%edx, 40(%esp)                  # 4-byte Spill
	movl	%eax, 52(%esp)                  # 4-byte Spill
	movl	%ecx, %eax
	mull	80(%esp)                        # 4-byte Folded Reload
	movl	%edx, 36(%esp)                  # 4-byte Spill
	movl	%eax, (%esp)                    # 4-byte Spill
	movl	%ecx, %eax
	mull	68(%esp)                        # 4-byte Folded Reload
	movl	%edx, %esi
	movl	%eax, 48(%esp)                  # 4-byte Spill
	movl	%ecx, %eax
	mull	84(%esp)                        # 4-byte Folded Reload
	movl	%edx, %edi
	movl	%eax, 76(%esp)                  # 4-byte Spill
	movl	%ecx, %eax
	mull	64(%esp)                        # 4-byte Folded Reload
	movl	%edx, %ebx
	movl	%eax, %ebp
	movl	%ecx, %eax
	mull	88(%esp)                        # 4-byte Folded Reload
	movl	%eax, %ecx
	addl	%ebp, %edx
	movl	%edx, 20(%esp)                  # 4-byte Spill
	adcl	76(%esp), %ebx                  # 4-byte Folded Reload
	adcl	48(%esp), %edi                  # 4-byte Folded Reload
	movl	%edi, %eax
	adcl	(%esp), %esi                    # 4-byte Folded Reload
	movl	%esi, (%esp)                    # 4-byte Spill
	movl	36(%esp), %ebp                  # 4-byte Reload
	adcl	52(%esp), %ebp                  # 4-byte Folded Reload
	movl	40(%esp), %edx                  # 4-byte Reload
	adcl	56(%esp), %edx                  # 4-byte Folded Reload
	movl	4(%esp), %edi                   # 4-byte Reload
	adcl	$0, %edi
	addl	8(%esp), %ecx                   # 4-byte Folded Reload
	movl	%ecx, 4(%esp)                   # 4-byte Spill
	movl	132(%esp), %ecx
	movl	4(%esp), %esi                   # 4-byte Reload
	movl	%esi, 20(%ecx)
	movl	20(%esp), %ecx                  # 4-byte Reload
	adcl	44(%esp), %ecx                  # 4-byte Folded Reload
	movl	%ecx, 20(%esp)                  # 4-byte Spill
	adcl	12(%esp), %ebx                  # 4-byte Folded Reload
	movl	%ebx, 12(%esp)                  # 4-byte Spill
	adcl	24(%esp), %eax                  # 4-byte Folded Reload
	movl	%eax, 24(%esp)                  # 4-byte Spill
	movl	(%esp), %eax                    # 4-byte Reload
	adcl	16(%esp), %eax                  # 4-byte Folded Reload
	movl	%eax, (%esp)                    # 4-byte Spill
	adcl	28(%esp), %ebp                  # 4-byte Folded Reload
	movl	%ebp, 36(%esp)                  # 4-byte Spill
	adcl	32(%esp), %edx                  # 4-byte Folded Reload
	movl	%edx, 40(%esp)                  # 4-byte Spill
	adcl	$0, %edi
	movl	%edi, 4(%esp)                   # 4-byte Spill
	movl	140(%esp), %eax
	movl	24(%eax), %edi
	movl	%edi, %eax
	mull	72(%esp)                        # 4-byte Folded Reload
	movl	%edx, 72(%esp)                  # 4-byte Spill
	movl	%eax, 32(%esp)                  # 4-byte Spill
	movl	%edi, %eax
	mull	60(%esp)                        # 4-byte Folded Reload
	movl	%edx, 60(%esp)                  # 4-byte Spill
	movl	%eax, 28(%esp)                  # 4-byte Spill
	movl	%edi, %eax
	mull	80(%esp)                        # 4-byte Folded Reload
	movl	%edx, %ebp
	movl	%eax, 80(%esp)                  # 4-byte Spill
	movl	%edi, %eax
	mull	64(%esp)                        # 4-byte Folded Reload
	movl	%edx, 64(%esp)                  # 4-byte Spill
	movl	%eax, 16(%esp)                  # 4-byte Spill
	movl	%edi, %eax
	mull	88(%esp)                        # 4-byte Folded Reload
	movl	%edx, %esi
	movl	%eax, %ebx
	movl	%edi, %eax
	mull	68(%esp)                        # 4-byte Folded Reload
	movl	%edx, %ecx
	movl	%eax, 68(%esp)                  # 4-byte Spill
	movl	%edi, %eax
	mull	84(%esp)                        # 4-byte Folded Reload
	addl	16(%esp), %esi                  # 4-byte Folded Reload
	adcl	64(%esp), %eax                  # 4-byte Folded Reload
	adcl	68(%esp), %edx                  # 4-byte Folded Reload
	adcl	80(%esp), %ecx                  # 4-byte Folded Reload
	adcl	28(%esp), %ebp                  # 4-byte Folded Reload
	movl	32(%esp), %edi                  # 4-byte Reload
	adcl	%edi, 60(%esp)                  # 4-byte Folded Spill
	adcl	$0, 72(%esp)                    # 4-byte Folded Spill
	addl	20(%esp), %ebx                  # 4-byte Folded Reload
	adcl	12(%esp), %esi                  # 4-byte Folded Reload
	movl	132(%esp), %edi
	movl	%ebx, 24(%edi)
	adcl	24(%esp), %eax                  # 4-byte Folded Reload
	movl	%esi, 28(%edi)
	adcl	(%esp), %edx                    # 4-byte Folded Reload
	movl	%eax, 32(%edi)
	adcl	36(%esp), %ecx                  # 4-byte Folded Reload
	movl	%edx, 36(%edi)
	adcl	40(%esp), %ebp                  # 4-byte Folded Reload
	movl	%ecx, 40(%edi)
	movl	%ebp, 44(%edi)
	movl	60(%esp), %eax                  # 4-byte Reload
	adcl	4(%esp), %eax                   # 4-byte Folded Reload
	movl	%eax, 48(%edi)
	movl	72(%esp), %eax                  # 4-byte Reload
	adcl	$0, %eax
	movl	%eax, 52(%edi)
	addl	$112, %esp
	popl	%esi
	popl	%edi
	popl	%ebx
	popl	%ebp
	retl
.Lfunc_end24:
	.size	mcl_fpDbl_mulPre7L, .Lfunc_end24-mcl_fpDbl_mulPre7L
                                        # -- End function
	.globl	mcl_fpDbl_sqrPre7L              # -- Begin function mcl_fpDbl_sqrPre7L
	.p2align	4, 0x90
	.type	mcl_fpDbl_sqrPre7L,@function
mcl_fpDbl_sqrPre7L:                     # @mcl_fpDbl_sqrPre7L
# %bb.0:
	pushl	%ebp
	pushl	%ebx
	pushl	%edi
	pushl	%esi
	subl	$244, %esp
	movl	268(%esp), %edi
	movl	24(%edi), %eax
	movl	%eax, 16(%esp)                  # 4-byte Spill
	movl	(%edi), %ecx
	mull	%ecx
	movl	%eax, 216(%esp)                 # 4-byte Spill
	movl	%edx, 212(%esp)                 # 4-byte Spill
	movl	20(%edi), %eax
	movl	%eax, 12(%esp)                  # 4-byte Spill
	mull	%ecx
	movl	%eax, 208(%esp)                 # 4-byte Spill
	movl	%edx, 60(%esp)                  # 4-byte Spill
	movl	16(%edi), %ebp
	movl	%ebp, %eax
	movl	%ebp, 40(%esp)                  # 4-byte Spill
	mull	%ecx
	movl	%eax, 192(%esp)                 # 4-byte Spill
	movl	%edx, 56(%esp)                  # 4-byte Spill
	movl	12(%edi), %esi
	movl	%esi, %eax
	movl	%esi, 36(%esp)                  # 4-byte Spill
	mull	%ecx
	movl	%eax, 184(%esp)                 # 4-byte Spill
	movl	%edx, 20(%esp)                  # 4-byte Spill
	movl	4(%edi), %ebx
	movl	8(%edi), %edi
	movl	%edi, %eax
	mull	%ecx
	movl	%edx, 236(%esp)                 # 4-byte Spill
	movl	%eax, 160(%esp)                 # 4-byte Spill
	movl	16(%esp), %eax                  # 4-byte Reload
	mull	%ebx
	movl	%edx, 204(%esp)                 # 4-byte Spill
	movl	%eax, 200(%esp)                 # 4-byte Spill
	movl	12(%esp), %eax                  # 4-byte Reload
	mull	%ebx
	movl	%edx, 196(%esp)                 # 4-byte Spill
	movl	%eax, 188(%esp)                 # 4-byte Spill
	movl	%ebp, %eax
	mull	%ebx
	movl	%edx, 52(%esp)                  # 4-byte Spill
	movl	%eax, 180(%esp)                 # 4-byte Spill
	movl	%esi, %eax
	mull	%ebx
	movl	%edx, 48(%esp)                  # 4-byte Spill
	movl	%eax, 148(%esp)                 # 4-byte Spill
	movl	%edi, %eax
	mull	%ebx
	movl	%edx, 28(%esp)                  # 4-byte Spill
	movl	%eax, 24(%esp)                  # 4-byte Spill
	movl	%ebx, %eax
	mull	%ebx
	movl	%edx, 8(%esp)                   # 4-byte Spill
	movl	%eax, 44(%esp)                  # 4-byte Spill
	movl	%ebx, %eax
	mull	%ecx
	movl	%edx, 4(%esp)                   # 4-byte Spill
	movl	%eax, 140(%esp)                 # 4-byte Spill
	movl	%ecx, %eax
	mull	%ecx
	movl	%edx, (%esp)                    # 4-byte Spill
	movl	264(%esp), %ecx
	movl	%eax, (%ecx)
	movl	16(%esp), %esi                  # 4-byte Reload
	movl	%esi, %eax
	movl	12(%esp), %ebp                  # 4-byte Reload
	mull	%ebp
	movl	%edx, 164(%esp)                 # 4-byte Spill
	movl	%eax, 172(%esp)                 # 4-byte Spill
	movl	%esi, %eax
	movl	40(%esp), %ebx                  # 4-byte Reload
	mull	%ebx
	movl	%edx, 168(%esp)                 # 4-byte Spill
	movl	%eax, 156(%esp)                 # 4-byte Spill
	movl	%esi, %eax
	movl	36(%esp), %ecx                  # 4-byte Reload
	mull	%ecx
	movl	%edx, 88(%esp)                  # 4-byte Spill
	movl	%eax, 152(%esp)                 # 4-byte Spill
	movl	%esi, %eax
	mull	%edi
	movl	%edx, 84(%esp)                  # 4-byte Spill
	movl	%eax, 144(%esp)                 # 4-byte Spill
	movl	%esi, %eax
	mull	%esi
	movl	%edx, 240(%esp)                 # 4-byte Spill
	movl	%eax, 176(%esp)                 # 4-byte Spill
	movl	%ebp, %eax
	mull	%ebx
	movl	%edx, 136(%esp)                 # 4-byte Spill
	movl	%eax, 120(%esp)                 # 4-byte Spill
	movl	%ebp, %eax
	mull	%ecx
	movl	%edx, 16(%esp)                  # 4-byte Spill
	movl	%eax, 132(%esp)                 # 4-byte Spill
	movl	%ebp, %eax
	mull	%edi
	movl	%edx, 128(%esp)                 # 4-byte Spill
	movl	%eax, 124(%esp)                 # 4-byte Spill
	movl	%ebp, %eax
	mull	%ebp
	movl	%edx, 80(%esp)                  # 4-byte Spill
	movl	%eax, 76(%esp)                  # 4-byte Spill
	movl	%ebx, %eax
	mull	%ecx
	movl	%edx, 108(%esp)                 # 4-byte Spill
	movl	%eax, 116(%esp)                 # 4-byte Spill
	movl	%ebx, %eax
	mull	%edi
	movl	%edx, 12(%esp)                  # 4-byte Spill
	movl	%eax, 112(%esp)                 # 4-byte Spill
	movl	%ebx, %eax
	mull	%ebx
	movl	%edx, 40(%esp)                  # 4-byte Spill
	movl	%eax, 72(%esp)                  # 4-byte Spill
	movl	%ecx, %eax
	mull	%edi
	movl	%edx, 104(%esp)                 # 4-byte Spill
	movl	%eax, 100(%esp)                 # 4-byte Spill
	movl	%ecx, %eax
	mull	%ecx
	movl	%edx, 36(%esp)                  # 4-byte Spill
	movl	%eax, 68(%esp)                  # 4-byte Spill
	movl	%edi, %eax
	mull	%edi
	movl	%eax, 64(%esp)                  # 4-byte Spill
	movl	%edx, 32(%esp)                  # 4-byte Spill
	movl	4(%esp), %ecx                   # 4-byte Reload
	addl	%ecx, 44(%esp)                  # 4-byte Folded Spill
	movl	8(%esp), %eax                   # 4-byte Reload
	adcl	24(%esp), %eax                  # 4-byte Folded Reload
	movl	%eax, 8(%esp)                   # 4-byte Spill
	movl	28(%esp), %eax                  # 4-byte Reload
	adcl	148(%esp), %eax                 # 4-byte Folded Reload
	movl	%eax, 228(%esp)                 # 4-byte Spill
	movl	48(%esp), %eax                  # 4-byte Reload
	adcl	180(%esp), %eax                 # 4-byte Folded Reload
	movl	%eax, 224(%esp)                 # 4-byte Spill
	movl	52(%esp), %eax                  # 4-byte Reload
	adcl	188(%esp), %eax                 # 4-byte Folded Reload
	movl	%eax, 220(%esp)                 # 4-byte Spill
	movl	196(%esp), %eax                 # 4-byte Reload
	adcl	200(%esp), %eax                 # 4-byte Folded Reload
	movl	%eax, 232(%esp)                 # 4-byte Spill
	movl	204(%esp), %eax                 # 4-byte Reload
	adcl	$0, %eax
	movl	%eax, 96(%esp)                  # 4-byte Spill
	movl	(%esp), %eax                    # 4-byte Reload
	addl	140(%esp), %eax                 # 4-byte Folded Reload
	movl	%eax, (%esp)                    # 4-byte Spill
	adcl	160(%esp), %ecx                 # 4-byte Folded Reload
	movl	%ecx, 4(%esp)                   # 4-byte Spill
	movl	236(%esp), %ebx                 # 4-byte Reload
	movl	%ebx, %eax
	adcl	184(%esp), %eax                 # 4-byte Folded Reload
	movl	20(%esp), %ecx                  # 4-byte Reload
	adcl	192(%esp), %ecx                 # 4-byte Folded Reload
	movl	%ecx, 92(%esp)                  # 4-byte Spill
	movl	56(%esp), %ecx                  # 4-byte Reload
	adcl	208(%esp), %ecx                 # 4-byte Folded Reload
	movl	%ecx, %edx
	movl	60(%esp), %esi                  # 4-byte Reload
	adcl	216(%esp), %esi                 # 4-byte Folded Reload
	movl	212(%esp), %edi                 # 4-byte Reload
	adcl	$0, %edi
	movl	(%esp), %ebp                    # 4-byte Reload
	addl	140(%esp), %ebp                 # 4-byte Folded Reload
	movl	%ebp, (%esp)                    # 4-byte Spill
	movl	4(%esp), %ecx                   # 4-byte Reload
	adcl	44(%esp), %ecx                  # 4-byte Folded Reload
	movl	%ecx, 4(%esp)                   # 4-byte Spill
	movl	(%esp), %ebp                    # 4-byte Reload
	movl	264(%esp), %ecx
	movl	%ebp, 4(%ecx)
	adcl	8(%esp), %eax                   # 4-byte Folded Reload
	movl	228(%esp), %ecx                 # 4-byte Reload
	adcl	%ecx, 92(%esp)                  # 4-byte Folded Spill
	adcl	224(%esp), %edx                 # 4-byte Folded Reload
	movl	%edx, 44(%esp)                  # 4-byte Spill
	adcl	220(%esp), %esi                 # 4-byte Folded Reload
	adcl	232(%esp), %edi                 # 4-byte Folded Reload
	adcl	$0, 96(%esp)                    # 4-byte Folded Spill
	addl	24(%esp), %ebx                  # 4-byte Folded Reload
	movl	64(%esp), %ebp                  # 4-byte Reload
	adcl	28(%esp), %ebp                  # 4-byte Folded Reload
	movl	32(%esp), %ecx                  # 4-byte Reload
	adcl	100(%esp), %ecx                 # 4-byte Folded Reload
	movl	104(%esp), %edx                 # 4-byte Reload
	adcl	112(%esp), %edx                 # 4-byte Folded Reload
	movl	%edx, 24(%esp)                  # 4-byte Spill
	movl	12(%esp), %edx                  # 4-byte Reload
	adcl	124(%esp), %edx                 # 4-byte Folded Reload
	movl	%edx, 28(%esp)                  # 4-byte Spill
	movl	128(%esp), %edx                 # 4-byte Reload
	adcl	144(%esp), %edx                 # 4-byte Folded Reload
	movl	%edx, 8(%esp)                   # 4-byte Spill
	movl	84(%esp), %edx                  # 4-byte Reload
	movl	%edx, (%esp)                    # 4-byte Spill
	adcl	$0, (%esp)                      # 4-byte Folded Spill
	movl	4(%esp), %edx                   # 4-byte Reload
	addl	160(%esp), %edx                 # 4-byte Folded Reload
	adcl	%eax, %ebx
	movl	264(%esp), %eax
	movl	%edx, 8(%eax)
	adcl	92(%esp), %ebp                  # 4-byte Folded Reload
	movl	%ebp, 64(%esp)                  # 4-byte Spill
	adcl	44(%esp), %ecx                  # 4-byte Folded Reload
	movl	%ecx, 32(%esp)                  # 4-byte Spill
	adcl	%esi, 24(%esp)                  # 4-byte Folded Spill
	adcl	%edi, 28(%esp)                  # 4-byte Folded Spill
	movl	8(%esp), %eax                   # 4-byte Reload
	adcl	96(%esp), %eax                  # 4-byte Folded Reload
	movl	%eax, 8(%esp)                   # 4-byte Spill
	adcl	$0, (%esp)                      # 4-byte Folded Spill
	movl	20(%esp), %edx                  # 4-byte Reload
	addl	148(%esp), %edx                 # 4-byte Folded Reload
	movl	48(%esp), %esi                  # 4-byte Reload
	adcl	100(%esp), %esi                 # 4-byte Folded Reload
	movl	68(%esp), %ebp                  # 4-byte Reload
	adcl	104(%esp), %ebp                 # 4-byte Folded Reload
	movl	36(%esp), %ecx                  # 4-byte Reload
	adcl	116(%esp), %ecx                 # 4-byte Folded Reload
	movl	108(%esp), %eax                 # 4-byte Reload
	adcl	132(%esp), %eax                 # 4-byte Folded Reload
	movl	%eax, %edi
	movl	16(%esp), %eax                  # 4-byte Reload
	adcl	152(%esp), %eax                 # 4-byte Folded Reload
	movl	%eax, 4(%esp)                   # 4-byte Spill
	movl	88(%esp), %eax                  # 4-byte Reload
	adcl	$0, %eax
	addl	184(%esp), %ebx                 # 4-byte Folded Reload
	adcl	64(%esp), %edx                  # 4-byte Folded Reload
	movl	%edx, 20(%esp)                  # 4-byte Spill
	movl	264(%esp), %edx
	movl	%ebx, 12(%edx)
	adcl	32(%esp), %esi                  # 4-byte Folded Reload
	movl	%esi, 48(%esp)                  # 4-byte Spill
	adcl	24(%esp), %ebp                  # 4-byte Folded Reload
	movl	%ebp, 68(%esp)                  # 4-byte Spill
	adcl	28(%esp), %ecx                  # 4-byte Folded Reload
	movl	%ecx, 36(%esp)                  # 4-byte Spill
	adcl	8(%esp), %edi                   # 4-byte Folded Reload
	movl	%edi, 32(%esp)                  # 4-byte Spill
	movl	(%esp), %ecx                    # 4-byte Reload
	adcl	%ecx, 4(%esp)                   # 4-byte Folded Spill
	adcl	$0, %eax
	movl	%eax, 8(%esp)                   # 4-byte Spill
	movl	56(%esp), %ebx                  # 4-byte Reload
	addl	180(%esp), %ebx                 # 4-byte Folded Reload
	movl	52(%esp), %ebp                  # 4-byte Reload
	adcl	112(%esp), %ebp                 # 4-byte Folded Reload
	movl	12(%esp), %edi                  # 4-byte Reload
	adcl	116(%esp), %edi                 # 4-byte Folded Reload
	movl	72(%esp), %edx                  # 4-byte Reload
	adcl	108(%esp), %edx                 # 4-byte Folded Reload
	movl	40(%esp), %ecx                  # 4-byte Reload
	adcl	120(%esp), %ecx                 # 4-byte Folded Reload
	movl	136(%esp), %esi                 # 4-byte Reload
	adcl	156(%esp), %esi                 # 4-byte Folded Reload
	movl	%esi, (%esp)                    # 4-byte Spill
	movl	168(%esp), %esi                 # 4-byte Reload
	adcl	$0, %esi
	movl	20(%esp), %eax                  # 4-byte Reload
	addl	192(%esp), %eax                 # 4-byte Folded Reload
	adcl	48(%esp), %ebx                  # 4-byte Folded Reload
	movl	%ebx, 56(%esp)                  # 4-byte Spill
	movl	264(%esp), %ebx
	movl	%eax, 16(%ebx)
	adcl	68(%esp), %ebp                  # 4-byte Folded Reload
	movl	%ebp, 52(%esp)                  # 4-byte Spill
	adcl	36(%esp), %edi                  # 4-byte Folded Reload
	movl	%edi, 12(%esp)                  # 4-byte Spill
	adcl	32(%esp), %edx                  # 4-byte Folded Reload
	movl	%edx, 72(%esp)                  # 4-byte Spill
	adcl	4(%esp), %ecx                   # 4-byte Folded Reload
	movl	%ecx, 40(%esp)                  # 4-byte Spill
	movl	8(%esp), %eax                   # 4-byte Reload
	adcl	%eax, (%esp)                    # 4-byte Folded Spill
	adcl	$0, %esi
	movl	%esi, 20(%esp)                  # 4-byte Spill
	movl	60(%esp), %edi                  # 4-byte Reload
	addl	188(%esp), %edi                 # 4-byte Folded Reload
	movl	196(%esp), %ecx                 # 4-byte Reload
	adcl	124(%esp), %ecx                 # 4-byte Folded Reload
	movl	128(%esp), %ebx                 # 4-byte Reload
	adcl	132(%esp), %ebx                 # 4-byte Folded Reload
	movl	16(%esp), %ebp                  # 4-byte Reload
	adcl	120(%esp), %ebp                 # 4-byte Folded Reload
	movl	76(%esp), %esi                  # 4-byte Reload
	adcl	136(%esp), %esi                 # 4-byte Folded Reload
	movl	80(%esp), %edx                  # 4-byte Reload
	adcl	172(%esp), %edx                 # 4-byte Folded Reload
	movl	164(%esp), %eax                 # 4-byte Reload
	adcl	$0, %eax
	movl	%eax, 4(%esp)                   # 4-byte Spill
	movl	56(%esp), %eax                  # 4-byte Reload
	addl	208(%esp), %eax                 # 4-byte Folded Reload
	adcl	52(%esp), %edi                  # 4-byte Folded Reload
	movl	%edi, 60(%esp)                  # 4-byte Spill
	movl	264(%esp), %edi
	movl	%eax, 20(%edi)
	adcl	12(%esp), %ecx                  # 4-byte Folded Reload
	movl	%ecx, %edi
	adcl	72(%esp), %ebx                  # 4-byte Folded Reload
	adcl	40(%esp), %ebp                  # 4-byte Folded Reload
	movl	%ebp, 16(%esp)                  # 4-byte Spill
	adcl	(%esp), %esi                    # 4-byte Folded Reload
	movl	%esi, 76(%esp)                  # 4-byte Spill
	adcl	20(%esp), %edx                  # 4-byte Folded Reload
	movl	%edx, 80(%esp)                  # 4-byte Spill
	adcl	$0, 4(%esp)                     # 4-byte Folded Spill
	movl	212(%esp), %ecx                 # 4-byte Reload
	addl	200(%esp), %ecx                 # 4-byte Folded Reload
	movl	204(%esp), %esi                 # 4-byte Reload
	adcl	144(%esp), %esi                 # 4-byte Folded Reload
	movl	152(%esp), %eax                 # 4-byte Reload
	adcl	%eax, 84(%esp)                  # 4-byte Folded Spill
	movl	156(%esp), %eax                 # 4-byte Reload
	adcl	%eax, 88(%esp)                  # 4-byte Folded Spill
	movl	168(%esp), %ebp                 # 4-byte Reload
	adcl	172(%esp), %ebp                 # 4-byte Folded Reload
	movl	164(%esp), %eax                 # 4-byte Reload
	adcl	%eax, 176(%esp)                 # 4-byte Folded Spill
	movl	240(%esp), %eax                 # 4-byte Reload
	adcl	$0, %eax
	movl	60(%esp), %edx                  # 4-byte Reload
	addl	216(%esp), %edx                 # 4-byte Folded Reload
	adcl	%edi, %ecx
	movl	264(%esp), %edi
	movl	%edx, 24(%edi)
	movl	%esi, %edx
	adcl	%ebx, %edx
	movl	%ecx, 28(%edi)
	movl	84(%esp), %ecx                  # 4-byte Reload
	adcl	16(%esp), %ecx                  # 4-byte Folded Reload
	movl	%edx, 32(%edi)
	movl	88(%esp), %edx                  # 4-byte Reload
	adcl	76(%esp), %edx                  # 4-byte Folded Reload
	movl	%ecx, 36(%edi)
	movl	%ebp, %ecx
	adcl	80(%esp), %ecx                  # 4-byte Folded Reload
	movl	%edx, 40(%edi)
	movl	%ecx, 44(%edi)
	movl	176(%esp), %ecx                 # 4-byte Reload
	adcl	4(%esp), %ecx                   # 4-byte Folded Reload
	movl	%ecx, 48(%edi)
	adcl	$0, %eax
	movl	%eax, 52(%edi)
	addl	$244, %esp
	popl	%esi
	popl	%edi
	popl	%ebx
	popl	%ebp
	retl
.Lfunc_end25:
	.size	mcl_fpDbl_sqrPre7L, .Lfunc_end25-mcl_fpDbl_sqrPre7L
                                        # -- End function
	.globl	mcl_fp_mont7L                   # -- Begin function mcl_fp_mont7L
	.p2align	4, 0x90
	.type	mcl_fp_mont7L,@function
mcl_fp_mont7L:                          # @mcl_fp_mont7L
# %bb.0:
	pushl	%ebp
	pushl	%ebx
	pushl	%edi
	pushl	%esi
	subl	$152, %esp
	movl	176(%esp), %edi
	movl	(%edi), %eax
	movl	%eax, 84(%esp)                  # 4-byte Spill
	movl	180(%esp), %ecx
	movl	(%ecx), %esi
	mull	%esi
	movl	%eax, 24(%esp)                  # 4-byte Spill
	movl	%edx, 36(%esp)                  # 4-byte Spill
	movl	184(%esp), %ebx
	movl	-4(%ebx), %ecx
	movl	%ecx, 92(%esp)                  # 4-byte Spill
	imull	%eax, %ecx
	movl	24(%ebx), %edx
	movl	%edx, 132(%esp)                 # 4-byte Spill
	movl	%ecx, %eax
	mull	%edx
	movl	%eax, 20(%esp)                  # 4-byte Spill
	movl	%edx, 12(%esp)                  # 4-byte Spill
	movl	20(%ebx), %edx
	movl	%edx, 128(%esp)                 # 4-byte Spill
	movl	%ecx, %eax
	mull	%edx
	movl	%eax, 64(%esp)                  # 4-byte Spill
	movl	%edx, 32(%esp)                  # 4-byte Spill
	movl	16(%ebx), %edx
	movl	%edx, 124(%esp)                 # 4-byte Spill
	movl	%ecx, %eax
	mull	%edx
	movl	%eax, 48(%esp)                  # 4-byte Spill
	movl	%edx, 28(%esp)                  # 4-byte Spill
	movl	12(%ebx), %edx
	movl	%edx, 120(%esp)                 # 4-byte Spill
	movl	%ecx, %eax
	mull	%edx
	movl	%eax, 44(%esp)                  # 4-byte Spill
	movl	%edx, 4(%esp)                   # 4-byte Spill
	movl	8(%ebx), %edx
	movl	%edx, 112(%esp)                 # 4-byte Spill
	movl	%ecx, %eax
	mull	%edx
	movl	%eax, 56(%esp)                  # 4-byte Spill
	movl	%edx, 16(%esp)                  # 4-byte Spill
	movl	(%ebx), %ebp
	movl	%ebp, 108(%esp)                 # 4-byte Spill
	movl	4(%ebx), %edx
	movl	%edx, 116(%esp)                 # 4-byte Spill
	movl	%ecx, %eax
	mull	%edx
	movl	%edx, 52(%esp)                  # 4-byte Spill
	movl	%eax, 68(%esp)                  # 4-byte Spill
	movl	%ecx, %eax
	mull	%ebp
	movl	%eax, 60(%esp)                  # 4-byte Spill
	movl	%edx, 40(%esp)                  # 4-byte Spill
	movl	%edi, %ebx
	movl	24(%edi), %eax
	movl	%eax, 80(%esp)                  # 4-byte Spill
	mull	%esi
	movl	%eax, 88(%esp)                  # 4-byte Spill
	movl	%edx, 8(%esp)                   # 4-byte Spill
	movl	20(%edi), %eax
	movl	%eax, 76(%esp)                  # 4-byte Spill
	mull	%esi
	movl	%eax, 148(%esp)                 # 4-byte Spill
	movl	%edx, %edi
	movl	16(%ebx), %eax
	movl	%eax, 100(%esp)                 # 4-byte Spill
	mull	%esi
	movl	%eax, 144(%esp)                 # 4-byte Spill
	movl	%edx, (%esp)                    # 4-byte Spill
	movl	12(%ebx), %eax
	movl	%eax, 72(%esp)                  # 4-byte Spill
	mull	%esi
	movl	%eax, 140(%esp)                 # 4-byte Spill
	movl	%edx, %ebp
	movl	4(%ebx), %ecx
	movl	%ecx, 96(%esp)                  # 4-byte Spill
	movl	8(%ebx), %eax
	movl	%eax, 104(%esp)                 # 4-byte Spill
	mull	%esi
	movl	%edx, %ebx
	movl	%eax, 136(%esp)                 # 4-byte Spill
	movl	%ecx, %eax
	mull	%esi
	addl	36(%esp), %eax                  # 4-byte Folded Reload
	movl	%eax, 36(%esp)                  # 4-byte Spill
	adcl	136(%esp), %edx                 # 4-byte Folded Reload
	movl	%edx, 136(%esp)                 # 4-byte Spill
	adcl	140(%esp), %ebx                 # 4-byte Folded Reload
	movl	%ebx, 140(%esp)                 # 4-byte Spill
	adcl	144(%esp), %ebp                 # 4-byte Folded Reload
	movl	%ebp, 144(%esp)                 # 4-byte Spill
	movl	148(%esp), %eax                 # 4-byte Reload
	adcl	%eax, (%esp)                    # 4-byte Folded Spill
	adcl	88(%esp), %edi                  # 4-byte Folded Reload
	movl	%edi, 88(%esp)                  # 4-byte Spill
	adcl	$0, 8(%esp)                     # 4-byte Folded Spill
	movl	40(%esp), %ebp                  # 4-byte Reload
	addl	68(%esp), %ebp                  # 4-byte Folded Reload
	movl	52(%esp), %edi                  # 4-byte Reload
	adcl	56(%esp), %edi                  # 4-byte Folded Reload
	movl	16(%esp), %edx                  # 4-byte Reload
	adcl	44(%esp), %edx                  # 4-byte Folded Reload
	movl	4(%esp), %eax                   # 4-byte Reload
	adcl	48(%esp), %eax                  # 4-byte Folded Reload
	movl	%eax, 4(%esp)                   # 4-byte Spill
	movl	28(%esp), %ebx                  # 4-byte Reload
	adcl	64(%esp), %ebx                  # 4-byte Folded Reload
	movl	32(%esp), %esi                  # 4-byte Reload
	adcl	20(%esp), %esi                  # 4-byte Folded Reload
	movl	12(%esp), %ecx                  # 4-byte Reload
	adcl	$0, %ecx
	movl	60(%esp), %eax                  # 4-byte Reload
	addl	24(%esp), %eax                  # 4-byte Folded Reload
	adcl	36(%esp), %ebp                  # 4-byte Folded Reload
	movl	%ebp, 40(%esp)                  # 4-byte Spill
	adcl	136(%esp), %edi                 # 4-byte Folded Reload
	movl	%edi, 52(%esp)                  # 4-byte Spill
	adcl	140(%esp), %edx                 # 4-byte Folded Reload
	movl	%edx, 16(%esp)                  # 4-byte Spill
	movl	4(%esp), %eax                   # 4-byte Reload
	adcl	144(%esp), %eax                 # 4-byte Folded Reload
	movl	%eax, 4(%esp)                   # 4-byte Spill
	adcl	(%esp), %ebx                    # 4-byte Folded Reload
	movl	%ebx, 28(%esp)                  # 4-byte Spill
	adcl	88(%esp), %esi                  # 4-byte Folded Reload
	movl	%esi, 32(%esp)                  # 4-byte Spill
	adcl	8(%esp), %ecx                   # 4-byte Folded Reload
	movl	%ecx, 12(%esp)                  # 4-byte Spill
	setb	64(%esp)                        # 1-byte Folded Spill
	movl	180(%esp), %eax
	movl	4(%eax), %edi
	movl	%edi, %eax
	mull	80(%esp)                        # 4-byte Folded Reload
	movl	%edx, (%esp)                    # 4-byte Spill
	movl	%eax, 36(%esp)                  # 4-byte Spill
	movl	%edi, %eax
	mull	76(%esp)                        # 4-byte Folded Reload
	movl	%edx, %ebx
	movl	%eax, 44(%esp)                  # 4-byte Spill
	movl	%edi, %eax
	mull	100(%esp)                       # 4-byte Folded Reload
	movl	%edx, 8(%esp)                   # 4-byte Spill
	movl	%eax, 60(%esp)                  # 4-byte Spill
	movl	%edi, %eax
	mull	72(%esp)                        # 4-byte Folded Reload
	movl	%edx, 24(%esp)                  # 4-byte Spill
	movl	%eax, 56(%esp)                  # 4-byte Spill
	movl	%edi, %eax
	mull	104(%esp)                       # 4-byte Folded Reload
	movl	%edx, 20(%esp)                  # 4-byte Spill
	movl	%eax, %ecx
	movl	%edi, %eax
	mull	96(%esp)                        # 4-byte Folded Reload
	movl	%edx, %esi
	movl	%eax, %ebp
	movl	%edi, %eax
	mull	84(%esp)                        # 4-byte Folded Reload
	addl	%ebp, %edx
	movl	%edx, 48(%esp)                  # 4-byte Spill
	adcl	%ecx, %esi
	movl	%esi, %ebp
	movl	20(%esp), %edi                  # 4-byte Reload
	adcl	56(%esp), %edi                  # 4-byte Folded Reload
	movl	24(%esp), %esi                  # 4-byte Reload
	adcl	60(%esp), %esi                  # 4-byte Folded Reload
	movl	8(%esp), %edx                   # 4-byte Reload
	adcl	44(%esp), %edx                  # 4-byte Folded Reload
	adcl	36(%esp), %ebx                  # 4-byte Folded Reload
	movl	(%esp), %ecx                    # 4-byte Reload
	adcl	$0, %ecx
	addl	40(%esp), %eax                  # 4-byte Folded Reload
	movl	%eax, 36(%esp)                  # 4-byte Spill
	movl	52(%esp), %eax                  # 4-byte Reload
	adcl	%eax, 48(%esp)                  # 4-byte Folded Spill
	adcl	16(%esp), %ebp                  # 4-byte Folded Reload
	movl	%ebp, 44(%esp)                  # 4-byte Spill
	adcl	4(%esp), %edi                   # 4-byte Folded Reload
	movl	%edi, 20(%esp)                  # 4-byte Spill
	adcl	28(%esp), %esi                  # 4-byte Folded Reload
	movl	%esi, 24(%esp)                  # 4-byte Spill
	adcl	32(%esp), %edx                  # 4-byte Folded Reload
	movl	%edx, 8(%esp)                   # 4-byte Spill
	adcl	12(%esp), %ebx                  # 4-byte Folded Reload
	movl	%ebx, 12(%esp)                  # 4-byte Spill
	movzbl	64(%esp), %eax                  # 1-byte Folded Reload
	adcl	%eax, %ecx
	movl	%ecx, (%esp)                    # 4-byte Spill
	setb	32(%esp)                        # 1-byte Folded Spill
	movl	92(%esp), %ebp                  # 4-byte Reload
	imull	36(%esp), %ebp                  # 4-byte Folded Reload
	movl	%ebp, %eax
	mull	132(%esp)                       # 4-byte Folded Reload
	movl	%edx, 16(%esp)                  # 4-byte Spill
	movl	%eax, 28(%esp)                  # 4-byte Spill
	movl	%ebp, %eax
	mull	128(%esp)                       # 4-byte Folded Reload
	movl	%edx, 52(%esp)                  # 4-byte Spill
	movl	%eax, 60(%esp)                  # 4-byte Spill
	movl	%ebp, %eax
	mull	124(%esp)                       # 4-byte Folded Reload
	movl	%edx, 40(%esp)                  # 4-byte Spill
	movl	%eax, 56(%esp)                  # 4-byte Spill
	movl	%ebp, %eax
	mull	120(%esp)                       # 4-byte Folded Reload
	movl	%edx, 64(%esp)                  # 4-byte Spill
	movl	%eax, 68(%esp)                  # 4-byte Spill
	movl	%ebp, %eax
	mull	112(%esp)                       # 4-byte Folded Reload
	movl	%edx, %esi
	movl	%eax, 88(%esp)                  # 4-byte Spill
	movl	%ebp, %eax
	mull	116(%esp)                       # 4-byte Folded Reload
	movl	%edx, %ebx
	movl	%eax, %ecx
	movl	%ebp, %eax
	mull	108(%esp)                       # 4-byte Folded Reload
	movl	%eax, %edi
	addl	%ecx, %edx
	movl	%edx, 4(%esp)                   # 4-byte Spill
	adcl	88(%esp), %ebx                  # 4-byte Folded Reload
	adcl	68(%esp), %esi                  # 4-byte Folded Reload
	movl	%esi, %ebp
	movl	64(%esp), %esi                  # 4-byte Reload
	adcl	56(%esp), %esi                  # 4-byte Folded Reload
	movl	40(%esp), %eax                  # 4-byte Reload
	adcl	60(%esp), %eax                  # 4-byte Folded Reload
	movl	52(%esp), %edx                  # 4-byte Reload
	adcl	28(%esp), %edx                  # 4-byte Folded Reload
	movl	16(%esp), %ecx                  # 4-byte Reload
	adcl	$0, %ecx
	addl	36(%esp), %edi                  # 4-byte Folded Reload
	movl	4(%esp), %edi                   # 4-byte Reload
	adcl	48(%esp), %edi                  # 4-byte Folded Reload
	movl	%edi, 4(%esp)                   # 4-byte Spill
	adcl	44(%esp), %ebx                  # 4-byte Folded Reload
	movl	%ebx, 60(%esp)                  # 4-byte Spill
	adcl	20(%esp), %ebp                  # 4-byte Folded Reload
	movl	%ebp, 20(%esp)                  # 4-byte Spill
	adcl	24(%esp), %esi                  # 4-byte Folded Reload
	movl	%esi, 64(%esp)                  # 4-byte Spill
	adcl	8(%esp), %eax                   # 4-byte Folded Reload
	movl	%eax, 40(%esp)                  # 4-byte Spill
	adcl	12(%esp), %edx                  # 4-byte Folded Reload
	movl	%edx, 52(%esp)                  # 4-byte Spill
	adcl	(%esp), %ecx                    # 4-byte Folded Reload
	movl	%ecx, 16(%esp)                  # 4-byte Spill
	movzbl	32(%esp), %eax                  # 1-byte Folded Reload
	adcl	$0, %eax
	movl	%eax, 8(%esp)                   # 4-byte Spill
	movl	180(%esp), %eax
	movl	8(%eax), %ecx
	movl	%ecx, %eax
	mull	80(%esp)                        # 4-byte Folded Reload
	movl	%edx, 12(%esp)                  # 4-byte Spill
	movl	%eax, 24(%esp)                  # 4-byte Spill
	movl	%ecx, %eax
	mull	76(%esp)                        # 4-byte Folded Reload
	movl	%edx, 32(%esp)                  # 4-byte Spill
	movl	%eax, 56(%esp)                  # 4-byte Spill
	movl	%ecx, %eax
	mull	100(%esp)                       # 4-byte Folded Reload
	movl	%edx, 28(%esp)                  # 4-byte Spill
	movl	%eax, 68(%esp)                  # 4-byte Spill
	movl	%ecx, %eax
	mull	72(%esp)                        # 4-byte Folded Reload
	movl	%edx, (%esp)                    # 4-byte Spill
	movl	%eax, 44(%esp)                  # 4-byte Spill
	movl	%ecx, %eax
	mull	104(%esp)                       # 4-byte Folded Reload
	movl	%edx, %edi
	movl	%eax, %ebp
	movl	%ecx, %eax
	mull	96(%esp)                        # 4-byte Folded Reload
	movl	%edx, %ebx
	movl	%eax, %esi
	movl	%ecx, %eax
	mull	84(%esp)                        # 4-byte Folded Reload
	movl	%eax, 48(%esp)                  # 4-byte Spill
	addl	%esi, %edx
	movl	%edx, 36(%esp)                  # 4-byte Spill
	adcl	%ebp, %ebx
	movl	%ebx, %ebp
	adcl	44(%esp), %edi                  # 4-byte Folded Reload
	movl	%edi, 44(%esp)                  # 4-byte Spill
	movl	(%esp), %esi                    # 4-byte Reload
	adcl	68(%esp), %esi                  # 4-byte Folded Reload
	movl	28(%esp), %edx                  # 4-byte Reload
	adcl	56(%esp), %edx                  # 4-byte Folded Reload
	movl	32(%esp), %ecx                  # 4-byte Reload
	adcl	24(%esp), %ecx                  # 4-byte Folded Reload
	movl	12(%esp), %eax                  # 4-byte Reload
	adcl	$0, %eax
	movl	48(%esp), %edi                  # 4-byte Reload
	addl	4(%esp), %edi                   # 4-byte Folded Reload
	movl	%edi, 48(%esp)                  # 4-byte Spill
	movl	60(%esp), %ebx                  # 4-byte Reload
	adcl	%ebx, 36(%esp)                  # 4-byte Folded Spill
	adcl	20(%esp), %ebp                  # 4-byte Folded Reload
	movl	%ebp, 60(%esp)                  # 4-byte Spill
	movl	64(%esp), %ebx                  # 4-byte Reload
	adcl	%ebx, 44(%esp)                  # 4-byte Folded Spill
	adcl	40(%esp), %esi                  # 4-byte Folded Reload
	movl	%esi, (%esp)                    # 4-byte Spill
	adcl	52(%esp), %edx                  # 4-byte Folded Reload
	movl	%edx, 28(%esp)                  # 4-byte Spill
	adcl	16(%esp), %ecx                  # 4-byte Folded Reload
	movl	%ecx, 32(%esp)                  # 4-byte Spill
	adcl	8(%esp), %eax                   # 4-byte Folded Reload
	movl	%eax, 12(%esp)                  # 4-byte Spill
	setb	40(%esp)                        # 1-byte Folded Spill
	movl	92(%esp), %esi                  # 4-byte Reload
	imull	%edi, %esi
	movl	%esi, %eax
	mull	132(%esp)                       # 4-byte Folded Reload
	movl	%edx, 24(%esp)                  # 4-byte Spill
	movl	%eax, 64(%esp)                  # 4-byte Spill
	movl	%esi, %eax
	mull	128(%esp)                       # 4-byte Folded Reload
	movl	%edx, 20(%esp)                  # 4-byte Spill
	movl	%eax, 56(%esp)                  # 4-byte Spill
	movl	%esi, %eax
	mull	124(%esp)                       # 4-byte Folded Reload
	movl	%edx, 16(%esp)                  # 4-byte Spill
	movl	%eax, 68(%esp)                  # 4-byte Spill
	movl	%esi, %eax
	mull	120(%esp)                       # 4-byte Folded Reload
	movl	%edx, 52(%esp)                  # 4-byte Spill
	movl	%eax, 88(%esp)                  # 4-byte Spill
	movl	%esi, %eax
	mull	112(%esp)                       # 4-byte Folded Reload
	movl	%edx, 8(%esp)                   # 4-byte Spill
	movl	%eax, %ebp
	movl	%esi, %eax
	mull	116(%esp)                       # 4-byte Folded Reload
	movl	%edx, %edi
	movl	%eax, %ecx
	movl	%esi, %eax
	mull	108(%esp)                       # 4-byte Folded Reload
	movl	%eax, %ebx
	addl	%ecx, %edx
	movl	%edx, 4(%esp)                   # 4-byte Spill
	adcl	%ebp, %edi
	movl	%edi, %ebp
	movl	8(%esp), %eax                   # 4-byte Reload
	adcl	88(%esp), %eax                  # 4-byte Folded Reload
	movl	52(%esp), %edi                  # 4-byte Reload
	adcl	68(%esp), %edi                  # 4-byte Folded Reload
	movl	16(%esp), %esi                  # 4-byte Reload
	adcl	56(%esp), %esi                  # 4-byte Folded Reload
	movl	20(%esp), %edx                  # 4-byte Reload
	adcl	64(%esp), %edx                  # 4-byte Folded Reload
	movl	24(%esp), %ecx                  # 4-byte Reload
	adcl	$0, %ecx
	addl	48(%esp), %ebx                  # 4-byte Folded Reload
	movl	4(%esp), %ebx                   # 4-byte Reload
	adcl	36(%esp), %ebx                  # 4-byte Folded Reload
	movl	%ebx, 4(%esp)                   # 4-byte Spill
	adcl	60(%esp), %ebp                  # 4-byte Folded Reload
	movl	%ebp, 36(%esp)                  # 4-byte Spill
	adcl	44(%esp), %eax                  # 4-byte Folded Reload
	movl	%eax, 8(%esp)                   # 4-byte Spill
	adcl	(%esp), %edi                    # 4-byte Folded Reload
	movl	%edi, 52(%esp)                  # 4-byte Spill
	adcl	28(%esp), %esi                  # 4-byte Folded Reload
	movl	%esi, 16(%esp)                  # 4-byte Spill
	adcl	32(%esp), %edx                  # 4-byte Folded Reload
	movl	%edx, 20(%esp)                  # 4-byte Spill
	adcl	12(%esp), %ecx                  # 4-byte Folded Reload
	movl	%ecx, 24(%esp)                  # 4-byte Spill
	movzbl	40(%esp), %eax                  # 1-byte Folded Reload
	adcl	$0, %eax
	movl	%eax, 60(%esp)                  # 4-byte Spill
	movl	180(%esp), %eax
	movl	12(%eax), %ecx
	movl	%ecx, %eax
	mull	80(%esp)                        # 4-byte Folded Reload
	movl	%edx, 12(%esp)                  # 4-byte Spill
	movl	%eax, 44(%esp)                  # 4-byte Spill
	movl	%ecx, %eax
	mull	76(%esp)                        # 4-byte Folded Reload
	movl	%edx, 32(%esp)                  # 4-byte Spill
	movl	%eax, 56(%esp)                  # 4-byte Spill
	movl	%ecx, %eax
	mull	100(%esp)                       # 4-byte Folded Reload
	movl	%edx, 28(%esp)                  # 4-byte Spill
	movl	%eax, 68(%esp)                  # 4-byte Spill
	movl	%ecx, %eax
	mull	72(%esp)                        # 4-byte Folded Reload
	movl	%edx, (%esp)                    # 4-byte Spill
	movl	%eax, 48(%esp)                  # 4-byte Spill
	movl	%ecx, %eax
	mull	104(%esp)                       # 4-byte Folded Reload
	movl	%edx, %edi
	movl	%eax, %ebp
	movl	%ecx, %eax
	mull	96(%esp)                        # 4-byte Folded Reload
	movl	%edx, %ebx
	movl	%eax, %esi
	movl	%ecx, %eax
	mull	84(%esp)                        # 4-byte Folded Reload
	movl	%eax, 40(%esp)                  # 4-byte Spill
	addl	%esi, %edx
	movl	%edx, 64(%esp)                  # 4-byte Spill
	adcl	%ebp, %ebx
	movl	%ebx, %ebp
	adcl	48(%esp), %edi                  # 4-byte Folded Reload
	movl	%edi, 48(%esp)                  # 4-byte Spill
	movl	(%esp), %esi                    # 4-byte Reload
	adcl	68(%esp), %esi                  # 4-byte Folded Reload
	movl	28(%esp), %edx                  # 4-byte Reload
	adcl	56(%esp), %edx                  # 4-byte Folded Reload
	movl	32(%esp), %ecx                  # 4-byte Reload
	adcl	44(%esp), %ecx                  # 4-byte Folded Reload
	movl	12(%esp), %eax                  # 4-byte Reload
	adcl	$0, %eax
	movl	40(%esp), %edi                  # 4-byte Reload
	addl	4(%esp), %edi                   # 4-byte Folded Reload
	movl	%edi, 40(%esp)                  # 4-byte Spill
	movl	36(%esp), %ebx                  # 4-byte Reload
	adcl	%ebx, 64(%esp)                  # 4-byte Folded Spill
	adcl	8(%esp), %ebp                   # 4-byte Folded Reload
	movl	%ebp, 44(%esp)                  # 4-byte Spill
	movl	52(%esp), %ebx                  # 4-byte Reload
	adcl	%ebx, 48(%esp)                  # 4-byte Folded Spill
	adcl	16(%esp), %esi                  # 4-byte Folded Reload
	movl	%esi, (%esp)                    # 4-byte Spill
	adcl	20(%esp), %edx                  # 4-byte Folded Reload
	movl	%edx, 28(%esp)                  # 4-byte Spill
	adcl	24(%esp), %ecx                  # 4-byte Folded Reload
	movl	%ecx, 32(%esp)                  # 4-byte Spill
	adcl	60(%esp), %eax                  # 4-byte Folded Reload
	movl	%eax, 12(%esp)                  # 4-byte Spill
	setb	36(%esp)                        # 1-byte Folded Spill
	movl	92(%esp), %esi                  # 4-byte Reload
	imull	%edi, %esi
	movl	%esi, %eax
	mull	132(%esp)                       # 4-byte Folded Reload
	movl	%edx, 24(%esp)                  # 4-byte Spill
	movl	%eax, 60(%esp)                  # 4-byte Spill
	movl	%esi, %eax
	mull	128(%esp)                       # 4-byte Folded Reload
	movl	%edx, 20(%esp)                  # 4-byte Spill
	movl	%eax, 56(%esp)                  # 4-byte Spill
	movl	%esi, %eax
	mull	124(%esp)                       # 4-byte Folded Reload
	movl	%edx, 16(%esp)                  # 4-byte Spill
	movl	%eax, 68(%esp)                  # 4-byte Spill
	movl	%esi, %eax
	mull	120(%esp)                       # 4-byte Folded Reload
	movl	%edx, 52(%esp)                  # 4-byte Spill
	movl	%eax, 88(%esp)                  # 4-byte Spill
	movl	%esi, %eax
	mull	112(%esp)                       # 4-byte Folded Reload
	movl	%edx, 8(%esp)                   # 4-byte Spill
	movl	%eax, %ebp
	movl	%esi, %eax
	mull	116(%esp)                       # 4-byte Folded Reload
	movl	%edx, %edi
	movl	%eax, %ecx
	movl	%esi, %eax
	mull	108(%esp)                       # 4-byte Folded Reload
	movl	%eax, %ebx
	addl	%ecx, %edx
	movl	%edx, 4(%esp)                   # 4-byte Spill
	adcl	%ebp, %edi
	movl	%edi, %ebp
	movl	8(%esp), %eax                   # 4-byte Reload
	adcl	88(%esp), %eax                  # 4-byte Folded Reload
	movl	52(%esp), %edi                  # 4-byte Reload
	adcl	68(%esp), %edi                  # 4-byte Folded Reload
	movl	16(%esp), %esi                  # 4-byte Reload
	adcl	56(%esp), %esi                  # 4-byte Folded Reload
	movl	20(%esp), %edx                  # 4-byte Reload
	adcl	60(%esp), %edx                  # 4-byte Folded Reload
	movl	24(%esp), %ecx                  # 4-byte Reload
	adcl	$0, %ecx
	addl	40(%esp), %ebx                  # 4-byte Folded Reload
	movl	4(%esp), %ebx                   # 4-byte Reload
	adcl	64(%esp), %ebx                  # 4-byte Folded Reload
	movl	%ebx, 4(%esp)                   # 4-byte Spill
	adcl	44(%esp), %ebp                  # 4-byte Folded Reload
	movl	%ebp, 44(%esp)                  # 4-byte Spill
	adcl	48(%esp), %eax                  # 4-byte Folded Reload
	movl	%eax, 8(%esp)                   # 4-byte Spill
	adcl	(%esp), %edi                    # 4-byte Folded Reload
	movl	%edi, 52(%esp)                  # 4-byte Spill
	adcl	28(%esp), %esi                  # 4-byte Folded Reload
	movl	%esi, 16(%esp)                  # 4-byte Spill
	adcl	32(%esp), %edx                  # 4-byte Folded Reload
	movl	%edx, 20(%esp)                  # 4-byte Spill
	adcl	12(%esp), %ecx                  # 4-byte Folded Reload
	movl	%ecx, 24(%esp)                  # 4-byte Spill
	movzbl	36(%esp), %eax                  # 1-byte Folded Reload
	adcl	$0, %eax
	movl	%eax, 60(%esp)                  # 4-byte Spill
	movl	180(%esp), %eax
	movl	16(%eax), %ecx
	movl	%ecx, %eax
	mull	80(%esp)                        # 4-byte Folded Reload
	movl	%edx, 12(%esp)                  # 4-byte Spill
	movl	%eax, 36(%esp)                  # 4-byte Spill
	movl	%ecx, %eax
	mull	76(%esp)                        # 4-byte Folded Reload
	movl	%edx, 32(%esp)                  # 4-byte Spill
	movl	%eax, 56(%esp)                  # 4-byte Spill
	movl	%ecx, %eax
	mull	100(%esp)                       # 4-byte Folded Reload
	movl	%edx, 28(%esp)                  # 4-byte Spill
	movl	%eax, 68(%esp)                  # 4-byte Spill
	movl	%ecx, %eax
	mull	72(%esp)                        # 4-byte Folded Reload
	movl	%edx, (%esp)                    # 4-byte Spill
	movl	%eax, 48(%esp)                  # 4-byte Spill
	movl	%ecx, %eax
	mull	104(%esp)                       # 4-byte Folded Reload
	movl	%edx, %edi
	movl	%eax, %ebp
	movl	%ecx, %eax
	mull	96(%esp)                        # 4-byte Folded Reload
	movl	%edx, %esi
	movl	%eax, %ebx
	movl	%ecx, %eax
	mull	84(%esp)                        # 4-byte Folded Reload
	movl	%eax, 40(%esp)                  # 4-byte Spill
	addl	%ebx, %edx
	movl	%edx, 64(%esp)                  # 4-byte Spill
	adcl	%ebp, %esi
	movl	%esi, %ebp
	adcl	48(%esp), %edi                  # 4-byte Folded Reload
	movl	%edi, 48(%esp)                  # 4-byte Spill
	movl	(%esp), %esi                    # 4-byte Reload
	adcl	68(%esp), %esi                  # 4-byte Folded Reload
	movl	28(%esp), %edx                  # 4-byte Reload
	adcl	56(%esp), %edx                  # 4-byte Folded Reload
	movl	32(%esp), %ecx                  # 4-byte Reload
	adcl	36(%esp), %ecx                  # 4-byte Folded Reload
	movl	12(%esp), %eax                  # 4-byte Reload
	adcl	$0, %eax
	movl	40(%esp), %edi                  # 4-byte Reload
	addl	4(%esp), %edi                   # 4-byte Folded Reload
	movl	%edi, 40(%esp)                  # 4-byte Spill
	movl	44(%esp), %ebx                  # 4-byte Reload
	adcl	%ebx, 64(%esp)                  # 4-byte Folded Spill
	adcl	8(%esp), %ebp                   # 4-byte Folded Reload
	movl	%ebp, 36(%esp)                  # 4-byte Spill
	movl	52(%esp), %ebx                  # 4-byte Reload
	adcl	%ebx, 48(%esp)                  # 4-byte Folded Spill
	adcl	16(%esp), %esi                  # 4-byte Folded Reload
	movl	%esi, (%esp)                    # 4-byte Spill
	adcl	20(%esp), %edx                  # 4-byte Folded Reload
	movl	%edx, 28(%esp)                  # 4-byte Spill
	adcl	24(%esp), %ecx                  # 4-byte Folded Reload
	movl	%ecx, 32(%esp)                  # 4-byte Spill
	adcl	60(%esp), %eax                  # 4-byte Folded Reload
	movl	%eax, 12(%esp)                  # 4-byte Spill
	setb	52(%esp)                        # 1-byte Folded Spill
	movl	92(%esp), %ecx                  # 4-byte Reload
	imull	%edi, %ecx
	movl	%ecx, %eax
	mull	132(%esp)                       # 4-byte Folded Reload
	movl	%edx, 24(%esp)                  # 4-byte Spill
	movl	%eax, 44(%esp)                  # 4-byte Spill
	movl	%ecx, %eax
	mull	128(%esp)                       # 4-byte Folded Reload
	movl	%edx, 20(%esp)                  # 4-byte Spill
	movl	%eax, 60(%esp)                  # 4-byte Spill
	movl	%ecx, %eax
	mull	124(%esp)                       # 4-byte Folded Reload
	movl	%edx, 16(%esp)                  # 4-byte Spill
	movl	%eax, 56(%esp)                  # 4-byte Spill
	movl	%ecx, %eax
	mull	120(%esp)                       # 4-byte Folded Reload
	movl	%edx, %ebp
	movl	%eax, 68(%esp)                  # 4-byte Spill
	movl	%ecx, %eax
	mull	112(%esp)                       # 4-byte Folded Reload
	movl	%edx, 8(%esp)                   # 4-byte Spill
	movl	%eax, %ebx
	movl	%ecx, %eax
	mull	116(%esp)                       # 4-byte Folded Reload
	movl	%edx, %edi
	movl	%eax, %esi
	movl	%ecx, %eax
	mull	108(%esp)                       # 4-byte Folded Reload
	addl	%esi, %edx
	movl	%edx, 4(%esp)                   # 4-byte Spill
	movl	%edi, %ecx
	adcl	%ebx, %ecx
	movl	8(%esp), %edi                   # 4-byte Reload
	adcl	68(%esp), %edi                  # 4-byte Folded Reload
	adcl	56(%esp), %ebp                  # 4-byte Folded Reload
	movl	%ebp, %ebx
	movl	16(%esp), %ebp                  # 4-byte Reload
	adcl	60(%esp), %ebp                  # 4-byte Folded Reload
	movl	20(%esp), %esi                  # 4-byte Reload
	adcl	44(%esp), %esi                  # 4-byte Folded Reload
	movl	24(%esp), %edx                  # 4-byte Reload
	adcl	$0, %edx
	addl	40(%esp), %eax                  # 4-byte Folded Reload
	movl	4(%esp), %eax                   # 4-byte Reload
	adcl	64(%esp), %eax                  # 4-byte Folded Reload
	movl	%eax, 4(%esp)                   # 4-byte Spill
	adcl	36(%esp), %ecx                  # 4-byte Folded Reload
	movl	%ecx, 40(%esp)                  # 4-byte Spill
	adcl	48(%esp), %edi                  # 4-byte Folded Reload
	movl	%edi, 8(%esp)                   # 4-byte Spill
	adcl	(%esp), %ebx                    # 4-byte Folded Reload
	movl	%ebx, 48(%esp)                  # 4-byte Spill
	adcl	28(%esp), %ebp                  # 4-byte Folded Reload
	movl	%ebp, 16(%esp)                  # 4-byte Spill
	adcl	32(%esp), %esi                  # 4-byte Folded Reload
	movl	%esi, 20(%esp)                  # 4-byte Spill
	adcl	12(%esp), %edx                  # 4-byte Folded Reload
	movl	%edx, 24(%esp)                  # 4-byte Spill
	movzbl	52(%esp), %eax                  # 1-byte Folded Reload
	adcl	$0, %eax
	movl	%eax, 64(%esp)                  # 4-byte Spill
	movl	180(%esp), %eax
	movl	20(%eax), %esi
	movl	%esi, %eax
	mull	80(%esp)                        # 4-byte Folded Reload
	movl	%edx, %ebp
	movl	%eax, 36(%esp)                  # 4-byte Spill
	movl	%esi, %eax
	mull	76(%esp)                        # 4-byte Folded Reload
	movl	%edx, 12(%esp)                  # 4-byte Spill
	movl	%eax, 44(%esp)                  # 4-byte Spill
	movl	%esi, %eax
	mull	100(%esp)                       # 4-byte Folded Reload
	movl	%edx, 32(%esp)                  # 4-byte Spill
	movl	%eax, 60(%esp)                  # 4-byte Spill
	movl	%esi, %eax
	mull	72(%esp)                        # 4-byte Folded Reload
	movl	%edx, 28(%esp)                  # 4-byte Spill
	movl	%eax, 56(%esp)                  # 4-byte Spill
	movl	%esi, %eax
	mull	104(%esp)                       # 4-byte Folded Reload
	movl	%edx, %edi
	movl	%eax, (%esp)                    # 4-byte Spill
	movl	%esi, %eax
	mull	96(%esp)                        # 4-byte Folded Reload
	movl	%edx, %ecx
	movl	%eax, %ebx
	movl	%esi, %eax
	mull	84(%esp)                        # 4-byte Folded Reload
	addl	%ebx, %edx
	movl	%edx, 52(%esp)                  # 4-byte Spill
	adcl	(%esp), %ecx                    # 4-byte Folded Reload
	movl	%ecx, (%esp)                    # 4-byte Spill
	adcl	56(%esp), %edi                  # 4-byte Folded Reload
	movl	28(%esp), %esi                  # 4-byte Reload
	adcl	60(%esp), %esi                  # 4-byte Folded Reload
	movl	32(%esp), %ebx                  # 4-byte Reload
	adcl	44(%esp), %ebx                  # 4-byte Folded Reload
	movl	12(%esp), %edx                  # 4-byte Reload
	adcl	36(%esp), %edx                  # 4-byte Folded Reload
	movl	%ebp, %ecx
	adcl	$0, %ecx
	addl	4(%esp), %eax                   # 4-byte Folded Reload
	movl	%eax, 36(%esp)                  # 4-byte Spill
	movl	40(%esp), %ebp                  # 4-byte Reload
	adcl	%ebp, 52(%esp)                  # 4-byte Folded Spill
	movl	(%esp), %ebp                    # 4-byte Reload
	adcl	8(%esp), %ebp                   # 4-byte Folded Reload
	movl	%ebp, (%esp)                    # 4-byte Spill
	adcl	48(%esp), %edi                  # 4-byte Folded Reload
	movl	%edi, 48(%esp)                  # 4-byte Spill
	adcl	16(%esp), %esi                  # 4-byte Folded Reload
	movl	%esi, 28(%esp)                  # 4-byte Spill
	adcl	20(%esp), %ebx                  # 4-byte Folded Reload
	movl	%ebx, 32(%esp)                  # 4-byte Spill
	adcl	24(%esp), %edx                  # 4-byte Folded Reload
	movl	%edx, 12(%esp)                  # 4-byte Spill
	adcl	64(%esp), %ecx                  # 4-byte Folded Reload
	movl	%ecx, 64(%esp)                  # 4-byte Spill
	setb	40(%esp)                        # 1-byte Folded Spill
	movl	92(%esp), %ebx                  # 4-byte Reload
	imull	%eax, %ebx
	movl	%ebx, %eax
	mull	132(%esp)                       # 4-byte Folded Reload
	movl	%edx, 24(%esp)                  # 4-byte Spill
	movl	%eax, 44(%esp)                  # 4-byte Spill
	movl	%ebx, %eax
	mull	128(%esp)                       # 4-byte Folded Reload
	movl	%edx, 20(%esp)                  # 4-byte Spill
	movl	%eax, 60(%esp)                  # 4-byte Spill
	movl	%ebx, %eax
	mull	124(%esp)                       # 4-byte Folded Reload
	movl	%edx, 16(%esp)                  # 4-byte Spill
	movl	%eax, 56(%esp)                  # 4-byte Spill
	movl	%ebx, %eax
	mull	120(%esp)                       # 4-byte Folded Reload
	movl	%edx, 8(%esp)                   # 4-byte Spill
	movl	%eax, 68(%esp)                  # 4-byte Spill
	movl	%ebx, %eax
	mull	112(%esp)                       # 4-byte Folded Reload
	movl	%edx, %esi
	movl	%eax, 88(%esp)                  # 4-byte Spill
	movl	%ebx, %eax
	mull	116(%esp)                       # 4-byte Folded Reload
	movl	%edx, %ebp
	movl	%eax, %ecx
	movl	%ebx, %eax
	mull	108(%esp)                       # 4-byte Folded Reload
	movl	%eax, %edi
	addl	%ecx, %edx
	movl	%edx, 4(%esp)                   # 4-byte Spill
	adcl	88(%esp), %ebp                  # 4-byte Folded Reload
	adcl	68(%esp), %esi                  # 4-byte Folded Reload
	movl	%esi, %ebx
	movl	8(%esp), %eax                   # 4-byte Reload
	adcl	56(%esp), %eax                  # 4-byte Folded Reload
	movl	16(%esp), %esi                  # 4-byte Reload
	adcl	60(%esp), %esi                  # 4-byte Folded Reload
	movl	20(%esp), %edx                  # 4-byte Reload
	adcl	44(%esp), %edx                  # 4-byte Folded Reload
	movl	24(%esp), %ecx                  # 4-byte Reload
	adcl	$0, %ecx
	addl	36(%esp), %edi                  # 4-byte Folded Reload
	movl	4(%esp), %edi                   # 4-byte Reload
	adcl	52(%esp), %edi                  # 4-byte Folded Reload
	movl	%edi, 4(%esp)                   # 4-byte Spill
	adcl	(%esp), %ebp                    # 4-byte Folded Reload
	movl	%ebp, (%esp)                    # 4-byte Spill
	adcl	48(%esp), %ebx                  # 4-byte Folded Reload
	movl	%ebx, 52(%esp)                  # 4-byte Spill
	adcl	28(%esp), %eax                  # 4-byte Folded Reload
	movl	%eax, 8(%esp)                   # 4-byte Spill
	adcl	32(%esp), %esi                  # 4-byte Folded Reload
	movl	%esi, 16(%esp)                  # 4-byte Spill
	adcl	12(%esp), %edx                  # 4-byte Folded Reload
	movl	%edx, 20(%esp)                  # 4-byte Spill
	adcl	64(%esp), %ecx                  # 4-byte Folded Reload
	movl	%ecx, 24(%esp)                  # 4-byte Spill
	movzbl	40(%esp), %eax                  # 1-byte Folded Reload
	adcl	$0, %eax
	movl	%eax, 32(%esp)                  # 4-byte Spill
	movl	180(%esp), %eax
	movl	24(%eax), %ecx
	movl	%ecx, %eax
	mull	80(%esp)                        # 4-byte Folded Reload
	movl	%edx, %ebx
	movl	%eax, 28(%esp)                  # 4-byte Spill
	movl	%ecx, %eax
	mull	76(%esp)                        # 4-byte Folded Reload
	movl	%edx, 80(%esp)                  # 4-byte Spill
	movl	%eax, 40(%esp)                  # 4-byte Spill
	movl	%ecx, %eax
	mull	100(%esp)                       # 4-byte Folded Reload
	movl	%edx, 76(%esp)                  # 4-byte Spill
	movl	%eax, 64(%esp)                  # 4-byte Spill
	movl	%ecx, %eax
	mull	72(%esp)                        # 4-byte Folded Reload
	movl	%edx, 100(%esp)                 # 4-byte Spill
	movl	%eax, 72(%esp)                  # 4-byte Spill
	movl	%ecx, %eax
	mull	104(%esp)                       # 4-byte Folded Reload
	movl	%edx, %edi
	movl	%eax, 12(%esp)                  # 4-byte Spill
	movl	%ecx, %eax
	mull	96(%esp)                        # 4-byte Folded Reload
	movl	%edx, %esi
	movl	%eax, %ebp
	movl	%ecx, %eax
	mull	84(%esp)                        # 4-byte Folded Reload
	movl	%eax, 84(%esp)                  # 4-byte Spill
	addl	%ebp, %edx
	movl	%edx, 104(%esp)                 # 4-byte Spill
	adcl	12(%esp), %esi                  # 4-byte Folded Reload
	movl	%esi, 12(%esp)                  # 4-byte Spill
	adcl	72(%esp), %edi                  # 4-byte Folded Reload
	movl	%edi, %ebp
	movl	100(%esp), %edi                 # 4-byte Reload
	adcl	64(%esp), %edi                  # 4-byte Folded Reload
	movl	76(%esp), %edx                  # 4-byte Reload
	adcl	40(%esp), %edx                  # 4-byte Folded Reload
	movl	80(%esp), %ecx                  # 4-byte Reload
	adcl	28(%esp), %ecx                  # 4-byte Folded Reload
	movl	%ebx, %eax
	adcl	$0, %eax
	movl	84(%esp), %esi                  # 4-byte Reload
	addl	4(%esp), %esi                   # 4-byte Folded Reload
	movl	%esi, 84(%esp)                  # 4-byte Spill
	movl	(%esp), %ebx                    # 4-byte Reload
	adcl	%ebx, 104(%esp)                 # 4-byte Folded Spill
	movl	52(%esp), %ebx                  # 4-byte Reload
	adcl	%ebx, 12(%esp)                  # 4-byte Folded Spill
	adcl	8(%esp), %ebp                   # 4-byte Folded Reload
	movl	%ebp, 28(%esp)                  # 4-byte Spill
	adcl	16(%esp), %edi                  # 4-byte Folded Reload
	movl	%edi, 100(%esp)                 # 4-byte Spill
	adcl	20(%esp), %edx                  # 4-byte Folded Reload
	movl	%edx, 76(%esp)                  # 4-byte Spill
	adcl	24(%esp), %ecx                  # 4-byte Folded Reload
	movl	%ecx, 80(%esp)                  # 4-byte Spill
	adcl	32(%esp), %eax                  # 4-byte Folded Reload
	movl	%eax, 32(%esp)                  # 4-byte Spill
	setb	4(%esp)                         # 1-byte Folded Spill
	movl	92(%esp), %edi                  # 4-byte Reload
	imull	%esi, %edi
	movl	%edi, %eax
	mull	132(%esp)                       # 4-byte Folded Reload
	movl	%edx, 96(%esp)                  # 4-byte Spill
	movl	%eax, (%esp)                    # 4-byte Spill
	movl	%edi, %eax
	mull	128(%esp)                       # 4-byte Folded Reload
	movl	%edx, 92(%esp)                  # 4-byte Spill
	movl	%eax, 72(%esp)                  # 4-byte Spill
	movl	%edi, %eax
	mull	124(%esp)                       # 4-byte Folded Reload
	movl	%edx, %esi
	movl	%eax, 8(%esp)                   # 4-byte Spill
	movl	%edi, %eax
	mull	120(%esp)                       # 4-byte Folded Reload
	movl	%edx, %ebp
	movl	%eax, 20(%esp)                  # 4-byte Spill
	movl	%edi, %eax
	mull	112(%esp)                       # 4-byte Folded Reload
	movl	%edx, %ebx
	movl	%eax, 16(%esp)                  # 4-byte Spill
	movl	%edi, %eax
	mull	108(%esp)                       # 4-byte Folded Reload
	movl	%edx, %ecx
	movl	%eax, 24(%esp)                  # 4-byte Spill
	movl	%edi, %eax
	mull	116(%esp)                       # 4-byte Folded Reload
	movl	%eax, %edi
	addl	%ecx, %edi
	adcl	16(%esp), %edx                  # 4-byte Folded Reload
	adcl	20(%esp), %ebx                  # 4-byte Folded Reload
	adcl	8(%esp), %ebp                   # 4-byte Folded Reload
	adcl	72(%esp), %esi                  # 4-byte Folded Reload
	movl	%esi, 72(%esp)                  # 4-byte Spill
	movl	92(%esp), %ecx                  # 4-byte Reload
	adcl	(%esp), %ecx                    # 4-byte Folded Reload
	movl	96(%esp), %eax                  # 4-byte Reload
	adcl	$0, %eax
	movl	24(%esp), %esi                  # 4-byte Reload
	addl	84(%esp), %esi                  # 4-byte Folded Reload
	adcl	104(%esp), %edi                 # 4-byte Folded Reload
	adcl	12(%esp), %edx                  # 4-byte Folded Reload
	adcl	28(%esp), %ebx                  # 4-byte Folded Reload
	adcl	100(%esp), %ebp                 # 4-byte Folded Reload
	movl	72(%esp), %esi                  # 4-byte Reload
	adcl	76(%esp), %esi                  # 4-byte Folded Reload
	adcl	80(%esp), %ecx                  # 4-byte Folded Reload
	movl	%ecx, 92(%esp)                  # 4-byte Spill
	adcl	32(%esp), %eax                  # 4-byte Folded Reload
	movl	%eax, %ecx
	movzbl	4(%esp), %eax                   # 1-byte Folded Reload
	adcl	$0, %eax
	movl	%edi, 84(%esp)                  # 4-byte Spill
	subl	108(%esp), %edi                 # 4-byte Folded Reload
	movl	%edi, 108(%esp)                 # 4-byte Spill
	movl	%edx, 76(%esp)                  # 4-byte Spill
	sbbl	116(%esp), %edx                 # 4-byte Folded Reload
	movl	%edx, 116(%esp)                 # 4-byte Spill
	movl	%ebx, 80(%esp)                  # 4-byte Spill
	movl	%ebx, %edx
	sbbl	112(%esp), %edx                 # 4-byte Folded Reload
	movl	%ebp, 112(%esp)                 # 4-byte Spill
	movl	%ebp, %edi
	sbbl	120(%esp), %edi                 # 4-byte Folded Reload
	movl	%esi, %ebp
	movl	%esi, 72(%esp)                  # 4-byte Spill
	sbbl	124(%esp), %ebp                 # 4-byte Folded Reload
	movl	92(%esp), %ebx                  # 4-byte Reload
	sbbl	128(%esp), %ebx                 # 4-byte Folded Reload
	movl	%ecx, 96(%esp)                  # 4-byte Spill
	sbbl	132(%esp), %ecx                 # 4-byte Folded Reload
	sbbl	$0, %eax
	testb	$1, %al
	jne	.LBB26_1
# %bb.2:
	movl	172(%esp), %eax
	movl	%ecx, 24(%eax)
	jne	.LBB26_3
.LBB26_4:
	movl	%ebx, 20(%eax)
	movl	108(%esp), %ecx                 # 4-byte Reload
	jne	.LBB26_5
.LBB26_6:
	movl	%ebp, 16(%eax)
	jne	.LBB26_7
.LBB26_8:
	movl	%edi, 12(%eax)
	jne	.LBB26_9
.LBB26_10:
	movl	%edx, 8(%eax)
	movl	116(%esp), %edx                 # 4-byte Reload
	jne	.LBB26_11
.LBB26_12:
	movl	%edx, 4(%eax)
	je	.LBB26_14
.LBB26_13:
	movl	84(%esp), %ecx                  # 4-byte Reload
.LBB26_14:
	movl	%ecx, (%eax)
	addl	$152, %esp
	popl	%esi
	popl	%edi
	popl	%ebx
	popl	%ebp
	retl
.LBB26_1:
	movl	96(%esp), %ecx                  # 4-byte Reload
	movl	172(%esp), %eax
	movl	%ecx, 24(%eax)
	je	.LBB26_4
.LBB26_3:
	movl	92(%esp), %ebx                  # 4-byte Reload
	movl	%ebx, 20(%eax)
	movl	108(%esp), %ecx                 # 4-byte Reload
	je	.LBB26_6
.LBB26_5:
	movl	72(%esp), %ebp                  # 4-byte Reload
	movl	%ebp, 16(%eax)
	je	.LBB26_8
.LBB26_7:
	movl	112(%esp), %edi                 # 4-byte Reload
	movl	%edi, 12(%eax)
	je	.LBB26_10
.LBB26_9:
	movl	80(%esp), %edx                  # 4-byte Reload
	movl	%edx, 8(%eax)
	movl	116(%esp), %edx                 # 4-byte Reload
	je	.LBB26_12
.LBB26_11:
	movl	76(%esp), %edx                  # 4-byte Reload
	movl	%edx, 4(%eax)
	jne	.LBB26_13
	jmp	.LBB26_14
.Lfunc_end26:
	.size	mcl_fp_mont7L, .Lfunc_end26-mcl_fp_mont7L
                                        # -- End function
	.globl	mcl_fp_montNF7L                 # -- Begin function mcl_fp_montNF7L
	.p2align	4, 0x90
	.type	mcl_fp_montNF7L,@function
mcl_fp_montNF7L:                        # @mcl_fp_montNF7L
# %bb.0:
	pushl	%ebp
	pushl	%ebx
	pushl	%edi
	pushl	%esi
	subl	$156, %esp
	movl	180(%esp), %edi
	movl	(%edi), %eax
	movl	%eax, 88(%esp)                  # 4-byte Spill
	movl	184(%esp), %ecx
	movl	(%ecx), %esi
	mull	%esi
	movl	%eax, 60(%esp)                  # 4-byte Spill
	movl	%edx, 44(%esp)                  # 4-byte Spill
	movl	188(%esp), %ebx
	movl	-4(%ebx), %ecx
	movl	%ecx, 80(%esp)                  # 4-byte Spill
	imull	%eax, %ecx
	movl	24(%ebx), %edx
	movl	%edx, 124(%esp)                 # 4-byte Spill
	movl	%ecx, %eax
	mull	%edx
	movl	%eax, 8(%esp)                   # 4-byte Spill
	movl	%edx, 16(%esp)                  # 4-byte Spill
	movl	20(%ebx), %edx
	movl	%edx, 120(%esp)                 # 4-byte Spill
	movl	%ecx, %eax
	mull	%edx
	movl	%eax, 12(%esp)                  # 4-byte Spill
	movl	%edx, 4(%esp)                   # 4-byte Spill
	movl	16(%ebx), %edx
	movl	%edx, 116(%esp)                 # 4-byte Spill
	movl	%ecx, %eax
	mull	%edx
	movl	%eax, 52(%esp)                  # 4-byte Spill
	movl	%edx, (%esp)                    # 4-byte Spill
	movl	12(%ebx), %edx
	movl	%edx, 112(%esp)                 # 4-byte Spill
	movl	%ecx, %eax
	mull	%edx
	movl	%eax, 64(%esp)                  # 4-byte Spill
	movl	%edx, 40(%esp)                  # 4-byte Spill
	movl	8(%ebx), %edx
	movl	%edx, 104(%esp)                 # 4-byte Spill
	movl	%ecx, %eax
	mull	%edx
	movl	%eax, 128(%esp)                 # 4-byte Spill
	movl	%edx, 56(%esp)                  # 4-byte Spill
	movl	(%ebx), %ebp
	movl	%ebp, 108(%esp)                 # 4-byte Spill
	movl	4(%ebx), %edx
	movl	%edx, 100(%esp)                 # 4-byte Spill
	movl	%ecx, %eax
	mull	%edx
	movl	%edx, 48(%esp)                  # 4-byte Spill
	movl	%eax, 152(%esp)                 # 4-byte Spill
	movl	%ecx, %eax
	mull	%ebp
	movl	%eax, 148(%esp)                 # 4-byte Spill
	movl	%edx, 132(%esp)                 # 4-byte Spill
	movl	24(%edi), %eax
	movl	%eax, 76(%esp)                  # 4-byte Spill
	mull	%esi
	movl	%eax, %ebp
	movl	%edx, 36(%esp)                  # 4-byte Spill
	movl	20(%edi), %eax
	movl	%eax, 72(%esp)                  # 4-byte Spill
	mull	%esi
	movl	%eax, 144(%esp)                 # 4-byte Spill
	movl	%edx, 32(%esp)                  # 4-byte Spill
	movl	16(%edi), %eax
	movl	%eax, 68(%esp)                  # 4-byte Spill
	mull	%esi
	movl	%eax, 140(%esp)                 # 4-byte Spill
	movl	%edx, 28(%esp)                  # 4-byte Spill
	movl	12(%edi), %eax
	movl	%eax, 96(%esp)                  # 4-byte Spill
	mull	%esi
	movl	%eax, 136(%esp)                 # 4-byte Spill
	movl	%edx, 24(%esp)                  # 4-byte Spill
	movl	4(%edi), %ecx
	movl	%ecx, 92(%esp)                  # 4-byte Spill
	movl	8(%edi), %eax
	movl	%eax, 84(%esp)                  # 4-byte Spill
	mull	%esi
	movl	%edx, 20(%esp)                  # 4-byte Spill
	movl	%eax, %ebx
	movl	%ecx, %eax
	mull	%esi
	addl	44(%esp), %eax                  # 4-byte Folded Reload
	movl	%eax, 44(%esp)                  # 4-byte Spill
	adcl	%ebx, %edx
	movl	%edx, %edi
	movl	20(%esp), %esi                  # 4-byte Reload
	adcl	136(%esp), %esi                 # 4-byte Folded Reload
	movl	24(%esp), %ebx                  # 4-byte Reload
	adcl	140(%esp), %ebx                 # 4-byte Folded Reload
	movl	28(%esp), %edx                  # 4-byte Reload
	adcl	144(%esp), %edx                 # 4-byte Folded Reload
	movl	32(%esp), %ecx                  # 4-byte Reload
	adcl	%ebp, %ecx
	movl	36(%esp), %eax                  # 4-byte Reload
	adcl	$0, %eax
	movl	148(%esp), %ebp                 # 4-byte Reload
	addl	60(%esp), %ebp                  # 4-byte Folded Reload
	movl	44(%esp), %ebp                  # 4-byte Reload
	adcl	152(%esp), %ebp                 # 4-byte Folded Reload
	adcl	128(%esp), %edi                 # 4-byte Folded Reload
	adcl	64(%esp), %esi                  # 4-byte Folded Reload
	adcl	52(%esp), %ebx                  # 4-byte Folded Reload
	adcl	12(%esp), %edx                  # 4-byte Folded Reload
	adcl	8(%esp), %ecx                   # 4-byte Folded Reload
	adcl	$0, %eax
	addl	132(%esp), %ebp                 # 4-byte Folded Reload
	adcl	48(%esp), %edi                  # 4-byte Folded Reload
	movl	%edi, 12(%esp)                  # 4-byte Spill
	adcl	56(%esp), %esi                  # 4-byte Folded Reload
	movl	%esi, 20(%esp)                  # 4-byte Spill
	adcl	40(%esp), %ebx                  # 4-byte Folded Reload
	movl	%ebx, 24(%esp)                  # 4-byte Spill
	adcl	(%esp), %edx                    # 4-byte Folded Reload
	movl	%edx, 28(%esp)                  # 4-byte Spill
	adcl	4(%esp), %ecx                   # 4-byte Folded Reload
	movl	%ecx, 32(%esp)                  # 4-byte Spill
	adcl	16(%esp), %eax                  # 4-byte Folded Reload
	movl	%eax, 36(%esp)                  # 4-byte Spill
	movl	184(%esp), %eax
	movl	4(%eax), %ecx
	movl	%ecx, %eax
	mull	76(%esp)                        # 4-byte Folded Reload
	movl	%edx, %ebx
	movl	%eax, 60(%esp)                  # 4-byte Spill
	movl	%ecx, %eax
	mull	72(%esp)                        # 4-byte Folded Reload
	movl	%edx, 16(%esp)                  # 4-byte Spill
	movl	%eax, 56(%esp)                  # 4-byte Spill
	movl	%ecx, %eax
	mull	68(%esp)                        # 4-byte Folded Reload
	movl	%edx, 4(%esp)                   # 4-byte Spill
	movl	%eax, 52(%esp)                  # 4-byte Spill
	movl	%ecx, %eax
	mull	96(%esp)                        # 4-byte Folded Reload
	movl	%edx, (%esp)                    # 4-byte Spill
	movl	%eax, 44(%esp)                  # 4-byte Spill
	movl	%ecx, %eax
	mull	84(%esp)                        # 4-byte Folded Reload
	movl	%edx, %edi
	movl	%eax, 40(%esp)                  # 4-byte Spill
	movl	%ecx, %eax
	mull	92(%esp)                        # 4-byte Folded Reload
	movl	%edx, %esi
	movl	%eax, 8(%esp)                   # 4-byte Spill
	movl	%ecx, %eax
	mull	88(%esp)                        # 4-byte Folded Reload
	movl	%eax, 48(%esp)                  # 4-byte Spill
	addl	8(%esp), %edx                   # 4-byte Folded Reload
	movl	%edx, 8(%esp)                   # 4-byte Spill
	adcl	40(%esp), %esi                  # 4-byte Folded Reload
	movl	%esi, 40(%esp)                  # 4-byte Spill
	adcl	44(%esp), %edi                  # 4-byte Folded Reload
	movl	(%esp), %esi                    # 4-byte Reload
	adcl	52(%esp), %esi                  # 4-byte Folded Reload
	movl	4(%esp), %ecx                   # 4-byte Reload
	adcl	56(%esp), %ecx                  # 4-byte Folded Reload
	movl	16(%esp), %edx                  # 4-byte Reload
	adcl	60(%esp), %edx                  # 4-byte Folded Reload
	movl	%ebx, %eax
	adcl	$0, %eax
	movl	48(%esp), %ebx                  # 4-byte Reload
	addl	%ebp, %ebx
	movl	12(%esp), %ebp                  # 4-byte Reload
	adcl	%ebp, 8(%esp)                   # 4-byte Folded Spill
	movl	20(%esp), %ebp                  # 4-byte Reload
	adcl	%ebp, 40(%esp)                  # 4-byte Folded Spill
	adcl	24(%esp), %edi                  # 4-byte Folded Reload
	movl	%edi, 60(%esp)                  # 4-byte Spill
	adcl	28(%esp), %esi                  # 4-byte Folded Reload
	movl	%esi, (%esp)                    # 4-byte Spill
	adcl	32(%esp), %ecx                  # 4-byte Folded Reload
	movl	%ecx, 4(%esp)                   # 4-byte Spill
	adcl	36(%esp), %edx                  # 4-byte Folded Reload
	movl	%edx, 16(%esp)                  # 4-byte Spill
	adcl	$0, %eax
	movl	%eax, 32(%esp)                  # 4-byte Spill
	movl	80(%esp), %edi                  # 4-byte Reload
	imull	%ebx, %edi
	movl	%edi, %eax
	mull	124(%esp)                       # 4-byte Folded Reload
	movl	%edx, 36(%esp)                  # 4-byte Spill
	movl	%eax, 28(%esp)                  # 4-byte Spill
	movl	%edi, %eax
	mull	120(%esp)                       # 4-byte Folded Reload
	movl	%edx, 12(%esp)                  # 4-byte Spill
	movl	%eax, 24(%esp)                  # 4-byte Spill
	movl	%edi, %eax
	mull	116(%esp)                       # 4-byte Folded Reload
	movl	%edx, 52(%esp)                  # 4-byte Spill
	movl	%eax, 20(%esp)                  # 4-byte Spill
	movl	%edi, %eax
	mull	112(%esp)                       # 4-byte Folded Reload
	movl	%edx, 44(%esp)                  # 4-byte Spill
	movl	%eax, %ebp
	movl	%edi, %eax
	mull	104(%esp)                       # 4-byte Folded Reload
	movl	%edx, 48(%esp)                  # 4-byte Spill
	movl	%eax, %ecx
	movl	%edi, %eax
	mull	100(%esp)                       # 4-byte Folded Reload
	movl	%edx, 64(%esp)                  # 4-byte Spill
	movl	%eax, %esi
	movl	%edi, %eax
	mull	108(%esp)                       # 4-byte Folded Reload
	movl	%edx, 56(%esp)                  # 4-byte Spill
	addl	%ebx, %eax
	adcl	8(%esp), %esi                   # 4-byte Folded Reload
	adcl	40(%esp), %ecx                  # 4-byte Folded Reload
	movl	%ecx, %edx
	adcl	60(%esp), %ebp                  # 4-byte Folded Reload
	movl	20(%esp), %ebx                  # 4-byte Reload
	adcl	(%esp), %ebx                    # 4-byte Folded Reload
	movl	24(%esp), %edi                  # 4-byte Reload
	adcl	4(%esp), %edi                   # 4-byte Folded Reload
	movl	28(%esp), %ecx                  # 4-byte Reload
	adcl	16(%esp), %ecx                  # 4-byte Folded Reload
	movl	32(%esp), %eax                  # 4-byte Reload
	adcl	$0, %eax
	addl	56(%esp), %esi                  # 4-byte Folded Reload
	movl	%esi, 56(%esp)                  # 4-byte Spill
	adcl	64(%esp), %edx                  # 4-byte Folded Reload
	movl	%edx, 4(%esp)                   # 4-byte Spill
	adcl	48(%esp), %ebp                  # 4-byte Folded Reload
	movl	%ebp, 16(%esp)                  # 4-byte Spill
	adcl	44(%esp), %ebx                  # 4-byte Folded Reload
	movl	%ebx, 20(%esp)                  # 4-byte Spill
	adcl	52(%esp), %edi                  # 4-byte Folded Reload
	movl	%edi, 24(%esp)                  # 4-byte Spill
	adcl	12(%esp), %ecx                  # 4-byte Folded Reload
	movl	%ecx, 28(%esp)                  # 4-byte Spill
	adcl	36(%esp), %eax                  # 4-byte Folded Reload
	movl	%eax, 32(%esp)                  # 4-byte Spill
	movl	184(%esp), %eax
	movl	8(%eax), %ebp
	movl	%ebp, %eax
	mull	76(%esp)                        # 4-byte Folded Reload
	movl	%edx, 36(%esp)                  # 4-byte Spill
	movl	%eax, 52(%esp)                  # 4-byte Spill
	movl	%ebp, %eax
	mull	72(%esp)                        # 4-byte Folded Reload
	movl	%edx, (%esp)                    # 4-byte Spill
	movl	%eax, 44(%esp)                  # 4-byte Spill
	movl	%ebp, %eax
	mull	68(%esp)                        # 4-byte Folded Reload
	movl	%edx, 8(%esp)                   # 4-byte Spill
	movl	%eax, 48(%esp)                  # 4-byte Spill
	movl	%ebp, %eax
	mull	96(%esp)                        # 4-byte Folded Reload
	movl	%edx, 40(%esp)                  # 4-byte Spill
	movl	%eax, 12(%esp)                  # 4-byte Spill
	movl	%ebp, %eax
	mull	84(%esp)                        # 4-byte Folded Reload
	movl	%edx, %ecx
	movl	%eax, %edi
	movl	%ebp, %eax
	mull	92(%esp)                        # 4-byte Folded Reload
	movl	%edx, %ebx
	movl	%eax, %esi
	movl	%ebp, %eax
	mull	88(%esp)                        # 4-byte Folded Reload
	movl	%eax, 60(%esp)                  # 4-byte Spill
	movl	%edx, %ebp
	addl	%esi, %ebp
	adcl	%edi, %ebx
	movl	%ebx, %edi
	adcl	12(%esp), %ecx                  # 4-byte Folded Reload
	movl	%ecx, 12(%esp)                  # 4-byte Spill
	movl	40(%esp), %ecx                  # 4-byte Reload
	adcl	48(%esp), %ecx                  # 4-byte Folded Reload
	movl	8(%esp), %ebx                   # 4-byte Reload
	adcl	44(%esp), %ebx                  # 4-byte Folded Reload
	movl	(%esp), %edx                    # 4-byte Reload
	adcl	52(%esp), %edx                  # 4-byte Folded Reload
	movl	36(%esp), %eax                  # 4-byte Reload
	adcl	$0, %eax
	movl	60(%esp), %esi                  # 4-byte Reload
	addl	56(%esp), %esi                  # 4-byte Folded Reload
	movl	%esi, 60(%esp)                  # 4-byte Spill
	adcl	4(%esp), %ebp                   # 4-byte Folded Reload
	adcl	16(%esp), %edi                  # 4-byte Folded Reload
	movl	%edi, 52(%esp)                  # 4-byte Spill
	movl	12(%esp), %esi                  # 4-byte Reload
	adcl	20(%esp), %esi                  # 4-byte Folded Reload
	movl	%esi, 12(%esp)                  # 4-byte Spill
	adcl	24(%esp), %ecx                  # 4-byte Folded Reload
	movl	%ecx, 40(%esp)                  # 4-byte Spill
	adcl	28(%esp), %ebx                  # 4-byte Folded Reload
	movl	%ebx, 8(%esp)                   # 4-byte Spill
	adcl	32(%esp), %edx                  # 4-byte Folded Reload
	movl	%edx, (%esp)                    # 4-byte Spill
	adcl	$0, %eax
	movl	%eax, 36(%esp)                  # 4-byte Spill
	movl	80(%esp), %esi                  # 4-byte Reload
	movl	60(%esp), %edi                  # 4-byte Reload
	imull	%edi, %esi
	movl	%esi, %eax
	mull	124(%esp)                       # 4-byte Folded Reload
	movl	%edx, 32(%esp)                  # 4-byte Spill
	movl	%eax, 28(%esp)                  # 4-byte Spill
	movl	%esi, %eax
	mull	120(%esp)                       # 4-byte Folded Reload
	movl	%edx, 56(%esp)                  # 4-byte Spill
	movl	%eax, 24(%esp)                  # 4-byte Spill
	movl	%esi, %eax
	mull	116(%esp)                       # 4-byte Folded Reload
	movl	%edx, 44(%esp)                  # 4-byte Spill
	movl	%eax, 20(%esp)                  # 4-byte Spill
	movl	%esi, %eax
	mull	112(%esp)                       # 4-byte Folded Reload
	movl	%edx, 48(%esp)                  # 4-byte Spill
	movl	%eax, 16(%esp)                  # 4-byte Spill
	movl	%esi, %eax
	mull	104(%esp)                       # 4-byte Folded Reload
	movl	%edx, 64(%esp)                  # 4-byte Spill
	movl	%eax, 4(%esp)                   # 4-byte Spill
	movl	%esi, %eax
	mull	100(%esp)                       # 4-byte Folded Reload
	movl	%edx, 132(%esp)                 # 4-byte Spill
	movl	%eax, %ecx
	movl	%esi, %eax
	mull	108(%esp)                       # 4-byte Folded Reload
	movl	%edx, 128(%esp)                 # 4-byte Spill
	addl	%edi, %eax
	adcl	%ebp, %ecx
	movl	%ecx, %ebx
	movl	4(%esp), %edx                   # 4-byte Reload
	adcl	52(%esp), %edx                  # 4-byte Folded Reload
	movl	16(%esp), %ebp                  # 4-byte Reload
	adcl	12(%esp), %ebp                  # 4-byte Folded Reload
	movl	20(%esp), %edi                  # 4-byte Reload
	adcl	40(%esp), %edi                  # 4-byte Folded Reload
	movl	24(%esp), %esi                  # 4-byte Reload
	adcl	8(%esp), %esi                   # 4-byte Folded Reload
	movl	28(%esp), %ecx                  # 4-byte Reload
	adcl	(%esp), %ecx                    # 4-byte Folded Reload
	movl	36(%esp), %eax                  # 4-byte Reload
	adcl	$0, %eax
	addl	128(%esp), %ebx                 # 4-byte Folded Reload
	movl	%ebx, 60(%esp)                  # 4-byte Spill
	adcl	132(%esp), %edx                 # 4-byte Folded Reload
	movl	%edx, 4(%esp)                   # 4-byte Spill
	adcl	64(%esp), %ebp                  # 4-byte Folded Reload
	movl	%ebp, 16(%esp)                  # 4-byte Spill
	adcl	48(%esp), %edi                  # 4-byte Folded Reload
	movl	%edi, 20(%esp)                  # 4-byte Spill
	adcl	44(%esp), %esi                  # 4-byte Folded Reload
	movl	%esi, 24(%esp)                  # 4-byte Spill
	adcl	56(%esp), %ecx                  # 4-byte Folded Reload
	movl	%ecx, 28(%esp)                  # 4-byte Spill
	adcl	32(%esp), %eax                  # 4-byte Folded Reload
	movl	%eax, 36(%esp)                  # 4-byte Spill
	movl	184(%esp), %eax
	movl	12(%eax), %esi
	movl	%esi, %eax
	mull	76(%esp)                        # 4-byte Folded Reload
	movl	%edx, 32(%esp)                  # 4-byte Spill
	movl	%eax, 56(%esp)                  # 4-byte Spill
	movl	%esi, %eax
	mull	72(%esp)                        # 4-byte Folded Reload
	movl	%edx, 8(%esp)                   # 4-byte Spill
	movl	%eax, 52(%esp)                  # 4-byte Spill
	movl	%esi, %eax
	mull	68(%esp)                        # 4-byte Folded Reload
	movl	%edx, %ebp
	movl	%eax, 44(%esp)                  # 4-byte Spill
	movl	%esi, %eax
	mull	96(%esp)                        # 4-byte Folded Reload
	movl	%edx, 40(%esp)                  # 4-byte Spill
	movl	%eax, 48(%esp)                  # 4-byte Spill
	movl	%esi, %eax
	mull	84(%esp)                        # 4-byte Folded Reload
	movl	%edx, %ecx
	movl	%eax, (%esp)                    # 4-byte Spill
	movl	%esi, %eax
	mull	92(%esp)                        # 4-byte Folded Reload
	movl	%edx, %edi
	movl	%eax, %ebx
	movl	%esi, %eax
	mull	88(%esp)                        # 4-byte Folded Reload
	movl	%eax, %esi
	addl	%ebx, %edx
	movl	%edx, 12(%esp)                  # 4-byte Spill
	adcl	(%esp), %edi                    # 4-byte Folded Reload
	movl	%edi, (%esp)                    # 4-byte Spill
	adcl	48(%esp), %ecx                  # 4-byte Folded Reload
	movl	%ecx, %ebx
	movl	40(%esp), %ecx                  # 4-byte Reload
	adcl	44(%esp), %ecx                  # 4-byte Folded Reload
	adcl	52(%esp), %ebp                  # 4-byte Folded Reload
	movl	8(%esp), %edx                   # 4-byte Reload
	adcl	56(%esp), %edx                  # 4-byte Folded Reload
	movl	32(%esp), %eax                  # 4-byte Reload
	adcl	$0, %eax
	addl	60(%esp), %esi                  # 4-byte Folded Reload
	movl	4(%esp), %edi                   # 4-byte Reload
	adcl	%edi, 12(%esp)                  # 4-byte Folded Spill
	movl	(%esp), %edi                    # 4-byte Reload
	adcl	16(%esp), %edi                  # 4-byte Folded Reload
	movl	%edi, (%esp)                    # 4-byte Spill
	adcl	20(%esp), %ebx                  # 4-byte Folded Reload
	movl	%ebx, 60(%esp)                  # 4-byte Spill
	adcl	24(%esp), %ecx                  # 4-byte Folded Reload
	movl	%ecx, 40(%esp)                  # 4-byte Spill
	adcl	28(%esp), %ebp                  # 4-byte Folded Reload
	movl	%ebp, 16(%esp)                  # 4-byte Spill
	adcl	36(%esp), %edx                  # 4-byte Folded Reload
	movl	%edx, 8(%esp)                   # 4-byte Spill
	adcl	$0, %eax
	movl	%eax, 32(%esp)                  # 4-byte Spill
	movl	80(%esp), %ebx                  # 4-byte Reload
	imull	%esi, %ebx
	movl	%ebx, %eax
	mull	124(%esp)                       # 4-byte Folded Reload
	movl	%edx, 36(%esp)                  # 4-byte Spill
	movl	%eax, 28(%esp)                  # 4-byte Spill
	movl	%ebx, %eax
	mull	120(%esp)                       # 4-byte Folded Reload
	movl	%edx, 4(%esp)                   # 4-byte Spill
	movl	%eax, 24(%esp)                  # 4-byte Spill
	movl	%ebx, %eax
	mull	116(%esp)                       # 4-byte Folded Reload
	movl	%edx, 52(%esp)                  # 4-byte Spill
	movl	%eax, 20(%esp)                  # 4-byte Spill
	movl	%ebx, %eax
	mull	112(%esp)                       # 4-byte Folded Reload
	movl	%edx, 44(%esp)                  # 4-byte Spill
	movl	%eax, %edi
	movl	%ebx, %eax
	mull	104(%esp)                       # 4-byte Folded Reload
	movl	%edx, 48(%esp)                  # 4-byte Spill
	movl	%eax, %ebp
	movl	%ebx, %eax
	mull	100(%esp)                       # 4-byte Folded Reload
	movl	%edx, 64(%esp)                  # 4-byte Spill
	movl	%eax, %ecx
	movl	%ebx, %eax
	mull	108(%esp)                       # 4-byte Folded Reload
	movl	%edx, 56(%esp)                  # 4-byte Spill
	addl	%esi, %eax
	adcl	12(%esp), %ecx                  # 4-byte Folded Reload
	movl	%ecx, %edx
	movl	%ebp, %ebx
	adcl	(%esp), %ebx                    # 4-byte Folded Reload
	adcl	60(%esp), %edi                  # 4-byte Folded Reload
	movl	20(%esp), %esi                  # 4-byte Reload
	adcl	40(%esp), %esi                  # 4-byte Folded Reload
	movl	24(%esp), %ecx                  # 4-byte Reload
	adcl	16(%esp), %ecx                  # 4-byte Folded Reload
	movl	28(%esp), %ebp                  # 4-byte Reload
	adcl	8(%esp), %ebp                   # 4-byte Folded Reload
	movl	32(%esp), %eax                  # 4-byte Reload
	adcl	$0, %eax
	addl	56(%esp), %edx                  # 4-byte Folded Reload
	movl	%edx, 56(%esp)                  # 4-byte Spill
	adcl	64(%esp), %ebx                  # 4-byte Folded Reload
	movl	%ebx, 60(%esp)                  # 4-byte Spill
	adcl	48(%esp), %edi                  # 4-byte Folded Reload
	movl	%edi, 16(%esp)                  # 4-byte Spill
	adcl	44(%esp), %esi                  # 4-byte Folded Reload
	movl	%esi, 20(%esp)                  # 4-byte Spill
	adcl	52(%esp), %ecx                  # 4-byte Folded Reload
	movl	%ecx, 24(%esp)                  # 4-byte Spill
	adcl	4(%esp), %ebp                   # 4-byte Folded Reload
	movl	%ebp, 28(%esp)                  # 4-byte Spill
	adcl	36(%esp), %eax                  # 4-byte Folded Reload
	movl	%eax, 32(%esp)                  # 4-byte Spill
	movl	184(%esp), %eax
	movl	16(%eax), %esi
	movl	%esi, %eax
	mull	76(%esp)                        # 4-byte Folded Reload
	movl	%edx, 36(%esp)                  # 4-byte Spill
	movl	%eax, 52(%esp)                  # 4-byte Spill
	movl	%esi, %eax
	mull	72(%esp)                        # 4-byte Folded Reload
	movl	%edx, 4(%esp)                   # 4-byte Spill
	movl	%eax, 44(%esp)                  # 4-byte Spill
	movl	%esi, %eax
	mull	68(%esp)                        # 4-byte Folded Reload
	movl	%edx, (%esp)                    # 4-byte Spill
	movl	%eax, 48(%esp)                  # 4-byte Spill
	movl	%esi, %eax
	mull	96(%esp)                        # 4-byte Folded Reload
	movl	%edx, 8(%esp)                   # 4-byte Spill
	movl	%eax, 64(%esp)                  # 4-byte Spill
	movl	%esi, %eax
	mull	84(%esp)                        # 4-byte Folded Reload
	movl	%edx, %ecx
	movl	%eax, 12(%esp)                  # 4-byte Spill
	movl	%esi, %eax
	mull	92(%esp)                        # 4-byte Folded Reload
	movl	%edx, %ebp
	movl	%eax, %ebx
	movl	%esi, %eax
	mull	88(%esp)                        # 4-byte Folded Reload
	movl	%eax, 40(%esp)                  # 4-byte Spill
	movl	%edx, %edi
	addl	%ebx, %edi
	adcl	12(%esp), %ebp                  # 4-byte Folded Reload
	movl	%ebp, 12(%esp)                  # 4-byte Spill
	adcl	64(%esp), %ecx                  # 4-byte Folded Reload
	movl	%ecx, %ebp
	movl	8(%esp), %ecx                   # 4-byte Reload
	adcl	48(%esp), %ecx                  # 4-byte Folded Reload
	movl	(%esp), %ebx                    # 4-byte Reload
	adcl	44(%esp), %ebx                  # 4-byte Folded Reload
	movl	4(%esp), %edx                   # 4-byte Reload
	adcl	52(%esp), %edx                  # 4-byte Folded Reload
	movl	36(%esp), %eax                  # 4-byte Reload
	adcl	$0, %eax
	movl	40(%esp), %esi                  # 4-byte Reload
	addl	56(%esp), %esi                  # 4-byte Folded Reload
	movl	%esi, 40(%esp)                  # 4-byte Spill
	adcl	60(%esp), %edi                  # 4-byte Folded Reload
	movl	%edi, 56(%esp)                  # 4-byte Spill
	movl	12(%esp), %esi                  # 4-byte Reload
	adcl	16(%esp), %esi                  # 4-byte Folded Reload
	movl	%esi, 12(%esp)                  # 4-byte Spill
	adcl	20(%esp), %ebp                  # 4-byte Folded Reload
	movl	%ebp, 60(%esp)                  # 4-byte Spill
	adcl	24(%esp), %ecx                  # 4-byte Folded Reload
	movl	%ecx, 8(%esp)                   # 4-byte Spill
	adcl	28(%esp), %ebx                  # 4-byte Folded Reload
	movl	%ebx, (%esp)                    # 4-byte Spill
	adcl	32(%esp), %edx                  # 4-byte Folded Reload
	movl	%edx, 4(%esp)                   # 4-byte Spill
	adcl	$0, %eax
	movl	%eax, 36(%esp)                  # 4-byte Spill
	movl	80(%esp), %ebx                  # 4-byte Reload
	movl	40(%esp), %edi                  # 4-byte Reload
	imull	%edi, %ebx
	movl	%ebx, %eax
	mull	124(%esp)                       # 4-byte Folded Reload
	movl	%edx, 32(%esp)                  # 4-byte Spill
	movl	%eax, 28(%esp)                  # 4-byte Spill
	movl	%ebx, %eax
	mull	120(%esp)                       # 4-byte Folded Reload
	movl	%edx, 52(%esp)                  # 4-byte Spill
	movl	%eax, 24(%esp)                  # 4-byte Spill
	movl	%ebx, %eax
	mull	116(%esp)                       # 4-byte Folded Reload
	movl	%edx, 44(%esp)                  # 4-byte Spill
	movl	%eax, %esi
	movl	%ebx, %eax
	mull	112(%esp)                       # 4-byte Folded Reload
	movl	%edx, 48(%esp)                  # 4-byte Spill
	movl	%eax, 20(%esp)                  # 4-byte Spill
	movl	%ebx, %eax
	mull	104(%esp)                       # 4-byte Folded Reload
	movl	%edx, 64(%esp)                  # 4-byte Spill
	movl	%eax, 16(%esp)                  # 4-byte Spill
	movl	%ebx, %eax
	mull	100(%esp)                       # 4-byte Folded Reload
	movl	%edx, 132(%esp)                 # 4-byte Spill
	movl	%eax, %ebp
	movl	%ebx, %eax
	mull	108(%esp)                       # 4-byte Folded Reload
	movl	%edx, 128(%esp)                 # 4-byte Spill
	addl	%edi, %eax
	adcl	56(%esp), %ebp                  # 4-byte Folded Reload
	movl	16(%esp), %edx                  # 4-byte Reload
	adcl	12(%esp), %edx                  # 4-byte Folded Reload
	movl	20(%esp), %ebx                  # 4-byte Reload
	adcl	60(%esp), %ebx                  # 4-byte Folded Reload
	movl	%esi, %edi
	adcl	8(%esp), %edi                   # 4-byte Folded Reload
	movl	24(%esp), %esi                  # 4-byte Reload
	adcl	(%esp), %esi                    # 4-byte Folded Reload
	movl	28(%esp), %ecx                  # 4-byte Reload
	adcl	4(%esp), %ecx                   # 4-byte Folded Reload
	movl	36(%esp), %eax                  # 4-byte Reload
	adcl	$0, %eax
	addl	128(%esp), %ebp                 # 4-byte Folded Reload
	movl	%ebp, 56(%esp)                  # 4-byte Spill
	adcl	132(%esp), %edx                 # 4-byte Folded Reload
	movl	%edx, 16(%esp)                  # 4-byte Spill
	adcl	64(%esp), %ebx                  # 4-byte Folded Reload
	movl	%ebx, 20(%esp)                  # 4-byte Spill
	adcl	48(%esp), %edi                  # 4-byte Folded Reload
	movl	%edi, 60(%esp)                  # 4-byte Spill
	adcl	44(%esp), %esi                  # 4-byte Folded Reload
	movl	%esi, 24(%esp)                  # 4-byte Spill
	adcl	52(%esp), %ecx                  # 4-byte Folded Reload
	movl	%ecx, 28(%esp)                  # 4-byte Spill
	adcl	32(%esp), %eax                  # 4-byte Folded Reload
	movl	%eax, 36(%esp)                  # 4-byte Spill
	movl	184(%esp), %eax
	movl	20(%eax), %esi
	movl	%esi, %eax
	mull	76(%esp)                        # 4-byte Folded Reload
	movl	%edx, 32(%esp)                  # 4-byte Spill
	movl	%eax, 52(%esp)                  # 4-byte Spill
	movl	%esi, %eax
	mull	72(%esp)                        # 4-byte Folded Reload
	movl	%edx, (%esp)                    # 4-byte Spill
	movl	%eax, 44(%esp)                  # 4-byte Spill
	movl	%esi, %eax
	mull	68(%esp)                        # 4-byte Folded Reload
	movl	%edx, 8(%esp)                   # 4-byte Spill
	movl	%eax, 48(%esp)                  # 4-byte Spill
	movl	%esi, %eax
	mull	96(%esp)                        # 4-byte Folded Reload
	movl	%edx, 40(%esp)                  # 4-byte Spill
	movl	%eax, 4(%esp)                   # 4-byte Spill
	movl	%esi, %eax
	mull	84(%esp)                        # 4-byte Folded Reload
	movl	%edx, %ecx
	movl	%eax, %ebx
	movl	%esi, %eax
	mull	92(%esp)                        # 4-byte Folded Reload
	movl	%edx, %edi
	movl	%eax, %ebp
	movl	%esi, %eax
	mull	88(%esp)                        # 4-byte Folded Reload
	movl	%eax, %esi
	addl	%ebp, %edx
	movl	%edx, 12(%esp)                  # 4-byte Spill
	adcl	%ebx, %edi
	movl	%edi, %ebx
	adcl	4(%esp), %ecx                   # 4-byte Folded Reload
	movl	%ecx, 4(%esp)                   # 4-byte Spill
	movl	40(%esp), %ecx                  # 4-byte Reload
	adcl	48(%esp), %ecx                  # 4-byte Folded Reload
	movl	8(%esp), %ebp                   # 4-byte Reload
	adcl	44(%esp), %ebp                  # 4-byte Folded Reload
	movl	(%esp), %edx                    # 4-byte Reload
	adcl	52(%esp), %edx                  # 4-byte Folded Reload
	movl	32(%esp), %eax                  # 4-byte Reload
	adcl	$0, %eax
	addl	56(%esp), %esi                  # 4-byte Folded Reload
	movl	16(%esp), %edi                  # 4-byte Reload
	adcl	%edi, 12(%esp)                  # 4-byte Folded Spill
	adcl	20(%esp), %ebx                  # 4-byte Folded Reload
	movl	4(%esp), %edi                   # 4-byte Reload
	adcl	60(%esp), %edi                  # 4-byte Folded Reload
	movl	%edi, 4(%esp)                   # 4-byte Spill
	adcl	24(%esp), %ecx                  # 4-byte Folded Reload
	movl	%ecx, 40(%esp)                  # 4-byte Spill
	adcl	28(%esp), %ebp                  # 4-byte Folded Reload
	movl	%ebp, 8(%esp)                   # 4-byte Spill
	adcl	36(%esp), %edx                  # 4-byte Folded Reload
	movl	%edx, (%esp)                    # 4-byte Spill
	adcl	$0, %eax
	movl	%eax, 32(%esp)                  # 4-byte Spill
	movl	80(%esp), %ebp                  # 4-byte Reload
	imull	%esi, %ebp
	movl	%ebp, %eax
	mull	124(%esp)                       # 4-byte Folded Reload
	movl	%edx, 60(%esp)                  # 4-byte Spill
	movl	%eax, 36(%esp)                  # 4-byte Spill
	movl	%ebp, %eax
	mull	120(%esp)                       # 4-byte Folded Reload
	movl	%edx, 56(%esp)                  # 4-byte Spill
	movl	%eax, 28(%esp)                  # 4-byte Spill
	movl	%ebp, %eax
	mull	116(%esp)                       # 4-byte Folded Reload
	movl	%edx, 52(%esp)                  # 4-byte Spill
	movl	%eax, 24(%esp)                  # 4-byte Spill
	movl	%ebp, %eax
	mull	112(%esp)                       # 4-byte Folded Reload
	movl	%edx, 44(%esp)                  # 4-byte Spill
	movl	%eax, %edi
	movl	%ebp, %eax
	mull	104(%esp)                       # 4-byte Folded Reload
	movl	%edx, 16(%esp)                  # 4-byte Spill
	movl	%eax, 20(%esp)                  # 4-byte Spill
	movl	%ebp, %eax
	mull	100(%esp)                       # 4-byte Folded Reload
	movl	%edx, 48(%esp)                  # 4-byte Spill
	movl	%eax, %ecx
	movl	%ebp, %eax
	mull	108(%esp)                       # 4-byte Folded Reload
	movl	%edx, 64(%esp)                  # 4-byte Spill
	addl	%esi, %eax
	adcl	12(%esp), %ecx                  # 4-byte Folded Reload
	movl	%ecx, %edx
	movl	20(%esp), %ebp                  # 4-byte Reload
	adcl	%ebx, %ebp
	adcl	4(%esp), %edi                   # 4-byte Folded Reload
	movl	24(%esp), %ecx                  # 4-byte Reload
	adcl	40(%esp), %ecx                  # 4-byte Folded Reload
	movl	28(%esp), %eax                  # 4-byte Reload
	adcl	8(%esp), %eax                   # 4-byte Folded Reload
	movl	36(%esp), %ebx                  # 4-byte Reload
	adcl	(%esp), %ebx                    # 4-byte Folded Reload
	movl	32(%esp), %esi                  # 4-byte Reload
	adcl	$0, %esi
	addl	64(%esp), %edx                  # 4-byte Folded Reload
	movl	%edx, 4(%esp)                   # 4-byte Spill
	adcl	48(%esp), %ebp                  # 4-byte Folded Reload
	movl	%ebp, 20(%esp)                  # 4-byte Spill
	adcl	16(%esp), %edi                  # 4-byte Folded Reload
	movl	%edi, 16(%esp)                  # 4-byte Spill
	adcl	44(%esp), %ecx                  # 4-byte Folded Reload
	movl	%ecx, 24(%esp)                  # 4-byte Spill
	adcl	52(%esp), %eax                  # 4-byte Folded Reload
	movl	%eax, 28(%esp)                  # 4-byte Spill
	adcl	56(%esp), %ebx                  # 4-byte Folded Reload
	movl	%ebx, 36(%esp)                  # 4-byte Spill
	adcl	60(%esp), %esi                  # 4-byte Folded Reload
	movl	%esi, 32(%esp)                  # 4-byte Spill
	movl	184(%esp), %eax
	movl	24(%eax), %edi
	movl	%edi, %eax
	mull	76(%esp)                        # 4-byte Folded Reload
	movl	%edx, 76(%esp)                  # 4-byte Spill
	movl	%eax, (%esp)                    # 4-byte Spill
	movl	%edi, %eax
	mull	72(%esp)                        # 4-byte Folded Reload
	movl	%edx, 72(%esp)                  # 4-byte Spill
	movl	%eax, 8(%esp)                   # 4-byte Spill
	movl	%edi, %eax
	mull	68(%esp)                        # 4-byte Folded Reload
	movl	%edx, 68(%esp)                  # 4-byte Spill
	movl	%eax, 40(%esp)                  # 4-byte Spill
	movl	%edi, %eax
	mull	96(%esp)                        # 4-byte Folded Reload
	movl	%edx, %esi
	movl	%eax, 12(%esp)                  # 4-byte Spill
	movl	%edi, %eax
	mull	84(%esp)                        # 4-byte Folded Reload
	movl	%edx, %ebx
	movl	%eax, 84(%esp)                  # 4-byte Spill
	movl	%edi, %eax
	mull	92(%esp)                        # 4-byte Folded Reload
	movl	%edx, %ebp
	movl	%eax, %ecx
	movl	%edi, %eax
	mull	88(%esp)                        # 4-byte Folded Reload
	addl	%ecx, %edx
	movl	%edx, 96(%esp)                  # 4-byte Spill
	movl	%ebp, %edx
	adcl	84(%esp), %edx                  # 4-byte Folded Reload
	movl	%ebx, %ecx
	adcl	12(%esp), %ecx                  # 4-byte Folded Reload
	movl	%esi, %edi
	adcl	40(%esp), %edi                  # 4-byte Folded Reload
	movl	68(%esp), %ebp                  # 4-byte Reload
	adcl	8(%esp), %ebp                   # 4-byte Folded Reload
	movl	72(%esp), %ebx                  # 4-byte Reload
	adcl	(%esp), %ebx                    # 4-byte Folded Reload
	movl	76(%esp), %esi                  # 4-byte Reload
	adcl	$0, %esi
	addl	4(%esp), %eax                   # 4-byte Folded Reload
	movl	%eax, 92(%esp)                  # 4-byte Spill
	movl	20(%esp), %eax                  # 4-byte Reload
	adcl	%eax, 96(%esp)                  # 4-byte Folded Spill
	adcl	16(%esp), %edx                  # 4-byte Folded Reload
	movl	%edx, 20(%esp)                  # 4-byte Spill
	adcl	24(%esp), %ecx                  # 4-byte Folded Reload
	movl	%ecx, 24(%esp)                  # 4-byte Spill
	adcl	28(%esp), %edi                  # 4-byte Folded Reload
	movl	%edi, 28(%esp)                  # 4-byte Spill
	adcl	36(%esp), %ebp                  # 4-byte Folded Reload
	movl	%ebp, 68(%esp)                  # 4-byte Spill
	adcl	32(%esp), %ebx                  # 4-byte Folded Reload
	movl	%ebx, 72(%esp)                  # 4-byte Spill
	adcl	$0, %esi
	movl	%esi, 76(%esp)                  # 4-byte Spill
	movl	80(%esp), %ecx                  # 4-byte Reload
	imull	92(%esp), %ecx                  # 4-byte Folded Reload
	movl	%ecx, %eax
	mull	124(%esp)                       # 4-byte Folded Reload
	movl	%edx, 84(%esp)                  # 4-byte Spill
	movl	%eax, 80(%esp)                  # 4-byte Spill
	movl	%ecx, %eax
	mull	120(%esp)                       # 4-byte Folded Reload
	movl	%edx, 36(%esp)                  # 4-byte Spill
	movl	%eax, 88(%esp)                  # 4-byte Spill
	movl	%ecx, %eax
	mull	116(%esp)                       # 4-byte Folded Reload
	movl	%edx, 32(%esp)                  # 4-byte Spill
	movl	%eax, %ebp
	movl	%ecx, %eax
	mull	112(%esp)                       # 4-byte Folded Reload
	movl	%edx, 16(%esp)                  # 4-byte Spill
	movl	%eax, %ebx
	movl	%ecx, %eax
	mull	108(%esp)                       # 4-byte Folded Reload
	movl	%edx, 4(%esp)                   # 4-byte Spill
	movl	%eax, %esi
	movl	%ecx, %eax
	mull	104(%esp)                       # 4-byte Folded Reload
	movl	%edx, (%esp)                    # 4-byte Spill
	movl	%eax, %edi
	movl	%ecx, %eax
	mull	100(%esp)                       # 4-byte Folded Reload
	movl	%eax, %ecx
	movl	%edx, 8(%esp)                   # 4-byte Spill
	addl	92(%esp), %esi                  # 4-byte Folded Reload
	adcl	96(%esp), %ecx                  # 4-byte Folded Reload
	adcl	20(%esp), %edi                  # 4-byte Folded Reload
	adcl	24(%esp), %ebx                  # 4-byte Folded Reload
	adcl	28(%esp), %ebp                  # 4-byte Folded Reload
	movl	88(%esp), %edx                  # 4-byte Reload
	adcl	68(%esp), %edx                  # 4-byte Folded Reload
	movl	80(%esp), %eax                  # 4-byte Reload
	adcl	72(%esp), %eax                  # 4-byte Folded Reload
	movl	76(%esp), %esi                  # 4-byte Reload
	adcl	$0, %esi
	addl	4(%esp), %ecx                   # 4-byte Folded Reload
	adcl	8(%esp), %edi                   # 4-byte Folded Reload
	adcl	(%esp), %ebx                    # 4-byte Folded Reload
	adcl	16(%esp), %ebp                  # 4-byte Folded Reload
	adcl	32(%esp), %edx                  # 4-byte Folded Reload
	adcl	36(%esp), %eax                  # 4-byte Folded Reload
	movl	%eax, 80(%esp)                  # 4-byte Spill
	adcl	84(%esp), %esi                  # 4-byte Folded Reload
	movl	%ecx, 72(%esp)                  # 4-byte Spill
	subl	108(%esp), %ecx                 # 4-byte Folded Reload
	movl	%ecx, 108(%esp)                 # 4-byte Spill
	movl	%edi, 68(%esp)                  # 4-byte Spill
	movl	%edi, %eax
	sbbl	100(%esp), %eax                 # 4-byte Folded Reload
	movl	%ebx, 100(%esp)                 # 4-byte Spill
	sbbl	104(%esp), %ebx                 # 4-byte Folded Reload
	movl	%ebp, 104(%esp)                 # 4-byte Spill
	sbbl	112(%esp), %ebp                 # 4-byte Folded Reload
	movl	%edx, 88(%esp)                  # 4-byte Spill
	sbbl	116(%esp), %edx                 # 4-byte Folded Reload
	movl	80(%esp), %ecx                  # 4-byte Reload
	sbbl	120(%esp), %ecx                 # 4-byte Folded Reload
	movl	%esi, 76(%esp)                  # 4-byte Spill
	sbbl	124(%esp), %esi                 # 4-byte Folded Reload
	movl	%esi, %edi
	sarl	$31, %edi
	testl	%edi, %edi
	js	.LBB27_1
# %bb.2:
	movl	176(%esp), %edi
	movl	%esi, 24(%edi)
	js	.LBB27_3
.LBB27_4:
	movl	%ecx, 20(%edi)
	js	.LBB27_5
.LBB27_6:
	movl	%edx, 16(%edi)
	js	.LBB27_7
.LBB27_8:
	movl	%ebp, 12(%edi)
	js	.LBB27_9
.LBB27_10:
	movl	%ebx, 8(%edi)
	js	.LBB27_11
.LBB27_12:
	movl	%eax, 4(%edi)
	movl	108(%esp), %eax                 # 4-byte Reload
	jns	.LBB27_14
.LBB27_13:
	movl	72(%esp), %eax                  # 4-byte Reload
.LBB27_14:
	movl	%eax, (%edi)
	addl	$156, %esp
	popl	%esi
	popl	%edi
	popl	%ebx
	popl	%ebp
	retl
.LBB27_1:
	movl	76(%esp), %esi                  # 4-byte Reload
	movl	176(%esp), %edi
	movl	%esi, 24(%edi)
	jns	.LBB27_4
.LBB27_3:
	movl	80(%esp), %ecx                  # 4-byte Reload
	movl	%ecx, 20(%edi)
	jns	.LBB27_6
.LBB27_5:
	movl	88(%esp), %edx                  # 4-byte Reload
	movl	%edx, 16(%edi)
	jns	.LBB27_8
.LBB27_7:
	movl	104(%esp), %ebp                 # 4-byte Reload
	movl	%ebp, 12(%edi)
	jns	.LBB27_10
.LBB27_9:
	movl	100(%esp), %ebx                 # 4-byte Reload
	movl	%ebx, 8(%edi)
	jns	.LBB27_12
.LBB27_11:
	movl	68(%esp), %eax                  # 4-byte Reload
	movl	%eax, 4(%edi)
	movl	108(%esp), %eax                 # 4-byte Reload
	js	.LBB27_13
	jmp	.LBB27_14
.Lfunc_end27:
	.size	mcl_fp_montNF7L, .Lfunc_end27-mcl_fp_montNF7L
                                        # -- End function
	.globl	mcl_fp_montRed7L                # -- Begin function mcl_fp_montRed7L
	.p2align	4, 0x90
	.type	mcl_fp_montRed7L,@function
mcl_fp_montRed7L:                       # @mcl_fp_montRed7L
# %bb.0:
	pushl	%ebp
	pushl	%ebx
	pushl	%edi
	pushl	%esi
	subl	$100, %esp
	movl	128(%esp), %ebx
	movl	-4(%ebx), %eax
	movl	%eax, 44(%esp)                  # 4-byte Spill
	movl	124(%esp), %ecx
	movl	(%ecx), %edi
	movl	%edi, 12(%esp)                  # 4-byte Spill
	imull	%eax, %edi
	movl	24(%ebx), %ecx
	movl	%ecx, 96(%esp)                  # 4-byte Spill
	movl	%edi, %eax
	mull	%ecx
	movl	%eax, 8(%esp)                   # 4-byte Spill
	movl	%edx, 16(%esp)                  # 4-byte Spill
	movl	20(%ebx), %ecx
	movl	%ecx, 84(%esp)                  # 4-byte Spill
	movl	%edi, %eax
	mull	%ecx
	movl	%eax, 28(%esp)                  # 4-byte Spill
	movl	%edx, (%esp)                    # 4-byte Spill
	movl	16(%ebx), %ecx
	movl	%ecx, 92(%esp)                  # 4-byte Spill
	movl	%edi, %eax
	mull	%ecx
	movl	%eax, 24(%esp)                  # 4-byte Spill
	movl	%edx, 40(%esp)                  # 4-byte Spill
	movl	12(%ebx), %ecx
	movl	%ecx, 88(%esp)                  # 4-byte Spill
	movl	%edi, %eax
	mull	%ecx
	movl	%eax, 32(%esp)                  # 4-byte Spill
	movl	%edx, 36(%esp)                  # 4-byte Spill
	movl	8(%ebx), %ecx
	movl	%ecx, 76(%esp)                  # 4-byte Spill
	movl	%edi, %eax
	mull	%ecx
	movl	%eax, 20(%esp)                  # 4-byte Spill
	movl	%edx, %ecx
	movl	(%ebx), %ebp
	movl	%ebp, 68(%esp)                  # 4-byte Spill
	movl	4(%ebx), %edx
	movl	%edx, 72(%esp)                  # 4-byte Spill
	movl	%edi, %eax
	mull	%edx
	movl	%edx, %esi
	movl	%eax, %ebx
	movl	%edi, %eax
	mull	%ebp
	movl	%eax, %ebp
	addl	%ebx, %edx
	movl	%edx, 4(%esp)                   # 4-byte Spill
	adcl	20(%esp), %esi                  # 4-byte Folded Reload
	movl	%ecx, %eax
	adcl	32(%esp), %eax                  # 4-byte Folded Reload
	movl	36(%esp), %edi                  # 4-byte Reload
	adcl	24(%esp), %edi                  # 4-byte Folded Reload
	movl	40(%esp), %ebx                  # 4-byte Reload
	adcl	28(%esp), %ebx                  # 4-byte Folded Reload
	movl	(%esp), %ecx                    # 4-byte Reload
	adcl	8(%esp), %ecx                   # 4-byte Folded Reload
	movl	%ecx, (%esp)                    # 4-byte Spill
	movl	16(%esp), %ecx                  # 4-byte Reload
	adcl	$0, %ecx
	addl	12(%esp), %ebp                  # 4-byte Folded Reload
	movl	4(%esp), %ebp                   # 4-byte Reload
	movl	124(%esp), %edx
	adcl	4(%edx), %ebp
	movl	%ebp, 4(%esp)                   # 4-byte Spill
	movl	124(%esp), %ebp
	adcl	8(%ebp), %esi
	movl	%esi, 60(%esp)                  # 4-byte Spill
	adcl	12(%ebp), %eax
	movl	%eax, 32(%esp)                  # 4-byte Spill
	adcl	16(%ebp), %edi
	movl	%edi, 36(%esp)                  # 4-byte Spill
	adcl	20(%ebp), %ebx
	movl	%ebx, 40(%esp)                  # 4-byte Spill
	movl	(%esp), %eax                    # 4-byte Reload
	adcl	24(%ebp), %eax
	movl	%eax, (%esp)                    # 4-byte Spill
	adcl	28(%ebp), %ecx
	movl	%ecx, 16(%esp)                  # 4-byte Spill
	setb	56(%esp)                        # 1-byte Folded Spill
	movl	44(%esp), %ecx                  # 4-byte Reload
	imull	4(%esp), %ecx                   # 4-byte Folded Reload
	movl	%ecx, %eax
	mull	96(%esp)                        # 4-byte Folded Reload
	movl	%edx, 8(%esp)                   # 4-byte Spill
	movl	%eax, 52(%esp)                  # 4-byte Spill
	movl	%ecx, %eax
	mull	84(%esp)                        # 4-byte Folded Reload
	movl	%edx, 12(%esp)                  # 4-byte Spill
	movl	%eax, 48(%esp)                  # 4-byte Spill
	movl	%ecx, %eax
	mull	92(%esp)                        # 4-byte Folded Reload
	movl	%edx, 28(%esp)                  # 4-byte Spill
	movl	%eax, 64(%esp)                  # 4-byte Spill
	movl	%ecx, %eax
	mull	88(%esp)                        # 4-byte Folded Reload
	movl	%edx, 24(%esp)                  # 4-byte Spill
	movl	%eax, %edi
	movl	%ecx, %eax
	mull	76(%esp)                        # 4-byte Folded Reload
	movl	%edx, %esi
	movl	%eax, 20(%esp)                  # 4-byte Spill
	movl	%ecx, %eax
	mull	68(%esp)                        # 4-byte Folded Reload
	movl	%edx, %ebp
	movl	%eax, 80(%esp)                  # 4-byte Spill
	movl	%ecx, %eax
	mull	72(%esp)                        # 4-byte Folded Reload
	addl	%ebp, %eax
	movl	%eax, %ebx
	adcl	20(%esp), %edx                  # 4-byte Folded Reload
	movl	%edx, 20(%esp)                  # 4-byte Spill
	adcl	%edi, %esi
	movl	%esi, %edi
	movl	24(%esp), %esi                  # 4-byte Reload
	adcl	64(%esp), %esi                  # 4-byte Folded Reload
	movl	28(%esp), %ecx                  # 4-byte Reload
	adcl	48(%esp), %ecx                  # 4-byte Folded Reload
	movl	12(%esp), %ebp                  # 4-byte Reload
	adcl	52(%esp), %ebp                  # 4-byte Folded Reload
	movzbl	56(%esp), %eax                  # 1-byte Folded Reload
	movl	8(%esp), %edx                   # 4-byte Reload
	adcl	%eax, %edx
	movl	80(%esp), %eax                  # 4-byte Reload
	addl	4(%esp), %eax                   # 4-byte Folded Reload
	adcl	60(%esp), %ebx                  # 4-byte Folded Reload
	movl	%ebx, 60(%esp)                  # 4-byte Spill
	movl	32(%esp), %eax                  # 4-byte Reload
	adcl	%eax, 20(%esp)                  # 4-byte Folded Spill
	adcl	36(%esp), %edi                  # 4-byte Folded Reload
	movl	%edi, 4(%esp)                   # 4-byte Spill
	adcl	40(%esp), %esi                  # 4-byte Folded Reload
	movl	%esi, 24(%esp)                  # 4-byte Spill
	adcl	(%esp), %ecx                    # 4-byte Folded Reload
	movl	%ecx, 28(%esp)                  # 4-byte Spill
	adcl	16(%esp), %ebp                  # 4-byte Folded Reload
	movl	%ebp, 12(%esp)                  # 4-byte Spill
	movl	124(%esp), %eax
	adcl	32(%eax), %edx
	movl	%edx, 8(%esp)                   # 4-byte Spill
	setb	16(%esp)                        # 1-byte Folded Spill
	movl	44(%esp), %edi                  # 4-byte Reload
	imull	%ebx, %edi
	movl	%edi, %eax
	mull	96(%esp)                        # 4-byte Folded Reload
	movl	%edx, 56(%esp)                  # 4-byte Spill
	movl	%eax, %esi
	movl	%edi, %eax
	mull	84(%esp)                        # 4-byte Folded Reload
	movl	%edx, (%esp)                    # 4-byte Spill
	movl	%eax, 52(%esp)                  # 4-byte Spill
	movl	%edi, %eax
	mull	92(%esp)                        # 4-byte Folded Reload
	movl	%edx, 40(%esp)                  # 4-byte Spill
	movl	%eax, 48(%esp)                  # 4-byte Spill
	movl	%edi, %eax
	mull	88(%esp)                        # 4-byte Folded Reload
	movl	%edx, %ebp
	movl	%eax, 80(%esp)                  # 4-byte Spill
	movl	%edi, %eax
	mull	76(%esp)                        # 4-byte Folded Reload
	movl	%edx, 36(%esp)                  # 4-byte Spill
	movl	%eax, %ebx
	movl	%edi, %eax
	mull	68(%esp)                        # 4-byte Folded Reload
	movl	%edx, %ecx
	movl	%eax, 64(%esp)                  # 4-byte Spill
	movl	%edi, %eax
	mull	72(%esp)                        # 4-byte Folded Reload
	addl	%ecx, %eax
	movl	%eax, 32(%esp)                  # 4-byte Spill
	adcl	%ebx, %edx
	movl	%edx, %ebx
	movl	36(%esp), %edi                  # 4-byte Reload
	adcl	80(%esp), %edi                  # 4-byte Folded Reload
	movl	%ebp, %ecx
	adcl	48(%esp), %ecx                  # 4-byte Folded Reload
	movl	40(%esp), %edx                  # 4-byte Reload
	adcl	52(%esp), %edx                  # 4-byte Folded Reload
	movl	(%esp), %eax                    # 4-byte Reload
	adcl	%esi, %eax
	movzbl	16(%esp), %esi                  # 1-byte Folded Reload
	adcl	56(%esp), %esi                  # 4-byte Folded Reload
	movl	64(%esp), %ebp                  # 4-byte Reload
	addl	60(%esp), %ebp                  # 4-byte Folded Reload
	movl	32(%esp), %ebp                  # 4-byte Reload
	adcl	20(%esp), %ebp                  # 4-byte Folded Reload
	movl	%ebp, 32(%esp)                  # 4-byte Spill
	adcl	4(%esp), %ebx                   # 4-byte Folded Reload
	movl	%ebx, 60(%esp)                  # 4-byte Spill
	adcl	24(%esp), %edi                  # 4-byte Folded Reload
	movl	%edi, 36(%esp)                  # 4-byte Spill
	adcl	28(%esp), %ecx                  # 4-byte Folded Reload
	movl	%ecx, 20(%esp)                  # 4-byte Spill
	adcl	12(%esp), %edx                  # 4-byte Folded Reload
	movl	%edx, 40(%esp)                  # 4-byte Spill
	adcl	8(%esp), %eax                   # 4-byte Folded Reload
	movl	%eax, (%esp)                    # 4-byte Spill
	movl	124(%esp), %eax
	adcl	36(%eax), %esi
	movl	%esi, 12(%esp)                  # 4-byte Spill
	setb	56(%esp)                        # 1-byte Folded Spill
	movl	44(%esp), %ecx                  # 4-byte Reload
	imull	%ebp, %ecx
	movl	%ecx, %eax
	mull	96(%esp)                        # 4-byte Folded Reload
	movl	%edx, 16(%esp)                  # 4-byte Spill
	movl	%eax, 52(%esp)                  # 4-byte Spill
	movl	%ecx, %eax
	mull	84(%esp)                        # 4-byte Folded Reload
	movl	%edx, 4(%esp)                   # 4-byte Spill
	movl	%eax, 48(%esp)                  # 4-byte Spill
	movl	%ecx, %eax
	mull	92(%esp)                        # 4-byte Folded Reload
	movl	%edx, 8(%esp)                   # 4-byte Spill
	movl	%eax, 64(%esp)                  # 4-byte Spill
	movl	%ecx, %eax
	mull	88(%esp)                        # 4-byte Folded Reload
	movl	%edx, 28(%esp)                  # 4-byte Spill
	movl	%eax, %ebx
	movl	%ecx, %eax
	mull	76(%esp)                        # 4-byte Folded Reload
	movl	%edx, %edi
	movl	%eax, 24(%esp)                  # 4-byte Spill
	movl	%ecx, %eax
	mull	68(%esp)                        # 4-byte Folded Reload
	movl	%edx, %esi
	movl	%eax, 80(%esp)                  # 4-byte Spill
	movl	%ecx, %eax
	mull	72(%esp)                        # 4-byte Folded Reload
	movl	%eax, %ebp
	addl	%esi, %ebp
	adcl	24(%esp), %edx                  # 4-byte Folded Reload
	movl	%edx, 24(%esp)                  # 4-byte Spill
	adcl	%ebx, %edi
	movl	%edi, %ebx
	movl	28(%esp), %edi                  # 4-byte Reload
	adcl	64(%esp), %edi                  # 4-byte Folded Reload
	movl	8(%esp), %ecx                   # 4-byte Reload
	adcl	48(%esp), %ecx                  # 4-byte Folded Reload
	movl	4(%esp), %esi                   # 4-byte Reload
	adcl	52(%esp), %esi                  # 4-byte Folded Reload
	movzbl	56(%esp), %eax                  # 1-byte Folded Reload
	movl	16(%esp), %edx                  # 4-byte Reload
	adcl	%eax, %edx
	movl	80(%esp), %eax                  # 4-byte Reload
	addl	32(%esp), %eax                  # 4-byte Folded Reload
	adcl	60(%esp), %ebp                  # 4-byte Folded Reload
	movl	36(%esp), %eax                  # 4-byte Reload
	adcl	%eax, 24(%esp)                  # 4-byte Folded Spill
	adcl	20(%esp), %ebx                  # 4-byte Folded Reload
	movl	%ebx, 32(%esp)                  # 4-byte Spill
	adcl	40(%esp), %edi                  # 4-byte Folded Reload
	movl	%edi, 28(%esp)                  # 4-byte Spill
	adcl	(%esp), %ecx                    # 4-byte Folded Reload
	movl	%ecx, 8(%esp)                   # 4-byte Spill
	adcl	12(%esp), %esi                  # 4-byte Folded Reload
	movl	%esi, 4(%esp)                   # 4-byte Spill
	movl	124(%esp), %eax
	adcl	40(%eax), %edx
	movl	%edx, 16(%esp)                  # 4-byte Spill
	setb	20(%esp)                        # 1-byte Folded Spill
	movl	44(%esp), %esi                  # 4-byte Reload
	movl	%ebp, 60(%esp)                  # 4-byte Spill
	imull	%ebp, %esi
	movl	%esi, %eax
	mull	96(%esp)                        # 4-byte Folded Reload
	movl	%edx, 56(%esp)                  # 4-byte Spill
	movl	%eax, %edi
	movl	%esi, %eax
	mull	84(%esp)                        # 4-byte Folded Reload
	movl	%edx, (%esp)                    # 4-byte Spill
	movl	%eax, 52(%esp)                  # 4-byte Spill
	movl	%esi, %eax
	mull	92(%esp)                        # 4-byte Folded Reload
	movl	%edx, 12(%esp)                  # 4-byte Spill
	movl	%eax, 48(%esp)                  # 4-byte Spill
	movl	%esi, %eax
	mull	88(%esp)                        # 4-byte Folded Reload
	movl	%edx, %ebx
	movl	%eax, 80(%esp)                  # 4-byte Spill
	movl	%esi, %eax
	mull	76(%esp)                        # 4-byte Folded Reload
	movl	%edx, 40(%esp)                  # 4-byte Spill
	movl	%eax, %ecx
	movl	%esi, %eax
	mull	68(%esp)                        # 4-byte Folded Reload
	movl	%edx, %ebp
	movl	%eax, 64(%esp)                  # 4-byte Spill
	movl	%esi, %eax
	mull	72(%esp)                        # 4-byte Folded Reload
	addl	%ebp, %eax
	movl	%eax, 36(%esp)                  # 4-byte Spill
	adcl	%ecx, %edx
	movl	%edx, %ecx
	movl	40(%esp), %ebp                  # 4-byte Reload
	adcl	80(%esp), %ebp                  # 4-byte Folded Reload
	adcl	48(%esp), %ebx                  # 4-byte Folded Reload
	movl	12(%esp), %edx                  # 4-byte Reload
	adcl	52(%esp), %edx                  # 4-byte Folded Reload
	movl	(%esp), %eax                    # 4-byte Reload
	adcl	%edi, %eax
	movzbl	20(%esp), %edi                  # 1-byte Folded Reload
	adcl	56(%esp), %edi                  # 4-byte Folded Reload
	movl	64(%esp), %esi                  # 4-byte Reload
	addl	60(%esp), %esi                  # 4-byte Folded Reload
	movl	36(%esp), %esi                  # 4-byte Reload
	adcl	24(%esp), %esi                  # 4-byte Folded Reload
	movl	%esi, 36(%esp)                  # 4-byte Spill
	adcl	32(%esp), %ecx                  # 4-byte Folded Reload
	movl	%ecx, 32(%esp)                  # 4-byte Spill
	adcl	28(%esp), %ebp                  # 4-byte Folded Reload
	movl	%ebp, 40(%esp)                  # 4-byte Spill
	adcl	8(%esp), %ebx                   # 4-byte Folded Reload
	movl	%ebx, 24(%esp)                  # 4-byte Spill
	adcl	4(%esp), %edx                   # 4-byte Folded Reload
	movl	%edx, 12(%esp)                  # 4-byte Spill
	adcl	16(%esp), %eax                  # 4-byte Folded Reload
	movl	%eax, (%esp)                    # 4-byte Spill
	movl	124(%esp), %eax
	adcl	44(%eax), %edi
	movl	%edi, 20(%esp)                  # 4-byte Spill
	setb	60(%esp)                        # 1-byte Folded Spill
	movl	44(%esp), %ecx                  # 4-byte Reload
	imull	%esi, %ecx
	movl	%ecx, %eax
	mull	96(%esp)                        # 4-byte Folded Reload
	movl	%edx, 16(%esp)                  # 4-byte Spill
	movl	%eax, 56(%esp)                  # 4-byte Spill
	movl	%ecx, %eax
	mull	84(%esp)                        # 4-byte Folded Reload
	movl	%edx, 4(%esp)                   # 4-byte Spill
	movl	%eax, 52(%esp)                  # 4-byte Spill
	movl	%ecx, %eax
	mull	92(%esp)                        # 4-byte Folded Reload
	movl	%edx, %edi
	movl	%eax, 48(%esp)                  # 4-byte Spill
	movl	%ecx, %eax
	mull	88(%esp)                        # 4-byte Folded Reload
	movl	%edx, 8(%esp)                   # 4-byte Spill
	movl	%eax, %esi
	movl	%ecx, %eax
	mull	76(%esp)                        # 4-byte Folded Reload
	movl	%edx, %ebp
	movl	%eax, 28(%esp)                  # 4-byte Spill
	movl	%ecx, %eax
	mull	68(%esp)                        # 4-byte Folded Reload
	movl	%edx, %ebx
	movl	%eax, 64(%esp)                  # 4-byte Spill
	movl	%ecx, %eax
	mull	72(%esp)                        # 4-byte Folded Reload
	addl	%ebx, %eax
	movl	%eax, %ebx
	adcl	28(%esp), %edx                  # 4-byte Folded Reload
	movl	%edx, 28(%esp)                  # 4-byte Spill
	adcl	%esi, %ebp
	movl	%ebp, %esi
	movl	8(%esp), %ecx                   # 4-byte Reload
	adcl	48(%esp), %ecx                  # 4-byte Folded Reload
	adcl	52(%esp), %edi                  # 4-byte Folded Reload
	movl	4(%esp), %ebp                   # 4-byte Reload
	adcl	56(%esp), %ebp                  # 4-byte Folded Reload
	movzbl	60(%esp), %eax                  # 1-byte Folded Reload
	movl	16(%esp), %edx                  # 4-byte Reload
	adcl	%eax, %edx
	movl	64(%esp), %eax                  # 4-byte Reload
	addl	36(%esp), %eax                  # 4-byte Folded Reload
	adcl	32(%esp), %ebx                  # 4-byte Folded Reload
	movl	%ebx, 36(%esp)                  # 4-byte Spill
	movl	40(%esp), %eax                  # 4-byte Reload
	adcl	%eax, 28(%esp)                  # 4-byte Folded Spill
	adcl	24(%esp), %esi                  # 4-byte Folded Reload
	movl	%esi, 40(%esp)                  # 4-byte Spill
	adcl	12(%esp), %ecx                  # 4-byte Folded Reload
	movl	%ecx, 8(%esp)                   # 4-byte Spill
	adcl	(%esp), %edi                    # 4-byte Folded Reload
	movl	%edi, (%esp)                    # 4-byte Spill
	adcl	20(%esp), %ebp                  # 4-byte Folded Reload
	movl	%ebp, 4(%esp)                   # 4-byte Spill
	movl	124(%esp), %eax
	adcl	48(%eax), %edx
	movl	%edx, 16(%esp)                  # 4-byte Spill
	setb	12(%esp)                        # 1-byte Folded Spill
	movl	44(%esp), %esi                  # 4-byte Reload
	imull	%ebx, %esi
	movl	%esi, %eax
	mull	96(%esp)                        # 4-byte Folded Reload
	movl	%edx, 24(%esp)                  # 4-byte Spill
	movl	%eax, 32(%esp)                  # 4-byte Spill
	movl	%esi, %eax
	mull	84(%esp)                        # 4-byte Folded Reload
	movl	%edx, %edi
	movl	%eax, 44(%esp)                  # 4-byte Spill
	movl	%esi, %eax
	mull	92(%esp)                        # 4-byte Folded Reload
	movl	%edx, %ebp
	movl	%eax, 60(%esp)                  # 4-byte Spill
	movl	%esi, %eax
	mull	68(%esp)                        # 4-byte Folded Reload
	movl	%edx, 56(%esp)                  # 4-byte Spill
	movl	%eax, 20(%esp)                  # 4-byte Spill
	movl	%esi, %eax
	mull	88(%esp)                        # 4-byte Folded Reload
	movl	%edx, %ebx
	movl	%eax, 52(%esp)                  # 4-byte Spill
	movl	%esi, %eax
	mull	76(%esp)                        # 4-byte Folded Reload
	movl	%edx, %ecx
	movl	%eax, 48(%esp)                  # 4-byte Spill
	movl	%esi, %eax
	mull	72(%esp)                        # 4-byte Folded Reload
	addl	56(%esp), %eax                  # 4-byte Folded Reload
	adcl	48(%esp), %edx                  # 4-byte Folded Reload
	adcl	52(%esp), %ecx                  # 4-byte Folded Reload
	adcl	60(%esp), %ebx                  # 4-byte Folded Reload
	adcl	44(%esp), %ebp                  # 4-byte Folded Reload
	movl	%ebp, 44(%esp)                  # 4-byte Spill
	movl	%edi, %esi
	adcl	32(%esp), %esi                  # 4-byte Folded Reload
	movzbl	12(%esp), %edi                  # 1-byte Folded Reload
	adcl	24(%esp), %edi                  # 4-byte Folded Reload
	movl	20(%esp), %ebp                  # 4-byte Reload
	addl	36(%esp), %ebp                  # 4-byte Folded Reload
	adcl	28(%esp), %eax                  # 4-byte Folded Reload
	adcl	40(%esp), %edx                  # 4-byte Folded Reload
	adcl	8(%esp), %ecx                   # 4-byte Folded Reload
	adcl	(%esp), %ebx                    # 4-byte Folded Reload
	movl	44(%esp), %ebp                  # 4-byte Reload
	adcl	4(%esp), %ebp                   # 4-byte Folded Reload
	adcl	16(%esp), %esi                  # 4-byte Folded Reload
	movl	%esi, (%esp)                    # 4-byte Spill
	movl	124(%esp), %esi
	adcl	52(%esi), %edi
	movl	%eax, 4(%esp)                   # 4-byte Spill
	subl	68(%esp), %eax                  # 4-byte Folded Reload
	movl	%eax, 68(%esp)                  # 4-byte Spill
	movl	%edx, 16(%esp)                  # 4-byte Spill
	sbbl	72(%esp), %edx                  # 4-byte Folded Reload
	movl	%edx, 72(%esp)                  # 4-byte Spill
	movl	%ecx, 12(%esp)                  # 4-byte Spill
	sbbl	76(%esp), %ecx                  # 4-byte Folded Reload
	movl	%ecx, 76(%esp)                  # 4-byte Spill
	movl	%ebx, 8(%esp)                   # 4-byte Spill
	movl	%ebx, %edx
	movl	(%esp), %esi                    # 4-byte Reload
	sbbl	88(%esp), %edx                  # 4-byte Folded Reload
	movl	%ebp, %ecx
	movl	%ebp, 44(%esp)                  # 4-byte Spill
	sbbl	92(%esp), %ecx                  # 4-byte Folded Reload
	movl	%esi, %ebp
	sbbl	84(%esp), %esi                  # 4-byte Folded Reload
	movl	%edi, %eax
	sbbl	96(%esp), %eax                  # 4-byte Folded Reload
	movl	$0, %ebx
	sbbl	%ebx, %ebx
	testb	$1, %bl
	jne	.LBB28_1
# %bb.2:
	movl	120(%esp), %edi
	movl	%eax, 24(%edi)
	jne	.LBB28_3
.LBB28_4:
	movl	%esi, 20(%edi)
	movl	68(%esp), %eax                  # 4-byte Reload
	jne	.LBB28_5
.LBB28_6:
	movl	%ecx, 16(%edi)
	jne	.LBB28_7
.LBB28_8:
	movl	%edx, 12(%edi)
	movl	72(%esp), %ecx                  # 4-byte Reload
	movl	76(%esp), %edx                  # 4-byte Reload
	jne	.LBB28_9
.LBB28_10:
	movl	%edx, 8(%edi)
	jne	.LBB28_11
.LBB28_12:
	movl	%ecx, 4(%edi)
	je	.LBB28_14
.LBB28_13:
	movl	4(%esp), %eax                   # 4-byte Reload
.LBB28_14:
	movl	%eax, (%edi)
	addl	$100, %esp
	popl	%esi
	popl	%edi
	popl	%ebx
	popl	%ebp
	retl
.LBB28_1:
	movl	%edi, %eax
	movl	120(%esp), %edi
	movl	%eax, 24(%edi)
	je	.LBB28_4
.LBB28_3:
	movl	%ebp, %esi
	movl	%esi, 20(%edi)
	movl	68(%esp), %eax                  # 4-byte Reload
	je	.LBB28_6
.LBB28_5:
	movl	44(%esp), %ecx                  # 4-byte Reload
	movl	%ecx, 16(%edi)
	je	.LBB28_8
.LBB28_7:
	movl	8(%esp), %edx                   # 4-byte Reload
	movl	%edx, 12(%edi)
	movl	72(%esp), %ecx                  # 4-byte Reload
	movl	76(%esp), %edx                  # 4-byte Reload
	je	.LBB28_10
.LBB28_9:
	movl	12(%esp), %edx                  # 4-byte Reload
	movl	%edx, 8(%edi)
	je	.LBB28_12
.LBB28_11:
	movl	16(%esp), %ecx                  # 4-byte Reload
	movl	%ecx, 4(%edi)
	jne	.LBB28_13
	jmp	.LBB28_14
.Lfunc_end28:
	.size	mcl_fp_montRed7L, .Lfunc_end28-mcl_fp_montRed7L
                                        # -- End function
	.globl	mcl_fp_montRedNF7L              # -- Begin function mcl_fp_montRedNF7L
	.p2align	4, 0x90
	.type	mcl_fp_montRedNF7L,@function
mcl_fp_montRedNF7L:                     # @mcl_fp_montRedNF7L
# %bb.0:
	pushl	%ebp
	pushl	%ebx
	pushl	%edi
	pushl	%esi
	subl	$100, %esp
	movl	128(%esp), %ebx
	movl	-4(%ebx), %eax
	movl	%eax, 44(%esp)                  # 4-byte Spill
	movl	124(%esp), %ecx
	movl	(%ecx), %edi
	movl	%edi, 16(%esp)                  # 4-byte Spill
	imull	%eax, %edi
	movl	24(%ebx), %ecx
	movl	%ecx, 88(%esp)                  # 4-byte Spill
	movl	%edi, %eax
	mull	%ecx
	movl	%eax, 12(%esp)                  # 4-byte Spill
	movl	%edx, 20(%esp)                  # 4-byte Spill
	movl	20(%ebx), %ecx
	movl	%ecx, 96(%esp)                  # 4-byte Spill
	movl	%edi, %eax
	mull	%ecx
	movl	%eax, 36(%esp)                  # 4-byte Spill
	movl	%edx, 40(%esp)                  # 4-byte Spill
	movl	16(%ebx), %ecx
	movl	%ecx, 92(%esp)                  # 4-byte Spill
	movl	%edi, %eax
	mull	%ecx
	movl	%eax, 28(%esp)                  # 4-byte Spill
	movl	%edx, 24(%esp)                  # 4-byte Spill
	movl	12(%ebx), %ecx
	movl	%ecx, 84(%esp)                  # 4-byte Spill
	movl	%edi, %eax
	mull	%ecx
	movl	%eax, 8(%esp)                   # 4-byte Spill
	movl	%edx, 32(%esp)                  # 4-byte Spill
	movl	8(%ebx), %ecx
	movl	%ecx, 76(%esp)                  # 4-byte Spill
	movl	%edi, %eax
	mull	%ecx
	movl	%eax, 4(%esp)                   # 4-byte Spill
	movl	%edx, %ecx
	movl	(%ebx), %ebp
	movl	%ebp, 68(%esp)                  # 4-byte Spill
	movl	4(%ebx), %edx
	movl	%edx, 72(%esp)                  # 4-byte Spill
	movl	%edi, %eax
	mull	%edx
	movl	%edx, %esi
	movl	%eax, %ebx
	movl	%edi, %eax
	mull	%ebp
	movl	%eax, %ebp
	addl	%ebx, %edx
	movl	%edx, (%esp)                    # 4-byte Spill
	movl	%esi, %edi
	adcl	4(%esp), %edi                   # 4-byte Folded Reload
	adcl	8(%esp), %ecx                   # 4-byte Folded Reload
	movl	%ecx, 8(%esp)                   # 4-byte Spill
	movl	32(%esp), %esi                  # 4-byte Reload
	adcl	28(%esp), %esi                  # 4-byte Folded Reload
	movl	24(%esp), %ebx                  # 4-byte Reload
	adcl	36(%esp), %ebx                  # 4-byte Folded Reload
	movl	40(%esp), %edx                  # 4-byte Reload
	adcl	12(%esp), %edx                  # 4-byte Folded Reload
	movl	20(%esp), %ecx                  # 4-byte Reload
	adcl	$0, %ecx
	addl	16(%esp), %ebp                  # 4-byte Folded Reload
	movl	(%esp), %ebp                    # 4-byte Reload
	movl	124(%esp), %eax
	adcl	4(%eax), %ebp
	movl	%ebp, (%esp)                    # 4-byte Spill
	movl	124(%esp), %ebp
	adcl	8(%ebp), %edi
	movl	%edi, 60(%esp)                  # 4-byte Spill
	movl	8(%esp), %eax                   # 4-byte Reload
	adcl	12(%ebp), %eax
	movl	%eax, 8(%esp)                   # 4-byte Spill
	adcl	16(%ebp), %esi
	movl	%esi, 32(%esp)                  # 4-byte Spill
	adcl	20(%ebp), %ebx
	movl	%ebx, 24(%esp)                  # 4-byte Spill
	adcl	24(%ebp), %edx
	movl	%edx, 40(%esp)                  # 4-byte Spill
	adcl	28(%ebp), %ecx
	movl	%ecx, 20(%esp)                  # 4-byte Spill
	setb	56(%esp)                        # 1-byte Folded Spill
	movl	44(%esp), %ecx                  # 4-byte Reload
	imull	(%esp), %ecx                    # 4-byte Folded Reload
	movl	%ecx, %eax
	mull	88(%esp)                        # 4-byte Folded Reload
	movl	%edx, 12(%esp)                  # 4-byte Spill
	movl	%eax, 52(%esp)                  # 4-byte Spill
	movl	%ecx, %eax
	mull	96(%esp)                        # 4-byte Folded Reload
	movl	%edx, 16(%esp)                  # 4-byte Spill
	movl	%eax, 48(%esp)                  # 4-byte Spill
	movl	%ecx, %eax
	mull	92(%esp)                        # 4-byte Folded Reload
	movl	%edx, 36(%esp)                  # 4-byte Spill
	movl	%eax, 64(%esp)                  # 4-byte Spill
	movl	%ecx, %eax
	mull	84(%esp)                        # 4-byte Folded Reload
	movl	%edx, 28(%esp)                  # 4-byte Spill
	movl	%eax, %edi
	movl	%ecx, %eax
	mull	76(%esp)                        # 4-byte Folded Reload
	movl	%edx, %esi
	movl	%eax, 4(%esp)                   # 4-byte Spill
	movl	%ecx, %eax
	mull	68(%esp)                        # 4-byte Folded Reload
	movl	%edx, %ebp
	movl	%eax, 80(%esp)                  # 4-byte Spill
	movl	%ecx, %eax
	mull	72(%esp)                        # 4-byte Folded Reload
	addl	%ebp, %eax
	movl	%eax, %ebx
	adcl	4(%esp), %edx                   # 4-byte Folded Reload
	movl	%edx, 4(%esp)                   # 4-byte Spill
	adcl	%edi, %esi
	movl	%esi, %edi
	movl	28(%esp), %esi                  # 4-byte Reload
	adcl	64(%esp), %esi                  # 4-byte Folded Reload
	movl	36(%esp), %ecx                  # 4-byte Reload
	adcl	48(%esp), %ecx                  # 4-byte Folded Reload
	movl	16(%esp), %ebp                  # 4-byte Reload
	adcl	52(%esp), %ebp                  # 4-byte Folded Reload
	movzbl	56(%esp), %eax                  # 1-byte Folded Reload
	movl	12(%esp), %edx                  # 4-byte Reload
	adcl	%eax, %edx
	movl	80(%esp), %eax                  # 4-byte Reload
	addl	(%esp), %eax                    # 4-byte Folded Reload
	adcl	60(%esp), %ebx                  # 4-byte Folded Reload
	movl	%ebx, 56(%esp)                  # 4-byte Spill
	movl	8(%esp), %eax                   # 4-byte Reload
	adcl	%eax, 4(%esp)                   # 4-byte Folded Spill
	adcl	32(%esp), %edi                  # 4-byte Folded Reload
	movl	%edi, 60(%esp)                  # 4-byte Spill
	adcl	24(%esp), %esi                  # 4-byte Folded Reload
	movl	%esi, 28(%esp)                  # 4-byte Spill
	adcl	40(%esp), %ecx                  # 4-byte Folded Reload
	movl	%ecx, 36(%esp)                  # 4-byte Spill
	adcl	20(%esp), %ebp                  # 4-byte Folded Reload
	movl	%ebp, 16(%esp)                  # 4-byte Spill
	movl	124(%esp), %eax
	adcl	32(%eax), %edx
	movl	%edx, 12(%esp)                  # 4-byte Spill
	setb	20(%esp)                        # 1-byte Folded Spill
	movl	44(%esp), %edi                  # 4-byte Reload
	imull	%ebx, %edi
	movl	%edi, %eax
	mull	88(%esp)                        # 4-byte Folded Reload
	movl	%edx, 24(%esp)                  # 4-byte Spill
	movl	%eax, %esi
	movl	%edi, %eax
	mull	96(%esp)                        # 4-byte Folded Reload
	movl	%edx, (%esp)                    # 4-byte Spill
	movl	%eax, 52(%esp)                  # 4-byte Spill
	movl	%edi, %eax
	mull	92(%esp)                        # 4-byte Folded Reload
	movl	%edx, 40(%esp)                  # 4-byte Spill
	movl	%eax, 48(%esp)                  # 4-byte Spill
	movl	%edi, %eax
	mull	84(%esp)                        # 4-byte Folded Reload
	movl	%edx, %ebp
	movl	%eax, 80(%esp)                  # 4-byte Spill
	movl	%edi, %eax
	mull	76(%esp)                        # 4-byte Folded Reload
	movl	%edx, 32(%esp)                  # 4-byte Spill
	movl	%eax, %ebx
	movl	%edi, %eax
	mull	68(%esp)                        # 4-byte Folded Reload
	movl	%edx, %ecx
	movl	%eax, 64(%esp)                  # 4-byte Spill
	movl	%edi, %eax
	mull	72(%esp)                        # 4-byte Folded Reload
	addl	%ecx, %eax
	movl	%eax, 8(%esp)                   # 4-byte Spill
	adcl	%ebx, %edx
	movl	%edx, %ebx
	movl	32(%esp), %edi                  # 4-byte Reload
	adcl	80(%esp), %edi                  # 4-byte Folded Reload
	movl	%ebp, %ecx
	adcl	48(%esp), %ecx                  # 4-byte Folded Reload
	movl	40(%esp), %edx                  # 4-byte Reload
	adcl	52(%esp), %edx                  # 4-byte Folded Reload
	movl	(%esp), %eax                    # 4-byte Reload
	adcl	%esi, %eax
	movzbl	20(%esp), %esi                  # 1-byte Folded Reload
	adcl	24(%esp), %esi                  # 4-byte Folded Reload
	movl	64(%esp), %ebp                  # 4-byte Reload
	addl	56(%esp), %ebp                  # 4-byte Folded Reload
	movl	8(%esp), %ebp                   # 4-byte Reload
	adcl	4(%esp), %ebp                   # 4-byte Folded Reload
	movl	%ebp, 8(%esp)                   # 4-byte Spill
	adcl	60(%esp), %ebx                  # 4-byte Folded Reload
	movl	%ebx, 60(%esp)                  # 4-byte Spill
	adcl	28(%esp), %edi                  # 4-byte Folded Reload
	movl	%edi, 32(%esp)                  # 4-byte Spill
	adcl	36(%esp), %ecx                  # 4-byte Folded Reload
	movl	%ecx, 28(%esp)                  # 4-byte Spill
	adcl	16(%esp), %edx                  # 4-byte Folded Reload
	movl	%edx, 40(%esp)                  # 4-byte Spill
	adcl	12(%esp), %eax                  # 4-byte Folded Reload
	movl	%eax, (%esp)                    # 4-byte Spill
	movl	124(%esp), %eax
	adcl	36(%eax), %esi
	movl	%esi, 12(%esp)                  # 4-byte Spill
	setb	56(%esp)                        # 1-byte Folded Spill
	movl	44(%esp), %ecx                  # 4-byte Reload
	imull	%ebp, %ecx
	movl	%ecx, %eax
	mull	88(%esp)                        # 4-byte Folded Reload
	movl	%edx, 16(%esp)                  # 4-byte Spill
	movl	%eax, 52(%esp)                  # 4-byte Spill
	movl	%ecx, %eax
	mull	96(%esp)                        # 4-byte Folded Reload
	movl	%edx, 20(%esp)                  # 4-byte Spill
	movl	%eax, 48(%esp)                  # 4-byte Spill
	movl	%ecx, %eax
	mull	92(%esp)                        # 4-byte Folded Reload
	movl	%edx, 24(%esp)                  # 4-byte Spill
	movl	%eax, 64(%esp)                  # 4-byte Spill
	movl	%ecx, %eax
	mull	84(%esp)                        # 4-byte Folded Reload
	movl	%edx, 36(%esp)                  # 4-byte Spill
	movl	%eax, %ebx
	movl	%ecx, %eax
	mull	76(%esp)                        # 4-byte Folded Reload
	movl	%edx, %edi
	movl	%eax, 4(%esp)                   # 4-byte Spill
	movl	%ecx, %eax
	mull	68(%esp)                        # 4-byte Folded Reload
	movl	%edx, %esi
	movl	%eax, 80(%esp)                  # 4-byte Spill
	movl	%ecx, %eax
	mull	72(%esp)                        # 4-byte Folded Reload
	movl	%eax, %ebp
	addl	%esi, %ebp
	adcl	4(%esp), %edx                   # 4-byte Folded Reload
	movl	%edx, 4(%esp)                   # 4-byte Spill
	adcl	%ebx, %edi
	movl	%edi, %ebx
	movl	36(%esp), %edi                  # 4-byte Reload
	adcl	64(%esp), %edi                  # 4-byte Folded Reload
	movl	24(%esp), %ecx                  # 4-byte Reload
	adcl	48(%esp), %ecx                  # 4-byte Folded Reload
	movl	20(%esp), %esi                  # 4-byte Reload
	adcl	52(%esp), %esi                  # 4-byte Folded Reload
	movzbl	56(%esp), %eax                  # 1-byte Folded Reload
	movl	16(%esp), %edx                  # 4-byte Reload
	adcl	%eax, %edx
	movl	80(%esp), %eax                  # 4-byte Reload
	addl	8(%esp), %eax                   # 4-byte Folded Reload
	adcl	60(%esp), %ebp                  # 4-byte Folded Reload
	movl	32(%esp), %eax                  # 4-byte Reload
	adcl	%eax, 4(%esp)                   # 4-byte Folded Spill
	adcl	28(%esp), %ebx                  # 4-byte Folded Reload
	movl	%ebx, 8(%esp)                   # 4-byte Spill
	adcl	40(%esp), %edi                  # 4-byte Folded Reload
	movl	%edi, 36(%esp)                  # 4-byte Spill
	adcl	(%esp), %ecx                    # 4-byte Folded Reload
	movl	%ecx, 24(%esp)                  # 4-byte Spill
	adcl	12(%esp), %esi                  # 4-byte Folded Reload
	movl	%esi, 20(%esp)                  # 4-byte Spill
	movl	124(%esp), %eax
	adcl	40(%eax), %edx
	movl	%edx, 16(%esp)                  # 4-byte Spill
	setb	40(%esp)                        # 1-byte Folded Spill
	movl	44(%esp), %esi                  # 4-byte Reload
	movl	%ebp, 60(%esp)                  # 4-byte Spill
	imull	%ebp, %esi
	movl	%esi, %eax
	mull	88(%esp)                        # 4-byte Folded Reload
	movl	%edx, 56(%esp)                  # 4-byte Spill
	movl	%eax, %edi
	movl	%esi, %eax
	mull	96(%esp)                        # 4-byte Folded Reload
	movl	%edx, (%esp)                    # 4-byte Spill
	movl	%eax, 52(%esp)                  # 4-byte Spill
	movl	%esi, %eax
	mull	92(%esp)                        # 4-byte Folded Reload
	movl	%edx, 12(%esp)                  # 4-byte Spill
	movl	%eax, 48(%esp)                  # 4-byte Spill
	movl	%esi, %eax
	mull	84(%esp)                        # 4-byte Folded Reload
	movl	%edx, %ebx
	movl	%eax, 80(%esp)                  # 4-byte Spill
	movl	%esi, %eax
	mull	76(%esp)                        # 4-byte Folded Reload
	movl	%edx, 32(%esp)                  # 4-byte Spill
	movl	%eax, %ecx
	movl	%esi, %eax
	mull	68(%esp)                        # 4-byte Folded Reload
	movl	%edx, %ebp
	movl	%eax, 64(%esp)                  # 4-byte Spill
	movl	%esi, %eax
	mull	72(%esp)                        # 4-byte Folded Reload
	addl	%ebp, %eax
	movl	%eax, 28(%esp)                  # 4-byte Spill
	adcl	%ecx, %edx
	movl	%edx, %ecx
	movl	32(%esp), %ebp                  # 4-byte Reload
	adcl	80(%esp), %ebp                  # 4-byte Folded Reload
	adcl	48(%esp), %ebx                  # 4-byte Folded Reload
	movl	12(%esp), %edx                  # 4-byte Reload
	adcl	52(%esp), %edx                  # 4-byte Folded Reload
	movl	(%esp), %eax                    # 4-byte Reload
	adcl	%edi, %eax
	movzbl	40(%esp), %edi                  # 1-byte Folded Reload
	adcl	56(%esp), %edi                  # 4-byte Folded Reload
	movl	64(%esp), %esi                  # 4-byte Reload
	addl	60(%esp), %esi                  # 4-byte Folded Reload
	movl	28(%esp), %esi                  # 4-byte Reload
	adcl	4(%esp), %esi                   # 4-byte Folded Reload
	movl	%esi, 28(%esp)                  # 4-byte Spill
	adcl	8(%esp), %ecx                   # 4-byte Folded Reload
	movl	%ecx, 8(%esp)                   # 4-byte Spill
	adcl	36(%esp), %ebp                  # 4-byte Folded Reload
	movl	%ebp, 32(%esp)                  # 4-byte Spill
	adcl	24(%esp), %ebx                  # 4-byte Folded Reload
	movl	%ebx, 36(%esp)                  # 4-byte Spill
	adcl	20(%esp), %edx                  # 4-byte Folded Reload
	movl	%edx, 12(%esp)                  # 4-byte Spill
	adcl	16(%esp), %eax                  # 4-byte Folded Reload
	movl	%eax, (%esp)                    # 4-byte Spill
	movl	124(%esp), %eax
	adcl	44(%eax), %edi
	movl	%edi, 4(%esp)                   # 4-byte Spill
	setb	60(%esp)                        # 1-byte Folded Spill
	movl	44(%esp), %ecx                  # 4-byte Reload
	imull	%esi, %ecx
	movl	%ecx, %eax
	mull	88(%esp)                        # 4-byte Folded Reload
	movl	%edx, 16(%esp)                  # 4-byte Spill
	movl	%eax, 56(%esp)                  # 4-byte Spill
	movl	%ecx, %eax
	mull	96(%esp)                        # 4-byte Folded Reload
	movl	%edx, 20(%esp)                  # 4-byte Spill
	movl	%eax, 52(%esp)                  # 4-byte Spill
	movl	%ecx, %eax
	mull	92(%esp)                        # 4-byte Folded Reload
	movl	%edx, %edi
	movl	%eax, 48(%esp)                  # 4-byte Spill
	movl	%ecx, %eax
	mull	84(%esp)                        # 4-byte Folded Reload
	movl	%edx, 40(%esp)                  # 4-byte Spill
	movl	%eax, %esi
	movl	%ecx, %eax
	mull	76(%esp)                        # 4-byte Folded Reload
	movl	%edx, %ebp
	movl	%eax, 24(%esp)                  # 4-byte Spill
	movl	%ecx, %eax
	mull	68(%esp)                        # 4-byte Folded Reload
	movl	%edx, %ebx
	movl	%eax, 64(%esp)                  # 4-byte Spill
	movl	%ecx, %eax
	mull	72(%esp)                        # 4-byte Folded Reload
	addl	%ebx, %eax
	movl	%eax, %ebx
	adcl	24(%esp), %edx                  # 4-byte Folded Reload
	movl	%edx, 24(%esp)                  # 4-byte Spill
	adcl	%esi, %ebp
	movl	%ebp, %esi
	movl	40(%esp), %ecx                  # 4-byte Reload
	adcl	48(%esp), %ecx                  # 4-byte Folded Reload
	adcl	52(%esp), %edi                  # 4-byte Folded Reload
	movl	20(%esp), %ebp                  # 4-byte Reload
	adcl	56(%esp), %ebp                  # 4-byte Folded Reload
	movzbl	60(%esp), %eax                  # 1-byte Folded Reload
	movl	16(%esp), %edx                  # 4-byte Reload
	adcl	%eax, %edx
	movl	64(%esp), %eax                  # 4-byte Reload
	addl	28(%esp), %eax                  # 4-byte Folded Reload
	adcl	8(%esp), %ebx                   # 4-byte Folded Reload
	movl	%ebx, 28(%esp)                  # 4-byte Spill
	movl	32(%esp), %eax                  # 4-byte Reload
	adcl	%eax, 24(%esp)                  # 4-byte Folded Spill
	adcl	36(%esp), %esi                  # 4-byte Folded Reload
	movl	%esi, 36(%esp)                  # 4-byte Spill
	adcl	12(%esp), %ecx                  # 4-byte Folded Reload
	movl	%ecx, 40(%esp)                  # 4-byte Spill
	adcl	(%esp), %edi                    # 4-byte Folded Reload
	movl	%edi, (%esp)                    # 4-byte Spill
	adcl	4(%esp), %ebp                   # 4-byte Folded Reload
	movl	%ebp, 20(%esp)                  # 4-byte Spill
	movl	124(%esp), %eax
	adcl	48(%eax), %edx
	movl	%edx, 16(%esp)                  # 4-byte Spill
	setb	12(%esp)                        # 1-byte Folded Spill
	movl	44(%esp), %ecx                  # 4-byte Reload
	imull	%ebx, %ecx
	movl	%ecx, %eax
	mull	88(%esp)                        # 4-byte Folded Reload
	movl	%edx, 32(%esp)                  # 4-byte Spill
	movl	%eax, 8(%esp)                   # 4-byte Spill
	movl	%ecx, %eax
	mull	96(%esp)                        # 4-byte Folded Reload
	movl	%edx, %ebp
	movl	%eax, 44(%esp)                  # 4-byte Spill
	movl	%ecx, %eax
	mull	92(%esp)                        # 4-byte Folded Reload
	movl	%edx, %ebx
	movl	%eax, 60(%esp)                  # 4-byte Spill
	movl	%ecx, %eax
	mull	68(%esp)                        # 4-byte Folded Reload
	movl	%edx, 56(%esp)                  # 4-byte Spill
	movl	%eax, 4(%esp)                   # 4-byte Spill
	movl	%ecx, %eax
	mull	84(%esp)                        # 4-byte Folded Reload
	movl	%edx, %edi
	movl	%eax, 52(%esp)                  # 4-byte Spill
	movl	%ecx, %eax
	mull	76(%esp)                        # 4-byte Folded Reload
	movl	%edx, %esi
	movl	%eax, 48(%esp)                  # 4-byte Spill
	movl	%ecx, %eax
	mull	72(%esp)                        # 4-byte Folded Reload
	movl	%edx, %ecx
	movl	%eax, %edx
	addl	56(%esp), %edx                  # 4-byte Folded Reload
	adcl	48(%esp), %ecx                  # 4-byte Folded Reload
	adcl	52(%esp), %esi                  # 4-byte Folded Reload
	adcl	60(%esp), %edi                  # 4-byte Folded Reload
	adcl	44(%esp), %ebx                  # 4-byte Folded Reload
	movl	%ebx, 44(%esp)                  # 4-byte Spill
	movl	%ebp, %eax
	adcl	8(%esp), %eax                   # 4-byte Folded Reload
	movzbl	12(%esp), %ebp                  # 1-byte Folded Reload
	adcl	32(%esp), %ebp                  # 4-byte Folded Reload
	movl	4(%esp), %ebx                   # 4-byte Reload
	addl	28(%esp), %ebx                  # 4-byte Folded Reload
	adcl	24(%esp), %edx                  # 4-byte Folded Reload
	adcl	36(%esp), %ecx                  # 4-byte Folded Reload
	adcl	40(%esp), %esi                  # 4-byte Folded Reload
	adcl	(%esp), %edi                    # 4-byte Folded Reload
	movl	44(%esp), %ebx                  # 4-byte Reload
	adcl	20(%esp), %ebx                  # 4-byte Folded Reload
	adcl	16(%esp), %eax                  # 4-byte Folded Reload
	movl	%eax, (%esp)                    # 4-byte Spill
	movl	124(%esp), %eax
	adcl	52(%eax), %ebp
	movl	%edx, 20(%esp)                  # 4-byte Spill
	subl	68(%esp), %edx                  # 4-byte Folded Reload
	movl	%edx, 68(%esp)                  # 4-byte Spill
	movl	%ecx, 16(%esp)                  # 4-byte Spill
	sbbl	72(%esp), %ecx                  # 4-byte Folded Reload
	movl	%ecx, 72(%esp)                  # 4-byte Spill
	movl	%esi, 12(%esp)                  # 4-byte Spill
	movl	%esi, %edx
	sbbl	76(%esp), %edx                  # 4-byte Folded Reload
	movl	%edi, 76(%esp)                  # 4-byte Spill
	movl	%edi, %esi
	sbbl	84(%esp), %esi                  # 4-byte Folded Reload
	movl	%ebx, %ecx
	movl	%ebx, 44(%esp)                  # 4-byte Spill
	sbbl	92(%esp), %ecx                  # 4-byte Folded Reload
	movl	(%esp), %ebx                    # 4-byte Reload
	sbbl	96(%esp), %ebx                  # 4-byte Folded Reload
	movl	%ebp, %edi
	sbbl	88(%esp), %edi                  # 4-byte Folded Reload
	movl	%edi, %eax
	sarl	$31, %eax
	testl	%eax, %eax
	js	.LBB29_1
# %bb.2:
	movl	120(%esp), %eax
	movl	%edi, 24(%eax)
	js	.LBB29_3
.LBB29_4:
	movl	%ebx, 20(%eax)
	js	.LBB29_5
.LBB29_6:
	movl	%ecx, 16(%eax)
	js	.LBB29_7
.LBB29_8:
	movl	%esi, 12(%eax)
	movl	68(%esp), %ecx                  # 4-byte Reload
	js	.LBB29_9
.LBB29_10:
	movl	%edx, 8(%eax)
	movl	72(%esp), %edx                  # 4-byte Reload
	js	.LBB29_11
.LBB29_12:
	movl	%edx, 4(%eax)
	jns	.LBB29_14
.LBB29_13:
	movl	20(%esp), %ecx                  # 4-byte Reload
.LBB29_14:
	movl	%ecx, (%eax)
	addl	$100, %esp
	popl	%esi
	popl	%edi
	popl	%ebx
	popl	%ebp
	retl
.LBB29_1:
	movl	%ebp, %edi
	movl	120(%esp), %eax
	movl	%edi, 24(%eax)
	jns	.LBB29_4
.LBB29_3:
	movl	(%esp), %ebx                    # 4-byte Reload
	movl	%ebx, 20(%eax)
	jns	.LBB29_6
.LBB29_5:
	movl	44(%esp), %ecx                  # 4-byte Reload
	movl	%ecx, 16(%eax)
	jns	.LBB29_8
.LBB29_7:
	movl	76(%esp), %esi                  # 4-byte Reload
	movl	%esi, 12(%eax)
	movl	68(%esp), %ecx                  # 4-byte Reload
	jns	.LBB29_10
.LBB29_9:
	movl	12(%esp), %edx                  # 4-byte Reload
	movl	%edx, 8(%eax)
	movl	72(%esp), %edx                  # 4-byte Reload
	jns	.LBB29_12
.LBB29_11:
	movl	16(%esp), %edx                  # 4-byte Reload
	movl	%edx, 4(%eax)
	js	.LBB29_13
	jmp	.LBB29_14
.Lfunc_end29:
	.size	mcl_fp_montRedNF7L, .Lfunc_end29-mcl_fp_montRedNF7L
                                        # -- End function
	.globl	mcl_fp_addPre7L                 # -- Begin function mcl_fp_addPre7L
	.p2align	4, 0x90
	.type	mcl_fp_addPre7L,@function
mcl_fp_addPre7L:                        # @mcl_fp_addPre7L
# %bb.0:
	pushl	%ebp
	pushl	%ebx
	pushl	%edi
	pushl	%esi
	pushl	%eax
	movl	28(%esp), %eax
	movl	(%eax), %ecx
	movl	4(%eax), %edx
	movl	32(%esp), %esi
	addl	(%esi), %ecx
	movl	%ecx, (%esp)                    # 4-byte Spill
	adcl	4(%esi), %edx
	movl	24(%eax), %edi
	movl	20(%eax), %ebx
	movl	16(%eax), %ebp
	movl	12(%eax), %ecx
	movl	8(%eax), %eax
	adcl	8(%esi), %eax
	adcl	12(%esi), %ecx
	adcl	16(%esi), %ebp
	adcl	20(%esi), %ebx
	adcl	24(%esi), %edi
	movl	24(%esp), %esi
	movl	%ebx, 20(%esi)
	movl	%ebp, 16(%esi)
	movl	%ecx, 12(%esi)
	movl	%eax, 8(%esi)
	movl	%edi, 24(%esi)
	movl	%edx, 4(%esi)
	movl	(%esp), %eax                    # 4-byte Reload
	movl	%eax, (%esi)
	setb	%al
	movzbl	%al, %eax
	addl	$4, %esp
	popl	%esi
	popl	%edi
	popl	%ebx
	popl	%ebp
	retl
.Lfunc_end30:
	.size	mcl_fp_addPre7L, .Lfunc_end30-mcl_fp_addPre7L
                                        # -- End function
	.globl	mcl_fp_subPre7L                 # -- Begin function mcl_fp_subPre7L
	.p2align	4, 0x90
	.type	mcl_fp_subPre7L,@function
mcl_fp_subPre7L:                        # @mcl_fp_subPre7L
# %bb.0:
	pushl	%ebp
	pushl	%ebx
	pushl	%edi
	pushl	%esi
	subl	$8, %esp
	movl	32(%esp), %ecx
	movl	(%ecx), %edx
	movl	4(%ecx), %esi
	xorl	%eax, %eax
	movl	36(%esp), %edi
	subl	(%edi), %edx
	movl	%edx, 4(%esp)                   # 4-byte Spill
	sbbl	4(%edi), %esi
	movl	%esi, (%esp)                    # 4-byte Spill
	movl	24(%ecx), %ebx
	movl	20(%ecx), %ebp
	movl	16(%ecx), %edx
	movl	12(%ecx), %esi
	movl	8(%ecx), %ecx
	sbbl	8(%edi), %ecx
	sbbl	12(%edi), %esi
	sbbl	16(%edi), %edx
	sbbl	20(%edi), %ebp
	sbbl	24(%edi), %ebx
	movl	28(%esp), %edi
	movl	%ebp, 20(%edi)
	movl	%edx, 16(%edi)
	movl	%esi, 12(%edi)
	movl	%ecx, 8(%edi)
	movl	(%esp), %ecx                    # 4-byte Reload
	movl	%ecx, 4(%edi)
	movl	%ebx, 24(%edi)
	movl	4(%esp), %ecx                   # 4-byte Reload
	movl	%ecx, (%edi)
	sbbl	%eax, %eax
	andl	$1, %eax
	addl	$8, %esp
	popl	%esi
	popl	%edi
	popl	%ebx
	popl	%ebp
	retl
.Lfunc_end31:
	.size	mcl_fp_subPre7L, .Lfunc_end31-mcl_fp_subPre7L
                                        # -- End function
	.globl	mcl_fp_shr1_7L                  # -- Begin function mcl_fp_shr1_7L
	.p2align	4, 0x90
	.type	mcl_fp_shr1_7L,@function
mcl_fp_shr1_7L:                         # @mcl_fp_shr1_7L
# %bb.0:
	pushl	%esi
	movl	12(%esp), %eax
	movl	24(%eax), %ecx
	movl	%ecx, %edx
	shrl	%edx
	movl	8(%esp), %esi
	movl	%edx, 24(%esi)
	movl	20(%eax), %edx
	shldl	$31, %edx, %ecx
	movl	%ecx, 20(%esi)
	movl	16(%eax), %ecx
	shldl	$31, %ecx, %edx
	movl	%edx, 16(%esi)
	movl	12(%eax), %edx
	shldl	$31, %edx, %ecx
	movl	%ecx, 12(%esi)
	movl	8(%eax), %ecx
	shldl	$31, %ecx, %edx
	movl	%edx, 8(%esi)
	movl	4(%eax), %edx
	shldl	$31, %edx, %ecx
	movl	%ecx, 4(%esi)
	movl	(%eax), %eax
	shrdl	$1, %edx, %eax
	movl	%eax, (%esi)
	popl	%esi
	retl
.Lfunc_end32:
	.size	mcl_fp_shr1_7L, .Lfunc_end32-mcl_fp_shr1_7L
                                        # -- End function
	.globl	mcl_fp_add7L                    # -- Begin function mcl_fp_add7L
	.p2align	4, 0x90
	.type	mcl_fp_add7L,@function
mcl_fp_add7L:                           # @mcl_fp_add7L
# %bb.0:
	pushl	%ebp
	pushl	%ebx
	pushl	%edi
	pushl	%esi
	subl	$20, %esp
	movl	44(%esp), %eax
	movl	(%eax), %ebp
	movl	4(%eax), %esi
	movl	48(%esp), %ecx
	addl	(%ecx), %ebp
	adcl	4(%ecx), %esi
	movl	24(%eax), %edi
	movl	20(%eax), %ebx
	movl	16(%eax), %edx
	movl	12(%eax), %ecx
	movl	%ecx, 4(%esp)                   # 4-byte Spill
	movl	8(%eax), %eax
	movl	48(%esp), %ecx
	adcl	8(%ecx), %eax
	movl	%eax, 16(%esp)                  # 4-byte Spill
	movl	4(%esp), %eax                   # 4-byte Reload
	adcl	12(%ecx), %eax
	movl	%eax, 4(%esp)                   # 4-byte Spill
	adcl	16(%ecx), %edx
	adcl	20(%ecx), %ebx
	adcl	24(%ecx), %edi
	movl	40(%esp), %ecx
	movl	%edi, 24(%ecx)
	movl	%ebx, 20(%ecx)
	movl	%edx, 16(%ecx)
	movl	%eax, 12(%ecx)
	movl	16(%esp), %eax                  # 4-byte Reload
	movl	%eax, 8(%ecx)
	movl	%esi, 4(%ecx)
	movl	%ebp, (%ecx)
	setb	3(%esp)                         # 1-byte Folded Spill
	movl	52(%esp), %ecx
	subl	(%ecx), %ebp
	movl	%ebp, 12(%esp)                  # 4-byte Spill
	movl	4(%esp), %ebp                   # 4-byte Reload
	sbbl	4(%ecx), %esi
	movl	%esi, 8(%esp)                   # 4-byte Spill
	sbbl	8(%ecx), %eax
	sbbl	12(%ecx), %ebp
	sbbl	16(%ecx), %edx
	sbbl	20(%ecx), %ebx
	sbbl	24(%ecx), %edi
	movzbl	3(%esp), %ecx                   # 1-byte Folded Reload
	sbbl	$0, %ecx
	testb	$1, %cl
	jne	.LBB33_2
# %bb.1:                                # %nocarry
	movl	%ebp, %esi
	movl	12(%esp), %ecx                  # 4-byte Reload
	movl	40(%esp), %ebp
	movl	%ecx, (%ebp)
	movl	8(%esp), %ecx                   # 4-byte Reload
	movl	%ecx, 4(%ebp)
	movl	%eax, 8(%ebp)
	movl	%esi, 12(%ebp)
	movl	%edx, 16(%ebp)
	movl	%ebx, 20(%ebp)
	movl	%edi, 24(%ebp)
.LBB33_2:                               # %carry
	addl	$20, %esp
	popl	%esi
	popl	%edi
	popl	%ebx
	popl	%ebp
	retl
.Lfunc_end33:
	.size	mcl_fp_add7L, .Lfunc_end33-mcl_fp_add7L
                                        # -- End function
	.globl	mcl_fp_addNF7L                  # -- Begin function mcl_fp_addNF7L
	.p2align	4, 0x90
	.type	mcl_fp_addNF7L,@function
mcl_fp_addNF7L:                         # @mcl_fp_addNF7L
# %bb.0:
	pushl	%ebp
	pushl	%ebx
	pushl	%edi
	pushl	%esi
	subl	$40, %esp
	movl	68(%esp), %ecx
	movl	(%ecx), %ebx
	movl	4(%ecx), %ebp
	movl	64(%esp), %esi
	addl	(%esi), %ebx
	adcl	4(%esi), %ebp
	movl	24(%ecx), %eax
	movl	%eax, (%esp)                    # 4-byte Spill
	movl	20(%ecx), %edx
	movl	16(%ecx), %edi
	movl	12(%ecx), %eax
	movl	8(%ecx), %ecx
	adcl	8(%esi), %ecx
	adcl	12(%esi), %eax
	adcl	16(%esi), %edi
	movl	%edi, 24(%esp)                  # 4-byte Spill
	adcl	20(%esi), %edx
	movl	(%esp), %edi                    # 4-byte Reload
	adcl	24(%esi), %edi
	movl	%edi, (%esp)                    # 4-byte Spill
	movl	72(%esp), %esi
	movl	%ebx, 20(%esp)                  # 4-byte Spill
	subl	(%esi), %ebx
	movl	%ebx, 36(%esp)                  # 4-byte Spill
	movl	%ebp, 16(%esp)                  # 4-byte Spill
	sbbl	4(%esi), %ebp
	movl	%ebp, 32(%esp)                  # 4-byte Spill
	movl	%ecx, 12(%esp)                  # 4-byte Spill
	sbbl	8(%esi), %ecx
	movl	%ecx, 28(%esp)                  # 4-byte Spill
	movl	%eax, 8(%esp)                   # 4-byte Spill
	movl	%eax, %ebx
	movl	24(%esp), %eax                  # 4-byte Reload
	movl	(%esp), %edi                    # 4-byte Reload
	sbbl	12(%esi), %ebx
	movl	%eax, %ebp
	sbbl	16(%esi), %ebp
	movl	%edx, 4(%esp)                   # 4-byte Spill
	movl	%edx, %ecx
	sbbl	20(%esi), %ecx
	movl	%edi, %edx
	sbbl	24(%esi), %edx
	movl	%edx, %esi
	sarl	$31, %esi
	testl	%esi, %esi
	js	.LBB34_1
# %bb.2:
	movl	60(%esp), %edi
	movl	%edx, 24(%edi)
	js	.LBB34_3
.LBB34_4:
	movl	%ecx, 20(%edi)
	movl	28(%esp), %edx                  # 4-byte Reload
	js	.LBB34_5
.LBB34_6:
	movl	%ebp, 16(%edi)
	movl	36(%esp), %eax                  # 4-byte Reload
	movl	32(%esp), %ecx                  # 4-byte Reload
	js	.LBB34_7
.LBB34_8:
	movl	%ebx, 12(%edi)
	js	.LBB34_9
.LBB34_10:
	movl	%edx, 8(%edi)
	js	.LBB34_11
.LBB34_12:
	movl	%ecx, 4(%edi)
	jns	.LBB34_14
.LBB34_13:
	movl	20(%esp), %eax                  # 4-byte Reload
.LBB34_14:
	movl	%eax, (%edi)
	addl	$40, %esp
	popl	%esi
	popl	%edi
	popl	%ebx
	popl	%ebp
	retl
.LBB34_1:
	movl	%edi, %edx
	movl	60(%esp), %edi
	movl	%edx, 24(%edi)
	jns	.LBB34_4
.LBB34_3:
	movl	4(%esp), %ecx                   # 4-byte Reload
	movl	%ecx, 20(%edi)
	movl	28(%esp), %edx                  # 4-byte Reload
	jns	.LBB34_6
.LBB34_5:
	movl	%eax, %ebp
	movl	%ebp, 16(%edi)
	movl	36(%esp), %eax                  # 4-byte Reload
	movl	32(%esp), %ecx                  # 4-byte Reload
	jns	.LBB34_8
.LBB34_7:
	movl	8(%esp), %ebx                   # 4-byte Reload
	movl	%ebx, 12(%edi)
	jns	.LBB34_10
.LBB34_9:
	movl	12(%esp), %edx                  # 4-byte Reload
	movl	%edx, 8(%edi)
	jns	.LBB34_12
.LBB34_11:
	movl	16(%esp), %ecx                  # 4-byte Reload
	movl	%ecx, 4(%edi)
	js	.LBB34_13
	jmp	.LBB34_14
.Lfunc_end34:
	.size	mcl_fp_addNF7L, .Lfunc_end34-mcl_fp_addNF7L
                                        # -- End function
	.globl	mcl_fp_sub7L                    # -- Begin function mcl_fp_sub7L
	.p2align	4, 0x90
	.type	mcl_fp_sub7L,@function
mcl_fp_sub7L:                           # @mcl_fp_sub7L
# %bb.0:
	pushl	%ebp
	pushl	%ebx
	pushl	%edi
	pushl	%esi
	subl	$24, %esp
	movl	48(%esp), %edx
	movl	(%edx), %ecx
	movl	4(%edx), %edi
	movl	52(%esp), %eax
	subl	(%eax), %ecx
	movl	%ecx, 4(%esp)                   # 4-byte Spill
	sbbl	4(%eax), %edi
	movl	24(%edx), %esi
	movl	20(%edx), %ecx
	movl	16(%edx), %ebp
	movl	12(%edx), %ebx
	movl	8(%edx), %edx
	sbbl	8(%eax), %edx
	sbbl	12(%eax), %ebx
	sbbl	16(%eax), %ebp
	sbbl	20(%eax), %ecx
	sbbl	24(%eax), %esi
	movl	$0, %eax
	sbbl	%eax, %eax
	testb	$1, %al
	movl	44(%esp), %eax
	movl	%esi, 8(%esp)                   # 4-byte Spill
	movl	%esi, 24(%eax)
	movl	%ecx, 12(%esp)                  # 4-byte Spill
	movl	%ecx, 20(%eax)
	movl	%ebp, 16(%esp)                  # 4-byte Spill
	movl	%ebp, 16(%eax)
	movl	%ebx, 20(%esp)                  # 4-byte Spill
	movl	%ebx, 12(%eax)
	movl	%edx, 8(%eax)
	movl	%edi, (%esp)                    # 4-byte Spill
	movl	%edi, 4(%eax)
	movl	4(%esp), %esi                   # 4-byte Reload
	movl	%esi, (%eax)
	je	.LBB35_2
# %bb.1:                                # %carry
	movl	%esi, %ecx
	movl	56(%esp), %esi
	addl	(%esi), %ecx
	movl	%ecx, 4(%esp)                   # 4-byte Spill
	movl	(%esp), %ecx                    # 4-byte Reload
	adcl	4(%esi), %ecx
	movl	%ecx, (%esp)                    # 4-byte Spill
	adcl	8(%esi), %edx
	movl	20(%esp), %ebx                  # 4-byte Reload
	adcl	12(%esi), %ebx
	movl	16(%esp), %ebp                  # 4-byte Reload
	adcl	16(%esi), %ebp
	movl	12(%esp), %ecx                  # 4-byte Reload
	adcl	20(%esi), %ecx
	movl	8(%esp), %edi                   # 4-byte Reload
	adcl	24(%esi), %edi
	movl	%edi, 24(%eax)
	movl	%ecx, 20(%eax)
	movl	%ebp, 16(%eax)
	movl	%ebx, 12(%eax)
	movl	%edx, 8(%eax)
	movl	(%esp), %ecx                    # 4-byte Reload
	movl	%ecx, 4(%eax)
	movl	4(%esp), %ecx                   # 4-byte Reload
	movl	%ecx, (%eax)
.LBB35_2:                               # %nocarry
	addl	$24, %esp
	popl	%esi
	popl	%edi
	popl	%ebx
	popl	%ebp
	retl
.Lfunc_end35:
	.size	mcl_fp_sub7L, .Lfunc_end35-mcl_fp_sub7L
                                        # -- End function
	.globl	mcl_fp_subNF7L                  # -- Begin function mcl_fp_subNF7L
	.p2align	4, 0x90
	.type	mcl_fp_subNF7L,@function
mcl_fp_subNF7L:                         # @mcl_fp_subNF7L
# %bb.0:
	pushl	%ebp
	pushl	%ebx
	pushl	%edi
	pushl	%esi
	subl	$32, %esp
	movl	56(%esp), %eax
	movl	(%eax), %esi
	movl	4(%eax), %edx
	movl	60(%esp), %ecx
	subl	(%ecx), %esi
	movl	%esi, 20(%esp)                  # 4-byte Spill
	sbbl	4(%ecx), %edx
	movl	%edx, 24(%esp)                  # 4-byte Spill
	movl	24(%eax), %edx
	movl	20(%eax), %esi
	movl	16(%eax), %edi
	movl	12(%eax), %ebx
	movl	8(%eax), %eax
	sbbl	8(%ecx), %eax
	movl	%eax, 4(%esp)                   # 4-byte Spill
	sbbl	12(%ecx), %ebx
	movl	%ebx, 12(%esp)                  # 4-byte Spill
	sbbl	16(%ecx), %edi
	movl	%edi, 16(%esp)                  # 4-byte Spill
	sbbl	20(%ecx), %esi
	movl	%esi, 28(%esp)                  # 4-byte Spill
	movl	%edx, %eax
	sbbl	24(%ecx), %eax
	movl	%eax, %ecx
	movl	%eax, %edx
	movl	%eax, 8(%esp)                   # 4-byte Spill
	sarl	$31, %ecx
	movl	%ecx, %eax
	shldl	$1, %edx, %eax
	movl	64(%esp), %edx
	andl	(%edx), %eax
	movl	24(%edx), %esi
	andl	%ecx, %esi
	movl	%esi, (%esp)                    # 4-byte Spill
	movl	20(%edx), %ebx
	andl	%ecx, %ebx
	movl	16(%edx), %edi
	andl	%ecx, %edi
	movl	12(%edx), %esi
	andl	%ecx, %esi
	movl	64(%esp), %edx
	movl	8(%edx), %edx
	andl	%ecx, %edx
	movl	64(%esp), %ebp
	andl	4(%ebp), %ecx
	addl	20(%esp), %eax                  # 4-byte Folded Reload
	adcl	24(%esp), %ecx                  # 4-byte Folded Reload
	movl	52(%esp), %ebp
	movl	%eax, (%ebp)
	adcl	4(%esp), %edx                   # 4-byte Folded Reload
	movl	%ecx, 4(%ebp)
	adcl	12(%esp), %esi                  # 4-byte Folded Reload
	movl	%edx, 8(%ebp)
	adcl	16(%esp), %edi                  # 4-byte Folded Reload
	movl	%esi, 12(%ebp)
	adcl	28(%esp), %ebx                  # 4-byte Folded Reload
	movl	%edi, 16(%ebp)
	movl	%ebx, 20(%ebp)
	movl	(%esp), %ecx                    # 4-byte Reload
	adcl	8(%esp), %ecx                   # 4-byte Folded Reload
	movl	%ecx, 24(%ebp)
	addl	$32, %esp
	popl	%esi
	popl	%edi
	popl	%ebx
	popl	%ebp
	retl
.Lfunc_end36:
	.size	mcl_fp_subNF7L, .Lfunc_end36-mcl_fp_subNF7L
                                        # -- End function
	.globl	mcl_fpDbl_add7L                 # -- Begin function mcl_fpDbl_add7L
	.p2align	4, 0x90
	.type	mcl_fpDbl_add7L,@function
mcl_fpDbl_add7L:                        # @mcl_fpDbl_add7L
# %bb.0:
	pushl	%ebp
	pushl	%ebx
	pushl	%edi
	pushl	%esi
	subl	$44, %esp
	movl	68(%esp), %eax
	movl	(%eax), %ecx
	movl	4(%eax), %edx
	movl	72(%esp), %esi
	addl	(%esi), %ecx
	movl	%ecx, 32(%esp)                  # 4-byte Spill
	adcl	4(%esi), %edx
	movl	%edx, 28(%esp)                  # 4-byte Spill
	movl	52(%eax), %ecx
	movl	%ecx, 4(%esp)                   # 4-byte Spill
	movl	48(%eax), %ecx
	movl	%ecx, 8(%esp)                   # 4-byte Spill
	movl	44(%eax), %ebx
	movl	40(%eax), %ecx
	movl	%ecx, 12(%esp)                  # 4-byte Spill
	movl	36(%eax), %ecx
	movl	%ecx, 16(%esp)                  # 4-byte Spill
	movl	32(%eax), %ecx
	movl	%ecx, 36(%esp)                  # 4-byte Spill
	movl	28(%eax), %ebp
	movl	24(%eax), %edi
	movl	20(%eax), %ecx
	movl	%ecx, (%esp)                    # 4-byte Spill
	movl	16(%eax), %edx
	movl	12(%eax), %ecx
	movl	8(%eax), %eax
	adcl	8(%esi), %eax
	movl	%eax, 24(%esp)                  # 4-byte Spill
	adcl	12(%esi), %ecx
	movl	%ecx, 20(%esp)                  # 4-byte Spill
	adcl	16(%esi), %edx
	movl	%edx, 40(%esp)                  # 4-byte Spill
	movl	(%esp), %ecx                    # 4-byte Reload
	adcl	20(%esi), %ecx
	adcl	24(%esi), %edi
	movl	%edi, %edx
	adcl	28(%esi), %ebp
	movl	36(%esp), %edi                  # 4-byte Reload
	adcl	32(%esi), %edi
	movl	16(%esp), %eax                  # 4-byte Reload
	adcl	36(%esi), %eax
	movl	%eax, 16(%esp)                  # 4-byte Spill
	movl	12(%esp), %eax                  # 4-byte Reload
	adcl	40(%esi), %eax
	movl	%eax, 12(%esp)                  # 4-byte Spill
	adcl	44(%esi), %ebx
	movl	%ebx, (%esp)                    # 4-byte Spill
	movl	8(%esp), %ebx                   # 4-byte Reload
	adcl	48(%esi), %ebx
	movl	%ebx, 8(%esp)                   # 4-byte Spill
	movl	4(%esp), %ebx                   # 4-byte Reload
	adcl	52(%esi), %ebx
	movl	%ebx, 4(%esp)                   # 4-byte Spill
	movl	64(%esp), %ebx
	movl	%edx, 24(%ebx)
	movl	%ecx, 20(%ebx)
	movl	40(%esp), %eax                  # 4-byte Reload
	movl	%eax, 16(%ebx)
	movl	20(%esp), %eax                  # 4-byte Reload
	movl	%eax, 12(%ebx)
	movl	24(%esp), %eax                  # 4-byte Reload
	movl	%eax, 8(%ebx)
	movl	28(%esp), %eax                  # 4-byte Reload
	movl	%eax, 4(%ebx)
	movl	32(%esp), %eax                  # 4-byte Reload
	movl	%eax, (%ebx)
	setb	24(%esp)                        # 1-byte Folded Spill
	movl	76(%esp), %edx
	movl	%ebp, 20(%esp)                  # 4-byte Spill
	subl	(%edx), %ebp
	movl	%ebp, 32(%esp)                  # 4-byte Spill
	movl	%edi, 36(%esp)                  # 4-byte Spill
	sbbl	4(%edx), %edi
	movl	%edi, 28(%esp)                  # 4-byte Spill
	movl	16(%esp), %esi                  # 4-byte Reload
	sbbl	8(%edx), %esi
	movl	12(%esp), %edi                  # 4-byte Reload
	sbbl	12(%edx), %edi
	movl	(%esp), %ebp                    # 4-byte Reload
	sbbl	16(%edx), %ebp
	movl	8(%esp), %eax                   # 4-byte Reload
	sbbl	20(%edx), %eax
	movl	4(%esp), %ecx                   # 4-byte Reload
	sbbl	24(%edx), %ecx
	movzbl	24(%esp), %edx                  # 1-byte Folded Reload
	sbbl	$0, %edx
	testb	$1, %dl
	jne	.LBB37_1
# %bb.2:
	movl	%ecx, 52(%ebx)
	jne	.LBB37_3
.LBB37_4:
	movl	%eax, 48(%ebx)
	movl	28(%esp), %ecx                  # 4-byte Reload
	jne	.LBB37_5
.LBB37_6:
	movl	%ebp, 44(%ebx)
	movl	32(%esp), %eax                  # 4-byte Reload
	jne	.LBB37_7
.LBB37_8:
	movl	%edi, 40(%ebx)
	jne	.LBB37_9
.LBB37_10:
	movl	%esi, 36(%ebx)
	jne	.LBB37_11
.LBB37_12:
	movl	%ecx, 32(%ebx)
	je	.LBB37_14
.LBB37_13:
	movl	20(%esp), %eax                  # 4-byte Reload
.LBB37_14:
	movl	%eax, 28(%ebx)
	addl	$44, %esp
	popl	%esi
	popl	%edi
	popl	%ebx
	popl	%ebp
	retl
.LBB37_1:
	movl	4(%esp), %ecx                   # 4-byte Reload
	movl	%ecx, 52(%ebx)
	je	.LBB37_4
.LBB37_3:
	movl	8(%esp), %eax                   # 4-byte Reload
	movl	%eax, 48(%ebx)
	movl	28(%esp), %ecx                  # 4-byte Reload
	je	.LBB37_6
.LBB37_5:
	movl	(%esp), %ebp                    # 4-byte Reload
	movl	%ebp, 44(%ebx)
	movl	32(%esp), %eax                  # 4-byte Reload
	je	.LBB37_8
.LBB37_7:
	movl	12(%esp), %edi                  # 4-byte Reload
	movl	%edi, 40(%ebx)
	je	.LBB37_10
.LBB37_9:
	movl	16(%esp), %esi                  # 4-byte Reload
	movl	%esi, 36(%ebx)
	je	.LBB37_12
.LBB37_11:
	movl	36(%esp), %ecx                  # 4-byte Reload
	movl	%ecx, 32(%ebx)
	jne	.LBB37_13
	jmp	.LBB37_14
.Lfunc_end37:
	.size	mcl_fpDbl_add7L, .Lfunc_end37-mcl_fpDbl_add7L
                                        # -- End function
	.globl	mcl_fpDbl_sub7L                 # -- Begin function mcl_fpDbl_sub7L
	.p2align	4, 0x90
	.type	mcl_fpDbl_sub7L,@function
mcl_fpDbl_sub7L:                        # @mcl_fpDbl_sub7L
# %bb.0:
	pushl	%ebp
	pushl	%ebx
	pushl	%edi
	pushl	%esi
	subl	$44, %esp
	movl	68(%esp), %eax
	movl	(%eax), %edx
	movl	4(%eax), %edi
	xorl	%esi, %esi
	movl	72(%esp), %ecx
	subl	(%ecx), %edx
	movl	%edx, (%esp)                    # 4-byte Spill
	sbbl	4(%ecx), %edi
	movl	%edi, 40(%esp)                  # 4-byte Spill
	movl	52(%eax), %edx
	movl	%edx, 8(%esp)                   # 4-byte Spill
	movl	48(%eax), %edx
	movl	%edx, 28(%esp)                  # 4-byte Spill
	movl	44(%eax), %edx
	movl	%edx, 24(%esp)                  # 4-byte Spill
	movl	40(%eax), %edx
	movl	%edx, 20(%esp)                  # 4-byte Spill
	movl	36(%eax), %edx
	movl	%edx, 16(%esp)                  # 4-byte Spill
	movl	32(%eax), %edx
	movl	%edx, 12(%esp)                  # 4-byte Spill
	movl	28(%eax), %edi
	movl	24(%eax), %edx
	movl	%edx, 4(%esp)                   # 4-byte Spill
	movl	20(%eax), %edx
	movl	16(%eax), %ebx
	movl	12(%eax), %ebp
	movl	8(%eax), %eax
	sbbl	8(%ecx), %eax
	movl	%eax, 36(%esp)                  # 4-byte Spill
	sbbl	12(%ecx), %ebp
	sbbl	16(%ecx), %ebx
	sbbl	20(%ecx), %edx
	movl	%edx, 32(%esp)                  # 4-byte Spill
	movl	4(%esp), %edx                   # 4-byte Reload
	sbbl	24(%ecx), %edx
	sbbl	28(%ecx), %edi
	movl	%edi, 4(%esp)                   # 4-byte Spill
	movl	12(%esp), %eax                  # 4-byte Reload
	sbbl	32(%ecx), %eax
	movl	%eax, 12(%esp)                  # 4-byte Spill
	movl	16(%esp), %eax                  # 4-byte Reload
	sbbl	36(%ecx), %eax
	movl	%eax, 16(%esp)                  # 4-byte Spill
	movl	20(%esp), %eax                  # 4-byte Reload
	sbbl	40(%ecx), %eax
	movl	%eax, 20(%esp)                  # 4-byte Spill
	movl	24(%esp), %eax                  # 4-byte Reload
	sbbl	44(%ecx), %eax
	movl	%eax, 24(%esp)                  # 4-byte Spill
	movl	28(%esp), %eax                  # 4-byte Reload
	sbbl	48(%ecx), %eax
	movl	%eax, 28(%esp)                  # 4-byte Spill
	movl	8(%esp), %eax                   # 4-byte Reload
	sbbl	52(%ecx), %eax
	movl	%eax, 8(%esp)                   # 4-byte Spill
	movl	64(%esp), %ecx
	movl	%edx, 24(%ecx)
	movl	32(%esp), %eax                  # 4-byte Reload
	movl	%eax, 20(%ecx)
	movl	%ebx, 16(%ecx)
	movl	%ebp, 12(%ecx)
	movl	36(%esp), %eax                  # 4-byte Reload
	movl	%eax, 8(%ecx)
	movl	40(%esp), %eax                  # 4-byte Reload
	movl	%eax, 4(%ecx)
	movl	(%esp), %eax                    # 4-byte Reload
	movl	%eax, (%ecx)
	sbbl	%esi, %esi
	andl	$1, %esi
	negl	%esi
	movl	76(%esp), %ecx
	movl	24(%ecx), %eax
	andl	%esi, %eax
	movl	%eax, (%esp)                    # 4-byte Spill
	movl	20(%ecx), %edx
	andl	%esi, %edx
	movl	16(%ecx), %ebx
	andl	%esi, %ebx
	movl	12(%ecx), %ebp
	andl	%esi, %ebp
	movl	8(%ecx), %ecx
	andl	%esi, %ecx
	movl	76(%esp), %eax
	movl	4(%eax), %eax
	andl	%esi, %eax
	movl	76(%esp), %edi
	andl	(%edi), %esi
	addl	4(%esp), %esi                   # 4-byte Folded Reload
	adcl	12(%esp), %eax                  # 4-byte Folded Reload
	movl	64(%esp), %edi
	movl	%esi, 28(%edi)
	adcl	16(%esp), %ecx                  # 4-byte Folded Reload
	movl	%eax, 32(%edi)
	adcl	20(%esp), %ebp                  # 4-byte Folded Reload
	movl	%ecx, 36(%edi)
	adcl	24(%esp), %ebx                  # 4-byte Folded Reload
	movl	%ebp, 40(%edi)
	adcl	28(%esp), %edx                  # 4-byte Folded Reload
	movl	%ebx, 44(%edi)
	movl	%edx, 48(%edi)
	movl	(%esp), %eax                    # 4-byte Reload
	adcl	8(%esp), %eax                   # 4-byte Folded Reload
	movl	%eax, 52(%edi)
	addl	$44, %esp
	popl	%esi
	popl	%edi
	popl	%ebx
	popl	%ebp
	retl
.Lfunc_end38:
	.size	mcl_fpDbl_sub7L, .Lfunc_end38-mcl_fpDbl_sub7L
                                        # -- End function
	.globl	mulPv256x32                     # -- Begin function mulPv256x32
	.p2align	4, 0x90
	.type	mulPv256x32,@function
mulPv256x32:                            # @mulPv256x32
# %bb.0:
	pushl	%ebp
	pushl	%ebx
	pushl	%edi
	pushl	%esi
	subl	$44, %esp
	movl	72(%esp), %esi
	movl	68(%esp), %ecx
	movl	%esi, %eax
	mull	28(%ecx)
	movl	%edx, 40(%esp)                  # 4-byte Spill
	movl	%eax, 36(%esp)                  # 4-byte Spill
	movl	%esi, %eax
	mull	24(%ecx)
	movl	%edx, 32(%esp)                  # 4-byte Spill
	movl	%eax, 28(%esp)                  # 4-byte Spill
	movl	%esi, %eax
	mull	20(%ecx)
	movl	%edx, 24(%esp)                  # 4-byte Spill
	movl	%eax, 20(%esp)                  # 4-byte Spill
	movl	%esi, %eax
	mull	16(%ecx)
	movl	%edx, 16(%esp)                  # 4-byte Spill
	movl	%eax, 12(%esp)                  # 4-byte Spill
	movl	%esi, %eax
	mull	12(%ecx)
	movl	%edx, %ebx
	movl	%eax, 8(%esp)                   # 4-byte Spill
	movl	%esi, %eax
	mull	8(%ecx)
	movl	%edx, %edi
	movl	%eax, 4(%esp)                   # 4-byte Spill
	movl	%esi, %eax
	mull	4(%ecx)
	movl	%edx, %ebp
	movl	%eax, (%esp)                    # 4-byte Spill
	movl	%esi, %eax
	mull	(%ecx)
	movl	64(%esp), %esi
	movl	%eax, (%esi)
	addl	(%esp), %edx                    # 4-byte Folded Reload
	movl	%edx, 4(%esi)
	adcl	4(%esp), %ebp                   # 4-byte Folded Reload
	movl	%ebp, 8(%esi)
	adcl	8(%esp), %edi                   # 4-byte Folded Reload
	movl	%edi, 12(%esi)
	adcl	12(%esp), %ebx                  # 4-byte Folded Reload
	movl	%ebx, 16(%esi)
	movl	16(%esp), %eax                  # 4-byte Reload
	adcl	20(%esp), %eax                  # 4-byte Folded Reload
	movl	%eax, 20(%esi)
	movl	24(%esp), %eax                  # 4-byte Reload
	adcl	28(%esp), %eax                  # 4-byte Folded Reload
	movl	%eax, 24(%esi)
	movl	32(%esp), %eax                  # 4-byte Reload
	adcl	36(%esp), %eax                  # 4-byte Folded Reload
	movl	%eax, 28(%esi)
	movl	40(%esp), %eax                  # 4-byte Reload
	adcl	$0, %eax
	movl	%eax, 32(%esi)
	movl	%esi, %eax
	addl	$44, %esp
	popl	%esi
	popl	%edi
	popl	%ebx
	popl	%ebp
	retl	$4
.Lfunc_end39:
	.size	mulPv256x32, .Lfunc_end39-mulPv256x32
                                        # -- End function
	.globl	mcl_fp_mulUnitPre8L             # -- Begin function mcl_fp_mulUnitPre8L
	.p2align	4, 0x90
	.type	mcl_fp_mulUnitPre8L,@function
mcl_fp_mulUnitPre8L:                    # @mcl_fp_mulUnitPre8L
# %bb.0:
	pushl	%ebp
	pushl	%ebx
	pushl	%edi
	pushl	%esi
	subl	$60, %esp
	calll	.L40$pb
.L40$pb:
	popl	%ebx
.Ltmp2:
	addl	$_GLOBAL_OFFSET_TABLE_+(.Ltmp2-.L40$pb), %ebx
	subl	$4, %esp
	movl	92(%esp), %eax
	movl	88(%esp), %ecx
	leal	28(%esp), %edx
	pushl	%eax
	pushl	%ecx
	pushl	%edx
	calll	mulPv256x32@PLT
	addl	$12, %esp
	movl	24(%esp), %eax
	movl	%eax, 20(%esp)                  # 4-byte Spill
	movl	28(%esp), %eax
	movl	%eax, 16(%esp)                  # 4-byte Spill
	movl	32(%esp), %eax
	movl	%eax, 12(%esp)                  # 4-byte Spill
	movl	36(%esp), %esi
	movl	40(%esp), %edi
	movl	44(%esp), %ebx
	movl	48(%esp), %ebp
	movl	52(%esp), %edx
	movl	56(%esp), %ecx
	movl	80(%esp), %eax
	movl	%ecx, 32(%eax)
	movl	%edx, 28(%eax)
	movl	%ebp, 24(%eax)
	movl	%ebx, 20(%eax)
	movl	%edi, 16(%eax)
	movl	%esi, 12(%eax)
	movl	12(%esp), %ecx                  # 4-byte Reload
	movl	%ecx, 8(%eax)
	movl	16(%esp), %ecx                  # 4-byte Reload
	movl	%ecx, 4(%eax)
	movl	20(%esp), %ecx                  # 4-byte Reload
	movl	%ecx, (%eax)
	addl	$60, %esp
	popl	%esi
	popl	%edi
	popl	%ebx
	popl	%ebp
	retl
.Lfunc_end40:
	.size	mcl_fp_mulUnitPre8L, .Lfunc_end40-mcl_fp_mulUnitPre8L
                                        # -- End function
	.globl	mcl_fpDbl_mulPre8L              # -- Begin function mcl_fpDbl_mulPre8L
	.p2align	4, 0x90
	.type	mcl_fpDbl_mulPre8L,@function
mcl_fpDbl_mulPre8L:                     # @mcl_fpDbl_mulPre8L
# %bb.0:
	pushl	%ebp
	pushl	%ebx
	pushl	%edi
	pushl	%esi
	subl	$348, %esp                      # imm = 0x15C
	calll	.L41$pb
.L41$pb:
	popl	%ebx
.Ltmp3:
	addl	$_GLOBAL_OFFSET_TABLE_+(.Ltmp3-.L41$pb), %ebx
	movl	376(%esp), %ecx
	subl	$4, %esp
	leal	316(%esp), %eax
	pushl	(%ecx)
	pushl	380(%esp)
	pushl	%eax
	calll	mulPv256x32@PLT
	addl	$12, %esp
	movl	344(%esp), %eax
	movl	%eax, 12(%esp)                  # 4-byte Spill
	movl	340(%esp), %eax
	movl	%eax, 8(%esp)                   # 4-byte Spill
	movl	336(%esp), %eax
	movl	%eax, 4(%esp)                   # 4-byte Spill
	movl	332(%esp), %esi
	movl	328(%esp), %eax
	movl	%eax, 16(%esp)                  # 4-byte Spill
	movl	324(%esp), %eax
	movl	%eax, 20(%esp)                  # 4-byte Spill
	movl	320(%esp), %edi
	movl	312(%esp), %eax
	movl	316(%esp), %ebp
	movl	368(%esp), %ecx
	movl	%eax, (%ecx)
	subl	$4, %esp
	leal	276(%esp), %eax
	movl	380(%esp), %ecx
	pushl	4(%ecx)
	pushl	380(%esp)
	pushl	%eax
	calll	mulPv256x32@PLT
	addl	$12, %esp
	movl	304(%esp), %eax
	movl	%ebp, %edx
	addl	272(%esp), %edx
	adcl	276(%esp), %edi
	movl	%edi, %ebp
	movl	20(%esp), %edi                  # 4-byte Reload
	adcl	280(%esp), %edi
	movl	16(%esp), %ecx                  # 4-byte Reload
	adcl	284(%esp), %ecx
	movl	%ecx, 16(%esp)                  # 4-byte Spill
	adcl	288(%esp), %esi
	movl	4(%esp), %ecx                   # 4-byte Reload
	adcl	292(%esp), %ecx
	movl	%ecx, 4(%esp)                   # 4-byte Spill
	movl	8(%esp), %ecx                   # 4-byte Reload
	adcl	296(%esp), %ecx
	movl	%ecx, 8(%esp)                   # 4-byte Spill
	movl	12(%esp), %ecx                  # 4-byte Reload
	adcl	300(%esp), %ecx
	movl	%ecx, 12(%esp)                  # 4-byte Spill
	movl	368(%esp), %ecx
	movl	%edx, 4(%ecx)
	adcl	$0, %eax
	movl	%eax, 24(%esp)                  # 4-byte Spill
	subl	$4, %esp
	leal	236(%esp), %eax
	movl	380(%esp), %ecx
	pushl	8(%ecx)
	movl	380(%esp), %ecx
	pushl	%ecx
	pushl	%eax
	calll	mulPv256x32@PLT
	addl	$12, %esp
	movl	264(%esp), %eax
	movl	%ebp, %edx
	addl	232(%esp), %edx
	adcl	236(%esp), %edi
	movl	%edi, 20(%esp)                  # 4-byte Spill
	movl	16(%esp), %edi                  # 4-byte Reload
	adcl	240(%esp), %edi
	adcl	244(%esp), %esi
	movl	%esi, 28(%esp)                  # 4-byte Spill
	movl	4(%esp), %ebp                   # 4-byte Reload
	adcl	248(%esp), %ebp
	movl	8(%esp), %esi                   # 4-byte Reload
	adcl	252(%esp), %esi
	movl	12(%esp), %ecx                  # 4-byte Reload
	adcl	256(%esp), %ecx
	movl	%ecx, 12(%esp)                  # 4-byte Spill
	movl	24(%esp), %ecx                  # 4-byte Reload
	adcl	260(%esp), %ecx
	movl	%ecx, 24(%esp)                  # 4-byte Spill
	movl	368(%esp), %ecx
	movl	%edx, 8(%ecx)
	adcl	$0, %eax
	movl	%eax, 4(%esp)                   # 4-byte Spill
	subl	$4, %esp
	leal	196(%esp), %eax
	movl	380(%esp), %ecx
	pushl	12(%ecx)
	pushl	380(%esp)
	pushl	%eax
	calll	mulPv256x32@PLT
	addl	$12, %esp
	movl	224(%esp), %eax
	movl	20(%esp), %edx                  # 4-byte Reload
	addl	192(%esp), %edx
	adcl	196(%esp), %edi
	movl	%edi, 16(%esp)                  # 4-byte Spill
	movl	28(%esp), %ecx                  # 4-byte Reload
	adcl	200(%esp), %ecx
	movl	%ecx, 28(%esp)                  # 4-byte Spill
	adcl	204(%esp), %ebp
	adcl	208(%esp), %esi
	movl	%esi, 8(%esp)                   # 4-byte Spill
	movl	12(%esp), %esi                  # 4-byte Reload
	adcl	212(%esp), %esi
	movl	24(%esp), %edi                  # 4-byte Reload
	adcl	216(%esp), %edi
	movl	4(%esp), %ecx                   # 4-byte Reload
	adcl	220(%esp), %ecx
	movl	%ecx, 4(%esp)                   # 4-byte Spill
	movl	368(%esp), %ecx
	movl	%edx, 12(%ecx)
	adcl	$0, %eax
	movl	%eax, 20(%esp)                  # 4-byte Spill
	subl	$4, %esp
	leal	156(%esp), %eax
	movl	380(%esp), %ecx
	pushl	16(%ecx)
	pushl	380(%esp)
	pushl	%eax
	calll	mulPv256x32@PLT
	addl	$12, %esp
	movl	184(%esp), %edx
	movl	16(%esp), %ecx                  # 4-byte Reload
	addl	152(%esp), %ecx
	movl	28(%esp), %eax                  # 4-byte Reload
	adcl	156(%esp), %eax
	movl	%eax, 28(%esp)                  # 4-byte Spill
	adcl	160(%esp), %ebp
	movl	8(%esp), %eax                   # 4-byte Reload
	adcl	164(%esp), %eax
	movl	%eax, 8(%esp)                   # 4-byte Spill
	adcl	168(%esp), %esi
	movl	%esi, 12(%esp)                  # 4-byte Spill
	adcl	172(%esp), %edi
	movl	%edi, 24(%esp)                  # 4-byte Spill
	movl	4(%esp), %eax                   # 4-byte Reload
	adcl	176(%esp), %eax
	movl	%eax, 4(%esp)                   # 4-byte Spill
	movl	20(%esp), %edi                  # 4-byte Reload
	adcl	180(%esp), %edi
	movl	368(%esp), %eax
	movl	%ecx, 16(%eax)
	adcl	$0, %edx
	movl	%edx, 16(%esp)                  # 4-byte Spill
	subl	$4, %esp
	leal	116(%esp), %eax
	movl	380(%esp), %ecx
	pushl	20(%ecx)
	pushl	380(%esp)
	pushl	%eax
	calll	mulPv256x32@PLT
	addl	$12, %esp
	movl	144(%esp), %edx
	movl	28(%esp), %eax                  # 4-byte Reload
	addl	112(%esp), %eax
	movl	%ebp, %esi
	adcl	116(%esp), %esi
	movl	8(%esp), %ecx                   # 4-byte Reload
	adcl	120(%esp), %ecx
	movl	%ecx, 8(%esp)                   # 4-byte Spill
	movl	12(%esp), %ecx                  # 4-byte Reload
	adcl	124(%esp), %ecx
	movl	%ecx, 12(%esp)                  # 4-byte Spill
	movl	24(%esp), %ecx                  # 4-byte Reload
	adcl	128(%esp), %ecx
	movl	%ecx, 24(%esp)                  # 4-byte Spill
	movl	4(%esp), %ecx                   # 4-byte Reload
	adcl	132(%esp), %ecx
	movl	%ecx, 4(%esp)                   # 4-byte Spill
	adcl	136(%esp), %edi
	movl	%edi, 20(%esp)                  # 4-byte Spill
	movl	16(%esp), %edi                  # 4-byte Reload
	adcl	140(%esp), %edi
	movl	368(%esp), %ecx
	movl	%eax, 20(%ecx)
	adcl	$0, %edx
	movl	%edx, %ebp
	subl	$4, %esp
	leal	76(%esp), %eax
	movl	380(%esp), %ecx
	pushl	24(%ecx)
	pushl	380(%esp)
	pushl	%eax
	calll	mulPv256x32@PLT
	addl	$12, %esp
	movl	%esi, %eax
	addl	72(%esp), %eax
	movl	8(%esp), %ecx                   # 4-byte Reload
	adcl	76(%esp), %ecx
	movl	%ecx, 8(%esp)                   # 4-byte Spill
	movl	12(%esp), %esi                  # 4-byte Reload
	adcl	80(%esp), %esi
	movl	24(%esp), %ecx                  # 4-byte Reload
	adcl	84(%esp), %ecx
	movl	%ecx, 24(%esp)                  # 4-byte Spill
	movl	4(%esp), %ecx                   # 4-byte Reload
	adcl	88(%esp), %ecx
	movl	%ecx, 4(%esp)                   # 4-byte Spill
	movl	20(%esp), %ecx                  # 4-byte Reload
	adcl	92(%esp), %ecx
	movl	%ecx, 20(%esp)                  # 4-byte Spill
	adcl	96(%esp), %edi
	movl	%edi, 16(%esp)                  # 4-byte Spill
	adcl	100(%esp), %ebp
	movl	%ebp, 28(%esp)                  # 4-byte Spill
	movl	104(%esp), %edx
	movl	368(%esp), %ecx
	movl	%eax, 24(%ecx)
	adcl	$0, %edx
	movl	%edx, %edi
	subl	$4, %esp
	leal	36(%esp), %eax
	movl	380(%esp), %ecx
	pushl	28(%ecx)
	pushl	380(%esp)
	pushl	%eax
	calll	mulPv256x32@PLT
	addl	$12, %esp
	movl	8(%esp), %eax                   # 4-byte Reload
	addl	32(%esp), %eax
	movl	%eax, 8(%esp)                   # 4-byte Spill
	adcl	36(%esp), %esi
	movl	%esi, 12(%esp)                  # 4-byte Spill
	movl	24(%esp), %ebp                  # 4-byte Reload
	adcl	40(%esp), %ebp
	movl	4(%esp), %ebx                   # 4-byte Reload
	adcl	44(%esp), %ebx
	movl	20(%esp), %esi                  # 4-byte Reload
	adcl	48(%esp), %esi
	movl	16(%esp), %edx                  # 4-byte Reload
	adcl	52(%esp), %edx
	movl	28(%esp), %eax                  # 4-byte Reload
	adcl	56(%esp), %eax
	adcl	60(%esp), %edi
	movl	%edi, 4(%esp)                   # 4-byte Spill
	movl	64(%esp), %edi
	movl	368(%esp), %ecx
	movl	%eax, 52(%ecx)
	movl	%edx, 48(%ecx)
	movl	%esi, 44(%ecx)
	movl	%ebx, 40(%ecx)
	movl	%ebp, 36(%ecx)
	movl	12(%esp), %eax                  # 4-byte Reload
	movl	%eax, 32(%ecx)
	movl	4(%esp), %eax                   # 4-byte Reload
	movl	%eax, 56(%ecx)
	movl	8(%esp), %edx                   # 4-byte Reload
	movl	%edx, 28(%ecx)
	adcl	$0, %edi
	movl	%edi, 60(%ecx)
	addl	$348, %esp                      # imm = 0x15C
	popl	%esi
	popl	%edi
	popl	%ebx
	popl	%ebp
	retl
.Lfunc_end41:
	.size	mcl_fpDbl_mulPre8L, .Lfunc_end41-mcl_fpDbl_mulPre8L
                                        # -- End function
	.globl	mcl_fpDbl_sqrPre8L              # -- Begin function mcl_fpDbl_sqrPre8L
	.p2align	4, 0x90
	.type	mcl_fpDbl_sqrPre8L,@function
mcl_fpDbl_sqrPre8L:                     # @mcl_fpDbl_sqrPre8L
# %bb.0:
	pushl	%ebp
	pushl	%ebx
	pushl	%edi
	pushl	%esi
	subl	$348, %esp                      # imm = 0x15C
	calll	.L42$pb
.L42$pb:
	popl	%ebx
.Ltmp4:
	addl	$_GLOBAL_OFFSET_TABLE_+(.Ltmp4-.L42$pb), %ebx
	movl	372(%esp), %ecx
	subl	$4, %esp
	leal	316(%esp), %eax
	pushl	(%ecx)
	pushl	%ecx
	pushl	%eax
	calll	mulPv256x32@PLT
	addl	$12, %esp
	movl	344(%esp), %eax
	movl	%eax, 12(%esp)                  # 4-byte Spill
	movl	340(%esp), %eax
	movl	%eax, 8(%esp)                   # 4-byte Spill
	movl	336(%esp), %eax
	movl	%eax, 4(%esp)                   # 4-byte Spill
	movl	332(%esp), %ebp
	movl	328(%esp), %eax
	movl	%eax, 16(%esp)                  # 4-byte Spill
	movl	324(%esp), %eax
	movl	%eax, 20(%esp)                  # 4-byte Spill
	movl	320(%esp), %edi
	movl	312(%esp), %eax
	movl	316(%esp), %esi
	movl	368(%esp), %ecx
	movl	%eax, (%ecx)
	subl	$4, %esp
	leal	276(%esp), %eax
	movl	376(%esp), %ecx
	pushl	4(%ecx)
	pushl	%ecx
	pushl	%eax
	calll	mulPv256x32@PLT
	addl	$12, %esp
	movl	304(%esp), %eax
	movl	%esi, %edx
	addl	272(%esp), %edx
	adcl	276(%esp), %edi
	movl	20(%esp), %esi                  # 4-byte Reload
	adcl	280(%esp), %esi
	movl	16(%esp), %ecx                  # 4-byte Reload
	adcl	284(%esp), %ecx
	movl	%ecx, 16(%esp)                  # 4-byte Spill
	adcl	288(%esp), %ebp
	movl	4(%esp), %ecx                   # 4-byte Reload
	adcl	292(%esp), %ecx
	movl	%ecx, 4(%esp)                   # 4-byte Spill
	movl	8(%esp), %ecx                   # 4-byte Reload
	adcl	296(%esp), %ecx
	movl	%ecx, 8(%esp)                   # 4-byte Spill
	movl	12(%esp), %ecx                  # 4-byte Reload
	adcl	300(%esp), %ecx
	movl	%ecx, 12(%esp)                  # 4-byte Spill
	movl	368(%esp), %ecx
	movl	%edx, 4(%ecx)
	adcl	$0, %eax
	movl	%eax, 24(%esp)                  # 4-byte Spill
	subl	$4, %esp
	leal	236(%esp), %eax
	movl	376(%esp), %ecx
	pushl	8(%ecx)
	pushl	%ecx
	pushl	%eax
	calll	mulPv256x32@PLT
	addl	$12, %esp
	movl	264(%esp), %eax
	movl	%edi, %edx
	addl	232(%esp), %edx
	adcl	236(%esp), %esi
	movl	%esi, 20(%esp)                  # 4-byte Spill
	movl	16(%esp), %esi                  # 4-byte Reload
	adcl	240(%esp), %esi
	adcl	244(%esp), %ebp
	movl	%ebp, 28(%esp)                  # 4-byte Spill
	movl	4(%esp), %ebp                   # 4-byte Reload
	adcl	248(%esp), %ebp
	movl	8(%esp), %edi                   # 4-byte Reload
	adcl	252(%esp), %edi
	movl	12(%esp), %ecx                  # 4-byte Reload
	adcl	256(%esp), %ecx
	movl	%ecx, 12(%esp)                  # 4-byte Spill
	movl	24(%esp), %ecx                  # 4-byte Reload
	adcl	260(%esp), %ecx
	movl	%ecx, 24(%esp)                  # 4-byte Spill
	movl	368(%esp), %ecx
	movl	%edx, 8(%ecx)
	adcl	$0, %eax
	movl	%eax, 4(%esp)                   # 4-byte Spill
	subl	$4, %esp
	leal	196(%esp), %eax
	movl	376(%esp), %ecx
	pushl	12(%ecx)
	pushl	%ecx
	pushl	%eax
	calll	mulPv256x32@PLT
	addl	$12, %esp
	movl	224(%esp), %eax
	movl	20(%esp), %edx                  # 4-byte Reload
	addl	192(%esp), %edx
	adcl	196(%esp), %esi
	movl	%esi, 16(%esp)                  # 4-byte Spill
	movl	28(%esp), %ecx                  # 4-byte Reload
	adcl	200(%esp), %ecx
	movl	%ecx, 28(%esp)                  # 4-byte Spill
	adcl	204(%esp), %ebp
	adcl	208(%esp), %edi
	movl	%edi, 8(%esp)                   # 4-byte Spill
	movl	12(%esp), %esi                  # 4-byte Reload
	adcl	212(%esp), %esi
	movl	24(%esp), %edi                  # 4-byte Reload
	adcl	216(%esp), %edi
	movl	4(%esp), %ecx                   # 4-byte Reload
	adcl	220(%esp), %ecx
	movl	%ecx, 4(%esp)                   # 4-byte Spill
	movl	368(%esp), %ecx
	movl	%edx, 12(%ecx)
	adcl	$0, %eax
	movl	%eax, 20(%esp)                  # 4-byte Spill
	subl	$4, %esp
	leal	156(%esp), %eax
	movl	376(%esp), %ecx
	pushl	16(%ecx)
	pushl	%ecx
	pushl	%eax
	calll	mulPv256x32@PLT
	addl	$12, %esp
	movl	184(%esp), %edx
	movl	16(%esp), %ecx                  # 4-byte Reload
	addl	152(%esp), %ecx
	movl	28(%esp), %eax                  # 4-byte Reload
	adcl	156(%esp), %eax
	movl	%eax, 28(%esp)                  # 4-byte Spill
	adcl	160(%esp), %ebp
	movl	8(%esp), %eax                   # 4-byte Reload
	adcl	164(%esp), %eax
	movl	%eax, 8(%esp)                   # 4-byte Spill
	adcl	168(%esp), %esi
	movl	%esi, 12(%esp)                  # 4-byte Spill
	adcl	172(%esp), %edi
	movl	%edi, 24(%esp)                  # 4-byte Spill
	movl	4(%esp), %eax                   # 4-byte Reload
	adcl	176(%esp), %eax
	movl	%eax, 4(%esp)                   # 4-byte Spill
	movl	20(%esp), %edi                  # 4-byte Reload
	adcl	180(%esp), %edi
	movl	368(%esp), %eax
	movl	%ecx, 16(%eax)
	adcl	$0, %edx
	movl	%edx, 16(%esp)                  # 4-byte Spill
	subl	$4, %esp
	leal	116(%esp), %eax
	movl	376(%esp), %ecx
	pushl	20(%ecx)
	pushl	%ecx
	pushl	%eax
	calll	mulPv256x32@PLT
	addl	$12, %esp
	movl	144(%esp), %edx
	movl	28(%esp), %eax                  # 4-byte Reload
	addl	112(%esp), %eax
	movl	%ebp, %esi
	adcl	116(%esp), %esi
	movl	8(%esp), %ecx                   # 4-byte Reload
	adcl	120(%esp), %ecx
	movl	%ecx, 8(%esp)                   # 4-byte Spill
	movl	12(%esp), %ecx                  # 4-byte Reload
	adcl	124(%esp), %ecx
	movl	%ecx, 12(%esp)                  # 4-byte Spill
	movl	24(%esp), %ecx                  # 4-byte Reload
	adcl	128(%esp), %ecx
	movl	%ecx, 24(%esp)                  # 4-byte Spill
	movl	4(%esp), %ecx                   # 4-byte Reload
	adcl	132(%esp), %ecx
	movl	%ecx, 4(%esp)                   # 4-byte Spill
	adcl	136(%esp), %edi
	movl	%edi, 20(%esp)                  # 4-byte Spill
	movl	16(%esp), %edi                  # 4-byte Reload
	adcl	140(%esp), %edi
	movl	368(%esp), %ecx
	movl	%eax, 20(%ecx)
	adcl	$0, %edx
	movl	%edx, %ebp
	subl	$4, %esp
	leal	76(%esp), %eax
	movl	376(%esp), %ecx
	pushl	24(%ecx)
	pushl	%ecx
	pushl	%eax
	calll	mulPv256x32@PLT
	addl	$12, %esp
	addl	72(%esp), %esi
	movl	%esi, %eax
	movl	8(%esp), %ecx                   # 4-byte Reload
	adcl	76(%esp), %ecx
	movl	%ecx, 8(%esp)                   # 4-byte Spill
	movl	12(%esp), %esi                  # 4-byte Reload
	adcl	80(%esp), %esi
	movl	24(%esp), %ecx                  # 4-byte Reload
	adcl	84(%esp), %ecx
	movl	%ecx, 24(%esp)                  # 4-byte Spill
	movl	4(%esp), %ecx                   # 4-byte Reload
	adcl	88(%esp), %ecx
	movl	%ecx, 4(%esp)                   # 4-byte Spill
	movl	20(%esp), %ecx                  # 4-byte Reload
	adcl	92(%esp), %ecx
	movl	%ecx, 20(%esp)                  # 4-byte Spill
	adcl	96(%esp), %edi
	movl	%edi, 16(%esp)                  # 4-byte Spill
	adcl	100(%esp), %ebp
	movl	%ebp, 28(%esp)                  # 4-byte Spill
	movl	104(%esp), %edx
	movl	368(%esp), %ecx
	movl	%eax, 24(%ecx)
	adcl	$0, %edx
	movl	%edx, %edi
	subl	$4, %esp
	leal	36(%esp), %eax
	movl	376(%esp), %ecx
	pushl	28(%ecx)
	pushl	%ecx
	pushl	%eax
	calll	mulPv256x32@PLT
	addl	$12, %esp
	movl	8(%esp), %eax                   # 4-byte Reload
	addl	32(%esp), %eax
	movl	%eax, 8(%esp)                   # 4-byte Spill
	adcl	36(%esp), %esi
	movl	%esi, 12(%esp)                  # 4-byte Spill
	movl	24(%esp), %ebp                  # 4-byte Reload
	adcl	40(%esp), %ebp
	movl	4(%esp), %ebx                   # 4-byte Reload
	adcl	44(%esp), %ebx
	movl	20(%esp), %esi                  # 4-byte Reload
	adcl	48(%esp), %esi
	movl	16(%esp), %edx                  # 4-byte Reload
	adcl	52(%esp), %edx
	movl	28(%esp), %eax                  # 4-byte Reload
	adcl	56(%esp), %eax
	adcl	60(%esp), %edi
	movl	%edi, 4(%esp)                   # 4-byte Spill
	movl	64(%esp), %edi
	movl	368(%esp), %ecx
	movl	%eax, 52(%ecx)
	movl	%edx, 48(%ecx)
	movl	%esi, 44(%ecx)
	movl	%ebx, 40(%ecx)
	movl	%ebp, 36(%ecx)
	movl	12(%esp), %eax                  # 4-byte Reload
	movl	%eax, 32(%ecx)
	movl	4(%esp), %eax                   # 4-byte Reload
	movl	%eax, 56(%ecx)
	movl	8(%esp), %edx                   # 4-byte Reload
	movl	%edx, 28(%ecx)
	adcl	$0, %edi
	movl	%edi, 60(%ecx)
	addl	$348, %esp                      # imm = 0x15C
	popl	%esi
	popl	%edi
	popl	%ebx
	popl	%ebp
	retl
.Lfunc_end42:
	.size	mcl_fpDbl_sqrPre8L, .Lfunc_end42-mcl_fpDbl_sqrPre8L
                                        # -- End function
	.globl	mcl_fp_mont8L                   # -- Begin function mcl_fp_mont8L
	.p2align	4, 0x90
	.type	mcl_fp_mont8L,@function
mcl_fp_mont8L:                          # @mcl_fp_mont8L
# %bb.0:
	pushl	%ebp
	pushl	%ebx
	pushl	%edi
	pushl	%esi
	subl	$684, %esp                      # imm = 0x2AC
	calll	.L43$pb
.L43$pb:
	popl	%ebx
.Ltmp5:
	addl	$_GLOBAL_OFFSET_TABLE_+(.Ltmp5-.L43$pb), %ebx
	movl	716(%esp), %eax
	movl	-4(%eax), %ebp
	movl	%ebp, 44(%esp)                  # 4-byte Spill
	movl	712(%esp), %ecx
	subl	$4, %esp
	leal	652(%esp), %eax
	pushl	(%ecx)
	pushl	716(%esp)
	pushl	%eax
	calll	mulPv256x32@PLT
	addl	$12, %esp
	movl	648(%esp), %edi
	movl	652(%esp), %esi
	movl	%ebp, %eax
	imull	%edi, %eax
	movl	680(%esp), %ecx
	movl	%ecx, 16(%esp)                  # 4-byte Spill
	movl	676(%esp), %ecx
	movl	%ecx, 4(%esp)                   # 4-byte Spill
	movl	672(%esp), %ecx
	movl	%ecx, 20(%esp)                  # 4-byte Spill
	movl	668(%esp), %ecx
	movl	%ecx, 36(%esp)                  # 4-byte Spill
	movl	664(%esp), %ecx
	movl	%ecx, 12(%esp)                  # 4-byte Spill
	movl	660(%esp), %ecx
	movl	%ecx, 8(%esp)                   # 4-byte Spill
	movl	656(%esp), %ebp
	subl	$4, %esp
	leal	612(%esp), %ecx
	pushl	%eax
	pushl	724(%esp)
	pushl	%ecx
	calll	mulPv256x32@PLT
	addl	$12, %esp
	addl	608(%esp), %edi
	adcl	612(%esp), %esi
	adcl	616(%esp), %ebp
	movl	8(%esp), %eax                   # 4-byte Reload
	adcl	620(%esp), %eax
	movl	%eax, 8(%esp)                   # 4-byte Spill
	movl	12(%esp), %eax                  # 4-byte Reload
	adcl	624(%esp), %eax
	movl	%eax, 12(%esp)                  # 4-byte Spill
	movl	36(%esp), %eax                  # 4-byte Reload
	adcl	628(%esp), %eax
	movl	%eax, 36(%esp)                  # 4-byte Spill
	movl	20(%esp), %edi                  # 4-byte Reload
	adcl	632(%esp), %edi
	movl	4(%esp), %eax                   # 4-byte Reload
	adcl	636(%esp), %eax
	movl	%eax, 4(%esp)                   # 4-byte Spill
	movl	16(%esp), %eax                  # 4-byte Reload
	adcl	640(%esp), %eax
	movl	%eax, 16(%esp)                  # 4-byte Spill
	setb	%al
	subl	$4, %esp
	leal	572(%esp), %ecx
	movzbl	%al, %eax
	movl	%eax, 32(%esp)                  # 4-byte Spill
	movl	716(%esp), %eax
	pushl	4(%eax)
	pushl	716(%esp)
	pushl	%ecx
	calll	mulPv256x32@PLT
	addl	$12, %esp
	addl	568(%esp), %esi
	adcl	572(%esp), %ebp
	movl	8(%esp), %eax                   # 4-byte Reload
	adcl	576(%esp), %eax
	movl	%eax, 8(%esp)                   # 4-byte Spill
	movl	12(%esp), %eax                  # 4-byte Reload
	adcl	580(%esp), %eax
	movl	%eax, 12(%esp)                  # 4-byte Spill
	movl	36(%esp), %eax                  # 4-byte Reload
	adcl	584(%esp), %eax
	movl	%eax, 36(%esp)                  # 4-byte Spill
	adcl	588(%esp), %edi
	movl	%edi, 20(%esp)                  # 4-byte Spill
	movl	4(%esp), %eax                   # 4-byte Reload
	adcl	592(%esp), %eax
	movl	%eax, 4(%esp)                   # 4-byte Spill
	movl	16(%esp), %eax                  # 4-byte Reload
	adcl	596(%esp), %eax
	movl	%eax, 16(%esp)                  # 4-byte Spill
	movl	28(%esp), %eax                  # 4-byte Reload
	adcl	600(%esp), %eax
	movl	%eax, 28(%esp)                  # 4-byte Spill
	setb	%al
	subl	$4, %esp
	leal	532(%esp), %ecx
	movl	48(%esp), %edx                  # 4-byte Reload
	imull	%esi, %edx
	movzbl	%al, %eax
	movl	%eax, 28(%esp)                  # 4-byte Spill
	pushl	%edx
	pushl	724(%esp)
	pushl	%ecx
	calll	mulPv256x32@PLT
	addl	$12, %esp
	addl	528(%esp), %esi
	adcl	532(%esp), %ebp
	movl	8(%esp), %eax                   # 4-byte Reload
	adcl	536(%esp), %eax
	movl	%eax, 8(%esp)                   # 4-byte Spill
	movl	12(%esp), %eax                  # 4-byte Reload
	adcl	540(%esp), %eax
	movl	%eax, 12(%esp)                  # 4-byte Spill
	movl	36(%esp), %edi                  # 4-byte Reload
	adcl	544(%esp), %edi
	movl	20(%esp), %eax                  # 4-byte Reload
	adcl	548(%esp), %eax
	movl	%eax, 20(%esp)                  # 4-byte Spill
	movl	4(%esp), %eax                   # 4-byte Reload
	adcl	552(%esp), %eax
	movl	%eax, 4(%esp)                   # 4-byte Spill
	movl	16(%esp), %eax                  # 4-byte Reload
	adcl	556(%esp), %eax
	movl	%eax, 16(%esp)                  # 4-byte Spill
	movl	28(%esp), %esi                  # 4-byte Reload
	adcl	560(%esp), %esi
	adcl	$0, 24(%esp)                    # 4-byte Folded Spill
	subl	$4, %esp
	leal	492(%esp), %eax
	movl	716(%esp), %ecx
	pushl	8(%ecx)
	pushl	716(%esp)
	pushl	%eax
	calll	mulPv256x32@PLT
	addl	$12, %esp
	addl	488(%esp), %ebp
	movl	8(%esp), %eax                   # 4-byte Reload
	adcl	492(%esp), %eax
	movl	%eax, 8(%esp)                   # 4-byte Spill
	movl	12(%esp), %eax                  # 4-byte Reload
	adcl	496(%esp), %eax
	movl	%eax, 12(%esp)                  # 4-byte Spill
	adcl	500(%esp), %edi
	movl	%edi, 36(%esp)                  # 4-byte Spill
	movl	20(%esp), %eax                  # 4-byte Reload
	adcl	504(%esp), %eax
	movl	%eax, 20(%esp)                  # 4-byte Spill
	movl	4(%esp), %eax                   # 4-byte Reload
	adcl	508(%esp), %eax
	movl	%eax, 4(%esp)                   # 4-byte Spill
	movl	16(%esp), %eax                  # 4-byte Reload
	adcl	512(%esp), %eax
	movl	%eax, 16(%esp)                  # 4-byte Spill
	adcl	516(%esp), %esi
	movl	%esi, 28(%esp)                  # 4-byte Spill
	movl	24(%esp), %eax                  # 4-byte Reload
	adcl	520(%esp), %eax
	movl	%eax, 24(%esp)                  # 4-byte Spill
	setb	%al
	subl	$4, %esp
	leal	452(%esp), %ecx
	movl	48(%esp), %edx                  # 4-byte Reload
	imull	%ebp, %edx
	movzbl	%al, %eax
	movl	%eax, 36(%esp)                  # 4-byte Spill
	pushl	%edx
	pushl	724(%esp)
	pushl	%ecx
	calll	mulPv256x32@PLT
	addl	$12, %esp
	addl	448(%esp), %ebp
	movl	8(%esp), %eax                   # 4-byte Reload
	adcl	452(%esp), %eax
	movl	%eax, 8(%esp)                   # 4-byte Spill
	movl	12(%esp), %edi                  # 4-byte Reload
	adcl	456(%esp), %edi
	movl	36(%esp), %eax                  # 4-byte Reload
	adcl	460(%esp), %eax
	movl	%eax, 36(%esp)                  # 4-byte Spill
	movl	20(%esp), %eax                  # 4-byte Reload
	adcl	464(%esp), %eax
	movl	%eax, 20(%esp)                  # 4-byte Spill
	movl	4(%esp), %ebp                   # 4-byte Reload
	adcl	468(%esp), %ebp
	movl	16(%esp), %esi                  # 4-byte Reload
	adcl	472(%esp), %esi
	movl	28(%esp), %eax                  # 4-byte Reload
	adcl	476(%esp), %eax
	movl	%eax, 28(%esp)                  # 4-byte Spill
	movl	24(%esp), %eax                  # 4-byte Reload
	adcl	480(%esp), %eax
	movl	%eax, 24(%esp)                  # 4-byte Spill
	adcl	$0, 32(%esp)                    # 4-byte Folded Spill
	subl	$4, %esp
	leal	412(%esp), %eax
	movl	716(%esp), %ecx
	pushl	12(%ecx)
	pushl	716(%esp)
	pushl	%eax
	calll	mulPv256x32@PLT
	addl	$12, %esp
	movl	8(%esp), %eax                   # 4-byte Reload
	addl	408(%esp), %eax
	adcl	412(%esp), %edi
	movl	%edi, 12(%esp)                  # 4-byte Spill
	movl	36(%esp), %ecx                  # 4-byte Reload
	adcl	416(%esp), %ecx
	movl	%ecx, 36(%esp)                  # 4-byte Spill
	movl	20(%esp), %ecx                  # 4-byte Reload
	adcl	420(%esp), %ecx
	movl	%ecx, 20(%esp)                  # 4-byte Spill
	adcl	424(%esp), %ebp
	movl	%ebp, 4(%esp)                   # 4-byte Spill
	adcl	428(%esp), %esi
	movl	%esi, 16(%esp)                  # 4-byte Spill
	movl	28(%esp), %esi                  # 4-byte Reload
	adcl	432(%esp), %esi
	movl	24(%esp), %ecx                  # 4-byte Reload
	adcl	436(%esp), %ecx
	movl	%ecx, 24(%esp)                  # 4-byte Spill
	movl	32(%esp), %ecx                  # 4-byte Reload
	adcl	440(%esp), %ecx
	movl	%ecx, 32(%esp)                  # 4-byte Spill
	setb	%cl
	subl	$4, %esp
	leal	372(%esp), %ebp
	movl	48(%esp), %edx                  # 4-byte Reload
	movl	%eax, %edi
	imull	%eax, %edx
	movzbl	%cl, %eax
	movl	%eax, 12(%esp)                  # 4-byte Spill
	pushl	%edx
	movl	724(%esp), %eax
	pushl	%eax
	pushl	%ebp
	calll	mulPv256x32@PLT
	addl	$12, %esp
	addl	368(%esp), %edi
	movl	12(%esp), %eax                  # 4-byte Reload
	adcl	372(%esp), %eax
	movl	%eax, 12(%esp)                  # 4-byte Spill
	movl	36(%esp), %ebp                  # 4-byte Reload
	adcl	376(%esp), %ebp
	movl	20(%esp), %edi                  # 4-byte Reload
	adcl	380(%esp), %edi
	movl	4(%esp), %eax                   # 4-byte Reload
	adcl	384(%esp), %eax
	movl	%eax, 4(%esp)                   # 4-byte Spill
	movl	16(%esp), %eax                  # 4-byte Reload
	adcl	388(%esp), %eax
	movl	%eax, 16(%esp)                  # 4-byte Spill
	adcl	392(%esp), %esi
	movl	%esi, 28(%esp)                  # 4-byte Spill
	movl	24(%esp), %esi                  # 4-byte Reload
	adcl	396(%esp), %esi
	movl	32(%esp), %eax                  # 4-byte Reload
	adcl	400(%esp), %eax
	movl	%eax, 32(%esp)                  # 4-byte Spill
	adcl	$0, 8(%esp)                     # 4-byte Folded Spill
	subl	$4, %esp
	leal	332(%esp), %eax
	movl	716(%esp), %ecx
	pushl	16(%ecx)
	pushl	716(%esp)
	pushl	%eax
	calll	mulPv256x32@PLT
	addl	$12, %esp
	movl	12(%esp), %eax                  # 4-byte Reload
	addl	328(%esp), %eax
	movl	%eax, 12(%esp)                  # 4-byte Spill
	adcl	332(%esp), %ebp
	movl	%ebp, 36(%esp)                  # 4-byte Spill
	adcl	336(%esp), %edi
	movl	%edi, 20(%esp)                  # 4-byte Spill
	movl	4(%esp), %ebp                   # 4-byte Reload
	adcl	340(%esp), %ebp
	movl	16(%esp), %ecx                  # 4-byte Reload
	adcl	344(%esp), %ecx
	movl	%ecx, 16(%esp)                  # 4-byte Spill
	movl	28(%esp), %edi                  # 4-byte Reload
	adcl	348(%esp), %edi
	adcl	352(%esp), %esi
	movl	%esi, 24(%esp)                  # 4-byte Spill
	movl	32(%esp), %esi                  # 4-byte Reload
	adcl	356(%esp), %esi
	movl	8(%esp), %ecx                   # 4-byte Reload
	adcl	360(%esp), %ecx
	movl	%ecx, 8(%esp)                   # 4-byte Spill
	setb	%cl
	subl	$4, %esp
	movl	48(%esp), %edx                  # 4-byte Reload
	imull	%eax, %edx
	movzbl	%cl, %eax
	movl	%eax, 44(%esp)                  # 4-byte Spill
	pushl	%edx
	pushl	724(%esp)
	leal	300(%esp), %eax
	pushl	%eax
	calll	mulPv256x32@PLT
	addl	$12, %esp
	movl	12(%esp), %eax                  # 4-byte Reload
	addl	288(%esp), %eax
	movl	36(%esp), %eax                  # 4-byte Reload
	adcl	292(%esp), %eax
	movl	%eax, 36(%esp)                  # 4-byte Spill
	movl	20(%esp), %eax                  # 4-byte Reload
	adcl	296(%esp), %eax
	movl	%eax, 20(%esp)                  # 4-byte Spill
	adcl	300(%esp), %ebp
	movl	%ebp, 4(%esp)                   # 4-byte Spill
	movl	16(%esp), %ebp                  # 4-byte Reload
	adcl	304(%esp), %ebp
	adcl	308(%esp), %edi
	movl	%edi, 28(%esp)                  # 4-byte Spill
	movl	24(%esp), %eax                  # 4-byte Reload
	adcl	312(%esp), %eax
	movl	%eax, 24(%esp)                  # 4-byte Spill
	adcl	316(%esp), %esi
	movl	%esi, 32(%esp)                  # 4-byte Spill
	movl	8(%esp), %edi                   # 4-byte Reload
	adcl	320(%esp), %edi
	adcl	$0, 40(%esp)                    # 4-byte Folded Spill
	subl	$4, %esp
	leal	252(%esp), %eax
	movl	716(%esp), %ecx
	pushl	20(%ecx)
	movl	716(%esp), %ecx
	pushl	%ecx
	pushl	%eax
	calll	mulPv256x32@PLT
	addl	$12, %esp
	movl	36(%esp), %eax                  # 4-byte Reload
	addl	248(%esp), %eax
	movl	20(%esp), %esi                  # 4-byte Reload
	adcl	252(%esp), %esi
	movl	4(%esp), %ecx                   # 4-byte Reload
	adcl	256(%esp), %ecx
	movl	%ecx, 4(%esp)                   # 4-byte Spill
	adcl	260(%esp), %ebp
	movl	%ebp, 16(%esp)                  # 4-byte Spill
	movl	28(%esp), %ebp                  # 4-byte Reload
	adcl	264(%esp), %ebp
	movl	24(%esp), %ecx                  # 4-byte Reload
	adcl	268(%esp), %ecx
	movl	%ecx, 24(%esp)                  # 4-byte Spill
	movl	32(%esp), %ecx                  # 4-byte Reload
	adcl	272(%esp), %ecx
	movl	%ecx, 32(%esp)                  # 4-byte Spill
	adcl	276(%esp), %edi
	movl	%edi, 8(%esp)                   # 4-byte Spill
	movl	40(%esp), %ecx                  # 4-byte Reload
	adcl	280(%esp), %ecx
	movl	%ecx, 40(%esp)                  # 4-byte Spill
	setb	%cl
	subl	$4, %esp
	movl	48(%esp), %edx                  # 4-byte Reload
	imull	%eax, %edx
	movl	%eax, %edi
	movzbl	%cl, %eax
	movl	%eax, 16(%esp)                  # 4-byte Spill
	pushl	%edx
	pushl	724(%esp)
	leal	220(%esp), %eax
	pushl	%eax
	calll	mulPv256x32@PLT
	addl	$12, %esp
	addl	208(%esp), %edi
	adcl	212(%esp), %esi
	movl	%esi, 20(%esp)                  # 4-byte Spill
	movl	4(%esp), %eax                   # 4-byte Reload
	adcl	216(%esp), %eax
	movl	%eax, 4(%esp)                   # 4-byte Spill
	movl	16(%esp), %esi                  # 4-byte Reload
	adcl	220(%esp), %esi
	adcl	224(%esp), %ebp
	movl	24(%esp), %eax                  # 4-byte Reload
	adcl	228(%esp), %eax
	movl	%eax, 24(%esp)                  # 4-byte Spill
	movl	32(%esp), %eax                  # 4-byte Reload
	adcl	232(%esp), %eax
	movl	%eax, 32(%esp)                  # 4-byte Spill
	movl	8(%esp), %eax                   # 4-byte Reload
	adcl	236(%esp), %eax
	movl	%eax, 8(%esp)                   # 4-byte Spill
	movl	40(%esp), %edi                  # 4-byte Reload
	adcl	240(%esp), %edi
	adcl	$0, 12(%esp)                    # 4-byte Folded Spill
	subl	$4, %esp
	movl	716(%esp), %eax
	pushl	24(%eax)
	movl	716(%esp), %eax
	pushl	%eax
	leal	180(%esp), %eax
	pushl	%eax
	calll	mulPv256x32@PLT
	addl	$12, %esp
	movl	20(%esp), %edx                  # 4-byte Reload
	addl	168(%esp), %edx
	movl	4(%esp), %eax                   # 4-byte Reload
	adcl	172(%esp), %eax
	movl	%eax, 4(%esp)                   # 4-byte Spill
	adcl	176(%esp), %esi
	movl	%esi, 16(%esp)                  # 4-byte Spill
	adcl	180(%esp), %ebp
	movl	%ebp, 28(%esp)                  # 4-byte Spill
	movl	24(%esp), %eax                  # 4-byte Reload
	adcl	184(%esp), %eax
	movl	%eax, 24(%esp)                  # 4-byte Spill
	movl	32(%esp), %eax                  # 4-byte Reload
	adcl	188(%esp), %eax
	movl	%eax, 32(%esp)                  # 4-byte Spill
	movl	8(%esp), %eax                   # 4-byte Reload
	adcl	192(%esp), %eax
	movl	%eax, 8(%esp)                   # 4-byte Spill
	adcl	196(%esp), %edi
	movl	%edi, 40(%esp)                  # 4-byte Spill
	movl	12(%esp), %ebp                  # 4-byte Reload
	adcl	200(%esp), %ebp
	setb	%al
	subl	$4, %esp
	movl	48(%esp), %ecx                  # 4-byte Reload
	imull	%edx, %ecx
	movl	%edx, %esi
	movzbl	%al, %eax
	movl	%eax, 24(%esp)                  # 4-byte Spill
	pushl	%ecx
	movl	724(%esp), %eax
	pushl	%eax
	leal	140(%esp), %eax
	pushl	%eax
	calll	mulPv256x32@PLT
	addl	$12, %esp
	addl	128(%esp), %esi
	movl	4(%esp), %eax                   # 4-byte Reload
	adcl	132(%esp), %eax
	movl	%eax, 4(%esp)                   # 4-byte Spill
	movl	16(%esp), %eax                  # 4-byte Reload
	adcl	136(%esp), %eax
	movl	%eax, 16(%esp)                  # 4-byte Spill
	movl	28(%esp), %eax                  # 4-byte Reload
	adcl	140(%esp), %eax
	movl	%eax, 28(%esp)                  # 4-byte Spill
	movl	24(%esp), %esi                  # 4-byte Reload
	adcl	144(%esp), %esi
	movl	32(%esp), %edi                  # 4-byte Reload
	adcl	148(%esp), %edi
	movl	8(%esp), %eax                   # 4-byte Reload
	adcl	152(%esp), %eax
	movl	%eax, 8(%esp)                   # 4-byte Spill
	movl	40(%esp), %eax                  # 4-byte Reload
	adcl	156(%esp), %eax
	movl	%eax, 40(%esp)                  # 4-byte Spill
	adcl	160(%esp), %ebp
	movl	%ebp, 12(%esp)                  # 4-byte Spill
	movl	20(%esp), %ebp                  # 4-byte Reload
	adcl	$0, %ebp
	subl	$4, %esp
	leal	92(%esp), %eax
	movl	716(%esp), %ecx
	pushl	28(%ecx)
	pushl	716(%esp)
	pushl	%eax
	calll	mulPv256x32@PLT
	addl	$12, %esp
	movl	4(%esp), %edx                   # 4-byte Reload
	addl	88(%esp), %edx
	movl	16(%esp), %eax                  # 4-byte Reload
	adcl	92(%esp), %eax
	movl	%eax, 16(%esp)                  # 4-byte Spill
	movl	28(%esp), %eax                  # 4-byte Reload
	adcl	96(%esp), %eax
	movl	%eax, 28(%esp)                  # 4-byte Spill
	adcl	100(%esp), %esi
	adcl	104(%esp), %edi
	movl	%edi, 32(%esp)                  # 4-byte Spill
	movl	8(%esp), %edi                   # 4-byte Reload
	adcl	108(%esp), %edi
	movl	40(%esp), %eax                  # 4-byte Reload
	adcl	112(%esp), %eax
	movl	%eax, 40(%esp)                  # 4-byte Spill
	movl	12(%esp), %eax                  # 4-byte Reload
	adcl	116(%esp), %eax
	movl	%eax, 12(%esp)                  # 4-byte Spill
	adcl	120(%esp), %ebp
	movl	%ebp, 20(%esp)                  # 4-byte Spill
	setb	4(%esp)                         # 1-byte Folded Spill
	subl	$4, %esp
	leal	52(%esp), %eax
	movl	48(%esp), %ecx                  # 4-byte Reload
	imull	%edx, %ecx
	movl	%edx, %ebp
	pushl	%ecx
	movl	724(%esp), %ecx
	pushl	%ecx
	pushl	%eax
	calll	mulPv256x32@PLT
	addl	$12, %esp
	addl	48(%esp), %ebp
	movzbl	4(%esp), %eax                   # 1-byte Folded Reload
	movl	16(%esp), %ecx                  # 4-byte Reload
	adcl	52(%esp), %ecx
	movl	28(%esp), %edx                  # 4-byte Reload
	adcl	56(%esp), %edx
	adcl	60(%esp), %esi
	movl	32(%esp), %ebx                  # 4-byte Reload
	adcl	64(%esp), %ebx
	movl	%edi, %ebp
	adcl	68(%esp), %ebp
	movl	40(%esp), %edi                  # 4-byte Reload
	adcl	72(%esp), %edi
	movl	%edi, 40(%esp)                  # 4-byte Spill
	movl	12(%esp), %edi                  # 4-byte Reload
	adcl	76(%esp), %edi
	movl	%edi, 12(%esp)                  # 4-byte Spill
	movl	20(%esp), %edi                  # 4-byte Reload
	adcl	80(%esp), %edi
	movl	%edi, 20(%esp)                  # 4-byte Spill
	adcl	$0, %eax
	movl	%ecx, 16(%esp)                  # 4-byte Spill
	movl	%ecx, %edi
	movl	716(%esp), %ecx
	subl	(%ecx), %edi
	movl	%edi, 4(%esp)                   # 4-byte Spill
	movl	%edx, 28(%esp)                  # 4-byte Spill
	sbbl	4(%ecx), %edx
	movl	%edx, 36(%esp)                  # 4-byte Spill
	movl	%esi, %edx
	movl	%esi, 24(%esp)                  # 4-byte Spill
	sbbl	8(%ecx), %edx
	movl	%edx, 44(%esp)                  # 4-byte Spill
	movl	%ebx, 32(%esp)                  # 4-byte Spill
	sbbl	12(%ecx), %ebx
	movl	%ebp, 8(%esp)                   # 4-byte Spill
	sbbl	16(%ecx), %ebp
	movl	%ecx, %edx
	movl	40(%esp), %ecx                  # 4-byte Reload
	sbbl	20(%edx), %ecx
	movl	12(%esp), %esi                  # 4-byte Reload
	sbbl	24(%edx), %esi
	movl	20(%esp), %edi                  # 4-byte Reload
	sbbl	28(%edx), %edi
	sbbl	$0, %eax
	testb	$1, %al
	jne	.LBB43_1
# %bb.2:
	movl	704(%esp), %eax
	movl	%edi, 28(%eax)
	jne	.LBB43_3
.LBB43_4:
	movl	%esi, 24(%eax)
	movl	36(%esp), %edx                  # 4-byte Reload
	jne	.LBB43_5
.LBB43_6:
	movl	%ecx, 20(%eax)
	movl	44(%esp), %esi                  # 4-byte Reload
	jne	.LBB43_7
.LBB43_8:
	movl	%ebp, 16(%eax)
	movl	4(%esp), %ecx                   # 4-byte Reload
	jne	.LBB43_9
.LBB43_10:
	movl	%ebx, 12(%eax)
	jne	.LBB43_11
.LBB43_12:
	movl	%esi, 8(%eax)
	jne	.LBB43_13
.LBB43_14:
	movl	%edx, 4(%eax)
	je	.LBB43_16
.LBB43_15:
	movl	16(%esp), %ecx                  # 4-byte Reload
.LBB43_16:
	movl	%ecx, (%eax)
	addl	$684, %esp                      # imm = 0x2AC
	popl	%esi
	popl	%edi
	popl	%ebx
	popl	%ebp
	retl
.LBB43_1:
	movl	20(%esp), %edi                  # 4-byte Reload
	movl	704(%esp), %eax
	movl	%edi, 28(%eax)
	je	.LBB43_4
.LBB43_3:
	movl	12(%esp), %esi                  # 4-byte Reload
	movl	%esi, 24(%eax)
	movl	36(%esp), %edx                  # 4-byte Reload
	je	.LBB43_6
.LBB43_5:
	movl	40(%esp), %ecx                  # 4-byte Reload
	movl	%ecx, 20(%eax)
	movl	44(%esp), %esi                  # 4-byte Reload
	je	.LBB43_8
.LBB43_7:
	movl	8(%esp), %ebp                   # 4-byte Reload
	movl	%ebp, 16(%eax)
	movl	4(%esp), %ecx                   # 4-byte Reload
	je	.LBB43_10
.LBB43_9:
	movl	32(%esp), %ebx                  # 4-byte Reload
	movl	%ebx, 12(%eax)
	je	.LBB43_12
.LBB43_11:
	movl	24(%esp), %esi                  # 4-byte Reload
	movl	%esi, 8(%eax)
	je	.LBB43_14
.LBB43_13:
	movl	28(%esp), %edx                  # 4-byte Reload
	movl	%edx, 4(%eax)
	jne	.LBB43_15
	jmp	.LBB43_16
.Lfunc_end43:
	.size	mcl_fp_mont8L, .Lfunc_end43-mcl_fp_mont8L
                                        # -- End function
	.globl	mcl_fp_montNF8L                 # -- Begin function mcl_fp_montNF8L
	.p2align	4, 0x90
	.type	mcl_fp_montNF8L,@function
mcl_fp_montNF8L:                        # @mcl_fp_montNF8L
# %bb.0:
	pushl	%ebp
	pushl	%ebx
	pushl	%edi
	pushl	%esi
	subl	$684, %esp                      # imm = 0x2AC
	calll	.L44$pb
.L44$pb:
	popl	%ebx
.Ltmp6:
	addl	$_GLOBAL_OFFSET_TABLE_+(.Ltmp6-.L44$pb), %ebx
	movl	716(%esp), %eax
	movl	-4(%eax), %esi
	movl	%esi, 40(%esp)                  # 4-byte Spill
	movl	712(%esp), %ecx
	subl	$4, %esp
	leal	652(%esp), %eax
	pushl	(%ecx)
	pushl	716(%esp)
	pushl	%eax
	calll	mulPv256x32@PLT
	addl	$12, %esp
	movl	648(%esp), %ebp
	movl	652(%esp), %edi
	movl	%esi, %eax
	imull	%ebp, %eax
	movl	680(%esp), %ecx
	movl	%ecx, 24(%esp)                  # 4-byte Spill
	movl	676(%esp), %ecx
	movl	%ecx, 16(%esp)                  # 4-byte Spill
	movl	672(%esp), %ecx
	movl	%ecx, 32(%esp)                  # 4-byte Spill
	movl	668(%esp), %ecx
	movl	%ecx, 12(%esp)                  # 4-byte Spill
	movl	664(%esp), %ecx
	movl	%ecx, 20(%esp)                  # 4-byte Spill
	movl	660(%esp), %ecx
	movl	%ecx, 8(%esp)                   # 4-byte Spill
	movl	656(%esp), %esi
	subl	$4, %esp
	leal	612(%esp), %ecx
	pushl	%eax
	pushl	724(%esp)
	pushl	%ecx
	calll	mulPv256x32@PLT
	addl	$12, %esp
	addl	608(%esp), %ebp
	adcl	612(%esp), %edi
	adcl	616(%esp), %esi
	movl	8(%esp), %eax                   # 4-byte Reload
	adcl	620(%esp), %eax
	movl	%eax, 8(%esp)                   # 4-byte Spill
	movl	20(%esp), %eax                  # 4-byte Reload
	adcl	624(%esp), %eax
	movl	%eax, 20(%esp)                  # 4-byte Spill
	movl	12(%esp), %eax                  # 4-byte Reload
	adcl	628(%esp), %eax
	movl	%eax, 12(%esp)                  # 4-byte Spill
	movl	32(%esp), %ebp                  # 4-byte Reload
	adcl	632(%esp), %ebp
	movl	16(%esp), %eax                  # 4-byte Reload
	adcl	636(%esp), %eax
	movl	%eax, 16(%esp)                  # 4-byte Spill
	movl	24(%esp), %eax                  # 4-byte Reload
	adcl	640(%esp), %eax
	movl	%eax, 24(%esp)                  # 4-byte Spill
	subl	$4, %esp
	leal	572(%esp), %eax
	movl	716(%esp), %ecx
	pushl	4(%ecx)
	pushl	716(%esp)
	pushl	%eax
	calll	mulPv256x32@PLT
	addl	$12, %esp
	movl	600(%esp), %eax
	addl	568(%esp), %edi
	adcl	572(%esp), %esi
	movl	8(%esp), %ecx                   # 4-byte Reload
	adcl	576(%esp), %ecx
	movl	%ecx, 8(%esp)                   # 4-byte Spill
	movl	20(%esp), %ecx                  # 4-byte Reload
	adcl	580(%esp), %ecx
	movl	%ecx, 20(%esp)                  # 4-byte Spill
	movl	12(%esp), %ecx                  # 4-byte Reload
	adcl	584(%esp), %ecx
	movl	%ecx, 12(%esp)                  # 4-byte Spill
	adcl	588(%esp), %ebp
	movl	%ebp, 32(%esp)                  # 4-byte Spill
	movl	16(%esp), %ecx                  # 4-byte Reload
	adcl	592(%esp), %ecx
	movl	%ecx, 16(%esp)                  # 4-byte Spill
	movl	24(%esp), %ecx                  # 4-byte Reload
	adcl	596(%esp), %ecx
	movl	%ecx, 24(%esp)                  # 4-byte Spill
	adcl	$0, %eax
	movl	%eax, %ebp
	subl	$4, %esp
	leal	532(%esp), %eax
	movl	44(%esp), %ecx                  # 4-byte Reload
	imull	%edi, %ecx
	pushl	%ecx
	movl	724(%esp), %ecx
	pushl	%ecx
	pushl	%eax
	calll	mulPv256x32@PLT
	addl	$12, %esp
	addl	528(%esp), %edi
	adcl	532(%esp), %esi
	movl	8(%esp), %eax                   # 4-byte Reload
	adcl	536(%esp), %eax
	movl	%eax, 8(%esp)                   # 4-byte Spill
	movl	20(%esp), %eax                  # 4-byte Reload
	adcl	540(%esp), %eax
	movl	%eax, 20(%esp)                  # 4-byte Spill
	movl	12(%esp), %eax                  # 4-byte Reload
	adcl	544(%esp), %eax
	movl	%eax, 12(%esp)                  # 4-byte Spill
	movl	32(%esp), %eax                  # 4-byte Reload
	adcl	548(%esp), %eax
	movl	%eax, 32(%esp)                  # 4-byte Spill
	movl	16(%esp), %eax                  # 4-byte Reload
	adcl	552(%esp), %eax
	movl	%eax, 16(%esp)                  # 4-byte Spill
	movl	24(%esp), %edi                  # 4-byte Reload
	adcl	556(%esp), %edi
	adcl	560(%esp), %ebp
	movl	%ebp, 36(%esp)                  # 4-byte Spill
	subl	$4, %esp
	leal	492(%esp), %eax
	movl	716(%esp), %ecx
	pushl	8(%ecx)
	pushl	716(%esp)
	pushl	%eax
	calll	mulPv256x32@PLT
	addl	$12, %esp
	movl	520(%esp), %ecx
	addl	488(%esp), %esi
	movl	8(%esp), %eax                   # 4-byte Reload
	adcl	492(%esp), %eax
	movl	%eax, 8(%esp)                   # 4-byte Spill
	movl	20(%esp), %eax                  # 4-byte Reload
	adcl	496(%esp), %eax
	movl	%eax, 20(%esp)                  # 4-byte Spill
	movl	12(%esp), %eax                  # 4-byte Reload
	adcl	500(%esp), %eax
	movl	%eax, 12(%esp)                  # 4-byte Spill
	movl	32(%esp), %ebp                  # 4-byte Reload
	adcl	504(%esp), %ebp
	movl	16(%esp), %eax                  # 4-byte Reload
	adcl	508(%esp), %eax
	movl	%eax, 16(%esp)                  # 4-byte Spill
	adcl	512(%esp), %edi
	movl	36(%esp), %eax                  # 4-byte Reload
	adcl	516(%esp), %eax
	movl	%eax, 36(%esp)                  # 4-byte Spill
	adcl	$0, %ecx
	movl	%ecx, 28(%esp)                  # 4-byte Spill
	subl	$4, %esp
	leal	452(%esp), %eax
	movl	44(%esp), %ecx                  # 4-byte Reload
	imull	%esi, %ecx
	pushl	%ecx
	pushl	724(%esp)
	pushl	%eax
	calll	mulPv256x32@PLT
	addl	$12, %esp
	addl	448(%esp), %esi
	movl	8(%esp), %eax                   # 4-byte Reload
	adcl	452(%esp), %eax
	movl	%eax, 8(%esp)                   # 4-byte Spill
	movl	20(%esp), %eax                  # 4-byte Reload
	adcl	456(%esp), %eax
	movl	%eax, 20(%esp)                  # 4-byte Spill
	movl	12(%esp), %eax                  # 4-byte Reload
	adcl	460(%esp), %eax
	movl	%eax, 12(%esp)                  # 4-byte Spill
	adcl	464(%esp), %ebp
	movl	%ebp, 32(%esp)                  # 4-byte Spill
	movl	16(%esp), %esi                  # 4-byte Reload
	adcl	468(%esp), %esi
	adcl	472(%esp), %edi
	movl	%edi, 24(%esp)                  # 4-byte Spill
	movl	36(%esp), %edi                  # 4-byte Reload
	adcl	476(%esp), %edi
	movl	28(%esp), %eax                  # 4-byte Reload
	adcl	480(%esp), %eax
	movl	%eax, 28(%esp)                  # 4-byte Spill
	subl	$4, %esp
	leal	412(%esp), %eax
	movl	716(%esp), %ecx
	pushl	12(%ecx)
	movl	716(%esp), %ecx
	pushl	%ecx
	pushl	%eax
	calll	mulPv256x32@PLT
	addl	$12, %esp
	movl	440(%esp), %ecx
	movl	8(%esp), %edx                   # 4-byte Reload
	addl	408(%esp), %edx
	movl	20(%esp), %ebp                  # 4-byte Reload
	adcl	412(%esp), %ebp
	movl	12(%esp), %eax                  # 4-byte Reload
	adcl	416(%esp), %eax
	movl	%eax, 12(%esp)                  # 4-byte Spill
	movl	32(%esp), %eax                  # 4-byte Reload
	adcl	420(%esp), %eax
	movl	%eax, 32(%esp)                  # 4-byte Spill
	adcl	424(%esp), %esi
	movl	%esi, 16(%esp)                  # 4-byte Spill
	movl	24(%esp), %eax                  # 4-byte Reload
	adcl	428(%esp), %eax
	movl	%eax, 24(%esp)                  # 4-byte Spill
	adcl	432(%esp), %edi
	movl	28(%esp), %eax                  # 4-byte Reload
	adcl	436(%esp), %eax
	movl	%eax, 28(%esp)                  # 4-byte Spill
	adcl	$0, %ecx
	movl	%ecx, 8(%esp)                   # 4-byte Spill
	subl	$4, %esp
	leal	372(%esp), %eax
	movl	44(%esp), %ecx                  # 4-byte Reload
	imull	%edx, %ecx
	movl	%edx, %esi
	pushl	%ecx
	pushl	724(%esp)
	pushl	%eax
	calll	mulPv256x32@PLT
	addl	$12, %esp
	addl	368(%esp), %esi
	adcl	372(%esp), %ebp
	movl	%ebp, 20(%esp)                  # 4-byte Spill
	movl	12(%esp), %esi                  # 4-byte Reload
	adcl	376(%esp), %esi
	movl	32(%esp), %ebp                  # 4-byte Reload
	adcl	380(%esp), %ebp
	movl	16(%esp), %eax                  # 4-byte Reload
	adcl	384(%esp), %eax
	movl	%eax, 16(%esp)                  # 4-byte Spill
	movl	24(%esp), %eax                  # 4-byte Reload
	adcl	388(%esp), %eax
	movl	%eax, 24(%esp)                  # 4-byte Spill
	adcl	392(%esp), %edi
	movl	%edi, 36(%esp)                  # 4-byte Spill
	movl	28(%esp), %edi                  # 4-byte Reload
	adcl	396(%esp), %edi
	movl	8(%esp), %eax                   # 4-byte Reload
	adcl	400(%esp), %eax
	movl	%eax, 8(%esp)                   # 4-byte Spill
	subl	$4, %esp
	leal	332(%esp), %eax
	movl	716(%esp), %ecx
	pushl	16(%ecx)
	pushl	716(%esp)
	pushl	%eax
	calll	mulPv256x32@PLT
	addl	$12, %esp
	movl	360(%esp), %eax
	movl	20(%esp), %edx                  # 4-byte Reload
	addl	328(%esp), %edx
	adcl	332(%esp), %esi
	movl	%esi, 12(%esp)                  # 4-byte Spill
	adcl	336(%esp), %ebp
	movl	%ebp, 32(%esp)                  # 4-byte Spill
	movl	16(%esp), %ecx                  # 4-byte Reload
	adcl	340(%esp), %ecx
	movl	%ecx, 16(%esp)                  # 4-byte Spill
	movl	24(%esp), %esi                  # 4-byte Reload
	adcl	344(%esp), %esi
	movl	36(%esp), %ecx                  # 4-byte Reload
	adcl	348(%esp), %ecx
	movl	%ecx, 36(%esp)                  # 4-byte Spill
	adcl	352(%esp), %edi
	movl	%edi, 28(%esp)                  # 4-byte Spill
	movl	8(%esp), %ecx                   # 4-byte Reload
	adcl	356(%esp), %ecx
	movl	%ecx, 8(%esp)                   # 4-byte Spill
	adcl	$0, %eax
	movl	%eax, %ebp
	subl	$4, %esp
	leal	292(%esp), %eax
	movl	44(%esp), %ecx                  # 4-byte Reload
	imull	%edx, %ecx
	movl	%edx, %edi
	pushl	%ecx
	pushl	724(%esp)
	pushl	%eax
	calll	mulPv256x32@PLT
	addl	$12, %esp
	addl	288(%esp), %edi
	movl	12(%esp), %eax                  # 4-byte Reload
	adcl	292(%esp), %eax
	movl	%eax, 12(%esp)                  # 4-byte Spill
	movl	32(%esp), %eax                  # 4-byte Reload
	adcl	296(%esp), %eax
	movl	%eax, 32(%esp)                  # 4-byte Spill
	movl	16(%esp), %edi                  # 4-byte Reload
	adcl	300(%esp), %edi
	adcl	304(%esp), %esi
	movl	%esi, 24(%esp)                  # 4-byte Spill
	movl	36(%esp), %eax                  # 4-byte Reload
	adcl	308(%esp), %eax
	movl	%eax, 36(%esp)                  # 4-byte Spill
	movl	28(%esp), %eax                  # 4-byte Reload
	adcl	312(%esp), %eax
	movl	%eax, 28(%esp)                  # 4-byte Spill
	movl	8(%esp), %esi                   # 4-byte Reload
	adcl	316(%esp), %esi
	adcl	320(%esp), %ebp
	movl	%ebp, 20(%esp)                  # 4-byte Spill
	subl	$4, %esp
	leal	252(%esp), %eax
	movl	716(%esp), %ecx
	pushl	20(%ecx)
	movl	716(%esp), %ecx
	pushl	%ecx
	pushl	%eax
	calll	mulPv256x32@PLT
	addl	$12, %esp
	leal	208(%esp), %eax
	movl	280(%esp), %ebp
	movl	12(%esp), %edx                  # 4-byte Reload
	addl	248(%esp), %edx
	movl	32(%esp), %ecx                  # 4-byte Reload
	adcl	252(%esp), %ecx
	movl	%ecx, 32(%esp)                  # 4-byte Spill
	adcl	256(%esp), %edi
	movl	%edi, 16(%esp)                  # 4-byte Spill
	movl	24(%esp), %ecx                  # 4-byte Reload
	adcl	260(%esp), %ecx
	movl	%ecx, 24(%esp)                  # 4-byte Spill
	movl	36(%esp), %ecx                  # 4-byte Reload
	adcl	264(%esp), %ecx
	movl	%ecx, 36(%esp)                  # 4-byte Spill
	movl	28(%esp), %ecx                  # 4-byte Reload
	adcl	268(%esp), %ecx
	movl	%ecx, 28(%esp)                  # 4-byte Spill
	adcl	272(%esp), %esi
	movl	%esi, 8(%esp)                   # 4-byte Spill
	movl	20(%esp), %ecx                  # 4-byte Reload
	adcl	276(%esp), %ecx
	movl	%ecx, 20(%esp)                  # 4-byte Spill
	adcl	$0, %ebp
	subl	$4, %esp
	movl	44(%esp), %ecx                  # 4-byte Reload
	imull	%edx, %ecx
	movl	%edx, %esi
	pushl	%ecx
	pushl	724(%esp)
	pushl	%eax
	calll	mulPv256x32@PLT
	addl	$12, %esp
	addl	208(%esp), %esi
	movl	32(%esp), %edi                  # 4-byte Reload
	adcl	212(%esp), %edi
	movl	16(%esp), %eax                  # 4-byte Reload
	adcl	216(%esp), %eax
	movl	%eax, 16(%esp)                  # 4-byte Spill
	movl	24(%esp), %eax                  # 4-byte Reload
	adcl	220(%esp), %eax
	movl	%eax, 24(%esp)                  # 4-byte Spill
	movl	36(%esp), %esi                  # 4-byte Reload
	adcl	224(%esp), %esi
	movl	28(%esp), %eax                  # 4-byte Reload
	adcl	228(%esp), %eax
	movl	%eax, 28(%esp)                  # 4-byte Spill
	movl	8(%esp), %eax                   # 4-byte Reload
	adcl	232(%esp), %eax
	movl	%eax, 8(%esp)                   # 4-byte Spill
	movl	20(%esp), %eax                  # 4-byte Reload
	adcl	236(%esp), %eax
	movl	%eax, 20(%esp)                  # 4-byte Spill
	adcl	240(%esp), %ebp
	movl	%ebp, 12(%esp)                  # 4-byte Spill
	subl	$4, %esp
	movl	716(%esp), %eax
	pushl	24(%eax)
	pushl	716(%esp)
	leal	180(%esp), %eax
	pushl	%eax
	calll	mulPv256x32@PLT
	addl	$12, %esp
	leal	128(%esp), %eax
	movl	200(%esp), %ebp
	movl	%edi, %edx
	addl	168(%esp), %edx
	movl	16(%esp), %ecx                  # 4-byte Reload
	adcl	172(%esp), %ecx
	movl	%ecx, 16(%esp)                  # 4-byte Spill
	movl	24(%esp), %edi                  # 4-byte Reload
	adcl	176(%esp), %edi
	adcl	180(%esp), %esi
	movl	%esi, 36(%esp)                  # 4-byte Spill
	movl	28(%esp), %ecx                  # 4-byte Reload
	adcl	184(%esp), %ecx
	movl	%ecx, 28(%esp)                  # 4-byte Spill
	movl	8(%esp), %ecx                   # 4-byte Reload
	adcl	188(%esp), %ecx
	movl	%ecx, 8(%esp)                   # 4-byte Spill
	movl	20(%esp), %ecx                  # 4-byte Reload
	adcl	192(%esp), %ecx
	movl	%ecx, 20(%esp)                  # 4-byte Spill
	movl	12(%esp), %ecx                  # 4-byte Reload
	adcl	196(%esp), %ecx
	movl	%ecx, 12(%esp)                  # 4-byte Spill
	adcl	$0, %ebp
	subl	$4, %esp
	movl	44(%esp), %ecx                  # 4-byte Reload
	imull	%edx, %ecx
	movl	%edx, %esi
	pushl	%ecx
	pushl	724(%esp)
	pushl	%eax
	calll	mulPv256x32@PLT
	addl	$12, %esp
	addl	128(%esp), %esi
	movl	16(%esp), %eax                  # 4-byte Reload
	adcl	132(%esp), %eax
	movl	%eax, 16(%esp)                  # 4-byte Spill
	adcl	136(%esp), %edi
	movl	%edi, 24(%esp)                  # 4-byte Spill
	movl	36(%esp), %esi                  # 4-byte Reload
	adcl	140(%esp), %esi
	movl	28(%esp), %eax                  # 4-byte Reload
	adcl	144(%esp), %eax
	movl	%eax, 28(%esp)                  # 4-byte Spill
	movl	8(%esp), %eax                   # 4-byte Reload
	adcl	148(%esp), %eax
	movl	%eax, 8(%esp)                   # 4-byte Spill
	movl	20(%esp), %edi                  # 4-byte Reload
	adcl	152(%esp), %edi
	movl	12(%esp), %eax                  # 4-byte Reload
	adcl	156(%esp), %eax
	movl	%eax, 12(%esp)                  # 4-byte Spill
	adcl	160(%esp), %ebp
	subl	$4, %esp
	leal	92(%esp), %eax
	movl	716(%esp), %ecx
	pushl	28(%ecx)
	pushl	716(%esp)
	pushl	%eax
	calll	mulPv256x32@PLT
	addl	$12, %esp
	movl	16(%esp), %edx                  # 4-byte Reload
	addl	88(%esp), %edx
	movl	24(%esp), %eax                  # 4-byte Reload
	adcl	92(%esp), %eax
	movl	%eax, 24(%esp)                  # 4-byte Spill
	adcl	96(%esp), %esi
	movl	%esi, 36(%esp)                  # 4-byte Spill
	movl	28(%esp), %eax                  # 4-byte Reload
	adcl	100(%esp), %eax
	movl	%eax, 28(%esp)                  # 4-byte Spill
	movl	8(%esp), %eax                   # 4-byte Reload
	adcl	104(%esp), %eax
	movl	%eax, 8(%esp)                   # 4-byte Spill
	adcl	108(%esp), %edi
	movl	12(%esp), %eax                  # 4-byte Reload
	adcl	112(%esp), %eax
	movl	%eax, 12(%esp)                  # 4-byte Spill
	adcl	116(%esp), %ebp
	movl	%ebp, 16(%esp)                  # 4-byte Spill
	movl	120(%esp), %ebp
	adcl	$0, %ebp
	subl	$4, %esp
	leal	52(%esp), %eax
	movl	44(%esp), %ecx                  # 4-byte Reload
	imull	%edx, %ecx
	movl	%edx, %esi
	pushl	%ecx
	movl	724(%esp), %ecx
	pushl	%ecx
	pushl	%eax
	calll	mulPv256x32@PLT
	addl	$12, %esp
	addl	48(%esp), %esi
	movl	24(%esp), %eax                  # 4-byte Reload
	adcl	52(%esp), %eax
	movl	36(%esp), %ecx                  # 4-byte Reload
	adcl	56(%esp), %ecx
	movl	28(%esp), %edx                  # 4-byte Reload
	adcl	60(%esp), %edx
	movl	8(%esp), %esi                   # 4-byte Reload
	adcl	64(%esp), %esi
	adcl	68(%esp), %edi
	movl	12(%esp), %ebx                  # 4-byte Reload
	adcl	72(%esp), %ebx
	movl	%ebx, 12(%esp)                  # 4-byte Spill
	movl	16(%esp), %ebx                  # 4-byte Reload
	adcl	76(%esp), %ebx
	movl	%ebx, 16(%esp)                  # 4-byte Spill
	adcl	80(%esp), %ebp
	movl	%eax, 24(%esp)                  # 4-byte Spill
	movl	716(%esp), %ebx
	subl	(%ebx), %eax
	movl	%eax, 32(%esp)                  # 4-byte Spill
	movl	%ecx, %eax
	movl	%ecx, 36(%esp)                  # 4-byte Spill
	sbbl	4(%ebx), %eax
	movl	%eax, 40(%esp)                  # 4-byte Spill
	movl	%edx, 28(%esp)                  # 4-byte Spill
	sbbl	8(%ebx), %edx
	movl	%esi, %eax
	movl	%esi, 8(%esp)                   # 4-byte Spill
	sbbl	12(%ebx), %eax
	movl	%edi, 20(%esp)                  # 4-byte Spill
	movl	%edi, %ecx
	sbbl	16(%ebx), %ecx
	movl	12(%esp), %edi                  # 4-byte Reload
	sbbl	20(%ebx), %edi
	movl	16(%esp), %esi                  # 4-byte Reload
	sbbl	24(%ebx), %esi
	movl	%ebp, 44(%esp)                  # 4-byte Spill
	movl	716(%esp), %ebx
	sbbl	28(%ebx), %ebp
	testl	%ebp, %ebp
	js	.LBB44_1
# %bb.2:
	movl	704(%esp), %ebx
	movl	%ebp, 28(%ebx)
	js	.LBB44_3
.LBB44_4:
	movl	%esi, 24(%ebx)
	js	.LBB44_5
.LBB44_6:
	movl	%edi, 20(%ebx)
	js	.LBB44_7
.LBB44_8:
	movl	%ecx, 16(%ebx)
	js	.LBB44_9
.LBB44_10:
	movl	%eax, 12(%ebx)
	movl	40(%esp), %ecx                  # 4-byte Reload
	js	.LBB44_11
.LBB44_12:
	movl	%edx, 8(%ebx)
	movl	32(%esp), %eax                  # 4-byte Reload
	js	.LBB44_13
.LBB44_14:
	movl	%ecx, 4(%ebx)
	jns	.LBB44_16
.LBB44_15:
	movl	24(%esp), %eax                  # 4-byte Reload
.LBB44_16:
	movl	%eax, (%ebx)
	addl	$684, %esp                      # imm = 0x2AC
	popl	%esi
	popl	%edi
	popl	%ebx
	popl	%ebp
	retl
.LBB44_1:
	movl	44(%esp), %ebp                  # 4-byte Reload
	movl	704(%esp), %ebx
	movl	%ebp, 28(%ebx)
	jns	.LBB44_4
.LBB44_3:
	movl	16(%esp), %esi                  # 4-byte Reload
	movl	%esi, 24(%ebx)
	jns	.LBB44_6
.LBB44_5:
	movl	12(%esp), %edi                  # 4-byte Reload
	movl	%edi, 20(%ebx)
	jns	.LBB44_8
.LBB44_7:
	movl	20(%esp), %ecx                  # 4-byte Reload
	movl	%ecx, 16(%ebx)
	jns	.LBB44_10
.LBB44_9:
	movl	8(%esp), %eax                   # 4-byte Reload
	movl	%eax, 12(%ebx)
	movl	40(%esp), %ecx                  # 4-byte Reload
	jns	.LBB44_12
.LBB44_11:
	movl	28(%esp), %edx                  # 4-byte Reload
	movl	%edx, 8(%ebx)
	movl	32(%esp), %eax                  # 4-byte Reload
	jns	.LBB44_14
.LBB44_13:
	movl	36(%esp), %ecx                  # 4-byte Reload
	movl	%ecx, 4(%ebx)
	js	.LBB44_15
	jmp	.LBB44_16
.Lfunc_end44:
	.size	mcl_fp_montNF8L, .Lfunc_end44-mcl_fp_montNF8L
                                        # -- End function
	.globl	mcl_fp_montRed8L                # -- Begin function mcl_fp_montRed8L
	.p2align	4, 0x90
	.type	mcl_fp_montRed8L,@function
mcl_fp_montRed8L:                       # @mcl_fp_montRed8L
# %bb.0:
	pushl	%ebp
	pushl	%ebx
	pushl	%edi
	pushl	%esi
	subl	$380, %esp                      # imm = 0x17C
	calll	.L45$pb
.L45$pb:
	popl	%ebx
.Ltmp7:
	addl	$_GLOBAL_OFFSET_TABLE_+(.Ltmp7-.L45$pb), %ebx
	movl	408(%esp), %ecx
	movl	28(%ecx), %eax
	movl	%eax, 60(%esp)                  # 4-byte Spill
	movl	24(%ecx), %eax
	movl	%eax, 56(%esp)                  # 4-byte Spill
	movl	20(%ecx), %eax
	movl	%eax, 52(%esp)                  # 4-byte Spill
	movl	16(%ecx), %eax
	movl	%eax, 36(%esp)                  # 4-byte Spill
	movl	12(%ecx), %eax
	movl	%eax, 32(%esp)                  # 4-byte Spill
	movl	8(%ecx), %eax
	movl	%eax, 48(%esp)                  # 4-byte Spill
	movl	4(%ecx), %eax
	movl	%ecx, %edx
	movl	%eax, 44(%esp)                  # 4-byte Spill
	movl	404(%esp), %eax
	movl	28(%eax), %ecx
	movl	%ecx, 16(%esp)                  # 4-byte Spill
	movl	24(%eax), %ecx
	movl	%ecx, 4(%esp)                   # 4-byte Spill
	movl	20(%eax), %ecx
	movl	%ecx, 24(%esp)                  # 4-byte Spill
	movl	16(%eax), %esi
	movl	12(%eax), %ecx
	movl	%ecx, (%esp)                    # 4-byte Spill
	movl	8(%eax), %ecx
	movl	%ecx, 12(%esp)                  # 4-byte Spill
	movl	(%eax), %edi
	movl	4(%eax), %ebp
	movl	-4(%edx), %ecx
	movl	%ecx, 28(%esp)                  # 4-byte Spill
	movl	(%edx), %eax
	movl	%eax, 40(%esp)                  # 4-byte Spill
	subl	$4, %esp
	movl	%edi, %eax
	imull	%ecx, %eax
	leal	348(%esp), %ecx
	pushl	%eax
	pushl	%edx
	pushl	%ecx
	calll	mulPv256x32@PLT
	addl	$12, %esp
	addl	344(%esp), %edi
	adcl	348(%esp), %ebp
	movl	12(%esp), %eax                  # 4-byte Reload
	adcl	352(%esp), %eax
	movl	%eax, 12(%esp)                  # 4-byte Spill
	movl	(%esp), %eax                    # 4-byte Reload
	adcl	356(%esp), %eax
	movl	%eax, (%esp)                    # 4-byte Spill
	adcl	360(%esp), %esi
	movl	%esi, 8(%esp)                   # 4-byte Spill
	movl	24(%esp), %esi                  # 4-byte Reload
	adcl	364(%esp), %esi
	movl	4(%esp), %eax                   # 4-byte Reload
	adcl	368(%esp), %eax
	movl	%eax, 4(%esp)                   # 4-byte Spill
	movl	16(%esp), %edi                  # 4-byte Reload
	adcl	372(%esp), %edi
	movl	404(%esp), %eax
	movl	32(%eax), %eax
	adcl	376(%esp), %eax
	movl	%eax, 20(%esp)                  # 4-byte Spill
	setb	16(%esp)                        # 1-byte Folded Spill
	subl	$4, %esp
	movl	32(%esp), %eax                  # 4-byte Reload
	imull	%ebp, %eax
	leal	308(%esp), %ecx
	pushl	%eax
	pushl	416(%esp)
	pushl	%ecx
	calll	mulPv256x32@PLT
	addl	$12, %esp
	addb	$255, 16(%esp)                  # 1-byte Folded Spill
	movl	336(%esp), %eax
	adcl	$0, %eax
	addl	304(%esp), %ebp
	movl	12(%esp), %edx                  # 4-byte Reload
	adcl	308(%esp), %edx
	movl	(%esp), %ecx                    # 4-byte Reload
	adcl	312(%esp), %ecx
	movl	%ecx, (%esp)                    # 4-byte Spill
	movl	8(%esp), %ecx                   # 4-byte Reload
	adcl	316(%esp), %ecx
	movl	%ecx, 8(%esp)                   # 4-byte Spill
	adcl	320(%esp), %esi
	movl	%esi, 24(%esp)                  # 4-byte Spill
	movl	4(%esp), %ecx                   # 4-byte Reload
	adcl	324(%esp), %ecx
	movl	%ecx, 4(%esp)                   # 4-byte Spill
	adcl	328(%esp), %edi
	movl	20(%esp), %ebp                  # 4-byte Reload
	adcl	332(%esp), %ebp
	movl	404(%esp), %ecx
	adcl	36(%ecx), %eax
	movl	%eax, 12(%esp)                  # 4-byte Spill
	setb	20(%esp)                        # 1-byte Folded Spill
	subl	$4, %esp
	movl	32(%esp), %eax                  # 4-byte Reload
	imull	%edx, %eax
	movl	%edx, %esi
	leal	268(%esp), %ecx
	pushl	%eax
	pushl	416(%esp)
	pushl	%ecx
	calll	mulPv256x32@PLT
	addl	$12, %esp
	addb	$255, 20(%esp)                  # 1-byte Folded Spill
	movl	296(%esp), %eax
	adcl	$0, %eax
	addl	264(%esp), %esi
	movl	(%esp), %edx                    # 4-byte Reload
	adcl	268(%esp), %edx
	movl	8(%esp), %ecx                   # 4-byte Reload
	adcl	272(%esp), %ecx
	movl	%ecx, 8(%esp)                   # 4-byte Spill
	movl	24(%esp), %ecx                  # 4-byte Reload
	adcl	276(%esp), %ecx
	movl	%ecx, 24(%esp)                  # 4-byte Spill
	movl	4(%esp), %ecx                   # 4-byte Reload
	adcl	280(%esp), %ecx
	movl	%ecx, 4(%esp)                   # 4-byte Spill
	adcl	284(%esp), %edi
	movl	%edi, 16(%esp)                  # 4-byte Spill
	adcl	288(%esp), %ebp
	movl	%ebp, 20(%esp)                  # 4-byte Spill
	movl	12(%esp), %ebp                  # 4-byte Reload
	adcl	292(%esp), %ebp
	movl	404(%esp), %ecx
	adcl	40(%ecx), %eax
	movl	%eax, %esi
	setb	(%esp)                          # 1-byte Folded Spill
	subl	$4, %esp
	movl	32(%esp), %eax                  # 4-byte Reload
	imull	%edx, %eax
	movl	%edx, %edi
	leal	228(%esp), %ecx
	pushl	%eax
	pushl	416(%esp)
	pushl	%ecx
	calll	mulPv256x32@PLT
	addl	$12, %esp
	addb	$255, (%esp)                    # 1-byte Folded Spill
	movl	256(%esp), %eax
	adcl	$0, %eax
	movl	%eax, %edx
	addl	224(%esp), %edi
	movl	8(%esp), %ecx                   # 4-byte Reload
	adcl	228(%esp), %ecx
	movl	%ecx, 8(%esp)                   # 4-byte Spill
	movl	24(%esp), %eax                  # 4-byte Reload
	adcl	232(%esp), %eax
	movl	%eax, 24(%esp)                  # 4-byte Spill
	movl	4(%esp), %edi                   # 4-byte Reload
	adcl	236(%esp), %edi
	movl	16(%esp), %eax                  # 4-byte Reload
	adcl	240(%esp), %eax
	movl	%eax, 16(%esp)                  # 4-byte Spill
	movl	20(%esp), %eax                  # 4-byte Reload
	adcl	244(%esp), %eax
	movl	%eax, 20(%esp)                  # 4-byte Spill
	adcl	248(%esp), %ebp
	movl	%ebp, 12(%esp)                  # 4-byte Spill
	adcl	252(%esp), %esi
	movl	%esi, (%esp)                    # 4-byte Spill
	movl	404(%esp), %eax
	adcl	44(%eax), %edx
	movl	%edx, %esi
	setb	4(%esp)                         # 1-byte Folded Spill
	subl	$4, %esp
	movl	32(%esp), %eax                  # 4-byte Reload
	imull	%ecx, %eax
	leal	188(%esp), %ecx
	pushl	%eax
	pushl	416(%esp)
	pushl	%ecx
	calll	mulPv256x32@PLT
	addl	$12, %esp
	addb	$255, 4(%esp)                   # 1-byte Folded Spill
	movl	216(%esp), %ebp
	adcl	$0, %ebp
	movl	8(%esp), %eax                   # 4-byte Reload
	addl	184(%esp), %eax
	movl	24(%esp), %ecx                  # 4-byte Reload
	adcl	188(%esp), %ecx
	movl	%ecx, 24(%esp)                  # 4-byte Spill
	adcl	192(%esp), %edi
	movl	%edi, 4(%esp)                   # 4-byte Spill
	movl	16(%esp), %eax                  # 4-byte Reload
	adcl	196(%esp), %eax
	movl	%eax, 16(%esp)                  # 4-byte Spill
	movl	20(%esp), %eax                  # 4-byte Reload
	adcl	200(%esp), %eax
	movl	%eax, 20(%esp)                  # 4-byte Spill
	movl	12(%esp), %edi                  # 4-byte Reload
	adcl	204(%esp), %edi
	movl	(%esp), %eax                    # 4-byte Reload
	adcl	208(%esp), %eax
	movl	%eax, (%esp)                    # 4-byte Spill
	adcl	212(%esp), %esi
	movl	%esi, 8(%esp)                   # 4-byte Spill
	movl	404(%esp), %eax
	adcl	48(%eax), %ebp
	setb	12(%esp)                        # 1-byte Folded Spill
	subl	$4, %esp
	movl	32(%esp), %eax                  # 4-byte Reload
	imull	%ecx, %eax
	leal	148(%esp), %ecx
	pushl	%eax
	pushl	416(%esp)
	pushl	%ecx
	calll	mulPv256x32@PLT
	addl	$12, %esp
	addb	$255, 12(%esp)                  # 1-byte Folded Spill
	movl	176(%esp), %esi
	adcl	$0, %esi
	movl	24(%esp), %eax                  # 4-byte Reload
	addl	144(%esp), %eax
	movl	4(%esp), %ecx                   # 4-byte Reload
	adcl	148(%esp), %ecx
	movl	%ecx, 4(%esp)                   # 4-byte Spill
	movl	16(%esp), %eax                  # 4-byte Reload
	adcl	152(%esp), %eax
	movl	%eax, 16(%esp)                  # 4-byte Spill
	movl	20(%esp), %eax                  # 4-byte Reload
	adcl	156(%esp), %eax
	movl	%eax, 20(%esp)                  # 4-byte Spill
	adcl	160(%esp), %edi
	movl	%edi, 12(%esp)                  # 4-byte Spill
	movl	(%esp), %eax                    # 4-byte Reload
	adcl	164(%esp), %eax
	movl	%eax, (%esp)                    # 4-byte Spill
	movl	8(%esp), %eax                   # 4-byte Reload
	adcl	168(%esp), %eax
	movl	%eax, 8(%esp)                   # 4-byte Spill
	adcl	172(%esp), %ebp
	movl	404(%esp), %eax
	adcl	52(%eax), %esi
	setb	24(%esp)                        # 1-byte Folded Spill
	subl	$4, %esp
	movl	32(%esp), %eax                  # 4-byte Reload
	imull	%ecx, %eax
	leal	108(%esp), %ecx
	pushl	%eax
	pushl	416(%esp)
	pushl	%ecx
	calll	mulPv256x32@PLT
	addl	$12, %esp
	addb	$255, 24(%esp)                  # 1-byte Folded Spill
	movl	136(%esp), %edi
	adcl	$0, %edi
	movl	4(%esp), %eax                   # 4-byte Reload
	addl	104(%esp), %eax
	movl	16(%esp), %eax                  # 4-byte Reload
	adcl	108(%esp), %eax
	movl	%eax, 16(%esp)                  # 4-byte Spill
	movl	20(%esp), %ecx                  # 4-byte Reload
	adcl	112(%esp), %ecx
	movl	%ecx, 20(%esp)                  # 4-byte Spill
	movl	12(%esp), %ecx                  # 4-byte Reload
	adcl	116(%esp), %ecx
	movl	%ecx, 12(%esp)                  # 4-byte Spill
	movl	(%esp), %ecx                    # 4-byte Reload
	adcl	120(%esp), %ecx
	movl	%ecx, (%esp)                    # 4-byte Spill
	movl	8(%esp), %ecx                   # 4-byte Reload
	adcl	124(%esp), %ecx
	movl	%ecx, 8(%esp)                   # 4-byte Spill
	adcl	128(%esp), %ebp
	adcl	132(%esp), %esi
	movl	404(%esp), %ecx
	adcl	56(%ecx), %edi
	setb	4(%esp)                         # 1-byte Folded Spill
	movl	28(%esp), %ecx                  # 4-byte Reload
	imull	%eax, %ecx
	subl	$4, %esp
	leal	68(%esp), %eax
	pushl	%ecx
	pushl	416(%esp)
	pushl	%eax
	calll	mulPv256x32@PLT
	addl	$12, %esp
	addb	$255, 4(%esp)                   # 1-byte Folded Spill
	movl	96(%esp), %eax
	adcl	$0, %eax
	movl	16(%esp), %ecx                  # 4-byte Reload
	addl	64(%esp), %ecx
	movl	20(%esp), %ecx                  # 4-byte Reload
	adcl	68(%esp), %ecx
	movl	12(%esp), %ebx                  # 4-byte Reload
	adcl	72(%esp), %ebx
	movl	(%esp), %edx                    # 4-byte Reload
	adcl	76(%esp), %edx
	movl	%edx, (%esp)                    # 4-byte Spill
	movl	8(%esp), %edx                   # 4-byte Reload
	adcl	80(%esp), %edx
	movl	%edx, 8(%esp)                   # 4-byte Spill
	adcl	84(%esp), %ebp
	adcl	88(%esp), %esi
	adcl	92(%esp), %edi
	movl	404(%esp), %edx
	adcl	60(%edx), %eax
	xorl	%edx, %edx
	movl	%ecx, 20(%esp)                  # 4-byte Spill
	subl	40(%esp), %ecx                  # 4-byte Folded Reload
	movl	%ecx, 4(%esp)                   # 4-byte Spill
	movl	%ebx, %ecx
	movl	%ebx, 12(%esp)                  # 4-byte Spill
	sbbl	44(%esp), %ecx                  # 4-byte Folded Reload
	movl	%ecx, 16(%esp)                  # 4-byte Spill
	movl	(%esp), %ecx                    # 4-byte Reload
	sbbl	48(%esp), %ecx                  # 4-byte Folded Reload
	movl	%ecx, 24(%esp)                  # 4-byte Spill
	movl	8(%esp), %ecx                   # 4-byte Reload
	sbbl	32(%esp), %ecx                  # 4-byte Folded Reload
	movl	%ecx, 28(%esp)                  # 4-byte Spill
	movl	%ebp, 32(%esp)                  # 4-byte Spill
	movl	%ebp, %ecx
	sbbl	36(%esp), %ecx                  # 4-byte Folded Reload
	movl	%esi, 36(%esp)                  # 4-byte Spill
	sbbl	52(%esp), %esi                  # 4-byte Folded Reload
	movl	%edi, %ebp
	sbbl	56(%esp), %ebp                  # 4-byte Folded Reload
	movl	%eax, %ebx
	sbbl	60(%esp), %ebx                  # 4-byte Folded Reload
	sbbl	%edx, %edx
	testb	$1, %dl
	jne	.LBB45_1
# %bb.2:
	movl	400(%esp), %eax
	movl	%ebx, 28(%eax)
	jne	.LBB45_3
.LBB45_4:
	movl	%ebp, 24(%eax)
	movl	16(%esp), %edx                  # 4-byte Reload
	jne	.LBB45_5
.LBB45_6:
	movl	%esi, 20(%eax)
	jne	.LBB45_7
.LBB45_8:
	movl	%ecx, 16(%eax)
	movl	24(%esp), %esi                  # 4-byte Reload
	movl	28(%esp), %ecx                  # 4-byte Reload
	jne	.LBB45_9
.LBB45_10:
	movl	%ecx, 12(%eax)
	movl	4(%esp), %ecx                   # 4-byte Reload
	jne	.LBB45_11
.LBB45_12:
	movl	%esi, 8(%eax)
	jne	.LBB45_13
.LBB45_14:
	movl	%edx, 4(%eax)
	je	.LBB45_16
.LBB45_15:
	movl	20(%esp), %ecx                  # 4-byte Reload
.LBB45_16:
	movl	%ecx, (%eax)
	addl	$380, %esp                      # imm = 0x17C
	popl	%esi
	popl	%edi
	popl	%ebx
	popl	%ebp
	retl
.LBB45_1:
	movl	%eax, %ebx
	movl	400(%esp), %eax
	movl	%ebx, 28(%eax)
	je	.LBB45_4
.LBB45_3:
	movl	%edi, %ebp
	movl	%ebp, 24(%eax)
	movl	16(%esp), %edx                  # 4-byte Reload
	je	.LBB45_6
.LBB45_5:
	movl	36(%esp), %esi                  # 4-byte Reload
	movl	%esi, 20(%eax)
	je	.LBB45_8
.LBB45_7:
	movl	32(%esp), %ecx                  # 4-byte Reload
	movl	%ecx, 16(%eax)
	movl	24(%esp), %esi                  # 4-byte Reload
	movl	28(%esp), %ecx                  # 4-byte Reload
	je	.LBB45_10
.LBB45_9:
	movl	8(%esp), %ecx                   # 4-byte Reload
	movl	%ecx, 12(%eax)
	movl	4(%esp), %ecx                   # 4-byte Reload
	je	.LBB45_12
.LBB45_11:
	movl	(%esp), %esi                    # 4-byte Reload
	movl	%esi, 8(%eax)
	je	.LBB45_14
.LBB45_13:
	movl	12(%esp), %edx                  # 4-byte Reload
	movl	%edx, 4(%eax)
	jne	.LBB45_15
	jmp	.LBB45_16
.Lfunc_end45:
	.size	mcl_fp_montRed8L, .Lfunc_end45-mcl_fp_montRed8L
                                        # -- End function
	.globl	mcl_fp_montRedNF8L              # -- Begin function mcl_fp_montRedNF8L
	.p2align	4, 0x90
	.type	mcl_fp_montRedNF8L,@function
mcl_fp_montRedNF8L:                     # @mcl_fp_montRedNF8L
# %bb.0:
	pushl	%ebp
	pushl	%ebx
	pushl	%edi
	pushl	%esi
	subl	$396, %esp                      # imm = 0x18C
	calll	.L46$pb
.L46$pb:
	popl	%ebx
.Ltmp8:
	addl	$_GLOBAL_OFFSET_TABLE_+(.Ltmp8-.L46$pb), %ebx
	movl	424(%esp), %ecx
	movl	28(%ecx), %eax
	movl	%eax, 76(%esp)                  # 4-byte Spill
	movl	24(%ecx), %eax
	movl	%eax, 72(%esp)                  # 4-byte Spill
	movl	20(%ecx), %eax
	movl	%eax, 52(%esp)                  # 4-byte Spill
	movl	16(%ecx), %eax
	movl	%eax, 68(%esp)                  # 4-byte Spill
	movl	12(%ecx), %eax
	movl	%eax, 64(%esp)                  # 4-byte Spill
	movl	8(%ecx), %eax
	movl	%eax, 48(%esp)                  # 4-byte Spill
	movl	4(%ecx), %eax
	movl	%ecx, %edx
	movl	%eax, 60(%esp)                  # 4-byte Spill
	movl	420(%esp), %eax
	movl	28(%eax), %ecx
	movl	%ecx, 24(%esp)                  # 4-byte Spill
	movl	24(%eax), %ecx
	movl	%ecx, 12(%esp)                  # 4-byte Spill
	movl	20(%eax), %esi
	movl	16(%eax), %ecx
	movl	%ecx, 16(%esp)                  # 4-byte Spill
	movl	12(%eax), %ecx
	movl	%ecx, 32(%esp)                  # 4-byte Spill
	movl	8(%eax), %ecx
	movl	%ecx, 20(%esp)                  # 4-byte Spill
	movl	(%eax), %ebp
	movl	4(%eax), %edi
	movl	-4(%edx), %ecx
	movl	%ecx, 40(%esp)                  # 4-byte Spill
	movl	(%edx), %eax
	movl	%eax, 56(%esp)                  # 4-byte Spill
	subl	$4, %esp
	movl	%ebp, %eax
	imull	%ecx, %eax
	leal	364(%esp), %ecx
	pushl	%eax
	pushl	%edx
	pushl	%ecx
	calll	mulPv256x32@PLT
	addl	$12, %esp
	addl	360(%esp), %ebp
	adcl	364(%esp), %edi
	movl	20(%esp), %eax                  # 4-byte Reload
	adcl	368(%esp), %eax
	movl	%eax, 20(%esp)                  # 4-byte Spill
	movl	32(%esp), %eax                  # 4-byte Reload
	adcl	372(%esp), %eax
	movl	%eax, 32(%esp)                  # 4-byte Spill
	movl	16(%esp), %eax                  # 4-byte Reload
	adcl	376(%esp), %eax
	movl	%eax, 16(%esp)                  # 4-byte Spill
	adcl	380(%esp), %esi
	movl	12(%esp), %eax                  # 4-byte Reload
	adcl	384(%esp), %eax
	movl	%eax, 12(%esp)                  # 4-byte Spill
	movl	24(%esp), %eax                  # 4-byte Reload
	adcl	388(%esp), %eax
	movl	%eax, 24(%esp)                  # 4-byte Spill
	movl	420(%esp), %eax
	movl	32(%eax), %eax
	adcl	392(%esp), %eax
	movl	%eax, 36(%esp)                  # 4-byte Spill
	setb	28(%esp)                        # 1-byte Folded Spill
	subl	$4, %esp
	movl	44(%esp), %ebp                  # 4-byte Reload
	movl	%ebp, %eax
	imull	%edi, %eax
	leal	324(%esp), %ecx
	pushl	%eax
	pushl	432(%esp)
	pushl	%ecx
	calll	mulPv256x32@PLT
	addl	$12, %esp
	addb	$255, 28(%esp)                  # 1-byte Folded Spill
	movl	352(%esp), %eax
	adcl	$0, %eax
	addl	320(%esp), %edi
	movl	20(%esp), %edx                  # 4-byte Reload
	adcl	324(%esp), %edx
	movl	32(%esp), %ecx                  # 4-byte Reload
	adcl	328(%esp), %ecx
	movl	%ecx, 32(%esp)                  # 4-byte Spill
	movl	16(%esp), %ecx                  # 4-byte Reload
	adcl	332(%esp), %ecx
	movl	%ecx, 16(%esp)                  # 4-byte Spill
	adcl	336(%esp), %esi
	movl	12(%esp), %edi                  # 4-byte Reload
	adcl	340(%esp), %edi
	movl	24(%esp), %ecx                  # 4-byte Reload
	adcl	344(%esp), %ecx
	movl	%ecx, 24(%esp)                  # 4-byte Spill
	movl	36(%esp), %ecx                  # 4-byte Reload
	adcl	348(%esp), %ecx
	movl	%ecx, 36(%esp)                  # 4-byte Spill
	movl	420(%esp), %ecx
	adcl	36(%ecx), %eax
	movl	%eax, 20(%esp)                  # 4-byte Spill
	setb	12(%esp)                        # 1-byte Folded Spill
	subl	$4, %esp
	movl	%ebp, %eax
	imull	%edx, %eax
	movl	%edx, %ebp
	leal	284(%esp), %ecx
	pushl	%eax
	pushl	432(%esp)
	pushl	%ecx
	calll	mulPv256x32@PLT
	addl	$12, %esp
	addb	$255, 12(%esp)                  # 1-byte Folded Spill
	movl	312(%esp), %eax
	adcl	$0, %eax
	addl	280(%esp), %ebp
	movl	32(%esp), %edx                  # 4-byte Reload
	adcl	284(%esp), %edx
	movl	16(%esp), %ecx                  # 4-byte Reload
	adcl	288(%esp), %ecx
	movl	%ecx, 16(%esp)                  # 4-byte Spill
	adcl	292(%esp), %esi
	movl	%esi, 28(%esp)                  # 4-byte Spill
	adcl	296(%esp), %edi
	movl	%edi, 12(%esp)                  # 4-byte Spill
	movl	24(%esp), %ebp                  # 4-byte Reload
	adcl	300(%esp), %ebp
	movl	36(%esp), %ecx                  # 4-byte Reload
	adcl	304(%esp), %ecx
	movl	%ecx, 36(%esp)                  # 4-byte Spill
	movl	20(%esp), %ecx                  # 4-byte Reload
	adcl	308(%esp), %ecx
	movl	%ecx, 20(%esp)                  # 4-byte Spill
	movl	420(%esp), %ecx
	adcl	40(%ecx), %eax
	movl	%eax, %edi
	setb	24(%esp)                        # 1-byte Folded Spill
	subl	$4, %esp
	movl	44(%esp), %eax                  # 4-byte Reload
	imull	%edx, %eax
	movl	%edx, %esi
	leal	244(%esp), %ecx
	pushl	%eax
	pushl	432(%esp)
	pushl	%ecx
	calll	mulPv256x32@PLT
	addl	$12, %esp
	addb	$255, 24(%esp)                  # 1-byte Folded Spill
	movl	272(%esp), %eax
	adcl	$0, %eax
	addl	240(%esp), %esi
	movl	16(%esp), %ecx                  # 4-byte Reload
	adcl	244(%esp), %ecx
	movl	28(%esp), %edx                  # 4-byte Reload
	adcl	248(%esp), %edx
	movl	%edx, 28(%esp)                  # 4-byte Spill
	movl	12(%esp), %edx                  # 4-byte Reload
	adcl	252(%esp), %edx
	movl	%edx, 12(%esp)                  # 4-byte Spill
	adcl	256(%esp), %ebp
	movl	36(%esp), %esi                  # 4-byte Reload
	adcl	260(%esp), %esi
	movl	20(%esp), %edx                  # 4-byte Reload
	adcl	264(%esp), %edx
	movl	%edx, 20(%esp)                  # 4-byte Spill
	adcl	268(%esp), %edi
	movl	%edi, 32(%esp)                  # 4-byte Spill
	movl	420(%esp), %edx
	adcl	44(%edx), %eax
	movl	%eax, 16(%esp)                  # 4-byte Spill
	setb	24(%esp)                        # 1-byte Folded Spill
	subl	$4, %esp
	movl	44(%esp), %eax                  # 4-byte Reload
	imull	%ecx, %eax
	movl	%ecx, %edi
	leal	204(%esp), %ecx
	pushl	%eax
	movl	432(%esp), %eax
	pushl	%eax
	pushl	%ecx
	calll	mulPv256x32@PLT
	addl	$12, %esp
	addb	$255, 24(%esp)                  # 1-byte Folded Spill
	movl	232(%esp), %eax
	adcl	$0, %eax
	movl	%eax, %edx
	addl	200(%esp), %edi
	movl	28(%esp), %ecx                  # 4-byte Reload
	adcl	204(%esp), %ecx
	movl	%ecx, 28(%esp)                  # 4-byte Spill
	movl	12(%esp), %eax                  # 4-byte Reload
	adcl	208(%esp), %eax
	movl	%eax, 12(%esp)                  # 4-byte Spill
	adcl	212(%esp), %ebp
	movl	%ebp, 24(%esp)                  # 4-byte Spill
	adcl	216(%esp), %esi
	movl	%esi, 36(%esp)                  # 4-byte Spill
	movl	20(%esp), %esi                  # 4-byte Reload
	adcl	220(%esp), %esi
	movl	32(%esp), %eax                  # 4-byte Reload
	adcl	224(%esp), %eax
	movl	%eax, 32(%esp)                  # 4-byte Spill
	movl	16(%esp), %eax                  # 4-byte Reload
	adcl	228(%esp), %eax
	movl	%eax, 16(%esp)                  # 4-byte Spill
	movl	420(%esp), %eax
	adcl	48(%eax), %edx
	movl	%edx, %edi
	setb	20(%esp)                        # 1-byte Folded Spill
	subl	$4, %esp
	movl	44(%esp), %ebp                  # 4-byte Reload
	movl	%ebp, %eax
	imull	%ecx, %eax
	leal	164(%esp), %ecx
	pushl	%eax
	pushl	432(%esp)
	pushl	%ecx
	calll	mulPv256x32@PLT
	addl	$12, %esp
	addb	$255, 20(%esp)                  # 1-byte Folded Spill
	movl	192(%esp), %eax
	adcl	$0, %eax
	movl	%eax, %edx
	movl	28(%esp), %eax                  # 4-byte Reload
	addl	160(%esp), %eax
	movl	12(%esp), %ecx                  # 4-byte Reload
	adcl	164(%esp), %ecx
	movl	24(%esp), %eax                  # 4-byte Reload
	adcl	168(%esp), %eax
	movl	%eax, 24(%esp)                  # 4-byte Spill
	movl	36(%esp), %eax                  # 4-byte Reload
	adcl	172(%esp), %eax
	movl	%eax, 36(%esp)                  # 4-byte Spill
	adcl	176(%esp), %esi
	movl	%esi, 20(%esp)                  # 4-byte Spill
	movl	32(%esp), %eax                  # 4-byte Reload
	adcl	180(%esp), %eax
	movl	%eax, 32(%esp)                  # 4-byte Spill
	movl	16(%esp), %eax                  # 4-byte Reload
	adcl	184(%esp), %eax
	movl	%eax, 16(%esp)                  # 4-byte Spill
	adcl	188(%esp), %edi
	movl	%edi, 12(%esp)                  # 4-byte Spill
	movl	420(%esp), %eax
	adcl	52(%eax), %edx
	movl	%edx, 28(%esp)                  # 4-byte Spill
	setb	47(%esp)                        # 1-byte Folded Spill
	subl	$4, %esp
	imull	%ecx, %ebp
	movl	%ecx, %esi
	leal	124(%esp), %ecx
	pushl	%ebp
	pushl	432(%esp)
	pushl	%ecx
	calll	mulPv256x32@PLT
	addl	$12, %esp
	addb	$255, 47(%esp)                  # 1-byte Folded Spill
	movl	152(%esp), %edi
	adcl	$0, %edi
	addl	120(%esp), %esi
	movl	24(%esp), %eax                  # 4-byte Reload
	adcl	124(%esp), %eax
	movl	%eax, 24(%esp)                  # 4-byte Spill
	movl	36(%esp), %ebp                  # 4-byte Reload
	adcl	128(%esp), %ebp
	movl	20(%esp), %ecx                  # 4-byte Reload
	adcl	132(%esp), %ecx
	movl	%ecx, 20(%esp)                  # 4-byte Spill
	movl	32(%esp), %esi                  # 4-byte Reload
	adcl	136(%esp), %esi
	movl	16(%esp), %ecx                  # 4-byte Reload
	adcl	140(%esp), %ecx
	movl	%ecx, 16(%esp)                  # 4-byte Spill
	movl	12(%esp), %ecx                  # 4-byte Reload
	adcl	144(%esp), %ecx
	movl	%ecx, 12(%esp)                  # 4-byte Spill
	movl	28(%esp), %ecx                  # 4-byte Reload
	adcl	148(%esp), %ecx
	movl	%ecx, 28(%esp)                  # 4-byte Spill
	movl	420(%esp), %ecx
	adcl	56(%ecx), %edi
	setb	36(%esp)                        # 1-byte Folded Spill
	movl	40(%esp), %ecx                  # 4-byte Reload
	imull	%eax, %ecx
	subl	$4, %esp
	leal	84(%esp), %eax
	pushl	%ecx
	pushl	432(%esp)
	pushl	%eax
	calll	mulPv256x32@PLT
	addl	$12, %esp
	addb	$255, 36(%esp)                  # 1-byte Folded Spill
	movl	112(%esp), %edx
	adcl	$0, %edx
	movl	24(%esp), %eax                  # 4-byte Reload
	addl	80(%esp), %eax
	movl	%ebp, %eax
	adcl	84(%esp), %eax
	movl	20(%esp), %ecx                  # 4-byte Reload
	adcl	88(%esp), %ecx
	adcl	92(%esp), %esi
	movl	16(%esp), %ebx                  # 4-byte Reload
	adcl	96(%esp), %ebx
	movl	12(%esp), %ebp                  # 4-byte Reload
	adcl	100(%esp), %ebp
	movl	%ebp, 12(%esp)                  # 4-byte Spill
	movl	28(%esp), %ebp                  # 4-byte Reload
	adcl	104(%esp), %ebp
	movl	%ebp, 28(%esp)                  # 4-byte Spill
	adcl	108(%esp), %edi
	movl	420(%esp), %ebp
	adcl	60(%ebp), %edx
	movl	%eax, 36(%esp)                  # 4-byte Spill
	subl	56(%esp), %eax                  # 4-byte Folded Reload
	movl	%eax, 24(%esp)                  # 4-byte Spill
	movl	%ecx, %eax
	movl	%ecx, 20(%esp)                  # 4-byte Spill
	sbbl	60(%esp), %eax                  # 4-byte Folded Reload
	movl	%eax, 40(%esp)                  # 4-byte Spill
	movl	%esi, %eax
	movl	%esi, 32(%esp)                  # 4-byte Spill
	sbbl	48(%esp), %eax                  # 4-byte Folded Reload
	movl	%eax, 48(%esp)                  # 4-byte Spill
	movl	%ebx, %eax
	movl	%ebx, 16(%esp)                  # 4-byte Spill
	sbbl	64(%esp), %eax                  # 4-byte Folded Reload
	movl	12(%esp), %ecx                  # 4-byte Reload
	sbbl	68(%esp), %ecx                  # 4-byte Folded Reload
	movl	28(%esp), %ebp                  # 4-byte Reload
	sbbl	52(%esp), %ebp                  # 4-byte Folded Reload
	movl	%edi, 52(%esp)                  # 4-byte Spill
	sbbl	72(%esp), %edi                  # 4-byte Folded Reload
	movl	%edx, %ebx
	sbbl	76(%esp), %ebx                  # 4-byte Folded Reload
	testl	%ebx, %ebx
	js	.LBB46_1
# %bb.2:
	movl	416(%esp), %edx
	movl	%ebx, 28(%edx)
	js	.LBB46_3
.LBB46_4:
	movl	%edi, 24(%edx)
	js	.LBB46_5
.LBB46_6:
	movl	%ebp, 20(%edx)
	js	.LBB46_7
.LBB46_8:
	movl	%ecx, 16(%edx)
	js	.LBB46_9
.LBB46_10:
	movl	%eax, 12(%edx)
	movl	40(%esp), %ecx                  # 4-byte Reload
	movl	48(%esp), %eax                  # 4-byte Reload
	js	.LBB46_11
.LBB46_12:
	movl	%eax, 8(%edx)
	movl	24(%esp), %eax                  # 4-byte Reload
	js	.LBB46_13
.LBB46_14:
	movl	%ecx, 4(%edx)
	jns	.LBB46_16
.LBB46_15:
	movl	36(%esp), %eax                  # 4-byte Reload
.LBB46_16:
	movl	%eax, (%edx)
	addl	$396, %esp                      # imm = 0x18C
	popl	%esi
	popl	%edi
	popl	%ebx
	popl	%ebp
	retl
.LBB46_1:
	movl	%edx, %ebx
	movl	416(%esp), %edx
	movl	%ebx, 28(%edx)
	jns	.LBB46_4
.LBB46_3:
	movl	52(%esp), %edi                  # 4-byte Reload
	movl	%edi, 24(%edx)
	jns	.LBB46_6
.LBB46_5:
	movl	28(%esp), %ebp                  # 4-byte Reload
	movl	%ebp, 20(%edx)
	jns	.LBB46_8
.LBB46_7:
	movl	12(%esp), %ecx                  # 4-byte Reload
	movl	%ecx, 16(%edx)
	jns	.LBB46_10
.LBB46_9:
	movl	16(%esp), %eax                  # 4-byte Reload
	movl	%eax, 12(%edx)
	movl	40(%esp), %ecx                  # 4-byte Reload
	movl	48(%esp), %eax                  # 4-byte Reload
	jns	.LBB46_12
.LBB46_11:
	movl	32(%esp), %eax                  # 4-byte Reload
	movl	%eax, 8(%edx)
	movl	24(%esp), %eax                  # 4-byte Reload
	jns	.LBB46_14
.LBB46_13:
	movl	20(%esp), %ecx                  # 4-byte Reload
	movl	%ecx, 4(%edx)
	js	.LBB46_15
	jmp	.LBB46_16
.Lfunc_end46:
	.size	mcl_fp_montRedNF8L, .Lfunc_end46-mcl_fp_montRedNF8L
                                        # -- End function
	.globl	mcl_fp_addPre8L                 # -- Begin function mcl_fp_addPre8L
	.p2align	4, 0x90
	.type	mcl_fp_addPre8L,@function
mcl_fp_addPre8L:                        # @mcl_fp_addPre8L
# %bb.0:
	pushl	%ebp
	pushl	%ebx
	pushl	%edi
	pushl	%esi
	subl	$8, %esp
	movl	32(%esp), %ecx
	movl	(%ecx), %eax
	movl	4(%ecx), %edx
	movl	36(%esp), %esi
	addl	(%esi), %eax
	movl	%eax, 4(%esp)                   # 4-byte Spill
	adcl	4(%esi), %edx
	movl	%edx, (%esp)                    # 4-byte Spill
	movl	28(%ecx), %edi
	movl	24(%ecx), %ebx
	movl	20(%ecx), %ebp
	movl	16(%ecx), %eax
	movl	12(%ecx), %edx
	movl	8(%ecx), %ecx
	adcl	8(%esi), %ecx
	adcl	12(%esi), %edx
	adcl	16(%esi), %eax
	adcl	20(%esi), %ebp
	adcl	24(%esi), %ebx
	adcl	28(%esi), %edi
	movl	28(%esp), %esi
	movl	%ebx, 24(%esi)
	movl	%ebp, 20(%esi)
	movl	%eax, 16(%esi)
	movl	%edx, 12(%esi)
	movl	%ecx, 8(%esi)
	movl	%edi, 28(%esi)
	movl	(%esp), %eax                    # 4-byte Reload
	movl	%eax, 4(%esi)
	movl	4(%esp), %eax                   # 4-byte Reload
	movl	%eax, (%esi)
	setb	%al
	movzbl	%al, %eax
	addl	$8, %esp
	popl	%esi
	popl	%edi
	popl	%ebx
	popl	%ebp
	retl
.Lfunc_end47:
	.size	mcl_fp_addPre8L, .Lfunc_end47-mcl_fp_addPre8L
                                        # -- End function
	.globl	mcl_fp_subPre8L                 # -- Begin function mcl_fp_subPre8L
	.p2align	4, 0x90
	.type	mcl_fp_subPre8L,@function
mcl_fp_subPre8L:                        # @mcl_fp_subPre8L
# %bb.0:
	pushl	%ebp
	pushl	%ebx
	pushl	%edi
	pushl	%esi
	subl	$16, %esp
	movl	40(%esp), %edx
	movl	(%edx), %ecx
	movl	4(%edx), %esi
	xorl	%eax, %eax
	movl	44(%esp), %edi
	subl	(%edi), %ecx
	movl	%ecx, 12(%esp)                  # 4-byte Spill
	sbbl	4(%edi), %esi
	movl	%esi, 8(%esp)                   # 4-byte Spill
	movl	28(%edx), %ecx
	movl	%ecx, (%esp)                    # 4-byte Spill
	movl	24(%edx), %ebp
	movl	20(%edx), %ecx
	movl	16(%edx), %esi
	movl	12(%edx), %ebx
	movl	8(%edx), %edx
	sbbl	8(%edi), %edx
	sbbl	12(%edi), %ebx
	sbbl	16(%edi), %esi
	sbbl	20(%edi), %ecx
	movl	%ecx, 4(%esp)                   # 4-byte Spill
	sbbl	24(%edi), %ebp
	movl	(%esp), %ecx                    # 4-byte Reload
	sbbl	28(%edi), %ecx
	movl	%ecx, (%esp)                    # 4-byte Spill
	movl	36(%esp), %edi
	movl	%ebp, 24(%edi)
	movl	4(%esp), %ecx                   # 4-byte Reload
	movl	%ecx, 20(%edi)
	movl	%esi, 16(%edi)
	movl	%ebx, 12(%edi)
	movl	%edx, 8(%edi)
	movl	8(%esp), %ecx                   # 4-byte Reload
	movl	%ecx, 4(%edi)
	movl	(%esp), %ecx                    # 4-byte Reload
	movl	%ecx, 28(%edi)
	movl	12(%esp), %ecx                  # 4-byte Reload
	movl	%ecx, (%edi)
	sbbl	%eax, %eax
	andl	$1, %eax
	addl	$16, %esp
	popl	%esi
	popl	%edi
	popl	%ebx
	popl	%ebp
	retl
.Lfunc_end48:
	.size	mcl_fp_subPre8L, .Lfunc_end48-mcl_fp_subPre8L
                                        # -- End function
	.globl	mcl_fp_shr1_8L                  # -- Begin function mcl_fp_shr1_8L
	.p2align	4, 0x90
	.type	mcl_fp_shr1_8L,@function
mcl_fp_shr1_8L:                         # @mcl_fp_shr1_8L
# %bb.0:
	pushl	%esi
	movl	12(%esp), %eax
	movl	28(%eax), %ecx
	movl	%ecx, %edx
	shrl	%edx
	movl	8(%esp), %esi
	movl	%edx, 28(%esi)
	movl	24(%eax), %edx
	shldl	$31, %edx, %ecx
	movl	%ecx, 24(%esi)
	movl	20(%eax), %ecx
	shldl	$31, %ecx, %edx
	movl	%edx, 20(%esi)
	movl	16(%eax), %edx
	shldl	$31, %edx, %ecx
	movl	%ecx, 16(%esi)
	movl	12(%eax), %ecx
	shldl	$31, %ecx, %edx
	movl	%edx, 12(%esi)
	movl	8(%eax), %edx
	shldl	$31, %edx, %ecx
	movl	%ecx, 8(%esi)
	movl	4(%eax), %ecx
	shldl	$31, %ecx, %edx
	movl	%edx, 4(%esi)
	movl	(%eax), %eax
	shrdl	$1, %ecx, %eax
	movl	%eax, (%esi)
	popl	%esi
	retl
.Lfunc_end49:
	.size	mcl_fp_shr1_8L, .Lfunc_end49-mcl_fp_shr1_8L
                                        # -- End function
	.globl	mcl_fp_add8L                    # -- Begin function mcl_fp_add8L
	.p2align	4, 0x90
	.type	mcl_fp_add8L,@function
mcl_fp_add8L:                           # @mcl_fp_add8L
# %bb.0:
	pushl	%ebp
	pushl	%ebx
	pushl	%edi
	pushl	%esi
	subl	$32, %esp
	movl	56(%esp), %eax
	movl	(%eax), %ebx
	movl	4(%eax), %ecx
	movl	60(%esp), %edx
	addl	(%edx), %ebx
	adcl	4(%edx), %ecx
	movl	%ecx, 4(%esp)                   # 4-byte Spill
	movl	28(%eax), %ebp
	movl	24(%eax), %ecx
	movl	%ecx, 8(%esp)                   # 4-byte Spill
	movl	20(%eax), %ecx
	movl	16(%eax), %esi
	movl	12(%eax), %edi
	movl	8(%eax), %edx
	movl	60(%esp), %eax
	adcl	8(%eax), %edx
	adcl	12(%eax), %edi
	adcl	16(%eax), %esi
	adcl	20(%eax), %ecx
	movl	%ecx, 12(%esp)                  # 4-byte Spill
	movl	8(%esp), %eax                   # 4-byte Reload
	movl	60(%esp), %ecx
	adcl	24(%ecx), %eax
	movl	%eax, 8(%esp)                   # 4-byte Spill
	adcl	28(%ecx), %ebp
	movl	%ebp, 28(%esp)                  # 4-byte Spill
	movl	52(%esp), %ecx
	movl	%ebp, 28(%ecx)
	movl	%eax, 24(%ecx)
	movl	12(%esp), %eax                  # 4-byte Reload
	movl	%eax, 20(%ecx)
	movl	%esi, 16(%ecx)
	movl	%edi, 12(%ecx)
	movl	%edx, 8(%ecx)
	movl	4(%esp), %eax                   # 4-byte Reload
	movl	%eax, 4(%ecx)
	movl	%ebx, (%ecx)
	setb	3(%esp)                         # 1-byte Folded Spill
	movl	64(%esp), %ebp
	subl	(%ebp), %ebx
	movl	%ebx, 24(%esp)                  # 4-byte Spill
	sbbl	4(%ebp), %eax
	movl	%eax, 4(%esp)                   # 4-byte Spill
	sbbl	8(%ebp), %edx
	movl	%edx, 20(%esp)                  # 4-byte Spill
	sbbl	12(%ebp), %edi
	movl	%edi, 16(%esp)                  # 4-byte Spill
	sbbl	16(%ebp), %esi
	movl	%esi, %edi
	movl	12(%esp), %ecx                  # 4-byte Reload
	sbbl	20(%ebp), %ecx
	movl	8(%esp), %esi                   # 4-byte Reload
	sbbl	24(%ebp), %esi
	movl	28(%esp), %ebx                  # 4-byte Reload
	sbbl	28(%ebp), %ebx
	movzbl	3(%esp), %edx                   # 1-byte Folded Reload
	sbbl	$0, %edx
	testb	$1, %dl
	jne	.LBB50_2
# %bb.1:                                # %nocarry
	movl	24(%esp), %edx                  # 4-byte Reload
	movl	52(%esp), %ebp
	movl	%edx, (%ebp)
	movl	4(%esp), %eax                   # 4-byte Reload
	movl	%eax, 4(%ebp)
	movl	20(%esp), %eax                  # 4-byte Reload
	movl	%eax, 8(%ebp)
	movl	16(%esp), %eax                  # 4-byte Reload
	movl	%eax, 12(%ebp)
	movl	%edi, 16(%ebp)
	movl	%ecx, 20(%ebp)
	movl	%esi, 24(%ebp)
	movl	%ebx, 28(%ebp)
.LBB50_2:                               # %carry
	addl	$32, %esp
	popl	%esi
	popl	%edi
	popl	%ebx
	popl	%ebp
	retl
.Lfunc_end50:
	.size	mcl_fp_add8L, .Lfunc_end50-mcl_fp_add8L
                                        # -- End function
	.globl	mcl_fp_addNF8L                  # -- Begin function mcl_fp_addNF8L
	.p2align	4, 0x90
	.type	mcl_fp_addNF8L,@function
mcl_fp_addNF8L:                         # @mcl_fp_addNF8L
# %bb.0:
	pushl	%ebp
	pushl	%ebx
	pushl	%edi
	pushl	%esi
	subl	$44, %esp
	movl	72(%esp), %eax
	movl	(%eax), %edx
	movl	4(%eax), %ecx
	movl	68(%esp), %esi
	addl	(%esi), %edx
	adcl	4(%esi), %ecx
	movl	%ecx, 8(%esp)                   # 4-byte Spill
	movl	28(%eax), %ecx
	movl	%ecx, 4(%esp)                   # 4-byte Spill
	movl	24(%eax), %ecx
	movl	%ecx, (%esp)                    # 4-byte Spill
	movl	20(%eax), %ecx
	movl	16(%eax), %ebx
	movl	12(%eax), %ebp
	movl	8(%eax), %eax
	adcl	8(%esi), %eax
	adcl	12(%esi), %ebp
	adcl	16(%esi), %ebx
	adcl	20(%esi), %ecx
	movl	(%esp), %esi                    # 4-byte Reload
	movl	68(%esp), %edi
	adcl	24(%edi), %esi
	movl	%esi, (%esp)                    # 4-byte Spill
	movl	4(%esp), %esi                   # 4-byte Reload
	movl	68(%esp), %edi
	adcl	28(%edi), %esi
	movl	%esi, 4(%esp)                   # 4-byte Spill
	movl	76(%esp), %esi
	movl	%edx, 28(%esp)                  # 4-byte Spill
	subl	(%esi), %edx
	movl	%edx, 40(%esp)                  # 4-byte Spill
	movl	8(%esp), %edi                   # 4-byte Reload
	sbbl	4(%esi), %edi
	movl	%edi, 36(%esp)                  # 4-byte Spill
	movl	%eax, 24(%esp)                  # 4-byte Spill
	sbbl	8(%esi), %eax
	movl	%eax, 32(%esp)                  # 4-byte Spill
	movl	%ebp, 20(%esp)                  # 4-byte Spill
	movl	%ebp, %edi
	sbbl	12(%esi), %edi
	movl	%ebx, 16(%esp)                  # 4-byte Spill
	movl	%ebx, %ebp
	movl	4(%esp), %ebx                   # 4-byte Reload
	sbbl	16(%esi), %ebp
	movl	%ecx, 12(%esp)                  # 4-byte Spill
	sbbl	20(%esi), %ecx
	movl	(%esp), %eax                    # 4-byte Reload
	sbbl	24(%esi), %eax
	movl	%ebx, %edx
	sbbl	28(%esi), %edx
	testl	%edx, %edx
	js	.LBB51_1
# %bb.2:
	movl	64(%esp), %ebx
	movl	%edx, 28(%ebx)
	js	.LBB51_3
.LBB51_4:
	movl	%eax, 24(%ebx)
	movl	32(%esp), %edx                  # 4-byte Reload
	js	.LBB51_5
.LBB51_6:
	movl	%ecx, 20(%ebx)
	movl	40(%esp), %eax                  # 4-byte Reload
	js	.LBB51_7
.LBB51_8:
	movl	%ebp, 16(%ebx)
	movl	36(%esp), %ecx                  # 4-byte Reload
	js	.LBB51_9
.LBB51_10:
	movl	%edi, 12(%ebx)
	js	.LBB51_11
.LBB51_12:
	movl	%edx, 8(%ebx)
	js	.LBB51_13
.LBB51_14:
	movl	%ecx, 4(%ebx)
	jns	.LBB51_16
.LBB51_15:
	movl	28(%esp), %eax                  # 4-byte Reload
.LBB51_16:
	movl	%eax, (%ebx)
	addl	$44, %esp
	popl	%esi
	popl	%edi
	popl	%ebx
	popl	%ebp
	retl
.LBB51_1:
	movl	%ebx, %edx
	movl	64(%esp), %ebx
	movl	%edx, 28(%ebx)
	jns	.LBB51_4
.LBB51_3:
	movl	(%esp), %eax                    # 4-byte Reload
	movl	%eax, 24(%ebx)
	movl	32(%esp), %edx                  # 4-byte Reload
	jns	.LBB51_6
.LBB51_5:
	movl	12(%esp), %ecx                  # 4-byte Reload
	movl	%ecx, 20(%ebx)
	movl	40(%esp), %eax                  # 4-byte Reload
	jns	.LBB51_8
.LBB51_7:
	movl	16(%esp), %ebp                  # 4-byte Reload
	movl	%ebp, 16(%ebx)
	movl	36(%esp), %ecx                  # 4-byte Reload
	jns	.LBB51_10
.LBB51_9:
	movl	20(%esp), %edi                  # 4-byte Reload
	movl	%edi, 12(%ebx)
	jns	.LBB51_12
.LBB51_11:
	movl	24(%esp), %edx                  # 4-byte Reload
	movl	%edx, 8(%ebx)
	jns	.LBB51_14
.LBB51_13:
	movl	8(%esp), %ecx                   # 4-byte Reload
	movl	%ecx, 4(%ebx)
	js	.LBB51_15
	jmp	.LBB51_16
.Lfunc_end51:
	.size	mcl_fp_addNF8L, .Lfunc_end51-mcl_fp_addNF8L
                                        # -- End function
	.globl	mcl_fp_sub8L                    # -- Begin function mcl_fp_sub8L
	.p2align	4, 0x90
	.type	mcl_fp_sub8L,@function
mcl_fp_sub8L:                           # @mcl_fp_sub8L
# %bb.0:
	pushl	%ebp
	pushl	%ebx
	pushl	%edi
	pushl	%esi
	subl	$28, %esp
	movl	52(%esp), %edx
	movl	(%edx), %esi
	movl	4(%edx), %edi
	movl	56(%esp), %eax
	subl	(%eax), %esi
	movl	%esi, 4(%esp)                   # 4-byte Spill
	sbbl	4(%eax), %edi
	movl	%edi, (%esp)                    # 4-byte Spill
	movl	28(%edx), %ecx
	movl	24(%edx), %edi
	movl	20(%edx), %esi
	movl	16(%edx), %ebp
	movl	12(%edx), %ebx
	movl	8(%edx), %edx
	sbbl	8(%eax), %edx
	sbbl	12(%eax), %ebx
	sbbl	16(%eax), %ebp
	sbbl	20(%eax), %esi
	sbbl	24(%eax), %edi
	sbbl	28(%eax), %ecx
	movl	%ecx, %eax
	movl	$0, %ecx
	sbbl	%ecx, %ecx
	testb	$1, %cl
	movl	48(%esp), %ecx
	movl	%eax, 12(%esp)                  # 4-byte Spill
	movl	%eax, 28(%ecx)
	movl	%edi, 16(%esp)                  # 4-byte Spill
	movl	%edi, 24(%ecx)
	movl	%esi, 20(%esp)                  # 4-byte Spill
	movl	%esi, 20(%ecx)
	movl	%ebp, 24(%esp)                  # 4-byte Spill
	movl	%ebp, 16(%ecx)
	movl	%ebx, 8(%esp)                   # 4-byte Spill
	movl	%ebx, 12(%ecx)
	movl	%edx, 8(%ecx)
	movl	(%esp), %eax                    # 4-byte Reload
	movl	%eax, 4(%ecx)
	movl	4(%esp), %eax                   # 4-byte Reload
	movl	%eax, (%ecx)
	je	.LBB52_2
# %bb.1:                                # %carry
	movl	60(%esp), %edi
	movl	4(%esp), %eax                   # 4-byte Reload
	addl	(%edi), %eax
	movl	%eax, 4(%esp)                   # 4-byte Spill
	movl	(%esp), %eax                    # 4-byte Reload
	adcl	4(%edi), %eax
	movl	%eax, (%esp)                    # 4-byte Spill
	adcl	8(%edi), %edx
	movl	8(%esp), %eax                   # 4-byte Reload
	adcl	12(%edi), %eax
	movl	%eax, 8(%esp)                   # 4-byte Spill
	movl	24(%esp), %ebp                  # 4-byte Reload
	adcl	16(%edi), %ebp
	movl	20(%esp), %esi                  # 4-byte Reload
	adcl	20(%edi), %esi
	movl	16(%esp), %eax                  # 4-byte Reload
	adcl	24(%edi), %eax
	movl	12(%esp), %ebx                  # 4-byte Reload
	adcl	28(%edi), %ebx
	movl	%ebx, 28(%ecx)
	movl	%eax, 24(%ecx)
	movl	%esi, 20(%ecx)
	movl	%ebp, 16(%ecx)
	movl	8(%esp), %eax                   # 4-byte Reload
	movl	%eax, 12(%ecx)
	movl	%edx, 8(%ecx)
	movl	(%esp), %eax                    # 4-byte Reload
	movl	%eax, 4(%ecx)
	movl	4(%esp), %eax                   # 4-byte Reload
	movl	%eax, (%ecx)
.LBB52_2:                               # %nocarry
	addl	$28, %esp
	popl	%esi
	popl	%edi
	popl	%ebx
	popl	%ebp
	retl
.Lfunc_end52:
	.size	mcl_fp_sub8L, .Lfunc_end52-mcl_fp_sub8L
                                        # -- End function
	.globl	mcl_fp_subNF8L                  # -- Begin function mcl_fp_subNF8L
	.p2align	4, 0x90
	.type	mcl_fp_subNF8L,@function
mcl_fp_subNF8L:                         # @mcl_fp_subNF8L
# %bb.0:
	pushl	%ebp
	pushl	%ebx
	pushl	%edi
	pushl	%esi
	subl	$40, %esp
	movl	64(%esp), %eax
	movl	(%eax), %esi
	movl	4(%eax), %edx
	movl	68(%esp), %ecx
	subl	(%ecx), %esi
	movl	%esi, 24(%esp)                  # 4-byte Spill
	sbbl	4(%ecx), %edx
	movl	%edx, 28(%esp)                  # 4-byte Spill
	movl	28(%eax), %edx
	movl	24(%eax), %esi
	movl	20(%eax), %edi
	movl	16(%eax), %ebx
	movl	12(%eax), %ebp
	movl	8(%eax), %eax
	sbbl	8(%ecx), %eax
	movl	%eax, 8(%esp)                   # 4-byte Spill
	sbbl	12(%ecx), %ebp
	movl	%ebp, 12(%esp)                  # 4-byte Spill
	sbbl	16(%ecx), %ebx
	movl	%ebx, 16(%esp)                  # 4-byte Spill
	sbbl	20(%ecx), %edi
	movl	%edi, 20(%esp)                  # 4-byte Spill
	sbbl	24(%ecx), %esi
	movl	%esi, 32(%esp)                  # 4-byte Spill
	movl	%edx, %edi
	sbbl	28(%ecx), %edi
	movl	%edi, 36(%esp)                  # 4-byte Spill
	sarl	$31, %edi
	movl	72(%esp), %ebp
	movl	28(%ebp), %eax
	andl	%edi, %eax
	movl	%eax, 4(%esp)                   # 4-byte Spill
	movl	24(%ebp), %eax
	andl	%edi, %eax
	movl	%eax, (%esp)                    # 4-byte Spill
	movl	20(%ebp), %ebx
	andl	%edi, %ebx
	movl	16(%ebp), %esi
	andl	%edi, %esi
	movl	12(%ebp), %edx
	andl	%edi, %edx
	movl	8(%ebp), %ecx
	andl	%edi, %ecx
	movl	4(%ebp), %eax
	andl	%edi, %eax
	andl	(%ebp), %edi
	addl	24(%esp), %edi                  # 4-byte Folded Reload
	adcl	28(%esp), %eax                  # 4-byte Folded Reload
	movl	60(%esp), %ebp
	movl	%edi, (%ebp)
	adcl	8(%esp), %ecx                   # 4-byte Folded Reload
	movl	%eax, 4(%ebp)
	adcl	12(%esp), %edx                  # 4-byte Folded Reload
	movl	%ecx, 8(%ebp)
	adcl	16(%esp), %esi                  # 4-byte Folded Reload
	movl	%edx, 12(%ebp)
	adcl	20(%esp), %ebx                  # 4-byte Folded Reload
	movl	%esi, 16(%ebp)
	movl	(%esp), %eax                    # 4-byte Reload
	adcl	32(%esp), %eax                  # 4-byte Folded Reload
	movl	%ebx, 20(%ebp)
	movl	%eax, 24(%ebp)
	movl	4(%esp), %eax                   # 4-byte Reload
	adcl	36(%esp), %eax                  # 4-byte Folded Reload
	movl	%eax, 28(%ebp)
	addl	$40, %esp
	popl	%esi
	popl	%edi
	popl	%ebx
	popl	%ebp
	retl
.Lfunc_end53:
	.size	mcl_fp_subNF8L, .Lfunc_end53-mcl_fp_subNF8L
                                        # -- End function
	.globl	mcl_fpDbl_add8L                 # -- Begin function mcl_fpDbl_add8L
	.p2align	4, 0x90
	.type	mcl_fpDbl_add8L,@function
mcl_fpDbl_add8L:                        # @mcl_fpDbl_add8L
# %bb.0:
	pushl	%ebp
	pushl	%ebx
	pushl	%edi
	pushl	%esi
	subl	$52, %esp
	movl	76(%esp), %eax
	movl	(%eax), %ecx
	movl	4(%eax), %edx
	movl	80(%esp), %edi
	addl	(%edi), %ecx
	movl	%ecx, 44(%esp)                  # 4-byte Spill
	adcl	4(%edi), %edx
	movl	%edx, 40(%esp)                  # 4-byte Spill
	movl	60(%eax), %ecx
	movl	%ecx, 8(%esp)                   # 4-byte Spill
	movl	56(%eax), %ecx
	movl	%ecx, 12(%esp)                  # 4-byte Spill
	movl	52(%eax), %ebp
	movl	48(%eax), %ecx
	movl	%ecx, 16(%esp)                  # 4-byte Spill
	movl	44(%eax), %ecx
	movl	%ecx, 20(%esp)                  # 4-byte Spill
	movl	40(%eax), %ecx
	movl	%ecx, 24(%esp)                  # 4-byte Spill
	movl	36(%eax), %ecx
	movl	%ecx, 48(%esp)                  # 4-byte Spill
	movl	32(%eax), %ebx
	movl	28(%eax), %ecx
	movl	%ecx, 4(%esp)                   # 4-byte Spill
	movl	24(%eax), %esi
	movl	20(%eax), %ecx
	movl	%ecx, (%esp)                    # 4-byte Spill
	movl	16(%eax), %edx
	movl	12(%eax), %ecx
	movl	8(%eax), %eax
	adcl	8(%edi), %eax
	movl	%eax, 36(%esp)                  # 4-byte Spill
	adcl	12(%edi), %ecx
	movl	%ecx, 32(%esp)                  # 4-byte Spill
	adcl	16(%edi), %edx
	movl	%edx, 28(%esp)                  # 4-byte Spill
	movl	(%esp), %edx                    # 4-byte Reload
	adcl	20(%edi), %edx
	adcl	24(%edi), %esi
	movl	%esi, (%esp)                    # 4-byte Spill
	movl	4(%esp), %ecx                   # 4-byte Reload
	adcl	28(%edi), %ecx
	adcl	32(%edi), %ebx
	movl	48(%esp), %esi                  # 4-byte Reload
	adcl	36(%edi), %esi
	movl	24(%esp), %eax                  # 4-byte Reload
	adcl	40(%edi), %eax
	movl	%eax, 24(%esp)                  # 4-byte Spill
	movl	20(%esp), %eax                  # 4-byte Reload
	adcl	44(%edi), %eax
	movl	%eax, 20(%esp)                  # 4-byte Spill
	movl	16(%esp), %eax                  # 4-byte Reload
	adcl	48(%edi), %eax
	movl	%eax, 16(%esp)                  # 4-byte Spill
	adcl	52(%edi), %ebp
	movl	%ebp, 4(%esp)                   # 4-byte Spill
	movl	12(%esp), %ebp                  # 4-byte Reload
	adcl	56(%edi), %ebp
	movl	%ebp, 12(%esp)                  # 4-byte Spill
	movl	8(%esp), %eax                   # 4-byte Reload
	adcl	60(%edi), %eax
	movl	%eax, 8(%esp)                   # 4-byte Spill
	movl	72(%esp), %ebp
	movl	%ecx, 28(%ebp)
	movl	(%esp), %eax                    # 4-byte Reload
	movl	%eax, 24(%ebp)
	movl	%edx, 20(%ebp)
	movl	28(%esp), %eax                  # 4-byte Reload
	movl	%eax, 16(%ebp)
	movl	32(%esp), %eax                  # 4-byte Reload
	movl	%eax, 12(%ebp)
	movl	36(%esp), %eax                  # 4-byte Reload
	movl	%eax, 8(%ebp)
	movl	40(%esp), %eax                  # 4-byte Reload
	movl	%eax, 4(%ebp)
	movl	44(%esp), %eax                  # 4-byte Reload
	movl	%eax, (%ebp)
	setb	28(%esp)                        # 1-byte Folded Spill
	movl	84(%esp), %edi
	movl	%ebx, (%esp)                    # 4-byte Spill
	subl	(%edi), %ebx
	movl	%ebx, 44(%esp)                  # 4-byte Spill
	movl	%esi, %edx
	movl	%esi, 48(%esp)                  # 4-byte Spill
	sbbl	4(%edi), %edx
	movl	%edx, 40(%esp)                  # 4-byte Spill
	movl	24(%esp), %eax                  # 4-byte Reload
	sbbl	8(%edi), %eax
	movl	%eax, 36(%esp)                  # 4-byte Spill
	movl	20(%esp), %eax                  # 4-byte Reload
	sbbl	12(%edi), %eax
	movl	%eax, 32(%esp)                  # 4-byte Spill
	movl	16(%esp), %eax                  # 4-byte Reload
	sbbl	16(%edi), %eax
	movl	4(%esp), %ecx                   # 4-byte Reload
	sbbl	20(%edi), %ecx
	movl	12(%esp), %edx                  # 4-byte Reload
	sbbl	24(%edi), %edx
	movl	8(%esp), %esi                   # 4-byte Reload
	sbbl	28(%edi), %esi
	movzbl	28(%esp), %ebx                  # 1-byte Folded Reload
	sbbl	$0, %ebx
	testb	$1, %bl
	jne	.LBB54_1
# %bb.2:
	movl	%esi, 60(%ebp)
	jne	.LBB54_3
.LBB54_4:
	movl	%edx, 56(%ebp)
	jne	.LBB54_5
.LBB54_6:
	movl	%ecx, 52(%ebp)
	movl	36(%esp), %edx                  # 4-byte Reload
	jne	.LBB54_7
.LBB54_8:
	movl	%eax, 48(%ebp)
	movl	40(%esp), %ecx                  # 4-byte Reload
	movl	32(%esp), %eax                  # 4-byte Reload
	jne	.LBB54_9
.LBB54_10:
	movl	%eax, 44(%ebp)
	movl	44(%esp), %eax                  # 4-byte Reload
	jne	.LBB54_11
.LBB54_12:
	movl	%edx, 40(%ebp)
	jne	.LBB54_13
.LBB54_14:
	movl	%ecx, 36(%ebp)
	je	.LBB54_16
.LBB54_15:
	movl	(%esp), %eax                    # 4-byte Reload
.LBB54_16:
	movl	%eax, 32(%ebp)
	addl	$52, %esp
	popl	%esi
	popl	%edi
	popl	%ebx
	popl	%ebp
	retl
.LBB54_1:
	movl	8(%esp), %esi                   # 4-byte Reload
	movl	%esi, 60(%ebp)
	je	.LBB54_4
.LBB54_3:
	movl	12(%esp), %edx                  # 4-byte Reload
	movl	%edx, 56(%ebp)
	je	.LBB54_6
.LBB54_5:
	movl	4(%esp), %ecx                   # 4-byte Reload
	movl	%ecx, 52(%ebp)
	movl	36(%esp), %edx                  # 4-byte Reload
	je	.LBB54_8
.LBB54_7:
	movl	16(%esp), %eax                  # 4-byte Reload
	movl	%eax, 48(%ebp)
	movl	40(%esp), %ecx                  # 4-byte Reload
	movl	32(%esp), %eax                  # 4-byte Reload
	je	.LBB54_10
.LBB54_9:
	movl	20(%esp), %eax                  # 4-byte Reload
	movl	%eax, 44(%ebp)
	movl	44(%esp), %eax                  # 4-byte Reload
	je	.LBB54_12
.LBB54_11:
	movl	24(%esp), %edx                  # 4-byte Reload
	movl	%edx, 40(%ebp)
	je	.LBB54_14
.LBB54_13:
	movl	48(%esp), %ecx                  # 4-byte Reload
	movl	%ecx, 36(%ebp)
	jne	.LBB54_15
	jmp	.LBB54_16
.Lfunc_end54:
	.size	mcl_fpDbl_add8L, .Lfunc_end54-mcl_fpDbl_add8L
                                        # -- End function
	.globl	mcl_fpDbl_sub8L                 # -- Begin function mcl_fpDbl_sub8L
	.p2align	4, 0x90
	.type	mcl_fpDbl_sub8L,@function
mcl_fpDbl_sub8L:                        # @mcl_fpDbl_sub8L
# %bb.0:
	pushl	%ebp
	pushl	%ebx
	pushl	%edi
	pushl	%esi
	subl	$56, %esp
	movl	80(%esp), %ecx
	movl	(%ecx), %edx
	movl	4(%ecx), %edi
	xorl	%esi, %esi
	movl	84(%esp), %ebp
	subl	(%ebp), %edx
	movl	%edx, 4(%esp)                   # 4-byte Spill
	sbbl	4(%ebp), %edi
	movl	%edi, (%esp)                    # 4-byte Spill
	movl	60(%ecx), %edx
	movl	%edx, 32(%esp)                  # 4-byte Spill
	movl	56(%ecx), %edx
	movl	%edx, 36(%esp)                  # 4-byte Spill
	movl	52(%ecx), %ebx
	movl	48(%ecx), %edx
	movl	%edx, 28(%esp)                  # 4-byte Spill
	movl	44(%ecx), %edx
	movl	%edx, 24(%esp)                  # 4-byte Spill
	movl	40(%ecx), %edx
	movl	%edx, 20(%esp)                  # 4-byte Spill
	movl	36(%ecx), %edx
	movl	%edx, 16(%esp)                  # 4-byte Spill
	movl	32(%ecx), %edx
	movl	%edx, 12(%esp)                  # 4-byte Spill
	movl	28(%ecx), %eax
	movl	%eax, 8(%esp)                   # 4-byte Spill
	movl	24(%ecx), %edx
	movl	20(%ecx), %eax
	movl	%eax, 40(%esp)                  # 4-byte Spill
	movl	16(%ecx), %edi
	movl	12(%ecx), %eax
	movl	8(%ecx), %ecx
	sbbl	8(%ebp), %ecx
	movl	%ecx, 52(%esp)                  # 4-byte Spill
	sbbl	12(%ebp), %eax
	movl	%eax, 48(%esp)                  # 4-byte Spill
	sbbl	16(%ebp), %edi
	movl	%edi, 44(%esp)                  # 4-byte Spill
	movl	40(%esp), %edi                  # 4-byte Reload
	sbbl	20(%ebp), %edi
	sbbl	24(%ebp), %edx
	movl	8(%esp), %eax                   # 4-byte Reload
	sbbl	28(%ebp), %eax
	movl	12(%esp), %ecx                  # 4-byte Reload
	sbbl	32(%ebp), %ecx
	movl	%ecx, 12(%esp)                  # 4-byte Spill
	movl	16(%esp), %ecx                  # 4-byte Reload
	sbbl	36(%ebp), %ecx
	movl	%ecx, 16(%esp)                  # 4-byte Spill
	movl	20(%esp), %ecx                  # 4-byte Reload
	sbbl	40(%ebp), %ecx
	movl	%ecx, 20(%esp)                  # 4-byte Spill
	movl	24(%esp), %ecx                  # 4-byte Reload
	sbbl	44(%ebp), %ecx
	movl	%ecx, 24(%esp)                  # 4-byte Spill
	movl	28(%esp), %ecx                  # 4-byte Reload
	sbbl	48(%ebp), %ecx
	movl	%ecx, 28(%esp)                  # 4-byte Spill
	sbbl	52(%ebp), %ebx
	movl	%ebx, 8(%esp)                   # 4-byte Spill
	movl	36(%esp), %ebx                  # 4-byte Reload
	sbbl	56(%ebp), %ebx
	movl	%ebx, 36(%esp)                  # 4-byte Spill
	movl	32(%esp), %ebx                  # 4-byte Reload
	sbbl	60(%ebp), %ebx
	movl	%ebx, 32(%esp)                  # 4-byte Spill
	movl	76(%esp), %ecx
	movl	%eax, 28(%ecx)
	movl	%edx, 24(%ecx)
	movl	%edi, 20(%ecx)
	movl	44(%esp), %eax                  # 4-byte Reload
	movl	%eax, 16(%ecx)
	movl	48(%esp), %eax                  # 4-byte Reload
	movl	%eax, 12(%ecx)
	movl	52(%esp), %eax                  # 4-byte Reload
	movl	%eax, 8(%ecx)
	movl	(%esp), %eax                    # 4-byte Reload
	movl	%eax, 4(%ecx)
	movl	4(%esp), %eax                   # 4-byte Reload
	movl	%eax, (%ecx)
	sbbl	%esi, %esi
	andl	$1, %esi
	negl	%esi
	movl	88(%esp), %ecx
	movl	28(%ecx), %eax
	andl	%esi, %eax
	movl	%eax, 4(%esp)                   # 4-byte Spill
	movl	24(%ecx), %eax
	andl	%esi, %eax
	movl	%eax, (%esp)                    # 4-byte Spill
	movl	20(%ecx), %edi
	andl	%esi, %edi
	movl	16(%ecx), %ebp
	andl	%esi, %ebp
	movl	12(%ecx), %edx
	andl	%esi, %edx
	movl	8(%ecx), %ecx
	andl	%esi, %ecx
	movl	88(%esp), %eax
	movl	4(%eax), %eax
	andl	%esi, %eax
	movl	88(%esp), %ebx
	andl	(%ebx), %esi
	addl	12(%esp), %esi                  # 4-byte Folded Reload
	adcl	16(%esp), %eax                  # 4-byte Folded Reload
	movl	76(%esp), %ebx
	movl	%esi, 32(%ebx)
	adcl	20(%esp), %ecx                  # 4-byte Folded Reload
	movl	%eax, 36(%ebx)
	adcl	24(%esp), %edx                  # 4-byte Folded Reload
	movl	%ecx, 40(%ebx)
	adcl	28(%esp), %ebp                  # 4-byte Folded Reload
	movl	%edx, 44(%ebx)
	adcl	8(%esp), %edi                   # 4-byte Folded Reload
	movl	%ebp, 48(%ebx)
	movl	(%esp), %eax                    # 4-byte Reload
	adcl	36(%esp), %eax                  # 4-byte Folded Reload
	movl	%edi, 52(%ebx)
	movl	%eax, 56(%ebx)
	movl	4(%esp), %eax                   # 4-byte Reload
	adcl	32(%esp), %eax                  # 4-byte Folded Reload
	movl	%eax, 60(%ebx)
	addl	$56, %esp
	popl	%esi
	popl	%edi
	popl	%ebx
	popl	%ebp
	retl
.Lfunc_end55:
	.size	mcl_fpDbl_sub8L, .Lfunc_end55-mcl_fpDbl_sub8L
                                        # -- End function
	.globl	mulPv384x32                     # -- Begin function mulPv384x32
	.p2align	4, 0x90
	.type	mulPv384x32,@function
mulPv384x32:                            # @mulPv384x32
# %bb.0:
	pushl	%ebp
	pushl	%ebx
	pushl	%edi
	pushl	%esi
	subl	$76, %esp
	movl	104(%esp), %edi
	movl	100(%esp), %ecx
	movl	%edi, %eax
	mull	44(%ecx)
	movl	%edx, 72(%esp)                  # 4-byte Spill
	movl	%eax, 68(%esp)                  # 4-byte Spill
	movl	%edi, %eax
	mull	40(%ecx)
	movl	%edx, 64(%esp)                  # 4-byte Spill
	movl	%eax, 60(%esp)                  # 4-byte Spill
	movl	%edi, %eax
	mull	36(%ecx)
	movl	%edx, 56(%esp)                  # 4-byte Spill
	movl	%eax, 52(%esp)                  # 4-byte Spill
	movl	%edi, %eax
	mull	32(%ecx)
	movl	%edx, 48(%esp)                  # 4-byte Spill
	movl	%eax, 44(%esp)                  # 4-byte Spill
	movl	%edi, %eax
	mull	28(%ecx)
	movl	%edx, 40(%esp)                  # 4-byte Spill
	movl	%eax, 36(%esp)                  # 4-byte Spill
	movl	%edi, %eax
	mull	24(%ecx)
	movl	%edx, 32(%esp)                  # 4-byte Spill
	movl	%eax, 28(%esp)                  # 4-byte Spill
	movl	%edi, %eax
	mull	20(%ecx)
	movl	%edx, 24(%esp)                  # 4-byte Spill
	movl	%eax, 20(%esp)                  # 4-byte Spill
	movl	%edi, %eax
	mull	16(%ecx)
	movl	%edx, 16(%esp)                  # 4-byte Spill
	movl	%eax, 12(%esp)                  # 4-byte Spill
	movl	%edi, %eax
	mull	12(%ecx)
	movl	%edx, %ebx
	movl	%eax, 8(%esp)                   # 4-byte Spill
	movl	%edi, %eax
	mull	8(%ecx)
	movl	%edx, %esi
	movl	%eax, 4(%esp)                   # 4-byte Spill
	movl	%edi, %eax
	mull	4(%ecx)
	movl	%edx, %ebp
	movl	%eax, (%esp)                    # 4-byte Spill
	movl	%edi, %eax
	mull	(%ecx)
	movl	%eax, %edi
	movl	96(%esp), %eax
	movl	%edi, (%eax)
	addl	(%esp), %edx                    # 4-byte Folded Reload
	movl	%edx, 4(%eax)
	adcl	4(%esp), %ebp                   # 4-byte Folded Reload
	movl	%ebp, 8(%eax)
	adcl	8(%esp), %esi                   # 4-byte Folded Reload
	movl	%esi, 12(%eax)
	adcl	12(%esp), %ebx                  # 4-byte Folded Reload
	movl	%ebx, 16(%eax)
	movl	16(%esp), %ecx                  # 4-byte Reload
	adcl	20(%esp), %ecx                  # 4-byte Folded Reload
	movl	%ecx, 20(%eax)
	movl	24(%esp), %ecx                  # 4-byte Reload
	adcl	28(%esp), %ecx                  # 4-byte Folded Reload
	movl	%ecx, 24(%eax)
	movl	32(%esp), %ecx                  # 4-byte Reload
	adcl	36(%esp), %ecx                  # 4-byte Folded Reload
	movl	%ecx, 28(%eax)
	movl	40(%esp), %ecx                  # 4-byte Reload
	adcl	44(%esp), %ecx                  # 4-byte Folded Reload
	movl	%ecx, 32(%eax)
	movl	48(%esp), %ecx                  # 4-byte Reload
	adcl	52(%esp), %ecx                  # 4-byte Folded Reload
	movl	%ecx, 36(%eax)
	movl	56(%esp), %ecx                  # 4-byte Reload
	adcl	60(%esp), %ecx                  # 4-byte Folded Reload
	movl	%ecx, 40(%eax)
	movl	64(%esp), %ecx                  # 4-byte Reload
	adcl	68(%esp), %ecx                  # 4-byte Folded Reload
	movl	%ecx, 44(%eax)
	movl	72(%esp), %ecx                  # 4-byte Reload
	adcl	$0, %ecx
	movl	%ecx, 48(%eax)
	addl	$76, %esp
	popl	%esi
	popl	%edi
	popl	%ebx
	popl	%ebp
	retl	$4
.Lfunc_end56:
	.size	mulPv384x32, .Lfunc_end56-mulPv384x32
                                        # -- End function
	.globl	mcl_fp_mulUnitPre12L            # -- Begin function mcl_fp_mulUnitPre12L
	.p2align	4, 0x90
	.type	mcl_fp_mulUnitPre12L,@function
mcl_fp_mulUnitPre12L:                   # @mcl_fp_mulUnitPre12L
# %bb.0:
	pushl	%ebp
	pushl	%ebx
	pushl	%edi
	pushl	%esi
	subl	$92, %esp
	calll	.L57$pb
.L57$pb:
	popl	%ebx
.Ltmp9:
	addl	$_GLOBAL_OFFSET_TABLE_+(.Ltmp9-.L57$pb), %ebx
	subl	$4, %esp
	movl	124(%esp), %eax
	movl	120(%esp), %ecx
	leal	44(%esp), %edx
	pushl	%eax
	pushl	%ecx
	pushl	%edx
	calll	mulPv384x32@PLT
	addl	$12, %esp
	movl	40(%esp), %eax
	movl	%eax, 36(%esp)                  # 4-byte Spill
	movl	44(%esp), %eax
	movl	%eax, 32(%esp)                  # 4-byte Spill
	movl	48(%esp), %eax
	movl	%eax, 28(%esp)                  # 4-byte Spill
	movl	52(%esp), %eax
	movl	%eax, 24(%esp)                  # 4-byte Spill
	movl	56(%esp), %eax
	movl	%eax, 20(%esp)                  # 4-byte Spill
	movl	60(%esp), %eax
	movl	%eax, 16(%esp)                  # 4-byte Spill
	movl	64(%esp), %eax
	movl	%eax, 12(%esp)                  # 4-byte Spill
	movl	68(%esp), %ebp
	movl	72(%esp), %ebx
	movl	76(%esp), %edi
	movl	80(%esp), %esi
	movl	84(%esp), %edx
	movl	88(%esp), %ecx
	movl	112(%esp), %eax
	movl	%ecx, 48(%eax)
	movl	%edx, 44(%eax)
	movl	%esi, 40(%eax)
	movl	%edi, 36(%eax)
	movl	%ebx, 32(%eax)
	movl	%ebp, 28(%eax)
	movl	12(%esp), %ecx                  # 4-byte Reload
	movl	%ecx, 24(%eax)
	movl	16(%esp), %ecx                  # 4-byte Reload
	movl	%ecx, 20(%eax)
	movl	20(%esp), %ecx                  # 4-byte Reload
	movl	%ecx, 16(%eax)
	movl	24(%esp), %ecx                  # 4-byte Reload
	movl	%ecx, 12(%eax)
	movl	28(%esp), %ecx                  # 4-byte Reload
	movl	%ecx, 8(%eax)
	movl	32(%esp), %ecx                  # 4-byte Reload
	movl	%ecx, 4(%eax)
	movl	36(%esp), %ecx                  # 4-byte Reload
	movl	%ecx, (%eax)
	addl	$92, %esp
	popl	%esi
	popl	%edi
	popl	%ebx
	popl	%ebp
	retl
.Lfunc_end57:
	.size	mcl_fp_mulUnitPre12L, .Lfunc_end57-mcl_fp_mulUnitPre12L
                                        # -- End function
	.globl	mcl_fpDbl_mulPre12L             # -- Begin function mcl_fpDbl_mulPre12L
	.p2align	4, 0x90
	.type	mcl_fpDbl_mulPre12L,@function
mcl_fpDbl_mulPre12L:                    # @mcl_fpDbl_mulPre12L
# %bb.0:
	pushl	%ebp
	pushl	%ebx
	pushl	%edi
	pushl	%esi
	subl	$220, %esp
	calll	.L58$pb
.L58$pb:
	popl	%edi
.Ltmp10:
	addl	$_GLOBAL_OFFSET_TABLE_+(.Ltmp10-.L58$pb), %edi
	subl	$4, %esp
	movl	252(%esp), %ebp
	movl	248(%esp), %esi
	movl	%edi, %ebx
	movl	%edi, 68(%esp)                  # 4-byte Spill
	pushl	%ebp
	pushl	%esi
	pushl	252(%esp)
	calll	mcl_fpDbl_mulPre6L@PLT
	addl	$12, %esp
	leal	24(%ebp), %eax
	leal	24(%esi), %ecx
	movl	244(%esp), %edx
	addl	$48, %edx
	pushl	%eax
	pushl	%ecx
	pushl	%edx
	calll	mcl_fpDbl_mulPre6L@PLT
	addl	$16, %esp
	movl	40(%esi), %edx
	movl	36(%esi), %ebx
	movl	32(%esi), %eax
	movl	24(%esi), %edi
	movl	28(%esi), %ecx
	addl	(%esi), %edi
	movl	%edi, 12(%esp)                  # 4-byte Spill
	adcl	4(%esi), %ecx
	movl	%ecx, 20(%esp)                  # 4-byte Spill
	adcl	8(%esi), %eax
	movl	%eax, 24(%esp)                  # 4-byte Spill
	adcl	12(%esi), %ebx
	movl	%ebx, %edi
	movl	%ebx, 16(%esp)                  # 4-byte Spill
	adcl	16(%esi), %edx
	movl	%edx, %ecx
	movl	%edx, 40(%esp)                  # 4-byte Spill
	movl	44(%esi), %eax
	adcl	20(%esi), %eax
	movl	%eax, 44(%esp)                  # 4-byte Spill
	setb	48(%esp)                        # 1-byte Folded Spill
	movl	24(%ebp), %ebx
	addl	(%ebp), %ebx
	movl	%ebx, 56(%esp)                  # 4-byte Spill
	movl	28(%ebp), %edx
	adcl	4(%ebp), %edx
	movl	%edx, 28(%esp)                  # 4-byte Spill
	movl	32(%ebp), %edx
	adcl	8(%ebp), %edx
	movl	%edx, 32(%esp)                  # 4-byte Spill
	movl	36(%ebp), %edx
	adcl	12(%ebp), %edx
	movl	%edx, 36(%esp)                  # 4-byte Spill
	movl	40(%ebp), %edx
	adcl	16(%ebp), %edx
	movl	%edx, %esi
	movl	%edx, 52(%esp)                  # 4-byte Spill
	movl	44(%ebp), %edx
	adcl	20(%ebp), %edx
	movl	%eax, 168(%esp)
	movl	%ecx, 164(%esp)
	movl	%edi, 160(%esp)
	movl	24(%esp), %ecx                  # 4-byte Reload
	movl	%ecx, 156(%esp)
	movl	20(%esp), %eax                  # 4-byte Reload
	movl	%eax, 152(%esp)
	movl	12(%esp), %ecx                  # 4-byte Reload
	movl	%ecx, 148(%esp)
	movl	%edx, 144(%esp)
	movl	%esi, 140(%esp)
	movl	36(%esp), %edi                  # 4-byte Reload
	movl	%edi, 136(%esp)
	movl	32(%esp), %ebp                  # 4-byte Reload
	movl	%ebp, 132(%esp)
	movl	28(%esp), %eax                  # 4-byte Reload
	movl	%eax, 128(%esp)
	movl	%ebx, 124(%esp)
	setb	60(%esp)                        # 1-byte Folded Spill
	subl	$4, %esp
	movzbl	52(%esp), %ecx                  # 1-byte Folded Reload
	movl	%ecx, %esi
	negl	%esi
	movl	%ecx, %ebx
	shll	$31, %ebx
	shrdl	$31, %esi, %ebx
	andl	60(%esp), %ebx                  # 4-byte Folded Reload
	andl	%esi, 56(%esp)                  # 4-byte Folded Spill
	andl	%esi, %edi
	movl	%edi, 40(%esp)                  # 4-byte Spill
	andl	%esi, %ebp
	movl	%ebp, 36(%esp)                  # 4-byte Spill
	andl	%esi, %eax
	movl	%eax, 32(%esp)                  # 4-byte Spill
	andl	%edx, %esi
	movzbl	64(%esp), %ebp                  # 1-byte Folded Reload
	andl	%ebp, %ecx
	movl	%ecx, 52(%esp)                  # 4-byte Spill
	movl	%ebp, %edx
	negl	%edx
	andl	%edx, 48(%esp)                  # 4-byte Folded Spill
	andl	%edx, 44(%esp)                  # 4-byte Folded Spill
	movl	20(%esp), %edi                  # 4-byte Reload
	andl	%edx, %edi
	movl	28(%esp), %ecx                  # 4-byte Reload
	andl	%edx, %ecx
	movl	24(%esp), %eax                  # 4-byte Reload
	andl	%edx, %eax
	shll	$31, %ebp
	shrdl	$31, %edx, %ebp
	andl	16(%esp), %ebp                  # 4-byte Folded Reload
	addl	%ebx, %ebp
	movl	%ebp, 16(%esp)                  # 4-byte Spill
	adcl	32(%esp), %eax                  # 4-byte Folded Reload
	movl	%eax, 24(%esp)                  # 4-byte Spill
	adcl	36(%esp), %ecx                  # 4-byte Folded Reload
	movl	%ecx, 28(%esp)                  # 4-byte Spill
	leal	176(%esp), %ecx
	adcl	40(%esp), %edi                  # 4-byte Folded Reload
	movl	%edi, 20(%esp)                  # 4-byte Spill
	movl	44(%esp), %edi                  # 4-byte Reload
	adcl	56(%esp), %edi                  # 4-byte Folded Reload
	leal	128(%esp), %edx
	movl	48(%esp), %ebp                  # 4-byte Reload
	adcl	%esi, %ebp
	setb	%al
	movzbl	%al, %esi
	movl	68(%esp), %ebx                  # 4-byte Reload
	pushl	%edx
	leal	156(%esp), %eax
	pushl	%eax
	pushl	%ecx
	calll	mcl_fpDbl_mulPre6L@PLT
	addl	$16, %esp
	movl	12(%esp), %eax                  # 4-byte Reload
	addl	196(%esp), %eax
	movl	%eax, 12(%esp)                  # 4-byte Spill
	movl	20(%esp), %eax                  # 4-byte Reload
	adcl	200(%esp), %eax
	movl	%eax, 20(%esp)                  # 4-byte Spill
	movl	24(%esp), %eax                  # 4-byte Reload
	adcl	204(%esp), %eax
	movl	%eax, 24(%esp)                  # 4-byte Spill
	movl	16(%esp), %eax                  # 4-byte Reload
	adcl	208(%esp), %eax
	movl	%eax, 16(%esp)                  # 4-byte Spill
	adcl	212(%esp), %edi
	movl	%edi, 40(%esp)                  # 4-byte Spill
	adcl	216(%esp), %ebp
	movl	%ebp, 44(%esp)                  # 4-byte Spill
	movl	%esi, %ebp
	adcl	48(%esp), %ebp                  # 4-byte Folded Reload
	movl	172(%esp), %eax
	movl	240(%esp), %edi
	subl	(%edi), %eax
	movl	%eax, 32(%esp)                  # 4-byte Spill
	movl	176(%esp), %eax
	sbbl	4(%edi), %eax
	movl	%eax, 28(%esp)                  # 4-byte Spill
	movl	180(%esp), %eax
	sbbl	8(%edi), %eax
	movl	%eax, %ecx
	movl	184(%esp), %esi
	sbbl	12(%edi), %esi
	movl	188(%esp), %ebx
	sbbl	16(%edi), %ebx
	movl	192(%esp), %eax
	sbbl	20(%edi), %eax
	movl	%eax, %edx
	movl	24(%edi), %eax
	movl	%eax, 52(%esp)                  # 4-byte Spill
	sbbl	%eax, 12(%esp)                  # 4-byte Folded Spill
	movl	28(%edi), %eax
	movl	%eax, 48(%esp)                  # 4-byte Spill
	sbbl	%eax, 20(%esp)                  # 4-byte Folded Spill
	movl	32(%edi), %eax
	movl	%eax, 60(%esp)                  # 4-byte Spill
	sbbl	%eax, 24(%esp)                  # 4-byte Folded Spill
	movl	36(%edi), %eax
	movl	%eax, 104(%esp)                 # 4-byte Spill
	sbbl	%eax, 16(%esp)                  # 4-byte Folded Spill
	movl	40(%edi), %eax
	movl	%eax, 76(%esp)                  # 4-byte Spill
	sbbl	%eax, 40(%esp)                  # 4-byte Folded Spill
	movl	44(%edi), %eax
	movl	%eax, 64(%esp)                  # 4-byte Spill
	sbbl	%eax, 44(%esp)                  # 4-byte Folded Spill
	sbbl	$0, %ebp
	movl	%ebp, 36(%esp)                  # 4-byte Spill
	movl	48(%edi), %eax
	movl	%eax, 100(%esp)                 # 4-byte Spill
	subl	%eax, 32(%esp)                  # 4-byte Folded Spill
	movl	52(%edi), %eax
	movl	%eax, 108(%esp)                 # 4-byte Spill
	sbbl	%eax, 28(%esp)                  # 4-byte Folded Spill
	movl	56(%edi), %eax
	movl	%eax, 96(%esp)                  # 4-byte Spill
	sbbl	%eax, %ecx
	movl	%ecx, 68(%esp)                  # 4-byte Spill
	movl	60(%edi), %eax
	movl	%eax, 92(%esp)                  # 4-byte Spill
	sbbl	%eax, %esi
	movl	64(%edi), %eax
	movl	%eax, 88(%esp)                  # 4-byte Spill
	sbbl	%eax, %ebx
	movl	68(%edi), %eax
	movl	%eax, 84(%esp)                  # 4-byte Spill
	sbbl	%eax, %edx
	movl	%edx, 72(%esp)                  # 4-byte Spill
	movl	72(%edi), %eax
	movl	%eax, 80(%esp)                  # 4-byte Spill
	sbbl	%eax, 12(%esp)                  # 4-byte Folded Spill
	movl	76(%edi), %eax
	movl	%eax, 56(%esp)                  # 4-byte Spill
	sbbl	%eax, 20(%esp)                  # 4-byte Folded Spill
	movl	80(%edi), %eax
	movl	%eax, 120(%esp)                 # 4-byte Spill
	sbbl	%eax, 24(%esp)                  # 4-byte Folded Spill
	movl	84(%edi), %eax
	movl	%eax, 116(%esp)                 # 4-byte Spill
	movl	16(%esp), %ebp                  # 4-byte Reload
	sbbl	%eax, %ebp
	movl	88(%edi), %eax
	movl	%eax, 16(%esp)                  # 4-byte Spill
	sbbl	%eax, 40(%esp)                  # 4-byte Folded Spill
	movl	92(%edi), %eax
	movl	%eax, 112(%esp)                 # 4-byte Spill
	sbbl	%eax, 44(%esp)                  # 4-byte Folded Spill
	sbbl	$0, 36(%esp)                    # 4-byte Folded Spill
	movl	32(%esp), %edx                  # 4-byte Reload
	addl	52(%esp), %edx                  # 4-byte Folded Reload
	movl	28(%esp), %ecx                  # 4-byte Reload
	adcl	48(%esp), %ecx                  # 4-byte Folded Reload
	movl	68(%esp), %eax                  # 4-byte Reload
	adcl	60(%esp), %eax                  # 4-byte Folded Reload
	adcl	104(%esp), %esi                 # 4-byte Folded Reload
	adcl	76(%esp), %ebx                  # 4-byte Folded Reload
	movl	%esi, 36(%edi)
	movl	%eax, 32(%edi)
	movl	%ecx, 28(%edi)
	movl	%edx, 24(%edi)
	movl	72(%esp), %ecx                  # 4-byte Reload
	adcl	64(%esp), %ecx                  # 4-byte Folded Reload
	movl	%ebx, 40(%edi)
	movl	12(%esp), %eax                  # 4-byte Reload
	adcl	100(%esp), %eax                 # 4-byte Folded Reload
	movl	%ecx, 44(%edi)
	movl	20(%esp), %ecx                  # 4-byte Reload
	adcl	108(%esp), %ecx                 # 4-byte Folded Reload
	movl	%eax, 48(%edi)
	movl	24(%esp), %eax                  # 4-byte Reload
	adcl	96(%esp), %eax                  # 4-byte Folded Reload
	movl	%ecx, 52(%edi)
	adcl	92(%esp), %ebp                  # 4-byte Folded Reload
	movl	%eax, 56(%edi)
	movl	40(%esp), %eax                  # 4-byte Reload
	adcl	88(%esp), %eax                  # 4-byte Folded Reload
	movl	%ebp, 60(%edi)
	movl	44(%esp), %ecx                  # 4-byte Reload
	adcl	84(%esp), %ecx                  # 4-byte Folded Reload
	movl	%eax, 64(%edi)
	movl	36(%esp), %eax                  # 4-byte Reload
	adcl	80(%esp), %eax                  # 4-byte Folded Reload
	movl	%ecx, 68(%edi)
	movl	%eax, 72(%edi)
	movl	56(%esp), %eax                  # 4-byte Reload
	adcl	$0, %eax
	movl	%eax, 76(%edi)
	movl	120(%esp), %eax                 # 4-byte Reload
	adcl	$0, %eax
	movl	%eax, 80(%edi)
	movl	116(%esp), %eax                 # 4-byte Reload
	adcl	$0, %eax
	movl	%eax, 84(%edi)
	movl	16(%esp), %eax                  # 4-byte Reload
	adcl	$0, %eax
	movl	%eax, 88(%edi)
	movl	112(%esp), %eax                 # 4-byte Reload
	adcl	$0, %eax
	movl	%eax, 92(%edi)
	addl	$220, %esp
	popl	%esi
	popl	%edi
	popl	%ebx
	popl	%ebp
	retl
.Lfunc_end58:
	.size	mcl_fpDbl_mulPre12L, .Lfunc_end58-mcl_fpDbl_mulPre12L
                                        # -- End function
	.globl	mcl_fpDbl_sqrPre12L             # -- Begin function mcl_fpDbl_sqrPre12L
	.p2align	4, 0x90
	.type	mcl_fpDbl_sqrPre12L,@function
mcl_fpDbl_sqrPre12L:                    # @mcl_fpDbl_sqrPre12L
# %bb.0:
	pushl	%ebp
	pushl	%ebx
	pushl	%edi
	pushl	%esi
	subl	$220, %esp
	calll	.L59$pb
.L59$pb:
	popl	%ebp
.Ltmp11:
	addl	$_GLOBAL_OFFSET_TABLE_+(.Ltmp11-.L59$pb), %ebp
	subl	$4, %esp
	movl	248(%esp), %esi
	movl	244(%esp), %edi
	movl	%ebp, %ebx
	movl	%ebp, 32(%esp)                  # 4-byte Spill
	pushl	%esi
	pushl	%esi
	pushl	%edi
	calll	mcl_fpDbl_mulPre6L@PLT
	addl	$12, %esp
	leal	24(%esi), %eax
	leal	48(%edi), %ecx
	pushl	%eax
	pushl	%eax
	pushl	%ecx
	calll	mcl_fpDbl_mulPre6L@PLT
	addl	$16, %esp
	movl	44(%esi), %edi
	movl	40(%esi), %ecx
	movl	36(%esi), %ebx
	movl	32(%esi), %eax
	movl	24(%esi), %ebp
	movl	28(%esi), %edx
	addl	(%esi), %ebp
	adcl	4(%esi), %edx
	adcl	8(%esi), %eax
	adcl	12(%esi), %ebx
	adcl	16(%esi), %ecx
	adcl	20(%esi), %edi
	movl	%edi, 8(%esp)                   # 4-byte Spill
	movl	%edi, 168(%esp)
	movl	%ecx, 164(%esp)
	movl	%ebx, 12(%esp)                  # 4-byte Spill
	movl	%ebx, 160(%esp)
	movl	%eax, 156(%esp)
	movl	%edx, 152(%esp)
	movl	%ebp, 148(%esp)
	movl	%edi, 144(%esp)
	movl	%ecx, 140(%esp)
	movl	%ebx, 136(%esp)
	movl	%eax, 132(%esp)
	movl	%edx, 128(%esp)
	movl	%ebp, 124(%esp)
	setb	%bl
	subl	$4, %esp
	movzbl	%bl, %edi
	movl	%edi, 28(%esp)                  # 4-byte Spill
	movl	%edi, %ebx
	shll	$31, %ebx
	negl	%edi
	shrdl	$31, %edi, %ebx
	andl	%ebp, %ebx
	movl	%ebx, %esi
	andl	%edi, %edx
	andl	%edi, %eax
	movl	16(%esp), %ebx                  # 4-byte Reload
	andl	%edi, %ebx
	andl	%edi, %ecx
	andl	12(%esp), %edi                  # 4-byte Folded Reload
	movl	%edi, %ebp
	shldl	$1, %ecx, %ebp
	movl	%ebp, 12(%esp)                  # 4-byte Spill
	shldl	$1, %ebx, %ecx
	movl	%ecx, 24(%esp)                  # 4-byte Spill
	shldl	$1, %eax, %ebx
	movl	%ebx, 16(%esp)                  # 4-byte Spill
	shldl	$1, %edx, %eax
	movl	%eax, 20(%esp)                  # 4-byte Spill
	shldl	$1, %esi, %edx
	movl	%edx, %ebp
	shrl	$31, %edi
	addl	%esi, %esi
	leal	128(%esp), %eax
	movl	32(%esp), %ebx                  # 4-byte Reload
	pushl	%eax
	leal	156(%esp), %eax
	pushl	%eax
	leal	184(%esp), %eax
	pushl	%eax
	calll	mcl_fpDbl_mulPre6L@PLT
	addl	$16, %esp
	addl	196(%esp), %esi
	adcl	200(%esp), %ebp
	movl	%ebp, %edx
	movl	16(%esp), %eax                  # 4-byte Reload
	adcl	204(%esp), %eax
	movl	%eax, 16(%esp)                  # 4-byte Spill
	movl	12(%esp), %eax                  # 4-byte Reload
	adcl	208(%esp), %eax
	movl	%eax, 12(%esp)                  # 4-byte Spill
	movl	20(%esp), %eax                  # 4-byte Reload
	adcl	212(%esp), %eax
	movl	%eax, 20(%esp)                  # 4-byte Spill
	movl	8(%esp), %eax                   # 4-byte Reload
	adcl	216(%esp), %eax
	movl	%eax, 8(%esp)                   # 4-byte Spill
	adcl	24(%esp), %edi                  # 4-byte Folded Reload
	movl	172(%esp), %ecx
	movl	240(%esp), %eax
	subl	(%eax), %ecx
	movl	%ecx, 28(%esp)                  # 4-byte Spill
	movl	176(%esp), %ebx
	sbbl	4(%eax), %ebx
	movl	180(%esp), %ecx
	sbbl	8(%eax), %ecx
	movl	%ecx, 32(%esp)                  # 4-byte Spill
	movl	184(%esp), %ebp
	sbbl	12(%eax), %ebp
	movl	188(%esp), %ecx
	sbbl	16(%eax), %ecx
	movl	%ecx, 36(%esp)                  # 4-byte Spill
	movl	192(%esp), %ecx
	sbbl	20(%eax), %ecx
	movl	%ecx, 24(%esp)                  # 4-byte Spill
	movl	24(%eax), %ecx
	movl	%ecx, 120(%esp)                 # 4-byte Spill
	sbbl	%ecx, %esi
	movl	28(%eax), %ecx
	movl	%ecx, 112(%esp)                 # 4-byte Spill
	sbbl	%ecx, %edx
	movl	32(%eax), %ecx
	movl	%ecx, 108(%esp)                 # 4-byte Spill
	sbbl	%ecx, 16(%esp)                  # 4-byte Folded Spill
	movl	36(%eax), %ecx
	movl	%ecx, 76(%esp)                  # 4-byte Spill
	sbbl	%ecx, 12(%esp)                  # 4-byte Folded Spill
	movl	40(%eax), %ecx
	movl	%ecx, 52(%esp)                  # 4-byte Spill
	sbbl	%ecx, 20(%esp)                  # 4-byte Folded Spill
	movl	44(%eax), %ecx
	movl	%ecx, 116(%esp)                 # 4-byte Spill
	sbbl	%ecx, 8(%esp)                   # 4-byte Folded Spill
	sbbl	$0, %edi
	movl	48(%eax), %ecx
	movl	%ecx, 72(%esp)                  # 4-byte Spill
	subl	%ecx, 28(%esp)                  # 4-byte Folded Spill
	movl	52(%eax), %ecx
	movl	%ecx, 84(%esp)                  # 4-byte Spill
	sbbl	%ecx, %ebx
	movl	%ebx, 40(%esp)                  # 4-byte Spill
	movl	56(%eax), %ecx
	movl	%ecx, 80(%esp)                  # 4-byte Spill
	sbbl	%ecx, 32(%esp)                  # 4-byte Folded Spill
	movl	60(%eax), %ecx
	movl	%ecx, 68(%esp)                  # 4-byte Spill
	sbbl	%ecx, %ebp
	movl	64(%eax), %ecx
	movl	%ecx, 64(%esp)                  # 4-byte Spill
	sbbl	%ecx, 36(%esp)                  # 4-byte Folded Spill
	movl	68(%eax), %ecx
	movl	%ecx, 60(%esp)                  # 4-byte Spill
	sbbl	%ecx, 24(%esp)                  # 4-byte Folded Spill
	movl	72(%eax), %ecx
	movl	%ecx, 56(%esp)                  # 4-byte Spill
	sbbl	%ecx, %esi
	movl	%esi, 48(%esp)                  # 4-byte Spill
	movl	76(%eax), %ecx
	movl	%ecx, 104(%esp)                 # 4-byte Spill
	sbbl	%ecx, %edx
	movl	%edx, 44(%esp)                  # 4-byte Spill
	movl	80(%eax), %ecx
	movl	%ecx, 100(%esp)                 # 4-byte Spill
	sbbl	%ecx, 16(%esp)                  # 4-byte Folded Spill
	movl	84(%eax), %ecx
	movl	%ecx, 96(%esp)                  # 4-byte Spill
	sbbl	%ecx, 12(%esp)                  # 4-byte Folded Spill
	movl	88(%eax), %ecx
	movl	%ecx, 92(%esp)                  # 4-byte Spill
	sbbl	%ecx, 20(%esp)                  # 4-byte Folded Spill
	movl	92(%eax), %ecx
	movl	%ecx, 88(%esp)                  # 4-byte Spill
	sbbl	%ecx, 8(%esp)                   # 4-byte Folded Spill
	sbbl	$0, %edi
	movl	28(%esp), %ebx                  # 4-byte Reload
	addl	120(%esp), %ebx                 # 4-byte Folded Reload
	movl	40(%esp), %edx                  # 4-byte Reload
	adcl	112(%esp), %edx                 # 4-byte Folded Reload
	movl	32(%esp), %ecx                  # 4-byte Reload
	adcl	108(%esp), %ecx                 # 4-byte Folded Reload
	adcl	76(%esp), %ebp                  # 4-byte Folded Reload
	movl	36(%esp), %esi                  # 4-byte Reload
	adcl	52(%esp), %esi                  # 4-byte Folded Reload
	movl	%ebp, 36(%eax)
	movl	%ecx, 32(%eax)
	movl	%edx, 28(%eax)
	movl	%ebx, 24(%eax)
	movl	24(%esp), %edx                  # 4-byte Reload
	adcl	116(%esp), %edx                 # 4-byte Folded Reload
	movl	%esi, 40(%eax)
	movl	48(%esp), %esi                  # 4-byte Reload
	adcl	72(%esp), %esi                  # 4-byte Folded Reload
	movl	%edx, 44(%eax)
	movl	44(%esp), %ecx                  # 4-byte Reload
	adcl	84(%esp), %ecx                  # 4-byte Folded Reload
	movl	%esi, 48(%eax)
	movl	16(%esp), %edx                  # 4-byte Reload
	adcl	80(%esp), %edx                  # 4-byte Folded Reload
	movl	%ecx, 52(%eax)
	movl	12(%esp), %ecx                  # 4-byte Reload
	adcl	68(%esp), %ecx                  # 4-byte Folded Reload
	movl	%edx, 56(%eax)
	movl	20(%esp), %edx                  # 4-byte Reload
	adcl	64(%esp), %edx                  # 4-byte Folded Reload
	movl	%ecx, 60(%eax)
	movl	8(%esp), %ecx                   # 4-byte Reload
	adcl	60(%esp), %ecx                  # 4-byte Folded Reload
	movl	%edx, 64(%eax)
	adcl	56(%esp), %edi                  # 4-byte Folded Reload
	movl	%ecx, 68(%eax)
	movl	%edi, 72(%eax)
	movl	104(%esp), %ecx                 # 4-byte Reload
	adcl	$0, %ecx
	movl	%ecx, 76(%eax)
	movl	100(%esp), %ecx                 # 4-byte Reload
	adcl	$0, %ecx
	movl	%ecx, 80(%eax)
	movl	96(%esp), %ecx                  # 4-byte Reload
	adcl	$0, %ecx
	movl	%ecx, 84(%eax)
	movl	92(%esp), %ecx                  # 4-byte Reload
	adcl	$0, %ecx
	movl	%ecx, 88(%eax)
	movl	88(%esp), %ecx                  # 4-byte Reload
	adcl	$0, %ecx
	movl	%ecx, 92(%eax)
	addl	$220, %esp
	popl	%esi
	popl	%edi
	popl	%ebx
	popl	%ebp
	retl
.Lfunc_end59:
	.size	mcl_fpDbl_sqrPre12L, .Lfunc_end59-mcl_fpDbl_sqrPre12L
                                        # -- End function
	.globl	mcl_fp_mont12L                  # -- Begin function mcl_fp_mont12L
	.p2align	4, 0x90
	.type	mcl_fp_mont12L,@function
mcl_fp_mont12L:                         # @mcl_fp_mont12L
# %bb.0:
	pushl	%ebp
	pushl	%ebx
	pushl	%edi
	pushl	%esi
	subl	$1420, %esp                     # imm = 0x58C
	calll	.L60$pb
.L60$pb:
	popl	%ebx
.Ltmp12:
	addl	$_GLOBAL_OFFSET_TABLE_+(.Ltmp12-.L60$pb), %ebx
	movl	1452(%esp), %eax
	movl	-4(%eax), %edi
	movl	%edi, 52(%esp)                  # 4-byte Spill
	movl	1448(%esp), %ecx
	subl	$4, %esp
	leal	1372(%esp), %eax
	pushl	(%ecx)
	pushl	1452(%esp)
	pushl	%eax
	calll	mulPv384x32@PLT
	addl	$12, %esp
	movl	1368(%esp), %esi
	movl	1372(%esp), %eax
	movl	%eax, 32(%esp)                  # 4-byte Spill
	movl	%edi, %eax
	imull	%esi, %eax
	movl	1416(%esp), %ecx
	movl	%ecx, 20(%esp)                  # 4-byte Spill
	movl	1412(%esp), %ecx
	movl	%ecx, 44(%esp)                  # 4-byte Spill
	movl	1408(%esp), %ecx
	movl	%ecx, (%esp)                    # 4-byte Spill
	movl	1404(%esp), %ecx
	movl	%ecx, 8(%esp)                   # 4-byte Spill
	movl	1400(%esp), %ecx
	movl	%ecx, 28(%esp)                  # 4-byte Spill
	movl	1396(%esp), %ecx
	movl	%ecx, 24(%esp)                  # 4-byte Spill
	movl	1392(%esp), %ecx
	movl	%ecx, 4(%esp)                   # 4-byte Spill
	movl	1388(%esp), %ebp
	movl	1384(%esp), %ecx
	movl	%ecx, 48(%esp)                  # 4-byte Spill
	movl	1380(%esp), %edi
	movl	1376(%esp), %ecx
	movl	%ecx, 16(%esp)                  # 4-byte Spill
	subl	$4, %esp
	leal	1316(%esp), %ecx
	pushl	%eax
	pushl	1460(%esp)
	pushl	%ecx
	calll	mulPv384x32@PLT
	addl	$12, %esp
	addl	1312(%esp), %esi
	movl	32(%esp), %eax                  # 4-byte Reload
	adcl	1316(%esp), %eax
	movl	%eax, 32(%esp)                  # 4-byte Spill
	movl	16(%esp), %eax                  # 4-byte Reload
	adcl	1320(%esp), %eax
	movl	%eax, 16(%esp)                  # 4-byte Spill
	adcl	1324(%esp), %edi
	movl	%edi, 12(%esp)                  # 4-byte Spill
	movl	48(%esp), %esi                  # 4-byte Reload
	adcl	1328(%esp), %esi
	adcl	1332(%esp), %ebp
	movl	4(%esp), %eax                   # 4-byte Reload
	adcl	1336(%esp), %eax
	movl	%eax, 4(%esp)                   # 4-byte Spill
	movl	24(%esp), %eax                  # 4-byte Reload
	adcl	1340(%esp), %eax
	movl	%eax, 24(%esp)                  # 4-byte Spill
	movl	28(%esp), %eax                  # 4-byte Reload
	adcl	1344(%esp), %eax
	movl	%eax, 28(%esp)                  # 4-byte Spill
	movl	8(%esp), %eax                   # 4-byte Reload
	adcl	1348(%esp), %eax
	movl	%eax, 8(%esp)                   # 4-byte Spill
	movl	(%esp), %eax                    # 4-byte Reload
	adcl	1352(%esp), %eax
	movl	%eax, (%esp)                    # 4-byte Spill
	movl	44(%esp), %edi                  # 4-byte Reload
	adcl	1356(%esp), %edi
	movl	20(%esp), %eax                  # 4-byte Reload
	adcl	1360(%esp), %eax
	movl	%eax, 20(%esp)                  # 4-byte Spill
	setb	%al
	subl	$4, %esp
	leal	1260(%esp), %ecx
	movzbl	%al, %eax
	movl	%eax, 40(%esp)                  # 4-byte Spill
	movl	1452(%esp), %eax
	pushl	4(%eax)
	pushl	1452(%esp)
	pushl	%ecx
	calll	mulPv384x32@PLT
	addl	$12, %esp
	movl	32(%esp), %ecx                  # 4-byte Reload
	addl	1256(%esp), %ecx
	movl	16(%esp), %eax                  # 4-byte Reload
	adcl	1260(%esp), %eax
	movl	%eax, 16(%esp)                  # 4-byte Spill
	movl	12(%esp), %eax                  # 4-byte Reload
	adcl	1264(%esp), %eax
	movl	%eax, 12(%esp)                  # 4-byte Spill
	adcl	1268(%esp), %esi
	adcl	1272(%esp), %ebp
	movl	4(%esp), %eax                   # 4-byte Reload
	adcl	1276(%esp), %eax
	movl	%eax, 4(%esp)                   # 4-byte Spill
	movl	24(%esp), %eax                  # 4-byte Reload
	adcl	1280(%esp), %eax
	movl	%eax, 24(%esp)                  # 4-byte Spill
	movl	28(%esp), %eax                  # 4-byte Reload
	adcl	1284(%esp), %eax
	movl	%eax, 28(%esp)                  # 4-byte Spill
	movl	8(%esp), %eax                   # 4-byte Reload
	adcl	1288(%esp), %eax
	movl	%eax, 8(%esp)                   # 4-byte Spill
	movl	(%esp), %eax                    # 4-byte Reload
	adcl	1292(%esp), %eax
	movl	%eax, (%esp)                    # 4-byte Spill
	adcl	1296(%esp), %edi
	movl	%edi, 44(%esp)                  # 4-byte Spill
	movl	20(%esp), %eax                  # 4-byte Reload
	adcl	1300(%esp), %eax
	movl	%eax, 20(%esp)                  # 4-byte Spill
	movl	36(%esp), %eax                  # 4-byte Reload
	adcl	1304(%esp), %eax
	movl	%eax, 36(%esp)                  # 4-byte Spill
	setb	%al
	subl	$4, %esp
	movl	56(%esp), %edx                  # 4-byte Reload
	movl	%ecx, %edi
	imull	%ecx, %edx
	movzbl	%al, %eax
	movl	%eax, 36(%esp)                  # 4-byte Spill
	pushl	%edx
	pushl	1460(%esp)
	leal	1212(%esp), %eax
	pushl	%eax
	calll	mulPv384x32@PLT
	addl	$12, %esp
	addl	1200(%esp), %edi
	movl	16(%esp), %eax                  # 4-byte Reload
	adcl	1204(%esp), %eax
	movl	%eax, 16(%esp)                  # 4-byte Spill
	movl	12(%esp), %eax                  # 4-byte Reload
	adcl	1208(%esp), %eax
	movl	%eax, 12(%esp)                  # 4-byte Spill
	adcl	1212(%esp), %esi
	movl	%esi, 48(%esp)                  # 4-byte Spill
	adcl	1216(%esp), %ebp
	movl	%ebp, 40(%esp)                  # 4-byte Spill
	movl	4(%esp), %esi                   # 4-byte Reload
	adcl	1220(%esp), %esi
	movl	24(%esp), %eax                  # 4-byte Reload
	adcl	1224(%esp), %eax
	movl	%eax, 24(%esp)                  # 4-byte Spill
	movl	28(%esp), %eax                  # 4-byte Reload
	adcl	1228(%esp), %eax
	movl	%eax, 28(%esp)                  # 4-byte Spill
	movl	8(%esp), %eax                   # 4-byte Reload
	adcl	1232(%esp), %eax
	movl	%eax, 8(%esp)                   # 4-byte Spill
	movl	(%esp), %edi                    # 4-byte Reload
	adcl	1236(%esp), %edi
	movl	44(%esp), %eax                  # 4-byte Reload
	adcl	1240(%esp), %eax
	movl	%eax, 44(%esp)                  # 4-byte Spill
	movl	20(%esp), %ebp                  # 4-byte Reload
	adcl	1244(%esp), %ebp
	movl	36(%esp), %eax                  # 4-byte Reload
	adcl	1248(%esp), %eax
	movl	%eax, 36(%esp)                  # 4-byte Spill
	adcl	$0, 32(%esp)                    # 4-byte Folded Spill
	subl	$4, %esp
	leal	1148(%esp), %eax
	movl	1452(%esp), %ecx
	pushl	8(%ecx)
	pushl	1452(%esp)
	pushl	%eax
	calll	mulPv384x32@PLT
	addl	$12, %esp
	movl	16(%esp), %ecx                  # 4-byte Reload
	addl	1144(%esp), %ecx
	movl	12(%esp), %eax                  # 4-byte Reload
	adcl	1148(%esp), %eax
	movl	%eax, 12(%esp)                  # 4-byte Spill
	movl	48(%esp), %eax                  # 4-byte Reload
	adcl	1152(%esp), %eax
	movl	%eax, 48(%esp)                  # 4-byte Spill
	movl	40(%esp), %eax                  # 4-byte Reload
	adcl	1156(%esp), %eax
	movl	%eax, 40(%esp)                  # 4-byte Spill
	adcl	1160(%esp), %esi
	movl	%esi, 4(%esp)                   # 4-byte Spill
	movl	24(%esp), %esi                  # 4-byte Reload
	adcl	1164(%esp), %esi
	movl	28(%esp), %eax                  # 4-byte Reload
	adcl	1168(%esp), %eax
	movl	%eax, 28(%esp)                  # 4-byte Spill
	movl	8(%esp), %eax                   # 4-byte Reload
	adcl	1172(%esp), %eax
	movl	%eax, 8(%esp)                   # 4-byte Spill
	adcl	1176(%esp), %edi
	movl	%edi, (%esp)                    # 4-byte Spill
	movl	44(%esp), %edi                  # 4-byte Reload
	adcl	1180(%esp), %edi
	adcl	1184(%esp), %ebp
	movl	%ebp, 20(%esp)                  # 4-byte Spill
	movl	36(%esp), %eax                  # 4-byte Reload
	adcl	1188(%esp), %eax
	movl	%eax, 36(%esp)                  # 4-byte Spill
	movl	32(%esp), %eax                  # 4-byte Reload
	adcl	1192(%esp), %eax
	movl	%eax, 32(%esp)                  # 4-byte Spill
	setb	%al
	subl	$4, %esp
	movl	56(%esp), %edx                  # 4-byte Reload
	imull	%ecx, %edx
	movl	%ecx, %ebp
	movzbl	%al, %eax
	movl	%eax, 20(%esp)                  # 4-byte Spill
	pushl	%edx
	movl	1460(%esp), %eax
	pushl	%eax
	leal	1100(%esp), %eax
	pushl	%eax
	calll	mulPv384x32@PLT
	addl	$12, %esp
	addl	1088(%esp), %ebp
	movl	12(%esp), %eax                  # 4-byte Reload
	adcl	1092(%esp), %eax
	movl	%eax, 12(%esp)                  # 4-byte Spill
	movl	48(%esp), %eax                  # 4-byte Reload
	adcl	1096(%esp), %eax
	movl	%eax, 48(%esp)                  # 4-byte Spill
	movl	40(%esp), %eax                  # 4-byte Reload
	adcl	1100(%esp), %eax
	movl	%eax, 40(%esp)                  # 4-byte Spill
	movl	4(%esp), %eax                   # 4-byte Reload
	adcl	1104(%esp), %eax
	movl	%eax, 4(%esp)                   # 4-byte Spill
	adcl	1108(%esp), %esi
	movl	%esi, 24(%esp)                  # 4-byte Spill
	movl	28(%esp), %eax                  # 4-byte Reload
	adcl	1112(%esp), %eax
	movl	%eax, 28(%esp)                  # 4-byte Spill
	movl	8(%esp), %eax                   # 4-byte Reload
	adcl	1116(%esp), %eax
	movl	%eax, 8(%esp)                   # 4-byte Spill
	movl	(%esp), %eax                    # 4-byte Reload
	adcl	1120(%esp), %eax
	movl	%eax, (%esp)                    # 4-byte Spill
	adcl	1124(%esp), %edi
	movl	%edi, 44(%esp)                  # 4-byte Spill
	movl	20(%esp), %eax                  # 4-byte Reload
	adcl	1128(%esp), %eax
	movl	%eax, 20(%esp)                  # 4-byte Spill
	movl	36(%esp), %ebp                  # 4-byte Reload
	adcl	1132(%esp), %ebp
	movl	32(%esp), %esi                  # 4-byte Reload
	adcl	1136(%esp), %esi
	movl	16(%esp), %edi                  # 4-byte Reload
	adcl	$0, %edi
	subl	$4, %esp
	leal	1036(%esp), %eax
	movl	1452(%esp), %ecx
	pushl	12(%ecx)
	pushl	1452(%esp)
	pushl	%eax
	calll	mulPv384x32@PLT
	addl	$12, %esp
	movl	12(%esp), %ecx                  # 4-byte Reload
	addl	1032(%esp), %ecx
	movl	48(%esp), %eax                  # 4-byte Reload
	adcl	1036(%esp), %eax
	movl	%eax, 48(%esp)                  # 4-byte Spill
	movl	40(%esp), %eax                  # 4-byte Reload
	adcl	1040(%esp), %eax
	movl	%eax, 40(%esp)                  # 4-byte Spill
	movl	4(%esp), %eax                   # 4-byte Reload
	adcl	1044(%esp), %eax
	movl	%eax, 4(%esp)                   # 4-byte Spill
	movl	24(%esp), %eax                  # 4-byte Reload
	adcl	1048(%esp), %eax
	movl	%eax, 24(%esp)                  # 4-byte Spill
	movl	28(%esp), %eax                  # 4-byte Reload
	adcl	1052(%esp), %eax
	movl	%eax, 28(%esp)                  # 4-byte Spill
	movl	8(%esp), %eax                   # 4-byte Reload
	adcl	1056(%esp), %eax
	movl	%eax, 8(%esp)                   # 4-byte Spill
	movl	(%esp), %eax                    # 4-byte Reload
	adcl	1060(%esp), %eax
	movl	%eax, (%esp)                    # 4-byte Spill
	movl	44(%esp), %eax                  # 4-byte Reload
	adcl	1064(%esp), %eax
	movl	%eax, 44(%esp)                  # 4-byte Spill
	movl	20(%esp), %eax                  # 4-byte Reload
	adcl	1068(%esp), %eax
	movl	%eax, 20(%esp)                  # 4-byte Spill
	adcl	1072(%esp), %ebp
	movl	%ebp, 36(%esp)                  # 4-byte Spill
	adcl	1076(%esp), %esi
	movl	%esi, 32(%esp)                  # 4-byte Spill
	adcl	1080(%esp), %edi
	movl	%edi, 16(%esp)                  # 4-byte Spill
	setb	%al
	subl	$4, %esp
	leal	980(%esp), %edi
	movl	56(%esp), %edx                  # 4-byte Reload
	movl	%ecx, %esi
	imull	%ecx, %edx
	movzbl	%al, %eax
	movl	%eax, 16(%esp)                  # 4-byte Spill
	pushl	%edx
	pushl	1460(%esp)
	pushl	%edi
	calll	mulPv384x32@PLT
	addl	$12, %esp
	addl	976(%esp), %esi
	movl	48(%esp), %eax                  # 4-byte Reload
	adcl	980(%esp), %eax
	movl	%eax, 48(%esp)                  # 4-byte Spill
	movl	40(%esp), %eax                  # 4-byte Reload
	adcl	984(%esp), %eax
	movl	%eax, 40(%esp)                  # 4-byte Spill
	movl	4(%esp), %eax                   # 4-byte Reload
	adcl	988(%esp), %eax
	movl	%eax, 4(%esp)                   # 4-byte Spill
	movl	24(%esp), %eax                  # 4-byte Reload
	adcl	992(%esp), %eax
	movl	%eax, 24(%esp)                  # 4-byte Spill
	movl	28(%esp), %ebp                  # 4-byte Reload
	adcl	996(%esp), %ebp
	movl	8(%esp), %esi                   # 4-byte Reload
	adcl	1000(%esp), %esi
	movl	(%esp), %eax                    # 4-byte Reload
	adcl	1004(%esp), %eax
	movl	%eax, (%esp)                    # 4-byte Spill
	movl	44(%esp), %edi                  # 4-byte Reload
	adcl	1008(%esp), %edi
	movl	20(%esp), %eax                  # 4-byte Reload
	adcl	1012(%esp), %eax
	movl	%eax, 20(%esp)                  # 4-byte Spill
	movl	36(%esp), %eax                  # 4-byte Reload
	adcl	1016(%esp), %eax
	movl	%eax, 36(%esp)                  # 4-byte Spill
	movl	32(%esp), %eax                  # 4-byte Reload
	adcl	1020(%esp), %eax
	movl	%eax, 32(%esp)                  # 4-byte Spill
	movl	16(%esp), %eax                  # 4-byte Reload
	adcl	1024(%esp), %eax
	movl	%eax, 16(%esp)                  # 4-byte Spill
	adcl	$0, 12(%esp)                    # 4-byte Folded Spill
	subl	$4, %esp
	leal	924(%esp), %eax
	movl	1452(%esp), %ecx
	pushl	16(%ecx)
	pushl	1452(%esp)
	pushl	%eax
	calll	mulPv384x32@PLT
	addl	$12, %esp
	movl	48(%esp), %ecx                  # 4-byte Reload
	addl	920(%esp), %ecx
	movl	40(%esp), %eax                  # 4-byte Reload
	adcl	924(%esp), %eax
	movl	%eax, 40(%esp)                  # 4-byte Spill
	movl	4(%esp), %eax                   # 4-byte Reload
	adcl	928(%esp), %eax
	movl	%eax, 4(%esp)                   # 4-byte Spill
	movl	24(%esp), %eax                  # 4-byte Reload
	adcl	932(%esp), %eax
	movl	%eax, 24(%esp)                  # 4-byte Spill
	adcl	936(%esp), %ebp
	movl	%ebp, 28(%esp)                  # 4-byte Spill
	adcl	940(%esp), %esi
	movl	%esi, 8(%esp)                   # 4-byte Spill
	movl	(%esp), %eax                    # 4-byte Reload
	adcl	944(%esp), %eax
	movl	%eax, (%esp)                    # 4-byte Spill
	adcl	948(%esp), %edi
	movl	%edi, 44(%esp)                  # 4-byte Spill
	movl	20(%esp), %eax                  # 4-byte Reload
	adcl	952(%esp), %eax
	movl	%eax, 20(%esp)                  # 4-byte Spill
	movl	36(%esp), %eax                  # 4-byte Reload
	adcl	956(%esp), %eax
	movl	%eax, 36(%esp)                  # 4-byte Spill
	movl	32(%esp), %esi                  # 4-byte Reload
	adcl	960(%esp), %esi
	movl	16(%esp), %eax                  # 4-byte Reload
	adcl	964(%esp), %eax
	movl	%eax, 16(%esp)                  # 4-byte Spill
	movl	12(%esp), %eax                  # 4-byte Reload
	adcl	968(%esp), %eax
	movl	%eax, 12(%esp)                  # 4-byte Spill
	setb	%al
	subl	$4, %esp
	movl	56(%esp), %edx                  # 4-byte Reload
	movl	%ecx, %ebp
	imull	%ecx, %edx
	movzbl	%al, %edi
	pushl	%edx
	pushl	1460(%esp)
	leal	876(%esp), %eax
	pushl	%eax
	calll	mulPv384x32@PLT
	addl	$12, %esp
	addl	864(%esp), %ebp
	movl	40(%esp), %eax                  # 4-byte Reload
	adcl	868(%esp), %eax
	movl	%eax, 40(%esp)                  # 4-byte Spill
	movl	4(%esp), %eax                   # 4-byte Reload
	adcl	872(%esp), %eax
	movl	%eax, 4(%esp)                   # 4-byte Spill
	movl	24(%esp), %ebp                  # 4-byte Reload
	adcl	876(%esp), %ebp
	movl	28(%esp), %eax                  # 4-byte Reload
	adcl	880(%esp), %eax
	movl	%eax, 28(%esp)                  # 4-byte Spill
	movl	8(%esp), %eax                   # 4-byte Reload
	adcl	884(%esp), %eax
	movl	%eax, 8(%esp)                   # 4-byte Spill
	movl	(%esp), %eax                    # 4-byte Reload
	adcl	888(%esp), %eax
	movl	%eax, (%esp)                    # 4-byte Spill
	movl	44(%esp), %eax                  # 4-byte Reload
	adcl	892(%esp), %eax
	movl	%eax, 44(%esp)                  # 4-byte Spill
	movl	20(%esp), %eax                  # 4-byte Reload
	adcl	896(%esp), %eax
	movl	%eax, 20(%esp)                  # 4-byte Spill
	movl	36(%esp), %eax                  # 4-byte Reload
	adcl	900(%esp), %eax
	movl	%eax, 36(%esp)                  # 4-byte Spill
	adcl	904(%esp), %esi
	movl	%esi, 32(%esp)                  # 4-byte Spill
	movl	16(%esp), %eax                  # 4-byte Reload
	adcl	908(%esp), %eax
	movl	%eax, 16(%esp)                  # 4-byte Spill
	movl	12(%esp), %eax                  # 4-byte Reload
	adcl	912(%esp), %eax
	movl	%eax, 12(%esp)                  # 4-byte Spill
	adcl	$0, %edi
	subl	$4, %esp
	movl	1452(%esp), %eax
	pushl	20(%eax)
	pushl	1452(%esp)
	leal	820(%esp), %eax
	pushl	%eax
	calll	mulPv384x32@PLT
	addl	$12, %esp
	movl	40(%esp), %edx                  # 4-byte Reload
	addl	808(%esp), %edx
	movl	4(%esp), %eax                   # 4-byte Reload
	adcl	812(%esp), %eax
	movl	%eax, 4(%esp)                   # 4-byte Spill
	adcl	816(%esp), %ebp
	movl	%ebp, 24(%esp)                  # 4-byte Spill
	movl	28(%esp), %eax                  # 4-byte Reload
	adcl	820(%esp), %eax
	movl	%eax, 28(%esp)                  # 4-byte Spill
	movl	8(%esp), %eax                   # 4-byte Reload
	adcl	824(%esp), %eax
	movl	%eax, 8(%esp)                   # 4-byte Spill
	movl	(%esp), %eax                    # 4-byte Reload
	adcl	828(%esp), %eax
	movl	%eax, (%esp)                    # 4-byte Spill
	movl	44(%esp), %ebp                  # 4-byte Reload
	adcl	832(%esp), %ebp
	movl	20(%esp), %eax                  # 4-byte Reload
	adcl	836(%esp), %eax
	movl	%eax, 20(%esp)                  # 4-byte Spill
	movl	36(%esp), %esi                  # 4-byte Reload
	adcl	840(%esp), %esi
	movl	32(%esp), %eax                  # 4-byte Reload
	adcl	844(%esp), %eax
	movl	%eax, 32(%esp)                  # 4-byte Spill
	movl	16(%esp), %eax                  # 4-byte Reload
	adcl	848(%esp), %eax
	movl	%eax, 16(%esp)                  # 4-byte Spill
	movl	12(%esp), %eax                  # 4-byte Reload
	adcl	852(%esp), %eax
	movl	%eax, 12(%esp)                  # 4-byte Spill
	adcl	856(%esp), %edi
	movl	%edi, 48(%esp)                  # 4-byte Spill
	setb	%al
	subl	$4, %esp
	movl	56(%esp), %ecx                  # 4-byte Reload
	movl	%edx, %edi
	imull	%edx, %ecx
	movzbl	%al, %eax
	movl	%eax, 44(%esp)                  # 4-byte Spill
	pushl	%ecx
	pushl	1460(%esp)
	leal	764(%esp), %eax
	pushl	%eax
	calll	mulPv384x32@PLT
	addl	$12, %esp
	addl	752(%esp), %edi
	movl	4(%esp), %eax                   # 4-byte Reload
	adcl	756(%esp), %eax
	movl	%eax, 4(%esp)                   # 4-byte Spill
	movl	24(%esp), %eax                  # 4-byte Reload
	adcl	760(%esp), %eax
	movl	%eax, 24(%esp)                  # 4-byte Spill
	movl	28(%esp), %eax                  # 4-byte Reload
	adcl	764(%esp), %eax
	movl	%eax, 28(%esp)                  # 4-byte Spill
	movl	8(%esp), %eax                   # 4-byte Reload
	adcl	768(%esp), %eax
	movl	%eax, 8(%esp)                   # 4-byte Spill
	movl	(%esp), %eax                    # 4-byte Reload
	adcl	772(%esp), %eax
	movl	%eax, (%esp)                    # 4-byte Spill
	adcl	776(%esp), %ebp
	movl	%ebp, 44(%esp)                  # 4-byte Spill
	movl	20(%esp), %edi                  # 4-byte Reload
	adcl	780(%esp), %edi
	adcl	784(%esp), %esi
	movl	%esi, %ebp
	movl	32(%esp), %eax                  # 4-byte Reload
	adcl	788(%esp), %eax
	movl	%eax, 32(%esp)                  # 4-byte Spill
	movl	16(%esp), %eax                  # 4-byte Reload
	adcl	792(%esp), %eax
	movl	%eax, 16(%esp)                  # 4-byte Spill
	movl	12(%esp), %eax                  # 4-byte Reload
	adcl	796(%esp), %eax
	movl	%eax, 12(%esp)                  # 4-byte Spill
	movl	48(%esp), %eax                  # 4-byte Reload
	adcl	800(%esp), %eax
	movl	%eax, 48(%esp)                  # 4-byte Spill
	adcl	$0, 40(%esp)                    # 4-byte Folded Spill
	subl	$4, %esp
	movl	1452(%esp), %eax
	pushl	24(%eax)
	movl	1452(%esp), %eax
	pushl	%eax
	leal	708(%esp), %eax
	pushl	%eax
	calll	mulPv384x32@PLT
	addl	$12, %esp
	movl	4(%esp), %edx                   # 4-byte Reload
	addl	696(%esp), %edx
	movl	24(%esp), %eax                  # 4-byte Reload
	adcl	700(%esp), %eax
	movl	%eax, 24(%esp)                  # 4-byte Spill
	movl	28(%esp), %eax                  # 4-byte Reload
	adcl	704(%esp), %eax
	movl	%eax, 28(%esp)                  # 4-byte Spill
	movl	8(%esp), %eax                   # 4-byte Reload
	adcl	708(%esp), %eax
	movl	%eax, 8(%esp)                   # 4-byte Spill
	movl	(%esp), %esi                    # 4-byte Reload
	adcl	712(%esp), %esi
	movl	44(%esp), %eax                  # 4-byte Reload
	adcl	716(%esp), %eax
	movl	%eax, 44(%esp)                  # 4-byte Spill
	adcl	720(%esp), %edi
	movl	%edi, 20(%esp)                  # 4-byte Spill
	adcl	724(%esp), %ebp
	movl	%ebp, 36(%esp)                  # 4-byte Spill
	movl	32(%esp), %eax                  # 4-byte Reload
	adcl	728(%esp), %eax
	movl	%eax, 32(%esp)                  # 4-byte Spill
	movl	16(%esp), %eax                  # 4-byte Reload
	adcl	732(%esp), %eax
	movl	%eax, 16(%esp)                  # 4-byte Spill
	movl	12(%esp), %eax                  # 4-byte Reload
	adcl	736(%esp), %eax
	movl	%eax, 12(%esp)                  # 4-byte Spill
	movl	48(%esp), %edi                  # 4-byte Reload
	adcl	740(%esp), %edi
	movl	40(%esp), %eax                  # 4-byte Reload
	adcl	744(%esp), %eax
	movl	%eax, 40(%esp)                  # 4-byte Spill
	setb	%al
	subl	$4, %esp
	movl	56(%esp), %ecx                  # 4-byte Reload
	imull	%edx, %ecx
	movl	%edx, %ebp
	movzbl	%al, %eax
	movl	%eax, 8(%esp)                   # 4-byte Spill
	pushl	%ecx
	pushl	1460(%esp)
	leal	652(%esp), %eax
	pushl	%eax
	calll	mulPv384x32@PLT
	addl	$12, %esp
	addl	640(%esp), %ebp
	movl	24(%esp), %eax                  # 4-byte Reload
	adcl	644(%esp), %eax
	movl	%eax, 24(%esp)                  # 4-byte Spill
	movl	28(%esp), %eax                  # 4-byte Reload
	adcl	648(%esp), %eax
	movl	%eax, 28(%esp)                  # 4-byte Spill
	movl	8(%esp), %eax                   # 4-byte Reload
	adcl	652(%esp), %eax
	movl	%eax, 8(%esp)                   # 4-byte Spill
	adcl	656(%esp), %esi
	movl	%esi, (%esp)                    # 4-byte Spill
	movl	44(%esp), %eax                  # 4-byte Reload
	adcl	660(%esp), %eax
	movl	%eax, 44(%esp)                  # 4-byte Spill
	movl	20(%esp), %eax                  # 4-byte Reload
	adcl	664(%esp), %eax
	movl	%eax, 20(%esp)                  # 4-byte Spill
	movl	36(%esp), %eax                  # 4-byte Reload
	adcl	668(%esp), %eax
	movl	%eax, 36(%esp)                  # 4-byte Spill
	movl	32(%esp), %eax                  # 4-byte Reload
	adcl	672(%esp), %eax
	movl	%eax, 32(%esp)                  # 4-byte Spill
	movl	16(%esp), %eax                  # 4-byte Reload
	adcl	676(%esp), %eax
	movl	%eax, 16(%esp)                  # 4-byte Spill
	movl	12(%esp), %eax                  # 4-byte Reload
	adcl	680(%esp), %eax
	movl	%eax, 12(%esp)                  # 4-byte Spill
	adcl	684(%esp), %edi
	movl	%edi, 48(%esp)                  # 4-byte Spill
	movl	40(%esp), %ebp                  # 4-byte Reload
	adcl	688(%esp), %ebp
	movl	4(%esp), %edi                   # 4-byte Reload
	adcl	$0, %edi
	subl	$4, %esp
	movl	1452(%esp), %eax
	pushl	28(%eax)
	pushl	1452(%esp)
	leal	596(%esp), %eax
	pushl	%eax
	calll	mulPv384x32@PLT
	addl	$12, %esp
	movl	24(%esp), %edx                  # 4-byte Reload
	addl	584(%esp), %edx
	movl	28(%esp), %esi                  # 4-byte Reload
	adcl	588(%esp), %esi
	movl	8(%esp), %eax                   # 4-byte Reload
	adcl	592(%esp), %eax
	movl	%eax, 8(%esp)                   # 4-byte Spill
	movl	(%esp), %eax                    # 4-byte Reload
	adcl	596(%esp), %eax
	movl	%eax, (%esp)                    # 4-byte Spill
	movl	44(%esp), %eax                  # 4-byte Reload
	adcl	600(%esp), %eax
	movl	%eax, 44(%esp)                  # 4-byte Spill
	movl	20(%esp), %eax                  # 4-byte Reload
	adcl	604(%esp), %eax
	movl	%eax, 20(%esp)                  # 4-byte Spill
	movl	36(%esp), %eax                  # 4-byte Reload
	adcl	608(%esp), %eax
	movl	%eax, 36(%esp)                  # 4-byte Spill
	movl	32(%esp), %eax                  # 4-byte Reload
	adcl	612(%esp), %eax
	movl	%eax, 32(%esp)                  # 4-byte Spill
	movl	16(%esp), %eax                  # 4-byte Reload
	adcl	616(%esp), %eax
	movl	%eax, 16(%esp)                  # 4-byte Spill
	movl	12(%esp), %eax                  # 4-byte Reload
	adcl	620(%esp), %eax
	movl	%eax, 12(%esp)                  # 4-byte Spill
	movl	48(%esp), %eax                  # 4-byte Reload
	adcl	624(%esp), %eax
	movl	%eax, 48(%esp)                  # 4-byte Spill
	adcl	628(%esp), %ebp
	movl	%ebp, 40(%esp)                  # 4-byte Spill
	adcl	632(%esp), %edi
	movl	%edi, 4(%esp)                   # 4-byte Spill
	setb	%al
	subl	$4, %esp
	movl	56(%esp), %ecx                  # 4-byte Reload
	imull	%edx, %ecx
	movl	%edx, %edi
	movzbl	%al, %eax
	movl	%eax, 28(%esp)                  # 4-byte Spill
	pushl	%ecx
	pushl	1460(%esp)
	leal	540(%esp), %eax
	pushl	%eax
	calll	mulPv384x32@PLT
	addl	$12, %esp
	addl	528(%esp), %edi
	adcl	532(%esp), %esi
	movl	%esi, 28(%esp)                  # 4-byte Spill
	movl	8(%esp), %eax                   # 4-byte Reload
	adcl	536(%esp), %eax
	movl	%eax, 8(%esp)                   # 4-byte Spill
	movl	(%esp), %eax                    # 4-byte Reload
	adcl	540(%esp), %eax
	movl	%eax, (%esp)                    # 4-byte Spill
	movl	44(%esp), %eax                  # 4-byte Reload
	adcl	544(%esp), %eax
	movl	%eax, 44(%esp)                  # 4-byte Spill
	movl	20(%esp), %eax                  # 4-byte Reload
	adcl	548(%esp), %eax
	movl	%eax, 20(%esp)                  # 4-byte Spill
	movl	36(%esp), %eax                  # 4-byte Reload
	adcl	552(%esp), %eax
	movl	%eax, 36(%esp)                  # 4-byte Spill
	movl	32(%esp), %esi                  # 4-byte Reload
	adcl	556(%esp), %esi
	movl	16(%esp), %ebp                  # 4-byte Reload
	adcl	560(%esp), %ebp
	movl	12(%esp), %edi                  # 4-byte Reload
	adcl	564(%esp), %edi
	movl	48(%esp), %eax                  # 4-byte Reload
	adcl	568(%esp), %eax
	movl	%eax, 48(%esp)                  # 4-byte Spill
	movl	40(%esp), %eax                  # 4-byte Reload
	adcl	572(%esp), %eax
	movl	%eax, 40(%esp)                  # 4-byte Spill
	movl	4(%esp), %eax                   # 4-byte Reload
	adcl	576(%esp), %eax
	movl	%eax, 4(%esp)                   # 4-byte Spill
	adcl	$0, 24(%esp)                    # 4-byte Folded Spill
	subl	$4, %esp
	movl	1452(%esp), %eax
	pushl	32(%eax)
	pushl	1452(%esp)
	leal	484(%esp), %eax
	pushl	%eax
	calll	mulPv384x32@PLT
	addl	$12, %esp
	movl	28(%esp), %edx                  # 4-byte Reload
	addl	472(%esp), %edx
	movl	8(%esp), %eax                   # 4-byte Reload
	adcl	476(%esp), %eax
	movl	%eax, 8(%esp)                   # 4-byte Spill
	movl	(%esp), %eax                    # 4-byte Reload
	adcl	480(%esp), %eax
	movl	%eax, (%esp)                    # 4-byte Spill
	movl	44(%esp), %eax                  # 4-byte Reload
	adcl	484(%esp), %eax
	movl	%eax, 44(%esp)                  # 4-byte Spill
	movl	20(%esp), %eax                  # 4-byte Reload
	adcl	488(%esp), %eax
	movl	%eax, 20(%esp)                  # 4-byte Spill
	movl	36(%esp), %eax                  # 4-byte Reload
	adcl	492(%esp), %eax
	movl	%eax, 36(%esp)                  # 4-byte Spill
	adcl	496(%esp), %esi
	movl	%esi, 32(%esp)                  # 4-byte Spill
	adcl	500(%esp), %ebp
	movl	%ebp, 16(%esp)                  # 4-byte Spill
	adcl	504(%esp), %edi
	movl	%edi, 12(%esp)                  # 4-byte Spill
	movl	48(%esp), %ebp                  # 4-byte Reload
	adcl	508(%esp), %ebp
	movl	40(%esp), %eax                  # 4-byte Reload
	adcl	512(%esp), %eax
	movl	%eax, 40(%esp)                  # 4-byte Spill
	movl	4(%esp), %eax                   # 4-byte Reload
	adcl	516(%esp), %eax
	movl	%eax, 4(%esp)                   # 4-byte Spill
	movl	24(%esp), %eax                  # 4-byte Reload
	adcl	520(%esp), %eax
	movl	%eax, 24(%esp)                  # 4-byte Spill
	setb	%al
	subl	$4, %esp
	movl	56(%esp), %ecx                  # 4-byte Reload
	imull	%edx, %ecx
	movl	%edx, %esi
	movzbl	%al, %eax
	movl	%eax, 32(%esp)                  # 4-byte Spill
	pushl	%ecx
	pushl	1460(%esp)
	leal	428(%esp), %eax
	pushl	%eax
	calll	mulPv384x32@PLT
	addl	$12, %esp
	addl	416(%esp), %esi
	movl	8(%esp), %edi                   # 4-byte Reload
	adcl	420(%esp), %edi
	movl	(%esp), %eax                    # 4-byte Reload
	adcl	424(%esp), %eax
	movl	%eax, (%esp)                    # 4-byte Spill
	movl	44(%esp), %eax                  # 4-byte Reload
	adcl	428(%esp), %eax
	movl	%eax, 44(%esp)                  # 4-byte Spill
	movl	20(%esp), %eax                  # 4-byte Reload
	adcl	432(%esp), %eax
	movl	%eax, 20(%esp)                  # 4-byte Spill
	movl	36(%esp), %eax                  # 4-byte Reload
	adcl	436(%esp), %eax
	movl	%eax, 36(%esp)                  # 4-byte Spill
	movl	32(%esp), %eax                  # 4-byte Reload
	adcl	440(%esp), %eax
	movl	%eax, 32(%esp)                  # 4-byte Spill
	movl	16(%esp), %esi                  # 4-byte Reload
	adcl	444(%esp), %esi
	movl	12(%esp), %eax                  # 4-byte Reload
	adcl	448(%esp), %eax
	movl	%eax, 12(%esp)                  # 4-byte Spill
	adcl	452(%esp), %ebp
	movl	%ebp, 48(%esp)                  # 4-byte Spill
	movl	40(%esp), %eax                  # 4-byte Reload
	adcl	456(%esp), %eax
	movl	%eax, 40(%esp)                  # 4-byte Spill
	movl	4(%esp), %eax                   # 4-byte Reload
	adcl	460(%esp), %eax
	movl	%eax, 4(%esp)                   # 4-byte Spill
	movl	24(%esp), %ebp                  # 4-byte Reload
	adcl	464(%esp), %ebp
	adcl	$0, 28(%esp)                    # 4-byte Folded Spill
	subl	$4, %esp
	movl	1452(%esp), %eax
	pushl	36(%eax)
	pushl	1452(%esp)
	leal	372(%esp), %eax
	pushl	%eax
	calll	mulPv384x32@PLT
	addl	$12, %esp
	movl	%edi, %edx
	addl	360(%esp), %edx
	movl	(%esp), %eax                    # 4-byte Reload
	adcl	364(%esp), %eax
	movl	%eax, (%esp)                    # 4-byte Spill
	movl	44(%esp), %edi                  # 4-byte Reload
	adcl	368(%esp), %edi
	movl	20(%esp), %eax                  # 4-byte Reload
	adcl	372(%esp), %eax
	movl	%eax, 20(%esp)                  # 4-byte Spill
	movl	36(%esp), %eax                  # 4-byte Reload
	adcl	376(%esp), %eax
	movl	%eax, 36(%esp)                  # 4-byte Spill
	movl	32(%esp), %eax                  # 4-byte Reload
	adcl	380(%esp), %eax
	movl	%eax, 32(%esp)                  # 4-byte Spill
	adcl	384(%esp), %esi
	movl	%esi, 16(%esp)                  # 4-byte Spill
	movl	12(%esp), %eax                  # 4-byte Reload
	adcl	388(%esp), %eax
	movl	%eax, 12(%esp)                  # 4-byte Spill
	movl	48(%esp), %eax                  # 4-byte Reload
	adcl	392(%esp), %eax
	movl	%eax, 48(%esp)                  # 4-byte Spill
	movl	40(%esp), %eax                  # 4-byte Reload
	adcl	396(%esp), %eax
	movl	%eax, 40(%esp)                  # 4-byte Spill
	movl	4(%esp), %eax                   # 4-byte Reload
	adcl	400(%esp), %eax
	movl	%eax, 4(%esp)                   # 4-byte Spill
	adcl	404(%esp), %ebp
	movl	%ebp, 24(%esp)                  # 4-byte Spill
	movl	28(%esp), %ebp                  # 4-byte Reload
	adcl	408(%esp), %ebp
	setb	%al
	subl	$4, %esp
	movl	56(%esp), %ecx                  # 4-byte Reload
	imull	%edx, %ecx
	movl	%edx, %esi
	movzbl	%al, %eax
	movl	%eax, 12(%esp)                  # 4-byte Spill
	pushl	%ecx
	pushl	1460(%esp)
	leal	316(%esp), %eax
	pushl	%eax
	calll	mulPv384x32@PLT
	addl	$12, %esp
	addl	304(%esp), %esi
	movl	(%esp), %eax                    # 4-byte Reload
	adcl	308(%esp), %eax
	movl	%eax, (%esp)                    # 4-byte Spill
	adcl	312(%esp), %edi
	movl	%edi, 44(%esp)                  # 4-byte Spill
	movl	20(%esp), %esi                  # 4-byte Reload
	adcl	316(%esp), %esi
	movl	36(%esp), %edi                  # 4-byte Reload
	adcl	320(%esp), %edi
	movl	32(%esp), %eax                  # 4-byte Reload
	adcl	324(%esp), %eax
	movl	%eax, 32(%esp)                  # 4-byte Spill
	movl	16(%esp), %eax                  # 4-byte Reload
	adcl	328(%esp), %eax
	movl	%eax, 16(%esp)                  # 4-byte Spill
	movl	12(%esp), %eax                  # 4-byte Reload
	adcl	332(%esp), %eax
	movl	%eax, 12(%esp)                  # 4-byte Spill
	movl	48(%esp), %eax                  # 4-byte Reload
	adcl	336(%esp), %eax
	movl	%eax, 48(%esp)                  # 4-byte Spill
	movl	40(%esp), %eax                  # 4-byte Reload
	adcl	340(%esp), %eax
	movl	%eax, 40(%esp)                  # 4-byte Spill
	movl	4(%esp), %eax                   # 4-byte Reload
	adcl	344(%esp), %eax
	movl	%eax, 4(%esp)                   # 4-byte Spill
	movl	24(%esp), %eax                  # 4-byte Reload
	adcl	348(%esp), %eax
	movl	%eax, 24(%esp)                  # 4-byte Spill
	adcl	352(%esp), %ebp
	movl	%ebp, 28(%esp)                  # 4-byte Spill
	movl	8(%esp), %ebp                   # 4-byte Reload
	adcl	$0, %ebp
	subl	$4, %esp
	movl	1452(%esp), %eax
	pushl	40(%eax)
	movl	1452(%esp), %eax
	pushl	%eax
	leal	260(%esp), %eax
	pushl	%eax
	calll	mulPv384x32@PLT
	addl	$12, %esp
	movl	(%esp), %ecx                    # 4-byte Reload
	addl	248(%esp), %ecx
	movl	44(%esp), %eax                  # 4-byte Reload
	adcl	252(%esp), %eax
	movl	%eax, 44(%esp)                  # 4-byte Spill
	adcl	256(%esp), %esi
	movl	%esi, 20(%esp)                  # 4-byte Spill
	adcl	260(%esp), %edi
	movl	%edi, 36(%esp)                  # 4-byte Spill
	movl	32(%esp), %eax                  # 4-byte Reload
	adcl	264(%esp), %eax
	movl	%eax, 32(%esp)                  # 4-byte Spill
	movl	16(%esp), %eax                  # 4-byte Reload
	adcl	268(%esp), %eax
	movl	%eax, 16(%esp)                  # 4-byte Spill
	movl	12(%esp), %eax                  # 4-byte Reload
	adcl	272(%esp), %eax
	movl	%eax, 12(%esp)                  # 4-byte Spill
	movl	48(%esp), %eax                  # 4-byte Reload
	adcl	276(%esp), %eax
	movl	%eax, 48(%esp)                  # 4-byte Spill
	movl	40(%esp), %edi                  # 4-byte Reload
	adcl	280(%esp), %edi
	movl	4(%esp), %eax                   # 4-byte Reload
	adcl	284(%esp), %eax
	movl	%eax, 4(%esp)                   # 4-byte Spill
	movl	24(%esp), %eax                  # 4-byte Reload
	adcl	288(%esp), %eax
	movl	%eax, 24(%esp)                  # 4-byte Spill
	movl	28(%esp), %eax                  # 4-byte Reload
	adcl	292(%esp), %eax
	movl	%eax, 28(%esp)                  # 4-byte Spill
	adcl	296(%esp), %ebp
	movl	%ebp, 8(%esp)                   # 4-byte Spill
	setb	(%esp)                          # 1-byte Folded Spill
	subl	$4, %esp
	movl	56(%esp), %eax                  # 4-byte Reload
	imull	%ecx, %eax
	movl	%ecx, %esi
	pushl	%eax
	pushl	1460(%esp)
	leal	204(%esp), %eax
	pushl	%eax
	calll	mulPv384x32@PLT
	addl	$12, %esp
	addl	192(%esp), %esi
	movl	44(%esp), %ebp                  # 4-byte Reload
	adcl	196(%esp), %ebp
	movl	20(%esp), %eax                  # 4-byte Reload
	adcl	200(%esp), %eax
	movl	%eax, 20(%esp)                  # 4-byte Spill
	movl	36(%esp), %eax                  # 4-byte Reload
	adcl	204(%esp), %eax
	movl	%eax, 36(%esp)                  # 4-byte Spill
	movl	32(%esp), %eax                  # 4-byte Reload
	adcl	208(%esp), %eax
	movl	%eax, 32(%esp)                  # 4-byte Spill
	movl	16(%esp), %eax                  # 4-byte Reload
	adcl	212(%esp), %eax
	movl	%eax, 16(%esp)                  # 4-byte Spill
	movl	12(%esp), %eax                  # 4-byte Reload
	adcl	216(%esp), %eax
	movl	%eax, 12(%esp)                  # 4-byte Spill
	movl	48(%esp), %eax                  # 4-byte Reload
	adcl	220(%esp), %eax
	movl	%eax, 48(%esp)                  # 4-byte Spill
	adcl	224(%esp), %edi
	movl	%edi, 40(%esp)                  # 4-byte Spill
	movl	4(%esp), %esi                   # 4-byte Reload
	adcl	228(%esp), %esi
	movl	24(%esp), %eax                  # 4-byte Reload
	adcl	232(%esp), %eax
	movl	%eax, 24(%esp)                  # 4-byte Spill
	movl	28(%esp), %eax                  # 4-byte Reload
	adcl	236(%esp), %eax
	movl	%eax, 28(%esp)                  # 4-byte Spill
	movl	8(%esp), %eax                   # 4-byte Reload
	adcl	240(%esp), %eax
	movl	%eax, 8(%esp)                   # 4-byte Spill
	movzbl	(%esp), %eax                    # 1-byte Folded Reload
	adcl	$0, %eax
	movl	%eax, (%esp)                    # 4-byte Spill
	subl	$4, %esp
	leal	140(%esp), %eax
	movl	1452(%esp), %ecx
	pushl	44(%ecx)
	pushl	1452(%esp)
	pushl	%eax
	calll	mulPv384x32@PLT
	addl	$12, %esp
	movl	%ebp, %edx
	addl	136(%esp), %edx
	movl	20(%esp), %eax                  # 4-byte Reload
	adcl	140(%esp), %eax
	movl	%eax, 20(%esp)                  # 4-byte Spill
	movl	36(%esp), %eax                  # 4-byte Reload
	adcl	144(%esp), %eax
	movl	%eax, 36(%esp)                  # 4-byte Spill
	movl	32(%esp), %eax                  # 4-byte Reload
	adcl	148(%esp), %eax
	movl	%eax, 32(%esp)                  # 4-byte Spill
	movl	16(%esp), %eax                  # 4-byte Reload
	adcl	152(%esp), %eax
	movl	%eax, 16(%esp)                  # 4-byte Spill
	movl	12(%esp), %ebp                  # 4-byte Reload
	adcl	156(%esp), %ebp
	movl	48(%esp), %edi                  # 4-byte Reload
	adcl	160(%esp), %edi
	movl	40(%esp), %eax                  # 4-byte Reload
	adcl	164(%esp), %eax
	movl	%eax, 40(%esp)                  # 4-byte Spill
	adcl	168(%esp), %esi
	movl	%esi, 4(%esp)                   # 4-byte Spill
	movl	24(%esp), %eax                  # 4-byte Reload
	adcl	172(%esp), %eax
	movl	%eax, 24(%esp)                  # 4-byte Spill
	movl	28(%esp), %eax                  # 4-byte Reload
	adcl	176(%esp), %eax
	movl	%eax, 28(%esp)                  # 4-byte Spill
	movl	8(%esp), %eax                   # 4-byte Reload
	adcl	180(%esp), %eax
	movl	%eax, 8(%esp)                   # 4-byte Spill
	movl	(%esp), %eax                    # 4-byte Reload
	adcl	184(%esp), %eax
	movl	%eax, (%esp)                    # 4-byte Spill
	setb	44(%esp)                        # 1-byte Folded Spill
	subl	$4, %esp
	leal	84(%esp), %eax
	movl	56(%esp), %ecx                  # 4-byte Reload
	imull	%edx, %ecx
	movl	%edx, %esi
	pushl	%ecx
	pushl	1460(%esp)
	pushl	%eax
	calll	mulPv384x32@PLT
	addl	$12, %esp
	addl	80(%esp), %esi
	movzbl	44(%esp), %eax                  # 1-byte Folded Reload
	movl	%eax, 44(%esp)                  # 4-byte Spill
	movl	20(%esp), %eax                  # 4-byte Reload
	adcl	84(%esp), %eax
	movl	36(%esp), %ecx                  # 4-byte Reload
	adcl	88(%esp), %ecx
	movl	%ecx, 36(%esp)                  # 4-byte Spill
	movl	32(%esp), %esi                  # 4-byte Reload
	adcl	92(%esp), %esi
	movl	16(%esp), %edx                  # 4-byte Reload
	adcl	96(%esp), %edx
	adcl	100(%esp), %ebp
	movl	%ebp, 12(%esp)                  # 4-byte Spill
	adcl	104(%esp), %edi
	movl	40(%esp), %ecx                  # 4-byte Reload
	adcl	108(%esp), %ecx
	movl	%ecx, 40(%esp)                  # 4-byte Spill
	movl	4(%esp), %ecx                   # 4-byte Reload
	adcl	112(%esp), %ecx
	movl	%ecx, 4(%esp)                   # 4-byte Spill
	movl	24(%esp), %ebx                  # 4-byte Reload
	adcl	116(%esp), %ebx
	movl	28(%esp), %ebp                  # 4-byte Reload
	adcl	120(%esp), %ebp
	movl	8(%esp), %ecx                   # 4-byte Reload
	adcl	124(%esp), %ecx
	movl	%ecx, 8(%esp)                   # 4-byte Spill
	movl	(%esp), %ecx                    # 4-byte Reload
	adcl	128(%esp), %ecx
	movl	%ecx, (%esp)                    # 4-byte Spill
	adcl	$0, 44(%esp)                    # 4-byte Folded Spill
	movl	%eax, 20(%esp)                  # 4-byte Spill
	movl	1452(%esp), %ecx
	subl	(%ecx), %eax
	movl	%eax, 52(%esp)                  # 4-byte Spill
	movl	36(%esp), %eax                  # 4-byte Reload
	sbbl	4(%ecx), %eax
	movl	%eax, 76(%esp)                  # 4-byte Spill
	movl	%esi, %eax
	movl	%esi, 32(%esp)                  # 4-byte Spill
	sbbl	8(%ecx), %eax
	movl	%eax, 72(%esp)                  # 4-byte Spill
	movl	%edx, %eax
	movl	%edx, 16(%esp)                  # 4-byte Spill
	sbbl	12(%ecx), %eax
	movl	%eax, 68(%esp)                  # 4-byte Spill
	movl	12(%esp), %eax                  # 4-byte Reload
	sbbl	16(%ecx), %eax
	movl	%eax, 64(%esp)                  # 4-byte Spill
	movl	%edi, 48(%esp)                  # 4-byte Spill
	sbbl	20(%ecx), %edi
	movl	%edi, 60(%esp)                  # 4-byte Spill
	movl	40(%esp), %eax                  # 4-byte Reload
	sbbl	24(%ecx), %eax
	movl	%eax, 56(%esp)                  # 4-byte Spill
	movl	4(%esp), %eax                   # 4-byte Reload
	sbbl	28(%ecx), %eax
	movl	%ebx, %edi
	movl	(%esp), %ebx                    # 4-byte Reload
	movl	%edi, 24(%esp)                  # 4-byte Spill
	sbbl	32(%ecx), %edi
	movl	%ebp, 28(%esp)                  # 4-byte Spill
	sbbl	36(%ecx), %ebp
	movl	%ecx, %edx
	movl	8(%esp), %ecx                   # 4-byte Reload
	sbbl	40(%edx), %ecx
	movl	%ebx, %esi
	sbbl	44(%edx), %esi
	movl	44(%esp), %edx                  # 4-byte Reload
	sbbl	$0, %edx
	testb	$1, %dl
	jne	.LBB60_1
# %bb.2:
	movl	1440(%esp), %ebx
	movl	%esi, 44(%ebx)
	jne	.LBB60_3
.LBB60_4:
	movl	%ecx, 40(%ebx)
	movl	68(%esp), %esi                  # 4-byte Reload
	jne	.LBB60_5
.LBB60_6:
	movl	%ebp, 36(%ebx)
	movl	76(%esp), %ecx                  # 4-byte Reload
	jne	.LBB60_7
.LBB60_8:
	movl	%edi, 32(%ebx)
	jne	.LBB60_9
.LBB60_10:
	movl	%eax, 28(%ebx)
	movl	64(%esp), %edi                  # 4-byte Reload
	movl	56(%esp), %eax                  # 4-byte Reload
	jne	.LBB60_11
.LBB60_12:
	movl	%eax, 24(%ebx)
	movl	52(%esp), %eax                  # 4-byte Reload
	movl	60(%esp), %edx                  # 4-byte Reload
	jne	.LBB60_13
.LBB60_14:
	movl	%edx, 20(%ebx)
	movl	72(%esp), %edx                  # 4-byte Reload
	jne	.LBB60_15
.LBB60_16:
	movl	%edi, 16(%ebx)
	jne	.LBB60_17
.LBB60_18:
	movl	%esi, 12(%ebx)
	jne	.LBB60_19
.LBB60_20:
	movl	%edx, 8(%ebx)
	jne	.LBB60_21
.LBB60_22:
	movl	%ecx, 4(%ebx)
	je	.LBB60_24
.LBB60_23:
	movl	20(%esp), %eax                  # 4-byte Reload
.LBB60_24:
	movl	%eax, (%ebx)
	addl	$1420, %esp                     # imm = 0x58C
	popl	%esi
	popl	%edi
	popl	%ebx
	popl	%ebp
	retl
.LBB60_1:
	movl	%ebx, %esi
	movl	1440(%esp), %ebx
	movl	%esi, 44(%ebx)
	je	.LBB60_4
.LBB60_3:
	movl	8(%esp), %ecx                   # 4-byte Reload
	movl	%ecx, 40(%ebx)
	movl	68(%esp), %esi                  # 4-byte Reload
	je	.LBB60_6
.LBB60_5:
	movl	28(%esp), %ebp                  # 4-byte Reload
	movl	%ebp, 36(%ebx)
	movl	76(%esp), %ecx                  # 4-byte Reload
	je	.LBB60_8
.LBB60_7:
	movl	24(%esp), %edi                  # 4-byte Reload
	movl	%edi, 32(%ebx)
	je	.LBB60_10
.LBB60_9:
	movl	4(%esp), %eax                   # 4-byte Reload
	movl	%eax, 28(%ebx)
	movl	64(%esp), %edi                  # 4-byte Reload
	movl	56(%esp), %eax                  # 4-byte Reload
	je	.LBB60_12
.LBB60_11:
	movl	40(%esp), %eax                  # 4-byte Reload
	movl	%eax, 24(%ebx)
	movl	52(%esp), %eax                  # 4-byte Reload
	movl	60(%esp), %edx                  # 4-byte Reload
	je	.LBB60_14
.LBB60_13:
	movl	48(%esp), %edx                  # 4-byte Reload
	movl	%edx, 20(%ebx)
	movl	72(%esp), %edx                  # 4-byte Reload
	je	.LBB60_16
.LBB60_15:
	movl	12(%esp), %edi                  # 4-byte Reload
	movl	%edi, 16(%ebx)
	je	.LBB60_18
.LBB60_17:
	movl	16(%esp), %esi                  # 4-byte Reload
	movl	%esi, 12(%ebx)
	je	.LBB60_20
.LBB60_19:
	movl	32(%esp), %edx                  # 4-byte Reload
	movl	%edx, 8(%ebx)
	je	.LBB60_22
.LBB60_21:
	movl	36(%esp), %ecx                  # 4-byte Reload
	movl	%ecx, 4(%ebx)
	jne	.LBB60_23
	jmp	.LBB60_24
.Lfunc_end60:
	.size	mcl_fp_mont12L, .Lfunc_end60-mcl_fp_mont12L
                                        # -- End function
	.globl	mcl_fp_montNF12L                # -- Begin function mcl_fp_montNF12L
	.p2align	4, 0x90
	.type	mcl_fp_montNF12L,@function
mcl_fp_montNF12L:                       # @mcl_fp_montNF12L
# %bb.0:
	pushl	%ebp
	pushl	%ebx
	pushl	%edi
	pushl	%esi
	subl	$1420, %esp                     # imm = 0x58C
	calll	.L61$pb
.L61$pb:
	popl	%ebx
.Ltmp13:
	addl	$_GLOBAL_OFFSET_TABLE_+(.Ltmp13-.L61$pb), %ebx
	movl	1452(%esp), %eax
	movl	-4(%eax), %edi
	movl	%edi, 60(%esp)                  # 4-byte Spill
	movl	1448(%esp), %ecx
	subl	$4, %esp
	leal	1372(%esp), %eax
	pushl	(%ecx)
	pushl	1452(%esp)
	pushl	%eax
	calll	mulPv384x32@PLT
	addl	$12, %esp
	movl	1368(%esp), %esi
	movl	1372(%esp), %eax
	movl	%eax, 36(%esp)                  # 4-byte Spill
	movl	%edi, %eax
	imull	%esi, %eax
	movl	1416(%esp), %ecx
	movl	%ecx, 40(%esp)                  # 4-byte Spill
	movl	1412(%esp), %ecx
	movl	%ecx, 28(%esp)                  # 4-byte Spill
	movl	1408(%esp), %ecx
	movl	%ecx, 20(%esp)                  # 4-byte Spill
	movl	1404(%esp), %ecx
	movl	%ecx, 16(%esp)                  # 4-byte Spill
	movl	1400(%esp), %ecx
	movl	%ecx, 8(%esp)                   # 4-byte Spill
	movl	1396(%esp), %ecx
	movl	%ecx, 12(%esp)                  # 4-byte Spill
	movl	1392(%esp), %ecx
	movl	%ecx, 52(%esp)                  # 4-byte Spill
	movl	1388(%esp), %ecx
	movl	%ecx, 24(%esp)                  # 4-byte Spill
	movl	1384(%esp), %ecx
	movl	%ecx, 32(%esp)                  # 4-byte Spill
	movl	1380(%esp), %ebp
	movl	1376(%esp), %edi
	subl	$4, %esp
	leal	1316(%esp), %ecx
	pushl	%eax
	pushl	1460(%esp)
	pushl	%ecx
	calll	mulPv384x32@PLT
	addl	$12, %esp
	addl	1312(%esp), %esi
	movl	36(%esp), %eax                  # 4-byte Reload
	adcl	1316(%esp), %eax
	movl	%eax, 36(%esp)                  # 4-byte Spill
	adcl	1320(%esp), %edi
	adcl	1324(%esp), %ebp
	movl	%ebp, 44(%esp)                  # 4-byte Spill
	movl	32(%esp), %eax                  # 4-byte Reload
	adcl	1328(%esp), %eax
	movl	%eax, 32(%esp)                  # 4-byte Spill
	movl	24(%esp), %eax                  # 4-byte Reload
	adcl	1332(%esp), %eax
	movl	%eax, 24(%esp)                  # 4-byte Spill
	movl	52(%esp), %eax                  # 4-byte Reload
	adcl	1336(%esp), %eax
	movl	%eax, 52(%esp)                  # 4-byte Spill
	movl	12(%esp), %eax                  # 4-byte Reload
	adcl	1340(%esp), %eax
	movl	%eax, 12(%esp)                  # 4-byte Spill
	movl	8(%esp), %eax                   # 4-byte Reload
	adcl	1344(%esp), %eax
	movl	%eax, 8(%esp)                   # 4-byte Spill
	movl	16(%esp), %eax                  # 4-byte Reload
	adcl	1348(%esp), %eax
	movl	%eax, 16(%esp)                  # 4-byte Spill
	movl	20(%esp), %esi                  # 4-byte Reload
	adcl	1352(%esp), %esi
	movl	28(%esp), %ebp                  # 4-byte Reload
	adcl	1356(%esp), %ebp
	movl	40(%esp), %eax                  # 4-byte Reload
	adcl	1360(%esp), %eax
	movl	%eax, 40(%esp)                  # 4-byte Spill
	subl	$4, %esp
	leal	1260(%esp), %eax
	movl	1452(%esp), %ecx
	pushl	4(%ecx)
	pushl	1452(%esp)
	pushl	%eax
	calll	mulPv384x32@PLT
	addl	$12, %esp
	movl	1304(%esp), %eax
	movl	36(%esp), %edx                  # 4-byte Reload
	addl	1256(%esp), %edx
	adcl	1260(%esp), %edi
	movl	%edi, 36(%esp)                  # 4-byte Spill
	movl	44(%esp), %ecx                  # 4-byte Reload
	adcl	1264(%esp), %ecx
	movl	%ecx, 44(%esp)                  # 4-byte Spill
	movl	32(%esp), %ecx                  # 4-byte Reload
	adcl	1268(%esp), %ecx
	movl	%ecx, 32(%esp)                  # 4-byte Spill
	movl	24(%esp), %ecx                  # 4-byte Reload
	adcl	1272(%esp), %ecx
	movl	%ecx, 24(%esp)                  # 4-byte Spill
	movl	52(%esp), %ecx                  # 4-byte Reload
	adcl	1276(%esp), %ecx
	movl	%ecx, 52(%esp)                  # 4-byte Spill
	movl	12(%esp), %ecx                  # 4-byte Reload
	adcl	1280(%esp), %ecx
	movl	%ecx, 12(%esp)                  # 4-byte Spill
	movl	8(%esp), %ecx                   # 4-byte Reload
	adcl	1284(%esp), %ecx
	movl	%ecx, 8(%esp)                   # 4-byte Spill
	movl	16(%esp), %ecx                  # 4-byte Reload
	adcl	1288(%esp), %ecx
	movl	%ecx, 16(%esp)                  # 4-byte Spill
	adcl	1292(%esp), %esi
	movl	%esi, 20(%esp)                  # 4-byte Spill
	adcl	1296(%esp), %ebp
	movl	%ebp, 28(%esp)                  # 4-byte Spill
	movl	40(%esp), %ebp                  # 4-byte Reload
	adcl	1300(%esp), %ebp
	adcl	$0, %eax
	movl	%eax, %edi
	subl	$4, %esp
	leal	1204(%esp), %eax
	movl	64(%esp), %ecx                  # 4-byte Reload
	imull	%edx, %ecx
	movl	%edx, %esi
	pushl	%ecx
	pushl	1460(%esp)
	pushl	%eax
	calll	mulPv384x32@PLT
	addl	$12, %esp
	addl	1200(%esp), %esi
	movl	36(%esp), %eax                  # 4-byte Reload
	adcl	1204(%esp), %eax
	movl	%eax, 36(%esp)                  # 4-byte Spill
	movl	44(%esp), %eax                  # 4-byte Reload
	adcl	1208(%esp), %eax
	movl	%eax, 44(%esp)                  # 4-byte Spill
	movl	32(%esp), %eax                  # 4-byte Reload
	adcl	1212(%esp), %eax
	movl	%eax, 32(%esp)                  # 4-byte Spill
	movl	24(%esp), %eax                  # 4-byte Reload
	adcl	1216(%esp), %eax
	movl	%eax, 24(%esp)                  # 4-byte Spill
	movl	52(%esp), %eax                  # 4-byte Reload
	adcl	1220(%esp), %eax
	movl	%eax, 52(%esp)                  # 4-byte Spill
	movl	12(%esp), %eax                  # 4-byte Reload
	adcl	1224(%esp), %eax
	movl	%eax, 12(%esp)                  # 4-byte Spill
	movl	8(%esp), %eax                   # 4-byte Reload
	adcl	1228(%esp), %eax
	movl	%eax, 8(%esp)                   # 4-byte Spill
	movl	16(%esp), %eax                  # 4-byte Reload
	adcl	1232(%esp), %eax
	movl	%eax, 16(%esp)                  # 4-byte Spill
	movl	20(%esp), %eax                  # 4-byte Reload
	adcl	1236(%esp), %eax
	movl	%eax, 20(%esp)                  # 4-byte Spill
	movl	28(%esp), %eax                  # 4-byte Reload
	adcl	1240(%esp), %eax
	movl	%eax, 28(%esp)                  # 4-byte Spill
	adcl	1244(%esp), %ebp
	adcl	1248(%esp), %edi
	subl	$4, %esp
	leal	1148(%esp), %eax
	movl	1452(%esp), %ecx
	pushl	8(%ecx)
	pushl	1452(%esp)
	pushl	%eax
	calll	mulPv384x32@PLT
	addl	$12, %esp
	movl	1192(%esp), %eax
	movl	36(%esp), %edx                  # 4-byte Reload
	addl	1144(%esp), %edx
	movl	44(%esp), %ecx                  # 4-byte Reload
	adcl	1148(%esp), %ecx
	movl	%ecx, 44(%esp)                  # 4-byte Spill
	movl	32(%esp), %ecx                  # 4-byte Reload
	adcl	1152(%esp), %ecx
	movl	%ecx, 32(%esp)                  # 4-byte Spill
	movl	24(%esp), %ecx                  # 4-byte Reload
	adcl	1156(%esp), %ecx
	movl	%ecx, 24(%esp)                  # 4-byte Spill
	movl	52(%esp), %ecx                  # 4-byte Reload
	adcl	1160(%esp), %ecx
	movl	%ecx, 52(%esp)                  # 4-byte Spill
	movl	12(%esp), %ecx                  # 4-byte Reload
	adcl	1164(%esp), %ecx
	movl	%ecx, 12(%esp)                  # 4-byte Spill
	movl	8(%esp), %ecx                   # 4-byte Reload
	adcl	1168(%esp), %ecx
	movl	%ecx, 8(%esp)                   # 4-byte Spill
	movl	16(%esp), %ecx                  # 4-byte Reload
	adcl	1172(%esp), %ecx
	movl	%ecx, 16(%esp)                  # 4-byte Spill
	movl	20(%esp), %ecx                  # 4-byte Reload
	adcl	1176(%esp), %ecx
	movl	%ecx, 20(%esp)                  # 4-byte Spill
	movl	28(%esp), %ecx                  # 4-byte Reload
	adcl	1180(%esp), %ecx
	movl	%ecx, 28(%esp)                  # 4-byte Spill
	adcl	1184(%esp), %ebp
	movl	%ebp, 40(%esp)                  # 4-byte Spill
	adcl	1188(%esp), %edi
	movl	%edi, %esi
	movl	%eax, %ebp
	adcl	$0, %ebp
	subl	$4, %esp
	leal	1092(%esp), %eax
	movl	64(%esp), %ecx                  # 4-byte Reload
	imull	%edx, %ecx
	movl	%edx, %edi
	pushl	%ecx
	movl	1460(%esp), %ecx
	pushl	%ecx
	pushl	%eax
	calll	mulPv384x32@PLT
	addl	$12, %esp
	addl	1088(%esp), %edi
	movl	44(%esp), %eax                  # 4-byte Reload
	adcl	1092(%esp), %eax
	movl	%eax, 44(%esp)                  # 4-byte Spill
	movl	32(%esp), %eax                  # 4-byte Reload
	adcl	1096(%esp), %eax
	movl	%eax, 32(%esp)                  # 4-byte Spill
	movl	24(%esp), %eax                  # 4-byte Reload
	adcl	1100(%esp), %eax
	movl	%eax, 24(%esp)                  # 4-byte Spill
	movl	52(%esp), %eax                  # 4-byte Reload
	adcl	1104(%esp), %eax
	movl	%eax, 52(%esp)                  # 4-byte Spill
	movl	12(%esp), %eax                  # 4-byte Reload
	adcl	1108(%esp), %eax
	movl	%eax, 12(%esp)                  # 4-byte Spill
	movl	8(%esp), %eax                   # 4-byte Reload
	adcl	1112(%esp), %eax
	movl	%eax, 8(%esp)                   # 4-byte Spill
	movl	16(%esp), %eax                  # 4-byte Reload
	adcl	1116(%esp), %eax
	movl	%eax, 16(%esp)                  # 4-byte Spill
	movl	20(%esp), %eax                  # 4-byte Reload
	adcl	1120(%esp), %eax
	movl	%eax, 20(%esp)                  # 4-byte Spill
	movl	28(%esp), %edi                  # 4-byte Reload
	adcl	1124(%esp), %edi
	movl	40(%esp), %eax                  # 4-byte Reload
	adcl	1128(%esp), %eax
	movl	%eax, 40(%esp)                  # 4-byte Spill
	adcl	1132(%esp), %esi
	movl	%esi, 36(%esp)                  # 4-byte Spill
	adcl	1136(%esp), %ebp
	movl	%ebp, 48(%esp)                  # 4-byte Spill
	subl	$4, %esp
	leal	1036(%esp), %eax
	movl	1452(%esp), %ecx
	pushl	12(%ecx)
	pushl	1452(%esp)
	pushl	%eax
	calll	mulPv384x32@PLT
	addl	$12, %esp
	movl	1080(%esp), %eax
	movl	44(%esp), %edx                  # 4-byte Reload
	addl	1032(%esp), %edx
	movl	32(%esp), %ecx                  # 4-byte Reload
	adcl	1036(%esp), %ecx
	movl	%ecx, 32(%esp)                  # 4-byte Spill
	movl	24(%esp), %ecx                  # 4-byte Reload
	adcl	1040(%esp), %ecx
	movl	%ecx, 24(%esp)                  # 4-byte Spill
	movl	52(%esp), %ecx                  # 4-byte Reload
	adcl	1044(%esp), %ecx
	movl	%ecx, 52(%esp)                  # 4-byte Spill
	movl	12(%esp), %ecx                  # 4-byte Reload
	adcl	1048(%esp), %ecx
	movl	%ecx, 12(%esp)                  # 4-byte Spill
	movl	8(%esp), %ecx                   # 4-byte Reload
	adcl	1052(%esp), %ecx
	movl	%ecx, 8(%esp)                   # 4-byte Spill
	movl	16(%esp), %esi                  # 4-byte Reload
	adcl	1056(%esp), %esi
	movl	20(%esp), %ebp                  # 4-byte Reload
	adcl	1060(%esp), %ebp
	adcl	1064(%esp), %edi
	movl	%edi, 28(%esp)                  # 4-byte Spill
	movl	40(%esp), %ecx                  # 4-byte Reload
	adcl	1068(%esp), %ecx
	movl	%ecx, 40(%esp)                  # 4-byte Spill
	movl	36(%esp), %ecx                  # 4-byte Reload
	adcl	1072(%esp), %ecx
	movl	%ecx, 36(%esp)                  # 4-byte Spill
	movl	48(%esp), %ecx                  # 4-byte Reload
	adcl	1076(%esp), %ecx
	movl	%ecx, 48(%esp)                  # 4-byte Spill
	adcl	$0, %eax
	movl	%eax, 44(%esp)                  # 4-byte Spill
	subl	$4, %esp
	leal	980(%esp), %eax
	movl	64(%esp), %ecx                  # 4-byte Reload
	movl	%edx, %edi
	imull	%edx, %ecx
	pushl	%ecx
	pushl	1460(%esp)
	pushl	%eax
	calll	mulPv384x32@PLT
	addl	$12, %esp
	addl	976(%esp), %edi
	movl	32(%esp), %eax                  # 4-byte Reload
	adcl	980(%esp), %eax
	movl	%eax, 32(%esp)                  # 4-byte Spill
	movl	24(%esp), %eax                  # 4-byte Reload
	adcl	984(%esp), %eax
	movl	%eax, 24(%esp)                  # 4-byte Spill
	movl	52(%esp), %eax                  # 4-byte Reload
	adcl	988(%esp), %eax
	movl	%eax, 52(%esp)                  # 4-byte Spill
	movl	12(%esp), %eax                  # 4-byte Reload
	adcl	992(%esp), %eax
	movl	%eax, 12(%esp)                  # 4-byte Spill
	movl	8(%esp), %edi                   # 4-byte Reload
	adcl	996(%esp), %edi
	adcl	1000(%esp), %esi
	movl	%esi, 16(%esp)                  # 4-byte Spill
	adcl	1004(%esp), %ebp
	movl	%ebp, 20(%esp)                  # 4-byte Spill
	movl	28(%esp), %ebp                  # 4-byte Reload
	adcl	1008(%esp), %ebp
	movl	40(%esp), %eax                  # 4-byte Reload
	adcl	1012(%esp), %eax
	movl	%eax, 40(%esp)                  # 4-byte Spill
	movl	36(%esp), %eax                  # 4-byte Reload
	adcl	1016(%esp), %eax
	movl	%eax, 36(%esp)                  # 4-byte Spill
	movl	48(%esp), %esi                  # 4-byte Reload
	adcl	1020(%esp), %esi
	movl	44(%esp), %eax                  # 4-byte Reload
	adcl	1024(%esp), %eax
	movl	%eax, 44(%esp)                  # 4-byte Spill
	subl	$4, %esp
	leal	924(%esp), %eax
	movl	1452(%esp), %ecx
	pushl	16(%ecx)
	pushl	1452(%esp)
	pushl	%eax
	calll	mulPv384x32@PLT
	addl	$12, %esp
	movl	968(%esp), %ecx
	movl	32(%esp), %eax                  # 4-byte Reload
	addl	920(%esp), %eax
	movl	24(%esp), %edx                  # 4-byte Reload
	adcl	924(%esp), %edx
	movl	%edx, 24(%esp)                  # 4-byte Spill
	movl	52(%esp), %edx                  # 4-byte Reload
	adcl	928(%esp), %edx
	movl	%edx, 52(%esp)                  # 4-byte Spill
	movl	12(%esp), %edx                  # 4-byte Reload
	adcl	932(%esp), %edx
	movl	%edx, 12(%esp)                  # 4-byte Spill
	adcl	936(%esp), %edi
	movl	%edi, 8(%esp)                   # 4-byte Spill
	movl	16(%esp), %edx                  # 4-byte Reload
	adcl	940(%esp), %edx
	movl	%edx, 16(%esp)                  # 4-byte Spill
	movl	20(%esp), %edx                  # 4-byte Reload
	adcl	944(%esp), %edx
	movl	%edx, 20(%esp)                  # 4-byte Spill
	adcl	948(%esp), %ebp
	movl	%ebp, 28(%esp)                  # 4-byte Spill
	movl	40(%esp), %edx                  # 4-byte Reload
	adcl	952(%esp), %edx
	movl	%edx, 40(%esp)                  # 4-byte Spill
	movl	36(%esp), %edx                  # 4-byte Reload
	adcl	956(%esp), %edx
	movl	%edx, 36(%esp)                  # 4-byte Spill
	adcl	960(%esp), %esi
	movl	%esi, 48(%esp)                  # 4-byte Spill
	movl	44(%esp), %edi                  # 4-byte Reload
	adcl	964(%esp), %edi
	movl	%ecx, %ebp
	adcl	$0, %ebp
	subl	$4, %esp
	movl	64(%esp), %ecx                  # 4-byte Reload
	movl	%eax, %esi
	imull	%eax, %ecx
	pushl	%ecx
	pushl	1460(%esp)
	leal	876(%esp), %eax
	pushl	%eax
	calll	mulPv384x32@PLT
	addl	$12, %esp
	addl	864(%esp), %esi
	movl	24(%esp), %eax                  # 4-byte Reload
	adcl	868(%esp), %eax
	movl	%eax, 24(%esp)                  # 4-byte Spill
	movl	52(%esp), %esi                  # 4-byte Reload
	adcl	872(%esp), %esi
	movl	12(%esp), %eax                  # 4-byte Reload
	adcl	876(%esp), %eax
	movl	%eax, 12(%esp)                  # 4-byte Spill
	movl	8(%esp), %eax                   # 4-byte Reload
	adcl	880(%esp), %eax
	movl	%eax, 8(%esp)                   # 4-byte Spill
	movl	16(%esp), %eax                  # 4-byte Reload
	adcl	884(%esp), %eax
	movl	%eax, 16(%esp)                  # 4-byte Spill
	movl	20(%esp), %eax                  # 4-byte Reload
	adcl	888(%esp), %eax
	movl	%eax, 20(%esp)                  # 4-byte Spill
	movl	28(%esp), %eax                  # 4-byte Reload
	adcl	892(%esp), %eax
	movl	%eax, 28(%esp)                  # 4-byte Spill
	movl	40(%esp), %eax                  # 4-byte Reload
	adcl	896(%esp), %eax
	movl	%eax, 40(%esp)                  # 4-byte Spill
	movl	36(%esp), %eax                  # 4-byte Reload
	adcl	900(%esp), %eax
	movl	%eax, 36(%esp)                  # 4-byte Spill
	movl	48(%esp), %eax                  # 4-byte Reload
	adcl	904(%esp), %eax
	movl	%eax, 48(%esp)                  # 4-byte Spill
	adcl	908(%esp), %edi
	movl	%edi, 44(%esp)                  # 4-byte Spill
	adcl	912(%esp), %ebp
	movl	%ebp, 32(%esp)                  # 4-byte Spill
	subl	$4, %esp
	movl	1452(%esp), %eax
	pushl	20(%eax)
	movl	1452(%esp), %eax
	pushl	%eax
	leal	820(%esp), %eax
	pushl	%eax
	calll	mulPv384x32@PLT
	addl	$12, %esp
	movl	856(%esp), %eax
	movl	24(%esp), %edx                  # 4-byte Reload
	addl	808(%esp), %edx
	movl	%edx, 24(%esp)                  # 4-byte Spill
	adcl	812(%esp), %esi
	movl	12(%esp), %ecx                  # 4-byte Reload
	adcl	816(%esp), %ecx
	movl	%ecx, 12(%esp)                  # 4-byte Spill
	movl	8(%esp), %ecx                   # 4-byte Reload
	adcl	820(%esp), %ecx
	movl	%ecx, 8(%esp)                   # 4-byte Spill
	movl	16(%esp), %ecx                  # 4-byte Reload
	adcl	824(%esp), %ecx
	movl	%ecx, 16(%esp)                  # 4-byte Spill
	movl	20(%esp), %ecx                  # 4-byte Reload
	adcl	828(%esp), %ecx
	movl	%ecx, 20(%esp)                  # 4-byte Spill
	movl	28(%esp), %ecx                  # 4-byte Reload
	adcl	832(%esp), %ecx
	movl	%ecx, 28(%esp)                  # 4-byte Spill
	movl	40(%esp), %ecx                  # 4-byte Reload
	adcl	836(%esp), %ecx
	movl	%ecx, 40(%esp)                  # 4-byte Spill
	movl	36(%esp), %edi                  # 4-byte Reload
	adcl	840(%esp), %edi
	movl	48(%esp), %ecx                  # 4-byte Reload
	adcl	844(%esp), %ecx
	movl	%ecx, 48(%esp)                  # 4-byte Spill
	movl	44(%esp), %ebp                  # 4-byte Reload
	adcl	848(%esp), %ebp
	movl	32(%esp), %ecx                  # 4-byte Reload
	adcl	852(%esp), %ecx
	movl	%ecx, 32(%esp)                  # 4-byte Spill
	adcl	$0, %eax
	movl	%eax, 56(%esp)                  # 4-byte Spill
	subl	$4, %esp
	movl	64(%esp), %ecx                  # 4-byte Reload
	imull	%edx, %ecx
	pushl	%ecx
	pushl	1460(%esp)
	leal	764(%esp), %eax
	pushl	%eax
	calll	mulPv384x32@PLT
	addl	$12, %esp
	movl	24(%esp), %eax                  # 4-byte Reload
	addl	752(%esp), %eax
	adcl	756(%esp), %esi
	movl	%esi, 52(%esp)                  # 4-byte Spill
	movl	12(%esp), %esi                  # 4-byte Reload
	adcl	760(%esp), %esi
	movl	8(%esp), %eax                   # 4-byte Reload
	adcl	764(%esp), %eax
	movl	%eax, 8(%esp)                   # 4-byte Spill
	movl	16(%esp), %eax                  # 4-byte Reload
	adcl	768(%esp), %eax
	movl	%eax, 16(%esp)                  # 4-byte Spill
	movl	20(%esp), %eax                  # 4-byte Reload
	adcl	772(%esp), %eax
	movl	%eax, 20(%esp)                  # 4-byte Spill
	movl	28(%esp), %eax                  # 4-byte Reload
	adcl	776(%esp), %eax
	movl	%eax, 28(%esp)                  # 4-byte Spill
	movl	40(%esp), %eax                  # 4-byte Reload
	adcl	780(%esp), %eax
	movl	%eax, 40(%esp)                  # 4-byte Spill
	adcl	784(%esp), %edi
	movl	%edi, 36(%esp)                  # 4-byte Spill
	movl	48(%esp), %eax                  # 4-byte Reload
	adcl	788(%esp), %eax
	movl	%eax, 48(%esp)                  # 4-byte Spill
	adcl	792(%esp), %ebp
	movl	32(%esp), %edi                  # 4-byte Reload
	adcl	796(%esp), %edi
	movl	56(%esp), %eax                  # 4-byte Reload
	adcl	800(%esp), %eax
	movl	%eax, 56(%esp)                  # 4-byte Spill
	subl	$4, %esp
	movl	1452(%esp), %eax
	pushl	24(%eax)
	pushl	1452(%esp)
	leal	708(%esp), %eax
	pushl	%eax
	calll	mulPv384x32@PLT
	addl	$12, %esp
	movl	744(%esp), %ecx
	movl	52(%esp), %eax                  # 4-byte Reload
	addl	696(%esp), %eax
	adcl	700(%esp), %esi
	movl	%esi, 12(%esp)                  # 4-byte Spill
	movl	8(%esp), %edx                   # 4-byte Reload
	adcl	704(%esp), %edx
	movl	%edx, 8(%esp)                   # 4-byte Spill
	movl	16(%esp), %edx                  # 4-byte Reload
	adcl	708(%esp), %edx
	movl	%edx, 16(%esp)                  # 4-byte Spill
	movl	20(%esp), %edx                  # 4-byte Reload
	adcl	712(%esp), %edx
	movl	%edx, 20(%esp)                  # 4-byte Spill
	movl	28(%esp), %edx                  # 4-byte Reload
	adcl	716(%esp), %edx
	movl	%edx, 28(%esp)                  # 4-byte Spill
	movl	40(%esp), %edx                  # 4-byte Reload
	adcl	720(%esp), %edx
	movl	%edx, 40(%esp)                  # 4-byte Spill
	movl	36(%esp), %edx                  # 4-byte Reload
	adcl	724(%esp), %edx
	movl	%edx, 36(%esp)                  # 4-byte Spill
	movl	48(%esp), %edx                  # 4-byte Reload
	adcl	728(%esp), %edx
	movl	%edx, 48(%esp)                  # 4-byte Spill
	adcl	732(%esp), %ebp
	movl	%ebp, 44(%esp)                  # 4-byte Spill
	adcl	736(%esp), %edi
	movl	%edi, 32(%esp)                  # 4-byte Spill
	movl	56(%esp), %esi                  # 4-byte Reload
	adcl	740(%esp), %esi
	adcl	$0, %ecx
	movl	%ecx, %ebp
	subl	$4, %esp
	movl	64(%esp), %ecx                  # 4-byte Reload
	movl	%eax, %edi
	imull	%eax, %ecx
	pushl	%ecx
	pushl	1460(%esp)
	leal	652(%esp), %eax
	pushl	%eax
	calll	mulPv384x32@PLT
	addl	$12, %esp
	addl	640(%esp), %edi
	movl	12(%esp), %eax                  # 4-byte Reload
	adcl	644(%esp), %eax
	movl	%eax, 12(%esp)                  # 4-byte Spill
	movl	8(%esp), %eax                   # 4-byte Reload
	adcl	648(%esp), %eax
	movl	%eax, 8(%esp)                   # 4-byte Spill
	movl	16(%esp), %eax                  # 4-byte Reload
	adcl	652(%esp), %eax
	movl	%eax, 16(%esp)                  # 4-byte Spill
	movl	20(%esp), %eax                  # 4-byte Reload
	adcl	656(%esp), %eax
	movl	%eax, 20(%esp)                  # 4-byte Spill
	movl	28(%esp), %edi                  # 4-byte Reload
	adcl	660(%esp), %edi
	movl	40(%esp), %eax                  # 4-byte Reload
	adcl	664(%esp), %eax
	movl	%eax, 40(%esp)                  # 4-byte Spill
	movl	36(%esp), %eax                  # 4-byte Reload
	adcl	668(%esp), %eax
	movl	%eax, 36(%esp)                  # 4-byte Spill
	movl	48(%esp), %eax                  # 4-byte Reload
	adcl	672(%esp), %eax
	movl	%eax, 48(%esp)                  # 4-byte Spill
	movl	44(%esp), %eax                  # 4-byte Reload
	adcl	676(%esp), %eax
	movl	%eax, 44(%esp)                  # 4-byte Spill
	movl	32(%esp), %eax                  # 4-byte Reload
	adcl	680(%esp), %eax
	movl	%eax, 32(%esp)                  # 4-byte Spill
	adcl	684(%esp), %esi
	movl	%esi, 56(%esp)                  # 4-byte Spill
	adcl	688(%esp), %ebp
	movl	%ebp, 24(%esp)                  # 4-byte Spill
	subl	$4, %esp
	movl	1452(%esp), %eax
	pushl	28(%eax)
	pushl	1452(%esp)
	leal	596(%esp), %eax
	pushl	%eax
	calll	mulPv384x32@PLT
	addl	$12, %esp
	movl	632(%esp), %eax
	movl	12(%esp), %edx                  # 4-byte Reload
	addl	584(%esp), %edx
	movl	8(%esp), %ecx                   # 4-byte Reload
	adcl	588(%esp), %ecx
	movl	%ecx, 8(%esp)                   # 4-byte Spill
	movl	16(%esp), %ecx                  # 4-byte Reload
	adcl	592(%esp), %ecx
	movl	%ecx, 16(%esp)                  # 4-byte Spill
	movl	20(%esp), %esi                  # 4-byte Reload
	adcl	596(%esp), %esi
	adcl	600(%esp), %edi
	movl	%edi, 28(%esp)                  # 4-byte Spill
	movl	40(%esp), %ecx                  # 4-byte Reload
	adcl	604(%esp), %ecx
	movl	%ecx, 40(%esp)                  # 4-byte Spill
	movl	36(%esp), %ecx                  # 4-byte Reload
	adcl	608(%esp), %ecx
	movl	%ecx, 36(%esp)                  # 4-byte Spill
	movl	48(%esp), %ebp                  # 4-byte Reload
	adcl	612(%esp), %ebp
	movl	44(%esp), %ecx                  # 4-byte Reload
	adcl	616(%esp), %ecx
	movl	%ecx, 44(%esp)                  # 4-byte Spill
	movl	32(%esp), %ecx                  # 4-byte Reload
	adcl	620(%esp), %ecx
	movl	%ecx, 32(%esp)                  # 4-byte Spill
	movl	56(%esp), %ecx                  # 4-byte Reload
	adcl	624(%esp), %ecx
	movl	%ecx, 56(%esp)                  # 4-byte Spill
	movl	24(%esp), %ecx                  # 4-byte Reload
	adcl	628(%esp), %ecx
	movl	%ecx, 24(%esp)                  # 4-byte Spill
	adcl	$0, %eax
	movl	%eax, 12(%esp)                  # 4-byte Spill
	subl	$4, %esp
	movl	64(%esp), %ecx                  # 4-byte Reload
	imull	%edx, %ecx
	movl	%edx, %edi
	pushl	%ecx
	pushl	1460(%esp)
	leal	540(%esp), %eax
	pushl	%eax
	calll	mulPv384x32@PLT
	addl	$12, %esp
	addl	528(%esp), %edi
	movl	8(%esp), %eax                   # 4-byte Reload
	adcl	532(%esp), %eax
	movl	%eax, 8(%esp)                   # 4-byte Spill
	movl	16(%esp), %eax                  # 4-byte Reload
	adcl	536(%esp), %eax
	movl	%eax, 16(%esp)                  # 4-byte Spill
	adcl	540(%esp), %esi
	movl	%esi, 20(%esp)                  # 4-byte Spill
	movl	28(%esp), %eax                  # 4-byte Reload
	adcl	544(%esp), %eax
	movl	%eax, 28(%esp)                  # 4-byte Spill
	movl	40(%esp), %edi                  # 4-byte Reload
	adcl	548(%esp), %edi
	movl	36(%esp), %eax                  # 4-byte Reload
	adcl	552(%esp), %eax
	movl	%eax, 36(%esp)                  # 4-byte Spill
	adcl	556(%esp), %ebp
	movl	%ebp, 48(%esp)                  # 4-byte Spill
	movl	44(%esp), %esi                  # 4-byte Reload
	adcl	560(%esp), %esi
	movl	32(%esp), %eax                  # 4-byte Reload
	adcl	564(%esp), %eax
	movl	%eax, 32(%esp)                  # 4-byte Spill
	movl	56(%esp), %eax                  # 4-byte Reload
	adcl	568(%esp), %eax
	movl	%eax, 56(%esp)                  # 4-byte Spill
	movl	24(%esp), %eax                  # 4-byte Reload
	adcl	572(%esp), %eax
	movl	%eax, 24(%esp)                  # 4-byte Spill
	movl	12(%esp), %eax                  # 4-byte Reload
	adcl	576(%esp), %eax
	movl	%eax, 12(%esp)                  # 4-byte Spill
	subl	$4, %esp
	movl	1452(%esp), %eax
	pushl	32(%eax)
	pushl	1452(%esp)
	leal	484(%esp), %eax
	pushl	%eax
	calll	mulPv384x32@PLT
	addl	$12, %esp
	movl	520(%esp), %ecx
	movl	8(%esp), %edx                   # 4-byte Reload
	addl	472(%esp), %edx
	movl	16(%esp), %ebp                  # 4-byte Reload
	adcl	476(%esp), %ebp
	movl	20(%esp), %eax                  # 4-byte Reload
	adcl	480(%esp), %eax
	movl	%eax, 20(%esp)                  # 4-byte Spill
	movl	28(%esp), %eax                  # 4-byte Reload
	adcl	484(%esp), %eax
	movl	%eax, 28(%esp)                  # 4-byte Spill
	adcl	488(%esp), %edi
	movl	%edi, 40(%esp)                  # 4-byte Spill
	movl	36(%esp), %edi                  # 4-byte Reload
	adcl	492(%esp), %edi
	movl	%edi, 36(%esp)                  # 4-byte Spill
	movl	48(%esp), %eax                  # 4-byte Reload
	adcl	496(%esp), %eax
	movl	%eax, 48(%esp)                  # 4-byte Spill
	adcl	500(%esp), %esi
	movl	%esi, 44(%esp)                  # 4-byte Spill
	movl	32(%esp), %esi                  # 4-byte Reload
	adcl	504(%esp), %esi
	movl	56(%esp), %eax                  # 4-byte Reload
	adcl	508(%esp), %eax
	movl	%eax, 56(%esp)                  # 4-byte Spill
	movl	24(%esp), %eax                  # 4-byte Reload
	adcl	512(%esp), %eax
	movl	%eax, 24(%esp)                  # 4-byte Spill
	movl	12(%esp), %eax                  # 4-byte Reload
	adcl	516(%esp), %eax
	movl	%eax, 12(%esp)                  # 4-byte Spill
	adcl	$0, %ecx
	movl	%ecx, 8(%esp)                   # 4-byte Spill
	subl	$4, %esp
	movl	64(%esp), %ecx                  # 4-byte Reload
	imull	%edx, %ecx
	movl	%edx, %edi
	pushl	%ecx
	pushl	1460(%esp)
	leal	428(%esp), %eax
	pushl	%eax
	calll	mulPv384x32@PLT
	addl	$12, %esp
	addl	416(%esp), %edi
	adcl	420(%esp), %ebp
	movl	%ebp, 16(%esp)                  # 4-byte Spill
	movl	20(%esp), %eax                  # 4-byte Reload
	adcl	424(%esp), %eax
	movl	%eax, 20(%esp)                  # 4-byte Spill
	movl	28(%esp), %eax                  # 4-byte Reload
	adcl	428(%esp), %eax
	movl	%eax, 28(%esp)                  # 4-byte Spill
	movl	40(%esp), %eax                  # 4-byte Reload
	adcl	432(%esp), %eax
	movl	%eax, 40(%esp)                  # 4-byte Spill
	movl	36(%esp), %eax                  # 4-byte Reload
	adcl	436(%esp), %eax
	movl	%eax, 36(%esp)                  # 4-byte Spill
	movl	48(%esp), %edi                  # 4-byte Reload
	adcl	440(%esp), %edi
	movl	44(%esp), %eax                  # 4-byte Reload
	adcl	444(%esp), %eax
	movl	%eax, 44(%esp)                  # 4-byte Spill
	adcl	448(%esp), %esi
	movl	56(%esp), %ebp                  # 4-byte Reload
	adcl	452(%esp), %ebp
	movl	24(%esp), %eax                  # 4-byte Reload
	adcl	456(%esp), %eax
	movl	%eax, 24(%esp)                  # 4-byte Spill
	movl	12(%esp), %eax                  # 4-byte Reload
	adcl	460(%esp), %eax
	movl	%eax, 12(%esp)                  # 4-byte Spill
	movl	8(%esp), %eax                   # 4-byte Reload
	adcl	464(%esp), %eax
	movl	%eax, 8(%esp)                   # 4-byte Spill
	subl	$4, %esp
	movl	1452(%esp), %eax
	pushl	36(%eax)
	pushl	1452(%esp)
	leal	372(%esp), %eax
	pushl	%eax
	calll	mulPv384x32@PLT
	addl	$12, %esp
	movl	408(%esp), %eax
	movl	16(%esp), %edx                  # 4-byte Reload
	addl	360(%esp), %edx
	movl	20(%esp), %ecx                  # 4-byte Reload
	adcl	364(%esp), %ecx
	movl	%ecx, 20(%esp)                  # 4-byte Spill
	movl	28(%esp), %ecx                  # 4-byte Reload
	adcl	368(%esp), %ecx
	movl	%ecx, 28(%esp)                  # 4-byte Spill
	movl	40(%esp), %ecx                  # 4-byte Reload
	adcl	372(%esp), %ecx
	movl	%ecx, 40(%esp)                  # 4-byte Spill
	movl	36(%esp), %ecx                  # 4-byte Reload
	adcl	376(%esp), %ecx
	movl	%ecx, 36(%esp)                  # 4-byte Spill
	adcl	380(%esp), %edi
	movl	%edi, 48(%esp)                  # 4-byte Spill
	movl	44(%esp), %ecx                  # 4-byte Reload
	adcl	384(%esp), %ecx
	movl	%ecx, 44(%esp)                  # 4-byte Spill
	adcl	388(%esp), %esi
	movl	%esi, 32(%esp)                  # 4-byte Spill
	adcl	392(%esp), %ebp
	movl	24(%esp), %ecx                  # 4-byte Reload
	adcl	396(%esp), %ecx
	movl	%ecx, 24(%esp)                  # 4-byte Spill
	movl	12(%esp), %ecx                  # 4-byte Reload
	adcl	400(%esp), %ecx
	movl	%ecx, 12(%esp)                  # 4-byte Spill
	movl	8(%esp), %ecx                   # 4-byte Reload
	adcl	404(%esp), %ecx
	movl	%ecx, 8(%esp)                   # 4-byte Spill
	adcl	$0, %eax
	movl	%eax, 16(%esp)                  # 4-byte Spill
	subl	$4, %esp
	movl	64(%esp), %ecx                  # 4-byte Reload
	imull	%edx, %ecx
	movl	%edx, %esi
	pushl	%ecx
	pushl	1460(%esp)
	leal	316(%esp), %eax
	pushl	%eax
	calll	mulPv384x32@PLT
	addl	$12, %esp
	addl	304(%esp), %esi
	movl	20(%esp), %eax                  # 4-byte Reload
	adcl	308(%esp), %eax
	movl	%eax, 20(%esp)                  # 4-byte Spill
	movl	28(%esp), %eax                  # 4-byte Reload
	adcl	312(%esp), %eax
	movl	%eax, 28(%esp)                  # 4-byte Spill
	movl	40(%esp), %esi                  # 4-byte Reload
	adcl	316(%esp), %esi
	movl	36(%esp), %edi                  # 4-byte Reload
	adcl	320(%esp), %edi
	movl	48(%esp), %eax                  # 4-byte Reload
	adcl	324(%esp), %eax
	movl	%eax, 48(%esp)                  # 4-byte Spill
	movl	44(%esp), %eax                  # 4-byte Reload
	adcl	328(%esp), %eax
	movl	%eax, 44(%esp)                  # 4-byte Spill
	movl	32(%esp), %eax                  # 4-byte Reload
	adcl	332(%esp), %eax
	movl	%eax, 32(%esp)                  # 4-byte Spill
	adcl	336(%esp), %ebp
	movl	24(%esp), %eax                  # 4-byte Reload
	adcl	340(%esp), %eax
	movl	%eax, 24(%esp)                  # 4-byte Spill
	movl	12(%esp), %eax                  # 4-byte Reload
	adcl	344(%esp), %eax
	movl	%eax, 12(%esp)                  # 4-byte Spill
	movl	8(%esp), %eax                   # 4-byte Reload
	adcl	348(%esp), %eax
	movl	%eax, 8(%esp)                   # 4-byte Spill
	movl	16(%esp), %eax                  # 4-byte Reload
	adcl	352(%esp), %eax
	movl	%eax, 16(%esp)                  # 4-byte Spill
	subl	$4, %esp
	leal	252(%esp), %eax
	movl	1452(%esp), %ecx
	pushl	40(%ecx)
	pushl	1452(%esp)
	pushl	%eax
	calll	mulPv384x32@PLT
	addl	$12, %esp
	movl	296(%esp), %ecx
	movl	20(%esp), %edx                  # 4-byte Reload
	addl	248(%esp), %edx
	movl	28(%esp), %eax                  # 4-byte Reload
	adcl	252(%esp), %eax
	movl	%eax, 28(%esp)                  # 4-byte Spill
	adcl	256(%esp), %esi
	movl	%esi, 40(%esp)                  # 4-byte Spill
	adcl	260(%esp), %edi
	movl	%edi, 36(%esp)                  # 4-byte Spill
	movl	48(%esp), %eax                  # 4-byte Reload
	adcl	264(%esp), %eax
	movl	%eax, 48(%esp)                  # 4-byte Spill
	movl	44(%esp), %edi                  # 4-byte Reload
	adcl	268(%esp), %edi
	movl	32(%esp), %eax                  # 4-byte Reload
	adcl	272(%esp), %eax
	movl	%eax, 32(%esp)                  # 4-byte Spill
	adcl	276(%esp), %ebp
	movl	%ebp, 56(%esp)                  # 4-byte Spill
	movl	24(%esp), %ebp                  # 4-byte Reload
	adcl	280(%esp), %ebp
	movl	12(%esp), %eax                  # 4-byte Reload
	adcl	284(%esp), %eax
	movl	%eax, 12(%esp)                  # 4-byte Spill
	movl	8(%esp), %eax                   # 4-byte Reload
	adcl	288(%esp), %eax
	movl	%eax, 8(%esp)                   # 4-byte Spill
	movl	16(%esp), %eax                  # 4-byte Reload
	adcl	292(%esp), %eax
	movl	%eax, 16(%esp)                  # 4-byte Spill
	adcl	$0, %ecx
	movl	%ecx, 20(%esp)                  # 4-byte Spill
	subl	$4, %esp
	leal	196(%esp), %eax
	movl	64(%esp), %ecx                  # 4-byte Reload
	imull	%edx, %ecx
	movl	%edx, %esi
	pushl	%ecx
	pushl	1460(%esp)
	pushl	%eax
	calll	mulPv384x32@PLT
	addl	$12, %esp
	addl	192(%esp), %esi
	movl	28(%esp), %eax                  # 4-byte Reload
	adcl	196(%esp), %eax
	movl	%eax, 28(%esp)                  # 4-byte Spill
	movl	40(%esp), %eax                  # 4-byte Reload
	adcl	200(%esp), %eax
	movl	%eax, 40(%esp)                  # 4-byte Spill
	movl	36(%esp), %esi                  # 4-byte Reload
	adcl	204(%esp), %esi
	movl	48(%esp), %eax                  # 4-byte Reload
	adcl	208(%esp), %eax
	movl	%eax, 48(%esp)                  # 4-byte Spill
	adcl	212(%esp), %edi
	movl	32(%esp), %eax                  # 4-byte Reload
	adcl	216(%esp), %eax
	movl	%eax, 32(%esp)                  # 4-byte Spill
	movl	56(%esp), %eax                  # 4-byte Reload
	adcl	220(%esp), %eax
	movl	%eax, 56(%esp)                  # 4-byte Spill
	adcl	224(%esp), %ebp
	movl	12(%esp), %eax                  # 4-byte Reload
	adcl	228(%esp), %eax
	movl	%eax, 12(%esp)                  # 4-byte Spill
	movl	8(%esp), %eax                   # 4-byte Reload
	adcl	232(%esp), %eax
	movl	%eax, 8(%esp)                   # 4-byte Spill
	movl	16(%esp), %eax                  # 4-byte Reload
	adcl	236(%esp), %eax
	movl	%eax, 16(%esp)                  # 4-byte Spill
	movl	20(%esp), %eax                  # 4-byte Reload
	adcl	240(%esp), %eax
	movl	%eax, 20(%esp)                  # 4-byte Spill
	subl	$4, %esp
	leal	140(%esp), %eax
	movl	1452(%esp), %ecx
	pushl	44(%ecx)
	pushl	1452(%esp)
	pushl	%eax
	calll	mulPv384x32@PLT
	addl	$12, %esp
	movl	28(%esp), %edx                  # 4-byte Reload
	addl	136(%esp), %edx
	movl	40(%esp), %eax                  # 4-byte Reload
	adcl	140(%esp), %eax
	movl	%eax, 40(%esp)                  # 4-byte Spill
	adcl	144(%esp), %esi
	movl	%esi, 36(%esp)                  # 4-byte Spill
	movl	48(%esp), %esi                  # 4-byte Reload
	adcl	148(%esp), %esi
	adcl	152(%esp), %edi
	movl	%edi, 44(%esp)                  # 4-byte Spill
	movl	32(%esp), %eax                  # 4-byte Reload
	adcl	156(%esp), %eax
	movl	%eax, 32(%esp)                  # 4-byte Spill
	movl	56(%esp), %eax                  # 4-byte Reload
	adcl	160(%esp), %eax
	movl	%eax, 56(%esp)                  # 4-byte Spill
	adcl	164(%esp), %ebp
	movl	%ebp, 24(%esp)                  # 4-byte Spill
	movl	12(%esp), %ebp                  # 4-byte Reload
	adcl	168(%esp), %ebp
	movl	8(%esp), %eax                   # 4-byte Reload
	adcl	172(%esp), %eax
	movl	%eax, 8(%esp)                   # 4-byte Spill
	movl	16(%esp), %eax                  # 4-byte Reload
	adcl	176(%esp), %eax
	movl	%eax, 16(%esp)                  # 4-byte Spill
	movl	20(%esp), %eax                  # 4-byte Reload
	adcl	180(%esp), %eax
	movl	%eax, 20(%esp)                  # 4-byte Spill
	movl	184(%esp), %eax
	adcl	$0, %eax
	movl	%eax, 28(%esp)                  # 4-byte Spill
	subl	$4, %esp
	leal	84(%esp), %eax
	movl	64(%esp), %ecx                  # 4-byte Reload
	imull	%edx, %ecx
	movl	%edx, %edi
	pushl	%ecx
	pushl	1460(%esp)
	pushl	%eax
	calll	mulPv384x32@PLT
	addl	$12, %esp
	addl	80(%esp), %edi
	movl	40(%esp), %eax                  # 4-byte Reload
	adcl	84(%esp), %eax
	movl	36(%esp), %ecx                  # 4-byte Reload
	adcl	88(%esp), %ecx
	adcl	92(%esp), %esi
	movl	%esi, 48(%esp)                  # 4-byte Spill
	movl	44(%esp), %esi                  # 4-byte Reload
	adcl	96(%esp), %esi
	movl	32(%esp), %edi                  # 4-byte Reload
	adcl	100(%esp), %edi
	movl	56(%esp), %ebx                  # 4-byte Reload
	adcl	104(%esp), %ebx
	movl	24(%esp), %edx                  # 4-byte Reload
	adcl	108(%esp), %edx
	movl	%edx, 24(%esp)                  # 4-byte Spill
	adcl	112(%esp), %ebp
	movl	%ebp, 12(%esp)                  # 4-byte Spill
	movl	8(%esp), %ebp                   # 4-byte Reload
	adcl	116(%esp), %ebp
	movl	16(%esp), %edx                  # 4-byte Reload
	adcl	120(%esp), %edx
	movl	%edx, 16(%esp)                  # 4-byte Spill
	movl	20(%esp), %edx                  # 4-byte Reload
	adcl	124(%esp), %edx
	movl	%edx, 20(%esp)                  # 4-byte Spill
	movl	28(%esp), %edx                  # 4-byte Reload
	adcl	128(%esp), %edx
	movl	%edx, 28(%esp)                  # 4-byte Spill
	movl	%eax, 40(%esp)                  # 4-byte Spill
	movl	%eax, %edx
	movl	1452(%esp), %eax
	subl	(%eax), %edx
	movl	%edx, 52(%esp)                  # 4-byte Spill
	movl	%ecx, 36(%esp)                  # 4-byte Spill
	sbbl	4(%eax), %ecx
	movl	%ecx, 60(%esp)                  # 4-byte Spill
	movl	48(%esp), %ecx                  # 4-byte Reload
	sbbl	8(%eax), %ecx
	movl	%ecx, 76(%esp)                  # 4-byte Spill
	movl	%esi, %ecx
	movl	%esi, 44(%esp)                  # 4-byte Spill
	sbbl	12(%eax), %ecx
	movl	%ecx, 72(%esp)                  # 4-byte Spill
	movl	%edi, %ecx
	movl	%edi, 32(%esp)                  # 4-byte Spill
	sbbl	16(%eax), %ecx
	movl	%ecx, 68(%esp)                  # 4-byte Spill
	movl	%ebx, %ecx
	movl	%ebx, 56(%esp)                  # 4-byte Spill
	sbbl	20(%eax), %ecx
	movl	%ecx, 64(%esp)                  # 4-byte Spill
	movl	24(%esp), %ebx                  # 4-byte Reload
	sbbl	24(%eax), %ebx
	movl	12(%esp), %ecx                  # 4-byte Reload
	sbbl	28(%eax), %ecx
	movl	%ebp, 8(%esp)                   # 4-byte Spill
	movl	%ebp, %edx
	sbbl	32(%eax), %edx
	movl	16(%esp), %ebp                  # 4-byte Reload
	sbbl	36(%eax), %ebp
	movl	20(%esp), %esi                  # 4-byte Reload
	sbbl	40(%eax), %esi
	movl	28(%esp), %edi                  # 4-byte Reload
	sbbl	44(%eax), %edi
	movl	%edi, %eax
	sarl	$31, %eax
	testl	%eax, %eax
	js	.LBB61_1
# %bb.2:
	movl	1440(%esp), %eax
	movl	%edi, 44(%eax)
	js	.LBB61_3
.LBB61_4:
	movl	%esi, 40(%eax)
	movl	72(%esp), %edi                  # 4-byte Reload
	js	.LBB61_5
.LBB61_6:
	movl	%ebp, 36(%eax)
	movl	76(%esp), %esi                  # 4-byte Reload
	js	.LBB61_7
.LBB61_8:
	movl	%edx, 32(%eax)
	js	.LBB61_9
.LBB61_10:
	movl	%ecx, 28(%eax)
	movl	60(%esp), %edx                  # 4-byte Reload
	js	.LBB61_11
.LBB61_12:
	movl	%ebx, 24(%eax)
	movl	52(%esp), %ecx                  # 4-byte Reload
	movl	64(%esp), %ebx                  # 4-byte Reload
	js	.LBB61_13
.LBB61_14:
	movl	%ebx, 20(%eax)
	movl	68(%esp), %ebx                  # 4-byte Reload
	js	.LBB61_15
.LBB61_16:
	movl	%ebx, 16(%eax)
	js	.LBB61_17
.LBB61_18:
	movl	%edi, 12(%eax)
	js	.LBB61_19
.LBB61_20:
	movl	%esi, 8(%eax)
	js	.LBB61_21
.LBB61_22:
	movl	%edx, 4(%eax)
	jns	.LBB61_24
.LBB61_23:
	movl	40(%esp), %ecx                  # 4-byte Reload
.LBB61_24:
	movl	%ecx, (%eax)
	addl	$1420, %esp                     # imm = 0x58C
	popl	%esi
	popl	%edi
	popl	%ebx
	popl	%ebp
	retl
.LBB61_1:
	movl	28(%esp), %edi                  # 4-byte Reload
	movl	1440(%esp), %eax
	movl	%edi, 44(%eax)
	jns	.LBB61_4
.LBB61_3:
	movl	20(%esp), %esi                  # 4-byte Reload
	movl	%esi, 40(%eax)
	movl	72(%esp), %edi                  # 4-byte Reload
	jns	.LBB61_6
.LBB61_5:
	movl	16(%esp), %ebp                  # 4-byte Reload
	movl	%ebp, 36(%eax)
	movl	76(%esp), %esi                  # 4-byte Reload
	jns	.LBB61_8
.LBB61_7:
	movl	8(%esp), %edx                   # 4-byte Reload
	movl	%edx, 32(%eax)
	jns	.LBB61_10
.LBB61_9:
	movl	12(%esp), %ecx                  # 4-byte Reload
	movl	%ecx, 28(%eax)
	movl	60(%esp), %edx                  # 4-byte Reload
	jns	.LBB61_12
.LBB61_11:
	movl	24(%esp), %ebx                  # 4-byte Reload
	movl	%ebx, 24(%eax)
	movl	52(%esp), %ecx                  # 4-byte Reload
	movl	64(%esp), %ebx                  # 4-byte Reload
	jns	.LBB61_14
.LBB61_13:
	movl	56(%esp), %ebx                  # 4-byte Reload
	movl	%ebx, 20(%eax)
	movl	68(%esp), %ebx                  # 4-byte Reload
	jns	.LBB61_16
.LBB61_15:
	movl	32(%esp), %ebx                  # 4-byte Reload
	movl	%ebx, 16(%eax)
	jns	.LBB61_18
.LBB61_17:
	movl	44(%esp), %edi                  # 4-byte Reload
	movl	%edi, 12(%eax)
	jns	.LBB61_20
.LBB61_19:
	movl	48(%esp), %esi                  # 4-byte Reload
	movl	%esi, 8(%eax)
	jns	.LBB61_22
.LBB61_21:
	movl	36(%esp), %edx                  # 4-byte Reload
	movl	%edx, 4(%eax)
	js	.LBB61_23
	jmp	.LBB61_24
.Lfunc_end61:
	.size	mcl_fp_montNF12L, .Lfunc_end61-mcl_fp_montNF12L
                                        # -- End function
	.globl	mcl_fp_montRed12L               # -- Begin function mcl_fp_montRed12L
	.p2align	4, 0x90
	.type	mcl_fp_montRed12L,@function
mcl_fp_montRed12L:                      # @mcl_fp_montRed12L
# %bb.0:
	pushl	%ebp
	pushl	%ebx
	pushl	%edi
	pushl	%esi
	subl	$780, %esp                      # imm = 0x30C
	calll	.L62$pb
.L62$pb:
	popl	%ebx
.Ltmp14:
	addl	$_GLOBAL_OFFSET_TABLE_+(.Ltmp14-.L62$pb), %ebx
	movl	808(%esp), %ecx
	movl	44(%ecx), %eax
	movl	%eax, 108(%esp)                 # 4-byte Spill
	movl	40(%ecx), %eax
	movl	%eax, 104(%esp)                 # 4-byte Spill
	movl	36(%ecx), %eax
	movl	%eax, 100(%esp)                 # 4-byte Spill
	movl	32(%ecx), %eax
	movl	%eax, 96(%esp)                  # 4-byte Spill
	movl	28(%ecx), %eax
	movl	%eax, 92(%esp)                  # 4-byte Spill
	movl	24(%ecx), %eax
	movl	%eax, 84(%esp)                  # 4-byte Spill
	movl	20(%ecx), %eax
	movl	%eax, 80(%esp)                  # 4-byte Spill
	movl	16(%ecx), %eax
	movl	%eax, 76(%esp)                  # 4-byte Spill
	movl	12(%ecx), %eax
	movl	%eax, 72(%esp)                  # 4-byte Spill
	movl	8(%ecx), %eax
	movl	%eax, 68(%esp)                  # 4-byte Spill
	movl	4(%ecx), %eax
	movl	%ecx, %edx
	movl	%eax, 64(%esp)                  # 4-byte Spill
	movl	804(%esp), %eax
	movl	44(%eax), %ecx
	movl	%ecx, 24(%esp)                  # 4-byte Spill
	movl	40(%eax), %ecx
	movl	%ecx, 36(%esp)                  # 4-byte Spill
	movl	36(%eax), %ecx
	movl	%ecx, 16(%esp)                  # 4-byte Spill
	movl	32(%eax), %ecx
	movl	%ecx, 40(%esp)                  # 4-byte Spill
	movl	28(%eax), %ecx
	movl	%ecx, 12(%esp)                  # 4-byte Spill
	movl	24(%eax), %ecx
	movl	%ecx, 32(%esp)                  # 4-byte Spill
	movl	20(%eax), %ecx
	movl	%ecx, 48(%esp)                  # 4-byte Spill
	movl	16(%eax), %ecx
	movl	%ecx, 20(%esp)                  # 4-byte Spill
	movl	12(%eax), %ecx
	movl	%ecx, 28(%esp)                  # 4-byte Spill
	movl	8(%eax), %esi
	movl	(%eax), %ebp
	movl	4(%eax), %edi
	movl	-4(%edx), %ecx
	movl	%ecx, 56(%esp)                  # 4-byte Spill
	movl	(%edx), %eax
	movl	%eax, 88(%esp)                  # 4-byte Spill
	subl	$4, %esp
	movl	%ebp, %eax
	imull	%ecx, %eax
	leal	732(%esp), %ecx
	pushl	%eax
	pushl	%edx
	pushl	%ecx
	calll	mulPv384x32@PLT
	addl	$12, %esp
	addl	728(%esp), %ebp
	adcl	732(%esp), %edi
	adcl	736(%esp), %esi
	movl	28(%esp), %eax                  # 4-byte Reload
	adcl	740(%esp), %eax
	movl	%eax, 28(%esp)                  # 4-byte Spill
	movl	20(%esp), %eax                  # 4-byte Reload
	adcl	744(%esp), %eax
	movl	%eax, 20(%esp)                  # 4-byte Spill
	movl	48(%esp), %eax                  # 4-byte Reload
	adcl	748(%esp), %eax
	movl	%eax, 48(%esp)                  # 4-byte Spill
	movl	32(%esp), %eax                  # 4-byte Reload
	adcl	752(%esp), %eax
	movl	%eax, 32(%esp)                  # 4-byte Spill
	movl	12(%esp), %eax                  # 4-byte Reload
	adcl	756(%esp), %eax
	movl	%eax, 12(%esp)                  # 4-byte Spill
	movl	40(%esp), %eax                  # 4-byte Reload
	adcl	760(%esp), %eax
	movl	%eax, 40(%esp)                  # 4-byte Spill
	movl	16(%esp), %ebp                  # 4-byte Reload
	adcl	764(%esp), %ebp
	movl	36(%esp), %eax                  # 4-byte Reload
	adcl	768(%esp), %eax
	movl	%eax, 36(%esp)                  # 4-byte Spill
	movl	24(%esp), %eax                  # 4-byte Reload
	adcl	772(%esp), %eax
	movl	%eax, 24(%esp)                  # 4-byte Spill
	movl	804(%esp), %eax
	movl	48(%eax), %eax
	adcl	776(%esp), %eax
	movl	%eax, 44(%esp)                  # 4-byte Spill
	setb	16(%esp)                        # 1-byte Folded Spill
	subl	$4, %esp
	movl	60(%esp), %eax                  # 4-byte Reload
	imull	%edi, %eax
	leal	676(%esp), %ecx
	pushl	%eax
	pushl	816(%esp)
	pushl	%ecx
	calll	mulPv384x32@PLT
	addl	$12, %esp
	addb	$255, 16(%esp)                  # 1-byte Folded Spill
	movl	720(%esp), %eax
	adcl	$0, %eax
	movl	%eax, %ecx
	addl	672(%esp), %edi
	adcl	676(%esp), %esi
	movl	28(%esp), %eax                  # 4-byte Reload
	adcl	680(%esp), %eax
	movl	%eax, 28(%esp)                  # 4-byte Spill
	movl	20(%esp), %eax                  # 4-byte Reload
	adcl	684(%esp), %eax
	movl	%eax, 20(%esp)                  # 4-byte Spill
	movl	48(%esp), %eax                  # 4-byte Reload
	adcl	688(%esp), %eax
	movl	%eax, 48(%esp)                  # 4-byte Spill
	movl	32(%esp), %eax                  # 4-byte Reload
	adcl	692(%esp), %eax
	movl	%eax, 32(%esp)                  # 4-byte Spill
	movl	12(%esp), %eax                  # 4-byte Reload
	adcl	696(%esp), %eax
	movl	%eax, 12(%esp)                  # 4-byte Spill
	movl	40(%esp), %eax                  # 4-byte Reload
	adcl	700(%esp), %eax
	movl	%eax, 40(%esp)                  # 4-byte Spill
	adcl	704(%esp), %ebp
	movl	%ebp, 16(%esp)                  # 4-byte Spill
	movl	36(%esp), %eax                  # 4-byte Reload
	adcl	708(%esp), %eax
	movl	%eax, 36(%esp)                  # 4-byte Spill
	movl	24(%esp), %eax                  # 4-byte Reload
	adcl	712(%esp), %eax
	movl	%eax, 24(%esp)                  # 4-byte Spill
	movl	44(%esp), %edi                  # 4-byte Reload
	adcl	716(%esp), %edi
	movl	804(%esp), %eax
	adcl	52(%eax), %ecx
	movl	%ecx, 52(%esp)                  # 4-byte Spill
	setb	44(%esp)                        # 1-byte Folded Spill
	subl	$4, %esp
	movl	60(%esp), %eax                  # 4-byte Reload
	imull	%esi, %eax
	leal	620(%esp), %ecx
	pushl	%eax
	pushl	816(%esp)
	pushl	%ecx
	calll	mulPv384x32@PLT
	addl	$12, %esp
	addb	$255, 44(%esp)                  # 1-byte Folded Spill
	movl	664(%esp), %eax
	adcl	$0, %eax
	addl	616(%esp), %esi
	movl	28(%esp), %edx                  # 4-byte Reload
	adcl	620(%esp), %edx
	movl	20(%esp), %ecx                  # 4-byte Reload
	adcl	624(%esp), %ecx
	movl	%ecx, 20(%esp)                  # 4-byte Spill
	movl	48(%esp), %ecx                  # 4-byte Reload
	adcl	628(%esp), %ecx
	movl	%ecx, 48(%esp)                  # 4-byte Spill
	movl	32(%esp), %ecx                  # 4-byte Reload
	adcl	632(%esp), %ecx
	movl	%ecx, 32(%esp)                  # 4-byte Spill
	movl	12(%esp), %ecx                  # 4-byte Reload
	adcl	636(%esp), %ecx
	movl	%ecx, 12(%esp)                  # 4-byte Spill
	movl	40(%esp), %ebp                  # 4-byte Reload
	adcl	640(%esp), %ebp
	movl	16(%esp), %ecx                  # 4-byte Reload
	adcl	644(%esp), %ecx
	movl	%ecx, 16(%esp)                  # 4-byte Spill
	movl	36(%esp), %ecx                  # 4-byte Reload
	adcl	648(%esp), %ecx
	movl	%ecx, 36(%esp)                  # 4-byte Spill
	movl	24(%esp), %esi                  # 4-byte Reload
	adcl	652(%esp), %esi
	adcl	656(%esp), %edi
	movl	%edi, 44(%esp)                  # 4-byte Spill
	movl	52(%esp), %ecx                  # 4-byte Reload
	adcl	660(%esp), %ecx
	movl	%ecx, 52(%esp)                  # 4-byte Spill
	movl	804(%esp), %ecx
	adcl	56(%ecx), %eax
	movl	%eax, 28(%esp)                  # 4-byte Spill
	setb	24(%esp)                        # 1-byte Folded Spill
	subl	$4, %esp
	movl	60(%esp), %eax                  # 4-byte Reload
	imull	%edx, %eax
	movl	%edx, %edi
	leal	564(%esp), %ecx
	pushl	%eax
	movl	816(%esp), %eax
	pushl	%eax
	pushl	%ecx
	calll	mulPv384x32@PLT
	addl	$12, %esp
	addb	$255, 24(%esp)                  # 1-byte Folded Spill
	movl	608(%esp), %eax
	adcl	$0, %eax
	addl	560(%esp), %edi
	movl	20(%esp), %edi                  # 4-byte Reload
	adcl	564(%esp), %edi
	movl	48(%esp), %ecx                  # 4-byte Reload
	adcl	568(%esp), %ecx
	movl	%ecx, 48(%esp)                  # 4-byte Spill
	movl	32(%esp), %ecx                  # 4-byte Reload
	adcl	572(%esp), %ecx
	movl	%ecx, 32(%esp)                  # 4-byte Spill
	movl	12(%esp), %ecx                  # 4-byte Reload
	adcl	576(%esp), %ecx
	movl	%ecx, 12(%esp)                  # 4-byte Spill
	adcl	580(%esp), %ebp
	movl	%ebp, 40(%esp)                  # 4-byte Spill
	movl	16(%esp), %ecx                  # 4-byte Reload
	adcl	584(%esp), %ecx
	movl	%ecx, 16(%esp)                  # 4-byte Spill
	movl	36(%esp), %ecx                  # 4-byte Reload
	adcl	588(%esp), %ecx
	movl	%ecx, 36(%esp)                  # 4-byte Spill
	adcl	592(%esp), %esi
	movl	%esi, %ebp
	movl	44(%esp), %ecx                  # 4-byte Reload
	adcl	596(%esp), %ecx
	movl	%ecx, 44(%esp)                  # 4-byte Spill
	movl	52(%esp), %esi                  # 4-byte Reload
	adcl	600(%esp), %esi
	movl	28(%esp), %ecx                  # 4-byte Reload
	adcl	604(%esp), %ecx
	movl	%ecx, 28(%esp)                  # 4-byte Spill
	movl	804(%esp), %ecx
	adcl	60(%ecx), %eax
	movl	%eax, 20(%esp)                  # 4-byte Spill
	setb	24(%esp)                        # 1-byte Folded Spill
	subl	$4, %esp
	movl	60(%esp), %eax                  # 4-byte Reload
	imull	%edi, %eax
	leal	508(%esp), %ecx
	pushl	%eax
	pushl	816(%esp)
	pushl	%ecx
	calll	mulPv384x32@PLT
	addl	$12, %esp
	addb	$255, 24(%esp)                  # 1-byte Folded Spill
	movl	552(%esp), %eax
	adcl	$0, %eax
	movl	%eax, %ecx
	addl	504(%esp), %edi
	movl	48(%esp), %edx                  # 4-byte Reload
	adcl	508(%esp), %edx
	movl	32(%esp), %eax                  # 4-byte Reload
	adcl	512(%esp), %eax
	movl	%eax, 32(%esp)                  # 4-byte Spill
	movl	12(%esp), %eax                  # 4-byte Reload
	adcl	516(%esp), %eax
	movl	%eax, 12(%esp)                  # 4-byte Spill
	movl	40(%esp), %eax                  # 4-byte Reload
	adcl	520(%esp), %eax
	movl	%eax, 40(%esp)                  # 4-byte Spill
	movl	16(%esp), %eax                  # 4-byte Reload
	adcl	524(%esp), %eax
	movl	%eax, 16(%esp)                  # 4-byte Spill
	movl	36(%esp), %eax                  # 4-byte Reload
	adcl	528(%esp), %eax
	movl	%eax, 36(%esp)                  # 4-byte Spill
	adcl	532(%esp), %ebp
	movl	%ebp, 24(%esp)                  # 4-byte Spill
	movl	44(%esp), %eax                  # 4-byte Reload
	adcl	536(%esp), %eax
	movl	%eax, 44(%esp)                  # 4-byte Spill
	adcl	540(%esp), %esi
	movl	%esi, %ebp
	movl	28(%esp), %eax                  # 4-byte Reload
	adcl	544(%esp), %eax
	movl	%eax, 28(%esp)                  # 4-byte Spill
	movl	20(%esp), %eax                  # 4-byte Reload
	adcl	548(%esp), %eax
	movl	%eax, 20(%esp)                  # 4-byte Spill
	movl	804(%esp), %eax
	adcl	64(%eax), %ecx
	movl	%ecx, %edi
	setb	52(%esp)                        # 1-byte Folded Spill
	subl	$4, %esp
	movl	60(%esp), %eax                  # 4-byte Reload
	imull	%edx, %eax
	movl	%edx, %esi
	leal	452(%esp), %ecx
	pushl	%eax
	pushl	816(%esp)
	pushl	%ecx
	calll	mulPv384x32@PLT
	addl	$12, %esp
	addb	$255, 52(%esp)                  # 1-byte Folded Spill
	movl	496(%esp), %eax
	adcl	$0, %eax
	movl	%eax, %ecx
	addl	448(%esp), %esi
	movl	32(%esp), %edx                  # 4-byte Reload
	adcl	452(%esp), %edx
	movl	12(%esp), %eax                  # 4-byte Reload
	adcl	456(%esp), %eax
	movl	%eax, 12(%esp)                  # 4-byte Spill
	movl	40(%esp), %eax                  # 4-byte Reload
	adcl	460(%esp), %eax
	movl	%eax, 40(%esp)                  # 4-byte Spill
	movl	16(%esp), %eax                  # 4-byte Reload
	adcl	464(%esp), %eax
	movl	%eax, 16(%esp)                  # 4-byte Spill
	movl	36(%esp), %esi                  # 4-byte Reload
	adcl	468(%esp), %esi
	movl	24(%esp), %eax                  # 4-byte Reload
	adcl	472(%esp), %eax
	movl	%eax, 24(%esp)                  # 4-byte Spill
	movl	44(%esp), %eax                  # 4-byte Reload
	adcl	476(%esp), %eax
	movl	%eax, 44(%esp)                  # 4-byte Spill
	adcl	480(%esp), %ebp
	movl	28(%esp), %eax                  # 4-byte Reload
	adcl	484(%esp), %eax
	movl	%eax, 28(%esp)                  # 4-byte Spill
	movl	20(%esp), %eax                  # 4-byte Reload
	adcl	488(%esp), %eax
	movl	%eax, 20(%esp)                  # 4-byte Spill
	adcl	492(%esp), %edi
	movl	%edi, 48(%esp)                  # 4-byte Spill
	movl	804(%esp), %eax
	adcl	68(%eax), %ecx
	movl	%ecx, 32(%esp)                  # 4-byte Spill
	setb	36(%esp)                        # 1-byte Folded Spill
	subl	$4, %esp
	movl	60(%esp), %eax                  # 4-byte Reload
	imull	%edx, %eax
	movl	%edx, %edi
	leal	396(%esp), %ecx
	pushl	%eax
	pushl	816(%esp)
	pushl	%ecx
	calll	mulPv384x32@PLT
	addl	$12, %esp
	addb	$255, 36(%esp)                  # 1-byte Folded Spill
	movl	440(%esp), %eax
	adcl	$0, %eax
	movl	%eax, %edx
	addl	392(%esp), %edi
	movl	12(%esp), %ecx                  # 4-byte Reload
	adcl	396(%esp), %ecx
	movl	40(%esp), %eax                  # 4-byte Reload
	adcl	400(%esp), %eax
	movl	%eax, 40(%esp)                  # 4-byte Spill
	movl	16(%esp), %eax                  # 4-byte Reload
	adcl	404(%esp), %eax
	movl	%eax, 16(%esp)                  # 4-byte Spill
	adcl	408(%esp), %esi
	movl	%esi, 36(%esp)                  # 4-byte Spill
	movl	24(%esp), %eax                  # 4-byte Reload
	adcl	412(%esp), %eax
	movl	%eax, 24(%esp)                  # 4-byte Spill
	movl	44(%esp), %edi                  # 4-byte Reload
	adcl	416(%esp), %edi
	adcl	420(%esp), %ebp
	movl	%ebp, 52(%esp)                  # 4-byte Spill
	movl	28(%esp), %eax                  # 4-byte Reload
	adcl	424(%esp), %eax
	movl	%eax, 28(%esp)                  # 4-byte Spill
	movl	20(%esp), %eax                  # 4-byte Reload
	adcl	428(%esp), %eax
	movl	%eax, 20(%esp)                  # 4-byte Spill
	movl	48(%esp), %esi                  # 4-byte Reload
	adcl	432(%esp), %esi
	movl	32(%esp), %eax                  # 4-byte Reload
	adcl	436(%esp), %eax
	movl	%eax, 32(%esp)                  # 4-byte Spill
	movl	804(%esp), %eax
	adcl	72(%eax), %edx
	movl	%edx, 12(%esp)                  # 4-byte Spill
	setb	44(%esp)                        # 1-byte Folded Spill
	subl	$4, %esp
	movl	60(%esp), %eax                  # 4-byte Reload
	imull	%ecx, %eax
	movl	%ecx, %ebp
	leal	340(%esp), %ecx
	pushl	%eax
	pushl	816(%esp)
	pushl	%ecx
	calll	mulPv384x32@PLT
	addl	$12, %esp
	addb	$255, 44(%esp)                  # 1-byte Folded Spill
	movl	384(%esp), %eax
	adcl	$0, %eax
	movl	%eax, %edx
	addl	336(%esp), %ebp
	movl	40(%esp), %ecx                  # 4-byte Reload
	adcl	340(%esp), %ecx
	movl	16(%esp), %eax                  # 4-byte Reload
	adcl	344(%esp), %eax
	movl	%eax, 16(%esp)                  # 4-byte Spill
	movl	36(%esp), %eax                  # 4-byte Reload
	adcl	348(%esp), %eax
	movl	%eax, 36(%esp)                  # 4-byte Spill
	movl	24(%esp), %eax                  # 4-byte Reload
	adcl	352(%esp), %eax
	movl	%eax, 24(%esp)                  # 4-byte Spill
	adcl	356(%esp), %edi
	movl	%edi, 44(%esp)                  # 4-byte Spill
	movl	52(%esp), %ebp                  # 4-byte Reload
	adcl	360(%esp), %ebp
	movl	28(%esp), %eax                  # 4-byte Reload
	adcl	364(%esp), %eax
	movl	%eax, 28(%esp)                  # 4-byte Spill
	movl	20(%esp), %edi                  # 4-byte Reload
	adcl	368(%esp), %edi
	adcl	372(%esp), %esi
	movl	%esi, 48(%esp)                  # 4-byte Spill
	movl	32(%esp), %eax                  # 4-byte Reload
	adcl	376(%esp), %eax
	movl	%eax, 32(%esp)                  # 4-byte Spill
	movl	12(%esp), %eax                  # 4-byte Reload
	adcl	380(%esp), %eax
	movl	%eax, 12(%esp)                  # 4-byte Spill
	movl	804(%esp), %eax
	adcl	76(%eax), %edx
	movl	%edx, 40(%esp)                  # 4-byte Spill
	setb	20(%esp)                        # 1-byte Folded Spill
	subl	$4, %esp
	movl	60(%esp), %eax                  # 4-byte Reload
	imull	%ecx, %eax
	movl	%ecx, %esi
	leal	284(%esp), %ecx
	pushl	%eax
	pushl	816(%esp)
	pushl	%ecx
	calll	mulPv384x32@PLT
	addl	$12, %esp
	addb	$255, 20(%esp)                  # 1-byte Folded Spill
	movl	328(%esp), %eax
	adcl	$0, %eax
	movl	%eax, %edx
	addl	280(%esp), %esi
	movl	16(%esp), %esi                  # 4-byte Reload
	adcl	284(%esp), %esi
	movl	36(%esp), %eax                  # 4-byte Reload
	adcl	288(%esp), %eax
	movl	%eax, 36(%esp)                  # 4-byte Spill
	movl	24(%esp), %eax                  # 4-byte Reload
	adcl	292(%esp), %eax
	movl	%eax, 24(%esp)                  # 4-byte Spill
	movl	44(%esp), %eax                  # 4-byte Reload
	adcl	296(%esp), %eax
	movl	%eax, 44(%esp)                  # 4-byte Spill
	adcl	300(%esp), %ebp
	movl	%ebp, 52(%esp)                  # 4-byte Spill
	movl	28(%esp), %eax                  # 4-byte Reload
	adcl	304(%esp), %eax
	movl	%eax, 28(%esp)                  # 4-byte Spill
	adcl	308(%esp), %edi
	movl	%edi, 20(%esp)                  # 4-byte Spill
	movl	48(%esp), %eax                  # 4-byte Reload
	adcl	312(%esp), %eax
	movl	%eax, 48(%esp)                  # 4-byte Spill
	movl	32(%esp), %eax                  # 4-byte Reload
	adcl	316(%esp), %eax
	movl	%eax, 32(%esp)                  # 4-byte Spill
	movl	12(%esp), %eax                  # 4-byte Reload
	adcl	320(%esp), %eax
	movl	%eax, 12(%esp)                  # 4-byte Spill
	movl	40(%esp), %edi                  # 4-byte Reload
	adcl	324(%esp), %edi
	movl	804(%esp), %eax
	adcl	80(%eax), %edx
	movl	%edx, 16(%esp)                  # 4-byte Spill
	setb	40(%esp)                        # 1-byte Folded Spill
	subl	$4, %esp
	movl	60(%esp), %ebp                  # 4-byte Reload
	movl	%ebp, %eax
	imull	%esi, %eax
	leal	228(%esp), %ecx
	pushl	%eax
	movl	816(%esp), %eax
	pushl	%eax
	pushl	%ecx
	calll	mulPv384x32@PLT
	addl	$12, %esp
	addb	$255, 40(%esp)                  # 1-byte Folded Spill
	movl	272(%esp), %eax
	adcl	$0, %eax
	movl	%eax, %edx
	addl	224(%esp), %esi
	movl	36(%esp), %esi                  # 4-byte Reload
	adcl	228(%esp), %esi
	movl	24(%esp), %eax                  # 4-byte Reload
	adcl	232(%esp), %eax
	movl	%eax, 24(%esp)                  # 4-byte Spill
	movl	44(%esp), %eax                  # 4-byte Reload
	adcl	236(%esp), %eax
	movl	%eax, 44(%esp)                  # 4-byte Spill
	movl	52(%esp), %eax                  # 4-byte Reload
	adcl	240(%esp), %eax
	movl	%eax, 52(%esp)                  # 4-byte Spill
	movl	28(%esp), %eax                  # 4-byte Reload
	adcl	244(%esp), %eax
	movl	%eax, 28(%esp)                  # 4-byte Spill
	movl	20(%esp), %eax                  # 4-byte Reload
	adcl	248(%esp), %eax
	movl	%eax, 20(%esp)                  # 4-byte Spill
	movl	48(%esp), %eax                  # 4-byte Reload
	adcl	252(%esp), %eax
	movl	%eax, 48(%esp)                  # 4-byte Spill
	movl	32(%esp), %eax                  # 4-byte Reload
	adcl	256(%esp), %eax
	movl	%eax, 32(%esp)                  # 4-byte Spill
	movl	12(%esp), %eax                  # 4-byte Reload
	adcl	260(%esp), %eax
	movl	%eax, 12(%esp)                  # 4-byte Spill
	adcl	264(%esp), %edi
	movl	%edi, 40(%esp)                  # 4-byte Spill
	movl	16(%esp), %eax                  # 4-byte Reload
	adcl	268(%esp), %eax
	movl	%eax, 16(%esp)                  # 4-byte Spill
	movl	804(%esp), %eax
	adcl	84(%eax), %edx
	movl	%edx, 36(%esp)                  # 4-byte Spill
	setb	63(%esp)                        # 1-byte Folded Spill
	subl	$4, %esp
	movl	%ebp, %eax
	movl	%ebp, %edi
	imull	%esi, %eax
	leal	172(%esp), %ecx
	pushl	%eax
	pushl	816(%esp)
	pushl	%ecx
	calll	mulPv384x32@PLT
	addl	$12, %esp
	addb	$255, 63(%esp)                  # 1-byte Folded Spill
	movl	216(%esp), %eax
	adcl	$0, %eax
	movl	%eax, %edx
	addl	168(%esp), %esi
	movl	24(%esp), %eax                  # 4-byte Reload
	adcl	172(%esp), %eax
	movl	44(%esp), %ebp                  # 4-byte Reload
	adcl	176(%esp), %ebp
	movl	52(%esp), %ecx                  # 4-byte Reload
	adcl	180(%esp), %ecx
	movl	%ecx, 52(%esp)                  # 4-byte Spill
	movl	28(%esp), %ecx                  # 4-byte Reload
	adcl	184(%esp), %ecx
	movl	%ecx, 28(%esp)                  # 4-byte Spill
	movl	20(%esp), %ecx                  # 4-byte Reload
	adcl	188(%esp), %ecx
	movl	%ecx, 20(%esp)                  # 4-byte Spill
	movl	48(%esp), %ecx                  # 4-byte Reload
	adcl	192(%esp), %ecx
	movl	%ecx, 48(%esp)                  # 4-byte Spill
	movl	32(%esp), %ecx                  # 4-byte Reload
	adcl	196(%esp), %ecx
	movl	%ecx, 32(%esp)                  # 4-byte Spill
	movl	12(%esp), %ecx                  # 4-byte Reload
	adcl	200(%esp), %ecx
	movl	%ecx, 12(%esp)                  # 4-byte Spill
	movl	40(%esp), %ecx                  # 4-byte Reload
	adcl	204(%esp), %ecx
	movl	%ecx, 40(%esp)                  # 4-byte Spill
	movl	16(%esp), %ecx                  # 4-byte Reload
	adcl	208(%esp), %ecx
	movl	%ecx, 16(%esp)                  # 4-byte Spill
	movl	36(%esp), %ecx                  # 4-byte Reload
	adcl	212(%esp), %ecx
	movl	%ecx, 36(%esp)                  # 4-byte Spill
	movl	804(%esp), %ecx
	adcl	88(%ecx), %edx
	movl	%edx, 24(%esp)                  # 4-byte Spill
	setb	44(%esp)                        # 1-byte Folded Spill
	movl	%edi, %ecx
	imull	%eax, %ecx
	movl	%eax, %edi
	subl	$4, %esp
	leal	116(%esp), %eax
	pushl	%ecx
	pushl	816(%esp)
	pushl	%eax
	calll	mulPv384x32@PLT
	addl	$12, %esp
	addb	$255, 44(%esp)                  # 1-byte Folded Spill
	movl	160(%esp), %esi
	adcl	$0, %esi
	addl	112(%esp), %edi
	movl	%ebp, %eax
	adcl	116(%esp), %eax
	movl	52(%esp), %ecx                  # 4-byte Reload
	adcl	120(%esp), %ecx
	movl	%ecx, 52(%esp)                  # 4-byte Spill
	movl	28(%esp), %ecx                  # 4-byte Reload
	adcl	124(%esp), %ecx
	movl	%ecx, 28(%esp)                  # 4-byte Spill
	movl	20(%esp), %edx                  # 4-byte Reload
	adcl	128(%esp), %edx
	movl	%edx, 20(%esp)                  # 4-byte Spill
	movl	48(%esp), %edx                  # 4-byte Reload
	adcl	132(%esp), %edx
	movl	32(%esp), %edi                  # 4-byte Reload
	adcl	136(%esp), %edi
	movl	12(%esp), %ebp                  # 4-byte Reload
	adcl	140(%esp), %ebp
	movl	%ebp, 12(%esp)                  # 4-byte Spill
	movl	40(%esp), %ebp                  # 4-byte Reload
	adcl	144(%esp), %ebp
	movl	16(%esp), %ebx                  # 4-byte Reload
	adcl	148(%esp), %ebx
	movl	%ebx, 16(%esp)                  # 4-byte Spill
	movl	36(%esp), %ebx                  # 4-byte Reload
	adcl	152(%esp), %ebx
	movl	24(%esp), %ecx                  # 4-byte Reload
	adcl	156(%esp), %ecx
	movl	%ecx, 24(%esp)                  # 4-byte Spill
	movl	804(%esp), %ecx
	adcl	92(%ecx), %esi
	movl	%eax, 44(%esp)                  # 4-byte Spill
	subl	88(%esp), %eax                  # 4-byte Folded Reload
	movl	%eax, 56(%esp)                  # 4-byte Spill
	movl	52(%esp), %eax                  # 4-byte Reload
	sbbl	64(%esp), %eax                  # 4-byte Folded Reload
	movl	%eax, 64(%esp)                  # 4-byte Spill
	movl	28(%esp), %eax                  # 4-byte Reload
	sbbl	68(%esp), %eax                  # 4-byte Folded Reload
	movl	%eax, 68(%esp)                  # 4-byte Spill
	movl	20(%esp), %eax                  # 4-byte Reload
	sbbl	72(%esp), %eax                  # 4-byte Folded Reload
	movl	%eax, 72(%esp)                  # 4-byte Spill
	movl	%edx, %eax
	movl	%esi, %edx
	movl	%eax, 48(%esp)                  # 4-byte Spill
	sbbl	76(%esp), %eax                  # 4-byte Folded Reload
	movl	%eax, 76(%esp)                  # 4-byte Spill
	movl	%edi, 32(%esp)                  # 4-byte Spill
	sbbl	80(%esp), %edi                  # 4-byte Folded Reload
	movl	%edi, 80(%esp)                  # 4-byte Spill
	movl	12(%esp), %eax                  # 4-byte Reload
	sbbl	84(%esp), %eax                  # 4-byte Folded Reload
	movl	%eax, 84(%esp)                  # 4-byte Spill
	movl	%ebp, %ecx
	movl	%ebp, 40(%esp)                  # 4-byte Spill
	sbbl	92(%esp), %ecx                  # 4-byte Folded Reload
	movl	16(%esp), %esi                  # 4-byte Reload
	sbbl	96(%esp), %esi                  # 4-byte Folded Reload
	movl	%ebx, %edi
	movl	%ebx, 36(%esp)                  # 4-byte Spill
	sbbl	100(%esp), %edi                 # 4-byte Folded Reload
	movl	24(%esp), %ebp                  # 4-byte Reload
	sbbl	104(%esp), %ebp                 # 4-byte Folded Reload
	movl	%edx, %eax
	sbbl	108(%esp), %eax                 # 4-byte Folded Reload
	movl	$0, %ebx
	sbbl	%ebx, %ebx
	testb	$1, %bl
	jne	.LBB62_1
# %bb.2:
	movl	800(%esp), %edx
	movl	%eax, 44(%edx)
	jne	.LBB62_3
.LBB62_4:
	movl	%ebp, 40(%edx)
	movl	56(%esp), %eax                  # 4-byte Reload
	jne	.LBB62_5
.LBB62_6:
	movl	%edi, 36(%edx)
	movl	76(%esp), %ebx                  # 4-byte Reload
	movl	80(%esp), %ebp                  # 4-byte Reload
	jne	.LBB62_7
.LBB62_8:
	movl	%esi, 32(%edx)
	movl	72(%esp), %edi                  # 4-byte Reload
	jne	.LBB62_9
.LBB62_10:
	movl	%ecx, 28(%edx)
	movl	68(%esp), %esi                  # 4-byte Reload
	movl	84(%esp), %ecx                  # 4-byte Reload
	jne	.LBB62_11
.LBB62_12:
	movl	%ecx, 24(%edx)
	movl	64(%esp), %ecx                  # 4-byte Reload
	jne	.LBB62_13
.LBB62_14:
	movl	%ebp, 20(%edx)
	jne	.LBB62_15
.LBB62_16:
	movl	%ebx, 16(%edx)
	jne	.LBB62_17
.LBB62_18:
	movl	%edi, 12(%edx)
	jne	.LBB62_19
.LBB62_20:
	movl	%esi, 8(%edx)
	jne	.LBB62_21
.LBB62_22:
	movl	%ecx, 4(%edx)
	je	.LBB62_24
.LBB62_23:
	movl	44(%esp), %eax                  # 4-byte Reload
.LBB62_24:
	movl	%eax, (%edx)
	addl	$780, %esp                      # imm = 0x30C
	popl	%esi
	popl	%edi
	popl	%ebx
	popl	%ebp
	retl
.LBB62_1:
	movl	%edx, %eax
	movl	800(%esp), %edx
	movl	%eax, 44(%edx)
	je	.LBB62_4
.LBB62_3:
	movl	24(%esp), %ebp                  # 4-byte Reload
	movl	%ebp, 40(%edx)
	movl	56(%esp), %eax                  # 4-byte Reload
	je	.LBB62_6
.LBB62_5:
	movl	36(%esp), %edi                  # 4-byte Reload
	movl	%edi, 36(%edx)
	movl	76(%esp), %ebx                  # 4-byte Reload
	movl	80(%esp), %ebp                  # 4-byte Reload
	je	.LBB62_8
.LBB62_7:
	movl	16(%esp), %esi                  # 4-byte Reload
	movl	%esi, 32(%edx)
	movl	72(%esp), %edi                  # 4-byte Reload
	je	.LBB62_10
.LBB62_9:
	movl	40(%esp), %ecx                  # 4-byte Reload
	movl	%ecx, 28(%edx)
	movl	68(%esp), %esi                  # 4-byte Reload
	movl	84(%esp), %ecx                  # 4-byte Reload
	je	.LBB62_12
.LBB62_11:
	movl	12(%esp), %ecx                  # 4-byte Reload
	movl	%ecx, 24(%edx)
	movl	64(%esp), %ecx                  # 4-byte Reload
	je	.LBB62_14
.LBB62_13:
	movl	32(%esp), %ebp                  # 4-byte Reload
	movl	%ebp, 20(%edx)
	je	.LBB62_16
.LBB62_15:
	movl	48(%esp), %ebx                  # 4-byte Reload
	movl	%ebx, 16(%edx)
	je	.LBB62_18
.LBB62_17:
	movl	20(%esp), %edi                  # 4-byte Reload
	movl	%edi, 12(%edx)
	je	.LBB62_20
.LBB62_19:
	movl	28(%esp), %esi                  # 4-byte Reload
	movl	%esi, 8(%edx)
	je	.LBB62_22
.LBB62_21:
	movl	52(%esp), %ecx                  # 4-byte Reload
	movl	%ecx, 4(%edx)
	jne	.LBB62_23
	jmp	.LBB62_24
.Lfunc_end62:
	.size	mcl_fp_montRed12L, .Lfunc_end62-mcl_fp_montRed12L
                                        # -- End function
	.globl	mcl_fp_montRedNF12L             # -- Begin function mcl_fp_montRedNF12L
	.p2align	4, 0x90
	.type	mcl_fp_montRedNF12L,@function
mcl_fp_montRedNF12L:                    # @mcl_fp_montRedNF12L
# %bb.0:
	pushl	%ebp
	pushl	%ebx
	pushl	%edi
	pushl	%esi
	subl	$780, %esp                      # imm = 0x30C
	calll	.L63$pb
.L63$pb:
	popl	%ebx
.Ltmp15:
	addl	$_GLOBAL_OFFSET_TABLE_+(.Ltmp15-.L63$pb), %ebx
	movl	808(%esp), %ecx
	movl	44(%ecx), %eax
	movl	%eax, 108(%esp)                 # 4-byte Spill
	movl	40(%ecx), %eax
	movl	%eax, 104(%esp)                 # 4-byte Spill
	movl	36(%ecx), %eax
	movl	%eax, 100(%esp)                 # 4-byte Spill
	movl	32(%ecx), %eax
	movl	%eax, 96(%esp)                  # 4-byte Spill
	movl	28(%ecx), %eax
	movl	%eax, 92(%esp)                  # 4-byte Spill
	movl	24(%ecx), %eax
	movl	%eax, 84(%esp)                  # 4-byte Spill
	movl	20(%ecx), %eax
	movl	%eax, 80(%esp)                  # 4-byte Spill
	movl	16(%ecx), %eax
	movl	%eax, 76(%esp)                  # 4-byte Spill
	movl	12(%ecx), %eax
	movl	%eax, 72(%esp)                  # 4-byte Spill
	movl	8(%ecx), %eax
	movl	%eax, 68(%esp)                  # 4-byte Spill
	movl	4(%ecx), %eax
	movl	%ecx, %edx
	movl	%eax, 64(%esp)                  # 4-byte Spill
	movl	804(%esp), %eax
	movl	44(%eax), %ecx
	movl	%ecx, 36(%esp)                  # 4-byte Spill
	movl	40(%eax), %ecx
	movl	%ecx, 48(%esp)                  # 4-byte Spill
	movl	36(%eax), %ecx
	movl	%ecx, 28(%esp)                  # 4-byte Spill
	movl	32(%eax), %ecx
	movl	%ecx, 24(%esp)                  # 4-byte Spill
	movl	28(%eax), %ecx
	movl	%ecx, 16(%esp)                  # 4-byte Spill
	movl	24(%eax), %ecx
	movl	%ecx, 12(%esp)                  # 4-byte Spill
	movl	20(%eax), %ecx
	movl	%ecx, 52(%esp)                  # 4-byte Spill
	movl	16(%eax), %ecx
	movl	%ecx, 44(%esp)                  # 4-byte Spill
	movl	12(%eax), %ecx
	movl	%ecx, 20(%esp)                  # 4-byte Spill
	movl	8(%eax), %esi
	movl	(%eax), %ebp
	movl	4(%eax), %edi
	movl	-4(%edx), %ecx
	movl	%ecx, 56(%esp)                  # 4-byte Spill
	movl	(%edx), %eax
	movl	%eax, 88(%esp)                  # 4-byte Spill
	subl	$4, %esp
	movl	%ebp, %eax
	imull	%ecx, %eax
	leal	732(%esp), %ecx
	pushl	%eax
	pushl	%edx
	pushl	%ecx
	calll	mulPv384x32@PLT
	addl	$12, %esp
	addl	728(%esp), %ebp
	adcl	732(%esp), %edi
	adcl	736(%esp), %esi
	movl	20(%esp), %eax                  # 4-byte Reload
	adcl	740(%esp), %eax
	movl	%eax, 20(%esp)                  # 4-byte Spill
	movl	44(%esp), %eax                  # 4-byte Reload
	adcl	744(%esp), %eax
	movl	%eax, 44(%esp)                  # 4-byte Spill
	movl	52(%esp), %eax                  # 4-byte Reload
	adcl	748(%esp), %eax
	movl	%eax, 52(%esp)                  # 4-byte Spill
	movl	12(%esp), %eax                  # 4-byte Reload
	adcl	752(%esp), %eax
	movl	%eax, 12(%esp)                  # 4-byte Spill
	movl	16(%esp), %eax                  # 4-byte Reload
	adcl	756(%esp), %eax
	movl	%eax, 16(%esp)                  # 4-byte Spill
	movl	24(%esp), %eax                  # 4-byte Reload
	adcl	760(%esp), %eax
	movl	%eax, 24(%esp)                  # 4-byte Spill
	movl	28(%esp), %eax                  # 4-byte Reload
	adcl	764(%esp), %eax
	movl	%eax, 28(%esp)                  # 4-byte Spill
	movl	48(%esp), %ebp                  # 4-byte Reload
	adcl	768(%esp), %ebp
	movl	36(%esp), %eax                  # 4-byte Reload
	adcl	772(%esp), %eax
	movl	%eax, 36(%esp)                  # 4-byte Spill
	movl	804(%esp), %eax
	movl	48(%eax), %eax
	adcl	776(%esp), %eax
	movl	%eax, 40(%esp)                  # 4-byte Spill
	setb	32(%esp)                        # 1-byte Folded Spill
	subl	$4, %esp
	movl	60(%esp), %eax                  # 4-byte Reload
	imull	%edi, %eax
	leal	676(%esp), %ecx
	pushl	%eax
	pushl	816(%esp)
	pushl	%ecx
	calll	mulPv384x32@PLT
	addl	$12, %esp
	addb	$255, 32(%esp)                  # 1-byte Folded Spill
	movl	720(%esp), %eax
	adcl	$0, %eax
	addl	672(%esp), %edi
	adcl	676(%esp), %esi
	movl	20(%esp), %ecx                  # 4-byte Reload
	adcl	680(%esp), %ecx
	movl	%ecx, 20(%esp)                  # 4-byte Spill
	movl	44(%esp), %ecx                  # 4-byte Reload
	adcl	684(%esp), %ecx
	movl	%ecx, 44(%esp)                  # 4-byte Spill
	movl	52(%esp), %ecx                  # 4-byte Reload
	adcl	688(%esp), %ecx
	movl	%ecx, 52(%esp)                  # 4-byte Spill
	movl	12(%esp), %ecx                  # 4-byte Reload
	adcl	692(%esp), %ecx
	movl	%ecx, 12(%esp)                  # 4-byte Spill
	movl	16(%esp), %ecx                  # 4-byte Reload
	adcl	696(%esp), %ecx
	movl	%ecx, 16(%esp)                  # 4-byte Spill
	movl	24(%esp), %ecx                  # 4-byte Reload
	adcl	700(%esp), %ecx
	movl	%ecx, 24(%esp)                  # 4-byte Spill
	movl	28(%esp), %edi                  # 4-byte Reload
	adcl	704(%esp), %edi
	adcl	708(%esp), %ebp
	movl	36(%esp), %ecx                  # 4-byte Reload
	adcl	712(%esp), %ecx
	movl	%ecx, 36(%esp)                  # 4-byte Spill
	movl	40(%esp), %ecx                  # 4-byte Reload
	adcl	716(%esp), %ecx
	movl	%ecx, 40(%esp)                  # 4-byte Spill
	movl	804(%esp), %ecx
	adcl	52(%ecx), %eax
	movl	%eax, 32(%esp)                  # 4-byte Spill
	setb	48(%esp)                        # 1-byte Folded Spill
	subl	$4, %esp
	movl	60(%esp), %eax                  # 4-byte Reload
	imull	%esi, %eax
	leal	620(%esp), %ecx
	pushl	%eax
	pushl	816(%esp)
	pushl	%ecx
	calll	mulPv384x32@PLT
	addl	$12, %esp
	addb	$255, 48(%esp)                  # 1-byte Folded Spill
	movl	664(%esp), %eax
	adcl	$0, %eax
	addl	616(%esp), %esi
	movl	20(%esp), %edx                  # 4-byte Reload
	adcl	620(%esp), %edx
	movl	44(%esp), %ecx                  # 4-byte Reload
	adcl	624(%esp), %ecx
	movl	%ecx, 44(%esp)                  # 4-byte Spill
	movl	52(%esp), %ecx                  # 4-byte Reload
	adcl	628(%esp), %ecx
	movl	%ecx, 52(%esp)                  # 4-byte Spill
	movl	12(%esp), %ecx                  # 4-byte Reload
	adcl	632(%esp), %ecx
	movl	%ecx, 12(%esp)                  # 4-byte Spill
	movl	16(%esp), %ecx                  # 4-byte Reload
	adcl	636(%esp), %ecx
	movl	%ecx, 16(%esp)                  # 4-byte Spill
	movl	24(%esp), %ecx                  # 4-byte Reload
	adcl	640(%esp), %ecx
	movl	%ecx, 24(%esp)                  # 4-byte Spill
	adcl	644(%esp), %edi
	movl	%edi, 28(%esp)                  # 4-byte Spill
	adcl	648(%esp), %ebp
	movl	36(%esp), %esi                  # 4-byte Reload
	adcl	652(%esp), %esi
	movl	40(%esp), %ecx                  # 4-byte Reload
	adcl	656(%esp), %ecx
	movl	%ecx, 40(%esp)                  # 4-byte Spill
	movl	32(%esp), %ecx                  # 4-byte Reload
	adcl	660(%esp), %ecx
	movl	%ecx, 32(%esp)                  # 4-byte Spill
	movl	804(%esp), %ecx
	adcl	56(%ecx), %eax
	movl	%eax, 20(%esp)                  # 4-byte Spill
	setb	36(%esp)                        # 1-byte Folded Spill
	subl	$4, %esp
	movl	60(%esp), %eax                  # 4-byte Reload
	imull	%edx, %eax
	movl	%edx, %edi
	leal	564(%esp), %ecx
	pushl	%eax
	movl	816(%esp), %eax
	pushl	%eax
	pushl	%ecx
	calll	mulPv384x32@PLT
	addl	$12, %esp
	addb	$255, 36(%esp)                  # 1-byte Folded Spill
	movl	608(%esp), %eax
	adcl	$0, %eax
	addl	560(%esp), %edi
	movl	44(%esp), %edi                  # 4-byte Reload
	adcl	564(%esp), %edi
	movl	52(%esp), %ecx                  # 4-byte Reload
	adcl	568(%esp), %ecx
	movl	%ecx, 52(%esp)                  # 4-byte Spill
	movl	12(%esp), %ecx                  # 4-byte Reload
	adcl	572(%esp), %ecx
	movl	%ecx, 12(%esp)                  # 4-byte Spill
	movl	16(%esp), %ecx                  # 4-byte Reload
	adcl	576(%esp), %ecx
	movl	%ecx, 16(%esp)                  # 4-byte Spill
	movl	24(%esp), %ecx                  # 4-byte Reload
	adcl	580(%esp), %ecx
	movl	%ecx, 24(%esp)                  # 4-byte Spill
	movl	28(%esp), %ecx                  # 4-byte Reload
	adcl	584(%esp), %ecx
	movl	%ecx, 28(%esp)                  # 4-byte Spill
	adcl	588(%esp), %ebp
	movl	%ebp, 48(%esp)                  # 4-byte Spill
	adcl	592(%esp), %esi
	movl	%esi, %ebp
	movl	40(%esp), %esi                  # 4-byte Reload
	adcl	596(%esp), %esi
	movl	32(%esp), %ecx                  # 4-byte Reload
	adcl	600(%esp), %ecx
	movl	%ecx, 32(%esp)                  # 4-byte Spill
	movl	20(%esp), %ecx                  # 4-byte Reload
	adcl	604(%esp), %ecx
	movl	%ecx, 20(%esp)                  # 4-byte Spill
	movl	804(%esp), %ecx
	adcl	60(%ecx), %eax
	movl	%eax, 44(%esp)                  # 4-byte Spill
	setb	40(%esp)                        # 1-byte Folded Spill
	subl	$4, %esp
	movl	60(%esp), %eax                  # 4-byte Reload
	imull	%edi, %eax
	leal	508(%esp), %ecx
	pushl	%eax
	pushl	816(%esp)
	pushl	%ecx
	calll	mulPv384x32@PLT
	addl	$12, %esp
	addb	$255, 40(%esp)                  # 1-byte Folded Spill
	movl	552(%esp), %eax
	adcl	$0, %eax
	movl	%eax, %ecx
	addl	504(%esp), %edi
	movl	52(%esp), %edx                  # 4-byte Reload
	adcl	508(%esp), %edx
	movl	12(%esp), %eax                  # 4-byte Reload
	adcl	512(%esp), %eax
	movl	%eax, 12(%esp)                  # 4-byte Spill
	movl	16(%esp), %eax                  # 4-byte Reload
	adcl	516(%esp), %eax
	movl	%eax, 16(%esp)                  # 4-byte Spill
	movl	24(%esp), %eax                  # 4-byte Reload
	adcl	520(%esp), %eax
	movl	%eax, 24(%esp)                  # 4-byte Spill
	movl	28(%esp), %eax                  # 4-byte Reload
	adcl	524(%esp), %eax
	movl	%eax, 28(%esp)                  # 4-byte Spill
	movl	48(%esp), %eax                  # 4-byte Reload
	adcl	528(%esp), %eax
	movl	%eax, 48(%esp)                  # 4-byte Spill
	adcl	532(%esp), %ebp
	movl	%ebp, 36(%esp)                  # 4-byte Spill
	adcl	536(%esp), %esi
	movl	%esi, 40(%esp)                  # 4-byte Spill
	movl	32(%esp), %esi                  # 4-byte Reload
	adcl	540(%esp), %esi
	movl	20(%esp), %eax                  # 4-byte Reload
	adcl	544(%esp), %eax
	movl	%eax, 20(%esp)                  # 4-byte Spill
	movl	44(%esp), %eax                  # 4-byte Reload
	adcl	548(%esp), %eax
	movl	%eax, 44(%esp)                  # 4-byte Spill
	movl	804(%esp), %eax
	adcl	64(%eax), %ecx
	movl	%ecx, %ebp
	setb	32(%esp)                        # 1-byte Folded Spill
	subl	$4, %esp
	movl	60(%esp), %eax                  # 4-byte Reload
	imull	%edx, %eax
	movl	%edx, %edi
	leal	452(%esp), %ecx
	pushl	%eax
	pushl	816(%esp)
	pushl	%ecx
	calll	mulPv384x32@PLT
	addl	$12, %esp
	addb	$255, 32(%esp)                  # 1-byte Folded Spill
	movl	496(%esp), %eax
	adcl	$0, %eax
	movl	%eax, %edx
	addl	448(%esp), %edi
	movl	12(%esp), %ecx                  # 4-byte Reload
	adcl	452(%esp), %ecx
	movl	16(%esp), %eax                  # 4-byte Reload
	adcl	456(%esp), %eax
	movl	%eax, 16(%esp)                  # 4-byte Spill
	movl	24(%esp), %eax                  # 4-byte Reload
	adcl	460(%esp), %eax
	movl	%eax, 24(%esp)                  # 4-byte Spill
	movl	28(%esp), %edi                  # 4-byte Reload
	adcl	464(%esp), %edi
	movl	48(%esp), %eax                  # 4-byte Reload
	adcl	468(%esp), %eax
	movl	%eax, 48(%esp)                  # 4-byte Spill
	movl	36(%esp), %eax                  # 4-byte Reload
	adcl	472(%esp), %eax
	movl	%eax, 36(%esp)                  # 4-byte Spill
	movl	40(%esp), %eax                  # 4-byte Reload
	adcl	476(%esp), %eax
	movl	%eax, 40(%esp)                  # 4-byte Spill
	adcl	480(%esp), %esi
	movl	%esi, 32(%esp)                  # 4-byte Spill
	movl	20(%esp), %eax                  # 4-byte Reload
	adcl	484(%esp), %eax
	movl	%eax, 20(%esp)                  # 4-byte Spill
	movl	44(%esp), %esi                  # 4-byte Reload
	adcl	488(%esp), %esi
	adcl	492(%esp), %ebp
	movl	%ebp, 12(%esp)                  # 4-byte Spill
	movl	804(%esp), %eax
	adcl	68(%eax), %edx
	movl	%edx, 52(%esp)                  # 4-byte Spill
	setb	28(%esp)                        # 1-byte Folded Spill
	subl	$4, %esp
	movl	60(%esp), %eax                  # 4-byte Reload
	imull	%ecx, %eax
	movl	%ecx, %ebp
	leal	396(%esp), %ecx
	pushl	%eax
	pushl	816(%esp)
	pushl	%ecx
	calll	mulPv384x32@PLT
	addl	$12, %esp
	addb	$255, 28(%esp)                  # 1-byte Folded Spill
	movl	440(%esp), %eax
	adcl	$0, %eax
	movl	%eax, %edx
	addl	392(%esp), %ebp
	movl	16(%esp), %ecx                  # 4-byte Reload
	adcl	396(%esp), %ecx
	movl	24(%esp), %eax                  # 4-byte Reload
	adcl	400(%esp), %eax
	movl	%eax, 24(%esp)                  # 4-byte Spill
	adcl	404(%esp), %edi
	movl	%edi, 28(%esp)                  # 4-byte Spill
	movl	48(%esp), %eax                  # 4-byte Reload
	adcl	408(%esp), %eax
	movl	%eax, 48(%esp)                  # 4-byte Spill
	movl	36(%esp), %eax                  # 4-byte Reload
	adcl	412(%esp), %eax
	movl	%eax, 36(%esp)                  # 4-byte Spill
	movl	40(%esp), %eax                  # 4-byte Reload
	adcl	416(%esp), %eax
	movl	%eax, 40(%esp)                  # 4-byte Spill
	movl	32(%esp), %eax                  # 4-byte Reload
	adcl	420(%esp), %eax
	movl	%eax, 32(%esp)                  # 4-byte Spill
	movl	20(%esp), %eax                  # 4-byte Reload
	adcl	424(%esp), %eax
	movl	%eax, 20(%esp)                  # 4-byte Spill
	adcl	428(%esp), %esi
	movl	%esi, 44(%esp)                  # 4-byte Spill
	movl	12(%esp), %edi                  # 4-byte Reload
	adcl	432(%esp), %edi
	movl	52(%esp), %eax                  # 4-byte Reload
	adcl	436(%esp), %eax
	movl	%eax, 52(%esp)                  # 4-byte Spill
	movl	804(%esp), %eax
	adcl	72(%eax), %edx
	movl	%edx, 16(%esp)                  # 4-byte Spill
	setb	12(%esp)                        # 1-byte Folded Spill
	subl	$4, %esp
	movl	60(%esp), %ebp                  # 4-byte Reload
	movl	%ebp, %eax
	imull	%ecx, %eax
	movl	%ecx, %esi
	leal	340(%esp), %ecx
	pushl	%eax
	pushl	816(%esp)
	pushl	%ecx
	calll	mulPv384x32@PLT
	addl	$12, %esp
	addb	$255, 12(%esp)                  # 1-byte Folded Spill
	movl	384(%esp), %eax
	adcl	$0, %eax
	movl	%eax, %edx
	addl	336(%esp), %esi
	movl	24(%esp), %ecx                  # 4-byte Reload
	adcl	340(%esp), %ecx
	movl	28(%esp), %eax                  # 4-byte Reload
	adcl	344(%esp), %eax
	movl	%eax, 28(%esp)                  # 4-byte Spill
	movl	48(%esp), %eax                  # 4-byte Reload
	adcl	348(%esp), %eax
	movl	%eax, 48(%esp)                  # 4-byte Spill
	movl	36(%esp), %eax                  # 4-byte Reload
	adcl	352(%esp), %eax
	movl	%eax, 36(%esp)                  # 4-byte Spill
	movl	40(%esp), %eax                  # 4-byte Reload
	adcl	356(%esp), %eax
	movl	%eax, 40(%esp)                  # 4-byte Spill
	movl	32(%esp), %eax                  # 4-byte Reload
	adcl	360(%esp), %eax
	movl	%eax, 32(%esp)                  # 4-byte Spill
	movl	20(%esp), %eax                  # 4-byte Reload
	adcl	364(%esp), %eax
	movl	%eax, 20(%esp)                  # 4-byte Spill
	movl	44(%esp), %eax                  # 4-byte Reload
	adcl	368(%esp), %eax
	movl	%eax, 44(%esp)                  # 4-byte Spill
	adcl	372(%esp), %edi
	movl	%edi, 12(%esp)                  # 4-byte Spill
	movl	52(%esp), %eax                  # 4-byte Reload
	adcl	376(%esp), %eax
	movl	%eax, 52(%esp)                  # 4-byte Spill
	movl	16(%esp), %eax                  # 4-byte Reload
	adcl	380(%esp), %eax
	movl	%eax, 16(%esp)                  # 4-byte Spill
	movl	804(%esp), %eax
	adcl	76(%eax), %edx
	movl	%edx, 24(%esp)                  # 4-byte Spill
	setb	63(%esp)                        # 1-byte Folded Spill
	subl	$4, %esp
	imull	%ecx, %ebp
	movl	%ecx, %esi
	leal	284(%esp), %ecx
	pushl	%ebp
	movl	816(%esp), %eax
	pushl	%eax
	pushl	%ecx
	calll	mulPv384x32@PLT
	addl	$12, %esp
	addb	$255, 63(%esp)                  # 1-byte Folded Spill
	movl	328(%esp), %eax
	adcl	$0, %eax
	movl	%eax, %edx
	addl	280(%esp), %esi
	movl	28(%esp), %ecx                  # 4-byte Reload
	adcl	284(%esp), %ecx
	movl	48(%esp), %eax                  # 4-byte Reload
	adcl	288(%esp), %eax
	movl	%eax, 48(%esp)                  # 4-byte Spill
	movl	36(%esp), %eax                  # 4-byte Reload
	adcl	292(%esp), %eax
	movl	%eax, 36(%esp)                  # 4-byte Spill
	movl	40(%esp), %eax                  # 4-byte Reload
	adcl	296(%esp), %eax
	movl	%eax, 40(%esp)                  # 4-byte Spill
	movl	32(%esp), %edi                  # 4-byte Reload
	adcl	300(%esp), %edi
	movl	20(%esp), %eax                  # 4-byte Reload
	adcl	304(%esp), %eax
	movl	%eax, 20(%esp)                  # 4-byte Spill
	movl	44(%esp), %ebp                  # 4-byte Reload
	adcl	308(%esp), %ebp
	movl	12(%esp), %eax                  # 4-byte Reload
	adcl	312(%esp), %eax
	movl	%eax, 12(%esp)                  # 4-byte Spill
	movl	52(%esp), %eax                  # 4-byte Reload
	adcl	316(%esp), %eax
	movl	%eax, 52(%esp)                  # 4-byte Spill
	movl	16(%esp), %eax                  # 4-byte Reload
	adcl	320(%esp), %eax
	movl	%eax, 16(%esp)                  # 4-byte Spill
	movl	24(%esp), %eax                  # 4-byte Reload
	adcl	324(%esp), %eax
	movl	%eax, 24(%esp)                  # 4-byte Spill
	movl	804(%esp), %eax
	adcl	80(%eax), %edx
	movl	%edx, 28(%esp)                  # 4-byte Spill
	setb	32(%esp)                        # 1-byte Folded Spill
	subl	$4, %esp
	movl	60(%esp), %eax                  # 4-byte Reload
	imull	%ecx, %eax
	movl	%ecx, %esi
	leal	228(%esp), %ecx
	pushl	%eax
	pushl	816(%esp)
	pushl	%ecx
	calll	mulPv384x32@PLT
	addl	$12, %esp
	addb	$255, 32(%esp)                  # 1-byte Folded Spill
	movl	272(%esp), %eax
	adcl	$0, %eax
	movl	%eax, %edx
	addl	224(%esp), %esi
	movl	48(%esp), %ecx                  # 4-byte Reload
	adcl	228(%esp), %ecx
	movl	36(%esp), %eax                  # 4-byte Reload
	adcl	232(%esp), %eax
	movl	%eax, 36(%esp)                  # 4-byte Spill
	movl	40(%esp), %eax                  # 4-byte Reload
	adcl	236(%esp), %eax
	movl	%eax, 40(%esp)                  # 4-byte Spill
	adcl	240(%esp), %edi
	movl	%edi, 32(%esp)                  # 4-byte Spill
	movl	20(%esp), %eax                  # 4-byte Reload
	adcl	244(%esp), %eax
	movl	%eax, 20(%esp)                  # 4-byte Spill
	adcl	248(%esp), %ebp
	movl	%ebp, 44(%esp)                  # 4-byte Spill
	movl	12(%esp), %esi                  # 4-byte Reload
	adcl	252(%esp), %esi
	movl	52(%esp), %edi                  # 4-byte Reload
	adcl	256(%esp), %edi
	movl	16(%esp), %eax                  # 4-byte Reload
	adcl	260(%esp), %eax
	movl	%eax, 16(%esp)                  # 4-byte Spill
	movl	24(%esp), %eax                  # 4-byte Reload
	adcl	264(%esp), %eax
	movl	%eax, 24(%esp)                  # 4-byte Spill
	movl	28(%esp), %eax                  # 4-byte Reload
	adcl	268(%esp), %eax
	movl	%eax, 28(%esp)                  # 4-byte Spill
	movl	804(%esp), %eax
	adcl	84(%eax), %edx
	movl	%edx, 48(%esp)                  # 4-byte Spill
	setb	12(%esp)                        # 1-byte Folded Spill
	subl	$4, %esp
	movl	60(%esp), %eax                  # 4-byte Reload
	imull	%ecx, %eax
	movl	%ecx, %ebp
	leal	172(%esp), %ecx
	pushl	%eax
	pushl	816(%esp)
	pushl	%ecx
	calll	mulPv384x32@PLT
	addl	$12, %esp
	addb	$255, 12(%esp)                  # 1-byte Folded Spill
	movl	216(%esp), %eax
	adcl	$0, %eax
	movl	%eax, %edx
	addl	168(%esp), %ebp
	movl	36(%esp), %eax                  # 4-byte Reload
	adcl	172(%esp), %eax
	movl	40(%esp), %ecx                  # 4-byte Reload
	adcl	176(%esp), %ecx
	movl	%ecx, 40(%esp)                  # 4-byte Spill
	movl	32(%esp), %ebp                  # 4-byte Reload
	adcl	180(%esp), %ebp
	movl	20(%esp), %ecx                  # 4-byte Reload
	adcl	184(%esp), %ecx
	movl	%ecx, 20(%esp)                  # 4-byte Spill
	movl	44(%esp), %ecx                  # 4-byte Reload
	adcl	188(%esp), %ecx
	movl	%ecx, 44(%esp)                  # 4-byte Spill
	adcl	192(%esp), %esi
	movl	%esi, 12(%esp)                  # 4-byte Spill
	adcl	196(%esp), %edi
	movl	16(%esp), %ecx                  # 4-byte Reload
	adcl	200(%esp), %ecx
	movl	%ecx, 16(%esp)                  # 4-byte Spill
	movl	24(%esp), %ecx                  # 4-byte Reload
	adcl	204(%esp), %ecx
	movl	%ecx, 24(%esp)                  # 4-byte Spill
	movl	28(%esp), %ecx                  # 4-byte Reload
	adcl	208(%esp), %ecx
	movl	%ecx, 28(%esp)                  # 4-byte Spill
	movl	48(%esp), %ecx                  # 4-byte Reload
	adcl	212(%esp), %ecx
	movl	%ecx, 48(%esp)                  # 4-byte Spill
	movl	804(%esp), %ecx
	adcl	88(%ecx), %edx
	movl	%edx, 36(%esp)                  # 4-byte Spill
	setb	32(%esp)                        # 1-byte Folded Spill
	movl	56(%esp), %ecx                  # 4-byte Reload
	imull	%eax, %ecx
	movl	%eax, %esi
	subl	$4, %esp
	leal	116(%esp), %eax
	pushl	%ecx
	pushl	816(%esp)
	pushl	%eax
	calll	mulPv384x32@PLT
	addl	$12, %esp
	addb	$255, 32(%esp)                  # 1-byte Folded Spill
	movl	160(%esp), %edx
	adcl	$0, %edx
	addl	112(%esp), %esi
	movl	40(%esp), %ecx                  # 4-byte Reload
	adcl	116(%esp), %ecx
	adcl	120(%esp), %ebp
	movl	%ebp, 32(%esp)                  # 4-byte Spill
	movl	20(%esp), %eax                  # 4-byte Reload
	adcl	124(%esp), %eax
	movl	%eax, 20(%esp)                  # 4-byte Spill
	movl	44(%esp), %esi                  # 4-byte Reload
	adcl	128(%esp), %esi
	movl	12(%esp), %eax                  # 4-byte Reload
	adcl	132(%esp), %eax
	movl	%eax, 12(%esp)                  # 4-byte Spill
	adcl	136(%esp), %edi
	movl	16(%esp), %ebp                  # 4-byte Reload
	adcl	140(%esp), %ebp
	movl	%ebp, 16(%esp)                  # 4-byte Spill
	movl	24(%esp), %ebp                  # 4-byte Reload
	adcl	144(%esp), %ebp
	movl	28(%esp), %ebx                  # 4-byte Reload
	adcl	148(%esp), %ebx
	movl	%ebx, 28(%esp)                  # 4-byte Spill
	movl	48(%esp), %eax                  # 4-byte Reload
	adcl	152(%esp), %eax
	movl	36(%esp), %ebx                  # 4-byte Reload
	adcl	156(%esp), %ebx
	movl	%ebx, 36(%esp)                  # 4-byte Spill
	movl	804(%esp), %ebx
	adcl	92(%ebx), %edx
	movl	%eax, %ebx
	movl	%ecx, 40(%esp)                  # 4-byte Spill
	subl	88(%esp), %ecx                  # 4-byte Folded Reload
	movl	%ecx, 56(%esp)                  # 4-byte Spill
	movl	32(%esp), %eax                  # 4-byte Reload
	sbbl	64(%esp), %eax                  # 4-byte Folded Reload
	movl	%eax, 64(%esp)                  # 4-byte Spill
	movl	20(%esp), %eax                  # 4-byte Reload
	sbbl	68(%esp), %eax                  # 4-byte Folded Reload
	movl	%eax, 68(%esp)                  # 4-byte Spill
	movl	%esi, %eax
	movl	%esi, 44(%esp)                  # 4-byte Spill
	sbbl	72(%esp), %eax                  # 4-byte Folded Reload
	movl	%eax, 72(%esp)                  # 4-byte Spill
	movl	12(%esp), %eax                  # 4-byte Reload
	sbbl	76(%esp), %eax                  # 4-byte Folded Reload
	movl	%eax, 76(%esp)                  # 4-byte Spill
	movl	%edi, 52(%esp)                  # 4-byte Spill
	sbbl	80(%esp), %edi                  # 4-byte Folded Reload
	movl	%edi, 80(%esp)                  # 4-byte Spill
	movl	16(%esp), %eax                  # 4-byte Reload
	sbbl	84(%esp), %eax                  # 4-byte Folded Reload
	movl	%eax, 84(%esp)                  # 4-byte Spill
	movl	%ebp, %ecx
	movl	%ebp, 24(%esp)                  # 4-byte Spill
	sbbl	92(%esp), %ecx                  # 4-byte Folded Reload
	movl	28(%esp), %esi                  # 4-byte Reload
	sbbl	96(%esp), %esi                  # 4-byte Folded Reload
	movl	%ebx, 48(%esp)                  # 4-byte Spill
	movl	%ebx, %edi
	sbbl	100(%esp), %edi                 # 4-byte Folded Reload
	movl	36(%esp), %ebp                  # 4-byte Reload
	sbbl	104(%esp), %ebp                 # 4-byte Folded Reload
	movl	%edx, %ebx
	sbbl	108(%esp), %edx                 # 4-byte Folded Reload
	movl	%edx, %eax
	sarl	$31, %eax
	testl	%eax, %eax
	js	.LBB63_1
# %bb.2:
	movl	800(%esp), %eax
	movl	%edx, 44(%eax)
	js	.LBB63_3
.LBB63_4:
	movl	%ebp, 40(%eax)
	movl	64(%esp), %edx                  # 4-byte Reload
	js	.LBB63_5
.LBB63_6:
	movl	%edi, 36(%eax)
	movl	80(%esp), %ebp                  # 4-byte Reload
	js	.LBB63_7
.LBB63_8:
	movl	%esi, 32(%eax)
	movl	72(%esp), %edi                  # 4-byte Reload
	movl	76(%esp), %ebx                  # 4-byte Reload
	js	.LBB63_9
.LBB63_10:
	movl	%ecx, 28(%eax)
	movl	68(%esp), %esi                  # 4-byte Reload
	movl	84(%esp), %ecx                  # 4-byte Reload
	js	.LBB63_11
.LBB63_12:
	movl	%ecx, 24(%eax)
	movl	56(%esp), %ecx                  # 4-byte Reload
	js	.LBB63_13
.LBB63_14:
	movl	%ebp, 20(%eax)
	js	.LBB63_15
.LBB63_16:
	movl	%ebx, 16(%eax)
	js	.LBB63_17
.LBB63_18:
	movl	%edi, 12(%eax)
	js	.LBB63_19
.LBB63_20:
	movl	%esi, 8(%eax)
	js	.LBB63_21
.LBB63_22:
	movl	%edx, 4(%eax)
	jns	.LBB63_24
.LBB63_23:
	movl	40(%esp), %ecx                  # 4-byte Reload
.LBB63_24:
	movl	%ecx, (%eax)
	addl	$780, %esp                      # imm = 0x30C
	popl	%esi
	popl	%edi
	popl	%ebx
	popl	%ebp
	retl
.LBB63_1:
	movl	%ebx, %edx
	movl	800(%esp), %eax
	movl	%edx, 44(%eax)
	jns	.LBB63_4
.LBB63_3:
	movl	36(%esp), %ebp                  # 4-byte Reload
	movl	%ebp, 40(%eax)
	movl	64(%esp), %edx                  # 4-byte Reload
	jns	.LBB63_6
.LBB63_5:
	movl	48(%esp), %edi                  # 4-byte Reload
	movl	%edi, 36(%eax)
	movl	80(%esp), %ebp                  # 4-byte Reload
	jns	.LBB63_8
.LBB63_7:
	movl	28(%esp), %esi                  # 4-byte Reload
	movl	%esi, 32(%eax)
	movl	72(%esp), %edi                  # 4-byte Reload
	movl	76(%esp), %ebx                  # 4-byte Reload
	jns	.LBB63_10
.LBB63_9:
	movl	24(%esp), %ecx                  # 4-byte Reload
	movl	%ecx, 28(%eax)
	movl	68(%esp), %esi                  # 4-byte Reload
	movl	84(%esp), %ecx                  # 4-byte Reload
	jns	.LBB63_12
.LBB63_11:
	movl	16(%esp), %ecx                  # 4-byte Reload
	movl	%ecx, 24(%eax)
	movl	56(%esp), %ecx                  # 4-byte Reload
	jns	.LBB63_14
.LBB63_13:
	movl	52(%esp), %ebp                  # 4-byte Reload
	movl	%ebp, 20(%eax)
	jns	.LBB63_16
.LBB63_15:
	movl	12(%esp), %ebx                  # 4-byte Reload
	movl	%ebx, 16(%eax)
	jns	.LBB63_18
.LBB63_17:
	movl	44(%esp), %edi                  # 4-byte Reload
	movl	%edi, 12(%eax)
	jns	.LBB63_20
.LBB63_19:
	movl	20(%esp), %esi                  # 4-byte Reload
	movl	%esi, 8(%eax)
	jns	.LBB63_22
.LBB63_21:
	movl	32(%esp), %edx                  # 4-byte Reload
	movl	%edx, 4(%eax)
	js	.LBB63_23
	jmp	.LBB63_24
.Lfunc_end63:
	.size	mcl_fp_montRedNF12L, .Lfunc_end63-mcl_fp_montRedNF12L
                                        # -- End function
	.globl	mcl_fp_addPre12L                # -- Begin function mcl_fp_addPre12L
	.p2align	4, 0x90
	.type	mcl_fp_addPre12L,@function
mcl_fp_addPre12L:                       # @mcl_fp_addPre12L
# %bb.0:
	pushl	%ebp
	pushl	%ebx
	pushl	%edi
	pushl	%esi
	subl	$36, %esp
	movl	60(%esp), %edi
	movl	(%edi), %eax
	movl	4(%edi), %ecx
	movl	64(%esp), %esi
	addl	(%esi), %eax
	movl	%eax, 32(%esp)                  # 4-byte Spill
	adcl	4(%esi), %ecx
	movl	%ecx, 28(%esp)                  # 4-byte Spill
	movl	44(%edi), %eax
	movl	%eax, 24(%esp)                  # 4-byte Spill
	movl	40(%edi), %ebx
	movl	36(%edi), %ebp
	movl	32(%edi), %eax
	movl	%eax, 8(%esp)                   # 4-byte Spill
	movl	28(%edi), %eax
	movl	%eax, 4(%esp)                   # 4-byte Spill
	movl	24(%edi), %edx
	movl	20(%edi), %eax
	movl	%eax, (%esp)                    # 4-byte Spill
	movl	16(%edi), %ecx
	movl	12(%edi), %eax
	movl	8(%edi), %edi
	adcl	8(%esi), %edi
	movl	%edi, 20(%esp)                  # 4-byte Spill
	adcl	12(%esi), %eax
	movl	%eax, 16(%esp)                  # 4-byte Spill
	adcl	16(%esi), %ecx
	movl	%ecx, 12(%esp)                  # 4-byte Spill
	movl	(%esp), %edi                    # 4-byte Reload
	adcl	20(%esi), %edi
	adcl	24(%esi), %edx
	movl	%edx, (%esp)                    # 4-byte Spill
	movl	4(%esp), %edx                   # 4-byte Reload
	adcl	28(%esi), %edx
	movl	8(%esp), %eax                   # 4-byte Reload
	adcl	32(%esi), %eax
	adcl	36(%esi), %ebp
	adcl	40(%esi), %ebx
	movl	24(%esp), %ecx                  # 4-byte Reload
	adcl	44(%esi), %ecx
	movl	56(%esp), %esi
	movl	%ebx, 40(%esi)
	movl	%ebp, 36(%esi)
	movl	%eax, 32(%esi)
	movl	%edx, 28(%esi)
	movl	(%esp), %eax                    # 4-byte Reload
	movl	%eax, 24(%esi)
	movl	%edi, 20(%esi)
	movl	12(%esp), %eax                  # 4-byte Reload
	movl	%eax, 16(%esi)
	movl	16(%esp), %eax                  # 4-byte Reload
	movl	%eax, 12(%esi)
	movl	20(%esp), %eax                  # 4-byte Reload
	movl	%eax, 8(%esi)
	movl	%ecx, 44(%esi)
	movl	28(%esp), %eax                  # 4-byte Reload
	movl	%eax, 4(%esi)
	movl	32(%esp), %eax                  # 4-byte Reload
	movl	%eax, (%esi)
	setb	%al
	movzbl	%al, %eax
	addl	$36, %esp
	popl	%esi
	popl	%edi
	popl	%ebx
	popl	%ebp
	retl
.Lfunc_end64:
	.size	mcl_fp_addPre12L, .Lfunc_end64-mcl_fp_addPre12L
                                        # -- End function
	.globl	mcl_fp_subPre12L                # -- Begin function mcl_fp_subPre12L
	.p2align	4, 0x90
	.type	mcl_fp_subPre12L,@function
mcl_fp_subPre12L:                       # @mcl_fp_subPre12L
# %bb.0:
	pushl	%ebp
	pushl	%ebx
	pushl	%edi
	pushl	%esi
	subl	$44, %esp
	movl	68(%esp), %ebx
	movl	(%ebx), %ecx
	movl	4(%ebx), %eax
	movl	72(%esp), %edi
	subl	(%edi), %ecx
	movl	%ecx, 40(%esp)                  # 4-byte Spill
	sbbl	4(%edi), %eax
	movl	%eax, 36(%esp)                  # 4-byte Spill
	movl	44(%ebx), %eax
	movl	%eax, 32(%esp)                  # 4-byte Spill
	movl	40(%ebx), %eax
	movl	%eax, 24(%esp)                  # 4-byte Spill
	movl	36(%ebx), %eax
	movl	%eax, 16(%esp)                  # 4-byte Spill
	movl	32(%ebx), %eax
	movl	%eax, 8(%esp)                   # 4-byte Spill
	movl	28(%ebx), %eax
	movl	%eax, 4(%esp)                   # 4-byte Spill
	movl	24(%ebx), %edx
	movl	20(%ebx), %ebp
	movl	16(%ebx), %ecx
	movl	12(%ebx), %eax
	movl	8(%ebx), %esi
	sbbl	8(%edi), %esi
	movl	%esi, 28(%esp)                  # 4-byte Spill
	sbbl	12(%edi), %eax
	movl	%eax, 20(%esp)                  # 4-byte Spill
	sbbl	16(%edi), %ecx
	movl	%ecx, 12(%esp)                  # 4-byte Spill
	sbbl	20(%edi), %ebp
	sbbl	24(%edi), %edx
	movl	%edx, (%esp)                    # 4-byte Spill
	movl	4(%esp), %ebx                   # 4-byte Reload
	sbbl	28(%edi), %ebx
	movl	8(%esp), %edx                   # 4-byte Reload
	sbbl	32(%edi), %edx
	movl	16(%esp), %ecx                  # 4-byte Reload
	sbbl	36(%edi), %ecx
	movl	24(%esp), %eax                  # 4-byte Reload
	sbbl	40(%edi), %eax
	movl	32(%esp), %esi                  # 4-byte Reload
	sbbl	44(%edi), %esi
	movl	64(%esp), %edi
	movl	%eax, 40(%edi)
	movl	%ecx, 36(%edi)
	movl	%edx, 32(%edi)
	movl	%ebx, 28(%edi)
	movl	(%esp), %eax                    # 4-byte Reload
	movl	%eax, 24(%edi)
	movl	%ebp, 20(%edi)
	movl	12(%esp), %eax                  # 4-byte Reload
	movl	%eax, 16(%edi)
	movl	20(%esp), %eax                  # 4-byte Reload
	movl	%eax, 12(%edi)
	movl	28(%esp), %eax                  # 4-byte Reload
	movl	%eax, 8(%edi)
	movl	36(%esp), %ecx                  # 4-byte Reload
	movl	%ecx, 4(%edi)
	movl	%esi, 44(%edi)
	movl	40(%esp), %ecx                  # 4-byte Reload
	movl	%ecx, (%edi)
	movl	$0, %eax
	sbbl	%eax, %eax
	andl	$1, %eax
	addl	$44, %esp
	popl	%esi
	popl	%edi
	popl	%ebx
	popl	%ebp
	retl
.Lfunc_end65:
	.size	mcl_fp_subPre12L, .Lfunc_end65-mcl_fp_subPre12L
                                        # -- End function
	.globl	mcl_fp_shr1_12L                 # -- Begin function mcl_fp_shr1_12L
	.p2align	4, 0x90
	.type	mcl_fp_shr1_12L,@function
mcl_fp_shr1_12L:                        # @mcl_fp_shr1_12L
# %bb.0:
	pushl	%esi
	movl	12(%esp), %eax
	movl	44(%eax), %edx
	movl	%edx, %esi
	shrl	%esi
	movl	8(%esp), %ecx
	movl	%esi, 44(%ecx)
	movl	40(%eax), %esi
	shldl	$31, %esi, %edx
	movl	%edx, 40(%ecx)
	movl	36(%eax), %edx
	shldl	$31, %edx, %esi
	movl	%esi, 36(%ecx)
	movl	32(%eax), %esi
	shldl	$31, %esi, %edx
	movl	%edx, 32(%ecx)
	movl	28(%eax), %edx
	shldl	$31, %edx, %esi
	movl	%esi, 28(%ecx)
	movl	24(%eax), %esi
	shldl	$31, %esi, %edx
	movl	%edx, 24(%ecx)
	movl	20(%eax), %edx
	shldl	$31, %edx, %esi
	movl	%esi, 20(%ecx)
	movl	16(%eax), %esi
	shldl	$31, %esi, %edx
	movl	%edx, 16(%ecx)
	movl	12(%eax), %edx
	shldl	$31, %edx, %esi
	movl	%esi, 12(%ecx)
	movl	8(%eax), %esi
	shldl	$31, %esi, %edx
	movl	%edx, 8(%ecx)
	movl	4(%eax), %edx
	shldl	$31, %edx, %esi
	movl	%esi, 4(%ecx)
	movl	(%eax), %eax
	shrdl	$1, %edx, %eax
	movl	%eax, (%ecx)
	popl	%esi
	retl
.Lfunc_end66:
	.size	mcl_fp_shr1_12L, .Lfunc_end66-mcl_fp_shr1_12L
                                        # -- End function
	.globl	mcl_fp_add12L                   # -- Begin function mcl_fp_add12L
	.p2align	4, 0x90
	.type	mcl_fp_add12L,@function
mcl_fp_add12L:                          # @mcl_fp_add12L
# %bb.0:
	pushl	%ebp
	pushl	%ebx
	pushl	%edi
	pushl	%esi
	subl	$48, %esp
	movl	72(%esp), %edi
	movl	(%edi), %ecx
	movl	4(%edi), %eax
	movl	76(%esp), %ebx
	addl	(%ebx), %ecx
	movl	%ecx, 24(%esp)                  # 4-byte Spill
	adcl	4(%ebx), %eax
	movl	%eax, 20(%esp)                  # 4-byte Spill
	movl	44(%edi), %ebp
	movl	40(%edi), %eax
	movl	%eax, 36(%esp)                  # 4-byte Spill
	movl	36(%edi), %eax
	movl	%eax, 32(%esp)                  # 4-byte Spill
	movl	32(%edi), %eax
	movl	%eax, 28(%esp)                  # 4-byte Spill
	movl	28(%edi), %eax
	movl	24(%edi), %ecx
	movl	%ecx, 4(%esp)                   # 4-byte Spill
	movl	20(%edi), %ecx
	movl	16(%edi), %esi
	movl	12(%edi), %edx
	movl	8(%edi), %edi
	adcl	8(%ebx), %edi
	movl	%edi, 16(%esp)                  # 4-byte Spill
	adcl	12(%ebx), %edx
	adcl	16(%ebx), %esi
	adcl	20(%ebx), %ecx
	movl	%ecx, 8(%esp)                   # 4-byte Spill
	movl	4(%esp), %ecx                   # 4-byte Reload
	adcl	24(%ebx), %ecx
	movl	%ecx, 4(%esp)                   # 4-byte Spill
	adcl	28(%ebx), %eax
	movl	%eax, 12(%esp)                  # 4-byte Spill
	movl	28(%esp), %edi                  # 4-byte Reload
	adcl	32(%ebx), %edi
	movl	32(%esp), %eax                  # 4-byte Reload
	adcl	36(%ebx), %eax
	movl	%eax, 32(%esp)                  # 4-byte Spill
	movl	36(%esp), %ecx                  # 4-byte Reload
	adcl	40(%ebx), %ecx
	movl	%ecx, 36(%esp)                  # 4-byte Spill
	adcl	44(%ebx), %ebp
	movl	%ebp, 44(%esp)                  # 4-byte Spill
	movl	68(%esp), %ebx
	movl	%ebp, 44(%ebx)
	movl	%ecx, 40(%ebx)
	movl	16(%esp), %ecx                  # 4-byte Reload
	movl	%eax, 36(%ebx)
	movl	%edi, 28(%esp)                  # 4-byte Spill
	movl	%edi, 32(%ebx)
	movl	12(%esp), %eax                  # 4-byte Reload
	movl	%eax, 28(%ebx)
	movl	4(%esp), %eax                   # 4-byte Reload
	movl	%eax, 24(%ebx)
	movl	8(%esp), %eax                   # 4-byte Reload
	movl	%eax, 20(%ebx)
	movl	%esi, 16(%ebx)
	movl	%edx, 12(%ebx)
	movl	%ecx, 8(%ebx)
	movl	20(%esp), %edi                  # 4-byte Reload
	movl	%edi, 4(%ebx)
	movl	24(%esp), %eax                  # 4-byte Reload
	movl	%eax, (%ebx)
	setb	3(%esp)                         # 1-byte Folded Spill
	movl	80(%esp), %ebp
	subl	(%ebp), %eax
	movl	%eax, 24(%esp)                  # 4-byte Spill
	sbbl	4(%ebp), %edi
	movl	%edi, 20(%esp)                  # 4-byte Spill
	sbbl	8(%ebp), %ecx
	movl	%ecx, %edi
	sbbl	12(%ebp), %edx
	movl	%edx, 40(%esp)                  # 4-byte Spill
	sbbl	16(%ebp), %esi
	movl	%esi, 16(%esp)                  # 4-byte Spill
	movl	8(%esp), %eax                   # 4-byte Reload
	sbbl	20(%ebp), %eax
	movl	%eax, 8(%esp)                   # 4-byte Spill
	movl	4(%esp), %eax                   # 4-byte Reload
	sbbl	24(%ebp), %eax
	movl	%eax, 4(%esp)                   # 4-byte Spill
	movl	12(%esp), %eax                  # 4-byte Reload
	sbbl	28(%ebp), %eax
	movl	%eax, 12(%esp)                  # 4-byte Spill
	movl	28(%esp), %esi                  # 4-byte Reload
	sbbl	32(%ebp), %esi
	movl	32(%esp), %edx                  # 4-byte Reload
	sbbl	36(%ebp), %edx
	movl	36(%esp), %ecx                  # 4-byte Reload
	sbbl	40(%ebp), %ecx
	movl	44(%esp), %eax                  # 4-byte Reload
	sbbl	44(%ebp), %eax
	movl	%eax, %ebp
	movzbl	3(%esp), %eax                   # 1-byte Folded Reload
	sbbl	$0, %eax
	testb	$1, %al
	jne	.LBB67_2
# %bb.1:                                # %nocarry
	movl	24(%esp), %eax                  # 4-byte Reload
	movl	%eax, (%ebx)
	movl	20(%esp), %eax                  # 4-byte Reload
	movl	%eax, 4(%ebx)
	movl	%edi, 8(%ebx)
	movl	40(%esp), %eax                  # 4-byte Reload
	movl	%eax, 12(%ebx)
	movl	16(%esp), %eax                  # 4-byte Reload
	movl	%eax, 16(%ebx)
	movl	8(%esp), %eax                   # 4-byte Reload
	movl	%eax, 20(%ebx)
	movl	4(%esp), %eax                   # 4-byte Reload
	movl	%eax, 24(%ebx)
	movl	12(%esp), %eax                  # 4-byte Reload
	movl	%eax, 28(%ebx)
	movl	%esi, 32(%ebx)
	movl	%edx, 36(%ebx)
	movl	%ecx, 40(%ebx)
	movl	%ebp, 44(%ebx)
.LBB67_2:                               # %carry
	addl	$48, %esp
	popl	%esi
	popl	%edi
	popl	%ebx
	popl	%ebp
	retl
.Lfunc_end67:
	.size	mcl_fp_add12L, .Lfunc_end67-mcl_fp_add12L
                                        # -- End function
	.globl	mcl_fp_addNF12L                 # -- Begin function mcl_fp_addNF12L
	.p2align	4, 0x90
	.type	mcl_fp_addNF12L,@function
mcl_fp_addNF12L:                        # @mcl_fp_addNF12L
# %bb.0:
	pushl	%ebp
	pushl	%ebx
	pushl	%edi
	pushl	%esi
	subl	$72, %esp
	movl	100(%esp), %ecx
	movl	(%ecx), %edx
	movl	4(%ecx), %esi
	movl	96(%esp), %eax
	addl	(%eax), %edx
	movl	%edx, 32(%esp)                  # 4-byte Spill
	adcl	4(%eax), %esi
	movl	%esi, 28(%esp)                  # 4-byte Spill
	movl	44(%ecx), %esi
	movl	%esi, (%esp)                    # 4-byte Spill
	movl	40(%ecx), %esi
	movl	%esi, 4(%esp)                   # 4-byte Spill
	movl	36(%ecx), %esi
	movl	32(%ecx), %edi
	movl	%edi, 8(%esp)                   # 4-byte Spill
	movl	28(%ecx), %edi
	movl	24(%ecx), %edx
	movl	%edx, 12(%esp)                  # 4-byte Spill
	movl	20(%ecx), %edx
	movl	16(%ecx), %ebp
	movl	12(%ecx), %ebx
	movl	8(%ecx), %ecx
	adcl	8(%eax), %ecx
	adcl	12(%eax), %ebx
	adcl	16(%eax), %ebp
	adcl	20(%eax), %edx
	movl	%edx, 24(%esp)                  # 4-byte Spill
	movl	12(%esp), %edx                  # 4-byte Reload
	adcl	24(%eax), %edx
	movl	%edx, 12(%esp)                  # 4-byte Spill
	adcl	28(%eax), %edi
	movl	%edi, 20(%esp)                  # 4-byte Spill
	movl	8(%esp), %edi                   # 4-byte Reload
	adcl	32(%eax), %edi
	movl	%edi, 8(%esp)                   # 4-byte Spill
	adcl	36(%eax), %esi
	movl	%esi, 16(%esp)                  # 4-byte Spill
	movl	4(%esp), %esi                   # 4-byte Reload
	adcl	40(%eax), %esi
	movl	%esi, 4(%esp)                   # 4-byte Spill
	movl	(%esp), %esi                    # 4-byte Reload
	adcl	44(%eax), %esi
	movl	%esi, (%esp)                    # 4-byte Spill
	movl	104(%esp), %esi
	movl	32(%esp), %edx                  # 4-byte Reload
	subl	(%esi), %edx
	movl	%edx, 68(%esp)                  # 4-byte Spill
	movl	28(%esp), %edi                  # 4-byte Reload
	sbbl	4(%esi), %edi
	movl	%edi, 64(%esp)                  # 4-byte Spill
	movl	%ecx, 44(%esp)                  # 4-byte Spill
	sbbl	8(%esi), %ecx
	movl	%ecx, 60(%esp)                  # 4-byte Spill
	movl	%ebx, 40(%esp)                  # 4-byte Spill
	sbbl	12(%esi), %ebx
	movl	%ebx, 56(%esp)                  # 4-byte Spill
	movl	%ebp, 36(%esp)                  # 4-byte Spill
	sbbl	16(%esi), %ebp
	movl	%ebp, 52(%esp)                  # 4-byte Spill
	movl	24(%esp), %eax                  # 4-byte Reload
	sbbl	20(%esi), %eax
	movl	%eax, 48(%esp)                  # 4-byte Spill
	movl	12(%esp), %edi                  # 4-byte Reload
	sbbl	24(%esi), %edi
	movl	20(%esp), %ebx                  # 4-byte Reload
	sbbl	28(%esi), %ebx
	movl	8(%esp), %eax                   # 4-byte Reload
	sbbl	32(%esi), %eax
	movl	16(%esp), %ecx                  # 4-byte Reload
	sbbl	36(%esi), %ecx
	movl	4(%esp), %edx                   # 4-byte Reload
	sbbl	40(%esi), %edx
	movl	(%esp), %ebp                    # 4-byte Reload
	sbbl	44(%esi), %ebp
	movl	%ebp, %esi
	sarl	$31, %esi
	testl	%esi, %esi
	js	.LBB68_1
# %bb.2:
	movl	92(%esp), %esi
	movl	%ebp, 44(%esi)
	js	.LBB68_3
.LBB68_4:
	movl	%edx, 40(%esi)
	js	.LBB68_5
.LBB68_6:
	movl	%ecx, 36(%esi)
	movl	60(%esp), %edx                  # 4-byte Reload
	js	.LBB68_7
.LBB68_8:
	movl	%eax, 32(%esi)
	movl	64(%esp), %ecx                  # 4-byte Reload
	js	.LBB68_9
.LBB68_10:
	movl	%ebx, 28(%esi)
	movl	68(%esp), %eax                  # 4-byte Reload
	js	.LBB68_11
.LBB68_12:
	movl	%edi, 24(%esi)
	movl	52(%esp), %ebx                  # 4-byte Reload
	movl	48(%esp), %edi                  # 4-byte Reload
	js	.LBB68_13
.LBB68_14:
	movl	%edi, 20(%esi)
	movl	56(%esp), %edi                  # 4-byte Reload
	js	.LBB68_15
.LBB68_16:
	movl	%ebx, 16(%esi)
	js	.LBB68_17
.LBB68_18:
	movl	%edi, 12(%esi)
	js	.LBB68_19
.LBB68_20:
	movl	%edx, 8(%esi)
	js	.LBB68_21
.LBB68_22:
	movl	%ecx, 4(%esi)
	jns	.LBB68_24
.LBB68_23:
	movl	32(%esp), %eax                  # 4-byte Reload
.LBB68_24:
	movl	%eax, (%esi)
	addl	$72, %esp
	popl	%esi
	popl	%edi
	popl	%ebx
	popl	%ebp
	retl
.LBB68_1:
	movl	(%esp), %ebp                    # 4-byte Reload
	movl	92(%esp), %esi
	movl	%ebp, 44(%esi)
	jns	.LBB68_4
.LBB68_3:
	movl	4(%esp), %edx                   # 4-byte Reload
	movl	%edx, 40(%esi)
	jns	.LBB68_6
.LBB68_5:
	movl	16(%esp), %ecx                  # 4-byte Reload
	movl	%ecx, 36(%esi)
	movl	60(%esp), %edx                  # 4-byte Reload
	jns	.LBB68_8
.LBB68_7:
	movl	8(%esp), %eax                   # 4-byte Reload
	movl	%eax, 32(%esi)
	movl	64(%esp), %ecx                  # 4-byte Reload
	jns	.LBB68_10
.LBB68_9:
	movl	20(%esp), %ebx                  # 4-byte Reload
	movl	%ebx, 28(%esi)
	movl	68(%esp), %eax                  # 4-byte Reload
	jns	.LBB68_12
.LBB68_11:
	movl	12(%esp), %edi                  # 4-byte Reload
	movl	%edi, 24(%esi)
	movl	52(%esp), %ebx                  # 4-byte Reload
	movl	48(%esp), %edi                  # 4-byte Reload
	jns	.LBB68_14
.LBB68_13:
	movl	24(%esp), %edi                  # 4-byte Reload
	movl	%edi, 20(%esi)
	movl	56(%esp), %edi                  # 4-byte Reload
	jns	.LBB68_16
.LBB68_15:
	movl	36(%esp), %ebx                  # 4-byte Reload
	movl	%ebx, 16(%esi)
	jns	.LBB68_18
.LBB68_17:
	movl	40(%esp), %edi                  # 4-byte Reload
	movl	%edi, 12(%esi)
	jns	.LBB68_20
.LBB68_19:
	movl	44(%esp), %edx                  # 4-byte Reload
	movl	%edx, 8(%esi)
	jns	.LBB68_22
.LBB68_21:
	movl	28(%esp), %ecx                  # 4-byte Reload
	movl	%ecx, 4(%esi)
	js	.LBB68_23
	jmp	.LBB68_24
.Lfunc_end68:
	.size	mcl_fp_addNF12L, .Lfunc_end68-mcl_fp_addNF12L
                                        # -- End function
	.globl	mcl_fp_sub12L                   # -- Begin function mcl_fp_sub12L
	.p2align	4, 0x90
	.type	mcl_fp_sub12L,@function
mcl_fp_sub12L:                          # @mcl_fp_sub12L
# %bb.0:
	pushl	%ebp
	pushl	%ebx
	pushl	%edi
	pushl	%esi
	subl	$48, %esp
	movl	72(%esp), %edx
	movl	(%edx), %ecx
	movl	4(%edx), %esi
	movl	76(%esp), %eax
	subl	(%eax), %ecx
	movl	%ecx, 36(%esp)                  # 4-byte Spill
	sbbl	4(%eax), %esi
	movl	%esi, (%esp)                    # 4-byte Spill
	movl	44(%edx), %ecx
	movl	%ecx, 40(%esp)                  # 4-byte Spill
	movl	40(%edx), %ecx
	movl	%ecx, 4(%esp)                   # 4-byte Spill
	movl	36(%edx), %ecx
	movl	%ecx, 8(%esp)                   # 4-byte Spill
	movl	32(%edx), %ecx
	movl	%ecx, 12(%esp)                  # 4-byte Spill
	movl	28(%edx), %ecx
	movl	%ecx, 16(%esp)                  # 4-byte Spill
	movl	24(%edx), %esi
	movl	20(%edx), %ebp
	movl	16(%edx), %ebx
	movl	12(%edx), %edi
	movl	8(%edx), %ecx
	sbbl	8(%eax), %ecx
	sbbl	12(%eax), %edi
	sbbl	16(%eax), %ebx
	sbbl	20(%eax), %ebp
	sbbl	24(%eax), %esi
	movl	16(%esp), %edx                  # 4-byte Reload
	sbbl	28(%eax), %edx
	movl	%edx, 16(%esp)                  # 4-byte Spill
	movl	12(%esp), %edx                  # 4-byte Reload
	sbbl	32(%eax), %edx
	movl	%edx, 12(%esp)                  # 4-byte Spill
	movl	8(%esp), %edx                   # 4-byte Reload
	sbbl	36(%eax), %edx
	movl	%edx, 8(%esp)                   # 4-byte Spill
	movl	4(%esp), %edx                   # 4-byte Reload
	sbbl	40(%eax), %edx
	movl	%edx, 4(%esp)                   # 4-byte Spill
	movl	40(%esp), %edx                  # 4-byte Reload
	sbbl	44(%eax), %edx
	movl	$0, %eax
	sbbl	%eax, %eax
	testb	$1, %al
	movl	68(%esp), %eax
	movl	%edx, 40(%esp)                  # 4-byte Spill
	movl	%edx, 44(%eax)
	movl	4(%esp), %edx                   # 4-byte Reload
	movl	%edx, 40(%eax)
	movl	8(%esp), %edx                   # 4-byte Reload
	movl	%edx, 36(%eax)
	movl	12(%esp), %edx                  # 4-byte Reload
	movl	%edx, 32(%eax)
	movl	16(%esp), %edx                  # 4-byte Reload
	movl	%edx, 28(%eax)
	movl	%esi, 20(%esp)                  # 4-byte Spill
	movl	%esi, 24(%eax)
	movl	%ebp, 24(%esp)                  # 4-byte Spill
	movl	%ebp, 20(%eax)
	movl	%ebx, 28(%esp)                  # 4-byte Spill
	movl	%ebx, 16(%eax)
	movl	%edi, 32(%esp)                  # 4-byte Spill
	movl	%edi, 12(%eax)
	movl	%ecx, %edx
	movl	%ecx, 8(%eax)
	movl	(%esp), %ecx                    # 4-byte Reload
	movl	%ecx, 4(%eax)
	movl	36(%esp), %esi                  # 4-byte Reload
	movl	%esi, (%eax)
	je	.LBB69_2
# %bb.1:                                # %carry
	movl	%esi, %ecx
	movl	80(%esp), %ecx
	addl	(%ecx), %esi
	movl	%esi, 36(%esp)                  # 4-byte Spill
	movl	(%esp), %esi                    # 4-byte Reload
	adcl	4(%ecx), %esi
	movl	%esi, (%esp)                    # 4-byte Spill
	adcl	8(%ecx), %edx
	movl	%edx, 44(%esp)                  # 4-byte Spill
	movl	32(%esp), %edx                  # 4-byte Reload
	adcl	12(%ecx), %edx
	movl	%edx, 32(%esp)                  # 4-byte Spill
	movl	28(%esp), %edx                  # 4-byte Reload
	adcl	16(%ecx), %edx
	movl	%edx, 28(%esp)                  # 4-byte Spill
	movl	24(%esp), %edx                  # 4-byte Reload
	adcl	20(%ecx), %edx
	movl	%edx, 24(%esp)                  # 4-byte Spill
	movl	20(%esp), %edx                  # 4-byte Reload
	adcl	24(%ecx), %edx
	movl	%edx, 20(%esp)                  # 4-byte Spill
	movl	16(%esp), %ebx                  # 4-byte Reload
	adcl	28(%ecx), %ebx
	movl	12(%esp), %edi                  # 4-byte Reload
	adcl	32(%ecx), %edi
	movl	8(%esp), %esi                   # 4-byte Reload
	adcl	36(%ecx), %esi
	movl	4(%esp), %edx                   # 4-byte Reload
	adcl	40(%ecx), %edx
	movl	40(%esp), %ebp                  # 4-byte Reload
	adcl	44(%ecx), %ebp
	movl	%ebp, 44(%eax)
	movl	%edx, 40(%eax)
	movl	%esi, 36(%eax)
	movl	%edi, 32(%eax)
	movl	%ebx, 28(%eax)
	movl	20(%esp), %ecx                  # 4-byte Reload
	movl	%ecx, 24(%eax)
	movl	24(%esp), %ecx                  # 4-byte Reload
	movl	%ecx, 20(%eax)
	movl	28(%esp), %ecx                  # 4-byte Reload
	movl	%ecx, 16(%eax)
	movl	32(%esp), %ecx                  # 4-byte Reload
	movl	%ecx, 12(%eax)
	movl	44(%esp), %ecx                  # 4-byte Reload
	movl	%ecx, 8(%eax)
	movl	(%esp), %ecx                    # 4-byte Reload
	movl	%ecx, 4(%eax)
	movl	36(%esp), %ecx                  # 4-byte Reload
	movl	%ecx, (%eax)
.LBB69_2:                               # %nocarry
	addl	$48, %esp
	popl	%esi
	popl	%edi
	popl	%ebx
	popl	%ebp
	retl
.Lfunc_end69:
	.size	mcl_fp_sub12L, .Lfunc_end69-mcl_fp_sub12L
                                        # -- End function
	.globl	mcl_fp_subNF12L                 # -- Begin function mcl_fp_subNF12L
	.p2align	4, 0x90
	.type	mcl_fp_subNF12L,@function
mcl_fp_subNF12L:                        # @mcl_fp_subNF12L
# %bb.0:
	pushl	%ebp
	pushl	%ebx
	pushl	%edi
	pushl	%esi
	subl	$72, %esp
	movl	96(%esp), %ecx
	movl	(%ecx), %edx
	movl	4(%ecx), %eax
	movl	100(%esp), %edi
	subl	(%edi), %edx
	movl	%edx, 64(%esp)                  # 4-byte Spill
	sbbl	4(%edi), %eax
	movl	%eax, 68(%esp)                  # 4-byte Spill
	movl	44(%ecx), %eax
	movl	%eax, (%esp)                    # 4-byte Spill
	movl	40(%ecx), %eax
	movl	%eax, 12(%esp)                  # 4-byte Spill
	movl	36(%ecx), %eax
	movl	%eax, 8(%esp)                   # 4-byte Spill
	movl	32(%ecx), %eax
	movl	%eax, 4(%esp)                   # 4-byte Spill
	movl	28(%ecx), %ebp
	movl	24(%ecx), %ebx
	movl	20(%ecx), %esi
	movl	16(%ecx), %edx
	movl	12(%ecx), %eax
	movl	8(%ecx), %ecx
	sbbl	8(%edi), %ecx
	movl	%ecx, 40(%esp)                  # 4-byte Spill
	sbbl	12(%edi), %eax
	movl	%eax, 44(%esp)                  # 4-byte Spill
	sbbl	16(%edi), %edx
	movl	%edx, 48(%esp)                  # 4-byte Spill
	sbbl	20(%edi), %esi
	movl	%esi, 52(%esp)                  # 4-byte Spill
	sbbl	24(%edi), %ebx
	movl	%ebx, 56(%esp)                  # 4-byte Spill
	sbbl	28(%edi), %ebp
	movl	%ebp, 60(%esp)                  # 4-byte Spill
	movl	4(%esp), %eax                   # 4-byte Reload
	sbbl	32(%edi), %eax
	movl	%eax, 4(%esp)                   # 4-byte Spill
	movl	8(%esp), %eax                   # 4-byte Reload
	sbbl	36(%edi), %eax
	movl	%eax, 8(%esp)                   # 4-byte Spill
	movl	12(%esp), %eax                  # 4-byte Reload
	sbbl	40(%edi), %eax
	movl	%eax, 12(%esp)                  # 4-byte Spill
	movl	(%esp), %eax                    # 4-byte Reload
	sbbl	44(%edi), %eax
	movl	%eax, (%esp)                    # 4-byte Spill
	movl	%eax, %edi
	sarl	$31, %edi
	movl	%edi, %edx
	shldl	$1, %eax, %edx
	movl	104(%esp), %ebx
	andl	(%ebx), %edx
	movl	44(%ebx), %eax
	andl	%edi, %eax
	movl	%eax, 36(%esp)                  # 4-byte Spill
	movl	40(%ebx), %eax
	andl	%edi, %eax
	movl	%eax, 32(%esp)                  # 4-byte Spill
	movl	36(%ebx), %eax
	andl	%edi, %eax
	movl	%eax, 28(%esp)                  # 4-byte Spill
	movl	32(%ebx), %eax
	andl	%edi, %eax
	movl	%eax, 24(%esp)                  # 4-byte Spill
	movl	28(%ebx), %eax
	andl	%edi, %eax
	movl	%eax, 20(%esp)                  # 4-byte Spill
	movl	24(%ebx), %eax
	andl	%edi, %eax
	movl	%eax, 16(%esp)                  # 4-byte Spill
	movl	20(%ebx), %ebp
	andl	%edi, %ebp
	movl	16(%ebx), %esi
	andl	%edi, %esi
	movl	12(%ebx), %ecx
	andl	%edi, %ecx
	movl	8(%ebx), %eax
	andl	%edi, %eax
	andl	4(%ebx), %edi
	addl	64(%esp), %edx                  # 4-byte Folded Reload
	adcl	68(%esp), %edi                  # 4-byte Folded Reload
	movl	92(%esp), %ebx
	movl	%edx, (%ebx)
	adcl	40(%esp), %eax                  # 4-byte Folded Reload
	movl	%edi, 4(%ebx)
	adcl	44(%esp), %ecx                  # 4-byte Folded Reload
	movl	%eax, 8(%ebx)
	adcl	48(%esp), %esi                  # 4-byte Folded Reload
	movl	%ecx, 12(%ebx)
	adcl	52(%esp), %ebp                  # 4-byte Folded Reload
	movl	%esi, 16(%ebx)
	movl	16(%esp), %eax                  # 4-byte Reload
	adcl	56(%esp), %eax                  # 4-byte Folded Reload
	movl	%ebp, 20(%ebx)
	movl	20(%esp), %ecx                  # 4-byte Reload
	adcl	60(%esp), %ecx                  # 4-byte Folded Reload
	movl	%eax, 24(%ebx)
	movl	24(%esp), %eax                  # 4-byte Reload
	adcl	4(%esp), %eax                   # 4-byte Folded Reload
	movl	%ecx, 28(%ebx)
	movl	28(%esp), %ecx                  # 4-byte Reload
	adcl	8(%esp), %ecx                   # 4-byte Folded Reload
	movl	%eax, 32(%ebx)
	movl	32(%esp), %eax                  # 4-byte Reload
	adcl	12(%esp), %eax                  # 4-byte Folded Reload
	movl	%ecx, 36(%ebx)
	movl	%eax, 40(%ebx)
	movl	36(%esp), %eax                  # 4-byte Reload
	adcl	(%esp), %eax                    # 4-byte Folded Reload
	movl	%eax, 44(%ebx)
	addl	$72, %esp
	popl	%esi
	popl	%edi
	popl	%ebx
	popl	%ebp
	retl
.Lfunc_end70:
	.size	mcl_fp_subNF12L, .Lfunc_end70-mcl_fp_subNF12L
                                        # -- End function
	.globl	mcl_fpDbl_add12L                # -- Begin function mcl_fpDbl_add12L
	.p2align	4, 0x90
	.type	mcl_fpDbl_add12L,@function
mcl_fpDbl_add12L:                       # @mcl_fpDbl_add12L
# %bb.0:
	pushl	%ebp
	pushl	%ebx
	pushl	%edi
	pushl	%esi
	subl	$92, %esp
	movl	116(%esp), %esi
	movl	(%esi), %eax
	movl	4(%esi), %ecx
	movl	120(%esp), %edx
	addl	(%edx), %eax
	movl	%eax, 76(%esp)                  # 4-byte Spill
	adcl	4(%edx), %ecx
	movl	%ecx, 72(%esp)                  # 4-byte Spill
	movl	92(%esi), %eax
	movl	%eax, 4(%esp)                   # 4-byte Spill
	movl	88(%esi), %eax
	movl	%eax, 8(%esp)                   # 4-byte Spill
	movl	84(%esi), %eax
	movl	%eax, 12(%esp)                  # 4-byte Spill
	movl	80(%esi), %eax
	movl	%eax, 16(%esp)                  # 4-byte Spill
	movl	76(%esi), %eax
	movl	%eax, 20(%esp)                  # 4-byte Spill
	movl	72(%esi), %eax
	movl	%eax, 24(%esp)                  # 4-byte Spill
	movl	68(%esi), %eax
	movl	%eax, 28(%esp)                  # 4-byte Spill
	movl	64(%esi), %eax
	movl	%eax, 32(%esp)                  # 4-byte Spill
	movl	60(%esi), %eax
	movl	%eax, 36(%esp)                  # 4-byte Spill
	movl	56(%esi), %eax
	movl	%eax, 40(%esp)                  # 4-byte Spill
	movl	52(%esi), %eax
	movl	%eax, 44(%esp)                  # 4-byte Spill
	movl	48(%esi), %eax
	movl	%eax, 80(%esp)                  # 4-byte Spill
	movl	44(%esi), %eax
	movl	%eax, 48(%esp)                  # 4-byte Spill
	movl	40(%esi), %eax
	movl	%eax, (%esp)                    # 4-byte Spill
	movl	36(%esi), %eax
	movl	%eax, 88(%esp)                  # 4-byte Spill
	movl	32(%esi), %eax
	movl	%eax, 84(%esp)                  # 4-byte Spill
	movl	28(%esi), %edi
	movl	24(%esi), %ecx
	movl	20(%esi), %ebx
	movl	16(%esi), %ebp
	movl	12(%esi), %eax
	movl	8(%esi), %esi
	adcl	8(%edx), %esi
	movl	%esi, 68(%esp)                  # 4-byte Spill
	adcl	12(%edx), %eax
	movl	%eax, 64(%esp)                  # 4-byte Spill
	adcl	16(%edx), %ebp
	adcl	20(%edx), %ebx
	movl	%ebx, 60(%esp)                  # 4-byte Spill
	adcl	24(%edx), %ecx
	movl	%ecx, 56(%esp)                  # 4-byte Spill
	adcl	28(%edx), %edi
	movl	%edi, 52(%esp)                  # 4-byte Spill
	movl	84(%esp), %edi                  # 4-byte Reload
	adcl	32(%edx), %edi
	movl	88(%esp), %ecx                  # 4-byte Reload
	adcl	36(%edx), %ecx
	movl	(%esp), %esi                    # 4-byte Reload
	adcl	40(%edx), %esi
	movl	%esi, (%esp)                    # 4-byte Spill
	movl	48(%esp), %eax                  # 4-byte Reload
	adcl	44(%edx), %eax
	movl	80(%esp), %ebx                  # 4-byte Reload
	adcl	48(%edx), %ebx
	movl	44(%esp), %esi                  # 4-byte Reload
	adcl	52(%edx), %esi
	movl	%esi, 44(%esp)                  # 4-byte Spill
	movl	40(%esp), %esi                  # 4-byte Reload
	adcl	56(%edx), %esi
	movl	%esi, 40(%esp)                  # 4-byte Spill
	movl	36(%esp), %esi                  # 4-byte Reload
	adcl	60(%edx), %esi
	movl	%esi, 36(%esp)                  # 4-byte Spill
	movl	32(%esp), %esi                  # 4-byte Reload
	adcl	64(%edx), %esi
	movl	%esi, 32(%esp)                  # 4-byte Spill
	movl	28(%esp), %esi                  # 4-byte Reload
	adcl	68(%edx), %esi
	movl	%esi, 28(%esp)                  # 4-byte Spill
	movl	24(%esp), %esi                  # 4-byte Reload
	adcl	72(%edx), %esi
	movl	%esi, 24(%esp)                  # 4-byte Spill
	movl	20(%esp), %esi                  # 4-byte Reload
	adcl	76(%edx), %esi
	movl	%esi, 20(%esp)                  # 4-byte Spill
	movl	16(%esp), %esi                  # 4-byte Reload
	adcl	80(%edx), %esi
	movl	%esi, 16(%esp)                  # 4-byte Spill
	movl	12(%esp), %esi                  # 4-byte Reload
	adcl	84(%edx), %esi
	movl	%esi, 12(%esp)                  # 4-byte Spill
	movl	8(%esp), %esi                   # 4-byte Reload
	adcl	88(%edx), %esi
	movl	%esi, 8(%esp)                   # 4-byte Spill
	movl	4(%esp), %esi                   # 4-byte Reload
	adcl	92(%edx), %esi
	movl	%esi, 4(%esp)                   # 4-byte Spill
	movl	112(%esp), %edx
	movl	%eax, 44(%edx)
	movl	(%esp), %esi                    # 4-byte Reload
	movl	%esi, 40(%edx)
	movl	%ecx, 36(%edx)
	movl	%edi, 32(%edx)
	movl	52(%esp), %eax                  # 4-byte Reload
	movl	%eax, 28(%edx)
	movl	56(%esp), %eax                  # 4-byte Reload
	movl	%eax, 24(%edx)
	movl	60(%esp), %eax                  # 4-byte Reload
	movl	%eax, 20(%edx)
	movl	%ebp, 16(%edx)
	movl	64(%esp), %eax                  # 4-byte Reload
	movl	%eax, 12(%edx)
	movl	68(%esp), %eax                  # 4-byte Reload
	movl	%eax, 8(%edx)
	movl	72(%esp), %esi                  # 4-byte Reload
	movl	%esi, 4(%edx)
	movl	76(%esp), %esi                  # 4-byte Reload
	movl	%esi, (%edx)
	setb	48(%esp)                        # 1-byte Folded Spill
	movl	124(%esp), %ebp
	movl	%ebx, %eax
	movl	%ebx, 80(%esp)                  # 4-byte Spill
	subl	(%ebp), %eax
	movl	%eax, 76(%esp)                  # 4-byte Spill
	movl	44(%esp), %ecx                  # 4-byte Reload
	sbbl	4(%ebp), %ecx
	movl	%ecx, 72(%esp)                  # 4-byte Spill
	movl	40(%esp), %eax                  # 4-byte Reload
	sbbl	8(%ebp), %eax
	movl	%eax, 68(%esp)                  # 4-byte Spill
	movl	36(%esp), %eax                  # 4-byte Reload
	sbbl	12(%ebp), %eax
	movl	%eax, 64(%esp)                  # 4-byte Spill
	movl	32(%esp), %eax                  # 4-byte Reload
	sbbl	16(%ebp), %eax
	movl	%eax, 60(%esp)                  # 4-byte Spill
	movl	28(%esp), %eax                  # 4-byte Reload
	sbbl	20(%ebp), %eax
	movl	%eax, 56(%esp)                  # 4-byte Spill
	movl	24(%esp), %eax                  # 4-byte Reload
	sbbl	24(%ebp), %eax
	movl	%eax, (%esp)                    # 4-byte Spill
	movl	20(%esp), %eax                  # 4-byte Reload
	sbbl	28(%ebp), %eax
	movl	%eax, 52(%esp)                  # 4-byte Spill
	movl	16(%esp), %ecx                  # 4-byte Reload
	sbbl	32(%ebp), %ecx
	movl	12(%esp), %esi                  # 4-byte Reload
	sbbl	36(%ebp), %esi
	movl	8(%esp), %edi                   # 4-byte Reload
	sbbl	40(%ebp), %edi
	movl	4(%esp), %ebx                   # 4-byte Reload
	sbbl	44(%ebp), %ebx
	movzbl	48(%esp), %eax                  # 1-byte Folded Reload
	sbbl	$0, %eax
	testb	$1, %al
	jne	.LBB71_1
# %bb.2:
	movl	%ebx, 92(%edx)
	jne	.LBB71_3
.LBB71_4:
	movl	%edi, 88(%edx)
	movl	60(%esp), %ebx                  # 4-byte Reload
	movl	56(%esp), %ebp                  # 4-byte Reload
	jne	.LBB71_5
.LBB71_6:
	movl	%esi, 84(%edx)
	movl	64(%esp), %edi                  # 4-byte Reload
	jne	.LBB71_7
.LBB71_8:
	movl	%ecx, 80(%edx)
	movl	68(%esp), %esi                  # 4-byte Reload
	movl	52(%esp), %eax                  # 4-byte Reload
	jne	.LBB71_9
.LBB71_10:
	movl	%eax, 76(%edx)
	movl	72(%esp), %ecx                  # 4-byte Reload
	jne	.LBB71_11
.LBB71_12:
	movl	(%esp), %eax                    # 4-byte Reload
	movl	%eax, 72(%edx)
	jne	.LBB71_13
.LBB71_14:
	movl	%ebp, 68(%edx)
	jne	.LBB71_15
.LBB71_16:
	movl	%ebx, 64(%edx)
	movl	76(%esp), %eax                  # 4-byte Reload
	jne	.LBB71_17
.LBB71_18:
	movl	%edi, 60(%edx)
	jne	.LBB71_19
.LBB71_20:
	movl	%esi, 56(%edx)
	jne	.LBB71_21
.LBB71_22:
	movl	%ecx, 52(%edx)
	je	.LBB71_24
.LBB71_23:
	movl	80(%esp), %eax                  # 4-byte Reload
.LBB71_24:
	movl	%eax, 48(%edx)
	addl	$92, %esp
	popl	%esi
	popl	%edi
	popl	%ebx
	popl	%ebp
	retl
.LBB71_1:
	movl	4(%esp), %ebx                   # 4-byte Reload
	movl	%ebx, 92(%edx)
	je	.LBB71_4
.LBB71_3:
	movl	8(%esp), %edi                   # 4-byte Reload
	movl	%edi, 88(%edx)
	movl	60(%esp), %ebx                  # 4-byte Reload
	movl	56(%esp), %ebp                  # 4-byte Reload
	je	.LBB71_6
.LBB71_5:
	movl	12(%esp), %esi                  # 4-byte Reload
	movl	%esi, 84(%edx)
	movl	64(%esp), %edi                  # 4-byte Reload
	je	.LBB71_8
.LBB71_7:
	movl	16(%esp), %ecx                  # 4-byte Reload
	movl	%ecx, 80(%edx)
	movl	68(%esp), %esi                  # 4-byte Reload
	movl	52(%esp), %eax                  # 4-byte Reload
	je	.LBB71_10
.LBB71_9:
	movl	20(%esp), %eax                  # 4-byte Reload
	movl	%eax, 76(%edx)
	movl	72(%esp), %ecx                  # 4-byte Reload
	je	.LBB71_12
.LBB71_11:
	movl	24(%esp), %eax                  # 4-byte Reload
	movl	%eax, (%esp)                    # 4-byte Spill
	movl	(%esp), %eax                    # 4-byte Reload
	movl	%eax, 72(%edx)
	je	.LBB71_14
.LBB71_13:
	movl	28(%esp), %ebp                  # 4-byte Reload
	movl	%ebp, 68(%edx)
	je	.LBB71_16
.LBB71_15:
	movl	32(%esp), %ebx                  # 4-byte Reload
	movl	%ebx, 64(%edx)
	movl	76(%esp), %eax                  # 4-byte Reload
	je	.LBB71_18
.LBB71_17:
	movl	36(%esp), %edi                  # 4-byte Reload
	movl	%edi, 60(%edx)
	je	.LBB71_20
.LBB71_19:
	movl	40(%esp), %esi                  # 4-byte Reload
	movl	%esi, 56(%edx)
	je	.LBB71_22
.LBB71_21:
	movl	44(%esp), %ecx                  # 4-byte Reload
	movl	%ecx, 52(%edx)
	jne	.LBB71_23
	jmp	.LBB71_24
.Lfunc_end71:
	.size	mcl_fpDbl_add12L, .Lfunc_end71-mcl_fpDbl_add12L
                                        # -- End function
	.globl	mcl_fpDbl_sub12L                # -- Begin function mcl_fpDbl_sub12L
	.p2align	4, 0x90
	.type	mcl_fpDbl_sub12L,@function
mcl_fpDbl_sub12L:                       # @mcl_fpDbl_sub12L
# %bb.0:
	pushl	%ebp
	pushl	%ebx
	pushl	%edi
	pushl	%esi
	subl	$92, %esp
	movl	116(%esp), %esi
	movl	(%esi), %ecx
	movl	4(%esi), %eax
	xorl	%ebp, %ebp
	movl	120(%esp), %edx
	subl	(%edx), %ecx
	movl	%ecx, 28(%esp)                  # 4-byte Spill
	sbbl	4(%edx), %eax
	movl	%eax, 20(%esp)                  # 4-byte Spill
	movl	92(%esi), %ecx
	movl	%ecx, 40(%esp)                  # 4-byte Spill
	movl	88(%esi), %ecx
	movl	%ecx, 72(%esp)                  # 4-byte Spill
	movl	84(%esi), %ecx
	movl	%ecx, 68(%esp)                  # 4-byte Spill
	movl	80(%esi), %ecx
	movl	%ecx, 64(%esp)                  # 4-byte Spill
	movl	76(%esi), %ecx
	movl	%ecx, 60(%esp)                  # 4-byte Spill
	movl	72(%esi), %ecx
	movl	%ecx, 56(%esp)                  # 4-byte Spill
	movl	68(%esi), %ecx
	movl	%ecx, 52(%esp)                  # 4-byte Spill
	movl	64(%esi), %ecx
	movl	%ecx, 48(%esp)                  # 4-byte Spill
	movl	60(%esi), %ecx
	movl	%ecx, 44(%esp)                  # 4-byte Spill
	movl	56(%esi), %eax
	movl	%eax, 36(%esp)                  # 4-byte Spill
	movl	52(%esi), %eax
	movl	%eax, 32(%esp)                  # 4-byte Spill
	movl	48(%esi), %ebx
	movl	44(%esi), %eax
	movl	%eax, (%esp)                    # 4-byte Spill
	movl	40(%esi), %eax
	movl	%eax, 4(%esp)                   # 4-byte Spill
	movl	36(%esi), %eax
	movl	%eax, 8(%esp)                   # 4-byte Spill
	movl	32(%esi), %eax
	movl	%eax, 12(%esp)                  # 4-byte Spill
	movl	28(%esi), %eax
	movl	%eax, 16(%esp)                  # 4-byte Spill
	movl	24(%esi), %edi
	movl	20(%esi), %ecx
	movl	16(%esi), %eax
	movl	%eax, 24(%esp)                  # 4-byte Spill
	movl	12(%esi), %eax
	movl	8(%esi), %esi
	sbbl	8(%edx), %esi
	movl	%esi, 88(%esp)                  # 4-byte Spill
	sbbl	12(%edx), %eax
	movl	%eax, 84(%esp)                  # 4-byte Spill
	movl	24(%esp), %eax                  # 4-byte Reload
	sbbl	16(%edx), %eax
	sbbl	20(%edx), %ecx
	movl	%ecx, 80(%esp)                  # 4-byte Spill
	sbbl	24(%edx), %edi
	movl	%edi, 76(%esp)                  # 4-byte Spill
	movl	16(%esp), %ecx                  # 4-byte Reload
	sbbl	28(%edx), %ecx
	movl	%ecx, 16(%esp)                  # 4-byte Spill
	movl	12(%esp), %ecx                  # 4-byte Reload
	sbbl	32(%edx), %ecx
	movl	%ecx, 12(%esp)                  # 4-byte Spill
	movl	8(%esp), %ecx                   # 4-byte Reload
	sbbl	36(%edx), %ecx
	movl	%ecx, 8(%esp)                   # 4-byte Spill
	movl	4(%esp), %ecx                   # 4-byte Reload
	sbbl	40(%edx), %ecx
	movl	%ecx, 4(%esp)                   # 4-byte Spill
	movl	(%esp), %ecx                    # 4-byte Reload
	sbbl	44(%edx), %ecx
	movl	%ecx, (%esp)                    # 4-byte Spill
	sbbl	48(%edx), %ebx
	movl	%ebx, 24(%esp)                  # 4-byte Spill
	movl	32(%esp), %ecx                  # 4-byte Reload
	sbbl	52(%edx), %ecx
	movl	%ecx, 32(%esp)                  # 4-byte Spill
	movl	36(%esp), %ecx                  # 4-byte Reload
	sbbl	56(%edx), %ecx
	movl	%ecx, 36(%esp)                  # 4-byte Spill
	movl	44(%esp), %esi                  # 4-byte Reload
	sbbl	60(%edx), %esi
	movl	%esi, 44(%esp)                  # 4-byte Spill
	movl	48(%esp), %esi                  # 4-byte Reload
	sbbl	64(%edx), %esi
	movl	%esi, 48(%esp)                  # 4-byte Spill
	movl	52(%esp), %esi                  # 4-byte Reload
	sbbl	68(%edx), %esi
	movl	%esi, 52(%esp)                  # 4-byte Spill
	movl	56(%esp), %esi                  # 4-byte Reload
	sbbl	72(%edx), %esi
	movl	%esi, 56(%esp)                  # 4-byte Spill
	movl	60(%esp), %esi                  # 4-byte Reload
	sbbl	76(%edx), %esi
	movl	%esi, 60(%esp)                  # 4-byte Spill
	movl	64(%esp), %esi                  # 4-byte Reload
	sbbl	80(%edx), %esi
	movl	%esi, 64(%esp)                  # 4-byte Spill
	movl	68(%esp), %esi                  # 4-byte Reload
	sbbl	84(%edx), %esi
	movl	%esi, 68(%esp)                  # 4-byte Spill
	movl	72(%esp), %esi                  # 4-byte Reload
	sbbl	88(%edx), %esi
	movl	%esi, 72(%esp)                  # 4-byte Spill
	movl	40(%esp), %ecx                  # 4-byte Reload
	sbbl	92(%edx), %ecx
	movl	%ecx, 40(%esp)                  # 4-byte Spill
	movl	112(%esp), %edi
	movl	(%esp), %ecx                    # 4-byte Reload
	movl	%ecx, 44(%edi)
	movl	4(%esp), %ecx                   # 4-byte Reload
	movl	%ecx, 40(%edi)
	movl	8(%esp), %ecx                   # 4-byte Reload
	movl	%ecx, 36(%edi)
	movl	12(%esp), %ecx                  # 4-byte Reload
	movl	%ecx, 32(%edi)
	movl	16(%esp), %ecx                  # 4-byte Reload
	movl	%ecx, 28(%edi)
	movl	76(%esp), %ecx                  # 4-byte Reload
	movl	%ecx, 24(%edi)
	movl	80(%esp), %ecx                  # 4-byte Reload
	movl	%ecx, 20(%edi)
	movl	%eax, 16(%edi)
	movl	84(%esp), %eax                  # 4-byte Reload
	movl	%eax, 12(%edi)
	movl	88(%esp), %eax                  # 4-byte Reload
	movl	%eax, 8(%edi)
	movl	20(%esp), %eax                  # 4-byte Reload
	movl	%eax, 4(%edi)
	movl	28(%esp), %eax                  # 4-byte Reload
	movl	%eax, (%edi)
	sbbl	%ebp, %ebp
	andl	$1, %ebp
	negl	%ebp
	movl	124(%esp), %ebx
	movl	44(%ebx), %eax
	andl	%ebp, %eax
	movl	%eax, 16(%esp)                  # 4-byte Spill
	movl	40(%ebx), %eax
	andl	%ebp, %eax
	movl	%eax, 12(%esp)                  # 4-byte Spill
	movl	36(%ebx), %eax
	andl	%ebp, %eax
	movl	%eax, 8(%esp)                   # 4-byte Spill
	movl	32(%ebx), %eax
	andl	%ebp, %eax
	movl	%eax, 4(%esp)                   # 4-byte Spill
	movl	28(%ebx), %eax
	andl	%ebp, %eax
	movl	%eax, (%esp)                    # 4-byte Spill
	movl	24(%ebx), %eax
	andl	%ebp, %eax
	movl	%eax, 28(%esp)                  # 4-byte Spill
	movl	20(%ebx), %eax
	andl	%ebp, %eax
	movl	%eax, 20(%esp)                  # 4-byte Spill
	movl	16(%ebx), %esi
	andl	%ebp, %esi
	movl	12(%ebx), %edx
	andl	%ebp, %edx
	movl	8(%ebx), %ecx
	andl	%ebp, %ecx
	movl	4(%ebx), %eax
	andl	%ebp, %eax
	andl	(%ebx), %ebp
	addl	24(%esp), %ebp                  # 4-byte Folded Reload
	adcl	32(%esp), %eax                  # 4-byte Folded Reload
	movl	%ebp, 48(%edi)
	adcl	36(%esp), %ecx                  # 4-byte Folded Reload
	movl	%eax, 52(%edi)
	adcl	44(%esp), %edx                  # 4-byte Folded Reload
	movl	%ecx, 56(%edi)
	adcl	48(%esp), %esi                  # 4-byte Folded Reload
	movl	%edx, 60(%edi)
	movl	20(%esp), %ecx                  # 4-byte Reload
	adcl	52(%esp), %ecx                  # 4-byte Folded Reload
	movl	%esi, 64(%edi)
	movl	28(%esp), %eax                  # 4-byte Reload
	adcl	56(%esp), %eax                  # 4-byte Folded Reload
	movl	%ecx, 68(%edi)
	movl	(%esp), %ecx                    # 4-byte Reload
	adcl	60(%esp), %ecx                  # 4-byte Folded Reload
	movl	%eax, 72(%edi)
	movl	4(%esp), %eax                   # 4-byte Reload
	adcl	64(%esp), %eax                  # 4-byte Folded Reload
	movl	%ecx, 76(%edi)
	movl	8(%esp), %ecx                   # 4-byte Reload
	adcl	68(%esp), %ecx                  # 4-byte Folded Reload
	movl	%eax, 80(%edi)
	movl	12(%esp), %eax                  # 4-byte Reload
	adcl	72(%esp), %eax                  # 4-byte Folded Reload
	movl	%ecx, 84(%edi)
	movl	%eax, 88(%edi)
	movl	16(%esp), %eax                  # 4-byte Reload
	adcl	40(%esp), %eax                  # 4-byte Folded Reload
	movl	%eax, 92(%edi)
	addl	$92, %esp
	popl	%esi
	popl	%edi
	popl	%ebx
	popl	%ebp
	retl
.Lfunc_end72:
	.size	mcl_fpDbl_sub12L, .Lfunc_end72-mcl_fpDbl_sub12L
                                        # -- End function
	.globl	mulPv512x32                     # -- Begin function mulPv512x32
	.p2align	4, 0x90
	.type	mulPv512x32,@function
mulPv512x32:                            # @mulPv512x32
# %bb.0:
	pushl	%ebp
	pushl	%ebx
	pushl	%edi
	pushl	%esi
	subl	$108, %esp
	movl	136(%esp), %ebp
	movl	132(%esp), %ecx
	movl	%ebp, %eax
	mull	60(%ecx)
	movl	%edx, 104(%esp)                 # 4-byte Spill
	movl	%eax, 100(%esp)                 # 4-byte Spill
	movl	%ebp, %eax
	mull	56(%ecx)
	movl	%edx, 96(%esp)                  # 4-byte Spill
	movl	%eax, 92(%esp)                  # 4-byte Spill
	movl	%ebp, %eax
	mull	52(%ecx)
	movl	%edx, 88(%esp)                  # 4-byte Spill
	movl	%eax, 84(%esp)                  # 4-byte Spill
	movl	%ebp, %eax
	mull	48(%ecx)
	movl	%edx, 80(%esp)                  # 4-byte Spill
	movl	%eax, 76(%esp)                  # 4-byte Spill
	movl	%ebp, %eax
	mull	44(%ecx)
	movl	%edx, 72(%esp)                  # 4-byte Spill
	movl	%eax, 68(%esp)                  # 4-byte Spill
	movl	%ebp, %eax
	mull	40(%ecx)
	movl	%edx, 64(%esp)                  # 4-byte Spill
	movl	%eax, 60(%esp)                  # 4-byte Spill
	movl	%ebp, %eax
	mull	36(%ecx)
	movl	%edx, 56(%esp)                  # 4-byte Spill
	movl	%eax, 52(%esp)                  # 4-byte Spill
	movl	%ebp, %eax
	mull	32(%ecx)
	movl	%edx, 48(%esp)                  # 4-byte Spill
	movl	%eax, 44(%esp)                  # 4-byte Spill
	movl	%ebp, %eax
	mull	28(%ecx)
	movl	%edx, 40(%esp)                  # 4-byte Spill
	movl	%eax, 36(%esp)                  # 4-byte Spill
	movl	%ebp, %eax
	mull	24(%ecx)
	movl	%edx, 32(%esp)                  # 4-byte Spill
	movl	%eax, 28(%esp)                  # 4-byte Spill
	movl	%ebp, %eax
	mull	20(%ecx)
	movl	%edx, 24(%esp)                  # 4-byte Spill
	movl	%eax, 20(%esp)                  # 4-byte Spill
	movl	%ebp, %eax
	mull	16(%ecx)
	movl	%edx, 16(%esp)                  # 4-byte Spill
	movl	%eax, 12(%esp)                  # 4-byte Spill
	movl	%ebp, %eax
	mull	12(%ecx)
	movl	%edx, %ebx
	movl	%eax, 8(%esp)                   # 4-byte Spill
	movl	%ebp, %eax
	mull	8(%ecx)
	movl	%edx, %esi
	movl	%eax, 4(%esp)                   # 4-byte Spill
	movl	%ebp, %eax
	mull	4(%ecx)
	movl	%edx, %edi
	movl	%eax, (%esp)                    # 4-byte Spill
	movl	%ebp, %eax
	mull	(%ecx)
	movl	%eax, %ebp
	movl	128(%esp), %eax
	movl	%ebp, (%eax)
	addl	(%esp), %edx                    # 4-byte Folded Reload
	movl	%edx, 4(%eax)
	adcl	4(%esp), %edi                   # 4-byte Folded Reload
	movl	%edi, 8(%eax)
	adcl	8(%esp), %esi                   # 4-byte Folded Reload
	movl	%esi, 12(%eax)
	adcl	12(%esp), %ebx                  # 4-byte Folded Reload
	movl	%ebx, 16(%eax)
	movl	16(%esp), %ecx                  # 4-byte Reload
	adcl	20(%esp), %ecx                  # 4-byte Folded Reload
	movl	%ecx, 20(%eax)
	movl	24(%esp), %ecx                  # 4-byte Reload
	adcl	28(%esp), %ecx                  # 4-byte Folded Reload
	movl	%ecx, 24(%eax)
	movl	32(%esp), %ecx                  # 4-byte Reload
	adcl	36(%esp), %ecx                  # 4-byte Folded Reload
	movl	%ecx, 28(%eax)
	movl	40(%esp), %ecx                  # 4-byte Reload
	adcl	44(%esp), %ecx                  # 4-byte Folded Reload
	movl	%ecx, 32(%eax)
	movl	48(%esp), %ecx                  # 4-byte Reload
	adcl	52(%esp), %ecx                  # 4-byte Folded Reload
	movl	%ecx, 36(%eax)
	movl	56(%esp), %ecx                  # 4-byte Reload
	adcl	60(%esp), %ecx                  # 4-byte Folded Reload
	movl	%ecx, 40(%eax)
	movl	64(%esp), %ecx                  # 4-byte Reload
	adcl	68(%esp), %ecx                  # 4-byte Folded Reload
	movl	%ecx, 44(%eax)
	movl	72(%esp), %ecx                  # 4-byte Reload
	adcl	76(%esp), %ecx                  # 4-byte Folded Reload
	movl	%ecx, 48(%eax)
	movl	80(%esp), %ecx                  # 4-byte Reload
	adcl	84(%esp), %ecx                  # 4-byte Folded Reload
	movl	%ecx, 52(%eax)
	movl	88(%esp), %ecx                  # 4-byte Reload
	adcl	92(%esp), %ecx                  # 4-byte Folded Reload
	movl	%ecx, 56(%eax)
	movl	96(%esp), %ecx                  # 4-byte Reload
	adcl	100(%esp), %ecx                 # 4-byte Folded Reload
	movl	%ecx, 60(%eax)
	movl	104(%esp), %ecx                 # 4-byte Reload
	adcl	$0, %ecx
	movl	%ecx, 64(%eax)
	addl	$108, %esp
	popl	%esi
	popl	%edi
	popl	%ebx
	popl	%ebp
	retl	$4
.Lfunc_end73:
	.size	mulPv512x32, .Lfunc_end73-mulPv512x32
                                        # -- End function
	.globl	mcl_fp_mulUnitPre16L            # -- Begin function mcl_fp_mulUnitPre16L
	.p2align	4, 0x90
	.type	mcl_fp_mulUnitPre16L,@function
mcl_fp_mulUnitPre16L:                   # @mcl_fp_mulUnitPre16L
# %bb.0:
	pushl	%ebp
	pushl	%ebx
	pushl	%edi
	pushl	%esi
	subl	$124, %esp
	calll	.L74$pb
.L74$pb:
	popl	%ebx
.Ltmp16:
	addl	$_GLOBAL_OFFSET_TABLE_+(.Ltmp16-.L74$pb), %ebx
	subl	$4, %esp
	movl	156(%esp), %eax
	movl	152(%esp), %ecx
	leal	60(%esp), %edx
	pushl	%eax
	pushl	%ecx
	pushl	%edx
	calll	mulPv512x32@PLT
	addl	$12, %esp
	movl	56(%esp), %eax
	movl	%eax, 52(%esp)                  # 4-byte Spill
	movl	60(%esp), %eax
	movl	%eax, 48(%esp)                  # 4-byte Spill
	movl	64(%esp), %eax
	movl	%eax, 44(%esp)                  # 4-byte Spill
	movl	68(%esp), %eax
	movl	%eax, 40(%esp)                  # 4-byte Spill
	movl	72(%esp), %eax
	movl	%eax, 36(%esp)                  # 4-byte Spill
	movl	76(%esp), %eax
	movl	%eax, 32(%esp)                  # 4-byte Spill
	movl	80(%esp), %eax
	movl	%eax, 28(%esp)                  # 4-byte Spill
	movl	84(%esp), %eax
	movl	%eax, 24(%esp)                  # 4-byte Spill
	movl	88(%esp), %eax
	movl	%eax, 20(%esp)                  # 4-byte Spill
	movl	92(%esp), %eax
	movl	%eax, 16(%esp)                  # 4-byte Spill
	movl	96(%esp), %eax
	movl	%eax, 12(%esp)                  # 4-byte Spill
	movl	100(%esp), %ebp
	movl	104(%esp), %ebx
	movl	108(%esp), %edi
	movl	112(%esp), %esi
	movl	116(%esp), %edx
	movl	120(%esp), %ecx
	movl	144(%esp), %eax
	movl	%ecx, 64(%eax)
	movl	%edx, 60(%eax)
	movl	%esi, 56(%eax)
	movl	%edi, 52(%eax)
	movl	%ebx, 48(%eax)
	movl	%ebp, 44(%eax)
	movl	12(%esp), %ecx                  # 4-byte Reload
	movl	%ecx, 40(%eax)
	movl	16(%esp), %ecx                  # 4-byte Reload
	movl	%ecx, 36(%eax)
	movl	20(%esp), %ecx                  # 4-byte Reload
	movl	%ecx, 32(%eax)
	movl	24(%esp), %ecx                  # 4-byte Reload
	movl	%ecx, 28(%eax)
	movl	28(%esp), %ecx                  # 4-byte Reload
	movl	%ecx, 24(%eax)
	movl	32(%esp), %ecx                  # 4-byte Reload
	movl	%ecx, 20(%eax)
	movl	36(%esp), %ecx                  # 4-byte Reload
	movl	%ecx, 16(%eax)
	movl	40(%esp), %ecx                  # 4-byte Reload
	movl	%ecx, 12(%eax)
	movl	44(%esp), %ecx                  # 4-byte Reload
	movl	%ecx, 8(%eax)
	movl	48(%esp), %ecx                  # 4-byte Reload
	movl	%ecx, 4(%eax)
	movl	52(%esp), %ecx                  # 4-byte Reload
	movl	%ecx, (%eax)
	addl	$124, %esp
	popl	%esi
	popl	%edi
	popl	%ebx
	popl	%ebp
	retl
.Lfunc_end74:
	.size	mcl_fp_mulUnitPre16L, .Lfunc_end74-mcl_fp_mulUnitPre16L
                                        # -- End function
	.globl	mcl_fpDbl_mulPre16L             # -- Begin function mcl_fpDbl_mulPre16L
	.p2align	4, 0x90
	.type	mcl_fpDbl_mulPre16L,@function
mcl_fpDbl_mulPre16L:                    # @mcl_fpDbl_mulPre16L
# %bb.0:
	pushl	%ebp
	pushl	%ebx
	pushl	%edi
	pushl	%esi
	subl	$300, %esp                      # imm = 0x12C
	calll	.L75$pb
.L75$pb:
	popl	%esi
.Ltmp17:
	addl	$_GLOBAL_OFFSET_TABLE_+(.Ltmp17-.L75$pb), %esi
	subl	$4, %esp
	movl	332(%esp), %ebp
	movl	328(%esp), %edi
	movl	%esi, %ebx
	movl	%esi, 88(%esp)                  # 4-byte Spill
	pushl	%ebp
	pushl	%edi
	pushl	332(%esp)
	calll	mcl_fpDbl_mulPre8L@PLT
	addl	$12, %esp
	leal	32(%ebp), %eax
	leal	32(%edi), %ecx
	movl	324(%esp), %edx
	addl	$64, %edx
	pushl	%eax
	pushl	%ecx
	pushl	%edx
	calll	mcl_fpDbl_mulPre8L@PLT
	addl	$16, %esp
	movl	48(%edi), %eax
	movl	44(%edi), %ecx
	movl	40(%edi), %edx
	movl	32(%edi), %esi
	movl	36(%edi), %ebx
	addl	(%edi), %esi
	movl	%esi, 16(%esp)                  # 4-byte Spill
	adcl	4(%edi), %ebx
	movl	%ebx, 24(%esp)                  # 4-byte Spill
	adcl	8(%edi), %edx
	movl	%edx, 36(%esp)                  # 4-byte Spill
	adcl	12(%edi), %ecx
	movl	%ecx, 20(%esp)                  # 4-byte Spill
	adcl	16(%edi), %eax
	movl	%eax, 40(%esp)                  # 4-byte Spill
	movl	52(%edi), %eax
	adcl	20(%edi), %eax
	movl	%eax, %edx
	movl	%eax, 32(%esp)                  # 4-byte Spill
	movl	56(%edi), %eax
	adcl	24(%edi), %eax
	movl	%eax, 44(%esp)                  # 4-byte Spill
	movl	60(%edi), %eax
	adcl	28(%edi), %eax
	movl	%eax, %ecx
	movl	%eax, 60(%esp)                  # 4-byte Spill
	setb	80(%esp)                        # 1-byte Folded Spill
	movl	32(%ebp), %eax
	addl	(%ebp), %eax
	movl	%eax, 48(%esp)                  # 4-byte Spill
	movl	36(%ebp), %eax
	adcl	4(%ebp), %eax
	movl	%eax, 52(%esp)                  # 4-byte Spill
	movl	40(%ebp), %eax
	adcl	8(%ebp), %eax
	movl	%eax, 56(%esp)                  # 4-byte Spill
	movl	44(%ebp), %eax
	adcl	12(%ebp), %eax
	movl	%eax, 28(%esp)                  # 4-byte Spill
	movl	48(%ebp), %esi
	adcl	16(%ebp), %esi
	movl	%esi, 64(%esp)                  # 4-byte Spill
	movl	52(%ebp), %eax
	adcl	20(%ebp), %eax
	movl	%eax, 12(%esp)                  # 4-byte Spill
	movl	56(%ebp), %eax
	adcl	24(%ebp), %eax
	movl	%eax, %edi
	movl	%eax, 68(%esp)                  # 4-byte Spill
	movl	60(%ebp), %ebx
	adcl	28(%ebp), %ebx
	movl	%ebx, 76(%esp)                  # 4-byte Spill
	movl	%ecx, 232(%esp)
	movl	44(%esp), %eax                  # 4-byte Reload
	movl	%eax, 228(%esp)
	movl	%edx, 224(%esp)
	movl	40(%esp), %eax                  # 4-byte Reload
	movl	%eax, 220(%esp)
	movl	20(%esp), %ecx                  # 4-byte Reload
	movl	%ecx, 216(%esp)
	movl	36(%esp), %ecx                  # 4-byte Reload
	movl	%ecx, 212(%esp)
	movl	24(%esp), %ecx                  # 4-byte Reload
	movl	%ecx, 208(%esp)
	movl	16(%esp), %ecx                  # 4-byte Reload
	movl	%ecx, 204(%esp)
	movl	%ebx, 200(%esp)
	movl	%edi, 196(%esp)
	movl	12(%esp), %ebp                  # 4-byte Reload
	movl	%ebp, 192(%esp)
	movl	%esi, 188(%esp)
	movl	28(%esp), %eax                  # 4-byte Reload
	movl	%eax, 184(%esp)
	movl	56(%esp), %edx                  # 4-byte Reload
	movl	%edx, 180(%esp)
	movl	52(%esp), %ebx                  # 4-byte Reload
	movl	%ebx, 176(%esp)
	movl	48(%esp), %ecx                  # 4-byte Reload
	movl	%ecx, 172(%esp)
	setb	72(%esp)                        # 1-byte Folded Spill
	subl	$4, %esp
	movzbl	84(%esp), %esi                  # 1-byte Folded Reload
	movl	%esi, %ecx
	negl	%ecx
	movl	%esi, %edi
	shll	$31, %edi
	shrdl	$31, %ecx, %edi
	andl	52(%esp), %edi                  # 4-byte Folded Reload
	andl	%ecx, 72(%esp)                  # 4-byte Folded Spill
	andl	%ecx, %ebp
	movl	%ebp, 16(%esp)                  # 4-byte Spill
	andl	%ecx, 68(%esp)                  # 4-byte Folded Spill
	andl	%ecx, %eax
	movl	%eax, 32(%esp)                  # 4-byte Spill
	andl	%ecx, %edx
	movl	%edx, 60(%esp)                  # 4-byte Spill
	andl	%ecx, %ebx
	movl	%ebx, 56(%esp)                  # 4-byte Spill
	andl	80(%esp), %ecx                  # 4-byte Folded Reload
	movzbl	76(%esp), %ebp                  # 1-byte Folded Reload
	andl	%ebp, %esi
	movl	%esi, 52(%esp)                  # 4-byte Spill
	movl	%ebp, %edx
	negl	%edx
	andl	%edx, 64(%esp)                  # 4-byte Folded Spill
	andl	%edx, 48(%esp)                  # 4-byte Folded Spill
	movl	36(%esp), %ebx                  # 4-byte Reload
	andl	%edx, %ebx
	andl	%edx, 44(%esp)                  # 4-byte Folded Spill
	movl	24(%esp), %esi                  # 4-byte Reload
	andl	%edx, %esi
	andl	%edx, 40(%esp)                  # 4-byte Folded Spill
	movl	28(%esp), %eax                  # 4-byte Reload
	andl	%edx, %eax
	shll	$31, %ebp
	shrdl	$31, %edx, %ebp
	andl	20(%esp), %ebp                  # 4-byte Folded Reload
	addl	%edi, %ebp
	movl	%ebp, 20(%esp)                  # 4-byte Spill
	adcl	56(%esp), %eax                  # 4-byte Folded Reload
	movl	%eax, 28(%esp)                  # 4-byte Spill
	movl	40(%esp), %ebp                  # 4-byte Reload
	adcl	60(%esp), %ebp                  # 4-byte Folded Reload
	adcl	32(%esp), %esi                  # 4-byte Folded Reload
	movl	%esi, 24(%esp)                  # 4-byte Spill
	movl	44(%esp), %esi                  # 4-byte Reload
	adcl	68(%esp), %esi                  # 4-byte Folded Reload
	adcl	16(%esp), %ebx                  # 4-byte Folded Reload
	movl	%ebx, 36(%esp)                  # 4-byte Spill
	movl	48(%esp), %edi                  # 4-byte Reload
	adcl	72(%esp), %edi                  # 4-byte Folded Reload
	leal	176(%esp), %edx
	adcl	%ecx, 64(%esp)                  # 4-byte Folded Spill
	setb	%al
	movzbl	%al, %eax
	movl	%eax, 16(%esp)                  # 4-byte Spill
	movl	88(%esp), %ebx                  # 4-byte Reload
	pushl	%edx
	leal	212(%esp), %eax
	pushl	%eax
	leal	248(%esp), %eax
	pushl	%eax
	calll	mcl_fpDbl_mulPre8L@PLT
	addl	$16, %esp
	movl	16(%esp), %eax                  # 4-byte Reload
	addl	268(%esp), %eax
	movl	%eax, 16(%esp)                  # 4-byte Spill
	movl	24(%esp), %eax                  # 4-byte Reload
	adcl	272(%esp), %eax
	movl	%eax, 24(%esp)                  # 4-byte Spill
	adcl	276(%esp), %ebp
	movl	%ebp, 36(%esp)                  # 4-byte Spill
	movl	20(%esp), %eax                  # 4-byte Reload
	adcl	280(%esp), %eax
	movl	%eax, 20(%esp)                  # 4-byte Spill
	adcl	284(%esp), %esi
	movl	%esi, 40(%esp)                  # 4-byte Spill
	movl	32(%esp), %eax                  # 4-byte Reload
	adcl	288(%esp), %eax
	adcl	292(%esp), %edi
	movl	%edi, 44(%esp)                  # 4-byte Spill
	movl	60(%esp), %ebp                  # 4-byte Reload
	adcl	296(%esp), %ebp
	movl	48(%esp), %ecx                  # 4-byte Reload
	adcl	%ecx, 12(%esp)                  # 4-byte Folded Spill
	movl	236(%esp), %ecx
	movl	320(%esp), %esi
	subl	(%esi), %ecx
	movl	%ecx, 56(%esp)                  # 4-byte Spill
	movl	240(%esp), %ebx
	sbbl	4(%esi), %ebx
	movl	244(%esp), %ecx
	sbbl	8(%esi), %ecx
	movl	248(%esp), %edx
	sbbl	12(%esi), %edx
	movl	%edx, 32(%esp)                  # 4-byte Spill
	movl	252(%esp), %edx
	sbbl	16(%esi), %edx
	movl	%edx, 28(%esp)                  # 4-byte Spill
	movl	256(%esp), %edx
	sbbl	20(%esi), %edx
	movl	%edx, 68(%esp)                  # 4-byte Spill
	movl	260(%esp), %edx
	sbbl	24(%esi), %edx
	movl	%edx, 64(%esp)                  # 4-byte Spill
	movl	264(%esp), %edx
	sbbl	28(%esi), %edx
	movl	%edx, 52(%esp)                  # 4-byte Spill
	movl	32(%esi), %edx
	movl	%edx, 48(%esp)                  # 4-byte Spill
	sbbl	%edx, 16(%esp)                  # 4-byte Folded Spill
	movl	36(%esi), %edx
	movl	%edx, 80(%esp)                  # 4-byte Spill
	sbbl	%edx, 24(%esp)                  # 4-byte Folded Spill
	movl	40(%esi), %edx
	movl	%edx, 76(%esp)                  # 4-byte Spill
	sbbl	%edx, 36(%esp)                  # 4-byte Folded Spill
	movl	44(%esi), %edx
	movl	%edx, 136(%esp)                 # 4-byte Spill
	sbbl	%edx, 20(%esp)                  # 4-byte Folded Spill
	movl	48(%esi), %edx
	movl	%edx, 104(%esp)                 # 4-byte Spill
	sbbl	%edx, 40(%esp)                  # 4-byte Folded Spill
	movl	52(%esi), %edx
	movl	%edx, 100(%esp)                 # 4-byte Spill
	sbbl	%edx, %eax
	movl	%eax, %edi
	movl	56(%esi), %eax
	movl	%eax, 96(%esp)                  # 4-byte Spill
	sbbl	%eax, 44(%esp)                  # 4-byte Folded Spill
	movl	60(%esi), %eax
	movl	%eax, 84(%esp)                  # 4-byte Spill
	sbbl	%eax, %ebp
	movl	%ebp, 60(%esp)                  # 4-byte Spill
	movl	12(%esp), %eax                  # 4-byte Reload
	sbbl	$0, %eax
	movl	64(%esi), %ebp
	movl	%ebp, 132(%esp)                 # 4-byte Spill
	subl	%ebp, 56(%esp)                  # 4-byte Folded Spill
	movl	68(%esi), %ebp
	movl	%ebp, 144(%esp)                 # 4-byte Spill
	sbbl	%ebp, %ebx
	movl	%ebx, 92(%esp)                  # 4-byte Spill
	movl	72(%esi), %ebx
	movl	%ebx, 140(%esp)                 # 4-byte Spill
	sbbl	%ebx, %ecx
	movl	%ecx, 88(%esp)                  # 4-byte Spill
	movl	76(%esi), %ecx
	movl	%ecx, 128(%esp)                 # 4-byte Spill
	movl	32(%esp), %edx                  # 4-byte Reload
	sbbl	%ecx, %edx
	movl	80(%esi), %ebx
	movl	%ebx, 124(%esp)                 # 4-byte Spill
	movl	28(%esp), %ecx                  # 4-byte Reload
	sbbl	%ebx, %ecx
	movl	84(%esi), %ebx
	movl	%ebx, 120(%esp)                 # 4-byte Spill
	sbbl	%ebx, 68(%esp)                  # 4-byte Folded Spill
	movl	88(%esi), %ebx
	movl	%ebx, 116(%esp)                 # 4-byte Spill
	sbbl	%ebx, 64(%esp)                  # 4-byte Folded Spill
	movl	92(%esi), %ebx
	movl	%ebx, 112(%esp)                 # 4-byte Spill
	sbbl	%ebx, 52(%esp)                  # 4-byte Folded Spill
	movl	96(%esi), %ebx
	movl	%ebx, 108(%esp)                 # 4-byte Spill
	sbbl	%ebx, 16(%esp)                  # 4-byte Folded Spill
	movl	100(%esi), %ebx
	movl	%ebx, 164(%esp)                 # 4-byte Spill
	sbbl	%ebx, 24(%esp)                  # 4-byte Folded Spill
	movl	104(%esi), %ebx
	movl	%ebx, 160(%esp)                 # 4-byte Spill
	sbbl	%ebx, 36(%esp)                  # 4-byte Folded Spill
	movl	108(%esi), %ebx
	movl	%ebx, 156(%esp)                 # 4-byte Spill
	sbbl	%ebx, 20(%esp)                  # 4-byte Folded Spill
	movl	112(%esi), %ebx
	movl	%ebx, 152(%esp)                 # 4-byte Spill
	sbbl	%ebx, 40(%esp)                  # 4-byte Folded Spill
	movl	116(%esi), %ebx
	movl	%ebx, 72(%esp)                  # 4-byte Spill
	sbbl	%ebx, %edi
	movl	%edi, 32(%esp)                  # 4-byte Spill
	movl	120(%esi), %edi
	movl	%edi, 148(%esp)                 # 4-byte Spill
	sbbl	%edi, 44(%esp)                  # 4-byte Folded Spill
	movl	124(%esi), %edi
	movl	%edi, 168(%esp)                 # 4-byte Spill
	sbbl	%edi, 60(%esp)                  # 4-byte Folded Spill
	sbbl	$0, %eax
	movl	%eax, 12(%esp)                  # 4-byte Spill
	movl	56(%esp), %ebp                  # 4-byte Reload
	addl	48(%esp), %ebp                  # 4-byte Folded Reload
	movl	92(%esp), %ebx                  # 4-byte Reload
	adcl	80(%esp), %ebx                  # 4-byte Folded Reload
	movl	88(%esp), %edi                  # 4-byte Reload
	adcl	76(%esp), %edi                  # 4-byte Folded Reload
	adcl	136(%esp), %edx                 # 4-byte Folded Reload
	adcl	104(%esp), %ecx                 # 4-byte Folded Reload
	movl	%ecx, 28(%esp)                  # 4-byte Spill
	movl	68(%esp), %eax                  # 4-byte Reload
	adcl	100(%esp), %eax                 # 4-byte Folded Reload
	movl	64(%esp), %ecx                  # 4-byte Reload
	adcl	96(%esp), %ecx                  # 4-byte Folded Reload
	movl	%eax, 52(%esi)
	movl	28(%esp), %eax                  # 4-byte Reload
	movl	%eax, 48(%esi)
	movl	%edx, 44(%esi)
	movl	%edi, 40(%esi)
	movl	%ebx, 36(%esi)
	movl	%ebp, 32(%esi)
	movl	52(%esp), %edx                  # 4-byte Reload
	adcl	84(%esp), %edx                  # 4-byte Folded Reload
	movl	%ecx, 56(%esi)
	movl	16(%esp), %eax                  # 4-byte Reload
	adcl	132(%esp), %eax                 # 4-byte Folded Reload
	movl	%edx, 60(%esi)
	movl	24(%esp), %ecx                  # 4-byte Reload
	adcl	144(%esp), %ecx                 # 4-byte Folded Reload
	movl	%eax, 64(%esi)
	movl	36(%esp), %eax                  # 4-byte Reload
	adcl	140(%esp), %eax                 # 4-byte Folded Reload
	movl	%ecx, 68(%esi)
	movl	20(%esp), %ecx                  # 4-byte Reload
	adcl	128(%esp), %ecx                 # 4-byte Folded Reload
	movl	%eax, 72(%esi)
	movl	40(%esp), %eax                  # 4-byte Reload
	adcl	124(%esp), %eax                 # 4-byte Folded Reload
	movl	%ecx, 76(%esi)
	movl	32(%esp), %edx                  # 4-byte Reload
	adcl	120(%esp), %edx                 # 4-byte Folded Reload
	movl	%eax, 80(%esi)
	movl	44(%esp), %ecx                  # 4-byte Reload
	adcl	116(%esp), %ecx                 # 4-byte Folded Reload
	movl	%edx, 84(%esi)
	movl	60(%esp), %eax                  # 4-byte Reload
	adcl	112(%esp), %eax                 # 4-byte Folded Reload
	movl	%ecx, 88(%esi)
	movl	12(%esp), %ecx                  # 4-byte Reload
	adcl	108(%esp), %ecx                 # 4-byte Folded Reload
	movl	%eax, 92(%esi)
	movl	%ecx, 96(%esi)
	movl	164(%esp), %eax                 # 4-byte Reload
	adcl	$0, %eax
	movl	%eax, 100(%esi)
	movl	160(%esp), %eax                 # 4-byte Reload
	adcl	$0, %eax
	movl	%eax, 104(%esi)
	movl	156(%esp), %eax                 # 4-byte Reload
	adcl	$0, %eax
	movl	%eax, 108(%esi)
	movl	152(%esp), %eax                 # 4-byte Reload
	adcl	$0, %eax
	movl	%eax, 112(%esi)
	movl	72(%esp), %eax                  # 4-byte Reload
	adcl	$0, %eax
	movl	%eax, 116(%esi)
	movl	148(%esp), %eax                 # 4-byte Reload
	adcl	$0, %eax
	movl	%eax, 120(%esi)
	movl	168(%esp), %eax                 # 4-byte Reload
	adcl	$0, %eax
	movl	%eax, 124(%esi)
	addl	$300, %esp                      # imm = 0x12C
	popl	%esi
	popl	%edi
	popl	%ebx
	popl	%ebp
	retl
.Lfunc_end75:
	.size	mcl_fpDbl_mulPre16L, .Lfunc_end75-mcl_fpDbl_mulPre16L
                                        # -- End function
	.globl	mcl_fpDbl_sqrPre16L             # -- Begin function mcl_fpDbl_sqrPre16L
	.p2align	4, 0x90
	.type	mcl_fpDbl_sqrPre16L,@function
mcl_fpDbl_sqrPre16L:                    # @mcl_fpDbl_sqrPre16L
# %bb.0:
	pushl	%ebp
	pushl	%ebx
	pushl	%edi
	pushl	%esi
	subl	$284, %esp                      # imm = 0x11C
	calll	.L76$pb
.L76$pb:
	popl	%esi
.Ltmp18:
	addl	$_GLOBAL_OFFSET_TABLE_+(.Ltmp18-.L76$pb), %esi
	subl	$4, %esp
	movl	312(%esp), %edi
	movl	308(%esp), %ebp
	movl	%esi, %ebx
	movl	%esi, 20(%esp)                  # 4-byte Spill
	pushl	%edi
	pushl	%edi
	pushl	%ebp
	calll	mcl_fpDbl_mulPre8L@PLT
	addl	$12, %esp
	leal	32(%edi), %eax
	leal	64(%ebp), %ecx
	pushl	%eax
	pushl	%eax
	pushl	%ecx
	calll	mcl_fpDbl_mulPre8L@PLT
	addl	$16, %esp
	movl	52(%edi), %ecx
	movl	48(%edi), %ebx
	movl	44(%edi), %ebp
	movl	40(%edi), %esi
	movl	32(%edi), %eax
	movl	36(%edi), %edx
	addl	(%edi), %eax
	movl	%eax, 4(%esp)                   # 4-byte Spill
	adcl	4(%edi), %edx
	movl	%edx, 8(%esp)                   # 4-byte Spill
	adcl	8(%edi), %esi
	adcl	12(%edi), %ebp
	adcl	16(%edi), %ebx
	adcl	20(%edi), %ecx
	movl	%ecx, %edx
	movl	%ecx, 24(%esp)                  # 4-byte Spill
	movl	56(%edi), %ecx
	adcl	24(%edi), %ecx
	movl	60(%edi), %eax
	adcl	28(%edi), %eax
	movl	%eax, 12(%esp)                  # 4-byte Spill
	movl	%eax, 216(%esp)
	movl	%ecx, 212(%esp)
	movl	%ecx, 28(%esp)                  # 4-byte Spill
	movl	%edx, 208(%esp)
	movl	%ebx, 20(%esp)                  # 4-byte Spill
	movl	%ebx, 204(%esp)
	movl	%ebp, %edi
	movl	%ebp, 200(%esp)
	movl	%esi, 196(%esp)
	movl	8(%esp), %eax                   # 4-byte Reload
	movl	%eax, 192(%esp)
	movl	4(%esp), %ebp                   # 4-byte Reload
	movl	%ebp, 188(%esp)
	movl	12(%esp), %ebp                  # 4-byte Reload
	movl	%ebp, 184(%esp)
	movl	%ecx, 180(%esp)
	movl	%edx, 176(%esp)
	movl	%ebx, 172(%esp)
	movl	%edi, 168(%esp)
	movl	%edi, %ebp
	movl	%esi, 164(%esp)
	movl	%esi, %ecx
	movl	%eax, 160(%esp)
	movl	%eax, %edx
	movl	4(%esp), %eax                   # 4-byte Reload
	movl	%eax, 156(%esp)
	setb	%bl
	subl	$4, %esp
	movzbl	%bl, %edi
	movl	%edi, 36(%esp)                  # 4-byte Spill
	movl	%edi, %esi
	shll	$31, %esi
	negl	%edi
	shrdl	$31, %edi, %esi
	andl	%eax, %esi
	movl	%esi, 44(%esp)                  # 4-byte Spill
	andl	%edi, %edx
	movl	%edx, 12(%esp)                  # 4-byte Spill
	andl	%edi, %ecx
	movl	%ecx, 8(%esp)                   # 4-byte Spill
	andl	%edi, %ebp
	movl	24(%esp), %ebx                  # 4-byte Reload
	andl	%edi, %ebx
	movl	28(%esp), %eax                  # 4-byte Reload
	andl	%edi, %eax
	movl	32(%esp), %ecx                  # 4-byte Reload
	andl	%edi, %ecx
	andl	16(%esp), %edi                  # 4-byte Folded Reload
	movl	%edi, %edx
	shldl	$1, %ecx, %edx
	movl	%edx, 16(%esp)                  # 4-byte Spill
	shldl	$1, %eax, %ecx
	movl	%ecx, 32(%esp)                  # 4-byte Spill
	shldl	$1, %ebx, %eax
	movl	%eax, 28(%esp)                  # 4-byte Spill
	shldl	$1, %ebp, %ebx
	movl	%ebx, 24(%esp)                  # 4-byte Spill
	movl	8(%esp), %eax                   # 4-byte Reload
	shldl	$1, %eax, %ebp
	movl	%ebp, 40(%esp)                  # 4-byte Spill
	movl	12(%esp), %ecx                  # 4-byte Reload
	shldl	$1, %ecx, %eax
	movl	%eax, 8(%esp)                   # 4-byte Spill
	movl	44(%esp), %esi                  # 4-byte Reload
	shldl	$1, %esi, %ecx
	movl	%ecx, 12(%esp)                  # 4-byte Spill
	shrl	$31, %edi
	addl	%esi, %esi
	leal	160(%esp), %eax
	movl	20(%esp), %ebx                  # 4-byte Reload
	pushl	%eax
	leal	196(%esp), %eax
	pushl	%eax
	leal	232(%esp), %eax
	pushl	%eax
	calll	mcl_fpDbl_mulPre8L@PLT
	addl	$16, %esp
	addl	252(%esp), %esi
	movl	8(%esp), %eax                   # 4-byte Reload
	adcl	256(%esp), %eax
	movl	%eax, 8(%esp)                   # 4-byte Spill
	movl	4(%esp), %eax                   # 4-byte Reload
	adcl	260(%esp), %eax
	movl	%eax, 4(%esp)                   # 4-byte Spill
	movl	36(%esp), %eax                  # 4-byte Reload
	adcl	264(%esp), %eax
	movl	%eax, 36(%esp)                  # 4-byte Spill
	movl	20(%esp), %eax                  # 4-byte Reload
	adcl	268(%esp), %eax
	movl	%eax, 20(%esp)                  # 4-byte Spill
	movl	24(%esp), %eax                  # 4-byte Reload
	adcl	272(%esp), %eax
	movl	%eax, 24(%esp)                  # 4-byte Spill
	movl	28(%esp), %eax                  # 4-byte Reload
	adcl	276(%esp), %eax
	movl	%eax, 28(%esp)                  # 4-byte Spill
	movl	12(%esp), %eax                  # 4-byte Reload
	adcl	280(%esp), %eax
	movl	%eax, 12(%esp)                  # 4-byte Spill
	adcl	32(%esp), %edi                  # 4-byte Folded Reload
	movl	220(%esp), %ebx
	movl	304(%esp), %ebp
	subl	(%ebp), %ebx
	movl	224(%esp), %eax
	sbbl	4(%ebp), %eax
	movl	%eax, 16(%esp)                  # 4-byte Spill
	movl	228(%esp), %ecx
	sbbl	8(%ebp), %ecx
	movl	%ecx, 56(%esp)                  # 4-byte Spill
	movl	232(%esp), %ecx
	sbbl	12(%ebp), %ecx
	movl	236(%esp), %eax
	sbbl	16(%ebp), %eax
	movl	%eax, 32(%esp)                  # 4-byte Spill
	movl	240(%esp), %edx
	sbbl	20(%ebp), %edx
	movl	%edx, 52(%esp)                  # 4-byte Spill
	movl	244(%esp), %edx
	sbbl	24(%ebp), %edx
	movl	%edx, 48(%esp)                  # 4-byte Spill
	movl	248(%esp), %eax
	sbbl	28(%ebp), %eax
	movl	%eax, 44(%esp)                  # 4-byte Spill
	movl	32(%ebp), %eax
	movl	%eax, 152(%esp)                 # 4-byte Spill
	sbbl	%eax, %esi
	movl	36(%ebp), %eax
	movl	%eax, 144(%esp)                 # 4-byte Spill
	sbbl	%eax, 8(%esp)                   # 4-byte Folded Spill
	movl	40(%ebp), %eax
	movl	%eax, 140(%esp)                 # 4-byte Spill
	sbbl	%eax, 4(%esp)                   # 4-byte Folded Spill
	movl	44(%ebp), %eax
	movl	%eax, 92(%esp)                  # 4-byte Spill
	sbbl	%eax, 36(%esp)                  # 4-byte Folded Spill
	movl	48(%ebp), %eax
	movl	%eax, 68(%esp)                  # 4-byte Spill
	sbbl	%eax, 20(%esp)                  # 4-byte Folded Spill
	movl	52(%ebp), %eax
	movl	%eax, 64(%esp)                  # 4-byte Spill
	sbbl	%eax, 24(%esp)                  # 4-byte Folded Spill
	movl	56(%ebp), %eax
	movl	%eax, 60(%esp)                  # 4-byte Spill
	sbbl	%eax, 28(%esp)                  # 4-byte Folded Spill
	movl	60(%ebp), %eax
	movl	%eax, 148(%esp)                 # 4-byte Spill
	sbbl	%eax, 12(%esp)                  # 4-byte Folded Spill
	sbbl	$0, %edi
	movl	64(%ebp), %eax
	movl	%eax, 88(%esp)                  # 4-byte Spill
	subl	%eax, %ebx
	movl	68(%ebp), %eax
	movl	%eax, 108(%esp)                 # 4-byte Spill
	movl	16(%esp), %edx                  # 4-byte Reload
	sbbl	%eax, %edx
	movl	72(%ebp), %eax
	movl	%eax, 84(%esp)                  # 4-byte Spill
	sbbl	%eax, 56(%esp)                  # 4-byte Folded Spill
	movl	76(%ebp), %eax
	movl	%eax, 104(%esp)                 # 4-byte Spill
	sbbl	%eax, %ecx
	movl	%ecx, 16(%esp)                  # 4-byte Spill
	movl	80(%ebp), %ecx
	movl	%ecx, 80(%esp)                  # 4-byte Spill
	movl	32(%esp), %eax                  # 4-byte Reload
	sbbl	%ecx, %eax
	movl	84(%ebp), %ecx
	movl	%ecx, 76(%esp)                  # 4-byte Spill
	sbbl	%ecx, 52(%esp)                  # 4-byte Folded Spill
	movl	88(%ebp), %ecx
	movl	%ecx, 72(%esp)                  # 4-byte Spill
	sbbl	%ecx, 48(%esp)                  # 4-byte Folded Spill
	movl	92(%ebp), %ecx
	movl	%ecx, 100(%esp)                 # 4-byte Spill
	sbbl	%ecx, 44(%esp)                  # 4-byte Folded Spill
	movl	96(%ebp), %ecx
	movl	%ecx, 96(%esp)                  # 4-byte Spill
	sbbl	%ecx, %esi
	movl	%esi, 40(%esp)                  # 4-byte Spill
	movl	100(%ebp), %ecx
	movl	%ecx, 136(%esp)                 # 4-byte Spill
	sbbl	%ecx, 8(%esp)                   # 4-byte Folded Spill
	movl	104(%ebp), %ecx
	movl	%ecx, 132(%esp)                 # 4-byte Spill
	sbbl	%ecx, 4(%esp)                   # 4-byte Folded Spill
	movl	108(%ebp), %ecx
	movl	%ecx, 128(%esp)                 # 4-byte Spill
	sbbl	%ecx, 36(%esp)                  # 4-byte Folded Spill
	movl	112(%ebp), %ecx
	movl	%ecx, 124(%esp)                 # 4-byte Spill
	sbbl	%ecx, 20(%esp)                  # 4-byte Folded Spill
	movl	116(%ebp), %ecx
	movl	%ecx, 120(%esp)                 # 4-byte Spill
	sbbl	%ecx, 24(%esp)                  # 4-byte Folded Spill
	movl	120(%ebp), %ecx
	movl	%ecx, 116(%esp)                 # 4-byte Spill
	sbbl	%ecx, 28(%esp)                  # 4-byte Folded Spill
	movl	124(%ebp), %ecx
	movl	%ecx, 112(%esp)                 # 4-byte Spill
	sbbl	%ecx, 12(%esp)                  # 4-byte Folded Spill
	sbbl	$0, %edi
	addl	152(%esp), %ebx                 # 4-byte Folded Reload
	movl	%edx, %esi
	adcl	144(%esp), %esi                 # 4-byte Folded Reload
	movl	56(%esp), %ecx                  # 4-byte Reload
	adcl	140(%esp), %ecx                 # 4-byte Folded Reload
	movl	16(%esp), %edx                  # 4-byte Reload
	adcl	92(%esp), %edx                  # 4-byte Folded Reload
	movl	%edx, 16(%esp)                  # 4-byte Spill
	adcl	68(%esp), %eax                  # 4-byte Folded Reload
	movl	%eax, 32(%esp)                  # 4-byte Spill
	movl	52(%esp), %eax                  # 4-byte Reload
	adcl	64(%esp), %eax                  # 4-byte Folded Reload
	movl	48(%esp), %edx                  # 4-byte Reload
	adcl	60(%esp), %edx                  # 4-byte Folded Reload
	movl	%eax, 52(%ebp)
	movl	32(%esp), %eax                  # 4-byte Reload
	movl	%eax, 48(%ebp)
	movl	16(%esp), %eax                  # 4-byte Reload
	movl	%eax, 44(%ebp)
	movl	%ecx, 40(%ebp)
	movl	%esi, 36(%ebp)
	movl	%ebx, 32(%ebp)
	movl	44(%esp), %ecx                  # 4-byte Reload
	adcl	148(%esp), %ecx                 # 4-byte Folded Reload
	movl	%edx, 56(%ebp)
	movl	40(%esp), %edx                  # 4-byte Reload
	adcl	88(%esp), %edx                  # 4-byte Folded Reload
	movl	%ecx, 60(%ebp)
	movl	8(%esp), %eax                   # 4-byte Reload
	adcl	108(%esp), %eax                 # 4-byte Folded Reload
	movl	%edx, 64(%ebp)
	movl	4(%esp), %ecx                   # 4-byte Reload
	adcl	84(%esp), %ecx                  # 4-byte Folded Reload
	movl	%eax, 68(%ebp)
	movl	36(%esp), %eax                  # 4-byte Reload
	adcl	104(%esp), %eax                 # 4-byte Folded Reload
	movl	%ecx, 72(%ebp)
	movl	20(%esp), %ecx                  # 4-byte Reload
	adcl	80(%esp), %ecx                  # 4-byte Folded Reload
	movl	%eax, 76(%ebp)
	movl	24(%esp), %eax                  # 4-byte Reload
	adcl	76(%esp), %eax                  # 4-byte Folded Reload
	movl	%ecx, 80(%ebp)
	movl	28(%esp), %ecx                  # 4-byte Reload
	adcl	72(%esp), %ecx                  # 4-byte Folded Reload
	movl	%eax, 84(%ebp)
	movl	12(%esp), %eax                  # 4-byte Reload
	adcl	100(%esp), %eax                 # 4-byte Folded Reload
	movl	%ecx, 88(%ebp)
	adcl	96(%esp), %edi                  # 4-byte Folded Reload
	movl	%eax, 92(%ebp)
	movl	%edi, 96(%ebp)
	movl	136(%esp), %eax                 # 4-byte Reload
	adcl	$0, %eax
	movl	%eax, 100(%ebp)
	movl	132(%esp), %eax                 # 4-byte Reload
	adcl	$0, %eax
	movl	%eax, 104(%ebp)
	movl	128(%esp), %eax                 # 4-byte Reload
	adcl	$0, %eax
	movl	%eax, 108(%ebp)
	movl	124(%esp), %eax                 # 4-byte Reload
	adcl	$0, %eax
	movl	%eax, 112(%ebp)
	movl	120(%esp), %eax                 # 4-byte Reload
	adcl	$0, %eax
	movl	%eax, 116(%ebp)
	movl	116(%esp), %eax                 # 4-byte Reload
	adcl	$0, %eax
	movl	%eax, 120(%ebp)
	movl	112(%esp), %eax                 # 4-byte Reload
	adcl	$0, %eax
	movl	%eax, 124(%ebp)
	addl	$284, %esp                      # imm = 0x11C
	popl	%esi
	popl	%edi
	popl	%ebx
	popl	%ebp
	retl
.Lfunc_end76:
	.size	mcl_fpDbl_sqrPre16L, .Lfunc_end76-mcl_fpDbl_sqrPre16L
                                        # -- End function
	.globl	mcl_fp_mont16L                  # -- Begin function mcl_fp_mont16L
	.p2align	4, 0x90
	.type	mcl_fp_mont16L,@function
mcl_fp_mont16L:                         # @mcl_fp_mont16L
# %bb.0:
	pushl	%ebp
	pushl	%ebx
	pushl	%edi
	pushl	%esi
	subl	$2412, %esp                     # imm = 0x96C
	calll	.L77$pb
.L77$pb:
	popl	%ebx
.Ltmp19:
	addl	$_GLOBAL_OFFSET_TABLE_+(.Ltmp19-.L77$pb), %ebx
	movl	2444(%esp), %eax
	movl	-4(%eax), %esi
	movl	%esi, 72(%esp)                  # 4-byte Spill
	movl	2440(%esp), %ecx
	subl	$4, %esp
	leal	2348(%esp), %eax
	pushl	(%ecx)
	pushl	2444(%esp)
	pushl	%eax
	calll	mulPv512x32@PLT
	addl	$12, %esp
	movl	2344(%esp), %edi
	movl	2348(%esp), %eax
	movl	%eax, 36(%esp)                  # 4-byte Spill
	movl	%esi, %eax
	imull	%edi, %eax
	movl	2408(%esp), %ecx
	movl	%ecx, 52(%esp)                  # 4-byte Spill
	movl	2404(%esp), %ecx
	movl	%ecx, 56(%esp)                  # 4-byte Spill
	movl	2400(%esp), %ecx
	movl	%ecx, 12(%esp)                  # 4-byte Spill
	movl	2396(%esp), %ecx
	movl	%ecx, 68(%esp)                  # 4-byte Spill
	movl	2392(%esp), %ecx
	movl	%ecx, 64(%esp)                  # 4-byte Spill
	movl	2388(%esp), %ecx
	movl	%ecx, 20(%esp)                  # 4-byte Spill
	movl	2384(%esp), %ecx
	movl	%ecx, 4(%esp)                   # 4-byte Spill
	movl	2380(%esp), %ecx
	movl	%ecx, 60(%esp)                  # 4-byte Spill
	movl	2376(%esp), %ecx
	movl	%ecx, 32(%esp)                  # 4-byte Spill
	movl	2372(%esp), %ecx
	movl	%ecx, 40(%esp)                  # 4-byte Spill
	movl	2368(%esp), %ecx
	movl	%ecx, 16(%esp)                  # 4-byte Spill
	movl	2364(%esp), %ebp
	movl	2360(%esp), %esi
	movl	2356(%esp), %ecx
	movl	%ecx, 8(%esp)                   # 4-byte Spill
	movl	2352(%esp), %ecx
	movl	%ecx, 44(%esp)                  # 4-byte Spill
	subl	$4, %esp
	leal	2276(%esp), %ecx
	pushl	%eax
	pushl	2452(%esp)
	pushl	%ecx
	calll	mulPv512x32@PLT
	addl	$12, %esp
	addl	2272(%esp), %edi
	movl	36(%esp), %eax                  # 4-byte Reload
	adcl	2276(%esp), %eax
	movl	%eax, 36(%esp)                  # 4-byte Spill
	movl	44(%esp), %eax                  # 4-byte Reload
	adcl	2280(%esp), %eax
	movl	%eax, 44(%esp)                  # 4-byte Spill
	movl	8(%esp), %eax                   # 4-byte Reload
	adcl	2284(%esp), %eax
	movl	%eax, 8(%esp)                   # 4-byte Spill
	adcl	2288(%esp), %esi
	movl	%esi, 28(%esp)                  # 4-byte Spill
	adcl	2292(%esp), %ebp
	movl	%ebp, 24(%esp)                  # 4-byte Spill
	movl	16(%esp), %eax                  # 4-byte Reload
	adcl	2296(%esp), %eax
	movl	%eax, 16(%esp)                  # 4-byte Spill
	movl	40(%esp), %eax                  # 4-byte Reload
	adcl	2300(%esp), %eax
	movl	%eax, 40(%esp)                  # 4-byte Spill
	movl	32(%esp), %eax                  # 4-byte Reload
	adcl	2304(%esp), %eax
	movl	%eax, 32(%esp)                  # 4-byte Spill
	movl	60(%esp), %edi                  # 4-byte Reload
	adcl	2308(%esp), %edi
	movl	4(%esp), %eax                   # 4-byte Reload
	adcl	2312(%esp), %eax
	movl	%eax, 4(%esp)                   # 4-byte Spill
	movl	20(%esp), %ebp                  # 4-byte Reload
	adcl	2316(%esp), %ebp
	movl	64(%esp), %eax                  # 4-byte Reload
	adcl	2320(%esp), %eax
	movl	%eax, 64(%esp)                  # 4-byte Spill
	movl	68(%esp), %eax                  # 4-byte Reload
	adcl	2324(%esp), %eax
	movl	%eax, 68(%esp)                  # 4-byte Spill
	movl	12(%esp), %eax                  # 4-byte Reload
	adcl	2328(%esp), %eax
	movl	%eax, 12(%esp)                  # 4-byte Spill
	movl	56(%esp), %esi                  # 4-byte Reload
	adcl	2332(%esp), %esi
	movl	52(%esp), %eax                  # 4-byte Reload
	adcl	2336(%esp), %eax
	movl	%eax, 52(%esp)                  # 4-byte Spill
	setb	%al
	subl	$4, %esp
	leal	2204(%esp), %ecx
	movzbl	%al, %eax
	movl	%eax, 52(%esp)                  # 4-byte Spill
	movl	2444(%esp), %eax
	pushl	4(%eax)
	pushl	2444(%esp)
	pushl	%ecx
	calll	mulPv512x32@PLT
	addl	$12, %esp
	movl	36(%esp), %ecx                  # 4-byte Reload
	addl	2200(%esp), %ecx
	movl	44(%esp), %eax                  # 4-byte Reload
	adcl	2204(%esp), %eax
	movl	%eax, 44(%esp)                  # 4-byte Spill
	movl	8(%esp), %eax                   # 4-byte Reload
	adcl	2208(%esp), %eax
	movl	%eax, 8(%esp)                   # 4-byte Spill
	movl	28(%esp), %eax                  # 4-byte Reload
	adcl	2212(%esp), %eax
	movl	%eax, 28(%esp)                  # 4-byte Spill
	movl	24(%esp), %eax                  # 4-byte Reload
	adcl	2216(%esp), %eax
	movl	%eax, 24(%esp)                  # 4-byte Spill
	movl	16(%esp), %eax                  # 4-byte Reload
	adcl	2220(%esp), %eax
	movl	%eax, 16(%esp)                  # 4-byte Spill
	movl	40(%esp), %eax                  # 4-byte Reload
	adcl	2224(%esp), %eax
	movl	%eax, 40(%esp)                  # 4-byte Spill
	movl	32(%esp), %eax                  # 4-byte Reload
	adcl	2228(%esp), %eax
	movl	%eax, 32(%esp)                  # 4-byte Spill
	adcl	2232(%esp), %edi
	movl	%edi, 60(%esp)                  # 4-byte Spill
	movl	4(%esp), %eax                   # 4-byte Reload
	adcl	2236(%esp), %eax
	movl	%eax, 4(%esp)                   # 4-byte Spill
	adcl	2240(%esp), %ebp
	movl	%ebp, 20(%esp)                  # 4-byte Spill
	movl	64(%esp), %edi                  # 4-byte Reload
	adcl	2244(%esp), %edi
	movl	68(%esp), %eax                  # 4-byte Reload
	adcl	2248(%esp), %eax
	movl	%eax, 68(%esp)                  # 4-byte Spill
	movl	12(%esp), %eax                  # 4-byte Reload
	adcl	2252(%esp), %eax
	movl	%eax, 12(%esp)                  # 4-byte Spill
	adcl	2256(%esp), %esi
	movl	%esi, 56(%esp)                  # 4-byte Spill
	movl	52(%esp), %eax                  # 4-byte Reload
	adcl	2260(%esp), %eax
	movl	%eax, 52(%esp)                  # 4-byte Spill
	movl	48(%esp), %eax                  # 4-byte Reload
	adcl	2264(%esp), %eax
	movl	%eax, 48(%esp)                  # 4-byte Spill
	setb	%al
	subl	$4, %esp
	leal	2132(%esp), %ebp
	movl	76(%esp), %edx                  # 4-byte Reload
	movl	%ecx, %esi
	imull	%ecx, %edx
	movzbl	%al, %eax
	movl	%eax, 40(%esp)                  # 4-byte Spill
	pushl	%edx
	pushl	2452(%esp)
	pushl	%ebp
	calll	mulPv512x32@PLT
	addl	$12, %esp
	addl	2128(%esp), %esi
	movl	44(%esp), %eax                  # 4-byte Reload
	adcl	2132(%esp), %eax
	movl	%eax, 44(%esp)                  # 4-byte Spill
	movl	8(%esp), %eax                   # 4-byte Reload
	adcl	2136(%esp), %eax
	movl	%eax, 8(%esp)                   # 4-byte Spill
	movl	28(%esp), %eax                  # 4-byte Reload
	adcl	2140(%esp), %eax
	movl	%eax, 28(%esp)                  # 4-byte Spill
	movl	24(%esp), %eax                  # 4-byte Reload
	adcl	2144(%esp), %eax
	movl	%eax, 24(%esp)                  # 4-byte Spill
	movl	16(%esp), %ebp                  # 4-byte Reload
	adcl	2148(%esp), %ebp
	movl	40(%esp), %eax                  # 4-byte Reload
	adcl	2152(%esp), %eax
	movl	%eax, 40(%esp)                  # 4-byte Spill
	movl	32(%esp), %eax                  # 4-byte Reload
	adcl	2156(%esp), %eax
	movl	%eax, 32(%esp)                  # 4-byte Spill
	movl	60(%esp), %eax                  # 4-byte Reload
	adcl	2160(%esp), %eax
	movl	%eax, 60(%esp)                  # 4-byte Spill
	movl	4(%esp), %eax                   # 4-byte Reload
	adcl	2164(%esp), %eax
	movl	%eax, 4(%esp)                   # 4-byte Spill
	movl	20(%esp), %eax                  # 4-byte Reload
	adcl	2168(%esp), %eax
	movl	%eax, 20(%esp)                  # 4-byte Spill
	adcl	2172(%esp), %edi
	movl	%edi, 64(%esp)                  # 4-byte Spill
	movl	68(%esp), %eax                  # 4-byte Reload
	adcl	2176(%esp), %eax
	movl	%eax, 68(%esp)                  # 4-byte Spill
	movl	12(%esp), %esi                  # 4-byte Reload
	adcl	2180(%esp), %esi
	movl	56(%esp), %eax                  # 4-byte Reload
	adcl	2184(%esp), %eax
	movl	%eax, 56(%esp)                  # 4-byte Spill
	movl	52(%esp), %edi                  # 4-byte Reload
	adcl	2188(%esp), %edi
	movl	48(%esp), %eax                  # 4-byte Reload
	adcl	2192(%esp), %eax
	movl	%eax, 48(%esp)                  # 4-byte Spill
	adcl	$0, 36(%esp)                    # 4-byte Folded Spill
	subl	$4, %esp
	leal	2060(%esp), %eax
	movl	2444(%esp), %ecx
	pushl	8(%ecx)
	pushl	2444(%esp)
	pushl	%eax
	calll	mulPv512x32@PLT
	addl	$12, %esp
	movl	44(%esp), %ecx                  # 4-byte Reload
	addl	2056(%esp), %ecx
	movl	8(%esp), %eax                   # 4-byte Reload
	adcl	2060(%esp), %eax
	movl	%eax, 8(%esp)                   # 4-byte Spill
	movl	28(%esp), %eax                  # 4-byte Reload
	adcl	2064(%esp), %eax
	movl	%eax, 28(%esp)                  # 4-byte Spill
	movl	24(%esp), %eax                  # 4-byte Reload
	adcl	2068(%esp), %eax
	movl	%eax, 24(%esp)                  # 4-byte Spill
	adcl	2072(%esp), %ebp
	movl	%ebp, 16(%esp)                  # 4-byte Spill
	movl	40(%esp), %eax                  # 4-byte Reload
	adcl	2076(%esp), %eax
	movl	%eax, 40(%esp)                  # 4-byte Spill
	movl	32(%esp), %eax                  # 4-byte Reload
	adcl	2080(%esp), %eax
	movl	%eax, 32(%esp)                  # 4-byte Spill
	movl	60(%esp), %eax                  # 4-byte Reload
	adcl	2084(%esp), %eax
	movl	%eax, 60(%esp)                  # 4-byte Spill
	movl	4(%esp), %eax                   # 4-byte Reload
	adcl	2088(%esp), %eax
	movl	%eax, 4(%esp)                   # 4-byte Spill
	movl	20(%esp), %eax                  # 4-byte Reload
	adcl	2092(%esp), %eax
	movl	%eax, 20(%esp)                  # 4-byte Spill
	movl	64(%esp), %eax                  # 4-byte Reload
	adcl	2096(%esp), %eax
	movl	%eax, 64(%esp)                  # 4-byte Spill
	movl	68(%esp), %ebp                  # 4-byte Reload
	adcl	2100(%esp), %ebp
	adcl	2104(%esp), %esi
	movl	%esi, 12(%esp)                  # 4-byte Spill
	movl	56(%esp), %eax                  # 4-byte Reload
	adcl	2108(%esp), %eax
	movl	%eax, 56(%esp)                  # 4-byte Spill
	adcl	2112(%esp), %edi
	movl	48(%esp), %eax                  # 4-byte Reload
	adcl	2116(%esp), %eax
	movl	%eax, 48(%esp)                  # 4-byte Spill
	movl	36(%esp), %eax                  # 4-byte Reload
	adcl	2120(%esp), %eax
	movl	%eax, 36(%esp)                  # 4-byte Spill
	setb	%al
	subl	$4, %esp
	movl	76(%esp), %edx                  # 4-byte Reload
	imull	%ecx, %edx
	movl	%ecx, %esi
	movzbl	%al, %eax
	movl	%eax, 48(%esp)                  # 4-byte Spill
	pushl	%edx
	pushl	2452(%esp)
	leal	1996(%esp), %eax
	pushl	%eax
	calll	mulPv512x32@PLT
	addl	$12, %esp
	addl	1984(%esp), %esi
	movl	8(%esp), %eax                   # 4-byte Reload
	adcl	1988(%esp), %eax
	movl	%eax, 8(%esp)                   # 4-byte Spill
	movl	28(%esp), %eax                  # 4-byte Reload
	adcl	1992(%esp), %eax
	movl	%eax, 28(%esp)                  # 4-byte Spill
	movl	24(%esp), %eax                  # 4-byte Reload
	adcl	1996(%esp), %eax
	movl	%eax, 24(%esp)                  # 4-byte Spill
	movl	16(%esp), %eax                  # 4-byte Reload
	adcl	2000(%esp), %eax
	movl	%eax, 16(%esp)                  # 4-byte Spill
	movl	40(%esp), %eax                  # 4-byte Reload
	adcl	2004(%esp), %eax
	movl	%eax, 40(%esp)                  # 4-byte Spill
	movl	32(%esp), %eax                  # 4-byte Reload
	adcl	2008(%esp), %eax
	movl	%eax, 32(%esp)                  # 4-byte Spill
	movl	60(%esp), %eax                  # 4-byte Reload
	adcl	2012(%esp), %eax
	movl	%eax, 60(%esp)                  # 4-byte Spill
	movl	4(%esp), %eax                   # 4-byte Reload
	adcl	2016(%esp), %eax
	movl	%eax, 4(%esp)                   # 4-byte Spill
	movl	20(%esp), %eax                  # 4-byte Reload
	adcl	2020(%esp), %eax
	movl	%eax, 20(%esp)                  # 4-byte Spill
	movl	64(%esp), %eax                  # 4-byte Reload
	adcl	2024(%esp), %eax
	movl	%eax, 64(%esp)                  # 4-byte Spill
	adcl	2028(%esp), %ebp
	movl	%ebp, 68(%esp)                  # 4-byte Spill
	movl	12(%esp), %eax                  # 4-byte Reload
	adcl	2032(%esp), %eax
	movl	%eax, 12(%esp)                  # 4-byte Spill
	movl	56(%esp), %eax                  # 4-byte Reload
	adcl	2036(%esp), %eax
	movl	%eax, 56(%esp)                  # 4-byte Spill
	adcl	2040(%esp), %edi
	movl	%edi, 52(%esp)                  # 4-byte Spill
	movl	48(%esp), %ebp                  # 4-byte Reload
	adcl	2044(%esp), %ebp
	movl	36(%esp), %eax                  # 4-byte Reload
	adcl	2048(%esp), %eax
	movl	%eax, 36(%esp)                  # 4-byte Spill
	movl	44(%esp), %esi                  # 4-byte Reload
	adcl	$0, %esi
	subl	$4, %esp
	leal	1916(%esp), %eax
	movl	2444(%esp), %ecx
	pushl	12(%ecx)
	pushl	2444(%esp)
	pushl	%eax
	calll	mulPv512x32@PLT
	addl	$12, %esp
	movl	8(%esp), %ecx                   # 4-byte Reload
	addl	1912(%esp), %ecx
	movl	28(%esp), %eax                  # 4-byte Reload
	adcl	1916(%esp), %eax
	movl	%eax, 28(%esp)                  # 4-byte Spill
	movl	24(%esp), %eax                  # 4-byte Reload
	adcl	1920(%esp), %eax
	movl	%eax, 24(%esp)                  # 4-byte Spill
	movl	16(%esp), %eax                  # 4-byte Reload
	adcl	1924(%esp), %eax
	movl	%eax, 16(%esp)                  # 4-byte Spill
	movl	40(%esp), %eax                  # 4-byte Reload
	adcl	1928(%esp), %eax
	movl	%eax, 40(%esp)                  # 4-byte Spill
	movl	32(%esp), %edi                  # 4-byte Reload
	adcl	1932(%esp), %edi
	movl	60(%esp), %eax                  # 4-byte Reload
	adcl	1936(%esp), %eax
	movl	%eax, 60(%esp)                  # 4-byte Spill
	movl	4(%esp), %eax                   # 4-byte Reload
	adcl	1940(%esp), %eax
	movl	%eax, 4(%esp)                   # 4-byte Spill
	movl	20(%esp), %eax                  # 4-byte Reload
	adcl	1944(%esp), %eax
	movl	%eax, 20(%esp)                  # 4-byte Spill
	movl	64(%esp), %eax                  # 4-byte Reload
	adcl	1948(%esp), %eax
	movl	%eax, 64(%esp)                  # 4-byte Spill
	movl	68(%esp), %eax                  # 4-byte Reload
	adcl	1952(%esp), %eax
	movl	%eax, 68(%esp)                  # 4-byte Spill
	movl	12(%esp), %eax                  # 4-byte Reload
	adcl	1956(%esp), %eax
	movl	%eax, 12(%esp)                  # 4-byte Spill
	movl	56(%esp), %eax                  # 4-byte Reload
	adcl	1960(%esp), %eax
	movl	%eax, 56(%esp)                  # 4-byte Spill
	movl	52(%esp), %eax                  # 4-byte Reload
	adcl	1964(%esp), %eax
	movl	%eax, 52(%esp)                  # 4-byte Spill
	adcl	1968(%esp), %ebp
	movl	%ebp, 48(%esp)                  # 4-byte Spill
	movl	36(%esp), %eax                  # 4-byte Reload
	adcl	1972(%esp), %eax
	movl	%eax, 36(%esp)                  # 4-byte Spill
	adcl	1976(%esp), %esi
	movl	%esi, 44(%esp)                  # 4-byte Spill
	setb	%al
	leal	1840(%esp), %ebp
	subl	$4, %esp
	movl	76(%esp), %edx                  # 4-byte Reload
	movl	%ecx, %esi
	imull	%ecx, %edx
	movzbl	%al, %eax
	movl	%eax, 12(%esp)                  # 4-byte Spill
	pushl	%edx
	movl	2452(%esp), %eax
	pushl	%eax
	pushl	%ebp
	calll	mulPv512x32@PLT
	addl	$12, %esp
	addl	1840(%esp), %esi
	movl	28(%esp), %eax                  # 4-byte Reload
	adcl	1844(%esp), %eax
	movl	%eax, 28(%esp)                  # 4-byte Spill
	movl	24(%esp), %eax                  # 4-byte Reload
	adcl	1848(%esp), %eax
	movl	%eax, 24(%esp)                  # 4-byte Spill
	movl	16(%esp), %eax                  # 4-byte Reload
	adcl	1852(%esp), %eax
	movl	%eax, 16(%esp)                  # 4-byte Spill
	movl	40(%esp), %ebp                  # 4-byte Reload
	adcl	1856(%esp), %ebp
	adcl	1860(%esp), %edi
	movl	%edi, 32(%esp)                  # 4-byte Spill
	movl	60(%esp), %eax                  # 4-byte Reload
	adcl	1864(%esp), %eax
	movl	%eax, 60(%esp)                  # 4-byte Spill
	movl	4(%esp), %edi                   # 4-byte Reload
	adcl	1868(%esp), %edi
	movl	20(%esp), %eax                  # 4-byte Reload
	adcl	1872(%esp), %eax
	movl	%eax, 20(%esp)                  # 4-byte Spill
	movl	64(%esp), %eax                  # 4-byte Reload
	adcl	1876(%esp), %eax
	movl	%eax, 64(%esp)                  # 4-byte Spill
	movl	68(%esp), %eax                  # 4-byte Reload
	adcl	1880(%esp), %eax
	movl	%eax, 68(%esp)                  # 4-byte Spill
	movl	12(%esp), %eax                  # 4-byte Reload
	adcl	1884(%esp), %eax
	movl	%eax, 12(%esp)                  # 4-byte Spill
	movl	56(%esp), %eax                  # 4-byte Reload
	adcl	1888(%esp), %eax
	movl	%eax, 56(%esp)                  # 4-byte Spill
	movl	52(%esp), %eax                  # 4-byte Reload
	adcl	1892(%esp), %eax
	movl	%eax, 52(%esp)                  # 4-byte Spill
	movl	48(%esp), %eax                  # 4-byte Reload
	adcl	1896(%esp), %eax
	movl	%eax, 48(%esp)                  # 4-byte Spill
	movl	36(%esp), %esi                  # 4-byte Reload
	adcl	1900(%esp), %esi
	movl	44(%esp), %eax                  # 4-byte Reload
	adcl	1904(%esp), %eax
	movl	%eax, 44(%esp)                  # 4-byte Spill
	adcl	$0, 8(%esp)                     # 4-byte Folded Spill
	subl	$4, %esp
	movl	2444(%esp), %eax
	pushl	16(%eax)
	pushl	2444(%esp)
	leal	1780(%esp), %eax
	pushl	%eax
	calll	mulPv512x32@PLT
	addl	$12, %esp
	movl	28(%esp), %edx                  # 4-byte Reload
	addl	1768(%esp), %edx
	movl	24(%esp), %eax                  # 4-byte Reload
	adcl	1772(%esp), %eax
	movl	%eax, 24(%esp)                  # 4-byte Spill
	movl	16(%esp), %eax                  # 4-byte Reload
	adcl	1776(%esp), %eax
	movl	%eax, 16(%esp)                  # 4-byte Spill
	adcl	1780(%esp), %ebp
	movl	%ebp, 40(%esp)                  # 4-byte Spill
	movl	32(%esp), %eax                  # 4-byte Reload
	adcl	1784(%esp), %eax
	movl	%eax, 32(%esp)                  # 4-byte Spill
	movl	60(%esp), %eax                  # 4-byte Reload
	adcl	1788(%esp), %eax
	movl	%eax, 60(%esp)                  # 4-byte Spill
	adcl	1792(%esp), %edi
	movl	%edi, 4(%esp)                   # 4-byte Spill
	movl	20(%esp), %eax                  # 4-byte Reload
	adcl	1796(%esp), %eax
	movl	%eax, 20(%esp)                  # 4-byte Spill
	movl	64(%esp), %ebp                  # 4-byte Reload
	adcl	1800(%esp), %ebp
	movl	68(%esp), %eax                  # 4-byte Reload
	adcl	1804(%esp), %eax
	movl	%eax, 68(%esp)                  # 4-byte Spill
	movl	12(%esp), %eax                  # 4-byte Reload
	adcl	1808(%esp), %eax
	movl	%eax, 12(%esp)                  # 4-byte Spill
	movl	56(%esp), %edi                  # 4-byte Reload
	adcl	1812(%esp), %edi
	movl	52(%esp), %eax                  # 4-byte Reload
	adcl	1816(%esp), %eax
	movl	%eax, 52(%esp)                  # 4-byte Spill
	movl	48(%esp), %eax                  # 4-byte Reload
	adcl	1820(%esp), %eax
	movl	%eax, 48(%esp)                  # 4-byte Spill
	adcl	1824(%esp), %esi
	movl	%esi, 36(%esp)                  # 4-byte Spill
	movl	44(%esp), %eax                  # 4-byte Reload
	adcl	1828(%esp), %eax
	movl	%eax, 44(%esp)                  # 4-byte Spill
	movl	8(%esp), %eax                   # 4-byte Reload
	adcl	1832(%esp), %eax
	movl	%eax, 8(%esp)                   # 4-byte Spill
	setb	%al
	subl	$4, %esp
	movl	76(%esp), %ecx                  # 4-byte Reload
	movl	%edx, %esi
	imull	%edx, %ecx
	movzbl	%al, %eax
	movl	%eax, 32(%esp)                  # 4-byte Spill
	pushl	%ecx
	pushl	2452(%esp)
	leal	1708(%esp), %eax
	pushl	%eax
	calll	mulPv512x32@PLT
	addl	$12, %esp
	addl	1696(%esp), %esi
	movl	24(%esp), %eax                  # 4-byte Reload
	adcl	1700(%esp), %eax
	movl	%eax, 24(%esp)                  # 4-byte Spill
	movl	16(%esp), %eax                  # 4-byte Reload
	adcl	1704(%esp), %eax
	movl	%eax, 16(%esp)                  # 4-byte Spill
	movl	40(%esp), %eax                  # 4-byte Reload
	adcl	1708(%esp), %eax
	movl	%eax, 40(%esp)                  # 4-byte Spill
	movl	32(%esp), %eax                  # 4-byte Reload
	adcl	1712(%esp), %eax
	movl	%eax, 32(%esp)                  # 4-byte Spill
	movl	60(%esp), %eax                  # 4-byte Reload
	adcl	1716(%esp), %eax
	movl	%eax, 60(%esp)                  # 4-byte Spill
	movl	4(%esp), %eax                   # 4-byte Reload
	adcl	1720(%esp), %eax
	movl	%eax, 4(%esp)                   # 4-byte Spill
	movl	20(%esp), %eax                  # 4-byte Reload
	adcl	1724(%esp), %eax
	movl	%eax, 20(%esp)                  # 4-byte Spill
	adcl	1728(%esp), %ebp
	movl	%ebp, 64(%esp)                  # 4-byte Spill
	movl	68(%esp), %eax                  # 4-byte Reload
	adcl	1732(%esp), %eax
	movl	%eax, 68(%esp)                  # 4-byte Spill
	movl	12(%esp), %esi                  # 4-byte Reload
	adcl	1736(%esp), %esi
	adcl	1740(%esp), %edi
	movl	%edi, 56(%esp)                  # 4-byte Spill
	movl	52(%esp), %eax                  # 4-byte Reload
	adcl	1744(%esp), %eax
	movl	%eax, 52(%esp)                  # 4-byte Spill
	movl	48(%esp), %eax                  # 4-byte Reload
	adcl	1748(%esp), %eax
	movl	%eax, 48(%esp)                  # 4-byte Spill
	movl	36(%esp), %eax                  # 4-byte Reload
	adcl	1752(%esp), %eax
	movl	%eax, 36(%esp)                  # 4-byte Spill
	movl	44(%esp), %eax                  # 4-byte Reload
	adcl	1756(%esp), %eax
	movl	%eax, 44(%esp)                  # 4-byte Spill
	movl	8(%esp), %eax                   # 4-byte Reload
	adcl	1760(%esp), %eax
	movl	%eax, 8(%esp)                   # 4-byte Spill
	adcl	$0, 28(%esp)                    # 4-byte Folded Spill
	subl	$4, %esp
	movl	2444(%esp), %eax
	pushl	20(%eax)
	pushl	2444(%esp)
	leal	1636(%esp), %eax
	pushl	%eax
	calll	mulPv512x32@PLT
	addl	$12, %esp
	movl	24(%esp), %edx                  # 4-byte Reload
	addl	1624(%esp), %edx
	movl	16(%esp), %eax                  # 4-byte Reload
	adcl	1628(%esp), %eax
	movl	%eax, 16(%esp)                  # 4-byte Spill
	movl	40(%esp), %eax                  # 4-byte Reload
	adcl	1632(%esp), %eax
	movl	%eax, 40(%esp)                  # 4-byte Spill
	movl	32(%esp), %eax                  # 4-byte Reload
	adcl	1636(%esp), %eax
	movl	%eax, 32(%esp)                  # 4-byte Spill
	movl	60(%esp), %ebp                  # 4-byte Reload
	adcl	1640(%esp), %ebp
	movl	4(%esp), %eax                   # 4-byte Reload
	adcl	1644(%esp), %eax
	movl	%eax, 4(%esp)                   # 4-byte Spill
	movl	20(%esp), %edi                  # 4-byte Reload
	adcl	1648(%esp), %edi
	movl	64(%esp), %eax                  # 4-byte Reload
	adcl	1652(%esp), %eax
	movl	%eax, 64(%esp)                  # 4-byte Spill
	movl	68(%esp), %eax                  # 4-byte Reload
	adcl	1656(%esp), %eax
	movl	%eax, 68(%esp)                  # 4-byte Spill
	adcl	1660(%esp), %esi
	movl	%esi, 12(%esp)                  # 4-byte Spill
	movl	56(%esp), %eax                  # 4-byte Reload
	adcl	1664(%esp), %eax
	movl	%eax, 56(%esp)                  # 4-byte Spill
	movl	52(%esp), %eax                  # 4-byte Reload
	adcl	1668(%esp), %eax
	movl	%eax, 52(%esp)                  # 4-byte Spill
	movl	48(%esp), %eax                  # 4-byte Reload
	adcl	1672(%esp), %eax
	movl	%eax, 48(%esp)                  # 4-byte Spill
	movl	36(%esp), %eax                  # 4-byte Reload
	adcl	1676(%esp), %eax
	movl	%eax, 36(%esp)                  # 4-byte Spill
	movl	44(%esp), %eax                  # 4-byte Reload
	adcl	1680(%esp), %eax
	movl	%eax, 44(%esp)                  # 4-byte Spill
	movl	8(%esp), %eax                   # 4-byte Reload
	adcl	1684(%esp), %eax
	movl	%eax, 8(%esp)                   # 4-byte Spill
	movl	28(%esp), %eax                  # 4-byte Reload
	adcl	1688(%esp), %eax
	movl	%eax, 28(%esp)                  # 4-byte Spill
	setb	%al
	subl	$4, %esp
	movl	76(%esp), %ecx                  # 4-byte Reload
	movl	%edx, %esi
	imull	%edx, %ecx
	movzbl	%al, %eax
	movl	%eax, 28(%esp)                  # 4-byte Spill
	pushl	%ecx
	pushl	2452(%esp)
	leal	1564(%esp), %eax
	pushl	%eax
	calll	mulPv512x32@PLT
	addl	$12, %esp
	addl	1552(%esp), %esi
	movl	16(%esp), %eax                  # 4-byte Reload
	adcl	1556(%esp), %eax
	movl	%eax, 16(%esp)                  # 4-byte Spill
	movl	40(%esp), %eax                  # 4-byte Reload
	adcl	1560(%esp), %eax
	movl	%eax, 40(%esp)                  # 4-byte Spill
	movl	32(%esp), %eax                  # 4-byte Reload
	adcl	1564(%esp), %eax
	movl	%eax, 32(%esp)                  # 4-byte Spill
	adcl	1568(%esp), %ebp
	movl	%ebp, 60(%esp)                  # 4-byte Spill
	movl	4(%esp), %eax                   # 4-byte Reload
	adcl	1572(%esp), %eax
	movl	%eax, 4(%esp)                   # 4-byte Spill
	adcl	1576(%esp), %edi
	movl	%edi, 20(%esp)                  # 4-byte Spill
	movl	64(%esp), %eax                  # 4-byte Reload
	adcl	1580(%esp), %eax
	movl	%eax, 64(%esp)                  # 4-byte Spill
	movl	68(%esp), %esi                  # 4-byte Reload
	adcl	1584(%esp), %esi
	movl	12(%esp), %eax                  # 4-byte Reload
	adcl	1588(%esp), %eax
	movl	%eax, 12(%esp)                  # 4-byte Spill
	movl	56(%esp), %eax                  # 4-byte Reload
	adcl	1592(%esp), %eax
	movl	%eax, 56(%esp)                  # 4-byte Spill
	movl	52(%esp), %eax                  # 4-byte Reload
	adcl	1596(%esp), %eax
	movl	%eax, 52(%esp)                  # 4-byte Spill
	movl	48(%esp), %eax                  # 4-byte Reload
	adcl	1600(%esp), %eax
	movl	%eax, 48(%esp)                  # 4-byte Spill
	movl	36(%esp), %eax                  # 4-byte Reload
	adcl	1604(%esp), %eax
	movl	%eax, 36(%esp)                  # 4-byte Spill
	movl	44(%esp), %eax                  # 4-byte Reload
	adcl	1608(%esp), %eax
	movl	%eax, 44(%esp)                  # 4-byte Spill
	movl	8(%esp), %ebp                   # 4-byte Reload
	adcl	1612(%esp), %ebp
	movl	28(%esp), %eax                  # 4-byte Reload
	adcl	1616(%esp), %eax
	movl	%eax, 28(%esp)                  # 4-byte Spill
	adcl	$0, 24(%esp)                    # 4-byte Folded Spill
	subl	$4, %esp
	movl	2444(%esp), %eax
	pushl	24(%eax)
	pushl	2444(%esp)
	leal	1492(%esp), %eax
	pushl	%eax
	calll	mulPv512x32@PLT
	addl	$12, %esp
	movl	16(%esp), %edx                  # 4-byte Reload
	addl	1480(%esp), %edx
	movl	40(%esp), %eax                  # 4-byte Reload
	adcl	1484(%esp), %eax
	movl	%eax, 40(%esp)                  # 4-byte Spill
	movl	32(%esp), %edi                  # 4-byte Reload
	adcl	1488(%esp), %edi
	movl	60(%esp), %eax                  # 4-byte Reload
	adcl	1492(%esp), %eax
	movl	%eax, 60(%esp)                  # 4-byte Spill
	movl	4(%esp), %eax                   # 4-byte Reload
	adcl	1496(%esp), %eax
	movl	%eax, 4(%esp)                   # 4-byte Spill
	movl	20(%esp), %eax                  # 4-byte Reload
	adcl	1500(%esp), %eax
	movl	%eax, 20(%esp)                  # 4-byte Spill
	movl	64(%esp), %eax                  # 4-byte Reload
	adcl	1504(%esp), %eax
	movl	%eax, 64(%esp)                  # 4-byte Spill
	adcl	1508(%esp), %esi
	movl	12(%esp), %eax                  # 4-byte Reload
	adcl	1512(%esp), %eax
	movl	%eax, 12(%esp)                  # 4-byte Spill
	movl	56(%esp), %eax                  # 4-byte Reload
	adcl	1516(%esp), %eax
	movl	%eax, 56(%esp)                  # 4-byte Spill
	movl	52(%esp), %eax                  # 4-byte Reload
	adcl	1520(%esp), %eax
	movl	%eax, 52(%esp)                  # 4-byte Spill
	movl	48(%esp), %eax                  # 4-byte Reload
	adcl	1524(%esp), %eax
	movl	%eax, 48(%esp)                  # 4-byte Spill
	movl	36(%esp), %eax                  # 4-byte Reload
	adcl	1528(%esp), %eax
	movl	%eax, 36(%esp)                  # 4-byte Spill
	movl	44(%esp), %eax                  # 4-byte Reload
	adcl	1532(%esp), %eax
	movl	%eax, 44(%esp)                  # 4-byte Spill
	adcl	1536(%esp), %ebp
	movl	%ebp, 8(%esp)                   # 4-byte Spill
	movl	28(%esp), %eax                  # 4-byte Reload
	adcl	1540(%esp), %eax
	movl	%eax, 28(%esp)                  # 4-byte Spill
	movl	24(%esp), %eax                  # 4-byte Reload
	adcl	1544(%esp), %eax
	movl	%eax, 24(%esp)                  # 4-byte Spill
	setb	%al
	subl	$4, %esp
	movl	76(%esp), %ecx                  # 4-byte Reload
	movl	%edx, %ebp
	imull	%edx, %ecx
	movzbl	%al, %eax
	movl	%eax, 20(%esp)                  # 4-byte Spill
	pushl	%ecx
	pushl	2452(%esp)
	leal	1420(%esp), %eax
	pushl	%eax
	calll	mulPv512x32@PLT
	addl	$12, %esp
	addl	1408(%esp), %ebp
	movl	40(%esp), %eax                  # 4-byte Reload
	adcl	1412(%esp), %eax
	movl	%eax, 40(%esp)                  # 4-byte Spill
	adcl	1416(%esp), %edi
	movl	%edi, 32(%esp)                  # 4-byte Spill
	movl	60(%esp), %ebp                  # 4-byte Reload
	adcl	1420(%esp), %ebp
	movl	4(%esp), %eax                   # 4-byte Reload
	adcl	1424(%esp), %eax
	movl	%eax, 4(%esp)                   # 4-byte Spill
	movl	20(%esp), %eax                  # 4-byte Reload
	adcl	1428(%esp), %eax
	movl	%eax, 20(%esp)                  # 4-byte Spill
	movl	64(%esp), %edi                  # 4-byte Reload
	adcl	1432(%esp), %edi
	adcl	1436(%esp), %esi
	movl	12(%esp), %eax                  # 4-byte Reload
	adcl	1440(%esp), %eax
	movl	%eax, 12(%esp)                  # 4-byte Spill
	movl	56(%esp), %eax                  # 4-byte Reload
	adcl	1444(%esp), %eax
	movl	%eax, 56(%esp)                  # 4-byte Spill
	movl	52(%esp), %eax                  # 4-byte Reload
	adcl	1448(%esp), %eax
	movl	%eax, 52(%esp)                  # 4-byte Spill
	movl	48(%esp), %eax                  # 4-byte Reload
	adcl	1452(%esp), %eax
	movl	%eax, 48(%esp)                  # 4-byte Spill
	movl	36(%esp), %eax                  # 4-byte Reload
	adcl	1456(%esp), %eax
	movl	%eax, 36(%esp)                  # 4-byte Spill
	movl	44(%esp), %eax                  # 4-byte Reload
	adcl	1460(%esp), %eax
	movl	%eax, 44(%esp)                  # 4-byte Spill
	movl	8(%esp), %eax                   # 4-byte Reload
	adcl	1464(%esp), %eax
	movl	%eax, 8(%esp)                   # 4-byte Spill
	movl	28(%esp), %eax                  # 4-byte Reload
	adcl	1468(%esp), %eax
	movl	%eax, 28(%esp)                  # 4-byte Spill
	movl	24(%esp), %eax                  # 4-byte Reload
	adcl	1472(%esp), %eax
	movl	%eax, 24(%esp)                  # 4-byte Spill
	adcl	$0, 16(%esp)                    # 4-byte Folded Spill
	subl	$4, %esp
	movl	2444(%esp), %eax
	pushl	28(%eax)
	pushl	2444(%esp)
	leal	1348(%esp), %eax
	pushl	%eax
	calll	mulPv512x32@PLT
	addl	$12, %esp
	movl	40(%esp), %edx                  # 4-byte Reload
	addl	1336(%esp), %edx
	movl	32(%esp), %eax                  # 4-byte Reload
	adcl	1340(%esp), %eax
	movl	%eax, 32(%esp)                  # 4-byte Spill
	adcl	1344(%esp), %ebp
	movl	%ebp, 60(%esp)                  # 4-byte Spill
	movl	4(%esp), %eax                   # 4-byte Reload
	adcl	1348(%esp), %eax
	movl	%eax, 4(%esp)                   # 4-byte Spill
	movl	20(%esp), %eax                  # 4-byte Reload
	adcl	1352(%esp), %eax
	movl	%eax, 20(%esp)                  # 4-byte Spill
	adcl	1356(%esp), %edi
	movl	%edi, 64(%esp)                  # 4-byte Spill
	adcl	1360(%esp), %esi
	movl	%esi, 68(%esp)                  # 4-byte Spill
	movl	12(%esp), %eax                  # 4-byte Reload
	adcl	1364(%esp), %eax
	movl	%eax, 12(%esp)                  # 4-byte Spill
	movl	56(%esp), %eax                  # 4-byte Reload
	adcl	1368(%esp), %eax
	movl	%eax, 56(%esp)                  # 4-byte Spill
	movl	52(%esp), %eax                  # 4-byte Reload
	adcl	1372(%esp), %eax
	movl	%eax, 52(%esp)                  # 4-byte Spill
	movl	48(%esp), %eax                  # 4-byte Reload
	adcl	1376(%esp), %eax
	movl	%eax, 48(%esp)                  # 4-byte Spill
	movl	36(%esp), %eax                  # 4-byte Reload
	adcl	1380(%esp), %eax
	movl	%eax, 36(%esp)                  # 4-byte Spill
	movl	44(%esp), %edi                  # 4-byte Reload
	adcl	1384(%esp), %edi
	movl	8(%esp), %eax                   # 4-byte Reload
	adcl	1388(%esp), %eax
	movl	%eax, 8(%esp)                   # 4-byte Spill
	movl	28(%esp), %eax                  # 4-byte Reload
	adcl	1392(%esp), %eax
	movl	%eax, 28(%esp)                  # 4-byte Spill
	movl	24(%esp), %ebp                  # 4-byte Reload
	adcl	1396(%esp), %ebp
	movl	16(%esp), %eax                  # 4-byte Reload
	adcl	1400(%esp), %eax
	movl	%eax, 16(%esp)                  # 4-byte Spill
	setb	%al
	subl	$4, %esp
	movl	76(%esp), %ecx                  # 4-byte Reload
	movl	%edx, %esi
	imull	%edx, %ecx
	movzbl	%al, %eax
	movl	%eax, 44(%esp)                  # 4-byte Spill
	pushl	%ecx
	pushl	2452(%esp)
	leal	1276(%esp), %eax
	pushl	%eax
	calll	mulPv512x32@PLT
	addl	$12, %esp
	addl	1264(%esp), %esi
	movl	32(%esp), %eax                  # 4-byte Reload
	adcl	1268(%esp), %eax
	movl	%eax, 32(%esp)                  # 4-byte Spill
	movl	60(%esp), %eax                  # 4-byte Reload
	adcl	1272(%esp), %eax
	movl	%eax, 60(%esp)                  # 4-byte Spill
	movl	4(%esp), %eax                   # 4-byte Reload
	adcl	1276(%esp), %eax
	movl	%eax, 4(%esp)                   # 4-byte Spill
	movl	20(%esp), %eax                  # 4-byte Reload
	adcl	1280(%esp), %eax
	movl	%eax, 20(%esp)                  # 4-byte Spill
	movl	64(%esp), %esi                  # 4-byte Reload
	adcl	1284(%esp), %esi
	movl	68(%esp), %eax                  # 4-byte Reload
	adcl	1288(%esp), %eax
	movl	%eax, 68(%esp)                  # 4-byte Spill
	movl	12(%esp), %eax                  # 4-byte Reload
	adcl	1292(%esp), %eax
	movl	%eax, 12(%esp)                  # 4-byte Spill
	movl	56(%esp), %eax                  # 4-byte Reload
	adcl	1296(%esp), %eax
	movl	%eax, 56(%esp)                  # 4-byte Spill
	movl	52(%esp), %eax                  # 4-byte Reload
	adcl	1300(%esp), %eax
	movl	%eax, 52(%esp)                  # 4-byte Spill
	movl	48(%esp), %eax                  # 4-byte Reload
	adcl	1304(%esp), %eax
	movl	%eax, 48(%esp)                  # 4-byte Spill
	movl	36(%esp), %eax                  # 4-byte Reload
	adcl	1308(%esp), %eax
	movl	%eax, 36(%esp)                  # 4-byte Spill
	adcl	1312(%esp), %edi
	movl	%edi, 44(%esp)                  # 4-byte Spill
	movl	8(%esp), %eax                   # 4-byte Reload
	adcl	1316(%esp), %eax
	movl	%eax, 8(%esp)                   # 4-byte Spill
	movl	28(%esp), %eax                  # 4-byte Reload
	adcl	1320(%esp), %eax
	movl	%eax, 28(%esp)                  # 4-byte Spill
	adcl	1324(%esp), %ebp
	movl	%ebp, 24(%esp)                  # 4-byte Spill
	movl	16(%esp), %eax                  # 4-byte Reload
	adcl	1328(%esp), %eax
	movl	%eax, 16(%esp)                  # 4-byte Spill
	adcl	$0, 40(%esp)                    # 4-byte Folded Spill
	subl	$4, %esp
	movl	2444(%esp), %eax
	pushl	32(%eax)
	pushl	2444(%esp)
	leal	1204(%esp), %eax
	pushl	%eax
	calll	mulPv512x32@PLT
	addl	$12, %esp
	movl	32(%esp), %edx                  # 4-byte Reload
	addl	1192(%esp), %edx
	movl	60(%esp), %edi                  # 4-byte Reload
	adcl	1196(%esp), %edi
	movl	4(%esp), %eax                   # 4-byte Reload
	adcl	1200(%esp), %eax
	movl	%eax, 4(%esp)                   # 4-byte Spill
	movl	20(%esp), %eax                  # 4-byte Reload
	adcl	1204(%esp), %eax
	movl	%eax, 20(%esp)                  # 4-byte Spill
	adcl	1208(%esp), %esi
	movl	%esi, 64(%esp)                  # 4-byte Spill
	movl	68(%esp), %eax                  # 4-byte Reload
	adcl	1212(%esp), %eax
	movl	%eax, 68(%esp)                  # 4-byte Spill
	movl	12(%esp), %eax                  # 4-byte Reload
	adcl	1216(%esp), %eax
	movl	%eax, 12(%esp)                  # 4-byte Spill
	movl	56(%esp), %eax                  # 4-byte Reload
	adcl	1220(%esp), %eax
	movl	%eax, 56(%esp)                  # 4-byte Spill
	movl	52(%esp), %ebp                  # 4-byte Reload
	adcl	1224(%esp), %ebp
	movl	48(%esp), %eax                  # 4-byte Reload
	adcl	1228(%esp), %eax
	movl	%eax, 48(%esp)                  # 4-byte Spill
	movl	36(%esp), %eax                  # 4-byte Reload
	adcl	1232(%esp), %eax
	movl	%eax, 36(%esp)                  # 4-byte Spill
	movl	44(%esp), %eax                  # 4-byte Reload
	adcl	1236(%esp), %eax
	movl	%eax, 44(%esp)                  # 4-byte Spill
	movl	8(%esp), %eax                   # 4-byte Reload
	adcl	1240(%esp), %eax
	movl	%eax, 8(%esp)                   # 4-byte Spill
	movl	28(%esp), %eax                  # 4-byte Reload
	adcl	1244(%esp), %eax
	movl	%eax, 28(%esp)                  # 4-byte Spill
	movl	24(%esp), %eax                  # 4-byte Reload
	adcl	1248(%esp), %eax
	movl	%eax, 24(%esp)                  # 4-byte Spill
	movl	16(%esp), %eax                  # 4-byte Reload
	adcl	1252(%esp), %eax
	movl	%eax, 16(%esp)                  # 4-byte Spill
	movl	40(%esp), %eax                  # 4-byte Reload
	adcl	1256(%esp), %eax
	movl	%eax, 40(%esp)                  # 4-byte Spill
	setb	%al
	subl	$4, %esp
	movl	76(%esp), %ecx                  # 4-byte Reload
	imull	%edx, %ecx
	movl	%edx, %esi
	movzbl	%al, %eax
	movl	%eax, 36(%esp)                  # 4-byte Spill
	pushl	%ecx
	pushl	2452(%esp)
	leal	1132(%esp), %eax
	pushl	%eax
	calll	mulPv512x32@PLT
	addl	$12, %esp
	addl	1120(%esp), %esi
	adcl	1124(%esp), %edi
	movl	%edi, 60(%esp)                  # 4-byte Spill
	movl	4(%esp), %eax                   # 4-byte Reload
	adcl	1128(%esp), %eax
	movl	%eax, 4(%esp)                   # 4-byte Spill
	movl	20(%esp), %eax                  # 4-byte Reload
	adcl	1132(%esp), %eax
	movl	%eax, 20(%esp)                  # 4-byte Spill
	movl	64(%esp), %eax                  # 4-byte Reload
	adcl	1136(%esp), %eax
	movl	%eax, 64(%esp)                  # 4-byte Spill
	movl	68(%esp), %eax                  # 4-byte Reload
	adcl	1140(%esp), %eax
	movl	%eax, 68(%esp)                  # 4-byte Spill
	movl	12(%esp), %eax                  # 4-byte Reload
	adcl	1144(%esp), %eax
	movl	%eax, 12(%esp)                  # 4-byte Spill
	movl	56(%esp), %eax                  # 4-byte Reload
	adcl	1148(%esp), %eax
	movl	%eax, 56(%esp)                  # 4-byte Spill
	adcl	1152(%esp), %ebp
	movl	%ebp, 52(%esp)                  # 4-byte Spill
	movl	48(%esp), %esi                  # 4-byte Reload
	adcl	1156(%esp), %esi
	movl	36(%esp), %eax                  # 4-byte Reload
	adcl	1160(%esp), %eax
	movl	%eax, 36(%esp)                  # 4-byte Spill
	movl	44(%esp), %eax                  # 4-byte Reload
	adcl	1164(%esp), %eax
	movl	%eax, 44(%esp)                  # 4-byte Spill
	movl	8(%esp), %eax                   # 4-byte Reload
	adcl	1168(%esp), %eax
	movl	%eax, 8(%esp)                   # 4-byte Spill
	movl	28(%esp), %eax                  # 4-byte Reload
	adcl	1172(%esp), %eax
	movl	%eax, 28(%esp)                  # 4-byte Spill
	movl	24(%esp), %eax                  # 4-byte Reload
	adcl	1176(%esp), %eax
	movl	%eax, 24(%esp)                  # 4-byte Spill
	movl	16(%esp), %eax                  # 4-byte Reload
	adcl	1180(%esp), %eax
	movl	%eax, 16(%esp)                  # 4-byte Spill
	movl	40(%esp), %edi                  # 4-byte Reload
	adcl	1184(%esp), %edi
	adcl	$0, 32(%esp)                    # 4-byte Folded Spill
	subl	$4, %esp
	movl	2444(%esp), %eax
	pushl	36(%eax)
	pushl	2444(%esp)
	leal	1060(%esp), %eax
	pushl	%eax
	calll	mulPv512x32@PLT
	addl	$12, %esp
	movl	60(%esp), %edx                  # 4-byte Reload
	addl	1048(%esp), %edx
	movl	4(%esp), %eax                   # 4-byte Reload
	adcl	1052(%esp), %eax
	movl	%eax, 4(%esp)                   # 4-byte Spill
	movl	20(%esp), %eax                  # 4-byte Reload
	adcl	1056(%esp), %eax
	movl	%eax, 20(%esp)                  # 4-byte Spill
	movl	64(%esp), %ebp                  # 4-byte Reload
	adcl	1060(%esp), %ebp
	movl	68(%esp), %eax                  # 4-byte Reload
	adcl	1064(%esp), %eax
	movl	%eax, 68(%esp)                  # 4-byte Spill
	movl	12(%esp), %eax                  # 4-byte Reload
	adcl	1068(%esp), %eax
	movl	%eax, 12(%esp)                  # 4-byte Spill
	movl	56(%esp), %eax                  # 4-byte Reload
	adcl	1072(%esp), %eax
	movl	%eax, 56(%esp)                  # 4-byte Spill
	movl	52(%esp), %eax                  # 4-byte Reload
	adcl	1076(%esp), %eax
	movl	%eax, 52(%esp)                  # 4-byte Spill
	adcl	1080(%esp), %esi
	movl	%esi, 48(%esp)                  # 4-byte Spill
	movl	36(%esp), %eax                  # 4-byte Reload
	adcl	1084(%esp), %eax
	movl	%eax, 36(%esp)                  # 4-byte Spill
	movl	44(%esp), %eax                  # 4-byte Reload
	adcl	1088(%esp), %eax
	movl	%eax, 44(%esp)                  # 4-byte Spill
	movl	8(%esp), %eax                   # 4-byte Reload
	adcl	1092(%esp), %eax
	movl	%eax, 8(%esp)                   # 4-byte Spill
	movl	28(%esp), %eax                  # 4-byte Reload
	adcl	1096(%esp), %eax
	movl	%eax, 28(%esp)                  # 4-byte Spill
	movl	24(%esp), %eax                  # 4-byte Reload
	adcl	1100(%esp), %eax
	movl	%eax, 24(%esp)                  # 4-byte Spill
	movl	16(%esp), %eax                  # 4-byte Reload
	adcl	1104(%esp), %eax
	movl	%eax, 16(%esp)                  # 4-byte Spill
	adcl	1108(%esp), %edi
	movl	%edi, 40(%esp)                  # 4-byte Spill
	movl	32(%esp), %eax                  # 4-byte Reload
	adcl	1112(%esp), %eax
	movl	%eax, 32(%esp)                  # 4-byte Spill
	setb	%al
	subl	$4, %esp
	movl	76(%esp), %ecx                  # 4-byte Reload
	imull	%edx, %ecx
	movl	%edx, %esi
	movzbl	%al, %eax
	movl	%eax, 64(%esp)                  # 4-byte Spill
	pushl	%ecx
	pushl	2452(%esp)
	leal	988(%esp), %eax
	pushl	%eax
	calll	mulPv512x32@PLT
	addl	$12, %esp
	addl	976(%esp), %esi
	movl	4(%esp), %eax                   # 4-byte Reload
	adcl	980(%esp), %eax
	movl	%eax, 4(%esp)                   # 4-byte Spill
	movl	20(%esp), %edi                  # 4-byte Reload
	adcl	984(%esp), %edi
	adcl	988(%esp), %ebp
	movl	%ebp, 64(%esp)                  # 4-byte Spill
	movl	68(%esp), %eax                  # 4-byte Reload
	adcl	992(%esp), %eax
	movl	%eax, 68(%esp)                  # 4-byte Spill
	movl	12(%esp), %ebp                  # 4-byte Reload
	adcl	996(%esp), %ebp
	movl	56(%esp), %eax                  # 4-byte Reload
	adcl	1000(%esp), %eax
	movl	%eax, 56(%esp)                  # 4-byte Spill
	movl	52(%esp), %eax                  # 4-byte Reload
	adcl	1004(%esp), %eax
	movl	%eax, 52(%esp)                  # 4-byte Spill
	movl	48(%esp), %eax                  # 4-byte Reload
	adcl	1008(%esp), %eax
	movl	%eax, 48(%esp)                  # 4-byte Spill
	movl	36(%esp), %eax                  # 4-byte Reload
	adcl	1012(%esp), %eax
	movl	%eax, 36(%esp)                  # 4-byte Spill
	movl	44(%esp), %eax                  # 4-byte Reload
	adcl	1016(%esp), %eax
	movl	%eax, 44(%esp)                  # 4-byte Spill
	movl	8(%esp), %eax                   # 4-byte Reload
	adcl	1020(%esp), %eax
	movl	%eax, 8(%esp)                   # 4-byte Spill
	movl	28(%esp), %esi                  # 4-byte Reload
	adcl	1024(%esp), %esi
	movl	24(%esp), %eax                  # 4-byte Reload
	adcl	1028(%esp), %eax
	movl	%eax, 24(%esp)                  # 4-byte Spill
	movl	16(%esp), %eax                  # 4-byte Reload
	adcl	1032(%esp), %eax
	movl	%eax, 16(%esp)                  # 4-byte Spill
	movl	40(%esp), %eax                  # 4-byte Reload
	adcl	1036(%esp), %eax
	movl	%eax, 40(%esp)                  # 4-byte Spill
	movl	32(%esp), %eax                  # 4-byte Reload
	adcl	1040(%esp), %eax
	movl	%eax, 32(%esp)                  # 4-byte Spill
	adcl	$0, 60(%esp)                    # 4-byte Folded Spill
	subl	$4, %esp
	movl	2444(%esp), %eax
	pushl	40(%eax)
	movl	2444(%esp), %eax
	pushl	%eax
	leal	916(%esp), %eax
	pushl	%eax
	calll	mulPv512x32@PLT
	addl	$12, %esp
	movl	4(%esp), %edx                   # 4-byte Reload
	addl	904(%esp), %edx
	adcl	908(%esp), %edi
	movl	%edi, 20(%esp)                  # 4-byte Spill
	movl	64(%esp), %eax                  # 4-byte Reload
	adcl	912(%esp), %eax
	movl	%eax, 64(%esp)                  # 4-byte Spill
	movl	68(%esp), %eax                  # 4-byte Reload
	adcl	916(%esp), %eax
	movl	%eax, 68(%esp)                  # 4-byte Spill
	adcl	920(%esp), %ebp
	movl	%ebp, 12(%esp)                  # 4-byte Spill
	movl	56(%esp), %eax                  # 4-byte Reload
	adcl	924(%esp), %eax
	movl	%eax, 56(%esp)                  # 4-byte Spill
	movl	52(%esp), %eax                  # 4-byte Reload
	adcl	928(%esp), %eax
	movl	%eax, 52(%esp)                  # 4-byte Spill
	movl	48(%esp), %eax                  # 4-byte Reload
	adcl	932(%esp), %eax
	movl	%eax, 48(%esp)                  # 4-byte Spill
	movl	36(%esp), %edi                  # 4-byte Reload
	adcl	936(%esp), %edi
	movl	44(%esp), %ebp                  # 4-byte Reload
	adcl	940(%esp), %ebp
	movl	8(%esp), %eax                   # 4-byte Reload
	adcl	944(%esp), %eax
	movl	%eax, 8(%esp)                   # 4-byte Spill
	adcl	948(%esp), %esi
	movl	%esi, 28(%esp)                  # 4-byte Spill
	movl	24(%esp), %eax                  # 4-byte Reload
	adcl	952(%esp), %eax
	movl	%eax, 24(%esp)                  # 4-byte Spill
	movl	16(%esp), %eax                  # 4-byte Reload
	adcl	956(%esp), %eax
	movl	%eax, 16(%esp)                  # 4-byte Spill
	movl	40(%esp), %eax                  # 4-byte Reload
	adcl	960(%esp), %eax
	movl	%eax, 40(%esp)                  # 4-byte Spill
	movl	32(%esp), %eax                  # 4-byte Reload
	adcl	964(%esp), %eax
	movl	%eax, 32(%esp)                  # 4-byte Spill
	movl	60(%esp), %eax                  # 4-byte Reload
	adcl	968(%esp), %eax
	movl	%eax, 60(%esp)                  # 4-byte Spill
	setb	%al
	subl	$4, %esp
	movl	76(%esp), %ecx                  # 4-byte Reload
	movl	%edx, %esi
	imull	%edx, %ecx
	movzbl	%al, %eax
	movl	%eax, 8(%esp)                   # 4-byte Spill
	pushl	%ecx
	pushl	2452(%esp)
	leal	844(%esp), %eax
	pushl	%eax
	calll	mulPv512x32@PLT
	addl	$12, %esp
	addl	832(%esp), %esi
	movl	20(%esp), %eax                  # 4-byte Reload
	adcl	836(%esp), %eax
	movl	%eax, 20(%esp)                  # 4-byte Spill
	movl	64(%esp), %eax                  # 4-byte Reload
	adcl	840(%esp), %eax
	movl	%eax, 64(%esp)                  # 4-byte Spill
	movl	68(%esp), %eax                  # 4-byte Reload
	adcl	844(%esp), %eax
	movl	%eax, 68(%esp)                  # 4-byte Spill
	movl	12(%esp), %eax                  # 4-byte Reload
	adcl	848(%esp), %eax
	movl	%eax, 12(%esp)                  # 4-byte Spill
	movl	56(%esp), %esi                  # 4-byte Reload
	adcl	852(%esp), %esi
	movl	52(%esp), %eax                  # 4-byte Reload
	adcl	856(%esp), %eax
	movl	%eax, 52(%esp)                  # 4-byte Spill
	movl	48(%esp), %eax                  # 4-byte Reload
	adcl	860(%esp), %eax
	movl	%eax, 48(%esp)                  # 4-byte Spill
	adcl	864(%esp), %edi
	movl	%edi, 36(%esp)                  # 4-byte Spill
	adcl	868(%esp), %ebp
	movl	%ebp, 44(%esp)                  # 4-byte Spill
	movl	8(%esp), %eax                   # 4-byte Reload
	adcl	872(%esp), %eax
	movl	%eax, 8(%esp)                   # 4-byte Spill
	movl	28(%esp), %eax                  # 4-byte Reload
	adcl	876(%esp), %eax
	movl	%eax, 28(%esp)                  # 4-byte Spill
	movl	24(%esp), %eax                  # 4-byte Reload
	adcl	880(%esp), %eax
	movl	%eax, 24(%esp)                  # 4-byte Spill
	movl	16(%esp), %edi                  # 4-byte Reload
	adcl	884(%esp), %edi
	movl	40(%esp), %eax                  # 4-byte Reload
	adcl	888(%esp), %eax
	movl	%eax, 40(%esp)                  # 4-byte Spill
	movl	32(%esp), %ebp                  # 4-byte Reload
	adcl	892(%esp), %ebp
	movl	60(%esp), %eax                  # 4-byte Reload
	adcl	896(%esp), %eax
	movl	%eax, 60(%esp)                  # 4-byte Spill
	adcl	$0, 4(%esp)                     # 4-byte Folded Spill
	subl	$4, %esp
	movl	2444(%esp), %eax
	pushl	44(%eax)
	pushl	2444(%esp)
	leal	772(%esp), %eax
	pushl	%eax
	calll	mulPv512x32@PLT
	addl	$12, %esp
	movl	20(%esp), %edx                  # 4-byte Reload
	addl	760(%esp), %edx
	movl	64(%esp), %eax                  # 4-byte Reload
	adcl	764(%esp), %eax
	movl	%eax, 64(%esp)                  # 4-byte Spill
	movl	68(%esp), %eax                  # 4-byte Reload
	adcl	768(%esp), %eax
	movl	%eax, 68(%esp)                  # 4-byte Spill
	movl	12(%esp), %eax                  # 4-byte Reload
	adcl	772(%esp), %eax
	movl	%eax, 12(%esp)                  # 4-byte Spill
	adcl	776(%esp), %esi
	movl	%esi, 56(%esp)                  # 4-byte Spill
	movl	52(%esp), %eax                  # 4-byte Reload
	adcl	780(%esp), %eax
	movl	%eax, 52(%esp)                  # 4-byte Spill
	movl	48(%esp), %eax                  # 4-byte Reload
	adcl	784(%esp), %eax
	movl	%eax, 48(%esp)                  # 4-byte Spill
	movl	36(%esp), %eax                  # 4-byte Reload
	adcl	788(%esp), %eax
	movl	%eax, 36(%esp)                  # 4-byte Spill
	movl	44(%esp), %eax                  # 4-byte Reload
	adcl	792(%esp), %eax
	movl	%eax, 44(%esp)                  # 4-byte Spill
	movl	8(%esp), %eax                   # 4-byte Reload
	adcl	796(%esp), %eax
	movl	%eax, 8(%esp)                   # 4-byte Spill
	movl	28(%esp), %eax                  # 4-byte Reload
	adcl	800(%esp), %eax
	movl	%eax, 28(%esp)                  # 4-byte Spill
	movl	24(%esp), %eax                  # 4-byte Reload
	adcl	804(%esp), %eax
	movl	%eax, 24(%esp)                  # 4-byte Spill
	adcl	808(%esp), %edi
	movl	%edi, 16(%esp)                  # 4-byte Spill
	movl	40(%esp), %eax                  # 4-byte Reload
	adcl	812(%esp), %eax
	movl	%eax, 40(%esp)                  # 4-byte Spill
	adcl	816(%esp), %ebp
	movl	%ebp, 32(%esp)                  # 4-byte Spill
	movl	60(%esp), %edi                  # 4-byte Reload
	adcl	820(%esp), %edi
	movl	4(%esp), %eax                   # 4-byte Reload
	adcl	824(%esp), %eax
	movl	%eax, 4(%esp)                   # 4-byte Spill
	setb	%al
	subl	$4, %esp
	movl	76(%esp), %ecx                  # 4-byte Reload
	movl	%edx, %esi
	imull	%edx, %ecx
	movzbl	%al, %eax
	movl	%eax, 24(%esp)                  # 4-byte Spill
	pushl	%ecx
	pushl	2452(%esp)
	leal	700(%esp), %eax
	pushl	%eax
	calll	mulPv512x32@PLT
	addl	$12, %esp
	addl	688(%esp), %esi
	movl	64(%esp), %eax                  # 4-byte Reload
	adcl	692(%esp), %eax
	movl	%eax, 64(%esp)                  # 4-byte Spill
	movl	68(%esp), %eax                  # 4-byte Reload
	adcl	696(%esp), %eax
	movl	%eax, 68(%esp)                  # 4-byte Spill
	movl	12(%esp), %eax                  # 4-byte Reload
	adcl	700(%esp), %eax
	movl	%eax, 12(%esp)                  # 4-byte Spill
	movl	56(%esp), %eax                  # 4-byte Reload
	adcl	704(%esp), %eax
	movl	%eax, 56(%esp)                  # 4-byte Spill
	movl	52(%esp), %eax                  # 4-byte Reload
	adcl	708(%esp), %eax
	movl	%eax, 52(%esp)                  # 4-byte Spill
	movl	48(%esp), %eax                  # 4-byte Reload
	adcl	712(%esp), %eax
	movl	%eax, 48(%esp)                  # 4-byte Spill
	movl	36(%esp), %eax                  # 4-byte Reload
	adcl	716(%esp), %eax
	movl	%eax, 36(%esp)                  # 4-byte Spill
	movl	44(%esp), %eax                  # 4-byte Reload
	adcl	720(%esp), %eax
	movl	%eax, 44(%esp)                  # 4-byte Spill
	movl	8(%esp), %ebp                   # 4-byte Reload
	adcl	724(%esp), %ebp
	movl	28(%esp), %eax                  # 4-byte Reload
	adcl	728(%esp), %eax
	movl	%eax, 28(%esp)                  # 4-byte Spill
	movl	24(%esp), %esi                  # 4-byte Reload
	adcl	732(%esp), %esi
	movl	16(%esp), %eax                  # 4-byte Reload
	adcl	736(%esp), %eax
	movl	%eax, 16(%esp)                  # 4-byte Spill
	movl	40(%esp), %eax                  # 4-byte Reload
	adcl	740(%esp), %eax
	movl	%eax, 40(%esp)                  # 4-byte Spill
	movl	32(%esp), %eax                  # 4-byte Reload
	adcl	744(%esp), %eax
	movl	%eax, 32(%esp)                  # 4-byte Spill
	adcl	748(%esp), %edi
	movl	%edi, 60(%esp)                  # 4-byte Spill
	movl	4(%esp), %eax                   # 4-byte Reload
	adcl	752(%esp), %eax
	movl	%eax, 4(%esp)                   # 4-byte Spill
	adcl	$0, 20(%esp)                    # 4-byte Folded Spill
	subl	$4, %esp
	movl	2444(%esp), %eax
	pushl	48(%eax)
	pushl	2444(%esp)
	leal	628(%esp), %eax
	pushl	%eax
	calll	mulPv512x32@PLT
	addl	$12, %esp
	movl	64(%esp), %edx                  # 4-byte Reload
	addl	616(%esp), %edx
	movl	68(%esp), %edi                  # 4-byte Reload
	adcl	620(%esp), %edi
	movl	12(%esp), %eax                  # 4-byte Reload
	adcl	624(%esp), %eax
	movl	%eax, 12(%esp)                  # 4-byte Spill
	movl	56(%esp), %eax                  # 4-byte Reload
	adcl	628(%esp), %eax
	movl	%eax, 56(%esp)                  # 4-byte Spill
	movl	52(%esp), %eax                  # 4-byte Reload
	adcl	632(%esp), %eax
	movl	%eax, 52(%esp)                  # 4-byte Spill
	movl	48(%esp), %eax                  # 4-byte Reload
	adcl	636(%esp), %eax
	movl	%eax, 48(%esp)                  # 4-byte Spill
	movl	36(%esp), %eax                  # 4-byte Reload
	adcl	640(%esp), %eax
	movl	%eax, 36(%esp)                  # 4-byte Spill
	movl	44(%esp), %eax                  # 4-byte Reload
	adcl	644(%esp), %eax
	movl	%eax, 44(%esp)                  # 4-byte Spill
	adcl	648(%esp), %ebp
	movl	%ebp, 8(%esp)                   # 4-byte Spill
	movl	28(%esp), %eax                  # 4-byte Reload
	adcl	652(%esp), %eax
	movl	%eax, 28(%esp)                  # 4-byte Spill
	adcl	656(%esp), %esi
	movl	%esi, 24(%esp)                  # 4-byte Spill
	movl	16(%esp), %eax                  # 4-byte Reload
	adcl	660(%esp), %eax
	movl	%eax, 16(%esp)                  # 4-byte Spill
	movl	40(%esp), %eax                  # 4-byte Reload
	adcl	664(%esp), %eax
	movl	%eax, 40(%esp)                  # 4-byte Spill
	movl	32(%esp), %eax                  # 4-byte Reload
	adcl	668(%esp), %eax
	movl	%eax, 32(%esp)                  # 4-byte Spill
	movl	60(%esp), %eax                  # 4-byte Reload
	adcl	672(%esp), %eax
	movl	%eax, 60(%esp)                  # 4-byte Spill
	movl	4(%esp), %eax                   # 4-byte Reload
	adcl	676(%esp), %eax
	movl	%eax, 4(%esp)                   # 4-byte Spill
	movl	20(%esp), %eax                  # 4-byte Reload
	adcl	680(%esp), %eax
	movl	%eax, 20(%esp)                  # 4-byte Spill
	setb	%al
	subl	$4, %esp
	movl	76(%esp), %ecx                  # 4-byte Reload
	imull	%edx, %ecx
	movl	%edx, %esi
	movzbl	%al, %eax
	movl	%eax, 68(%esp)                  # 4-byte Spill
	pushl	%ecx
	pushl	2452(%esp)
	leal	556(%esp), %eax
	pushl	%eax
	calll	mulPv512x32@PLT
	addl	$12, %esp
	addl	544(%esp), %esi
	adcl	548(%esp), %edi
	movl	%edi, 68(%esp)                  # 4-byte Spill
	movl	12(%esp), %eax                  # 4-byte Reload
	adcl	552(%esp), %eax
	movl	%eax, 12(%esp)                  # 4-byte Spill
	movl	56(%esp), %esi                  # 4-byte Reload
	adcl	556(%esp), %esi
	movl	52(%esp), %eax                  # 4-byte Reload
	adcl	560(%esp), %eax
	movl	%eax, 52(%esp)                  # 4-byte Spill
	movl	48(%esp), %ebp                  # 4-byte Reload
	adcl	564(%esp), %ebp
	movl	36(%esp), %eax                  # 4-byte Reload
	adcl	568(%esp), %eax
	movl	%eax, 36(%esp)                  # 4-byte Spill
	movl	44(%esp), %eax                  # 4-byte Reload
	adcl	572(%esp), %eax
	movl	%eax, 44(%esp)                  # 4-byte Spill
	movl	8(%esp), %eax                   # 4-byte Reload
	adcl	576(%esp), %eax
	movl	%eax, 8(%esp)                   # 4-byte Spill
	movl	28(%esp), %edi                  # 4-byte Reload
	adcl	580(%esp), %edi
	movl	24(%esp), %eax                  # 4-byte Reload
	adcl	584(%esp), %eax
	movl	%eax, 24(%esp)                  # 4-byte Spill
	movl	16(%esp), %eax                  # 4-byte Reload
	adcl	588(%esp), %eax
	movl	%eax, 16(%esp)                  # 4-byte Spill
	movl	40(%esp), %eax                  # 4-byte Reload
	adcl	592(%esp), %eax
	movl	%eax, 40(%esp)                  # 4-byte Spill
	movl	32(%esp), %eax                  # 4-byte Reload
	adcl	596(%esp), %eax
	movl	%eax, 32(%esp)                  # 4-byte Spill
	movl	60(%esp), %eax                  # 4-byte Reload
	adcl	600(%esp), %eax
	movl	%eax, 60(%esp)                  # 4-byte Spill
	movl	4(%esp), %eax                   # 4-byte Reload
	adcl	604(%esp), %eax
	movl	%eax, 4(%esp)                   # 4-byte Spill
	movl	20(%esp), %eax                  # 4-byte Reload
	adcl	608(%esp), %eax
	movl	%eax, 20(%esp)                  # 4-byte Spill
	adcl	$0, 64(%esp)                    # 4-byte Folded Spill
	subl	$4, %esp
	movl	2444(%esp), %eax
	pushl	52(%eax)
	pushl	2444(%esp)
	leal	484(%esp), %eax
	pushl	%eax
	calll	mulPv512x32@PLT
	addl	$12, %esp
	movl	68(%esp), %edx                  # 4-byte Reload
	addl	472(%esp), %edx
	movl	12(%esp), %eax                  # 4-byte Reload
	adcl	476(%esp), %eax
	movl	%eax, 12(%esp)                  # 4-byte Spill
	adcl	480(%esp), %esi
	movl	%esi, 56(%esp)                  # 4-byte Spill
	movl	52(%esp), %eax                  # 4-byte Reload
	adcl	484(%esp), %eax
	movl	%eax, 52(%esp)                  # 4-byte Spill
	adcl	488(%esp), %ebp
	movl	%ebp, 48(%esp)                  # 4-byte Spill
	movl	36(%esp), %eax                  # 4-byte Reload
	adcl	492(%esp), %eax
	movl	%eax, 36(%esp)                  # 4-byte Spill
	movl	44(%esp), %ebp                  # 4-byte Reload
	adcl	496(%esp), %ebp
	movl	8(%esp), %eax                   # 4-byte Reload
	adcl	500(%esp), %eax
	movl	%eax, 8(%esp)                   # 4-byte Spill
	adcl	504(%esp), %edi
	movl	%edi, 28(%esp)                  # 4-byte Spill
	movl	24(%esp), %eax                  # 4-byte Reload
	adcl	508(%esp), %eax
	movl	%eax, 24(%esp)                  # 4-byte Spill
	movl	16(%esp), %eax                  # 4-byte Reload
	adcl	512(%esp), %eax
	movl	%eax, 16(%esp)                  # 4-byte Spill
	movl	40(%esp), %eax                  # 4-byte Reload
	adcl	516(%esp), %eax
	movl	%eax, 40(%esp)                  # 4-byte Spill
	movl	32(%esp), %eax                  # 4-byte Reload
	adcl	520(%esp), %eax
	movl	%eax, 32(%esp)                  # 4-byte Spill
	movl	60(%esp), %eax                  # 4-byte Reload
	adcl	524(%esp), %eax
	movl	%eax, 60(%esp)                  # 4-byte Spill
	movl	4(%esp), %eax                   # 4-byte Reload
	adcl	528(%esp), %eax
	movl	%eax, 4(%esp)                   # 4-byte Spill
	movl	20(%esp), %eax                  # 4-byte Reload
	adcl	532(%esp), %eax
	movl	%eax, 20(%esp)                  # 4-byte Spill
	movl	64(%esp), %eax                  # 4-byte Reload
	adcl	536(%esp), %eax
	movl	%eax, 64(%esp)                  # 4-byte Spill
	setb	%al
	subl	$4, %esp
	movl	76(%esp), %ecx                  # 4-byte Reload
	imull	%edx, %ecx
	movl	%edx, %esi
	movzbl	%al, %eax
	movl	%eax, 72(%esp)                  # 4-byte Spill
	pushl	%ecx
	pushl	2452(%esp)
	leal	412(%esp), %eax
	pushl	%eax
	calll	mulPv512x32@PLT
	addl	$12, %esp
	addl	400(%esp), %esi
	movl	12(%esp), %eax                  # 4-byte Reload
	adcl	404(%esp), %eax
	movl	%eax, 12(%esp)                  # 4-byte Spill
	movl	56(%esp), %esi                  # 4-byte Reload
	adcl	408(%esp), %esi
	movl	52(%esp), %edi                  # 4-byte Reload
	adcl	412(%esp), %edi
	movl	48(%esp), %eax                  # 4-byte Reload
	adcl	416(%esp), %eax
	movl	%eax, 48(%esp)                  # 4-byte Spill
	movl	36(%esp), %eax                  # 4-byte Reload
	adcl	420(%esp), %eax
	movl	%eax, 36(%esp)                  # 4-byte Spill
	adcl	424(%esp), %ebp
	movl	%ebp, 44(%esp)                  # 4-byte Spill
	movl	8(%esp), %eax                   # 4-byte Reload
	adcl	428(%esp), %eax
	movl	%eax, 8(%esp)                   # 4-byte Spill
	movl	28(%esp), %eax                  # 4-byte Reload
	adcl	432(%esp), %eax
	movl	%eax, 28(%esp)                  # 4-byte Spill
	movl	24(%esp), %eax                  # 4-byte Reload
	adcl	436(%esp), %eax
	movl	%eax, 24(%esp)                  # 4-byte Spill
	movl	16(%esp), %eax                  # 4-byte Reload
	adcl	440(%esp), %eax
	movl	%eax, 16(%esp)                  # 4-byte Spill
	movl	40(%esp), %ebp                  # 4-byte Reload
	adcl	444(%esp), %ebp
	movl	32(%esp), %eax                  # 4-byte Reload
	adcl	448(%esp), %eax
	movl	%eax, 32(%esp)                  # 4-byte Spill
	movl	60(%esp), %eax                  # 4-byte Reload
	adcl	452(%esp), %eax
	movl	%eax, 60(%esp)                  # 4-byte Spill
	movl	4(%esp), %eax                   # 4-byte Reload
	adcl	456(%esp), %eax
	movl	%eax, 4(%esp)                   # 4-byte Spill
	movl	20(%esp), %eax                  # 4-byte Reload
	adcl	460(%esp), %eax
	movl	%eax, 20(%esp)                  # 4-byte Spill
	movl	64(%esp), %eax                  # 4-byte Reload
	adcl	464(%esp), %eax
	movl	%eax, 64(%esp)                  # 4-byte Spill
	adcl	$0, 68(%esp)                    # 4-byte Folded Spill
	subl	$4, %esp
	movl	2444(%esp), %eax
	pushl	56(%eax)
	pushl	2444(%esp)
	leal	340(%esp), %eax
	pushl	%eax
	calll	mulPv512x32@PLT
	addl	$12, %esp
	movl	12(%esp), %edx                  # 4-byte Reload
	addl	328(%esp), %edx
	adcl	332(%esp), %esi
	movl	%esi, 56(%esp)                  # 4-byte Spill
	adcl	336(%esp), %edi
	movl	%edi, 52(%esp)                  # 4-byte Spill
	movl	48(%esp), %eax                  # 4-byte Reload
	adcl	340(%esp), %eax
	movl	%eax, 48(%esp)                  # 4-byte Spill
	movl	36(%esp), %eax                  # 4-byte Reload
	adcl	344(%esp), %eax
	movl	%eax, 36(%esp)                  # 4-byte Spill
	movl	44(%esp), %eax                  # 4-byte Reload
	adcl	348(%esp), %eax
	movl	%eax, 44(%esp)                  # 4-byte Spill
	movl	8(%esp), %eax                   # 4-byte Reload
	adcl	352(%esp), %eax
	movl	%eax, 8(%esp)                   # 4-byte Spill
	movl	28(%esp), %eax                  # 4-byte Reload
	adcl	356(%esp), %eax
	movl	%eax, 28(%esp)                  # 4-byte Spill
	movl	24(%esp), %eax                  # 4-byte Reload
	adcl	360(%esp), %eax
	movl	%eax, 24(%esp)                  # 4-byte Spill
	movl	16(%esp), %eax                  # 4-byte Reload
	adcl	364(%esp), %eax
	movl	%eax, 16(%esp)                  # 4-byte Spill
	movl	%ebp, %edi
	adcl	368(%esp), %edi
	movl	32(%esp), %eax                  # 4-byte Reload
	adcl	372(%esp), %eax
	movl	%eax, 32(%esp)                  # 4-byte Spill
	movl	60(%esp), %eax                  # 4-byte Reload
	adcl	376(%esp), %eax
	movl	%eax, 60(%esp)                  # 4-byte Spill
	movl	4(%esp), %eax                   # 4-byte Reload
	adcl	380(%esp), %eax
	movl	%eax, 4(%esp)                   # 4-byte Spill
	movl	20(%esp), %eax                  # 4-byte Reload
	adcl	384(%esp), %eax
	movl	%eax, 20(%esp)                  # 4-byte Spill
	movl	64(%esp), %eax                  # 4-byte Reload
	adcl	388(%esp), %eax
	movl	%eax, 64(%esp)                  # 4-byte Spill
	movl	68(%esp), %eax                  # 4-byte Reload
	adcl	392(%esp), %eax
	movl	%eax, 68(%esp)                  # 4-byte Spill
	setb	12(%esp)                        # 1-byte Folded Spill
	subl	$4, %esp
	leal	260(%esp), %eax
	movl	76(%esp), %ecx                  # 4-byte Reload
	imull	%edx, %ecx
	movl	%edx, %esi
	pushl	%ecx
	pushl	2452(%esp)
	pushl	%eax
	calll	mulPv512x32@PLT
	addl	$12, %esp
	addl	256(%esp), %esi
	movl	56(%esp), %eax                  # 4-byte Reload
	adcl	260(%esp), %eax
	movl	%eax, 56(%esp)                  # 4-byte Spill
	movl	52(%esp), %eax                  # 4-byte Reload
	adcl	264(%esp), %eax
	movl	%eax, 52(%esp)                  # 4-byte Spill
	movl	48(%esp), %eax                  # 4-byte Reload
	adcl	268(%esp), %eax
	movl	%eax, 48(%esp)                  # 4-byte Spill
	movl	36(%esp), %esi                  # 4-byte Reload
	adcl	272(%esp), %esi
	movl	44(%esp), %eax                  # 4-byte Reload
	adcl	276(%esp), %eax
	movl	%eax, 44(%esp)                  # 4-byte Spill
	movl	8(%esp), %eax                   # 4-byte Reload
	adcl	280(%esp), %eax
	movl	%eax, 8(%esp)                   # 4-byte Spill
	movl	28(%esp), %eax                  # 4-byte Reload
	adcl	284(%esp), %eax
	movl	%eax, 28(%esp)                  # 4-byte Spill
	movl	24(%esp), %eax                  # 4-byte Reload
	adcl	288(%esp), %eax
	movl	%eax, 24(%esp)                  # 4-byte Spill
	movl	16(%esp), %ebp                  # 4-byte Reload
	adcl	292(%esp), %ebp
	adcl	296(%esp), %edi
	movl	%edi, 40(%esp)                  # 4-byte Spill
	movl	32(%esp), %eax                  # 4-byte Reload
	adcl	300(%esp), %eax
	movl	%eax, 32(%esp)                  # 4-byte Spill
	movl	60(%esp), %eax                  # 4-byte Reload
	adcl	304(%esp), %eax
	movl	%eax, 60(%esp)                  # 4-byte Spill
	movl	4(%esp), %edi                   # 4-byte Reload
	adcl	308(%esp), %edi
	movl	20(%esp), %eax                  # 4-byte Reload
	adcl	312(%esp), %eax
	movl	%eax, 20(%esp)                  # 4-byte Spill
	movl	64(%esp), %eax                  # 4-byte Reload
	adcl	316(%esp), %eax
	movl	%eax, 64(%esp)                  # 4-byte Spill
	movl	68(%esp), %eax                  # 4-byte Reload
	adcl	320(%esp), %eax
	movl	%eax, 68(%esp)                  # 4-byte Spill
	movzbl	12(%esp), %eax                  # 1-byte Folded Reload
	adcl	$0, %eax
	movl	%eax, 12(%esp)                  # 4-byte Spill
	subl	$4, %esp
	leal	188(%esp), %eax
	movl	2444(%esp), %ecx
	pushl	60(%ecx)
	pushl	2444(%esp)
	pushl	%eax
	calll	mulPv512x32@PLT
	addl	$12, %esp
	movl	56(%esp), %edx                  # 4-byte Reload
	addl	184(%esp), %edx
	movl	52(%esp), %eax                  # 4-byte Reload
	adcl	188(%esp), %eax
	movl	%eax, 52(%esp)                  # 4-byte Spill
	movl	48(%esp), %eax                  # 4-byte Reload
	adcl	192(%esp), %eax
	movl	%eax, 48(%esp)                  # 4-byte Spill
	adcl	196(%esp), %esi
	movl	%esi, 36(%esp)                  # 4-byte Spill
	movl	44(%esp), %eax                  # 4-byte Reload
	adcl	200(%esp), %eax
	movl	%eax, 44(%esp)                  # 4-byte Spill
	movl	8(%esp), %eax                   # 4-byte Reload
	adcl	204(%esp), %eax
	movl	%eax, 8(%esp)                   # 4-byte Spill
	movl	28(%esp), %eax                  # 4-byte Reload
	adcl	208(%esp), %eax
	movl	%eax, 28(%esp)                  # 4-byte Spill
	movl	24(%esp), %eax                  # 4-byte Reload
	adcl	212(%esp), %eax
	movl	%eax, 24(%esp)                  # 4-byte Spill
	adcl	216(%esp), %ebp
	movl	%ebp, 16(%esp)                  # 4-byte Spill
	movl	40(%esp), %eax                  # 4-byte Reload
	adcl	220(%esp), %eax
	movl	%eax, 40(%esp)                  # 4-byte Spill
	movl	32(%esp), %eax                  # 4-byte Reload
	adcl	224(%esp), %eax
	movl	%eax, 32(%esp)                  # 4-byte Spill
	movl	60(%esp), %eax                  # 4-byte Reload
	adcl	228(%esp), %eax
	movl	%eax, 60(%esp)                  # 4-byte Spill
	adcl	232(%esp), %edi
	movl	%edi, 4(%esp)                   # 4-byte Spill
	movl	20(%esp), %eax                  # 4-byte Reload
	adcl	236(%esp), %eax
	movl	%eax, 20(%esp)                  # 4-byte Spill
	movl	64(%esp), %eax                  # 4-byte Reload
	adcl	240(%esp), %eax
	movl	%eax, 64(%esp)                  # 4-byte Spill
	movl	68(%esp), %eax                  # 4-byte Reload
	adcl	244(%esp), %eax
	movl	%eax, 68(%esp)                  # 4-byte Spill
	movl	12(%esp), %eax                  # 4-byte Reload
	adcl	248(%esp), %eax
	movl	%eax, 12(%esp)                  # 4-byte Spill
	setb	56(%esp)                        # 1-byte Folded Spill
	subl	$4, %esp
	leal	116(%esp), %eax
	movl	76(%esp), %ecx                  # 4-byte Reload
	imull	%edx, %ecx
	movl	%edx, %esi
	pushl	%ecx
	pushl	2452(%esp)
	pushl	%eax
	calll	mulPv512x32@PLT
	addl	$12, %esp
	addl	112(%esp), %esi
	movzbl	56(%esp), %ecx                  # 1-byte Folded Reload
	movl	52(%esp), %eax                  # 4-byte Reload
	adcl	116(%esp), %eax
	movl	48(%esp), %esi                  # 4-byte Reload
	adcl	120(%esp), %esi
	movl	36(%esp), %ebx                  # 4-byte Reload
	adcl	124(%esp), %ebx
	movl	44(%esp), %ebp                  # 4-byte Reload
	adcl	128(%esp), %ebp
	movl	8(%esp), %edx                   # 4-byte Reload
	adcl	132(%esp), %edx
	movl	%edx, 8(%esp)                   # 4-byte Spill
	movl	28(%esp), %edx                  # 4-byte Reload
	adcl	136(%esp), %edx
	movl	%edx, 28(%esp)                  # 4-byte Spill
	movl	24(%esp), %edx                  # 4-byte Reload
	adcl	140(%esp), %edx
	movl	%edx, 24(%esp)                  # 4-byte Spill
	movl	16(%esp), %edx                  # 4-byte Reload
	adcl	144(%esp), %edx
	movl	%edx, 16(%esp)                  # 4-byte Spill
	movl	40(%esp), %edx                  # 4-byte Reload
	adcl	148(%esp), %edx
	movl	%edx, 40(%esp)                  # 4-byte Spill
	movl	32(%esp), %edi                  # 4-byte Reload
	adcl	152(%esp), %edi
	movl	60(%esp), %edx                  # 4-byte Reload
	adcl	156(%esp), %edx
	movl	%edx, 60(%esp)                  # 4-byte Spill
	movl	4(%esp), %edx                   # 4-byte Reload
	adcl	160(%esp), %edx
	movl	%edx, 4(%esp)                   # 4-byte Spill
	movl	20(%esp), %edx                  # 4-byte Reload
	adcl	164(%esp), %edx
	movl	%edx, 20(%esp)                  # 4-byte Spill
	movl	64(%esp), %edx                  # 4-byte Reload
	adcl	168(%esp), %edx
	movl	%edx, 64(%esp)                  # 4-byte Spill
	movl	68(%esp), %edx                  # 4-byte Reload
	adcl	172(%esp), %edx
	movl	%edx, 68(%esp)                  # 4-byte Spill
	movl	12(%esp), %edx                  # 4-byte Reload
	adcl	176(%esp), %edx
	movl	%edx, 12(%esp)                  # 4-byte Spill
	adcl	$0, %ecx
	movl	%eax, 52(%esp)                  # 4-byte Spill
	movl	2444(%esp), %edx
	subl	(%edx), %eax
	movl	%eax, 108(%esp)                 # 4-byte Spill
	movl	%esi, %eax
	movl	%esi, 48(%esp)                  # 4-byte Spill
	sbbl	4(%edx), %eax
	movl	%eax, 104(%esp)                 # 4-byte Spill
	movl	%ebx, %eax
	movl	%ebx, 36(%esp)                  # 4-byte Spill
	sbbl	8(%edx), %eax
	movl	%eax, 100(%esp)                 # 4-byte Spill
	movl	%ebp, %eax
	movl	%ebp, 44(%esp)                  # 4-byte Spill
	sbbl	12(%edx), %eax
	movl	%eax, 96(%esp)                  # 4-byte Spill
	movl	8(%esp), %eax                   # 4-byte Reload
	sbbl	16(%edx), %eax
	movl	%eax, 92(%esp)                  # 4-byte Spill
	movl	28(%esp), %eax                  # 4-byte Reload
	sbbl	20(%edx), %eax
	movl	%eax, 88(%esp)                  # 4-byte Spill
	movl	24(%esp), %eax                  # 4-byte Reload
	sbbl	24(%edx), %eax
	movl	%eax, 56(%esp)                  # 4-byte Spill
	movl	16(%esp), %eax                  # 4-byte Reload
	sbbl	28(%edx), %eax
	movl	%eax, 84(%esp)                  # 4-byte Spill
	movl	40(%esp), %eax                  # 4-byte Reload
	sbbl	32(%edx), %eax
	movl	%eax, 72(%esp)                  # 4-byte Spill
	movl	%edi, %eax
	movl	%edi, 32(%esp)                  # 4-byte Spill
	sbbl	36(%edx), %eax
	movl	%eax, 80(%esp)                  # 4-byte Spill
	movl	60(%esp), %eax                  # 4-byte Reload
	sbbl	40(%edx), %eax
	movl	%eax, 76(%esp)                  # 4-byte Spill
	movl	4(%esp), %esi                   # 4-byte Reload
	sbbl	44(%edx), %esi
	movl	20(%esp), %ebx                  # 4-byte Reload
	sbbl	48(%edx), %ebx
	movl	64(%esp), %eax                  # 4-byte Reload
	sbbl	52(%edx), %eax
	movl	%edx, %ebp
	movl	68(%esp), %edx                  # 4-byte Reload
	sbbl	56(%ebp), %edx
	movl	12(%esp), %ebp                  # 4-byte Reload
	movl	2444(%esp), %edi
	sbbl	60(%edi), %ebp
	sbbl	$0, %ecx
	testb	$1, %cl
	jne	.LBB77_1
# %bb.2:
	movl	2432(%esp), %ecx
	movl	%ebp, 60(%ecx)
	jne	.LBB77_3
.LBB77_4:
	movl	%edx, 56(%ecx)
	movl	88(%esp), %edx                  # 4-byte Reload
	jne	.LBB77_5
.LBB77_6:
	movl	%eax, 52(%ecx)
	jne	.LBB77_7
.LBB77_8:
	movl	%ebx, 48(%ecx)
	movl	84(%esp), %eax                  # 4-byte Reload
	jne	.LBB77_9
.LBB77_10:
	movl	%esi, 44(%ecx)
	movl	92(%esp), %ebx                  # 4-byte Reload
	movl	76(%esp), %esi                  # 4-byte Reload
	jne	.LBB77_11
.LBB77_12:
	movl	%esi, 40(%ecx)
	movl	100(%esp), %esi                 # 4-byte Reload
	movl	80(%esp), %edi                  # 4-byte Reload
	jne	.LBB77_13
.LBB77_14:
	movl	%edi, 36(%ecx)
	movl	96(%esp), %edi                  # 4-byte Reload
	jne	.LBB77_15
.LBB77_16:
	movl	72(%esp), %ebp                  # 4-byte Reload
	movl	%ebp, 32(%ecx)
	jne	.LBB77_17
.LBB77_18:
	movl	%eax, 28(%ecx)
	jne	.LBB77_19
.LBB77_20:
	movl	56(%esp), %eax                  # 4-byte Reload
	movl	%eax, 24(%ecx)
	jne	.LBB77_21
.LBB77_22:
	movl	%edx, 20(%ecx)
	movl	108(%esp), %edx                 # 4-byte Reload
	movl	104(%esp), %eax                 # 4-byte Reload
	jne	.LBB77_23
.LBB77_24:
	movl	%ebx, 16(%ecx)
	jne	.LBB77_25
.LBB77_26:
	movl	%edi, 12(%ecx)
	jne	.LBB77_27
.LBB77_28:
	movl	%esi, 8(%ecx)
	jne	.LBB77_29
.LBB77_30:
	movl	%eax, 4(%ecx)
	je	.LBB77_32
.LBB77_31:
	movl	52(%esp), %edx                  # 4-byte Reload
.LBB77_32:
	movl	%edx, (%ecx)
	addl	$2412, %esp                     # imm = 0x96C
	popl	%esi
	popl	%edi
	popl	%ebx
	popl	%ebp
	retl
.LBB77_1:
	movl	12(%esp), %ebp                  # 4-byte Reload
	movl	2432(%esp), %ecx
	movl	%ebp, 60(%ecx)
	je	.LBB77_4
.LBB77_3:
	movl	68(%esp), %edx                  # 4-byte Reload
	movl	%edx, 56(%ecx)
	movl	88(%esp), %edx                  # 4-byte Reload
	je	.LBB77_6
.LBB77_5:
	movl	64(%esp), %eax                  # 4-byte Reload
	movl	%eax, 52(%ecx)
	je	.LBB77_8
.LBB77_7:
	movl	20(%esp), %ebx                  # 4-byte Reload
	movl	%ebx, 48(%ecx)
	movl	84(%esp), %eax                  # 4-byte Reload
	je	.LBB77_10
.LBB77_9:
	movl	4(%esp), %esi                   # 4-byte Reload
	movl	%esi, 44(%ecx)
	movl	92(%esp), %ebx                  # 4-byte Reload
	movl	76(%esp), %esi                  # 4-byte Reload
	je	.LBB77_12
.LBB77_11:
	movl	60(%esp), %esi                  # 4-byte Reload
	movl	%esi, 40(%ecx)
	movl	100(%esp), %esi                 # 4-byte Reload
	movl	80(%esp), %edi                  # 4-byte Reload
	je	.LBB77_14
.LBB77_13:
	movl	32(%esp), %edi                  # 4-byte Reload
	movl	%edi, 36(%ecx)
	movl	96(%esp), %edi                  # 4-byte Reload
	je	.LBB77_16
.LBB77_15:
	movl	40(%esp), %ebp                  # 4-byte Reload
	movl	%ebp, 72(%esp)                  # 4-byte Spill
	movl	72(%esp), %ebp                  # 4-byte Reload
	movl	%ebp, 32(%ecx)
	je	.LBB77_18
.LBB77_17:
	movl	16(%esp), %eax                  # 4-byte Reload
	movl	%eax, 28(%ecx)
	je	.LBB77_20
.LBB77_19:
	movl	24(%esp), %eax                  # 4-byte Reload
	movl	%eax, 56(%esp)                  # 4-byte Spill
	movl	56(%esp), %eax                  # 4-byte Reload
	movl	%eax, 24(%ecx)
	je	.LBB77_22
.LBB77_21:
	movl	28(%esp), %edx                  # 4-byte Reload
	movl	%edx, 20(%ecx)
	movl	108(%esp), %edx                 # 4-byte Reload
	movl	104(%esp), %eax                 # 4-byte Reload
	je	.LBB77_24
.LBB77_23:
	movl	8(%esp), %ebx                   # 4-byte Reload
	movl	%ebx, 16(%ecx)
	je	.LBB77_26
.LBB77_25:
	movl	44(%esp), %edi                  # 4-byte Reload
	movl	%edi, 12(%ecx)
	je	.LBB77_28
.LBB77_27:
	movl	36(%esp), %esi                  # 4-byte Reload
	movl	%esi, 8(%ecx)
	je	.LBB77_30
.LBB77_29:
	movl	48(%esp), %eax                  # 4-byte Reload
	movl	%eax, 4(%ecx)
	jne	.LBB77_31
	jmp	.LBB77_32
.Lfunc_end77:
	.size	mcl_fp_mont16L, .Lfunc_end77-mcl_fp_mont16L
                                        # -- End function
	.globl	mcl_fp_montNF16L                # -- Begin function mcl_fp_montNF16L
	.p2align	4, 0x90
	.type	mcl_fp_montNF16L,@function
mcl_fp_montNF16L:                       # @mcl_fp_montNF16L
# %bb.0:
	pushl	%ebp
	pushl	%ebx
	pushl	%edi
	pushl	%esi
	subl	$2412, %esp                     # imm = 0x96C
	calll	.L78$pb
.L78$pb:
	popl	%ebx
.Ltmp20:
	addl	$_GLOBAL_OFFSET_TABLE_+(.Ltmp20-.L78$pb), %ebx
	movl	2444(%esp), %eax
	movl	-4(%eax), %esi
	movl	%esi, 68(%esp)                  # 4-byte Spill
	movl	2440(%esp), %ecx
	subl	$4, %esp
	leal	2348(%esp), %eax
	pushl	(%ecx)
	pushl	2444(%esp)
	pushl	%eax
	calll	mulPv512x32@PLT
	addl	$12, %esp
	movl	2344(%esp), %ebp
	movl	2348(%esp), %eax
	movl	%eax, 64(%esp)                  # 4-byte Spill
	movl	%esi, %eax
	imull	%ebp, %eax
	movl	2408(%esp), %ecx
	movl	%ecx, 44(%esp)                  # 4-byte Spill
	movl	2404(%esp), %ecx
	movl	%ecx, 40(%esp)                  # 4-byte Spill
	movl	2400(%esp), %ecx
	movl	%ecx, 52(%esp)                  # 4-byte Spill
	movl	2396(%esp), %ecx
	movl	%ecx, 20(%esp)                  # 4-byte Spill
	movl	2392(%esp), %ecx
	movl	%ecx, 36(%esp)                  # 4-byte Spill
	movl	2388(%esp), %ecx
	movl	%ecx, 48(%esp)                  # 4-byte Spill
	movl	2384(%esp), %ecx
	movl	%ecx, 4(%esp)                   # 4-byte Spill
	movl	2380(%esp), %ecx
	movl	%ecx, 12(%esp)                  # 4-byte Spill
	movl	2376(%esp), %ecx
	movl	%ecx, 8(%esp)                   # 4-byte Spill
	movl	2372(%esp), %ecx
	movl	%ecx, 16(%esp)                  # 4-byte Spill
	movl	2368(%esp), %edi
	movl	2364(%esp), %ecx
	movl	%ecx, 60(%esp)                  # 4-byte Spill
	movl	2360(%esp), %ecx
	movl	%ecx, 32(%esp)                  # 4-byte Spill
	movl	2356(%esp), %esi
	movl	2352(%esp), %ecx
	movl	%ecx, 24(%esp)                  # 4-byte Spill
	subl	$4, %esp
	leal	2276(%esp), %ecx
	pushl	%eax
	pushl	2452(%esp)
	pushl	%ecx
	calll	mulPv512x32@PLT
	addl	$12, %esp
	addl	2272(%esp), %ebp
	movl	64(%esp), %eax                  # 4-byte Reload
	adcl	2276(%esp), %eax
	movl	%eax, 64(%esp)                  # 4-byte Spill
	movl	24(%esp), %eax                  # 4-byte Reload
	adcl	2280(%esp), %eax
	movl	%eax, 24(%esp)                  # 4-byte Spill
	adcl	2284(%esp), %esi
	movl	%esi, 56(%esp)                  # 4-byte Spill
	movl	32(%esp), %eax                  # 4-byte Reload
	adcl	2288(%esp), %eax
	movl	%eax, 32(%esp)                  # 4-byte Spill
	movl	60(%esp), %eax                  # 4-byte Reload
	adcl	2292(%esp), %eax
	movl	%eax, 60(%esp)                  # 4-byte Spill
	adcl	2296(%esp), %edi
	movl	%edi, 28(%esp)                  # 4-byte Spill
	movl	16(%esp), %eax                  # 4-byte Reload
	adcl	2300(%esp), %eax
	movl	%eax, 16(%esp)                  # 4-byte Spill
	movl	8(%esp), %eax                   # 4-byte Reload
	adcl	2304(%esp), %eax
	movl	%eax, 8(%esp)                   # 4-byte Spill
	movl	12(%esp), %edi                  # 4-byte Reload
	adcl	2308(%esp), %edi
	movl	4(%esp), %eax                   # 4-byte Reload
	adcl	2312(%esp), %eax
	movl	%eax, 4(%esp)                   # 4-byte Spill
	movl	48(%esp), %esi                  # 4-byte Reload
	adcl	2316(%esp), %esi
	movl	36(%esp), %eax                  # 4-byte Reload
	adcl	2320(%esp), %eax
	movl	%eax, 36(%esp)                  # 4-byte Spill
	movl	20(%esp), %eax                  # 4-byte Reload
	adcl	2324(%esp), %eax
	movl	%eax, 20(%esp)                  # 4-byte Spill
	movl	52(%esp), %ebp                  # 4-byte Reload
	adcl	2328(%esp), %ebp
	movl	40(%esp), %eax                  # 4-byte Reload
	adcl	2332(%esp), %eax
	movl	%eax, 40(%esp)                  # 4-byte Spill
	movl	44(%esp), %eax                  # 4-byte Reload
	adcl	2336(%esp), %eax
	movl	%eax, 44(%esp)                  # 4-byte Spill
	subl	$4, %esp
	leal	2204(%esp), %eax
	movl	2444(%esp), %ecx
	pushl	4(%ecx)
	pushl	2444(%esp)
	pushl	%eax
	calll	mulPv512x32@PLT
	addl	$12, %esp
	movl	2264(%esp), %eax
	movl	64(%esp), %edx                  # 4-byte Reload
	addl	2200(%esp), %edx
	movl	24(%esp), %ecx                  # 4-byte Reload
	adcl	2204(%esp), %ecx
	movl	%ecx, 24(%esp)                  # 4-byte Spill
	movl	56(%esp), %ecx                  # 4-byte Reload
	adcl	2208(%esp), %ecx
	movl	%ecx, 56(%esp)                  # 4-byte Spill
	movl	32(%esp), %ecx                  # 4-byte Reload
	adcl	2212(%esp), %ecx
	movl	%ecx, 32(%esp)                  # 4-byte Spill
	movl	60(%esp), %ecx                  # 4-byte Reload
	adcl	2216(%esp), %ecx
	movl	%ecx, 60(%esp)                  # 4-byte Spill
	movl	28(%esp), %ecx                  # 4-byte Reload
	adcl	2220(%esp), %ecx
	movl	%ecx, 28(%esp)                  # 4-byte Spill
	movl	16(%esp), %ecx                  # 4-byte Reload
	adcl	2224(%esp), %ecx
	movl	%ecx, 16(%esp)                  # 4-byte Spill
	movl	8(%esp), %ecx                   # 4-byte Reload
	adcl	2228(%esp), %ecx
	movl	%ecx, 8(%esp)                   # 4-byte Spill
	adcl	2232(%esp), %edi
	movl	%edi, 12(%esp)                  # 4-byte Spill
	movl	4(%esp), %ecx                   # 4-byte Reload
	adcl	2236(%esp), %ecx
	movl	%ecx, 4(%esp)                   # 4-byte Spill
	adcl	2240(%esp), %esi
	movl	36(%esp), %ecx                  # 4-byte Reload
	adcl	2244(%esp), %ecx
	movl	%ecx, 36(%esp)                  # 4-byte Spill
	movl	20(%esp), %ecx                  # 4-byte Reload
	adcl	2248(%esp), %ecx
	movl	%ecx, 20(%esp)                  # 4-byte Spill
	adcl	2252(%esp), %ebp
	movl	%ebp, 52(%esp)                  # 4-byte Spill
	movl	40(%esp), %ecx                  # 4-byte Reload
	adcl	2256(%esp), %ecx
	movl	%ecx, 40(%esp)                  # 4-byte Spill
	movl	44(%esp), %ecx                  # 4-byte Reload
	adcl	2260(%esp), %ecx
	movl	%ecx, 44(%esp)                  # 4-byte Spill
	movl	%eax, %ebp
	adcl	$0, %ebp
	subl	$4, %esp
	leal	2132(%esp), %eax
	movl	72(%esp), %ecx                  # 4-byte Reload
	movl	%edx, %edi
	imull	%edx, %ecx
	pushl	%ecx
	pushl	2452(%esp)
	pushl	%eax
	calll	mulPv512x32@PLT
	addl	$12, %esp
	addl	2128(%esp), %edi
	movl	24(%esp), %eax                  # 4-byte Reload
	adcl	2132(%esp), %eax
	movl	%eax, 24(%esp)                  # 4-byte Spill
	movl	56(%esp), %eax                  # 4-byte Reload
	adcl	2136(%esp), %eax
	movl	%eax, 56(%esp)                  # 4-byte Spill
	movl	32(%esp), %eax                  # 4-byte Reload
	adcl	2140(%esp), %eax
	movl	%eax, 32(%esp)                  # 4-byte Spill
	movl	60(%esp), %eax                  # 4-byte Reload
	adcl	2144(%esp), %eax
	movl	%eax, 60(%esp)                  # 4-byte Spill
	movl	28(%esp), %eax                  # 4-byte Reload
	adcl	2148(%esp), %eax
	movl	%eax, 28(%esp)                  # 4-byte Spill
	movl	16(%esp), %edi                  # 4-byte Reload
	adcl	2152(%esp), %edi
	movl	8(%esp), %eax                   # 4-byte Reload
	adcl	2156(%esp), %eax
	movl	%eax, 8(%esp)                   # 4-byte Spill
	movl	12(%esp), %eax                  # 4-byte Reload
	adcl	2160(%esp), %eax
	movl	%eax, 12(%esp)                  # 4-byte Spill
	movl	4(%esp), %eax                   # 4-byte Reload
	adcl	2164(%esp), %eax
	movl	%eax, 4(%esp)                   # 4-byte Spill
	adcl	2168(%esp), %esi
	movl	%esi, 48(%esp)                  # 4-byte Spill
	movl	36(%esp), %eax                  # 4-byte Reload
	adcl	2172(%esp), %eax
	movl	%eax, 36(%esp)                  # 4-byte Spill
	movl	20(%esp), %eax                  # 4-byte Reload
	adcl	2176(%esp), %eax
	movl	%eax, 20(%esp)                  # 4-byte Spill
	movl	52(%esp), %eax                  # 4-byte Reload
	adcl	2180(%esp), %eax
	movl	%eax, 52(%esp)                  # 4-byte Spill
	movl	40(%esp), %eax                  # 4-byte Reload
	adcl	2184(%esp), %eax
	movl	%eax, 40(%esp)                  # 4-byte Spill
	movl	44(%esp), %esi                  # 4-byte Reload
	adcl	2188(%esp), %esi
	adcl	2192(%esp), %ebp
	subl	$4, %esp
	leal	2060(%esp), %eax
	movl	2444(%esp), %ecx
	pushl	8(%ecx)
	pushl	2444(%esp)
	pushl	%eax
	calll	mulPv512x32@PLT
	addl	$12, %esp
	movl	2120(%esp), %eax
	movl	24(%esp), %edx                  # 4-byte Reload
	addl	2056(%esp), %edx
	movl	56(%esp), %ecx                  # 4-byte Reload
	adcl	2060(%esp), %ecx
	movl	%ecx, 56(%esp)                  # 4-byte Spill
	movl	32(%esp), %ecx                  # 4-byte Reload
	adcl	2064(%esp), %ecx
	movl	%ecx, 32(%esp)                  # 4-byte Spill
	movl	60(%esp), %ecx                  # 4-byte Reload
	adcl	2068(%esp), %ecx
	movl	%ecx, 60(%esp)                  # 4-byte Spill
	movl	28(%esp), %ecx                  # 4-byte Reload
	adcl	2072(%esp), %ecx
	movl	%ecx, 28(%esp)                  # 4-byte Spill
	adcl	2076(%esp), %edi
	movl	%edi, 16(%esp)                  # 4-byte Spill
	movl	8(%esp), %ecx                   # 4-byte Reload
	adcl	2080(%esp), %ecx
	movl	%ecx, 8(%esp)                   # 4-byte Spill
	movl	12(%esp), %ecx                  # 4-byte Reload
	adcl	2084(%esp), %ecx
	movl	%ecx, 12(%esp)                  # 4-byte Spill
	movl	4(%esp), %ecx                   # 4-byte Reload
	adcl	2088(%esp), %ecx
	movl	%ecx, 4(%esp)                   # 4-byte Spill
	movl	48(%esp), %ecx                  # 4-byte Reload
	adcl	2092(%esp), %ecx
	movl	%ecx, 48(%esp)                  # 4-byte Spill
	movl	36(%esp), %edi                  # 4-byte Reload
	adcl	2096(%esp), %edi
	movl	20(%esp), %ecx                  # 4-byte Reload
	adcl	2100(%esp), %ecx
	movl	%ecx, 20(%esp)                  # 4-byte Spill
	movl	52(%esp), %ecx                  # 4-byte Reload
	adcl	2104(%esp), %ecx
	movl	%ecx, 52(%esp)                  # 4-byte Spill
	movl	40(%esp), %ecx                  # 4-byte Reload
	adcl	2108(%esp), %ecx
	movl	%ecx, 40(%esp)                  # 4-byte Spill
	adcl	2112(%esp), %esi
	movl	%esi, 44(%esp)                  # 4-byte Spill
	adcl	2116(%esp), %ebp
	movl	%ebp, 24(%esp)                  # 4-byte Spill
	adcl	$0, %eax
	movl	%eax, %ebp
	subl	$4, %esp
	leal	1988(%esp), %eax
	movl	72(%esp), %ecx                  # 4-byte Reload
	movl	%edx, %esi
	imull	%edx, %ecx
	pushl	%ecx
	pushl	2452(%esp)
	pushl	%eax
	calll	mulPv512x32@PLT
	addl	$12, %esp
	addl	1984(%esp), %esi
	movl	56(%esp), %eax                  # 4-byte Reload
	adcl	1988(%esp), %eax
	movl	%eax, 56(%esp)                  # 4-byte Spill
	movl	32(%esp), %eax                  # 4-byte Reload
	adcl	1992(%esp), %eax
	movl	%eax, 32(%esp)                  # 4-byte Spill
	movl	60(%esp), %eax                  # 4-byte Reload
	adcl	1996(%esp), %eax
	movl	%eax, 60(%esp)                  # 4-byte Spill
	movl	28(%esp), %eax                  # 4-byte Reload
	adcl	2000(%esp), %eax
	movl	%eax, 28(%esp)                  # 4-byte Spill
	movl	16(%esp), %eax                  # 4-byte Reload
	adcl	2004(%esp), %eax
	movl	%eax, 16(%esp)                  # 4-byte Spill
	movl	8(%esp), %eax                   # 4-byte Reload
	adcl	2008(%esp), %eax
	movl	%eax, 8(%esp)                   # 4-byte Spill
	movl	12(%esp), %eax                  # 4-byte Reload
	adcl	2012(%esp), %eax
	movl	%eax, 12(%esp)                  # 4-byte Spill
	movl	4(%esp), %eax                   # 4-byte Reload
	adcl	2016(%esp), %eax
	movl	%eax, 4(%esp)                   # 4-byte Spill
	movl	48(%esp), %eax                  # 4-byte Reload
	adcl	2020(%esp), %eax
	movl	%eax, 48(%esp)                  # 4-byte Spill
	adcl	2024(%esp), %edi
	movl	%edi, 36(%esp)                  # 4-byte Spill
	movl	20(%esp), %esi                  # 4-byte Reload
	adcl	2028(%esp), %esi
	movl	52(%esp), %eax                  # 4-byte Reload
	adcl	2032(%esp), %eax
	movl	%eax, 52(%esp)                  # 4-byte Spill
	movl	40(%esp), %edi                  # 4-byte Reload
	adcl	2036(%esp), %edi
	movl	44(%esp), %eax                  # 4-byte Reload
	adcl	2040(%esp), %eax
	movl	%eax, 44(%esp)                  # 4-byte Spill
	movl	24(%esp), %eax                  # 4-byte Reload
	adcl	2044(%esp), %eax
	movl	%eax, 24(%esp)                  # 4-byte Spill
	adcl	2048(%esp), %ebp
	movl	%ebp, 64(%esp)                  # 4-byte Spill
	subl	$4, %esp
	leal	1916(%esp), %eax
	movl	2444(%esp), %ecx
	pushl	12(%ecx)
	movl	2444(%esp), %ecx
	pushl	%ecx
	pushl	%eax
	calll	mulPv512x32@PLT
	addl	$12, %esp
	movl	1976(%esp), %eax
	movl	56(%esp), %edx                  # 4-byte Reload
	addl	1912(%esp), %edx
	movl	32(%esp), %ecx                  # 4-byte Reload
	adcl	1916(%esp), %ecx
	movl	%ecx, 32(%esp)                  # 4-byte Spill
	movl	60(%esp), %ecx                  # 4-byte Reload
	adcl	1920(%esp), %ecx
	movl	%ecx, 60(%esp)                  # 4-byte Spill
	movl	28(%esp), %ecx                  # 4-byte Reload
	adcl	1924(%esp), %ecx
	movl	%ecx, 28(%esp)                  # 4-byte Spill
	movl	16(%esp), %ecx                  # 4-byte Reload
	adcl	1928(%esp), %ecx
	movl	%ecx, 16(%esp)                  # 4-byte Spill
	movl	8(%esp), %ebp                   # 4-byte Reload
	adcl	1932(%esp), %ebp
	movl	12(%esp), %ecx                  # 4-byte Reload
	adcl	1936(%esp), %ecx
	movl	%ecx, 12(%esp)                  # 4-byte Spill
	movl	4(%esp), %ecx                   # 4-byte Reload
	adcl	1940(%esp), %ecx
	movl	%ecx, 4(%esp)                   # 4-byte Spill
	movl	48(%esp), %ecx                  # 4-byte Reload
	adcl	1944(%esp), %ecx
	movl	%ecx, 48(%esp)                  # 4-byte Spill
	movl	36(%esp), %ecx                  # 4-byte Reload
	adcl	1948(%esp), %ecx
	movl	%ecx, 36(%esp)                  # 4-byte Spill
	adcl	1952(%esp), %esi
	movl	%esi, 20(%esp)                  # 4-byte Spill
	movl	52(%esp), %ecx                  # 4-byte Reload
	adcl	1956(%esp), %ecx
	movl	%ecx, 52(%esp)                  # 4-byte Spill
	adcl	1960(%esp), %edi
	movl	%edi, 40(%esp)                  # 4-byte Spill
	movl	44(%esp), %ecx                  # 4-byte Reload
	adcl	1964(%esp), %ecx
	movl	%ecx, 44(%esp)                  # 4-byte Spill
	movl	24(%esp), %ecx                  # 4-byte Reload
	adcl	1968(%esp), %ecx
	movl	%ecx, 24(%esp)                  # 4-byte Spill
	movl	64(%esp), %edi                  # 4-byte Reload
	adcl	1972(%esp), %edi
	adcl	$0, %eax
	movl	%eax, 56(%esp)                  # 4-byte Spill
	subl	$4, %esp
	movl	72(%esp), %ecx                  # 4-byte Reload
	imull	%edx, %ecx
	movl	%edx, %esi
	pushl	%ecx
	movl	2452(%esp), %ecx
	pushl	%ecx
	leal	1852(%esp), %eax
	pushl	%eax
	calll	mulPv512x32@PLT
	addl	$12, %esp
	addl	1840(%esp), %esi
	movl	32(%esp), %eax                  # 4-byte Reload
	adcl	1844(%esp), %eax
	movl	%eax, 32(%esp)                  # 4-byte Spill
	movl	60(%esp), %eax                  # 4-byte Reload
	adcl	1848(%esp), %eax
	movl	%eax, 60(%esp)                  # 4-byte Spill
	movl	28(%esp), %eax                  # 4-byte Reload
	adcl	1852(%esp), %eax
	movl	%eax, 28(%esp)                  # 4-byte Spill
	movl	16(%esp), %eax                  # 4-byte Reload
	adcl	1856(%esp), %eax
	movl	%eax, 16(%esp)                  # 4-byte Spill
	adcl	1860(%esp), %ebp
	movl	%ebp, 8(%esp)                   # 4-byte Spill
	movl	12(%esp), %eax                  # 4-byte Reload
	adcl	1864(%esp), %eax
	movl	%eax, 12(%esp)                  # 4-byte Spill
	movl	4(%esp), %eax                   # 4-byte Reload
	adcl	1868(%esp), %eax
	movl	%eax, 4(%esp)                   # 4-byte Spill
	movl	48(%esp), %eax                  # 4-byte Reload
	adcl	1872(%esp), %eax
	movl	%eax, 48(%esp)                  # 4-byte Spill
	movl	36(%esp), %eax                  # 4-byte Reload
	adcl	1876(%esp), %eax
	movl	%eax, 36(%esp)                  # 4-byte Spill
	movl	20(%esp), %eax                  # 4-byte Reload
	adcl	1880(%esp), %eax
	movl	%eax, 20(%esp)                  # 4-byte Spill
	movl	52(%esp), %esi                  # 4-byte Reload
	adcl	1884(%esp), %esi
	movl	40(%esp), %ebp                  # 4-byte Reload
	adcl	1888(%esp), %ebp
	movl	44(%esp), %eax                  # 4-byte Reload
	adcl	1892(%esp), %eax
	movl	%eax, 44(%esp)                  # 4-byte Spill
	movl	24(%esp), %eax                  # 4-byte Reload
	adcl	1896(%esp), %eax
	movl	%eax, 24(%esp)                  # 4-byte Spill
	adcl	1900(%esp), %edi
	movl	%edi, 64(%esp)                  # 4-byte Spill
	movl	56(%esp), %edi                  # 4-byte Reload
	adcl	1904(%esp), %edi
	subl	$4, %esp
	movl	2444(%esp), %eax
	pushl	16(%eax)
	pushl	2444(%esp)
	leal	1780(%esp), %eax
	pushl	%eax
	calll	mulPv512x32@PLT
	addl	$12, %esp
	movl	1832(%esp), %ecx
	movl	32(%esp), %eax                  # 4-byte Reload
	addl	1768(%esp), %eax
	movl	60(%esp), %edx                  # 4-byte Reload
	adcl	1772(%esp), %edx
	movl	%edx, 60(%esp)                  # 4-byte Spill
	movl	28(%esp), %edx                  # 4-byte Reload
	adcl	1776(%esp), %edx
	movl	%edx, 28(%esp)                  # 4-byte Spill
	movl	16(%esp), %edx                  # 4-byte Reload
	adcl	1780(%esp), %edx
	movl	%edx, 16(%esp)                  # 4-byte Spill
	movl	8(%esp), %edx                   # 4-byte Reload
	adcl	1784(%esp), %edx
	movl	%edx, 8(%esp)                   # 4-byte Spill
	movl	12(%esp), %edx                  # 4-byte Reload
	adcl	1788(%esp), %edx
	movl	%edx, 12(%esp)                  # 4-byte Spill
	movl	4(%esp), %edx                   # 4-byte Reload
	adcl	1792(%esp), %edx
	movl	%edx, 4(%esp)                   # 4-byte Spill
	movl	48(%esp), %edx                  # 4-byte Reload
	adcl	1796(%esp), %edx
	movl	%edx, 48(%esp)                  # 4-byte Spill
	movl	36(%esp), %edx                  # 4-byte Reload
	adcl	1800(%esp), %edx
	movl	%edx, 36(%esp)                  # 4-byte Spill
	movl	20(%esp), %edx                  # 4-byte Reload
	adcl	1804(%esp), %edx
	movl	%edx, 20(%esp)                  # 4-byte Spill
	adcl	1808(%esp), %esi
	movl	%esi, 52(%esp)                  # 4-byte Spill
	adcl	1812(%esp), %ebp
	movl	%ebp, 40(%esp)                  # 4-byte Spill
	movl	44(%esp), %edx                  # 4-byte Reload
	adcl	1816(%esp), %edx
	movl	%edx, 44(%esp)                  # 4-byte Spill
	movl	24(%esp), %ebp                  # 4-byte Reload
	adcl	1820(%esp), %ebp
	movl	64(%esp), %edx                  # 4-byte Reload
	adcl	1824(%esp), %edx
	movl	%edx, 64(%esp)                  # 4-byte Spill
	adcl	1828(%esp), %edi
	movl	%edi, 56(%esp)                  # 4-byte Spill
	movl	%ecx, %edi
	adcl	$0, %edi
	subl	$4, %esp
	movl	72(%esp), %ecx                  # 4-byte Reload
	imull	%eax, %ecx
	movl	%eax, %esi
	pushl	%ecx
	pushl	2452(%esp)
	leal	1708(%esp), %eax
	pushl	%eax
	calll	mulPv512x32@PLT
	addl	$12, %esp
	addl	1696(%esp), %esi
	movl	60(%esp), %eax                  # 4-byte Reload
	adcl	1700(%esp), %eax
	movl	%eax, 60(%esp)                  # 4-byte Spill
	movl	28(%esp), %eax                  # 4-byte Reload
	adcl	1704(%esp), %eax
	movl	%eax, 28(%esp)                  # 4-byte Spill
	movl	16(%esp), %eax                  # 4-byte Reload
	adcl	1708(%esp), %eax
	movl	%eax, 16(%esp)                  # 4-byte Spill
	movl	8(%esp), %eax                   # 4-byte Reload
	adcl	1712(%esp), %eax
	movl	%eax, 8(%esp)                   # 4-byte Spill
	movl	12(%esp), %eax                  # 4-byte Reload
	adcl	1716(%esp), %eax
	movl	%eax, 12(%esp)                  # 4-byte Spill
	movl	4(%esp), %eax                   # 4-byte Reload
	adcl	1720(%esp), %eax
	movl	%eax, 4(%esp)                   # 4-byte Spill
	movl	48(%esp), %eax                  # 4-byte Reload
	adcl	1724(%esp), %eax
	movl	%eax, 48(%esp)                  # 4-byte Spill
	movl	36(%esp), %eax                  # 4-byte Reload
	adcl	1728(%esp), %eax
	movl	%eax, 36(%esp)                  # 4-byte Spill
	movl	20(%esp), %eax                  # 4-byte Reload
	adcl	1732(%esp), %eax
	movl	%eax, 20(%esp)                  # 4-byte Spill
	movl	52(%esp), %eax                  # 4-byte Reload
	adcl	1736(%esp), %eax
	movl	%eax, 52(%esp)                  # 4-byte Spill
	movl	40(%esp), %eax                  # 4-byte Reload
	adcl	1740(%esp), %eax
	movl	%eax, 40(%esp)                  # 4-byte Spill
	movl	44(%esp), %eax                  # 4-byte Reload
	adcl	1744(%esp), %eax
	movl	%eax, 44(%esp)                  # 4-byte Spill
	adcl	1748(%esp), %ebp
	movl	%ebp, 24(%esp)                  # 4-byte Spill
	movl	64(%esp), %esi                  # 4-byte Reload
	adcl	1752(%esp), %esi
	movl	56(%esp), %eax                  # 4-byte Reload
	adcl	1756(%esp), %eax
	movl	%eax, 56(%esp)                  # 4-byte Spill
	adcl	1760(%esp), %edi
	movl	%edi, 32(%esp)                  # 4-byte Spill
	subl	$4, %esp
	movl	2444(%esp), %eax
	pushl	20(%eax)
	pushl	2444(%esp)
	leal	1636(%esp), %eax
	pushl	%eax
	calll	mulPv512x32@PLT
	addl	$12, %esp
	movl	1688(%esp), %eax
	movl	60(%esp), %edx                  # 4-byte Reload
	addl	1624(%esp), %edx
	movl	28(%esp), %ecx                  # 4-byte Reload
	adcl	1628(%esp), %ecx
	movl	%ecx, 28(%esp)                  # 4-byte Spill
	movl	16(%esp), %ecx                  # 4-byte Reload
	adcl	1632(%esp), %ecx
	movl	%ecx, 16(%esp)                  # 4-byte Spill
	movl	8(%esp), %ecx                   # 4-byte Reload
	adcl	1636(%esp), %ecx
	movl	%ecx, 8(%esp)                   # 4-byte Spill
	movl	12(%esp), %ecx                  # 4-byte Reload
	adcl	1640(%esp), %ecx
	movl	%ecx, 12(%esp)                  # 4-byte Spill
	movl	4(%esp), %ebp                   # 4-byte Reload
	adcl	1644(%esp), %ebp
	movl	48(%esp), %edi                  # 4-byte Reload
	adcl	1648(%esp), %edi
	movl	36(%esp), %ecx                  # 4-byte Reload
	adcl	1652(%esp), %ecx
	movl	%ecx, 36(%esp)                  # 4-byte Spill
	movl	20(%esp), %ecx                  # 4-byte Reload
	adcl	1656(%esp), %ecx
	movl	%ecx, 20(%esp)                  # 4-byte Spill
	movl	52(%esp), %ecx                  # 4-byte Reload
	adcl	1660(%esp), %ecx
	movl	%ecx, 52(%esp)                  # 4-byte Spill
	movl	40(%esp), %ecx                  # 4-byte Reload
	adcl	1664(%esp), %ecx
	movl	%ecx, 40(%esp)                  # 4-byte Spill
	movl	44(%esp), %ecx                  # 4-byte Reload
	adcl	1668(%esp), %ecx
	movl	%ecx, 44(%esp)                  # 4-byte Spill
	movl	24(%esp), %ecx                  # 4-byte Reload
	adcl	1672(%esp), %ecx
	movl	%ecx, 24(%esp)                  # 4-byte Spill
	adcl	1676(%esp), %esi
	movl	%esi, 64(%esp)                  # 4-byte Spill
	movl	56(%esp), %ecx                  # 4-byte Reload
	adcl	1680(%esp), %ecx
	movl	%ecx, 56(%esp)                  # 4-byte Spill
	movl	32(%esp), %ecx                  # 4-byte Reload
	adcl	1684(%esp), %ecx
	movl	%ecx, 32(%esp)                  # 4-byte Spill
	adcl	$0, %eax
	movl	%eax, 60(%esp)                  # 4-byte Spill
	subl	$4, %esp
	movl	72(%esp), %ecx                  # 4-byte Reload
	movl	%edx, %esi
	imull	%edx, %ecx
	pushl	%ecx
	pushl	2452(%esp)
	leal	1564(%esp), %eax
	pushl	%eax
	calll	mulPv512x32@PLT
	addl	$12, %esp
	addl	1552(%esp), %esi
	movl	28(%esp), %eax                  # 4-byte Reload
	adcl	1556(%esp), %eax
	movl	%eax, 28(%esp)                  # 4-byte Spill
	movl	16(%esp), %eax                  # 4-byte Reload
	adcl	1560(%esp), %eax
	movl	%eax, 16(%esp)                  # 4-byte Spill
	movl	8(%esp), %eax                   # 4-byte Reload
	adcl	1564(%esp), %eax
	movl	%eax, 8(%esp)                   # 4-byte Spill
	movl	12(%esp), %eax                  # 4-byte Reload
	adcl	1568(%esp), %eax
	movl	%eax, 12(%esp)                  # 4-byte Spill
	adcl	1572(%esp), %ebp
	movl	%ebp, 4(%esp)                   # 4-byte Spill
	adcl	1576(%esp), %edi
	movl	%edi, 48(%esp)                  # 4-byte Spill
	movl	36(%esp), %eax                  # 4-byte Reload
	adcl	1580(%esp), %eax
	movl	%eax, 36(%esp)                  # 4-byte Spill
	movl	20(%esp), %eax                  # 4-byte Reload
	adcl	1584(%esp), %eax
	movl	%eax, 20(%esp)                  # 4-byte Spill
	movl	52(%esp), %eax                  # 4-byte Reload
	adcl	1588(%esp), %eax
	movl	%eax, 52(%esp)                  # 4-byte Spill
	movl	40(%esp), %eax                  # 4-byte Reload
	adcl	1592(%esp), %eax
	movl	%eax, 40(%esp)                  # 4-byte Spill
	movl	44(%esp), %esi                  # 4-byte Reload
	adcl	1596(%esp), %esi
	movl	24(%esp), %eax                  # 4-byte Reload
	adcl	1600(%esp), %eax
	movl	%eax, 24(%esp)                  # 4-byte Spill
	movl	64(%esp), %eax                  # 4-byte Reload
	adcl	1604(%esp), %eax
	movl	%eax, 64(%esp)                  # 4-byte Spill
	movl	56(%esp), %eax                  # 4-byte Reload
	adcl	1608(%esp), %eax
	movl	%eax, 56(%esp)                  # 4-byte Spill
	movl	32(%esp), %eax                  # 4-byte Reload
	adcl	1612(%esp), %eax
	movl	%eax, 32(%esp)                  # 4-byte Spill
	movl	60(%esp), %edi                  # 4-byte Reload
	adcl	1616(%esp), %edi
	subl	$4, %esp
	movl	2444(%esp), %eax
	pushl	24(%eax)
	pushl	2444(%esp)
	leal	1492(%esp), %eax
	pushl	%eax
	calll	mulPv512x32@PLT
	addl	$12, %esp
	movl	1544(%esp), %eax
	movl	28(%esp), %edx                  # 4-byte Reload
	addl	1480(%esp), %edx
	movl	16(%esp), %ecx                  # 4-byte Reload
	adcl	1484(%esp), %ecx
	movl	%ecx, 16(%esp)                  # 4-byte Spill
	movl	8(%esp), %ecx                   # 4-byte Reload
	adcl	1488(%esp), %ecx
	movl	%ecx, 8(%esp)                   # 4-byte Spill
	movl	12(%esp), %ebp                  # 4-byte Reload
	adcl	1492(%esp), %ebp
	movl	4(%esp), %ecx                   # 4-byte Reload
	adcl	1496(%esp), %ecx
	movl	%ecx, 4(%esp)                   # 4-byte Spill
	movl	48(%esp), %ecx                  # 4-byte Reload
	adcl	1500(%esp), %ecx
	movl	%ecx, 48(%esp)                  # 4-byte Spill
	movl	36(%esp), %ecx                  # 4-byte Reload
	adcl	1504(%esp), %ecx
	movl	%ecx, 36(%esp)                  # 4-byte Spill
	movl	20(%esp), %ecx                  # 4-byte Reload
	adcl	1508(%esp), %ecx
	movl	%ecx, 20(%esp)                  # 4-byte Spill
	movl	52(%esp), %ecx                  # 4-byte Reload
	adcl	1512(%esp), %ecx
	movl	%ecx, 52(%esp)                  # 4-byte Spill
	movl	40(%esp), %ecx                  # 4-byte Reload
	adcl	1516(%esp), %ecx
	movl	%ecx, 40(%esp)                  # 4-byte Spill
	adcl	1520(%esp), %esi
	movl	%esi, 44(%esp)                  # 4-byte Spill
	movl	24(%esp), %ecx                  # 4-byte Reload
	adcl	1524(%esp), %ecx
	movl	%ecx, 24(%esp)                  # 4-byte Spill
	movl	64(%esp), %ecx                  # 4-byte Reload
	adcl	1528(%esp), %ecx
	movl	%ecx, 64(%esp)                  # 4-byte Spill
	movl	56(%esp), %ecx                  # 4-byte Reload
	adcl	1532(%esp), %ecx
	movl	%ecx, 56(%esp)                  # 4-byte Spill
	movl	32(%esp), %ecx                  # 4-byte Reload
	adcl	1536(%esp), %ecx
	movl	%ecx, 32(%esp)                  # 4-byte Spill
	adcl	1540(%esp), %edi
	adcl	$0, %eax
	movl	%eax, 28(%esp)                  # 4-byte Spill
	subl	$4, %esp
	movl	72(%esp), %ecx                  # 4-byte Reload
	movl	%edx, %esi
	imull	%edx, %ecx
	pushl	%ecx
	pushl	2452(%esp)
	leal	1420(%esp), %eax
	pushl	%eax
	calll	mulPv512x32@PLT
	addl	$12, %esp
	addl	1408(%esp), %esi
	movl	16(%esp), %eax                  # 4-byte Reload
	adcl	1412(%esp), %eax
	movl	%eax, 16(%esp)                  # 4-byte Spill
	movl	8(%esp), %eax                   # 4-byte Reload
	adcl	1416(%esp), %eax
	movl	%eax, 8(%esp)                   # 4-byte Spill
	adcl	1420(%esp), %ebp
	movl	%ebp, 12(%esp)                  # 4-byte Spill
	movl	4(%esp), %eax                   # 4-byte Reload
	adcl	1424(%esp), %eax
	movl	%eax, 4(%esp)                   # 4-byte Spill
	movl	48(%esp), %eax                  # 4-byte Reload
	adcl	1428(%esp), %eax
	movl	%eax, 48(%esp)                  # 4-byte Spill
	movl	36(%esp), %esi                  # 4-byte Reload
	adcl	1432(%esp), %esi
	movl	20(%esp), %eax                  # 4-byte Reload
	adcl	1436(%esp), %eax
	movl	%eax, 20(%esp)                  # 4-byte Spill
	movl	52(%esp), %ebp                  # 4-byte Reload
	adcl	1440(%esp), %ebp
	movl	40(%esp), %eax                  # 4-byte Reload
	adcl	1444(%esp), %eax
	movl	%eax, 40(%esp)                  # 4-byte Spill
	movl	44(%esp), %eax                  # 4-byte Reload
	adcl	1448(%esp), %eax
	movl	%eax, 44(%esp)                  # 4-byte Spill
	movl	24(%esp), %eax                  # 4-byte Reload
	adcl	1452(%esp), %eax
	movl	%eax, 24(%esp)                  # 4-byte Spill
	movl	64(%esp), %eax                  # 4-byte Reload
	adcl	1456(%esp), %eax
	movl	%eax, 64(%esp)                  # 4-byte Spill
	movl	56(%esp), %eax                  # 4-byte Reload
	adcl	1460(%esp), %eax
	movl	%eax, 56(%esp)                  # 4-byte Spill
	movl	32(%esp), %eax                  # 4-byte Reload
	adcl	1464(%esp), %eax
	movl	%eax, 32(%esp)                  # 4-byte Spill
	adcl	1468(%esp), %edi
	movl	28(%esp), %eax                  # 4-byte Reload
	adcl	1472(%esp), %eax
	movl	%eax, 28(%esp)                  # 4-byte Spill
	subl	$4, %esp
	movl	2444(%esp), %eax
	pushl	28(%eax)
	pushl	2444(%esp)
	leal	1348(%esp), %eax
	pushl	%eax
	calll	mulPv512x32@PLT
	addl	$12, %esp
	movl	1400(%esp), %eax
	movl	16(%esp), %edx                  # 4-byte Reload
	addl	1336(%esp), %edx
	movl	8(%esp), %ecx                   # 4-byte Reload
	adcl	1340(%esp), %ecx
	movl	%ecx, 8(%esp)                   # 4-byte Spill
	movl	12(%esp), %ecx                  # 4-byte Reload
	adcl	1344(%esp), %ecx
	movl	%ecx, 12(%esp)                  # 4-byte Spill
	movl	4(%esp), %ecx                   # 4-byte Reload
	adcl	1348(%esp), %ecx
	movl	%ecx, 4(%esp)                   # 4-byte Spill
	movl	48(%esp), %ecx                  # 4-byte Reload
	adcl	1352(%esp), %ecx
	movl	%ecx, 48(%esp)                  # 4-byte Spill
	adcl	1356(%esp), %esi
	movl	20(%esp), %ecx                  # 4-byte Reload
	adcl	1360(%esp), %ecx
	movl	%ecx, 20(%esp)                  # 4-byte Spill
	adcl	1364(%esp), %ebp
	movl	%ebp, 52(%esp)                  # 4-byte Spill
	movl	40(%esp), %ecx                  # 4-byte Reload
	adcl	1368(%esp), %ecx
	movl	%ecx, 40(%esp)                  # 4-byte Spill
	movl	44(%esp), %ecx                  # 4-byte Reload
	adcl	1372(%esp), %ecx
	movl	%ecx, 44(%esp)                  # 4-byte Spill
	movl	24(%esp), %ecx                  # 4-byte Reload
	adcl	1376(%esp), %ecx
	movl	%ecx, 24(%esp)                  # 4-byte Spill
	movl	64(%esp), %ecx                  # 4-byte Reload
	adcl	1380(%esp), %ecx
	movl	%ecx, 64(%esp)                  # 4-byte Spill
	movl	56(%esp), %ebp                  # 4-byte Reload
	adcl	1384(%esp), %ebp
	movl	32(%esp), %ecx                  # 4-byte Reload
	adcl	1388(%esp), %ecx
	movl	%ecx, 32(%esp)                  # 4-byte Spill
	adcl	1392(%esp), %edi
	movl	%edi, 60(%esp)                  # 4-byte Spill
	movl	28(%esp), %ecx                  # 4-byte Reload
	adcl	1396(%esp), %ecx
	movl	%ecx, 28(%esp)                  # 4-byte Spill
	adcl	$0, %eax
	movl	%eax, 16(%esp)                  # 4-byte Spill
	subl	$4, %esp
	movl	72(%esp), %ecx                  # 4-byte Reload
	movl	%edx, %edi
	imull	%edx, %ecx
	pushl	%ecx
	pushl	2452(%esp)
	leal	1276(%esp), %eax
	pushl	%eax
	calll	mulPv512x32@PLT
	addl	$12, %esp
	addl	1264(%esp), %edi
	movl	8(%esp), %eax                   # 4-byte Reload
	adcl	1268(%esp), %eax
	movl	%eax, 8(%esp)                   # 4-byte Spill
	movl	12(%esp), %eax                  # 4-byte Reload
	adcl	1272(%esp), %eax
	movl	%eax, 12(%esp)                  # 4-byte Spill
	movl	4(%esp), %eax                   # 4-byte Reload
	adcl	1276(%esp), %eax
	movl	%eax, 4(%esp)                   # 4-byte Spill
	movl	48(%esp), %eax                  # 4-byte Reload
	adcl	1280(%esp), %eax
	movl	%eax, 48(%esp)                  # 4-byte Spill
	adcl	1284(%esp), %esi
	movl	20(%esp), %eax                  # 4-byte Reload
	adcl	1288(%esp), %eax
	movl	%eax, 20(%esp)                  # 4-byte Spill
	movl	52(%esp), %eax                  # 4-byte Reload
	adcl	1292(%esp), %eax
	movl	%eax, 52(%esp)                  # 4-byte Spill
	movl	40(%esp), %edi                  # 4-byte Reload
	adcl	1296(%esp), %edi
	movl	44(%esp), %eax                  # 4-byte Reload
	adcl	1300(%esp), %eax
	movl	%eax, 44(%esp)                  # 4-byte Spill
	movl	24(%esp), %eax                  # 4-byte Reload
	adcl	1304(%esp), %eax
	movl	%eax, 24(%esp)                  # 4-byte Spill
	movl	64(%esp), %eax                  # 4-byte Reload
	adcl	1308(%esp), %eax
	movl	%eax, 64(%esp)                  # 4-byte Spill
	adcl	1312(%esp), %ebp
	movl	32(%esp), %eax                  # 4-byte Reload
	adcl	1316(%esp), %eax
	movl	%eax, 32(%esp)                  # 4-byte Spill
	movl	60(%esp), %eax                  # 4-byte Reload
	adcl	1320(%esp), %eax
	movl	%eax, 60(%esp)                  # 4-byte Spill
	movl	28(%esp), %eax                  # 4-byte Reload
	adcl	1324(%esp), %eax
	movl	%eax, 28(%esp)                  # 4-byte Spill
	movl	16(%esp), %eax                  # 4-byte Reload
	adcl	1328(%esp), %eax
	movl	%eax, 16(%esp)                  # 4-byte Spill
	subl	$4, %esp
	movl	2444(%esp), %eax
	pushl	32(%eax)
	pushl	2444(%esp)
	leal	1204(%esp), %eax
	pushl	%eax
	calll	mulPv512x32@PLT
	addl	$12, %esp
	movl	1256(%esp), %eax
	movl	8(%esp), %edx                   # 4-byte Reload
	addl	1192(%esp), %edx
	movl	12(%esp), %ecx                  # 4-byte Reload
	adcl	1196(%esp), %ecx
	movl	%ecx, 12(%esp)                  # 4-byte Spill
	movl	4(%esp), %ecx                   # 4-byte Reload
	adcl	1200(%esp), %ecx
	movl	%ecx, 4(%esp)                   # 4-byte Spill
	movl	48(%esp), %ecx                  # 4-byte Reload
	adcl	1204(%esp), %ecx
	movl	%ecx, 48(%esp)                  # 4-byte Spill
	adcl	1208(%esp), %esi
	movl	%esi, 36(%esp)                  # 4-byte Spill
	movl	20(%esp), %ecx                  # 4-byte Reload
	adcl	1212(%esp), %ecx
	movl	%ecx, 20(%esp)                  # 4-byte Spill
	movl	52(%esp), %ecx                  # 4-byte Reload
	adcl	1216(%esp), %ecx
	movl	%ecx, 52(%esp)                  # 4-byte Spill
	adcl	1220(%esp), %edi
	movl	%edi, 40(%esp)                  # 4-byte Spill
	movl	44(%esp), %ecx                  # 4-byte Reload
	adcl	1224(%esp), %ecx
	movl	%ecx, 44(%esp)                  # 4-byte Spill
	movl	24(%esp), %ecx                  # 4-byte Reload
	adcl	1228(%esp), %ecx
	movl	%ecx, 24(%esp)                  # 4-byte Spill
	movl	64(%esp), %ecx                  # 4-byte Reload
	adcl	1232(%esp), %ecx
	movl	%ecx, 64(%esp)                  # 4-byte Spill
	adcl	1236(%esp), %ebp
	movl	%ebp, 56(%esp)                  # 4-byte Spill
	movl	32(%esp), %ebp                  # 4-byte Reload
	adcl	1240(%esp), %ebp
	movl	60(%esp), %edi                  # 4-byte Reload
	adcl	1244(%esp), %edi
	movl	28(%esp), %ecx                  # 4-byte Reload
	adcl	1248(%esp), %ecx
	movl	%ecx, 28(%esp)                  # 4-byte Spill
	movl	16(%esp), %ecx                  # 4-byte Reload
	adcl	1252(%esp), %ecx
	movl	%ecx, 16(%esp)                  # 4-byte Spill
	adcl	$0, %eax
	movl	%eax, 8(%esp)                   # 4-byte Spill
	subl	$4, %esp
	movl	72(%esp), %ecx                  # 4-byte Reload
	imull	%edx, %ecx
	movl	%edx, %esi
	pushl	%ecx
	pushl	2452(%esp)
	leal	1132(%esp), %eax
	pushl	%eax
	calll	mulPv512x32@PLT
	addl	$12, %esp
	addl	1120(%esp), %esi
	movl	12(%esp), %eax                  # 4-byte Reload
	adcl	1124(%esp), %eax
	movl	%eax, 12(%esp)                  # 4-byte Spill
	movl	4(%esp), %eax                   # 4-byte Reload
	adcl	1128(%esp), %eax
	movl	%eax, 4(%esp)                   # 4-byte Spill
	movl	48(%esp), %eax                  # 4-byte Reload
	adcl	1132(%esp), %eax
	movl	%eax, 48(%esp)                  # 4-byte Spill
	movl	36(%esp), %eax                  # 4-byte Reload
	adcl	1136(%esp), %eax
	movl	%eax, 36(%esp)                  # 4-byte Spill
	movl	20(%esp), %esi                  # 4-byte Reload
	adcl	1140(%esp), %esi
	movl	52(%esp), %eax                  # 4-byte Reload
	adcl	1144(%esp), %eax
	movl	%eax, 52(%esp)                  # 4-byte Spill
	movl	40(%esp), %eax                  # 4-byte Reload
	adcl	1148(%esp), %eax
	movl	%eax, 40(%esp)                  # 4-byte Spill
	movl	44(%esp), %eax                  # 4-byte Reload
	adcl	1152(%esp), %eax
	movl	%eax, 44(%esp)                  # 4-byte Spill
	movl	24(%esp), %eax                  # 4-byte Reload
	adcl	1156(%esp), %eax
	movl	%eax, 24(%esp)                  # 4-byte Spill
	movl	64(%esp), %eax                  # 4-byte Reload
	adcl	1160(%esp), %eax
	movl	%eax, 64(%esp)                  # 4-byte Spill
	movl	56(%esp), %eax                  # 4-byte Reload
	adcl	1164(%esp), %eax
	movl	%eax, 56(%esp)                  # 4-byte Spill
	adcl	1168(%esp), %ebp
	movl	%ebp, 32(%esp)                  # 4-byte Spill
	adcl	1172(%esp), %edi
	movl	%edi, 60(%esp)                  # 4-byte Spill
	movl	28(%esp), %edi                  # 4-byte Reload
	adcl	1176(%esp), %edi
	movl	16(%esp), %eax                  # 4-byte Reload
	adcl	1180(%esp), %eax
	movl	%eax, 16(%esp)                  # 4-byte Spill
	movl	8(%esp), %ebp                   # 4-byte Reload
	adcl	1184(%esp), %ebp
	subl	$4, %esp
	movl	2444(%esp), %eax
	pushl	36(%eax)
	movl	2444(%esp), %eax
	pushl	%eax
	leal	1060(%esp), %eax
	pushl	%eax
	calll	mulPv512x32@PLT
	addl	$12, %esp
	movl	1112(%esp), %eax
	movl	12(%esp), %edx                  # 4-byte Reload
	addl	1048(%esp), %edx
	movl	4(%esp), %ecx                   # 4-byte Reload
	adcl	1052(%esp), %ecx
	movl	%ecx, 4(%esp)                   # 4-byte Spill
	movl	48(%esp), %ecx                  # 4-byte Reload
	adcl	1056(%esp), %ecx
	movl	%ecx, 48(%esp)                  # 4-byte Spill
	movl	36(%esp), %ecx                  # 4-byte Reload
	adcl	1060(%esp), %ecx
	movl	%ecx, 36(%esp)                  # 4-byte Spill
	adcl	1064(%esp), %esi
	movl	52(%esp), %ecx                  # 4-byte Reload
	adcl	1068(%esp), %ecx
	movl	%ecx, 52(%esp)                  # 4-byte Spill
	movl	40(%esp), %ecx                  # 4-byte Reload
	adcl	1072(%esp), %ecx
	movl	%ecx, 40(%esp)                  # 4-byte Spill
	movl	44(%esp), %ecx                  # 4-byte Reload
	adcl	1076(%esp), %ecx
	movl	%ecx, 44(%esp)                  # 4-byte Spill
	movl	24(%esp), %ecx                  # 4-byte Reload
	adcl	1080(%esp), %ecx
	movl	%ecx, 24(%esp)                  # 4-byte Spill
	movl	64(%esp), %ecx                  # 4-byte Reload
	adcl	1084(%esp), %ecx
	movl	%ecx, 64(%esp)                  # 4-byte Spill
	movl	56(%esp), %ecx                  # 4-byte Reload
	adcl	1088(%esp), %ecx
	movl	%ecx, 56(%esp)                  # 4-byte Spill
	movl	32(%esp), %ecx                  # 4-byte Reload
	adcl	1092(%esp), %ecx
	movl	%ecx, 32(%esp)                  # 4-byte Spill
	movl	60(%esp), %ecx                  # 4-byte Reload
	adcl	1096(%esp), %ecx
	movl	%ecx, 60(%esp)                  # 4-byte Spill
	adcl	1100(%esp), %edi
	movl	%edi, 28(%esp)                  # 4-byte Spill
	movl	16(%esp), %ecx                  # 4-byte Reload
	adcl	1104(%esp), %ecx
	movl	%ecx, 16(%esp)                  # 4-byte Spill
	adcl	1108(%esp), %ebp
	movl	%ebp, 8(%esp)                   # 4-byte Spill
	adcl	$0, %eax
	movl	%eax, 12(%esp)                  # 4-byte Spill
	subl	$4, %esp
	movl	72(%esp), %ecx                  # 4-byte Reload
	imull	%edx, %ecx
	movl	%edx, %edi
	pushl	%ecx
	pushl	2452(%esp)
	leal	988(%esp), %eax
	pushl	%eax
	calll	mulPv512x32@PLT
	addl	$12, %esp
	addl	976(%esp), %edi
	movl	4(%esp), %eax                   # 4-byte Reload
	adcl	980(%esp), %eax
	movl	%eax, 4(%esp)                   # 4-byte Spill
	movl	48(%esp), %eax                  # 4-byte Reload
	adcl	984(%esp), %eax
	movl	%eax, 48(%esp)                  # 4-byte Spill
	movl	36(%esp), %eax                  # 4-byte Reload
	adcl	988(%esp), %eax
	movl	%eax, 36(%esp)                  # 4-byte Spill
	adcl	992(%esp), %esi
	movl	%esi, 20(%esp)                  # 4-byte Spill
	movl	52(%esp), %eax                  # 4-byte Reload
	adcl	996(%esp), %eax
	movl	%eax, 52(%esp)                  # 4-byte Spill
	movl	40(%esp), %eax                  # 4-byte Reload
	adcl	1000(%esp), %eax
	movl	%eax, 40(%esp)                  # 4-byte Spill
	movl	44(%esp), %eax                  # 4-byte Reload
	adcl	1004(%esp), %eax
	movl	%eax, 44(%esp)                  # 4-byte Spill
	movl	24(%esp), %esi                  # 4-byte Reload
	adcl	1008(%esp), %esi
	movl	64(%esp), %ebp                  # 4-byte Reload
	adcl	1012(%esp), %ebp
	movl	56(%esp), %eax                  # 4-byte Reload
	adcl	1016(%esp), %eax
	movl	%eax, 56(%esp)                  # 4-byte Spill
	movl	32(%esp), %eax                  # 4-byte Reload
	adcl	1020(%esp), %eax
	movl	%eax, 32(%esp)                  # 4-byte Spill
	movl	60(%esp), %eax                  # 4-byte Reload
	adcl	1024(%esp), %eax
	movl	%eax, 60(%esp)                  # 4-byte Spill
	movl	28(%esp), %eax                  # 4-byte Reload
	adcl	1028(%esp), %eax
	movl	%eax, 28(%esp)                  # 4-byte Spill
	movl	16(%esp), %edi                  # 4-byte Reload
	adcl	1032(%esp), %edi
	movl	8(%esp), %eax                   # 4-byte Reload
	adcl	1036(%esp), %eax
	movl	%eax, 8(%esp)                   # 4-byte Spill
	movl	12(%esp), %eax                  # 4-byte Reload
	adcl	1040(%esp), %eax
	movl	%eax, 12(%esp)                  # 4-byte Spill
	subl	$4, %esp
	movl	2444(%esp), %eax
	pushl	40(%eax)
	pushl	2444(%esp)
	leal	916(%esp), %eax
	pushl	%eax
	calll	mulPv512x32@PLT
	addl	$12, %esp
	movl	968(%esp), %eax
	movl	4(%esp), %edx                   # 4-byte Reload
	addl	904(%esp), %edx
	movl	48(%esp), %ecx                  # 4-byte Reload
	adcl	908(%esp), %ecx
	movl	%ecx, 48(%esp)                  # 4-byte Spill
	movl	36(%esp), %ecx                  # 4-byte Reload
	adcl	912(%esp), %ecx
	movl	%ecx, 36(%esp)                  # 4-byte Spill
	movl	20(%esp), %ecx                  # 4-byte Reload
	adcl	916(%esp), %ecx
	movl	%ecx, 20(%esp)                  # 4-byte Spill
	movl	52(%esp), %ecx                  # 4-byte Reload
	adcl	920(%esp), %ecx
	movl	%ecx, 52(%esp)                  # 4-byte Spill
	movl	40(%esp), %ecx                  # 4-byte Reload
	adcl	924(%esp), %ecx
	movl	%ecx, 40(%esp)                  # 4-byte Spill
	movl	44(%esp), %ecx                  # 4-byte Reload
	adcl	928(%esp), %ecx
	movl	%ecx, 44(%esp)                  # 4-byte Spill
	adcl	932(%esp), %esi
	movl	%esi, 24(%esp)                  # 4-byte Spill
	adcl	936(%esp), %ebp
	movl	%ebp, 64(%esp)                  # 4-byte Spill
	movl	56(%esp), %ecx                  # 4-byte Reload
	adcl	940(%esp), %ecx
	movl	%ecx, 56(%esp)                  # 4-byte Spill
	movl	32(%esp), %ecx                  # 4-byte Reload
	adcl	944(%esp), %ecx
	movl	%ecx, 32(%esp)                  # 4-byte Spill
	movl	60(%esp), %ecx                  # 4-byte Reload
	adcl	948(%esp), %ecx
	movl	%ecx, 60(%esp)                  # 4-byte Spill
	movl	28(%esp), %ecx                  # 4-byte Reload
	adcl	952(%esp), %ecx
	movl	%ecx, 28(%esp)                  # 4-byte Spill
	adcl	956(%esp), %edi
	movl	%edi, 16(%esp)                  # 4-byte Spill
	movl	8(%esp), %ecx                   # 4-byte Reload
	adcl	960(%esp), %ecx
	movl	%ecx, 8(%esp)                   # 4-byte Spill
	movl	12(%esp), %ecx                  # 4-byte Reload
	adcl	964(%esp), %ecx
	movl	%ecx, 12(%esp)                  # 4-byte Spill
	adcl	$0, %eax
	movl	%eax, 4(%esp)                   # 4-byte Spill
	subl	$4, %esp
	movl	72(%esp), %ecx                  # 4-byte Reload
	imull	%edx, %ecx
	movl	%edx, %esi
	pushl	%ecx
	pushl	2452(%esp)
	leal	844(%esp), %eax
	pushl	%eax
	calll	mulPv512x32@PLT
	addl	$12, %esp
	addl	832(%esp), %esi
	movl	48(%esp), %eax                  # 4-byte Reload
	adcl	836(%esp), %eax
	movl	%eax, 48(%esp)                  # 4-byte Spill
	movl	36(%esp), %eax                  # 4-byte Reload
	adcl	840(%esp), %eax
	movl	%eax, 36(%esp)                  # 4-byte Spill
	movl	20(%esp), %eax                  # 4-byte Reload
	adcl	844(%esp), %eax
	movl	%eax, 20(%esp)                  # 4-byte Spill
	movl	52(%esp), %ebp                  # 4-byte Reload
	adcl	848(%esp), %ebp
	movl	40(%esp), %eax                  # 4-byte Reload
	adcl	852(%esp), %eax
	movl	%eax, 40(%esp)                  # 4-byte Spill
	movl	44(%esp), %edi                  # 4-byte Reload
	adcl	856(%esp), %edi
	movl	24(%esp), %eax                  # 4-byte Reload
	adcl	860(%esp), %eax
	movl	%eax, 24(%esp)                  # 4-byte Spill
	movl	64(%esp), %esi                  # 4-byte Reload
	adcl	864(%esp), %esi
	movl	56(%esp), %eax                  # 4-byte Reload
	adcl	868(%esp), %eax
	movl	%eax, 56(%esp)                  # 4-byte Spill
	movl	32(%esp), %eax                  # 4-byte Reload
	adcl	872(%esp), %eax
	movl	%eax, 32(%esp)                  # 4-byte Spill
	movl	60(%esp), %eax                  # 4-byte Reload
	adcl	876(%esp), %eax
	movl	%eax, 60(%esp)                  # 4-byte Spill
	movl	28(%esp), %eax                  # 4-byte Reload
	adcl	880(%esp), %eax
	movl	%eax, 28(%esp)                  # 4-byte Spill
	movl	16(%esp), %eax                  # 4-byte Reload
	adcl	884(%esp), %eax
	movl	%eax, 16(%esp)                  # 4-byte Spill
	movl	8(%esp), %eax                   # 4-byte Reload
	adcl	888(%esp), %eax
	movl	%eax, 8(%esp)                   # 4-byte Spill
	movl	12(%esp), %eax                  # 4-byte Reload
	adcl	892(%esp), %eax
	movl	%eax, 12(%esp)                  # 4-byte Spill
	movl	4(%esp), %eax                   # 4-byte Reload
	adcl	896(%esp), %eax
	movl	%eax, 4(%esp)                   # 4-byte Spill
	subl	$4, %esp
	movl	2444(%esp), %eax
	pushl	44(%eax)
	pushl	2444(%esp)
	leal	772(%esp), %eax
	pushl	%eax
	calll	mulPv512x32@PLT
	addl	$12, %esp
	movl	824(%esp), %eax
	movl	48(%esp), %edx                  # 4-byte Reload
	addl	760(%esp), %edx
	movl	36(%esp), %ecx                  # 4-byte Reload
	adcl	764(%esp), %ecx
	movl	%ecx, 36(%esp)                  # 4-byte Spill
	movl	20(%esp), %ecx                  # 4-byte Reload
	adcl	768(%esp), %ecx
	movl	%ecx, 20(%esp)                  # 4-byte Spill
	adcl	772(%esp), %ebp
	movl	%ebp, 52(%esp)                  # 4-byte Spill
	movl	40(%esp), %ecx                  # 4-byte Reload
	adcl	776(%esp), %ecx
	movl	%ecx, 40(%esp)                  # 4-byte Spill
	adcl	780(%esp), %edi
	movl	%edi, 44(%esp)                  # 4-byte Spill
	movl	24(%esp), %ecx                  # 4-byte Reload
	adcl	784(%esp), %ecx
	movl	%ecx, 24(%esp)                  # 4-byte Spill
	adcl	788(%esp), %esi
	movl	%esi, 64(%esp)                  # 4-byte Spill
	movl	56(%esp), %ecx                  # 4-byte Reload
	adcl	792(%esp), %ecx
	movl	%ecx, 56(%esp)                  # 4-byte Spill
	movl	32(%esp), %ebp                  # 4-byte Reload
	adcl	796(%esp), %ebp
	movl	60(%esp), %ecx                  # 4-byte Reload
	adcl	800(%esp), %ecx
	movl	%ecx, 60(%esp)                  # 4-byte Spill
	movl	28(%esp), %edi                  # 4-byte Reload
	adcl	804(%esp), %edi
	movl	16(%esp), %ecx                  # 4-byte Reload
	adcl	808(%esp), %ecx
	movl	%ecx, 16(%esp)                  # 4-byte Spill
	movl	8(%esp), %ecx                   # 4-byte Reload
	adcl	812(%esp), %ecx
	movl	%ecx, 8(%esp)                   # 4-byte Spill
	movl	12(%esp), %ecx                  # 4-byte Reload
	adcl	816(%esp), %ecx
	movl	%ecx, 12(%esp)                  # 4-byte Spill
	movl	4(%esp), %ecx                   # 4-byte Reload
	adcl	820(%esp), %ecx
	movl	%ecx, 4(%esp)                   # 4-byte Spill
	adcl	$0, %eax
	movl	%eax, 48(%esp)                  # 4-byte Spill
	subl	$4, %esp
	movl	72(%esp), %ecx                  # 4-byte Reload
	imull	%edx, %ecx
	movl	%edx, %esi
	pushl	%ecx
	pushl	2452(%esp)
	leal	700(%esp), %eax
	pushl	%eax
	calll	mulPv512x32@PLT
	addl	$12, %esp
	addl	688(%esp), %esi
	movl	36(%esp), %eax                  # 4-byte Reload
	adcl	692(%esp), %eax
	movl	%eax, 36(%esp)                  # 4-byte Spill
	movl	20(%esp), %eax                  # 4-byte Reload
	adcl	696(%esp), %eax
	movl	%eax, 20(%esp)                  # 4-byte Spill
	movl	52(%esp), %eax                  # 4-byte Reload
	adcl	700(%esp), %eax
	movl	%eax, 52(%esp)                  # 4-byte Spill
	movl	40(%esp), %eax                  # 4-byte Reload
	adcl	704(%esp), %eax
	movl	%eax, 40(%esp)                  # 4-byte Spill
	movl	44(%esp), %eax                  # 4-byte Reload
	adcl	708(%esp), %eax
	movl	%eax, 44(%esp)                  # 4-byte Spill
	movl	24(%esp), %eax                  # 4-byte Reload
	adcl	712(%esp), %eax
	movl	%eax, 24(%esp)                  # 4-byte Spill
	movl	64(%esp), %eax                  # 4-byte Reload
	adcl	716(%esp), %eax
	movl	%eax, 64(%esp)                  # 4-byte Spill
	movl	56(%esp), %esi                  # 4-byte Reload
	adcl	720(%esp), %esi
	adcl	724(%esp), %ebp
	movl	%ebp, 32(%esp)                  # 4-byte Spill
	movl	60(%esp), %eax                  # 4-byte Reload
	adcl	728(%esp), %eax
	movl	%eax, 60(%esp)                  # 4-byte Spill
	adcl	732(%esp), %edi
	movl	%edi, 28(%esp)                  # 4-byte Spill
	movl	16(%esp), %edi                  # 4-byte Reload
	adcl	736(%esp), %edi
	movl	8(%esp), %ebp                   # 4-byte Reload
	adcl	740(%esp), %ebp
	movl	12(%esp), %eax                  # 4-byte Reload
	adcl	744(%esp), %eax
	movl	%eax, 12(%esp)                  # 4-byte Spill
	movl	4(%esp), %eax                   # 4-byte Reload
	adcl	748(%esp), %eax
	movl	%eax, 4(%esp)                   # 4-byte Spill
	movl	48(%esp), %eax                  # 4-byte Reload
	adcl	752(%esp), %eax
	movl	%eax, 48(%esp)                  # 4-byte Spill
	subl	$4, %esp
	movl	2444(%esp), %eax
	pushl	48(%eax)
	pushl	2444(%esp)
	leal	628(%esp), %eax
	pushl	%eax
	calll	mulPv512x32@PLT
	addl	$12, %esp
	movl	680(%esp), %eax
	movl	36(%esp), %edx                  # 4-byte Reload
	addl	616(%esp), %edx
	movl	20(%esp), %ecx                  # 4-byte Reload
	adcl	620(%esp), %ecx
	movl	%ecx, 20(%esp)                  # 4-byte Spill
	movl	52(%esp), %ecx                  # 4-byte Reload
	adcl	624(%esp), %ecx
	movl	%ecx, 52(%esp)                  # 4-byte Spill
	movl	40(%esp), %ecx                  # 4-byte Reload
	adcl	628(%esp), %ecx
	movl	%ecx, 40(%esp)                  # 4-byte Spill
	movl	44(%esp), %ecx                  # 4-byte Reload
	adcl	632(%esp), %ecx
	movl	%ecx, 44(%esp)                  # 4-byte Spill
	movl	24(%esp), %ecx                  # 4-byte Reload
	adcl	636(%esp), %ecx
	movl	%ecx, 24(%esp)                  # 4-byte Spill
	movl	64(%esp), %ecx                  # 4-byte Reload
	adcl	640(%esp), %ecx
	movl	%ecx, 64(%esp)                  # 4-byte Spill
	adcl	644(%esp), %esi
	movl	%esi, 56(%esp)                  # 4-byte Spill
	movl	32(%esp), %ecx                  # 4-byte Reload
	adcl	648(%esp), %ecx
	movl	%ecx, 32(%esp)                  # 4-byte Spill
	movl	60(%esp), %ecx                  # 4-byte Reload
	adcl	652(%esp), %ecx
	movl	%ecx, 60(%esp)                  # 4-byte Spill
	movl	28(%esp), %ecx                  # 4-byte Reload
	adcl	656(%esp), %ecx
	movl	%ecx, 28(%esp)                  # 4-byte Spill
	adcl	660(%esp), %edi
	movl	%edi, 16(%esp)                  # 4-byte Spill
	adcl	664(%esp), %ebp
	movl	%ebp, 8(%esp)                   # 4-byte Spill
	movl	12(%esp), %ecx                  # 4-byte Reload
	adcl	668(%esp), %ecx
	movl	%ecx, 12(%esp)                  # 4-byte Spill
	movl	4(%esp), %ebp                   # 4-byte Reload
	adcl	672(%esp), %ebp
	movl	48(%esp), %ecx                  # 4-byte Reload
	adcl	676(%esp), %ecx
	movl	%ecx, 48(%esp)                  # 4-byte Spill
	adcl	$0, %eax
	movl	%eax, 36(%esp)                  # 4-byte Spill
	subl	$4, %esp
	movl	72(%esp), %ecx                  # 4-byte Reload
	imull	%edx, %ecx
	movl	%edx, %esi
	pushl	%ecx
	pushl	2452(%esp)
	leal	556(%esp), %eax
	pushl	%eax
	calll	mulPv512x32@PLT
	addl	$12, %esp
	addl	544(%esp), %esi
	movl	20(%esp), %eax                  # 4-byte Reload
	adcl	548(%esp), %eax
	movl	%eax, 20(%esp)                  # 4-byte Spill
	movl	52(%esp), %edi                  # 4-byte Reload
	adcl	552(%esp), %edi
	movl	40(%esp), %esi                  # 4-byte Reload
	adcl	556(%esp), %esi
	movl	44(%esp), %eax                  # 4-byte Reload
	adcl	560(%esp), %eax
	movl	%eax, 44(%esp)                  # 4-byte Spill
	movl	24(%esp), %eax                  # 4-byte Reload
	adcl	564(%esp), %eax
	movl	%eax, 24(%esp)                  # 4-byte Spill
	movl	64(%esp), %eax                  # 4-byte Reload
	adcl	568(%esp), %eax
	movl	%eax, 64(%esp)                  # 4-byte Spill
	movl	56(%esp), %eax                  # 4-byte Reload
	adcl	572(%esp), %eax
	movl	%eax, 56(%esp)                  # 4-byte Spill
	movl	32(%esp), %eax                  # 4-byte Reload
	adcl	576(%esp), %eax
	movl	%eax, 32(%esp)                  # 4-byte Spill
	movl	60(%esp), %eax                  # 4-byte Reload
	adcl	580(%esp), %eax
	movl	%eax, 60(%esp)                  # 4-byte Spill
	movl	28(%esp), %eax                  # 4-byte Reload
	adcl	584(%esp), %eax
	movl	%eax, 28(%esp)                  # 4-byte Spill
	movl	16(%esp), %eax                  # 4-byte Reload
	adcl	588(%esp), %eax
	movl	%eax, 16(%esp)                  # 4-byte Spill
	movl	8(%esp), %eax                   # 4-byte Reload
	adcl	592(%esp), %eax
	movl	%eax, 8(%esp)                   # 4-byte Spill
	movl	12(%esp), %eax                  # 4-byte Reload
	adcl	596(%esp), %eax
	movl	%eax, 12(%esp)                  # 4-byte Spill
	adcl	600(%esp), %ebp
	movl	%ebp, 4(%esp)                   # 4-byte Spill
	movl	48(%esp), %eax                  # 4-byte Reload
	adcl	604(%esp), %eax
	movl	%eax, 48(%esp)                  # 4-byte Spill
	movl	36(%esp), %eax                  # 4-byte Reload
	adcl	608(%esp), %eax
	movl	%eax, 36(%esp)                  # 4-byte Spill
	subl	$4, %esp
	movl	2444(%esp), %eax
	pushl	52(%eax)
	pushl	2444(%esp)
	leal	484(%esp), %eax
	pushl	%eax
	calll	mulPv512x32@PLT
	addl	$12, %esp
	movl	536(%esp), %eax
	movl	20(%esp), %edx                  # 4-byte Reload
	addl	472(%esp), %edx
	adcl	476(%esp), %edi
	movl	%edi, 52(%esp)                  # 4-byte Spill
	adcl	480(%esp), %esi
	movl	%esi, 40(%esp)                  # 4-byte Spill
	movl	44(%esp), %ecx                  # 4-byte Reload
	adcl	484(%esp), %ecx
	movl	%ecx, 44(%esp)                  # 4-byte Spill
	movl	24(%esp), %ecx                  # 4-byte Reload
	adcl	488(%esp), %ecx
	movl	%ecx, 24(%esp)                  # 4-byte Spill
	movl	64(%esp), %ecx                  # 4-byte Reload
	adcl	492(%esp), %ecx
	movl	%ecx, 64(%esp)                  # 4-byte Spill
	movl	56(%esp), %ecx                  # 4-byte Reload
	adcl	496(%esp), %ecx
	movl	%ecx, 56(%esp)                  # 4-byte Spill
	movl	32(%esp), %ecx                  # 4-byte Reload
	adcl	500(%esp), %ecx
	movl	%ecx, 32(%esp)                  # 4-byte Spill
	movl	60(%esp), %ebp                  # 4-byte Reload
	adcl	504(%esp), %ebp
	movl	28(%esp), %edi                  # 4-byte Reload
	adcl	508(%esp), %edi
	movl	16(%esp), %ecx                  # 4-byte Reload
	adcl	512(%esp), %ecx
	movl	%ecx, 16(%esp)                  # 4-byte Spill
	movl	8(%esp), %ecx                   # 4-byte Reload
	adcl	516(%esp), %ecx
	movl	%ecx, 8(%esp)                   # 4-byte Spill
	movl	12(%esp), %ecx                  # 4-byte Reload
	adcl	520(%esp), %ecx
	movl	%ecx, 12(%esp)                  # 4-byte Spill
	movl	4(%esp), %ecx                   # 4-byte Reload
	adcl	524(%esp), %ecx
	movl	%ecx, 4(%esp)                   # 4-byte Spill
	movl	48(%esp), %ecx                  # 4-byte Reload
	adcl	528(%esp), %ecx
	movl	%ecx, 48(%esp)                  # 4-byte Spill
	movl	36(%esp), %ecx                  # 4-byte Reload
	adcl	532(%esp), %ecx
	movl	%ecx, 36(%esp)                  # 4-byte Spill
	adcl	$0, %eax
	movl	%eax, 20(%esp)                  # 4-byte Spill
	subl	$4, %esp
	movl	72(%esp), %ecx                  # 4-byte Reload
	imull	%edx, %ecx
	movl	%edx, %esi
	pushl	%ecx
	pushl	2452(%esp)
	leal	412(%esp), %eax
	pushl	%eax
	calll	mulPv512x32@PLT
	addl	$12, %esp
	addl	400(%esp), %esi
	movl	52(%esp), %eax                  # 4-byte Reload
	adcl	404(%esp), %eax
	movl	%eax, 52(%esp)                  # 4-byte Spill
	movl	40(%esp), %eax                  # 4-byte Reload
	adcl	408(%esp), %eax
	movl	%eax, 40(%esp)                  # 4-byte Spill
	movl	44(%esp), %esi                  # 4-byte Reload
	adcl	412(%esp), %esi
	movl	24(%esp), %eax                  # 4-byte Reload
	adcl	416(%esp), %eax
	movl	%eax, 24(%esp)                  # 4-byte Spill
	movl	64(%esp), %eax                  # 4-byte Reload
	adcl	420(%esp), %eax
	movl	%eax, 64(%esp)                  # 4-byte Spill
	movl	56(%esp), %eax                  # 4-byte Reload
	adcl	424(%esp), %eax
	movl	%eax, 56(%esp)                  # 4-byte Spill
	movl	32(%esp), %eax                  # 4-byte Reload
	adcl	428(%esp), %eax
	movl	%eax, 32(%esp)                  # 4-byte Spill
	adcl	432(%esp), %ebp
	movl	%ebp, 60(%esp)                  # 4-byte Spill
	adcl	436(%esp), %edi
	movl	%edi, 28(%esp)                  # 4-byte Spill
	movl	16(%esp), %eax                  # 4-byte Reload
	adcl	440(%esp), %eax
	movl	%eax, 16(%esp)                  # 4-byte Spill
	movl	8(%esp), %eax                   # 4-byte Reload
	adcl	444(%esp), %eax
	movl	%eax, 8(%esp)                   # 4-byte Spill
	movl	12(%esp), %edi                  # 4-byte Reload
	adcl	448(%esp), %edi
	movl	4(%esp), %eax                   # 4-byte Reload
	adcl	452(%esp), %eax
	movl	%eax, 4(%esp)                   # 4-byte Spill
	movl	48(%esp), %ebp                  # 4-byte Reload
	adcl	456(%esp), %ebp
	movl	36(%esp), %eax                  # 4-byte Reload
	adcl	460(%esp), %eax
	movl	%eax, 36(%esp)                  # 4-byte Spill
	movl	20(%esp), %eax                  # 4-byte Reload
	adcl	464(%esp), %eax
	movl	%eax, 20(%esp)                  # 4-byte Spill
	subl	$4, %esp
	leal	332(%esp), %eax
	movl	2444(%esp), %ecx
	pushl	56(%ecx)
	pushl	2444(%esp)
	pushl	%eax
	calll	mulPv512x32@PLT
	addl	$12, %esp
	movl	52(%esp), %edx                  # 4-byte Reload
	addl	328(%esp), %edx
	movl	40(%esp), %eax                  # 4-byte Reload
	adcl	332(%esp), %eax
	movl	%eax, 40(%esp)                  # 4-byte Spill
	adcl	336(%esp), %esi
	movl	%esi, 44(%esp)                  # 4-byte Spill
	movl	24(%esp), %eax                  # 4-byte Reload
	adcl	340(%esp), %eax
	movl	%eax, 24(%esp)                  # 4-byte Spill
	movl	64(%esp), %eax                  # 4-byte Reload
	adcl	344(%esp), %eax
	movl	%eax, 64(%esp)                  # 4-byte Spill
	movl	56(%esp), %eax                  # 4-byte Reload
	adcl	348(%esp), %eax
	movl	%eax, 56(%esp)                  # 4-byte Spill
	movl	32(%esp), %eax                  # 4-byte Reload
	adcl	352(%esp), %eax
	movl	%eax, 32(%esp)                  # 4-byte Spill
	movl	60(%esp), %eax                  # 4-byte Reload
	adcl	356(%esp), %eax
	movl	%eax, 60(%esp)                  # 4-byte Spill
	movl	28(%esp), %eax                  # 4-byte Reload
	adcl	360(%esp), %eax
	movl	%eax, 28(%esp)                  # 4-byte Spill
	movl	16(%esp), %eax                  # 4-byte Reload
	adcl	364(%esp), %eax
	movl	%eax, 16(%esp)                  # 4-byte Spill
	movl	8(%esp), %eax                   # 4-byte Reload
	adcl	368(%esp), %eax
	movl	%eax, 8(%esp)                   # 4-byte Spill
	adcl	372(%esp), %edi
	movl	%edi, 12(%esp)                  # 4-byte Spill
	movl	4(%esp), %eax                   # 4-byte Reload
	adcl	376(%esp), %eax
	movl	%eax, 4(%esp)                   # 4-byte Spill
	adcl	380(%esp), %ebp
	movl	%ebp, 48(%esp)                  # 4-byte Spill
	movl	36(%esp), %eax                  # 4-byte Reload
	adcl	384(%esp), %eax
	movl	%eax, 36(%esp)                  # 4-byte Spill
	movl	20(%esp), %eax                  # 4-byte Reload
	adcl	388(%esp), %eax
	movl	%eax, 20(%esp)                  # 4-byte Spill
	movl	392(%esp), %eax
	adcl	$0, %eax
	movl	%eax, 52(%esp)                  # 4-byte Spill
	subl	$4, %esp
	leal	260(%esp), %eax
	movl	72(%esp), %ecx                  # 4-byte Reload
	imull	%edx, %ecx
	movl	%edx, %esi
	pushl	%ecx
	pushl	2452(%esp)
	pushl	%eax
	calll	mulPv512x32@PLT
	addl	$12, %esp
	addl	256(%esp), %esi
	movl	40(%esp), %eax                  # 4-byte Reload
	adcl	260(%esp), %eax
	movl	%eax, 40(%esp)                  # 4-byte Spill
	movl	44(%esp), %eax                  # 4-byte Reload
	adcl	264(%esp), %eax
	movl	%eax, 44(%esp)                  # 4-byte Spill
	movl	24(%esp), %esi                  # 4-byte Reload
	adcl	268(%esp), %esi
	movl	64(%esp), %eax                  # 4-byte Reload
	adcl	272(%esp), %eax
	movl	%eax, 64(%esp)                  # 4-byte Spill
	movl	56(%esp), %eax                  # 4-byte Reload
	adcl	276(%esp), %eax
	movl	%eax, 56(%esp)                  # 4-byte Spill
	movl	32(%esp), %ebp                  # 4-byte Reload
	adcl	280(%esp), %ebp
	movl	60(%esp), %edi                  # 4-byte Reload
	adcl	284(%esp), %edi
	movl	28(%esp), %eax                  # 4-byte Reload
	adcl	288(%esp), %eax
	movl	%eax, 28(%esp)                  # 4-byte Spill
	movl	16(%esp), %eax                  # 4-byte Reload
	adcl	292(%esp), %eax
	movl	%eax, 16(%esp)                  # 4-byte Spill
	movl	8(%esp), %eax                   # 4-byte Reload
	adcl	296(%esp), %eax
	movl	%eax, 8(%esp)                   # 4-byte Spill
	movl	12(%esp), %eax                  # 4-byte Reload
	adcl	300(%esp), %eax
	movl	%eax, 12(%esp)                  # 4-byte Spill
	movl	4(%esp), %eax                   # 4-byte Reload
	adcl	304(%esp), %eax
	movl	%eax, 4(%esp)                   # 4-byte Spill
	movl	48(%esp), %eax                  # 4-byte Reload
	adcl	308(%esp), %eax
	movl	%eax, 48(%esp)                  # 4-byte Spill
	movl	36(%esp), %eax                  # 4-byte Reload
	adcl	312(%esp), %eax
	movl	%eax, 36(%esp)                  # 4-byte Spill
	movl	20(%esp), %eax                  # 4-byte Reload
	adcl	316(%esp), %eax
	movl	%eax, 20(%esp)                  # 4-byte Spill
	movl	52(%esp), %eax                  # 4-byte Reload
	adcl	320(%esp), %eax
	movl	%eax, 52(%esp)                  # 4-byte Spill
	subl	$4, %esp
	leal	188(%esp), %eax
	movl	2444(%esp), %ecx
	pushl	60(%ecx)
	pushl	2444(%esp)
	pushl	%eax
	calll	mulPv512x32@PLT
	addl	$12, %esp
	movl	40(%esp), %edx                  # 4-byte Reload
	addl	184(%esp), %edx
	movl	44(%esp), %eax                  # 4-byte Reload
	adcl	188(%esp), %eax
	movl	%eax, 44(%esp)                  # 4-byte Spill
	adcl	192(%esp), %esi
	movl	%esi, 24(%esp)                  # 4-byte Spill
	movl	64(%esp), %eax                  # 4-byte Reload
	adcl	196(%esp), %eax
	movl	%eax, 64(%esp)                  # 4-byte Spill
	movl	56(%esp), %eax                  # 4-byte Reload
	adcl	200(%esp), %eax
	movl	%eax, 56(%esp)                  # 4-byte Spill
	adcl	204(%esp), %ebp
	movl	%ebp, 32(%esp)                  # 4-byte Spill
	adcl	208(%esp), %edi
	movl	%edi, 60(%esp)                  # 4-byte Spill
	movl	28(%esp), %eax                  # 4-byte Reload
	adcl	212(%esp), %eax
	movl	%eax, 28(%esp)                  # 4-byte Spill
	movl	16(%esp), %eax                  # 4-byte Reload
	adcl	216(%esp), %eax
	movl	%eax, 16(%esp)                  # 4-byte Spill
	movl	8(%esp), %eax                   # 4-byte Reload
	adcl	220(%esp), %eax
	movl	%eax, 8(%esp)                   # 4-byte Spill
	movl	12(%esp), %eax                  # 4-byte Reload
	adcl	224(%esp), %eax
	movl	%eax, 12(%esp)                  # 4-byte Spill
	movl	4(%esp), %eax                   # 4-byte Reload
	adcl	228(%esp), %eax
	movl	%eax, 4(%esp)                   # 4-byte Spill
	movl	48(%esp), %ebp                  # 4-byte Reload
	adcl	232(%esp), %ebp
	movl	36(%esp), %eax                  # 4-byte Reload
	adcl	236(%esp), %eax
	movl	%eax, 36(%esp)                  # 4-byte Spill
	movl	20(%esp), %eax                  # 4-byte Reload
	adcl	240(%esp), %eax
	movl	%eax, 20(%esp)                  # 4-byte Spill
	movl	52(%esp), %eax                  # 4-byte Reload
	adcl	244(%esp), %eax
	movl	%eax, 52(%esp)                  # 4-byte Spill
	movl	248(%esp), %eax
	adcl	$0, %eax
	movl	%eax, 40(%esp)                  # 4-byte Spill
	subl	$4, %esp
	leal	116(%esp), %eax
	movl	72(%esp), %ecx                  # 4-byte Reload
	imull	%edx, %ecx
	movl	%edx, %esi
	pushl	%ecx
	pushl	2452(%esp)
	pushl	%eax
	calll	mulPv512x32@PLT
	addl	$12, %esp
	addl	112(%esp), %esi
	movl	64(%esp), %edx                  # 4-byte Reload
	movl	44(%esp), %eax                  # 4-byte Reload
	adcl	116(%esp), %eax
	movl	24(%esp), %ecx                  # 4-byte Reload
	adcl	120(%esp), %ecx
	movl	%ecx, 24(%esp)                  # 4-byte Spill
	adcl	124(%esp), %edx
	movl	56(%esp), %edi                  # 4-byte Reload
	adcl	128(%esp), %edi
	movl	32(%esp), %esi                  # 4-byte Reload
	adcl	132(%esp), %esi
	movl	60(%esp), %ebx                  # 4-byte Reload
	adcl	136(%esp), %ebx
	movl	28(%esp), %ecx                  # 4-byte Reload
	adcl	140(%esp), %ecx
	movl	%ecx, 28(%esp)                  # 4-byte Spill
	movl	16(%esp), %ecx                  # 4-byte Reload
	adcl	144(%esp), %ecx
	movl	%ecx, 16(%esp)                  # 4-byte Spill
	movl	8(%esp), %ecx                   # 4-byte Reload
	adcl	148(%esp), %ecx
	movl	%ecx, 8(%esp)                   # 4-byte Spill
	movl	12(%esp), %ecx                  # 4-byte Reload
	adcl	152(%esp), %ecx
	movl	%ecx, 12(%esp)                  # 4-byte Spill
	movl	4(%esp), %ecx                   # 4-byte Reload
	adcl	156(%esp), %ecx
	movl	%ecx, 4(%esp)                   # 4-byte Spill
	adcl	160(%esp), %ebp
	movl	%ebp, 48(%esp)                  # 4-byte Spill
	movl	36(%esp), %ebp                  # 4-byte Reload
	adcl	164(%esp), %ebp
	movl	20(%esp), %ecx                  # 4-byte Reload
	adcl	168(%esp), %ecx
	movl	%ecx, 20(%esp)                  # 4-byte Spill
	movl	52(%esp), %ecx                  # 4-byte Reload
	adcl	172(%esp), %ecx
	movl	%ecx, 52(%esp)                  # 4-byte Spill
	movl	40(%esp), %ecx                  # 4-byte Reload
	adcl	176(%esp), %ecx
	movl	%ecx, 40(%esp)                  # 4-byte Spill
	movl	%eax, 44(%esp)                  # 4-byte Spill
	movl	2444(%esp), %ecx
	subl	(%ecx), %eax
	movl	%eax, 108(%esp)                 # 4-byte Spill
	movl	24(%esp), %eax                  # 4-byte Reload
	sbbl	4(%ecx), %eax
	movl	%eax, 104(%esp)                 # 4-byte Spill
	movl	%edx, 64(%esp)                  # 4-byte Spill
	sbbl	8(%ecx), %edx
	movl	%edx, 100(%esp)                 # 4-byte Spill
	movl	%edi, 56(%esp)                  # 4-byte Spill
	sbbl	12(%ecx), %edi
	movl	%edi, 96(%esp)                  # 4-byte Spill
	movl	%esi, 32(%esp)                  # 4-byte Spill
	sbbl	16(%ecx), %esi
	movl	%esi, 92(%esp)                  # 4-byte Spill
	movl	%ebx, %eax
	movl	%ebx, 60(%esp)                  # 4-byte Spill
	sbbl	20(%ecx), %eax
	movl	%eax, 88(%esp)                  # 4-byte Spill
	movl	28(%esp), %eax                  # 4-byte Reload
	sbbl	24(%ecx), %eax
	movl	%eax, 68(%esp)                  # 4-byte Spill
	movl	16(%esp), %eax                  # 4-byte Reload
	sbbl	28(%ecx), %eax
	movl	%eax, 84(%esp)                  # 4-byte Spill
	movl	8(%esp), %eax                   # 4-byte Reload
	sbbl	32(%ecx), %eax
	movl	%eax, 76(%esp)                  # 4-byte Spill
	movl	12(%esp), %eax                  # 4-byte Reload
	sbbl	36(%ecx), %eax
	movl	%eax, 72(%esp)                  # 4-byte Spill
	movl	4(%esp), %eax                   # 4-byte Reload
	sbbl	40(%ecx), %eax
	movl	%eax, 80(%esp)                  # 4-byte Spill
	movl	48(%esp), %eax                  # 4-byte Reload
	sbbl	44(%ecx), %eax
	movl	%ebp, 36(%esp)                  # 4-byte Spill
	movl	%ebp, %edx
	sbbl	48(%ecx), %edx
	movl	20(%esp), %ebp                  # 4-byte Reload
	sbbl	52(%ecx), %ebp
	movl	52(%esp), %ebx                  # 4-byte Reload
	sbbl	56(%ecx), %ebx
	movl	40(%esp), %edi                  # 4-byte Reload
	movl	%edi, %esi
	sbbl	60(%ecx), %edi
	testl	%edi, %edi
	js	.LBB78_1
# %bb.2:
	movl	2432(%esp), %ecx
	movl	%edi, 60(%ecx)
	js	.LBB78_3
.LBB78_4:
	movl	%ebx, 56(%ecx)
	movl	96(%esp), %edi                  # 4-byte Reload
	movl	84(%esp), %esi                  # 4-byte Reload
	js	.LBB78_5
.LBB78_6:
	movl	%ebp, 52(%ecx)
	js	.LBB78_7
.LBB78_8:
	movl	%edx, 48(%ecx)
	movl	88(%esp), %ebp                  # 4-byte Reload
	js	.LBB78_9
.LBB78_10:
	movl	%eax, 44(%ecx)
	movl	104(%esp), %ebx                 # 4-byte Reload
	movl	80(%esp), %eax                  # 4-byte Reload
	js	.LBB78_11
.LBB78_12:
	movl	%eax, 40(%ecx)
	movl	108(%esp), %edx                 # 4-byte Reload
	js	.LBB78_13
.LBB78_14:
	movl	%edi, %eax
	movl	72(%esp), %edi                  # 4-byte Reload
	movl	%edi, 36(%ecx)
	js	.LBB78_15
.LBB78_16:
	movl	76(%esp), %edi                  # 4-byte Reload
	movl	%edi, 32(%ecx)
	js	.LBB78_17
.LBB78_18:
	movl	%esi, 28(%ecx)
	movl	%eax, %edi
	js	.LBB78_19
.LBB78_20:
	movl	68(%esp), %esi                  # 4-byte Reload
	movl	%esi, 24(%ecx)
	movl	%edx, %eax
	js	.LBB78_21
.LBB78_22:
	movl	%ebp, 20(%ecx)
	movl	100(%esp), %esi                 # 4-byte Reload
	movl	92(%esp), %edx                  # 4-byte Reload
	js	.LBB78_23
.LBB78_24:
	movl	%edx, 16(%ecx)
	js	.LBB78_25
.LBB78_26:
	movl	%edi, 12(%ecx)
	js	.LBB78_27
.LBB78_28:
	movl	%esi, 8(%ecx)
	js	.LBB78_29
.LBB78_30:
	movl	%ebx, 4(%ecx)
	jns	.LBB78_32
.LBB78_31:
	movl	44(%esp), %eax                  # 4-byte Reload
.LBB78_32:
	movl	%eax, (%ecx)
	addl	$2412, %esp                     # imm = 0x96C
	popl	%esi
	popl	%edi
	popl	%ebx
	popl	%ebp
	retl
.LBB78_1:
	movl	%esi, %edi
	movl	2432(%esp), %ecx
	movl	%edi, 60(%ecx)
	jns	.LBB78_4
.LBB78_3:
	movl	52(%esp), %ebx                  # 4-byte Reload
	movl	%ebx, 56(%ecx)
	movl	96(%esp), %edi                  # 4-byte Reload
	movl	84(%esp), %esi                  # 4-byte Reload
	jns	.LBB78_6
.LBB78_5:
	movl	20(%esp), %ebp                  # 4-byte Reload
	movl	%ebp, 52(%ecx)
	jns	.LBB78_8
.LBB78_7:
	movl	36(%esp), %edx                  # 4-byte Reload
	movl	%edx, 48(%ecx)
	movl	88(%esp), %ebp                  # 4-byte Reload
	jns	.LBB78_10
.LBB78_9:
	movl	48(%esp), %eax                  # 4-byte Reload
	movl	%eax, 44(%ecx)
	movl	104(%esp), %ebx                 # 4-byte Reload
	movl	80(%esp), %eax                  # 4-byte Reload
	jns	.LBB78_12
.LBB78_11:
	movl	4(%esp), %eax                   # 4-byte Reload
	movl	%eax, 40(%ecx)
	movl	108(%esp), %edx                 # 4-byte Reload
	jns	.LBB78_14
.LBB78_13:
	movl	12(%esp), %eax                  # 4-byte Reload
	movl	%eax, 72(%esp)                  # 4-byte Spill
	movl	%edi, %eax
	movl	72(%esp), %edi                  # 4-byte Reload
	movl	%edi, 36(%ecx)
	jns	.LBB78_16
.LBB78_15:
	movl	8(%esp), %edi                   # 4-byte Reload
	movl	%edi, 76(%esp)                  # 4-byte Spill
	movl	76(%esp), %edi                  # 4-byte Reload
	movl	%edi, 32(%ecx)
	jns	.LBB78_18
.LBB78_17:
	movl	16(%esp), %esi                  # 4-byte Reload
	movl	%esi, 28(%ecx)
	movl	%eax, %edi
	jns	.LBB78_20
.LBB78_19:
	movl	28(%esp), %esi                  # 4-byte Reload
	movl	%esi, 68(%esp)                  # 4-byte Spill
	movl	68(%esp), %esi                  # 4-byte Reload
	movl	%esi, 24(%ecx)
	movl	%edx, %eax
	jns	.LBB78_22
.LBB78_21:
	movl	60(%esp), %ebp                  # 4-byte Reload
	movl	%ebp, 20(%ecx)
	movl	100(%esp), %esi                 # 4-byte Reload
	movl	92(%esp), %edx                  # 4-byte Reload
	jns	.LBB78_24
.LBB78_23:
	movl	32(%esp), %edx                  # 4-byte Reload
	movl	%edx, 16(%ecx)
	jns	.LBB78_26
.LBB78_25:
	movl	56(%esp), %edi                  # 4-byte Reload
	movl	%edi, 12(%ecx)
	jns	.LBB78_28
.LBB78_27:
	movl	64(%esp), %esi                  # 4-byte Reload
	movl	%esi, 8(%ecx)
	jns	.LBB78_30
.LBB78_29:
	movl	24(%esp), %ebx                  # 4-byte Reload
	movl	%ebx, 4(%ecx)
	js	.LBB78_31
	jmp	.LBB78_32
.Lfunc_end78:
	.size	mcl_fp_montNF16L, .Lfunc_end78-mcl_fp_montNF16L
                                        # -- End function
	.globl	mcl_fp_montRed16L               # -- Begin function mcl_fp_montRed16L
	.p2align	4, 0x90
	.type	mcl_fp_montRed16L,@function
mcl_fp_montRed16L:                      # @mcl_fp_montRed16L
# %bb.0:
	pushl	%ebp
	pushl	%ebx
	pushl	%edi
	pushl	%esi
	subl	$1292, %esp                     # imm = 0x50C
	calll	.L79$pb
.L79$pb:
	popl	%ebx
.Ltmp21:
	addl	$_GLOBAL_OFFSET_TABLE_+(.Ltmp21-.L79$pb), %ebx
	movl	1320(%esp), %ecx
	movl	60(%ecx), %eax
	movl	%eax, 140(%esp)                 # 4-byte Spill
	movl	56(%ecx), %eax
	movl	%eax, 136(%esp)                 # 4-byte Spill
	movl	52(%ecx), %eax
	movl	%eax, 132(%esp)                 # 4-byte Spill
	movl	48(%ecx), %eax
	movl	%eax, 128(%esp)                 # 4-byte Spill
	movl	44(%ecx), %eax
	movl	%eax, 116(%esp)                 # 4-byte Spill
	movl	40(%ecx), %eax
	movl	%eax, 112(%esp)                 # 4-byte Spill
	movl	36(%ecx), %eax
	movl	%eax, 80(%esp)                  # 4-byte Spill
	movl	32(%ecx), %eax
	movl	%eax, 108(%esp)                 # 4-byte Spill
	movl	28(%ecx), %eax
	movl	%eax, 124(%esp)                 # 4-byte Spill
	movl	24(%ecx), %eax
	movl	%eax, 120(%esp)                 # 4-byte Spill
	movl	20(%ecx), %eax
	movl	%eax, 104(%esp)                 # 4-byte Spill
	movl	16(%ecx), %eax
	movl	%eax, 100(%esp)                 # 4-byte Spill
	movl	12(%ecx), %eax
	movl	%eax, 96(%esp)                  # 4-byte Spill
	movl	8(%ecx), %eax
	movl	%eax, 92(%esp)                  # 4-byte Spill
	movl	4(%ecx), %eax
	movl	%ecx, %edx
	movl	%eax, 88(%esp)                  # 4-byte Spill
	movl	1316(%esp), %eax
	movl	60(%eax), %ecx
	movl	%ecx, 44(%esp)                  # 4-byte Spill
	movl	56(%eax), %ecx
	movl	%ecx, 64(%esp)                  # 4-byte Spill
	movl	52(%eax), %ecx
	movl	%ecx, 16(%esp)                  # 4-byte Spill
	movl	48(%eax), %ecx
	movl	%ecx, 68(%esp)                  # 4-byte Spill
	movl	44(%eax), %ecx
	movl	%ecx, 32(%esp)                  # 4-byte Spill
	movl	40(%eax), %ecx
	movl	%ecx, 28(%esp)                  # 4-byte Spill
	movl	36(%eax), %ecx
	movl	%ecx, 12(%esp)                  # 4-byte Spill
	movl	32(%eax), %ecx
	movl	%ecx, 24(%esp)                  # 4-byte Spill
	movl	28(%eax), %ecx
	movl	%ecx, 48(%esp)                  # 4-byte Spill
	movl	24(%eax), %ecx
	movl	%ecx, 36(%esp)                  # 4-byte Spill
	movl	20(%eax), %ecx
	movl	%ecx, 40(%esp)                  # 4-byte Spill
	movl	16(%eax), %edi
	movl	12(%eax), %ecx
	movl	%ecx, 20(%esp)                  # 4-byte Spill
	movl	8(%eax), %ecx
	movl	%ecx, 56(%esp)                  # 4-byte Spill
	movl	(%eax), %esi
	movl	4(%eax), %ebp
	movl	-4(%edx), %ecx
	movl	%ecx, 72(%esp)                  # 4-byte Spill
	movl	(%edx), %eax
	movl	%eax, 84(%esp)                  # 4-byte Spill
	subl	$4, %esp
	movl	%esi, %eax
	imull	%ecx, %eax
	leal	1228(%esp), %ecx
	pushl	%eax
	pushl	%edx
	pushl	%ecx
	calll	mulPv512x32@PLT
	addl	$12, %esp
	addl	1224(%esp), %esi
	adcl	1228(%esp), %ebp
	movl	56(%esp), %eax                  # 4-byte Reload
	adcl	1232(%esp), %eax
	movl	%eax, 56(%esp)                  # 4-byte Spill
	movl	20(%esp), %eax                  # 4-byte Reload
	adcl	1236(%esp), %eax
	movl	%eax, 20(%esp)                  # 4-byte Spill
	adcl	1240(%esp), %edi
	movl	%edi, 52(%esp)                  # 4-byte Spill
	movl	40(%esp), %eax                  # 4-byte Reload
	adcl	1244(%esp), %eax
	movl	%eax, 40(%esp)                  # 4-byte Spill
	movl	36(%esp), %eax                  # 4-byte Reload
	adcl	1248(%esp), %eax
	movl	%eax, 36(%esp)                  # 4-byte Spill
	movl	48(%esp), %eax                  # 4-byte Reload
	adcl	1252(%esp), %eax
	movl	%eax, 48(%esp)                  # 4-byte Spill
	movl	24(%esp), %eax                  # 4-byte Reload
	adcl	1256(%esp), %eax
	movl	%eax, 24(%esp)                  # 4-byte Spill
	movl	12(%esp), %eax                  # 4-byte Reload
	adcl	1260(%esp), %eax
	movl	%eax, 12(%esp)                  # 4-byte Spill
	movl	28(%esp), %eax                  # 4-byte Reload
	adcl	1264(%esp), %eax
	movl	%eax, 28(%esp)                  # 4-byte Spill
	movl	32(%esp), %eax                  # 4-byte Reload
	adcl	1268(%esp), %eax
	movl	%eax, 32(%esp)                  # 4-byte Spill
	movl	68(%esp), %eax                  # 4-byte Reload
	adcl	1272(%esp), %eax
	movl	%eax, 68(%esp)                  # 4-byte Spill
	movl	16(%esp), %eax                  # 4-byte Reload
	adcl	1276(%esp), %eax
	movl	%eax, 16(%esp)                  # 4-byte Spill
	movl	64(%esp), %edi                  # 4-byte Reload
	adcl	1280(%esp), %edi
	movl	44(%esp), %esi                  # 4-byte Reload
	adcl	1284(%esp), %esi
	movl	1316(%esp), %eax
	movl	64(%eax), %eax
	adcl	1288(%esp), %eax
	movl	%eax, 60(%esp)                  # 4-byte Spill
	setb	44(%esp)                        # 1-byte Folded Spill
	subl	$4, %esp
	movl	76(%esp), %eax                  # 4-byte Reload
	imull	%ebp, %eax
	leal	1156(%esp), %ecx
	pushl	%eax
	pushl	1328(%esp)
	pushl	%ecx
	calll	mulPv512x32@PLT
	addl	$12, %esp
	addb	$255, 44(%esp)                  # 1-byte Folded Spill
	movl	1216(%esp), %eax
	adcl	$0, %eax
	addl	1152(%esp), %ebp
	movl	56(%esp), %edx                  # 4-byte Reload
	adcl	1156(%esp), %edx
	movl	20(%esp), %ebp                  # 4-byte Reload
	adcl	1160(%esp), %ebp
	movl	52(%esp), %ecx                  # 4-byte Reload
	adcl	1164(%esp), %ecx
	movl	%ecx, 52(%esp)                  # 4-byte Spill
	movl	40(%esp), %ecx                  # 4-byte Reload
	adcl	1168(%esp), %ecx
	movl	%ecx, 40(%esp)                  # 4-byte Spill
	movl	36(%esp), %ecx                  # 4-byte Reload
	adcl	1172(%esp), %ecx
	movl	%ecx, 36(%esp)                  # 4-byte Spill
	movl	48(%esp), %ecx                  # 4-byte Reload
	adcl	1176(%esp), %ecx
	movl	%ecx, 48(%esp)                  # 4-byte Spill
	movl	24(%esp), %ecx                  # 4-byte Reload
	adcl	1180(%esp), %ecx
	movl	%ecx, 24(%esp)                  # 4-byte Spill
	movl	12(%esp), %ecx                  # 4-byte Reload
	adcl	1184(%esp), %ecx
	movl	%ecx, 12(%esp)                  # 4-byte Spill
	movl	28(%esp), %ecx                  # 4-byte Reload
	adcl	1188(%esp), %ecx
	movl	%ecx, 28(%esp)                  # 4-byte Spill
	movl	32(%esp), %ecx                  # 4-byte Reload
	adcl	1192(%esp), %ecx
	movl	%ecx, 32(%esp)                  # 4-byte Spill
	movl	68(%esp), %ecx                  # 4-byte Reload
	adcl	1196(%esp), %ecx
	movl	%ecx, 68(%esp)                  # 4-byte Spill
	movl	16(%esp), %ecx                  # 4-byte Reload
	adcl	1200(%esp), %ecx
	movl	%ecx, 16(%esp)                  # 4-byte Spill
	adcl	1204(%esp), %edi
	movl	%edi, 64(%esp)                  # 4-byte Spill
	adcl	1208(%esp), %esi
	movl	%esi, 44(%esp)                  # 4-byte Spill
	movl	60(%esp), %esi                  # 4-byte Reload
	adcl	1212(%esp), %esi
	movl	1316(%esp), %ecx
	adcl	68(%ecx), %eax
	movl	%eax, 20(%esp)                  # 4-byte Spill
	setb	60(%esp)                        # 1-byte Folded Spill
	subl	$4, %esp
	movl	76(%esp), %eax                  # 4-byte Reload
	imull	%edx, %eax
	movl	%edx, %edi
	leal	1084(%esp), %ecx
	pushl	%eax
	movl	1328(%esp), %eax
	pushl	%eax
	pushl	%ecx
	calll	mulPv512x32@PLT
	addl	$12, %esp
	addb	$255, 60(%esp)                  # 1-byte Folded Spill
	movl	1144(%esp), %eax
	adcl	$0, %eax
	addl	1080(%esp), %edi
	adcl	1084(%esp), %ebp
	movl	%ebp, %edx
	movl	52(%esp), %ecx                  # 4-byte Reload
	adcl	1088(%esp), %ecx
	movl	%ecx, 52(%esp)                  # 4-byte Spill
	movl	40(%esp), %ecx                  # 4-byte Reload
	adcl	1092(%esp), %ecx
	movl	%ecx, 40(%esp)                  # 4-byte Spill
	movl	36(%esp), %ecx                  # 4-byte Reload
	adcl	1096(%esp), %ecx
	movl	%ecx, 36(%esp)                  # 4-byte Spill
	movl	48(%esp), %ecx                  # 4-byte Reload
	adcl	1100(%esp), %ecx
	movl	%ecx, 48(%esp)                  # 4-byte Spill
	movl	24(%esp), %ecx                  # 4-byte Reload
	adcl	1104(%esp), %ecx
	movl	%ecx, 24(%esp)                  # 4-byte Spill
	movl	12(%esp), %ecx                  # 4-byte Reload
	adcl	1108(%esp), %ecx
	movl	%ecx, 12(%esp)                  # 4-byte Spill
	movl	28(%esp), %ecx                  # 4-byte Reload
	adcl	1112(%esp), %ecx
	movl	%ecx, 28(%esp)                  # 4-byte Spill
	movl	32(%esp), %ecx                  # 4-byte Reload
	adcl	1116(%esp), %ecx
	movl	%ecx, 32(%esp)                  # 4-byte Spill
	movl	68(%esp), %ecx                  # 4-byte Reload
	adcl	1120(%esp), %ecx
	movl	%ecx, 68(%esp)                  # 4-byte Spill
	movl	16(%esp), %ecx                  # 4-byte Reload
	adcl	1124(%esp), %ecx
	movl	%ecx, 16(%esp)                  # 4-byte Spill
	movl	64(%esp), %ebp                  # 4-byte Reload
	adcl	1128(%esp), %ebp
	movl	44(%esp), %ecx                  # 4-byte Reload
	adcl	1132(%esp), %ecx
	movl	%ecx, 44(%esp)                  # 4-byte Spill
	adcl	1136(%esp), %esi
	movl	%esi, 60(%esp)                  # 4-byte Spill
	movl	20(%esp), %esi                  # 4-byte Reload
	adcl	1140(%esp), %esi
	movl	1316(%esp), %ecx
	adcl	72(%ecx), %eax
	movl	%eax, 56(%esp)                  # 4-byte Spill
	setb	20(%esp)                        # 1-byte Folded Spill
	subl	$4, %esp
	movl	76(%esp), %eax                  # 4-byte Reload
	movl	%edx, %edi
	imull	%edx, %eax
	leal	1012(%esp), %ecx
	pushl	%eax
	pushl	1328(%esp)
	pushl	%ecx
	calll	mulPv512x32@PLT
	addl	$12, %esp
	addb	$255, 20(%esp)                  # 1-byte Folded Spill
	movl	1072(%esp), %eax
	adcl	$0, %eax
	addl	1008(%esp), %edi
	movl	52(%esp), %edx                  # 4-byte Reload
	adcl	1012(%esp), %edx
	movl	40(%esp), %ecx                  # 4-byte Reload
	adcl	1016(%esp), %ecx
	movl	%ecx, 40(%esp)                  # 4-byte Spill
	movl	36(%esp), %ecx                  # 4-byte Reload
	adcl	1020(%esp), %ecx
	movl	%ecx, 36(%esp)                  # 4-byte Spill
	movl	48(%esp), %ecx                  # 4-byte Reload
	adcl	1024(%esp), %ecx
	movl	%ecx, 48(%esp)                  # 4-byte Spill
	movl	24(%esp), %ecx                  # 4-byte Reload
	adcl	1028(%esp), %ecx
	movl	%ecx, 24(%esp)                  # 4-byte Spill
	movl	12(%esp), %ecx                  # 4-byte Reload
	adcl	1032(%esp), %ecx
	movl	%ecx, 12(%esp)                  # 4-byte Spill
	movl	28(%esp), %ecx                  # 4-byte Reload
	adcl	1036(%esp), %ecx
	movl	%ecx, 28(%esp)                  # 4-byte Spill
	movl	32(%esp), %ecx                  # 4-byte Reload
	adcl	1040(%esp), %ecx
	movl	%ecx, 32(%esp)                  # 4-byte Spill
	movl	68(%esp), %ecx                  # 4-byte Reload
	adcl	1044(%esp), %ecx
	movl	%ecx, 68(%esp)                  # 4-byte Spill
	movl	16(%esp), %edi                  # 4-byte Reload
	adcl	1048(%esp), %edi
	adcl	1052(%esp), %ebp
	movl	%ebp, 64(%esp)                  # 4-byte Spill
	movl	44(%esp), %ecx                  # 4-byte Reload
	adcl	1056(%esp), %ecx
	movl	%ecx, 44(%esp)                  # 4-byte Spill
	movl	60(%esp), %ecx                  # 4-byte Reload
	adcl	1060(%esp), %ecx
	movl	%ecx, 60(%esp)                  # 4-byte Spill
	adcl	1064(%esp), %esi
	movl	%esi, 20(%esp)                  # 4-byte Spill
	movl	56(%esp), %ecx                  # 4-byte Reload
	adcl	1068(%esp), %ecx
	movl	%ecx, 56(%esp)                  # 4-byte Spill
	movl	1316(%esp), %ecx
	adcl	76(%ecx), %eax
	movl	%eax, 52(%esp)                  # 4-byte Spill
	setb	16(%esp)                        # 1-byte Folded Spill
	subl	$4, %esp
	movl	76(%esp), %eax                  # 4-byte Reload
	movl	%edx, %esi
	imull	%edx, %eax
	leal	940(%esp), %ecx
	pushl	%eax
	pushl	1328(%esp)
	pushl	%ecx
	calll	mulPv512x32@PLT
	addl	$12, %esp
	addb	$255, 16(%esp)                  # 1-byte Folded Spill
	movl	1000(%esp), %eax
	adcl	$0, %eax
	addl	936(%esp), %esi
	movl	40(%esp), %edx                  # 4-byte Reload
	adcl	940(%esp), %edx
	movl	36(%esp), %ecx                  # 4-byte Reload
	adcl	944(%esp), %ecx
	movl	%ecx, 36(%esp)                  # 4-byte Spill
	movl	48(%esp), %ecx                  # 4-byte Reload
	adcl	948(%esp), %ecx
	movl	%ecx, 48(%esp)                  # 4-byte Spill
	movl	24(%esp), %ecx                  # 4-byte Reload
	adcl	952(%esp), %ecx
	movl	%ecx, 24(%esp)                  # 4-byte Spill
	movl	12(%esp), %ecx                  # 4-byte Reload
	adcl	956(%esp), %ecx
	movl	%ecx, 12(%esp)                  # 4-byte Spill
	movl	28(%esp), %ecx                  # 4-byte Reload
	adcl	960(%esp), %ecx
	movl	%ecx, 28(%esp)                  # 4-byte Spill
	movl	32(%esp), %ecx                  # 4-byte Reload
	adcl	964(%esp), %ecx
	movl	%ecx, 32(%esp)                  # 4-byte Spill
	movl	68(%esp), %ebp                  # 4-byte Reload
	adcl	968(%esp), %ebp
	adcl	972(%esp), %edi
	movl	%edi, 16(%esp)                  # 4-byte Spill
	movl	64(%esp), %ecx                  # 4-byte Reload
	adcl	976(%esp), %ecx
	movl	%ecx, 64(%esp)                  # 4-byte Spill
	movl	44(%esp), %esi                  # 4-byte Reload
	adcl	980(%esp), %esi
	movl	60(%esp), %ecx                  # 4-byte Reload
	adcl	984(%esp), %ecx
	movl	%ecx, 60(%esp)                  # 4-byte Spill
	movl	20(%esp), %ecx                  # 4-byte Reload
	adcl	988(%esp), %ecx
	movl	%ecx, 20(%esp)                  # 4-byte Spill
	movl	56(%esp), %ecx                  # 4-byte Reload
	adcl	992(%esp), %ecx
	movl	%ecx, 56(%esp)                  # 4-byte Spill
	movl	52(%esp), %ecx                  # 4-byte Reload
	adcl	996(%esp), %ecx
	movl	%ecx, 52(%esp)                  # 4-byte Spill
	movl	1316(%esp), %ecx
	adcl	80(%ecx), %eax
	movl	%eax, 40(%esp)                  # 4-byte Spill
	setb	44(%esp)                        # 1-byte Folded Spill
	subl	$4, %esp
	movl	76(%esp), %eax                  # 4-byte Reload
	imull	%edx, %eax
	movl	%edx, %edi
	leal	868(%esp), %ecx
	pushl	%eax
	pushl	1328(%esp)
	pushl	%ecx
	calll	mulPv512x32@PLT
	addl	$12, %esp
	addb	$255, 44(%esp)                  # 1-byte Folded Spill
	movl	928(%esp), %eax
	adcl	$0, %eax
	addl	864(%esp), %edi
	movl	36(%esp), %edx                  # 4-byte Reload
	adcl	868(%esp), %edx
	movl	48(%esp), %ecx                  # 4-byte Reload
	adcl	872(%esp), %ecx
	movl	%ecx, 48(%esp)                  # 4-byte Spill
	movl	24(%esp), %ecx                  # 4-byte Reload
	adcl	876(%esp), %ecx
	movl	%ecx, 24(%esp)                  # 4-byte Spill
	movl	12(%esp), %ecx                  # 4-byte Reload
	adcl	880(%esp), %ecx
	movl	%ecx, 12(%esp)                  # 4-byte Spill
	movl	28(%esp), %ecx                  # 4-byte Reload
	adcl	884(%esp), %ecx
	movl	%ecx, 28(%esp)                  # 4-byte Spill
	movl	32(%esp), %ecx                  # 4-byte Reload
	adcl	888(%esp), %ecx
	movl	%ecx, 32(%esp)                  # 4-byte Spill
	adcl	892(%esp), %ebp
	movl	%ebp, 68(%esp)                  # 4-byte Spill
	movl	16(%esp), %ecx                  # 4-byte Reload
	adcl	896(%esp), %ecx
	movl	%ecx, 16(%esp)                  # 4-byte Spill
	movl	64(%esp), %ecx                  # 4-byte Reload
	adcl	900(%esp), %ecx
	movl	%ecx, 64(%esp)                  # 4-byte Spill
	adcl	904(%esp), %esi
	movl	%esi, 44(%esp)                  # 4-byte Spill
	movl	60(%esp), %esi                  # 4-byte Reload
	adcl	908(%esp), %esi
	movl	20(%esp), %ecx                  # 4-byte Reload
	adcl	912(%esp), %ecx
	movl	%ecx, 20(%esp)                  # 4-byte Spill
	movl	56(%esp), %ecx                  # 4-byte Reload
	adcl	916(%esp), %ecx
	movl	%ecx, 56(%esp)                  # 4-byte Spill
	movl	52(%esp), %ecx                  # 4-byte Reload
	adcl	920(%esp), %ecx
	movl	%ecx, 52(%esp)                  # 4-byte Spill
	movl	40(%esp), %ebp                  # 4-byte Reload
	adcl	924(%esp), %ebp
	movl	1316(%esp), %ecx
	adcl	84(%ecx), %eax
	movl	%eax, 36(%esp)                  # 4-byte Spill
	setb	60(%esp)                        # 1-byte Folded Spill
	subl	$4, %esp
	movl	76(%esp), %eax                  # 4-byte Reload
	movl	%edx, %edi
	imull	%edx, %eax
	leal	796(%esp), %ecx
	pushl	%eax
	pushl	1328(%esp)
	pushl	%ecx
	calll	mulPv512x32@PLT
	addl	$12, %esp
	addb	$255, 60(%esp)                  # 1-byte Folded Spill
	movl	856(%esp), %eax
	adcl	$0, %eax
	addl	792(%esp), %edi
	movl	48(%esp), %edx                  # 4-byte Reload
	adcl	796(%esp), %edx
	movl	24(%esp), %ecx                  # 4-byte Reload
	adcl	800(%esp), %ecx
	movl	%ecx, 24(%esp)                  # 4-byte Spill
	movl	12(%esp), %ecx                  # 4-byte Reload
	adcl	804(%esp), %ecx
	movl	%ecx, 12(%esp)                  # 4-byte Spill
	movl	28(%esp), %ecx                  # 4-byte Reload
	adcl	808(%esp), %ecx
	movl	%ecx, 28(%esp)                  # 4-byte Spill
	movl	32(%esp), %edi                  # 4-byte Reload
	adcl	812(%esp), %edi
	movl	68(%esp), %ecx                  # 4-byte Reload
	adcl	816(%esp), %ecx
	movl	%ecx, 68(%esp)                  # 4-byte Spill
	movl	16(%esp), %ecx                  # 4-byte Reload
	adcl	820(%esp), %ecx
	movl	%ecx, 16(%esp)                  # 4-byte Spill
	movl	64(%esp), %ecx                  # 4-byte Reload
	adcl	824(%esp), %ecx
	movl	%ecx, 64(%esp)                  # 4-byte Spill
	movl	44(%esp), %ecx                  # 4-byte Reload
	adcl	828(%esp), %ecx
	movl	%ecx, 44(%esp)                  # 4-byte Spill
	adcl	832(%esp), %esi
	movl	%esi, 60(%esp)                  # 4-byte Spill
	movl	20(%esp), %ecx                  # 4-byte Reload
	adcl	836(%esp), %ecx
	movl	%ecx, 20(%esp)                  # 4-byte Spill
	movl	56(%esp), %esi                  # 4-byte Reload
	adcl	840(%esp), %esi
	movl	52(%esp), %ecx                  # 4-byte Reload
	adcl	844(%esp), %ecx
	movl	%ecx, 52(%esp)                  # 4-byte Spill
	adcl	848(%esp), %ebp
	movl	%ebp, 40(%esp)                  # 4-byte Spill
	movl	36(%esp), %ecx                  # 4-byte Reload
	adcl	852(%esp), %ecx
	movl	%ecx, 36(%esp)                  # 4-byte Spill
	movl	1316(%esp), %ecx
	adcl	88(%ecx), %eax
	movl	%eax, 48(%esp)                  # 4-byte Spill
	setb	56(%esp)                        # 1-byte Folded Spill
	subl	$4, %esp
	movl	76(%esp), %eax                  # 4-byte Reload
	movl	%edx, %ebp
	imull	%edx, %eax
	leal	724(%esp), %ecx
	pushl	%eax
	pushl	1328(%esp)
	pushl	%ecx
	calll	mulPv512x32@PLT
	addl	$12, %esp
	addb	$255, 56(%esp)                  # 1-byte Folded Spill
	movl	784(%esp), %eax
	adcl	$0, %eax
	movl	%eax, %edx
	addl	720(%esp), %ebp
	movl	24(%esp), %ecx                  # 4-byte Reload
	adcl	724(%esp), %ecx
	movl	%ecx, 24(%esp)                  # 4-byte Spill
	movl	12(%esp), %eax                  # 4-byte Reload
	adcl	728(%esp), %eax
	movl	%eax, 12(%esp)                  # 4-byte Spill
	movl	28(%esp), %ebp                  # 4-byte Reload
	adcl	732(%esp), %ebp
	adcl	736(%esp), %edi
	movl	%edi, 32(%esp)                  # 4-byte Spill
	movl	68(%esp), %eax                  # 4-byte Reload
	adcl	740(%esp), %eax
	movl	%eax, 68(%esp)                  # 4-byte Spill
	movl	16(%esp), %eax                  # 4-byte Reload
	adcl	744(%esp), %eax
	movl	%eax, 16(%esp)                  # 4-byte Spill
	movl	64(%esp), %edi                  # 4-byte Reload
	adcl	748(%esp), %edi
	movl	44(%esp), %eax                  # 4-byte Reload
	adcl	752(%esp), %eax
	movl	%eax, 44(%esp)                  # 4-byte Spill
	movl	60(%esp), %eax                  # 4-byte Reload
	adcl	756(%esp), %eax
	movl	%eax, 60(%esp)                  # 4-byte Spill
	movl	20(%esp), %eax                  # 4-byte Reload
	adcl	760(%esp), %eax
	movl	%eax, 20(%esp)                  # 4-byte Spill
	adcl	764(%esp), %esi
	movl	%esi, 56(%esp)                  # 4-byte Spill
	movl	52(%esp), %esi                  # 4-byte Reload
	adcl	768(%esp), %esi
	movl	40(%esp), %eax                  # 4-byte Reload
	adcl	772(%esp), %eax
	movl	%eax, 40(%esp)                  # 4-byte Spill
	movl	36(%esp), %eax                  # 4-byte Reload
	adcl	776(%esp), %eax
	movl	%eax, 36(%esp)                  # 4-byte Spill
	movl	48(%esp), %eax                  # 4-byte Reload
	adcl	780(%esp), %eax
	movl	%eax, 48(%esp)                  # 4-byte Spill
	movl	1316(%esp), %eax
	adcl	92(%eax), %edx
	movl	%edx, 76(%esp)                  # 4-byte Spill
	setb	52(%esp)                        # 1-byte Folded Spill
	subl	$4, %esp
	movl	76(%esp), %eax                  # 4-byte Reload
	imull	%ecx, %eax
	leal	652(%esp), %ecx
	pushl	%eax
	pushl	1328(%esp)
	pushl	%ecx
	calll	mulPv512x32@PLT
	addl	$12, %esp
	addb	$255, 52(%esp)                  # 1-byte Folded Spill
	movl	712(%esp), %eax
	adcl	$0, %eax
	movl	24(%esp), %ecx                  # 4-byte Reload
	addl	648(%esp), %ecx
	movl	12(%esp), %ecx                  # 4-byte Reload
	adcl	652(%esp), %ecx
	adcl	656(%esp), %ebp
	movl	%ebp, 28(%esp)                  # 4-byte Spill
	movl	32(%esp), %edx                  # 4-byte Reload
	adcl	660(%esp), %edx
	movl	%edx, 32(%esp)                  # 4-byte Spill
	movl	68(%esp), %edx                  # 4-byte Reload
	adcl	664(%esp), %edx
	movl	%edx, 68(%esp)                  # 4-byte Spill
	movl	16(%esp), %edx                  # 4-byte Reload
	adcl	668(%esp), %edx
	movl	%edx, 16(%esp)                  # 4-byte Spill
	adcl	672(%esp), %edi
	movl	%edi, 64(%esp)                  # 4-byte Spill
	movl	44(%esp), %edx                  # 4-byte Reload
	adcl	676(%esp), %edx
	movl	%edx, 44(%esp)                  # 4-byte Spill
	movl	60(%esp), %edx                  # 4-byte Reload
	adcl	680(%esp), %edx
	movl	%edx, 60(%esp)                  # 4-byte Spill
	movl	20(%esp), %edx                  # 4-byte Reload
	adcl	684(%esp), %edx
	movl	%edx, 20(%esp)                  # 4-byte Spill
	movl	56(%esp), %edx                  # 4-byte Reload
	adcl	688(%esp), %edx
	movl	%edx, 56(%esp)                  # 4-byte Spill
	adcl	692(%esp), %esi
	movl	%esi, 52(%esp)                  # 4-byte Spill
	movl	40(%esp), %ebp                  # 4-byte Reload
	adcl	696(%esp), %ebp
	movl	36(%esp), %edx                  # 4-byte Reload
	adcl	700(%esp), %edx
	movl	%edx, 36(%esp)                  # 4-byte Spill
	movl	48(%esp), %edx                  # 4-byte Reload
	adcl	704(%esp), %edx
	movl	%edx, 48(%esp)                  # 4-byte Spill
	movl	76(%esp), %edx                  # 4-byte Reload
	adcl	708(%esp), %edx
	movl	%edx, 76(%esp)                  # 4-byte Spill
	movl	1316(%esp), %edx
	adcl	96(%edx), %eax
	movl	%eax, 12(%esp)                  # 4-byte Spill
	setb	40(%esp)                        # 1-byte Folded Spill
	subl	$4, %esp
	movl	76(%esp), %esi                  # 4-byte Reload
	movl	%esi, %eax
	movl	%ecx, %edi
	imull	%ecx, %eax
	leal	580(%esp), %ecx
	pushl	%eax
	movl	1328(%esp), %eax
	pushl	%eax
	pushl	%ecx
	calll	mulPv512x32@PLT
	addl	$12, %esp
	addb	$255, 40(%esp)                  # 1-byte Folded Spill
	movl	640(%esp), %eax
	adcl	$0, %eax
	movl	%eax, %edx
	addl	576(%esp), %edi
	movl	28(%esp), %ecx                  # 4-byte Reload
	adcl	580(%esp), %ecx
	movl	32(%esp), %eax                  # 4-byte Reload
	adcl	584(%esp), %eax
	movl	%eax, 32(%esp)                  # 4-byte Spill
	movl	68(%esp), %edi                  # 4-byte Reload
	adcl	588(%esp), %edi
	movl	16(%esp), %eax                  # 4-byte Reload
	adcl	592(%esp), %eax
	movl	%eax, 16(%esp)                  # 4-byte Spill
	movl	64(%esp), %eax                  # 4-byte Reload
	adcl	596(%esp), %eax
	movl	%eax, 64(%esp)                  # 4-byte Spill
	movl	44(%esp), %eax                  # 4-byte Reload
	adcl	600(%esp), %eax
	movl	%eax, 44(%esp)                  # 4-byte Spill
	movl	60(%esp), %eax                  # 4-byte Reload
	adcl	604(%esp), %eax
	movl	%eax, 60(%esp)                  # 4-byte Spill
	movl	20(%esp), %eax                  # 4-byte Reload
	adcl	608(%esp), %eax
	movl	%eax, 20(%esp)                  # 4-byte Spill
	movl	56(%esp), %eax                  # 4-byte Reload
	adcl	612(%esp), %eax
	movl	%eax, 56(%esp)                  # 4-byte Spill
	movl	52(%esp), %eax                  # 4-byte Reload
	adcl	616(%esp), %eax
	movl	%eax, 52(%esp)                  # 4-byte Spill
	adcl	620(%esp), %ebp
	movl	%ebp, 40(%esp)                  # 4-byte Spill
	movl	36(%esp), %eax                  # 4-byte Reload
	adcl	624(%esp), %eax
	movl	%eax, 36(%esp)                  # 4-byte Spill
	movl	48(%esp), %eax                  # 4-byte Reload
	adcl	628(%esp), %eax
	movl	%eax, 48(%esp)                  # 4-byte Spill
	movl	76(%esp), %ebp                  # 4-byte Reload
	adcl	632(%esp), %ebp
	movl	12(%esp), %eax                  # 4-byte Reload
	adcl	636(%esp), %eax
	movl	%eax, 12(%esp)                  # 4-byte Spill
	movl	1316(%esp), %eax
	adcl	100(%eax), %edx
	movl	%edx, 28(%esp)                  # 4-byte Spill
	setb	24(%esp)                        # 1-byte Folded Spill
	subl	$4, %esp
	movl	%esi, %eax
	movl	%ecx, %esi
	imull	%ecx, %eax
	leal	508(%esp), %ecx
	pushl	%eax
	pushl	1328(%esp)
	pushl	%ecx
	calll	mulPv512x32@PLT
	addl	$12, %esp
	addb	$255, 24(%esp)                  # 1-byte Folded Spill
	movl	568(%esp), %eax
	adcl	$0, %eax
	movl	%eax, %edx
	addl	504(%esp), %esi
	movl	32(%esp), %esi                  # 4-byte Reload
	adcl	508(%esp), %esi
	adcl	512(%esp), %edi
	movl	16(%esp), %eax                  # 4-byte Reload
	adcl	516(%esp), %eax
	movl	%eax, 16(%esp)                  # 4-byte Spill
	movl	64(%esp), %eax                  # 4-byte Reload
	adcl	520(%esp), %eax
	movl	%eax, 64(%esp)                  # 4-byte Spill
	movl	44(%esp), %eax                  # 4-byte Reload
	adcl	524(%esp), %eax
	movl	%eax, 44(%esp)                  # 4-byte Spill
	movl	60(%esp), %eax                  # 4-byte Reload
	adcl	528(%esp), %eax
	movl	%eax, 60(%esp)                  # 4-byte Spill
	movl	20(%esp), %eax                  # 4-byte Reload
	adcl	532(%esp), %eax
	movl	%eax, 20(%esp)                  # 4-byte Spill
	movl	56(%esp), %eax                  # 4-byte Reload
	adcl	536(%esp), %eax
	movl	%eax, 56(%esp)                  # 4-byte Spill
	movl	52(%esp), %eax                  # 4-byte Reload
	adcl	540(%esp), %eax
	movl	%eax, 52(%esp)                  # 4-byte Spill
	movl	40(%esp), %eax                  # 4-byte Reload
	adcl	544(%esp), %eax
	movl	%eax, 40(%esp)                  # 4-byte Spill
	movl	36(%esp), %eax                  # 4-byte Reload
	adcl	548(%esp), %eax
	movl	%eax, 36(%esp)                  # 4-byte Spill
	movl	48(%esp), %eax                  # 4-byte Reload
	adcl	552(%esp), %eax
	movl	%eax, 48(%esp)                  # 4-byte Spill
	adcl	556(%esp), %ebp
	movl	%ebp, 76(%esp)                  # 4-byte Spill
	movl	12(%esp), %eax                  # 4-byte Reload
	adcl	560(%esp), %eax
	movl	%eax, 12(%esp)                  # 4-byte Spill
	movl	28(%esp), %eax                  # 4-byte Reload
	adcl	564(%esp), %eax
	movl	%eax, 28(%esp)                  # 4-byte Spill
	movl	1316(%esp), %eax
	adcl	104(%eax), %edx
	movl	%edx, 32(%esp)                  # 4-byte Spill
	setb	24(%esp)                        # 1-byte Folded Spill
	subl	$4, %esp
	movl	76(%esp), %eax                  # 4-byte Reload
	imull	%esi, %eax
	leal	436(%esp), %ecx
	pushl	%eax
	pushl	1328(%esp)
	pushl	%ecx
	calll	mulPv512x32@PLT
	addl	$12, %esp
	addb	$255, 24(%esp)                  # 1-byte Folded Spill
	movl	496(%esp), %eax
	adcl	$0, %eax
	movl	%eax, %edx
	addl	432(%esp), %esi
	movl	%edi, %esi
	adcl	436(%esp), %esi
	movl	16(%esp), %eax                  # 4-byte Reload
	adcl	440(%esp), %eax
	movl	%eax, 16(%esp)                  # 4-byte Spill
	movl	64(%esp), %eax                  # 4-byte Reload
	adcl	444(%esp), %eax
	movl	%eax, 64(%esp)                  # 4-byte Spill
	movl	44(%esp), %ebp                  # 4-byte Reload
	adcl	448(%esp), %ebp
	movl	60(%esp), %eax                  # 4-byte Reload
	adcl	452(%esp), %eax
	movl	%eax, 60(%esp)                  # 4-byte Spill
	movl	20(%esp), %edi                  # 4-byte Reload
	adcl	456(%esp), %edi
	movl	56(%esp), %eax                  # 4-byte Reload
	adcl	460(%esp), %eax
	movl	%eax, 56(%esp)                  # 4-byte Spill
	movl	52(%esp), %eax                  # 4-byte Reload
	adcl	464(%esp), %eax
	movl	%eax, 52(%esp)                  # 4-byte Spill
	movl	40(%esp), %eax                  # 4-byte Reload
	adcl	468(%esp), %eax
	movl	%eax, 40(%esp)                  # 4-byte Spill
	movl	36(%esp), %eax                  # 4-byte Reload
	adcl	472(%esp), %eax
	movl	%eax, 36(%esp)                  # 4-byte Spill
	movl	48(%esp), %eax                  # 4-byte Reload
	adcl	476(%esp), %eax
	movl	%eax, 48(%esp)                  # 4-byte Spill
	movl	76(%esp), %eax                  # 4-byte Reload
	adcl	480(%esp), %eax
	movl	%eax, 76(%esp)                  # 4-byte Spill
	movl	12(%esp), %eax                  # 4-byte Reload
	adcl	484(%esp), %eax
	movl	%eax, 12(%esp)                  # 4-byte Spill
	movl	28(%esp), %eax                  # 4-byte Reload
	adcl	488(%esp), %eax
	movl	%eax, 28(%esp)                  # 4-byte Spill
	movl	32(%esp), %eax                  # 4-byte Reload
	adcl	492(%esp), %eax
	movl	%eax, 32(%esp)                  # 4-byte Spill
	movl	1316(%esp), %eax
	adcl	108(%eax), %edx
	movl	%edx, 24(%esp)                  # 4-byte Spill
	setb	20(%esp)                        # 1-byte Folded Spill
	subl	$4, %esp
	movl	76(%esp), %eax                  # 4-byte Reload
	imull	%esi, %eax
	leal	364(%esp), %ecx
	pushl	%eax
	pushl	1328(%esp)
	pushl	%ecx
	calll	mulPv512x32@PLT
	addl	$12, %esp
	addb	$255, 20(%esp)                  # 1-byte Folded Spill
	movl	424(%esp), %eax
	adcl	$0, %eax
	movl	%eax, %edx
	addl	360(%esp), %esi
	movl	16(%esp), %esi                  # 4-byte Reload
	adcl	364(%esp), %esi
	movl	64(%esp), %eax                  # 4-byte Reload
	adcl	368(%esp), %eax
	movl	%eax, 64(%esp)                  # 4-byte Spill
	adcl	372(%esp), %ebp
	movl	%ebp, 44(%esp)                  # 4-byte Spill
	movl	60(%esp), %ebp                  # 4-byte Reload
	adcl	376(%esp), %ebp
	adcl	380(%esp), %edi
	movl	%edi, 20(%esp)                  # 4-byte Spill
	movl	56(%esp), %eax                  # 4-byte Reload
	adcl	384(%esp), %eax
	movl	%eax, 56(%esp)                  # 4-byte Spill
	movl	52(%esp), %eax                  # 4-byte Reload
	adcl	388(%esp), %eax
	movl	%eax, 52(%esp)                  # 4-byte Spill
	movl	40(%esp), %eax                  # 4-byte Reload
	adcl	392(%esp), %eax
	movl	%eax, 40(%esp)                  # 4-byte Spill
	movl	36(%esp), %edi                  # 4-byte Reload
	adcl	396(%esp), %edi
	movl	48(%esp), %eax                  # 4-byte Reload
	adcl	400(%esp), %eax
	movl	%eax, 48(%esp)                  # 4-byte Spill
	movl	76(%esp), %eax                  # 4-byte Reload
	adcl	404(%esp), %eax
	movl	%eax, 76(%esp)                  # 4-byte Spill
	movl	12(%esp), %eax                  # 4-byte Reload
	adcl	408(%esp), %eax
	movl	%eax, 12(%esp)                  # 4-byte Spill
	movl	28(%esp), %eax                  # 4-byte Reload
	adcl	412(%esp), %eax
	movl	%eax, 28(%esp)                  # 4-byte Spill
	movl	32(%esp), %eax                  # 4-byte Reload
	adcl	416(%esp), %eax
	movl	%eax, 32(%esp)                  # 4-byte Spill
	movl	24(%esp), %eax                  # 4-byte Reload
	adcl	420(%esp), %eax
	movl	%eax, 24(%esp)                  # 4-byte Spill
	movl	1316(%esp), %eax
	adcl	112(%eax), %edx
	movl	%edx, 16(%esp)                  # 4-byte Spill
	setb	60(%esp)                        # 1-byte Folded Spill
	subl	$4, %esp
	movl	76(%esp), %eax                  # 4-byte Reload
	imull	%esi, %eax
	leal	292(%esp), %ecx
	pushl	%eax
	pushl	1328(%esp)
	pushl	%ecx
	calll	mulPv512x32@PLT
	addl	$12, %esp
	addb	$255, 60(%esp)                  # 1-byte Folded Spill
	movl	352(%esp), %eax
	adcl	$0, %eax
	movl	%eax, %edx
	addl	288(%esp), %esi
	movl	64(%esp), %esi                  # 4-byte Reload
	adcl	292(%esp), %esi
	movl	44(%esp), %eax                  # 4-byte Reload
	adcl	296(%esp), %eax
	movl	%eax, 44(%esp)                  # 4-byte Spill
	adcl	300(%esp), %ebp
	movl	%ebp, 60(%esp)                  # 4-byte Spill
	movl	20(%esp), %eax                  # 4-byte Reload
	adcl	304(%esp), %eax
	movl	%eax, 20(%esp)                  # 4-byte Spill
	movl	56(%esp), %eax                  # 4-byte Reload
	adcl	308(%esp), %eax
	movl	%eax, 56(%esp)                  # 4-byte Spill
	movl	52(%esp), %eax                  # 4-byte Reload
	adcl	312(%esp), %eax
	movl	%eax, 52(%esp)                  # 4-byte Spill
	movl	40(%esp), %eax                  # 4-byte Reload
	adcl	316(%esp), %eax
	movl	%eax, 40(%esp)                  # 4-byte Spill
	adcl	320(%esp), %edi
	movl	%edi, 36(%esp)                  # 4-byte Spill
	movl	48(%esp), %ebp                  # 4-byte Reload
	adcl	324(%esp), %ebp
	movl	76(%esp), %eax                  # 4-byte Reload
	adcl	328(%esp), %eax
	movl	%eax, 76(%esp)                  # 4-byte Spill
	movl	12(%esp), %eax                  # 4-byte Reload
	adcl	332(%esp), %eax
	movl	%eax, 12(%esp)                  # 4-byte Spill
	movl	28(%esp), %eax                  # 4-byte Reload
	adcl	336(%esp), %eax
	movl	%eax, 28(%esp)                  # 4-byte Spill
	movl	32(%esp), %eax                  # 4-byte Reload
	adcl	340(%esp), %eax
	movl	%eax, 32(%esp)                  # 4-byte Spill
	movl	24(%esp), %eax                  # 4-byte Reload
	adcl	344(%esp), %eax
	movl	%eax, 24(%esp)                  # 4-byte Spill
	movl	16(%esp), %eax                  # 4-byte Reload
	adcl	348(%esp), %eax
	movl	%eax, 16(%esp)                  # 4-byte Spill
	movl	1316(%esp), %eax
	adcl	116(%eax), %edx
	movl	%edx, 64(%esp)                  # 4-byte Spill
	setb	68(%esp)                        # 1-byte Folded Spill
	subl	$4, %esp
	movl	76(%esp), %eax                  # 4-byte Reload
	imull	%esi, %eax
	leal	220(%esp), %ecx
	pushl	%eax
	pushl	1328(%esp)
	pushl	%ecx
	calll	mulPv512x32@PLT
	addl	$12, %esp
	addb	$255, 68(%esp)                  # 1-byte Folded Spill
	movl	280(%esp), %eax
	adcl	$0, %eax
	movl	%eax, %edx
	addl	216(%esp), %esi
	movl	44(%esp), %esi                  # 4-byte Reload
	adcl	220(%esp), %esi
	movl	60(%esp), %ecx                  # 4-byte Reload
	adcl	224(%esp), %ecx
	movl	%ecx, 60(%esp)                  # 4-byte Spill
	movl	20(%esp), %ecx                  # 4-byte Reload
	adcl	228(%esp), %ecx
	movl	%ecx, 20(%esp)                  # 4-byte Spill
	movl	56(%esp), %edi                  # 4-byte Reload
	adcl	232(%esp), %edi
	movl	52(%esp), %ecx                  # 4-byte Reload
	adcl	236(%esp), %ecx
	movl	%ecx, 52(%esp)                  # 4-byte Spill
	movl	40(%esp), %ecx                  # 4-byte Reload
	adcl	240(%esp), %ecx
	movl	%ecx, 40(%esp)                  # 4-byte Spill
	movl	36(%esp), %ecx                  # 4-byte Reload
	adcl	244(%esp), %ecx
	movl	%ecx, 36(%esp)                  # 4-byte Spill
	adcl	248(%esp), %ebp
	movl	%ebp, 48(%esp)                  # 4-byte Spill
	movl	76(%esp), %ecx                  # 4-byte Reload
	adcl	252(%esp), %ecx
	movl	%ecx, 76(%esp)                  # 4-byte Spill
	movl	12(%esp), %ecx                  # 4-byte Reload
	adcl	256(%esp), %ecx
	movl	%ecx, 12(%esp)                  # 4-byte Spill
	movl	28(%esp), %ecx                  # 4-byte Reload
	adcl	260(%esp), %ecx
	movl	%ecx, 28(%esp)                  # 4-byte Spill
	movl	32(%esp), %ecx                  # 4-byte Reload
	adcl	264(%esp), %ecx
	movl	%ecx, 32(%esp)                  # 4-byte Spill
	movl	24(%esp), %ecx                  # 4-byte Reload
	adcl	268(%esp), %ecx
	movl	%ecx, 24(%esp)                  # 4-byte Spill
	movl	16(%esp), %ecx                  # 4-byte Reload
	adcl	272(%esp), %ecx
	movl	%ecx, 16(%esp)                  # 4-byte Spill
	movl	64(%esp), %ecx                  # 4-byte Reload
	adcl	276(%esp), %ecx
	movl	%ecx, 64(%esp)                  # 4-byte Spill
	movl	1316(%esp), %ecx
	adcl	120(%ecx), %edx
	movl	%edx, 44(%esp)                  # 4-byte Spill
	setb	56(%esp)                        # 1-byte Folded Spill
	movl	72(%esp), %ecx                  # 4-byte Reload
	imull	%esi, %ecx
	subl	$4, %esp
	leal	148(%esp), %eax
	pushl	%ecx
	pushl	1328(%esp)
	pushl	%eax
	calll	mulPv512x32@PLT
	addl	$12, %esp
	addb	$255, 56(%esp)                  # 1-byte Folded Spill
	movl	208(%esp), %ebp
	adcl	$0, %ebp
	addl	144(%esp), %esi
	movl	60(%esp), %ecx                  # 4-byte Reload
	adcl	148(%esp), %ecx
	movl	20(%esp), %edx                  # 4-byte Reload
	adcl	152(%esp), %edx
	movl	%edi, %esi
	adcl	156(%esp), %esi
	movl	52(%esp), %edi                  # 4-byte Reload
	adcl	160(%esp), %edi
	movl	40(%esp), %eax                  # 4-byte Reload
	adcl	164(%esp), %eax
	movl	%eax, 40(%esp)                  # 4-byte Spill
	movl	36(%esp), %eax                  # 4-byte Reload
	adcl	168(%esp), %eax
	movl	%eax, 36(%esp)                  # 4-byte Spill
	movl	48(%esp), %eax                  # 4-byte Reload
	adcl	172(%esp), %eax
	movl	76(%esp), %ebx                  # 4-byte Reload
	adcl	176(%esp), %ebx
	movl	%ebx, 76(%esp)                  # 4-byte Spill
	movl	12(%esp), %ebx                  # 4-byte Reload
	adcl	180(%esp), %ebx
	movl	%ebx, 12(%esp)                  # 4-byte Spill
	movl	28(%esp), %ebx                  # 4-byte Reload
	adcl	184(%esp), %ebx
	movl	%ebx, 28(%esp)                  # 4-byte Spill
	movl	32(%esp), %ebx                  # 4-byte Reload
	adcl	188(%esp), %ebx
	movl	%ebx, 32(%esp)                  # 4-byte Spill
	movl	24(%esp), %ebx                  # 4-byte Reload
	adcl	192(%esp), %ebx
	movl	%ebx, 24(%esp)                  # 4-byte Spill
	movl	16(%esp), %ebx                  # 4-byte Reload
	adcl	196(%esp), %ebx
	movl	%ebx, 16(%esp)                  # 4-byte Spill
	movl	64(%esp), %ebx                  # 4-byte Reload
	adcl	200(%esp), %ebx
	movl	%ebx, 64(%esp)                  # 4-byte Spill
	movl	44(%esp), %ebx                  # 4-byte Reload
	adcl	204(%esp), %ebx
	movl	%ebx, 44(%esp)                  # 4-byte Spill
	movl	1316(%esp), %ebx
	adcl	124(%ebx), %ebp
	xorl	%ebx, %ebx
	movl	%ecx, 60(%esp)                  # 4-byte Spill
	subl	84(%esp), %ecx                  # 4-byte Folded Reload
	movl	%ecx, 84(%esp)                  # 4-byte Spill
	movl	%edx, %ecx
	movl	%edx, 20(%esp)                  # 4-byte Spill
	sbbl	88(%esp), %ecx                  # 4-byte Folded Reload
	movl	%ecx, 88(%esp)                  # 4-byte Spill
	movl	%esi, %ecx
	movl	%esi, 56(%esp)                  # 4-byte Spill
	sbbl	92(%esp), %ecx                  # 4-byte Folded Reload
	movl	%ecx, 92(%esp)                  # 4-byte Spill
	movl	%edi, %ecx
	movl	%edi, 52(%esp)                  # 4-byte Spill
	sbbl	96(%esp), %ecx                  # 4-byte Folded Reload
	movl	%ecx, 96(%esp)                  # 4-byte Spill
	movl	40(%esp), %ecx                  # 4-byte Reload
	sbbl	100(%esp), %ecx                 # 4-byte Folded Reload
	movl	%ecx, 100(%esp)                 # 4-byte Spill
	movl	36(%esp), %ecx                  # 4-byte Reload
	sbbl	104(%esp), %ecx                 # 4-byte Folded Reload
	movl	%ecx, 104(%esp)                 # 4-byte Spill
	movl	%eax, 48(%esp)                  # 4-byte Spill
	sbbl	120(%esp), %eax                 # 4-byte Folded Reload
	movl	%eax, 68(%esp)                  # 4-byte Spill
	movl	76(%esp), %eax                  # 4-byte Reload
	sbbl	124(%esp), %eax                 # 4-byte Folded Reload
	movl	%eax, 72(%esp)                  # 4-byte Spill
	movl	12(%esp), %eax                  # 4-byte Reload
	sbbl	108(%esp), %eax                 # 4-byte Folded Reload
	movl	%eax, 108(%esp)                 # 4-byte Spill
	movl	28(%esp), %eax                  # 4-byte Reload
	sbbl	80(%esp), %eax                  # 4-byte Folded Reload
	movl	%eax, 80(%esp)                  # 4-byte Spill
	movl	32(%esp), %eax                  # 4-byte Reload
	sbbl	112(%esp), %eax                 # 4-byte Folded Reload
	movl	%eax, 112(%esp)                 # 4-byte Spill
	movl	24(%esp), %eax                  # 4-byte Reload
	sbbl	116(%esp), %eax                 # 4-byte Folded Reload
	movl	%eax, 116(%esp)                 # 4-byte Spill
	movl	16(%esp), %edx                  # 4-byte Reload
	sbbl	128(%esp), %edx                 # 4-byte Folded Reload
	movl	64(%esp), %esi                  # 4-byte Reload
	sbbl	132(%esp), %esi                 # 4-byte Folded Reload
	movl	44(%esp), %edi                  # 4-byte Reload
	sbbl	136(%esp), %edi                 # 4-byte Folded Reload
	movl	%ebp, %ecx
	sbbl	140(%esp), %ecx                 # 4-byte Folded Reload
	sbbl	%ebx, %ebx
	testb	$1, %bl
	je	.LBB79_2
# %bb.1:
	movl	%ebp, %ecx
.LBB79_2:
	movl	1312(%esp), %eax
	movl	%ecx, 60(%eax)
	je	.LBB79_4
# %bb.3:
	movl	44(%esp), %edi                  # 4-byte Reload
.LBB79_4:
	movl	%edi, 56(%eax)
	movl	84(%esp), %ecx                  # 4-byte Reload
	movl	100(%esp), %ebx                 # 4-byte Reload
	movl	104(%esp), %ebp                 # 4-byte Reload
	je	.LBB79_6
# %bb.5:
	movl	64(%esp), %esi                  # 4-byte Reload
.LBB79_6:
	movl	%esi, 52(%eax)
	je	.LBB79_8
# %bb.7:
	movl	16(%esp), %edx                  # 4-byte Reload
.LBB79_8:
	movl	%edx, 48(%eax)
	movl	92(%esp), %edi                  # 4-byte Reload
	movl	112(%esp), %esi                 # 4-byte Reload
	movl	116(%esp), %edx                 # 4-byte Reload
	jne	.LBB79_9
# %bb.10:
	movl	%edx, 44(%eax)
	movl	108(%esp), %edx                 # 4-byte Reload
	jne	.LBB79_11
.LBB79_12:
	movl	%esi, 40(%eax)
	jne	.LBB79_13
.LBB79_14:
	movl	80(%esp), %esi                  # 4-byte Reload
	movl	%esi, 36(%eax)
	jne	.LBB79_15
.LBB79_16:
	movl	%edx, 32(%eax)
	je	.LBB79_18
.LBB79_17:
	movl	76(%esp), %edx                  # 4-byte Reload
	movl	%edx, 72(%esp)                  # 4-byte Spill
.LBB79_18:
	movl	%ecx, %edx
	movl	72(%esp), %ecx                  # 4-byte Reload
	movl	%ecx, 28(%eax)
	movl	%edi, %esi
	jne	.LBB79_19
# %bb.20:
	movl	68(%esp), %ecx                  # 4-byte Reload
	movl	%ecx, 24(%eax)
	movl	96(%esp), %edi                  # 4-byte Reload
	jne	.LBB79_21
.LBB79_22:
	movl	%ebp, 20(%eax)
	movl	%edx, %ecx
	jne	.LBB79_23
.LBB79_24:
	movl	%ebx, 16(%eax)
	movl	88(%esp), %edx                  # 4-byte Reload
	jne	.LBB79_25
.LBB79_26:
	movl	%edi, 12(%eax)
	jne	.LBB79_27
.LBB79_28:
	movl	%esi, 8(%eax)
	jne	.LBB79_29
.LBB79_30:
	movl	%edx, 4(%eax)
	je	.LBB79_32
.LBB79_31:
	movl	60(%esp), %ecx                  # 4-byte Reload
.LBB79_32:
	movl	%ecx, (%eax)
	addl	$1292, %esp                     # imm = 0x50C
	popl	%esi
	popl	%edi
	popl	%ebx
	popl	%ebp
	retl
.LBB79_9:
	movl	24(%esp), %edx                  # 4-byte Reload
	movl	%edx, 44(%eax)
	movl	108(%esp), %edx                 # 4-byte Reload
	je	.LBB79_12
.LBB79_11:
	movl	32(%esp), %esi                  # 4-byte Reload
	movl	%esi, 40(%eax)
	je	.LBB79_14
.LBB79_13:
	movl	28(%esp), %esi                  # 4-byte Reload
	movl	%esi, 80(%esp)                  # 4-byte Spill
	movl	80(%esp), %esi                  # 4-byte Reload
	movl	%esi, 36(%eax)
	je	.LBB79_16
.LBB79_15:
	movl	12(%esp), %edx                  # 4-byte Reload
	movl	%edx, 32(%eax)
	jne	.LBB79_17
	jmp	.LBB79_18
.LBB79_19:
	movl	48(%esp), %ecx                  # 4-byte Reload
	movl	%ecx, 68(%esp)                  # 4-byte Spill
	movl	68(%esp), %ecx                  # 4-byte Reload
	movl	%ecx, 24(%eax)
	movl	96(%esp), %edi                  # 4-byte Reload
	je	.LBB79_22
.LBB79_21:
	movl	36(%esp), %ebp                  # 4-byte Reload
	movl	%ebp, 20(%eax)
	movl	%edx, %ecx
	je	.LBB79_24
.LBB79_23:
	movl	40(%esp), %ebx                  # 4-byte Reload
	movl	%ebx, 16(%eax)
	movl	88(%esp), %edx                  # 4-byte Reload
	je	.LBB79_26
.LBB79_25:
	movl	52(%esp), %edi                  # 4-byte Reload
	movl	%edi, 12(%eax)
	je	.LBB79_28
.LBB79_27:
	movl	56(%esp), %esi                  # 4-byte Reload
	movl	%esi, 8(%eax)
	je	.LBB79_30
.LBB79_29:
	movl	20(%esp), %edx                  # 4-byte Reload
	movl	%edx, 4(%eax)
	jne	.LBB79_31
	jmp	.LBB79_32
.Lfunc_end79:
	.size	mcl_fp_montRed16L, .Lfunc_end79-mcl_fp_montRed16L
                                        # -- End function
	.globl	mcl_fp_montRedNF16L             # -- Begin function mcl_fp_montRedNF16L
	.p2align	4, 0x90
	.type	mcl_fp_montRedNF16L,@function
mcl_fp_montRedNF16L:                    # @mcl_fp_montRedNF16L
# %bb.0:
	pushl	%ebp
	pushl	%ebx
	pushl	%edi
	pushl	%esi
	subl	$1276, %esp                     # imm = 0x4FC
	calll	.L80$pb
.L80$pb:
	popl	%ebx
.Ltmp22:
	addl	$_GLOBAL_OFFSET_TABLE_+(.Ltmp22-.L80$pb), %ebx
	movl	1304(%esp), %ecx
	movl	60(%ecx), %eax
	movl	%eax, 124(%esp)                 # 4-byte Spill
	movl	56(%ecx), %eax
	movl	%eax, 120(%esp)                 # 4-byte Spill
	movl	52(%ecx), %eax
	movl	%eax, 116(%esp)                 # 4-byte Spill
	movl	48(%ecx), %eax
	movl	%eax, 112(%esp)                 # 4-byte Spill
	movl	44(%ecx), %eax
	movl	%eax, 108(%esp)                 # 4-byte Spill
	movl	40(%ecx), %eax
	movl	%eax, 104(%esp)                 # 4-byte Spill
	movl	36(%ecx), %eax
	movl	%eax, 96(%esp)                  # 4-byte Spill
	movl	32(%ecx), %eax
	movl	%eax, 92(%esp)                  # 4-byte Spill
	movl	28(%ecx), %eax
	movl	%eax, 64(%esp)                  # 4-byte Spill
	movl	24(%ecx), %eax
	movl	%eax, 100(%esp)                 # 4-byte Spill
	movl	20(%ecx), %eax
	movl	%eax, 88(%esp)                  # 4-byte Spill
	movl	16(%ecx), %eax
	movl	%eax, 84(%esp)                  # 4-byte Spill
	movl	12(%ecx), %eax
	movl	%eax, 80(%esp)                  # 4-byte Spill
	movl	8(%ecx), %eax
	movl	%eax, 76(%esp)                  # 4-byte Spill
	movl	4(%ecx), %eax
	movl	%ecx, %edx
	movl	%eax, 72(%esp)                  # 4-byte Spill
	movl	1300(%esp), %eax
	movl	60(%eax), %ecx
	movl	%ecx, 8(%esp)                   # 4-byte Spill
	movl	56(%eax), %ecx
	movl	%ecx, 48(%esp)                  # 4-byte Spill
	movl	52(%eax), %ecx
	movl	%ecx, 12(%esp)                  # 4-byte Spill
	movl	48(%eax), %ecx
	movl	%ecx, 4(%esp)                   # 4-byte Spill
	movl	44(%eax), %ecx
	movl	%ecx, 32(%esp)                  # 4-byte Spill
	movl	40(%eax), %ecx
	movl	%ecx, 24(%esp)                  # 4-byte Spill
	movl	36(%eax), %ecx
	movl	%ecx, 28(%esp)                  # 4-byte Spill
	movl	32(%eax), %ecx
	movl	%ecx, 20(%esp)                  # 4-byte Spill
	movl	28(%eax), %ecx
	movl	%ecx, (%esp)                    # 4-byte Spill
	movl	24(%eax), %ecx
	movl	%ecx, 36(%esp)                  # 4-byte Spill
	movl	20(%eax), %ecx
	movl	%ecx, 40(%esp)                  # 4-byte Spill
	movl	16(%eax), %edi
	movl	12(%eax), %ecx
	movl	%ecx, 16(%esp)                  # 4-byte Spill
	movl	8(%eax), %ecx
	movl	%ecx, 52(%esp)                  # 4-byte Spill
	movl	(%eax), %esi
	movl	4(%eax), %ebp
	movl	-4(%edx), %ecx
	movl	%ecx, 60(%esp)                  # 4-byte Spill
	movl	(%edx), %eax
	movl	%eax, 68(%esp)                  # 4-byte Spill
	subl	$4, %esp
	movl	%esi, %eax
	imull	%ecx, %eax
	leal	1212(%esp), %ecx
	pushl	%eax
	pushl	%edx
	pushl	%ecx
	calll	mulPv512x32@PLT
	addl	$12, %esp
	addl	1208(%esp), %esi
	adcl	1212(%esp), %ebp
	movl	52(%esp), %eax                  # 4-byte Reload
	adcl	1216(%esp), %eax
	movl	%eax, 52(%esp)                  # 4-byte Spill
	movl	16(%esp), %eax                  # 4-byte Reload
	adcl	1220(%esp), %eax
	movl	%eax, 16(%esp)                  # 4-byte Spill
	adcl	1224(%esp), %edi
	movl	%edi, 56(%esp)                  # 4-byte Spill
	movl	40(%esp), %eax                  # 4-byte Reload
	adcl	1228(%esp), %eax
	movl	%eax, 40(%esp)                  # 4-byte Spill
	movl	36(%esp), %eax                  # 4-byte Reload
	adcl	1232(%esp), %eax
	movl	%eax, 36(%esp)                  # 4-byte Spill
	movl	(%esp), %eax                    # 4-byte Reload
	adcl	1236(%esp), %eax
	movl	%eax, (%esp)                    # 4-byte Spill
	movl	20(%esp), %eax                  # 4-byte Reload
	adcl	1240(%esp), %eax
	movl	%eax, 20(%esp)                  # 4-byte Spill
	movl	28(%esp), %eax                  # 4-byte Reload
	adcl	1244(%esp), %eax
	movl	%eax, 28(%esp)                  # 4-byte Spill
	movl	24(%esp), %eax                  # 4-byte Reload
	adcl	1248(%esp), %eax
	movl	%eax, 24(%esp)                  # 4-byte Spill
	movl	32(%esp), %eax                  # 4-byte Reload
	adcl	1252(%esp), %eax
	movl	%eax, 32(%esp)                  # 4-byte Spill
	movl	4(%esp), %eax                   # 4-byte Reload
	adcl	1256(%esp), %eax
	movl	%eax, 4(%esp)                   # 4-byte Spill
	movl	12(%esp), %eax                  # 4-byte Reload
	adcl	1260(%esp), %eax
	movl	%eax, 12(%esp)                  # 4-byte Spill
	movl	48(%esp), %edi                  # 4-byte Reload
	adcl	1264(%esp), %edi
	movl	8(%esp), %esi                   # 4-byte Reload
	adcl	1268(%esp), %esi
	movl	1300(%esp), %eax
	movl	64(%eax), %eax
	adcl	1272(%esp), %eax
	movl	%eax, 44(%esp)                  # 4-byte Spill
	setb	8(%esp)                         # 1-byte Folded Spill
	subl	$4, %esp
	movl	64(%esp), %eax                  # 4-byte Reload
	imull	%ebp, %eax
	leal	1140(%esp), %ecx
	pushl	%eax
	pushl	1312(%esp)
	pushl	%ecx
	calll	mulPv512x32@PLT
	addl	$12, %esp
	addb	$255, 8(%esp)                   # 1-byte Folded Spill
	movl	1200(%esp), %eax
	adcl	$0, %eax
	addl	1136(%esp), %ebp
	movl	52(%esp), %edx                  # 4-byte Reload
	adcl	1140(%esp), %edx
	movl	16(%esp), %ebp                  # 4-byte Reload
	adcl	1144(%esp), %ebp
	movl	56(%esp), %ecx                  # 4-byte Reload
	adcl	1148(%esp), %ecx
	movl	%ecx, 56(%esp)                  # 4-byte Spill
	movl	40(%esp), %ecx                  # 4-byte Reload
	adcl	1152(%esp), %ecx
	movl	%ecx, 40(%esp)                  # 4-byte Spill
	movl	36(%esp), %ecx                  # 4-byte Reload
	adcl	1156(%esp), %ecx
	movl	%ecx, 36(%esp)                  # 4-byte Spill
	movl	(%esp), %ecx                    # 4-byte Reload
	adcl	1160(%esp), %ecx
	movl	%ecx, (%esp)                    # 4-byte Spill
	movl	20(%esp), %ecx                  # 4-byte Reload
	adcl	1164(%esp), %ecx
	movl	%ecx, 20(%esp)                  # 4-byte Spill
	movl	28(%esp), %ecx                  # 4-byte Reload
	adcl	1168(%esp), %ecx
	movl	%ecx, 28(%esp)                  # 4-byte Spill
	movl	24(%esp), %ecx                  # 4-byte Reload
	adcl	1172(%esp), %ecx
	movl	%ecx, 24(%esp)                  # 4-byte Spill
	movl	32(%esp), %ecx                  # 4-byte Reload
	adcl	1176(%esp), %ecx
	movl	%ecx, 32(%esp)                  # 4-byte Spill
	movl	4(%esp), %ecx                   # 4-byte Reload
	adcl	1180(%esp), %ecx
	movl	%ecx, 4(%esp)                   # 4-byte Spill
	movl	12(%esp), %ecx                  # 4-byte Reload
	adcl	1184(%esp), %ecx
	movl	%ecx, 12(%esp)                  # 4-byte Spill
	adcl	1188(%esp), %edi
	movl	%edi, 48(%esp)                  # 4-byte Spill
	adcl	1192(%esp), %esi
	movl	%esi, 8(%esp)                   # 4-byte Spill
	movl	44(%esp), %esi                  # 4-byte Reload
	adcl	1196(%esp), %esi
	movl	1300(%esp), %ecx
	adcl	68(%ecx), %eax
	movl	%eax, 16(%esp)                  # 4-byte Spill
	setb	44(%esp)                        # 1-byte Folded Spill
	subl	$4, %esp
	movl	64(%esp), %eax                  # 4-byte Reload
	imull	%edx, %eax
	movl	%edx, %edi
	leal	1068(%esp), %ecx
	pushl	%eax
	movl	1312(%esp), %eax
	pushl	%eax
	pushl	%ecx
	calll	mulPv512x32@PLT
	addl	$12, %esp
	addb	$255, 44(%esp)                  # 1-byte Folded Spill
	movl	1128(%esp), %eax
	adcl	$0, %eax
	movl	%eax, %edx
	addl	1064(%esp), %edi
	adcl	1068(%esp), %ebp
	movl	%ebp, %ecx
	movl	56(%esp), %eax                  # 4-byte Reload
	adcl	1072(%esp), %eax
	movl	%eax, 56(%esp)                  # 4-byte Spill
	movl	40(%esp), %eax                  # 4-byte Reload
	adcl	1076(%esp), %eax
	movl	%eax, 40(%esp)                  # 4-byte Spill
	movl	36(%esp), %eax                  # 4-byte Reload
	adcl	1080(%esp), %eax
	movl	%eax, 36(%esp)                  # 4-byte Spill
	movl	(%esp), %eax                    # 4-byte Reload
	adcl	1084(%esp), %eax
	movl	%eax, (%esp)                    # 4-byte Spill
	movl	20(%esp), %eax                  # 4-byte Reload
	adcl	1088(%esp), %eax
	movl	%eax, 20(%esp)                  # 4-byte Spill
	movl	28(%esp), %eax                  # 4-byte Reload
	adcl	1092(%esp), %eax
	movl	%eax, 28(%esp)                  # 4-byte Spill
	movl	24(%esp), %eax                  # 4-byte Reload
	adcl	1096(%esp), %eax
	movl	%eax, 24(%esp)                  # 4-byte Spill
	movl	32(%esp), %eax                  # 4-byte Reload
	adcl	1100(%esp), %eax
	movl	%eax, 32(%esp)                  # 4-byte Spill
	movl	4(%esp), %eax                   # 4-byte Reload
	adcl	1104(%esp), %eax
	movl	%eax, 4(%esp)                   # 4-byte Spill
	movl	12(%esp), %eax                  # 4-byte Reload
	adcl	1108(%esp), %eax
	movl	%eax, 12(%esp)                  # 4-byte Spill
	movl	48(%esp), %ebp                  # 4-byte Reload
	adcl	1112(%esp), %ebp
	movl	8(%esp), %eax                   # 4-byte Reload
	adcl	1116(%esp), %eax
	movl	%eax, 8(%esp)                   # 4-byte Spill
	adcl	1120(%esp), %esi
	movl	%esi, 44(%esp)                  # 4-byte Spill
	movl	16(%esp), %esi                  # 4-byte Reload
	adcl	1124(%esp), %esi
	movl	1300(%esp), %eax
	adcl	72(%eax), %edx
	movl	%edx, 52(%esp)                  # 4-byte Spill
	setb	16(%esp)                        # 1-byte Folded Spill
	subl	$4, %esp
	movl	64(%esp), %eax                  # 4-byte Reload
	movl	%ecx, %edi
	imull	%ecx, %eax
	leal	996(%esp), %ecx
	pushl	%eax
	pushl	1312(%esp)
	pushl	%ecx
	calll	mulPv512x32@PLT
	addl	$12, %esp
	addb	$255, 16(%esp)                  # 1-byte Folded Spill
	movl	1056(%esp), %eax
	adcl	$0, %eax
	addl	992(%esp), %edi
	movl	56(%esp), %edx                  # 4-byte Reload
	adcl	996(%esp), %edx
	movl	40(%esp), %ecx                  # 4-byte Reload
	adcl	1000(%esp), %ecx
	movl	%ecx, 40(%esp)                  # 4-byte Spill
	movl	36(%esp), %ecx                  # 4-byte Reload
	adcl	1004(%esp), %ecx
	movl	%ecx, 36(%esp)                  # 4-byte Spill
	movl	(%esp), %ecx                    # 4-byte Reload
	adcl	1008(%esp), %ecx
	movl	%ecx, (%esp)                    # 4-byte Spill
	movl	20(%esp), %ecx                  # 4-byte Reload
	adcl	1012(%esp), %ecx
	movl	%ecx, 20(%esp)                  # 4-byte Spill
	movl	28(%esp), %ecx                  # 4-byte Reload
	adcl	1016(%esp), %ecx
	movl	%ecx, 28(%esp)                  # 4-byte Spill
	movl	24(%esp), %ecx                  # 4-byte Reload
	adcl	1020(%esp), %ecx
	movl	%ecx, 24(%esp)                  # 4-byte Spill
	movl	32(%esp), %ecx                  # 4-byte Reload
	adcl	1024(%esp), %ecx
	movl	%ecx, 32(%esp)                  # 4-byte Spill
	movl	4(%esp), %ecx                   # 4-byte Reload
	adcl	1028(%esp), %ecx
	movl	%ecx, 4(%esp)                   # 4-byte Spill
	movl	12(%esp), %edi                  # 4-byte Reload
	adcl	1032(%esp), %edi
	adcl	1036(%esp), %ebp
	movl	%ebp, 48(%esp)                  # 4-byte Spill
	movl	8(%esp), %ecx                   # 4-byte Reload
	adcl	1040(%esp), %ecx
	movl	%ecx, 8(%esp)                   # 4-byte Spill
	movl	44(%esp), %ecx                  # 4-byte Reload
	adcl	1044(%esp), %ecx
	movl	%ecx, 44(%esp)                  # 4-byte Spill
	adcl	1048(%esp), %esi
	movl	%esi, 16(%esp)                  # 4-byte Spill
	movl	52(%esp), %esi                  # 4-byte Reload
	adcl	1052(%esp), %esi
	movl	1300(%esp), %ecx
	adcl	76(%ecx), %eax
	movl	%eax, 56(%esp)                  # 4-byte Spill
	setb	12(%esp)                        # 1-byte Folded Spill
	subl	$4, %esp
	movl	64(%esp), %eax                  # 4-byte Reload
	imull	%edx, %eax
	movl	%edx, %ebp
	leal	924(%esp), %ecx
	pushl	%eax
	pushl	1312(%esp)
	pushl	%ecx
	calll	mulPv512x32@PLT
	addl	$12, %esp
	addb	$255, 12(%esp)                  # 1-byte Folded Spill
	movl	984(%esp), %eax
	adcl	$0, %eax
	addl	920(%esp), %ebp
	movl	40(%esp), %edx                  # 4-byte Reload
	adcl	924(%esp), %edx
	movl	36(%esp), %ecx                  # 4-byte Reload
	adcl	928(%esp), %ecx
	movl	%ecx, 36(%esp)                  # 4-byte Spill
	movl	(%esp), %ecx                    # 4-byte Reload
	adcl	932(%esp), %ecx
	movl	%ecx, (%esp)                    # 4-byte Spill
	movl	20(%esp), %ecx                  # 4-byte Reload
	adcl	936(%esp), %ecx
	movl	%ecx, 20(%esp)                  # 4-byte Spill
	movl	28(%esp), %ecx                  # 4-byte Reload
	adcl	940(%esp), %ecx
	movl	%ecx, 28(%esp)                  # 4-byte Spill
	movl	24(%esp), %ecx                  # 4-byte Reload
	adcl	944(%esp), %ecx
	movl	%ecx, 24(%esp)                  # 4-byte Spill
	movl	32(%esp), %ecx                  # 4-byte Reload
	adcl	948(%esp), %ecx
	movl	%ecx, 32(%esp)                  # 4-byte Spill
	movl	4(%esp), %ecx                   # 4-byte Reload
	adcl	952(%esp), %ecx
	movl	%ecx, 4(%esp)                   # 4-byte Spill
	adcl	956(%esp), %edi
	movl	%edi, 12(%esp)                  # 4-byte Spill
	movl	48(%esp), %ecx                  # 4-byte Reload
	adcl	960(%esp), %ecx
	movl	%ecx, 48(%esp)                  # 4-byte Spill
	movl	8(%esp), %ecx                   # 4-byte Reload
	adcl	964(%esp), %ecx
	movl	%ecx, 8(%esp)                   # 4-byte Spill
	movl	44(%esp), %ecx                  # 4-byte Reload
	adcl	968(%esp), %ecx
	movl	%ecx, 44(%esp)                  # 4-byte Spill
	movl	16(%esp), %ecx                  # 4-byte Reload
	adcl	972(%esp), %ecx
	movl	%ecx, 16(%esp)                  # 4-byte Spill
	adcl	976(%esp), %esi
	movl	%esi, %ebp
	movl	56(%esp), %edi                  # 4-byte Reload
	adcl	980(%esp), %edi
	movl	1300(%esp), %ecx
	adcl	80(%ecx), %eax
	movl	%eax, 40(%esp)                  # 4-byte Spill
	setb	52(%esp)                        # 1-byte Folded Spill
	subl	$4, %esp
	movl	64(%esp), %eax                  # 4-byte Reload
	movl	%edx, %esi
	imull	%edx, %eax
	leal	852(%esp), %ecx
	pushl	%eax
	pushl	1312(%esp)
	pushl	%ecx
	calll	mulPv512x32@PLT
	addl	$12, %esp
	addb	$255, 52(%esp)                  # 1-byte Folded Spill
	movl	912(%esp), %eax
	adcl	$0, %eax
	addl	848(%esp), %esi
	movl	36(%esp), %edx                  # 4-byte Reload
	adcl	852(%esp), %edx
	movl	(%esp), %ecx                    # 4-byte Reload
	adcl	856(%esp), %ecx
	movl	%ecx, (%esp)                    # 4-byte Spill
	movl	20(%esp), %ecx                  # 4-byte Reload
	adcl	860(%esp), %ecx
	movl	%ecx, 20(%esp)                  # 4-byte Spill
	movl	28(%esp), %ecx                  # 4-byte Reload
	adcl	864(%esp), %ecx
	movl	%ecx, 28(%esp)                  # 4-byte Spill
	movl	24(%esp), %ecx                  # 4-byte Reload
	adcl	868(%esp), %ecx
	movl	%ecx, 24(%esp)                  # 4-byte Spill
	movl	32(%esp), %ecx                  # 4-byte Reload
	adcl	872(%esp), %ecx
	movl	%ecx, 32(%esp)                  # 4-byte Spill
	movl	4(%esp), %ecx                   # 4-byte Reload
	adcl	876(%esp), %ecx
	movl	%ecx, 4(%esp)                   # 4-byte Spill
	movl	12(%esp), %ecx                  # 4-byte Reload
	adcl	880(%esp), %ecx
	movl	%ecx, 12(%esp)                  # 4-byte Spill
	movl	48(%esp), %ecx                  # 4-byte Reload
	adcl	884(%esp), %ecx
	movl	%ecx, 48(%esp)                  # 4-byte Spill
	movl	8(%esp), %esi                   # 4-byte Reload
	adcl	888(%esp), %esi
	movl	44(%esp), %ecx                  # 4-byte Reload
	adcl	892(%esp), %ecx
	movl	%ecx, 44(%esp)                  # 4-byte Spill
	movl	16(%esp), %ecx                  # 4-byte Reload
	adcl	896(%esp), %ecx
	movl	%ecx, 16(%esp)                  # 4-byte Spill
	adcl	900(%esp), %ebp
	movl	%ebp, 52(%esp)                  # 4-byte Spill
	adcl	904(%esp), %edi
	movl	%edi, %ebp
	movl	40(%esp), %ecx                  # 4-byte Reload
	adcl	908(%esp), %ecx
	movl	%ecx, 40(%esp)                  # 4-byte Spill
	movl	1300(%esp), %ecx
	adcl	84(%ecx), %eax
	movl	%eax, 36(%esp)                  # 4-byte Spill
	setb	8(%esp)                         # 1-byte Folded Spill
	subl	$4, %esp
	movl	64(%esp), %eax                  # 4-byte Reload
	movl	%edx, %edi
	imull	%edx, %eax
	leal	780(%esp), %ecx
	pushl	%eax
	pushl	1312(%esp)
	pushl	%ecx
	calll	mulPv512x32@PLT
	addl	$12, %esp
	addb	$255, 8(%esp)                   # 1-byte Folded Spill
	movl	840(%esp), %eax
	adcl	$0, %eax
	addl	776(%esp), %edi
	movl	(%esp), %edx                    # 4-byte Reload
	adcl	780(%esp), %edx
	movl	20(%esp), %ecx                  # 4-byte Reload
	adcl	784(%esp), %ecx
	movl	%ecx, 20(%esp)                  # 4-byte Spill
	movl	28(%esp), %ecx                  # 4-byte Reload
	adcl	788(%esp), %ecx
	movl	%ecx, 28(%esp)                  # 4-byte Spill
	movl	24(%esp), %ecx                  # 4-byte Reload
	adcl	792(%esp), %ecx
	movl	%ecx, 24(%esp)                  # 4-byte Spill
	movl	32(%esp), %edi                  # 4-byte Reload
	adcl	796(%esp), %edi
	movl	4(%esp), %ecx                   # 4-byte Reload
	adcl	800(%esp), %ecx
	movl	%ecx, 4(%esp)                   # 4-byte Spill
	movl	12(%esp), %ecx                  # 4-byte Reload
	adcl	804(%esp), %ecx
	movl	%ecx, 12(%esp)                  # 4-byte Spill
	movl	48(%esp), %ecx                  # 4-byte Reload
	adcl	808(%esp), %ecx
	movl	%ecx, 48(%esp)                  # 4-byte Spill
	adcl	812(%esp), %esi
	movl	%esi, 8(%esp)                   # 4-byte Spill
	movl	44(%esp), %esi                  # 4-byte Reload
	adcl	816(%esp), %esi
	movl	16(%esp), %ecx                  # 4-byte Reload
	adcl	820(%esp), %ecx
	movl	%ecx, 16(%esp)                  # 4-byte Spill
	movl	52(%esp), %ecx                  # 4-byte Reload
	adcl	824(%esp), %ecx
	movl	%ecx, 52(%esp)                  # 4-byte Spill
	adcl	828(%esp), %ebp
	movl	%ebp, 56(%esp)                  # 4-byte Spill
	movl	40(%esp), %ecx                  # 4-byte Reload
	adcl	832(%esp), %ecx
	movl	%ecx, 40(%esp)                  # 4-byte Spill
	movl	36(%esp), %ecx                  # 4-byte Reload
	adcl	836(%esp), %ecx
	movl	%ecx, 36(%esp)                  # 4-byte Spill
	movl	1300(%esp), %ecx
	adcl	88(%ecx), %eax
	movl	%eax, (%esp)                    # 4-byte Spill
	setb	44(%esp)                        # 1-byte Folded Spill
	subl	$4, %esp
	movl	64(%esp), %eax                  # 4-byte Reload
	movl	%edx, %ebp
	imull	%edx, %eax
	leal	708(%esp), %ecx
	pushl	%eax
	pushl	1312(%esp)
	pushl	%ecx
	calll	mulPv512x32@PLT
	addl	$12, %esp
	addb	$255, 44(%esp)                  # 1-byte Folded Spill
	movl	768(%esp), %eax
	adcl	$0, %eax
	addl	704(%esp), %ebp
	movl	20(%esp), %edx                  # 4-byte Reload
	adcl	708(%esp), %edx
	movl	28(%esp), %ecx                  # 4-byte Reload
	adcl	712(%esp), %ecx
	movl	%ecx, 28(%esp)                  # 4-byte Spill
	movl	24(%esp), %ebp                  # 4-byte Reload
	adcl	716(%esp), %ebp
	adcl	720(%esp), %edi
	movl	%edi, 32(%esp)                  # 4-byte Spill
	movl	4(%esp), %edi                   # 4-byte Reload
	adcl	724(%esp), %edi
	movl	12(%esp), %ecx                  # 4-byte Reload
	adcl	728(%esp), %ecx
	movl	%ecx, 12(%esp)                  # 4-byte Spill
	movl	48(%esp), %ecx                  # 4-byte Reload
	adcl	732(%esp), %ecx
	movl	%ecx, 48(%esp)                  # 4-byte Spill
	movl	8(%esp), %ecx                   # 4-byte Reload
	adcl	736(%esp), %ecx
	movl	%ecx, 8(%esp)                   # 4-byte Spill
	adcl	740(%esp), %esi
	movl	%esi, 44(%esp)                  # 4-byte Spill
	movl	16(%esp), %ecx                  # 4-byte Reload
	adcl	744(%esp), %ecx
	movl	%ecx, 16(%esp)                  # 4-byte Spill
	movl	52(%esp), %ecx                  # 4-byte Reload
	adcl	748(%esp), %ecx
	movl	%ecx, 52(%esp)                  # 4-byte Spill
	movl	56(%esp), %ecx                  # 4-byte Reload
	adcl	752(%esp), %ecx
	movl	%ecx, 56(%esp)                  # 4-byte Spill
	movl	40(%esp), %ecx                  # 4-byte Reload
	adcl	756(%esp), %ecx
	movl	%ecx, 40(%esp)                  # 4-byte Spill
	movl	36(%esp), %ecx                  # 4-byte Reload
	adcl	760(%esp), %ecx
	movl	%ecx, 36(%esp)                  # 4-byte Spill
	movl	(%esp), %ecx                    # 4-byte Reload
	adcl	764(%esp), %ecx
	movl	%ecx, (%esp)                    # 4-byte Spill
	movl	1300(%esp), %ecx
	adcl	92(%ecx), %eax
	movl	%eax, 20(%esp)                  # 4-byte Spill
	setb	4(%esp)                         # 1-byte Folded Spill
	subl	$4, %esp
	movl	64(%esp), %eax                  # 4-byte Reload
	imull	%edx, %eax
	movl	%edx, %esi
	leal	636(%esp), %ecx
	pushl	%eax
	movl	1312(%esp), %eax
	pushl	%eax
	pushl	%ecx
	calll	mulPv512x32@PLT
	addl	$12, %esp
	addb	$255, 4(%esp)                   # 1-byte Folded Spill
	movl	696(%esp), %eax
	adcl	$0, %eax
	movl	%eax, %edx
	addl	632(%esp), %esi
	movl	28(%esp), %ecx                  # 4-byte Reload
	adcl	636(%esp), %ecx
	adcl	640(%esp), %ebp
	movl	%ebp, 24(%esp)                  # 4-byte Spill
	movl	32(%esp), %eax                  # 4-byte Reload
	adcl	644(%esp), %eax
	movl	%eax, 32(%esp)                  # 4-byte Spill
	adcl	648(%esp), %edi
	movl	%edi, 4(%esp)                   # 4-byte Spill
	movl	12(%esp), %eax                  # 4-byte Reload
	adcl	652(%esp), %eax
	movl	%eax, 12(%esp)                  # 4-byte Spill
	movl	48(%esp), %edi                  # 4-byte Reload
	adcl	656(%esp), %edi
	movl	8(%esp), %eax                   # 4-byte Reload
	adcl	660(%esp), %eax
	movl	%eax, 8(%esp)                   # 4-byte Spill
	movl	44(%esp), %eax                  # 4-byte Reload
	adcl	664(%esp), %eax
	movl	%eax, 44(%esp)                  # 4-byte Spill
	movl	16(%esp), %eax                  # 4-byte Reload
	adcl	668(%esp), %eax
	movl	%eax, 16(%esp)                  # 4-byte Spill
	movl	52(%esp), %eax                  # 4-byte Reload
	adcl	672(%esp), %eax
	movl	%eax, 52(%esp)                  # 4-byte Spill
	movl	56(%esp), %ebp                  # 4-byte Reload
	adcl	676(%esp), %ebp
	movl	40(%esp), %eax                  # 4-byte Reload
	adcl	680(%esp), %eax
	movl	%eax, 40(%esp)                  # 4-byte Spill
	movl	36(%esp), %eax                  # 4-byte Reload
	adcl	684(%esp), %eax
	movl	%eax, 36(%esp)                  # 4-byte Spill
	movl	(%esp), %eax                    # 4-byte Reload
	adcl	688(%esp), %eax
	movl	%eax, (%esp)                    # 4-byte Spill
	movl	20(%esp), %eax                  # 4-byte Reload
	adcl	692(%esp), %eax
	movl	%eax, 20(%esp)                  # 4-byte Spill
	movl	1300(%esp), %eax
	adcl	96(%eax), %edx
	movl	%edx, 28(%esp)                  # 4-byte Spill
	setb	48(%esp)                        # 1-byte Folded Spill
	subl	$4, %esp
	movl	64(%esp), %eax                  # 4-byte Reload
	movl	%ecx, %esi
	imull	%ecx, %eax
	leal	564(%esp), %ecx
	pushl	%eax
	pushl	1312(%esp)
	pushl	%ecx
	calll	mulPv512x32@PLT
	addl	$12, %esp
	addb	$255, 48(%esp)                  # 1-byte Folded Spill
	movl	624(%esp), %eax
	adcl	$0, %eax
	movl	%eax, %edx
	addl	560(%esp), %esi
	movl	24(%esp), %ecx                  # 4-byte Reload
	adcl	564(%esp), %ecx
	movl	32(%esp), %eax                  # 4-byte Reload
	adcl	568(%esp), %eax
	movl	%eax, 32(%esp)                  # 4-byte Spill
	movl	4(%esp), %esi                   # 4-byte Reload
	adcl	572(%esp), %esi
	movl	12(%esp), %eax                  # 4-byte Reload
	adcl	576(%esp), %eax
	movl	%eax, 12(%esp)                  # 4-byte Spill
	adcl	580(%esp), %edi
	movl	%edi, 48(%esp)                  # 4-byte Spill
	movl	8(%esp), %eax                   # 4-byte Reload
	adcl	584(%esp), %eax
	movl	%eax, 8(%esp)                   # 4-byte Spill
	movl	44(%esp), %eax                  # 4-byte Reload
	adcl	588(%esp), %eax
	movl	%eax, 44(%esp)                  # 4-byte Spill
	movl	16(%esp), %eax                  # 4-byte Reload
	adcl	592(%esp), %eax
	movl	%eax, 16(%esp)                  # 4-byte Spill
	movl	52(%esp), %eax                  # 4-byte Reload
	adcl	596(%esp), %eax
	movl	%eax, 52(%esp)                  # 4-byte Spill
	adcl	600(%esp), %ebp
	movl	%ebp, 56(%esp)                  # 4-byte Spill
	movl	40(%esp), %eax                  # 4-byte Reload
	adcl	604(%esp), %eax
	movl	%eax, 40(%esp)                  # 4-byte Spill
	movl	36(%esp), %eax                  # 4-byte Reload
	adcl	608(%esp), %eax
	movl	%eax, 36(%esp)                  # 4-byte Spill
	movl	(%esp), %eax                    # 4-byte Reload
	adcl	612(%esp), %eax
	movl	%eax, (%esp)                    # 4-byte Spill
	movl	20(%esp), %ebp                  # 4-byte Reload
	adcl	616(%esp), %ebp
	movl	28(%esp), %eax                  # 4-byte Reload
	adcl	620(%esp), %eax
	movl	%eax, 28(%esp)                  # 4-byte Spill
	movl	1300(%esp), %eax
	adcl	100(%eax), %edx
	movl	%edx, 24(%esp)                  # 4-byte Spill
	setb	4(%esp)                         # 1-byte Folded Spill
	subl	$4, %esp
	movl	64(%esp), %eax                  # 4-byte Reload
	imull	%ecx, %eax
	movl	%ecx, %edi
	leal	492(%esp), %ecx
	pushl	%eax
	pushl	1312(%esp)
	pushl	%ecx
	calll	mulPv512x32@PLT
	addl	$12, %esp
	addb	$255, 4(%esp)                   # 1-byte Folded Spill
	movl	552(%esp), %eax
	adcl	$0, %eax
	movl	%eax, %edx
	addl	488(%esp), %edi
	movl	32(%esp), %ecx                  # 4-byte Reload
	adcl	492(%esp), %ecx
	adcl	496(%esp), %esi
	movl	%esi, 4(%esp)                   # 4-byte Spill
	movl	12(%esp), %eax                  # 4-byte Reload
	adcl	500(%esp), %eax
	movl	%eax, 12(%esp)                  # 4-byte Spill
	movl	48(%esp), %eax                  # 4-byte Reload
	adcl	504(%esp), %eax
	movl	%eax, 48(%esp)                  # 4-byte Spill
	movl	8(%esp), %eax                   # 4-byte Reload
	adcl	508(%esp), %eax
	movl	%eax, 8(%esp)                   # 4-byte Spill
	movl	44(%esp), %eax                  # 4-byte Reload
	adcl	512(%esp), %eax
	movl	%eax, 44(%esp)                  # 4-byte Spill
	movl	16(%esp), %edi                  # 4-byte Reload
	adcl	516(%esp), %edi
	movl	52(%esp), %eax                  # 4-byte Reload
	adcl	520(%esp), %eax
	movl	%eax, 52(%esp)                  # 4-byte Spill
	movl	56(%esp), %eax                  # 4-byte Reload
	adcl	524(%esp), %eax
	movl	%eax, 56(%esp)                  # 4-byte Spill
	movl	40(%esp), %eax                  # 4-byte Reload
	adcl	528(%esp), %eax
	movl	%eax, 40(%esp)                  # 4-byte Spill
	movl	36(%esp), %eax                  # 4-byte Reload
	adcl	532(%esp), %eax
	movl	%eax, 36(%esp)                  # 4-byte Spill
	movl	(%esp), %eax                    # 4-byte Reload
	adcl	536(%esp), %eax
	movl	%eax, (%esp)                    # 4-byte Spill
	adcl	540(%esp), %ebp
	movl	%ebp, 20(%esp)                  # 4-byte Spill
	movl	28(%esp), %ebp                  # 4-byte Reload
	adcl	544(%esp), %ebp
	movl	24(%esp), %eax                  # 4-byte Reload
	adcl	548(%esp), %eax
	movl	%eax, 24(%esp)                  # 4-byte Spill
	movl	1300(%esp), %eax
	adcl	104(%eax), %edx
	movl	%edx, 32(%esp)                  # 4-byte Spill
	setb	16(%esp)                        # 1-byte Folded Spill
	subl	$4, %esp
	movl	64(%esp), %eax                  # 4-byte Reload
	imull	%ecx, %eax
	movl	%ecx, %esi
	leal	420(%esp), %ecx
	pushl	%eax
	pushl	1312(%esp)
	pushl	%ecx
	calll	mulPv512x32@PLT
	addl	$12, %esp
	addb	$255, 16(%esp)                  # 1-byte Folded Spill
	movl	480(%esp), %eax
	adcl	$0, %eax
	movl	%eax, %edx
	addl	416(%esp), %esi
	movl	4(%esp), %ecx                   # 4-byte Reload
	adcl	420(%esp), %ecx
	movl	12(%esp), %eax                  # 4-byte Reload
	adcl	424(%esp), %eax
	movl	%eax, 12(%esp)                  # 4-byte Spill
	movl	48(%esp), %eax                  # 4-byte Reload
	adcl	428(%esp), %eax
	movl	%eax, 48(%esp)                  # 4-byte Spill
	movl	8(%esp), %eax                   # 4-byte Reload
	adcl	432(%esp), %eax
	movl	%eax, 8(%esp)                   # 4-byte Spill
	movl	44(%esp), %eax                  # 4-byte Reload
	adcl	436(%esp), %eax
	movl	%eax, 44(%esp)                  # 4-byte Spill
	adcl	440(%esp), %edi
	movl	52(%esp), %eax                  # 4-byte Reload
	adcl	444(%esp), %eax
	movl	%eax, 52(%esp)                  # 4-byte Spill
	movl	56(%esp), %eax                  # 4-byte Reload
	adcl	448(%esp), %eax
	movl	%eax, 56(%esp)                  # 4-byte Spill
	movl	40(%esp), %eax                  # 4-byte Reload
	adcl	452(%esp), %eax
	movl	%eax, 40(%esp)                  # 4-byte Spill
	movl	36(%esp), %eax                  # 4-byte Reload
	adcl	456(%esp), %eax
	movl	%eax, 36(%esp)                  # 4-byte Spill
	movl	(%esp), %eax                    # 4-byte Reload
	adcl	460(%esp), %eax
	movl	%eax, (%esp)                    # 4-byte Spill
	movl	20(%esp), %eax                  # 4-byte Reload
	adcl	464(%esp), %eax
	movl	%eax, 20(%esp)                  # 4-byte Spill
	adcl	468(%esp), %ebp
	movl	%ebp, 28(%esp)                  # 4-byte Spill
	movl	24(%esp), %eax                  # 4-byte Reload
	adcl	472(%esp), %eax
	movl	%eax, 24(%esp)                  # 4-byte Spill
	movl	32(%esp), %eax                  # 4-byte Reload
	adcl	476(%esp), %eax
	movl	%eax, 32(%esp)                  # 4-byte Spill
	movl	1300(%esp), %eax
	adcl	108(%eax), %edx
	movl	%edx, 4(%esp)                   # 4-byte Spill
	setb	16(%esp)                        # 1-byte Folded Spill
	subl	$4, %esp
	movl	64(%esp), %eax                  # 4-byte Reload
	imull	%ecx, %eax
	movl	%ecx, %esi
	leal	348(%esp), %ecx
	pushl	%eax
	pushl	1312(%esp)
	pushl	%ecx
	calll	mulPv512x32@PLT
	addl	$12, %esp
	addb	$255, 16(%esp)                  # 1-byte Folded Spill
	movl	408(%esp), %eax
	adcl	$0, %eax
	movl	%eax, %edx
	addl	344(%esp), %esi
	movl	12(%esp), %esi                  # 4-byte Reload
	adcl	348(%esp), %esi
	movl	48(%esp), %eax                  # 4-byte Reload
	adcl	352(%esp), %eax
	movl	%eax, 48(%esp)                  # 4-byte Spill
	movl	8(%esp), %ebp                   # 4-byte Reload
	adcl	356(%esp), %ebp
	movl	44(%esp), %eax                  # 4-byte Reload
	adcl	360(%esp), %eax
	movl	%eax, 44(%esp)                  # 4-byte Spill
	adcl	364(%esp), %edi
	movl	%edi, 16(%esp)                  # 4-byte Spill
	movl	52(%esp), %eax                  # 4-byte Reload
	adcl	368(%esp), %eax
	movl	%eax, 52(%esp)                  # 4-byte Spill
	movl	56(%esp), %eax                  # 4-byte Reload
	adcl	372(%esp), %eax
	movl	%eax, 56(%esp)                  # 4-byte Spill
	movl	40(%esp), %edi                  # 4-byte Reload
	adcl	376(%esp), %edi
	movl	36(%esp), %eax                  # 4-byte Reload
	adcl	380(%esp), %eax
	movl	%eax, 36(%esp)                  # 4-byte Spill
	movl	(%esp), %eax                    # 4-byte Reload
	adcl	384(%esp), %eax
	movl	%eax, (%esp)                    # 4-byte Spill
	movl	20(%esp), %eax                  # 4-byte Reload
	adcl	388(%esp), %eax
	movl	%eax, 20(%esp)                  # 4-byte Spill
	movl	28(%esp), %eax                  # 4-byte Reload
	adcl	392(%esp), %eax
	movl	%eax, 28(%esp)                  # 4-byte Spill
	movl	24(%esp), %eax                  # 4-byte Reload
	adcl	396(%esp), %eax
	movl	%eax, 24(%esp)                  # 4-byte Spill
	movl	32(%esp), %eax                  # 4-byte Reload
	adcl	400(%esp), %eax
	movl	%eax, 32(%esp)                  # 4-byte Spill
	movl	4(%esp), %eax                   # 4-byte Reload
	adcl	404(%esp), %eax
	movl	%eax, 4(%esp)                   # 4-byte Spill
	movl	1300(%esp), %eax
	adcl	112(%eax), %edx
	movl	%edx, 12(%esp)                  # 4-byte Spill
	setb	8(%esp)                         # 1-byte Folded Spill
	subl	$4, %esp
	movl	64(%esp), %eax                  # 4-byte Reload
	imull	%esi, %eax
	leal	276(%esp), %ecx
	pushl	%eax
	pushl	1312(%esp)
	pushl	%ecx
	calll	mulPv512x32@PLT
	addl	$12, %esp
	addb	$255, 8(%esp)                   # 1-byte Folded Spill
	movl	336(%esp), %eax
	adcl	$0, %eax
	movl	%eax, %edx
	addl	272(%esp), %esi
	movl	48(%esp), %ecx                  # 4-byte Reload
	adcl	276(%esp), %ecx
	adcl	280(%esp), %ebp
	movl	%ebp, 8(%esp)                   # 4-byte Spill
	movl	44(%esp), %eax                  # 4-byte Reload
	adcl	284(%esp), %eax
	movl	%eax, 44(%esp)                  # 4-byte Spill
	movl	16(%esp), %eax                  # 4-byte Reload
	adcl	288(%esp), %eax
	movl	%eax, 16(%esp)                  # 4-byte Spill
	movl	52(%esp), %esi                  # 4-byte Reload
	adcl	292(%esp), %esi
	movl	56(%esp), %eax                  # 4-byte Reload
	adcl	296(%esp), %eax
	movl	%eax, 56(%esp)                  # 4-byte Spill
	adcl	300(%esp), %edi
	movl	%edi, 40(%esp)                  # 4-byte Spill
	movl	36(%esp), %edi                  # 4-byte Reload
	adcl	304(%esp), %edi
	movl	(%esp), %eax                    # 4-byte Reload
	adcl	308(%esp), %eax
	movl	%eax, (%esp)                    # 4-byte Spill
	movl	20(%esp), %eax                  # 4-byte Reload
	adcl	312(%esp), %eax
	movl	%eax, 20(%esp)                  # 4-byte Spill
	movl	28(%esp), %eax                  # 4-byte Reload
	adcl	316(%esp), %eax
	movl	%eax, 28(%esp)                  # 4-byte Spill
	movl	24(%esp), %eax                  # 4-byte Reload
	adcl	320(%esp), %eax
	movl	%eax, 24(%esp)                  # 4-byte Spill
	movl	32(%esp), %eax                  # 4-byte Reload
	adcl	324(%esp), %eax
	movl	%eax, 32(%esp)                  # 4-byte Spill
	movl	4(%esp), %eax                   # 4-byte Reload
	adcl	328(%esp), %eax
	movl	%eax, 4(%esp)                   # 4-byte Spill
	movl	12(%esp), %eax                  # 4-byte Reload
	adcl	332(%esp), %eax
	movl	%eax, 12(%esp)                  # 4-byte Spill
	movl	1300(%esp), %eax
	adcl	116(%eax), %edx
	movl	%edx, 48(%esp)                  # 4-byte Spill
	setb	52(%esp)                        # 1-byte Folded Spill
	subl	$4, %esp
	movl	64(%esp), %eax                  # 4-byte Reload
	imull	%ecx, %eax
	movl	%ecx, %ebp
	leal	204(%esp), %ecx
	pushl	%eax
	pushl	1312(%esp)
	pushl	%ecx
	calll	mulPv512x32@PLT
	addl	$12, %esp
	addb	$255, 52(%esp)                  # 1-byte Folded Spill
	movl	264(%esp), %eax
	adcl	$0, %eax
	movl	%eax, %edx
	addl	200(%esp), %ebp
	movl	8(%esp), %eax                   # 4-byte Reload
	adcl	204(%esp), %eax
	movl	44(%esp), %ecx                  # 4-byte Reload
	adcl	208(%esp), %ecx
	movl	%ecx, 44(%esp)                  # 4-byte Spill
	movl	16(%esp), %ecx                  # 4-byte Reload
	adcl	212(%esp), %ecx
	movl	%ecx, 16(%esp)                  # 4-byte Spill
	adcl	216(%esp), %esi
	movl	%esi, 52(%esp)                  # 4-byte Spill
	movl	56(%esp), %ecx                  # 4-byte Reload
	adcl	220(%esp), %ecx
	movl	%ecx, 56(%esp)                  # 4-byte Spill
	movl	40(%esp), %ecx                  # 4-byte Reload
	adcl	224(%esp), %ecx
	movl	%ecx, 40(%esp)                  # 4-byte Spill
	adcl	228(%esp), %edi
	movl	%edi, 36(%esp)                  # 4-byte Spill
	movl	(%esp), %ebp                    # 4-byte Reload
	adcl	232(%esp), %ebp
	movl	20(%esp), %ecx                  # 4-byte Reload
	adcl	236(%esp), %ecx
	movl	%ecx, 20(%esp)                  # 4-byte Spill
	movl	28(%esp), %ecx                  # 4-byte Reload
	adcl	240(%esp), %ecx
	movl	%ecx, 28(%esp)                  # 4-byte Spill
	movl	24(%esp), %ecx                  # 4-byte Reload
	adcl	244(%esp), %ecx
	movl	%ecx, 24(%esp)                  # 4-byte Spill
	movl	32(%esp), %ecx                  # 4-byte Reload
	adcl	248(%esp), %ecx
	movl	%ecx, 32(%esp)                  # 4-byte Spill
	movl	4(%esp), %ecx                   # 4-byte Reload
	adcl	252(%esp), %ecx
	movl	%ecx, 4(%esp)                   # 4-byte Spill
	movl	12(%esp), %ecx                  # 4-byte Reload
	adcl	256(%esp), %ecx
	movl	%ecx, 12(%esp)                  # 4-byte Spill
	movl	48(%esp), %ecx                  # 4-byte Reload
	adcl	260(%esp), %ecx
	movl	%ecx, 48(%esp)                  # 4-byte Spill
	movl	1300(%esp), %ecx
	adcl	120(%ecx), %edx
	movl	%edx, 8(%esp)                   # 4-byte Spill
	setb	(%esp)                          # 1-byte Folded Spill
	movl	60(%esp), %ecx                  # 4-byte Reload
	imull	%eax, %ecx
	movl	%eax, %esi
	subl	$4, %esp
	leal	132(%esp), %eax
	pushl	%ecx
	pushl	1312(%esp)
	pushl	%eax
	calll	mulPv512x32@PLT
	addl	$12, %esp
	addb	$255, (%esp)                    # 1-byte Folded Spill
	movl	192(%esp), %eax
	adcl	$0, %eax
	addl	128(%esp), %esi
	movl	44(%esp), %ecx                  # 4-byte Reload
	adcl	132(%esp), %ecx
	movl	16(%esp), %edx                  # 4-byte Reload
	adcl	136(%esp), %edx
	movl	%edx, 16(%esp)                  # 4-byte Spill
	movl	52(%esp), %edx                  # 4-byte Reload
	adcl	140(%esp), %edx
	movl	56(%esp), %esi                  # 4-byte Reload
	adcl	144(%esp), %esi
	movl	40(%esp), %edi                  # 4-byte Reload
	adcl	148(%esp), %edi
	movl	36(%esp), %ebx                  # 4-byte Reload
	adcl	152(%esp), %ebx
	movl	%ebx, 36(%esp)                  # 4-byte Spill
	movl	%eax, %ebx
	adcl	156(%esp), %ebp
	movl	%ebp, (%esp)                    # 4-byte Spill
	movl	20(%esp), %ebp                  # 4-byte Reload
	adcl	160(%esp), %ebp
	movl	%ebp, 20(%esp)                  # 4-byte Spill
	movl	28(%esp), %ebp                  # 4-byte Reload
	adcl	164(%esp), %ebp
	movl	%ebp, 28(%esp)                  # 4-byte Spill
	movl	24(%esp), %ebp                  # 4-byte Reload
	adcl	168(%esp), %ebp
	movl	%ebp, 24(%esp)                  # 4-byte Spill
	movl	32(%esp), %ebp                  # 4-byte Reload
	adcl	172(%esp), %ebp
	movl	%ebp, 32(%esp)                  # 4-byte Spill
	movl	4(%esp), %ebp                   # 4-byte Reload
	adcl	176(%esp), %ebp
	movl	%ebp, 4(%esp)                   # 4-byte Spill
	movl	12(%esp), %ebp                  # 4-byte Reload
	adcl	180(%esp), %ebp
	movl	%ebp, 12(%esp)                  # 4-byte Spill
	movl	48(%esp), %ebp                  # 4-byte Reload
	adcl	184(%esp), %ebp
	movl	%ebp, 48(%esp)                  # 4-byte Spill
	movl	8(%esp), %ebp                   # 4-byte Reload
	adcl	188(%esp), %ebp
	movl	%ebp, 8(%esp)                   # 4-byte Spill
	movl	1300(%esp), %ebp
	adcl	124(%ebp), %ebx
	movl	%ecx, 44(%esp)                  # 4-byte Spill
	subl	68(%esp), %ecx                  # 4-byte Folded Reload
	movl	%ecx, 68(%esp)                  # 4-byte Spill
	movl	16(%esp), %eax                  # 4-byte Reload
	sbbl	72(%esp), %eax                  # 4-byte Folded Reload
	movl	%eax, 72(%esp)                  # 4-byte Spill
	movl	%edx, %eax
	movl	%edx, 52(%esp)                  # 4-byte Spill
	sbbl	76(%esp), %eax                  # 4-byte Folded Reload
	movl	%eax, 76(%esp)                  # 4-byte Spill
	movl	%esi, %eax
	movl	%esi, 56(%esp)                  # 4-byte Spill
	sbbl	80(%esp), %eax                  # 4-byte Folded Reload
	movl	%eax, 80(%esp)                  # 4-byte Spill
	movl	%edi, 40(%esp)                  # 4-byte Spill
	sbbl	84(%esp), %edi                  # 4-byte Folded Reload
	movl	%edi, 84(%esp)                  # 4-byte Spill
	movl	36(%esp), %eax                  # 4-byte Reload
	sbbl	88(%esp), %eax                  # 4-byte Folded Reload
	movl	%eax, 88(%esp)                  # 4-byte Spill
	movl	(%esp), %eax                    # 4-byte Reload
	sbbl	100(%esp), %eax                 # 4-byte Folded Reload
	movl	%eax, 60(%esp)                  # 4-byte Spill
	movl	20(%esp), %eax                  # 4-byte Reload
	sbbl	64(%esp), %eax                  # 4-byte Folded Reload
	movl	%eax, 64(%esp)                  # 4-byte Spill
	movl	28(%esp), %eax                  # 4-byte Reload
	sbbl	92(%esp), %eax                  # 4-byte Folded Reload
	movl	%eax, 92(%esp)                  # 4-byte Spill
	movl	24(%esp), %eax                  # 4-byte Reload
	sbbl	96(%esp), %eax                  # 4-byte Folded Reload
	movl	%eax, 96(%esp)                  # 4-byte Spill
	movl	32(%esp), %eax                  # 4-byte Reload
	sbbl	104(%esp), %eax                 # 4-byte Folded Reload
	movl	4(%esp), %ebp                   # 4-byte Reload
	sbbl	108(%esp), %ebp                 # 4-byte Folded Reload
	movl	12(%esp), %edx                  # 4-byte Reload
	sbbl	112(%esp), %edx                 # 4-byte Folded Reload
	movl	48(%esp), %esi                  # 4-byte Reload
	sbbl	116(%esp), %esi                 # 4-byte Folded Reload
	movl	8(%esp), %edi                   # 4-byte Reload
	sbbl	120(%esp), %edi                 # 4-byte Folded Reload
	movl	%ebx, %ecx
	sbbl	124(%esp), %ecx                 # 4-byte Folded Reload
	testl	%ecx, %ecx
	js	.LBB80_1
# %bb.2:
	movl	1296(%esp), %ebx
	movl	%ecx, 60(%ebx)
	js	.LBB80_3
.LBB80_4:
	movl	%edi, 56(%ebx)
	movl	72(%esp), %ecx                  # 4-byte Reload
	js	.LBB80_5
.LBB80_6:
	movl	%esi, 52(%ebx)
	movl	84(%esp), %edi                  # 4-byte Reload
	js	.LBB80_7
.LBB80_8:
	movl	%edx, 48(%ebx)
	movl	80(%esp), %esi                  # 4-byte Reload
	js	.LBB80_9
.LBB80_10:
	movl	%ebp, 44(%ebx)
	js	.LBB80_11
.LBB80_12:
	movl	%eax, 40(%ebx)
	movl	88(%esp), %ebp                  # 4-byte Reload
	movl	96(%esp), %eax                  # 4-byte Reload
	js	.LBB80_13
.LBB80_14:
	movl	%eax, 36(%ebx)
	movl	68(%esp), %edx                  # 4-byte Reload
	movl	92(%esp), %eax                  # 4-byte Reload
	js	.LBB80_15
.LBB80_16:
	movl	%eax, 32(%ebx)
	js	.LBB80_17
.LBB80_18:
	movl	%ecx, %eax
	movl	64(%esp), %ecx                  # 4-byte Reload
	movl	%ecx, 28(%ebx)
	js	.LBB80_19
.LBB80_20:
	movl	60(%esp), %ecx                  # 4-byte Reload
	movl	%ecx, 24(%ebx)
	js	.LBB80_21
.LBB80_22:
	movl	%ebp, 20(%ebx)
	movl	%eax, %ecx
	js	.LBB80_23
.LBB80_24:
	movl	%edi, 16(%ebx)
	movl	76(%esp), %eax                  # 4-byte Reload
	js	.LBB80_25
.LBB80_26:
	movl	%esi, 12(%ebx)
	js	.LBB80_27
.LBB80_28:
	movl	%eax, 8(%ebx)
	js	.LBB80_29
.LBB80_30:
	movl	%ecx, 4(%ebx)
	jns	.LBB80_32
.LBB80_31:
	movl	44(%esp), %edx                  # 4-byte Reload
.LBB80_32:
	movl	%edx, (%ebx)
	addl	$1276, %esp                     # imm = 0x4FC
	popl	%esi
	popl	%edi
	popl	%ebx
	popl	%ebp
	retl
.LBB80_1:
	movl	%ebx, %ecx
	movl	1296(%esp), %ebx
	movl	%ecx, 60(%ebx)
	jns	.LBB80_4
.LBB80_3:
	movl	8(%esp), %edi                   # 4-byte Reload
	movl	%edi, 56(%ebx)
	movl	72(%esp), %ecx                  # 4-byte Reload
	jns	.LBB80_6
.LBB80_5:
	movl	48(%esp), %esi                  # 4-byte Reload
	movl	%esi, 52(%ebx)
	movl	84(%esp), %edi                  # 4-byte Reload
	jns	.LBB80_8
.LBB80_7:
	movl	12(%esp), %edx                  # 4-byte Reload
	movl	%edx, 48(%ebx)
	movl	80(%esp), %esi                  # 4-byte Reload
	jns	.LBB80_10
.LBB80_9:
	movl	4(%esp), %ebp                   # 4-byte Reload
	movl	%ebp, 44(%ebx)
	jns	.LBB80_12
.LBB80_11:
	movl	32(%esp), %eax                  # 4-byte Reload
	movl	%eax, 40(%ebx)
	movl	88(%esp), %ebp                  # 4-byte Reload
	movl	96(%esp), %eax                  # 4-byte Reload
	jns	.LBB80_14
.LBB80_13:
	movl	24(%esp), %eax                  # 4-byte Reload
	movl	%eax, 36(%ebx)
	movl	68(%esp), %edx                  # 4-byte Reload
	movl	92(%esp), %eax                  # 4-byte Reload
	jns	.LBB80_16
.LBB80_15:
	movl	28(%esp), %eax                  # 4-byte Reload
	movl	%eax, 32(%ebx)
	jns	.LBB80_18
.LBB80_17:
	movl	20(%esp), %eax                  # 4-byte Reload
	movl	%eax, 64(%esp)                  # 4-byte Spill
	movl	%ecx, %eax
	movl	64(%esp), %ecx                  # 4-byte Reload
	movl	%ecx, 28(%ebx)
	jns	.LBB80_20
.LBB80_19:
	movl	(%esp), %ecx                    # 4-byte Reload
	movl	%ecx, 60(%esp)                  # 4-byte Spill
	movl	60(%esp), %ecx                  # 4-byte Reload
	movl	%ecx, 24(%ebx)
	jns	.LBB80_22
.LBB80_21:
	movl	36(%esp), %ebp                  # 4-byte Reload
	movl	%ebp, 20(%ebx)
	movl	%eax, %ecx
	jns	.LBB80_24
.LBB80_23:
	movl	40(%esp), %edi                  # 4-byte Reload
	movl	%edi, 16(%ebx)
	movl	76(%esp), %eax                  # 4-byte Reload
	jns	.LBB80_26
.LBB80_25:
	movl	56(%esp), %esi                  # 4-byte Reload
	movl	%esi, 12(%ebx)
	jns	.LBB80_28
.LBB80_27:
	movl	52(%esp), %eax                  # 4-byte Reload
	movl	%eax, 8(%ebx)
	jns	.LBB80_30
.LBB80_29:
	movl	16(%esp), %ecx                  # 4-byte Reload
	movl	%ecx, 4(%ebx)
	js	.LBB80_31
	jmp	.LBB80_32
.Lfunc_end80:
	.size	mcl_fp_montRedNF16L, .Lfunc_end80-mcl_fp_montRedNF16L
                                        # -- End function
	.globl	mcl_fp_addPre16L                # -- Begin function mcl_fp_addPre16L
	.p2align	4, 0x90
	.type	mcl_fp_addPre16L,@function
mcl_fp_addPre16L:                       # @mcl_fp_addPre16L
# %bb.0:
	pushl	%ebp
	pushl	%ebx
	pushl	%edi
	pushl	%esi
	subl	$64, %esp
	movl	88(%esp), %edi
	movl	(%edi), %eax
	movl	4(%edi), %ecx
	movl	92(%esp), %esi
	addl	(%esi), %eax
	movl	%eax, 60(%esp)                  # 4-byte Spill
	adcl	4(%esi), %ecx
	movl	%ecx, 56(%esp)                  # 4-byte Spill
	movl	60(%edi), %eax
	movl	%eax, 52(%esp)                  # 4-byte Spill
	movl	56(%edi), %eax
	movl	%eax, 44(%esp)                  # 4-byte Spill
	movl	52(%edi), %eax
	movl	%eax, 36(%esp)                  # 4-byte Spill
	movl	48(%edi), %eax
	movl	%eax, 28(%esp)                  # 4-byte Spill
	movl	44(%edi), %eax
	movl	%eax, 20(%esp)                  # 4-byte Spill
	movl	40(%edi), %eax
	movl	%eax, 12(%esp)                  # 4-byte Spill
	movl	36(%edi), %eax
	movl	%eax, (%esp)                    # 4-byte Spill
	movl	32(%edi), %edx
	movl	28(%edi), %eax
	movl	%eax, 4(%esp)                   # 4-byte Spill
	movl	24(%edi), %ecx
	movl	20(%edi), %eax
	movl	16(%edi), %ebp
	movl	12(%edi), %ebx
	movl	8(%edi), %edi
	adcl	8(%esi), %edi
	movl	%edi, 48(%esp)                  # 4-byte Spill
	adcl	12(%esi), %ebx
	movl	%ebx, 40(%esp)                  # 4-byte Spill
	adcl	16(%esi), %ebp
	movl	%ebp, 32(%esp)                  # 4-byte Spill
	adcl	20(%esi), %eax
	movl	%eax, 24(%esp)                  # 4-byte Spill
	adcl	24(%esi), %ecx
	movl	%ecx, 16(%esp)                  # 4-byte Spill
	movl	4(%esp), %eax                   # 4-byte Reload
	adcl	28(%esi), %eax
	movl	%eax, 4(%esp)                   # 4-byte Spill
	adcl	32(%esi), %edx
	movl	%edx, 8(%esp)                   # 4-byte Spill
	movl	(%esp), %eax                    # 4-byte Reload
	adcl	36(%esi), %eax
	movl	%eax, (%esp)                    # 4-byte Spill
	movl	12(%esp), %ebp                  # 4-byte Reload
	adcl	40(%esi), %ebp
	movl	20(%esp), %edi                  # 4-byte Reload
	adcl	44(%esi), %edi
	movl	28(%esp), %edx                  # 4-byte Reload
	adcl	48(%esi), %edx
	movl	36(%esp), %ecx                  # 4-byte Reload
	adcl	52(%esi), %ecx
	movl	44(%esp), %eax                  # 4-byte Reload
	adcl	56(%esi), %eax
	movl	52(%esp), %ebx                  # 4-byte Reload
	adcl	60(%esi), %ebx
	movl	84(%esp), %esi
	movl	%eax, 56(%esi)
	movl	%ecx, 52(%esi)
	movl	%edx, 48(%esi)
	movl	%edi, 44(%esi)
	movl	%ebp, 40(%esi)
	movl	(%esp), %eax                    # 4-byte Reload
	movl	%eax, 36(%esi)
	movl	8(%esp), %eax                   # 4-byte Reload
	movl	%eax, 32(%esi)
	movl	4(%esp), %eax                   # 4-byte Reload
	movl	%eax, 28(%esi)
	movl	16(%esp), %eax                  # 4-byte Reload
	movl	%eax, 24(%esi)
	movl	24(%esp), %eax                  # 4-byte Reload
	movl	%eax, 20(%esi)
	movl	32(%esp), %eax                  # 4-byte Reload
	movl	%eax, 16(%esi)
	movl	40(%esp), %eax                  # 4-byte Reload
	movl	%eax, 12(%esi)
	movl	48(%esp), %eax                  # 4-byte Reload
	movl	%eax, 8(%esi)
	movl	%ebx, 60(%esi)
	movl	56(%esp), %eax                  # 4-byte Reload
	movl	%eax, 4(%esi)
	movl	60(%esp), %eax                  # 4-byte Reload
	movl	%eax, (%esi)
	setb	%al
	movzbl	%al, %eax
	addl	$64, %esp
	popl	%esi
	popl	%edi
	popl	%ebx
	popl	%ebp
	retl
.Lfunc_end81:
	.size	mcl_fp_addPre16L, .Lfunc_end81-mcl_fp_addPre16L
                                        # -- End function
	.globl	mcl_fp_subPre16L                # -- Begin function mcl_fp_subPre16L
	.p2align	4, 0x90
	.type	mcl_fp_subPre16L,@function
mcl_fp_subPre16L:                       # @mcl_fp_subPre16L
# %bb.0:
	pushl	%ebp
	pushl	%ebx
	pushl	%edi
	pushl	%esi
	subl	$64, %esp
	movl	88(%esp), %ebx
	movl	(%ebx), %ecx
	movl	4(%ebx), %eax
	movl	92(%esp), %edi
	subl	(%edi), %ecx
	movl	%ecx, 60(%esp)                  # 4-byte Spill
	sbbl	4(%edi), %eax
	movl	%eax, 56(%esp)                  # 4-byte Spill
	movl	60(%ebx), %eax
	movl	%eax, 52(%esp)                  # 4-byte Spill
	movl	56(%ebx), %eax
	movl	%eax, 44(%esp)                  # 4-byte Spill
	movl	52(%ebx), %eax
	movl	%eax, 36(%esp)                  # 4-byte Spill
	movl	48(%ebx), %eax
	movl	%eax, 28(%esp)                  # 4-byte Spill
	movl	44(%ebx), %eax
	movl	%eax, 20(%esp)                  # 4-byte Spill
	movl	40(%ebx), %eax
	movl	%eax, 12(%esp)                  # 4-byte Spill
	movl	36(%ebx), %eax
	movl	%eax, (%esp)                    # 4-byte Spill
	movl	32(%ebx), %eax
	movl	%eax, 4(%esp)                   # 4-byte Spill
	movl	28(%ebx), %ebp
	movl	24(%ebx), %esi
	movl	20(%ebx), %edx
	movl	16(%ebx), %ecx
	movl	12(%ebx), %eax
	movl	8(%ebx), %ebx
	sbbl	8(%edi), %ebx
	movl	%ebx, 48(%esp)                  # 4-byte Spill
	sbbl	12(%edi), %eax
	movl	%eax, 40(%esp)                  # 4-byte Spill
	sbbl	16(%edi), %ecx
	movl	%ecx, 32(%esp)                  # 4-byte Spill
	sbbl	20(%edi), %edx
	movl	%edx, 24(%esp)                  # 4-byte Spill
	sbbl	24(%edi), %esi
	movl	%esi, 16(%esp)                  # 4-byte Spill
	sbbl	28(%edi), %ebp
	movl	%ebp, 8(%esp)                   # 4-byte Spill
	movl	4(%esp), %eax                   # 4-byte Reload
	sbbl	32(%edi), %eax
	movl	%eax, 4(%esp)                   # 4-byte Spill
	movl	(%esp), %eax                    # 4-byte Reload
	sbbl	36(%edi), %eax
	movl	%eax, (%esp)                    # 4-byte Spill
	movl	12(%esp), %ebp                  # 4-byte Reload
	sbbl	40(%edi), %ebp
	movl	20(%esp), %esi                  # 4-byte Reload
	sbbl	44(%edi), %esi
	movl	28(%esp), %edx                  # 4-byte Reload
	sbbl	48(%edi), %edx
	movl	36(%esp), %ecx                  # 4-byte Reload
	sbbl	52(%edi), %ecx
	movl	44(%esp), %eax                  # 4-byte Reload
	sbbl	56(%edi), %eax
	movl	52(%esp), %ebx                  # 4-byte Reload
	sbbl	60(%edi), %ebx
	movl	84(%esp), %edi
	movl	%eax, 56(%edi)
	movl	%ecx, 52(%edi)
	movl	%edx, 48(%edi)
	movl	%esi, 44(%edi)
	movl	%ebp, 40(%edi)
	movl	(%esp), %eax                    # 4-byte Reload
	movl	%eax, 36(%edi)
	movl	4(%esp), %eax                   # 4-byte Reload
	movl	%eax, 32(%edi)
	movl	8(%esp), %eax                   # 4-byte Reload
	movl	%eax, 28(%edi)
	movl	16(%esp), %eax                  # 4-byte Reload
	movl	%eax, 24(%edi)
	movl	24(%esp), %eax                  # 4-byte Reload
	movl	%eax, 20(%edi)
	movl	32(%esp), %eax                  # 4-byte Reload
	movl	%eax, 16(%edi)
	movl	40(%esp), %eax                  # 4-byte Reload
	movl	%eax, 12(%edi)
	movl	48(%esp), %eax                  # 4-byte Reload
	movl	%eax, 8(%edi)
	movl	56(%esp), %ecx                  # 4-byte Reload
	movl	%ecx, 4(%edi)
	movl	%ebx, 60(%edi)
	movl	60(%esp), %ecx                  # 4-byte Reload
	movl	%ecx, (%edi)
	movl	$0, %eax
	sbbl	%eax, %eax
	andl	$1, %eax
	addl	$64, %esp
	popl	%esi
	popl	%edi
	popl	%ebx
	popl	%ebp
	retl
.Lfunc_end82:
	.size	mcl_fp_subPre16L, .Lfunc_end82-mcl_fp_subPre16L
                                        # -- End function
	.globl	mcl_fp_shr1_16L                 # -- Begin function mcl_fp_shr1_16L
	.p2align	4, 0x90
	.type	mcl_fp_shr1_16L,@function
mcl_fp_shr1_16L:                        # @mcl_fp_shr1_16L
# %bb.0:
	pushl	%esi
	movl	12(%esp), %eax
	movl	60(%eax), %edx
	movl	%edx, %esi
	shrl	%esi
	movl	8(%esp), %ecx
	movl	%esi, 60(%ecx)
	movl	56(%eax), %esi
	shldl	$31, %esi, %edx
	movl	%edx, 56(%ecx)
	movl	52(%eax), %edx
	shldl	$31, %edx, %esi
	movl	%esi, 52(%ecx)
	movl	48(%eax), %esi
	shldl	$31, %esi, %edx
	movl	%edx, 48(%ecx)
	movl	44(%eax), %edx
	shldl	$31, %edx, %esi
	movl	%esi, 44(%ecx)
	movl	40(%eax), %esi
	shldl	$31, %esi, %edx
	movl	%edx, 40(%ecx)
	movl	36(%eax), %edx
	shldl	$31, %edx, %esi
	movl	%esi, 36(%ecx)
	movl	32(%eax), %esi
	shldl	$31, %esi, %edx
	movl	%edx, 32(%ecx)
	movl	28(%eax), %edx
	shldl	$31, %edx, %esi
	movl	%esi, 28(%ecx)
	movl	24(%eax), %esi
	shldl	$31, %esi, %edx
	movl	%edx, 24(%ecx)
	movl	20(%eax), %edx
	shldl	$31, %edx, %esi
	movl	%esi, 20(%ecx)
	movl	16(%eax), %esi
	shldl	$31, %esi, %edx
	movl	%edx, 16(%ecx)
	movl	12(%eax), %edx
	shldl	$31, %edx, %esi
	movl	%esi, 12(%ecx)
	movl	8(%eax), %esi
	shldl	$31, %esi, %edx
	movl	%edx, 8(%ecx)
	movl	4(%eax), %edx
	shldl	$31, %edx, %esi
	movl	%esi, 4(%ecx)
	movl	(%eax), %eax
	shrdl	$1, %edx, %eax
	movl	%eax, (%ecx)
	popl	%esi
	retl
.Lfunc_end83:
	.size	mcl_fp_shr1_16L, .Lfunc_end83-mcl_fp_shr1_16L
                                        # -- End function
	.globl	mcl_fp_add16L                   # -- Begin function mcl_fp_add16L
	.p2align	4, 0x90
	.type	mcl_fp_add16L,@function
mcl_fp_add16L:                          # @mcl_fp_add16L
# %bb.0:
	pushl	%ebp
	pushl	%ebx
	pushl	%edi
	pushl	%esi
	subl	$64, %esp
	movl	88(%esp), %ecx
	movl	(%ecx), %esi
	movl	4(%ecx), %eax
	movl	92(%esp), %edx
	addl	(%edx), %esi
	movl	%esi, 48(%esp)                  # 4-byte Spill
	adcl	4(%edx), %eax
	movl	%eax, 44(%esp)                  # 4-byte Spill
	movl	60(%ecx), %eax
	movl	%eax, 60(%esp)                  # 4-byte Spill
	movl	56(%ecx), %eax
	movl	%eax, 56(%esp)                  # 4-byte Spill
	movl	52(%ecx), %eax
	movl	%eax, 52(%esp)                  # 4-byte Spill
	movl	48(%ecx), %eax
	movl	%eax, 28(%esp)                  # 4-byte Spill
	movl	44(%ecx), %eax
	movl	%eax, 12(%esp)                  # 4-byte Spill
	movl	40(%ecx), %eax
	movl	%eax, 8(%esp)                   # 4-byte Spill
	movl	36(%ecx), %eax
	movl	%eax, 16(%esp)                  # 4-byte Spill
	movl	32(%ecx), %eax
	movl	%eax, 4(%esp)                   # 4-byte Spill
	movl	28(%ecx), %edi
	movl	24(%ecx), %ebx
	movl	20(%ecx), %esi
	movl	16(%ecx), %eax
	movl	12(%ecx), %ebp
	movl	8(%ecx), %ecx
	adcl	8(%edx), %ecx
	movl	%ecx, 32(%esp)                  # 4-byte Spill
	adcl	12(%edx), %ebp
	adcl	16(%edx), %eax
	movl	%eax, 36(%esp)                  # 4-byte Spill
	adcl	20(%edx), %esi
	movl	%esi, 40(%esp)                  # 4-byte Spill
	adcl	24(%edx), %ebx
	movl	%ebx, 20(%esp)                  # 4-byte Spill
	adcl	28(%edx), %edi
	movl	%edi, 24(%esp)                  # 4-byte Spill
	movl	4(%esp), %eax                   # 4-byte Reload
	adcl	32(%edx), %eax
	movl	%eax, 4(%esp)                   # 4-byte Spill
	movl	16(%esp), %edi                  # 4-byte Reload
	adcl	36(%edx), %edi
	movl	%edi, 16(%esp)                  # 4-byte Spill
	movl	8(%esp), %eax                   # 4-byte Reload
	adcl	40(%edx), %eax
	movl	%eax, 8(%esp)                   # 4-byte Spill
	movl	12(%esp), %eax                  # 4-byte Reload
	adcl	44(%edx), %eax
	movl	%eax, 12(%esp)                  # 4-byte Spill
	movl	28(%esp), %eax                  # 4-byte Reload
	adcl	48(%edx), %eax
	movl	%eax, 28(%esp)                  # 4-byte Spill
	movl	52(%esp), %eax                  # 4-byte Reload
	adcl	52(%edx), %eax
	movl	%eax, 52(%esp)                  # 4-byte Spill
	movl	56(%esp), %ecx                  # 4-byte Reload
	adcl	56(%edx), %ecx
	movl	%ecx, 56(%esp)                  # 4-byte Spill
	movl	60(%esp), %ebx                  # 4-byte Reload
	adcl	60(%edx), %ebx
	movl	%ebx, 60(%esp)                  # 4-byte Spill
	movl	84(%esp), %esi
	movl	%ebx, 60(%esi)
	movl	%ecx, 56(%esi)
	movl	32(%esp), %ebx                  # 4-byte Reload
	movl	%eax, 52(%esi)
	movl	28(%esp), %eax                  # 4-byte Reload
	movl	%eax, 48(%esi)
	movl	12(%esp), %eax                  # 4-byte Reload
	movl	%eax, 44(%esi)
	movl	8(%esp), %eax                   # 4-byte Reload
	movl	%eax, 40(%esi)
	movl	%edi, 36(%esi)
	movl	4(%esp), %eax                   # 4-byte Reload
	movl	%eax, 32(%esi)
	movl	24(%esp), %eax                  # 4-byte Reload
	movl	%eax, 28(%esi)
	movl	20(%esp), %eax                  # 4-byte Reload
	movl	%eax, 24(%esi)
	movl	40(%esp), %edi                  # 4-byte Reload
	movl	%edi, 20(%esi)
	movl	36(%esp), %eax                  # 4-byte Reload
	movl	%eax, 16(%esi)
	movl	%ebp, 12(%esi)
	movl	%ebx, 8(%esi)
	movl	44(%esp), %ecx                  # 4-byte Reload
	movl	%ecx, 4(%esi)
	movl	48(%esp), %edx                  # 4-byte Reload
	movl	%edx, (%esi)
	setb	3(%esp)                         # 1-byte Folded Spill
	movl	96(%esp), %esi
	subl	(%esi), %edx
	movl	%edx, 48(%esp)                  # 4-byte Spill
	sbbl	4(%esi), %ecx
	movl	%ecx, 44(%esp)                  # 4-byte Spill
	sbbl	8(%esi), %ebx
	movl	%ebx, %ecx
	sbbl	12(%esi), %ebp
	movl	%ebp, 32(%esp)                  # 4-byte Spill
	sbbl	16(%esi), %eax
	movl	%eax, 36(%esp)                  # 4-byte Spill
	sbbl	20(%esi), %edi
	movl	%edi, 40(%esp)                  # 4-byte Spill
	movl	20(%esp), %eax                  # 4-byte Reload
	sbbl	24(%esi), %eax
	movl	%eax, 20(%esp)                  # 4-byte Spill
	movl	24(%esp), %eax                  # 4-byte Reload
	sbbl	28(%esi), %eax
	movl	%eax, 24(%esp)                  # 4-byte Spill
	movl	4(%esp), %eax                   # 4-byte Reload
	sbbl	32(%esi), %eax
	movl	%eax, 4(%esp)                   # 4-byte Spill
	movl	16(%esp), %eax                  # 4-byte Reload
	sbbl	36(%esi), %eax
	movl	%eax, 16(%esp)                  # 4-byte Spill
	movl	8(%esp), %eax                   # 4-byte Reload
	sbbl	40(%esi), %eax
	movl	%eax, 8(%esp)                   # 4-byte Spill
	movl	12(%esp), %eax                  # 4-byte Reload
	sbbl	44(%esi), %eax
	movl	%eax, 12(%esp)                  # 4-byte Spill
	movl	28(%esp), %eax                  # 4-byte Reload
	sbbl	48(%esi), %eax
	movl	52(%esp), %ebp                  # 4-byte Reload
	sbbl	52(%esi), %ebp
	movl	56(%esp), %edi                  # 4-byte Reload
	sbbl	56(%esi), %edi
	movl	60(%esp), %ebx                  # 4-byte Reload
	sbbl	60(%esi), %ebx
	movl	%ebx, %esi
	movzbl	3(%esp), %ebx                   # 1-byte Folded Reload
	sbbl	$0, %ebx
	testb	$1, %bl
	jne	.LBB84_2
# %bb.1:                                # %nocarry
	movl	48(%esp), %ebx                  # 4-byte Reload
	movl	84(%esp), %edx
	movl	%ebx, (%edx)
	movl	44(%esp), %ebx                  # 4-byte Reload
	movl	%ebx, 4(%edx)
	movl	%ecx, 8(%edx)
	movl	32(%esp), %ecx                  # 4-byte Reload
	movl	%ecx, 12(%edx)
	movl	36(%esp), %ecx                  # 4-byte Reload
	movl	%ecx, 16(%edx)
	movl	40(%esp), %ecx                  # 4-byte Reload
	movl	%ecx, 20(%edx)
	movl	20(%esp), %ecx                  # 4-byte Reload
	movl	%ecx, 24(%edx)
	movl	24(%esp), %ecx                  # 4-byte Reload
	movl	%ecx, 28(%edx)
	movl	4(%esp), %ecx                   # 4-byte Reload
	movl	%ecx, 32(%edx)
	movl	16(%esp), %ecx                  # 4-byte Reload
	movl	%ecx, 36(%edx)
	movl	8(%esp), %ecx                   # 4-byte Reload
	movl	%ecx, 40(%edx)
	movl	12(%esp), %ecx                  # 4-byte Reload
	movl	%ecx, 44(%edx)
	movl	%eax, 48(%edx)
	movl	%ebp, 52(%edx)
	movl	%edi, 56(%edx)
	movl	%esi, 60(%edx)
.LBB84_2:                               # %carry
	addl	$64, %esp
	popl	%esi
	popl	%edi
	popl	%ebx
	popl	%ebp
	retl
.Lfunc_end84:
	.size	mcl_fp_add16L, .Lfunc_end84-mcl_fp_add16L
                                        # -- End function
	.globl	mcl_fp_addNF16L                 # -- Begin function mcl_fp_addNF16L
	.p2align	4, 0x90
	.type	mcl_fp_addNF16L,@function
mcl_fp_addNF16L:                        # @mcl_fp_addNF16L
# %bb.0:
	pushl	%ebp
	pushl	%ebx
	pushl	%edi
	pushl	%esi
	subl	$104, %esp
	movl	132(%esp), %eax
	movl	(%eax), %esi
	movl	4(%eax), %edx
	movl	128(%esp), %ecx
	addl	(%ecx), %esi
	movl	%esi, 56(%esp)                  # 4-byte Spill
	adcl	4(%ecx), %edx
	movl	%edx, 52(%esp)                  # 4-byte Spill
	movl	60(%eax), %esi
	movl	%esi, (%esp)                    # 4-byte Spill
	movl	56(%eax), %esi
	movl	%esi, 4(%esp)                   # 4-byte Spill
	movl	52(%eax), %esi
	movl	%esi, 8(%esp)                   # 4-byte Spill
	movl	48(%eax), %esi
	movl	%esi, 12(%esp)                  # 4-byte Spill
	movl	44(%eax), %edx
	movl	%edx, 16(%esp)                  # 4-byte Spill
	movl	40(%eax), %edx
	movl	%edx, 20(%esp)                  # 4-byte Spill
	movl	36(%eax), %edx
	movl	32(%eax), %esi
	movl	%esi, 24(%esp)                  # 4-byte Spill
	movl	28(%eax), %esi
	movl	24(%eax), %edi
	movl	%edi, 28(%esp)                  # 4-byte Spill
	movl	20(%eax), %ebx
	movl	16(%eax), %ebp
	movl	12(%eax), %edi
	movl	8(%eax), %eax
	adcl	8(%ecx), %eax
	adcl	12(%ecx), %edi
	adcl	16(%ecx), %ebp
	adcl	20(%ecx), %ebx
	movl	%ebx, 48(%esp)                  # 4-byte Spill
	movl	28(%esp), %ebx                  # 4-byte Reload
	adcl	24(%ecx), %ebx
	movl	%ebx, 28(%esp)                  # 4-byte Spill
	adcl	28(%ecx), %esi
	movl	%esi, 44(%esp)                  # 4-byte Spill
	movl	24(%esp), %esi                  # 4-byte Reload
	adcl	32(%ecx), %esi
	movl	%esi, 24(%esp)                  # 4-byte Spill
	adcl	36(%ecx), %edx
	movl	%edx, 40(%esp)                  # 4-byte Spill
	movl	20(%esp), %edx                  # 4-byte Reload
	adcl	40(%ecx), %edx
	movl	%edx, 20(%esp)                  # 4-byte Spill
	movl	16(%esp), %edx                  # 4-byte Reload
	adcl	44(%ecx), %edx
	movl	%edx, 16(%esp)                  # 4-byte Spill
	movl	12(%esp), %ebx                  # 4-byte Reload
	adcl	48(%ecx), %ebx
	movl	%ebx, 12(%esp)                  # 4-byte Spill
	movl	8(%esp), %ebx                   # 4-byte Reload
	adcl	52(%ecx), %ebx
	movl	%ebx, 8(%esp)                   # 4-byte Spill
	movl	4(%esp), %ebx                   # 4-byte Reload
	adcl	56(%ecx), %ebx
	movl	%ebx, 4(%esp)                   # 4-byte Spill
	movl	(%esp), %ebx                    # 4-byte Reload
	adcl	60(%ecx), %ebx
	movl	%ebx, (%esp)                    # 4-byte Spill
	movl	136(%esp), %ebx
	movl	56(%esp), %edx                  # 4-byte Reload
	subl	(%ebx), %edx
	movl	%edx, 100(%esp)                 # 4-byte Spill
	movl	52(%esp), %ecx                  # 4-byte Reload
	sbbl	4(%ebx), %ecx
	movl	%ecx, 96(%esp)                  # 4-byte Spill
	movl	%eax, 68(%esp)                  # 4-byte Spill
	sbbl	8(%ebx), %eax
	movl	%eax, 92(%esp)                  # 4-byte Spill
	movl	%edi, 64(%esp)                  # 4-byte Spill
	sbbl	12(%ebx), %edi
	movl	%edi, 88(%esp)                  # 4-byte Spill
	movl	%ebp, 60(%esp)                  # 4-byte Spill
	sbbl	16(%ebx), %ebp
	movl	%ebp, 84(%esp)                  # 4-byte Spill
	movl	48(%esp), %esi                  # 4-byte Reload
	sbbl	20(%ebx), %esi
	movl	%esi, 80(%esp)                  # 4-byte Spill
	movl	28(%esp), %eax                  # 4-byte Reload
	sbbl	24(%ebx), %eax
	movl	%eax, 36(%esp)                  # 4-byte Spill
	movl	44(%esp), %eax                  # 4-byte Reload
	sbbl	28(%ebx), %eax
	movl	%eax, 32(%esp)                  # 4-byte Spill
	movl	24(%esp), %eax                  # 4-byte Reload
	sbbl	32(%ebx), %eax
	movl	%eax, 76(%esp)                  # 4-byte Spill
	movl	40(%esp), %eax                  # 4-byte Reload
	sbbl	36(%ebx), %eax
	movl	%eax, 72(%esp)                  # 4-byte Spill
	movl	20(%esp), %ebp                  # 4-byte Reload
	sbbl	40(%ebx), %ebp
	movl	16(%esp), %ecx                  # 4-byte Reload
	sbbl	44(%ebx), %ecx
	movl	12(%esp), %eax                  # 4-byte Reload
	sbbl	48(%ebx), %eax
	movl	8(%esp), %esi                   # 4-byte Reload
	sbbl	52(%ebx), %esi
	movl	4(%esp), %edi                   # 4-byte Reload
	sbbl	56(%ebx), %edi
	movl	(%esp), %edx                    # 4-byte Reload
	sbbl	60(%ebx), %edx
	testl	%edx, %edx
	js	.LBB85_1
# %bb.2:
	movl	124(%esp), %ebx
	movl	%edx, 60(%ebx)
	js	.LBB85_3
.LBB85_4:
	movl	%edi, 56(%ebx)
	movl	92(%esp), %edx                  # 4-byte Reload
	js	.LBB85_5
.LBB85_6:
	movl	%esi, 52(%ebx)
	movl	84(%esp), %edi                  # 4-byte Reload
	js	.LBB85_7
.LBB85_8:
	movl	%eax, 48(%ebx)
	js	.LBB85_9
.LBB85_10:
	movl	%ecx, 44(%ebx)
	movl	100(%esp), %ecx                 # 4-byte Reload
	movl	76(%esp), %eax                  # 4-byte Reload
	js	.LBB85_11
.LBB85_12:
	movl	%ebp, 40(%ebx)
	movl	96(%esp), %esi                  # 4-byte Reload
	movl	72(%esp), %ebp                  # 4-byte Reload
	js	.LBB85_13
.LBB85_14:
	movl	%ebp, 36(%ebx)
	movl	80(%esp), %ebp                  # 4-byte Reload
	js	.LBB85_15
.LBB85_16:
	movl	%eax, 32(%ebx)
	js	.LBB85_17
.LBB85_18:
	movl	%edx, %eax
	movl	32(%esp), %edx                  # 4-byte Reload
	movl	%edx, 28(%ebx)
	js	.LBB85_19
.LBB85_20:
	movl	36(%esp), %edx                  # 4-byte Reload
	movl	%edx, 24(%ebx)
	js	.LBB85_21
.LBB85_22:
	movl	%ebp, 20(%ebx)
	movl	%eax, %edx
	js	.LBB85_23
.LBB85_24:
	movl	%edi, 16(%ebx)
	movl	88(%esp), %eax                  # 4-byte Reload
	js	.LBB85_25
.LBB85_26:
	movl	%eax, 12(%ebx)
	js	.LBB85_27
.LBB85_28:
	movl	%edx, 8(%ebx)
	js	.LBB85_29
.LBB85_30:
	movl	%esi, 4(%ebx)
	jns	.LBB85_32
.LBB85_31:
	movl	56(%esp), %ecx                  # 4-byte Reload
.LBB85_32:
	movl	%ecx, (%ebx)
	addl	$104, %esp
	popl	%esi
	popl	%edi
	popl	%ebx
	popl	%ebp
	retl
.LBB85_1:
	movl	(%esp), %edx                    # 4-byte Reload
	movl	124(%esp), %ebx
	movl	%edx, 60(%ebx)
	jns	.LBB85_4
.LBB85_3:
	movl	4(%esp), %edi                   # 4-byte Reload
	movl	%edi, 56(%ebx)
	movl	92(%esp), %edx                  # 4-byte Reload
	jns	.LBB85_6
.LBB85_5:
	movl	8(%esp), %esi                   # 4-byte Reload
	movl	%esi, 52(%ebx)
	movl	84(%esp), %edi                  # 4-byte Reload
	jns	.LBB85_8
.LBB85_7:
	movl	12(%esp), %eax                  # 4-byte Reload
	movl	%eax, 48(%ebx)
	jns	.LBB85_10
.LBB85_9:
	movl	16(%esp), %ecx                  # 4-byte Reload
	movl	%ecx, 44(%ebx)
	movl	100(%esp), %ecx                 # 4-byte Reload
	movl	76(%esp), %eax                  # 4-byte Reload
	jns	.LBB85_12
.LBB85_11:
	movl	20(%esp), %ebp                  # 4-byte Reload
	movl	%ebp, 40(%ebx)
	movl	96(%esp), %esi                  # 4-byte Reload
	movl	72(%esp), %ebp                  # 4-byte Reload
	jns	.LBB85_14
.LBB85_13:
	movl	40(%esp), %ebp                  # 4-byte Reload
	movl	%ebp, 36(%ebx)
	movl	80(%esp), %ebp                  # 4-byte Reload
	jns	.LBB85_16
.LBB85_15:
	movl	24(%esp), %eax                  # 4-byte Reload
	movl	%eax, 32(%ebx)
	jns	.LBB85_18
.LBB85_17:
	movl	44(%esp), %eax                  # 4-byte Reload
	movl	%eax, 32(%esp)                  # 4-byte Spill
	movl	%edx, %eax
	movl	32(%esp), %edx                  # 4-byte Reload
	movl	%edx, 28(%ebx)
	jns	.LBB85_20
.LBB85_19:
	movl	28(%esp), %edx                  # 4-byte Reload
	movl	%edx, 36(%esp)                  # 4-byte Spill
	movl	36(%esp), %edx                  # 4-byte Reload
	movl	%edx, 24(%ebx)
	jns	.LBB85_22
.LBB85_21:
	movl	48(%esp), %ebp                  # 4-byte Reload
	movl	%ebp, 20(%ebx)
	movl	%eax, %edx
	jns	.LBB85_24
.LBB85_23:
	movl	60(%esp), %edi                  # 4-byte Reload
	movl	%edi, 16(%ebx)
	movl	88(%esp), %eax                  # 4-byte Reload
	jns	.LBB85_26
.LBB85_25:
	movl	64(%esp), %eax                  # 4-byte Reload
	movl	%eax, 12(%ebx)
	jns	.LBB85_28
.LBB85_27:
	movl	68(%esp), %edx                  # 4-byte Reload
	movl	%edx, 8(%ebx)
	jns	.LBB85_30
.LBB85_29:
	movl	52(%esp), %esi                  # 4-byte Reload
	movl	%esi, 4(%ebx)
	js	.LBB85_31
	jmp	.LBB85_32
.Lfunc_end85:
	.size	mcl_fp_addNF16L, .Lfunc_end85-mcl_fp_addNF16L
                                        # -- End function
	.globl	mcl_fp_sub16L                   # -- Begin function mcl_fp_sub16L
	.p2align	4, 0x90
	.type	mcl_fp_sub16L,@function
mcl_fp_sub16L:                          # @mcl_fp_sub16L
# %bb.0:
	pushl	%ebp
	pushl	%ebx
	pushl	%edi
	pushl	%esi
	subl	$64, %esp
	movl	88(%esp), %edx
	movl	(%edx), %ecx
	movl	4(%edx), %esi
	movl	92(%esp), %edi
	subl	(%edi), %ecx
	movl	%ecx, 56(%esp)                  # 4-byte Spill
	sbbl	4(%edi), %esi
	movl	%esi, 52(%esp)                  # 4-byte Spill
	movl	60(%edx), %eax
	movl	%eax, 60(%esp)                  # 4-byte Spill
	movl	56(%edx), %ecx
	movl	%ecx, 20(%esp)                  # 4-byte Spill
	movl	52(%edx), %ecx
	movl	%ecx, 24(%esp)                  # 4-byte Spill
	movl	48(%edx), %ecx
	movl	%ecx, 28(%esp)                  # 4-byte Spill
	movl	44(%edx), %ecx
	movl	%ecx, 32(%esp)                  # 4-byte Spill
	movl	40(%edx), %ecx
	movl	%ecx, (%esp)                    # 4-byte Spill
	movl	36(%edx), %ebp
	movl	32(%edx), %eax
	movl	%eax, 4(%esp)                   # 4-byte Spill
	movl	28(%edx), %eax
	movl	%eax, 8(%esp)                   # 4-byte Spill
	movl	24(%edx), %ebx
	movl	20(%edx), %esi
	movl	16(%edx), %ecx
	movl	12(%edx), %eax
	movl	8(%edx), %edx
	sbbl	8(%edi), %edx
	movl	%edx, 48(%esp)                  # 4-byte Spill
	sbbl	12(%edi), %eax
	movl	%eax, 44(%esp)                  # 4-byte Spill
	sbbl	16(%edi), %ecx
	movl	%ecx, 16(%esp)                  # 4-byte Spill
	sbbl	20(%edi), %esi
	movl	%esi, 12(%esp)                  # 4-byte Spill
	sbbl	24(%edi), %ebx
	movl	8(%esp), %esi                   # 4-byte Reload
	sbbl	28(%edi), %esi
	movl	4(%esp), %ecx                   # 4-byte Reload
	sbbl	32(%edi), %ecx
	sbbl	36(%edi), %ebp
	movl	(%esp), %edx                    # 4-byte Reload
	sbbl	40(%edi), %edx
	movl	%edx, (%esp)                    # 4-byte Spill
	movl	32(%esp), %edx                  # 4-byte Reload
	sbbl	44(%edi), %edx
	movl	%edx, 32(%esp)                  # 4-byte Spill
	movl	28(%esp), %edx                  # 4-byte Reload
	sbbl	48(%edi), %edx
	movl	%edx, 28(%esp)                  # 4-byte Spill
	movl	24(%esp), %edx                  # 4-byte Reload
	sbbl	52(%edi), %edx
	movl	%edx, 24(%esp)                  # 4-byte Spill
	movl	20(%esp), %edx                  # 4-byte Reload
	sbbl	56(%edi), %edx
	movl	%edx, 20(%esp)                  # 4-byte Spill
	movl	60(%esp), %edx                  # 4-byte Reload
	sbbl	60(%edi), %edx
	movl	$0, %eax
	sbbl	%eax, %eax
	testb	$1, %al
	movl	84(%esp), %eax
	movl	%edx, 60(%esp)                  # 4-byte Spill
	movl	%edx, 60(%eax)
	movl	20(%esp), %edx                  # 4-byte Reload
	movl	%edx, 56(%eax)
	movl	24(%esp), %edx                  # 4-byte Reload
	movl	%edx, 52(%eax)
	movl	28(%esp), %edx                  # 4-byte Reload
	movl	%edx, 48(%eax)
	movl	32(%esp), %edx                  # 4-byte Reload
	movl	%edx, 44(%eax)
	movl	(%esp), %edx                    # 4-byte Reload
	movl	%edx, 40(%eax)
	movl	%ebp, 36(%esp)                  # 4-byte Spill
	movl	%ebp, 36(%eax)
	movl	%ecx, 4(%esp)                   # 4-byte Spill
	movl	%ecx, 32(%eax)
	movl	%esi, %ecx
	movl	%esi, 8(%esp)                   # 4-byte Spill
	movl	%esi, 28(%eax)
	movl	%ebx, 40(%esp)                  # 4-byte Spill
	movl	%ebx, 24(%eax)
	movl	12(%esp), %ecx                  # 4-byte Reload
	movl	%ecx, 20(%eax)
	movl	16(%esp), %ecx                  # 4-byte Reload
	movl	%ecx, 16(%eax)
	movl	44(%esp), %ecx                  # 4-byte Reload
	movl	%ecx, %edx
	movl	%ecx, 12(%eax)
	movl	48(%esp), %ecx                  # 4-byte Reload
	movl	%ecx, %ebx
	movl	%ecx, 8(%eax)
	movl	52(%esp), %ecx                  # 4-byte Reload
	movl	%ecx, 4(%eax)
	movl	56(%esp), %edi                  # 4-byte Reload
	movl	%edi, (%eax)
	je	.LBB86_2
# %bb.1:                                # %carry
	movl	%ecx, %esi
	movl	%edi, %ecx
	movl	96(%esp), %ecx
	addl	(%ecx), %edi
	movl	%edi, 56(%esp)                  # 4-byte Spill
	adcl	4(%ecx), %esi
	movl	%esi, 52(%esp)                  # 4-byte Spill
	adcl	8(%ecx), %ebx
	movl	%ebx, 48(%esp)                  # 4-byte Spill
	adcl	12(%ecx), %edx
	movl	%edx, 44(%esp)                  # 4-byte Spill
	movl	16(%esp), %edx                  # 4-byte Reload
	adcl	16(%ecx), %edx
	movl	%edx, 16(%esp)                  # 4-byte Spill
	movl	12(%esp), %edx                  # 4-byte Reload
	adcl	20(%ecx), %edx
	movl	%edx, 12(%esp)                  # 4-byte Spill
	movl	40(%esp), %edx                  # 4-byte Reload
	adcl	24(%ecx), %edx
	movl	%edx, 40(%esp)                  # 4-byte Spill
	movl	8(%esp), %edx                   # 4-byte Reload
	adcl	28(%ecx), %edx
	movl	%edx, 8(%esp)                   # 4-byte Spill
	movl	4(%esp), %edx                   # 4-byte Reload
	adcl	32(%ecx), %edx
	movl	%edx, 4(%esp)                   # 4-byte Spill
	movl	36(%esp), %edx                  # 4-byte Reload
	adcl	36(%ecx), %edx
	movl	%edx, 36(%esp)                  # 4-byte Spill
	movl	(%esp), %edx                    # 4-byte Reload
	adcl	40(%ecx), %edx
	movl	%edx, (%esp)                    # 4-byte Spill
	movl	32(%esp), %ebx                  # 4-byte Reload
	adcl	44(%ecx), %ebx
	movl	28(%esp), %edi                  # 4-byte Reload
	adcl	48(%ecx), %edi
	movl	24(%esp), %esi                  # 4-byte Reload
	adcl	52(%ecx), %esi
	movl	20(%esp), %edx                  # 4-byte Reload
	adcl	56(%ecx), %edx
	movl	60(%esp), %ebp                  # 4-byte Reload
	adcl	60(%ecx), %ebp
	movl	%ebp, 60(%eax)
	movl	%edx, 56(%eax)
	movl	%esi, 52(%eax)
	movl	%edi, 48(%eax)
	movl	%ebx, 44(%eax)
	movl	(%esp), %ecx                    # 4-byte Reload
	movl	%ecx, 40(%eax)
	movl	36(%esp), %ecx                  # 4-byte Reload
	movl	%ecx, 36(%eax)
	movl	4(%esp), %ecx                   # 4-byte Reload
	movl	%ecx, 32(%eax)
	movl	8(%esp), %ecx                   # 4-byte Reload
	movl	%ecx, 28(%eax)
	movl	40(%esp), %ecx                  # 4-byte Reload
	movl	%ecx, 24(%eax)
	movl	12(%esp), %ecx                  # 4-byte Reload
	movl	%ecx, 20(%eax)
	movl	16(%esp), %ecx                  # 4-byte Reload
	movl	%ecx, 16(%eax)
	movl	44(%esp), %ecx                  # 4-byte Reload
	movl	%ecx, 12(%eax)
	movl	48(%esp), %ecx                  # 4-byte Reload
	movl	%ecx, 8(%eax)
	movl	52(%esp), %ecx                  # 4-byte Reload
	movl	%ecx, 4(%eax)
	movl	56(%esp), %ecx                  # 4-byte Reload
	movl	%ecx, (%eax)
.LBB86_2:                               # %nocarry
	addl	$64, %esp
	popl	%esi
	popl	%edi
	popl	%ebx
	popl	%ebp
	retl
.Lfunc_end86:
	.size	mcl_fp_sub16L, .Lfunc_end86-mcl_fp_sub16L
                                        # -- End function
	.globl	mcl_fp_subNF16L                 # -- Begin function mcl_fp_subNF16L
	.p2align	4, 0x90
	.type	mcl_fp_subNF16L,@function
mcl_fp_subNF16L:                        # @mcl_fp_subNF16L
# %bb.0:
	pushl	%ebp
	pushl	%ebx
	pushl	%edi
	pushl	%esi
	subl	$104, %esp
	movl	128(%esp), %ecx
	movl	(%ecx), %esi
	movl	4(%ecx), %edx
	movl	132(%esp), %edi
	subl	(%edi), %esi
	movl	%esi, 96(%esp)                  # 4-byte Spill
	sbbl	4(%edi), %edx
	movl	%edx, 100(%esp)                 # 4-byte Spill
	movl	60(%ecx), %eax
	movl	%eax, 8(%esp)                   # 4-byte Spill
	movl	56(%ecx), %edx
	movl	%edx, 24(%esp)                  # 4-byte Spill
	movl	52(%ecx), %edx
	movl	%edx, 20(%esp)                  # 4-byte Spill
	movl	48(%ecx), %edx
	movl	%edx, 16(%esp)                  # 4-byte Spill
	movl	44(%ecx), %edx
	movl	%edx, 12(%esp)                  # 4-byte Spill
	movl	40(%ecx), %eax
	movl	%eax, 4(%esp)                   # 4-byte Spill
	movl	36(%ecx), %esi
	movl	%esi, 28(%esp)                  # 4-byte Spill
	movl	32(%ecx), %eax
	movl	%eax, (%esp)                    # 4-byte Spill
	movl	28(%ecx), %ebp
	movl	24(%ecx), %ebx
	movl	20(%ecx), %esi
	movl	16(%ecx), %edx
	movl	12(%ecx), %eax
	movl	8(%ecx), %ecx
	sbbl	8(%edi), %ecx
	movl	%ecx, 64(%esp)                  # 4-byte Spill
	sbbl	12(%edi), %eax
	movl	%eax, 72(%esp)                  # 4-byte Spill
	sbbl	16(%edi), %edx
	movl	%edx, 76(%esp)                  # 4-byte Spill
	sbbl	20(%edi), %esi
	movl	%esi, 80(%esp)                  # 4-byte Spill
	sbbl	24(%edi), %ebx
	movl	%ebx, 84(%esp)                  # 4-byte Spill
	sbbl	28(%edi), %ebp
	movl	%ebp, 88(%esp)                  # 4-byte Spill
	movl	(%esp), %eax                    # 4-byte Reload
	sbbl	32(%edi), %eax
	movl	%eax, (%esp)                    # 4-byte Spill
	movl	28(%esp), %ecx                  # 4-byte Reload
	sbbl	36(%edi), %ecx
	movl	%ecx, 28(%esp)                  # 4-byte Spill
	movl	4(%esp), %eax                   # 4-byte Reload
	sbbl	40(%edi), %eax
	movl	%eax, 4(%esp)                   # 4-byte Spill
	movl	12(%esp), %ecx                  # 4-byte Reload
	sbbl	44(%edi), %ecx
	movl	%ecx, 12(%esp)                  # 4-byte Spill
	movl	16(%esp), %ecx                  # 4-byte Reload
	sbbl	48(%edi), %ecx
	movl	%ecx, 16(%esp)                  # 4-byte Spill
	movl	20(%esp), %ecx                  # 4-byte Reload
	sbbl	52(%edi), %ecx
	movl	%ecx, 20(%esp)                  # 4-byte Spill
	movl	24(%esp), %ecx                  # 4-byte Reload
	sbbl	56(%edi), %ecx
	movl	%ecx, 24(%esp)                  # 4-byte Spill
	movl	8(%esp), %eax                   # 4-byte Reload
	sbbl	60(%edi), %eax
	movl	%eax, 8(%esp)                   # 4-byte Spill
	sarl	$31, %eax
	movl	136(%esp), %esi
	movl	60(%esi), %ecx
	andl	%eax, %ecx
	movl	%ecx, 92(%esp)                  # 4-byte Spill
	movl	56(%esi), %ecx
	andl	%eax, %ecx
	movl	%ecx, 68(%esp)                  # 4-byte Spill
	movl	52(%esi), %ecx
	andl	%eax, %ecx
	movl	%ecx, 60(%esp)                  # 4-byte Spill
	movl	48(%esi), %ecx
	andl	%eax, %ecx
	movl	%ecx, 56(%esp)                  # 4-byte Spill
	movl	44(%esi), %ecx
	andl	%eax, %ecx
	movl	%ecx, 52(%esp)                  # 4-byte Spill
	movl	40(%esi), %ecx
	andl	%eax, %ecx
	movl	%ecx, 48(%esp)                  # 4-byte Spill
	movl	36(%esi), %ecx
	andl	%eax, %ecx
	movl	%ecx, 44(%esp)                  # 4-byte Spill
	movl	32(%esi), %ecx
	andl	%eax, %ecx
	movl	%ecx, 40(%esp)                  # 4-byte Spill
	movl	28(%esi), %ecx
	andl	%eax, %ecx
	movl	%ecx, 36(%esp)                  # 4-byte Spill
	movl	24(%esi), %ecx
	andl	%eax, %ecx
	movl	%ecx, 32(%esp)                  # 4-byte Spill
	movl	20(%esi), %ebp
	andl	%eax, %ebp
	movl	16(%esi), %ebx
	andl	%eax, %ebx
	movl	12(%esi), %edi
	andl	%eax, %edi
	movl	8(%esi), %edx
	andl	%eax, %edx
	movl	4(%esi), %ecx
	andl	%eax, %ecx
	andl	(%esi), %eax
	addl	96(%esp), %eax                  # 4-byte Folded Reload
	adcl	100(%esp), %ecx                 # 4-byte Folded Reload
	movl	124(%esp), %esi
	movl	%eax, (%esi)
	adcl	64(%esp), %edx                  # 4-byte Folded Reload
	movl	%ecx, 4(%esi)
	adcl	72(%esp), %edi                  # 4-byte Folded Reload
	movl	%edx, 8(%esi)
	adcl	76(%esp), %ebx                  # 4-byte Folded Reload
	movl	%edi, 12(%esi)
	adcl	80(%esp), %ebp                  # 4-byte Folded Reload
	movl	%ebx, 16(%esi)
	movl	32(%esp), %eax                  # 4-byte Reload
	adcl	84(%esp), %eax                  # 4-byte Folded Reload
	movl	%ebp, 20(%esi)
	movl	36(%esp), %ecx                  # 4-byte Reload
	adcl	88(%esp), %ecx                  # 4-byte Folded Reload
	movl	%eax, 24(%esi)
	movl	40(%esp), %eax                  # 4-byte Reload
	adcl	(%esp), %eax                    # 4-byte Folded Reload
	movl	%ecx, 28(%esi)
	movl	44(%esp), %ecx                  # 4-byte Reload
	adcl	28(%esp), %ecx                  # 4-byte Folded Reload
	movl	%eax, 32(%esi)
	movl	48(%esp), %eax                  # 4-byte Reload
	adcl	4(%esp), %eax                   # 4-byte Folded Reload
	movl	%ecx, 36(%esi)
	movl	52(%esp), %ecx                  # 4-byte Reload
	adcl	12(%esp), %ecx                  # 4-byte Folded Reload
	movl	%eax, 40(%esi)
	movl	56(%esp), %eax                  # 4-byte Reload
	adcl	16(%esp), %eax                  # 4-byte Folded Reload
	movl	%ecx, 44(%esi)
	movl	60(%esp), %ecx                  # 4-byte Reload
	adcl	20(%esp), %ecx                  # 4-byte Folded Reload
	movl	%eax, 48(%esi)
	movl	68(%esp), %eax                  # 4-byte Reload
	adcl	24(%esp), %eax                  # 4-byte Folded Reload
	movl	%ecx, 52(%esi)
	movl	%eax, 56(%esi)
	movl	92(%esp), %eax                  # 4-byte Reload
	adcl	8(%esp), %eax                   # 4-byte Folded Reload
	movl	%eax, 60(%esi)
	addl	$104, %esp
	popl	%esi
	popl	%edi
	popl	%ebx
	popl	%ebp
	retl
.Lfunc_end87:
	.size	mcl_fp_subNF16L, .Lfunc_end87-mcl_fp_subNF16L
                                        # -- End function
	.globl	mcl_fpDbl_add16L                # -- Begin function mcl_fpDbl_add16L
	.p2align	4, 0x90
	.type	mcl_fpDbl_add16L,@function
mcl_fpDbl_add16L:                       # @mcl_fpDbl_add16L
# %bb.0:
	pushl	%ebp
	pushl	%ebx
	pushl	%edi
	pushl	%esi
	subl	$128, %esp
	movl	152(%esp), %edx
	movl	(%edx), %eax
	movl	4(%edx), %esi
	movl	156(%esp), %ecx
	addl	(%ecx), %eax
	movl	%eax, 100(%esp)                 # 4-byte Spill
	adcl	4(%ecx), %esi
	movl	%esi, 96(%esp)                  # 4-byte Spill
	movl	124(%edx), %eax
	movl	%eax, 24(%esp)                  # 4-byte Spill
	movl	120(%edx), %eax
	movl	%eax, 28(%esp)                  # 4-byte Spill
	movl	116(%edx), %eax
	movl	%eax, 32(%esp)                  # 4-byte Spill
	movl	112(%edx), %eax
	movl	%eax, 36(%esp)                  # 4-byte Spill
	movl	108(%edx), %eax
	movl	%eax, 40(%esp)                  # 4-byte Spill
	movl	104(%edx), %eax
	movl	%eax, 44(%esp)                  # 4-byte Spill
	movl	100(%edx), %eax
	movl	%eax, 48(%esp)                  # 4-byte Spill
	movl	96(%edx), %eax
	movl	%eax, 52(%esp)                  # 4-byte Spill
	movl	92(%edx), %eax
	movl	%eax, 56(%esp)                  # 4-byte Spill
	movl	88(%edx), %eax
	movl	%eax, 60(%esp)                  # 4-byte Spill
	movl	84(%edx), %eax
	movl	%eax, 64(%esp)                  # 4-byte Spill
	movl	80(%edx), %eax
	movl	%eax, 68(%esp)                  # 4-byte Spill
	movl	76(%edx), %eax
	movl	%eax, 72(%esp)                  # 4-byte Spill
	movl	72(%edx), %eax
	movl	%eax, 76(%esp)                  # 4-byte Spill
	movl	68(%edx), %eax
	movl	%eax, 104(%esp)                 # 4-byte Spill
	movl	64(%edx), %eax
	movl	%eax, 108(%esp)                 # 4-byte Spill
	movl	60(%edx), %eax
	movl	%eax, 12(%esp)                  # 4-byte Spill
	movl	56(%edx), %eax
	movl	%eax, 16(%esp)                  # 4-byte Spill
	movl	52(%edx), %eax
	movl	%eax, 20(%esp)                  # 4-byte Spill
	movl	48(%edx), %eax
	movl	%eax, (%esp)                    # 4-byte Spill
	movl	44(%edx), %eax
	movl	%eax, 4(%esp)                   # 4-byte Spill
	movl	40(%edx), %eax
	movl	%eax, 8(%esp)                   # 4-byte Spill
	movl	36(%edx), %eax
	movl	%eax, 116(%esp)                 # 4-byte Spill
	movl	32(%edx), %eax
	movl	%eax, 112(%esp)                 # 4-byte Spill
	movl	28(%edx), %ebp
	movl	24(%edx), %ebx
	movl	20(%edx), %edi
	movl	16(%edx), %esi
	movl	12(%edx), %eax
	movl	8(%edx), %edx
	adcl	8(%ecx), %edx
	movl	%edx, 92(%esp)                  # 4-byte Spill
	adcl	12(%ecx), %eax
	movl	%eax, 88(%esp)                  # 4-byte Spill
	adcl	16(%ecx), %esi
	movl	%esi, 84(%esp)                  # 4-byte Spill
	adcl	20(%ecx), %edi
	movl	%edi, 80(%esp)                  # 4-byte Spill
	adcl	24(%ecx), %ebx
	movl	%ebx, 124(%esp)                 # 4-byte Spill
	adcl	28(%ecx), %ebp
	movl	%ebp, 120(%esp)                 # 4-byte Spill
	movl	112(%esp), %esi                 # 4-byte Reload
	adcl	32(%ecx), %esi
	movl	116(%esp), %eax                 # 4-byte Reload
	adcl	36(%ecx), %eax
	movl	8(%esp), %edx                   # 4-byte Reload
	adcl	40(%ecx), %edx
	movl	%edx, 8(%esp)                   # 4-byte Spill
	movl	4(%esp), %edx                   # 4-byte Reload
	adcl	44(%ecx), %edx
	movl	%edx, 4(%esp)                   # 4-byte Spill
	movl	(%esp), %edx                    # 4-byte Reload
	adcl	48(%ecx), %edx
	movl	%edx, (%esp)                    # 4-byte Spill
	movl	20(%esp), %edx                  # 4-byte Reload
	adcl	52(%ecx), %edx
	movl	%edx, 20(%esp)                  # 4-byte Spill
	movl	16(%esp), %edx                  # 4-byte Reload
	adcl	56(%ecx), %edx
	movl	%edx, 16(%esp)                  # 4-byte Spill
	movl	12(%esp), %edx                  # 4-byte Reload
	adcl	60(%ecx), %edx
	movl	%edx, 12(%esp)                  # 4-byte Spill
	movl	108(%esp), %ebx                 # 4-byte Reload
	adcl	64(%ecx), %ebx
	movl	104(%esp), %edi                 # 4-byte Reload
	adcl	68(%ecx), %edi
	movl	76(%esp), %edx                  # 4-byte Reload
	adcl	72(%ecx), %edx
	movl	%edx, 76(%esp)                  # 4-byte Spill
	movl	72(%esp), %edx                  # 4-byte Reload
	adcl	76(%ecx), %edx
	movl	%edx, 72(%esp)                  # 4-byte Spill
	movl	68(%esp), %edx                  # 4-byte Reload
	adcl	80(%ecx), %edx
	movl	%edx, 68(%esp)                  # 4-byte Spill
	movl	64(%esp), %edx                  # 4-byte Reload
	adcl	84(%ecx), %edx
	movl	%edx, 64(%esp)                  # 4-byte Spill
	movl	60(%esp), %edx                  # 4-byte Reload
	adcl	88(%ecx), %edx
	movl	%edx, 60(%esp)                  # 4-byte Spill
	movl	56(%esp), %edx                  # 4-byte Reload
	adcl	92(%ecx), %edx
	movl	%edx, 56(%esp)                  # 4-byte Spill
	movl	52(%esp), %edx                  # 4-byte Reload
	adcl	96(%ecx), %edx
	movl	%edx, 52(%esp)                  # 4-byte Spill
	movl	48(%esp), %edx                  # 4-byte Reload
	adcl	100(%ecx), %edx
	movl	%edx, 48(%esp)                  # 4-byte Spill
	movl	44(%esp), %ebp                  # 4-byte Reload
	adcl	104(%ecx), %ebp
	movl	%ebp, 44(%esp)                  # 4-byte Spill
	movl	40(%esp), %ebp                  # 4-byte Reload
	adcl	108(%ecx), %ebp
	movl	%ebp, 40(%esp)                  # 4-byte Spill
	movl	36(%esp), %ebp                  # 4-byte Reload
	adcl	112(%ecx), %ebp
	movl	%ebp, 36(%esp)                  # 4-byte Spill
	movl	32(%esp), %ebp                  # 4-byte Reload
	adcl	116(%ecx), %ebp
	movl	%ebp, 32(%esp)                  # 4-byte Spill
	movl	28(%esp), %ebp                  # 4-byte Reload
	adcl	120(%ecx), %ebp
	movl	%ebp, 28(%esp)                  # 4-byte Spill
	movl	24(%esp), %ebp                  # 4-byte Reload
	adcl	124(%ecx), %ebp
	movl	%ebp, 24(%esp)                  # 4-byte Spill
	movl	148(%esp), %ebp
	movl	12(%esp), %ecx                  # 4-byte Reload
	movl	%ecx, 60(%ebp)
	movl	16(%esp), %ecx                  # 4-byte Reload
	movl	%ecx, 56(%ebp)
	movl	20(%esp), %ecx                  # 4-byte Reload
	movl	%ecx, 52(%ebp)
	movl	(%esp), %ecx                    # 4-byte Reload
	movl	%ecx, 48(%ebp)
	movl	4(%esp), %ecx                   # 4-byte Reload
	movl	%ecx, 44(%ebp)
	movl	8(%esp), %ecx                   # 4-byte Reload
	movl	%ecx, 40(%ebp)
	movl	%eax, 36(%ebp)
	movl	%esi, 32(%ebp)
	movl	120(%esp), %eax                 # 4-byte Reload
	movl	%eax, 28(%ebp)
	movl	124(%esp), %eax                 # 4-byte Reload
	movl	%eax, 24(%ebp)
	movl	80(%esp), %eax                  # 4-byte Reload
	movl	%eax, 20(%ebp)
	movl	84(%esp), %eax                  # 4-byte Reload
	movl	%eax, 16(%ebp)
	movl	88(%esp), %eax                  # 4-byte Reload
	movl	%eax, 12(%ebp)
	movl	92(%esp), %eax                  # 4-byte Reload
	movl	%eax, 8(%ebp)
	movl	96(%esp), %ecx                  # 4-byte Reload
	movl	%ecx, 4(%ebp)
	movl	100(%esp), %ecx                 # 4-byte Reload
	movl	%ecx, (%ebp)
	setb	80(%esp)                        # 1-byte Folded Spill
	movl	160(%esp), %ecx
	movl	%ebx, %eax
	movl	%ebx, 108(%esp)                 # 4-byte Spill
	subl	(%ecx), %eax
	movl	%eax, 20(%esp)                  # 4-byte Spill
	movl	%edi, %esi
	movl	%edi, 104(%esp)                 # 4-byte Spill
	sbbl	4(%ecx), %esi
	movl	%esi, 16(%esp)                  # 4-byte Spill
	movl	76(%esp), %edi                  # 4-byte Reload
	sbbl	8(%ecx), %edi
	movl	%edi, 12(%esp)                  # 4-byte Spill
	movl	72(%esp), %eax                  # 4-byte Reload
	sbbl	12(%ecx), %eax
	movl	%eax, 100(%esp)                 # 4-byte Spill
	movl	68(%esp), %eax                  # 4-byte Reload
	sbbl	16(%ecx), %eax
	movl	%eax, 96(%esp)                  # 4-byte Spill
	movl	64(%esp), %eax                  # 4-byte Reload
	sbbl	20(%ecx), %eax
	movl	%eax, 92(%esp)                  # 4-byte Spill
	movl	60(%esp), %eax                  # 4-byte Reload
	sbbl	24(%ecx), %eax
	movl	%eax, 8(%esp)                   # 4-byte Spill
	movl	56(%esp), %eax                  # 4-byte Reload
	sbbl	28(%ecx), %eax
	movl	%eax, 4(%esp)                   # 4-byte Spill
	movl	52(%esp), %eax                  # 4-byte Reload
	sbbl	32(%ecx), %eax
	movl	%eax, (%esp)                    # 4-byte Spill
	movl	48(%esp), %eax                  # 4-byte Reload
	sbbl	36(%ecx), %eax
	movl	%eax, 88(%esp)                  # 4-byte Spill
	movl	44(%esp), %eax                  # 4-byte Reload
	sbbl	40(%ecx), %eax
	movl	%eax, 84(%esp)                  # 4-byte Spill
	movl	40(%esp), %edx                  # 4-byte Reload
	sbbl	44(%ecx), %edx
	movl	36(%esp), %esi                  # 4-byte Reload
	sbbl	48(%ecx), %esi
	movl	32(%esp), %edi                  # 4-byte Reload
	sbbl	52(%ecx), %edi
	movl	28(%esp), %ebx                  # 4-byte Reload
	sbbl	56(%ecx), %ebx
	movl	24(%esp), %eax                  # 4-byte Reload
	sbbl	60(%ecx), %eax
	movzbl	80(%esp), %ecx                  # 1-byte Folded Reload
	sbbl	$0, %ecx
	testb	$1, %cl
	jne	.LBB88_1
# %bb.2:
	movl	%eax, 124(%ebp)
	jne	.LBB88_3
.LBB88_4:
	movl	%ebx, 120(%ebp)
	movl	20(%esp), %eax                  # 4-byte Reload
	movl	16(%esp), %ecx                  # 4-byte Reload
	jne	.LBB88_5
.LBB88_6:
	movl	%edi, 116(%ebp)
	movl	92(%esp), %ebx                  # 4-byte Reload
	jne	.LBB88_7
.LBB88_8:
	movl	%esi, 112(%ebp)
	movl	96(%esp), %edi                  # 4-byte Reload
	jne	.LBB88_9
.LBB88_10:
	movl	%edx, 108(%ebp)
	movl	100(%esp), %esi                 # 4-byte Reload
	movl	84(%esp), %edx                  # 4-byte Reload
	jne	.LBB88_11
.LBB88_12:
	movl	%edx, 104(%ebp)
	movl	88(%esp), %edx                  # 4-byte Reload
	jne	.LBB88_13
.LBB88_14:
	movl	%edx, 100(%ebp)
	jne	.LBB88_15
.LBB88_16:
	movl	%ecx, %edx
	movl	(%esp), %ecx                    # 4-byte Reload
	movl	%ecx, 96(%ebp)
	jne	.LBB88_17
.LBB88_18:
	movl	%eax, %ecx
	movl	4(%esp), %eax                   # 4-byte Reload
	movl	%eax, 92(%ebp)
	jne	.LBB88_19
.LBB88_20:
	movl	8(%esp), %eax                   # 4-byte Reload
	movl	%eax, 88(%ebp)
	jne	.LBB88_21
.LBB88_22:
	movl	%ebx, 84(%ebp)
	movl	%ecx, %eax
	jne	.LBB88_23
.LBB88_24:
	movl	%edi, 80(%ebp)
	movl	12(%esp), %ecx                  # 4-byte Reload
	jne	.LBB88_25
.LBB88_26:
	movl	%esi, 76(%ebp)
	jne	.LBB88_27
.LBB88_28:
	movl	%ecx, 72(%ebp)
	jne	.LBB88_29
.LBB88_30:
	movl	%edx, 68(%ebp)
	je	.LBB88_32
.LBB88_31:
	movl	108(%esp), %eax                 # 4-byte Reload
.LBB88_32:
	movl	%eax, 64(%ebp)
	addl	$128, %esp
	popl	%esi
	popl	%edi
	popl	%ebx
	popl	%ebp
	retl
.LBB88_1:
	movl	24(%esp), %eax                  # 4-byte Reload
	movl	%eax, 124(%ebp)
	je	.LBB88_4
.LBB88_3:
	movl	28(%esp), %ebx                  # 4-byte Reload
	movl	%ebx, 120(%ebp)
	movl	20(%esp), %eax                  # 4-byte Reload
	movl	16(%esp), %ecx                  # 4-byte Reload
	je	.LBB88_6
.LBB88_5:
	movl	32(%esp), %edi                  # 4-byte Reload
	movl	%edi, 116(%ebp)
	movl	92(%esp), %ebx                  # 4-byte Reload
	je	.LBB88_8
.LBB88_7:
	movl	36(%esp), %esi                  # 4-byte Reload
	movl	%esi, 112(%ebp)
	movl	96(%esp), %edi                  # 4-byte Reload
	je	.LBB88_10
.LBB88_9:
	movl	40(%esp), %edx                  # 4-byte Reload
	movl	%edx, 108(%ebp)
	movl	100(%esp), %esi                 # 4-byte Reload
	movl	84(%esp), %edx                  # 4-byte Reload
	je	.LBB88_12
.LBB88_11:
	movl	44(%esp), %edx                  # 4-byte Reload
	movl	%edx, 104(%ebp)
	movl	88(%esp), %edx                  # 4-byte Reload
	je	.LBB88_14
.LBB88_13:
	movl	48(%esp), %edx                  # 4-byte Reload
	movl	%edx, 100(%ebp)
	je	.LBB88_16
.LBB88_15:
	movl	52(%esp), %edx                  # 4-byte Reload
	movl	%edx, (%esp)                    # 4-byte Spill
	movl	%ecx, %edx
	movl	(%esp), %ecx                    # 4-byte Reload
	movl	%ecx, 96(%ebp)
	je	.LBB88_18
.LBB88_17:
	movl	56(%esp), %ecx                  # 4-byte Reload
	movl	%ecx, 4(%esp)                   # 4-byte Spill
	movl	%eax, %ecx
	movl	4(%esp), %eax                   # 4-byte Reload
	movl	%eax, 92(%ebp)
	je	.LBB88_20
.LBB88_19:
	movl	60(%esp), %eax                  # 4-byte Reload
	movl	%eax, 8(%esp)                   # 4-byte Spill
	movl	8(%esp), %eax                   # 4-byte Reload
	movl	%eax, 88(%ebp)
	je	.LBB88_22
.LBB88_21:
	movl	64(%esp), %ebx                  # 4-byte Reload
	movl	%ebx, 84(%ebp)
	movl	%ecx, %eax
	je	.LBB88_24
.LBB88_23:
	movl	68(%esp), %edi                  # 4-byte Reload
	movl	%edi, 80(%ebp)
	movl	12(%esp), %ecx                  # 4-byte Reload
	je	.LBB88_26
.LBB88_25:
	movl	72(%esp), %esi                  # 4-byte Reload
	movl	%esi, 76(%ebp)
	je	.LBB88_28
.LBB88_27:
	movl	76(%esp), %ecx                  # 4-byte Reload
	movl	%ecx, 72(%ebp)
	je	.LBB88_30
.LBB88_29:
	movl	104(%esp), %edx                 # 4-byte Reload
	movl	%edx, 68(%ebp)
	jne	.LBB88_31
	jmp	.LBB88_32
.Lfunc_end88:
	.size	mcl_fpDbl_add16L, .Lfunc_end88-mcl_fpDbl_add16L
                                        # -- End function
	.globl	mcl_fpDbl_sub16L                # -- Begin function mcl_fpDbl_sub16L
	.p2align	4, 0x90
	.type	mcl_fpDbl_sub16L,@function
mcl_fpDbl_sub16L:                       # @mcl_fpDbl_sub16L
# %bb.0:
	pushl	%ebp
	pushl	%ebx
	pushl	%edi
	pushl	%esi
	subl	$128, %esp
	movl	152(%esp), %edx
	movl	(%edx), %eax
	movl	4(%edx), %edi
	xorl	%esi, %esi
	movl	156(%esp), %ecx
	subl	(%ecx), %eax
	movl	%eax, 40(%esp)                  # 4-byte Spill
	sbbl	4(%ecx), %edi
	movl	%edi, 36(%esp)                  # 4-byte Spill
	movl	124(%edx), %eax
	movl	%eax, 64(%esp)                  # 4-byte Spill
	movl	120(%edx), %eax
	movl	%eax, 104(%esp)                 # 4-byte Spill
	movl	116(%edx), %eax
	movl	%eax, 100(%esp)                 # 4-byte Spill
	movl	112(%edx), %eax
	movl	%eax, 96(%esp)                  # 4-byte Spill
	movl	108(%edx), %eax
	movl	%eax, 92(%esp)                  # 4-byte Spill
	movl	104(%edx), %eax
	movl	%eax, 88(%esp)                  # 4-byte Spill
	movl	100(%edx), %eax
	movl	%eax, 84(%esp)                  # 4-byte Spill
	movl	96(%edx), %eax
	movl	%eax, 80(%esp)                  # 4-byte Spill
	movl	92(%edx), %eax
	movl	%eax, 76(%esp)                  # 4-byte Spill
	movl	88(%edx), %eax
	movl	%eax, 72(%esp)                  # 4-byte Spill
	movl	84(%edx), %eax
	movl	%eax, 68(%esp)                  # 4-byte Spill
	movl	80(%edx), %eax
	movl	%eax, 56(%esp)                  # 4-byte Spill
	movl	76(%edx), %eax
	movl	%eax, 52(%esp)                  # 4-byte Spill
	movl	72(%edx), %eax
	movl	%eax, 48(%esp)                  # 4-byte Spill
	movl	68(%edx), %eax
	movl	%eax, 44(%esp)                  # 4-byte Spill
	movl	64(%edx), %ebp
	movl	%ebp, 60(%esp)                  # 4-byte Spill
	movl	60(%edx), %eax
	movl	%eax, (%esp)                    # 4-byte Spill
	movl	56(%edx), %eax
	movl	%eax, 4(%esp)                   # 4-byte Spill
	movl	52(%edx), %eax
	movl	%eax, 8(%esp)                   # 4-byte Spill
	movl	48(%edx), %eax
	movl	%eax, 12(%esp)                  # 4-byte Spill
	movl	44(%edx), %eax
	movl	%eax, 16(%esp)                  # 4-byte Spill
	movl	40(%edx), %eax
	movl	%eax, 20(%esp)                  # 4-byte Spill
	movl	36(%edx), %eax
	movl	%eax, 24(%esp)                  # 4-byte Spill
	movl	32(%edx), %eax
	movl	%eax, 112(%esp)                 # 4-byte Spill
	movl	28(%edx), %eax
	movl	%eax, 108(%esp)                 # 4-byte Spill
	movl	24(%edx), %ebp
	movl	20(%edx), %ebx
	movl	16(%edx), %edi
	movl	12(%edx), %eax
	movl	8(%edx), %edx
	sbbl	8(%ecx), %edx
	movl	%edx, 32(%esp)                  # 4-byte Spill
	sbbl	12(%ecx), %eax
	movl	%eax, 28(%esp)                  # 4-byte Spill
	sbbl	16(%ecx), %edi
	movl	%edi, 124(%esp)                 # 4-byte Spill
	sbbl	20(%ecx), %ebx
	movl	%ebx, 120(%esp)                 # 4-byte Spill
	sbbl	24(%ecx), %ebp
	movl	%ebp, 116(%esp)                 # 4-byte Spill
	movl	108(%esp), %ebp                 # 4-byte Reload
	sbbl	28(%ecx), %ebp
	movl	112(%esp), %edi                 # 4-byte Reload
	sbbl	32(%ecx), %edi
	movl	24(%esp), %eax                  # 4-byte Reload
	sbbl	36(%ecx), %eax
	movl	%eax, 24(%esp)                  # 4-byte Spill
	movl	20(%esp), %eax                  # 4-byte Reload
	sbbl	40(%ecx), %eax
	movl	%eax, 20(%esp)                  # 4-byte Spill
	movl	16(%esp), %eax                  # 4-byte Reload
	sbbl	44(%ecx), %eax
	movl	%eax, 16(%esp)                  # 4-byte Spill
	movl	12(%esp), %eax                  # 4-byte Reload
	sbbl	48(%ecx), %eax
	movl	%eax, 12(%esp)                  # 4-byte Spill
	movl	8(%esp), %eax                   # 4-byte Reload
	sbbl	52(%ecx), %eax
	movl	%eax, 8(%esp)                   # 4-byte Spill
	movl	4(%esp), %eax                   # 4-byte Reload
	sbbl	56(%ecx), %eax
	movl	%eax, 4(%esp)                   # 4-byte Spill
	movl	(%esp), %eax                    # 4-byte Reload
	sbbl	60(%ecx), %eax
	movl	%eax, (%esp)                    # 4-byte Spill
	movl	60(%esp), %edx                  # 4-byte Reload
	sbbl	64(%ecx), %edx
	movl	%edx, 60(%esp)                  # 4-byte Spill
	movl	44(%esp), %eax                  # 4-byte Reload
	sbbl	68(%ecx), %eax
	movl	%eax, 44(%esp)                  # 4-byte Spill
	movl	48(%esp), %eax                  # 4-byte Reload
	sbbl	72(%ecx), %eax
	movl	%eax, 48(%esp)                  # 4-byte Spill
	movl	52(%esp), %eax                  # 4-byte Reload
	sbbl	76(%ecx), %eax
	movl	%eax, 52(%esp)                  # 4-byte Spill
	movl	56(%esp), %eax                  # 4-byte Reload
	sbbl	80(%ecx), %eax
	movl	%eax, 56(%esp)                  # 4-byte Spill
	movl	68(%esp), %edx                  # 4-byte Reload
	sbbl	84(%ecx), %edx
	movl	%edx, 68(%esp)                  # 4-byte Spill
	movl	72(%esp), %edx                  # 4-byte Reload
	sbbl	88(%ecx), %edx
	movl	%edx, 72(%esp)                  # 4-byte Spill
	movl	76(%esp), %edx                  # 4-byte Reload
	sbbl	92(%ecx), %edx
	movl	%edx, 76(%esp)                  # 4-byte Spill
	movl	80(%esp), %edx                  # 4-byte Reload
	sbbl	96(%ecx), %edx
	movl	%edx, 80(%esp)                  # 4-byte Spill
	movl	84(%esp), %edx                  # 4-byte Reload
	sbbl	100(%ecx), %edx
	movl	%edx, 84(%esp)                  # 4-byte Spill
	movl	88(%esp), %edx                  # 4-byte Reload
	sbbl	104(%ecx), %edx
	movl	%edx, 88(%esp)                  # 4-byte Spill
	movl	92(%esp), %edx                  # 4-byte Reload
	sbbl	108(%ecx), %edx
	movl	%edx, 92(%esp)                  # 4-byte Spill
	movl	96(%esp), %edx                  # 4-byte Reload
	sbbl	112(%ecx), %edx
	movl	%edx, 96(%esp)                  # 4-byte Spill
	movl	100(%esp), %edx                 # 4-byte Reload
	sbbl	116(%ecx), %edx
	movl	%edx, 100(%esp)                 # 4-byte Spill
	movl	104(%esp), %edx                 # 4-byte Reload
	sbbl	120(%ecx), %edx
	movl	%edx, 104(%esp)                 # 4-byte Spill
	movl	64(%esp), %eax                  # 4-byte Reload
	sbbl	124(%ecx), %eax
	movl	%eax, 64(%esp)                  # 4-byte Spill
	movl	148(%esp), %ebx
	movl	(%esp), %eax                    # 4-byte Reload
	movl	%eax, 60(%ebx)
	movl	4(%esp), %eax                   # 4-byte Reload
	movl	%eax, 56(%ebx)
	movl	8(%esp), %eax                   # 4-byte Reload
	movl	%eax, 52(%ebx)
	movl	12(%esp), %eax                  # 4-byte Reload
	movl	%eax, 48(%ebx)
	movl	16(%esp), %eax                  # 4-byte Reload
	movl	%eax, 44(%ebx)
	movl	20(%esp), %eax                  # 4-byte Reload
	movl	%eax, 40(%ebx)
	movl	24(%esp), %eax                  # 4-byte Reload
	movl	%eax, 36(%ebx)
	movl	%edi, 32(%ebx)
	movl	%ebp, 28(%ebx)
	movl	116(%esp), %eax                 # 4-byte Reload
	movl	%eax, 24(%ebx)
	movl	120(%esp), %eax                 # 4-byte Reload
	movl	%eax, 20(%ebx)
	movl	124(%esp), %eax                 # 4-byte Reload
	movl	%eax, 16(%ebx)
	movl	28(%esp), %eax                  # 4-byte Reload
	movl	%eax, 12(%ebx)
	movl	32(%esp), %eax                  # 4-byte Reload
	movl	%eax, 8(%ebx)
	movl	36(%esp), %eax                  # 4-byte Reload
	movl	%eax, 4(%ebx)
	movl	40(%esp), %eax                  # 4-byte Reload
	movl	%eax, (%ebx)
	sbbl	%esi, %esi
	andl	$1, %esi
	negl	%esi
	movl	160(%esp), %edi
	movl	60(%edi), %eax
	andl	%esi, %eax
	movl	%eax, 24(%esp)                  # 4-byte Spill
	movl	56(%edi), %eax
	andl	%esi, %eax
	movl	%eax, 20(%esp)                  # 4-byte Spill
	movl	52(%edi), %eax
	andl	%esi, %eax
	movl	%eax, 16(%esp)                  # 4-byte Spill
	movl	48(%edi), %eax
	andl	%esi, %eax
	movl	%eax, 12(%esp)                  # 4-byte Spill
	movl	44(%edi), %eax
	andl	%esi, %eax
	movl	%eax, 8(%esp)                   # 4-byte Spill
	movl	40(%edi), %eax
	andl	%esi, %eax
	movl	%eax, 4(%esp)                   # 4-byte Spill
	movl	36(%edi), %eax
	andl	%esi, %eax
	movl	%eax, (%esp)                    # 4-byte Spill
	movl	32(%edi), %eax
	andl	%esi, %eax
	movl	%eax, 40(%esp)                  # 4-byte Spill
	movl	28(%edi), %eax
	andl	%esi, %eax
	movl	%eax, 36(%esp)                  # 4-byte Spill
	movl	24(%edi), %eax
	andl	%esi, %eax
	movl	%eax, 32(%esp)                  # 4-byte Spill
	movl	20(%edi), %eax
	andl	%esi, %eax
	movl	%eax, 28(%esp)                  # 4-byte Spill
	movl	16(%edi), %ebp
	andl	%esi, %ebp
	movl	12(%edi), %edx
	andl	%esi, %edx
	movl	8(%edi), %ecx
	andl	%esi, %ecx
	movl	4(%edi), %eax
	andl	%esi, %eax
	andl	(%edi), %esi
	addl	60(%esp), %esi                  # 4-byte Folded Reload
	adcl	44(%esp), %eax                  # 4-byte Folded Reload
	movl	%esi, 64(%ebx)
	adcl	48(%esp), %ecx                  # 4-byte Folded Reload
	movl	%eax, 68(%ebx)
	adcl	52(%esp), %edx                  # 4-byte Folded Reload
	movl	%ecx, 72(%ebx)
	adcl	56(%esp), %ebp                  # 4-byte Folded Reload
	movl	%edx, 76(%ebx)
	movl	28(%esp), %ecx                  # 4-byte Reload
	adcl	68(%esp), %ecx                  # 4-byte Folded Reload
	movl	%ebp, 80(%ebx)
	movl	32(%esp), %eax                  # 4-byte Reload
	adcl	72(%esp), %eax                  # 4-byte Folded Reload
	movl	%ecx, 84(%ebx)
	movl	36(%esp), %ecx                  # 4-byte Reload
	adcl	76(%esp), %ecx                  # 4-byte Folded Reload
	movl	%eax, 88(%ebx)
	movl	40(%esp), %eax                  # 4-byte Reload
	adcl	80(%esp), %eax                  # 4-byte Folded Reload
	movl	%ecx, 92(%ebx)
	movl	(%esp), %ecx                    # 4-byte Reload
	adcl	84(%esp), %ecx                  # 4-byte Folded Reload
	movl	%eax, 96(%ebx)
	movl	4(%esp), %eax                   # 4-byte Reload
	adcl	88(%esp), %eax                  # 4-byte Folded Reload
	movl	%ecx, 100(%ebx)
	movl	8(%esp), %ecx                   # 4-byte Reload
	adcl	92(%esp), %ecx                  # 4-byte Folded Reload
	movl	%eax, 104(%ebx)
	movl	12(%esp), %eax                  # 4-byte Reload
	adcl	96(%esp), %eax                  # 4-byte Folded Reload
	movl	%ecx, 108(%ebx)
	movl	16(%esp), %ecx                  # 4-byte Reload
	adcl	100(%esp), %ecx                 # 4-byte Folded Reload
	movl	%eax, 112(%ebx)
	movl	20(%esp), %eax                  # 4-byte Reload
	adcl	104(%esp), %eax                 # 4-byte Folded Reload
	movl	%ecx, 116(%ebx)
	movl	%eax, 120(%ebx)
	movl	24(%esp), %eax                  # 4-byte Reload
	adcl	64(%esp), %eax                  # 4-byte Folded Reload
	movl	%eax, 124(%ebx)
	addl	$128, %esp
	popl	%esi
	popl	%edi
	popl	%ebx
	popl	%ebp
	retl
.Lfunc_end89:
	.size	mcl_fpDbl_sub16L, .Lfunc_end89-mcl_fpDbl_sub16L
                                        # -- End function
	.section	".note.GNU-stack","",@progbits
