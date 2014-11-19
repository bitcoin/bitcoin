	;; Added by Diederik Huys, March 2013
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
	;; 		r10-r13  = t0-t3
	;; 		r14	 = b.n[0] / t4
	;; 		r15	 = b.n[1] / t5
	;; 		rbx	 = b.n[2] / t6
	;; 		rcx	 = b.n[3] / t7
	;; 		rbp	 = Constant 0FFFFFFFFFFFFFh / t8
	;; 		rsi	 = b.n / b.n[4] / t9

	GLOBAL SYM(secp256k1_fe_mul_inner)
	ALIGN 32
SYM(secp256k1_fe_mul_inner):
	push rbp
	push rbx
	push r12
	push r13
	push r14
	push r15
	push rdx
	mov r14,[rsi+8*0]	; preload b.n[0]. This will be the case until
				; b.n[0] is no longer needed, then we reassign
				; r14 to t4
	;; c=a.n[0] * b.n[0]
   	mov rax,[rdi+0*8]	; load a.n[0]
	mov rbp,0FFFFFFFFFFFFFh
	mul r14			; rdx:rax=a.n[0]*b.n[0]
	mov r15,[rsi+1*8]
	mov r10,rbp		; load modulus into target register for t0
	mov r8,rax
	and r10,rax		; only need lower qword of c
	shrd r8,rdx,52
	xor r9,r9		; c < 2^64, so we ditch the HO part 

	;; c+=a.n[0] * b.n[1] + a.n[1] * b.n[0]
	mov rax,[rdi+0*8]
	mul r15			
	add r8,rax
	adc r9,rdx

	mov rax,[rdi+1*8]
	mul r14			
	mov r11,rbp
	mov rbx,[rsi+2*8]
	add r8,rax
	adc r9,rdx
	and r11,r8
	shrd r8,r9,52
	xor r9,r9
	
	;; c+=a.n[0 1 2] * b.n[2 1 0]
	mov rax,[rdi+0*8]
	mul rbx			
	add r8,rax
	adc r9,rdx

	mov rax,[rdi+1*8]
	mul r15			
	add r8,rax
	adc r9,rdx

	mov rax,[rdi+2*8]
	mul r14
	mov r12,rbp		
	mov rcx,[rsi+3*8]
	add r8,rax
	adc r9,rdx
	and r12,r8		
	shrd r8,r9,52
	xor r9,r9		

	;; c+=a.n[0 1 2 3] * b.n[3 2 1 0]
	mov rax,[rdi+0*8]
	mul rcx			
	add r8,rax
	adc r9,rdx

	mov rax,[rdi+1*8]
	mul rbx			
	add r8,rax
	adc r9,rdx

	mov rax,[rdi+2*8]
	mul r15			
	add r8,rax
	adc r9,rdx
	
	mov rax,[rdi+3*8]
	mul r14			
	mov r13,rbp             
	mov rsi,[rsi+4*8]	; load b.n[4] and destroy pointer
	add r8,rax
	adc r9,rdx
	and r13,r8

	shrd r8,r9,52
	xor r9,r9		


	;; c+=a.n[0 1 2 3 4] * b.n[4 3 2 1 0]
	mov rax,[rdi+0*8]
	mul rsi
	add r8,rax
	adc r9,rdx

	mov rax,[rdi+1*8]
	mul rcx
	add r8,rax
	adc r9,rdx

	mov rax,[rdi+2*8]
	mul rbx			
	add r8,rax
	adc r9,rdx

	mov rax,[rdi+3*8]
	mul r15			
	add r8,rax
	adc r9,rdx

	mov rax,[rdi+4*8]
	mul r14			
	mov r14,rbp             ; load modulus into t4 and destroy a.n[0]
	add r8,rax
	adc r9,rdx
	and r14,r8
	shrd r8,r9,52
	xor r9,r9		

	;; c+=a.n[1 2 3 4] * b.n[4 3 2 1]
	mov rax,[rdi+1*8]
	mul rsi
	add r8,rax
	adc r9,rdx

	mov rax,[rdi+2*8]
	mul rcx
	add r8,rax
	adc r9,rdx

	mov rax,[rdi+3*8]
	mul rbx
	add r8,rax
	adc r9,rdx

	mov rax,[rdi+4*8]
	mul r15
	mov r15,rbp		
	add r8,rax
	adc r9,rdx

	and r15,r8
	shrd r8,r9,52
	xor r9,r9		

	;; c+=a.n[2 3 4] * b.n[4 3 2]
	mov rax,[rdi+2*8]
	mul rsi
	add r8,rax
	adc r9,rdx

	mov rax,[rdi+3*8]
	mul rcx
	add r8,rax
	adc r9,rdx

	mov rax,[rdi+4*8]
	mul rbx
	mov rbx,rbp		
	add r8,rax
	adc r9,rdx

	and rbx,r8		
	shrd r8,r9,52
	xor r9,r9		

	;; c+=a.n[3 4] * b.n[4 3]
	mov rax,[rdi+3*8]
	mul rsi
	add r8,rax
	adc r9,rdx

	mov rax,[rdi+4*8]
	mul rcx
	mov rcx,rbp		
	add r8,rax
	adc r9,rdx
	and rcx,r8		
	shrd r8,r9,52
	xor r9,r9		

	;; c+=a.n[4] * b.n[4]
	mov rax,[rdi+4*8]
	mul rsi
	;; mov rbp,rbp		; modulus already there!
	add r8,rax
	adc r9,rdx
	and rbp,r8 
	shrd r8,r9,52
	xor r9,r9		

	mov rsi,r8		; load c into t9 and destroy b.n[4]

	;; *******************************************************
