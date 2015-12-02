include ksamd64.inc
EXTERNDEF ?Te@rdtable@CryptoPP@@3PA_KA:FAR
EXTERNDEF ?g_cacheLineSize@CryptoPP@@3IA:FAR
EXTERNDEF ?SHA256_K@CryptoPP@@3QBIB:FAR
.CODE

    ALIGN   8
Baseline_Add	PROC
	lea		rdx, [rdx+8*rcx]
	lea		r8, [r8+8*rcx]
	lea		r9, [r9+8*rcx]
	neg		rcx					; rcx is negative index
	jz		$1@Baseline_Add
	mov		rax,[r8+8*rcx]
	add		rax,[r9+8*rcx]
	mov		[rdx+8*rcx],rax
$0@Baseline_Add:
	mov		rax,[r8+8*rcx+8]
	adc		rax,[r9+8*rcx+8]
	mov		[rdx+8*rcx+8],rax
	lea		rcx,[rcx+2]			; advance index, avoid inc which causes slowdown on Intel Core 2
	jrcxz	$1@Baseline_Add		; loop until rcx overflows and becomes zero
	mov		rax,[r8+8*rcx]
	adc		rax,[r9+8*rcx]
	mov		[rdx+8*rcx],rax
	jmp		$0@Baseline_Add
$1@Baseline_Add:
	mov		rax, 0
	adc		rax, rax			; store carry into rax (return result register)
	ret
Baseline_Add ENDP

    ALIGN   8
Baseline_Sub	PROC
	lea		rdx, [rdx+8*rcx]
	lea		r8, [r8+8*rcx]
	lea		r9, [r9+8*rcx]
	neg		rcx					; rcx is negative index
	jz		$1@Baseline_Sub
	mov		rax,[r8+8*rcx]
	sub		rax,[r9+8*rcx]
	mov		[rdx+8*rcx],rax
$0@Baseline_Sub:
	mov		rax,[r8+8*rcx+8]
	sbb		rax,[r9+8*rcx+8]
	mov		[rdx+8*rcx+8],rax
	lea		rcx,[rcx+2]			; advance index, avoid inc which causes slowdown on Intel Core 2
	jrcxz	$1@Baseline_Sub		; loop until rcx overflows and becomes zero
	mov		rax,[r8+8*rcx]
	sbb		rax,[r9+8*rcx]
	mov		[rdx+8*rcx],rax
	jmp		$0@Baseline_Sub
$1@Baseline_Sub:
	mov		rax, 0
	adc		rax, rax			; store carry into rax (return result register)

	ret
Baseline_Sub ENDP

