	.text
	.syntax unified
	.eabi_attribute	67, "2.09"	@ Tag_conformance
	.eabi_attribute	6, 1	@ Tag_CPU_arch
	.eabi_attribute	8, 1	@ Tag_ARM_ISA_use
	.eabi_attribute	34, 1	@ Tag_CPU_unaligned_access
	.eabi_attribute	15, 1	@ Tag_ABI_PCS_RW_data
	.eabi_attribute	16, 1	@ Tag_ABI_PCS_RO_data
	.eabi_attribute	17, 2	@ Tag_ABI_PCS_GOT_use
	.eabi_attribute	20, 1	@ Tag_ABI_FP_denormal
	.eabi_attribute	21, 1	@ Tag_ABI_FP_exceptions
	.eabi_attribute	23, 3	@ Tag_ABI_FP_number_model
	.eabi_attribute	24, 1	@ Tag_ABI_align_needed
	.eabi_attribute	25, 1	@ Tag_ABI_align_preserved
	.eabi_attribute	28, 1	@ Tag_ABI_VFP_args
	.eabi_attribute	38, 1	@ Tag_ABI_FP_16bit_format
	.eabi_attribute	14, 0	@ Tag_ABI_PCS_R9_use
	.file	"base32.ll"
	.globl	makeNIST_P192L                  @ -- Begin function makeNIST_P192L
	.p2align	2
	.type	makeNIST_P192L,%function
	.code	32                              @ @makeNIST_P192L
makeNIST_P192L:
	.fnstart
@ %bb.0:
	mvn	r1, #0
	mvn	r2, #1
	str	r1, [r0]
	stmib	r0, {r1, r2}
	str	r1, [r0, #12]
	str	r1, [r0, #16]
	str	r1, [r0, #20]
	mov	pc, lr
.Lfunc_end0:
	.size	makeNIST_P192L, .Lfunc_end0-makeNIST_P192L
	.cantunwind
	.fnend
                                        @ -- End function
	.globl	mcl_fpDbl_mod_NIST_P192L        @ -- Begin function mcl_fpDbl_mod_NIST_P192L
	.p2align	2
	.type	mcl_fpDbl_mod_NIST_P192L,%function
	.code	32                              @ @mcl_fpDbl_mod_NIST_P192L
mcl_fpDbl_mod_NIST_P192L:
	.fnstart
@ %bb.0:
	.save	{r4, r5, r6, r7, r8, r9, r10, r11, lr}
	push	{r4, r5, r6, r7, r8, r9, r10, r11, lr}
	add	lr, r1, #28
	add	r9, r1, #8
	ldm	r1, {r10, r12}
	ldr	r4, [r1, #24]
	ldr	r2, [r1, #40]
	ldr	r3, [r1, #44]
	adds	r10, r10, r4
	ldm	lr, {r1, r5, lr}
	ldm	r9, {r6, r7, r8, r9}
	adcs	r11, r12, r1
	adcs	r12, r6, r5
	mov	r6, #0
	adcs	r7, r7, lr
	adcs	r8, r8, r2
	adcs	r9, r9, r3
	adc	r6, r6, #0
	adds	r10, r10, r2
	adcs	r11, r11, r3
	adcs	r4, r12, r4
	adcs	r12, r7, r1
	mov	r1, #0
	adcs	r5, r8, r5
	adcs	r7, r9, lr
	adcs	r6, r6, #0
	adc	r1, r1, #0
	adds	r2, r4, r2
	adcs	lr, r12, r3
	adcs	r3, r5, #0
	adcs	r7, r7, #0
	mrs	r5, apsr
	adcs	r4, r6, #0
	adc	r1, r1, #0
	msr	APSR_nzcvq, r5
	adcs	r12, r10, r6
	adcs	r8, r1, r11
	adcs	r9, r4, r2
	adcs	r10, r1, lr
	mov	r1, #0
	adcs	r11, r3, #0
	adcs	r7, r7, #0
	adc	r6, r1, #0
	adds	lr, r12, #1
	adcs	r5, r8, #0
	adcs	r2, r9, #1
	adcs	r4, r10, #0
	adcs	r1, r11, #0
	adcs	r3, r7, #0
	sbc	r6, r6, #0
	ands	r6, r6, #1
	movne	r3, r7
	movne	r1, r11
	movne	r4, r10
	cmp	r6, #0
	movne	r2, r9
	movne	r5, r8
	movne	lr, r12
	str	r3, [r0, #20]
	str	r1, [r0, #16]
	str	r4, [r0, #12]
	str	r2, [r0, #8]
	str	r5, [r0, #4]
	str	lr, [r0]
	pop	{r4, r5, r6, r7, r8, r9, r10, r11, lr}
	mov	pc, lr
.Lfunc_end1:
	.size	mcl_fpDbl_mod_NIST_P192L, .Lfunc_end1-mcl_fpDbl_mod_NIST_P192L
	.cantunwind
	.fnend
                                        @ -- End function
	.globl	mcl_fp_sqr_NIST_P192L           @ -- Begin function mcl_fp_sqr_NIST_P192L
	.p2align	2
	.type	mcl_fp_sqr_NIST_P192L,%function
	.code	32                              @ @mcl_fp_sqr_NIST_P192L
mcl_fp_sqr_NIST_P192L:
	.fnstart
@ %bb.0:
	.save	{r4, r5, r6, r7, r8, r9, r10, r11, lr}
	push	{r4, r5, r6, r7, r8, r9, r10, r11, lr}
	.pad	#60
	sub	sp, sp, #60
	mov	r8, r0
	add	r0, sp, #12
	bl	mcl_fpDbl_sqrPre6L
	add	r5, sp, #16
	ldr	r3, [sp, #12]
	ldr	r6, [sp, #36]
	ldr	r2, [sp, #40]
	ldm	r5, {r0, r1, r5}
	adds	r10, r3, r6
	ldr	lr, [sp, #44]
	adcs	r4, r0, r2
	ldr	r12, [sp, #48]
	adcs	r9, r1, lr
	ldr	r7, [sp, #52]
	ldr	r0, [sp, #28]
	adcs	r11, r5, r12
	ldr	r1, [sp, #56]
	ldr	r3, [sp, #32]
	adcs	r5, r0, r7
	mov	r0, #0
	adcs	r3, r3, r1
	adc	r0, r0, #0
	adds	r10, r10, r7
	adcs	r4, r4, r1
	str	r4, [sp, #4]                    @ 4-byte Spill
	adcs	r6, r9, r6
	mov	r4, #0
	adcs	r2, r11, r2
	adcs	r5, r5, lr
	adcs	r3, r3, r12
	adcs	r0, r0, #0
	adc	r12, r4, #0
	adds	r7, r6, r7
	adcs	lr, r2, r1
	mov	r4, #0
	adcs	r2, r5, #0
	adcs	r3, r3, #0
	mrs	r6, apsr
	adcs	r5, r0, #0
	adc	r1, r12, #0
	msr	APSR_nzcvq, r6
	ldr	r6, [sp, #4]                    @ 4-byte Reload
	adcs	r0, r10, r0
	str	r0, [sp, #8]                    @ 4-byte Spill
	adcs	r12, r1, r6
	adcs	r9, r5, r7
	adcs	r10, r1, lr
	adcs	r11, r2, #0
	adcs	r3, r3, #0
	adc	lr, r4, #0
	adds	r0, r0, #1
	adcs	r6, r12, #0
	adcs	r7, r9, #1
	adcs	r5, r10, #0
	adcs	r1, r11, #0
	adcs	r2, r3, #0
	sbc	r4, lr, #0
	ands	r4, r4, #1
	movne	r1, r11
	movne	r2, r3
	str	r1, [r8, #16]
	movne	r5, r10
	ldr	r1, [sp, #8]                    @ 4-byte Reload
	cmp	r4, #0
	movne	r7, r9
	movne	r6, r12
	str	r2, [r8, #20]
	movne	r0, r1
	str	r5, [r8, #12]
	str	r7, [r8, #8]
	str	r6, [r8, #4]
	str	r0, [r8]
	add	sp, sp, #60
	pop	{r4, r5, r6, r7, r8, r9, r10, r11, lr}
	mov	pc, lr
.Lfunc_end2:
	.size	mcl_fp_sqr_NIST_P192L, .Lfunc_end2-mcl_fp_sqr_NIST_P192L
	.cantunwind
	.fnend
                                        @ -- End function
	.globl	mcl_fp_mulNIST_P192L            @ -- Begin function mcl_fp_mulNIST_P192L
	.p2align	2
	.type	mcl_fp_mulNIST_P192L,%function
	.code	32                              @ @mcl_fp_mulNIST_P192L
mcl_fp_mulNIST_P192L:
	.fnstart
@ %bb.0:
	.save	{r4, r5, r6, r7, r8, r9, r10, r11, lr}
	push	{r4, r5, r6, r7, r8, r9, r10, r11, lr}
	.pad	#60
	sub	sp, sp, #60
	mov	r8, r0
	add	r0, sp, #12
	bl	mcl_fpDbl_mulPre6L
	add	r5, sp, #16
	ldr	r3, [sp, #12]
	ldr	r6, [sp, #36]
	ldr	r2, [sp, #40]
	ldm	r5, {r0, r1, r5}
	adds	r10, r3, r6
	ldr	lr, [sp, #44]
	adcs	r4, r0, r2
	ldr	r12, [sp, #48]
	adcs	r9, r1, lr
	ldr	r7, [sp, #52]
	ldr	r0, [sp, #28]
	adcs	r11, r5, r12
	ldr	r1, [sp, #56]
	ldr	r3, [sp, #32]
	adcs	r5, r0, r7
	mov	r0, #0
	adcs	r3, r3, r1
	adc	r0, r0, #0
	adds	r10, r10, r7
	adcs	r4, r4, r1
	str	r4, [sp, #4]                    @ 4-byte Spill
	adcs	r6, r9, r6
	mov	r4, #0
	adcs	r2, r11, r2
	adcs	r5, r5, lr
	adcs	r3, r3, r12
	adcs	r0, r0, #0
	adc	r12, r4, #0
	adds	r7, r6, r7
	adcs	lr, r2, r1
	mov	r4, #0
	adcs	r2, r5, #0
	adcs	r3, r3, #0
	mrs	r6, apsr
	adcs	r5, r0, #0
	adc	r1, r12, #0
	msr	APSR_nzcvq, r6
	ldr	r6, [sp, #4]                    @ 4-byte Reload
	adcs	r0, r10, r0
	str	r0, [sp, #8]                    @ 4-byte Spill
	adcs	r12, r1, r6
	adcs	r9, r5, r7
	adcs	r10, r1, lr
	adcs	r11, r2, #0
	adcs	r3, r3, #0
	adc	lr, r4, #0
	adds	r0, r0, #1
	adcs	r6, r12, #0
	adcs	r7, r9, #1
	adcs	r5, r10, #0
	adcs	r1, r11, #0
	adcs	r2, r3, #0
	sbc	r4, lr, #0
	ands	r4, r4, #1
	movne	r1, r11
	movne	r2, r3
	str	r1, [r8, #16]
	movne	r5, r10
	ldr	r1, [sp, #8]                    @ 4-byte Reload
	cmp	r4, #0
	movne	r7, r9
	movne	r6, r12
	str	r2, [r8, #20]
	movne	r0, r1
	str	r5, [r8, #12]
	str	r7, [r8, #8]
	str	r6, [r8, #4]
	str	r0, [r8]
	add	sp, sp, #60
	pop	{r4, r5, r6, r7, r8, r9, r10, r11, lr}
	mov	pc, lr
.Lfunc_end3:
	.size	mcl_fp_mulNIST_P192L, .Lfunc_end3-mcl_fp_mulNIST_P192L
	.cantunwind
	.fnend
                                        @ -- End function
	.globl	mcl_fpDbl_mod_NIST_P521L        @ -- Begin function mcl_fpDbl_mod_NIST_P521L
	.p2align	2
	.type	mcl_fpDbl_mod_NIST_P521L,%function
	.code	32                              @ @mcl_fpDbl_mod_NIST_P521L
mcl_fpDbl_mod_NIST_P521L:
	.fnstart
@ %bb.0:
	.save	{r4, r5, r6, r7, r8, r9, r10, r11, lr}
	push	{r4, r5, r6, r7, r8, r9, r10, r11, lr}
	.pad	#60
	sub	sp, sp, #60
	mov	r3, #255
	ldr	r2, [r1, #64]
	orr	r3, r3, #256
	ldr	r7, [r1, #72]
	and	r11, r2, r3
	ldr	r3, [r1, #68]
	lsr	r2, r2, #9
	ldr	r5, [r1, #4]
	ldr	r4, [r1, #8]
	lsr	r6, r3, #9
	orr	r2, r2, r3, lsl #23
	orr	r6, r6, r7, lsl #23
	lsr	r3, r7, #9
	ldr	r7, [r1]
	ldr	lr, [r1, #12]
	adds	r12, r7, r2
	ldr	r2, [r1, #76]
	adcs	r8, r5, r6
	ldr	r5, [r1, #80]
	ldr	r6, [r1, #24]
	orr	r3, r3, r2, lsl #23
	lsr	r2, r2, #9
	adcs	r9, r4, r3
	ldr	r3, [r1, #84]
	orr	r2, r2, r5, lsl #23
	lsr	r7, r5, #9
	adcs	r4, lr, r2
	ldr	r2, [r1, #16]
	orr	r7, r7, r3, lsl #23
	lsr	r3, r3, #9
	adcs	lr, r2, r7
	ldr	r7, [r1, #88]
	ldr	r5, [r1, #28]
	orr	r2, r3, r7, lsl #23
	ldr	r3, [r1, #20]
	adcs	r10, r3, r2
	lsr	r3, r7, #9
	ldr	r7, [r1, #92]
	orr	r3, r3, r7, lsl #23
	lsr	r7, r7, #9
	adcs	r2, r6, r3
	ldr	r6, [r1, #96]
	orr	r7, r7, r6, lsl #23
	adcs	r3, r5, r7
	lsr	r7, r6, #9
	ldr	r6, [r1, #100]
	ldr	r5, [r1, #32]
	orr	r7, r7, r6, lsl #23
	adcs	r5, r5, r7
	lsr	r7, r6, #9
	ldr	r6, [r1, #104]
	str	r5, [sp, #56]                   @ 4-byte Spill
	ldr	r5, [r1, #36]
	orr	r7, r7, r6, lsl #23
	adcs	r5, r5, r7
	lsr	r7, r6, #9
	ldr	r6, [r1, #108]
	str	r5, [sp, #52]                   @ 4-byte Spill
	ldr	r5, [r1, #40]
	orr	r7, r7, r6, lsl #23
	adcs	r5, r5, r7
	lsr	r7, r6, #9
	ldr	r6, [r1, #112]
	str	r5, [sp, #48]                   @ 4-byte Spill
	ldr	r5, [r1, #44]
	orr	r7, r7, r6, lsl #23
	adcs	r5, r5, r7
	lsr	r7, r6, #9
	ldr	r6, [r1, #116]
	str	r5, [sp, #44]                   @ 4-byte Spill
	ldr	r5, [r1, #48]
	orr	r7, r7, r6, lsl #23
	adcs	r5, r5, r7
	lsr	r7, r6, #9
	ldr	r6, [r1, #120]
	str	r5, [sp, #40]                   @ 4-byte Spill
	ldr	r5, [r1, #52]
	orr	r7, r7, r6, lsl #23
	adcs	r5, r5, r7
	lsr	r7, r6, #9
	ldr	r6, [r1, #124]
	str	r5, [sp, #36]                   @ 4-byte Spill
	ldr	r5, [r1, #56]
	orr	r7, r7, r6, lsl #23
	adcs	r5, r5, r7
	lsr	r7, r6, #9
	ldr	r6, [r1, #128]
	ldr	r1, [r1, #60]
	str	r5, [sp, #32]                   @ 4-byte Spill
	orr	r7, r7, r6, lsl #23
	adcs	r7, r1, r7
	mov	r1, #1
	adc	r11, r11, r6, lsr #9
	and	r1, r1, r11, lsr #9
	adds	r1, r1, r12
	str	r1, [sp, #28]                   @ 4-byte Spill
	adcs	r6, r8, #0
	str	r6, [sp, #24]                   @ 4-byte Spill
	adcs	r5, r9, #0
	and	r1, r6, r1
	adcs	r6, r4, #0
	and	r1, r1, r5
	str	r6, [sp, #16]                   @ 4-byte Spill
	and	r1, r1, r6
	adcs	r6, lr, #0
	str	r6, [sp, #12]                   @ 4-byte Spill
	and	r1, r1, r6
	adcs	r6, r10, #0
	adcs	r2, r2, #0
	and	r1, r1, r6
	str	r2, [sp, #4]                    @ 4-byte Spill
	and	r2, r1, r2
	adcs	r1, r3, #0
	ldr	r3, [sp, #56]                   @ 4-byte Reload
	and	r2, r2, r1
	str	r5, [sp, #20]                   @ 4-byte Spill
	adcs	r9, r3, #0
	ldr	r3, [sp, #52]                   @ 4-byte Reload
	and	r2, r2, r9
	str	r6, [sp, #8]                    @ 4-byte Spill
	adcs	r8, r3, #0
	ldr	r3, [sp, #48]                   @ 4-byte Reload
	and	r2, r2, r8
	adcs	lr, r3, #0
	ldr	r3, [sp, #44]                   @ 4-byte Reload
	and	r2, r2, lr
	adcs	r4, r3, #0
	ldr	r3, [sp, #40]                   @ 4-byte Reload
	and	r2, r2, r4
	adcs	r5, r3, #0
	ldr	r3, [sp, #36]                   @ 4-byte Reload
	and	r2, r2, r5
	adcs	r10, r3, #0
	ldr	r3, [sp, #32]                   @ 4-byte Reload
	and	r2, r2, r10
	adcs	r6, r3, #0
	ldr	r3, .LCPI4_0
	and	r2, r2, r6
	adcs	r7, r7, #0
	and	r12, r2, r7
	adc	r2, r11, #0
	orr	r3, r2, r3
	and	r3, r3, r12
	cmn	r3, #1
	beq	.LBB4_2
@ %bb.1:                                @ %nonzero
	mov	r3, #255
	str	r9, [r0, #32]
	orr	r3, r3, #256
	str	r8, [r0, #36]
	and	r2, r2, r3
	str	r2, [r0, #64]
	add	r2, r0, #44
	str	lr, [r0, #40]
	stm	r2, {r4, r5, r10}
	ldr	r2, [sp, #28]                   @ 4-byte Reload
	str	r2, [r0]
	ldr	r2, [sp, #24]                   @ 4-byte Reload
	str	r2, [r0, #4]
	ldr	r2, [sp, #20]                   @ 4-byte Reload
	str	r2, [r0, #8]
	ldr	r2, [sp, #16]                   @ 4-byte Reload
	str	r2, [r0, #12]
	ldr	r2, [sp, #12]                   @ 4-byte Reload
	str	r2, [r0, #16]
	ldr	r2, [sp, #8]                    @ 4-byte Reload
	str	r2, [r0, #20]
	ldr	r2, [sp, #4]                    @ 4-byte Reload
	str	r6, [r0, #56]
	str	r7, [r0, #60]
	str	r2, [r0, #24]
	str	r1, [r0, #28]
	add	sp, sp, #60
	pop	{r4, r5, r6, r7, r8, r9, r10, r11, lr}
	mov	pc, lr
.LBB4_2:                                @ %zero
	mov	r1, #0
	mov	r2, #68
	bl	memset
	add	sp, sp, #60
	pop	{r4, r5, r6, r7, r8, r9, r10, r11, lr}
	mov	pc, lr
	.p2align	2
@ %bb.3:
.LCPI4_0:
	.long	4294966784                      @ 0xfffffe00
.Lfunc_end4:
	.size	mcl_fpDbl_mod_NIST_P521L, .Lfunc_end4-mcl_fpDbl_mod_NIST_P521L
	.cantunwind
	.fnend
                                        @ -- End function
	.globl	mulPv192x32                     @ -- Begin function mulPv192x32
	.p2align	2
	.type	mulPv192x32,%function
	.code	32                              @ @mulPv192x32
mulPv192x32:
	.fnstart
@ %bb.0:
	.save	{r4, r5, r6, r7, r8, r9, r11, lr}
	push	{r4, r5, r6, r7, r8, r9, r11, lr}
	ldr	r12, [r1]
	ldmib	r1, {r3, lr}
	umull	r7, r6, r12, r2
	ldr	r4, [r1, #12]
	umull	r5, r8, r3, r2
	str	r7, [r0]
	umull	r9, r12, r4, r2
	adds	r7, r6, r5
	umull	r5, r4, lr, r2
	adcs	r7, r8, r5
	umlal	r6, r5, r3, r2
	adcs	r7, r4, r9
	str	r7, [r0, #12]
	str	r5, [r0, #8]
	str	r6, [r0, #4]
	ldr	r3, [r1, #16]
	umull	r7, r6, r3, r2
	adcs	r3, r12, r7
	str	r3, [r0, #16]
	ldr	r1, [r1, #20]
	umull	r3, r7, r1, r2
	adcs	r1, r6, r3
	str	r1, [r0, #20]
	adc	r1, r7, #0
	str	r1, [r0, #24]
	pop	{r4, r5, r6, r7, r8, r9, r11, lr}
	mov	pc, lr
.Lfunc_end5:
	.size	mulPv192x32, .Lfunc_end5-mulPv192x32
	.cantunwind
	.fnend
                                        @ -- End function
	.globl	mcl_fp_mulUnitPre6L             @ -- Begin function mcl_fp_mulUnitPre6L
	.p2align	2
	.type	mcl_fp_mulUnitPre6L,%function
	.code	32                              @ @mcl_fp_mulUnitPre6L
mcl_fp_mulUnitPre6L:
	.fnstart
@ %bb.0:
	.save	{r4, r5, r6, r7, r8, r9, r11, lr}
	push	{r4, r5, r6, r7, r8, r9, r11, lr}
	ldr	r12, [r1]
	ldmib	r1, {r3, lr}
	umull	r7, r6, r12, r2
	ldr	r4, [r1, #12]
	umull	r5, r8, r3, r2
	str	r7, [r0]
	umull	r9, r12, r4, r2
	adds	r7, r6, r5
	umull	r5, r4, lr, r2
	adcs	r7, r8, r5
	umlal	r6, r5, r3, r2
	ldr	r3, [r1, #16]
	adcs	r7, r4, r9
	str	r7, [r0, #12]
	str	r6, [r0, #4]
	umull	r7, r6, r3, r2
	ldr	r1, [r1, #20]
	str	r5, [r0, #8]
	adcs	r3, r12, r7
	str	r3, [r0, #16]
	umull	r3, r7, r1, r2
	adcs	r1, r6, r3
	str	r1, [r0, #20]
	adc	r1, r7, #0
	str	r1, [r0, #24]
	pop	{r4, r5, r6, r7, r8, r9, r11, lr}
	mov	pc, lr
.Lfunc_end6:
	.size	mcl_fp_mulUnitPre6L, .Lfunc_end6-mcl_fp_mulUnitPre6L
	.cantunwind
	.fnend
                                        @ -- End function
	.globl	mcl_fpDbl_mulPre6L              @ -- Begin function mcl_fpDbl_mulPre6L
	.p2align	2
	.type	mcl_fpDbl_mulPre6L,%function
	.code	32                              @ @mcl_fpDbl_mulPre6L
mcl_fpDbl_mulPre6L:
	.fnstart
@ %bb.0:
	.save	{r4, r5, r6, r7, r8, r9, r10, r11, lr}
	push	{r4, r5, r6, r7, r8, r9, r10, r11, lr}
	.pad	#148
	sub	sp, sp, #148
	str	r0, [sp, #36]                   @ 4-byte Spill
	mov	r3, r2
	ldm	r2, {r0, r10}
	ldmib	r1, {r4, r12}
	ldr	lr, [r1]
	ldr	r2, [r2, #8]
	umull	r5, r9, r4, r0
	str	r2, [sp, #124]                  @ 4-byte Spill
	ldr	r2, [r3, #12]
	str	r2, [sp, #104]                  @ 4-byte Spill
	umull	r2, r11, lr, r0
	ldr	r6, [r1, #12]
	str	r12, [sp, #144]                 @ 4-byte Spill
	str	r6, [sp, #28]                   @ 4-byte Spill
	str	r2, [sp, #136]                  @ 4-byte Spill
	adds	r5, r11, r5
	str	lr, [sp, #8]                    @ 4-byte Spill
	umull	r2, r5, r12, r0
	ldr	r12, [r1, #20]
	str	r12, [sp, #20]                  @ 4-byte Spill
	adcs	r7, r9, r2
	umlal	r11, r2, r4, r0
	umull	r7, r9, r6, r0
	adcs	r7, r5, r7
	str	r7, [sp, #132]                  @ 4-byte Spill
	ldr	r7, [r1, #16]
	str	r7, [sp, #32]                   @ 4-byte Spill
	umull	r5, r8, r7, r0
	adcs	r6, r9, r5
	umull	r1, r9, r12, r0
	str	r6, [sp, #128]                  @ 4-byte Spill
	adcs	r1, r8, r1
	str	r1, [sp, #120]                  @ 4-byte Spill
	umull	r5, r1, r10, lr
	adc	r0, r9, #0
	str	r0, [sp, #108]                  @ 4-byte Spill
	ldr	r8, [sp, #28]                   @ 4-byte Reload
	adds	r0, r11, r5
	str	r0, [sp, #140]                  @ 4-byte Spill
	umull	r5, r0, r10, r4
	str	r0, [sp, #116]                  @ 4-byte Spill
	adcs	r0, r2, r5
	str	r0, [sp, #100]                  @ 4-byte Spill
	ldr	r0, [sp, #144]                  @ 4-byte Reload
	ldr	r6, [sp, #100]                  @ 4-byte Reload
	umull	r5, r2, r10, r0
	ldr	r0, [sp, #132]                  @ 4-byte Reload
	str	r2, [sp, #112]                  @ 4-byte Spill
	adcs	r5, r0, r5
	umull	r9, r0, r10, r8
	ldr	r2, [sp, #108]                  @ 4-byte Reload
	str	r0, [sp, #132]                  @ 4-byte Spill
	ldr	r0, [sp, #128]                  @ 4-byte Reload
	adcs	r9, r0, r9
	umull	r11, r0, r10, r7
	str	r0, [sp, #128]                  @ 4-byte Spill
	ldr	r0, [sp, #120]                  @ 4-byte Reload
	adcs	r11, r0, r11
	umull	lr, r0, r10, r12
	ldr	r10, [sp, #8]                   @ 4-byte Reload
	adcs	lr, r2, lr
	mov	r2, #0
	adc	r2, r2, #0
	adds	r1, r6, r1
	str	r1, [sp, #96]                   @ 4-byte Spill
	mov	r6, r8
	ldr	r1, [sp, #116]                  @ 4-byte Reload
	adcs	r1, r5, r1
	str	r1, [sp, #92]                   @ 4-byte Spill
	ldr	r1, [sp, #112]                  @ 4-byte Reload
	adcs	r1, r9, r1
	str	r1, [sp, #88]                   @ 4-byte Spill
	ldr	r1, [sp, #132]                  @ 4-byte Reload
	adcs	r1, r11, r1
	str	r1, [sp, #84]                   @ 4-byte Spill
	ldr	r1, [sp, #128]                  @ 4-byte Reload
	ldr	r11, [sp, #144]                 @ 4-byte Reload
	adcs	r1, lr, r1
	str	r1, [sp, #80]                   @ 4-byte Spill
	adc	r0, r2, r0
	str	r0, [sp, #76]                   @ 4-byte Spill
	ldr	r0, [r3, #20]
	umull	r2, r9, r0, r10
	umull	r1, lr, r0, r4
	str	r2, [sp, #120]                  @ 4-byte Spill
	adds	r1, r9, r1
	umull	r5, r1, r0, r11
	adcs	r2, lr, r5
	umlal	r9, r5, r0, r4
	umull	r2, lr, r0, r8
	str	r5, [sp, #108]                  @ 4-byte Spill
	str	r9, [sp, #100]                  @ 4-byte Spill
	mov	r9, r10
	adcs	r1, r1, r2
	str	r1, [sp, #132]                  @ 4-byte Spill
	umull	r1, r2, r0, r7
	adcs	r1, lr, r1
	str	r1, [sp, #128]                  @ 4-byte Spill
	umull	r1, r5, r0, r12
	adcs	r0, r2, r1
	str	r0, [sp, #116]                  @ 4-byte Spill
	adc	r0, r5, #0
	str	r0, [sp, #112]                  @ 4-byte Spill
	ldr	r0, [r3, #16]
	umull	r5, r2, r0, r10
	umull	r1, r3, r0, r4
	str	r5, [sp, #72]                   @ 4-byte Spill
	adds	r1, r2, r1
	umull	r5, r1, r0, r11
	adcs	r3, r3, r5
	umlal	r2, r5, r0, r4
	umull	r3, lr, r0, r8
	str	r2, [sp, #48]                   @ 4-byte Spill
	str	r5, [sp, #52]                   @ 4-byte Spill
	adcs	r1, r1, r3
	str	r1, [sp, #68]                   @ 4-byte Spill
	umull	r1, r3, r0, r7
	ldr	r7, [sp, #124]                  @ 4-byte Reload
	adcs	r1, lr, r1
	str	r1, [sp, #64]                   @ 4-byte Spill
	umull	r1, r2, r0, r12
	adcs	r0, r3, r1
	str	r0, [sp, #60]                   @ 4-byte Spill
	adc	r0, r2, #0
	ldr	r2, [sp, #104]                  @ 4-byte Reload
	str	r0, [sp, #56]                   @ 4-byte Spill
	umull	r1, r8, r2, r10
	mov	r10, r6
	umull	r0, r3, r2, r4
	str	r1, [sp, #44]                   @ 4-byte Spill
	adds	r0, r8, r0
	umull	lr, r0, r2, r11
	adcs	r3, r3, lr
	umlal	r8, lr, r2, r4
	umull	r3, r1, r2, r6
	str	r1, [sp, #12]                   @ 4-byte Spill
	adcs	r0, r0, r3
	str	r0, [sp, #40]                   @ 4-byte Spill
	mov	r0, r2
	umull	r6, r2, r7, r11
	str	r2, [sp, #16]                   @ 4-byte Spill
	umull	r2, r12, r7, r9
	mov	r3, r6
	str	r2, [sp, #24]                   @ 4-byte Spill
	umull	r1, r2, r7, r10
	mov	r5, r12
	stmib	sp, {r1, r2}                    @ 8-byte Folded Spill
	ldr	r2, [sp, #32]                   @ 4-byte Reload
	umull	r11, r1, r7, r4
	umlal	r5, r3, r7, r4
	str	r1, [sp]                        @ 4-byte Spill
	ldr	r1, [sp, #12]                   @ 4-byte Reload
	umull	r4, r10, r0, r2
	adcs	r1, r1, r4
	str	r1, [sp, #144]                  @ 4-byte Spill
	ldr	r1, [sp, #20]                   @ 4-byte Reload
	umull	r4, r9, r0, r1
	adcs	r0, r10, r4
	str	r0, [sp, #104]                  @ 4-byte Spill
	adc	r0, r9, #0
	str	r0, [sp, #28]                   @ 4-byte Spill
	adds	r0, r12, r11
	ldr	r11, [sp, #36]                  @ 4-byte Reload
	ldr	r0, [sp]                        @ 4-byte Reload
	adcs	r0, r0, r6
	umull	r0, r10, r7, r1
	umull	r4, r1, r7, r2
	ldr	r2, [sp, #16]                   @ 4-byte Reload
	ldr	r7, [sp, #4]                    @ 4-byte Reload
	adcs	r9, r2, r7
	ldr	r2, [sp, #8]                    @ 4-byte Reload
	ldr	r7, [sp, #96]                   @ 4-byte Reload
	adcs	r4, r2, r4
	ldr	r2, [sp, #136]                  @ 4-byte Reload
	adcs	r1, r1, r0
	ldr	r0, [sp, #24]                   @ 4-byte Reload
	str	r2, [r11]
	adc	r2, r10, #0
	adds	r10, r0, r7
	ldr	r7, [sp, #92]                   @ 4-byte Reload
	ldr	r0, [sp, #80]                   @ 4-byte Reload
	adcs	r5, r5, r7
	ldr	r7, [sp, #88]                   @ 4-byte Reload
	adcs	r3, r3, r7
	ldr	r7, [sp, #84]                   @ 4-byte Reload
	adcs	r7, r9, r7
	adcs	r4, r4, r0
	ldr	r0, [sp, #76]                   @ 4-byte Reload
	adcs	r6, r1, r0
	ldr	r0, [sp, #44]                   @ 4-byte Reload
	adc	r2, r2, #0
	adds	r9, r0, r5
	ldr	r0, [sp, #40]                   @ 4-byte Reload
	adcs	r3, r8, r3
	ldr	r5, [sp, #72]                   @ 4-byte Reload
	adcs	r1, lr, r7
	adcs	r7, r0, r4
	ldr	r0, [sp, #144]                  @ 4-byte Reload
	adcs	r0, r0, r6
	ldr	r6, [sp, #104]                  @ 4-byte Reload
	adcs	r2, r6, r2
	ldr	r6, [sp, #28]                   @ 4-byte Reload
	adc	r4, r6, #0
	adds	r3, r5, r3
	ldr	r5, [sp, #48]                   @ 4-byte Reload
	adcs	r1, r5, r1
	ldr	r5, [sp, #52]                   @ 4-byte Reload
	adcs	r7, r5, r7
	ldr	r5, [sp, #68]                   @ 4-byte Reload
	adcs	r0, r5, r0
	ldr	r5, [sp, #64]                   @ 4-byte Reload
	adcs	r12, r5, r2
	ldr	r2, [sp, #60]                   @ 4-byte Reload
	adcs	r4, r2, r4
	ldr	r2, [sp, #56]                   @ 4-byte Reload
	adc	r5, r2, #0
	ldr	r2, [sp, #120]                  @ 4-byte Reload
	adds	r1, r2, r1
	ldr	r2, [sp, #100]                  @ 4-byte Reload
	adcs	r7, r2, r7
	ldr	r2, [sp, #108]                  @ 4-byte Reload
	adcs	r0, r2, r0
	ldr	r2, [sp, #140]                  @ 4-byte Reload
	stmib	r11, {r2, r10}
	str	r0, [r11, #28]
	ldr	r0, [sp, #132]                  @ 4-byte Reload
	str	r1, [r11, #20]
	ldr	r1, [sp, #128]                  @ 4-byte Reload
	adcs	r0, r0, r12
	ldr	r2, [sp, #116]                  @ 4-byte Reload
	add	r12, r11, #32
	adcs	r1, r1, r4
	str	r3, [r11, #16]
	ldr	r3, [sp, #112]                  @ 4-byte Reload
	adcs	r2, r2, r5
	str	r9, [r11, #12]
	str	r7, [r11, #24]
	adc	r3, r3, #0
	stm	r12, {r0, r1, r2, r3}
	add	sp, sp, #148
	pop	{r4, r5, r6, r7, r8, r9, r10, r11, lr}
	mov	pc, lr
.Lfunc_end7:
	.size	mcl_fpDbl_mulPre6L, .Lfunc_end7-mcl_fpDbl_mulPre6L
	.cantunwind
	.fnend
                                        @ -- End function
	.globl	mcl_fpDbl_sqrPre6L              @ -- Begin function mcl_fpDbl_sqrPre6L
	.p2align	2
	.type	mcl_fpDbl_sqrPre6L,%function
	.code	32                              @ @mcl_fpDbl_sqrPre6L
mcl_fpDbl_sqrPre6L:
	.fnstart
@ %bb.0:
	.save	{r4, r5, r6, r7, r8, r9, r10, r11, lr}
	push	{r4, r5, r6, r7, r8, r9, r10, r11, lr}
	.pad	#152
	sub	sp, sp, #152
	mov	r5, r0
	ldr	r9, [r1]
	ldmib	r1, {r0, lr}
	ldr	r2, [r1, #20]
	ldr	r12, [r1, #12]
	ldr	r3, [r1, #16]
	umull	r6, r7, r2, r0
	umull	r4, r1, r2, r9
	str	r6, [sp, #60]                   @ 4-byte Spill
	str	r7, [sp, #64]                   @ 4-byte Spill
	str	r4, [sp, #132]                  @ 4-byte Spill
	adds	r4, r1, r6
	str	r1, [sp, #124]                  @ 4-byte Spill
	umull	r4, r1, r2, lr
	str	r4, [sp, #148]                  @ 4-byte Spill
	adcs	r4, r7, r4
	umull	r7, r10, r2, r3
	str	r1, [sp, #44]                   @ 4-byte Spill
	umull	r4, r6, r2, r12
	str	r4, [sp, #140]                  @ 4-byte Spill
	adcs	r4, r1, r4
	str	r4, [sp, #112]                  @ 4-byte Spill
	adcs	r4, r6, r7
	str	r4, [sp, #108]                  @ 4-byte Spill
	umull	r4, r1, r2, r2
	str	r6, [sp, #116]                  @ 4-byte Spill
	umull	r6, r8, r3, r0
	adcs	r4, r10, r4
	str	r4, [sp, #104]                  @ 4-byte Spill
	adc	r4, r1, #0
	str	r4, [sp, #100]                  @ 4-byte Spill
	umull	r4, r1, r3, r9
	str	r6, [sp, #36]                   @ 4-byte Spill
	str	r8, [sp, #24]                   @ 4-byte Spill
	str	r4, [sp, #128]                  @ 4-byte Spill
	str	r1, [sp, #120]                  @ 4-byte Spill
	adds	r4, r1, r6
	umull	r1, r6, r3, lr
	str	r1, [sp, #136]                  @ 4-byte Spill
	adcs	r4, r8, r1
	umull	r8, r11, r3, r3
	str	r6, [sp, #48]                   @ 4-byte Spill
	umull	r4, r1, r3, r12
	adcs	r6, r6, r4
	str	r6, [sp, #96]                   @ 4-byte Spill
	adcs	r6, r1, r8
	str	r6, [sp, #92]                   @ 4-byte Spill
	adcs	r6, r11, r7
	str	r6, [sp, #88]                   @ 4-byte Spill
	adc	r6, r10, #0
	umull	r8, r10, r12, r0
	str	r6, [sp, #84]                   @ 4-byte Spill
	umull	r7, r6, r12, r9
	str	r8, [sp, #32]                   @ 4-byte Spill
	str	r10, [sp, #28]                  @ 4-byte Spill
	str	r7, [sp, #40]                   @ 4-byte Spill
	str	r6, [sp, #16]                   @ 4-byte Spill
	adds	r7, r6, r8
	umull	r6, r8, r12, lr
	str	r6, [sp, #144]                  @ 4-byte Spill
	adcs	r7, r10, r6
	mov	r10, r6
	umull	r7, r6, r12, r12
	adcs	r7, r8, r7
	str	r7, [sp, #80]                   @ 4-byte Spill
	adcs	r4, r6, r4
	str	r4, [sp, #76]                   @ 4-byte Spill
	ldr	r4, [sp, #140]                  @ 4-byte Reload
	adcs	r1, r1, r4
	str	r1, [sp, #72]                   @ 4-byte Spill
	ldr	r1, [sp, #116]                  @ 4-byte Reload
	umull	r4, r7, lr, r0
	adc	r1, r1, #0
	str	r1, [sp, #68]                   @ 4-byte Spill
	umull	r6, r1, lr, r9
	str	r7, [sp]                        @ 4-byte Spill
	str	r6, [sp, #116]                  @ 4-byte Spill
	adds	r6, r1, r4
	str	r1, [sp, #8]                    @ 4-byte Spill
	umull	r1, r6, lr, lr
	str	r1, [sp, #140]                  @ 4-byte Spill
	adcs	r11, r7, r1
	ldr	r11, [sp, #136]                 @ 4-byte Reload
	adcs	r1, r6, r10
	str	r1, [sp, #56]                   @ 4-byte Spill
	adcs	r1, r8, r11
	str	r1, [sp, #52]                   @ 4-byte Spill
	ldr	r1, [sp, #148]                  @ 4-byte Reload
	umull	r8, r10, r0, r9
	ldr	r6, [sp, #48]                   @ 4-byte Reload
	adcs	r6, r6, r1
	str	r6, [sp, #48]                   @ 4-byte Spill
	ldr	r6, [sp, #44]                   @ 4-byte Reload
	str	r8, [sp, #12]                   @ 4-byte Spill
	adc	r6, r6, #0
	str	r6, [sp, #44]                   @ 4-byte Spill
	umull	r7, r6, r0, r0
	str	r7, [sp, #4]                    @ 4-byte Spill
	str	r6, [sp, #20]                   @ 4-byte Spill
	ldr	r6, [sp, #4]                    @ 4-byte Reload
	ldr	r7, [sp]                        @ 4-byte Reload
	adds	r6, r10, r6
	ldr	r6, [sp, #20]                   @ 4-byte Reload
	adcs	r6, r6, r4
	ldr	r6, [sp, #32]                   @ 4-byte Reload
	adcs	r6, r7, r6
	str	r6, [sp, #32]                   @ 4-byte Spill
	ldr	r6, [sp, #36]                   @ 4-byte Reload
	ldr	r7, [sp, #28]                   @ 4-byte Reload
	adcs	r6, r7, r6
	str	r6, [sp, #28]                   @ 4-byte Spill
	ldr	r6, [sp, #60]                   @ 4-byte Reload
	ldr	r7, [sp, #24]                   @ 4-byte Reload
	adcs	r6, r7, r6
	str	r6, [sp, #24]                   @ 4-byte Spill
	ldr	r6, [sp, #124]                  @ 4-byte Reload
	mov	r7, r10
	umlal	r7, r4, r0, r0
	umlal	r6, r1, r2, r0
	str	r1, [sp, #148]                  @ 4-byte Spill
	ldr	r1, [sp, #120]                  @ 4-byte Reload
	str	r6, [sp, #60]                   @ 4-byte Spill
	umlal	r1, r11, r3, r0
	ldr	r3, [sp, #8]                    @ 4-byte Reload
	mov	r8, r3
	str	r11, [sp, #136]                 @ 4-byte Spill
	ldr	r11, [sp, #16]                  @ 4-byte Reload
	str	r1, [sp, #36]                   @ 4-byte Spill
	ldr	r1, [sp, #144]                  @ 4-byte Reload
	mov	r2, r11
	umlal	r2, r1, r12, r0
	ldr	r12, [sp, #116]                 @ 4-byte Reload
	str	r1, [sp, #144]                  @ 4-byte Spill
	ldr	r1, [sp, #140]                  @ 4-byte Reload
	str	r2, [sp, #20]                   @ 4-byte Spill
	umlal	r8, r1, lr, r0
	ldr	lr, [sp, #12]                   @ 4-byte Reload
	str	r1, [sp, #140]                  @ 4-byte Spill
	umull	r1, r2, r9, r9
	str	r1, [sp, #4]                    @ 4-byte Spill
	mov	r1, r12
	mov	r6, r2
	umlal	r6, r1, r0, r9
	ldr	r0, [sp, #64]                   @ 4-byte Reload
	adc	r0, r0, #0
	adds	r2, r2, lr
	str	r0, [sp, #64]                   @ 4-byte Spill
	adcs	r2, r10, r12
	ldr	r12, [sp, #40]                  @ 4-byte Reload
	ldr	r0, [sp, #128]                  @ 4-byte Reload
	adcs	r9, r3, r12
	ldr	r2, [sp, #120]                  @ 4-byte Reload
	adcs	r10, r11, r0
	ldr	r0, [sp, #132]                  @ 4-byte Reload
	adcs	r3, r2, r0
	ldr	r0, [sp, #124]                  @ 4-byte Reload
	adc	r0, r0, #0
	adds	r11, lr, r6
	adcs	r2, r7, r1
	ldr	r1, [sp, #4]                    @ 4-byte Reload
	str	r1, [r5]
	adcs	r1, r4, r9
	ldr	r4, [sp, #32]                   @ 4-byte Reload
	ldr	r6, [sp, #28]                   @ 4-byte Reload
	adcs	r7, r4, r10
	str	r11, [r5, #4]
	adcs	r6, r6, r3
	ldr	r3, [sp, #24]                   @ 4-byte Reload
	adcs	r0, r3, r0
	ldr	r3, [sp, #64]                   @ 4-byte Reload
	adc	r4, r3, #0
	ldr	r3, [sp, #116]                  @ 4-byte Reload
	adds	r9, r3, r2
	ldr	r2, [sp, #20]                   @ 4-byte Reload
	adcs	lr, r8, r1
	ldr	r1, [sp, #140]                  @ 4-byte Reload
	str	r9, [r5, #8]
	adcs	r3, r1, r7
	ldr	r1, [sp, #56]                   @ 4-byte Reload
	adcs	r7, r1, r6
	ldr	r1, [sp, #52]                   @ 4-byte Reload
	ldr	r6, [sp, #128]                  @ 4-byte Reload
	adcs	r0, r1, r0
	ldr	r1, [sp, #48]                   @ 4-byte Reload
	adcs	r1, r1, r4
	ldr	r4, [sp, #44]                   @ 4-byte Reload
	adc	r4, r4, #0
	adds	r8, r12, lr
	adcs	r3, r2, r3
	ldr	r2, [sp, #144]                  @ 4-byte Reload
	str	r8, [r5, #12]
	adcs	r7, r2, r7
	ldr	r2, [sp, #80]                   @ 4-byte Reload
	adcs	r0, r2, r0
	ldr	r2, [sp, #76]                   @ 4-byte Reload
	adcs	r2, r2, r1
	ldr	r1, [sp, #72]                   @ 4-byte Reload
	adcs	r4, r1, r4
	ldr	r1, [sp, #68]                   @ 4-byte Reload
	adc	r1, r1, #0
	adds	r3, r6, r3
	ldr	r6, [sp, #36]                   @ 4-byte Reload
	str	r3, [r5, #16]
	adcs	r7, r6, r7
	ldr	r6, [sp, #136]                  @ 4-byte Reload
	ldr	r3, [sp, #108]                  @ 4-byte Reload
	adcs	r0, r6, r0
	ldr	r6, [sp, #96]                   @ 4-byte Reload
	adcs	r2, r6, r2
	ldr	r6, [sp, #92]                   @ 4-byte Reload
	adcs	r4, r6, r4
	ldr	r6, [sp, #88]                   @ 4-byte Reload
	adcs	r1, r6, r1
	ldr	r6, [sp, #84]                   @ 4-byte Reload
	adc	r12, r6, #0
	ldr	r6, [sp, #132]                  @ 4-byte Reload
	adds	r7, r6, r7
	ldr	r6, [sp, #60]                   @ 4-byte Reload
	str	r7, [r5, #20]
	adcs	r0, r6, r0
	ldr	r6, [sp, #148]                  @ 4-byte Reload
	str	r0, [r5, #24]
	ldr	r0, [sp, #112]                  @ 4-byte Reload
	adcs	r2, r6, r2
	str	r2, [r5, #28]
	adcs	r0, r0, r4
	adcs	r1, r3, r1
	ldr	r3, [sp, #104]                  @ 4-byte Reload
	adcs	r2, r3, r12
	ldr	r3, [sp, #100]                  @ 4-byte Reload
	add	r12, r5, #32
	adc	r3, r3, #0
	stm	r12, {r0, r1, r2, r3}
	add	sp, sp, #152
	pop	{r4, r5, r6, r7, r8, r9, r10, r11, lr}
	mov	pc, lr
.Lfunc_end8:
	.size	mcl_fpDbl_sqrPre6L, .Lfunc_end8-mcl_fpDbl_sqrPre6L
	.cantunwind
	.fnend
                                        @ -- End function
	.globl	mcl_fp_mont6L                   @ -- Begin function mcl_fp_mont6L
	.p2align	2
	.type	mcl_fp_mont6L,%function
	.code	32                              @ @mcl_fp_mont6L
mcl_fp_mont6L:
	.fnstart
@ %bb.0:
	.save	{r4, r5, r6, r7, r8, r9, r10, r11, lr}
	push	{r4, r5, r6, r7, r8, r9, r10, r11, lr}
	.pad	#112
	sub	sp, sp, #112
	str	r0, [sp, #52]                   @ 4-byte Spill
	mov	r0, r2
	ldr	r7, [r0, #8]
	ldr	lr, [r0, #4]
	ldr	r0, [r0, #12]
	str	r2, [sp, #56]                   @ 4-byte Spill
	str	r0, [sp, #48]                   @ 4-byte Spill
	ldr	r0, [r1]
	ldr	r2, [r2]
	str	r7, [sp, #44]                   @ 4-byte Spill
	ldr	r7, [r1, #4]
	umull	r4, r6, r0, r2
	str	r7, [sp, #104]                  @ 4-byte Spill
	ldr	r7, [r3, #-4]
	str	r0, [sp, #64]                   @ 4-byte Spill
	ldr	r5, [r3, #8]
	str	r4, [sp, #40]                   @ 4-byte Spill
	ldr	r9, [r3]
	mul	r0, r7, r4
	str	r5, [sp, #108]                  @ 4-byte Spill
	ldr	r8, [r3, #4]
	str	r7, [sp, #84]                   @ 4-byte Spill
	ldr	r7, [r1, #20]
	ldr	r12, [r1, #8]
	ldr	r11, [r1, #12]
	umull	r10, r4, r0, r5
	ldr	r1, [r1, #16]
	str	r9, [sp, #80]                   @ 4-byte Spill
	str	r1, [sp, #60]                   @ 4-byte Spill
	str	r7, [sp, #72]                   @ 4-byte Spill
	str	r4, [sp, #32]                   @ 4-byte Spill
	umull	r4, r5, r0, r9
	str	r10, [sp, #12]                  @ 4-byte Spill
	str	r11, [sp, #96]                  @ 4-byte Spill
	str	r12, [sp, #68]                  @ 4-byte Spill
	str	r5, [sp, #8]                    @ 4-byte Spill
	umlal	r5, r10, r0, r8
	str	r4, [sp, #36]                   @ 4-byte Spill
	str	r8, [sp, #88]                   @ 4-byte Spill
	str	r5, [sp, #4]                    @ 4-byte Spill
	umull	r5, r4, r7, r2
	ldr	r7, [sp, #104]                  @ 4-byte Reload
	str	r5, [sp, #92]                   @ 4-byte Spill
	str	r4, [sp, #100]                  @ 4-byte Spill
	umull	r5, r4, r1, r2
	umull	r9, r1, r11, r2
	str	r1, [sp, #76]                   @ 4-byte Spill
	umull	r11, r1, r7, r2
	adds	r7, r6, r11
	umull	r7, r11, r12, r2
	ldr	r12, [r3, #16]
	adcs	r1, r1, r7
	adcs	r1, r11, r9
	str	r1, [sp, #28]                   @ 4-byte Spill
	ldr	r1, [sp, #76]                   @ 4-byte Reload
	ldr	r11, [r3, #12]
	adcs	r1, r1, r5
	str	r1, [sp, #24]                   @ 4-byte Spill
	ldr	r1, [sp, #92]                   @ 4-byte Reload
	str	r11, [sp, #92]                  @ 4-byte Spill
	adcs	r1, r4, r1
	str	r1, [sp, #20]                   @ 4-byte Spill
	ldr	r1, [sp, #100]                  @ 4-byte Reload
	ldr	r4, [sp, #8]                    @ 4-byte Reload
	adc	r1, r1, #0
	str	r1, [sp, #16]                   @ 4-byte Spill
	ldr	r1, [r3, #20]
	umull	r3, r9, r0, r8
	str	r1, [sp, #76]                   @ 4-byte Spill
	str	r12, [sp, #100]                 @ 4-byte Spill
	umull	r8, r5, r0, r1
	ldr	r1, [sp, #12]                   @ 4-byte Reload
	adds	r3, r4, r3
	adcs	r1, r9, r1
	ldr	r4, [sp, #32]                   @ 4-byte Reload
	umull	r1, r3, r0, r11
	adcs	r11, r4, r1
	umull	r9, r1, r0, r12
	ldr	r0, [sp, #104]                  @ 4-byte Reload
	umlal	r6, r7, r0, r2
	ldr	r2, [sp, #40]                   @ 4-byte Reload
	adcs	r12, r3, r9
	adcs	r9, r1, r8
	ldr	r1, [sp, #36]                   @ 4-byte Reload
	adc	r3, r5, #0
	mov	r8, #0
	adds	r2, r1, r2
	ldr	r1, [sp, #4]                    @ 4-byte Reload
	mov	r2, r0
	adcs	r1, r1, r6
	str	r1, [sp, #40]                   @ 4-byte Spill
	adcs	r1, r10, r7
	str	r1, [sp, #36]                   @ 4-byte Spill
	ldr	r1, [sp, #28]                   @ 4-byte Reload
	ldr	r6, [sp, #68]                   @ 4-byte Reload
	adcs	r1, r11, r1
	str	r1, [sp, #32]                   @ 4-byte Spill
	ldr	r1, [sp, #24]                   @ 4-byte Reload
	ldr	r11, [sp, #72]                  @ 4-byte Reload
	adcs	r1, r12, r1
	str	r1, [sp, #28]                   @ 4-byte Spill
	ldr	r1, [sp, #20]                   @ 4-byte Reload
	adcs	r1, r9, r1
	str	r1, [sp, #24]                   @ 4-byte Spill
	ldr	r1, [sp, #16]                   @ 4-byte Reload
	umull	r4, r9, lr, r11
	adcs	r1, r3, r1
	str	r1, [sp, #20]                   @ 4-byte Spill
	adc	r1, r8, #0
	str	r1, [sp, #16]                   @ 4-byte Spill
	umull	r3, r5, lr, r0
	ldr	r1, [sp, #96]                   @ 4-byte Reload
	ldr	r0, [sp, #64]                   @ 4-byte Reload
	umull	r12, r7, lr, r1
	umull	r10, r1, lr, r0
	adds	r3, r1, r3
	umull	r0, r3, lr, r6
	adcs	r5, r5, r0
	umlal	r1, r0, lr, r2
	adcs	r8, r3, r12
	ldr	r12, [sp, #60]                  @ 4-byte Reload
	ldr	r2, [sp, #40]                   @ 4-byte Reload
	umull	r3, r5, lr, r12
	adcs	r3, r7, r3
	adcs	r5, r5, r4
	adc	r6, r9, #0
	adds	r7, r2, r10
	ldr	r2, [sp, #36]                   @ 4-byte Reload
	ldr	r10, [sp, #76]                  @ 4-byte Reload
	adcs	r9, r2, r1
	ldr	r1, [sp, #32]                   @ 4-byte Reload
	adcs	r0, r1, r0
	str	r0, [sp, #40]                   @ 4-byte Spill
	ldr	r0, [sp, #28]                   @ 4-byte Reload
	ldr	r1, [sp, #84]                   @ 4-byte Reload
	adcs	r0, r0, r8
	str	r0, [sp, #36]                   @ 4-byte Spill
	ldr	r0, [sp, #24]                   @ 4-byte Reload
	adcs	r0, r0, r3
	str	r0, [sp, #32]                   @ 4-byte Spill
	ldr	r0, [sp, #20]                   @ 4-byte Reload
	adcs	r0, r0, r5
	str	r0, [sp, #28]                   @ 4-byte Spill
	ldr	r0, [sp, #16]                   @ 4-byte Reload
	ldr	r5, [sp, #88]                   @ 4-byte Reload
	adcs	r0, r0, r6
	str	r0, [sp, #24]                   @ 4-byte Spill
	mov	r0, #0
	adc	r0, r0, #0
	str	r0, [sp, #20]                   @ 4-byte Spill
	mul	r0, r1, r7
	ldr	r1, [sp, #108]                  @ 4-byte Reload
	umull	r6, r8, r0, r1
	ldr	r1, [sp, #80]                   @ 4-byte Reload
	umull	r3, r2, r0, r1
	mov	r1, r6
	str	r3, [sp, #16]                   @ 4-byte Spill
	umull	lr, r3, r0, r5
	mov	r4, r2
	umlal	r4, r1, r0, r5
	ldr	r5, [sp, #92]                   @ 4-byte Reload
	adds	r2, r2, lr
	umull	r2, lr, r0, r5
	adcs	r3, r3, r6
	adcs	r8, r8, r2
	ldr	r2, [sp, #100]                  @ 4-byte Reload
	umull	r3, r6, r0, r2
	umull	r5, r2, r0, r10
	adcs	r3, lr, r3
	adcs	r0, r6, r5
	ldr	r6, [sp, #16]                   @ 4-byte Reload
	adc	r2, r2, #0
	adds	r7, r7, r6
	ldr	r6, [sp, #68]                   @ 4-byte Reload
	adcs	r7, r9, r4
	str	r7, [sp, #16]                   @ 4-byte Spill
	ldr	r7, [sp, #40]                   @ 4-byte Reload
	adcs	r1, r7, r1
	str	r1, [sp, #40]                   @ 4-byte Spill
	ldr	r1, [sp, #36]                   @ 4-byte Reload
	ldr	r7, [sp, #104]                  @ 4-byte Reload
	adcs	r1, r1, r8
	str	r1, [sp, #36]                   @ 4-byte Spill
	ldr	r1, [sp, #32]                   @ 4-byte Reload
	ldr	r8, [sp, #64]                   @ 4-byte Reload
	adcs	r1, r1, r3
	str	r1, [sp, #32]                   @ 4-byte Spill
	ldr	r1, [sp, #28]                   @ 4-byte Reload
	ldr	r3, [sp, #44]                   @ 4-byte Reload
	adcs	r0, r1, r0
	str	r0, [sp, #28]                   @ 4-byte Spill
	ldr	r0, [sp, #24]                   @ 4-byte Reload
	adcs	r0, r0, r2
	umull	r2, r1, r3, r11
	str	r0, [sp, #24]                   @ 4-byte Spill
	ldr	r0, [sp, #20]                   @ 4-byte Reload
	str	r1, [sp, #12]                   @ 4-byte Spill
	umull	r9, r1, r3, r12
	adc	r0, r0, #0
	str	r0, [sp, #20]                   @ 4-byte Spill
	str	r2, [sp, #8]                    @ 4-byte Spill
	umull	r11, r0, r3, r8
	str	r1, [sp]                        @ 4-byte Spill
	ldr	r1, [sp, #96]                   @ 4-byte Reload
	umull	r2, r12, r3, r7
	str	r11, [sp, #4]                   @ 4-byte Spill
	umull	r4, r5, r3, r1
	adds	r2, r0, r2
	umull	r1, lr, r3, r6
	adcs	r2, r12, r1
	umlal	r0, r1, r3, r7
	adcs	r2, lr, r4
	adcs	r12, r5, r9
	ldr	r5, [sp, #8]                    @ 4-byte Reload
	ldr	r4, [sp]                        @ 4-byte Reload
	ldr	r3, [sp, #16]                   @ 4-byte Reload
	adcs	r4, r4, r5
	ldr	r5, [sp, #12]                   @ 4-byte Reload
	ldr	r7, [sp, #4]                    @ 4-byte Reload
	adc	r5, r5, #0
	adds	r9, r3, r7
	ldr	r3, [sp, #40]                   @ 4-byte Reload
	ldr	r7, [sp, #88]                   @ 4-byte Reload
	adcs	r0, r3, r0
	str	r0, [sp, #44]                   @ 4-byte Spill
	ldr	r0, [sp, #36]                   @ 4-byte Reload
	adcs	r0, r0, r1
	str	r0, [sp, #40]                   @ 4-byte Spill
	ldr	r0, [sp, #32]                   @ 4-byte Reload
	ldr	r1, [sp, #84]                   @ 4-byte Reload
	adcs	r0, r0, r2
	str	r0, [sp, #36]                   @ 4-byte Spill
	ldr	r0, [sp, #28]                   @ 4-byte Reload
	adcs	r0, r0, r12
	str	r0, [sp, #32]                   @ 4-byte Spill
	ldr	r0, [sp, #24]                   @ 4-byte Reload
	adcs	r0, r0, r4
	str	r0, [sp, #28]                   @ 4-byte Spill
	ldr	r0, [sp, #20]                   @ 4-byte Reload
	adcs	r0, r0, r5
	str	r0, [sp, #24]                   @ 4-byte Spill
	mov	r0, #0
	adc	r0, r0, #0
	str	r0, [sp, #20]                   @ 4-byte Spill
	mul	r0, r1, r9
	ldr	r1, [sp, #108]                  @ 4-byte Reload
	umull	r2, r3, r0, r1
	ldr	r1, [sp, #80]                   @ 4-byte Reload
	str	r3, [sp, #12]                   @ 4-byte Spill
	umull	r3, r4, r0, r1
	mov	r1, r2
	str	r3, [sp, #16]                   @ 4-byte Spill
	umull	r11, r3, r0, r10
	mov	r5, r4
	umlal	r5, r1, r0, r7
	str	r3, [sp]                        @ 4-byte Spill
	ldr	r3, [sp, #100]                  @ 4-byte Reload
	umull	r12, lr, r0, r3
	umull	r10, r3, r0, r7
	ldr	r7, [sp, #92]                   @ 4-byte Reload
	adds	r4, r4, r10
	umull	r4, r10, r0, r7
	adcs	r0, r3, r2
	ldr	r0, [sp, #12]                   @ 4-byte Reload
	adcs	r0, r0, r4
	str	r0, [sp, #8]                    @ 4-byte Spill
	adcs	r0, r10, r12
	str	r0, [sp, #4]                    @ 4-byte Spill
	ldr	r0, [sp]                        @ 4-byte Reload
	adcs	r3, lr, r11
	ldr	r12, [sp, #104]                 @ 4-byte Reload
	adc	r10, r0, #0
	ldr	r0, [sp, #16]                   @ 4-byte Reload
	adds	r7, r9, r0
	ldr	r0, [sp, #48]                   @ 4-byte Reload
	ldr	r7, [sp, #96]                   @ 4-byte Reload
	umull	r4, r11, r0, r12
	umull	r9, r2, r0, r7
	umull	r7, lr, r0, r6
	str	r2, [sp, #12]                   @ 4-byte Spill
	umull	r2, r6, r0, r8
	ldr	r8, [sp, #60]                   @ 4-byte Reload
	str	r2, [sp, #16]                   @ 4-byte Spill
	ldr	r2, [sp, #44]                   @ 4-byte Reload
	adcs	r5, r2, r5
	ldr	r2, [sp, #40]                   @ 4-byte Reload
	adcs	r1, r2, r1
	str	r1, [sp, #44]                   @ 4-byte Spill
	ldr	r1, [sp, #36]                   @ 4-byte Reload
	ldr	r2, [sp, #8]                    @ 4-byte Reload
	adcs	r1, r1, r2
	str	r1, [sp, #40]                   @ 4-byte Spill
	ldr	r1, [sp, #32]                   @ 4-byte Reload
	ldr	r2, [sp, #4]                    @ 4-byte Reload
	adcs	r1, r1, r2
	str	r1, [sp, #36]                   @ 4-byte Spill
	ldr	r1, [sp, #28]                   @ 4-byte Reload
	ldr	r2, [sp, #12]                   @ 4-byte Reload
	adcs	r1, r1, r3
	str	r1, [sp, #32]                   @ 4-byte Spill
	ldr	r1, [sp, #24]                   @ 4-byte Reload
	ldr	r3, [sp, #72]                   @ 4-byte Reload
	adcs	r1, r1, r10
	str	r1, [sp, #28]                   @ 4-byte Spill
	ldr	r1, [sp, #20]                   @ 4-byte Reload
	adc	r10, r1, #0
	adds	r4, r6, r4
	adcs	r4, r11, r7
	umlal	r6, r7, r0, r12
	adcs	lr, lr, r9
	ldr	r11, [sp, #80]                  @ 4-byte Reload
	umull	r1, r4, r0, r8
	adcs	r9, r2, r1
	umull	r2, r1, r0, r3
	ldr	r0, [sp, #16]                   @ 4-byte Reload
	adcs	r2, r4, r2
	adc	r1, r1, #0
	adds	r4, r5, r0
	ldr	r0, [sp, #44]                   @ 4-byte Reload
	adcs	r0, r0, r6
	str	r0, [sp, #48]                   @ 4-byte Spill
	ldr	r0, [sp, #40]                   @ 4-byte Reload
	adcs	r0, r0, r7
	str	r0, [sp, #44]                   @ 4-byte Spill
	ldr	r0, [sp, #36]                   @ 4-byte Reload
	ldr	r7, [sp, #88]                   @ 4-byte Reload
	adcs	r0, r0, lr
	str	r0, [sp, #40]                   @ 4-byte Spill
	ldr	r0, [sp, #32]                   @ 4-byte Reload
	adcs	r0, r0, r9
	str	r0, [sp, #36]                   @ 4-byte Spill
	ldr	r0, [sp, #28]                   @ 4-byte Reload
	ldr	r9, [sp, #92]                   @ 4-byte Reload
	adcs	r0, r0, r2
	str	r0, [sp, #32]                   @ 4-byte Spill
	adcs	r0, r10, r1
	ldr	r10, [sp, #84]                  @ 4-byte Reload
	str	r0, [sp, #28]                   @ 4-byte Spill
	mov	r0, #0
	adc	r0, r0, #0
	str	r0, [sp, #24]                   @ 4-byte Spill
	mul	r0, r10, r4
	ldr	r1, [sp, #108]                  @ 4-byte Reload
	umull	r2, r12, r0, r1
	umull	r1, r6, r0, r11
	umull	lr, r3, r0, r7
	str	r1, [sp, #20]                   @ 4-byte Spill
	mov	r1, r2
	mov	r5, r6
	umlal	r5, r1, r0, r7
	adds	r6, r6, lr
	adcs	r2, r3, r2
	umull	r6, lr, r0, r9
	ldr	r2, [sp, #100]                  @ 4-byte Reload
	adcs	r12, r12, r6
	umull	r3, r6, r0, r2
	adcs	lr, lr, r3
	ldr	r3, [sp, #76]                   @ 4-byte Reload
	umull	r7, r2, r0, r3
	ldr	r3, [sp, #20]                   @ 4-byte Reload
	adcs	r0, r6, r7
	adc	r2, r2, #0
	adds	r7, r4, r3
	ldr	r3, [sp, #48]                   @ 4-byte Reload
	ldr	r7, [sp, #64]                   @ 4-byte Reload
	adcs	r3, r3, r5
	str	r3, [sp, #48]                   @ 4-byte Spill
	ldr	r3, [sp, #44]                   @ 4-byte Reload
	adcs	r1, r3, r1
	str	r1, [sp, #44]                   @ 4-byte Spill
	ldr	r1, [sp, #40]                   @ 4-byte Reload
	adcs	r1, r1, r12
	str	r1, [sp, #40]                   @ 4-byte Spill
	ldr	r1, [sp, #36]                   @ 4-byte Reload
	adcs	r1, r1, lr
	str	r1, [sp, #36]                   @ 4-byte Spill
	ldr	r1, [sp, #32]                   @ 4-byte Reload
	adcs	r0, r1, r0
	str	r0, [sp, #32]                   @ 4-byte Spill
	ldr	r0, [sp, #28]                   @ 4-byte Reload
	ldr	r1, [sp, #104]                  @ 4-byte Reload
	adcs	r0, r0, r2
	str	r0, [sp, #28]                   @ 4-byte Spill
	ldr	r0, [sp, #24]                   @ 4-byte Reload
	adc	r0, r0, #0
	str	r0, [sp, #24]                   @ 4-byte Spill
	ldr	r0, [sp, #56]                   @ 4-byte Reload
	ldr	r4, [r0, #16]
	ldr	r0, [sp, #68]                   @ 4-byte Reload
	umull	r12, lr, r4, r8
	umull	r3, r2, r4, r0
	umull	r8, r0, r4, r7
	ldr	r7, [sp, #96]                   @ 4-byte Reload
	umull	r5, r6, r4, r1
	adds	r5, r0, r5
	adcs	r5, r6, r3
	umlal	r0, r3, r4, r1
	ldr	r1, [sp, #48]                   @ 4-byte Reload
	umull	r5, r6, r4, r7
	ldr	r7, [sp, #72]                   @ 4-byte Reload
	adcs	r2, r2, r5
	adcs	r12, r6, r12
	umull	r5, r6, r4, r7
	adcs	r5, lr, r5
	adc	r6, r6, #0
	adds	r7, r1, r8
	ldr	r1, [sp, #44]                   @ 4-byte Reload
	adcs	r0, r1, r0
	str	r0, [sp, #48]                   @ 4-byte Spill
	ldr	r0, [sp, #40]                   @ 4-byte Reload
	ldr	r1, [sp, #108]                  @ 4-byte Reload
	adcs	r0, r0, r3
	str	r0, [sp, #44]                   @ 4-byte Spill
	ldr	r0, [sp, #36]                   @ 4-byte Reload
	adcs	r0, r0, r2
	str	r0, [sp, #40]                   @ 4-byte Spill
	ldr	r0, [sp, #32]                   @ 4-byte Reload
	adcs	r0, r0, r12
	str	r0, [sp, #36]                   @ 4-byte Spill
	ldr	r0, [sp, #28]                   @ 4-byte Reload
	adcs	r0, r0, r5
	str	r0, [sp, #32]                   @ 4-byte Spill
	ldr	r0, [sp, #24]                   @ 4-byte Reload
	ldr	r5, [sp, #88]                   @ 4-byte Reload
	adcs	r0, r0, r6
	str	r0, [sp, #28]                   @ 4-byte Spill
	mov	r0, #0
	adc	r0, r0, #0
	str	r0, [sp, #24]                   @ 4-byte Spill
	mul	r0, r10, r7
	umull	r8, r2, r0, r11
	ldr	r11, [sp, #64]                  @ 4-byte Reload
	umull	lr, r3, r0, r5
	umull	r6, r12, r0, r1
	mov	r4, r2
	adds	r2, r2, lr
	umull	r2, lr, r0, r9
	adcs	r3, r3, r6
	mov	r1, r6
	umlal	r4, r1, r0, r5
	adcs	r12, r12, r2
	ldr	r2, [sp, #100]                  @ 4-byte Reload
	umull	r3, r6, r0, r2
	adcs	lr, lr, r3
	ldr	r3, [sp, #76]                   @ 4-byte Reload
	umull	r5, r2, r0, r3
	ldr	r3, [sp, #48]                   @ 4-byte Reload
	adcs	r0, r6, r5
	adc	r2, r2, #0
	adds	r7, r7, r8
	adcs	r3, r3, r4
	str	r3, [sp, #48]                   @ 4-byte Spill
	ldr	r3, [sp, #44]                   @ 4-byte Reload
	adcs	r1, r3, r1
	str	r1, [sp, #44]                   @ 4-byte Spill
	ldr	r1, [sp, #40]                   @ 4-byte Reload
	ldr	r3, [sp, #104]                  @ 4-byte Reload
	adcs	r1, r1, r12
	str	r1, [sp, #40]                   @ 4-byte Spill
	ldr	r1, [sp, #36]                   @ 4-byte Reload
	adcs	r1, r1, lr
	str	r1, [sp, #36]                   @ 4-byte Spill
	ldr	r1, [sp, #32]                   @ 4-byte Reload
	adcs	r0, r1, r0
	str	r0, [sp, #32]                   @ 4-byte Spill
	ldr	r0, [sp, #28]                   @ 4-byte Reload
	adcs	r0, r0, r2
	str	r0, [sp, #28]                   @ 4-byte Spill
	ldr	r0, [sp, #24]                   @ 4-byte Reload
	adc	r0, r0, #0
	str	r0, [sp, #24]                   @ 4-byte Spill
	ldr	r0, [sp, #56]                   @ 4-byte Reload
	ldr	r4, [r0, #20]
	ldr	r0, [sp, #72]                   @ 4-byte Reload
	umull	r2, r12, r4, r3
	umull	r9, r1, r4, r0
	ldr	r0, [sp, #60]                   @ 4-byte Reload
	umull	r7, r8, r4, r0
	ldr	r0, [sp, #96]                   @ 4-byte Reload
	str	r1, [sp, #56]                   @ 4-byte Spill
	umull	r5, r6, r4, r0
	ldr	r0, [sp, #68]                   @ 4-byte Reload
	umull	r1, lr, r4, r0
	umull	r10, r0, r4, r11
	ldr	r11, [sp, #100]                 @ 4-byte Reload
	adds	r2, r0, r2
	adcs	r2, r12, r1
	umlal	r0, r1, r4, r3
	adcs	r2, lr, r5
	adcs	r5, r6, r7
	ldr	r6, [sp, #56]                   @ 4-byte Reload
	adcs	r7, r8, r9
	ldr	r3, [sp, #48]                   @ 4-byte Reload
	adc	r6, r6, #0
	ldr	r12, [sp, #88]                  @ 4-byte Reload
	adds	r8, r3, r10
	ldr	r3, [sp, #44]                   @ 4-byte Reload
	ldr	lr, [sp, #76]                   @ 4-byte Reload
	adcs	r0, r3, r0
	str	r0, [sp, #104]                  @ 4-byte Spill
	ldr	r0, [sp, #40]                   @ 4-byte Reload
	adcs	r0, r0, r1
	str	r0, [sp, #96]                   @ 4-byte Spill
	ldr	r0, [sp, #36]                   @ 4-byte Reload
	ldr	r1, [sp, #84]                   @ 4-byte Reload
	adcs	r0, r0, r2
	str	r0, [sp, #72]                   @ 4-byte Spill
	ldr	r0, [sp, #32]                   @ 4-byte Reload
	adcs	r0, r0, r5
	str	r0, [sp, #68]                   @ 4-byte Spill
	ldr	r0, [sp, #28]                   @ 4-byte Reload
	ldr	r5, [sp, #92]                   @ 4-byte Reload
	adcs	r0, r0, r7
	str	r0, [sp, #64]                   @ 4-byte Spill
	ldr	r0, [sp, #24]                   @ 4-byte Reload
	ldr	r7, [sp, #80]                   @ 4-byte Reload
	adcs	r0, r0, r6
	str	r0, [sp, #60]                   @ 4-byte Spill
	mov	r0, #0
	adc	r0, r0, #0
	str	r0, [sp, #56]                   @ 4-byte Spill
	mul	r0, r1, r8
	umull	r3, r4, r0, r7
	umull	r1, r2, r0, r12
	str	r3, [sp, #84]                   @ 4-byte Spill
	ldr	r3, [sp, #108]                  @ 4-byte Reload
	adds	r1, r4, r1
	umull	r6, r1, r0, r3
	adcs	r2, r2, r6
	umlal	r4, r6, r0, r12
	umull	r2, r3, r0, r5
	adcs	r10, r1, r2
	umull	r2, r1, r0, r11
	adcs	r9, r3, r2
	umull	r3, r2, r0, lr
	adcs	r1, r1, r3
	adc	r0, r2, #0
	ldr	r2, [sp, #84]                   @ 4-byte Reload
	adds	r2, r8, r2
	ldr	r2, [sp, #104]                  @ 4-byte Reload
	adcs	r4, r2, r4
	ldr	r2, [sp, #96]                   @ 4-byte Reload
	str	r4, [sp, #104]                  @ 4-byte Spill
	adcs	r5, r2, r6
	ldr	r2, [sp, #72]                   @ 4-byte Reload
	adcs	r3, r2, r10
	ldr	r2, [sp, #68]                   @ 4-byte Reload
	adcs	r8, r2, r9
	ldr	r2, [sp, #64]                   @ 4-byte Reload
	adcs	r9, r2, r1
	ldr	r1, [sp, #60]                   @ 4-byte Reload
	adcs	r0, r1, r0
	ldr	r1, [sp, #56]                   @ 4-byte Reload
	adc	r2, r1, #0
	subs	r10, r4, r7
	ldr	r1, [sp, #108]                  @ 4-byte Reload
	sbcs	r7, r5, r12
	mov	r12, r5
	sbcs	r5, r3, r1
	ldr	r1, [sp, #92]                   @ 4-byte Reload
	sbcs	r6, r8, r1
	sbcs	r1, r9, r11
	sbcs	r4, r0, lr
	sbc	r2, r2, #0
	ands	r2, r2, #1
	movne	r4, r0
	ldr	r0, [sp, #52]                   @ 4-byte Reload
	movne	r1, r9
	movne	r6, r8
	cmp	r2, #0
	str	r1, [r0, #16]
	movne	r5, r3
	ldr	r1, [sp, #104]                  @ 4-byte Reload
	movne	r7, r12
	str	r4, [r0, #20]
	str	r6, [r0, #12]
	movne	r10, r1
	str	r5, [r0, #8]
	str	r7, [r0, #4]
	str	r10, [r0]
	add	sp, sp, #112
	pop	{r4, r5, r6, r7, r8, r9, r10, r11, lr}
	mov	pc, lr
.Lfunc_end9:
	.size	mcl_fp_mont6L, .Lfunc_end9-mcl_fp_mont6L
	.cantunwind
	.fnend
                                        @ -- End function
	.globl	mcl_fp_montNF6L                 @ -- Begin function mcl_fp_montNF6L
	.p2align	2
	.type	mcl_fp_montNF6L,%function
	.code	32                              @ @mcl_fp_montNF6L
mcl_fp_montNF6L:
	.fnstart
@ %bb.0:
	.save	{r4, r5, r6, r7, r8, r9, r10, r11, lr}
	push	{r4, r5, r6, r7, r8, r9, r10, r11, lr}
	.pad	#92
	sub	sp, sp, #92
	str	r0, [sp, #32]                   @ 4-byte Spill
	str	r2, [sp, #36]                   @ 4-byte Spill
	ldm	r2, {r4, r12}
	ldr	r0, [r2, #12]
	ldr	r9, [r2, #8]
	ldr	r2, [r1]
	str	r0, [sp, #28]                   @ 4-byte Spill
	str	r2, [sp, #88]                   @ 4-byte Spill
	ldmib	r1, {r5, r7}
	ldr	r0, [r1, #12]
	mov	r10, r5
	umull	r6, r8, r5, r4
	str	r7, [sp, #56]                   @ 4-byte Spill
	str	r0, [sp, #76]                   @ 4-byte Spill
	umull	r11, r5, r2, r4
	ldr	lr, [r3, #8]
	str	lr, [sp, #40]                   @ 4-byte Spill
	str	r10, [sp, #44]                  @ 4-byte Spill
	adds	r6, r5, r6
	umull	r2, r6, r7, r4
	adcs	r7, r8, r2
	umlal	r5, r2, r10, r4
	umull	r7, r8, r0, r4
	adcs	r0, r6, r7
	ldr	r6, [r1, #16]
	str	r0, [sp, #64]                   @ 4-byte Spill
	str	r6, [sp, #72]                   @ 4-byte Spill
	umull	r7, r0, r6, r4
	ldr	r6, [r3]
	str	r6, [sp, #84]                   @ 4-byte Spill
	adcs	r7, r8, r7
	str	r7, [sp, #60]                   @ 4-byte Spill
	ldr	r7, [r1, #20]
	str	r7, [sp, #80]                   @ 4-byte Spill
	umull	r1, r8, r7, r4
	adcs	r0, r0, r1
	ldr	r1, [r3, #-4]
	str	r0, [sp, #24]                   @ 4-byte Spill
	adc	r0, r8, #0
	str	r0, [sp, #20]                   @ 4-byte Spill
	mul	r0, r1, r11
	str	r1, [sp, #52]                   @ 4-byte Spill
	ldr	r8, [r3, #4]
	str	r8, [sp, #68]                   @ 4-byte Spill
	umull	r1, r7, r0, r6
	str	r7, [sp, #16]                   @ 4-byte Spill
	adds	r1, r11, r1
	umull	r1, r4, r0, r8
	str	r4, [sp, #12]                   @ 4-byte Spill
	adcs	r8, r5, r1
	umull	r5, r11, r0, lr
	ldr	r1, [r3, #12]
	str	r1, [sp, #48]                   @ 4-byte Spill
	adcs	r6, r2, r5
	umull	r5, r7, r0, r1
	ldr	r1, [sp, #64]                   @ 4-byte Reload
	adcs	lr, r1, r5
	ldr	r1, [r3, #16]
	str	r1, [sp, #64]                   @ 4-byte Spill
	umull	r5, r4, r0, r1
	ldr	r1, [sp, #60]                   @ 4-byte Reload
	adcs	r5, r1, r5
	ldr	r1, [r3, #20]
	str	r1, [sp, #60]                   @ 4-byte Spill
	umull	r3, r2, r0, r1
	ldr	r0, [sp, #24]                   @ 4-byte Reload
	ldr	r1, [sp, #20]                   @ 4-byte Reload
	adcs	r0, r0, r3
	adc	r3, r1, #0
	ldr	r1, [sp, #16]                   @ 4-byte Reload
	adds	r1, r8, r1
	str	r1, [sp, #24]                   @ 4-byte Spill
	ldr	r1, [sp, #12]                   @ 4-byte Reload
	ldr	r8, [sp, #56]                   @ 4-byte Reload
	adcs	r1, r6, r1
	str	r1, [sp, #20]                   @ 4-byte Spill
	adcs	r11, lr, r11
	ldr	lr, [sp, #80]                   @ 4-byte Reload
	adcs	r1, r5, r7
	str	r1, [sp, #16]                   @ 4-byte Spill
	adcs	r0, r0, r4
	str	r0, [sp, #12]                   @ 4-byte Spill
	adc	r0, r3, r2
	str	r0, [sp, #8]                    @ 4-byte Spill
	ldr	r0, [sp, #88]                   @ 4-byte Reload
	umull	r3, r6, r12, r10
	ldr	r5, [sp, #76]                   @ 4-byte Reload
	umull	r7, r1, r12, r0
	adds	r3, r1, r3
	umull	r2, r3, r12, r8
	adcs	r6, r6, r2
	umlal	r1, r2, r12, r10
	ldr	r10, [sp, #68]                  @ 4-byte Reload
	umull	r6, r0, r12, r5
	ldr	r5, [sp, #72]                   @ 4-byte Reload
	adcs	r4, r3, r6
	umull	r6, r3, r12, r5
	adcs	r5, r0, r6
	umull	r6, r0, r12, lr
	ldr	r12, [sp, #60]                  @ 4-byte Reload
	adcs	r3, r3, r6
	ldr	r6, [sp, #24]                   @ 4-byte Reload
	adc	r0, r0, #0
	adds	r7, r7, r6
	ldr	r6, [sp, #20]                   @ 4-byte Reload
	adcs	r1, r1, r6
	ldr	r6, [sp, #16]                   @ 4-byte Reload
	adcs	r2, r2, r11
	adcs	r6, r4, r6
	ldr	r4, [sp, #12]                   @ 4-byte Reload
	adcs	r11, r5, r4
	ldr	r5, [sp, #8]                    @ 4-byte Reload
	adcs	r3, r3, r5
	str	r3, [sp, #24]                   @ 4-byte Spill
	adc	r0, r0, #0
	str	r0, [sp, #20]                   @ 4-byte Spill
	ldr	r0, [sp, #52]                   @ 4-byte Reload
	ldr	r3, [sp, #84]                   @ 4-byte Reload
	mul	r4, r0, r7
	umull	r0, r5, r4, r3
	str	r5, [sp, #16]                   @ 4-byte Spill
	adds	r0, r7, r0
	ldr	r5, [sp, #16]                   @ 4-byte Reload
	umull	r0, r3, r4, r10
	str	r3, [sp, #12]                   @ 4-byte Spill
	adcs	r3, r1, r0
	ldr	r0, [sp, #40]                   @ 4-byte Reload
	umull	r1, r7, r4, r0
	ldr	r0, [sp, #48]                   @ 4-byte Reload
	str	r7, [sp, #8]                    @ 4-byte Spill
	adcs	r1, r2, r1
	umull	r2, r7, r4, r0
	str	r7, [sp, #4]                    @ 4-byte Spill
	ldr	r7, [sp, #64]                   @ 4-byte Reload
	adcs	r2, r6, r2
	umull	r6, r0, r4, r7
	adcs	r6, r11, r6
	umull	r7, r11, r4, r12
	ldr	r4, [sp, #24]                   @ 4-byte Reload
	adcs	r4, r4, r7
	ldr	r7, [sp, #20]                   @ 4-byte Reload
	adc	r7, r7, #0
	adds	r3, r3, r5
	str	r3, [sp, #24]                   @ 4-byte Spill
	ldr	r3, [sp, #12]                   @ 4-byte Reload
	adcs	r1, r1, r3
	str	r1, [sp, #20]                   @ 4-byte Spill
	ldr	r1, [sp, #8]                    @ 4-byte Reload
	ldr	r3, [sp, #72]                   @ 4-byte Reload
	adcs	r1, r2, r1
	str	r1, [sp, #16]                   @ 4-byte Spill
	ldr	r1, [sp, #4]                    @ 4-byte Reload
	adcs	r1, r6, r1
	ldr	r6, [sp, #44]                   @ 4-byte Reload
	adcs	r0, r4, r0
	str	r0, [sp, #8]                    @ 4-byte Spill
	ldr	r0, [sp, #88]                   @ 4-byte Reload
	adc	r11, r7, r11
	str	r1, [sp, #12]                   @ 4-byte Spill
	umull	r5, r4, r9, r6
	umull	r12, r1, r9, r0
	ldr	r0, [sp, #76]                   @ 4-byte Reload
	adds	r5, r1, r5
	umull	r2, r5, r9, r8
	ldr	r8, [sp, #52]                   @ 4-byte Reload
	adcs	r4, r4, r2
	umlal	r1, r2, r9, r6
	ldr	r6, [sp, #20]                   @ 4-byte Reload
	umull	r4, r7, r9, r0
	adcs	r4, r5, r4
	umull	r5, r0, r9, r3
	adcs	r5, r7, r5
	umull	r7, r3, r9, lr
	ldr	lr, [sp, #60]                   @ 4-byte Reload
	adcs	r0, r0, r7
	ldr	r7, [sp, #24]                   @ 4-byte Reload
	adc	r3, r3, #0
	adds	r7, r12, r7
	adcs	r1, r1, r6
	ldr	r6, [sp, #16]                   @ 4-byte Reload
	adcs	r2, r2, r6
	ldr	r6, [sp, #12]                   @ 4-byte Reload
	adcs	r6, r4, r6
	ldr	r4, [sp, #8]                    @ 4-byte Reload
	adcs	r9, r5, r4
	mul	r4, r8, r7
	adcs	r0, r0, r11
	str	r0, [sp, #24]                   @ 4-byte Spill
	adc	r0, r3, #0
	ldr	r3, [sp, #84]                   @ 4-byte Reload
	str	r0, [sp, #20]                   @ 4-byte Spill
	ldr	r11, [sp, #40]                  @ 4-byte Reload
	umull	r0, r5, r4, r3
	str	r5, [sp, #16]                   @ 4-byte Spill
	adds	r0, r7, r0
	ldr	r5, [sp, #16]                   @ 4-byte Reload
	umull	r0, r3, r4, r10
	ldr	r10, [sp, #48]                  @ 4-byte Reload
	str	r3, [sp, #12]                   @ 4-byte Spill
	adcs	r0, r1, r0
	umull	r1, r3, r4, r11
	str	r3, [sp, #8]                    @ 4-byte Spill
	adcs	r1, r2, r1
	umull	r2, r12, r4, r10
	ldr	r3, [sp, #64]                   @ 4-byte Reload
	adcs	r2, r6, r2
	umull	r6, r7, r4, r3
	adcs	r6, r9, r6
	umull	r3, r9, r4, lr
	ldr	r4, [sp, #24]                   @ 4-byte Reload
	adcs	r3, r4, r3
	ldr	r4, [sp, #20]                   @ 4-byte Reload
	adc	r4, r4, #0
	adds	r0, r0, r5
	str	r0, [sp, #24]                   @ 4-byte Spill
	ldr	r0, [sp, #12]                   @ 4-byte Reload
	adcs	r0, r1, r0
	str	r0, [sp, #20]                   @ 4-byte Spill
	ldr	r0, [sp, #8]                    @ 4-byte Reload
	ldr	r1, [sp, #56]                   @ 4-byte Reload
	adcs	r0, r2, r0
	str	r0, [sp, #16]                   @ 4-byte Spill
	adcs	r0, r6, r12
	str	r0, [sp, #12]                   @ 4-byte Spill
	adcs	r0, r3, r7
	str	r0, [sp, #8]                    @ 4-byte Spill
	adc	r0, r4, r9
	str	r0, [sp, #4]                    @ 4-byte Spill
	ldr	r0, [sp, #28]                   @ 4-byte Reload
	ldr	r12, [sp, #76]                  @ 4-byte Reload
	ldr	r9, [sp, #80]                   @ 4-byte Reload
	umull	r3, lr, r0, r1
	ldr	r1, [sp, #88]                   @ 4-byte Reload
	umull	r4, r2, r0, r1
	mov	r5, r3
	str	r4, [sp]                        @ 4-byte Spill
	ldr	r4, [sp, #44]                   @ 4-byte Reload
	mov	r1, r2
	umull	r6, r7, r0, r4
	umlal	r1, r5, r0, r4
	adds	r2, r2, r6
	adcs	r2, r7, r3
	umull	r2, r3, r0, r12
	adcs	r2, lr, r2
	ldr	lr, [sp, #72]                   @ 4-byte Reload
	umull	r4, r6, r0, lr
	adcs	r3, r3, r4
	umull	r4, r7, r0, r9
	ldr	r0, [sp, #24]                   @ 4-byte Reload
	ldr	r9, [sp, #64]                   @ 4-byte Reload
	adcs	r4, r6, r4
	adc	r6, r7, #0
	ldr	r7, [sp]                        @ 4-byte Reload
	adds	r0, r7, r0
	ldr	r7, [sp, #20]                   @ 4-byte Reload
	adcs	r1, r1, r7
	ldr	r7, [sp, #16]                   @ 4-byte Reload
	adcs	r7, r5, r7
	ldr	r5, [sp, #12]                   @ 4-byte Reload
	adcs	r2, r2, r5
	ldr	r5, [sp, #8]                    @ 4-byte Reload
	adcs	r3, r3, r5
	str	r3, [sp, #16]                   @ 4-byte Spill
	ldr	r3, [sp, #4]                    @ 4-byte Reload
	adcs	r3, r4, r3
	mul	r4, r8, r0
	ldr	r8, [sp, #84]                   @ 4-byte Reload
	str	r3, [sp, #28]                   @ 4-byte Spill
	adc	r3, r6, #0
	str	r3, [sp, #24]                   @ 4-byte Spill
	umull	r5, r3, r4, r8
	str	r3, [sp, #20]                   @ 4-byte Spill
	ldr	r3, [sp, #68]                   @ 4-byte Reload
	adds	r0, r0, r5
	umull	r0, r5, r4, r3
	str	r5, [sp, #12]                   @ 4-byte Spill
	adcs	r0, r1, r0
	umull	r1, r3, r4, r11
	str	r3, [sp, #8]                    @ 4-byte Spill
	adcs	r1, r7, r1
	umull	r7, r3, r4, r10
	ldr	r10, [sp, #60]                  @ 4-byte Reload
	umull	r6, r11, r4, r10
	str	r3, [sp, #4]                    @ 4-byte Spill
	adcs	r2, r2, r7
	ldr	r3, [sp, #16]                   @ 4-byte Reload
	umull	r7, r5, r4, r9
	ldr	r4, [sp, #28]                   @ 4-byte Reload
	adcs	r7, r3, r7
	ldr	r3, [sp, #20]                   @ 4-byte Reload
	adcs	r4, r4, r6
	ldr	r6, [sp, #24]                   @ 4-byte Reload
	adc	r6, r6, #0
	adds	r0, r0, r3
	str	r0, [sp, #28]                   @ 4-byte Spill
	ldr	r0, [sp, #12]                   @ 4-byte Reload
	adcs	r0, r1, r0
	str	r0, [sp, #24]                   @ 4-byte Spill
	ldr	r0, [sp, #8]                    @ 4-byte Reload
	ldr	r1, [sp, #88]                   @ 4-byte Reload
	adcs	r0, r2, r0
	str	r0, [sp, #20]                   @ 4-byte Spill
	ldr	r0, [sp, #4]                    @ 4-byte Reload
	adcs	r0, r7, r0
	str	r0, [sp, #16]                   @ 4-byte Spill
	adcs	r0, r4, r5
	str	r0, [sp, #12]                   @ 4-byte Spill
	adc	r0, r6, r11
	str	r0, [sp, #8]                    @ 4-byte Spill
	ldr	r0, [sp, #36]                   @ 4-byte Reload
	ldr	r7, [sp, #44]                   @ 4-byte Reload
	ldr	r5, [r0, #16]
	umull	r11, r2, r5, r1
	ldr	r1, [sp, #56]                   @ 4-byte Reload
	umull	r4, r0, r5, r7
	adds	r4, r2, r4
	umull	r3, r4, r5, r1
	adcs	r0, r0, r3
	umlal	r2, r3, r5, r7
	ldr	r7, [sp, #80]                   @ 4-byte Reload
	umull	r0, r6, r5, r12
	adcs	r12, r4, r0
	umull	r4, r1, r5, lr
	adcs	r4, r6, r4
	umull	r6, r0, r5, r7
	ldr	r7, [sp, #28]                   @ 4-byte Reload
	adcs	r1, r1, r6
	adc	r0, r0, #0
	adds	r6, r11, r7
	ldr	r7, [sp, #24]                   @ 4-byte Reload
	adcs	r2, r2, r7
	ldr	r7, [sp, #20]                   @ 4-byte Reload
	adcs	r3, r3, r7
	ldr	r7, [sp, #16]                   @ 4-byte Reload
	adcs	r5, r12, r7
	ldr	r7, [sp, #12]                   @ 4-byte Reload
	adcs	r7, r4, r7
	str	r7, [sp, #16]                   @ 4-byte Spill
	ldr	r7, [sp, #8]                    @ 4-byte Reload
	adcs	r1, r1, r7
	str	r1, [sp, #28]                   @ 4-byte Spill
	adc	r0, r0, #0
	str	r0, [sp, #24]                   @ 4-byte Spill
	ldr	r0, [sp, #52]                   @ 4-byte Reload
	mul	r4, r0, r6
	umull	r0, r1, r4, r8
	ldr	r8, [sp, #40]                   @ 4-byte Reload
	str	r1, [sp, #20]                   @ 4-byte Spill
	ldr	r1, [sp, #68]                   @ 4-byte Reload
	adds	r0, r6, r0
	ldr	r7, [sp, #20]                   @ 4-byte Reload
	umull	r0, r11, r4, r1
	ldr	r1, [sp, #48]                   @ 4-byte Reload
	adcs	r0, r2, r0
	umull	r2, lr, r4, r8
	adcs	r2, r3, r2
	umull	r3, r12, r4, r1
	ldr	r1, [sp, #16]                   @ 4-byte Reload
	adcs	r3, r5, r3
	umull	r5, r6, r4, r9
	adcs	r5, r1, r5
	umull	r1, r9, r4, r10
	ldr	r4, [sp, #28]                   @ 4-byte Reload
	adcs	r1, r4, r1
	ldr	r4, [sp, #24]                   @ 4-byte Reload
	adc	r4, r4, #0
	adds	r0, r0, r7
	adcs	r10, r2, r11
	str	r0, [sp, #28]                   @ 4-byte Spill
	adcs	r11, r3, lr
	ldr	r7, [sp, #44]                   @ 4-byte Reload
	adcs	r0, r5, r12
	str	r0, [sp, #24]                   @ 4-byte Spill
	adcs	r0, r1, r6
	str	r0, [sp, #20]                   @ 4-byte Spill
	adc	r0, r4, r9
	str	r0, [sp, #16]                   @ 4-byte Spill
	ldr	r0, [sp, #36]                   @ 4-byte Reload
	ldr	r5, [r0, #20]
	ldr	r0, [sp, #56]                   @ 4-byte Reload
	umull	r6, r1, r5, r0
	ldr	r0, [sp, #88]                   @ 4-byte Reload
	umull	lr, r3, r5, r0
	umull	r12, r0, r5, r7
	mov	r4, r6
	mov	r2, r3
	umlal	r2, r4, r5, r7
	ldr	r7, [sp, #76]                   @ 4-byte Reload
	adds	r3, r3, r12
	adcs	r0, r0, r6
	umull	r0, r3, r5, r7
	ldr	r7, [sp, #80]                   @ 4-byte Reload
	adcs	r12, r1, r0
	ldr	r0, [sp, #72]                   @ 4-byte Reload
	umull	r1, r6, r5, r0
	adcs	r1, r3, r1
	umull	r3, r0, r5, r7
	ldr	r5, [sp, #28]                   @ 4-byte Reload
	adcs	r3, r6, r3
	adc	r0, r0, #0
	adds	r6, lr, r5
	adcs	r7, r2, r10
	ldr	r2, [sp, #24]                   @ 4-byte Reload
	adcs	r9, r4, r11
	ldr	r5, [sp, #84]                   @ 4-byte Reload
	adcs	r10, r12, r2
	ldr	r2, [sp, #20]                   @ 4-byte Reload
	ldr	lr, [sp, #48]                   @ 4-byte Reload
	adcs	r12, r1, r2
	ldr	r1, [sp, #16]                   @ 4-byte Reload
	ldr	r2, [sp, #68]                   @ 4-byte Reload
	adcs	r1, r3, r1
	str	r1, [sp, #88]                   @ 4-byte Spill
	adc	r0, r0, #0
	str	r0, [sp, #80]                   @ 4-byte Spill
	ldr	r0, [sp, #52]                   @ 4-byte Reload
	mul	r4, r0, r6
	umull	r0, r1, r4, r5
	str	r1, [sp, #76]                   @ 4-byte Spill
	adds	r0, r6, r0
	umull	r6, r0, r4, r8
	str	r0, [sp, #72]                   @ 4-byte Spill
	umull	r3, r0, r4, r2
	str	r0, [sp, #56]                   @ 4-byte Spill
	adcs	r11, r7, r3
	umull	r3, r0, r4, lr
	adcs	r1, r9, r6
	str	r0, [sp, #52]                   @ 4-byte Spill
	adcs	r10, r10, r3
	ldr	r3, [sp, #64]                   @ 4-byte Reload
	umull	r7, r0, r4, r3
	str	r0, [sp, #44]                   @ 4-byte Spill
	adcs	r9, r12, r7
	ldr	r12, [sp, #60]                  @ 4-byte Reload
	ldr	r7, [sp, #56]                   @ 4-byte Reload
	umull	r6, r0, r4, r12
	ldr	r4, [sp, #88]                   @ 4-byte Reload
	str	r0, [sp, #36]                   @ 4-byte Spill
	adcs	r6, r4, r6
	ldr	r4, [sp, #80]                   @ 4-byte Reload
	ldr	r0, [sp, #76]                   @ 4-byte Reload
	adc	r4, r4, #0
	adds	r0, r11, r0
	str	r0, [sp, #88]                   @ 4-byte Spill
	adcs	r1, r1, r7
	ldr	r7, [sp, #72]                   @ 4-byte Reload
	str	r1, [sp, #80]                   @ 4-byte Spill
	adcs	r10, r10, r7
	ldr	r7, [sp, #52]                   @ 4-byte Reload
	adcs	r9, r9, r7
	ldr	r7, [sp, #44]                   @ 4-byte Reload
	adcs	r11, r6, r7
	ldr	r6, [sp, #36]                   @ 4-byte Reload
	adc	r6, r4, r6
	subs	r5, r0, r5
	sbcs	r4, r1, r2
	sbcs	r2, r10, r8
	sbcs	r0, r9, lr
	sbcs	r3, r11, r3
	sbc	r7, r6, r12
	asr	r1, r7, #31
	cmn	r1, #1
	movgt	r6, r7
	ldr	r7, [sp, #32]                   @ 4-byte Reload
	movle	r0, r9
	movle	r3, r11
	cmn	r1, #1
	str	r0, [r7, #12]
	movle	r2, r10
	ldr	r0, [sp, #80]                   @ 4-byte Reload
	str	r6, [r7, #20]
	str	r3, [r7, #16]
	movle	r4, r0
	ldr	r0, [sp, #88]                   @ 4-byte Reload
	str	r2, [r7, #8]
	str	r4, [r7, #4]
	movle	r5, r0
	str	r5, [r7]
	add	sp, sp, #92
	pop	{r4, r5, r6, r7, r8, r9, r10, r11, lr}
	mov	pc, lr
.Lfunc_end10:
	.size	mcl_fp_montNF6L, .Lfunc_end10-mcl_fp_montNF6L
	.cantunwind
	.fnend
                                        @ -- End function
	.globl	mcl_fp_montRed6L                @ -- Begin function mcl_fp_montRed6L
	.p2align	2
	.type	mcl_fp_montRed6L,%function
	.code	32                              @ @mcl_fp_montRed6L
mcl_fp_montRed6L:
	.fnstart
@ %bb.0:
	.save	{r4, r5, r6, r7, r8, r9, r10, r11, lr}
	push	{r4, r5, r6, r7, r8, r9, r10, r11, lr}
	.pad	#72
	sub	sp, sp, #72
	mov	r8, r1
	ldr	r3, [r2, #-4]
	ldr	r6, [r8, #4]
	ldr	r9, [r8]
	str	r6, [sp, #36]                   @ 4-byte Spill
	ldr	r6, [r8, #8]
	str	r6, [sp, #32]                   @ 4-byte Spill
	ldr	r6, [r8, #12]
	str	r6, [sp, #28]                   @ 4-byte Spill
	mul	r6, r9, r3
	ldr	r1, [r2, #4]
	str	r0, [sp, #40]                   @ 4-byte Spill
	ldr	r0, [r2]
	ldr	r7, [r2, #8]
	str	r1, [sp, #68]                   @ 4-byte Spill
	umull	r4, r5, r6, r1
	str	r0, [sp, #56]                   @ 4-byte Spill
	str	r3, [sp, #60]                   @ 4-byte Spill
	str	r7, [sp, #64]                   @ 4-byte Spill
	umull	lr, r1, r6, r0
	umull	r0, r12, r6, r7
	adds	r4, r1, r4
	adcs	r4, r5, r0
	ldr	r4, [r2, #12]
	str	r4, [sp, #48]                   @ 4-byte Spill
	umull	r5, r3, r6, r4
	ldr	r4, [r2, #16]
	str	r4, [sp, #52]                   @ 4-byte Spill
	adcs	r11, r12, r5
	umull	r10, r12, r6, r4
	adcs	r5, r3, r10
	ldr	r3, [r2, #20]
	str	r3, [sp, #44]                   @ 4-byte Spill
	umull	r4, r2, r6, r3
	adcs	r12, r12, r4
	ldr	r4, [sp, #68]                   @ 4-byte Reload
	adc	r2, r2, #0
	adds	r7, lr, r9
	ldr	r7, [sp, #36]                   @ 4-byte Reload
	umlal	r1, r0, r6, r4
	ldr	lr, [sp, #48]                   @ 4-byte Reload
	adcs	r9, r1, r7
	ldr	r1, [sp, #32]                   @ 4-byte Reload
	ldr	r7, [r8, #16]
	adcs	r0, r0, r1
	str	r0, [sp, #36]                   @ 4-byte Spill
	ldr	r0, [sp, #28]                   @ 4-byte Reload
	adcs	r0, r11, r0
	str	r0, [sp, #32]                   @ 4-byte Spill
	adcs	r0, r5, r7
	ldr	r7, [r8, #20]
	str	r0, [sp, #28]                   @ 4-byte Spill
	adcs	r0, r12, r7
	ldr	r7, [r8, #24]
	str	r0, [sp, #24]                   @ 4-byte Spill
	adcs	r0, r2, r7
	ldr	r12, [sp, #60]                  @ 4-byte Reload
	str	r0, [sp, #20]                   @ 4-byte Spill
	mov	r0, #0
	adc	r0, r0, #0
	str	r0, [sp, #16]                   @ 4-byte Spill
	mul	r0, r12, r9
	ldr	r7, [sp, #56]                   @ 4-byte Reload
	ldr	r5, [sp, #64]                   @ 4-byte Reload
	umull	r2, r1, r0, r7
	str	r2, [sp, #12]                   @ 4-byte Spill
	umull	r6, r2, r0, r4
	adds	r1, r6, r1
	str	r1, [sp, #8]                    @ 4-byte Spill
	umull	r6, r1, r0, r5
	adcs	r2, r6, r2
	umull	r6, r4, r0, lr
	str	r2, [sp, #4]                    @ 4-byte Spill
	ldr	r2, [sp, #52]                   @ 4-byte Reload
	adcs	r1, r6, r1
	umull	r6, r10, r0, r2
	str	r1, [sp]                        @ 4-byte Spill
	ldr	r1, [sp, #4]                    @ 4-byte Reload
	adcs	r11, r6, r4
	umull	r6, r4, r0, r3
	adcs	r0, r6, r10
	ldr	r6, [sp, #16]                   @ 4-byte Reload
	adc	r10, r6, r4
	ldr	r4, [sp, #12]                   @ 4-byte Reload
	adds	r6, r4, r9
	ldr	r4, [sp, #36]                   @ 4-byte Reload
	ldr	r6, [sp, #8]                    @ 4-byte Reload
	adcs	r6, r6, r4
	ldr	r4, [sp, #32]                   @ 4-byte Reload
	adcs	r4, r1, r4
	str	r4, [sp, #36]                   @ 4-byte Spill
	ldr	r4, [sp, #28]                   @ 4-byte Reload
	ldr	r1, [sp]                        @ 4-byte Reload
	adcs	r4, r1, r4
	str	r4, [sp, #32]                   @ 4-byte Spill
	ldr	r4, [sp, #24]                   @ 4-byte Reload
	adcs	r1, r11, r4
	str	r1, [sp, #28]                   @ 4-byte Spill
	ldr	r1, [sp, #20]                   @ 4-byte Reload
	ldr	r4, [r8, #28]
	adcs	r0, r0, r1
	str	r0, [sp, #24]                   @ 4-byte Spill
	adcs	r0, r10, r4
	str	r0, [sp, #20]                   @ 4-byte Spill
	mov	r0, #0
	adc	r0, r0, #0
	str	r0, [sp, #16]                   @ 4-byte Spill
	mul	r0, r12, r6
	umull	r1, r4, r0, r7
	str	r1, [sp, #12]                   @ 4-byte Spill
	ldr	r1, [sp, #68]                   @ 4-byte Reload
	umull	r7, r12, r0, r1
	adds	r9, r7, r4
	umull	r7, r11, r0, r5
	adcs	r7, r7, r12
	str	r7, [sp, #8]                    @ 4-byte Spill
	umull	r7, r10, r0, lr
	adcs	r11, r7, r11
	umull	r7, r12, r0, r2
	adcs	r5, r7, r10
	umull	r7, r2, r0, r3
	mov	r10, r3
	ldr	r3, [sp, #16]                   @ 4-byte Reload
	adcs	r0, r7, r12
	ldr	r7, [sp, #8]                    @ 4-byte Reload
	adc	r3, r3, r2
	ldr	r2, [sp, #12]                   @ 4-byte Reload
	ldr	r12, [sp, #52]                  @ 4-byte Reload
	adds	r2, r2, r6
	ldr	r2, [sp, #36]                   @ 4-byte Reload
	adcs	r9, r9, r2
	ldr	r2, [sp, #32]                   @ 4-byte Reload
	adcs	r2, r7, r2
	str	r2, [sp, #36]                   @ 4-byte Spill
	ldr	r2, [sp, #28]                   @ 4-byte Reload
	adcs	r2, r11, r2
	str	r2, [sp, #32]                   @ 4-byte Spill
	ldr	r2, [sp, #24]                   @ 4-byte Reload
	adcs	r2, r5, r2
	str	r2, [sp, #28]                   @ 4-byte Spill
	ldr	r2, [sp, #20]                   @ 4-byte Reload
	ldr	r5, [r8, #32]
	adcs	r0, r0, r2
	str	r0, [sp, #24]                   @ 4-byte Spill
	adcs	r0, r3, r5
	ldr	r3, [sp, #60]                   @ 4-byte Reload
	str	r0, [sp, #20]                   @ 4-byte Spill
	mov	r0, #0
	adc	r0, r0, #0
	str	r0, [sp, #16]                   @ 4-byte Spill
	mul	r0, r3, r9
	ldr	r5, [sp, #56]                   @ 4-byte Reload
	umull	r2, r6, r0, r5
	umull	r7, r4, r0, r1
	str	r2, [sp, #12]                   @ 4-byte Spill
	adds	r1, r7, r6
	str	r1, [sp, #8]                    @ 4-byte Spill
	ldr	r1, [sp, #64]                   @ 4-byte Reload
	umull	r7, r2, r0, r1
	adcs	r7, r7, r4
	str	r7, [sp, #4]                    @ 4-byte Spill
	umull	r7, r4, r0, lr
	adcs	r11, r7, r2
	umull	r7, r6, r0, r12
	adcs	r4, r7, r4
	umull	r7, r2, r0, r10
	mov	r10, r5
	adcs	r0, r7, r6
	ldr	r6, [sp, #16]                   @ 4-byte Reload
	adc	r7, r6, r2
	ldr	r2, [sp, #12]                   @ 4-byte Reload
	ldr	r6, [sp, #8]                    @ 4-byte Reload
	adds	r2, r2, r9
	ldr	r2, [sp, #36]                   @ 4-byte Reload
	adcs	r9, r6, r2
	ldr	r2, [sp, #32]                   @ 4-byte Reload
	ldr	r6, [sp, #4]                    @ 4-byte Reload
	adcs	r2, r6, r2
	str	r2, [sp, #36]                   @ 4-byte Spill
	ldr	r2, [sp, #28]                   @ 4-byte Reload
	adcs	r2, r11, r2
	str	r2, [sp, #32]                   @ 4-byte Spill
	ldr	r2, [sp, #24]                   @ 4-byte Reload
	adcs	r2, r4, r2
	str	r2, [sp, #28]                   @ 4-byte Spill
	ldr	r2, [sp, #20]                   @ 4-byte Reload
	ldr	r4, [r8, #36]
	adcs	r0, r0, r2
	str	r0, [sp, #24]                   @ 4-byte Spill
	adcs	r0, r7, r4
	str	r0, [sp, #20]                   @ 4-byte Spill
	mov	r0, #0
	adc	r0, r0, #0
	str	r0, [sp, #16]                   @ 4-byte Spill
	mul	r0, r3, r9
	umull	r2, r6, r0, r5
	ldr	r5, [sp, #68]                   @ 4-byte Reload
	umull	r7, r3, r0, r5
	str	r2, [sp, #12]                   @ 4-byte Spill
	adds	r11, r7, r6
	umull	r7, r2, r0, r1
	adcs	r6, r7, r3
	umull	r7, r1, r0, lr
	ldr	lr, [sp, #44]                   @ 4-byte Reload
	adcs	r4, r7, r2
	umull	r7, r2, r0, r12
	adcs	r12, r7, r1
	umull	r7, r1, r0, lr
	adcs	r0, r7, r2
	ldr	r2, [sp, #16]                   @ 4-byte Reload
	adc	r1, r2, r1
	ldr	r2, [sp, #12]                   @ 4-byte Reload
	adds	r2, r2, r9
	ldr	r2, [sp, #36]                   @ 4-byte Reload
	adcs	r3, r11, r2
	ldr	r2, [sp, #32]                   @ 4-byte Reload
	adcs	r2, r6, r2
	str	r2, [sp, #36]                   @ 4-byte Spill
	ldr	r2, [sp, #28]                   @ 4-byte Reload
	adcs	r2, r4, r2
	str	r2, [sp, #32]                   @ 4-byte Spill
	ldr	r2, [sp, #24]                   @ 4-byte Reload
	ldr	r4, [r8, #40]
	adcs	r2, r12, r2
	str	r2, [sp, #28]                   @ 4-byte Spill
	ldr	r2, [sp, #20]                   @ 4-byte Reload
	ldr	r12, [sp, #52]                  @ 4-byte Reload
	adcs	r0, r0, r2
	str	r0, [sp, #24]                   @ 4-byte Spill
	adcs	r0, r1, r4
	ldr	r1, [sp, #60]                   @ 4-byte Reload
	str	r0, [sp, #20]                   @ 4-byte Spill
	mov	r0, #0
	adc	r0, r0, #0
	str	r0, [sp, #16]                   @ 4-byte Spill
	mul	r0, r1, r3
	umull	r1, r6, r0, r10
	str	r1, [sp, #60]                   @ 4-byte Spill
	umull	r7, r1, r0, r5
	ldr	r5, [sp, #48]                   @ 4-byte Reload
	adds	r11, r7, r6
	ldr	r6, [sp, #64]                   @ 4-byte Reload
	umull	r7, r2, r0, r6
	adcs	r10, r7, r1
	umull	r7, r1, r0, r5
	adcs	r9, r7, r2
	umull	r7, r2, r0, r12
	adcs	r4, r7, r1
	umull	r7, r1, r0, lr
	adcs	r0, r7, r2
	ldr	r2, [sp, #16]                   @ 4-byte Reload
	adc	r1, r2, r1
	ldr	r2, [sp, #60]                   @ 4-byte Reload
	adds	r2, r2, r3
	ldr	r3, [sp, #32]                   @ 4-byte Reload
	ldr	r2, [sp, #36]                   @ 4-byte Reload
	adcs	r2, r11, r2
	adcs	lr, r10, r3
	ldr	r3, [sp, #28]                   @ 4-byte Reload
	adcs	r9, r9, r3
	ldr	r3, [sp, #24]                   @ 4-byte Reload
	adcs	r10, r4, r3
	ldr	r3, [sp, #20]                   @ 4-byte Reload
	adcs	r11, r0, r3
	ldr	r3, [r8, #44]
	ldr	r0, [sp, #56]                   @ 4-byte Reload
	mov	r8, r2
	adc	r1, r1, r3
	subs	r3, r2, r0
	ldr	r0, [sp, #68]                   @ 4-byte Reload
	sbcs	r4, lr, r0
	ldr	r0, [sp, #44]                   @ 4-byte Reload
	sbcs	r2, r9, r6
	sbcs	r7, r10, r5
	sbcs	r6, r11, r12
	sbcs	r5, r1, r0
	mov	r0, #0
	sbc	r0, r0, #0
	ands	r0, r0, #1
	movne	r5, r1
	ldr	r1, [sp, #40]                   @ 4-byte Reload
	movne	r6, r11
	movne	r7, r10
	cmp	r0, #0
	movne	r2, r9
	movne	r4, lr
	movne	r3, r8
	str	r5, [r1, #20]
	str	r6, [r1, #16]
	str	r7, [r1, #12]
	str	r2, [r1, #8]
	str	r4, [r1, #4]
	str	r3, [r1]
	add	sp, sp, #72
	pop	{r4, r5, r6, r7, r8, r9, r10, r11, lr}
	mov	pc, lr
.Lfunc_end11:
	.size	mcl_fp_montRed6L, .Lfunc_end11-mcl_fp_montRed6L
	.cantunwind
	.fnend
                                        @ -- End function
	.globl	mcl_fp_montRedNF6L              @ -- Begin function mcl_fp_montRedNF6L
	.p2align	2
	.type	mcl_fp_montRedNF6L,%function
	.code	32                              @ @mcl_fp_montRedNF6L
mcl_fp_montRedNF6L:
	.fnstart
@ %bb.0:
	.save	{r4, r5, r6, r7, r8, r9, r10, r11, lr}
	push	{r4, r5, r6, r7, r8, r9, r10, r11, lr}
	.pad	#72
	sub	sp, sp, #72
	mov	r8, r1
	ldr	r3, [r2, #-4]
	ldr	r6, [r8, #4]
	ldr	r9, [r8]
	str	r6, [sp, #36]                   @ 4-byte Spill
	ldr	r6, [r8, #8]
	str	r6, [sp, #32]                   @ 4-byte Spill
	ldr	r6, [r8, #12]
	str	r6, [sp, #28]                   @ 4-byte Spill
	mul	r6, r9, r3
	ldr	r1, [r2, #4]
	str	r0, [sp, #40]                   @ 4-byte Spill
	ldr	r0, [r2]
	ldr	r7, [r2, #8]
	str	r1, [sp, #68]                   @ 4-byte Spill
	umull	r4, r5, r6, r1
	str	r0, [sp, #56]                   @ 4-byte Spill
	str	r3, [sp, #60]                   @ 4-byte Spill
	str	r7, [sp, #64]                   @ 4-byte Spill
	umull	lr, r1, r6, r0
	umull	r0, r12, r6, r7
	adds	r4, r1, r4
	adcs	r4, r5, r0
	ldr	r4, [r2, #12]
	str	r4, [sp, #48]                   @ 4-byte Spill
	umull	r5, r3, r6, r4
	ldr	r4, [r2, #16]
	str	r4, [sp, #52]                   @ 4-byte Spill
	adcs	r11, r12, r5
	umull	r10, r12, r6, r4
	adcs	r5, r3, r10
	ldr	r3, [r2, #20]
	str	r3, [sp, #44]                   @ 4-byte Spill
	umull	r4, r2, r6, r3
	adcs	r12, r12, r4
	ldr	r4, [sp, #68]                   @ 4-byte Reload
	adc	r2, r2, #0
	adds	r7, lr, r9
	ldr	r7, [sp, #36]                   @ 4-byte Reload
	umlal	r1, r0, r6, r4
	ldr	lr, [sp, #48]                   @ 4-byte Reload
	adcs	r9, r1, r7
	ldr	r1, [sp, #32]                   @ 4-byte Reload
	ldr	r7, [r8, #16]
	adcs	r0, r0, r1
	str	r0, [sp, #36]                   @ 4-byte Spill
	ldr	r0, [sp, #28]                   @ 4-byte Reload
	adcs	r0, r11, r0
	str	r0, [sp, #32]                   @ 4-byte Spill
	adcs	r0, r5, r7
	ldr	r7, [r8, #20]
	str	r0, [sp, #28]                   @ 4-byte Spill
	adcs	r0, r12, r7
	ldr	r7, [r8, #24]
	str	r0, [sp, #24]                   @ 4-byte Spill
	adcs	r0, r2, r7
	ldr	r12, [sp, #60]                  @ 4-byte Reload
	str	r0, [sp, #20]                   @ 4-byte Spill
	mov	r0, #0
	adc	r0, r0, #0
	str	r0, [sp, #16]                   @ 4-byte Spill
	mul	r0, r12, r9
	ldr	r7, [sp, #56]                   @ 4-byte Reload
	ldr	r5, [sp, #64]                   @ 4-byte Reload
	umull	r2, r1, r0, r7
	str	r2, [sp, #12]                   @ 4-byte Spill
	umull	r6, r2, r0, r4
	adds	r1, r6, r1
	str	r1, [sp, #8]                    @ 4-byte Spill
	umull	r6, r1, r0, r5
	adcs	r2, r6, r2
	umull	r6, r4, r0, lr
	str	r2, [sp, #4]                    @ 4-byte Spill
	ldr	r2, [sp, #52]                   @ 4-byte Reload
	adcs	r1, r6, r1
	umull	r6, r10, r0, r2
	str	r1, [sp]                        @ 4-byte Spill
	ldr	r1, [sp, #4]                    @ 4-byte Reload
	adcs	r11, r6, r4
	umull	r6, r4, r0, r3
	adcs	r0, r6, r10
	ldr	r6, [sp, #16]                   @ 4-byte Reload
	adc	r10, r6, r4
	ldr	r4, [sp, #12]                   @ 4-byte Reload
	adds	r6, r4, r9
	ldr	r4, [sp, #36]                   @ 4-byte Reload
	ldr	r6, [sp, #8]                    @ 4-byte Reload
	adcs	r6, r6, r4
	ldr	r4, [sp, #32]                   @ 4-byte Reload
	adcs	r4, r1, r4
	str	r4, [sp, #36]                   @ 4-byte Spill
	ldr	r4, [sp, #28]                   @ 4-byte Reload
	ldr	r1, [sp]                        @ 4-byte Reload
	adcs	r4, r1, r4
	str	r4, [sp, #32]                   @ 4-byte Spill
	ldr	r4, [sp, #24]                   @ 4-byte Reload
	adcs	r1, r11, r4
	str	r1, [sp, #28]                   @ 4-byte Spill
	ldr	r1, [sp, #20]                   @ 4-byte Reload
	ldr	r4, [r8, #28]
	adcs	r0, r0, r1
	str	r0, [sp, #24]                   @ 4-byte Spill
	adcs	r0, r10, r4
	str	r0, [sp, #20]                   @ 4-byte Spill
	mov	r0, #0
	adc	r0, r0, #0
	str	r0, [sp, #16]                   @ 4-byte Spill
	mul	r0, r12, r6
	umull	r1, r4, r0, r7
	str	r1, [sp, #12]                   @ 4-byte Spill
	ldr	r1, [sp, #68]                   @ 4-byte Reload
	umull	r7, r12, r0, r1
	adds	r9, r7, r4
	umull	r7, r11, r0, r5
	adcs	r7, r7, r12
	str	r7, [sp, #8]                    @ 4-byte Spill
	umull	r7, r10, r0, lr
	adcs	r11, r7, r11
	umull	r7, r12, r0, r2
	adcs	r5, r7, r10
	umull	r7, r2, r0, r3
	mov	r10, r3
	ldr	r3, [sp, #16]                   @ 4-byte Reload
	adcs	r0, r7, r12
	ldr	r7, [sp, #8]                    @ 4-byte Reload
	adc	r3, r3, r2
	ldr	r2, [sp, #12]                   @ 4-byte Reload
	ldr	r12, [sp, #52]                  @ 4-byte Reload
	adds	r2, r2, r6
	ldr	r2, [sp, #36]                   @ 4-byte Reload
	adcs	r9, r9, r2
	ldr	r2, [sp, #32]                   @ 4-byte Reload
	adcs	r2, r7, r2
	str	r2, [sp, #36]                   @ 4-byte Spill
	ldr	r2, [sp, #28]                   @ 4-byte Reload
	adcs	r2, r11, r2
	str	r2, [sp, #32]                   @ 4-byte Spill
	ldr	r2, [sp, #24]                   @ 4-byte Reload
	adcs	r2, r5, r2
	str	r2, [sp, #28]                   @ 4-byte Spill
	ldr	r2, [sp, #20]                   @ 4-byte Reload
	ldr	r5, [r8, #32]
	adcs	r0, r0, r2
	str	r0, [sp, #24]                   @ 4-byte Spill
	adcs	r0, r3, r5
	ldr	r3, [sp, #60]                   @ 4-byte Reload
	str	r0, [sp, #20]                   @ 4-byte Spill
	mov	r0, #0
	adc	r0, r0, #0
	str	r0, [sp, #16]                   @ 4-byte Spill
	mul	r0, r3, r9
	ldr	r5, [sp, #56]                   @ 4-byte Reload
	umull	r2, r6, r0, r5
	umull	r7, r4, r0, r1
	str	r2, [sp, #12]                   @ 4-byte Spill
	adds	r1, r7, r6
	str	r1, [sp, #8]                    @ 4-byte Spill
	ldr	r1, [sp, #64]                   @ 4-byte Reload
	umull	r7, r2, r0, r1
	adcs	r7, r7, r4
	str	r7, [sp, #4]                    @ 4-byte Spill
	umull	r7, r4, r0, lr
	adcs	r11, r7, r2
	umull	r7, r6, r0, r12
	adcs	r4, r7, r4
	umull	r7, r2, r0, r10
	mov	r10, r5
	adcs	r0, r7, r6
	ldr	r6, [sp, #16]                   @ 4-byte Reload
	adc	r7, r6, r2
	ldr	r2, [sp, #12]                   @ 4-byte Reload
	ldr	r6, [sp, #8]                    @ 4-byte Reload
	adds	r2, r2, r9
	ldr	r2, [sp, #36]                   @ 4-byte Reload
	adcs	r9, r6, r2
	ldr	r2, [sp, #32]                   @ 4-byte Reload
	ldr	r6, [sp, #4]                    @ 4-byte Reload
	adcs	r2, r6, r2
	str	r2, [sp, #36]                   @ 4-byte Spill
	ldr	r2, [sp, #28]                   @ 4-byte Reload
	adcs	r2, r11, r2
	str	r2, [sp, #32]                   @ 4-byte Spill
	ldr	r2, [sp, #24]                   @ 4-byte Reload
	adcs	r2, r4, r2
	str	r2, [sp, #28]                   @ 4-byte Spill
	ldr	r2, [sp, #20]                   @ 4-byte Reload
	ldr	r4, [r8, #36]
	adcs	r0, r0, r2
	str	r0, [sp, #24]                   @ 4-byte Spill
	adcs	r0, r7, r4
	str	r0, [sp, #20]                   @ 4-byte Spill
	mov	r0, #0
	adc	r0, r0, #0
	str	r0, [sp, #16]                   @ 4-byte Spill
	mul	r0, r3, r9
	umull	r2, r6, r0, r5
	ldr	r5, [sp, #68]                   @ 4-byte Reload
	umull	r7, r3, r0, r5
	str	r2, [sp, #12]                   @ 4-byte Spill
	adds	r11, r7, r6
	umull	r7, r2, r0, r1
	adcs	r6, r7, r3
	umull	r7, r1, r0, lr
	ldr	lr, [sp, #44]                   @ 4-byte Reload
	adcs	r4, r7, r2
	umull	r7, r2, r0, r12
	adcs	r12, r7, r1
	umull	r7, r1, r0, lr
	adcs	r0, r7, r2
	ldr	r2, [sp, #16]                   @ 4-byte Reload
	adc	r1, r2, r1
	ldr	r2, [sp, #12]                   @ 4-byte Reload
	adds	r2, r2, r9
	ldr	r2, [sp, #36]                   @ 4-byte Reload
	adcs	r3, r11, r2
	ldr	r2, [sp, #32]                   @ 4-byte Reload
	adcs	r2, r6, r2
	str	r2, [sp, #36]                   @ 4-byte Spill
	ldr	r2, [sp, #28]                   @ 4-byte Reload
	adcs	r2, r4, r2
	str	r2, [sp, #32]                   @ 4-byte Spill
	ldr	r2, [sp, #24]                   @ 4-byte Reload
	ldr	r4, [r8, #40]
	adcs	r2, r12, r2
	str	r2, [sp, #28]                   @ 4-byte Spill
	ldr	r2, [sp, #20]                   @ 4-byte Reload
	ldr	r12, [sp, #52]                  @ 4-byte Reload
	adcs	r0, r0, r2
	str	r0, [sp, #24]                   @ 4-byte Spill
	adcs	r0, r1, r4
	ldr	r1, [sp, #60]                   @ 4-byte Reload
	str	r0, [sp, #20]                   @ 4-byte Spill
	mov	r0, #0
	adc	r0, r0, #0
	str	r0, [sp, #16]                   @ 4-byte Spill
	mul	r0, r1, r3
	umull	r1, r6, r0, r10
	str	r1, [sp, #60]                   @ 4-byte Spill
	umull	r7, r1, r0, r5
	ldr	r5, [sp, #48]                   @ 4-byte Reload
	adds	r11, r7, r6
	ldr	r6, [sp, #64]                   @ 4-byte Reload
	umull	r7, r2, r0, r6
	adcs	r10, r7, r1
	umull	r7, r1, r0, r5
	adcs	r9, r7, r2
	umull	r7, r2, r0, r12
	adcs	r4, r7, r1
	umull	r7, r1, r0, lr
	adcs	r0, r7, r2
	ldr	r2, [sp, #16]                   @ 4-byte Reload
	adc	r1, r2, r1
	ldr	r2, [sp, #60]                   @ 4-byte Reload
	adds	r2, r2, r3
	ldr	r3, [sp, #32]                   @ 4-byte Reload
	ldr	r2, [sp, #36]                   @ 4-byte Reload
	adcs	r2, r11, r2
	adcs	lr, r10, r3
	ldr	r3, [sp, #28]                   @ 4-byte Reload
	adcs	r9, r9, r3
	ldr	r3, [sp, #24]                   @ 4-byte Reload
	adcs	r10, r4, r3
	ldr	r3, [sp, #20]                   @ 4-byte Reload
	adcs	r11, r0, r3
	ldr	r3, [r8, #44]
	ldr	r0, [sp, #56]                   @ 4-byte Reload
	mov	r8, r2
	adc	r1, r1, r3
	subs	r3, r2, r0
	ldr	r0, [sp, #68]                   @ 4-byte Reload
	sbcs	r4, lr, r0
	ldr	r0, [sp, #44]                   @ 4-byte Reload
	sbcs	r2, r9, r6
	sbcs	r7, r10, r5
	sbcs	r6, r11, r12
	sbc	r5, r1, r0
	asr	r0, r5, #31
	cmn	r0, #1
	movgt	r1, r5
	ldr	r5, [sp, #40]                   @ 4-byte Reload
	movle	r6, r11
	movle	r7, r10
	cmn	r0, #1
	movle	r2, r9
	movle	r4, lr
	movle	r3, r8
	str	r1, [r5, #20]
	str	r6, [r5, #16]
	str	r7, [r5, #12]
	str	r2, [r5, #8]
	str	r4, [r5, #4]
	str	r3, [r5]
	add	sp, sp, #72
	pop	{r4, r5, r6, r7, r8, r9, r10, r11, lr}
	mov	pc, lr
.Lfunc_end12:
	.size	mcl_fp_montRedNF6L, .Lfunc_end12-mcl_fp_montRedNF6L
	.cantunwind
	.fnend
                                        @ -- End function
	.globl	mcl_fp_addPre6L                 @ -- Begin function mcl_fp_addPre6L
	.p2align	2
	.type	mcl_fp_addPre6L,%function
	.code	32                              @ @mcl_fp_addPre6L
mcl_fp_addPre6L:
	.fnstart
@ %bb.0:
	.save	{r4, r5, r6, r7, r8, lr}
	push	{r4, r5, r6, r7, r8, lr}
	ldm	r2, {r3, r12, lr}
	ldm	r1, {r5, r6, r7}
	adds	r3, r5, r3
	str	r3, [r0]
	adcs	r3, r6, r12
	ldr	r8, [r2, #12]
	ldr	r4, [r1, #12]
	str	r3, [r0, #4]
	adcs	r3, r7, lr
	str	r3, [r0, #8]
	adcs	r3, r4, r8
	str	r3, [r0, #12]
	ldr	r3, [r2, #16]
	ldr	r7, [r1, #16]
	ldr	r2, [r2, #20]
	ldr	r1, [r1, #20]
	adcs	r3, r7, r3
	str	r3, [r0, #16]
	adcs	r1, r1, r2
	str	r1, [r0, #20]
	mov	r0, #0
	adc	r0, r0, #0
	pop	{r4, r5, r6, r7, r8, lr}
	mov	pc, lr
.Lfunc_end13:
	.size	mcl_fp_addPre6L, .Lfunc_end13-mcl_fp_addPre6L
	.cantunwind
	.fnend
                                        @ -- End function
	.globl	mcl_fp_subPre6L                 @ -- Begin function mcl_fp_subPre6L
	.p2align	2
	.type	mcl_fp_subPre6L,%function
	.code	32                              @ @mcl_fp_subPre6L
mcl_fp_subPre6L:
	.fnstart
@ %bb.0:
	.save	{r4, r5, r6, r7, r8, lr}
	push	{r4, r5, r6, r7, r8, lr}
	ldm	r2, {r3, r12, lr}
	ldm	r1, {r5, r6, r7}
	subs	r3, r5, r3
	str	r3, [r0]
	sbcs	r3, r6, r12
	ldr	r8, [r2, #12]
	ldr	r4, [r1, #12]
	str	r3, [r0, #4]
	sbcs	r3, r7, lr
	str	r3, [r0, #8]
	sbcs	r3, r4, r8
	str	r3, [r0, #12]
	ldr	r3, [r2, #16]
	ldr	r7, [r1, #16]
	ldr	r2, [r2, #20]
	ldr	r1, [r1, #20]
	sbcs	r3, r7, r3
	str	r3, [r0, #16]
	sbcs	r1, r1, r2
	str	r1, [r0, #20]
	mov	r0, #0
	sbc	r0, r0, #0
	and	r0, r0, #1
	pop	{r4, r5, r6, r7, r8, lr}
	mov	pc, lr
.Lfunc_end14:
	.size	mcl_fp_subPre6L, .Lfunc_end14-mcl_fp_subPre6L
	.cantunwind
	.fnend
                                        @ -- End function
	.globl	mcl_fp_shr1_6L                  @ -- Begin function mcl_fp_shr1_6L
	.p2align	2
	.type	mcl_fp_shr1_6L,%function
	.code	32                              @ @mcl_fp_shr1_6L
mcl_fp_shr1_6L:
	.fnstart
@ %bb.0:
	.save	{r4, r5, r6, r7, r11, lr}
	push	{r4, r5, r6, r7, r11, lr}
	add	r4, r1, #8
	ldm	r1, {r12, lr}
	lsr	r6, lr, #1
	ldr	r1, [r1, #20]
	ldm	r4, {r2, r3, r4}
	lsr	r5, r3, #1
	orr	r5, r5, r4, lsl #31
	lsr	r7, r1, #1
	lsrs	r1, r1, #1
	rrx	r1, r4
	lsrs	r3, r3, #1
	orr	r6, r6, r2, lsl #31
	rrx	r2, r2
	lsrs	r3, lr, #1
	rrx	r3, r12
	stm	r0, {r3, r6}
	str	r2, [r0, #8]
	str	r5, [r0, #12]
	str	r1, [r0, #16]
	str	r7, [r0, #20]
	pop	{r4, r5, r6, r7, r11, lr}
	mov	pc, lr
.Lfunc_end15:
	.size	mcl_fp_shr1_6L, .Lfunc_end15-mcl_fp_shr1_6L
	.cantunwind
	.fnend
                                        @ -- End function
	.globl	mcl_fp_add6L                    @ -- Begin function mcl_fp_add6L
	.p2align	2
	.type	mcl_fp_add6L,%function
	.code	32                              @ @mcl_fp_add6L
mcl_fp_add6L:
	.fnstart
@ %bb.0:
	.save	{r4, r5, r6, r7, r8, r9, r10, r11, lr}
	push	{r4, r5, r6, r7, r8, r9, r10, r11, lr}
	ldm	r2, {r8, r12, lr}
	ldm	r1, {r6, r7}
	adds	r10, r6, r8
	ldr	r4, [r1, #8]
	adcs	r11, r7, r12
	ldr	r9, [r2, #12]
	ldr	r5, [r1, #12]
	adcs	r12, r4, lr
	ldr	r4, [r2, #16]
	adcs	lr, r5, r9
	ldr	r5, [r1, #16]
	ldr	r2, [r2, #20]
	ldr	r1, [r1, #20]
	adcs	r4, r5, r4
	stm	r0, {r10, r11, r12, lr}
	adcs	r5, r1, r2
	mov	r1, #0
	ldr	r2, [r3]
	adc	r8, r1, #0
	ldmib	r3, {r1, r6, r7, r9}
	subs	r10, r10, r2
	sbcs	r2, r11, r1
	ldr	r3, [r3, #20]
	sbcs	r1, r12, r6
	str	r4, [r0, #16]
	sbcs	r12, lr, r7
	str	r5, [r0, #20]
	sbcs	lr, r4, r9
	sbcs	r4, r5, r3
	sbc	r3, r8, #0
	tst	r3, #1
	bne	.LBB16_2
@ %bb.1:                                @ %nocarry
	str	r2, [r0, #4]
	add	r2, r0, #8
	str	r10, [r0]
	stm	r2, {r1, r12, lr}
	str	r4, [r0, #20]
.LBB16_2:                               @ %carry
	pop	{r4, r5, r6, r7, r8, r9, r10, r11, lr}
	mov	pc, lr
.Lfunc_end16:
	.size	mcl_fp_add6L, .Lfunc_end16-mcl_fp_add6L
	.cantunwind
	.fnend
                                        @ -- End function
	.globl	mcl_fp_addNF6L                  @ -- Begin function mcl_fp_addNF6L
	.p2align	2
	.type	mcl_fp_addNF6L,%function
	.code	32                              @ @mcl_fp_addNF6L
mcl_fp_addNF6L:
	.fnstart
@ %bb.0:
	.save	{r4, r5, r6, r7, r8, r9, r10, r11, lr}
	push	{r4, r5, r6, r7, r8, r9, r10, r11, lr}
	add	r11, r1, #8
	ldm	r1, {r12, lr}
	ldm	r2, {r1, r4, r5, r6, r7}
	adds	r12, r1, r12
	ldm	r11, {r8, r9, r10, r11}
	adcs	lr, r4, lr
	adcs	r8, r5, r8
	ldr	r2, [r2, #20]
	adcs	r9, r6, r9
	adcs	r7, r7, r10
	adc	r2, r2, r11
	ldm	r3, {r1, r4, r5, r6, r10, r11}
	subs	r1, r12, r1
	sbcs	r4, lr, r4
	sbcs	r5, r8, r5
	sbcs	r6, r9, r6
	sbcs	r3, r7, r10
	sbc	r10, r2, r11
	asr	r11, r10, #31
	cmn	r11, #1
	movgt	r2, r10
	movle	r3, r7
	movle	r6, r9
	cmn	r11, #1
	movle	r5, r8
	movle	r4, lr
	movle	r1, r12
	str	r2, [r0, #20]
	str	r3, [r0, #16]
	str	r6, [r0, #12]
	str	r5, [r0, #8]
	str	r4, [r0, #4]
	str	r1, [r0]
	pop	{r4, r5, r6, r7, r8, r9, r10, r11, lr}
	mov	pc, lr
.Lfunc_end17:
	.size	mcl_fp_addNF6L, .Lfunc_end17-mcl_fp_addNF6L
	.cantunwind
	.fnend
                                        @ -- End function
	.globl	mcl_fp_sub6L                    @ -- Begin function mcl_fp_sub6L
	.p2align	2
	.type	mcl_fp_sub6L,%function
	.code	32                              @ @mcl_fp_sub6L
mcl_fp_sub6L:
	.fnstart
@ %bb.0:
	.save	{r4, r5, r6, r7, r8, r9, r11, lr}
	push	{r4, r5, r6, r7, r8, r9, r11, lr}
	ldr	r9, [r2]
	ldm	r1, {r4, r7}
	ldmib	r2, {r8, lr}
	subs	r4, r4, r9
	ldr	r5, [r1, #8]
	sbcs	r7, r7, r8
	ldr	r12, [r2, #12]
	ldr	r6, [r1, #12]
	sbcs	r8, r5, lr
	ldr	r5, [r1, #16]
	sbcs	r12, r6, r12
	ldr	r6, [r2, #16]
	ldr	r2, [r2, #20]
	ldr	r1, [r1, #20]
	sbcs	lr, r5, r6
	stm	r0, {r4, r7, r8, r12, lr}
	sbcs	r1, r1, r2
	mov	r2, #0
	str	r1, [r0, #20]
	sbc	r2, r2, #0
	tst	r2, #1
	bne	.LBB18_2
@ %bb.1:                                @ %nocarry
	pop	{r4, r5, r6, r7, r8, r9, r11, lr}
	mov	pc, lr
.LBB18_2:                               @ %carry
	ldm	r3, {r2, r5, r6, r9}
	adds	r2, r2, r4
	str	r2, [r0]
	adcs	r2, r5, r7
	str	r2, [r0, #4]
	adcs	r2, r6, r8
	str	r2, [r0, #8]
	adcs	r2, r9, r12
	str	r2, [r0, #12]
	ldr	r2, [r3, #16]
	adcs	r2, r2, lr
	str	r2, [r0, #16]
	ldr	r2, [r3, #20]
	adc	r1, r2, r1
	str	r1, [r0, #20]
	pop	{r4, r5, r6, r7, r8, r9, r11, lr}
	mov	pc, lr
.Lfunc_end18:
	.size	mcl_fp_sub6L, .Lfunc_end18-mcl_fp_sub6L
	.cantunwind
	.fnend
                                        @ -- End function
	.globl	mcl_fp_subNF6L                  @ -- Begin function mcl_fp_subNF6L
	.p2align	2
	.type	mcl_fp_subNF6L,%function
	.code	32                              @ @mcl_fp_subNF6L
mcl_fp_subNF6L:
	.fnstart
@ %bb.0:
	.save	{r4, r5, r6, r7, r8, r9, r10, r11, lr}
	push	{r4, r5, r6, r7, r8, r9, r10, r11, lr}
	add	r11, r2, #8
	ldm	r2, {r12, lr}
	ldm	r1, {r2, r4, r5, r6, r7}
	subs	r12, r2, r12
	ldm	r11, {r8, r9, r10, r11}
	sbcs	lr, r4, lr
	sbcs	r8, r5, r8
	ldr	r1, [r1, #20]
	sbcs	r9, r6, r9
	ldr	r4, [r3]
	sbcs	r7, r7, r10
	sbc	r1, r1, r11
	ldmib	r3, {r2, r5, r6, r10, r11}
	adds	r4, r12, r4
	adcs	r2, lr, r2
	adcs	r5, r8, r5
	adcs	r6, r9, r6
	adcs	r10, r7, r10
	adc	r3, r1, r11
	asr	r11, r1, #31
	cmp	r11, #0
	movpl	r3, r1
	movpl	r10, r7
	movpl	r6, r9
	cmp	r11, #0
	movpl	r5, r8
	movpl	r2, lr
	movpl	r4, r12
	str	r3, [r0, #20]
	str	r10, [r0, #16]
	str	r6, [r0, #12]
	str	r5, [r0, #8]
	str	r2, [r0, #4]
	str	r4, [r0]
	pop	{r4, r5, r6, r7, r8, r9, r10, r11, lr}
	mov	pc, lr
.Lfunc_end19:
	.size	mcl_fp_subNF6L, .Lfunc_end19-mcl_fp_subNF6L
	.cantunwind
	.fnend
                                        @ -- End function
	.globl	mcl_fpDbl_add6L                 @ -- Begin function mcl_fpDbl_add6L
	.p2align	2
	.type	mcl_fpDbl_add6L,%function
	.code	32                              @ @mcl_fpDbl_add6L
mcl_fpDbl_add6L:
	.fnstart
@ %bb.0:
	.save	{r4, r5, r6, r7, r8, r9, r10, r11, lr}
	push	{r4, r5, r6, r7, r8, r9, r10, r11, lr}
	.pad	#28
	sub	sp, sp, #28
	ldm	r2, {r8, r9, lr}
	ldm	r1, {r4, r5, r6, r7}
	adds	r4, r4, r8
	str	r4, [sp, #24]                   @ 4-byte Spill
	adcs	r4, r5, r9
	ldr	r12, [r2, #12]
	adcs	r6, r6, lr
	str	r4, [sp, #20]                   @ 4-byte Spill
	str	r6, [sp, #16]                   @ 4-byte Spill
	adcs	r7, r7, r12
	ldr	r4, [r2, #16]
	add	lr, r1, #32
	ldr	r6, [r1, #16]
	str	r7, [sp, #12]                   @ 4-byte Spill
	adcs	r7, r6, r4
	ldr	r5, [r2, #20]
	ldr	r6, [r1, #20]
	str	r7, [sp, #8]                    @ 4-byte Spill
	adcs	r7, r6, r5
	str	r7, [sp, #4]                    @ 4-byte Spill
	ldr	r6, [r2, #24]
	ldr	r7, [r1, #24]
	ldr	r4, [r2, #40]
	adcs	r11, r7, r6
	ldr	r6, [r1, #28]
	ldr	r7, [r2, #28]
	ldr	r5, [r2, #44]
	adcs	r10, r6, r7
	ldr	r6, [r2, #32]
	ldr	r7, [r2, #36]
	ldm	lr, {r2, r12, lr}
	adcs	r8, r2, r6
	ldr	r1, [r1, #44]
	adcs	r7, r12, r7
	mov	r2, #0
	adcs	lr, lr, r4
	ldr	r4, [r3, #8]
	adcs	r12, r1, r5
	ldr	r6, [sp, #20]                   @ 4-byte Reload
	adc	r9, r2, #0
	ldm	r3, {r2, r5}
	subs	r2, r11, r2
	str	r11, [sp]                       @ 4-byte Spill
	sbcs	r5, r10, r5
	ldr	r11, [sp, #24]                  @ 4-byte Reload
	ldr	r1, [r3, #12]
	sbcs	r4, r8, r4
	str	r11, [r0]
	str	r6, [r0, #4]
	sbcs	r1, r7, r1
	ldr	r6, [sp, #16]                   @ 4-byte Reload
	ldr	r11, [r3, #20]
	ldr	r3, [r3, #16]
	str	r6, [r0, #8]
	ldr	r6, [sp, #12]                   @ 4-byte Reload
	sbcs	r3, lr, r3
	str	r6, [r0, #12]
	sbcs	r11, r12, r11
	ldr	r6, [sp, #8]                    @ 4-byte Reload
	str	r6, [r0, #16]
	ldr	r6, [sp, #4]                    @ 4-byte Reload
	str	r6, [r0, #20]
	sbc	r6, r9, #0
	ands	r6, r6, #1
	movne	r11, r12
	movne	r3, lr
	movne	r1, r7
	cmp	r6, #0
	movne	r4, r8
	add	r12, r0, #36
	str	r4, [r0, #32]
	movne	r5, r10
	stm	r12, {r1, r3, r11}
	ldr	r1, [sp]                        @ 4-byte Reload
	str	r5, [r0, #28]
	movne	r2, r1
	str	r2, [r0, #24]
	add	sp, sp, #28
	pop	{r4, r5, r6, r7, r8, r9, r10, r11, lr}
	mov	pc, lr
.Lfunc_end20:
	.size	mcl_fpDbl_add6L, .Lfunc_end20-mcl_fpDbl_add6L
	.cantunwind
	.fnend
                                        @ -- End function
	.globl	mcl_fpDbl_sub6L                 @ -- Begin function mcl_fpDbl_sub6L
	.p2align	2
	.type	mcl_fpDbl_sub6L,%function
	.code	32                              @ @mcl_fpDbl_sub6L
mcl_fpDbl_sub6L:
	.fnstart
@ %bb.0:
	.save	{r4, r5, r6, r7, r8, r9, r10, r11, lr}
	push	{r4, r5, r6, r7, r8, r9, r10, r11, lr}
	.pad	#56
	sub	sp, sp, #56
	ldr	r7, [r2, #32]
	add	r8, r1, #12
	str	r7, [sp, #40]                   @ 4-byte Spill
	ldr	r7, [r2, #36]
	str	r7, [sp, #44]                   @ 4-byte Spill
	ldr	r7, [r2, #40]
	str	r7, [sp, #48]                   @ 4-byte Spill
	ldr	r7, [r2, #44]
	str	r7, [sp, #52]                   @ 4-byte Spill
	ldr	r7, [r1, #44]
	str	r7, [sp, #36]                   @ 4-byte Spill
	ldr	r7, [r1, #40]
	str	r7, [sp, #32]                   @ 4-byte Spill
	ldr	r7, [r2, #8]
	str	r7, [sp, #16]                   @ 4-byte Spill
	ldr	r7, [r2, #16]
	str	r7, [sp, #8]                    @ 4-byte Spill
	ldr	r7, [r2, #20]
	str	r7, [sp, #4]                    @ 4-byte Spill
	ldr	r7, [r2, #24]
	ldm	r2, {r9, r10}
	ldr	r11, [r2, #12]
	ldr	r2, [r2, #28]
	str	r2, [sp]                        @ 4-byte Spill
	ldm	r1, {r2, r12, lr}
	subs	r2, r2, r9
	str	r2, [sp, #28]                   @ 4-byte Spill
	sbcs	r2, r12, r10
	str	r2, [sp, #24]                   @ 4-byte Spill
	ldr	r2, [sp, #16]                   @ 4-byte Reload
	str	r7, [sp, #20]                   @ 4-byte Spill
	ldm	r8, {r4, r5, r6, r7, r8}
	sbcs	r2, lr, r2
	str	r2, [sp, #16]                   @ 4-byte Spill
	sbcs	r2, r4, r11
	str	r2, [sp, #12]                   @ 4-byte Spill
	ldr	r2, [sp, #8]                    @ 4-byte Reload
	ldr	r9, [r1, #36]
	sbcs	r2, r5, r2
	str	r2, [sp, #8]                    @ 4-byte Spill
	ldr	r2, [sp, #4]                    @ 4-byte Reload
	mov	r5, #0
	ldr	r1, [r1, #32]
	sbcs	r2, r6, r2
	str	r2, [sp, #4]                    @ 4-byte Spill
	ldr	r2, [sp, #20]                   @ 4-byte Reload
	ldr	r11, [sp, #28]                  @ 4-byte Reload
	sbcs	r6, r7, r2
	ldr	r2, [sp]                        @ 4-byte Reload
	ldr	r7, [sp, #40]                   @ 4-byte Reload
	sbcs	r10, r8, r2
	ldr	r2, [sp, #32]                   @ 4-byte Reload
	sbcs	r8, r1, r7
	ldr	r1, [sp, #44]                   @ 4-byte Reload
	str	r11, [r0]
	sbcs	r4, r9, r1
	ldr	r1, [sp, #48]                   @ 4-byte Reload
	ldr	r11, [r3, #20]
	sbcs	r7, r2, r1
	ldr	r1, [sp, #52]                   @ 4-byte Reload
	ldr	r2, [sp, #36]                   @ 4-byte Reload
	str	r6, [sp, #20]                   @ 4-byte Spill
	sbcs	lr, r2, r1
	ldmib	r3, {r1, r2, r12}
	sbc	r9, r5, #0
	ldr	r5, [r3]
	ldr	r3, [r3, #16]
	adds	r5, r6, r5
	ldr	r6, [sp, #24]                   @ 4-byte Reload
	adcs	r1, r10, r1
	str	r6, [r0, #4]
	adcs	r2, r8, r2
	ldr	r6, [sp, #16]                   @ 4-byte Reload
	adcs	r12, r4, r12
	str	r6, [r0, #8]
	adcs	r3, r7, r3
	ldr	r6, [sp, #12]                   @ 4-byte Reload
	adc	r11, lr, r11
	ands	r9, r9, #1
	moveq	r11, lr
	moveq	r3, r7
	moveq	r12, r4
	cmp	r9, #0
	moveq	r1, r10
	str	r6, [r0, #12]
	str	r1, [r0, #28]
	moveq	r2, r8
	ldr	r1, [sp, #20]                   @ 4-byte Reload
	ldr	r6, [sp, #8]                    @ 4-byte Reload
	str	r6, [r0, #16]
	ldr	r6, [sp, #4]                    @ 4-byte Reload
	moveq	r5, r1
	str	r6, [r0, #20]
	str	r2, [r0, #32]
	str	r12, [r0, #36]
	str	r3, [r0, #40]
	str	r11, [r0, #44]
	str	r5, [r0, #24]
	add	sp, sp, #56
	pop	{r4, r5, r6, r7, r8, r9, r10, r11, lr}
	mov	pc, lr
.Lfunc_end21:
	.size	mcl_fpDbl_sub6L, .Lfunc_end21-mcl_fpDbl_sub6L
	.cantunwind
	.fnend
                                        @ -- End function
	.globl	mulPv224x32                     @ -- Begin function mulPv224x32
	.p2align	2
	.type	mulPv224x32,%function
	.code	32                              @ @mulPv224x32
mulPv224x32:
	.fnstart
@ %bb.0:
	.save	{r4, r5, r6, r7, r8, r9, r11, lr}
	push	{r4, r5, r6, r7, r8, r9, r11, lr}
	ldr	r12, [r1]
	ldmib	r1, {r3, lr}
	umull	r7, r6, r12, r2
	ldr	r4, [r1, #12]
	umull	r5, r8, r3, r2
	str	r7, [r0]
	umull	r9, r12, r4, r2
	adds	r7, r6, r5
	umull	r5, r4, lr, r2
	adcs	r7, r8, r5
	umlal	r6, r5, r3, r2
	adcs	r7, r4, r9
	str	r7, [r0, #12]
	str	r5, [r0, #8]
	str	r6, [r0, #4]
	ldr	r3, [r1, #16]
	umull	r7, r6, r3, r2
	adcs	r3, r12, r7
	str	r3, [r0, #16]
	ldr	r3, [r1, #20]
	umull	r7, r5, r3, r2
	adcs	r3, r6, r7
	str	r3, [r0, #20]
	ldr	r1, [r1, #24]
	umull	r3, r7, r1, r2
	adcs	r1, r5, r3
	str	r1, [r0, #24]
	adc	r1, r7, #0
	str	r1, [r0, #28]
	pop	{r4, r5, r6, r7, r8, r9, r11, lr}
	mov	pc, lr
.Lfunc_end22:
	.size	mulPv224x32, .Lfunc_end22-mulPv224x32
	.cantunwind
	.fnend
                                        @ -- End function
	.globl	mcl_fp_mulUnitPre7L             @ -- Begin function mcl_fp_mulUnitPre7L
	.p2align	2
	.type	mcl_fp_mulUnitPre7L,%function
	.code	32                              @ @mcl_fp_mulUnitPre7L
mcl_fp_mulUnitPre7L:
	.fnstart
@ %bb.0:
	.save	{r4, r5, r6, r7, r8, r9, r11, lr}
	push	{r4, r5, r6, r7, r8, r9, r11, lr}
	ldr	r12, [r1]
	ldmib	r1, {r3, lr}
	umull	r7, r6, r12, r2
	ldr	r4, [r1, #12]
	umull	r5, r8, r3, r2
	str	r7, [r0]
	umull	r9, r12, r4, r2
	adds	r7, r6, r5
	umull	r5, r4, lr, r2
	adcs	r7, r8, r5
	umlal	r6, r5, r3, r2
	ldr	r3, [r1, #16]
	adcs	r7, r4, r9
	str	r7, [r0, #12]
	str	r6, [r0, #4]
	umull	r7, r6, r3, r2
	str	r5, [r0, #8]
	adcs	r3, r12, r7
	str	r3, [r0, #16]
	ldr	r3, [r1, #20]
	ldr	r1, [r1, #24]
	umull	r7, r5, r3, r2
	adcs	r3, r6, r7
	str	r3, [r0, #20]
	umull	r3, r7, r1, r2
	adcs	r1, r5, r3
	str	r1, [r0, #24]
	adc	r1, r7, #0
	str	r1, [r0, #28]
	pop	{r4, r5, r6, r7, r8, r9, r11, lr}
	mov	pc, lr
.Lfunc_end23:
	.size	mcl_fp_mulUnitPre7L, .Lfunc_end23-mcl_fp_mulUnitPre7L
	.cantunwind
	.fnend
                                        @ -- End function
	.globl	mcl_fpDbl_mulPre7L              @ -- Begin function mcl_fpDbl_mulPre7L
	.p2align	2
	.type	mcl_fpDbl_mulPre7L,%function
	.code	32                              @ @mcl_fpDbl_mulPre7L
mcl_fpDbl_mulPre7L:
	.fnstart
@ %bb.0:
	.save	{r4, r5, r6, r7, r8, r9, r10, r11, lr}
	push	{r4, r5, r6, r7, r8, r9, r10, r11, lr}
	.pad	#116
	sub	sp, sp, #116
	str	r0, [sp, #100]                  @ 4-byte Spill
	str	r2, [sp, #88]                   @ 4-byte Spill
	ldm	r2, {r0, r10}
	ldr	r4, [r2, #8]
	ldr	r9, [r2, #12]
	ldr	r2, [r1, #8]
	ldr	lr, [r1]
	str	r2, [sp, #108]                  @ 4-byte Spill
	ldr	r5, [r1, #12]
	ldr	r6, [r1, #16]
	ldr	r2, [r1, #20]
	ldr	r12, [r1, #4]
	ldr	r1, [r1, #24]
	str	r1, [sp, #104]                  @ 4-byte Spill
	str	r5, [sp, #96]                   @ 4-byte Spill
	umull	r3, r7, r1, r0
	str	r6, [sp, #112]                  @ 4-byte Spill
	str	r2, [sp, #92]                   @ 4-byte Spill
	str	lr, [sp, #84]                   @ 4-byte Spill
	str	r3, [sp, #72]                   @ 4-byte Spill
	umull	r3, r1, r2, r0
	str	r7, [sp, #76]                   @ 4-byte Spill
	umull	r8, r7, r5, r0
	str	r1, [sp, #68]                   @ 4-byte Spill
	str	r3, [sp, #64]                   @ 4-byte Spill
	umull	r11, r1, r6, r0
	ldr	r3, [sp, #108]                  @ 4-byte Reload
	umull	r2, r5, r12, r0
	str	r1, [sp, #60]                   @ 4-byte Spill
	umull	r6, r1, lr, r0
	str	r6, [sp, #80]                   @ 4-byte Spill
	mov	r6, lr
	adds	r2, r1, r2
	umull	r2, lr, r3, r0
	adcs	r5, r5, r2
	umlal	r1, r2, r12, r0
	adcs	r8, lr, r8
	adcs	r7, r7, r11
	str	r7, [sp, #48]                   @ 4-byte Spill
	ldr	r7, [sp, #64]                   @ 4-byte Reload
	ldr	r5, [sp, #60]                   @ 4-byte Reload
	adcs	r5, r5, r7
	str	r5, [sp, #56]                   @ 4-byte Spill
	ldr	r7, [sp, #72]                   @ 4-byte Reload
	ldr	r5, [sp, #68]                   @ 4-byte Reload
	adcs	r5, r5, r7
	ldr	r7, [sp, #76]                   @ 4-byte Reload
	str	r5, [sp, #64]                   @ 4-byte Spill
	adc	r5, r7, #0
	str	r5, [sp, #68]                   @ 4-byte Spill
	umull	r0, r5, r10, r3
	umull	r11, r3, r10, r6
	str	r5, [sp, #72]                   @ 4-byte Spill
	str	r3, [sp, #60]                   @ 4-byte Spill
	adds	r1, r1, r11
	str	r1, [sp, #76]                   @ 4-byte Spill
	umull	r1, r3, r10, r12
	str	r3, [sp, #52]                   @ 4-byte Spill
	ldr	r3, [sp, #96]                   @ 4-byte Reload
	adcs	r7, r2, r1
	adcs	r0, r8, r0
	str	r0, [sp, #40]                   @ 4-byte Spill
	umull	r2, r0, r10, r3
	str	r0, [sp, #44]                   @ 4-byte Spill
	ldr	r0, [sp, #48]                   @ 4-byte Reload
	adcs	r8, r0, r2
	ldr	r0, [sp, #112]                  @ 4-byte Reload
	ldr	r2, [sp, #60]                   @ 4-byte Reload
	umull	r5, r1, r10, r0
	ldr	r0, [sp, #56]                   @ 4-byte Reload
	str	r1, [sp, #48]                   @ 4-byte Spill
	adcs	r5, r0, r5
	ldr	r0, [sp, #92]                   @ 4-byte Reload
	ldr	r1, [sp, #104]                  @ 4-byte Reload
	umull	r6, lr, r10, r0
	ldr	r0, [sp, #64]                   @ 4-byte Reload
	adcs	r6, r0, r6
	umull	r11, r0, r10, r1
	ldr	r1, [sp, #68]                   @ 4-byte Reload
	ldr	r10, [sp, #92]                  @ 4-byte Reload
	adcs	r11, r1, r11
	mov	r1, #0
	adc	r1, r1, #0
	adds	r2, r7, r2
	str	r2, [sp, #32]                   @ 4-byte Spill
	ldr	r2, [sp, #52]                   @ 4-byte Reload
	ldr	r7, [sp, #40]                   @ 4-byte Reload
	adcs	r2, r7, r2
	str	r2, [sp, #28]                   @ 4-byte Spill
	ldr	r2, [sp, #72]                   @ 4-byte Reload
	adcs	r2, r8, r2
	str	r2, [sp, #24]                   @ 4-byte Spill
	ldr	r2, [sp, #44]                   @ 4-byte Reload
	adcs	r2, r5, r2
	str	r2, [sp, #60]                   @ 4-byte Spill
	ldr	r2, [sp, #48]                   @ 4-byte Reload
	adcs	r2, r6, r2
	str	r2, [sp, #56]                   @ 4-byte Spill
	adcs	r2, r11, lr
	str	r2, [sp, #52]                   @ 4-byte Spill
	adc	r0, r1, r0
	ldr	r1, [sp, #84]                   @ 4-byte Reload
	str	r0, [sp, #48]                   @ 4-byte Spill
	umull	r0, r2, r9, r12
	ldr	r11, [sp, #112]                 @ 4-byte Reload
	mov	lr, r3
	umull	r6, r7, r9, r1
	str	r6, [sp, #64]                   @ 4-byte Spill
	adds	r0, r7, r0
	ldr	r0, [sp, #108]                  @ 4-byte Reload
	umull	r8, r6, r9, r0
	adcs	r2, r2, r8
	umlal	r7, r8, r9, r12
	umull	r2, r5, r9, r3
	ldr	r3, [sp, #104]                  @ 4-byte Reload
	adcs	r2, r6, r2
	str	r2, [sp, #72]                   @ 4-byte Spill
	umull	r2, r6, r9, r11
	adcs	r2, r5, r2
	str	r2, [sp, #68]                   @ 4-byte Spill
	umull	r2, r5, r9, r10
	adcs	r2, r6, r2
	str	r2, [sp, #44]                   @ 4-byte Spill
	umull	r2, r6, r9, r3
	adcs	r2, r5, r2
	str	r2, [sp, #40]                   @ 4-byte Spill
	adc	r2, r6, #0
	str	r2, [sp, #36]                   @ 4-byte Spill
	umull	r2, r5, r4, r12
	str	r5, [sp, #16]                   @ 4-byte Spill
	umull	r6, r5, r4, r1
	str	r6, [sp, #20]                   @ 4-byte Spill
	adds	r2, r5, r2
	umull	r6, r2, r4, r0
	ldr	r0, [sp, #16]                   @ 4-byte Reload
	adcs	r1, r0, r6
	umlal	r5, r6, r4, r12
	umull	r1, r0, r4, lr
	adcs	r1, r2, r1
	str	r1, [sp, #16]                   @ 4-byte Spill
	umull	r2, r1, r4, r11
	adcs	r0, r0, r2
	str	r0, [sp, #12]                   @ 4-byte Spill
	umull	r2, r0, r4, r10
	adcs	r11, r1, r2
	umull	r2, r1, r4, r3
	ldr	r3, [sp, #80]                   @ 4-byte Reload
	adcs	lr, r0, r2
	ldr	r0, [sp, #100]                  @ 4-byte Reload
	ldr	r2, [sp, #32]                   @ 4-byte Reload
	adc	r1, r1, #0
	str	r3, [r0]
	ldr	r3, [sp, #20]                   @ 4-byte Reload
	adds	r4, r3, r2
	ldr	r2, [sp, #28]                   @ 4-byte Reload
	ldr	r3, [sp, #76]                   @ 4-byte Reload
	adcs	r5, r5, r2
	ldr	r2, [sp, #24]                   @ 4-byte Reload
	stmib	r0, {r3, r4}
	adcs	r2, r6, r2
	str	r2, [sp, #80]                   @ 4-byte Spill
	ldr	r3, [sp, #60]                   @ 4-byte Reload
	ldr	r2, [sp, #16]                   @ 4-byte Reload
	adcs	r4, r2, r3
	ldr	r3, [sp, #56]                   @ 4-byte Reload
	ldr	r2, [sp, #12]                   @ 4-byte Reload
	adcs	r6, r2, r3
	ldr	r3, [sp, #52]                   @ 4-byte Reload
	ldr	r2, [sp, #48]                   @ 4-byte Reload
	adcs	r3, r11, r3
	ldr	r11, [sp, #104]                 @ 4-byte Reload
	adcs	r2, lr, r2
	adc	lr, r1, #0
	ldr	r1, [sp, #64]                   @ 4-byte Reload
	adds	r5, r1, r5
	str	r5, [r0, #12]
	ldr	r0, [sp, #80]                   @ 4-byte Reload
	adcs	r0, r7, r0
	str	r0, [sp, #56]                   @ 4-byte Spill
	adcs	r0, r8, r4
	str	r0, [sp, #80]                   @ 4-byte Spill
	ldr	r0, [sp, #72]                   @ 4-byte Reload
	ldr	r8, [sp, #84]                   @ 4-byte Reload
	adcs	r0, r0, r6
	str	r0, [sp, #76]                   @ 4-byte Spill
	ldr	r0, [sp, #68]                   @ 4-byte Reload
	adcs	r0, r0, r3
	str	r0, [sp, #72]                   @ 4-byte Spill
	ldr	r0, [sp, #44]                   @ 4-byte Reload
	adcs	r0, r0, r2
	str	r0, [sp, #68]                   @ 4-byte Spill
	ldr	r0, [sp, #40]                   @ 4-byte Reload
	adcs	r0, r0, lr
	str	r0, [sp, #64]                   @ 4-byte Spill
	ldr	r0, [sp, #36]                   @ 4-byte Reload
	adc	r0, r0, #0
	str	r0, [sp, #60]                   @ 4-byte Spill
	ldr	r0, [sp, #88]                   @ 4-byte Reload
	ldr	r0, [r0, #16]
	umull	r2, r1, r0, r11
	umull	r3, lr, r0, r12
	str	r2, [sp, #48]                   @ 4-byte Spill
	str	r1, [sp, #52]                   @ 4-byte Spill
	umull	r2, r1, r0, r10
	str	r1, [sp, #44]                   @ 4-byte Spill
	ldr	r1, [sp, #112]                  @ 4-byte Reload
	str	r2, [sp, #40]                   @ 4-byte Spill
	umull	r10, r2, r0, r1
	ldr	r1, [sp, #96]                   @ 4-byte Reload
	umull	r5, r6, r0, r1
	ldr	r1, [sp, #108]                  @ 4-byte Reload
	str	r2, [sp, #36]                   @ 4-byte Spill
	umull	r2, r4, r0, r1
	umull	r9, r1, r0, r8
	adds	r7, r1, r3
	ldr	r3, [sp, #40]                   @ 4-byte Reload
	adcs	r7, lr, r2
	umlal	r1, r2, r0, r12
	adcs	r7, r4, r5
	ldr	r5, [sp, #36]                   @ 4-byte Reload
	adcs	r6, r6, r10
	ldr	r4, [sp, #44]                   @ 4-byte Reload
	adcs	r5, r5, r3
	ldr	r3, [sp, #48]                   @ 4-byte Reload
	ldr	r0, [sp, #80]                   @ 4-byte Reload
	adcs	lr, r4, r3
	ldr	r3, [sp, #52]                   @ 4-byte Reload
	adc	r10, r3, #0
	ldr	r3, [sp, #56]                   @ 4-byte Reload
	adds	r4, r9, r3
	ldr	r3, [sp, #100]                  @ 4-byte Reload
	adcs	r0, r1, r0
	str	r0, [sp, #80]                   @ 4-byte Spill
	ldr	r0, [sp, #76]                   @ 4-byte Reload
	ldr	r1, [sp, #88]                   @ 4-byte Reload
	adcs	r0, r2, r0
	str	r0, [sp, #76]                   @ 4-byte Spill
	ldr	r0, [sp, #72]                   @ 4-byte Reload
	str	r4, [r3, #16]
	adcs	r0, r7, r0
	str	r0, [sp, #72]                   @ 4-byte Spill
	ldr	r0, [sp, #68]                   @ 4-byte Reload
	ldr	r7, [sp, #108]                  @ 4-byte Reload
	adcs	r0, r6, r0
	str	r0, [sp, #68]                   @ 4-byte Spill
	ldr	r0, [sp, #64]                   @ 4-byte Reload
	ldr	r6, [r1, #24]
	adcs	r0, r5, r0
	str	r0, [sp, #64]                   @ 4-byte Spill
	ldr	r0, [sp, #60]                   @ 4-byte Reload
	umull	r3, r2, r6, r8
	adcs	r0, lr, r0
	str	r0, [sp, #60]                   @ 4-byte Spill
	adc	r0, r10, #0
	str	r0, [sp, #56]                   @ 4-byte Spill
	ldr	r0, [r1, #20]
	umull	r9, r1, r6, r7
	str	r3, [sp, #88]                   @ 4-byte Spill
	mov	r3, r2
	str	r1, [sp, #36]                   @ 4-byte Spill
	ldr	r1, [sp, #96]                   @ 4-byte Reload
	mov	r5, r9
	umlal	r3, r5, r6, r12
	str	r3, [sp, #48]                   @ 4-byte Spill
	umull	r4, r3, r0, r7
	str	r5, [sp, #52]                   @ 4-byte Spill
	str	r3, [sp, #40]                   @ 4-byte Spill
	umull	r3, r10, r0, r8
	str	r4, [sp]                        @ 4-byte Spill
	mov	r7, r4
	ldr	r8, [sp, #92]                   @ 4-byte Reload
	str	r3, [sp, #44]                   @ 4-byte Spill
	umull	r4, r3, r0, r1
	mov	lr, r10
	umlal	lr, r7, r0, r12
	str	r4, [sp, #28]                   @ 4-byte Spill
	str	r3, [sp, #32]                   @ 4-byte Spill
	umull	r4, r3, r0, r12
	str	r4, [sp, #20]                   @ 4-byte Spill
	str	r3, [sp, #24]                   @ 4-byte Spill
	umull	r4, r3, r6, r11
	str	r3, [sp, #16]                   @ 4-byte Spill
	umull	r5, r3, r6, r8
	str	r4, [sp, #84]                   @ 4-byte Spill
	str	r3, [sp, #12]                   @ 4-byte Spill
	ldr	r3, [sp, #112]                  @ 4-byte Reload
	str	r5, [sp, #8]                    @ 4-byte Spill
	umull	r4, r5, r6, r3
	str	r4, [sp, #108]                  @ 4-byte Spill
	str	r5, [sp, #4]                    @ 4-byte Spill
	umull	r5, r4, r6, r1
	umull	r11, r1, r6, r12
	adds	r2, r2, r11
	adcs	r1, r1, r9
	ldr	r2, [sp, #4]                    @ 4-byte Reload
	ldr	r1, [sp, #36]                   @ 4-byte Reload
	adcs	r11, r1, r5
	ldr	r1, [sp, #108]                  @ 4-byte Reload
	adcs	r1, r4, r1
	str	r1, [sp, #108]                  @ 4-byte Spill
	ldr	r1, [sp, #8]                    @ 4-byte Reload
	ldr	r4, [sp, #104]                  @ 4-byte Reload
	adcs	r1, r2, r1
	str	r1, [sp, #96]                   @ 4-byte Spill
	ldr	r1, [sp, #84]                   @ 4-byte Reload
	ldr	r2, [sp, #12]                   @ 4-byte Reload
	adcs	r1, r2, r1
	str	r1, [sp, #84]                   @ 4-byte Spill
	ldr	r1, [sp, #16]                   @ 4-byte Reload
	ldr	r2, [sp]                        @ 4-byte Reload
	adc	r5, r1, #0
	ldr	r1, [sp, #20]                   @ 4-byte Reload
	adds	r6, r10, r1
	ldr	r1, [sp, #24]                   @ 4-byte Reload
	adcs	r6, r1, r2
	ldr	r1, [sp, #40]                   @ 4-byte Reload
	ldr	r2, [sp, #28]                   @ 4-byte Reload
	adcs	r12, r1, r2
	umull	r1, r2, r0, r3
	ldr	r3, [sp, #32]                   @ 4-byte Reload
	adcs	r9, r3, r1
	umull	r3, r6, r0, r8
	adcs	r2, r2, r3
	umull	r3, r1, r0, r4
	ldr	r4, [sp, #44]                   @ 4-byte Reload
	adcs	r0, r6, r3
	ldr	r3, [sp, #80]                   @ 4-byte Reload
	adc	r1, r1, #0
	adds	r3, r4, r3
	ldr	r4, [sp, #76]                   @ 4-byte Reload
	adcs	r6, lr, r4
	ldr	r4, [sp, #72]                   @ 4-byte Reload
	adcs	r7, r7, r4
	ldr	r4, [sp, #68]                   @ 4-byte Reload
	adcs	r12, r12, r4
	ldr	r4, [sp, #64]                   @ 4-byte Reload
	adcs	lr, r9, r4
	ldr	r4, [sp, #60]                   @ 4-byte Reload
	adcs	r2, r2, r4
	ldr	r4, [sp, #56]                   @ 4-byte Reload
	adcs	r9, r0, r4
	ldr	r0, [sp, #88]                   @ 4-byte Reload
	adc	r1, r1, #0
	ldr	r4, [sp, #100]                  @ 4-byte Reload
	adds	r6, r0, r6
	ldr	r0, [sp, #48]                   @ 4-byte Reload
	adcs	r7, r0, r7
	ldr	r0, [sp, #52]                   @ 4-byte Reload
	adcs	r0, r0, r12
	add	r12, r4, #20
	stm	r12, {r3, r6, r7}
	adcs	r3, r11, lr
	ldr	r7, [sp, #108]                  @ 4-byte Reload
	ldr	r6, [sp, #84]                   @ 4-byte Reload
	adcs	r2, r7, r2
	ldr	r7, [sp, #96]                   @ 4-byte Reload
	str	r0, [r4, #32]
	adcs	r7, r7, r9
	str	r3, [r4, #36]
	adcs	r1, r6, r1
	str	r2, [r4, #40]
	adc	r6, r5, #0
	str	r7, [r4, #44]
	str	r1, [r4, #48]
	str	r6, [r4, #52]
	add	sp, sp, #116
	pop	{r4, r5, r6, r7, r8, r9, r10, r11, lr}
	mov	pc, lr
.Lfunc_end24:
	.size	mcl_fpDbl_mulPre7L, .Lfunc_end24-mcl_fpDbl_mulPre7L
	.cantunwind
	.fnend
                                        @ -- End function
	.globl	mcl_fpDbl_sqrPre7L              @ -- Begin function mcl_fpDbl_sqrPre7L
	.p2align	2
	.type	mcl_fpDbl_sqrPre7L,%function
	.code	32                              @ @mcl_fpDbl_sqrPre7L
mcl_fpDbl_sqrPre7L:
	.fnstart
@ %bb.0:
	.save	{r4, r5, r6, r7, r8, r9, r10, r11, lr}
	push	{r4, r5, r6, r7, r8, r9, r10, r11, lr}
	.pad	#212
	sub	sp, sp, #212
	ldr	r11, [r1]
	ldmib	r1, {r2, r3}
	ldr	r12, [r1, #24]
	str	r0, [sp, #120]                  @ 4-byte Spill
	ldr	r10, [r1, #12]
	umull	r5, r0, r12, r11
	ldr	r4, [r1, #16]
	ldr	lr, [r1, #20]
	str	r11, [sp, #164]                 @ 4-byte Spill
	umull	r6, r1, r12, r2
	str	r3, [sp, #8]                    @ 4-byte Spill
	str	r5, [sp, #188]                  @ 4-byte Spill
	str	r0, [sp, #176]                  @ 4-byte Spill
	str	r1, [sp, #96]                   @ 4-byte Spill
	adds	r5, r0, r6
	str	r6, [sp, #72]                   @ 4-byte Spill
	umull	r5, r0, r12, r3
	str	r5, [sp, #208]                  @ 4-byte Spill
	adcs	r5, r1, r5
	str	r0, [sp, #68]                   @ 4-byte Spill
	umull	r5, r1, r12, r10
	str	r5, [sp, #192]                  @ 4-byte Spill
	adcs	r5, r0, r5
	str	r5, [sp, #160]                  @ 4-byte Spill
	umull	r5, r0, r12, r4
	str	r1, [sp, #168]                  @ 4-byte Spill
	str	r5, [sp, #204]                  @ 4-byte Spill
	adcs	r5, r1, r5
	umull	r6, r1, r12, lr
	str	r5, [sp, #156]                  @ 4-byte Spill
	str	r0, [sp, #100]                  @ 4-byte Spill
	str	r1, [sp, #196]                  @ 4-byte Spill
	adcs	r5, r0, r6
	str	r5, [sp, #152]                  @ 4-byte Spill
	umull	r8, r5, r12, r12
	adcs	r7, r1, r8
	umull	r0, r1, lr, r11
	adc	r5, r5, #0
	str	r7, [sp, #148]                  @ 4-byte Spill
	str	r5, [sp, #144]                  @ 4-byte Spill
	umull	r5, r7, lr, r2
	str	r0, [sp, #184]                  @ 4-byte Spill
	str	r1, [sp, #172]                  @ 4-byte Spill
	umull	r0, r8, lr, r3
	str	r5, [sp, #40]                   @ 4-byte Spill
	adds	r5, r1, r5
	str	r7, [sp, #36]                   @ 4-byte Spill
	str	r0, [sp, #200]                  @ 4-byte Spill
	adcs	r5, r7, r0
	umull	r1, r0, lr, r10
	str	r8, [sp, #52]                   @ 4-byte Spill
	umull	r9, r7, lr, lr
	str	r1, [sp, #84]                   @ 4-byte Spill
	adcs	r5, r8, r1
	umull	r8, r1, lr, r4
	str	r5, [sp, #140]                  @ 4-byte Spill
	str	r0, [sp, #80]                   @ 4-byte Spill
	str	r1, [sp, #104]                  @ 4-byte Spill
	adcs	r5, r0, r8
	str	r5, [sp, #136]                  @ 4-byte Spill
	adcs	r5, r1, r9
	ldr	r0, [sp, #196]                  @ 4-byte Reload
	str	r5, [sp, #132]                  @ 4-byte Spill
	adcs	r5, r7, r6
	str	r5, [sp, #128]                  @ 4-byte Spill
	adc	r5, r0, #0
	umull	r6, r9, r4, r2
	str	r5, [sp, #124]                  @ 4-byte Spill
	umull	r5, r1, r4, r11
	str	r6, [sp, #24]                   @ 4-byte Spill
	str	r9, [sp, #20]                   @ 4-byte Spill
	str	r1, [sp, #32]                   @ 4-byte Spill
	adds	r7, r1, r6
	umull	r1, r0, r4, r3
	str	r5, [sp, #180]                  @ 4-byte Spill
	str	r1, [sp, #196]                  @ 4-byte Spill
	adcs	r7, r9, r1
	umull	r5, r9, r4, r4
	umull	r7, r6, r4, r10
	adcs	r1, r0, r7
	str	r1, [sp, #116]                  @ 4-byte Spill
	adcs	r1, r6, r5
	str	r1, [sp, #112]                  @ 4-byte Spill
	adcs	r1, r9, r8
	str	r1, [sp, #108]                  @ 4-byte Spill
	ldr	r1, [sp, #204]                  @ 4-byte Reload
	umull	r8, r9, r10, r2
	ldr	r5, [sp, #104]                  @ 4-byte Reload
	adcs	r1, r5, r1
	str	r1, [sp, #104]                  @ 4-byte Spill
	ldr	r1, [sp, #100]                  @ 4-byte Reload
	str	r8, [sp, #28]                   @ 4-byte Spill
	adc	r1, r1, #0
	str	r1, [sp, #100]                  @ 4-byte Spill
	umull	r5, r1, r10, r11
	str	r9, [sp]                        @ 4-byte Spill
	str	r5, [sp, #44]                   @ 4-byte Spill
	str	r1, [sp, #16]                   @ 4-byte Spill
	adds	r5, r1, r8
	umull	r1, r8, r10, r3
	str	r1, [sp, #204]                  @ 4-byte Spill
	adcs	r5, r9, r1
	umull	r5, r9, r10, r10
	adcs	r5, r8, r5
	str	r5, [sp, #92]                   @ 4-byte Spill
	adcs	r5, r9, r7
	str	r5, [sp, #88]                   @ 4-byte Spill
	ldr	r5, [sp, #84]                   @ 4-byte Reload
	umull	r7, r9, r3, r11
	adcs	r5, r6, r5
	str	r5, [sp, #84]                   @ 4-byte Spill
	ldr	r5, [sp, #192]                  @ 4-byte Reload
	ldr	r6, [sp, #80]                   @ 4-byte Reload
	str	r9, [sp, #12]                   @ 4-byte Spill
	adcs	r5, r6, r5
	str	r5, [sp, #80]                   @ 4-byte Spill
	ldr	r5, [sp, #168]                  @ 4-byte Reload
	str	r7, [sp, #168]                  @ 4-byte Spill
	adc	r5, r5, #0
	str	r5, [sp, #76]                   @ 4-byte Spill
	umull	r5, r6, r3, r2
	adds	r7, r9, r5
	umull	r7, r9, r3, r3
	str	r7, [sp, #192]                  @ 4-byte Spill
	adcs	r11, r6, r7
	ldr	r7, [sp, #200]                  @ 4-byte Reload
	adcs	r1, r9, r1
	str	r1, [sp, #64]                   @ 4-byte Spill
	ldr	r1, [sp, #196]                  @ 4-byte Reload
	adcs	r3, r8, r1
	str	r3, [sp, #60]                   @ 4-byte Spill
	adcs	r3, r0, r7
	str	r3, [sp, #56]                   @ 4-byte Spill
	mov	r3, r7
	ldr	r0, [sp, #208]                  @ 4-byte Reload
	ldr	r7, [sp, #52]                   @ 4-byte Reload
	adcs	r7, r7, r0
	str	r7, [sp, #52]                   @ 4-byte Spill
	ldr	r7, [sp, #68]                   @ 4-byte Reload
	adc	r7, r7, #0
	str	r7, [sp, #48]                   @ 4-byte Spill
	umull	r9, r7, r2, r2
	str	r7, [sp, #68]                   @ 4-byte Spill
	ldr	r7, [sp, #164]                  @ 4-byte Reload
	umull	r8, r11, r2, r7
	str	r8, [sp, #4]                    @ 4-byte Spill
	adds	r7, r11, r9
	ldr	r8, [sp, #32]                   @ 4-byte Reload
	ldr	r7, [sp, #68]                   @ 4-byte Reload
	adcs	r7, r7, r5
	ldr	r7, [sp, #28]                   @ 4-byte Reload
	adcs	r6, r6, r7
	str	r6, [sp, #28]                   @ 4-byte Spill
	ldr	r6, [sp, #24]                   @ 4-byte Reload
	ldr	r7, [sp]                        @ 4-byte Reload
	adcs	r6, r7, r6
	str	r6, [sp, #24]                   @ 4-byte Spill
	ldr	r6, [sp, #40]                   @ 4-byte Reload
	ldr	r7, [sp, #20]                   @ 4-byte Reload
	adcs	r6, r7, r6
	str	r6, [sp, #20]                   @ 4-byte Spill
	ldr	r6, [sp, #72]                   @ 4-byte Reload
	ldr	r7, [sp, #36]                   @ 4-byte Reload
	adcs	r6, r7, r6
	str	r6, [sp, #36]                   @ 4-byte Spill
	ldr	r6, [sp, #176]                  @ 4-byte Reload
	ldr	r7, [sp, #168]                  @ 4-byte Reload
	umlal	r6, r0, r12, r2
	str	r0, [sp, #208]                  @ 4-byte Spill
	ldr	r0, [sp, #172]                  @ 4-byte Reload
	str	r6, [sp, #72]                   @ 4-byte Spill
	umlal	r0, r3, lr, r2
	mov	lr, r11
	umlal	lr, r5, r2, r2
	str	r0, [sp, #68]                   @ 4-byte Spill
	mov	r0, r8
	umlal	r0, r1, r4, r2
	str	r3, [sp, #200]                  @ 4-byte Spill
	ldr	r3, [sp, #16]                   @ 4-byte Reload
	mov	r4, r7
	str	r0, [sp, #40]                   @ 4-byte Spill
	mov	r9, r3
	ldr	r0, [sp, #204]                  @ 4-byte Reload
	str	r1, [sp, #196]                  @ 4-byte Spill
	umlal	r9, r0, r10, r2
	ldr	r10, [sp, #12]                  @ 4-byte Reload
	ldr	r1, [sp, #8]                    @ 4-byte Reload
	mov	r12, r10
	str	r0, [sp, #204]                  @ 4-byte Spill
	ldr	r0, [sp, #192]                  @ 4-byte Reload
	umlal	r12, r0, r1, r2
	str	r0, [sp, #192]                  @ 4-byte Spill
	ldr	r0, [sp, #164]                  @ 4-byte Reload
	umull	r1, r6, r0, r0
	str	r1, [sp, #8]                    @ 4-byte Spill
	mov	r1, r6
	umlal	r1, r4, r2, r0
	ldr	r0, [sp, #96]                   @ 4-byte Reload
	ldr	r2, [sp, #4]                    @ 4-byte Reload
	adc	r0, r0, #0
	adds	r6, r6, r2
	str	r0, [sp, #164]                  @ 4-byte Spill
	adcs	r6, r11, r7
	ldr	r6, [sp, #44]                   @ 4-byte Reload
	adcs	r0, r10, r6
	str	r0, [sp, #96]                   @ 4-byte Spill
	ldr	r0, [sp, #180]                  @ 4-byte Reload
	adcs	r10, r3, r0
	ldr	r0, [sp, #184]                  @ 4-byte Reload
	ldr	r3, [sp, #172]                  @ 4-byte Reload
	adcs	r11, r8, r0
	ldr	r0, [sp, #188]                  @ 4-byte Reload
	adcs	r7, r3, r0
	ldr	r0, [sp, #176]                  @ 4-byte Reload
	adc	r3, r0, #0
	adds	r0, r2, r1
	adcs	lr, lr, r4
	ldr	r4, [sp, #120]                  @ 4-byte Reload
	str	r0, [sp, #176]                  @ 4-byte Spill
	ldr	r0, [sp, #8]                    @ 4-byte Reload
	str	r0, [r4]
	ldr	r0, [sp, #96]                   @ 4-byte Reload
	ldr	r1, [sp, #24]                   @ 4-byte Reload
	adcs	r5, r5, r0
	ldr	r0, [sp, #28]                   @ 4-byte Reload
	adcs	r0, r0, r10
	adcs	r2, r1, r11
	ldr	r1, [sp, #20]                   @ 4-byte Reload
	adcs	r10, r1, r7
	ldr	r1, [sp, #36]                   @ 4-byte Reload
	ldr	r7, [sp, #168]                  @ 4-byte Reload
	adcs	r3, r1, r3
	ldr	r1, [sp, #164]                  @ 4-byte Reload
	adc	r1, r1, #0
	adds	r7, r7, lr
	str	r7, [sp, #172]                  @ 4-byte Spill
	adcs	r7, r12, r5
	ldr	r5, [sp, #192]                  @ 4-byte Reload
	adcs	r11, r5, r0
	ldr	r5, [sp, #64]                   @ 4-byte Reload
	ldr	r0, [sp, #52]                   @ 4-byte Reload
	adcs	r2, r5, r2
	ldr	r5, [sp, #60]                   @ 4-byte Reload
	adcs	r10, r5, r10
	ldr	r5, [sp, #56]                   @ 4-byte Reload
	adcs	r5, r5, r3
	adcs	lr, r0, r1
	ldr	r0, [sp, #48]                   @ 4-byte Reload
	ldr	r1, [sp, #204]                  @ 4-byte Reload
	adc	r12, r0, #0
	adds	r0, r6, r7
	str	r0, [sp, #192]                  @ 4-byte Spill
	adcs	r0, r9, r11
	adcs	r3, r1, r2
	ldr	r1, [sp, #92]                   @ 4-byte Reload
	adcs	r2, r1, r10
	ldr	r1, [sp, #88]                   @ 4-byte Reload
	adcs	r5, r1, r5
	ldr	r1, [sp, #84]                   @ 4-byte Reload
	adcs	r6, r1, lr
	ldr	r1, [sp, #80]                   @ 4-byte Reload
	adcs	r7, r1, r12
	ldr	r1, [sp, #76]                   @ 4-byte Reload
	add	r12, r4, #12
	adc	lr, r1, #0
	ldr	r1, [sp, #180]                  @ 4-byte Reload
	adds	r8, r1, r0
	ldr	r0, [sp, #40]                   @ 4-byte Reload
	ldr	r1, [sp, #100]                  @ 4-byte Reload
	adcs	r3, r0, r3
	ldr	r0, [sp, #196]                  @ 4-byte Reload
	adcs	r2, r0, r2
	ldr	r0, [sp, #116]                  @ 4-byte Reload
	adcs	r5, r0, r5
	ldr	r0, [sp, #112]                  @ 4-byte Reload
	adcs	r6, r0, r6
	ldr	r0, [sp, #108]                  @ 4-byte Reload
	adcs	r7, r0, r7
	ldr	r0, [sp, #104]                  @ 4-byte Reload
	adcs	r0, r0, lr
	adc	lr, r1, #0
	ldr	r1, [sp, #184]                  @ 4-byte Reload
	adds	r10, r1, r3
	ldr	r1, [sp, #68]                   @ 4-byte Reload
	adcs	r2, r1, r2
	ldr	r1, [sp, #200]                  @ 4-byte Reload
	adcs	r5, r1, r5
	ldr	r1, [sp, #140]                  @ 4-byte Reload
	adcs	r6, r1, r6
	ldr	r1, [sp, #136]                  @ 4-byte Reload
	adcs	r7, r1, r7
	ldr	r1, [sp, #132]                  @ 4-byte Reload
	adcs	r0, r1, r0
	ldr	r1, [sp, #128]                  @ 4-byte Reload
	adcs	r3, r1, lr
	ldr	r1, [sp, #124]                  @ 4-byte Reload
	adc	lr, r1, #0
	ldr	r1, [sp, #188]                  @ 4-byte Reload
	adds	r2, r1, r2
	ldr	r1, [sp, #72]                   @ 4-byte Reload
	adcs	r5, r1, r5
	ldr	r1, [sp, #208]                  @ 4-byte Reload
	adcs	r6, r1, r6
	ldr	r1, [sp, #176]                  @ 4-byte Reload
	str	r1, [r4, #4]
	ldr	r1, [sp, #172]                  @ 4-byte Reload
	str	r1, [r4, #8]
	ldr	r1, [sp, #192]                  @ 4-byte Reload
	stm	r12, {r1, r8, r10}
	ldr	r1, [sp, #160]                  @ 4-byte Reload
	str	r2, [r4, #24]
	ldr	r2, [sp, #156]                  @ 4-byte Reload
	adcs	r1, r1, r7
	ldr	r7, [sp, #144]                  @ 4-byte Reload
	adcs	r0, r2, r0
	ldr	r2, [sp, #152]                  @ 4-byte Reload
	str	r1, [r4, #36]
	add	r1, r4, #40
	adcs	r2, r2, r3
	ldr	r3, [sp, #148]                  @ 4-byte Reload
	str	r5, [r4, #28]
	adcs	r3, r3, lr
	str	r6, [r4, #32]
	adc	r7, r7, #0
	stm	r1, {r0, r2, r3, r7}
	add	sp, sp, #212
	pop	{r4, r5, r6, r7, r8, r9, r10, r11, lr}
	mov	pc, lr
.Lfunc_end25:
	.size	mcl_fpDbl_sqrPre7L, .Lfunc_end25-mcl_fpDbl_sqrPre7L
	.cantunwind
	.fnend
                                        @ -- End function
	.globl	mcl_fp_mont7L                   @ -- Begin function mcl_fp_mont7L
	.p2align	2
	.type	mcl_fp_mont7L,%function
	.code	32                              @ @mcl_fp_mont7L
mcl_fp_mont7L:
	.fnstart
@ %bb.0:
	.save	{r4, r5, r6, r7, r8, r9, r10, r11, lr}
	push	{r4, r5, r6, r7, r8, r9, r10, r11, lr}
	.pad	#132
	sub	sp, sp, #132
	str	r0, [sp, #64]                   @ 4-byte Spill
	mov	r0, r2
	ldr	r7, [r0, #8]
	ldr	r12, [r0, #4]
	ldr	r0, [r0, #12]
	str	r2, [sp, #68]                   @ 4-byte Spill
	str	r0, [sp, #60]                   @ 4-byte Spill
	ldr	r0, [r1]
	ldr	r2, [r2]
	ldr	r4, [r3, #-4]
	str	r0, [sp, #108]                  @ 4-byte Spill
	umull	r6, r8, r0, r2
	ldr	r5, [r3, #8]
	str	r4, [sp, #116]                  @ 4-byte Spill
	str	r7, [sp, #56]                   @ 4-byte Spill
	ldr	r7, [r3]
	str	r6, [sp, #52]                   @ 4-byte Spill
	str	r5, [sp, #128]                  @ 4-byte Spill
	mul	r0, r4, r6
	ldr	r9, [r3, #4]
	ldr	lr, [r1, #4]
	ldr	r10, [r1, #8]
	ldr	r11, [r1, #12]
	str	r11, [sp, #88]                  @ 4-byte Spill
	str	r7, [sp, #112]                  @ 4-byte Spill
	umull	r6, r4, r0, r5
	str	lr, [sp, #72]                   @ 4-byte Spill
	str	r10, [sp, #80]                  @ 4-byte Spill
	str	r9, [sp, #76]                   @ 4-byte Spill
	str	r4, [sp, #36]                   @ 4-byte Spill
	umull	r4, r5, r0, r7
	str	r6, [sp, #12]                   @ 4-byte Spill
	str	r4, [sp, #48]                   @ 4-byte Spill
	str	r5, [sp, #8]                    @ 4-byte Spill
	mov	r4, r5
	mov	r5, r6
	umlal	r4, r5, r0, r9
	str	r4, [sp, #40]                   @ 4-byte Spill
	ldr	r4, [r1, #24]
	str	r5, [sp, #44]                   @ 4-byte Spill
	str	r4, [sp, #96]                   @ 4-byte Spill
	umull	r6, r5, r4, r2
	ldr	r4, [r1, #20]
	ldr	r1, [r1, #16]
	str	r4, [sp, #92]                   @ 4-byte Spill
	str	r1, [sp, #84]                   @ 4-byte Spill
	str	r6, [sp, #120]                  @ 4-byte Spill
	str	r5, [sp, #124]                  @ 4-byte Spill
	umull	r6, r5, r4, r2
	str	r5, [sp, #104]                  @ 4-byte Spill
	umull	r4, r5, r1, r2
	str	r6, [sp, #100]                  @ 4-byte Spill
	str	r5, [sp, #24]                   @ 4-byte Spill
	umull	r6, r5, r11, r2
	umull	r11, r1, lr, r2
	ldr	lr, [r3, #24]
	adds	r7, r8, r11
	umull	r7, r11, r10, r2
	adcs	r1, r1, r7
	adcs	r1, r11, r6
	str	r1, [sp, #32]                   @ 4-byte Spill
	adcs	r1, r5, r4
	str	r1, [sp, #28]                   @ 4-byte Spill
	ldr	r1, [sp, #100]                  @ 4-byte Reload
	ldr	r6, [sp, #24]                   @ 4-byte Reload
	ldr	r5, [r3, #12]
	adcs	r1, r6, r1
	str	r1, [sp, #24]                   @ 4-byte Spill
	ldr	r1, [sp, #120]                  @ 4-byte Reload
	ldr	r6, [sp, #104]                  @ 4-byte Reload
	ldr	r4, [sp, #8]                    @ 4-byte Reload
	adcs	r1, r6, r1
	str	r1, [sp, #20]                   @ 4-byte Spill
	ldr	r1, [sp, #124]                  @ 4-byte Reload
	ldr	r6, [r3, #20]
	adc	r1, r1, #0
	str	r1, [sp, #16]                   @ 4-byte Spill
	ldr	r1, [r3, #16]
	umull	r3, r10, r0, r9
	str	r1, [sp, #120]                  @ 4-byte Spill
	str	r5, [sp, #124]                  @ 4-byte Spill
	umull	r11, r9, r0, r1
	ldr	r1, [sp, #12]                   @ 4-byte Reload
	str	r6, [sp, #104]                  @ 4-byte Spill
	adds	r3, r4, r3
	adcs	r1, r10, r1
	ldr	r4, [sp, #36]                   @ 4-byte Reload
	umull	r1, r3, r0, r5
	str	lr, [sp, #100]                  @ 4-byte Spill
	adcs	r1, r4, r1
	ldr	r4, [sp, #48]                   @ 4-byte Reload
	adcs	r11, r3, r11
	umull	r5, r3, r0, r6
	adcs	r10, r9, r5
	umull	r6, r5, r0, lr
	adcs	r0, r3, r6
	ldr	r3, [sp, #72]                   @ 4-byte Reload
	adc	r5, r5, #0
	mov	r6, #0
	umlal	r8, r7, r3, r2
	ldr	r2, [sp, #52]                   @ 4-byte Reload
	adds	r2, r4, r2
	ldr	r2, [sp, #40]                   @ 4-byte Reload
	adcs	r2, r2, r8
	str	r2, [sp, #52]                   @ 4-byte Spill
	ldr	r2, [sp, #44]                   @ 4-byte Reload
	adcs	r2, r2, r7
	str	r2, [sp, #48]                   @ 4-byte Spill
	ldr	r2, [sp, #32]                   @ 4-byte Reload
	adcs	r1, r1, r2
	str	r1, [sp, #44]                   @ 4-byte Spill
	ldr	r1, [sp, #28]                   @ 4-byte Reload
	adcs	r1, r11, r1
	str	r1, [sp, #40]                   @ 4-byte Spill
	ldr	r1, [sp, #24]                   @ 4-byte Reload
	ldr	r11, [sp, #108]                 @ 4-byte Reload
	adcs	r1, r10, r1
	str	r1, [sp, #36]                   @ 4-byte Spill
	ldr	r1, [sp, #20]                   @ 4-byte Reload
	adcs	r0, r0, r1
	str	r0, [sp, #32]                   @ 4-byte Spill
	ldr	r0, [sp, #16]                   @ 4-byte Reload
	adcs	r0, r5, r0
	str	r0, [sp, #28]                   @ 4-byte Spill
	adc	r0, r6, #0
	str	r0, [sp, #24]                   @ 4-byte Spill
	ldr	r0, [sp, #96]                   @ 4-byte Reload
	umull	r2, r1, r12, r0
	ldr	r0, [sp, #92]                   @ 4-byte Reload
	str	r1, [sp, #20]                   @ 4-byte Spill
	umull	r9, r1, r12, r0
	ldr	r0, [sp, #84]                   @ 4-byte Reload
	str	r2, [sp, #16]                   @ 4-byte Spill
	umull	r2, lr, r12, r3
	str	r1, [sp, #8]                    @ 4-byte Spill
	umull	r7, r8, r12, r0
	ldr	r0, [sp, #88]                   @ 4-byte Reload
	umull	r5, r6, r12, r0
	ldr	r0, [sp, #80]                   @ 4-byte Reload
	umull	r1, r4, r12, r0
	umull	r10, r0, r12, r11
	str	r10, [sp, #12]                  @ 4-byte Spill
	adds	r2, r0, r2
	adcs	r2, lr, r1
	umlal	r0, r1, r12, r3
	adcs	lr, r4, r5
	adcs	r7, r6, r7
	ldr	r2, [sp, #16]                   @ 4-byte Reload
	adcs	r6, r8, r9
	ldr	r4, [sp, #8]                    @ 4-byte Reload
	ldr	r3, [sp, #52]                   @ 4-byte Reload
	adcs	r4, r4, r2
	ldr	r2, [sp, #20]                   @ 4-byte Reload
	adc	r5, r2, #0
	ldr	r2, [sp, #12]                   @ 4-byte Reload
	adds	r3, r3, r2
	ldr	r2, [sp, #48]                   @ 4-byte Reload
	str	r3, [sp, #20]                   @ 4-byte Spill
	adcs	r0, r2, r0
	str	r0, [sp, #52]                   @ 4-byte Spill
	ldr	r0, [sp, #44]                   @ 4-byte Reload
	adcs	r0, r0, r1
	str	r0, [sp, #48]                   @ 4-byte Spill
	ldr	r0, [sp, #40]                   @ 4-byte Reload
	ldr	r1, [sp, #116]                  @ 4-byte Reload
	adcs	r0, r0, lr
	str	r0, [sp, #44]                   @ 4-byte Spill
	ldr	r0, [sp, #36]                   @ 4-byte Reload
	adcs	r0, r0, r7
	str	r0, [sp, #40]                   @ 4-byte Spill
	ldr	r0, [sp, #32]                   @ 4-byte Reload
	ldr	r7, [sp, #100]                  @ 4-byte Reload
	adcs	r0, r0, r6
	str	r0, [sp, #36]                   @ 4-byte Spill
	ldr	r0, [sp, #28]                   @ 4-byte Reload
	adcs	r0, r0, r4
	str	r0, [sp, #32]                   @ 4-byte Spill
	ldr	r0, [sp, #24]                   @ 4-byte Reload
	adcs	r0, r0, r5
	str	r0, [sp, #28]                   @ 4-byte Spill
	mov	r0, #0
	adc	r0, r0, #0
	str	r0, [sp, #24]                   @ 4-byte Spill
	mul	r0, r1, r3
	ldr	r1, [sp, #128]                  @ 4-byte Reload
	umull	r12, r2, r0, r1
	ldr	r1, [sp, #112]                  @ 4-byte Reload
	umull	r4, r6, r0, r7
	ldr	r7, [sp, #104]                  @ 4-byte Reload
	str	r2, [sp, #12]                   @ 4-byte Spill
	umull	r2, r3, r0, r1
	ldr	r1, [sp, #76]                   @ 4-byte Reload
	stmib	sp, {r4, r6}                    @ 8-byte Folded Spill
	umull	r10, r6, r0, r7
	ldr	r7, [sp, #120]                  @ 4-byte Reload
	str	r2, [sp, #16]                   @ 4-byte Spill
	mov	r5, r3
	umull	r11, lr, r0, r1
	mov	r2, r12
	str	r6, [sp]                        @ 4-byte Spill
	umull	r8, r9, r0, r7
	ldr	r7, [sp, #124]                  @ 4-byte Reload
	umull	r4, r6, r0, r7
	ldr	r7, [sp]                        @ 4-byte Reload
	umlal	r5, r2, r0, r1
	adds	r0, r3, r11
	adcs	r0, lr, r12
	ldr	r0, [sp, #12]                   @ 4-byte Reload
	ldr	r11, [sp, #108]                 @ 4-byte Reload
	adcs	r12, r0, r4
	ldr	r0, [sp, #4]                    @ 4-byte Reload
	adcs	r1, r6, r8
	ldr	r4, [sp, #20]                   @ 4-byte Reload
	adcs	r3, r9, r10
	ldr	r10, [sp, #56]                  @ 4-byte Reload
	adcs	r7, r7, r0
	ldr	r0, [sp, #8]                    @ 4-byte Reload
	adc	r6, r0, #0
	ldr	r0, [sp, #16]                   @ 4-byte Reload
	adds	r4, r4, r0
	ldr	r4, [sp, #52]                   @ 4-byte Reload
	adcs	r5, r4, r5
	str	r5, [sp, #52]                   @ 4-byte Spill
	ldr	r5, [sp, #48]                   @ 4-byte Reload
	adcs	r2, r5, r2
	str	r2, [sp, #48]                   @ 4-byte Spill
	ldr	r2, [sp, #44]                   @ 4-byte Reload
	adcs	r0, r2, r12
	str	r0, [sp, #44]                   @ 4-byte Spill
	ldr	r0, [sp, #40]                   @ 4-byte Reload
	adcs	r0, r0, r1
	str	r0, [sp, #40]                   @ 4-byte Spill
	ldr	r0, [sp, #36]                   @ 4-byte Reload
	adcs	r0, r0, r3
	str	r0, [sp, #36]                   @ 4-byte Spill
	ldr	r0, [sp, #32]                   @ 4-byte Reload
	ldr	r3, [sp, #72]                   @ 4-byte Reload
	adcs	r0, r0, r7
	str	r0, [sp, #32]                   @ 4-byte Spill
	ldr	r0, [sp, #28]                   @ 4-byte Reload
	adcs	r0, r0, r6
	str	r0, [sp, #28]                   @ 4-byte Spill
	ldr	r0, [sp, #24]                   @ 4-byte Reload
	adc	r0, r0, #0
	str	r0, [sp, #24]                   @ 4-byte Spill
	ldr	r0, [sp, #96]                   @ 4-byte Reload
	umull	r2, r1, r10, r0
	ldr	r0, [sp, #92]                   @ 4-byte Reload
	str	r1, [sp, #20]                   @ 4-byte Spill
	umull	r8, r1, r10, r0
	ldr	r0, [sp, #84]                   @ 4-byte Reload
	str	r2, [sp, #16]                   @ 4-byte Spill
	umull	r2, r12, r10, r3
	str	r1, [sp, #8]                    @ 4-byte Spill
	umull	r6, r7, r10, r0
	ldr	r0, [sp, #88]                   @ 4-byte Reload
	umull	r4, r5, r10, r0
	ldr	r0, [sp, #80]                   @ 4-byte Reload
	umull	r1, lr, r10, r0
	umull	r9, r0, r10, r11
	str	r9, [sp, #12]                   @ 4-byte Spill
	adds	r2, r0, r2
	adcs	r2, r12, r1
	umlal	r0, r1, r10, r3
	adcs	r2, lr, r4
	adcs	r12, r5, r6
	adcs	r6, r7, r8
	ldr	r7, [sp, #16]                   @ 4-byte Reload
	ldr	r5, [sp, #8]                    @ 4-byte Reload
	ldr	r3, [sp, #52]                   @ 4-byte Reload
	adcs	r4, r5, r7
	ldr	r7, [sp, #20]                   @ 4-byte Reload
	adc	r5, r7, #0
	ldr	r7, [sp, #12]                   @ 4-byte Reload
	adds	r3, r3, r7
	ldr	r7, [sp, #48]                   @ 4-byte Reload
	str	r3, [sp, #20]                   @ 4-byte Spill
	adcs	r0, r7, r0
	str	r0, [sp, #56]                   @ 4-byte Spill
	ldr	r0, [sp, #44]                   @ 4-byte Reload
	adcs	r0, r0, r1
	str	r0, [sp, #52]                   @ 4-byte Spill
	ldr	r0, [sp, #40]                   @ 4-byte Reload
	ldr	r1, [sp, #116]                  @ 4-byte Reload
	adcs	r0, r0, r2
	str	r0, [sp, #48]                   @ 4-byte Spill
	ldr	r0, [sp, #36]                   @ 4-byte Reload
	adcs	r0, r0, r12
	str	r0, [sp, #44]                   @ 4-byte Spill
	ldr	r0, [sp, #32]                   @ 4-byte Reload
	adcs	r0, r0, r6
	str	r0, [sp, #40]                   @ 4-byte Spill
	ldr	r0, [sp, #28]                   @ 4-byte Reload
	ldr	r6, [sp, #76]                   @ 4-byte Reload
	adcs	r0, r0, r4
	str	r0, [sp, #36]                   @ 4-byte Spill
	ldr	r0, [sp, #24]                   @ 4-byte Reload
	adcs	r0, r0, r5
	str	r0, [sp, #32]                   @ 4-byte Spill
	mov	r0, #0
	adc	r0, r0, #0
	str	r0, [sp, #28]                   @ 4-byte Spill
	mul	r0, r1, r3
	ldr	r1, [sp, #128]                  @ 4-byte Reload
	umull	r9, r2, r0, r1
	str	r2, [sp, #16]                   @ 4-byte Spill
	ldr	r2, [sp, #112]                  @ 4-byte Reload
	umull	r7, r1, r0, r2
	mov	r2, r9
	str	r7, [sp, #24]                   @ 4-byte Spill
	ldr	r7, [sp, #100]                  @ 4-byte Reload
	mov	r5, r1
	umlal	r5, r2, r0, r6
	umull	r3, r4, r0, r7
	ldr	r7, [sp, #104]                  @ 4-byte Reload
	str	r3, [sp, #8]                    @ 4-byte Spill
	umull	r3, r10, r0, r7
	ldr	r7, [sp, #120]                  @ 4-byte Reload
	str	r4, [sp, #12]                   @ 4-byte Spill
	umull	r12, r8, r0, r7
	ldr	r7, [sp, #124]                  @ 4-byte Reload
	umull	lr, r4, r0, r7
	umull	r11, r7, r0, r6
	adds	r0, r1, r11
	ldr	r11, [sp, #108]                 @ 4-byte Reload
	adcs	r0, r7, r9
	ldr	r0, [sp, #16]                   @ 4-byte Reload
	adcs	lr, r0, lr
	ldr	r0, [sp, #8]                    @ 4-byte Reload
	adcs	r1, r4, r12
	ldr	r4, [sp, #20]                   @ 4-byte Reload
	adcs	r3, r8, r3
	adcs	r7, r10, r0
	ldr	r0, [sp, #12]                   @ 4-byte Reload
	ldr	r10, [sp, #60]                  @ 4-byte Reload
	adc	r6, r0, #0
	ldr	r0, [sp, #24]                   @ 4-byte Reload
	adds	r4, r4, r0
	ldr	r4, [sp, #56]                   @ 4-byte Reload
	adcs	r5, r4, r5
	str	r5, [sp, #56]                   @ 4-byte Spill
	ldr	r5, [sp, #52]                   @ 4-byte Reload
	adcs	r2, r5, r2
	str	r2, [sp, #52]                   @ 4-byte Spill
	ldr	r2, [sp, #48]                   @ 4-byte Reload
	adcs	r0, r2, lr
	str	r0, [sp, #48]                   @ 4-byte Spill
	ldr	r0, [sp, #44]                   @ 4-byte Reload
	adcs	r0, r0, r1
	str	r0, [sp, #44]                   @ 4-byte Spill
	ldr	r0, [sp, #40]                   @ 4-byte Reload
	adcs	r0, r0, r3
	str	r0, [sp, #40]                   @ 4-byte Spill
	ldr	r0, [sp, #36]                   @ 4-byte Reload
	ldr	r3, [sp, #72]                   @ 4-byte Reload
	adcs	r0, r0, r7
	str	r0, [sp, #36]                   @ 4-byte Spill
	ldr	r0, [sp, #32]                   @ 4-byte Reload
	adcs	r0, r0, r6
	str	r0, [sp, #32]                   @ 4-byte Spill
	ldr	r0, [sp, #28]                   @ 4-byte Reload
	adc	r0, r0, #0
	str	r0, [sp, #28]                   @ 4-byte Spill
	ldr	r0, [sp, #96]                   @ 4-byte Reload
	umull	r2, r1, r10, r0
	ldr	r0, [sp, #92]                   @ 4-byte Reload
	str	r1, [sp, #24]                   @ 4-byte Spill
	umull	r8, r1, r10, r0
	ldr	r0, [sp, #84]                   @ 4-byte Reload
	str	r2, [sp, #20]                   @ 4-byte Spill
	umull	r2, r12, r10, r3
	str	r1, [sp, #12]                   @ 4-byte Spill
	umull	r6, r7, r10, r0
	ldr	r0, [sp, #88]                   @ 4-byte Reload
	umull	r4, r5, r10, r0
	ldr	r0, [sp, #80]                   @ 4-byte Reload
	umull	r1, lr, r10, r0
	umull	r9, r0, r10, r11
	str	r9, [sp, #16]                   @ 4-byte Spill
	adds	r2, r0, r2
	adcs	r2, r12, r1
	umlal	r0, r1, r10, r3
	adcs	r2, lr, r4
	adcs	r12, r5, r6
	adcs	r6, r7, r8
	ldr	r7, [sp, #20]                   @ 4-byte Reload
	ldr	r5, [sp, #12]                   @ 4-byte Reload
	ldr	r3, [sp, #56]                   @ 4-byte Reload
	adcs	r4, r5, r7
	ldr	r7, [sp, #24]                   @ 4-byte Reload
	adc	r5, r7, #0
	ldr	r7, [sp, #16]                   @ 4-byte Reload
	adds	r3, r3, r7
	ldr	r7, [sp, #52]                   @ 4-byte Reload
	str	r3, [sp, #24]                   @ 4-byte Spill
	adcs	r0, r7, r0
	str	r0, [sp, #60]                   @ 4-byte Spill
	ldr	r0, [sp, #48]                   @ 4-byte Reload
	adcs	r0, r0, r1
	str	r0, [sp, #56]                   @ 4-byte Spill
	ldr	r0, [sp, #44]                   @ 4-byte Reload
	ldr	r1, [sp, #116]                  @ 4-byte Reload
	adcs	r0, r0, r2
	str	r0, [sp, #52]                   @ 4-byte Spill
	ldr	r0, [sp, #40]                   @ 4-byte Reload
	adcs	r0, r0, r12
	str	r0, [sp, #48]                   @ 4-byte Spill
	ldr	r0, [sp, #36]                   @ 4-byte Reload
	adcs	r0, r0, r6
	str	r0, [sp, #44]                   @ 4-byte Spill
	ldr	r0, [sp, #32]                   @ 4-byte Reload
	ldr	r6, [sp, #76]                   @ 4-byte Reload
	adcs	r0, r0, r4
	str	r0, [sp, #40]                   @ 4-byte Spill
	ldr	r0, [sp, #28]                   @ 4-byte Reload
	adcs	r0, r0, r5
	str	r0, [sp, #36]                   @ 4-byte Spill
	mov	r0, #0
	adc	r0, r0, #0
	str	r0, [sp, #32]                   @ 4-byte Spill
	mul	r0, r1, r3
	ldr	r1, [sp, #128]                  @ 4-byte Reload
	umull	r9, r2, r0, r1
	str	r2, [sp, #20]                   @ 4-byte Spill
	ldr	r2, [sp, #112]                  @ 4-byte Reload
	umull	r7, r1, r0, r2
	mov	r2, r9
	str	r7, [sp, #28]                   @ 4-byte Spill
	ldr	r7, [sp, #100]                  @ 4-byte Reload
	mov	r5, r1
	umlal	r5, r2, r0, r6
	umull	r3, r4, r0, r7
	ldr	r7, [sp, #104]                  @ 4-byte Reload
	str	r3, [sp, #12]                   @ 4-byte Spill
	umull	r3, r10, r0, r7
	ldr	r7, [sp, #120]                  @ 4-byte Reload
	str	r4, [sp, #16]                   @ 4-byte Spill
	umull	r12, r8, r0, r7
	ldr	r7, [sp, #124]                  @ 4-byte Reload
	umull	lr, r4, r0, r7
	umull	r11, r7, r0, r6
	adds	r0, r1, r11
	ldr	r11, [sp, #108]                 @ 4-byte Reload
	adcs	r0, r7, r9
	ldr	r0, [sp, #20]                   @ 4-byte Reload
	adcs	lr, r0, lr
	ldr	r0, [sp, #12]                   @ 4-byte Reload
	adcs	r1, r4, r12
	ldr	r4, [sp, #24]                   @ 4-byte Reload
	adcs	r3, r8, r3
	ldr	r12, [sp, #72]                  @ 4-byte Reload
	adcs	r7, r10, r0
	ldr	r0, [sp, #16]                   @ 4-byte Reload
	adc	r6, r0, #0
	ldr	r0, [sp, #28]                   @ 4-byte Reload
	adds	r4, r4, r0
	ldr	r4, [sp, #60]                   @ 4-byte Reload
	adcs	r5, r4, r5
	str	r5, [sp, #60]                   @ 4-byte Spill
	ldr	r5, [sp, #56]                   @ 4-byte Reload
	adcs	r2, r5, r2
	str	r2, [sp, #56]                   @ 4-byte Spill
	ldr	r2, [sp, #52]                   @ 4-byte Reload
	adcs	r0, r2, lr
	str	r0, [sp, #52]                   @ 4-byte Spill
	ldr	r0, [sp, #48]                   @ 4-byte Reload
	adcs	r0, r0, r1
	str	r0, [sp, #48]                   @ 4-byte Spill
	ldr	r0, [sp, #44]                   @ 4-byte Reload
	ldr	r1, [sp, #96]                   @ 4-byte Reload
	adcs	r0, r0, r3
	str	r0, [sp, #44]                   @ 4-byte Spill
	ldr	r0, [sp, #40]                   @ 4-byte Reload
	adcs	r0, r0, r7
	str	r0, [sp, #40]                   @ 4-byte Spill
	ldr	r0, [sp, #36]                   @ 4-byte Reload
	adcs	r0, r0, r6
	str	r0, [sp, #36]                   @ 4-byte Spill
	ldr	r0, [sp, #32]                   @ 4-byte Reload
	adc	r0, r0, #0
	str	r0, [sp, #32]                   @ 4-byte Spill
	ldr	r0, [sp, #68]                   @ 4-byte Reload
	ldr	r0, [r0, #16]
	umull	r3, r2, r0, r1
	ldr	r1, [sp, #92]                   @ 4-byte Reload
	str	r2, [sp, #28]                   @ 4-byte Spill
	umull	r9, r2, r0, r1
	ldr	r1, [sp, #84]                   @ 4-byte Reload
	str	r3, [sp, #24]                   @ 4-byte Spill
	umull	r3, lr, r0, r12
	str	r2, [sp, #16]                   @ 4-byte Spill
	umull	r7, r8, r0, r1
	ldr	r1, [sp, #88]                   @ 4-byte Reload
	umull	r5, r6, r0, r1
	ldr	r1, [sp, #80]                   @ 4-byte Reload
	umull	r2, r4, r0, r1
	umull	r10, r1, r0, r11
	str	r10, [sp, #20]                  @ 4-byte Spill
	adds	r3, r1, r3
	adcs	r3, lr, r2
	umlal	r1, r2, r0, r12
	adcs	lr, r4, r5
	adcs	r4, r6, r7
	ldr	r3, [sp, #24]                   @ 4-byte Reload
	adcs	r6, r8, r9
	ldr	r5, [sp, #16]                   @ 4-byte Reload
	ldr	r0, [sp, #60]                   @ 4-byte Reload
	adcs	r5, r5, r3
	ldr	r3, [sp, #28]                   @ 4-byte Reload
	adc	r7, r3, #0
	ldr	r3, [sp, #20]                   @ 4-byte Reload
	adds	r3, r0, r3
	ldr	r0, [sp, #56]                   @ 4-byte Reload
	str	r3, [sp, #28]                   @ 4-byte Spill
	adcs	r0, r0, r1
	str	r0, [sp, #60]                   @ 4-byte Spill
	ldr	r0, [sp, #52]                   @ 4-byte Reload
	adcs	r0, r0, r2
	str	r0, [sp, #56]                   @ 4-byte Spill
	ldr	r0, [sp, #48]                   @ 4-byte Reload
	adcs	r0, r0, lr
	str	r0, [sp, #52]                   @ 4-byte Spill
	ldr	r0, [sp, #44]                   @ 4-byte Reload
	adcs	r0, r0, r4
	str	r0, [sp, #48]                   @ 4-byte Spill
	ldr	r0, [sp, #40]                   @ 4-byte Reload
	adcs	r0, r0, r6
	str	r0, [sp, #44]                   @ 4-byte Spill
	ldr	r0, [sp, #36]                   @ 4-byte Reload
	ldr	r6, [sp, #76]                   @ 4-byte Reload
	adcs	r0, r0, r5
	str	r0, [sp, #40]                   @ 4-byte Spill
	ldr	r0, [sp, #32]                   @ 4-byte Reload
	adcs	r0, r0, r7
	str	r0, [sp, #36]                   @ 4-byte Spill
	mov	r0, #0
	adc	r0, r0, #0
	str	r0, [sp, #32]                   @ 4-byte Spill
	ldr	r0, [sp, #116]                  @ 4-byte Reload
	mul	r1, r0, r3
	ldr	r0, [sp, #128]                  @ 4-byte Reload
	umull	r9, r2, r1, r0
	str	r2, [sp, #20]                   @ 4-byte Spill
	ldr	r2, [sp, #112]                  @ 4-byte Reload
	umull	r7, r0, r1, r2
	mov	r2, r9
	str	r7, [sp, #24]                   @ 4-byte Spill
	ldr	r7, [sp, #100]                  @ 4-byte Reload
	mov	r5, r0
	umlal	r5, r2, r1, r6
	umull	r3, r4, r1, r7
	ldr	r7, [sp, #104]                  @ 4-byte Reload
	str	r3, [sp, #12]                   @ 4-byte Spill
	umull	r3, r10, r1, r7
	ldr	r7, [sp, #120]                  @ 4-byte Reload
	str	r4, [sp, #16]                   @ 4-byte Spill
	umull	r12, r8, r1, r7
	ldr	r7, [sp, #124]                  @ 4-byte Reload
	umull	lr, r4, r1, r7
	umull	r11, r7, r1, r6
	adds	r0, r0, r11
	ldr	r11, [sp, #108]                 @ 4-byte Reload
	adcs	r0, r7, r9
	ldr	r0, [sp, #20]                   @ 4-byte Reload
	adcs	lr, r0, lr
	ldr	r0, [sp, #12]                   @ 4-byte Reload
	adcs	r1, r4, r12
	ldr	r4, [sp, #28]                   @ 4-byte Reload
	adcs	r3, r8, r3
	ldr	r12, [sp, #72]                  @ 4-byte Reload
	adcs	r7, r10, r0
	ldr	r0, [sp, #16]                   @ 4-byte Reload
	adc	r6, r0, #0
	ldr	r0, [sp, #24]                   @ 4-byte Reload
	adds	r4, r4, r0
	ldr	r4, [sp, #60]                   @ 4-byte Reload
	adcs	r5, r4, r5
	str	r5, [sp, #60]                   @ 4-byte Spill
	ldr	r5, [sp, #56]                   @ 4-byte Reload
	adcs	r2, r5, r2
	str	r2, [sp, #56]                   @ 4-byte Spill
	ldr	r2, [sp, #52]                   @ 4-byte Reload
	adcs	r0, r2, lr
	str	r0, [sp, #52]                   @ 4-byte Spill
	ldr	r0, [sp, #48]                   @ 4-byte Reload
	adcs	r0, r0, r1
	str	r0, [sp, #48]                   @ 4-byte Spill
	ldr	r0, [sp, #44]                   @ 4-byte Reload
	ldr	r1, [sp, #96]                   @ 4-byte Reload
	adcs	r0, r0, r3
	str	r0, [sp, #44]                   @ 4-byte Spill
	ldr	r0, [sp, #40]                   @ 4-byte Reload
	adcs	r0, r0, r7
	str	r0, [sp, #40]                   @ 4-byte Spill
	ldr	r0, [sp, #36]                   @ 4-byte Reload
	adcs	r0, r0, r6
	str	r0, [sp, #36]                   @ 4-byte Spill
	ldr	r0, [sp, #32]                   @ 4-byte Reload
	adc	r0, r0, #0
	str	r0, [sp, #32]                   @ 4-byte Spill
	ldr	r0, [sp, #68]                   @ 4-byte Reload
	ldr	r0, [r0, #20]
	umull	r3, r2, r0, r1
	ldr	r1, [sp, #92]                   @ 4-byte Reload
	str	r2, [sp, #28]                   @ 4-byte Spill
	umull	r9, r2, r0, r1
	ldr	r1, [sp, #84]                   @ 4-byte Reload
	str	r3, [sp, #24]                   @ 4-byte Spill
	umull	r3, lr, r0, r12
	str	r2, [sp, #16]                   @ 4-byte Spill
	umull	r7, r8, r0, r1
	ldr	r1, [sp, #88]                   @ 4-byte Reload
	umull	r5, r6, r0, r1
	ldr	r1, [sp, #80]                   @ 4-byte Reload
	umull	r2, r4, r0, r1
	umull	r10, r1, r0, r11
	str	r10, [sp, #20]                  @ 4-byte Spill
	adds	r3, r1, r3
	adcs	r3, lr, r2
	umlal	r1, r2, r0, r12
	adcs	lr, r4, r5
	adcs	r4, r6, r7
	ldr	r3, [sp, #24]                   @ 4-byte Reload
	adcs	r6, r8, r9
	ldr	r5, [sp, #16]                   @ 4-byte Reload
	ldr	r0, [sp, #60]                   @ 4-byte Reload
	adcs	r5, r5, r3
	ldr	r3, [sp, #28]                   @ 4-byte Reload
	adc	r7, r3, #0
	ldr	r3, [sp, #20]                   @ 4-byte Reload
	adds	r3, r0, r3
	ldr	r0, [sp, #56]                   @ 4-byte Reload
	str	r3, [sp, #28]                   @ 4-byte Spill
	adcs	r0, r0, r1
	str	r0, [sp, #60]                   @ 4-byte Spill
	ldr	r0, [sp, #52]                   @ 4-byte Reload
	adcs	r0, r0, r2
	str	r0, [sp, #56]                   @ 4-byte Spill
	ldr	r0, [sp, #48]                   @ 4-byte Reload
	adcs	r0, r0, lr
	str	r0, [sp, #52]                   @ 4-byte Spill
	ldr	r0, [sp, #44]                   @ 4-byte Reload
	adcs	r0, r0, r4
	str	r0, [sp, #48]                   @ 4-byte Spill
	ldr	r0, [sp, #40]                   @ 4-byte Reload
	adcs	r0, r0, r6
	str	r0, [sp, #44]                   @ 4-byte Spill
	ldr	r0, [sp, #36]                   @ 4-byte Reload
	ldr	r6, [sp, #76]                   @ 4-byte Reload
	adcs	r0, r0, r5
	str	r0, [sp, #40]                   @ 4-byte Spill
	ldr	r0, [sp, #32]                   @ 4-byte Reload
	adcs	r0, r0, r7
	str	r0, [sp, #36]                   @ 4-byte Spill
	mov	r0, #0
	adc	r0, r0, #0
	str	r0, [sp, #32]                   @ 4-byte Spill
	ldr	r0, [sp, #116]                  @ 4-byte Reload
	mul	r1, r0, r3
	ldr	r0, [sp, #128]                  @ 4-byte Reload
	umull	r9, r2, r1, r0
	str	r2, [sp, #20]                   @ 4-byte Spill
	ldr	r2, [sp, #112]                  @ 4-byte Reload
	umull	r7, r0, r1, r2
	mov	r2, r9
	str	r7, [sp, #24]                   @ 4-byte Spill
	ldr	r7, [sp, #100]                  @ 4-byte Reload
	mov	r5, r0
	umlal	r5, r2, r1, r6
	umull	r3, r4, r1, r7
	ldr	r7, [sp, #104]                  @ 4-byte Reload
	str	r3, [sp, #12]                   @ 4-byte Spill
	umull	r3, r10, r1, r7
	ldr	r7, [sp, #120]                  @ 4-byte Reload
	str	r4, [sp, #16]                   @ 4-byte Spill
	umull	r12, r8, r1, r7
	ldr	r7, [sp, #124]                  @ 4-byte Reload
	umull	lr, r4, r1, r7
	umull	r11, r7, r1, r6
	adds	r0, r0, r11
	ldr	r11, [sp, #108]                 @ 4-byte Reload
	adcs	r0, r7, r9
	ldr	r0, [sp, #20]                   @ 4-byte Reload
	adcs	lr, r0, lr
	ldr	r0, [sp, #12]                   @ 4-byte Reload
	adcs	r1, r4, r12
	ldr	r4, [sp, #28]                   @ 4-byte Reload
	adcs	r3, r8, r3
	ldr	r12, [sp, #72]                  @ 4-byte Reload
	adcs	r7, r10, r0
	ldr	r0, [sp, #16]                   @ 4-byte Reload
	adc	r6, r0, #0
	ldr	r0, [sp, #24]                   @ 4-byte Reload
	adds	r4, r4, r0
	ldr	r4, [sp, #60]                   @ 4-byte Reload
	adcs	r5, r4, r5
	str	r5, [sp, #60]                   @ 4-byte Spill
	ldr	r5, [sp, #56]                   @ 4-byte Reload
	adcs	r2, r5, r2
	str	r2, [sp, #56]                   @ 4-byte Spill
	ldr	r2, [sp, #52]                   @ 4-byte Reload
	adcs	r0, r2, lr
	str	r0, [sp, #52]                   @ 4-byte Spill
	ldr	r0, [sp, #48]                   @ 4-byte Reload
	adcs	r0, r0, r1
	str	r0, [sp, #48]                   @ 4-byte Spill
	ldr	r0, [sp, #44]                   @ 4-byte Reload
	ldr	r1, [sp, #96]                   @ 4-byte Reload
	adcs	r0, r0, r3
	str	r0, [sp, #44]                   @ 4-byte Spill
	ldr	r0, [sp, #40]                   @ 4-byte Reload
	adcs	r0, r0, r7
	str	r0, [sp, #40]                   @ 4-byte Spill
	ldr	r0, [sp, #36]                   @ 4-byte Reload
	adcs	r0, r0, r6
	str	r0, [sp, #36]                   @ 4-byte Spill
	ldr	r0, [sp, #32]                   @ 4-byte Reload
	adc	r0, r0, #0
	str	r0, [sp, #32]                   @ 4-byte Spill
	ldr	r0, [sp, #68]                   @ 4-byte Reload
	ldr	r0, [r0, #24]
	umull	r3, r2, r0, r1
	ldr	r1, [sp, #92]                   @ 4-byte Reload
	str	r2, [sp, #68]                   @ 4-byte Spill
	umull	r9, r2, r0, r1
	ldr	r1, [sp, #84]                   @ 4-byte Reload
	str	r3, [sp, #28]                   @ 4-byte Spill
	umull	r3, lr, r0, r12
	str	r2, [sp, #24]                   @ 4-byte Spill
	umull	r7, r8, r0, r1
	ldr	r1, [sp, #88]                   @ 4-byte Reload
	umull	r5, r6, r0, r1
	ldr	r1, [sp, #80]                   @ 4-byte Reload
	umull	r2, r4, r0, r1
	umull	r10, r1, r0, r11
	str	r10, [sp, #96]                  @ 4-byte Spill
	adds	r3, r1, r3
	adcs	r3, lr, r2
	umlal	r1, r2, r0, r12
	adcs	lr, r4, r5
	adcs	r6, r6, r7
	ldr	r5, [sp, #28]                   @ 4-byte Reload
	adcs	r7, r8, r9
	ldr	r3, [sp, #24]                   @ 4-byte Reload
	ldr	r4, [sp, #68]                   @ 4-byte Reload
	adcs	r5, r3, r5
	ldr	r0, [sp, #60]                   @ 4-byte Reload
	ldr	r3, [sp, #96]                   @ 4-byte Reload
	adc	r4, r4, #0
	ldr	r9, [sp, #76]                   @ 4-byte Reload
	adds	r10, r0, r3
	ldr	r0, [sp, #56]                   @ 4-byte Reload
	ldr	r8, [sp, #104]                  @ 4-byte Reload
	adcs	r0, r0, r1
	str	r0, [sp, #108]                  @ 4-byte Spill
	ldr	r0, [sp, #52]                   @ 4-byte Reload
	adcs	r0, r0, r2
	str	r0, [sp, #96]                   @ 4-byte Spill
	ldr	r0, [sp, #48]                   @ 4-byte Reload
	adcs	r0, r0, lr
	str	r0, [sp, #92]                   @ 4-byte Spill
	ldr	r0, [sp, #44]                   @ 4-byte Reload
	adcs	r0, r0, r6
	str	r0, [sp, #88]                   @ 4-byte Spill
	ldr	r0, [sp, #40]                   @ 4-byte Reload
	ldr	r6, [sp, #100]                  @ 4-byte Reload
	adcs	r0, r0, r7
	str	r0, [sp, #84]                   @ 4-byte Spill
	ldr	r0, [sp, #36]                   @ 4-byte Reload
	adcs	r0, r0, r5
	str	r0, [sp, #80]                   @ 4-byte Spill
	ldr	r0, [sp, #32]                   @ 4-byte Reload
	adcs	r0, r0, r4
	str	r0, [sp, #72]                   @ 4-byte Spill
	mov	r0, #0
	ldr	r4, [sp, #120]                  @ 4-byte Reload
	adc	r0, r0, #0
	str	r0, [sp, #68]                   @ 4-byte Spill
	ldr	r0, [sp, #116]                  @ 4-byte Reload
	mul	r1, r0, r10
	ldr	r0, [sp, #112]                  @ 4-byte Reload
	mov	lr, r0
	umull	r7, r5, r1, r0
	umull	r2, r3, r1, r9
	str	r7, [sp, #116]                  @ 4-byte Spill
	ldr	r7, [sp, #128]                  @ 4-byte Reload
	adds	r2, r5, r2
	umull	r0, r2, r1, r7
	ldr	r7, [sp, #124]                  @ 4-byte Reload
	adcs	r3, r3, r0
	umlal	r5, r0, r1, r9
	umull	r3, r12, r1, r7
	adcs	r2, r2, r3
	str	r2, [sp, #60]                   @ 4-byte Spill
	umull	r2, r3, r1, r4
	adcs	r4, r12, r2
	umull	r7, r2, r1, r8
	adcs	r11, r3, r7
	umull	r7, r3, r1, r6
	ldr	r1, [sp, #116]                  @ 4-byte Reload
	adcs	r12, r2, r7
	mov	r7, r9
	adc	r9, r3, #0
	adds	r1, r10, r1
	ldr	r1, [sp, #108]                  @ 4-byte Reload
	ldr	r2, [sp, #96]                   @ 4-byte Reload
	adcs	r1, r1, r5
	ldr	r3, [sp, #60]                   @ 4-byte Reload
	adcs	r2, r2, r0
	ldr	r0, [sp, #92]                   @ 4-byte Reload
	ldr	r5, [sp, #88]                   @ 4-byte Reload
	adcs	r0, r0, r3
	str	r1, [sp, #116]                  @ 4-byte Spill
	adcs	r3, r5, r4
	ldr	r5, [sp, #84]                   @ 4-byte Reload
	ldr	r4, [sp, #80]                   @ 4-byte Reload
	adcs	r5, r5, r11
	str	r0, [sp, #96]                   @ 4-byte Spill
	adcs	r11, r4, r12
	ldr	r4, [sp, #72]                   @ 4-byte Reload
	str	r2, [sp, #108]                  @ 4-byte Spill
	adcs	r12, r4, r9
	ldr	r4, [sp, #68]                   @ 4-byte Reload
	str	r3, [sp, #92]                   @ 4-byte Spill
	adc	r9, r4, #0
	subs	r10, r1, lr
	ldr	r1, [sp, #128]                  @ 4-byte Reload
	sbcs	lr, r2, r7
	sbcs	r7, r0, r1
	ldr	r0, [sp, #124]                  @ 4-byte Reload
	sbcs	r4, r3, r0
	ldr	r0, [sp, #120]                  @ 4-byte Reload
	ldr	r3, [sp, #64]                   @ 4-byte Reload
	sbcs	r1, r5, r0
	sbcs	r2, r11, r8
	sbcs	r0, r12, r6
	sbc	r6, r9, #0
	ands	r6, r6, #1
	movne	r0, r12
	movne	r2, r11
	str	r0, [r3, #24]
	movne	r1, r5
	ldr	r0, [sp, #92]                   @ 4-byte Reload
	cmp	r6, #0
	str	r2, [r3, #20]
	str	r1, [r3, #16]
	movne	r4, r0
	ldr	r0, [sp, #96]                   @ 4-byte Reload
	str	r4, [r3, #12]
	movne	r7, r0
	ldr	r0, [sp, #108]                  @ 4-byte Reload
	str	r7, [r3, #8]
	movne	lr, r0
	ldr	r0, [sp, #116]                  @ 4-byte Reload
	cmp	r6, #0
	str	lr, [r3, #4]
	movne	r10, r0
	str	r10, [r3]
	add	sp, sp, #132
	pop	{r4, r5, r6, r7, r8, r9, r10, r11, lr}
	mov	pc, lr
.Lfunc_end26:
	.size	mcl_fp_mont7L, .Lfunc_end26-mcl_fp_mont7L
	.cantunwind
	.fnend
                                        @ -- End function
	.globl	mcl_fp_montNF7L                 @ -- Begin function mcl_fp_montNF7L
	.p2align	2
	.type	mcl_fp_montNF7L,%function
	.code	32                              @ @mcl_fp_montNF7L
mcl_fp_montNF7L:
	.fnstart
@ %bb.0:
	.save	{r4, r5, r6, r7, r8, r9, r10, r11, lr}
	push	{r4, r5, r6, r7, r8, r9, r10, r11, lr}
	.pad	#104
	sub	sp, sp, #104
	str	r0, [sp, #36]                   @ 4-byte Spill
	mov	r0, r2
	str	r2, [sp, #40]                   @ 4-byte Spill
	ldm	r2, {r4, r12}
	ldr	r6, [r1, #4]
	ldr	r7, [r1]
	ldr	r5, [r1, #8]
	umull	r9, r8, r6, r4
	mov	r11, r6
	str	r6, [sp, #60]                   @ 4-byte Spill
	ldr	r0, [r0, #12]
	umull	lr, r10, r7, r4
	str	r0, [sp, #32]                   @ 4-byte Spill
	ldr	r0, [r1, #12]
	str	r7, [sp, #96]                   @ 4-byte Spill
	str	r0, [sp, #52]                   @ 4-byte Spill
	str	r5, [sp, #80]                   @ 4-byte Spill
	adds	r6, r10, r9
	ldr	r2, [r2, #8]
	umull	r6, r9, r5, r4
	ldr	r5, [r1, #20]
	str	r5, [sp, #48]                   @ 4-byte Spill
	str	r2, [sp]                        @ 4-byte Spill
	adcs	r7, r8, r6
	umlal	r10, r6, r11, r4
	umull	r7, r8, r0, r4
	adcs	r0, r9, r7
	str	r0, [sp, #88]                   @ 4-byte Spill
	ldr	r0, [r1, #16]
	str	r0, [sp, #68]                   @ 4-byte Spill
	umull	r7, r9, r0, r4
	adcs	r0, r8, r7
	str	r0, [sp, #84]                   @ 4-byte Spill
	umull	r7, r0, r5, r4
	adcs	r5, r9, r7
	str	r5, [sp, #76]                   @ 4-byte Spill
	ldr	r5, [r1, #24]
	str	r5, [sp, #72]                   @ 4-byte Spill
	ldr	r7, [r3, #4]
	umull	r1, r9, r5, r4
	ldr	r5, [r3]
	str	r5, [sp, #44]                   @ 4-byte Spill
	str	r7, [sp, #56]                   @ 4-byte Spill
	adcs	r0, r0, r1
	ldr	r1, [r3, #-4]
	str	r0, [sp, #28]                   @ 4-byte Spill
	adc	r0, r9, #0
	str	r0, [sp, #24]                   @ 4-byte Spill
	mul	r0, r1, lr
	str	r1, [sp, #64]                   @ 4-byte Spill
	ldr	r9, [r3, #8]
	str	r9, [sp, #100]                  @ 4-byte Spill
	umull	r1, r2, r0, r5
	str	r2, [sp, #20]                   @ 4-byte Spill
	adds	r1, lr, r1
	umull	r1, lr, r0, r7
	adcs	r11, r10, r1
	umull	r5, r1, r0, r9
	str	r1, [sp, #16]                   @ 4-byte Spill
	ldr	r1, [r3, #12]
	adcs	r9, r6, r5
	str	r1, [sp, #92]                   @ 4-byte Spill
	umull	r5, r10, r0, r1
	ldr	r1, [sp, #88]                   @ 4-byte Reload
	adcs	r7, r1, r5
	ldr	r1, [r3, #16]
	str	r1, [sp, #88]                   @ 4-byte Spill
	umull	r5, r8, r0, r1
	ldr	r1, [sp, #84]                   @ 4-byte Reload
	adcs	r4, r1, r5
	ldr	r1, [r3, #20]
	str	r1, [sp, #84]                   @ 4-byte Spill
	umull	r5, r6, r0, r1
	ldr	r1, [sp, #76]                   @ 4-byte Reload
	adcs	r5, r1, r5
	ldr	r1, [r3, #24]
	str	r1, [sp, #76]                   @ 4-byte Spill
	umull	r3, r2, r0, r1
	ldr	r0, [sp, #28]                   @ 4-byte Reload
	ldr	r1, [sp, #24]                   @ 4-byte Reload
	adcs	r0, r0, r3
	adc	r3, r1, #0
	ldr	r1, [sp, #20]                   @ 4-byte Reload
	adds	r11, r11, r1
	adcs	r1, r9, lr
	str	r1, [sp, #28]                   @ 4-byte Spill
	ldr	r1, [sp, #16]                   @ 4-byte Reload
	adcs	r1, r7, r1
	str	r1, [sp, #24]                   @ 4-byte Spill
	adcs	r1, r4, r10
	str	r1, [sp, #20]                   @ 4-byte Spill
	adcs	r1, r5, r8
	str	r1, [sp, #16]                   @ 4-byte Spill
	adcs	r0, r0, r6
	str	r0, [sp, #12]                   @ 4-byte Spill
	adc	r0, r3, r2
	ldr	r2, [sp, #60]                   @ 4-byte Reload
	ldr	r1, [sp, #96]                   @ 4-byte Reload
	str	r0, [sp, #8]                    @ 4-byte Spill
	umull	r3, r4, r12, r2
	ldr	r7, [sp, #80]                   @ 4-byte Reload
	ldr	r5, [sp, #52]                   @ 4-byte Reload
	umull	r9, r0, r12, r1
	adds	r3, r0, r3
	umull	r1, r3, r12, r7
	ldr	r7, [sp, #48]                   @ 4-byte Reload
	adcs	r4, r4, r1
	umlal	r0, r1, r12, r2
	umull	r4, r6, r12, r5
	ldr	r5, [sp, #68]                   @ 4-byte Reload
	adcs	r10, r3, r4
	umull	r4, r3, r12, r5
	adcs	r8, r6, r4
	umull	r6, r4, r12, r7
	ldr	r7, [sp, #72]                   @ 4-byte Reload
	adcs	r5, r3, r6
	umull	r6, r3, r12, r7
	ldr	r7, [sp, #28]                   @ 4-byte Reload
	adcs	r4, r4, r6
	adc	r2, r3, #0
	adds	r3, r9, r11
	adcs	r0, r0, r7
	ldr	r7, [sp, #24]                   @ 4-byte Reload
	adcs	r1, r1, r7
	ldr	r7, [sp, #20]                   @ 4-byte Reload
	adcs	r6, r10, r7
	ldr	r7, [sp, #16]                   @ 4-byte Reload
	adcs	r11, r8, r7
	ldr	r7, [sp, #12]                   @ 4-byte Reload
	ldr	r8, [sp, #56]                   @ 4-byte Reload
	adcs	r7, r5, r7
	ldr	r5, [sp, #8]                    @ 4-byte Reload
	str	r7, [sp, #16]                   @ 4-byte Spill
	adcs	r7, r4, r5
	ldr	r5, [sp, #64]                   @ 4-byte Reload
	adc	r2, r2, #0
	str	r2, [sp, #28]                   @ 4-byte Spill
	str	r7, [sp, #20]                   @ 4-byte Spill
	mul	r2, r5, r3
	ldr	r5, [sp, #44]                   @ 4-byte Reload
	umull	r4, r7, r2, r5
	str	r7, [sp, #24]                   @ 4-byte Spill
	adds	r3, r3, r4
	ldr	r4, [sp, #24]                   @ 4-byte Reload
	umull	r3, r7, r2, r8
	str	r7, [sp, #12]                   @ 4-byte Spill
	adcs	lr, r0, r3
	ldr	r0, [sp, #100]                  @ 4-byte Reload
	umull	r3, r7, r2, r0
	ldr	r0, [sp, #92]                   @ 4-byte Reload
	str	r7, [sp, #8]                    @ 4-byte Spill
	adcs	r12, r1, r3
	umull	r3, r10, r2, r0
	ldr	r0, [sp, #88]                   @ 4-byte Reload
	adcs	r3, r6, r3
	umull	r6, r9, r2, r0
	ldr	r0, [sp, #84]                   @ 4-byte Reload
	adcs	r5, r11, r6
	umull	r6, r1, r2, r0
	ldr	r11, [sp, #76]                  @ 4-byte Reload
	ldr	r0, [sp, #16]                   @ 4-byte Reload
	adcs	r6, r0, r6
	umull	r7, r0, r2, r11
	ldr	r2, [sp, #20]                   @ 4-byte Reload
	adcs	r2, r2, r7
	ldr	r7, [sp, #28]                   @ 4-byte Reload
	adc	r7, r7, #0
	adds	r4, lr, r4
	str	r4, [sp, #28]                   @ 4-byte Spill
	ldr	r4, [sp, #12]                   @ 4-byte Reload
	adcs	r4, r12, r4
	str	r4, [sp, #24]                   @ 4-byte Spill
	ldr	r4, [sp, #8]                    @ 4-byte Reload
	ldr	r12, [sp, #60]                  @ 4-byte Reload
	adcs	r3, r3, r4
	str	r3, [sp, #20]                   @ 4-byte Spill
	adcs	r3, r5, r10
	str	r3, [sp, #16]                   @ 4-byte Spill
	adcs	r3, r6, r9
	str	r3, [sp, #12]                   @ 4-byte Spill
	adcs	r1, r2, r1
	str	r1, [sp, #8]                    @ 4-byte Spill
	adc	r0, r7, r0
	str	r0, [sp, #4]                    @ 4-byte Spill
	ldr	r0, [sp]                        @ 4-byte Reload
	ldr	r1, [sp, #96]                   @ 4-byte Reload
	ldr	r3, [sp, #80]                   @ 4-byte Reload
	umull	r2, r6, r0, r12
	ldr	r5, [sp, #52]                   @ 4-byte Reload
	ldr	r4, [sp, #68]                   @ 4-byte Reload
	ldr	r7, [sp, #48]                   @ 4-byte Reload
	umull	r11, lr, r0, r1
	ldr	r9, [sp, #72]                   @ 4-byte Reload
	adds	r2, lr, r2
	umull	r1, r2, r0, r3
	adcs	r6, r6, r1
	umlal	lr, r1, r0, r12
	umull	r6, r3, r0, r5
	adcs	r5, r2, r6
	umull	r6, r2, r0, r4
	adcs	r10, r3, r6
	umull	r6, r3, r0, r7
	ldr	r7, [sp, #28]                   @ 4-byte Reload
	adcs	r4, r2, r6
	umull	r6, r2, r0, r9
	ldr	r9, [sp, #44]                   @ 4-byte Reload
	adcs	r3, r3, r6
	ldr	r6, [sp, #24]                   @ 4-byte Reload
	adc	r2, r2, #0
	adds	r7, r11, r7
	adcs	r0, lr, r6
	ldr	r6, [sp, #20]                   @ 4-byte Reload
	adcs	r1, r1, r6
	ldr	r6, [sp, #16]                   @ 4-byte Reload
	adcs	r6, r5, r6
	ldr	r5, [sp, #12]                   @ 4-byte Reload
	adcs	r11, r10, r5
	ldr	r5, [sp, #8]                    @ 4-byte Reload
	adcs	r10, r4, r5
	ldr	r5, [sp, #4]                    @ 4-byte Reload
	ldr	r4, [sp, #92]                   @ 4-byte Reload
	adcs	r3, r3, r5
	str	r3, [sp, #28]                   @ 4-byte Spill
	ldr	r3, [sp, #64]                   @ 4-byte Reload
	adc	r2, r2, #0
	str	r2, [sp, #24]                   @ 4-byte Spill
	mul	r2, r3, r7
	umull	r3, r5, r2, r9
	str	r5, [sp, #20]                   @ 4-byte Spill
	adds	r3, r7, r3
	umull	r3, r7, r2, r8
	str	r7, [sp, #16]                   @ 4-byte Spill
	ldr	r7, [sp, #100]                  @ 4-byte Reload
	adcs	r8, r0, r3
	ldr	r0, [sp, #76]                   @ 4-byte Reload
	umull	r3, lr, r2, r7
	ldr	r7, [sp, #84]                   @ 4-byte Reload
	adcs	r1, r1, r3
	umull	r3, r12, r2, r4
	ldr	r4, [sp, #88]                   @ 4-byte Reload
	adcs	r3, r6, r3
	umull	r6, r5, r2, r4
	adcs	r6, r11, r6
	umull	r4, r11, r2, r7
	adcs	r4, r10, r4
	umull	r7, r10, r2, r0
	ldr	r0, [sp, #28]                   @ 4-byte Reload
	adcs	r2, r0, r7
	ldr	r0, [sp, #24]                   @ 4-byte Reload
	adc	r7, r0, #0
	ldr	r0, [sp, #20]                   @ 4-byte Reload
	adds	r0, r8, r0
	str	r0, [sp, #28]                   @ 4-byte Spill
	ldr	r0, [sp, #16]                   @ 4-byte Reload
	ldr	r8, [sp, #52]                   @ 4-byte Reload
	adcs	r0, r1, r0
	str	r0, [sp, #24]                   @ 4-byte Spill
	adcs	r0, r3, lr
	str	r0, [sp, #20]                   @ 4-byte Spill
	adcs	r0, r6, r12
	str	r0, [sp, #16]                   @ 4-byte Spill
	adcs	r0, r4, r5
	ldr	r1, [sp, #60]                   @ 4-byte Reload
	ldr	r6, [sp, #32]                   @ 4-byte Reload
	ldr	r3, [sp, #96]                   @ 4-byte Reload
	str	r0, [sp, #12]                   @ 4-byte Spill
	adcs	r0, r2, r11
	str	r0, [sp, #8]                    @ 4-byte Spill
	adc	r0, r7, r10
	str	r0, [sp, #4]                    @ 4-byte Spill
	umull	r4, r0, r6, r1
	ldr	r7, [sp, #80]                   @ 4-byte Reload
	ldr	r10, [sp, #48]                  @ 4-byte Reload
	umull	r11, r2, r6, r3
	adds	r4, r2, r4
	umull	r3, r4, r6, r7
	adcs	r0, r0, r3
	umlal	r2, r3, r6, r1
	umull	r0, r7, r6, r8
	adcs	r5, r4, r0
	ldr	r0, [sp, #68]                   @ 4-byte Reload
	umull	r4, r1, r6, r0
	mov	r0, r6
	adcs	r4, r7, r4
	umull	r7, r12, r6, r10
	ldr	r6, [sp, #72]                   @ 4-byte Reload
	adcs	lr, r1, r7
	umull	r7, r1, r0, r6
	ldr	r0, [sp, #28]                   @ 4-byte Reload
	adcs	r7, r12, r7
	adc	r12, r1, #0
	ldr	r1, [sp, #24]                   @ 4-byte Reload
	adds	r0, r11, r0
	adcs	r2, r2, r1
	ldr	r1, [sp, #20]                   @ 4-byte Reload
	adcs	r3, r3, r1
	ldr	r1, [sp, #16]                   @ 4-byte Reload
	adcs	r6, r5, r1
	ldr	r1, [sp, #12]                   @ 4-byte Reload
	adcs	r11, r4, r1
	ldr	r1, [sp, #8]                    @ 4-byte Reload
	adcs	r1, lr, r1
	str	r1, [sp, #20]                   @ 4-byte Spill
	ldr	r1, [sp, #4]                    @ 4-byte Reload
	adcs	r1, r7, r1
	str	r1, [sp, #24]                   @ 4-byte Spill
	adc	r1, r12, #0
	str	r1, [sp, #32]                   @ 4-byte Spill
	ldr	r1, [sp, #64]                   @ 4-byte Reload
	ldr	r12, [sp, #76]                  @ 4-byte Reload
	mul	r4, r1, r0
	umull	r7, r1, r4, r9
	str	r1, [sp, #28]                   @ 4-byte Spill
	ldr	r1, [sp, #56]                   @ 4-byte Reload
	adds	r0, r0, r7
	umull	r0, r7, r4, r1
	ldr	r1, [sp, #100]                  @ 4-byte Reload
	str	r7, [sp, #16]                   @ 4-byte Spill
	adcs	lr, r2, r0
	umull	r2, r0, r4, r1
	ldr	r1, [sp, #92]                   @ 4-byte Reload
	ldr	r7, [sp, #84]                   @ 4-byte Reload
	str	r0, [sp, #12]                   @ 4-byte Spill
	adcs	r2, r3, r2
	umull	r3, r0, r4, r1
	ldr	r1, [sp, #88]                   @ 4-byte Reload
	str	r0, [sp, #8]                    @ 4-byte Spill
	adcs	r3, r6, r3
	umull	r6, r5, r4, r1
	ldr	r0, [sp, #20]                   @ 4-byte Reload
	adcs	r6, r11, r6
	umull	r1, r11, r4, r7
	umull	r7, r9, r4, r12
	ldr	r12, [sp, #60]                  @ 4-byte Reload
	adcs	r1, r0, r1
	ldr	r0, [sp, #24]                   @ 4-byte Reload
	adcs	r4, r0, r7
	ldr	r7, [sp, #32]                   @ 4-byte Reload
	ldr	r0, [sp, #28]                   @ 4-byte Reload
	adc	r7, r7, #0
	adds	r0, lr, r0
	str	r0, [sp, #32]                   @ 4-byte Spill
	ldr	r0, [sp, #16]                   @ 4-byte Reload
	adcs	r0, r2, r0
	str	r0, [sp, #28]                   @ 4-byte Spill
	ldr	r0, [sp, #12]                   @ 4-byte Reload
	ldr	r2, [sp, #96]                   @ 4-byte Reload
	adcs	r0, r3, r0
	str	r0, [sp, #24]                   @ 4-byte Spill
	ldr	r0, [sp, #8]                    @ 4-byte Reload
	adcs	r0, r6, r0
	str	r0, [sp, #20]                   @ 4-byte Spill
	adcs	r0, r1, r5
	str	r0, [sp, #16]                   @ 4-byte Spill
	adcs	r0, r4, r11
	str	r0, [sp, #12]                   @ 4-byte Spill
	adc	r0, r7, r9
	ldr	r9, [sp, #40]                   @ 4-byte Reload
	str	r0, [sp, #8]                    @ 4-byte Spill
	ldr	r6, [sp, #72]                   @ 4-byte Reload
	ldr	r4, [r9, #16]
	umull	r11, r3, r4, r2
	ldr	r2, [sp, #80]                   @ 4-byte Reload
	umull	r0, r1, r4, r12
	adds	r0, r3, r0
	umull	r5, r0, r4, r2
	ldr	r2, [sp, #68]                   @ 4-byte Reload
	adcs	r1, r1, r5
	umlal	r3, r5, r4, r12
	umull	r1, r7, r4, r8
	adcs	r8, r0, r1
	umull	r1, r0, r4, r2
	adcs	lr, r7, r1
	umull	r7, r1, r4, r10
	adcs	r2, r0, r7
	umull	r7, r0, r4, r6
	ldr	r6, [sp, #16]                   @ 4-byte Reload
	adcs	r1, r1, r7
	ldr	r7, [sp, #32]                   @ 4-byte Reload
	adc	r0, r0, #0
	adds	r4, r11, r7
	ldr	r7, [sp, #28]                   @ 4-byte Reload
	adcs	r3, r3, r7
	ldr	r7, [sp, #24]                   @ 4-byte Reload
	adcs	r5, r5, r7
	ldr	r7, [sp, #20]                   @ 4-byte Reload
	adcs	r7, r8, r7
	adcs	r11, lr, r6
	ldr	r6, [sp, #12]                   @ 4-byte Reload
	adcs	r10, r2, r6
	ldr	r2, [sp, #8]                    @ 4-byte Reload
	adcs	r1, r1, r2
	str	r1, [sp, #32]                   @ 4-byte Spill
	ldr	r1, [sp, #64]                   @ 4-byte Reload
	adc	r0, r0, #0
	str	r0, [sp, #28]                   @ 4-byte Spill
	ldr	r2, [sp, #44]                   @ 4-byte Reload
	mul	r0, r1, r4
	umull	r1, r6, r0, r2
	ldr	r2, [sp, #56]                   @ 4-byte Reload
	str	r6, [sp, #24]                   @ 4-byte Spill
	adds	r1, r4, r1
	ldr	r4, [sp, #84]                   @ 4-byte Reload
	umull	r1, r6, r0, r2
	str	r6, [sp, #20]                   @ 4-byte Spill
	adcs	lr, r3, r1
	ldr	r1, [sp, #100]                  @ 4-byte Reload
	umull	r3, r2, r0, r1
	ldr	r1, [sp, #92]                   @ 4-byte Reload
	str	r2, [sp, #16]                   @ 4-byte Spill
	adcs	r3, r5, r3
	umull	r5, r8, r0, r1
	ldr	r1, [sp, #88]                   @ 4-byte Reload
	ldr	r2, [sp, #24]                   @ 4-byte Reload
	adcs	r5, r7, r5
	umull	r7, r12, r0, r1
	adcs	r6, r11, r7
	umull	r7, r1, r0, r4
	ldr	r11, [sp, #76]                  @ 4-byte Reload
	adcs	r7, r10, r7
	umull	r4, r10, r0, r11
	ldr	r0, [sp, #32]                   @ 4-byte Reload
	adcs	r0, r0, r4
	ldr	r4, [sp, #28]                   @ 4-byte Reload
	adc	r4, r4, #0
	adds	r2, lr, r2
	str	r2, [sp, #32]                   @ 4-byte Spill
	ldr	r2, [sp, #20]                   @ 4-byte Reload
	adcs	r2, r3, r2
	str	r2, [sp, #28]                   @ 4-byte Spill
	ldr	r2, [sp, #16]                   @ 4-byte Reload
	ldr	r3, [sp, #60]                   @ 4-byte Reload
	adcs	r11, r5, r2
	adcs	r2, r6, r8
	str	r2, [sp, #24]                   @ 4-byte Spill
	adcs	r2, r7, r12
	ldr	r7, [r9, #20]
	adcs	r0, r0, r1
	ldr	r1, [sp, #96]                   @ 4-byte Reload
	str	r0, [sp, #16]                   @ 4-byte Spill
	adc	r0, r4, r10
	str	r2, [sp, #20]                   @ 4-byte Spill
	str	r0, [sp, #12]                   @ 4-byte Spill
	umull	r10, r2, r7, r1
	ldr	r1, [sp, #80]                   @ 4-byte Reload
	ldr	r6, [sp, #52]                   @ 4-byte Reload
	umull	r4, r0, r7, r3
	ldr	r8, [sp, #76]                   @ 4-byte Reload
	adds	r4, r2, r4
	umull	r5, r4, r7, r1
	adcs	r0, r0, r5
	umlal	r2, r5, r7, r3
	ldr	r3, [sp, #72]                   @ 4-byte Reload
	umull	r0, r1, r7, r6
	ldr	r6, [sp, #68]                   @ 4-byte Reload
	adcs	lr, r4, r0
	umull	r4, r0, r7, r6
	ldr	r6, [sp, #48]                   @ 4-byte Reload
	adcs	r12, r1, r4
	umull	r4, r1, r7, r6
	adcs	r9, r0, r4
	umull	r4, r0, r7, r3
	ldr	r3, [sp, #32]                   @ 4-byte Reload
	adcs	r1, r1, r4
	adc	r0, r0, #0
	adds	r4, r10, r3
	ldr	r3, [sp, #28]                   @ 4-byte Reload
	adcs	r2, r2, r3
	ldr	r3, [sp, #24]                   @ 4-byte Reload
	adcs	r5, r5, r11
	adcs	r7, lr, r3
	ldr	r3, [sp, #20]                   @ 4-byte Reload
	adcs	r11, r12, r3
	ldr	r3, [sp, #16]                   @ 4-byte Reload
	adcs	r9, r9, r3
	ldr	r3, [sp, #12]                   @ 4-byte Reload
	adcs	r1, r1, r3
	str	r1, [sp, #32]                   @ 4-byte Spill
	ldr	r1, [sp, #64]                   @ 4-byte Reload
	adc	r0, r0, #0
	str	r0, [sp, #28]                   @ 4-byte Spill
	ldr	r3, [sp, #44]                   @ 4-byte Reload
	mul	r0, r1, r4
	umull	r1, r6, r0, r3
	ldr	r3, [sp, #56]                   @ 4-byte Reload
	str	r6, [sp, #24]                   @ 4-byte Spill
	adds	r1, r4, r1
	ldr	r4, [sp, #84]                   @ 4-byte Reload
	umull	r1, r6, r0, r3
	ldr	r3, [sp, #100]                  @ 4-byte Reload
	str	r6, [sp, #20]                   @ 4-byte Spill
	adcs	r12, r2, r1
	umull	r2, r10, r0, r3
	ldr	r3, [sp, #92]                   @ 4-byte Reload
	ldr	r1, [sp, #24]                   @ 4-byte Reload
	adcs	r2, r5, r2
	umull	r5, lr, r0, r3
	ldr	r3, [sp, #88]                   @ 4-byte Reload
	adcs	r5, r7, r5
	umull	r7, r6, r0, r3
	adcs	r7, r11, r7
	umull	r3, r11, r0, r4
	adcs	r3, r9, r3
	umull	r4, r9, r0, r8
	ldr	r0, [sp, #32]                   @ 4-byte Reload
	adcs	r0, r0, r4
	ldr	r4, [sp, #28]                   @ 4-byte Reload
	adc	r4, r4, #0
	adds	r8, r12, r1
	ldr	r1, [sp, #20]                   @ 4-byte Reload
	adcs	r1, r2, r1
	str	r1, [sp, #16]                   @ 4-byte Spill
	adcs	r1, r5, r10
	str	r1, [sp, #32]                   @ 4-byte Spill
	adcs	r1, r7, lr
	str	r1, [sp, #28]                   @ 4-byte Spill
	adcs	r1, r3, r6
	ldr	r5, [sp, #60]                   @ 4-byte Reload
	adcs	r0, r0, r11
	str	r0, [sp, #20]                   @ 4-byte Spill
	ldr	r0, [sp, #40]                   @ 4-byte Reload
	adc	r9, r4, r9
	str	r1, [sp, #24]                   @ 4-byte Spill
	ldr	r7, [sp, #68]                   @ 4-byte Reload
	ldr	r4, [r0, #24]
	ldr	r0, [sp, #80]                   @ 4-byte Reload
	ldr	r10, [sp, #44]                  @ 4-byte Reload
	umull	r12, r1, r4, r5
	umull	r6, lr, r4, r0
	ldr	r0, [sp, #96]                   @ 4-byte Reload
	umull	r11, r2, r4, r0
	mov	r0, r6
	mov	r3, r2
	adds	r2, r2, r12
	adcs	r1, r1, r6
	ldr	r6, [sp, #52]                   @ 4-byte Reload
	umlal	r3, r0, r4, r5
	umull	r1, r2, r4, r6
	adcs	r5, lr, r1
	umull	r6, r1, r4, r7
	ldr	r7, [sp, #48]                   @ 4-byte Reload
	adcs	lr, r2, r6
	umull	r6, r2, r4, r7
	ldr	r7, [sp, #72]                   @ 4-byte Reload
	adcs	r12, r1, r6
	umull	r6, r1, r4, r7
	adcs	r2, r2, r6
	ldr	r6, [sp, #16]                   @ 4-byte Reload
	adc	r1, r1, #0
	adds	r4, r11, r8
	adcs	r6, r3, r6
	ldr	r3, [sp, #32]                   @ 4-byte Reload
	ldr	r8, [sp, #84]                   @ 4-byte Reload
	adcs	r0, r0, r3
	ldr	r3, [sp, #28]                   @ 4-byte Reload
	adcs	r7, r5, r3
	ldr	r3, [sp, #24]                   @ 4-byte Reload
	adcs	lr, lr, r3
	ldr	r3, [sp, #20]                   @ 4-byte Reload
	adcs	r11, r12, r3
	ldr	r12, [sp, #56]                  @ 4-byte Reload
	adcs	r2, r2, r9
	str	r2, [sp, #96]                   @ 4-byte Spill
	ldr	r2, [sp, #64]                   @ 4-byte Reload
	adc	r1, r1, #0
	str	r1, [sp, #80]                   @ 4-byte Spill
	ldr	r9, [sp, #88]                   @ 4-byte Reload
	mul	r1, r2, r4
	umull	r2, r3, r1, r10
	str	r3, [sp, #72]                   @ 4-byte Spill
	ldr	r3, [sp, #100]                  @ 4-byte Reload
	adds	r2, r4, r2
	umull	r2, r5, r1, r3
	umull	r4, r3, r1, r12
	str	r5, [sp, #68]                   @ 4-byte Spill
	ldr	r5, [sp, #92]                   @ 4-byte Reload
	str	r3, [sp, #64]                   @ 4-byte Spill
	adcs	r3, r6, r4
	str	r3, [sp, #48]                   @ 4-byte Spill
	adcs	r0, r0, r2
	str	r0, [sp, #40]                   @ 4-byte Spill
	umull	r0, r2, r1, r5
	str	r2, [sp, #60]                   @ 4-byte Spill
	adcs	r0, r7, r0
	str	r0, [sp, #28]                   @ 4-byte Spill
	umull	r0, r2, r1, r9
	ldr	r7, [sp, #76]                   @ 4-byte Reload
	str	r2, [sp, #52]                   @ 4-byte Spill
	adcs	r3, lr, r0
	umull	r6, r0, r1, r8
	ldr	r2, [sp, #48]                   @ 4-byte Reload
	str	r0, [sp, #32]                   @ 4-byte Spill
	umull	r4, r0, r1, r7
	adcs	r11, r11, r6
	ldr	r1, [sp, #28]                   @ 4-byte Reload
	str	r0, [sp, #24]                   @ 4-byte Spill
	ldr	r0, [sp, #96]                   @ 4-byte Reload
	adcs	lr, r0, r4
	ldr	r0, [sp, #80]                   @ 4-byte Reload
	ldr	r4, [sp, #40]                   @ 4-byte Reload
	adc	r0, r0, #0
	str	r0, [sp, #20]                   @ 4-byte Spill
	ldr	r0, [sp, #72]                   @ 4-byte Reload
	adds	r0, r2, r0
	ldr	r2, [sp, #64]                   @ 4-byte Reload
	str	r0, [sp, #96]                   @ 4-byte Spill
	adcs	r2, r4, r2
	ldr	r4, [sp, #68]                   @ 4-byte Reload
	str	r2, [sp, #80]                   @ 4-byte Spill
	adcs	r6, r1, r4
	ldr	r4, [sp, #60]                   @ 4-byte Reload
	str	r6, [sp, #72]                   @ 4-byte Spill
	adcs	r3, r3, r4
	ldr	r4, [sp, #52]                   @ 4-byte Reload
	str	r3, [sp, #68]                   @ 4-byte Spill
	adcs	r1, r11, r4
	str	r1, [sp, #64]                   @ 4-byte Spill
	ldr	r1, [sp, #32]                   @ 4-byte Reload
	ldr	r4, [sp, #20]                   @ 4-byte Reload
	adcs	r11, lr, r1
	ldr	r1, [sp, #24]                   @ 4-byte Reload
	adc	r1, r4, r1
	subs	r10, r0, r10
	ldr	r0, [sp, #100]                  @ 4-byte Reload
	sbcs	lr, r2, r12
	sbcs	r12, r6, r0
	sbcs	r5, r3, r5
	ldr	r3, [sp, #64]                   @ 4-byte Reload
	sbcs	r0, r3, r9
	sbcs	r4, r11, r8
	sbc	r6, r1, r7
	asr	r2, r6, #31
	cmn	r2, #1
	movgt	r1, r6
	ldr	r6, [sp, #36]                   @ 4-byte Reload
	movle	r0, r3
	movle	r4, r11
	cmn	r2, #1
	str	r0, [r6, #16]
	ldr	r0, [sp, #68]                   @ 4-byte Reload
	str	r1, [r6, #24]
	str	r4, [r6, #20]
	movle	r5, r0
	ldr	r0, [sp, #72]                   @ 4-byte Reload
	str	r5, [r6, #12]
	movle	r12, r0
	ldr	r0, [sp, #80]                   @ 4-byte Reload
	str	r12, [r6, #8]
	movle	lr, r0
	ldr	r0, [sp, #96]                   @ 4-byte Reload
	cmn	r2, #1
	str	lr, [r6, #4]
	movle	r10, r0
	str	r10, [r6]
	add	sp, sp, #104
	pop	{r4, r5, r6, r7, r8, r9, r10, r11, lr}
	mov	pc, lr
.Lfunc_end27:
	.size	mcl_fp_montNF7L, .Lfunc_end27-mcl_fp_montNF7L
	.cantunwind
	.fnend
                                        @ -- End function
	.globl	mcl_fp_montRed7L                @ -- Begin function mcl_fp_montRed7L
	.p2align	2
	.type	mcl_fp_montRed7L,%function
	.code	32                              @ @mcl_fp_montRed7L
mcl_fp_montRed7L:
	.fnstart
@ %bb.0:
	.save	{r4, r5, r6, r7, r8, r9, r10, r11, lr}
	push	{r4, r5, r6, r7, r8, r9, r10, r11, lr}
	.pad	#84
	sub	sp, sp, #84
	mov	r9, r1
	str	r0, [sp, #48]                   @ 4-byte Spill
	ldr	r3, [r2, #-4]
	ldr	r0, [r9, #4]
	ldr	lr, [r9]
	str	r0, [sp, #44]                   @ 4-byte Spill
	ldr	r0, [r9, #8]
	str	r0, [sp, #40]                   @ 4-byte Spill
	ldr	r0, [r9, #12]
	str	r0, [sp, #36]                   @ 4-byte Spill
	mul	r0, lr, r3
	ldr	r8, [r2, #4]
	ldr	r1, [r2]
	str	r3, [sp, #60]                   @ 4-byte Spill
	ldr	r4, [r2, #8]
	str	r1, [sp, #80]                   @ 4-byte Spill
	umull	r7, r3, r0, r1
	str	r4, [sp, #64]                   @ 4-byte Spill
	mov	r10, r4
	str	r8, [sp, #52]                   @ 4-byte Spill
	umull	r5, r6, r0, r8
	str	r7, [sp, #32]                   @ 4-byte Spill
	adds	r5, r3, r5
	umull	r1, r5, r0, r4
	ldr	r4, [r2, #12]
	str	r4, [sp, #56]                   @ 4-byte Spill
	adcs	r6, r6, r1
	umlal	r3, r1, r0, r8
	umull	r6, r12, r0, r4
	adcs	r4, r5, r6
	ldr	r5, [r2, #16]
	str	r4, [sp, #28]                   @ 4-byte Spill
	str	r5, [sp, #76]                   @ 4-byte Spill
	umull	r6, r4, r0, r5
	adcs	r7, r12, r6
	ldr	r6, [r2, #20]
	str	r6, [sp, #72]                   @ 4-byte Spill
	str	r7, [sp, #24]                   @ 4-byte Spill
	umull	r11, r5, r0, r6
	adcs	r6, r4, r11
	ldr	r4, [r2, #24]
	str	r4, [sp, #68]                   @ 4-byte Spill
	umull	r2, r12, r0, r4
	ldr	r0, [sp, #44]                   @ 4-byte Reload
	adcs	r2, r5, r2
	ldr	r5, [sp, #32]                   @ 4-byte Reload
	adc	r4, r12, #0
	ldr	r12, [sp, #76]                  @ 4-byte Reload
	adds	r7, r5, lr
	ldr	lr, [sp, #68]                   @ 4-byte Reload
	adcs	r3, r3, r0
	ldr	r0, [sp, #40]                   @ 4-byte Reload
	str	r3, [sp, #16]                   @ 4-byte Spill
	mov	r7, r8
	adcs	r0, r1, r0
	str	r0, [sp, #44]                   @ 4-byte Spill
	ldr	r0, [sp, #36]                   @ 4-byte Reload
	ldr	r1, [sp, #28]                   @ 4-byte Reload
	adcs	r0, r1, r0
	str	r0, [sp, #40]                   @ 4-byte Spill
	ldr	r0, [r9, #16]
	ldr	r1, [sp, #24]                   @ 4-byte Reload
	adcs	r0, r1, r0
	str	r0, [sp, #36]                   @ 4-byte Spill
	ldr	r0, [r9, #20]
	adcs	r0, r6, r0
	str	r0, [sp, #32]                   @ 4-byte Spill
	ldr	r0, [r9, #24]
	adcs	r0, r2, r0
	str	r0, [sp, #28]                   @ 4-byte Spill
	ldr	r0, [r9, #28]
	adcs	r0, r4, r0
	str	r0, [sp, #24]                   @ 4-byte Spill
	mov	r0, #0
	ldr	r4, [sp, #80]                   @ 4-byte Reload
	adc	r0, r0, #0
	str	r0, [sp, #20]                   @ 4-byte Spill
	ldr	r0, [sp, #60]                   @ 4-byte Reload
	mul	r1, r0, r3
	umull	r2, r3, r1, r4
	str	r2, [sp, #12]                   @ 4-byte Spill
	umull	r8, r2, r1, r7
	umull	r6, r7, r1, r10
	adds	r3, r8, r3
	str	r3, [sp, #8]                    @ 4-byte Spill
	ldr	r3, [sp, #56]                   @ 4-byte Reload
	adcs	r2, r6, r2
	str	r2, [sp, #4]                    @ 4-byte Spill
	umull	r6, r2, r1, r3
	adcs	r7, r6, r7
	umull	r6, r8, r1, r12
	str	r7, [sp]                        @ 4-byte Spill
	ldr	r7, [sp, #72]                   @ 4-byte Reload
	adcs	r11, r6, r2
	umull	r6, r5, r1, r7
	adcs	r10, r6, r8
	umull	r6, r2, r1, lr
	adcs	r1, r6, r5
	ldr	r5, [sp, #20]                   @ 4-byte Reload
	ldr	r6, [sp, #12]                   @ 4-byte Reload
	adc	r5, r5, r2
	ldr	r2, [sp, #16]                   @ 4-byte Reload
	adds	r2, r6, r2
	ldr	r6, [sp, #8]                    @ 4-byte Reload
	ldr	r2, [sp, #44]                   @ 4-byte Reload
	adcs	r8, r6, r2
	ldr	r2, [sp, #40]                   @ 4-byte Reload
	ldr	r6, [sp, #4]                    @ 4-byte Reload
	adcs	r2, r6, r2
	str	r2, [sp, #44]                   @ 4-byte Spill
	ldr	r2, [sp, #36]                   @ 4-byte Reload
	ldr	r6, [sp]                        @ 4-byte Reload
	adcs	r2, r6, r2
	str	r2, [sp, #40]                   @ 4-byte Spill
	ldr	r2, [sp, #32]                   @ 4-byte Reload
	ldr	r6, [r9, #32]
	adcs	r2, r11, r2
	str	r2, [sp, #36]                   @ 4-byte Spill
	ldr	r2, [sp, #28]                   @ 4-byte Reload
	mov	r11, r3
	adcs	r2, r10, r2
	str	r2, [sp, #32]                   @ 4-byte Spill
	ldr	r2, [sp, #24]                   @ 4-byte Reload
	ldr	r10, [sp, #64]                  @ 4-byte Reload
	adcs	r1, r1, r2
	str	r1, [sp, #28]                   @ 4-byte Spill
	adcs	r1, r5, r6
	str	r1, [sp, #24]                   @ 4-byte Spill
	mov	r1, #0
	adc	r1, r1, #0
	str	r1, [sp, #20]                   @ 4-byte Spill
	mul	r1, r0, r8
	umull	r0, r2, r1, r4
	str	r0, [sp, #16]                   @ 4-byte Spill
	ldr	r0, [sp, #52]                   @ 4-byte Reload
	umull	r5, r6, r1, r0
	adds	r2, r5, r2
	str	r2, [sp, #12]                   @ 4-byte Spill
	umull	r5, r2, r1, r10
	adcs	r4, r5, r6
	str	r4, [sp, #8]                    @ 4-byte Spill
	umull	r5, r4, r1, r3
	ldr	r6, [sp, #8]                    @ 4-byte Reload
	adcs	r2, r5, r2
	str	r2, [sp, #4]                    @ 4-byte Spill
	umull	r5, r2, r1, r12
	adcs	r4, r5, r4
	umull	r5, r12, r1, r7
	ldr	r7, [sp, #20]                   @ 4-byte Reload
	adcs	r2, r5, r2
	umull	r5, r3, r1, lr
	adcs	r1, r5, r12
	ldr	r12, [sp, #76]                  @ 4-byte Reload
	adc	r5, r7, r3
	ldr	r3, [sp, #16]                   @ 4-byte Reload
	ldr	r7, [sp, #12]                   @ 4-byte Reload
	adds	r3, r3, r8
	ldr	r8, [sp, #60]                   @ 4-byte Reload
	ldr	r3, [sp, #44]                   @ 4-byte Reload
	adcs	r7, r7, r3
	ldr	r3, [sp, #40]                   @ 4-byte Reload
	adcs	r3, r6, r3
	str	r3, [sp, #44]                   @ 4-byte Spill
	ldr	r3, [sp, #36]                   @ 4-byte Reload
	ldr	r6, [sp, #4]                    @ 4-byte Reload
	adcs	r3, r6, r3
	str	r3, [sp, #40]                   @ 4-byte Spill
	ldr	r3, [sp, #32]                   @ 4-byte Reload
	adcs	r3, r4, r3
	str	r3, [sp, #36]                   @ 4-byte Spill
	ldr	r3, [sp, #28]                   @ 4-byte Reload
	ldr	r4, [r9, #36]
	adcs	r2, r2, r3
	str	r2, [sp, #32]                   @ 4-byte Spill
	ldr	r2, [sp, #24]                   @ 4-byte Reload
	adcs	r1, r1, r2
	str	r1, [sp, #28]                   @ 4-byte Spill
	adcs	r1, r5, r4
	str	r1, [sp, #24]                   @ 4-byte Spill
	mov	r1, #0
	ldr	r2, [sp, #80]                   @ 4-byte Reload
	adc	r1, r1, #0
	str	r1, [sp, #20]                   @ 4-byte Spill
	mul	r1, r8, r7
	umull	r3, r5, r1, r2
	str	r3, [sp, #16]                   @ 4-byte Spill
	umull	r6, r3, r1, r0
	adds	r0, r6, r5
	umull	r6, r5, r1, r10
	str	r0, [sp, #12]                   @ 4-byte Spill
	adcs	r10, r6, r3
	umull	r6, r3, r1, r11
	adcs	r0, r6, r5
	umull	r6, lr, r1, r12
	str	r0, [sp, #8]                    @ 4-byte Spill
	ldr	r0, [sp, #72]                   @ 4-byte Reload
	ldr	r5, [sp, #68]                   @ 4-byte Reload
	adcs	r11, r6, r3
	umull	r6, r3, r1, r0
	adcs	r4, r6, lr
	umull	r6, r0, r1, r5
	adcs	r1, r6, r3
	ldr	r3, [sp, #20]                   @ 4-byte Reload
	ldr	r6, [sp, #8]                    @ 4-byte Reload
	adc	r0, r3, r0
	ldr	r3, [sp, #16]                   @ 4-byte Reload
	adds	r3, r3, r7
	ldr	r7, [sp, #12]                   @ 4-byte Reload
	ldr	r3, [sp, #44]                   @ 4-byte Reload
	adcs	r7, r7, r3
	ldr	r3, [sp, #40]                   @ 4-byte Reload
	adcs	r3, r10, r3
	str	r3, [sp, #44]                   @ 4-byte Spill
	ldr	r3, [sp, #36]                   @ 4-byte Reload
	mov	r10, r8
	adcs	r3, r6, r3
	str	r3, [sp, #40]                   @ 4-byte Spill
	ldr	r3, [sp, #32]                   @ 4-byte Reload
	adcs	r3, r11, r3
	str	r3, [sp, #36]                   @ 4-byte Spill
	ldr	r3, [sp, #28]                   @ 4-byte Reload
	ldr	r11, [sp, #52]                  @ 4-byte Reload
	adcs	r3, r4, r3
	str	r3, [sp, #32]                   @ 4-byte Spill
	ldr	r3, [sp, #24]                   @ 4-byte Reload
	ldr	r4, [r9, #40]
	adcs	r1, r1, r3
	str	r1, [sp, #28]                   @ 4-byte Spill
	mul	r1, r8, r7
	adcs	r0, r0, r4
	str	r0, [sp, #24]                   @ 4-byte Spill
	mov	r0, #0
	adc	r0, r0, #0
	str	r0, [sp, #20]                   @ 4-byte Spill
	umull	r0, r3, r1, r2
	str	r0, [sp, #16]                   @ 4-byte Spill
	umull	r6, r0, r1, r11
	adds	r8, r6, r3
	ldr	r3, [sp, #64]                   @ 4-byte Reload
	umull	r6, r2, r1, r3
	adcs	r0, r6, r0
	str	r0, [sp, #12]                   @ 4-byte Spill
	ldr	r0, [sp, #56]                   @ 4-byte Reload
	umull	r6, r3, r1, r0
	adcs	r2, r6, r2
	str	r2, [sp, #8]                    @ 4-byte Spill
	umull	r6, r2, r1, r12
	ldr	r12, [sp, #72]                  @ 4-byte Reload
	adcs	lr, r6, r3
	umull	r6, r3, r1, r12
	adcs	r4, r6, r2
	umull	r6, r2, r1, r5
	ldr	r5, [sp, #12]                   @ 4-byte Reload
	adcs	r1, r6, r3
	ldr	r3, [sp, #20]                   @ 4-byte Reload
	adc	r2, r3, r2
	ldr	r3, [sp, #16]                   @ 4-byte Reload
	adds	r3, r3, r7
	ldr	r3, [sp, #44]                   @ 4-byte Reload
	adcs	r8, r8, r3
	ldr	r3, [sp, #40]                   @ 4-byte Reload
	adcs	r3, r5, r3
	str	r3, [sp, #44]                   @ 4-byte Spill
	ldr	r3, [sp, #36]                   @ 4-byte Reload
	ldr	r5, [sp, #8]                    @ 4-byte Reload
	adcs	r3, r5, r3
	str	r3, [sp, #40]                   @ 4-byte Spill
	ldr	r3, [sp, #32]                   @ 4-byte Reload
	adcs	r3, lr, r3
	str	r3, [sp, #36]                   @ 4-byte Spill
	ldr	r3, [sp, #28]                   @ 4-byte Reload
	ldr	lr, [sp, #80]                   @ 4-byte Reload
	adcs	r3, r4, r3
	str	r3, [sp, #32]                   @ 4-byte Spill
	ldr	r3, [sp, #24]                   @ 4-byte Reload
	ldr	r4, [r9, #44]
	adcs	r1, r1, r3
	str	r1, [sp, #28]                   @ 4-byte Spill
	adcs	r1, r2, r4
	str	r1, [sp, #24]                   @ 4-byte Spill
	mov	r1, #0
	adc	r1, r1, #0
	str	r1, [sp, #20]                   @ 4-byte Spill
	mul	r1, r10, r8
	umull	r2, r5, r1, lr
	umull	r6, r3, r1, r11
	str	r2, [sp, #16]                   @ 4-byte Spill
	mov	r2, r11
	adds	r4, r6, r5
	ldr	r5, [sp, #64]                   @ 4-byte Reload
	str	r4, [sp, #12]                   @ 4-byte Spill
	umull	r6, r4, r1, r5
	adcs	r3, r6, r3
	str	r3, [sp, #8]                    @ 4-byte Spill
	umull	r6, r3, r1, r0
	adcs	r0, r6, r4
	str	r0, [sp, #4]                    @ 4-byte Spill
	ldr	r0, [sp, #76]                   @ 4-byte Reload
	umull	r6, r4, r1, r0
	adcs	r11, r6, r3
	umull	r6, r7, r1, r12
	ldr	r12, [sp, #68]                  @ 4-byte Reload
	adcs	r10, r6, r4
	umull	r6, r3, r1, r12
	ldr	r4, [sp, #8]                    @ 4-byte Reload
	adcs	r1, r6, r7
	ldr	r6, [sp, #20]                   @ 4-byte Reload
	ldr	r7, [sp, #12]                   @ 4-byte Reload
	adc	r6, r6, r3
	ldr	r3, [sp, #16]                   @ 4-byte Reload
	adds	r3, r3, r8
	ldr	r3, [sp, #44]                   @ 4-byte Reload
	adcs	r7, r7, r3
	ldr	r3, [sp, #40]                   @ 4-byte Reload
	adcs	r3, r4, r3
	str	r3, [sp, #44]                   @ 4-byte Spill
	ldr	r3, [sp, #36]                   @ 4-byte Reload
	ldr	r4, [sp, #4]                    @ 4-byte Reload
	adcs	r3, r4, r3
	str	r3, [sp, #40]                   @ 4-byte Spill
	ldr	r3, [sp, #32]                   @ 4-byte Reload
	ldr	r4, [r9, #48]
	adcs	r3, r11, r3
	str	r3, [sp, #36]                   @ 4-byte Spill
	ldr	r3, [sp, #28]                   @ 4-byte Reload
	adcs	r3, r10, r3
	str	r3, [sp, #32]                   @ 4-byte Spill
	ldr	r3, [sp, #24]                   @ 4-byte Reload
	ldr	r10, [sp, #72]                  @ 4-byte Reload
	adcs	r1, r1, r3
	str	r1, [sp, #28]                   @ 4-byte Spill
	adcs	r1, r6, r4
	ldr	r3, [sp, #60]                   @ 4-byte Reload
	str	r1, [sp, #24]                   @ 4-byte Spill
	mov	r1, #0
	adc	r1, r1, #0
	str	r1, [sp, #20]                   @ 4-byte Spill
	mul	r1, r3, r7
	umull	r4, r3, r1, lr
	str	r4, [sp, #60]                   @ 4-byte Spill
	umull	r6, r4, r1, r2
	adds	r11, r6, r3
	umull	r6, r2, r1, r5
	ldr	r5, [sp, #56]                   @ 4-byte Reload
	adcs	r8, r6, r4
	umull	r6, r3, r1, r5
	adcs	lr, r6, r2
	umull	r6, r2, r1, r0
	adcs	r4, r6, r3
	umull	r6, r3, r1, r10
	adcs	r2, r6, r2
	umull	r6, r0, r1, r12
	ldr	r1, [sp, #20]                   @ 4-byte Reload
	adcs	r12, r6, r3
	adc	r6, r1, r0
	ldr	r0, [sp, #60]                   @ 4-byte Reload
	ldr	r1, [sp, #40]                   @ 4-byte Reload
	adds	r0, r0, r7
	ldr	r0, [sp, #44]                   @ 4-byte Reload
	adcs	r0, r11, r0
	str	r0, [sp, #60]                   @ 4-byte Spill
	adcs	r7, r8, r1
	ldr	r1, [sp, #36]                   @ 4-byte Reload
	str	r7, [sp, #44]                   @ 4-byte Spill
	adcs	r11, lr, r1
	ldr	r1, [sp, #32]                   @ 4-byte Reload
	str	r11, [sp, #40]                  @ 4-byte Spill
	adcs	r1, r4, r1
	ldr	r4, [sp, #28]                   @ 4-byte Reload
	adcs	r3, r2, r4
	ldr	r2, [sp, #24]                   @ 4-byte Reload
	ldr	r4, [r9, #52]
	adcs	r8, r12, r2
	ldr	r2, [sp, #80]                   @ 4-byte Reload
	adc	r6, r6, r4
	subs	r9, r0, r2
	ldr	r0, [sp, #52]                   @ 4-byte Reload
	ldr	r2, [sp, #76]                   @ 4-byte Reload
	sbcs	lr, r7, r0
	ldr	r0, [sp, #64]                   @ 4-byte Reload
	sbcs	r0, r11, r0
	sbcs	r7, r1, r5
	sbcs	r5, r3, r2
	ldr	r2, [sp, #68]                   @ 4-byte Reload
	sbcs	r4, r8, r10
	sbcs	r11, r6, r2
	mov	r2, #0
	sbc	r12, r2, #0
	ands	r12, r12, #1
	movne	r11, r6
	movne	r4, r8
	movne	r5, r3
	cmp	r12, #0
	movne	r7, r1
	ldr	r1, [sp, #40]                   @ 4-byte Reload
	ldr	r6, [sp, #48]                   @ 4-byte Reload
	movne	r0, r1
	str	r0, [r6, #8]
	ldr	r0, [sp, #44]                   @ 4-byte Reload
	str	r11, [r6, #24]
	str	r4, [r6, #20]
	movne	lr, r0
	ldr	r0, [sp, #60]                   @ 4-byte Reload
	cmp	r12, #0
	str	r5, [r6, #16]
	str	r7, [r6, #12]
	movne	r9, r0
	str	lr, [r6, #4]
	str	r9, [r6]
	add	sp, sp, #84
	pop	{r4, r5, r6, r7, r8, r9, r10, r11, lr}
	mov	pc, lr
.Lfunc_end28:
	.size	mcl_fp_montRed7L, .Lfunc_end28-mcl_fp_montRed7L
	.cantunwind
	.fnend
                                        @ -- End function
	.globl	mcl_fp_montRedNF7L              @ -- Begin function mcl_fp_montRedNF7L
	.p2align	2
	.type	mcl_fp_montRedNF7L,%function
	.code	32                              @ @mcl_fp_montRedNF7L
mcl_fp_montRedNF7L:
	.fnstart
@ %bb.0:
	.save	{r4, r5, r6, r7, r8, r9, r10, r11, lr}
	push	{r4, r5, r6, r7, r8, r9, r10, r11, lr}
	.pad	#84
	sub	sp, sp, #84
	mov	r9, r1
	str	r0, [sp, #48]                   @ 4-byte Spill
	ldr	r3, [r2, #-4]
	ldr	r0, [r9, #4]
	ldr	lr, [r9]
	str	r0, [sp, #44]                   @ 4-byte Spill
	ldr	r0, [r9, #8]
	str	r0, [sp, #40]                   @ 4-byte Spill
	ldr	r0, [r9, #12]
	str	r0, [sp, #36]                   @ 4-byte Spill
	mul	r0, lr, r3
	ldr	r8, [r2, #4]
	ldr	r1, [r2]
	str	r3, [sp, #60]                   @ 4-byte Spill
	ldr	r4, [r2, #8]
	str	r1, [sp, #80]                   @ 4-byte Spill
	umull	r7, r3, r0, r1
	str	r4, [sp, #64]                   @ 4-byte Spill
	mov	r10, r4
	str	r8, [sp, #52]                   @ 4-byte Spill
	umull	r5, r6, r0, r8
	str	r7, [sp, #32]                   @ 4-byte Spill
	adds	r5, r3, r5
	umull	r1, r5, r0, r4
	ldr	r4, [r2, #12]
	str	r4, [sp, #56]                   @ 4-byte Spill
	adcs	r6, r6, r1
	umlal	r3, r1, r0, r8
	umull	r6, r12, r0, r4
	adcs	r4, r5, r6
	ldr	r5, [r2, #16]
	str	r4, [sp, #28]                   @ 4-byte Spill
	str	r5, [sp, #76]                   @ 4-byte Spill
	umull	r6, r4, r0, r5
	adcs	r7, r12, r6
	ldr	r6, [r2, #20]
	str	r6, [sp, #72]                   @ 4-byte Spill
	str	r7, [sp, #24]                   @ 4-byte Spill
	umull	r11, r5, r0, r6
	adcs	r6, r4, r11
	ldr	r4, [r2, #24]
	str	r4, [sp, #68]                   @ 4-byte Spill
	umull	r2, r12, r0, r4
	ldr	r0, [sp, #44]                   @ 4-byte Reload
	adcs	r2, r5, r2
	ldr	r5, [sp, #32]                   @ 4-byte Reload
	adc	r4, r12, #0
	ldr	r12, [sp, #76]                  @ 4-byte Reload
	adds	r7, r5, lr
	ldr	lr, [sp, #68]                   @ 4-byte Reload
	adcs	r3, r3, r0
	ldr	r0, [sp, #40]                   @ 4-byte Reload
	str	r3, [sp, #16]                   @ 4-byte Spill
	mov	r7, r8
	adcs	r0, r1, r0
	str	r0, [sp, #44]                   @ 4-byte Spill
	ldr	r0, [sp, #36]                   @ 4-byte Reload
	ldr	r1, [sp, #28]                   @ 4-byte Reload
	adcs	r0, r1, r0
	str	r0, [sp, #40]                   @ 4-byte Spill
	ldr	r0, [r9, #16]
	ldr	r1, [sp, #24]                   @ 4-byte Reload
	adcs	r0, r1, r0
	str	r0, [sp, #36]                   @ 4-byte Spill
	ldr	r0, [r9, #20]
	adcs	r0, r6, r0
	str	r0, [sp, #32]                   @ 4-byte Spill
	ldr	r0, [r9, #24]
	adcs	r0, r2, r0
	str	r0, [sp, #28]                   @ 4-byte Spill
	ldr	r0, [r9, #28]
	adcs	r0, r4, r0
	str	r0, [sp, #24]                   @ 4-byte Spill
	mov	r0, #0
	ldr	r4, [sp, #80]                   @ 4-byte Reload
	adc	r0, r0, #0
	str	r0, [sp, #20]                   @ 4-byte Spill
	ldr	r0, [sp, #60]                   @ 4-byte Reload
	mul	r1, r0, r3
	umull	r2, r3, r1, r4
	str	r2, [sp, #12]                   @ 4-byte Spill
	umull	r8, r2, r1, r7
	umull	r6, r7, r1, r10
	adds	r3, r8, r3
	str	r3, [sp, #8]                    @ 4-byte Spill
	ldr	r3, [sp, #56]                   @ 4-byte Reload
	adcs	r2, r6, r2
	str	r2, [sp, #4]                    @ 4-byte Spill
	umull	r6, r2, r1, r3
	adcs	r7, r6, r7
	umull	r6, r8, r1, r12
	str	r7, [sp]                        @ 4-byte Spill
	ldr	r7, [sp, #72]                   @ 4-byte Reload
	adcs	r11, r6, r2
	umull	r6, r5, r1, r7
	adcs	r10, r6, r8
	umull	r6, r2, r1, lr
	adcs	r1, r6, r5
	ldr	r5, [sp, #20]                   @ 4-byte Reload
	ldr	r6, [sp, #12]                   @ 4-byte Reload
	adc	r5, r5, r2
	ldr	r2, [sp, #16]                   @ 4-byte Reload
	adds	r2, r6, r2
	ldr	r6, [sp, #8]                    @ 4-byte Reload
	ldr	r2, [sp, #44]                   @ 4-byte Reload
	adcs	r8, r6, r2
	ldr	r2, [sp, #40]                   @ 4-byte Reload
	ldr	r6, [sp, #4]                    @ 4-byte Reload
	adcs	r2, r6, r2
	str	r2, [sp, #44]                   @ 4-byte Spill
	ldr	r2, [sp, #36]                   @ 4-byte Reload
	ldr	r6, [sp]                        @ 4-byte Reload
	adcs	r2, r6, r2
	str	r2, [sp, #40]                   @ 4-byte Spill
	ldr	r2, [sp, #32]                   @ 4-byte Reload
	ldr	r6, [r9, #32]
	adcs	r2, r11, r2
	str	r2, [sp, #36]                   @ 4-byte Spill
	ldr	r2, [sp, #28]                   @ 4-byte Reload
	mov	r11, r3
	adcs	r2, r10, r2
	str	r2, [sp, #32]                   @ 4-byte Spill
	ldr	r2, [sp, #24]                   @ 4-byte Reload
	ldr	r10, [sp, #64]                  @ 4-byte Reload
	adcs	r1, r1, r2
	str	r1, [sp, #28]                   @ 4-byte Spill
	adcs	r1, r5, r6
	str	r1, [sp, #24]                   @ 4-byte Spill
	mov	r1, #0
	adc	r1, r1, #0
	str	r1, [sp, #20]                   @ 4-byte Spill
	mul	r1, r0, r8
	umull	r0, r2, r1, r4
	str	r0, [sp, #16]                   @ 4-byte Spill
	ldr	r0, [sp, #52]                   @ 4-byte Reload
	umull	r5, r6, r1, r0
	adds	r2, r5, r2
	str	r2, [sp, #12]                   @ 4-byte Spill
	umull	r5, r2, r1, r10
	adcs	r4, r5, r6
	str	r4, [sp, #8]                    @ 4-byte Spill
	umull	r5, r4, r1, r3
	ldr	r6, [sp, #8]                    @ 4-byte Reload
	adcs	r2, r5, r2
	str	r2, [sp, #4]                    @ 4-byte Spill
	umull	r5, r2, r1, r12
	adcs	r4, r5, r4
	umull	r5, r12, r1, r7
	ldr	r7, [sp, #20]                   @ 4-byte Reload
	adcs	r2, r5, r2
	umull	r5, r3, r1, lr
	adcs	r1, r5, r12
	ldr	r12, [sp, #76]                  @ 4-byte Reload
	adc	r5, r7, r3
	ldr	r3, [sp, #16]                   @ 4-byte Reload
	ldr	r7, [sp, #12]                   @ 4-byte Reload
	adds	r3, r3, r8
	ldr	r8, [sp, #60]                   @ 4-byte Reload
	ldr	r3, [sp, #44]                   @ 4-byte Reload
	adcs	r7, r7, r3
	ldr	r3, [sp, #40]                   @ 4-byte Reload
	adcs	r3, r6, r3
	str	r3, [sp, #44]                   @ 4-byte Spill
	ldr	r3, [sp, #36]                   @ 4-byte Reload
	ldr	r6, [sp, #4]                    @ 4-byte Reload
	adcs	r3, r6, r3
	str	r3, [sp, #40]                   @ 4-byte Spill
	ldr	r3, [sp, #32]                   @ 4-byte Reload
	adcs	r3, r4, r3
	str	r3, [sp, #36]                   @ 4-byte Spill
	ldr	r3, [sp, #28]                   @ 4-byte Reload
	ldr	r4, [r9, #36]
	adcs	r2, r2, r3
	str	r2, [sp, #32]                   @ 4-byte Spill
	ldr	r2, [sp, #24]                   @ 4-byte Reload
	adcs	r1, r1, r2
	str	r1, [sp, #28]                   @ 4-byte Spill
	adcs	r1, r5, r4
	str	r1, [sp, #24]                   @ 4-byte Spill
	mov	r1, #0
	ldr	r2, [sp, #80]                   @ 4-byte Reload
	adc	r1, r1, #0
	str	r1, [sp, #20]                   @ 4-byte Spill
	mul	r1, r8, r7
	umull	r3, r5, r1, r2
	str	r3, [sp, #16]                   @ 4-byte Spill
	umull	r6, r3, r1, r0
	adds	r0, r6, r5
	umull	r6, r5, r1, r10
	str	r0, [sp, #12]                   @ 4-byte Spill
	adcs	r10, r6, r3
	umull	r6, r3, r1, r11
	ldr	r11, [sp, #72]                  @ 4-byte Reload
	adcs	r0, r6, r5
	umull	r6, lr, r1, r12
	str	r0, [sp, #8]                    @ 4-byte Spill
	adcs	r5, r6, r3
	umull	r6, r3, r1, r11
	adcs	r4, r6, lr
	ldr	lr, [sp, #68]                   @ 4-byte Reload
	umull	r6, r0, r1, lr
	adcs	r1, r6, r3
	ldr	r3, [sp, #20]                   @ 4-byte Reload
	ldr	r6, [sp, #12]                   @ 4-byte Reload
	adc	r0, r3, r0
	ldr	r3, [sp, #16]                   @ 4-byte Reload
	adds	r3, r3, r7
	ldr	r3, [sp, #44]                   @ 4-byte Reload
	adcs	r7, r6, r3
	ldr	r3, [sp, #40]                   @ 4-byte Reload
	ldr	r6, [sp, #8]                    @ 4-byte Reload
	adcs	r3, r10, r3
	str	r3, [sp, #44]                   @ 4-byte Spill
	ldr	r3, [sp, #36]                   @ 4-byte Reload
	ldr	r10, [sp, #52]                  @ 4-byte Reload
	adcs	r3, r6, r3
	str	r3, [sp, #40]                   @ 4-byte Spill
	ldr	r3, [sp, #32]                   @ 4-byte Reload
	adcs	r3, r5, r3
	str	r3, [sp, #36]                   @ 4-byte Spill
	ldr	r3, [sp, #28]                   @ 4-byte Reload
	adcs	r3, r4, r3
	str	r3, [sp, #32]                   @ 4-byte Spill
	ldr	r3, [sp, #24]                   @ 4-byte Reload
	ldr	r4, [r9, #40]
	adcs	r1, r1, r3
	str	r1, [sp, #28]                   @ 4-byte Spill
	mul	r1, r8, r7
	adcs	r0, r0, r4
	str	r0, [sp, #24]                   @ 4-byte Spill
	mov	r0, #0
	ldr	r3, [sp, #64]                   @ 4-byte Reload
	adc	r0, r0, #0
	str	r0, [sp, #20]                   @ 4-byte Spill
	umull	r0, r5, r1, r2
	str	r0, [sp, #16]                   @ 4-byte Spill
	umull	r6, r0, r1, r10
	adds	r2, r6, r5
	str	r2, [sp, #12]                   @ 4-byte Spill
	umull	r6, r2, r1, r3
	adcs	r0, r6, r0
	str	r0, [sp, #8]                    @ 4-byte Spill
	ldr	r0, [sp, #56]                   @ 4-byte Reload
	umull	r6, r3, r1, r0
	adcs	r2, r6, r2
	str	r2, [sp, #4]                    @ 4-byte Spill
	umull	r6, r2, r1, r12
	adcs	r5, r6, r3
	umull	r6, r3, r1, r11
	adcs	r4, r6, r2
	umull	r6, r2, r1, lr
	ldr	lr, [sp, #76]                   @ 4-byte Reload
	adcs	r1, r6, r3
	ldr	r3, [sp, #20]                   @ 4-byte Reload
	adc	r2, r3, r2
	ldr	r3, [sp, #16]                   @ 4-byte Reload
	adds	r3, r3, r7
	ldr	r7, [sp, #12]                   @ 4-byte Reload
	ldr	r3, [sp, #44]                   @ 4-byte Reload
	adcs	r12, r7, r3
	ldr	r3, [sp, #40]                   @ 4-byte Reload
	ldr	r7, [sp, #8]                    @ 4-byte Reload
	adcs	r3, r7, r3
	str	r3, [sp, #44]                   @ 4-byte Spill
	ldr	r3, [sp, #36]                   @ 4-byte Reload
	ldr	r7, [sp, #4]                    @ 4-byte Reload
	adcs	r3, r7, r3
	str	r3, [sp, #40]                   @ 4-byte Spill
	ldr	r3, [sp, #32]                   @ 4-byte Reload
	ldr	r7, [sp, #64]                   @ 4-byte Reload
	adcs	r3, r5, r3
	str	r3, [sp, #36]                   @ 4-byte Spill
	ldr	r3, [sp, #28]                   @ 4-byte Reload
	adcs	r3, r4, r3
	str	r3, [sp, #32]                   @ 4-byte Spill
	ldr	r3, [sp, #24]                   @ 4-byte Reload
	ldr	r4, [r9, #44]
	adcs	r1, r1, r3
	str	r1, [sp, #28]                   @ 4-byte Spill
	adcs	r1, r2, r4
	str	r1, [sp, #24]                   @ 4-byte Spill
	mov	r1, #0
	ldr	r2, [sp, #80]                   @ 4-byte Reload
	adc	r1, r1, #0
	str	r1, [sp, #20]                   @ 4-byte Spill
	mul	r1, r8, r12
	umull	r3, r5, r1, r2
	str	r3, [sp, #16]                   @ 4-byte Spill
	umull	r6, r3, r1, r10
	adds	r10, r6, r5
	umull	r6, r5, r1, r7
	adcs	r3, r6, r3
	str	r3, [sp, #12]                   @ 4-byte Spill
	umull	r6, r3, r1, r0
	adcs	r0, r6, r5
	umull	r6, r11, r1, lr
	str	r0, [sp, #8]                    @ 4-byte Spill
	ldr	r0, [sp, #72]                   @ 4-byte Reload
	ldr	r5, [sp, #68]                   @ 4-byte Reload
	adcs	r8, r6, r3
	umull	r6, r4, r1, r0
	adcs	r11, r6, r11
	umull	r6, r3, r1, r5
	adcs	r1, r6, r4
	ldr	r4, [sp, #20]                   @ 4-byte Reload
	adc	r6, r4, r3
	ldr	r3, [sp, #16]                   @ 4-byte Reload
	ldr	r4, [sp, #12]                   @ 4-byte Reload
	adds	r3, r3, r12
	ldr	r3, [sp, #44]                   @ 4-byte Reload
	adcs	r10, r10, r3
	ldr	r3, [sp, #40]                   @ 4-byte Reload
	adcs	r3, r4, r3
	str	r3, [sp, #44]                   @ 4-byte Spill
	ldr	r3, [sp, #36]                   @ 4-byte Reload
	ldr	r4, [sp, #8]                    @ 4-byte Reload
	adcs	r3, r4, r3
	str	r3, [sp, #40]                   @ 4-byte Spill
	ldr	r3, [sp, #32]                   @ 4-byte Reload
	ldr	r4, [r9, #48]
	adcs	r3, r8, r3
	str	r3, [sp, #36]                   @ 4-byte Spill
	ldr	r3, [sp, #28]                   @ 4-byte Reload
	ldr	r8, [sp, #52]                   @ 4-byte Reload
	adcs	r3, r11, r3
	str	r3, [sp, #32]                   @ 4-byte Spill
	ldr	r3, [sp, #24]                   @ 4-byte Reload
	adcs	r1, r1, r3
	str	r1, [sp, #28]                   @ 4-byte Spill
	adcs	r1, r6, r4
	ldr	r3, [sp, #60]                   @ 4-byte Reload
	str	r1, [sp, #24]                   @ 4-byte Spill
	mov	r1, #0
	adc	r1, r1, #0
	str	r1, [sp, #20]                   @ 4-byte Spill
	mul	r1, r3, r10
	umull	r3, r4, r1, r2
	str	r3, [sp, #60]                   @ 4-byte Spill
	umull	r6, r3, r1, r8
	adds	r4, r6, r4
	umull	r6, r2, r1, r7
	ldr	r7, [sp, #56]                   @ 4-byte Reload
	adcs	r11, r6, r3
	umull	r6, r3, r1, r7
	adcs	r12, r6, r2
	umull	r6, r2, r1, lr
	adcs	lr, r6, r3
	umull	r6, r3, r1, r0
	adcs	r2, r6, r2
	umull	r6, r0, r1, r5
	ldr	r1, [sp, #20]                   @ 4-byte Reload
	ldr	r5, [sp, #36]                   @ 4-byte Reload
	adcs	r3, r6, r3
	adc	r6, r1, r0
	ldr	r0, [sp, #60]                   @ 4-byte Reload
	ldr	r1, [sp, #40]                   @ 4-byte Reload
	adds	r0, r0, r10
	ldr	r0, [sp, #44]                   @ 4-byte Reload
	adcs	r0, r4, r0
	ldr	r4, [sp, #32]                   @ 4-byte Reload
	adcs	r1, r11, r1
	str	r0, [sp, #60]                   @ 4-byte Spill
	adcs	r5, r12, r5
	adcs	r10, lr, r4
	ldr	r4, [sp, #28]                   @ 4-byte Reload
	adcs	r12, r2, r4
	ldr	r2, [sp, #24]                   @ 4-byte Reload
	ldr	r4, [r9, #52]
	mov	r9, r1
	adcs	r3, r3, r2
	ldr	r2, [sp, #80]                   @ 4-byte Reload
	adc	r6, r6, r4
	subs	lr, r0, r2
	ldr	r0, [sp, #64]                   @ 4-byte Reload
	sbcs	r2, r1, r8
	mov	r8, r5
	sbcs	r1, r5, r0
	ldr	r0, [sp, #76]                   @ 4-byte Reload
	sbcs	r7, r10, r7
	sbcs	r5, r12, r0
	ldr	r0, [sp, #72]                   @ 4-byte Reload
	sbcs	r4, r3, r0
	ldr	r0, [sp, #68]                   @ 4-byte Reload
	sbc	r11, r6, r0
	asr	r0, r11, #31
	cmn	r0, #1
	movgt	r6, r11
	movle	r4, r3
	movle	r5, r12
	cmn	r0, #1
	movle	r7, r10
	movle	r1, r8
	movle	r2, r9
	cmn	r0, #1
	ldr	r0, [sp, #60]                   @ 4-byte Reload
	ldr	r11, [sp, #48]                  @ 4-byte Reload
	movle	lr, r0
	str	r6, [r11, #24]
	str	r4, [r11, #20]
	str	r5, [r11, #16]
	str	r7, [r11, #12]
	str	r1, [r11, #8]
	str	r2, [r11, #4]
	str	lr, [r11]
	add	sp, sp, #84
	pop	{r4, r5, r6, r7, r8, r9, r10, r11, lr}
	mov	pc, lr
.Lfunc_end29:
	.size	mcl_fp_montRedNF7L, .Lfunc_end29-mcl_fp_montRedNF7L
	.cantunwind
	.fnend
                                        @ -- End function
	.globl	mcl_fp_addPre7L                 @ -- Begin function mcl_fp_addPre7L
	.p2align	2
	.type	mcl_fp_addPre7L,%function
	.code	32                              @ @mcl_fp_addPre7L
mcl_fp_addPre7L:
	.fnstart
@ %bb.0:
	.save	{r4, r5, r6, r7, r8, lr}
	push	{r4, r5, r6, r7, r8, lr}
	ldm	r2, {r3, r12, lr}
	ldm	r1, {r5, r6, r7}
	adds	r3, r5, r3
	str	r3, [r0]
	adcs	r3, r6, r12
	ldr	r8, [r2, #12]
	ldr	r4, [r1, #12]
	str	r3, [r0, #4]
	adcs	r3, r7, lr
	str	r3, [r0, #8]
	adcs	r3, r4, r8
	str	r3, [r0, #12]
	ldr	r3, [r2, #16]
	ldr	r7, [r1, #16]
	adcs	r3, r7, r3
	str	r3, [r0, #16]
	ldr	r3, [r2, #20]
	ldr	r7, [r1, #20]
	ldr	r2, [r2, #24]
	ldr	r1, [r1, #24]
	adcs	r3, r7, r3
	str	r3, [r0, #20]
	adcs	r1, r1, r2
	str	r1, [r0, #24]
	mov	r0, #0
	adc	r0, r0, #0
	pop	{r4, r5, r6, r7, r8, lr}
	mov	pc, lr
.Lfunc_end30:
	.size	mcl_fp_addPre7L, .Lfunc_end30-mcl_fp_addPre7L
	.cantunwind
	.fnend
                                        @ -- End function
	.globl	mcl_fp_subPre7L                 @ -- Begin function mcl_fp_subPre7L
	.p2align	2
	.type	mcl_fp_subPre7L,%function
	.code	32                              @ @mcl_fp_subPre7L
mcl_fp_subPre7L:
	.fnstart
@ %bb.0:
	.save	{r4, r5, r6, r7, r8, lr}
	push	{r4, r5, r6, r7, r8, lr}
	ldm	r2, {r3, r12, lr}
	ldm	r1, {r5, r6, r7}
	subs	r3, r5, r3
	str	r3, [r0]
	sbcs	r3, r6, r12
	ldr	r8, [r2, #12]
	ldr	r4, [r1, #12]
	str	r3, [r0, #4]
	sbcs	r3, r7, lr
	str	r3, [r0, #8]
	sbcs	r3, r4, r8
	str	r3, [r0, #12]
	ldr	r3, [r2, #16]
	ldr	r7, [r1, #16]
	sbcs	r3, r7, r3
	str	r3, [r0, #16]
	ldr	r3, [r2, #20]
	ldr	r7, [r1, #20]
	ldr	r2, [r2, #24]
	ldr	r1, [r1, #24]
	sbcs	r3, r7, r3
	str	r3, [r0, #20]
	sbcs	r1, r1, r2
	str	r1, [r0, #24]
	mov	r0, #0
	sbc	r0, r0, #0
	and	r0, r0, #1
	pop	{r4, r5, r6, r7, r8, lr}
	mov	pc, lr
.Lfunc_end31:
	.size	mcl_fp_subPre7L, .Lfunc_end31-mcl_fp_subPre7L
	.cantunwind
	.fnend
                                        @ -- End function
	.globl	mcl_fp_shr1_7L                  @ -- Begin function mcl_fp_shr1_7L
	.p2align	2
	.type	mcl_fp_shr1_7L,%function
	.code	32                              @ @mcl_fp_shr1_7L
mcl_fp_shr1_7L:
	.fnstart
@ %bb.0:
	.save	{r4, r5, r6, r7, r8, lr}
	push	{r4, r5, r6, r7, r8, lr}
	add	r5, r1, #8
	ldm	r1, {r12, lr}
	ldr	r1, [r1, #24]
	ldm	r5, {r2, r3, r4, r5}
	lsr	r6, r5, #1
	lsr	r7, r3, #1
	lsrs	r5, r5, #1
	orr	r8, r6, r1, lsl #31
	lsr	r6, lr, #1
	orr	r7, r7, r4, lsl #31
	rrx	r4, r4
	lsrs	r3, r3, #1
	orr	r6, r6, r2, lsl #31
	rrx	r2, r2
	lsrs	r3, lr, #1
	lsr	r1, r1, #1
	rrx	r3, r12
	stm	r0, {r3, r6}
	str	r2, [r0, #8]
	str	r7, [r0, #12]
	str	r4, [r0, #16]
	str	r8, [r0, #20]
	str	r1, [r0, #24]
	pop	{r4, r5, r6, r7, r8, lr}
	mov	pc, lr
.Lfunc_end32:
	.size	mcl_fp_shr1_7L, .Lfunc_end32-mcl_fp_shr1_7L
	.cantunwind
	.fnend
                                        @ -- End function
	.globl	mcl_fp_add7L                    @ -- Begin function mcl_fp_add7L
	.p2align	2
	.type	mcl_fp_add7L,%function
	.code	32                              @ @mcl_fp_add7L
mcl_fp_add7L:
	.fnstart
@ %bb.0:
	.save	{r4, r5, r6, r7, r8, r9, r10, r11, lr}
	push	{r4, r5, r6, r7, r8, r9, r10, r11, lr}
	.pad	#12
	sub	sp, sp, #12
	ldr	r9, [r2]
	ldm	r1, {r6, r7}
	ldmib	r2, {r8, r12}
	adds	r5, r6, r9
	ldr	r4, [r1, #8]
	adcs	r7, r7, r8
	ldr	r11, [r2, #12]
	ldr	lr, [r1, #12]
	adcs	r9, r4, r12
	ldr	r6, [r2, #16]
	ldr	r4, [r1, #16]
	adcs	r8, lr, r11
	ldr	r10, [r2, #20]
	adcs	r11, r4, r6
	ldr	r4, [r1, #20]
	ldr	r2, [r2, #24]
	ldr	r1, [r1, #24]
	adcs	r4, r4, r10
	str	r5, [sp, #8]                    @ 4-byte Spill
	adcs	r6, r1, r2
	mov	r1, #0
	ldr	r2, [r3]
	adc	r1, r1, #0
	ldr	r10, [sp, #8]                   @ 4-byte Reload
	str	r7, [sp, #4]                    @ 4-byte Spill
	stm	r0, {r5, r7, r9}
	subs	r10, r10, r2
	str	r1, [sp]                        @ 4-byte Spill
	ldmib	r3, {r1, r5, r7, r12, lr}
	ldr	r2, [sp, #4]                    @ 4-byte Reload
	ldr	r3, [r3, #24]
	sbcs	r2, r2, r1
	str	r4, [r0, #20]
	sbcs	r9, r9, r5
	str	r8, [r0, #12]
	sbcs	r1, r8, r7
	str	r11, [r0, #16]
	sbcs	r7, r11, r12
	str	r6, [r0, #24]
	sbcs	r4, r4, lr
	sbcs	r5, r6, r3
	ldr	r3, [sp]                        @ 4-byte Reload
	sbc	r3, r3, #0
	tst	r3, #1
	bne	.LBB33_2
@ %bb.1:                                @ %nocarry
	str	r10, [r0]
	stmib	r0, {r2, r9}
	str	r1, [r0, #12]
	str	r7, [r0, #16]
	str	r4, [r0, #20]
	str	r5, [r0, #24]
.LBB33_2:                               @ %carry
	add	sp, sp, #12
	pop	{r4, r5, r6, r7, r8, r9, r10, r11, lr}
	mov	pc, lr
.Lfunc_end33:
	.size	mcl_fp_add7L, .Lfunc_end33-mcl_fp_add7L
	.cantunwind
	.fnend
                                        @ -- End function
	.globl	mcl_fp_addNF7L                  @ -- Begin function mcl_fp_addNF7L
	.p2align	2
	.type	mcl_fp_addNF7L,%function
	.code	32                              @ @mcl_fp_addNF7L
mcl_fp_addNF7L:
	.fnstart
@ %bb.0:
	.save	{r4, r5, r6, r7, r8, r9, r10, r11, lr}
	push	{r4, r5, r6, r7, r8, r9, r10, r11, lr}
	.pad	#16
	sub	sp, sp, #16
	ldm	r1, {r5, r7}
	add	r11, r1, #8
	add	lr, r2, #12
	str	r7, [sp, #8]                    @ 4-byte Spill
	ldr	r7, [r1, #20]
	ldr	r8, [r1, #24]
	ldm	r2, {r1, r4, r12}
	adds	r1, r1, r5
	ldr	r5, [sp, #8]                    @ 4-byte Reload
	ldm	r11, {r9, r10, r11}
	adcs	r5, r4, r5
	str	r7, [sp, #4]                    @ 4-byte Spill
	adcs	r4, r12, r9
	ldm	lr, {r6, r7, lr}
	adcs	r10, r6, r10
	str	r4, [sp]                        @ 4-byte Spill
	adcs	r9, r7, r11
	ldr	r7, [sp, #4]                    @ 4-byte Reload
	ldr	r2, [r2, #24]
	ldm	r3, {r4, r12}
	adcs	r7, lr, r7
	adc	r2, r2, r8
	subs	r4, r1, r4
	str	r5, [sp, #8]                    @ 4-byte Spill
	sbcs	r5, r5, r12
	ldr	r6, [r3, #8]
	ldr	r12, [sp]                       @ 4-byte Reload
	ldr	lr, [r3, #12]
	sbcs	r6, r12, r6
	ldr	r8, [r3, #16]
	sbcs	lr, r10, lr
	ldr	r11, [r3, #20]
	sbcs	r8, r9, r8
	ldr	r3, [r3, #24]
	sbcs	r11, r7, r11
	str	r1, [sp, #12]                   @ 4-byte Spill
	sbc	r3, r2, r3
	asr	r1, r3, #31
	cmn	r1, #1
	movgt	r2, r3
	movle	r11, r7
	str	r2, [r0, #24]
	movle	r8, r9
	ldr	r2, [sp, #8]                    @ 4-byte Reload
	cmn	r1, #1
	movle	lr, r10
	movle	r6, r12
	str	r11, [r0, #20]
	movle	r5, r2
	cmn	r1, #1
	ldr	r1, [sp, #12]                   @ 4-byte Reload
	str	r8, [r0, #16]
	str	lr, [r0, #12]
	movle	r4, r1
	str	r6, [r0, #8]
	str	r5, [r0, #4]
	str	r4, [r0]
	add	sp, sp, #16
	pop	{r4, r5, r6, r7, r8, r9, r10, r11, lr}
	mov	pc, lr
.Lfunc_end34:
	.size	mcl_fp_addNF7L, .Lfunc_end34-mcl_fp_addNF7L
	.cantunwind
	.fnend
                                        @ -- End function
	.globl	mcl_fp_sub7L                    @ -- Begin function mcl_fp_sub7L
	.p2align	2
	.type	mcl_fp_sub7L,%function
	.code	32                              @ @mcl_fp_sub7L
mcl_fp_sub7L:
	.fnstart
@ %bb.0:
	.save	{r4, r5, r6, r7, r8, r9, r10, lr}
	push	{r4, r5, r6, r7, r8, r9, r10, lr}
	ldr	r9, [r2]
	ldm	r1, {r4, r5, r6, r7}
	ldmib	r2, {r8, lr}
	subs	r4, r4, r9
	sbcs	r8, r5, r8
	ldr	r12, [r2, #12]
	sbcs	r9, r6, lr
	ldr	r6, [r2, #16]
	sbcs	r10, r7, r12
	ldr	r7, [r1, #16]
	ldr	r5, [r2, #20]
	sbcs	lr, r7, r6
	ldr	r7, [r1, #20]
	ldr	r2, [r2, #24]
	ldr	r1, [r1, #24]
	sbcs	r12, r7, r5
	stm	r0, {r4, r8, r9, r10, lr}
	sbcs	r1, r1, r2
	mov	r2, #0
	str	r12, [r0, #20]
	sbc	r2, r2, #0
	str	r1, [r0, #24]
	tst	r2, #1
	bne	.LBB35_2
@ %bb.1:                                @ %nocarry
	pop	{r4, r5, r6, r7, r8, r9, r10, lr}
	mov	pc, lr
.LBB35_2:                               @ %carry
	ldm	r3, {r2, r5, r6, r7}
	adds	r2, r2, r4
	str	r2, [r0]
	adcs	r2, r5, r8
	str	r2, [r0, #4]
	adcs	r2, r6, r9
	str	r2, [r0, #8]
	adcs	r2, r7, r10
	str	r2, [r0, #12]
	ldr	r2, [r3, #16]
	adcs	r2, r2, lr
	str	r2, [r0, #16]
	ldr	r2, [r3, #20]
	adcs	r2, r2, r12
	str	r2, [r0, #20]
	ldr	r2, [r3, #24]
	adc	r1, r2, r1
	str	r1, [r0, #24]
	pop	{r4, r5, r6, r7, r8, r9, r10, lr}
	mov	pc, lr
.Lfunc_end35:
	.size	mcl_fp_sub7L, .Lfunc_end35-mcl_fp_sub7L
	.cantunwind
	.fnend
                                        @ -- End function
	.globl	mcl_fp_subNF7L                  @ -- Begin function mcl_fp_subNF7L
	.p2align	2
	.type	mcl_fp_subNF7L,%function
	.code	32                              @ @mcl_fp_subNF7L
mcl_fp_subNF7L:
	.fnstart
@ %bb.0:
	.save	{r4, r5, r6, r7, r8, r9, r10, r11, lr}
	push	{r4, r5, r6, r7, r8, r9, r10, r11, lr}
	.pad	#20
	sub	sp, sp, #20
	ldm	r2, {r5, r7}
	add	r11, r2, #8
	add	lr, r1, #12
	str	r7, [sp, #12]                   @ 4-byte Spill
	ldr	r7, [r2, #20]
	ldr	r8, [r2, #24]
	ldm	r1, {r2, r4, r12}
	subs	r2, r2, r5
	ldr	r5, [sp, #12]                   @ 4-byte Reload
	ldm	r11, {r9, r10, r11}
	sbcs	r5, r4, r5
	str	r7, [sp, #4]                    @ 4-byte Spill
	sbcs	r4, r12, r9
	ldm	lr, {r6, r7, lr}
	sbcs	r6, r6, r10
	ldr	r1, [r1, #24]
	sbcs	r10, r7, r11
	ldr	r7, [sp, #4]                    @ 4-byte Reload
	str	r6, [sp]                        @ 4-byte Spill
	sbcs	r7, lr, r7
	ldm	r3, {r6, r12, lr}
	sbc	r1, r1, r8
	adds	r6, r2, r6
	str	r5, [sp, #12]                   @ 4-byte Spill
	adcs	r5, r5, r12
	str	r4, [sp, #8]                    @ 4-byte Spill
	adcs	lr, r4, lr
	ldr	r8, [r3, #12]
	ldr	r4, [sp]                        @ 4-byte Reload
	asr	r12, r1, #31
	ldr	r11, [r3, #16]
	adcs	r8, r4, r8
	ldr	r9, [r3, #20]
	ldr	r3, [r3, #24]
	adcs	r11, r10, r11
	str	r2, [sp, #16]                   @ 4-byte Spill
	adcs	r2, r7, r9
	adc	r3, r1, r3
	cmp	r12, #0
	movpl	r3, r1
	ldr	r1, [sp, #8]                    @ 4-byte Reload
	movpl	r2, r7
	movpl	r11, r10
	cmp	r12, #0
	str	r3, [r0, #24]
	movpl	lr, r1
	ldr	r1, [sp, #12]                   @ 4-byte Reload
	movpl	r8, r4
	str	r2, [r0, #20]
	str	r11, [r0, #16]
	movpl	r5, r1
	ldr	r1, [sp, #16]                   @ 4-byte Reload
	cmp	r12, #0
	str	r8, [r0, #12]
	str	lr, [r0, #8]
	movpl	r6, r1
	str	r5, [r0, #4]
	str	r6, [r0]
	add	sp, sp, #20
	pop	{r4, r5, r6, r7, r8, r9, r10, r11, lr}
	mov	pc, lr
.Lfunc_end36:
	.size	mcl_fp_subNF7L, .Lfunc_end36-mcl_fp_subNF7L
	.cantunwind
	.fnend
                                        @ -- End function
	.globl	mcl_fpDbl_add7L                 @ -- Begin function mcl_fpDbl_add7L
	.p2align	2
	.type	mcl_fpDbl_add7L,%function
	.code	32                              @ @mcl_fpDbl_add7L
mcl_fpDbl_add7L:
	.fnstart
@ %bb.0:
	.save	{r4, r5, r6, r7, r8, r9, r10, r11, lr}
	push	{r4, r5, r6, r7, r8, r9, r10, r11, lr}
	.pad	#40
	sub	sp, sp, #40
	ldr	r9, [r2]
	ldm	r1, {r4, r5, r6, r7}
	ldmib	r2, {r8, lr}
	adds	r4, r4, r9
	str	r4, [sp, #32]                   @ 4-byte Spill
	adcs	r4, r5, r8
	ldr	r12, [r2, #12]
	adcs	r6, r6, lr
	str	r6, [sp, #24]                   @ 4-byte Spill
	adcs	r7, r7, r12
	str	r7, [sp, #20]                   @ 4-byte Spill
	ldr	r6, [r2, #16]
	ldr	r7, [r1, #16]
	str	r4, [sp, #28]                   @ 4-byte Spill
	adcs	r7, r7, r6
	str	r7, [sp, #16]                   @ 4-byte Spill
	ldr	r4, [r2, #20]
	ldr	r7, [r1, #20]
	ldr	r6, [r1, #24]
	adcs	r7, r7, r4
	str	r7, [sp, #12]                   @ 4-byte Spill
	ldr	r7, [r2, #24]
	ldr	r5, [r1, #32]
	adcs	r7, r6, r7
	str	r7, [sp, #4]                    @ 4-byte Spill
	ldr	r7, [r2, #28]
	ldr	r6, [r1, #28]
	ldr	r4, [r1, #36]
	adcs	r9, r6, r7
	ldr	r6, [r2, #32]
	str	r9, [sp, #8]                    @ 4-byte Spill
	adcs	r7, r5, r6
	ldr	r5, [r2, #36]
	str	r7, [sp, #36]                   @ 4-byte Spill
	adcs	r8, r4, r5
	ldr	r5, [r2, #40]
	ldr	r7, [r1, #40]
	ldr	r6, [r1, #44]
	adcs	r5, r7, r5
	ldr	r7, [r2, #44]
	adcs	r7, r6, r7
	str	r7, [sp]                        @ 4-byte Spill
	ldr	r7, [r2, #48]
	ldr	r6, [r1, #48]
	ldr	r2, [r2, #52]
	ldr	r1, [r1, #52]
	adcs	r11, r6, r7
	adcs	r10, r1, r2
	mov	r1, #0
	adc	r7, r1, #0
	ldm	r3, {r1, r2, r12, lr}
	subs	r6, r9, r1
	ldr	r1, [sp, #36]                   @ 4-byte Reload
	sbcs	r9, r1, r2
	ldr	r1, [sp, #32]                   @ 4-byte Reload
	str	r1, [r0]
	sbcs	r4, r8, r12
	ldr	r1, [sp, #28]                   @ 4-byte Reload
	sbcs	lr, r5, lr
	str	r1, [r0, #4]
	ldr	r1, [sp, #24]                   @ 4-byte Reload
	str	r1, [r0, #8]
	ldr	r1, [sp, #20]                   @ 4-byte Reload
	str	r1, [r0, #12]
	ldr	r1, [sp, #16]                   @ 4-byte Reload
	str	r1, [r0, #16]
	ldr	r1, [sp, #12]                   @ 4-byte Reload
	str	r1, [r0, #20]
	ldr	r1, [sp, #4]                    @ 4-byte Reload
	str	r1, [r0, #24]
	ldr	r2, [r3, #24]
	ldr	r1, [r3, #20]
	ldr	r3, [r3, #16]
	ldr	r12, [sp]                       @ 4-byte Reload
	sbcs	r3, r12, r3
	sbcs	r1, r11, r1
	sbcs	r2, r10, r2
	sbc	r7, r7, #0
	ands	r7, r7, #1
	movne	r1, r11
	movne	r2, r10
	str	r1, [r0, #48]
	movne	r3, r12
	ldr	r1, [sp, #36]                   @ 4-byte Reload
	cmp	r7, #0
	movne	lr, r5
	movne	r4, r8
	str	r2, [r0, #52]
	movne	r9, r1
	ldr	r1, [sp, #8]                    @ 4-byte Reload
	cmp	r7, #0
	str	r3, [r0, #44]
	str	lr, [r0, #40]
	movne	r6, r1
	str	r4, [r0, #36]
	str	r9, [r0, #32]
	str	r6, [r0, #28]
	add	sp, sp, #40
	pop	{r4, r5, r6, r7, r8, r9, r10, r11, lr}
	mov	pc, lr
.Lfunc_end37:
	.size	mcl_fpDbl_add7L, .Lfunc_end37-mcl_fpDbl_add7L
	.cantunwind
	.fnend
                                        @ -- End function
	.globl	mcl_fpDbl_sub7L                 @ -- Begin function mcl_fpDbl_sub7L
	.p2align	2
	.type	mcl_fpDbl_sub7L,%function
	.code	32                              @ @mcl_fpDbl_sub7L
mcl_fpDbl_sub7L:
	.fnstart
@ %bb.0:
	.save	{r4, r5, r6, r7, r8, r9, r10, r11, lr}
	push	{r4, r5, r6, r7, r8, r9, r10, r11, lr}
	.pad	#60
	sub	sp, sp, #60
	ldr	r7, [r2, #32]
	str	r7, [sp, #28]                   @ 4-byte Spill
	ldr	r7, [r2, #36]
	str	r7, [sp, #36]                   @ 4-byte Spill
	ldr	r7, [r2, #40]
	str	r7, [sp, #40]                   @ 4-byte Spill
	ldr	r7, [r2, #44]
	str	r7, [sp, #44]                   @ 4-byte Spill
	ldr	r7, [r2, #48]
	str	r7, [sp, #48]                   @ 4-byte Spill
	ldr	r7, [r2, #52]
	str	r7, [sp, #52]                   @ 4-byte Spill
	ldr	r7, [r1, #52]
	str	r7, [sp, #32]                   @ 4-byte Spill
	ldr	r6, [r2]
	ldmib	r2, {r5, r7, r8, r9, r10, r11}
	ldr	r2, [r2, #28]
	str	r2, [sp, #56]                   @ 4-byte Spill
	ldm	r1, {r2, r12, lr}
	subs	r2, r2, r6
	str	r2, [sp, #24]                   @ 4-byte Spill
	ldr	r2, [r1, #48]
	str	r2, [sp]                        @ 4-byte Spill
	sbcs	r2, r12, r5
	ldr	r4, [r1, #12]
	str	r2, [sp, #20]                   @ 4-byte Spill
	sbcs	r2, lr, r7
	str	r2, [sp, #16]                   @ 4-byte Spill
	sbcs	r2, r4, r8
	str	r2, [sp, #12]                   @ 4-byte Spill
	mov	lr, #0
	ldr	r2, [r1, #16]
	ldr	r5, [sp, #56]                   @ 4-byte Reload
	sbcs	r2, r2, r9
	str	r2, [sp, #8]                    @ 4-byte Spill
	ldr	r2, [r1, #20]
	ldr	r6, [r1, #44]
	sbcs	r2, r2, r10
	str	r2, [sp, #4]                    @ 4-byte Spill
	ldr	r2, [r1, #24]
	ldr	r7, [r1, #40]
	sbcs	r11, r2, r11
	ldr	r2, [r1, #28]
	str	r11, [r0, #24]
	sbcs	r4, r2, r5
	ldr	r5, [r1, #36]
	ldr	r1, [r1, #32]
	ldr	r2, [sp, #28]                   @ 4-byte Reload
	ldr	r11, [r3, #24]
	sbcs	r12, r1, r2
	ldr	r1, [sp, #36]                   @ 4-byte Reload
	ldr	r2, [sp, #40]                   @ 4-byte Reload
	sbcs	r1, r5, r1
	ldr	r5, [sp, #44]                   @ 4-byte Reload
	sbcs	r2, r7, r2
	str	r12, [sp, #28]                  @ 4-byte Spill
	sbcs	r10, r6, r5
	ldr	r5, [sp, #48]                   @ 4-byte Reload
	ldr	r6, [sp]                        @ 4-byte Reload
	str	r4, [sp, #56]                   @ 4-byte Spill
	sbcs	r5, r6, r5
	str	r5, [sp, #48]                   @ 4-byte Spill
	ldr	r5, [sp, #52]                   @ 4-byte Reload
	ldr	r6, [sp, #32]                   @ 4-byte Reload
	sbcs	r9, r6, r5
	ldr	r6, [r3, #12]
	sbc	r7, lr, #0
	str	r7, [sp, #52]                   @ 4-byte Spill
	ldr	r7, [r3]
	ldmib	r3, {r5, lr}
	adds	r8, r4, r7
	adcs	r5, r12, r5
	ldr	r4, [sp, #52]                   @ 4-byte Reload
	adcs	r7, r1, lr
	mov	lr, r1
	ldr	r1, [sp, #24]                   @ 4-byte Reload
	adcs	r12, r2, r6
	str	r1, [r0]
	mov	r6, r10
	ldr	r1, [sp, #20]                   @ 4-byte Reload
	str	r1, [r0, #4]
	ldr	r1, [sp, #16]                   @ 4-byte Reload
	str	r1, [r0, #8]
	ldr	r1, [sp, #12]                   @ 4-byte Reload
	str	r1, [r0, #12]
	ldr	r1, [sp, #8]                    @ 4-byte Reload
	str	r1, [r0, #16]
	ldr	r1, [sp, #4]                    @ 4-byte Reload
	str	r1, [r0, #20]
	ldr	r1, [r3, #20]
	ldr	r3, [r3, #16]
	adcs	r3, r10, r3
	ldr	r10, [sp, #48]                  @ 4-byte Reload
	adcs	r1, r10, r1
	adc	r11, r9, r11
	ands	r4, r4, #1
	moveq	r1, r10
	moveq	r11, r9
	str	r1, [r0, #48]
	moveq	r3, r6
	ldr	r1, [sp, #28]                   @ 4-byte Reload
	cmp	r4, #0
	moveq	r12, r2
	moveq	r7, lr
	str	r11, [r0, #52]
	moveq	r5, r1
	ldr	r1, [sp, #56]                   @ 4-byte Reload
	cmp	r4, #0
	str	r3, [r0, #44]
	str	r12, [r0, #40]
	moveq	r8, r1
	str	r7, [r0, #36]
	str	r5, [r0, #32]
	str	r8, [r0, #28]
	add	sp, sp, #60
	pop	{r4, r5, r6, r7, r8, r9, r10, r11, lr}
	mov	pc, lr
.Lfunc_end38:
	.size	mcl_fpDbl_sub7L, .Lfunc_end38-mcl_fpDbl_sub7L
	.cantunwind
	.fnend
                                        @ -- End function
	.globl	mulPv256x32                     @ -- Begin function mulPv256x32
	.p2align	2
	.type	mulPv256x32,%function
	.code	32                              @ @mulPv256x32
mulPv256x32:
	.fnstart
@ %bb.0:
	.save	{r4, r5, r6, r7, r8, r9, r11, lr}
	push	{r4, r5, r6, r7, r8, r9, r11, lr}
	ldr	r12, [r1]
	ldmib	r1, {r3, lr}
	umull	r7, r6, r12, r2
	ldr	r4, [r1, #12]
	umull	r5, r8, r3, r2
	str	r7, [r0]
	umull	r9, r12, r4, r2
	adds	r7, r6, r5
	umull	r5, r4, lr, r2
	adcs	r7, r8, r5
	umlal	r6, r5, r3, r2
	adcs	r7, r4, r9
	str	r7, [r0, #12]
	str	r5, [r0, #8]
	str	r6, [r0, #4]
	ldr	r3, [r1, #16]
	umull	r7, r6, r3, r2
	adcs	r3, r12, r7
	str	r3, [r0, #16]
	ldr	r3, [r1, #20]
	umull	r7, r5, r3, r2
	adcs	r3, r6, r7
	str	r3, [r0, #20]
	ldr	r3, [r1, #24]
	umull	r7, r6, r3, r2
	adcs	r3, r5, r7
	str	r3, [r0, #24]
	ldr	r1, [r1, #28]
	umull	r3, r7, r1, r2
	adcs	r1, r6, r3
	str	r1, [r0, #28]
	adc	r1, r7, #0
	str	r1, [r0, #32]
	pop	{r4, r5, r6, r7, r8, r9, r11, lr}
	mov	pc, lr
.Lfunc_end39:
	.size	mulPv256x32, .Lfunc_end39-mulPv256x32
	.cantunwind
	.fnend
                                        @ -- End function
	.globl	mcl_fp_mulUnitPre8L             @ -- Begin function mcl_fp_mulUnitPre8L
	.p2align	2
	.type	mcl_fp_mulUnitPre8L,%function
	.code	32                              @ @mcl_fp_mulUnitPre8L
mcl_fp_mulUnitPre8L:
	.fnstart
@ %bb.0:
	.save	{r4, r5, r6, r7, r11, lr}
	push	{r4, r5, r6, r7, r11, lr}
	.pad	#40
	sub	sp, sp, #40
	mov	r4, r0
	mov	r0, sp
	bl	mulPv256x32
	add	r7, sp, #24
	ldm	sp, {r0, r1, r2, r3, r12, lr}
	ldm	r7, {r5, r6, r7}
	stm	r4, {r0, r1, r2, r3, r12, lr}
	add	r0, r4, #24
	stm	r0, {r5, r6, r7}
	add	sp, sp, #40
	pop	{r4, r5, r6, r7, r11, lr}
	mov	pc, lr
.Lfunc_end40:
	.size	mcl_fp_mulUnitPre8L, .Lfunc_end40-mcl_fp_mulUnitPre8L
	.cantunwind
	.fnend
                                        @ -- End function
	.globl	mcl_fpDbl_mulPre8L              @ -- Begin function mcl_fpDbl_mulPre8L
	.p2align	2
	.type	mcl_fpDbl_mulPre8L,%function
	.code	32                              @ @mcl_fpDbl_mulPre8L
mcl_fpDbl_mulPre8L:
	.fnstart
@ %bb.0:
	.save	{r4, r5, r6, r7, r8, r9, r10, r11, lr}
	push	{r4, r5, r6, r7, r8, r9, r10, r11, lr}
	.pad	#356
	sub	sp, sp, #356
	mov	r9, r2
	ldr	r2, [r2]
	mov	r4, r0
	add	r0, sp, #312
	mov	r7, r1
	bl	mulPv256x32
	ldr	r0, [sp, #344]
	str	r0, [sp, #28]                   @ 4-byte Spill
	ldr	r0, [sp, #340]
	str	r0, [sp, #24]                   @ 4-byte Spill
	ldr	r0, [sp, #336]
	str	r0, [sp, #20]                   @ 4-byte Spill
	ldr	r0, [sp, #332]
	str	r0, [sp, #16]                   @ 4-byte Spill
	ldr	r0, [sp, #328]
	str	r0, [sp, #12]                   @ 4-byte Spill
	ldr	r1, [sp, #320]
	ldr	r0, [sp, #312]
	ldr	r8, [sp, #316]
	str	r1, [sp, #4]                    @ 4-byte Spill
	mov	r1, r7
	ldr	r2, [r9, #4]
	ldr	r10, [sp, #324]
	str	r0, [r4]
	add	r0, sp, #272
	bl	mulPv256x32
	ldr	r5, [sp, #272]
	add	lr, sp, #280
	ldr	r12, [sp, #304]
	adds	r5, r5, r8
	ldr	r6, [sp, #276]
	ldm	lr, {r0, r1, r2, r3, r11, lr}
	str	r5, [r4, #4]
	ldr	r5, [sp, #4]                    @ 4-byte Reload
	adcs	r8, r6, r5
	adcs	r0, r0, r10
	str	r0, [sp, #4]                    @ 4-byte Spill
	ldr	r0, [sp, #12]                   @ 4-byte Reload
	adcs	r0, r1, r0
	str	r0, [sp, #12]                   @ 4-byte Spill
	ldr	r0, [sp, #16]                   @ 4-byte Reload
	mov	r1, r7
	adcs	r0, r2, r0
	str	r0, [sp, #16]                   @ 4-byte Spill
	ldr	r0, [sp, #20]                   @ 4-byte Reload
	ldr	r2, [r9, #8]
	adcs	r10, r3, r0
	ldr	r0, [sp, #24]                   @ 4-byte Reload
	adcs	r0, r11, r0
	str	r0, [sp, #24]                   @ 4-byte Spill
	ldr	r0, [sp, #28]                   @ 4-byte Reload
	adcs	r0, lr, r0
	str	r0, [sp, #28]                   @ 4-byte Spill
	adc	r0, r12, #0
	str	r0, [sp, #20]                   @ 4-byte Spill
	add	r0, sp, #232
	bl	mulPv256x32
	ldr	r1, [sp, #232]
	add	r11, sp, #256
	add	lr, sp, #236
	adds	r1, r1, r8
	ldm	r11, {r5, r6, r11}
	ldm	lr, {r0, r2, r3, r12, lr}
	str	r1, [r4, #8]
	ldr	r1, [sp, #4]                    @ 4-byte Reload
	adcs	r8, r0, r1
	ldr	r0, [sp, #12]                   @ 4-byte Reload
	mov	r1, r7
	adcs	r0, r2, r0
	str	r0, [sp, #12]                   @ 4-byte Spill
	ldr	r0, [sp, #16]                   @ 4-byte Reload
	ldr	r2, [r9, #12]
	adcs	r0, r3, r0
	str	r0, [sp, #16]                   @ 4-byte Spill
	ldr	r0, [sp, #24]                   @ 4-byte Reload
	adcs	r10, r12, r10
	adcs	r0, lr, r0
	str	r0, [sp, #24]                   @ 4-byte Spill
	ldr	r0, [sp, #28]                   @ 4-byte Reload
	adcs	r0, r5, r0
	str	r0, [sp, #28]                   @ 4-byte Spill
	ldr	r0, [sp, #20]                   @ 4-byte Reload
	mov	r5, r7
	adcs	r0, r6, r0
	str	r0, [sp, #20]                   @ 4-byte Spill
	adc	r0, r11, #0
	str	r0, [sp, #4]                    @ 4-byte Spill
	add	r0, sp, #192
	bl	mulPv256x32
	ldr	r6, [sp, #192]
	add	lr, sp, #200
	ldr	r12, [sp, #224]
	adds	r6, r6, r8
	ldr	r7, [sp, #196]
	ldm	lr, {r0, r1, r2, r3, r11, lr}
	str	r6, [r4, #12]
	ldr	r6, [sp, #12]                   @ 4-byte Reload
	adcs	r8, r7, r6
	ldr	r6, [sp, #16]                   @ 4-byte Reload
	adcs	r0, r0, r6
	str	r0, [sp, #16]                   @ 4-byte Spill
	adcs	r0, r1, r10
	str	r0, [sp, #12]                   @ 4-byte Spill
	ldr	r0, [sp, #24]                   @ 4-byte Reload
	mov	r1, r5
	adcs	r0, r2, r0
	str	r0, [sp, #24]                   @ 4-byte Spill
	ldr	r0, [sp, #28]                   @ 4-byte Reload
	ldr	r2, [r9, #16]
	adcs	r10, r3, r0
	ldr	r0, [sp, #20]                   @ 4-byte Reload
	adcs	r0, r11, r0
	str	r0, [sp, #28]                   @ 4-byte Spill
	ldr	r0, [sp, #4]                    @ 4-byte Reload
	adcs	r0, lr, r0
	str	r0, [sp, #20]                   @ 4-byte Spill
	adc	r0, r12, #0
	str	r0, [sp, #4]                    @ 4-byte Spill
	add	r0, sp, #152
	bl	mulPv256x32
	ldr	r1, [sp, #152]
	add	lr, sp, #156
	ldr	r11, [sp, #184]
	adds	r1, r1, r8
	ldr	r6, [sp, #180]
	ldr	r7, [sp, #176]
	ldm	lr, {r0, r2, r3, r12, lr}
	str	r1, [r4, #16]
	ldr	r1, [sp, #16]                   @ 4-byte Reload
	str	r5, [sp, #8]                    @ 4-byte Spill
	adcs	r8, r0, r1
	ldr	r0, [sp, #12]                   @ 4-byte Reload
	mov	r1, r5
	adcs	r0, r2, r0
	str	r0, [sp, #16]                   @ 4-byte Spill
	ldr	r0, [sp, #24]                   @ 4-byte Reload
	ldr	r2, [r9, #20]
	adcs	r0, r3, r0
	str	r0, [sp, #24]                   @ 4-byte Spill
	ldr	r0, [sp, #28]                   @ 4-byte Reload
	adcs	r10, r12, r10
	adcs	r0, lr, r0
	str	r0, [sp, #28]                   @ 4-byte Spill
	ldr	r0, [sp, #20]                   @ 4-byte Reload
	adcs	r0, r7, r0
	str	r0, [sp, #20]                   @ 4-byte Spill
	ldr	r0, [sp, #4]                    @ 4-byte Reload
	adcs	r0, r6, r0
	str	r0, [sp, #12]                   @ 4-byte Spill
	adc	r0, r11, #0
	str	r0, [sp, #4]                    @ 4-byte Spill
	add	r0, sp, #112
	bl	mulPv256x32
	ldr	r6, [sp, #112]
	add	lr, sp, #120
	ldr	r12, [sp, #144]
	adds	r6, r6, r8
	ldr	r7, [sp, #116]
	ldm	lr, {r0, r1, r2, r3, r11, lr}
	str	r6, [r4, #20]
	ldr	r6, [sp, #16]                   @ 4-byte Reload
	adcs	r8, r7, r6
	ldr	r6, [sp, #24]                   @ 4-byte Reload
	adcs	r7, r0, r6
	ldr	r0, [sp, #28]                   @ 4-byte Reload
	adcs	r10, r1, r10
	mov	r1, r5
	adcs	r0, r2, r0
	str	r0, [sp, #28]                   @ 4-byte Spill
	ldr	r0, [sp, #20]                   @ 4-byte Reload
	ldr	r2, [r9, #24]
	adcs	r0, r3, r0
	str	r0, [sp, #24]                   @ 4-byte Spill
	ldr	r0, [sp, #12]                   @ 4-byte Reload
	adcs	r0, r11, r0
	str	r0, [sp, #20]                   @ 4-byte Spill
	ldr	r0, [sp, #4]                    @ 4-byte Reload
	adcs	r0, lr, r0
	str	r0, [sp, #16]                   @ 4-byte Spill
	adc	r0, r12, #0
	str	r0, [sp, #12]                   @ 4-byte Spill
	add	r0, sp, #72
	bl	mulPv256x32
	add	lr, sp, #76
	ldr	r2, [sp, #72]
	add	r11, sp, #96
	ldm	lr, {r0, r1, r3, r12, lr}
	adds	r2, r2, r8
	adcs	r8, r0, r7
	ldr	r0, [sp, #28]                   @ 4-byte Reload
	ldm	r11, {r5, r6, r11}
	str	r2, [r4, #24]
	ldr	r2, [r9, #28]
	adcs	r9, r1, r10
	adcs	r7, r3, r0
	ldr	r0, [sp, #24]                   @ 4-byte Reload
	ldr	r1, [sp, #8]                    @ 4-byte Reload
	adcs	r10, r12, r0
	ldr	r0, [sp, #20]                   @ 4-byte Reload
	adcs	r0, lr, r0
	str	r0, [sp, #28]                   @ 4-byte Spill
	ldr	r0, [sp, #16]                   @ 4-byte Reload
	adcs	r0, r5, r0
	str	r0, [sp, #24]                   @ 4-byte Spill
	ldr	r0, [sp, #12]                   @ 4-byte Reload
	adcs	r0, r6, r0
	str	r0, [sp, #20]                   @ 4-byte Spill
	add	r0, sp, #32
	adc	r11, r11, #0
	bl	mulPv256x32
	add	r3, sp, #32
	ldr	r6, [sp, #56]
	ldr	r5, [sp, #60]
	ldm	r3, {r0, r1, r2, r3}
	adds	lr, r0, r8
	ldr	r0, [sp, #48]
	adcs	r8, r1, r9
	ldr	r1, [sp, #28]                   @ 4-byte Reload
	adcs	r2, r2, r7
	ldr	r7, [sp, #52]
	adcs	r3, r3, r10
	ldr	r12, [sp, #64]
	adcs	r0, r0, r1
	ldr	r1, [sp, #24]                   @ 4-byte Reload
	str	lr, [r4, #28]
	adcs	r7, r7, r1
	ldr	r1, [sp, #20]                   @ 4-byte Reload
	str	r8, [r4, #32]
	adcs	r6, r6, r1
	str	r2, [r4, #36]
	adcs	r5, r5, r11
	str	r0, [r4, #44]
	adc	r0, r12, #0
	str	r3, [r4, #40]
	str	r7, [r4, #48]
	str	r6, [r4, #52]
	str	r5, [r4, #56]
	str	r0, [r4, #60]
	add	sp, sp, #356
	pop	{r4, r5, r6, r7, r8, r9, r10, r11, lr}
	mov	pc, lr
.Lfunc_end41:
	.size	mcl_fpDbl_mulPre8L, .Lfunc_end41-mcl_fpDbl_mulPre8L
	.cantunwind
	.fnend
                                        @ -- End function
	.globl	mcl_fpDbl_sqrPre8L              @ -- Begin function mcl_fpDbl_sqrPre8L
	.p2align	2
	.type	mcl_fpDbl_sqrPre8L,%function
	.code	32                              @ @mcl_fpDbl_sqrPre8L
mcl_fpDbl_sqrPre8L:
	.fnstart
@ %bb.0:
	.save	{r4, r5, r6, r7, r8, r9, r10, r11, lr}
	push	{r4, r5, r6, r7, r8, r9, r10, r11, lr}
	.pad	#356
	sub	sp, sp, #356
	ldr	r2, [r1]
	mov	r4, r0
	add	r0, sp, #312
	mov	r5, r1
	bl	mulPv256x32
	ldr	r0, [sp, #344]
	mov	r1, r5
	str	r0, [sp, #28]                   @ 4-byte Spill
	ldr	r0, [sp, #340]
	str	r0, [sp, #24]                   @ 4-byte Spill
	ldr	r0, [sp, #336]
	str	r0, [sp, #20]                   @ 4-byte Spill
	ldr	r0, [sp, #332]
	str	r0, [sp, #16]                   @ 4-byte Spill
	ldr	r0, [sp, #328]
	str	r0, [sp, #12]                   @ 4-byte Spill
	ldr	r0, [sp, #312]
	ldr	r2, [r5, #4]
	ldr	r11, [sp, #316]
	ldr	r10, [sp, #320]
	ldr	r9, [sp, #324]
	str	r0, [r4]
	add	r0, sp, #272
	bl	mulPv256x32
	add	r6, sp, #276
	ldr	r7, [sp, #272]
	add	lr, sp, #292
	ldr	r8, [sp, #304]
	ldm	r6, {r0, r1, r2, r6}
	adds	r7, r7, r11
	adcs	r10, r0, r10
	ldr	r0, [sp, #12]                   @ 4-byte Reload
	adcs	r9, r1, r9
	ldm	lr, {r3, r12, lr}
	adcs	r0, r2, r0
	str	r0, [sp, #12]                   @ 4-byte Spill
	mov	r1, r5
	ldr	r0, [sp, #16]                   @ 4-byte Reload
	ldr	r2, [r5, #8]
	adcs	r0, r6, r0
	str	r0, [sp, #16]                   @ 4-byte Spill
	ldr	r0, [sp, #20]                   @ 4-byte Reload
	str	r7, [r4, #4]
	adcs	r11, r3, r0
	ldr	r0, [sp, #24]                   @ 4-byte Reload
	adcs	r0, r12, r0
	str	r0, [sp, #24]                   @ 4-byte Spill
	ldr	r0, [sp, #28]                   @ 4-byte Reload
	adcs	r0, lr, r0
	str	r0, [sp, #28]                   @ 4-byte Spill
	adc	r0, r8, #0
	str	r0, [sp, #20]                   @ 4-byte Spill
	add	r0, sp, #232
	bl	mulPv256x32
	add	lr, sp, #236
	ldr	r1, [sp, #232]
	add	r8, sp, #256
	ldm	lr, {r0, r2, r3, r12, lr}
	adds	r1, r1, r10
	adcs	r9, r0, r9
	ldr	r0, [sp, #12]                   @ 4-byte Reload
	ldm	r8, {r6, r7, r8}
	adcs	r10, r2, r0
	ldr	r0, [sp, #16]                   @ 4-byte Reload
	ldr	r2, [r5, #12]
	adcs	r0, r3, r0
	str	r0, [sp, #16]                   @ 4-byte Spill
	ldr	r0, [sp, #24]                   @ 4-byte Reload
	adcs	r11, r12, r11
	str	r1, [r4, #8]
	mov	r1, r5
	adcs	r0, lr, r0
	str	r0, [sp, #24]                   @ 4-byte Spill
	ldr	r0, [sp, #28]                   @ 4-byte Reload
	adcs	r0, r6, r0
	str	r0, [sp, #28]                   @ 4-byte Spill
	ldr	r0, [sp, #20]                   @ 4-byte Reload
	adcs	r0, r7, r0
	str	r0, [sp, #20]                   @ 4-byte Spill
	adc	r0, r8, #0
	str	r0, [sp, #12]                   @ 4-byte Spill
	add	r0, sp, #192
	bl	mulPv256x32
	add	r6, sp, #196
	ldr	r7, [sp, #192]
	add	lr, sp, #212
	ldr	r8, [sp, #224]
	ldm	r6, {r0, r1, r2, r6}
	adds	r7, r7, r9
	adcs	r9, r0, r10
	ldr	r0, [sp, #16]                   @ 4-byte Reload
	ldm	lr, {r3, r12, lr}
	adcs	r0, r1, r0
	str	r0, [sp, #16]                   @ 4-byte Spill
	adcs	r10, r2, r11
	ldr	r0, [sp, #24]                   @ 4-byte Reload
	mov	r1, r5
	ldr	r2, [r5, #16]
	adcs	r0, r6, r0
	str	r0, [sp, #24]                   @ 4-byte Spill
	ldr	r0, [sp, #28]                   @ 4-byte Reload
	str	r7, [r4, #12]
	adcs	r11, r3, r0
	ldr	r0, [sp, #20]                   @ 4-byte Reload
	adcs	r0, r12, r0
	str	r0, [sp, #28]                   @ 4-byte Spill
	ldr	r0, [sp, #12]                   @ 4-byte Reload
	adcs	r0, lr, r0
	str	r0, [sp, #20]                   @ 4-byte Spill
	adc	r0, r8, #0
	str	r0, [sp, #12]                   @ 4-byte Spill
	add	r0, sp, #152
	bl	mulPv256x32
	ldr	r1, [sp, #152]
	add	r8, sp, #176
	add	lr, sp, #156
	adds	r1, r1, r9
	ldm	r8, {r6, r7, r8}
	ldm	lr, {r0, r2, r3, r12, lr}
	str	r1, [r4, #16]
	ldr	r1, [sp, #16]                   @ 4-byte Reload
	adcs	r9, r0, r1
	ldr	r0, [sp, #24]                   @ 4-byte Reload
	adcs	r10, r2, r10
	ldr	r2, [r5, #20]
	adcs	r0, r3, r0
	str	r0, [sp, #24]                   @ 4-byte Spill
	ldr	r0, [sp, #28]                   @ 4-byte Reload
	adcs	r11, r12, r11
	mov	r1, r5
	adcs	r0, lr, r0
	str	r0, [sp, #16]                   @ 4-byte Spill
	ldr	r0, [sp, #20]                   @ 4-byte Reload
	adcs	r0, r6, r0
	str	r0, [sp, #20]                   @ 4-byte Spill
	ldr	r0, [sp, #12]                   @ 4-byte Reload
	adcs	r0, r7, r0
	str	r0, [sp, #12]                   @ 4-byte Spill
	adc	r0, r8, #0
	str	r0, [sp, #8]                    @ 4-byte Spill
	add	r0, sp, #112
	bl	mulPv256x32
	add	r6, sp, #116
	ldr	r7, [sp, #112]
	add	lr, sp, #132
	ldr	r8, [sp, #144]
	ldm	r6, {r0, r1, r2, r6}
	adds	r7, r7, r9
	adcs	r9, r0, r10
	ldr	r0, [sp, #24]                   @ 4-byte Reload
	ldm	lr, {r3, r12, lr}
	adcs	r0, r1, r0
	str	r0, [sp, #28]                   @ 4-byte Spill
	adcs	r10, r2, r11
	ldr	r0, [sp, #16]                   @ 4-byte Reload
	mov	r1, r5
	ldr	r2, [r5, #24]
	adcs	r11, r6, r0
	ldr	r0, [sp, #20]                   @ 4-byte Reload
	str	r7, [r4, #20]
	adcs	r0, r3, r0
	str	r0, [sp, #24]                   @ 4-byte Spill
	ldr	r0, [sp, #12]                   @ 4-byte Reload
	adcs	r0, r12, r0
	str	r0, [sp, #20]                   @ 4-byte Spill
	ldr	r0, [sp, #8]                    @ 4-byte Reload
	adcs	r0, lr, r0
	str	r0, [sp, #16]                   @ 4-byte Spill
	adc	r0, r8, #0
	str	r0, [sp, #12]                   @ 4-byte Spill
	add	r0, sp, #72
	bl	mulPv256x32
	ldr	r0, [sp, #104]
	add	lr, sp, #76
	str	r0, [sp, #8]                    @ 4-byte Spill
	ldr	r0, [sp, #100]
	str	r0, [sp, #4]                    @ 4-byte Spill
	ldr	r2, [sp, #72]
	ldm	lr, {r0, r1, r3, r12, lr}
	adds	r2, r2, r9
	ldr	r7, [sp, #28]                   @ 4-byte Reload
	ldr	r6, [sp, #96]
	adcs	r9, r0, r7
	ldr	r0, [sp, #24]                   @ 4-byte Reload
	adcs	r10, r1, r10
	ldr	r1, [sp, #4]                    @ 4-byte Reload
	adcs	r8, r3, r11
	str	r2, [r4, #24]
	adcs	r11, r12, r0
	ldr	r0, [sp, #20]                   @ 4-byte Reload
	ldr	r2, [r5, #28]
	adcs	r0, lr, r0
	str	r0, [sp, #28]                   @ 4-byte Spill
	ldr	r0, [sp, #16]                   @ 4-byte Reload
	adcs	r0, r6, r0
	str	r0, [sp, #24]                   @ 4-byte Spill
	ldr	r0, [sp, #12]                   @ 4-byte Reload
	adcs	r0, r1, r0
	str	r0, [sp, #20]                   @ 4-byte Spill
	ldr	r0, [sp, #8]                    @ 4-byte Reload
	mov	r1, r5
	adc	r0, r0, #0
	str	r0, [sp, #16]                   @ 4-byte Spill
	add	r0, sp, #32
	bl	mulPv256x32
	add	r3, sp, #32
	add	r7, sp, #48
	ldr	r6, [sp, #60]
	ldm	r3, {r0, r1, r2, r3}
	adds	lr, r0, r9
	ldm	r7, {r0, r5, r7}
	adcs	r9, r1, r10
	adcs	r2, r2, r8
	ldr	r1, [sp, #28]                   @ 4-byte Reload
	adcs	r3, r3, r11
	ldr	r12, [sp, #64]
	adcs	r0, r0, r1
	ldr	r1, [sp, #24]                   @ 4-byte Reload
	str	lr, [r4, #28]
	adcs	r5, r5, r1
	ldr	r1, [sp, #20]                   @ 4-byte Reload
	str	r9, [r4, #32]
	adcs	r7, r7, r1
	ldr	r1, [sp, #16]                   @ 4-byte Reload
	str	r2, [r4, #36]
	adcs	r6, r6, r1
	add	r1, r4, #44
	str	r3, [r4, #40]
	stm	r1, {r0, r5, r7}
	adc	r0, r12, #0
	str	r6, [r4, #56]
	str	r0, [r4, #60]
	add	sp, sp, #356
	pop	{r4, r5, r6, r7, r8, r9, r10, r11, lr}
	mov	pc, lr
.Lfunc_end42:
	.size	mcl_fpDbl_sqrPre8L, .Lfunc_end42-mcl_fpDbl_sqrPre8L
	.cantunwind
	.fnend
                                        @ -- End function
	.globl	mcl_fp_mont8L                   @ -- Begin function mcl_fp_mont8L
	.p2align	2
	.type	mcl_fp_mont8L,%function
	.code	32                              @ @mcl_fp_mont8L
mcl_fp_mont8L:
	.fnstart
@ %bb.0:
	.save	{r4, r5, r6, r7, r8, r9, r10, r11, lr}
	push	{r4, r5, r6, r7, r8, r9, r10, r11, lr}
	.pad	#716
	sub	sp, sp, #716
	mov	r7, r2
	ldr	r2, [r2]
	str	r0, [sp, #52]                   @ 4-byte Spill
	add	r0, sp, #672
	ldr	r5, [r3, #-4]
	mov	r4, r3
	str	r5, [sp, #64]                   @ 4-byte Spill
	mov	r11, r1
	str	r3, [sp, #68]                   @ 4-byte Spill
	str	r7, [sp, #60]                   @ 4-byte Spill
	str	r1, [sp, #56]                   @ 4-byte Spill
	bl	mulPv256x32
	ldr	r0, [sp, #676]
	mov	r1, r4
	ldr	r9, [sp, #672]
	str	r0, [sp, #40]                   @ 4-byte Spill
	ldr	r0, [sp, #680]
	str	r0, [sp, #36]                   @ 4-byte Spill
	mul	r2, r5, r9
	ldr	r0, [sp, #684]
	str	r0, [sp, #32]                   @ 4-byte Spill
	ldr	r0, [sp, #704]
	str	r0, [sp, #48]                   @ 4-byte Spill
	ldr	r0, [sp, #700]
	str	r0, [sp, #44]                   @ 4-byte Spill
	ldr	r0, [sp, #696]
	str	r0, [sp, #28]                   @ 4-byte Spill
	ldr	r0, [sp, #692]
	str	r0, [sp, #24]                   @ 4-byte Spill
	ldr	r0, [sp, #688]
	str	r0, [sp, #20]                   @ 4-byte Spill
	add	r0, sp, #632
	bl	mulPv256x32
	ldr	r0, [sp, #664]
	add	r10, sp, #636
	str	r0, [sp, #16]                   @ 4-byte Spill
	mov	r1, r11
	ldr	r0, [sp, #660]
	str	r0, [sp, #12]                   @ 4-byte Spill
	ldr	r0, [sp, #656]
	str	r0, [sp, #8]                    @ 4-byte Spill
	add	r0, sp, #592
	ldr	r2, [r7, #4]
	ldm	r10, {r5, r8, r10}
	ldr	r7, [sp, #652]
	ldr	r4, [sp, #648]
	ldr	r6, [sp, #632]
	bl	mulPv256x32
	adds	r0, r6, r9
	ldr	r2, [sp, #8]                    @ 4-byte Reload
	ldr	r0, [sp, #40]                   @ 4-byte Reload
	mov	r1, #0
	add	r12, sp, #596
	adcs	r0, r5, r0
	str	r0, [sp, #40]                   @ 4-byte Spill
	ldr	r0, [sp, #36]                   @ 4-byte Reload
	ldr	lr, [sp, #40]                   @ 4-byte Reload
	adcs	r0, r8, r0
	str	r0, [sp, #36]                   @ 4-byte Spill
	ldr	r0, [sp, #32]                   @ 4-byte Reload
	ldr	r5, [sp, #36]                   @ 4-byte Reload
	adcs	r10, r10, r0
	ldr	r0, [sp, #20]                   @ 4-byte Reload
	adcs	r0, r4, r0
	str	r0, [sp, #32]                   @ 4-byte Spill
	ldr	r0, [sp, #24]                   @ 4-byte Reload
	adcs	r0, r7, r0
	str	r0, [sp, #24]                   @ 4-byte Spill
	ldr	r0, [sp, #28]                   @ 4-byte Reload
	adcs	r11, r2, r0
	ldr	r0, [sp, #44]                   @ 4-byte Reload
	ldr	r2, [sp, #12]                   @ 4-byte Reload
	adcs	r7, r2, r0
	ldr	r0, [sp, #48]                   @ 4-byte Reload
	ldr	r2, [sp, #16]                   @ 4-byte Reload
	adcs	r0, r2, r0
	str	r0, [sp, #20]                   @ 4-byte Spill
	adc	r0, r1, #0
	str	r0, [sp, #16]                   @ 4-byte Spill
	ldr	r4, [sp, #592]
	ldm	r12, {r0, r1, r2, r3, r6, r12}
	adds	r4, lr, r4
	adcs	r0, r5, r0
	ldr	r8, [sp, #624]
	ldr	r9, [sp, #620]
	str	r0, [sp, #48]                   @ 4-byte Spill
	adcs	r0, r10, r1
	str	r0, [sp, #44]                   @ 4-byte Spill
	ldr	r0, [sp, #32]                   @ 4-byte Reload
	ldr	r1, [sp, #68]                   @ 4-byte Reload
	adcs	r0, r0, r2
	str	r0, [sp, #40]                   @ 4-byte Spill
	ldr	r0, [sp, #24]                   @ 4-byte Reload
	adcs	r0, r0, r3
	str	r0, [sp, #36]                   @ 4-byte Spill
	adcs	r0, r11, r6
	str	r0, [sp, #32]                   @ 4-byte Spill
	adcs	r0, r7, r12
	str	r0, [sp, #28]                   @ 4-byte Spill
	ldr	r0, [sp, #20]                   @ 4-byte Reload
	adcs	r0, r0, r9
	str	r0, [sp, #24]                   @ 4-byte Spill
	ldr	r0, [sp, #16]                   @ 4-byte Reload
	adcs	r0, r0, r8
	str	r0, [sp, #20]                   @ 4-byte Spill
	mov	r0, #0
	adc	r0, r0, #0
	str	r0, [sp, #16]                   @ 4-byte Spill
	ldr	r0, [sp, #64]                   @ 4-byte Reload
	mul	r2, r0, r4
	add	r0, sp, #552
	bl	mulPv256x32
	ldr	r0, [sp, #584]
	str	r0, [sp, #12]                   @ 4-byte Spill
	ldr	r5, [sp, #60]                   @ 4-byte Reload
	ldr	r0, [sp, #580]
	str	r0, [sp, #8]                    @ 4-byte Spill
	ldr	r0, [sp, #576]
	ldr	r2, [r5, #8]
	ldr	r1, [sp, #56]                   @ 4-byte Reload
	str	r0, [sp, #4]                    @ 4-byte Spill
	add	r0, sp, #512
	ldr	r6, [sp, #572]
	ldr	r7, [sp, #568]
	ldr	r10, [sp, #552]
	ldr	r11, [sp, #556]
	ldr	r8, [sp, #560]
	ldr	r9, [sp, #564]
	bl	mulPv256x32
	adds	r0, r4, r10
	ldr	r1, [sp, #4]                    @ 4-byte Reload
	ldr	r0, [sp, #48]                   @ 4-byte Reload
	add	lr, sp, #512
	adcs	r0, r0, r11
	str	r0, [sp, #48]                   @ 4-byte Spill
	ldr	r0, [sp, #44]                   @ 4-byte Reload
	adcs	r10, r0, r8
	ldr	r0, [sp, #40]                   @ 4-byte Reload
	adcs	r11, r0, r9
	ldr	r0, [sp, #36]                   @ 4-byte Reload
	adcs	r0, r0, r7
	str	r0, [sp, #40]                   @ 4-byte Spill
	ldr	r0, [sp, #32]                   @ 4-byte Reload
	adcs	r0, r0, r6
	str	r0, [sp, #36]                   @ 4-byte Spill
	ldr	r0, [sp, #28]                   @ 4-byte Reload
	ldr	r6, [sp, #48]                   @ 4-byte Reload
	adcs	r0, r0, r1
	str	r0, [sp, #32]                   @ 4-byte Spill
	ldr	r0, [sp, #24]                   @ 4-byte Reload
	ldr	r1, [sp, #8]                    @ 4-byte Reload
	adcs	r7, r0, r1
	ldr	r0, [sp, #20]                   @ 4-byte Reload
	ldr	r1, [sp, #12]                   @ 4-byte Reload
	adcs	r0, r0, r1
	str	r0, [sp, #24]                   @ 4-byte Spill
	ldr	r0, [sp, #16]                   @ 4-byte Reload
	adc	r0, r0, #0
	str	r0, [sp, #20]                   @ 4-byte Spill
	ldm	lr, {r0, r1, r2, r3, r12, lr}
	adds	r6, r6, r0
	ldr	r8, [sp, #544]
	adcs	r0, r10, r1
	ldr	r9, [sp, #540]
	ldr	r4, [sp, #536]
	str	r0, [sp, #48]                   @ 4-byte Spill
	adcs	r0, r11, r2
	str	r0, [sp, #44]                   @ 4-byte Spill
	ldr	r0, [sp, #40]                   @ 4-byte Reload
	ldr	r1, [sp, #68]                   @ 4-byte Reload
	adcs	r0, r0, r3
	str	r0, [sp, #40]                   @ 4-byte Spill
	ldr	r0, [sp, #36]                   @ 4-byte Reload
	adcs	r0, r0, r12
	str	r0, [sp, #36]                   @ 4-byte Spill
	ldr	r0, [sp, #32]                   @ 4-byte Reload
	adcs	r0, r0, lr
	str	r0, [sp, #32]                   @ 4-byte Spill
	adcs	r0, r7, r4
	str	r0, [sp, #28]                   @ 4-byte Spill
	ldr	r0, [sp, #24]                   @ 4-byte Reload
	adcs	r0, r0, r9
	str	r0, [sp, #24]                   @ 4-byte Spill
	ldr	r0, [sp, #20]                   @ 4-byte Reload
	adcs	r0, r0, r8
	str	r0, [sp, #20]                   @ 4-byte Spill
	mov	r0, #0
	adc	r0, r0, #0
	str	r0, [sp, #16]                   @ 4-byte Spill
	ldr	r0, [sp, #64]                   @ 4-byte Reload
	mul	r2, r0, r6
	add	r0, sp, #472
	bl	mulPv256x32
	ldr	r0, [sp, #504]
	str	r0, [sp, #12]                   @ 4-byte Spill
	ldr	r0, [sp, #500]
	str	r0, [sp, #8]                    @ 4-byte Spill
	ldr	r2, [r5, #12]
	ldr	r5, [sp, #56]                   @ 4-byte Reload
	ldr	r0, [sp, #496]
	str	r0, [sp, #4]                    @ 4-byte Spill
	add	r0, sp, #432
	mov	r1, r5
	ldr	r4, [sp, #492]
	ldr	r7, [sp, #488]
	ldr	r10, [sp, #472]
	ldr	r11, [sp, #476]
	ldr	r8, [sp, #480]
	ldr	r9, [sp, #484]
	bl	mulPv256x32
	adds	r0, r6, r10
	ldr	r1, [sp, #4]                    @ 4-byte Reload
	ldr	r0, [sp, #48]                   @ 4-byte Reload
	add	lr, sp, #436
	adcs	r10, r0, r11
	ldr	r0, [sp, #44]                   @ 4-byte Reload
	adcs	r11, r0, r8
	ldr	r0, [sp, #40]                   @ 4-byte Reload
	adcs	r0, r0, r9
	str	r0, [sp, #44]                   @ 4-byte Spill
	ldr	r0, [sp, #36]                   @ 4-byte Reload
	adcs	r0, r0, r7
	str	r0, [sp, #40]                   @ 4-byte Spill
	ldr	r0, [sp, #32]                   @ 4-byte Reload
	adcs	r0, r0, r4
	str	r0, [sp, #36]                   @ 4-byte Spill
	ldr	r0, [sp, #28]                   @ 4-byte Reload
	adcs	r6, r0, r1
	ldr	r0, [sp, #24]                   @ 4-byte Reload
	ldr	r1, [sp, #8]                    @ 4-byte Reload
	adcs	r7, r0, r1
	ldr	r0, [sp, #20]                   @ 4-byte Reload
	ldr	r1, [sp, #12]                   @ 4-byte Reload
	adcs	r0, r0, r1
	str	r0, [sp, #24]                   @ 4-byte Spill
	ldr	r0, [sp, #16]                   @ 4-byte Reload
	adc	r0, r0, #0
	str	r0, [sp, #20]                   @ 4-byte Spill
	ldr	r4, [sp, #432]
	ldm	lr, {r0, r1, r2, r3, r12, lr}
	adds	r4, r10, r4
	adcs	r0, r11, r0
	ldr	r8, [sp, #464]
	ldr	r9, [sp, #460]
	str	r0, [sp, #48]                   @ 4-byte Spill
	ldr	r0, [sp, #44]                   @ 4-byte Reload
	adcs	r0, r0, r1
	str	r0, [sp, #44]                   @ 4-byte Spill
	ldr	r0, [sp, #40]                   @ 4-byte Reload
	ldr	r1, [sp, #68]                   @ 4-byte Reload
	adcs	r0, r0, r2
	str	r0, [sp, #40]                   @ 4-byte Spill
	ldr	r0, [sp, #36]                   @ 4-byte Reload
	adcs	r0, r0, r3
	str	r0, [sp, #36]                   @ 4-byte Spill
	adcs	r0, r6, r12
	str	r0, [sp, #32]                   @ 4-byte Spill
	adcs	r0, r7, lr
	str	r0, [sp, #28]                   @ 4-byte Spill
	ldr	r0, [sp, #24]                   @ 4-byte Reload
	adcs	r0, r0, r9
	str	r0, [sp, #24]                   @ 4-byte Spill
	ldr	r0, [sp, #20]                   @ 4-byte Reload
	adcs	r0, r0, r8
	str	r0, [sp, #20]                   @ 4-byte Spill
	mov	r0, #0
	adc	r0, r0, #0
	str	r0, [sp, #16]                   @ 4-byte Spill
	ldr	r0, [sp, #64]                   @ 4-byte Reload
	mul	r2, r0, r4
	add	r0, sp, #392
	bl	mulPv256x32
	ldr	r0, [sp, #60]                   @ 4-byte Reload
	mov	r1, r5
	ldr	r2, [r0, #16]
	ldr	r0, [sp, #424]
	str	r0, [sp, #12]                   @ 4-byte Spill
	ldr	r0, [sp, #420]
	str	r0, [sp, #8]                    @ 4-byte Spill
	ldr	r0, [sp, #416]
	str	r0, [sp, #4]                    @ 4-byte Spill
	add	r0, sp, #352
	ldr	r6, [sp, #412]
	ldr	r7, [sp, #408]
	ldr	r10, [sp, #392]
	ldr	r11, [sp, #396]
	ldr	r8, [sp, #400]
	ldr	r9, [sp, #404]
	bl	mulPv256x32
	adds	r0, r4, r10
	ldr	r1, [sp, #4]                    @ 4-byte Reload
	ldr	r0, [sp, #48]                   @ 4-byte Reload
	add	lr, sp, #352
	adcs	r5, r0, r11
	ldr	r0, [sp, #44]                   @ 4-byte Reload
	adcs	r10, r0, r8
	ldr	r0, [sp, #40]                   @ 4-byte Reload
	adcs	r11, r0, r9
	ldr	r0, [sp, #36]                   @ 4-byte Reload
	adcs	r0, r0, r7
	str	r0, [sp, #40]                   @ 4-byte Spill
	ldr	r0, [sp, #32]                   @ 4-byte Reload
	adcs	r0, r0, r6
	str	r0, [sp, #36]                   @ 4-byte Spill
	ldr	r0, [sp, #28]                   @ 4-byte Reload
	adcs	r0, r0, r1
	str	r0, [sp, #32]                   @ 4-byte Spill
	ldr	r0, [sp, #24]                   @ 4-byte Reload
	ldr	r1, [sp, #8]                    @ 4-byte Reload
	adcs	r7, r0, r1
	ldr	r0, [sp, #20]                   @ 4-byte Reload
	ldr	r1, [sp, #12]                   @ 4-byte Reload
	adcs	r0, r0, r1
	str	r0, [sp, #24]                   @ 4-byte Spill
	ldr	r0, [sp, #16]                   @ 4-byte Reload
	adc	r0, r0, #0
	str	r0, [sp, #20]                   @ 4-byte Spill
	ldm	lr, {r0, r1, r2, r3, r12, lr}
	adds	r6, r5, r0
	ldr	r8, [sp, #384]
	adcs	r0, r10, r1
	ldr	r9, [sp, #380]
	ldr	r4, [sp, #376]
	str	r0, [sp, #48]                   @ 4-byte Spill
	adcs	r0, r11, r2
	str	r0, [sp, #44]                   @ 4-byte Spill
	ldr	r0, [sp, #40]                   @ 4-byte Reload
	ldr	r5, [sp, #64]                   @ 4-byte Reload
	adcs	r0, r0, r3
	str	r0, [sp, #40]                   @ 4-byte Spill
	ldr	r0, [sp, #36]                   @ 4-byte Reload
	mul	r2, r5, r6
	ldr	r1, [sp, #68]                   @ 4-byte Reload
	adcs	r0, r0, r12
	str	r0, [sp, #36]                   @ 4-byte Spill
	ldr	r0, [sp, #32]                   @ 4-byte Reload
	adcs	r0, r0, lr
	str	r0, [sp, #32]                   @ 4-byte Spill
	adcs	r0, r7, r4
	str	r0, [sp, #28]                   @ 4-byte Spill
	ldr	r0, [sp, #24]                   @ 4-byte Reload
	adcs	r0, r0, r9
	str	r0, [sp, #24]                   @ 4-byte Spill
	ldr	r0, [sp, #20]                   @ 4-byte Reload
	adcs	r0, r0, r8
	str	r0, [sp, #20]                   @ 4-byte Spill
	mov	r0, #0
	adc	r0, r0, #0
	str	r0, [sp, #16]                   @ 4-byte Spill
	add	r0, sp, #312
	bl	mulPv256x32
	ldr	r0, [sp, #60]                   @ 4-byte Reload
	ldr	r1, [sp, #56]                   @ 4-byte Reload
	ldr	r2, [r0, #20]
	ldr	r0, [sp, #344]
	str	r0, [sp, #12]                   @ 4-byte Spill
	ldr	r0, [sp, #340]
	str	r0, [sp, #8]                    @ 4-byte Spill
	ldr	r0, [sp, #336]
	str	r0, [sp, #4]                    @ 4-byte Spill
	add	r0, sp, #272
	ldr	r4, [sp, #332]
	ldr	r7, [sp, #328]
	ldr	r10, [sp, #312]
	ldr	r11, [sp, #316]
	ldr	r8, [sp, #320]
	ldr	r9, [sp, #324]
	bl	mulPv256x32
	adds	r0, r6, r10
	ldr	r1, [sp, #4]                    @ 4-byte Reload
	ldr	r0, [sp, #48]                   @ 4-byte Reload
	add	lr, sp, #276
	adcs	r10, r0, r11
	ldr	r0, [sp, #44]                   @ 4-byte Reload
	adcs	r11, r0, r8
	ldr	r0, [sp, #40]                   @ 4-byte Reload
	adcs	r0, r0, r9
	str	r0, [sp, #44]                   @ 4-byte Spill
	ldr	r0, [sp, #36]                   @ 4-byte Reload
	adcs	r0, r0, r7
	str	r0, [sp, #40]                   @ 4-byte Spill
	ldr	r0, [sp, #32]                   @ 4-byte Reload
	adcs	r0, r0, r4
	str	r0, [sp, #36]                   @ 4-byte Spill
	ldr	r0, [sp, #28]                   @ 4-byte Reload
	adcs	r6, r0, r1
	ldr	r0, [sp, #24]                   @ 4-byte Reload
	ldr	r1, [sp, #8]                    @ 4-byte Reload
	adcs	r7, r0, r1
	ldr	r0, [sp, #20]                   @ 4-byte Reload
	ldr	r1, [sp, #12]                   @ 4-byte Reload
	adcs	r0, r0, r1
	str	r0, [sp, #24]                   @ 4-byte Spill
	ldr	r0, [sp, #16]                   @ 4-byte Reload
	adc	r0, r0, #0
	str	r0, [sp, #20]                   @ 4-byte Spill
	ldr	r4, [sp, #272]
	ldm	lr, {r0, r1, r2, r3, r12, lr}
	adds	r4, r10, r4
	adcs	r0, r11, r0
	ldr	r8, [sp, #304]
	ldr	r9, [sp, #300]
	str	r0, [sp, #48]                   @ 4-byte Spill
	ldr	r0, [sp, #44]                   @ 4-byte Reload
	adcs	r0, r0, r1
	str	r0, [sp, #44]                   @ 4-byte Spill
	ldr	r0, [sp, #40]                   @ 4-byte Reload
	adcs	r0, r0, r2
	str	r0, [sp, #40]                   @ 4-byte Spill
	ldr	r0, [sp, #36]                   @ 4-byte Reload
	mul	r2, r5, r4
	ldr	r5, [sp, #68]                   @ 4-byte Reload
	adcs	r0, r0, r3
	str	r0, [sp, #36]                   @ 4-byte Spill
	adcs	r0, r6, r12
	str	r0, [sp, #32]                   @ 4-byte Spill
	adcs	r0, r7, lr
	str	r0, [sp, #28]                   @ 4-byte Spill
	ldr	r0, [sp, #24]                   @ 4-byte Reload
	mov	r1, r5
	adcs	r0, r0, r9
	str	r0, [sp, #24]                   @ 4-byte Spill
	ldr	r0, [sp, #20]                   @ 4-byte Reload
	adcs	r0, r0, r8
	str	r0, [sp, #20]                   @ 4-byte Spill
	mov	r0, #0
	adc	r0, r0, #0
	str	r0, [sp, #16]                   @ 4-byte Spill
	add	r0, sp, #232
	bl	mulPv256x32
	ldr	r0, [sp, #60]                   @ 4-byte Reload
	ldr	r1, [sp, #56]                   @ 4-byte Reload
	ldr	r2, [r0, #24]
	ldr	r0, [sp, #264]
	str	r0, [sp, #12]                   @ 4-byte Spill
	ldr	r0, [sp, #260]
	str	r0, [sp, #8]                    @ 4-byte Spill
	ldr	r0, [sp, #256]
	str	r0, [sp, #4]                    @ 4-byte Spill
	add	r0, sp, #192
	ldr	r6, [sp, #252]
	ldr	r7, [sp, #248]
	ldr	r10, [sp, #232]
	ldr	r11, [sp, #236]
	ldr	r8, [sp, #240]
	ldr	r9, [sp, #244]
	bl	mulPv256x32
	adds	r0, r4, r10
	ldr	r1, [sp, #4]                    @ 4-byte Reload
	ldr	r0, [sp, #48]                   @ 4-byte Reload
	add	lr, sp, #192
	adcs	r0, r0, r11
	str	r0, [sp, #48]                   @ 4-byte Spill
	ldr	r0, [sp, #44]                   @ 4-byte Reload
	adcs	r10, r0, r8
	ldr	r0, [sp, #40]                   @ 4-byte Reload
	adcs	r11, r0, r9
	ldr	r0, [sp, #36]                   @ 4-byte Reload
	adcs	r0, r0, r7
	str	r0, [sp, #40]                   @ 4-byte Spill
	ldr	r0, [sp, #32]                   @ 4-byte Reload
	adcs	r0, r0, r6
	str	r0, [sp, #36]                   @ 4-byte Spill
	ldr	r0, [sp, #28]                   @ 4-byte Reload
	ldr	r6, [sp, #48]                   @ 4-byte Reload
	adcs	r0, r0, r1
	str	r0, [sp, #32]                   @ 4-byte Spill
	ldr	r0, [sp, #24]                   @ 4-byte Reload
	ldr	r1, [sp, #8]                    @ 4-byte Reload
	adcs	r7, r0, r1
	ldr	r0, [sp, #20]                   @ 4-byte Reload
	ldr	r1, [sp, #12]                   @ 4-byte Reload
	adcs	r0, r0, r1
	str	r0, [sp, #24]                   @ 4-byte Spill
	ldr	r0, [sp, #16]                   @ 4-byte Reload
	adc	r0, r0, #0
	str	r0, [sp, #20]                   @ 4-byte Spill
	ldm	lr, {r0, r1, r2, r3, r12, lr}
	adds	r6, r6, r0
	ldr	r8, [sp, #224]
	adcs	r0, r10, r1
	ldr	r9, [sp, #220]
	ldr	r4, [sp, #216]
	mov	r1, r5
	str	r0, [sp, #48]                   @ 4-byte Spill
	adcs	r0, r11, r2
	str	r0, [sp, #44]                   @ 4-byte Spill
	ldr	r0, [sp, #40]                   @ 4-byte Reload
	adcs	r0, r0, r3
	str	r0, [sp, #40]                   @ 4-byte Spill
	ldr	r0, [sp, #36]                   @ 4-byte Reload
	adcs	r0, r0, r12
	str	r0, [sp, #36]                   @ 4-byte Spill
	ldr	r0, [sp, #32]                   @ 4-byte Reload
	adcs	r0, r0, lr
	str	r0, [sp, #32]                   @ 4-byte Spill
	adcs	r0, r7, r4
	str	r0, [sp, #28]                   @ 4-byte Spill
	ldr	r0, [sp, #24]                   @ 4-byte Reload
	adcs	r0, r0, r9
	str	r0, [sp, #24]                   @ 4-byte Spill
	ldr	r0, [sp, #20]                   @ 4-byte Reload
	adcs	r0, r0, r8
	str	r0, [sp, #20]                   @ 4-byte Spill
	mov	r0, #0
	adc	r0, r0, #0
	str	r0, [sp, #16]                   @ 4-byte Spill
	ldr	r0, [sp, #64]                   @ 4-byte Reload
	mul	r2, r0, r6
	add	r0, sp, #152
	bl	mulPv256x32
	ldr	r0, [sp, #60]                   @ 4-byte Reload
	ldr	r1, [sp, #56]                   @ 4-byte Reload
	ldr	r2, [r0, #28]
	ldr	r0, [sp, #184]
	str	r0, [sp, #60]                   @ 4-byte Spill
	ldr	r0, [sp, #180]
	str	r0, [sp, #12]                   @ 4-byte Spill
	add	r0, sp, #112
	ldr	r5, [sp, #176]
	ldr	r4, [sp, #172]
	ldr	r7, [sp, #168]
	ldr	r10, [sp, #152]
	ldr	r11, [sp, #156]
	ldr	r8, [sp, #160]
	ldr	r9, [sp, #164]
	bl	mulPv256x32
	adds	r0, r6, r10
	ldr	r1, [sp, #44]                   @ 4-byte Reload
	ldr	r0, [sp, #48]                   @ 4-byte Reload
	add	lr, sp, #128
	ldr	r2, [sp, #12]                   @ 4-byte Reload
	adcs	r0, r0, r11
	adcs	r8, r1, r8
	ldr	r1, [sp, #40]                   @ 4-byte Reload
	adcs	r9, r1, r9
	ldr	r1, [sp, #36]                   @ 4-byte Reload
	adcs	r1, r1, r7
	str	r1, [sp, #56]                   @ 4-byte Spill
	ldr	r1, [sp, #32]                   @ 4-byte Reload
	adcs	r11, r1, r4
	ldr	r1, [sp, #28]                   @ 4-byte Reload
	adcs	r1, r1, r5
	str	r1, [sp, #48]                   @ 4-byte Spill
	ldr	r1, [sp, #24]                   @ 4-byte Reload
	adcs	r10, r1, r2
	ldr	r1, [sp, #20]                   @ 4-byte Reload
	ldr	r2, [sp, #60]                   @ 4-byte Reload
	adcs	r1, r1, r2
	str	r1, [sp, #60]                   @ 4-byte Spill
	ldr	r1, [sp, #16]                   @ 4-byte Reload
	adc	r1, r1, #0
	str	r1, [sp, #44]                   @ 4-byte Spill
	ldr	r2, [sp, #112]
	ldr	r12, [sp, #116]
	adds	r5, r0, r2
	ldr	r0, [sp, #64]                   @ 4-byte Reload
	ldr	r3, [sp, #120]
	adcs	r8, r8, r12
	ldr	r4, [sp, #124]
	adcs	r9, r9, r3
	ldr	r3, [sp, #56]                   @ 4-byte Reload
	mul	r7, r0, r5
	ldr	r6, [sp, #144]
	adcs	r4, r3, r4
	ldm	lr, {r0, r1, r2, lr}
	adcs	r11, r11, r0
	ldr	r0, [sp, #48]                   @ 4-byte Reload
	adcs	r0, r0, r1
	str	r0, [sp, #36]                   @ 4-byte Spill
	ldr	r0, [sp, #60]                   @ 4-byte Reload
	adcs	r10, r10, r2
	mov	r2, r7
	adcs	r0, r0, lr
	str	r0, [sp, #40]                   @ 4-byte Spill
	ldr	r0, [sp, #44]                   @ 4-byte Reload
	adcs	r0, r0, r6
	str	r0, [sp, #56]                   @ 4-byte Spill
	mov	r0, #0
	ldr	r6, [sp, #68]                   @ 4-byte Reload
	adc	r0, r0, #0
	str	r0, [sp, #48]                   @ 4-byte Spill
	add	r0, sp, #72
	mov	r1, r6
	bl	mulPv256x32
	add	r3, sp, #72
	ldm	r3, {r0, r1, r2, r3}
	adds	r0, r5, r0
	adcs	r1, r8, r1
	str	r1, [sp, #64]                   @ 4-byte Spill
	adcs	r9, r9, r2
	str	r9, [sp, #60]                   @ 4-byte Spill
	adcs	lr, r4, r3
	str	lr, [sp, #44]                   @ 4-byte Spill
	ldr	r3, [sp, #88]
	ldr	r0, [sp, #36]                   @ 4-byte Reload
	adcs	r2, r11, r3
	str	r2, [sp, #32]                   @ 4-byte Spill
	ldr	r7, [sp, #92]
	ldr	r5, [sp, #96]
	adcs	r12, r0, r7
	ldr	r3, [sp, #56]                   @ 4-byte Reload
	adcs	r0, r10, r5
	str	r0, [sp, #36]                   @ 4-byte Spill
	ldr	r4, [sp, #100]
	ldr	r0, [sp, #40]                   @ 4-byte Reload
	adcs	r0, r0, r4
	str	r0, [sp, #40]                   @ 4-byte Spill
	ldr	r0, [sp, #104]
	mov	r4, r6
	ldr	r6, [r6]
	add	r11, r4, #8
	adcs	r0, r3, r0
	ldr	r3, [sp, #48]                   @ 4-byte Reload
	adc	r3, r3, #0
	str	r3, [sp, #56]                   @ 4-byte Spill
	ldr	r3, [r4, #4]
	subs	r8, r1, r6
	str	r3, [sp, #48]                   @ 4-byte Spill
	ldr	r1, [sp, #48]                   @ 4-byte Reload
	ldm	r11, {r3, r5, r7, r10, r11}
	sbcs	r6, r9, r1
	sbcs	r3, lr, r3
	mov	r9, r12
	sbcs	r1, r2, r5
	ldr	r2, [sp, #36]                   @ 4-byte Reload
	sbcs	r7, r12, r7
	ldr	r4, [r4, #28]
	sbcs	r12, r2, r10
	ldr	r10, [sp, #40]                  @ 4-byte Reload
	ldr	r5, [sp, #56]                   @ 4-byte Reload
	sbcs	lr, r10, r11
	sbcs	r4, r0, r4
	sbc	r5, r5, #0
	ands	r5, r5, #1
	movne	r12, r2
	ldr	r2, [sp, #32]                   @ 4-byte Reload
	movne	r4, r0
	ldr	r0, [sp, #52]                   @ 4-byte Reload
	movne	lr, r10
	cmp	r5, #0
	movne	r1, r2
	movne	r7, r9
	str	r1, [r0, #12]
	ldr	r1, [sp, #44]                   @ 4-byte Reload
	str	r4, [r0, #28]
	str	lr, [r0, #24]
	movne	r3, r1
	ldr	r1, [sp, #60]                   @ 4-byte Reload
	cmp	r5, #0
	str	r12, [r0, #20]
	str	r7, [r0, #16]
	movne	r6, r1
	ldr	r1, [sp, #64]                   @ 4-byte Reload
	str	r3, [r0, #8]
	str	r6, [r0, #4]
	movne	r8, r1
	str	r8, [r0]
	add	sp, sp, #716
	pop	{r4, r5, r6, r7, r8, r9, r10, r11, lr}
	mov	pc, lr
.Lfunc_end43:
	.size	mcl_fp_mont8L, .Lfunc_end43-mcl_fp_mont8L
	.cantunwind
	.fnend
                                        @ -- End function
	.globl	mcl_fp_montNF8L                 @ -- Begin function mcl_fp_montNF8L
	.p2align	2
	.type	mcl_fp_montNF8L,%function
	.code	32                              @ @mcl_fp_montNF8L
mcl_fp_montNF8L:
	.fnstart
@ %bb.0:
	.save	{r4, r5, r6, r7, r8, r9, r10, r11, lr}
	push	{r4, r5, r6, r7, r8, r9, r10, r11, lr}
	.pad	#716
	sub	sp, sp, #716
	mov	r7, r2
	ldr	r2, [r2]
	str	r0, [sp, #52]                   @ 4-byte Spill
	add	r0, sp, #672
	ldr	r5, [r3, #-4]
	mov	r4, r3
	str	r5, [sp, #60]                   @ 4-byte Spill
	mov	r6, r7
	str	r7, [sp, #64]                   @ 4-byte Spill
	mov	r7, r1
	str	r3, [sp, #56]                   @ 4-byte Spill
	str	r1, [sp, #68]                   @ 4-byte Spill
	bl	mulPv256x32
	ldr	r0, [sp, #676]
	mov	r1, r4
	ldr	r10, [sp, #672]
	str	r0, [sp, #40]                   @ 4-byte Spill
	ldr	r0, [sp, #680]
	str	r0, [sp, #36]                   @ 4-byte Spill
	mul	r2, r5, r10
	ldr	r0, [sp, #684]
	str	r0, [sp, #32]                   @ 4-byte Spill
	ldr	r0, [sp, #704]
	str	r0, [sp, #48]                   @ 4-byte Spill
	ldr	r0, [sp, #700]
	str	r0, [sp, #44]                   @ 4-byte Spill
	ldr	r0, [sp, #696]
	str	r0, [sp, #28]                   @ 4-byte Spill
	ldr	r0, [sp, #692]
	str	r0, [sp, #24]                   @ 4-byte Spill
	ldr	r0, [sp, #688]
	str	r0, [sp, #20]                   @ 4-byte Spill
	add	r0, sp, #632
	bl	mulPv256x32
	ldr	r0, [sp, #664]
	add	r11, sp, #640
	str	r0, [sp, #16]                   @ 4-byte Spill
	mov	r1, r7
	ldr	r0, [sp, #660]
	str	r0, [sp, #12]                   @ 4-byte Spill
	ldr	r0, [sp, #656]
	str	r0, [sp, #8]                    @ 4-byte Spill
	add	r0, sp, #592
	ldr	r2, [r6, #4]
	ldm	r11, {r5, r9, r11}
	ldr	r4, [sp, #652]
	ldr	r6, [sp, #632]
	ldr	r8, [sp, #636]
	bl	mulPv256x32
	adds	r0, r6, r10
	ldr	r1, [sp, #8]                    @ 4-byte Reload
	ldr	r0, [sp, #40]                   @ 4-byte Reload
	add	r6, sp, #596
	adcs	r0, r8, r0
	str	r0, [sp, #40]                   @ 4-byte Spill
	ldr	r0, [sp, #36]                   @ 4-byte Reload
	ldr	lr, [sp, #40]                   @ 4-byte Reload
	adcs	r0, r5, r0
	str	r0, [sp, #36]                   @ 4-byte Spill
	ldr	r0, [sp, #32]                   @ 4-byte Reload
	ldr	r5, [sp, #36]                   @ 4-byte Reload
	adcs	r9, r9, r0
	ldr	r0, [sp, #20]                   @ 4-byte Reload
	adcs	r11, r11, r0
	ldr	r0, [sp, #24]                   @ 4-byte Reload
	adcs	r0, r4, r0
	str	r0, [sp, #32]                   @ 4-byte Spill
	ldr	r0, [sp, #28]                   @ 4-byte Reload
	adcs	r10, r1, r0
	ldr	r0, [sp, #44]                   @ 4-byte Reload
	ldr	r1, [sp, #12]                   @ 4-byte Reload
	adcs	r7, r1, r0
	ldr	r0, [sp, #48]                   @ 4-byte Reload
	ldr	r1, [sp, #16]                   @ 4-byte Reload
	adc	r0, r1, r0
	str	r0, [sp, #24]                   @ 4-byte Spill
	ldr	r0, [sp, #624]
	str	r0, [sp, #20]                   @ 4-byte Spill
	ldr	r4, [sp, #592]
	ldm	r6, {r0, r1, r2, r6}
	adds	r4, lr, r4
	adcs	r0, r5, r0
	ldr	r8, [sp, #620]
	ldr	r12, [sp, #616]
	ldr	r3, [sp, #612]
	str	r0, [sp, #48]                   @ 4-byte Spill
	adcs	r0, r9, r1
	str	r0, [sp, #44]                   @ 4-byte Spill
	adcs	r0, r11, r2
	str	r0, [sp, #40]                   @ 4-byte Spill
	ldr	r0, [sp, #32]                   @ 4-byte Reload
	ldr	r1, [sp, #56]                   @ 4-byte Reload
	adcs	r0, r0, r6
	str	r0, [sp, #36]                   @ 4-byte Spill
	adcs	r0, r10, r3
	str	r0, [sp, #32]                   @ 4-byte Spill
	adcs	r0, r7, r12
	ldr	r7, [sp, #60]                   @ 4-byte Reload
	str	r0, [sp, #28]                   @ 4-byte Spill
	ldr	r0, [sp, #24]                   @ 4-byte Reload
	mul	r2, r7, r4
	adcs	r0, r0, r8
	str	r0, [sp, #24]                   @ 4-byte Spill
	ldr	r0, [sp, #20]                   @ 4-byte Reload
	adc	r0, r0, #0
	str	r0, [sp, #20]                   @ 4-byte Spill
	add	r0, sp, #552
	bl	mulPv256x32
	ldr	r0, [sp, #64]                   @ 4-byte Reload
	ldr	r5, [sp, #68]                   @ 4-byte Reload
	ldr	r2, [r0, #8]
	ldr	r0, [sp, #584]
	mov	r1, r5
	str	r0, [sp, #16]                   @ 4-byte Spill
	ldr	r0, [sp, #580]
	str	r0, [sp, #12]                   @ 4-byte Spill
	ldr	r0, [sp, #576]
	str	r0, [sp, #8]                    @ 4-byte Spill
	ldr	r0, [sp, #572]
	str	r0, [sp, #4]                    @ 4-byte Spill
	add	r0, sp, #512
	ldr	r9, [sp, #568]
	ldr	r10, [sp, #552]
	ldr	r11, [sp, #556]
	ldr	r8, [sp, #560]
	ldr	r6, [sp, #564]
	bl	mulPv256x32
	adds	r0, r4, r10
	ldr	r1, [sp, #4]                    @ 4-byte Reload
	ldr	r0, [sp, #48]                   @ 4-byte Reload
	add	lr, sp, #516
	adcs	r0, r0, r11
	str	r0, [sp, #48]                   @ 4-byte Spill
	ldr	r0, [sp, #44]                   @ 4-byte Reload
	adcs	r10, r0, r8
	ldr	r0, [sp, #40]                   @ 4-byte Reload
	adcs	r11, r0, r6
	ldr	r0, [sp, #36]                   @ 4-byte Reload
	ldr	r6, [sp, #48]                   @ 4-byte Reload
	adcs	r0, r0, r9
	str	r0, [sp, #40]                   @ 4-byte Spill
	ldr	r0, [sp, #32]                   @ 4-byte Reload
	adcs	r0, r0, r1
	str	r0, [sp, #36]                   @ 4-byte Spill
	ldr	r0, [sp, #28]                   @ 4-byte Reload
	ldr	r1, [sp, #8]                    @ 4-byte Reload
	adcs	r0, r0, r1
	str	r0, [sp, #32]                   @ 4-byte Spill
	ldr	r0, [sp, #24]                   @ 4-byte Reload
	ldr	r1, [sp, #12]                   @ 4-byte Reload
	adcs	r0, r0, r1
	str	r0, [sp, #28]                   @ 4-byte Spill
	ldr	r0, [sp, #20]                   @ 4-byte Reload
	ldr	r1, [sp, #16]                   @ 4-byte Reload
	adc	r0, r0, r1
	str	r0, [sp, #24]                   @ 4-byte Spill
	ldr	r0, [sp, #544]
	str	r0, [sp, #20]                   @ 4-byte Spill
	ldr	r3, [sp, #512]
	ldm	lr, {r0, r1, r2, r12, lr}
	adds	r9, r6, r3
	adcs	r0, r10, r0
	ldr	r8, [sp, #540]
	ldr	r4, [sp, #536]
	str	r0, [sp, #48]                   @ 4-byte Spill
	adcs	r0, r11, r1
	str	r0, [sp, #44]                   @ 4-byte Spill
	ldr	r0, [sp, #40]                   @ 4-byte Reload
	adcs	r0, r0, r2
	str	r0, [sp, #40]                   @ 4-byte Spill
	ldr	r0, [sp, #36]                   @ 4-byte Reload
	mul	r2, r7, r9
	ldr	r7, [sp, #56]                   @ 4-byte Reload
	adcs	r0, r0, r12
	str	r0, [sp, #36]                   @ 4-byte Spill
	ldr	r0, [sp, #32]                   @ 4-byte Reload
	mov	r1, r7
	adcs	r0, r0, lr
	str	r0, [sp, #32]                   @ 4-byte Spill
	ldr	r0, [sp, #28]                   @ 4-byte Reload
	adcs	r0, r0, r4
	str	r0, [sp, #28]                   @ 4-byte Spill
	ldr	r0, [sp, #24]                   @ 4-byte Reload
	adcs	r0, r0, r8
	str	r0, [sp, #24]                   @ 4-byte Spill
	ldr	r0, [sp, #20]                   @ 4-byte Reload
	adc	r0, r0, #0
	str	r0, [sp, #20]                   @ 4-byte Spill
	add	r0, sp, #472
	bl	mulPv256x32
	ldr	r0, [sp, #64]                   @ 4-byte Reload
	mov	r1, r5
	ldr	r2, [r0, #12]
	ldr	r0, [sp, #504]
	str	r0, [sp, #16]                   @ 4-byte Spill
	ldr	r0, [sp, #500]
	str	r0, [sp, #12]                   @ 4-byte Spill
	ldr	r0, [sp, #496]
	str	r0, [sp, #8]                    @ 4-byte Spill
	ldr	r0, [sp, #492]
	str	r0, [sp, #4]                    @ 4-byte Spill
	add	r0, sp, #432
	ldr	r4, [sp, #488]
	ldr	r10, [sp, #472]
	ldr	r11, [sp, #476]
	ldr	r8, [sp, #480]
	ldr	r6, [sp, #484]
	bl	mulPv256x32
	adds	r0, r9, r10
	ldr	r1, [sp, #4]                    @ 4-byte Reload
	ldr	r0, [sp, #48]                   @ 4-byte Reload
	add	lr, sp, #436
	adcs	r5, r0, r11
	ldr	r0, [sp, #44]                   @ 4-byte Reload
	adcs	r10, r0, r8
	ldr	r0, [sp, #40]                   @ 4-byte Reload
	adcs	r11, r0, r6
	ldr	r0, [sp, #36]                   @ 4-byte Reload
	adcs	r0, r0, r4
	str	r0, [sp, #40]                   @ 4-byte Spill
	ldr	r0, [sp, #32]                   @ 4-byte Reload
	adcs	r0, r0, r1
	str	r0, [sp, #36]                   @ 4-byte Spill
	ldr	r0, [sp, #28]                   @ 4-byte Reload
	ldr	r1, [sp, #8]                    @ 4-byte Reload
	adcs	r6, r0, r1
	ldr	r0, [sp, #24]                   @ 4-byte Reload
	ldr	r1, [sp, #12]                   @ 4-byte Reload
	adcs	r0, r0, r1
	str	r0, [sp, #28]                   @ 4-byte Spill
	ldr	r0, [sp, #20]                   @ 4-byte Reload
	ldr	r1, [sp, #16]                   @ 4-byte Reload
	adc	r0, r0, r1
	str	r0, [sp, #24]                   @ 4-byte Spill
	ldr	r0, [sp, #464]
	str	r0, [sp, #20]                   @ 4-byte Spill
	ldr	r3, [sp, #432]
	ldm	lr, {r0, r1, r2, r12, lr}
	adds	r9, r5, r3
	adcs	r0, r10, r0
	ldr	r8, [sp, #460]
	ldr	r4, [sp, #456]
	str	r0, [sp, #48]                   @ 4-byte Spill
	adcs	r0, r11, r1
	str	r0, [sp, #44]                   @ 4-byte Spill
	mov	r1, r7
	ldr	r0, [sp, #40]                   @ 4-byte Reload
	ldr	r5, [sp, #60]                   @ 4-byte Reload
	adcs	r0, r0, r2
	str	r0, [sp, #40]                   @ 4-byte Spill
	ldr	r0, [sp, #36]                   @ 4-byte Reload
	mul	r2, r5, r9
	adcs	r0, r0, r12
	str	r0, [sp, #36]                   @ 4-byte Spill
	adcs	r0, r6, lr
	str	r0, [sp, #32]                   @ 4-byte Spill
	ldr	r0, [sp, #28]                   @ 4-byte Reload
	adcs	r0, r0, r4
	str	r0, [sp, #28]                   @ 4-byte Spill
	ldr	r0, [sp, #24]                   @ 4-byte Reload
	adcs	r0, r0, r8
	str	r0, [sp, #24]                   @ 4-byte Spill
	ldr	r0, [sp, #20]                   @ 4-byte Reload
	adc	r0, r0, #0
	str	r0, [sp, #20]                   @ 4-byte Spill
	add	r0, sp, #392
	bl	mulPv256x32
	ldr	r0, [sp, #64]                   @ 4-byte Reload
	ldr	r1, [sp, #68]                   @ 4-byte Reload
	ldr	r2, [r0, #16]
	ldr	r0, [sp, #424]
	str	r0, [sp, #16]                   @ 4-byte Spill
	ldr	r0, [sp, #420]
	str	r0, [sp, #12]                   @ 4-byte Spill
	ldr	r0, [sp, #416]
	str	r0, [sp, #8]                    @ 4-byte Spill
	ldr	r0, [sp, #412]
	str	r0, [sp, #4]                    @ 4-byte Spill
	add	r0, sp, #352
	ldr	r4, [sp, #408]
	ldr	r10, [sp, #392]
	ldr	r11, [sp, #396]
	ldr	r8, [sp, #400]
	ldr	r6, [sp, #404]
	bl	mulPv256x32
	adds	r0, r9, r10
	ldr	r1, [sp, #4]                    @ 4-byte Reload
	ldr	r0, [sp, #48]                   @ 4-byte Reload
	add	lr, sp, #356
	adcs	r7, r0, r11
	ldr	r0, [sp, #44]                   @ 4-byte Reload
	adcs	r10, r0, r8
	ldr	r0, [sp, #40]                   @ 4-byte Reload
	adcs	r11, r0, r6
	ldr	r0, [sp, #36]                   @ 4-byte Reload
	adcs	r0, r0, r4
	str	r0, [sp, #40]                   @ 4-byte Spill
	ldr	r0, [sp, #32]                   @ 4-byte Reload
	adcs	r0, r0, r1
	str	r0, [sp, #36]                   @ 4-byte Spill
	ldr	r0, [sp, #28]                   @ 4-byte Reload
	ldr	r1, [sp, #8]                    @ 4-byte Reload
	adcs	r6, r0, r1
	ldr	r0, [sp, #24]                   @ 4-byte Reload
	ldr	r1, [sp, #12]                   @ 4-byte Reload
	adcs	r0, r0, r1
	str	r0, [sp, #28]                   @ 4-byte Spill
	ldr	r0, [sp, #20]                   @ 4-byte Reload
	ldr	r1, [sp, #16]                   @ 4-byte Reload
	adc	r0, r0, r1
	str	r0, [sp, #24]                   @ 4-byte Spill
	ldr	r0, [sp, #384]
	str	r0, [sp, #20]                   @ 4-byte Spill
	ldr	r3, [sp, #352]
	ldm	lr, {r0, r1, r2, r12, lr}
	adds	r9, r7, r3
	adcs	r0, r10, r0
	ldr	r8, [sp, #380]
	ldr	r4, [sp, #376]
	str	r0, [sp, #48]                   @ 4-byte Spill
	adcs	r0, r11, r1
	str	r0, [sp, #44]                   @ 4-byte Spill
	ldr	r0, [sp, #40]                   @ 4-byte Reload
	ldr	r1, [sp, #56]                   @ 4-byte Reload
	adcs	r0, r0, r2
	str	r0, [sp, #40]                   @ 4-byte Spill
	ldr	r0, [sp, #36]                   @ 4-byte Reload
	mul	r2, r5, r9
	adcs	r0, r0, r12
	str	r0, [sp, #36]                   @ 4-byte Spill
	adcs	r0, r6, lr
	str	r0, [sp, #32]                   @ 4-byte Spill
	ldr	r0, [sp, #28]                   @ 4-byte Reload
	adcs	r0, r0, r4
	str	r0, [sp, #28]                   @ 4-byte Spill
	ldr	r0, [sp, #24]                   @ 4-byte Reload
	adcs	r0, r0, r8
	str	r0, [sp, #24]                   @ 4-byte Spill
	ldr	r0, [sp, #20]                   @ 4-byte Reload
	adc	r0, r0, #0
	str	r0, [sp, #20]                   @ 4-byte Spill
	add	r0, sp, #312
	bl	mulPv256x32
	ldr	r0, [sp, #344]
	str	r0, [sp, #16]                   @ 4-byte Spill
	ldr	r0, [sp, #340]
	str	r0, [sp, #12]                   @ 4-byte Spill
	ldr	r7, [sp, #64]                   @ 4-byte Reload
	ldr	r0, [sp, #336]
	str	r0, [sp, #8]                    @ 4-byte Spill
	ldr	r0, [sp, #332]
	ldr	r2, [r7, #20]
	ldr	r1, [sp, #68]                   @ 4-byte Reload
	str	r0, [sp, #4]                    @ 4-byte Spill
	add	r0, sp, #272
	ldr	r4, [sp, #328]
	ldr	r10, [sp, #312]
	ldr	r11, [sp, #316]
	ldr	r8, [sp, #320]
	ldr	r6, [sp, #324]
	bl	mulPv256x32
	adds	r0, r9, r10
	ldr	r1, [sp, #4]                    @ 4-byte Reload
	ldr	r0, [sp, #48]                   @ 4-byte Reload
	add	lr, sp, #276
	adcs	r5, r0, r11
	ldr	r0, [sp, #44]                   @ 4-byte Reload
	adcs	r10, r0, r8
	ldr	r0, [sp, #40]                   @ 4-byte Reload
	adcs	r11, r0, r6
	ldr	r0, [sp, #36]                   @ 4-byte Reload
	adcs	r0, r0, r4
	str	r0, [sp, #40]                   @ 4-byte Spill
	ldr	r0, [sp, #32]                   @ 4-byte Reload
	adcs	r0, r0, r1
	str	r0, [sp, #36]                   @ 4-byte Spill
	ldr	r0, [sp, #28]                   @ 4-byte Reload
	ldr	r1, [sp, #8]                    @ 4-byte Reload
	adcs	r6, r0, r1
	ldr	r0, [sp, #24]                   @ 4-byte Reload
	ldr	r1, [sp, #12]                   @ 4-byte Reload
	adcs	r0, r0, r1
	str	r0, [sp, #28]                   @ 4-byte Spill
	ldr	r0, [sp, #20]                   @ 4-byte Reload
	ldr	r1, [sp, #16]                   @ 4-byte Reload
	adc	r0, r0, r1
	str	r0, [sp, #24]                   @ 4-byte Spill
	ldr	r0, [sp, #304]
	str	r0, [sp, #20]                   @ 4-byte Spill
	ldr	r3, [sp, #272]
	ldm	lr, {r0, r1, r2, r12, lr}
	adds	r9, r5, r3
	adcs	r0, r10, r0
	ldr	r8, [sp, #300]
	ldr	r4, [sp, #296]
	str	r0, [sp, #48]                   @ 4-byte Spill
	adcs	r0, r11, r1
	str	r0, [sp, #44]                   @ 4-byte Spill
	ldr	r0, [sp, #40]                   @ 4-byte Reload
	ldr	r5, [sp, #56]                   @ 4-byte Reload
	adcs	r0, r0, r2
	str	r0, [sp, #40]                   @ 4-byte Spill
	ldr	r0, [sp, #36]                   @ 4-byte Reload
	mov	r1, r5
	adcs	r0, r0, r12
	str	r0, [sp, #36]                   @ 4-byte Spill
	adcs	r0, r6, lr
	str	r0, [sp, #32]                   @ 4-byte Spill
	ldr	r0, [sp, #28]                   @ 4-byte Reload
	adcs	r0, r0, r4
	str	r0, [sp, #28]                   @ 4-byte Spill
	ldr	r0, [sp, #24]                   @ 4-byte Reload
	adcs	r0, r0, r8
	str	r0, [sp, #24]                   @ 4-byte Spill
	ldr	r0, [sp, #20]                   @ 4-byte Reload
	adc	r0, r0, #0
	str	r0, [sp, #20]                   @ 4-byte Spill
	ldr	r0, [sp, #60]                   @ 4-byte Reload
	mul	r2, r0, r9
	add	r0, sp, #232
	bl	mulPv256x32
	ldr	r0, [sp, #264]
	str	r0, [sp, #16]                   @ 4-byte Spill
	ldr	r0, [sp, #260]
	str	r0, [sp, #12]                   @ 4-byte Spill
	ldr	r0, [sp, #256]
	str	r0, [sp, #8]                    @ 4-byte Spill
	ldr	r0, [sp, #252]
	ldr	r2, [r7, #24]
	ldr	r1, [sp, #68]                   @ 4-byte Reload
	str	r0, [sp, #4]                    @ 4-byte Spill
	add	r0, sp, #192
	ldr	r4, [sp, #248]
	ldr	r10, [sp, #232]
	ldr	r11, [sp, #236]
	ldr	r8, [sp, #240]
	ldr	r6, [sp, #244]
	bl	mulPv256x32
	adds	r0, r9, r10
	ldr	r1, [sp, #4]                    @ 4-byte Reload
	ldr	r0, [sp, #48]                   @ 4-byte Reload
	add	lr, sp, #196
	adcs	r7, r0, r11
	ldr	r0, [sp, #44]                   @ 4-byte Reload
	adcs	r10, r0, r8
	ldr	r0, [sp, #40]                   @ 4-byte Reload
	adcs	r11, r0, r6
	ldr	r0, [sp, #36]                   @ 4-byte Reload
	adcs	r9, r0, r4
	ldr	r0, [sp, #32]                   @ 4-byte Reload
	adcs	r0, r0, r1
	str	r0, [sp, #36]                   @ 4-byte Spill
	ldr	r0, [sp, #28]                   @ 4-byte Reload
	ldr	r1, [sp, #8]                    @ 4-byte Reload
	adcs	r6, r0, r1
	ldr	r0, [sp, #24]                   @ 4-byte Reload
	ldr	r1, [sp, #12]                   @ 4-byte Reload
	adcs	r0, r0, r1
	str	r0, [sp, #28]                   @ 4-byte Spill
	ldr	r0, [sp, #20]                   @ 4-byte Reload
	ldr	r1, [sp, #16]                   @ 4-byte Reload
	adc	r0, r0, r1
	str	r0, [sp, #24]                   @ 4-byte Spill
	ldr	r0, [sp, #224]
	str	r0, [sp, #20]                   @ 4-byte Spill
	ldr	r3, [sp, #192]
	ldm	lr, {r0, r1, r2, r12, lr}
	adds	r7, r7, r3
	adcs	r0, r10, r0
	ldr	r8, [sp, #220]
	ldr	r4, [sp, #216]
	str	r0, [sp, #48]                   @ 4-byte Spill
	adcs	r0, r11, r1
	str	r0, [sp, #44]                   @ 4-byte Spill
	adcs	r0, r9, r2
	str	r0, [sp, #40]                   @ 4-byte Spill
	mov	r1, r5
	ldr	r0, [sp, #36]                   @ 4-byte Reload
	adcs	r0, r0, r12
	str	r0, [sp, #36]                   @ 4-byte Spill
	adcs	r0, r6, lr
	str	r0, [sp, #32]                   @ 4-byte Spill
	ldr	r0, [sp, #28]                   @ 4-byte Reload
	adcs	r0, r0, r4
	str	r0, [sp, #28]                   @ 4-byte Spill
	ldr	r0, [sp, #24]                   @ 4-byte Reload
	adcs	r0, r0, r8
	str	r0, [sp, #24]                   @ 4-byte Spill
	ldr	r0, [sp, #20]                   @ 4-byte Reload
	adc	r0, r0, #0
	str	r0, [sp, #20]                   @ 4-byte Spill
	ldr	r0, [sp, #60]                   @ 4-byte Reload
	mul	r2, r0, r7
	add	r0, sp, #152
	bl	mulPv256x32
	ldr	r0, [sp, #64]                   @ 4-byte Reload
	ldr	r1, [sp, #68]                   @ 4-byte Reload
	ldr	r2, [r0, #28]
	ldr	r0, [sp, #184]
	str	r0, [sp, #64]                   @ 4-byte Spill
	ldr	r0, [sp, #180]
	str	r0, [sp, #16]                   @ 4-byte Spill
	add	r0, sp, #112
	ldr	r5, [sp, #176]
	ldr	r4, [sp, #172]
	ldr	r9, [sp, #168]
	ldr	r10, [sp, #152]
	ldr	r11, [sp, #156]
	ldr	r8, [sp, #160]
	ldr	r6, [sp, #164]
	bl	mulPv256x32
	adds	r0, r7, r10
	ldr	r1, [sp, #44]                   @ 4-byte Reload
	ldr	r0, [sp, #48]                   @ 4-byte Reload
	add	lr, sp, #128
	ldr	r2, [sp, #16]                   @ 4-byte Reload
	adcs	r0, r0, r11
	adcs	r1, r1, r8
	str	r1, [sp, #68]                   @ 4-byte Spill
	ldr	r1, [sp, #40]                   @ 4-byte Reload
	ldr	r8, [sp, #68]                   @ 4-byte Reload
	adcs	r1, r1, r6
	str	r1, [sp, #48]                   @ 4-byte Spill
	ldr	r1, [sp, #36]                   @ 4-byte Reload
	adcs	r11, r1, r9
	ldr	r1, [sp, #32]                   @ 4-byte Reload
	adcs	r10, r1, r4
	ldr	r1, [sp, #28]                   @ 4-byte Reload
	adcs	r1, r1, r5
	str	r1, [sp, #44]                   @ 4-byte Spill
	ldr	r1, [sp, #24]                   @ 4-byte Reload
	adcs	r1, r1, r2
	str	r1, [sp, #40]                   @ 4-byte Spill
	ldr	r1, [sp, #20]                   @ 4-byte Reload
	ldr	r2, [sp, #64]                   @ 4-byte Reload
	adc	r1, r1, r2
	str	r1, [sp, #64]                   @ 4-byte Spill
	ldr	r2, [sp, #112]
	ldr	r12, [sp, #116]
	adds	r5, r0, r2
	ldr	r0, [sp, #60]                   @ 4-byte Reload
	adcs	r7, r8, r12
	ldr	r3, [sp, #120]
	ldr	r4, [sp, #124]
	mul	r9, r0, r5
	ldr	r8, [sp, #56]                   @ 4-byte Reload
	ldm	lr, {r0, r1, r2, r6, lr}
	str	r7, [sp, #68]                   @ 4-byte Spill
	ldr	r7, [sp, #48]                   @ 4-byte Reload
	adcs	r7, r7, r3
	adcs	r4, r11, r4
	adcs	r10, r10, r0
	ldr	r0, [sp, #44]                   @ 4-byte Reload
	adcs	r11, r0, r1
	ldr	r0, [sp, #40]                   @ 4-byte Reload
	mov	r1, r8
	adcs	r0, r0, r2
	str	r0, [sp, #40]                   @ 4-byte Spill
	ldr	r0, [sp, #64]                   @ 4-byte Reload
	mov	r2, r9
	adcs	r6, r0, r6
	adc	r0, lr, #0
	str	r0, [sp, #60]                   @ 4-byte Spill
	add	r0, sp, #72
	bl	mulPv256x32
	add	r3, sp, #72
	add	lr, r8, #20
	ldm	r3, {r0, r1, r2, r3}
	adds	r0, r5, r0
	ldr	r0, [sp, #68]                   @ 4-byte Reload
	adcs	r0, r0, r1
	str	r0, [sp, #68]                   @ 4-byte Spill
	adcs	r2, r7, r2
	str	r2, [sp, #64]                   @ 4-byte Spill
	adcs	r0, r4, r3
	str	r0, [sp, #48]                   @ 4-byte Spill
	ldr	r3, [sp, #88]
	ldr	r1, [sp, #40]                   @ 4-byte Reload
	adcs	r3, r10, r3
	str	r3, [sp, #44]                   @ 4-byte Spill
	ldr	r7, [sp, #92]
	ldr	r5, [sp, #96]
	adcs	r0, r11, r7
	ldr	r7, [r8, #8]
	adcs	r1, r1, r5
	str	r1, [sp, #40]                   @ 4-byte Spill
	ldr	r4, [sp, #100]
	ldr	r1, [sp, #60]                   @ 4-byte Reload
	adcs	r11, r6, r4
	ldr	r6, [sp, #104]
	ldr	r4, [sp, #68]                   @ 4-byte Reload
	adc	r1, r1, r6
	ldm	r8, {r6, r9}
	str	r7, [sp, #60]                   @ 4-byte Spill
	ldr	r10, [r8, #12]
	ldr	r7, [r8, #16]
	subs	r8, r4, r6
	sbcs	r6, r2, r9
	ldr	r9, [sp, #48]                   @ 4-byte Reload
	ldr	r2, [sp, #60]                   @ 4-byte Reload
	ldm	lr, {r5, r12, lr}
	sbcs	r4, r9, r2
	sbcs	r3, r3, r10
	ldr	r2, [sp, #40]                   @ 4-byte Reload
	sbcs	r7, r0, r7
	mov	r10, r0
	sbcs	r5, r2, r5
	ldr	r0, [sp, #52]                   @ 4-byte Reload
	sbcs	r12, r11, r12
	sbc	lr, r1, lr
	cmn	lr, #1
	movgt	r1, lr
	movle	r12, r11
	str	r1, [r0, #28]
	movle	r5, r2
	ldr	r1, [sp, #44]                   @ 4-byte Reload
	cmn	lr, #1
	movle	r7, r10
	movle	r4, r9
	str	r12, [r0, #24]
	movle	r3, r1
	ldr	r1, [sp, #64]                   @ 4-byte Reload
	cmn	lr, #1
	str	r5, [r0, #20]
	str	r7, [r0, #16]
	movle	r6, r1
	ldr	r1, [sp, #68]                   @ 4-byte Reload
	str	r3, [r0, #12]
	str	r4, [r0, #8]
	movle	r8, r1
	str	r6, [r0, #4]
	str	r8, [r0]
	add	sp, sp, #716
	pop	{r4, r5, r6, r7, r8, r9, r10, r11, lr}
	mov	pc, lr
.Lfunc_end44:
	.size	mcl_fp_montNF8L, .Lfunc_end44-mcl_fp_montNF8L
	.cantunwind
	.fnend
                                        @ -- End function
	.globl	mcl_fp_montRed8L                @ -- Begin function mcl_fp_montRed8L
	.p2align	2
	.type	mcl_fp_montRed8L,%function
	.code	32                              @ @mcl_fp_montRed8L
mcl_fp_montRed8L:
	.fnstart
@ %bb.0:
	.save	{r4, r5, r6, r7, r8, r9, r10, r11, lr}
	push	{r4, r5, r6, r7, r8, r9, r10, r11, lr}
	.pad	#436
	sub	sp, sp, #436
	str	r0, [sp, #108]                  @ 4-byte Spill
	mov	r6, r2
	ldr	r0, [r2]
	mov	r5, r1
	str	r0, [sp, #104]                  @ 4-byte Spill
	ldr	r0, [r2, #4]
	str	r0, [sp, #100]                  @ 4-byte Spill
	ldr	r0, [r2, #8]
	str	r0, [sp, #96]                   @ 4-byte Spill
	ldr	r0, [r2, #12]
	str	r0, [sp, #80]                   @ 4-byte Spill
	ldr	r0, [r2, #16]
	str	r0, [sp, #84]                   @ 4-byte Spill
	ldr	r0, [r2, #20]
	str	r0, [sp, #88]                   @ 4-byte Spill
	ldr	r0, [r2, #24]
	ldr	r9, [r2, #-4]
	ldr	r4, [r1]
	str	r0, [sp, #92]                   @ 4-byte Spill
	ldr	r0, [r1, #4]
	str	r0, [sp, #72]                   @ 4-byte Spill
	mul	r2, r4, r9
	ldr	r0, [r1, #8]
	str	r0, [sp, #68]                   @ 4-byte Spill
	ldr	r0, [r1, #20]
	str	r0, [sp, #56]                   @ 4-byte Spill
	ldr	r0, [r1, #24]
	str	r0, [sp, #60]                   @ 4-byte Spill
	ldr	r0, [r1, #28]
	str	r0, [sp, #64]                   @ 4-byte Spill
	ldr	r0, [r6, #28]
	str	r0, [sp, #76]                   @ 4-byte Spill
	add	r0, sp, #392
	ldr	r10, [r1, #12]
	ldr	r11, [r1, #16]
	mov	r1, r6
	bl	mulPv256x32
	ldr	r7, [sp, #392]
	add	lr, sp, #396
	ldr	r8, [sp, #420]
	adds	r4, r4, r7
	ldm	lr, {r0, r1, r2, r3, r12, lr}
	ldr	r4, [sp, #72]                   @ 4-byte Reload
	adcs	r4, r4, r0
	ldr	r0, [sp, #68]                   @ 4-byte Reload
	adcs	r0, r0, r1
	str	r0, [sp, #72]                   @ 4-byte Spill
	adcs	r0, r10, r2
	str	r0, [sp, #52]                   @ 4-byte Spill
	ldr	r0, [sp, #56]                   @ 4-byte Reload
	adcs	r10, r11, r3
	mul	r2, r9, r4
	mov	r1, r6
	adcs	r11, r0, r12
	ldr	r0, [sp, #60]                   @ 4-byte Reload
	adcs	r0, r0, lr
	str	r0, [sp, #56]                   @ 4-byte Spill
	ldr	r0, [sp, #64]                   @ 4-byte Reload
	adcs	r8, r0, r8
	mrs	r0, apsr
	str	r0, [sp, #48]                   @ 4-byte Spill
	ldr	r0, [r5, #32]
	str	r0, [sp, #68]                   @ 4-byte Spill
	ldr	r0, [sp, #424]
	str	r0, [sp, #44]                   @ 4-byte Spill
	add	r0, sp, #352
	bl	mulPv256x32
	ldr	r3, [sp, #352]
	add	lr, sp, #356
	ldr	r7, [sp, #376]
	adds	r3, r4, r3
	ldm	lr, {r0, r1, r2, r12, lr}
	ldr	r3, [sp, #72]                   @ 4-byte Reload
	adcs	r4, r3, r0
	ldr	r0, [sp, #52]                   @ 4-byte Reload
	adcs	r0, r0, r1
	str	r0, [sp, #64]                   @ 4-byte Spill
	adcs	r10, r10, r2
	mul	r2, r9, r4
	adcs	r0, r11, r12
	str	r0, [sp, #60]                   @ 4-byte Spill
	mov	r11, r9
	ldr	r0, [sp, #56]                   @ 4-byte Reload
	adcs	r0, r0, lr
	str	r0, [sp, #56]                   @ 4-byte Spill
	adcs	r0, r8, r7
	str	r0, [sp, #52]                   @ 4-byte Spill
	mrs	r0, apsr
	str	r0, [sp, #72]                   @ 4-byte Spill
	ldr	r0, [sp, #48]                   @ 4-byte Reload
	msr	APSR_nzcvq, r0
	ldr	r1, [sp, #68]                   @ 4-byte Reload
	ldr	r3, [sp, #44]                   @ 4-byte Reload
	ldr	r0, [sp, #384]
	adcs	r1, r1, r3
	str	r1, [sp, #48]                   @ 4-byte Spill
	adc	r0, r0, #0
	str	r0, [sp, #44]                   @ 4-byte Spill
	add	r0, sp, #312
	mov	r1, r6
	ldr	r9, [r5, #36]
	ldr	r8, [sp, #380]
	bl	mulPv256x32
	add	r7, sp, #312
	add	r12, sp, #324
	ldm	r7, {r2, r3, r7}
	adds	r2, r4, r2
	ldm	r12, {r0, r1, r12}
	ldr	r2, [sp, #64]                   @ 4-byte Reload
	adcs	r4, r2, r3
	adcs	r2, r10, r7
	str	r2, [sp, #64]                   @ 4-byte Spill
	ldr	r2, [sp, #60]                   @ 4-byte Reload
	adcs	r0, r2, r0
	str	r0, [sp, #60]                   @ 4-byte Spill
	ldr	r0, [sp, #56]                   @ 4-byte Reload
	mul	r2, r11, r4
	adcs	r0, r0, r1
	str	r0, [sp, #56]                   @ 4-byte Spill
	ldr	r0, [sp, #52]                   @ 4-byte Reload
	adcs	r0, r0, r12
	str	r0, [sp, #52]                   @ 4-byte Spill
	mrs	r0, apsr
	str	r0, [sp, #68]                   @ 4-byte Spill
	ldr	r0, [sp, #72]                   @ 4-byte Reload
	msr	APSR_nzcvq, r0
	ldr	r1, [sp, #48]                   @ 4-byte Reload
	ldr	r0, [sp, #344]
	adcs	r1, r1, r8
	str	r1, [sp, #48]                   @ 4-byte Spill
	ldr	r1, [sp, #44]                   @ 4-byte Reload
	ldr	r10, [r5, #40]
	adcs	r1, r9, r1
	str	r1, [sp, #44]                   @ 4-byte Spill
	adc	r0, r0, #0
	str	r0, [sp, #40]                   @ 4-byte Spill
	add	r0, sp, #272
	mov	r1, r6
	ldr	r8, [sp, #340]
	ldr	r9, [sp, #336]
	bl	mulPv256x32
	add	r7, sp, #272
	ldr	r0, [sp, #288]
	ldm	r7, {r1, r2, r3, r7}
	adds	r1, r4, r1
	ldr	r1, [sp, #64]                   @ 4-byte Reload
	adcs	r4, r1, r2
	ldr	r1, [sp, #60]                   @ 4-byte Reload
	adcs	r1, r1, r3
	str	r1, [sp, #64]                   @ 4-byte Spill
	ldr	r1, [sp, #56]                   @ 4-byte Reload
	mul	r2, r11, r4
	adcs	r1, r1, r7
	str	r1, [sp, #60]                   @ 4-byte Spill
	ldr	r1, [sp, #52]                   @ 4-byte Reload
	adcs	r0, r1, r0
	str	r0, [sp, #56]                   @ 4-byte Spill
	mrs	r0, apsr
	str	r0, [sp, #72]                   @ 4-byte Spill
	ldr	r0, [sp, #68]                   @ 4-byte Reload
	msr	APSR_nzcvq, r0
	ldr	r1, [sp, #48]                   @ 4-byte Reload
	ldr	r0, [sp, #304]
	adcs	r1, r1, r9
	str	r1, [sp, #52]                   @ 4-byte Spill
	ldr	r1, [sp, #44]                   @ 4-byte Reload
	ldr	r7, [r5, #44]
	adcs	r1, r1, r8
	str	r1, [sp, #48]                   @ 4-byte Spill
	ldr	r1, [sp, #40]                   @ 4-byte Reload
	adcs	r1, r10, r1
	str	r1, [sp, #44]                   @ 4-byte Spill
	adc	r0, r0, #0
	str	r0, [sp, #40]                   @ 4-byte Spill
	add	r0, sp, #232
	mov	r1, r6
	ldr	r8, [sp, #300]
	ldr	r9, [sp, #296]
	ldr	r10, [sp, #292]
	bl	mulPv256x32
	add	r3, sp, #232
	ldm	r3, {r0, r1, r2, r3}
	adds	r0, r4, r0
	ldr	r0, [sp, #64]                   @ 4-byte Reload
	adcs	r4, r0, r1
	ldr	r0, [sp, #60]                   @ 4-byte Reload
	adcs	r0, r0, r2
	str	r0, [sp, #64]                   @ 4-byte Spill
	ldr	r0, [sp, #56]                   @ 4-byte Reload
	mul	r2, r11, r4
	adcs	r0, r0, r3
	str	r0, [sp, #60]                   @ 4-byte Spill
	mrs	r0, apsr
	str	r0, [sp, #68]                   @ 4-byte Spill
	ldr	r0, [sp, #72]                   @ 4-byte Reload
	msr	APSR_nzcvq, r0
	ldr	r1, [sp, #52]                   @ 4-byte Reload
	ldr	r0, [sp, #264]
	adcs	r1, r1, r10
	str	r1, [sp, #56]                   @ 4-byte Spill
	ldr	r1, [sp, #48]                   @ 4-byte Reload
	adcs	r1, r1, r9
	str	r1, [sp, #52]                   @ 4-byte Spill
	ldr	r1, [sp, #44]                   @ 4-byte Reload
	adcs	r1, r1, r8
	str	r1, [sp, #44]                   @ 4-byte Spill
	ldr	r1, [sp, #40]                   @ 4-byte Reload
	adcs	r1, r7, r1
	str	r1, [sp, #40]                   @ 4-byte Spill
	adc	r0, r0, #0
	str	r0, [sp, #36]                   @ 4-byte Spill
	ldr	r0, [r5, #48]
	mov	r1, r6
	str	r0, [sp, #28]                   @ 4-byte Spill
	add	r0, sp, #192
	ldr	r8, [sp, #260]
	ldr	r9, [sp, #256]
	ldr	r10, [sp, #252]
	ldr	r7, [sp, #248]
	bl	mulPv256x32
	add	r3, sp, #192
	ldm	r3, {r0, r1, r2, r3}
	adds	r0, r4, r0
	str	r3, [sp, #72]                   @ 4-byte Spill
	ldr	r0, [sp, #64]                   @ 4-byte Reload
	adcs	r4, r0, r1
	ldr	r0, [sp, #60]                   @ 4-byte Reload
	adcs	r0, r0, r2
	str	r0, [sp, #32]                   @ 4-byte Spill
	mrs	r0, apsr
	mul	r2, r11, r4
	str	r0, [sp, #48]                   @ 4-byte Spill
	ldr	r0, [sp, #68]                   @ 4-byte Reload
	msr	APSR_nzcvq, r0
	ldr	r1, [sp, #56]                   @ 4-byte Reload
	ldr	r3, [sp, #28]                   @ 4-byte Reload
	adcs	r7, r1, r7
	ldr	r1, [sp, #52]                   @ 4-byte Reload
	ldr	r0, [sp, #224]
	adcs	r10, r1, r10
	ldr	r1, [sp, #44]                   @ 4-byte Reload
	adcs	r9, r1, r9
	ldr	r1, [sp, #40]                   @ 4-byte Reload
	adcs	r8, r1, r8
	ldr	r1, [sp, #36]                   @ 4-byte Reload
	adcs	r1, r3, r1
	str	r1, [sp, #64]                   @ 4-byte Spill
	adc	r0, r0, #0
	str	r0, [sp, #60]                   @ 4-byte Spill
	ldr	r0, [r5, #52]
	mov	r1, r6
	str	r0, [sp, #56]                   @ 4-byte Spill
	ldr	r0, [sp, #220]
	str	r0, [sp, #52]                   @ 4-byte Spill
	ldr	r0, [sp, #216]
	str	r0, [sp, #44]                   @ 4-byte Spill
	ldr	r0, [sp, #212]
	str	r0, [sp, #28]                   @ 4-byte Spill
	ldr	r0, [sp, #208]
	str	r0, [sp, #24]                   @ 4-byte Spill
	add	r0, sp, #152
	bl	mulPv256x32
	add	r2, sp, #152
	ldm	r2, {r0, r1, r2}
	adds	r0, r4, r0
	str	r2, [sp, #40]                   @ 4-byte Spill
	ldr	r0, [sp, #32]                   @ 4-byte Reload
	ldr	r2, [sp, #164]
	adcs	r0, r0, r1
	str	r2, [sp, #36]                   @ 4-byte Spill
	str	r0, [sp, #68]                   @ 4-byte Spill
	mrs	r1, apsr
	str	r1, [sp, #32]                   @ 4-byte Spill
	mul	r2, r11, r0
	ldr	r1, [sp, #48]                   @ 4-byte Reload
	msr	APSR_nzcvq, r1
	ldr	r0, [sp, #72]                   @ 4-byte Reload
	ldr	r1, [sp, #24]                   @ 4-byte Reload
	adcs	r0, r7, r0
	str	r0, [sp, #16]                   @ 4-byte Spill
	ldr	r0, [r5, #56]
	adcs	r4, r10, r1
	str	r0, [sp, #72]                   @ 4-byte Spill
	ldr	r0, [sp, #180]
	str	r0, [sp, #48]                   @ 4-byte Spill
	ldr	r1, [sp, #28]                   @ 4-byte Reload
	ldr	r0, [sp, #176]
	str	r0, [sp, #20]                   @ 4-byte Spill
	adcs	r11, r9, r1
	ldr	r1, [sp, #44]                   @ 4-byte Reload
	ldr	r0, [sp, #172]
	str	r0, [sp, #12]                   @ 4-byte Spill
	adcs	r7, r8, r1
	ldr	r1, [sp, #64]                   @ 4-byte Reload
	ldr	r3, [sp, #52]                   @ 4-byte Reload
	ldr	r0, [sp, #168]
	str	r0, [sp, #8]                    @ 4-byte Spill
	adcs	r10, r1, r3
	ldr	r1, [sp, #60]                   @ 4-byte Reload
	ldr	r3, [sp, #56]                   @ 4-byte Reload
	ldr	r0, [sp, #184]
	adcs	r8, r3, r1
	mov	r1, r6
	adc	r9, r0, #0
	add	r0, sp, #112
	bl	mulPv256x32
	ldr	r0, [sp, #32]                   @ 4-byte Reload
	msr	APSR_nzcvq, r0
	ldr	r0, [sp, #40]                   @ 4-byte Reload
	ldr	r1, [sp, #16]                   @ 4-byte Reload
	ldr	r6, [sp, #120]
	adcs	r12, r1, r0
	ldr	r0, [sp, #36]                   @ 4-byte Reload
	ldr	r3, [sp, #124]
	adcs	r1, r4, r0
	ldr	r0, [sp, #8]                    @ 4-byte Reload
	ldr	r4, [sp, #116]
	adcs	r2, r11, r0
	ldr	r0, [sp, #12]                   @ 4-byte Reload
	adcs	r11, r7, r0
	ldr	r0, [sp, #20]                   @ 4-byte Reload
	ldr	r7, [sp, #144]
	adcs	r10, r10, r0
	ldr	r0, [sp, #48]                   @ 4-byte Reload
	adcs	r8, r8, r0
	ldr	r0, [sp, #72]                   @ 4-byte Reload
	adcs	r9, r0, r9
	ldr	r0, [sp, #68]                   @ 4-byte Reload
	adc	lr, r7, #0
	ldr	r7, [sp, #112]
	adds	r7, r0, r7
	ldr	r0, [sp, #104]                  @ 4-byte Reload
	adcs	r4, r12, r4
	str	r4, [sp, #72]                   @ 4-byte Spill
	adcs	r1, r1, r6
	str	r1, [sp, #68]                   @ 4-byte Spill
	adcs	r2, r2, r3
	str	r2, [sp, #64]                   @ 4-byte Spill
	ldr	r3, [sp, #128]
	adcs	r6, r11, r3
	str	r6, [sp, #60]                   @ 4-byte Spill
	ldr	r3, [sp, #132]
	adcs	r11, r10, r3
	ldr	r3, [sp, #136]
	adcs	r10, r8, r3
	ldr	r3, [sp, #140]
	adcs	r12, r9, r3
	ldr	r3, [r5, #60]
	mov	r9, #0
	adc	r5, r3, lr
	subs	r8, r4, r0
	ldr	r0, [sp, #100]                  @ 4-byte Reload
	sbcs	lr, r1, r0
	ldr	r0, [sp, #96]                   @ 4-byte Reload
	sbcs	r4, r2, r0
	ldr	r0, [sp, #80]                   @ 4-byte Reload
	sbcs	r6, r6, r0
	ldr	r0, [sp, #84]                   @ 4-byte Reload
	sbcs	r3, r11, r0
	ldr	r0, [sp, #88]                   @ 4-byte Reload
	sbcs	r7, r10, r0
	ldr	r0, [sp, #92]                   @ 4-byte Reload
	sbcs	r1, r12, r0
	ldr	r0, [sp, #76]                   @ 4-byte Reload
	sbcs	r0, r5, r0
	sbc	r2, r9, #0
	ands	r2, r2, #1
	movne	r0, r5
	ldr	r5, [sp, #108]                  @ 4-byte Reload
	movne	r1, r12
	movne	r7, r10
	cmp	r2, #0
	str	r0, [r5, #28]
	movne	r3, r11
	ldr	r0, [sp, #60]                   @ 4-byte Reload
	str	r1, [r5, #24]
	str	r7, [r5, #20]
	movne	r6, r0
	ldr	r0, [sp, #64]                   @ 4-byte Reload
	str	r3, [r5, #16]
	str	r6, [r5, #12]
	movne	r4, r0
	ldr	r0, [sp, #68]                   @ 4-byte Reload
	cmp	r2, #0
	str	r4, [r5, #8]
	movne	lr, r0
	ldr	r0, [sp, #72]                   @ 4-byte Reload
	str	lr, [r5, #4]
	movne	r8, r0
	str	r8, [r5]
	add	sp, sp, #436
	pop	{r4, r5, r6, r7, r8, r9, r10, r11, lr}
	mov	pc, lr
.Lfunc_end45:
	.size	mcl_fp_montRed8L, .Lfunc_end45-mcl_fp_montRed8L
	.cantunwind
	.fnend
                                        @ -- End function
	.globl	mcl_fp_montRedNF8L              @ -- Begin function mcl_fp_montRedNF8L
	.p2align	2
	.type	mcl_fp_montRedNF8L,%function
	.code	32                              @ @mcl_fp_montRedNF8L
mcl_fp_montRedNF8L:
	.fnstart
@ %bb.0:
	.save	{r4, r5, r6, r7, r8, r9, r10, r11, lr}
	push	{r4, r5, r6, r7, r8, r9, r10, r11, lr}
	.pad	#436
	sub	sp, sp, #436
	str	r0, [sp, #108]                  @ 4-byte Spill
	mov	r6, r2
	ldr	r0, [r2]
	mov	r5, r1
	str	r0, [sp, #104]                  @ 4-byte Spill
	ldr	r0, [r2, #4]
	str	r0, [sp, #100]                  @ 4-byte Spill
	ldr	r0, [r2, #8]
	str	r0, [sp, #96]                   @ 4-byte Spill
	ldr	r0, [r2, #12]
	str	r0, [sp, #80]                   @ 4-byte Spill
	ldr	r0, [r2, #16]
	str	r0, [sp, #84]                   @ 4-byte Spill
	ldr	r0, [r2, #20]
	str	r0, [sp, #88]                   @ 4-byte Spill
	ldr	r0, [r2, #24]
	ldr	r9, [r2, #-4]
	ldr	r4, [r1]
	str	r0, [sp, #92]                   @ 4-byte Spill
	ldr	r0, [r1, #4]
	str	r0, [sp, #72]                   @ 4-byte Spill
	mul	r2, r4, r9
	ldr	r0, [r1, #8]
	str	r0, [sp, #68]                   @ 4-byte Spill
	ldr	r0, [r1, #20]
	str	r0, [sp, #56]                   @ 4-byte Spill
	ldr	r0, [r1, #24]
	str	r0, [sp, #60]                   @ 4-byte Spill
	ldr	r0, [r1, #28]
	str	r0, [sp, #64]                   @ 4-byte Spill
	ldr	r0, [r6, #28]
	str	r0, [sp, #76]                   @ 4-byte Spill
	add	r0, sp, #392
	ldr	r10, [r1, #12]
	ldr	r11, [r1, #16]
	mov	r1, r6
	bl	mulPv256x32
	ldr	r7, [sp, #392]
	add	lr, sp, #396
	ldr	r8, [sp, #420]
	adds	r4, r4, r7
	ldm	lr, {r0, r1, r2, r3, r12, lr}
	ldr	r4, [sp, #72]                   @ 4-byte Reload
	adcs	r4, r4, r0
	ldr	r0, [sp, #68]                   @ 4-byte Reload
	adcs	r0, r0, r1
	str	r0, [sp, #72]                   @ 4-byte Spill
	adcs	r0, r10, r2
	str	r0, [sp, #52]                   @ 4-byte Spill
	ldr	r0, [sp, #56]                   @ 4-byte Reload
	adcs	r10, r11, r3
	mul	r2, r9, r4
	mov	r1, r6
	adcs	r11, r0, r12
	ldr	r0, [sp, #60]                   @ 4-byte Reload
	adcs	r0, r0, lr
	str	r0, [sp, #56]                   @ 4-byte Spill
	ldr	r0, [sp, #64]                   @ 4-byte Reload
	adcs	r8, r0, r8
	mrs	r0, apsr
	str	r0, [sp, #48]                   @ 4-byte Spill
	ldr	r0, [r5, #32]
	str	r0, [sp, #68]                   @ 4-byte Spill
	ldr	r0, [sp, #424]
	str	r0, [sp, #44]                   @ 4-byte Spill
	add	r0, sp, #352
	bl	mulPv256x32
	ldr	r3, [sp, #352]
	add	lr, sp, #356
	ldr	r7, [sp, #376]
	adds	r3, r4, r3
	ldm	lr, {r0, r1, r2, r12, lr}
	ldr	r3, [sp, #72]                   @ 4-byte Reload
	adcs	r4, r3, r0
	ldr	r0, [sp, #52]                   @ 4-byte Reload
	adcs	r0, r0, r1
	str	r0, [sp, #64]                   @ 4-byte Spill
	adcs	r10, r10, r2
	mul	r2, r9, r4
	adcs	r0, r11, r12
	str	r0, [sp, #60]                   @ 4-byte Spill
	mov	r11, r9
	ldr	r0, [sp, #56]                   @ 4-byte Reload
	adcs	r0, r0, lr
	str	r0, [sp, #56]                   @ 4-byte Spill
	adcs	r0, r8, r7
	str	r0, [sp, #52]                   @ 4-byte Spill
	mrs	r0, apsr
	str	r0, [sp, #72]                   @ 4-byte Spill
	ldr	r0, [sp, #48]                   @ 4-byte Reload
	msr	APSR_nzcvq, r0
	ldr	r1, [sp, #68]                   @ 4-byte Reload
	ldr	r3, [sp, #44]                   @ 4-byte Reload
	ldr	r0, [sp, #384]
	adcs	r1, r1, r3
	str	r1, [sp, #48]                   @ 4-byte Spill
	adc	r0, r0, #0
	str	r0, [sp, #44]                   @ 4-byte Spill
	add	r0, sp, #312
	mov	r1, r6
	ldr	r9, [r5, #36]
	ldr	r8, [sp, #380]
	bl	mulPv256x32
	add	r7, sp, #312
	add	r12, sp, #324
	ldm	r7, {r2, r3, r7}
	adds	r2, r4, r2
	ldm	r12, {r0, r1, r12}
	ldr	r2, [sp, #64]                   @ 4-byte Reload
	adcs	r4, r2, r3
	adcs	r2, r10, r7
	str	r2, [sp, #64]                   @ 4-byte Spill
	ldr	r2, [sp, #60]                   @ 4-byte Reload
	adcs	r0, r2, r0
	str	r0, [sp, #60]                   @ 4-byte Spill
	ldr	r0, [sp, #56]                   @ 4-byte Reload
	mul	r2, r11, r4
	adcs	r0, r0, r1
	str	r0, [sp, #56]                   @ 4-byte Spill
	ldr	r0, [sp, #52]                   @ 4-byte Reload
	adcs	r0, r0, r12
	str	r0, [sp, #52]                   @ 4-byte Spill
	mrs	r0, apsr
	str	r0, [sp, #68]                   @ 4-byte Spill
	ldr	r0, [sp, #72]                   @ 4-byte Reload
	msr	APSR_nzcvq, r0
	ldr	r1, [sp, #48]                   @ 4-byte Reload
	ldr	r0, [sp, #344]
	adcs	r1, r1, r8
	str	r1, [sp, #48]                   @ 4-byte Spill
	ldr	r1, [sp, #44]                   @ 4-byte Reload
	ldr	r10, [r5, #40]
	adcs	r1, r9, r1
	str	r1, [sp, #44]                   @ 4-byte Spill
	adc	r0, r0, #0
	str	r0, [sp, #40]                   @ 4-byte Spill
	add	r0, sp, #272
	mov	r1, r6
	ldr	r8, [sp, #340]
	ldr	r9, [sp, #336]
	bl	mulPv256x32
	add	r7, sp, #272
	ldr	r0, [sp, #288]
	ldm	r7, {r1, r2, r3, r7}
	adds	r1, r4, r1
	ldr	r1, [sp, #64]                   @ 4-byte Reload
	adcs	r4, r1, r2
	ldr	r1, [sp, #60]                   @ 4-byte Reload
	adcs	r1, r1, r3
	str	r1, [sp, #64]                   @ 4-byte Spill
	ldr	r1, [sp, #56]                   @ 4-byte Reload
	mul	r2, r11, r4
	adcs	r1, r1, r7
	str	r1, [sp, #60]                   @ 4-byte Spill
	ldr	r1, [sp, #52]                   @ 4-byte Reload
	adcs	r0, r1, r0
	str	r0, [sp, #56]                   @ 4-byte Spill
	mrs	r0, apsr
	str	r0, [sp, #72]                   @ 4-byte Spill
	ldr	r0, [sp, #68]                   @ 4-byte Reload
	msr	APSR_nzcvq, r0
	ldr	r1, [sp, #48]                   @ 4-byte Reload
	ldr	r0, [sp, #304]
	adcs	r1, r1, r9
	str	r1, [sp, #52]                   @ 4-byte Spill
	ldr	r1, [sp, #44]                   @ 4-byte Reload
	ldr	r7, [r5, #44]
	adcs	r1, r1, r8
	str	r1, [sp, #48]                   @ 4-byte Spill
	ldr	r1, [sp, #40]                   @ 4-byte Reload
	adcs	r1, r10, r1
	str	r1, [sp, #44]                   @ 4-byte Spill
	adc	r0, r0, #0
	str	r0, [sp, #40]                   @ 4-byte Spill
	add	r0, sp, #232
	mov	r1, r6
	ldr	r8, [sp, #300]
	ldr	r9, [sp, #296]
	ldr	r10, [sp, #292]
	bl	mulPv256x32
	add	r3, sp, #232
	ldm	r3, {r0, r1, r2, r3}
	adds	r0, r4, r0
	ldr	r0, [sp, #64]                   @ 4-byte Reload
	adcs	r4, r0, r1
	ldr	r0, [sp, #60]                   @ 4-byte Reload
	adcs	r0, r0, r2
	str	r0, [sp, #64]                   @ 4-byte Spill
	ldr	r0, [sp, #56]                   @ 4-byte Reload
	mul	r2, r11, r4
	adcs	r0, r0, r3
	str	r0, [sp, #60]                   @ 4-byte Spill
	mrs	r0, apsr
	str	r0, [sp, #68]                   @ 4-byte Spill
	ldr	r0, [sp, #72]                   @ 4-byte Reload
	msr	APSR_nzcvq, r0
	ldr	r1, [sp, #52]                   @ 4-byte Reload
	ldr	r0, [sp, #264]
	adcs	r1, r1, r10
	str	r1, [sp, #56]                   @ 4-byte Spill
	ldr	r1, [sp, #48]                   @ 4-byte Reload
	adcs	r1, r1, r9
	str	r1, [sp, #52]                   @ 4-byte Spill
	ldr	r1, [sp, #44]                   @ 4-byte Reload
	adcs	r1, r1, r8
	str	r1, [sp, #44]                   @ 4-byte Spill
	ldr	r1, [sp, #40]                   @ 4-byte Reload
	adcs	r1, r7, r1
	str	r1, [sp, #40]                   @ 4-byte Spill
	adc	r0, r0, #0
	str	r0, [sp, #36]                   @ 4-byte Spill
	ldr	r0, [r5, #48]
	mov	r1, r6
	str	r0, [sp, #28]                   @ 4-byte Spill
	add	r0, sp, #192
	ldr	r8, [sp, #260]
	ldr	r9, [sp, #256]
	ldr	r10, [sp, #252]
	ldr	r7, [sp, #248]
	bl	mulPv256x32
	add	r3, sp, #192
	ldm	r3, {r0, r1, r2, r3}
	adds	r0, r4, r0
	str	r3, [sp, #72]                   @ 4-byte Spill
	ldr	r0, [sp, #64]                   @ 4-byte Reload
	adcs	r4, r0, r1
	ldr	r0, [sp, #60]                   @ 4-byte Reload
	adcs	r0, r0, r2
	str	r0, [sp, #32]                   @ 4-byte Spill
	mrs	r0, apsr
	mul	r2, r11, r4
	str	r0, [sp, #48]                   @ 4-byte Spill
	ldr	r0, [sp, #68]                   @ 4-byte Reload
	msr	APSR_nzcvq, r0
	ldr	r1, [sp, #56]                   @ 4-byte Reload
	ldr	r3, [sp, #28]                   @ 4-byte Reload
	adcs	r7, r1, r7
	ldr	r1, [sp, #52]                   @ 4-byte Reload
	ldr	r0, [sp, #224]
	adcs	r10, r1, r10
	ldr	r1, [sp, #44]                   @ 4-byte Reload
	adcs	r9, r1, r9
	ldr	r1, [sp, #40]                   @ 4-byte Reload
	adcs	r8, r1, r8
	ldr	r1, [sp, #36]                   @ 4-byte Reload
	adcs	r1, r3, r1
	str	r1, [sp, #64]                   @ 4-byte Spill
	adc	r0, r0, #0
	str	r0, [sp, #60]                   @ 4-byte Spill
	ldr	r0, [r5, #52]
	mov	r1, r6
	str	r0, [sp, #56]                   @ 4-byte Spill
	ldr	r0, [sp, #220]
	str	r0, [sp, #52]                   @ 4-byte Spill
	ldr	r0, [sp, #216]
	str	r0, [sp, #44]                   @ 4-byte Spill
	ldr	r0, [sp, #212]
	str	r0, [sp, #28]                   @ 4-byte Spill
	ldr	r0, [sp, #208]
	str	r0, [sp, #24]                   @ 4-byte Spill
	add	r0, sp, #152
	bl	mulPv256x32
	add	r2, sp, #152
	ldm	r2, {r0, r1, r2}
	adds	r0, r4, r0
	str	r2, [sp, #40]                   @ 4-byte Spill
	ldr	r0, [sp, #32]                   @ 4-byte Reload
	ldr	r2, [sp, #164]
	adcs	r0, r0, r1
	str	r2, [sp, #36]                   @ 4-byte Spill
	str	r0, [sp, #68]                   @ 4-byte Spill
	mrs	r1, apsr
	str	r1, [sp, #32]                   @ 4-byte Spill
	mul	r2, r11, r0
	ldr	r1, [sp, #48]                   @ 4-byte Reload
	msr	APSR_nzcvq, r1
	ldr	r0, [sp, #72]                   @ 4-byte Reload
	ldr	r1, [sp, #24]                   @ 4-byte Reload
	adcs	r0, r7, r0
	str	r0, [sp, #16]                   @ 4-byte Spill
	ldr	r0, [r5, #56]
	adcs	r4, r10, r1
	str	r0, [sp, #72]                   @ 4-byte Spill
	ldr	r0, [sp, #180]
	str	r0, [sp, #48]                   @ 4-byte Spill
	ldr	r1, [sp, #28]                   @ 4-byte Reload
	ldr	r0, [sp, #176]
	str	r0, [sp, #20]                   @ 4-byte Spill
	adcs	r11, r9, r1
	ldr	r1, [sp, #44]                   @ 4-byte Reload
	ldr	r0, [sp, #172]
	str	r0, [sp, #12]                   @ 4-byte Spill
	adcs	r7, r8, r1
	ldr	r1, [sp, #64]                   @ 4-byte Reload
	ldr	r3, [sp, #52]                   @ 4-byte Reload
	ldr	r0, [sp, #168]
	str	r0, [sp, #8]                    @ 4-byte Spill
	adcs	r10, r1, r3
	ldr	r1, [sp, #60]                   @ 4-byte Reload
	ldr	r3, [sp, #56]                   @ 4-byte Reload
	ldr	r0, [sp, #184]
	adcs	r8, r3, r1
	mov	r1, r6
	adc	r9, r0, #0
	add	r0, sp, #112
	bl	mulPv256x32
	ldr	r0, [sp, #32]                   @ 4-byte Reload
	msr	APSR_nzcvq, r0
	ldr	r0, [sp, #40]                   @ 4-byte Reload
	ldr	r1, [sp, #16]                   @ 4-byte Reload
	adcs	r12, r1, r0
	ldr	r0, [sp, #36]                   @ 4-byte Reload
	adcs	r1, r4, r0
	ldr	r0, [sp, #8]                    @ 4-byte Reload
	adcs	r2, r11, r0
	ldr	r0, [sp, #12]                   @ 4-byte Reload
	adcs	r11, r7, r0
	ldr	r0, [sp, #20]                   @ 4-byte Reload
	add	r7, sp, #112
	adcs	r10, r10, r0
	ldr	r0, [sp, #48]                   @ 4-byte Reload
	adcs	r8, r8, r0
	ldr	r0, [sp, #72]                   @ 4-byte Reload
	adcs	r0, r0, r9
	str	r0, [sp, #60]                   @ 4-byte Spill
	ldr	r4, [sp, #144]
	ldr	r0, [sp, #68]                   @ 4-byte Reload
	adc	lr, r4, #0
	ldm	r7, {r4, r6, r7}
	adds	r4, r0, r4
	ldr	r3, [sp, #124]
	adcs	r4, r12, r6
	str	r4, [sp, #72]                   @ 4-byte Spill
	adcs	r1, r1, r7
	str	r1, [sp, #68]                   @ 4-byte Spill
	adcs	r2, r2, r3
	str	r2, [sp, #64]                   @ 4-byte Spill
	ldr	r3, [sp, #128]
	ldr	r0, [sp, #60]                   @ 4-byte Reload
	adcs	r11, r11, r3
	ldr	r3, [sp, #132]
	adcs	r10, r10, r3
	ldr	r3, [sp, #136]
	adcs	r9, r8, r3
	ldr	r3, [sp, #140]
	adcs	r12, r0, r3
	ldr	r3, [r5, #60]
	ldr	r0, [sp, #104]                  @ 4-byte Reload
	adc	r5, r3, lr
	subs	r8, r4, r0
	ldr	r0, [sp, #100]                  @ 4-byte Reload
	sbcs	lr, r1, r0
	ldr	r0, [sp, #96]                   @ 4-byte Reload
	ldr	r1, [sp, #76]                   @ 4-byte Reload
	sbcs	r6, r2, r0
	ldr	r0, [sp, #80]                   @ 4-byte Reload
	ldr	r2, [sp, #108]                  @ 4-byte Reload
	sbcs	r7, r11, r0
	ldr	r0, [sp, #84]                   @ 4-byte Reload
	sbcs	r3, r10, r0
	ldr	r0, [sp, #88]                   @ 4-byte Reload
	sbcs	r4, r9, r0
	ldr	r0, [sp, #92]                   @ 4-byte Reload
	sbcs	r0, r12, r0
	sbc	r1, r5, r1
	cmn	r1, #1
	movle	r0, r12
	movgt	r5, r1
	str	r0, [r2, #24]
	movle	r4, r9
	ldr	r0, [sp, #64]                   @ 4-byte Reload
	cmn	r1, #1
	movle	r3, r10
	movle	r7, r11
	str	r5, [r2, #28]
	movle	r6, r0
	ldr	r0, [sp, #68]                   @ 4-byte Reload
	cmn	r1, #1
	str	r4, [r2, #20]
	str	r3, [r2, #16]
	movle	lr, r0
	ldr	r0, [sp, #72]                   @ 4-byte Reload
	str	r7, [r2, #12]
	str	r6, [r2, #8]
	movle	r8, r0
	str	lr, [r2, #4]
	str	r8, [r2]
	add	sp, sp, #436
	pop	{r4, r5, r6, r7, r8, r9, r10, r11, lr}
	mov	pc, lr
.Lfunc_end46:
	.size	mcl_fp_montRedNF8L, .Lfunc_end46-mcl_fp_montRedNF8L
	.cantunwind
	.fnend
                                        @ -- End function
	.globl	mcl_fp_addPre8L                 @ -- Begin function mcl_fp_addPre8L
	.p2align	2
	.type	mcl_fp_addPre8L,%function
	.code	32                              @ @mcl_fp_addPre8L
mcl_fp_addPre8L:
	.fnstart
@ %bb.0:
	.save	{r4, r5, r6, r7, r8, lr}
	push	{r4, r5, r6, r7, r8, lr}
	ldm	r2, {r3, r12, lr}
	ldm	r1, {r5, r6, r7}
	adds	r3, r5, r3
	str	r3, [r0]
	adcs	r3, r6, r12
	ldr	r8, [r2, #12]
	ldr	r4, [r1, #12]
	str	r3, [r0, #4]
	adcs	r3, r7, lr
	str	r3, [r0, #8]
	adcs	r3, r4, r8
	str	r3, [r0, #12]
	ldr	r3, [r2, #16]
	ldr	r7, [r1, #16]
	adcs	r3, r7, r3
	str	r3, [r0, #16]
	ldr	r3, [r2, #20]
	ldr	r7, [r1, #20]
	adcs	r3, r7, r3
	str	r3, [r0, #20]
	ldr	r3, [r2, #24]
	ldr	r7, [r1, #24]
	ldr	r2, [r2, #28]
	ldr	r1, [r1, #28]
	adcs	r3, r7, r3
	str	r3, [r0, #24]
	adcs	r1, r1, r2
	str	r1, [r0, #28]
	mov	r0, #0
	adc	r0, r0, #0
	pop	{r4, r5, r6, r7, r8, lr}
	mov	pc, lr
.Lfunc_end47:
	.size	mcl_fp_addPre8L, .Lfunc_end47-mcl_fp_addPre8L
	.cantunwind
	.fnend
                                        @ -- End function
	.globl	mcl_fp_subPre8L                 @ -- Begin function mcl_fp_subPre8L
	.p2align	2
	.type	mcl_fp_subPre8L,%function
	.code	32                              @ @mcl_fp_subPre8L
mcl_fp_subPre8L:
	.fnstart
@ %bb.0:
	.save	{r4, r5, r6, r7, r8, lr}
	push	{r4, r5, r6, r7, r8, lr}
	ldm	r2, {r3, r12, lr}
	ldm	r1, {r5, r6, r7}
	subs	r3, r5, r3
	str	r3, [r0]
	sbcs	r3, r6, r12
	ldr	r8, [r2, #12]
	ldr	r4, [r1, #12]
	str	r3, [r0, #4]
	sbcs	r3, r7, lr
	str	r3, [r0, #8]
	sbcs	r3, r4, r8
	str	r3, [r0, #12]
	ldr	r3, [r2, #16]
	ldr	r7, [r1, #16]
	sbcs	r3, r7, r3
	str	r3, [r0, #16]
	ldr	r3, [r2, #20]
	ldr	r7, [r1, #20]
	sbcs	r3, r7, r3
	str	r3, [r0, #20]
	ldr	r3, [r2, #24]
	ldr	r7, [r1, #24]
	ldr	r2, [r2, #28]
	ldr	r1, [r1, #28]
	sbcs	r3, r7, r3
	str	r3, [r0, #24]
	sbcs	r1, r1, r2
	str	r1, [r0, #28]
	mov	r0, #0
	sbc	r0, r0, #0
	and	r0, r0, #1
	pop	{r4, r5, r6, r7, r8, lr}
	mov	pc, lr
.Lfunc_end48:
	.size	mcl_fp_subPre8L, .Lfunc_end48-mcl_fp_subPre8L
	.cantunwind
	.fnend
                                        @ -- End function
	.globl	mcl_fp_shr1_8L                  @ -- Begin function mcl_fp_shr1_8L
	.p2align	2
	.type	mcl_fp_shr1_8L,%function
	.code	32                              @ @mcl_fp_shr1_8L
mcl_fp_shr1_8L:
	.fnstart
@ %bb.0:
	.save	{r4, r5, r6, r7, r8, r9, r10, lr}
	push	{r4, r5, r6, r7, r8, r9, r10, lr}
	add	r6, r1, #8
	ldm	r1, {r12, lr}
	ldr	r1, [r1, #28]
	ldm	r6, {r2, r3, r4, r5, r6}
	lsr	r7, r5, #1
	orr	r8, r7, r6, lsl #31
	lsr	r10, r1, #1
	lsrs	r1, r1, #1
	lsr	r7, r3, #1
	rrx	r1, r6
	lsrs	r5, r5, #1
	orr	r9, r7, r4, lsl #31
	lsr	r7, lr, #1
	rrx	r4, r4
	lsrs	r3, r3, #1
	orr	r7, r7, r2, lsl #31
	rrx	r2, r2
	lsrs	r3, lr, #1
	rrx	r3, r12
	stm	r0, {r3, r7}
	str	r2, [r0, #8]
	str	r9, [r0, #12]
	str	r4, [r0, #16]
	str	r8, [r0, #20]
	str	r1, [r0, #24]
	str	r10, [r0, #28]
	pop	{r4, r5, r6, r7, r8, r9, r10, lr}
	mov	pc, lr
.Lfunc_end49:
	.size	mcl_fp_shr1_8L, .Lfunc_end49-mcl_fp_shr1_8L
	.cantunwind
	.fnend
                                        @ -- End function
	.globl	mcl_fp_add8L                    @ -- Begin function mcl_fp_add8L
	.p2align	2
	.type	mcl_fp_add8L,%function
	.code	32                              @ @mcl_fp_add8L
mcl_fp_add8L:
	.fnstart
@ %bb.0:
	.save	{r4, r5, r6, r7, r8, r9, r10, r11, lr}
	push	{r4, r5, r6, r7, r8, r9, r10, r11, lr}
	.pad	#20
	sub	sp, sp, #20
	ldm	r2, {r12, lr}
	ldr	r7, [r1]
	ldmib	r1, {r4, r5, r6}
	adds	r12, r7, r12
	ldr	r9, [r2, #8]
	adcs	lr, r4, lr
	ldr	r8, [r2, #12]
	adcs	r10, r5, r9
	ldr	r5, [r2, #16]
	ldr	r4, [r1, #16]
	adcs	r9, r6, r8
	ldr	r7, [r2, #20]
	adcs	r8, r4, r5
	ldr	r4, [r1, #20]
	ldr	r6, [r1, #24]
	adcs	r7, r4, r7
	ldr	r4, [r2, #24]
	str	r12, [sp, #16]                  @ 4-byte Spill
	ldr	r2, [r2, #28]
	adcs	r6, r6, r4
	ldr	r1, [r1, #28]
	stm	r0, {r12, lr}
	adcs	r5, r1, r2
	str	r9, [sp, #4]                    @ 4-byte Spill
	mov	r2, #0
	str	r9, [r0, #12]
	adc	r1, r2, #0
	ldm	r3, {r4, r11}
	ldr	r9, [sp, #16]                   @ 4-byte Reload
	str	lr, [sp, #12]                   @ 4-byte Spill
	add	lr, r3, #12
	subs	r9, r9, r4
	ldr	r4, [sp, #12]                   @ 4-byte Reload
	str	r10, [sp, #8]                   @ 4-byte Spill
	str	r10, [r0, #8]
	sbcs	r11, r4, r11
	ldr	r10, [r3, #8]
	ldr	r4, [sp, #8]                    @ 4-byte Reload
	str	r1, [sp]                        @ 4-byte Spill
	sbcs	r10, r4, r10
	ldm	lr, {r1, r2, r12, lr}
	ldr	r4, [sp, #4]                    @ 4-byte Reload
	ldr	r3, [r3, #28]
	sbcs	r4, r4, r1
	str	r7, [r0, #20]
	sbcs	r1, r8, r2
	str	r6, [r0, #24]
	sbcs	r2, r7, r12
	str	r8, [r0, #16]
	sbcs	r7, r6, lr
	str	r5, [r0, #28]
	sbcs	r6, r5, r3
	ldr	r3, [sp]                        @ 4-byte Reload
	sbc	r3, r3, #0
	tst	r3, #1
	bne	.LBB50_2
@ %bb.1:                                @ %nocarry
	add	r3, r0, #16
	stm	r0, {r9, r11}
	str	r10, [r0, #8]
	str	r4, [r0, #12]
	stm	r3, {r1, r2, r7}
	str	r6, [r0, #28]
.LBB50_2:                               @ %carry
	add	sp, sp, #20
	pop	{r4, r5, r6, r7, r8, r9, r10, r11, lr}
	mov	pc, lr
.Lfunc_end50:
	.size	mcl_fp_add8L, .Lfunc_end50-mcl_fp_add8L
	.cantunwind
	.fnend
                                        @ -- End function
	.globl	mcl_fp_addNF8L                  @ -- Begin function mcl_fp_addNF8L
	.p2align	2
	.type	mcl_fp_addNF8L,%function
	.code	32                              @ @mcl_fp_addNF8L
mcl_fp_addNF8L:
	.fnstart
@ %bb.0:
	.save	{r4, r5, r6, r7, r8, r9, r10, r11, lr}
	push	{r4, r5, r6, r7, r8, r9, r10, r11, lr}
	.pad	#28
	sub	sp, sp, #28
	ldm	r1, {r12, lr}
	ldr	r7, [r2]
	ldmib	r2, {r4, r5, r6}
	adds	r9, r7, r12
	ldr	r8, [r1, #8]
	adcs	r10, r4, lr
	ldr	r11, [r1, #12]
	adcs	lr, r5, r8
	ldr	r4, [r1, #16]
	adcs	r12, r6, r11
	ldr	r6, [r2, #16]
	ldr	r7, [r1, #20]
	adcs	r4, r6, r4
	str	r4, [sp, #12]                   @ 4-byte Spill
	ldr	r4, [r2, #20]
	ldr	r5, [r2, #24]
	adcs	r4, r4, r7
	ldr	r7, [r1, #24]
	ldr	r1, [r1, #28]
	ldr	r2, [r2, #28]
	str	r4, [sp, #8]                    @ 4-byte Spill
	adcs	r4, r5, r7
	adc	r2, r2, r1
	ldm	r3, {r1, r7}
	str	r9, [sp, #24]                   @ 4-byte Spill
	subs	r9, r9, r1
	ldr	r5, [r3, #16]
	sbcs	r8, r10, r7
	ldr	r6, [r3, #8]
	str	r5, [sp]                        @ 4-byte Spill
	str	r4, [sp, #4]                    @ 4-byte Spill
	sbcs	r6, lr, r6
	ldr	r4, [r3, #12]
	ldr	r7, [sp, #12]                   @ 4-byte Reload
	ldr	r1, [sp]                        @ 4-byte Reload
	sbcs	r4, r12, r4
	str	r10, [sp, #20]                  @ 4-byte Spill
	mov	r10, r12
	sbcs	r12, r7, r1
	ldr	r11, [r3, #20]
	ldr	r1, [sp, #8]                    @ 4-byte Reload
	str	lr, [sp, #16]                   @ 4-byte Spill
	ldr	r5, [r3, #24]
	sbcs	lr, r1, r11
	ldr	r11, [sp, #4]                   @ 4-byte Reload
	ldr	r3, [r3, #28]
	sbcs	r5, r11, r5
	sbc	r3, r2, r3
	cmn	r3, #1
	movle	lr, r1
	ldr	r1, [sp, #16]                   @ 4-byte Reload
	movgt	r2, r3
	movle	r5, r11
	cmn	r3, #1
	str	r2, [r0, #28]
	movle	r6, r1
	ldr	r1, [sp, #20]                   @ 4-byte Reload
	movle	r12, r7
	movle	r4, r10
	cmn	r3, #1
	str	r5, [r0, #24]
	movle	r8, r1
	ldr	r1, [sp, #24]                   @ 4-byte Reload
	str	lr, [r0, #20]
	str	r12, [r0, #16]
	movle	r9, r1
	str	r4, [r0, #12]
	str	r6, [r0, #8]
	str	r8, [r0, #4]
	str	r9, [r0]
	add	sp, sp, #28
	pop	{r4, r5, r6, r7, r8, r9, r10, r11, lr}
	mov	pc, lr
.Lfunc_end51:
	.size	mcl_fp_addNF8L, .Lfunc_end51-mcl_fp_addNF8L
	.cantunwind
	.fnend
                                        @ -- End function
	.globl	mcl_fp_sub8L                    @ -- Begin function mcl_fp_sub8L
	.p2align	2
	.type	mcl_fp_sub8L,%function
	.code	32                              @ @mcl_fp_sub8L
mcl_fp_sub8L:
	.fnstart
@ %bb.0:
	.save	{r4, r5, r6, r7, r8, r9, r10, r11, lr}
	push	{r4, r5, r6, r7, r8, r9, r10, r11, lr}
	ldr	r9, [r2]
	ldm	r1, {r4, r5, r6, r7}
	ldmib	r2, {r8, lr}
	subs	r4, r4, r9
	sbcs	r9, r5, r8
	ldr	r12, [r2, #12]
	sbcs	r10, r6, lr
	ldr	r6, [r2, #16]
	sbcs	r11, r7, r12
	ldr	r7, [r1, #16]
	ldr	r5, [r2, #20]
	sbcs	r8, r7, r6
	ldr	r7, [r1, #20]
	ldr	r6, [r1, #24]
	sbcs	r12, r7, r5
	ldr	r7, [r2, #24]
	ldr	r2, [r2, #28]
	ldr	r1, [r1, #28]
	sbcs	lr, r6, r7
	stm	r0, {r4, r9, r10, r11}
	sbcs	r1, r1, r2
	add	r2, r0, #16
	stm	r2, {r8, r12, lr}
	mov	r2, #0
	sbc	r2, r2, #0
	tst	r2, #1
	str	r1, [r0, #28]
	bne	.LBB52_2
@ %bb.1:                                @ %nocarry
	pop	{r4, r5, r6, r7, r8, r9, r10, r11, lr}
	mov	pc, lr
.LBB52_2:                               @ %carry
	ldm	r3, {r2, r5, r6, r7}
	adds	r2, r2, r4
	str	r2, [r0]
	adcs	r2, r5, r9
	str	r2, [r0, #4]
	adcs	r2, r6, r10
	str	r2, [r0, #8]
	adcs	r2, r7, r11
	str	r2, [r0, #12]
	ldr	r2, [r3, #16]
	adcs	r2, r2, r8
	str	r2, [r0, #16]
	ldr	r2, [r3, #20]
	adcs	r2, r2, r12
	str	r2, [r0, #20]
	ldr	r2, [r3, #24]
	adcs	r2, r2, lr
	str	r2, [r0, #24]
	ldr	r2, [r3, #28]
	adc	r1, r2, r1
	str	r1, [r0, #28]
	pop	{r4, r5, r6, r7, r8, r9, r10, r11, lr}
	mov	pc, lr
.Lfunc_end52:
	.size	mcl_fp_sub8L, .Lfunc_end52-mcl_fp_sub8L
	.cantunwind
	.fnend
                                        @ -- End function
	.globl	mcl_fp_subNF8L                  @ -- Begin function mcl_fp_subNF8L
	.p2align	2
	.type	mcl_fp_subNF8L,%function
	.code	32                              @ @mcl_fp_subNF8L
mcl_fp_subNF8L:
	.fnstart
@ %bb.0:
	.save	{r4, r5, r6, r7, r8, r9, r10, r11, lr}
	push	{r4, r5, r6, r7, r8, r9, r10, r11, lr}
	.pad	#36
	sub	sp, sp, #36
	ldr	r7, [r2, #8]
	str	r7, [sp, #20]                   @ 4-byte Spill
	ldr	r7, [r2, #16]
	str	r7, [sp, #24]                   @ 4-byte Spill
	ldr	r7, [r2, #20]
	str	r7, [sp, #28]                   @ 4-byte Spill
	ldr	r7, [r2, #24]
	ldm	r2, {r6, r8}
	ldr	r11, [r2, #12]
	ldr	r9, [r2, #28]
	ldm	r1, {r2, r4, r5, r12, lr}
	subs	r6, r2, r6
	ldr	r2, [sp, #20]                   @ 4-byte Reload
	sbcs	r8, r4, r8
	ldr	r4, [sp, #24]                   @ 4-byte Reload
	sbcs	r5, r5, r2
	ldr	r10, [r1, #20]
	sbcs	r2, r12, r11
	str	r7, [sp, #32]                   @ 4-byte Spill
	sbcs	r12, lr, r4
	ldr	r4, [sp, #28]                   @ 4-byte Reload
	ldr	r7, [r1, #24]
	sbcs	lr, r10, r4
	ldr	r4, [sp, #32]                   @ 4-byte Reload
	ldr	r1, [r1, #28]
	sbcs	r4, r7, r4
	ldr	r7, [r3]
	str	r7, [sp, #4]                    @ 4-byte Spill
	sbc	r1, r1, r9
	ldr	r7, [r3, #4]
	str	r7, [sp]                        @ 4-byte Spill
	ldr	r7, [r3, #20]
	str	r7, [sp, #8]                    @ 4-byte Spill
	ldr	r7, [r3, #24]
	ldr	r9, [r3, #8]
	ldr	r11, [r3, #12]
	ldr	r10, [r3, #16]
	ldr	r3, [r3, #28]
	str	r3, [sp, #12]                   @ 4-byte Spill
	ldr	r3, [sp, #4]                    @ 4-byte Reload
	str	r7, [sp, #32]                   @ 4-byte Spill
	adds	r7, r6, r3
	ldr	r3, [sp]                        @ 4-byte Reload
	str	r6, [sp, #16]                   @ 4-byte Spill
	adcs	r6, r8, r3
	ldr	r3, [sp, #8]                    @ 4-byte Reload
	adcs	r9, r5, r9
	str	r2, [sp, #20]                   @ 4-byte Spill
	adcs	r11, r2, r11
	str	r12, [sp, #24]                  @ 4-byte Spill
	adcs	r2, r12, r10
	str	lr, [sp, #28]                   @ 4-byte Spill
	adcs	r12, lr, r3
	ldr	r3, [sp, #32]                   @ 4-byte Reload
	ldr	r10, [sp, #16]                  @ 4-byte Reload
	adcs	lr, r4, r3
	ldr	r3, [sp, #12]                   @ 4-byte Reload
	adc	r3, r1, r3
	cmp	r1, #0
	movpl	r9, r5
	ldr	r5, [sp, #20]                   @ 4-byte Reload
	movpl	r7, r10
	movpl	r6, r8
	cmp	r1, #0
	str	r7, [r0]
	movpl	r11, r5
	ldr	r5, [sp, #24]                   @ 4-byte Reload
	stmib	r0, {r6, r9, r11}
	movpl	r2, r5
	ldr	r5, [sp, #28]                   @ 4-byte Reload
	movpl	r12, r5
	cmp	r1, #0
	movpl	r3, r1
	add	r1, r0, #16
	movpl	lr, r4
	stm	r1, {r2, r12, lr}
	str	r3, [r0, #28]
	add	sp, sp, #36
	pop	{r4, r5, r6, r7, r8, r9, r10, r11, lr}
	mov	pc, lr
.Lfunc_end53:
	.size	mcl_fp_subNF8L, .Lfunc_end53-mcl_fp_subNF8L
	.cantunwind
	.fnend
                                        @ -- End function
	.globl	mcl_fpDbl_add8L                 @ -- Begin function mcl_fpDbl_add8L
	.p2align	2
	.type	mcl_fpDbl_add8L,%function
	.code	32                              @ @mcl_fpDbl_add8L
mcl_fpDbl_add8L:
	.fnstart
@ %bb.0:
	.save	{r4, r5, r6, r7, r8, r9, r10, r11, lr}
	push	{r4, r5, r6, r7, r8, r9, r10, r11, lr}
	.pad	#60
	sub	sp, sp, #60
	ldr	r9, [r2]
	ldm	r1, {r4, r5, r6, r7}
	ldmib	r2, {r8, lr}
	adds	r4, r4, r9
	str	r4, [sp, #52]                   @ 4-byte Spill
	adcs	r4, r5, r8
	ldr	r12, [r2, #12]
	adcs	r6, r6, lr
	str	r6, [sp, #44]                   @ 4-byte Spill
	adcs	r7, r7, r12
	str	r7, [sp, #40]                   @ 4-byte Spill
	ldr	r6, [r2, #16]
	ldr	r7, [r1, #16]
	str	r4, [sp, #48]                   @ 4-byte Spill
	adcs	r7, r7, r6
	str	r7, [sp, #36]                   @ 4-byte Spill
	ldr	r4, [r2, #20]
	ldr	r7, [r1, #20]
	ldr	r6, [r1, #24]
	adcs	r7, r7, r4
	str	r7, [sp, #32]                   @ 4-byte Spill
	ldr	r7, [r2, #24]
	ldr	r5, [r1, #48]
	adcs	r7, r6, r7
	str	r7, [sp, #24]                   @ 4-byte Spill
	ldr	r7, [r2, #28]
	ldr	r6, [r1, #28]
	ldr	r4, [r1, #52]
	adcs	r7, r6, r7
	str	r7, [sp, #20]                   @ 4-byte Spill
	ldr	r7, [r2, #32]
	ldr	r6, [r1, #32]
	adcs	r10, r6, r7
	ldr	r7, [r2, #36]
	ldr	r6, [r1, #36]
	str	r10, [sp, #28]                  @ 4-byte Spill
	adcs	r7, r6, r7
	str	r7, [sp, #56]                   @ 4-byte Spill
	ldr	r7, [r2, #40]
	ldr	r6, [r1, #40]
	adcs	r9, r6, r7
	ldr	r7, [r2, #44]
	ldr	r6, [r1, #44]
	str	r9, [sp, #16]                   @ 4-byte Spill
	adcs	r8, r6, r7
	ldr	r7, [r2, #48]
	ldr	r6, [r1, #56]
	adcs	r7, r5, r7
	ldr	r5, [r2, #52]
	ldr	r1, [r1, #60]
	adcs	r5, r4, r5
	ldr	r4, [r2, #56]
	ldr	r2, [r2, #60]
	str	r7, [sp, #12]                   @ 4-byte Spill
	adcs	r7, r6, r4
	adcs	r1, r1, r2
	str	r1, [sp]                        @ 4-byte Spill
	mov	r1, #0
	str	r7, [sp, #4]                    @ 4-byte Spill
	adc	r7, r1, #0
	ldm	r3, {r1, r2, r12, lr}
	subs	r1, r10, r1
	str	r1, [sp, #8]                    @ 4-byte Spill
	ldr	r1, [sp, #56]                   @ 4-byte Reload
	ldr	r4, [r3, #28]
	sbcs	r11, r1, r2
	ldr	r1, [sp, #52]                   @ 4-byte Reload
	str	r1, [r0]
	sbcs	r10, r9, r12
	ldr	r1, [sp, #48]                   @ 4-byte Reload
	sbcs	r9, r8, lr
	str	r1, [r0, #4]
	ldr	r1, [sp, #44]                   @ 4-byte Reload
	str	r1, [r0, #8]
	ldr	r1, [sp, #40]                   @ 4-byte Reload
	str	r1, [r0, #12]
	ldr	r1, [sp, #36]                   @ 4-byte Reload
	str	r1, [r0, #16]
	ldr	r1, [sp, #32]                   @ 4-byte Reload
	str	r1, [r0, #20]
	ldr	r1, [sp, #24]                   @ 4-byte Reload
	str	r1, [r0, #24]
	ldr	r1, [sp, #20]                   @ 4-byte Reload
	str	r1, [r0, #28]
	ldr	lr, [r3, #24]
	ldr	r1, [r3, #20]
	ldr	r3, [r3, #16]
	ldr	r6, [sp, #12]                   @ 4-byte Reload
	ldr	r12, [sp, #4]                   @ 4-byte Reload
	sbcs	r3, r6, r3
	ldr	r2, [sp]                        @ 4-byte Reload
	sbcs	r1, r5, r1
	sbcs	lr, r12, lr
	sbcs	r4, r2, r4
	sbc	r7, r7, #0
	ands	r7, r7, #1
	movne	r1, r5
	movne	r4, r2
	str	r1, [r0, #52]
	movne	lr, r12
	ldr	r1, [sp, #16]                   @ 4-byte Reload
	cmp	r7, #0
	movne	r3, r6
	movne	r9, r8
	ldr	r2, [sp, #8]                    @ 4-byte Reload
	movne	r10, r1
	ldr	r1, [sp, #56]                   @ 4-byte Reload
	cmp	r7, #0
	str	r4, [r0, #60]
	str	lr, [r0, #56]
	movne	r11, r1
	ldr	r1, [sp, #28]                   @ 4-byte Reload
	str	r3, [r0, #48]
	str	r9, [r0, #44]
	movne	r2, r1
	str	r10, [r0, #40]
	str	r11, [r0, #36]
	str	r2, [r0, #32]
	add	sp, sp, #60
	pop	{r4, r5, r6, r7, r8, r9, r10, r11, lr}
	mov	pc, lr
.Lfunc_end54:
	.size	mcl_fpDbl_add8L, .Lfunc_end54-mcl_fpDbl_add8L
	.cantunwind
	.fnend
                                        @ -- End function
	.globl	mcl_fpDbl_sub8L                 @ -- Begin function mcl_fpDbl_sub8L
	.p2align	2
	.type	mcl_fpDbl_sub8L,%function
	.code	32                              @ @mcl_fpDbl_sub8L
mcl_fpDbl_sub8L:
	.fnstart
@ %bb.0:
	.save	{r4, r5, r6, r7, r8, r9, r10, r11, lr}
	push	{r4, r5, r6, r7, r8, r9, r10, r11, lr}
	.pad	#68
	sub	sp, sp, #68
	ldr	r7, [r2, #32]
	add	r11, r2, #16
	str	r7, [sp, #64]                   @ 4-byte Spill
	ldr	r7, [r2, #36]
	str	r7, [sp, #44]                   @ 4-byte Spill
	ldr	r7, [r2, #40]
	str	r7, [sp, #36]                   @ 4-byte Spill
	ldr	r7, [r2, #44]
	str	r7, [sp, #40]                   @ 4-byte Spill
	ldr	r7, [r2, #48]
	str	r7, [sp, #48]                   @ 4-byte Spill
	ldr	r7, [r2, #52]
	str	r7, [sp, #52]                   @ 4-byte Spill
	ldr	r7, [r2, #56]
	str	r7, [sp, #56]                   @ 4-byte Spill
	ldr	r7, [r2, #60]
	str	r7, [sp, #60]                   @ 4-byte Spill
	ldr	r6, [r2]
	ldmib	r2, {r5, r7, r9}
	ldr	r2, [r2, #28]
	str	r2, [sp, #4]                    @ 4-byte Spill
	ldm	r1, {r2, r12, lr}
	subs	r2, r2, r6
	str	r2, [sp, #32]                   @ 4-byte Spill
	ldr	r2, [r1, #60]
	str	r2, [sp]                        @ 4-byte Spill
	sbcs	r2, r12, r5
	ldr	r4, [r1, #12]
	str	r2, [sp, #28]                   @ 4-byte Spill
	sbcs	r2, lr, r7
	str	r2, [sp, #24]                   @ 4-byte Spill
	sbcs	r2, r4, r9
	str	r2, [sp, #20]                   @ 4-byte Spill
	ldm	r11, {r8, r10, r11}
	ldr	r2, [r1, #16]
	ldr	r4, [sp, #4]                    @ 4-byte Reload
	sbcs	r2, r2, r8
	str	r2, [sp, #16]                   @ 4-byte Spill
	ldr	r2, [r1, #20]
	mov	r8, #0
	ldr	r5, [sp, #44]                   @ 4-byte Reload
	sbcs	r2, r2, r10
	str	r2, [sp, #12]                   @ 4-byte Spill
	ldr	r2, [r1, #24]
	ldr	r12, [r1, #56]
	sbcs	r2, r2, r11
	str	r2, [sp, #8]                    @ 4-byte Spill
	ldr	r2, [r1, #28]
	ldr	lr, [r1, #52]
	sbcs	r2, r2, r4
	str	r2, [sp, #4]                    @ 4-byte Spill
	ldr	r2, [r1, #32]
	ldr	r4, [sp, #64]                   @ 4-byte Reload
	ldr	r6, [sp, #40]                   @ 4-byte Reload
	sbcs	r4, r2, r4
	ldr	r2, [r1, #36]
	str	r4, [sp, #64]                   @ 4-byte Spill
	sbcs	r7, r2, r5
	ldr	r2, [r1, #40]
	ldr	r5, [sp, #36]                   @ 4-byte Reload
	str	r7, [sp, #44]                   @ 4-byte Spill
	sbcs	r2, r2, r5
	ldr	r5, [r1, #48]
	ldr	r1, [r1, #44]
	str	r2, [sp, #36]                   @ 4-byte Spill
	sbcs	r1, r1, r6
	ldr	r6, [sp, #48]                   @ 4-byte Reload
	str	r1, [sp, #40]                   @ 4-byte Spill
	sbcs	r11, r5, r6
	ldr	r5, [sp, #52]                   @ 4-byte Reload
	ldr	r6, [sp]                        @ 4-byte Reload
	sbcs	r10, lr, r5
	ldr	r5, [sp, #56]                   @ 4-byte Reload
	sbcs	r9, r12, r5
	ldr	r5, [sp, #60]                   @ 4-byte Reload
	sbcs	r6, r6, r5
	sbc	r5, r8, #0
	ldr	r8, [r3]
	str	r5, [sp, #60]                   @ 4-byte Spill
	ldmib	r3, {r5, r12, lr}
	adds	r8, r4, r8
	adcs	r5, r7, r5
	ldr	r4, [sp, #60]                   @ 4-byte Reload
	adcs	r7, r2, r12
	adcs	r2, r1, lr
	ldr	r1, [sp, #32]                   @ 4-byte Reload
	str	r1, [r0]
	add	lr, r3, #20
	ldr	r1, [sp, #28]                   @ 4-byte Reload
	str	r1, [r0, #4]
	ldr	r1, [sp, #24]                   @ 4-byte Reload
	str	r1, [r0, #8]
	ldr	r1, [sp, #20]                   @ 4-byte Reload
	str	r1, [r0, #12]
	ldr	r1, [sp, #16]                   @ 4-byte Reload
	str	r1, [r0, #16]
	ldr	r1, [sp, #12]                   @ 4-byte Reload
	str	r1, [r0, #20]
	ldr	r1, [sp, #8]                    @ 4-byte Reload
	ldr	r3, [r3, #16]
	str	r1, [r0, #24]
	ldr	r1, [sp, #4]                    @ 4-byte Reload
	adcs	r3, r11, r3
	str	r1, [r0, #28]
	ldm	lr, {r1, r12, lr}
	adcs	r1, r10, r1
	adcs	r12, r9, r12
	adc	lr, r6, lr
	ands	r4, r4, #1
	moveq	r1, r10
	moveq	lr, r6
	str	r1, [r0, #52]
	moveq	r12, r9
	ldr	r1, [sp, #40]                   @ 4-byte Reload
	cmp	r4, #0
	moveq	r3, r11
	str	lr, [r0, #60]
	str	r12, [r0, #56]
	moveq	r2, r1
	ldr	r1, [sp, #36]                   @ 4-byte Reload
	str	r3, [r0, #48]
	str	r2, [r0, #44]
	moveq	r7, r1
	ldr	r1, [sp, #44]                   @ 4-byte Reload
	cmp	r4, #0
	str	r7, [r0, #40]
	moveq	r5, r1
	ldr	r1, [sp, #64]                   @ 4-byte Reload
	str	r5, [r0, #36]
	moveq	r8, r1
	str	r8, [r0, #32]
	add	sp, sp, #68
	pop	{r4, r5, r6, r7, r8, r9, r10, r11, lr}
	mov	pc, lr
.Lfunc_end55:
	.size	mcl_fpDbl_sub8L, .Lfunc_end55-mcl_fpDbl_sub8L
	.cantunwind
	.fnend
                                        @ -- End function
	.globl	mulPv384x32                     @ -- Begin function mulPv384x32
	.p2align	2
	.type	mulPv384x32,%function
	.code	32                              @ @mulPv384x32
mulPv384x32:
	.fnstart
@ %bb.0:
	.save	{r4, r5, r6, r7, r8, r9, r11, lr}
	push	{r4, r5, r6, r7, r8, r9, r11, lr}
	ldr	r12, [r1]
	ldmib	r1, {r3, lr}
	umull	r7, r6, r12, r2
	ldr	r4, [r1, #12]
	umull	r5, r8, r3, r2
	str	r7, [r0]
	umull	r9, r12, r4, r2
	adds	r7, r6, r5
	umull	r5, r4, lr, r2
	adcs	r7, r8, r5
	umlal	r6, r5, r3, r2
	adcs	r7, r4, r9
	str	r7, [r0, #12]
	str	r5, [r0, #8]
	str	r6, [r0, #4]
	ldr	r3, [r1, #16]
	umull	r7, r6, r3, r2
	adcs	r3, r12, r7
	str	r3, [r0, #16]
	ldr	r3, [r1, #20]
	umull	r7, r5, r3, r2
	adcs	r3, r6, r7
	str	r3, [r0, #20]
	ldr	r3, [r1, #24]
	umull	r7, r6, r3, r2
	adcs	r3, r5, r7
	str	r3, [r0, #24]
	ldr	r3, [r1, #28]
	umull	r7, r5, r3, r2
	adcs	r3, r6, r7
	str	r3, [r0, #28]
	ldr	r3, [r1, #32]
	umull	r7, r6, r3, r2
	adcs	r3, r5, r7
	str	r3, [r0, #32]
	ldr	r3, [r1, #36]
	umull	r7, r5, r3, r2
	adcs	r3, r6, r7
	str	r3, [r0, #36]
	ldr	r3, [r1, #40]
	umull	r7, r6, r3, r2
	adcs	r3, r5, r7
	str	r3, [r0, #40]
	ldr	r1, [r1, #44]
	umull	r3, r7, r1, r2
	adcs	r1, r6, r3
	str	r1, [r0, #44]
	adc	r1, r7, #0
	str	r1, [r0, #48]
	pop	{r4, r5, r6, r7, r8, r9, r11, lr}
	mov	pc, lr
.Lfunc_end56:
	.size	mulPv384x32, .Lfunc_end56-mulPv384x32
	.cantunwind
	.fnend
                                        @ -- End function
	.globl	mcl_fp_mulUnitPre12L            @ -- Begin function mcl_fp_mulUnitPre12L
	.p2align	2
	.type	mcl_fp_mulUnitPre12L,%function
	.code	32                              @ @mcl_fp_mulUnitPre12L
mcl_fp_mulUnitPre12L:
	.fnstart
@ %bb.0:
	.save	{r4, r5, r6, r7, r8, r9, r10, r11, lr}
	push	{r4, r5, r6, r7, r8, r9, r10, r11, lr}
	.pad	#60
	sub	sp, sp, #60
	mov	r4, r0
	mov	r0, sp
	bl	mulPv384x32
	add	r7, sp, #24
	add	r3, sp, #36
	ldm	sp, {r8, r9, r10, r11, r12, lr}
	ldm	r7, {r5, r6, r7}
	ldm	r3, {r0, r1, r2, r3}
	str	r0, [r4, #36]
	add	r0, r4, #24
	str	r1, [r4, #40]
	str	r2, [r4, #44]
	str	r3, [r4, #48]
	stm	r4, {r8, r9, r10, r11, r12, lr}
	stm	r0, {r5, r6, r7}
	add	sp, sp, #60
	pop	{r4, r5, r6, r7, r8, r9, r10, r11, lr}
	mov	pc, lr
.Lfunc_end57:
	.size	mcl_fp_mulUnitPre12L, .Lfunc_end57-mcl_fp_mulUnitPre12L
	.cantunwind
	.fnend
                                        @ -- End function
	.globl	mcl_fpDbl_mulPre12L             @ -- Begin function mcl_fpDbl_mulPre12L
	.p2align	2
	.type	mcl_fpDbl_mulPre12L,%function
	.code	32                              @ @mcl_fpDbl_mulPre12L
mcl_fpDbl_mulPre12L:
	.fnstart
@ %bb.0:
	.save	{r4, r5, r6, r7, r8, r9, r10, r11, lr}
	push	{r4, r5, r6, r7, r8, r9, r10, r11, lr}
	.pad	#196
	sub	sp, sp, #196
	mov	r6, r2
	mov	r5, r1
	mov	r4, r0
	bl	mcl_fpDbl_mulPre6L
	add	r0, r4, #48
	add	r1, r5, #24
	add	r2, r6, #24
	bl	mcl_fpDbl_mulPre6L
	add	r7, r6, #24
	ldr	lr, [r6]
	ldmib	r6, {r8, r9, r10, r11, r12}
	ldm	r7, {r0, r1, r2, r3, r7}
	adds	lr, r0, lr
	ldr	r6, [r6, #44]
	adcs	r0, r1, r8
	str	r0, [sp, #48]                   @ 4-byte Spill
	adcs	r0, r2, r9
	str	r0, [sp, #56]                   @ 4-byte Spill
	adcs	r0, r3, r10
	str	r0, [sp, #60]                   @ 4-byte Spill
	adcs	r0, r7, r11
	str	r0, [sp, #68]                   @ 4-byte Spill
	adcs	r0, r6, r12
	str	r0, [sp, #76]                   @ 4-byte Spill
	mov	r0, #0
	ldr	r1, [r5, #16]
	adc	r0, r0, #0
	str	r0, [sp, #96]                   @ 4-byte Spill
	str	r1, [sp, #64]                   @ 4-byte Spill
	add	r9, r5, #36
	ldr	r1, [r5, #20]
	ldm	r5, {r0, r8, r12}
	ldr	r7, [r5, #24]
	str	r1, [sp, #52]                   @ 4-byte Spill
	ldr	r1, [r5, #28]
	adds	r0, r7, r0
	ldr	r10, [r5, #32]
	ldr	r6, [r5, #12]
	ldm	r9, {r2, r3, r9}
	str	r0, [sp, #92]                   @ 4-byte Spill
	str	r0, [sp, #124]
	adcs	r0, r1, r8
	str	r0, [sp, #88]                   @ 4-byte Spill
	add	r1, sp, #124
	str	r0, [sp, #128]
	adcs	r0, r10, r12
	str	r0, [sp, #80]                   @ 4-byte Spill
	str	r0, [sp, #132]
	adcs	r0, r2, r6
	str	r0, [sp, #72]                   @ 4-byte Spill
	add	r2, sp, #100
	str	r0, [sp, #136]
	ldr	r0, [sp, #64]                   @ 4-byte Reload
	ldr	r11, [sp, #48]                  @ 4-byte Reload
	adcs	r0, r3, r0
	str	r0, [sp, #64]                   @ 4-byte Spill
	str	r0, [sp, #140]
	ldr	r0, [sp, #52]                   @ 4-byte Reload
	ldr	r8, [sp, #56]                   @ 4-byte Reload
	adcs	r10, r9, r0
	mov	r0, #0
	adc	r9, r0, #0
	add	r0, sp, #148
	ldr	r5, [sp, #60]                   @ 4-byte Reload
	ldr	r7, [sp, #68]                   @ 4-byte Reload
	ldr	r6, [sp, #76]                   @ 4-byte Reload
	str	lr, [sp, #84]                   @ 4-byte Spill
	str	lr, [sp, #100]
	str	r11, [sp, #104]
	str	r8, [sp, #108]
	str	r5, [sp, #112]
	str	r7, [sp, #116]
	str	r10, [sp, #144]
	str	r6, [sp, #120]
	bl	mcl_fpDbl_mulPre6L
	rsb	r0, r9, #0
	mov	r1, r9
	str	r9, [sp, #52]                   @ 4-byte Spill
	and	r2, r6, r0
	and	r9, r7, r0
	and	r12, r5, r0
	and	r7, r8, r0
	and	lr, r11, r0
	ldr	r0, [sp, #84]                   @ 4-byte Reload
	sub	r6, r1, r1, lsl #1
	and	r6, r0, r6
	ldr	r0, [sp, #92]                   @ 4-byte Reload
	adds	r1, r6, r0
	ldr	r0, [sp, #88]                   @ 4-byte Reload
	adcs	r5, lr, r0
	ldr	r0, [sp, #80]                   @ 4-byte Reload
	adcs	r3, r7, r0
	ldr	r0, [sp, #72]                   @ 4-byte Reload
	adcs	r11, r12, r0
	ldr	r0, [sp, #64]                   @ 4-byte Reload
	adcs	r8, r9, r0
	mov	r0, #0
	adcs	r10, r2, r10
	adc	r0, r0, #0
	str	r0, [sp, #92]                   @ 4-byte Spill
	ldr	r0, [sp, #96]                   @ 4-byte Reload
	cmp	r0, #0
	moveq	r3, r7
	moveq	r5, lr
	moveq	r1, r6
	moveq	r11, r12
	moveq	r8, r9
	cmp	r0, #0
	moveq	r10, r2
	ldr	r2, [sp, #92]                   @ 4-byte Reload
	ldr	r7, [sp, #172]
	and	r12, r0, r2
	ldr	r2, [sp, #52]                   @ 4-byte Reload
	ldr	r6, [sp, #180]
	and	r2, r0, r2
	adds	r0, r7, r1
	ldr	r7, [sp, #176]
	str	r0, [sp, #88]                   @ 4-byte Spill
	adcs	r0, r7, r5
	str	r0, [sp, #84]                   @ 4-byte Spill
	adcs	r0, r6, r3
	ldr	r6, [sp, #184]
	ldr	r5, [sp, #188]
	adcs	r11, r6, r11
	ldr	r1, [sp, #192]
	str	r0, [sp, #80]                   @ 4-byte Spill
	adcs	r0, r5, r8
	str	r0, [sp, #76]                   @ 4-byte Spill
	adcs	r0, r1, r10
	str	r0, [sp, #72]                   @ 4-byte Spill
	adc	r0, r2, r12
	ldm	r4, {r5, r8}
	add	r10, r4, #12
	ldr	r12, [sp, #148]
	ldr	lr, [sp, #152]
	subs	r5, r12, r5
	ldr	r7, [r4, #8]
	ldr	r2, [sp, #156]
	sbcs	r12, lr, r8
	ldm	r10, {r6, r9, r10}
	sbcs	r2, r2, r7
	ldr	r3, [sp, #160]
	str	r0, [sp, #68]                   @ 4-byte Spill
	sbcs	r3, r3, r6
	ldr	r6, [sp, #164]
	ldr	r0, [r4, #24]
	sbcs	r7, r6, r9
	str	r7, [sp, #64]                   @ 4-byte Spill
	ldr	r7, [sp, #168]
	str	r0, [sp, #96]                   @ 4-byte Spill
	sbcs	r7, r7, r10
	str	r7, [sp, #60]                   @ 4-byte Spill
	ldr	r7, [sp, #88]                   @ 4-byte Reload
	ldr	r1, [r4, #28]
	sbcs	r0, r7, r0
	str	r0, [sp, #56]                   @ 4-byte Spill
	ldr	r0, [sp, #84]                   @ 4-byte Reload
	str	r1, [sp, #92]                   @ 4-byte Spill
	sbcs	r0, r0, r1
	str	r0, [sp, #52]                   @ 4-byte Spill
	ldr	r0, [r4, #32]
	ldr	r1, [sp, #80]                   @ 4-byte Reload
	str	r0, [sp, #88]                   @ 4-byte Spill
	sbcs	r0, r1, r0
	str	r0, [sp, #48]                   @ 4-byte Spill
	ldr	r0, [r4, #36]
	str	r0, [sp, #84]                   @ 4-byte Spill
	sbcs	r0, r11, r0
	str	r0, [sp, #44]                   @ 4-byte Spill
	ldr	r0, [r4, #40]
	ldr	r1, [sp, #76]                   @ 4-byte Reload
	str	r0, [sp, #80]                   @ 4-byte Spill
	sbcs	r0, r1, r0
	str	r0, [sp, #40]                   @ 4-byte Spill
	ldr	r0, [r4, #44]
	ldr	r1, [sp, #72]                   @ 4-byte Reload
	str	r0, [sp, #76]                   @ 4-byte Spill
	sbcs	r0, r1, r0
	str	r0, [sp, #36]                   @ 4-byte Spill
	ldr	r0, [sp, #68]                   @ 4-byte Reload
	ldr	r9, [r4, #48]
	sbc	r0, r0, #0
	ldr	r10, [r4, #52]
	str	r0, [sp, #32]                   @ 4-byte Spill
	subs	r0, r5, r9
	ldr	r11, [r4, #56]
	str	r0, [sp, #28]                   @ 4-byte Spill
	sbcs	r0, r12, r10
	ldr	r8, [r4, #60]
	add	r12, r4, #76
	str	r0, [sp, #24]                   @ 4-byte Spill
	sbcs	r0, r2, r11
	ldr	r5, [r4, #64]
	ldr	r7, [sp, #64]                   @ 4-byte Reload
	str	r0, [sp, #20]                   @ 4-byte Spill
	sbcs	r0, r3, r8
	sbcs	r7, r7, r5
	str	r7, [sp]                        @ 4-byte Spill
	ldr	lr, [r4, #68]
	ldr	r7, [sp, #60]                   @ 4-byte Reload
	ldr	r6, [r4, #72]
	sbcs	r7, r7, lr
	str	r7, [sp, #4]                    @ 4-byte Spill
	ldr	r7, [sp, #56]                   @ 4-byte Reload
	ldm	r12, {r2, r3, r12}
	sbcs	r7, r7, r6
	str	r7, [sp, #8]                    @ 4-byte Spill
	ldr	r7, [sp, #52]                   @ 4-byte Reload
	str	r0, [sp, #16]                   @ 4-byte Spill
	sbcs	r7, r7, r2
	str	r7, [sp, #12]                   @ 4-byte Spill
	ldr	r7, [sp, #48]                   @ 4-byte Reload
	ldr	r0, [r4, #88]
	sbcs	r7, r7, r3
	str	r7, [sp, #48]                   @ 4-byte Spill
	ldr	r7, [sp, #44]                   @ 4-byte Reload
	str	r0, [sp, #72]                   @ 4-byte Spill
	sbcs	r7, r7, r12
	str	r7, [sp, #64]                   @ 4-byte Spill
	ldr	r7, [sp, #40]                   @ 4-byte Reload
	ldr	r1, [r4, #92]
	sbcs	r0, r7, r0
	str	r0, [sp, #60]                   @ 4-byte Spill
	ldr	r0, [sp, #36]                   @ 4-byte Reload
	str	r1, [sp, #68]                   @ 4-byte Spill
	sbcs	r0, r0, r1
	str	r0, [sp, #56]                   @ 4-byte Spill
	ldr	r0, [sp, #32]                   @ 4-byte Reload
	ldr	r1, [sp, #28]                   @ 4-byte Reload
	sbc	r0, r0, #0
	str	r0, [sp, #52]                   @ 4-byte Spill
	ldr	r0, [sp, #96]                   @ 4-byte Reload
	ldr	r7, [sp, #24]                   @ 4-byte Reload
	adds	r0, r0, r1
	ldr	r1, [sp, #92]                   @ 4-byte Reload
	str	r0, [r4, #24]
	adcs	r1, r1, r7
	ldr	r0, [sp, #88]                   @ 4-byte Reload
	ldr	r7, [sp, #20]                   @ 4-byte Reload
	str	r1, [r4, #28]
	adcs	r0, r0, r7
	ldr	r1, [sp, #84]                   @ 4-byte Reload
	ldr	r7, [sp, #16]                   @ 4-byte Reload
	str	r0, [r4, #32]
	adcs	r1, r1, r7
	ldr	r0, [sp, #80]                   @ 4-byte Reload
	ldr	r7, [sp]                        @ 4-byte Reload
	str	r1, [r4, #36]
	adcs	r0, r0, r7
	ldr	r1, [sp, #76]                   @ 4-byte Reload
	ldr	r7, [sp, #4]                    @ 4-byte Reload
	str	r0, [r4, #40]
	ldr	r0, [sp, #8]                    @ 4-byte Reload
	adcs	r1, r1, r7
	str	r1, [r4, #44]
	adcs	r0, r9, r0
	ldr	r1, [sp, #12]                   @ 4-byte Reload
	str	r0, [r4, #48]
	ldr	r0, [sp, #48]                   @ 4-byte Reload
	adcs	r1, r10, r1
	str	r1, [r4, #52]
	adcs	r9, r11, r0
	ldr	r0, [sp, #64]                   @ 4-byte Reload
	str	r9, [r4, #56]
	adcs	r1, r8, r0
	ldr	r0, [sp, #60]                   @ 4-byte Reload
	adcs	r5, r5, r0
	ldr	r0, [sp, #56]                   @ 4-byte Reload
	adcs	r7, lr, r0
	ldr	r0, [sp, #52]                   @ 4-byte Reload
	adcs	r6, r6, r0
	adcs	r2, r2, #0
	adcs	r3, r3, #0
	adcs	r0, r12, #0
	add	r12, r4, #60
	stm	r12, {r1, r5, r7}
	str	r0, [r4, #84]
	ldr	r0, [sp, #72]                   @ 4-byte Reload
	ldr	r1, [sp, #68]                   @ 4-byte Reload
	adcs	r0, r0, #0
	str	r6, [r4, #72]
	adc	r1, r1, #0
	str	r2, [r4, #76]
	str	r3, [r4, #80]
	str	r0, [r4, #88]
	str	r1, [r4, #92]
	add	sp, sp, #196
	pop	{r4, r5, r6, r7, r8, r9, r10, r11, lr}
	mov	pc, lr
.Lfunc_end58:
	.size	mcl_fpDbl_mulPre12L, .Lfunc_end58-mcl_fpDbl_mulPre12L
	.cantunwind
	.fnend
                                        @ -- End function
	.globl	mcl_fpDbl_sqrPre12L             @ -- Begin function mcl_fpDbl_sqrPre12L
	.p2align	2
	.type	mcl_fpDbl_sqrPre12L,%function
	.code	32                              @ @mcl_fpDbl_sqrPre12L
mcl_fpDbl_sqrPre12L:
	.fnstart
@ %bb.0:
	.save	{r4, r5, r6, r7, r8, r9, r10, r11, lr}
	push	{r4, r5, r6, r7, r8, r9, r10, r11, lr}
	.pad	#196
	sub	sp, sp, #196
	mov	r2, r1
	mov	r5, r1
	mov	r4, r0
	bl	mcl_fpDbl_mulPre6L
	add	r1, r5, #24
	add	r0, r4, #48
	mov	r2, r1
	bl	mcl_fpDbl_mulPre6L
	ldm	r5, {r0, r1, r10}
	add	r3, r5, #32
	add	lr, r5, #12
	ldr	r6, [r5, #24]
	ldr	r7, [r5, #28]
	adds	r9, r6, r0
	ldm	lr, {r8, r12, lr}
	adcs	r11, r7, r1
	ldm	r3, {r0, r1, r3}
	adcs	r10, r0, r10
	ldr	r2, [r5, #44]
	adcs	r5, r1, r8
	mov	r0, #0
	adcs	r7, r3, r12
	add	r1, sp, #124
	adcs	r6, r2, lr
	add	r2, sp, #100
	adc	r8, r0, #0
	add	r0, sp, #148
	str	r9, [sp, #124]
	str	r9, [sp, #100]
	str	r11, [sp, #128]
	str	r11, [sp, #104]
	str	r10, [sp, #132]
	str	r10, [sp, #108]
	str	r5, [sp, #136]
	str	r5, [sp, #112]
	str	r7, [sp, #140]
	str	r7, [sp, #116]
	str	r6, [sp, #144]
	str	r6, [sp, #120]
	bl	mcl_fpDbl_mulPre6L
	rsb	r0, r8, #0
	ldr	lr, [sp, #152]
	and	r1, r7, r0
	and	r7, r5, r0
	and	r2, r6, r0
	lsr	r6, r7, #31
	lsl	r7, r7, #1
	lsl	r3, r2, #1
	orr	r12, r3, r1, lsr #31
	orr	r1, r6, r1, lsl #1
	and	r6, r10, r0
	and	r0, r11, r0
	ldr	r3, [sp, #172]
	orr	r7, r7, r6, lsr #31
	add	r10, r4, #12
	lsr	r5, r0, #31
	orr	r6, r5, r6, lsl #1
	sub	r5, r8, r8, lsl #1
	lsl	r0, r0, #1
	and	r5, r9, r5
	adds	r3, r3, r5, lsl #1
	orr	r0, r0, r5, lsr #31
	ldr	r5, [sp, #176]
	str	r3, [sp, #88]                   @ 4-byte Spill
	adcs	r0, r5, r0
	ldr	r5, [sp, #180]
	str	r0, [sp, #84]                   @ 4-byte Spill
	adcs	r11, r5, r6
	ldr	r5, [sp, #184]
	ldr	r3, [sp, #160]
	adcs	r0, r5, r7
	ldr	r5, [sp, #188]
	str	r0, [sp, #80]                   @ 4-byte Spill
	adcs	r0, r5, r1
	ldr	r5, [sp, #192]
	str	r0, [sp, #76]                   @ 4-byte Spill
	adcs	r0, r5, r12
	str	r0, [sp, #72]                   @ 4-byte Spill
	adc	r0, r8, r2, lsr #31
	ldm	r4, {r6, r7, r8}
	ldr	r12, [sp, #148]
	ldr	r2, [sp, #156]
	subs	r12, r12, r6
	ldm	r10, {r5, r9, r10}
	sbcs	lr, lr, r7
	sbcs	r2, r2, r8
	ldr	r6, [sp, #164]
	sbcs	r3, r3, r5
	ldr	r7, [sp, #168]
	sbcs	r5, r6, r9
	str	r5, [sp, #64]                   @ 4-byte Spill
	sbcs	r5, r7, r10
	str	r0, [sp, #68]                   @ 4-byte Spill
	ldr	r0, [r4, #24]
	str	r5, [sp, #60]                   @ 4-byte Spill
	ldr	r5, [sp, #88]                   @ 4-byte Reload
	str	r0, [sp, #96]                   @ 4-byte Spill
	sbcs	r0, r5, r0
	ldr	r1, [r4, #28]
	str	r0, [sp, #56]                   @ 4-byte Spill
	ldr	r0, [sp, #84]                   @ 4-byte Reload
	str	r1, [sp, #92]                   @ 4-byte Spill
	sbcs	r0, r0, r1
	str	r0, [sp, #52]                   @ 4-byte Spill
	ldr	r0, [r4, #32]
	str	r0, [sp, #88]                   @ 4-byte Spill
	sbcs	r0, r11, r0
	str	r0, [sp, #48]                   @ 4-byte Spill
	ldr	r0, [r4, #36]
	ldr	r1, [sp, #80]                   @ 4-byte Reload
	str	r0, [sp, #84]                   @ 4-byte Spill
	sbcs	r0, r1, r0
	str	r0, [sp, #44]                   @ 4-byte Spill
	ldr	r0, [r4, #40]
	ldr	r1, [sp, #76]                   @ 4-byte Reload
	str	r0, [sp, #80]                   @ 4-byte Spill
	sbcs	r0, r1, r0
	str	r0, [sp, #40]                   @ 4-byte Spill
	ldr	r0, [r4, #44]
	ldr	r1, [sp, #72]                   @ 4-byte Reload
	str	r0, [sp, #76]                   @ 4-byte Spill
	sbcs	r0, r1, r0
	str	r0, [sp, #36]                   @ 4-byte Spill
	ldr	r0, [sp, #68]                   @ 4-byte Reload
	ldr	r9, [r4, #48]
	sbc	r0, r0, #0
	ldr	r10, [r4, #52]
	str	r0, [sp, #32]                   @ 4-byte Spill
	subs	r0, r12, r9
	ldr	r11, [r4, #56]
	add	r12, r4, #76
	str	r0, [sp, #28]                   @ 4-byte Spill
	sbcs	r0, lr, r10
	ldr	r8, [r4, #60]
	str	r0, [sp, #24]                   @ 4-byte Spill
	sbcs	r0, r2, r11
	ldr	r5, [r4, #64]
	ldr	r7, [sp, #64]                   @ 4-byte Reload
	str	r0, [sp, #20]                   @ 4-byte Spill
	sbcs	r0, r3, r8
	sbcs	r7, r7, r5
	str	r7, [sp]                        @ 4-byte Spill
	ldr	lr, [r4, #68]
	ldr	r7, [sp, #60]                   @ 4-byte Reload
	ldr	r6, [r4, #72]
	sbcs	r7, r7, lr
	str	r7, [sp, #4]                    @ 4-byte Spill
	ldr	r7, [sp, #56]                   @ 4-byte Reload
	ldm	r12, {r2, r3, r12}
	sbcs	r7, r7, r6
	str	r7, [sp, #8]                    @ 4-byte Spill
	ldr	r7, [sp, #52]                   @ 4-byte Reload
	str	r0, [sp, #16]                   @ 4-byte Spill
	sbcs	r7, r7, r2
	str	r7, [sp, #12]                   @ 4-byte Spill
	ldr	r7, [sp, #48]                   @ 4-byte Reload
	ldr	r0, [r4, #88]
	sbcs	r7, r7, r3
	str	r7, [sp, #48]                   @ 4-byte Spill
	ldr	r7, [sp, #44]                   @ 4-byte Reload
	str	r0, [sp, #72]                   @ 4-byte Spill
	sbcs	r7, r7, r12
	str	r7, [sp, #64]                   @ 4-byte Spill
	ldr	r7, [sp, #40]                   @ 4-byte Reload
	ldr	r1, [r4, #92]
	sbcs	r0, r7, r0
	str	r0, [sp, #60]                   @ 4-byte Spill
	ldr	r0, [sp, #36]                   @ 4-byte Reload
	str	r1, [sp, #68]                   @ 4-byte Spill
	sbcs	r0, r0, r1
	str	r0, [sp, #56]                   @ 4-byte Spill
	ldr	r0, [sp, #32]                   @ 4-byte Reload
	ldr	r1, [sp, #28]                   @ 4-byte Reload
	sbc	r0, r0, #0
	str	r0, [sp, #52]                   @ 4-byte Spill
	ldr	r0, [sp, #96]                   @ 4-byte Reload
	ldr	r7, [sp, #24]                   @ 4-byte Reload
	adds	r0, r0, r1
	ldr	r1, [sp, #92]                   @ 4-byte Reload
	str	r0, [r4, #24]
	adcs	r1, r1, r7
	ldr	r0, [sp, #88]                   @ 4-byte Reload
	ldr	r7, [sp, #20]                   @ 4-byte Reload
	str	r1, [r4, #28]
	adcs	r0, r0, r7
	ldr	r1, [sp, #84]                   @ 4-byte Reload
	ldr	r7, [sp, #16]                   @ 4-byte Reload
	str	r0, [r4, #32]
	adcs	r1, r1, r7
	ldr	r0, [sp, #80]                   @ 4-byte Reload
	ldr	r7, [sp]                        @ 4-byte Reload
	str	r1, [r4, #36]
	adcs	r0, r0, r7
	ldr	r1, [sp, #76]                   @ 4-byte Reload
	ldr	r7, [sp, #4]                    @ 4-byte Reload
	str	r0, [r4, #40]
	ldr	r0, [sp, #8]                    @ 4-byte Reload
	adcs	r1, r1, r7
	str	r1, [r4, #44]
	adcs	r0, r9, r0
	ldr	r1, [sp, #12]                   @ 4-byte Reload
	str	r0, [r4, #48]
	ldr	r0, [sp, #48]                   @ 4-byte Reload
	adcs	r1, r10, r1
	str	r1, [r4, #52]
	adcs	r9, r11, r0
	ldr	r0, [sp, #64]                   @ 4-byte Reload
	str	r9, [r4, #56]
	adcs	r1, r8, r0
	ldr	r0, [sp, #60]                   @ 4-byte Reload
	adcs	r5, r5, r0
	ldr	r0, [sp, #56]                   @ 4-byte Reload
	adcs	r7, lr, r0
	ldr	r0, [sp, #52]                   @ 4-byte Reload
	adcs	r6, r6, r0
	adcs	r2, r2, #0
	adcs	r3, r3, #0
	adcs	r0, r12, #0
	add	r12, r4, #60
	stm	r12, {r1, r5, r7}
	str	r0, [r4, #84]
	ldr	r0, [sp, #72]                   @ 4-byte Reload
	ldr	r1, [sp, #68]                   @ 4-byte Reload
	adcs	r0, r0, #0
	str	r6, [r4, #72]
	adc	r1, r1, #0
	str	r2, [r4, #76]
	str	r3, [r4, #80]
	str	r0, [r4, #88]
	str	r1, [r4, #92]
	add	sp, sp, #196
	pop	{r4, r5, r6, r7, r8, r9, r10, r11, lr}
	mov	pc, lr
.Lfunc_end59:
	.size	mcl_fpDbl_sqrPre12L, .Lfunc_end59-mcl_fpDbl_sqrPre12L
	.cantunwind
	.fnend
                                        @ -- End function
	.globl	mcl_fp_mont12L                  @ -- Begin function mcl_fp_mont12L
	.p2align	2
	.type	mcl_fp_mont12L,%function
	.code	32                              @ @mcl_fp_mont12L
mcl_fp_mont12L:
	.fnstart
@ %bb.0:
	.save	{r4, r5, r6, r7, r8, r9, r10, r11, lr}
	push	{r4, r5, r6, r7, r8, r9, r10, r11, lr}
	.pad	#428
	sub	sp, sp, #428
	.pad	#1024
	sub	sp, sp, #1024
	mov	r7, r2
	ldr	r2, [r2]
	str	r0, [sp, #76]                   @ 4-byte Spill
	add	r0, sp, #1392
	ldr	r5, [r3, #-4]
	mov	r4, r3
	str	r1, [sp, #96]                   @ 4-byte Spill
	mov	r6, r7
	str	r5, [sp, #88]                   @ 4-byte Spill
	str	r3, [sp, #100]                  @ 4-byte Spill
	str	r7, [sp, #92]                   @ 4-byte Spill
	bl	mulPv384x32
	ldr	r0, [sp, #1396]
	add	lr, sp, #1024
	ldr	r7, [sp, #1392]
	mov	r1, r4
	str	r0, [sp, #64]                   @ 4-byte Spill
	ldr	r0, [sp, #1400]
	str	r0, [sp, #60]                   @ 4-byte Spill
	mul	r2, r5, r7
	ldr	r0, [sp, #1404]
	str	r0, [sp, #56]                   @ 4-byte Spill
	ldr	r0, [sp, #1440]
	str	r0, [sp, #84]                   @ 4-byte Spill
	ldr	r0, [sp, #1436]
	str	r0, [sp, #80]                   @ 4-byte Spill
	ldr	r0, [sp, #1432]
	str	r0, [sp, #72]                   @ 4-byte Spill
	ldr	r0, [sp, #1428]
	str	r0, [sp, #68]                   @ 4-byte Spill
	ldr	r0, [sp, #1424]
	str	r0, [sp, #52]                   @ 4-byte Spill
	ldr	r0, [sp, #1420]
	str	r0, [sp, #48]                   @ 4-byte Spill
	ldr	r0, [sp, #1416]
	str	r0, [sp, #44]                   @ 4-byte Spill
	ldr	r0, [sp, #1412]
	str	r0, [sp, #40]                   @ 4-byte Spill
	ldr	r0, [sp, #1408]
	str	r0, [sp, #36]                   @ 4-byte Spill
	add	r0, lr, #312
	bl	mulPv384x32
	ldr	r0, [sp, #1384]
	str	r0, [sp, #32]                   @ 4-byte Spill
	ldr	r0, [sp, #1380]
	str	r0, [sp, #28]                   @ 4-byte Spill
	ldr	r0, [sp, #1376]
	str	r0, [sp, #24]                   @ 4-byte Spill
	ldr	r0, [sp, #1372]
	str	r0, [sp, #20]                   @ 4-byte Spill
	ldr	r0, [sp, #1368]
	str	r0, [sp, #16]                   @ 4-byte Spill
	ldr	r0, [sp, #1364]
	ldr	r2, [r6, #4]
	ldr	r1, [sp, #96]                   @ 4-byte Reload
	str	r0, [sp, #12]                   @ 4-byte Spill
	add	r0, sp, #1280
	ldr	r4, [sp, #1360]
	ldr	r6, [sp, #1356]
	ldr	r8, [sp, #1352]
	ldr	r11, [sp, #1336]
	ldr	r5, [sp, #1340]
	ldr	r9, [sp, #1344]
	ldr	r10, [sp, #1348]
	bl	mulPv384x32
	adds	r0, r11, r7
	ldr	r1, [sp, #36]                   @ 4-byte Reload
	ldr	r0, [sp, #64]                   @ 4-byte Reload
	ldr	r2, [sp, #12]                   @ 4-byte Reload
	adcs	r0, r5, r0
	str	r0, [sp, #64]                   @ 4-byte Spill
	ldr	r0, [sp, #60]                   @ 4-byte Reload
	ldr	r11, [sp, #64]                  @ 4-byte Reload
	adcs	r0, r9, r0
	str	r0, [sp, #60]                   @ 4-byte Spill
	ldr	r0, [sp, #56]                   @ 4-byte Reload
	adcs	r0, r10, r0
	str	r0, [sp, #56]                   @ 4-byte Spill
	adcs	r1, r8, r1
	str	r1, [sp, #36]                   @ 4-byte Spill
	ldr	r1, [sp, #40]                   @ 4-byte Reload
	mov	r0, #0
	adcs	r1, r6, r1
	str	r1, [sp, #40]                   @ 4-byte Spill
	ldr	r1, [sp, #44]                   @ 4-byte Reload
	adcs	r1, r4, r1
	str	r1, [sp, #44]                   @ 4-byte Spill
	ldr	r1, [sp, #48]                   @ 4-byte Reload
	adcs	r1, r2, r1
	str	r1, [sp, #48]                   @ 4-byte Spill
	ldr	r1, [sp, #52]                   @ 4-byte Reload
	ldr	r2, [sp, #16]                   @ 4-byte Reload
	adcs	r1, r2, r1
	str	r1, [sp, #52]                   @ 4-byte Spill
	ldr	r1, [sp, #68]                   @ 4-byte Reload
	ldr	r2, [sp, #20]                   @ 4-byte Reload
	adcs	r1, r2, r1
	str	r1, [sp, #20]                   @ 4-byte Spill
	ldr	r1, [sp, #72]                   @ 4-byte Reload
	ldr	r2, [sp, #24]                   @ 4-byte Reload
	adcs	r1, r2, r1
	str	r1, [sp, #24]                   @ 4-byte Spill
	ldr	r1, [sp, #80]                   @ 4-byte Reload
	ldr	r2, [sp, #28]                   @ 4-byte Reload
	adcs	r1, r2, r1
	str	r1, [sp, #28]                   @ 4-byte Spill
	ldr	r1, [sp, #84]                   @ 4-byte Reload
	ldr	r2, [sp, #32]                   @ 4-byte Reload
	adcs	r1, r2, r1
	str	r1, [sp, #32]                   @ 4-byte Spill
	adc	r0, r0, #0
	str	r0, [sp, #16]                   @ 4-byte Spill
	ldr	r4, [sp, #1280]
	ldr	r0, [sp, #1284]
	adds	r11, r11, r4
	ldr	r4, [sp, #60]                   @ 4-byte Reload
	ldr	r10, [sp, #1328]
	adcs	r0, r4, r0
	ldr	r9, [sp, #1324]
	ldr	r8, [sp, #1320]
	mov	r4, r11
	ldr	r7, [sp, #1316]
	ldr	r6, [sp, #1312]
	ldr	r5, [sp, #1308]
	ldr	lr, [sp, #1304]
	ldr	r12, [sp, #1300]
	ldr	r3, [sp, #1296]
	ldr	r1, [sp, #1288]
	ldr	r2, [sp, #1292]
	str	r0, [sp, #84]                   @ 4-byte Spill
	ldr	r0, [sp, #56]                   @ 4-byte Reload
	adcs	r0, r0, r1
	str	r0, [sp, #80]                   @ 4-byte Spill
	ldr	r0, [sp, #36]                   @ 4-byte Reload
	ldr	r1, [sp, #100]                  @ 4-byte Reload
	adcs	r0, r0, r2
	str	r0, [sp, #72]                   @ 4-byte Spill
	ldr	r0, [sp, #40]                   @ 4-byte Reload
	adcs	r0, r0, r3
	str	r0, [sp, #68]                   @ 4-byte Spill
	ldr	r0, [sp, #44]                   @ 4-byte Reload
	adcs	r0, r0, r12
	str	r0, [sp, #64]                   @ 4-byte Spill
	ldr	r0, [sp, #48]                   @ 4-byte Reload
	adcs	r0, r0, lr
	str	r0, [sp, #60]                   @ 4-byte Spill
	ldr	r0, [sp, #52]                   @ 4-byte Reload
	add	lr, sp, #1024
	adcs	r0, r0, r5
	str	r0, [sp, #56]                   @ 4-byte Spill
	ldr	r0, [sp, #20]                   @ 4-byte Reload
	adcs	r0, r0, r6
	str	r0, [sp, #52]                   @ 4-byte Spill
	ldr	r0, [sp, #24]                   @ 4-byte Reload
	adcs	r0, r0, r7
	str	r0, [sp, #48]                   @ 4-byte Spill
	ldr	r0, [sp, #28]                   @ 4-byte Reload
	adcs	r0, r0, r8
	str	r0, [sp, #44]                   @ 4-byte Spill
	ldr	r0, [sp, #32]                   @ 4-byte Reload
	adcs	r0, r0, r9
	str	r0, [sp, #40]                   @ 4-byte Spill
	ldr	r0, [sp, #16]                   @ 4-byte Reload
	adcs	r0, r0, r10
	str	r0, [sp, #36]                   @ 4-byte Spill
	mov	r0, #0
	adc	r0, r0, #0
	str	r0, [sp, #32]                   @ 4-byte Spill
	ldr	r0, [sp, #88]                   @ 4-byte Reload
	mul	r2, r0, r11
	add	r0, lr, #200
	bl	mulPv384x32
	ldr	r0, [sp, #92]                   @ 4-byte Reload
	add	r9, sp, #1232
	ldr	r1, [sp, #96]                   @ 4-byte Reload
	ldr	r2, [r0, #8]
	ldr	r0, [sp, #1272]
	str	r0, [sp, #28]                   @ 4-byte Spill
	ldr	r0, [sp, #1268]
	str	r0, [sp, #24]                   @ 4-byte Spill
	ldr	r0, [sp, #1264]
	str	r0, [sp, #20]                   @ 4-byte Spill
	ldr	r0, [sp, #1260]
	str	r0, [sp, #16]                   @ 4-byte Spill
	ldr	r0, [sp, #1256]
	str	r0, [sp, #12]                   @ 4-byte Spill
	ldr	r0, [sp, #1252]
	str	r0, [sp, #8]                    @ 4-byte Spill
	add	r0, sp, #1168
	ldm	r9, {r6, r7, r9}
	ldr	r5, [sp, #1248]
	ldr	r8, [sp, #1244]
	ldr	r10, [sp, #1224]
	ldr	r11, [sp, #1228]
	bl	mulPv384x32
	adds	r0, r4, r10
	ldr	r1, [sp, #8]                    @ 4-byte Reload
	ldr	r0, [sp, #84]                   @ 4-byte Reload
	add	lr, sp, #1168
	adcs	r0, r0, r11
	str	r0, [sp, #84]                   @ 4-byte Spill
	ldr	r0, [sp, #80]                   @ 4-byte Reload
	adcs	r0, r0, r6
	str	r0, [sp, #80]                   @ 4-byte Spill
	ldr	r0, [sp, #72]                   @ 4-byte Reload
	adcs	r0, r0, r7
	str	r0, [sp, #72]                   @ 4-byte Spill
	ldr	r0, [sp, #68]                   @ 4-byte Reload
	ldr	r7, [sp, #84]                   @ 4-byte Reload
	adcs	r0, r0, r9
	str	r0, [sp, #68]                   @ 4-byte Spill
	ldr	r0, [sp, #64]                   @ 4-byte Reload
	adcs	r0, r0, r8
	str	r0, [sp, #64]                   @ 4-byte Spill
	ldr	r0, [sp, #60]                   @ 4-byte Reload
	adcs	r0, r0, r5
	str	r0, [sp, #60]                   @ 4-byte Spill
	ldr	r0, [sp, #56]                   @ 4-byte Reload
	adcs	r0, r0, r1
	str	r0, [sp, #56]                   @ 4-byte Spill
	ldr	r0, [sp, #52]                   @ 4-byte Reload
	ldr	r1, [sp, #12]                   @ 4-byte Reload
	adcs	r0, r0, r1
	str	r0, [sp, #52]                   @ 4-byte Spill
	ldr	r0, [sp, #48]                   @ 4-byte Reload
	ldr	r1, [sp, #16]                   @ 4-byte Reload
	adcs	r0, r0, r1
	str	r0, [sp, #48]                   @ 4-byte Spill
	ldr	r0, [sp, #44]                   @ 4-byte Reload
	ldr	r1, [sp, #20]                   @ 4-byte Reload
	adcs	r0, r0, r1
	str	r0, [sp, #44]                   @ 4-byte Spill
	ldr	r0, [sp, #40]                   @ 4-byte Reload
	ldr	r1, [sp, #24]                   @ 4-byte Reload
	adcs	r0, r0, r1
	str	r0, [sp, #40]                   @ 4-byte Spill
	ldr	r0, [sp, #36]                   @ 4-byte Reload
	ldr	r1, [sp, #28]                   @ 4-byte Reload
	adcs	r0, r0, r1
	str	r0, [sp, #36]                   @ 4-byte Spill
	ldr	r0, [sp, #32]                   @ 4-byte Reload
	adc	r0, r0, #0
	str	r0, [sp, #32]                   @ 4-byte Spill
	ldm	lr, {r0, r1, r2, r3, r12, lr}
	adds	r7, r7, r0
	ldr	r0, [sp, #80]                   @ 4-byte Reload
	ldr	r11, [sp, #1216]
	adcs	r0, r0, r1
	ldr	r10, [sp, #1212]
	ldr	r9, [sp, #1208]
	ldr	r8, [sp, #1204]
	ldr	r6, [sp, #1200]
	ldr	r5, [sp, #1196]
	ldr	r4, [sp, #1192]
	str	r0, [sp, #84]                   @ 4-byte Spill
	ldr	r0, [sp, #72]                   @ 4-byte Reload
	ldr	r1, [sp, #100]                  @ 4-byte Reload
	adcs	r0, r0, r2
	str	r0, [sp, #80]                   @ 4-byte Spill
	ldr	r0, [sp, #68]                   @ 4-byte Reload
	adcs	r0, r0, r3
	str	r0, [sp, #72]                   @ 4-byte Spill
	ldr	r0, [sp, #64]                   @ 4-byte Reload
	adcs	r0, r0, r12
	str	r0, [sp, #68]                   @ 4-byte Spill
	ldr	r0, [sp, #60]                   @ 4-byte Reload
	adcs	r0, r0, lr
	str	r0, [sp, #64]                   @ 4-byte Spill
	ldr	r0, [sp, #56]                   @ 4-byte Reload
	add	lr, sp, #1024
	adcs	r0, r0, r4
	str	r0, [sp, #60]                   @ 4-byte Spill
	ldr	r0, [sp, #52]                   @ 4-byte Reload
	adcs	r0, r0, r5
	str	r0, [sp, #56]                   @ 4-byte Spill
	ldr	r0, [sp, #48]                   @ 4-byte Reload
	adcs	r0, r0, r6
	str	r0, [sp, #52]                   @ 4-byte Spill
	ldr	r0, [sp, #44]                   @ 4-byte Reload
	adcs	r0, r0, r8
	str	r0, [sp, #48]                   @ 4-byte Spill
	ldr	r0, [sp, #40]                   @ 4-byte Reload
	mov	r8, r7
	adcs	r0, r0, r9
	str	r0, [sp, #44]                   @ 4-byte Spill
	ldr	r0, [sp, #36]                   @ 4-byte Reload
	adcs	r0, r0, r10
	str	r0, [sp, #40]                   @ 4-byte Spill
	ldr	r0, [sp, #32]                   @ 4-byte Reload
	adcs	r0, r0, r11
	str	r0, [sp, #36]                   @ 4-byte Spill
	mov	r0, #0
	adc	r0, r0, #0
	str	r0, [sp, #32]                   @ 4-byte Spill
	ldr	r0, [sp, #88]                   @ 4-byte Reload
	mul	r2, r0, r7
	add	r0, lr, #88
	bl	mulPv384x32
	ldr	r0, [sp, #92]                   @ 4-byte Reload
	add	r10, sp, #1120
	ldr	r1, [sp, #96]                   @ 4-byte Reload
	ldr	r2, [r0, #12]
	ldr	r0, [sp, #1160]
	str	r0, [sp, #28]                   @ 4-byte Spill
	ldr	r0, [sp, #1156]
	str	r0, [sp, #24]                   @ 4-byte Spill
	ldr	r0, [sp, #1152]
	str	r0, [sp, #20]                   @ 4-byte Spill
	ldr	r0, [sp, #1148]
	str	r0, [sp, #16]                   @ 4-byte Spill
	ldr	r0, [sp, #1144]
	str	r0, [sp, #12]                   @ 4-byte Spill
	ldr	r0, [sp, #1140]
	str	r0, [sp, #8]                    @ 4-byte Spill
	add	r0, sp, #1056
	ldm	r10, {r4, r5, r10}
	ldr	r6, [sp, #1136]
	ldr	r9, [sp, #1132]
	ldr	r11, [sp, #1112]
	ldr	r7, [sp, #1116]
	bl	mulPv384x32
	adds	r0, r8, r11
	ldr	r1, [sp, #8]                    @ 4-byte Reload
	ldr	r0, [sp, #84]                   @ 4-byte Reload
	adcs	r0, r0, r7
	str	r0, [sp, #84]                   @ 4-byte Spill
	ldr	r0, [sp, #80]                   @ 4-byte Reload
	ldr	r11, [sp, #84]                  @ 4-byte Reload
	adcs	r0, r0, r4
	str	r0, [sp, #80]                   @ 4-byte Spill
	ldr	r0, [sp, #72]                   @ 4-byte Reload
	adcs	r0, r0, r5
	str	r0, [sp, #72]                   @ 4-byte Spill
	ldr	r0, [sp, #68]                   @ 4-byte Reload
	adcs	r0, r0, r10
	str	r0, [sp, #68]                   @ 4-byte Spill
	ldr	r0, [sp, #64]                   @ 4-byte Reload
	adcs	r0, r0, r9
	str	r0, [sp, #64]                   @ 4-byte Spill
	ldr	r0, [sp, #60]                   @ 4-byte Reload
	adcs	r0, r0, r6
	str	r0, [sp, #60]                   @ 4-byte Spill
	ldr	r0, [sp, #56]                   @ 4-byte Reload
	adcs	r0, r0, r1
	str	r0, [sp, #56]                   @ 4-byte Spill
	ldr	r0, [sp, #52]                   @ 4-byte Reload
	ldr	r1, [sp, #12]                   @ 4-byte Reload
	adcs	r0, r0, r1
	str	r0, [sp, #52]                   @ 4-byte Spill
	ldr	r0, [sp, #48]                   @ 4-byte Reload
	ldr	r1, [sp, #16]                   @ 4-byte Reload
	adcs	r0, r0, r1
	str	r0, [sp, #48]                   @ 4-byte Spill
	ldr	r0, [sp, #44]                   @ 4-byte Reload
	ldr	r1, [sp, #20]                   @ 4-byte Reload
	adcs	r0, r0, r1
	str	r0, [sp, #44]                   @ 4-byte Spill
	ldr	r0, [sp, #40]                   @ 4-byte Reload
	ldr	r1, [sp, #24]                   @ 4-byte Reload
	adcs	r0, r0, r1
	str	r0, [sp, #40]                   @ 4-byte Spill
	ldr	r0, [sp, #36]                   @ 4-byte Reload
	ldr	r1, [sp, #28]                   @ 4-byte Reload
	adcs	r0, r0, r1
	str	r0, [sp, #36]                   @ 4-byte Spill
	ldr	r0, [sp, #32]                   @ 4-byte Reload
	adc	r0, r0, #0
	str	r0, [sp, #32]                   @ 4-byte Spill
	ldr	r4, [sp, #1056]
	ldr	r0, [sp, #1060]
	adds	r11, r11, r4
	ldr	r4, [sp, #80]                   @ 4-byte Reload
	ldr	r10, [sp, #1104]
	adcs	r0, r4, r0
	ldr	r9, [sp, #1100]
	ldr	r8, [sp, #1096]
	mov	r4, r11
	ldr	r7, [sp, #1092]
	ldr	r6, [sp, #1088]
	ldr	r5, [sp, #1084]
	ldr	lr, [sp, #1080]
	ldr	r12, [sp, #1076]
	ldr	r3, [sp, #1072]
	ldr	r1, [sp, #1064]
	ldr	r2, [sp, #1068]
	str	r0, [sp, #84]                   @ 4-byte Spill
	ldr	r0, [sp, #72]                   @ 4-byte Reload
	adcs	r0, r0, r1
	str	r0, [sp, #80]                   @ 4-byte Spill
	ldr	r0, [sp, #68]                   @ 4-byte Reload
	ldr	r1, [sp, #100]                  @ 4-byte Reload
	adcs	r0, r0, r2
	str	r0, [sp, #72]                   @ 4-byte Spill
	ldr	r0, [sp, #64]                   @ 4-byte Reload
	adcs	r0, r0, r3
	str	r0, [sp, #68]                   @ 4-byte Spill
	ldr	r0, [sp, #60]                   @ 4-byte Reload
	adcs	r0, r0, r12
	str	r0, [sp, #64]                   @ 4-byte Spill
	ldr	r0, [sp, #56]                   @ 4-byte Reload
	adcs	r0, r0, lr
	str	r0, [sp, #60]                   @ 4-byte Spill
	ldr	r0, [sp, #52]                   @ 4-byte Reload
	adcs	r0, r0, r5
	str	r0, [sp, #56]                   @ 4-byte Spill
	ldr	r0, [sp, #48]                   @ 4-byte Reload
	adcs	r0, r0, r6
	str	r0, [sp, #52]                   @ 4-byte Spill
	ldr	r0, [sp, #44]                   @ 4-byte Reload
	adcs	r0, r0, r7
	str	r0, [sp, #48]                   @ 4-byte Spill
	ldr	r0, [sp, #40]                   @ 4-byte Reload
	adcs	r0, r0, r8
	str	r0, [sp, #44]                   @ 4-byte Spill
	ldr	r0, [sp, #36]                   @ 4-byte Reload
	adcs	r0, r0, r9
	str	r0, [sp, #40]                   @ 4-byte Spill
	ldr	r0, [sp, #32]                   @ 4-byte Reload
	adcs	r0, r0, r10
	str	r0, [sp, #36]                   @ 4-byte Spill
	mov	r0, #0
	adc	r0, r0, #0
	str	r0, [sp, #32]                   @ 4-byte Spill
	ldr	r0, [sp, #88]                   @ 4-byte Reload
	mul	r2, r0, r11
	add	r0, sp, #1000
	bl	mulPv384x32
	ldr	r0, [sp, #92]                   @ 4-byte Reload
	add	r9, sp, #1008
	ldr	r1, [sp, #96]                   @ 4-byte Reload
	ldr	r2, [r0, #16]
	ldr	r0, [sp, #1048]
	str	r0, [sp, #28]                   @ 4-byte Spill
	ldr	r0, [sp, #1044]
	str	r0, [sp, #24]                   @ 4-byte Spill
	ldr	r0, [sp, #1040]
	str	r0, [sp, #20]                   @ 4-byte Spill
	ldr	r0, [sp, #1036]
	str	r0, [sp, #16]                   @ 4-byte Spill
	ldr	r0, [sp, #1032]
	str	r0, [sp, #12]                   @ 4-byte Spill
	ldr	r0, [sp, #1028]
	str	r0, [sp, #8]                    @ 4-byte Spill
	add	r0, sp, #944
	ldm	r9, {r6, r7, r9}
	ldr	r5, [sp, #1024]
	ldr	r8, [sp, #1020]
	ldr	r10, [sp, #1000]
	ldr	r11, [sp, #1004]
	bl	mulPv384x32
	adds	r0, r4, r10
	ldr	r1, [sp, #8]                    @ 4-byte Reload
	ldr	r0, [sp, #84]                   @ 4-byte Reload
	add	lr, sp, #944
	adcs	r0, r0, r11
	str	r0, [sp, #84]                   @ 4-byte Spill
	ldr	r0, [sp, #80]                   @ 4-byte Reload
	add	r11, sp, #968
	adcs	r0, r0, r6
	str	r0, [sp, #80]                   @ 4-byte Spill
	ldr	r0, [sp, #72]                   @ 4-byte Reload
	adcs	r0, r0, r7
	str	r0, [sp, #72]                   @ 4-byte Spill
	ldr	r0, [sp, #68]                   @ 4-byte Reload
	ldr	r7, [sp, #84]                   @ 4-byte Reload
	adcs	r0, r0, r9
	str	r0, [sp, #68]                   @ 4-byte Spill
	ldr	r0, [sp, #64]                   @ 4-byte Reload
	adcs	r0, r0, r8
	str	r0, [sp, #64]                   @ 4-byte Spill
	ldr	r0, [sp, #60]                   @ 4-byte Reload
	adcs	r0, r0, r5
	str	r0, [sp, #60]                   @ 4-byte Spill
	ldr	r0, [sp, #56]                   @ 4-byte Reload
	adcs	r0, r0, r1
	str	r0, [sp, #56]                   @ 4-byte Spill
	ldr	r0, [sp, #52]                   @ 4-byte Reload
	ldr	r1, [sp, #12]                   @ 4-byte Reload
	adcs	r0, r0, r1
	str	r0, [sp, #52]                   @ 4-byte Spill
	ldr	r0, [sp, #48]                   @ 4-byte Reload
	ldr	r1, [sp, #16]                   @ 4-byte Reload
	adcs	r0, r0, r1
	str	r0, [sp, #48]                   @ 4-byte Spill
	ldr	r0, [sp, #44]                   @ 4-byte Reload
	ldr	r1, [sp, #20]                   @ 4-byte Reload
	adcs	r0, r0, r1
	str	r0, [sp, #44]                   @ 4-byte Spill
	ldr	r0, [sp, #40]                   @ 4-byte Reload
	ldr	r1, [sp, #24]                   @ 4-byte Reload
	adcs	r0, r0, r1
	str	r0, [sp, #40]                   @ 4-byte Spill
	ldr	r0, [sp, #36]                   @ 4-byte Reload
	ldr	r1, [sp, #28]                   @ 4-byte Reload
	adcs	r0, r0, r1
	str	r0, [sp, #36]                   @ 4-byte Spill
	ldr	r0, [sp, #32]                   @ 4-byte Reload
	adc	r0, r0, #0
	str	r0, [sp, #32]                   @ 4-byte Spill
	ldm	lr, {r0, r1, r2, r3, r12, lr}
	adds	r7, r7, r0
	ldr	r0, [sp, #80]                   @ 4-byte Reload
	ldm	r11, {r4, r5, r6, r8, r9, r10, r11}
	adcs	r0, r0, r1
	str	r0, [sp, #84]                   @ 4-byte Spill
	ldr	r0, [sp, #72]                   @ 4-byte Reload
	ldr	r1, [sp, #100]                  @ 4-byte Reload
	adcs	r0, r0, r2
	str	r0, [sp, #80]                   @ 4-byte Spill
	ldr	r0, [sp, #68]                   @ 4-byte Reload
	adcs	r0, r0, r3
	str	r0, [sp, #72]                   @ 4-byte Spill
	ldr	r0, [sp, #64]                   @ 4-byte Reload
	adcs	r0, r0, r12
	str	r0, [sp, #68]                   @ 4-byte Spill
	ldr	r0, [sp, #60]                   @ 4-byte Reload
	adcs	r0, r0, lr
	str	r0, [sp, #64]                   @ 4-byte Spill
	ldr	r0, [sp, #56]                   @ 4-byte Reload
	adcs	r0, r0, r4
	str	r0, [sp, #60]                   @ 4-byte Spill
	ldr	r0, [sp, #52]                   @ 4-byte Reload
	adcs	r0, r0, r5
	str	r0, [sp, #56]                   @ 4-byte Spill
	ldr	r0, [sp, #48]                   @ 4-byte Reload
	adcs	r0, r0, r6
	str	r0, [sp, #52]                   @ 4-byte Spill
	ldr	r0, [sp, #44]                   @ 4-byte Reload
	adcs	r0, r0, r8
	str	r0, [sp, #48]                   @ 4-byte Spill
	ldr	r0, [sp, #40]                   @ 4-byte Reload
	mov	r8, r7
	adcs	r0, r0, r9
	str	r0, [sp, #44]                   @ 4-byte Spill
	ldr	r0, [sp, #36]                   @ 4-byte Reload
	adcs	r0, r0, r10
	str	r0, [sp, #40]                   @ 4-byte Spill
	ldr	r0, [sp, #32]                   @ 4-byte Reload
	adcs	r0, r0, r11
	str	r0, [sp, #36]                   @ 4-byte Spill
	mov	r0, #0
	adc	r0, r0, #0
	str	r0, [sp, #32]                   @ 4-byte Spill
	ldr	r0, [sp, #88]                   @ 4-byte Reload
	mul	r2, r0, r7
	add	r0, sp, #888
	bl	mulPv384x32
	ldr	r0, [sp, #92]                   @ 4-byte Reload
	add	r10, sp, #896
	ldr	r1, [sp, #96]                   @ 4-byte Reload
	ldr	r2, [r0, #20]
	ldr	r0, [sp, #936]
	str	r0, [sp, #28]                   @ 4-byte Spill
	ldr	r0, [sp, #932]
	str	r0, [sp, #24]                   @ 4-byte Spill
	ldr	r0, [sp, #928]
	str	r0, [sp, #20]                   @ 4-byte Spill
	ldr	r0, [sp, #924]
	str	r0, [sp, #16]                   @ 4-byte Spill
	ldr	r0, [sp, #920]
	str	r0, [sp, #12]                   @ 4-byte Spill
	ldr	r0, [sp, #916]
	str	r0, [sp, #8]                    @ 4-byte Spill
	add	r0, sp, #832
	ldm	r10, {r4, r5, r10}
	ldr	r6, [sp, #912]
	ldr	r9, [sp, #908]
	ldr	r11, [sp, #888]
	ldr	r7, [sp, #892]
	bl	mulPv384x32
	adds	r0, r8, r11
	ldr	r1, [sp, #8]                    @ 4-byte Reload
	ldr	r0, [sp, #84]                   @ 4-byte Reload
	add	lr, sp, #836
	adcs	r0, r0, r7
	str	r0, [sp, #84]                   @ 4-byte Spill
	ldr	r0, [sp, #80]                   @ 4-byte Reload
	ldr	r11, [sp, #84]                  @ 4-byte Reload
	adcs	r0, r0, r4
	str	r0, [sp, #80]                   @ 4-byte Spill
	ldr	r0, [sp, #72]                   @ 4-byte Reload
	adcs	r0, r0, r5
	str	r0, [sp, #72]                   @ 4-byte Spill
	ldr	r0, [sp, #68]                   @ 4-byte Reload
	adcs	r0, r0, r10
	str	r0, [sp, #68]                   @ 4-byte Spill
	ldr	r0, [sp, #64]                   @ 4-byte Reload
	add	r10, sp, #860
	adcs	r0, r0, r9
	str	r0, [sp, #64]                   @ 4-byte Spill
	ldr	r0, [sp, #60]                   @ 4-byte Reload
	adcs	r0, r0, r6
	str	r0, [sp, #60]                   @ 4-byte Spill
	ldr	r0, [sp, #56]                   @ 4-byte Reload
	adcs	r0, r0, r1
	str	r0, [sp, #56]                   @ 4-byte Spill
	ldr	r0, [sp, #52]                   @ 4-byte Reload
	ldr	r1, [sp, #12]                   @ 4-byte Reload
	adcs	r0, r0, r1
	str	r0, [sp, #52]                   @ 4-byte Spill
	ldr	r0, [sp, #48]                   @ 4-byte Reload
	ldr	r1, [sp, #16]                   @ 4-byte Reload
	adcs	r0, r0, r1
	str	r0, [sp, #48]                   @ 4-byte Spill
	ldr	r0, [sp, #44]                   @ 4-byte Reload
	ldr	r1, [sp, #20]                   @ 4-byte Reload
	adcs	r0, r0, r1
	str	r0, [sp, #44]                   @ 4-byte Spill
	ldr	r0, [sp, #40]                   @ 4-byte Reload
	ldr	r1, [sp, #24]                   @ 4-byte Reload
	adcs	r0, r0, r1
	str	r0, [sp, #40]                   @ 4-byte Spill
	ldr	r0, [sp, #36]                   @ 4-byte Reload
	ldr	r1, [sp, #28]                   @ 4-byte Reload
	adcs	r0, r0, r1
	str	r0, [sp, #36]                   @ 4-byte Spill
	ldr	r0, [sp, #32]                   @ 4-byte Reload
	adc	r0, r0, #0
	str	r0, [sp, #32]                   @ 4-byte Spill
	ldr	r4, [sp, #832]
	ldm	lr, {r0, r1, r2, r3, r12, lr}
	adds	r11, r11, r4
	ldr	r4, [sp, #80]                   @ 4-byte Reload
	ldm	r10, {r5, r6, r7, r8, r9, r10}
	adcs	r0, r4, r0
	str	r0, [sp, #84]                   @ 4-byte Spill
	mov	r4, r11
	ldr	r0, [sp, #72]                   @ 4-byte Reload
	adcs	r0, r0, r1
	str	r0, [sp, #80]                   @ 4-byte Spill
	ldr	r0, [sp, #68]                   @ 4-byte Reload
	ldr	r1, [sp, #100]                  @ 4-byte Reload
	adcs	r0, r0, r2
	str	r0, [sp, #72]                   @ 4-byte Spill
	ldr	r0, [sp, #64]                   @ 4-byte Reload
	adcs	r0, r0, r3
	str	r0, [sp, #68]                   @ 4-byte Spill
	ldr	r0, [sp, #60]                   @ 4-byte Reload
	adcs	r0, r0, r12
	str	r0, [sp, #64]                   @ 4-byte Spill
	ldr	r0, [sp, #56]                   @ 4-byte Reload
	adcs	r0, r0, lr
	str	r0, [sp, #60]                   @ 4-byte Spill
	ldr	r0, [sp, #52]                   @ 4-byte Reload
	adcs	r0, r0, r5
	str	r0, [sp, #56]                   @ 4-byte Spill
	ldr	r0, [sp, #48]                   @ 4-byte Reload
	adcs	r0, r0, r6
	str	r0, [sp, #52]                   @ 4-byte Spill
	ldr	r0, [sp, #44]                   @ 4-byte Reload
	adcs	r0, r0, r7
	str	r0, [sp, #48]                   @ 4-byte Spill
	ldr	r0, [sp, #40]                   @ 4-byte Reload
	adcs	r0, r0, r8
	str	r0, [sp, #44]                   @ 4-byte Spill
	ldr	r0, [sp, #36]                   @ 4-byte Reload
	adcs	r0, r0, r9
	str	r0, [sp, #40]                   @ 4-byte Spill
	ldr	r0, [sp, #32]                   @ 4-byte Reload
	adcs	r0, r0, r10
	str	r0, [sp, #36]                   @ 4-byte Spill
	mov	r0, #0
	adc	r0, r0, #0
	str	r0, [sp, #32]                   @ 4-byte Spill
	ldr	r0, [sp, #88]                   @ 4-byte Reload
	mul	r2, r0, r11
	add	r0, sp, #776
	bl	mulPv384x32
	ldr	r0, [sp, #92]                   @ 4-byte Reload
	add	r9, sp, #784
	ldr	r1, [sp, #96]                   @ 4-byte Reload
	ldr	r2, [r0, #24]
	ldr	r0, [sp, #824]
	str	r0, [sp, #28]                   @ 4-byte Spill
	ldr	r0, [sp, #820]
	str	r0, [sp, #24]                   @ 4-byte Spill
	ldr	r0, [sp, #816]
	str	r0, [sp, #20]                   @ 4-byte Spill
	ldr	r0, [sp, #812]
	str	r0, [sp, #16]                   @ 4-byte Spill
	ldr	r0, [sp, #808]
	str	r0, [sp, #12]                   @ 4-byte Spill
	ldr	r0, [sp, #804]
	str	r0, [sp, #8]                    @ 4-byte Spill
	add	r0, sp, #720
	ldm	r9, {r6, r7, r9}
	ldr	r5, [sp, #800]
	ldr	r8, [sp, #796]
	ldr	r10, [sp, #776]
	ldr	r11, [sp, #780]
	bl	mulPv384x32
	adds	r0, r4, r10
	ldr	r1, [sp, #8]                    @ 4-byte Reload
	ldr	r0, [sp, #84]                   @ 4-byte Reload
	add	lr, sp, #720
	adcs	r0, r0, r11
	str	r0, [sp, #84]                   @ 4-byte Spill
	ldr	r0, [sp, #80]                   @ 4-byte Reload
	add	r11, sp, #744
	adcs	r0, r0, r6
	str	r0, [sp, #80]                   @ 4-byte Spill
	ldr	r0, [sp, #72]                   @ 4-byte Reload
	adcs	r0, r0, r7
	str	r0, [sp, #72]                   @ 4-byte Spill
	ldr	r0, [sp, #68]                   @ 4-byte Reload
	ldr	r7, [sp, #84]                   @ 4-byte Reload
	adcs	r0, r0, r9
	str	r0, [sp, #68]                   @ 4-byte Spill
	ldr	r0, [sp, #64]                   @ 4-byte Reload
	adcs	r0, r0, r8
	str	r0, [sp, #64]                   @ 4-byte Spill
	ldr	r0, [sp, #60]                   @ 4-byte Reload
	adcs	r0, r0, r5
	str	r0, [sp, #60]                   @ 4-byte Spill
	ldr	r0, [sp, #56]                   @ 4-byte Reload
	adcs	r0, r0, r1
	str	r0, [sp, #56]                   @ 4-byte Spill
	ldr	r0, [sp, #52]                   @ 4-byte Reload
	ldr	r1, [sp, #12]                   @ 4-byte Reload
	adcs	r0, r0, r1
	str	r0, [sp, #52]                   @ 4-byte Spill
	ldr	r0, [sp, #48]                   @ 4-byte Reload
	ldr	r1, [sp, #16]                   @ 4-byte Reload
	adcs	r0, r0, r1
	str	r0, [sp, #48]                   @ 4-byte Spill
	ldr	r0, [sp, #44]                   @ 4-byte Reload
	ldr	r1, [sp, #20]                   @ 4-byte Reload
	adcs	r0, r0, r1
	str	r0, [sp, #44]                   @ 4-byte Spill
	ldr	r0, [sp, #40]                   @ 4-byte Reload
	ldr	r1, [sp, #24]                   @ 4-byte Reload
	adcs	r0, r0, r1
	str	r0, [sp, #40]                   @ 4-byte Spill
	ldr	r0, [sp, #36]                   @ 4-byte Reload
	ldr	r1, [sp, #28]                   @ 4-byte Reload
	adcs	r0, r0, r1
	str	r0, [sp, #36]                   @ 4-byte Spill
	ldr	r0, [sp, #32]                   @ 4-byte Reload
	adc	r0, r0, #0
	str	r0, [sp, #32]                   @ 4-byte Spill
	ldm	lr, {r0, r1, r2, r3, r12, lr}
	adds	r7, r7, r0
	ldr	r0, [sp, #80]                   @ 4-byte Reload
	ldm	r11, {r4, r5, r6, r8, r9, r10, r11}
	adcs	r0, r0, r1
	str	r0, [sp, #84]                   @ 4-byte Spill
	ldr	r0, [sp, #72]                   @ 4-byte Reload
	ldr	r1, [sp, #100]                  @ 4-byte Reload
	adcs	r0, r0, r2
	str	r0, [sp, #80]                   @ 4-byte Spill
	ldr	r0, [sp, #68]                   @ 4-byte Reload
	adcs	r0, r0, r3
	str	r0, [sp, #72]                   @ 4-byte Spill
	ldr	r0, [sp, #64]                   @ 4-byte Reload
	adcs	r0, r0, r12
	str	r0, [sp, #68]                   @ 4-byte Spill
	ldr	r0, [sp, #60]                   @ 4-byte Reload
	adcs	r0, r0, lr
	str	r0, [sp, #64]                   @ 4-byte Spill
	ldr	r0, [sp, #56]                   @ 4-byte Reload
	adcs	r0, r0, r4
	str	r0, [sp, #60]                   @ 4-byte Spill
	ldr	r0, [sp, #52]                   @ 4-byte Reload
	adcs	r0, r0, r5
	str	r0, [sp, #56]                   @ 4-byte Spill
	ldr	r0, [sp, #48]                   @ 4-byte Reload
	adcs	r0, r0, r6
	str	r0, [sp, #52]                   @ 4-byte Spill
	ldr	r0, [sp, #44]                   @ 4-byte Reload
	adcs	r0, r0, r8
	str	r0, [sp, #48]                   @ 4-byte Spill
	ldr	r0, [sp, #40]                   @ 4-byte Reload
	mov	r8, r7
	adcs	r0, r0, r9
	str	r0, [sp, #44]                   @ 4-byte Spill
	ldr	r0, [sp, #36]                   @ 4-byte Reload
	adcs	r0, r0, r10
	str	r0, [sp, #40]                   @ 4-byte Spill
	ldr	r0, [sp, #32]                   @ 4-byte Reload
	adcs	r0, r0, r11
	str	r0, [sp, #36]                   @ 4-byte Spill
	mov	r0, #0
	adc	r0, r0, #0
	str	r0, [sp, #32]                   @ 4-byte Spill
	ldr	r0, [sp, #88]                   @ 4-byte Reload
	mul	r2, r0, r7
	add	r0, sp, #664
	bl	mulPv384x32
	ldr	r0, [sp, #92]                   @ 4-byte Reload
	add	r10, sp, #672
	ldr	r1, [sp, #96]                   @ 4-byte Reload
	ldr	r2, [r0, #28]
	ldr	r0, [sp, #712]
	str	r0, [sp, #28]                   @ 4-byte Spill
	ldr	r0, [sp, #708]
	str	r0, [sp, #24]                   @ 4-byte Spill
	ldr	r0, [sp, #704]
	str	r0, [sp, #20]                   @ 4-byte Spill
	ldr	r0, [sp, #700]
	str	r0, [sp, #16]                   @ 4-byte Spill
	ldr	r0, [sp, #696]
	str	r0, [sp, #12]                   @ 4-byte Spill
	ldr	r0, [sp, #692]
	str	r0, [sp, #8]                    @ 4-byte Spill
	add	r0, sp, #608
	ldm	r10, {r4, r5, r10}
	ldr	r6, [sp, #688]
	ldr	r9, [sp, #684]
	ldr	r11, [sp, #664]
	ldr	r7, [sp, #668]
	bl	mulPv384x32
	adds	r0, r8, r11
	ldr	r1, [sp, #8]                    @ 4-byte Reload
	ldr	r0, [sp, #84]                   @ 4-byte Reload
	add	lr, sp, #612
	adcs	r0, r0, r7
	str	r0, [sp, #84]                   @ 4-byte Spill
	ldr	r0, [sp, #80]                   @ 4-byte Reload
	ldr	r11, [sp, #84]                  @ 4-byte Reload
	adcs	r0, r0, r4
	str	r0, [sp, #80]                   @ 4-byte Spill
	ldr	r0, [sp, #72]                   @ 4-byte Reload
	adcs	r0, r0, r5
	str	r0, [sp, #72]                   @ 4-byte Spill
	ldr	r0, [sp, #68]                   @ 4-byte Reload
	adcs	r0, r0, r10
	str	r0, [sp, #68]                   @ 4-byte Spill
	ldr	r0, [sp, #64]                   @ 4-byte Reload
	add	r10, sp, #636
	adcs	r0, r0, r9
	str	r0, [sp, #64]                   @ 4-byte Spill
	ldr	r0, [sp, #60]                   @ 4-byte Reload
	adcs	r0, r0, r6
	str	r0, [sp, #60]                   @ 4-byte Spill
	ldr	r0, [sp, #56]                   @ 4-byte Reload
	adcs	r0, r0, r1
	str	r0, [sp, #56]                   @ 4-byte Spill
	ldr	r0, [sp, #52]                   @ 4-byte Reload
	ldr	r1, [sp, #12]                   @ 4-byte Reload
	adcs	r0, r0, r1
	str	r0, [sp, #52]                   @ 4-byte Spill
	ldr	r0, [sp, #48]                   @ 4-byte Reload
	ldr	r1, [sp, #16]                   @ 4-byte Reload
	adcs	r0, r0, r1
	str	r0, [sp, #48]                   @ 4-byte Spill
	ldr	r0, [sp, #44]                   @ 4-byte Reload
	ldr	r1, [sp, #20]                   @ 4-byte Reload
	adcs	r0, r0, r1
	str	r0, [sp, #44]                   @ 4-byte Spill
	ldr	r0, [sp, #40]                   @ 4-byte Reload
	ldr	r1, [sp, #24]                   @ 4-byte Reload
	adcs	r0, r0, r1
	str	r0, [sp, #40]                   @ 4-byte Spill
	ldr	r0, [sp, #36]                   @ 4-byte Reload
	ldr	r1, [sp, #28]                   @ 4-byte Reload
	adcs	r0, r0, r1
	str	r0, [sp, #36]                   @ 4-byte Spill
	ldr	r0, [sp, #32]                   @ 4-byte Reload
	adc	r0, r0, #0
	str	r0, [sp, #32]                   @ 4-byte Spill
	ldr	r4, [sp, #608]
	ldm	lr, {r0, r1, r2, r3, r12, lr}
	adds	r11, r11, r4
	ldr	r4, [sp, #80]                   @ 4-byte Reload
	ldm	r10, {r5, r6, r7, r8, r9, r10}
	adcs	r0, r4, r0
	str	r0, [sp, #84]                   @ 4-byte Spill
	mov	r4, r11
	ldr	r0, [sp, #72]                   @ 4-byte Reload
	adcs	r0, r0, r1
	str	r0, [sp, #80]                   @ 4-byte Spill
	ldr	r0, [sp, #68]                   @ 4-byte Reload
	ldr	r1, [sp, #100]                  @ 4-byte Reload
	adcs	r0, r0, r2
	str	r0, [sp, #72]                   @ 4-byte Spill
	ldr	r0, [sp, #64]                   @ 4-byte Reload
	adcs	r0, r0, r3
	str	r0, [sp, #68]                   @ 4-byte Spill
	ldr	r0, [sp, #60]                   @ 4-byte Reload
	adcs	r0, r0, r12
	str	r0, [sp, #64]                   @ 4-byte Spill
	ldr	r0, [sp, #56]                   @ 4-byte Reload
	adcs	r0, r0, lr
	str	r0, [sp, #60]                   @ 4-byte Spill
	ldr	r0, [sp, #52]                   @ 4-byte Reload
	adcs	r0, r0, r5
	str	r0, [sp, #56]                   @ 4-byte Spill
	ldr	r0, [sp, #48]                   @ 4-byte Reload
	adcs	r0, r0, r6
	str	r0, [sp, #52]                   @ 4-byte Spill
	ldr	r0, [sp, #44]                   @ 4-byte Reload
	adcs	r0, r0, r7
	str	r0, [sp, #48]                   @ 4-byte Spill
	ldr	r0, [sp, #40]                   @ 4-byte Reload
	adcs	r0, r0, r8
	str	r0, [sp, #44]                   @ 4-byte Spill
	ldr	r0, [sp, #36]                   @ 4-byte Reload
	adcs	r0, r0, r9
	str	r0, [sp, #40]                   @ 4-byte Spill
	ldr	r0, [sp, #32]                   @ 4-byte Reload
	adcs	r0, r0, r10
	str	r0, [sp, #36]                   @ 4-byte Spill
	mov	r0, #0
	adc	r0, r0, #0
	str	r0, [sp, #32]                   @ 4-byte Spill
	ldr	r0, [sp, #88]                   @ 4-byte Reload
	mul	r2, r0, r11
	add	r0, sp, #552
	bl	mulPv384x32
	ldr	r0, [sp, #92]                   @ 4-byte Reload
	add	r9, sp, #560
	ldr	r1, [sp, #96]                   @ 4-byte Reload
	ldr	r2, [r0, #32]
	ldr	r0, [sp, #600]
	str	r0, [sp, #28]                   @ 4-byte Spill
	ldr	r0, [sp, #596]
	str	r0, [sp, #24]                   @ 4-byte Spill
	ldr	r0, [sp, #592]
	str	r0, [sp, #20]                   @ 4-byte Spill
	ldr	r0, [sp, #588]
	str	r0, [sp, #16]                   @ 4-byte Spill
	ldr	r0, [sp, #584]
	str	r0, [sp, #12]                   @ 4-byte Spill
	ldr	r0, [sp, #580]
	str	r0, [sp, #8]                    @ 4-byte Spill
	add	r0, sp, #496
	ldm	r9, {r6, r7, r9}
	ldr	r5, [sp, #576]
	ldr	r8, [sp, #572]
	ldr	r10, [sp, #552]
	ldr	r11, [sp, #556]
	bl	mulPv384x32
	adds	r0, r4, r10
	ldr	r1, [sp, #8]                    @ 4-byte Reload
	ldr	r0, [sp, #84]                   @ 4-byte Reload
	add	lr, sp, #496
	adcs	r0, r0, r11
	str	r0, [sp, #84]                   @ 4-byte Spill
	ldr	r0, [sp, #80]                   @ 4-byte Reload
	add	r11, sp, #520
	adcs	r0, r0, r6
	str	r0, [sp, #80]                   @ 4-byte Spill
	ldr	r0, [sp, #72]                   @ 4-byte Reload
	adcs	r0, r0, r7
	str	r0, [sp, #72]                   @ 4-byte Spill
	ldr	r0, [sp, #68]                   @ 4-byte Reload
	ldr	r7, [sp, #84]                   @ 4-byte Reload
	adcs	r0, r0, r9
	str	r0, [sp, #68]                   @ 4-byte Spill
	ldr	r0, [sp, #64]                   @ 4-byte Reload
	adcs	r0, r0, r8
	str	r0, [sp, #64]                   @ 4-byte Spill
	ldr	r0, [sp, #60]                   @ 4-byte Reload
	adcs	r0, r0, r5
	str	r0, [sp, #60]                   @ 4-byte Spill
	ldr	r0, [sp, #56]                   @ 4-byte Reload
	adcs	r0, r0, r1
	str	r0, [sp, #56]                   @ 4-byte Spill
	ldr	r0, [sp, #52]                   @ 4-byte Reload
	ldr	r1, [sp, #12]                   @ 4-byte Reload
	adcs	r0, r0, r1
	str	r0, [sp, #52]                   @ 4-byte Spill
	ldr	r0, [sp, #48]                   @ 4-byte Reload
	ldr	r1, [sp, #16]                   @ 4-byte Reload
	adcs	r0, r0, r1
	str	r0, [sp, #48]                   @ 4-byte Spill
	ldr	r0, [sp, #44]                   @ 4-byte Reload
	ldr	r1, [sp, #20]                   @ 4-byte Reload
	adcs	r0, r0, r1
	str	r0, [sp, #44]                   @ 4-byte Spill
	ldr	r0, [sp, #40]                   @ 4-byte Reload
	ldr	r1, [sp, #24]                   @ 4-byte Reload
	adcs	r0, r0, r1
	str	r0, [sp, #40]                   @ 4-byte Spill
	ldr	r0, [sp, #36]                   @ 4-byte Reload
	ldr	r1, [sp, #28]                   @ 4-byte Reload
	adcs	r0, r0, r1
	str	r0, [sp, #36]                   @ 4-byte Spill
	ldr	r0, [sp, #32]                   @ 4-byte Reload
	adc	r0, r0, #0
	str	r0, [sp, #32]                   @ 4-byte Spill
	ldm	lr, {r0, r1, r2, r3, r12, lr}
	adds	r7, r7, r0
	ldr	r0, [sp, #80]                   @ 4-byte Reload
	ldm	r11, {r4, r5, r6, r8, r9, r10, r11}
	adcs	r0, r0, r1
	str	r0, [sp, #84]                   @ 4-byte Spill
	ldr	r0, [sp, #72]                   @ 4-byte Reload
	ldr	r1, [sp, #100]                  @ 4-byte Reload
	adcs	r0, r0, r2
	str	r0, [sp, #80]                   @ 4-byte Spill
	ldr	r0, [sp, #68]                   @ 4-byte Reload
	adcs	r0, r0, r3
	str	r0, [sp, #72]                   @ 4-byte Spill
	ldr	r0, [sp, #64]                   @ 4-byte Reload
	adcs	r0, r0, r12
	str	r0, [sp, #68]                   @ 4-byte Spill
	ldr	r0, [sp, #60]                   @ 4-byte Reload
	adcs	r0, r0, lr
	str	r0, [sp, #64]                   @ 4-byte Spill
	ldr	r0, [sp, #56]                   @ 4-byte Reload
	adcs	r0, r0, r4
	str	r0, [sp, #60]                   @ 4-byte Spill
	ldr	r0, [sp, #52]                   @ 4-byte Reload
	adcs	r0, r0, r5
	str	r0, [sp, #56]                   @ 4-byte Spill
	ldr	r0, [sp, #48]                   @ 4-byte Reload
	adcs	r0, r0, r6
	str	r0, [sp, #52]                   @ 4-byte Spill
	ldr	r0, [sp, #44]                   @ 4-byte Reload
	adcs	r0, r0, r8
	str	r0, [sp, #48]                   @ 4-byte Spill
	ldr	r0, [sp, #40]                   @ 4-byte Reload
	mov	r8, r7
	adcs	r0, r0, r9
	str	r0, [sp, #44]                   @ 4-byte Spill
	ldr	r0, [sp, #36]                   @ 4-byte Reload
	adcs	r0, r0, r10
	str	r0, [sp, #40]                   @ 4-byte Spill
	ldr	r0, [sp, #32]                   @ 4-byte Reload
	adcs	r0, r0, r11
	str	r0, [sp, #36]                   @ 4-byte Spill
	mov	r0, #0
	adc	r0, r0, #0
	str	r0, [sp, #32]                   @ 4-byte Spill
	ldr	r0, [sp, #88]                   @ 4-byte Reload
	mul	r2, r0, r7
	add	r0, sp, #440
	bl	mulPv384x32
	ldr	r0, [sp, #92]                   @ 4-byte Reload
	add	r10, sp, #448
	ldr	r1, [sp, #96]                   @ 4-byte Reload
	ldr	r2, [r0, #36]
	ldr	r0, [sp, #488]
	str	r0, [sp, #28]                   @ 4-byte Spill
	ldr	r0, [sp, #484]
	str	r0, [sp, #24]                   @ 4-byte Spill
	ldr	r0, [sp, #480]
	str	r0, [sp, #20]                   @ 4-byte Spill
	ldr	r0, [sp, #476]
	str	r0, [sp, #16]                   @ 4-byte Spill
	ldr	r0, [sp, #472]
	str	r0, [sp, #12]                   @ 4-byte Spill
	ldr	r0, [sp, #468]
	str	r0, [sp, #8]                    @ 4-byte Spill
	add	r0, sp, #384
	ldm	r10, {r4, r5, r10}
	ldr	r6, [sp, #464]
	ldr	r9, [sp, #460]
	ldr	r11, [sp, #440]
	ldr	r7, [sp, #444]
	bl	mulPv384x32
	adds	r0, r8, r11
	ldr	r1, [sp, #8]                    @ 4-byte Reload
	ldr	r0, [sp, #84]                   @ 4-byte Reload
	add	lr, sp, #388
	adcs	r0, r0, r7
	str	r0, [sp, #84]                   @ 4-byte Spill
	ldr	r0, [sp, #80]                   @ 4-byte Reload
	ldr	r11, [sp, #84]                  @ 4-byte Reload
	adcs	r0, r0, r4
	str	r0, [sp, #80]                   @ 4-byte Spill
	ldr	r0, [sp, #72]                   @ 4-byte Reload
	adcs	r0, r0, r5
	str	r0, [sp, #72]                   @ 4-byte Spill
	ldr	r0, [sp, #68]                   @ 4-byte Reload
	adcs	r0, r0, r10
	str	r0, [sp, #68]                   @ 4-byte Spill
	ldr	r0, [sp, #64]                   @ 4-byte Reload
	add	r10, sp, #412
	adcs	r0, r0, r9
	str	r0, [sp, #64]                   @ 4-byte Spill
	ldr	r0, [sp, #60]                   @ 4-byte Reload
	adcs	r0, r0, r6
	str	r0, [sp, #60]                   @ 4-byte Spill
	ldr	r0, [sp, #56]                   @ 4-byte Reload
	adcs	r0, r0, r1
	str	r0, [sp, #56]                   @ 4-byte Spill
	ldr	r0, [sp, #52]                   @ 4-byte Reload
	ldr	r1, [sp, #12]                   @ 4-byte Reload
	adcs	r0, r0, r1
	str	r0, [sp, #52]                   @ 4-byte Spill
	ldr	r0, [sp, #48]                   @ 4-byte Reload
	ldr	r1, [sp, #16]                   @ 4-byte Reload
	adcs	r0, r0, r1
	str	r0, [sp, #48]                   @ 4-byte Spill
	ldr	r0, [sp, #44]                   @ 4-byte Reload
	ldr	r1, [sp, #20]                   @ 4-byte Reload
	adcs	r0, r0, r1
	str	r0, [sp, #44]                   @ 4-byte Spill
	ldr	r0, [sp, #40]                   @ 4-byte Reload
	ldr	r1, [sp, #24]                   @ 4-byte Reload
	adcs	r0, r0, r1
	str	r0, [sp, #40]                   @ 4-byte Spill
	ldr	r0, [sp, #36]                   @ 4-byte Reload
	ldr	r1, [sp, #28]                   @ 4-byte Reload
	adcs	r0, r0, r1
	str	r0, [sp, #36]                   @ 4-byte Spill
	ldr	r0, [sp, #32]                   @ 4-byte Reload
	adc	r0, r0, #0
	str	r0, [sp, #32]                   @ 4-byte Spill
	ldr	r4, [sp, #384]
	ldm	lr, {r0, r1, r2, r3, r12, lr}
	adds	r11, r11, r4
	ldr	r4, [sp, #80]                   @ 4-byte Reload
	ldm	r10, {r5, r6, r7, r8, r9, r10}
	adcs	r0, r4, r0
	str	r0, [sp, #84]                   @ 4-byte Spill
	mov	r4, r11
	ldr	r0, [sp, #72]                   @ 4-byte Reload
	adcs	r0, r0, r1
	str	r0, [sp, #80]                   @ 4-byte Spill
	ldr	r0, [sp, #68]                   @ 4-byte Reload
	ldr	r1, [sp, #100]                  @ 4-byte Reload
	adcs	r0, r0, r2
	str	r0, [sp, #72]                   @ 4-byte Spill
	ldr	r0, [sp, #64]                   @ 4-byte Reload
	adcs	r0, r0, r3
	str	r0, [sp, #68]                   @ 4-byte Spill
	ldr	r0, [sp, #60]                   @ 4-byte Reload
	adcs	r0, r0, r12
	str	r0, [sp, #64]                   @ 4-byte Spill
	ldr	r0, [sp, #56]                   @ 4-byte Reload
	adcs	r0, r0, lr
	str	r0, [sp, #60]                   @ 4-byte Spill
	ldr	r0, [sp, #52]                   @ 4-byte Reload
	adcs	r0, r0, r5
	str	r0, [sp, #56]                   @ 4-byte Spill
	ldr	r0, [sp, #48]                   @ 4-byte Reload
	adcs	r0, r0, r6
	str	r0, [sp, #52]                   @ 4-byte Spill
	ldr	r0, [sp, #44]                   @ 4-byte Reload
	adcs	r0, r0, r7
	str	r0, [sp, #48]                   @ 4-byte Spill
	ldr	r0, [sp, #40]                   @ 4-byte Reload
	adcs	r0, r0, r8
	str	r0, [sp, #44]                   @ 4-byte Spill
	ldr	r0, [sp, #36]                   @ 4-byte Reload
	ldr	r8, [sp, #88]                   @ 4-byte Reload
	adcs	r0, r0, r9
	str	r0, [sp, #40]                   @ 4-byte Spill
	ldr	r0, [sp, #32]                   @ 4-byte Reload
	mul	r2, r8, r11
	adcs	r0, r0, r10
	str	r0, [sp, #36]                   @ 4-byte Spill
	mov	r0, #0
	adc	r0, r0, #0
	str	r0, [sp, #32]                   @ 4-byte Spill
	add	r0, sp, #328
	bl	mulPv384x32
	ldr	r0, [sp, #92]                   @ 4-byte Reload
	add	r9, sp, #336
	ldr	r1, [sp, #96]                   @ 4-byte Reload
	ldr	r2, [r0, #40]
	ldr	r0, [sp, #376]
	str	r0, [sp, #28]                   @ 4-byte Spill
	ldr	r0, [sp, #372]
	str	r0, [sp, #24]                   @ 4-byte Spill
	ldr	r0, [sp, #368]
	str	r0, [sp, #20]                   @ 4-byte Spill
	ldr	r0, [sp, #364]
	str	r0, [sp, #16]                   @ 4-byte Spill
	ldr	r0, [sp, #360]
	str	r0, [sp, #12]                   @ 4-byte Spill
	ldr	r0, [sp, #356]
	str	r0, [sp, #8]                    @ 4-byte Spill
	ldr	r0, [sp, #352]
	str	r0, [sp, #4]                    @ 4-byte Spill
	add	r0, sp, #272
	ldm	r9, {r6, r7, r9}
	ldr	r5, [sp, #348]
	ldr	r10, [sp, #328]
	ldr	r11, [sp, #332]
	bl	mulPv384x32
	adds	r0, r4, r10
	ldr	r1, [sp, #4]                    @ 4-byte Reload
	ldr	r0, [sp, #84]                   @ 4-byte Reload
	add	lr, sp, #272
	add	r12, sp, #288
	adcs	r4, r0, r11
	ldr	r0, [sp, #80]                   @ 4-byte Reload
	adcs	r10, r0, r6
	ldr	r0, [sp, #72]                   @ 4-byte Reload
	adcs	r0, r0, r7
	str	r0, [sp, #80]                   @ 4-byte Spill
	ldr	r0, [sp, #68]                   @ 4-byte Reload
	adcs	r0, r0, r9
	str	r0, [sp, #72]                   @ 4-byte Spill
	ldr	r0, [sp, #64]                   @ 4-byte Reload
	add	r9, sp, #308
	adcs	r0, r0, r5
	str	r0, [sp, #68]                   @ 4-byte Spill
	ldr	r0, [sp, #60]                   @ 4-byte Reload
	adcs	r0, r0, r1
	str	r0, [sp, #64]                   @ 4-byte Spill
	ldr	r0, [sp, #56]                   @ 4-byte Reload
	ldr	r1, [sp, #8]                    @ 4-byte Reload
	adcs	r0, r0, r1
	str	r0, [sp, #60]                   @ 4-byte Spill
	ldr	r0, [sp, #52]                   @ 4-byte Reload
	ldr	r1, [sp, #12]                   @ 4-byte Reload
	adcs	r0, r0, r1
	str	r0, [sp, #56]                   @ 4-byte Spill
	ldr	r0, [sp, #48]                   @ 4-byte Reload
	ldr	r1, [sp, #16]                   @ 4-byte Reload
	adcs	r0, r0, r1
	str	r0, [sp, #52]                   @ 4-byte Spill
	ldr	r0, [sp, #44]                   @ 4-byte Reload
	ldr	r1, [sp, #20]                   @ 4-byte Reload
	adcs	r0, r0, r1
	str	r0, [sp, #48]                   @ 4-byte Spill
	ldr	r0, [sp, #40]                   @ 4-byte Reload
	ldr	r1, [sp, #24]                   @ 4-byte Reload
	adcs	r0, r0, r1
	str	r0, [sp, #44]                   @ 4-byte Spill
	ldr	r0, [sp, #36]                   @ 4-byte Reload
	ldr	r1, [sp, #28]                   @ 4-byte Reload
	adcs	r0, r0, r1
	str	r0, [sp, #40]                   @ 4-byte Spill
	ldr	r0, [sp, #32]                   @ 4-byte Reload
	adc	r0, r0, #0
	str	r0, [sp, #24]                   @ 4-byte Spill
	ldm	lr, {r2, r6, lr}
	adds	r0, r4, r2
	ldr	r5, [sp, #284]
	str	r0, [sp, #84]                   @ 4-byte Spill
	adcs	r6, r10, r6
	mul	r11, r8, r0
	ldm	r9, {r4, r7, r8, r9}
	ldm	r12, {r0, r1, r2, r3, r12}
	str	r6, [sp, #36]                   @ 4-byte Spill
	ldr	r6, [sp, #80]                   @ 4-byte Reload
	adcs	r6, r6, lr
	str	r6, [sp, #32]                   @ 4-byte Spill
	ldr	r6, [sp, #72]                   @ 4-byte Reload
	adcs	r6, r6, r5
	str	r6, [sp, #28]                   @ 4-byte Spill
	ldr	r6, [sp, #68]                   @ 4-byte Reload
	adcs	r0, r6, r0
	str	r0, [sp, #80]                   @ 4-byte Spill
	ldr	r0, [sp, #64]                   @ 4-byte Reload
	adcs	r0, r0, r1
	str	r0, [sp, #72]                   @ 4-byte Spill
	ldr	r0, [sp, #60]                   @ 4-byte Reload
	ldr	r1, [sp, #100]                  @ 4-byte Reload
	adcs	r0, r0, r2
	str	r0, [sp, #68]                   @ 4-byte Spill
	ldr	r0, [sp, #56]                   @ 4-byte Reload
	mov	r2, r11
	adcs	r0, r0, r3
	str	r0, [sp, #64]                   @ 4-byte Spill
	ldr	r0, [sp, #52]                   @ 4-byte Reload
	adcs	r0, r0, r12
	str	r0, [sp, #60]                   @ 4-byte Spill
	ldr	r0, [sp, #48]                   @ 4-byte Reload
	adcs	r0, r0, r4
	str	r0, [sp, #56]                   @ 4-byte Spill
	ldr	r0, [sp, #44]                   @ 4-byte Reload
	adcs	r0, r0, r7
	str	r0, [sp, #52]                   @ 4-byte Spill
	ldr	r0, [sp, #40]                   @ 4-byte Reload
	adcs	r0, r0, r8
	str	r0, [sp, #48]                   @ 4-byte Spill
	ldr	r0, [sp, #24]                   @ 4-byte Reload
	adcs	r0, r0, r9
	str	r0, [sp, #44]                   @ 4-byte Spill
	mov	r0, #0
	adc	r0, r0, #0
	str	r0, [sp, #40]                   @ 4-byte Spill
	add	r0, sp, #216
	bl	mulPv384x32
	ldr	r1, [sp, #92]                   @ 4-byte Reload
	add	r0, sp, #160
	ldr	r2, [r1, #44]
	ldr	r1, [sp, #264]
	str	r1, [sp, #92]                   @ 4-byte Spill
	ldr	r1, [sp, #260]
	str	r1, [sp, #24]                   @ 4-byte Spill
	ldr	r1, [sp, #256]
	str	r1, [sp, #20]                   @ 4-byte Spill
	ldr	r1, [sp, #252]
	str	r1, [sp, #16]                   @ 4-byte Spill
	ldr	r1, [sp, #248]
	str	r1, [sp, #12]                   @ 4-byte Spill
	ldr	r1, [sp, #96]                   @ 4-byte Reload
	ldr	r7, [sp, #244]
	ldr	r4, [sp, #240]
	ldr	r8, [sp, #236]
	ldr	r9, [sp, #232]
	ldr	r11, [sp, #216]
	ldr	r5, [sp, #220]
	ldr	r10, [sp, #224]
	ldr	r6, [sp, #228]
	bl	mulPv384x32
	ldr	r0, [sp, #84]                   @ 4-byte Reload
	add	lr, sp, #160
	ldr	r1, [sp, #32]                   @ 4-byte Reload
	add	r12, sp, #176
	adds	r0, r0, r11
	ldr	r2, [sp, #12]                   @ 4-byte Reload
	ldr	r0, [sp, #36]                   @ 4-byte Reload
	adcs	r0, r0, r5
	adcs	r11, r1, r10
	ldr	r1, [sp, #28]                   @ 4-byte Reload
	add	r10, sp, #196
	adcs	r1, r1, r6
	str	r1, [sp, #96]                   @ 4-byte Spill
	ldr	r1, [sp, #80]                   @ 4-byte Reload
	adcs	r1, r1, r9
	str	r1, [sp, #84]                   @ 4-byte Spill
	ldr	r1, [sp, #72]                   @ 4-byte Reload
	adcs	r1, r1, r8
	str	r1, [sp, #80]                   @ 4-byte Spill
	ldr	r1, [sp, #68]                   @ 4-byte Reload
	adcs	r1, r1, r4
	str	r1, [sp, #72]                   @ 4-byte Spill
	ldr	r1, [sp, #64]                   @ 4-byte Reload
	adcs	r1, r1, r7
	str	r1, [sp, #68]                   @ 4-byte Spill
	ldr	r1, [sp, #60]                   @ 4-byte Reload
	adcs	r1, r1, r2
	str	r1, [sp, #64]                   @ 4-byte Spill
	ldr	r1, [sp, #56]                   @ 4-byte Reload
	ldr	r2, [sp, #16]                   @ 4-byte Reload
	adcs	r1, r1, r2
	str	r1, [sp, #60]                   @ 4-byte Spill
	ldr	r1, [sp, #52]                   @ 4-byte Reload
	ldr	r2, [sp, #20]                   @ 4-byte Reload
	adcs	r1, r1, r2
	str	r1, [sp, #56]                   @ 4-byte Spill
	ldr	r1, [sp, #48]                   @ 4-byte Reload
	ldr	r2, [sp, #24]                   @ 4-byte Reload
	adcs	r1, r1, r2
	str	r1, [sp, #52]                   @ 4-byte Spill
	ldr	r1, [sp, #44]                   @ 4-byte Reload
	ldr	r2, [sp, #92]                   @ 4-byte Reload
	adcs	r1, r1, r2
	str	r1, [sp, #92]                   @ 4-byte Spill
	ldr	r1, [sp, #40]                   @ 4-byte Reload
	adc	r1, r1, #0
	str	r1, [sp, #48]                   @ 4-byte Spill
	ldm	lr, {r2, r7, lr}
	adds	r4, r0, r2
	ldr	r0, [sp, #88]                   @ 4-byte Reload
	ldr	r6, [sp, #172]
	adcs	r7, r11, r7
	mul	r1, r0, r4
	str	r1, [sp, #44]                   @ 4-byte Spill
	ldm	r10, {r5, r8, r9, r10}
	ldm	r12, {r0, r1, r2, r3, r12}
	str	r7, [sp, #36]                   @ 4-byte Spill
	ldr	r7, [sp, #96]                   @ 4-byte Reload
	adcs	r11, r7, lr
	ldr	r7, [sp, #84]                   @ 4-byte Reload
	adcs	r6, r7, r6
	ldr	r7, [sp, #80]                   @ 4-byte Reload
	adcs	r0, r7, r0
	str	r0, [sp, #96]                   @ 4-byte Spill
	ldr	r0, [sp, #72]                   @ 4-byte Reload
	adcs	r0, r0, r1
	str	r0, [sp, #40]                   @ 4-byte Spill
	ldr	r0, [sp, #68]                   @ 4-byte Reload
	adcs	r0, r0, r2
	str	r0, [sp, #88]                   @ 4-byte Spill
	ldr	r0, [sp, #64]                   @ 4-byte Reload
	ldr	r2, [sp, #44]                   @ 4-byte Reload
	adcs	r0, r0, r3
	str	r0, [sp, #84]                   @ 4-byte Spill
	ldr	r0, [sp, #60]                   @ 4-byte Reload
	adcs	r0, r0, r12
	str	r0, [sp, #80]                   @ 4-byte Spill
	ldr	r0, [sp, #56]                   @ 4-byte Reload
	adcs	r7, r0, r5
	ldr	r0, [sp, #52]                   @ 4-byte Reload
	ldr	r5, [sp, #100]                  @ 4-byte Reload
	adcs	r8, r0, r8
	ldr	r0, [sp, #92]                   @ 4-byte Reload
	adcs	r0, r0, r9
	str	r0, [sp, #68]                   @ 4-byte Spill
	ldr	r0, [sp, #48]                   @ 4-byte Reload
	mov	r1, r5
	adcs	r9, r0, r10
	mov	r0, #0
	adc	r10, r0, #0
	add	r0, sp, #104
	bl	mulPv384x32
	add	r3, sp, #104
	ldm	r3, {r0, r1, r2, r3}
	adds	r0, r4, r0
	ldr	r0, [sp, #36]                   @ 4-byte Reload
	adcs	r12, r0, r1
	str	r12, [sp, #72]                  @ 4-byte Spill
	adcs	r2, r11, r2
	str	r2, [sp, #64]                   @ 4-byte Spill
	adcs	r4, r6, r3
	str	r4, [sp, #56]                   @ 4-byte Spill
	ldr	r0, [sp, #120]
	ldr	r1, [sp, #96]                   @ 4-byte Reload
	adcs	r0, r1, r0
	str	r0, [sp, #96]                   @ 4-byte Spill
	ldr	r0, [sp, #124]
	ldr	r1, [sp, #40]                   @ 4-byte Reload
	adcs	r0, r1, r0
	str	r0, [sp, #92]                   @ 4-byte Spill
	ldr	r0, [sp, #128]
	ldr	r1, [sp, #88]                   @ 4-byte Reload
	adcs	r0, r1, r0
	str	r0, [sp, #88]                   @ 4-byte Spill
	ldr	r0, [sp, #132]
	ldr	r1, [sp, #84]                   @ 4-byte Reload
	adcs	r0, r1, r0
	str	r0, [sp, #84]                   @ 4-byte Spill
	ldr	r0, [sp, #136]
	ldr	r1, [sp, #80]                   @ 4-byte Reload
	adcs	r0, r1, r0
	str	r0, [sp, #80]                   @ 4-byte Spill
	ldr	r0, [sp, #140]
	ldr	r1, [sp, #68]                   @ 4-byte Reload
	adcs	r0, r7, r0
	str	r0, [sp, #44]                   @ 4-byte Spill
	ldr	r0, [sp, #144]
	adcs	r0, r8, r0
	str	r0, [sp, #40]                   @ 4-byte Spill
	ldr	r0, [sp, #148]
	adcs	r0, r1, r0
	str	r0, [sp, #36]                   @ 4-byte Spill
	ldr	r0, [sp, #152]
	adcs	r0, r9, r0
	str	r0, [sp, #28]                   @ 4-byte Spill
	adc	r0, r10, #0
	str	r0, [sp, #32]                   @ 4-byte Spill
	ldm	r5, {r0, r1, r3, r7, lr}
	add	r9, r5, #20
	add	r5, r5, #32
	subs	r0, r12, r0
	ldm	r9, {r6, r8, r9}
	str	r0, [sp, #68]                   @ 4-byte Spill
	sbcs	r0, r2, r1
	str	r0, [sp, #60]                   @ 4-byte Spill
	sbcs	r0, r4, r3
	str	r0, [sp, #52]                   @ 4-byte Spill
	ldr	r0, [sp, #96]                   @ 4-byte Reload
	ldr	r4, [sp, #88]                   @ 4-byte Reload
	sbcs	r0, r0, r7
	str	r0, [sp, #48]                   @ 4-byte Spill
	ldr	r0, [sp, #92]                   @ 4-byte Reload
	ldm	r5, {r1, r2, r3, r5}
	sbcs	r11, r0, lr
	ldr	r0, [sp, #84]                   @ 4-byte Reload
	sbcs	r10, r4, r6
	ldr	r4, [sp, #44]                   @ 4-byte Reload
	sbcs	r8, r0, r8
	ldr	r0, [sp, #80]                   @ 4-byte Reload
	ldr	lr, [sp, #40]                   @ 4-byte Reload
	sbcs	r9, r0, r9
	ldr	r12, [sp, #36]                  @ 4-byte Reload
	sbcs	r1, r4, r1
	ldr	r0, [sp, #28]                   @ 4-byte Reload
	sbcs	r2, lr, r2
	sbcs	r3, r12, r3
	sbcs	r6, r0, r5
	ldr	r5, [sp, #32]                   @ 4-byte Reload
	sbc	r5, r5, #0
	ands	r7, r5, #1
	ldr	r5, [sp, #76]                   @ 4-byte Reload
	movne	r6, r0
	movne	r3, r12
	movne	r2, lr
	cmp	r7, #0
	movne	r1, r4
	add	r0, r5, #32
	stm	r0, {r1, r2, r3, r6}
	ldr	r0, [sp, #80]                   @ 4-byte Reload
	ldr	r1, [sp, #88]                   @ 4-byte Reload
	movne	r9, r0
	ldr	r0, [sp, #84]                   @ 4-byte Reload
	str	r9, [r5, #28]
	movne	r8, r0
	cmp	r7, #0
	movne	r10, r1
	ldr	r1, [sp, #92]                   @ 4-byte Reload
	ldr	r0, [sp, #96]                   @ 4-byte Reload
	str	r8, [r5, #24]
	movne	r11, r1
	ldr	r1, [sp, #48]                   @ 4-byte Reload
	movne	r1, r0
	cmp	r7, #0
	str	r1, [r5, #12]
	ldr	r1, [sp, #56]                   @ 4-byte Reload
	ldr	r0, [sp, #52]                   @ 4-byte Reload
	str	r10, [r5, #20]
	movne	r0, r1
	ldr	r1, [sp, #64]                   @ 4-byte Reload
	str	r0, [r5, #8]
	ldr	r0, [sp, #60]                   @ 4-byte Reload
	movne	r0, r1
	ldr	r1, [sp, #72]                   @ 4-byte Reload
	str	r0, [r5, #4]
	ldr	r0, [sp, #68]                   @ 4-byte Reload
	movne	r0, r1
	str	r11, [r5, #16]
	str	r0, [r5]
	add	sp, sp, #428
	add	sp, sp, #1024
	pop	{r4, r5, r6, r7, r8, r9, r10, r11, lr}
	mov	pc, lr
.Lfunc_end60:
	.size	mcl_fp_mont12L, .Lfunc_end60-mcl_fp_mont12L
	.cantunwind
	.fnend
                                        @ -- End function
	.globl	mcl_fp_montNF12L                @ -- Begin function mcl_fp_montNF12L
	.p2align	2
	.type	mcl_fp_montNF12L,%function
	.code	32                              @ @mcl_fp_montNF12L
mcl_fp_montNF12L:
	.fnstart
@ %bb.0:
	.save	{r4, r5, r6, r7, r8, r9, r10, r11, lr}
	push	{r4, r5, r6, r7, r8, r9, r10, r11, lr}
	.pad	#428
	sub	sp, sp, #428
	.pad	#1024
	sub	sp, sp, #1024
	mov	r7, r2
	ldr	r2, [r2]
	str	r0, [sp, #84]                   @ 4-byte Spill
	add	r0, sp, #1392
	ldr	r5, [r3, #-4]
	mov	r4, r3
	str	r1, [sp, #96]                   @ 4-byte Spill
	str	r5, [sp, #88]                   @ 4-byte Spill
	str	r3, [sp, #100]                  @ 4-byte Spill
	str	r7, [sp, #92]                   @ 4-byte Spill
	bl	mulPv384x32
	ldr	r0, [sp, #1396]
	add	lr, sp, #1024
	ldr	r8, [sp, #1392]
	mov	r1, r4
	str	r0, [sp, #64]                   @ 4-byte Spill
	ldr	r0, [sp, #1400]
	str	r0, [sp, #60]                   @ 4-byte Spill
	mul	r2, r5, r8
	ldr	r0, [sp, #1404]
	str	r0, [sp, #56]                   @ 4-byte Spill
	ldr	r0, [sp, #1440]
	str	r0, [sp, #80]                   @ 4-byte Spill
	ldr	r0, [sp, #1436]
	str	r0, [sp, #76]                   @ 4-byte Spill
	ldr	r0, [sp, #1432]
	str	r0, [sp, #72]                   @ 4-byte Spill
	ldr	r0, [sp, #1428]
	str	r0, [sp, #68]                   @ 4-byte Spill
	ldr	r0, [sp, #1424]
	str	r0, [sp, #52]                   @ 4-byte Spill
	ldr	r0, [sp, #1420]
	str	r0, [sp, #48]                   @ 4-byte Spill
	ldr	r0, [sp, #1416]
	str	r0, [sp, #44]                   @ 4-byte Spill
	ldr	r0, [sp, #1412]
	str	r0, [sp, #40]                   @ 4-byte Spill
	ldr	r0, [sp, #1408]
	str	r0, [sp, #36]                   @ 4-byte Spill
	add	r0, lr, #312
	bl	mulPv384x32
	ldr	r0, [sp, #1384]
	str	r0, [sp, #32]                   @ 4-byte Spill
	ldr	r0, [sp, #1380]
	str	r0, [sp, #28]                   @ 4-byte Spill
	ldr	r0, [sp, #1376]
	str	r0, [sp, #24]                   @ 4-byte Spill
	ldr	r0, [sp, #1372]
	str	r0, [sp, #20]                   @ 4-byte Spill
	ldr	r0, [sp, #1368]
	str	r0, [sp, #16]                   @ 4-byte Spill
	ldr	r0, [sp, #1364]
	ldr	r2, [r7, #4]
	ldr	r1, [sp, #96]                   @ 4-byte Reload
	str	r0, [sp, #12]                   @ 4-byte Spill
	add	r0, sp, #1280
	ldr	r7, [sp, #1360]
	ldr	r11, [sp, #1356]
	ldr	r5, [sp, #1352]
	ldr	r6, [sp, #1336]
	ldr	r10, [sp, #1340]
	ldr	r9, [sp, #1344]
	ldr	r4, [sp, #1348]
	bl	mulPv384x32
	adds	r0, r6, r8
	ldr	r1, [sp, #12]                   @ 4-byte Reload
	ldr	r0, [sp, #64]                   @ 4-byte Reload
	adcs	r0, r10, r0
	str	r0, [sp, #64]                   @ 4-byte Spill
	ldr	r0, [sp, #60]                   @ 4-byte Reload
	ldr	r10, [sp, #64]                  @ 4-byte Reload
	adcs	r0, r9, r0
	str	r0, [sp, #60]                   @ 4-byte Spill
	ldr	r0, [sp, #56]                   @ 4-byte Reload
	adcs	r0, r4, r0
	str	r0, [sp, #56]                   @ 4-byte Spill
	ldr	r0, [sp, #36]                   @ 4-byte Reload
	adcs	r0, r5, r0
	str	r0, [sp, #36]                   @ 4-byte Spill
	ldr	r0, [sp, #40]                   @ 4-byte Reload
	adcs	r0, r11, r0
	str	r0, [sp, #40]                   @ 4-byte Spill
	ldr	r0, [sp, #44]                   @ 4-byte Reload
	adcs	r0, r7, r0
	str	r0, [sp, #44]                   @ 4-byte Spill
	ldr	r0, [sp, #48]                   @ 4-byte Reload
	adcs	r0, r1, r0
	str	r0, [sp, #48]                   @ 4-byte Spill
	ldr	r0, [sp, #52]                   @ 4-byte Reload
	ldr	r1, [sp, #16]                   @ 4-byte Reload
	adcs	r0, r1, r0
	str	r0, [sp, #52]                   @ 4-byte Spill
	ldr	r0, [sp, #68]                   @ 4-byte Reload
	ldr	r1, [sp, #20]                   @ 4-byte Reload
	adcs	r0, r1, r0
	str	r0, [sp, #20]                   @ 4-byte Spill
	ldr	r0, [sp, #72]                   @ 4-byte Reload
	ldr	r1, [sp, #24]                   @ 4-byte Reload
	adcs	r0, r1, r0
	str	r0, [sp, #24]                   @ 4-byte Spill
	ldr	r0, [sp, #76]                   @ 4-byte Reload
	ldr	r1, [sp, #28]                   @ 4-byte Reload
	adcs	r0, r1, r0
	str	r0, [sp, #28]                   @ 4-byte Spill
	ldr	r0, [sp, #80]                   @ 4-byte Reload
	ldr	r1, [sp, #32]                   @ 4-byte Reload
	adc	r0, r1, r0
	str	r0, [sp, #32]                   @ 4-byte Spill
	ldr	r4, [sp, #1280]
	ldr	r0, [sp, #1284]
	adds	r10, r10, r4
	ldr	r4, [sp, #60]                   @ 4-byte Reload
	ldr	r11, [sp, #1328]
	adcs	r0, r4, r0
	ldr	r9, [sp, #1324]
	ldr	r8, [sp, #1320]
	mov	r4, r10
	ldr	r7, [sp, #1316]
	ldr	r6, [sp, #1312]
	ldr	r5, [sp, #1308]
	ldr	lr, [sp, #1304]
	ldr	r12, [sp, #1300]
	ldr	r3, [sp, #1296]
	ldr	r1, [sp, #1288]
	ldr	r2, [sp, #1292]
	str	r0, [sp, #80]                   @ 4-byte Spill
	ldr	r0, [sp, #56]                   @ 4-byte Reload
	adcs	r0, r0, r1
	str	r0, [sp, #76]                   @ 4-byte Spill
	ldr	r0, [sp, #36]                   @ 4-byte Reload
	ldr	r1, [sp, #100]                  @ 4-byte Reload
	adcs	r0, r0, r2
	str	r0, [sp, #72]                   @ 4-byte Spill
	ldr	r0, [sp, #40]                   @ 4-byte Reload
	adcs	r0, r0, r3
	str	r0, [sp, #68]                   @ 4-byte Spill
	ldr	r0, [sp, #44]                   @ 4-byte Reload
	adcs	r0, r0, r12
	str	r0, [sp, #64]                   @ 4-byte Spill
	ldr	r0, [sp, #48]                   @ 4-byte Reload
	adcs	r0, r0, lr
	str	r0, [sp, #60]                   @ 4-byte Spill
	ldr	r0, [sp, #52]                   @ 4-byte Reload
	add	lr, sp, #1024
	adcs	r0, r0, r5
	str	r0, [sp, #56]                   @ 4-byte Spill
	ldr	r0, [sp, #20]                   @ 4-byte Reload
	adcs	r0, r0, r6
	str	r0, [sp, #52]                   @ 4-byte Spill
	ldr	r0, [sp, #24]                   @ 4-byte Reload
	adcs	r0, r0, r7
	str	r0, [sp, #48]                   @ 4-byte Spill
	ldr	r0, [sp, #28]                   @ 4-byte Reload
	adcs	r0, r0, r8
	str	r0, [sp, #44]                   @ 4-byte Spill
	ldr	r0, [sp, #32]                   @ 4-byte Reload
	adcs	r0, r0, r9
	str	r0, [sp, #40]                   @ 4-byte Spill
	adc	r0, r11, #0
	str	r0, [sp, #36]                   @ 4-byte Spill
	ldr	r0, [sp, #88]                   @ 4-byte Reload
	mul	r2, r0, r10
	add	r0, lr, #200
	bl	mulPv384x32
	ldr	r0, [sp, #92]                   @ 4-byte Reload
	add	r9, sp, #1232
	ldr	r1, [sp, #96]                   @ 4-byte Reload
	ldr	r2, [r0, #8]
	ldr	r0, [sp, #1272]
	str	r0, [sp, #32]                   @ 4-byte Spill
	ldr	r0, [sp, #1268]
	str	r0, [sp, #28]                   @ 4-byte Spill
	ldr	r0, [sp, #1264]
	str	r0, [sp, #24]                   @ 4-byte Spill
	ldr	r0, [sp, #1260]
	str	r0, [sp, #20]                   @ 4-byte Spill
	ldr	r0, [sp, #1256]
	str	r0, [sp, #16]                   @ 4-byte Spill
	ldr	r0, [sp, #1252]
	str	r0, [sp, #12]                   @ 4-byte Spill
	add	r0, sp, #1168
	ldm	r9, {r6, r7, r9}
	ldr	r5, [sp, #1248]
	ldr	r8, [sp, #1244]
	ldr	r10, [sp, #1224]
	ldr	r11, [sp, #1228]
	bl	mulPv384x32
	adds	r0, r4, r10
	ldr	r1, [sp, #12]                   @ 4-byte Reload
	ldr	r0, [sp, #80]                   @ 4-byte Reload
	add	lr, sp, #1168
	adcs	r0, r0, r11
	str	r0, [sp, #80]                   @ 4-byte Spill
	ldr	r0, [sp, #76]                   @ 4-byte Reload
	adcs	r0, r0, r6
	str	r0, [sp, #76]                   @ 4-byte Spill
	ldr	r0, [sp, #72]                   @ 4-byte Reload
	adcs	r0, r0, r7
	str	r0, [sp, #72]                   @ 4-byte Spill
	ldr	r0, [sp, #68]                   @ 4-byte Reload
	ldr	r7, [sp, #80]                   @ 4-byte Reload
	adcs	r0, r0, r9
	str	r0, [sp, #68]                   @ 4-byte Spill
	ldr	r0, [sp, #64]                   @ 4-byte Reload
	adcs	r0, r0, r8
	str	r0, [sp, #64]                   @ 4-byte Spill
	ldr	r0, [sp, #60]                   @ 4-byte Reload
	adcs	r0, r0, r5
	str	r0, [sp, #60]                   @ 4-byte Spill
	ldr	r0, [sp, #56]                   @ 4-byte Reload
	adcs	r0, r0, r1
	str	r0, [sp, #56]                   @ 4-byte Spill
	ldr	r0, [sp, #52]                   @ 4-byte Reload
	ldr	r1, [sp, #16]                   @ 4-byte Reload
	adcs	r0, r0, r1
	str	r0, [sp, #52]                   @ 4-byte Spill
	ldr	r0, [sp, #48]                   @ 4-byte Reload
	ldr	r1, [sp, #20]                   @ 4-byte Reload
	adcs	r0, r0, r1
	str	r0, [sp, #48]                   @ 4-byte Spill
	ldr	r0, [sp, #44]                   @ 4-byte Reload
	ldr	r1, [sp, #24]                   @ 4-byte Reload
	adcs	r0, r0, r1
	str	r0, [sp, #44]                   @ 4-byte Spill
	ldr	r0, [sp, #40]                   @ 4-byte Reload
	ldr	r1, [sp, #28]                   @ 4-byte Reload
	adcs	r0, r0, r1
	str	r0, [sp, #40]                   @ 4-byte Spill
	ldr	r0, [sp, #36]                   @ 4-byte Reload
	ldr	r1, [sp, #32]                   @ 4-byte Reload
	adc	r0, r0, r1
	str	r0, [sp, #36]                   @ 4-byte Spill
	ldm	lr, {r0, r1, r2, r3, r12, lr}
	adds	r7, r7, r0
	ldr	r0, [sp, #76]                   @ 4-byte Reload
	ldr	r11, [sp, #1216]
	adcs	r0, r0, r1
	ldr	r10, [sp, #1212]
	ldr	r9, [sp, #1208]
	ldr	r8, [sp, #1204]
	ldr	r6, [sp, #1200]
	ldr	r5, [sp, #1196]
	ldr	r4, [sp, #1192]
	str	r0, [sp, #80]                   @ 4-byte Spill
	ldr	r0, [sp, #72]                   @ 4-byte Reload
	ldr	r1, [sp, #100]                  @ 4-byte Reload
	adcs	r0, r0, r2
	str	r0, [sp, #76]                   @ 4-byte Spill
	ldr	r0, [sp, #68]                   @ 4-byte Reload
	adcs	r0, r0, r3
	str	r0, [sp, #72]                   @ 4-byte Spill
	ldr	r0, [sp, #64]                   @ 4-byte Reload
	adcs	r0, r0, r12
	str	r0, [sp, #68]                   @ 4-byte Spill
	ldr	r0, [sp, #60]                   @ 4-byte Reload
	adcs	r0, r0, lr
	str	r0, [sp, #64]                   @ 4-byte Spill
	ldr	r0, [sp, #56]                   @ 4-byte Reload
	add	lr, sp, #1024
	adcs	r0, r0, r4
	str	r0, [sp, #60]                   @ 4-byte Spill
	ldr	r0, [sp, #52]                   @ 4-byte Reload
	adcs	r0, r0, r5
	str	r0, [sp, #56]                   @ 4-byte Spill
	ldr	r0, [sp, #48]                   @ 4-byte Reload
	adcs	r0, r0, r6
	str	r0, [sp, #52]                   @ 4-byte Spill
	ldr	r0, [sp, #44]                   @ 4-byte Reload
	adcs	r0, r0, r8
	str	r0, [sp, #48]                   @ 4-byte Spill
	ldr	r0, [sp, #40]                   @ 4-byte Reload
	mov	r8, r7
	adcs	r0, r0, r9
	str	r0, [sp, #44]                   @ 4-byte Spill
	ldr	r0, [sp, #36]                   @ 4-byte Reload
	adcs	r0, r0, r10
	str	r0, [sp, #40]                   @ 4-byte Spill
	adc	r0, r11, #0
	str	r0, [sp, #36]                   @ 4-byte Spill
	ldr	r0, [sp, #88]                   @ 4-byte Reload
	mul	r2, r0, r7
	add	r0, lr, #88
	bl	mulPv384x32
	ldr	r0, [sp, #92]                   @ 4-byte Reload
	add	r10, sp, #1120
	ldr	r1, [sp, #96]                   @ 4-byte Reload
	ldr	r2, [r0, #12]
	ldr	r0, [sp, #1160]
	str	r0, [sp, #32]                   @ 4-byte Spill
	ldr	r0, [sp, #1156]
	str	r0, [sp, #28]                   @ 4-byte Spill
	ldr	r0, [sp, #1152]
	str	r0, [sp, #24]                   @ 4-byte Spill
	ldr	r0, [sp, #1148]
	str	r0, [sp, #20]                   @ 4-byte Spill
	ldr	r0, [sp, #1144]
	str	r0, [sp, #16]                   @ 4-byte Spill
	ldr	r0, [sp, #1140]
	str	r0, [sp, #12]                   @ 4-byte Spill
	add	r0, sp, #1056
	ldm	r10, {r4, r5, r10}
	ldr	r6, [sp, #1136]
	ldr	r9, [sp, #1132]
	ldr	r11, [sp, #1112]
	ldr	r7, [sp, #1116]
	bl	mulPv384x32
	adds	r0, r8, r11
	ldr	r1, [sp, #12]                   @ 4-byte Reload
	ldr	r0, [sp, #80]                   @ 4-byte Reload
	adcs	r0, r0, r7
	str	r0, [sp, #80]                   @ 4-byte Spill
	ldr	r0, [sp, #76]                   @ 4-byte Reload
	adcs	r0, r0, r4
	str	r0, [sp, #76]                   @ 4-byte Spill
	ldr	r0, [sp, #72]                   @ 4-byte Reload
	adcs	r0, r0, r5
	str	r0, [sp, #72]                   @ 4-byte Spill
	ldr	r0, [sp, #68]                   @ 4-byte Reload
	adcs	r0, r0, r10
	str	r0, [sp, #68]                   @ 4-byte Spill
	ldr	r0, [sp, #64]                   @ 4-byte Reload
	ldr	r10, [sp, #80]                  @ 4-byte Reload
	adcs	r0, r0, r9
	str	r0, [sp, #64]                   @ 4-byte Spill
	ldr	r0, [sp, #60]                   @ 4-byte Reload
	adcs	r0, r0, r6
	str	r0, [sp, #60]                   @ 4-byte Spill
	ldr	r0, [sp, #56]                   @ 4-byte Reload
	adcs	r0, r0, r1
	str	r0, [sp, #56]                   @ 4-byte Spill
	ldr	r0, [sp, #52]                   @ 4-byte Reload
	ldr	r1, [sp, #16]                   @ 4-byte Reload
	adcs	r0, r0, r1
	str	r0, [sp, #52]                   @ 4-byte Spill
	ldr	r0, [sp, #48]                   @ 4-byte Reload
	ldr	r1, [sp, #20]                   @ 4-byte Reload
	adcs	r0, r0, r1
	str	r0, [sp, #48]                   @ 4-byte Spill
	ldr	r0, [sp, #44]                   @ 4-byte Reload
	ldr	r1, [sp, #24]                   @ 4-byte Reload
	adcs	r0, r0, r1
	str	r0, [sp, #44]                   @ 4-byte Spill
	ldr	r0, [sp, #40]                   @ 4-byte Reload
	ldr	r1, [sp, #28]                   @ 4-byte Reload
	adcs	r0, r0, r1
	str	r0, [sp, #40]                   @ 4-byte Spill
	ldr	r0, [sp, #36]                   @ 4-byte Reload
	ldr	r1, [sp, #32]                   @ 4-byte Reload
	adc	r0, r0, r1
	str	r0, [sp, #36]                   @ 4-byte Spill
	ldr	r4, [sp, #1056]
	ldr	r0, [sp, #1060]
	adds	r10, r10, r4
	ldr	r4, [sp, #76]                   @ 4-byte Reload
	ldr	r11, [sp, #1104]
	adcs	r0, r4, r0
	ldr	r9, [sp, #1100]
	ldr	r8, [sp, #1096]
	mov	r4, r10
	ldr	r7, [sp, #1092]
	ldr	r6, [sp, #1088]
	ldr	r5, [sp, #1084]
	ldr	lr, [sp, #1080]
	ldr	r12, [sp, #1076]
	ldr	r3, [sp, #1072]
	ldr	r1, [sp, #1064]
	ldr	r2, [sp, #1068]
	str	r0, [sp, #80]                   @ 4-byte Spill
	ldr	r0, [sp, #72]                   @ 4-byte Reload
	adcs	r0, r0, r1
	str	r0, [sp, #76]                   @ 4-byte Spill
	ldr	r0, [sp, #68]                   @ 4-byte Reload
	ldr	r1, [sp, #100]                  @ 4-byte Reload
	adcs	r0, r0, r2
	str	r0, [sp, #72]                   @ 4-byte Spill
	ldr	r0, [sp, #64]                   @ 4-byte Reload
	adcs	r0, r0, r3
	str	r0, [sp, #68]                   @ 4-byte Spill
	ldr	r0, [sp, #60]                   @ 4-byte Reload
	adcs	r0, r0, r12
	str	r0, [sp, #64]                   @ 4-byte Spill
	ldr	r0, [sp, #56]                   @ 4-byte Reload
	adcs	r0, r0, lr
	str	r0, [sp, #60]                   @ 4-byte Spill
	ldr	r0, [sp, #52]                   @ 4-byte Reload
	adcs	r0, r0, r5
	str	r0, [sp, #56]                   @ 4-byte Spill
	ldr	r0, [sp, #48]                   @ 4-byte Reload
	adcs	r0, r0, r6
	str	r0, [sp, #52]                   @ 4-byte Spill
	ldr	r0, [sp, #44]                   @ 4-byte Reload
	adcs	r0, r0, r7
	str	r0, [sp, #48]                   @ 4-byte Spill
	ldr	r0, [sp, #40]                   @ 4-byte Reload
	adcs	r0, r0, r8
	str	r0, [sp, #44]                   @ 4-byte Spill
	ldr	r0, [sp, #36]                   @ 4-byte Reload
	adcs	r0, r0, r9
	str	r0, [sp, #40]                   @ 4-byte Spill
	adc	r0, r11, #0
	str	r0, [sp, #36]                   @ 4-byte Spill
	ldr	r0, [sp, #88]                   @ 4-byte Reload
	mul	r2, r0, r10
	add	r0, sp, #1000
	bl	mulPv384x32
	ldr	r0, [sp, #92]                   @ 4-byte Reload
	add	r9, sp, #1008
	ldr	r1, [sp, #96]                   @ 4-byte Reload
	ldr	r2, [r0, #16]
	ldr	r0, [sp, #1048]
	str	r0, [sp, #32]                   @ 4-byte Spill
	ldr	r0, [sp, #1044]
	str	r0, [sp, #28]                   @ 4-byte Spill
	ldr	r0, [sp, #1040]
	str	r0, [sp, #24]                   @ 4-byte Spill
	ldr	r0, [sp, #1036]
	str	r0, [sp, #20]                   @ 4-byte Spill
	ldr	r0, [sp, #1032]
	str	r0, [sp, #16]                   @ 4-byte Spill
	ldr	r0, [sp, #1028]
	str	r0, [sp, #12]                   @ 4-byte Spill
	add	r0, sp, #944
	ldm	r9, {r6, r7, r9}
	ldr	r5, [sp, #1024]
	ldr	r8, [sp, #1020]
	ldr	r10, [sp, #1000]
	ldr	r11, [sp, #1004]
	bl	mulPv384x32
	adds	r0, r4, r10
	ldr	r1, [sp, #12]                   @ 4-byte Reload
	ldr	r0, [sp, #80]                   @ 4-byte Reload
	add	lr, sp, #944
	adcs	r0, r0, r11
	str	r0, [sp, #80]                   @ 4-byte Spill
	ldr	r0, [sp, #76]                   @ 4-byte Reload
	add	r11, sp, #968
	adcs	r0, r0, r6
	str	r0, [sp, #76]                   @ 4-byte Spill
	ldr	r0, [sp, #72]                   @ 4-byte Reload
	adcs	r0, r0, r7
	str	r0, [sp, #72]                   @ 4-byte Spill
	ldr	r0, [sp, #68]                   @ 4-byte Reload
	ldr	r7, [sp, #80]                   @ 4-byte Reload
	adcs	r0, r0, r9
	str	r0, [sp, #68]                   @ 4-byte Spill
	ldr	r0, [sp, #64]                   @ 4-byte Reload
	adcs	r0, r0, r8
	str	r0, [sp, #64]                   @ 4-byte Spill
	ldr	r0, [sp, #60]                   @ 4-byte Reload
	adcs	r0, r0, r5
	str	r0, [sp, #60]                   @ 4-byte Spill
	ldr	r0, [sp, #56]                   @ 4-byte Reload
	adcs	r0, r0, r1
	str	r0, [sp, #56]                   @ 4-byte Spill
	ldr	r0, [sp, #52]                   @ 4-byte Reload
	ldr	r1, [sp, #16]                   @ 4-byte Reload
	adcs	r0, r0, r1
	str	r0, [sp, #52]                   @ 4-byte Spill
	ldr	r0, [sp, #48]                   @ 4-byte Reload
	ldr	r1, [sp, #20]                   @ 4-byte Reload
	adcs	r0, r0, r1
	str	r0, [sp, #48]                   @ 4-byte Spill
	ldr	r0, [sp, #44]                   @ 4-byte Reload
	ldr	r1, [sp, #24]                   @ 4-byte Reload
	adcs	r0, r0, r1
	str	r0, [sp, #44]                   @ 4-byte Spill
	ldr	r0, [sp, #40]                   @ 4-byte Reload
	ldr	r1, [sp, #28]                   @ 4-byte Reload
	adcs	r0, r0, r1
	str	r0, [sp, #40]                   @ 4-byte Spill
	ldr	r0, [sp, #36]                   @ 4-byte Reload
	ldr	r1, [sp, #32]                   @ 4-byte Reload
	adc	r0, r0, r1
	str	r0, [sp, #36]                   @ 4-byte Spill
	ldm	lr, {r0, r1, r2, r3, r12, lr}
	adds	r7, r7, r0
	ldr	r0, [sp, #76]                   @ 4-byte Reload
	ldm	r11, {r4, r5, r6, r8, r9, r10, r11}
	adcs	r0, r0, r1
	str	r0, [sp, #80]                   @ 4-byte Spill
	ldr	r0, [sp, #72]                   @ 4-byte Reload
	ldr	r1, [sp, #100]                  @ 4-byte Reload
	adcs	r0, r0, r2
	str	r0, [sp, #76]                   @ 4-byte Spill
	ldr	r0, [sp, #68]                   @ 4-byte Reload
	adcs	r0, r0, r3
	str	r0, [sp, #72]                   @ 4-byte Spill
	ldr	r0, [sp, #64]                   @ 4-byte Reload
	adcs	r0, r0, r12
	str	r0, [sp, #68]                   @ 4-byte Spill
	ldr	r0, [sp, #60]                   @ 4-byte Reload
	adcs	r0, r0, lr
	str	r0, [sp, #64]                   @ 4-byte Spill
	ldr	r0, [sp, #56]                   @ 4-byte Reload
	adcs	r0, r0, r4
	str	r0, [sp, #60]                   @ 4-byte Spill
	ldr	r0, [sp, #52]                   @ 4-byte Reload
	adcs	r0, r0, r5
	str	r0, [sp, #56]                   @ 4-byte Spill
	ldr	r0, [sp, #48]                   @ 4-byte Reload
	adcs	r0, r0, r6
	str	r0, [sp, #52]                   @ 4-byte Spill
	ldr	r0, [sp, #44]                   @ 4-byte Reload
	adcs	r0, r0, r8
	str	r0, [sp, #48]                   @ 4-byte Spill
	ldr	r0, [sp, #40]                   @ 4-byte Reload
	mov	r8, r7
	adcs	r0, r0, r9
	str	r0, [sp, #44]                   @ 4-byte Spill
	ldr	r0, [sp, #36]                   @ 4-byte Reload
	adcs	r0, r0, r10
	str	r0, [sp, #40]                   @ 4-byte Spill
	adc	r0, r11, #0
	str	r0, [sp, #36]                   @ 4-byte Spill
	ldr	r0, [sp, #88]                   @ 4-byte Reload
	mul	r2, r0, r7
	add	r0, sp, #888
	bl	mulPv384x32
	ldr	r0, [sp, #92]                   @ 4-byte Reload
	add	r10, sp, #896
	ldr	r1, [sp, #96]                   @ 4-byte Reload
	ldr	r2, [r0, #20]
	ldr	r0, [sp, #936]
	str	r0, [sp, #32]                   @ 4-byte Spill
	ldr	r0, [sp, #932]
	str	r0, [sp, #28]                   @ 4-byte Spill
	ldr	r0, [sp, #928]
	str	r0, [sp, #24]                   @ 4-byte Spill
	ldr	r0, [sp, #924]
	str	r0, [sp, #20]                   @ 4-byte Spill
	ldr	r0, [sp, #920]
	str	r0, [sp, #16]                   @ 4-byte Spill
	ldr	r0, [sp, #916]
	str	r0, [sp, #12]                   @ 4-byte Spill
	add	r0, sp, #832
	ldm	r10, {r4, r5, r10}
	ldr	r6, [sp, #912]
	ldr	r9, [sp, #908]
	ldr	r11, [sp, #888]
	ldr	r7, [sp, #892]
	bl	mulPv384x32
	adds	r0, r8, r11
	ldr	r1, [sp, #12]                   @ 4-byte Reload
	ldr	r0, [sp, #80]                   @ 4-byte Reload
	add	lr, sp, #836
	add	r11, sp, #860
	adcs	r0, r0, r7
	str	r0, [sp, #80]                   @ 4-byte Spill
	ldr	r0, [sp, #76]                   @ 4-byte Reload
	adcs	r0, r0, r4
	str	r0, [sp, #76]                   @ 4-byte Spill
	ldr	r0, [sp, #72]                   @ 4-byte Reload
	adcs	r0, r0, r5
	str	r0, [sp, #72]                   @ 4-byte Spill
	ldr	r0, [sp, #68]                   @ 4-byte Reload
	adcs	r0, r0, r10
	str	r0, [sp, #68]                   @ 4-byte Spill
	ldr	r0, [sp, #64]                   @ 4-byte Reload
	ldr	r10, [sp, #80]                  @ 4-byte Reload
	adcs	r0, r0, r9
	str	r0, [sp, #64]                   @ 4-byte Spill
	ldr	r0, [sp, #60]                   @ 4-byte Reload
	adcs	r0, r0, r6
	str	r0, [sp, #60]                   @ 4-byte Spill
	ldr	r0, [sp, #56]                   @ 4-byte Reload
	adcs	r0, r0, r1
	str	r0, [sp, #56]                   @ 4-byte Spill
	ldr	r0, [sp, #52]                   @ 4-byte Reload
	ldr	r1, [sp, #16]                   @ 4-byte Reload
	adcs	r0, r0, r1
	str	r0, [sp, #52]                   @ 4-byte Spill
	ldr	r0, [sp, #48]                   @ 4-byte Reload
	ldr	r1, [sp, #20]                   @ 4-byte Reload
	adcs	r0, r0, r1
	str	r0, [sp, #48]                   @ 4-byte Spill
	ldr	r0, [sp, #44]                   @ 4-byte Reload
	ldr	r1, [sp, #24]                   @ 4-byte Reload
	adcs	r0, r0, r1
	str	r0, [sp, #44]                   @ 4-byte Spill
	ldr	r0, [sp, #40]                   @ 4-byte Reload
	ldr	r1, [sp, #28]                   @ 4-byte Reload
	adcs	r0, r0, r1
	str	r0, [sp, #40]                   @ 4-byte Spill
	ldr	r0, [sp, #36]                   @ 4-byte Reload
	ldr	r1, [sp, #32]                   @ 4-byte Reload
	adc	r0, r0, r1
	str	r0, [sp, #36]                   @ 4-byte Spill
	ldr	r4, [sp, #832]
	ldm	lr, {r0, r1, r2, r3, r12, lr}
	adds	r10, r10, r4
	ldr	r4, [sp, #76]                   @ 4-byte Reload
	ldm	r11, {r5, r6, r7, r8, r9, r11}
	adcs	r0, r4, r0
	str	r0, [sp, #80]                   @ 4-byte Spill
	mov	r4, r10
	ldr	r0, [sp, #72]                   @ 4-byte Reload
	adcs	r0, r0, r1
	str	r0, [sp, #76]                   @ 4-byte Spill
	ldr	r0, [sp, #68]                   @ 4-byte Reload
	ldr	r1, [sp, #100]                  @ 4-byte Reload
	adcs	r0, r0, r2
	str	r0, [sp, #72]                   @ 4-byte Spill
	ldr	r0, [sp, #64]                   @ 4-byte Reload
	adcs	r0, r0, r3
	str	r0, [sp, #68]                   @ 4-byte Spill
	ldr	r0, [sp, #60]                   @ 4-byte Reload
	adcs	r0, r0, r12
	str	r0, [sp, #64]                   @ 4-byte Spill
	ldr	r0, [sp, #56]                   @ 4-byte Reload
	adcs	r0, r0, lr
	str	r0, [sp, #60]                   @ 4-byte Spill
	ldr	r0, [sp, #52]                   @ 4-byte Reload
	adcs	r0, r0, r5
	str	r0, [sp, #56]                   @ 4-byte Spill
	ldr	r0, [sp, #48]                   @ 4-byte Reload
	adcs	r0, r0, r6
	str	r0, [sp, #52]                   @ 4-byte Spill
	ldr	r0, [sp, #44]                   @ 4-byte Reload
	adcs	r0, r0, r7
	str	r0, [sp, #48]                   @ 4-byte Spill
	ldr	r0, [sp, #40]                   @ 4-byte Reload
	adcs	r0, r0, r8
	str	r0, [sp, #44]                   @ 4-byte Spill
	ldr	r0, [sp, #36]                   @ 4-byte Reload
	adcs	r0, r0, r9
	str	r0, [sp, #40]                   @ 4-byte Spill
	adc	r0, r11, #0
	str	r0, [sp, #36]                   @ 4-byte Spill
	ldr	r0, [sp, #88]                   @ 4-byte Reload
	mul	r2, r0, r10
	add	r0, sp, #776
	bl	mulPv384x32
	ldr	r0, [sp, #92]                   @ 4-byte Reload
	add	r9, sp, #784
	ldr	r1, [sp, #96]                   @ 4-byte Reload
	ldr	r2, [r0, #24]
	ldr	r0, [sp, #824]
	str	r0, [sp, #32]                   @ 4-byte Spill
	ldr	r0, [sp, #820]
	str	r0, [sp, #28]                   @ 4-byte Spill
	ldr	r0, [sp, #816]
	str	r0, [sp, #24]                   @ 4-byte Spill
	ldr	r0, [sp, #812]
	str	r0, [sp, #20]                   @ 4-byte Spill
	ldr	r0, [sp, #808]
	str	r0, [sp, #16]                   @ 4-byte Spill
	ldr	r0, [sp, #804]
	str	r0, [sp, #12]                   @ 4-byte Spill
	add	r0, sp, #720
	ldm	r9, {r6, r7, r9}
	ldr	r5, [sp, #800]
	ldr	r8, [sp, #796]
	ldr	r10, [sp, #776]
	ldr	r11, [sp, #780]
	bl	mulPv384x32
	adds	r0, r4, r10
	ldr	r1, [sp, #12]                   @ 4-byte Reload
	ldr	r0, [sp, #80]                   @ 4-byte Reload
	add	lr, sp, #720
	adcs	r0, r0, r11
	str	r0, [sp, #80]                   @ 4-byte Spill
	ldr	r0, [sp, #76]                   @ 4-byte Reload
	add	r11, sp, #744
	adcs	r0, r0, r6
	str	r0, [sp, #76]                   @ 4-byte Spill
	ldr	r0, [sp, #72]                   @ 4-byte Reload
	adcs	r0, r0, r7
	str	r0, [sp, #72]                   @ 4-byte Spill
	ldr	r0, [sp, #68]                   @ 4-byte Reload
	ldr	r7, [sp, #80]                   @ 4-byte Reload
	adcs	r0, r0, r9
	str	r0, [sp, #68]                   @ 4-byte Spill
	ldr	r0, [sp, #64]                   @ 4-byte Reload
	adcs	r0, r0, r8
	str	r0, [sp, #64]                   @ 4-byte Spill
	ldr	r0, [sp, #60]                   @ 4-byte Reload
	adcs	r0, r0, r5
	str	r0, [sp, #60]                   @ 4-byte Spill
	ldr	r0, [sp, #56]                   @ 4-byte Reload
	adcs	r0, r0, r1
	str	r0, [sp, #56]                   @ 4-byte Spill
	ldr	r0, [sp, #52]                   @ 4-byte Reload
	ldr	r1, [sp, #16]                   @ 4-byte Reload
	adcs	r0, r0, r1
	str	r0, [sp, #52]                   @ 4-byte Spill
	ldr	r0, [sp, #48]                   @ 4-byte Reload
	ldr	r1, [sp, #20]                   @ 4-byte Reload
	adcs	r0, r0, r1
	str	r0, [sp, #48]                   @ 4-byte Spill
	ldr	r0, [sp, #44]                   @ 4-byte Reload
	ldr	r1, [sp, #24]                   @ 4-byte Reload
	adcs	r0, r0, r1
	str	r0, [sp, #44]                   @ 4-byte Spill
	ldr	r0, [sp, #40]                   @ 4-byte Reload
	ldr	r1, [sp, #28]                   @ 4-byte Reload
	adcs	r0, r0, r1
	str	r0, [sp, #40]                   @ 4-byte Spill
	ldr	r0, [sp, #36]                   @ 4-byte Reload
	ldr	r1, [sp, #32]                   @ 4-byte Reload
	adc	r0, r0, r1
	str	r0, [sp, #36]                   @ 4-byte Spill
	ldm	lr, {r0, r1, r2, r3, r12, lr}
	adds	r7, r7, r0
	ldr	r0, [sp, #76]                   @ 4-byte Reload
	ldm	r11, {r4, r5, r6, r8, r9, r10, r11}
	adcs	r0, r0, r1
	str	r0, [sp, #80]                   @ 4-byte Spill
	ldr	r0, [sp, #72]                   @ 4-byte Reload
	ldr	r1, [sp, #100]                  @ 4-byte Reload
	adcs	r0, r0, r2
	str	r0, [sp, #76]                   @ 4-byte Spill
	ldr	r0, [sp, #68]                   @ 4-byte Reload
	adcs	r0, r0, r3
	str	r0, [sp, #72]                   @ 4-byte Spill
	ldr	r0, [sp, #64]                   @ 4-byte Reload
	adcs	r0, r0, r12
	str	r0, [sp, #68]                   @ 4-byte Spill
	ldr	r0, [sp, #60]                   @ 4-byte Reload
	adcs	r0, r0, lr
	str	r0, [sp, #64]                   @ 4-byte Spill
	ldr	r0, [sp, #56]                   @ 4-byte Reload
	adcs	r0, r0, r4
	str	r0, [sp, #60]                   @ 4-byte Spill
	ldr	r0, [sp, #52]                   @ 4-byte Reload
	adcs	r0, r0, r5
	str	r0, [sp, #56]                   @ 4-byte Spill
	ldr	r0, [sp, #48]                   @ 4-byte Reload
	adcs	r0, r0, r6
	str	r0, [sp, #52]                   @ 4-byte Spill
	ldr	r0, [sp, #44]                   @ 4-byte Reload
	adcs	r0, r0, r8
	str	r0, [sp, #48]                   @ 4-byte Spill
	ldr	r0, [sp, #40]                   @ 4-byte Reload
	mov	r8, r7
	adcs	r0, r0, r9
	str	r0, [sp, #44]                   @ 4-byte Spill
	ldr	r0, [sp, #36]                   @ 4-byte Reload
	adcs	r0, r0, r10
	str	r0, [sp, #40]                   @ 4-byte Spill
	adc	r0, r11, #0
	str	r0, [sp, #36]                   @ 4-byte Spill
	ldr	r0, [sp, #88]                   @ 4-byte Reload
	mul	r2, r0, r7
	add	r0, sp, #664
	bl	mulPv384x32
	ldr	r0, [sp, #92]                   @ 4-byte Reload
	add	r10, sp, #672
	ldr	r1, [sp, #96]                   @ 4-byte Reload
	ldr	r2, [r0, #28]
	ldr	r0, [sp, #712]
	str	r0, [sp, #32]                   @ 4-byte Spill
	ldr	r0, [sp, #708]
	str	r0, [sp, #28]                   @ 4-byte Spill
	ldr	r0, [sp, #704]
	str	r0, [sp, #24]                   @ 4-byte Spill
	ldr	r0, [sp, #700]
	str	r0, [sp, #20]                   @ 4-byte Spill
	ldr	r0, [sp, #696]
	str	r0, [sp, #16]                   @ 4-byte Spill
	ldr	r0, [sp, #692]
	str	r0, [sp, #12]                   @ 4-byte Spill
	add	r0, sp, #608
	ldm	r10, {r4, r5, r10}
	ldr	r6, [sp, #688]
	ldr	r9, [sp, #684]
	ldr	r11, [sp, #664]
	ldr	r7, [sp, #668]
	bl	mulPv384x32
	adds	r0, r8, r11
	ldr	r1, [sp, #12]                   @ 4-byte Reload
	ldr	r0, [sp, #80]                   @ 4-byte Reload
	add	lr, sp, #612
	add	r11, sp, #636
	adcs	r0, r0, r7
	str	r0, [sp, #80]                   @ 4-byte Spill
	ldr	r0, [sp, #76]                   @ 4-byte Reload
	adcs	r0, r0, r4
	str	r0, [sp, #76]                   @ 4-byte Spill
	ldr	r0, [sp, #72]                   @ 4-byte Reload
	adcs	r0, r0, r5
	str	r0, [sp, #72]                   @ 4-byte Spill
	ldr	r0, [sp, #68]                   @ 4-byte Reload
	adcs	r0, r0, r10
	str	r0, [sp, #68]                   @ 4-byte Spill
	ldr	r0, [sp, #64]                   @ 4-byte Reload
	ldr	r10, [sp, #80]                  @ 4-byte Reload
	adcs	r0, r0, r9
	str	r0, [sp, #64]                   @ 4-byte Spill
	ldr	r0, [sp, #60]                   @ 4-byte Reload
	adcs	r0, r0, r6
	str	r0, [sp, #60]                   @ 4-byte Spill
	ldr	r0, [sp, #56]                   @ 4-byte Reload
	adcs	r0, r0, r1
	str	r0, [sp, #56]                   @ 4-byte Spill
	ldr	r0, [sp, #52]                   @ 4-byte Reload
	ldr	r1, [sp, #16]                   @ 4-byte Reload
	adcs	r0, r0, r1
	str	r0, [sp, #52]                   @ 4-byte Spill
	ldr	r0, [sp, #48]                   @ 4-byte Reload
	ldr	r1, [sp, #20]                   @ 4-byte Reload
	adcs	r0, r0, r1
	str	r0, [sp, #48]                   @ 4-byte Spill
	ldr	r0, [sp, #44]                   @ 4-byte Reload
	ldr	r1, [sp, #24]                   @ 4-byte Reload
	adcs	r0, r0, r1
	str	r0, [sp, #44]                   @ 4-byte Spill
	ldr	r0, [sp, #40]                   @ 4-byte Reload
	ldr	r1, [sp, #28]                   @ 4-byte Reload
	adcs	r0, r0, r1
	str	r0, [sp, #40]                   @ 4-byte Spill
	ldr	r0, [sp, #36]                   @ 4-byte Reload
	ldr	r1, [sp, #32]                   @ 4-byte Reload
	adc	r0, r0, r1
	str	r0, [sp, #36]                   @ 4-byte Spill
	ldr	r4, [sp, #608]
	ldm	lr, {r0, r1, r2, r3, r12, lr}
	adds	r10, r10, r4
	ldr	r4, [sp, #76]                   @ 4-byte Reload
	ldm	r11, {r5, r6, r7, r8, r9, r11}
	adcs	r0, r4, r0
	str	r0, [sp, #80]                   @ 4-byte Spill
	mov	r4, r10
	ldr	r0, [sp, #72]                   @ 4-byte Reload
	adcs	r0, r0, r1
	str	r0, [sp, #76]                   @ 4-byte Spill
	ldr	r0, [sp, #68]                   @ 4-byte Reload
	ldr	r1, [sp, #100]                  @ 4-byte Reload
	adcs	r0, r0, r2
	str	r0, [sp, #72]                   @ 4-byte Spill
	ldr	r0, [sp, #64]                   @ 4-byte Reload
	adcs	r0, r0, r3
	str	r0, [sp, #68]                   @ 4-byte Spill
	ldr	r0, [sp, #60]                   @ 4-byte Reload
	adcs	r0, r0, r12
	str	r0, [sp, #64]                   @ 4-byte Spill
	ldr	r0, [sp, #56]                   @ 4-byte Reload
	adcs	r0, r0, lr
	str	r0, [sp, #60]                   @ 4-byte Spill
	ldr	r0, [sp, #52]                   @ 4-byte Reload
	adcs	r0, r0, r5
	str	r0, [sp, #56]                   @ 4-byte Spill
	ldr	r0, [sp, #48]                   @ 4-byte Reload
	adcs	r0, r0, r6
	str	r0, [sp, #52]                   @ 4-byte Spill
	ldr	r0, [sp, #44]                   @ 4-byte Reload
	adcs	r0, r0, r7
	str	r0, [sp, #48]                   @ 4-byte Spill
	ldr	r0, [sp, #40]                   @ 4-byte Reload
	adcs	r0, r0, r8
	str	r0, [sp, #44]                   @ 4-byte Spill
	ldr	r0, [sp, #36]                   @ 4-byte Reload
	adcs	r0, r0, r9
	str	r0, [sp, #40]                   @ 4-byte Spill
	adc	r0, r11, #0
	str	r0, [sp, #36]                   @ 4-byte Spill
	ldr	r0, [sp, #88]                   @ 4-byte Reload
	mul	r2, r0, r10
	add	r0, sp, #552
	bl	mulPv384x32
	ldr	r0, [sp, #92]                   @ 4-byte Reload
	add	r9, sp, #560
	ldr	r1, [sp, #96]                   @ 4-byte Reload
	ldr	r2, [r0, #32]
	ldr	r0, [sp, #600]
	str	r0, [sp, #32]                   @ 4-byte Spill
	ldr	r0, [sp, #596]
	str	r0, [sp, #28]                   @ 4-byte Spill
	ldr	r0, [sp, #592]
	str	r0, [sp, #24]                   @ 4-byte Spill
	ldr	r0, [sp, #588]
	str	r0, [sp, #20]                   @ 4-byte Spill
	ldr	r0, [sp, #584]
	str	r0, [sp, #16]                   @ 4-byte Spill
	ldr	r0, [sp, #580]
	str	r0, [sp, #12]                   @ 4-byte Spill
	add	r0, sp, #496
	ldm	r9, {r6, r7, r9}
	ldr	r5, [sp, #576]
	ldr	r8, [sp, #572]
	ldr	r10, [sp, #552]
	ldr	r11, [sp, #556]
	bl	mulPv384x32
	adds	r0, r4, r10
	ldr	r1, [sp, #12]                   @ 4-byte Reload
	ldr	r0, [sp, #80]                   @ 4-byte Reload
	add	lr, sp, #496
	adcs	r0, r0, r11
	str	r0, [sp, #80]                   @ 4-byte Spill
	ldr	r0, [sp, #76]                   @ 4-byte Reload
	add	r11, sp, #520
	adcs	r0, r0, r6
	str	r0, [sp, #76]                   @ 4-byte Spill
	ldr	r0, [sp, #72]                   @ 4-byte Reload
	adcs	r0, r0, r7
	str	r0, [sp, #72]                   @ 4-byte Spill
	ldr	r0, [sp, #68]                   @ 4-byte Reload
	ldr	r7, [sp, #80]                   @ 4-byte Reload
	adcs	r0, r0, r9
	str	r0, [sp, #68]                   @ 4-byte Spill
	ldr	r0, [sp, #64]                   @ 4-byte Reload
	adcs	r0, r0, r8
	str	r0, [sp, #64]                   @ 4-byte Spill
	ldr	r0, [sp, #60]                   @ 4-byte Reload
	adcs	r0, r0, r5
	str	r0, [sp, #60]                   @ 4-byte Spill
	ldr	r0, [sp, #56]                   @ 4-byte Reload
	adcs	r0, r0, r1
	str	r0, [sp, #56]                   @ 4-byte Spill
	ldr	r0, [sp, #52]                   @ 4-byte Reload
	ldr	r1, [sp, #16]                   @ 4-byte Reload
	adcs	r0, r0, r1
	str	r0, [sp, #52]                   @ 4-byte Spill
	ldr	r0, [sp, #48]                   @ 4-byte Reload
	ldr	r1, [sp, #20]                   @ 4-byte Reload
	adcs	r0, r0, r1
	str	r0, [sp, #48]                   @ 4-byte Spill
	ldr	r0, [sp, #44]                   @ 4-byte Reload
	ldr	r1, [sp, #24]                   @ 4-byte Reload
	adcs	r0, r0, r1
	str	r0, [sp, #44]                   @ 4-byte Spill
	ldr	r0, [sp, #40]                   @ 4-byte Reload
	ldr	r1, [sp, #28]                   @ 4-byte Reload
	adcs	r0, r0, r1
	str	r0, [sp, #40]                   @ 4-byte Spill
	ldr	r0, [sp, #36]                   @ 4-byte Reload
	ldr	r1, [sp, #32]                   @ 4-byte Reload
	adc	r0, r0, r1
	str	r0, [sp, #36]                   @ 4-byte Spill
	ldm	lr, {r0, r1, r2, r3, r12, lr}
	adds	r7, r7, r0
	ldr	r0, [sp, #76]                   @ 4-byte Reload
	ldm	r11, {r4, r5, r6, r8, r9, r10, r11}
	adcs	r0, r0, r1
	str	r0, [sp, #80]                   @ 4-byte Spill
	ldr	r0, [sp, #72]                   @ 4-byte Reload
	ldr	r1, [sp, #100]                  @ 4-byte Reload
	adcs	r0, r0, r2
	str	r0, [sp, #76]                   @ 4-byte Spill
	ldr	r0, [sp, #68]                   @ 4-byte Reload
	adcs	r0, r0, r3
	str	r0, [sp, #72]                   @ 4-byte Spill
	ldr	r0, [sp, #64]                   @ 4-byte Reload
	adcs	r0, r0, r12
	str	r0, [sp, #68]                   @ 4-byte Spill
	ldr	r0, [sp, #60]                   @ 4-byte Reload
	adcs	r0, r0, lr
	str	r0, [sp, #64]                   @ 4-byte Spill
	ldr	r0, [sp, #56]                   @ 4-byte Reload
	adcs	r0, r0, r4
	str	r0, [sp, #60]                   @ 4-byte Spill
	ldr	r0, [sp, #52]                   @ 4-byte Reload
	adcs	r0, r0, r5
	str	r0, [sp, #56]                   @ 4-byte Spill
	ldr	r0, [sp, #48]                   @ 4-byte Reload
	adcs	r0, r0, r6
	str	r0, [sp, #52]                   @ 4-byte Spill
	ldr	r0, [sp, #44]                   @ 4-byte Reload
	adcs	r0, r0, r8
	str	r0, [sp, #48]                   @ 4-byte Spill
	ldr	r0, [sp, #40]                   @ 4-byte Reload
	mov	r8, r7
	adcs	r0, r0, r9
	str	r0, [sp, #44]                   @ 4-byte Spill
	ldr	r0, [sp, #36]                   @ 4-byte Reload
	adcs	r0, r0, r10
	str	r0, [sp, #40]                   @ 4-byte Spill
	adc	r0, r11, #0
	str	r0, [sp, #36]                   @ 4-byte Spill
	ldr	r0, [sp, #88]                   @ 4-byte Reload
	mul	r2, r0, r7
	add	r0, sp, #440
	bl	mulPv384x32
	ldr	r0, [sp, #92]                   @ 4-byte Reload
	add	r10, sp, #448
	ldr	r1, [sp, #96]                   @ 4-byte Reload
	ldr	r2, [r0, #36]
	ldr	r0, [sp, #488]
	str	r0, [sp, #32]                   @ 4-byte Spill
	ldr	r0, [sp, #484]
	str	r0, [sp, #28]                   @ 4-byte Spill
	ldr	r0, [sp, #480]
	str	r0, [sp, #24]                   @ 4-byte Spill
	ldr	r0, [sp, #476]
	str	r0, [sp, #20]                   @ 4-byte Spill
	ldr	r0, [sp, #472]
	str	r0, [sp, #16]                   @ 4-byte Spill
	ldr	r0, [sp, #468]
	str	r0, [sp, #12]                   @ 4-byte Spill
	add	r0, sp, #384
	ldm	r10, {r4, r5, r10}
	ldr	r6, [sp, #464]
	ldr	r9, [sp, #460]
	ldr	r11, [sp, #440]
	ldr	r7, [sp, #444]
	bl	mulPv384x32
	adds	r0, r8, r11
	ldr	r1, [sp, #12]                   @ 4-byte Reload
	ldr	r0, [sp, #80]                   @ 4-byte Reload
	add	lr, sp, #388
	add	r11, sp, #412
	adcs	r0, r0, r7
	str	r0, [sp, #80]                   @ 4-byte Spill
	ldr	r0, [sp, #76]                   @ 4-byte Reload
	adcs	r0, r0, r4
	str	r0, [sp, #76]                   @ 4-byte Spill
	ldr	r0, [sp, #72]                   @ 4-byte Reload
	ldr	r7, [sp, #76]                   @ 4-byte Reload
	adcs	r0, r0, r5
	str	r0, [sp, #72]                   @ 4-byte Spill
	ldr	r0, [sp, #68]                   @ 4-byte Reload
	adcs	r0, r0, r10
	str	r0, [sp, #68]                   @ 4-byte Spill
	ldr	r0, [sp, #64]                   @ 4-byte Reload
	ldr	r10, [sp, #80]                  @ 4-byte Reload
	adcs	r0, r0, r9
	str	r0, [sp, #64]                   @ 4-byte Spill
	ldr	r0, [sp, #60]                   @ 4-byte Reload
	adcs	r0, r0, r6
	str	r0, [sp, #60]                   @ 4-byte Spill
	ldr	r0, [sp, #56]                   @ 4-byte Reload
	adcs	r0, r0, r1
	str	r0, [sp, #56]                   @ 4-byte Spill
	ldr	r0, [sp, #52]                   @ 4-byte Reload
	ldr	r1, [sp, #16]                   @ 4-byte Reload
	adcs	r0, r0, r1
	str	r0, [sp, #52]                   @ 4-byte Spill
	ldr	r0, [sp, #48]                   @ 4-byte Reload
	ldr	r1, [sp, #20]                   @ 4-byte Reload
	adcs	r0, r0, r1
	str	r0, [sp, #48]                   @ 4-byte Spill
	ldr	r0, [sp, #44]                   @ 4-byte Reload
	ldr	r1, [sp, #24]                   @ 4-byte Reload
	adcs	r0, r0, r1
	str	r0, [sp, #44]                   @ 4-byte Spill
	ldr	r0, [sp, #40]                   @ 4-byte Reload
	ldr	r1, [sp, #28]                   @ 4-byte Reload
	adcs	r0, r0, r1
	str	r0, [sp, #40]                   @ 4-byte Spill
	ldr	r0, [sp, #36]                   @ 4-byte Reload
	ldr	r1, [sp, #32]                   @ 4-byte Reload
	adc	r0, r0, r1
	str	r0, [sp, #36]                   @ 4-byte Spill
	ldr	r0, [sp, #432]
	str	r0, [sp, #32]                   @ 4-byte Spill
	ldr	r4, [sp, #384]
	ldm	lr, {r0, r1, r2, r3, r12, lr}
	adds	r4, r10, r4
	adcs	r0, r7, r0
	ldm	r11, {r5, r6, r8, r9, r11}
	str	r0, [sp, #80]                   @ 4-byte Spill
	ldr	r0, [sp, #72]                   @ 4-byte Reload
	str	r4, [sp, #24]                   @ 4-byte Spill
	adcs	r0, r0, r1
	str	r0, [sp, #76]                   @ 4-byte Spill
	ldr	r0, [sp, #68]                   @ 4-byte Reload
	ldr	r1, [sp, #100]                  @ 4-byte Reload
	adcs	r0, r0, r2
	str	r0, [sp, #72]                   @ 4-byte Spill
	ldr	r0, [sp, #64]                   @ 4-byte Reload
	adcs	r0, r0, r3
	str	r0, [sp, #68]                   @ 4-byte Spill
	ldr	r0, [sp, #60]                   @ 4-byte Reload
	adcs	r0, r0, r12
	str	r0, [sp, #64]                   @ 4-byte Spill
	ldr	r0, [sp, #56]                   @ 4-byte Reload
	adcs	r0, r0, lr
	str	r0, [sp, #60]                   @ 4-byte Spill
	ldr	r0, [sp, #52]                   @ 4-byte Reload
	adcs	r0, r0, r5
	str	r0, [sp, #56]                   @ 4-byte Spill
	ldr	r0, [sp, #48]                   @ 4-byte Reload
	adcs	r0, r0, r6
	str	r0, [sp, #52]                   @ 4-byte Spill
	ldr	r0, [sp, #44]                   @ 4-byte Reload
	adcs	r0, r0, r8
	str	r0, [sp, #48]                   @ 4-byte Spill
	ldr	r0, [sp, #40]                   @ 4-byte Reload
	ldr	r8, [sp, #88]                   @ 4-byte Reload
	adcs	r0, r0, r9
	str	r0, [sp, #44]                   @ 4-byte Spill
	ldr	r0, [sp, #36]                   @ 4-byte Reload
	mul	r2, r8, r4
	adcs	r0, r0, r11
	str	r0, [sp, #40]                   @ 4-byte Spill
	ldr	r0, [sp, #32]                   @ 4-byte Reload
	adc	r0, r0, #0
	str	r0, [sp, #36]                   @ 4-byte Spill
	add	r0, sp, #328
	bl	mulPv384x32
	ldr	r0, [sp, #92]                   @ 4-byte Reload
	add	r9, sp, #336
	ldr	r1, [sp, #96]                   @ 4-byte Reload
	ldr	r2, [r0, #40]
	ldr	r0, [sp, #376]
	str	r0, [sp, #32]                   @ 4-byte Spill
	ldr	r0, [sp, #372]
	str	r0, [sp, #28]                   @ 4-byte Spill
	ldr	r0, [sp, #368]
	str	r0, [sp, #20]                   @ 4-byte Spill
	ldr	r0, [sp, #364]
	str	r0, [sp, #16]                   @ 4-byte Spill
	ldr	r0, [sp, #360]
	str	r0, [sp, #12]                   @ 4-byte Spill
	ldr	r0, [sp, #356]
	str	r0, [sp, #8]                    @ 4-byte Spill
	add	r0, sp, #272
	ldm	r9, {r6, r7, r9}
	ldr	r4, [sp, #352]
	ldr	r5, [sp, #348]
	ldr	r10, [sp, #328]
	ldr	r11, [sp, #332]
	bl	mulPv384x32
	ldr	r0, [sp, #24]                   @ 4-byte Reload
	add	r12, sp, #288
	ldr	r1, [sp, #76]                   @ 4-byte Reload
	adds	r0, r0, r10
	ldr	r2, [sp, #8]                    @ 4-byte Reload
	ldr	r0, [sp, #80]                   @ 4-byte Reload
	adcs	r0, r0, r11
	adcs	r1, r1, r6
	str	r1, [sp, #76]                   @ 4-byte Spill
	ldr	r1, [sp, #72]                   @ 4-byte Reload
	adcs	r1, r1, r7
	str	r1, [sp, #72]                   @ 4-byte Spill
	ldr	r1, [sp, #68]                   @ 4-byte Reload
	ldr	r7, [sp, #76]                   @ 4-byte Reload
	adcs	r1, r1, r9
	str	r1, [sp, #68]                   @ 4-byte Spill
	ldr	r1, [sp, #64]                   @ 4-byte Reload
	adcs	r1, r1, r5
	str	r1, [sp, #64]                   @ 4-byte Spill
	ldr	r1, [sp, #60]                   @ 4-byte Reload
	adcs	r1, r1, r4
	str	r1, [sp, #60]                   @ 4-byte Spill
	ldr	r1, [sp, #56]                   @ 4-byte Reload
	adcs	r1, r1, r2
	str	r1, [sp, #56]                   @ 4-byte Spill
	ldr	r1, [sp, #52]                   @ 4-byte Reload
	ldr	r2, [sp, #12]                   @ 4-byte Reload
	adcs	r1, r1, r2
	str	r1, [sp, #52]                   @ 4-byte Spill
	ldr	r1, [sp, #48]                   @ 4-byte Reload
	ldr	r2, [sp, #16]                   @ 4-byte Reload
	adcs	r1, r1, r2
	str	r1, [sp, #48]                   @ 4-byte Spill
	ldr	r1, [sp, #44]                   @ 4-byte Reload
	ldr	r2, [sp, #20]                   @ 4-byte Reload
	adcs	r1, r1, r2
	str	r1, [sp, #44]                   @ 4-byte Spill
	ldr	r1, [sp, #40]                   @ 4-byte Reload
	ldr	r2, [sp, #28]                   @ 4-byte Reload
	adcs	r1, r1, r2
	str	r1, [sp, #28]                   @ 4-byte Spill
	ldr	r1, [sp, #36]                   @ 4-byte Reload
	ldr	r2, [sp, #32]                   @ 4-byte Reload
	adc	r1, r1, r2
	str	r1, [sp, #24]                   @ 4-byte Spill
	ldr	r2, [sp, #272]
	ldr	lr, [sp, #276]
	adds	r0, r0, r2
	ldr	r5, [sp, #280]
	ldr	r6, [sp, #284]
	adcs	r7, r7, lr
	str	r0, [sp, #80]                   @ 4-byte Spill
	mul	r10, r8, r0
	ldr	r9, [sp, #320]
	ldr	r8, [sp, #316]
	ldr	r11, [sp, #312]
	ldr	r4, [sp, #308]
	ldm	r12, {r0, r1, r2, r3, r12}
	str	r7, [sp, #40]                   @ 4-byte Spill
	ldr	r7, [sp, #72]                   @ 4-byte Reload
	adcs	r7, r7, r5
	str	r7, [sp, #36]                   @ 4-byte Spill
	ldr	r7, [sp, #68]                   @ 4-byte Reload
	adcs	r7, r7, r6
	str	r7, [sp, #32]                   @ 4-byte Spill
	ldr	r7, [sp, #64]                   @ 4-byte Reload
	adcs	r0, r7, r0
	str	r0, [sp, #76]                   @ 4-byte Spill
	ldr	r0, [sp, #60]                   @ 4-byte Reload
	adcs	r0, r0, r1
	str	r0, [sp, #72]                   @ 4-byte Spill
	ldr	r0, [sp, #56]                   @ 4-byte Reload
	ldr	r1, [sp, #100]                  @ 4-byte Reload
	adcs	r0, r0, r2
	str	r0, [sp, #68]                   @ 4-byte Spill
	ldr	r0, [sp, #52]                   @ 4-byte Reload
	mov	r2, r10
	adcs	r0, r0, r3
	str	r0, [sp, #64]                   @ 4-byte Spill
	ldr	r0, [sp, #48]                   @ 4-byte Reload
	adcs	r0, r0, r12
	str	r0, [sp, #60]                   @ 4-byte Spill
	ldr	r0, [sp, #44]                   @ 4-byte Reload
	adcs	r0, r0, r4
	str	r0, [sp, #56]                   @ 4-byte Spill
	ldr	r0, [sp, #28]                   @ 4-byte Reload
	adcs	r0, r0, r11
	str	r0, [sp, #52]                   @ 4-byte Spill
	ldr	r0, [sp, #24]                   @ 4-byte Reload
	adcs	r0, r0, r8
	str	r0, [sp, #48]                   @ 4-byte Spill
	adc	r0, r9, #0
	str	r0, [sp, #44]                   @ 4-byte Spill
	add	r0, sp, #216
	bl	mulPv384x32
	ldr	r1, [sp, #92]                   @ 4-byte Reload
	add	r11, sp, #220
	add	r0, sp, #160
	ldr	r2, [r1, #44]
	ldr	r1, [sp, #264]
	str	r1, [sp, #92]                   @ 4-byte Spill
	ldr	r1, [sp, #260]
	str	r1, [sp, #28]                   @ 4-byte Spill
	ldr	r1, [sp, #256]
	str	r1, [sp, #24]                   @ 4-byte Spill
	ldr	r1, [sp, #252]
	str	r1, [sp, #20]                   @ 4-byte Spill
	ldr	r1, [sp, #248]
	str	r1, [sp, #16]                   @ 4-byte Spill
	ldr	r1, [sp, #96]                   @ 4-byte Reload
	ldm	r11, {r4, r5, r11}
	ldr	r6, [sp, #244]
	ldr	r7, [sp, #240]
	ldr	r8, [sp, #236]
	ldr	r9, [sp, #232]
	ldr	r10, [sp, #216]
	bl	mulPv384x32
	ldr	r0, [sp, #80]                   @ 4-byte Reload
	add	lr, sp, #160
	ldr	r1, [sp, #16]                   @ 4-byte Reload
	add	r12, sp, #176
	adds	r0, r0, r10
	add	r10, sp, #200
	ldr	r0, [sp, #40]                   @ 4-byte Reload
	adcs	r4, r0, r4
	ldr	r0, [sp, #36]                   @ 4-byte Reload
	adcs	r0, r0, r5
	str	r0, [sp, #96]                   @ 4-byte Spill
	ldr	r0, [sp, #32]                   @ 4-byte Reload
	ldr	r5, [sp, #96]                   @ 4-byte Reload
	adcs	r0, r0, r11
	str	r0, [sp, #80]                   @ 4-byte Spill
	ldr	r0, [sp, #76]                   @ 4-byte Reload
	adcs	r0, r0, r9
	str	r0, [sp, #76]                   @ 4-byte Spill
	ldr	r0, [sp, #72]                   @ 4-byte Reload
	adcs	r0, r0, r8
	str	r0, [sp, #72]                   @ 4-byte Spill
	ldr	r0, [sp, #68]                   @ 4-byte Reload
	adcs	r0, r0, r7
	str	r0, [sp, #68]                   @ 4-byte Spill
	ldr	r0, [sp, #64]                   @ 4-byte Reload
	adcs	r0, r0, r6
	str	r0, [sp, #64]                   @ 4-byte Spill
	ldr	r0, [sp, #60]                   @ 4-byte Reload
	adcs	r0, r0, r1
	str	r0, [sp, #60]                   @ 4-byte Spill
	ldr	r0, [sp, #56]                   @ 4-byte Reload
	ldr	r1, [sp, #20]                   @ 4-byte Reload
	adcs	r0, r0, r1
	str	r0, [sp, #56]                   @ 4-byte Spill
	ldr	r0, [sp, #52]                   @ 4-byte Reload
	ldr	r1, [sp, #24]                   @ 4-byte Reload
	adcs	r0, r0, r1
	str	r0, [sp, #52]                   @ 4-byte Spill
	ldr	r0, [sp, #48]                   @ 4-byte Reload
	ldr	r1, [sp, #28]                   @ 4-byte Reload
	adcs	r0, r0, r1
	str	r0, [sp, #48]                   @ 4-byte Spill
	ldr	r0, [sp, #44]                   @ 4-byte Reload
	ldr	r1, [sp, #92]                   @ 4-byte Reload
	adc	r0, r0, r1
	str	r0, [sp, #92]                   @ 4-byte Spill
	ldm	lr, {r2, r7, lr}
	ldr	r0, [sp, #88]                   @ 4-byte Reload
	adds	r4, r4, r2
	adcs	r7, r5, r7
	ldr	r5, [sp, #80]                   @ 4-byte Reload
	ldr	r6, [sp, #172]
	mul	r1, r0, r4
	adcs	r5, r5, lr
	str	r1, [sp, #44]                   @ 4-byte Spill
	ldm	r10, {r8, r9, r10}
	ldr	r11, [sp, #196]
	ldm	r12, {r0, r1, r2, r3, r12}
	str	r5, [sp, #32]                   @ 4-byte Spill
	ldr	r5, [sp, #76]                   @ 4-byte Reload
	adcs	r6, r5, r6
	str	r6, [sp, #28]                   @ 4-byte Spill
	ldr	r6, [sp, #72]                   @ 4-byte Reload
	adcs	r0, r6, r0
	str	r0, [sp, #36]                   @ 4-byte Spill
	ldr	r0, [sp, #68]                   @ 4-byte Reload
	adcs	r0, r0, r1
	str	r0, [sp, #96]                   @ 4-byte Spill
	ldr	r0, [sp, #64]                   @ 4-byte Reload
	adcs	r0, r0, r2
	str	r0, [sp, #40]                   @ 4-byte Spill
	ldr	r0, [sp, #60]                   @ 4-byte Reload
	ldr	r2, [sp, #44]                   @ 4-byte Reload
	adcs	r0, r0, r3
	str	r0, [sp, #88]                   @ 4-byte Spill
	ldr	r0, [sp, #56]                   @ 4-byte Reload
	adcs	r0, r0, r12
	str	r0, [sp, #56]                   @ 4-byte Spill
	ldr	r0, [sp, #52]                   @ 4-byte Reload
	adcs	r6, r0, r11
	ldr	r0, [sp, #48]                   @ 4-byte Reload
	adcs	r0, r0, r8
	str	r0, [sp, #68]                   @ 4-byte Spill
	ldr	r0, [sp, #92]                   @ 4-byte Reload
	adcs	r9, r0, r9
	adc	r0, r10, #0
	ldr	r10, [sp, #100]                 @ 4-byte Reload
	str	r0, [sp, #76]                   @ 4-byte Spill
	add	r0, sp, #104
	mov	r1, r10
	bl	mulPv384x32
	add	r3, sp, #104
	ldm	r3, {r0, r1, r2, r3}
	adds	r0, r4, r0
	ldr	r4, [r10, #12]
	ldr	r0, [sp, #32]                   @ 4-byte Reload
	adcs	r8, r7, r1
	str	r8, [sp, #80]                   @ 4-byte Spill
	adcs	r5, r0, r2
	ldr	r0, [sp, #28]                   @ 4-byte Reload
	str	r5, [sp, #72]                   @ 4-byte Spill
	adcs	r7, r0, r3
	str	r7, [sp, #64]                   @ 4-byte Spill
	ldr	r0, [sp, #120]
	ldr	r1, [sp, #36]                   @ 4-byte Reload
	adcs	r2, r1, r0
	str	r2, [sp, #60]                   @ 4-byte Spill
	ldr	r0, [sp, #124]
	ldr	r1, [sp, #96]                   @ 4-byte Reload
	adcs	r0, r1, r0
	str	r0, [sp, #96]                   @ 4-byte Spill
	ldr	r0, [sp, #128]
	ldr	r1, [sp, #40]                   @ 4-byte Reload
	adcs	r0, r1, r0
	str	r0, [sp, #92]                   @ 4-byte Spill
	ldr	r0, [sp, #132]
	ldr	r1, [sp, #88]                   @ 4-byte Reload
	adcs	r0, r1, r0
	str	r0, [sp, #88]                   @ 4-byte Spill
	ldr	r0, [sp, #136]
	ldr	r1, [sp, #56]                   @ 4-byte Reload
	adcs	r0, r1, r0
	str	r0, [sp, #48]                   @ 4-byte Spill
	ldr	r0, [sp, #140]
	ldr	r1, [sp, #68]                   @ 4-byte Reload
	adcs	r0, r6, r0
	str	r0, [sp, #44]                   @ 4-byte Spill
	ldr	r0, [sp, #144]
	adcs	r0, r1, r0
	str	r0, [sp, #32]                   @ 4-byte Spill
	ldr	r0, [sp, #148]
	ldr	r1, [sp, #76]                   @ 4-byte Reload
	adcs	r0, r9, r0
	str	r0, [sp, #40]                   @ 4-byte Spill
	ldr	r0, [sp, #152]
	adc	r6, r1, r0
	ldm	r10, {r1, r3, r9}
	mov	r0, r10
	subs	r1, r8, r1
	str	r1, [sp, #76]                   @ 4-byte Spill
	sbcs	r1, r5, r3
	str	r1, [sp, #68]                   @ 4-byte Spill
	sbcs	r1, r7, r9
	ldr	r12, [r0, #28]
	str	r1, [sp, #56]                   @ 4-byte Spill
	sbcs	r1, r2, r4
	ldr	r10, [r10, #16]
	add	r3, r0, #36
	ldr	r11, [r0, #20]
	ldr	lr, [r0, #24]
	ldr	r4, [r0, #32]
	ldr	r0, [sp, #96]                   @ 4-byte Reload
	str	r12, [sp, #36]                  @ 4-byte Spill
	sbcs	r10, r0, r10
	ldr	r0, [sp, #92]                   @ 4-byte Reload
	str	r1, [sp, #52]                   @ 4-byte Spill
	sbcs	r9, r0, r11
	ldr	r0, [sp, #88]                   @ 4-byte Reload
	ldr	r11, [sp, #48]                  @ 4-byte Reload
	sbcs	r7, r0, lr
	ldr	r0, [sp, #36]                   @ 4-byte Reload
	ldr	r8, [sp, #44]                   @ 4-byte Reload
	sbcs	r5, r11, r0
	ldm	r3, {r1, r2, r3}
	sbcs	r4, r8, r4
	ldr	lr, [sp, #32]                   @ 4-byte Reload
	ldr	r0, [sp, #40]                   @ 4-byte Reload
	sbcs	r1, lr, r1
	sbcs	r2, r0, r2
	sbc	r3, r6, r3
	asr	r12, r3, #31
	cmn	r12, #1
	movgt	r6, r3
	ldr	r3, [sp, #84]                   @ 4-byte Reload
	movle	r2, r0
	movle	r1, lr
	cmn	r12, #1
	add	r0, r3, #36
	movle	r4, r8
	movle	r5, r11
	str	r4, [r3, #32]
	stm	r0, {r1, r2, r6}
	ldr	r0, [sp, #88]                   @ 4-byte Reload
	ldr	r1, [sp, #92]                   @ 4-byte Reload
	ldr	r2, [sp, #52]                   @ 4-byte Reload
	movle	r7, r0
	cmn	r12, #1
	movle	r9, r1
	ldr	r1, [sp, #96]                   @ 4-byte Reload
	ldr	r0, [sp, #64]                   @ 4-byte Reload
	str	r5, [r3, #28]
	movle	r10, r1
	ldr	r1, [sp, #60]                   @ 4-byte Reload
	str	r7, [r3, #24]
	str	r9, [r3, #20]
	movle	r2, r1
	cmn	r12, #1
	ldr	r1, [sp, #56]                   @ 4-byte Reload
	movle	r1, r0
	ldr	r0, [sp, #68]                   @ 4-byte Reload
	str	r1, [r3, #8]
	ldr	r1, [sp, #72]                   @ 4-byte Reload
	str	r10, [r3, #16]
	str	r2, [r3, #12]
	movle	r0, r1
	ldr	r1, [sp, #80]                   @ 4-byte Reload
	str	r0, [r3, #4]
	ldr	r0, [sp, #76]                   @ 4-byte Reload
	movle	r0, r1
	str	r0, [r3]
	add	sp, sp, #428
	add	sp, sp, #1024
	pop	{r4, r5, r6, r7, r8, r9, r10, r11, lr}
	mov	pc, lr
.Lfunc_end61:
	.size	mcl_fp_montNF12L, .Lfunc_end61-mcl_fp_montNF12L
	.cantunwind
	.fnend
                                        @ -- End function
	.globl	mcl_fp_montRed12L               @ -- Begin function mcl_fp_montRed12L
	.p2align	2
	.type	mcl_fp_montRed12L,%function
	.code	32                              @ @mcl_fp_montRed12L
mcl_fp_montRed12L:
	.fnstart
@ %bb.0:
	.save	{r4, r5, r6, r7, r8, r9, r10, r11, lr}
	push	{r4, r5, r6, r7, r8, r9, r10, r11, lr}
	.pad	#852
	sub	sp, sp, #852
	str	r0, [sp, #160]                  @ 4-byte Spill
	mov	r6, r2
	ldr	r0, [r2]
	str	r0, [sp, #156]                  @ 4-byte Spill
	ldr	r0, [r2, #4]
	str	r0, [sp, #152]                  @ 4-byte Spill
	ldr	r0, [r2, #8]
	str	r0, [sp, #148]                  @ 4-byte Spill
	ldr	r0, [r2, #12]
	str	r0, [sp, #132]                  @ 4-byte Spill
	ldr	r0, [r2, #16]
	str	r0, [sp, #136]                  @ 4-byte Spill
	ldr	r0, [r2, #20]
	str	r0, [sp, #140]                  @ 4-byte Spill
	ldr	r0, [r2, #24]
	str	r0, [sp, #144]                  @ 4-byte Spill
	ldr	r0, [r1, #4]
	str	r0, [sp, #108]                  @ 4-byte Spill
	ldr	r0, [r1, #8]
	str	r0, [sp, #104]                  @ 4-byte Spill
	ldr	r0, [r1, #12]
	str	r0, [sp, #100]                  @ 4-byte Spill
	ldr	r0, [r1, #16]
	str	r0, [sp, #84]                   @ 4-byte Spill
	ldr	r0, [r1, #20]
	str	r0, [sp, #88]                   @ 4-byte Spill
	ldr	r0, [r1, #24]
	str	r0, [sp, #92]                   @ 4-byte Spill
	ldr	r0, [r1, #28]
	str	r0, [sp, #96]                   @ 4-byte Spill
	ldr	r0, [r6, #28]
	str	r0, [sp, #112]                  @ 4-byte Spill
	ldr	r0, [r6, #32]
	ldr	r3, [r2, #-4]
	ldr	r4, [r1]
	str	r0, [sp, #116]                  @ 4-byte Spill
	ldr	r0, [r6, #36]
	str	r0, [sp, #120]                  @ 4-byte Spill
	mul	r2, r4, r3
	ldr	r0, [r6, #40]
	str	r0, [sp, #124]                  @ 4-byte Spill
	ldr	r0, [r6, #44]
	str	r0, [sp, #128]                  @ 4-byte Spill
	ldr	r0, [r1, #32]
	str	r0, [sp, #72]                   @ 4-byte Spill
	ldr	r0, [r1, #36]
	str	r0, [sp, #68]                   @ 4-byte Spill
	ldr	r0, [r1, #40]
	str	r0, [sp, #60]                   @ 4-byte Spill
	ldr	r0, [r1, #44]
	str	r0, [sp, #64]                   @ 4-byte Spill
	add	r0, sp, #792
	str	r1, [sp, #172]                  @ 4-byte Spill
	mov	r1, r6
	str	r3, [sp, #164]                  @ 4-byte Spill
	str	r6, [sp, #168]                  @ 4-byte Spill
	bl	mulPv384x32
	add	lr, sp, #792
	add	r11, sp, #816
	ldm	lr, {r0, r1, r2, r3, r12, lr}
	adds	r0, r4, r0
	ldm	r11, {r5, r7, r8, r9, r10, r11}
	ldr	r0, [sp, #108]                  @ 4-byte Reload
	adcs	r4, r0, r1
	ldr	r0, [sp, #104]                  @ 4-byte Reload
	mov	r1, r6
	adcs	r0, r0, r2
	str	r0, [sp, #108]                  @ 4-byte Spill
	ldr	r0, [sp, #100]                  @ 4-byte Reload
	adcs	r0, r0, r3
	str	r0, [sp, #100]                  @ 4-byte Spill
	ldr	r0, [sp, #84]                   @ 4-byte Reload
	adcs	r0, r0, r12
	str	r0, [sp, #84]                   @ 4-byte Spill
	ldr	r0, [sp, #88]                   @ 4-byte Reload
	adcs	r0, r0, lr
	str	r0, [sp, #88]                   @ 4-byte Spill
	ldr	r0, [sp, #92]                   @ 4-byte Reload
	adcs	r0, r0, r5
	str	r0, [sp, #80]                   @ 4-byte Spill
	ldr	r0, [sp, #96]                   @ 4-byte Reload
	adcs	r0, r0, r7
	str	r0, [sp, #76]                   @ 4-byte Spill
	ldr	r0, [sp, #72]                   @ 4-byte Reload
	adcs	r0, r0, r8
	str	r0, [sp, #72]                   @ 4-byte Spill
	ldr	r0, [sp, #68]                   @ 4-byte Reload
	adcs	r0, r0, r9
	str	r0, [sp, #68]                   @ 4-byte Spill
	ldr	r0, [sp, #60]                   @ 4-byte Reload
	adcs	r0, r0, r10
	str	r0, [sp, #56]                   @ 4-byte Spill
	ldr	r0, [sp, #64]                   @ 4-byte Reload
	adcs	r0, r0, r11
	str	r0, [sp, #52]                   @ 4-byte Spill
	mrs	r0, apsr
	ldr	r5, [sp, #164]                  @ 4-byte Reload
	ldr	r7, [sp, #172]                  @ 4-byte Reload
	str	r0, [sp, #64]                   @ 4-byte Spill
	mul	r2, r5, r4
	ldr	r0, [r7, #48]
	str	r0, [sp, #104]                  @ 4-byte Spill
	ldr	r0, [sp, #840]
	str	r0, [sp, #60]                   @ 4-byte Spill
	add	r0, sp, #736
	bl	mulPv384x32
	add	r3, sp, #736
	ldr	r12, [sp, #776]
	ldr	lr, [sp, #772]
	ldm	r3, {r0, r1, r2, r3}
	adds	r0, r4, r0
	ldr	r4, [sp, #108]                  @ 4-byte Reload
	ldr	r6, [sp, #768]
	adcs	r8, r4, r1
	ldr	r1, [sp, #100]                  @ 4-byte Reload
	ldr	r9, [sp, #764]
	adcs	r1, r1, r2
	ldr	r10, [sp, #760]
	ldr	r11, [sp, #756]
	mul	r2, r5, r8
	ldr	r0, [sp, #752]
	str	r1, [sp, #100]                  @ 4-byte Spill
	ldr	r1, [sp, #84]                   @ 4-byte Reload
	adcs	r1, r1, r3
	str	r1, [sp, #96]                   @ 4-byte Spill
	ldr	r1, [sp, #88]                   @ 4-byte Reload
	adcs	r0, r1, r0
	str	r0, [sp, #92]                   @ 4-byte Spill
	ldr	r0, [sp, #80]                   @ 4-byte Reload
	adcs	r0, r0, r11
	str	r0, [sp, #88]                   @ 4-byte Spill
	ldr	r0, [sp, #76]                   @ 4-byte Reload
	adcs	r0, r0, r10
	str	r0, [sp, #84]                   @ 4-byte Spill
	ldr	r0, [sp, #72]                   @ 4-byte Reload
	adcs	r0, r0, r9
	str	r0, [sp, #80]                   @ 4-byte Spill
	ldr	r0, [sp, #68]                   @ 4-byte Reload
	mov	r9, r7
	adcs	r0, r0, r6
	str	r0, [sp, #76]                   @ 4-byte Spill
	ldr	r0, [sp, #56]                   @ 4-byte Reload
	mov	r6, r5
	adcs	r0, r0, lr
	str	r0, [sp, #72]                   @ 4-byte Spill
	ldr	r0, [sp, #52]                   @ 4-byte Reload
	adcs	r0, r0, r12
	str	r0, [sp, #68]                   @ 4-byte Spill
	mrs	r0, apsr
	str	r0, [sp, #108]                  @ 4-byte Spill
	ldr	r0, [sp, #64]                   @ 4-byte Reload
	msr	APSR_nzcvq, r0
	ldr	r1, [sp, #104]                  @ 4-byte Reload
	ldr	r3, [sp, #60]                   @ 4-byte Reload
	ldr	r0, [sp, #784]
	adcs	r1, r1, r3
	str	r1, [sp, #64]                   @ 4-byte Spill
	adc	r0, r0, #0
	str	r0, [sp, #60]                   @ 4-byte Spill
	ldr	r0, [r7, #52]
	str	r0, [sp, #56]                   @ 4-byte Spill
	ldr	r4, [sp, #168]                  @ 4-byte Reload
	ldr	r0, [sp, #780]
	str	r0, [sp, #52]                   @ 4-byte Spill
	add	r0, sp, #680
	mov	r1, r4
	bl	mulPv384x32
	add	lr, sp, #680
	ldr	r11, [sp, #716]
	ldr	r10, [sp, #712]
	ldm	lr, {r0, r1, r2, r3, r5, r7, r12, lr}
	adds	r0, r8, r0
	ldr	r0, [sp, #100]                  @ 4-byte Reload
	adcs	r8, r0, r1
	ldr	r0, [sp, #96]                   @ 4-byte Reload
	adcs	r0, r0, r2
	str	r0, [sp, #100]                  @ 4-byte Spill
	ldr	r0, [sp, #92]                   @ 4-byte Reload
	mul	r2, r6, r8
	adcs	r0, r0, r3
	str	r0, [sp, #96]                   @ 4-byte Spill
	ldr	r0, [sp, #88]                   @ 4-byte Reload
	adcs	r0, r0, r5
	str	r0, [sp, #92]                   @ 4-byte Spill
	ldr	r0, [sp, #84]                   @ 4-byte Reload
	adcs	r0, r0, r7
	str	r0, [sp, #88]                   @ 4-byte Spill
	ldr	r0, [sp, #80]                   @ 4-byte Reload
	adcs	r0, r0, r12
	str	r0, [sp, #84]                   @ 4-byte Spill
	ldr	r0, [sp, #76]                   @ 4-byte Reload
	adcs	r0, r0, lr
	str	r0, [sp, #80]                   @ 4-byte Spill
	ldr	r0, [sp, #72]                   @ 4-byte Reload
	adcs	r0, r0, r10
	str	r0, [sp, #76]                   @ 4-byte Spill
	ldr	r0, [sp, #68]                   @ 4-byte Reload
	adcs	r0, r0, r11
	str	r0, [sp, #72]                   @ 4-byte Spill
	mrs	r0, apsr
	str	r0, [sp, #104]                  @ 4-byte Spill
	ldr	r0, [sp, #108]                  @ 4-byte Reload
	msr	APSR_nzcvq, r0
	ldr	r1, [sp, #64]                   @ 4-byte Reload
	ldr	r3, [sp, #52]                   @ 4-byte Reload
	ldr	r0, [sp, #728]
	adcs	r1, r1, r3
	str	r1, [sp, #68]                   @ 4-byte Spill
	ldr	r1, [sp, #60]                   @ 4-byte Reload
	ldr	r3, [sp, #56]                   @ 4-byte Reload
	adcs	r1, r3, r1
	str	r1, [sp, #64]                   @ 4-byte Spill
	adc	r0, r0, #0
	str	r0, [sp, #60]                   @ 4-byte Spill
	ldr	r0, [r9, #56]
	mov	r1, r4
	str	r0, [sp, #56]                   @ 4-byte Spill
	add	r0, sp, #624
	ldr	r10, [sp, #724]
	ldr	r9, [sp, #720]
	bl	mulPv384x32
	ldr	r5, [sp, #624]
	add	r11, sp, #628
	ldr	r12, [sp, #656]
	adds	r4, r8, r5
	ldm	r11, {r0, r1, r7, r11}
	mov	r8, r6
	ldr	r4, [sp, #100]                  @ 4-byte Reload
	ldr	lr, [sp, #652]
	adcs	r4, r4, r0
	ldr	r0, [sp, #96]                   @ 4-byte Reload
	ldr	r2, [sp, #648]
	adcs	r0, r0, r1
	ldr	r3, [sp, #644]
	str	r0, [sp, #100]                  @ 4-byte Spill
	ldr	r0, [sp, #92]                   @ 4-byte Reload
	adcs	r0, r0, r7
	str	r0, [sp, #96]                   @ 4-byte Spill
	ldr	r0, [sp, #88]                   @ 4-byte Reload
	adcs	r0, r0, r11
	str	r0, [sp, #92]                   @ 4-byte Spill
	ldr	r0, [sp, #84]                   @ 4-byte Reload
	adcs	r0, r0, r3
	str	r0, [sp, #88]                   @ 4-byte Spill
	ldr	r0, [sp, #80]                   @ 4-byte Reload
	adcs	r0, r0, r2
	str	r0, [sp, #84]                   @ 4-byte Spill
	ldr	r0, [sp, #76]                   @ 4-byte Reload
	mul	r2, r6, r4
	adcs	r0, r0, lr
	str	r0, [sp, #80]                   @ 4-byte Spill
	ldr	r0, [sp, #72]                   @ 4-byte Reload
	adcs	r0, r0, r12
	str	r0, [sp, #76]                   @ 4-byte Spill
	mrs	r0, apsr
	str	r0, [sp, #108]                  @ 4-byte Spill
	ldr	r0, [sp, #104]                  @ 4-byte Reload
	msr	APSR_nzcvq, r0
	ldr	r1, [sp, #68]                   @ 4-byte Reload
	ldr	r0, [sp, #672]
	adcs	r1, r1, r9
	str	r1, [sp, #72]                   @ 4-byte Spill
	ldr	r1, [sp, #64]                   @ 4-byte Reload
	ldr	r3, [sp, #56]                   @ 4-byte Reload
	adcs	r1, r1, r10
	str	r1, [sp, #68]                   @ 4-byte Spill
	ldr	r1, [sp, #60]                   @ 4-byte Reload
	ldr	r6, [sp, #172]                  @ 4-byte Reload
	adcs	r1, r3, r1
	str	r1, [sp, #64]                   @ 4-byte Spill
	adc	r0, r0, #0
	str	r0, [sp, #60]                   @ 4-byte Spill
	ldr	r0, [r6, #60]
	str	r0, [sp, #56]                   @ 4-byte Spill
	ldr	r9, [sp, #168]                  @ 4-byte Reload
	ldr	r0, [sp, #668]
	str	r0, [sp, #52]                   @ 4-byte Spill
	add	r0, sp, #568
	mov	r1, r9
	ldr	r11, [sp, #664]
	ldr	r10, [sp, #660]
	bl	mulPv384x32
	ldr	r7, [sp, #568]
	add	r3, sp, #576
	ldr	r5, [sp, #572]
	adds	r4, r4, r7
	ldm	r3, {r0, r1, r3}
	ldr	r4, [sp, #100]                  @ 4-byte Reload
	ldr	r12, [sp, #596]
	adcs	r4, r4, r5
	ldr	r5, [sp, #96]                   @ 4-byte Reload
	ldr	lr, [sp, #592]
	adcs	r0, r5, r0
	ldr	r2, [sp, #588]
	str	r0, [sp, #100]                  @ 4-byte Spill
	ldr	r0, [sp, #92]                   @ 4-byte Reload
	adcs	r0, r0, r1
	str	r0, [sp, #96]                   @ 4-byte Spill
	ldr	r0, [sp, #88]                   @ 4-byte Reload
	adcs	r0, r0, r3
	str	r0, [sp, #92]                   @ 4-byte Spill
	ldr	r0, [sp, #84]                   @ 4-byte Reload
	adcs	r0, r0, r2
	str	r0, [sp, #88]                   @ 4-byte Spill
	ldr	r0, [sp, #80]                   @ 4-byte Reload
	mul	r2, r8, r4
	adcs	r0, r0, lr
	str	r0, [sp, #84]                   @ 4-byte Spill
	ldr	r0, [sp, #76]                   @ 4-byte Reload
	adcs	r0, r0, r12
	str	r0, [sp, #80]                   @ 4-byte Spill
	mrs	r0, apsr
	str	r0, [sp, #104]                  @ 4-byte Spill
	ldr	r0, [sp, #108]                  @ 4-byte Reload
	msr	APSR_nzcvq, r0
	ldr	r1, [sp, #72]                   @ 4-byte Reload
	ldr	r0, [sp, #616]
	adcs	r1, r1, r10
	str	r1, [sp, #108]                  @ 4-byte Spill
	ldr	r1, [sp, #68]                   @ 4-byte Reload
	ldr	r3, [sp, #52]                   @ 4-byte Reload
	adcs	r1, r1, r11
	str	r1, [sp, #76]                   @ 4-byte Spill
	ldr	r1, [sp, #64]                   @ 4-byte Reload
	adcs	r1, r1, r3
	str	r1, [sp, #72]                   @ 4-byte Spill
	ldr	r1, [sp, #60]                   @ 4-byte Reload
	ldr	r3, [sp, #56]                   @ 4-byte Reload
	adcs	r1, r3, r1
	str	r1, [sp, #68]                   @ 4-byte Spill
	adc	r0, r0, #0
	str	r0, [sp, #64]                   @ 4-byte Spill
	ldr	r0, [r6, #64]
	mov	r1, r9
	str	r0, [sp, #60]                   @ 4-byte Spill
	ldr	r0, [sp, #612]
	str	r0, [sp, #52]                   @ 4-byte Spill
	ldr	r0, [sp, #608]
	str	r0, [sp, #48]                   @ 4-byte Spill
	ldr	r0, [sp, #604]
	str	r0, [sp, #44]                   @ 4-byte Spill
	add	r0, sp, #512
	ldr	r10, [sp, #600]
	bl	mulPv384x32
	ldr	r3, [sp, #512]
	ldr	r7, [sp, #516]
	adds	r3, r4, r3
	ldr	r5, [sp, #520]
	ldr	r3, [sp, #100]                  @ 4-byte Reload
	ldr	r12, [sp, #536]
	adcs	r4, r3, r7
	ldr	r3, [sp, #96]                   @ 4-byte Reload
	ldr	r1, [sp, #532]
	adcs	r3, r3, r5
	ldr	r2, [sp, #528]
	ldr	r0, [sp, #524]
	str	r3, [sp, #100]                  @ 4-byte Spill
	ldr	r3, [sp, #92]                   @ 4-byte Reload
	adcs	r0, r3, r0
	str	r0, [sp, #96]                   @ 4-byte Spill
	ldr	r0, [sp, #88]                   @ 4-byte Reload
	adcs	r0, r0, r2
	str	r0, [sp, #92]                   @ 4-byte Spill
	ldr	r0, [sp, #84]                   @ 4-byte Reload
	mul	r2, r8, r4
	adcs	r0, r0, r1
	str	r0, [sp, #88]                   @ 4-byte Spill
	ldr	r0, [sp, #80]                   @ 4-byte Reload
	adcs	r0, r0, r12
	str	r0, [sp, #84]                   @ 4-byte Spill
	mrs	r0, apsr
	str	r0, [sp, #56]                   @ 4-byte Spill
	ldr	r0, [sp, #104]                  @ 4-byte Reload
	msr	APSR_nzcvq, r0
	ldr	r1, [sp, #108]                  @ 4-byte Reload
	ldr	r0, [sp, #560]
	adcs	r1, r1, r10
	str	r1, [sp, #104]                  @ 4-byte Spill
	ldr	r1, [sp, #76]                   @ 4-byte Reload
	ldr	r3, [sp, #44]                   @ 4-byte Reload
	adcs	r1, r1, r3
	str	r1, [sp, #80]                   @ 4-byte Spill
	ldr	r1, [sp, #72]                   @ 4-byte Reload
	ldr	r3, [sp, #48]                   @ 4-byte Reload
	adcs	r1, r1, r3
	str	r1, [sp, #76]                   @ 4-byte Spill
	ldr	r1, [sp, #68]                   @ 4-byte Reload
	ldr	r3, [sp, #52]                   @ 4-byte Reload
	adcs	r1, r1, r3
	str	r1, [sp, #72]                   @ 4-byte Spill
	ldr	r1, [sp, #64]                   @ 4-byte Reload
	ldr	r3, [sp, #60]                   @ 4-byte Reload
	adcs	r1, r3, r1
	str	r1, [sp, #68]                   @ 4-byte Spill
	adc	r0, r0, #0
	str	r0, [sp, #64]                   @ 4-byte Spill
	ldr	r0, [r6, #68]
	mov	r1, r9
	str	r0, [sp, #60]                   @ 4-byte Spill
	ldr	r0, [sp, #556]
	str	r0, [sp, #52]                   @ 4-byte Spill
	ldr	r0, [sp, #552]
	str	r0, [sp, #48]                   @ 4-byte Spill
	add	r0, sp, #456
	ldr	r8, [sp, #548]
	ldr	r10, [sp, #544]
	ldr	r11, [sp, #540]
	bl	mulPv384x32
	add	r7, sp, #456
	ldr	r0, [sp, #476]
	ldr	r1, [sp, #472]
	ldm	r7, {r2, r3, r7}
	adds	r2, r4, r2
	ldr	r5, [sp, #468]
	ldr	r2, [sp, #100]                  @ 4-byte Reload
	adcs	r9, r2, r3
	ldr	r2, [sp, #96]                   @ 4-byte Reload
	adcs	r2, r2, r7
	str	r2, [sp, #100]                  @ 4-byte Spill
	ldr	r2, [sp, #92]                   @ 4-byte Reload
	adcs	r2, r2, r5
	str	r2, [sp, #96]                   @ 4-byte Spill
	ldr	r2, [sp, #88]                   @ 4-byte Reload
	adcs	r1, r2, r1
	str	r1, [sp, #92]                   @ 4-byte Spill
	ldr	r1, [sp, #84]                   @ 4-byte Reload
	adcs	r0, r1, r0
	str	r0, [sp, #88]                   @ 4-byte Spill
	mrs	r0, apsr
	str	r0, [sp, #108]                  @ 4-byte Spill
	ldr	r0, [sp, #56]                   @ 4-byte Reload
	msr	APSR_nzcvq, r0
	ldr	r1, [sp, #104]                  @ 4-byte Reload
	ldr	r0, [sp, #504]
	adcs	r1, r1, r11
	str	r1, [sp, #84]                   @ 4-byte Spill
	ldr	r1, [sp, #80]                   @ 4-byte Reload
	ldr	r3, [sp, #48]                   @ 4-byte Reload
	adcs	r1, r1, r10
	str	r1, [sp, #80]                   @ 4-byte Spill
	ldr	r1, [sp, #76]                   @ 4-byte Reload
	ldr	r6, [sp, #172]                  @ 4-byte Reload
	adcs	r1, r1, r8
	str	r1, [sp, #76]                   @ 4-byte Spill
	ldr	r1, [sp, #72]                   @ 4-byte Reload
	ldr	r4, [sp, #164]                  @ 4-byte Reload
	adcs	r1, r1, r3
	str	r1, [sp, #72]                   @ 4-byte Spill
	ldr	r1, [sp, #68]                   @ 4-byte Reload
	ldr	r3, [sp, #52]                   @ 4-byte Reload
	mul	r2, r4, r9
	ldr	r7, [sp, #168]                  @ 4-byte Reload
	adcs	r1, r1, r3
	str	r1, [sp, #68]                   @ 4-byte Spill
	ldr	r1, [sp, #64]                   @ 4-byte Reload
	ldr	r3, [sp, #60]                   @ 4-byte Reload
	adcs	r1, r3, r1
	str	r1, [sp, #64]                   @ 4-byte Spill
	adc	r0, r0, #0
	str	r0, [sp, #60]                   @ 4-byte Spill
	ldr	r0, [r6, #72]
	mov	r1, r7
	str	r0, [sp, #56]                   @ 4-byte Spill
	ldr	r0, [sp, #500]
	str	r0, [sp, #52]                   @ 4-byte Spill
	ldr	r0, [sp, #496]
	str	r0, [sp, #48]                   @ 4-byte Spill
	ldr	r0, [sp, #492]
	str	r0, [sp, #44]                   @ 4-byte Spill
	ldr	r0, [sp, #488]
	str	r0, [sp, #40]                   @ 4-byte Spill
	add	r0, sp, #400
	ldr	r10, [sp, #484]
	ldr	r8, [sp, #480]
	bl	mulPv384x32
	add	r5, sp, #400
	ldr	r0, [sp, #416]
	ldm	r5, {r1, r2, r3, r5}
	adds	r1, r9, r1
	ldr	r1, [sp, #100]                  @ 4-byte Reload
	adcs	r11, r1, r2
	ldr	r1, [sp, #96]                   @ 4-byte Reload
	adcs	r1, r1, r3
	str	r1, [sp, #100]                  @ 4-byte Spill
	ldr	r1, [sp, #92]                   @ 4-byte Reload
	mul	r2, r4, r11
	adcs	r1, r1, r5
	str	r1, [sp, #96]                   @ 4-byte Spill
	ldr	r1, [sp, #88]                   @ 4-byte Reload
	mov	r5, r6
	adcs	r0, r1, r0
	str	r0, [sp, #92]                   @ 4-byte Spill
	mrs	r0, apsr
	str	r0, [sp, #104]                  @ 4-byte Spill
	ldr	r0, [sp, #108]                  @ 4-byte Reload
	msr	APSR_nzcvq, r0
	ldr	r1, [sp, #84]                   @ 4-byte Reload
	ldr	r0, [sp, #448]
	adcs	r1, r1, r8
	str	r1, [sp, #88]                   @ 4-byte Spill
	ldr	r1, [sp, #80]                   @ 4-byte Reload
	ldr	r3, [sp, #40]                   @ 4-byte Reload
	adcs	r1, r1, r10
	str	r1, [sp, #84]                   @ 4-byte Spill
	ldr	r1, [sp, #76]                   @ 4-byte Reload
	adcs	r1, r1, r3
	str	r1, [sp, #80]                   @ 4-byte Spill
	ldr	r1, [sp, #72]                   @ 4-byte Reload
	ldr	r3, [sp, #44]                   @ 4-byte Reload
	adcs	r1, r1, r3
	str	r1, [sp, #76]                   @ 4-byte Spill
	ldr	r1, [sp, #68]                   @ 4-byte Reload
	ldr	r3, [sp, #48]                   @ 4-byte Reload
	adcs	r1, r1, r3
	str	r1, [sp, #72]                   @ 4-byte Spill
	ldr	r1, [sp, #64]                   @ 4-byte Reload
	ldr	r3, [sp, #52]                   @ 4-byte Reload
	adcs	r1, r1, r3
	str	r1, [sp, #68]                   @ 4-byte Spill
	ldr	r1, [sp, #60]                   @ 4-byte Reload
	ldr	r3, [sp, #56]                   @ 4-byte Reload
	adcs	r1, r3, r1
	str	r1, [sp, #64]                   @ 4-byte Spill
	adc	r0, r0, #0
	str	r0, [sp, #60]                   @ 4-byte Spill
	ldr	r0, [r6, #76]
	mov	r1, r7
	str	r0, [sp, #56]                   @ 4-byte Spill
	ldr	r0, [sp, #444]
	str	r0, [sp, #52]                   @ 4-byte Spill
	ldr	r0, [sp, #440]
	str	r0, [sp, #48]                   @ 4-byte Spill
	ldr	r0, [sp, #436]
	str	r0, [sp, #44]                   @ 4-byte Spill
	add	r0, sp, #344
	ldr	r4, [sp, #432]
	ldr	r8, [sp, #428]
	ldr	r10, [sp, #424]
	ldr	r9, [sp, #420]
	bl	mulPv384x32
	add	r3, sp, #344
	ldm	r3, {r0, r1, r2, r3}
	adds	r0, r11, r0
	ldr	r0, [sp, #100]                  @ 4-byte Reload
	adcs	r7, r0, r1
	ldr	r0, [sp, #96]                   @ 4-byte Reload
	adcs	r0, r0, r2
	str	r0, [sp, #100]                  @ 4-byte Spill
	ldr	r0, [sp, #92]                   @ 4-byte Reload
	adcs	r0, r0, r3
	str	r0, [sp, #96]                   @ 4-byte Spill
	mrs	r0, apsr
	str	r0, [sp, #108]                  @ 4-byte Spill
	ldr	r0, [sp, #104]                  @ 4-byte Reload
	msr	APSR_nzcvq, r0
	ldr	r1, [sp, #88]                   @ 4-byte Reload
	ldr	r0, [sp, #392]
	adcs	r1, r1, r9
	str	r1, [sp, #104]                  @ 4-byte Spill
	ldr	r1, [sp, #84]                   @ 4-byte Reload
	ldr	r3, [sp, #44]                   @ 4-byte Reload
	adcs	r1, r1, r10
	str	r1, [sp, #92]                   @ 4-byte Spill
	ldr	r1, [sp, #80]                   @ 4-byte Reload
	ldr	r6, [sp, #164]                  @ 4-byte Reload
	adcs	r1, r1, r8
	str	r1, [sp, #88]                   @ 4-byte Spill
	ldr	r1, [sp, #76]                   @ 4-byte Reload
	mul	r2, r6, r7
	adcs	r1, r1, r4
	str	r1, [sp, #84]                   @ 4-byte Spill
	ldr	r1, [sp, #72]                   @ 4-byte Reload
	adcs	r1, r1, r3
	str	r1, [sp, #80]                   @ 4-byte Spill
	ldr	r1, [sp, #68]                   @ 4-byte Reload
	ldr	r3, [sp, #48]                   @ 4-byte Reload
	adcs	r1, r1, r3
	str	r1, [sp, #76]                   @ 4-byte Spill
	ldr	r1, [sp, #64]                   @ 4-byte Reload
	ldr	r3, [sp, #52]                   @ 4-byte Reload
	adcs	r1, r1, r3
	str	r1, [sp, #68]                   @ 4-byte Spill
	ldr	r1, [sp, #60]                   @ 4-byte Reload
	ldr	r3, [sp, #56]                   @ 4-byte Reload
	adcs	r1, r3, r1
	str	r1, [sp, #64]                   @ 4-byte Spill
	adc	r0, r0, #0
	str	r0, [sp, #60]                   @ 4-byte Spill
	ldr	r0, [r5, #80]
	str	r0, [sp, #52]                   @ 4-byte Spill
	ldr	r0, [sp, #388]
	str	r0, [sp, #44]                   @ 4-byte Spill
	ldr	r0, [sp, #384]
	str	r0, [sp, #40]                   @ 4-byte Spill
	ldr	r5, [sp, #168]                  @ 4-byte Reload
	ldr	r0, [sp, #380]
	str	r0, [sp, #36]                   @ 4-byte Spill
	add	r0, sp, #288
	mov	r1, r5
	ldr	r4, [sp, #376]
	ldr	r8, [sp, #372]
	ldr	r9, [sp, #368]
	ldr	r10, [sp, #364]
	ldr	r11, [sp, #360]
	bl	mulPv384x32
	add	r3, sp, #288
	ldm	r3, {r0, r1, r2, r3}
	adds	r0, r7, r0
	str	r3, [sp, #72]                   @ 4-byte Spill
	ldr	r0, [sp, #100]                  @ 4-byte Reload
	adcs	r7, r0, r1
	ldr	r0, [sp, #96]                   @ 4-byte Reload
	adcs	r0, r0, r2
	str	r0, [sp, #48]                   @ 4-byte Spill
	mrs	r0, apsr
	mul	r2, r6, r7
	str	r0, [sp, #56]                   @ 4-byte Spill
	ldr	r0, [sp, #108]                  @ 4-byte Reload
	msr	APSR_nzcvq, r0
	ldr	r1, [sp, #104]                  @ 4-byte Reload
	ldr	r0, [sp, #336]
	adcs	r11, r1, r11
	ldr	r1, [sp, #92]                   @ 4-byte Reload
	ldr	r3, [sp, #36]                   @ 4-byte Reload
	adcs	r10, r1, r10
	ldr	r1, [sp, #88]                   @ 4-byte Reload
	adcs	r9, r1, r9
	ldr	r1, [sp, #84]                   @ 4-byte Reload
	adcs	r1, r1, r8
	str	r1, [sp, #104]                  @ 4-byte Spill
	ldr	r1, [sp, #80]                   @ 4-byte Reload
	adcs	r1, r1, r4
	str	r1, [sp, #100]                  @ 4-byte Spill
	ldr	r1, [sp, #76]                   @ 4-byte Reload
	ldr	r4, [sp, #172]                  @ 4-byte Reload
	adcs	r1, r1, r3
	str	r1, [sp, #96]                   @ 4-byte Spill
	ldr	r1, [sp, #68]                   @ 4-byte Reload
	ldr	r3, [sp, #40]                   @ 4-byte Reload
	adcs	r1, r1, r3
	str	r1, [sp, #92]                   @ 4-byte Spill
	ldr	r1, [sp, #64]                   @ 4-byte Reload
	ldr	r3, [sp, #44]                   @ 4-byte Reload
	adcs	r1, r1, r3
	str	r1, [sp, #88]                   @ 4-byte Spill
	ldr	r1, [sp, #60]                   @ 4-byte Reload
	ldr	r3, [sp, #52]                   @ 4-byte Reload
	adcs	r1, r3, r1
	str	r1, [sp, #84]                   @ 4-byte Spill
	adc	r0, r0, #0
	str	r0, [sp, #80]                   @ 4-byte Spill
	ldr	r0, [r4, #84]
	mov	r1, r5
	str	r0, [sp, #76]                   @ 4-byte Spill
	ldr	r0, [sp, #332]
	str	r0, [sp, #68]                   @ 4-byte Spill
	ldr	r0, [sp, #328]
	str	r0, [sp, #64]                   @ 4-byte Spill
	ldr	r0, [sp, #324]
	str	r0, [sp, #60]                   @ 4-byte Spill
	ldr	r0, [sp, #320]
	str	r0, [sp, #52]                   @ 4-byte Spill
	ldr	r0, [sp, #316]
	str	r0, [sp, #36]                   @ 4-byte Spill
	ldr	r0, [sp, #312]
	str	r0, [sp, #32]                   @ 4-byte Spill
	ldr	r0, [sp, #308]
	str	r0, [sp, #28]                   @ 4-byte Spill
	add	r0, sp, #232
	ldr	r8, [sp, #304]
	bl	mulPv384x32
	add	r2, sp, #232
	ldm	r2, {r0, r1, r2}
	adds	r0, r7, r0
	str	r2, [sp, #44]                   @ 4-byte Spill
	ldr	r0, [sp, #48]                   @ 4-byte Reload
	ldr	r2, [sp, #244]
	adcs	r0, r0, r1
	str	r2, [sp, #40]                   @ 4-byte Spill
	str	r0, [sp, #108]                  @ 4-byte Spill
	mrs	r1, apsr
	str	r1, [sp, #48]                   @ 4-byte Spill
	mul	r2, r6, r0
	ldr	r1, [sp, #56]                   @ 4-byte Reload
	msr	APSR_nzcvq, r1
	ldr	r0, [sp, #72]                   @ 4-byte Reload
	ldr	r1, [sp, #28]                   @ 4-byte Reload
	adcs	r0, r11, r0
	str	r0, [sp, #20]                   @ 4-byte Spill
	ldr	r0, [sp, #276]
	adcs	r5, r10, r8
	str	r0, [sp, #164]                  @ 4-byte Spill
	adcs	r11, r9, r1
	ldr	r0, [sp, #272]
	str	r0, [sp, #72]                   @ 4-byte Spill
	ldr	r1, [sp, #104]                  @ 4-byte Reload
	ldr	r3, [sp, #32]                   @ 4-byte Reload
	ldr	r0, [sp, #268]
	str	r0, [sp, #56]                   @ 4-byte Spill
	adcs	r9, r1, r3
	ldr	r1, [sp, #100]                  @ 4-byte Reload
	ldr	r3, [sp, #36]                   @ 4-byte Reload
	ldr	r0, [sp, #264]
	str	r0, [sp, #24]                   @ 4-byte Spill
	adcs	r10, r1, r3
	ldr	r1, [sp, #96]                   @ 4-byte Reload
	ldr	r3, [sp, #52]                   @ 4-byte Reload
	ldr	r0, [sp, #260]
	str	r0, [sp, #16]                   @ 4-byte Spill
	adcs	r8, r1, r3
	ldr	r1, [sp, #92]                   @ 4-byte Reload
	ldr	r3, [sp, #60]                   @ 4-byte Reload
	ldr	r0, [sp, #256]
	str	r0, [sp, #12]                   @ 4-byte Spill
	adcs	r7, r1, r3
	ldr	r1, [sp, #88]                   @ 4-byte Reload
	ldr	r3, [sp, #64]                   @ 4-byte Reload
	ldr	r0, [sp, #252]
	str	r0, [sp, #8]                    @ 4-byte Spill
	ldr	r6, [r4, #88]
	adcs	r4, r1, r3
	ldr	r1, [sp, #84]                   @ 4-byte Reload
	ldr	r3, [sp, #68]                   @ 4-byte Reload
	ldr	r0, [sp, #248]
	str	r0, [sp, #4]                    @ 4-byte Spill
	adcs	r1, r1, r3
	ldr	r0, [sp, #280]
	str	r1, [sp, #104]                  @ 4-byte Spill
	ldr	r1, [sp, #80]                   @ 4-byte Reload
	ldr	r3, [sp, #76]                   @ 4-byte Reload
	adcs	r1, r3, r1
	str	r1, [sp, #100]                  @ 4-byte Spill
	adc	r0, r0, #0
	ldr	r1, [sp, #168]                  @ 4-byte Reload
	str	r0, [sp, #96]                   @ 4-byte Spill
	add	r0, sp, #176
	bl	mulPv384x32
	ldr	r0, [sp, #48]                   @ 4-byte Reload
	msr	APSR_nzcvq, r0
	ldr	r0, [sp, #44]                   @ 4-byte Reload
	ldr	r1, [sp, #20]                   @ 4-byte Reload
	adcs	r3, r1, r0
	ldr	r0, [sp, #40]                   @ 4-byte Reload
	ldr	r1, [sp, #104]                  @ 4-byte Reload
	adcs	r12, r5, r0
	ldr	r0, [sp, #4]                    @ 4-byte Reload
	adcs	r2, r11, r0
	ldr	r0, [sp, #8]                    @ 4-byte Reload
	adcs	r9, r9, r0
	ldr	r0, [sp, #12]                   @ 4-byte Reload
	adcs	r10, r10, r0
	ldr	r0, [sp, #16]                   @ 4-byte Reload
	adcs	r11, r8, r0
	ldr	r0, [sp, #24]                   @ 4-byte Reload
	adcs	r7, r7, r0
	ldr	r0, [sp, #56]                   @ 4-byte Reload
	adcs	r8, r4, r0
	ldr	r0, [sp, #72]                   @ 4-byte Reload
	adcs	r0, r1, r0
	str	r0, [sp, #80]                   @ 4-byte Spill
	ldr	r0, [sp, #164]                  @ 4-byte Reload
	ldr	r1, [sp, #100]                  @ 4-byte Reload
	adcs	r0, r1, r0
	str	r0, [sp, #84]                   @ 4-byte Spill
	ldr	r0, [sp, #96]                   @ 4-byte Reload
	adcs	r0, r6, r0
	str	r0, [sp, #92]                   @ 4-byte Spill
	ldr	r4, [sp, #224]
	adc	r0, r4, #0
	str	r0, [sp, #88]                   @ 4-byte Spill
	ldr	r6, [sp, #176]
	ldr	r4, [sp, #108]                  @ 4-byte Reload
	ldr	r5, [sp, #180]
	adds	r6, r4, r6
	ldr	r0, [sp, #184]
	adcs	r3, r3, r5
	ldr	r1, [sp, #188]
	adcs	r6, r12, r0
	str	r3, [sp, #168]                  @ 4-byte Spill
	adcs	lr, r2, r1
	str	r6, [sp, #164]                  @ 4-byte Spill
	str	lr, [sp, #108]                  @ 4-byte Spill
	ldr	r0, [sp, #192]
	ldr	r4, [sp, #80]                   @ 4-byte Reload
	adcs	r2, r9, r0
	str	r2, [sp, #104]                  @ 4-byte Spill
	ldr	r0, [sp, #196]
	adcs	r5, r10, r0
	str	r5, [sp, #100]                  @ 4-byte Spill
	ldr	r0, [sp, #200]
	adcs	r12, r11, r0
	str	r12, [sp, #96]                  @ 4-byte Spill
	ldr	r0, [sp, #204]
	adcs	r11, r7, r0
	str	r11, [sp, #76]                  @ 4-byte Spill
	ldr	r0, [sp, #208]
	adcs	r1, r8, r0
	str	r1, [sp, #72]                   @ 4-byte Spill
	ldr	r0, [sp, #212]
	adcs	r0, r4, r0
	str	r0, [sp, #80]                   @ 4-byte Spill
	ldr	r0, [sp, #216]
	ldr	r4, [sp, #84]                   @ 4-byte Reload
	adcs	r0, r4, r0
	str	r0, [sp, #84]                   @ 4-byte Spill
	ldr	r0, [sp, #220]
	ldr	r4, [sp, #92]                   @ 4-byte Reload
	adcs	r10, r4, r0
	ldr	r0, [sp, #172]                  @ 4-byte Reload
	ldr	r4, [sp, #88]                   @ 4-byte Reload
	ldr	r0, [r0, #92]
	adc	r7, r0, r4
	ldr	r0, [sp, #156]                  @ 4-byte Reload
	ldr	r4, [sp, #80]                   @ 4-byte Reload
	subs	r0, r3, r0
	str	r0, [sp, #172]                  @ 4-byte Spill
	ldr	r0, [sp, #152]                  @ 4-byte Reload
	ldr	r3, [sp, #128]                  @ 4-byte Reload
	sbcs	r0, r6, r0
	str	r0, [sp, #156]                  @ 4-byte Spill
	ldr	r0, [sp, #148]                  @ 4-byte Reload
	sbcs	r0, lr, r0
	str	r0, [sp, #152]                  @ 4-byte Spill
	ldr	r0, [sp, #132]                  @ 4-byte Reload
	sbcs	r0, r2, r0
	str	r0, [sp, #148]                  @ 4-byte Spill
	ldr	r0, [sp, #136]                  @ 4-byte Reload
	ldr	r2, [sp, #124]                  @ 4-byte Reload
	sbcs	r9, r5, r0
	ldr	r0, [sp, #140]                  @ 4-byte Reload
	sbcs	r8, r12, r0
	ldr	r0, [sp, #144]                  @ 4-byte Reload
	ldr	r12, [sp, #84]                  @ 4-byte Reload
	sbcs	r5, r11, r0
	ldr	r0, [sp, #112]                  @ 4-byte Reload
	mov	r11, #0
	sbcs	lr, r1, r0
	ldr	r0, [sp, #116]                  @ 4-byte Reload
	ldr	r1, [sp, #120]                  @ 4-byte Reload
	sbcs	r0, r4, r0
	sbcs	r1, r12, r1
	sbcs	r2, r10, r2
	sbcs	r3, r7, r3
	sbc	r6, r11, #0
	ands	r6, r6, #1
	movne	r3, r7
	movne	r2, r10
	movne	r1, r12
	cmp	r6, #0
	movne	r0, r4
	ldr	r4, [sp, #160]                  @ 4-byte Reload
	add	r12, r4, #32
	stm	r12, {r0, r1, r2, r3}
	ldr	r0, [sp, #72]                   @ 4-byte Reload
	ldr	r1, [sp, #148]                  @ 4-byte Reload
	movne	lr, r0
	ldr	r0, [sp, #76]                   @ 4-byte Reload
	str	lr, [r4, #28]
	movne	r5, r0
	ldr	r0, [sp, #96]                   @ 4-byte Reload
	cmp	r6, #0
	str	r5, [r4, #24]
	movne	r8, r0
	ldr	r0, [sp, #100]                  @ 4-byte Reload
	str	r8, [r4, #20]
	movne	r9, r0
	ldr	r0, [sp, #104]                  @ 4-byte Reload
	str	r9, [r4, #16]
	movne	r1, r0
	cmp	r6, #0
	str	r1, [r4, #12]
	ldr	r1, [sp, #108]                  @ 4-byte Reload
	ldr	r0, [sp, #152]                  @ 4-byte Reload
	movne	r0, r1
	ldr	r1, [sp, #164]                  @ 4-byte Reload
	str	r0, [r4, #8]
	ldr	r0, [sp, #156]                  @ 4-byte Reload
	movne	r0, r1
	ldr	r1, [sp, #168]                  @ 4-byte Reload
	str	r0, [r4, #4]
	ldr	r0, [sp, #172]                  @ 4-byte Reload
	movne	r0, r1
	str	r0, [r4]
	add	sp, sp, #852
	pop	{r4, r5, r6, r7, r8, r9, r10, r11, lr}
	mov	pc, lr
.Lfunc_end62:
	.size	mcl_fp_montRed12L, .Lfunc_end62-mcl_fp_montRed12L
	.cantunwind
	.fnend
                                        @ -- End function
	.globl	mcl_fp_montRedNF12L             @ -- Begin function mcl_fp_montRedNF12L
	.p2align	2
	.type	mcl_fp_montRedNF12L,%function
	.code	32                              @ @mcl_fp_montRedNF12L
mcl_fp_montRedNF12L:
	.fnstart
@ %bb.0:
	.save	{r4, r5, r6, r7, r8, r9, r10, r11, lr}
	push	{r4, r5, r6, r7, r8, r9, r10, r11, lr}
	.pad	#852
	sub	sp, sp, #852
	str	r0, [sp, #160]                  @ 4-byte Spill
	mov	r6, r2
	ldr	r0, [r2]
	str	r0, [sp, #156]                  @ 4-byte Spill
	ldr	r0, [r2, #4]
	str	r0, [sp, #152]                  @ 4-byte Spill
	ldr	r0, [r2, #8]
	str	r0, [sp, #148]                  @ 4-byte Spill
	ldr	r0, [r2, #12]
	str	r0, [sp, #132]                  @ 4-byte Spill
	ldr	r0, [r2, #16]
	str	r0, [sp, #136]                  @ 4-byte Spill
	ldr	r0, [r2, #20]
	str	r0, [sp, #140]                  @ 4-byte Spill
	ldr	r0, [r2, #24]
	str	r0, [sp, #144]                  @ 4-byte Spill
	ldr	r0, [r1, #4]
	str	r0, [sp, #108]                  @ 4-byte Spill
	ldr	r0, [r1, #8]
	str	r0, [sp, #104]                  @ 4-byte Spill
	ldr	r0, [r1, #12]
	str	r0, [sp, #100]                  @ 4-byte Spill
	ldr	r0, [r1, #16]
	str	r0, [sp, #84]                   @ 4-byte Spill
	ldr	r0, [r1, #20]
	str	r0, [sp, #88]                   @ 4-byte Spill
	ldr	r0, [r1, #24]
	str	r0, [sp, #92]                   @ 4-byte Spill
	ldr	r0, [r1, #28]
	str	r0, [sp, #96]                   @ 4-byte Spill
	ldr	r0, [r6, #28]
	str	r0, [sp, #112]                  @ 4-byte Spill
	ldr	r0, [r6, #32]
	ldr	r3, [r2, #-4]
	ldr	r4, [r1]
	str	r0, [sp, #116]                  @ 4-byte Spill
	ldr	r0, [r6, #36]
	str	r0, [sp, #120]                  @ 4-byte Spill
	mul	r2, r4, r3
	ldr	r0, [r6, #40]
	str	r0, [sp, #124]                  @ 4-byte Spill
	ldr	r0, [r6, #44]
	str	r0, [sp, #128]                  @ 4-byte Spill
	ldr	r0, [r1, #32]
	str	r0, [sp, #72]                   @ 4-byte Spill
	ldr	r0, [r1, #36]
	str	r0, [sp, #68]                   @ 4-byte Spill
	ldr	r0, [r1, #40]
	str	r0, [sp, #60]                   @ 4-byte Spill
	ldr	r0, [r1, #44]
	str	r0, [sp, #64]                   @ 4-byte Spill
	add	r0, sp, #792
	str	r1, [sp, #172]                  @ 4-byte Spill
	mov	r1, r6
	str	r3, [sp, #164]                  @ 4-byte Spill
	str	r6, [sp, #168]                  @ 4-byte Spill
	bl	mulPv384x32
	add	lr, sp, #792
	add	r11, sp, #816
	ldm	lr, {r0, r1, r2, r3, r12, lr}
	adds	r0, r4, r0
	ldm	r11, {r5, r7, r8, r9, r10, r11}
	ldr	r0, [sp, #108]                  @ 4-byte Reload
	adcs	r4, r0, r1
	ldr	r0, [sp, #104]                  @ 4-byte Reload
	mov	r1, r6
	adcs	r0, r0, r2
	str	r0, [sp, #108]                  @ 4-byte Spill
	ldr	r0, [sp, #100]                  @ 4-byte Reload
	adcs	r0, r0, r3
	str	r0, [sp, #100]                  @ 4-byte Spill
	ldr	r0, [sp, #84]                   @ 4-byte Reload
	adcs	r0, r0, r12
	str	r0, [sp, #84]                   @ 4-byte Spill
	ldr	r0, [sp, #88]                   @ 4-byte Reload
	adcs	r0, r0, lr
	str	r0, [sp, #88]                   @ 4-byte Spill
	ldr	r0, [sp, #92]                   @ 4-byte Reload
	adcs	r0, r0, r5
	str	r0, [sp, #80]                   @ 4-byte Spill
	ldr	r0, [sp, #96]                   @ 4-byte Reload
	adcs	r0, r0, r7
	str	r0, [sp, #76]                   @ 4-byte Spill
	ldr	r0, [sp, #72]                   @ 4-byte Reload
	adcs	r0, r0, r8
	str	r0, [sp, #72]                   @ 4-byte Spill
	ldr	r0, [sp, #68]                   @ 4-byte Reload
	adcs	r0, r0, r9
	str	r0, [sp, #68]                   @ 4-byte Spill
	ldr	r0, [sp, #60]                   @ 4-byte Reload
	adcs	r0, r0, r10
	str	r0, [sp, #56]                   @ 4-byte Spill
	ldr	r0, [sp, #64]                   @ 4-byte Reload
	adcs	r0, r0, r11
	str	r0, [sp, #52]                   @ 4-byte Spill
	mrs	r0, apsr
	ldr	r5, [sp, #164]                  @ 4-byte Reload
	ldr	r7, [sp, #172]                  @ 4-byte Reload
	str	r0, [sp, #64]                   @ 4-byte Spill
	mul	r2, r5, r4
	ldr	r0, [r7, #48]
	str	r0, [sp, #104]                  @ 4-byte Spill
	ldr	r0, [sp, #840]
	str	r0, [sp, #60]                   @ 4-byte Spill
	add	r0, sp, #736
	bl	mulPv384x32
	add	r3, sp, #736
	ldr	r12, [sp, #776]
	ldr	lr, [sp, #772]
	ldm	r3, {r0, r1, r2, r3}
	adds	r0, r4, r0
	ldr	r4, [sp, #108]                  @ 4-byte Reload
	ldr	r6, [sp, #768]
	adcs	r8, r4, r1
	ldr	r1, [sp, #100]                  @ 4-byte Reload
	ldr	r9, [sp, #764]
	adcs	r1, r1, r2
	ldr	r10, [sp, #760]
	ldr	r11, [sp, #756]
	mul	r2, r5, r8
	ldr	r0, [sp, #752]
	str	r1, [sp, #100]                  @ 4-byte Spill
	ldr	r1, [sp, #84]                   @ 4-byte Reload
	adcs	r1, r1, r3
	str	r1, [sp, #96]                   @ 4-byte Spill
	ldr	r1, [sp, #88]                   @ 4-byte Reload
	adcs	r0, r1, r0
	str	r0, [sp, #92]                   @ 4-byte Spill
	ldr	r0, [sp, #80]                   @ 4-byte Reload
	adcs	r0, r0, r11
	str	r0, [sp, #88]                   @ 4-byte Spill
	ldr	r0, [sp, #76]                   @ 4-byte Reload
	adcs	r0, r0, r10
	str	r0, [sp, #84]                   @ 4-byte Spill
	ldr	r0, [sp, #72]                   @ 4-byte Reload
	adcs	r0, r0, r9
	str	r0, [sp, #80]                   @ 4-byte Spill
	ldr	r0, [sp, #68]                   @ 4-byte Reload
	mov	r9, r7
	adcs	r0, r0, r6
	str	r0, [sp, #76]                   @ 4-byte Spill
	ldr	r0, [sp, #56]                   @ 4-byte Reload
	mov	r6, r5
	adcs	r0, r0, lr
	str	r0, [sp, #72]                   @ 4-byte Spill
	ldr	r0, [sp, #52]                   @ 4-byte Reload
	adcs	r0, r0, r12
	str	r0, [sp, #68]                   @ 4-byte Spill
	mrs	r0, apsr
	str	r0, [sp, #108]                  @ 4-byte Spill
	ldr	r0, [sp, #64]                   @ 4-byte Reload
	msr	APSR_nzcvq, r0
	ldr	r1, [sp, #104]                  @ 4-byte Reload
	ldr	r3, [sp, #60]                   @ 4-byte Reload
	ldr	r0, [sp, #784]
	adcs	r1, r1, r3
	str	r1, [sp, #64]                   @ 4-byte Spill
	adc	r0, r0, #0
	str	r0, [sp, #60]                   @ 4-byte Spill
	ldr	r0, [r7, #52]
	str	r0, [sp, #56]                   @ 4-byte Spill
	ldr	r4, [sp, #168]                  @ 4-byte Reload
	ldr	r0, [sp, #780]
	str	r0, [sp, #52]                   @ 4-byte Spill
	add	r0, sp, #680
	mov	r1, r4
	bl	mulPv384x32
	add	lr, sp, #680
	ldr	r11, [sp, #716]
	ldr	r10, [sp, #712]
	ldm	lr, {r0, r1, r2, r3, r5, r7, r12, lr}
	adds	r0, r8, r0
	ldr	r0, [sp, #100]                  @ 4-byte Reload
	adcs	r8, r0, r1
	ldr	r0, [sp, #96]                   @ 4-byte Reload
	adcs	r0, r0, r2
	str	r0, [sp, #100]                  @ 4-byte Spill
	ldr	r0, [sp, #92]                   @ 4-byte Reload
	mul	r2, r6, r8
	adcs	r0, r0, r3
	str	r0, [sp, #96]                   @ 4-byte Spill
	ldr	r0, [sp, #88]                   @ 4-byte Reload
	adcs	r0, r0, r5
	str	r0, [sp, #92]                   @ 4-byte Spill
	ldr	r0, [sp, #84]                   @ 4-byte Reload
	adcs	r0, r0, r7
	str	r0, [sp, #88]                   @ 4-byte Spill
	ldr	r0, [sp, #80]                   @ 4-byte Reload
	adcs	r0, r0, r12
	str	r0, [sp, #84]                   @ 4-byte Spill
	ldr	r0, [sp, #76]                   @ 4-byte Reload
	adcs	r0, r0, lr
	str	r0, [sp, #80]                   @ 4-byte Spill
	ldr	r0, [sp, #72]                   @ 4-byte Reload
	adcs	r0, r0, r10
	str	r0, [sp, #76]                   @ 4-byte Spill
	ldr	r0, [sp, #68]                   @ 4-byte Reload
	adcs	r0, r0, r11
	str	r0, [sp, #72]                   @ 4-byte Spill
	mrs	r0, apsr
	str	r0, [sp, #104]                  @ 4-byte Spill
	ldr	r0, [sp, #108]                  @ 4-byte Reload
	msr	APSR_nzcvq, r0
	ldr	r1, [sp, #64]                   @ 4-byte Reload
	ldr	r3, [sp, #52]                   @ 4-byte Reload
	ldr	r0, [sp, #728]
	adcs	r1, r1, r3
	str	r1, [sp, #68]                   @ 4-byte Spill
	ldr	r1, [sp, #60]                   @ 4-byte Reload
	ldr	r3, [sp, #56]                   @ 4-byte Reload
	adcs	r1, r3, r1
	str	r1, [sp, #64]                   @ 4-byte Spill
	adc	r0, r0, #0
	str	r0, [sp, #60]                   @ 4-byte Spill
	ldr	r0, [r9, #56]
	mov	r1, r4
	str	r0, [sp, #56]                   @ 4-byte Spill
	add	r0, sp, #624
	ldr	r10, [sp, #724]
	ldr	r9, [sp, #720]
	bl	mulPv384x32
	ldr	r5, [sp, #624]
	add	r11, sp, #628
	ldr	r12, [sp, #656]
	adds	r4, r8, r5
	ldm	r11, {r0, r1, r7, r11}
	mov	r8, r6
	ldr	r4, [sp, #100]                  @ 4-byte Reload
	ldr	lr, [sp, #652]
	adcs	r4, r4, r0
	ldr	r0, [sp, #96]                   @ 4-byte Reload
	ldr	r2, [sp, #648]
	adcs	r0, r0, r1
	ldr	r3, [sp, #644]
	str	r0, [sp, #100]                  @ 4-byte Spill
	ldr	r0, [sp, #92]                   @ 4-byte Reload
	adcs	r0, r0, r7
	str	r0, [sp, #96]                   @ 4-byte Spill
	ldr	r0, [sp, #88]                   @ 4-byte Reload
	adcs	r0, r0, r11
	str	r0, [sp, #92]                   @ 4-byte Spill
	ldr	r0, [sp, #84]                   @ 4-byte Reload
	adcs	r0, r0, r3
	str	r0, [sp, #88]                   @ 4-byte Spill
	ldr	r0, [sp, #80]                   @ 4-byte Reload
	adcs	r0, r0, r2
	str	r0, [sp, #84]                   @ 4-byte Spill
	ldr	r0, [sp, #76]                   @ 4-byte Reload
	mul	r2, r6, r4
	adcs	r0, r0, lr
	str	r0, [sp, #80]                   @ 4-byte Spill
	ldr	r0, [sp, #72]                   @ 4-byte Reload
	adcs	r0, r0, r12
	str	r0, [sp, #76]                   @ 4-byte Spill
	mrs	r0, apsr
	str	r0, [sp, #108]                  @ 4-byte Spill
	ldr	r0, [sp, #104]                  @ 4-byte Reload
	msr	APSR_nzcvq, r0
	ldr	r1, [sp, #68]                   @ 4-byte Reload
	ldr	r0, [sp, #672]
	adcs	r1, r1, r9
	str	r1, [sp, #72]                   @ 4-byte Spill
	ldr	r1, [sp, #64]                   @ 4-byte Reload
	ldr	r3, [sp, #56]                   @ 4-byte Reload
	adcs	r1, r1, r10
	str	r1, [sp, #68]                   @ 4-byte Spill
	ldr	r1, [sp, #60]                   @ 4-byte Reload
	ldr	r6, [sp, #172]                  @ 4-byte Reload
	adcs	r1, r3, r1
	str	r1, [sp, #64]                   @ 4-byte Spill
	adc	r0, r0, #0
	str	r0, [sp, #60]                   @ 4-byte Spill
	ldr	r0, [r6, #60]
	str	r0, [sp, #56]                   @ 4-byte Spill
	ldr	r9, [sp, #168]                  @ 4-byte Reload
	ldr	r0, [sp, #668]
	str	r0, [sp, #52]                   @ 4-byte Spill
	add	r0, sp, #568
	mov	r1, r9
	ldr	r11, [sp, #664]
	ldr	r10, [sp, #660]
	bl	mulPv384x32
	ldr	r7, [sp, #568]
	add	r3, sp, #576
	ldr	r5, [sp, #572]
	adds	r4, r4, r7
	ldm	r3, {r0, r1, r3}
	ldr	r4, [sp, #100]                  @ 4-byte Reload
	ldr	r12, [sp, #596]
	adcs	r4, r4, r5
	ldr	r5, [sp, #96]                   @ 4-byte Reload
	ldr	lr, [sp, #592]
	adcs	r0, r5, r0
	ldr	r2, [sp, #588]
	str	r0, [sp, #100]                  @ 4-byte Spill
	ldr	r0, [sp, #92]                   @ 4-byte Reload
	adcs	r0, r0, r1
	str	r0, [sp, #96]                   @ 4-byte Spill
	ldr	r0, [sp, #88]                   @ 4-byte Reload
	adcs	r0, r0, r3
	str	r0, [sp, #92]                   @ 4-byte Spill
	ldr	r0, [sp, #84]                   @ 4-byte Reload
	adcs	r0, r0, r2
	str	r0, [sp, #88]                   @ 4-byte Spill
	ldr	r0, [sp, #80]                   @ 4-byte Reload
	mul	r2, r8, r4
	adcs	r0, r0, lr
	str	r0, [sp, #84]                   @ 4-byte Spill
	ldr	r0, [sp, #76]                   @ 4-byte Reload
	adcs	r0, r0, r12
	str	r0, [sp, #80]                   @ 4-byte Spill
	mrs	r0, apsr
	str	r0, [sp, #104]                  @ 4-byte Spill
	ldr	r0, [sp, #108]                  @ 4-byte Reload
	msr	APSR_nzcvq, r0
	ldr	r1, [sp, #72]                   @ 4-byte Reload
	ldr	r0, [sp, #616]
	adcs	r1, r1, r10
	str	r1, [sp, #108]                  @ 4-byte Spill
	ldr	r1, [sp, #68]                   @ 4-byte Reload
	ldr	r3, [sp, #52]                   @ 4-byte Reload
	adcs	r1, r1, r11
	str	r1, [sp, #76]                   @ 4-byte Spill
	ldr	r1, [sp, #64]                   @ 4-byte Reload
	adcs	r1, r1, r3
	str	r1, [sp, #72]                   @ 4-byte Spill
	ldr	r1, [sp, #60]                   @ 4-byte Reload
	ldr	r3, [sp, #56]                   @ 4-byte Reload
	adcs	r1, r3, r1
	str	r1, [sp, #68]                   @ 4-byte Spill
	adc	r0, r0, #0
	str	r0, [sp, #64]                   @ 4-byte Spill
	ldr	r0, [r6, #64]
	mov	r1, r9
	str	r0, [sp, #60]                   @ 4-byte Spill
	ldr	r0, [sp, #612]
	str	r0, [sp, #52]                   @ 4-byte Spill
	ldr	r0, [sp, #608]
	str	r0, [sp, #48]                   @ 4-byte Spill
	ldr	r0, [sp, #604]
	str	r0, [sp, #44]                   @ 4-byte Spill
	add	r0, sp, #512
	ldr	r10, [sp, #600]
	bl	mulPv384x32
	ldr	r3, [sp, #512]
	ldr	r7, [sp, #516]
	adds	r3, r4, r3
	ldr	r5, [sp, #520]
	ldr	r3, [sp, #100]                  @ 4-byte Reload
	ldr	r12, [sp, #536]
	adcs	r4, r3, r7
	ldr	r3, [sp, #96]                   @ 4-byte Reload
	ldr	r1, [sp, #532]
	adcs	r3, r3, r5
	ldr	r2, [sp, #528]
	ldr	r0, [sp, #524]
	str	r3, [sp, #100]                  @ 4-byte Spill
	ldr	r3, [sp, #92]                   @ 4-byte Reload
	adcs	r0, r3, r0
	str	r0, [sp, #96]                   @ 4-byte Spill
	ldr	r0, [sp, #88]                   @ 4-byte Reload
	adcs	r0, r0, r2
	str	r0, [sp, #92]                   @ 4-byte Spill
	ldr	r0, [sp, #84]                   @ 4-byte Reload
	mul	r2, r8, r4
	adcs	r0, r0, r1
	str	r0, [sp, #88]                   @ 4-byte Spill
	ldr	r0, [sp, #80]                   @ 4-byte Reload
	adcs	r0, r0, r12
	str	r0, [sp, #84]                   @ 4-byte Spill
	mrs	r0, apsr
	str	r0, [sp, #56]                   @ 4-byte Spill
	ldr	r0, [sp, #104]                  @ 4-byte Reload
	msr	APSR_nzcvq, r0
	ldr	r1, [sp, #108]                  @ 4-byte Reload
	ldr	r0, [sp, #560]
	adcs	r1, r1, r10
	str	r1, [sp, #104]                  @ 4-byte Spill
	ldr	r1, [sp, #76]                   @ 4-byte Reload
	ldr	r3, [sp, #44]                   @ 4-byte Reload
	adcs	r1, r1, r3
	str	r1, [sp, #80]                   @ 4-byte Spill
	ldr	r1, [sp, #72]                   @ 4-byte Reload
	ldr	r3, [sp, #48]                   @ 4-byte Reload
	adcs	r1, r1, r3
	str	r1, [sp, #76]                   @ 4-byte Spill
	ldr	r1, [sp, #68]                   @ 4-byte Reload
	ldr	r3, [sp, #52]                   @ 4-byte Reload
	adcs	r1, r1, r3
	str	r1, [sp, #72]                   @ 4-byte Spill
	ldr	r1, [sp, #64]                   @ 4-byte Reload
	ldr	r3, [sp, #60]                   @ 4-byte Reload
	adcs	r1, r3, r1
	str	r1, [sp, #68]                   @ 4-byte Spill
	adc	r0, r0, #0
	str	r0, [sp, #64]                   @ 4-byte Spill
	ldr	r0, [r6, #68]
	mov	r1, r9
	str	r0, [sp, #60]                   @ 4-byte Spill
	ldr	r0, [sp, #556]
	str	r0, [sp, #52]                   @ 4-byte Spill
	ldr	r0, [sp, #552]
	str	r0, [sp, #48]                   @ 4-byte Spill
	add	r0, sp, #456
	ldr	r8, [sp, #548]
	ldr	r10, [sp, #544]
	ldr	r11, [sp, #540]
	bl	mulPv384x32
	add	r7, sp, #456
	ldr	r0, [sp, #476]
	ldr	r1, [sp, #472]
	ldm	r7, {r2, r3, r7}
	adds	r2, r4, r2
	ldr	r5, [sp, #468]
	ldr	r2, [sp, #100]                  @ 4-byte Reload
	adcs	r9, r2, r3
	ldr	r2, [sp, #96]                   @ 4-byte Reload
	adcs	r2, r2, r7
	str	r2, [sp, #100]                  @ 4-byte Spill
	ldr	r2, [sp, #92]                   @ 4-byte Reload
	adcs	r2, r2, r5
	str	r2, [sp, #96]                   @ 4-byte Spill
	ldr	r2, [sp, #88]                   @ 4-byte Reload
	adcs	r1, r2, r1
	str	r1, [sp, #92]                   @ 4-byte Spill
	ldr	r1, [sp, #84]                   @ 4-byte Reload
	adcs	r0, r1, r0
	str	r0, [sp, #88]                   @ 4-byte Spill
	mrs	r0, apsr
	str	r0, [sp, #108]                  @ 4-byte Spill
	ldr	r0, [sp, #56]                   @ 4-byte Reload
	msr	APSR_nzcvq, r0
	ldr	r1, [sp, #104]                  @ 4-byte Reload
	ldr	r0, [sp, #504]
	adcs	r1, r1, r11
	str	r1, [sp, #84]                   @ 4-byte Spill
	ldr	r1, [sp, #80]                   @ 4-byte Reload
	ldr	r3, [sp, #48]                   @ 4-byte Reload
	adcs	r1, r1, r10
	str	r1, [sp, #80]                   @ 4-byte Spill
	ldr	r1, [sp, #76]                   @ 4-byte Reload
	ldr	r6, [sp, #172]                  @ 4-byte Reload
	adcs	r1, r1, r8
	str	r1, [sp, #76]                   @ 4-byte Spill
	ldr	r1, [sp, #72]                   @ 4-byte Reload
	ldr	r4, [sp, #164]                  @ 4-byte Reload
	adcs	r1, r1, r3
	str	r1, [sp, #72]                   @ 4-byte Spill
	ldr	r1, [sp, #68]                   @ 4-byte Reload
	ldr	r3, [sp, #52]                   @ 4-byte Reload
	mul	r2, r4, r9
	ldr	r7, [sp, #168]                  @ 4-byte Reload
	adcs	r1, r1, r3
	str	r1, [sp, #68]                   @ 4-byte Spill
	ldr	r1, [sp, #64]                   @ 4-byte Reload
	ldr	r3, [sp, #60]                   @ 4-byte Reload
	adcs	r1, r3, r1
	str	r1, [sp, #64]                   @ 4-byte Spill
	adc	r0, r0, #0
	str	r0, [sp, #60]                   @ 4-byte Spill
	ldr	r0, [r6, #72]
	mov	r1, r7
	str	r0, [sp, #56]                   @ 4-byte Spill
	ldr	r0, [sp, #500]
	str	r0, [sp, #52]                   @ 4-byte Spill
	ldr	r0, [sp, #496]
	str	r0, [sp, #48]                   @ 4-byte Spill
	ldr	r0, [sp, #492]
	str	r0, [sp, #44]                   @ 4-byte Spill
	ldr	r0, [sp, #488]
	str	r0, [sp, #40]                   @ 4-byte Spill
	add	r0, sp, #400
	ldr	r10, [sp, #484]
	ldr	r8, [sp, #480]
	bl	mulPv384x32
	add	r5, sp, #400
	ldr	r0, [sp, #416]
	ldm	r5, {r1, r2, r3, r5}
	adds	r1, r9, r1
	ldr	r1, [sp, #100]                  @ 4-byte Reload
	adcs	r11, r1, r2
	ldr	r1, [sp, #96]                   @ 4-byte Reload
	adcs	r1, r1, r3
	str	r1, [sp, #100]                  @ 4-byte Spill
	ldr	r1, [sp, #92]                   @ 4-byte Reload
	mul	r2, r4, r11
	adcs	r1, r1, r5
	str	r1, [sp, #96]                   @ 4-byte Spill
	ldr	r1, [sp, #88]                   @ 4-byte Reload
	mov	r5, r6
	adcs	r0, r1, r0
	str	r0, [sp, #92]                   @ 4-byte Spill
	mrs	r0, apsr
	str	r0, [sp, #104]                  @ 4-byte Spill
	ldr	r0, [sp, #108]                  @ 4-byte Reload
	msr	APSR_nzcvq, r0
	ldr	r1, [sp, #84]                   @ 4-byte Reload
	ldr	r0, [sp, #448]
	adcs	r1, r1, r8
	str	r1, [sp, #88]                   @ 4-byte Spill
	ldr	r1, [sp, #80]                   @ 4-byte Reload
	ldr	r3, [sp, #40]                   @ 4-byte Reload
	adcs	r1, r1, r10
	str	r1, [sp, #84]                   @ 4-byte Spill
	ldr	r1, [sp, #76]                   @ 4-byte Reload
	adcs	r1, r1, r3
	str	r1, [sp, #80]                   @ 4-byte Spill
	ldr	r1, [sp, #72]                   @ 4-byte Reload
	ldr	r3, [sp, #44]                   @ 4-byte Reload
	adcs	r1, r1, r3
	str	r1, [sp, #76]                   @ 4-byte Spill
	ldr	r1, [sp, #68]                   @ 4-byte Reload
	ldr	r3, [sp, #48]                   @ 4-byte Reload
	adcs	r1, r1, r3
	str	r1, [sp, #72]                   @ 4-byte Spill
	ldr	r1, [sp, #64]                   @ 4-byte Reload
	ldr	r3, [sp, #52]                   @ 4-byte Reload
	adcs	r1, r1, r3
	str	r1, [sp, #68]                   @ 4-byte Spill
	ldr	r1, [sp, #60]                   @ 4-byte Reload
	ldr	r3, [sp, #56]                   @ 4-byte Reload
	adcs	r1, r3, r1
	str	r1, [sp, #64]                   @ 4-byte Spill
	adc	r0, r0, #0
	str	r0, [sp, #60]                   @ 4-byte Spill
	ldr	r0, [r6, #76]
	mov	r1, r7
	str	r0, [sp, #56]                   @ 4-byte Spill
	ldr	r0, [sp, #444]
	str	r0, [sp, #52]                   @ 4-byte Spill
	ldr	r0, [sp, #440]
	str	r0, [sp, #48]                   @ 4-byte Spill
	ldr	r0, [sp, #436]
	str	r0, [sp, #44]                   @ 4-byte Spill
	add	r0, sp, #344
	ldr	r4, [sp, #432]
	ldr	r8, [sp, #428]
	ldr	r10, [sp, #424]
	ldr	r9, [sp, #420]
	bl	mulPv384x32
	add	r3, sp, #344
	ldm	r3, {r0, r1, r2, r3}
	adds	r0, r11, r0
	ldr	r0, [sp, #100]                  @ 4-byte Reload
	adcs	r7, r0, r1
	ldr	r0, [sp, #96]                   @ 4-byte Reload
	adcs	r0, r0, r2
	str	r0, [sp, #100]                  @ 4-byte Spill
	ldr	r0, [sp, #92]                   @ 4-byte Reload
	adcs	r0, r0, r3
	str	r0, [sp, #96]                   @ 4-byte Spill
	mrs	r0, apsr
	str	r0, [sp, #108]                  @ 4-byte Spill
	ldr	r0, [sp, #104]                  @ 4-byte Reload
	msr	APSR_nzcvq, r0
	ldr	r1, [sp, #88]                   @ 4-byte Reload
	ldr	r0, [sp, #392]
	adcs	r1, r1, r9
	str	r1, [sp, #104]                  @ 4-byte Spill
	ldr	r1, [sp, #84]                   @ 4-byte Reload
	ldr	r3, [sp, #44]                   @ 4-byte Reload
	adcs	r1, r1, r10
	str	r1, [sp, #92]                   @ 4-byte Spill
	ldr	r1, [sp, #80]                   @ 4-byte Reload
	ldr	r6, [sp, #164]                  @ 4-byte Reload
	adcs	r1, r1, r8
	str	r1, [sp, #88]                   @ 4-byte Spill
	ldr	r1, [sp, #76]                   @ 4-byte Reload
	mul	r2, r6, r7
	adcs	r1, r1, r4
	str	r1, [sp, #84]                   @ 4-byte Spill
	ldr	r1, [sp, #72]                   @ 4-byte Reload
	adcs	r1, r1, r3
	str	r1, [sp, #80]                   @ 4-byte Spill
	ldr	r1, [sp, #68]                   @ 4-byte Reload
	ldr	r3, [sp, #48]                   @ 4-byte Reload
	adcs	r1, r1, r3
	str	r1, [sp, #76]                   @ 4-byte Spill
	ldr	r1, [sp, #64]                   @ 4-byte Reload
	ldr	r3, [sp, #52]                   @ 4-byte Reload
	adcs	r1, r1, r3
	str	r1, [sp, #68]                   @ 4-byte Spill
	ldr	r1, [sp, #60]                   @ 4-byte Reload
	ldr	r3, [sp, #56]                   @ 4-byte Reload
	adcs	r1, r3, r1
	str	r1, [sp, #64]                   @ 4-byte Spill
	adc	r0, r0, #0
	str	r0, [sp, #60]                   @ 4-byte Spill
	ldr	r0, [r5, #80]
	str	r0, [sp, #52]                   @ 4-byte Spill
	ldr	r0, [sp, #388]
	str	r0, [sp, #44]                   @ 4-byte Spill
	ldr	r0, [sp, #384]
	str	r0, [sp, #40]                   @ 4-byte Spill
	ldr	r5, [sp, #168]                  @ 4-byte Reload
	ldr	r0, [sp, #380]
	str	r0, [sp, #36]                   @ 4-byte Spill
	add	r0, sp, #288
	mov	r1, r5
	ldr	r4, [sp, #376]
	ldr	r8, [sp, #372]
	ldr	r9, [sp, #368]
	ldr	r10, [sp, #364]
	ldr	r11, [sp, #360]
	bl	mulPv384x32
	add	r3, sp, #288
	ldm	r3, {r0, r1, r2, r3}
	adds	r0, r7, r0
	str	r3, [sp, #72]                   @ 4-byte Spill
	ldr	r0, [sp, #100]                  @ 4-byte Reload
	adcs	r7, r0, r1
	ldr	r0, [sp, #96]                   @ 4-byte Reload
	adcs	r0, r0, r2
	str	r0, [sp, #48]                   @ 4-byte Spill
	mrs	r0, apsr
	mul	r2, r6, r7
	str	r0, [sp, #56]                   @ 4-byte Spill
	ldr	r0, [sp, #108]                  @ 4-byte Reload
	msr	APSR_nzcvq, r0
	ldr	r1, [sp, #104]                  @ 4-byte Reload
	ldr	r0, [sp, #336]
	adcs	r11, r1, r11
	ldr	r1, [sp, #92]                   @ 4-byte Reload
	ldr	r3, [sp, #36]                   @ 4-byte Reload
	adcs	r10, r1, r10
	ldr	r1, [sp, #88]                   @ 4-byte Reload
	adcs	r9, r1, r9
	ldr	r1, [sp, #84]                   @ 4-byte Reload
	adcs	r1, r1, r8
	str	r1, [sp, #104]                  @ 4-byte Spill
	ldr	r1, [sp, #80]                   @ 4-byte Reload
	adcs	r1, r1, r4
	str	r1, [sp, #100]                  @ 4-byte Spill
	ldr	r1, [sp, #76]                   @ 4-byte Reload
	ldr	r4, [sp, #172]                  @ 4-byte Reload
	adcs	r1, r1, r3
	str	r1, [sp, #96]                   @ 4-byte Spill
	ldr	r1, [sp, #68]                   @ 4-byte Reload
	ldr	r3, [sp, #40]                   @ 4-byte Reload
	adcs	r1, r1, r3
	str	r1, [sp, #92]                   @ 4-byte Spill
	ldr	r1, [sp, #64]                   @ 4-byte Reload
	ldr	r3, [sp, #44]                   @ 4-byte Reload
	adcs	r1, r1, r3
	str	r1, [sp, #88]                   @ 4-byte Spill
	ldr	r1, [sp, #60]                   @ 4-byte Reload
	ldr	r3, [sp, #52]                   @ 4-byte Reload
	adcs	r1, r3, r1
	str	r1, [sp, #84]                   @ 4-byte Spill
	adc	r0, r0, #0
	str	r0, [sp, #80]                   @ 4-byte Spill
	ldr	r0, [r4, #84]
	mov	r1, r5
	str	r0, [sp, #76]                   @ 4-byte Spill
	ldr	r0, [sp, #332]
	str	r0, [sp, #68]                   @ 4-byte Spill
	ldr	r0, [sp, #328]
	str	r0, [sp, #64]                   @ 4-byte Spill
	ldr	r0, [sp, #324]
	str	r0, [sp, #60]                   @ 4-byte Spill
	ldr	r0, [sp, #320]
	str	r0, [sp, #52]                   @ 4-byte Spill
	ldr	r0, [sp, #316]
	str	r0, [sp, #36]                   @ 4-byte Spill
	ldr	r0, [sp, #312]
	str	r0, [sp, #32]                   @ 4-byte Spill
	ldr	r0, [sp, #308]
	str	r0, [sp, #28]                   @ 4-byte Spill
	add	r0, sp, #232
	ldr	r8, [sp, #304]
	bl	mulPv384x32
	add	r2, sp, #232
	ldm	r2, {r0, r1, r2}
	adds	r0, r7, r0
	str	r2, [sp, #44]                   @ 4-byte Spill
	ldr	r0, [sp, #48]                   @ 4-byte Reload
	ldr	r2, [sp, #244]
	adcs	r0, r0, r1
	str	r2, [sp, #40]                   @ 4-byte Spill
	str	r0, [sp, #108]                  @ 4-byte Spill
	mrs	r1, apsr
	str	r1, [sp, #48]                   @ 4-byte Spill
	mul	r2, r6, r0
	ldr	r1, [sp, #56]                   @ 4-byte Reload
	msr	APSR_nzcvq, r1
	ldr	r0, [sp, #72]                   @ 4-byte Reload
	ldr	r1, [sp, #28]                   @ 4-byte Reload
	adcs	r0, r11, r0
	str	r0, [sp, #20]                   @ 4-byte Spill
	ldr	r0, [sp, #276]
	adcs	r5, r10, r8
	str	r0, [sp, #164]                  @ 4-byte Spill
	adcs	r11, r9, r1
	ldr	r0, [sp, #272]
	str	r0, [sp, #72]                   @ 4-byte Spill
	ldr	r1, [sp, #104]                  @ 4-byte Reload
	ldr	r3, [sp, #32]                   @ 4-byte Reload
	ldr	r0, [sp, #268]
	str	r0, [sp, #56]                   @ 4-byte Spill
	adcs	r9, r1, r3
	ldr	r1, [sp, #100]                  @ 4-byte Reload
	ldr	r3, [sp, #36]                   @ 4-byte Reload
	ldr	r0, [sp, #264]
	str	r0, [sp, #24]                   @ 4-byte Spill
	adcs	r10, r1, r3
	ldr	r1, [sp, #96]                   @ 4-byte Reload
	ldr	r3, [sp, #52]                   @ 4-byte Reload
	ldr	r0, [sp, #260]
	str	r0, [sp, #16]                   @ 4-byte Spill
	adcs	r8, r1, r3
	ldr	r1, [sp, #92]                   @ 4-byte Reload
	ldr	r3, [sp, #60]                   @ 4-byte Reload
	ldr	r0, [sp, #256]
	str	r0, [sp, #12]                   @ 4-byte Spill
	adcs	r7, r1, r3
	ldr	r1, [sp, #88]                   @ 4-byte Reload
	ldr	r3, [sp, #64]                   @ 4-byte Reload
	ldr	r0, [sp, #252]
	str	r0, [sp, #8]                    @ 4-byte Spill
	ldr	r6, [r4, #88]
	adcs	r4, r1, r3
	ldr	r1, [sp, #84]                   @ 4-byte Reload
	ldr	r3, [sp, #68]                   @ 4-byte Reload
	ldr	r0, [sp, #248]
	str	r0, [sp, #4]                    @ 4-byte Spill
	adcs	r1, r1, r3
	ldr	r0, [sp, #280]
	str	r1, [sp, #104]                  @ 4-byte Spill
	ldr	r1, [sp, #80]                   @ 4-byte Reload
	ldr	r3, [sp, #76]                   @ 4-byte Reload
	adcs	r1, r3, r1
	str	r1, [sp, #100]                  @ 4-byte Spill
	adc	r0, r0, #0
	ldr	r1, [sp, #168]                  @ 4-byte Reload
	str	r0, [sp, #96]                   @ 4-byte Spill
	add	r0, sp, #176
	bl	mulPv384x32
	ldr	r0, [sp, #48]                   @ 4-byte Reload
	msr	APSR_nzcvq, r0
	ldr	r0, [sp, #44]                   @ 4-byte Reload
	ldr	r1, [sp, #20]                   @ 4-byte Reload
	adcs	r3, r1, r0
	ldr	r0, [sp, #40]                   @ 4-byte Reload
	ldr	r1, [sp, #104]                  @ 4-byte Reload
	adcs	r12, r5, r0
	ldr	r0, [sp, #4]                    @ 4-byte Reload
	adcs	r2, r11, r0
	ldr	r0, [sp, #8]                    @ 4-byte Reload
	adcs	lr, r9, r0
	ldr	r0, [sp, #12]                   @ 4-byte Reload
	adcs	r10, r10, r0
	ldr	r0, [sp, #16]                   @ 4-byte Reload
	adcs	r11, r8, r0
	ldr	r0, [sp, #24]                   @ 4-byte Reload
	adcs	r8, r7, r0
	ldr	r0, [sp, #56]                   @ 4-byte Reload
	adcs	r0, r4, r0
	str	r0, [sp, #76]                   @ 4-byte Spill
	ldr	r0, [sp, #72]                   @ 4-byte Reload
	adcs	r0, r1, r0
	str	r0, [sp, #80]                   @ 4-byte Spill
	ldr	r0, [sp, #164]                  @ 4-byte Reload
	ldr	r1, [sp, #100]                  @ 4-byte Reload
	adcs	r0, r1, r0
	str	r0, [sp, #84]                   @ 4-byte Spill
	ldr	r0, [sp, #96]                   @ 4-byte Reload
	adcs	r0, r6, r0
	str	r0, [sp, #92]                   @ 4-byte Spill
	ldr	r4, [sp, #224]
	adc	r0, r4, #0
	str	r0, [sp, #88]                   @ 4-byte Spill
	ldr	r6, [sp, #176]
	ldr	r4, [sp, #108]                  @ 4-byte Reload
	ldr	r5, [sp, #180]
	adds	r6, r4, r6
	ldr	r0, [sp, #184]
	adcs	r9, r3, r5
	ldr	r1, [sp, #188]
	adcs	r6, r12, r0
	str	r9, [sp, #168]                  @ 4-byte Spill
	adcs	r7, r2, r1
	str	r6, [sp, #164]                  @ 4-byte Spill
	str	r7, [sp, #108]                  @ 4-byte Spill
	ldr	r0, [sp, #192]
	ldr	r1, [sp, #76]                   @ 4-byte Reload
	adcs	r3, lr, r0
	str	r3, [sp, #104]                  @ 4-byte Spill
	ldr	r0, [sp, #196]
	ldr	r4, [sp, #84]                   @ 4-byte Reload
	adcs	r5, r10, r0
	str	r5, [sp, #100]                  @ 4-byte Spill
	ldr	r0, [sp, #200]
	adcs	r12, r11, r0
	str	r12, [sp, #96]                  @ 4-byte Spill
	ldr	r0, [sp, #204]
	adcs	lr, r8, r0
	str	lr, [sp, #72]                   @ 4-byte Spill
	ldr	r0, [sp, #208]
	adcs	r2, r1, r0
	str	r2, [sp, #76]                   @ 4-byte Spill
	ldr	r0, [sp, #212]
	ldr	r1, [sp, #80]                   @ 4-byte Reload
	adcs	r1, r1, r0
	str	r1, [sp, #80]                   @ 4-byte Spill
	ldr	r0, [sp, #216]
	adcs	r10, r4, r0
	ldr	r0, [sp, #220]
	ldr	r4, [sp, #92]                   @ 4-byte Reload
	adcs	r8, r4, r0
	ldr	r0, [sp, #172]                  @ 4-byte Reload
	ldr	r4, [sp, #88]                   @ 4-byte Reload
	ldr	r0, [r0, #92]
	adc	r4, r0, r4
	ldr	r0, [sp, #156]                  @ 4-byte Reload
	subs	r0, r9, r0
	str	r0, [sp, #172]                  @ 4-byte Spill
	ldr	r0, [sp, #152]                  @ 4-byte Reload
	sbcs	r0, r6, r0
	str	r0, [sp, #156]                  @ 4-byte Spill
	ldr	r0, [sp, #148]                  @ 4-byte Reload
	sbcs	r11, r7, r0
	ldr	r0, [sp, #132]                  @ 4-byte Reload
	sbcs	r9, r3, r0
	ldr	r0, [sp, #136]                  @ 4-byte Reload
	sbcs	r7, r5, r0
	ldr	r0, [sp, #140]                  @ 4-byte Reload
	sbcs	r6, r12, r0
	ldr	r0, [sp, #144]                  @ 4-byte Reload
	sbcs	r5, lr, r0
	ldr	r0, [sp, #112]                  @ 4-byte Reload
	sbcs	lr, r2, r0
	ldr	r0, [sp, #116]                  @ 4-byte Reload
	sbcs	r3, r1, r0
	ldr	r0, [sp, #120]                  @ 4-byte Reload
	sbcs	r1, r10, r0
	ldr	r0, [sp, #124]                  @ 4-byte Reload
	sbcs	r2, r8, r0
	ldr	r0, [sp, #128]                  @ 4-byte Reload
	sbc	r0, r4, r0
	asr	r12, r0, #31
	cmn	r12, #1
	movgt	r4, r0
	ldr	r0, [sp, #80]                   @ 4-byte Reload
	movle	r2, r8
	movle	r1, r10
	cmn	r12, #1
	movle	r3, r0
	ldr	r0, [sp, #160]                  @ 4-byte Reload
	str	r3, [r0, #32]
	add	r3, r0, #36
	stm	r3, {r1, r2, r4}
	ldr	r1, [sp, #76]                   @ 4-byte Reload
	ldr	r2, [sp, #164]                  @ 4-byte Reload
	movle	lr, r1
	ldr	r1, [sp, #72]                   @ 4-byte Reload
	str	lr, [r0, #28]
	movle	r5, r1
	ldr	r1, [sp, #96]                   @ 4-byte Reload
	cmn	r12, #1
	str	r5, [r0, #24]
	movle	r6, r1
	ldr	r1, [sp, #100]                  @ 4-byte Reload
	str	r6, [r0, #20]
	movle	r7, r1
	ldr	r1, [sp, #104]                  @ 4-byte Reload
	str	r7, [r0, #16]
	movle	r9, r1
	ldr	r1, [sp, #108]                  @ 4-byte Reload
	cmn	r12, #1
	str	r9, [r0, #12]
	movle	r11, r1
	ldr	r1, [sp, #156]                  @ 4-byte Reload
	movle	r1, r2
	ldr	r2, [sp, #168]                  @ 4-byte Reload
	str	r1, [r0, #4]
	ldr	r1, [sp, #172]                  @ 4-byte Reload
	movle	r1, r2
	str	r11, [r0, #8]
	str	r1, [r0]
	add	sp, sp, #852
	pop	{r4, r5, r6, r7, r8, r9, r10, r11, lr}
	mov	pc, lr
.Lfunc_end63:
	.size	mcl_fp_montRedNF12L, .Lfunc_end63-mcl_fp_montRedNF12L
	.cantunwind
	.fnend
                                        @ -- End function
	.globl	mcl_fp_addPre12L                @ -- Begin function mcl_fp_addPre12L
	.p2align	2
	.type	mcl_fp_addPre12L,%function
	.code	32                              @ @mcl_fp_addPre12L
mcl_fp_addPre12L:
	.fnstart
@ %bb.0:
	.save	{r4, r5, r6, r7, r8, r9, r10, r11, lr}
	push	{r4, r5, r6, r7, r8, r9, r10, r11, lr}
	.pad	#32
	sub	sp, sp, #32
	ldr	r3, [r2, #16]
	add	r10, r1, #32
	str	r3, [sp]                        @ 4-byte Spill
	ldr	r3, [r2, #20]
	ldr	lr, [r2]
	ldm	r1, {r5, r6, r7}
	str	r3, [sp, #4]                    @ 4-byte Spill
	adds	r11, r5, lr
	ldr	r12, [r2, #4]
	ldr	r3, [r2, #24]
	str	r3, [sp, #8]                    @ 4-byte Spill
	adcs	r6, r6, r12
	ldr	r8, [r2, #8]
	add	r12, r1, #16
	ldr	r3, [r2, #28]
	ldr	r5, [r2, #32]
	adcs	r7, r7, r8
	str	r3, [sp, #12]                   @ 4-byte Spill
	str	r5, [sp, #16]                   @ 4-byte Spill
	ldr	r5, [r2, #36]
	ldr	r4, [r2, #12]
	ldr	r3, [r1, #12]
	str	r5, [sp, #20]                   @ 4-byte Spill
	ldr	r5, [r2, #40]
	adcs	lr, r3, r4
	ldr	r2, [r2, #44]
	str	r5, [sp, #24]                   @ 4-byte Spill
	str	r2, [sp, #28]                   @ 4-byte Spill
	ldr	r9, [r1, #44]
	ldm	r12, {r1, r2, r3, r12}
	ldr	r5, [sp]                        @ 4-byte Reload
	str	r11, [r0]
	adcs	r1, r1, r5
	ldr	r5, [sp, #4]                    @ 4-byte Reload
	stmib	r0, {r6, r7, lr}
	add	lr, r0, #16
	adcs	r2, r2, r5
	ldr	r5, [sp, #8]                    @ 4-byte Reload
	add	r0, r0, #32
	ldm	r10, {r4, r8, r10}
	adcs	r3, r3, r5
	ldr	r5, [sp, #12]                   @ 4-byte Reload
	ldr	r7, [sp, #28]                   @ 4-byte Reload
	adcs	r12, r12, r5
	stm	lr, {r1, r2, r3, r12}
	ldr	r1, [sp, #16]                   @ 4-byte Reload
	ldr	r2, [sp, #20]                   @ 4-byte Reload
	adcs	r1, r4, r1
	ldr	r3, [sp, #24]                   @ 4-byte Reload
	adcs	r2, r8, r2
	adcs	r3, r10, r3
	adcs	r7, r9, r7
	stm	r0, {r1, r2, r3, r7}
	mov	r0, #0
	adc	r0, r0, #0
	add	sp, sp, #32
	pop	{r4, r5, r6, r7, r8, r9, r10, r11, lr}
	mov	pc, lr
.Lfunc_end64:
	.size	mcl_fp_addPre12L, .Lfunc_end64-mcl_fp_addPre12L
	.cantunwind
	.fnend
                                        @ -- End function
	.globl	mcl_fp_subPre12L                @ -- Begin function mcl_fp_subPre12L
	.p2align	2
	.type	mcl_fp_subPre12L,%function
	.code	32                              @ @mcl_fp_subPre12L
mcl_fp_subPre12L:
	.fnstart
@ %bb.0:
	.save	{r4, r5, r6, r7, r8, r9, r10, r11, lr}
	push	{r4, r5, r6, r7, r8, r9, r10, r11, lr}
	.pad	#32
	sub	sp, sp, #32
	ldr	r3, [r2, #16]
	add	r10, r1, #32
	str	r3, [sp]                        @ 4-byte Spill
	ldr	r3, [r2, #20]
	ldr	lr, [r2]
	ldm	r1, {r5, r6, r7}
	str	r3, [sp, #4]                    @ 4-byte Spill
	subs	r11, r5, lr
	ldr	r12, [r2, #4]
	ldr	r3, [r2, #24]
	str	r3, [sp, #8]                    @ 4-byte Spill
	sbcs	r6, r6, r12
	ldr	r8, [r2, #8]
	add	r12, r1, #16
	ldr	r3, [r2, #28]
	ldr	r5, [r2, #32]
	sbcs	r7, r7, r8
	str	r3, [sp, #12]                   @ 4-byte Spill
	str	r5, [sp, #16]                   @ 4-byte Spill
	ldr	r5, [r2, #36]
	ldr	r4, [r2, #12]
	ldr	r3, [r1, #12]
	str	r5, [sp, #20]                   @ 4-byte Spill
	ldr	r5, [r2, #40]
	sbcs	lr, r3, r4
	ldr	r2, [r2, #44]
	str	r5, [sp, #24]                   @ 4-byte Spill
	str	r2, [sp, #28]                   @ 4-byte Spill
	ldr	r9, [r1, #44]
	ldm	r12, {r1, r2, r3, r12}
	ldr	r5, [sp]                        @ 4-byte Reload
	str	r11, [r0]
	sbcs	r1, r1, r5
	ldr	r5, [sp, #4]                    @ 4-byte Reload
	stmib	r0, {r6, r7, lr}
	add	lr, r0, #16
	sbcs	r2, r2, r5
	ldr	r5, [sp, #8]                    @ 4-byte Reload
	add	r0, r0, #32
	ldm	r10, {r4, r8, r10}
	sbcs	r3, r3, r5
	ldr	r5, [sp, #12]                   @ 4-byte Reload
	ldr	r7, [sp, #28]                   @ 4-byte Reload
	sbcs	r12, r12, r5
	stm	lr, {r1, r2, r3, r12}
	ldr	r1, [sp, #16]                   @ 4-byte Reload
	ldr	r2, [sp, #20]                   @ 4-byte Reload
	sbcs	r1, r4, r1
	ldr	r3, [sp, #24]                   @ 4-byte Reload
	sbcs	r2, r8, r2
	sbcs	r3, r10, r3
	sbcs	r7, r9, r7
	stm	r0, {r1, r2, r3, r7}
	mov	r0, #0
	sbc	r0, r0, #0
	and	r0, r0, #1
	add	sp, sp, #32
	pop	{r4, r5, r6, r7, r8, r9, r10, r11, lr}
	mov	pc, lr
.Lfunc_end65:
	.size	mcl_fp_subPre12L, .Lfunc_end65-mcl_fp_subPre12L
	.cantunwind
	.fnend
                                        @ -- End function
	.globl	mcl_fp_shr1_12L                 @ -- Begin function mcl_fp_shr1_12L
	.p2align	2
	.type	mcl_fp_shr1_12L,%function
	.code	32                              @ @mcl_fp_shr1_12L
mcl_fp_shr1_12L:
	.fnstart
@ %bb.0:
	.save	{r4, r5, r6, r7, r8, r9, r10, r11, lr}
	push	{r4, r5, r6, r7, r8, r9, r10, r11, lr}
	ldr	r9, [r1, #36]
	add	r6, r1, #20
	ldr	r11, [r1, #40]
	ldr	r5, [r1, #32]
	lsr	r2, r9, #1
	ldr	r10, [r1, #12]
	orr	r2, r2, r11, lsl #31
	str	r2, [r0, #36]
	ldm	r6, {r2, r3, r6}
	lsr	r4, r6, #1
	orr	r4, r4, r5, lsl #31
	str	r4, [r0, #28]
	lsr	r4, r2, #1
	ldr	r7, [r1, #16]
	orr	r4, r4, r3, lsl #31
	str	r4, [r0, #20]
	lsr	r4, r10, #1
	ldr	r8, [r1, #8]
	ldm	r1, {r12, lr}
	orr	r4, r4, r7, lsl #31
	ldr	r1, [r1, #44]
	str	r4, [r0, #12]
	lsr	r4, lr, #1
	orr	r4, r4, r8, lsl #31
	str	r4, [r0, #4]
	lsr	r4, r1, #1
	lsrs	r1, r1, #1
	rrx	r1, r11
	str	r4, [r0, #44]
	str	r1, [r0, #40]
	lsrs	r1, r9, #1
	rrx	r1, r5
	str	r1, [r0, #32]
	lsrs	r1, r6, #1
	rrx	r1, r3
	str	r1, [r0, #24]
	lsrs	r1, r2, #1
	rrx	r1, r7
	str	r1, [r0, #16]
	lsrs	r1, r10, #1
	rrx	r1, r8
	str	r1, [r0, #8]
	lsrs	r1, lr, #1
	rrx	r1, r12
	str	r1, [r0]
	pop	{r4, r5, r6, r7, r8, r9, r10, r11, lr}
	mov	pc, lr
.Lfunc_end66:
	.size	mcl_fp_shr1_12L, .Lfunc_end66-mcl_fp_shr1_12L
	.cantunwind
	.fnend
                                        @ -- End function
	.globl	mcl_fp_add12L                   @ -- Begin function mcl_fp_add12L
	.p2align	2
	.type	mcl_fp_add12L,%function
	.code	32                              @ @mcl_fp_add12L
mcl_fp_add12L:
	.fnstart
@ %bb.0:
	.save	{r4, r5, r6, r7, r8, r9, r10, r11, lr}
	push	{r4, r5, r6, r7, r8, r9, r10, r11, lr}
	.pad	#40
	sub	sp, sp, #40
	ldm	r2, {r7, r8, lr}
	ldr	r6, [r1]
	ldmib	r1, {r4, r5, r9}
	adds	r11, r6, r7
	adcs	r4, r4, r8
	ldr	r12, [r2, #12]
	adcs	r10, r5, lr
	ldr	r5, [r2, #16]
	ldr	r6, [r1, #16]
	adcs	r8, r9, r12
	str	r4, [sp, #24]                   @ 4-byte Spill
	add	lr, r2, #32
	adcs	r7, r6, r5
	ldr	r4, [r2, #20]
	ldr	r5, [r1, #20]
	str	r7, [sp, #32]                   @ 4-byte Spill
	adcs	r7, r5, r4
	ldr	r4, [r2, #24]
	ldr	r5, [r1, #24]
	str	r7, [sp, #28]                   @ 4-byte Spill
	adcs	r7, r5, r4
	ldr	r4, [r2, #28]
	ldr	r5, [r1, #28]
	str	r7, [sp, #20]                   @ 4-byte Spill
	adcs	r7, r5, r4
	str	r7, [sp, #16]                   @ 4-byte Spill
	ldm	lr, {r4, r5, r7, lr}
	ldr	r6, [r1, #32]
	ldr	r2, [r1, #36]
	adcs	r9, r6, r4
	ldr	r12, [r1, #40]
	adcs	r6, r2, r5
	ldr	r1, [r1, #44]
	adcs	r4, r12, r7
	str	r11, [r0]
	adcs	r5, r1, lr
	mov	r1, #0
	adc	r1, r1, #0
	str	r1, [sp, #36]                   @ 4-byte Spill
	ldm	r3, {r1, r2, r7, r12}
	subs	r1, r11, r1
	str	r1, [sp, #12]                   @ 4-byte Spill
	ldr	r1, [sp, #24]                   @ 4-byte Reload
	sbcs	r2, r1, r2
	str	r2, [sp, #8]                    @ 4-byte Spill
	sbcs	r2, r10, r7
	str	r2, [sp, #4]                    @ 4-byte Spill
	sbcs	r2, r8, r12
	stmib	r0, {r1, r10}
	str	r2, [sp]                        @ 4-byte Spill
	ldr	r2, [sp, #32]                   @ 4-byte Reload
	ldr	r1, [r3, #16]
	ldr	r7, [sp, #28]                   @ 4-byte Reload
	sbcs	r12, r2, r1
	ldr	r1, [r3, #20]
	str	r8, [r0, #12]
	ldr	r8, [sp, #20]                   @ 4-byte Reload
	sbcs	lr, r7, r1
	ldr	r1, [r3, #24]
	str	r7, [r0, #20]
	add	r7, r3, #32
	str	r8, [r0, #24]
	sbcs	r8, r8, r1
	ldr	r10, [sp, #16]                  @ 4-byte Reload
	ldr	r1, [r3, #28]
	str	r10, [r0, #28]
	str	r2, [r0, #16]
	sbcs	r10, r10, r1
	ldm	r7, {r1, r2, r7}
	sbcs	r1, r9, r1
	ldr	r3, [r3, #44]
	sbcs	r2, r6, r2
	str	r6, [r0, #36]
	sbcs	r6, r4, r7
	str	r4, [r0, #40]
	sbcs	r4, r5, r3
	ldr	r3, [sp, #36]                   @ 4-byte Reload
	str	r9, [r0, #32]
	sbc	r3, r3, #0
	str	r5, [r0, #44]
	tst	r3, #1
	bne	.LBB67_2
@ %bb.1:                                @ %nocarry
	ldr	r3, [sp, #12]                   @ 4-byte Reload
	add	r5, r0, #12
	str	r3, [r0]
	ldr	r3, [sp, #8]                    @ 4-byte Reload
	str	r3, [r0, #4]
	ldr	r3, [sp, #4]                    @ 4-byte Reload
	str	r3, [r0, #8]
	ldr	r3, [sp]                        @ 4-byte Reload
	stm	r5, {r3, r12, lr}
	add	r3, r0, #32
	str	r8, [r0, #24]
	str	r10, [r0, #28]
	stm	r3, {r1, r2, r6}
	str	r4, [r0, #44]
.LBB67_2:                               @ %carry
	add	sp, sp, #40
	pop	{r4, r5, r6, r7, r8, r9, r10, r11, lr}
	mov	pc, lr
.Lfunc_end67:
	.size	mcl_fp_add12L, .Lfunc_end67-mcl_fp_add12L
	.cantunwind
	.fnend
                                        @ -- End function
	.globl	mcl_fp_addNF12L                 @ -- Begin function mcl_fp_addNF12L
	.p2align	2
	.type	mcl_fp_addNF12L,%function
	.code	32                              @ @mcl_fp_addNF12L
mcl_fp_addNF12L:
	.fnstart
@ %bb.0:
	.save	{r4, r5, r6, r7, r8, r9, r10, r11, lr}
	push	{r4, r5, r6, r7, r8, r9, r10, r11, lr}
	.pad	#60
	sub	sp, sp, #60
	ldr	r9, [r1]
	ldm	r2, {r4, r5, r6, r7}
	ldmib	r1, {r8, lr}
	adds	r4, r4, r9
	str	r4, [sp, #56]                   @ 4-byte Spill
	adcs	r4, r5, r8
	ldr	r12, [r1, #12]
	adcs	r6, r6, lr
	str	r6, [sp, #48]                   @ 4-byte Spill
	add	lr, r1, #32
	adcs	r7, r7, r12
	str	r7, [sp, #44]                   @ 4-byte Spill
	ldr	r6, [r1, #16]
	ldr	r7, [r2, #16]
	str	r4, [sp, #52]                   @ 4-byte Spill
	adcs	r7, r7, r6
	str	r7, [sp, #40]                   @ 4-byte Spill
	ldr	r4, [r1, #20]
	ldr	r7, [r2, #20]
	ldr	r6, [r2, #24]
	adcs	r7, r7, r4
	str	r7, [sp, #36]                   @ 4-byte Spill
	ldr	r7, [r1, #24]
	ldr	r4, [r2, #32]
	adcs	r7, r6, r7
	str	r7, [sp, #32]                   @ 4-byte Spill
	ldr	r7, [r1, #28]
	ldr	r6, [r2, #28]
	ldr	r5, [r1, #44]
	adcs	r7, r6, r7
	str	r7, [sp, #28]                   @ 4-byte Spill
	ldm	lr, {r7, r12, lr}
	ldr	r6, [r2, #36]
	adcs	r4, r4, r7
	ldr	r1, [r2, #40]
	adcs	r7, r6, r12
	ldr	r2, [r2, #44]
	adcs	r1, r1, lr
	str	r4, [sp, #8]                    @ 4-byte Spill
	adc	r6, r2, r5
	ldr	r2, [r3]
	ldr	r4, [sp, #56]                   @ 4-byte Reload
	add	lr, r3, #36
	str	r7, [sp, #4]                    @ 4-byte Spill
	subs	r2, r4, r2
	str	r1, [sp]                        @ 4-byte Spill
	str	r2, [sp, #24]                   @ 4-byte Spill
	ldmib	r3, {r1, r7, r12}
	ldr	r2, [sp, #52]                   @ 4-byte Reload
	ldr	r9, [r3, #16]
	sbcs	r2, r2, r1
	str	r2, [sp, #20]                   @ 4-byte Spill
	ldr	r2, [sp, #48]                   @ 4-byte Reload
	ldr	r10, [r3, #20]
	sbcs	r2, r2, r7
	str	r2, [sp, #16]                   @ 4-byte Spill
	ldr	r2, [sp, #44]                   @ 4-byte Reload
	ldr	r5, [r3, #24]
	ldr	r8, [r3, #28]
	sbcs	r2, r2, r12
	ldr	r7, [r3, #32]
	ldr	r3, [sp, #40]                   @ 4-byte Reload
	ldr	r1, [sp, #32]                   @ 4-byte Reload
	sbcs	r11, r3, r9
	ldr	r3, [sp, #36]                   @ 4-byte Reload
	ldr	r9, [sp, #8]                    @ 4-byte Reload
	sbcs	r10, r3, r10
	str	r2, [sp, #12]                   @ 4-byte Spill
	sbcs	r5, r1, r5
	ldr	r1, [sp, #28]                   @ 4-byte Reload
	ldm	lr, {r2, r12, lr}
	sbcs	r8, r1, r8
	ldr	r4, [sp, #4]                    @ 4-byte Reload
	sbcs	r7, r9, r7
	ldr	r1, [sp]                        @ 4-byte Reload
	sbcs	r2, r4, r2
	sbcs	r12, r1, r12
	sbc	r3, r6, lr
	asr	lr, r3, #31
	cmn	lr, #1
	movle	r12, r1
	ldr	r1, [sp, #28]                   @ 4-byte Reload
	movgt	r6, r3
	movle	r2, r4
	cmn	lr, #1
	str	r2, [r0, #36]
	movle	r8, r1
	ldr	r1, [sp, #32]                   @ 4-byte Reload
	movle	r7, r9
	ldr	r2, [sp, #12]                   @ 4-byte Reload
	str	r7, [r0, #32]
	movle	r5, r1
	ldr	r1, [sp, #36]                   @ 4-byte Reload
	cmn	lr, #1
	str	r12, [r0, #40]
	str	r6, [r0, #44]
	movle	r10, r1
	ldr	r1, [sp, #40]                   @ 4-byte Reload
	str	r8, [r0, #28]
	str	r5, [r0, #24]
	movle	r11, r1
	ldr	r1, [sp, #44]                   @ 4-byte Reload
	str	r10, [r0, #20]
	str	r11, [r0, #16]
	movle	r2, r1
	ldr	r1, [sp, #48]                   @ 4-byte Reload
	str	r2, [r0, #12]
	cmn	lr, #1
	ldr	r2, [sp, #16]                   @ 4-byte Reload
	movle	r2, r1
	ldr	r1, [sp, #52]                   @ 4-byte Reload
	str	r2, [r0, #8]
	ldr	r2, [sp, #20]                   @ 4-byte Reload
	movle	r2, r1
	ldr	r1, [sp, #56]                   @ 4-byte Reload
	str	r2, [r0, #4]
	ldr	r2, [sp, #24]                   @ 4-byte Reload
	movle	r2, r1
	str	r2, [r0]
	add	sp, sp, #60
	pop	{r4, r5, r6, r7, r8, r9, r10, r11, lr}
	mov	pc, lr
.Lfunc_end68:
	.size	mcl_fp_addNF12L, .Lfunc_end68-mcl_fp_addNF12L
	.cantunwind
	.fnend
                                        @ -- End function
	.globl	mcl_fp_sub12L                   @ -- Begin function mcl_fp_sub12L
	.p2align	2
	.type	mcl_fp_sub12L,%function
	.code	32                              @ @mcl_fp_sub12L
mcl_fp_sub12L:
	.fnstart
@ %bb.0:
	.save	{r4, r5, r6, r7, r8, r9, r10, r11, lr}
	push	{r4, r5, r6, r7, r8, r9, r10, r11, lr}
	.pad	#40
	sub	sp, sp, #40
	ldr	r12, [r2]
	mov	r11, r3
	ldr	r5, [r1]
	ldr	r9, [r2, #4]
	ldmib	r1, {r4, r6, r7}
	subs	r3, r5, r12
	ldr	r8, [r2, #8]
	sbcs	r10, r4, r9
	ldr	lr, [r2, #12]
	add	r9, r1, #32
	str	r3, [sp, #36]                   @ 4-byte Spill
	sbcs	r3, r6, r8
	sbcs	r12, r7, lr
	ldr	r6, [r2, #16]
	ldr	r7, [r1, #16]
	add	lr, r2, #32
	str	r3, [sp, #16]                   @ 4-byte Spill
	sbcs	r3, r7, r6
	ldr	r4, [r2, #20]
	ldr	r7, [r1, #20]
	str	r3, [sp, #20]                   @ 4-byte Spill
	sbcs	r3, r7, r4
	ldr	r6, [r2, #24]
	ldr	r4, [r1, #24]
	str	r3, [sp, #32]                   @ 4-byte Spill
	sbcs	r3, r4, r6
	ldr	r4, [r2, #28]
	ldr	r6, [r1, #28]
	str	r3, [sp, #24]                   @ 4-byte Spill
	sbcs	r3, r6, r4
	str	r3, [sp, #28]                   @ 4-byte Spill
	ldm	lr, {r4, r6, lr}
	ldm	r9, {r3, r8, r9}
	sbcs	r3, r3, r4
	ldr	r5, [r1, #44]
	sbcs	r8, r8, r6
	ldr	r1, [sp, #16]                   @ 4-byte Reload
	ldr	r7, [r2, #44]
	sbcs	lr, r9, lr
	str	r1, [r0, #8]
	ldr	r1, [sp, #20]                   @ 4-byte Reload
	sbcs	r4, r5, r7
	mov	r7, #0
	str	r1, [r0, #16]
	ldr	r1, [sp, #32]                   @ 4-byte Reload
	sbc	r7, r7, #0
	str	r1, [r0, #20]
	tst	r7, #1
	ldr	r1, [sp, #24]                   @ 4-byte Reload
	ldr	r5, [sp, #36]                   @ 4-byte Reload
	str	r1, [r0, #24]
	ldr	r1, [sp, #28]                   @ 4-byte Reload
	str	r5, [r0]
	str	r10, [sp, #12]                  @ 4-byte Spill
	str	r10, [r0, #4]
	str	r12, [sp, #8]                   @ 4-byte Spill
	str	r12, [r0, #12]
	str	r1, [r0, #28]
	str	r3, [sp, #4]                    @ 4-byte Spill
	str	r3, [r0, #32]
	str	r8, [sp]                        @ 4-byte Spill
	str	r8, [r0, #36]
	str	lr, [r0, #40]
	str	r4, [r0, #44]
	beq	.LBB69_2
@ %bb.1:                                @ %carry
	ldr	r7, [r11]
	ldr	r12, [sp, #36]                  @ 4-byte Reload
	ldmib	r11, {r1, r10}
	adds	r7, r7, r12
	str	r7, [r0]
	ldr	r7, [sp, #12]                   @ 4-byte Reload
	ldr	r2, [r11, #12]
	adcs	r7, r1, r7
	ldr	r1, [sp, #16]                   @ 4-byte Reload
	str	r7, [r0, #4]
	adcs	r7, r10, r1
	str	r7, [r0, #8]
	ldr	r7, [sp, #8]                    @ 4-byte Reload
	ldr	r1, [sp, #20]                   @ 4-byte Reload
	adcs	r7, r2, r7
	str	r7, [r0, #12]
	ldr	r7, [r11, #16]
	ldr	r2, [sp, #4]                    @ 4-byte Reload
	adcs	r7, r7, r1
	str	r7, [r0, #16]
	ldr	r7, [r11, #20]
	ldr	r1, [sp, #32]                   @ 4-byte Reload
	ldr	r6, [r11, #36]
	adcs	r7, r7, r1
	str	r7, [r0, #20]
	ldr	r7, [r11, #24]
	ldr	r1, [sp, #24]                   @ 4-byte Reload
	ldr	r3, [r11, #44]
	adcs	r7, r7, r1
	str	r7, [r0, #24]
	ldr	r7, [r11, #28]
	ldr	r1, [sp, #28]                   @ 4-byte Reload
	adcs	r7, r7, r1
	str	r7, [r0, #28]
	ldr	r7, [r11, #32]
	ldr	r1, [r11, #40]
	adcs	r7, r7, r2
	ldr	r2, [sp]                        @ 4-byte Reload
	str	r7, [r0, #32]
	adcs	r2, r6, r2
	str	r2, [r0, #36]
	adcs	r1, r1, lr
	str	r1, [r0, #40]
	adc	r3, r3, r4
	str	r3, [r0, #44]
.LBB69_2:                               @ %nocarry
	add	sp, sp, #40
	pop	{r4, r5, r6, r7, r8, r9, r10, r11, lr}
	mov	pc, lr
.Lfunc_end69:
	.size	mcl_fp_sub12L, .Lfunc_end69-mcl_fp_sub12L
	.cantunwind
	.fnend
                                        @ -- End function
	.globl	mcl_fp_subNF12L                 @ -- Begin function mcl_fp_subNF12L
	.p2align	2
	.type	mcl_fp_subNF12L,%function
	.code	32                              @ @mcl_fp_subNF12L
mcl_fp_subNF12L:
	.fnstart
@ %bb.0:
	.save	{r4, r5, r6, r7, r8, r9, r10, r11, lr}
	push	{r4, r5, r6, r7, r8, r9, r10, r11, lr}
	.pad	#64
	sub	sp, sp, #64
	ldr	r9, [r2]
	add	r10, r3, #8
	ldm	r1, {r4, r5, r6, r7}
	ldmib	r2, {r8, lr}
	subs	r4, r4, r9
	str	r4, [sp, #60]                   @ 4-byte Spill
	sbcs	r4, r5, r8
	ldr	r12, [r2, #12]
	sbcs	r6, r6, lr
	str	r6, [sp, #52]                   @ 4-byte Spill
	sbcs	r7, r7, r12
	str	r7, [sp, #48]                   @ 4-byte Spill
	ldr	r6, [r2, #16]
	ldr	r7, [r1, #16]
	str	r4, [sp, #56]                   @ 4-byte Spill
	sbcs	r7, r7, r6
	str	r7, [sp, #44]                   @ 4-byte Spill
	ldr	r4, [r2, #20]
	ldr	r7, [r1, #20]
	ldr	r6, [r1, #24]
	sbcs	r7, r7, r4
	str	r7, [sp, #40]                   @ 4-byte Spill
	ldr	r7, [r2, #24]
	ldr	lr, [r2, #32]
	sbcs	r7, r6, r7
	str	r7, [sp, #36]                   @ 4-byte Spill
	ldr	r7, [r2, #28]
	ldr	r6, [r1, #28]
	ldr	r5, [r2, #40]
	sbcs	r7, r6, r7
	str	r7, [sp, #32]                   @ 4-byte Spill
	ldr	r7, [r1, #32]
	ldr	r6, [r2, #36]
	ldr	r4, [r2, #44]
	sbcs	r7, r7, lr
	ldr	r2, [r1, #36]
	add	lr, r3, #36
	ldr	r12, [r1, #40]
	sbcs	r2, r2, r6
	ldr	r1, [r1, #44]
	str	r2, [sp, #8]                    @ 4-byte Spill
	sbcs	r2, r12, r5
	ldr	r5, [r3, #24]
	sbc	r9, r1, r4
	str	r2, [sp, #4]                    @ 4-byte Spill
	str	r5, [sp]                        @ 4-byte Spill
	ldm	r3, {r2, r4}
	ldr	r5, [sp, #60]                   @ 4-byte Reload
	str	r7, [sp, #12]                   @ 4-byte Spill
	adds	r2, r5, r2
	str	r2, [sp, #28]                   @ 4-byte Spill
	ldr	r2, [sp, #56]                   @ 4-byte Reload
	ldm	r10, {r1, r6, r7, r10}
	adcs	r2, r2, r4
	str	r2, [sp, #24]                   @ 4-byte Spill
	ldr	r2, [sp, #52]                   @ 4-byte Reload
	ldr	r8, [r3, #28]
	adcs	r2, r2, r1
	str	r2, [sp, #20]                   @ 4-byte Spill
	ldr	r2, [sp, #48]                   @ 4-byte Reload
	ldr	r4, [r3, #32]
	ldr	r3, [sp, #44]                   @ 4-byte Reload
	adcs	r1, r2, r6
	str	r1, [sp, #16]                   @ 4-byte Spill
	adcs	r11, r3, r7
	ldr	r3, [sp, #40]                   @ 4-byte Reload
	ldr	r1, [sp, #36]                   @ 4-byte Reload
	adcs	r10, r3, r10
	ldr	r3, [sp]                        @ 4-byte Reload
	ldr	r7, [sp, #12]                   @ 4-byte Reload
	adcs	r6, r1, r3
	ldr	r1, [sp, #32]                   @ 4-byte Reload
	ldm	lr, {r2, r12, lr}
	adcs	r8, r1, r8
	ldr	r5, [sp, #8]                    @ 4-byte Reload
	adcs	r4, r7, r4
	ldr	r1, [sp, #4]                    @ 4-byte Reload
	adcs	r2, r5, r2
	adcs	r3, r1, r12
	adc	r12, r9, lr
	asr	lr, r9, #31
	cmp	lr, #0
	movpl	r3, r1
	movpl	r12, r9
	movpl	r2, r5
	cmp	lr, #0
	movpl	r4, r7
	add	r1, r0, #36
	str	r4, [r0, #32]
	stm	r1, {r2, r3, r12}
	ldr	r1, [sp, #32]                   @ 4-byte Reload
	ldr	r2, [sp, #16]                   @ 4-byte Reload
	movpl	r8, r1
	ldr	r1, [sp, #36]                   @ 4-byte Reload
	str	r8, [r0, #28]
	movpl	r6, r1
	ldr	r1, [sp, #40]                   @ 4-byte Reload
	cmp	lr, #0
	str	r6, [r0, #24]
	movpl	r10, r1
	ldr	r1, [sp, #44]                   @ 4-byte Reload
	str	r10, [r0, #20]
	movpl	r11, r1
	ldr	r1, [sp, #48]                   @ 4-byte Reload
	str	r11, [r0, #16]
	movpl	r2, r1
	ldr	r1, [sp, #52]                   @ 4-byte Reload
	str	r2, [r0, #12]
	cmp	lr, #0
	ldr	r2, [sp, #20]                   @ 4-byte Reload
	movpl	r2, r1
	ldr	r1, [sp, #56]                   @ 4-byte Reload
	str	r2, [r0, #8]
	ldr	r2, [sp, #24]                   @ 4-byte Reload
	movpl	r2, r1
	ldr	r1, [sp, #60]                   @ 4-byte Reload
	str	r2, [r0, #4]
	ldr	r2, [sp, #28]                   @ 4-byte Reload
	movpl	r2, r1
	str	r2, [r0]
	add	sp, sp, #64
	pop	{r4, r5, r6, r7, r8, r9, r10, r11, lr}
	mov	pc, lr
.Lfunc_end70:
	.size	mcl_fp_subNF12L, .Lfunc_end70-mcl_fp_subNF12L
	.cantunwind
	.fnend
                                        @ -- End function
	.globl	mcl_fpDbl_add12L                @ -- Begin function mcl_fpDbl_add12L
	.p2align	2
	.type	mcl_fpDbl_add12L,%function
	.code	32                              @ @mcl_fpDbl_add12L
mcl_fpDbl_add12L:
	.fnstart
@ %bb.0:
	.save	{r4, r5, r6, r7, r8, r9, r10, r11, lr}
	push	{r4, r5, r6, r7, r8, r9, r10, r11, lr}
	.pad	#116
	sub	sp, sp, #116
	ldr	r9, [r2]
	ldm	r1, {r4, r5, r6, r7}
	ldmib	r2, {r8, lr}
	adds	r4, r4, r9
	str	r4, [sp, #92]                   @ 4-byte Spill
	adcs	r4, r5, r8
	ldr	r12, [r2, #12]
	adcs	r6, r6, lr
	str	r6, [sp, #84]                   @ 4-byte Spill
	adcs	r7, r7, r12
	str	r7, [sp, #80]                   @ 4-byte Spill
	ldr	r6, [r2, #16]
	ldr	r7, [r1, #16]
	str	r4, [sp, #88]                   @ 4-byte Spill
	adcs	r7, r7, r6
	str	r7, [sp, #76]                   @ 4-byte Spill
	ldr	r4, [r2, #20]
	ldr	r7, [r1, #20]
	ldr	r6, [r1, #24]
	adcs	r7, r7, r4
	str	r7, [sp, #64]                   @ 4-byte Spill
	ldr	r7, [r2, #24]
	ldr	r5, [r1, #84]
	adcs	r7, r6, r7
	str	r7, [sp, #60]                   @ 4-byte Spill
	ldr	r7, [r2, #28]
	ldr	r6, [r1, #28]
	ldr	r4, [r1, #88]
	adcs	r7, r6, r7
	str	r7, [sp, #32]                   @ 4-byte Spill
	ldr	r7, [r2, #32]
	ldr	r6, [r1, #32]
	adcs	r7, r6, r7
	str	r7, [sp, #48]                   @ 4-byte Spill
	ldr	r7, [r2, #36]
	ldr	r6, [r1, #36]
	adcs	r7, r6, r7
	str	r7, [sp, #36]                   @ 4-byte Spill
	ldr	r7, [r2, #40]
	ldr	r6, [r1, #40]
	adcs	r7, r6, r7
	str	r7, [sp, #28]                   @ 4-byte Spill
	ldr	r7, [r2, #44]
	ldr	r6, [r1, #44]
	adcs	r7, r6, r7
	str	r7, [sp, #24]                   @ 4-byte Spill
	ldr	r7, [r2, #48]
	ldr	r6, [r1, #48]
	adcs	r9, r6, r7
	ldr	r7, [r2, #52]
	ldr	r6, [r1, #52]
	str	r9, [sp, #72]                   @ 4-byte Spill
	adcs	r10, r6, r7
	ldr	r7, [r2, #56]
	ldr	r6, [r1, #56]
	str	r10, [sp, #56]                  @ 4-byte Spill
	adcs	r8, r6, r7
	ldr	r7, [r2, #60]
	ldr	r6, [r1, #60]
	str	r8, [sp, #44]                   @ 4-byte Spill
	adcs	r7, r6, r7
	str	r7, [sp, #112]                  @ 4-byte Spill
	ldr	r7, [r2, #64]
	ldr	r6, [r1, #64]
	adcs	r7, r6, r7
	str	r7, [sp, #108]                  @ 4-byte Spill
	ldr	r7, [r2, #68]
	ldr	r6, [r1, #68]
	adcs	r7, r6, r7
	str	r7, [sp, #104]                  @ 4-byte Spill
	ldr	r7, [r2, #72]
	ldr	r6, [r1, #72]
	adcs	r7, r6, r7
	str	r7, [sp, #100]                  @ 4-byte Spill
	ldr	r7, [r2, #76]
	ldr	r6, [r1, #76]
	adcs	r7, r6, r7
	str	r7, [sp, #96]                   @ 4-byte Spill
	ldr	r6, [r1, #80]
	ldr	r7, [r2, #80]
	ldr	r1, [r1, #92]
	adcs	r11, r6, r7
	ldr	r6, [r2, #84]
	str	r11, [sp]                       @ 4-byte Spill
	adcs	r7, r5, r6
	ldr	r5, [r2, #88]
	ldr	r2, [r2, #92]
	str	r7, [sp, #16]                   @ 4-byte Spill
	adcs	r7, r4, r5
	adcs	r1, r1, r2
	str	r1, [sp, #8]                    @ 4-byte Spill
	mov	r1, #0
	str	r7, [sp, #12]                   @ 4-byte Spill
	adc	r1, r1, #0
	str	r1, [sp, #4]                    @ 4-byte Spill
	ldm	r3, {r1, r2, r12, lr}
	subs	r1, r9, r1
	str	r1, [sp, #68]                   @ 4-byte Spill
	sbcs	r1, r10, r2
	str	r1, [sp, #52]                   @ 4-byte Spill
	sbcs	r1, r8, r12
	str	r1, [sp, #40]                   @ 4-byte Spill
	ldr	r1, [sp, #112]                  @ 4-byte Reload
	add	r12, r3, #20
	ldr	r5, [r3, #32]
	sbcs	r1, r1, lr
	str	r1, [sp, #20]                   @ 4-byte Spill
	ldr	r1, [sp, #92]                   @ 4-byte Reload
	add	lr, r3, #36
	str	r1, [r0]
	ldr	r1, [sp, #88]                   @ 4-byte Reload
	str	r1, [r0, #4]
	ldr	r1, [sp, #84]                   @ 4-byte Reload
	str	r1, [r0, #8]
	ldr	r1, [sp, #80]                   @ 4-byte Reload
	str	r1, [r0, #12]
	ldr	r1, [sp, #76]                   @ 4-byte Reload
	str	r1, [r0, #16]
	ldr	r1, [sp, #64]                   @ 4-byte Reload
	str	r1, [r0, #20]
	ldr	r1, [sp, #60]                   @ 4-byte Reload
	str	r1, [r0, #24]
	ldr	r1, [sp, #32]                   @ 4-byte Reload
	str	r1, [r0, #28]
	ldr	r1, [sp, #48]                   @ 4-byte Reload
	str	r1, [r0, #32]
	ldr	r1, [sp, #36]                   @ 4-byte Reload
	str	r1, [r0, #36]
	ldr	r1, [sp, #28]                   @ 4-byte Reload
	ldr	r3, [r3, #16]
	ldr	r7, [sp, #108]                  @ 4-byte Reload
	str	r1, [r0, #40]
	ldr	r1, [sp, #24]                   @ 4-byte Reload
	sbcs	r10, r7, r3
	str	r1, [r0, #44]
	ldm	r12, {r1, r2, r12}
	ldr	r3, [sp, #104]                  @ 4-byte Reload
	ldm	lr, {r4, r6, lr}
	sbcs	r9, r3, r1
	ldr	r1, [sp, #100]                  @ 4-byte Reload
	ldr	r3, [sp, #16]                   @ 4-byte Reload
	sbcs	r8, r1, r2
	ldr	r1, [sp, #96]                   @ 4-byte Reload
	ldr	r2, [sp, #12]                   @ 4-byte Reload
	sbcs	r12, r1, r12
	ldr	r1, [sp, #8]                    @ 4-byte Reload
	sbcs	r5, r11, r5
	ldr	r7, [sp, #4]                    @ 4-byte Reload
	sbcs	r4, r3, r4
	sbcs	r6, r2, r6
	sbcs	lr, r1, lr
	sbc	r11, r7, #0
	ands	r11, r11, #1
	movne	lr, r1
	ldr	r1, [sp]                        @ 4-byte Reload
	movne	r6, r2
	movne	r4, r3
	cmp	r11, #0
	ldr	r2, [sp, #20]                   @ 4-byte Reload
	movne	r5, r1
	ldr	r1, [sp, #96]                   @ 4-byte Reload
	str	lr, [r0, #92]
	str	r6, [r0, #88]
	movne	r12, r1
	ldr	r1, [sp, #100]                  @ 4-byte Reload
	str	r4, [r0, #84]
	str	r5, [r0, #80]
	movne	r8, r1
	ldr	r1, [sp, #104]                  @ 4-byte Reload
	cmp	r11, #0
	str	r12, [r0, #76]
	str	r8, [r0, #72]
	movne	r9, r1
	ldr	r1, [sp, #108]                  @ 4-byte Reload
	str	r9, [r0, #68]
	movne	r10, r1
	ldr	r1, [sp, #112]                  @ 4-byte Reload
	str	r10, [r0, #64]
	movne	r2, r1
	cmp	r11, #0
	str	r2, [r0, #60]
	ldr	r2, [sp, #44]                   @ 4-byte Reload
	ldr	r1, [sp, #40]                   @ 4-byte Reload
	movne	r1, r2
	ldr	r2, [sp, #56]                   @ 4-byte Reload
	str	r1, [r0, #56]
	ldr	r1, [sp, #52]                   @ 4-byte Reload
	movne	r1, r2
	ldr	r2, [sp, #72]                   @ 4-byte Reload
	str	r1, [r0, #52]
	ldr	r1, [sp, #68]                   @ 4-byte Reload
	movne	r1, r2
	str	r1, [r0, #48]
	add	sp, sp, #116
	pop	{r4, r5, r6, r7, r8, r9, r10, r11, lr}
	mov	pc, lr
.Lfunc_end71:
	.size	mcl_fpDbl_add12L, .Lfunc_end71-mcl_fpDbl_add12L
	.cantunwind
	.fnend
                                        @ -- End function
	.globl	mcl_fpDbl_sub12L                @ -- Begin function mcl_fpDbl_sub12L
	.p2align	2
	.type	mcl_fpDbl_sub12L,%function
	.code	32                              @ @mcl_fpDbl_sub12L
mcl_fpDbl_sub12L:
	.fnstart
@ %bb.0:
	.save	{r4, r5, r6, r7, r8, r9, r10, r11, lr}
	push	{r4, r5, r6, r7, r8, r9, r10, r11, lr}
	.pad	#108
	sub	sp, sp, #108
	ldr	r7, [r2, #84]
	add	r9, r1, #12
	str	r7, [sp, #84]                   @ 4-byte Spill
	ldr	r7, [r2, #80]
	str	r7, [sp, #76]                   @ 4-byte Spill
	ldr	r7, [r2, #76]
	str	r7, [sp, #92]                   @ 4-byte Spill
	ldr	r7, [r2, #72]
	str	r7, [sp, #96]                   @ 4-byte Spill
	ldr	r7, [r2, #68]
	str	r7, [sp, #100]                  @ 4-byte Spill
	ldr	r7, [r2, #64]
	str	r7, [sp, #104]                  @ 4-byte Spill
	ldr	r7, [r2, #32]
	str	r7, [sp, #32]                   @ 4-byte Spill
	ldr	r7, [r2, #36]
	str	r7, [sp, #28]                   @ 4-byte Spill
	ldr	r7, [r2, #40]
	str	r7, [sp, #60]                   @ 4-byte Spill
	ldr	r7, [r2, #44]
	str	r7, [sp, #64]                   @ 4-byte Spill
	ldr	r7, [r2, #48]
	str	r7, [sp, #88]                   @ 4-byte Spill
	ldr	r7, [r2, #52]
	str	r7, [sp, #80]                   @ 4-byte Spill
	ldr	r7, [r2, #56]
	str	r7, [sp, #72]                   @ 4-byte Spill
	ldr	r7, [r2, #60]
	str	r7, [sp, #68]                   @ 4-byte Spill
	ldr	r7, [r2, #8]
	ldm	r2, {r4, r11}
	ldm	r1, {r12, lr}
	str	r7, [sp, #56]                   @ 4-byte Spill
	subs	r4, r12, r4
	ldr	r7, [r2, #12]
	sbcs	r11, lr, r11
	str	r7, [sp, #52]                   @ 4-byte Spill
	add	lr, r1, #44
	ldr	r7, [r2, #16]
	str	r7, [sp, #48]                   @ 4-byte Spill
	ldr	r7, [r2, #20]
	str	r4, [sp, #8]                    @ 4-byte Spill
	ldr	r10, [r1, #8]
	ldr	r4, [sp, #56]                   @ 4-byte Reload
	str	r7, [sp, #44]                   @ 4-byte Spill
	ldr	r7, [r2, #24]
	sbcs	r4, r10, r4
	str	r7, [sp, #40]                   @ 4-byte Spill
	ldr	r7, [r2, #28]
	str	r7, [sp, #36]                   @ 4-byte Spill
	str	r4, [sp, #56]                   @ 4-byte Spill
	ldm	r9, {r5, r6, r7, r8, r9}
	ldr	r4, [sp, #52]                   @ 4-byte Reload
	ldr	r10, [sp, #32]                  @ 4-byte Reload
	sbcs	r5, r5, r4
	str	r5, [sp, #52]                   @ 4-byte Spill
	ldr	r5, [sp, #48]                   @ 4-byte Reload
	ldm	lr, {r4, r12, lr}
	sbcs	r6, r6, r5
	str	r6, [sp, #48]                   @ 4-byte Spill
	ldr	r6, [sp, #44]                   @ 4-byte Reload
	ldr	r5, [r1, #40]
	sbcs	r7, r7, r6
	str	r7, [sp, #44]                   @ 4-byte Spill
	ldr	r7, [sp, #40]                   @ 4-byte Reload
	ldr	r6, [r1, #36]
	sbcs	r7, r8, r7
	str	r7, [sp, #40]                   @ 4-byte Spill
	ldr	r7, [sp, #36]                   @ 4-byte Reload
	ldr	r8, [r1, #56]
	sbcs	r7, r9, r7
	str	r7, [sp, #36]                   @ 4-byte Spill
	ldr	r7, [r1, #32]
	ldr	r9, [r1, #60]
	sbcs	r7, r7, r10
	str	r7, [sp, #32]                   @ 4-byte Spill
	ldr	r7, [sp, #28]                   @ 4-byte Reload
	sbcs	r6, r6, r7
	str	r6, [sp, #28]                   @ 4-byte Spill
	ldr	r6, [sp, #60]                   @ 4-byte Reload
	ldr	r7, [sp, #80]                   @ 4-byte Reload
	sbcs	r5, r5, r6
	str	r5, [sp, #24]                   @ 4-byte Spill
	ldr	r5, [sp, #64]                   @ 4-byte Reload
	ldr	r6, [sp, #104]                  @ 4-byte Reload
	sbcs	r4, r4, r5
	str	r4, [sp, #20]                   @ 4-byte Spill
	ldr	r4, [sp, #88]                   @ 4-byte Reload
	sbcs	r5, r12, r4
	ldr	r4, [r1, #88]
	sbcs	lr, lr, r7
	ldr	r7, [sp, #72]                   @ 4-byte Reload
	ldr	r12, [r3, #12]
	sbcs	r8, r8, r7
	ldr	r7, [sp, #68]                   @ 4-byte Reload
	str	r5, [sp, #88]                   @ 4-byte Spill
	sbcs	r9, r9, r7
	ldr	r7, [r1, #64]
	str	lr, [sp, #80]                   @ 4-byte Spill
	sbcs	r7, r7, r6
	str	r7, [sp, #104]                  @ 4-byte Spill
	ldr	r7, [r1, #68]
	ldr	r6, [sp, #100]                  @ 4-byte Reload
	str	r8, [sp, #72]                   @ 4-byte Spill
	sbcs	r7, r7, r6
	str	r7, [sp, #100]                  @ 4-byte Spill
	ldr	r7, [r1, #72]
	ldr	r6, [sp, #96]                   @ 4-byte Reload
	str	r9, [sp, #64]                   @ 4-byte Spill
	sbcs	r7, r7, r6
	str	r7, [sp, #96]                   @ 4-byte Spill
	ldr	r7, [r1, #76]
	ldr	r6, [sp, #92]                   @ 4-byte Reload
	sbcs	r7, r7, r6
	str	r7, [sp, #92]                   @ 4-byte Spill
	ldr	r7, [r1, #80]
	ldr	r6, [sp, #76]                   @ 4-byte Reload
	sbcs	r7, r7, r6
	str	r7, [sp, #16]                   @ 4-byte Spill
	ldr	r6, [r1, #84]
	ldr	r7, [sp, #84]                   @ 4-byte Reload
	ldr	r1, [r1, #92]
	sbcs	r7, r6, r7
	ldr	r6, [r2, #88]
	ldr	r2, [r2, #92]
	sbcs	r10, r4, r6
	str	r7, [sp, #12]                   @ 4-byte Spill
	sbcs	r1, r1, r2
	str	r1, [sp, #4]                    @ 4-byte Spill
	mov	r1, #0
	ldr	r2, [r3, #8]
	sbc	r1, r1, #0
	str	r1, [sp]                        @ 4-byte Spill
	ldm	r3, {r1, r6}
	adds	r1, r5, r1
	str	r1, [sp, #84]                   @ 4-byte Spill
	adcs	r1, lr, r6
	str	r1, [sp, #76]                   @ 4-byte Spill
	adcs	r1, r8, r2
	str	r1, [sp, #68]                   @ 4-byte Spill
	adcs	r1, r9, r12
	str	r1, [sp, #60]                   @ 4-byte Spill
	ldr	r1, [sp, #8]                    @ 4-byte Reload
	add	lr, r3, #36
	ldr	r5, [r3, #32]
	add	r12, r3, #20
	stm	r0, {r1, r11}
	ldr	r1, [sp, #56]                   @ 4-byte Reload
	str	r1, [r0, #8]
	ldr	r1, [sp, #52]                   @ 4-byte Reload
	str	r1, [r0, #12]
	ldr	r1, [sp, #48]                   @ 4-byte Reload
	str	r1, [r0, #16]
	ldr	r1, [sp, #44]                   @ 4-byte Reload
	str	r1, [r0, #20]
	ldr	r1, [sp, #40]                   @ 4-byte Reload
	str	r1, [r0, #24]
	ldr	r1, [sp, #36]                   @ 4-byte Reload
	str	r1, [r0, #28]
	ldr	r1, [sp, #32]                   @ 4-byte Reload
	str	r1, [r0, #32]
	ldr	r1, [sp, #28]                   @ 4-byte Reload
	ldr	r3, [r3, #16]
	ldr	r7, [sp, #104]                  @ 4-byte Reload
	str	r1, [r0, #36]
	ldr	r1, [sp, #24]                   @ 4-byte Reload
	adcs	r3, r7, r3
	str	r1, [r0, #40]
	ldr	r1, [sp, #20]                   @ 4-byte Reload
	str	r1, [r0, #44]
	str	r3, [sp, #56]                   @ 4-byte Spill
	ldm	r12, {r1, r2, r12}
	ldr	r3, [sp, #100]                  @ 4-byte Reload
	ldr	r9, [sp, #16]                   @ 4-byte Reload
	adcs	r11, r3, r1
	ldr	r1, [sp, #96]                   @ 4-byte Reload
	ldm	lr, {r4, r6, lr}
	adcs	r8, r1, r2
	ldr	r1, [sp, #92]                   @ 4-byte Reload
	mov	r2, r10
	ldr	r3, [sp, #12]                   @ 4-byte Reload
	adcs	r12, r1, r12
	ldr	r1, [sp, #4]                    @ 4-byte Reload
	adcs	r5, r9, r5
	ldr	r7, [sp]                        @ 4-byte Reload
	adcs	r4, r3, r4
	adcs	r6, r10, r6
	adc	lr, r1, lr
	ands	r10, r7, #1
	moveq	lr, r1
	ldr	r1, [sp, #92]                   @ 4-byte Reload
	moveq	r6, r2
	moveq	r4, r3
	cmp	r10, #0
	ldr	r2, [sp, #56]                   @ 4-byte Reload
	moveq	r12, r1
	ldr	r1, [sp, #96]                   @ 4-byte Reload
	moveq	r5, r9
	str	lr, [r0, #92]
	str	r6, [r0, #88]
	moveq	r8, r1
	ldr	r1, [sp, #100]                  @ 4-byte Reload
	cmp	r10, #0
	str	r4, [r0, #84]
	str	r5, [r0, #80]
	moveq	r11, r1
	ldr	r1, [sp, #104]                  @ 4-byte Reload
	str	r12, [r0, #76]
	str	r8, [r0, #72]
	moveq	r2, r1
	ldr	r1, [sp, #60]                   @ 4-byte Reload
	str	r2, [r0, #64]
	ldr	r2, [sp, #64]                   @ 4-byte Reload
	str	r11, [r0, #68]
	moveq	r1, r2
	ldr	r2, [sp, #72]                   @ 4-byte Reload
	str	r1, [r0, #60]
	cmp	r10, #0
	ldr	r1, [sp, #68]                   @ 4-byte Reload
	moveq	r1, r2
	ldr	r2, [sp, #80]                   @ 4-byte Reload
	str	r1, [r0, #56]
	ldr	r1, [sp, #76]                   @ 4-byte Reload
	moveq	r1, r2
	ldr	r2, [sp, #88]                   @ 4-byte Reload
	str	r1, [r0, #52]
	ldr	r1, [sp, #84]                   @ 4-byte Reload
	moveq	r1, r2
	str	r1, [r0, #48]
	add	sp, sp, #108
	pop	{r4, r5, r6, r7, r8, r9, r10, r11, lr}
	mov	pc, lr
.Lfunc_end72:
	.size	mcl_fpDbl_sub12L, .Lfunc_end72-mcl_fpDbl_sub12L
	.cantunwind
	.fnend
                                        @ -- End function
	.globl	mulPv512x32                     @ -- Begin function mulPv512x32
	.p2align	2
	.type	mulPv512x32,%function
	.code	32                              @ @mulPv512x32
mulPv512x32:
	.fnstart
@ %bb.0:
	.save	{r4, r5, r6, r7, r8, r9, r11, lr}
	push	{r4, r5, r6, r7, r8, r9, r11, lr}
	ldr	r12, [r1]
	ldmib	r1, {r3, lr}
	umull	r7, r6, r12, r2
	ldr	r4, [r1, #12]
	umull	r5, r8, r3, r2
	str	r7, [r0]
	umull	r9, r12, r4, r2
	adds	r7, r6, r5
	umull	r5, r4, lr, r2
	adcs	r7, r8, r5
	umlal	r6, r5, r3, r2
	adcs	r7, r4, r9
	str	r7, [r0, #12]
	str	r5, [r0, #8]
	str	r6, [r0, #4]
	ldr	r3, [r1, #16]
	umull	r7, r6, r3, r2
	adcs	r3, r12, r7
	str	r3, [r0, #16]
	ldr	r3, [r1, #20]
	umull	r7, r5, r3, r2
	adcs	r3, r6, r7
	str	r3, [r0, #20]
	ldr	r3, [r1, #24]
	umull	r7, r6, r3, r2
	adcs	r3, r5, r7
	str	r3, [r0, #24]
	ldr	r3, [r1, #28]
	umull	r7, r5, r3, r2
	adcs	r3, r6, r7
	str	r3, [r0, #28]
	ldr	r3, [r1, #32]
	umull	r7, r6, r3, r2
	adcs	r3, r5, r7
	str	r3, [r0, #32]
	ldr	r3, [r1, #36]
	umull	r7, r5, r3, r2
	adcs	r3, r6, r7
	str	r3, [r0, #36]
	ldr	r3, [r1, #40]
	umull	r7, r6, r3, r2
	adcs	r3, r5, r7
	str	r3, [r0, #40]
	ldr	r3, [r1, #44]
	umull	r7, r5, r3, r2
	adcs	r3, r6, r7
	str	r3, [r0, #44]
	ldr	r3, [r1, #48]
	umull	r7, r6, r3, r2
	adcs	r3, r5, r7
	str	r3, [r0, #48]
	ldr	r3, [r1, #52]
	umull	r7, r5, r3, r2
	adcs	r3, r6, r7
	str	r3, [r0, #52]
	ldr	r3, [r1, #56]
	umull	r7, r6, r3, r2
	adcs	r3, r5, r7
	str	r3, [r0, #56]
	ldr	r1, [r1, #60]
	umull	r3, r7, r1, r2
	adcs	r1, r6, r3
	str	r1, [r0, #60]
	adc	r1, r7, #0
	str	r1, [r0, #64]
	pop	{r4, r5, r6, r7, r8, r9, r11, lr}
	mov	pc, lr
.Lfunc_end73:
	.size	mulPv512x32, .Lfunc_end73-mulPv512x32
	.cantunwind
	.fnend
                                        @ -- End function
	.globl	mcl_fp_mulUnitPre16L            @ -- Begin function mcl_fp_mulUnitPre16L
	.p2align	2
	.type	mcl_fp_mulUnitPre16L,%function
	.code	32                              @ @mcl_fp_mulUnitPre16L
mcl_fp_mulUnitPre16L:
	.fnstart
@ %bb.0:
	.save	{r4, r5, r6, r7, r8, r9, r10, r11, lr}
	push	{r4, r5, r6, r7, r8, r9, r10, r11, lr}
	.pad	#92
	sub	sp, sp, #92
	mov	r4, r0
	add	r0, sp, #16
	bl	mulPv512x32
	add	r11, sp, #16
	ldr	r0, [sp, #32]
	add	lr, sp, #52
	ldm	r11, {r8, r9, r10, r11}
	str	r0, [sp, #12]                   @ 4-byte Spill
	ldr	r0, [sp, #36]
	str	r0, [sp, #8]                    @ 4-byte Spill
	ldr	r0, [sp, #40]
	str	r0, [sp, #4]                    @ 4-byte Spill
	ldr	r0, [sp, #44]
	str	r0, [sp]                        @ 4-byte Spill
	ldm	lr, {r0, r1, r2, r3, r12, lr}
	ldr	r6, [sp, #80]
	ldr	r7, [sp, #48]
	ldr	r5, [sp, #76]
	str	r6, [r4, #64]
	add	r6, r4, #36
	str	r7, [r4, #32]
	stm	r6, {r0, r1, r2, r3, r12, lr}
	ldr	r0, [sp, #12]                   @ 4-byte Reload
	str	r5, [r4, #60]
	stm	r4, {r8, r9, r10, r11}
	str	r0, [r4, #16]
	ldr	r0, [sp, #8]                    @ 4-byte Reload
	str	r0, [r4, #20]
	ldr	r0, [sp, #4]                    @ 4-byte Reload
	str	r0, [r4, #24]
	ldr	r0, [sp]                        @ 4-byte Reload
	str	r0, [r4, #28]
	add	sp, sp, #92
	pop	{r4, r5, r6, r7, r8, r9, r10, r11, lr}
	mov	pc, lr
.Lfunc_end74:
	.size	mcl_fp_mulUnitPre16L, .Lfunc_end74-mcl_fp_mulUnitPre16L
	.cantunwind
	.fnend
                                        @ -- End function
	.globl	mcl_fpDbl_mulPre16L             @ -- Begin function mcl_fpDbl_mulPre16L
	.p2align	2
	.type	mcl_fpDbl_mulPre16L,%function
	.code	32                              @ @mcl_fpDbl_mulPre16L
mcl_fpDbl_mulPre16L:
	.fnstart
@ %bb.0:
	.save	{r4, r5, r6, r7, r8, r9, r10, r11, lr}
	push	{r4, r5, r6, r7, r8, r9, r10, r11, lr}
	.pad	#276
	sub	sp, sp, #276
	mov	r6, r2
	mov	r5, r1
	mov	r4, r0
	bl	mcl_fpDbl_mulPre8L
	add	r0, r4, #64
	add	r1, r5, #32
	add	r2, r6, #32
	bl	mcl_fpDbl_mulPre8L
	ldr	r0, [r6, #12]
	add	lr, r6, #32
	str	r0, [sp, #120]                  @ 4-byte Spill
	ldr	r0, [r6, #16]
	str	r0, [sp, #124]                  @ 4-byte Spill
	ldr	r0, [r6, #20]
	str	r0, [sp, #132]                  @ 4-byte Spill
	ldr	r0, [r6, #24]
	str	r0, [sp, #136]                  @ 4-byte Spill
	ldr	r0, [r6, #28]
	str	r0, [sp, #140]                  @ 4-byte Spill
	ldm	r6, {r9, r10, r11}
	ldm	lr, {r0, r1, r2, r3, r12, lr}
	adds	r8, r0, r9
	ldr	r7, [r6, #56]
	adcs	r0, r1, r10
	str	r0, [sp, #144]                  @ 4-byte Spill
	ldr	r0, [sp, #120]                  @ 4-byte Reload
	adcs	r1, r2, r11
	ldr	r6, [r6, #60]
	add	r10, r5, #44
	adcs	r0, r3, r0
	str	r0, [sp, #76]                   @ 4-byte Spill
	ldr	r0, [sp, #124]                  @ 4-byte Reload
	ldr	r3, [r5, #36]
	adcs	r0, r12, r0
	str	r0, [sp, #104]                  @ 4-byte Spill
	ldr	r0, [sp, #132]                  @ 4-byte Reload
	ldr	r11, [r5, #8]
	adcs	r0, lr, r0
	str	r0, [sp, #108]                  @ 4-byte Spill
	ldr	r0, [sp, #136]                  @ 4-byte Reload
	ldm	r5, {r12, lr}
	adcs	r0, r7, r0
	str	r0, [sp, #120]                  @ 4-byte Spill
	ldr	r0, [sp, #140]                  @ 4-byte Reload
	ldr	r7, [r5, #32]
	adcs	r0, r6, r0
	str	r0, [sp, #124]                  @ 4-byte Spill
	mov	r0, #0
	ldr	r2, [r5, #40]
	adc	r0, r0, #0
	str	r0, [sp, #140]                  @ 4-byte Spill
	ldr	r0, [r5, #12]
	adds	r7, r7, r12
	str	r0, [sp, #112]                  @ 4-byte Spill
	adcs	r3, r3, lr
	ldr	r0, [r5, #16]
	adcs	r2, r2, r11
	str	r0, [sp, #84]                   @ 4-byte Spill
	ldr	r0, [r5, #20]
	str	r0, [sp, #100]                  @ 4-byte Spill
	ldr	r0, [r5, #24]
	str	r0, [sp, #88]                   @ 4-byte Spill
	ldr	r0, [r5, #28]
	str	r0, [sp, #92]                   @ 4-byte Spill
	str	r1, [sp, #96]                   @ 4-byte Spill
	str	r1, [sp, #156]
	ldm	r10, {r0, r6, r10}
	ldr	r1, [sp, #112]                  @ 4-byte Reload
	ldr	r9, [r5, #56]
	adcs	r0, r0, r1
	str	r0, [sp, #112]                  @ 4-byte Spill
	str	r0, [sp, #192]
	ldr	r0, [sp, #84]                   @ 4-byte Reload
	str	r9, [sp, #80]                   @ 4-byte Spill
	adcs	r9, r6, r0
	ldr	r0, [sp, #100]                  @ 4-byte Reload
	ldr	r1, [sp, #80]                   @ 4-byte Reload
	adcs	r0, r10, r0
	str	r0, [sp, #100]                  @ 4-byte Spill
	str	r0, [sp, #200]
	ldr	r0, [sp, #88]                   @ 4-byte Reload
	ldr	r5, [r5, #60]
	adcs	r0, r1, r0
	str	r0, [sp, #88]                   @ 4-byte Spill
	str	r0, [sp, #204]
	add	r1, sp, #180
	ldr	r0, [sp, #92]                   @ 4-byte Reload
	str	r2, [sp, #116]                  @ 4-byte Spill
	adcs	r0, r5, r0
	str	r0, [sp, #84]                   @ 4-byte Spill
	str	r0, [sp, #208]
	mov	r0, #0
	str	r2, [sp, #188]
	adc	r10, r0, #0
	add	r0, sp, #212
	add	r2, sp, #148
	ldr	r11, [sp, #104]                 @ 4-byte Reload
	ldr	r6, [sp, #120]                  @ 4-byte Reload
	ldr	r5, [sp, #124]                  @ 4-byte Reload
	str	r8, [sp, #128]                  @ 4-byte Spill
	str	r8, [sp, #148]
	ldr	r8, [sp, #76]                   @ 4-byte Reload
	str	r7, [sp, #136]                  @ 4-byte Spill
	str	r7, [sp, #180]
	ldr	r7, [sp, #108]                  @ 4-byte Reload
	str	r3, [sp, #132]                  @ 4-byte Spill
	str	r3, [sp, #184]
	ldr	r3, [sp, #144]                  @ 4-byte Reload
	str	r3, [sp, #152]
	str	r8, [sp, #160]
	str	r9, [sp, #196]
	str	r11, [sp, #164]
	str	r7, [sp, #168]
	str	r6, [sp, #172]
	str	r5, [sp, #176]
	bl	mcl_fpDbl_mulPre8L
	ldr	r2, [sp, #96]                   @ 4-byte Reload
	rsb	r0, r10, #0
	mov	r1, r10
	str	r10, [sp, #92]                  @ 4-byte Spill
	and	r10, r7, r0
	and	r12, r5, r0
	and	r3, r6, r0
	and	r11, r11, r0
	and	r7, r2, r0
	ldr	r0, [sp, #144]                  @ 4-byte Reload
	sub	r1, r1, r1, lsl #1
	and	r6, r0, r1
	ldr	r0, [sp, #128]                  @ 4-byte Reload
	and	r5, r8, r1
	str	r6, [sp, #124]                  @ 4-byte Spill
	and	r1, r0, r1
	ldr	r0, [sp, #136]                  @ 4-byte Reload
	str	r1, [sp, #144]                  @ 4-byte Spill
	adds	r2, r1, r0
	ldr	r0, [sp, #132]                  @ 4-byte Reload
	ldr	r1, [sp, #88]                   @ 4-byte Reload
	adcs	lr, r6, r0
	ldr	r0, [sp, #116]                  @ 4-byte Reload
	adcs	r6, r7, r0
	ldr	r0, [sp, #112]                  @ 4-byte Reload
	adcs	r8, r5, r0
	ldr	r0, [sp, #100]                  @ 4-byte Reload
	adcs	r9, r11, r9
	adcs	r0, r10, r0
	adcs	r1, r3, r1
	str	r1, [sp, #120]                  @ 4-byte Spill
	ldr	r1, [sp, #84]                   @ 4-byte Reload
	adcs	r1, r12, r1
	str	r1, [sp, #128]                  @ 4-byte Spill
	mov	r1, #0
	adc	r1, r1, #0
	str	r1, [sp, #136]                  @ 4-byte Spill
	ldr	r1, [sp, #140]                  @ 4-byte Reload
	cmp	r1, #0
	moveq	r0, r10
	moveq	r9, r11
	str	r0, [sp, #132]                  @ 4-byte Spill
	moveq	r8, r5
	ldr	r0, [sp, #124]                  @ 4-byte Reload
	moveq	r6, r7
	ldr	r7, [sp, #244]
	ldr	r11, [sp, #120]                 @ 4-byte Reload
	moveq	lr, r0
	ldr	r0, [sp, #144]                  @ 4-byte Reload
	cmp	r1, #0
	ldr	r5, [sp, #128]                  @ 4-byte Reload
	moveq	r11, r3
	moveq	r5, r12
	moveq	r2, r0
	ldr	r0, [sp, #136]                  @ 4-byte Reload
	ldr	r3, [sp, #92]                   @ 4-byte Reload
	and	r10, r1, r0
	adds	r0, r7, r2
	str	r0, [sp, #144]                  @ 4-byte Spill
	and	r1, r1, r3
	ldr	r0, [sp, #248]
	ldr	r2, [sp, #132]                  @ 4-byte Reload
	adcs	r0, r0, lr
	str	r0, [sp, #136]                  @ 4-byte Spill
	ldr	r0, [sp, #252]
	add	lr, r4, #8
	ldr	r7, [r4, #28]
	adcs	r0, r0, r6
	str	r0, [sp, #128]                  @ 4-byte Spill
	ldr	r0, [sp, #256]
	ldm	lr, {r3, r6, lr}
	adcs	r0, r0, r8
	str	r0, [sp, #120]                  @ 4-byte Spill
	ldr	r0, [sp, #260]
	ldm	r4, {r8, r12}
	adcs	r0, r0, r9
	str	r0, [sp, #112]                  @ 4-byte Spill
	ldr	r0, [sp, #264]
	adcs	r0, r0, r2
	str	r0, [sp, #104]                  @ 4-byte Spill
	ldr	r0, [sp, #268]
	add	r2, sp, #216
	adcs	r0, r0, r11
	str	r0, [sp, #96]                   @ 4-byte Spill
	ldr	r0, [sp, #272]
	ldr	r11, [r4, #24]
	adcs	r9, r0, r5
	ldr	r5, [sp, #212]
	adc	r0, r1, r10
	str	r0, [sp, #92]                   @ 4-byte Spill
	ldm	r2, {r0, r1, r2}
	subs	r5, r5, r8
	sbcs	r0, r0, r12
	str	r0, [sp, #84]                   @ 4-byte Spill
	sbcs	r0, r1, r3
	str	r0, [sp, #80]                   @ 4-byte Spill
	sbcs	r0, r2, r6
	str	r0, [sp, #76]                   @ 4-byte Spill
	ldr	r0, [sp, #228]
	ldr	r10, [r4, #20]
	sbcs	r0, r0, lr
	str	r0, [sp, #72]                   @ 4-byte Spill
	ldr	r0, [sp, #232]
	str	r5, [sp, #88]                   @ 4-byte Spill
	sbcs	r0, r0, r10
	str	r0, [sp, #68]                   @ 4-byte Spill
	ldr	r0, [sp, #236]
	ldr	r10, [r4, #32]
	sbcs	r8, r0, r11
	ldr	r0, [sp, #240]
	ldr	r5, [sp, #144]                  @ 4-byte Reload
	sbcs	lr, r0, r7
	ldr	r11, [r4, #36]
	sbcs	r5, r5, r10
	str	r5, [sp, #64]                   @ 4-byte Spill
	ldr	r5, [sp, #136]                  @ 4-byte Reload
	ldr	r3, [r4, #40]
	sbcs	r5, r5, r11
	str	r5, [sp, #60]                   @ 4-byte Spill
	ldr	r5, [sp, #128]                  @ 4-byte Reload
	str	r3, [sp, #100]                  @ 4-byte Spill
	sbcs	r3, r5, r3
	ldr	r2, [r4, #44]
	str	r3, [sp, #56]                   @ 4-byte Spill
	ldr	r3, [sp, #120]                  @ 4-byte Reload
	str	r2, [sp, #108]                  @ 4-byte Spill
	sbcs	r2, r3, r2
	ldr	r1, [r4, #48]
	str	r2, [sp, #52]                   @ 4-byte Spill
	ldr	r2, [sp, #112]                  @ 4-byte Reload
	str	r1, [sp, #116]                  @ 4-byte Spill
	sbcs	r1, r2, r1
	ldr	r6, [r4, #52]
	str	r1, [sp, #48]                   @ 4-byte Spill
	ldr	r1, [sp, #104]                  @ 4-byte Reload
	ldr	r7, [r4, #56]
	sbcs	r1, r1, r6
	str	r1, [sp, #44]                   @ 4-byte Spill
	ldr	r1, [sp, #96]                   @ 4-byte Reload
	ldr	r0, [r4, #60]
	sbcs	r1, r1, r7
	str	r0, [sp, #140]                  @ 4-byte Spill
	sbcs	r0, r9, r0
	str	r7, [sp, #132]                  @ 4-byte Spill
	str	r0, [sp, #36]                   @ 4-byte Spill
	ldr	r0, [sp, #92]                   @ 4-byte Reload
	ldr	r12, [r4, #64]
	ldr	r7, [sp, #88]                   @ 4-byte Reload
	sbc	r0, r0, #0
	ldr	r9, [r4, #68]
	subs	r7, r7, r12
	str	r7, [sp, #12]                   @ 4-byte Spill
	ldr	r7, [sp, #84]                   @ 4-byte Reload
	str	r0, [sp, #32]                   @ 4-byte Spill
	sbcs	r7, r7, r9
	ldr	r0, [r4, #72]
	str	r7, [sp, #8]                    @ 4-byte Spill
	ldr	r7, [sp, #80]                   @ 4-byte Reload
	str	r0, [sp, #104]                  @ 4-byte Spill
	sbcs	r0, r7, r0
	ldr	r3, [r4, #76]
	str	r0, [sp, #20]                   @ 4-byte Spill
	ldr	r0, [sp, #76]                   @ 4-byte Reload
	ldr	r2, [r4, #80]
	sbcs	r0, r0, r3
	str	r0, [sp, #24]                   @ 4-byte Spill
	ldr	r0, [sp, #72]                   @ 4-byte Reload
	str	r1, [sp, #40]                   @ 4-byte Spill
	sbcs	r0, r0, r2
	ldr	r1, [r4, #84]
	str	r0, [sp, #28]                   @ 4-byte Spill
	ldr	r0, [sp, #68]                   @ 4-byte Reload
	str	r6, [sp, #124]                  @ 4-byte Spill
	str	r9, [sp, #92]                   @ 4-byte Spill
	add	r9, r4, #96
	ldr	r6, [r4, #88]
	sbcs	r0, r0, r1
	ldr	r5, [r4, #92]
	str	r2, [sp, #120]                  @ 4-byte Spill
	str	r0, [sp, #68]                   @ 4-byte Spill
	sbcs	r0, r8, r6
	str	r1, [sp, #128]                  @ 4-byte Spill
	ldm	r9, {r1, r8, r9}
	ldr	r2, [sp, #64]                   @ 4-byte Reload
	str	r0, [sp, #76]                   @ 4-byte Spill
	sbcs	r0, lr, r5
	ldr	r7, [sp, #60]                   @ 4-byte Reload
	sbcs	r2, r2, r1
	str	r3, [sp, #112]                  @ 4-byte Spill
	sbcs	r7, r7, r8
	str	r7, [sp, #4]                    @ 4-byte Spill
	ldr	r7, [sp, #56]                   @ 4-byte Reload
	ldr	r3, [r4, #108]
	sbcs	r7, r7, r9
	str	r7, [sp, #16]                   @ 4-byte Spill
	ldr	r7, [sp, #52]                   @ 4-byte Reload
	str	r0, [sp, #88]                   @ 4-byte Spill
	sbcs	r7, r7, r3
	ldr	r0, [r4, #112]
	str	r7, [sp, #56]                   @ 4-byte Spill
	ldr	r7, [sp, #48]                   @ 4-byte Reload
	str	r0, [sp, #96]                   @ 4-byte Spill
	sbcs	r0, r7, r0
	str	r0, [sp, #60]                   @ 4-byte Spill
	ldr	lr, [r4, #116]
	ldr	r0, [sp, #44]                   @ 4-byte Reload
	str	r5, [sp, #144]                  @ 4-byte Spill
	sbcs	r0, r0, lr
	str	r0, [sp, #64]                   @ 4-byte Spill
	ldr	r5, [r4, #120]
	ldr	r0, [sp, #40]                   @ 4-byte Reload
	str	r6, [sp, #136]                  @ 4-byte Spill
	sbcs	r0, r0, r5
	str	r0, [sp, #72]                   @ 4-byte Spill
	ldr	r6, [r4, #124]
	ldr	r0, [sp, #36]                   @ 4-byte Reload
	ldr	r7, [sp, #20]                   @ 4-byte Reload
	sbcs	r0, r0, r6
	str	r0, [sp, #80]                   @ 4-byte Spill
	ldr	r0, [sp, #32]                   @ 4-byte Reload
	sbc	r0, r0, #0
	str	r0, [sp, #84]                   @ 4-byte Spill
	ldr	r0, [sp, #12]                   @ 4-byte Reload
	adds	r10, r10, r0
	ldr	r0, [sp, #8]                    @ 4-byte Reload
	str	r10, [r4, #32]
	adcs	r11, r11, r0
	ldr	r0, [sp, #100]                  @ 4-byte Reload
	str	r11, [r4, #36]
	adcs	r10, r0, r7
	ldr	r0, [sp, #108]                  @ 4-byte Reload
	ldr	r7, [sp, #24]                   @ 4-byte Reload
	str	r10, [r4, #40]
	adcs	r11, r0, r7
	ldr	r0, [sp, #116]                  @ 4-byte Reload
	ldr	r7, [sp, #28]                   @ 4-byte Reload
	str	r11, [r4, #44]
	adcs	r10, r0, r7
	ldr	r0, [sp, #124]                  @ 4-byte Reload
	ldr	r7, [sp, #68]                   @ 4-byte Reload
	str	r10, [r4, #48]
	adcs	r11, r0, r7
	ldr	r0, [sp, #132]                  @ 4-byte Reload
	ldr	r7, [sp, #76]                   @ 4-byte Reload
	str	r11, [r4, #52]
	adcs	r10, r0, r7
	ldr	r0, [sp, #140]                  @ 4-byte Reload
	ldr	r7, [sp, #88]                   @ 4-byte Reload
	str	r10, [r4, #56]
	adcs	r11, r0, r7
	ldr	r7, [sp, #4]                    @ 4-byte Reload
	adcs	r0, r12, r2
	ldr	r2, [sp, #92]                   @ 4-byte Reload
	str	r0, [r4, #64]
	add	r12, r4, #100
	adcs	r2, r2, r7
	ldr	r0, [sp, #104]                  @ 4-byte Reload
	ldr	r7, [sp, #16]                   @ 4-byte Reload
	str	r2, [r4, #68]
	adcs	r0, r0, r7
	ldr	r2, [sp, #112]                  @ 4-byte Reload
	ldr	r7, [sp, #56]                   @ 4-byte Reload
	str	r0, [r4, #72]
	adcs	r2, r2, r7
	ldr	r0, [sp, #120]                  @ 4-byte Reload
	ldr	r7, [sp, #60]                   @ 4-byte Reload
	str	r2, [r4, #76]
	adcs	r0, r0, r7
	ldr	r2, [sp, #128]                  @ 4-byte Reload
	ldr	r7, [sp, #64]                   @ 4-byte Reload
	str	r0, [r4, #80]
	adcs	r2, r2, r7
	ldr	r0, [sp, #136]                  @ 4-byte Reload
	ldr	r7, [sp, #72]                   @ 4-byte Reload
	str	r2, [r4, #84]
	adcs	r0, r0, r7
	ldr	r2, [sp, #144]                  @ 4-byte Reload
	ldr	r7, [sp, #80]                   @ 4-byte Reload
	str	r0, [r4, #88]
	ldr	r0, [sp, #84]                   @ 4-byte Reload
	adcs	r2, r2, r7
	str	r2, [r4, #92]
	adcs	r10, r1, r0
	ldr	r0, [sp, #96]                   @ 4-byte Reload
	adcs	r1, r8, #0
	str	r11, [r4, #60]
	adcs	r2, r9, #0
	str	r10, [r4, #96]
	adcs	r3, r3, #0
	adcs	r7, r0, #0
	stm	r12, {r1, r2, r3, r7}
	adcs	r0, lr, #0
	adcs	r5, r5, #0
	add	r1, r4, #116
	adc	r6, r6, #0
	stm	r1, {r0, r5, r6}
	add	sp, sp, #276
	pop	{r4, r5, r6, r7, r8, r9, r10, r11, lr}
	mov	pc, lr
.Lfunc_end75:
	.size	mcl_fpDbl_mulPre16L, .Lfunc_end75-mcl_fpDbl_mulPre16L
	.cantunwind
	.fnend
                                        @ -- End function
	.globl	mcl_fpDbl_sqrPre16L             @ -- Begin function mcl_fpDbl_sqrPre16L
	.p2align	2
	.type	mcl_fpDbl_sqrPre16L,%function
	.code	32                              @ @mcl_fpDbl_sqrPre16L
mcl_fpDbl_sqrPre16L:
	.fnstart
@ %bb.0:
	.save	{r4, r5, r6, r7, r8, r9, r10, r11, lr}
	push	{r4, r5, r6, r7, r8, r9, r10, r11, lr}
	.pad	#276
	sub	sp, sp, #276
	mov	r2, r1
	mov	r5, r1
	mov	r4, r0
	bl	mcl_fpDbl_mulPre8L
	add	r1, r5, #32
	add	r0, r4, #64
	mov	r2, r1
	bl	mcl_fpDbl_mulPre8L
	ldr	r0, [r5, #16]
	add	lr, r5, #44
	str	r0, [sp, #132]                  @ 4-byte Spill
	ldr	r0, [r5, #20]
	ldm	r5, {r8, r9, r10, r11}
	ldr	r7, [r5, #32]
	str	r0, [sp, #136]                  @ 4-byte Spill
	ldr	r0, [r5, #24]
	adds	r8, r7, r8
	ldr	r6, [r5, #36]
	str	r0, [sp, #140]                  @ 4-byte Spill
	ldr	r0, [r5, #28]
	ldr	r3, [r5, #40]
	adcs	r5, r6, r9
	str	r0, [sp, #144]                  @ 4-byte Spill
	ldm	lr, {r0, r1, r2, r12, lr}
	adcs	r3, r3, r10
	adcs	r7, r0, r11
	ldr	r0, [sp, #132]                  @ 4-byte Reload
	str	r8, [sp, #180]
	adcs	r11, r1, r0
	ldr	r0, [sp, #136]                  @ 4-byte Reload
	add	r1, sp, #180
	str	r5, [sp, #184]
	adcs	r6, r2, r0
	ldr	r0, [sp, #140]                  @ 4-byte Reload
	add	r2, sp, #148
	str	r3, [sp, #188]
	adcs	r9, r12, r0
	ldr	r0, [sp, #144]                  @ 4-byte Reload
	str	r3, [sp, #128]                  @ 4-byte Spill
	adcs	r10, lr, r0
	add	r0, sp, #168
	stm	r0, {r6, r9, r10}
	add	r0, sp, #156
	stm	r0, {r3, r7, r11}
	mov	r0, #0
	adc	r0, r0, #0
	str	r0, [sp, #144]                  @ 4-byte Spill
	add	r0, sp, #212
	str	r7, [sp, #192]
	str	r11, [sp, #196]
	str	r6, [sp, #200]
	str	r9, [sp, #204]
	str	r10, [sp, #208]
	str	r5, [sp, #152]
	str	r8, [sp, #148]
	bl	mcl_fpDbl_mulPre8L
	ldr	r1, [sp, #144]                  @ 4-byte Reload
	rsb	r3, r1, #0
	sub	r0, r1, r1, lsl #1
	ldr	r1, [sp, #128]                  @ 4-byte Reload
	and	r2, r5, r0
	and	r12, r8, r0
	and	r0, r7, r0
	and	r7, r1, r3
	and	r1, r9, r3
	and	lr, r10, r3
	and	r6, r6, r3
	and	r5, r11, r3
	lsl	r3, lr, #1
	orr	r8, r3, r1, lsr #31
	lsl	r1, r1, #1
	ldr	r3, [sp, #244]
	orr	r1, r1, r6, lsr #31
	lsl	r6, r6, #1
	orr	r6, r6, r5, lsr #31
	lsl	r5, r5, #1
	orr	r5, r5, r0, lsr #31
	lsl	r0, r0, #1
	orr	r0, r0, r7, lsr #31
	lsl	r7, r7, #1
	adds	r3, r3, r12, lsl #1
	str	r3, [sp, #140]                  @ 4-byte Spill
	orr	r7, r7, r2, lsr #31
	lsl	r2, r2, #1
	ldr	r3, [sp, #248]
	orr	r2, r2, r12, lsr #31
	add	r12, r4, #8
	ldr	r9, [r4, #20]
	adcs	r2, r3, r2
	str	r2, [sp, #136]                  @ 4-byte Spill
	ldr	r2, [sp, #252]
	ldr	r11, [r4, #24]
	adcs	r2, r2, r7
	str	r2, [sp, #128]                  @ 4-byte Spill
	ldr	r2, [sp, #256]
	ldr	r7, [r4, #28]
	adcs	r0, r2, r0
	str	r0, [sp, #120]                  @ 4-byte Spill
	ldr	r0, [sp, #260]
	ldr	r2, [sp, #224]
	adcs	r0, r0, r5
	str	r0, [sp, #112]                  @ 4-byte Spill
	ldr	r0, [sp, #264]
	ldr	r5, [sp, #212]
	adcs	r0, r0, r6
	str	r0, [sp, #104]                  @ 4-byte Spill
	ldr	r0, [sp, #268]
	ldm	r12, {r3, r6, r12}
	adcs	r0, r0, r1
	str	r0, [sp, #96]                   @ 4-byte Spill
	ldr	r0, [sp, #272]
	ldr	r1, [sp, #216]
	adcs	r0, r0, r8
	str	r0, [sp, #88]                   @ 4-byte Spill
	ldr	r0, [sp, #144]                  @ 4-byte Reload
	ldr	r8, [r4, #4]
	adc	r0, r0, lr, lsr #31
	ldr	lr, [r4]
	str	r0, [sp, #84]                   @ 4-byte Spill
	subs	r5, r5, lr
	ldr	r0, [sp, #220]
	sbcs	r1, r1, r8
	ldr	r10, [sp, #140]                 @ 4-byte Reload
	sbcs	r0, r0, r3
	str	r0, [sp, #72]                   @ 4-byte Spill
	sbcs	r0, r2, r6
	str	r0, [sp, #68]                   @ 4-byte Spill
	ldr	r0, [sp, #228]
	ldr	lr, [r4, #36]
	sbcs	r0, r0, r12
	str	r0, [sp, #64]                   @ 4-byte Spill
	ldr	r0, [sp, #232]
	ldr	r6, [sp, #136]                  @ 4-byte Reload
	sbcs	r0, r0, r9
	str	r0, [sp, #60]                   @ 4-byte Spill
	ldr	r0, [sp, #236]
	str	r5, [sp, #80]                   @ 4-byte Spill
	sbcs	r9, r0, r11
	ldr	r0, [sp, #240]
	ldr	r11, [r4, #32]
	sbcs	r8, r0, r7
	ldr	r12, [r4, #40]
	sbcs	r5, r10, r11
	ldr	r3, [r4, #44]
	sbcs	r6, r6, lr
	str	r6, [sp, #52]                   @ 4-byte Spill
	ldr	r6, [sp, #128]                  @ 4-byte Reload
	str	r3, [sp, #108]                  @ 4-byte Spill
	sbcs	r6, r6, r12
	str	r6, [sp, #48]                   @ 4-byte Spill
	ldr	r6, [sp, #120]                  @ 4-byte Reload
	str	r1, [sp, #76]                   @ 4-byte Spill
	sbcs	r3, r6, r3
	ldr	r1, [r4, #48]
	str	r3, [sp, #44]                   @ 4-byte Spill
	ldr	r3, [sp, #112]                  @ 4-byte Reload
	str	r1, [sp, #116]                  @ 4-byte Spill
	sbcs	r1, r3, r1
	ldr	r7, [r4, #52]
	str	r1, [sp, #40]                   @ 4-byte Spill
	ldr	r1, [sp, #104]                  @ 4-byte Reload
	ldr	r2, [r4, #56]
	sbcs	r1, r1, r7
	str	r1, [sp, #36]                   @ 4-byte Spill
	ldr	r1, [sp, #96]                   @ 4-byte Reload
	ldr	r0, [r4, #60]
	sbcs	r1, r1, r2
	str	r1, [sp, #32]                   @ 4-byte Spill
	ldr	r1, [sp, #88]                   @ 4-byte Reload
	str	lr, [sp, #92]                   @ 4-byte Spill
	add	lr, r4, #64
	str	r0, [sp, #144]                  @ 4-byte Spill
	sbcs	r0, r1, r0
	str	r5, [sp, #56]                   @ 4-byte Spill
	str	r0, [sp, #88]                   @ 4-byte Spill
	ldr	r0, [sp, #84]                   @ 4-byte Reload
	str	r7, [sp, #124]                  @ 4-byte Spill
	str	r2, [sp, #132]                  @ 4-byte Spill
	sbc	r0, r0, #0
	ldm	lr, {r2, r7, lr}
	ldr	r5, [sp, #80]                   @ 4-byte Reload
	str	r12, [sp, #100]                 @ 4-byte Spill
	subs	r5, r5, r2
	str	r5, [sp, #12]                   @ 4-byte Spill
	ldr	r5, [sp, #76]                   @ 4-byte Reload
	ldr	r12, [r4, #76]
	sbcs	r5, r5, r7
	str	r5, [sp, #8]                    @ 4-byte Spill
	ldr	r5, [sp, #72]                   @ 4-byte Reload
	ldr	r3, [r4, #80]
	sbcs	r5, r5, lr
	str	r5, [sp, #20]                   @ 4-byte Spill
	ldr	r5, [sp, #68]                   @ 4-byte Reload
	str	r3, [sp, #120]                  @ 4-byte Spill
	sbcs	r5, r5, r12
	str	r5, [sp, #24]                   @ 4-byte Spill
	ldr	r5, [sp, #64]                   @ 4-byte Reload
	ldr	r1, [r4, #84]
	sbcs	r3, r5, r3
	str	r3, [sp, #28]                   @ 4-byte Spill
	ldr	r3, [sp, #60]                   @ 4-byte Reload
	ldr	r6, [r4, #88]
	str	r0, [sp, #84]                   @ 4-byte Spill
	str	lr, [sp, #104]                  @ 4-byte Spill
	add	lr, r4, #96
	str	r1, [sp, #128]                  @ 4-byte Spill
	sbcs	r1, r3, r1
	ldr	r0, [r4, #92]
	str	r1, [sp, #60]                   @ 4-byte Spill
	sbcs	r1, r9, r6
	str	r0, [sp, #140]                  @ 4-byte Spill
	sbcs	r0, r8, r0
	str	r12, [sp, #112]                 @ 4-byte Spill
	str	r1, [sp, #72]                   @ 4-byte Spill
	ldm	lr, {r1, r8, r9, r10, r12, lr}
	ldr	r3, [sp, #56]                   @ 4-byte Reload
	ldr	r5, [sp, #52]                   @ 4-byte Reload
	sbcs	r3, r3, r1
	str	r0, [sp, #80]                   @ 4-byte Spill
	sbcs	r5, r5, r8
	str	r5, [sp, #4]                    @ 4-byte Spill
	ldr	r5, [sp, #48]                   @ 4-byte Reload
	ldr	r0, [r4, #120]
	sbcs	r5, r5, r9
	str	r5, [sp, #16]                   @ 4-byte Spill
	ldr	r5, [sp, #44]                   @ 4-byte Reload
	str	r0, [sp, #96]                   @ 4-byte Spill
	sbcs	r5, r5, r10
	str	r5, [sp, #48]                   @ 4-byte Spill
	ldr	r5, [sp, #40]                   @ 4-byte Reload
	str	r6, [sp, #136]                  @ 4-byte Spill
	sbcs	r5, r5, r12
	str	r5, [sp, #52]                   @ 4-byte Spill
	ldr	r5, [sp, #36]                   @ 4-byte Reload
	ldr	r6, [r4, #124]
	sbcs	r5, r5, lr
	str	r5, [sp, #56]                   @ 4-byte Spill
	ldr	r5, [sp, #32]                   @ 4-byte Reload
	str	r6, [sp, #68]                   @ 4-byte Spill
	sbcs	r0, r5, r0
	str	r0, [sp, #64]                   @ 4-byte Spill
	ldr	r0, [sp, #88]                   @ 4-byte Reload
	ldr	r5, [sp, #8]                    @ 4-byte Reload
	sbcs	r0, r0, r6
	str	r0, [sp, #76]                   @ 4-byte Spill
	ldr	r0, [sp, #84]                   @ 4-byte Reload
	ldr	r6, [sp, #20]                   @ 4-byte Reload
	sbc	r0, r0, #0
	str	r0, [sp, #88]                   @ 4-byte Spill
	ldr	r0, [sp, #12]                   @ 4-byte Reload
	adds	r11, r11, r0
	ldr	r0, [sp, #92]                   @ 4-byte Reload
	str	r11, [r4, #32]
	adcs	r0, r0, r5
	ldr	r5, [sp, #100]                  @ 4-byte Reload
	str	r0, [r4, #36]
	adcs	r11, r5, r6
	ldr	r0, [sp, #108]                  @ 4-byte Reload
	ldr	r5, [sp, #24]                   @ 4-byte Reload
	ldr	r6, [sp, #28]                   @ 4-byte Reload
	adcs	r0, r0, r5
	ldr	r5, [sp, #116]                  @ 4-byte Reload
	str	r11, [r4, #40]
	adcs	r11, r5, r6
	str	r0, [r4, #44]
	ldr	r0, [sp, #124]                  @ 4-byte Reload
	ldr	r5, [sp, #60]                   @ 4-byte Reload
	ldr	r6, [sp, #72]                   @ 4-byte Reload
	adcs	r0, r0, r5
	ldr	r5, [sp, #132]                  @ 4-byte Reload
	str	r11, [r4, #48]
	adcs	r11, r5, r6
	str	r0, [r4, #52]
	ldr	r0, [sp, #144]                  @ 4-byte Reload
	ldr	r5, [sp, #80]                   @ 4-byte Reload
	str	r11, [r4, #56]
	adcs	r0, r0, r5
	str	r0, [r4, #60]
	adcs	r2, r2, r3
	ldr	r0, [sp, #4]                    @ 4-byte Reload
	str	r2, [r4, #64]
	ldr	r2, [sp, #104]                  @ 4-byte Reload
	adcs	r0, r7, r0
	ldr	r3, [sp, #16]                   @ 4-byte Reload
	str	r0, [r4, #68]
	adcs	r2, r2, r3
	ldr	r0, [sp, #112]                  @ 4-byte Reload
	ldr	r3, [sp, #48]                   @ 4-byte Reload
	str	r2, [r4, #72]
	adcs	r0, r0, r3
	ldr	r2, [sp, #120]                  @ 4-byte Reload
	ldr	r3, [sp, #52]                   @ 4-byte Reload
	str	r0, [r4, #76]
	adcs	r2, r2, r3
	ldr	r0, [sp, #128]                  @ 4-byte Reload
	ldr	r3, [sp, #56]                   @ 4-byte Reload
	str	r2, [r4, #80]
	adcs	r0, r0, r3
	ldr	r2, [sp, #136]                  @ 4-byte Reload
	ldr	r3, [sp, #64]                   @ 4-byte Reload
	str	r0, [r4, #84]
	adcs	r2, r2, r3
	ldr	r0, [sp, #140]                  @ 4-byte Reload
	ldr	r3, [sp, #76]                   @ 4-byte Reload
	str	r2, [r4, #88]
	ldr	r2, [sp, #88]                   @ 4-byte Reload
	adcs	r0, r0, r3
	str	r0, [r4, #92]
	adcs	r11, r1, r2
	ldr	r5, [sp, #96]                   @ 4-byte Reload
	adcs	r0, r8, #0
	ldr	r6, [sp, #68]                   @ 4-byte Reload
	adcs	r2, r9, #0
	str	r11, [r4, #96]
	adcs	r3, r10, #0
	adcs	r7, r12, #0
	add	r12, r4, #100
	adcs	r1, lr, #0
	stm	r12, {r0, r2, r3, r7}
	adcs	r5, r5, #0
	add	r0, r4, #116
	adc	r6, r6, #0
	stm	r0, {r1, r5, r6}
	add	sp, sp, #276
	pop	{r4, r5, r6, r7, r8, r9, r10, r11, lr}
	mov	pc, lr
.Lfunc_end76:
	.size	mcl_fpDbl_sqrPre16L, .Lfunc_end76-mcl_fpDbl_sqrPre16L
	.cantunwind
	.fnend
                                        @ -- End function
	.globl	mcl_fp_mont16L                  @ -- Begin function mcl_fp_mont16L
	.p2align	2
	.type	mcl_fp_mont16L,%function
	.code	32                              @ @mcl_fp_mont16L
mcl_fp_mont16L:
	.fnstart
@ %bb.0:
	.save	{r4, r5, r6, r7, r8, r9, r10, r11, lr}
	push	{r4, r5, r6, r7, r8, r9, r10, r11, lr}
	.pad	#404
	sub	sp, sp, #404
	.pad	#2048
	sub	sp, sp, #2048
	mov	r7, r2
	ldr	r2, [r2]
	add	lr, sp, #2048
	str	r0, [sp, #108]                  @ 4-byte Spill
	add	r0, lr, #328
	ldr	r5, [r3, #-4]
	mov	r4, r3
	str	r1, [sp, #136]                  @ 4-byte Spill
	str	r5, [sp, #128]                  @ 4-byte Spill
	str	r3, [sp, #140]                  @ 4-byte Spill
	str	r7, [sp, #132]                  @ 4-byte Spill
	bl	mulPv512x32
	ldr	r0, [sp, #2376]
	str	r0, [sp, #72]                   @ 4-byte Spill
	ldr	r1, [sp, #2380]
	str	r1, [sp, #96]                   @ 4-byte Spill
	mul	r2, r5, r0
	ldr	r1, [sp, #2384]
	str	r1, [sp, #92]                   @ 4-byte Spill
	ldr	r1, [sp, #2388]
	str	r1, [sp, #88]                   @ 4-byte Spill
	mov	r1, r4
	ldr	r0, [sp, #2440]
	str	r0, [sp, #124]                  @ 4-byte Spill
	ldr	r0, [sp, #2436]
	str	r0, [sp, #120]                  @ 4-byte Spill
	ldr	r0, [sp, #2432]
	str	r0, [sp, #116]                  @ 4-byte Spill
	ldr	r0, [sp, #2428]
	str	r0, [sp, #112]                  @ 4-byte Spill
	ldr	r0, [sp, #2424]
	str	r0, [sp, #104]                  @ 4-byte Spill
	ldr	r0, [sp, #2420]
	str	r0, [sp, #100]                  @ 4-byte Spill
	ldr	r0, [sp, #2416]
	str	r0, [sp, #84]                   @ 4-byte Spill
	ldr	r0, [sp, #2412]
	str	r0, [sp, #80]                   @ 4-byte Spill
	ldr	r0, [sp, #2408]
	str	r0, [sp, #76]                   @ 4-byte Spill
	ldr	r0, [sp, #2404]
	str	r0, [sp, #68]                   @ 4-byte Spill
	ldr	r0, [sp, #2400]
	str	r0, [sp, #64]                   @ 4-byte Spill
	ldr	r0, [sp, #2396]
	str	r0, [sp, #56]                   @ 4-byte Spill
	ldr	r0, [sp, #2392]
	str	r0, [sp, #48]                   @ 4-byte Spill
	add	r0, sp, #2304
	bl	mulPv512x32
	ldr	r0, [sp, #2368]
	add	lr, sp, #2048
	str	r0, [sp, #60]                   @ 4-byte Spill
	ldr	r0, [sp, #2364]
	str	r0, [sp, #52]                   @ 4-byte Spill
	ldr	r0, [sp, #2360]
	str	r0, [sp, #44]                   @ 4-byte Spill
	ldr	r0, [sp, #2356]
	str	r0, [sp, #40]                   @ 4-byte Spill
	ldr	r0, [sp, #2352]
	str	r0, [sp, #36]                   @ 4-byte Spill
	ldr	r0, [sp, #2348]
	str	r0, [sp, #32]                   @ 4-byte Spill
	ldr	r0, [sp, #2344]
	str	r0, [sp, #28]                   @ 4-byte Spill
	ldr	r0, [sp, #2340]
	str	r0, [sp, #24]                   @ 4-byte Spill
	ldr	r0, [sp, #2336]
	ldr	r2, [r7, #4]
	ldr	r1, [sp, #136]                  @ 4-byte Reload
	str	r0, [sp, #20]                   @ 4-byte Spill
	add	r0, lr, #184
	ldr	r10, [sp, #2332]
	ldr	r5, [sp, #2328]
	ldr	r8, [sp, #2324]
	ldr	r9, [sp, #2320]
	ldr	r11, [sp, #2304]
	ldr	r4, [sp, #2308]
	ldr	r6, [sp, #2312]
	ldr	r7, [sp, #2316]
	bl	mulPv512x32
	ldr	r0, [sp, #72]                   @ 4-byte Reload
	ldr	r1, [sp, #76]                   @ 4-byte Reload
	adds	r0, r11, r0
	ldr	r2, [sp, #20]                   @ 4-byte Reload
	ldr	r0, [sp, #96]                   @ 4-byte Reload
	adcs	r0, r4, r0
	str	r0, [sp, #96]                   @ 4-byte Spill
	ldr	r0, [sp, #92]                   @ 4-byte Reload
	ldr	r11, [sp, #96]                  @ 4-byte Reload
	adcs	r0, r6, r0
	str	r0, [sp, #92]                   @ 4-byte Spill
	ldr	r0, [sp, #88]                   @ 4-byte Reload
	ldr	r6, [sp, #92]                   @ 4-byte Reload
	adcs	r0, r7, r0
	str	r0, [sp, #88]                   @ 4-byte Spill
	ldr	r0, [sp, #48]                   @ 4-byte Reload
	adcs	r0, r9, r0
	str	r0, [sp, #72]                   @ 4-byte Spill
	ldr	r0, [sp, #56]                   @ 4-byte Reload
	adcs	r0, r8, r0
	str	r0, [sp, #56]                   @ 4-byte Spill
	ldr	r0, [sp, #64]                   @ 4-byte Reload
	adcs	r0, r5, r0
	str	r0, [sp, #64]                   @ 4-byte Spill
	ldr	r0, [sp, #68]                   @ 4-byte Reload
	adcs	r0, r10, r0
	str	r0, [sp, #68]                   @ 4-byte Spill
	adcs	r1, r2, r1
	str	r1, [sp, #76]                   @ 4-byte Spill
	ldr	r1, [sp, #80]                   @ 4-byte Reload
	mov	r0, #0
	ldr	r2, [sp, #24]                   @ 4-byte Reload
	adcs	r1, r2, r1
	str	r1, [sp, #80]                   @ 4-byte Spill
	ldr	r1, [sp, #84]                   @ 4-byte Reload
	ldr	r2, [sp, #28]                   @ 4-byte Reload
	adcs	r1, r2, r1
	str	r1, [sp, #84]                   @ 4-byte Spill
	ldr	r1, [sp, #100]                  @ 4-byte Reload
	ldr	r2, [sp, #32]                   @ 4-byte Reload
	adcs	r1, r2, r1
	str	r1, [sp, #48]                   @ 4-byte Spill
	ldr	r1, [sp, #104]                  @ 4-byte Reload
	ldr	r2, [sp, #36]                   @ 4-byte Reload
	adcs	r1, r2, r1
	str	r1, [sp, #36]                   @ 4-byte Spill
	ldr	r1, [sp, #112]                  @ 4-byte Reload
	ldr	r2, [sp, #40]                   @ 4-byte Reload
	adcs	r1, r2, r1
	str	r1, [sp, #40]                   @ 4-byte Spill
	ldr	r1, [sp, #116]                  @ 4-byte Reload
	ldr	r2, [sp, #44]                   @ 4-byte Reload
	adcs	r1, r2, r1
	str	r1, [sp, #44]                   @ 4-byte Spill
	ldr	r1, [sp, #120]                  @ 4-byte Reload
	ldr	r2, [sp, #52]                   @ 4-byte Reload
	adcs	r1, r2, r1
	str	r1, [sp, #52]                   @ 4-byte Spill
	ldr	r1, [sp, #124]                  @ 4-byte Reload
	ldr	r2, [sp, #60]                   @ 4-byte Reload
	adcs	r1, r2, r1
	str	r1, [sp, #60]                   @ 4-byte Spill
	adc	r0, r0, #0
	str	r0, [sp, #32]                   @ 4-byte Spill
	ldr	r0, [sp, #2296]
	str	r0, [sp, #28]                   @ 4-byte Spill
	ldr	r0, [sp, #2292]
	str	r0, [sp, #20]                   @ 4-byte Spill
	ldr	r0, [sp, #2288]
	str	r0, [sp, #16]                   @ 4-byte Spill
	ldr	r0, [sp, #2284]
	str	r0, [sp, #12]                   @ 4-byte Spill
	ldr	r0, [sp, #2280]
	str	r0, [sp, #8]                    @ 4-byte Spill
	ldr	r7, [sp, #2232]
	ldr	r0, [sp, #2236]
	adds	r7, r11, r7
	ldr	r10, [sp, #2276]
	adcs	r0, r6, r0
	ldr	r9, [sp, #2272]
	ldr	r8, [sp, #2268]
	ldr	r5, [sp, #2264]
	ldr	r4, [sp, #2260]
	ldr	lr, [sp, #2256]
	ldr	r12, [sp, #2252]
	ldr	r3, [sp, #2248]
	ldr	r1, [sp, #2240]
	ldr	r2, [sp, #2244]
	str	r0, [sp, #124]                  @ 4-byte Spill
	ldr	r0, [sp, #88]                   @ 4-byte Reload
	str	r7, [sp, #24]                   @ 4-byte Spill
	adcs	r0, r0, r1
	str	r0, [sp, #120]                  @ 4-byte Spill
	ldr	r0, [sp, #72]                   @ 4-byte Reload
	ldr	r1, [sp, #8]                    @ 4-byte Reload
	adcs	r0, r0, r2
	str	r0, [sp, #116]                  @ 4-byte Spill
	ldr	r0, [sp, #56]                   @ 4-byte Reload
	adcs	r0, r0, r3
	str	r0, [sp, #112]                  @ 4-byte Spill
	ldr	r0, [sp, #64]                   @ 4-byte Reload
	adcs	r0, r0, r12
	str	r0, [sp, #104]                  @ 4-byte Spill
	ldr	r0, [sp, #68]                   @ 4-byte Reload
	adcs	r0, r0, lr
	str	r0, [sp, #100]                  @ 4-byte Spill
	ldr	r0, [sp, #76]                   @ 4-byte Reload
	adcs	r0, r0, r4
	str	r0, [sp, #96]                   @ 4-byte Spill
	ldr	r0, [sp, #80]                   @ 4-byte Reload
	adcs	r0, r0, r5
	str	r0, [sp, #92]                   @ 4-byte Spill
	ldr	r0, [sp, #84]                   @ 4-byte Reload
	adcs	r0, r0, r8
	str	r0, [sp, #88]                   @ 4-byte Spill
	ldr	r0, [sp, #48]                   @ 4-byte Reload
	adcs	r0, r0, r9
	str	r0, [sp, #84]                   @ 4-byte Spill
	ldr	r0, [sp, #36]                   @ 4-byte Reload
	adcs	r0, r0, r10
	str	r0, [sp, #80]                   @ 4-byte Spill
	ldr	r0, [sp, #40]                   @ 4-byte Reload
	adcs	r0, r0, r1
	str	r0, [sp, #76]                   @ 4-byte Spill
	ldr	r0, [sp, #44]                   @ 4-byte Reload
	ldr	r1, [sp, #12]                   @ 4-byte Reload
	adcs	r0, r0, r1
	str	r0, [sp, #72]                   @ 4-byte Spill
	ldr	r0, [sp, #52]                   @ 4-byte Reload
	ldr	r1, [sp, #16]                   @ 4-byte Reload
	adcs	r0, r0, r1
	str	r0, [sp, #68]                   @ 4-byte Spill
	ldr	r0, [sp, #60]                   @ 4-byte Reload
	ldr	r1, [sp, #20]                   @ 4-byte Reload
	adcs	r0, r0, r1
	str	r0, [sp, #64]                   @ 4-byte Spill
	ldr	r0, [sp, #32]                   @ 4-byte Reload
	ldr	r1, [sp, #28]                   @ 4-byte Reload
	adcs	r0, r0, r1
	str	r0, [sp, #60]                   @ 4-byte Spill
	mov	r0, #0
	ldr	r1, [sp, #140]                  @ 4-byte Reload
	adc	r0, r0, #0
	str	r0, [sp, #56]                   @ 4-byte Spill
	ldr	r0, [sp, #128]                  @ 4-byte Reload
	mul	r2, r0, r7
	add	r0, sp, #2160
	bl	mulPv512x32
	ldr	r0, [sp, #132]                  @ 4-byte Reload
	add	r11, sp, #2160
	add	lr, sp, #2048
	ldr	r1, [sp, #136]                  @ 4-byte Reload
	ldr	r2, [r0, #8]
	ldr	r0, [sp, #2224]
	str	r0, [sp, #52]                   @ 4-byte Spill
	ldr	r0, [sp, #2220]
	str	r0, [sp, #48]                   @ 4-byte Spill
	ldr	r0, [sp, #2216]
	str	r0, [sp, #44]                   @ 4-byte Spill
	ldr	r0, [sp, #2212]
	str	r0, [sp, #40]                   @ 4-byte Spill
	ldr	r0, [sp, #2208]
	str	r0, [sp, #36]                   @ 4-byte Spill
	ldr	r0, [sp, #2204]
	str	r0, [sp, #32]                   @ 4-byte Spill
	ldr	r0, [sp, #2200]
	str	r0, [sp, #28]                   @ 4-byte Spill
	ldr	r0, [sp, #2196]
	str	r0, [sp, #20]                   @ 4-byte Spill
	ldr	r0, [sp, #2192]
	str	r0, [sp, #16]                   @ 4-byte Spill
	add	r0, lr, #40
	ldm	r11, {r8, r9, r10, r11}
	ldr	r4, [sp, #2188]
	ldr	r5, [sp, #2184]
	ldr	r6, [sp, #2180]
	ldr	r7, [sp, #2176]
	bl	mulPv512x32
	ldr	r0, [sp, #24]                   @ 4-byte Reload
	ldr	r1, [sp, #16]                   @ 4-byte Reload
	adds	r0, r0, r8
	ldr	r0, [sp, #124]                  @ 4-byte Reload
	adcs	r0, r0, r9
	str	r0, [sp, #124]                  @ 4-byte Spill
	ldr	r0, [sp, #120]                  @ 4-byte Reload
	adcs	r0, r0, r10
	str	r0, [sp, #120]                  @ 4-byte Spill
	ldr	r0, [sp, #116]                  @ 4-byte Reload
	add	r10, sp, #2128
	adcs	r0, r0, r11
	str	r0, [sp, #116]                  @ 4-byte Spill
	ldr	r0, [sp, #112]                  @ 4-byte Reload
	adcs	r0, r0, r7
	str	r0, [sp, #112]                  @ 4-byte Spill
	ldr	r0, [sp, #104]                  @ 4-byte Reload
	adcs	r0, r0, r6
	str	r0, [sp, #104]                  @ 4-byte Spill
	ldr	r0, [sp, #100]                  @ 4-byte Reload
	ldr	r6, [sp, #124]                  @ 4-byte Reload
	adcs	r0, r0, r5
	str	r0, [sp, #100]                  @ 4-byte Spill
	ldr	r0, [sp, #96]                   @ 4-byte Reload
	adcs	r0, r0, r4
	str	r0, [sp, #96]                   @ 4-byte Spill
	ldr	r0, [sp, #92]                   @ 4-byte Reload
	adcs	r0, r0, r1
	str	r0, [sp, #92]                   @ 4-byte Spill
	ldr	r0, [sp, #88]                   @ 4-byte Reload
	ldr	r1, [sp, #20]                   @ 4-byte Reload
	adcs	r0, r0, r1
	str	r0, [sp, #88]                   @ 4-byte Spill
	ldr	r0, [sp, #84]                   @ 4-byte Reload
	ldr	r1, [sp, #28]                   @ 4-byte Reload
	adcs	r0, r0, r1
	str	r0, [sp, #84]                   @ 4-byte Spill
	ldr	r0, [sp, #80]                   @ 4-byte Reload
	ldr	r1, [sp, #32]                   @ 4-byte Reload
	adcs	r0, r0, r1
	str	r0, [sp, #80]                   @ 4-byte Spill
	ldr	r0, [sp, #76]                   @ 4-byte Reload
	ldr	r1, [sp, #36]                   @ 4-byte Reload
	adcs	r0, r0, r1
	str	r0, [sp, #76]                   @ 4-byte Spill
	ldr	r0, [sp, #72]                   @ 4-byte Reload
	ldr	r1, [sp, #40]                   @ 4-byte Reload
	adcs	r0, r0, r1
	str	r0, [sp, #72]                   @ 4-byte Spill
	ldr	r0, [sp, #68]                   @ 4-byte Reload
	ldr	r1, [sp, #44]                   @ 4-byte Reload
	adcs	r0, r0, r1
	str	r0, [sp, #68]                   @ 4-byte Spill
	ldr	r0, [sp, #64]                   @ 4-byte Reload
	ldr	r1, [sp, #48]                   @ 4-byte Reload
	adcs	r0, r0, r1
	str	r0, [sp, #64]                   @ 4-byte Spill
	ldr	r0, [sp, #60]                   @ 4-byte Reload
	ldr	r1, [sp, #52]                   @ 4-byte Reload
	adcs	r0, r0, r1
	str	r0, [sp, #60]                   @ 4-byte Spill
	ldr	r0, [sp, #56]                   @ 4-byte Reload
	adc	r0, r0, #0
	str	r0, [sp, #56]                   @ 4-byte Spill
	ldr	r0, [sp, #2152]
	str	r0, [sp, #52]                   @ 4-byte Spill
	ldr	r0, [sp, #2148]
	str	r0, [sp, #48]                   @ 4-byte Spill
	ldr	r0, [sp, #2144]
	str	r0, [sp, #44]                   @ 4-byte Spill
	ldr	r0, [sp, #2140]
	str	r0, [sp, #40]                   @ 4-byte Spill
	ldr	r7, [sp, #2088]
	ldr	r0, [sp, #2092]
	adds	r7, r6, r7
	ldr	r6, [sp, #120]                  @ 4-byte Reload
	ldm	r10, {r8, r9, r10}
	adcs	r0, r6, r0
	ldr	r11, [sp, #2124]
	ldr	r5, [sp, #2120]
	ldr	r4, [sp, #2116]
	ldr	lr, [sp, #2112]
	ldr	r12, [sp, #2108]
	ldr	r3, [sp, #2104]
	ldr	r1, [sp, #2096]
	ldr	r2, [sp, #2100]
	str	r0, [sp, #124]                  @ 4-byte Spill
	ldr	r0, [sp, #116]                  @ 4-byte Reload
	str	r7, [sp, #36]                   @ 4-byte Spill
	adcs	r0, r0, r1
	str	r0, [sp, #120]                  @ 4-byte Spill
	ldr	r0, [sp, #112]                  @ 4-byte Reload
	ldr	r1, [sp, #40]                   @ 4-byte Reload
	adcs	r0, r0, r2
	str	r0, [sp, #116]                  @ 4-byte Spill
	ldr	r0, [sp, #104]                  @ 4-byte Reload
	adcs	r0, r0, r3
	str	r0, [sp, #112]                  @ 4-byte Spill
	ldr	r0, [sp, #100]                  @ 4-byte Reload
	adcs	r0, r0, r12
	str	r0, [sp, #104]                  @ 4-byte Spill
	ldr	r0, [sp, #96]                   @ 4-byte Reload
	adcs	r0, r0, lr
	str	r0, [sp, #100]                  @ 4-byte Spill
	ldr	r0, [sp, #92]                   @ 4-byte Reload
	adcs	r0, r0, r4
	str	r0, [sp, #96]                   @ 4-byte Spill
	ldr	r0, [sp, #88]                   @ 4-byte Reload
	adcs	r0, r0, r5
	str	r0, [sp, #92]                   @ 4-byte Spill
	ldr	r0, [sp, #84]                   @ 4-byte Reload
	adcs	r0, r0, r11
	str	r0, [sp, #88]                   @ 4-byte Spill
	ldr	r0, [sp, #80]                   @ 4-byte Reload
	adcs	r0, r0, r8
	str	r0, [sp, #84]                   @ 4-byte Spill
	ldr	r0, [sp, #76]                   @ 4-byte Reload
	adcs	r0, r0, r9
	str	r0, [sp, #80]                   @ 4-byte Spill
	ldr	r0, [sp, #72]                   @ 4-byte Reload
	adcs	r0, r0, r10
	str	r0, [sp, #76]                   @ 4-byte Spill
	ldr	r0, [sp, #68]                   @ 4-byte Reload
	adcs	r0, r0, r1
	str	r0, [sp, #72]                   @ 4-byte Spill
	ldr	r0, [sp, #64]                   @ 4-byte Reload
	ldr	r1, [sp, #44]                   @ 4-byte Reload
	adcs	r0, r0, r1
	str	r0, [sp, #68]                   @ 4-byte Spill
	ldr	r0, [sp, #60]                   @ 4-byte Reload
	ldr	r1, [sp, #48]                   @ 4-byte Reload
	adcs	r0, r0, r1
	str	r0, [sp, #64]                   @ 4-byte Spill
	ldr	r0, [sp, #56]                   @ 4-byte Reload
	ldr	r1, [sp, #52]                   @ 4-byte Reload
	adcs	r0, r0, r1
	str	r0, [sp, #60]                   @ 4-byte Spill
	mov	r0, #0
	ldr	r1, [sp, #140]                  @ 4-byte Reload
	adc	r0, r0, #0
	str	r0, [sp, #56]                   @ 4-byte Spill
	ldr	r0, [sp, #128]                  @ 4-byte Reload
	mul	r2, r0, r7
	add	r0, sp, #2016
	bl	mulPv512x32
	ldr	r0, [sp, #132]                  @ 4-byte Reload
	add	r11, sp, #2016
	add	lr, sp, #1024
	ldr	r1, [sp, #136]                  @ 4-byte Reload
	ldr	r2, [r0, #12]
	ldr	r0, [sp, #2080]
	str	r0, [sp, #52]                   @ 4-byte Spill
	ldr	r0, [sp, #2076]
	str	r0, [sp, #48]                   @ 4-byte Spill
	ldr	r0, [sp, #2072]
	str	r0, [sp, #44]                   @ 4-byte Spill
	ldr	r0, [sp, #2068]
	str	r0, [sp, #40]                   @ 4-byte Spill
	ldr	r0, [sp, #2064]
	str	r0, [sp, #32]                   @ 4-byte Spill
	ldr	r0, [sp, #2060]
	str	r0, [sp, #28]                   @ 4-byte Spill
	ldr	r0, [sp, #2056]
	str	r0, [sp, #24]                   @ 4-byte Spill
	ldr	r0, [sp, #2052]
	str	r0, [sp, #20]                   @ 4-byte Spill
	ldr	r0, [sp, #2048]
	str	r0, [sp, #16]                   @ 4-byte Spill
	add	r0, lr, #920
	ldm	r11, {r8, r9, r10, r11}
	ldr	r4, [sp, #2044]
	ldr	r5, [sp, #2040]
	ldr	r6, [sp, #2036]
	ldr	r7, [sp, #2032]
	bl	mulPv512x32
	ldr	r0, [sp, #36]                   @ 4-byte Reload
	ldr	r1, [sp, #16]                   @ 4-byte Reload
	adds	r0, r0, r8
	ldr	r0, [sp, #124]                  @ 4-byte Reload
	adcs	r0, r0, r9
	str	r0, [sp, #124]                  @ 4-byte Spill
	ldr	r0, [sp, #120]                  @ 4-byte Reload
	adcs	r0, r0, r10
	str	r0, [sp, #120]                  @ 4-byte Spill
	ldr	r0, [sp, #116]                  @ 4-byte Reload
	add	r10, sp, #1984
	adcs	r0, r0, r11
	str	r0, [sp, #116]                  @ 4-byte Spill
	ldr	r0, [sp, #112]                  @ 4-byte Reload
	adcs	r0, r0, r7
	str	r0, [sp, #112]                  @ 4-byte Spill
	ldr	r0, [sp, #104]                  @ 4-byte Reload
	adcs	r0, r0, r6
	str	r0, [sp, #104]                  @ 4-byte Spill
	ldr	r0, [sp, #100]                  @ 4-byte Reload
	ldr	r6, [sp, #124]                  @ 4-byte Reload
	adcs	r0, r0, r5
	str	r0, [sp, #100]                  @ 4-byte Spill
	ldr	r0, [sp, #96]                   @ 4-byte Reload
	adcs	r0, r0, r4
	str	r0, [sp, #96]                   @ 4-byte Spill
	ldr	r0, [sp, #92]                   @ 4-byte Reload
	adcs	r0, r0, r1
	str	r0, [sp, #92]                   @ 4-byte Spill
	ldr	r0, [sp, #88]                   @ 4-byte Reload
	ldr	r1, [sp, #20]                   @ 4-byte Reload
	adcs	r0, r0, r1
	str	r0, [sp, #88]                   @ 4-byte Spill
	ldr	r0, [sp, #84]                   @ 4-byte Reload
	ldr	r1, [sp, #24]                   @ 4-byte Reload
	adcs	r0, r0, r1
	str	r0, [sp, #84]                   @ 4-byte Spill
	ldr	r0, [sp, #80]                   @ 4-byte Reload
	ldr	r1, [sp, #28]                   @ 4-byte Reload
	adcs	r0, r0, r1
	str	r0, [sp, #80]                   @ 4-byte Spill
	ldr	r0, [sp, #76]                   @ 4-byte Reload
	ldr	r1, [sp, #32]                   @ 4-byte Reload
	adcs	r0, r0, r1
	str	r0, [sp, #76]                   @ 4-byte Spill
	ldr	r0, [sp, #72]                   @ 4-byte Reload
	ldr	r1, [sp, #40]                   @ 4-byte Reload
	adcs	r0, r0, r1
	str	r0, [sp, #72]                   @ 4-byte Spill
	ldr	r0, [sp, #68]                   @ 4-byte Reload
	ldr	r1, [sp, #44]                   @ 4-byte Reload
	adcs	r0, r0, r1
	str	r0, [sp, #68]                   @ 4-byte Spill
	ldr	r0, [sp, #64]                   @ 4-byte Reload
	ldr	r1, [sp, #48]                   @ 4-byte Reload
	adcs	r0, r0, r1
	str	r0, [sp, #64]                   @ 4-byte Spill
	ldr	r0, [sp, #60]                   @ 4-byte Reload
	ldr	r1, [sp, #52]                   @ 4-byte Reload
	adcs	r0, r0, r1
	str	r0, [sp, #60]                   @ 4-byte Spill
	ldr	r0, [sp, #56]                   @ 4-byte Reload
	adc	r0, r0, #0
	str	r0, [sp, #56]                   @ 4-byte Spill
	ldr	r0, [sp, #2008]
	str	r0, [sp, #52]                   @ 4-byte Spill
	ldr	r0, [sp, #2004]
	str	r0, [sp, #48]                   @ 4-byte Spill
	ldr	r0, [sp, #2000]
	str	r0, [sp, #44]                   @ 4-byte Spill
	ldr	r0, [sp, #1996]
	str	r0, [sp, #40]                   @ 4-byte Spill
	ldr	r7, [sp, #1944]
	ldr	r0, [sp, #1948]
	adds	r7, r6, r7
	ldr	r6, [sp, #120]                  @ 4-byte Reload
	ldm	r10, {r8, r9, r10}
	adcs	r0, r6, r0
	ldr	r11, [sp, #1980]
	ldr	r5, [sp, #1976]
	ldr	r4, [sp, #1972]
	ldr	lr, [sp, #1968]
	ldr	r12, [sp, #1964]
	ldr	r3, [sp, #1960]
	ldr	r1, [sp, #1952]
	ldr	r2, [sp, #1956]
	str	r0, [sp, #124]                  @ 4-byte Spill
	ldr	r0, [sp, #116]                  @ 4-byte Reload
	str	r7, [sp, #36]                   @ 4-byte Spill
	adcs	r0, r0, r1
	str	r0, [sp, #120]                  @ 4-byte Spill
	ldr	r0, [sp, #112]                  @ 4-byte Reload
	ldr	r1, [sp, #40]                   @ 4-byte Reload
	adcs	r0, r0, r2
	str	r0, [sp, #116]                  @ 4-byte Spill
	ldr	r0, [sp, #104]                  @ 4-byte Reload
	adcs	r0, r0, r3
	str	r0, [sp, #112]                  @ 4-byte Spill
	ldr	r0, [sp, #100]                  @ 4-byte Reload
	adcs	r0, r0, r12
	str	r0, [sp, #104]                  @ 4-byte Spill
	ldr	r0, [sp, #96]                   @ 4-byte Reload
	adcs	r0, r0, lr
	str	r0, [sp, #100]                  @ 4-byte Spill
	ldr	r0, [sp, #92]                   @ 4-byte Reload
	adcs	r0, r0, r4
	str	r0, [sp, #96]                   @ 4-byte Spill
	ldr	r0, [sp, #88]                   @ 4-byte Reload
	adcs	r0, r0, r5
	str	r0, [sp, #92]                   @ 4-byte Spill
	ldr	r0, [sp, #84]                   @ 4-byte Reload
	adcs	r0, r0, r11
	str	r0, [sp, #88]                   @ 4-byte Spill
	ldr	r0, [sp, #80]                   @ 4-byte Reload
	adcs	r0, r0, r8
	str	r0, [sp, #84]                   @ 4-byte Spill
	ldr	r0, [sp, #76]                   @ 4-byte Reload
	adcs	r0, r0, r9
	str	r0, [sp, #80]                   @ 4-byte Spill
	ldr	r0, [sp, #72]                   @ 4-byte Reload
	adcs	r0, r0, r10
	str	r0, [sp, #76]                   @ 4-byte Spill
	ldr	r0, [sp, #68]                   @ 4-byte Reload
	adcs	r0, r0, r1
	str	r0, [sp, #72]                   @ 4-byte Spill
	ldr	r0, [sp, #64]                   @ 4-byte Reload
	ldr	r1, [sp, #44]                   @ 4-byte Reload
	adcs	r0, r0, r1
	str	r0, [sp, #68]                   @ 4-byte Spill
	ldr	r0, [sp, #60]                   @ 4-byte Reload
	ldr	r1, [sp, #48]                   @ 4-byte Reload
	adcs	r0, r0, r1
	str	r0, [sp, #64]                   @ 4-byte Spill
	ldr	r0, [sp, #56]                   @ 4-byte Reload
	ldr	r1, [sp, #52]                   @ 4-byte Reload
	adcs	r0, r0, r1
	str	r0, [sp, #60]                   @ 4-byte Spill
	mov	r0, #0
	ldr	r1, [sp, #140]                  @ 4-byte Reload
	adc	r0, r0, #0
	str	r0, [sp, #56]                   @ 4-byte Spill
	ldr	r0, [sp, #128]                  @ 4-byte Reload
	mul	r2, r0, r7
	add	r0, sp, #1872
	bl	mulPv512x32
	ldr	r0, [sp, #132]                  @ 4-byte Reload
	add	r11, sp, #1872
	add	lr, sp, #1024
	ldr	r1, [sp, #136]                  @ 4-byte Reload
	ldr	r2, [r0, #16]
	ldr	r0, [sp, #1936]
	str	r0, [sp, #52]                   @ 4-byte Spill
	ldr	r0, [sp, #1932]
	str	r0, [sp, #48]                   @ 4-byte Spill
	ldr	r0, [sp, #1928]
	str	r0, [sp, #44]                   @ 4-byte Spill
	ldr	r0, [sp, #1924]
	str	r0, [sp, #40]                   @ 4-byte Spill
	ldr	r0, [sp, #1920]
	str	r0, [sp, #32]                   @ 4-byte Spill
	ldr	r0, [sp, #1916]
	str	r0, [sp, #28]                   @ 4-byte Spill
	ldr	r0, [sp, #1912]
	str	r0, [sp, #24]                   @ 4-byte Spill
	ldr	r0, [sp, #1908]
	str	r0, [sp, #20]                   @ 4-byte Spill
	ldr	r0, [sp, #1904]
	str	r0, [sp, #16]                   @ 4-byte Spill
	add	r0, lr, #776
	ldm	r11, {r8, r9, r10, r11}
	ldr	r4, [sp, #1900]
	ldr	r5, [sp, #1896]
	ldr	r6, [sp, #1892]
	ldr	r7, [sp, #1888]
	bl	mulPv512x32
	ldr	r0, [sp, #36]                   @ 4-byte Reload
	ldr	r1, [sp, #16]                   @ 4-byte Reload
	adds	r0, r0, r8
	ldr	r0, [sp, #124]                  @ 4-byte Reload
	adcs	r0, r0, r9
	str	r0, [sp, #124]                  @ 4-byte Spill
	ldr	r0, [sp, #120]                  @ 4-byte Reload
	adcs	r0, r0, r10
	str	r0, [sp, #120]                  @ 4-byte Spill
	ldr	r0, [sp, #116]                  @ 4-byte Reload
	add	r10, sp, #1840
	adcs	r0, r0, r11
	str	r0, [sp, #116]                  @ 4-byte Spill
	ldr	r0, [sp, #112]                  @ 4-byte Reload
	adcs	r0, r0, r7
	str	r0, [sp, #112]                  @ 4-byte Spill
	ldr	r0, [sp, #104]                  @ 4-byte Reload
	adcs	r0, r0, r6
	str	r0, [sp, #104]                  @ 4-byte Spill
	ldr	r0, [sp, #100]                  @ 4-byte Reload
	ldr	r6, [sp, #124]                  @ 4-byte Reload
	adcs	r0, r0, r5
	str	r0, [sp, #100]                  @ 4-byte Spill
	ldr	r0, [sp, #96]                   @ 4-byte Reload
	adcs	r0, r0, r4
	str	r0, [sp, #96]                   @ 4-byte Spill
	ldr	r0, [sp, #92]                   @ 4-byte Reload
	adcs	r0, r0, r1
	str	r0, [sp, #92]                   @ 4-byte Spill
	ldr	r0, [sp, #88]                   @ 4-byte Reload
	ldr	r1, [sp, #20]                   @ 4-byte Reload
	adcs	r0, r0, r1
	str	r0, [sp, #88]                   @ 4-byte Spill
	ldr	r0, [sp, #84]                   @ 4-byte Reload
	ldr	r1, [sp, #24]                   @ 4-byte Reload
	adcs	r0, r0, r1
	str	r0, [sp, #84]                   @ 4-byte Spill
	ldr	r0, [sp, #80]                   @ 4-byte Reload
	ldr	r1, [sp, #28]                   @ 4-byte Reload
	adcs	r0, r0, r1
	str	r0, [sp, #80]                   @ 4-byte Spill
	ldr	r0, [sp, #76]                   @ 4-byte Reload
	ldr	r1, [sp, #32]                   @ 4-byte Reload
	adcs	r0, r0, r1
	str	r0, [sp, #76]                   @ 4-byte Spill
	ldr	r0, [sp, #72]                   @ 4-byte Reload
	ldr	r1, [sp, #40]                   @ 4-byte Reload
	adcs	r0, r0, r1
	str	r0, [sp, #72]                   @ 4-byte Spill
	ldr	r0, [sp, #68]                   @ 4-byte Reload
	ldr	r1, [sp, #44]                   @ 4-byte Reload
	adcs	r0, r0, r1
	str	r0, [sp, #68]                   @ 4-byte Spill
	ldr	r0, [sp, #64]                   @ 4-byte Reload
	ldr	r1, [sp, #48]                   @ 4-byte Reload
	adcs	r0, r0, r1
	str	r0, [sp, #64]                   @ 4-byte Spill
	ldr	r0, [sp, #60]                   @ 4-byte Reload
	ldr	r1, [sp, #52]                   @ 4-byte Reload
	adcs	r0, r0, r1
	str	r0, [sp, #60]                   @ 4-byte Spill
	ldr	r0, [sp, #56]                   @ 4-byte Reload
	adc	r0, r0, #0
	str	r0, [sp, #56]                   @ 4-byte Spill
	ldr	r0, [sp, #1864]
	str	r0, [sp, #52]                   @ 4-byte Spill
	ldr	r0, [sp, #1860]
	str	r0, [sp, #48]                   @ 4-byte Spill
	ldr	r0, [sp, #1856]
	str	r0, [sp, #44]                   @ 4-byte Spill
	ldr	r0, [sp, #1852]
	str	r0, [sp, #40]                   @ 4-byte Spill
	ldr	r7, [sp, #1800]
	ldr	r0, [sp, #1804]
	adds	r7, r6, r7
	ldr	r6, [sp, #120]                  @ 4-byte Reload
	ldm	r10, {r8, r9, r10}
	adcs	r0, r6, r0
	ldr	r11, [sp, #1836]
	ldr	r5, [sp, #1832]
	ldr	r4, [sp, #1828]
	ldr	lr, [sp, #1824]
	ldr	r12, [sp, #1820]
	ldr	r3, [sp, #1816]
	ldr	r1, [sp, #1808]
	ldr	r2, [sp, #1812]
	str	r0, [sp, #124]                  @ 4-byte Spill
	ldr	r0, [sp, #116]                  @ 4-byte Reload
	str	r7, [sp, #36]                   @ 4-byte Spill
	adcs	r0, r0, r1
	str	r0, [sp, #120]                  @ 4-byte Spill
	ldr	r0, [sp, #112]                  @ 4-byte Reload
	ldr	r1, [sp, #40]                   @ 4-byte Reload
	adcs	r0, r0, r2
	str	r0, [sp, #116]                  @ 4-byte Spill
	ldr	r0, [sp, #104]                  @ 4-byte Reload
	adcs	r0, r0, r3
	str	r0, [sp, #112]                  @ 4-byte Spill
	ldr	r0, [sp, #100]                  @ 4-byte Reload
	adcs	r0, r0, r12
	str	r0, [sp, #104]                  @ 4-byte Spill
	ldr	r0, [sp, #96]                   @ 4-byte Reload
	adcs	r0, r0, lr
	str	r0, [sp, #100]                  @ 4-byte Spill
	ldr	r0, [sp, #92]                   @ 4-byte Reload
	adcs	r0, r0, r4
	str	r0, [sp, #96]                   @ 4-byte Spill
	ldr	r0, [sp, #88]                   @ 4-byte Reload
	adcs	r0, r0, r5
	str	r0, [sp, #92]                   @ 4-byte Spill
	ldr	r0, [sp, #84]                   @ 4-byte Reload
	adcs	r0, r0, r11
	str	r0, [sp, #88]                   @ 4-byte Spill
	ldr	r0, [sp, #80]                   @ 4-byte Reload
	adcs	r0, r0, r8
	str	r0, [sp, #84]                   @ 4-byte Spill
	ldr	r0, [sp, #76]                   @ 4-byte Reload
	adcs	r0, r0, r9
	str	r0, [sp, #80]                   @ 4-byte Spill
	ldr	r0, [sp, #72]                   @ 4-byte Reload
	adcs	r0, r0, r10
	str	r0, [sp, #76]                   @ 4-byte Spill
	ldr	r0, [sp, #68]                   @ 4-byte Reload
	adcs	r0, r0, r1
	str	r0, [sp, #72]                   @ 4-byte Spill
	ldr	r0, [sp, #64]                   @ 4-byte Reload
	ldr	r1, [sp, #44]                   @ 4-byte Reload
	adcs	r0, r0, r1
	str	r0, [sp, #68]                   @ 4-byte Spill
	ldr	r0, [sp, #60]                   @ 4-byte Reload
	ldr	r1, [sp, #48]                   @ 4-byte Reload
	adcs	r0, r0, r1
	str	r0, [sp, #64]                   @ 4-byte Spill
	ldr	r0, [sp, #56]                   @ 4-byte Reload
	ldr	r1, [sp, #52]                   @ 4-byte Reload
	adcs	r0, r0, r1
	str	r0, [sp, #60]                   @ 4-byte Spill
	mov	r0, #0
	ldr	r1, [sp, #140]                  @ 4-byte Reload
	adc	r0, r0, #0
	str	r0, [sp, #56]                   @ 4-byte Spill
	ldr	r0, [sp, #128]                  @ 4-byte Reload
	mul	r2, r0, r7
	add	r0, sp, #1728
	bl	mulPv512x32
	ldr	r0, [sp, #132]                  @ 4-byte Reload
	add	r11, sp, #1728
	add	lr, sp, #1024
	ldr	r1, [sp, #136]                  @ 4-byte Reload
	ldr	r2, [r0, #20]
	ldr	r0, [sp, #1792]
	str	r0, [sp, #52]                   @ 4-byte Spill
	ldr	r0, [sp, #1788]
	str	r0, [sp, #48]                   @ 4-byte Spill
	ldr	r0, [sp, #1784]
	str	r0, [sp, #44]                   @ 4-byte Spill
	ldr	r0, [sp, #1780]
	str	r0, [sp, #40]                   @ 4-byte Spill
	ldr	r0, [sp, #1776]
	str	r0, [sp, #32]                   @ 4-byte Spill
	ldr	r0, [sp, #1772]
	str	r0, [sp, #28]                   @ 4-byte Spill
	ldr	r0, [sp, #1768]
	str	r0, [sp, #24]                   @ 4-byte Spill
	ldr	r0, [sp, #1764]
	str	r0, [sp, #20]                   @ 4-byte Spill
	ldr	r0, [sp, #1760]
	str	r0, [sp, #16]                   @ 4-byte Spill
	add	r0, lr, #632
	ldm	r11, {r8, r9, r10, r11}
	ldr	r4, [sp, #1756]
	ldr	r5, [sp, #1752]
	ldr	r6, [sp, #1748]
	ldr	r7, [sp, #1744]
	bl	mulPv512x32
	ldr	r0, [sp, #36]                   @ 4-byte Reload
	ldr	r1, [sp, #16]                   @ 4-byte Reload
	adds	r0, r0, r8
	ldr	r0, [sp, #124]                  @ 4-byte Reload
	adcs	r0, r0, r9
	str	r0, [sp, #124]                  @ 4-byte Spill
	ldr	r0, [sp, #120]                  @ 4-byte Reload
	adcs	r0, r0, r10
	str	r0, [sp, #120]                  @ 4-byte Spill
	ldr	r0, [sp, #116]                  @ 4-byte Reload
	add	r10, sp, #1696
	adcs	r0, r0, r11
	str	r0, [sp, #116]                  @ 4-byte Spill
	ldr	r0, [sp, #112]                  @ 4-byte Reload
	adcs	r0, r0, r7
	str	r0, [sp, #112]                  @ 4-byte Spill
	ldr	r0, [sp, #104]                  @ 4-byte Reload
	adcs	r0, r0, r6
	str	r0, [sp, #104]                  @ 4-byte Spill
	ldr	r0, [sp, #100]                  @ 4-byte Reload
	ldr	r6, [sp, #124]                  @ 4-byte Reload
	adcs	r0, r0, r5
	str	r0, [sp, #100]                  @ 4-byte Spill
	ldr	r0, [sp, #96]                   @ 4-byte Reload
	adcs	r0, r0, r4
	str	r0, [sp, #96]                   @ 4-byte Spill
	ldr	r0, [sp, #92]                   @ 4-byte Reload
	adcs	r0, r0, r1
	str	r0, [sp, #92]                   @ 4-byte Spill
	ldr	r0, [sp, #88]                   @ 4-byte Reload
	ldr	r1, [sp, #20]                   @ 4-byte Reload
	adcs	r0, r0, r1
	str	r0, [sp, #88]                   @ 4-byte Spill
	ldr	r0, [sp, #84]                   @ 4-byte Reload
	ldr	r1, [sp, #24]                   @ 4-byte Reload
	adcs	r0, r0, r1
	str	r0, [sp, #84]                   @ 4-byte Spill
	ldr	r0, [sp, #80]                   @ 4-byte Reload
	ldr	r1, [sp, #28]                   @ 4-byte Reload
	adcs	r0, r0, r1
	str	r0, [sp, #80]                   @ 4-byte Spill
	ldr	r0, [sp, #76]                   @ 4-byte Reload
	ldr	r1, [sp, #32]                   @ 4-byte Reload
	adcs	r0, r0, r1
	str	r0, [sp, #76]                   @ 4-byte Spill
	ldr	r0, [sp, #72]                   @ 4-byte Reload
	ldr	r1, [sp, #40]                   @ 4-byte Reload
	adcs	r0, r0, r1
	str	r0, [sp, #72]                   @ 4-byte Spill
	ldr	r0, [sp, #68]                   @ 4-byte Reload
	ldr	r1, [sp, #44]                   @ 4-byte Reload
	adcs	r0, r0, r1
	str	r0, [sp, #68]                   @ 4-byte Spill
	ldr	r0, [sp, #64]                   @ 4-byte Reload
	ldr	r1, [sp, #48]                   @ 4-byte Reload
	adcs	r0, r0, r1
	str	r0, [sp, #64]                   @ 4-byte Spill
	ldr	r0, [sp, #60]                   @ 4-byte Reload
	ldr	r1, [sp, #52]                   @ 4-byte Reload
	adcs	r0, r0, r1
	str	r0, [sp, #60]                   @ 4-byte Spill
	ldr	r0, [sp, #56]                   @ 4-byte Reload
	adc	r0, r0, #0
	str	r0, [sp, #56]                   @ 4-byte Spill
	ldr	r0, [sp, #1720]
	str	r0, [sp, #52]                   @ 4-byte Spill
	ldr	r0, [sp, #1716]
	str	r0, [sp, #48]                   @ 4-byte Spill
	ldr	r0, [sp, #1712]
	str	r0, [sp, #44]                   @ 4-byte Spill
	ldr	r0, [sp, #1708]
	str	r0, [sp, #40]                   @ 4-byte Spill
	ldr	r7, [sp, #1656]
	ldr	r0, [sp, #1660]
	adds	r7, r6, r7
	ldr	r6, [sp, #120]                  @ 4-byte Reload
	ldm	r10, {r8, r9, r10}
	adcs	r0, r6, r0
	ldr	r11, [sp, #1692]
	ldr	r5, [sp, #1688]
	ldr	r4, [sp, #1684]
	ldr	lr, [sp, #1680]
	ldr	r12, [sp, #1676]
	ldr	r3, [sp, #1672]
	ldr	r1, [sp, #1664]
	ldr	r2, [sp, #1668]
	str	r0, [sp, #124]                  @ 4-byte Spill
	ldr	r0, [sp, #116]                  @ 4-byte Reload
	str	r7, [sp, #36]                   @ 4-byte Spill
	adcs	r0, r0, r1
	str	r0, [sp, #120]                  @ 4-byte Spill
	ldr	r0, [sp, #112]                  @ 4-byte Reload
	ldr	r1, [sp, #40]                   @ 4-byte Reload
	adcs	r0, r0, r2
	str	r0, [sp, #116]                  @ 4-byte Spill
	ldr	r0, [sp, #104]                  @ 4-byte Reload
	adcs	r0, r0, r3
	str	r0, [sp, #112]                  @ 4-byte Spill
	ldr	r0, [sp, #100]                  @ 4-byte Reload
	adcs	r0, r0, r12
	str	r0, [sp, #104]                  @ 4-byte Spill
	ldr	r0, [sp, #96]                   @ 4-byte Reload
	adcs	r0, r0, lr
	str	r0, [sp, #100]                  @ 4-byte Spill
	ldr	r0, [sp, #92]                   @ 4-byte Reload
	adcs	r0, r0, r4
	str	r0, [sp, #96]                   @ 4-byte Spill
	ldr	r0, [sp, #88]                   @ 4-byte Reload
	adcs	r0, r0, r5
	str	r0, [sp, #92]                   @ 4-byte Spill
	ldr	r0, [sp, #84]                   @ 4-byte Reload
	adcs	r0, r0, r11
	str	r0, [sp, #88]                   @ 4-byte Spill
	ldr	r0, [sp, #80]                   @ 4-byte Reload
	adcs	r0, r0, r8
	str	r0, [sp, #84]                   @ 4-byte Spill
	ldr	r0, [sp, #76]                   @ 4-byte Reload
	adcs	r0, r0, r9
	str	r0, [sp, #80]                   @ 4-byte Spill
	ldr	r0, [sp, #72]                   @ 4-byte Reload
	adcs	r0, r0, r10
	str	r0, [sp, #76]                   @ 4-byte Spill
	ldr	r0, [sp, #68]                   @ 4-byte Reload
	adcs	r0, r0, r1
	str	r0, [sp, #72]                   @ 4-byte Spill
	ldr	r0, [sp, #64]                   @ 4-byte Reload
	ldr	r1, [sp, #44]                   @ 4-byte Reload
	adcs	r0, r0, r1
	str	r0, [sp, #68]                   @ 4-byte Spill
	ldr	r0, [sp, #60]                   @ 4-byte Reload
	ldr	r1, [sp, #48]                   @ 4-byte Reload
	adcs	r0, r0, r1
	str	r0, [sp, #64]                   @ 4-byte Spill
	ldr	r0, [sp, #56]                   @ 4-byte Reload
	ldr	r1, [sp, #52]                   @ 4-byte Reload
	adcs	r0, r0, r1
	str	r0, [sp, #60]                   @ 4-byte Spill
	mov	r0, #0
	ldr	r1, [sp, #140]                  @ 4-byte Reload
	adc	r0, r0, #0
	str	r0, [sp, #56]                   @ 4-byte Spill
	ldr	r0, [sp, #128]                  @ 4-byte Reload
	mul	r2, r0, r7
	add	r0, sp, #1584
	bl	mulPv512x32
	ldr	r0, [sp, #132]                  @ 4-byte Reload
	add	r11, sp, #1584
	add	lr, sp, #1024
	ldr	r1, [sp, #136]                  @ 4-byte Reload
	ldr	r2, [r0, #24]
	ldr	r0, [sp, #1648]
	str	r0, [sp, #52]                   @ 4-byte Spill
	ldr	r0, [sp, #1644]
	str	r0, [sp, #48]                   @ 4-byte Spill
	ldr	r0, [sp, #1640]
	str	r0, [sp, #44]                   @ 4-byte Spill
	ldr	r0, [sp, #1636]
	str	r0, [sp, #40]                   @ 4-byte Spill
	ldr	r0, [sp, #1632]
	str	r0, [sp, #32]                   @ 4-byte Spill
	ldr	r0, [sp, #1628]
	str	r0, [sp, #28]                   @ 4-byte Spill
	ldr	r0, [sp, #1624]
	str	r0, [sp, #24]                   @ 4-byte Spill
	ldr	r0, [sp, #1620]
	str	r0, [sp, #20]                   @ 4-byte Spill
	ldr	r0, [sp, #1616]
	str	r0, [sp, #16]                   @ 4-byte Spill
	add	r0, lr, #488
	ldm	r11, {r8, r9, r10, r11}
	ldr	r4, [sp, #1612]
	ldr	r5, [sp, #1608]
	ldr	r6, [sp, #1604]
	ldr	r7, [sp, #1600]
	bl	mulPv512x32
	ldr	r0, [sp, #36]                   @ 4-byte Reload
	ldr	r1, [sp, #16]                   @ 4-byte Reload
	adds	r0, r0, r8
	ldr	r0, [sp, #124]                  @ 4-byte Reload
	adcs	r0, r0, r9
	str	r0, [sp, #124]                  @ 4-byte Spill
	ldr	r0, [sp, #120]                  @ 4-byte Reload
	adcs	r0, r0, r10
	str	r0, [sp, #120]                  @ 4-byte Spill
	ldr	r0, [sp, #116]                  @ 4-byte Reload
	add	r10, sp, #1552
	adcs	r0, r0, r11
	str	r0, [sp, #116]                  @ 4-byte Spill
	ldr	r0, [sp, #112]                  @ 4-byte Reload
	adcs	r0, r0, r7
	str	r0, [sp, #112]                  @ 4-byte Spill
	ldr	r0, [sp, #104]                  @ 4-byte Reload
	adcs	r0, r0, r6
	str	r0, [sp, #104]                  @ 4-byte Spill
	ldr	r0, [sp, #100]                  @ 4-byte Reload
	ldr	r6, [sp, #124]                  @ 4-byte Reload
	adcs	r0, r0, r5
	str	r0, [sp, #100]                  @ 4-byte Spill
	ldr	r0, [sp, #96]                   @ 4-byte Reload
	adcs	r0, r0, r4
	str	r0, [sp, #96]                   @ 4-byte Spill
	ldr	r0, [sp, #92]                   @ 4-byte Reload
	adcs	r0, r0, r1
	str	r0, [sp, #92]                   @ 4-byte Spill
	ldr	r0, [sp, #88]                   @ 4-byte Reload
	ldr	r1, [sp, #20]                   @ 4-byte Reload
	adcs	r0, r0, r1
	str	r0, [sp, #88]                   @ 4-byte Spill
	ldr	r0, [sp, #84]                   @ 4-byte Reload
	ldr	r1, [sp, #24]                   @ 4-byte Reload
	adcs	r0, r0, r1
	str	r0, [sp, #84]                   @ 4-byte Spill
	ldr	r0, [sp, #80]                   @ 4-byte Reload
	ldr	r1, [sp, #28]                   @ 4-byte Reload
	adcs	r0, r0, r1
	str	r0, [sp, #80]                   @ 4-byte Spill
	ldr	r0, [sp, #76]                   @ 4-byte Reload
	ldr	r1, [sp, #32]                   @ 4-byte Reload
	adcs	r0, r0, r1
	str	r0, [sp, #76]                   @ 4-byte Spill
	ldr	r0, [sp, #72]                   @ 4-byte Reload
	ldr	r1, [sp, #40]                   @ 4-byte Reload
	adcs	r0, r0, r1
	str	r0, [sp, #72]                   @ 4-byte Spill
	ldr	r0, [sp, #68]                   @ 4-byte Reload
	ldr	r1, [sp, #44]                   @ 4-byte Reload
	adcs	r0, r0, r1
	str	r0, [sp, #68]                   @ 4-byte Spill
	ldr	r0, [sp, #64]                   @ 4-byte Reload
	ldr	r1, [sp, #48]                   @ 4-byte Reload
	adcs	r0, r0, r1
	str	r0, [sp, #64]                   @ 4-byte Spill
	ldr	r0, [sp, #60]                   @ 4-byte Reload
	ldr	r1, [sp, #52]                   @ 4-byte Reload
	adcs	r0, r0, r1
	str	r0, [sp, #60]                   @ 4-byte Spill
	ldr	r0, [sp, #56]                   @ 4-byte Reload
	adc	r0, r0, #0
	str	r0, [sp, #56]                   @ 4-byte Spill
	ldr	r0, [sp, #1576]
	str	r0, [sp, #52]                   @ 4-byte Spill
	ldr	r0, [sp, #1572]
	str	r0, [sp, #48]                   @ 4-byte Spill
	ldr	r0, [sp, #1568]
	str	r0, [sp, #44]                   @ 4-byte Spill
	ldr	r0, [sp, #1564]
	str	r0, [sp, #40]                   @ 4-byte Spill
	ldr	r7, [sp, #1512]
	ldr	r0, [sp, #1516]
	adds	r7, r6, r7
	ldr	r6, [sp, #120]                  @ 4-byte Reload
	ldm	r10, {r8, r9, r10}
	adcs	r0, r6, r0
	ldr	r11, [sp, #1548]
	ldr	r5, [sp, #1544]
	ldr	r4, [sp, #1540]
	ldr	lr, [sp, #1536]
	ldr	r12, [sp, #1532]
	ldr	r3, [sp, #1528]
	ldr	r1, [sp, #1520]
	ldr	r2, [sp, #1524]
	str	r0, [sp, #124]                  @ 4-byte Spill
	ldr	r0, [sp, #116]                  @ 4-byte Reload
	str	r7, [sp, #36]                   @ 4-byte Spill
	adcs	r0, r0, r1
	str	r0, [sp, #120]                  @ 4-byte Spill
	ldr	r0, [sp, #112]                  @ 4-byte Reload
	ldr	r1, [sp, #40]                   @ 4-byte Reload
	adcs	r0, r0, r2
	str	r0, [sp, #116]                  @ 4-byte Spill
	ldr	r0, [sp, #104]                  @ 4-byte Reload
	adcs	r0, r0, r3
	str	r0, [sp, #112]                  @ 4-byte Spill
	ldr	r0, [sp, #100]                  @ 4-byte Reload
	adcs	r0, r0, r12
	str	r0, [sp, #104]                  @ 4-byte Spill
	ldr	r0, [sp, #96]                   @ 4-byte Reload
	adcs	r0, r0, lr
	str	r0, [sp, #100]                  @ 4-byte Spill
	ldr	r0, [sp, #92]                   @ 4-byte Reload
	adcs	r0, r0, r4
	str	r0, [sp, #96]                   @ 4-byte Spill
	ldr	r0, [sp, #88]                   @ 4-byte Reload
	adcs	r0, r0, r5
	str	r0, [sp, #92]                   @ 4-byte Spill
	ldr	r0, [sp, #84]                   @ 4-byte Reload
	adcs	r0, r0, r11
	str	r0, [sp, #88]                   @ 4-byte Spill
	ldr	r0, [sp, #80]                   @ 4-byte Reload
	adcs	r0, r0, r8
	str	r0, [sp, #84]                   @ 4-byte Spill
	ldr	r0, [sp, #76]                   @ 4-byte Reload
	adcs	r0, r0, r9
	str	r0, [sp, #80]                   @ 4-byte Spill
	ldr	r0, [sp, #72]                   @ 4-byte Reload
	adcs	r0, r0, r10
	str	r0, [sp, #76]                   @ 4-byte Spill
	ldr	r0, [sp, #68]                   @ 4-byte Reload
	adcs	r0, r0, r1
	str	r0, [sp, #72]                   @ 4-byte Spill
	ldr	r0, [sp, #64]                   @ 4-byte Reload
	ldr	r1, [sp, #44]                   @ 4-byte Reload
	adcs	r0, r0, r1
	str	r0, [sp, #68]                   @ 4-byte Spill
	ldr	r0, [sp, #60]                   @ 4-byte Reload
	ldr	r1, [sp, #48]                   @ 4-byte Reload
	adcs	r0, r0, r1
	str	r0, [sp, #64]                   @ 4-byte Spill
	ldr	r0, [sp, #56]                   @ 4-byte Reload
	ldr	r1, [sp, #52]                   @ 4-byte Reload
	adcs	r0, r0, r1
	str	r0, [sp, #60]                   @ 4-byte Spill
	mov	r0, #0
	ldr	r1, [sp, #140]                  @ 4-byte Reload
	adc	r0, r0, #0
	str	r0, [sp, #56]                   @ 4-byte Spill
	ldr	r0, [sp, #128]                  @ 4-byte Reload
	mul	r2, r0, r7
	add	r0, sp, #1440
	bl	mulPv512x32
	ldr	r0, [sp, #132]                  @ 4-byte Reload
	add	r11, sp, #1440
	add	lr, sp, #1024
	ldr	r1, [sp, #136]                  @ 4-byte Reload
	ldr	r2, [r0, #28]
	ldr	r0, [sp, #1504]
	str	r0, [sp, #52]                   @ 4-byte Spill
	ldr	r0, [sp, #1500]
	str	r0, [sp, #48]                   @ 4-byte Spill
	ldr	r0, [sp, #1496]
	str	r0, [sp, #44]                   @ 4-byte Spill
	ldr	r0, [sp, #1492]
	str	r0, [sp, #40]                   @ 4-byte Spill
	ldr	r0, [sp, #1488]
	str	r0, [sp, #32]                   @ 4-byte Spill
	ldr	r0, [sp, #1484]
	str	r0, [sp, #28]                   @ 4-byte Spill
	ldr	r0, [sp, #1480]
	str	r0, [sp, #24]                   @ 4-byte Spill
	ldr	r0, [sp, #1476]
	str	r0, [sp, #20]                   @ 4-byte Spill
	ldr	r0, [sp, #1472]
	str	r0, [sp, #16]                   @ 4-byte Spill
	add	r0, lr, #344
	ldm	r11, {r8, r9, r10, r11}
	ldr	r4, [sp, #1468]
	ldr	r5, [sp, #1464]
	ldr	r6, [sp, #1460]
	ldr	r7, [sp, #1456]
	bl	mulPv512x32
	ldr	r0, [sp, #36]                   @ 4-byte Reload
	ldr	r1, [sp, #16]                   @ 4-byte Reload
	adds	r0, r0, r8
	ldr	r0, [sp, #124]                  @ 4-byte Reload
	adcs	r0, r0, r9
	str	r0, [sp, #124]                  @ 4-byte Spill
	ldr	r0, [sp, #120]                  @ 4-byte Reload
	adcs	r0, r0, r10
	str	r0, [sp, #120]                  @ 4-byte Spill
	ldr	r0, [sp, #116]                  @ 4-byte Reload
	add	r10, sp, #1408
	adcs	r0, r0, r11
	str	r0, [sp, #116]                  @ 4-byte Spill
	ldr	r0, [sp, #112]                  @ 4-byte Reload
	adcs	r0, r0, r7
	str	r0, [sp, #112]                  @ 4-byte Spill
	ldr	r0, [sp, #104]                  @ 4-byte Reload
	adcs	r0, r0, r6
	str	r0, [sp, #104]                  @ 4-byte Spill
	ldr	r0, [sp, #100]                  @ 4-byte Reload
	ldr	r6, [sp, #124]                  @ 4-byte Reload
	adcs	r0, r0, r5
	str	r0, [sp, #100]                  @ 4-byte Spill
	ldr	r0, [sp, #96]                   @ 4-byte Reload
	adcs	r0, r0, r4
	str	r0, [sp, #96]                   @ 4-byte Spill
	ldr	r0, [sp, #92]                   @ 4-byte Reload
	adcs	r0, r0, r1
	str	r0, [sp, #92]                   @ 4-byte Spill
	ldr	r0, [sp, #88]                   @ 4-byte Reload
	ldr	r1, [sp, #20]                   @ 4-byte Reload
	adcs	r0, r0, r1
	str	r0, [sp, #88]                   @ 4-byte Spill
	ldr	r0, [sp, #84]                   @ 4-byte Reload
	ldr	r1, [sp, #24]                   @ 4-byte Reload
	adcs	r0, r0, r1
	str	r0, [sp, #84]                   @ 4-byte Spill
	ldr	r0, [sp, #80]                   @ 4-byte Reload
	ldr	r1, [sp, #28]                   @ 4-byte Reload
	adcs	r0, r0, r1
	str	r0, [sp, #80]                   @ 4-byte Spill
	ldr	r0, [sp, #76]                   @ 4-byte Reload
	ldr	r1, [sp, #32]                   @ 4-byte Reload
	adcs	r0, r0, r1
	str	r0, [sp, #76]                   @ 4-byte Spill
	ldr	r0, [sp, #72]                   @ 4-byte Reload
	ldr	r1, [sp, #40]                   @ 4-byte Reload
	adcs	r0, r0, r1
	str	r0, [sp, #72]                   @ 4-byte Spill
	ldr	r0, [sp, #68]                   @ 4-byte Reload
	ldr	r1, [sp, #44]                   @ 4-byte Reload
	adcs	r0, r0, r1
	str	r0, [sp, #68]                   @ 4-byte Spill
	ldr	r0, [sp, #64]                   @ 4-byte Reload
	ldr	r1, [sp, #48]                   @ 4-byte Reload
	adcs	r0, r0, r1
	str	r0, [sp, #64]                   @ 4-byte Spill
	ldr	r0, [sp, #60]                   @ 4-byte Reload
	ldr	r1, [sp, #52]                   @ 4-byte Reload
	adcs	r0, r0, r1
	str	r0, [sp, #60]                   @ 4-byte Spill
	ldr	r0, [sp, #56]                   @ 4-byte Reload
	adc	r0, r0, #0
	str	r0, [sp, #56]                   @ 4-byte Spill
	ldr	r0, [sp, #1432]
	str	r0, [sp, #52]                   @ 4-byte Spill
	ldr	r0, [sp, #1428]
	str	r0, [sp, #48]                   @ 4-byte Spill
	ldr	r0, [sp, #1424]
	str	r0, [sp, #44]                   @ 4-byte Spill
	ldr	r0, [sp, #1420]
	str	r0, [sp, #40]                   @ 4-byte Spill
	ldr	r7, [sp, #1368]
	ldr	r0, [sp, #1372]
	adds	r7, r6, r7
	ldr	r6, [sp, #120]                  @ 4-byte Reload
	ldm	r10, {r8, r9, r10}
	adcs	r0, r6, r0
	ldr	r11, [sp, #1404]
	ldr	r5, [sp, #1400]
	ldr	r4, [sp, #1396]
	ldr	lr, [sp, #1392]
	ldr	r12, [sp, #1388]
	ldr	r3, [sp, #1384]
	ldr	r1, [sp, #1376]
	ldr	r2, [sp, #1380]
	str	r0, [sp, #124]                  @ 4-byte Spill
	ldr	r0, [sp, #116]                  @ 4-byte Reload
	str	r7, [sp, #36]                   @ 4-byte Spill
	adcs	r0, r0, r1
	str	r0, [sp, #120]                  @ 4-byte Spill
	ldr	r0, [sp, #112]                  @ 4-byte Reload
	ldr	r1, [sp, #40]                   @ 4-byte Reload
	adcs	r0, r0, r2
	str	r0, [sp, #116]                  @ 4-byte Spill
	ldr	r0, [sp, #104]                  @ 4-byte Reload
	adcs	r0, r0, r3
	str	r0, [sp, #112]                  @ 4-byte Spill
	ldr	r0, [sp, #100]                  @ 4-byte Reload
	adcs	r0, r0, r12
	str	r0, [sp, #104]                  @ 4-byte Spill
	ldr	r0, [sp, #96]                   @ 4-byte Reload
	adcs	r0, r0, lr
	str	r0, [sp, #100]                  @ 4-byte Spill
	ldr	r0, [sp, #92]                   @ 4-byte Reload
	adcs	r0, r0, r4
	str	r0, [sp, #96]                   @ 4-byte Spill
	ldr	r0, [sp, #88]                   @ 4-byte Reload
	adcs	r0, r0, r5
	str	r0, [sp, #92]                   @ 4-byte Spill
	ldr	r0, [sp, #84]                   @ 4-byte Reload
	adcs	r0, r0, r11
	str	r0, [sp, #88]                   @ 4-byte Spill
	ldr	r0, [sp, #80]                   @ 4-byte Reload
	adcs	r0, r0, r8
	str	r0, [sp, #84]                   @ 4-byte Spill
	ldr	r0, [sp, #76]                   @ 4-byte Reload
	adcs	r0, r0, r9
	str	r0, [sp, #80]                   @ 4-byte Spill
	ldr	r0, [sp, #72]                   @ 4-byte Reload
	adcs	r0, r0, r10
	str	r0, [sp, #76]                   @ 4-byte Spill
	ldr	r0, [sp, #68]                   @ 4-byte Reload
	adcs	r0, r0, r1
	str	r0, [sp, #72]                   @ 4-byte Spill
	ldr	r0, [sp, #64]                   @ 4-byte Reload
	ldr	r1, [sp, #44]                   @ 4-byte Reload
	adcs	r0, r0, r1
	str	r0, [sp, #68]                   @ 4-byte Spill
	ldr	r0, [sp, #60]                   @ 4-byte Reload
	ldr	r1, [sp, #48]                   @ 4-byte Reload
	adcs	r0, r0, r1
	str	r0, [sp, #64]                   @ 4-byte Spill
	ldr	r0, [sp, #56]                   @ 4-byte Reload
	ldr	r1, [sp, #52]                   @ 4-byte Reload
	adcs	r0, r0, r1
	str	r0, [sp, #60]                   @ 4-byte Spill
	mov	r0, #0
	ldr	r1, [sp, #140]                  @ 4-byte Reload
	adc	r0, r0, #0
	str	r0, [sp, #56]                   @ 4-byte Spill
	ldr	r0, [sp, #128]                  @ 4-byte Reload
	mul	r2, r0, r7
	add	r0, sp, #1296
	bl	mulPv512x32
	ldr	r0, [sp, #132]                  @ 4-byte Reload
	add	r11, sp, #1296
	add	lr, sp, #1024
	ldr	r1, [sp, #136]                  @ 4-byte Reload
	ldr	r2, [r0, #32]
	ldr	r0, [sp, #1360]
	str	r0, [sp, #52]                   @ 4-byte Spill
	ldr	r0, [sp, #1356]
	str	r0, [sp, #48]                   @ 4-byte Spill
	ldr	r0, [sp, #1352]
	str	r0, [sp, #44]                   @ 4-byte Spill
	ldr	r0, [sp, #1348]
	str	r0, [sp, #40]                   @ 4-byte Spill
	ldr	r0, [sp, #1344]
	str	r0, [sp, #32]                   @ 4-byte Spill
	ldr	r0, [sp, #1340]
	str	r0, [sp, #28]                   @ 4-byte Spill
	ldr	r0, [sp, #1336]
	str	r0, [sp, #24]                   @ 4-byte Spill
	ldr	r0, [sp, #1332]
	str	r0, [sp, #20]                   @ 4-byte Spill
	ldr	r0, [sp, #1328]
	str	r0, [sp, #16]                   @ 4-byte Spill
	add	r0, lr, #200
	ldm	r11, {r8, r9, r10, r11}
	ldr	r4, [sp, #1324]
	ldr	r5, [sp, #1320]
	ldr	r6, [sp, #1316]
	ldr	r7, [sp, #1312]
	bl	mulPv512x32
	ldr	r0, [sp, #36]                   @ 4-byte Reload
	ldr	r1, [sp, #16]                   @ 4-byte Reload
	adds	r0, r0, r8
	ldr	r0, [sp, #124]                  @ 4-byte Reload
	adcs	r0, r0, r9
	str	r0, [sp, #124]                  @ 4-byte Spill
	ldr	r0, [sp, #120]                  @ 4-byte Reload
	adcs	r0, r0, r10
	str	r0, [sp, #120]                  @ 4-byte Spill
	ldr	r0, [sp, #116]                  @ 4-byte Reload
	add	r10, sp, #1264
	adcs	r0, r0, r11
	str	r0, [sp, #116]                  @ 4-byte Spill
	ldr	r0, [sp, #112]                  @ 4-byte Reload
	adcs	r0, r0, r7
	str	r0, [sp, #112]                  @ 4-byte Spill
	ldr	r0, [sp, #104]                  @ 4-byte Reload
	adcs	r0, r0, r6
	str	r0, [sp, #104]                  @ 4-byte Spill
	ldr	r0, [sp, #100]                  @ 4-byte Reload
	ldr	r6, [sp, #124]                  @ 4-byte Reload
	adcs	r0, r0, r5
	str	r0, [sp, #100]                  @ 4-byte Spill
	ldr	r0, [sp, #96]                   @ 4-byte Reload
	adcs	r0, r0, r4
	str	r0, [sp, #96]                   @ 4-byte Spill
	ldr	r0, [sp, #92]                   @ 4-byte Reload
	adcs	r0, r0, r1
	str	r0, [sp, #92]                   @ 4-byte Spill
	ldr	r0, [sp, #88]                   @ 4-byte Reload
	ldr	r1, [sp, #20]                   @ 4-byte Reload
	adcs	r0, r0, r1
	str	r0, [sp, #88]                   @ 4-byte Spill
	ldr	r0, [sp, #84]                   @ 4-byte Reload
	ldr	r1, [sp, #24]                   @ 4-byte Reload
	adcs	r0, r0, r1
	str	r0, [sp, #84]                   @ 4-byte Spill
	ldr	r0, [sp, #80]                   @ 4-byte Reload
	ldr	r1, [sp, #28]                   @ 4-byte Reload
	adcs	r0, r0, r1
	str	r0, [sp, #80]                   @ 4-byte Spill
	ldr	r0, [sp, #76]                   @ 4-byte Reload
	ldr	r1, [sp, #32]                   @ 4-byte Reload
	adcs	r0, r0, r1
	str	r0, [sp, #76]                   @ 4-byte Spill
	ldr	r0, [sp, #72]                   @ 4-byte Reload
	ldr	r1, [sp, #40]                   @ 4-byte Reload
	adcs	r0, r0, r1
	str	r0, [sp, #72]                   @ 4-byte Spill
	ldr	r0, [sp, #68]                   @ 4-byte Reload
	ldr	r1, [sp, #44]                   @ 4-byte Reload
	adcs	r0, r0, r1
	str	r0, [sp, #68]                   @ 4-byte Spill
	ldr	r0, [sp, #64]                   @ 4-byte Reload
	ldr	r1, [sp, #48]                   @ 4-byte Reload
	adcs	r0, r0, r1
	str	r0, [sp, #64]                   @ 4-byte Spill
	ldr	r0, [sp, #60]                   @ 4-byte Reload
	ldr	r1, [sp, #52]                   @ 4-byte Reload
	adcs	r0, r0, r1
	str	r0, [sp, #60]                   @ 4-byte Spill
	ldr	r0, [sp, #56]                   @ 4-byte Reload
	adc	r0, r0, #0
	str	r0, [sp, #56]                   @ 4-byte Spill
	ldr	r0, [sp, #1288]
	str	r0, [sp, #52]                   @ 4-byte Spill
	ldr	r0, [sp, #1284]
	str	r0, [sp, #48]                   @ 4-byte Spill
	ldr	r0, [sp, #1280]
	str	r0, [sp, #44]                   @ 4-byte Spill
	ldr	r0, [sp, #1276]
	str	r0, [sp, #40]                   @ 4-byte Spill
	ldr	r7, [sp, #1224]
	ldr	r0, [sp, #1228]
	adds	r7, r6, r7
	ldr	r6, [sp, #120]                  @ 4-byte Reload
	ldm	r10, {r8, r9, r10}
	adcs	r0, r6, r0
	ldr	r11, [sp, #1260]
	ldr	r5, [sp, #1256]
	ldr	r4, [sp, #1252]
	ldr	lr, [sp, #1248]
	ldr	r12, [sp, #1244]
	ldr	r3, [sp, #1240]
	ldr	r1, [sp, #1232]
	ldr	r2, [sp, #1236]
	str	r0, [sp, #124]                  @ 4-byte Spill
	ldr	r0, [sp, #116]                  @ 4-byte Reload
	str	r7, [sp, #36]                   @ 4-byte Spill
	adcs	r0, r0, r1
	str	r0, [sp, #120]                  @ 4-byte Spill
	ldr	r0, [sp, #112]                  @ 4-byte Reload
	ldr	r1, [sp, #40]                   @ 4-byte Reload
	adcs	r0, r0, r2
	str	r0, [sp, #116]                  @ 4-byte Spill
	ldr	r0, [sp, #104]                  @ 4-byte Reload
	adcs	r0, r0, r3
	str	r0, [sp, #112]                  @ 4-byte Spill
	ldr	r0, [sp, #100]                  @ 4-byte Reload
	adcs	r0, r0, r12
	str	r0, [sp, #104]                  @ 4-byte Spill
	ldr	r0, [sp, #96]                   @ 4-byte Reload
	adcs	r0, r0, lr
	str	r0, [sp, #100]                  @ 4-byte Spill
	ldr	r0, [sp, #92]                   @ 4-byte Reload
	adcs	r0, r0, r4
	str	r0, [sp, #96]                   @ 4-byte Spill
	ldr	r0, [sp, #88]                   @ 4-byte Reload
	adcs	r0, r0, r5
	str	r0, [sp, #92]                   @ 4-byte Spill
	ldr	r0, [sp, #84]                   @ 4-byte Reload
	ldr	r5, [sp, #128]                  @ 4-byte Reload
	adcs	r0, r0, r11
	str	r0, [sp, #88]                   @ 4-byte Spill
	ldr	r0, [sp, #80]                   @ 4-byte Reload
	mul	r2, r5, r7
	adcs	r0, r0, r8
	str	r0, [sp, #84]                   @ 4-byte Spill
	ldr	r0, [sp, #76]                   @ 4-byte Reload
	adcs	r0, r0, r9
	str	r0, [sp, #80]                   @ 4-byte Spill
	ldr	r0, [sp, #72]                   @ 4-byte Reload
	adcs	r0, r0, r10
	str	r0, [sp, #76]                   @ 4-byte Spill
	ldr	r0, [sp, #68]                   @ 4-byte Reload
	adcs	r0, r0, r1
	str	r0, [sp, #72]                   @ 4-byte Spill
	ldr	r0, [sp, #64]                   @ 4-byte Reload
	ldr	r1, [sp, #44]                   @ 4-byte Reload
	adcs	r0, r0, r1
	str	r0, [sp, #68]                   @ 4-byte Spill
	ldr	r0, [sp, #60]                   @ 4-byte Reload
	ldr	r1, [sp, #48]                   @ 4-byte Reload
	adcs	r0, r0, r1
	str	r0, [sp, #64]                   @ 4-byte Spill
	ldr	r0, [sp, #56]                   @ 4-byte Reload
	ldr	r1, [sp, #52]                   @ 4-byte Reload
	adcs	r0, r0, r1
	str	r0, [sp, #60]                   @ 4-byte Spill
	mov	r0, #0
	ldr	r1, [sp, #140]                  @ 4-byte Reload
	adc	r0, r0, #0
	str	r0, [sp, #56]                   @ 4-byte Spill
	add	r0, sp, #1152
	bl	mulPv512x32
	ldr	r0, [sp, #132]                  @ 4-byte Reload
	add	r11, sp, #1152
	add	lr, sp, #1024
	ldr	r1, [sp, #136]                  @ 4-byte Reload
	ldr	r2, [r0, #36]
	ldr	r0, [sp, #1216]
	str	r0, [sp, #52]                   @ 4-byte Spill
	ldr	r0, [sp, #1212]
	str	r0, [sp, #48]                   @ 4-byte Spill
	ldr	r0, [sp, #1208]
	str	r0, [sp, #44]                   @ 4-byte Spill
	ldr	r0, [sp, #1204]
	str	r0, [sp, #40]                   @ 4-byte Spill
	ldr	r0, [sp, #1200]
	str	r0, [sp, #32]                   @ 4-byte Spill
	ldr	r0, [sp, #1196]
	str	r0, [sp, #28]                   @ 4-byte Spill
	ldr	r0, [sp, #1192]
	str	r0, [sp, #24]                   @ 4-byte Spill
	ldr	r0, [sp, #1188]
	str	r0, [sp, #20]                   @ 4-byte Spill
	ldr	r0, [sp, #1184]
	str	r0, [sp, #16]                   @ 4-byte Spill
	ldr	r0, [sp, #1180]
	str	r0, [sp, #12]                   @ 4-byte Spill
	add	r0, lr, #56
	ldm	r11, {r8, r9, r10, r11}
	ldr	r4, [sp, #1176]
	ldr	r6, [sp, #1172]
	ldr	r7, [sp, #1168]
	bl	mulPv512x32
	ldr	r0, [sp, #36]                   @ 4-byte Reload
	ldr	r1, [sp, #120]                  @ 4-byte Reload
	adds	r0, r0, r8
	ldr	r2, [sp, #12]                   @ 4-byte Reload
	ldr	r0, [sp, #124]                  @ 4-byte Reload
	adcs	r0, r0, r9
	adcs	r1, r1, r10
	str	r1, [sp, #120]                  @ 4-byte Spill
	ldr	r1, [sp, #116]                  @ 4-byte Reload
	add	r10, sp, #1120
	adcs	r1, r1, r11
	str	r1, [sp, #116]                  @ 4-byte Spill
	ldr	r1, [sp, #112]                  @ 4-byte Reload
	adcs	r1, r1, r7
	str	r1, [sp, #112]                  @ 4-byte Spill
	ldr	r1, [sp, #104]                  @ 4-byte Reload
	adcs	r1, r1, r6
	str	r1, [sp, #104]                  @ 4-byte Spill
	ldr	r1, [sp, #100]                  @ 4-byte Reload
	adcs	r1, r1, r4
	str	r1, [sp, #100]                  @ 4-byte Spill
	ldr	r1, [sp, #96]                   @ 4-byte Reload
	adcs	r1, r1, r2
	str	r1, [sp, #96]                   @ 4-byte Spill
	ldr	r1, [sp, #92]                   @ 4-byte Reload
	ldr	r2, [sp, #16]                   @ 4-byte Reload
	adcs	r1, r1, r2
	str	r1, [sp, #92]                   @ 4-byte Spill
	ldr	r1, [sp, #88]                   @ 4-byte Reload
	ldr	r2, [sp, #20]                   @ 4-byte Reload
	adcs	r1, r1, r2
	str	r1, [sp, #88]                   @ 4-byte Spill
	ldr	r1, [sp, #84]                   @ 4-byte Reload
	ldr	r2, [sp, #24]                   @ 4-byte Reload
	adcs	r1, r1, r2
	str	r1, [sp, #84]                   @ 4-byte Spill
	ldr	r1, [sp, #80]                   @ 4-byte Reload
	ldr	r2, [sp, #28]                   @ 4-byte Reload
	adcs	r1, r1, r2
	str	r1, [sp, #80]                   @ 4-byte Spill
	ldr	r1, [sp, #76]                   @ 4-byte Reload
	ldr	r2, [sp, #32]                   @ 4-byte Reload
	adcs	r1, r1, r2
	str	r1, [sp, #76]                   @ 4-byte Spill
	ldr	r1, [sp, #72]                   @ 4-byte Reload
	ldr	r2, [sp, #40]                   @ 4-byte Reload
	adcs	r1, r1, r2
	str	r1, [sp, #72]                   @ 4-byte Spill
	ldr	r1, [sp, #68]                   @ 4-byte Reload
	ldr	r2, [sp, #44]                   @ 4-byte Reload
	adcs	r1, r1, r2
	str	r1, [sp, #68]                   @ 4-byte Spill
	ldr	r1, [sp, #64]                   @ 4-byte Reload
	ldr	r2, [sp, #48]                   @ 4-byte Reload
	adcs	r1, r1, r2
	str	r1, [sp, #64]                   @ 4-byte Spill
	ldr	r1, [sp, #60]                   @ 4-byte Reload
	ldr	r2, [sp, #52]                   @ 4-byte Reload
	adcs	r1, r1, r2
	str	r1, [sp, #60]                   @ 4-byte Spill
	ldr	r1, [sp, #56]                   @ 4-byte Reload
	adc	r1, r1, #0
	str	r1, [sp, #56]                   @ 4-byte Spill
	ldr	r4, [sp, #1080]
	ldr	r6, [sp, #1084]
	adds	r0, r0, r4
	ldr	r7, [sp, #1088]
	ldr	r11, [sp, #1092]
	mul	r1, r5, r0
	str	r0, [sp, #124]                  @ 4-byte Spill
	ldr	r5, [sp, #120]                  @ 4-byte Reload
	adcs	r6, r5, r6
	ldr	r5, [sp, #116]                  @ 4-byte Reload
	str	r1, [sp, #48]                   @ 4-byte Spill
	ldr	r0, [sp, #1144]
	str	r0, [sp, #52]                   @ 4-byte Spill
	ldr	r0, [sp, #1140]
	str	r0, [sp, #44]                   @ 4-byte Spill
	ldr	r0, [sp, #1136]
	str	r0, [sp, #40]                   @ 4-byte Spill
	ldm	r10, {r4, r8, r9, r10}
	ldr	lr, [sp, #1116]
	ldr	r12, [sp, #1112]
	ldr	r3, [sp, #1108]
	ldr	r2, [sp, #1104]
	ldr	r1, [sp, #1100]
	ldr	r0, [sp, #1096]
	str	r6, [sp, #120]                  @ 4-byte Spill
	adcs	r6, r5, r7
	ldr	r5, [sp, #112]                  @ 4-byte Reload
	str	r6, [sp, #116]                  @ 4-byte Spill
	adcs	r6, r5, r11
	ldr	r5, [sp, #104]                  @ 4-byte Reload
	str	r6, [sp, #112]                  @ 4-byte Spill
	adcs	r0, r5, r0
	str	r0, [sp, #104]                  @ 4-byte Spill
	ldr	r0, [sp, #100]                  @ 4-byte Reload
	adcs	r0, r0, r1
	str	r0, [sp, #100]                  @ 4-byte Spill
	ldr	r0, [sp, #96]                   @ 4-byte Reload
	ldr	r1, [sp, #40]                   @ 4-byte Reload
	adcs	r0, r0, r2
	str	r0, [sp, #96]                   @ 4-byte Spill
	ldr	r0, [sp, #92]                   @ 4-byte Reload
	ldr	r2, [sp, #48]                   @ 4-byte Reload
	adcs	r0, r0, r3
	str	r0, [sp, #92]                   @ 4-byte Spill
	ldr	r0, [sp, #88]                   @ 4-byte Reload
	adcs	r0, r0, r12
	str	r0, [sp, #88]                   @ 4-byte Spill
	ldr	r0, [sp, #84]                   @ 4-byte Reload
	adcs	r0, r0, lr
	str	r0, [sp, #84]                   @ 4-byte Spill
	ldr	r0, [sp, #80]                   @ 4-byte Reload
	adcs	r0, r0, r4
	str	r0, [sp, #80]                   @ 4-byte Spill
	ldr	r0, [sp, #76]                   @ 4-byte Reload
	adcs	r0, r0, r8
	str	r0, [sp, #76]                   @ 4-byte Spill
	ldr	r0, [sp, #72]                   @ 4-byte Reload
	adcs	r0, r0, r9
	str	r0, [sp, #72]                   @ 4-byte Spill
	ldr	r0, [sp, #68]                   @ 4-byte Reload
	adcs	r0, r0, r10
	str	r0, [sp, #68]                   @ 4-byte Spill
	ldr	r0, [sp, #64]                   @ 4-byte Reload
	adcs	r0, r0, r1
	str	r0, [sp, #64]                   @ 4-byte Spill
	ldr	r0, [sp, #60]                   @ 4-byte Reload
	ldr	r1, [sp, #44]                   @ 4-byte Reload
	adcs	r0, r0, r1
	str	r0, [sp, #60]                   @ 4-byte Spill
	ldr	r0, [sp, #56]                   @ 4-byte Reload
	ldr	r1, [sp, #52]                   @ 4-byte Reload
	adcs	r0, r0, r1
	str	r0, [sp, #56]                   @ 4-byte Spill
	mov	r0, #0
	ldr	r1, [sp, #140]                  @ 4-byte Reload
	adc	r0, r0, #0
	str	r0, [sp, #52]                   @ 4-byte Spill
	add	r0, sp, #1008
	bl	mulPv512x32
	ldr	r0, [sp, #132]                  @ 4-byte Reload
	add	r11, sp, #1008
	ldr	r1, [sp, #136]                  @ 4-byte Reload
	ldr	r2, [r0, #40]
	ldr	r0, [sp, #1072]
	str	r0, [sp, #48]                   @ 4-byte Spill
	ldr	r0, [sp, #1068]
	str	r0, [sp, #44]                   @ 4-byte Spill
	ldr	r0, [sp, #1064]
	str	r0, [sp, #32]                   @ 4-byte Spill
	ldr	r0, [sp, #1060]
	str	r0, [sp, #36]                   @ 4-byte Spill
	ldr	r0, [sp, #1056]
	str	r0, [sp, #40]                   @ 4-byte Spill
	ldr	r0, [sp, #1052]
	str	r0, [sp, #28]                   @ 4-byte Spill
	ldr	r0, [sp, #1048]
	str	r0, [sp, #24]                   @ 4-byte Spill
	ldr	r0, [sp, #1044]
	str	r0, [sp, #20]                   @ 4-byte Spill
	ldr	r0, [sp, #1040]
	str	r0, [sp, #16]                   @ 4-byte Spill
	add	r0, sp, #936
	ldm	r11, {r8, r9, r10, r11}
	ldr	r4, [sp, #1036]
	ldr	r5, [sp, #1032]
	ldr	r6, [sp, #1028]
	ldr	r7, [sp, #1024]
	bl	mulPv512x32
	ldr	r0, [sp, #124]                  @ 4-byte Reload
	add	lr, sp, #952
	ldr	r1, [sp, #116]                  @ 4-byte Reload
	adds	r0, r0, r8
	ldr	r2, [sp, #16]                   @ 4-byte Reload
	ldr	r0, [sp, #120]                  @ 4-byte Reload
	adcs	r0, r0, r9
	adcs	r1, r1, r10
	str	r1, [sp, #120]                  @ 4-byte Spill
	ldr	r1, [sp, #112]                  @ 4-byte Reload
	add	r10, sp, #976
	adcs	r1, r1, r11
	str	r1, [sp, #116]                  @ 4-byte Spill
	ldr	r1, [sp, #104]                  @ 4-byte Reload
	ldr	r11, [sp, #120]                 @ 4-byte Reload
	adcs	r1, r1, r7
	str	r1, [sp, #112]                  @ 4-byte Spill
	ldr	r1, [sp, #100]                  @ 4-byte Reload
	add	r7, sp, #936
	adcs	r1, r1, r6
	str	r1, [sp, #104]                  @ 4-byte Spill
	ldr	r1, [sp, #96]                   @ 4-byte Reload
	adcs	r1, r1, r5
	str	r1, [sp, #100]                  @ 4-byte Spill
	ldr	r1, [sp, #92]                   @ 4-byte Reload
	adcs	r1, r1, r4
	str	r1, [sp, #96]                   @ 4-byte Spill
	ldr	r1, [sp, #88]                   @ 4-byte Reload
	adcs	r1, r1, r2
	str	r1, [sp, #92]                   @ 4-byte Spill
	ldr	r1, [sp, #84]                   @ 4-byte Reload
	ldr	r2, [sp, #20]                   @ 4-byte Reload
	adcs	r1, r1, r2
	str	r1, [sp, #88]                   @ 4-byte Spill
	ldr	r1, [sp, #80]                   @ 4-byte Reload
	ldr	r2, [sp, #24]                   @ 4-byte Reload
	adcs	r1, r1, r2
	str	r1, [sp, #84]                   @ 4-byte Spill
	ldr	r1, [sp, #76]                   @ 4-byte Reload
	ldr	r2, [sp, #28]                   @ 4-byte Reload
	adcs	r1, r1, r2
	str	r1, [sp, #80]                   @ 4-byte Spill
	ldr	r1, [sp, #72]                   @ 4-byte Reload
	ldr	r2, [sp, #40]                   @ 4-byte Reload
	adcs	r1, r1, r2
	str	r1, [sp, #40]                   @ 4-byte Spill
	ldr	r1, [sp, #68]                   @ 4-byte Reload
	ldr	r2, [sp, #36]                   @ 4-byte Reload
	adcs	r1, r1, r2
	str	r1, [sp, #36]                   @ 4-byte Spill
	ldr	r1, [sp, #64]                   @ 4-byte Reload
	ldr	r2, [sp, #32]                   @ 4-byte Reload
	adcs	r1, r1, r2
	str	r1, [sp, #32]                   @ 4-byte Spill
	ldr	r1, [sp, #60]                   @ 4-byte Reload
	ldr	r2, [sp, #44]                   @ 4-byte Reload
	adcs	r1, r1, r2
	str	r1, [sp, #44]                   @ 4-byte Spill
	ldr	r1, [sp, #56]                   @ 4-byte Reload
	ldr	r2, [sp, #48]                   @ 4-byte Reload
	adcs	r1, r1, r2
	str	r1, [sp, #48]                   @ 4-byte Spill
	ldr	r1, [sp, #52]                   @ 4-byte Reload
	adc	r1, r1, #0
	str	r1, [sp, #28]                   @ 4-byte Spill
	ldm	r7, {r4, r6, r7}
	adds	r1, r0, r4
	ldr	r0, [sp, #128]                  @ 4-byte Reload
	ldr	r5, [sp, #948]
	adcs	r6, r11, r6
	str	r1, [sp, #124]                  @ 4-byte Spill
	mul	r2, r0, r1
	str	r2, [sp, #24]                   @ 4-byte Spill
	ldr	r0, [sp, #1000]
	str	r0, [sp, #20]                   @ 4-byte Spill
	ldr	r0, [sp, #996]
	str	r0, [sp, #16]                   @ 4-byte Spill
	ldr	r0, [sp, #992]
	str	r0, [sp, #12]                   @ 4-byte Spill
	ldm	r10, {r4, r8, r9, r10}
	ldm	lr, {r0, r1, r2, r3, r12, lr}
	str	r6, [sp, #76]                   @ 4-byte Spill
	ldr	r6, [sp, #116]                  @ 4-byte Reload
	adcs	r6, r6, r7
	str	r6, [sp, #72]                   @ 4-byte Spill
	ldr	r6, [sp, #112]                  @ 4-byte Reload
	adcs	r5, r6, r5
	str	r5, [sp, #68]                   @ 4-byte Spill
	ldr	r5, [sp, #104]                  @ 4-byte Reload
	adcs	r0, r5, r0
	str	r0, [sp, #64]                   @ 4-byte Spill
	ldr	r0, [sp, #100]                  @ 4-byte Reload
	adcs	r0, r0, r1
	str	r0, [sp, #60]                   @ 4-byte Spill
	ldr	r0, [sp, #96]                   @ 4-byte Reload
	ldr	r1, [sp, #12]                   @ 4-byte Reload
	adcs	r0, r0, r2
	str	r0, [sp, #56]                   @ 4-byte Spill
	ldr	r0, [sp, #92]                   @ 4-byte Reload
	ldr	r2, [sp, #24]                   @ 4-byte Reload
	adcs	r0, r0, r3
	str	r0, [sp, #52]                   @ 4-byte Spill
	ldr	r0, [sp, #88]                   @ 4-byte Reload
	adcs	r0, r0, r12
	str	r0, [sp, #120]                  @ 4-byte Spill
	ldr	r0, [sp, #84]                   @ 4-byte Reload
	adcs	r0, r0, lr
	str	r0, [sp, #116]                  @ 4-byte Spill
	ldr	r0, [sp, #80]                   @ 4-byte Reload
	adcs	r0, r0, r4
	str	r0, [sp, #112]                  @ 4-byte Spill
	ldr	r0, [sp, #40]                   @ 4-byte Reload
	adcs	r0, r0, r8
	str	r0, [sp, #104]                  @ 4-byte Spill
	ldr	r0, [sp, #36]                   @ 4-byte Reload
	adcs	r0, r0, r9
	str	r0, [sp, #100]                  @ 4-byte Spill
	ldr	r0, [sp, #32]                   @ 4-byte Reload
	adcs	r0, r0, r10
	str	r0, [sp, #96]                   @ 4-byte Spill
	ldr	r0, [sp, #44]                   @ 4-byte Reload
	adcs	r0, r0, r1
	str	r0, [sp, #92]                   @ 4-byte Spill
	ldr	r0, [sp, #48]                   @ 4-byte Reload
	ldr	r1, [sp, #16]                   @ 4-byte Reload
	adcs	r0, r0, r1
	str	r0, [sp, #88]                   @ 4-byte Spill
	ldr	r0, [sp, #28]                   @ 4-byte Reload
	ldr	r1, [sp, #20]                   @ 4-byte Reload
	adcs	r0, r0, r1
	str	r0, [sp, #84]                   @ 4-byte Spill
	mov	r0, #0
	ldr	r1, [sp, #140]                  @ 4-byte Reload
	adc	r0, r0, #0
	str	r0, [sp, #80]                   @ 4-byte Spill
	add	r0, sp, #864
	bl	mulPv512x32
	ldr	r1, [sp, #132]                  @ 4-byte Reload
	add	r11, sp, #864
	add	r0, sp, #792
	ldr	r2, [r1, #44]
	ldr	r1, [sp, #928]
	str	r1, [sp, #48]                   @ 4-byte Spill
	ldr	r1, [sp, #924]
	str	r1, [sp, #44]                   @ 4-byte Spill
	ldr	r1, [sp, #920]
	str	r1, [sp, #40]                   @ 4-byte Spill
	ldr	r1, [sp, #916]
	str	r1, [sp, #36]                   @ 4-byte Spill
	ldr	r1, [sp, #912]
	str	r1, [sp, #32]                   @ 4-byte Spill
	ldr	r1, [sp, #908]
	str	r1, [sp, #28]                   @ 4-byte Spill
	ldr	r1, [sp, #904]
	str	r1, [sp, #24]                   @ 4-byte Spill
	ldr	r1, [sp, #900]
	str	r1, [sp, #20]                   @ 4-byte Spill
	ldr	r1, [sp, #896]
	str	r1, [sp, #16]                   @ 4-byte Spill
	ldr	r1, [sp, #136]                  @ 4-byte Reload
	ldm	r11, {r9, r10, r11}
	ldr	r6, [sp, #892]
	ldr	r7, [sp, #888]
	ldr	r4, [sp, #884]
	ldr	r8, [sp, #880]
	ldr	r5, [sp, #876]
	bl	mulPv512x32
	ldr	r0, [sp, #124]                  @ 4-byte Reload
	add	lr, sp, #796
	ldr	r1, [sp, #16]                   @ 4-byte Reload
	adds	r0, r0, r9
	ldr	r0, [sp, #76]                   @ 4-byte Reload
	adcs	r0, r0, r10
	str	r0, [sp, #124]                  @ 4-byte Spill
	ldr	r0, [sp, #72]                   @ 4-byte Reload
	add	r10, sp, #820
	adcs	r0, r0, r11
	str	r0, [sp, #76]                   @ 4-byte Spill
	ldr	r0, [sp, #68]                   @ 4-byte Reload
	ldr	r11, [sp, #124]                 @ 4-byte Reload
	adcs	r0, r0, r5
	str	r0, [sp, #72]                   @ 4-byte Spill
	ldr	r0, [sp, #64]                   @ 4-byte Reload
	adcs	r0, r0, r8
	str	r0, [sp, #68]                   @ 4-byte Spill
	ldr	r0, [sp, #60]                   @ 4-byte Reload
	adcs	r0, r0, r4
	str	r0, [sp, #64]                   @ 4-byte Spill
	ldr	r0, [sp, #56]                   @ 4-byte Reload
	adcs	r0, r0, r7
	str	r0, [sp, #60]                   @ 4-byte Spill
	ldr	r0, [sp, #52]                   @ 4-byte Reload
	ldr	r7, [sp, #76]                   @ 4-byte Reload
	adcs	r0, r0, r6
	str	r0, [sp, #56]                   @ 4-byte Spill
	ldr	r0, [sp, #120]                  @ 4-byte Reload
	adcs	r0, r0, r1
	str	r0, [sp, #120]                  @ 4-byte Spill
	ldr	r0, [sp, #116]                  @ 4-byte Reload
	ldr	r1, [sp, #20]                   @ 4-byte Reload
	adcs	r0, r0, r1
	str	r0, [sp, #116]                  @ 4-byte Spill
	ldr	r0, [sp, #112]                  @ 4-byte Reload
	ldr	r1, [sp, #24]                   @ 4-byte Reload
	adcs	r0, r0, r1
	str	r0, [sp, #112]                  @ 4-byte Spill
	ldr	r0, [sp, #104]                  @ 4-byte Reload
	ldr	r1, [sp, #28]                   @ 4-byte Reload
	adcs	r0, r0, r1
	str	r0, [sp, #104]                  @ 4-byte Spill
	ldr	r0, [sp, #100]                  @ 4-byte Reload
	ldr	r1, [sp, #32]                   @ 4-byte Reload
	adcs	r0, r0, r1
	str	r0, [sp, #100]                  @ 4-byte Spill
	ldr	r0, [sp, #96]                   @ 4-byte Reload
	ldr	r1, [sp, #36]                   @ 4-byte Reload
	adcs	r0, r0, r1
	str	r0, [sp, #96]                   @ 4-byte Spill
	ldr	r0, [sp, #92]                   @ 4-byte Reload
	ldr	r1, [sp, #40]                   @ 4-byte Reload
	adcs	r0, r0, r1
	str	r0, [sp, #92]                   @ 4-byte Spill
	ldr	r0, [sp, #88]                   @ 4-byte Reload
	ldr	r1, [sp, #44]                   @ 4-byte Reload
	adcs	r0, r0, r1
	str	r0, [sp, #88]                   @ 4-byte Spill
	ldr	r0, [sp, #84]                   @ 4-byte Reload
	ldr	r1, [sp, #48]                   @ 4-byte Reload
	adcs	r0, r0, r1
	str	r0, [sp, #84]                   @ 4-byte Spill
	ldr	r0, [sp, #80]                   @ 4-byte Reload
	adc	r0, r0, #0
	str	r0, [sp, #52]                   @ 4-byte Spill
	ldr	r0, [sp, #856]
	str	r0, [sp, #48]                   @ 4-byte Spill
	ldr	r0, [sp, #852]
	str	r0, [sp, #44]                   @ 4-byte Spill
	ldr	r0, [sp, #848]
	str	r0, [sp, #40]                   @ 4-byte Spill
	ldr	r0, [sp, #844]
	str	r0, [sp, #36]                   @ 4-byte Spill
	ldr	r0, [sp, #840]
	str	r0, [sp, #28]                   @ 4-byte Spill
	ldr	r6, [sp, #792]
	ldm	lr, {r0, r1, r2, r3, r12, lr}
	adds	r6, r11, r6
	adcs	r0, r7, r0
	ldm	r10, {r4, r5, r8, r9, r10}
	str	r0, [sp, #80]                   @ 4-byte Spill
	ldr	r0, [sp, #72]                   @ 4-byte Reload
	str	r6, [sp, #32]                   @ 4-byte Spill
	adcs	r0, r0, r1
	str	r0, [sp, #76]                   @ 4-byte Spill
	ldr	r0, [sp, #68]                   @ 4-byte Reload
	ldr	r1, [sp, #28]                   @ 4-byte Reload
	adcs	r0, r0, r2
	str	r0, [sp, #72]                   @ 4-byte Spill
	ldr	r0, [sp, #64]                   @ 4-byte Reload
	adcs	r0, r0, r3
	str	r0, [sp, #68]                   @ 4-byte Spill
	ldr	r0, [sp, #60]                   @ 4-byte Reload
	adcs	r0, r0, r12
	str	r0, [sp, #64]                   @ 4-byte Spill
	ldr	r0, [sp, #56]                   @ 4-byte Reload
	adcs	r0, r0, lr
	str	r0, [sp, #60]                   @ 4-byte Spill
	ldr	r0, [sp, #120]                  @ 4-byte Reload
	adcs	r0, r0, r4
	str	r0, [sp, #56]                   @ 4-byte Spill
	ldr	r0, [sp, #116]                  @ 4-byte Reload
	adcs	r0, r0, r5
	str	r0, [sp, #124]                  @ 4-byte Spill
	ldr	r0, [sp, #112]                  @ 4-byte Reload
	adcs	r0, r0, r8
	str	r0, [sp, #120]                  @ 4-byte Spill
	ldr	r0, [sp, #104]                  @ 4-byte Reload
	adcs	r0, r0, r9
	str	r0, [sp, #116]                  @ 4-byte Spill
	ldr	r0, [sp, #100]                  @ 4-byte Reload
	adcs	r0, r0, r10
	str	r0, [sp, #112]                  @ 4-byte Spill
	ldr	r0, [sp, #96]                   @ 4-byte Reload
	adcs	r0, r0, r1
	str	r0, [sp, #104]                  @ 4-byte Spill
	ldr	r0, [sp, #92]                   @ 4-byte Reload
	ldr	r1, [sp, #36]                   @ 4-byte Reload
	adcs	r0, r0, r1
	str	r0, [sp, #100]                  @ 4-byte Spill
	ldr	r0, [sp, #88]                   @ 4-byte Reload
	ldr	r1, [sp, #40]                   @ 4-byte Reload
	adcs	r0, r0, r1
	str	r0, [sp, #96]                   @ 4-byte Spill
	ldr	r0, [sp, #84]                   @ 4-byte Reload
	ldr	r1, [sp, #44]                   @ 4-byte Reload
	adcs	r0, r0, r1
	str	r0, [sp, #92]                   @ 4-byte Spill
	ldr	r0, [sp, #52]                   @ 4-byte Reload
	ldr	r1, [sp, #48]                   @ 4-byte Reload
	adcs	r0, r0, r1
	str	r0, [sp, #88]                   @ 4-byte Spill
	mov	r0, #0
	ldr	r1, [sp, #140]                  @ 4-byte Reload
	adc	r0, r0, #0
	str	r0, [sp, #84]                   @ 4-byte Spill
	ldr	r0, [sp, #128]                  @ 4-byte Reload
	mul	r2, r0, r6
	add	r0, sp, #720
	bl	mulPv512x32
	ldr	r1, [sp, #132]                  @ 4-byte Reload
	add	r10, sp, #728
	add	r0, sp, #648
	ldr	r2, [r1, #48]
	ldr	r1, [sp, #784]
	str	r1, [sp, #52]                   @ 4-byte Spill
	ldr	r1, [sp, #780]
	str	r1, [sp, #48]                   @ 4-byte Spill
	ldr	r1, [sp, #776]
	str	r1, [sp, #44]                   @ 4-byte Spill
	ldr	r1, [sp, #772]
	str	r1, [sp, #40]                   @ 4-byte Spill
	ldr	r1, [sp, #768]
	str	r1, [sp, #36]                   @ 4-byte Spill
	ldr	r1, [sp, #764]
	str	r1, [sp, #28]                   @ 4-byte Spill
	ldr	r1, [sp, #760]
	str	r1, [sp, #24]                   @ 4-byte Spill
	ldr	r1, [sp, #756]
	str	r1, [sp, #20]                   @ 4-byte Spill
	ldr	r1, [sp, #752]
	str	r1, [sp, #16]                   @ 4-byte Spill
	ldm	r10, {r4, r6, r8, r10}
	ldr	r1, [sp, #136]                  @ 4-byte Reload
	ldr	r7, [sp, #748]
	ldr	r5, [sp, #744]
	ldr	r9, [sp, #720]
	ldr	r11, [sp, #724]
	bl	mulPv512x32
	ldr	r0, [sp, #32]                   @ 4-byte Reload
	add	lr, sp, #652
	ldr	r1, [sp, #16]                   @ 4-byte Reload
	adds	r0, r0, r9
	ldr	r0, [sp, #80]                   @ 4-byte Reload
	adcs	r0, r0, r11
	str	r0, [sp, #80]                   @ 4-byte Spill
	ldr	r0, [sp, #76]                   @ 4-byte Reload
	ldr	r11, [sp, #80]                  @ 4-byte Reload
	adcs	r0, r0, r4
	str	r0, [sp, #76]                   @ 4-byte Spill
	ldr	r0, [sp, #72]                   @ 4-byte Reload
	adcs	r0, r0, r6
	str	r0, [sp, #72]                   @ 4-byte Spill
	ldr	r0, [sp, #68]                   @ 4-byte Reload
	adcs	r0, r0, r8
	str	r0, [sp, #68]                   @ 4-byte Spill
	ldr	r0, [sp, #64]                   @ 4-byte Reload
	adcs	r0, r0, r10
	str	r0, [sp, #64]                   @ 4-byte Spill
	ldr	r0, [sp, #60]                   @ 4-byte Reload
	add	r10, sp, #676
	adcs	r0, r0, r5
	str	r0, [sp, #60]                   @ 4-byte Spill
	ldr	r0, [sp, #56]                   @ 4-byte Reload
	adcs	r0, r0, r7
	str	r0, [sp, #56]                   @ 4-byte Spill
	ldr	r0, [sp, #124]                  @ 4-byte Reload
	ldr	r7, [sp, #76]                   @ 4-byte Reload
	adcs	r0, r0, r1
	str	r0, [sp, #124]                  @ 4-byte Spill
	ldr	r0, [sp, #120]                  @ 4-byte Reload
	ldr	r1, [sp, #20]                   @ 4-byte Reload
	adcs	r0, r0, r1
	str	r0, [sp, #120]                  @ 4-byte Spill
	ldr	r0, [sp, #116]                  @ 4-byte Reload
	ldr	r1, [sp, #24]                   @ 4-byte Reload
	adcs	r0, r0, r1
	str	r0, [sp, #116]                  @ 4-byte Spill
	ldr	r0, [sp, #112]                  @ 4-byte Reload
	ldr	r1, [sp, #28]                   @ 4-byte Reload
	adcs	r0, r0, r1
	str	r0, [sp, #112]                  @ 4-byte Spill
	ldr	r0, [sp, #104]                  @ 4-byte Reload
	ldr	r1, [sp, #36]                   @ 4-byte Reload
	adcs	r0, r0, r1
	str	r0, [sp, #104]                  @ 4-byte Spill
	ldr	r0, [sp, #100]                  @ 4-byte Reload
	ldr	r1, [sp, #40]                   @ 4-byte Reload
	adcs	r0, r0, r1
	str	r0, [sp, #100]                  @ 4-byte Spill
	ldr	r0, [sp, #96]                   @ 4-byte Reload
	ldr	r1, [sp, #44]                   @ 4-byte Reload
	adcs	r0, r0, r1
	str	r0, [sp, #96]                   @ 4-byte Spill
	ldr	r0, [sp, #92]                   @ 4-byte Reload
	ldr	r1, [sp, #48]                   @ 4-byte Reload
	adcs	r0, r0, r1
	str	r0, [sp, #92]                   @ 4-byte Spill
	ldr	r0, [sp, #88]                   @ 4-byte Reload
	ldr	r1, [sp, #52]                   @ 4-byte Reload
	adcs	r0, r0, r1
	str	r0, [sp, #88]                   @ 4-byte Spill
	ldr	r0, [sp, #84]                   @ 4-byte Reload
	adc	r0, r0, #0
	str	r0, [sp, #84]                   @ 4-byte Spill
	ldr	r0, [sp, #712]
	str	r0, [sp, #52]                   @ 4-byte Spill
	ldr	r0, [sp, #708]
	str	r0, [sp, #48]                   @ 4-byte Spill
	ldr	r0, [sp, #704]
	str	r0, [sp, #44]                   @ 4-byte Spill
	ldr	r0, [sp, #700]
	str	r0, [sp, #40]                   @ 4-byte Spill
	ldr	r0, [sp, #696]
	str	r0, [sp, #36]                   @ 4-byte Spill
	ldr	r6, [sp, #648]
	ldm	lr, {r0, r1, r2, r3, r12, lr}
	adds	r6, r11, r6
	adcs	r0, r7, r0
	ldm	r10, {r4, r5, r8, r9, r10}
	str	r0, [sp, #80]                   @ 4-byte Spill
	ldr	r0, [sp, #72]                   @ 4-byte Reload
	str	r6, [sp, #32]                   @ 4-byte Spill
	adcs	r0, r0, r1
	str	r0, [sp, #76]                   @ 4-byte Spill
	ldr	r0, [sp, #68]                   @ 4-byte Reload
	ldr	r1, [sp, #36]                   @ 4-byte Reload
	adcs	r0, r0, r2
	str	r0, [sp, #72]                   @ 4-byte Spill
	ldr	r0, [sp, #64]                   @ 4-byte Reload
	adcs	r0, r0, r3
	str	r0, [sp, #68]                   @ 4-byte Spill
	ldr	r0, [sp, #60]                   @ 4-byte Reload
	adcs	r0, r0, r12
	str	r0, [sp, #64]                   @ 4-byte Spill
	ldr	r0, [sp, #56]                   @ 4-byte Reload
	adcs	r0, r0, lr
	str	r0, [sp, #60]                   @ 4-byte Spill
	ldr	r0, [sp, #124]                  @ 4-byte Reload
	adcs	r0, r0, r4
	str	r0, [sp, #56]                   @ 4-byte Spill
	ldr	r0, [sp, #120]                  @ 4-byte Reload
	adcs	r0, r0, r5
	str	r0, [sp, #124]                  @ 4-byte Spill
	ldr	r0, [sp, #116]                  @ 4-byte Reload
	adcs	r0, r0, r8
	str	r0, [sp, #120]                  @ 4-byte Spill
	ldr	r0, [sp, #112]                  @ 4-byte Reload
	adcs	r0, r0, r9
	str	r0, [sp, #116]                  @ 4-byte Spill
	ldr	r0, [sp, #104]                  @ 4-byte Reload
	adcs	r0, r0, r10
	str	r0, [sp, #112]                  @ 4-byte Spill
	ldr	r0, [sp, #100]                  @ 4-byte Reload
	adcs	r0, r0, r1
	str	r0, [sp, #104]                  @ 4-byte Spill
	ldr	r0, [sp, #96]                   @ 4-byte Reload
	ldr	r1, [sp, #40]                   @ 4-byte Reload
	adcs	r0, r0, r1
	str	r0, [sp, #100]                  @ 4-byte Spill
	ldr	r0, [sp, #92]                   @ 4-byte Reload
	ldr	r1, [sp, #44]                   @ 4-byte Reload
	adcs	r0, r0, r1
	str	r0, [sp, #96]                   @ 4-byte Spill
	ldr	r0, [sp, #88]                   @ 4-byte Reload
	ldr	r1, [sp, #48]                   @ 4-byte Reload
	adcs	r0, r0, r1
	str	r0, [sp, #92]                   @ 4-byte Spill
	ldr	r0, [sp, #84]                   @ 4-byte Reload
	ldr	r1, [sp, #52]                   @ 4-byte Reload
	adcs	r0, r0, r1
	str	r0, [sp, #88]                   @ 4-byte Spill
	mov	r0, #0
	ldr	r1, [sp, #140]                  @ 4-byte Reload
	adc	r0, r0, #0
	str	r0, [sp, #84]                   @ 4-byte Spill
	ldr	r0, [sp, #128]                  @ 4-byte Reload
	mul	r2, r0, r6
	add	r0, sp, #576
	bl	mulPv512x32
	ldr	r1, [sp, #132]                  @ 4-byte Reload
	add	r10, sp, #584
	add	r0, sp, #504
	ldr	r2, [r1, #52]
	ldr	r1, [sp, #640]
	str	r1, [sp, #52]                   @ 4-byte Spill
	ldr	r1, [sp, #636]
	str	r1, [sp, #48]                   @ 4-byte Spill
	ldr	r1, [sp, #632]
	str	r1, [sp, #44]                   @ 4-byte Spill
	ldr	r1, [sp, #628]
	str	r1, [sp, #40]                   @ 4-byte Spill
	ldr	r1, [sp, #624]
	str	r1, [sp, #36]                   @ 4-byte Spill
	ldr	r1, [sp, #620]
	str	r1, [sp, #28]                   @ 4-byte Spill
	ldr	r1, [sp, #616]
	str	r1, [sp, #24]                   @ 4-byte Spill
	ldr	r1, [sp, #612]
	str	r1, [sp, #20]                   @ 4-byte Spill
	ldr	r1, [sp, #608]
	str	r1, [sp, #16]                   @ 4-byte Spill
	ldm	r10, {r4, r6, r8, r10}
	ldr	r1, [sp, #136]                  @ 4-byte Reload
	ldr	r7, [sp, #604]
	ldr	r5, [sp, #600]
	ldr	r9, [sp, #576]
	ldr	r11, [sp, #580]
	bl	mulPv512x32
	ldr	r0, [sp, #32]                   @ 4-byte Reload
	add	lr, sp, #508
	ldr	r1, [sp, #16]                   @ 4-byte Reload
	adds	r0, r0, r9
	ldr	r0, [sp, #80]                   @ 4-byte Reload
	adcs	r0, r0, r11
	str	r0, [sp, #80]                   @ 4-byte Spill
	ldr	r0, [sp, #76]                   @ 4-byte Reload
	ldr	r11, [sp, #80]                  @ 4-byte Reload
	adcs	r0, r0, r4
	str	r0, [sp, #76]                   @ 4-byte Spill
	ldr	r0, [sp, #72]                   @ 4-byte Reload
	adcs	r0, r0, r6
	str	r0, [sp, #72]                   @ 4-byte Spill
	ldr	r0, [sp, #68]                   @ 4-byte Reload
	adcs	r0, r0, r8
	str	r0, [sp, #68]                   @ 4-byte Spill
	ldr	r0, [sp, #64]                   @ 4-byte Reload
	adcs	r0, r0, r10
	str	r0, [sp, #64]                   @ 4-byte Spill
	ldr	r0, [sp, #60]                   @ 4-byte Reload
	add	r10, sp, #532
	adcs	r0, r0, r5
	str	r0, [sp, #60]                   @ 4-byte Spill
	ldr	r0, [sp, #56]                   @ 4-byte Reload
	adcs	r0, r0, r7
	str	r0, [sp, #56]                   @ 4-byte Spill
	ldr	r0, [sp, #124]                  @ 4-byte Reload
	ldr	r7, [sp, #76]                   @ 4-byte Reload
	adcs	r0, r0, r1
	str	r0, [sp, #124]                  @ 4-byte Spill
	ldr	r0, [sp, #120]                  @ 4-byte Reload
	ldr	r1, [sp, #20]                   @ 4-byte Reload
	adcs	r0, r0, r1
	str	r0, [sp, #120]                  @ 4-byte Spill
	ldr	r0, [sp, #116]                  @ 4-byte Reload
	ldr	r1, [sp, #24]                   @ 4-byte Reload
	adcs	r0, r0, r1
	str	r0, [sp, #116]                  @ 4-byte Spill
	ldr	r0, [sp, #112]                  @ 4-byte Reload
	ldr	r1, [sp, #28]                   @ 4-byte Reload
	adcs	r0, r0, r1
	str	r0, [sp, #112]                  @ 4-byte Spill
	ldr	r0, [sp, #104]                  @ 4-byte Reload
	ldr	r1, [sp, #36]                   @ 4-byte Reload
	adcs	r0, r0, r1
	str	r0, [sp, #104]                  @ 4-byte Spill
	ldr	r0, [sp, #100]                  @ 4-byte Reload
	ldr	r1, [sp, #40]                   @ 4-byte Reload
	adcs	r0, r0, r1
	str	r0, [sp, #100]                  @ 4-byte Spill
	ldr	r0, [sp, #96]                   @ 4-byte Reload
	ldr	r1, [sp, #44]                   @ 4-byte Reload
	adcs	r0, r0, r1
	str	r0, [sp, #96]                   @ 4-byte Spill
	ldr	r0, [sp, #92]                   @ 4-byte Reload
	ldr	r1, [sp, #48]                   @ 4-byte Reload
	adcs	r0, r0, r1
	str	r0, [sp, #92]                   @ 4-byte Spill
	ldr	r0, [sp, #88]                   @ 4-byte Reload
	ldr	r1, [sp, #52]                   @ 4-byte Reload
	adcs	r0, r0, r1
	str	r0, [sp, #88]                   @ 4-byte Spill
	ldr	r0, [sp, #84]                   @ 4-byte Reload
	adc	r0, r0, #0
	str	r0, [sp, #84]                   @ 4-byte Spill
	ldr	r0, [sp, #568]
	str	r0, [sp, #52]                   @ 4-byte Spill
	ldr	r0, [sp, #564]
	str	r0, [sp, #48]                   @ 4-byte Spill
	ldr	r0, [sp, #560]
	str	r0, [sp, #44]                   @ 4-byte Spill
	ldr	r0, [sp, #556]
	str	r0, [sp, #40]                   @ 4-byte Spill
	ldr	r0, [sp, #552]
	str	r0, [sp, #36]                   @ 4-byte Spill
	ldr	r6, [sp, #504]
	ldm	lr, {r0, r1, r2, r3, r12, lr}
	adds	r6, r11, r6
	adcs	r0, r7, r0
	ldm	r10, {r4, r5, r8, r9, r10}
	str	r0, [sp, #80]                   @ 4-byte Spill
	ldr	r0, [sp, #72]                   @ 4-byte Reload
	str	r6, [sp, #32]                   @ 4-byte Spill
	adcs	r0, r0, r1
	str	r0, [sp, #76]                   @ 4-byte Spill
	ldr	r0, [sp, #68]                   @ 4-byte Reload
	ldr	r1, [sp, #36]                   @ 4-byte Reload
	adcs	r0, r0, r2
	str	r0, [sp, #72]                   @ 4-byte Spill
	ldr	r0, [sp, #64]                   @ 4-byte Reload
	adcs	r0, r0, r3
	str	r0, [sp, #68]                   @ 4-byte Spill
	ldr	r0, [sp, #60]                   @ 4-byte Reload
	adcs	r0, r0, r12
	str	r0, [sp, #64]                   @ 4-byte Spill
	ldr	r0, [sp, #56]                   @ 4-byte Reload
	adcs	r0, r0, lr
	str	r0, [sp, #60]                   @ 4-byte Spill
	ldr	r0, [sp, #124]                  @ 4-byte Reload
	adcs	r0, r0, r4
	str	r0, [sp, #56]                   @ 4-byte Spill
	ldr	r0, [sp, #120]                  @ 4-byte Reload
	adcs	r0, r0, r5
	str	r0, [sp, #124]                  @ 4-byte Spill
	ldr	r0, [sp, #116]                  @ 4-byte Reload
	adcs	r0, r0, r8
	str	r0, [sp, #120]                  @ 4-byte Spill
	ldr	r0, [sp, #112]                  @ 4-byte Reload
	adcs	r0, r0, r9
	str	r0, [sp, #116]                  @ 4-byte Spill
	ldr	r0, [sp, #104]                  @ 4-byte Reload
	adcs	r0, r0, r10
	str	r0, [sp, #112]                  @ 4-byte Spill
	ldr	r0, [sp, #100]                  @ 4-byte Reload
	adcs	r0, r0, r1
	str	r0, [sp, #104]                  @ 4-byte Spill
	ldr	r0, [sp, #96]                   @ 4-byte Reload
	ldr	r1, [sp, #40]                   @ 4-byte Reload
	adcs	r0, r0, r1
	str	r0, [sp, #100]                  @ 4-byte Spill
	ldr	r0, [sp, #92]                   @ 4-byte Reload
	ldr	r1, [sp, #44]                   @ 4-byte Reload
	adcs	r0, r0, r1
	str	r0, [sp, #96]                   @ 4-byte Spill
	ldr	r0, [sp, #88]                   @ 4-byte Reload
	ldr	r1, [sp, #48]                   @ 4-byte Reload
	adcs	r0, r0, r1
	str	r0, [sp, #92]                   @ 4-byte Spill
	ldr	r0, [sp, #84]                   @ 4-byte Reload
	ldr	r1, [sp, #52]                   @ 4-byte Reload
	adcs	r0, r0, r1
	str	r0, [sp, #88]                   @ 4-byte Spill
	mov	r0, #0
	ldr	r1, [sp, #140]                  @ 4-byte Reload
	adc	r0, r0, #0
	str	r0, [sp, #84]                   @ 4-byte Spill
	ldr	r0, [sp, #128]                  @ 4-byte Reload
	mul	r2, r0, r6
	add	r0, sp, #432
	bl	mulPv512x32
	ldr	r1, [sp, #132]                  @ 4-byte Reload
	add	r10, sp, #440
	add	r0, sp, #360
	ldr	r2, [r1, #56]
	ldr	r1, [sp, #496]
	str	r1, [sp, #52]                   @ 4-byte Spill
	ldr	r1, [sp, #492]
	str	r1, [sp, #48]                   @ 4-byte Spill
	ldr	r1, [sp, #488]
	str	r1, [sp, #44]                   @ 4-byte Spill
	ldr	r1, [sp, #484]
	str	r1, [sp, #40]                   @ 4-byte Spill
	ldr	r1, [sp, #480]
	str	r1, [sp, #36]                   @ 4-byte Spill
	ldr	r1, [sp, #476]
	str	r1, [sp, #28]                   @ 4-byte Spill
	ldr	r1, [sp, #472]
	str	r1, [sp, #24]                   @ 4-byte Spill
	ldr	r1, [sp, #468]
	str	r1, [sp, #20]                   @ 4-byte Spill
	ldr	r1, [sp, #464]
	str	r1, [sp, #16]                   @ 4-byte Spill
	ldm	r10, {r4, r6, r8, r10}
	ldr	r1, [sp, #136]                  @ 4-byte Reload
	ldr	r7, [sp, #460]
	ldr	r5, [sp, #456]
	ldr	r9, [sp, #432]
	ldr	r11, [sp, #436]
	bl	mulPv512x32
	ldr	r0, [sp, #32]                   @ 4-byte Reload
	add	lr, sp, #364
	ldr	r1, [sp, #16]                   @ 4-byte Reload
	adds	r0, r0, r9
	ldr	r0, [sp, #80]                   @ 4-byte Reload
	adcs	r0, r0, r11
	str	r0, [sp, #80]                   @ 4-byte Spill
	ldr	r0, [sp, #76]                   @ 4-byte Reload
	ldr	r11, [sp, #80]                  @ 4-byte Reload
	adcs	r0, r0, r4
	str	r0, [sp, #76]                   @ 4-byte Spill
	ldr	r0, [sp, #72]                   @ 4-byte Reload
	adcs	r0, r0, r6
	str	r0, [sp, #72]                   @ 4-byte Spill
	ldr	r0, [sp, #68]                   @ 4-byte Reload
	adcs	r0, r0, r8
	str	r0, [sp, #68]                   @ 4-byte Spill
	ldr	r0, [sp, #64]                   @ 4-byte Reload
	adcs	r0, r0, r10
	str	r0, [sp, #64]                   @ 4-byte Spill
	ldr	r0, [sp, #60]                   @ 4-byte Reload
	add	r10, sp, #388
	adcs	r0, r0, r5
	str	r0, [sp, #60]                   @ 4-byte Spill
	ldr	r0, [sp, #56]                   @ 4-byte Reload
	adcs	r0, r0, r7
	str	r0, [sp, #56]                   @ 4-byte Spill
	ldr	r0, [sp, #124]                  @ 4-byte Reload
	ldr	r7, [sp, #76]                   @ 4-byte Reload
	adcs	r0, r0, r1
	str	r0, [sp, #124]                  @ 4-byte Spill
	ldr	r0, [sp, #120]                  @ 4-byte Reload
	ldr	r1, [sp, #20]                   @ 4-byte Reload
	adcs	r0, r0, r1
	str	r0, [sp, #120]                  @ 4-byte Spill
	ldr	r0, [sp, #116]                  @ 4-byte Reload
	ldr	r1, [sp, #24]                   @ 4-byte Reload
	adcs	r0, r0, r1
	str	r0, [sp, #116]                  @ 4-byte Spill
	ldr	r0, [sp, #112]                  @ 4-byte Reload
	ldr	r1, [sp, #28]                   @ 4-byte Reload
	adcs	r0, r0, r1
	str	r0, [sp, #112]                  @ 4-byte Spill
	ldr	r0, [sp, #104]                  @ 4-byte Reload
	ldr	r1, [sp, #36]                   @ 4-byte Reload
	adcs	r0, r0, r1
	str	r0, [sp, #104]                  @ 4-byte Spill
	ldr	r0, [sp, #100]                  @ 4-byte Reload
	ldr	r1, [sp, #40]                   @ 4-byte Reload
	adcs	r0, r0, r1
	str	r0, [sp, #100]                  @ 4-byte Spill
	ldr	r0, [sp, #96]                   @ 4-byte Reload
	ldr	r1, [sp, #44]                   @ 4-byte Reload
	adcs	r0, r0, r1
	str	r0, [sp, #96]                   @ 4-byte Spill
	ldr	r0, [sp, #92]                   @ 4-byte Reload
	ldr	r1, [sp, #48]                   @ 4-byte Reload
	adcs	r0, r0, r1
	str	r0, [sp, #92]                   @ 4-byte Spill
	ldr	r0, [sp, #88]                   @ 4-byte Reload
	ldr	r1, [sp, #52]                   @ 4-byte Reload
	adcs	r0, r0, r1
	str	r0, [sp, #88]                   @ 4-byte Spill
	ldr	r0, [sp, #84]                   @ 4-byte Reload
	adc	r0, r0, #0
	str	r0, [sp, #84]                   @ 4-byte Spill
	ldr	r0, [sp, #424]
	str	r0, [sp, #52]                   @ 4-byte Spill
	ldr	r0, [sp, #420]
	str	r0, [sp, #48]                   @ 4-byte Spill
	ldr	r0, [sp, #416]
	str	r0, [sp, #44]                   @ 4-byte Spill
	ldr	r0, [sp, #412]
	str	r0, [sp, #40]                   @ 4-byte Spill
	ldr	r0, [sp, #408]
	str	r0, [sp, #32]                   @ 4-byte Spill
	ldr	r6, [sp, #360]
	ldm	lr, {r0, r1, r2, r3, r12, lr}
	adds	r6, r11, r6
	adcs	r0, r7, r0
	ldm	r10, {r4, r5, r8, r9, r10}
	str	r0, [sp, #80]                   @ 4-byte Spill
	ldr	r0, [sp, #72]                   @ 4-byte Reload
	str	r6, [sp, #36]                   @ 4-byte Spill
	adcs	r0, r0, r1
	str	r0, [sp, #76]                   @ 4-byte Spill
	ldr	r0, [sp, #68]                   @ 4-byte Reload
	ldr	r1, [sp, #32]                   @ 4-byte Reload
	adcs	r0, r0, r2
	str	r0, [sp, #72]                   @ 4-byte Spill
	ldr	r0, [sp, #64]                   @ 4-byte Reload
	adcs	r0, r0, r3
	str	r0, [sp, #68]                   @ 4-byte Spill
	ldr	r0, [sp, #60]                   @ 4-byte Reload
	adcs	r0, r0, r12
	str	r0, [sp, #64]                   @ 4-byte Spill
	ldr	r0, [sp, #56]                   @ 4-byte Reload
	adcs	r0, r0, lr
	str	r0, [sp, #60]                   @ 4-byte Spill
	ldr	r0, [sp, #124]                  @ 4-byte Reload
	adcs	r0, r0, r4
	str	r0, [sp, #56]                   @ 4-byte Spill
	ldr	r0, [sp, #120]                  @ 4-byte Reload
	adcs	r0, r0, r5
	str	r0, [sp, #124]                  @ 4-byte Spill
	ldr	r0, [sp, #116]                  @ 4-byte Reload
	adcs	r0, r0, r8
	str	r0, [sp, #120]                  @ 4-byte Spill
	ldr	r0, [sp, #112]                  @ 4-byte Reload
	adcs	r0, r0, r9
	str	r0, [sp, #116]                  @ 4-byte Spill
	ldr	r0, [sp, #104]                  @ 4-byte Reload
	adcs	r0, r0, r10
	str	r0, [sp, #112]                  @ 4-byte Spill
	ldr	r0, [sp, #100]                  @ 4-byte Reload
	adcs	r0, r0, r1
	str	r0, [sp, #104]                  @ 4-byte Spill
	ldr	r0, [sp, #96]                   @ 4-byte Reload
	ldr	r1, [sp, #40]                   @ 4-byte Reload
	adcs	r0, r0, r1
	str	r0, [sp, #100]                  @ 4-byte Spill
	ldr	r0, [sp, #92]                   @ 4-byte Reload
	ldr	r1, [sp, #44]                   @ 4-byte Reload
	adcs	r0, r0, r1
	str	r0, [sp, #96]                   @ 4-byte Spill
	ldr	r0, [sp, #88]                   @ 4-byte Reload
	ldr	r1, [sp, #48]                   @ 4-byte Reload
	adcs	r0, r0, r1
	str	r0, [sp, #92]                   @ 4-byte Spill
	ldr	r0, [sp, #84]                   @ 4-byte Reload
	ldr	r1, [sp, #52]                   @ 4-byte Reload
	adcs	r0, r0, r1
	str	r0, [sp, #88]                   @ 4-byte Spill
	mov	r0, #0
	ldr	r1, [sp, #140]                  @ 4-byte Reload
	adc	r0, r0, #0
	str	r0, [sp, #84]                   @ 4-byte Spill
	ldr	r0, [sp, #128]                  @ 4-byte Reload
	mul	r2, r0, r6
	add	r0, sp, #288
	bl	mulPv512x32
	ldr	r1, [sp, #132]                  @ 4-byte Reload
	add	r0, sp, #216
	ldr	r2, [r1, #60]
	ldr	r1, [sp, #352]
	str	r1, [sp, #132]                  @ 4-byte Spill
	ldr	r1, [sp, #348]
	str	r1, [sp, #52]                   @ 4-byte Spill
	ldr	r1, [sp, #344]
	str	r1, [sp, #48]                   @ 4-byte Spill
	ldr	r1, [sp, #340]
	str	r1, [sp, #44]                   @ 4-byte Spill
	ldr	r1, [sp, #336]
	str	r1, [sp, #40]                   @ 4-byte Spill
	ldr	r1, [sp, #332]
	str	r1, [sp, #32]                   @ 4-byte Spill
	ldr	r1, [sp, #328]
	str	r1, [sp, #28]                   @ 4-byte Spill
	ldr	r1, [sp, #324]
	str	r1, [sp, #24]                   @ 4-byte Spill
	ldr	r1, [sp, #320]
	str	r1, [sp, #20]                   @ 4-byte Spill
	ldr	r1, [sp, #136]                  @ 4-byte Reload
	ldr	r5, [sp, #316]
	ldr	r10, [sp, #312]
	ldr	r8, [sp, #308]
	ldr	r9, [sp, #304]
	ldr	r11, [sp, #288]
	ldr	r4, [sp, #292]
	ldr	r7, [sp, #296]
	ldr	r6, [sp, #300]
	bl	mulPv512x32
	ldr	r0, [sp, #36]                   @ 4-byte Reload
	add	lr, sp, #232
	ldr	r1, [sp, #20]                   @ 4-byte Reload
	adds	r0, r0, r11
	ldr	r0, [sp, #80]                   @ 4-byte Reload
	adcs	r2, r0, r4
	ldr	r0, [sp, #76]                   @ 4-byte Reload
	adcs	r0, r0, r7
	str	r0, [sp, #136]                  @ 4-byte Spill
	ldr	r0, [sp, #72]                   @ 4-byte Reload
	ldr	r11, [sp, #136]                 @ 4-byte Reload
	adcs	r0, r0, r6
	str	r0, [sp, #80]                   @ 4-byte Spill
	ldr	r0, [sp, #68]                   @ 4-byte Reload
	adcs	r0, r0, r9
	str	r0, [sp, #76]                   @ 4-byte Spill
	ldr	r0, [sp, #64]                   @ 4-byte Reload
	adcs	r0, r0, r8
	str	r0, [sp, #72]                   @ 4-byte Spill
	ldr	r0, [sp, #60]                   @ 4-byte Reload
	adcs	r0, r0, r10
	str	r0, [sp, #68]                   @ 4-byte Spill
	ldr	r0, [sp, #56]                   @ 4-byte Reload
	adcs	r0, r0, r5
	str	r0, [sp, #64]                   @ 4-byte Spill
	ldr	r0, [sp, #124]                  @ 4-byte Reload
	ldr	r5, [sp, #80]                   @ 4-byte Reload
	adcs	r0, r0, r1
	str	r0, [sp, #124]                  @ 4-byte Spill
	ldr	r0, [sp, #120]                  @ 4-byte Reload
	ldr	r1, [sp, #24]                   @ 4-byte Reload
	adcs	r0, r0, r1
	str	r0, [sp, #120]                  @ 4-byte Spill
	ldr	r0, [sp, #116]                  @ 4-byte Reload
	ldr	r1, [sp, #28]                   @ 4-byte Reload
	adcs	r0, r0, r1
	str	r0, [sp, #116]                  @ 4-byte Spill
	ldr	r0, [sp, #112]                  @ 4-byte Reload
	ldr	r1, [sp, #32]                   @ 4-byte Reload
	adcs	r0, r0, r1
	str	r0, [sp, #112]                  @ 4-byte Spill
	ldr	r0, [sp, #104]                  @ 4-byte Reload
	ldr	r1, [sp, #40]                   @ 4-byte Reload
	adcs	r0, r0, r1
	str	r0, [sp, #104]                  @ 4-byte Spill
	ldr	r0, [sp, #100]                  @ 4-byte Reload
	ldr	r1, [sp, #44]                   @ 4-byte Reload
	adcs	r0, r0, r1
	str	r0, [sp, #100]                  @ 4-byte Spill
	ldr	r0, [sp, #96]                   @ 4-byte Reload
	ldr	r1, [sp, #48]                   @ 4-byte Reload
	adcs	r0, r0, r1
	str	r0, [sp, #96]                   @ 4-byte Spill
	ldr	r0, [sp, #92]                   @ 4-byte Reload
	ldr	r1, [sp, #52]                   @ 4-byte Reload
	adcs	r0, r0, r1
	str	r0, [sp, #92]                   @ 4-byte Spill
	ldr	r0, [sp, #88]                   @ 4-byte Reload
	ldr	r1, [sp, #132]                  @ 4-byte Reload
	adcs	r0, r0, r1
	str	r0, [sp, #132]                  @ 4-byte Spill
	ldr	r0, [sp, #84]                   @ 4-byte Reload
	adc	r0, r0, #0
	str	r0, [sp, #88]                   @ 4-byte Spill
	ldr	r4, [sp, #216]
	ldr	r0, [sp, #128]                  @ 4-byte Reload
	adds	r9, r2, r4
	ldr	r10, [sp, #220]
	ldr	r6, [sp, #224]
	mul	r1, r0, r9
	ldr	r8, [sp, #228]
	adcs	r10, r11, r10
	adcs	r6, r5, r6
	ldr	r5, [sp, #76]                   @ 4-byte Reload
	ldr	r11, [sp, #140]                 @ 4-byte Reload
	str	r1, [sp, #60]                   @ 4-byte Spill
	adcs	r8, r5, r8
	ldr	r0, [sp, #280]
	str	r0, [sp, #56]                   @ 4-byte Spill
	ldr	r0, [sp, #276]
	str	r0, [sp, #52]                   @ 4-byte Spill
	ldr	r0, [sp, #272]
	str	r0, [sp, #84]                   @ 4-byte Spill
	ldr	r0, [sp, #268]
	str	r0, [sp, #48]                   @ 4-byte Spill
	ldr	r0, [sp, #264]
	str	r0, [sp, #44]                   @ 4-byte Spill
	ldm	lr, {r0, r1, r2, r3, r12, lr}
	ldr	r5, [sp, #72]                   @ 4-byte Reload
	ldr	r7, [sp, #260]
	adcs	r0, r5, r0
	ldr	r4, [sp, #256]
	str	r0, [sp, #80]                   @ 4-byte Spill
	ldr	r0, [sp, #68]                   @ 4-byte Reload
	adcs	r0, r0, r1
	str	r0, [sp, #72]                   @ 4-byte Spill
	ldr	r0, [sp, #64]                   @ 4-byte Reload
	ldr	r1, [sp, #44]                   @ 4-byte Reload
	adcs	r0, r0, r2
	str	r0, [sp, #136]                  @ 4-byte Spill
	ldr	r0, [sp, #124]                  @ 4-byte Reload
	ldr	r2, [sp, #60]                   @ 4-byte Reload
	adcs	r0, r0, r3
	str	r0, [sp, #68]                   @ 4-byte Spill
	ldr	r0, [sp, #120]                  @ 4-byte Reload
	adcs	r0, r0, r12
	str	r0, [sp, #128]                  @ 4-byte Spill
	ldr	r0, [sp, #116]                  @ 4-byte Reload
	adcs	r0, r0, lr
	str	r0, [sp, #124]                  @ 4-byte Spill
	ldr	r0, [sp, #112]                  @ 4-byte Reload
	adcs	r0, r0, r4
	str	r0, [sp, #120]                  @ 4-byte Spill
	ldr	r0, [sp, #104]                  @ 4-byte Reload
	adcs	r0, r0, r7
	str	r0, [sp, #116]                  @ 4-byte Spill
	ldr	r0, [sp, #100]                  @ 4-byte Reload
	adcs	r0, r0, r1
	str	r0, [sp, #112]                  @ 4-byte Spill
	ldr	r0, [sp, #96]                   @ 4-byte Reload
	ldr	r1, [sp, #48]                   @ 4-byte Reload
	adcs	r0, r0, r1
	str	r0, [sp, #76]                   @ 4-byte Spill
	ldr	r0, [sp, #92]                   @ 4-byte Reload
	ldr	r1, [sp, #84]                   @ 4-byte Reload
	adcs	r0, r0, r1
	str	r0, [sp, #84]                   @ 4-byte Spill
	ldr	r0, [sp, #132]                  @ 4-byte Reload
	ldr	r1, [sp, #52]                   @ 4-byte Reload
	adcs	r0, r0, r1
	str	r0, [sp, #92]                   @ 4-byte Spill
	ldr	r0, [sp, #88]                   @ 4-byte Reload
	ldr	r1, [sp, #56]                   @ 4-byte Reload
	adcs	r0, r0, r1
	str	r0, [sp, #100]                  @ 4-byte Spill
	mov	r0, #0
	adc	r5, r0, #0
	add	r0, sp, #144
	mov	r1, r11
	bl	mulPv512x32
	add	r3, sp, #144
	add	lr, r11, #32
	ldm	r3, {r0, r1, r2, r3}
	adds	r0, r9, r0
	adcs	r12, r10, r1
	str	r12, [sp, #104]                 @ 4-byte Spill
	adcs	r6, r6, r2
	str	r6, [sp, #96]                   @ 4-byte Spill
	adcs	r7, r8, r3
	str	r7, [sp, #88]                   @ 4-byte Spill
	ldr	r0, [sp, #160]
	add	r8, r11, #44
	ldr	r1, [sp, #80]                   @ 4-byte Reload
	adcs	r4, r1, r0
	str	r4, [sp, #80]                   @ 4-byte Spill
	ldr	r0, [sp, #164]
	ldr	r1, [sp, #72]                   @ 4-byte Reload
	adcs	r10, r1, r0
	str	r10, [sp, #72]                  @ 4-byte Spill
	ldr	r0, [sp, #168]
	ldr	r1, [sp, #136]                  @ 4-byte Reload
	adcs	r0, r1, r0
	str	r0, [sp, #136]                  @ 4-byte Spill
	ldr	r0, [sp, #172]
	ldr	r1, [sp, #68]                   @ 4-byte Reload
	adcs	r0, r1, r0
	str	r0, [sp, #132]                  @ 4-byte Spill
	ldr	r0, [sp, #176]
	ldr	r1, [sp, #128]                  @ 4-byte Reload
	adcs	r0, r1, r0
	str	r0, [sp, #128]                  @ 4-byte Spill
	ldr	r0, [sp, #180]
	ldr	r1, [sp, #124]                  @ 4-byte Reload
	adcs	r0, r1, r0
	str	r0, [sp, #124]                  @ 4-byte Spill
	ldr	r0, [sp, #184]
	ldr	r1, [sp, #120]                  @ 4-byte Reload
	adcs	r0, r1, r0
	str	r0, [sp, #120]                  @ 4-byte Spill
	ldr	r0, [sp, #188]
	ldr	r1, [sp, #116]                  @ 4-byte Reload
	adcs	r0, r1, r0
	str	r0, [sp, #116]                  @ 4-byte Spill
	ldr	r0, [sp, #192]
	ldr	r1, [sp, #112]                  @ 4-byte Reload
	adcs	r0, r1, r0
	str	r0, [sp, #112]                  @ 4-byte Spill
	ldr	r0, [sp, #196]
	ldr	r1, [sp, #76]                   @ 4-byte Reload
	adcs	r0, r1, r0
	str	r0, [sp, #56]                   @ 4-byte Spill
	ldr	r0, [sp, #200]
	ldr	r1, [sp, #84]                   @ 4-byte Reload
	adcs	r0, r1, r0
	str	r0, [sp, #52]                   @ 4-byte Spill
	ldr	r0, [sp, #204]
	ldr	r1, [sp, #92]                   @ 4-byte Reload
	adcs	r0, r1, r0
	str	r0, [sp, #48]                   @ 4-byte Spill
	ldr	r0, [sp, #208]
	ldr	r1, [sp, #100]                  @ 4-byte Reload
	adcs	r0, r1, r0
	str	r0, [sp, #40]                   @ 4-byte Spill
	adc	r0, r5, #0
	str	r0, [sp, #44]                   @ 4-byte Spill
	ldm	r11, {r1, r2, r3, r5}
	subs	r1, r12, r1
	str	r1, [sp, #100]                  @ 4-byte Spill
	sbcs	r1, r6, r2
	str	r1, [sp, #92]                   @ 4-byte Spill
	sbcs	r1, r7, r3
	ldr	r0, [r11, #16]
	str	r1, [sp, #84]                   @ 4-byte Spill
	sbcs	r1, r4, r5
	str	r1, [sp, #76]                   @ 4-byte Spill
	sbcs	r0, r10, r0
	ldm	lr, {r1, r9, lr}
	ldm	r8, {r4, r5, r6, r7, r8}
	str	r0, [sp, #140]                  @ 4-byte Spill
	ldr	r12, [r11, #20]
	ldr	r0, [sp, #136]                  @ 4-byte Reload
	ldr	r3, [r11, #24]
	sbcs	r0, r0, r12
	str	r0, [sp, #68]                   @ 4-byte Spill
	ldr	r0, [sp, #132]                  @ 4-byte Reload
	ldr	r2, [r11, #28]
	sbcs	r0, r0, r3
	str	r0, [sp, #64]                   @ 4-byte Spill
	ldr	r0, [sp, #128]                  @ 4-byte Reload
	ldr	r10, [sp, #56]                  @ 4-byte Reload
	sbcs	r0, r0, r2
	str	r0, [sp, #60]                   @ 4-byte Spill
	ldr	r0, [sp, #124]                  @ 4-byte Reload
	ldr	r12, [sp, #52]                  @ 4-byte Reload
	sbcs	r11, r0, r1
	ldr	r0, [sp, #120]                  @ 4-byte Reload
	ldr	r2, [sp, #48]                   @ 4-byte Reload
	sbcs	r9, r0, r9
	ldr	r0, [sp, #116]                  @ 4-byte Reload
	ldr	r1, [sp, #40]                   @ 4-byte Reload
	sbcs	lr, r0, lr
	ldr	r0, [sp, #112]                  @ 4-byte Reload
	ldr	r3, [sp, #44]                   @ 4-byte Reload
	sbcs	r4, r0, r4
	sbcs	r5, r10, r5
	sbcs	r6, r12, r6
	sbcs	r7, r2, r7
	sbcs	r0, r1, r8
	sbc	r3, r3, #0
	ands	r8, r3, #1
	ldr	r3, [sp, #108]                  @ 4-byte Reload
	movne	r0, r1
	movne	r7, r2
	movne	r6, r12
	cmp	r8, #0
	str	r0, [r3, #60]
	movne	r5, r10
	ldr	r0, [sp, #112]                  @ 4-byte Reload
	ldr	r1, [sp, #60]                   @ 4-byte Reload
	str	r7, [r3, #56]
	movne	r4, r0
	ldr	r0, [sp, #116]                  @ 4-byte Reload
	str	r6, [r3, #52]
	str	r5, [r3, #48]
	movne	lr, r0
	ldr	r0, [sp, #120]                  @ 4-byte Reload
	cmp	r8, #0
	str	r4, [r3, #44]
	str	lr, [r3, #40]
	movne	r9, r0
	ldr	r0, [sp, #124]                  @ 4-byte Reload
	str	r9, [r3, #36]
	movne	r11, r0
	ldr	r0, [sp, #128]                  @ 4-byte Reload
	str	r11, [r3, #32]
	movne	r1, r0
	ldr	r0, [sp, #132]                  @ 4-byte Reload
	str	r1, [r3, #28]
	cmp	r8, #0
	ldr	r1, [sp, #64]                   @ 4-byte Reload
	movne	r1, r0
	ldr	r0, [sp, #136]                  @ 4-byte Reload
	str	r1, [r3, #24]
	ldr	r1, [sp, #68]                   @ 4-byte Reload
	movne	r1, r0
	ldr	r0, [sp, #140]                  @ 4-byte Reload
	str	r1, [r3, #20]
	ldr	r1, [sp, #72]                   @ 4-byte Reload
	movne	r0, r1
	ldr	r1, [sp, #80]                   @ 4-byte Reload
	str	r0, [r3, #16]
	cmp	r8, #0
	ldr	r0, [sp, #76]                   @ 4-byte Reload
	movne	r0, r1
	ldr	r1, [sp, #88]                   @ 4-byte Reload
	str	r0, [r3, #12]
	ldr	r0, [sp, #84]                   @ 4-byte Reload
	movne	r0, r1
	ldr	r1, [sp, #96]                   @ 4-byte Reload
	str	r0, [r3, #8]
	ldr	r0, [sp, #92]                   @ 4-byte Reload
	movne	r0, r1
	ldr	r1, [sp, #104]                  @ 4-byte Reload
	str	r0, [r3, #4]
	cmp	r8, #0
	ldr	r0, [sp, #100]                  @ 4-byte Reload
	movne	r0, r1
	str	r0, [r3]
	add	sp, sp, #404
	add	sp, sp, #2048
	pop	{r4, r5, r6, r7, r8, r9, r10, r11, lr}
	mov	pc, lr
.Lfunc_end77:
	.size	mcl_fp_mont16L, .Lfunc_end77-mcl_fp_mont16L
	.cantunwind
	.fnend
                                        @ -- End function
	.globl	mcl_fp_montNF16L                @ -- Begin function mcl_fp_montNF16L
	.p2align	2
	.type	mcl_fp_montNF16L,%function
	.code	32                              @ @mcl_fp_montNF16L
mcl_fp_montNF16L:
	.fnstart
@ %bb.0:
	.save	{r4, r5, r6, r7, r8, r9, r10, r11, lr}
	push	{r4, r5, r6, r7, r8, r9, r10, r11, lr}
	.pad	#396
	sub	sp, sp, #396
	.pad	#2048
	sub	sp, sp, #2048
	mov	r7, r2
	ldr	r2, [r2]
	str	r0, [sp, #100]                  @ 4-byte Spill
	add	r0, sp, #2368
	ldr	r5, [r3, #-4]
	mov	r4, r3
	str	r1, [sp, #128]                  @ 4-byte Spill
	str	r5, [sp, #120]                  @ 4-byte Spill
	str	r3, [sp, #132]                  @ 4-byte Spill
	str	r7, [sp, #124]                  @ 4-byte Spill
	bl	mulPv512x32
	ldr	r0, [sp, #2368]
	add	lr, sp, #2048
	str	r0, [sp, #68]                   @ 4-byte Spill
	ldr	r1, [sp, #2372]
	str	r1, [sp, #88]                   @ 4-byte Spill
	mul	r2, r5, r0
	ldr	r1, [sp, #2376]
	str	r1, [sp, #84]                   @ 4-byte Spill
	ldr	r1, [sp, #2380]
	str	r1, [sp, #80]                   @ 4-byte Spill
	mov	r1, r4
	ldr	r0, [sp, #2432]
	str	r0, [sp, #116]                  @ 4-byte Spill
	ldr	r0, [sp, #2428]
	str	r0, [sp, #112]                  @ 4-byte Spill
	ldr	r0, [sp, #2424]
	str	r0, [sp, #108]                  @ 4-byte Spill
	ldr	r0, [sp, #2420]
	str	r0, [sp, #104]                  @ 4-byte Spill
	ldr	r0, [sp, #2416]
	str	r0, [sp, #96]                   @ 4-byte Spill
	ldr	r0, [sp, #2412]
	str	r0, [sp, #92]                   @ 4-byte Spill
	ldr	r0, [sp, #2408]
	str	r0, [sp, #76]                   @ 4-byte Spill
	ldr	r0, [sp, #2404]
	str	r0, [sp, #72]                   @ 4-byte Spill
	ldr	r0, [sp, #2400]
	str	r0, [sp, #64]                   @ 4-byte Spill
	ldr	r0, [sp, #2396]
	str	r0, [sp, #60]                   @ 4-byte Spill
	ldr	r0, [sp, #2392]
	str	r0, [sp, #56]                   @ 4-byte Spill
	ldr	r0, [sp, #2388]
	str	r0, [sp, #48]                   @ 4-byte Spill
	ldr	r0, [sp, #2384]
	str	r0, [sp, #40]                   @ 4-byte Spill
	add	r0, lr, #248
	bl	mulPv512x32
	ldr	r0, [sp, #2360]
	str	r0, [sp, #52]                   @ 4-byte Spill
	ldr	r0, [sp, #2356]
	str	r0, [sp, #44]                   @ 4-byte Spill
	ldr	r0, [sp, #2352]
	str	r0, [sp, #36]                   @ 4-byte Spill
	ldr	r0, [sp, #2348]
	str	r0, [sp, #32]                   @ 4-byte Spill
	ldr	r0, [sp, #2344]
	str	r0, [sp, #28]                   @ 4-byte Spill
	ldr	r0, [sp, #2340]
	str	r0, [sp, #24]                   @ 4-byte Spill
	ldr	r0, [sp, #2336]
	str	r0, [sp, #20]                   @ 4-byte Spill
	ldr	r0, [sp, #2332]
	str	r0, [sp, #16]                   @ 4-byte Spill
	ldr	r0, [sp, #2328]
	ldr	r2, [r7, #4]
	ldr	r1, [sp, #128]                  @ 4-byte Reload
	str	r0, [sp, #12]                   @ 4-byte Spill
	add	r0, sp, #2224
	ldr	r7, [sp, #2324]
	ldr	r5, [sp, #2320]
	ldr	r6, [sp, #2316]
	ldr	r8, [sp, #2312]
	ldr	r9, [sp, #2296]
	ldr	r11, [sp, #2300]
	ldr	r10, [sp, #2304]
	ldr	r4, [sp, #2308]
	bl	mulPv512x32
	ldr	r0, [sp, #68]                   @ 4-byte Reload
	ldr	r1, [sp, #12]                   @ 4-byte Reload
	adds	r0, r9, r0
	ldr	r0, [sp, #88]                   @ 4-byte Reload
	adcs	r0, r11, r0
	str	r0, [sp, #88]                   @ 4-byte Spill
	ldr	r0, [sp, #84]                   @ 4-byte Reload
	ldr	r11, [sp, #88]                  @ 4-byte Reload
	adcs	r0, r10, r0
	str	r0, [sp, #84]                   @ 4-byte Spill
	ldr	r0, [sp, #80]                   @ 4-byte Reload
	adcs	r0, r4, r0
	str	r0, [sp, #80]                   @ 4-byte Spill
	ldr	r0, [sp, #40]                   @ 4-byte Reload
	adcs	r0, r8, r0
	str	r0, [sp, #68]                   @ 4-byte Spill
	ldr	r0, [sp, #48]                   @ 4-byte Reload
	adcs	r0, r6, r0
	str	r0, [sp, #48]                   @ 4-byte Spill
	ldr	r0, [sp, #56]                   @ 4-byte Reload
	adcs	r0, r5, r0
	str	r0, [sp, #56]                   @ 4-byte Spill
	ldr	r0, [sp, #60]                   @ 4-byte Reload
	adcs	r0, r7, r0
	str	r0, [sp, #60]                   @ 4-byte Spill
	ldr	r0, [sp, #64]                   @ 4-byte Reload
	ldr	r7, [sp, #84]                   @ 4-byte Reload
	adcs	r0, r1, r0
	str	r0, [sp, #64]                   @ 4-byte Spill
	ldr	r0, [sp, #72]                   @ 4-byte Reload
	ldr	r1, [sp, #16]                   @ 4-byte Reload
	adcs	r0, r1, r0
	str	r0, [sp, #72]                   @ 4-byte Spill
	ldr	r0, [sp, #76]                   @ 4-byte Reload
	ldr	r1, [sp, #20]                   @ 4-byte Reload
	adcs	r0, r1, r0
	str	r0, [sp, #76]                   @ 4-byte Spill
	ldr	r0, [sp, #92]                   @ 4-byte Reload
	ldr	r1, [sp, #24]                   @ 4-byte Reload
	adcs	r0, r1, r0
	str	r0, [sp, #40]                   @ 4-byte Spill
	ldr	r0, [sp, #96]                   @ 4-byte Reload
	ldr	r1, [sp, #28]                   @ 4-byte Reload
	adcs	r0, r1, r0
	str	r0, [sp, #28]                   @ 4-byte Spill
	ldr	r0, [sp, #104]                  @ 4-byte Reload
	ldr	r1, [sp, #32]                   @ 4-byte Reload
	adcs	r0, r1, r0
	str	r0, [sp, #32]                   @ 4-byte Spill
	ldr	r0, [sp, #108]                  @ 4-byte Reload
	ldr	r1, [sp, #36]                   @ 4-byte Reload
	adcs	r0, r1, r0
	str	r0, [sp, #36]                   @ 4-byte Spill
	ldr	r0, [sp, #112]                  @ 4-byte Reload
	ldr	r1, [sp, #44]                   @ 4-byte Reload
	adcs	r0, r1, r0
	str	r0, [sp, #44]                   @ 4-byte Spill
	ldr	r0, [sp, #116]                  @ 4-byte Reload
	ldr	r1, [sp, #52]                   @ 4-byte Reload
	adc	r0, r1, r0
	str	r0, [sp, #52]                   @ 4-byte Spill
	ldr	r0, [sp, #2288]
	str	r0, [sp, #24]                   @ 4-byte Spill
	ldr	r0, [sp, #2284]
	str	r0, [sp, #16]                   @ 4-byte Spill
	ldr	r0, [sp, #2280]
	str	r0, [sp, #12]                   @ 4-byte Spill
	ldr	r0, [sp, #2276]
	str	r0, [sp, #8]                    @ 4-byte Spill
	ldr	r0, [sp, #2272]
	str	r0, [sp, #4]                    @ 4-byte Spill
	ldr	r6, [sp, #2224]
	ldr	r0, [sp, #2228]
	adds	r6, r11, r6
	ldr	r10, [sp, #2268]
	adcs	r0, r7, r0
	ldr	r9, [sp, #2264]
	ldr	r8, [sp, #2260]
	ldr	r5, [sp, #2256]
	ldr	r4, [sp, #2252]
	ldr	lr, [sp, #2248]
	ldr	r12, [sp, #2244]
	ldr	r3, [sp, #2240]
	ldr	r1, [sp, #2232]
	ldr	r2, [sp, #2236]
	str	r0, [sp, #116]                  @ 4-byte Spill
	ldr	r0, [sp, #80]                   @ 4-byte Reload
	str	r6, [sp, #20]                   @ 4-byte Spill
	adcs	r0, r0, r1
	str	r0, [sp, #112]                  @ 4-byte Spill
	ldr	r0, [sp, #68]                   @ 4-byte Reload
	ldr	r1, [sp, #4]                    @ 4-byte Reload
	adcs	r0, r0, r2
	str	r0, [sp, #108]                  @ 4-byte Spill
	ldr	r0, [sp, #48]                   @ 4-byte Reload
	adcs	r0, r0, r3
	str	r0, [sp, #104]                  @ 4-byte Spill
	ldr	r0, [sp, #56]                   @ 4-byte Reload
	adcs	r0, r0, r12
	str	r0, [sp, #96]                   @ 4-byte Spill
	ldr	r0, [sp, #60]                   @ 4-byte Reload
	adcs	r0, r0, lr
	str	r0, [sp, #92]                   @ 4-byte Spill
	ldr	r0, [sp, #64]                   @ 4-byte Reload
	add	lr, sp, #2048
	adcs	r0, r0, r4
	str	r0, [sp, #88]                   @ 4-byte Spill
	ldr	r0, [sp, #72]                   @ 4-byte Reload
	adcs	r0, r0, r5
	str	r0, [sp, #84]                   @ 4-byte Spill
	ldr	r0, [sp, #76]                   @ 4-byte Reload
	adcs	r0, r0, r8
	str	r0, [sp, #80]                   @ 4-byte Spill
	ldr	r0, [sp, #40]                   @ 4-byte Reload
	adcs	r0, r0, r9
	str	r0, [sp, #76]                   @ 4-byte Spill
	ldr	r0, [sp, #28]                   @ 4-byte Reload
	adcs	r0, r0, r10
	str	r0, [sp, #72]                   @ 4-byte Spill
	ldr	r0, [sp, #32]                   @ 4-byte Reload
	adcs	r0, r0, r1
	str	r0, [sp, #68]                   @ 4-byte Spill
	ldr	r0, [sp, #36]                   @ 4-byte Reload
	ldr	r1, [sp, #8]                    @ 4-byte Reload
	adcs	r0, r0, r1
	str	r0, [sp, #64]                   @ 4-byte Spill
	ldr	r0, [sp, #44]                   @ 4-byte Reload
	ldr	r1, [sp, #12]                   @ 4-byte Reload
	adcs	r0, r0, r1
	str	r0, [sp, #60]                   @ 4-byte Spill
	ldr	r0, [sp, #52]                   @ 4-byte Reload
	ldr	r1, [sp, #16]                   @ 4-byte Reload
	adcs	r0, r0, r1
	str	r0, [sp, #56]                   @ 4-byte Spill
	ldr	r0, [sp, #24]                   @ 4-byte Reload
	ldr	r1, [sp, #132]                  @ 4-byte Reload
	adc	r0, r0, #0
	str	r0, [sp, #52]                   @ 4-byte Spill
	ldr	r0, [sp, #120]                  @ 4-byte Reload
	mul	r2, r0, r6
	add	r0, lr, #104
	bl	mulPv512x32
	ldr	r0, [sp, #124]                  @ 4-byte Reload
	add	r9, sp, #2160
	ldr	r1, [sp, #128]                  @ 4-byte Reload
	ldr	r2, [r0, #8]
	ldr	r0, [sp, #2216]
	str	r0, [sp, #48]                   @ 4-byte Spill
	ldr	r0, [sp, #2212]
	str	r0, [sp, #44]                   @ 4-byte Spill
	ldr	r0, [sp, #2208]
	str	r0, [sp, #40]                   @ 4-byte Spill
	ldr	r0, [sp, #2204]
	str	r0, [sp, #36]                   @ 4-byte Spill
	ldr	r0, [sp, #2200]
	str	r0, [sp, #32]                   @ 4-byte Spill
	ldr	r0, [sp, #2196]
	str	r0, [sp, #28]                   @ 4-byte Spill
	ldr	r0, [sp, #2192]
	str	r0, [sp, #24]                   @ 4-byte Spill
	ldr	r0, [sp, #2188]
	str	r0, [sp, #16]                   @ 4-byte Spill
	ldr	r0, [sp, #2184]
	str	r0, [sp, #12]                   @ 4-byte Spill
	add	r0, sp, #2080
	ldm	r9, {r4, r6, r9}
	ldr	r7, [sp, #2180]
	ldr	r5, [sp, #2176]
	ldr	r8, [sp, #2172]
	ldr	r10, [sp, #2152]
	ldr	r11, [sp, #2156]
	bl	mulPv512x32
	ldr	r0, [sp, #20]                   @ 4-byte Reload
	ldr	r1, [sp, #12]                   @ 4-byte Reload
	adds	r0, r0, r10
	ldr	r0, [sp, #116]                  @ 4-byte Reload
	adcs	r0, r0, r11
	str	r0, [sp, #116]                  @ 4-byte Spill
	ldr	r0, [sp, #112]                  @ 4-byte Reload
	ldr	r11, [sp, #116]                 @ 4-byte Reload
	adcs	r0, r0, r4
	str	r0, [sp, #112]                  @ 4-byte Spill
	ldr	r0, [sp, #108]                  @ 4-byte Reload
	adcs	r0, r0, r6
	str	r0, [sp, #108]                  @ 4-byte Spill
	ldr	r0, [sp, #104]                  @ 4-byte Reload
	adcs	r0, r0, r9
	str	r0, [sp, #104]                  @ 4-byte Spill
	ldr	r0, [sp, #96]                   @ 4-byte Reload
	adcs	r0, r0, r8
	str	r0, [sp, #96]                   @ 4-byte Spill
	ldr	r0, [sp, #92]                   @ 4-byte Reload
	adcs	r0, r0, r5
	str	r0, [sp, #92]                   @ 4-byte Spill
	ldr	r0, [sp, #88]                   @ 4-byte Reload
	adcs	r0, r0, r7
	str	r0, [sp, #88]                   @ 4-byte Spill
	ldr	r0, [sp, #84]                   @ 4-byte Reload
	ldr	r7, [sp, #112]                  @ 4-byte Reload
	adcs	r0, r0, r1
	str	r0, [sp, #84]                   @ 4-byte Spill
	ldr	r0, [sp, #80]                   @ 4-byte Reload
	ldr	r1, [sp, #16]                   @ 4-byte Reload
	adcs	r0, r0, r1
	str	r0, [sp, #80]                   @ 4-byte Spill
	ldr	r0, [sp, #76]                   @ 4-byte Reload
	ldr	r1, [sp, #24]                   @ 4-byte Reload
	adcs	r0, r0, r1
	str	r0, [sp, #76]                   @ 4-byte Spill
	ldr	r0, [sp, #72]                   @ 4-byte Reload
	ldr	r1, [sp, #28]                   @ 4-byte Reload
	adcs	r0, r0, r1
	str	r0, [sp, #72]                   @ 4-byte Spill
	ldr	r0, [sp, #68]                   @ 4-byte Reload
	ldr	r1, [sp, #32]                   @ 4-byte Reload
	adcs	r0, r0, r1
	str	r0, [sp, #68]                   @ 4-byte Spill
	ldr	r0, [sp, #64]                   @ 4-byte Reload
	ldr	r1, [sp, #36]                   @ 4-byte Reload
	adcs	r0, r0, r1
	str	r0, [sp, #64]                   @ 4-byte Spill
	ldr	r0, [sp, #60]                   @ 4-byte Reload
	ldr	r1, [sp, #40]                   @ 4-byte Reload
	adcs	r0, r0, r1
	str	r0, [sp, #60]                   @ 4-byte Spill
	ldr	r0, [sp, #56]                   @ 4-byte Reload
	ldr	r1, [sp, #44]                   @ 4-byte Reload
	adcs	r0, r0, r1
	str	r0, [sp, #56]                   @ 4-byte Spill
	ldr	r0, [sp, #52]                   @ 4-byte Reload
	ldr	r1, [sp, #48]                   @ 4-byte Reload
	adc	r0, r0, r1
	str	r0, [sp, #52]                   @ 4-byte Spill
	ldr	r0, [sp, #2144]
	str	r0, [sp, #48]                   @ 4-byte Spill
	ldr	r0, [sp, #2140]
	str	r0, [sp, #44]                   @ 4-byte Spill
	ldr	r0, [sp, #2136]
	str	r0, [sp, #40]                   @ 4-byte Spill
	ldr	r0, [sp, #2132]
	str	r0, [sp, #36]                   @ 4-byte Spill
	ldr	r0, [sp, #2128]
	str	r0, [sp, #28]                   @ 4-byte Spill
	ldr	r6, [sp, #2080]
	ldr	r0, [sp, #2084]
	adds	r6, r11, r6
	ldr	r10, [sp, #2124]
	adcs	r0, r7, r0
	ldr	r9, [sp, #2120]
	ldr	r8, [sp, #2116]
	ldr	r5, [sp, #2112]
	ldr	r4, [sp, #2108]
	ldr	lr, [sp, #2104]
	ldr	r12, [sp, #2100]
	ldr	r3, [sp, #2096]
	ldr	r1, [sp, #2088]
	ldr	r2, [sp, #2092]
	str	r0, [sp, #116]                  @ 4-byte Spill
	ldr	r0, [sp, #108]                  @ 4-byte Reload
	str	r6, [sp, #32]                   @ 4-byte Spill
	adcs	r0, r0, r1
	str	r0, [sp, #112]                  @ 4-byte Spill
	ldr	r0, [sp, #104]                  @ 4-byte Reload
	ldr	r1, [sp, #28]                   @ 4-byte Reload
	adcs	r0, r0, r2
	str	r0, [sp, #108]                  @ 4-byte Spill
	ldr	r0, [sp, #96]                   @ 4-byte Reload
	adcs	r0, r0, r3
	str	r0, [sp, #104]                  @ 4-byte Spill
	ldr	r0, [sp, #92]                   @ 4-byte Reload
	adcs	r0, r0, r12
	str	r0, [sp, #96]                   @ 4-byte Spill
	ldr	r0, [sp, #88]                   @ 4-byte Reload
	adcs	r0, r0, lr
	str	r0, [sp, #92]                   @ 4-byte Spill
	ldr	r0, [sp, #84]                   @ 4-byte Reload
	add	lr, sp, #1024
	adcs	r0, r0, r4
	str	r0, [sp, #88]                   @ 4-byte Spill
	ldr	r0, [sp, #80]                   @ 4-byte Reload
	adcs	r0, r0, r5
	str	r0, [sp, #84]                   @ 4-byte Spill
	ldr	r0, [sp, #76]                   @ 4-byte Reload
	adcs	r0, r0, r8
	str	r0, [sp, #80]                   @ 4-byte Spill
	ldr	r0, [sp, #72]                   @ 4-byte Reload
	adcs	r0, r0, r9
	str	r0, [sp, #76]                   @ 4-byte Spill
	ldr	r0, [sp, #68]                   @ 4-byte Reload
	adcs	r0, r0, r10
	str	r0, [sp, #72]                   @ 4-byte Spill
	ldr	r0, [sp, #64]                   @ 4-byte Reload
	adcs	r0, r0, r1
	str	r0, [sp, #68]                   @ 4-byte Spill
	ldr	r0, [sp, #60]                   @ 4-byte Reload
	ldr	r1, [sp, #36]                   @ 4-byte Reload
	adcs	r0, r0, r1
	str	r0, [sp, #64]                   @ 4-byte Spill
	ldr	r0, [sp, #56]                   @ 4-byte Reload
	ldr	r1, [sp, #40]                   @ 4-byte Reload
	adcs	r0, r0, r1
	str	r0, [sp, #60]                   @ 4-byte Spill
	ldr	r0, [sp, #52]                   @ 4-byte Reload
	ldr	r1, [sp, #44]                   @ 4-byte Reload
	adcs	r0, r0, r1
	str	r0, [sp, #56]                   @ 4-byte Spill
	ldr	r0, [sp, #48]                   @ 4-byte Reload
	ldr	r1, [sp, #132]                  @ 4-byte Reload
	adc	r0, r0, #0
	str	r0, [sp, #52]                   @ 4-byte Spill
	ldr	r0, [sp, #120]                  @ 4-byte Reload
	mul	r2, r0, r6
	add	r0, lr, #984
	bl	mulPv512x32
	ldr	r0, [sp, #124]                  @ 4-byte Reload
	add	r9, sp, #2016
	ldr	r1, [sp, #128]                  @ 4-byte Reload
	ldr	r2, [r0, #12]
	ldr	r0, [sp, #2072]
	str	r0, [sp, #48]                   @ 4-byte Spill
	ldr	r0, [sp, #2068]
	str	r0, [sp, #44]                   @ 4-byte Spill
	ldr	r0, [sp, #2064]
	str	r0, [sp, #40]                   @ 4-byte Spill
	ldr	r0, [sp, #2060]
	str	r0, [sp, #36]                   @ 4-byte Spill
	ldr	r0, [sp, #2056]
	str	r0, [sp, #28]                   @ 4-byte Spill
	ldr	r0, [sp, #2052]
	str	r0, [sp, #24]                   @ 4-byte Spill
	ldr	r0, [sp, #2048]
	str	r0, [sp, #20]                   @ 4-byte Spill
	ldr	r0, [sp, #2044]
	str	r0, [sp, #16]                   @ 4-byte Spill
	ldr	r0, [sp, #2040]
	str	r0, [sp, #12]                   @ 4-byte Spill
	add	r0, sp, #1936
	ldm	r9, {r4, r6, r9}
	ldr	r7, [sp, #2036]
	ldr	r5, [sp, #2032]
	ldr	r8, [sp, #2028]
	ldr	r10, [sp, #2008]
	ldr	r11, [sp, #2012]
	bl	mulPv512x32
	ldr	r0, [sp, #32]                   @ 4-byte Reload
	ldr	r1, [sp, #12]                   @ 4-byte Reload
	adds	r0, r0, r10
	ldr	r0, [sp, #116]                  @ 4-byte Reload
	adcs	r0, r0, r11
	str	r0, [sp, #116]                  @ 4-byte Spill
	ldr	r0, [sp, #112]                  @ 4-byte Reload
	ldr	r11, [sp, #116]                 @ 4-byte Reload
	adcs	r0, r0, r4
	str	r0, [sp, #112]                  @ 4-byte Spill
	ldr	r0, [sp, #108]                  @ 4-byte Reload
	adcs	r0, r0, r6
	str	r0, [sp, #108]                  @ 4-byte Spill
	ldr	r0, [sp, #104]                  @ 4-byte Reload
	adcs	r0, r0, r9
	str	r0, [sp, #104]                  @ 4-byte Spill
	ldr	r0, [sp, #96]                   @ 4-byte Reload
	adcs	r0, r0, r8
	str	r0, [sp, #96]                   @ 4-byte Spill
	ldr	r0, [sp, #92]                   @ 4-byte Reload
	adcs	r0, r0, r5
	str	r0, [sp, #92]                   @ 4-byte Spill
	ldr	r0, [sp, #88]                   @ 4-byte Reload
	adcs	r0, r0, r7
	str	r0, [sp, #88]                   @ 4-byte Spill
	ldr	r0, [sp, #84]                   @ 4-byte Reload
	ldr	r7, [sp, #112]                  @ 4-byte Reload
	adcs	r0, r0, r1
	str	r0, [sp, #84]                   @ 4-byte Spill
	ldr	r0, [sp, #80]                   @ 4-byte Reload
	ldr	r1, [sp, #16]                   @ 4-byte Reload
	adcs	r0, r0, r1
	str	r0, [sp, #80]                   @ 4-byte Spill
	ldr	r0, [sp, #76]                   @ 4-byte Reload
	ldr	r1, [sp, #20]                   @ 4-byte Reload
	adcs	r0, r0, r1
	str	r0, [sp, #76]                   @ 4-byte Spill
	ldr	r0, [sp, #72]                   @ 4-byte Reload
	ldr	r1, [sp, #24]                   @ 4-byte Reload
	adcs	r0, r0, r1
	str	r0, [sp, #72]                   @ 4-byte Spill
	ldr	r0, [sp, #68]                   @ 4-byte Reload
	ldr	r1, [sp, #28]                   @ 4-byte Reload
	adcs	r0, r0, r1
	str	r0, [sp, #68]                   @ 4-byte Spill
	ldr	r0, [sp, #64]                   @ 4-byte Reload
	ldr	r1, [sp, #36]                   @ 4-byte Reload
	adcs	r0, r0, r1
	str	r0, [sp, #64]                   @ 4-byte Spill
	ldr	r0, [sp, #60]                   @ 4-byte Reload
	ldr	r1, [sp, #40]                   @ 4-byte Reload
	adcs	r0, r0, r1
	str	r0, [sp, #60]                   @ 4-byte Spill
	ldr	r0, [sp, #56]                   @ 4-byte Reload
	ldr	r1, [sp, #44]                   @ 4-byte Reload
	adcs	r0, r0, r1
	str	r0, [sp, #56]                   @ 4-byte Spill
	ldr	r0, [sp, #52]                   @ 4-byte Reload
	ldr	r1, [sp, #48]                   @ 4-byte Reload
	adc	r0, r0, r1
	str	r0, [sp, #52]                   @ 4-byte Spill
	ldr	r0, [sp, #2000]
	str	r0, [sp, #48]                   @ 4-byte Spill
	ldr	r0, [sp, #1996]
	str	r0, [sp, #44]                   @ 4-byte Spill
	ldr	r0, [sp, #1992]
	str	r0, [sp, #40]                   @ 4-byte Spill
	ldr	r0, [sp, #1988]
	str	r0, [sp, #36]                   @ 4-byte Spill
	ldr	r0, [sp, #1984]
	str	r0, [sp, #28]                   @ 4-byte Spill
	ldr	r6, [sp, #1936]
	ldr	r0, [sp, #1940]
	adds	r6, r11, r6
	ldr	r10, [sp, #1980]
	adcs	r0, r7, r0
	ldr	r9, [sp, #1976]
	ldr	r8, [sp, #1972]
	ldr	r5, [sp, #1968]
	ldr	r4, [sp, #1964]
	ldr	lr, [sp, #1960]
	ldr	r12, [sp, #1956]
	ldr	r3, [sp, #1952]
	ldr	r1, [sp, #1944]
	ldr	r2, [sp, #1948]
	str	r0, [sp, #116]                  @ 4-byte Spill
	ldr	r0, [sp, #108]                  @ 4-byte Reload
	str	r6, [sp, #32]                   @ 4-byte Spill
	adcs	r0, r0, r1
	str	r0, [sp, #112]                  @ 4-byte Spill
	ldr	r0, [sp, #104]                  @ 4-byte Reload
	ldr	r1, [sp, #28]                   @ 4-byte Reload
	adcs	r0, r0, r2
	str	r0, [sp, #108]                  @ 4-byte Spill
	ldr	r0, [sp, #96]                   @ 4-byte Reload
	adcs	r0, r0, r3
	str	r0, [sp, #104]                  @ 4-byte Spill
	ldr	r0, [sp, #92]                   @ 4-byte Reload
	adcs	r0, r0, r12
	str	r0, [sp, #96]                   @ 4-byte Spill
	ldr	r0, [sp, #88]                   @ 4-byte Reload
	adcs	r0, r0, lr
	str	r0, [sp, #92]                   @ 4-byte Spill
	ldr	r0, [sp, #84]                   @ 4-byte Reload
	add	lr, sp, #1024
	adcs	r0, r0, r4
	str	r0, [sp, #88]                   @ 4-byte Spill
	ldr	r0, [sp, #80]                   @ 4-byte Reload
	adcs	r0, r0, r5
	str	r0, [sp, #84]                   @ 4-byte Spill
	ldr	r0, [sp, #76]                   @ 4-byte Reload
	adcs	r0, r0, r8
	str	r0, [sp, #80]                   @ 4-byte Spill
	ldr	r0, [sp, #72]                   @ 4-byte Reload
	adcs	r0, r0, r9
	str	r0, [sp, #76]                   @ 4-byte Spill
	ldr	r0, [sp, #68]                   @ 4-byte Reload
	adcs	r0, r0, r10
	str	r0, [sp, #72]                   @ 4-byte Spill
	ldr	r0, [sp, #64]                   @ 4-byte Reload
	adcs	r0, r0, r1
	str	r0, [sp, #68]                   @ 4-byte Spill
	ldr	r0, [sp, #60]                   @ 4-byte Reload
	ldr	r1, [sp, #36]                   @ 4-byte Reload
	adcs	r0, r0, r1
	str	r0, [sp, #64]                   @ 4-byte Spill
	ldr	r0, [sp, #56]                   @ 4-byte Reload
	ldr	r1, [sp, #40]                   @ 4-byte Reload
	adcs	r0, r0, r1
	str	r0, [sp, #60]                   @ 4-byte Spill
	ldr	r0, [sp, #52]                   @ 4-byte Reload
	ldr	r1, [sp, #44]                   @ 4-byte Reload
	adcs	r0, r0, r1
	str	r0, [sp, #56]                   @ 4-byte Spill
	ldr	r0, [sp, #48]                   @ 4-byte Reload
	ldr	r1, [sp, #132]                  @ 4-byte Reload
	adc	r0, r0, #0
	str	r0, [sp, #52]                   @ 4-byte Spill
	ldr	r0, [sp, #120]                  @ 4-byte Reload
	mul	r2, r0, r6
	add	r0, lr, #840
	bl	mulPv512x32
	ldr	r0, [sp, #124]                  @ 4-byte Reload
	add	r9, sp, #1872
	ldr	r1, [sp, #128]                  @ 4-byte Reload
	ldr	r2, [r0, #16]
	ldr	r0, [sp, #1928]
	str	r0, [sp, #48]                   @ 4-byte Spill
	ldr	r0, [sp, #1924]
	str	r0, [sp, #44]                   @ 4-byte Spill
	ldr	r0, [sp, #1920]
	str	r0, [sp, #40]                   @ 4-byte Spill
	ldr	r0, [sp, #1916]
	str	r0, [sp, #36]                   @ 4-byte Spill
	ldr	r0, [sp, #1912]
	str	r0, [sp, #28]                   @ 4-byte Spill
	ldr	r0, [sp, #1908]
	str	r0, [sp, #24]                   @ 4-byte Spill
	ldr	r0, [sp, #1904]
	str	r0, [sp, #20]                   @ 4-byte Spill
	ldr	r0, [sp, #1900]
	str	r0, [sp, #16]                   @ 4-byte Spill
	ldr	r0, [sp, #1896]
	str	r0, [sp, #12]                   @ 4-byte Spill
	add	r0, sp, #1792
	ldm	r9, {r4, r6, r9}
	ldr	r7, [sp, #1892]
	ldr	r5, [sp, #1888]
	ldr	r8, [sp, #1884]
	ldr	r10, [sp, #1864]
	ldr	r11, [sp, #1868]
	bl	mulPv512x32
	ldr	r0, [sp, #32]                   @ 4-byte Reload
	ldr	r1, [sp, #12]                   @ 4-byte Reload
	adds	r0, r0, r10
	ldr	r0, [sp, #116]                  @ 4-byte Reload
	adcs	r0, r0, r11
	str	r0, [sp, #116]                  @ 4-byte Spill
	ldr	r0, [sp, #112]                  @ 4-byte Reload
	ldr	r11, [sp, #116]                 @ 4-byte Reload
	adcs	r0, r0, r4
	str	r0, [sp, #112]                  @ 4-byte Spill
	ldr	r0, [sp, #108]                  @ 4-byte Reload
	adcs	r0, r0, r6
	str	r0, [sp, #108]                  @ 4-byte Spill
	ldr	r0, [sp, #104]                  @ 4-byte Reload
	adcs	r0, r0, r9
	str	r0, [sp, #104]                  @ 4-byte Spill
	ldr	r0, [sp, #96]                   @ 4-byte Reload
	adcs	r0, r0, r8
	str	r0, [sp, #96]                   @ 4-byte Spill
	ldr	r0, [sp, #92]                   @ 4-byte Reload
	adcs	r0, r0, r5
	str	r0, [sp, #92]                   @ 4-byte Spill
	ldr	r0, [sp, #88]                   @ 4-byte Reload
	adcs	r0, r0, r7
	str	r0, [sp, #88]                   @ 4-byte Spill
	ldr	r0, [sp, #84]                   @ 4-byte Reload
	ldr	r7, [sp, #112]                  @ 4-byte Reload
	adcs	r0, r0, r1
	str	r0, [sp, #84]                   @ 4-byte Spill
	ldr	r0, [sp, #80]                   @ 4-byte Reload
	ldr	r1, [sp, #16]                   @ 4-byte Reload
	adcs	r0, r0, r1
	str	r0, [sp, #80]                   @ 4-byte Spill
	ldr	r0, [sp, #76]                   @ 4-byte Reload
	ldr	r1, [sp, #20]                   @ 4-byte Reload
	adcs	r0, r0, r1
	str	r0, [sp, #76]                   @ 4-byte Spill
	ldr	r0, [sp, #72]                   @ 4-byte Reload
	ldr	r1, [sp, #24]                   @ 4-byte Reload
	adcs	r0, r0, r1
	str	r0, [sp, #72]                   @ 4-byte Spill
	ldr	r0, [sp, #68]                   @ 4-byte Reload
	ldr	r1, [sp, #28]                   @ 4-byte Reload
	adcs	r0, r0, r1
	str	r0, [sp, #68]                   @ 4-byte Spill
	ldr	r0, [sp, #64]                   @ 4-byte Reload
	ldr	r1, [sp, #36]                   @ 4-byte Reload
	adcs	r0, r0, r1
	str	r0, [sp, #64]                   @ 4-byte Spill
	ldr	r0, [sp, #60]                   @ 4-byte Reload
	ldr	r1, [sp, #40]                   @ 4-byte Reload
	adcs	r0, r0, r1
	str	r0, [sp, #60]                   @ 4-byte Spill
	ldr	r0, [sp, #56]                   @ 4-byte Reload
	ldr	r1, [sp, #44]                   @ 4-byte Reload
	adcs	r0, r0, r1
	str	r0, [sp, #56]                   @ 4-byte Spill
	ldr	r0, [sp, #52]                   @ 4-byte Reload
	ldr	r1, [sp, #48]                   @ 4-byte Reload
	adc	r0, r0, r1
	str	r0, [sp, #52]                   @ 4-byte Spill
	ldr	r0, [sp, #1856]
	str	r0, [sp, #48]                   @ 4-byte Spill
	ldr	r0, [sp, #1852]
	str	r0, [sp, #44]                   @ 4-byte Spill
	ldr	r0, [sp, #1848]
	str	r0, [sp, #40]                   @ 4-byte Spill
	ldr	r0, [sp, #1844]
	str	r0, [sp, #36]                   @ 4-byte Spill
	ldr	r0, [sp, #1840]
	str	r0, [sp, #28]                   @ 4-byte Spill
	ldr	r6, [sp, #1792]
	ldr	r0, [sp, #1796]
	adds	r6, r11, r6
	ldr	r10, [sp, #1836]
	adcs	r0, r7, r0
	ldr	r9, [sp, #1832]
	ldr	r8, [sp, #1828]
	ldr	r5, [sp, #1824]
	ldr	r4, [sp, #1820]
	ldr	lr, [sp, #1816]
	ldr	r12, [sp, #1812]
	ldr	r3, [sp, #1808]
	ldr	r1, [sp, #1800]
	ldr	r2, [sp, #1804]
	str	r0, [sp, #116]                  @ 4-byte Spill
	ldr	r0, [sp, #108]                  @ 4-byte Reload
	str	r6, [sp, #32]                   @ 4-byte Spill
	adcs	r0, r0, r1
	str	r0, [sp, #112]                  @ 4-byte Spill
	ldr	r0, [sp, #104]                  @ 4-byte Reload
	ldr	r1, [sp, #28]                   @ 4-byte Reload
	adcs	r0, r0, r2
	str	r0, [sp, #108]                  @ 4-byte Spill
	ldr	r0, [sp, #96]                   @ 4-byte Reload
	adcs	r0, r0, r3
	str	r0, [sp, #104]                  @ 4-byte Spill
	ldr	r0, [sp, #92]                   @ 4-byte Reload
	adcs	r0, r0, r12
	str	r0, [sp, #96]                   @ 4-byte Spill
	ldr	r0, [sp, #88]                   @ 4-byte Reload
	adcs	r0, r0, lr
	str	r0, [sp, #92]                   @ 4-byte Spill
	ldr	r0, [sp, #84]                   @ 4-byte Reload
	add	lr, sp, #1024
	adcs	r0, r0, r4
	str	r0, [sp, #88]                   @ 4-byte Spill
	ldr	r0, [sp, #80]                   @ 4-byte Reload
	adcs	r0, r0, r5
	str	r0, [sp, #84]                   @ 4-byte Spill
	ldr	r0, [sp, #76]                   @ 4-byte Reload
	adcs	r0, r0, r8
	str	r0, [sp, #80]                   @ 4-byte Spill
	ldr	r0, [sp, #72]                   @ 4-byte Reload
	adcs	r0, r0, r9
	str	r0, [sp, #76]                   @ 4-byte Spill
	ldr	r0, [sp, #68]                   @ 4-byte Reload
	adcs	r0, r0, r10
	str	r0, [sp, #72]                   @ 4-byte Spill
	ldr	r0, [sp, #64]                   @ 4-byte Reload
	adcs	r0, r0, r1
	str	r0, [sp, #68]                   @ 4-byte Spill
	ldr	r0, [sp, #60]                   @ 4-byte Reload
	ldr	r1, [sp, #36]                   @ 4-byte Reload
	adcs	r0, r0, r1
	str	r0, [sp, #64]                   @ 4-byte Spill
	ldr	r0, [sp, #56]                   @ 4-byte Reload
	ldr	r1, [sp, #40]                   @ 4-byte Reload
	adcs	r0, r0, r1
	str	r0, [sp, #60]                   @ 4-byte Spill
	ldr	r0, [sp, #52]                   @ 4-byte Reload
	ldr	r1, [sp, #44]                   @ 4-byte Reload
	adcs	r0, r0, r1
	str	r0, [sp, #56]                   @ 4-byte Spill
	ldr	r0, [sp, #48]                   @ 4-byte Reload
	ldr	r1, [sp, #132]                  @ 4-byte Reload
	adc	r0, r0, #0
	str	r0, [sp, #52]                   @ 4-byte Spill
	ldr	r0, [sp, #120]                  @ 4-byte Reload
	mul	r2, r0, r6
	add	r0, lr, #696
	bl	mulPv512x32
	ldr	r0, [sp, #124]                  @ 4-byte Reload
	add	r9, sp, #1728
	ldr	r1, [sp, #128]                  @ 4-byte Reload
	ldr	r2, [r0, #20]
	ldr	r0, [sp, #1784]
	str	r0, [sp, #48]                   @ 4-byte Spill
	ldr	r0, [sp, #1780]
	str	r0, [sp, #44]                   @ 4-byte Spill
	ldr	r0, [sp, #1776]
	str	r0, [sp, #40]                   @ 4-byte Spill
	ldr	r0, [sp, #1772]
	str	r0, [sp, #36]                   @ 4-byte Spill
	ldr	r0, [sp, #1768]
	str	r0, [sp, #28]                   @ 4-byte Spill
	ldr	r0, [sp, #1764]
	str	r0, [sp, #24]                   @ 4-byte Spill
	ldr	r0, [sp, #1760]
	str	r0, [sp, #20]                   @ 4-byte Spill
	ldr	r0, [sp, #1756]
	str	r0, [sp, #16]                   @ 4-byte Spill
	ldr	r0, [sp, #1752]
	str	r0, [sp, #12]                   @ 4-byte Spill
	add	r0, sp, #1648
	ldm	r9, {r4, r6, r9}
	ldr	r7, [sp, #1748]
	ldr	r5, [sp, #1744]
	ldr	r8, [sp, #1740]
	ldr	r10, [sp, #1720]
	ldr	r11, [sp, #1724]
	bl	mulPv512x32
	ldr	r0, [sp, #32]                   @ 4-byte Reload
	ldr	r1, [sp, #12]                   @ 4-byte Reload
	adds	r0, r0, r10
	ldr	r0, [sp, #116]                  @ 4-byte Reload
	adcs	r0, r0, r11
	str	r0, [sp, #116]                  @ 4-byte Spill
	ldr	r0, [sp, #112]                  @ 4-byte Reload
	ldr	r11, [sp, #116]                 @ 4-byte Reload
	adcs	r0, r0, r4
	str	r0, [sp, #112]                  @ 4-byte Spill
	ldr	r0, [sp, #108]                  @ 4-byte Reload
	adcs	r0, r0, r6
	str	r0, [sp, #108]                  @ 4-byte Spill
	ldr	r0, [sp, #104]                  @ 4-byte Reload
	adcs	r0, r0, r9
	str	r0, [sp, #104]                  @ 4-byte Spill
	ldr	r0, [sp, #96]                   @ 4-byte Reload
	adcs	r0, r0, r8
	str	r0, [sp, #96]                   @ 4-byte Spill
	ldr	r0, [sp, #92]                   @ 4-byte Reload
	adcs	r0, r0, r5
	str	r0, [sp, #92]                   @ 4-byte Spill
	ldr	r0, [sp, #88]                   @ 4-byte Reload
	adcs	r0, r0, r7
	str	r0, [sp, #88]                   @ 4-byte Spill
	ldr	r0, [sp, #84]                   @ 4-byte Reload
	ldr	r7, [sp, #112]                  @ 4-byte Reload
	adcs	r0, r0, r1
	str	r0, [sp, #84]                   @ 4-byte Spill
	ldr	r0, [sp, #80]                   @ 4-byte Reload
	ldr	r1, [sp, #16]                   @ 4-byte Reload
	adcs	r0, r0, r1
	str	r0, [sp, #80]                   @ 4-byte Spill
	ldr	r0, [sp, #76]                   @ 4-byte Reload
	ldr	r1, [sp, #20]                   @ 4-byte Reload
	adcs	r0, r0, r1
	str	r0, [sp, #76]                   @ 4-byte Spill
	ldr	r0, [sp, #72]                   @ 4-byte Reload
	ldr	r1, [sp, #24]                   @ 4-byte Reload
	adcs	r0, r0, r1
	str	r0, [sp, #72]                   @ 4-byte Spill
	ldr	r0, [sp, #68]                   @ 4-byte Reload
	ldr	r1, [sp, #28]                   @ 4-byte Reload
	adcs	r0, r0, r1
	str	r0, [sp, #68]                   @ 4-byte Spill
	ldr	r0, [sp, #64]                   @ 4-byte Reload
	ldr	r1, [sp, #36]                   @ 4-byte Reload
	adcs	r0, r0, r1
	str	r0, [sp, #64]                   @ 4-byte Spill
	ldr	r0, [sp, #60]                   @ 4-byte Reload
	ldr	r1, [sp, #40]                   @ 4-byte Reload
	adcs	r0, r0, r1
	str	r0, [sp, #60]                   @ 4-byte Spill
	ldr	r0, [sp, #56]                   @ 4-byte Reload
	ldr	r1, [sp, #44]                   @ 4-byte Reload
	adcs	r0, r0, r1
	str	r0, [sp, #56]                   @ 4-byte Spill
	ldr	r0, [sp, #52]                   @ 4-byte Reload
	ldr	r1, [sp, #48]                   @ 4-byte Reload
	adc	r0, r0, r1
	str	r0, [sp, #52]                   @ 4-byte Spill
	ldr	r0, [sp, #1712]
	str	r0, [sp, #48]                   @ 4-byte Spill
	ldr	r0, [sp, #1708]
	str	r0, [sp, #44]                   @ 4-byte Spill
	ldr	r0, [sp, #1704]
	str	r0, [sp, #40]                   @ 4-byte Spill
	ldr	r0, [sp, #1700]
	str	r0, [sp, #36]                   @ 4-byte Spill
	ldr	r0, [sp, #1696]
	str	r0, [sp, #28]                   @ 4-byte Spill
	ldr	r6, [sp, #1648]
	ldr	r0, [sp, #1652]
	adds	r6, r11, r6
	ldr	r10, [sp, #1692]
	adcs	r0, r7, r0
	ldr	r9, [sp, #1688]
	ldr	r8, [sp, #1684]
	ldr	r5, [sp, #1680]
	ldr	r4, [sp, #1676]
	ldr	lr, [sp, #1672]
	ldr	r12, [sp, #1668]
	ldr	r3, [sp, #1664]
	ldr	r1, [sp, #1656]
	ldr	r2, [sp, #1660]
	str	r0, [sp, #116]                  @ 4-byte Spill
	ldr	r0, [sp, #108]                  @ 4-byte Reload
	str	r6, [sp, #32]                   @ 4-byte Spill
	adcs	r0, r0, r1
	str	r0, [sp, #112]                  @ 4-byte Spill
	ldr	r0, [sp, #104]                  @ 4-byte Reload
	ldr	r1, [sp, #28]                   @ 4-byte Reload
	adcs	r0, r0, r2
	str	r0, [sp, #108]                  @ 4-byte Spill
	ldr	r0, [sp, #96]                   @ 4-byte Reload
	adcs	r0, r0, r3
	str	r0, [sp, #104]                  @ 4-byte Spill
	ldr	r0, [sp, #92]                   @ 4-byte Reload
	adcs	r0, r0, r12
	str	r0, [sp, #96]                   @ 4-byte Spill
	ldr	r0, [sp, #88]                   @ 4-byte Reload
	adcs	r0, r0, lr
	str	r0, [sp, #92]                   @ 4-byte Spill
	ldr	r0, [sp, #84]                   @ 4-byte Reload
	add	lr, sp, #1024
	adcs	r0, r0, r4
	str	r0, [sp, #88]                   @ 4-byte Spill
	ldr	r0, [sp, #80]                   @ 4-byte Reload
	adcs	r0, r0, r5
	str	r0, [sp, #84]                   @ 4-byte Spill
	ldr	r0, [sp, #76]                   @ 4-byte Reload
	adcs	r0, r0, r8
	str	r0, [sp, #80]                   @ 4-byte Spill
	ldr	r0, [sp, #72]                   @ 4-byte Reload
	adcs	r0, r0, r9
	str	r0, [sp, #76]                   @ 4-byte Spill
	ldr	r0, [sp, #68]                   @ 4-byte Reload
	adcs	r0, r0, r10
	str	r0, [sp, #72]                   @ 4-byte Spill
	ldr	r0, [sp, #64]                   @ 4-byte Reload
	adcs	r0, r0, r1
	str	r0, [sp, #68]                   @ 4-byte Spill
	ldr	r0, [sp, #60]                   @ 4-byte Reload
	ldr	r1, [sp, #36]                   @ 4-byte Reload
	adcs	r0, r0, r1
	str	r0, [sp, #64]                   @ 4-byte Spill
	ldr	r0, [sp, #56]                   @ 4-byte Reload
	ldr	r1, [sp, #40]                   @ 4-byte Reload
	adcs	r0, r0, r1
	str	r0, [sp, #60]                   @ 4-byte Spill
	ldr	r0, [sp, #52]                   @ 4-byte Reload
	ldr	r1, [sp, #44]                   @ 4-byte Reload
	adcs	r0, r0, r1
	str	r0, [sp, #56]                   @ 4-byte Spill
	ldr	r0, [sp, #48]                   @ 4-byte Reload
	ldr	r1, [sp, #132]                  @ 4-byte Reload
	adc	r0, r0, #0
	str	r0, [sp, #52]                   @ 4-byte Spill
	ldr	r0, [sp, #120]                  @ 4-byte Reload
	mul	r2, r0, r6
	add	r0, lr, #552
	bl	mulPv512x32
	ldr	r0, [sp, #124]                  @ 4-byte Reload
	add	r9, sp, #1584
	ldr	r1, [sp, #128]                  @ 4-byte Reload
	ldr	r2, [r0, #24]
	ldr	r0, [sp, #1640]
	str	r0, [sp, #48]                   @ 4-byte Spill
	ldr	r0, [sp, #1636]
	str	r0, [sp, #44]                   @ 4-byte Spill
	ldr	r0, [sp, #1632]
	str	r0, [sp, #40]                   @ 4-byte Spill
	ldr	r0, [sp, #1628]
	str	r0, [sp, #36]                   @ 4-byte Spill
	ldr	r0, [sp, #1624]
	str	r0, [sp, #28]                   @ 4-byte Spill
	ldr	r0, [sp, #1620]
	str	r0, [sp, #24]                   @ 4-byte Spill
	ldr	r0, [sp, #1616]
	str	r0, [sp, #20]                   @ 4-byte Spill
	ldr	r0, [sp, #1612]
	str	r0, [sp, #16]                   @ 4-byte Spill
	ldr	r0, [sp, #1608]
	str	r0, [sp, #12]                   @ 4-byte Spill
	add	r0, sp, #1504
	ldm	r9, {r4, r6, r9}
	ldr	r7, [sp, #1604]
	ldr	r5, [sp, #1600]
	ldr	r8, [sp, #1596]
	ldr	r10, [sp, #1576]
	ldr	r11, [sp, #1580]
	bl	mulPv512x32
	ldr	r0, [sp, #32]                   @ 4-byte Reload
	ldr	r1, [sp, #12]                   @ 4-byte Reload
	adds	r0, r0, r10
	ldr	r0, [sp, #116]                  @ 4-byte Reload
	adcs	r0, r0, r11
	str	r0, [sp, #116]                  @ 4-byte Spill
	ldr	r0, [sp, #112]                  @ 4-byte Reload
	ldr	r11, [sp, #116]                 @ 4-byte Reload
	adcs	r0, r0, r4
	str	r0, [sp, #112]                  @ 4-byte Spill
	ldr	r0, [sp, #108]                  @ 4-byte Reload
	adcs	r0, r0, r6
	str	r0, [sp, #108]                  @ 4-byte Spill
	ldr	r0, [sp, #104]                  @ 4-byte Reload
	adcs	r0, r0, r9
	str	r0, [sp, #104]                  @ 4-byte Spill
	ldr	r0, [sp, #96]                   @ 4-byte Reload
	adcs	r0, r0, r8
	str	r0, [sp, #96]                   @ 4-byte Spill
	ldr	r0, [sp, #92]                   @ 4-byte Reload
	adcs	r0, r0, r5
	str	r0, [sp, #92]                   @ 4-byte Spill
	ldr	r0, [sp, #88]                   @ 4-byte Reload
	adcs	r0, r0, r7
	str	r0, [sp, #88]                   @ 4-byte Spill
	ldr	r0, [sp, #84]                   @ 4-byte Reload
	ldr	r7, [sp, #112]                  @ 4-byte Reload
	adcs	r0, r0, r1
	str	r0, [sp, #84]                   @ 4-byte Spill
	ldr	r0, [sp, #80]                   @ 4-byte Reload
	ldr	r1, [sp, #16]                   @ 4-byte Reload
	adcs	r0, r0, r1
	str	r0, [sp, #80]                   @ 4-byte Spill
	ldr	r0, [sp, #76]                   @ 4-byte Reload
	ldr	r1, [sp, #20]                   @ 4-byte Reload
	adcs	r0, r0, r1
	str	r0, [sp, #76]                   @ 4-byte Spill
	ldr	r0, [sp, #72]                   @ 4-byte Reload
	ldr	r1, [sp, #24]                   @ 4-byte Reload
	adcs	r0, r0, r1
	str	r0, [sp, #72]                   @ 4-byte Spill
	ldr	r0, [sp, #68]                   @ 4-byte Reload
	ldr	r1, [sp, #28]                   @ 4-byte Reload
	adcs	r0, r0, r1
	str	r0, [sp, #68]                   @ 4-byte Spill
	ldr	r0, [sp, #64]                   @ 4-byte Reload
	ldr	r1, [sp, #36]                   @ 4-byte Reload
	adcs	r0, r0, r1
	str	r0, [sp, #64]                   @ 4-byte Spill
	ldr	r0, [sp, #60]                   @ 4-byte Reload
	ldr	r1, [sp, #40]                   @ 4-byte Reload
	adcs	r0, r0, r1
	str	r0, [sp, #60]                   @ 4-byte Spill
	ldr	r0, [sp, #56]                   @ 4-byte Reload
	ldr	r1, [sp, #44]                   @ 4-byte Reload
	adcs	r0, r0, r1
	str	r0, [sp, #56]                   @ 4-byte Spill
	ldr	r0, [sp, #52]                   @ 4-byte Reload
	ldr	r1, [sp, #48]                   @ 4-byte Reload
	adc	r0, r0, r1
	str	r0, [sp, #52]                   @ 4-byte Spill
	ldr	r0, [sp, #1568]
	str	r0, [sp, #48]                   @ 4-byte Spill
	ldr	r0, [sp, #1564]
	str	r0, [sp, #44]                   @ 4-byte Spill
	ldr	r0, [sp, #1560]
	str	r0, [sp, #40]                   @ 4-byte Spill
	ldr	r0, [sp, #1556]
	str	r0, [sp, #36]                   @ 4-byte Spill
	ldr	r0, [sp, #1552]
	str	r0, [sp, #28]                   @ 4-byte Spill
	ldr	r6, [sp, #1504]
	ldr	r0, [sp, #1508]
	adds	r6, r11, r6
	ldr	r10, [sp, #1548]
	adcs	r0, r7, r0
	ldr	r9, [sp, #1544]
	ldr	r8, [sp, #1540]
	ldr	r5, [sp, #1536]
	ldr	r4, [sp, #1532]
	ldr	lr, [sp, #1528]
	ldr	r12, [sp, #1524]
	ldr	r3, [sp, #1520]
	ldr	r1, [sp, #1512]
	ldr	r2, [sp, #1516]
	str	r0, [sp, #116]                  @ 4-byte Spill
	ldr	r0, [sp, #108]                  @ 4-byte Reload
	str	r6, [sp, #32]                   @ 4-byte Spill
	adcs	r0, r0, r1
	str	r0, [sp, #112]                  @ 4-byte Spill
	ldr	r0, [sp, #104]                  @ 4-byte Reload
	ldr	r1, [sp, #28]                   @ 4-byte Reload
	adcs	r0, r0, r2
	str	r0, [sp, #108]                  @ 4-byte Spill
	ldr	r0, [sp, #96]                   @ 4-byte Reload
	adcs	r0, r0, r3
	str	r0, [sp, #104]                  @ 4-byte Spill
	ldr	r0, [sp, #92]                   @ 4-byte Reload
	adcs	r0, r0, r12
	str	r0, [sp, #96]                   @ 4-byte Spill
	ldr	r0, [sp, #88]                   @ 4-byte Reload
	adcs	r0, r0, lr
	str	r0, [sp, #92]                   @ 4-byte Spill
	ldr	r0, [sp, #84]                   @ 4-byte Reload
	add	lr, sp, #1024
	adcs	r0, r0, r4
	str	r0, [sp, #88]                   @ 4-byte Spill
	ldr	r0, [sp, #80]                   @ 4-byte Reload
	adcs	r0, r0, r5
	str	r0, [sp, #84]                   @ 4-byte Spill
	ldr	r0, [sp, #76]                   @ 4-byte Reload
	adcs	r0, r0, r8
	str	r0, [sp, #80]                   @ 4-byte Spill
	ldr	r0, [sp, #72]                   @ 4-byte Reload
	adcs	r0, r0, r9
	str	r0, [sp, #76]                   @ 4-byte Spill
	ldr	r0, [sp, #68]                   @ 4-byte Reload
	adcs	r0, r0, r10
	str	r0, [sp, #72]                   @ 4-byte Spill
	ldr	r0, [sp, #64]                   @ 4-byte Reload
	adcs	r0, r0, r1
	str	r0, [sp, #68]                   @ 4-byte Spill
	ldr	r0, [sp, #60]                   @ 4-byte Reload
	ldr	r1, [sp, #36]                   @ 4-byte Reload
	adcs	r0, r0, r1
	str	r0, [sp, #64]                   @ 4-byte Spill
	ldr	r0, [sp, #56]                   @ 4-byte Reload
	ldr	r1, [sp, #40]                   @ 4-byte Reload
	adcs	r0, r0, r1
	str	r0, [sp, #60]                   @ 4-byte Spill
	ldr	r0, [sp, #52]                   @ 4-byte Reload
	ldr	r1, [sp, #44]                   @ 4-byte Reload
	adcs	r0, r0, r1
	str	r0, [sp, #56]                   @ 4-byte Spill
	ldr	r0, [sp, #48]                   @ 4-byte Reload
	ldr	r1, [sp, #132]                  @ 4-byte Reload
	adc	r0, r0, #0
	str	r0, [sp, #52]                   @ 4-byte Spill
	ldr	r0, [sp, #120]                  @ 4-byte Reload
	mul	r2, r0, r6
	add	r0, lr, #408
	bl	mulPv512x32
	ldr	r0, [sp, #124]                  @ 4-byte Reload
	add	r9, sp, #1440
	ldr	r1, [sp, #128]                  @ 4-byte Reload
	ldr	r2, [r0, #28]
	ldr	r0, [sp, #1496]
	str	r0, [sp, #48]                   @ 4-byte Spill
	ldr	r0, [sp, #1492]
	str	r0, [sp, #44]                   @ 4-byte Spill
	ldr	r0, [sp, #1488]
	str	r0, [sp, #40]                   @ 4-byte Spill
	ldr	r0, [sp, #1484]
	str	r0, [sp, #36]                   @ 4-byte Spill
	ldr	r0, [sp, #1480]
	str	r0, [sp, #28]                   @ 4-byte Spill
	ldr	r0, [sp, #1476]
	str	r0, [sp, #24]                   @ 4-byte Spill
	ldr	r0, [sp, #1472]
	str	r0, [sp, #20]                   @ 4-byte Spill
	ldr	r0, [sp, #1468]
	str	r0, [sp, #16]                   @ 4-byte Spill
	ldr	r0, [sp, #1464]
	str	r0, [sp, #12]                   @ 4-byte Spill
	add	r0, sp, #1360
	ldm	r9, {r4, r6, r9}
	ldr	r7, [sp, #1460]
	ldr	r5, [sp, #1456]
	ldr	r8, [sp, #1452]
	ldr	r10, [sp, #1432]
	ldr	r11, [sp, #1436]
	bl	mulPv512x32
	ldr	r0, [sp, #32]                   @ 4-byte Reload
	ldr	r1, [sp, #12]                   @ 4-byte Reload
	adds	r0, r0, r10
	ldr	r0, [sp, #116]                  @ 4-byte Reload
	adcs	r0, r0, r11
	str	r0, [sp, #116]                  @ 4-byte Spill
	ldr	r0, [sp, #112]                  @ 4-byte Reload
	ldr	r11, [sp, #116]                 @ 4-byte Reload
	adcs	r0, r0, r4
	str	r0, [sp, #112]                  @ 4-byte Spill
	ldr	r0, [sp, #108]                  @ 4-byte Reload
	adcs	r0, r0, r6
	str	r0, [sp, #108]                  @ 4-byte Spill
	ldr	r0, [sp, #104]                  @ 4-byte Reload
	adcs	r0, r0, r9
	str	r0, [sp, #104]                  @ 4-byte Spill
	ldr	r0, [sp, #96]                   @ 4-byte Reload
	adcs	r0, r0, r8
	str	r0, [sp, #96]                   @ 4-byte Spill
	ldr	r0, [sp, #92]                   @ 4-byte Reload
	adcs	r0, r0, r5
	str	r0, [sp, #92]                   @ 4-byte Spill
	ldr	r0, [sp, #88]                   @ 4-byte Reload
	adcs	r0, r0, r7
	str	r0, [sp, #88]                   @ 4-byte Spill
	ldr	r0, [sp, #84]                   @ 4-byte Reload
	ldr	r7, [sp, #112]                  @ 4-byte Reload
	adcs	r0, r0, r1
	str	r0, [sp, #84]                   @ 4-byte Spill
	ldr	r0, [sp, #80]                   @ 4-byte Reload
	ldr	r1, [sp, #16]                   @ 4-byte Reload
	adcs	r0, r0, r1
	str	r0, [sp, #80]                   @ 4-byte Spill
	ldr	r0, [sp, #76]                   @ 4-byte Reload
	ldr	r1, [sp, #20]                   @ 4-byte Reload
	adcs	r0, r0, r1
	str	r0, [sp, #76]                   @ 4-byte Spill
	ldr	r0, [sp, #72]                   @ 4-byte Reload
	ldr	r1, [sp, #24]                   @ 4-byte Reload
	adcs	r0, r0, r1
	str	r0, [sp, #72]                   @ 4-byte Spill
	ldr	r0, [sp, #68]                   @ 4-byte Reload
	ldr	r1, [sp, #28]                   @ 4-byte Reload
	adcs	r0, r0, r1
	str	r0, [sp, #68]                   @ 4-byte Spill
	ldr	r0, [sp, #64]                   @ 4-byte Reload
	ldr	r1, [sp, #36]                   @ 4-byte Reload
	adcs	r0, r0, r1
	str	r0, [sp, #64]                   @ 4-byte Spill
	ldr	r0, [sp, #60]                   @ 4-byte Reload
	ldr	r1, [sp, #40]                   @ 4-byte Reload
	adcs	r0, r0, r1
	str	r0, [sp, #60]                   @ 4-byte Spill
	ldr	r0, [sp, #56]                   @ 4-byte Reload
	ldr	r1, [sp, #44]                   @ 4-byte Reload
	adcs	r0, r0, r1
	str	r0, [sp, #56]                   @ 4-byte Spill
	ldr	r0, [sp, #52]                   @ 4-byte Reload
	ldr	r1, [sp, #48]                   @ 4-byte Reload
	adc	r0, r0, r1
	str	r0, [sp, #52]                   @ 4-byte Spill
	ldr	r0, [sp, #1424]
	str	r0, [sp, #48]                   @ 4-byte Spill
	ldr	r0, [sp, #1420]
	str	r0, [sp, #44]                   @ 4-byte Spill
	ldr	r0, [sp, #1416]
	str	r0, [sp, #40]                   @ 4-byte Spill
	ldr	r0, [sp, #1412]
	str	r0, [sp, #36]                   @ 4-byte Spill
	ldr	r0, [sp, #1408]
	str	r0, [sp, #28]                   @ 4-byte Spill
	ldr	r6, [sp, #1360]
	ldr	r0, [sp, #1364]
	adds	r6, r11, r6
	ldr	r10, [sp, #1404]
	adcs	r0, r7, r0
	ldr	r9, [sp, #1400]
	ldr	r8, [sp, #1396]
	ldr	r5, [sp, #1392]
	ldr	r4, [sp, #1388]
	ldr	lr, [sp, #1384]
	ldr	r12, [sp, #1380]
	ldr	r3, [sp, #1376]
	ldr	r1, [sp, #1368]
	ldr	r2, [sp, #1372]
	str	r0, [sp, #116]                  @ 4-byte Spill
	ldr	r0, [sp, #108]                  @ 4-byte Reload
	str	r6, [sp, #32]                   @ 4-byte Spill
	adcs	r0, r0, r1
	str	r0, [sp, #112]                  @ 4-byte Spill
	ldr	r0, [sp, #104]                  @ 4-byte Reload
	ldr	r1, [sp, #28]                   @ 4-byte Reload
	adcs	r0, r0, r2
	str	r0, [sp, #108]                  @ 4-byte Spill
	ldr	r0, [sp, #96]                   @ 4-byte Reload
	adcs	r0, r0, r3
	str	r0, [sp, #104]                  @ 4-byte Spill
	ldr	r0, [sp, #92]                   @ 4-byte Reload
	adcs	r0, r0, r12
	str	r0, [sp, #96]                   @ 4-byte Spill
	ldr	r0, [sp, #88]                   @ 4-byte Reload
	adcs	r0, r0, lr
	str	r0, [sp, #92]                   @ 4-byte Spill
	ldr	r0, [sp, #84]                   @ 4-byte Reload
	add	lr, sp, #1024
	adcs	r0, r0, r4
	str	r0, [sp, #88]                   @ 4-byte Spill
	ldr	r0, [sp, #80]                   @ 4-byte Reload
	adcs	r0, r0, r5
	str	r0, [sp, #84]                   @ 4-byte Spill
	ldr	r0, [sp, #76]                   @ 4-byte Reload
	adcs	r0, r0, r8
	str	r0, [sp, #80]                   @ 4-byte Spill
	ldr	r0, [sp, #72]                   @ 4-byte Reload
	adcs	r0, r0, r9
	str	r0, [sp, #76]                   @ 4-byte Spill
	ldr	r0, [sp, #68]                   @ 4-byte Reload
	adcs	r0, r0, r10
	str	r0, [sp, #72]                   @ 4-byte Spill
	ldr	r0, [sp, #64]                   @ 4-byte Reload
	adcs	r0, r0, r1
	str	r0, [sp, #68]                   @ 4-byte Spill
	ldr	r0, [sp, #60]                   @ 4-byte Reload
	ldr	r1, [sp, #36]                   @ 4-byte Reload
	adcs	r0, r0, r1
	str	r0, [sp, #64]                   @ 4-byte Spill
	ldr	r0, [sp, #56]                   @ 4-byte Reload
	ldr	r1, [sp, #40]                   @ 4-byte Reload
	adcs	r0, r0, r1
	str	r0, [sp, #60]                   @ 4-byte Spill
	ldr	r0, [sp, #52]                   @ 4-byte Reload
	ldr	r1, [sp, #44]                   @ 4-byte Reload
	adcs	r0, r0, r1
	str	r0, [sp, #56]                   @ 4-byte Spill
	ldr	r0, [sp, #48]                   @ 4-byte Reload
	ldr	r1, [sp, #132]                  @ 4-byte Reload
	adc	r0, r0, #0
	str	r0, [sp, #52]                   @ 4-byte Spill
	ldr	r0, [sp, #120]                  @ 4-byte Reload
	mul	r2, r0, r6
	add	r0, lr, #264
	bl	mulPv512x32
	ldr	r0, [sp, #124]                  @ 4-byte Reload
	add	r9, sp, #1296
	ldr	r1, [sp, #128]                  @ 4-byte Reload
	ldr	r2, [r0, #32]
	ldr	r0, [sp, #1352]
	str	r0, [sp, #48]                   @ 4-byte Spill
	ldr	r0, [sp, #1348]
	str	r0, [sp, #44]                   @ 4-byte Spill
	ldr	r0, [sp, #1344]
	str	r0, [sp, #40]                   @ 4-byte Spill
	ldr	r0, [sp, #1340]
	str	r0, [sp, #36]                   @ 4-byte Spill
	ldr	r0, [sp, #1336]
	str	r0, [sp, #28]                   @ 4-byte Spill
	ldr	r0, [sp, #1332]
	str	r0, [sp, #24]                   @ 4-byte Spill
	ldr	r0, [sp, #1328]
	str	r0, [sp, #20]                   @ 4-byte Spill
	ldr	r0, [sp, #1324]
	str	r0, [sp, #16]                   @ 4-byte Spill
	ldr	r0, [sp, #1320]
	str	r0, [sp, #12]                   @ 4-byte Spill
	add	r0, sp, #1216
	ldm	r9, {r4, r6, r9}
	ldr	r7, [sp, #1316]
	ldr	r5, [sp, #1312]
	ldr	r8, [sp, #1308]
	ldr	r10, [sp, #1288]
	ldr	r11, [sp, #1292]
	bl	mulPv512x32
	ldr	r0, [sp, #32]                   @ 4-byte Reload
	ldr	r1, [sp, #12]                   @ 4-byte Reload
	adds	r0, r0, r10
	ldr	r0, [sp, #116]                  @ 4-byte Reload
	adcs	r0, r0, r11
	str	r0, [sp, #116]                  @ 4-byte Spill
	ldr	r0, [sp, #112]                  @ 4-byte Reload
	ldr	r11, [sp, #116]                 @ 4-byte Reload
	adcs	r0, r0, r4
	str	r0, [sp, #112]                  @ 4-byte Spill
	ldr	r0, [sp, #108]                  @ 4-byte Reload
	adcs	r0, r0, r6
	str	r0, [sp, #108]                  @ 4-byte Spill
	ldr	r0, [sp, #104]                  @ 4-byte Reload
	adcs	r0, r0, r9
	str	r0, [sp, #104]                  @ 4-byte Spill
	ldr	r0, [sp, #96]                   @ 4-byte Reload
	adcs	r0, r0, r8
	str	r0, [sp, #96]                   @ 4-byte Spill
	ldr	r0, [sp, #92]                   @ 4-byte Reload
	adcs	r0, r0, r5
	str	r0, [sp, #92]                   @ 4-byte Spill
	ldr	r0, [sp, #88]                   @ 4-byte Reload
	adcs	r0, r0, r7
	str	r0, [sp, #88]                   @ 4-byte Spill
	ldr	r0, [sp, #84]                   @ 4-byte Reload
	ldr	r7, [sp, #112]                  @ 4-byte Reload
	adcs	r0, r0, r1
	str	r0, [sp, #84]                   @ 4-byte Spill
	ldr	r0, [sp, #80]                   @ 4-byte Reload
	ldr	r1, [sp, #16]                   @ 4-byte Reload
	adcs	r0, r0, r1
	str	r0, [sp, #80]                   @ 4-byte Spill
	ldr	r0, [sp, #76]                   @ 4-byte Reload
	ldr	r1, [sp, #20]                   @ 4-byte Reload
	adcs	r0, r0, r1
	str	r0, [sp, #76]                   @ 4-byte Spill
	ldr	r0, [sp, #72]                   @ 4-byte Reload
	ldr	r1, [sp, #24]                   @ 4-byte Reload
	adcs	r0, r0, r1
	str	r0, [sp, #72]                   @ 4-byte Spill
	ldr	r0, [sp, #68]                   @ 4-byte Reload
	ldr	r1, [sp, #28]                   @ 4-byte Reload
	adcs	r0, r0, r1
	str	r0, [sp, #68]                   @ 4-byte Spill
	ldr	r0, [sp, #64]                   @ 4-byte Reload
	ldr	r1, [sp, #36]                   @ 4-byte Reload
	adcs	r0, r0, r1
	str	r0, [sp, #64]                   @ 4-byte Spill
	ldr	r0, [sp, #60]                   @ 4-byte Reload
	ldr	r1, [sp, #40]                   @ 4-byte Reload
	adcs	r0, r0, r1
	str	r0, [sp, #60]                   @ 4-byte Spill
	ldr	r0, [sp, #56]                   @ 4-byte Reload
	ldr	r1, [sp, #44]                   @ 4-byte Reload
	adcs	r0, r0, r1
	str	r0, [sp, #56]                   @ 4-byte Spill
	ldr	r0, [sp, #52]                   @ 4-byte Reload
	ldr	r1, [sp, #48]                   @ 4-byte Reload
	adc	r0, r0, r1
	str	r0, [sp, #52]                   @ 4-byte Spill
	ldr	r0, [sp, #1280]
	str	r0, [sp, #48]                   @ 4-byte Spill
	ldr	r0, [sp, #1276]
	str	r0, [sp, #44]                   @ 4-byte Spill
	ldr	r0, [sp, #1272]
	str	r0, [sp, #40]                   @ 4-byte Spill
	ldr	r0, [sp, #1268]
	str	r0, [sp, #36]                   @ 4-byte Spill
	ldr	r0, [sp, #1264]
	str	r0, [sp, #28]                   @ 4-byte Spill
	ldr	r6, [sp, #1216]
	ldr	r0, [sp, #1220]
	adds	r6, r11, r6
	ldr	r10, [sp, #1260]
	adcs	r0, r7, r0
	ldr	r9, [sp, #1256]
	ldr	r8, [sp, #1252]
	ldr	r5, [sp, #1248]
	ldr	r4, [sp, #1244]
	ldr	lr, [sp, #1240]
	ldr	r12, [sp, #1236]
	ldr	r3, [sp, #1232]
	ldr	r1, [sp, #1224]
	ldr	r2, [sp, #1228]
	str	r0, [sp, #116]                  @ 4-byte Spill
	ldr	r0, [sp, #108]                  @ 4-byte Reload
	ldr	r11, [sp, #120]                 @ 4-byte Reload
	adcs	r0, r0, r1
	str	r0, [sp, #112]                  @ 4-byte Spill
	ldr	r0, [sp, #104]                  @ 4-byte Reload
	ldr	r1, [sp, #28]                   @ 4-byte Reload
	adcs	r0, r0, r2
	str	r0, [sp, #108]                  @ 4-byte Spill
	ldr	r0, [sp, #96]                   @ 4-byte Reload
	mul	r2, r11, r6
	str	r6, [sp, #32]                   @ 4-byte Spill
	adcs	r0, r0, r3
	str	r0, [sp, #104]                  @ 4-byte Spill
	ldr	r0, [sp, #92]                   @ 4-byte Reload
	adcs	r0, r0, r12
	str	r0, [sp, #96]                   @ 4-byte Spill
	ldr	r0, [sp, #88]                   @ 4-byte Reload
	adcs	r0, r0, lr
	str	r0, [sp, #92]                   @ 4-byte Spill
	ldr	r0, [sp, #84]                   @ 4-byte Reload
	add	lr, sp, #1024
	adcs	r0, r0, r4
	str	r0, [sp, #88]                   @ 4-byte Spill
	ldr	r0, [sp, #80]                   @ 4-byte Reload
	adcs	r0, r0, r5
	str	r0, [sp, #84]                   @ 4-byte Spill
	ldr	r0, [sp, #76]                   @ 4-byte Reload
	adcs	r0, r0, r8
	str	r0, [sp, #80]                   @ 4-byte Spill
	ldr	r0, [sp, #72]                   @ 4-byte Reload
	adcs	r0, r0, r9
	str	r0, [sp, #76]                   @ 4-byte Spill
	ldr	r0, [sp, #68]                   @ 4-byte Reload
	adcs	r0, r0, r10
	str	r0, [sp, #72]                   @ 4-byte Spill
	ldr	r0, [sp, #64]                   @ 4-byte Reload
	adcs	r0, r0, r1
	str	r0, [sp, #68]                   @ 4-byte Spill
	ldr	r0, [sp, #60]                   @ 4-byte Reload
	ldr	r1, [sp, #36]                   @ 4-byte Reload
	adcs	r0, r0, r1
	str	r0, [sp, #64]                   @ 4-byte Spill
	ldr	r0, [sp, #56]                   @ 4-byte Reload
	ldr	r1, [sp, #40]                   @ 4-byte Reload
	adcs	r0, r0, r1
	str	r0, [sp, #60]                   @ 4-byte Spill
	ldr	r0, [sp, #52]                   @ 4-byte Reload
	ldr	r1, [sp, #44]                   @ 4-byte Reload
	adcs	r0, r0, r1
	str	r0, [sp, #56]                   @ 4-byte Spill
	ldr	r0, [sp, #48]                   @ 4-byte Reload
	ldr	r1, [sp, #132]                  @ 4-byte Reload
	adc	r0, r0, #0
	str	r0, [sp, #52]                   @ 4-byte Spill
	add	r0, lr, #120
	bl	mulPv512x32
	ldr	r0, [sp, #124]                  @ 4-byte Reload
	ldr	r1, [sp, #128]                  @ 4-byte Reload
	ldr	r2, [r0, #36]
	ldr	r0, [sp, #1208]
	str	r0, [sp, #48]                   @ 4-byte Spill
	ldr	r0, [sp, #1204]
	str	r0, [sp, #44]                   @ 4-byte Spill
	ldr	r0, [sp, #1200]
	str	r0, [sp, #40]                   @ 4-byte Spill
	ldr	r0, [sp, #1196]
	str	r0, [sp, #36]                   @ 4-byte Spill
	ldr	r0, [sp, #1192]
	str	r0, [sp, #28]                   @ 4-byte Spill
	ldr	r0, [sp, #1188]
	str	r0, [sp, #24]                   @ 4-byte Spill
	ldr	r0, [sp, #1184]
	str	r0, [sp, #20]                   @ 4-byte Spill
	ldr	r0, [sp, #1180]
	str	r0, [sp, #16]                   @ 4-byte Spill
	ldr	r0, [sp, #1176]
	str	r0, [sp, #12]                   @ 4-byte Spill
	ldr	r0, [sp, #1172]
	str	r0, [sp, #8]                    @ 4-byte Spill
	add	r0, sp, #1072
	ldr	r4, [sp, #1168]
	ldr	r5, [sp, #1164]
	ldr	r6, [sp, #1160]
	ldr	r7, [sp, #1144]
	ldr	r8, [sp, #1148]
	ldr	r9, [sp, #1152]
	ldr	r10, [sp, #1156]
	bl	mulPv512x32
	ldr	r0, [sp, #32]                   @ 4-byte Reload
	add	lr, sp, #1088
	ldr	r1, [sp, #8]                    @ 4-byte Reload
	adds	r0, r0, r7
	ldr	r0, [sp, #116]                  @ 4-byte Reload
	adcs	r2, r0, r8
	ldr	r0, [sp, #112]                  @ 4-byte Reload
	adcs	r0, r0, r9
	str	r0, [sp, #112]                  @ 4-byte Spill
	ldr	r0, [sp, #108]                  @ 4-byte Reload
	adcs	r0, r0, r10
	str	r0, [sp, #108]                  @ 4-byte Spill
	ldr	r0, [sp, #104]                  @ 4-byte Reload
	adcs	r0, r0, r6
	str	r0, [sp, #104]                  @ 4-byte Spill
	ldr	r0, [sp, #96]                   @ 4-byte Reload
	adcs	r0, r0, r5
	str	r0, [sp, #96]                   @ 4-byte Spill
	ldr	r0, [sp, #92]                   @ 4-byte Reload
	adcs	r0, r0, r4
	str	r0, [sp, #92]                   @ 4-byte Spill
	ldr	r0, [sp, #88]                   @ 4-byte Reload
	adcs	r0, r0, r1
	str	r0, [sp, #88]                   @ 4-byte Spill
	ldr	r0, [sp, #84]                   @ 4-byte Reload
	ldr	r1, [sp, #12]                   @ 4-byte Reload
	adcs	r0, r0, r1
	str	r0, [sp, #84]                   @ 4-byte Spill
	ldr	r0, [sp, #80]                   @ 4-byte Reload
	ldr	r1, [sp, #16]                   @ 4-byte Reload
	adcs	r0, r0, r1
	str	r0, [sp, #80]                   @ 4-byte Spill
	ldr	r0, [sp, #76]                   @ 4-byte Reload
	ldr	r1, [sp, #20]                   @ 4-byte Reload
	adcs	r0, r0, r1
	str	r0, [sp, #76]                   @ 4-byte Spill
	ldr	r0, [sp, #72]                   @ 4-byte Reload
	ldr	r1, [sp, #24]                   @ 4-byte Reload
	adcs	r0, r0, r1
	str	r0, [sp, #72]                   @ 4-byte Spill
	ldr	r0, [sp, #68]                   @ 4-byte Reload
	ldr	r1, [sp, #28]                   @ 4-byte Reload
	adcs	r0, r0, r1
	str	r0, [sp, #68]                   @ 4-byte Spill
	ldr	r0, [sp, #64]                   @ 4-byte Reload
	ldr	r1, [sp, #36]                   @ 4-byte Reload
	adcs	r0, r0, r1
	str	r0, [sp, #64]                   @ 4-byte Spill
	ldr	r0, [sp, #60]                   @ 4-byte Reload
	ldr	r1, [sp, #40]                   @ 4-byte Reload
	adcs	r0, r0, r1
	str	r0, [sp, #60]                   @ 4-byte Spill
	ldr	r0, [sp, #56]                   @ 4-byte Reload
	ldr	r1, [sp, #44]                   @ 4-byte Reload
	adcs	r0, r0, r1
	str	r0, [sp, #56]                   @ 4-byte Spill
	ldr	r0, [sp, #52]                   @ 4-byte Reload
	ldr	r1, [sp, #48]                   @ 4-byte Reload
	adc	r0, r0, r1
	str	r0, [sp, #52]                   @ 4-byte Spill
	ldr	r4, [sp, #1072]
	ldr	r6, [sp, #1076]
	adds	r0, r2, r4
	ldr	r5, [sp, #1080]
	ldr	r7, [sp, #1084]
	mul	r1, r11, r0
	str	r0, [sp, #116]                  @ 4-byte Spill
	ldr	r11, [sp, #112]                 @ 4-byte Reload
	adcs	r6, r11, r6
	str	r1, [sp, #44]                   @ 4-byte Spill
	ldr	r0, [sp, #1136]
	str	r0, [sp, #48]                   @ 4-byte Spill
	ldr	r0, [sp, #1132]
	str	r0, [sp, #40]                   @ 4-byte Spill
	ldr	r0, [sp, #1128]
	str	r0, [sp, #36]                   @ 4-byte Spill
	ldr	r10, [sp, #1124]
	ldr	r9, [sp, #1120]
	ldr	r8, [sp, #1116]
	ldr	r4, [sp, #1112]
	ldm	lr, {r0, r1, r2, r3, r12, lr}
	str	r6, [sp, #112]                  @ 4-byte Spill
	ldr	r6, [sp, #108]                  @ 4-byte Reload
	adcs	r5, r6, r5
	str	r5, [sp, #108]                  @ 4-byte Spill
	ldr	r5, [sp, #104]                  @ 4-byte Reload
	adcs	r5, r5, r7
	str	r5, [sp, #104]                  @ 4-byte Spill
	ldr	r5, [sp, #96]                   @ 4-byte Reload
	adcs	r0, r5, r0
	str	r0, [sp, #96]                   @ 4-byte Spill
	ldr	r0, [sp, #92]                   @ 4-byte Reload
	adcs	r0, r0, r1
	str	r0, [sp, #92]                   @ 4-byte Spill
	ldr	r0, [sp, #88]                   @ 4-byte Reload
	ldr	r1, [sp, #36]                   @ 4-byte Reload
	adcs	r0, r0, r2
	str	r0, [sp, #88]                   @ 4-byte Spill
	ldr	r0, [sp, #84]                   @ 4-byte Reload
	ldr	r2, [sp, #44]                   @ 4-byte Reload
	adcs	r0, r0, r3
	str	r0, [sp, #84]                   @ 4-byte Spill
	ldr	r0, [sp, #80]                   @ 4-byte Reload
	adcs	r0, r0, r12
	str	r0, [sp, #80]                   @ 4-byte Spill
	ldr	r0, [sp, #76]                   @ 4-byte Reload
	adcs	r0, r0, lr
	str	r0, [sp, #76]                   @ 4-byte Spill
	ldr	r0, [sp, #72]                   @ 4-byte Reload
	adcs	r0, r0, r4
	str	r0, [sp, #72]                   @ 4-byte Spill
	ldr	r0, [sp, #68]                   @ 4-byte Reload
	adcs	r0, r0, r8
	str	r0, [sp, #68]                   @ 4-byte Spill
	ldr	r0, [sp, #64]                   @ 4-byte Reload
	adcs	r0, r0, r9
	str	r0, [sp, #64]                   @ 4-byte Spill
	ldr	r0, [sp, #60]                   @ 4-byte Reload
	adcs	r0, r0, r10
	str	r0, [sp, #60]                   @ 4-byte Spill
	ldr	r0, [sp, #56]                   @ 4-byte Reload
	adcs	r0, r0, r1
	str	r0, [sp, #56]                   @ 4-byte Spill
	ldr	r0, [sp, #52]                   @ 4-byte Reload
	ldr	r1, [sp, #40]                   @ 4-byte Reload
	adcs	r0, r0, r1
	str	r0, [sp, #52]                   @ 4-byte Spill
	ldr	r0, [sp, #48]                   @ 4-byte Reload
	ldr	r1, [sp, #132]                  @ 4-byte Reload
	adc	r0, r0, #0
	str	r0, [sp, #48]                   @ 4-byte Spill
	add	r0, sp, #1000
	bl	mulPv512x32
	ldr	r0, [sp, #124]                  @ 4-byte Reload
	add	r10, sp, #1000
	ldr	r1, [sp, #128]                  @ 4-byte Reload
	ldr	r2, [r0, #40]
	ldr	r0, [sp, #1064]
	str	r0, [sp, #44]                   @ 4-byte Spill
	ldr	r0, [sp, #1060]
	str	r0, [sp, #40]                   @ 4-byte Spill
	ldr	r0, [sp, #1056]
	str	r0, [sp, #36]                   @ 4-byte Spill
	ldr	r0, [sp, #1052]
	str	r0, [sp, #24]                   @ 4-byte Spill
	ldr	r0, [sp, #1048]
	str	r0, [sp, #28]                   @ 4-byte Spill
	ldr	r0, [sp, #1044]
	str	r0, [sp, #32]                   @ 4-byte Spill
	ldr	r0, [sp, #1040]
	str	r0, [sp, #20]                   @ 4-byte Spill
	ldr	r0, [sp, #1036]
	str	r0, [sp, #16]                   @ 4-byte Spill
	ldr	r0, [sp, #1032]
	str	r0, [sp, #12]                   @ 4-byte Spill
	add	r0, sp, #928
	ldm	r10, {r7, r8, r9, r10}
	ldr	r11, [sp, #1028]
	ldr	r4, [sp, #1024]
	ldr	r5, [sp, #1020]
	ldr	r6, [sp, #1016]
	bl	mulPv512x32
	ldr	r0, [sp, #116]                  @ 4-byte Reload
	add	lr, sp, #944
	ldr	r1, [sp, #12]                   @ 4-byte Reload
	adds	r0, r0, r7
	ldr	r0, [sp, #112]                  @ 4-byte Reload
	adcs	r2, r0, r8
	ldr	r0, [sp, #108]                  @ 4-byte Reload
	adcs	r0, r0, r9
	str	r0, [sp, #112]                  @ 4-byte Spill
	ldr	r0, [sp, #104]                  @ 4-byte Reload
	adcs	r0, r0, r10
	str	r0, [sp, #108]                  @ 4-byte Spill
	ldr	r0, [sp, #96]                   @ 4-byte Reload
	add	r10, sp, #968
	adcs	r0, r0, r6
	str	r0, [sp, #104]                  @ 4-byte Spill
	ldr	r0, [sp, #92]                   @ 4-byte Reload
	adcs	r0, r0, r5
	str	r0, [sp, #96]                   @ 4-byte Spill
	ldr	r0, [sp, #88]                   @ 4-byte Reload
	adcs	r0, r0, r4
	str	r0, [sp, #92]                   @ 4-byte Spill
	ldr	r0, [sp, #84]                   @ 4-byte Reload
	adcs	r0, r0, r11
	str	r0, [sp, #88]                   @ 4-byte Spill
	ldr	r0, [sp, #80]                   @ 4-byte Reload
	ldr	r11, [sp, #112]                 @ 4-byte Reload
	adcs	r0, r0, r1
	str	r0, [sp, #84]                   @ 4-byte Spill
	ldr	r0, [sp, #76]                   @ 4-byte Reload
	ldr	r1, [sp, #16]                   @ 4-byte Reload
	adcs	r0, r0, r1
	str	r0, [sp, #80]                   @ 4-byte Spill
	ldr	r0, [sp, #72]                   @ 4-byte Reload
	ldr	r1, [sp, #20]                   @ 4-byte Reload
	adcs	r0, r0, r1
	str	r0, [sp, #76]                   @ 4-byte Spill
	ldr	r0, [sp, #68]                   @ 4-byte Reload
	ldr	r1, [sp, #32]                   @ 4-byte Reload
	adcs	r0, r0, r1
	str	r0, [sp, #32]                   @ 4-byte Spill
	ldr	r0, [sp, #64]                   @ 4-byte Reload
	ldr	r1, [sp, #28]                   @ 4-byte Reload
	adcs	r0, r0, r1
	str	r0, [sp, #28]                   @ 4-byte Spill
	ldr	r0, [sp, #60]                   @ 4-byte Reload
	ldr	r1, [sp, #24]                   @ 4-byte Reload
	adcs	r0, r0, r1
	str	r0, [sp, #24]                   @ 4-byte Spill
	ldr	r0, [sp, #56]                   @ 4-byte Reload
	ldr	r1, [sp, #36]                   @ 4-byte Reload
	adcs	r0, r0, r1
	str	r0, [sp, #36]                   @ 4-byte Spill
	ldr	r0, [sp, #52]                   @ 4-byte Reload
	ldr	r1, [sp, #40]                   @ 4-byte Reload
	adcs	r0, r0, r1
	str	r0, [sp, #40]                   @ 4-byte Spill
	ldr	r0, [sp, #48]                   @ 4-byte Reload
	ldr	r1, [sp, #44]                   @ 4-byte Reload
	adc	r0, r0, r1
	str	r0, [sp, #44]                   @ 4-byte Spill
	ldr	r4, [sp, #928]
	ldr	r0, [sp, #120]                  @ 4-byte Reload
	adds	r1, r2, r4
	ldr	r6, [sp, #932]
	ldr	r5, [sp, #936]
	mul	r2, r0, r1
	ldr	r7, [sp, #940]
	str	r1, [sp, #116]                  @ 4-byte Spill
	adcs	r6, r11, r6
	str	r2, [sp, #20]                   @ 4-byte Spill
	ldr	r0, [sp, #992]
	str	r0, [sp, #16]                   @ 4-byte Spill
	ldr	r0, [sp, #988]
	str	r0, [sp, #12]                   @ 4-byte Spill
	ldr	r0, [sp, #984]
	str	r0, [sp, #8]                    @ 4-byte Spill
	ldm	r10, {r4, r8, r9, r10}
	ldm	lr, {r0, r1, r2, r3, r12, lr}
	str	r6, [sp, #72]                   @ 4-byte Spill
	ldr	r6, [sp, #108]                  @ 4-byte Reload
	adcs	r5, r6, r5
	str	r5, [sp, #68]                   @ 4-byte Spill
	ldr	r5, [sp, #104]                  @ 4-byte Reload
	adcs	r5, r5, r7
	str	r5, [sp, #64]                   @ 4-byte Spill
	ldr	r5, [sp, #96]                   @ 4-byte Reload
	adcs	r0, r5, r0
	str	r0, [sp, #60]                   @ 4-byte Spill
	ldr	r0, [sp, #92]                   @ 4-byte Reload
	adcs	r0, r0, r1
	str	r0, [sp, #56]                   @ 4-byte Spill
	ldr	r0, [sp, #88]                   @ 4-byte Reload
	ldr	r1, [sp, #8]                    @ 4-byte Reload
	adcs	r0, r0, r2
	str	r0, [sp, #52]                   @ 4-byte Spill
	ldr	r0, [sp, #84]                   @ 4-byte Reload
	ldr	r2, [sp, #20]                   @ 4-byte Reload
	adcs	r0, r0, r3
	str	r0, [sp, #48]                   @ 4-byte Spill
	ldr	r0, [sp, #80]                   @ 4-byte Reload
	adcs	r0, r0, r12
	str	r0, [sp, #112]                  @ 4-byte Spill
	ldr	r0, [sp, #76]                   @ 4-byte Reload
	adcs	r0, r0, lr
	str	r0, [sp, #108]                  @ 4-byte Spill
	ldr	r0, [sp, #32]                   @ 4-byte Reload
	adcs	r0, r0, r4
	str	r0, [sp, #104]                  @ 4-byte Spill
	ldr	r0, [sp, #28]                   @ 4-byte Reload
	adcs	r0, r0, r8
	str	r0, [sp, #96]                   @ 4-byte Spill
	ldr	r0, [sp, #24]                   @ 4-byte Reload
	adcs	r0, r0, r9
	str	r0, [sp, #92]                   @ 4-byte Spill
	ldr	r0, [sp, #36]                   @ 4-byte Reload
	adcs	r0, r0, r10
	str	r0, [sp, #88]                   @ 4-byte Spill
	ldr	r0, [sp, #40]                   @ 4-byte Reload
	adcs	r0, r0, r1
	str	r0, [sp, #84]                   @ 4-byte Spill
	ldr	r0, [sp, #44]                   @ 4-byte Reload
	ldr	r1, [sp, #12]                   @ 4-byte Reload
	adcs	r0, r0, r1
	str	r0, [sp, #80]                   @ 4-byte Spill
	ldr	r0, [sp, #16]                   @ 4-byte Reload
	ldr	r1, [sp, #132]                  @ 4-byte Reload
	adc	r0, r0, #0
	str	r0, [sp, #76]                   @ 4-byte Spill
	add	r0, sp, #856
	bl	mulPv512x32
	ldr	r1, [sp, #124]                  @ 4-byte Reload
	add	r7, sp, #876
	add	r11, sp, #856
	add	r0, sp, #784
	ldr	r2, [r1, #44]
	ldr	r1, [sp, #920]
	str	r1, [sp, #44]                   @ 4-byte Spill
	ldr	r1, [sp, #916]
	str	r1, [sp, #40]                   @ 4-byte Spill
	ldr	r1, [sp, #912]
	str	r1, [sp, #36]                   @ 4-byte Spill
	ldr	r1, [sp, #908]
	str	r1, [sp, #32]                   @ 4-byte Spill
	ldr	r1, [sp, #904]
	str	r1, [sp, #28]                   @ 4-byte Spill
	ldr	r1, [sp, #900]
	str	r1, [sp, #24]                   @ 4-byte Spill
	ldr	r1, [sp, #896]
	str	r1, [sp, #20]                   @ 4-byte Spill
	ldr	r1, [sp, #892]
	str	r1, [sp, #16]                   @ 4-byte Spill
	ldr	r1, [sp, #888]
	str	r1, [sp, #12]                   @ 4-byte Spill
	ldr	r1, [sp, #128]                  @ 4-byte Reload
	ldm	r7, {r4, r6, r7}
	ldm	r11, {r8, r9, r11}
	ldr	r10, [sp, #872]
	ldr	r5, [sp, #868]
	bl	mulPv512x32
	ldr	r0, [sp, #116]                  @ 4-byte Reload
	add	lr, sp, #788
	ldr	r1, [sp, #12]                   @ 4-byte Reload
	adds	r0, r0, r8
	ldr	r0, [sp, #72]                   @ 4-byte Reload
	adcs	r0, r0, r9
	str	r0, [sp, #116]                  @ 4-byte Spill
	ldr	r0, [sp, #68]                   @ 4-byte Reload
	adcs	r0, r0, r11
	str	r0, [sp, #72]                   @ 4-byte Spill
	ldr	r0, [sp, #64]                   @ 4-byte Reload
	ldr	r11, [sp, #116]                 @ 4-byte Reload
	adcs	r0, r0, r5
	str	r0, [sp, #68]                   @ 4-byte Spill
	ldr	r0, [sp, #60]                   @ 4-byte Reload
	adcs	r0, r0, r10
	str	r0, [sp, #64]                   @ 4-byte Spill
	ldr	r0, [sp, #56]                   @ 4-byte Reload
	add	r10, sp, #812
	adcs	r0, r0, r4
	str	r0, [sp, #60]                   @ 4-byte Spill
	ldr	r0, [sp, #52]                   @ 4-byte Reload
	adcs	r0, r0, r6
	str	r0, [sp, #56]                   @ 4-byte Spill
	ldr	r0, [sp, #48]                   @ 4-byte Reload
	adcs	r0, r0, r7
	str	r0, [sp, #52]                   @ 4-byte Spill
	ldr	r0, [sp, #112]                  @ 4-byte Reload
	ldr	r7, [sp, #72]                   @ 4-byte Reload
	adcs	r0, r0, r1
	str	r0, [sp, #112]                  @ 4-byte Spill
	ldr	r0, [sp, #108]                  @ 4-byte Reload
	ldr	r1, [sp, #16]                   @ 4-byte Reload
	adcs	r0, r0, r1
	str	r0, [sp, #108]                  @ 4-byte Spill
	ldr	r0, [sp, #104]                  @ 4-byte Reload
	ldr	r1, [sp, #20]                   @ 4-byte Reload
	adcs	r0, r0, r1
	str	r0, [sp, #104]                  @ 4-byte Spill
	ldr	r0, [sp, #96]                   @ 4-byte Reload
	ldr	r1, [sp, #24]                   @ 4-byte Reload
	adcs	r0, r0, r1
	str	r0, [sp, #96]                   @ 4-byte Spill
	ldr	r0, [sp, #92]                   @ 4-byte Reload
	ldr	r1, [sp, #28]                   @ 4-byte Reload
	adcs	r0, r0, r1
	str	r0, [sp, #92]                   @ 4-byte Spill
	ldr	r0, [sp, #88]                   @ 4-byte Reload
	ldr	r1, [sp, #32]                   @ 4-byte Reload
	adcs	r0, r0, r1
	str	r0, [sp, #88]                   @ 4-byte Spill
	ldr	r0, [sp, #84]                   @ 4-byte Reload
	ldr	r1, [sp, #36]                   @ 4-byte Reload
	adcs	r0, r0, r1
	str	r0, [sp, #84]                   @ 4-byte Spill
	ldr	r0, [sp, #80]                   @ 4-byte Reload
	ldr	r1, [sp, #40]                   @ 4-byte Reload
	adcs	r0, r0, r1
	str	r0, [sp, #80]                   @ 4-byte Spill
	ldr	r0, [sp, #76]                   @ 4-byte Reload
	ldr	r1, [sp, #44]                   @ 4-byte Reload
	adc	r0, r0, r1
	str	r0, [sp, #48]                   @ 4-byte Spill
	ldr	r0, [sp, #848]
	str	r0, [sp, #44]                   @ 4-byte Spill
	ldr	r0, [sp, #844]
	str	r0, [sp, #40]                   @ 4-byte Spill
	ldr	r0, [sp, #840]
	str	r0, [sp, #36]                   @ 4-byte Spill
	ldr	r0, [sp, #836]
	str	r0, [sp, #32]                   @ 4-byte Spill
	ldr	r0, [sp, #832]
	str	r0, [sp, #24]                   @ 4-byte Spill
	ldr	r6, [sp, #784]
	ldm	lr, {r0, r1, r2, r3, r12, lr}
	adds	r6, r11, r6
	adcs	r0, r7, r0
	ldm	r10, {r4, r5, r8, r9, r10}
	str	r0, [sp, #76]                   @ 4-byte Spill
	ldr	r0, [sp, #68]                   @ 4-byte Reload
	str	r6, [sp, #28]                   @ 4-byte Spill
	adcs	r0, r0, r1
	str	r0, [sp, #72]                   @ 4-byte Spill
	ldr	r0, [sp, #64]                   @ 4-byte Reload
	ldr	r1, [sp, #24]                   @ 4-byte Reload
	adcs	r0, r0, r2
	str	r0, [sp, #68]                   @ 4-byte Spill
	ldr	r0, [sp, #60]                   @ 4-byte Reload
	adcs	r0, r0, r3
	str	r0, [sp, #64]                   @ 4-byte Spill
	ldr	r0, [sp, #56]                   @ 4-byte Reload
	adcs	r0, r0, r12
	str	r0, [sp, #60]                   @ 4-byte Spill
	ldr	r0, [sp, #52]                   @ 4-byte Reload
	adcs	r0, r0, lr
	str	r0, [sp, #56]                   @ 4-byte Spill
	ldr	r0, [sp, #112]                  @ 4-byte Reload
	adcs	r0, r0, r4
	str	r0, [sp, #52]                   @ 4-byte Spill
	ldr	r0, [sp, #108]                  @ 4-byte Reload
	adcs	r0, r0, r5
	str	r0, [sp, #116]                  @ 4-byte Spill
	ldr	r0, [sp, #104]                  @ 4-byte Reload
	adcs	r0, r0, r8
	str	r0, [sp, #112]                  @ 4-byte Spill
	ldr	r0, [sp, #96]                   @ 4-byte Reload
	adcs	r0, r0, r9
	str	r0, [sp, #108]                  @ 4-byte Spill
	ldr	r0, [sp, #92]                   @ 4-byte Reload
	adcs	r0, r0, r10
	str	r0, [sp, #104]                  @ 4-byte Spill
	ldr	r0, [sp, #88]                   @ 4-byte Reload
	adcs	r0, r0, r1
	str	r0, [sp, #96]                   @ 4-byte Spill
	ldr	r0, [sp, #84]                   @ 4-byte Reload
	ldr	r1, [sp, #32]                   @ 4-byte Reload
	adcs	r0, r0, r1
	str	r0, [sp, #92]                   @ 4-byte Spill
	ldr	r0, [sp, #80]                   @ 4-byte Reload
	ldr	r1, [sp, #36]                   @ 4-byte Reload
	adcs	r0, r0, r1
	str	r0, [sp, #88]                   @ 4-byte Spill
	ldr	r0, [sp, #48]                   @ 4-byte Reload
	ldr	r1, [sp, #40]                   @ 4-byte Reload
	adcs	r0, r0, r1
	str	r0, [sp, #84]                   @ 4-byte Spill
	ldr	r0, [sp, #44]                   @ 4-byte Reload
	ldr	r1, [sp, #132]                  @ 4-byte Reload
	adc	r0, r0, #0
	str	r0, [sp, #80]                   @ 4-byte Spill
	ldr	r0, [sp, #120]                  @ 4-byte Reload
	mul	r2, r0, r6
	add	r0, sp, #712
	bl	mulPv512x32
	ldr	r1, [sp, #124]                  @ 4-byte Reload
	add	r11, sp, #720
	add	r0, sp, #640
	ldr	r2, [r1, #48]
	ldr	r1, [sp, #776]
	str	r1, [sp, #48]                   @ 4-byte Spill
	ldr	r1, [sp, #772]
	str	r1, [sp, #44]                   @ 4-byte Spill
	ldr	r1, [sp, #768]
	str	r1, [sp, #40]                   @ 4-byte Spill
	ldr	r1, [sp, #764]
	str	r1, [sp, #36]                   @ 4-byte Spill
	ldr	r1, [sp, #760]
	str	r1, [sp, #32]                   @ 4-byte Spill
	ldr	r1, [sp, #756]
	str	r1, [sp, #24]                   @ 4-byte Spill
	ldr	r1, [sp, #752]
	str	r1, [sp, #20]                   @ 4-byte Spill
	ldr	r1, [sp, #748]
	str	r1, [sp, #16]                   @ 4-byte Spill
	ldr	r1, [sp, #744]
	str	r1, [sp, #12]                   @ 4-byte Spill
	ldr	r1, [sp, #128]                  @ 4-byte Reload
	ldm	r11, {r5, r6, r11}
	ldr	r7, [sp, #740]
	ldr	r4, [sp, #736]
	ldr	r9, [sp, #732]
	ldr	r10, [sp, #712]
	ldr	r8, [sp, #716]
	bl	mulPv512x32
	ldr	r0, [sp, #28]                   @ 4-byte Reload
	add	lr, sp, #644
	ldr	r1, [sp, #12]                   @ 4-byte Reload
	adds	r0, r0, r10
	add	r10, sp, #668
	ldr	r0, [sp, #76]                   @ 4-byte Reload
	adcs	r0, r0, r8
	str	r0, [sp, #76]                   @ 4-byte Spill
	ldr	r0, [sp, #72]                   @ 4-byte Reload
	adcs	r0, r0, r5
	str	r0, [sp, #72]                   @ 4-byte Spill
	ldr	r0, [sp, #68]                   @ 4-byte Reload
	adcs	r0, r0, r6
	str	r0, [sp, #68]                   @ 4-byte Spill
	ldr	r0, [sp, #64]                   @ 4-byte Reload
	adcs	r0, r0, r11
	str	r0, [sp, #64]                   @ 4-byte Spill
	ldr	r0, [sp, #60]                   @ 4-byte Reload
	ldr	r11, [sp, #76]                  @ 4-byte Reload
	adcs	r0, r0, r9
	str	r0, [sp, #60]                   @ 4-byte Spill
	ldr	r0, [sp, #56]                   @ 4-byte Reload
	adcs	r0, r0, r4
	str	r0, [sp, #56]                   @ 4-byte Spill
	ldr	r0, [sp, #52]                   @ 4-byte Reload
	adcs	r0, r0, r7
	str	r0, [sp, #52]                   @ 4-byte Spill
	ldr	r0, [sp, #116]                  @ 4-byte Reload
	ldr	r7, [sp, #72]                   @ 4-byte Reload
	adcs	r0, r0, r1
	str	r0, [sp, #116]                  @ 4-byte Spill
	ldr	r0, [sp, #112]                  @ 4-byte Reload
	ldr	r1, [sp, #16]                   @ 4-byte Reload
	adcs	r0, r0, r1
	str	r0, [sp, #112]                  @ 4-byte Spill
	ldr	r0, [sp, #108]                  @ 4-byte Reload
	ldr	r1, [sp, #20]                   @ 4-byte Reload
	adcs	r0, r0, r1
	str	r0, [sp, #108]                  @ 4-byte Spill
	ldr	r0, [sp, #104]                  @ 4-byte Reload
	ldr	r1, [sp, #24]                   @ 4-byte Reload
	adcs	r0, r0, r1
	str	r0, [sp, #104]                  @ 4-byte Spill
	ldr	r0, [sp, #96]                   @ 4-byte Reload
	ldr	r1, [sp, #32]                   @ 4-byte Reload
	adcs	r0, r0, r1
	str	r0, [sp, #96]                   @ 4-byte Spill
	ldr	r0, [sp, #92]                   @ 4-byte Reload
	ldr	r1, [sp, #36]                   @ 4-byte Reload
	adcs	r0, r0, r1
	str	r0, [sp, #92]                   @ 4-byte Spill
	ldr	r0, [sp, #88]                   @ 4-byte Reload
	ldr	r1, [sp, #40]                   @ 4-byte Reload
	adcs	r0, r0, r1
	str	r0, [sp, #88]                   @ 4-byte Spill
	ldr	r0, [sp, #84]                   @ 4-byte Reload
	ldr	r1, [sp, #44]                   @ 4-byte Reload
	adcs	r0, r0, r1
	str	r0, [sp, #84]                   @ 4-byte Spill
	ldr	r0, [sp, #80]                   @ 4-byte Reload
	ldr	r1, [sp, #48]                   @ 4-byte Reload
	adc	r0, r0, r1
	str	r0, [sp, #80]                   @ 4-byte Spill
	ldr	r0, [sp, #704]
	str	r0, [sp, #48]                   @ 4-byte Spill
	ldr	r0, [sp, #700]
	str	r0, [sp, #44]                   @ 4-byte Spill
	ldr	r0, [sp, #696]
	str	r0, [sp, #40]                   @ 4-byte Spill
	ldr	r0, [sp, #692]
	str	r0, [sp, #36]                   @ 4-byte Spill
	ldr	r0, [sp, #688]
	str	r0, [sp, #32]                   @ 4-byte Spill
	ldr	r6, [sp, #640]
	ldm	lr, {r0, r1, r2, r3, r12, lr}
	adds	r6, r11, r6
	adcs	r0, r7, r0
	ldm	r10, {r4, r5, r8, r9, r10}
	str	r0, [sp, #76]                   @ 4-byte Spill
	ldr	r0, [sp, #68]                   @ 4-byte Reload
	str	r6, [sp, #28]                   @ 4-byte Spill
	adcs	r0, r0, r1
	str	r0, [sp, #72]                   @ 4-byte Spill
	ldr	r0, [sp, #64]                   @ 4-byte Reload
	ldr	r1, [sp, #32]                   @ 4-byte Reload
	adcs	r0, r0, r2
	str	r0, [sp, #68]                   @ 4-byte Spill
	ldr	r0, [sp, #60]                   @ 4-byte Reload
	adcs	r0, r0, r3
	str	r0, [sp, #64]                   @ 4-byte Spill
	ldr	r0, [sp, #56]                   @ 4-byte Reload
	adcs	r0, r0, r12
	str	r0, [sp, #60]                   @ 4-byte Spill
	ldr	r0, [sp, #52]                   @ 4-byte Reload
	adcs	r0, r0, lr
	str	r0, [sp, #56]                   @ 4-byte Spill
	ldr	r0, [sp, #116]                  @ 4-byte Reload
	adcs	r0, r0, r4
	str	r0, [sp, #52]                   @ 4-byte Spill
	ldr	r0, [sp, #112]                  @ 4-byte Reload
	adcs	r0, r0, r5
	str	r0, [sp, #116]                  @ 4-byte Spill
	ldr	r0, [sp, #108]                  @ 4-byte Reload
	adcs	r0, r0, r8
	str	r0, [sp, #112]                  @ 4-byte Spill
	ldr	r0, [sp, #104]                  @ 4-byte Reload
	adcs	r0, r0, r9
	str	r0, [sp, #108]                  @ 4-byte Spill
	ldr	r0, [sp, #96]                   @ 4-byte Reload
	adcs	r0, r0, r10
	str	r0, [sp, #104]                  @ 4-byte Spill
	ldr	r0, [sp, #92]                   @ 4-byte Reload
	adcs	r0, r0, r1
	str	r0, [sp, #96]                   @ 4-byte Spill
	ldr	r0, [sp, #88]                   @ 4-byte Reload
	ldr	r1, [sp, #36]                   @ 4-byte Reload
	adcs	r0, r0, r1
	str	r0, [sp, #92]                   @ 4-byte Spill
	ldr	r0, [sp, #84]                   @ 4-byte Reload
	ldr	r1, [sp, #40]                   @ 4-byte Reload
	adcs	r0, r0, r1
	str	r0, [sp, #88]                   @ 4-byte Spill
	ldr	r0, [sp, #80]                   @ 4-byte Reload
	ldr	r1, [sp, #44]                   @ 4-byte Reload
	adcs	r0, r0, r1
	str	r0, [sp, #84]                   @ 4-byte Spill
	ldr	r0, [sp, #48]                   @ 4-byte Reload
	ldr	r1, [sp, #132]                  @ 4-byte Reload
	adc	r0, r0, #0
	str	r0, [sp, #80]                   @ 4-byte Spill
	ldr	r0, [sp, #120]                  @ 4-byte Reload
	mul	r2, r0, r6
	add	r0, sp, #568
	bl	mulPv512x32
	ldr	r1, [sp, #124]                  @ 4-byte Reload
	add	r11, sp, #576
	add	r0, sp, #496
	ldr	r2, [r1, #52]
	ldr	r1, [sp, #632]
	str	r1, [sp, #48]                   @ 4-byte Spill
	ldr	r1, [sp, #628]
	str	r1, [sp, #44]                   @ 4-byte Spill
	ldr	r1, [sp, #624]
	str	r1, [sp, #40]                   @ 4-byte Spill
	ldr	r1, [sp, #620]
	str	r1, [sp, #36]                   @ 4-byte Spill
	ldr	r1, [sp, #616]
	str	r1, [sp, #32]                   @ 4-byte Spill
	ldr	r1, [sp, #612]
	str	r1, [sp, #24]                   @ 4-byte Spill
	ldr	r1, [sp, #608]
	str	r1, [sp, #20]                   @ 4-byte Spill
	ldr	r1, [sp, #604]
	str	r1, [sp, #16]                   @ 4-byte Spill
	ldr	r1, [sp, #600]
	str	r1, [sp, #12]                   @ 4-byte Spill
	ldr	r1, [sp, #128]                  @ 4-byte Reload
	ldm	r11, {r5, r6, r11}
	ldr	r7, [sp, #596]
	ldr	r4, [sp, #592]
	ldr	r9, [sp, #588]
	ldr	r10, [sp, #568]
	ldr	r8, [sp, #572]
	bl	mulPv512x32
	ldr	r0, [sp, #28]                   @ 4-byte Reload
	add	lr, sp, #500
	ldr	r1, [sp, #12]                   @ 4-byte Reload
	adds	r0, r0, r10
	add	r10, sp, #524
	ldr	r0, [sp, #76]                   @ 4-byte Reload
	adcs	r0, r0, r8
	str	r0, [sp, #76]                   @ 4-byte Spill
	ldr	r0, [sp, #72]                   @ 4-byte Reload
	adcs	r0, r0, r5
	str	r0, [sp, #72]                   @ 4-byte Spill
	ldr	r0, [sp, #68]                   @ 4-byte Reload
	adcs	r0, r0, r6
	str	r0, [sp, #68]                   @ 4-byte Spill
	ldr	r0, [sp, #64]                   @ 4-byte Reload
	adcs	r0, r0, r11
	str	r0, [sp, #64]                   @ 4-byte Spill
	ldr	r0, [sp, #60]                   @ 4-byte Reload
	ldr	r11, [sp, #76]                  @ 4-byte Reload
	adcs	r0, r0, r9
	str	r0, [sp, #60]                   @ 4-byte Spill
	ldr	r0, [sp, #56]                   @ 4-byte Reload
	adcs	r0, r0, r4
	str	r0, [sp, #56]                   @ 4-byte Spill
	ldr	r0, [sp, #52]                   @ 4-byte Reload
	adcs	r0, r0, r7
	str	r0, [sp, #52]                   @ 4-byte Spill
	ldr	r0, [sp, #116]                  @ 4-byte Reload
	ldr	r7, [sp, #72]                   @ 4-byte Reload
	adcs	r0, r0, r1
	str	r0, [sp, #116]                  @ 4-byte Spill
	ldr	r0, [sp, #112]                  @ 4-byte Reload
	ldr	r1, [sp, #16]                   @ 4-byte Reload
	adcs	r0, r0, r1
	str	r0, [sp, #112]                  @ 4-byte Spill
	ldr	r0, [sp, #108]                  @ 4-byte Reload
	ldr	r1, [sp, #20]                   @ 4-byte Reload
	adcs	r0, r0, r1
	str	r0, [sp, #108]                  @ 4-byte Spill
	ldr	r0, [sp, #104]                  @ 4-byte Reload
	ldr	r1, [sp, #24]                   @ 4-byte Reload
	adcs	r0, r0, r1
	str	r0, [sp, #104]                  @ 4-byte Spill
	ldr	r0, [sp, #96]                   @ 4-byte Reload
	ldr	r1, [sp, #32]                   @ 4-byte Reload
	adcs	r0, r0, r1
	str	r0, [sp, #96]                   @ 4-byte Spill
	ldr	r0, [sp, #92]                   @ 4-byte Reload
	ldr	r1, [sp, #36]                   @ 4-byte Reload
	adcs	r0, r0, r1
	str	r0, [sp, #92]                   @ 4-byte Spill
	ldr	r0, [sp, #88]                   @ 4-byte Reload
	ldr	r1, [sp, #40]                   @ 4-byte Reload
	adcs	r0, r0, r1
	str	r0, [sp, #88]                   @ 4-byte Spill
	ldr	r0, [sp, #84]                   @ 4-byte Reload
	ldr	r1, [sp, #44]                   @ 4-byte Reload
	adcs	r0, r0, r1
	str	r0, [sp, #84]                   @ 4-byte Spill
	ldr	r0, [sp, #80]                   @ 4-byte Reload
	ldr	r1, [sp, #48]                   @ 4-byte Reload
	adc	r0, r0, r1
	str	r0, [sp, #80]                   @ 4-byte Spill
	ldr	r0, [sp, #560]
	str	r0, [sp, #48]                   @ 4-byte Spill
	ldr	r0, [sp, #556]
	str	r0, [sp, #44]                   @ 4-byte Spill
	ldr	r0, [sp, #552]
	str	r0, [sp, #40]                   @ 4-byte Spill
	ldr	r0, [sp, #548]
	str	r0, [sp, #36]                   @ 4-byte Spill
	ldr	r0, [sp, #544]
	str	r0, [sp, #32]                   @ 4-byte Spill
	ldr	r6, [sp, #496]
	ldm	lr, {r0, r1, r2, r3, r12, lr}
	adds	r6, r11, r6
	adcs	r0, r7, r0
	ldm	r10, {r4, r5, r8, r9, r10}
	str	r0, [sp, #76]                   @ 4-byte Spill
	ldr	r0, [sp, #68]                   @ 4-byte Reload
	str	r6, [sp, #28]                   @ 4-byte Spill
	adcs	r0, r0, r1
	str	r0, [sp, #72]                   @ 4-byte Spill
	ldr	r0, [sp, #64]                   @ 4-byte Reload
	ldr	r1, [sp, #32]                   @ 4-byte Reload
	adcs	r0, r0, r2
	str	r0, [sp, #68]                   @ 4-byte Spill
	ldr	r0, [sp, #60]                   @ 4-byte Reload
	adcs	r0, r0, r3
	str	r0, [sp, #64]                   @ 4-byte Spill
	ldr	r0, [sp, #56]                   @ 4-byte Reload
	adcs	r0, r0, r12
	str	r0, [sp, #60]                   @ 4-byte Spill
	ldr	r0, [sp, #52]                   @ 4-byte Reload
	adcs	r0, r0, lr
	str	r0, [sp, #56]                   @ 4-byte Spill
	ldr	r0, [sp, #116]                  @ 4-byte Reload
	adcs	r0, r0, r4
	str	r0, [sp, #52]                   @ 4-byte Spill
	ldr	r0, [sp, #112]                  @ 4-byte Reload
	adcs	r0, r0, r5
	str	r0, [sp, #116]                  @ 4-byte Spill
	ldr	r0, [sp, #108]                  @ 4-byte Reload
	adcs	r0, r0, r8
	str	r0, [sp, #112]                  @ 4-byte Spill
	ldr	r0, [sp, #104]                  @ 4-byte Reload
	adcs	r0, r0, r9
	str	r0, [sp, #108]                  @ 4-byte Spill
	ldr	r0, [sp, #96]                   @ 4-byte Reload
	adcs	r0, r0, r10
	str	r0, [sp, #104]                  @ 4-byte Spill
	ldr	r0, [sp, #92]                   @ 4-byte Reload
	adcs	r0, r0, r1
	str	r0, [sp, #96]                   @ 4-byte Spill
	ldr	r0, [sp, #88]                   @ 4-byte Reload
	ldr	r1, [sp, #36]                   @ 4-byte Reload
	adcs	r0, r0, r1
	str	r0, [sp, #92]                   @ 4-byte Spill
	ldr	r0, [sp, #84]                   @ 4-byte Reload
	ldr	r1, [sp, #40]                   @ 4-byte Reload
	adcs	r0, r0, r1
	str	r0, [sp, #88]                   @ 4-byte Spill
	ldr	r0, [sp, #80]                   @ 4-byte Reload
	ldr	r1, [sp, #44]                   @ 4-byte Reload
	adcs	r0, r0, r1
	str	r0, [sp, #84]                   @ 4-byte Spill
	ldr	r0, [sp, #48]                   @ 4-byte Reload
	ldr	r1, [sp, #132]                  @ 4-byte Reload
	adc	r0, r0, #0
	str	r0, [sp, #80]                   @ 4-byte Spill
	ldr	r0, [sp, #120]                  @ 4-byte Reload
	mul	r2, r0, r6
	add	r0, sp, #424
	bl	mulPv512x32
	ldr	r1, [sp, #124]                  @ 4-byte Reload
	add	r11, sp, #432
	add	r0, sp, #352
	ldr	r2, [r1, #56]
	ldr	r1, [sp, #488]
	str	r1, [sp, #48]                   @ 4-byte Spill
	ldr	r1, [sp, #484]
	str	r1, [sp, #44]                   @ 4-byte Spill
	ldr	r1, [sp, #480]
	str	r1, [sp, #40]                   @ 4-byte Spill
	ldr	r1, [sp, #476]
	str	r1, [sp, #36]                   @ 4-byte Spill
	ldr	r1, [sp, #472]
	str	r1, [sp, #32]                   @ 4-byte Spill
	ldr	r1, [sp, #468]
	str	r1, [sp, #24]                   @ 4-byte Spill
	ldr	r1, [sp, #464]
	str	r1, [sp, #20]                   @ 4-byte Spill
	ldr	r1, [sp, #460]
	str	r1, [sp, #16]                   @ 4-byte Spill
	ldr	r1, [sp, #456]
	str	r1, [sp, #12]                   @ 4-byte Spill
	ldr	r1, [sp, #128]                  @ 4-byte Reload
	ldm	r11, {r5, r6, r11}
	ldr	r7, [sp, #452]
	ldr	r4, [sp, #448]
	ldr	r9, [sp, #444]
	ldr	r10, [sp, #424]
	ldr	r8, [sp, #428]
	bl	mulPv512x32
	ldr	r0, [sp, #28]                   @ 4-byte Reload
	add	lr, sp, #356
	ldr	r1, [sp, #12]                   @ 4-byte Reload
	adds	r0, r0, r10
	add	r10, sp, #380
	ldr	r0, [sp, #76]                   @ 4-byte Reload
	adcs	r0, r0, r8
	str	r0, [sp, #76]                   @ 4-byte Spill
	ldr	r0, [sp, #72]                   @ 4-byte Reload
	adcs	r0, r0, r5
	str	r0, [sp, #72]                   @ 4-byte Spill
	ldr	r0, [sp, #68]                   @ 4-byte Reload
	adcs	r0, r0, r6
	str	r0, [sp, #68]                   @ 4-byte Spill
	ldr	r0, [sp, #64]                   @ 4-byte Reload
	adcs	r0, r0, r11
	str	r0, [sp, #64]                   @ 4-byte Spill
	ldr	r0, [sp, #60]                   @ 4-byte Reload
	ldr	r11, [sp, #76]                  @ 4-byte Reload
	adcs	r0, r0, r9
	str	r0, [sp, #60]                   @ 4-byte Spill
	ldr	r0, [sp, #56]                   @ 4-byte Reload
	adcs	r0, r0, r4
	str	r0, [sp, #56]                   @ 4-byte Spill
	ldr	r0, [sp, #52]                   @ 4-byte Reload
	adcs	r0, r0, r7
	str	r0, [sp, #52]                   @ 4-byte Spill
	ldr	r0, [sp, #116]                  @ 4-byte Reload
	ldr	r7, [sp, #72]                   @ 4-byte Reload
	adcs	r0, r0, r1
	str	r0, [sp, #116]                  @ 4-byte Spill
	ldr	r0, [sp, #112]                  @ 4-byte Reload
	ldr	r1, [sp, #16]                   @ 4-byte Reload
	adcs	r0, r0, r1
	str	r0, [sp, #112]                  @ 4-byte Spill
	ldr	r0, [sp, #108]                  @ 4-byte Reload
	ldr	r1, [sp, #20]                   @ 4-byte Reload
	adcs	r0, r0, r1
	str	r0, [sp, #108]                  @ 4-byte Spill
	ldr	r0, [sp, #104]                  @ 4-byte Reload
	ldr	r1, [sp, #24]                   @ 4-byte Reload
	adcs	r0, r0, r1
	str	r0, [sp, #104]                  @ 4-byte Spill
	ldr	r0, [sp, #96]                   @ 4-byte Reload
	ldr	r1, [sp, #32]                   @ 4-byte Reload
	adcs	r0, r0, r1
	str	r0, [sp, #96]                   @ 4-byte Spill
	ldr	r0, [sp, #92]                   @ 4-byte Reload
	ldr	r1, [sp, #36]                   @ 4-byte Reload
	adcs	r0, r0, r1
	str	r0, [sp, #92]                   @ 4-byte Spill
	ldr	r0, [sp, #88]                   @ 4-byte Reload
	ldr	r1, [sp, #40]                   @ 4-byte Reload
	adcs	r0, r0, r1
	str	r0, [sp, #88]                   @ 4-byte Spill
	ldr	r0, [sp, #84]                   @ 4-byte Reload
	ldr	r1, [sp, #44]                   @ 4-byte Reload
	adcs	r0, r0, r1
	str	r0, [sp, #84]                   @ 4-byte Spill
	ldr	r0, [sp, #80]                   @ 4-byte Reload
	ldr	r1, [sp, #48]                   @ 4-byte Reload
	adc	r0, r0, r1
	str	r0, [sp, #80]                   @ 4-byte Spill
	ldr	r0, [sp, #416]
	str	r0, [sp, #48]                   @ 4-byte Spill
	ldr	r0, [sp, #412]
	str	r0, [sp, #44]                   @ 4-byte Spill
	ldr	r0, [sp, #408]
	str	r0, [sp, #40]                   @ 4-byte Spill
	ldr	r0, [sp, #404]
	str	r0, [sp, #36]                   @ 4-byte Spill
	ldr	r0, [sp, #400]
	str	r0, [sp, #28]                   @ 4-byte Spill
	ldr	r6, [sp, #352]
	ldm	lr, {r0, r1, r2, r3, r12, lr}
	adds	r6, r11, r6
	adcs	r0, r7, r0
	ldm	r10, {r4, r5, r8, r9, r10}
	str	r0, [sp, #76]                   @ 4-byte Spill
	ldr	r0, [sp, #68]                   @ 4-byte Reload
	str	r6, [sp, #32]                   @ 4-byte Spill
	adcs	r0, r0, r1
	str	r0, [sp, #72]                   @ 4-byte Spill
	ldr	r0, [sp, #64]                   @ 4-byte Reload
	ldr	r1, [sp, #28]                   @ 4-byte Reload
	adcs	r0, r0, r2
	str	r0, [sp, #68]                   @ 4-byte Spill
	ldr	r0, [sp, #60]                   @ 4-byte Reload
	adcs	r0, r0, r3
	str	r0, [sp, #64]                   @ 4-byte Spill
	ldr	r0, [sp, #56]                   @ 4-byte Reload
	adcs	r0, r0, r12
	str	r0, [sp, #60]                   @ 4-byte Spill
	ldr	r0, [sp, #52]                   @ 4-byte Reload
	adcs	r0, r0, lr
	str	r0, [sp, #56]                   @ 4-byte Spill
	ldr	r0, [sp, #116]                  @ 4-byte Reload
	adcs	r0, r0, r4
	str	r0, [sp, #52]                   @ 4-byte Spill
	ldr	r0, [sp, #112]                  @ 4-byte Reload
	adcs	r0, r0, r5
	str	r0, [sp, #116]                  @ 4-byte Spill
	ldr	r0, [sp, #108]                  @ 4-byte Reload
	adcs	r0, r0, r8
	str	r0, [sp, #112]                  @ 4-byte Spill
	ldr	r0, [sp, #104]                  @ 4-byte Reload
	adcs	r0, r0, r9
	str	r0, [sp, #108]                  @ 4-byte Spill
	ldr	r0, [sp, #96]                   @ 4-byte Reload
	adcs	r0, r0, r10
	str	r0, [sp, #104]                  @ 4-byte Spill
	ldr	r0, [sp, #92]                   @ 4-byte Reload
	adcs	r0, r0, r1
	str	r0, [sp, #96]                   @ 4-byte Spill
	ldr	r0, [sp, #88]                   @ 4-byte Reload
	ldr	r1, [sp, #36]                   @ 4-byte Reload
	adcs	r0, r0, r1
	str	r0, [sp, #92]                   @ 4-byte Spill
	ldr	r0, [sp, #84]                   @ 4-byte Reload
	ldr	r1, [sp, #40]                   @ 4-byte Reload
	adcs	r0, r0, r1
	str	r0, [sp, #88]                   @ 4-byte Spill
	ldr	r0, [sp, #80]                   @ 4-byte Reload
	ldr	r1, [sp, #44]                   @ 4-byte Reload
	adcs	r0, r0, r1
	str	r0, [sp, #84]                   @ 4-byte Spill
	ldr	r0, [sp, #48]                   @ 4-byte Reload
	ldr	r1, [sp, #132]                  @ 4-byte Reload
	adc	r0, r0, #0
	str	r0, [sp, #80]                   @ 4-byte Spill
	ldr	r0, [sp, #120]                  @ 4-byte Reload
	mul	r2, r0, r6
	add	r0, sp, #280
	bl	mulPv512x32
	ldr	r1, [sp, #124]                  @ 4-byte Reload
	add	r0, sp, #208
	ldr	r2, [r1, #60]
	ldr	r1, [sp, #344]
	str	r1, [sp, #124]                  @ 4-byte Spill
	ldr	r1, [sp, #340]
	str	r1, [sp, #48]                   @ 4-byte Spill
	ldr	r1, [sp, #336]
	str	r1, [sp, #44]                   @ 4-byte Spill
	ldr	r1, [sp, #332]
	str	r1, [sp, #40]                   @ 4-byte Spill
	ldr	r1, [sp, #328]
	str	r1, [sp, #36]                   @ 4-byte Spill
	ldr	r1, [sp, #324]
	str	r1, [sp, #28]                   @ 4-byte Spill
	ldr	r1, [sp, #320]
	str	r1, [sp, #24]                   @ 4-byte Spill
	ldr	r1, [sp, #316]
	str	r1, [sp, #20]                   @ 4-byte Spill
	ldr	r1, [sp, #312]
	str	r1, [sp, #16]                   @ 4-byte Spill
	ldr	r1, [sp, #128]                  @ 4-byte Reload
	ldr	r4, [sp, #308]
	ldr	r11, [sp, #304]
	ldr	r8, [sp, #300]
	ldr	r9, [sp, #296]
	ldr	r10, [sp, #280]
	ldr	r5, [sp, #284]
	ldr	r7, [sp, #288]
	ldr	r6, [sp, #292]
	bl	mulPv512x32
	ldr	r0, [sp, #32]                   @ 4-byte Reload
	add	lr, sp, #224
	ldr	r1, [sp, #72]                   @ 4-byte Reload
	adds	r0, r0, r10
	ldr	r2, [sp, #16]                   @ 4-byte Reload
	ldr	r0, [sp, #76]                   @ 4-byte Reload
	adcs	r0, r0, r5
	adcs	r1, r1, r7
	str	r1, [sp, #128]                  @ 4-byte Spill
	ldr	r1, [sp, #68]                   @ 4-byte Reload
	adcs	r1, r1, r6
	str	r1, [sp, #76]                   @ 4-byte Spill
	ldr	r1, [sp, #64]                   @ 4-byte Reload
	ldr	r5, [sp, #76]                   @ 4-byte Reload
	adcs	r1, r1, r9
	str	r1, [sp, #72]                   @ 4-byte Spill
	ldr	r1, [sp, #60]                   @ 4-byte Reload
	adcs	r1, r1, r8
	str	r1, [sp, #68]                   @ 4-byte Spill
	ldr	r1, [sp, #56]                   @ 4-byte Reload
	adcs	r1, r1, r11
	str	r1, [sp, #64]                   @ 4-byte Spill
	ldr	r1, [sp, #52]                   @ 4-byte Reload
	ldr	r11, [sp, #128]                 @ 4-byte Reload
	adcs	r1, r1, r4
	str	r1, [sp, #60]                   @ 4-byte Spill
	ldr	r1, [sp, #116]                  @ 4-byte Reload
	adcs	r1, r1, r2
	str	r1, [sp, #116]                  @ 4-byte Spill
	ldr	r1, [sp, #112]                  @ 4-byte Reload
	ldr	r2, [sp, #20]                   @ 4-byte Reload
	adcs	r1, r1, r2
	str	r1, [sp, #112]                  @ 4-byte Spill
	ldr	r1, [sp, #108]                  @ 4-byte Reload
	ldr	r2, [sp, #24]                   @ 4-byte Reload
	adcs	r1, r1, r2
	str	r1, [sp, #108]                  @ 4-byte Spill
	ldr	r1, [sp, #104]                  @ 4-byte Reload
	ldr	r2, [sp, #28]                   @ 4-byte Reload
	adcs	r1, r1, r2
	str	r1, [sp, #104]                  @ 4-byte Spill
	ldr	r1, [sp, #96]                   @ 4-byte Reload
	ldr	r2, [sp, #36]                   @ 4-byte Reload
	adcs	r1, r1, r2
	str	r1, [sp, #96]                   @ 4-byte Spill
	ldr	r1, [sp, #92]                   @ 4-byte Reload
	ldr	r2, [sp, #40]                   @ 4-byte Reload
	adcs	r1, r1, r2
	str	r1, [sp, #92]                   @ 4-byte Spill
	ldr	r1, [sp, #88]                   @ 4-byte Reload
	ldr	r2, [sp, #44]                   @ 4-byte Reload
	adcs	r1, r1, r2
	str	r1, [sp, #88]                   @ 4-byte Spill
	ldr	r1, [sp, #84]                   @ 4-byte Reload
	ldr	r2, [sp, #48]                   @ 4-byte Reload
	adcs	r1, r1, r2
	str	r1, [sp, #84]                   @ 4-byte Spill
	ldr	r1, [sp, #80]                   @ 4-byte Reload
	ldr	r2, [sp, #124]                  @ 4-byte Reload
	adc	r1, r1, r2
	str	r1, [sp, #124]                  @ 4-byte Spill
	ldr	r4, [sp, #208]
	ldr	r10, [sp, #212]
	adds	r7, r0, r4
	ldr	r0, [sp, #120]                  @ 4-byte Reload
	ldr	r9, [sp, #216]
	adcs	r10, r11, r10
	ldr	r6, [sp, #220]
	mul	r1, r0, r7
	adcs	r11, r5, r9
	ldr	r5, [sp, #72]                   @ 4-byte Reload
	ldr	r9, [sp, #132]                  @ 4-byte Reload
	adcs	r6, r5, r6
	ldr	r5, [sp, #68]                   @ 4-byte Reload
	str	r1, [sp, #80]                   @ 4-byte Spill
	ldr	r0, [sp, #272]
	str	r0, [sp, #56]                   @ 4-byte Spill
	ldr	r0, [sp, #268]
	str	r0, [sp, #52]                   @ 4-byte Spill
	ldr	r0, [sp, #264]
	str	r0, [sp, #48]                   @ 4-byte Spill
	ldr	r0, [sp, #260]
	str	r0, [sp, #44]                   @ 4-byte Spill
	ldr	r0, [sp, #256]
	str	r0, [sp, #40]                   @ 4-byte Spill
	ldm	lr, {r0, r1, r2, r3, r12, lr}
	adcs	r0, r5, r0
	ldr	r8, [sp, #252]
	ldr	r4, [sp, #248]
	str	r0, [sp, #72]                   @ 4-byte Spill
	ldr	r0, [sp, #64]                   @ 4-byte Reload
	adcs	r0, r0, r1
	str	r0, [sp, #128]                  @ 4-byte Spill
	ldr	r0, [sp, #60]                   @ 4-byte Reload
	ldr	r1, [sp, #40]                   @ 4-byte Reload
	adcs	r0, r0, r2
	str	r0, [sp, #64]                   @ 4-byte Spill
	ldr	r0, [sp, #116]                  @ 4-byte Reload
	ldr	r2, [sp, #80]                   @ 4-byte Reload
	adcs	r0, r0, r3
	str	r0, [sp, #120]                  @ 4-byte Spill
	ldr	r0, [sp, #112]                  @ 4-byte Reload
	adcs	r0, r0, r12
	str	r0, [sp, #116]                  @ 4-byte Spill
	ldr	r0, [sp, #108]                  @ 4-byte Reload
	adcs	r0, r0, lr
	str	r0, [sp, #112]                  @ 4-byte Spill
	ldr	r0, [sp, #104]                  @ 4-byte Reload
	adcs	r0, r0, r4
	str	r0, [sp, #108]                  @ 4-byte Spill
	ldr	r0, [sp, #96]                   @ 4-byte Reload
	adcs	r8, r0, r8
	ldr	r0, [sp, #92]                   @ 4-byte Reload
	adcs	r0, r0, r1
	str	r0, [sp, #68]                   @ 4-byte Spill
	ldr	r0, [sp, #88]                   @ 4-byte Reload
	ldr	r1, [sp, #44]                   @ 4-byte Reload
	adcs	r0, r0, r1
	str	r0, [sp, #76]                   @ 4-byte Spill
	ldr	r0, [sp, #84]                   @ 4-byte Reload
	ldr	r1, [sp, #48]                   @ 4-byte Reload
	adcs	r0, r0, r1
	str	r0, [sp, #84]                   @ 4-byte Spill
	ldr	r0, [sp, #124]                  @ 4-byte Reload
	ldr	r1, [sp, #52]                   @ 4-byte Reload
	adcs	r0, r0, r1
	str	r0, [sp, #92]                   @ 4-byte Spill
	ldr	r0, [sp, #56]                   @ 4-byte Reload
	mov	r1, r9
	adc	r5, r0, #0
	add	r0, sp, #136
	bl	mulPv512x32
	add	r3, sp, #136
	ldm	r3, {r0, r1, r2, r3}
	adds	r0, r7, r0
	ldr	r7, [r9]
	adcs	lr, r10, r1
	str	lr, [sp, #96]                   @ 4-byte Spill
	adcs	r12, r11, r2
	str	r12, [sp, #88]                  @ 4-byte Spill
	adcs	r6, r6, r3
	str	r6, [sp, #80]                   @ 4-byte Spill
	ldr	r0, [sp, #152]
	ldr	r1, [sp, #72]                   @ 4-byte Reload
	ldr	r2, [r9, #4]
	adcs	r4, r1, r0
	str	r4, [sp, #72]                   @ 4-byte Spill
	ldr	r0, [sp, #156]
	ldr	r1, [sp, #128]                  @ 4-byte Reload
	adcs	r0, r1, r0
	str	r0, [sp, #128]                  @ 4-byte Spill
	ldr	r0, [sp, #160]
	ldr	r1, [sp, #64]                   @ 4-byte Reload
	adcs	r0, r1, r0
	str	r0, [sp, #124]                  @ 4-byte Spill
	ldr	r0, [sp, #164]
	ldr	r1, [sp, #120]                  @ 4-byte Reload
	adcs	r0, r1, r0
	str	r0, [sp, #120]                  @ 4-byte Spill
	ldr	r0, [sp, #168]
	ldr	r1, [sp, #116]                  @ 4-byte Reload
	adcs	r0, r1, r0
	str	r0, [sp, #116]                  @ 4-byte Spill
	ldr	r0, [sp, #172]
	ldr	r1, [sp, #112]                  @ 4-byte Reload
	adcs	r0, r1, r0
	str	r0, [sp, #112]                  @ 4-byte Spill
	ldr	r0, [sp, #176]
	ldr	r1, [sp, #108]                  @ 4-byte Reload
	adcs	r0, r1, r0
	str	r0, [sp, #108]                  @ 4-byte Spill
	ldr	r0, [sp, #180]
	ldr	r1, [sp, #68]                   @ 4-byte Reload
	adcs	r0, r8, r0
	str	r0, [sp, #104]                  @ 4-byte Spill
	ldr	r0, [sp, #184]
	adcs	r0, r1, r0
	str	r0, [sp, #52]                   @ 4-byte Spill
	ldr	r0, [sp, #188]
	ldr	r1, [sp, #76]                   @ 4-byte Reload
	adcs	r0, r1, r0
	str	r0, [sp, #48]                   @ 4-byte Spill
	ldr	r0, [sp, #192]
	ldr	r1, [sp, #84]                   @ 4-byte Reload
	adcs	r0, r1, r0
	str	r0, [sp, #44]                   @ 4-byte Spill
	ldr	r0, [sp, #196]
	ldr	r1, [sp, #92]                   @ 4-byte Reload
	adcs	r0, r1, r0
	str	r0, [sp, #40]                   @ 4-byte Spill
	ldr	r0, [sp, #200]
	mov	r1, r9
	adc	r3, r5, r0
	subs	r7, lr, r7
	ldr	r0, [r9, #8]
	sbcs	r2, r12, r2
	ldr	r5, [r9, #12]
	add	lr, r9, #32
	str	r2, [sp, #84]                   @ 4-byte Spill
	sbcs	r2, r6, r0
	str	r2, [sp, #76]                   @ 4-byte Spill
	sbcs	r2, r4, r5
	str	r2, [sp, #68]                   @ 4-byte Spill
	add	r9, r9, #44
	ldr	r2, [r1, #28]
	ldr	r0, [r1, #24]
	ldr	r12, [r1, #20]
	ldr	r1, [r1, #16]
	ldr	r4, [sp, #128]                  @ 4-byte Reload
	str	r7, [sp, #92]                   @ 4-byte Spill
	sbcs	r1, r4, r1
	ldm	lr, {r10, r11, lr}
	ldm	r9, {r5, r6, r7, r8, r9}
	str	r1, [sp, #132]                  @ 4-byte Spill
	ldr	r1, [sp, #124]                  @ 4-byte Reload
	sbcs	r1, r1, r12
	str	r1, [sp, #64]                   @ 4-byte Spill
	ldr	r1, [sp, #120]                  @ 4-byte Reload
	sbcs	r0, r1, r0
	ldr	r1, [sp, #116]                  @ 4-byte Reload
	str	r0, [sp, #60]                   @ 4-byte Spill
	sbcs	r0, r1, r2
	ldr	r1, [sp, #112]                  @ 4-byte Reload
	str	r0, [sp, #56]                   @ 4-byte Spill
	sbcs	r10, r1, r10
	ldr	r1, [sp, #108]                  @ 4-byte Reload
	ldr	r0, [sp, #104]                  @ 4-byte Reload
	sbcs	r12, r1, r11
	ldr	r2, [sp, #48]                   @ 4-byte Reload
	sbcs	r4, r0, lr
	ldr	lr, [sp, #52]                   @ 4-byte Reload
	ldr	r1, [sp, #44]                   @ 4-byte Reload
	sbcs	r5, lr, r5
	ldr	r0, [sp, #40]                   @ 4-byte Reload
	sbcs	r6, r2, r6
	sbcs	r7, r1, r7
	sbcs	r8, r0, r8
	sbc	r11, r3, r9
	ldr	r9, [sp, #100]                  @ 4-byte Reload
	cmn	r11, #1
	movle	r8, r0
	ldr	r0, [sp, #104]                  @ 4-byte Reload
	movle	r7, r1
	movgt	r3, r11
	cmn	r11, #1
	ldr	r1, [sp, #56]                   @ 4-byte Reload
	movle	r4, r0
	ldr	r0, [sp, #108]                  @ 4-byte Reload
	movle	r6, r2
	movle	r5, lr
	cmn	r11, #1
	str	r3, [r9, #60]
	movle	r12, r0
	ldr	r0, [sp, #112]                  @ 4-byte Reload
	str	r8, [r9, #56]
	str	r7, [r9, #52]
	movle	r10, r0
	ldr	r0, [sp, #116]                  @ 4-byte Reload
	str	r6, [r9, #48]
	str	r5, [r9, #44]
	movle	r1, r0
	ldr	r0, [sp, #120]                  @ 4-byte Reload
	str	r1, [r9, #28]
	cmn	r11, #1
	ldr	r1, [sp, #60]                   @ 4-byte Reload
	movle	r1, r0
	ldr	r0, [sp, #124]                  @ 4-byte Reload
	str	r1, [r9, #24]
	ldr	r1, [sp, #64]                   @ 4-byte Reload
	movle	r1, r0
	ldr	r0, [sp, #128]                  @ 4-byte Reload
	str	r1, [r9, #20]
	ldr	r1, [sp, #132]                  @ 4-byte Reload
	movle	r1, r0
	cmn	r11, #1
	str	r1, [r9, #16]
	ldr	r1, [sp, #72]                   @ 4-byte Reload
	ldr	r0, [sp, #68]                   @ 4-byte Reload
	str	r4, [r9, #40]
	movle	r0, r1
	ldr	r1, [sp, #80]                   @ 4-byte Reload
	str	r0, [r9, #12]
	ldr	r0, [sp, #76]                   @ 4-byte Reload
	movle	r0, r1
	ldr	r1, [sp, #88]                   @ 4-byte Reload
	str	r0, [r9, #8]
	ldr	r0, [sp, #84]                   @ 4-byte Reload
	movle	r0, r1
	ldr	r1, [sp, #96]                   @ 4-byte Reload
	str	r0, [r9, #4]
	cmn	r11, #1
	ldr	r0, [sp, #92]                   @ 4-byte Reload
	movle	r0, r1
	str	r12, [r9, #36]
	str	r10, [r9, #32]
	str	r0, [r9]
	add	sp, sp, #396
	add	sp, sp, #2048
	pop	{r4, r5, r6, r7, r8, r9, r10, r11, lr}
	mov	pc, lr
.Lfunc_end78:
	.size	mcl_fp_montNF16L, .Lfunc_end78-mcl_fp_montNF16L
	.cantunwind
	.fnend
                                        @ -- End function
	.globl	mcl_fp_montRed16L               @ -- Begin function mcl_fp_montRed16L
	.p2align	2
	.type	mcl_fp_montRed16L,%function
	.code	32                              @ @mcl_fp_montRed16L
mcl_fp_montRed16L:
	.fnstart
@ %bb.0:
	.save	{r4, r5, r6, r7, r8, r9, r10, r11, lr}
	push	{r4, r5, r6, r7, r8, r9, r10, r11, lr}
	.pad	#372
	sub	sp, sp, #372
	.pad	#1024
	sub	sp, sp, #1024
	str	r0, [sp, #224]                  @ 4-byte Spill
	mov	r3, r2
	ldr	r0, [r2]
	add	lr, sp, #1024
	str	r0, [sp, #216]                  @ 4-byte Spill
	ldr	r0, [r2, #4]
	str	r0, [sp, #212]                  @ 4-byte Spill
	ldr	r0, [r2, #8]
	str	r0, [sp, #208]                  @ 4-byte Spill
	ldr	r0, [r2, #12]
	str	r0, [sp, #192]                  @ 4-byte Spill
	ldr	r0, [r2, #16]
	str	r0, [sp, #196]                  @ 4-byte Spill
	ldr	r0, [r2, #20]
	str	r0, [sp, #200]                  @ 4-byte Spill
	ldr	r0, [r2, #24]
	str	r0, [sp, #204]                  @ 4-byte Spill
	ldr	r0, [r1, #4]
	str	r0, [sp, #156]                  @ 4-byte Spill
	ldr	r0, [r1, #8]
	str	r0, [sp, #152]                  @ 4-byte Spill
	ldr	r0, [r1, #12]
	str	r0, [sp, #148]                  @ 4-byte Spill
	ldr	r0, [r3, #60]
	str	r0, [sp, #220]                  @ 4-byte Spill
	ldr	r0, [r3, #28]
	str	r0, [sp, #160]                  @ 4-byte Spill
	ldr	r0, [r3, #32]
	str	r0, [sp, #164]                  @ 4-byte Spill
	ldr	r0, [r3, #36]
	str	r0, [sp, #168]                  @ 4-byte Spill
	ldr	r0, [r3, #40]
	str	r0, [sp, #172]                  @ 4-byte Spill
	ldr	r0, [r3, #44]
	str	r0, [sp, #176]                  @ 4-byte Spill
	ldr	r0, [r3, #48]
	str	r0, [sp, #180]                  @ 4-byte Spill
	ldr	r0, [r3, #52]
	str	r0, [sp, #184]                  @ 4-byte Spill
	ldr	r0, [r3, #56]
	str	r0, [sp, #188]                  @ 4-byte Spill
	ldr	r0, [r1, #32]
	str	r0, [sp, #116]                  @ 4-byte Spill
	ldr	r0, [r1, #36]
	str	r0, [sp, #120]                  @ 4-byte Spill
	ldr	r0, [r1, #40]
	str	r0, [sp, #124]                  @ 4-byte Spill
	ldr	r0, [r1, #44]
	str	r0, [sp, #128]                  @ 4-byte Spill
	ldr	r0, [r1, #48]
	ldr	r7, [r2, #-4]
	ldr	r10, [r1]
	str	r0, [sp, #132]                  @ 4-byte Spill
	ldr	r0, [r1, #52]
	str	r0, [sp, #136]                  @ 4-byte Spill
	mul	r2, r10, r7
	ldr	r0, [r1, #56]
	str	r0, [sp, #140]                  @ 4-byte Spill
	ldr	r0, [r1, #60]
	str	r0, [sp, #144]                  @ 4-byte Spill
	ldr	r0, [r1, #28]
	str	r0, [sp, #112]                  @ 4-byte Spill
	ldr	r0, [r1, #24]
	str	r0, [sp, #108]                  @ 4-byte Spill
	ldr	r0, [r1, #20]
	str	r0, [sp, #104]                  @ 4-byte Spill
	ldr	r0, [r1, #16]
	str	r0, [sp, #100]                  @ 4-byte Spill
	add	r0, lr, #296
	str	r1, [sp, #236]                  @ 4-byte Spill
	mov	r1, r3
	str	r7, [sp, #228]                  @ 4-byte Spill
	str	r3, [sp, #232]                  @ 4-byte Spill
	bl	mulPv512x32
	ldr	r0, [sp, #1380]
	add	r11, sp, #1344
	str	r0, [sp, #96]                   @ 4-byte Spill
	ldr	r0, [sp, #1376]
	str	r0, [sp, #92]                   @ 4-byte Spill
	ldr	r0, [sp, #1372]
	str	r0, [sp, #88]                   @ 4-byte Spill
	ldr	r0, [sp, #1320]
	ldr	r1, [sp, #1324]
	adds	r0, r10, r0
	ldr	r2, [sp, #1328]
	ldr	r0, [sp, #156]                  @ 4-byte Reload
	ldm	r11, {r4, r5, r6, r7, r8, r9, r11}
	adcs	r10, r0, r1
	ldr	r0, [sp, #152]                  @ 4-byte Reload
	ldr	lr, [sp, #1340]
	adcs	r0, r0, r2
	ldr	r12, [sp, #1336]
	ldr	r3, [sp, #1332]
	str	r0, [sp, #68]                   @ 4-byte Spill
	ldr	r0, [sp, #148]                  @ 4-byte Reload
	ldr	r1, [sp, #88]                   @ 4-byte Reload
	adcs	r0, r0, r3
	str	r0, [sp, #148]                  @ 4-byte Spill
	ldr	r0, [sp, #100]                  @ 4-byte Reload
	adcs	r0, r0, r12
	str	r0, [sp, #72]                   @ 4-byte Spill
	ldr	r0, [sp, #104]                  @ 4-byte Reload
	adcs	r0, r0, lr
	str	r0, [sp, #76]                   @ 4-byte Spill
	ldr	r0, [sp, #108]                  @ 4-byte Reload
	adcs	r0, r0, r4
	str	r0, [sp, #156]                  @ 4-byte Spill
	ldr	r0, [sp, #112]                  @ 4-byte Reload
	adcs	r0, r0, r5
	str	r0, [sp, #112]                  @ 4-byte Spill
	ldr	r0, [sp, #116]                  @ 4-byte Reload
	adcs	r0, r0, r6
	str	r0, [sp, #116]                  @ 4-byte Spill
	ldr	r0, [sp, #120]                  @ 4-byte Reload
	adcs	r0, r0, r7
	str	r0, [sp, #120]                  @ 4-byte Spill
	ldr	r0, [sp, #124]                  @ 4-byte Reload
	adcs	r0, r0, r8
	str	r0, [sp, #108]                  @ 4-byte Spill
	ldr	r0, [sp, #128]                  @ 4-byte Reload
	adcs	r0, r0, r9
	str	r0, [sp, #104]                  @ 4-byte Spill
	ldr	r0, [sp, #132]                  @ 4-byte Reload
	adcs	r0, r0, r11
	str	r0, [sp, #100]                  @ 4-byte Spill
	ldr	r0, [sp, #136]                  @ 4-byte Reload
	adcs	r0, r0, r1
	str	r0, [sp, #88]                   @ 4-byte Spill
	ldr	r0, [sp, #140]                  @ 4-byte Reload
	ldr	r1, [sp, #92]                   @ 4-byte Reload
	adcs	r0, r0, r1
	str	r0, [sp, #84]                   @ 4-byte Spill
	ldr	r0, [sp, #144]                  @ 4-byte Reload
	ldr	r1, [sp, #96]                   @ 4-byte Reload
	adcs	r0, r0, r1
	str	r0, [sp, #80]                   @ 4-byte Spill
	mrs	r0, apsr
	ldr	r6, [sp, #228]                  @ 4-byte Reload
	ldr	r7, [sp, #236]                  @ 4-byte Reload
	str	r0, [sp, #96]                   @ 4-byte Spill
	mul	r2, r6, r10
	ldr	r8, [sp, #232]                  @ 4-byte Reload
	ldr	r0, [r7, #64]
	str	r0, [sp, #152]                  @ 4-byte Spill
	ldr	r0, [sp, #1384]
	mov	r1, r8
	str	r0, [sp, #92]                   @ 4-byte Spill
	add	r0, sp, #1248
	bl	mulPv512x32
	ldr	r0, [sp, #1304]
	str	r0, [sp, #64]                   @ 4-byte Spill
	ldr	r0, [sp, #1300]
	str	r0, [sp, #60]                   @ 4-byte Spill
	ldr	r5, [sp, #1248]
	ldr	r4, [sp, #1252]
	ldr	r2, [sp, #68]                   @ 4-byte Reload
	adds	r5, r10, r5
	ldr	r0, [sp, #1256]
	adcs	r9, r2, r4
	ldr	r4, [sp, #148]                  @ 4-byte Reload
	ldr	r1, [sp, #1260]
	adcs	r0, r4, r0
	ldr	r4, [sp, #72]                   @ 4-byte Reload
	ldr	lr, [sp, #1296]
	ldr	r12, [sp, #1292]
	adcs	r1, r4, r1
	ldr	r11, [sp, #1288]
	ldr	r3, [sp, #1284]
	ldr	r5, [sp, #1280]
	ldr	r2, [sp, #1276]
	str	r0, [sp, #148]                  @ 4-byte Spill
	ldr	r0, [sp, #1272]
	str	r1, [sp, #144]                  @ 4-byte Spill
	ldr	r1, [sp, #1264]
	ldr	r4, [sp, #76]                   @ 4-byte Reload
	adcs	r1, r4, r1
	str	r1, [sp, #140]                  @ 4-byte Spill
	ldr	r1, [sp, #1268]
	ldr	r4, [sp, #156]                  @ 4-byte Reload
	adcs	r1, r4, r1
	str	r1, [sp, #136]                  @ 4-byte Spill
	ldr	r1, [sp, #112]                  @ 4-byte Reload
	adcs	r0, r1, r0
	str	r0, [sp, #132]                  @ 4-byte Spill
	ldr	r0, [sp, #116]                  @ 4-byte Reload
	ldr	r1, [sp, #60]                   @ 4-byte Reload
	adcs	r0, r0, r2
	str	r0, [sp, #128]                  @ 4-byte Spill
	ldr	r0, [sp, #120]                  @ 4-byte Reload
	mul	r2, r6, r9
	adcs	r0, r0, r5
	str	r0, [sp, #124]                  @ 4-byte Spill
	ldr	r0, [sp, #108]                  @ 4-byte Reload
	adcs	r0, r0, r3
	str	r0, [sp, #120]                  @ 4-byte Spill
	ldr	r0, [sp, #104]                  @ 4-byte Reload
	adcs	r0, r0, r11
	str	r0, [sp, #116]                  @ 4-byte Spill
	ldr	r0, [sp, #100]                  @ 4-byte Reload
	mov	r11, r7
	adcs	r0, r0, r12
	str	r0, [sp, #112]                  @ 4-byte Spill
	ldr	r0, [sp, #88]                   @ 4-byte Reload
	adcs	r0, r0, lr
	str	r0, [sp, #108]                  @ 4-byte Spill
	ldr	r0, [sp, #84]                   @ 4-byte Reload
	add	lr, sp, #1024
	adcs	r0, r0, r1
	str	r0, [sp, #104]                  @ 4-byte Spill
	ldr	r0, [sp, #80]                   @ 4-byte Reload
	ldr	r1, [sp, #64]                   @ 4-byte Reload
	adcs	r0, r0, r1
	str	r0, [sp, #100]                  @ 4-byte Spill
	mrs	r0, apsr
	str	r0, [sp, #156]                  @ 4-byte Spill
	ldr	r0, [sp, #96]                   @ 4-byte Reload
	msr	APSR_nzcvq, r0
	ldr	r1, [sp, #152]                  @ 4-byte Reload
	ldr	r3, [sp, #92]                   @ 4-byte Reload
	ldr	r0, [sp, #1312]
	adcs	r1, r1, r3
	str	r1, [sp, #96]                   @ 4-byte Spill
	adc	r0, r0, #0
	str	r0, [sp, #92]                   @ 4-byte Spill
	ldr	r0, [r7, #68]
	mov	r1, r8
	str	r0, [sp, #88]                   @ 4-byte Spill
	ldr	r0, [sp, #1308]
	str	r0, [sp, #84]                   @ 4-byte Spill
	add	r0, lr, #152
	bl	mulPv512x32
	ldr	r0, [sp, #1228]
	str	r0, [sp, #152]                  @ 4-byte Spill
	ldr	r0, [sp, #1224]
	str	r0, [sp, #80]                   @ 4-byte Spill
	ldr	r7, [sp, #1176]
	ldr	r0, [sp, #1180]
	ldr	r2, [sp, #148]                  @ 4-byte Reload
	adds	r4, r9, r7
	ldr	r1, [sp, #1184]
	adcs	r4, r2, r0
	ldr	r2, [sp, #144]                  @ 4-byte Reload
	ldr	r10, [sp, #1188]
	adcs	r1, r2, r1
	ldr	r2, [sp, #140]                  @ 4-byte Reload
	ldr	lr, [sp, #1220]
	ldr	r12, [sp, #1216]
	adcs	r2, r2, r10
	ldr	r6, [sp, #1212]
	ldr	r5, [sp, #1208]
	ldr	r7, [sp, #1204]
	ldr	r0, [sp, #1200]
	str	r1, [sp, #148]                  @ 4-byte Spill
	ldr	r1, [sp, #1196]
	str	r2, [sp, #144]                  @ 4-byte Spill
	ldr	r2, [sp, #1192]
	ldr	r3, [sp, #136]                  @ 4-byte Reload
	adcs	r2, r3, r2
	str	r2, [sp, #140]                  @ 4-byte Spill
	ldr	r2, [sp, #132]                  @ 4-byte Reload
	adcs	r1, r2, r1
	str	r1, [sp, #136]                  @ 4-byte Spill
	ldr	r1, [sp, #128]                  @ 4-byte Reload
	adcs	r0, r1, r0
	str	r0, [sp, #132]                  @ 4-byte Spill
	ldr	r0, [sp, #124]                  @ 4-byte Reload
	ldr	r1, [sp, #80]                   @ 4-byte Reload
	adcs	r0, r0, r7
	str	r0, [sp, #128]                  @ 4-byte Spill
	ldr	r0, [sp, #120]                  @ 4-byte Reload
	adcs	r0, r0, r5
	str	r0, [sp, #124]                  @ 4-byte Spill
	ldr	r0, [sp, #116]                  @ 4-byte Reload
	adcs	r0, r0, r6
	str	r0, [sp, #120]                  @ 4-byte Spill
	ldr	r0, [sp, #112]                  @ 4-byte Reload
	adcs	r0, r0, r12
	str	r0, [sp, #116]                  @ 4-byte Spill
	ldr	r0, [sp, #108]                  @ 4-byte Reload
	adcs	r0, r0, lr
	str	r0, [sp, #112]                  @ 4-byte Spill
	ldr	r0, [sp, #104]                  @ 4-byte Reload
	adcs	r0, r0, r1
	str	r0, [sp, #108]                  @ 4-byte Spill
	ldr	r0, [sp, #100]                  @ 4-byte Reload
	ldr	r1, [sp, #152]                  @ 4-byte Reload
	adcs	r0, r0, r1
	str	r0, [sp, #104]                  @ 4-byte Spill
	mrs	r0, apsr
	str	r0, [sp, #152]                  @ 4-byte Spill
	ldr	r0, [sp, #156]                  @ 4-byte Reload
	msr	APSR_nzcvq, r0
	ldr	r1, [sp, #96]                   @ 4-byte Reload
	ldr	r3, [sp, #84]                   @ 4-byte Reload
	ldr	r0, [sp, #1240]
	adcs	r1, r1, r3
	str	r1, [sp, #100]                  @ 4-byte Spill
	ldr	r1, [sp, #92]                   @ 4-byte Reload
	ldr	r3, [sp, #88]                   @ 4-byte Reload
	ldr	r5, [sp, #228]                  @ 4-byte Reload
	adcs	r1, r3, r1
	str	r1, [sp, #96]                   @ 4-byte Spill
	adc	r0, r0, #0
	str	r0, [sp, #92]                   @ 4-byte Spill
	ldr	r0, [r11, #72]
	mul	r2, r5, r4
	str	r0, [sp, #88]                   @ 4-byte Spill
	ldr	r0, [sp, #1236]
	mov	r1, r8
	str	r0, [sp, #84]                   @ 4-byte Spill
	ldr	r0, [sp, #1232]
	str	r0, [sp, #80]                   @ 4-byte Spill
	add	r0, sp, #1104
	bl	mulPv512x32
	ldr	r7, [sp, #1104]
	ldr	r0, [sp, #1108]
	adds	r4, r4, r7
	ldr	r1, [sp, #1112]
	ldr	r4, [sp, #148]                  @ 4-byte Reload
	ldr	r10, [sp, #144]                 @ 4-byte Reload
	adcs	r9, r4, r0
	ldr	r11, [sp, #1116]
	ldr	r4, [sp, #140]                  @ 4-byte Reload
	adcs	r1, r10, r1
	ldr	r8, [sp, #1152]
	ldr	lr, [sp, #1148]
	adcs	r4, r4, r11
	ldr	r12, [sp, #1144]
	ldr	r3, [sp, #1140]
	ldr	r6, [sp, #1136]
	ldr	r2, [sp, #1132]
	ldr	r7, [sp, #1128]
	ldr	r0, [sp, #1124]
	str	r1, [sp, #148]                  @ 4-byte Spill
	ldr	r1, [sp, #1120]
	str	r4, [sp, #144]                  @ 4-byte Spill
	ldr	r4, [sp, #136]                  @ 4-byte Reload
	adcs	r1, r4, r1
	str	r1, [sp, #140]                  @ 4-byte Spill
	ldr	r1, [sp, #132]                  @ 4-byte Reload
	adcs	r0, r1, r0
	str	r0, [sp, #136]                  @ 4-byte Spill
	ldr	r0, [sp, #128]                  @ 4-byte Reload
	adcs	r0, r0, r7
	str	r0, [sp, #132]                  @ 4-byte Spill
	ldr	r0, [sp, #124]                  @ 4-byte Reload
	adcs	r0, r0, r2
	str	r0, [sp, #128]                  @ 4-byte Spill
	ldr	r0, [sp, #120]                  @ 4-byte Reload
	mul	r2, r5, r9
	adcs	r0, r0, r6
	str	r0, [sp, #124]                  @ 4-byte Spill
	ldr	r0, [sp, #116]                  @ 4-byte Reload
	adcs	r0, r0, r3
	str	r0, [sp, #120]                  @ 4-byte Spill
	ldr	r0, [sp, #112]                  @ 4-byte Reload
	adcs	r0, r0, r12
	str	r0, [sp, #116]                  @ 4-byte Spill
	ldr	r0, [sp, #108]                  @ 4-byte Reload
	adcs	r0, r0, lr
	str	r0, [sp, #112]                  @ 4-byte Spill
	ldr	r0, [sp, #104]                  @ 4-byte Reload
	add	lr, sp, #1024
	adcs	r0, r0, r8
	str	r0, [sp, #108]                  @ 4-byte Spill
	mrs	r0, apsr
	str	r0, [sp, #156]                  @ 4-byte Spill
	ldr	r0, [sp, #152]                  @ 4-byte Reload
	msr	APSR_nzcvq, r0
	ldr	r1, [sp, #100]                  @ 4-byte Reload
	ldr	r3, [sp, #80]                   @ 4-byte Reload
	ldr	r0, [sp, #1168]
	adcs	r1, r1, r3
	str	r1, [sp, #104]                  @ 4-byte Spill
	ldr	r1, [sp, #96]                   @ 4-byte Reload
	ldr	r3, [sp, #84]                   @ 4-byte Reload
	ldr	r10, [sp, #236]                 @ 4-byte Reload
	adcs	r1, r1, r3
	str	r1, [sp, #100]                  @ 4-byte Spill
	ldr	r1, [sp, #92]                   @ 4-byte Reload
	ldr	r3, [sp, #88]                   @ 4-byte Reload
	ldr	r7, [sp, #232]                  @ 4-byte Reload
	adcs	r1, r3, r1
	str	r1, [sp, #96]                   @ 4-byte Spill
	adc	r0, r0, #0
	str	r0, [sp, #92]                   @ 4-byte Spill
	ldr	r0, [r10, #76]
	mov	r1, r7
	str	r0, [sp, #88]                   @ 4-byte Spill
	ldr	r0, [sp, #1164]
	str	r0, [sp, #84]                   @ 4-byte Spill
	ldr	r0, [sp, #1160]
	str	r0, [sp, #80]                   @ 4-byte Spill
	ldr	r0, [sp, #1156]
	str	r0, [sp, #76]                   @ 4-byte Spill
	add	r0, lr, #8
	bl	mulPv512x32
	ldr	r0, [sp, #1076]
	add	lr, sp, #1056
	str	r0, [sp, #152]                  @ 4-byte Spill
	ldr	r5, [sp, #1032]
	ldr	r0, [sp, #1036]
	adds	r4, r9, r5
	ldr	r1, [sp, #1040]
	ldr	r4, [sp, #148]                  @ 4-byte Reload
	ldr	r6, [sp, #144]                  @ 4-byte Reload
	adcs	r4, r4, r0
	ldr	r11, [sp, #1072]
	adcs	r1, r6, r1
	ldr	r8, [sp, #1068]
	ldm	lr, {r3, r12, lr}
	ldr	r2, [sp, #1044]
	ldr	r5, [sp, #1052]
	ldr	r0, [sp, #1048]
	str	r1, [sp, #148]                  @ 4-byte Spill
	ldr	r1, [sp, #140]                  @ 4-byte Reload
	adcs	r1, r1, r2
	str	r1, [sp, #144]                  @ 4-byte Spill
	ldr	r1, [sp, #136]                  @ 4-byte Reload
	adcs	r0, r1, r0
	str	r0, [sp, #140]                  @ 4-byte Spill
	ldr	r0, [sp, #132]                  @ 4-byte Reload
	ldr	r1, [sp, #152]                  @ 4-byte Reload
	adcs	r0, r0, r5
	str	r0, [sp, #136]                  @ 4-byte Spill
	ldr	r0, [sp, #128]                  @ 4-byte Reload
	adcs	r0, r0, r3
	str	r0, [sp, #132]                  @ 4-byte Spill
	ldr	r0, [sp, #124]                  @ 4-byte Reload
	adcs	r0, r0, r12
	str	r0, [sp, #128]                  @ 4-byte Spill
	ldr	r0, [sp, #120]                  @ 4-byte Reload
	adcs	r0, r0, lr
	str	r0, [sp, #124]                  @ 4-byte Spill
	ldr	r0, [sp, #116]                  @ 4-byte Reload
	adcs	r0, r0, r8
	str	r0, [sp, #120]                  @ 4-byte Spill
	ldr	r0, [sp, #112]                  @ 4-byte Reload
	adcs	r0, r0, r11
	str	r0, [sp, #116]                  @ 4-byte Spill
	ldr	r0, [sp, #108]                  @ 4-byte Reload
	mov	r11, r10
	adcs	r0, r0, r1
	str	r0, [sp, #112]                  @ 4-byte Spill
	mrs	r0, apsr
	str	r0, [sp, #152]                  @ 4-byte Spill
	ldr	r0, [sp, #156]                  @ 4-byte Reload
	msr	APSR_nzcvq, r0
	ldr	r1, [sp, #104]                  @ 4-byte Reload
	ldr	r3, [sp, #76]                   @ 4-byte Reload
	ldr	r0, [sp, #1096]
	adcs	r1, r1, r3
	str	r1, [sp, #108]                  @ 4-byte Spill
	ldr	r1, [sp, #100]                  @ 4-byte Reload
	ldr	r3, [sp, #80]                   @ 4-byte Reload
	ldr	r6, [sp, #228]                  @ 4-byte Reload
	adcs	r1, r1, r3
	str	r1, [sp, #104]                  @ 4-byte Spill
	ldr	r1, [sp, #96]                   @ 4-byte Reload
	ldr	r3, [sp, #84]                   @ 4-byte Reload
	mul	r2, r6, r4
	adcs	r1, r1, r3
	str	r1, [sp, #100]                  @ 4-byte Spill
	ldr	r1, [sp, #92]                   @ 4-byte Reload
	ldr	r3, [sp, #88]                   @ 4-byte Reload
	adcs	r1, r3, r1
	str	r1, [sp, #96]                   @ 4-byte Spill
	adc	r0, r0, #0
	str	r0, [sp, #92]                   @ 4-byte Spill
	ldr	r0, [r10, #80]
	mov	r1, r7
	str	r0, [sp, #88]                   @ 4-byte Spill
	ldr	r0, [sp, #1092]
	str	r0, [sp, #84]                   @ 4-byte Spill
	ldr	r0, [sp, #1088]
	str	r0, [sp, #80]                   @ 4-byte Spill
	ldr	r0, [sp, #1084]
	str	r0, [sp, #76]                   @ 4-byte Spill
	ldr	r0, [sp, #1080]
	str	r0, [sp, #72]                   @ 4-byte Spill
	add	r0, sp, #960
	bl	mulPv512x32
	ldr	r5, [sp, #960]
	add	r2, sp, #964
	add	lr, sp, #984
	ldr	r9, [sp, #1000]
	adds	r4, r4, r5
	ldm	r2, {r0, r1, r2}
	ldr	r4, [sp, #148]                  @ 4-byte Reload
	ldr	r8, [sp, #996]
	adcs	r10, r4, r0
	ldr	r0, [sp, #144]                  @ 4-byte Reload
	ldm	lr, {r3, r12, lr}
	adcs	r0, r0, r1
	ldr	r7, [sp, #980]
	ldr	r5, [sp, #976]
	str	r0, [sp, #148]                  @ 4-byte Spill
	ldr	r0, [sp, #140]                  @ 4-byte Reload
	adcs	r0, r0, r2
	str	r0, [sp, #144]                  @ 4-byte Spill
	ldr	r0, [sp, #136]                  @ 4-byte Reload
	mul	r2, r6, r10
	adcs	r0, r0, r5
	str	r0, [sp, #140]                  @ 4-byte Spill
	ldr	r0, [sp, #132]                  @ 4-byte Reload
	adcs	r0, r0, r7
	str	r0, [sp, #136]                  @ 4-byte Spill
	ldr	r0, [sp, #128]                  @ 4-byte Reload
	adcs	r0, r0, r3
	str	r0, [sp, #132]                  @ 4-byte Spill
	ldr	r0, [sp, #124]                  @ 4-byte Reload
	adcs	r0, r0, r12
	str	r0, [sp, #128]                  @ 4-byte Spill
	ldr	r0, [sp, #120]                  @ 4-byte Reload
	adcs	r0, r0, lr
	str	r0, [sp, #124]                  @ 4-byte Spill
	ldr	r0, [sp, #116]                  @ 4-byte Reload
	adcs	r0, r0, r8
	str	r0, [sp, #120]                  @ 4-byte Spill
	ldr	r0, [sp, #112]                  @ 4-byte Reload
	adcs	r0, r0, r9
	str	r0, [sp, #116]                  @ 4-byte Spill
	mrs	r0, apsr
	mov	r9, r6
	str	r0, [sp, #156]                  @ 4-byte Spill
	ldr	r0, [sp, #152]                  @ 4-byte Reload
	msr	APSR_nzcvq, r0
	ldr	r1, [sp, #108]                  @ 4-byte Reload
	ldr	r3, [sp, #72]                   @ 4-byte Reload
	ldr	r0, [sp, #1024]
	adcs	r1, r1, r3
	str	r1, [sp, #112]                  @ 4-byte Spill
	ldr	r1, [sp, #104]                  @ 4-byte Reload
	ldr	r3, [sp, #76]                   @ 4-byte Reload
	ldr	r4, [sp, #232]                  @ 4-byte Reload
	adcs	r1, r1, r3
	str	r1, [sp, #108]                  @ 4-byte Spill
	ldr	r1, [sp, #100]                  @ 4-byte Reload
	ldr	r3, [sp, #80]                   @ 4-byte Reload
	adcs	r1, r1, r3
	str	r1, [sp, #104]                  @ 4-byte Spill
	ldr	r1, [sp, #96]                   @ 4-byte Reload
	ldr	r3, [sp, #84]                   @ 4-byte Reload
	adcs	r1, r1, r3
	str	r1, [sp, #100]                  @ 4-byte Spill
	ldr	r1, [sp, #92]                   @ 4-byte Reload
	ldr	r3, [sp, #88]                   @ 4-byte Reload
	adcs	r1, r3, r1
	str	r1, [sp, #96]                   @ 4-byte Spill
	adc	r0, r0, #0
	str	r0, [sp, #92]                   @ 4-byte Spill
	ldr	r0, [r11, #84]
	mov	r1, r4
	str	r0, [sp, #88]                   @ 4-byte Spill
	ldr	r0, [sp, #1020]
	str	r0, [sp, #84]                   @ 4-byte Spill
	ldr	r0, [sp, #1016]
	str	r0, [sp, #80]                   @ 4-byte Spill
	ldr	r0, [sp, #1012]
	str	r0, [sp, #76]                   @ 4-byte Spill
	ldr	r0, [sp, #1008]
	str	r0, [sp, #72]                   @ 4-byte Spill
	add	r0, sp, #888
	ldr	r11, [sp, #1004]
	bl	mulPv512x32
	ldr	r6, [sp, #888]
	add	r7, sp, #892
	add	lr, sp, #912
	ldr	r8, [sp, #924]
	adds	r6, r10, r6
	ldm	r7, {r0, r1, r2, r5, r7}
	ldr	r6, [sp, #148]                  @ 4-byte Reload
	ldm	lr, {r3, r12, lr}
	adcs	r10, r6, r0
	ldr	r0, [sp, #144]                  @ 4-byte Reload
	adcs	r0, r0, r1
	str	r0, [sp, #148]                  @ 4-byte Spill
	ldr	r0, [sp, #140]                  @ 4-byte Reload
	adcs	r0, r0, r2
	str	r0, [sp, #144]                  @ 4-byte Spill
	ldr	r0, [sp, #136]                  @ 4-byte Reload
	mul	r2, r9, r10
	adcs	r0, r0, r5
	str	r0, [sp, #140]                  @ 4-byte Spill
	ldr	r0, [sp, #132]                  @ 4-byte Reload
	adcs	r0, r0, r7
	str	r0, [sp, #136]                  @ 4-byte Spill
	ldr	r0, [sp, #128]                  @ 4-byte Reload
	adcs	r0, r0, r3
	str	r0, [sp, #132]                  @ 4-byte Spill
	ldr	r0, [sp, #124]                  @ 4-byte Reload
	adcs	r0, r0, r12
	str	r0, [sp, #128]                  @ 4-byte Spill
	ldr	r0, [sp, #120]                  @ 4-byte Reload
	adcs	r0, r0, lr
	str	r0, [sp, #124]                  @ 4-byte Spill
	ldr	r0, [sp, #116]                  @ 4-byte Reload
	adcs	r0, r0, r8
	str	r0, [sp, #120]                  @ 4-byte Spill
	mrs	r0, apsr
	str	r0, [sp, #152]                  @ 4-byte Spill
	ldr	r0, [sp, #156]                  @ 4-byte Reload
	msr	APSR_nzcvq, r0
	ldr	r1, [sp, #112]                  @ 4-byte Reload
	ldr	r0, [sp, #952]
	adcs	r1, r1, r11
	str	r1, [sp, #116]                  @ 4-byte Spill
	ldr	r1, [sp, #108]                  @ 4-byte Reload
	ldr	r3, [sp, #72]                   @ 4-byte Reload
	ldr	r6, [sp, #236]                  @ 4-byte Reload
	adcs	r1, r1, r3
	str	r1, [sp, #112]                  @ 4-byte Spill
	ldr	r1, [sp, #104]                  @ 4-byte Reload
	ldr	r3, [sp, #76]                   @ 4-byte Reload
	adcs	r1, r1, r3
	str	r1, [sp, #108]                  @ 4-byte Spill
	ldr	r1, [sp, #100]                  @ 4-byte Reload
	ldr	r3, [sp, #80]                   @ 4-byte Reload
	adcs	r1, r1, r3
	str	r1, [sp, #104]                  @ 4-byte Spill
	ldr	r1, [sp, #96]                   @ 4-byte Reload
	ldr	r3, [sp, #84]                   @ 4-byte Reload
	adcs	r1, r1, r3
	str	r1, [sp, #100]                  @ 4-byte Spill
	ldr	r1, [sp, #92]                   @ 4-byte Reload
	ldr	r3, [sp, #88]                   @ 4-byte Reload
	adcs	r1, r3, r1
	str	r1, [sp, #96]                   @ 4-byte Spill
	adc	r0, r0, #0
	str	r0, [sp, #92]                   @ 4-byte Spill
	ldr	r0, [r6, #88]
	mov	r1, r4
	str	r0, [sp, #88]                   @ 4-byte Spill
	ldr	r0, [sp, #948]
	str	r0, [sp, #84]                   @ 4-byte Spill
	ldr	r0, [sp, #944]
	str	r0, [sp, #80]                   @ 4-byte Spill
	ldr	r0, [sp, #940]
	str	r0, [sp, #76]                   @ 4-byte Spill
	ldr	r0, [sp, #936]
	str	r0, [sp, #72]                   @ 4-byte Spill
	add	r0, sp, #816
	ldr	r11, [sp, #932]
	ldr	r8, [sp, #928]
	bl	mulPv512x32
	ldr	r4, [sp, #816]
	add	r5, sp, #824
	ldr	r7, [sp, #820]
	adds	r4, r10, r4
	ldm	r5, {r0, r1, r5}
	ldr	r4, [sp, #148]                  @ 4-byte Reload
	ldr	r12, [sp, #848]
	adcs	r4, r4, r7
	ldr	r7, [sp, #144]                  @ 4-byte Reload
	ldr	lr, [sp, #844]
	adcs	r0, r7, r0
	ldr	r2, [sp, #840]
	ldr	r3, [sp, #836]
	str	r0, [sp, #148]                  @ 4-byte Spill
	ldr	r0, [sp, #140]                  @ 4-byte Reload
	adcs	r0, r0, r1
	str	r0, [sp, #144]                  @ 4-byte Spill
	ldr	r0, [sp, #136]                  @ 4-byte Reload
	adcs	r0, r0, r5
	str	r0, [sp, #140]                  @ 4-byte Spill
	ldr	r0, [sp, #132]                  @ 4-byte Reload
	adcs	r0, r0, r3
	str	r0, [sp, #136]                  @ 4-byte Spill
	ldr	r0, [sp, #128]                  @ 4-byte Reload
	adcs	r0, r0, r2
	str	r0, [sp, #132]                  @ 4-byte Spill
	ldr	r0, [sp, #124]                  @ 4-byte Reload
	mul	r2, r9, r4
	adcs	r0, r0, lr
	str	r0, [sp, #128]                  @ 4-byte Spill
	ldr	r0, [sp, #120]                  @ 4-byte Reload
	adcs	r0, r0, r12
	str	r0, [sp, #124]                  @ 4-byte Spill
	mrs	r0, apsr
	str	r0, [sp, #156]                  @ 4-byte Spill
	ldr	r0, [sp, #152]                  @ 4-byte Reload
	msr	APSR_nzcvq, r0
	ldr	r1, [sp, #116]                  @ 4-byte Reload
	ldr	r0, [sp, #880]
	adcs	r1, r1, r8
	str	r1, [sp, #120]                  @ 4-byte Spill
	ldr	r1, [sp, #112]                  @ 4-byte Reload
	ldr	r3, [sp, #72]                   @ 4-byte Reload
	adcs	r1, r1, r11
	str	r1, [sp, #116]                  @ 4-byte Spill
	ldr	r1, [sp, #108]                  @ 4-byte Reload
	ldr	r11, [sp, #232]                 @ 4-byte Reload
	adcs	r1, r1, r3
	str	r1, [sp, #112]                  @ 4-byte Spill
	ldr	r1, [sp, #104]                  @ 4-byte Reload
	ldr	r3, [sp, #76]                   @ 4-byte Reload
	adcs	r1, r1, r3
	str	r1, [sp, #108]                  @ 4-byte Spill
	ldr	r1, [sp, #100]                  @ 4-byte Reload
	ldr	r3, [sp, #80]                   @ 4-byte Reload
	adcs	r1, r1, r3
	str	r1, [sp, #104]                  @ 4-byte Spill
	ldr	r1, [sp, #96]                   @ 4-byte Reload
	ldr	r3, [sp, #84]                   @ 4-byte Reload
	adcs	r1, r1, r3
	str	r1, [sp, #100]                  @ 4-byte Spill
	ldr	r1, [sp, #92]                   @ 4-byte Reload
	ldr	r3, [sp, #88]                   @ 4-byte Reload
	adcs	r1, r3, r1
	str	r1, [sp, #96]                   @ 4-byte Spill
	adc	r0, r0, #0
	str	r0, [sp, #92]                   @ 4-byte Spill
	ldr	r0, [r6, #92]
	mov	r1, r11
	str	r0, [sp, #88]                   @ 4-byte Spill
	ldr	r0, [sp, #876]
	str	r0, [sp, #84]                   @ 4-byte Spill
	ldr	r0, [sp, #872]
	str	r0, [sp, #80]                   @ 4-byte Spill
	ldr	r0, [sp, #868]
	str	r0, [sp, #76]                   @ 4-byte Spill
	ldr	r0, [sp, #864]
	str	r0, [sp, #72]                   @ 4-byte Spill
	ldr	r0, [sp, #860]
	str	r0, [sp, #68]                   @ 4-byte Spill
	ldr	r0, [sp, #852]
	ldr	r10, [sp, #856]
	str	r0, [sp, #64]                   @ 4-byte Spill
	add	r0, sp, #744
	bl	mulPv512x32
	add	r7, sp, #744
	ldr	r12, [sp, #772]
	ldr	r1, [sp, #768]
	ldm	r7, {r5, r6, r7}
	adds	r5, r4, r5
	ldr	r2, [sp, #764]
	ldr	r5, [sp, #148]                  @ 4-byte Reload
	ldr	r3, [sp, #760]
	adcs	r8, r5, r6
	ldr	r6, [sp, #144]                  @ 4-byte Reload
	ldr	r0, [sp, #756]
	adcs	r7, r6, r7
	str	r7, [sp, #148]                  @ 4-byte Spill
	ldr	r7, [sp, #140]                  @ 4-byte Reload
	adcs	r0, r7, r0
	str	r0, [sp, #144]                  @ 4-byte Spill
	ldr	r0, [sp, #136]                  @ 4-byte Reload
	adcs	r0, r0, r3
	str	r0, [sp, #140]                  @ 4-byte Spill
	ldr	r0, [sp, #132]                  @ 4-byte Reload
	adcs	r0, r0, r2
	str	r0, [sp, #136]                  @ 4-byte Spill
	ldr	r0, [sp, #128]                  @ 4-byte Reload
	mul	r2, r9, r8
	adcs	r0, r0, r1
	str	r0, [sp, #132]                  @ 4-byte Spill
	ldr	r0, [sp, #124]                  @ 4-byte Reload
	adcs	r0, r0, r12
	str	r0, [sp, #128]                  @ 4-byte Spill
	mrs	r0, apsr
	str	r0, [sp, #152]                  @ 4-byte Spill
	ldr	r0, [sp, #156]                  @ 4-byte Reload
	msr	APSR_nzcvq, r0
	ldr	r1, [sp, #120]                  @ 4-byte Reload
	ldr	r3, [sp, #64]                   @ 4-byte Reload
	ldr	r0, [sp, #808]
	adcs	r1, r1, r3
	str	r1, [sp, #124]                  @ 4-byte Spill
	ldr	r1, [sp, #116]                  @ 4-byte Reload
	ldr	r3, [sp, #68]                   @ 4-byte Reload
	adcs	r1, r1, r10
	str	r1, [sp, #120]                  @ 4-byte Spill
	ldr	r1, [sp, #112]                  @ 4-byte Reload
	ldr	r10, [sp, #236]                 @ 4-byte Reload
	adcs	r1, r1, r3
	str	r1, [sp, #116]                  @ 4-byte Spill
	ldr	r1, [sp, #108]                  @ 4-byte Reload
	ldr	r3, [sp, #72]                   @ 4-byte Reload
	adcs	r1, r1, r3
	str	r1, [sp, #112]                  @ 4-byte Spill
	ldr	r1, [sp, #104]                  @ 4-byte Reload
	ldr	r3, [sp, #76]                   @ 4-byte Reload
	adcs	r1, r1, r3
	str	r1, [sp, #108]                  @ 4-byte Spill
	ldr	r1, [sp, #100]                  @ 4-byte Reload
	ldr	r3, [sp, #80]                   @ 4-byte Reload
	adcs	r1, r1, r3
	str	r1, [sp, #104]                  @ 4-byte Spill
	ldr	r1, [sp, #96]                   @ 4-byte Reload
	ldr	r3, [sp, #84]                   @ 4-byte Reload
	adcs	r1, r1, r3
	str	r1, [sp, #100]                  @ 4-byte Spill
	ldr	r1, [sp, #92]                   @ 4-byte Reload
	ldr	r3, [sp, #88]                   @ 4-byte Reload
	adcs	r1, r3, r1
	str	r1, [sp, #96]                   @ 4-byte Spill
	adc	r0, r0, #0
	str	r0, [sp, #92]                   @ 4-byte Spill
	ldr	r0, [r10, #96]
	mov	r1, r11
	str	r0, [sp, #88]                   @ 4-byte Spill
	ldr	r0, [sp, #804]
	str	r0, [sp, #84]                   @ 4-byte Spill
	ldr	r0, [sp, #800]
	str	r0, [sp, #80]                   @ 4-byte Spill
	ldr	r0, [sp, #796]
	str	r0, [sp, #76]                   @ 4-byte Spill
	ldr	r0, [sp, #792]
	str	r0, [sp, #72]                   @ 4-byte Spill
	ldr	r0, [sp, #788]
	str	r0, [sp, #68]                   @ 4-byte Spill
	ldr	r0, [sp, #784]
	str	r0, [sp, #64]                   @ 4-byte Spill
	ldr	r0, [sp, #780]
	str	r0, [sp, #60]                   @ 4-byte Spill
	add	r0, sp, #672
	ldr	r9, [sp, #776]
	bl	mulPv512x32
	add	r7, sp, #672
	ldr	r0, [sp, #696]
	ldr	r1, [sp, #692]
	ldm	r7, {r3, r5, r6, r7}
	adds	r3, r8, r3
	ldr	r2, [sp, #688]
	ldr	r3, [sp, #148]                  @ 4-byte Reload
	adcs	r8, r3, r5
	ldr	r3, [sp, #144]                  @ 4-byte Reload
	adcs	r3, r3, r6
	str	r3, [sp, #148]                  @ 4-byte Spill
	ldr	r3, [sp, #140]                  @ 4-byte Reload
	adcs	r3, r3, r7
	str	r3, [sp, #144]                  @ 4-byte Spill
	ldr	r3, [sp, #136]                  @ 4-byte Reload
	adcs	r2, r3, r2
	str	r2, [sp, #140]                  @ 4-byte Spill
	ldr	r2, [sp, #132]                  @ 4-byte Reload
	adcs	r1, r2, r1
	str	r1, [sp, #136]                  @ 4-byte Spill
	ldr	r1, [sp, #128]                  @ 4-byte Reload
	adcs	r0, r1, r0
	str	r0, [sp, #132]                  @ 4-byte Spill
	mrs	r0, apsr
	str	r0, [sp, #156]                  @ 4-byte Spill
	ldr	r0, [sp, #152]                  @ 4-byte Reload
	msr	APSR_nzcvq, r0
	ldr	r1, [sp, #124]                  @ 4-byte Reload
	ldr	r0, [sp, #736]
	adcs	r1, r1, r9
	str	r1, [sp, #128]                  @ 4-byte Spill
	ldr	r1, [sp, #120]                  @ 4-byte Reload
	ldr	r3, [sp, #60]                   @ 4-byte Reload
	ldr	r4, [sp, #228]                  @ 4-byte Reload
	adcs	r1, r1, r3
	str	r1, [sp, #124]                  @ 4-byte Spill
	ldr	r1, [sp, #116]                  @ 4-byte Reload
	ldr	r3, [sp, #64]                   @ 4-byte Reload
	mul	r2, r4, r8
	ldr	r7, [sp, #232]                  @ 4-byte Reload
	adcs	r1, r1, r3
	str	r1, [sp, #120]                  @ 4-byte Spill
	ldr	r1, [sp, #112]                  @ 4-byte Reload
	ldr	r3, [sp, #68]                   @ 4-byte Reload
	adcs	r1, r1, r3
	str	r1, [sp, #116]                  @ 4-byte Spill
	ldr	r1, [sp, #108]                  @ 4-byte Reload
	ldr	r3, [sp, #72]                   @ 4-byte Reload
	adcs	r1, r1, r3
	str	r1, [sp, #112]                  @ 4-byte Spill
	ldr	r1, [sp, #104]                  @ 4-byte Reload
	ldr	r3, [sp, #76]                   @ 4-byte Reload
	adcs	r1, r1, r3
	str	r1, [sp, #108]                  @ 4-byte Spill
	ldr	r1, [sp, #100]                  @ 4-byte Reload
	ldr	r3, [sp, #80]                   @ 4-byte Reload
	adcs	r1, r1, r3
	str	r1, [sp, #104]                  @ 4-byte Spill
	ldr	r1, [sp, #96]                   @ 4-byte Reload
	ldr	r3, [sp, #84]                   @ 4-byte Reload
	adcs	r1, r1, r3
	str	r1, [sp, #100]                  @ 4-byte Spill
	ldr	r1, [sp, #92]                   @ 4-byte Reload
	ldr	r3, [sp, #88]                   @ 4-byte Reload
	adcs	r1, r3, r1
	str	r1, [sp, #96]                   @ 4-byte Spill
	adc	r0, r0, #0
	str	r0, [sp, #92]                   @ 4-byte Spill
	ldr	r0, [r10, #100]
	mov	r1, r7
	str	r0, [sp, #88]                   @ 4-byte Spill
	ldr	r0, [sp, #732]
	str	r0, [sp, #84]                   @ 4-byte Spill
	ldr	r0, [sp, #728]
	str	r0, [sp, #80]                   @ 4-byte Spill
	ldr	r0, [sp, #724]
	str	r0, [sp, #76]                   @ 4-byte Spill
	ldr	r0, [sp, #720]
	str	r0, [sp, #72]                   @ 4-byte Spill
	ldr	r0, [sp, #716]
	str	r0, [sp, #68]                   @ 4-byte Spill
	ldr	r0, [sp, #712]
	str	r0, [sp, #64]                   @ 4-byte Spill
	ldr	r0, [sp, #704]
	ldr	r10, [sp, #708]
	str	r0, [sp, #60]                   @ 4-byte Spill
	add	r0, sp, #600
	ldr	r9, [sp, #700]
	bl	mulPv512x32
	add	r6, sp, #600
	ldr	r0, [sp, #620]
	ldr	r1, [sp, #616]
	ldm	r6, {r2, r3, r5, r6}
	adds	r2, r8, r2
	ldr	r2, [sp, #148]                  @ 4-byte Reload
	adcs	r11, r2, r3
	ldr	r2, [sp, #144]                  @ 4-byte Reload
	adcs	r2, r2, r5
	str	r2, [sp, #148]                  @ 4-byte Spill
	ldr	r2, [sp, #140]                  @ 4-byte Reload
	adcs	r2, r2, r6
	str	r2, [sp, #144]                  @ 4-byte Spill
	ldr	r2, [sp, #136]                  @ 4-byte Reload
	adcs	r1, r2, r1
	str	r1, [sp, #140]                  @ 4-byte Spill
	ldr	r1, [sp, #132]                  @ 4-byte Reload
	mul	r2, r4, r11
	adcs	r0, r1, r0
	str	r0, [sp, #136]                  @ 4-byte Spill
	mrs	r0, apsr
	str	r0, [sp, #152]                  @ 4-byte Spill
	ldr	r0, [sp, #156]                  @ 4-byte Reload
	msr	APSR_nzcvq, r0
	ldr	r1, [sp, #128]                  @ 4-byte Reload
	ldr	r0, [sp, #664]
	adcs	r1, r1, r9
	str	r1, [sp, #156]                  @ 4-byte Spill
	ldr	r1, [sp, #124]                  @ 4-byte Reload
	ldr	r3, [sp, #60]                   @ 4-byte Reload
	adcs	r1, r1, r3
	str	r1, [sp, #132]                  @ 4-byte Spill
	ldr	r1, [sp, #120]                  @ 4-byte Reload
	ldr	r3, [sp, #64]                   @ 4-byte Reload
	adcs	r1, r1, r10
	str	r1, [sp, #128]                  @ 4-byte Spill
	ldr	r1, [sp, #116]                  @ 4-byte Reload
	ldr	r10, [sp, #236]                 @ 4-byte Reload
	adcs	r1, r1, r3
	str	r1, [sp, #124]                  @ 4-byte Spill
	ldr	r1, [sp, #112]                  @ 4-byte Reload
	ldr	r3, [sp, #68]                   @ 4-byte Reload
	adcs	r1, r1, r3
	str	r1, [sp, #120]                  @ 4-byte Spill
	ldr	r1, [sp, #108]                  @ 4-byte Reload
	ldr	r3, [sp, #72]                   @ 4-byte Reload
	adcs	r1, r1, r3
	str	r1, [sp, #116]                  @ 4-byte Spill
	ldr	r1, [sp, #104]                  @ 4-byte Reload
	ldr	r3, [sp, #76]                   @ 4-byte Reload
	adcs	r1, r1, r3
	str	r1, [sp, #112]                  @ 4-byte Spill
	ldr	r1, [sp, #100]                  @ 4-byte Reload
	ldr	r3, [sp, #80]                   @ 4-byte Reload
	adcs	r1, r1, r3
	str	r1, [sp, #108]                  @ 4-byte Spill
	ldr	r1, [sp, #96]                   @ 4-byte Reload
	ldr	r3, [sp, #84]                   @ 4-byte Reload
	adcs	r1, r1, r3
	str	r1, [sp, #104]                  @ 4-byte Spill
	ldr	r1, [sp, #92]                   @ 4-byte Reload
	ldr	r3, [sp, #88]                   @ 4-byte Reload
	adcs	r1, r3, r1
	str	r1, [sp, #100]                  @ 4-byte Spill
	adc	r0, r0, #0
	str	r0, [sp, #96]                   @ 4-byte Spill
	ldr	r0, [r10, #104]
	mov	r1, r7
	str	r0, [sp, #80]                   @ 4-byte Spill
	ldr	r0, [sp, #660]
	str	r0, [sp, #76]                   @ 4-byte Spill
	ldr	r0, [sp, #656]
	str	r0, [sp, #72]                   @ 4-byte Spill
	ldr	r0, [sp, #652]
	str	r0, [sp, #68]                   @ 4-byte Spill
	ldr	r0, [sp, #648]
	str	r0, [sp, #64]                   @ 4-byte Spill
	ldr	r0, [sp, #644]
	str	r0, [sp, #60]                   @ 4-byte Spill
	ldr	r0, [sp, #640]
	str	r0, [sp, #56]                   @ 4-byte Spill
	ldr	r0, [sp, #636]
	str	r0, [sp, #52]                   @ 4-byte Spill
	add	r0, sp, #528
	ldr	r8, [sp, #632]
	ldr	r9, [sp, #628]
	ldr	r5, [sp, #624]
	bl	mulPv512x32
	add	r6, sp, #528
	ldr	r0, [sp, #544]
	ldm	r6, {r1, r2, r3, r6}
	adds	r1, r11, r1
	ldr	r1, [sp, #148]                  @ 4-byte Reload
	adcs	r4, r1, r2
	ldr	r1, [sp, #144]                  @ 4-byte Reload
	adcs	r1, r1, r3
	str	r1, [sp, #92]                   @ 4-byte Spill
	ldr	r1, [sp, #140]                  @ 4-byte Reload
	adcs	r1, r1, r6
	str	r1, [sp, #88]                   @ 4-byte Spill
	ldr	r1, [sp, #136]                  @ 4-byte Reload
	adcs	r0, r1, r0
	str	r0, [sp, #84]                   @ 4-byte Spill
	mrs	r0, apsr
	str	r0, [sp, #148]                  @ 4-byte Spill
	ldr	r0, [sp, #152]                  @ 4-byte Reload
	msr	APSR_nzcvq, r0
	ldr	r1, [sp, #156]                  @ 4-byte Reload
	ldr	r0, [sp, #592]
	adcs	r1, r1, r5
	str	r1, [sp, #152]                  @ 4-byte Spill
	ldr	r1, [sp, #132]                  @ 4-byte Reload
	ldr	r3, [sp, #52]                   @ 4-byte Reload
	adcs	r1, r1, r9
	str	r1, [sp, #144]                  @ 4-byte Spill
	ldr	r1, [sp, #128]                  @ 4-byte Reload
	ldr	r7, [sp, #228]                  @ 4-byte Reload
	adcs	r1, r1, r8
	str	r1, [sp, #140]                  @ 4-byte Spill
	ldr	r1, [sp, #124]                  @ 4-byte Reload
	mul	r2, r7, r4
	ldr	r6, [sp, #232]                  @ 4-byte Reload
	adcs	r1, r1, r3
	str	r1, [sp, #136]                  @ 4-byte Spill
	ldr	r1, [sp, #120]                  @ 4-byte Reload
	ldr	r3, [sp, #56]                   @ 4-byte Reload
	adcs	r1, r1, r3
	str	r1, [sp, #132]                  @ 4-byte Spill
	ldr	r1, [sp, #116]                  @ 4-byte Reload
	ldr	r3, [sp, #60]                   @ 4-byte Reload
	adcs	r1, r1, r3
	str	r1, [sp, #128]                  @ 4-byte Spill
	ldr	r1, [sp, #112]                  @ 4-byte Reload
	ldr	r3, [sp, #64]                   @ 4-byte Reload
	adcs	r1, r1, r3
	str	r1, [sp, #124]                  @ 4-byte Spill
	ldr	r1, [sp, #108]                  @ 4-byte Reload
	ldr	r3, [sp, #68]                   @ 4-byte Reload
	adcs	r1, r1, r3
	str	r1, [sp, #120]                  @ 4-byte Spill
	ldr	r1, [sp, #104]                  @ 4-byte Reload
	ldr	r3, [sp, #72]                   @ 4-byte Reload
	adcs	r1, r1, r3
	str	r1, [sp, #116]                  @ 4-byte Spill
	ldr	r1, [sp, #100]                  @ 4-byte Reload
	ldr	r3, [sp, #76]                   @ 4-byte Reload
	adcs	r1, r1, r3
	str	r1, [sp, #112]                  @ 4-byte Spill
	ldr	r1, [sp, #96]                   @ 4-byte Reload
	ldr	r3, [sp, #80]                   @ 4-byte Reload
	adcs	r1, r3, r1
	str	r1, [sp, #108]                  @ 4-byte Spill
	adc	r0, r0, #0
	str	r0, [sp, #104]                  @ 4-byte Spill
	ldr	r0, [r10, #108]
	mov	r1, r6
	str	r0, [sp, #100]                  @ 4-byte Spill
	ldr	r0, [sp, #588]
	str	r0, [sp, #80]                   @ 4-byte Spill
	ldr	r0, [sp, #584]
	str	r0, [sp, #76]                   @ 4-byte Spill
	ldr	r0, [sp, #580]
	str	r0, [sp, #72]                   @ 4-byte Spill
	ldr	r0, [sp, #576]
	str	r0, [sp, #68]                   @ 4-byte Spill
	ldr	r0, [sp, #572]
	str	r0, [sp, #64]                   @ 4-byte Spill
	ldr	r0, [sp, #568]
	str	r0, [sp, #60]                   @ 4-byte Spill
	ldr	r0, [sp, #564]
	str	r0, [sp, #56]                   @ 4-byte Spill
	add	r0, sp, #456
	ldr	r11, [sp, #560]
	ldr	r8, [sp, #556]
	ldr	r9, [sp, #552]
	ldr	r5, [sp, #548]
	bl	mulPv512x32
	add	r3, sp, #456
	ldm	r3, {r0, r1, r2, r3}
	adds	r0, r4, r0
	ldr	r0, [sp, #92]                   @ 4-byte Reload
	adcs	r4, r0, r1
	ldr	r0, [sp, #88]                   @ 4-byte Reload
	adcs	r0, r0, r2
	str	r0, [sp, #96]                   @ 4-byte Spill
	ldr	r0, [sp, #84]                   @ 4-byte Reload
	mul	r2, r7, r4
	adcs	r0, r0, r3
	str	r0, [sp, #92]                   @ 4-byte Spill
	mrs	r0, apsr
	str	r0, [sp, #156]                  @ 4-byte Spill
	ldr	r0, [sp, #148]                  @ 4-byte Reload
	msr	APSR_nzcvq, r0
	ldr	r3, [sp, #152]                  @ 4-byte Reload
	ldr	r1, [sp, #520]
	add	r0, sp, #384
	adcs	r3, r3, r5
	str	r3, [sp, #152]                  @ 4-byte Spill
	ldr	r3, [sp, #144]                  @ 4-byte Reload
	ldr	r7, [sp, #56]                   @ 4-byte Reload
	adcs	r3, r3, r9
	str	r3, [sp, #148]                  @ 4-byte Spill
	ldr	r3, [sp, #140]                  @ 4-byte Reload
	adcs	r3, r3, r8
	str	r3, [sp, #144]                  @ 4-byte Spill
	ldr	r3, [sp, #136]                  @ 4-byte Reload
	adcs	r3, r3, r11
	str	r3, [sp, #140]                  @ 4-byte Spill
	ldr	r3, [sp, #132]                  @ 4-byte Reload
	adcs	r3, r3, r7
	str	r3, [sp, #136]                  @ 4-byte Spill
	ldr	r3, [sp, #128]                  @ 4-byte Reload
	ldr	r7, [sp, #60]                   @ 4-byte Reload
	adcs	r3, r3, r7
	str	r3, [sp, #132]                  @ 4-byte Spill
	ldr	r3, [sp, #124]                  @ 4-byte Reload
	ldr	r7, [sp, #64]                   @ 4-byte Reload
	adcs	r3, r3, r7
	str	r3, [sp, #128]                  @ 4-byte Spill
	ldr	r3, [sp, #120]                  @ 4-byte Reload
	ldr	r7, [sp, #68]                   @ 4-byte Reload
	adcs	r3, r3, r7
	str	r3, [sp, #124]                  @ 4-byte Spill
	ldr	r3, [sp, #116]                  @ 4-byte Reload
	ldr	r7, [sp, #72]                   @ 4-byte Reload
	adcs	r3, r3, r7
	str	r3, [sp, #120]                  @ 4-byte Spill
	ldr	r3, [sp, #112]                  @ 4-byte Reload
	ldr	r7, [sp, #76]                   @ 4-byte Reload
	adcs	r3, r3, r7
	str	r3, [sp, #116]                  @ 4-byte Spill
	ldr	r3, [sp, #108]                  @ 4-byte Reload
	ldr	r7, [sp, #80]                   @ 4-byte Reload
	adcs	r3, r3, r7
	str	r3, [sp, #112]                  @ 4-byte Spill
	ldr	r3, [sp, #104]                  @ 4-byte Reload
	ldr	r7, [sp, #100]                  @ 4-byte Reload
	adcs	r3, r7, r3
	str	r3, [sp, #108]                  @ 4-byte Spill
	adc	r1, r1, #0
	str	r1, [sp, #104]                  @ 4-byte Spill
	ldr	r1, [r10, #112]
	mov	r7, r6
	str	r1, [sp, #88]                   @ 4-byte Spill
	ldr	r1, [sp, #516]
	str	r1, [sp, #80]                   @ 4-byte Spill
	ldr	r1, [sp, #512]
	str	r1, [sp, #72]                   @ 4-byte Spill
	ldr	r1, [sp, #508]
	str	r1, [sp, #68]                   @ 4-byte Spill
	ldr	r1, [sp, #504]
	str	r1, [sp, #64]                   @ 4-byte Spill
	ldr	r1, [sp, #500]
	str	r1, [sp, #56]                   @ 4-byte Spill
	ldr	r1, [sp, #496]
	str	r1, [sp, #52]                   @ 4-byte Spill
	ldr	r1, [sp, #492]
	str	r1, [sp, #48]                   @ 4-byte Spill
	mov	r1, r6
	ldr	r8, [sp, #488]
	ldr	r9, [sp, #484]
	ldr	r10, [sp, #480]
	ldr	r5, [sp, #476]
	ldr	r11, [sp, #472]
	bl	mulPv512x32
	add	r3, sp, #384
	ldm	r3, {r0, r1, r2, r3}
	adds	r0, r4, r0
	str	r3, [sp, #100]                  @ 4-byte Spill
	ldr	r0, [sp, #96]                   @ 4-byte Reload
	adcs	r4, r0, r1
	ldr	r0, [sp, #92]                   @ 4-byte Reload
	adcs	r0, r0, r2
	str	r0, [sp, #76]                   @ 4-byte Spill
	mrs	r0, apsr
	str	r0, [sp, #84]                   @ 4-byte Spill
	ldr	r0, [sp, #156]                  @ 4-byte Reload
	msr	APSR_nzcvq, r0
	ldr	r3, [sp, #152]                  @ 4-byte Reload
	ldr	r1, [sp, #448]
	add	r0, sp, #312
	adcs	r3, r3, r11
	str	r3, [sp, #60]                   @ 4-byte Spill
	ldr	r3, [sp, #148]                  @ 4-byte Reload
	ldr	r6, [sp, #228]                  @ 4-byte Reload
	adcs	r11, r3, r5
	ldr	r3, [sp, #144]                  @ 4-byte Reload
	ldr	r5, [sp, #48]                   @ 4-byte Reload
	adcs	r10, r3, r10
	ldr	r3, [sp, #140]                  @ 4-byte Reload
	mul	r2, r6, r4
	adcs	r3, r3, r9
	str	r3, [sp, #152]                  @ 4-byte Spill
	ldr	r3, [sp, #136]                  @ 4-byte Reload
	adcs	r8, r3, r8
	ldr	r3, [sp, #132]                  @ 4-byte Reload
	adcs	r3, r3, r5
	str	r3, [sp, #148]                  @ 4-byte Spill
	ldr	r3, [sp, #128]                  @ 4-byte Reload
	ldr	r5, [sp, #52]                   @ 4-byte Reload
	adcs	r3, r3, r5
	str	r3, [sp, #144]                  @ 4-byte Spill
	ldr	r3, [sp, #124]                  @ 4-byte Reload
	ldr	r5, [sp, #56]                   @ 4-byte Reload
	adcs	r3, r3, r5
	str	r3, [sp, #140]                  @ 4-byte Spill
	ldr	r3, [sp, #120]                  @ 4-byte Reload
	ldr	r5, [sp, #64]                   @ 4-byte Reload
	adcs	r3, r3, r5
	str	r3, [sp, #136]                  @ 4-byte Spill
	ldr	r3, [sp, #116]                  @ 4-byte Reload
	ldr	r5, [sp, #68]                   @ 4-byte Reload
	adcs	r3, r3, r5
	str	r3, [sp, #132]                  @ 4-byte Spill
	ldr	r3, [sp, #112]                  @ 4-byte Reload
	ldr	r5, [sp, #72]                   @ 4-byte Reload
	adcs	r3, r3, r5
	str	r3, [sp, #128]                  @ 4-byte Spill
	ldr	r3, [sp, #108]                  @ 4-byte Reload
	ldr	r5, [sp, #80]                   @ 4-byte Reload
	adcs	r3, r3, r5
	str	r3, [sp, #124]                  @ 4-byte Spill
	ldr	r3, [sp, #104]                  @ 4-byte Reload
	ldr	r5, [sp, #88]                   @ 4-byte Reload
	adcs	r3, r5, r3
	ldr	r5, [sp, #236]                  @ 4-byte Reload
	adc	r1, r1, #0
	str	r1, [sp, #116]                  @ 4-byte Spill
	str	r3, [sp, #120]                  @ 4-byte Spill
	ldr	r1, [r5, #116]
	str	r1, [sp, #112]                  @ 4-byte Spill
	ldr	r1, [sp, #444]
	str	r1, [sp, #108]                  @ 4-byte Spill
	ldr	r1, [sp, #440]
	str	r1, [sp, #104]                  @ 4-byte Spill
	ldr	r1, [sp, #436]
	str	r1, [sp, #96]                   @ 4-byte Spill
	ldr	r1, [sp, #432]
	str	r1, [sp, #92]                   @ 4-byte Spill
	ldr	r1, [sp, #428]
	str	r1, [sp, #88]                   @ 4-byte Spill
	ldr	r1, [sp, #424]
	str	r1, [sp, #80]                   @ 4-byte Spill
	ldr	r1, [sp, #420]
	str	r1, [sp, #64]                   @ 4-byte Spill
	ldr	r1, [sp, #416]
	str	r1, [sp, #56]                   @ 4-byte Spill
	ldr	r1, [sp, #412]
	str	r1, [sp, #48]                   @ 4-byte Spill
	ldr	r1, [sp, #408]
	str	r1, [sp, #36]                   @ 4-byte Spill
	ldr	r1, [sp, #404]
	str	r1, [sp, #24]                   @ 4-byte Spill
	mov	r1, r7
	ldr	r9, [sp, #400]
	bl	mulPv512x32
	add	r2, sp, #312
	ldm	r2, {r0, r1, r2}
	adds	r0, r4, r0
	str	r2, [sp, #72]                   @ 4-byte Spill
	ldr	r0, [sp, #76]                   @ 4-byte Reload
	ldr	r2, [sp, #324]
	adcs	r0, r0, r1
	str	r2, [sp, #68]                   @ 4-byte Spill
	str	r0, [sp, #156]                  @ 4-byte Spill
	mrs	r1, apsr
	str	r1, [sp, #44]                   @ 4-byte Spill
	mul	r2, r6, r0
	ldr	r1, [sp, #84]                   @ 4-byte Reload
	msr	APSR_nzcvq, r1
	ldr	r0, [sp, #100]                  @ 4-byte Reload
	ldr	r1, [sp, #60]                   @ 4-byte Reload
	ldr	r3, [sp, #24]                   @ 4-byte Reload
	adcs	r0, r1, r0
	str	r0, [sp, #32]                   @ 4-byte Spill
	ldr	r0, [r5, #120]
	adcs	r7, r11, r9
	str	r0, [sp, #228]                  @ 4-byte Spill
	adcs	r3, r10, r3
	ldr	r0, [sp, #372]
	str	r0, [sp, #100]                  @ 4-byte Spill
	ldr	r0, [sp, #368]
	str	r0, [sp, #84]                   @ 4-byte Spill
	ldr	r0, [sp, #364]
	str	r0, [sp, #76]                   @ 4-byte Spill
	ldr	r0, [sp, #360]
	str	r0, [sp, #60]                   @ 4-byte Spill
	ldr	r0, [sp, #356]
	str	r0, [sp, #52]                   @ 4-byte Spill
	ldr	r0, [sp, #352]
	str	r0, [sp, #40]                   @ 4-byte Spill
	ldr	r0, [sp, #348]
	str	r0, [sp, #28]                   @ 4-byte Spill
	ldr	r0, [sp, #344]
	str	r0, [sp, #20]                   @ 4-byte Spill
	ldr	r0, [sp, #340]
	str	r0, [sp, #16]                   @ 4-byte Spill
	ldr	r0, [sp, #336]
	str	r0, [sp, #12]                   @ 4-byte Spill
	ldr	r0, [sp, #332]
	str	r0, [sp, #8]                    @ 4-byte Spill
	ldr	r0, [sp, #328]
	str	r0, [sp, #4]                    @ 4-byte Spill
	add	r0, sp, #240
	ldr	r1, [sp, #376]
	str	r3, [sp, #24]                   @ 4-byte Spill
	ldr	r3, [sp, #152]                  @ 4-byte Reload
	ldr	r6, [sp, #36]                   @ 4-byte Reload
	ldr	r5, [sp, #64]                   @ 4-byte Reload
	adcs	r11, r3, r6
	ldr	r3, [sp, #48]                   @ 4-byte Reload
	ldr	r6, [sp, #56]                   @ 4-byte Reload
	adcs	r8, r8, r3
	ldr	r3, [sp, #148]                  @ 4-byte Reload
	ldr	r4, [sp, #92]                   @ 4-byte Reload
	adcs	r6, r3, r6
	ldr	r3, [sp, #144]                  @ 4-byte Reload
	adcs	r9, r3, r5
	ldr	r3, [sp, #140]                  @ 4-byte Reload
	ldr	r5, [sp, #80]                   @ 4-byte Reload
	adcs	r10, r3, r5
	ldr	r3, [sp, #136]                  @ 4-byte Reload
	ldr	r5, [sp, #88]                   @ 4-byte Reload
	adcs	r5, r3, r5
	ldr	r3, [sp, #132]                  @ 4-byte Reload
	adcs	r3, r3, r4
	str	r3, [sp, #152]                  @ 4-byte Spill
	ldr	r3, [sp, #128]                  @ 4-byte Reload
	ldr	r4, [sp, #96]                   @ 4-byte Reload
	adcs	r3, r3, r4
	str	r3, [sp, #148]                  @ 4-byte Spill
	ldr	r3, [sp, #124]                  @ 4-byte Reload
	ldr	r4, [sp, #104]                  @ 4-byte Reload
	adcs	r3, r3, r4
	str	r3, [sp, #144]                  @ 4-byte Spill
	ldr	r3, [sp, #120]                  @ 4-byte Reload
	ldr	r4, [sp, #108]                  @ 4-byte Reload
	adcs	r3, r3, r4
	str	r3, [sp, #132]                  @ 4-byte Spill
	ldr	r3, [sp, #116]                  @ 4-byte Reload
	ldr	r4, [sp, #112]                  @ 4-byte Reload
	adcs	r3, r4, r3
	str	r3, [sp, #128]                  @ 4-byte Spill
	adc	r1, r1, #0
	str	r1, [sp, #104]                  @ 4-byte Spill
	ldr	r1, [sp, #232]                  @ 4-byte Reload
	bl	mulPv512x32
	ldr	r0, [sp, #44]                   @ 4-byte Reload
	add	r2, sp, #244
	msr	APSR_nzcvq, r0
	ldr	r0, [sp, #72]                   @ 4-byte Reload
	ldr	r1, [sp, #32]                   @ 4-byte Reload
	adcs	r3, r1, r0
	ldr	r0, [sp, #68]                   @ 4-byte Reload
	ldr	r1, [sp, #24]                   @ 4-byte Reload
	adcs	r7, r7, r0
	ldr	r0, [sp, #4]                    @ 4-byte Reload
	adcs	r4, r1, r0
	ldr	r0, [sp, #8]                    @ 4-byte Reload
	ldr	r1, [sp, #152]                  @ 4-byte Reload
	adcs	r12, r11, r0
	ldr	r0, [sp, #12]                   @ 4-byte Reload
	adcs	lr, r8, r0
	ldr	r0, [sp, #16]                   @ 4-byte Reload
	adcs	r6, r6, r0
	ldr	r0, [sp, #20]                   @ 4-byte Reload
	adcs	r0, r9, r0
	str	r0, [sp, #140]                  @ 4-byte Spill
	ldr	r0, [sp, #28]                   @ 4-byte Reload
	adcs	r0, r10, r0
	str	r0, [sp, #136]                  @ 4-byte Spill
	ldr	r0, [sp, #40]                   @ 4-byte Reload
	ldr	r10, [sp, #156]                 @ 4-byte Reload
	adcs	r0, r5, r0
	str	r0, [sp, #116]                  @ 4-byte Spill
	ldr	r0, [sp, #52]                   @ 4-byte Reload
	adcs	r0, r1, r0
	str	r0, [sp, #108]                  @ 4-byte Spill
	ldr	r0, [sp, #60]                   @ 4-byte Reload
	ldr	r1, [sp, #148]                  @ 4-byte Reload
	adcs	r11, r1, r0
	ldr	r0, [sp, #76]                   @ 4-byte Reload
	ldr	r1, [sp, #144]                  @ 4-byte Reload
	adcs	r0, r1, r0
	str	r0, [sp, #112]                  @ 4-byte Spill
	ldr	r0, [sp, #84]                   @ 4-byte Reload
	ldr	r1, [sp, #132]                  @ 4-byte Reload
	adcs	r0, r1, r0
	str	r0, [sp, #120]                  @ 4-byte Spill
	ldr	r0, [sp, #100]                  @ 4-byte Reload
	ldr	r1, [sp, #128]                  @ 4-byte Reload
	adcs	r0, r1, r0
	str	r0, [sp, #124]                  @ 4-byte Spill
	ldr	r0, [sp, #228]                  @ 4-byte Reload
	ldr	r1, [sp, #104]                  @ 4-byte Reload
	adcs	r0, r0, r1
	str	r0, [sp, #132]                  @ 4-byte Spill
	ldr	r5, [sp, #304]
	adc	r0, r5, #0
	str	r0, [sp, #128]                  @ 4-byte Spill
	ldr	r5, [sp, #240]
	ldm	r2, {r0, r1, r2}
	adds	r5, r10, r5
	adcs	r9, r3, r0
	str	r9, [sp, #232]                  @ 4-byte Spill
	adcs	r10, r7, r1
	str	r10, [sp, #228]                 @ 4-byte Spill
	adcs	r8, r4, r2
	str	r8, [sp, #156]                  @ 4-byte Spill
	ldr	r0, [sp, #256]
	ldr	r1, [sp, #140]                  @ 4-byte Reload
	adcs	r7, r12, r0
	str	r7, [sp, #152]                  @ 4-byte Spill
	ldr	r0, [sp, #260]
	ldr	r3, [sp, #112]                  @ 4-byte Reload
	adcs	r5, lr, r0
	str	r5, [sp, #148]                  @ 4-byte Spill
	ldr	r0, [sp, #264]
	adcs	r6, r6, r0
	str	r6, [sp, #144]                  @ 4-byte Spill
	ldr	r0, [sp, #268]
	adcs	r12, r1, r0
	str	r12, [sp, #140]                 @ 4-byte Spill
	ldr	r0, [sp, #272]
	ldr	r1, [sp, #136]                  @ 4-byte Reload
	adcs	lr, r1, r0
	str	lr, [sp, #136]                  @ 4-byte Spill
	ldr	r0, [sp, #276]
	ldr	r1, [sp, #116]                  @ 4-byte Reload
	adcs	r4, r1, r0
	str	r4, [sp, #116]                  @ 4-byte Spill
	ldr	r0, [sp, #280]
	ldr	r1, [sp, #108]                  @ 4-byte Reload
	adcs	r2, r1, r0
	str	r2, [sp, #108]                  @ 4-byte Spill
	ldr	r0, [sp, #284]
	adcs	r1, r11, r0
	str	r1, [sp, #104]                  @ 4-byte Spill
	ldr	r0, [sp, #288]
	adcs	r11, r3, r0
	ldr	r0, [sp, #292]
	ldr	r3, [sp, #120]                  @ 4-byte Reload
	adcs	r0, r3, r0
	str	r0, [sp, #120]                  @ 4-byte Spill
	ldr	r0, [sp, #296]
	ldr	r3, [sp, #124]                  @ 4-byte Reload
	adcs	r0, r3, r0
	str	r0, [sp, #124]                  @ 4-byte Spill
	ldr	r0, [sp, #300]
	ldr	r3, [sp, #132]                  @ 4-byte Reload
	adcs	r0, r3, r0
	str	r0, [sp, #112]                  @ 4-byte Spill
	ldr	r0, [sp, #236]                  @ 4-byte Reload
	ldr	r3, [sp, #128]                  @ 4-byte Reload
	ldr	r0, [r0, #124]
	adc	r0, r0, r3
	str	r0, [sp, #132]                  @ 4-byte Spill
	ldr	r0, [sp, #216]                  @ 4-byte Reload
	subs	r0, r9, r0
	str	r0, [sp, #236]                  @ 4-byte Spill
	ldr	r0, [sp, #212]                  @ 4-byte Reload
	sbcs	r0, r10, r0
	str	r0, [sp, #216]                  @ 4-byte Spill
	ldr	r0, [sp, #208]                  @ 4-byte Reload
	sbcs	r0, r8, r0
	str	r0, [sp, #212]                  @ 4-byte Spill
	ldr	r0, [sp, #192]                  @ 4-byte Reload
	sbcs	r0, r7, r0
	str	r0, [sp, #208]                  @ 4-byte Spill
	ldr	r0, [sp, #196]                  @ 4-byte Reload
	mov	r7, #0
	sbcs	r0, r5, r0
	str	r0, [sp, #196]                  @ 4-byte Spill
	ldr	r0, [sp, #200]                  @ 4-byte Reload
	ldr	r5, [sp, #120]                  @ 4-byte Reload
	sbcs	r0, r6, r0
	str	r0, [sp, #200]                  @ 4-byte Spill
	ldr	r0, [sp, #204]                  @ 4-byte Reload
	sbcs	r0, r12, r0
	str	r0, [sp, #204]                  @ 4-byte Spill
	ldr	r0, [sp, #160]                  @ 4-byte Reload
	ldr	r12, [sp, #112]                 @ 4-byte Reload
	sbcs	r0, lr, r0
	str	r0, [sp, #192]                  @ 4-byte Spill
	ldr	r0, [sp, #164]                  @ 4-byte Reload
	ldr	lr, [sp, #124]                  @ 4-byte Reload
	sbcs	r0, r4, r0
	str	r0, [sp, #164]                  @ 4-byte Spill
	ldr	r0, [sp, #168]                  @ 4-byte Reload
	sbcs	r10, r2, r0
	ldr	r0, [sp, #172]                  @ 4-byte Reload
	ldr	r2, [sp, #132]                  @ 4-byte Reload
	sbcs	r9, r1, r0
	ldr	r0, [sp, #176]                  @ 4-byte Reload
	sbcs	r6, r11, r0
	ldr	r0, [sp, #180]                  @ 4-byte Reload
	sbcs	r4, r5, r0
	ldr	r0, [sp, #184]                  @ 4-byte Reload
	sbcs	r3, lr, r0
	ldr	r0, [sp, #188]                  @ 4-byte Reload
	sbcs	r1, r12, r0
	ldr	r0, [sp, #220]                  @ 4-byte Reload
	sbcs	r0, r2, r0
	sbc	r7, r7, #0
	ands	r8, r7, #1
	ldr	r7, [sp, #224]                  @ 4-byte Reload
	movne	r0, r2
	movne	r1, r12
	movne	r3, lr
	cmp	r8, #0
	str	r0, [r7, #60]
	movne	r4, r5
	ldr	r0, [sp, #104]                  @ 4-byte Reload
	movne	r6, r11
	str	r1, [r7, #56]
	ldr	r1, [sp, #164]                  @ 4-byte Reload
	movne	r9, r0
	ldr	r0, [sp, #108]                  @ 4-byte Reload
	cmp	r8, #0
	str	r3, [r7, #52]
	str	r4, [r7, #48]
	movne	r10, r0
	ldr	r0, [sp, #116]                  @ 4-byte Reload
	str	r6, [r7, #44]
	str	r9, [r7, #40]
	movne	r1, r0
	ldr	r0, [sp, #136]                  @ 4-byte Reload
	str	r1, [r7, #32]
	ldr	r1, [sp, #192]                  @ 4-byte Reload
	movne	r1, r0
	cmp	r8, #0
	str	r1, [r7, #28]
	ldr	r1, [sp, #140]                  @ 4-byte Reload
	ldr	r0, [sp, #204]                  @ 4-byte Reload
	str	r10, [r7, #36]
	movne	r0, r1
	ldr	r1, [sp, #144]                  @ 4-byte Reload
	str	r0, [r7, #24]
	ldr	r0, [sp, #200]                  @ 4-byte Reload
	movne	r0, r1
	ldr	r1, [sp, #148]                  @ 4-byte Reload
	str	r0, [r7, #20]
	ldr	r0, [sp, #196]                  @ 4-byte Reload
	movne	r0, r1
	ldr	r1, [sp, #152]                  @ 4-byte Reload
	str	r0, [r7, #16]
	cmp	r8, #0
	ldr	r0, [sp, #208]                  @ 4-byte Reload
	movne	r0, r1
	ldr	r1, [sp, #156]                  @ 4-byte Reload
	str	r0, [r7, #12]
	ldr	r0, [sp, #212]                  @ 4-byte Reload
	movne	r0, r1
	ldr	r1, [sp, #228]                  @ 4-byte Reload
	str	r0, [r7, #8]
	ldr	r0, [sp, #216]                  @ 4-byte Reload
	movne	r0, r1
	ldr	r1, [sp, #232]                  @ 4-byte Reload
	str	r0, [r7, #4]
	cmp	r8, #0
	ldr	r0, [sp, #236]                  @ 4-byte Reload
	movne	r0, r1
	str	r0, [r7]
	add	sp, sp, #372
	add	sp, sp, #1024
	pop	{r4, r5, r6, r7, r8, r9, r10, r11, lr}
	mov	pc, lr
.Lfunc_end79:
	.size	mcl_fp_montRed16L, .Lfunc_end79-mcl_fp_montRed16L
	.cantunwind
	.fnend
                                        @ -- End function
	.globl	mcl_fp_montRedNF16L             @ -- Begin function mcl_fp_montRedNF16L
	.p2align	2
	.type	mcl_fp_montRedNF16L,%function
	.code	32                              @ @mcl_fp_montRedNF16L
mcl_fp_montRedNF16L:
	.fnstart
@ %bb.0:
	.save	{r4, r5, r6, r7, r8, r9, r10, r11, lr}
	push	{r4, r5, r6, r7, r8, r9, r10, r11, lr}
	.pad	#372
	sub	sp, sp, #372
	.pad	#1024
	sub	sp, sp, #1024
	str	r0, [sp, #224]                  @ 4-byte Spill
	mov	r3, r2
	ldr	r0, [r2]
	add	lr, sp, #1024
	str	r0, [sp, #216]                  @ 4-byte Spill
	ldr	r0, [r2, #4]
	str	r0, [sp, #212]                  @ 4-byte Spill
	ldr	r0, [r2, #8]
	str	r0, [sp, #208]                  @ 4-byte Spill
	ldr	r0, [r2, #12]
	str	r0, [sp, #192]                  @ 4-byte Spill
	ldr	r0, [r2, #16]
	str	r0, [sp, #196]                  @ 4-byte Spill
	ldr	r0, [r2, #20]
	str	r0, [sp, #200]                  @ 4-byte Spill
	ldr	r0, [r2, #24]
	str	r0, [sp, #204]                  @ 4-byte Spill
	ldr	r0, [r1, #4]
	str	r0, [sp, #156]                  @ 4-byte Spill
	ldr	r0, [r1, #8]
	str	r0, [sp, #152]                  @ 4-byte Spill
	ldr	r0, [r1, #12]
	str	r0, [sp, #148]                  @ 4-byte Spill
	ldr	r0, [r3, #60]
	str	r0, [sp, #220]                  @ 4-byte Spill
	ldr	r0, [r3, #28]
	str	r0, [sp, #160]                  @ 4-byte Spill
	ldr	r0, [r3, #32]
	str	r0, [sp, #164]                  @ 4-byte Spill
	ldr	r0, [r3, #36]
	str	r0, [sp, #168]                  @ 4-byte Spill
	ldr	r0, [r3, #40]
	str	r0, [sp, #172]                  @ 4-byte Spill
	ldr	r0, [r3, #44]
	str	r0, [sp, #176]                  @ 4-byte Spill
	ldr	r0, [r3, #48]
	str	r0, [sp, #180]                  @ 4-byte Spill
	ldr	r0, [r3, #52]
	str	r0, [sp, #184]                  @ 4-byte Spill
	ldr	r0, [r3, #56]
	str	r0, [sp, #188]                  @ 4-byte Spill
	ldr	r0, [r1, #32]
	str	r0, [sp, #116]                  @ 4-byte Spill
	ldr	r0, [r1, #36]
	str	r0, [sp, #120]                  @ 4-byte Spill
	ldr	r0, [r1, #40]
	str	r0, [sp, #124]                  @ 4-byte Spill
	ldr	r0, [r1, #44]
	str	r0, [sp, #128]                  @ 4-byte Spill
	ldr	r0, [r1, #48]
	ldr	r7, [r2, #-4]
	ldr	r9, [r1]
	str	r0, [sp, #132]                  @ 4-byte Spill
	ldr	r0, [r1, #52]
	str	r0, [sp, #136]                  @ 4-byte Spill
	mul	r2, r9, r7
	ldr	r0, [r1, #56]
	str	r0, [sp, #140]                  @ 4-byte Spill
	ldr	r0, [r1, #60]
	str	r0, [sp, #144]                  @ 4-byte Spill
	ldr	r0, [r1, #28]
	str	r0, [sp, #112]                  @ 4-byte Spill
	ldr	r0, [r1, #24]
	str	r0, [sp, #108]                  @ 4-byte Spill
	ldr	r0, [r1, #20]
	str	r0, [sp, #104]                  @ 4-byte Spill
	ldr	r0, [r1, #16]
	str	r0, [sp, #100]                  @ 4-byte Spill
	add	r0, lr, #296
	str	r1, [sp, #236]                  @ 4-byte Spill
	mov	r1, r3
	str	r7, [sp, #228]                  @ 4-byte Spill
	str	r3, [sp, #232]                  @ 4-byte Spill
	bl	mulPv512x32
	ldr	r0, [sp, #1380]
	add	r11, sp, #1344
	str	r0, [sp, #96]                   @ 4-byte Spill
	ldr	r0, [sp, #1376]
	str	r0, [sp, #92]                   @ 4-byte Spill
	ldr	r0, [sp, #1372]
	str	r0, [sp, #88]                   @ 4-byte Spill
	ldr	r0, [sp, #1320]
	ldr	r1, [sp, #1324]
	adds	r0, r9, r0
	ldr	r2, [sp, #1328]
	ldr	r0, [sp, #156]                  @ 4-byte Reload
	ldm	r11, {r4, r5, r6, r7, r8, r10, r11}
	adcs	r9, r0, r1
	ldr	r0, [sp, #152]                  @ 4-byte Reload
	ldr	lr, [sp, #1340]
	adcs	r0, r0, r2
	ldr	r12, [sp, #1336]
	ldr	r3, [sp, #1332]
	str	r0, [sp, #68]                   @ 4-byte Spill
	ldr	r0, [sp, #148]                  @ 4-byte Reload
	ldr	r1, [sp, #88]                   @ 4-byte Reload
	adcs	r0, r0, r3
	str	r0, [sp, #148]                  @ 4-byte Spill
	ldr	r0, [sp, #100]                  @ 4-byte Reload
	adcs	r0, r0, r12
	str	r0, [sp, #72]                   @ 4-byte Spill
	ldr	r0, [sp, #104]                  @ 4-byte Reload
	adcs	r0, r0, lr
	str	r0, [sp, #76]                   @ 4-byte Spill
	ldr	r0, [sp, #108]                  @ 4-byte Reload
	adcs	r0, r0, r4
	str	r0, [sp, #156]                  @ 4-byte Spill
	ldr	r0, [sp, #112]                  @ 4-byte Reload
	adcs	r0, r0, r5
	str	r0, [sp, #112]                  @ 4-byte Spill
	ldr	r0, [sp, #116]                  @ 4-byte Reload
	adcs	r0, r0, r6
	str	r0, [sp, #116]                  @ 4-byte Spill
	ldr	r0, [sp, #120]                  @ 4-byte Reload
	adcs	r0, r0, r7
	str	r0, [sp, #120]                  @ 4-byte Spill
	ldr	r0, [sp, #124]                  @ 4-byte Reload
	adcs	r0, r0, r8
	str	r0, [sp, #108]                  @ 4-byte Spill
	ldr	r0, [sp, #128]                  @ 4-byte Reload
	adcs	r0, r0, r10
	str	r0, [sp, #104]                  @ 4-byte Spill
	ldr	r0, [sp, #132]                  @ 4-byte Reload
	adcs	r0, r0, r11
	str	r0, [sp, #100]                  @ 4-byte Spill
	ldr	r0, [sp, #136]                  @ 4-byte Reload
	adcs	r0, r0, r1
	str	r0, [sp, #88]                   @ 4-byte Spill
	ldr	r0, [sp, #140]                  @ 4-byte Reload
	ldr	r1, [sp, #92]                   @ 4-byte Reload
	adcs	r0, r0, r1
	str	r0, [sp, #84]                   @ 4-byte Spill
	ldr	r0, [sp, #144]                  @ 4-byte Reload
	ldr	r1, [sp, #96]                   @ 4-byte Reload
	adcs	r0, r0, r1
	str	r0, [sp, #80]                   @ 4-byte Spill
	mrs	r0, apsr
	ldr	r7, [sp, #228]                  @ 4-byte Reload
	ldr	r11, [sp, #236]                 @ 4-byte Reload
	str	r0, [sp, #96]                   @ 4-byte Spill
	mul	r2, r7, r9
	ldr	r6, [sp, #232]                  @ 4-byte Reload
	ldr	r0, [r11, #64]
	str	r0, [sp, #152]                  @ 4-byte Spill
	ldr	r0, [sp, #1384]
	mov	r1, r6
	str	r0, [sp, #92]                   @ 4-byte Spill
	add	r0, sp, #1248
	bl	mulPv512x32
	ldr	r0, [sp, #1304]
	str	r0, [sp, #64]                   @ 4-byte Spill
	ldr	r0, [sp, #1300]
	str	r0, [sp, #60]                   @ 4-byte Spill
	ldr	r5, [sp, #1248]
	ldr	r4, [sp, #1252]
	ldr	r2, [sp, #68]                   @ 4-byte Reload
	adds	r5, r9, r5
	ldr	r0, [sp, #1256]
	adcs	r8, r2, r4
	ldr	r4, [sp, #148]                  @ 4-byte Reload
	ldr	r1, [sp, #1260]
	adcs	r0, r4, r0
	ldr	r4, [sp, #72]                   @ 4-byte Reload
	ldr	lr, [sp, #1296]
	ldr	r10, [sp, #1292]
	adcs	r1, r4, r1
	ldr	r12, [sp, #1288]
	ldr	r3, [sp, #1284]
	ldr	r5, [sp, #1280]
	ldr	r2, [sp, #1276]
	str	r0, [sp, #148]                  @ 4-byte Spill
	ldr	r0, [sp, #1272]
	str	r1, [sp, #144]                  @ 4-byte Spill
	ldr	r1, [sp, #1264]
	ldr	r4, [sp, #76]                   @ 4-byte Reload
	adcs	r1, r4, r1
	str	r1, [sp, #140]                  @ 4-byte Spill
	ldr	r1, [sp, #1268]
	ldr	r4, [sp, #156]                  @ 4-byte Reload
	adcs	r1, r4, r1
	str	r1, [sp, #136]                  @ 4-byte Spill
	ldr	r1, [sp, #112]                  @ 4-byte Reload
	adcs	r0, r1, r0
	str	r0, [sp, #132]                  @ 4-byte Spill
	ldr	r0, [sp, #116]                  @ 4-byte Reload
	ldr	r1, [sp, #60]                   @ 4-byte Reload
	adcs	r0, r0, r2
	str	r0, [sp, #128]                  @ 4-byte Spill
	ldr	r0, [sp, #120]                  @ 4-byte Reload
	mul	r2, r7, r8
	adcs	r0, r0, r5
	str	r0, [sp, #124]                  @ 4-byte Spill
	ldr	r0, [sp, #108]                  @ 4-byte Reload
	adcs	r0, r0, r3
	str	r0, [sp, #120]                  @ 4-byte Spill
	ldr	r0, [sp, #104]                  @ 4-byte Reload
	adcs	r0, r0, r12
	str	r0, [sp, #116]                  @ 4-byte Spill
	ldr	r0, [sp, #100]                  @ 4-byte Reload
	adcs	r0, r0, r10
	str	r0, [sp, #112]                  @ 4-byte Spill
	ldr	r0, [sp, #88]                   @ 4-byte Reload
	adcs	r0, r0, lr
	str	r0, [sp, #108]                  @ 4-byte Spill
	ldr	r0, [sp, #84]                   @ 4-byte Reload
	add	lr, sp, #1024
	adcs	r0, r0, r1
	str	r0, [sp, #104]                  @ 4-byte Spill
	ldr	r0, [sp, #80]                   @ 4-byte Reload
	ldr	r1, [sp, #64]                   @ 4-byte Reload
	adcs	r0, r0, r1
	str	r0, [sp, #100]                  @ 4-byte Spill
	mrs	r0, apsr
	str	r0, [sp, #156]                  @ 4-byte Spill
	ldr	r0, [sp, #96]                   @ 4-byte Reload
	msr	APSR_nzcvq, r0
	ldr	r1, [sp, #152]                  @ 4-byte Reload
	ldr	r3, [sp, #92]                   @ 4-byte Reload
	ldr	r0, [sp, #1312]
	adcs	r1, r1, r3
	str	r1, [sp, #96]                   @ 4-byte Spill
	adc	r0, r0, #0
	str	r0, [sp, #92]                   @ 4-byte Spill
	ldr	r0, [r11, #68]
	mov	r1, r6
	str	r0, [sp, #88]                   @ 4-byte Spill
	ldr	r0, [sp, #1308]
	str	r0, [sp, #84]                   @ 4-byte Spill
	add	r0, lr, #152
	bl	mulPv512x32
	ldr	r7, [sp, #1176]
	ldr	r0, [sp, #1180]
	ldr	r2, [sp, #148]                  @ 4-byte Reload
	adds	r4, r8, r7
	ldr	r1, [sp, #1184]
	adcs	r4, r2, r0
	ldr	r2, [sp, #144]                  @ 4-byte Reload
	ldr	r10, [sp, #1188]
	adcs	r1, r2, r1
	ldr	r2, [sp, #140]                  @ 4-byte Reload
	ldr	r11, [sp, #1228]
	ldr	r9, [sp, #1224]
	adcs	r2, r2, r10
	ldr	lr, [sp, #1220]
	ldr	r12, [sp, #1216]
	ldr	r6, [sp, #1212]
	ldr	r5, [sp, #1208]
	ldr	r7, [sp, #1204]
	ldr	r0, [sp, #1200]
	str	r1, [sp, #148]                  @ 4-byte Spill
	ldr	r1, [sp, #1196]
	str	r2, [sp, #144]                  @ 4-byte Spill
	ldr	r2, [sp, #1192]
	ldr	r3, [sp, #136]                  @ 4-byte Reload
	adcs	r2, r3, r2
	str	r2, [sp, #140]                  @ 4-byte Spill
	ldr	r2, [sp, #132]                  @ 4-byte Reload
	adcs	r1, r2, r1
	str	r1, [sp, #136]                  @ 4-byte Spill
	ldr	r1, [sp, #128]                  @ 4-byte Reload
	adcs	r0, r1, r0
	str	r0, [sp, #132]                  @ 4-byte Spill
	ldr	r0, [sp, #124]                  @ 4-byte Reload
	adcs	r0, r0, r7
	str	r0, [sp, #128]                  @ 4-byte Spill
	ldr	r0, [sp, #120]                  @ 4-byte Reload
	adcs	r0, r0, r5
	str	r0, [sp, #124]                  @ 4-byte Spill
	ldr	r0, [sp, #116]                  @ 4-byte Reload
	adcs	r0, r0, r6
	str	r0, [sp, #120]                  @ 4-byte Spill
	ldr	r0, [sp, #112]                  @ 4-byte Reload
	adcs	r0, r0, r12
	str	r0, [sp, #116]                  @ 4-byte Spill
	ldr	r0, [sp, #108]                  @ 4-byte Reload
	adcs	r0, r0, lr
	str	r0, [sp, #112]                  @ 4-byte Spill
	ldr	r0, [sp, #104]                  @ 4-byte Reload
	adcs	r0, r0, r9
	str	r0, [sp, #108]                  @ 4-byte Spill
	ldr	r0, [sp, #100]                  @ 4-byte Reload
	adcs	r0, r0, r11
	str	r0, [sp, #104]                  @ 4-byte Spill
	mrs	r0, apsr
	str	r0, [sp, #152]                  @ 4-byte Spill
	ldr	r0, [sp, #156]                  @ 4-byte Reload
	msr	APSR_nzcvq, r0
	ldr	r1, [sp, #96]                   @ 4-byte Reload
	ldr	r3, [sp, #84]                   @ 4-byte Reload
	ldr	r0, [sp, #1240]
	adcs	r1, r1, r3
	str	r1, [sp, #100]                  @ 4-byte Spill
	ldr	r1, [sp, #92]                   @ 4-byte Reload
	ldr	r3, [sp, #88]                   @ 4-byte Reload
	ldr	r7, [sp, #236]                  @ 4-byte Reload
	adcs	r1, r3, r1
	ldr	r6, [sp, #228]                  @ 4-byte Reload
	adc	r0, r0, #0
	str	r0, [sp, #92]                   @ 4-byte Spill
	ldr	r0, [r7, #72]
	mul	r2, r6, r4
	str	r1, [sp, #96]                   @ 4-byte Spill
	str	r0, [sp, #88]                   @ 4-byte Spill
	ldr	r0, [sp, #1236]
	str	r0, [sp, #84]                   @ 4-byte Spill
	ldr	r8, [sp, #232]                  @ 4-byte Reload
	ldr	r0, [sp, #1232]
	str	r0, [sp, #80]                   @ 4-byte Spill
	add	r0, sp, #1104
	mov	r1, r8
	bl	mulPv512x32
	ldr	r0, [sp, #1152]
	str	r0, [sp, #156]                  @ 4-byte Spill
	ldr	r0, [sp, #1148]
	str	r0, [sp, #76]                   @ 4-byte Spill
	ldr	r5, [sp, #1104]
	ldr	r0, [sp, #1108]
	adds	r4, r4, r5
	ldr	r1, [sp, #1112]
	ldr	r4, [sp, #148]                  @ 4-byte Reload
	ldr	r11, [sp, #144]                 @ 4-byte Reload
	adcs	r9, r4, r0
	ldr	r2, [sp, #1116]
	ldr	r4, [sp, #140]                  @ 4-byte Reload
	adcs	r1, r11, r1
	ldr	r10, [sp, #1144]
	ldr	lr, [sp, #1140]
	adcs	r2, r4, r2
	ldr	r12, [sp, #1136]
	ldr	r3, [sp, #1132]
	ldr	r5, [sp, #1128]
	ldr	r0, [sp, #1124]
	str	r1, [sp, #148]                  @ 4-byte Spill
	ldr	r1, [sp, #1120]
	str	r2, [sp, #144]                  @ 4-byte Spill
	ldr	r2, [sp, #136]                  @ 4-byte Reload
	adcs	r1, r2, r1
	str	r1, [sp, #140]                  @ 4-byte Spill
	ldr	r1, [sp, #132]                  @ 4-byte Reload
	mul	r2, r6, r9
	adcs	r0, r1, r0
	str	r0, [sp, #136]                  @ 4-byte Spill
	ldr	r0, [sp, #128]                  @ 4-byte Reload
	ldr	r1, [sp, #76]                   @ 4-byte Reload
	adcs	r0, r0, r5
	str	r0, [sp, #132]                  @ 4-byte Spill
	ldr	r0, [sp, #124]                  @ 4-byte Reload
	adcs	r0, r0, r3
	str	r0, [sp, #128]                  @ 4-byte Spill
	ldr	r0, [sp, #120]                  @ 4-byte Reload
	adcs	r0, r0, r12
	str	r0, [sp, #124]                  @ 4-byte Spill
	ldr	r0, [sp, #116]                  @ 4-byte Reload
	adcs	r0, r0, lr
	str	r0, [sp, #120]                  @ 4-byte Spill
	ldr	r0, [sp, #112]                  @ 4-byte Reload
	add	lr, sp, #1024
	adcs	r0, r0, r10
	str	r0, [sp, #116]                  @ 4-byte Spill
	ldr	r0, [sp, #108]                  @ 4-byte Reload
	adcs	r0, r0, r1
	str	r0, [sp, #112]                  @ 4-byte Spill
	ldr	r0, [sp, #104]                  @ 4-byte Reload
	ldr	r1, [sp, #156]                  @ 4-byte Reload
	adcs	r0, r0, r1
	str	r0, [sp, #108]                  @ 4-byte Spill
	mrs	r0, apsr
	str	r0, [sp, #156]                  @ 4-byte Spill
	ldr	r0, [sp, #152]                  @ 4-byte Reload
	msr	APSR_nzcvq, r0
	ldr	r1, [sp, #100]                  @ 4-byte Reload
	ldr	r3, [sp, #80]                   @ 4-byte Reload
	ldr	r0, [sp, #1168]
	adcs	r1, r1, r3
	str	r1, [sp, #104]                  @ 4-byte Spill
	ldr	r1, [sp, #96]                   @ 4-byte Reload
	ldr	r3, [sp, #84]                   @ 4-byte Reload
	adcs	r1, r1, r3
	str	r1, [sp, #100]                  @ 4-byte Spill
	ldr	r1, [sp, #92]                   @ 4-byte Reload
	ldr	r3, [sp, #88]                   @ 4-byte Reload
	adcs	r1, r3, r1
	str	r1, [sp, #96]                   @ 4-byte Spill
	adc	r0, r0, #0
	str	r0, [sp, #92]                   @ 4-byte Spill
	ldr	r0, [r7, #76]
	mov	r1, r8
	str	r0, [sp, #88]                   @ 4-byte Spill
	ldr	r0, [sp, #1164]
	str	r0, [sp, #84]                   @ 4-byte Spill
	ldr	r0, [sp, #1160]
	str	r0, [sp, #80]                   @ 4-byte Spill
	ldr	r0, [sp, #1156]
	str	r0, [sp, #76]                   @ 4-byte Spill
	add	r0, lr, #8
	bl	mulPv512x32
	ldr	r5, [sp, #1032]
	ldr	r0, [sp, #1036]
	adds	r4, r9, r5
	ldr	r1, [sp, #1040]
	ldr	r4, [sp, #148]                  @ 4-byte Reload
	ldr	r11, [sp, #1076]
	adcs	r10, r4, r0
	ldr	r4, [sp, #144]                  @ 4-byte Reload
	ldr	lr, [sp, #1072]
	adcs	r1, r4, r1
	ldr	r12, [sp, #1068]
	ldr	r3, [sp, #1064]
	ldr	r6, [sp, #1060]
	ldr	r7, [sp, #1056]
	ldr	r2, [sp, #1044]
	ldr	r5, [sp, #1052]
	ldr	r0, [sp, #1048]
	str	r1, [sp, #148]                  @ 4-byte Spill
	ldr	r1, [sp, #140]                  @ 4-byte Reload
	adcs	r1, r1, r2
	str	r1, [sp, #144]                  @ 4-byte Spill
	ldr	r1, [sp, #136]                  @ 4-byte Reload
	adcs	r0, r1, r0
	str	r0, [sp, #140]                  @ 4-byte Spill
	ldr	r0, [sp, #132]                  @ 4-byte Reload
	adcs	r0, r0, r5
	str	r0, [sp, #136]                  @ 4-byte Spill
	ldr	r0, [sp, #128]                  @ 4-byte Reload
	adcs	r0, r0, r7
	str	r0, [sp, #132]                  @ 4-byte Spill
	ldr	r0, [sp, #124]                  @ 4-byte Reload
	adcs	r0, r0, r6
	str	r0, [sp, #128]                  @ 4-byte Spill
	ldr	r0, [sp, #120]                  @ 4-byte Reload
	adcs	r0, r0, r3
	str	r0, [sp, #124]                  @ 4-byte Spill
	ldr	r0, [sp, #116]                  @ 4-byte Reload
	adcs	r0, r0, r12
	str	r0, [sp, #120]                  @ 4-byte Spill
	ldr	r0, [sp, #112]                  @ 4-byte Reload
	adcs	r0, r0, lr
	str	r0, [sp, #116]                  @ 4-byte Spill
	ldr	r0, [sp, #108]                  @ 4-byte Reload
	adcs	r0, r0, r11
	str	r0, [sp, #112]                  @ 4-byte Spill
	mrs	r0, apsr
	str	r0, [sp, #152]                  @ 4-byte Spill
	ldr	r0, [sp, #156]                  @ 4-byte Reload
	msr	APSR_nzcvq, r0
	ldr	r1, [sp, #104]                  @ 4-byte Reload
	ldr	r3, [sp, #76]                   @ 4-byte Reload
	ldr	r0, [sp, #1096]
	adcs	r1, r1, r3
	str	r1, [sp, #108]                  @ 4-byte Spill
	ldr	r1, [sp, #100]                  @ 4-byte Reload
	ldr	r3, [sp, #80]                   @ 4-byte Reload
	ldr	r7, [sp, #236]                  @ 4-byte Reload
	adcs	r1, r1, r3
	str	r1, [sp, #104]                  @ 4-byte Spill
	ldr	r1, [sp, #96]                   @ 4-byte Reload
	ldr	r3, [sp, #84]                   @ 4-byte Reload
	ldr	r6, [sp, #228]                  @ 4-byte Reload
	adcs	r1, r1, r3
	str	r1, [sp, #100]                  @ 4-byte Spill
	ldr	r1, [sp, #92]                   @ 4-byte Reload
	ldr	r3, [sp, #88]                   @ 4-byte Reload
	mul	r2, r6, r10
	adcs	r1, r3, r1
	str	r1, [sp, #96]                   @ 4-byte Spill
	adc	r0, r0, #0
	str	r0, [sp, #92]                   @ 4-byte Spill
	ldr	r0, [r7, #80]
	mov	r1, r8
	str	r0, [sp, #88]                   @ 4-byte Spill
	ldr	r0, [sp, #1092]
	str	r0, [sp, #84]                   @ 4-byte Spill
	ldr	r0, [sp, #1088]
	str	r0, [sp, #80]                   @ 4-byte Spill
	ldr	r0, [sp, #1084]
	str	r0, [sp, #76]                   @ 4-byte Spill
	ldr	r0, [sp, #1080]
	str	r0, [sp, #72]                   @ 4-byte Spill
	add	r0, sp, #960
	bl	mulPv512x32
	ldr	r5, [sp, #960]
	add	r2, sp, #964
	add	r11, sp, #992
	add	lr, sp, #980
	adds	r4, r10, r5
	ldm	r2, {r0, r1, r2}
	ldr	r4, [sp, #148]                  @ 4-byte Reload
	ldm	r11, {r8, r9, r11}
	adcs	r10, r4, r0
	ldr	r0, [sp, #144]                  @ 4-byte Reload
	ldm	lr, {r3, r12, lr}
	adcs	r0, r0, r1
	ldr	r5, [sp, #976]
	str	r0, [sp, #148]                  @ 4-byte Spill
	ldr	r0, [sp, #140]                  @ 4-byte Reload
	adcs	r0, r0, r2
	str	r0, [sp, #144]                  @ 4-byte Spill
	ldr	r0, [sp, #136]                  @ 4-byte Reload
	mul	r2, r6, r10
	adcs	r0, r0, r5
	str	r0, [sp, #140]                  @ 4-byte Spill
	ldr	r0, [sp, #132]                  @ 4-byte Reload
	adcs	r0, r0, r3
	str	r0, [sp, #136]                  @ 4-byte Spill
	ldr	r0, [sp, #128]                  @ 4-byte Reload
	adcs	r0, r0, r12
	str	r0, [sp, #132]                  @ 4-byte Spill
	ldr	r0, [sp, #124]                  @ 4-byte Reload
	adcs	r0, r0, lr
	str	r0, [sp, #128]                  @ 4-byte Spill
	ldr	r0, [sp, #120]                  @ 4-byte Reload
	adcs	r0, r0, r8
	str	r0, [sp, #124]                  @ 4-byte Spill
	ldr	r0, [sp, #116]                  @ 4-byte Reload
	adcs	r0, r0, r9
	str	r0, [sp, #120]                  @ 4-byte Spill
	ldr	r0, [sp, #112]                  @ 4-byte Reload
	adcs	r0, r0, r11
	str	r0, [sp, #116]                  @ 4-byte Spill
	mrs	r0, apsr
	str	r0, [sp, #156]                  @ 4-byte Spill
	ldr	r0, [sp, #152]                  @ 4-byte Reload
	msr	APSR_nzcvq, r0
	ldr	r1, [sp, #108]                  @ 4-byte Reload
	ldr	r3, [sp, #72]                   @ 4-byte Reload
	ldr	r0, [sp, #1024]
	adcs	r1, r1, r3
	str	r1, [sp, #112]                  @ 4-byte Spill
	ldr	r1, [sp, #104]                  @ 4-byte Reload
	ldr	r3, [sp, #76]                   @ 4-byte Reload
	ldr	r4, [sp, #232]                  @ 4-byte Reload
	adcs	r1, r1, r3
	str	r1, [sp, #108]                  @ 4-byte Spill
	ldr	r1, [sp, #100]                  @ 4-byte Reload
	ldr	r3, [sp, #80]                   @ 4-byte Reload
	adcs	r1, r1, r3
	str	r1, [sp, #104]                  @ 4-byte Spill
	ldr	r1, [sp, #96]                   @ 4-byte Reload
	ldr	r3, [sp, #84]                   @ 4-byte Reload
	adcs	r1, r1, r3
	str	r1, [sp, #100]                  @ 4-byte Spill
	ldr	r1, [sp, #92]                   @ 4-byte Reload
	ldr	r3, [sp, #88]                   @ 4-byte Reload
	adcs	r1, r3, r1
	str	r1, [sp, #96]                   @ 4-byte Spill
	adc	r0, r0, #0
	str	r0, [sp, #92]                   @ 4-byte Spill
	ldr	r0, [r7, #84]
	mov	r1, r4
	str	r0, [sp, #88]                   @ 4-byte Spill
	ldr	r0, [sp, #1020]
	str	r0, [sp, #84]                   @ 4-byte Spill
	ldr	r0, [sp, #1016]
	str	r0, [sp, #80]                   @ 4-byte Spill
	ldr	r0, [sp, #1012]
	str	r0, [sp, #76]                   @ 4-byte Spill
	ldr	r0, [sp, #1008]
	str	r0, [sp, #72]                   @ 4-byte Spill
	add	r0, sp, #888
	ldr	r11, [sp, #1004]
	bl	mulPv512x32
	ldr	r6, [sp, #888]
	add	r7, sp, #892
	add	lr, sp, #912
	ldr	r8, [sp, #924]
	adds	r6, r10, r6
	ldm	r7, {r0, r1, r2, r5, r7}
	ldr	r6, [sp, #148]                  @ 4-byte Reload
	ldm	lr, {r3, r12, lr}
	adcs	r9, r6, r0
	ldr	r0, [sp, #144]                  @ 4-byte Reload
	adcs	r0, r0, r1
	str	r0, [sp, #148]                  @ 4-byte Spill
	ldr	r0, [sp, #140]                  @ 4-byte Reload
	adcs	r0, r0, r2
	str	r0, [sp, #144]                  @ 4-byte Spill
	ldr	r0, [sp, #136]                  @ 4-byte Reload
	adcs	r0, r0, r5
	str	r0, [sp, #140]                  @ 4-byte Spill
	ldr	r0, [sp, #132]                  @ 4-byte Reload
	adcs	r0, r0, r7
	str	r0, [sp, #136]                  @ 4-byte Spill
	ldr	r0, [sp, #128]                  @ 4-byte Reload
	adcs	r0, r0, r3
	str	r0, [sp, #132]                  @ 4-byte Spill
	ldr	r0, [sp, #124]                  @ 4-byte Reload
	adcs	r0, r0, r12
	str	r0, [sp, #128]                  @ 4-byte Spill
	ldr	r0, [sp, #120]                  @ 4-byte Reload
	adcs	r0, r0, lr
	str	r0, [sp, #124]                  @ 4-byte Spill
	ldr	r0, [sp, #116]                  @ 4-byte Reload
	adcs	r0, r0, r8
	str	r0, [sp, #120]                  @ 4-byte Spill
	mrs	r0, apsr
	str	r0, [sp, #152]                  @ 4-byte Spill
	ldr	r0, [sp, #156]                  @ 4-byte Reload
	msr	APSR_nzcvq, r0
	ldr	r1, [sp, #112]                  @ 4-byte Reload
	ldr	r0, [sp, #952]
	adcs	r1, r1, r11
	str	r1, [sp, #116]                  @ 4-byte Spill
	ldr	r1, [sp, #108]                  @ 4-byte Reload
	ldr	r3, [sp, #72]                   @ 4-byte Reload
	ldr	r8, [sp, #236]                  @ 4-byte Reload
	adcs	r1, r1, r3
	str	r1, [sp, #112]                  @ 4-byte Spill
	ldr	r1, [sp, #104]                  @ 4-byte Reload
	ldr	r3, [sp, #76]                   @ 4-byte Reload
	ldr	r6, [sp, #228]                  @ 4-byte Reload
	adcs	r1, r1, r3
	str	r1, [sp, #108]                  @ 4-byte Spill
	ldr	r1, [sp, #100]                  @ 4-byte Reload
	ldr	r3, [sp, #80]                   @ 4-byte Reload
	mul	r2, r6, r9
	adcs	r1, r1, r3
	str	r1, [sp, #104]                  @ 4-byte Spill
	ldr	r1, [sp, #96]                   @ 4-byte Reload
	ldr	r3, [sp, #84]                   @ 4-byte Reload
	adcs	r1, r1, r3
	str	r1, [sp, #100]                  @ 4-byte Spill
	ldr	r1, [sp, #92]                   @ 4-byte Reload
	ldr	r3, [sp, #88]                   @ 4-byte Reload
	adcs	r1, r3, r1
	str	r1, [sp, #96]                   @ 4-byte Spill
	adc	r0, r0, #0
	str	r0, [sp, #92]                   @ 4-byte Spill
	ldr	r0, [r8, #88]
	mov	r1, r4
	str	r0, [sp, #88]                   @ 4-byte Spill
	ldr	r0, [sp, #948]
	str	r0, [sp, #84]                   @ 4-byte Spill
	ldr	r0, [sp, #944]
	str	r0, [sp, #80]                   @ 4-byte Spill
	ldr	r0, [sp, #940]
	str	r0, [sp, #76]                   @ 4-byte Spill
	ldr	r0, [sp, #936]
	str	r0, [sp, #72]                   @ 4-byte Spill
	add	r0, sp, #816
	ldr	r11, [sp, #932]
	ldr	r10, [sp, #928]
	bl	mulPv512x32
	ldr	r4, [sp, #816]
	add	r5, sp, #824
	ldr	r7, [sp, #820]
	adds	r4, r9, r4
	ldm	r5, {r0, r1, r5}
	ldr	r4, [sp, #148]                  @ 4-byte Reload
	ldr	r12, [sp, #848]
	adcs	r4, r4, r7
	ldr	r7, [sp, #144]                  @ 4-byte Reload
	ldr	lr, [sp, #844]
	adcs	r0, r7, r0
	ldr	r2, [sp, #840]
	ldr	r3, [sp, #836]
	str	r0, [sp, #148]                  @ 4-byte Spill
	ldr	r0, [sp, #140]                  @ 4-byte Reload
	adcs	r0, r0, r1
	str	r0, [sp, #144]                  @ 4-byte Spill
	ldr	r0, [sp, #136]                  @ 4-byte Reload
	adcs	r0, r0, r5
	str	r0, [sp, #140]                  @ 4-byte Spill
	ldr	r0, [sp, #132]                  @ 4-byte Reload
	adcs	r0, r0, r3
	str	r0, [sp, #136]                  @ 4-byte Spill
	ldr	r0, [sp, #128]                  @ 4-byte Reload
	adcs	r0, r0, r2
	str	r0, [sp, #132]                  @ 4-byte Spill
	ldr	r0, [sp, #124]                  @ 4-byte Reload
	mul	r2, r6, r4
	adcs	r0, r0, lr
	str	r0, [sp, #128]                  @ 4-byte Spill
	ldr	r0, [sp, #120]                  @ 4-byte Reload
	adcs	r0, r0, r12
	str	r0, [sp, #124]                  @ 4-byte Spill
	mrs	r0, apsr
	str	r0, [sp, #156]                  @ 4-byte Spill
	ldr	r0, [sp, #152]                  @ 4-byte Reload
	msr	APSR_nzcvq, r0
	ldr	r1, [sp, #116]                  @ 4-byte Reload
	ldr	r0, [sp, #880]
	adcs	r1, r1, r10
	str	r1, [sp, #120]                  @ 4-byte Spill
	ldr	r1, [sp, #112]                  @ 4-byte Reload
	ldr	r3, [sp, #72]                   @ 4-byte Reload
	adcs	r1, r1, r11
	str	r1, [sp, #116]                  @ 4-byte Spill
	ldr	r1, [sp, #108]                  @ 4-byte Reload
	ldr	r10, [sp, #232]                 @ 4-byte Reload
	adcs	r1, r1, r3
	str	r1, [sp, #112]                  @ 4-byte Spill
	ldr	r1, [sp, #104]                  @ 4-byte Reload
	ldr	r3, [sp, #76]                   @ 4-byte Reload
	adcs	r1, r1, r3
	str	r1, [sp, #108]                  @ 4-byte Spill
	ldr	r1, [sp, #100]                  @ 4-byte Reload
	ldr	r3, [sp, #80]                   @ 4-byte Reload
	adcs	r1, r1, r3
	str	r1, [sp, #104]                  @ 4-byte Spill
	ldr	r1, [sp, #96]                   @ 4-byte Reload
	ldr	r3, [sp, #84]                   @ 4-byte Reload
	adcs	r1, r1, r3
	str	r1, [sp, #100]                  @ 4-byte Spill
	ldr	r1, [sp, #92]                   @ 4-byte Reload
	ldr	r3, [sp, #88]                   @ 4-byte Reload
	adcs	r1, r3, r1
	str	r1, [sp, #96]                   @ 4-byte Spill
	adc	r0, r0, #0
	str	r0, [sp, #92]                   @ 4-byte Spill
	ldr	r0, [r8, #92]
	mov	r1, r10
	str	r0, [sp, #88]                   @ 4-byte Spill
	ldr	r0, [sp, #876]
	str	r0, [sp, #84]                   @ 4-byte Spill
	ldr	r0, [sp, #872]
	str	r0, [sp, #80]                   @ 4-byte Spill
	ldr	r0, [sp, #868]
	str	r0, [sp, #76]                   @ 4-byte Spill
	ldr	r0, [sp, #864]
	str	r0, [sp, #72]                   @ 4-byte Spill
	ldr	r0, [sp, #860]
	str	r0, [sp, #68]                   @ 4-byte Spill
	ldr	r0, [sp, #852]
	ldr	r8, [sp, #856]
	str	r0, [sp, #64]                   @ 4-byte Spill
	add	r0, sp, #744
	bl	mulPv512x32
	add	r7, sp, #744
	ldr	r12, [sp, #772]
	ldr	r1, [sp, #768]
	ldm	r7, {r5, r6, r7}
	adds	r5, r4, r5
	ldr	r2, [sp, #764]
	ldr	r5, [sp, #148]                  @ 4-byte Reload
	ldr	r3, [sp, #760]
	adcs	r9, r5, r6
	ldr	r6, [sp, #144]                  @ 4-byte Reload
	ldr	r0, [sp, #756]
	adcs	r7, r6, r7
	str	r7, [sp, #148]                  @ 4-byte Spill
	ldr	r7, [sp, #140]                  @ 4-byte Reload
	adcs	r0, r7, r0
	str	r0, [sp, #144]                  @ 4-byte Spill
	ldr	r0, [sp, #136]                  @ 4-byte Reload
	adcs	r0, r0, r3
	str	r0, [sp, #140]                  @ 4-byte Spill
	ldr	r0, [sp, #132]                  @ 4-byte Reload
	adcs	r0, r0, r2
	str	r0, [sp, #136]                  @ 4-byte Spill
	ldr	r0, [sp, #128]                  @ 4-byte Reload
	adcs	r0, r0, r1
	str	r0, [sp, #132]                  @ 4-byte Spill
	ldr	r0, [sp, #124]                  @ 4-byte Reload
	adcs	r0, r0, r12
	str	r0, [sp, #128]                  @ 4-byte Spill
	mrs	r0, apsr
	str	r0, [sp, #152]                  @ 4-byte Spill
	ldr	r0, [sp, #156]                  @ 4-byte Reload
	msr	APSR_nzcvq, r0
	ldr	r1, [sp, #120]                  @ 4-byte Reload
	ldr	r3, [sp, #64]                   @ 4-byte Reload
	ldr	r0, [sp, #808]
	adcs	r1, r1, r3
	str	r1, [sp, #124]                  @ 4-byte Spill
	ldr	r1, [sp, #116]                  @ 4-byte Reload
	ldr	r3, [sp, #68]                   @ 4-byte Reload
	adcs	r1, r1, r8
	str	r1, [sp, #120]                  @ 4-byte Spill
	ldr	r1, [sp, #112]                  @ 4-byte Reload
	ldr	r4, [sp, #236]                  @ 4-byte Reload
	adcs	r1, r1, r3
	str	r1, [sp, #116]                  @ 4-byte Spill
	ldr	r1, [sp, #108]                  @ 4-byte Reload
	ldr	r3, [sp, #72]                   @ 4-byte Reload
	ldr	r11, [sp, #228]                 @ 4-byte Reload
	adcs	r1, r1, r3
	str	r1, [sp, #112]                  @ 4-byte Spill
	ldr	r1, [sp, #104]                  @ 4-byte Reload
	ldr	r3, [sp, #76]                   @ 4-byte Reload
	mul	r2, r11, r9
	adcs	r1, r1, r3
	str	r1, [sp, #108]                  @ 4-byte Spill
	ldr	r1, [sp, #100]                  @ 4-byte Reload
	ldr	r3, [sp, #80]                   @ 4-byte Reload
	adcs	r1, r1, r3
	str	r1, [sp, #104]                  @ 4-byte Spill
	ldr	r1, [sp, #96]                   @ 4-byte Reload
	ldr	r3, [sp, #84]                   @ 4-byte Reload
	adcs	r1, r1, r3
	str	r1, [sp, #100]                  @ 4-byte Spill
	ldr	r1, [sp, #92]                   @ 4-byte Reload
	ldr	r3, [sp, #88]                   @ 4-byte Reload
	adcs	r1, r3, r1
	str	r1, [sp, #96]                   @ 4-byte Spill
	adc	r0, r0, #0
	str	r0, [sp, #92]                   @ 4-byte Spill
	ldr	r0, [r4, #96]
	mov	r1, r10
	str	r0, [sp, #88]                   @ 4-byte Spill
	ldr	r0, [sp, #804]
	str	r0, [sp, #84]                   @ 4-byte Spill
	ldr	r0, [sp, #800]
	str	r0, [sp, #80]                   @ 4-byte Spill
	ldr	r0, [sp, #796]
	str	r0, [sp, #76]                   @ 4-byte Spill
	ldr	r0, [sp, #792]
	str	r0, [sp, #72]                   @ 4-byte Spill
	ldr	r0, [sp, #788]
	str	r0, [sp, #68]                   @ 4-byte Spill
	ldr	r0, [sp, #784]
	str	r0, [sp, #64]                   @ 4-byte Spill
	ldr	r0, [sp, #780]
	str	r0, [sp, #60]                   @ 4-byte Spill
	ldr	r0, [sp, #776]
	str	r0, [sp, #56]                   @ 4-byte Spill
	add	r0, sp, #672
	bl	mulPv512x32
	add	r7, sp, #672
	ldr	r0, [sp, #696]
	ldr	r1, [sp, #692]
	ldm	r7, {r3, r5, r6, r7}
	adds	r3, r9, r3
	ldr	r2, [sp, #688]
	ldr	r3, [sp, #148]                  @ 4-byte Reload
	adcs	r8, r3, r5
	ldr	r3, [sp, #144]                  @ 4-byte Reload
	adcs	r3, r3, r6
	str	r3, [sp, #148]                  @ 4-byte Spill
	ldr	r3, [sp, #140]                  @ 4-byte Reload
	adcs	r3, r3, r7
	str	r3, [sp, #144]                  @ 4-byte Spill
	ldr	r3, [sp, #136]                  @ 4-byte Reload
	adcs	r2, r3, r2
	str	r2, [sp, #140]                  @ 4-byte Spill
	ldr	r2, [sp, #132]                  @ 4-byte Reload
	adcs	r1, r2, r1
	str	r1, [sp, #136]                  @ 4-byte Spill
	ldr	r1, [sp, #128]                  @ 4-byte Reload
	mul	r2, r11, r8
	adcs	r0, r1, r0
	str	r0, [sp, #132]                  @ 4-byte Spill
	mrs	r0, apsr
	str	r0, [sp, #156]                  @ 4-byte Spill
	ldr	r0, [sp, #152]                  @ 4-byte Reload
	msr	APSR_nzcvq, r0
	ldr	r1, [sp, #124]                  @ 4-byte Reload
	ldr	r3, [sp, #56]                   @ 4-byte Reload
	ldr	r0, [sp, #736]
	adcs	r1, r1, r3
	str	r1, [sp, #128]                  @ 4-byte Spill
	ldr	r1, [sp, #120]                  @ 4-byte Reload
	ldr	r3, [sp, #60]                   @ 4-byte Reload
	ldr	r5, [sp, #232]                  @ 4-byte Reload
	adcs	r1, r1, r3
	str	r1, [sp, #124]                  @ 4-byte Spill
	ldr	r1, [sp, #116]                  @ 4-byte Reload
	ldr	r3, [sp, #64]                   @ 4-byte Reload
	adcs	r1, r1, r3
	str	r1, [sp, #120]                  @ 4-byte Spill
	ldr	r1, [sp, #112]                  @ 4-byte Reload
	ldr	r3, [sp, #68]                   @ 4-byte Reload
	adcs	r1, r1, r3
	str	r1, [sp, #116]                  @ 4-byte Spill
	ldr	r1, [sp, #108]                  @ 4-byte Reload
	ldr	r3, [sp, #72]                   @ 4-byte Reload
	adcs	r1, r1, r3
	str	r1, [sp, #112]                  @ 4-byte Spill
	ldr	r1, [sp, #104]                  @ 4-byte Reload
	ldr	r3, [sp, #76]                   @ 4-byte Reload
	adcs	r1, r1, r3
	str	r1, [sp, #108]                  @ 4-byte Spill
	ldr	r1, [sp, #100]                  @ 4-byte Reload
	ldr	r3, [sp, #80]                   @ 4-byte Reload
	adcs	r1, r1, r3
	str	r1, [sp, #104]                  @ 4-byte Spill
	ldr	r1, [sp, #96]                   @ 4-byte Reload
	ldr	r3, [sp, #84]                   @ 4-byte Reload
	adcs	r1, r1, r3
	str	r1, [sp, #100]                  @ 4-byte Spill
	ldr	r1, [sp, #92]                   @ 4-byte Reload
	ldr	r3, [sp, #88]                   @ 4-byte Reload
	adcs	r1, r3, r1
	str	r1, [sp, #96]                   @ 4-byte Spill
	adc	r0, r0, #0
	str	r0, [sp, #92]                   @ 4-byte Spill
	ldr	r0, [r4, #100]
	mov	r1, r5
	str	r0, [sp, #88]                   @ 4-byte Spill
	ldr	r0, [sp, #732]
	str	r0, [sp, #84]                   @ 4-byte Spill
	ldr	r0, [sp, #728]
	str	r0, [sp, #80]                   @ 4-byte Spill
	ldr	r0, [sp, #724]
	str	r0, [sp, #76]                   @ 4-byte Spill
	ldr	r0, [sp, #720]
	str	r0, [sp, #72]                   @ 4-byte Spill
	ldr	r0, [sp, #716]
	str	r0, [sp, #68]                   @ 4-byte Spill
	ldr	r0, [sp, #712]
	str	r0, [sp, #64]                   @ 4-byte Spill
	ldr	r0, [sp, #704]
	ldr	r9, [sp, #708]
	str	r0, [sp, #60]                   @ 4-byte Spill
	add	r0, sp, #600
	ldr	r10, [sp, #700]
	bl	mulPv512x32
	add	r7, sp, #600
	ldr	r0, [sp, #620]
	ldr	r1, [sp, #616]
	ldm	r7, {r2, r3, r6, r7}
	adds	r2, r8, r2
	ldr	r2, [sp, #148]                  @ 4-byte Reload
	adcs	r4, r2, r3
	ldr	r2, [sp, #144]                  @ 4-byte Reload
	adcs	r2, r2, r6
	str	r2, [sp, #148]                  @ 4-byte Spill
	ldr	r2, [sp, #140]                  @ 4-byte Reload
	adcs	r2, r2, r7
	str	r2, [sp, #144]                  @ 4-byte Spill
	ldr	r2, [sp, #136]                  @ 4-byte Reload
	adcs	r1, r2, r1
	str	r1, [sp, #140]                  @ 4-byte Spill
	ldr	r1, [sp, #132]                  @ 4-byte Reload
	mul	r2, r11, r4
	adcs	r0, r1, r0
	str	r0, [sp, #136]                  @ 4-byte Spill
	mrs	r0, apsr
	str	r0, [sp, #152]                  @ 4-byte Spill
	ldr	r0, [sp, #156]                  @ 4-byte Reload
	msr	APSR_nzcvq, r0
	ldr	r1, [sp, #128]                  @ 4-byte Reload
	ldr	r0, [sp, #664]
	adcs	r1, r1, r10
	str	r1, [sp, #156]                  @ 4-byte Spill
	ldr	r1, [sp, #124]                  @ 4-byte Reload
	ldr	r3, [sp, #60]                   @ 4-byte Reload
	ldr	r7, [sp, #236]                  @ 4-byte Reload
	adcs	r1, r1, r3
	str	r1, [sp, #132]                  @ 4-byte Spill
	ldr	r1, [sp, #120]                  @ 4-byte Reload
	ldr	r3, [sp, #64]                   @ 4-byte Reload
	adcs	r1, r1, r9
	str	r1, [sp, #128]                  @ 4-byte Spill
	ldr	r1, [sp, #116]                  @ 4-byte Reload
	adcs	r1, r1, r3
	str	r1, [sp, #124]                  @ 4-byte Spill
	ldr	r1, [sp, #112]                  @ 4-byte Reload
	ldr	r3, [sp, #68]                   @ 4-byte Reload
	adcs	r1, r1, r3
	str	r1, [sp, #120]                  @ 4-byte Spill
	ldr	r1, [sp, #108]                  @ 4-byte Reload
	ldr	r3, [sp, #72]                   @ 4-byte Reload
	adcs	r1, r1, r3
	str	r1, [sp, #116]                  @ 4-byte Spill
	ldr	r1, [sp, #104]                  @ 4-byte Reload
	ldr	r3, [sp, #76]                   @ 4-byte Reload
	adcs	r1, r1, r3
	str	r1, [sp, #112]                  @ 4-byte Spill
	ldr	r1, [sp, #100]                  @ 4-byte Reload
	ldr	r3, [sp, #80]                   @ 4-byte Reload
	adcs	r1, r1, r3
	str	r1, [sp, #108]                  @ 4-byte Spill
	ldr	r1, [sp, #96]                   @ 4-byte Reload
	ldr	r3, [sp, #84]                   @ 4-byte Reload
	adcs	r1, r1, r3
	str	r1, [sp, #104]                  @ 4-byte Spill
	ldr	r1, [sp, #92]                   @ 4-byte Reload
	ldr	r3, [sp, #88]                   @ 4-byte Reload
	adcs	r1, r3, r1
	str	r1, [sp, #100]                  @ 4-byte Spill
	adc	r0, r0, #0
	str	r0, [sp, #96]                   @ 4-byte Spill
	ldr	r0, [r7, #104]
	mov	r1, r5
	str	r0, [sp, #80]                   @ 4-byte Spill
	ldr	r0, [sp, #660]
	str	r0, [sp, #76]                   @ 4-byte Spill
	ldr	r0, [sp, #656]
	str	r0, [sp, #72]                   @ 4-byte Spill
	ldr	r0, [sp, #652]
	str	r0, [sp, #68]                   @ 4-byte Spill
	ldr	r0, [sp, #648]
	str	r0, [sp, #64]                   @ 4-byte Spill
	ldr	r0, [sp, #644]
	str	r0, [sp, #60]                   @ 4-byte Spill
	ldr	r0, [sp, #640]
	str	r0, [sp, #56]                   @ 4-byte Spill
	ldr	r0, [sp, #636]
	str	r0, [sp, #52]                   @ 4-byte Spill
	add	r0, sp, #528
	ldr	r8, [sp, #632]
	ldr	r10, [sp, #628]
	ldr	r9, [sp, #624]
	bl	mulPv512x32
	add	r6, sp, #528
	ldr	r0, [sp, #544]
	mov	r5, r7
	ldm	r6, {r1, r2, r3, r6}
	adds	r1, r4, r1
	ldr	r1, [sp, #148]                  @ 4-byte Reload
	adcs	r4, r1, r2
	ldr	r1, [sp, #144]                  @ 4-byte Reload
	adcs	r1, r1, r3
	str	r1, [sp, #92]                   @ 4-byte Spill
	ldr	r1, [sp, #140]                  @ 4-byte Reload
	mul	r2, r11, r4
	adcs	r1, r1, r6
	str	r1, [sp, #88]                   @ 4-byte Spill
	ldr	r1, [sp, #136]                  @ 4-byte Reload
	adcs	r0, r1, r0
	str	r0, [sp, #84]                   @ 4-byte Spill
	mrs	r0, apsr
	str	r0, [sp, #148]                  @ 4-byte Spill
	ldr	r0, [sp, #152]                  @ 4-byte Reload
	msr	APSR_nzcvq, r0
	ldr	r1, [sp, #156]                  @ 4-byte Reload
	ldr	r0, [sp, #592]
	adcs	r1, r1, r9
	str	r1, [sp, #152]                  @ 4-byte Spill
	ldr	r1, [sp, #132]                  @ 4-byte Reload
	ldr	r3, [sp, #52]                   @ 4-byte Reload
	adcs	r1, r1, r10
	str	r1, [sp, #144]                  @ 4-byte Spill
	ldr	r1, [sp, #128]                  @ 4-byte Reload
	add	r10, sp, #548
	ldr	r6, [sp, #232]                  @ 4-byte Reload
	adcs	r1, r1, r8
	str	r1, [sp, #140]                  @ 4-byte Spill
	ldr	r1, [sp, #124]                  @ 4-byte Reload
	adcs	r1, r1, r3
	str	r1, [sp, #136]                  @ 4-byte Spill
	ldr	r1, [sp, #120]                  @ 4-byte Reload
	ldr	r3, [sp, #56]                   @ 4-byte Reload
	adcs	r1, r1, r3
	str	r1, [sp, #132]                  @ 4-byte Spill
	ldr	r1, [sp, #116]                  @ 4-byte Reload
	ldr	r3, [sp, #60]                   @ 4-byte Reload
	adcs	r1, r1, r3
	str	r1, [sp, #128]                  @ 4-byte Spill
	ldr	r1, [sp, #112]                  @ 4-byte Reload
	ldr	r3, [sp, #64]                   @ 4-byte Reload
	adcs	r1, r1, r3
	str	r1, [sp, #124]                  @ 4-byte Spill
	ldr	r1, [sp, #108]                  @ 4-byte Reload
	ldr	r3, [sp, #68]                   @ 4-byte Reload
	adcs	r1, r1, r3
	str	r1, [sp, #120]                  @ 4-byte Spill
	ldr	r1, [sp, #104]                  @ 4-byte Reload
	ldr	r3, [sp, #72]                   @ 4-byte Reload
	adcs	r1, r1, r3
	str	r1, [sp, #116]                  @ 4-byte Spill
	ldr	r1, [sp, #100]                  @ 4-byte Reload
	ldr	r3, [sp, #76]                   @ 4-byte Reload
	adcs	r1, r1, r3
	str	r1, [sp, #112]                  @ 4-byte Spill
	ldr	r1, [sp, #96]                   @ 4-byte Reload
	ldr	r3, [sp, #80]                   @ 4-byte Reload
	adcs	r1, r3, r1
	str	r1, [sp, #108]                  @ 4-byte Spill
	adc	r0, r0, #0
	str	r0, [sp, #104]                  @ 4-byte Spill
	ldr	r0, [r7, #108]
	mov	r1, r6
	str	r0, [sp, #100]                  @ 4-byte Spill
	ldr	r0, [sp, #588]
	str	r0, [sp, #80]                   @ 4-byte Spill
	ldr	r0, [sp, #584]
	str	r0, [sp, #76]                   @ 4-byte Spill
	ldr	r0, [sp, #580]
	str	r0, [sp, #72]                   @ 4-byte Spill
	ldr	r0, [sp, #576]
	str	r0, [sp, #68]                   @ 4-byte Spill
	ldr	r0, [sp, #572]
	str	r0, [sp, #64]                   @ 4-byte Spill
	ldr	r0, [sp, #568]
	str	r0, [sp, #60]                   @ 4-byte Spill
	ldr	r0, [sp, #564]
	str	r0, [sp, #56]                   @ 4-byte Spill
	add	r0, sp, #456
	ldm	r10, {r7, r8, r10}
	ldr	r9, [sp, #560]
	bl	mulPv512x32
	add	r3, sp, #456
	ldm	r3, {r0, r1, r2, r3}
	adds	r0, r4, r0
	ldr	r0, [sp, #92]                   @ 4-byte Reload
	adcs	r4, r0, r1
	ldr	r0, [sp, #88]                   @ 4-byte Reload
	adcs	r0, r0, r2
	str	r0, [sp, #96]                   @ 4-byte Spill
	ldr	r0, [sp, #84]                   @ 4-byte Reload
	mul	r2, r11, r4
	adcs	r0, r0, r3
	str	r0, [sp, #92]                   @ 4-byte Spill
	mrs	r0, apsr
	str	r0, [sp, #156]                  @ 4-byte Spill
	ldr	r0, [sp, #148]                  @ 4-byte Reload
	msr	APSR_nzcvq, r0
	ldr	r3, [sp, #152]                  @ 4-byte Reload
	ldr	r1, [sp, #520]
	add	r0, sp, #384
	adcs	r3, r3, r7
	str	r3, [sp, #152]                  @ 4-byte Spill
	ldr	r3, [sp, #144]                  @ 4-byte Reload
	ldr	r7, [sp, #56]                   @ 4-byte Reload
	adcs	r3, r3, r8
	str	r3, [sp, #148]                  @ 4-byte Spill
	ldr	r3, [sp, #140]                  @ 4-byte Reload
	adcs	r3, r3, r10
	str	r3, [sp, #144]                  @ 4-byte Spill
	ldr	r3, [sp, #136]                  @ 4-byte Reload
	adcs	r3, r3, r9
	str	r3, [sp, #140]                  @ 4-byte Spill
	ldr	r3, [sp, #132]                  @ 4-byte Reload
	adcs	r3, r3, r7
	str	r3, [sp, #136]                  @ 4-byte Spill
	ldr	r3, [sp, #128]                  @ 4-byte Reload
	ldr	r7, [sp, #60]                   @ 4-byte Reload
	adcs	r3, r3, r7
	str	r3, [sp, #132]                  @ 4-byte Spill
	ldr	r3, [sp, #124]                  @ 4-byte Reload
	ldr	r7, [sp, #64]                   @ 4-byte Reload
	adcs	r3, r3, r7
	str	r3, [sp, #128]                  @ 4-byte Spill
	ldr	r3, [sp, #120]                  @ 4-byte Reload
	ldr	r7, [sp, #68]                   @ 4-byte Reload
	adcs	r3, r3, r7
	str	r3, [sp, #124]                  @ 4-byte Spill
	ldr	r3, [sp, #116]                  @ 4-byte Reload
	ldr	r7, [sp, #72]                   @ 4-byte Reload
	adcs	r3, r3, r7
	str	r3, [sp, #120]                  @ 4-byte Spill
	ldr	r3, [sp, #112]                  @ 4-byte Reload
	ldr	r7, [sp, #76]                   @ 4-byte Reload
	adcs	r3, r3, r7
	str	r3, [sp, #116]                  @ 4-byte Spill
	ldr	r3, [sp, #108]                  @ 4-byte Reload
	ldr	r7, [sp, #80]                   @ 4-byte Reload
	adcs	r3, r3, r7
	str	r3, [sp, #112]                  @ 4-byte Spill
	ldr	r3, [sp, #104]                  @ 4-byte Reload
	ldr	r7, [sp, #100]                  @ 4-byte Reload
	adcs	r3, r7, r3
	str	r3, [sp, #108]                  @ 4-byte Spill
	adc	r1, r1, #0
	str	r1, [sp, #104]                  @ 4-byte Spill
	ldr	r1, [r5, #112]
	mov	r5, r6
	str	r1, [sp, #88]                   @ 4-byte Spill
	ldr	r1, [sp, #516]
	str	r1, [sp, #80]                   @ 4-byte Spill
	ldr	r1, [sp, #512]
	str	r1, [sp, #72]                   @ 4-byte Spill
	ldr	r1, [sp, #508]
	str	r1, [sp, #68]                   @ 4-byte Spill
	ldr	r1, [sp, #504]
	str	r1, [sp, #64]                   @ 4-byte Spill
	ldr	r1, [sp, #500]
	str	r1, [sp, #56]                   @ 4-byte Spill
	ldr	r1, [sp, #496]
	str	r1, [sp, #52]                   @ 4-byte Spill
	ldr	r1, [sp, #492]
	str	r1, [sp, #48]                   @ 4-byte Spill
	mov	r1, r6
	ldr	r8, [sp, #488]
	ldr	r9, [sp, #484]
	ldr	r7, [sp, #480]
	ldr	r10, [sp, #476]
	ldr	r11, [sp, #472]
	bl	mulPv512x32
	add	r3, sp, #384
	ldm	r3, {r0, r1, r2, r3}
	adds	r0, r4, r0
	str	r3, [sp, #100]                  @ 4-byte Spill
	ldr	r0, [sp, #96]                   @ 4-byte Reload
	adcs	r4, r0, r1
	ldr	r0, [sp, #92]                   @ 4-byte Reload
	adcs	r0, r0, r2
	str	r0, [sp, #76]                   @ 4-byte Spill
	mrs	r0, apsr
	str	r0, [sp, #84]                   @ 4-byte Spill
	ldr	r0, [sp, #156]                  @ 4-byte Reload
	msr	APSR_nzcvq, r0
	ldr	r3, [sp, #152]                  @ 4-byte Reload
	ldr	r1, [sp, #448]
	add	r0, sp, #312
	adcs	r3, r3, r11
	str	r3, [sp, #60]                   @ 4-byte Spill
	ldr	r3, [sp, #148]                  @ 4-byte Reload
	ldr	r6, [sp, #228]                  @ 4-byte Reload
	adcs	r10, r3, r10
	ldr	r3, [sp, #144]                  @ 4-byte Reload
	adcs	r11, r3, r7
	ldr	r3, [sp, #140]                  @ 4-byte Reload
	ldr	r7, [sp, #48]                   @ 4-byte Reload
	mul	r2, r6, r4
	adcs	r3, r3, r9
	str	r3, [sp, #152]                  @ 4-byte Spill
	ldr	r3, [sp, #136]                  @ 4-byte Reload
	adcs	r8, r3, r8
	ldr	r3, [sp, #132]                  @ 4-byte Reload
	adcs	r3, r3, r7
	str	r3, [sp, #148]                  @ 4-byte Spill
	ldr	r3, [sp, #128]                  @ 4-byte Reload
	ldr	r7, [sp, #52]                   @ 4-byte Reload
	adcs	r3, r3, r7
	str	r3, [sp, #144]                  @ 4-byte Spill
	ldr	r3, [sp, #124]                  @ 4-byte Reload
	ldr	r7, [sp, #56]                   @ 4-byte Reload
	adcs	r3, r3, r7
	str	r3, [sp, #140]                  @ 4-byte Spill
	ldr	r3, [sp, #120]                  @ 4-byte Reload
	ldr	r7, [sp, #64]                   @ 4-byte Reload
	adcs	r3, r3, r7
	str	r3, [sp, #136]                  @ 4-byte Spill
	ldr	r3, [sp, #116]                  @ 4-byte Reload
	ldr	r7, [sp, #68]                   @ 4-byte Reload
	adcs	r3, r3, r7
	str	r3, [sp, #132]                  @ 4-byte Spill
	ldr	r3, [sp, #112]                  @ 4-byte Reload
	ldr	r7, [sp, #72]                   @ 4-byte Reload
	adcs	r3, r3, r7
	str	r3, [sp, #128]                  @ 4-byte Spill
	ldr	r3, [sp, #108]                  @ 4-byte Reload
	ldr	r7, [sp, #80]                   @ 4-byte Reload
	adcs	r3, r3, r7
	str	r3, [sp, #124]                  @ 4-byte Spill
	ldr	r3, [sp, #104]                  @ 4-byte Reload
	ldr	r7, [sp, #88]                   @ 4-byte Reload
	adcs	r3, r7, r3
	ldr	r7, [sp, #236]                  @ 4-byte Reload
	adc	r1, r1, #0
	str	r1, [sp, #116]                  @ 4-byte Spill
	str	r3, [sp, #120]                  @ 4-byte Spill
	ldr	r1, [r7, #116]
	str	r1, [sp, #112]                  @ 4-byte Spill
	ldr	r1, [sp, #444]
	str	r1, [sp, #108]                  @ 4-byte Spill
	ldr	r1, [sp, #440]
	str	r1, [sp, #104]                  @ 4-byte Spill
	ldr	r1, [sp, #436]
	str	r1, [sp, #96]                   @ 4-byte Spill
	ldr	r1, [sp, #432]
	str	r1, [sp, #92]                   @ 4-byte Spill
	ldr	r1, [sp, #428]
	str	r1, [sp, #88]                   @ 4-byte Spill
	ldr	r1, [sp, #424]
	str	r1, [sp, #80]                   @ 4-byte Spill
	ldr	r1, [sp, #420]
	str	r1, [sp, #64]                   @ 4-byte Spill
	ldr	r1, [sp, #416]
	str	r1, [sp, #56]                   @ 4-byte Spill
	ldr	r1, [sp, #412]
	str	r1, [sp, #48]                   @ 4-byte Spill
	ldr	r1, [sp, #408]
	str	r1, [sp, #36]                   @ 4-byte Spill
	ldr	r1, [sp, #404]
	str	r1, [sp, #24]                   @ 4-byte Spill
	mov	r1, r5
	ldr	r9, [sp, #400]
	bl	mulPv512x32
	add	r2, sp, #312
	ldm	r2, {r0, r1, r2}
	adds	r0, r4, r0
	str	r2, [sp, #72]                   @ 4-byte Spill
	ldr	r0, [sp, #76]                   @ 4-byte Reload
	ldr	r2, [sp, #324]
	adcs	r0, r0, r1
	str	r2, [sp, #68]                   @ 4-byte Spill
	str	r0, [sp, #156]                  @ 4-byte Spill
	mrs	r1, apsr
	str	r1, [sp, #44]                   @ 4-byte Spill
	mul	r2, r6, r0
	ldr	r1, [sp, #84]                   @ 4-byte Reload
	msr	APSR_nzcvq, r1
	ldr	r0, [sp, #100]                  @ 4-byte Reload
	ldr	r1, [sp, #60]                   @ 4-byte Reload
	ldr	r3, [sp, #24]                   @ 4-byte Reload
	adcs	r0, r1, r0
	str	r0, [sp, #32]                   @ 4-byte Spill
	ldr	r0, [r7, #120]
	adcs	r5, r10, r9
	str	r0, [sp, #228]                  @ 4-byte Spill
	adcs	r3, r11, r3
	ldr	r0, [sp, #372]
	str	r0, [sp, #100]                  @ 4-byte Spill
	ldr	r0, [sp, #368]
	str	r0, [sp, #84]                   @ 4-byte Spill
	ldr	r0, [sp, #364]
	str	r0, [sp, #76]                   @ 4-byte Spill
	ldr	r0, [sp, #360]
	str	r0, [sp, #60]                   @ 4-byte Spill
	ldr	r0, [sp, #356]
	str	r0, [sp, #52]                   @ 4-byte Spill
	ldr	r0, [sp, #352]
	str	r0, [sp, #40]                   @ 4-byte Spill
	ldr	r0, [sp, #348]
	str	r0, [sp, #28]                   @ 4-byte Spill
	ldr	r0, [sp, #344]
	str	r0, [sp, #20]                   @ 4-byte Spill
	ldr	r0, [sp, #340]
	str	r0, [sp, #16]                   @ 4-byte Spill
	ldr	r0, [sp, #336]
	str	r0, [sp, #12]                   @ 4-byte Spill
	ldr	r0, [sp, #332]
	str	r0, [sp, #8]                    @ 4-byte Spill
	ldr	r0, [sp, #328]
	str	r0, [sp, #4]                    @ 4-byte Spill
	add	r0, sp, #240
	ldr	r1, [sp, #376]
	str	r3, [sp, #24]                   @ 4-byte Spill
	ldr	r3, [sp, #152]                  @ 4-byte Reload
	ldr	r7, [sp, #36]                   @ 4-byte Reload
	ldr	r4, [sp, #80]                   @ 4-byte Reload
	adcs	r6, r3, r7
	ldr	r3, [sp, #48]                   @ 4-byte Reload
	ldr	r7, [sp, #56]                   @ 4-byte Reload
	adcs	r11, r8, r3
	ldr	r3, [sp, #148]                  @ 4-byte Reload
	adcs	r8, r3, r7
	ldr	r3, [sp, #144]                  @ 4-byte Reload
	ldr	r7, [sp, #64]                   @ 4-byte Reload
	adcs	r7, r3, r7
	ldr	r3, [sp, #140]                  @ 4-byte Reload
	adcs	r10, r3, r4
	ldr	r3, [sp, #136]                  @ 4-byte Reload
	ldr	r4, [sp, #88]                   @ 4-byte Reload
	adcs	r9, r3, r4
	ldr	r3, [sp, #132]                  @ 4-byte Reload
	ldr	r4, [sp, #92]                   @ 4-byte Reload
	adcs	r3, r3, r4
	str	r3, [sp, #152]                  @ 4-byte Spill
	ldr	r3, [sp, #128]                  @ 4-byte Reload
	ldr	r4, [sp, #96]                   @ 4-byte Reload
	adcs	r3, r3, r4
	str	r3, [sp, #148]                  @ 4-byte Spill
	ldr	r3, [sp, #124]                  @ 4-byte Reload
	ldr	r4, [sp, #104]                  @ 4-byte Reload
	adcs	r3, r3, r4
	str	r3, [sp, #144]                  @ 4-byte Spill
	ldr	r3, [sp, #120]                  @ 4-byte Reload
	ldr	r4, [sp, #108]                  @ 4-byte Reload
	adcs	r3, r3, r4
	str	r3, [sp, #140]                  @ 4-byte Spill
	ldr	r3, [sp, #116]                  @ 4-byte Reload
	ldr	r4, [sp, #112]                  @ 4-byte Reload
	adcs	r3, r4, r3
	str	r3, [sp, #136]                  @ 4-byte Spill
	adc	r1, r1, #0
	str	r1, [sp, #132]                  @ 4-byte Spill
	ldr	r1, [sp, #232]                  @ 4-byte Reload
	bl	mulPv512x32
	ldr	r0, [sp, #44]                   @ 4-byte Reload
	add	r3, sp, #244
	msr	APSR_nzcvq, r0
	ldr	r0, [sp, #72]                   @ 4-byte Reload
	ldr	r1, [sp, #32]                   @ 4-byte Reload
	adcs	r12, r1, r0
	ldr	r0, [sp, #68]                   @ 4-byte Reload
	ldr	r1, [sp, #24]                   @ 4-byte Reload
	adcs	r5, r5, r0
	ldr	r0, [sp, #4]                    @ 4-byte Reload
	adcs	r4, r1, r0
	ldr	r0, [sp, #8]                    @ 4-byte Reload
	ldr	r1, [sp, #152]                  @ 4-byte Reload
	adcs	r6, r6, r0
	ldr	r0, [sp, #12]                   @ 4-byte Reload
	adcs	lr, r11, r0
	ldr	r0, [sp, #16]                   @ 4-byte Reload
	adcs	r8, r8, r0
	ldr	r0, [sp, #20]                   @ 4-byte Reload
	adcs	r7, r7, r0
	ldr	r0, [sp, #28]                   @ 4-byte Reload
	adcs	r11, r10, r0
	ldr	r0, [sp, #40]                   @ 4-byte Reload
	ldr	r10, [sp, #156]                 @ 4-byte Reload
	adcs	r0, r9, r0
	str	r0, [sp, #108]                  @ 4-byte Spill
	ldr	r0, [sp, #52]                   @ 4-byte Reload
	adcs	r0, r1, r0
	str	r0, [sp, #104]                  @ 4-byte Spill
	ldr	r0, [sp, #60]                   @ 4-byte Reload
	ldr	r1, [sp, #148]                  @ 4-byte Reload
	adcs	r0, r1, r0
	str	r0, [sp, #232]                  @ 4-byte Spill
	ldr	r0, [sp, #76]                   @ 4-byte Reload
	ldr	r1, [sp, #144]                  @ 4-byte Reload
	adcs	r0, r1, r0
	str	r0, [sp, #112]                  @ 4-byte Spill
	ldr	r0, [sp, #84]                   @ 4-byte Reload
	ldr	r1, [sp, #140]                  @ 4-byte Reload
	adcs	r0, r1, r0
	str	r0, [sp, #116]                  @ 4-byte Spill
	ldr	r0, [sp, #100]                  @ 4-byte Reload
	ldr	r1, [sp, #136]                  @ 4-byte Reload
	adcs	r0, r1, r0
	str	r0, [sp, #120]                  @ 4-byte Spill
	ldr	r0, [sp, #228]                  @ 4-byte Reload
	ldr	r1, [sp, #132]                  @ 4-byte Reload
	adcs	r0, r0, r1
	str	r0, [sp, #132]                  @ 4-byte Spill
	ldr	r2, [sp, #304]
	adc	r0, r2, #0
	str	r0, [sp, #128]                  @ 4-byte Spill
	ldr	r2, [sp, #240]
	ldm	r3, {r0, r1, r3}
	adds	r2, r10, r2
	adcs	r12, r12, r0
	str	r12, [sp, #228]                 @ 4-byte Spill
	adcs	r9, r5, r1
	str	r9, [sp, #156]                  @ 4-byte Spill
	adcs	r10, r4, r3
	str	r10, [sp, #152]                 @ 4-byte Spill
	ldr	r0, [sp, #256]
	ldr	r1, [sp, #108]                  @ 4-byte Reload
	adcs	r4, r6, r0
	str	r4, [sp, #148]                  @ 4-byte Spill
	ldr	r0, [sp, #260]
	ldr	r2, [sp, #104]                  @ 4-byte Reload
	adcs	r6, lr, r0
	str	r6, [sp, #144]                  @ 4-byte Spill
	ldr	r0, [sp, #264]
	adcs	r5, r8, r0
	str	r5, [sp, #140]                  @ 4-byte Spill
	ldr	r0, [sp, #268]
	adcs	r3, r7, r0
	str	r3, [sp, #136]                  @ 4-byte Spill
	ldr	r0, [sp, #272]
	adcs	lr, r11, r0
	str	lr, [sp, #124]                  @ 4-byte Spill
	ldr	r0, [sp, #276]
	adcs	r1, r1, r0
	str	r1, [sp, #108]                  @ 4-byte Spill
	ldr	r0, [sp, #280]
	adcs	r7, r2, r0
	str	r7, [sp, #104]                  @ 4-byte Spill
	ldr	r0, [sp, #284]
	ldr	r2, [sp, #232]                  @ 4-byte Reload
	adcs	r0, r2, r0
	str	r0, [sp, #232]                  @ 4-byte Spill
	ldr	r0, [sp, #288]
	ldr	r2, [sp, #112]                  @ 4-byte Reload
	adcs	r0, r2, r0
	str	r0, [sp, #112]                  @ 4-byte Spill
	ldr	r0, [sp, #292]
	ldr	r2, [sp, #116]                  @ 4-byte Reload
	adcs	r0, r2, r0
	str	r0, [sp, #116]                  @ 4-byte Spill
	ldr	r0, [sp, #296]
	ldr	r2, [sp, #120]                  @ 4-byte Reload
	adcs	r0, r2, r0
	str	r0, [sp, #120]                  @ 4-byte Spill
	ldr	r0, [sp, #300]
	ldr	r2, [sp, #132]                  @ 4-byte Reload
	adcs	r0, r2, r0
	str	r0, [sp, #132]                  @ 4-byte Spill
	ldr	r0, [sp, #236]                  @ 4-byte Reload
	ldr	r2, [sp, #128]                  @ 4-byte Reload
	ldr	r0, [r0, #124]
	adc	r2, r0, r2
	ldr	r0, [sp, #216]                  @ 4-byte Reload
	subs	r0, r12, r0
	str	r0, [sp, #236]                  @ 4-byte Spill
	ldr	r0, [sp, #212]                  @ 4-byte Reload
	sbcs	r0, r9, r0
	str	r0, [sp, #216]                  @ 4-byte Spill
	ldr	r0, [sp, #208]                  @ 4-byte Reload
	sbcs	r0, r10, r0
	str	r0, [sp, #212]                  @ 4-byte Spill
	ldr	r0, [sp, #192]                  @ 4-byte Reload
	ldr	r10, [sp, #112]                 @ 4-byte Reload
	sbcs	r0, r4, r0
	str	r0, [sp, #208]                  @ 4-byte Spill
	ldr	r0, [sp, #196]                  @ 4-byte Reload
	sbcs	r0, r6, r0
	str	r0, [sp, #196]                  @ 4-byte Spill
	ldr	r0, [sp, #200]                  @ 4-byte Reload
	sbcs	r0, r5, r0
	str	r0, [sp, #200]                  @ 4-byte Spill
	ldr	r0, [sp, #204]                  @ 4-byte Reload
	ldr	r5, [sp, #120]                  @ 4-byte Reload
	sbcs	r0, r3, r0
	str	r0, [sp, #204]                  @ 4-byte Spill
	ldr	r0, [sp, #160]                  @ 4-byte Reload
	sbcs	r0, lr, r0
	str	r0, [sp, #192]                  @ 4-byte Spill
	ldr	r0, [sp, #164]                  @ 4-byte Reload
	ldr	lr, [sp, #132]                  @ 4-byte Reload
	sbcs	r11, r1, r0
	ldr	r0, [sp, #168]                  @ 4-byte Reload
	ldr	r1, [sp, #232]                  @ 4-byte Reload
	sbcs	r9, r7, r0
	ldr	r0, [sp, #172]                  @ 4-byte Reload
	ldr	r7, [sp, #116]                  @ 4-byte Reload
	sbcs	r8, r1, r0
	ldr	r0, [sp, #176]                  @ 4-byte Reload
	sbcs	r6, r10, r0
	ldr	r0, [sp, #180]                  @ 4-byte Reload
	sbcs	r4, r7, r0
	ldr	r0, [sp, #184]                  @ 4-byte Reload
	sbcs	r3, r5, r0
	ldr	r0, [sp, #188]                  @ 4-byte Reload
	sbcs	r1, lr, r0
	ldr	r0, [sp, #220]                  @ 4-byte Reload
	sbc	r12, r2, r0
	ldr	r0, [sp, #224]                  @ 4-byte Reload
	cmn	r12, #1
	movle	r1, lr
	movgt	r2, r12
	str	r1, [r0, #56]
	movle	r3, r5
	ldr	r1, [sp, #232]                  @ 4-byte Reload
	cmn	r12, #1
	movle	r4, r7
	movle	r6, r10
	str	r2, [r0, #60]
	movle	r8, r1
	ldr	r1, [sp, #104]                  @ 4-byte Reload
	cmn	r12, #1
	ldr	r2, [sp, #192]                  @ 4-byte Reload
	str	r3, [r0, #52]
	movle	r9, r1
	ldr	r1, [sp, #108]                  @ 4-byte Reload
	str	r4, [r0, #48]
	str	r6, [r0, #44]
	movle	r11, r1
	ldr	r1, [sp, #124]                  @ 4-byte Reload
	str	r8, [r0, #40]
	str	r9, [r0, #36]
	movle	r2, r1
	cmn	r12, #1
	str	r2, [r0, #28]
	ldr	r2, [sp, #136]                  @ 4-byte Reload
	ldr	r1, [sp, #204]                  @ 4-byte Reload
	str	r11, [r0, #32]
	movle	r1, r2
	ldr	r2, [sp, #140]                  @ 4-byte Reload
	str	r1, [r0, #24]
	ldr	r1, [sp, #200]                  @ 4-byte Reload
	movle	r1, r2
	ldr	r2, [sp, #144]                  @ 4-byte Reload
	str	r1, [r0, #20]
	ldr	r1, [sp, #196]                  @ 4-byte Reload
	movle	r1, r2
	ldr	r2, [sp, #148]                  @ 4-byte Reload
	str	r1, [r0, #16]
	cmn	r12, #1
	ldr	r1, [sp, #208]                  @ 4-byte Reload
	movle	r1, r2
	ldr	r2, [sp, #152]                  @ 4-byte Reload
	str	r1, [r0, #12]
	ldr	r1, [sp, #212]                  @ 4-byte Reload
	movle	r1, r2
	ldr	r2, [sp, #156]                  @ 4-byte Reload
	str	r1, [r0, #8]
	ldr	r1, [sp, #216]                  @ 4-byte Reload
	movle	r1, r2
	ldr	r2, [sp, #228]                  @ 4-byte Reload
	str	r1, [r0, #4]
	cmn	r12, #1
	ldr	r1, [sp, #236]                  @ 4-byte Reload
	movle	r1, r2
	str	r1, [r0]
	add	sp, sp, #372
	add	sp, sp, #1024
	pop	{r4, r5, r6, r7, r8, r9, r10, r11, lr}
	mov	pc, lr
.Lfunc_end80:
	.size	mcl_fp_montRedNF16L, .Lfunc_end80-mcl_fp_montRedNF16L
	.cantunwind
	.fnend
                                        @ -- End function
	.globl	mcl_fp_addPre16L                @ -- Begin function mcl_fp_addPre16L
	.p2align	2
	.type	mcl_fp_addPre16L,%function
	.code	32                              @ @mcl_fp_addPre16L
mcl_fp_addPre16L:
	.fnstart
@ %bb.0:
	.save	{r4, r5, r6, r7, r8, r9, r10, r11, lr}
	push	{r4, r5, r6, r7, r8, r9, r10, r11, lr}
	.pad	#60
	sub	sp, sp, #60
	ldm	r2, {r3, r12}
	add	r11, r1, #48
	ldm	r1, {r4, r7, lr}
	adds	r3, r4, r3
	str	r3, [sp, #28]                   @ 4-byte Spill
	adcs	r3, r7, r12
	str	r3, [sp, #24]                   @ 4-byte Spill
	ldr	r3, [r2, #32]
	add	r7, r2, #20
	str	r3, [sp, #20]                   @ 4-byte Spill
	add	r12, r1, #16
	ldr	r3, [r2, #36]
	str	r3, [sp, #32]                   @ 4-byte Spill
	ldr	r3, [r2, #40]
	str	r3, [sp, #36]                   @ 4-byte Spill
	ldr	r3, [r2, #44]
	str	r3, [sp, #40]                   @ 4-byte Spill
	ldr	r3, [r2, #48]
	str	r3, [sp, #44]                   @ 4-byte Spill
	ldr	r3, [r2, #52]
	str	r3, [sp, #48]                   @ 4-byte Spill
	ldr	r3, [r2, #56]
	ldr	r8, [r2, #8]
	str	r3, [sp, #52]                   @ 4-byte Spill
	ldr	r3, [r2, #60]
	str	r3, [sp, #56]                   @ 4-byte Spill
	adcs	r3, lr, r8
	ldr	r6, [r2, #12]
	ldr	lr, [r2, #16]
	ldr	r2, [r1, #36]
	ldr	r5, [r1, #12]
	str	r2, [sp]                        @ 4-byte Spill
	ldr	r2, [r1, #40]
	str	r3, [sp, #16]                   @ 4-byte Spill
	adcs	r3, r5, r6
	str	r2, [sp, #4]                    @ 4-byte Spill
	ldr	r2, [r1, #44]
	str	r3, [sp, #12]                   @ 4-byte Spill
	ldr	r5, [r1, #32]
	str	r2, [sp, #8]                    @ 4-byte Spill
	ldm	r12, {r1, r2, r3, r12}
	ldm	r7, {r4, r6, r7}
	adcs	r1, r1, lr
	add	lr, r0, #16
	adcs	r2, r2, r4
	ldm	r11, {r8, r9, r10, r11}
	adcs	r3, r3, r6
	ldr	r6, [sp, #20]                   @ 4-byte Reload
	adcs	r7, r12, r7
	adcs	r12, r5, r6
	ldr	r6, [sp, #28]                   @ 4-byte Reload
	str	r6, [r0]
	ldr	r6, [sp, #24]                   @ 4-byte Reload
	str	r6, [r0, #4]
	ldr	r6, [sp, #16]                   @ 4-byte Reload
	str	r6, [r0, #8]
	ldr	r6, [sp, #12]                   @ 4-byte Reload
	str	r6, [r0, #12]
	stm	lr, {r1, r2, r3, r7}
	ldr	r1, [sp, #32]                   @ 4-byte Reload
	ldr	r2, [sp]                        @ 4-byte Reload
	ldr	r3, [sp, #4]                    @ 4-byte Reload
	adcs	r1, r2, r1
	ldr	r2, [sp, #36]                   @ 4-byte Reload
	ldr	r7, [sp, #8]                    @ 4-byte Reload
	adcs	r2, r3, r2
	ldr	r3, [sp, #40]                   @ 4-byte Reload
	ldr	r6, [sp, #48]                   @ 4-byte Reload
	adcs	r3, r7, r3
	ldr	r7, [sp, #44]                   @ 4-byte Reload
	str	r12, [r0, #32]
	add	r12, r0, #36
	adcs	r7, r8, r7
	stm	r12, {r1, r2, r3, r7}
	adcs	r5, r9, r6
	ldr	r6, [sp, #52]                   @ 4-byte Reload
	str	r5, [r0, #52]
	adcs	r4, r10, r6
	ldr	r6, [sp, #56]                   @ 4-byte Reload
	str	r4, [r0, #56]
	adcs	r6, r11, r6
	str	r6, [r0, #60]
	mov	r0, #0
	adc	r0, r0, #0
	add	sp, sp, #60
	pop	{r4, r5, r6, r7, r8, r9, r10, r11, lr}
	mov	pc, lr
.Lfunc_end81:
	.size	mcl_fp_addPre16L, .Lfunc_end81-mcl_fp_addPre16L
	.cantunwind
	.fnend
                                        @ -- End function
	.globl	mcl_fp_subPre16L                @ -- Begin function mcl_fp_subPre16L
	.p2align	2
	.type	mcl_fp_subPre16L,%function
	.code	32                              @ @mcl_fp_subPre16L
mcl_fp_subPre16L:
	.fnstart
@ %bb.0:
	.save	{r4, r5, r6, r7, r8, r9, r10, r11, lr}
	push	{r4, r5, r6, r7, r8, r9, r10, r11, lr}
	.pad	#60
	sub	sp, sp, #60
	ldm	r2, {r3, r12}
	add	r11, r1, #48
	ldm	r1, {r4, r7, lr}
	subs	r3, r4, r3
	str	r3, [sp, #28]                   @ 4-byte Spill
	sbcs	r3, r7, r12
	str	r3, [sp, #24]                   @ 4-byte Spill
	ldr	r3, [r2, #32]
	add	r7, r2, #20
	str	r3, [sp, #20]                   @ 4-byte Spill
	add	r12, r1, #16
	ldr	r3, [r2, #36]
	str	r3, [sp, #32]                   @ 4-byte Spill
	ldr	r3, [r2, #40]
	str	r3, [sp, #36]                   @ 4-byte Spill
	ldr	r3, [r2, #44]
	str	r3, [sp, #40]                   @ 4-byte Spill
	ldr	r3, [r2, #48]
	str	r3, [sp, #44]                   @ 4-byte Spill
	ldr	r3, [r2, #52]
	str	r3, [sp, #48]                   @ 4-byte Spill
	ldr	r3, [r2, #56]
	ldr	r8, [r2, #8]
	str	r3, [sp, #52]                   @ 4-byte Spill
	ldr	r3, [r2, #60]
	str	r3, [sp, #56]                   @ 4-byte Spill
	sbcs	r3, lr, r8
	ldr	r6, [r2, #12]
	ldr	lr, [r2, #16]
	ldr	r2, [r1, #36]
	ldr	r5, [r1, #12]
	str	r2, [sp]                        @ 4-byte Spill
	ldr	r2, [r1, #40]
	str	r3, [sp, #16]                   @ 4-byte Spill
	sbcs	r3, r5, r6
	str	r2, [sp, #4]                    @ 4-byte Spill
	ldr	r2, [r1, #44]
	str	r3, [sp, #12]                   @ 4-byte Spill
	ldr	r5, [r1, #32]
	str	r2, [sp, #8]                    @ 4-byte Spill
	ldm	r12, {r1, r2, r3, r12}
	ldm	r7, {r4, r6, r7}
	sbcs	r1, r1, lr
	add	lr, r0, #16
	sbcs	r2, r2, r4
	ldm	r11, {r8, r9, r10, r11}
	sbcs	r3, r3, r6
	ldr	r6, [sp, #20]                   @ 4-byte Reload
	sbcs	r7, r12, r7
	sbcs	r12, r5, r6
	ldr	r6, [sp, #28]                   @ 4-byte Reload
	str	r6, [r0]
	ldr	r6, [sp, #24]                   @ 4-byte Reload
	str	r6, [r0, #4]
	ldr	r6, [sp, #16]                   @ 4-byte Reload
	str	r6, [r0, #8]
	ldr	r6, [sp, #12]                   @ 4-byte Reload
	str	r6, [r0, #12]
	stm	lr, {r1, r2, r3, r7}
	ldr	r1, [sp, #32]                   @ 4-byte Reload
	ldr	r2, [sp]                        @ 4-byte Reload
	ldr	r3, [sp, #4]                    @ 4-byte Reload
	sbcs	r1, r2, r1
	ldr	r2, [sp, #36]                   @ 4-byte Reload
	ldr	r7, [sp, #8]                    @ 4-byte Reload
	sbcs	r2, r3, r2
	ldr	r3, [sp, #40]                   @ 4-byte Reload
	ldr	r6, [sp, #48]                   @ 4-byte Reload
	sbcs	r3, r7, r3
	ldr	r7, [sp, #44]                   @ 4-byte Reload
	str	r12, [r0, #32]
	add	r12, r0, #36
	sbcs	r7, r8, r7
	stm	r12, {r1, r2, r3, r7}
	sbcs	r5, r9, r6
	ldr	r6, [sp, #52]                   @ 4-byte Reload
	str	r5, [r0, #52]
	sbcs	r4, r10, r6
	ldr	r6, [sp, #56]                   @ 4-byte Reload
	str	r4, [r0, #56]
	sbcs	r6, r11, r6
	str	r6, [r0, #60]
	mov	r0, #0
	sbc	r0, r0, #0
	and	r0, r0, #1
	add	sp, sp, #60
	pop	{r4, r5, r6, r7, r8, r9, r10, r11, lr}
	mov	pc, lr
.Lfunc_end82:
	.size	mcl_fp_subPre16L, .Lfunc_end82-mcl_fp_subPre16L
	.cantunwind
	.fnend
                                        @ -- End function
	.globl	mcl_fp_shr1_16L                 @ -- Begin function mcl_fp_shr1_16L
	.p2align	2
	.type	mcl_fp_shr1_16L,%function
	.code	32                              @ @mcl_fp_shr1_16L
mcl_fp_shr1_16L:
	.fnstart
@ %bb.0:
	.save	{r4, r5, r6, r7, r8, r9, r10, r11, lr}
	push	{r4, r5, r6, r7, r8, r9, r10, r11, lr}
	.pad	#24
	sub	sp, sp, #24
	ldr	r2, [r1, #52]
	ldr	r3, [r1, #56]
	str	r2, [sp, #16]                   @ 4-byte Spill
	lsr	r2, r2, #1
	str	r3, [sp, #8]                    @ 4-byte Spill
	orr	r2, r2, r3, lsl #31
	str	r2, [r0, #52]
	ldr	r2, [r1, #44]
	ldr	r3, [r1, #48]
	ldr	r11, [r1, #36]
	str	r2, [sp, #12]                   @ 4-byte Spill
	lsr	r2, r2, #1
	ldr	r10, [r1, #40]
	orr	r2, r2, r3, lsl #31
	str	r2, [r0, #44]
	lsr	r2, r11, #1
	ldr	r12, [r1, #28]
	ldr	r5, [r1, #32]
	orr	r2, r2, r10, lsl #31
	ldr	r4, [r1, #20]
	str	r2, [r0, #36]
	lsr	r9, r12, #1
	ldr	r2, [r1]
	orr	r7, r9, r5, lsl #31
	ldr	lr, [r1, #24]
	str	r2, [sp, #20]                   @ 4-byte Spill
	ldr	r2, [r1, #12]
	str	r7, [r0, #28]
	lsr	r7, r4, #1
	ldr	r6, [r1, #16]
	orr	r7, r7, lr, lsl #31
	ldr	r8, [r1, #4]
	str	r3, [sp, #4]                    @ 4-byte Spill
	ldr	r3, [r1, #8]
	str	r7, [r0, #20]
	lsr	r7, r2, #1
	orr	r7, r7, r6, lsl #31
	ldr	r1, [r1, #60]
	str	r7, [r0, #12]
	lsr	r7, r8, #1
	orr	r7, r7, r3, lsl #31
	str	r7, [r0, #4]
	lsr	r7, r1, #1
	lsrs	r1, r1, #1
	ldr	r1, [sp, #8]                    @ 4-byte Reload
	str	r2, [sp]                        @ 4-byte Spill
	str	r7, [r0, #60]
	rrx	r1, r1
	str	r1, [r0, #56]
	ldr	r1, [sp, #16]                   @ 4-byte Reload
	lsrs	r1, r1, #1
	ldr	r1, [sp, #4]                    @ 4-byte Reload
	rrx	r1, r1
	str	r1, [r0, #48]
	ldr	r1, [sp, #12]                   @ 4-byte Reload
	lsrs	r1, r1, #1
	rrx	r1, r10
	str	r1, [r0, #40]
	lsrs	r1, r11, #1
	rrx	r1, r5
	str	r1, [r0, #32]
	lsrs	r1, r12, #1
	rrx	r1, lr
	str	r1, [r0, #24]
	lsrs	r1, r4, #1
	rrx	r1, r6
	str	r1, [r0, #16]
	ldr	r1, [sp]                        @ 4-byte Reload
	lsrs	r1, r1, #1
	rrx	r1, r3
	str	r1, [r0, #8]
	lsrs	r1, r8, #1
	ldr	r1, [sp, #20]                   @ 4-byte Reload
	rrx	r1, r1
	str	r1, [r0]
	add	sp, sp, #24
	pop	{r4, r5, r6, r7, r8, r9, r10, r11, lr}
	mov	pc, lr
.Lfunc_end83:
	.size	mcl_fp_shr1_16L, .Lfunc_end83-mcl_fp_shr1_16L
	.cantunwind
	.fnend
                                        @ -- End function
	.globl	mcl_fp_add16L                   @ -- Begin function mcl_fp_add16L
	.p2align	2
	.type	mcl_fp_add16L,%function
	.code	32                              @ @mcl_fp_add16L
mcl_fp_add16L:
	.fnstart
@ %bb.0:
	.save	{r4, r5, r6, r7, r8, r9, r10, r11, lr}
	push	{r4, r5, r6, r7, r8, r9, r10, r11, lr}
	.pad	#68
	sub	sp, sp, #68
	ldm	r2, {r4, lr}
	ldm	r1, {r5, r6, r7, r9}
	adds	r4, r5, r4
	ldr	r12, [r2, #8]
	adcs	r6, r6, lr
	ldr	r8, [r2, #12]
	adcs	r5, r7, r12
	str	r4, [sp, #56]                   @ 4-byte Spill
	ldr	r7, [r2, #16]
	adcs	lr, r9, r8
	ldr	r4, [r1, #16]
	str	r6, [sp, #52]                   @ 4-byte Spill
	adcs	r8, r4, r7
	ldr	r6, [r2, #20]
	ldr	r4, [r1, #20]
	ldr	r12, [r3, #12]
	adcs	r9, r4, r6
	ldr	r4, [r2, #24]
	ldr	r6, [r1, #24]
	adcs	r7, r6, r4
	str	r7, [sp, #48]                   @ 4-byte Spill
	ldr	r6, [r2, #28]
	ldr	r7, [r1, #28]
	ldr	r4, [r1, #56]
	adcs	r7, r7, r6
	str	r7, [sp, #44]                   @ 4-byte Spill
	ldr	r6, [r2, #32]
	ldr	r7, [r1, #32]
	adcs	r7, r7, r6
	str	r7, [sp, #40]                   @ 4-byte Spill
	ldr	r6, [r2, #36]
	ldr	r7, [r1, #36]
	adcs	r7, r7, r6
	str	r7, [sp, #36]                   @ 4-byte Spill
	ldr	r6, [r2, #40]
	ldr	r7, [r1, #40]
	adcs	r7, r7, r6
	str	r7, [sp, #32]                   @ 4-byte Spill
	ldr	r6, [r2, #44]
	ldr	r7, [r1, #44]
	adcs	r7, r7, r6
	str	r7, [sp, #64]                   @ 4-byte Spill
	ldr	r6, [r2, #48]
	ldr	r7, [r1, #48]
	adcs	r7, r7, r6
	str	r7, [sp, #28]                   @ 4-byte Spill
	ldr	r7, [r1, #52]
	ldr	r6, [r2, #52]
	ldr	r1, [r1, #60]
	adcs	r11, r7, r6
	ldr	r6, [r2, #56]
	ldr	r2, [r2, #60]
	adcs	r7, r4, r6
	ldr	r6, [sp, #56]                   @ 4-byte Reload
	adcs	r10, r1, r2
	mov	r2, #0
	adc	r1, r2, #0
	ldm	r3, {r2, r4}
	subs	r2, r6, r2
	str	r2, [sp, #12]                   @ 4-byte Spill
	ldr	r2, [sp, #52]                   @ 4-byte Reload
	str	r1, [sp, #60]                   @ 4-byte Spill
	ldr	r1, [r3, #8]
	sbcs	r4, r2, r4
	str	r6, [r0]
	mov	r6, r8
	sbcs	r1, r5, r1
	str	r1, [sp, #4]                    @ 4-byte Spill
	sbcs	r1, lr, r12
	str	r1, [sp]                        @ 4-byte Spill
	ldr	r1, [sp, #64]                   @ 4-byte Reload
	stmib	r0, {r2, r5, lr}
	str	r1, [r0, #44]
	ldr	r1, [r3, #16]
	str	r4, [sp, #8]                    @ 4-byte Spill
	mov	r4, r9
	sbcs	r1, r6, r1
	str	r1, [sp, #56]                   @ 4-byte Spill
	ldr	r1, [r3, #20]
	str	r7, [sp, #24]                   @ 4-byte Spill
	sbcs	r1, r4, r1
	ldr	r7, [sp, #48]                   @ 4-byte Reload
	str	r1, [sp, #52]                   @ 4-byte Spill
	ldr	r1, [r3, #24]
	ldr	lr, [sp, #44]                   @ 4-byte Reload
	sbcs	r1, r7, r1
	str	r1, [sp, #48]                   @ 4-byte Spill
	ldr	r1, [r3, #28]
	ldr	r5, [sp, #40]                   @ 4-byte Reload
	sbcs	r1, lr, r1
	str	lr, [r0, #28]
	str	r1, [sp, #44]                   @ 4-byte Spill
	add	lr, r3, #40
	ldr	r1, [r3, #32]
	str	r8, [r0, #16]
	ldr	r8, [sp, #36]                   @ 4-byte Reload
	sbcs	r1, r5, r1
	ldr	r6, [r3, #36]
	str	r9, [r0, #20]
	str	r7, [r0, #24]
	ldr	r9, [sp, #32]                   @ 4-byte Reload
	ldm	lr, {r4, r7, lr}
	str	r8, [r0, #36]
	sbcs	r8, r8, r6
	str	r1, [sp, #40]                   @ 4-byte Spill
	ldr	r1, [sp, #64]                   @ 4-byte Reload
	str	r5, [r0, #32]
	sbcs	r5, r9, r4
	ldr	r2, [sp, #28]                   @ 4-byte Reload
	sbcs	r4, r1, r7
	str	r11, [sp, #20]                  @ 4-byte Spill
	str	r11, [r0, #52]
	sbcs	r6, r2, lr
	ldr	r11, [r3, #52]
	ldr	r1, [sp, #20]                   @ 4-byte Reload
	ldr	r12, [sp, #24]                  @ 4-byte Reload
	str	r10, [sp, #16]                  @ 4-byte Spill
	str	r10, [r0, #60]
	ldr	r10, [r3, #56]
	str	r2, [r0, #48]
	sbcs	r2, r1, r11
	ldr	r3, [r3, #60]
	sbcs	lr, r12, r10
	ldr	r1, [sp, #16]                   @ 4-byte Reload
	str	r9, [r0, #40]
	sbcs	r1, r1, r3
	ldr	r3, [sp, #60]                   @ 4-byte Reload
	str	r12, [r0, #56]
	sbc	r3, r3, #0
	tst	r3, #1
	bne	.LBB84_2
@ %bb.1:                                @ %nocarry
	ldr	r3, [sp, #12]                   @ 4-byte Reload
	str	r3, [r0]
	ldr	r3, [sp, #8]                    @ 4-byte Reload
	str	r3, [r0, #4]
	ldr	r3, [sp, #4]                    @ 4-byte Reload
	str	r3, [r0, #8]
	ldr	r3, [sp]                        @ 4-byte Reload
	str	r3, [r0, #12]
	ldr	r3, [sp, #56]                   @ 4-byte Reload
	str	r3, [r0, #16]
	ldr	r3, [sp, #52]                   @ 4-byte Reload
	str	r3, [r0, #20]
	ldr	r3, [sp, #48]                   @ 4-byte Reload
	str	r3, [r0, #24]
	ldr	r3, [sp, #44]                   @ 4-byte Reload
	str	r3, [r0, #28]
	ldr	r3, [sp, #40]                   @ 4-byte Reload
	str	r3, [r0, #32]
	str	r8, [r0, #36]
	str	r5, [r0, #40]
	str	r4, [r0, #44]
	str	r6, [r0, #48]
	str	r2, [r0, #52]
	str	lr, [r0, #56]
	str	r1, [r0, #60]
.LBB84_2:                               @ %carry
	add	sp, sp, #68
	pop	{r4, r5, r6, r7, r8, r9, r10, r11, lr}
	mov	pc, lr
.Lfunc_end84:
	.size	mcl_fp_add16L, .Lfunc_end84-mcl_fp_add16L
	.cantunwind
	.fnend
                                        @ -- End function
	.globl	mcl_fp_addNF16L                 @ -- Begin function mcl_fp_addNF16L
	.p2align	2
	.type	mcl_fp_addNF16L,%function
	.code	32                              @ @mcl_fp_addNF16L
mcl_fp_addNF16L:
	.fnstart
@ %bb.0:
	.save	{r4, r5, r6, r7, r8, r9, r10, r11, lr}
	push	{r4, r5, r6, r7, r8, r9, r10, r11, lr}
	.pad	#96
	sub	sp, sp, #96
	ldr	r9, [r1]
	add	r11, r3, #32
	ldm	r2, {r4, r5, r6, r7}
	ldmib	r1, {r8, lr}
	adds	r9, r4, r9
	adcs	r8, r5, r8
	ldr	r12, [r1, #12]
	adcs	lr, r6, lr
	ldr	r6, [r1, #16]
	adcs	r10, r7, r12
	ldr	r7, [r2, #16]
	ldr	r4, [r1, #20]
	adcs	r5, r7, r6
	ldr	r7, [r2, #20]
	ldr	r6, [r2, #24]
	adcs	r4, r7, r4
	ldr	r7, [r1, #24]
	str	r4, [sp, #88]                   @ 4-byte Spill
	adcs	r4, r6, r7
	ldr	r7, [r1, #28]
	ldr	r6, [r2, #28]
	str	r4, [sp, #84]                   @ 4-byte Spill
	adcs	r4, r6, r7
	ldr	r7, [r1, #32]
	ldr	r6, [r2, #32]
	str	r4, [sp, #80]                   @ 4-byte Spill
	adcs	r4, r6, r7
	ldr	r7, [r1, #36]
	ldr	r6, [r2, #36]
	str	r4, [sp, #76]                   @ 4-byte Spill
	adcs	r4, r6, r7
	ldr	r7, [r1, #40]
	ldr	r6, [r2, #40]
	str	r5, [sp, #92]                   @ 4-byte Spill
	adcs	r7, r6, r7
	str	r7, [sp, #68]                   @ 4-byte Spill
	ldr	r7, [r1, #44]
	ldr	r6, [r2, #44]
	ldr	r5, [r3, #8]
	adcs	r7, r6, r7
	str	r7, [sp, #64]                   @ 4-byte Spill
	ldr	r7, [r1, #48]
	ldr	r6, [r2, #48]
	str	r4, [sp, #72]                   @ 4-byte Spill
	adcs	r7, r6, r7
	str	r7, [sp, #16]                   @ 4-byte Spill
	ldr	r7, [r1, #52]
	ldr	r6, [r2, #52]
	str	lr, [sp, #44]                   @ 4-byte Spill
	adcs	r7, r6, r7
	str	r7, [sp, #8]                    @ 4-byte Spill
	ldr	r7, [r1, #56]
	ldr	r6, [r2, #56]
	ldr	r1, [r1, #60]
	ldr	r2, [r2, #60]
	adcs	r7, r6, r7
	str	r7, [sp, #4]                    @ 4-byte Spill
	adc	r12, r2, r1
	ldm	r3, {r2, r7}
	subs	r2, r9, r2
	str	r2, [sp, #56]                   @ 4-byte Spill
	sbcs	r2, r8, r7
	str	r2, [sp, #48]                   @ 4-byte Spill
	sbcs	r2, lr, r5
	ldr	r4, [r3, #12]
	add	lr, r3, #16
	ldr	r1, [r3, #60]
	str	r2, [sp, #40]                   @ 4-byte Spill
	sbcs	r2, r10, r4
	str	r1, [sp]                        @ 4-byte Spill
	ldr	r1, [r3, #28]
	ldm	lr, {r3, r4, lr}
	ldr	r5, [sp, #92]                   @ 4-byte Reload
	str	r9, [sp, #60]                   @ 4-byte Spill
	sbcs	r3, r5, r3
	str	r3, [sp, #28]                   @ 4-byte Spill
	ldr	r3, [sp, #88]                   @ 4-byte Reload
	str	r8, [sp, #52]                   @ 4-byte Spill
	sbcs	r3, r3, r4
	str	r3, [sp, #24]                   @ 4-byte Spill
	ldr	r3, [sp, #84]                   @ 4-byte Reload
	str	r10, [sp, #36]                  @ 4-byte Spill
	sbcs	r3, r3, lr
	str	r3, [sp, #20]                   @ 4-byte Spill
	ldr	r3, [sp, #80]                   @ 4-byte Reload
	str	r2, [sp, #32]                   @ 4-byte Spill
	sbcs	r1, r3, r1
	ldm	r11, {r2, r6, r7, r8, r9, r10, r11}
	ldr	r3, [sp, #76]                   @ 4-byte Reload
	str	r1, [sp, #12]                   @ 4-byte Spill
	sbcs	r4, r3, r2
	ldr	r3, [sp, #72]                   @ 4-byte Reload
	ldr	r1, [sp, #68]                   @ 4-byte Reload
	sbcs	r6, r3, r6
	ldr	lr, [sp, #16]                   @ 4-byte Reload
	sbcs	r7, r1, r7
	ldr	r1, [sp, #64]                   @ 4-byte Reload
	ldr	r2, [sp, #8]                    @ 4-byte Reload
	sbcs	r8, r1, r8
	ldr	r1, [sp, #4]                    @ 4-byte Reload
	sbcs	r9, lr, r9
	ldr	r5, [sp]                        @ 4-byte Reload
	sbcs	r3, r2, r10
	sbcs	r11, r1, r11
	sbc	r10, r12, r5
	cmn	r10, #1
	movle	r11, r1
	ldr	r1, [sp, #64]                   @ 4-byte Reload
	movle	r3, r2
	movgt	r12, r10
	cmn	r10, #1
	ldr	r2, [sp, #12]                   @ 4-byte Reload
	movle	r8, r1
	ldr	r1, [sp, #68]                   @ 4-byte Reload
	movle	r9, lr
	str	r12, [r0, #60]
	str	r11, [r0, #56]
	movle	r7, r1
	ldr	r1, [sp, #72]                   @ 4-byte Reload
	cmn	r10, #1
	str	r3, [r0, #52]
	str	r9, [r0, #48]
	movle	r6, r1
	ldr	r1, [sp, #76]                   @ 4-byte Reload
	str	r8, [r0, #44]
	str	r7, [r0, #40]
	movle	r4, r1
	ldr	r1, [sp, #80]                   @ 4-byte Reload
	str	r6, [r0, #36]
	str	r4, [r0, #32]
	movle	r2, r1
	ldr	r1, [sp, #84]                   @ 4-byte Reload
	str	r2, [r0, #28]
	cmn	r10, #1
	ldr	r2, [sp, #20]                   @ 4-byte Reload
	movle	r2, r1
	ldr	r1, [sp, #88]                   @ 4-byte Reload
	str	r2, [r0, #24]
	ldr	r2, [sp, #24]                   @ 4-byte Reload
	movle	r2, r1
	ldr	r1, [sp, #92]                   @ 4-byte Reload
	str	r2, [r0, #20]
	ldr	r2, [sp, #28]                   @ 4-byte Reload
	movle	r2, r1
	cmn	r10, #1
	str	r2, [r0, #16]
	ldr	r2, [sp, #36]                   @ 4-byte Reload
	ldr	r1, [sp, #32]                   @ 4-byte Reload
	movle	r1, r2
	ldr	r2, [sp, #44]                   @ 4-byte Reload
	str	r1, [r0, #12]
	ldr	r1, [sp, #40]                   @ 4-byte Reload
	movle	r1, r2
	ldr	r2, [sp, #52]                   @ 4-byte Reload
	str	r1, [r0, #8]
	ldr	r1, [sp, #48]                   @ 4-byte Reload
	movle	r1, r2
	ldr	r2, [sp, #60]                   @ 4-byte Reload
	str	r1, [r0, #4]
	cmn	r10, #1
	ldr	r1, [sp, #56]                   @ 4-byte Reload
	movle	r1, r2
	str	r1, [r0]
	add	sp, sp, #96
	pop	{r4, r5, r6, r7, r8, r9, r10, r11, lr}
	mov	pc, lr
.Lfunc_end85:
	.size	mcl_fp_addNF16L, .Lfunc_end85-mcl_fp_addNF16L
	.cantunwind
	.fnend
                                        @ -- End function
	.globl	mcl_fp_sub16L                   @ -- Begin function mcl_fp_sub16L
	.p2align	2
	.type	mcl_fp_sub16L,%function
	.code	32                              @ @mcl_fp_sub16L
mcl_fp_sub16L:
	.fnstart
@ %bb.0:
	.save	{r4, r5, r6, r7, r8, r9, r10, r11, lr}
	push	{r4, r5, r6, r7, r8, r9, r10, r11, lr}
	.pad	#60
	sub	sp, sp, #60
	ldm	r2, {r9, lr}
	ldr	r6, [r1]
	ldmib	r1, {r4, r5, r7}
	subs	r6, r6, r9
	ldr	r8, [r2, #8]
	sbcs	r9, r4, lr
	ldr	r12, [r2, #12]
	str	r6, [sp, #56]                   @ 4-byte Spill
	sbcs	r6, r5, r8
	sbcs	lr, r7, r12
	ldr	r5, [r2, #16]
	ldr	r7, [r1, #16]
	ldr	r4, [r2, #20]
	sbcs	r12, r7, r5
	ldr	r5, [r1, #20]
	ldr	r7, [r1, #44]
	sbcs	r10, r5, r4
	ldr	r4, [r2, #24]
	ldr	r5, [r1, #24]
	str	r9, [sp, #20]                   @ 4-byte Spill
	sbcs	r4, r5, r4
	str	r4, [sp, #52]                   @ 4-byte Spill
	ldr	r4, [r2, #28]
	ldr	r5, [r1, #28]
	str	r9, [r0, #4]
	sbcs	r4, r5, r4
	str	r4, [sp, #48]                   @ 4-byte Spill
	ldr	r4, [r2, #32]
	ldr	r5, [r1, #32]
	str	r6, [sp, #16]                   @ 4-byte Spill
	sbcs	r4, r5, r4
	str	r4, [sp, #44]                   @ 4-byte Spill
	ldr	r4, [r2, #36]
	ldr	r5, [r1, #36]
	str	r6, [r0, #8]
	sbcs	r4, r5, r4
	str	r4, [sp, #40]                   @ 4-byte Spill
	ldr	r4, [r2, #40]
	ldr	r5, [r1, #40]
	str	lr, [sp, #12]                   @ 4-byte Spill
	sbcs	r4, r5, r4
	str	r4, [sp, #36]                   @ 4-byte Spill
	ldr	r4, [r2, #44]
	ldr	r5, [r1, #52]
	sbcs	r8, r7, r4
	ldr	r7, [r2, #48]
	ldr	r4, [r1, #48]
	str	lr, [r0, #12]
	sbcs	r7, r4, r7
	ldr	r4, [r2, #52]
	ldr	lr, [sp, #40]                   @ 4-byte Reload
	sbcs	r11, r5, r4
	ldr	r4, [r2, #56]
	ldr	r5, [r1, #56]
	mov	r9, r7
	ldr	r2, [r2, #60]
	add	r7, sp, #44
	ldr	r1, [r1, #60]
	sbcs	r4, r5, r4
	ldm	r7, {r5, r6, r7}                @ 12-byte Folded Reload
	sbcs	r1, r1, r2
	ldr	r2, [sp, #56]                   @ 4-byte Reload
	str	r2, [r0]
	add	r2, r0, #44
	str	r10, [sp, #32]                  @ 4-byte Spill
	str	r10, [r0, #20]
	ldr	r10, [sp, #36]                  @ 4-byte Reload
	str	r12, [r0, #16]
	str	r7, [r0, #24]
	str	r6, [r0, #28]
	str	r5, [r0, #32]
	str	lr, [r0, #36]
	str	r10, [r0, #40]
	stm	r2, {r8, r9, r11}
	mov	r2, #0
	sbc	r2, r2, #0
	tst	r2, #1
	str	r12, [sp, #24]                  @ 4-byte Spill
	str	r4, [sp, #28]                   @ 4-byte Spill
	str	r4, [r0, #56]
	str	r1, [r0, #60]
	beq	.LBB86_2
@ %bb.1:                                @ %carry
	ldr	r2, [r3]
	ldr	r12, [sp, #56]                  @ 4-byte Reload
	str	r1, [sp, #8]                    @ 4-byte Spill
	adds	r2, r2, r12
	ldmib	r3, {r1, r4}
	str	r2, [r0]
	ldr	r2, [sp, #20]                   @ 4-byte Reload
	str	r4, [sp]                        @ 4-byte Spill
	adcs	r2, r1, r2
	str	r2, [r0, #4]
	ldr	r1, [sp, #16]                   @ 4-byte Reload
	ldr	r2, [sp]                        @ 4-byte Reload
	ldr	r4, [r3, #12]
	adcs	r2, r2, r1
	str	r4, [sp, #4]                    @ 4-byte Spill
	str	r2, [r0, #8]
	ldr	r1, [sp, #12]                   @ 4-byte Reload
	ldr	r2, [sp, #4]                    @ 4-byte Reload
	adcs	r1, r2, r1
	str	r1, [r0, #12]
	ldr	r1, [r3, #16]
	ldr	r2, [sp, #24]                   @ 4-byte Reload
	adcs	r1, r1, r2
	str	r1, [r0, #16]
	ldr	r1, [r3, #20]
	ldr	r2, [sp, #32]                   @ 4-byte Reload
	adcs	r1, r1, r2
	str	r1, [r0, #20]
	ldr	r1, [r3, #24]
	ldr	r2, [sp, #28]                   @ 4-byte Reload
	adcs	r1, r1, r7
	str	r1, [r0, #24]
	ldr	r1, [r3, #28]
	adcs	r1, r1, r6
	str	r1, [r0, #28]
	ldr	r1, [r3, #32]
	adcs	r1, r1, r5
	str	r1, [r0, #32]
	ldr	r1, [r3, #36]
	adcs	r1, r1, lr
	str	r1, [r0, #36]
	ldr	r1, [r3, #40]
	adcs	r1, r1, r10
	str	r1, [r0, #40]
	ldr	r1, [r3, #44]
	adcs	r1, r1, r8
	str	r1, [r0, #44]
	ldr	r1, [r3, #48]
	adcs	r1, r1, r9
	str	r1, [r0, #48]
	ldr	r1, [r3, #52]
	adcs	r1, r1, r11
	str	r1, [r0, #52]
	ldr	r1, [r3, #56]
	adcs	r1, r1, r2
	str	r1, [r0, #56]
	ldr	r1, [r3, #60]
	ldr	r2, [sp, #8]                    @ 4-byte Reload
	adc	r1, r1, r2
	str	r1, [r0, #60]
.LBB86_2:                               @ %nocarry
	add	sp, sp, #60
	pop	{r4, r5, r6, r7, r8, r9, r10, r11, lr}
	mov	pc, lr
.Lfunc_end86:
	.size	mcl_fp_sub16L, .Lfunc_end86-mcl_fp_sub16L
	.cantunwind
	.fnend
                                        @ -- End function
	.globl	mcl_fp_subNF16L                 @ -- Begin function mcl_fp_subNF16L
	.p2align	2
	.type	mcl_fp_subNF16L,%function
	.code	32                              @ @mcl_fp_subNF16L
mcl_fp_subNF16L:
	.fnstart
@ %bb.0:
	.save	{r4, r5, r6, r7, r8, r9, r10, r11, lr}
	push	{r4, r5, r6, r7, r8, r9, r10, r11, lr}
	.pad	#84
	sub	sp, sp, #84
	ldr	r7, [r2, #32]
	str	r7, [sp, #56]                   @ 4-byte Spill
	ldr	r7, [r2, #36]
	str	r7, [sp, #60]                   @ 4-byte Spill
	ldr	r7, [r2, #40]
	str	r7, [sp, #64]                   @ 4-byte Spill
	ldr	r7, [r2, #44]
	str	r7, [sp, #68]                   @ 4-byte Spill
	ldr	r7, [r2, #48]
	str	r7, [sp, #72]                   @ 4-byte Spill
	ldr	r7, [r2, #52]
	str	r7, [sp, #80]                   @ 4-byte Spill
	ldr	r7, [r2, #56]
	str	r7, [sp, #76]                   @ 4-byte Spill
	ldr	r7, [r2, #60]
	str	r7, [sp, #32]                   @ 4-byte Spill
	ldr	r7, [r1, #60]
	str	r7, [sp, #28]                   @ 4-byte Spill
	ldr	r7, [r1, #56]
	str	r7, [sp, #24]                   @ 4-byte Spill
	ldr	r7, [r1, #52]
	str	r7, [sp, #20]                   @ 4-byte Spill
	ldr	r7, [r1, #48]
	str	r7, [sp, #16]                   @ 4-byte Spill
	ldr	r4, [r2, #24]
	ldm	r2, {r5, r6, r7, r8, r9, r11}
	ldr	r2, [r2, #28]
	str	r2, [sp, #48]                   @ 4-byte Spill
	ldm	r1, {r2, r12, lr}
	subs	r2, r2, r5
	str	r4, [sp, #12]                   @ 4-byte Spill
	sbcs	r10, r12, r6
	ldr	r4, [r1, #12]
	sbcs	lr, lr, r7
	str	r2, [sp, #44]                   @ 4-byte Spill
	sbcs	r5, r4, r8
	str	r5, [sp, #52]                   @ 4-byte Spill
	ldr	r5, [r1, #16]
	ldr	r4, [sp, #12]                   @ 4-byte Reload
	sbcs	r5, r5, r9
	str	r5, [sp, #40]                   @ 4-byte Spill
	ldr	r5, [r1, #20]
	ldr	r2, [r1, #44]
	sbcs	r5, r5, r11
	str	r5, [sp, #36]                   @ 4-byte Spill
	ldr	r5, [r1, #24]
	ldr	r6, [r1, #40]
	sbcs	r5, r5, r4
	str	r5, [sp]                        @ 4-byte Spill
	ldr	r7, [r1, #36]
	ldr	r5, [r1, #32]
	ldr	r1, [r1, #28]
	ldr	r4, [sp, #48]                   @ 4-byte Reload
	ldr	r9, [r3]
	sbcs	r1, r1, r4
	str	r1, [sp, #48]                   @ 4-byte Spill
	ldr	r1, [sp, #56]                   @ 4-byte Reload
	ldr	r4, [r3, #16]
	sbcs	r1, r5, r1
	str	r1, [sp, #56]                   @ 4-byte Spill
	ldr	r1, [sp, #60]                   @ 4-byte Reload
	ldr	r5, [r3, #12]
	sbcs	r1, r7, r1
	str	r1, [sp, #60]                   @ 4-byte Spill
	ldr	r1, [sp, #64]                   @ 4-byte Reload
	ldr	r7, [r3, #4]
	sbcs	r1, r6, r1
	str	r1, [sp, #64]                   @ 4-byte Spill
	ldr	r1, [sp, #68]                   @ 4-byte Reload
	ldr	r6, [r3, #8]
	sbcs	r1, r2, r1
	str	r1, [sp, #68]                   @ 4-byte Spill
	ldr	r1, [sp, #72]                   @ 4-byte Reload
	ldr	r2, [sp, #16]                   @ 4-byte Reload
	str	r10, [sp, #8]                   @ 4-byte Spill
	sbcs	r1, r2, r1
	str	r1, [sp, #72]                   @ 4-byte Spill
	ldr	r1, [sp, #80]                   @ 4-byte Reload
	ldr	r2, [sp, #20]                   @ 4-byte Reload
	ldr	r12, [r3, #20]
	sbcs	r1, r2, r1
	str	r1, [sp, #80]                   @ 4-byte Spill
	ldr	r1, [sp, #76]                   @ 4-byte Reload
	ldr	r2, [sp, #24]                   @ 4-byte Reload
	str	lr, [sp, #4]                    @ 4-byte Spill
	sbcs	r1, r2, r1
	str	r1, [sp, #76]                   @ 4-byte Spill
	ldr	r1, [sp, #32]                   @ 4-byte Reload
	ldr	r2, [sp, #28]                   @ 4-byte Reload
	ldr	r11, [r3, #28]
	sbc	r8, r2, r1
	ldr	r2, [sp, #44]                   @ 4-byte Reload
	ldr	r1, [r3, #36]
	adds	r9, r2, r9
	ldr	r2, [sp, #52]                   @ 4-byte Reload
	adcs	r7, r10, r7
	str	r1, [sp, #32]                   @ 4-byte Spill
	adcs	r6, lr, r6
	ldr	r1, [r3, #32]
	adcs	r5, r2, r5
	ldr	r2, [sp, #40]                   @ 4-byte Reload
	str	r1, [sp, #28]                   @ 4-byte Spill
	adcs	r10, r2, r4
	ldr	r2, [sp, #36]                   @ 4-byte Reload
	ldr	r1, [r3, #24]
	adcs	lr, r2, r12
	ldr	r12, [sp]                       @ 4-byte Reload
	ldr	r2, [sp, #48]                   @ 4-byte Reload
	adcs	r1, r12, r1
	ldr	r4, [sp, #28]                   @ 4-byte Reload
	adcs	r11, r2, r11
	ldr	r2, [sp, #56]                   @ 4-byte Reload
	adcs	r2, r2, r4
	str	r2, [sp, #16]                   @ 4-byte Spill
	ldr	r2, [sp, #60]                   @ 4-byte Reload
	ldr	r4, [sp, #32]                   @ 4-byte Reload
	adcs	r2, r2, r4
	str	r2, [sp, #20]                   @ 4-byte Spill
	ldr	r2, [r3, #40]
	ldr	r4, [sp, #64]                   @ 4-byte Reload
	adcs	r2, r4, r2
	str	r2, [sp, #32]                   @ 4-byte Spill
	ldr	r2, [r3, #44]
	ldr	r4, [sp, #68]                   @ 4-byte Reload
	adcs	r2, r4, r2
	str	r2, [sp, #28]                   @ 4-byte Spill
	ldr	r2, [r3, #48]
	ldr	r4, [sp, #72]                   @ 4-byte Reload
	adcs	r2, r4, r2
	str	r2, [sp, #24]                   @ 4-byte Spill
	ldr	r2, [r3, #52]
	ldr	r4, [sp, #80]                   @ 4-byte Reload
	adcs	r2, r4, r2
	str	r2, [sp, #12]                   @ 4-byte Spill
	ldr	r2, [r3, #56]
	ldr	r4, [sp, #76]                   @ 4-byte Reload
	ldr	r3, [r3, #60]
	adcs	r2, r4, r2
	ldr	r4, [sp, #44]                   @ 4-byte Reload
	adc	r3, r8, r3
	cmp	r8, #0
	movpl	r1, r12
	add	r12, r0, #12
	movpl	r9, r4
	ldr	r4, [sp, #36]                   @ 4-byte Reload
	str	r9, [r0]
	movpl	lr, r4
	ldr	r4, [sp, #40]                   @ 4-byte Reload
	movpl	r10, r4
	ldr	r4, [sp, #52]                   @ 4-byte Reload
	movpl	r5, r4
	ldr	r4, [sp, #4]                    @ 4-byte Reload
	cmp	r8, #0
	movpl	r6, r4
	ldr	r4, [sp, #8]                    @ 4-byte Reload
	str	r6, [r0, #8]
	ldr	r6, [sp, #28]                   @ 4-byte Reload
	movpl	r7, r4
	cmp	r8, #0
	str	r7, [r0, #4]
	ldr	r7, [sp, #56]                   @ 4-byte Reload
	stm	r12, {r5, r10, lr}
	ldr	r12, [sp, #16]                  @ 4-byte Reload
	movpl	r12, r7
	ldr	r7, [sp, #60]                   @ 4-byte Reload
	ldr	lr, [sp, #20]                   @ 4-byte Reload
	ldr	r4, [sp, #48]                   @ 4-byte Reload
	movpl	lr, r7
	ldr	r7, [sp, #64]                   @ 4-byte Reload
	ldr	r5, [sp, #24]                   @ 4-byte Reload
	movpl	r11, r4
	cmp	r8, #0
	ldr	r4, [sp, #32]                   @ 4-byte Reload
	movpl	r4, r7
	ldr	r7, [sp, #68]                   @ 4-byte Reload
	str	r1, [r0, #24]
	ldr	r1, [sp, #12]                   @ 4-byte Reload
	movpl	r6, r7
	ldr	r7, [sp, #72]                   @ 4-byte Reload
	str	r11, [r0, #28]
	str	r12, [r0, #32]
	movpl	r5, r7
	ldr	r7, [sp, #80]                   @ 4-byte Reload
	cmp	r8, #0
	str	lr, [r0, #36]
	str	r4, [r0, #40]
	movpl	r3, r8
	movpl	r1, r7
	ldr	r7, [sp, #76]                   @ 4-byte Reload
	str	r6, [r0, #44]
	str	r5, [r0, #48]
	add	r0, r0, #52
	movpl	r2, r7
	stm	r0, {r1, r2, r3}
	add	sp, sp, #84
	pop	{r4, r5, r6, r7, r8, r9, r10, r11, lr}
	mov	pc, lr
.Lfunc_end87:
	.size	mcl_fp_subNF16L, .Lfunc_end87-mcl_fp_subNF16L
	.cantunwind
	.fnend
                                        @ -- End function
	.globl	mcl_fpDbl_add16L                @ -- Begin function mcl_fpDbl_add16L
	.p2align	2
	.type	mcl_fpDbl_add16L,%function
	.code	32                              @ @mcl_fpDbl_add16L
mcl_fpDbl_add16L:
	.fnstart
@ %bb.0:
	.save	{r4, r5, r6, r7, r8, r9, r10, r11, lr}
	push	{r4, r5, r6, r7, r8, r9, r10, r11, lr}
	.pad	#140
	sub	sp, sp, #140
	ldr	r9, [r2]
	ldm	r1, {r4, r5, r6, r7}
	ldmib	r2, {r8, lr}
	adds	r10, r4, r9
	adcs	r11, r5, r8
	ldr	r12, [r2, #12]
	adcs	r6, r6, lr
	str	r6, [sp, #104]                  @ 4-byte Spill
	adcs	r7, r7, r12
	str	r7, [sp, #92]                   @ 4-byte Spill
	ldr	r6, [r2, #16]
	ldr	r7, [r1, #16]
	ldr	r4, [r2, #20]
	adcs	r7, r7, r6
	str	r7, [sp, #88]                   @ 4-byte Spill
	ldr	r7, [r1, #20]
	ldr	r6, [r1, #24]
	adcs	r7, r7, r4
	str	r7, [sp, #84]                   @ 4-byte Spill
	ldr	r7, [r2, #24]
	ldr	r4, [r3, #8]
	adcs	r7, r6, r7
	str	r7, [sp, #80]                   @ 4-byte Spill
	ldr	r7, [r2, #28]
	ldr	r6, [r1, #28]
	ldr	r12, [r3, #12]
	adcs	r7, r6, r7
	str	r7, [sp, #68]                   @ 4-byte Spill
	ldr	r7, [r2, #32]
	ldr	r6, [r1, #32]
	stm	r0, {r10, r11}
	add	r10, r3, #40
	adcs	r7, r6, r7
	str	r7, [sp, #64]                   @ 4-byte Spill
	add	r11, r3, #28
	ldr	r7, [r2, #36]
	ldr	r6, [r1, #36]
	adcs	r7, r6, r7
	str	r7, [sp, #52]                   @ 4-byte Spill
	ldr	r7, [r2, #40]
	ldr	r6, [r1, #40]
	adcs	r7, r6, r7
	str	r7, [sp, #48]                   @ 4-byte Spill
	ldr	r7, [r2, #44]
	ldr	r6, [r1, #44]
	adcs	r7, r6, r7
	str	r7, [sp, #36]                   @ 4-byte Spill
	ldr	r7, [r2, #48]
	ldr	r6, [r1, #48]
	adcs	r7, r6, r7
	str	r7, [sp, #32]                   @ 4-byte Spill
	ldr	r7, [r2, #52]
	ldr	r6, [r1, #52]
	adcs	r7, r6, r7
	str	r7, [sp, #28]                   @ 4-byte Spill
	ldr	r7, [r2, #56]
	ldr	r6, [r1, #56]
	adcs	r7, r6, r7
	str	r7, [sp, #24]                   @ 4-byte Spill
	ldr	r7, [r2, #60]
	ldr	r6, [r1, #60]
	adcs	r7, r6, r7
	str	r7, [sp, #20]                   @ 4-byte Spill
	ldr	r7, [r2, #64]
	ldr	r6, [r1, #64]
	adcs	lr, r6, r7
	ldr	r7, [r2, #68]
	ldr	r6, [r1, #68]
	str	lr, [sp, #100]                  @ 4-byte Spill
	adcs	r8, r6, r7
	ldr	r7, [r2, #72]
	ldr	r6, [r1, #72]
	str	r8, [sp, #76]                   @ 4-byte Spill
	adcs	r9, r6, r7
	ldr	r7, [r2, #76]
	ldr	r6, [r1, #76]
	str	r9, [sp, #60]                   @ 4-byte Spill
	adcs	r5, r6, r7
	ldr	r7, [r2, #80]
	ldr	r6, [r1, #80]
	str	r5, [sp, #44]                   @ 4-byte Spill
	adcs	r7, r6, r7
	str	r7, [sp, #136]                  @ 4-byte Spill
	ldr	r7, [r2, #84]
	ldr	r6, [r1, #84]
	adcs	r7, r6, r7
	str	r7, [sp, #132]                  @ 4-byte Spill
	ldr	r7, [r2, #88]
	ldr	r6, [r1, #88]
	adcs	r7, r6, r7
	str	r7, [sp, #128]                  @ 4-byte Spill
	ldr	r7, [r2, #92]
	ldr	r6, [r1, #92]
	adcs	r7, r6, r7
	str	r7, [sp, #124]                  @ 4-byte Spill
	ldr	r7, [r2, #96]
	ldr	r6, [r1, #96]
	adcs	r7, r6, r7
	str	r7, [sp, #120]                  @ 4-byte Spill
	ldr	r7, [r2, #100]
	ldr	r6, [r1, #100]
	adcs	r7, r6, r7
	str	r7, [sp, #116]                  @ 4-byte Spill
	ldr	r7, [r2, #104]
	ldr	r6, [r1, #104]
	adcs	r7, r6, r7
	str	r7, [sp, #112]                  @ 4-byte Spill
	ldr	r7, [r2, #108]
	ldr	r6, [r1, #108]
	adcs	r7, r6, r7
	str	r7, [sp, #108]                  @ 4-byte Spill
	ldr	r7, [r2, #112]
	ldr	r6, [r1, #112]
	adcs	r7, r6, r7
	str	r7, [sp, #16]                   @ 4-byte Spill
	ldr	r7, [r2, #116]
	ldr	r6, [r1, #116]
	adcs	r7, r6, r7
	str	r7, [sp, #12]                   @ 4-byte Spill
	ldr	r7, [r2, #120]
	ldr	r6, [r1, #120]
	ldr	r2, [r2, #124]
	ldr	r1, [r1, #124]
	adcs	r7, r6, r7
	str	r7, [sp, #8]                    @ 4-byte Spill
	adcs	r1, r1, r2
	str	r1, [sp, #4]                    @ 4-byte Spill
	mov	r1, #0
	ldm	r3, {r2, r6}
	adc	r1, r1, #0
	str	r1, [sp]                        @ 4-byte Spill
	subs	r1, lr, r2
	str	r1, [sp, #96]                   @ 4-byte Spill
	sbcs	r1, r8, r6
	str	r1, [sp, #72]                   @ 4-byte Spill
	sbcs	r1, r9, r4
	str	r1, [sp, #56]                   @ 4-byte Spill
	sbcs	r1, r5, r12
	str	r1, [sp, #40]                   @ 4-byte Spill
	ldr	r1, [sp, #104]                  @ 4-byte Reload
	str	r1, [r0, #8]
	ldr	r1, [sp, #92]                   @ 4-byte Reload
	str	r1, [r0, #12]
	ldr	r1, [sp, #88]                   @ 4-byte Reload
	str	r1, [r0, #16]
	ldr	r1, [sp, #84]                   @ 4-byte Reload
	str	r1, [r0, #20]
	ldr	r1, [sp, #80]                   @ 4-byte Reload
	str	r1, [r0, #24]
	ldr	r1, [sp, #68]                   @ 4-byte Reload
	ldr	r12, [r3, #24]
	ldr	lr, [r3, #20]
	ldr	r3, [r3, #16]
	ldr	r4, [sp, #136]                  @ 4-byte Reload
	str	r1, [r0, #28]
	ldr	r1, [sp, #64]                   @ 4-byte Reload
	sbcs	r3, r4, r3
	str	r1, [r0, #32]
	ldr	r1, [sp, #52]                   @ 4-byte Reload
	str	r1, [r0, #36]
	ldr	r1, [sp, #48]                   @ 4-byte Reload
	str	r3, [sp, #104]                  @ 4-byte Spill
	ldr	r3, [sp, #132]                  @ 4-byte Reload
	str	r1, [r0, #40]
	ldr	r1, [sp, #36]                   @ 4-byte Reload
	sbcs	r3, r3, lr
	str	r1, [r0, #44]
	ldr	r1, [sp, #32]                   @ 4-byte Reload
	str	r1, [r0, #48]
	ldr	r1, [sp, #28]                   @ 4-byte Reload
	str	r3, [sp, #92]                   @ 4-byte Spill
	ldr	r3, [sp, #128]                  @ 4-byte Reload
	str	r1, [r0, #52]
	ldr	r1, [sp, #24]                   @ 4-byte Reload
	sbcs	r3, r3, r12
	str	r1, [r0, #56]
	ldr	r1, [sp, #20]                   @ 4-byte Reload
	str	r1, [r0, #60]
	str	r3, [sp, #88]                   @ 4-byte Spill
	ldm	r11, {r1, r2, r11}
	ldr	r3, [sp, #124]                  @ 4-byte Reload
	ldm	r10, {r5, r6, r7, r8, r9, r10}
	sbcs	r1, r3, r1
	str	r1, [sp, #84]                   @ 4-byte Spill
	ldr	r1, [sp, #120]                  @ 4-byte Reload
	ldr	r12, [sp, #16]                  @ 4-byte Reload
	sbcs	lr, r1, r2
	ldr	r1, [sp, #116]                  @ 4-byte Reload
	ldr	r3, [sp, #12]                   @ 4-byte Reload
	sbcs	r11, r1, r11
	ldr	r1, [sp, #112]                  @ 4-byte Reload
	ldr	r2, [sp, #4]                    @ 4-byte Reload
	sbcs	r5, r1, r5
	ldr	r1, [sp, #108]                  @ 4-byte Reload
	ldr	r4, [sp]                        @ 4-byte Reload
	sbcs	r6, r1, r6
	sbcs	r7, r12, r7
	sbcs	r1, r3, r8
	ldr	r8, [sp, #8]                    @ 4-byte Reload
	sbcs	r9, r8, r9
	sbcs	r10, r2, r10
	sbc	r4, r4, #0
	ands	r4, r4, #1
	movne	r1, r3
	movne	r10, r2
	str	r1, [r0, #116]
	movne	r9, r8
	ldr	r1, [sp, #108]                  @ 4-byte Reload
	cmp	r4, #0
	movne	r7, r12
	ldr	r2, [sp, #84]                   @ 4-byte Reload
	str	r10, [r0, #124]
	movne	r6, r1
	ldr	r1, [sp, #112]                  @ 4-byte Reload
	str	r9, [r0, #120]
	str	r7, [r0, #112]
	movne	r5, r1
	ldr	r1, [sp, #116]                  @ 4-byte Reload
	cmp	r4, #0
	str	r6, [r0, #108]
	str	r5, [r0, #104]
	movne	r11, r1
	ldr	r1, [sp, #120]                  @ 4-byte Reload
	str	r11, [r0, #100]
	movne	lr, r1
	ldr	r1, [sp, #124]                  @ 4-byte Reload
	str	lr, [r0, #96]
	movne	r2, r1
	ldr	r1, [sp, #128]                  @ 4-byte Reload
	str	r2, [r0, #92]
	cmp	r4, #0
	ldr	r2, [sp, #88]                   @ 4-byte Reload
	movne	r2, r1
	ldr	r1, [sp, #132]                  @ 4-byte Reload
	str	r2, [r0, #88]
	ldr	r2, [sp, #92]                   @ 4-byte Reload
	movne	r2, r1
	ldr	r1, [sp, #136]                  @ 4-byte Reload
	str	r2, [r0, #84]
	ldr	r2, [sp, #104]                  @ 4-byte Reload
	movne	r2, r1
	cmp	r4, #0
	str	r2, [r0, #80]
	ldr	r2, [sp, #44]                   @ 4-byte Reload
	ldr	r1, [sp, #40]                   @ 4-byte Reload
	movne	r1, r2
	ldr	r2, [sp, #60]                   @ 4-byte Reload
	str	r1, [r0, #76]
	ldr	r1, [sp, #56]                   @ 4-byte Reload
	movne	r1, r2
	ldr	r2, [sp, #76]                   @ 4-byte Reload
	str	r1, [r0, #72]
	ldr	r1, [sp, #72]                   @ 4-byte Reload
	movne	r1, r2
	ldr	r2, [sp, #100]                  @ 4-byte Reload
	str	r1, [r0, #68]
	cmp	r4, #0
	ldr	r1, [sp, #96]                   @ 4-byte Reload
	movne	r1, r2
	str	r1, [r0, #64]
	add	sp, sp, #140
	pop	{r4, r5, r6, r7, r8, r9, r10, r11, lr}
	mov	pc, lr
.Lfunc_end88:
	.size	mcl_fpDbl_add16L, .Lfunc_end88-mcl_fpDbl_add16L
	.cantunwind
	.fnend
                                        @ -- End function
	.globl	mcl_fpDbl_sub16L                @ -- Begin function mcl_fpDbl_sub16L
	.p2align	2
	.type	mcl_fpDbl_sub16L,%function
	.code	32                              @ @mcl_fpDbl_sub16L
mcl_fpDbl_sub16L:
	.fnstart
@ %bb.0:
	.save	{r4, r5, r6, r7, r8, r9, r10, r11, lr}
	push	{r4, r5, r6, r7, r8, r9, r10, r11, lr}
	.pad	#148
	sub	sp, sp, #148
	ldr	r7, [r2, #100]
	add	r9, r1, #12
	str	r7, [sp, #116]                  @ 4-byte Spill
	ldr	r7, [r2, #96]
	str	r7, [sp, #124]                  @ 4-byte Spill
	ldr	r7, [r2, #64]
	str	r7, [sp, #120]                  @ 4-byte Spill
	ldr	r7, [r2, #68]
	str	r7, [sp, #108]                  @ 4-byte Spill
	ldr	r7, [r2, #72]
	str	r7, [sp, #112]                  @ 4-byte Spill
	ldr	r7, [r2, #76]
	str	r7, [sp, #144]                  @ 4-byte Spill
	ldr	r7, [r2, #80]
	str	r7, [sp, #140]                  @ 4-byte Spill
	ldr	r7, [r2, #84]
	str	r7, [sp, #136]                  @ 4-byte Spill
	ldr	r7, [r2, #88]
	str	r7, [sp, #132]                  @ 4-byte Spill
	ldr	r7, [r2, #92]
	str	r7, [sp, #128]                  @ 4-byte Spill
	ldr	r7, [r2, #32]
	str	r7, [sp, #48]                   @ 4-byte Spill
	ldr	r7, [r2, #36]
	str	r7, [sp, #44]                   @ 4-byte Spill
	ldr	r7, [r2, #40]
	str	r7, [sp, #84]                   @ 4-byte Spill
	ldr	r7, [r2, #44]
	str	r7, [sp, #88]                   @ 4-byte Spill
	ldr	r7, [r2, #48]
	str	r7, [sp, #92]                   @ 4-byte Spill
	ldr	r7, [r2, #52]
	str	r7, [sp, #96]                   @ 4-byte Spill
	ldr	r7, [r2, #56]
	str	r7, [sp, #100]                  @ 4-byte Spill
	ldr	r7, [r2, #60]
	ldm	r2, {r4, r11}
	ldm	r1, {r12, lr}
	str	r7, [sp, #104]                  @ 4-byte Spill
	subs	r4, r12, r4
	ldr	r7, [r2, #8]
	str	r7, [sp, #72]                   @ 4-byte Spill
	ldr	r7, [r2, #12]
	str	r7, [sp, #68]                   @ 4-byte Spill
	ldr	r7, [r2, #16]
	str	r4, [sp, #80]                   @ 4-byte Spill
	sbcs	r4, lr, r11
	str	r7, [sp, #64]                   @ 4-byte Spill
	add	lr, r1, #40
	ldr	r7, [r2, #20]
	str	r4, [sp, #76]                   @ 4-byte Spill
	ldr	r10, [r1, #8]
	ldr	r4, [sp, #72]                   @ 4-byte Reload
	str	r7, [sp, #60]                   @ 4-byte Spill
	ldr	r7, [r2, #24]
	sbcs	r4, r10, r4
	str	r7, [sp, #56]                   @ 4-byte Spill
	ldr	r7, [r2, #28]
	str	r7, [sp, #52]                   @ 4-byte Spill
	str	r4, [sp, #72]                   @ 4-byte Spill
	ldm	r9, {r5, r6, r7, r8, r9}
	ldr	r4, [sp, #68]                   @ 4-byte Reload
	sbcs	r5, r5, r4
	str	r5, [sp, #68]                   @ 4-byte Spill
	ldr	r5, [sp, #64]                   @ 4-byte Reload
	ldr	r4, [sp, #48]                   @ 4-byte Reload
	sbcs	r6, r6, r5
	str	r6, [sp, #64]                   @ 4-byte Spill
	ldr	r6, [sp, #60]                   @ 4-byte Reload
	ldm	lr, {r5, r10, r12, lr}
	sbcs	r7, r7, r6
	str	r7, [sp, #60]                   @ 4-byte Spill
	ldr	r7, [sp, #56]                   @ 4-byte Reload
	ldr	r6, [r1, #36]
	sbcs	r7, r8, r7
	str	r7, [sp, #56]                   @ 4-byte Spill
	ldr	r7, [sp, #52]                   @ 4-byte Reload
	ldr	r8, [r1, #56]
	sbcs	r7, r9, r7
	str	r7, [sp, #52]                   @ 4-byte Spill
	ldr	r7, [r1, #32]
	ldr	r9, [r1, #60]
	sbcs	r7, r7, r4
	str	r7, [sp, #48]                   @ 4-byte Spill
	ldr	r7, [sp, #44]                   @ 4-byte Reload
	sbcs	r6, r6, r7
	str	r6, [sp, #44]                   @ 4-byte Spill
	ldr	r6, [sp, #84]                   @ 4-byte Reload
	sbcs	r5, r5, r6
	str	r5, [sp, #40]                   @ 4-byte Spill
	ldr	r5, [sp, #88]                   @ 4-byte Reload
	ldr	r6, [r1, #68]
	sbcs	r4, r10, r5
	str	r4, [sp, #36]                   @ 4-byte Spill
	ldr	r4, [sp, #92]                   @ 4-byte Reload
	sbcs	r7, r12, r4
	str	r7, [sp, #32]                   @ 4-byte Spill
	ldr	r7, [sp, #96]                   @ 4-byte Reload
	ldr	r4, [sp, #120]                  @ 4-byte Reload
	sbcs	r7, lr, r7
	str	r7, [sp, #28]                   @ 4-byte Spill
	ldr	r7, [sp, #100]                  @ 4-byte Reload
	add	lr, r1, #72
	sbcs	r7, r8, r7
	str	r7, [sp, #24]                   @ 4-byte Spill
	ldr	r7, [sp, #104]                  @ 4-byte Reload
	ldm	lr, {r5, r10, r12, lr}
	sbcs	r7, r9, r7
	str	r7, [sp, #20]                   @ 4-byte Spill
	ldr	r7, [r1, #64]
	ldr	r8, [r1, #88]
	sbcs	r4, r7, r4
	ldr	r7, [sp, #108]                  @ 4-byte Reload
	str	r4, [sp, #120]                  @ 4-byte Spill
	sbcs	r11, r6, r7
	ldr	r6, [sp, #112]                  @ 4-byte Reload
	ldr	r9, [r1, #92]
	sbcs	r5, r5, r6
	ldr	r6, [sp, #144]                  @ 4-byte Reload
	str	r11, [sp, #100]                 @ 4-byte Spill
	sbcs	r4, r10, r6
	str	r4, [sp, #144]                  @ 4-byte Spill
	ldr	r4, [sp, #140]                  @ 4-byte Reload
	add	r10, r3, #40
	ldr	r6, [sp, #124]                  @ 4-byte Reload
	sbcs	r7, r12, r4
	str	r7, [sp, #140]                  @ 4-byte Spill
	ldr	r7, [sp, #136]                  @ 4-byte Reload
	ldr	r4, [r3, #8]
	sbcs	r7, lr, r7
	str	r7, [sp, #136]                  @ 4-byte Spill
	ldr	r7, [sp, #132]                  @ 4-byte Reload
	ldr	r12, [r3, #12]
	sbcs	r7, r8, r7
	str	r7, [sp, #132]                  @ 4-byte Spill
	ldr	r7, [sp, #128]                  @ 4-byte Reload
	ldr	lr, [r3, #20]
	sbcs	r7, r9, r7
	str	r7, [sp, #128]                  @ 4-byte Spill
	ldr	r7, [r1, #96]
	str	r5, [sp, #92]                   @ 4-byte Spill
	sbcs	r7, r7, r6
	str	r7, [sp, #124]                  @ 4-byte Spill
	ldr	r7, [r1, #100]
	ldr	r6, [sp, #116]                  @ 4-byte Reload
	sbcs	r7, r7, r6
	str	r7, [sp, #116]                  @ 4-byte Spill
	ldr	r7, [r2, #104]
	ldr	r6, [r1, #104]
	sbcs	r7, r6, r7
	str	r7, [sp, #112]                  @ 4-byte Spill
	ldr	r7, [r2, #108]
	ldr	r6, [r1, #108]
	sbcs	r7, r6, r7
	str	r7, [sp, #108]                  @ 4-byte Spill
	ldr	r7, [r2, #112]
	ldr	r6, [r1, #112]
	sbcs	r7, r6, r7
	str	r7, [sp, #16]                   @ 4-byte Spill
	ldr	r7, [r2, #116]
	ldr	r6, [r1, #116]
	sbcs	r7, r6, r7
	str	r7, [sp, #12]                   @ 4-byte Spill
	ldr	r7, [r2, #120]
	ldr	r6, [r1, #120]
	ldr	r2, [r2, #124]
	ldr	r1, [r1, #124]
	sbcs	r7, r6, r7
	str	r7, [sp, #8]                    @ 4-byte Spill
	sbcs	r1, r1, r2
	str	r1, [sp, #4]                    @ 4-byte Spill
	mov	r1, #0
	ldm	r3, {r2, r6}
	sbc	r1, r1, #0
	str	r1, [sp]                        @ 4-byte Spill
	ldr	r1, [sp, #120]                  @ 4-byte Reload
	adds	r1, r1, r2
	str	r1, [sp, #104]                  @ 4-byte Spill
	adcs	r1, r11, r6
	str	r1, [sp, #96]                   @ 4-byte Spill
	adcs	r1, r5, r4
	str	r1, [sp, #88]                   @ 4-byte Spill
	ldr	r1, [sp, #144]                  @ 4-byte Reload
	add	r11, r3, #28
	ldr	r4, [sp, #140]                  @ 4-byte Reload
	adcs	r1, r1, r12
	str	r1, [sp, #84]                   @ 4-byte Spill
	ldr	r1, [sp, #80]                   @ 4-byte Reload
	str	r1, [r0]
	ldr	r1, [sp, #76]                   @ 4-byte Reload
	str	r1, [r0, #4]
	ldr	r1, [sp, #72]                   @ 4-byte Reload
	str	r1, [r0, #8]
	ldr	r1, [sp, #68]                   @ 4-byte Reload
	str	r1, [r0, #12]
	ldr	r1, [sp, #64]                   @ 4-byte Reload
	str	r1, [r0, #16]
	ldr	r1, [sp, #60]                   @ 4-byte Reload
	str	r1, [r0, #20]
	ldr	r1, [sp, #56]                   @ 4-byte Reload
	str	r1, [r0, #24]
	ldr	r1, [sp, #52]                   @ 4-byte Reload
	ldr	r12, [r3, #24]
	ldr	r3, [r3, #16]
	str	r1, [r0, #28]
	ldr	r1, [sp, #48]                   @ 4-byte Reload
	adcs	r3, r4, r3
	str	r1, [r0, #32]
	ldr	r1, [sp, #44]                   @ 4-byte Reload
	str	r1, [r0, #36]
	ldr	r1, [sp, #40]                   @ 4-byte Reload
	str	r3, [sp, #80]                   @ 4-byte Spill
	ldr	r3, [sp, #136]                  @ 4-byte Reload
	str	r1, [r0, #40]
	ldr	r1, [sp, #36]                   @ 4-byte Reload
	adcs	r3, r3, lr
	str	r1, [r0, #44]
	ldr	r1, [sp, #32]                   @ 4-byte Reload
	str	r1, [r0, #48]
	ldr	r1, [sp, #28]                   @ 4-byte Reload
	str	r3, [sp, #76]                   @ 4-byte Spill
	ldr	r3, [sp, #132]                  @ 4-byte Reload
	str	r1, [r0, #52]
	ldr	r1, [sp, #24]                   @ 4-byte Reload
	adcs	r3, r3, r12
	str	r1, [r0, #56]
	ldr	r1, [sp, #20]                   @ 4-byte Reload
	str	r1, [r0, #60]
	str	r3, [sp, #72]                   @ 4-byte Spill
	ldm	r11, {r1, r2, r11}
	ldr	r3, [sp, #128]                  @ 4-byte Reload
	ldm	r10, {r5, r6, r7, r8, r9, r10}
	adcs	r1, r3, r1
	str	r1, [sp, #68]                   @ 4-byte Spill
	ldr	r1, [sp, #124]                  @ 4-byte Reload
	ldr	lr, [sp, #16]                   @ 4-byte Reload
	adcs	r1, r1, r2
	str	r1, [sp, #64]                   @ 4-byte Spill
	ldr	r1, [sp, #116]                  @ 4-byte Reload
	ldr	r12, [sp, #12]                  @ 4-byte Reload
	adcs	r11, r1, r11
	ldr	r1, [sp, #112]                  @ 4-byte Reload
	ldr	r3, [sp, #8]                    @ 4-byte Reload
	adcs	r5, r1, r5
	ldr	r1, [sp, #108]                  @ 4-byte Reload
	ldr	r2, [sp, #4]                    @ 4-byte Reload
	adcs	r6, r1, r6
	ldr	r4, [sp]                        @ 4-byte Reload
	adcs	r7, lr, r7
	adcs	r1, r12, r8
	adcs	r9, r3, r9
	adc	r10, r2, r10
	ands	r8, r4, #1
	moveq	r1, r12
	moveq	r10, r2
	str	r1, [r0, #116]
	moveq	r9, r3
	ldr	r1, [sp, #108]                  @ 4-byte Reload
	cmp	r8, #0
	moveq	r7, lr
	ldr	r2, [sp, #64]                   @ 4-byte Reload
	str	r10, [r0, #124]
	moveq	r6, r1
	ldr	r1, [sp, #112]                  @ 4-byte Reload
	str	r9, [r0, #120]
	str	r7, [r0, #112]
	moveq	r5, r1
	ldr	r1, [sp, #116]                  @ 4-byte Reload
	cmp	r8, #0
	str	r6, [r0, #108]
	str	r5, [r0, #104]
	moveq	r11, r1
	ldr	r1, [sp, #124]                  @ 4-byte Reload
	str	r11, [r0, #100]
	moveq	r2, r1
	ldr	r1, [sp, #128]                  @ 4-byte Reload
	str	r2, [r0, #96]
	ldr	r2, [sp, #68]                   @ 4-byte Reload
	moveq	r2, r1
	ldr	r1, [sp, #132]                  @ 4-byte Reload
	str	r2, [r0, #92]
	cmp	r8, #0
	ldr	r2, [sp, #72]                   @ 4-byte Reload
	moveq	r2, r1
	ldr	r1, [sp, #136]                  @ 4-byte Reload
	str	r2, [r0, #88]
	ldr	r2, [sp, #76]                   @ 4-byte Reload
	moveq	r2, r1
	ldr	r1, [sp, #140]                  @ 4-byte Reload
	str	r2, [r0, #84]
	ldr	r2, [sp, #80]                   @ 4-byte Reload
	moveq	r2, r1
	ldr	r1, [sp, #144]                  @ 4-byte Reload
	str	r2, [r0, #80]
	cmp	r8, #0
	ldr	r2, [sp, #84]                   @ 4-byte Reload
	moveq	r2, r1
	ldr	r1, [sp, #88]                   @ 4-byte Reload
	str	r2, [r0, #76]
	ldr	r2, [sp, #92]                   @ 4-byte Reload
	moveq	r1, r2
	ldr	r2, [sp, #100]                  @ 4-byte Reload
	str	r1, [r0, #72]
	ldr	r1, [sp, #96]                   @ 4-byte Reload
	moveq	r1, r2
	ldr	r2, [sp, #120]                  @ 4-byte Reload
	str	r1, [r0, #68]
	cmp	r8, #0
	ldr	r1, [sp, #104]                  @ 4-byte Reload
	moveq	r1, r2
	str	r1, [r0, #64]
	add	sp, sp, #148
	pop	{r4, r5, r6, r7, r8, r9, r10, r11, lr}
	mov	pc, lr
.Lfunc_end89:
	.size	mcl_fpDbl_sub16L, .Lfunc_end89-mcl_fpDbl_sub16L
	.cantunwind
	.fnend
                                        @ -- End function
	.section	".note.GNU-stack","",%progbits
	.eabi_attribute	30, 2	@ Tag_ABI_optimization_goals