common_exit_norm:
	mov rdi,01000003D10h	; load constant

	mov rax,r15		; get t5
	mul rdi
	add rax,r10    		; +t0
	adc rdx,0
	mov r10,0FFFFFFFFFFFFFh ; modulus. Sadly, we ran out of registers!
	mov r8,rax		; +c
	and r10,rax
	shrd r8,rdx,52
	xor r9,r9

	mov rax,rbx		; get t6
	mul rdi
	add rax,r11		; +t1
	adc rdx,0
	mov r11,0FFFFFFFFFFFFFh ; modulus
	add r8,rax		; +c
	adc r9,rdx
	and r11,r8
	shrd r8,r9,52
	xor r9,r9

	mov rax,rcx    		; get t7
	mul rdi
	add rax,r12		; +t2
	adc rdx,0
	pop rbx			; retrieve pointer to this.n	
	mov r12,0FFFFFFFFFFFFFh ; modulus
	add r8,rax		; +c
	adc r9,rdx
	and r12,r8
	mov [rbx+2*8],r12	; mov into this.n[2]
	shrd r8,r9,52
	xor r9,r9
	
	mov rax,rbp    		; get t8
	mul rdi
	add rax,r13    		; +t3
	adc rdx,0
	mov r13,0FFFFFFFFFFFFFh ; modulus
	add r8,rax		; +c
	adc r9,rdx
	and r13,r8
	mov [rbx+3*8],r13	; -> this.n[3]
	shrd r8,r9,52
	xor r9,r9
	
	mov rax,rsi    		; get t9
	mul rdi
	add rax,r14    		; +t4
	adc rdx,0
	mov r14,0FFFFFFFFFFFFh	; !!!
	add r8,rax		; +c
	adc r9,rdx
	and r14,r8
	mov [rbx+4*8],r14	; -> this.n[4]
	shrd r8,r9,48		; !!!
	xor r9,r9
	
	mov rax,01000003D1h
	mul r8		
	add rax,r10
	adc rdx,0
	mov r10,0FFFFFFFFFFFFFh ; modulus
	mov r8,rax
	and rax,r10
	shrd r8,rdx,52
	mov [rbx+0*8],rax	; -> this.n[0]
	add r8,r11
	mov [rbx+1*8],r8	; -> this.n[1]

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
	;; 	   	rsi  	 = this.a
	;;  INTERNAL:	rdx:rax  = multiplication accumulator
	;; 		r9:r8    = c
	;; 		r10-r13  = t0-t3
	;; 		r14	 = a.n[0] / t4
	;; 		r15	 = a.n[1] / t5
	;; 		rbx	 = a.n[2] / t6
	;; 		rcx	 = a.n[3] / t7
	;; 		rbp	 = 0FFFFFFFFFFFFFh / t8
	;; 		rsi	 = a.n[4] / t9
	GLOBAL SYM(secp256k1_fe_sqr_inner)
	ALIGN 32