ALIGN   8
Rijndael_Enc_AdvancedProcessBlocks	PROC FRAME
rex_push_reg rsi
push_reg rdi
push_reg rbx
push_reg r12
.endprolog
mov r8, rcx
mov r11, ?Te@rdtable@CryptoPP@@3PA_KA
mov edi, DWORD PTR [?g_cacheLineSize@CryptoPP@@3IA]
mov rsi, [(r8+16*19)]
mov rax, 16
and rax, rsi
movdqa xmm3, XMMWORD PTR [rdx+16+rax]
movdqa [(r8+16*12)], xmm3
lea rax, [rdx+rax+2*16]
sub rax, rsi
label0:
movdqa xmm0, [rax+rsi]
movdqa XMMWORD PTR [(r8+0)+rsi], xmm0
add rsi, 16
cmp rsi, 16*12
jl label0
movdqa xmm4, [rax+rsi]
movdqa xmm1, [rdx]
mov r12d, [rdx+4*4]
mov ebx, [rdx+5*4]
mov ecx, [rdx+6*4]
mov edx, [rdx+7*4]
xor rax, rax
label9:
mov esi, [r11+rax]
add rax, rdi
mov esi, [r11+rax]
add rax, rdi
mov esi, [r11+rax]
add rax, rdi
mov esi, [r11+rax]
add rax, rdi
cmp rax, 2048
jl label9
lfence
test DWORD PTR [(r8+16*18+8)], 1
jz label8
mov rsi, [(r8+16*14)]
movdqu xmm2, [rsi]
pxor xmm2, xmm1
psrldq xmm1, 14
movd eax, xmm1
mov al, BYTE PTR [rsi+15]
mov r10d, eax
movd eax, xmm2
psrldq xmm2, 4
movd edi, xmm2
psrldq xmm2, 4
movzx esi, al
xor r12d, DWORD PTR [r11+8*rsi+(((0+3) MOD (4))+1)]
movzx esi, ah
xor edx, DWORD PTR [r11+8*rsi+(((1+3) MOD (4))+1)]
shr eax, 16
movzx esi, al
xor ecx, DWORD PTR [r11+8*rsi+(((2+3) MOD (4))+1)]
movzx esi, ah
xor ebx, DWORD PTR [r11+8*rsi+(((3+3) MOD (4))+1)]
mov eax, edi
movd edi, xmm2
psrldq xmm2, 4
movzx esi, al
xor ebx, DWORD PTR [r11+8*rsi+(((0+3) MOD (4))+1)]
movzx esi, ah
xor r12d, DWORD PTR [r11+8*rsi+(((1+3) MOD (4))+1)]
shr eax, 16
movzx esi, al
xor edx, DWORD PTR [r11+8*rsi+(((2+3) MOD (4))+1)]
movzx esi, ah
xor ecx, DWORD PTR [r11+8*rsi+(((3+3) MOD (4))+1)]
mov eax, edi
movd edi, xmm2
movzx esi, al
xor ecx, DWORD PTR [r11+8*rsi+(((0+3) MOD (4))+1)]
movzx esi, ah
xor ebx, DWORD PTR [r11+8*rsi+(((1+3) MOD (4))+1)]
shr eax, 16
movzx esi, al
xor r12d, DWORD PTR [r11+8*rsi+(((2+3) MOD (4))+1)]
movzx esi, ah
xor edx, DWORD PTR [r11+8*rsi+(((3+3) MOD (4))+1)]
mov eax, edi
movzx esi, al
xor edx, DWORD PTR [r11+8*rsi+(((0+3) MOD (4))+1)]
movzx esi, ah
xor ecx, DWORD PTR [r11+8*rsi+(((1+3) MOD (4))+1)]
shr eax, 16
movzx esi, al
xor ebx, DWORD PTR [r11+8*rsi+(((2+3) MOD (4))+1)]
psrldq xmm2, 3
mov eax, [(r8+16*12)+0*4]
mov edi, [(r8+16*12)+2*4]
mov r9d, [(r8+16*12)+3*4]
movzx esi, cl
xor r9d, DWORD PTR [r11+8*rsi+(((3+3) MOD (4))+1)]
movzx esi, bl
xor edi, DWORD PTR [r11+8*rsi+(((3+3) MOD (4))+1)]
movzx esi, bh
xor r9d, DWORD PTR [r11+8*rsi+(((2+3) MOD (4))+1)]
shr ebx, 16
movzx esi, bl
xor eax, DWORD PTR [r11+8*rsi+(((1+3) MOD (4))+1)]
movzx esi, bh
mov ebx, DWORD PTR [r11+8*rsi+(((0+3) MOD (4))+1)]
xor ebx, [(r8+16*12)+1*4]
movzx esi, ch
xor eax, DWORD PTR [r11+8*rsi+(((2+3) MOD (4))+1)]
shr ecx, 16
movzx esi, dl
xor eax, DWORD PTR [r11+8*rsi+(((3+3) MOD (4))+1)]
movzx esi, dh
xor ebx, DWORD PTR [r11+8*rsi+(((2+3) MOD (4))+1)]
shr edx, 16
movzx esi, ch
xor edi, DWORD PTR [r11+8*rsi+(((0+3) MOD (4))+1)]
movzx esi, cl
xor ebx, DWORD PTR [r11+8*rsi+(((1+3) MOD (4))+1)]
movzx esi, dl
xor edi, DWORD PTR [r11+8*rsi+(((1+3) MOD (4))+1)]
movzx esi, dh
xor r9d, DWORD PTR [r11+8*rsi+(((0+3) MOD (4))+1)]
movd ecx, xmm2
mov edx, r12d
mov [(r8+0)+3*4], r9d
mov [(r8+0)+0*4], eax
mov [(r8+0)+1*4], ebx
mov [(r8+0)+2*4], edi
jmp label5
label3:
mov r12d, [(r8+16*12)+0*4]
mov ebx, [(r8+16*12)+1*4]
mov ecx, [(r8+16*12)+2*4]
mov edx, [(r8+16*12)+3*4]
label8:
mov rax, [(r8+16*14)]
movdqu xmm2, [rax]
mov rsi, [(r8+16*14)+8]
movdqu xmm5, [rsi]
pxor xmm2, xmm1
pxor xmm2, xmm5
movd eax, xmm2
psrldq xmm2, 4
movd edi, xmm2
psrldq xmm2, 4
movzx esi, al
xor r12d, DWORD PTR [r11+8*rsi+(((0+3) MOD (4))+1)]
movzx esi, ah
xor edx, DWORD PTR [r11+8*rsi+(((1+3) MOD (4))+1)]
shr eax, 16
movzx esi, al
xor ecx, DWORD PTR [r11+8*rsi+(((2+3) MOD (4))+1)]
movzx esi, ah
xor ebx, DWORD PTR [r11+8*rsi+(((3+3) MOD (4))+1)]
mov eax, edi
movd edi, xmm2
psrldq xmm2, 4
movzx esi, al
xor ebx, DWORD PTR [r11+8*rsi+(((0+3) MOD (4))+1)]
movzx esi, ah
xor r12d, DWORD PTR [r11+8*rsi+(((1+3) MOD (4))+1)]
shr eax, 16
movzx esi, al
xor edx, DWORD PTR [r11+8*rsi+(((2+3) MOD (4))+1)]
movzx esi, ah
xor ecx, DWORD PTR [r11+8*rsi+(((3+3) MOD (4))+1)]
mov eax, edi
movd edi, xmm2
movzx esi, al
xor ecx, DWORD PTR [r11+8*rsi+(((0+3) MOD (4))+1)]
movzx esi, ah
xor ebx, DWORD PTR [r11+8*rsi+(((1+3) MOD (4))+1)]
shr eax, 16
movzx esi, al
xor r12d, DWORD PTR [r11+8*rsi+(((2+3) MOD (4))+1)]
movzx esi, ah
xor edx, DWORD PTR [r11+8*rsi+(((3+3) MOD (4))+1)]
mov eax, edi
movzx esi, al
xor edx, DWORD PTR [r11+8*rsi+(((0+3) MOD (4))+1)]
movzx esi, ah
xor ecx, DWORD PTR [r11+8*rsi+(((1+3) MOD (4))+1)]
shr eax, 16
movzx esi, al
xor ebx, DWORD PTR [r11+8*rsi+(((2+3) MOD (4))+1)]
movzx esi, ah
xor r12d, DWORD PTR [r11+8*rsi+(((3+3) MOD (4))+1)]
mov eax, r12d
add r8, [(r8+16*19)]
add r8, 4*16
jmp label2
label1:
mov ecx, r10d
mov edx, r12d
mov eax, [(r8+0)+0*4]
mov ebx, [(r8+0)+1*4]
xor cl, ch
and rcx, 255
label5:
add r10d, 1
xor edx, DWORD PTR [r11+rcx*8+3]
movzx esi, dl
xor ebx, DWORD PTR [r11+8*rsi+(((3+3) MOD (4))+1)]
movzx esi, dh
mov ecx, DWORD PTR [r11+8*rsi+(((2+3) MOD (4))+1)]
shr edx, 16
xor ecx, [(r8+0)+2*4]
movzx esi, dh
xor eax, DWORD PTR [r11+8*rsi+(((0+3) MOD (4))+1)]
movzx esi, dl
mov edx, DWORD PTR [r11+8*rsi+(((1+3) MOD (4))+1)]
xor edx, [(r8+0)+3*4]
add r8, [(r8+16*19)]
add r8, 3*16
jmp label4
label2:
mov r9d, [(r8+0)-4*16+3*4]
mov edi, [(r8+0)-4*16+2*4]
movzx esi, cl
xor r9d, DWORD PTR [r11+8*rsi+(((3+3) MOD (4))+1)]
mov cl, al
movzx esi, ah
xor edi, DWORD PTR [r11+8*rsi+(((2+3) MOD (4))+1)]
shr eax, 16
movzx esi, bl
xor edi, DWORD PTR [r11+8*rsi+(((3+3) MOD (4))+1)]
movzx esi, bh
xor r9d, DWORD PTR [r11+8*rsi+(((2+3) MOD (4))+1)]
shr ebx, 16
movzx esi, al
xor r9d, DWORD PTR [r11+8*rsi+(((1+3) MOD (4))+1)]
movzx esi, ah
mov eax, DWORD PTR [r11+8*rsi+(((0+3) MOD (4))+1)]
movzx esi, bl
xor eax, DWORD PTR [r11+8*rsi+(((1+3) MOD (4))+1)]
movzx esi, bh
mov ebx, DWORD PTR [r11+8*rsi+(((0+3) MOD (4))+1)]
movzx esi, ch
xor eax, DWORD PTR [r11+8*rsi+(((2+3) MOD (4))+1)]
movzx esi, cl
xor ebx, DWORD PTR [r11+8*rsi+(((3+3) MOD (4))+1)]
shr ecx, 16
movzx esi, dl
xor eax, DWORD PTR [r11+8*rsi+(((3+3) MOD (4))+1)]
movzx esi, dh
xor ebx, DWORD PTR [r11+8*rsi+(((2+3) MOD (4))+1)]
shr edx, 16
movzx esi, ch
xor edi, DWORD PTR [r11+8*rsi+(((0+3) MOD (4))+1)]
movzx esi, cl
xor ebx, DWORD PTR [r11+8*rsi+(((1+3) MOD (4))+1)]
movzx esi, dl
xor edi, DWORD PTR [r11+8*rsi+(((1+3) MOD (4))+1)]
movzx esi, dh
xor r9d, DWORD PTR [r11+8*rsi+(((0+3) MOD (4))+1)]
mov ecx, edi
xor eax, [(r8+0)-4*16+0*4]
xor ebx, [(r8+0)-4*16+1*4]
mov edx, r9d
label4:
mov r9d, [(r8+0)-4*16+7*4]
mov edi, [(r8+0)-4*16+6*4]
movzx esi, cl
xor r9d, DWORD PTR [r11+8*rsi+(((3+3) MOD (4))+1)]
mov cl, al
movzx esi, ah
xor edi, DWORD PTR [r11+8*rsi+(((2+3) MOD (4))+1)]
shr eax, 16
movzx esi, bl
xor edi, DWORD PTR [r11+8*rsi+(((3+3) MOD (4))+1)]
movzx esi, bh
xor r9d, DWORD PTR [r11+8*rsi+(((2+3) MOD (4))+1)]
shr ebx, 16
movzx esi, al
xor r9d, DWORD PTR [r11+8*rsi+(((1+3) MOD (4))+1)]
movzx esi, ah
mov eax, DWORD PTR [r11+8*rsi+(((0+3) MOD (4))+1)]
movzx esi, bl
xor eax, DWORD PTR [r11+8*rsi+(((1+3) MOD (4))+1)]
movzx esi, bh
mov ebx, DWORD PTR [r11+8*rsi+(((0+3) MOD (4))+1)]
movzx esi, ch
xor eax, DWORD PTR [r11+8*rsi+(((2+3) MOD (4))+1)]
movzx esi, cl
xor ebx, DWORD PTR [r11+8*rsi+(((3+3) MOD (4))+1)]
shr ecx, 16
movzx esi, dl
xor eax, DWORD PTR [r11+8*rsi+(((3+3) MOD (4))+1)]
movzx esi, dh
xor ebx, DWORD PTR [r11+8*rsi+(((2+3) MOD (4))+1)]
shr edx, 16
movzx esi, ch
xor edi, DWORD PTR [r11+8*rsi+(((0+3) MOD (4))+1)]
movzx esi, cl
xor ebx, DWORD PTR [r11+8*rsi+(((1+3) MOD (4))+1)]
movzx esi, dl
xor edi, DWORD PTR [r11+8*rsi+(((1+3) MOD (4))+1)]
movzx esi, dh
xor r9d, DWORD PTR [r11+8*rsi+(((0+3) MOD (4))+1)]
mov ecx, edi
xor eax, [(r8+0)-4*16+4*4]
xor ebx, [(r8+0)-4*16+5*4]
mov edx, r9d
add r8, 32
test r8, 255
jnz label2
sub r8, 16*16
movzx esi, ch
movzx edi, BYTE PTR [r11+rsi*8+1]
movzx esi, dl
xor edi, DWORD PTR [r11+rsi*8+0]
mov WORD PTR [(r8+16*13)+2], di
movzx esi, dh
movzx edi, BYTE PTR [r11+rsi*8+1]
movzx esi, al
xor edi, DWORD PTR [r11+rsi*8+0]
mov WORD PTR [(r8+16*13)+6], di
shr edx, 16
movzx esi, ah
movzx edi, BYTE PTR [r11+rsi*8+1]
movzx esi, bl
xor edi, DWORD PTR [r11+rsi*8+0]
mov WORD PTR [(r8+16*13)+10], di
shr eax, 16
movzx esi, bh
movzx edi, BYTE PTR [r11+rsi*8+1]
movzx esi, cl
xor edi, DWORD PTR [r11+rsi*8+0]
mov WORD PTR [(r8+16*13)+14], di
shr ebx, 16
movzx esi, dh
movzx edi, BYTE PTR [r11+rsi*8+1]
movzx esi, al
xor edi, DWORD PTR [r11+rsi*8+0]
mov WORD PTR [(r8+16*13)+12], di
shr ecx, 16
movzx esi, ah
movzx edi, BYTE PTR [r11+rsi*8+1]
movzx esi, bl
xor edi, DWORD PTR [r11+rsi*8+0]
mov WORD PTR [(r8+16*13)+0], di
movzx esi, bh
movzx edi, BYTE PTR [r11+rsi*8+1]
movzx esi, cl
xor edi, DWORD PTR [r11+rsi*8+0]
mov WORD PTR [(r8+16*13)+4], di
movzx esi, ch
movzx edi, BYTE PTR [r11+rsi*8+1]
movzx esi, dl
xor edi, DWORD PTR [r11+rsi*8+0]
mov WORD PTR [(r8+16*13)+8], di
mov rax, [(r8+16*14)+16]
mov rbx, [(r8+16*14)+24]
mov rcx, [(r8+16*18+8)]
sub rcx, 16
movdqu xmm2, [rax]
pxor xmm2, xmm4
movdqa xmm0, [(r8+16*16)+16]
paddq xmm0, [(r8+16*14)+16]
movdqa [(r8+16*14)+16], xmm0
pxor xmm2, [(r8+16*13)]
movdqu [rbx], xmm2
jle label7
mov [(r8+16*18+8)], rcx
test rcx, 1
jnz label1
movdqa xmm0, [(r8+16*16)]
paddq xmm0, [(r8+16*14)]
movdqa [(r8+16*14)], xmm0
jmp label3
label7:
xorps xmm0, xmm0
lea rax, [(r8+0)+7*16]
movaps [rax-7*16], xmm0
movaps [rax-6*16], xmm0
movaps [rax-5*16], xmm0
movaps [rax-4*16], xmm0
movaps [rax-3*16], xmm0
movaps [rax-2*16], xmm0
movaps [rax-1*16], xmm0
movaps [rax+0*16], xmm0
movaps [rax+1*16], xmm0
movaps [rax+2*16], xmm0
movaps [rax+3*16], xmm0
movaps [rax+4*16], xmm0
movaps [rax+5*16], xmm0
movaps [rax+6*16], xmm0
pop r12
pop rbx
pop rdi
pop rsi
ret
Rijndael_Enc_AdvancedProcessBlocks ENDP

