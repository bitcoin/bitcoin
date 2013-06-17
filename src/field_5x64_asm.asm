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

COMP_LIMB EQU 000000001000003D1h
	
	;;  Procedure ExSetMult
	;;  Register Layout:
	;;  INPUT: 	rdi	= a->n
	;; 	   	rsi  	= b->n
	;; 	   	rdx  	= r->a
	;; 
	;;  INTERNAL:	rdx:rax  = multiplication accumulator
	;; 		r8-r10   = c0-c2
	;; 		r11-r15  = b.n[0]-b.n[4] / r3 - r7
	;; 		rbx	 = r0
	;; 		rcx	 = r1
	;; 		rbp	 = r2
	;; 	  
	GLOBAL secp256k1_fe_mul_inner
	ALIGN 32
secp256k1_fe_mul_inner:
	push rbp
	push rbx
	push r12
	push r13
	push r14
	push r15
	push rdx

	mov r11,[rsi+8*0]	; preload b.n[0]

	;;  step 1: mul_c2
   	mov rax,[rdi+0*8]	; load a.n[0]
	mul r11			; rdx:rax=a.n[0]*b.n[0]
	mov r12,[rsi+1*8]	; preload b.n[1]
	mov rbx,rax		; retire LO qword (r[0])
	mov r8,rdx		; save overflow
	xor r9,r9		; overflow HO qwords
	xor r10,r10
	
	;; c+=a.n[0] * b.n[1] + a.n[1] * b.n[0]
	mov rax,[rdi+0*8]
	mul r12				
	mov r13,[rsi+2*8]	; preload b.n[2]
	add r8,rax		; still the same :-)
	adc r9,rdx		; 
	adc r10,0		; mmm...
	
	mov rax,[rdi+1*8]
	mul r11			
	add r8,rax
	adc r9,rdx
	adc r10,0
	mov rcx,r8	; retire r[1]
	xor r8,r8
	
	;; c+=a.n[0 1 2] * b.n[2 1 0]
	mov rax,[rdi+0*8]
	mul r13			
	mov r14,[rsi+3*8]	; preload b.n[3]
	add r9,rax
	adc r10,rdx
	adc r8,0
	
	mov rax,[rdi+1*8]
	mul r12			
	add r9,rax
	adc r10,rdx
	adc r8,0
	
	mov rax,[rdi+2*8]
	mul r11
	add r9,rax
	adc r10,rdx
	adc r8,0
	mov rbp,r9		; retire r[2]
	xor r9,r9

	;; c+=a.n[0 1 2 3] * b.n[3 2 1 0]
	mov rax,[rdi+0*8]
	mul r14		
	add r10,rax
	adc r8,rdx
	adc r9,0

	mov rax,[rdi+1*8]
	mul r13
	add r10,rax
	adc r8,rdx
	adc r9,0

	mov rax,[rdi+2*8]
	mul r12
	add r10,rax
	adc r8,rdx
	adc r9,0
	
	mov rax,[rdi+3*8]
	mul r11			
	add r10,rax
	adc r8,rdx
	adc r9,0
	mov r11,r10		; retire r[3]
	xor r10,r10

	;; c+=a.n[1 2 3] * b.n[3 2 1]
	mov rax,[rdi+1*8]
	mul r14		
	add r8,rax
	adc r9,rdx
	adc r10,0
	
	mov rax,[rdi+2*8]
	mul r13		
	add r8,rax
	adc r9,rdx
	adc r10,0
	
	mov rax,[rdi+3*8]
	mul r12
	add r8,rax
	adc r9,rdx
	adc r10,0
	mov r12,r8		; retire r[4]
	xor r8,r8		

	;; c+=a.n[2 3] * b.n[3 2]
	mov rax,[rdi+2*8]
	mul r14
	add r9,rax		; still the same :-)
	adc r10,rdx		; 
	adc r8,0		; mmm...
	
	mov rax,[rdi+3*8]
	mul r13		
	add r9,rax
	adc r10,rdx
	adc r8,0
	mov r13,r9		; retire r[5]
	xor r9,r9

	;; c+=a.n[3] * b.n[3]
	mov rax,[rdi+3*8]
	mul r14
	add r10,rax
	adc r8,rdx
	
	mov r14,r10
	mov r15,r8
	

	;; *******************************************************
