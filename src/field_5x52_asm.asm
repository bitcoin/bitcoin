	;; Copyright (c) 2013-2014 Diederik Huys, Pieter Wuille
	;; Distributed under the MIT software license, see the accompanying
	;; file COPYING or http://www.opensource.org/licenses/mit-license.php.

	;; Changelog:
	;; * March 2013, Diederik Huys: Original version
	;; * November 2014, Pieter Wuille: Updated to use Peter Dettman's parallel
	;;                                 multiplication algorithm
	;;
	;; Provided public procedures:
	;; 	secp256k1_fe_mul_inner
	;; 	secp256k1_fe_sqr_inner
	;;
	;; Needed tools: YASM (http://yasm.tortall.net)
	;;
	;; 

	BITS 64

%ifidn   __OUTPUT_FORMAT__,macho64
%define SYM(x) _ %+ x
%else
%define SYM(x) x
%endif

	;;  Procedure ExSetMult
	;;  Register Layout:
	;;  INPUT: 	rdi	= a->n
	;; 	   	rsi  	= b->n
	;; 	   	rdx  	= r->a
	;; 
	;;  INTERNAL:	rdx:rax  = multiplication accumulator
	;; 		r9:r8    = c
	;;		r10:r14  = a0-a4
	;;		rcx:rbx  = d
	;; 		rbp	 = R
	;; 		rdi	 = t?
	;;		r15	 = b->n
	;;		rsi	 = r->n
	GLOBAL SYM(secp256k1_fe_mul_inner)
	ALIGN 32
SYM(secp256k1_fe_mul_inner):
	push rbp
	push rbx
	push r12
	push r13
	push r14
	push r15
	mov r10,[rdi+0*8]
	mov r11,[rdi+1*8]
	mov r12,[rdi+2*8]
	mov r13,[rdi+3*8]
	mov r14,[rdi+4*8]
	mov rbp,01000003D10h
	mov r15,rsi
	mov rsi,rdx

	;; d += a3 * b0
	mov rax,[r15+0*8]
	mul r13
	mov rbx,rax
	mov rcx,rdx
	;; d += a2 * b1
	mov rax,[r15+1*8]
	mul r12
	add rbx,rax
	adc rcx,rdx
	;; d += a1 * b2
	mov rax,[r15+2*8]
	mul r11
	add rbx,rax
	adc rcx,rdx
	;; d = a0 * b3
	mov rax,[r15+3*8]
	mul r10
	add rbx,rax
	adc rcx,rdx
	;; c = a4 * b4
	mov rax,[r15+4*8]
	mul r14
	mov r8,rax
	mov r9,rdx
	;; d += (c & M) * R
	mov rdx,0fffffffffffffh
	and rax,rdx
	mul rbp
	add rbx,rax
	adc rcx,rdx
	;; c >>= 52 (r8 only)
	shrd r8,r9,52
	;; t3 (stack) = d & M
	mov rdi,rbx
	mov rdx,0fffffffffffffh
	and rdi,rdx
	push rdi
	;; d >>= 52
	shrd rbx,rcx,52
	mov rcx,0
	;; d += a4 * b0
	mov rax,[r15+0*8]
	mul r14
	add rbx,rax
	adc rcx,rdx
	;; d += a3 * b1
	mov rax,[r15+1*8]
	mul r13
	add rbx,rax
	adc rcx,rdx
	;; d += a2 * b2
	mov rax,[r15+2*8]
	mul r12
	add rbx,rax
	adc rcx,rdx
	;; d += a1 * b3
	mov rax,[r15+3*8]
	mul r11
	add rbx,rax
	adc rcx,rdx
	;; d += a0 * b4
	mov rax,[r15+4*8]
	mul r10
	add rbx,rax
	adc rcx,rdx
	;; d += c * R
	mov rax,r8
	mul rbp
	add rbx,rax
	adc rcx,rdx
	;; t4 = d & M (rdi)
	mov rdi,rbx
	mov rdx,0fffffffffffffh
	and rdi,rdx
	;; d >>= 52
	shrd rbx,rcx,52
	mov rcx,0
	;; tx = t4 >> 48 (rbp, overwrites R)
	mov rbp,rdi
	shr rbp,48
	;; t4 &= (M >> 4) (stack)
	mov rax,0ffffffffffffh
	and rdi,rax
	push rdi
	;; c = a0 * b0
	mov rax,[r15+0*8]
	mul r10
	mov r8,rax
	mov r9,rdx
	;; d += a4 * b1
	mov rax,[r15+1*8]
	mul r14
	add rbx,rax
	adc rcx,rdx
	;; d += a3 * b2
	mov rax,[r15+2*8]
	mul r13
	add rbx,rax
	adc rcx,rdx
	;; d += a2 * b3
	mov rax,[r15+3*8]
	mul r12
	add rbx,rax
	adc rcx,rdx
	;; d += a1 * b4
	mov rax,[r15+4*8]
	mul r11
	add rbx,rax
	adc rcx,rdx
	;; u0 = d & M (rdi)
	mov rdi,rbx
	mov rdx,0fffffffffffffh
	and rdi,rdx
	;; d >>= 52
	shrd rbx,rcx,52
	mov rcx,0
	;; u0 = (u0 << 4) | tx (rdi)
	shl rdi,4
	or rdi,rbp
	;; c += u0 * (R >> 4)
	mov rax,01000003D1h
	mul rdi
	add r8,rax
	adc r9,rdx
	;; r[0] = c & M
	mov rax,r8
	mov rdx,0fffffffffffffh
	and rax,rdx
	mov [rsi+0*8],rax
	;; c >>= 52
	shrd r8,r9,52
	mov r9,0
	;; c += a1 * b0
	mov rax,[r15+0*8]
	mul r11
	add r8,rax
	adc r9,rdx
	;; c += a0 * b1
	mov rax,[r15+1*8]
	mul r10
	add r8,rax
	adc r9,rdx
	;; d += a4 * b2
	mov rax,[r15+2*8]
	mul r14
	add rbx,rax
	adc rcx,rdx
	;; d += a3 * b3
	mov rax,[r15+3*8]
	mul r13
	add rbx,rax
	adc rcx,rdx
	;; d += a2 * b4
	mov rax,[r15+4*8]
	mul r12
	add rbx,rax
	adc rcx,rdx
	;; restore rdp = R
	mov rbp,01000003D10h
	;; c += (d & M) * R
	mov rax,rbx
	mov rdx,0fffffffffffffh
	and rax,rdx
	mul rbp
	add r8,rax
	adc r9,rdx
	;; d >>= 52
	shrd rbx,rcx,52
	mov rcx,0
	;; r[1] = c & M
	mov rax,r8
	mov rdx,0fffffffffffffh
	and rax,rdx
	mov [rsi+8*1],rax
	;; c >>= 52
	shrd r8,r9,52
	mov r9,0
	;; c += a2 * b0
	mov rax,[r15+0*8]
	mul r12
	add r8,rax
	adc r9,rdx
	;; c += a1 * b1
	mov rax,[r15+1*8]
	mul r11
	add r8,rax
	adc r9,rdx
	;; c += a0 * b2 (last use of r10 = a0)
	mov rax,[r15+2*8]
	mul r10
	add r8,rax
	adc r9,rdx
	;; fetch t3 (r10, overwrites a0),t4 (rdi)
	pop rdi
	pop r10
	;; d += a4 * b3
	mov rax,[r15+3*8]
	mul r14
	add rbx,rax
	adc rcx,rdx
	;; d += a3 * b4
	mov rax,[r15+4*8]
	mul r13
	add rbx,rax
	adc rcx,rdx
	;; c += (d & M) * R
	mov rax,rbx
	mov rdx,0fffffffffffffh
	and rax,rdx
	mul rbp
	add r8,rax
	adc r9,rdx
	;; d >>= 52 (rbx only)
	shrd rbx,rcx,52
	;; r[2] = c & M
	mov rax,r8
	mov rdx,0fffffffffffffh
	and rax,rdx
	mov [rsi+2*8],rax
	;; c >>= 52
	shrd r8,r9,52
	mov r9,0
	;; c += t3
	add r8,r10
	;; c += d * R
	mov rax,rbx
	mul rbp
	add r8,rax
	adc r9,rdx
	;; r[3] = c & M
	mov rax,r8
	mov rdx,0fffffffffffffh
	and rax,rdx
	mov [rsi+3*8],rax
	;; c >>= 52 (r8 only)
	shrd r8,r9,52
	;; c += t4 (r8 only)
	add r8,rdi
	;; r[4] = c
	mov [rsi+4*8],r8

	pop r15
	pop r14
	pop r13
	pop r12
	pop rbx
	pop rbp
	ret

	
	;;  PROC ExSetSquare
	;;  Register Layout:
	;;  INPUT: 	rdi	 = a.n
	;; 	   	rsi  	 = r.n
	;;  INTERNAL:	rdx:rax  = multiplication accumulator
	;; 		r9:r8    = c
	;;		r10:r14  = a0-a4
	;;		rcx:rbx  = d
	;; 		rbp	 = R
	;; 		rdi	 = t?
	;;		r15	 = M
	GLOBAL SYM(secp256k1_fe_sqr_inner)
	ALIGN 32
SYM(secp256k1_fe_sqr_inner):
	push rbp
	push rbx
	push r12
	push r13
	push r14
	push r15
	mov r10,[rdi+0*8]
	mov r11,[rdi+1*8]
	mov r12,[rdi+2*8]
	mov r13,[rdi+3*8]
	mov r14,[rdi+4*8]
	mov rbp,01000003D10h
	mov r15,0fffffffffffffh

	;; d = (a0*2) * a3
	lea rax,[r10*2]
	mul r13
	mov rbx,rax
	mov rcx,rdx
	;; d += (a1*2) * a2
	lea rax,[r11*2]
	mul r12
	add rbx,rax
	adc rcx,rdx
	;; c = a4 * a4
	mov rax,r14
	mul r14
	mov r8,rax
	mov r9,rdx
	;; d += (c & M) * R
	and rax,r15
	mul rbp
	add rbx,rax
	adc rcx,rdx
	;; c >>= 52 (r8 only)
	shrd r8,r9,52
	;; t3 (stack) = d & M
	mov rdi,rbx
	and rdi,r15
	push rdi
	;; d >>= 52
	shrd rbx,rcx,52
	mov rcx,0
	;; a4 *= 2
	add r14,r14
	;; d += a0 * a4
	mov rax,r10
	mul r14
	add rbx,rax
	adc rcx,rdx
	;; d+= (a1*2) * a3
	lea rax,[r11*2]
	mul r13
	add rbx,rax
	adc rcx,rdx
	;; d += a2 * a2
	mov rax,r12
	mul r12
	add rbx,rax
	adc rcx,rdx
	;; d += c * R
	mov rax,r8
	mul rbp
	add rbx,rax
	adc rcx,rdx
	;; t4 = d & M (rdi)
	mov rdi,rbx
	and rdi,r15
	;; d >>= 52
	shrd rbx,rcx,52
	mov rcx,0
	;; tx = t4 >> 48 (rbp, overwrites constant)
	mov rbp,rdi
	shr rbp,48
	;; t4 &= (M >> 4) (stack)
	mov rax,0ffffffffffffh
	and rdi,rax
	push rdi
	;; c = a0 * a0
	mov rax,r10
	mul r10
	mov r8,rax
	mov r9,rdx
	;; d += a1 * a4
	mov rax,r11
	mul r14
	add rbx,rax
	adc rcx,rdx
	;; d += (a2*2) * a3
	lea rax,[r12*2]
	mul r13
	add rbx,rax
	adc rcx,rdx
	;; u0 = d & M (rdi)
	mov rdi,rbx
	and rdi,r15
	;; d >>= 52
	shrd rbx,rcx,52
	mov rcx,0
	;; u0 = (u0 << 4) | tx (rdi)
	shl rdi,4
	or rdi,rbp
	;; c += u0 * (R >> 4)
	mov rax,01000003D1h
	mul rdi
	add r8,rax
	adc r9,rdx
	;; r[0] = c & M
	mov rax,r8
	and rax,r15
	mov [rsi+0*8],rax
	;; c >>= 52
	shrd r8,r9,52
	mov r9,0
	;; a0 *= 2
	add r10,r10
	;; c += a0 * a1
	mov rax,r10
	mul r11
	add r8,rax
	adc r9,rdx
	;; d += a2 * a4
	mov rax,r12
	mul r14
	add rbx,rax
	adc rcx,rdx
	;; d += a3 * a3
	mov rax,r13
	mul r13
	add rbx,rax
	adc rcx,rdx
	;; load R in rbp
	mov rbp,01000003D10h
	;; c += (d & M) * R
	mov rax,rbx
	and rax,r15
	mul rbp
	add r8,rax
	adc r9,rdx
	;; d >>= 52
	shrd rbx,rcx,52
	mov rcx,0
	;; r[1] = c & M
	mov rax,r8
	and rax,r15
	mov [rsi+8*1],rax
	;; c >>= 52
	shrd r8,r9,52
	mov r9,0
	;; c += a0 * a2 (last use of r10)
	mov rax,r10
	mul r12
	add r8,rax
	adc r9,rdx
	;; fetch t3 (r10, overwrites a0),t4 (rdi)
	pop rdi
	pop r10
	;; c += a1 * a1
	mov rax,r11
	mul r11
	add r8,rax
	adc r9,rdx
	;; d += a3 * a4
	mov rax,r13
	mul r14
	add rbx,rax
	adc rcx,rdx
	;; c += (d & M) * R
	mov rax,rbx
	and rax,r15
	mul rbp
	add r8,rax
	adc r9,rdx
	;; d >>= 52 (rbx only)
	shrd rbx,rcx,52
	;; r[2] = c & M
	mov rax,r8
	and rax,r15
	mov [rsi+2*8],rax
	;; c >>= 52
	shrd r8,r9,52
	mov r9,0
	;; c += t3
	add r8,r10
	;; c += d * R
	mov rax,rbx
	mul rbp
	add r8,rax
	adc r9,rdx
	;; r[3] = c & M
	mov rax,r8
	and rax,r15
	mov [rsi+3*8],rax
	;; c >>= 52 (r8 only)
	shrd r8,r9,52
	;; c += t4 (r8 only)
	add r8,rdi
	;; r[4] = c
	mov [rsi+4*8],r8

	pop r15
	pop r14
	pop r13
	pop r12
	pop rbx
	pop rbp
	ret