ALIGN   8
GCM_AuthenticateBlocks_2K	PROC FRAME
rex_push_reg rsi
push_reg rdi
push_reg rbx
.endprolog
mov rsi, r8
mov r11, r9
movdqa xmm0, [rsi]
label0:
movdqu xmm4, [rcx]
pxor xmm0, xmm4
movd ebx, xmm0
mov eax, 0f0f0f0f0h
and eax, ebx
shl ebx, 4
and ebx, 0f0f0f0f0h
movzx edi, ah
movdqa xmm5, XMMWORD PTR [rsi + 32 + 1024 + rdi]
movzx edi, al
movdqa xmm4, XMMWORD PTR [rsi + 32 + 1024 + rdi]
shr eax, 16
movzx edi, ah
movdqa xmm3, XMMWORD PTR [rsi + 32 + 1024 + rdi]
movzx edi, al
movdqa xmm2, XMMWORD PTR [rsi + 32 + 1024 + rdi]
psrldq xmm0, 4
movd eax, xmm0
and eax, 0f0f0f0f0h
movzx edi, bh
pxor xmm5, XMMWORD PTR [rsi + 32 + (1-1)*256 + rdi]
movzx edi, bl
pxor xmm4, XMMWORD PTR [rsi + 32 + (1-1)*256 + rdi]
shr ebx, 16
movzx edi, bh
pxor xmm3, XMMWORD PTR [rsi + 32 + (1-1)*256 + rdi]
movzx edi, bl
pxor xmm2, XMMWORD PTR [rsi + 32 + (1-1)*256 + rdi]
movd ebx, xmm0
shl ebx, 4
and ebx, 0f0f0f0f0h
movzx edi, ah
pxor xmm5, XMMWORD PTR [rsi + 32 + 1024 + 1*256 + rdi]
movzx edi, al
pxor xmm4, XMMWORD PTR [rsi + 32 + 1024 + 1*256 + rdi]
shr eax, 16
movzx edi, ah
pxor xmm3, XMMWORD PTR [rsi + 32 + 1024 + 1*256 + rdi]
movzx edi, al
pxor xmm2, XMMWORD PTR [rsi + 32 + 1024 + 1*256 + rdi]
psrldq xmm0, 4
movd eax, xmm0
and eax, 0f0f0f0f0h
movzx edi, bh
pxor xmm5, XMMWORD PTR [rsi + 32 + (2-1)*256 + rdi]
movzx edi, bl
pxor xmm4, XMMWORD PTR [rsi + 32 + (2-1)*256 + rdi]
shr ebx, 16
movzx edi, bh
pxor xmm3, XMMWORD PTR [rsi + 32 + (2-1)*256 + rdi]
movzx edi, bl
pxor xmm2, XMMWORD PTR [rsi + 32 + (2-1)*256 + rdi]
movd ebx, xmm0
shl ebx, 4
and ebx, 0f0f0f0f0h
movzx edi, ah
pxor xmm5, XMMWORD PTR [rsi + 32 + 1024 + 2*256 + rdi]
movzx edi, al
pxor xmm4, XMMWORD PTR [rsi + 32 + 1024 + 2*256 + rdi]
shr eax, 16
movzx edi, ah
pxor xmm3, XMMWORD PTR [rsi + 32 + 1024 + 2*256 + rdi]
movzx edi, al
pxor xmm2, XMMWORD PTR [rsi + 32 + 1024 + 2*256 + rdi]
psrldq xmm0, 4
movd eax, xmm0
and eax, 0f0f0f0f0h
movzx edi, bh
pxor xmm5, XMMWORD PTR [rsi + 32 + (3-1)*256 + rdi]
movzx edi, bl
pxor xmm4, XMMWORD PTR [rsi + 32 + (3-1)*256 + rdi]
shr ebx, 16
movzx edi, bh
pxor xmm3, XMMWORD PTR [rsi + 32 + (3-1)*256 + rdi]
movzx edi, bl
pxor xmm2, XMMWORD PTR [rsi + 32 + (3-1)*256 + rdi]
movd ebx, xmm0
shl ebx, 4
and ebx, 0f0f0f0f0h
movzx edi, ah
pxor xmm5, XMMWORD PTR [rsi + 32 + 1024 + 3*256 + rdi]
movzx edi, al
pxor xmm4, XMMWORD PTR [rsi + 32 + 1024 + 3*256 + rdi]
shr eax, 16
movzx edi, ah
pxor xmm3, XMMWORD PTR [rsi + 32 + 1024 + 3*256 + rdi]
movzx edi, al
pxor xmm2, XMMWORD PTR [rsi + 32 + 1024 + 3*256 + rdi]
movzx edi, bh
pxor xmm5, XMMWORD PTR [rsi + 32 + 3*256 + rdi]
movzx edi, bl
pxor xmm4, XMMWORD PTR [rsi + 32 + 3*256 + rdi]
shr ebx, 16
movzx edi, bh
pxor xmm3, XMMWORD PTR [rsi + 32 + 3*256 + rdi]
movzx edi, bl
pxor xmm2, XMMWORD PTR [rsi + 32 + 3*256 + rdi]
movdqa xmm0, xmm3
pslldq xmm3, 1
pxor xmm2, xmm3
movdqa xmm1, xmm2
pslldq xmm2, 1
pxor xmm5, xmm2
psrldq xmm0, 15
movd rdi, xmm0
movzx eax, WORD PTR [r11 + rdi*2]
shl eax, 8
movdqa xmm0, xmm5
pslldq xmm5, 1
pxor xmm4, xmm5
psrldq xmm1, 15
movd rdi, xmm1
xor ax, WORD PTR [r11 + rdi*2]
shl eax, 8
psrldq xmm0, 15
movd rdi, xmm0
xor ax, WORD PTR [r11 + rdi*2]
movd xmm0, eax
pxor xmm0, xmm4
add rcx, 16
sub rdx, 1
jnz label0
movdqa [rsi], xmm0
pop rbx
pop rdi
pop rsi
ret
GCM_AuthenticateBlocks_2K ENDP

ALIGN   8
GCM_AuthenticateBlocks_64K	PROC FRAME
rex_push_reg rsi
push_reg rdi
.endprolog
mov rsi, r8
movdqa xmm0, [rsi]
label1:
movdqu xmm1, [rcx]
pxor xmm1, xmm0
pxor xmm0, xmm0
movd eax, xmm1
psrldq xmm1, 4
movzx edi, al
add rdi, rdi
pxor xmm0, [rsi + 32 + (0*4+0)*256*16 + rdi*8]
movzx edi, ah
add rdi, rdi
pxor xmm0, [rsi + 32 + (0*4+1)*256*16 + rdi*8]
shr eax, 16
movzx edi, al
add rdi, rdi
pxor xmm0, [rsi + 32 + (0*4+2)*256*16 + rdi*8]
movzx edi, ah
add rdi, rdi
pxor xmm0, [rsi + 32 + (0*4+3)*256*16 + rdi*8]
movd eax, xmm1
psrldq xmm1, 4
movzx edi, al
add rdi, rdi
pxor xmm0, [rsi + 32 + (1*4+0)*256*16 + rdi*8]
movzx edi, ah
add rdi, rdi
pxor xmm0, [rsi + 32 + (1*4+1)*256*16 + rdi*8]
shr eax, 16
movzx edi, al
add rdi, rdi
pxor xmm0, [rsi + 32 + (1*4+2)*256*16 + rdi*8]
movzx edi, ah
add rdi, rdi
pxor xmm0, [rsi + 32 + (1*4+3)*256*16 + rdi*8]
movd eax, xmm1
psrldq xmm1, 4
movzx edi, al
add rdi, rdi
pxor xmm0, [rsi + 32 + (2*4+0)*256*16 + rdi*8]
movzx edi, ah
add rdi, rdi
pxor xmm0, [rsi + 32 + (2*4+1)*256*16 + rdi*8]
shr eax, 16
movzx edi, al
add rdi, rdi
pxor xmm0, [rsi + 32 + (2*4+2)*256*16 + rdi*8]
movzx edi, ah
add rdi, rdi
pxor xmm0, [rsi + 32 + (2*4+3)*256*16 + rdi*8]
movd eax, xmm1
psrldq xmm1, 4
movzx edi, al
add rdi, rdi
pxor xmm0, [rsi + 32 + (3*4+0)*256*16 + rdi*8]
movzx edi, ah
add rdi, rdi
pxor xmm0, [rsi + 32 + (3*4+1)*256*16 + rdi*8]
shr eax, 16
movzx edi, al
add rdi, rdi
pxor xmm0, [rsi + 32 + (3*4+2)*256*16 + rdi*8]
movzx edi, ah
add rdi, rdi
pxor xmm0, [rsi + 32 + (3*4+3)*256*16 + rdi*8]
add rcx, 16
sub rdx, 1
jnz label1
movdqa [rsi], xmm0
pop rdi
pop rsi
ret
GCM_AuthenticateBlocks_64K ENDP