common_exit_norm:
	mov rdi,COMP_LIMB
	mov rax,r12
	mul rdi
	add rax,rbx
	adc rcx,rdx
	pop rbx
	mov [rbx],rax

	mov rax,r13		; get r5
	mul rdi
	add rax,rcx    		; +r1
	adc rbp,rdx
	mov [rbx+1*8],rax
	
	mov rax,r14		; get r6
	mul rdi
	add rax,rbp    		; +r2
	adc r11,rdx
	mov [rbx+2*8],rax
	
	mov rax,r15		; get r7
	mul rdi
	add rax,r11    		; +r3
	adc rdx,0
	mov [rbx+3*8],rax
	mov [rbx+4*8],rdx
	
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
	;; 		r8-r10   = c
	;; 		r11-r15  = a.n[0]-a.n[4] / r3-r7
	;; 		rbx	 = r0
	;; 		rcx	 = r1
	;; 		rbp	 = r2
	GLOBAL secp256k1_fe_sqr_inner
	
	ALIGN 32
secp256k1_fe_sqr_inner:
	push rbp
	push rbx
	push r12
	push r13
	push r14
	push r15
	push rsi

	mov r11,[rdi+8*0]	; preload a.n[0]
	
	;;  step 1: mul_c2
   	mov rax,r11		; load a.n[0]
	mul rax			; rdx:rax=a.n[0]²
	mov r12,[rdi+1*8]	; preload a.n[1]
	mov rbx,rax		; retire LO qword (r[0])
	mov r8,rdx		; save overflow
	xor r9,r9		; overflow HO qwords
	xor r10,r10
	
	;; c+=2*a.n[0] * a.n[1]
	mov rax,r11		; load a.n[0]
	mul r12			; rdx:rax=a.n[0] * a.n[1]
	mov r13,[rdi+2*8]	; preload a.n[2]
	add rax,rax		; rdx:rax*=2
	adc rdx,rdx
	adc r10,0
	add r8,rax		; still the same :-)
	adc r9,rdx		
	adc r10,0		; mmm...
	
	mov rcx,r8		; retire r[1]
	xor r8,r8

	;; c+=2*a.n[0]*a.n[2]+a.n[1]*a.n[1]
	mov rax,r11		; load a.n[0]
	mul r13			; * a.n[2]
	mov r14,[rdi+3*8]	; preload a.n[3]
	add rax,rax		; rdx:rax*=2
	adc rdx,rdx
	adc r8,0
	add r9,rax
	adc r10,rdx
	adc r8,0

	mov rax,r12
	mul rax
	add r9,rax
	adc r10,rdx
	adc r8,0
	
	
	mov rbp,r9
	xor r9,r9
	
	;; c+=2*a.n[0]*a.n[3]+2*a.n[1]*a.n[2]
	mov rax,r11		; load a.n[0]
	mul r14			; * a.n[3]
	add rax,rax		; rdx:rax*=2
	adc rdx,rdx
	adc r9,0
	add r10,rax
	adc r8,rdx
	adc r9,0

	mov rax,r12		; load a.n[1]
	mul r13			; * a.n[2]
	add rax,rax
	adc rdx,rdx
	adc r9,0
	add r10,rax
	adc r8,rdx
	adc r9,0
		
	mov r11,r10
	xor r10,r10

	;; c+=2*a.n[1]*a.n[3]+a.n[2]*a.n[2]
	mov rax,r12		; load a.n[1]
	mul r14			; * a.n[3]
	add rax,rax		; rdx:rax*=2
	adc rdx,rdx
	adc r10,0
	add r8,rax
	adc r9,rdx
	adc r10,0

	mov rax,r13
	mul rax
	add r8,rax
	adc r9,rdx
	adc r10,0

	mov r12,r8
	xor r8,r8
	;; c+=2*a.n[2]*a.n[3]
	mov rax,r13		; load a.n[2]
	mul r14			; * a.n[3]
	add rax,rax		; rdx:rax*=2
	adc rdx,rdx
	adc r8,0
	add r9,rax
	adc r10,rdx
	adc r8,0

	mov r13,r9
	xor r9,r9

	;; c+=a.n[3]²
	mov rax,r14
	mul rax
	add r10,rax
	adc r8,rdx
	
	mov r14,r10
	mov r15,r8
	
	jmp common_exit_norm
	end

	
