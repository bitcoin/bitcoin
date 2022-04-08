	.arch armv7-a

	.align 2
	.global mcl_fp_addPre64
mcl_fp_addPre64:
	ldm		r1, {r3, r12}
	ldm		r2, {r1, r2}
	adds	r1, r1, r3
	adc		r2, r2, r12
	stm		r0, {r1, r2}
	bx		lr


	.align 2
	.global mcl_fp_addPre96
mcl_fp_addPre96:
	push	{r4, lr}
	ldm		r1, {r1, r3, r12}
	ldm		r2, {r2, r4, lr}
	adds	r1, r1, r2
	adcs	r3, r3, r4
	adc		r12, r12, lr
	stm		r0, {r1, r3, r12}
	pop		{r4, lr}
	bx		lr

# slower
	.align 2
	.global mcl_fp_addPre96_2
mcl_fp_addPre96_2:
	ldr		r3, [r1], #4
	ldr		r12, [r2], #4
	adds	r3, r3, r12
	str		r3, [r0], #4

	ldm		r1, {r1, r3}
	ldm		r2, {r2, r12}
	adcs	r1, r1, r2
	adcs	r3, r3, r12
	stm		r0, {r1, r3}
	bx		lr

	.globl	mcl_fp_addPre128
	.align	2
mcl_fp_addPre128:
	push	{r4, lr}
	ldm		r1!, {r3, r4}
	ldm		r2!, {r12, lr}
	adds	r3, r3, r12
	adcs	r4, r4, lr
	stm		r0!, {r3, r4}
	ldm		r1, {r3, r4}
	ldm		r2, {r12, lr}
	adcs	r3, r3, r12
	adcs	r4, r4, lr
	stm		r0, {r3, r4}
	pop		{r4, lr}
	bx		lr

	# almost same
	.globl	mcl_fp_addPre128_2
	.align	2
cl_fp_addPre128_2:
	push	{r4, r5, r6, lr}
	ldm		r1, {r1, r3, r4, r5}
	ldm		r2, {r2, r6, r12, lr}
	adds	r1, r1, r2
	adcs	r3, r3, r6
	adcs	r4, r4, r12
	adcs	r5, r5, lr
	stm		r0, {r1, r3, r4, r5}
	pop		{r4, r5, r6, lr}
	bx		lr

	.globl	mcl_fp_addPre160
	.align	2
mcl_fp_addPre160:
	push	{r4, lr}
	ldm		r1!, {r3, r4}
	ldm		r2!, {r12, lr}
	adds	r3, r3, r12
	adcs	r4, r4, lr
	stm		r0!, {r3, r4}
	ldm		r1, {r1, r3, r4}
	ldm		r2, {r2, r12, lr}
	adcs	r1, r1, r2
	adcs	r3, r3, r12
	adcs	r4, r4, lr
	stm		r0, {r1, r3, r4}
	pop		{r4, lr}
	bx		lr

	.globl	mcl_fp_addPre192
	.align	2
mcl_fp_addPre192:
	push	{r4, r5, r6, lr}
	ldm		r1!, {r3, r4, r5}
	ldm		r2!, {r6, r12, lr}
	adds	r3, r3, r6
	adcs	r4, r4, r12
	adcs	r5, r5, lr
	stm		r0!, {r3, r4, r5}

	ldm		r1, {r3, r4, r5}
	ldm		r2, {r6, r12, lr}
	adcs	r3, r3, r6
	adcs	r4, r4, r12
	adcs	r5, r5, lr
	stm		r0, {r3, r4, r5}
	pop		{r4, r5, r6, lr}
	bx		lr

	.globl	mcl_fp_addPre224
	.align	2
mcl_fp_addPre224:
	push	{r4, r5, r6, lr}
	ldm		r1!, {r3, r4, r5}
	ldm		r2!, {r6, r12, lr}
	adds	r3, r3, r6
	adcs	r4, r4, r12
	adcs	r5, r5, lr
	stm		r0!, {r3, r4, r5}

	ldm		r1, {r1, r3, r4, r5}
	ldm		r2, {r2, r6, r12, lr}
	adcs	r1, r1, r2
	adcs	r3, r3, r6
	adcs	r4, r4, r12
	adcs	r5, r5, lr
	stm		r0, {r1, r3, r4, r5}
	pop		{r4, r5, r6, lr}
	bx		lr

	.globl	mcl_fp_addPre256
	.align	2
mcl_fp_addPre256:
	push	{r4, r5, r6, r7, r8, lr}
	ldm		r1!, {r3, r4, r5, r6}
	ldm		r2!, {r7, r8, r12, lr}
	adds	r3, r3, r7
	adcs	r4, r4, r8
	adcs	r5, r5, r12
	adcs	r6, r6, lr
	stm		r0!, {r3, r4, r5, r6}

	ldm		r1, {r3, r4, r5, r6}
	ldm		r2, {r7, r8, r12, lr}
	adcs	r3, r3, r7
	adcs	r4, r4, r8
	adcs	r5, r5, r12
	adcs	r6, r6, lr
	stm		r0, {r3, r4, r5, r6}
	pop		{r4, r5, r6, r7, r8, lr}
	bx		lr