ALIGN   8
X86_SHA256_HashBlocks	PROC FRAME
rex_push_reg rsi
push_reg rdi
push_reg rbx
push_reg rbp
alloc_stack(8*4 + 16*4 + 4*8 + 8)
.endprolog
mov rdi, r8
lea rsi, [?SHA256_K@CryptoPP@@3QBIB + 48*4]
mov [rsp+8*4+16*4+1*8], rcx
mov [rsp+8*4+16*4+2*8], rdx
add rdi, rdx
mov [rsp+8*4+16*4+3*8], rdi
movdqa xmm0, XMMWORD PTR [rcx+0*16]
movdqa xmm1, XMMWORD PTR [rcx+1*16]
mov [rsp+8*4+16*4+0*8], rsi
label0:
sub rsi, 48*4
movdqa [rsp+((1024+7-(0+3)) MOD (8))*4], xmm1
movdqa [rsp+((1024+7-(0+7)) MOD (8))*4], xmm0
mov rbx, [rdx+0*8]
bswap rbx
mov [rsp+8*4+((1024+15-(0*(1+1)+1)) MOD (16))*4], rbx
mov rbx, [rdx+1*8]
bswap rbx
mov [rsp+8*4+((1024+15-(1*(1+1)+1)) MOD (16))*4], rbx
mov rbx, [rdx+2*8]
bswap rbx
mov [rsp+8*4+((1024+15-(2*(1+1)+1)) MOD (16))*4], rbx
mov rbx, [rdx+3*8]
bswap rbx
mov [rsp+8*4+((1024+15-(3*(1+1)+1)) MOD (16))*4], rbx
mov rbx, [rdx+4*8]
bswap rbx
mov [rsp+8*4+((1024+15-(4*(1+1)+1)) MOD (16))*4], rbx
mov rbx, [rdx+5*8]
bswap rbx
mov [rsp+8*4+((1024+15-(5*(1+1)+1)) MOD (16))*4], rbx
mov rbx, [rdx+6*8]
bswap rbx
mov [rsp+8*4+((1024+15-(6*(1+1)+1)) MOD (16))*4], rbx
mov rbx, [rdx+7*8]
bswap rbx
mov [rsp+8*4+((1024+15-(7*(1+1)+1)) MOD (16))*4], rbx
mov edi, [rsp+((1024+7-(0+3)) MOD (8))*4]
mov eax, [rsp+((1024+7-(0+6)) MOD (8))*4]
xor eax, [rsp+((1024+7-(0+5)) MOD (8))*4]
mov ecx, [rsp+((1024+7-(0+7)) MOD (8))*4]
mov edx, [rsp+((1024+7-(0+2)) MOD (8))*4]
xor edx, [rsp+((1024+7-(0+1)) MOD (8))*4]
and edx, edi
xor edx, [rsp+((1024+7-(0+1)) MOD (8))*4]
mov ebp, edi
ror edi, 6
ror ebp, 25
add edx, [rsi+(0)*4]
add edx, [rsp+8*4+((1024+15-(0)) MOD (16))*4]
add edx, [rsp+((1024+7-(0)) MOD (8))*4]
xor ebp, edi
ror edi, 5
xor ebp, edi
add edx, ebp
mov ebx, ecx
xor ecx, [rsp+((1024+7-(0+6)) MOD (8))*4]
and eax, ecx
xor eax, [rsp+((1024+7-(0+6)) MOD (8))*4]
mov ebp, ebx
ror ebx, 2
add eax, edx
add edx, [rsp+((1024+7-(0+4)) MOD (8))*4]
mov [rsp+((1024+7-(0+4)) MOD (8))*4], edx
ror ebp, 22
xor ebp, ebx
ror ebx, 11
xor ebp, ebx
add eax, ebp
mov [rsp+((1024+7-(0)) MOD (8))*4], eax
mov edi, [rsp+((1024+7-(1+2)) MOD (8))*4]
xor edi, [rsp+((1024+7-(1+1)) MOD (8))*4]
and edi, edx
xor edi, [rsp+((1024+7-(1+1)) MOD (8))*4]
mov ebp, edx
ror edx, 6
ror ebp, 25
add edi, [rsi+(1)*4]
add edi, [rsp+8*4+((1024+15-(1)) MOD (16))*4]
add edi, [rsp+((1024+7-(1)) MOD (8))*4]
xor ebp, edx
ror edx, 5
xor ebp, edx
add edi, ebp
mov ebx, eax
xor eax, [rsp+((1024+7-(1+6)) MOD (8))*4]
and ecx, eax
xor ecx, [rsp+((1024+7-(1+6)) MOD (8))*4]
mov ebp, ebx
ror ebx, 2
add ecx, edi
add edi, [rsp+((1024+7-(1+4)) MOD (8))*4]
mov [rsp+((1024+7-(1+4)) MOD (8))*4], edi
ror ebp, 22
xor ebp, ebx
ror ebx, 11
xor ebp, ebx
add ecx, ebp
mov [rsp+((1024+7-(1)) MOD (8))*4], ecx
mov edx, [rsp+((1024+7-(2+2)) MOD (8))*4]
xor edx, [rsp+((1024+7-(2+1)) MOD (8))*4]
and edx, edi
xor edx, [rsp+((1024+7-(2+1)) MOD (8))*4]
mov ebp, edi
ror edi, 6
ror ebp, 25
add edx, [rsi+(2)*4]
add edx, [rsp+8*4+((1024+15-(2)) MOD (16))*4]
add edx, [rsp+((1024+7-(2)) MOD (8))*4]
xor ebp, edi
ror edi, 5
xor ebp, edi
add edx, ebp
mov ebx, ecx
xor ecx, [rsp+((1024+7-(2+6)) MOD (8))*4]
and eax, ecx
xor eax, [rsp+((1024+7-(2+6)) MOD (8))*4]
mov ebp, ebx
ror ebx, 2
add eax, edx
add edx, [rsp+((1024+7-(2+4)) MOD (8))*4]
mov [rsp+((1024+7-(2+4)) MOD (8))*4], edx
ror ebp, 22
xor ebp, ebx
ror ebx, 11
xor ebp, ebx
add eax, ebp
mov [rsp+((1024+7-(2)) MOD (8))*4], eax
mov edi, [rsp+((1024+7-(3+2)) MOD (8))*4]
xor edi, [rsp+((1024+7-(3+1)) MOD (8))*4]
and edi, edx
xor edi, [rsp+((1024+7-(3+1)) MOD (8))*4]
mov ebp, edx
ror edx, 6
ror ebp, 25
add edi, [rsi+(3)*4]
add edi, [rsp+8*4+((1024+15-(3)) MOD (16))*4]
add edi, [rsp+((1024+7-(3)) MOD (8))*4]
xor ebp, edx
ror edx, 5
xor ebp, edx
add edi, ebp
mov ebx, eax
xor eax, [rsp+((1024+7-(3+6)) MOD (8))*4]
and ecx, eax
xor ecx, [rsp+((1024+7-(3+6)) MOD (8))*4]
mov ebp, ebx
ror ebx, 2
add ecx, edi
add edi, [rsp+((1024+7-(3+4)) MOD (8))*4]
mov [rsp+((1024+7-(3+4)) MOD (8))*4], edi
ror ebp, 22
xor ebp, ebx
ror ebx, 11
xor ebp, ebx
add ecx, ebp
mov [rsp+((1024+7-(3)) MOD (8))*4], ecx
mov edx, [rsp+((1024+7-(4+2)) MOD (8))*4]
xor edx, [rsp+((1024+7-(4+1)) MOD (8))*4]
and edx, edi
xor edx, [rsp+((1024+7-(4+1)) MOD (8))*4]
mov ebp, edi
ror edi, 6
ror ebp, 25
add edx, [rsi+(4)*4]
add edx, [rsp+8*4+((1024+15-(4)) MOD (16))*4]
add edx, [rsp+((1024+7-(4)) MOD (8))*4]
xor ebp, edi
ror edi, 5
xor ebp, edi
add edx, ebp
mov ebx, ecx
xor ecx, [rsp+((1024+7-(4+6)) MOD (8))*4]
and eax, ecx
xor eax, [rsp+((1024+7-(4+6)) MOD (8))*4]
mov ebp, ebx
ror ebx, 2
add eax, edx
add edx, [rsp+((1024+7-(4+4)) MOD (8))*4]
mov [rsp+((1024+7-(4+4)) MOD (8))*4], edx
ror ebp, 22
xor ebp, ebx
ror ebx, 11
xor ebp, ebx
add eax, ebp
mov [rsp+((1024+7-(4)) MOD (8))*4], eax
mov edi, [rsp+((1024+7-(5+2)) MOD (8))*4]
xor edi, [rsp+((1024+7-(5+1)) MOD (8))*4]
and edi, edx
xor edi, [rsp+((1024+7-(5+1)) MOD (8))*4]
mov ebp, edx
ror edx, 6
ror ebp, 25
add edi, [rsi+(5)*4]
add edi, [rsp+8*4+((1024+15-(5)) MOD (16))*4]
add edi, [rsp+((1024+7-(5)) MOD (8))*4]
xor ebp, edx
ror edx, 5
xor ebp, edx
add edi, ebp
mov ebx, eax
xor eax, [rsp+((1024+7-(5+6)) MOD (8))*4]
and ecx, eax
xor ecx, [rsp+((1024+7-(5+6)) MOD (8))*4]
mov ebp, ebx
ror ebx, 2
add ecx, edi
add edi, [rsp+((1024+7-(5+4)) MOD (8))*4]
mov [rsp+((1024+7-(5+4)) MOD (8))*4], edi
ror ebp, 22
xor ebp, ebx
ror ebx, 11
xor ebp, ebx
add ecx, ebp
mov [rsp+((1024+7-(5)) MOD (8))*4], ecx
mov edx, [rsp+((1024+7-(6+2)) MOD (8))*4]
xor edx, [rsp+((1024+7-(6+1)) MOD (8))*4]
and edx, edi
xor edx, [rsp+((1024+7-(6+1)) MOD (8))*4]
mov ebp, edi
ror edi, 6
ror ebp, 25
add edx, [rsi+(6)*4]
add edx, [rsp+8*4+((1024+15-(6)) MOD (16))*4]
add edx, [rsp+((1024+7-(6)) MOD (8))*4]
xor ebp, edi
ror edi, 5
xor ebp, edi
add edx, ebp
mov ebx, ecx
xor ecx, [rsp+((1024+7-(6+6)) MOD (8))*4]
and eax, ecx
xor eax, [rsp+((1024+7-(6+6)) MOD (8))*4]
mov ebp, ebx
ror ebx, 2
add eax, edx
add edx, [rsp+((1024+7-(6+4)) MOD (8))*4]
mov [rsp+((1024+7-(6+4)) MOD (8))*4], edx
ror ebp, 22
xor ebp, ebx
ror ebx, 11
xor ebp, ebx
add eax, ebp
mov [rsp+((1024+7-(6)) MOD (8))*4], eax
mov edi, [rsp+((1024+7-(7+2)) MOD (8))*4]
xor edi, [rsp+((1024+7-(7+1)) MOD (8))*4]
and edi, edx
xor edi, [rsp+((1024+7-(7+1)) MOD (8))*4]
mov ebp, edx
ror edx, 6
ror ebp, 25
add edi, [rsi+(7)*4]
add edi, [rsp+8*4+((1024+15-(7)) MOD (16))*4]
add edi, [rsp+((1024+7-(7)) MOD (8))*4]
xor ebp, edx
ror edx, 5
xor ebp, edx
add edi, ebp
mov ebx, eax
xor eax, [rsp+((1024+7-(7+6)) MOD (8))*4]
and ecx, eax
xor ecx, [rsp+((1024+7-(7+6)) MOD (8))*4]
mov ebp, ebx
ror ebx, 2
add ecx, edi
add edi, [rsp+((1024+7-(7+4)) MOD (8))*4]
mov [rsp+((1024+7-(7+4)) MOD (8))*4], edi
ror ebp, 22
xor ebp, ebx
ror ebx, 11
xor ebp, ebx
add ecx, ebp
mov [rsp+((1024+7-(7)) MOD (8))*4], ecx
mov edx, [rsp+((1024+7-(8+2)) MOD (8))*4]
xor edx, [rsp+((1024+7-(8+1)) MOD (8))*4]
and edx, edi
xor edx, [rsp+((1024+7-(8+1)) MOD (8))*4]
mov ebp, edi
ror edi, 6
ror ebp, 25
add edx, [rsi+(8)*4]
add edx, [rsp+8*4+((1024+15-(8)) MOD (16))*4]
add edx, [rsp+((1024+7-(8)) MOD (8))*4]
xor ebp, edi
ror edi, 5
xor ebp, edi
add edx, ebp
mov ebx, ecx
xor ecx, [rsp+((1024+7-(8+6)) MOD (8))*4]
and eax, ecx
xor eax, [rsp+((1024+7-(8+6)) MOD (8))*4]
mov ebp, ebx
ror ebx, 2
add eax, edx
add edx, [rsp+((1024+7-(8+4)) MOD (8))*4]
mov [rsp+((1024+7-(8+4)) MOD (8))*4], edx
ror ebp, 22
xor ebp, ebx
ror ebx, 11
xor ebp, ebx
add eax, ebp
mov [rsp+((1024+7-(8)) MOD (8))*4], eax
mov edi, [rsp+((1024+7-(9+2)) MOD (8))*4]
xor edi, [rsp+((1024+7-(9+1)) MOD (8))*4]
and edi, edx
xor edi, [rsp+((1024+7-(9+1)) MOD (8))*4]
mov ebp, edx
ror edx, 6
ror ebp, 25
add edi, [rsi+(9)*4]
add edi, [rsp+8*4+((1024+15-(9)) MOD (16))*4]
add edi, [rsp+((1024+7-(9)) MOD (8))*4]
xor ebp, edx
ror edx, 5
xor ebp, edx
add edi, ebp
mov ebx, eax
xor eax, [rsp+((1024+7-(9+6)) MOD (8))*4]
and ecx, eax
xor ecx, [rsp+((1024+7-(9+6)) MOD (8))*4]
mov ebp, ebx
ror ebx, 2
add ecx, edi
add edi, [rsp+((1024+7-(9+4)) MOD (8))*4]
mov [rsp+((1024+7-(9+4)) MOD (8))*4], edi
ror ebp, 22
xor ebp, ebx
ror ebx, 11
xor ebp, ebx
add ecx, ebp
mov [rsp+((1024+7-(9)) MOD (8))*4], ecx
mov edx, [rsp+((1024+7-(10+2)) MOD (8))*4]
xor edx, [rsp+((1024+7-(10+1)) MOD (8))*4]
and edx, edi
xor edx, [rsp+((1024+7-(10+1)) MOD (8))*4]
mov ebp, edi
ror edi, 6
ror ebp, 25
add edx, [rsi+(10)*4]
add edx, [rsp+8*4+((1024+15-(10)) MOD (16))*4]
add edx, [rsp+((1024+7-(10)) MOD (8))*4]
xor ebp, edi
ror edi, 5
xor ebp, edi
add edx, ebp
mov ebx, ecx
xor ecx, [rsp+((1024+7-(10+6)) MOD (8))*4]
and eax, ecx
xor eax, [rsp+((1024+7-(10+6)) MOD (8))*4]
mov ebp, ebx
ror ebx, 2
add eax, edx
add edx, [rsp+((1024+7-(10+4)) MOD (8))*4]
mov [rsp+((1024+7-(10+4)) MOD (8))*4], edx
ror ebp, 22
xor ebp, ebx
ror ebx, 11
xor ebp, ebx
add eax, ebp
mov [rsp+((1024+7-(10)) MOD (8))*4], eax
mov edi, [rsp+((1024+7-(11+2)) MOD (8))*4]
xor edi, [rsp+((1024+7-(11+1)) MOD (8))*4]
and edi, edx
xor edi, [rsp+((1024+7-(11+1)) MOD (8))*4]
mov ebp, edx
ror edx, 6
ror ebp, 25
add edi, [rsi+(11)*4]
add edi, [rsp+8*4+((1024+15-(11)) MOD (16))*4]
add edi, [rsp+((1024+7-(11)) MOD (8))*4]
xor ebp, edx
ror edx, 5
xor ebp, edx
add edi, ebp
mov ebx, eax
xor eax, [rsp+((1024+7-(11+6)) MOD (8))*4]
and ecx, eax
xor ecx, [rsp+((1024+7-(11+6)) MOD (8))*4]
mov ebp, ebx
ror ebx, 2
add ecx, edi
add edi, [rsp+((1024+7-(11+4)) MOD (8))*4]
mov [rsp+((1024+7-(11+4)) MOD (8))*4], edi
ror ebp, 22
xor ebp, ebx
ror ebx, 11
xor ebp, ebx
add ecx, ebp
mov [rsp+((1024+7-(11)) MOD (8))*4], ecx
mov edx, [rsp+((1024+7-(12+2)) MOD (8))*4]
xor edx, [rsp+((1024+7-(12+1)) MOD (8))*4]
and edx, edi
xor edx, [rsp+((1024+7-(12+1)) MOD (8))*4]
mov ebp, edi
ror edi, 6
ror ebp, 25
add edx, [rsi+(12)*4]
add edx, [rsp+8*4+((1024+15-(12)) MOD (16))*4]
add edx, [rsp+((1024+7-(12)) MOD (8))*4]
xor ebp, edi
ror edi, 5
xor ebp, edi
add edx, ebp
mov ebx, ecx
xor ecx, [rsp+((1024+7-(12+6)) MOD (8))*4]
and eax, ecx
xor eax, [rsp+((1024+7-(12+6)) MOD (8))*4]
mov ebp, ebx
ror ebx, 2
add eax, edx
add edx, [rsp+((1024+7-(12+4)) MOD (8))*4]
mov [rsp+((1024+7-(12+4)) MOD (8))*4], edx
ror ebp, 22
xor ebp, ebx
ror ebx, 11
xor ebp, ebx
add eax, ebp
mov [rsp+((1024+7-(12)) MOD (8))*4], eax
mov edi, [rsp+((1024+7-(13+2)) MOD (8))*4]
xor edi, [rsp+((1024+7-(13+1)) MOD (8))*4]
and edi, edx
xor edi, [rsp+((1024+7-(13+1)) MOD (8))*4]
mov ebp, edx
ror edx, 6
ror ebp, 25
add edi, [rsi+(13)*4]
add edi, [rsp+8*4+((1024+15-(13)) MOD (16))*4]
add edi, [rsp+((1024+7-(13)) MOD (8))*4]
xor ebp, edx
ror edx, 5
xor ebp, edx
add edi, ebp
mov ebx, eax
xor eax, [rsp+((1024+7-(13+6)) MOD (8))*4]
and ecx, eax
xor ecx, [rsp+((1024+7-(13+6)) MOD (8))*4]
mov ebp, ebx
ror ebx, 2
add ecx, edi
add edi, [rsp+((1024+7-(13+4)) MOD (8))*4]
mov [rsp+((1024+7-(13+4)) MOD (8))*4], edi
ror ebp, 22
xor ebp, ebx
ror ebx, 11
xor ebp, ebx
add ecx, ebp
mov [rsp+((1024+7-(13)) MOD (8))*4], ecx
mov edx, [rsp+((1024+7-(14+2)) MOD (8))*4]
xor edx, [rsp+((1024+7-(14+1)) MOD (8))*4]
and edx, edi
xor edx, [rsp+((1024+7-(14+1)) MOD (8))*4]
mov ebp, edi
ror edi, 6
ror ebp, 25
add edx, [rsi+(14)*4]
add edx, [rsp+8*4+((1024+15-(14)) MOD (16))*4]
add edx, [rsp+((1024+7-(14)) MOD (8))*4]
xor ebp, edi
ror edi, 5
xor ebp, edi
add edx, ebp
mov ebx, ecx
xor ecx, [rsp+((1024+7-(14+6)) MOD (8))*4]
and eax, ecx
xor eax, [rsp+((1024+7-(14+6)) MOD (8))*4]
mov ebp, ebx
ror ebx, 2
add eax, edx
add edx, [rsp+((1024+7-(14+4)) MOD (8))*4]
mov [rsp+((1024+7-(14+4)) MOD (8))*4], edx
ror ebp, 22
xor ebp, ebx
ror ebx, 11
xor ebp, ebx
add eax, ebp
mov [rsp+((1024+7-(14)) MOD (8))*4], eax
mov edi, [rsp+((1024+7-(15+2)) MOD (8))*4]
xor edi, [rsp+((1024+7-(15+1)) MOD (8))*4]
and edi, edx
xor edi, [rsp+((1024+7-(15+1)) MOD (8))*4]
mov ebp, edx
ror edx, 6
ror ebp, 25
add edi, [rsi+(15)*4]
add edi, [rsp+8*4+((1024+15-(15)) MOD (16))*4]
add edi, [rsp+((1024+7-(15)) MOD (8))*4]
xor ebp, edx
ror edx, 5
xor ebp, edx
add edi, ebp
mov ebx, eax
xor eax, [rsp+((1024+7-(15+6)) MOD (8))*4]
and ecx, eax
xor ecx, [rsp+((1024+7-(15+6)) MOD (8))*4]
mov ebp, ebx
ror ebx, 2
add ecx, edi
add edi, [rsp+((1024+7-(15+4)) MOD (8))*4]
mov [rsp+((1024+7-(15+4)) MOD (8))*4], edi
ror ebp, 22
xor ebp, ebx
ror ebx, 11
xor ebp, ebx
add ecx, ebp
mov [rsp+((1024+7-(15)) MOD (8))*4], ecx
label1:
add rsi, 4*16
mov edx, [rsp+((1024+7-(0+2)) MOD (8))*4]
xor edx, [rsp+((1024+7-(0+1)) MOD (8))*4]
and edx, edi
xor edx, [rsp+((1024+7-(0+1)) MOD (8))*4]
mov ebp, edi
ror edi, 6
ror ebp, 25
xor ebp, edi
ror edi, 5
xor ebp, edi
add edx, ebp
mov ebp, [rsp+8*4+((1024+15-((0)-2)) MOD (16))*4]
mov edi, [rsp+8*4+((1024+15-((0)-15)) MOD (16))*4]
mov ebx, ebp
shr ebp, 10
ror ebx, 17
xor ebp, ebx
ror ebx, 2
xor ebx, ebp
add ebx, [rsp+8*4+((1024+15-((0)-7)) MOD (16))*4]
mov ebp, edi
shr ebp, 3
ror edi, 7
add ebx, [rsp+8*4+((1024+15-(0)) MOD (16))*4]
xor ebp, edi
add edx, [rsi+(0)*4]
ror edi, 11
add edx, [rsp+((1024+7-(0)) MOD (8))*4]
xor ebp, edi
add ebp, ebx
mov [rsp+8*4+((1024+15-(0)) MOD (16))*4], ebp
add edx, ebp
mov ebx, ecx
xor ecx, [rsp+((1024+7-(0+6)) MOD (8))*4]
and eax, ecx
xor eax, [rsp+((1024+7-(0+6)) MOD (8))*4]
mov ebp, ebx
ror ebx, 2
add eax, edx
add edx, [rsp+((1024+7-(0+4)) MOD (8))*4]
mov [rsp+((1024+7-(0+4)) MOD (8))*4], edx
ror ebp, 22
xor ebp, ebx
ror ebx, 11
xor ebp, ebx
add eax, ebp
mov [rsp+((1024+7-(0)) MOD (8))*4], eax
mov edi, [rsp+((1024+7-(1+2)) MOD (8))*4]
xor edi, [rsp+((1024+7-(1+1)) MOD (8))*4]
and edi, edx
xor edi, [rsp+((1024+7-(1+1)) MOD (8))*4]
mov ebp, edx
ror edx, 6
ror ebp, 25
xor ebp, edx
ror edx, 5
xor ebp, edx
add edi, ebp
mov ebp, [rsp+8*4+((1024+15-((1)-2)) MOD (16))*4]
mov edx, [rsp+8*4+((1024+15-((1)-15)) MOD (16))*4]
mov ebx, ebp
shr ebp, 10
ror ebx, 17
xor ebp, ebx
ror ebx, 2
xor ebx, ebp
add ebx, [rsp+8*4+((1024+15-((1)-7)) MOD (16))*4]
mov ebp, edx
shr ebp, 3
ror edx, 7
add ebx, [rsp+8*4+((1024+15-(1)) MOD (16))*4]
xor ebp, edx
add edi, [rsi+(1)*4]
ror edx, 11
add edi, [rsp+((1024+7-(1)) MOD (8))*4]
xor ebp, edx
add ebp, ebx
mov [rsp+8*4+((1024+15-(1)) MOD (16))*4], ebp
add edi, ebp
mov ebx, eax
xor eax, [rsp+((1024+7-(1+6)) MOD (8))*4]
and ecx, eax
xor ecx, [rsp+((1024+7-(1+6)) MOD (8))*4]
mov ebp, ebx
ror ebx, 2
add ecx, edi
add edi, [rsp+((1024+7-(1+4)) MOD (8))*4]
mov [rsp+((1024+7-(1+4)) MOD (8))*4], edi
ror ebp, 22
xor ebp, ebx
ror ebx, 11
xor ebp, ebx
add ecx, ebp
mov [rsp+((1024+7-(1)) MOD (8))*4], ecx
mov edx, [rsp+((1024+7-(2+2)) MOD (8))*4]
xor edx, [rsp+((1024+7-(2+1)) MOD (8))*4]
and edx, edi
xor edx, [rsp+((1024+7-(2+1)) MOD (8))*4]
mov ebp, edi
ror edi, 6
ror ebp, 25
xor ebp, edi
ror edi, 5
xor ebp, edi
add edx, ebp
mov ebp, [rsp+8*4+((1024+15-((2)-2)) MOD (16))*4]
mov edi, [rsp+8*4+((1024+15-((2)-15)) MOD (16))*4]
mov ebx, ebp
shr ebp, 10
ror ebx, 17
xor ebp, ebx
ror ebx, 2
xor ebx, ebp
add ebx, [rsp+8*4+((1024+15-((2)-7)) MOD (16))*4]
mov ebp, edi
shr ebp, 3
ror edi, 7
add ebx, [rsp+8*4+((1024+15-(2)) MOD (16))*4]
xor ebp, edi
add edx, [rsi+(2)*4]
ror edi, 11
add edx, [rsp+((1024+7-(2)) MOD (8))*4]
xor ebp, edi
add ebp, ebx
mov [rsp+8*4+((1024+15-(2)) MOD (16))*4], ebp
add edx, ebp
mov ebx, ecx
xor ecx, [rsp+((1024+7-(2+6)) MOD (8))*4]
and eax, ecx
xor eax, [rsp+((1024+7-(2+6)) MOD (8))*4]
mov ebp, ebx
ror ebx, 2
add eax, edx
add edx, [rsp+((1024+7-(2+4)) MOD (8))*4]
mov [rsp+((1024+7-(2+4)) MOD (8))*4], edx
ror ebp, 22
xor ebp, ebx
ror ebx, 11
xor ebp, ebx
add eax, ebp
mov [rsp+((1024+7-(2)) MOD (8))*4], eax
mov edi, [rsp+((1024+7-(3+2)) MOD (8))*4]
xor edi, [rsp+((1024+7-(3+1)) MOD (8))*4]
and edi, edx
xor edi, [rsp+((1024+7-(3+1)) MOD (8))*4]
mov ebp, edx
ror edx, 6
ror ebp, 25
xor ebp, edx
ror edx, 5
xor ebp, edx
add edi, ebp
mov ebp, [rsp+8*4+((1024+15-((3)-2)) MOD (16))*4]
mov edx, [rsp+8*4+((1024+15-((3)-15)) MOD (16))*4]
mov ebx, ebp
shr ebp, 10
ror ebx, 17
xor ebp, ebx
ror ebx, 2
xor ebx, ebp
add ebx, [rsp+8*4+((1024+15-((3)-7)) MOD (16))*4]
mov ebp, edx
shr ebp, 3
ror edx, 7
add ebx, [rsp+8*4+((1024+15-(3)) MOD (16))*4]
xor ebp, edx
add edi, [rsi+(3)*4]
ror edx, 11
add edi, [rsp+((1024+7-(3)) MOD (8))*4]
xor ebp, edx
add ebp, ebx
mov [rsp+8*4+((1024+15-(3)) MOD (16))*4], ebp
add edi, ebp
mov ebx, eax
xor eax, [rsp+((1024+7-(3+6)) MOD (8))*4]
and ecx, eax
xor ecx, [rsp+((1024+7-(3+6)) MOD (8))*4]
mov ebp, ebx
ror ebx, 2
add ecx, edi
add edi, [rsp+((1024+7-(3+4)) MOD (8))*4]
mov [rsp+((1024+7-(3+4)) MOD (8))*4], edi
ror ebp, 22
xor ebp, ebx
ror ebx, 11
xor ebp, ebx
add ecx, ebp
mov [rsp+((1024+7-(3)) MOD (8))*4], ecx
mov edx, [rsp+((1024+7-(4+2)) MOD (8))*4]
xor edx, [rsp+((1024+7-(4+1)) MOD (8))*4]
and edx, edi
xor edx, [rsp+((1024+7-(4+1)) MOD (8))*4]
mov ebp, edi
ror edi, 6
ror ebp, 25
xor ebp, edi
ror edi, 5
xor ebp, edi
add edx, ebp
mov ebp, [rsp+8*4+((1024+15-((4)-2)) MOD (16))*4]
mov edi, [rsp+8*4+((1024+15-((4)-15)) MOD (16))*4]
mov ebx, ebp
shr ebp, 10
ror ebx, 17
xor ebp, ebx
ror ebx, 2
xor ebx, ebp
add ebx, [rsp+8*4+((1024+15-((4)-7)) MOD (16))*4]
mov ebp, edi
shr ebp, 3
ror edi, 7
add ebx, [rsp+8*4+((1024+15-(4)) MOD (16))*4]
xor ebp, edi
add edx, [rsi+(4)*4]
ror edi, 11
add edx, [rsp+((1024+7-(4)) MOD (8))*4]
xor ebp, edi
add ebp, ebx
mov [rsp+8*4+((1024+15-(4)) MOD (16))*4], ebp
add edx, ebp
mov ebx, ecx
xor ecx, [rsp+((1024+7-(4+6)) MOD (8))*4]
and eax, ecx
xor eax, [rsp+((1024+7-(4+6)) MOD (8))*4]
mov ebp, ebx
ror ebx, 2
add eax, edx
add edx, [rsp+((1024+7-(4+4)) MOD (8))*4]
mov [rsp+((1024+7-(4+4)) MOD (8))*4], edx
ror ebp, 22
xor ebp, ebx
ror ebx, 11
xor ebp, ebx
add eax, ebp
mov [rsp+((1024+7-(4)) MOD (8))*4], eax
mov edi, [rsp+((1024+7-(5+2)) MOD (8))*4]
xor edi, [rsp+((1024+7-(5+1)) MOD (8))*4]
and edi, edx
xor edi, [rsp+((1024+7-(5+1)) MOD (8))*4]
mov ebp, edx
ror edx, 6
ror ebp, 25
xor ebp, edx
ror edx, 5
xor ebp, edx
add edi, ebp
mov ebp, [rsp+8*4+((1024+15-((5)-2)) MOD (16))*4]
mov edx, [rsp+8*4+((1024+15-((5)-15)) MOD (16))*4]
mov ebx, ebp
shr ebp, 10
ror ebx, 17
xor ebp, ebx
ror ebx, 2
xor ebx, ebp
add ebx, [rsp+8*4+((1024+15-((5)-7)) MOD (16))*4]
mov ebp, edx
shr ebp, 3
ror edx, 7
add ebx, [rsp+8*4+((1024+15-(5)) MOD (16))*4]
xor ebp, edx
add edi, [rsi+(5)*4]
ror edx, 11
add edi, [rsp+((1024+7-(5)) MOD (8))*4]
xor ebp, edx
add ebp, ebx
mov [rsp+8*4+((1024+15-(5)) MOD (16))*4], ebp
add edi, ebp
mov ebx, eax
xor eax, [rsp+((1024+7-(5+6)) MOD (8))*4]
and ecx, eax
xor ecx, [rsp+((1024+7-(5+6)) MOD (8))*4]
mov ebp, ebx
ror ebx, 2
add ecx, edi
add edi, [rsp+((1024+7-(5+4)) MOD (8))*4]
mov [rsp+((1024+7-(5+4)) MOD (8))*4], edi
ror ebp, 22
xor ebp, ebx
ror ebx, 11
xor ebp, ebx
add ecx, ebp
mov [rsp+((1024+7-(5)) MOD (8))*4], ecx
mov edx, [rsp+((1024+7-(6+2)) MOD (8))*4]
xor edx, [rsp+((1024+7-(6+1)) MOD (8))*4]
and edx, edi
xor edx, [rsp+((1024+7-(6+1)) MOD (8))*4]
mov ebp, edi
ror edi, 6
ror ebp, 25
xor ebp, edi
ror edi, 5
xor ebp, edi
add edx, ebp
mov ebp, [rsp+8*4+((1024+15-((6)-2)) MOD (16))*4]
mov edi, [rsp+8*4+((1024+15-((6)-15)) MOD (16))*4]
mov ebx, ebp
shr ebp, 10
ror ebx, 17
xor ebp, ebx
ror ebx, 2
xor ebx, ebp
add ebx, [rsp+8*4+((1024+15-((6)-7)) MOD (16))*4]
mov ebp, edi
shr ebp, 3
ror edi, 7
add ebx, [rsp+8*4+((1024+15-(6)) MOD (16))*4]
xor ebp, edi
add edx, [rsi+(6)*4]
ror edi, 11
add edx, [rsp+((1024+7-(6)) MOD (8))*4]
xor ebp, edi
add ebp, ebx
mov [rsp+8*4+((1024+15-(6)) MOD (16))*4], ebp
add edx, ebp
mov ebx, ecx
xor ecx, [rsp+((1024+7-(6+6)) MOD (8))*4]
and eax, ecx
xor eax, [rsp+((1024+7-(6+6)) MOD (8))*4]
mov ebp, ebx
ror ebx, 2
add eax, edx
add edx, [rsp+((1024+7-(6+4)) MOD (8))*4]
mov [rsp+((1024+7-(6+4)) MOD (8))*4], edx
ror ebp, 22
xor ebp, ebx
ror ebx, 11
xor ebp, ebx
add eax, ebp
mov [rsp+((1024+7-(6)) MOD (8))*4], eax
mov edi, [rsp+((1024+7-(7+2)) MOD (8))*4]
xor edi, [rsp+((1024+7-(7+1)) MOD (8))*4]
and edi, edx
xor edi, [rsp+((1024+7-(7+1)) MOD (8))*4]
mov ebp, edx
ror edx, 6
ror ebp, 25
xor ebp, edx
ror edx, 5
xor ebp, edx
add edi, ebp
mov ebp, [rsp+8*4+((1024+15-((7)-2)) MOD (16))*4]
mov edx, [rsp+8*4+((1024+15-((7)-15)) MOD (16))*4]
mov ebx, ebp
shr ebp, 10
ror ebx, 17
xor ebp, ebx
ror ebx, 2
xor ebx, ebp
add ebx, [rsp+8*4+((1024+15-((7)-7)) MOD (16))*4]
mov ebp, edx
shr ebp, 3
ror edx, 7
add ebx, [rsp+8*4+((1024+15-(7)) MOD (16))*4]
xor ebp, edx
add edi, [rsi+(7)*4]
ror edx, 11
add edi, [rsp+((1024+7-(7)) MOD (8))*4]
xor ebp, edx
add ebp, ebx
mov [rsp+8*4+((1024+15-(7)) MOD (16))*4], ebp
add edi, ebp
mov ebx, eax
xor eax, [rsp+((1024+7-(7+6)) MOD (8))*4]
and ecx, eax
xor ecx, [rsp+((1024+7-(7+6)) MOD (8))*4]
mov ebp, ebx
ror ebx, 2
add ecx, edi
add edi, [rsp+((1024+7-(7+4)) MOD (8))*4]
mov [rsp+((1024+7-(7+4)) MOD (8))*4], edi
ror ebp, 22
xor ebp, ebx
ror ebx, 11
xor ebp, ebx
add ecx, ebp
mov [rsp+((1024+7-(7)) MOD (8))*4], ecx
mov edx, [rsp+((1024+7-(8+2)) MOD (8))*4]
xor edx, [rsp+((1024+7-(8+1)) MOD (8))*4]
and edx, edi
xor edx, [rsp+((1024+7-(8+1)) MOD (8))*4]
mov ebp, edi
ror edi, 6
ror ebp, 25
xor ebp, edi
ror edi, 5
xor ebp, edi
add edx, ebp
mov ebp, [rsp+8*4+((1024+15-((8)-2)) MOD (16))*4]
mov edi, [rsp+8*4+((1024+15-((8)-15)) MOD (16))*4]
mov ebx, ebp
shr ebp, 10
ror ebx, 17
xor ebp, ebx
ror ebx, 2
xor ebx, ebp
add ebx, [rsp+8*4+((1024+15-((8)-7)) MOD (16))*4]
mov ebp, edi
shr ebp, 3
ror edi, 7
add ebx, [rsp+8*4+((1024+15-(8)) MOD (16))*4]
xor ebp, edi
add edx, [rsi+(8)*4]
ror edi, 11
add edx, [rsp+((1024+7-(8)) MOD (8))*4]
xor ebp, edi
add ebp, ebx
mov [rsp+8*4+((1024+15-(8)) MOD (16))*4], ebp
add edx, ebp
mov ebx, ecx
xor ecx, [rsp+((1024+7-(8+6)) MOD (8))*4]
and eax, ecx
xor eax, [rsp+((1024+7-(8+6)) MOD (8))*4]
mov ebp, ebx
ror ebx, 2
add eax, edx
add edx, [rsp+((1024+7-(8+4)) MOD (8))*4]
mov [rsp+((1024+7-(8+4)) MOD (8))*4], edx
ror ebp, 22
xor ebp, ebx
ror ebx, 11
xor ebp, ebx
add eax, ebp
mov [rsp+((1024+7-(8)) MOD (8))*4], eax
mov edi, [rsp+((1024+7-(9+2)) MOD (8))*4]
xor edi, [rsp+((1024+7-(9+1)) MOD (8))*4]
and edi, edx
xor edi, [rsp+((1024+7-(9+1)) MOD (8))*4]
mov ebp, edx
ror edx, 6
ror ebp, 25
xor ebp, edx
ror edx, 5
xor ebp, edx
add edi, ebp
mov ebp, [rsp+8*4+((1024+15-((9)-2)) MOD (16))*4]
mov edx, [rsp+8*4+((1024+15-((9)-15)) MOD (16))*4]
mov ebx, ebp
shr ebp, 10
ror ebx, 17
xor ebp, ebx
ror ebx, 2
xor ebx, ebp
add ebx, [rsp+8*4+((1024+15-((9)-7)) MOD (16))*4]
mov ebp, edx
shr ebp, 3
ror edx, 7
add ebx, [rsp+8*4+((1024+15-(9)) MOD (16))*4]
xor ebp, edx
add edi, [rsi+(9)*4]
ror edx, 11
add edi, [rsp+((1024+7-(9)) MOD (8))*4]
xor ebp, edx
add ebp, ebx
mov [rsp+8*4+((1024+15-(9)) MOD (16))*4], ebp
add edi, ebp
mov ebx, eax
xor eax, [rsp+((1024+7-(9+6)) MOD (8))*4]
and ecx, eax
xor ecx, [rsp+((1024+7-(9+6)) MOD (8))*4]
mov ebp, ebx
ror ebx, 2
add ecx, edi
add edi, [rsp+((1024+7-(9+4)) MOD (8))*4]
mov [rsp+((1024+7-(9+4)) MOD (8))*4], edi
ror ebp, 22
xor ebp, ebx
ror ebx, 11
xor ebp, ebx
add ecx, ebp
mov [rsp+((1024+7-(9)) MOD (8))*4], ecx
mov edx, [rsp+((1024+7-(10+2)) MOD (8))*4]
xor edx, [rsp+((1024+7-(10+1)) MOD (8))*4]
and edx, edi
xor edx, [rsp+((1024+7-(10+1)) MOD (8))*4]
mov ebp, edi
ror edi, 6
ror ebp, 25
xor ebp, edi
ror edi, 5
xor ebp, edi
add edx, ebp
mov ebp, [rsp+8*4+((1024+15-((10)-2)) MOD (16))*4]
mov edi, [rsp+8*4+((1024+15-((10)-15)) MOD (16))*4]
mov ebx, ebp
shr ebp, 10
ror ebx, 17
xor ebp, ebx
ror ebx, 2
xor ebx, ebp
add ebx, [rsp+8*4+((1024+15-((10)-7)) MOD (16))*4]
mov ebp, edi
shr ebp, 3
ror edi, 7
add ebx, [rsp+8*4+((1024+15-(10)) MOD (16))*4]
xor ebp, edi
add edx, [rsi+(10)*4]
ror edi, 11
add edx, [rsp+((1024+7-(10)) MOD (8))*4]
xor ebp, edi
add ebp, ebx
mov [rsp+8*4+((1024+15-(10)) MOD (16))*4], ebp
add edx, ebp
mov ebx, ecx
xor ecx, [rsp+((1024+7-(10+6)) MOD (8))*4]
and eax, ecx
xor eax, [rsp+((1024+7-(10+6)) MOD (8))*4]
mov ebp, ebx
ror ebx, 2
add eax, edx
add edx, [rsp+((1024+7-(10+4)) MOD (8))*4]
mov [rsp+((1024+7-(10+4)) MOD (8))*4], edx
ror ebp, 22
xor ebp, ebx
ror ebx, 11
xor ebp, ebx
add eax, ebp
mov [rsp+((1024+7-(10)) MOD (8))*4], eax
mov edi, [rsp+((1024+7-(11+2)) MOD (8))*4]
xor edi, [rsp+((1024+7-(11+1)) MOD (8))*4]
and edi, edx
xor edi, [rsp+((1024+7-(11+1)) MOD (8))*4]
mov ebp, edx
ror edx, 6
ror ebp, 25
xor ebp, edx
ror edx, 5
xor ebp, edx
add edi, ebp
mov ebp, [rsp+8*4+((1024+15-((11)-2)) MOD (16))*4]
mov edx, [rsp+8*4+((1024+15-((11)-15)) MOD (16))*4]
mov ebx, ebp
shr ebp, 10
ror ebx, 17
xor ebp, ebx
ror ebx, 2
xor ebx, ebp
add ebx, [rsp+8*4+((1024+15-((11)-7)) MOD (16))*4]
mov ebp, edx
shr ebp, 3
ror edx, 7
add ebx, [rsp+8*4+((1024+15-(11)) MOD (16))*4]
xor ebp, edx
add edi, [rsi+(11)*4]
ror edx, 11
add edi, [rsp+((1024+7-(11)) MOD (8))*4]
xor ebp, edx
add ebp, ebx
mov [rsp+8*4+((1024+15-(11)) MOD (16))*4], ebp
add edi, ebp
mov ebx, eax
xor eax, [rsp+((1024+7-(11+6)) MOD (8))*4]
and ecx, eax
xor ecx, [rsp+((1024+7-(11+6)) MOD (8))*4]
mov ebp, ebx
ror ebx, 2
add ecx, edi
add edi, [rsp+((1024+7-(11+4)) MOD (8))*4]
mov [rsp+((1024+7-(11+4)) MOD (8))*4], edi
ror ebp, 22
xor ebp, ebx
ror ebx, 11
xor ebp, ebx
add ecx, ebp
mov [rsp+((1024+7-(11)) MOD (8))*4], ecx
mov edx, [rsp+((1024+7-(12+2)) MOD (8))*4]
xor edx, [rsp+((1024+7-(12+1)) MOD (8))*4]
and edx, edi
xor edx, [rsp+((1024+7-(12+1)) MOD (8))*4]
mov ebp, edi
ror edi, 6
ror ebp, 25
xor ebp, edi
ror edi, 5
xor ebp, edi
add edx, ebp
mov ebp, [rsp+8*4+((1024+15-((12)-2)) MOD (16))*4]
mov edi, [rsp+8*4+((1024+15-((12)-15)) MOD (16))*4]
mov ebx, ebp
shr ebp, 10
ror ebx, 17
xor ebp, ebx
ror ebx, 2
xor ebx, ebp
add ebx, [rsp+8*4+((1024+15-((12)-7)) MOD (16))*4]
mov ebp, edi
shr ebp, 3
ror edi, 7
add ebx, [rsp+8*4+((1024+15-(12)) MOD (16))*4]
xor ebp, edi
add edx, [rsi+(12)*4]
ror edi, 11
add edx, [rsp+((1024+7-(12)) MOD (8))*4]
xor ebp, edi
add ebp, ebx
mov [rsp+8*4+((1024+15-(12)) MOD (16))*4], ebp
add edx, ebp
mov ebx, ecx
xor ecx, [rsp+((1024+7-(12+6)) MOD (8))*4]
and eax, ecx
xor eax, [rsp+((1024+7-(12+6)) MOD (8))*4]
mov ebp, ebx
ror ebx, 2
add eax, edx
add edx, [rsp+((1024+7-(12+4)) MOD (8))*4]
mov [rsp+((1024+7-(12+4)) MOD (8))*4], edx
ror ebp, 22
xor ebp, ebx
ror ebx, 11
xor ebp, ebx
add eax, ebp
mov [rsp+((1024+7-(12)) MOD (8))*4], eax
mov edi, [rsp+((1024+7-(13+2)) MOD (8))*4]
xor edi, [rsp+((1024+7-(13+1)) MOD (8))*4]
and edi, edx
xor edi, [rsp+((1024+7-(13+1)) MOD (8))*4]
mov ebp, edx
ror edx, 6
ror ebp, 25
xor ebp, edx
ror edx, 5
xor ebp, edx
add edi, ebp
mov ebp, [rsp+8*4+((1024+15-((13)-2)) MOD (16))*4]
mov edx, [rsp+8*4+((1024+15-((13)-15)) MOD (16))*4]
mov ebx, ebp
shr ebp, 10
ror ebx, 17
xor ebp, ebx
ror ebx, 2
xor ebx, ebp
add ebx, [rsp+8*4+((1024+15-((13)-7)) MOD (16))*4]
mov ebp, edx
shr ebp, 3
ror edx, 7
add ebx, [rsp+8*4+((1024+15-(13)) MOD (16))*4]
xor ebp, edx
add edi, [rsi+(13)*4]
ror edx, 11
add edi, [rsp+((1024+7-(13)) MOD (8))*4]
xor ebp, edx
add ebp, ebx
mov [rsp+8*4+((1024+15-(13)) MOD (16))*4], ebp
add edi, ebp
mov ebx, eax
xor eax, [rsp+((1024+7-(13+6)) MOD (8))*4]
and ecx, eax
xor ecx, [rsp+((1024+7-(13+6)) MOD (8))*4]
mov ebp, ebx
ror ebx, 2
add ecx, edi
add edi, [rsp+((1024+7-(13+4)) MOD (8))*4]
mov [rsp+((1024+7-(13+4)) MOD (8))*4], edi
ror ebp, 22
xor ebp, ebx
ror ebx, 11
xor ebp, ebx
add ecx, ebp
mov [rsp+((1024+7-(13)) MOD (8))*4], ecx
mov edx, [rsp+((1024+7-(14+2)) MOD (8))*4]
xor edx, [rsp+((1024+7-(14+1)) MOD (8))*4]
and edx, edi
xor edx, [rsp+((1024+7-(14+1)) MOD (8))*4]
mov ebp, edi
ror edi, 6
ror ebp, 25
xor ebp, edi
ror edi, 5
xor ebp, edi
add edx, ebp
mov ebp, [rsp+8*4+((1024+15-((14)-2)) MOD (16))*4]
mov edi, [rsp+8*4+((1024+15-((14)-15)) MOD (16))*4]
mov ebx, ebp
shr ebp, 10
ror ebx, 17
xor ebp, ebx
ror ebx, 2
xor ebx, ebp
add ebx, [rsp+8*4+((1024+15-((14)-7)) MOD (16))*4]
mov ebp, edi
shr ebp, 3
ror edi, 7
add ebx, [rsp+8*4+((1024+15-(14)) MOD (16))*4]
xor ebp, edi
add edx, [rsi+(14)*4]
ror edi, 11
add edx, [rsp+((1024+7-(14)) MOD (8))*4]
xor ebp, edi
add ebp, ebx
mov [rsp+8*4+((1024+15-(14)) MOD (16))*4], ebp
add edx, ebp
mov ebx, ecx
xor ecx, [rsp+((1024+7-(14+6)) MOD (8))*4]
and eax, ecx
xor eax, [rsp+((1024+7-(14+6)) MOD (8))*4]
mov ebp, ebx
ror ebx, 2
add eax, edx
add edx, [rsp+((1024+7-(14+4)) MOD (8))*4]
mov [rsp+((1024+7-(14+4)) MOD (8))*4], edx
ror ebp, 22
xor ebp, ebx
ror ebx, 11
xor ebp, ebx
add eax, ebp
mov [rsp+((1024+7-(14)) MOD (8))*4], eax
mov edi, [rsp+((1024+7-(15+2)) MOD (8))*4]
xor edi, [rsp+((1024+7-(15+1)) MOD (8))*4]
and edi, edx
xor edi, [rsp+((1024+7-(15+1)) MOD (8))*4]
mov ebp, edx
ror edx, 6
ror ebp, 25
xor ebp, edx
ror edx, 5
xor ebp, edx
add edi, ebp
mov ebp, [rsp+8*4+((1024+15-((15)-2)) MOD (16))*4]
mov edx, [rsp+8*4+((1024+15-((15)-15)) MOD (16))*4]
mov ebx, ebp
shr ebp, 10
ror ebx, 17
xor ebp, ebx
ror ebx, 2
xor ebx, ebp
add ebx, [rsp+8*4+((1024+15-((15)-7)) MOD (16))*4]
mov ebp, edx
shr ebp, 3
ror edx, 7
add ebx, [rsp+8*4+((1024+15-(15)) MOD (16))*4]
xor ebp, edx
add edi, [rsi+(15)*4]
ror edx, 11
add edi, [rsp+((1024+7-(15)) MOD (8))*4]
xor ebp, edx
add ebp, ebx
mov [rsp+8*4+((1024+15-(15)) MOD (16))*4], ebp
add edi, ebp
mov ebx, eax
xor eax, [rsp+((1024+7-(15+6)) MOD (8))*4]
and ecx, eax
xor ecx, [rsp+((1024+7-(15+6)) MOD (8))*4]
mov ebp, ebx
ror ebx, 2
add ecx, edi
add edi, [rsp+((1024+7-(15+4)) MOD (8))*4]
mov [rsp+((1024+7-(15+4)) MOD (8))*4], edi
ror ebp, 22
xor ebp, ebx
ror ebx, 11
xor ebp, ebx
add ecx, ebp
mov [rsp+((1024+7-(15)) MOD (8))*4], ecx
cmp rsi, [rsp+8*4+16*4+0*8]
jne label1
mov rcx, [rsp+8*4+16*4+1*8]
movdqa xmm1, XMMWORD PTR [rcx+1*16]
movdqa xmm0, XMMWORD PTR [rcx+0*16]
paddd xmm1, [rsp+((1024+7-(0+3)) MOD (8))*4]
paddd xmm0, [rsp+((1024+7-(0+7)) MOD (8))*4]
movdqa [rcx+1*16], xmm1
movdqa [rcx+0*16], xmm0
mov rdx, [rsp+8*4+16*4+2*8]
add rdx, 64
mov [rsp+8*4+16*4+2*8], rdx
cmp rdx, [rsp+8*4+16*4+3*8]
jne label0
add		rsp, 8*4 + 16*4 + 4*8 + 8
pop		rbp
pop		rbx
pop		rdi
pop		rsi
ret
X86_SHA256_HashBlocks ENDP

_TEXT ENDS
END