SYM(secp256k1_fe_sqr_inner):
	push rbp
	push rbx
	push r12
	push r13
	push r14
	push r15
	push rsi
	mov rbp,0FFFFFFFFFFFFFh
	
	;; c=a.n[0] * a.n[0]
   	mov r14,[rdi+0*8]	; r14=a.n[0]
	mov r10,rbp		; modulus 
	mov rax,r14
	mul rax
	mov r15,[rdi+1*8]	; a.n[1]
	add r14,r14		; r14=2*a.n[0]
	mov r8,rax
	and r10,rax		; only need lower qword
	shrd r8,rdx,52
	xor r9,r9

	;; c+=2*a.n[0] * a.n[1]
	mov rax,r14		; r14=2*a.n[0]
	mul r15
	mov rbx,[rdi+2*8]	; rbx=a.n[2]
	mov r11,rbp 		; modulus
	add r8,rax
	adc r9,rdx
	and r11,r8
	shrd r8,r9,52
	xor r9,r9
	
	;; c+=2*a.n[0]*a.n[2]+a.n[1]*a.n[1]
	mov rax,r14
	mul rbx
	add r8,rax
	adc r9,rdx

	mov rax,r15
	mov r12,rbp		; modulus
	mul rax
	mov rcx,[rdi+3*8]	; rcx=a.n[3]
	add r15,r15		; r15=a.n[1]*2
	add r8,rax
	adc r9,rdx
	and r12,r8		; only need lower dword
	shrd r8,r9,52
	xor r9,r9		

	;; c+=2*a.n[0]*a.n[3]+2*a.n[1]*a.n[2]
	mov rax,r14
	mul rcx
	add r8,rax
	adc r9,rdx

	mov rax,r15		; rax=2*a.n[1]
	mov r13,rbp		; modulus
	mul rbx
	mov rsi,[rdi+4*8]	; rsi=a.n[4]
	add r8,rax
	adc r9,rdx
	and r13,r8
	shrd r8,r9,52
	xor r9,r9		

	;; c+=2*a.n[0]*a.n[4]+2*a.n[1]*a.n[3]+a.n[2]*a.n[2]
	mov rax,r14		; last time we need 2*a.n[0]
	mul rsi
	add r8,rax
	adc r9,rdx

	mov rax,r15
	mul rcx
	mov r14,rbp		; modulus
	add r8,rax
	adc r9,rdx

	mov rax,rbx
	mul rax
	add rbx,rbx		; rcx=2*a.n[2]
	add r8,rax
	adc r9,rdx
	and r14,r8
	shrd r8,r9,52
	xor r9,r9		

	;; c+=2*a.n[1]*a.n[4]+2*a.n[2]*a.n[3]
	mov rax,r15		; last time we need 2*a.n[1]
	mul rsi
	add r8,rax
	adc r9,rdx

	mov rax,rbx
	mul rcx
	mov r15,rbp		; modulus
	add r8,rax
	adc r9,rdx
	and r15,r8
	shrd r8,r9,52
	xor r9,r9		

	;; c+=2*a.n[2]*a.n[4]+a.n[3]*a.n[3]
	mov rax,rbx		; last time we need 2*a.n[2]
	mul rsi
	add r8,rax
	adc r9,rdx

	mov rax,rcx		; a.n[3]
	mul rax
	mov rbx,rbp		; modulus
	add r8,rax
	adc r9,rdx
	and rbx,r8		; only need lower dword
	lea rax,[2*rcx]
	shrd r8,r9,52
	xor r9,r9		

	;; c+=2*a.n[3]*a.n[4]
	mul rsi
	mov rcx,rbp		; modulus
	add r8,rax
	adc r9,rdx
	and rcx,r8		; only need lower dword
	shrd r8,r9,52
	xor r9,r9		

	;; c+=a.n[4]*a.n[4]
	mov rax,rsi
	mul rax
	;; mov rbp,rbp		; modulus is already there!
	add r8,rax
	adc r9,rdx
	and rbp,r8 
	shrd r8,r9,52
	xor r9,r9		

	mov rsi,r8

	;; *******************************************************
	jmp common_exit_norm
	end

	
