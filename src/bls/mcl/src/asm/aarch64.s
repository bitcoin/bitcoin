	.text
	.file	"<stdin>"
	.globl	makeNIST_P192L
	.p2align	2
	.type	makeNIST_P192L,@function
makeNIST_P192L:                         // @makeNIST_P192L
// BB#0:
	mov	x0, #-1
	orr	x1, xzr, #0xfffffffffffffffe
	mov	x2, #-1
	ret
.Lfunc_end0:
	.size	makeNIST_P192L, .Lfunc_end0-makeNIST_P192L

	.globl	mcl_fpDbl_mod_NIST_P192L
	.p2align	2
	.type	mcl_fpDbl_mod_NIST_P192L,@function
mcl_fpDbl_mod_NIST_P192L:               // @mcl_fpDbl_mod_NIST_P192L
// BB#0:
	ldp		x8, x9, [x1]
	ldp	x10, x11, [x1, #32]
	ldp	x12, x13, [x1, #16]
	orr	w14, wzr, #0x1
	adds		x9, x11, x9
	adcs	x12, x12, xzr
	adcs	x15, xzr, xzr
	adds		x8, x8, x13
	adcs	x9, x9, x10
	adcs	x12, x12, x11
	adcs	x15, x15, xzr
	adds		x8, x8, x11
	adcs	x9, x9, x13
	adcs	x10, x12, x10
	adcs	x12, x15, xzr
	adds		x8, x12, x8
	adcs	x9, x12, x9
	adcs	x10, x10, xzr
	adcs	x12, xzr, xzr
	adds	x13, x8, #1             // =1
	adcs	x14, x9, x14
	mov	x11, #-1
	adcs	x15, x10, xzr
	adcs	x11, x12, x11
	tst	 x11, #0x1
	csel	x8, x8, x13, ne
	csel	x9, x9, x14, ne
	csel	x10, x10, x15, ne
	stp		x8, x9, [x0]
	str	x10, [x0, #16]
	ret
.Lfunc_end1:
	.size	mcl_fpDbl_mod_NIST_P192L, .Lfunc_end1-mcl_fpDbl_mod_NIST_P192L

	.globl	mcl_fp_sqr_NIST_P192L
	.p2align	2
	.type	mcl_fp_sqr_NIST_P192L,@function
mcl_fp_sqr_NIST_P192L:                  // @mcl_fp_sqr_NIST_P192L
// BB#0:
	ldp	x10, x8, [x1, #8]
	ldr		x9, [x1]
	umulh	x16, x8, x10
	mul		x14, x10, x9
	umulh	x15, x9, x9
	mul		x12, x8, x9
	umulh	x13, x10, x9
	adds		x15, x15, x14
	umulh	x11, x8, x9
	adcs	x1, x13, x12
	mul		x17, x8, x10
	umulh	x18, x10, x10
	mul		x10, x10, x10
	adcs	x2, x11, xzr
	adds		x10, x13, x10
	adcs	x13, x18, x17
	adcs	x18, x16, xzr
	adds		x14, x14, x15
	adcs	x10, x10, x1
	adcs	x13, x13, x2
	adcs	x18, x18, xzr
	umulh	x2, x8, x8
	mul		x8, x8, x8
	adds		x11, x11, x17
	adcs	x8, x16, x8
	adcs	x16, x2, xzr
	adds		x10, x12, x10
	adcs	x11, x11, x13
	adcs	x8, x8, x18
	adcs	x12, x16, xzr
	adds		x13, x14, x12
	adcs	x10, x10, xzr
	mul		x9, x9, x9
	adcs	x14, xzr, xzr
	adds		x9, x9, x11
	adcs	x13, x13, x8
	adcs	x10, x10, x12
	adcs	x14, x14, xzr
	adds		x9, x9, x12
	adcs	x11, x13, x11
	adcs	x8, x10, x8
	adcs	x10, x14, xzr
	adds		x9, x10, x9
	adcs	x10, x10, x11
	adcs	x8, x8, xzr
	adcs	x11, xzr, xzr
	orr	w15, wzr, #0x1
	adds	x12, x9, #1             // =1
	adcs	x13, x10, x15
	mov	x1, #-1
	adcs	x14, x8, xzr
	adcs	x11, x11, x1
	tst	 x11, #0x1
	csel	x9, x9, x12, ne
	csel	x10, x10, x13, ne
	csel	x8, x8, x14, ne
	stp		x9, x10, [x0]
	str	x8, [x0, #16]
	ret
.Lfunc_end2:
	.size	mcl_fp_sqr_NIST_P192L, .Lfunc_end2-mcl_fp_sqr_NIST_P192L

	.globl	mcl_fp_mulNIST_P192L
	.p2align	2
	.type	mcl_fp_mulNIST_P192L,@function
mcl_fp_mulNIST_P192L:                   // @mcl_fp_mulNIST_P192L
// BB#0:
	sub	sp, sp, #64             // =64
	stp	x19, x30, [sp, #48]     // 8-byte Folded Spill
	mov	 x19, x0
	mov	 x0, sp
	bl	mcl_fpDbl_mulPre3L
	ldp	x9, x8, [sp, #8]
	ldp	x11, x10, [sp, #32]
	ldr	x12, [sp, #24]
	ldr		x13, [sp]
	orr	w14, wzr, #0x1
	adds		x9, x10, x9
	adcs	x8, x8, xzr
	adcs	x15, xzr, xzr
	adds		x13, x13, x12
	adcs	x9, x9, x11
	adcs	x8, x8, x10
	adcs	x15, x15, xzr
	adds		x10, x13, x10
	adcs	x9, x9, x12
	adcs	x8, x8, x11
	adcs	x11, x15, xzr
	adds		x10, x11, x10
	adcs	x9, x11, x9
	adcs	x8, x8, xzr
	adcs	x11, xzr, xzr
	adds	x12, x10, #1            // =1
	adcs	x14, x9, x14
	mov	x13, #-1
	adcs	x15, x8, xzr
	adcs	x11, x11, x13
	tst	 x11, #0x1
	csel	x10, x10, x12, ne
	csel	x9, x9, x14, ne
	csel	x8, x8, x15, ne
	stp		x10, x9, [x19]
	str	x8, [x19, #16]
	ldp	x19, x30, [sp, #48]     // 8-byte Folded Reload
	add	sp, sp, #64             // =64
	ret
.Lfunc_end3:
	.size	mcl_fp_mulNIST_P192L, .Lfunc_end3-mcl_fp_mulNIST_P192L

	.globl	mcl_fpDbl_mod_NIST_P521L
	.p2align	2
	.type	mcl_fpDbl_mod_NIST_P521L,@function
mcl_fpDbl_mod_NIST_P521L:               // @mcl_fpDbl_mod_NIST_P521L
// BB#0:
	ldp	x8, x9, [x1, #112]
	ldr	x10, [x1, #128]
	ldp	x11, x12, [x1, #96]
	ldp	x13, x14, [x1, #80]
	ldp	x15, x16, [x1, #64]
	ldp	x17, x18, [x1, #48]
	ldp	x2, x3, [x1, #32]
	ldp	x4, x5, [x1, #16]
	ldp		x6, x1, [x1]
	extr	x7, x10, x9, #9
	extr	x9, x9, x8, #9
	extr	x8, x8, x12, #9
	extr	x12, x12, x11, #9
	extr	x11, x11, x14, #9
	extr	x14, x14, x13, #9
	extr	x13, x13, x16, #9
	extr	x16, x16, x15, #9
	adds		x16, x16, x6
	adcs	x13, x13, x1
	adcs	x14, x14, x4
	adcs	x11, x11, x5
	adcs	x12, x12, x2
	adcs	x1, x8, x3
	adcs	x17, x9, x17
	and	x15, x15, #0x1ff
	lsr	x10, x10, #9
	adcs	x18, x7, x18
	adcs	x2, x10, x15
	ubfx	x8, x2, #9, #1
	adds		x8, x8, x16
	adcs	x9, x13, xzr
	and		x13, x9, x8
	adcs	x10, x14, xzr
	and		x13, x13, x10
	adcs	x11, x11, xzr
	and		x13, x13, x11
	adcs	x12, x12, xzr
	and		x14, x13, x12
	adcs	x13, x1, xzr
	and		x15, x14, x13
	adcs	x14, x17, xzr
	and		x16, x15, x14
	adcs	x15, x18, xzr
	and		x17, x16, x15
	adcs	x16, x2, xzr
	orr	x18, x16, #0xfffffffffffffe00
	and		x17, x17, x18
	cmn		x17, #1         // =1
	b.eq	.LBB4_2
// BB#1:                                // %nonzero
	stp		x8, x9, [x0]
	and	x8, x16, #0x1ff
	stp	x10, x11, [x0, #16]
	stp	x12, x13, [x0, #32]
	stp	x14, x15, [x0, #48]
	str	x8, [x0, #64]
	ret
.LBB4_2:                                // %zero
	str	x30, [sp, #-16]!        // 8-byte Folded Spill
	mov	w2, #72
	mov	 w1, wzr
	bl	memset
	ldr	x30, [sp], #16          // 8-byte Folded Reload
	ret
.Lfunc_end4:
	.size	mcl_fpDbl_mod_NIST_P521L, .Lfunc_end4-mcl_fpDbl_mod_NIST_P521L

	.globl	mulPv192x64
	.p2align	2
	.type	mulPv192x64,@function
mulPv192x64:                            // @mulPv192x64
// BB#0:
	ldp	x9, x8, [x0, #8]
	ldr		x10, [x0]
	umulh	x11, x8, x1
	mul		x12, x8, x1
	umulh	x13, x9, x1
	mul		x8, x9, x1
	umulh	x9, x10, x1
	adds		x8, x9, x8
	adcs	x2, x13, x12
	adcs	x3, x11, xzr
	mul		x0, x10, x1
	mov	 x1, x8
	ret
.Lfunc_end5:
	.size	mulPv192x64, .Lfunc_end5-mulPv192x64

	.globl	mcl_fp_mulUnitPre3L
	.p2align	2
	.type	mcl_fp_mulUnitPre3L,@function
mcl_fp_mulUnitPre3L:                    // @mcl_fp_mulUnitPre3L
// BB#0:
	ldp		x8, x9, [x1]
	ldr	x10, [x1, #16]
	mul		x11, x8, x2
	mul		x12, x9, x2
	umulh	x8, x8, x2
	mul		x13, x10, x2
	umulh	x9, x9, x2
	adds		x8, x8, x12
	umulh	x10, x10, x2
	stp		x11, x8, [x0]
	adcs	x8, x9, x13
	adcs	x9, x10, xzr
	stp	x8, x9, [x0, #16]
	ret
.Lfunc_end6:
	.size	mcl_fp_mulUnitPre3L, .Lfunc_end6-mcl_fp_mulUnitPre3L

	.globl	mcl_fpDbl_mulPre3L
	.p2align	2
	.type	mcl_fpDbl_mulPre3L,@function
mcl_fpDbl_mulPre3L:                     // @mcl_fpDbl_mulPre3L
// BB#0:
	str	x19, [sp, #-16]!        // 8-byte Folded Spill
	ldp		x8, x9, [x1]
	ldp		x10, x12, [x2]
	ldr	x11, [x1, #16]
	ldr	x13, [x2, #16]
	mul		x14, x8, x10
	umulh	x15, x11, x10
	mul		x16, x11, x10
	umulh	x17, x9, x10
	mul		x18, x9, x10
	umulh	x10, x8, x10
	adds		x10, x10, x18
	mul		x1, x8, x12
	mul		x2, x11, x12
	mul		x3, x9, x12
	umulh	x4, x11, x12
	umulh	x5, x9, x12
	umulh	x12, x8, x12
	mul		x6, x8, x13
	mul		x7, x11, x13
	mul		x19, x9, x13
	umulh	x8, x8, x13
	umulh	x9, x9, x13
	umulh	x11, x11, x13
	adcs	x13, x17, x16
	adcs	x15, x15, xzr
	adds		x10, x1, x10
	stp		x14, x10, [x0]
	adcs	x10, x3, x13
	adcs	x13, x2, x15
	adcs	x14, xzr, xzr
	adds		x10, x10, x12
	adcs	x12, x13, x5
	adcs	x13, x14, x4
	adds		x10, x10, x6
	adcs	x12, x12, x19
	adcs	x13, x13, x7
	adcs	x14, xzr, xzr
	adds		x8, x12, x8
	stp	x10, x8, [x0, #16]
	adcs	x8, x13, x9
	adcs	x9, x14, x11
	stp	x8, x9, [x0, #32]
	ldr	x19, [sp], #16          // 8-byte Folded Reload
	ret
.Lfunc_end7:
	.size	mcl_fpDbl_mulPre3L, .Lfunc_end7-mcl_fpDbl_mulPre3L

	.globl	mcl_fpDbl_sqrPre3L
	.p2align	2
	.type	mcl_fpDbl_sqrPre3L,@function
mcl_fpDbl_sqrPre3L:                     // @mcl_fpDbl_sqrPre3L
// BB#0:
	ldp		x8, x10, [x1]
	ldr	x9, [x1, #16]
	mul		x11, x8, x8
	umulh	x12, x9, x8
	mul		x13, x9, x8
	umulh	x14, x10, x8
	mul		x15, x10, x8
	umulh	x8, x8, x8
	adds		x8, x8, x15
	adcs	x17, x14, x13
	adcs	x18, x12, xzr
	adds		x8, x8, x15
	mul		x15, x10, x10
	mul		x16, x9, x10
	stp		x11, x8, [x0]
	adcs	x11, x17, x15
	adcs	x15, x18, x16
	adcs	x17, xzr, xzr
	umulh	x8, x9, x10
	umulh	x10, x10, x10
	adds		x11, x11, x14
	adcs	x10, x15, x10
	adcs	x15, x17, x8
	adds		x11, x11, x13
	mul		x14, x9, x9
	adcs	x10, x10, x16
	adcs	x13, x15, x14
	adcs	x14, xzr, xzr
	adds		x10, x10, x12
	umulh	x9, x9, x9
	adcs	x8, x13, x8
	adcs	x9, x14, x9
	stp	x11, x10, [x0, #16]
	stp	x8, x9, [x0, #32]
	ret
.Lfunc_end8:
	.size	mcl_fpDbl_sqrPre3L, .Lfunc_end8-mcl_fpDbl_sqrPre3L

	.globl	mcl_fp_mont3L
	.p2align	2
	.type	mcl_fp_mont3L,@function
mcl_fp_mont3L:                          // @mcl_fp_mont3L
// BB#0:
	str	x23, [sp, #-48]!        // 8-byte Folded Spill
	stp	x22, x21, [sp, #16]     // 8-byte Folded Spill
	stp	x20, x19, [sp, #32]     // 8-byte Folded Spill
	ldp		x15, x16, [x2]
	ldp	x13, x14, [x1, #8]
	ldr		x12, [x1]
	ldp	x11, x10, [x3, #-8]
	ldp	x9, x8, [x3, #8]
	mul		x3, x13, x15
	umulh	x4, x12, x15
	ldr	x17, [x2, #16]
	umulh	x18, x14, x15
	mul		x1, x14, x15
	umulh	x2, x13, x15
	mul		x15, x12, x15
	adds		x3, x4, x3
	mul		x4, x15, x11
	adcs	x1, x2, x1
	mul		x22, x4, x9
	umulh	x23, x4, x10
	adcs	x18, x18, xzr
	mul		x2, x4, x8
	adds		x22, x23, x22
	umulh	x23, x4, x9
	adcs	x2, x23, x2
	umulh	x23, x4, x8
	mul		x4, x4, x10
	adcs	x23, x23, xzr
	cmn		x4, x15
	umulh	x5, x16, x14
	mul		x6, x16, x14
	umulh	x7, x16, x13
	mul		x19, x16, x13
	umulh	x20, x16, x12
	mul		x16, x16, x12
	umulh	x21, x17, x14
	mul		x14, x17, x14
	umulh	x15, x17, x13
	mul		x13, x17, x13
	umulh	x4, x17, x12
	mul		x12, x17, x12
	adcs	x17, x22, x3
	adcs	x1, x2, x1
	adcs	x18, x23, x18
	adcs	x2, xzr, xzr
	adds		x3, x20, x19
	adcs	x6, x7, x6
	adcs	x5, x5, xzr
	adds		x16, x17, x16
	adcs	x17, x1, x3
	adcs	x18, x18, x6
	mul		x1, x16, x11
	adcs	x2, x2, x5
	mul		x6, x1, x9
	umulh	x7, x1, x10
	adcs	x5, xzr, xzr
	mul		x3, x1, x8
	adds		x6, x7, x6
	umulh	x7, x1, x9
	adcs	x3, x7, x3
	umulh	x7, x1, x8
	mul		x1, x1, x10
	adcs	x7, x7, xzr
	cmn		x1, x16
	adcs	x16, x6, x17
	adcs	x17, x3, x18
	adcs	x18, x7, x2
	adcs	x1, x5, xzr
	adds		x13, x4, x13
	adcs	x14, x15, x14
	adcs	x15, x21, xzr
	adds		x12, x16, x12
	adcs	x13, x17, x13
	adcs	x14, x18, x14
	mul		x11, x12, x11
	adcs	x15, x1, x15
	mul		x2, x11, x9
	umulh	x3, x11, x10
	adcs	x1, xzr, xzr
	mul		x17, x11, x8
	umulh	x18, x11, x9
	adds		x2, x3, x2
	umulh	x16, x11, x8
	adcs	x17, x18, x17
	mul		x11, x11, x10
	adcs	x16, x16, xzr
	cmn		x11, x12
	adcs	x11, x2, x13
	adcs	x12, x17, x14
	adcs	x13, x16, x15
	adcs	x14, x1, xzr
	subs		x10, x11, x10
	sbcs	x9, x12, x9
	sbcs	x8, x13, x8
	sbcs	x14, x14, xzr
	tst	 x14, #0x1
	csel	x10, x11, x10, ne
	csel	x9, x12, x9, ne
	csel	x8, x13, x8, ne
	stp		x10, x9, [x0]
	str	x8, [x0, #16]
	ldp	x20, x19, [sp, #32]     // 8-byte Folded Reload
	ldp	x22, x21, [sp, #16]     // 8-byte Folded Reload
	ldr	x23, [sp], #48          // 8-byte Folded Reload
	ret
.Lfunc_end9:
	.size	mcl_fp_mont3L, .Lfunc_end9-mcl_fp_mont3L

	.globl	mcl_fp_montNF3L
	.p2align	2
	.type	mcl_fp_montNF3L,@function
mcl_fp_montNF3L:                        // @mcl_fp_montNF3L
// BB#0:
	str	x21, [sp, #-32]!        // 8-byte Folded Spill
	stp	x20, x19, [sp, #16]     // 8-byte Folded Spill
	ldp		x14, x16, [x2]
	ldp	x15, x13, [x1, #8]
	ldr		x12, [x1]
	ldp	x11, x10, [x3, #-8]
	ldp	x9, x8, [x3, #8]
	ldr	x17, [x2, #16]
	mul		x3, x15, x14
	umulh	x4, x12, x14
	umulh	x18, x13, x14
	mul		x1, x13, x14
	umulh	x2, x15, x14
	mul		x14, x12, x14
	adds		x3, x4, x3
	mul		x4, x14, x11
	adcs	x1, x2, x1
	mul		x2, x4, x10
	adcs	x18, x18, xzr
	umulh	x5, x16, x13
	mul		x6, x16, x13
	umulh	x7, x16, x15
	mul		x19, x16, x15
	umulh	x20, x16, x12
	mul		x16, x16, x12
	umulh	x21, x17, x13
	mul		x13, x17, x13
	cmn		x2, x14
	umulh	x14, x17, x15
	mul		x15, x17, x15
	umulh	x2, x17, x12
	mul		x12, x17, x12
	mul		x17, x4, x9
	adcs	x17, x17, x3
	mul		x3, x4, x8
	adcs	x1, x3, x1
	umulh	x3, x4, x10
	adcs	x18, x18, xzr
	adds		x17, x17, x3
	umulh	x3, x4, x9
	adcs	x1, x1, x3
	umulh	x3, x4, x8
	adcs	x18, x18, x3
	adds		x3, x20, x19
	adcs	x4, x7, x6
	adcs	x5, x5, xzr
	adds		x16, x16, x17
	adcs	x17, x3, x1
	mul		x1, x16, x11
	adcs	x18, x4, x18
	mul		x4, x1, x10
	adcs	x5, x5, xzr
	cmn		x4, x16
	mul		x16, x1, x9
	mul		x3, x1, x8
	adcs	x16, x16, x17
	adcs	x18, x3, x18
	umulh	x4, x1, x8
	umulh	x17, x1, x9
	umulh	x1, x1, x10
	adcs	x3, x5, xzr
	adds		x16, x16, x1
	adcs	x17, x18, x17
	adcs	x18, x3, x4
	adds		x15, x2, x15
	adcs	x13, x14, x13
	adcs	x14, x21, xzr
	adds		x12, x12, x16
	adcs	x15, x15, x17
	mul		x11, x12, x11
	adcs	x13, x13, x18
	mul		x18, x11, x10
	adcs	x14, x14, xzr
	mul		x17, x11, x9
	cmn		x18, x12
	mul		x16, x11, x8
	adcs	x12, x17, x15
	adcs	x13, x16, x13
	umulh	x1, x11, x8
	umulh	x2, x11, x9
	umulh	x11, x11, x10
	adcs	x14, x14, xzr
	adds		x11, x12, x11
	adcs	x12, x13, x2
	adcs	x13, x14, x1
	subs		x10, x11, x10
	sbcs	x9, x12, x9
	sbcs	x8, x13, x8
	asr	x14, x8, #63
	cmp		x14, #0         // =0
	csel	x10, x11, x10, lt
	csel	x9, x12, x9, lt
	csel	x8, x13, x8, lt
	stp		x10, x9, [x0]
	str	x8, [x0, #16]
	ldp	x20, x19, [sp, #16]     // 8-byte Folded Reload
	ldr	x21, [sp], #32          // 8-byte Folded Reload
	ret
.Lfunc_end10:
	.size	mcl_fp_montNF3L, .Lfunc_end10-mcl_fp_montNF3L

	.globl	mcl_fp_montRed3L
	.p2align	2
	.type	mcl_fp_montRed3L,@function
mcl_fp_montRed3L:                       // @mcl_fp_montRed3L
// BB#0:
	ldp	x9, x10, [x2, #-8]
	ldp		x14, x15, [x1]
	ldp	x11, x8, [x2, #8]
	ldp	x12, x13, [x1, #16]
	ldp	x16, x17, [x1, #32]
	mul		x18, x14, x9
	mul		x3, x18, x11
	umulh	x4, x18, x10
	mul		x2, x18, x8
	adds		x3, x4, x3
	umulh	x4, x18, x11
	umulh	x1, x18, x8
	adcs	x2, x4, x2
	mul		x18, x18, x10
	adcs	x1, x1, xzr
	cmn		x18, x14
	adcs	x14, x3, x15
	adcs	x12, x2, x12
	mul		x15, x14, x9
	adcs	x13, x1, x13
	umulh	x2, x15, x10
	mul		x3, x15, x11
	adcs	x4, xzr, xzr
	umulh	x1, x15, x11
	adds		x2, x3, x2
	mul		x3, x15, x8
	umulh	x18, x15, x8
	adcs	x1, x3, x1
	mul		x15, x15, x10
	adcs	x18, x4, x18
	cmn		x15, x14
	adcs	x12, x2, x12
	adcs	x13, x1, x13
	mul		x9, x12, x9
	adcs	x15, x18, x16
	umulh	x1, x9, x10
	mul		x2, x9, x11
	adcs	x3, xzr, xzr
	umulh	x16, x9, x11
	mul		x18, x9, x8
	adds		x1, x2, x1
	umulh	x14, x9, x8
	adcs	x16, x18, x16
	mul		x9, x9, x10
	adcs	x14, x3, x14
	cmn		x9, x12
	adcs	x9, x1, x13
	adcs	x12, x16, x15
	adcs	x13, x14, x17
	subs		x10, x9, x10
	sbcs	x11, x12, x11
	sbcs	x8, x13, x8
	ngcs	 x14, xzr
	tst	 x14, #0x1
	csel	x9, x9, x10, ne
	csel	x10, x12, x11, ne
	csel	x8, x13, x8, ne
	stp		x9, x10, [x0]
	str	x8, [x0, #16]
	ret
.Lfunc_end11:
	.size	mcl_fp_montRed3L, .Lfunc_end11-mcl_fp_montRed3L

	.globl	mcl_fp_montRedNF3L
	.p2align	2
	.type	mcl_fp_montRedNF3L,@function
mcl_fp_montRedNF3L:                     // @mcl_fp_montRedNF3L
// BB#0:
	ldp	x9, x10, [x2, #-8]
	ldp		x14, x15, [x1]
	ldp	x11, x8, [x2, #8]
	ldp	x12, x13, [x1, #16]
	ldp	x16, x17, [x1, #32]
	mul		x18, x14, x9
	mul		x3, x18, x11
	umulh	x4, x18, x10
	mul		x2, x18, x8
	adds		x3, x4, x3
	umulh	x4, x18, x11
	umulh	x1, x18, x8
	adcs	x2, x4, x2
	mul		x18, x18, x10
	adcs	x1, x1, xzr
	cmn		x18, x14
	adcs	x14, x3, x15
	adcs	x12, x2, x12
	mul		x15, x14, x9
	adcs	x13, x1, x13
	umulh	x2, x15, x10
	mul		x3, x15, x11
	adcs	x4, xzr, xzr
	umulh	x1, x15, x11
	adds		x2, x3, x2
	mul		x3, x15, x8
	umulh	x18, x15, x8
	adcs	x1, x3, x1
	mul		x15, x15, x10
	adcs	x18, x4, x18
	cmn		x15, x14
	adcs	x12, x2, x12
	adcs	x13, x1, x13
	mul		x9, x12, x9
	adcs	x15, x18, x16
	umulh	x1, x9, x10
	mul		x2, x9, x11
	adcs	x3, xzr, xzr
	umulh	x16, x9, x11
	mul		x18, x9, x8
	adds		x1, x2, x1
	umulh	x14, x9, x8
	adcs	x16, x18, x16
	mul		x9, x9, x10
	adcs	x14, x3, x14
	cmn		x9, x12
	adcs	x9, x1, x13
	adcs	x12, x16, x15
	adcs	x13, x14, x17
	subs		x10, x9, x10
	sbcs	x11, x12, x11
	sbcs	x8, x13, x8
	asr	x14, x8, #63
	cmp		x14, #0         // =0
	csel	x9, x9, x10, lt
	csel	x10, x12, x11, lt
	csel	x8, x13, x8, lt
	stp		x9, x10, [x0]
	str	x8, [x0, #16]
	ret
.Lfunc_end12:
	.size	mcl_fp_montRedNF3L, .Lfunc_end12-mcl_fp_montRedNF3L

	.globl	mcl_fp_addPre3L
	.p2align	2
	.type	mcl_fp_addPre3L,@function
mcl_fp_addPre3L:                        // @mcl_fp_addPre3L
// BB#0:
	ldp		x8, x9, [x2]
	ldp		x10, x11, [x1]
	ldr	x12, [x2, #16]
	ldr	x13, [x1, #16]
	adds		x8, x8, x10
	adcs	x9, x9, x11
	stp		x8, x9, [x0]
	adcs	x9, x12, x13
	adcs	x8, xzr, xzr
	str	x9, [x0, #16]
	mov	 x0, x8
	ret
.Lfunc_end13:
	.size	mcl_fp_addPre3L, .Lfunc_end13-mcl_fp_addPre3L

	.globl	mcl_fp_subPre3L
	.p2align	2
	.type	mcl_fp_subPre3L,@function
mcl_fp_subPre3L:                        // @mcl_fp_subPre3L
// BB#0:
	ldp		x8, x9, [x2]
	ldp		x10, x11, [x1]
	ldr	x12, [x2, #16]
	ldr	x13, [x1, #16]
	subs		x8, x10, x8
	sbcs	x9, x11, x9
	stp		x8, x9, [x0]
	sbcs	x9, x13, x12
	ngcs	 x8, xzr
	and	x8, x8, #0x1
	str	x9, [x0, #16]
	mov	 x0, x8
	ret
.Lfunc_end14:
	.size	mcl_fp_subPre3L, .Lfunc_end14-mcl_fp_subPre3L

	.globl	mcl_fp_shr1_3L
	.p2align	2
	.type	mcl_fp_shr1_3L,@function
mcl_fp_shr1_3L:                         // @mcl_fp_shr1_3L
// BB#0:
	ldp		x8, x9, [x1]
	ldr	x10, [x1, #16]
	extr	x8, x9, x8, #1
	extr	x9, x10, x9, #1
	lsr	x10, x10, #1
	stp		x8, x9, [x0]
	str	x10, [x0, #16]
	ret
.Lfunc_end15:
	.size	mcl_fp_shr1_3L, .Lfunc_end15-mcl_fp_shr1_3L

	.globl	mcl_fp_add3L
	.p2align	2
	.type	mcl_fp_add3L,@function
mcl_fp_add3L:                           // @mcl_fp_add3L
// BB#0:
	ldp		x8, x9, [x2]
	ldp		x10, x11, [x1]
	ldr	x12, [x2, #16]
	ldr	x13, [x1, #16]
	adds		x8, x8, x10
	adcs	x9, x9, x11
	ldp		x10, x11, [x3]
	adcs	x12, x12, x13
	ldr	x13, [x3, #16]
	adcs	x14, xzr, xzr
	subs		x10, x8, x10
	stp		x8, x9, [x0]
	sbcs	x9, x9, x11
	sbcs	x8, x12, x13
	sbcs	x11, x14, xzr
	str	x12, [x0, #16]
	tbnz	w11, #0, .LBB16_2
// BB#1:                                // %nocarry
	stp		x10, x9, [x0]
	str	x8, [x0, #16]
.LBB16_2:                               // %carry
	ret
.Lfunc_end16:
	.size	mcl_fp_add3L, .Lfunc_end16-mcl_fp_add3L

	.globl	mcl_fp_addNF3L
	.p2align	2
	.type	mcl_fp_addNF3L,@function
mcl_fp_addNF3L:                         // @mcl_fp_addNF3L
// BB#0:
	ldp		x8, x9, [x1]
	ldp		x10, x11, [x2]
	ldr	x12, [x1, #16]
	ldr	x13, [x2, #16]
	ldr	x14, [x3, #16]
	adds		x8, x10, x8
	adcs	x9, x11, x9
	ldp		x10, x11, [x3]
	adcs	x12, x13, x12
	subs		x10, x8, x10
	sbcs	x11, x9, x11
	sbcs	x13, x12, x14
	asr	x14, x13, #63
	cmp		x14, #0         // =0
	csel	x8, x8, x10, lt
	csel	x9, x9, x11, lt
	csel	x10, x12, x13, lt
	stp		x8, x9, [x0]
	str	x10, [x0, #16]
	ret
.Lfunc_end17:
	.size	mcl_fp_addNF3L, .Lfunc_end17-mcl_fp_addNF3L

	.globl	mcl_fp_sub3L
	.p2align	2
	.type	mcl_fp_sub3L,@function
mcl_fp_sub3L:                           // @mcl_fp_sub3L
// BB#0:
	ldp		x8, x9, [x2]
	ldp		x10, x11, [x1]
	ldr	x12, [x2, #16]
	ldr	x13, [x1, #16]
	subs		x10, x10, x8
	sbcs	x8, x11, x9
	sbcs	x9, x13, x12
	ngcs	 x11, xzr
	stp		x10, x8, [x0]
	str	x9, [x0, #16]
	tbnz	w11, #0, .LBB18_2
// BB#1:                                // %nocarry
	ret
.LBB18_2:                               // %carry
	ldp		x11, x12, [x3]
	ldr	x13, [x3, #16]
	adds		x10, x11, x10
	adcs	x8, x12, x8
	stp		x10, x8, [x0]
	adcs	x8, x13, x9
	str	x8, [x0, #16]
	ret
.Lfunc_end18:
	.size	mcl_fp_sub3L, .Lfunc_end18-mcl_fp_sub3L

	.globl	mcl_fp_subNF3L
	.p2align	2
	.type	mcl_fp_subNF3L,@function
mcl_fp_subNF3L:                         // @mcl_fp_subNF3L
// BB#0:
	ldp		x8, x9, [x2]
	ldp		x10, x11, [x1]
	ldr	x12, [x2, #16]
	ldr	x13, [x1, #16]
	ldr	x14, [x3, #16]
	subs		x8, x10, x8
	sbcs	x9, x11, x9
	ldp		x10, x11, [x3]
	sbcs	x12, x13, x12
	asr	x13, x12, #63
	and		x14, x13, x14
	and		x11, x13, x11
	extr	x13, x13, x12, #63
	and		x10, x13, x10
	adds		x8, x10, x8
	adcs	x9, x11, x9
	stp		x8, x9, [x0]
	adcs	x8, x14, x12
	str	x8, [x0, #16]
	ret
.Lfunc_end19:
	.size	mcl_fp_subNF3L, .Lfunc_end19-mcl_fp_subNF3L

	.globl	mcl_fpDbl_add3L
	.p2align	2
	.type	mcl_fpDbl_add3L,@function
mcl_fpDbl_add3L:                        // @mcl_fpDbl_add3L
// BB#0:
	ldp		x14, x15, [x2]
	ldp		x16, x17, [x1]
	ldp	x10, x11, [x1, #32]
	ldp	x12, x13, [x2, #16]
	ldp	x18, x1, [x1, #16]
	ldp	x8, x9, [x2, #32]
	adds		x14, x14, x16
	adcs	x15, x15, x17
	adcs	x12, x12, x18
	ldr		x17, [x3]
	adcs	x13, x13, x1
	ldp	x2, x16, [x3, #8]
	adcs	x8, x8, x10
	adcs	x9, x9, x11
	adcs	x10, xzr, xzr
	subs		x11, x13, x17
	stp		x14, x15, [x0]
	sbcs	x14, x8, x2
	sbcs	x15, x9, x16
	sbcs	x10, x10, xzr
	tst	 x10, #0x1
	csel	x10, x13, x11, ne
	csel	x8, x8, x14, ne
	csel	x9, x9, x15, ne
	stp	x12, x10, [x0, #16]
	stp	x8, x9, [x0, #32]
	ret
.Lfunc_end20:
	.size	mcl_fpDbl_add3L, .Lfunc_end20-mcl_fpDbl_add3L

	.globl	mcl_fpDbl_sub3L
	.p2align	2
	.type	mcl_fpDbl_sub3L,@function
mcl_fpDbl_sub3L:                        // @mcl_fpDbl_sub3L
// BB#0:
	ldp		x14, x15, [x2]
	ldp		x16, x17, [x1]
	ldp	x10, x11, [x1, #32]
	ldp	x12, x13, [x2, #16]
	ldp	x18, x1, [x1, #16]
	ldp	x8, x9, [x2, #32]
	subs		x14, x16, x14
	sbcs	x15, x17, x15
	sbcs	x12, x18, x12
	sbcs	x13, x1, x13
	ldr		x17, [x3]
	sbcs	x8, x10, x8
	ldp	x2, x16, [x3, #8]
	sbcs	x9, x11, x9
	ngcs	 x10, xzr
	tst	 x10, #0x1
	stp		x14, x15, [x0]
	csel	x14, x17, xzr, ne
	csel	x10, x16, xzr, ne
	csel	x11, x2, xzr, ne
	adds		x13, x14, x13
	adcs	x8, x11, x8
	adcs	x9, x10, x9
	stp	x12, x13, [x0, #16]
	stp	x8, x9, [x0, #32]
	ret
.Lfunc_end21:
	.size	mcl_fpDbl_sub3L, .Lfunc_end21-mcl_fpDbl_sub3L

	.globl	mulPv256x64
	.p2align	2
	.type	mulPv256x64,@function
mulPv256x64:                            // @mulPv256x64
// BB#0:
	ldp	x9, x8, [x0, #16]
	ldp		x10, x11, [x0]
	umulh	x12, x8, x1
	mul		x13, x8, x1
	umulh	x15, x11, x1
	mul		x8, x11, x1
	umulh	x11, x10, x1
	umulh	x14, x9, x1
	mul		x9, x9, x1
	adds		x8, x11, x8
	adcs	x2, x15, x9
	adcs	x3, x14, x13
	adcs	x4, x12, xzr
	mul		x0, x10, x1
	mov	 x1, x8
	ret
.Lfunc_end22:
	.size	mulPv256x64, .Lfunc_end22-mulPv256x64

	.globl	mcl_fp_mulUnitPre4L
	.p2align	2
	.type	mcl_fp_mulUnitPre4L,@function
mcl_fp_mulUnitPre4L:                    // @mcl_fp_mulUnitPre4L
// BB#0:
	ldp		x8, x9, [x1]
	ldp	x10, x11, [x1, #16]
	mul		x12, x8, x2
	mul		x13, x9, x2
	umulh	x8, x8, x2
	mul		x14, x10, x2
	umulh	x9, x9, x2
	adds		x8, x8, x13
	mul		x15, x11, x2
	umulh	x10, x10, x2
	stp		x12, x8, [x0]
	adcs	x8, x9, x14
	umulh	x11, x11, x2
	adcs	x9, x10, x15
	stp	x8, x9, [x0, #16]
	adcs	x8, x11, xzr
	str	x8, [x0, #32]
	ret
.Lfunc_end23:
	.size	mcl_fp_mulUnitPre4L, .Lfunc_end23-mcl_fp_mulUnitPre4L

	.globl	mcl_fpDbl_mulPre4L
	.p2align	2
	.type	mcl_fpDbl_mulPre4L,@function
mcl_fpDbl_mulPre4L:                     // @mcl_fpDbl_mulPre4L
// BB#0:
	sub	sp, sp, #128            // =128
	ldp		x8, x10, [x1]
	ldp		x15, x16, [x2]
	ldp	x12, x14, [x1, #16]
	ldp	x17, x18, [x2, #16]
	ldp		x9, x11, [x1]
	ldp	x13, x1, [x1, #16]
	mul		x2, x8, x15
	stp	x20, x19, [sp, #96]     // 8-byte Folded Spill
	str	x2, [sp, #24]           // 8-byte Folded Spill
	umulh	x2, x14, x15
	mul		x4, x14, x15
	umulh	x5, x12, x15
	mul		x6, x12, x15
	umulh	x7, x10, x15
	mul		x19, x10, x15
	umulh	x15, x8, x15
	stp	x28, x27, [sp, #32]     // 8-byte Folded Spill
	stp	x24, x23, [sp, #64]     // 8-byte Folded Spill
	stp	x22, x21, [sp, #80]     // 8-byte Folded Spill
	mul		x21, x14, x16
	umulh	x24, x14, x16
	mul		x28, x14, x17
	umulh	x14, x14, x17
	adds		x15, x15, x19
	stp	x26, x25, [sp, #48]     // 8-byte Folded Spill
	stp	x29, x30, [sp, #112]    // 8-byte Folded Spill
	mul		x20, x8, x16
	mul		x22, x12, x16
	mul		x23, x10, x16
	umulh	x25, x12, x16
	umulh	x26, x10, x16
	umulh	x16, x8, x16
	mul		x27, x8, x17
	mul		x29, x12, x17
	mul		x30, x10, x17
	stp	x2, x14, [sp, #8]       // 8-byte Folded Spill
	umulh	x3, x12, x17
	umulh	x2, x10, x17
	umulh	x14, x8, x17
	mul		x17, x9, x18
	umulh	x12, x9, x18
	mul		x10, x11, x18
	umulh	x11, x11, x18
	mul		x9, x13, x18
	umulh	x13, x13, x18
	mul		x8, x1, x18
	umulh	x18, x1, x18
	adcs	x1, x7, x6
	adcs	x4, x5, x4
	ldr	x5, [sp, #8]            // 8-byte Folded Reload
	ldr	x6, [sp, #24]           // 8-byte Folded Reload
	adcs	x5, x5, xzr
	adds		x15, x20, x15
	stp		x6, x15, [x0]
	adcs	x15, x23, x1
	adcs	x1, x22, x4
	adcs	x4, x21, x5
	adcs	x5, xzr, xzr
	adds		x15, x15, x16
	adcs	x16, x1, x26
	adcs	x1, x4, x25
	adcs	x4, x5, x24
	adds		x15, x27, x15
	adcs	x16, x30, x16
	adcs	x1, x29, x1
	adcs	x4, x28, x4
	adcs	x5, xzr, xzr
	adds		x14, x16, x14
	adcs	x16, x1, x2
	ldr	x2, [sp, #16]           // 8-byte Folded Reload
	adcs	x1, x4, x3
	ldp	x29, x30, [sp, #112]    // 8-byte Folded Reload
	ldp	x20, x19, [sp, #96]     // 8-byte Folded Reload
	adcs	x2, x5, x2
	adds		x14, x17, x14
	adcs	x10, x10, x16
	adcs	x9, x9, x1
	adcs	x8, x8, x2
	stp	x15, x14, [x0, #16]
	adcs	x14, xzr, xzr
	adds		x10, x10, x12
	adcs	x9, x9, x11
	ldp	x22, x21, [sp, #80]     // 8-byte Folded Reload
	ldp	x24, x23, [sp, #64]     // 8-byte Folded Reload
	ldp	x26, x25, [sp, #48]     // 8-byte Folded Reload
	ldp	x28, x27, [sp, #32]     // 8-byte Folded Reload
	adcs	x8, x8, x13
	stp	x10, x9, [x0, #32]
	adcs	x9, x14, x18
	stp	x8, x9, [x0, #48]
	add	sp, sp, #128            // =128
	ret
.Lfunc_end24:
	.size	mcl_fpDbl_mulPre4L, .Lfunc_end24-mcl_fpDbl_mulPre4L

	.globl	mcl_fpDbl_sqrPre4L
	.p2align	2
	.type	mcl_fpDbl_sqrPre4L,@function
mcl_fpDbl_sqrPre4L:                     // @mcl_fpDbl_sqrPre4L
// BB#0:
	ldp		x8, x9, [x1]
	ldp		x10, x13, [x1]
	ldp	x11, x12, [x1, #16]
	ldr	x14, [x1, #16]
	ldr	x1, [x1, #24]
	mul		x15, x10, x10
	umulh	x16, x12, x10
	mul		x17, x12, x10
	umulh	x18, x14, x10
	mul		x2, x14, x10
	umulh	x3, x9, x10
	mul		x4, x9, x10
	umulh	x10, x10, x10
	adds		x10, x10, x4
	adcs	x5, x3, x2
	adcs	x17, x18, x17
	adcs	x16, x16, xzr
	adds		x10, x4, x10
	stp		x15, x10, [x0]
	mul		x10, x9, x9
	adcs	x10, x10, x5
	mul		x15, x14, x9
	mul		x4, x12, x9
	adcs	x17, x15, x17
	adcs	x16, x4, x16
	adcs	x4, xzr, xzr
	adds		x10, x10, x3
	umulh	x3, x9, x9
	adcs	x17, x17, x3
	umulh	x3, x12, x9
	umulh	x9, x14, x9
	adcs	x16, x16, x9
	adcs	x3, x4, x3
	adds		x10, x2, x10
	adcs	x15, x15, x17
	mul		x17, x14, x14
	mul		x2, x12, x14
	adcs	x16, x17, x16
	adcs	x2, x2, x3
	adcs	x3, xzr, xzr
	adds		x15, x15, x18
	umulh	x12, x12, x14
	umulh	x14, x14, x14
	adcs	x9, x16, x9
	adcs	x14, x2, x14
	mul		x17, x8, x1
	adcs	x12, x3, x12
	mul		x16, x13, x1
	adds		x15, x17, x15
	mul		x18, x11, x1
	adcs	x9, x16, x9
	mul		x2, x1, x1
	stp	x10, x15, [x0, #16]
	adcs	x10, x18, x14
	adcs	x12, x2, x12
	umulh	x8, x8, x1
	adcs	x14, xzr, xzr
	umulh	x13, x13, x1
	adds		x8, x9, x8
	umulh	x11, x11, x1
	adcs	x9, x10, x13
	umulh	x1, x1, x1
	stp	x8, x9, [x0, #32]
	adcs	x8, x12, x11
	adcs	x9, x14, x1
	stp	x8, x9, [x0, #48]
	ret
.Lfunc_end25:
	.size	mcl_fpDbl_sqrPre4L, .Lfunc_end25-mcl_fpDbl_sqrPre4L

	.globl	mcl_fp_mont4L
	.p2align	2
	.type	mcl_fp_mont4L,@function
mcl_fp_mont4L:                          // @mcl_fp_mont4L
// BB#0:
	sub	sp, sp, #112            // =112
	stp	x28, x27, [sp, #16]     // 8-byte Folded Spill
	stp	x26, x25, [sp, #32]     // 8-byte Folded Spill
	stp	x24, x23, [sp, #48]     // 8-byte Folded Spill
	stp	x22, x21, [sp, #64]     // 8-byte Folded Spill
	stp	x20, x19, [sp, #80]     // 8-byte Folded Spill
	stp	x29, x30, [sp, #96]     // 8-byte Folded Spill
	str	x0, [sp, #8]            // 8-byte Folded Spill
	ldp		x14, x15, [x1]
	ldp		x17, x18, [x2]
	ldp	x13, x16, [x1, #16]
	ldp	x0, x11, [x3, #-8]
	ldr	x10, [x3, #8]
	mul		x19, x15, x17
	umulh	x20, x14, x17
	ldp	x9, x8, [x3, #16]
	mul		x6, x13, x17
	umulh	x7, x15, x17
	adds		x19, x20, x19
	umulh	x3, x16, x17
	mul		x4, x16, x17
	umulh	x5, x13, x17
	mul		x17, x14, x17
	adcs	x6, x7, x6
	mul		x20, x17, x0
	adcs	x4, x5, x4
	mul		x30, x20, x10
	umulh	x5, x20, x11
	adcs	x3, x3, xzr
	mul		x29, x20, x9
	adds		x5, x5, x30
	umulh	x30, x20, x10
	mul		x7, x20, x8
	adcs	x29, x30, x29
	umulh	x30, x20, x9
	adcs	x7, x30, x7
	umulh	x30, x20, x8
	mul		x20, x20, x11
	adcs	x30, x30, xzr
	cmn		x20, x17
	adcs	x5, x5, x19
	adcs	x6, x29, x6
	adcs	x4, x7, x4
	adcs	x3, x30, x3
	mul		x26, x18, x15
	umulh	x27, x18, x14
	adcs	x30, xzr, xzr
	mul		x24, x18, x13
	umulh	x25, x18, x15
	adds		x26, x27, x26
	mul		x22, x18, x16
	umulh	x23, x18, x13
	adcs	x24, x25, x24
	umulh	x21, x18, x16
	adcs	x22, x23, x22
	mul		x18, x18, x14
	adcs	x21, x21, xzr
	adds		x18, x5, x18
	adcs	x5, x6, x26
	adcs	x4, x4, x24
	adcs	x3, x3, x22
	mul		x6, x18, x0
	adcs	x21, x30, x21
	mul		x26, x6, x10
	umulh	x22, x6, x11
	adcs	x30, xzr, xzr
	mul		x24, x6, x9
	adds		x22, x22, x26
	umulh	x26, x6, x10
	ldp	x1, x2, [x2, #16]
	mul		x23, x6, x8
	adcs	x24, x26, x24
	umulh	x26, x6, x9
	adcs	x23, x26, x23
	umulh	x26, x6, x8
	mul		x6, x6, x11
	adcs	x26, x26, xzr
	cmn		x6, x18
	umulh	x28, x1, x16
	mul		x17, x1, x16
	umulh	x20, x1, x13
	mul		x19, x1, x13
	umulh	x29, x1, x15
	mul		x7, x1, x15
	umulh	x27, x1, x14
	mul		x1, x1, x14
	umulh	x25, x2, x16
	mul		x16, x2, x16
	umulh	x18, x2, x13
	mul		x13, x2, x13
	umulh	x6, x2, x15
	mul		x15, x2, x15
	umulh	x12, x2, x14
	mul		x14, x2, x14
	adcs	x2, x22, x5
	adcs	x4, x24, x4
	adcs	x3, x23, x3
	adcs	x5, x26, x21
	adcs	x21, x30, xzr
	adds		x7, x27, x7
	adcs	x19, x29, x19
	adcs	x17, x20, x17
	adcs	x20, x28, xzr
	adds		x1, x2, x1
	adcs	x2, x4, x7
	adcs	x3, x3, x19
	adcs	x17, x5, x17
	mul		x4, x1, x0
	adcs	x20, x21, x20
	mul		x22, x4, x10
	umulh	x5, x4, x11
	adcs	x21, xzr, xzr
	mul		x19, x4, x9
	adds		x5, x5, x22
	umulh	x22, x4, x10
	mul		x7, x4, x8
	adcs	x19, x22, x19
	umulh	x22, x4, x9
	adcs	x7, x22, x7
	umulh	x22, x4, x8
	mul		x4, x4, x11
	adcs	x22, x22, xzr
	cmn		x4, x1
	adcs	x1, x5, x2
	adcs	x2, x19, x3
	adcs	x17, x7, x17
	adcs	x3, x22, x20
	adcs	x4, x21, xzr
	adds		x12, x12, x15
	adcs	x13, x6, x13
	adcs	x15, x18, x16
	adcs	x16, x25, xzr
	adds		x14, x1, x14
	adcs	x12, x2, x12
	adcs	x13, x17, x13
	adcs	x15, x3, x15
	mul		x18, x14, x0
	adcs	x16, x4, x16
	mul		x6, x18, x10
	umulh	x7, x18, x11
	adcs	x3, xzr, xzr
	mul		x2, x18, x9
	umulh	x5, x18, x10
	adds		x4, x7, x6
	mul		x0, x18, x8
	umulh	x1, x18, x9
	adcs	x2, x5, x2
	umulh	x17, x18, x8
	adcs	x0, x1, x0
	mul		x18, x18, x11
	adcs	x17, x17, xzr
	cmn		x18, x14
	adcs	x12, x4, x12
	adcs	x13, x2, x13
	adcs	x14, x0, x15
	adcs	x15, x17, x16
	adcs	x16, x3, xzr
	subs		x11, x12, x11
	sbcs	x10, x13, x10
	sbcs	x9, x14, x9
	sbcs	x8, x15, x8
	sbcs	x16, x16, xzr
	tst	 x16, #0x1
	csel	x11, x12, x11, ne
	ldr	x12, [sp, #8]           // 8-byte Folded Reload
	csel	x10, x13, x10, ne
	csel	x9, x14, x9, ne
	csel	x8, x15, x8, ne
	stp		x11, x10, [x12]
	stp	x9, x8, [x12, #16]
	ldp	x29, x30, [sp, #96]     // 8-byte Folded Reload
	ldp	x20, x19, [sp, #80]     // 8-byte Folded Reload
	ldp	x22, x21, [sp, #64]     // 8-byte Folded Reload
	ldp	x24, x23, [sp, #48]     // 8-byte Folded Reload
	ldp	x26, x25, [sp, #32]     // 8-byte Folded Reload
	ldp	x28, x27, [sp, #16]     // 8-byte Folded Reload
	add	sp, sp, #112            // =112
	ret
.Lfunc_end26:
	.size	mcl_fp_mont4L, .Lfunc_end26-mcl_fp_mont4L

	.globl	mcl_fp_montNF4L
	.p2align	2
	.type	mcl_fp_montNF4L,@function
mcl_fp_montNF4L:                        // @mcl_fp_montNF4L
// BB#0:
	str	x27, [sp, #-80]!        // 8-byte Folded Spill
	stp	x26, x25, [sp, #16]     // 8-byte Folded Spill
	stp	x24, x23, [sp, #32]     // 8-byte Folded Spill
	stp	x22, x21, [sp, #48]     // 8-byte Folded Spill
	stp	x20, x19, [sp, #64]     // 8-byte Folded Spill
	ldp		x13, x16, [x1]
	ldp		x17, x18, [x2]
	ldp	x14, x15, [x1, #16]
	ldp	x12, x11, [x3, #-8]
	ldr	x10, [x3, #8]
	mul		x19, x16, x17
	umulh	x20, x13, x17
	mul		x6, x14, x17
	umulh	x7, x16, x17
	adds		x19, x20, x19
	ldp	x9, x8, [x3, #16]
	umulh	x3, x15, x17
	mul		x4, x15, x17
	umulh	x5, x14, x17
	mul		x17, x13, x17
	adcs	x6, x7, x6
	mul		x7, x17, x12
	adcs	x4, x5, x4
	mul		x5, x7, x11
	adcs	x3, x3, xzr
	cmn		x5, x17
	mul		x5, x7, x10
	adcs	x5, x5, x19
	mul		x19, x7, x9
	adcs	x6, x19, x6
	mul		x19, x7, x8
	adcs	x4, x19, x4
	umulh	x19, x7, x11
	adcs	x3, x3, xzr
	adds		x5, x5, x19
	umulh	x19, x7, x10
	adcs	x6, x6, x19
	umulh	x19, x7, x9
	adcs	x4, x4, x19
	umulh	x7, x7, x8
	mul		x26, x18, x16
	umulh	x27, x18, x13
	adcs	x3, x3, x7
	mul		x24, x18, x14
	umulh	x25, x18, x16
	adds		x26, x27, x26
	mul		x22, x18, x15
	umulh	x23, x18, x14
	adcs	x24, x25, x24
	umulh	x21, x18, x15
	adcs	x22, x23, x22
	mul		x18, x18, x13
	adcs	x21, x21, xzr
	adds		x18, x18, x5
	ldp	x1, x2, [x2, #16]
	adcs	x6, x26, x6
	adcs	x4, x24, x4
	mul		x24, x18, x12
	adcs	x3, x22, x3
	mul		x22, x24, x11
	adcs	x21, x21, xzr
	umulh	x20, x1, x15
	mul		x17, x1, x15
	umulh	x19, x1, x14
	mul		x7, x1, x14
	umulh	x27, x1, x16
	mul		x25, x1, x16
	umulh	x23, x1, x13
	mul		x1, x1, x13
	umulh	x5, x2, x15
	mul		x15, x2, x15
	umulh	x26, x2, x14
	mul		x14, x2, x14
	cmn		x22, x18
	umulh	x18, x2, x16
	mul		x16, x2, x16
	umulh	x22, x2, x13
	mul		x13, x2, x13
	mul		x2, x24, x10
	adcs	x2, x2, x6
	mul		x6, x24, x9
	adcs	x4, x6, x4
	mul		x6, x24, x8
	adcs	x3, x6, x3
	umulh	x6, x24, x11
	adcs	x21, x21, xzr
	adds		x2, x2, x6
	umulh	x6, x24, x10
	adcs	x4, x4, x6
	umulh	x6, x24, x9
	adcs	x3, x3, x6
	umulh	x6, x24, x8
	adcs	x6, x21, x6
	adds		x21, x23, x25
	adcs	x7, x27, x7
	adcs	x17, x19, x17
	adcs	x19, x20, xzr
	adds		x1, x1, x2
	adcs	x2, x21, x4
	adcs	x3, x7, x3
	mul		x4, x1, x12
	adcs	x17, x17, x6
	mul		x6, x4, x11
	adcs	x19, x19, xzr
	cmn		x6, x1
	mul		x1, x4, x10
	mul		x20, x4, x9
	adcs	x1, x1, x2
	mul		x7, x4, x8
	adcs	x3, x20, x3
	adcs	x17, x7, x17
	umulh	x6, x4, x8
	umulh	x2, x4, x9
	umulh	x20, x4, x10
	umulh	x4, x4, x11
	adcs	x7, x19, xzr
	adds		x1, x1, x4
	adcs	x3, x3, x20
	adcs	x17, x17, x2
	adcs	x2, x7, x6
	adds		x16, x22, x16
	adcs	x14, x18, x14
	adcs	x15, x26, x15
	adcs	x18, x5, xzr
	adds		x13, x13, x1
	adcs	x16, x16, x3
	adcs	x14, x14, x17
	mul		x12, x13, x12
	adcs	x15, x15, x2
	mul		x4, x12, x11
	adcs	x18, x18, xzr
	mul		x3, x12, x10
	cmn		x4, x13
	mul		x1, x12, x9
	adcs	x13, x3, x16
	mul		x17, x12, x8
	adcs	x14, x1, x14
	adcs	x15, x17, x15
	umulh	x5, x12, x8
	umulh	x6, x12, x9
	umulh	x7, x12, x10
	umulh	x12, x12, x11
	adcs	x16, x18, xzr
	adds		x12, x13, x12
	adcs	x13, x14, x7
	adcs	x14, x15, x6
	adcs	x15, x16, x5
	subs		x11, x12, x11
	sbcs	x10, x13, x10
	sbcs	x9, x14, x9
	sbcs	x8, x15, x8
	cmp		x8, #0          // =0
	csel	x11, x12, x11, lt
	csel	x10, x13, x10, lt
	csel	x9, x14, x9, lt
	csel	x8, x15, x8, lt
	stp		x11, x10, [x0]
	stp	x9, x8, [x0, #16]
	ldp	x20, x19, [sp, #64]     // 8-byte Folded Reload
	ldp	x22, x21, [sp, #48]     // 8-byte Folded Reload
	ldp	x24, x23, [sp, #32]     // 8-byte Folded Reload
	ldp	x26, x25, [sp, #16]     // 8-byte Folded Reload
	ldr	x27, [sp], #80          // 8-byte Folded Reload
	ret
.Lfunc_end27:
	.size	mcl_fp_montNF4L, .Lfunc_end27-mcl_fp_montNF4L

	.globl	mcl_fp_montRed4L
	.p2align	2
	.type	mcl_fp_montRed4L,@function
mcl_fp_montRed4L:                       // @mcl_fp_montRed4L
// BB#0:
	str	x19, [sp, #-16]!        // 8-byte Folded Spill
	ldp	x10, x12, [x2, #-8]
	ldp		x16, x17, [x1]
	ldr	x11, [x2, #8]
	ldp	x9, x8, [x2, #16]
	ldp	x15, x14, [x1, #16]
	mul		x3, x16, x10
	mul		x7, x3, x11
	umulh	x19, x3, x12
	mul		x6, x3, x9
	adds		x7, x19, x7
	umulh	x19, x3, x11
	mul		x5, x3, x8
	adcs	x6, x19, x6
	umulh	x19, x3, x9
	umulh	x4, x3, x8
	adcs	x5, x19, x5
	mul		x3, x3, x12
	adcs	x4, x4, xzr
	ldp	x13, x18, [x1, #32]
	cmn		x3, x16
	adcs	x16, x7, x17
	adcs	x15, x6, x15
	adcs	x14, x5, x14
	mul		x17, x16, x10
	adcs	x13, x4, x13
	umulh	x7, x17, x12
	mul		x19, x17, x11
	adcs	x4, xzr, xzr
	umulh	x6, x17, x11
	adds		x7, x19, x7
	mul		x19, x17, x9
	umulh	x5, x17, x9
	adcs	x6, x19, x6
	mul		x19, x17, x8
	umulh	x3, x17, x8
	adcs	x5, x19, x5
	mul		x17, x17, x12
	adcs	x3, x4, x3
	cmn		x17, x16
	adcs	x15, x7, x15
	adcs	x14, x6, x14
	adcs	x13, x5, x13
	mul		x16, x15, x10
	adcs	x18, x3, x18
	umulh	x7, x16, x12
	mul		x19, x16, x11
	adcs	x3, xzr, xzr
	umulh	x6, x16, x11
	adds		x7, x19, x7
	mul		x19, x16, x9
	umulh	x4, x16, x9
	mul		x5, x16, x8
	adcs	x6, x19, x6
	umulh	x17, x16, x8
	adcs	x4, x5, x4
	mul		x16, x16, x12
	adcs	x17, x3, x17
	ldp	x2, x1, [x1, #48]
	cmn		x16, x15
	adcs	x14, x7, x14
	adcs	x13, x6, x13
	adcs	x16, x4, x18
	mul		x10, x14, x10
	adcs	x17, x17, x2
	umulh	x6, x10, x12
	mul		x7, x10, x11
	adcs	x2, xzr, xzr
	umulh	x4, x10, x11
	mul		x5, x10, x9
	adds		x6, x7, x6
	umulh	x18, x10, x9
	mul		x3, x10, x8
	adcs	x4, x5, x4
	umulh	x15, x10, x8
	adcs	x18, x3, x18
	mul		x10, x10, x12
	adcs	x15, x2, x15
	cmn		x10, x14
	adcs	x10, x6, x13
	adcs	x13, x4, x16
	adcs	x14, x18, x17
	adcs	x15, x15, x1
	subs		x12, x10, x12
	sbcs	x11, x13, x11
	sbcs	x9, x14, x9
	sbcs	x8, x15, x8
	ngcs	 x16, xzr
	tst	 x16, #0x1
	csel	x10, x10, x12, ne
	csel	x11, x13, x11, ne
	csel	x9, x14, x9, ne
	csel	x8, x15, x8, ne
	stp		x10, x11, [x0]
	stp	x9, x8, [x0, #16]
	ldr	x19, [sp], #16          // 8-byte Folded Reload
	ret
.Lfunc_end28:
	.size	mcl_fp_montRed4L, .Lfunc_end28-mcl_fp_montRed4L

	.globl	mcl_fp_montRedNF4L
	.p2align	2
	.type	mcl_fp_montRedNF4L,@function
mcl_fp_montRedNF4L:                     // @mcl_fp_montRedNF4L
// BB#0:
	str	x19, [sp, #-16]!        // 8-byte Folded Spill
	ldp	x10, x12, [x2, #-8]
	ldp		x16, x17, [x1]
	ldr	x11, [x2, #8]
	ldp	x9, x8, [x2, #16]
	ldp	x15, x14, [x1, #16]
	mul		x3, x16, x10
	mul		x7, x3, x11
	umulh	x19, x3, x12
	mul		x6, x3, x9
	adds		x7, x19, x7
	umulh	x19, x3, x11
	mul		x5, x3, x8
	adcs	x6, x19, x6
	umulh	x19, x3, x9
	umulh	x4, x3, x8
	adcs	x5, x19, x5
	mul		x3, x3, x12
	adcs	x4, x4, xzr
	ldp	x13, x18, [x1, #32]
	cmn		x3, x16
	adcs	x16, x7, x17
	adcs	x15, x6, x15
	adcs	x14, x5, x14
	mul		x17, x16, x10
	adcs	x13, x4, x13
	umulh	x7, x17, x12
	mul		x19, x17, x11
	adcs	x4, xzr, xzr
	umulh	x6, x17, x11
	adds		x7, x19, x7
	mul		x19, x17, x9
	umulh	x5, x17, x9
	adcs	x6, x19, x6
	mul		x19, x17, x8
	umulh	x3, x17, x8
	adcs	x5, x19, x5
	mul		x17, x17, x12
	adcs	x3, x4, x3
	cmn		x17, x16
	adcs	x15, x7, x15
	adcs	x14, x6, x14
	adcs	x13, x5, x13
	mul		x16, x15, x10
	adcs	x18, x3, x18
	umulh	x7, x16, x12
	mul		x19, x16, x11
	adcs	x3, xzr, xzr
	umulh	x6, x16, x11
	adds		x7, x19, x7
	mul		x19, x16, x9
	umulh	x4, x16, x9
	mul		x5, x16, x8
	adcs	x6, x19, x6
	umulh	x17, x16, x8
	adcs	x4, x5, x4
	mul		x16, x16, x12
	adcs	x17, x3, x17
	ldp	x2, x1, [x1, #48]
	cmn		x16, x15
	adcs	x14, x7, x14
	adcs	x13, x6, x13
	adcs	x16, x4, x18
	mul		x10, x14, x10
	adcs	x17, x17, x2
	umulh	x6, x10, x12
	mul		x7, x10, x11
	adcs	x2, xzr, xzr
	umulh	x4, x10, x11
	mul		x5, x10, x9
	adds		x6, x7, x6
	umulh	x18, x10, x9
	mul		x3, x10, x8
	adcs	x4, x5, x4
	umulh	x15, x10, x8
	adcs	x18, x3, x18
	mul		x10, x10, x12
	adcs	x15, x2, x15
	cmn		x10, x14
	adcs	x10, x6, x13
	adcs	x13, x4, x16
	adcs	x14, x18, x17
	adcs	x15, x15, x1
	subs		x12, x10, x12
	sbcs	x11, x13, x11
	sbcs	x9, x14, x9
	sbcs	x8, x15, x8
	cmp		x8, #0          // =0
	csel	x10, x10, x12, lt
	csel	x11, x13, x11, lt
	csel	x9, x14, x9, lt
	csel	x8, x15, x8, lt
	stp		x10, x11, [x0]
	stp	x9, x8, [x0, #16]
	ldr	x19, [sp], #16          // 8-byte Folded Reload
	ret
.Lfunc_end29:
	.size	mcl_fp_montRedNF4L, .Lfunc_end29-mcl_fp_montRedNF4L

	.globl	mcl_fp_addPre4L
	.p2align	2
	.type	mcl_fp_addPre4L,@function
mcl_fp_addPre4L:                        // @mcl_fp_addPre4L
// BB#0:
	ldp		x10, x11, [x2]
	ldp		x12, x13, [x1]
	ldp	x8, x9, [x2, #16]
	ldp	x14, x15, [x1, #16]
	adds		x10, x10, x12
	adcs	x11, x11, x13
	stp		x10, x11, [x0]
	adcs	x10, x8, x14
	adcs	x9, x9, x15
	adcs	x8, xzr, xzr
	stp	x10, x9, [x0, #16]
	mov	 x0, x8
	ret
.Lfunc_end30:
	.size	mcl_fp_addPre4L, .Lfunc_end30-mcl_fp_addPre4L

	.globl	mcl_fp_subPre4L
	.p2align	2
	.type	mcl_fp_subPre4L,@function
mcl_fp_subPre4L:                        // @mcl_fp_subPre4L
// BB#0:
	ldp		x10, x11, [x2]
	ldp		x12, x13, [x1]
	ldp	x8, x9, [x2, #16]
	ldp	x14, x15, [x1, #16]
	subs		x10, x12, x10
	sbcs	x11, x13, x11
	stp		x10, x11, [x0]
	sbcs	x10, x14, x8
	sbcs	x9, x15, x9
	ngcs	 x8, xzr
	and	x8, x8, #0x1
	stp	x10, x9, [x0, #16]
	mov	 x0, x8
	ret
.Lfunc_end31:
	.size	mcl_fp_subPre4L, .Lfunc_end31-mcl_fp_subPre4L

	.globl	mcl_fp_shr1_4L
	.p2align	2
	.type	mcl_fp_shr1_4L,@function
mcl_fp_shr1_4L:                         // @mcl_fp_shr1_4L
// BB#0:
	ldp		x8, x9, [x1]
	ldp	x10, x11, [x1, #16]
	extr	x8, x9, x8, #1
	extr	x9, x10, x9, #1
	extr	x10, x11, x10, #1
	lsr	x11, x11, #1
	stp		x8, x9, [x0]
	stp	x10, x11, [x0, #16]
	ret
.Lfunc_end32:
	.size	mcl_fp_shr1_4L, .Lfunc_end32-mcl_fp_shr1_4L

	.globl	mcl_fp_add4L
	.p2align	2
	.type	mcl_fp_add4L,@function
mcl_fp_add4L:                           // @mcl_fp_add4L
// BB#0:
	ldp		x10, x11, [x2]
	ldp		x12, x13, [x1]
	ldp	x8, x9, [x2, #16]
	ldp	x14, x15, [x1, #16]
	adds		x10, x10, x12
	adcs	x12, x11, x13
	adcs	x14, x8, x14
	ldp		x8, x17, [x3]
	ldp	x13, x16, [x3, #16]
	adcs	x15, x9, x15
	adcs	x18, xzr, xzr
	subs		x11, x10, x8
	stp		x10, x12, [x0]
	sbcs	x10, x12, x17
	sbcs	x9, x14, x13
	sbcs	x8, x15, x16
	sbcs	x12, x18, xzr
	stp	x14, x15, [x0, #16]
	tbnz	w12, #0, .LBB33_2
// BB#1:                                // %nocarry
	stp		x11, x10, [x0]
	stp	x9, x8, [x0, #16]
.LBB33_2:                               // %carry
	ret
.Lfunc_end33:
	.size	mcl_fp_add4L, .Lfunc_end33-mcl_fp_add4L

	.globl	mcl_fp_addNF4L
	.p2align	2
	.type	mcl_fp_addNF4L,@function
mcl_fp_addNF4L:                         // @mcl_fp_addNF4L
// BB#0:
	ldp		x10, x11, [x1]
	ldp		x12, x13, [x2]
	ldp	x8, x9, [x1, #16]
	ldp	x14, x15, [x2, #16]
	adds		x10, x12, x10
	adcs	x11, x13, x11
	ldp		x12, x13, [x3]
	adcs	x8, x14, x8
	ldp	x14, x16, [x3, #16]
	adcs	x9, x15, x9
	subs		x12, x10, x12
	sbcs	x13, x11, x13
	sbcs	x14, x8, x14
	sbcs	x15, x9, x16
	cmp		x15, #0         // =0
	csel	x10, x10, x12, lt
	csel	x11, x11, x13, lt
	csel	x8, x8, x14, lt
	csel	x9, x9, x15, lt
	stp		x10, x11, [x0]
	stp	x8, x9, [x0, #16]
	ret
.Lfunc_end34:
	.size	mcl_fp_addNF4L, .Lfunc_end34-mcl_fp_addNF4L

	.globl	mcl_fp_sub4L
	.p2align	2
	.type	mcl_fp_sub4L,@function
mcl_fp_sub4L:                           // @mcl_fp_sub4L
// BB#0:
	ldp		x8, x9, [x2]
	ldp		x12, x13, [x1]
	ldp	x10, x11, [x2, #16]
	ldp	x14, x15, [x1, #16]
	subs		x8, x12, x8
	sbcs	x9, x13, x9
	sbcs	x10, x14, x10
	sbcs	x11, x15, x11
	ngcs	 x12, xzr
	stp		x8, x9, [x0]
	stp	x10, x11, [x0, #16]
	tbnz	w12, #0, .LBB35_2
// BB#1:                                // %nocarry
	ret
.LBB35_2:                               // %carry
	ldp		x12, x13, [x3]
	ldp	x14, x15, [x3, #16]
	adds		x8, x12, x8
	adcs	x9, x13, x9
	stp		x8, x9, [x0]
	adcs	x8, x14, x10
	adcs	x9, x15, x11
	stp	x8, x9, [x0, #16]
	ret
.Lfunc_end35:
	.size	mcl_fp_sub4L, .Lfunc_end35-mcl_fp_sub4L

	.globl	mcl_fp_subNF4L
	.p2align	2
	.type	mcl_fp_subNF4L,@function
mcl_fp_subNF4L:                         // @mcl_fp_subNF4L
// BB#0:
	ldp		x10, x11, [x2]
	ldp		x12, x13, [x1]
	ldp	x8, x9, [x2, #16]
	ldp	x14, x15, [x1, #16]
	subs		x10, x12, x10
	sbcs	x11, x13, x11
	sbcs	x8, x14, x8
	ldp		x14, x16, [x3]
	ldp	x12, x13, [x3, #16]
	sbcs	x9, x15, x9
	asr	x15, x9, #63
	and		x14, x15, x14
	and		x16, x15, x16
	adds		x10, x14, x10
	and		x12, x15, x12
	adcs	x11, x16, x11
	and		x13, x15, x13
	adcs	x8, x12, x8
	adcs	x9, x13, x9
	stp		x10, x11, [x0]
	stp	x8, x9, [x0, #16]
	ret
.Lfunc_end36:
	.size	mcl_fp_subNF4L, .Lfunc_end36-mcl_fp_subNF4L

	.globl	mcl_fpDbl_add4L
	.p2align	2
	.type	mcl_fpDbl_add4L,@function
mcl_fpDbl_add4L:                        // @mcl_fpDbl_add4L
// BB#0:
	ldp	x8, x9, [x2, #48]
	ldp	x10, x11, [x1, #48]
	ldp	x12, x13, [x2, #32]
	ldp	x14, x15, [x1, #32]
	ldp	x16, x17, [x2, #16]
	ldp		x4, x2, [x2]
	ldp	x5, x6, [x1, #16]
	ldp		x18, x1, [x1]
	adds		x18, x4, x18
	adcs	x1, x2, x1
	adcs	x16, x16, x5
	adcs	x17, x17, x6
	adcs	x12, x12, x14
	ldp	x4, x7, [x3, #16]
	ldp		x2, x3, [x3]
	adcs	x13, x13, x15
	adcs	x8, x8, x10
	adcs	x9, x9, x11
	adcs	x10, xzr, xzr
	subs		x11, x12, x2
	sbcs	x14, x13, x3
	sbcs	x15, x8, x4
	stp	x16, x17, [x0, #16]
	sbcs	x16, x9, x7
	sbcs	x10, x10, xzr
	tst	 x10, #0x1
	csel	x10, x12, x11, ne
	csel	x11, x13, x14, ne
	csel	x8, x8, x15, ne
	csel	x9, x9, x16, ne
	stp		x18, x1, [x0]
	stp	x10, x11, [x0, #32]
	stp	x8, x9, [x0, #48]
	ret
.Lfunc_end37:
	.size	mcl_fpDbl_add4L, .Lfunc_end37-mcl_fpDbl_add4L

	.globl	mcl_fpDbl_sub4L
	.p2align	2
	.type	mcl_fpDbl_sub4L,@function
mcl_fpDbl_sub4L:                        // @mcl_fpDbl_sub4L
// BB#0:
	ldp	x8, x9, [x2, #48]
	ldp	x10, x11, [x1, #48]
	ldp	x12, x13, [x2, #32]
	ldp	x14, x15, [x1, #32]
	ldp	x16, x17, [x2, #16]
	ldp		x18, x2, [x2]
	ldp	x5, x6, [x1, #16]
	ldp		x4, x1, [x1]
	subs		x18, x4, x18
	sbcs	x1, x1, x2
	sbcs	x16, x5, x16
	sbcs	x17, x6, x17
	sbcs	x12, x14, x12
	sbcs	x13, x15, x13
	ldp	x4, x7, [x3, #16]
	ldp		x2, x3, [x3]
	sbcs	x8, x10, x8
	sbcs	x9, x11, x9
	ngcs	 x10, xzr
	tst	 x10, #0x1
	csel	x15, x2, xzr, ne
	csel	x10, x7, xzr, ne
	csel	x11, x4, xzr, ne
	csel	x14, x3, xzr, ne
	adds		x12, x15, x12
	adcs	x13, x14, x13
	adcs	x8, x11, x8
	adcs	x9, x10, x9
	stp		x18, x1, [x0]
	stp	x16, x17, [x0, #16]
	stp	x12, x13, [x0, #32]
	stp	x8, x9, [x0, #48]
	ret
.Lfunc_end38:
	.size	mcl_fpDbl_sub4L, .Lfunc_end38-mcl_fpDbl_sub4L

	.globl	mulPv384x64
	.p2align	2
	.type	mulPv384x64,@function
mulPv384x64:                            // @mulPv384x64
// BB#0:
	ldp	x8, x9, [x0, #32]
	ldp		x12, x13, [x0]
	ldp	x10, x11, [x0, #16]
	umulh	x15, x8, x1
	mul		x16, x8, x1
	umulh	x0, x13, x1
	mul		x8, x13, x1
	umulh	x13, x12, x1
	umulh	x18, x10, x1
	mul		x10, x10, x1
	adds		x8, x13, x8
	umulh	x17, x11, x1
	mul		x11, x11, x1
	adcs	x2, x0, x10
	adcs	x3, x18, x11
	umulh	x14, x9, x1
	mul		x9, x9, x1
	adcs	x4, x17, x16
	adcs	x5, x15, x9
	adcs	x6, x14, xzr
	mul		x0, x12, x1
	mov	 x1, x8
	ret
.Lfunc_end39:
	.size	mulPv384x64, .Lfunc_end39-mulPv384x64

	.globl	mcl_fp_mulUnitPre6L
	.p2align	2
	.type	mcl_fp_mulUnitPre6L,@function
mcl_fp_mulUnitPre6L:                    // @mcl_fp_mulUnitPre6L
// BB#0:
	ldp		x10, x11, [x1]
	ldp	x12, x13, [x1, #16]
	ldp	x8, x9, [x1, #32]
	mul		x14, x10, x2
	mul		x15, x11, x2
	umulh	x10, x10, x2
	mul		x16, x12, x2
	umulh	x11, x11, x2
	adds		x10, x10, x15
	mul		x17, x13, x2
	umulh	x12, x12, x2
	stp		x14, x10, [x0]
	adcs	x10, x11, x16
	mul		x18, x8, x2
	umulh	x13, x13, x2
	adcs	x11, x12, x17
	mul		x1, x9, x2
	umulh	x8, x8, x2
	stp	x10, x11, [x0, #16]
	adcs	x10, x13, x18
	umulh	x9, x9, x2
	adcs	x8, x8, x1
	stp	x10, x8, [x0, #32]
	adcs	x8, x9, xzr
	str	x8, [x0, #48]
	ret
.Lfunc_end40:
	.size	mcl_fp_mulUnitPre6L, .Lfunc_end40-mcl_fp_mulUnitPre6L

	.globl	mcl_fpDbl_mulPre6L
	.p2align	2
	.type	mcl_fpDbl_mulPre6L,@function
mcl_fpDbl_mulPre6L:                     // @mcl_fpDbl_mulPre6L
// BB#0:
	sub	sp, sp, #496            // =496
	ldp		x8, x18, [x1]
	ldp		x14, x15, [x2]
	ldp	x12, x13, [x1, #32]
	ldp	x10, x17, [x1, #16]
	ldp		x16, x11, [x1]
	mul		x3, x8, x14
	str	x3, [sp, #392]          // 8-byte Folded Spill
	umulh	x3, x13, x14
	str	x3, [sp, #384]          // 8-byte Folded Spill
	mul		x3, x13, x14
	str	x3, [sp, #376]          // 8-byte Folded Spill
	umulh	x3, x12, x14
	str	x3, [sp, #360]          // 8-byte Folded Spill
	mul		x3, x12, x14
	str	x3, [sp, #336]          // 8-byte Folded Spill
	umulh	x3, x17, x14
	str	x3, [sp, #328]          // 8-byte Folded Spill
	mul		x3, x17, x14
	str	x3, [sp, #304]          // 8-byte Folded Spill
	umulh	x3, x10, x14
	str	x3, [sp, #280]          // 8-byte Folded Spill
	mul		x3, x10, x14
	str	x3, [sp, #248]          // 8-byte Folded Spill
	umulh	x3, x18, x14
	str	x3, [sp, #240]          // 8-byte Folded Spill
	mul		x3, x18, x14
	umulh	x14, x8, x14
	stp	x14, x3, [sp, #208]     // 8-byte Folded Spill
	mul		x14, x8, x15
	str	x14, [sp, #272]         // 8-byte Folded Spill
	mul		x14, x13, x15
	str	x14, [sp, #352]         // 8-byte Folded Spill
	mul		x14, x12, x15
	str	x14, [sp, #320]         // 8-byte Folded Spill
	mul		x14, x17, x15
	str	x14, [sp, #296]         // 8-byte Folded Spill
	mul		x14, x10, x15
	umulh	x12, x12, x15
	str	x14, [sp, #264]         // 8-byte Folded Spill
	mul		x14, x18, x15
	str	x12, [sp, #344]         // 8-byte Folded Spill
	umulh	x12, x17, x15
	umulh	x10, x10, x15
	umulh	x8, x8, x15
	str	x12, [sp, #312]         // 8-byte Folded Spill
	str	x10, [sp, #288]         // 8-byte Folded Spill
	umulh	x10, x18, x15
	stp	x8, x14, [sp, #224]     // 8-byte Folded Spill
	ldp	x12, x8, [x2, #16]
	umulh	x13, x13, x15
	str	x10, [sp, #256]         // 8-byte Folded Spill
	ldp	x10, x15, [x1, #24]
	ldr	x9, [x1, #16]
	mul		x14, x16, x12
	str	x13, [sp, #368]         // 8-byte Folded Spill
	ldr	x13, [x1, #40]
	str	x14, [sp, #144]         // 8-byte Folded Spill
	mul		x14, x15, x12
	str	x14, [sp, #176]         // 8-byte Folded Spill
	mul		x14, x10, x12
	str	x14, [sp, #160]         // 8-byte Folded Spill
	mul		x14, x9, x12
	str	x14, [sp, #136]         // 8-byte Folded Spill
	mul		x14, x11, x12
	str	x14, [sp, #112]         // 8-byte Folded Spill
	umulh	x14, x13, x12
	str	x14, [sp, #200]         // 8-byte Folded Spill
	umulh	x14, x15, x12
	str	x14, [sp, #184]         // 8-byte Folded Spill
	umulh	x14, x10, x12
	str	x14, [sp, #168]         // 8-byte Folded Spill
	umulh	x14, x9, x12
	stp	x29, x30, [sp, #480]    // 8-byte Folded Spill
	mul		x29, x13, x12
	str	x14, [sp, #152]         // 8-byte Folded Spill
	umulh	x14, x11, x12
	umulh	x12, x16, x12
	str	x12, [sp, #104]         // 8-byte Folded Spill
	mul		x12, x13, x8
	str	x12, [sp, #192]         // 8-byte Folded Spill
	umulh	x12, x13, x8
	stp	x12, x14, [sp, #120]    // 8-byte Folded Spill
	mul		x12, x15, x8
	str	x12, [sp, #80]          // 8-byte Folded Spill
	umulh	x12, x15, x8
	str	x12, [sp, #96]          // 8-byte Folded Spill
	mul		x12, x10, x8
	umulh	x10, x10, x8
	str	x10, [sp, #88]          // 8-byte Folded Spill
	mul		x10, x9, x8
	umulh	x9, x9, x8
	stp	x12, x9, [sp, #64]      // 8-byte Folded Spill
	mul		x9, x11, x8
	str	x9, [sp, #32]           // 8-byte Folded Spill
	umulh	x9, x11, x8
	stp	x10, x9, [sp, #48]      // 8-byte Folded Spill
	mul		x9, x16, x8
	umulh	x8, x16, x8
	str	x9, [sp, #24]           // 8-byte Folded Spill
	str	x8, [sp, #40]           // 8-byte Folded Spill
	ldp		x16, x9, [x1]
	ldp	x10, x11, [x1, #16]
	ldp	x8, x2, [x2, #32]
	ldp	x1, x12, [x1, #32]
	stp	x28, x27, [sp, #400]    // 8-byte Folded Spill
	stp	x26, x25, [sp, #416]    // 8-byte Folded Spill
	stp	x24, x23, [sp, #432]    // 8-byte Folded Spill
	umulh	x13, x12, x8
	stp	x22, x21, [sp, #448]    // 8-byte Folded Spill
	stp	x20, x19, [sp, #464]    // 8-byte Folded Spill
	mul		x23, x16, x8
	mul		x30, x12, x8
	mul		x27, x1, x8
	mul		x24, x11, x8
	mul		x22, x10, x8
	mul		x19, x9, x8
	str	x13, [sp, #8]           // 8-byte Folded Spill
	umulh	x13, x1, x8
	umulh	x28, x11, x8
	umulh	x25, x10, x8
	umulh	x21, x9, x8
	umulh	x20, x16, x8
	mul		x5, x9, x2
	umulh	x6, x9, x2
	ldp	x9, x8, [sp, #208]      // 8-byte Folded Reload
	mul		x26, x16, x2
	umulh	x7, x16, x2
	mul		x3, x10, x2
	umulh	x4, x10, x2
	mul		x17, x11, x2
	umulh	x18, x11, x2
	mul		x15, x1, x2
	umulh	x1, x1, x2
	mul		x14, x12, x2
	umulh	x16, x12, x2
	adds		x2, x9, x8
	ldp	x9, x8, [sp, #240]      // 8-byte Folded Reload
	str	x13, [sp, #16]          // 8-byte Folded Spill
	ldp	x13, x10, [sp, #272]    // 8-byte Folded Reload
	ldr	x12, [sp, #360]         // 8-byte Folded Reload
	adcs	x8, x9, x8
	ldr	x9, [sp, #304]          // 8-byte Folded Reload
	adcs	x9, x10, x9
	ldp	x11, x10, [sp, #328]    // 8-byte Folded Reload
	adcs	x10, x11, x10
	ldr	x11, [sp, #376]         // 8-byte Folded Reload
	adcs	x11, x12, x11
	ldr	x12, [sp, #384]         // 8-byte Folded Reload
	adcs	x12, x12, xzr
	adds		x2, x13, x2
	ldr	x13, [sp, #392]         // 8-byte Folded Reload
	stp		x13, x2, [x0]
	ldr	x13, [sp, #232]         // 8-byte Folded Reload
	adcs	x8, x13, x8
	ldr	x13, [sp, #264]         // 8-byte Folded Reload
	adcs	x9, x13, x9
	ldr	x13, [sp, #296]         // 8-byte Folded Reload
	adcs	x10, x13, x10
	ldr	x13, [sp, #320]         // 8-byte Folded Reload
	adcs	x11, x13, x11
	ldr	x13, [sp, #352]         // 8-byte Folded Reload
	adcs	x12, x13, x12
	ldr	x13, [sp, #224]         // 8-byte Folded Reload
	adcs	x2, xzr, xzr
	adds		x8, x8, x13
	ldr	x13, [sp, #256]         // 8-byte Folded Reload
	adcs	x9, x9, x13
	ldr	x13, [sp, #288]         // 8-byte Folded Reload
	adcs	x10, x10, x13
	ldr	x13, [sp, #312]         // 8-byte Folded Reload
	adcs	x11, x11, x13
	ldr	x13, [sp, #344]         // 8-byte Folded Reload
	adcs	x12, x12, x13
	ldr	x13, [sp, #368]         // 8-byte Folded Reload
	adcs	x2, x2, x13
	ldr	x13, [sp, #144]         // 8-byte Folded Reload
	adds		x13, x13, x8
	ldr	x8, [sp, #112]          // 8-byte Folded Reload
	adcs	x9, x8, x9
	ldr	x8, [sp, #136]          // 8-byte Folded Reload
	adcs	x10, x8, x10
	ldr	x8, [sp, #160]          // 8-byte Folded Reload
	adcs	x11, x8, x11
	ldr	x8, [sp, #176]          // 8-byte Folded Reload
	adcs	x12, x8, x12
	adcs	x2, x29, x2
	ldr	x29, [sp, #104]         // 8-byte Folded Reload
	adcs	x8, xzr, xzr
	adds		x9, x9, x29
	ldr	x29, [sp, #128]         // 8-byte Folded Reload
	adcs	x10, x10, x29
	ldr	x29, [sp, #152]         // 8-byte Folded Reload
	adcs	x11, x11, x29
	ldr	x29, [sp, #168]         // 8-byte Folded Reload
	adcs	x12, x12, x29
	ldr	x29, [sp, #184]         // 8-byte Folded Reload
	adcs	x2, x2, x29
	ldr	x29, [sp, #200]         // 8-byte Folded Reload
	adcs	x8, x8, x29
	ldr	x29, [sp, #24]          // 8-byte Folded Reload
	adds		x9, x29, x9
	stp	x13, x9, [x0, #16]
	ldr	x9, [sp, #32]           // 8-byte Folded Reload
	ldr	x13, [sp, #192]         // 8-byte Folded Reload
	adcs	x9, x9, x10
	ldr	x10, [sp, #48]          // 8-byte Folded Reload
	adcs	x10, x10, x11
	ldr	x11, [sp, #64]          // 8-byte Folded Reload
	adcs	x11, x11, x12
	ldr	x12, [sp, #80]          // 8-byte Folded Reload
	adcs	x12, x12, x2
	ldr	x2, [sp, #40]           // 8-byte Folded Reload
	adcs	x8, x13, x8
	adcs	x13, xzr, xzr
	adds		x9, x9, x2
	ldr	x2, [sp, #56]           // 8-byte Folded Reload
	adcs	x10, x10, x2
	ldr	x2, [sp, #72]           // 8-byte Folded Reload
	adcs	x11, x11, x2
	ldr	x2, [sp, #88]           // 8-byte Folded Reload
	adcs	x12, x12, x2
	ldr	x2, [sp, #96]           // 8-byte Folded Reload
	adcs	x8, x8, x2
	ldr	x2, [sp, #120]          // 8-byte Folded Reload
	adcs	x13, x13, x2
	adds		x9, x23, x9
	adcs	x10, x19, x10
	adcs	x11, x22, x11
	adcs	x12, x24, x12
	adcs	x8, x27, x8
	adcs	x13, x30, x13
	adcs	x2, xzr, xzr
	adds		x10, x10, x20
	ldr	x19, [sp, #16]          // 8-byte Folded Reload
	adcs	x11, x11, x21
	adcs	x12, x12, x25
	adcs	x8, x8, x28
	adcs	x13, x13, x19
	ldr	x19, [sp, #8]           // 8-byte Folded Reload
	ldp	x29, x30, [sp, #480]    // 8-byte Folded Reload
	ldp	x22, x21, [sp, #448]    // 8-byte Folded Reload
	ldp	x24, x23, [sp, #432]    // 8-byte Folded Reload
	adcs	x2, x2, x19
	adds		x10, x26, x10
	stp	x9, x10, [x0, #32]
	adcs	x9, x5, x11
	adcs	x10, x3, x12
	adcs	x8, x17, x8
	adcs	x11, x15, x13
	adcs	x12, x14, x2
	adcs	x13, xzr, xzr
	adds		x9, x9, x7
	adcs	x10, x10, x6
	adcs	x8, x8, x4
	stp	x9, x10, [x0, #48]
	adcs	x9, x11, x18
	ldp	x20, x19, [sp, #464]    // 8-byte Folded Reload
	ldp	x26, x25, [sp, #416]    // 8-byte Folded Reload
	ldp	x28, x27, [sp, #400]    // 8-byte Folded Reload
	stp	x8, x9, [x0, #64]
	adcs	x8, x12, x1
	adcs	x9, x13, x16
	stp	x8, x9, [x0, #80]
	add	sp, sp, #496            // =496
	ret
.Lfunc_end41:
	.size	mcl_fpDbl_mulPre6L, .Lfunc_end41-mcl_fpDbl_mulPre6L

	.globl	mcl_fpDbl_sqrPre6L
	.p2align	2
	.type	mcl_fpDbl_sqrPre6L,@function
mcl_fpDbl_sqrPre6L:                     // @mcl_fpDbl_sqrPre6L
// BB#0:
	str	x23, [sp, #-48]!        // 8-byte Folded Spill
	ldp		x9, x12, [x1]
	ldp		x18, x4, [x1]
	ldp	x11, x13, [x1, #8]
	ldp	x10, x14, [x1, #16]
	ldp	x8, x15, [x1, #24]
	stp	x22, x21, [sp, #16]     // 8-byte Folded Spill
	ldp	x16, x17, [x1, #32]
	mul		x22, x12, x18
	umulh	x23, x18, x18
	stp	x20, x19, [sp, #32]     // 8-byte Folded Spill
	mul		x20, x13, x18
	umulh	x21, x12, x18
	adds		x23, x23, x22
	mul		x7, x14, x18
	umulh	x19, x13, x18
	adcs	x20, x21, x20
	mul		x6, x15, x18
	adcs	x7, x19, x7
	umulh	x19, x14, x18
	mul		x5, x17, x18
	adcs	x6, x19, x6
	umulh	x19, x15, x18
	adcs	x5, x19, x5
	umulh	x19, x17, x18
	adcs	x19, x19, xzr
	adds		x22, x22, x23
	mul		x18, x18, x18
	stp		x18, x22, [x0]
	mul		x18, x12, x12
	adcs	x18, x18, x20
	mul		x20, x13, x12
	adcs	x7, x20, x7
	mul		x20, x14, x12
	adcs	x6, x20, x6
	mul		x20, x15, x12
	adcs	x5, x20, x5
	mul		x20, x17, x12
	adcs	x19, x20, x19
	adcs	x20, xzr, xzr
	adds		x18, x18, x21
	umulh	x17, x17, x12
	umulh	x15, x15, x12
	umulh	x14, x14, x12
	umulh	x13, x13, x12
	umulh	x12, x12, x12
	ldr		x3, [x1]
	ldp	x2, x23, [x1, #16]
	adcs	x12, x7, x12
	adcs	x13, x6, x13
	adcs	x14, x5, x14
	adcs	x15, x19, x15
	ldp	x21, x7, [x1, #32]
	mul		x6, x3, x2
	adcs	x17, x20, x17
	adds		x18, x6, x18
	mul		x6, x4, x2
	adcs	x12, x6, x12
	mul		x6, x2, x2
	mul		x20, x23, x2
	adcs	x13, x6, x13
	mul		x19, x21, x2
	adcs	x14, x20, x14
	mul		x5, x7, x2
	adcs	x15, x19, x15
	adcs	x17, x5, x17
	umulh	x19, x3, x2
	adcs	x5, xzr, xzr
	adds		x12, x12, x19
	umulh	x19, x4, x2
	adcs	x13, x13, x19
	umulh	x19, x2, x2
	adcs	x14, x14, x19
	umulh	x19, x23, x2
	umulh	x6, x21, x2
	adcs	x15, x15, x19
	adcs	x17, x17, x6
	umulh	x2, x7, x2
	adcs	x2, x5, x2
	mul		x5, x3, x23
	adds		x12, x5, x12
	stp	x18, x12, [x0, #16]
	mul		x12, x4, x23
	adcs	x12, x12, x13
	adcs	x14, x20, x14
	mul		x18, x23, x23
	adcs	x15, x18, x15
	mul		x18, x21, x23
	mul		x13, x7, x23
	adcs	x17, x18, x17
	adcs	x13, x13, x2
	umulh	x3, x3, x23
	adcs	x2, xzr, xzr
	umulh	x4, x4, x23
	adds		x12, x12, x3
	adcs	x14, x14, x4
	umulh	x5, x7, x23
	umulh	x7, x23, x23
	adcs	x15, x15, x19
	umulh	x6, x21, x23
	adcs	x17, x17, x7
	adcs	x13, x13, x6
	mul		x18, x9, x16
	adcs	x2, x2, x5
	ldr	x1, [x1, #40]
	mul		x7, x11, x16
	adds		x12, x18, x12
	mul		x19, x10, x16
	adcs	x14, x7, x14
	mul		x4, x8, x16
	adcs	x15, x19, x15
	mul		x6, x16, x16
	adcs	x17, x4, x17
	mul		x3, x1, x16
	adcs	x13, x6, x13
	adcs	x2, x3, x2
	umulh	x4, x9, x16
	adcs	x6, xzr, xzr
	umulh	x19, x11, x16
	adds		x14, x14, x4
	umulh	x7, x10, x16
	adcs	x15, x15, x19
	umulh	x18, x8, x16
	adcs	x17, x17, x7
	umulh	x5, x1, x16
	umulh	x16, x16, x16
	adcs	x13, x13, x18
	adcs	x16, x2, x16
	mul		x4, x9, x1
	adcs	x6, x6, x5
	mul		x18, x11, x1
	adds		x14, x4, x14
	mul		x7, x10, x1
	stp	x12, x14, [x0, #32]
	adcs	x12, x18, x15
	mul		x19, x8, x1
	adcs	x14, x7, x17
	adcs	x13, x19, x13
	mul		x2, x1, x1
	adcs	x15, x3, x16
	adcs	x16, x2, x6
	umulh	x9, x9, x1
	adcs	x17, xzr, xzr
	umulh	x11, x11, x1
	adds		x9, x12, x9
	umulh	x10, x10, x1
	adcs	x11, x14, x11
	umulh	x8, x8, x1
	stp	x9, x11, [x0, #48]
	adcs	x9, x13, x10
	adcs	x8, x15, x8
	ldp	x20, x19, [sp, #32]     // 8-byte Folded Reload
	ldp	x22, x21, [sp, #16]     // 8-byte Folded Reload
	umulh	x1, x1, x1
	stp	x9, x8, [x0, #64]
	adcs	x8, x16, x5
	adcs	x9, x17, x1
	stp	x8, x9, [x0, #80]
	ldr	x23, [sp], #48          // 8-byte Folded Reload
	ret
.Lfunc_end42:
	.size	mcl_fpDbl_sqrPre6L, .Lfunc_end42-mcl_fpDbl_sqrPre6L

	.globl	mcl_fp_mont6L
	.p2align	2
	.type	mcl_fp_mont6L,@function
mcl_fp_mont6L:                          // @mcl_fp_mont6L
// BB#0:
	sub	sp, sp, #144            // =144
	stp	x28, x27, [sp, #48]     // 8-byte Folded Spill
	stp	x26, x25, [sp, #64]     // 8-byte Folded Spill
	stp	x24, x23, [sp, #80]     // 8-byte Folded Spill
	stp	x22, x21, [sp, #96]     // 8-byte Folded Spill
	stp	x20, x19, [sp, #112]    // 8-byte Folded Spill
	stp	x29, x30, [sp, #128]    // 8-byte Folded Spill
	str	x0, [sp, #24]           // 8-byte Folded Spill
	ldr		x5, [x2]
	str	x2, [sp, #32]           // 8-byte Folded Spill
	ldp	x0, x4, [x1, #32]
	ldp	x16, x18, [x1, #16]
	ldp		x10, x1, [x1]
	ldp	x8, x14, [x3, #-8]
	ldr	x15, [x3, #8]
	mul		x24, x16, x5
	mul		x26, x1, x5
	umulh	x27, x10, x5
	umulh	x25, x1, x5
	adds		x26, x27, x26
	mul		x22, x18, x5
	umulh	x23, x16, x5
	adcs	x24, x25, x24
	ldp	x13, x17, [x3, #16]
	mul		x20, x0, x5
	umulh	x21, x18, x5
	adcs	x22, x23, x22
	umulh	x6, x4, x5
	mul		x7, x4, x5
	umulh	x19, x0, x5
	mul		x5, x10, x5
	adcs	x20, x21, x20
	ldp	x11, x12, [x3, #32]
	mul		x27, x5, x8
	adcs	x7, x19, x7
	mul		x21, x27, x15
	umulh	x19, x27, x14
	adcs	x6, x6, xzr
	mul		x23, x27, x13
	adds		x19, x19, x21
	umulh	x21, x27, x15
	mul		x30, x27, x17
	adcs	x21, x21, x23
	umulh	x23, x27, x13
	mul		x29, x27, x11
	adcs	x23, x23, x30
	umulh	x30, x27, x17
	mul		x25, x27, x12
	adcs	x29, x30, x29
	umulh	x30, x27, x11
	adcs	x25, x30, x25
	umulh	x30, x27, x12
	mul		x27, x27, x14
	adcs	x30, x30, xzr
	cmn		x27, x5
	adcs	x19, x19, x26
	adcs	x21, x21, x24
	ldr	x3, [x2, #8]
	adcs	x22, x23, x22
	adcs	x20, x29, x20
	adcs	x7, x25, x7
	adcs	x30, x30, x6
	mul		x29, x3, x1
	umulh	x25, x3, x10
	adcs	x6, xzr, xzr
	mul		x23, x3, x16
	adds		x25, x25, x29
	umulh	x29, x3, x1
	mul		x24, x3, x18
	adcs	x23, x29, x23
	umulh	x29, x3, x16
	mul		x26, x3, x0
	adcs	x24, x29, x24
	umulh	x29, x3, x18
	mul		x5, x3, x4
	umulh	x27, x3, x0
	adcs	x26, x29, x26
	umulh	x28, x3, x4
	adcs	x27, x27, x5
	mul		x3, x3, x10
	adcs	x29, x28, xzr
	adds		x3, x19, x3
	adcs	x5, x21, x25
	adcs	x28, x22, x23
	adcs	x19, x20, x24
	adcs	x20, x7, x26
	mov	 x2, x8
	adcs	x30, x30, x27
	mul		x21, x3, x2
	adcs	x6, x6, x29
	mul		x24, x21, x15
	umulh	x26, x21, x14
	adcs	x7, xzr, xzr
	mul		x8, x21, x13
	adds		x24, x26, x24
	umulh	x26, x21, x15
	mul		x9, x21, x17
	adcs	x29, x26, x8
	umulh	x8, x21, x13
	mul		x25, x21, x11
	adcs	x26, x8, x9
	umulh	x8, x21, x17
	mul		x23, x21, x12
	adcs	x27, x8, x25
	umulh	x8, x21, x11
	umulh	x22, x21, x12
	adcs	x8, x8, x23
	mul		x9, x21, x14
	ldr	x25, [sp, #32]          // 8-byte Folded Reload
	adcs	x21, x22, xzr
	cmn		x9, x3
	adcs	x5, x24, x5
	str	x2, [sp, #40]           // 8-byte Folded Spill
	adcs	x24, x29, x28
	ldp	x23, x3, [x25, #16]
	adcs	x19, x26, x19
	adcs	x20, x27, x20
	adcs	x8, x8, x30
	adcs	x21, x21, x6
	mul		x28, x23, x1
	umulh	x6, x23, x10
	adcs	x7, x7, xzr
	mul		x27, x23, x16
	adds		x6, x6, x28
	umulh	x28, x23, x1
	mul		x26, x23, x18
	adcs	x27, x28, x27
	umulh	x28, x23, x16
	mul		x25, x23, x0
	adcs	x26, x28, x26
	umulh	x28, x23, x18
	mul		x22, x23, x4
	adcs	x25, x28, x25
	umulh	x28, x23, x0
	umulh	x9, x23, x4
	adcs	x22, x28, x22
	mul		x23, x23, x10
	adcs	x9, x9, xzr
	adds		x23, x5, x23
	adcs	x5, x24, x6
	adcs	x6, x19, x27
	adcs	x19, x20, x26
	adcs	x20, x8, x25
	adcs	x21, x21, x22
	mul		x29, x23, x2
	adcs	x22, x7, x9
	mul		x8, x29, x15
	umulh	x24, x29, x14
	adcs	x7, xzr, xzr
	mul		x26, x29, x13
	adds		x24, x24, x8
	umulh	x8, x29, x15
	mul		x30, x29, x17
	adcs	x25, x8, x26
	umulh	x8, x29, x13
	mul		x27, x29, x11
	adcs	x26, x8, x30
	umulh	x8, x29, x17
	mul		x28, x29, x12
	adcs	x27, x8, x27
	umulh	x8, x29, x11
	adcs	x28, x8, x28
	umulh	x8, x29, x12
	mul		x9, x29, x14
	adcs	x29, x8, xzr
	cmn		x9, x23
	adcs	x2, x24, x5
	adcs	x6, x25, x6
	adcs	x19, x26, x19
	adcs	x20, x27, x20
	adcs	x21, x28, x21
	adcs	x22, x29, x22
	mov	 x9, x10
	mul		x27, x3, x1
	umulh	x28, x3, x9
	adcs	x7, x7, xzr
	mul		x26, x3, x16
	adds		x27, x28, x27
	umulh	x28, x3, x1
	mul		x25, x3, x18
	adcs	x26, x28, x26
	umulh	x28, x3, x16
	mul		x24, x3, x0
	adcs	x25, x28, x25
	umulh	x28, x3, x18
	mul		x5, x3, x4
	adcs	x24, x28, x24
	umulh	x28, x3, x0
	umulh	x30, x3, x4
	adcs	x5, x28, x5
	mul		x3, x3, x9
	adcs	x29, x30, xzr
	adds		x2, x2, x3
	adcs	x3, x6, x27
	ldr	x10, [sp, #40]          // 8-byte Folded Reload
	adcs	x19, x19, x26
	adcs	x20, x20, x25
	adcs	x21, x21, x24
	adcs	x5, x22, x5
	mul		x6, x2, x10
	mov	 x30, x17
	mov	 x17, x15
	adcs	x29, x7, x29
	mul		x24, x6, x17
	umulh	x22, x6, x14
	adcs	x7, xzr, xzr
	mul		x25, x6, x13
	adds		x22, x22, x24
	umulh	x24, x6, x17
	mul		x28, x6, x30
	adcs	x24, x24, x25
	umulh	x25, x6, x13
	mul		x27, x6, x11
	adcs	x25, x25, x28
	umulh	x28, x6, x30
	mul		x26, x6, x12
	adcs	x27, x28, x27
	umulh	x28, x6, x11
	adcs	x26, x28, x26
	umulh	x28, x6, x12
	ldr	x8, [sp, #32]           // 8-byte Folded Reload
	mul		x6, x6, x14
	adcs	x28, x28, xzr
	cmn		x6, x2
	adcs	x3, x22, x3
	adcs	x19, x24, x19
	ldp	x23, x8, [x8, #32]
	adcs	x20, x25, x20
	adcs	x21, x27, x21
	adcs	x5, x26, x5
	adcs	x29, x28, x29
	mul		x26, x23, x1
	umulh	x28, x23, x9
	adcs	x7, x7, xzr
	mul		x27, x23, x16
	adds		x26, x28, x26
	umulh	x28, x23, x1
	mul		x25, x23, x18
	adcs	x27, x28, x27
	umulh	x28, x23, x16
	mul		x24, x23, x0
	adcs	x25, x28, x25
	umulh	x28, x23, x18
	mul		x6, x23, x4
	umulh	x22, x23, x0
	adcs	x24, x28, x24
	umulh	x2, x23, x4
	adcs	x6, x22, x6
	mul		x23, x23, x9
	adcs	x2, x2, xzr
	adds		x3, x3, x23
	adcs	x19, x19, x26
	adcs	x20, x20, x27
	adcs	x21, x21, x25
	umulh	x28, x8, x4
	adcs	x5, x5, x24
	str	x28, [sp, #32]          // 8-byte Folded Spill
	mul		x28, x8, x4
	adcs	x4, x29, x6
	mul		x22, x3, x10
	adcs	x2, x7, x2
	mov	 x15, x13
	mul		x24, x22, x17
	umulh	x6, x22, x14
	adcs	x7, xzr, xzr
	mov	 x13, x30
	mul		x25, x22, x15
	adds		x6, x6, x24
	umulh	x24, x22, x17
	mul		x27, x22, x13
	adcs	x24, x24, x25
	umulh	x25, x22, x15
	mul		x26, x22, x11
	adcs	x25, x25, x27
	umulh	x27, x22, x13
	mul		x23, x22, x12
	adcs	x26, x27, x26
	umulh	x27, x22, x11
	adcs	x23, x27, x23
	umulh	x27, x22, x12
	mul		x22, x22, x14
	adcs	x27, x27, xzr
	cmn		x22, x3
	adcs	x6, x6, x19
	adcs	x19, x24, x20
	adcs	x20, x25, x21
	adcs	x5, x26, x5
	umulh	x3, x8, x0
	mul		x0, x8, x0
	umulh	x22, x8, x18
	mul		x18, x8, x18
	umulh	x29, x8, x16
	mul		x16, x8, x16
	umulh	x30, x8, x1
	mul		x1, x8, x1
	umulh	x10, x8, x9
	mul		x8, x8, x9
	adcs	x9, x23, x4
	adcs	x2, x27, x2
	adcs	x7, x7, xzr
	stp	x9, x12, [sp, #8]       // 8-byte Folded Spill
	adds		x9, x10, x1
	adcs	x16, x30, x16
	ldr	x10, [sp, #32]          // 8-byte Folded Reload
	adcs	x18, x29, x18
	adcs	x0, x22, x0
	adcs	x1, x3, x28
	adcs	x3, x10, xzr
	ldr	x10, [sp, #40]          // 8-byte Folded Reload
	adds		x8, x6, x8
	adcs	x9, x19, x9
	adcs	x16, x20, x16
	mul		x4, x8, x10
	ldr	x10, [sp, #8]           // 8-byte Folded Reload
	adcs	x18, x5, x18
	mul		x27, x4, x17
	umulh	x28, x4, x14
	adcs	x10, x10, x0
	adcs	x0, x2, x1
	adcs	x1, x7, x3
	adcs	x2, xzr, xzr
	mul		x25, x4, x15
	umulh	x26, x4, x17
	adds		x3, x28, x27
	mov	 x30, x11
	mul		x23, x4, x13
	umulh	x24, x4, x15
	adcs	x5, x26, x25
	mul		x21, x4, x30
	umulh	x22, x4, x13
	adcs	x7, x24, x23
	mul		x19, x4, x12
	umulh	x20, x4, x30
	adcs	x21, x22, x21
	umulh	x6, x4, x12
	adcs	x19, x20, x19
	mul		x4, x4, x14
	adcs	x6, x6, xzr
	cmn		x4, x8
	adcs	x8, x3, x9
	adcs	x9, x5, x16
	adcs	x16, x7, x18
	adcs	x10, x21, x10
	adcs	x18, x19, x0
	adcs	x0, x6, x1
	adcs	x1, x2, xzr
	mov	 x29, x13
	subs		x13, x8, x14
	sbcs	x12, x9, x17
	ldr	x17, [sp, #16]          // 8-byte Folded Reload
	sbcs	x11, x16, x15
	sbcs	x14, x10, x29
	sbcs	x15, x18, x30
	sbcs	x17, x0, x17
	sbcs	x1, x1, xzr
	tst	 x1, #0x1
	csel	x10, x10, x14, ne
	ldr	x14, [sp, #24]          // 8-byte Folded Reload
	csel	x8, x8, x13, ne
	csel	x9, x9, x12, ne
	csel	x11, x16, x11, ne
	csel	x12, x18, x15, ne
	csel	x13, x0, x17, ne
	stp		x8, x9, [x14]
	stp	x11, x10, [x14, #16]
	stp	x12, x13, [x14, #32]
	ldp	x29, x30, [sp, #128]    // 8-byte Folded Reload
	ldp	x20, x19, [sp, #112]    // 8-byte Folded Reload
	ldp	x22, x21, [sp, #96]     // 8-byte Folded Reload
	ldp	x24, x23, [sp, #80]     // 8-byte Folded Reload
	ldp	x26, x25, [sp, #64]     // 8-byte Folded Reload
	ldp	x28, x27, [sp, #48]     // 8-byte Folded Reload
	add	sp, sp, #144            // =144
	ret
.Lfunc_end43:
	.size	mcl_fp_mont6L, .Lfunc_end43-mcl_fp_mont6L

	.globl	mcl_fp_montNF6L
	.p2align	2
	.type	mcl_fp_montNF6L,@function
mcl_fp_montNF6L:                        // @mcl_fp_montNF6L
// BB#0:
	sub	sp, sp, #208            // =208
	stp	x28, x27, [sp, #112]    // 8-byte Folded Spill
	stp	x26, x25, [sp, #128]    // 8-byte Folded Spill
	stp	x24, x23, [sp, #144]    // 8-byte Folded Spill
	stp	x22, x21, [sp, #160]    // 8-byte Folded Spill
	stp	x20, x19, [sp, #176]    // 8-byte Folded Spill
	stp	x29, x30, [sp, #192]    // 8-byte Folded Spill
	str	x0, [sp, #96]           // 8-byte Folded Spill
	ldr	x9, [x3, #32]
	ldp	x16, x12, [x1, #32]
	ldp	x13, x11, [x1, #16]
	ldp		x17, x0, [x1]
	ldur	x18, [x3, #-8]
	str	x9, [sp, #104]          // 8-byte Folded Spill
	ldr	x14, [x3, #40]
	ldp	x4, x10, [x3, #16]
	ldp		x15, x9, [x3]
	ldp		x5, x3, [x2]
	mov	 x1, x13
	mov	 x13, x0
	mov	 x8, x17
	mul		x29, x13, x5
	umulh	x30, x8, x5
	mul		x27, x1, x5
	umulh	x28, x13, x5
	adds		x29, x30, x29
	mul		x25, x11, x5
	umulh	x26, x1, x5
	adcs	x27, x28, x27
	mul		x23, x16, x5
	umulh	x24, x11, x5
	adcs	x25, x26, x25
	umulh	x20, x12, x5
	mul		x21, x12, x5
	umulh	x22, x16, x5
	mul		x5, x8, x5
	adcs	x23, x24, x23
	mul		x24, x5, x18
	adcs	x21, x22, x21
	mul		x22, x24, x15
	adcs	x20, x20, xzr
	mov	 x0, x9
	ldr	x9, [sp, #104]          // 8-byte Folded Reload
	cmn		x22, x5
	mul		x22, x24, x0
	adcs	x22, x22, x29
	mul		x29, x24, x4
	adcs	x17, x29, x27
	mul		x29, x24, x10
	adcs	x25, x29, x25
	mul		x29, x24, x9
	adcs	x23, x29, x23
	mul		x29, x24, x14
	adcs	x21, x29, x21
	umulh	x29, x24, x15
	adcs	x20, x20, xzr
	adds		x22, x22, x29
	umulh	x29, x24, x0
	ldp	x6, x7, [x2, #16]
	ldp	x19, x2, [x2, #32]
	str	x15, [sp, #8]           // 8-byte Folded Spill
	adcs	x15, x17, x29
	umulh	x29, x24, x4
	adcs	x25, x25, x29
	umulh	x29, x24, x10
	adcs	x23, x23, x29
	umulh	x29, x24, x9
	adcs	x21, x21, x29
	umulh	x24, x24, x14
	mul		x29, x3, x13
	adcs	x20, x20, x24
	umulh	x24, x3, x8
	mul		x5, x3, x1
	adds		x24, x24, x29
	umulh	x29, x3, x13
	mul		x26, x3, x11
	adcs	x5, x29, x5
	umulh	x29, x3, x1
	mul		x28, x3, x16
	adcs	x26, x29, x26
	umulh	x29, x3, x11
	mul		x30, x3, x12
	adcs	x28, x29, x28
	umulh	x29, x3, x16
	adcs	x29, x29, x30
	umulh	x30, x3, x12
	mul		x3, x3, x8
	adcs	x30, x30, xzr
	adds		x3, x3, x22
	adcs	x24, x24, x15
	adcs	x5, x5, x25
	adcs	x23, x26, x23
	mov	 x17, x4
	adcs	x21, x28, x21
	mul		x28, x3, x18
	mov	 x4, x18
	ldr	x18, [sp, #8]           // 8-byte Folded Reload
	adcs	x20, x29, x20
	adcs	x30, x30, xzr
	mul		x26, x6, x11
	mul		x29, x28, x18
	cmn		x29, x3
	mul		x29, x28, x0
	adcs	x24, x29, x24
	mul		x29, x28, x17
	adcs	x5, x29, x5
	mul		x29, x28, x10
	adcs	x23, x29, x23
	mul		x29, x28, x9
	adcs	x21, x29, x21
	mul		x29, x28, x14
	adcs	x20, x29, x20
	umulh	x29, x28, x18
	adcs	x30, x30, xzr
	adds		x24, x24, x29
	umulh	x29, x28, x0
	adcs	x5, x5, x29
	umulh	x29, x28, x17
	adcs	x23, x23, x29
	umulh	x29, x28, x10
	adcs	x21, x21, x29
	umulh	x29, x28, x9
	adcs	x20, x20, x29
	umulh	x28, x28, x14
	mul		x29, x6, x13
	adcs	x28, x30, x28
	umulh	x30, x6, x8
	mul		x3, x6, x1
	adds		x29, x30, x29
	umulh	x30, x6, x13
	adcs	x3, x30, x3
	umulh	x30, x6, x1
	mul		x25, x6, x16
	adcs	x26, x30, x26
	umulh	x30, x6, x11
	mul		x27, x6, x12
	adcs	x25, x30, x25
	umulh	x30, x6, x16
	umulh	x22, x6, x12
	adcs	x27, x30, x27
	mul		x6, x6, x8
	adcs	x22, x22, xzr
	adds		x6, x6, x24
	adcs	x5, x29, x5
	adcs	x3, x3, x23
	adcs	x21, x26, x21
	adcs	x20, x25, x20
	mul		x25, x6, x4
	adcs	x27, x27, x28
	mul		x28, x25, x18
	adcs	x22, x22, xzr
	cmn		x28, x6
	mul		x28, x25, x0
	adcs	x5, x28, x5
	mul		x28, x25, x17
	adcs	x3, x28, x3
	mul		x28, x25, x10
	adcs	x21, x28, x21
	mul		x28, x25, x9
	adcs	x20, x28, x20
	mul		x28, x25, x14
	adcs	x27, x28, x27
	umulh	x28, x25, x18
	adcs	x22, x22, xzr
	adds		x5, x5, x28
	umulh	x28, x25, x0
	adcs	x3, x3, x28
	umulh	x28, x25, x17
	adcs	x21, x21, x28
	umulh	x28, x25, x10
	adcs	x20, x20, x28
	umulh	x28, x25, x9
	adcs	x27, x27, x28
	umulh	x25, x25, x14
	mul		x28, x7, x13
	adcs	x22, x22, x25
	umulh	x25, x7, x8
	mul		x6, x7, x1
	adds		x25, x25, x28
	umulh	x28, x7, x13
	mul		x26, x7, x11
	adcs	x6, x28, x6
	umulh	x28, x7, x1
	mul		x23, x7, x16
	adcs	x26, x28, x26
	umulh	x28, x7, x11
	mul		x24, x7, x12
	umulh	x29, x7, x16
	adcs	x23, x28, x23
	umulh	x30, x7, x12
	adcs	x24, x29, x24
	mul		x7, x7, x8
	adcs	x30, x30, xzr
	adds		x5, x7, x5
	adcs	x3, x25, x3
	umulh	x9, x19, x12
	adcs	x6, x6, x21
	umulh	x29, x2, x12
	str	x9, [sp, #16]           // 8-byte Folded Spill
	mul		x9, x19, x12
	adcs	x20, x26, x20
	str	x29, [sp, #88]          // 8-byte Folded Spill
	mul		x29, x2, x12
	umulh	x12, x2, x16
	adcs	x23, x23, x27
	str	x12, [sp, #80]          // 8-byte Folded Spill
	mul		x12, x2, x16
	umulh	x21, x19, x11
	mul		x26, x19, x11
	mul		x27, x5, x4
	adcs	x22, x24, x22
	mov	 x28, x1
	str	x12, [sp, #72]          // 8-byte Folded Spill
	umulh	x12, x2, x11
	mul		x11, x2, x11
	mul		x24, x27, x18
	adcs	x30, x30, xzr
	stp	x11, x12, [sp, #56]     // 8-byte Folded Spill
	umulh	x11, x2, x28
	str	x9, [sp, #32]           // 8-byte Folded Spill
	umulh	x7, x19, x16
	mul		x25, x19, x16
	cmn		x24, x5
	mul		x5, x19, x28
	mul		x24, x19, x13
	umulh	x1, x19, x8
	umulh	x9, x19, x13
	umulh	x15, x19, x28
	mul		x19, x19, x8
	str	x11, [sp, #48]          // 8-byte Folded Spill
	mul		x11, x2, x28
	umulh	x16, x2, x8
	mul		x28, x2, x8
	ldr	x8, [sp, #104]          // 8-byte Folded Reload
	str	x11, [sp, #40]          // 8-byte Folded Spill
	umulh	x11, x2, x13
	mul		x13, x2, x13
	mul		x2, x27, x0
	adcs	x2, x2, x3
	mul		x3, x27, x17
	adcs	x3, x3, x6
	mul		x6, x27, x10
	adcs	x6, x6, x20
	mul		x20, x27, x8
	adcs	x20, x20, x23
	mul		x23, x27, x14
	adcs	x22, x23, x22
	adcs	x23, x30, xzr
	umulh	x30, x27, x18
	adds		x2, x2, x30
	umulh	x30, x27, x0
	adcs	x3, x3, x30
	umulh	x30, x27, x17
	adcs	x6, x6, x30
	umulh	x30, x27, x10
	adcs	x20, x20, x30
	umulh	x30, x27, x8
	adcs	x22, x22, x30
	mov	 x30, x14
	umulh	x27, x27, x30
	adcs	x23, x23, x27
	str	x11, [sp, #24]          // 8-byte Folded Spill
	mov	 x11, x8
	adds		x8, x1, x24
	adcs	x9, x9, x5
	adcs	x14, x15, x26
	ldr	x15, [sp, #32]          // 8-byte Folded Reload
	adcs	x5, x21, x25
	mov	 x24, x4
	mov	 x12, x17
	adcs	x7, x7, x15
	ldr	x15, [sp, #16]          // 8-byte Folded Reload
	mov	 x27, x11
	adcs	x21, x15, xzr
	adds		x2, x19, x2
	adcs	x8, x8, x3
	adcs	x9, x9, x6
	adcs	x14, x14, x20
	adcs	x5, x5, x22
	mul		x3, x2, x24
	adcs	x7, x7, x23
	mul		x20, x3, x18
	adcs	x21, x21, xzr
	cmn		x20, x2
	mul		x20, x3, x0
	adcs	x8, x20, x8
	mul		x20, x3, x12
	mul		x2, x3, x10
	adcs	x9, x20, x9
	mul		x19, x3, x11
	adcs	x14, x2, x14
	mul		x6, x3, x30
	adcs	x5, x19, x5
	adcs	x6, x6, x7
	umulh	x2, x3, x11
	mov	 x11, x10
	umulh	x7, x3, x18
	adcs	x21, x21, xzr
	umulh	x20, x3, x30
	umulh	x19, x3, x11
	adds		x8, x8, x7
	umulh	x7, x3, x12
	umulh	x3, x3, x0
	adcs	x9, x9, x3
	adcs	x10, x14, x7
	adcs	x3, x5, x19
	adcs	x2, x6, x2
	adcs	x5, x21, x20
	adds		x15, x16, x13
	ldr	x13, [sp, #40]          // 8-byte Folded Reload
	ldr	x14, [sp, #24]          // 8-byte Folded Reload
	adcs	x16, x14, x13
	ldp	x14, x13, [sp, #48]     // 8-byte Folded Reload
	adcs	x17, x14, x13
	ldp	x14, x13, [sp, #64]     // 8-byte Folded Reload
	adcs	x1, x14, x13
	ldr	x13, [sp, #80]          // 8-byte Folded Reload
	adcs	x4, x13, x29
	ldr	x13, [sp, #88]          // 8-byte Folded Reload
	adcs	x6, x13, xzr
	adds		x8, x28, x8
	adcs	x9, x15, x9
	adcs	x10, x16, x10
	adcs	x17, x17, x3
	adcs	x1, x1, x2
	mul		x15, x8, x24
	adcs	x2, x4, x5
	mul		x21, x15, x18
	adcs	x3, x6, xzr
	mul		x20, x15, x0
	cmn		x21, x8
	mul		x19, x15, x12
	adcs	x8, x20, x9
	mul		x7, x15, x11
	adcs	x9, x19, x10
	mul		x14, x15, x27
	adcs	x10, x7, x17
	mul		x16, x15, x30
	adcs	x17, x14, x1
	adcs	x16, x16, x2
	umulh	x22, x15, x30
	umulh	x23, x15, x27
	umulh	x24, x15, x11
	mov	 x28, x11
	umulh	x25, x15, x12
	umulh	x26, x15, x0
	umulh	x15, x15, x18
	adcs	x11, x3, xzr
	adds		x8, x8, x15
	adcs	x9, x9, x26
	adcs	x10, x10, x25
	adcs	x15, x17, x24
	adcs	x16, x16, x23
	adcs	x17, x11, x22
	subs		x3, x8, x18
	sbcs	x2, x9, x0
	sbcs	x11, x10, x12
	sbcs	x14, x15, x28
	sbcs	x18, x16, x27
	sbcs	x0, x17, x30
	asr	x1, x0, #63
	cmp		x1, #0          // =0
	csel	x10, x10, x11, lt
	csel	x11, x15, x14, lt
	ldr	x14, [sp, #96]          // 8-byte Folded Reload
	csel	x8, x8, x3, lt
	csel	x9, x9, x2, lt
	csel	x12, x16, x18, lt
	csel	x13, x17, x0, lt
	stp		x8, x9, [x14]
	stp	x10, x11, [x14, #16]
	stp	x12, x13, [x14, #32]
	ldp	x29, x30, [sp, #192]    // 8-byte Folded Reload
	ldp	x20, x19, [sp, #176]    // 8-byte Folded Reload
	ldp	x22, x21, [sp, #160]    // 8-byte Folded Reload
	ldp	x24, x23, [sp, #144]    // 8-byte Folded Reload
	ldp	x26, x25, [sp, #128]    // 8-byte Folded Reload
	ldp	x28, x27, [sp, #112]    // 8-byte Folded Reload
	add	sp, sp, #208            // =208
	ret
.Lfunc_end44:
	.size	mcl_fp_montNF6L, .Lfunc_end44-mcl_fp_montNF6L

	.globl	mcl_fp_montRed6L
	.p2align	2
	.type	mcl_fp_montRed6L,@function
mcl_fp_montRed6L:                       // @mcl_fp_montRed6L
// BB#0:
	str	x27, [sp, #-80]!        // 8-byte Folded Spill
	ldp	x12, x14, [x2, #-8]
	ldp		x3, x4, [x1]
	ldr	x13, [x2, #8]
	ldp	x11, x10, [x2, #16]
	stp	x20, x19, [sp, #64]     // 8-byte Folded Spill
	ldp	x9, x8, [x2, #32]
	mul		x20, x3, x12
	stp	x26, x25, [sp, #16]     // 8-byte Folded Spill
	mul		x26, x20, x13
	umulh	x27, x20, x14
	mul		x25, x20, x11
	adds		x26, x27, x26
	umulh	x27, x20, x13
	stp	x24, x23, [sp, #32]     // 8-byte Folded Spill
	mul		x24, x20, x10
	adcs	x25, x27, x25
	umulh	x27, x20, x11
	mul		x23, x20, x9
	adcs	x24, x27, x24
	umulh	x27, x20, x10
	stp	x22, x21, [sp, #48]     // 8-byte Folded Spill
	mul		x22, x20, x8
	adcs	x23, x27, x23
	umulh	x27, x20, x9
	ldp	x2, x18, [x1, #16]
	umulh	x21, x20, x8
	adcs	x22, x27, x22
	mul		x20, x20, x14
	adcs	x21, x21, xzr
	ldp	x16, x15, [x1, #32]
	cmn		x20, x3
	adcs	x3, x26, x4
	ldp	x17, x5, [x1, #48]
	adcs	x2, x25, x2
	adcs	x18, x24, x18
	adcs	x16, x23, x16
	adcs	x15, x22, x15
	mul		x4, x3, x12
	adcs	x17, x21, x17
	umulh	x23, x4, x14
	mul		x22, x4, x13
	adcs	x21, xzr, xzr
	umulh	x27, x4, x13
	adds		x22, x22, x23
	mul		x23, x4, x11
	umulh	x26, x4, x11
	adcs	x23, x23, x27
	mul		x27, x4, x10
	umulh	x25, x4, x10
	adcs	x26, x27, x26
	mul		x27, x4, x9
	umulh	x24, x4, x9
	adcs	x25, x27, x25
	mul		x27, x4, x8
	umulh	x20, x4, x8
	adcs	x24, x27, x24
	mul		x4, x4, x14
	adcs	x20, x21, x20
	cmn		x4, x3
	adcs	x2, x22, x2
	adcs	x18, x23, x18
	adcs	x16, x26, x16
	adcs	x15, x25, x15
	adcs	x17, x24, x17
	mul		x3, x2, x12
	adcs	x5, x20, x5
	umulh	x25, x3, x14
	mul		x24, x3, x13
	adcs	x20, xzr, xzr
	umulh	x27, x3, x13
	adds		x24, x24, x25
	mul		x25, x3, x11
	umulh	x26, x3, x11
	adcs	x25, x25, x27
	mul		x27, x3, x10
	umulh	x23, x3, x10
	adcs	x26, x27, x26
	mul		x27, x3, x9
	umulh	x21, x3, x9
	mul		x22, x3, x8
	adcs	x23, x27, x23
	umulh	x4, x3, x8
	adcs	x21, x22, x21
	mul		x3, x3, x14
	adcs	x4, x20, x4
	cmn		x3, x2
	adcs	x18, x24, x18
	ldp	x6, x7, [x1, #64]
	adcs	x16, x25, x16
	adcs	x15, x26, x15
	adcs	x17, x23, x17
	adcs	x5, x21, x5
	mul		x2, x18, x12
	adcs	x4, x4, x6
	umulh	x23, x2, x14
	mul		x21, x2, x13
	adcs	x6, xzr, xzr
	umulh	x27, x2, x13
	adds		x21, x21, x23
	mul		x23, x2, x11
	umulh	x26, x2, x11
	adcs	x23, x23, x27
	mul		x27, x2, x10
	umulh	x24, x2, x10
	mul		x25, x2, x9
	adcs	x26, x27, x26
	umulh	x20, x2, x9
	mul		x22, x2, x8
	adcs	x24, x25, x24
	umulh	x3, x2, x8
	adcs	x20, x22, x20
	mul		x2, x2, x14
	adcs	x3, x6, x3
	cmn		x2, x18
	adcs	x16, x21, x16
	adcs	x15, x23, x15
	adcs	x17, x26, x17
	adcs	x5, x24, x5
	adcs	x4, x20, x4
	mul		x18, x16, x12
	adcs	x3, x3, x7
	umulh	x24, x18, x14
	mul		x20, x18, x13
	adcs	x7, xzr, xzr
	umulh	x27, x18, x13
	adds		x20, x20, x24
	mul		x24, x18, x11
	umulh	x25, x18, x11
	mul		x26, x18, x10
	adcs	x24, x24, x27
	umulh	x22, x18, x10
	mul		x23, x18, x9
	adcs	x25, x26, x25
	umulh	x6, x18, x9
	mul		x21, x18, x8
	adcs	x22, x23, x22
	umulh	x2, x18, x8
	adcs	x6, x21, x6
	mul		x18, x18, x14
	adcs	x2, x7, x2
	cmn		x18, x16
	adcs	x15, x20, x15
	ldp	x19, x1, [x1, #80]
	adcs	x16, x24, x17
	adcs	x18, x25, x5
	adcs	x4, x22, x4
	adcs	x3, x6, x3
	mul		x12, x15, x12
	adcs	x2, x2, x19
	umulh	x27, x12, x14
	mul		x22, x12, x13
	adcs	x6, xzr, xzr
	umulh	x25, x12, x13
	mul		x26, x12, x11
	adds		x19, x22, x27
	umulh	x23, x12, x11
	mul		x24, x12, x10
	adcs	x22, x26, x25
	umulh	x20, x12, x10
	mul		x21, x12, x9
	adcs	x23, x24, x23
	umulh	x5, x12, x9
	mul		x7, x12, x8
	adcs	x20, x21, x20
	umulh	x17, x12, x8
	adcs	x5, x7, x5
	mul		x12, x12, x14
	adcs	x17, x6, x17
	cmn		x12, x15
	adcs	x12, x19, x16
	adcs	x15, x22, x18
	adcs	x16, x23, x4
	adcs	x18, x20, x3
	adcs	x2, x5, x2
	adcs	x17, x17, x1
	subs		x14, x12, x14
	sbcs	x13, x15, x13
	sbcs	x11, x16, x11
	sbcs	x10, x18, x10
	sbcs	x9, x2, x9
	sbcs	x8, x17, x8
	ngcs	 x1, xzr
	ldp	x20, x19, [sp, #64]     // 8-byte Folded Reload
	ldp	x22, x21, [sp, #48]     // 8-byte Folded Reload
	ldp	x24, x23, [sp, #32]     // 8-byte Folded Reload
	ldp	x26, x25, [sp, #16]     // 8-byte Folded Reload
	tst	 x1, #0x1
	csel	x12, x12, x14, ne
	csel	x13, x15, x13, ne
	csel	x11, x16, x11, ne
	csel	x10, x18, x10, ne
	csel	x9, x2, x9, ne
	csel	x8, x17, x8, ne
	stp		x12, x13, [x0]
	stp	x11, x10, [x0, #16]
	stp	x9, x8, [x0, #32]
	ldr	x27, [sp], #80          // 8-byte Folded Reload
	ret
.Lfunc_end45:
	.size	mcl_fp_montRed6L, .Lfunc_end45-mcl_fp_montRed6L

	.globl	mcl_fp_montRedNF6L
	.p2align	2
	.type	mcl_fp_montRedNF6L,@function
mcl_fp_montRedNF6L:                     // @mcl_fp_montRedNF6L
// BB#0:
	str	x27, [sp, #-80]!        // 8-byte Folded Spill
	ldp	x12, x14, [x2, #-8]
	ldp		x3, x4, [x1]
	ldr	x13, [x2, #8]
	ldp	x11, x10, [x2, #16]
	stp	x20, x19, [sp, #64]     // 8-byte Folded Spill
	ldp	x9, x8, [x2, #32]
	mul		x20, x3, x12
	stp	x26, x25, [sp, #16]     // 8-byte Folded Spill
	mul		x26, x20, x13
	umulh	x27, x20, x14
	mul		x25, x20, x11
	adds		x26, x27, x26
	umulh	x27, x20, x13
	stp	x24, x23, [sp, #32]     // 8-byte Folded Spill
	mul		x24, x20, x10
	adcs	x25, x27, x25
	umulh	x27, x20, x11
	mul		x23, x20, x9
	adcs	x24, x27, x24
	umulh	x27, x20, x10
	stp	x22, x21, [sp, #48]     // 8-byte Folded Spill
	mul		x22, x20, x8
	adcs	x23, x27, x23
	umulh	x27, x20, x9
	ldp	x2, x18, [x1, #16]
	umulh	x21, x20, x8
	adcs	x22, x27, x22
	mul		x20, x20, x14
	adcs	x21, x21, xzr
	ldp	x16, x15, [x1, #32]
	cmn		x20, x3
	adcs	x3, x26, x4
	ldp	x17, x5, [x1, #48]
	adcs	x2, x25, x2
	adcs	x18, x24, x18
	adcs	x16, x23, x16
	adcs	x15, x22, x15
	mul		x4, x3, x12
	adcs	x17, x21, x17
	umulh	x23, x4, x14
	mul		x22, x4, x13
	adcs	x21, xzr, xzr
	umulh	x27, x4, x13
	adds		x22, x22, x23
	mul		x23, x4, x11
	umulh	x26, x4, x11
	adcs	x23, x23, x27
	mul		x27, x4, x10
	umulh	x25, x4, x10
	adcs	x26, x27, x26
	mul		x27, x4, x9
	umulh	x24, x4, x9
	adcs	x25, x27, x25
	mul		x27, x4, x8
	umulh	x20, x4, x8
	adcs	x24, x27, x24
	mul		x4, x4, x14
	adcs	x20, x21, x20
	cmn		x4, x3
	adcs	x2, x22, x2
	adcs	x18, x23, x18
	adcs	x16, x26, x16
	adcs	x15, x25, x15
	adcs	x17, x24, x17
	mul		x3, x2, x12
	adcs	x5, x20, x5
	umulh	x25, x3, x14
	mul		x24, x3, x13
	adcs	x20, xzr, xzr
	umulh	x27, x3, x13
	adds		x24, x24, x25
	mul		x25, x3, x11
	umulh	x26, x3, x11
	adcs	x25, x25, x27
	mul		x27, x3, x10
	umulh	x23, x3, x10
	adcs	x26, x27, x26
	mul		x27, x3, x9
	umulh	x21, x3, x9
	mul		x22, x3, x8
	adcs	x23, x27, x23
	umulh	x4, x3, x8
	adcs	x21, x22, x21
	mul		x3, x3, x14
	adcs	x4, x20, x4
	cmn		x3, x2
	adcs	x18, x24, x18
	ldp	x6, x7, [x1, #64]
	adcs	x16, x25, x16
	adcs	x15, x26, x15
	adcs	x17, x23, x17
	adcs	x5, x21, x5
	mul		x2, x18, x12
	adcs	x4, x4, x6
	umulh	x23, x2, x14
	mul		x21, x2, x13
	adcs	x6, xzr, xzr
	umulh	x27, x2, x13
	adds		x21, x21, x23
	mul		x23, x2, x11
	umulh	x26, x2, x11
	adcs	x23, x23, x27
	mul		x27, x2, x10
	umulh	x24, x2, x10
	mul		x25, x2, x9
	adcs	x26, x27, x26
	umulh	x20, x2, x9
	mul		x22, x2, x8
	adcs	x24, x25, x24
	umulh	x3, x2, x8
	adcs	x20, x22, x20
	mul		x2, x2, x14
	adcs	x3, x6, x3
	cmn		x2, x18
	adcs	x16, x21, x16
	adcs	x15, x23, x15
	adcs	x17, x26, x17
	adcs	x5, x24, x5
	adcs	x4, x20, x4
	mul		x18, x16, x12
	adcs	x3, x3, x7
	umulh	x24, x18, x14
	mul		x20, x18, x13
	adcs	x7, xzr, xzr
	umulh	x27, x18, x13
	adds		x20, x20, x24
	mul		x24, x18, x11
	umulh	x25, x18, x11
	mul		x26, x18, x10
	adcs	x24, x24, x27
	umulh	x22, x18, x10
	mul		x23, x18, x9
	adcs	x25, x26, x25
	umulh	x6, x18, x9
	mul		x21, x18, x8
	adcs	x22, x23, x22
	umulh	x2, x18, x8
	adcs	x6, x21, x6
	mul		x18, x18, x14
	adcs	x2, x7, x2
	cmn		x18, x16
	adcs	x15, x20, x15
	ldp	x19, x1, [x1, #80]
	adcs	x16, x24, x17
	adcs	x18, x25, x5
	adcs	x4, x22, x4
	adcs	x3, x6, x3
	mul		x12, x15, x12
	adcs	x2, x2, x19
	umulh	x27, x12, x14
	mul		x22, x12, x13
	adcs	x6, xzr, xzr
	umulh	x25, x12, x13
	mul		x26, x12, x11
	adds		x19, x22, x27
	umulh	x23, x12, x11
	mul		x24, x12, x10
	adcs	x22, x26, x25
	umulh	x20, x12, x10
	mul		x21, x12, x9
	adcs	x23, x24, x23
	umulh	x5, x12, x9
	mul		x7, x12, x8
	adcs	x20, x21, x20
	umulh	x17, x12, x8
	adcs	x5, x7, x5
	mul		x12, x12, x14
	adcs	x17, x6, x17
	cmn		x12, x15
	adcs	x12, x19, x16
	adcs	x15, x22, x18
	adcs	x16, x23, x4
	adcs	x18, x20, x3
	adcs	x2, x5, x2
	adcs	x17, x17, x1
	subs		x14, x12, x14
	sbcs	x13, x15, x13
	sbcs	x11, x16, x11
	sbcs	x10, x18, x10
	sbcs	x9, x2, x9
	sbcs	x8, x17, x8
	asr	x1, x8, #63
	ldp	x20, x19, [sp, #64]     // 8-byte Folded Reload
	ldp	x22, x21, [sp, #48]     // 8-byte Folded Reload
	ldp	x24, x23, [sp, #32]     // 8-byte Folded Reload
	ldp	x26, x25, [sp, #16]     // 8-byte Folded Reload
	cmp		x1, #0          // =0
	csel	x12, x12, x14, lt
	csel	x13, x15, x13, lt
	csel	x11, x16, x11, lt
	csel	x10, x18, x10, lt
	csel	x9, x2, x9, lt
	csel	x8, x17, x8, lt
	stp		x12, x13, [x0]
	stp	x11, x10, [x0, #16]
	stp	x9, x8, [x0, #32]
	ldr	x27, [sp], #80          // 8-byte Folded Reload
	ret
.Lfunc_end46:
	.size	mcl_fp_montRedNF6L, .Lfunc_end46-mcl_fp_montRedNF6L

	.globl	mcl_fp_addPre6L
	.p2align	2
	.type	mcl_fp_addPre6L,@function
mcl_fp_addPre6L:                        // @mcl_fp_addPre6L
// BB#0:
	ldp		x14, x15, [x2]
	ldp		x16, x17, [x1]
	ldp	x10, x11, [x1, #32]
	ldp	x12, x13, [x2, #16]
	ldp	x18, x1, [x1, #16]
	ldp	x8, x9, [x2, #32]
	adds		x14, x14, x16
	adcs	x15, x15, x17
	adcs	x12, x12, x18
	adcs	x13, x13, x1
	adcs	x10, x8, x10
	adcs	x9, x9, x11
	adcs	x8, xzr, xzr
	stp		x14, x15, [x0]
	stp	x12, x13, [x0, #16]
	stp	x10, x9, [x0, #32]
	mov	 x0, x8
	ret
.Lfunc_end47:
	.size	mcl_fp_addPre6L, .Lfunc_end47-mcl_fp_addPre6L

	.globl	mcl_fp_subPre6L
	.p2align	2
	.type	mcl_fp_subPre6L,@function
mcl_fp_subPre6L:                        // @mcl_fp_subPre6L
// BB#0:
	ldp		x14, x15, [x2]
	ldp		x16, x17, [x1]
	ldp	x10, x11, [x1, #32]
	ldp	x12, x13, [x2, #16]
	ldp	x18, x1, [x1, #16]
	ldp	x8, x9, [x2, #32]
	subs		x14, x16, x14
	sbcs	x15, x17, x15
	sbcs	x12, x18, x12
	sbcs	x13, x1, x13
	sbcs	x10, x10, x8
	sbcs	x9, x11, x9
	ngcs	 x8, xzr
	and	x8, x8, #0x1
	stp		x14, x15, [x0]
	stp	x12, x13, [x0, #16]
	stp	x10, x9, [x0, #32]
	mov	 x0, x8
	ret
.Lfunc_end48:
	.size	mcl_fp_subPre6L, .Lfunc_end48-mcl_fp_subPre6L

	.globl	mcl_fp_shr1_6L
	.p2align	2
	.type	mcl_fp_shr1_6L,@function
mcl_fp_shr1_6L:                         // @mcl_fp_shr1_6L
// BB#0:
	ldp		x8, x9, [x1]
	ldp	x10, x11, [x1, #16]
	ldp	x12, x13, [x1, #32]
	extr	x8, x9, x8, #1
	extr	x9, x10, x9, #1
	extr	x10, x11, x10, #1
	extr	x11, x12, x11, #1
	extr	x12, x13, x12, #1
	lsr	x13, x13, #1
	stp		x8, x9, [x0]
	stp	x10, x11, [x0, #16]
	stp	x12, x13, [x0, #32]
	ret
.Lfunc_end49:
	.size	mcl_fp_shr1_6L, .Lfunc_end49-mcl_fp_shr1_6L

	.globl	mcl_fp_add6L
	.p2align	2
	.type	mcl_fp_add6L,@function
mcl_fp_add6L:                           // @mcl_fp_add6L
// BB#0:
	ldp		x14, x15, [x2]
	ldp		x16, x17, [x1]
	ldp	x10, x11, [x1, #32]
	ldp	x12, x13, [x2, #16]
	ldp	x18, x1, [x1, #16]
	ldp	x8, x9, [x2, #32]
	adds		x14, x14, x16
	adcs	x15, x15, x17
	adcs	x18, x12, x18
	adcs	x1, x13, x1
	adcs	x5, x8, x10
	ldp		x8, x10, [x3]
	ldp	x16, x17, [x3, #32]
	ldp	x2, x4, [x3, #16]
	adcs	x3, x9, x11
	adcs	x6, xzr, xzr
	subs		x13, x14, x8
	sbcs	x12, x15, x10
	sbcs	x11, x18, x2
	sbcs	x10, x1, x4
	sbcs	x9, x5, x16
	sbcs	x8, x3, x17
	stp		x14, x15, [x0]
	sbcs	x14, x6, xzr
	stp	x18, x1, [x0, #16]
	stp	x5, x3, [x0, #32]
	tbnz	w14, #0, .LBB50_2
// BB#1:                                // %nocarry
	stp		x13, x12, [x0]
	stp	x11, x10, [x0, #16]
	stp	x9, x8, [x0, #32]
.LBB50_2:                               // %carry
	ret
.Lfunc_end50:
	.size	mcl_fp_add6L, .Lfunc_end50-mcl_fp_add6L

	.globl	mcl_fp_addNF6L
	.p2align	2
	.type	mcl_fp_addNF6L,@function
mcl_fp_addNF6L:                         // @mcl_fp_addNF6L
// BB#0:
	ldp		x14, x15, [x1]
	ldp		x16, x17, [x2]
	ldp	x8, x9, [x1, #32]
	ldp	x12, x13, [x1, #16]
	ldp	x18, x1, [x2, #16]
	adds		x14, x16, x14
	ldp	x10, x11, [x2, #32]
	adcs	x15, x17, x15
	adcs	x12, x18, x12
	adcs	x13, x1, x13
	ldp		x18, x1, [x3]
	adcs	x8, x10, x8
	ldp	x10, x2, [x3, #16]
	adcs	x9, x11, x9
	ldp	x16, x17, [x3, #32]
	subs		x11, x14, x18
	sbcs	x18, x15, x1
	sbcs	x10, x12, x10
	sbcs	x1, x13, x2
	sbcs	x16, x8, x16
	sbcs	x17, x9, x17
	asr	x2, x17, #63
	cmp		x2, #0          // =0
	csel	x11, x14, x11, lt
	csel	x14, x15, x18, lt
	csel	x10, x12, x10, lt
	csel	x12, x13, x1, lt
	csel	x8, x8, x16, lt
	csel	x9, x9, x17, lt
	stp		x11, x14, [x0]
	stp	x10, x12, [x0, #16]
	stp	x8, x9, [x0, #32]
	ret
.Lfunc_end51:
	.size	mcl_fp_addNF6L, .Lfunc_end51-mcl_fp_addNF6L

	.globl	mcl_fp_sub6L
	.p2align	2
	.type	mcl_fp_sub6L,@function
mcl_fp_sub6L:                           // @mcl_fp_sub6L
// BB#0:
	ldp		x8, x9, [x2]
	ldp		x16, x17, [x1]
	ldp	x14, x15, [x1, #32]
	ldp	x10, x11, [x2, #16]
	ldp	x18, x1, [x1, #16]
	ldp	x12, x13, [x2, #32]
	subs		x8, x16, x8
	sbcs	x9, x17, x9
	sbcs	x10, x18, x10
	sbcs	x11, x1, x11
	sbcs	x12, x14, x12
	sbcs	x13, x15, x13
	ngcs	 x14, xzr
	stp		x8, x9, [x0]
	stp	x10, x11, [x0, #16]
	stp	x12, x13, [x0, #32]
	tbnz	w14, #0, .LBB52_2
// BB#1:                                // %nocarry
	ret
.LBB52_2:                               // %carry
	ldp		x16, x17, [x3]
	ldp	x18, x1, [x3, #16]
	ldp	x14, x15, [x3, #32]
	adds		x8, x16, x8
	adcs	x9, x17, x9
	stp		x8, x9, [x0]
	adcs	x8, x18, x10
	adcs	x9, x1, x11
	stp	x8, x9, [x0, #16]
	adcs	x8, x14, x12
	adcs	x9, x15, x13
	stp	x8, x9, [x0, #32]
	ret
.Lfunc_end52:
	.size	mcl_fp_sub6L, .Lfunc_end52-mcl_fp_sub6L

	.globl	mcl_fp_subNF6L
	.p2align	2
	.type	mcl_fp_subNF6L,@function
mcl_fp_subNF6L:                         // @mcl_fp_subNF6L
// BB#0:
	ldp		x14, x15, [x2]
	ldp		x16, x17, [x1]
	ldp	x10, x11, [x1, #32]
	ldp	x12, x13, [x2, #16]
	ldp	x18, x1, [x1, #16]
	ldp	x8, x9, [x2, #32]
	subs		x14, x16, x14
	sbcs	x15, x17, x15
	sbcs	x12, x18, x12
	sbcs	x13, x1, x13
	ldp	x16, x17, [x3, #32]
	ldp		x18, x1, [x3]
	sbcs	x8, x10, x8
	ldp	x10, x2, [x3, #16]
	sbcs	x9, x11, x9
	asr	x11, x9, #63
	and	x1, x1, x11, ror #63
	and		x10, x11, x10
	and		x2, x11, x2
	and		x16, x11, x16
	and		x17, x11, x17
	extr	x11, x11, x9, #63
	and		x11, x11, x18
	adds		x11, x11, x14
	adcs	x14, x1, x15
	adcs	x10, x10, x12
	stp		x11, x14, [x0]
	adcs	x11, x2, x13
	adcs	x8, x16, x8
	adcs	x9, x17, x9
	stp	x10, x11, [x0, #16]
	stp	x8, x9, [x0, #32]
	ret
.Lfunc_end53:
	.size	mcl_fp_subNF6L, .Lfunc_end53-mcl_fp_subNF6L

	.globl	mcl_fpDbl_add6L
	.p2align	2
	.type	mcl_fpDbl_add6L,@function
mcl_fpDbl_add6L:                        // @mcl_fpDbl_add6L
// BB#0:
	str	x25, [sp, #-64]!        // 8-byte Folded Spill
	stp	x24, x23, [sp, #16]     // 8-byte Folded Spill
	stp	x22, x21, [sp, #32]     // 8-byte Folded Spill
	stp	x20, x19, [sp, #48]     // 8-byte Folded Spill
	ldp	x8, x9, [x2, #80]
	ldp	x12, x13, [x2, #64]
	ldp	x16, x17, [x2, #48]
	ldp	x5, x6, [x2, #32]
	ldp	x20, x21, [x2, #16]
	ldp		x22, x2, [x2]
	ldp		x23, x24, [x1]
	ldp	x10, x11, [x1, #80]
	ldp	x14, x15, [x1, #64]
	ldp	x18, x4, [x1, #48]
	ldp	x7, x19, [x1, #32]
	ldp	x25, x1, [x1, #16]
	adds		x22, x22, x23
	adcs	x2, x2, x24
	ldp	x23, x24, [x3, #32]
	adcs	x20, x20, x25
	adcs	x1, x21, x1
	stp	x20, x1, [x0, #16]
	adcs	x1, x5, x7
	adcs	x5, x6, x19
	adcs	x16, x16, x18
	adcs	x17, x17, x4
	adcs	x12, x12, x14
	stp		x22, x2, [x0]
	ldp	x2, x22, [x3, #16]
	ldp		x25, x3, [x3]
	adcs	x13, x13, x15
	adcs	x8, x8, x10
	adcs	x9, x9, x11
	adcs	x10, xzr, xzr
	subs		x11, x16, x25
	sbcs	x14, x17, x3
	sbcs	x15, x12, x2
	sbcs	x18, x13, x22
	stp	x1, x5, [x0, #32]
	sbcs	x1, x8, x23
	sbcs	x2, x9, x24
	sbcs	x10, x10, xzr
	ldp	x20, x19, [sp, #48]     // 8-byte Folded Reload
	ldp	x22, x21, [sp, #32]     // 8-byte Folded Reload
	ldp	x24, x23, [sp, #16]     // 8-byte Folded Reload
	tst	 x10, #0x1
	csel	x10, x16, x11, ne
	csel	x11, x17, x14, ne
	csel	x12, x12, x15, ne
	csel	x13, x13, x18, ne
	csel	x8, x8, x1, ne
	csel	x9, x9, x2, ne
	stp	x10, x11, [x0, #48]
	stp	x12, x13, [x0, #64]
	stp	x8, x9, [x0, #80]
	ldr	x25, [sp], #64          // 8-byte Folded Reload
	ret
.Lfunc_end54:
	.size	mcl_fpDbl_add6L, .Lfunc_end54-mcl_fpDbl_add6L

	.globl	mcl_fpDbl_sub6L
	.p2align	2
	.type	mcl_fpDbl_sub6L,@function
mcl_fpDbl_sub6L:                        // @mcl_fpDbl_sub6L
// BB#0:
	str	x25, [sp, #-64]!        // 8-byte Folded Spill
	stp	x24, x23, [sp, #16]     // 8-byte Folded Spill
	stp	x22, x21, [sp, #32]     // 8-byte Folded Spill
	stp	x20, x19, [sp, #48]     // 8-byte Folded Spill
	ldp	x8, x9, [x2, #80]
	ldp	x12, x13, [x2, #64]
	ldp	x16, x17, [x2, #48]
	ldp	x5, x6, [x2, #32]
	ldp	x20, x21, [x2, #16]
	ldp		x22, x2, [x2]
	ldp		x23, x24, [x1]
	ldp	x10, x11, [x1, #80]
	ldp	x14, x15, [x1, #64]
	ldp	x18, x4, [x1, #48]
	ldp	x7, x19, [x1, #32]
	ldp	x25, x1, [x1, #16]
	subs		x22, x23, x22
	sbcs	x2, x24, x2
	ldp	x23, x24, [x3, #32]
	sbcs	x20, x25, x20
	sbcs	x1, x1, x21
	stp	x20, x1, [x0, #16]
	sbcs	x1, x7, x5
	sbcs	x5, x19, x6
	sbcs	x16, x18, x16
	sbcs	x17, x4, x17
	sbcs	x12, x14, x12
	sbcs	x13, x15, x13
	stp		x22, x2, [x0]
	ldp	x2, x22, [x3, #16]
	ldp		x25, x3, [x3]
	sbcs	x8, x10, x8
	sbcs	x9, x11, x9
	ngcs	 x10, xzr
	tst	 x10, #0x1
	stp	x1, x5, [x0, #32]
	csel	x1, x25, xzr, ne
	csel	x10, x24, xzr, ne
	csel	x11, x23, xzr, ne
	csel	x14, x22, xzr, ne
	csel	x15, x2, xzr, ne
	csel	x18, x3, xzr, ne
	adds		x16, x1, x16
	adcs	x17, x18, x17
	adcs	x12, x15, x12
	adcs	x13, x14, x13
	ldp	x20, x19, [sp, #48]     // 8-byte Folded Reload
	ldp	x22, x21, [sp, #32]     // 8-byte Folded Reload
	ldp	x24, x23, [sp, #16]     // 8-byte Folded Reload
	adcs	x8, x11, x8
	adcs	x9, x10, x9
	stp	x16, x17, [x0, #48]
	stp	x12, x13, [x0, #64]
	stp	x8, x9, [x0, #80]
	ldr	x25, [sp], #64          // 8-byte Folded Reload
	ret
.Lfunc_end55:
	.size	mcl_fpDbl_sub6L, .Lfunc_end55-mcl_fpDbl_sub6L

	.globl	mulPv512x64
	.p2align	2
	.type	mulPv512x64,@function
mulPv512x64:                            // @mulPv512x64
// BB#0:
	ldr		x9, [x0]
	mul		x10, x9, x1
	str		x10, [x8]
	ldr	x10, [x0, #8]
	umulh	x9, x9, x1
	mul		x11, x10, x1
	adds		x9, x9, x11
	str	x9, [x8, #8]
	ldr	x9, [x0, #16]
	umulh	x10, x10, x1
	mul		x11, x9, x1
	adcs	x10, x10, x11
	str	x10, [x8, #16]
	ldr	x10, [x0, #24]
	umulh	x9, x9, x1
	mul		x11, x10, x1
	adcs	x9, x9, x11
	str	x9, [x8, #24]
	ldr	x9, [x0, #32]
	umulh	x10, x10, x1
	mul		x11, x9, x1
	adcs	x10, x10, x11
	str	x10, [x8, #32]
	ldr	x10, [x0, #40]
	umulh	x9, x9, x1
	mul		x11, x10, x1
	adcs	x9, x9, x11
	str	x9, [x8, #40]
	ldr	x9, [x0, #48]
	umulh	x10, x10, x1
	mul		x11, x9, x1
	adcs	x10, x10, x11
	str	x10, [x8, #48]
	ldr	x10, [x0, #56]
	umulh	x9, x9, x1
	mul		x11, x10, x1
	umulh	x10, x10, x1
	adcs	x9, x9, x11
	adcs	x10, x10, xzr
	stp	x9, x10, [x8, #56]
	ret
.Lfunc_end56:
	.size	mulPv512x64, .Lfunc_end56-mulPv512x64

	.globl	mcl_fp_mulUnitPre8L
	.p2align	2
	.type	mcl_fp_mulUnitPre8L,@function
mcl_fp_mulUnitPre8L:                    // @mcl_fp_mulUnitPre8L
// BB#0:
	sub	sp, sp, #96             // =96
	stp	x19, x30, [sp, #80]     // 8-byte Folded Spill
	mov	 x19, x0
	mov	 x8, sp
	mov	 x0, x1
	mov	 x1, x2
	bl	mulPv512x64
	ldp	x9, x8, [sp, #56]
	ldp	x11, x10, [sp, #40]
	ldp	x16, x12, [sp, #24]
	ldp		x13, x14, [sp]
	ldr	x15, [sp, #16]
	stp	x10, x9, [x19, #48]
	stp	x12, x11, [x19, #32]
	stp		x13, x14, [x19]
	stp	x15, x16, [x19, #16]
	str	x8, [x19, #64]
	ldp	x19, x30, [sp, #80]     // 8-byte Folded Reload
	add	sp, sp, #96             // =96
	ret
.Lfunc_end57:
	.size	mcl_fp_mulUnitPre8L, .Lfunc_end57-mcl_fp_mulUnitPre8L

	.globl	mcl_fpDbl_mulPre8L
	.p2align	2
	.type	mcl_fpDbl_mulPre8L,@function
mcl_fpDbl_mulPre8L:                     // @mcl_fpDbl_mulPre8L
// BB#0:
	stp	x28, x27, [sp, #-96]!   // 8-byte Folded Spill
	stp	x26, x25, [sp, #16]     // 8-byte Folded Spill
	stp	x24, x23, [sp, #32]     // 8-byte Folded Spill
	stp	x22, x21, [sp, #48]     // 8-byte Folded Spill
	stp	x20, x19, [sp, #64]     // 8-byte Folded Spill
	stp	x29, x30, [sp, #80]     // 8-byte Folded Spill
	sub	sp, sp, #656            // =656
	mov	 x21, x2
	mov	 x20, x1
	ldr		x1, [x21]
	mov	 x19, x0
	add	x8, sp, #576            // =576
	mov	 x0, x20
	bl	mulPv512x64
	ldr	x8, [sp, #576]
	ldr	x1, [x21, #8]
	ldr	x22, [sp, #640]
	ldr	x23, [sp, #632]
	ldr	x24, [sp, #624]
	ldr	x25, [sp, #616]
	ldr	x26, [sp, #608]
	ldr	x27, [sp, #600]
	ldr	x28, [sp, #592]
	ldr	x29, [sp, #584]
	str		x8, [x19]
	add	x8, sp, #496            // =496
	mov	 x0, x20
	bl	mulPv512x64
	ldp	x13, x15, [sp, #496]
	ldr	x16, [sp, #512]
	ldr	x14, [sp, #520]
	ldr	x12, [sp, #528]
	adds		x13, x13, x29
	ldr	x11, [sp, #536]
	adcs	x28, x15, x28
	ldr	x10, [sp, #544]
	adcs	x27, x16, x27
	ldr	x9, [sp, #552]
	adcs	x26, x14, x26
	ldr	x8, [sp, #560]
	adcs	x25, x12, x25
	adcs	x24, x11, x24
	ldr	x1, [x21, #16]
	adcs	x23, x10, x23
	adcs	x22, x9, x22
	adcs	x29, x8, xzr
	add	x8, sp, #416            // =416
	mov	 x0, x20
	str	x13, [x19, #8]
	bl	mulPv512x64
	ldp	x13, x15, [sp, #416]
	ldr	x16, [sp, #432]
	ldp	x14, x12, [sp, #440]
	ldp	x11, x10, [sp, #456]
	adds		x13, x13, x28
	adcs	x27, x15, x27
	adcs	x26, x16, x26
	ldp	x9, x8, [sp, #472]
	adcs	x25, x14, x25
	adcs	x24, x12, x24
	adcs	x23, x11, x23
	ldr	x1, [x21, #24]
	adcs	x22, x10, x22
	adcs	x28, x9, x29
	adcs	x29, x8, xzr
	add	x8, sp, #336            // =336
	mov	 x0, x20
	str	x13, [x19, #16]
	bl	mulPv512x64
	ldp	x13, x15, [sp, #336]
	ldr	x16, [sp, #352]
	ldp	x14, x12, [sp, #360]
	ldp	x11, x10, [sp, #376]
	adds		x13, x13, x27
	adcs	x26, x15, x26
	adcs	x25, x16, x25
	ldp	x9, x8, [sp, #392]
	adcs	x24, x14, x24
	adcs	x23, x12, x23
	adcs	x22, x11, x22
	ldr	x1, [x21, #32]
	adcs	x27, x10, x28
	adcs	x28, x9, x29
	adcs	x29, x8, xzr
	add	x8, sp, #256            // =256
	mov	 x0, x20
	str	x13, [x19, #24]
	bl	mulPv512x64
	ldp	x13, x15, [sp, #256]
	ldr	x16, [sp, #272]
	ldp	x14, x12, [sp, #280]
	ldp	x11, x10, [sp, #296]
	adds		x13, x13, x26
	adcs	x25, x15, x25
	adcs	x24, x16, x24
	ldp	x9, x8, [sp, #312]
	adcs	x23, x14, x23
	adcs	x22, x12, x22
	adcs	x26, x11, x27
	ldr	x1, [x21, #40]
	adcs	x27, x10, x28
	adcs	x28, x9, x29
	adcs	x29, x8, xzr
	add	x8, sp, #176            // =176
	mov	 x0, x20
	str	x13, [x19, #32]
	bl	mulPv512x64
	ldp	x13, x15, [sp, #176]
	ldr	x16, [sp, #192]
	ldp	x14, x12, [sp, #200]
	ldp	x11, x10, [sp, #216]
	adds		x13, x13, x25
	adcs	x24, x15, x24
	adcs	x23, x16, x23
	ldp	x9, x8, [sp, #232]
	adcs	x22, x14, x22
	adcs	x25, x12, x26
	adcs	x26, x11, x27
	ldr	x1, [x21, #48]
	adcs	x27, x10, x28
	adcs	x28, x9, x29
	adcs	x29, x8, xzr
	add	x8, sp, #96             // =96
	mov	 x0, x20
	str	x13, [x19, #40]
	bl	mulPv512x64
	ldp	x13, x15, [sp, #96]
	ldr	x16, [sp, #112]
	ldp	x14, x12, [sp, #120]
	ldp	x11, x10, [sp, #136]
	adds		x13, x13, x24
	adcs	x23, x15, x23
	adcs	x22, x16, x22
	ldp	x9, x8, [sp, #152]
	adcs	x24, x14, x25
	adcs	x25, x12, x26
	adcs	x26, x11, x27
	ldr	x1, [x21, #56]
	adcs	x21, x10, x28
	adcs	x27, x9, x29
	adcs	x28, x8, xzr
	add	x8, sp, #16             // =16
	mov	 x0, x20
	str	x13, [x19, #48]
	bl	mulPv512x64
	ldp	x13, x14, [sp, #16]
	ldr	x16, [sp, #32]
	ldp	x15, x12, [sp, #40]
	ldp	x11, x10, [sp, #56]
	adds		x13, x13, x23
	adcs	x14, x14, x22
	ldp	x9, x8, [sp, #72]
	stp	x13, x14, [x19, #56]
	adcs	x13, x16, x24
	adcs	x14, x15, x25
	adcs	x12, x12, x26
	adcs	x11, x11, x21
	adcs	x10, x10, x27
	adcs	x9, x9, x28
	adcs	x8, x8, xzr
	stp	x13, x14, [x19, #72]
	stp	x12, x11, [x19, #88]
	stp	x10, x9, [x19, #104]
	str	x8, [x19, #120]
	add	sp, sp, #656            // =656
	ldp	x29, x30, [sp, #80]     // 8-byte Folded Reload
	ldp	x20, x19, [sp, #64]     // 8-byte Folded Reload
	ldp	x22, x21, [sp, #48]     // 8-byte Folded Reload
	ldp	x24, x23, [sp, #32]     // 8-byte Folded Reload
	ldp	x26, x25, [sp, #16]     // 8-byte Folded Reload
	ldp	x28, x27, [sp], #96     // 8-byte Folded Reload
	ret
.Lfunc_end58:
	.size	mcl_fpDbl_mulPre8L, .Lfunc_end58-mcl_fpDbl_mulPre8L

	.globl	mcl_fpDbl_sqrPre8L
	.p2align	2
	.type	mcl_fpDbl_sqrPre8L,@function
mcl_fpDbl_sqrPre8L:                     // @mcl_fpDbl_sqrPre8L
// BB#0:
	stp	x28, x27, [sp, #-96]!   // 8-byte Folded Spill
	stp	x26, x25, [sp, #16]     // 8-byte Folded Spill
	stp	x24, x23, [sp, #32]     // 8-byte Folded Spill
	stp	x22, x21, [sp, #48]     // 8-byte Folded Spill
	stp	x20, x19, [sp, #64]     // 8-byte Folded Spill
	stp	x29, x30, [sp, #80]     // 8-byte Folded Spill
	sub	sp, sp, #640            // =640
	mov	 x20, x1
	ldr		x1, [x20]
	mov	 x19, x0
	add	x8, sp, #560            // =560
	mov	 x0, x20
	bl	mulPv512x64
	ldr	x8, [sp, #560]
	ldr	x1, [x20, #8]
	ldr	x21, [sp, #624]
	ldr	x22, [sp, #616]
	ldr	x23, [sp, #608]
	ldr	x24, [sp, #600]
	ldr	x25, [sp, #592]
	ldr	x26, [sp, #584]
	ldr	x27, [sp, #576]
	ldr	x28, [sp, #568]
	str		x8, [x19]
	add	x8, sp, #480            // =480
	mov	 x0, x20
	bl	mulPv512x64
	ldp	x13, x15, [sp, #480]
	ldr	x16, [sp, #496]
	ldp	x14, x12, [sp, #504]
	ldr	x11, [sp, #520]
	adds		x13, x13, x28
	adcs	x27, x15, x27
	ldr	x10, [sp, #528]
	adcs	x26, x16, x26
	ldr	x9, [sp, #536]
	adcs	x25, x14, x25
	ldr	x8, [sp, #544]
	adcs	x24, x12, x24
	adcs	x23, x11, x23
	ldr	x1, [x20, #16]
	adcs	x22, x10, x22
	adcs	x21, x9, x21
	adcs	x28, x8, xzr
	add	x8, sp, #400            // =400
	mov	 x0, x20
	str	x13, [x19, #8]
	bl	mulPv512x64
	ldp	x13, x15, [sp, #400]
	ldr	x16, [sp, #416]
	ldp	x14, x12, [sp, #424]
	ldp	x11, x10, [sp, #440]
	adds		x13, x13, x27
	adcs	x26, x15, x26
	adcs	x25, x16, x25
	ldp	x9, x8, [sp, #456]
	adcs	x24, x14, x24
	adcs	x23, x12, x23
	adcs	x22, x11, x22
	ldr	x1, [x20, #24]
	adcs	x21, x10, x21
	adcs	x27, x9, x28
	adcs	x28, x8, xzr
	add	x8, sp, #320            // =320
	mov	 x0, x20
	str	x13, [x19, #16]
	bl	mulPv512x64
	ldp	x13, x15, [sp, #320]
	ldr	x16, [sp, #336]
	ldp	x14, x12, [sp, #344]
	ldp	x11, x10, [sp, #360]
	adds		x13, x13, x26
	adcs	x25, x15, x25
	adcs	x24, x16, x24
	ldp	x9, x8, [sp, #376]
	adcs	x23, x14, x23
	adcs	x22, x12, x22
	adcs	x21, x11, x21
	ldr	x1, [x20, #32]
	adcs	x26, x10, x27
	adcs	x27, x9, x28
	adcs	x28, x8, xzr
	add	x8, sp, #240            // =240
	mov	 x0, x20
	str	x13, [x19, #24]
	bl	mulPv512x64
	ldp	x13, x15, [sp, #240]
	ldr	x16, [sp, #256]
	ldp	x14, x12, [sp, #264]
	ldp	x11, x10, [sp, #280]
	adds		x13, x13, x25
	adcs	x24, x15, x24
	adcs	x23, x16, x23
	ldp	x9, x8, [sp, #296]
	adcs	x22, x14, x22
	adcs	x21, x12, x21
	adcs	x25, x11, x26
	ldr	x1, [x20, #40]
	adcs	x26, x10, x27
	adcs	x27, x9, x28
	adcs	x28, x8, xzr
	add	x8, sp, #160            // =160
	mov	 x0, x20
	str	x13, [x19, #32]
	bl	mulPv512x64
	ldp	x13, x15, [sp, #160]
	ldr	x16, [sp, #176]
	ldp	x14, x12, [sp, #184]
	ldp	x11, x10, [sp, #200]
	adds		x13, x13, x24
	adcs	x23, x15, x23
	adcs	x22, x16, x22
	ldp	x9, x8, [sp, #216]
	adcs	x21, x14, x21
	adcs	x24, x12, x25
	adcs	x25, x11, x26
	ldr	x1, [x20, #48]
	adcs	x26, x10, x27
	adcs	x27, x9, x28
	adcs	x28, x8, xzr
	add	x8, sp, #80             // =80
	mov	 x0, x20
	str	x13, [x19, #40]
	bl	mulPv512x64
	ldp	x13, x15, [sp, #80]
	ldr	x16, [sp, #96]
	ldp	x14, x12, [sp, #104]
	ldp	x11, x10, [sp, #120]
	adds		x13, x13, x23
	adcs	x22, x15, x22
	adcs	x21, x16, x21
	ldp	x9, x8, [sp, #136]
	adcs	x23, x14, x24
	adcs	x24, x12, x25
	adcs	x25, x11, x26
	ldr	x1, [x20, #56]
	adcs	x26, x10, x27
	adcs	x27, x9, x28
	adcs	x28, x8, xzr
	mov	 x8, sp
	mov	 x0, x20
	str	x13, [x19, #48]
	bl	mulPv512x64
	ldp		x13, x14, [sp]
	ldr	x16, [sp, #16]
	ldp	x15, x12, [sp, #24]
	ldp	x11, x10, [sp, #40]
	adds		x13, x13, x22
	adcs	x14, x14, x21
	ldp	x9, x8, [sp, #56]
	stp	x13, x14, [x19, #56]
	adcs	x13, x16, x23
	adcs	x14, x15, x24
	adcs	x12, x12, x25
	adcs	x11, x11, x26
	adcs	x10, x10, x27
	adcs	x9, x9, x28
	adcs	x8, x8, xzr
	stp	x13, x14, [x19, #72]
	stp	x12, x11, [x19, #88]
	stp	x10, x9, [x19, #104]
	str	x8, [x19, #120]
	add	sp, sp, #640            // =640
	ldp	x29, x30, [sp, #80]     // 8-byte Folded Reload
	ldp	x20, x19, [sp, #64]     // 8-byte Folded Reload
	ldp	x22, x21, [sp, #48]     // 8-byte Folded Reload
	ldp	x24, x23, [sp, #32]     // 8-byte Folded Reload
	ldp	x26, x25, [sp, #16]     // 8-byte Folded Reload
	ldp	x28, x27, [sp], #96     // 8-byte Folded Reload
	ret
.Lfunc_end59:
	.size	mcl_fpDbl_sqrPre8L, .Lfunc_end59-mcl_fpDbl_sqrPre8L

	.globl	mcl_fp_mont8L
	.p2align	2
	.type	mcl_fp_mont8L,@function
mcl_fp_mont8L:                          // @mcl_fp_mont8L
// BB#0:
	stp	x28, x27, [sp, #-96]!   // 8-byte Folded Spill
	stp	x26, x25, [sp, #16]     // 8-byte Folded Spill
	stp	x24, x23, [sp, #32]     // 8-byte Folded Spill
	stp	x22, x21, [sp, #48]     // 8-byte Folded Spill
	stp	x20, x19, [sp, #64]     // 8-byte Folded Spill
	stp	x29, x30, [sp, #80]     // 8-byte Folded Spill
	sub	sp, sp, #1424           // =1424
	mov	 x20, x3
	ldur	x19, [x20, #-8]
	mov	 x22, x2
	mov	 x25, x1
	add	x8, sp, #1344           // =1344
	str	x19, [sp, #136]         // 8-byte Folded Spill
	ldr		x1, [x22]
	str	x0, [sp, #112]          // 8-byte Folded Spill
	mov	 x0, x25
	str	x25, [sp, #128]         // 8-byte Folded Spill
	bl	mulPv512x64
	ldr	x8, [sp, #1408]
	ldr	x24, [sp, #1344]
	mov	 x0, x20
	str	x8, [sp, #104]          // 8-byte Folded Spill
	ldr	x8, [sp, #1400]
	mul		x1, x24, x19
	str	x8, [sp, #96]           // 8-byte Folded Spill
	ldr	x8, [sp, #1392]
	str	x8, [sp, #88]           // 8-byte Folded Spill
	ldr	x8, [sp, #1384]
	str	x8, [sp, #80]           // 8-byte Folded Spill
	ldr	x8, [sp, #1376]
	str	x8, [sp, #72]           // 8-byte Folded Spill
	ldr	x8, [sp, #1368]
	str	x8, [sp, #64]           // 8-byte Folded Spill
	ldr	x8, [sp, #1360]
	str	x8, [sp, #56]           // 8-byte Folded Spill
	ldr	x8, [sp, #1352]
	str	x8, [sp, #48]           // 8-byte Folded Spill
	add	x8, sp, #1264           // =1264
	bl	mulPv512x64
	ldr	x8, [sp, #1328]
	ldr	x26, [sp, #1312]
	ldr	x27, [sp, #1304]
	ldr	x28, [sp, #1296]
	str	x8, [sp, #40]           // 8-byte Folded Spill
	ldr	x8, [sp, #1320]
	ldr	x29, [sp, #1288]
	ldr	x19, [sp, #1280]
	ldr	x23, [sp, #1272]
	str	x8, [sp, #32]           // 8-byte Folded Spill
	ldr	x1, [x22, #8]
	ldr	x21, [sp, #1264]
	add	x8, sp, #1184           // =1184
	mov	 x0, x25
	str	x22, [sp, #120]         // 8-byte Folded Spill
	bl	mulPv512x64
	ldp	x10, x12, [sp, #48]     // 8-byte Folded Reload
	ldp	x14, x16, [sp, #64]     // 8-byte Folded Reload
	cmn		x21, x24
	ldp	x18, x1, [sp, #80]      // 8-byte Folded Reload
	adcs	x10, x23, x10
	adcs	x12, x19, x12
	adcs	x14, x29, x14
	ldp	x3, x5, [sp, #96]       // 8-byte Folded Reload
	ldp	x4, x6, [sp, #32]       // 8-byte Folded Reload
	adcs	x16, x28, x16
	adcs	x18, x27, x18
	adcs	x1, x26, x1
	ldr	x2, [sp, #1184]
	adcs	x3, x4, x3
	ldr	x4, [sp, #1192]
	ldr	x0, [sp, #1200]
	adcs	x5, x6, x5
	ldr	x17, [sp, #1208]
	adcs	x6, xzr, xzr
	ldr	x15, [sp, #1216]
	adds		x19, x10, x2
	ldr	x13, [sp, #1224]
	adcs	x10, x12, x4
	ldr	x11, [sp, #1232]
	str	x10, [sp, #40]          // 8-byte Folded Spill
	adcs	x10, x14, x0
	ldr	x9, [sp, #1240]
	str	x10, [sp, #88]          // 8-byte Folded Spill
	adcs	x10, x16, x17
	ldr	x8, [sp, #1248]
	str	x10, [sp, #80]          // 8-byte Folded Spill
	adcs	x10, x18, x15
	str	x10, [sp, #72]          // 8-byte Folded Spill
	adcs	x10, x1, x13
	str	x10, [sp, #64]          // 8-byte Folded Spill
	adcs	x10, x3, x11
	ldr	x27, [sp, #136]         // 8-byte Folded Reload
	adcs	x9, x5, x9
	adcs	x8, x6, x8
	stp	x8, x9, [sp, #96]       // 8-byte Folded Spill
	adcs	x8, xzr, xzr
	stp	x8, x10, [sp, #48]      // 8-byte Folded Spill
	mul		x1, x19, x27
	add	x8, sp, #1104           // =1104
	mov	 x0, x20
	bl	mulPv512x64
	ldr	x8, [sp, #1168]
	ldr	x28, [sp, #128]         // 8-byte Folded Reload
	ldr	x23, [sp, #1144]
	ldr	x24, [sp, #1136]
	str	x8, [sp, #32]           // 8-byte Folded Spill
	ldr	x8, [sp, #1160]
	ldr	x25, [sp, #1128]
	ldr	x26, [sp, #1120]
	ldr	x29, [sp, #1112]
	str	x8, [sp, #24]           // 8-byte Folded Spill
	ldr	x8, [sp, #1152]
	ldr	x21, [sp, #1104]
	mov	 x0, x28
	str	x8, [sp, #16]           // 8-byte Folded Spill
	ldr	x1, [x22, #16]
	add	x8, sp, #1024           // =1024
	bl	mulPv512x64
	ldr	x10, [sp, #40]          // 8-byte Folded Reload
	ldp	x14, x12, [sp, #80]     // 8-byte Folded Reload
	cmn		x19, x21
	ldp	x18, x16, [sp, #64]     // 8-byte Folded Reload
	adcs	x10, x10, x29
	adcs	x12, x12, x26
	ldr	x1, [sp, #56]           // 8-byte Folded Reload
	ldp	x2, x4, [sp, #16]       // 8-byte Folded Reload
	adcs	x14, x14, x25
	ldp	x5, x3, [sp, #96]       // 8-byte Folded Reload
	adcs	x16, x16, x24
	ldr	x6, [sp, #32]           // 8-byte Folded Reload
	adcs	x18, x18, x23
	adcs	x1, x1, x2
	adcs	x3, x3, x4
	adcs	x5, x5, x6
	ldr	x6, [sp, #48]           // 8-byte Folded Reload
	ldr	x2, [sp, #1024]
	ldr	x4, [sp, #1032]
	ldr	x0, [sp, #1040]
	ldr	x17, [sp, #1048]
	adcs	x6, x6, xzr
	ldr	x15, [sp, #1056]
	adds		x19, x10, x2
	ldr	x13, [sp, #1064]
	adcs	x10, x12, x4
	ldr	x11, [sp, #1072]
	str	x10, [sp, #40]          // 8-byte Folded Spill
	adcs	x10, x14, x0
	ldr	x9, [sp, #1080]
	str	x10, [sp, #88]          // 8-byte Folded Spill
	adcs	x10, x16, x17
	ldr	x8, [sp, #1088]
	str	x10, [sp, #80]          // 8-byte Folded Spill
	adcs	x10, x18, x15
	str	x10, [sp, #72]          // 8-byte Folded Spill
	adcs	x10, x1, x13
	str	x10, [sp, #64]          // 8-byte Folded Spill
	adcs	x10, x3, x11
	adcs	x9, x5, x9
	adcs	x8, x6, x8
	stp	x8, x9, [sp, #96]       // 8-byte Folded Spill
	adcs	x8, xzr, xzr
	stp	x8, x10, [sp, #48]      // 8-byte Folded Spill
	mul		x1, x19, x27
	add	x8, sp, #944            // =944
	mov	 x0, x20
	bl	mulPv512x64
	ldr	x8, [sp, #1008]
	ldr	x27, [sp, #120]         // 8-byte Folded Reload
	ldr	x22, [sp, #992]
	ldr	x23, [sp, #984]
	str	x8, [sp, #32]           // 8-byte Folded Spill
	ldr	x8, [sp, #1000]
	ldr	x24, [sp, #976]
	ldr	x25, [sp, #968]
	ldr	x26, [sp, #960]
	str	x8, [sp, #24]           // 8-byte Folded Spill
	ldr	x1, [x27, #24]
	ldr	x29, [sp, #952]
	ldr	x21, [sp, #944]
	add	x8, sp, #864            // =864
	mov	 x0, x28
	bl	mulPv512x64
	ldr	x10, [sp, #40]          // 8-byte Folded Reload
	ldp	x14, x12, [sp, #80]     // 8-byte Folded Reload
	cmn		x19, x21
	ldp	x18, x16, [sp, #64]     // 8-byte Folded Reload
	adcs	x10, x10, x29
	adcs	x12, x12, x26
	ldr	x1, [sp, #56]           // 8-byte Folded Reload
	adcs	x14, x14, x25
	ldp	x5, x3, [sp, #96]       // 8-byte Folded Reload
	ldp	x4, x6, [sp, #24]       // 8-byte Folded Reload
	adcs	x16, x16, x24
	adcs	x18, x18, x23
	adcs	x1, x1, x22
	adcs	x3, x3, x4
	adcs	x5, x5, x6
	ldr	x6, [sp, #48]           // 8-byte Folded Reload
	ldr	x2, [sp, #864]
	ldr	x4, [sp, #872]
	ldr	x0, [sp, #880]
	ldr	x17, [sp, #888]
	adcs	x6, x6, xzr
	ldr	x15, [sp, #896]
	adds		x19, x10, x2
	ldr	x13, [sp, #904]
	adcs	x10, x12, x4
	ldr	x11, [sp, #912]
	str	x10, [sp, #40]          // 8-byte Folded Spill
	adcs	x10, x14, x0
	ldr	x9, [sp, #920]
	str	x10, [sp, #88]          // 8-byte Folded Spill
	adcs	x10, x16, x17
	ldr	x8, [sp, #928]
	str	x10, [sp, #80]          // 8-byte Folded Spill
	adcs	x10, x18, x15
	str	x10, [sp, #72]          // 8-byte Folded Spill
	adcs	x10, x1, x13
	str	x10, [sp, #64]          // 8-byte Folded Spill
	adcs	x10, x3, x11
	ldr	x28, [sp, #136]         // 8-byte Folded Reload
	adcs	x9, x5, x9
	adcs	x8, x6, x8
	stp	x8, x9, [sp, #96]       // 8-byte Folded Spill
	adcs	x8, xzr, xzr
	stp	x8, x10, [sp, #48]      // 8-byte Folded Spill
	mul		x1, x19, x28
	add	x8, sp, #784            // =784
	mov	 x0, x20
	bl	mulPv512x64
	ldr	x8, [sp, #848]
	ldr	x22, [sp, #832]
	ldr	x23, [sp, #824]
	ldr	x24, [sp, #816]
	str	x8, [sp, #32]           // 8-byte Folded Spill
	ldr	x8, [sp, #840]
	ldr	x25, [sp, #808]
	ldr	x26, [sp, #800]
	ldr	x29, [sp, #792]
	str	x8, [sp, #24]           // 8-byte Folded Spill
	ldr	x1, [x27, #32]
	ldr	x27, [sp, #128]         // 8-byte Folded Reload
	ldr	x21, [sp, #784]
	add	x8, sp, #704            // =704
	mov	 x0, x27
	bl	mulPv512x64
	ldr	x10, [sp, #40]          // 8-byte Folded Reload
	ldp	x14, x12, [sp, #80]     // 8-byte Folded Reload
	cmn		x19, x21
	ldp	x18, x16, [sp, #64]     // 8-byte Folded Reload
	adcs	x10, x10, x29
	adcs	x12, x12, x26
	ldr	x1, [sp, #56]           // 8-byte Folded Reload
	adcs	x14, x14, x25
	ldp	x5, x3, [sp, #96]       // 8-byte Folded Reload
	ldp	x4, x6, [sp, #24]       // 8-byte Folded Reload
	adcs	x16, x16, x24
	adcs	x18, x18, x23
	adcs	x1, x1, x22
	adcs	x3, x3, x4
	adcs	x5, x5, x6
	ldr	x6, [sp, #48]           // 8-byte Folded Reload
	ldr	x2, [sp, #704]
	ldr	x4, [sp, #712]
	ldr	x0, [sp, #720]
	ldr	x17, [sp, #728]
	adcs	x6, x6, xzr
	ldr	x15, [sp, #736]
	adds		x19, x10, x2
	ldr	x13, [sp, #744]
	adcs	x10, x12, x4
	ldr	x11, [sp, #752]
	str	x10, [sp, #40]          // 8-byte Folded Spill
	adcs	x10, x14, x0
	ldr	x9, [sp, #760]
	str	x10, [sp, #88]          // 8-byte Folded Spill
	adcs	x10, x16, x17
	ldr	x8, [sp, #768]
	str	x10, [sp, #80]          // 8-byte Folded Spill
	adcs	x10, x18, x15
	str	x10, [sp, #72]          // 8-byte Folded Spill
	adcs	x10, x1, x13
	str	x10, [sp, #64]          // 8-byte Folded Spill
	adcs	x10, x3, x11
	adcs	x9, x5, x9
	adcs	x8, x6, x8
	stp	x8, x9, [sp, #96]       // 8-byte Folded Spill
	adcs	x8, xzr, xzr
	stp	x8, x10, [sp, #48]      // 8-byte Folded Spill
	mul		x1, x19, x28
	add	x8, sp, #624            // =624
	mov	 x0, x20
	bl	mulPv512x64
	ldr	x8, [sp, #688]
	ldr	x28, [sp, #120]         // 8-byte Folded Reload
	ldr	x22, [sp, #672]
	ldr	x23, [sp, #664]
	str	x8, [sp, #32]           // 8-byte Folded Spill
	ldr	x8, [sp, #680]
	ldr	x24, [sp, #656]
	ldr	x25, [sp, #648]
	ldr	x26, [sp, #640]
	str	x8, [sp, #24]           // 8-byte Folded Spill
	ldr	x1, [x28, #40]
	ldr	x29, [sp, #632]
	ldr	x21, [sp, #624]
	add	x8, sp, #544            // =544
	mov	 x0, x27
	bl	mulPv512x64
	ldr	x10, [sp, #40]          // 8-byte Folded Reload
	ldp	x14, x12, [sp, #80]     // 8-byte Folded Reload
	cmn		x19, x21
	ldp	x18, x16, [sp, #64]     // 8-byte Folded Reload
	adcs	x10, x10, x29
	adcs	x12, x12, x26
	ldr	x1, [sp, #56]           // 8-byte Folded Reload
	adcs	x14, x14, x25
	ldp	x5, x3, [sp, #96]       // 8-byte Folded Reload
	ldp	x4, x6, [sp, #24]       // 8-byte Folded Reload
	adcs	x16, x16, x24
	adcs	x18, x18, x23
	adcs	x1, x1, x22
	adcs	x3, x3, x4
	adcs	x5, x5, x6
	ldr	x6, [sp, #48]           // 8-byte Folded Reload
	ldr	x2, [sp, #544]
	ldr	x4, [sp, #552]
	ldr	x0, [sp, #560]
	ldr	x17, [sp, #568]
	adcs	x6, x6, xzr
	ldr	x15, [sp, #576]
	adds		x19, x10, x2
	ldr	x13, [sp, #584]
	adcs	x10, x12, x4
	ldr	x11, [sp, #592]
	str	x10, [sp, #40]          // 8-byte Folded Spill
	adcs	x10, x14, x0
	ldr	x9, [sp, #600]
	str	x10, [sp, #88]          // 8-byte Folded Spill
	adcs	x10, x16, x17
	ldr	x8, [sp, #608]
	str	x10, [sp, #80]          // 8-byte Folded Spill
	adcs	x10, x18, x15
	str	x10, [sp, #72]          // 8-byte Folded Spill
	adcs	x10, x1, x13
	str	x10, [sp, #64]          // 8-byte Folded Spill
	adcs	x10, x3, x11
	ldr	x27, [sp, #136]         // 8-byte Folded Reload
	adcs	x9, x5, x9
	adcs	x8, x6, x8
	stp	x8, x9, [sp, #96]       // 8-byte Folded Spill
	adcs	x8, xzr, xzr
	stp	x8, x10, [sp, #48]      // 8-byte Folded Spill
	mul		x1, x19, x27
	add	x8, sp, #464            // =464
	mov	 x0, x20
	bl	mulPv512x64
	ldr	x8, [sp, #528]
	ldp	x23, x22, [sp, #504]
	ldp	x25, x24, [sp, #488]
	ldp	x29, x26, [sp, #472]
	str	x8, [sp, #32]           // 8-byte Folded Spill
	ldr	x8, [sp, #520]
	ldr	x21, [sp, #464]
	str	x8, [sp, #24]           // 8-byte Folded Spill
	ldr	x1, [x28, #48]
	ldr	x28, [sp, #128]         // 8-byte Folded Reload
	add	x8, sp, #384            // =384
	mov	 x0, x28
	bl	mulPv512x64
	ldr	x10, [sp, #40]          // 8-byte Folded Reload
	ldp	x14, x12, [sp, #80]     // 8-byte Folded Reload
	cmn		x19, x21
	ldp	x18, x16, [sp, #64]     // 8-byte Folded Reload
	adcs	x10, x10, x29
	adcs	x12, x12, x26
	ldr	x1, [sp, #56]           // 8-byte Folded Reload
	adcs	x14, x14, x25
	ldp	x5, x3, [sp, #96]       // 8-byte Folded Reload
	ldp	x4, x6, [sp, #24]       // 8-byte Folded Reload
	adcs	x16, x16, x24
	adcs	x18, x18, x23
	adcs	x1, x1, x22
	adcs	x3, x3, x4
	adcs	x5, x5, x6
	ldr	x6, [sp, #48]           // 8-byte Folded Reload
	ldr	x2, [sp, #384]
	ldp	x4, x0, [sp, #392]
	ldp	x17, x15, [sp, #408]
	adcs	x6, x6, xzr
	adds		x19, x10, x2
	ldp	x13, x11, [sp, #424]
	adcs	x10, x12, x4
	str	x10, [sp, #40]          // 8-byte Folded Spill
	adcs	x10, x14, x0
	ldp	x9, x8, [sp, #440]
	str	x10, [sp, #88]          // 8-byte Folded Spill
	adcs	x10, x16, x17
	str	x10, [sp, #80]          // 8-byte Folded Spill
	adcs	x10, x18, x15
	str	x10, [sp, #72]          // 8-byte Folded Spill
	adcs	x10, x1, x13
	str	x10, [sp, #64]          // 8-byte Folded Spill
	adcs	x10, x3, x11
	adcs	x9, x5, x9
	adcs	x8, x6, x8
	stp	x8, x9, [sp, #96]       // 8-byte Folded Spill
	adcs	x8, xzr, xzr
	stp	x8, x10, [sp, #48]      // 8-byte Folded Spill
	mul		x1, x19, x27
	add	x8, sp, #304            // =304
	mov	 x0, x20
	bl	mulPv512x64
	ldp	x27, x8, [sp, #360]
	ldp	x23, x22, [sp, #344]
	ldp	x25, x24, [sp, #328]
	ldp	x29, x26, [sp, #312]
	str	x8, [sp, #32]           // 8-byte Folded Spill
	ldr	x8, [sp, #120]          // 8-byte Folded Reload
	ldr	x21, [sp, #304]
	mov	 x0, x28
	ldr	x1, [x8, #56]
	add	x8, sp, #224            // =224
	bl	mulPv512x64
	ldr	x10, [sp, #40]          // 8-byte Folded Reload
	ldp	x14, x12, [sp, #80]     // 8-byte Folded Reload
	cmn		x19, x21
	ldp	x18, x16, [sp, #64]     // 8-byte Folded Reload
	adcs	x10, x10, x29
	adcs	x12, x12, x26
	ldr	x1, [sp, #56]           // 8-byte Folded Reload
	adcs	x14, x14, x25
	ldp	x5, x3, [sp, #96]       // 8-byte Folded Reload
	adcs	x16, x16, x24
	ldr	x6, [sp, #32]           // 8-byte Folded Reload
	adcs	x18, x18, x23
	adcs	x1, x1, x22
	adcs	x3, x3, x27
	adcs	x5, x5, x6
	ldr	x6, [sp, #48]           // 8-byte Folded Reload
	ldr	x2, [sp, #224]
	ldp	x4, x0, [sp, #232]
	ldp	x17, x15, [sp, #248]
	adcs	x6, x6, xzr
	adds		x19, x10, x2
	ldp	x13, x11, [sp, #264]
	adcs	x21, x12, x4
	adcs	x22, x14, x0
	ldp	x9, x8, [sp, #280]
	adcs	x23, x16, x17
	adcs	x24, x18, x15
	adcs	x25, x1, x13
	adcs	x26, x3, x11
	adcs	x27, x5, x9
	adcs	x28, x6, x8
	ldr	x8, [sp, #136]          // 8-byte Folded Reload
	mov	 x0, x20
	adcs	x29, xzr, xzr
	mul		x1, x19, x8
	add	x8, sp, #144            // =144
	bl	mulPv512x64
	ldp	x13, x16, [sp, #144]
	ldr	x15, [sp, #160]
	ldp	x14, x12, [sp, #168]
	ldp	x11, x10, [sp, #184]
	cmn		x19, x13
	adcs	x16, x21, x16
	adcs	x15, x22, x15
	ldp	x9, x8, [sp, #200]
	adcs	x14, x23, x14
	adcs	x12, x24, x12
	adcs	x11, x25, x11
	ldp		x3, x4, [x20]
	adcs	x10, x26, x10
	adcs	x9, x27, x9
	ldp	x1, x2, [x20, #16]
	adcs	x8, x28, x8
	adcs	x5, x29, xzr
	ldp	x18, x0, [x20, #32]
	subs		x3, x16, x3
	sbcs	x4, x15, x4
	ldp	x13, x17, [x20, #48]
	sbcs	x1, x14, x1
	sbcs	x2, x12, x2
	sbcs	x18, x11, x18
	sbcs	x0, x10, x0
	sbcs	x13, x9, x13
	sbcs	x17, x8, x17
	sbcs	x5, x5, xzr
	tst	 x5, #0x1
	csel	x9, x9, x13, ne
	ldr	x13, [sp, #112]         // 8-byte Folded Reload
	csel	x16, x16, x3, ne
	csel	x15, x15, x4, ne
	csel	x14, x14, x1, ne
	csel	x12, x12, x2, ne
	csel	x11, x11, x18, ne
	csel	x10, x10, x0, ne
	csel	x8, x8, x17, ne
	stp		x16, x15, [x13]
	stp	x14, x12, [x13, #16]
	stp	x11, x10, [x13, #32]
	stp	x9, x8, [x13, #48]
	add	sp, sp, #1424           // =1424
	ldp	x29, x30, [sp, #80]     // 8-byte Folded Reload
	ldp	x20, x19, [sp, #64]     // 8-byte Folded Reload
	ldp	x22, x21, [sp, #48]     // 8-byte Folded Reload
	ldp	x24, x23, [sp, #32]     // 8-byte Folded Reload
	ldp	x26, x25, [sp, #16]     // 8-byte Folded Reload
	ldp	x28, x27, [sp], #96     // 8-byte Folded Reload
	ret
.Lfunc_end60:
	.size	mcl_fp_mont8L, .Lfunc_end60-mcl_fp_mont8L

	.globl	mcl_fp_montNF8L
	.p2align	2
	.type	mcl_fp_montNF8L,@function
mcl_fp_montNF8L:                        // @mcl_fp_montNF8L
// BB#0:
	stp	x28, x27, [sp, #-96]!   // 8-byte Folded Spill
	stp	x26, x25, [sp, #16]     // 8-byte Folded Spill
	stp	x24, x23, [sp, #32]     // 8-byte Folded Spill
	stp	x22, x21, [sp, #48]     // 8-byte Folded Spill
	stp	x20, x19, [sp, #64]     // 8-byte Folded Spill
	stp	x29, x30, [sp, #80]     // 8-byte Folded Spill
	sub	sp, sp, #1408           // =1408
	mov	 x20, x3
	ldur	x19, [x20, #-8]
	mov	 x26, x2
	mov	 x22, x1
	add	x8, sp, #1328           // =1328
	str	x19, [sp, #120]         // 8-byte Folded Spill
	ldr		x1, [x26]
	str	x0, [sp, #96]           // 8-byte Folded Spill
	mov	 x0, x22
	str	x22, [sp, #112]         // 8-byte Folded Spill
	bl	mulPv512x64
	ldr	x8, [sp, #1392]
	ldr	x24, [sp, #1328]
	mov	 x0, x20
	str	x8, [sp, #88]           // 8-byte Folded Spill
	ldr	x8, [sp, #1384]
	mul		x1, x24, x19
	str	x8, [sp, #80]           // 8-byte Folded Spill
	ldr	x8, [sp, #1376]
	str	x8, [sp, #72]           // 8-byte Folded Spill
	ldr	x8, [sp, #1368]
	str	x8, [sp, #64]           // 8-byte Folded Spill
	ldr	x8, [sp, #1360]
	str	x8, [sp, #56]           // 8-byte Folded Spill
	ldr	x8, [sp, #1352]
	str	x8, [sp, #48]           // 8-byte Folded Spill
	ldr	x8, [sp, #1344]
	str	x8, [sp, #40]           // 8-byte Folded Spill
	ldr	x8, [sp, #1336]
	str	x8, [sp, #32]           // 8-byte Folded Spill
	add	x8, sp, #1248           // =1248
	bl	mulPv512x64
	ldr	x8, [sp, #1312]
	ldr	x27, [sp, #1288]
	ldr	x28, [sp, #1280]
	ldr	x29, [sp, #1272]
	str	x8, [sp, #24]           // 8-byte Folded Spill
	ldr	x8, [sp, #1304]
	ldr	x19, [sp, #1264]
	ldr	x23, [sp, #1256]
	ldr	x21, [sp, #1248]
	str	x8, [sp, #16]           // 8-byte Folded Spill
	ldr	x8, [sp, #1296]
	mov	 x0, x22
	str	x8, [sp, #8]            // 8-byte Folded Spill
	ldr	x1, [x26, #8]
	add	x8, sp, #1168           // =1168
	str	x26, [sp, #104]         // 8-byte Folded Spill
	bl	mulPv512x64
	ldp	x10, x12, [sp, #32]     // 8-byte Folded Reload
	ldp	x14, x16, [sp, #48]     // 8-byte Folded Reload
	cmn		x21, x24
	ldp	x18, x1, [sp, #64]      // 8-byte Folded Reload
	adcs	x10, x23, x10
	adcs	x12, x19, x12
	ldp	x2, x4, [sp, #8]        // 8-byte Folded Reload
	adcs	x14, x29, x14
	adcs	x16, x28, x16
	ldp	x3, x5, [sp, #80]       // 8-byte Folded Reload
	adcs	x18, x27, x18
	ldr	x6, [sp, #24]           // 8-byte Folded Reload
	adcs	x1, x2, x1
	ldr	x2, [sp, #1168]
	ldr	x0, [sp, #1176]
	adcs	x3, x4, x3
	ldr	x4, [sp, #1184]
	ldr	x17, [sp, #1192]
	adcs	x5, x6, x5
	ldr	x15, [sp, #1200]
	adds		x19, x10, x2
	ldr	x13, [sp, #1208]
	adcs	x10, x12, x0
	ldr	x11, [sp, #1216]
	str	x10, [sp, #32]          // 8-byte Folded Spill
	adcs	x10, x14, x4
	ldr	x9, [sp, #1224]
	str	x10, [sp, #64]          // 8-byte Folded Spill
	adcs	x10, x16, x17
	ldr	x8, [sp, #1232]
	str	x10, [sp, #56]          // 8-byte Folded Spill
	adcs	x10, x18, x15
	str	x10, [sp, #48]          // 8-byte Folded Spill
	adcs	x10, x1, x13
	ldr	x29, [sp, #120]         // 8-byte Folded Reload
	str	x10, [sp, #40]          // 8-byte Folded Spill
	adcs	x10, x3, x11
	adcs	x9, x5, x9
	adcs	x8, x8, xzr
	str	x8, [sp, #72]           // 8-byte Folded Spill
	mul		x1, x19, x29
	add	x8, sp, #1088           // =1088
	mov	 x0, x20
	stp	x9, x10, [sp, #80]      // 8-byte Folded Spill
	bl	mulPv512x64
	ldr	x8, [sp, #1152]
	ldr	x27, [sp, #1136]
	ldr	x28, [sp, #1128]
	ldr	x22, [sp, #1120]
	str	x8, [sp, #24]           // 8-byte Folded Spill
	ldr	x8, [sp, #1144]
	ldr	x23, [sp, #1112]
	ldr	x24, [sp, #1104]
	ldr	x25, [sp, #1096]
	str	x8, [sp, #16]           // 8-byte Folded Spill
	ldr	x1, [x26, #16]
	ldr	x26, [sp, #112]         // 8-byte Folded Reload
	ldr	x21, [sp, #1088]
	add	x8, sp, #1008           // =1008
	mov	 x0, x26
	bl	mulPv512x64
	ldp	x10, x18, [sp, #32]     // 8-byte Folded Reload
	ldp	x14, x12, [sp, #56]     // 8-byte Folded Reload
	cmn		x19, x21
	ldr	x16, [sp, #48]          // 8-byte Folded Reload
	adcs	x10, x10, x25
	adcs	x12, x12, x24
	ldp	x3, x1, [sp, #80]       // 8-byte Folded Reload
	adcs	x14, x14, x23
	ldp	x4, x6, [sp, #16]       // 8-byte Folded Reload
	adcs	x16, x16, x22
	ldr	x5, [sp, #72]           // 8-byte Folded Reload
	adcs	x18, x18, x28
	ldr	x2, [sp, #1008]
	ldr	x0, [sp, #1016]
	adcs	x1, x1, x27
	adcs	x3, x3, x4
	ldr	x4, [sp, #1024]
	ldr	x17, [sp, #1032]
	adcs	x5, x5, x6
	ldr	x15, [sp, #1040]
	adds		x19, x10, x2
	ldr	x13, [sp, #1048]
	adcs	x10, x12, x0
	ldr	x11, [sp, #1056]
	str	x10, [sp, #32]          // 8-byte Folded Spill
	adcs	x10, x14, x4
	ldr	x9, [sp, #1064]
	str	x10, [sp, #64]          // 8-byte Folded Spill
	adcs	x10, x16, x17
	ldr	x8, [sp, #1072]
	str	x10, [sp, #56]          // 8-byte Folded Spill
	adcs	x10, x18, x15
	str	x10, [sp, #48]          // 8-byte Folded Spill
	adcs	x10, x1, x13
	str	x10, [sp, #40]          // 8-byte Folded Spill
	adcs	x10, x3, x11
	adcs	x9, x5, x9
	adcs	x8, x8, xzr
	str	x8, [sp, #72]           // 8-byte Folded Spill
	mul		x1, x19, x29
	add	x8, sp, #928            // =928
	mov	 x0, x20
	stp	x9, x10, [sp, #80]      // 8-byte Folded Spill
	bl	mulPv512x64
	ldr	x8, [sp, #992]
	ldr	x29, [sp, #104]         // 8-byte Folded Reload
	ldr	x27, [sp, #976]
	ldr	x28, [sp, #968]
	str	x8, [sp, #24]           // 8-byte Folded Spill
	ldr	x8, [sp, #984]
	ldr	x22, [sp, #960]
	ldr	x23, [sp, #952]
	ldr	x24, [sp, #944]
	str	x8, [sp, #16]           // 8-byte Folded Spill
	ldr	x1, [x29, #24]
	ldr	x25, [sp, #936]
	ldr	x21, [sp, #928]
	add	x8, sp, #848            // =848
	mov	 x0, x26
	bl	mulPv512x64
	ldp	x10, x18, [sp, #32]     // 8-byte Folded Reload
	ldp	x14, x12, [sp, #56]     // 8-byte Folded Reload
	cmn		x19, x21
	ldr	x16, [sp, #48]          // 8-byte Folded Reload
	adcs	x10, x10, x25
	adcs	x12, x12, x24
	ldp	x3, x1, [sp, #80]       // 8-byte Folded Reload
	adcs	x14, x14, x23
	ldp	x4, x6, [sp, #16]       // 8-byte Folded Reload
	adcs	x16, x16, x22
	ldr	x5, [sp, #72]           // 8-byte Folded Reload
	adcs	x18, x18, x28
	ldr	x2, [sp, #848]
	ldr	x0, [sp, #856]
	adcs	x1, x1, x27
	adcs	x3, x3, x4
	ldr	x4, [sp, #864]
	ldr	x17, [sp, #872]
	adcs	x5, x5, x6
	ldr	x15, [sp, #880]
	adds		x19, x10, x2
	ldr	x13, [sp, #888]
	adcs	x10, x12, x0
	ldr	x11, [sp, #896]
	str	x10, [sp, #32]          // 8-byte Folded Spill
	adcs	x10, x14, x4
	ldr	x9, [sp, #904]
	str	x10, [sp, #64]          // 8-byte Folded Spill
	adcs	x10, x16, x17
	ldr	x8, [sp, #912]
	str	x10, [sp, #56]          // 8-byte Folded Spill
	adcs	x10, x18, x15
	str	x10, [sp, #48]          // 8-byte Folded Spill
	adcs	x10, x1, x13
	ldr	x26, [sp, #120]         // 8-byte Folded Reload
	str	x10, [sp, #40]          // 8-byte Folded Spill
	adcs	x10, x3, x11
	adcs	x9, x5, x9
	adcs	x8, x8, xzr
	str	x8, [sp, #72]           // 8-byte Folded Spill
	mul		x1, x19, x26
	add	x8, sp, #768            // =768
	mov	 x0, x20
	stp	x9, x10, [sp, #80]      // 8-byte Folded Spill
	bl	mulPv512x64
	ldr	x8, [sp, #832]
	ldr	x27, [sp, #816]
	ldr	x28, [sp, #808]
	ldr	x22, [sp, #800]
	str	x8, [sp, #24]           // 8-byte Folded Spill
	ldr	x8, [sp, #824]
	ldr	x23, [sp, #792]
	ldr	x24, [sp, #784]
	ldr	x25, [sp, #776]
	str	x8, [sp, #16]           // 8-byte Folded Spill
	ldr	x1, [x29, #32]
	ldr	x29, [sp, #112]         // 8-byte Folded Reload
	ldr	x21, [sp, #768]
	add	x8, sp, #688            // =688
	mov	 x0, x29
	bl	mulPv512x64
	ldp	x10, x18, [sp, #32]     // 8-byte Folded Reload
	ldp	x14, x12, [sp, #56]     // 8-byte Folded Reload
	cmn		x19, x21
	ldr	x16, [sp, #48]          // 8-byte Folded Reload
	adcs	x10, x10, x25
	adcs	x12, x12, x24
	ldp	x3, x1, [sp, #80]       // 8-byte Folded Reload
	adcs	x14, x14, x23
	ldp	x4, x6, [sp, #16]       // 8-byte Folded Reload
	adcs	x16, x16, x22
	ldr	x5, [sp, #72]           // 8-byte Folded Reload
	adcs	x18, x18, x28
	ldr	x2, [sp, #688]
	ldr	x0, [sp, #696]
	adcs	x1, x1, x27
	adcs	x3, x3, x4
	ldr	x4, [sp, #704]
	ldr	x17, [sp, #712]
	adcs	x5, x5, x6
	ldr	x15, [sp, #720]
	adds		x19, x10, x2
	ldr	x13, [sp, #728]
	adcs	x10, x12, x0
	ldr	x11, [sp, #736]
	str	x10, [sp, #32]          // 8-byte Folded Spill
	adcs	x10, x14, x4
	ldr	x9, [sp, #744]
	str	x10, [sp, #64]          // 8-byte Folded Spill
	adcs	x10, x16, x17
	ldr	x8, [sp, #752]
	str	x10, [sp, #56]          // 8-byte Folded Spill
	adcs	x10, x18, x15
	str	x10, [sp, #48]          // 8-byte Folded Spill
	adcs	x10, x1, x13
	str	x10, [sp, #40]          // 8-byte Folded Spill
	adcs	x10, x3, x11
	adcs	x9, x5, x9
	adcs	x8, x8, xzr
	str	x8, [sp, #72]           // 8-byte Folded Spill
	mul		x1, x19, x26
	add	x8, sp, #608            // =608
	mov	 x0, x20
	stp	x9, x10, [sp, #80]      // 8-byte Folded Spill
	bl	mulPv512x64
	ldr	x8, [sp, #672]
	ldr	x26, [sp, #104]         // 8-byte Folded Reload
	ldr	x27, [sp, #656]
	ldr	x28, [sp, #648]
	str	x8, [sp, #24]           // 8-byte Folded Spill
	ldr	x8, [sp, #664]
	ldr	x22, [sp, #640]
	ldr	x23, [sp, #632]
	ldr	x24, [sp, #624]
	str	x8, [sp, #16]           // 8-byte Folded Spill
	ldr	x1, [x26, #40]
	ldr	x25, [sp, #616]
	ldr	x21, [sp, #608]
	add	x8, sp, #528            // =528
	mov	 x0, x29
	bl	mulPv512x64
	ldp	x10, x18, [sp, #32]     // 8-byte Folded Reload
	ldp	x14, x12, [sp, #56]     // 8-byte Folded Reload
	cmn		x19, x21
	ldr	x16, [sp, #48]          // 8-byte Folded Reload
	adcs	x10, x10, x25
	adcs	x12, x12, x24
	ldp	x3, x1, [sp, #80]       // 8-byte Folded Reload
	adcs	x14, x14, x23
	ldp	x4, x6, [sp, #16]       // 8-byte Folded Reload
	adcs	x16, x16, x22
	ldr	x5, [sp, #72]           // 8-byte Folded Reload
	adcs	x18, x18, x28
	ldr	x2, [sp, #528]
	ldr	x0, [sp, #536]
	adcs	x1, x1, x27
	adcs	x3, x3, x4
	ldr	x4, [sp, #544]
	ldr	x17, [sp, #552]
	adcs	x5, x5, x6
	ldr	x15, [sp, #560]
	adds		x19, x10, x2
	ldr	x13, [sp, #568]
	adcs	x10, x12, x0
	ldr	x11, [sp, #576]
	str	x10, [sp, #32]          // 8-byte Folded Spill
	adcs	x10, x14, x4
	ldr	x9, [sp, #584]
	str	x10, [sp, #64]          // 8-byte Folded Spill
	adcs	x10, x16, x17
	ldr	x8, [sp, #592]
	str	x10, [sp, #56]          // 8-byte Folded Spill
	adcs	x10, x18, x15
	str	x10, [sp, #48]          // 8-byte Folded Spill
	adcs	x10, x1, x13
	ldr	x29, [sp, #120]         // 8-byte Folded Reload
	str	x10, [sp, #40]          // 8-byte Folded Spill
	adcs	x10, x3, x11
	adcs	x9, x5, x9
	adcs	x8, x8, xzr
	str	x8, [sp, #72]           // 8-byte Folded Spill
	mul		x1, x19, x29
	add	x8, sp, #448            // =448
	mov	 x0, x20
	stp	x9, x10, [sp, #80]      // 8-byte Folded Spill
	bl	mulPv512x64
	ldr	x8, [sp, #512]
	ldp	x22, x28, [sp, #480]
	ldp	x24, x23, [sp, #464]
	ldp	x21, x25, [sp, #448]
	str	x8, [sp, #24]           // 8-byte Folded Spill
	ldp	x27, x8, [sp, #496]
	str	x8, [sp, #16]           // 8-byte Folded Spill
	ldr	x1, [x26, #48]
	ldr	x26, [sp, #112]         // 8-byte Folded Reload
	add	x8, sp, #368            // =368
	mov	 x0, x26
	bl	mulPv512x64
	ldp	x10, x18, [sp, #32]     // 8-byte Folded Reload
	ldp	x14, x12, [sp, #56]     // 8-byte Folded Reload
	cmn		x19, x21
	ldr	x16, [sp, #48]          // 8-byte Folded Reload
	adcs	x10, x10, x25
	adcs	x12, x12, x24
	ldp	x3, x1, [sp, #80]       // 8-byte Folded Reload
	adcs	x14, x14, x23
	ldp	x4, x6, [sp, #16]       // 8-byte Folded Reload
	adcs	x16, x16, x22
	ldr	x5, [sp, #72]           // 8-byte Folded Reload
	adcs	x18, x18, x28
	ldp	x2, x0, [sp, #368]
	adcs	x1, x1, x27
	adcs	x3, x3, x4
	ldr	x4, [sp, #384]
	ldp	x17, x15, [sp, #392]
	adcs	x5, x5, x6
	adds		x19, x10, x2
	ldp	x13, x11, [sp, #408]
	adcs	x10, x12, x0
	str	x10, [sp, #32]          // 8-byte Folded Spill
	adcs	x10, x14, x4
	ldp	x9, x8, [sp, #424]
	str	x10, [sp, #64]          // 8-byte Folded Spill
	adcs	x10, x16, x17
	str	x10, [sp, #56]          // 8-byte Folded Spill
	adcs	x10, x18, x15
	str	x10, [sp, #48]          // 8-byte Folded Spill
	adcs	x10, x1, x13
	str	x10, [sp, #40]          // 8-byte Folded Spill
	adcs	x10, x3, x11
	adcs	x9, x5, x9
	adcs	x8, x8, xzr
	str	x8, [sp, #72]           // 8-byte Folded Spill
	mul		x1, x19, x29
	add	x8, sp, #288            // =288
	mov	 x0, x20
	stp	x9, x10, [sp, #80]      // 8-byte Folded Spill
	bl	mulPv512x64
	ldp	x29, x8, [sp, #344]
	ldp	x28, x27, [sp, #328]
	ldp	x23, x22, [sp, #312]
	ldp	x25, x24, [sp, #296]
	str	x8, [sp, #24]           // 8-byte Folded Spill
	ldr	x8, [sp, #104]          // 8-byte Folded Reload
	ldr	x21, [sp, #288]
	mov	 x0, x26
	ldr	x1, [x8, #56]
	add	x8, sp, #208            // =208
	bl	mulPv512x64
	ldp	x10, x18, [sp, #32]     // 8-byte Folded Reload
	ldp	x14, x12, [sp, #56]     // 8-byte Folded Reload
	cmn		x19, x21
	ldr	x16, [sp, #48]          // 8-byte Folded Reload
	adcs	x10, x10, x25
	adcs	x12, x12, x24
	ldp	x3, x1, [sp, #80]       // 8-byte Folded Reload
	adcs	x14, x14, x23
	adcs	x16, x16, x22
	ldr	x5, [sp, #72]           // 8-byte Folded Reload
	ldr	x6, [sp, #24]           // 8-byte Folded Reload
	adcs	x18, x18, x28
	ldp	x2, x0, [sp, #208]
	adcs	x1, x1, x27
	adcs	x3, x3, x29
	ldr	x4, [sp, #224]
	ldp	x17, x15, [sp, #232]
	adcs	x5, x5, x6
	adds		x19, x10, x2
	ldp	x13, x11, [sp, #248]
	adcs	x21, x12, x0
	adcs	x22, x14, x4
	ldp	x9, x8, [sp, #264]
	adcs	x23, x16, x17
	adcs	x24, x18, x15
	adcs	x25, x1, x13
	adcs	x26, x3, x11
	adcs	x27, x5, x9
	adcs	x28, x8, xzr
	ldr	x8, [sp, #120]          // 8-byte Folded Reload
	mov	 x0, x20
	mul		x1, x19, x8
	add	x8, sp, #128            // =128
	bl	mulPv512x64
	ldp	x13, x16, [sp, #128]
	ldr	x15, [sp, #144]
	ldp	x14, x12, [sp, #152]
	ldp	x11, x10, [sp, #168]
	cmn		x19, x13
	adcs	x16, x21, x16
	adcs	x15, x22, x15
	ldp	x9, x8, [sp, #184]
	adcs	x14, x23, x14
	adcs	x12, x24, x12
	adcs	x11, x25, x11
	ldp		x3, x4, [x20]
	adcs	x10, x26, x10
	ldp	x1, x2, [x20, #16]
	adcs	x9, x27, x9
	adcs	x8, x28, x8
	ldp	x18, x0, [x20, #32]
	subs		x3, x16, x3
	sbcs	x4, x15, x4
	ldp	x13, x17, [x20, #48]
	sbcs	x1, x14, x1
	sbcs	x2, x12, x2
	sbcs	x18, x11, x18
	sbcs	x0, x10, x0
	sbcs	x13, x9, x13
	sbcs	x17, x8, x17
	cmp		x17, #0         // =0
	csel	x9, x9, x13, lt
	ldr	x13, [sp, #96]          // 8-byte Folded Reload
	csel	x16, x16, x3, lt
	csel	x15, x15, x4, lt
	csel	x14, x14, x1, lt
	csel	x12, x12, x2, lt
	csel	x11, x11, x18, lt
	csel	x10, x10, x0, lt
	csel	x8, x8, x17, lt
	stp		x16, x15, [x13]
	stp	x14, x12, [x13, #16]
	stp	x11, x10, [x13, #32]
	stp	x9, x8, [x13, #48]
	add	sp, sp, #1408           // =1408
	ldp	x29, x30, [sp, #80]     // 8-byte Folded Reload
	ldp	x20, x19, [sp, #64]     // 8-byte Folded Reload
	ldp	x22, x21, [sp, #48]     // 8-byte Folded Reload
	ldp	x24, x23, [sp, #32]     // 8-byte Folded Reload
	ldp	x26, x25, [sp, #16]     // 8-byte Folded Reload
	ldp	x28, x27, [sp], #96     // 8-byte Folded Reload
	ret
.Lfunc_end61:
	.size	mcl_fp_montNF8L, .Lfunc_end61-mcl_fp_montNF8L

	.globl	mcl_fp_montRed8L
	.p2align	2
	.type	mcl_fp_montRed8L,@function
mcl_fp_montRed8L:                       // @mcl_fp_montRed8L
// BB#0:
	stp	x28, x27, [sp, #-96]!   // 8-byte Folded Spill
	stp	x26, x25, [sp, #16]     // 8-byte Folded Spill
	stp	x24, x23, [sp, #32]     // 8-byte Folded Spill
	stp	x22, x21, [sp, #48]     // 8-byte Folded Spill
	stp	x20, x19, [sp, #64]     // 8-byte Folded Spill
	stp	x29, x30, [sp, #80]     // 8-byte Folded Spill
	sub	sp, sp, #736            // =736
	mov	 x21, x2
	ldr	x8, [x21, #48]
	mov	 x20, x1
	ldur	x23, [x21, #-8]
	ldp		x29, x19, [x20]
	str	x8, [sp, #72]           // 8-byte Folded Spill
	ldr	x8, [x21, #56]
	ldp	x22, x24, [x20, #48]
	ldp	x25, x26, [x20, #32]
	ldp	x27, x28, [x20, #16]
	str	x8, [sp, #80]           // 8-byte Folded Spill
	ldr	x8, [x21, #32]
	str	x0, [sp, #88]           // 8-byte Folded Spill
	mul		x1, x29, x23
	mov	 x0, x21
	str	x8, [sp, #56]           // 8-byte Folded Spill
	ldr	x8, [x21, #40]
	str	x8, [sp, #64]           // 8-byte Folded Spill
	ldr	x8, [x21, #16]
	str	x8, [sp, #40]           // 8-byte Folded Spill
	ldr	x8, [x21, #24]
	str	x8, [sp, #48]           // 8-byte Folded Spill
	ldr		x8, [x21]
	str	x8, [sp, #24]           // 8-byte Folded Spill
	ldr	x8, [x21, #8]
	str	x8, [sp, #32]           // 8-byte Folded Spill
	add	x8, sp, #656            // =656
	bl	mulPv512x64
	ldr	x14, [sp, #656]
	ldr	x15, [sp, #664]
	ldr	x16, [sp, #672]
	ldr	x13, [sp, #680]
	ldr	x12, [sp, #688]
	cmn		x29, x14
	ldr	x11, [sp, #696]
	adcs	x19, x19, x15
	ldr	x10, [sp, #704]
	adcs	x27, x27, x16
	ldr	x9, [sp, #712]
	adcs	x28, x28, x13
	ldr	x8, [sp, #720]
	ldr	x17, [x20, #64]
	adcs	x25, x25, x12
	adcs	x26, x26, x11
	adcs	x10, x22, x10
	adcs	x24, x24, x9
	adcs	x29, x17, x8
	mul		x1, x19, x23
	add	x8, sp, #576            // =576
	mov	 x0, x21
	str	x10, [sp, #16]          // 8-byte Folded Spill
	adcs	x22, xzr, xzr
	bl	mulPv512x64
	ldr	x15, [sp, #576]
	ldr	x16, [sp, #584]
	ldr	x8, [sp, #640]
	ldr	x14, [sp, #592]
	ldr	x13, [sp, #600]
	ldr	x12, [sp, #608]
	cmn		x19, x15
	adcs	x19, x27, x16
	add		x8, x22, x8
	adcs	x22, x28, x14
	adcs	x25, x25, x13
	ldr	x11, [sp, #616]
	adcs	x26, x26, x12
	ldr	x12, [sp, #16]          // 8-byte Folded Reload
	ldr	x10, [sp, #624]
	ldr	x9, [sp, #632]
	ldr	x17, [x20, #72]
	adcs	x11, x12, x11
	adcs	x24, x24, x10
	adcs	x27, x29, x9
	adcs	x28, x17, x8
	mul		x1, x19, x23
	add	x8, sp, #496            // =496
	mov	 x0, x21
	str	x11, [sp, #16]          // 8-byte Folded Spill
	adcs	x29, xzr, xzr
	bl	mulPv512x64
	ldr	x15, [sp, #496]
	ldp	x16, x14, [sp, #504]
	ldr	x13, [sp, #520]
	ldr	x12, [sp, #528]
	cmn		x19, x15
	adcs	x19, x22, x16
	adcs	x22, x25, x14
	adcs	x25, x26, x13
	ldr	x13, [sp, #16]          // 8-byte Folded Reload
	ldr	x11, [sp, #536]
	ldr	x10, [sp, #544]
	ldr	x8, [sp, #560]
	ldr	x9, [sp, #552]
	ldr	x17, [x20, #80]
	adcs	x12, x13, x12
	adcs	x24, x24, x11
	adcs	x26, x27, x10
	add		x8, x29, x8
	adcs	x27, x28, x9
	adcs	x28, x17, x8
	mul		x1, x19, x23
	add	x8, sp, #416            // =416
	mov	 x0, x21
	str	x12, [sp, #16]          // 8-byte Folded Spill
	adcs	x29, xzr, xzr
	bl	mulPv512x64
	ldr	x15, [sp, #416]
	ldp	x16, x14, [sp, #424]
	ldp	x13, x12, [sp, #440]
	ldp	x11, x10, [sp, #456]
	cmn		x19, x15
	adcs	x19, x22, x16
	adcs	x22, x25, x14
	ldr	x14, [sp, #16]          // 8-byte Folded Reload
	ldp	x9, x8, [sp, #472]
	ldr	x17, [x20, #88]
	mul		x1, x19, x23
	adcs	x13, x14, x13
	adcs	x24, x24, x12
	adcs	x25, x26, x11
	adcs	x26, x27, x10
	add		x8, x29, x8
	adcs	x27, x28, x9
	adcs	x28, x17, x8
	add	x8, sp, #336            // =336
	mov	 x0, x21
	str	x13, [sp, #16]          // 8-byte Folded Spill
	adcs	x29, xzr, xzr
	bl	mulPv512x64
	ldr	x15, [sp, #336]
	ldp	x16, x14, [sp, #344]
	ldp	x13, x12, [sp, #360]
	ldp	x11, x10, [sp, #376]
	cmn		x19, x15
	ldr	x15, [sp, #16]          // 8-byte Folded Reload
	adcs	x19, x22, x16
	ldp	x9, x8, [sp, #392]
	ldr	x17, [x20, #96]
	adcs	x22, x15, x14
	adcs	x13, x24, x13
	adcs	x24, x25, x12
	adcs	x25, x26, x11
	adcs	x26, x27, x10
	add		x8, x29, x8
	adcs	x27, x28, x9
	adcs	x28, x17, x8
	mul		x1, x19, x23
	add	x8, sp, #256            // =256
	mov	 x0, x21
	str	x13, [sp, #16]          // 8-byte Folded Spill
	adcs	x29, xzr, xzr
	bl	mulPv512x64
	ldr	x15, [sp, #256]
	ldp	x16, x14, [sp, #264]
	ldp	x13, x12, [sp, #280]
	ldp	x11, x10, [sp, #296]
	cmn		x19, x15
	ldr	x15, [sp, #16]          // 8-byte Folded Reload
	adcs	x19, x22, x16
	ldp	x9, x8, [sp, #312]
	ldr	x17, [x20, #104]
	adcs	x22, x15, x14
	adcs	x13, x24, x13
	adcs	x24, x25, x12
	adcs	x25, x26, x11
	adcs	x26, x27, x10
	add		x8, x29, x8
	adcs	x27, x28, x9
	adcs	x28, x17, x8
	mul		x1, x19, x23
	add	x8, sp, #176            // =176
	mov	 x0, x21
	str	x13, [sp, #16]          // 8-byte Folded Spill
	adcs	x29, xzr, xzr
	bl	mulPv512x64
	ldr	x15, [sp, #176]
	ldp	x16, x14, [sp, #184]
	ldp	x13, x12, [sp, #200]
	ldp	x11, x10, [sp, #216]
	cmn		x19, x15
	ldr	x15, [sp, #16]          // 8-byte Folded Reload
	adcs	x19, x22, x16
	ldp	x9, x8, [sp, #232]
	ldr	x17, [x20, #112]
	adcs	x22, x15, x14
	adcs	x13, x24, x13
	adcs	x24, x25, x12
	adcs	x25, x26, x11
	adcs	x26, x27, x10
	add		x8, x29, x8
	adcs	x27, x28, x9
	adcs	x28, x17, x8
	mul		x1, x19, x23
	add	x8, sp, #96             // =96
	mov	 x0, x21
	str	x13, [sp, #16]          // 8-byte Folded Spill
	adcs	x29, xzr, xzr
	bl	mulPv512x64
	ldp	x15, x14, [sp, #96]
	ldr	x16, [sp, #112]
	ldp	x13, x12, [sp, #120]
	ldp	x11, x10, [sp, #136]
	cmn		x19, x15
	ldr	x15, [sp, #16]          // 8-byte Folded Reload
	adcs	x14, x22, x14
	ldp	x9, x8, [sp, #152]
	ldr	x17, [x20, #120]
	adcs	x15, x15, x16
	adcs	x13, x24, x13
	adcs	x12, x25, x12
	adcs	x11, x26, x11
	adcs	x10, x27, x10
	add		x8, x29, x8
	adcs	x9, x28, x9
	adcs	x8, x17, x8
	ldp	x16, x17, [sp, #24]     // 8-byte Folded Reload
	ldp	x18, x0, [sp, #40]      // 8-byte Folded Reload
	ldp	x1, x2, [sp, #56]       // 8-byte Folded Reload
	ldp	x3, x4, [sp, #72]       // 8-byte Folded Reload
	subs		x16, x14, x16
	sbcs	x17, x15, x17
	sbcs	x18, x13, x18
	sbcs	x0, x12, x0
	sbcs	x1, x11, x1
	sbcs	x2, x10, x2
	sbcs	x3, x9, x3
	sbcs	x4, x8, x4
	ngcs	 x5, xzr
	tst	 x5, #0x1
	csel	x14, x14, x16, ne
	ldr	x16, [sp, #88]          // 8-byte Folded Reload
	csel	x15, x15, x17, ne
	csel	x13, x13, x18, ne
	csel	x12, x12, x0, ne
	csel	x11, x11, x1, ne
	csel	x10, x10, x2, ne
	csel	x9, x9, x3, ne
	csel	x8, x8, x4, ne
	stp		x14, x15, [x16]
	stp	x13, x12, [x16, #16]
	stp	x11, x10, [x16, #32]
	stp	x9, x8, [x16, #48]
	add	sp, sp, #736            // =736
	ldp	x29, x30, [sp, #80]     // 8-byte Folded Reload
	ldp	x20, x19, [sp, #64]     // 8-byte Folded Reload
	ldp	x22, x21, [sp, #48]     // 8-byte Folded Reload
	ldp	x24, x23, [sp, #32]     // 8-byte Folded Reload
	ldp	x26, x25, [sp, #16]     // 8-byte Folded Reload
	ldp	x28, x27, [sp], #96     // 8-byte Folded Reload
	ret
.Lfunc_end62:
	.size	mcl_fp_montRed8L, .Lfunc_end62-mcl_fp_montRed8L

	.globl	mcl_fp_montRedNF8L
	.p2align	2
	.type	mcl_fp_montRedNF8L,@function
mcl_fp_montRedNF8L:                     // @mcl_fp_montRedNF8L
// BB#0:
	stp	x28, x27, [sp, #-96]!   // 8-byte Folded Spill
	stp	x26, x25, [sp, #16]     // 8-byte Folded Spill
	stp	x24, x23, [sp, #32]     // 8-byte Folded Spill
	stp	x22, x21, [sp, #48]     // 8-byte Folded Spill
	stp	x20, x19, [sp, #64]     // 8-byte Folded Spill
	stp	x29, x30, [sp, #80]     // 8-byte Folded Spill
	sub	sp, sp, #736            // =736
	mov	 x21, x2
	ldr	x8, [x21, #48]
	mov	 x20, x1
	ldur	x23, [x21, #-8]
	ldp		x29, x19, [x20]
	str	x8, [sp, #72]           // 8-byte Folded Spill
	ldr	x8, [x21, #56]
	ldp	x22, x24, [x20, #48]
	ldp	x25, x26, [x20, #32]
	ldp	x27, x28, [x20, #16]
	str	x8, [sp, #80]           // 8-byte Folded Spill
	ldr	x8, [x21, #32]
	str	x0, [sp, #88]           // 8-byte Folded Spill
	mul		x1, x29, x23
	mov	 x0, x21
	str	x8, [sp, #56]           // 8-byte Folded Spill
	ldr	x8, [x21, #40]
	str	x8, [sp, #64]           // 8-byte Folded Spill
	ldr	x8, [x21, #16]
	str	x8, [sp, #40]           // 8-byte Folded Spill
	ldr	x8, [x21, #24]
	str	x8, [sp, #48]           // 8-byte Folded Spill
	ldr		x8, [x21]
	str	x8, [sp, #24]           // 8-byte Folded Spill
	ldr	x8, [x21, #8]
	str	x8, [sp, #32]           // 8-byte Folded Spill
	add	x8, sp, #656            // =656
	bl	mulPv512x64
	ldr	x14, [sp, #656]
	ldr	x15, [sp, #664]
	ldr	x16, [sp, #672]
	ldr	x13, [sp, #680]
	ldr	x12, [sp, #688]
	cmn		x29, x14
	ldr	x11, [sp, #696]
	adcs	x19, x19, x15
	ldr	x10, [sp, #704]
	adcs	x27, x27, x16
	ldr	x9, [sp, #712]
	adcs	x28, x28, x13
	ldr	x8, [sp, #720]
	ldr	x17, [x20, #64]
	adcs	x25, x25, x12
	adcs	x26, x26, x11
	adcs	x10, x22, x10
	adcs	x24, x24, x9
	adcs	x29, x17, x8
	mul		x1, x19, x23
	add	x8, sp, #576            // =576
	mov	 x0, x21
	str	x10, [sp, #16]          // 8-byte Folded Spill
	adcs	x22, xzr, xzr
	bl	mulPv512x64
	ldr	x15, [sp, #576]
	ldr	x16, [sp, #584]
	ldr	x8, [sp, #640]
	ldr	x14, [sp, #592]
	ldr	x13, [sp, #600]
	ldr	x12, [sp, #608]
	cmn		x19, x15
	adcs	x19, x27, x16
	add		x8, x22, x8
	adcs	x22, x28, x14
	adcs	x25, x25, x13
	ldr	x11, [sp, #616]
	adcs	x26, x26, x12
	ldr	x12, [sp, #16]          // 8-byte Folded Reload
	ldr	x10, [sp, #624]
	ldr	x9, [sp, #632]
	ldr	x17, [x20, #72]
	adcs	x11, x12, x11
	adcs	x24, x24, x10
	adcs	x27, x29, x9
	adcs	x28, x17, x8
	mul		x1, x19, x23
	add	x8, sp, #496            // =496
	mov	 x0, x21
	str	x11, [sp, #16]          // 8-byte Folded Spill
	adcs	x29, xzr, xzr
	bl	mulPv512x64
	ldr	x15, [sp, #496]
	ldp	x16, x14, [sp, #504]
	ldr	x13, [sp, #520]
	ldr	x12, [sp, #528]
	cmn		x19, x15
	adcs	x19, x22, x16
	adcs	x22, x25, x14
	adcs	x25, x26, x13
	ldr	x13, [sp, #16]          // 8-byte Folded Reload
	ldr	x11, [sp, #536]
	ldr	x10, [sp, #544]
	ldr	x8, [sp, #560]
	ldr	x9, [sp, #552]
	ldr	x17, [x20, #80]
	adcs	x12, x13, x12
	adcs	x24, x24, x11
	adcs	x26, x27, x10
	add		x8, x29, x8
	adcs	x27, x28, x9
	adcs	x28, x17, x8
	mul		x1, x19, x23
	add	x8, sp, #416            // =416
	mov	 x0, x21
	str	x12, [sp, #16]          // 8-byte Folded Spill
	adcs	x29, xzr, xzr
	bl	mulPv512x64
	ldr	x15, [sp, #416]
	ldp	x16, x14, [sp, #424]
	ldp	x13, x12, [sp, #440]
	ldp	x11, x10, [sp, #456]
	cmn		x19, x15
	adcs	x19, x22, x16
	adcs	x22, x25, x14
	ldr	x14, [sp, #16]          // 8-byte Folded Reload
	ldp	x9, x8, [sp, #472]
	ldr	x17, [x20, #88]
	mul		x1, x19, x23
	adcs	x13, x14, x13
	adcs	x24, x24, x12
	adcs	x25, x26, x11
	adcs	x26, x27, x10
	add		x8, x29, x8
	adcs	x27, x28, x9
	adcs	x28, x17, x8
	add	x8, sp, #336            // =336
	mov	 x0, x21
	str	x13, [sp, #16]          // 8-byte Folded Spill
	adcs	x29, xzr, xzr
	bl	mulPv512x64
	ldr	x15, [sp, #336]
	ldp	x16, x14, [sp, #344]
	ldp	x13, x12, [sp, #360]
	ldp	x11, x10, [sp, #376]
	cmn		x19, x15
	ldr	x15, [sp, #16]          // 8-byte Folded Reload
	adcs	x19, x22, x16
	ldp	x9, x8, [sp, #392]
	ldr	x17, [x20, #96]
	adcs	x22, x15, x14
	adcs	x13, x24, x13
	adcs	x24, x25, x12
	adcs	x25, x26, x11
	adcs	x26, x27, x10
	add		x8, x29, x8
	adcs	x27, x28, x9
	adcs	x28, x17, x8
	mul		x1, x19, x23
	add	x8, sp, #256            // =256
	mov	 x0, x21
	str	x13, [sp, #16]          // 8-byte Folded Spill
	adcs	x29, xzr, xzr
	bl	mulPv512x64
	ldr	x15, [sp, #256]
	ldp	x16, x14, [sp, #264]
	ldp	x13, x12, [sp, #280]
	ldp	x11, x10, [sp, #296]
	cmn		x19, x15
	ldr	x15, [sp, #16]          // 8-byte Folded Reload
	adcs	x19, x22, x16
	ldp	x9, x8, [sp, #312]
	ldr	x17, [x20, #104]
	adcs	x22, x15, x14
	adcs	x13, x24, x13
	adcs	x24, x25, x12
	adcs	x25, x26, x11
	adcs	x26, x27, x10
	add		x8, x29, x8
	adcs	x27, x28, x9
	adcs	x28, x17, x8
	mul		x1, x19, x23
	add	x8, sp, #176            // =176
	mov	 x0, x21
	str	x13, [sp, #16]          // 8-byte Folded Spill
	adcs	x29, xzr, xzr
	bl	mulPv512x64
	ldr	x15, [sp, #176]
	ldp	x16, x14, [sp, #184]
	ldp	x13, x12, [sp, #200]
	ldp	x11, x10, [sp, #216]
	cmn		x19, x15
	ldr	x15, [sp, #16]          // 8-byte Folded Reload
	adcs	x19, x22, x16
	ldp	x9, x8, [sp, #232]
	ldr	x17, [x20, #112]
	adcs	x22, x15, x14
	adcs	x13, x24, x13
	adcs	x24, x25, x12
	adcs	x25, x26, x11
	adcs	x26, x27, x10
	add		x8, x29, x8
	adcs	x27, x28, x9
	adcs	x28, x17, x8
	mul		x1, x19, x23
	add	x8, sp, #96             // =96
	mov	 x0, x21
	str	x13, [sp, #16]          // 8-byte Folded Spill
	adcs	x29, xzr, xzr
	bl	mulPv512x64
	ldp	x15, x14, [sp, #96]
	ldr	x16, [sp, #112]
	ldp	x13, x12, [sp, #120]
	ldp	x11, x10, [sp, #136]
	cmn		x19, x15
	ldr	x15, [sp, #16]          // 8-byte Folded Reload
	adcs	x14, x22, x14
	ldp	x9, x8, [sp, #152]
	ldr	x17, [x20, #120]
	adcs	x15, x15, x16
	adcs	x13, x24, x13
	adcs	x12, x25, x12
	adcs	x11, x26, x11
	adcs	x10, x27, x10
	add		x8, x29, x8
	adcs	x9, x28, x9
	adcs	x8, x17, x8
	ldp	x16, x17, [sp, #24]     // 8-byte Folded Reload
	ldp	x18, x0, [sp, #40]      // 8-byte Folded Reload
	ldp	x1, x2, [sp, #56]       // 8-byte Folded Reload
	ldp	x3, x4, [sp, #72]       // 8-byte Folded Reload
	subs		x16, x14, x16
	sbcs	x17, x15, x17
	sbcs	x18, x13, x18
	sbcs	x0, x12, x0
	sbcs	x1, x11, x1
	sbcs	x2, x10, x2
	sbcs	x3, x9, x3
	sbcs	x4, x8, x4
	cmp		x4, #0          // =0
	csel	x14, x14, x16, lt
	ldr	x16, [sp, #88]          // 8-byte Folded Reload
	csel	x15, x15, x17, lt
	csel	x13, x13, x18, lt
	csel	x12, x12, x0, lt
	csel	x11, x11, x1, lt
	csel	x10, x10, x2, lt
	csel	x9, x9, x3, lt
	csel	x8, x8, x4, lt
	stp		x14, x15, [x16]
	stp	x13, x12, [x16, #16]
	stp	x11, x10, [x16, #32]
	stp	x9, x8, [x16, #48]
	add	sp, sp, #736            // =736
	ldp	x29, x30, [sp, #80]     // 8-byte Folded Reload
	ldp	x20, x19, [sp, #64]     // 8-byte Folded Reload
	ldp	x22, x21, [sp, #48]     // 8-byte Folded Reload
	ldp	x24, x23, [sp, #32]     // 8-byte Folded Reload
	ldp	x26, x25, [sp, #16]     // 8-byte Folded Reload
	ldp	x28, x27, [sp], #96     // 8-byte Folded Reload
	ret
.Lfunc_end63:
	.size	mcl_fp_montRedNF8L, .Lfunc_end63-mcl_fp_montRedNF8L

	.globl	mcl_fp_addPre8L
	.p2align	2
	.type	mcl_fp_addPre8L,@function
mcl_fp_addPre8L:                        // @mcl_fp_addPre8L
// BB#0:
	ldp	x8, x9, [x2, #48]
	ldp	x12, x13, [x2, #32]
	ldp	x16, x17, [x2, #16]
	ldp		x18, x2, [x2]
	ldp		x3, x4, [x1]
	ldp	x10, x11, [x1, #48]
	ldp	x14, x15, [x1, #32]
	ldp	x5, x1, [x1, #16]
	adds		x18, x18, x3
	adcs	x2, x2, x4
	stp		x18, x2, [x0]
	adcs	x16, x16, x5
	adcs	x17, x17, x1
	adcs	x12, x12, x14
	adcs	x13, x13, x15
	adcs	x10, x8, x10
	adcs	x9, x9, x11
	adcs	x8, xzr, xzr
	stp	x16, x17, [x0, #16]
	stp	x12, x13, [x0, #32]
	stp	x10, x9, [x0, #48]
	mov	 x0, x8
	ret
.Lfunc_end64:
	.size	mcl_fp_addPre8L, .Lfunc_end64-mcl_fp_addPre8L

	.globl	mcl_fp_subPre8L
	.p2align	2
	.type	mcl_fp_subPre8L,@function
mcl_fp_subPre8L:                        // @mcl_fp_subPre8L
// BB#0:
	ldp	x8, x9, [x2, #48]
	ldp	x12, x13, [x2, #32]
	ldp	x16, x17, [x2, #16]
	ldp		x18, x2, [x2]
	ldp		x3, x4, [x1]
	ldp	x10, x11, [x1, #48]
	ldp	x14, x15, [x1, #32]
	ldp	x5, x1, [x1, #16]
	subs		x18, x3, x18
	sbcs	x2, x4, x2
	stp		x18, x2, [x0]
	sbcs	x16, x5, x16
	sbcs	x17, x1, x17
	sbcs	x12, x14, x12
	sbcs	x13, x15, x13
	sbcs	x10, x10, x8
	sbcs	x9, x11, x9
	ngcs	 x8, xzr
	and	x8, x8, #0x1
	stp	x16, x17, [x0, #16]
	stp	x12, x13, [x0, #32]
	stp	x10, x9, [x0, #48]
	mov	 x0, x8
	ret
.Lfunc_end65:
	.size	mcl_fp_subPre8L, .Lfunc_end65-mcl_fp_subPre8L

	.globl	mcl_fp_shr1_8L
	.p2align	2
	.type	mcl_fp_shr1_8L,@function
mcl_fp_shr1_8L:                         // @mcl_fp_shr1_8L
// BB#0:
	ldp		x8, x9, [x1]
	ldp	x10, x11, [x1, #48]
	ldp	x12, x13, [x1, #16]
	ldp	x14, x15, [x1, #32]
	extr	x8, x9, x8, #1
	extr	x9, x12, x9, #1
	extr	x12, x13, x12, #1
	extr	x13, x14, x13, #1
	extr	x14, x15, x14, #1
	extr	x15, x10, x15, #1
	extr	x10, x11, x10, #1
	lsr	x11, x11, #1
	stp		x8, x9, [x0]
	stp	x12, x13, [x0, #16]
	stp	x14, x15, [x0, #32]
	stp	x10, x11, [x0, #48]
	ret
.Lfunc_end66:
	.size	mcl_fp_shr1_8L, .Lfunc_end66-mcl_fp_shr1_8L

	.globl	mcl_fp_add8L
	.p2align	2
	.type	mcl_fp_add8L,@function
mcl_fp_add8L:                           // @mcl_fp_add8L
// BB#0:
	stp	x22, x21, [sp, #-32]!   // 8-byte Folded Spill
	ldp	x8, x9, [x2, #48]
	ldp	x12, x13, [x2, #32]
	ldp	x16, x17, [x2, #16]
	ldp		x18, x2, [x2]
	ldp		x4, x5, [x1]
	ldp	x10, x11, [x1, #48]
	ldp	x14, x15, [x1, #32]
	ldp	x6, x1, [x1, #16]
	adds		x18, x18, x4
	adcs	x2, x2, x5
	stp	x20, x19, [sp, #16]     // 8-byte Folded Spill
	adcs	x16, x16, x6
	adcs	x17, x17, x1
	adcs	x7, x12, x14
	adcs	x19, x13, x15
	adcs	x21, x8, x10
	ldp		x8, x10, [x3]
	ldp	x4, x5, [x3, #48]
	ldp	x1, x6, [x3, #32]
	ldp	x12, x20, [x3, #16]
	adcs	x3, x9, x11
	adcs	x22, xzr, xzr
	subs		x15, x18, x8
	sbcs	x14, x2, x10
	sbcs	x13, x16, x12
	sbcs	x12, x17, x20
	sbcs	x11, x7, x1
	sbcs	x10, x19, x6
	sbcs	x9, x21, x4
	sbcs	x8, x3, x5
	stp	x16, x17, [x0, #16]
	sbcs	x16, x22, xzr
	stp		x18, x2, [x0]
	stp	x7, x19, [x0, #32]
	stp	x21, x3, [x0, #48]
	tbnz	w16, #0, .LBB67_2
// BB#1:                                // %nocarry
	stp		x15, x14, [x0]
	stp	x13, x12, [x0, #16]
	stp	x11, x10, [x0, #32]
	stp	x9, x8, [x0, #48]
.LBB67_2:                               // %carry
	ldp	x20, x19, [sp, #16]     // 8-byte Folded Reload
	ldp	x22, x21, [sp], #32     // 8-byte Folded Reload
	ret
.Lfunc_end67:
	.size	mcl_fp_add8L, .Lfunc_end67-mcl_fp_add8L

	.globl	mcl_fp_addNF8L
	.p2align	2
	.type	mcl_fp_addNF8L,@function
mcl_fp_addNF8L:                         // @mcl_fp_addNF8L
// BB#0:
	ldp	x8, x9, [x1, #48]
	ldp	x12, x13, [x1, #32]
	ldp	x16, x17, [x1, #16]
	ldp		x18, x1, [x1]
	ldp		x4, x5, [x2]
	ldp	x10, x11, [x2, #48]
	ldp	x14, x15, [x2, #32]
	ldp	x6, x2, [x2, #16]
	adds		x18, x4, x18
	adcs	x1, x5, x1
	ldp	x4, x5, [x3, #48]
	adcs	x16, x6, x16
	adcs	x17, x2, x17
	adcs	x12, x14, x12
	adcs	x13, x15, x13
	ldp		x14, x15, [x3]
	ldp	x2, x6, [x3, #32]
	adcs	x8, x10, x8
	ldp	x10, x3, [x3, #16]
	adcs	x9, x11, x9
	subs		x11, x18, x14
	sbcs	x14, x1, x15
	sbcs	x10, x16, x10
	sbcs	x15, x17, x3
	sbcs	x2, x12, x2
	sbcs	x3, x13, x6
	sbcs	x4, x8, x4
	sbcs	x5, x9, x5
	cmp		x5, #0          // =0
	csel	x11, x18, x11, lt
	csel	x14, x1, x14, lt
	csel	x10, x16, x10, lt
	csel	x15, x17, x15, lt
	csel	x12, x12, x2, lt
	csel	x13, x13, x3, lt
	csel	x8, x8, x4, lt
	csel	x9, x9, x5, lt
	stp		x11, x14, [x0]
	stp	x10, x15, [x0, #16]
	stp	x12, x13, [x0, #32]
	stp	x8, x9, [x0, #48]
	ret
.Lfunc_end68:
	.size	mcl_fp_addNF8L, .Lfunc_end68-mcl_fp_addNF8L

	.globl	mcl_fp_sub8L
	.p2align	2
	.type	mcl_fp_sub8L,@function
mcl_fp_sub8L:                           // @mcl_fp_sub8L
// BB#0:
	ldp	x14, x15, [x2, #48]
	ldp	x12, x13, [x2, #32]
	ldp	x10, x11, [x2, #16]
	ldp		x8, x9, [x2]
	ldp		x2, x5, [x1]
	ldp	x16, x17, [x1, #48]
	ldp	x18, x4, [x1, #32]
	ldp	x6, x1, [x1, #16]
	subs		x8, x2, x8
	sbcs	x9, x5, x9
	stp		x8, x9, [x0]
	sbcs	x10, x6, x10
	sbcs	x11, x1, x11
	sbcs	x12, x18, x12
	sbcs	x13, x4, x13
	sbcs	x14, x16, x14
	sbcs	x15, x17, x15
	ngcs	 x16, xzr
	stp	x10, x11, [x0, #16]
	stp	x12, x13, [x0, #32]
	stp	x14, x15, [x0, #48]
	tbnz	w16, #0, .LBB69_2
// BB#1:                                // %nocarry
	ret
.LBB69_2:                               // %carry
	ldp		x2, x4, [x3]
	ldp	x16, x17, [x3, #48]
	ldp	x18, x1, [x3, #32]
	ldp	x5, x3, [x3, #16]
	adds		x8, x2, x8
	adcs	x9, x4, x9
	stp		x8, x9, [x0]
	adcs	x8, x5, x10
	adcs	x9, x3, x11
	stp	x8, x9, [x0, #16]
	adcs	x8, x18, x12
	adcs	x9, x1, x13
	stp	x8, x9, [x0, #32]
	adcs	x8, x16, x14
	adcs	x9, x17, x15
	stp	x8, x9, [x0, #48]
	ret
.Lfunc_end69:
	.size	mcl_fp_sub8L, .Lfunc_end69-mcl_fp_sub8L

	.globl	mcl_fp_subNF8L
	.p2align	2
	.type	mcl_fp_subNF8L,@function
mcl_fp_subNF8L:                         // @mcl_fp_subNF8L
// BB#0:
	add	x9, x1, #24             // =24
	ld1	{ v0.d }[0], [x9]
	add	x8, x1, #16             // =16
	add	x10, x2, #24            // =24
	add	x9, x2, #16             // =16
	mov	 x11, x2
	ld1	{ v1.d }[0], [x8]
	ld1	{ v2.d }[0], [x9]
	ld1	{ v0.d }[1], [x10]
	ld1	{ v3.d }[0], [x11], #8
	ins	v1.d[1], v0.d[0]
	ldr		x8, [x11]
	mov	 x11, x1
	ext	v4.16b, v0.16b, v0.16b, #8
	fmov	x9, d0
	ld1	{ v0.d }[0], [x11], #8
	ins	v3.d[1], x8
	fmov	x12, d3
	ldp	x13, x14, [x2, #48]
	ldr		x11, [x11]
	ldp	x15, x16, [x1, #48]
	ldp	x17, x18, [x2, #32]
	ldp	x2, x1, [x1, #32]
	ins	v0.d[1], x11
	fmov	x6, d0
	ins	v2.d[1], v4.d[0]
	subs		x12, x6, x12
	fmov	x10, d1
	sbcs	x8, x11, x8
	fmov	x11, d2
	fmov	x6, d4
	sbcs	x10, x10, x11
	sbcs	x9, x9, x6
	sbcs	x17, x2, x17
	sbcs	x18, x1, x18
	ldp	x4, x5, [x3, #48]
	ldp	x11, x6, [x3, #32]
	ldp	x1, x2, [x3, #16]
	sbcs	x13, x15, x13
	ldp		x15, x3, [x3]
	sbcs	x14, x16, x14
	asr	x16, x14, #63
	and		x1, x16, x1
	and		x15, x16, x15
	and		x3, x16, x3
	adds		x12, x15, x12
	adcs	x8, x3, x8
	and		x2, x16, x2
	stp		x12, x8, [x0]
	adcs	x8, x1, x10
	and		x11, x16, x11
	adcs	x9, x2, x9
	and		x6, x16, x6
	stp	x8, x9, [x0, #16]
	adcs	x8, x11, x17
	and		x4, x16, x4
	adcs	x9, x6, x18
	and		x16, x16, x5
	stp	x8, x9, [x0, #32]
	adcs	x8, x4, x13
	adcs	x9, x16, x14
	stp	x8, x9, [x0, #48]
	ret
.Lfunc_end70:
	.size	mcl_fp_subNF8L, .Lfunc_end70-mcl_fp_subNF8L

	.globl	mcl_fpDbl_add8L
	.p2align	2
	.type	mcl_fpDbl_add8L,@function
mcl_fpDbl_add8L:                        // @mcl_fpDbl_add8L
// BB#0:
	ldp		x16, x5, [x2]
	ldp		x17, x6, [x1]
	ldp	x8, x9, [x2, #112]
	ldp	x12, x13, [x2, #96]
	ldp	x18, x4, [x2, #80]
	adds		x16, x16, x17
	ldr	x17, [x1, #16]
	str		x16, [x0]
	adcs	x16, x5, x6
	ldp	x5, x6, [x2, #16]
	str	x16, [x0, #8]
	ldp	x10, x11, [x1, #112]
	ldp	x14, x15, [x1, #96]
	adcs	x17, x5, x17
	ldp	x16, x5, [x1, #24]
	str	x17, [x0, #16]
	adcs	x16, x6, x16
	ldp	x17, x6, [x2, #32]
	str	x16, [x0, #24]
	adcs	x17, x17, x5
	ldp	x16, x5, [x1, #40]
	str	x17, [x0, #32]
	adcs	x16, x6, x16
	ldp	x17, x6, [x2, #48]
	str	x16, [x0, #40]
	ldr	x16, [x1, #56]
	adcs	x17, x17, x5
	ldp	x5, x2, [x2, #64]
	str	x17, [x0, #48]
	adcs	x16, x6, x16
	ldp	x17, x6, [x1, #64]
	str	x16, [x0, #56]
	ldp	x16, x1, [x1, #80]
	adcs	x17, x5, x17
	adcs	x2, x2, x6
	adcs	x16, x18, x16
	adcs	x18, x4, x1
	adcs	x12, x12, x14
	adcs	x13, x13, x15
	ldp	x5, x6, [x3, #48]
	ldp	x1, x4, [x3, #32]
	ldp	x14, x15, [x3, #16]
	adcs	x8, x8, x10
	ldp		x10, x3, [x3]
	adcs	x9, x9, x11
	adcs	x11, xzr, xzr
	subs		x10, x17, x10
	sbcs	x3, x2, x3
	sbcs	x14, x16, x14
	sbcs	x15, x18, x15
	sbcs	x1, x12, x1
	sbcs	x4, x13, x4
	sbcs	x5, x8, x5
	sbcs	x6, x9, x6
	sbcs	x11, x11, xzr
	tst	 x11, #0x1
	csel	x10, x17, x10, ne
	csel	x11, x2, x3, ne
	csel	x14, x16, x14, ne
	csel	x15, x18, x15, ne
	csel	x12, x12, x1, ne
	csel	x13, x13, x4, ne
	csel	x8, x8, x5, ne
	csel	x9, x9, x6, ne
	stp	x10, x11, [x0, #64]
	stp	x14, x15, [x0, #80]
	stp	x12, x13, [x0, #96]
	stp	x8, x9, [x0, #112]
	ret
.Lfunc_end71:
	.size	mcl_fpDbl_add8L, .Lfunc_end71-mcl_fpDbl_add8L

	.globl	mcl_fpDbl_sub8L
	.p2align	2
	.type	mcl_fpDbl_sub8L,@function
mcl_fpDbl_sub8L:                        // @mcl_fpDbl_sub8L
// BB#0:
	ldp		x16, x5, [x1]
	ldp		x17, x4, [x2]
	ldp	x10, x8, [x2, #112]
	ldp	x12, x13, [x2, #96]
	ldr	x18, [x1, #80]
	subs		x16, x16, x17
	ldr	x17, [x1, #16]
	str		x16, [x0]
	sbcs	x16, x5, x4
	ldp	x4, x5, [x2, #16]
	str	x16, [x0, #8]
	ldp	x11, x9, [x1, #112]
	ldp	x14, x15, [x1, #96]
	sbcs	x17, x17, x4
	ldp	x16, x4, [x1, #24]
	str	x17, [x0, #16]
	sbcs	x16, x16, x5
	ldp	x17, x5, [x2, #32]
	str	x16, [x0, #24]
	sbcs	x17, x4, x17
	ldp	x16, x4, [x1, #40]
	str	x17, [x0, #32]
	sbcs	x16, x16, x5
	ldp	x17, x5, [x2, #48]
	str	x16, [x0, #40]
	sbcs	x17, x4, x17
	ldp	x16, x4, [x1, #56]
	str	x17, [x0, #48]
	sbcs	x16, x16, x5
	ldp	x17, x5, [x2, #64]
	str	x16, [x0, #56]
	ldr	x16, [x1, #72]
	ldr	x1, [x1, #88]
	sbcs	x17, x4, x17
	ldp	x4, x2, [x2, #80]
	sbcs	x16, x16, x5
	sbcs	x18, x18, x4
	sbcs	x1, x1, x2
	sbcs	x12, x14, x12
	sbcs	x13, x15, x13
	sbcs	x10, x11, x10
	sbcs	x8, x9, x8
	ngcs	 x9, xzr
	ldp	x4, x5, [x3, #48]
	ldp	x14, x2, [x3, #32]
	ldp	x11, x15, [x3, #16]
	tst	 x9, #0x1
	ldp		x9, x3, [x3]
	csel	x5, x5, xzr, ne
	csel	x4, x4, xzr, ne
	csel	x2, x2, xzr, ne
	csel	x9, x9, xzr, ne
	csel	x14, x14, xzr, ne
	csel	x15, x15, xzr, ne
	csel	x11, x11, xzr, ne
	csel	x3, x3, xzr, ne
	adds		x9, x9, x17
	adcs	x16, x3, x16
	stp	x9, x16, [x0, #64]
	adcs	x9, x11, x18
	adcs	x11, x15, x1
	stp	x9, x11, [x0, #80]
	adcs	x9, x14, x12
	adcs	x11, x2, x13
	stp	x9, x11, [x0, #96]
	adcs	x9, x4, x10
	adcs	x8, x5, x8
	stp	x9, x8, [x0, #112]
	ret
.Lfunc_end72:
	.size	mcl_fpDbl_sub8L, .Lfunc_end72-mcl_fpDbl_sub8L


	.section	".note.GNU-stack","",@progbits
