include ksamd64.inc
EXTERNDEF s_sosemanukMulTables:FAR
.CODE

ALIGN 8
Salsa20_OperateKeystream PROC FRAME
mov r10, [rsp + 5*8]
alloc_stack(10*16 + 32*16 + 8)
save_xmm128 xmm6, 0200h
save_xmm128 xmm7, 0210h
save_xmm128 xmm8, 0220h
save_xmm128 xmm9, 0230h
save_xmm128 xmm10, 0240h
save_xmm128 xmm11, 0250h
save_xmm128 xmm12, 0260h
save_xmm128 xmm13, 0270h
save_xmm128 xmm14, 0280h
save_xmm128 xmm15, 0290h
.endprolog
cmp r8, 4
jl label5
movdqa xmm0, [r10 + 0*16]
movdqa xmm1, [r10 + 1*16]
movdqa xmm2, [r10 + 2*16]
movdqa xmm3, [r10 + 3*16]
pshufd xmm4, xmm0, 0*64+0*16+0*4+0
movdqa [rsp + (0*4+0)*16 + 256], xmm4
pshufd xmm4, xmm0, 1*64+1*16+1*4+1
movdqa [rsp + (0*4+1)*16 + 256], xmm4
pshufd xmm4, xmm0, 2*64+2*16+2*4+2
movdqa [rsp + (0*4+2)*16 + 256], xmm4
pshufd xmm4, xmm0, 3*64+3*16+3*4+3
movdqa [rsp + (0*4+3)*16 + 256], xmm4
pshufd xmm4, xmm1, 0*64+0*16+0*4+0
movdqa [rsp + (1*4+0)*16 + 256], xmm4
pshufd xmm4, xmm1, 2*64+2*16+2*4+2
movdqa [rsp + (1*4+2)*16 + 256], xmm4
pshufd xmm4, xmm1, 3*64+3*16+3*4+3
movdqa [rsp + (1*4+3)*16 + 256], xmm4
pshufd xmm4, xmm2, 1*64+1*16+1*4+1
movdqa [rsp + (2*4+1)*16 + 256], xmm4
pshufd xmm4, xmm2, 2*64+2*16+2*4+2
movdqa [rsp + (2*4+2)*16 + 256], xmm4
pshufd xmm4, xmm2, 3*64+3*16+3*4+3
movdqa [rsp + (2*4+3)*16 + 256], xmm4
pshufd xmm4, xmm3, 0*64+0*16+0*4+0
movdqa [rsp + (3*4+0)*16 + 256], xmm4
pshufd xmm4, xmm3, 1*64+1*16+1*4+1
movdqa [rsp + (3*4+1)*16 + 256], xmm4
pshufd xmm4, xmm3, 2*64+2*16+2*4+2
movdqa [rsp + (3*4+2)*16 + 256], xmm4
pshufd xmm4, xmm3, 3*64+3*16+3*4+3
movdqa [rsp + (3*4+3)*16 + 256], xmm4
label1:
mov eax, dword ptr [r10 + 8*4]
mov r11d, dword ptr [r10 + 5*4]
mov dword ptr [rsp + 8*16 + 0*4 + 256], eax
mov dword ptr [rsp + 5*16 + 0*4 + 256], r11d
add eax, 1
adc r11d, 0
mov dword ptr [rsp + 8*16 + 1*4 + 256], eax
mov dword ptr [rsp + 5*16 + 1*4 + 256], r11d
add eax, 1
adc r11d, 0
mov dword ptr [rsp + 8*16 + 2*4 + 256], eax
mov dword ptr [rsp + 5*16 + 2*4 + 256], r11d
add eax, 1
adc r11d, 0
mov dword ptr [rsp + 8*16 + 3*4 + 256], eax
mov dword ptr [rsp + 5*16 + 3*4 + 256], r11d
add eax, 1
adc r11d, 0
mov dword ptr [r10 + 8*4], eax
mov dword ptr [r10 + 5*4], r11d
movdqa xmm0, [rsp + 12*16 + 1*256]
movdqa xmm4, [rsp + 13*16 + 1*256]
movdqa xmm8, [rsp + 14*16 + 1*256]
movdqa xmm12, [rsp + 15*16 + 1*256]
movdqa xmm2, [rsp + 0*16 + 1*256]
movdqa xmm6, [rsp + 1*16 + 1*256]
movdqa xmm10, [rsp + 2*16 + 1*256]
movdqa xmm14, [rsp + 3*16 + 1*256]
paddd xmm0, xmm2
paddd xmm4, xmm6
paddd xmm8, xmm10
paddd xmm12, xmm14
movdqa xmm1, xmm0
movdqa xmm5, xmm4
movdqa xmm9, xmm8
movdqa xmm13, xmm12
pslld xmm0, 7
pslld xmm4, 7
pslld xmm8, 7
pslld xmm12, 7
psrld xmm1, 32-7
psrld xmm5, 32-7
psrld xmm9, 32-7
psrld xmm13, 32-7
pxor xmm0, [rsp + 4*16 + 1*256]
pxor xmm4, [rsp + 5*16 + 1*256]
pxor xmm8, [rsp + 6*16 + 1*256]
pxor xmm12, [rsp + 7*16 + 1*256]
pxor xmm0, xmm1
pxor xmm4, xmm5
pxor xmm8, xmm9
pxor xmm12, xmm13
movdqa [rsp + 4*16], xmm0
movdqa [rsp + 5*16], xmm4
movdqa [rsp + 6*16], xmm8
movdqa [rsp + 7*16], xmm12
movdqa xmm1, xmm0
movdqa xmm5, xmm4
movdqa xmm9, xmm8
movdqa xmm13, xmm12
paddd xmm0, xmm2
paddd xmm4, xmm6
paddd xmm8, xmm10
paddd xmm12, xmm14
movdqa xmm3, xmm0
movdqa xmm7, xmm4
movdqa xmm11, xmm8
movdqa xmm15, xmm12
pslld xmm0, 9
pslld xmm4, 9
pslld xmm8, 9
pslld xmm12, 9
psrld xmm3, 32-9
psrld xmm7, 32-9
psrld xmm11, 32-9
psrld xmm15, 32-9
pxor xmm0, [rsp + 8*16 + 1*256]
pxor xmm4, [rsp + 9*16 + 1*256]
pxor xmm8, [rsp + 10*16 + 1*256]
pxor xmm12, [rsp + 11*16 + 1*256]
pxor xmm0, xmm3
pxor xmm4, xmm7
pxor xmm8, xmm11
pxor xmm12, xmm15
movdqa [rsp + 8*16], xmm0
movdqa [rsp + 9*16], xmm4
movdqa [rsp + 10*16], xmm8
movdqa [rsp + 11*16], xmm12
movdqa xmm3, xmm0
movdqa xmm7, xmm4
movdqa xmm11, xmm8
movdqa xmm15, xmm12
paddd xmm0, xmm1
paddd xmm4, xmm5
paddd xmm8, xmm9
paddd xmm12, xmm13
movdqa xmm1, xmm0
movdqa xmm5, xmm4
movdqa xmm9, xmm8
movdqa xmm13, xmm12
pslld xmm0, 13
pslld xmm4, 13
pslld xmm8, 13
pslld xmm12, 13
psrld xmm1, 32-13
psrld xmm5, 32-13
psrld xmm9, 32-13
psrld xmm13, 32-13
pxor xmm0, [rsp + 12*16 + 1*256]
pxor xmm4, [rsp + 13*16 + 1*256]
pxor xmm8, [rsp + 14*16 + 1*256]
pxor xmm12, [rsp + 15*16 + 1*256]
pxor xmm0, xmm1
pxor xmm4, xmm5
pxor xmm8, xmm9
pxor xmm12, xmm13
movdqa [rsp + 12*16], xmm0
movdqa [rsp + 13*16], xmm4
movdqa [rsp + 14*16], xmm8
movdqa [rsp + 15*16], xmm12
paddd xmm0, xmm3
paddd xmm4, xmm7
paddd xmm8, xmm11
paddd xmm12, xmm15
movdqa xmm3, xmm0
movdqa xmm7, xmm4
movdqa xmm11, xmm8
movdqa xmm15, xmm12
pslld xmm0, 18
pslld xmm4, 18
pslld xmm8, 18
pslld xmm12, 18
psrld xmm3, 32-18
psrld xmm7, 32-18
psrld xmm11, 32-18
psrld xmm15, 32-18
pxor xmm0, xmm2
pxor xmm4, xmm6
pxor xmm8, xmm10
pxor xmm12, xmm14
pxor xmm0, xmm3
pxor xmm4, xmm7
pxor xmm8, xmm11
pxor xmm12, xmm15
movdqa [rsp + 0*16], xmm0
movdqa [rsp + 1*16], xmm4
movdqa [rsp + 2*16], xmm8
movdqa [rsp + 3*16], xmm12
mov rax, r9
jmp label2
labelSSE2_Salsa_Output:
movdqa xmm0, xmm4
punpckldq xmm4, xmm5
movdqa xmm1, xmm6
punpckldq xmm6, xmm7
movdqa xmm2, xmm4
punpcklqdq xmm4, xmm6
punpckhqdq xmm2, xmm6
punpckhdq xmm0, xmm5
punpckhdq xmm1, xmm7
movdqa xmm6, xmm0
punpcklqdq xmm0, xmm1
punpckhqdq xmm6, xmm1
test rdx, rdx
jz labelSSE2_Salsa_Output_A3
test rdx, 15
jnz labelSSE2_Salsa_Output_A7
pxor xmm4, [rdx+0*16]
pxor xmm2, [rdx+4*16]
pxor xmm0, [rdx+8*16]
pxor xmm6, [rdx+12*16]
add rdx, 1*16
jmp labelSSE2_Salsa_Output_A3
labelSSE2_Salsa_Output_A7:
movdqu xmm1, [rdx+0*16]
pxor xmm4, xmm1
movdqu xmm1, [rdx+4*16]
pxor xmm2, xmm1
movdqu xmm1, [rdx+8*16]
pxor xmm0, xmm1
movdqu xmm1, [rdx+12*16]
pxor xmm6, xmm1
add rdx, 1*16
labelSSE2_Salsa_Output_A3:
test rcx, 15
jnz labelSSE2_Salsa_Output_A8
movdqa [rcx+0*16], xmm4
movdqa [rcx+4*16], xmm2
movdqa [rcx+8*16], xmm0
movdqa [rcx+12*16], xmm6
jmp labelSSE2_Salsa_Output_A9
labelSSE2_Salsa_Output_A8:
movdqu [rcx+0*16], xmm4
movdqu [rcx+4*16], xmm2
movdqu [rcx+8*16], xmm0
movdqu [rcx+12*16], xmm6
labelSSE2_Salsa_Output_A9:
add rcx, 1*16
ret
label6:
movdqa xmm0, [rsp + 12*16 + 0*256]
movdqa xmm4, [rsp + 13*16 + 0*256]
movdqa xmm8, [rsp + 14*16 + 0*256]
movdqa xmm12, [rsp + 15*16 + 0*256]
movdqa xmm2, [rsp + 0*16 + 0*256]
movdqa xmm6, [rsp + 1*16 + 0*256]
movdqa xmm10, [rsp + 2*16 + 0*256]
movdqa xmm14, [rsp + 3*16 + 0*256]
paddd xmm0, xmm2
paddd xmm4, xmm6
paddd xmm8, xmm10
paddd xmm12, xmm14
movdqa xmm1, xmm0
movdqa xmm5, xmm4
movdqa xmm9, xmm8
movdqa xmm13, xmm12
pslld xmm0, 7
pslld xmm4, 7
pslld xmm8, 7
pslld xmm12, 7
psrld xmm1, 32-7
psrld xmm5, 32-7
psrld xmm9, 32-7
psrld xmm13, 32-7
pxor xmm0, [rsp + 4*16 + 0*256]
pxor xmm4, [rsp + 5*16 + 0*256]
pxor xmm8, [rsp + 6*16 + 0*256]
pxor xmm12, [rsp + 7*16 + 0*256]
pxor xmm0, xmm1
pxor xmm4, xmm5
pxor xmm8, xmm9
pxor xmm12, xmm13
movdqa [rsp + 4*16], xmm0
movdqa [rsp + 5*16], xmm4
movdqa [rsp + 6*16], xmm8
movdqa [rsp + 7*16], xmm12
movdqa xmm1, xmm0
movdqa xmm5, xmm4
movdqa xmm9, xmm8
movdqa xmm13, xmm12
paddd xmm0, xmm2
paddd xmm4, xmm6
paddd xmm8, xmm10
paddd xmm12, xmm14
movdqa xmm3, xmm0
movdqa xmm7, xmm4
movdqa xmm11, xmm8
movdqa xmm15, xmm12
pslld xmm0, 9
pslld xmm4, 9
pslld xmm8, 9
pslld xmm12, 9
psrld xmm3, 32-9
psrld xmm7, 32-9
psrld xmm11, 32-9
psrld xmm15, 32-9
pxor xmm0, [rsp + 8*16 + 0*256]
pxor xmm4, [rsp + 9*16 + 0*256]
pxor xmm8, [rsp + 10*16 + 0*256]
pxor xmm12, [rsp + 11*16 + 0*256]
pxor xmm0, xmm3
pxor xmm4, xmm7
pxor xmm8, xmm11
pxor xmm12, xmm15
movdqa [rsp + 8*16], xmm0
movdqa [rsp + 9*16], xmm4
movdqa [rsp + 10*16], xmm8
movdqa [rsp + 11*16], xmm12
movdqa xmm3, xmm0
movdqa xmm7, xmm4
movdqa xmm11, xmm8
movdqa xmm15, xmm12
paddd xmm0, xmm1
paddd xmm4, xmm5
paddd xmm8, xmm9
paddd xmm12, xmm13
movdqa xmm1, xmm0
movdqa xmm5, xmm4
movdqa xmm9, xmm8
movdqa xmm13, xmm12
pslld xmm0, 13
pslld xmm4, 13
pslld xmm8, 13
pslld xmm12, 13
psrld xmm1, 32-13
psrld xmm5, 32-13
psrld xmm9, 32-13
psrld xmm13, 32-13
pxor xmm0, [rsp + 12*16 + 0*256]
pxor xmm4, [rsp + 13*16 + 0*256]
pxor xmm8, [rsp + 14*16 + 0*256]
pxor xmm12, [rsp + 15*16 + 0*256]
pxor xmm0, xmm1
pxor xmm4, xmm5
pxor xmm8, xmm9
pxor xmm12, xmm13
movdqa [rsp + 12*16], xmm0
movdqa [rsp + 13*16], xmm4
movdqa [rsp + 14*16], xmm8
movdqa [rsp + 15*16], xmm12
paddd xmm0, xmm3
paddd xmm4, xmm7
paddd xmm8, xmm11
paddd xmm12, xmm15
movdqa xmm3, xmm0
movdqa xmm7, xmm4
movdqa xmm11, xmm8
movdqa xmm15, xmm12
pslld xmm0, 18
pslld xmm4, 18
pslld xmm8, 18
pslld xmm12, 18
psrld xmm3, 32-18
psrld xmm7, 32-18
psrld xmm11, 32-18
psrld xmm15, 32-18
pxor xmm0, xmm2
pxor xmm4, xmm6
pxor xmm8, xmm10
pxor xmm12, xmm14
pxor xmm0, xmm3
pxor xmm4, xmm7
pxor xmm8, xmm11
pxor xmm12, xmm15
movdqa [rsp + 0*16], xmm0
movdqa [rsp + 1*16], xmm4
movdqa [rsp + 2*16], xmm8
movdqa [rsp + 3*16], xmm12
label2:
movdqa xmm0, [rsp + 7*16 + 0*256]
movdqa xmm4, [rsp + 4*16 + 0*256]
movdqa xmm8, [rsp + 5*16 + 0*256]
movdqa xmm12, [rsp + 6*16 + 0*256]
movdqa xmm2, [rsp + 0*16 + 0*256]
movdqa xmm6, [rsp + 1*16 + 0*256]
movdqa xmm10, [rsp + 2*16 + 0*256]
movdqa xmm14, [rsp + 3*16 + 0*256]
paddd xmm0, xmm2
paddd xmm4, xmm6
paddd xmm8, xmm10
paddd xmm12, xmm14
movdqa xmm1, xmm0
movdqa xmm5, xmm4
movdqa xmm9, xmm8
movdqa xmm13, xmm12
pslld xmm0, 7
pslld xmm4, 7
pslld xmm8, 7
pslld xmm12, 7
psrld xmm1, 32-7
psrld xmm5, 32-7
psrld xmm9, 32-7
psrld xmm13, 32-7
pxor xmm0, [rsp + 13*16 + 0*256]
pxor xmm4, [rsp + 14*16 + 0*256]
pxor xmm8, [rsp + 15*16 + 0*256]
pxor xmm12, [rsp + 12*16 + 0*256]
pxor xmm0, xmm1
pxor xmm4, xmm5
pxor xmm8, xmm9
pxor xmm12, xmm13
movdqa [rsp + 13*16], xmm0
movdqa [rsp + 14*16], xmm4
movdqa [rsp + 15*16], xmm8
movdqa [rsp + 12*16], xmm12
movdqa xmm1, xmm0
movdqa xmm5, xmm4
movdqa xmm9, xmm8
movdqa xmm13, xmm12
paddd xmm0, xmm2
paddd xmm4, xmm6
paddd xmm8, xmm10
paddd xmm12, xmm14
movdqa xmm3, xmm0
movdqa xmm7, xmm4
movdqa xmm11, xmm8
movdqa xmm15, xmm12
pslld xmm0, 9
pslld xmm4, 9
pslld xmm8, 9
pslld xmm12, 9
psrld xmm3, 32-9
psrld xmm7, 32-9
psrld xmm11, 32-9
psrld xmm15, 32-9
pxor xmm0, [rsp + 10*16 + 0*256]
pxor xmm4, [rsp + 11*16 + 0*256]
pxor xmm8, [rsp + 8*16 + 0*256]
pxor xmm12, [rsp + 9*16 + 0*256]
pxor xmm0, xmm3
pxor xmm4, xmm7
pxor xmm8, xmm11
pxor xmm12, xmm15
movdqa [rsp + 10*16], xmm0
movdqa [rsp + 11*16], xmm4
movdqa [rsp + 8*16], xmm8
movdqa [rsp + 9*16], xmm12
movdqa xmm3, xmm0
movdqa xmm7, xmm4
movdqa xmm11, xmm8
movdqa xmm15, xmm12
paddd xmm0, xmm1
paddd xmm4, xmm5
paddd xmm8, xmm9
paddd xmm12, xmm13
movdqa xmm1, xmm0
movdqa xmm5, xmm4
movdqa xmm9, xmm8
movdqa xmm13, xmm12
pslld xmm0, 13
pslld xmm4, 13
pslld xmm8, 13
pslld xmm12, 13
psrld xmm1, 32-13
psrld xmm5, 32-13
psrld xmm9, 32-13
psrld xmm13, 32-13
pxor xmm0, [rsp + 7*16 + 0*256]
pxor xmm4, [rsp + 4*16 + 0*256]
pxor xmm8, [rsp + 5*16 + 0*256]
pxor xmm12, [rsp + 6*16 + 0*256]
pxor xmm0, xmm1
pxor xmm4, xmm5
pxor xmm8, xmm9
pxor xmm12, xmm13
movdqa [rsp + 7*16], xmm0
movdqa [rsp + 4*16], xmm4
movdqa [rsp + 5*16], xmm8
movdqa [rsp + 6*16], xmm12
paddd xmm0, xmm3
paddd xmm4, xmm7
paddd xmm8, xmm11
paddd xmm12, xmm15
movdqa xmm3, xmm0
movdqa xmm7, xmm4
movdqa xmm11, xmm8
movdqa xmm15, xmm12
pslld xmm0, 18
pslld xmm4, 18
pslld xmm8, 18
pslld xmm12, 18
psrld xmm3, 32-18
psrld xmm7, 32-18
psrld xmm11, 32-18
psrld xmm15, 32-18
pxor xmm0, xmm2
pxor xmm4, xmm6
pxor xmm8, xmm10
pxor xmm12, xmm14
pxor xmm0, xmm3
pxor xmm4, xmm7
pxor xmm8, xmm11
pxor xmm12, xmm15
movdqa [rsp + 0*16], xmm0
movdqa [rsp + 1*16], xmm4
movdqa [rsp + 2*16], xmm8
movdqa [rsp + 3*16], xmm12
sub eax, 2
jnz label6
movdqa xmm4, [rsp + 0*16 + 256]
paddd xmm4, [rsp + 0*16]
movdqa xmm5, [rsp + 13*16 + 256]
paddd xmm5, [rsp + 13*16]
movdqa xmm6, [rsp + 10*16 + 256]
paddd xmm6, [rsp + 10*16]
movdqa xmm7, [rsp + 7*16 + 256]
paddd xmm7, [rsp + 7*16]
call labelSSE2_Salsa_Output
movdqa xmm4, [rsp + 4*16 + 256]
paddd xmm4, [rsp + 4*16]
movdqa xmm5, [rsp + 1*16 + 256]
paddd xmm5, [rsp + 1*16]
movdqa xmm6, [rsp + 14*16 + 256]
paddd xmm6, [rsp + 14*16]
movdqa xmm7, [rsp + 11*16 + 256]
paddd xmm7, [rsp + 11*16]
call labelSSE2_Salsa_Output
movdqa xmm4, [rsp + 8*16 + 256]
paddd xmm4, [rsp + 8*16]
movdqa xmm5, [rsp + 5*16 + 256]
paddd xmm5, [rsp + 5*16]
movdqa xmm6, [rsp + 2*16 + 256]
paddd xmm6, [rsp + 2*16]
movdqa xmm7, [rsp + 15*16 + 256]
paddd xmm7, [rsp + 15*16]
call labelSSE2_Salsa_Output
movdqa xmm4, [rsp + 12*16 + 256]
paddd xmm4, [rsp + 12*16]
movdqa xmm5, [rsp + 9*16 + 256]
paddd xmm5, [rsp + 9*16]
movdqa xmm6, [rsp + 6*16 + 256]
paddd xmm6, [rsp + 6*16]
movdqa xmm7, [rsp + 3*16 + 256]
paddd xmm7, [rsp + 3*16]
call labelSSE2_Salsa_Output
test rdx, rdx
jz label9
add rdx, 12*16
label9:
add rcx, 12*16
sub r8, 4
cmp r8, 4
jge label1
label5:
sub r8, 1
jl label4
movdqa xmm0, [r10 + 0*16]
movdqa xmm1, [r10 + 1*16]
movdqa xmm2, [r10 + 2*16]
movdqa xmm3, [r10 + 3*16]
mov rax, r9
label0:
movdqa xmm4, xmm3
paddd xmm4, xmm0
movdqa xmm5, xmm4
pslld xmm4, 7
psrld xmm5, 32-7
pxor xmm1, xmm4
pxor xmm1, xmm5
movdqa xmm4, xmm0
paddd xmm4, xmm1
movdqa xmm5, xmm4
pslld xmm4, 9
psrld xmm5, 32-9
pxor xmm2, xmm4
pxor xmm2, xmm5
movdqa xmm4, xmm1
paddd xmm4, xmm2
movdqa xmm5, xmm4
pslld xmm4, 13
psrld xmm5, 32-13
pxor xmm3, xmm4
pxor xmm3, xmm5
movdqa xmm4, xmm2
paddd xmm4, xmm3
movdqa xmm5, xmm4
pslld xmm4, 18
psrld xmm5, 32-18
pxor xmm0, xmm4
pxor xmm0, xmm5
pshufd xmm1, xmm1, 2*64+1*16+0*4+3
pshufd xmm2, xmm2, 1*64+0*16+3*4+2
pshufd xmm3, xmm3, 0*64+3*16+2*4+1
movdqa xmm4, xmm1
paddd xmm4, xmm0
movdqa xmm5, xmm4
pslld xmm4, 7
psrld xmm5, 32-7
pxor xmm3, xmm4
pxor xmm3, xmm5
movdqa xmm4, xmm0
paddd xmm4, xmm3
movdqa xmm5, xmm4
pslld xmm4, 9
psrld xmm5, 32-9
pxor xmm2, xmm4
pxor xmm2, xmm5
movdqa xmm4, xmm3
paddd xmm4, xmm2
movdqa xmm5, xmm4
pslld xmm4, 13
psrld xmm5, 32-13
pxor xmm1, xmm4
pxor xmm1, xmm5
movdqa xmm4, xmm2
paddd xmm4, xmm1
movdqa xmm5, xmm4
pslld xmm4, 18
psrld xmm5, 32-18
pxor xmm0, xmm4
pxor xmm0, xmm5
pshufd xmm1, xmm1, 0*64+3*16+2*4+1
pshufd xmm2, xmm2, 1*64+0*16+3*4+2
pshufd xmm3, xmm3, 2*64+1*16+0*4+3
sub eax, 2
jnz label0
paddd xmm0, [r10 + 0*16]
paddd xmm1, [r10 + 1*16]
paddd xmm2, [r10 + 2*16]
paddd xmm3, [r10 + 3*16]
add dword ptr [r10 + 8*4], 1
adc dword ptr [r10 + 5*4], 0
pcmpeqb xmm6, xmm6
psrlq xmm6, 32
pshufd xmm7, xmm6, 0*64+1*16+2*4+3
movdqa xmm4, xmm0
movdqa xmm5, xmm3
pand xmm0, xmm7
pand xmm4, xmm6
pand xmm3, xmm6
pand xmm5, xmm7
por xmm4, xmm5
movdqa xmm5, xmm1
pand xmm1, xmm7
pand xmm5, xmm6
por xmm0, xmm5
pand xmm6, xmm2
pand xmm2, xmm7
por xmm1, xmm6
por xmm2, xmm3
movdqa xmm5, xmm4
movdqa xmm6, xmm0
shufpd xmm4, xmm1, 2
shufpd xmm0, xmm2, 2
shufpd xmm1, xmm5, 2
shufpd xmm2, xmm6, 2
test rdx, rdx
jz labelSSE2_Salsa_Output_B3
test rdx, 15
jnz labelSSE2_Salsa_Output_B7
pxor xmm4, [rdx+0*16]
pxor xmm0, [rdx+1*16]
pxor xmm1, [rdx+2*16]
pxor xmm2, [rdx+3*16]
add rdx, 4*16
jmp labelSSE2_Salsa_Output_B3
labelSSE2_Salsa_Output_B7:
movdqu xmm3, [rdx+0*16]
pxor xmm4, xmm3
movdqu xmm3, [rdx+1*16]
pxor xmm0, xmm3
movdqu xmm3, [rdx+2*16]
pxor xmm1, xmm3
movdqu xmm3, [rdx+3*16]
pxor xmm2, xmm3
add rdx, 4*16
labelSSE2_Salsa_Output_B3:
test rcx, 15
jnz labelSSE2_Salsa_Output_B8
movdqa [rcx+0*16], xmm4
movdqa [rcx+1*16], xmm0
movdqa [rcx+2*16], xmm1
movdqa [rcx+3*16], xmm2
jmp labelSSE2_Salsa_Output_B9
labelSSE2_Salsa_Output_B8:
movdqu [rcx+0*16], xmm4
movdqu [rcx+1*16], xmm0
movdqu [rcx+2*16], xmm1
movdqu [rcx+3*16], xmm2
labelSSE2_Salsa_Output_B9:
add rcx, 4*16
jmp label5
label4:
movdqa xmm6, [rsp + 0200h]
movdqa xmm7, [rsp + 0210h]
movdqa xmm8, [rsp + 0220h]
movdqa xmm9, [rsp + 0230h]
movdqa xmm10, [rsp + 0240h]
movdqa xmm11, [rsp + 0250h]
movdqa xmm12, [rsp + 0260h]
movdqa xmm13, [rsp + 0270h]
movdqa xmm14, [rsp + 0280h]
movdqa xmm15, [rsp + 0290h]
add rsp, 10*16 + 32*16 + 8
ret
Salsa20_OperateKeystream ENDP

ALIGN 8
Sosemanuk_OperateKeystream PROC FRAME
rex_push_reg rsi
push_reg rdi
alloc_stack(80*4*2+12*4+8*8 + 2*16+8)
save_xmm128 xmm6, 02f0h
save_xmm128 xmm7, 0300h
.endprolog
mov rdi, r8
mov rax, r9
mov QWORD PTR [rsp+1*8], rdi
mov QWORD PTR [rsp+2*8], rdx
mov QWORD PTR [rsp+6*8], rax
lea rcx, [4*rcx+rcx]
lea rsi, [4*rcx]
mov QWORD PTR [rsp+3*8], rsi
movdqa xmm0, [rax+0*16]
movdqa [rsp + 8*8+0*16], xmm0
movdqa xmm0, [rax+1*16]
movdqa [rsp + 8*8+1*16], xmm0
movq xmm0, QWORD PTR [rax+2*16]
movq QWORD PTR [rsp + 8*8+2*16], xmm0
psrlq xmm0, 32
movd r10d, xmm0
mov ecx, [rax+10*4]
mov edx, [rax+11*4]
pcmpeqb xmm7, xmm7
label2:
lea rdi, [rsp + 8*8 + 12*4]
mov rax, 80
cmp rsi, 80
cmovg rsi, rax
mov QWORD PTR [rsp+7*8], rsi
lea rsi, [rdi+rsi]
mov QWORD PTR [rsp+4*8], rsi
lea rsi, s_sosemanukMulTables
label0:
mov eax, [rsp + 8*8 + ((0+0)-((0+0)/(10))*(10))*4]
mov [rdi + (((0)-((0)/(4))*(4))*20 + (0/4)) * 4 + 80*4], eax
rol eax, 8
lea r11d, [r10d + edx]
xor r11d, ecx
mov [rdi + (((0)-((0)/(4))*(4))*20 + (0/4)) * 4], r11d
mov r11d, 1
and r11d, edx
neg r11d
and r11d, r10d
xor r10d, eax
movzx eax, al
xor r10d, [rsi+rax*4]
mov eax, [rsp + 8*8 + ((0+3)-((0+3)/(10))*(10))*4]
xor r11d, [rsp + 8*8 + ((0+2)-((0+2)/(10))*(10))*4]
add ecx, r11d
movzx r11d, al
shr eax, 8
xor r10d, [rsi+1024+r11*4]
xor r10d, eax
imul edx, 54655307h
rol edx, 7
mov [rsp + 8*8 + ((0+0)-((0+0)/(10))*(10))*4], r10d
mov eax, [rsp + 8*8 + ((1+0)-((1+0)/(10))*(10))*4]
mov [rdi + (((1)-((1)/(4))*(4))*20 + (1/4)) * 4 + 80*4], eax
rol eax, 8
lea r11d, [r10d + ecx]
xor r11d, edx
mov [rdi + (((1)-((1)/(4))*(4))*20 + (1/4)) * 4], r11d
mov r11d, 1
and r11d, ecx
neg r11d
and r11d, r10d
xor r10d, eax
movzx eax, al
xor r10d, [rsi+rax*4]
mov eax, [rsp + 8*8 + ((1+3)-((1+3)/(10))*(10))*4]
xor r11d, [rsp + 8*8 + ((1+2)-((1+2)/(10))*(10))*4]
add edx, r11d
movzx r11d, al
shr eax, 8
xor r10d, [rsi+1024+r11*4]
xor r10d, eax
imul ecx, 54655307h
rol ecx, 7
mov [rsp + 8*8 + ((1+0)-((1+0)/(10))*(10))*4], r10d
mov eax, [rsp + 8*8 + ((2+0)-((2+0)/(10))*(10))*4]
mov [rdi + (((2)-((2)/(4))*(4))*20 + (2/4)) * 4 + 80*4], eax
rol eax, 8
lea r11d, [r10d + edx]
xor r11d, ecx
mov [rdi + (((2)-((2)/(4))*(4))*20 + (2/4)) * 4], r11d
mov r11d, 1
and r11d, edx
neg r11d
and r11d, r10d
xor r10d, eax
movzx eax, al
xor r10d, [rsi+rax*4]
mov eax, [rsp + 8*8 + ((2+3)-((2+3)/(10))*(10))*4]
xor r11d, [rsp + 8*8 + ((2+2)-((2+2)/(10))*(10))*4]
add ecx, r11d
movzx r11d, al
shr eax, 8
xor r10d, [rsi+1024+r11*4]
xor r10d, eax
imul edx, 54655307h
rol edx, 7
mov [rsp + 8*8 + ((2+0)-((2+0)/(10))*(10))*4], r10d
mov eax, [rsp + 8*8 + ((3+0)-((3+0)/(10))*(10))*4]
mov [rdi + (((3)-((3)/(4))*(4))*20 + (3/4)) * 4 + 80*4], eax
rol eax, 8
lea r11d, [r10d + ecx]
xor r11d, edx
mov [rdi + (((3)-((3)/(4))*(4))*20 + (3/4)) * 4], r11d
mov r11d, 1
and r11d, ecx
neg r11d
and r11d, r10d
xor r10d, eax
movzx eax, al
xor r10d, [rsi+rax*4]
mov eax, [rsp + 8*8 + ((3+3)-((3+3)/(10))*(10))*4]
xor r11d, [rsp + 8*8 + ((3+2)-((3+2)/(10))*(10))*4]
add edx, r11d
movzx r11d, al
shr eax, 8
xor r10d, [rsi+1024+r11*4]
xor r10d, eax
imul ecx, 54655307h
rol ecx, 7
mov [rsp + 8*8 + ((3+0)-((3+0)/(10))*(10))*4], r10d
mov eax, [rsp + 8*8 + ((4+0)-((4+0)/(10))*(10))*4]
mov [rdi + (((4)-((4)/(4))*(4))*20 + (4/4)) * 4 + 80*4], eax
rol eax, 8
lea r11d, [r10d + edx]
xor r11d, ecx
mov [rdi + (((4)-((4)/(4))*(4))*20 + (4/4)) * 4], r11d
mov r11d, 1
and r11d, edx
neg r11d
and r11d, r10d
xor r10d, eax
movzx eax, al
xor r10d, [rsi+rax*4]
mov eax, [rsp + 8*8 + ((4+3)-((4+3)/(10))*(10))*4]
xor r11d, [rsp + 8*8 + ((4+2)-((4+2)/(10))*(10))*4]
add ecx, r11d
movzx r11d, al
shr eax, 8
xor r10d, [rsi+1024+r11*4]
xor r10d, eax
imul edx, 54655307h
rol edx, 7
mov [rsp + 8*8 + ((4+0)-((4+0)/(10))*(10))*4], r10d
mov eax, [rsp + 8*8 + ((5+0)-((5+0)/(10))*(10))*4]
mov [rdi + (((5)-((5)/(4))*(4))*20 + (5/4)) * 4 + 80*4], eax
rol eax, 8
lea r11d, [r10d + ecx]
xor r11d, edx
mov [rdi + (((5)-((5)/(4))*(4))*20 + (5/4)) * 4], r11d
mov r11d, 1
and r11d, ecx
neg r11d
and r11d, r10d
xor r10d, eax
movzx eax, al
xor r10d, [rsi+rax*4]
mov eax, [rsp + 8*8 + ((5+3)-((5+3)/(10))*(10))*4]
xor r11d, [rsp + 8*8 + ((5+2)-((5+2)/(10))*(10))*4]
add edx, r11d
movzx r11d, al
shr eax, 8
xor r10d, [rsi+1024+r11*4]
xor r10d, eax
imul ecx, 54655307h
rol ecx, 7
mov [rsp + 8*8 + ((5+0)-((5+0)/(10))*(10))*4], r10d
mov eax, [rsp + 8*8 + ((6+0)-((6+0)/(10))*(10))*4]
mov [rdi + (((6)-((6)/(4))*(4))*20 + (6/4)) * 4 + 80*4], eax
rol eax, 8
lea r11d, [r10d + edx]
xor r11d, ecx
mov [rdi + (((6)-((6)/(4))*(4))*20 + (6/4)) * 4], r11d
mov r11d, 1
and r11d, edx
neg r11d
and r11d, r10d
xor r10d, eax
movzx eax, al
xor r10d, [rsi+rax*4]
mov eax, [rsp + 8*8 + ((6+3)-((6+3)/(10))*(10))*4]
xor r11d, [rsp + 8*8 + ((6+2)-((6+2)/(10))*(10))*4]
add ecx, r11d
movzx r11d, al
shr eax, 8
xor r10d, [rsi+1024+r11*4]
xor r10d, eax
imul edx, 54655307h
rol edx, 7
mov [rsp + 8*8 + ((6+0)-((6+0)/(10))*(10))*4], r10d
mov eax, [rsp + 8*8 + ((7+0)-((7+0)/(10))*(10))*4]
mov [rdi + (((7)-((7)/(4))*(4))*20 + (7/4)) * 4 + 80*4], eax
rol eax, 8
lea r11d, [r10d + ecx]
xor r11d, edx
mov [rdi + (((7)-((7)/(4))*(4))*20 + (7/4)) * 4], r11d
mov r11d, 1
and r11d, ecx
neg r11d
and r11d, r10d
xor r10d, eax
movzx eax, al
xor r10d, [rsi+rax*4]
mov eax, [rsp + 8*8 + ((7+3)-((7+3)/(10))*(10))*4]
xor r11d, [rsp + 8*8 + ((7+2)-((7+2)/(10))*(10))*4]
add edx, r11d
movzx r11d, al
shr eax, 8
xor r10d, [rsi+1024+r11*4]
xor r10d, eax
imul ecx, 54655307h
rol ecx, 7
mov [rsp + 8*8 + ((7+0)-((7+0)/(10))*(10))*4], r10d
mov eax, [rsp + 8*8 + ((8+0)-((8+0)/(10))*(10))*4]
mov [rdi + (((8)-((8)/(4))*(4))*20 + (8/4)) * 4 + 80*4], eax
rol eax, 8
lea r11d, [r10d + edx]
xor r11d, ecx
mov [rdi + (((8)-((8)/(4))*(4))*20 + (8/4)) * 4], r11d
mov r11d, 1
and r11d, edx
neg r11d
and r11d, r10d
xor r10d, eax
movzx eax, al
xor r10d, [rsi+rax*4]
mov eax, [rsp + 8*8 + ((8+3)-((8+3)/(10))*(10))*4]
xor r11d, [rsp + 8*8 + ((8+2)-((8+2)/(10))*(10))*4]
add ecx, r11d
movzx r11d, al
shr eax, 8
xor r10d, [rsi+1024+r11*4]
xor r10d, eax
imul edx, 54655307h
rol edx, 7
mov [rsp + 8*8 + ((8+0)-((8+0)/(10))*(10))*4], r10d
mov eax, [rsp + 8*8 + ((9+0)-((9+0)/(10))*(10))*4]
mov [rdi + (((9)-((9)/(4))*(4))*20 + (9/4)) * 4 + 80*4], eax
rol eax, 8
lea r11d, [r10d + ecx]
xor r11d, edx
mov [rdi + (((9)-((9)/(4))*(4))*20 + (9/4)) * 4], r11d
mov r11d, 1
and r11d, ecx
neg r11d
and r11d, r10d
xor r10d, eax
movzx eax, al
xor r10d, [rsi+rax*4]
mov eax, [rsp + 8*8 + ((9+3)-((9+3)/(10))*(10))*4]
xor r11d, [rsp + 8*8 + ((9+2)-((9+2)/(10))*(10))*4]
add edx, r11d
movzx r11d, al
shr eax, 8
xor r10d, [rsi+1024+r11*4]
xor r10d, eax
imul ecx, 54655307h
rol ecx, 7
mov [rsp + 8*8 + ((9+0)-((9+0)/(10))*(10))*4], r10d
mov eax, [rsp + 8*8 + ((10+0)-((10+0)/(10))*(10))*4]
mov [rdi + (((10)-((10)/(4))*(4))*20 + (10/4)) * 4 + 80*4], eax
rol eax, 8
lea r11d, [r10d + edx]
xor r11d, ecx
mov [rdi + (((10)-((10)/(4))*(4))*20 + (10/4)) * 4], r11d
mov r11d, 1
and r11d, edx
neg r11d
and r11d, r10d
xor r10d, eax
movzx eax, al
xor r10d, [rsi+rax*4]
mov eax, [rsp + 8*8 + ((10+3)-((10+3)/(10))*(10))*4]
xor r11d, [rsp + 8*8 + ((10+2)-((10+2)/(10))*(10))*4]
add ecx, r11d
movzx r11d, al
shr eax, 8
xor r10d, [rsi+1024+r11*4]
xor r10d, eax
imul edx, 54655307h
rol edx, 7
mov [rsp + 8*8 + ((10+0)-((10+0)/(10))*(10))*4], r10d
mov eax, [rsp + 8*8 + ((11+0)-((11+0)/(10))*(10))*4]
mov [rdi + (((11)-((11)/(4))*(4))*20 + (11/4)) * 4 + 80*4], eax
rol eax, 8
lea r11d, [r10d + ecx]
xor r11d, edx
mov [rdi + (((11)-((11)/(4))*(4))*20 + (11/4)) * 4], r11d
mov r11d, 1
and r11d, ecx
neg r11d
and r11d, r10d
xor r10d, eax
movzx eax, al
xor r10d, [rsi+rax*4]
mov eax, [rsp + 8*8 + ((11+3)-((11+3)/(10))*(10))*4]
xor r11d, [rsp + 8*8 + ((11+2)-((11+2)/(10))*(10))*4]
add edx, r11d
movzx r11d, al
shr eax, 8
xor r10d, [rsi+1024+r11*4]
xor r10d, eax
imul ecx, 54655307h
rol ecx, 7
mov [rsp + 8*8 + ((11+0)-((11+0)/(10))*(10))*4], r10d
mov eax, [rsp + 8*8 + ((12+0)-((12+0)/(10))*(10))*4]
mov [rdi + (((12)-((12)/(4))*(4))*20 + (12/4)) * 4 + 80*4], eax
rol eax, 8
lea r11d, [r10d + edx]
xor r11d, ecx
mov [rdi + (((12)-((12)/(4))*(4))*20 + (12/4)) * 4], r11d
mov r11d, 1
and r11d, edx
neg r11d
and r11d, r10d
xor r10d, eax
movzx eax, al
xor r10d, [rsi+rax*4]
mov eax, [rsp + 8*8 + ((12+3)-((12+3)/(10))*(10))*4]
xor r11d, [rsp + 8*8 + ((12+2)-((12+2)/(10))*(10))*4]
add ecx, r11d
movzx r11d, al
shr eax, 8
xor r10d, [rsi+1024+r11*4]
xor r10d, eax
imul edx, 54655307h
rol edx, 7
mov [rsp + 8*8 + ((12+0)-((12+0)/(10))*(10))*4], r10d
mov eax, [rsp + 8*8 + ((13+0)-((13+0)/(10))*(10))*4]
mov [rdi + (((13)-((13)/(4))*(4))*20 + (13/4)) * 4 + 80*4], eax
rol eax, 8
lea r11d, [r10d + ecx]
xor r11d, edx
mov [rdi + (((13)-((13)/(4))*(4))*20 + (13/4)) * 4], r11d
mov r11d, 1
and r11d, ecx
neg r11d
and r11d, r10d
xor r10d, eax
movzx eax, al
xor r10d, [rsi+rax*4]
mov eax, [rsp + 8*8 + ((13+3)-((13+3)/(10))*(10))*4]
xor r11d, [rsp + 8*8 + ((13+2)-((13+2)/(10))*(10))*4]
add edx, r11d
movzx r11d, al
shr eax, 8
xor r10d, [rsi+1024+r11*4]
xor r10d, eax
imul ecx, 54655307h
rol ecx, 7
mov [rsp + 8*8 + ((13+0)-((13+0)/(10))*(10))*4], r10d
mov eax, [rsp + 8*8 + ((14+0)-((14+0)/(10))*(10))*4]
mov [rdi + (((14)-((14)/(4))*(4))*20 + (14/4)) * 4 + 80*4], eax
rol eax, 8
lea r11d, [r10d + edx]
xor r11d, ecx
mov [rdi + (((14)-((14)/(4))*(4))*20 + (14/4)) * 4], r11d
mov r11d, 1
and r11d, edx
neg r11d
and r11d, r10d
xor r10d, eax
movzx eax, al
xor r10d, [rsi+rax*4]
mov eax, [rsp + 8*8 + ((14+3)-((14+3)/(10))*(10))*4]
xor r11d, [rsp + 8*8 + ((14+2)-((14+2)/(10))*(10))*4]
add ecx, r11d
movzx r11d, al
shr eax, 8
xor r10d, [rsi+1024+r11*4]
xor r10d, eax
imul edx, 54655307h
rol edx, 7
mov [rsp + 8*8 + ((14+0)-((14+0)/(10))*(10))*4], r10d
mov eax, [rsp + 8*8 + ((15+0)-((15+0)/(10))*(10))*4]
mov [rdi + (((15)-((15)/(4))*(4))*20 + (15/4)) * 4 + 80*4], eax
rol eax, 8
lea r11d, [r10d + ecx]
xor r11d, edx
mov [rdi + (((15)-((15)/(4))*(4))*20 + (15/4)) * 4], r11d
mov r11d, 1
and r11d, ecx
neg r11d
and r11d, r10d
xor r10d, eax
movzx eax, al
xor r10d, [rsi+rax*4]
mov eax, [rsp + 8*8 + ((15+3)-((15+3)/(10))*(10))*4]
xor r11d, [rsp + 8*8 + ((15+2)-((15+2)/(10))*(10))*4]
add edx, r11d
movzx r11d, al
shr eax, 8
xor r10d, [rsi+1024+r11*4]
xor r10d, eax
imul ecx, 54655307h
rol ecx, 7
mov [rsp + 8*8 + ((15+0)-((15+0)/(10))*(10))*4], r10d
mov eax, [rsp + 8*8 + ((16+0)-((16+0)/(10))*(10))*4]
mov [rdi + (((16)-((16)/(4))*(4))*20 + (16/4)) * 4 + 80*4], eax
rol eax, 8
lea r11d, [r10d + edx]
xor r11d, ecx
mov [rdi + (((16)-((16)/(4))*(4))*20 + (16/4)) * 4], r11d
mov r11d, 1
and r11d, edx
neg r11d
and r11d, r10d
xor r10d, eax
movzx eax, al
xor r10d, [rsi+rax*4]
mov eax, [rsp + 8*8 + ((16+3)-((16+3)/(10))*(10))*4]
xor r11d, [rsp + 8*8 + ((16+2)-((16+2)/(10))*(10))*4]
add ecx, r11d
movzx r11d, al
shr eax, 8
xor r10d, [rsi+1024+r11*4]
xor r10d, eax
imul edx, 54655307h
rol edx, 7
mov [rsp + 8*8 + ((16+0)-((16+0)/(10))*(10))*4], r10d
mov eax, [rsp + 8*8 + ((17+0)-((17+0)/(10))*(10))*4]
mov [rdi + (((17)-((17)/(4))*(4))*20 + (17/4)) * 4 + 80*4], eax
rol eax, 8
lea r11d, [r10d + ecx]
xor r11d, edx
mov [rdi + (((17)-((17)/(4))*(4))*20 + (17/4)) * 4], r11d
mov r11d, 1
and r11d, ecx
neg r11d
and r11d, r10d
xor r10d, eax
movzx eax, al
xor r10d, [rsi+rax*4]
mov eax, [rsp + 8*8 + ((17+3)-((17+3)/(10))*(10))*4]
xor r11d, [rsp + 8*8 + ((17+2)-((17+2)/(10))*(10))*4]
add edx, r11d
movzx r11d, al
shr eax, 8
xor r10d, [rsi+1024+r11*4]
xor r10d, eax
imul ecx, 54655307h
rol ecx, 7
mov [rsp + 8*8 + ((17+0)-((17+0)/(10))*(10))*4], r10d
mov eax, [rsp + 8*8 + ((18+0)-((18+0)/(10))*(10))*4]
mov [rdi + (((18)-((18)/(4))*(4))*20 + (18/4)) * 4 + 80*4], eax
rol eax, 8
lea r11d, [r10d + edx]
xor r11d, ecx
mov [rdi + (((18)-((18)/(4))*(4))*20 + (18/4)) * 4], r11d
mov r11d, 1
and r11d, edx
neg r11d
and r11d, r10d
xor r10d, eax
movzx eax, al
xor r10d, [rsi+rax*4]
mov eax, [rsp + 8*8 + ((18+3)-((18+3)/(10))*(10))*4]
xor r11d, [rsp + 8*8 + ((18+2)-((18+2)/(10))*(10))*4]
add ecx, r11d
movzx r11d, al
shr eax, 8
xor r10d, [rsi+1024+r11*4]
xor r10d, eax
imul edx, 54655307h
rol edx, 7
mov [rsp + 8*8 + ((18+0)-((18+0)/(10))*(10))*4], r10d
mov eax, [rsp + 8*8 + ((19+0)-((19+0)/(10))*(10))*4]
mov [rdi + (((19)-((19)/(4))*(4))*20 + (19/4)) * 4 + 80*4], eax
rol eax, 8
lea r11d, [r10d + ecx]
xor r11d, edx
mov [rdi + (((19)-((19)/(4))*(4))*20 + (19/4)) * 4], r11d
mov r11d, 1
and r11d, ecx
neg r11d
and r11d, r10d
xor r10d, eax
movzx eax, al
xor r10d, [rsi+rax*4]
mov eax, [rsp + 8*8 + ((19+3)-((19+3)/(10))*(10))*4]
xor r11d, [rsp + 8*8 + ((19+2)-((19+2)/(10))*(10))*4]
add edx, r11d
movzx r11d, al
shr eax, 8
xor r10d, [rsi+1024+r11*4]
xor r10d, eax
imul ecx, 54655307h
rol ecx, 7
mov [rsp + 8*8 + ((19+0)-((19+0)/(10))*(10))*4], r10d
add rdi, 5*4
cmp rdi, QWORD PTR [rsp+4*8]
jne label0
mov rax, QWORD PTR [rsp+2*8]
mov r11, QWORD PTR [rsp+1*8]
lea rdi, [rsp + 8*8 + 12*4]
mov rsi, QWORD PTR [rsp+7*8]
label1:
movdqa xmm0, [rdi+0*20*4]
movdqa xmm2, [rdi+2*20*4]
movdqa xmm3, [rdi+3*20*4]
movdqa xmm1, [rdi+1*20*4]
movdqa xmm4, xmm0
pand xmm0, xmm2
pxor xmm0, xmm3
pxor xmm2, xmm1
pxor xmm2, xmm0
por xmm3, xmm4
pxor xmm3, xmm1
pxor xmm4, xmm2
movdqa xmm1, xmm3
por xmm3, xmm4
pxor xmm3, xmm0
pand xmm0, xmm1
pxor xmm4, xmm0
pxor xmm1, xmm3
pxor xmm1, xmm4
pxor xmm4, xmm7
pxor xmm2, [rdi+80*4]
pxor xmm3, [rdi+80*5]
pxor xmm1, [rdi+80*6]
pxor xmm4, [rdi+80*7]
cmp rsi, 16
jl label4
movdqa xmm6, xmm2
punpckldq xmm2, xmm3
movdqa xmm5, xmm1
punpckldq xmm1, xmm4
movdqa xmm0, xmm2
punpcklqdq xmm2, xmm1
punpckhqdq xmm0, xmm1
punpckhdq xmm6, xmm3
punpckhdq xmm5, xmm4
movdqa xmm3, xmm6
punpcklqdq xmm6, xmm5
punpckhqdq xmm3, xmm5
test rax, rax
jz labelSSE2_Sosemanuk_Output3
test rax, 15
jnz labelSSE2_Sosemanuk_Output7
pxor xmm2, [rax+0*16]
pxor xmm0, [rax+1*16]
pxor xmm6, [rax+2*16]
pxor xmm3, [rax+3*16]
add rax, 4*16
jmp labelSSE2_Sosemanuk_Output3
labelSSE2_Sosemanuk_Output7:
movdqu xmm1, [rax+0*16]
pxor xmm2, xmm1
movdqu xmm1, [rax+1*16]
pxor xmm0, xmm1
movdqu xmm1, [rax+2*16]
pxor xmm6, xmm1
movdqu xmm1, [rax+3*16]
pxor xmm3, xmm1
add rax, 4*16
labelSSE2_Sosemanuk_Output3:
test r11, 15
jnz labelSSE2_Sosemanuk_Output8
movdqa [r11+0*16], xmm2
movdqa [r11+1*16], xmm0
movdqa [r11+2*16], xmm6
movdqa [r11+3*16], xmm3
jmp labelSSE2_Sosemanuk_Output9
labelSSE2_Sosemanuk_Output8:
movdqu [r11+0*16], xmm2
movdqu [r11+1*16], xmm0
movdqu [r11+2*16], xmm6
movdqu [r11+3*16], xmm3
labelSSE2_Sosemanuk_Output9:
add r11, 4*16
add rdi, 4*4
sub rsi, 16
jnz label1
mov rsi, QWORD PTR [rsp+3*8]
sub rsi, 80
jz label6
mov QWORD PTR [rsp+3*8], rsi
mov QWORD PTR [rsp+2*8], rax
mov QWORD PTR [rsp+1*8], r11
jmp label2
label4:
test rax, rax
jz label5
movd xmm0, dword ptr [rax+0*4]
pxor xmm2, xmm0
movd xmm0, dword ptr [rax+1*4]
pxor xmm3, xmm0
movd xmm0, dword ptr [rax+2*4]
pxor xmm1, xmm0
movd xmm0, dword ptr [rax+3*4]
pxor xmm4, xmm0
add rax, 16
label5:
movd dword ptr [r11+0*4], xmm2
movd dword ptr [r11+1*4], xmm3
movd dword ptr [r11+2*4], xmm1
movd dword ptr [r11+3*4], xmm4
sub rsi, 4
jz label6
add r11, 16
psrldq xmm2, 4
psrldq xmm3, 4
psrldq xmm1, 4
psrldq xmm4, 4
jmp label4
label6:
mov r10, QWORD PTR [rsp+6*8]
movdqa xmm0, [rsp + 8*8+0*16]
movdqa [r10+0*16], xmm0
movdqa xmm0, [rsp + 8*8+1*16]
movdqa [r10+1*16], xmm0
movq xmm0, QWORD PTR [rsp + 8*8+2*16]
movq QWORD PTR [r10+2*16], xmm0
mov [r10+10*4], ecx
mov [r10+11*4], edx
movdqa xmm6, [rsp + 02f0h]
movdqa xmm7, [rsp + 0300h]
add rsp, 80*4*2+12*4+8*8 + 2*16+8
pop		rdi
pop		rsi
ret
Sosemanuk_OperateKeystream ENDP

Panama_SSE2_Pull PROC FRAME
rex_push_reg rdi
alloc_stack(2*16)
save_xmm128 xmm6, 0h
save_xmm128 xmm7, 10h
.endprolog
shl rcx, 5
jz label5
mov r10d, [rdx+4*17]
add rcx, r10
mov rdi, rcx
movdqa xmm0, xmmword ptr [rdx+0*16]
movdqa xmm1, xmmword ptr [rdx+1*16]
movdqa xmm2, xmmword ptr [rdx+2*16]
movdqa xmm3, xmmword ptr [rdx+3*16]
mov eax, dword ptr [rdx+4*16]
label4:
movdqa xmm6, xmm2
movss xmm6, xmm3
pshufd xmm5, xmm6, 0*64+3*16+2*4+1
movd xmm6, eax
movdqa xmm7, xmm3
movss xmm7, xmm6
pshufd xmm6, xmm7, 0*64+3*16+2*4+1
movd ecx, xmm2
not ecx
movd r11d, xmm3
or ecx, r11d
xor eax, ecx
pcmpeqb xmm7, xmm7
pxor xmm7, xmm1
por xmm7, xmm2
pxor xmm7, xmm3
movd ecx, xmm7
rol ecx, (((((5*1) MOD (17))*(((5*1) MOD (17))+1)/2)) MOD (32))
mov [rdx+((((((5*(1)) MOD (17)))*13+16)) MOD (17))*4], ecx
pshuflw xmm7, xmm7, 1*64+0*16+3*4+2
movd ecx, xmm7
rol ecx, (((((5*5) MOD (17))*(((5*5) MOD (17))+1)/2)) MOD (32))
mov [rdx+((((((5*(5)) MOD (17)))*13+16)) MOD (17))*4], ecx
punpckhqdq xmm7, xmm7
movd ecx, xmm7
rol ecx, (((((5*9) MOD (17))*(((5*9) MOD (17))+1)/2)) MOD (32))
mov [rdx+((((((5*(9)) MOD (17)))*13+16)) MOD (17))*4], ecx
pshuflw xmm7, xmm7, 1*64+0*16+3*4+2
movd ecx, xmm7
rol ecx, (((((5*13) MOD (17))*(((5*13) MOD (17))+1)/2)) MOD (32))
mov [rdx+((((((5*(13)) MOD (17)))*13+16)) MOD (17))*4], ecx
pcmpeqb xmm7, xmm7
pxor xmm7, xmm0
por xmm7, xmm1
pxor xmm7, xmm2
movd ecx, xmm7
rol ecx, (((((5*2) MOD (17))*(((5*2) MOD (17))+1)/2)) MOD (32))
mov [rdx+((((((5*(2)) MOD (17)))*13+16)) MOD (17))*4], ecx
pshuflw xmm7, xmm7, 1*64+0*16+3*4+2
movd ecx, xmm7
rol ecx, (((((5*6) MOD (17))*(((5*6) MOD (17))+1)/2)) MOD (32))
mov [rdx+((((((5*(6)) MOD (17)))*13+16)) MOD (17))*4], ecx
punpckhqdq xmm7, xmm7
movd ecx, xmm7
rol ecx, (((((5*10) MOD (17))*(((5*10) MOD (17))+1)/2)) MOD (32))
mov [rdx+((((((5*(10)) MOD (17)))*13+16)) MOD (17))*4], ecx
pshuflw xmm7, xmm7, 1*64+0*16+3*4+2
movd ecx, xmm7
rol ecx, (((((5*14) MOD (17))*(((5*14) MOD (17))+1)/2)) MOD (32))
mov [rdx+((((((5*(14)) MOD (17)))*13+16)) MOD (17))*4], ecx
pcmpeqb xmm7, xmm7
pxor xmm7, xmm6
por xmm7, xmm0
pxor xmm7, xmm1
movd ecx, xmm7
rol ecx, (((((5*3) MOD (17))*(((5*3) MOD (17))+1)/2)) MOD (32))
mov [rdx+((((((5*(3)) MOD (17)))*13+16)) MOD (17))*4], ecx
pshuflw xmm7, xmm7, 1*64+0*16+3*4+2
movd ecx, xmm7
rol ecx, (((((5*7) MOD (17))*(((5*7) MOD (17))+1)/2)) MOD (32))
mov [rdx+((((((5*(7)) MOD (17)))*13+16)) MOD (17))*4], ecx
punpckhqdq xmm7, xmm7
movd ecx, xmm7
rol ecx, (((((5*11) MOD (17))*(((5*11) MOD (17))+1)/2)) MOD (32))
mov [rdx+((((((5*(11)) MOD (17)))*13+16)) MOD (17))*4], ecx
pshuflw xmm7, xmm7, 1*64+0*16+3*4+2
movd ecx, xmm7
rol ecx, (((((5*15) MOD (17))*(((5*15) MOD (17))+1)/2)) MOD (32))
mov [rdx+((((((5*(15)) MOD (17)))*13+16)) MOD (17))*4], ecx
pcmpeqb xmm7, xmm7
pxor xmm7, xmm5
por xmm7, xmm6
pxor xmm7, xmm0
movd ecx, xmm7
rol ecx, (((((5*4) MOD (17))*(((5*4) MOD (17))+1)/2)) MOD (32))
mov [rdx+((((((5*(4)) MOD (17)))*13+16)) MOD (17))*4], ecx
pshuflw xmm7, xmm7, 1*64+0*16+3*4+2
movd ecx, xmm7
rol ecx, (((((5*8) MOD (17))*(((5*8) MOD (17))+1)/2)) MOD (32))
mov [rdx+((((((5*(8)) MOD (17)))*13+16)) MOD (17))*4], ecx
punpckhqdq xmm7, xmm7
movd ecx, xmm7
rol ecx, (((((5*12) MOD (17))*(((5*12) MOD (17))+1)/2)) MOD (32))
mov [rdx+((((((5*(12)) MOD (17)))*13+16)) MOD (17))*4], ecx
pshuflw xmm7, xmm7, 1*64+0*16+3*4+2
movd ecx, xmm7
rol ecx, (((((5*16) MOD (17))*(((5*16) MOD (17))+1)/2)) MOD (32))
mov [rdx+((((((5*(16)) MOD (17)))*13+16)) MOD (17))*4], ecx
movdqa xmm4, xmm3
punpcklqdq xmm3, xmm2
punpckhdq xmm4, xmm2
movdqa xmm2, xmm1
punpcklqdq xmm1, xmm0
punpckhdq xmm2, xmm0
test r8, r8
jz label0
movdqa xmm6, xmm4
punpcklqdq xmm4, xmm2
punpckhqdq xmm6, xmm2
test r9, 15
jnz label2
test r9, r9
jz label1
pxor xmm4, [r9]
pxor xmm6, [r9+16]
add r9, 32
jmp label1
label2:
movdqu xmm0, [r9]
movdqu xmm2, [r9+16]
pxor xmm4, xmm0
pxor xmm6, xmm2
add r9, 32
label1:
test r8, 15
jnz label3
movdqa xmmword ptr [r8], xmm4
movdqa xmmword ptr [r8+16], xmm6
add r8, 32
jmp label0
label3:
movdqu xmmword ptr [r8], xmm4
movdqu xmmword ptr [r8+16], xmm6
add r8, 32
label0:
lea rcx, [r10 + 32]
and rcx, 31*32
lea r11, [r10 + (32-24)*32]
and r11, 31*32
movdqa xmm0, xmmword ptr [rdx+20*4+rcx+0*8]
pxor xmm3, xmm0
pshufd xmm0, xmm0, 2*64+3*16+0*4+1
movdqa xmmword ptr [rdx+20*4+rcx+0*8], xmm3
pxor xmm0, xmmword ptr [rdx+20*4+r11+2*8]
movdqa xmmword ptr [rdx+20*4+r11+2*8], xmm0
movdqa xmm4, xmmword ptr [rdx+20*4+rcx+2*8]
pxor xmm1, xmm4
movdqa xmmword ptr [rdx+20*4+rcx+2*8], xmm1
pxor xmm4, xmmword ptr [rdx+20*4+r11+0*8]
movdqa xmmword ptr [rdx+20*4+r11+0*8], xmm4
movdqa xmm3, xmmword ptr [rdx+3*16]
movdqa xmm2, xmmword ptr [rdx+2*16]
movdqa xmm1, xmmword ptr [rdx+1*16]
movdqa xmm0, xmmword ptr [rdx+0*16]
movd xmm6, eax
movdqa xmm7, xmm3
movss xmm7, xmm6
movdqa xmm6, xmm2
movss xmm6, xmm3
movdqa xmm5, xmm1
movss xmm5, xmm2
movdqa xmm4, xmm0
movss xmm4, xmm1
pshufd xmm7, xmm7, 0*64+3*16+2*4+1
pshufd xmm6, xmm6, 0*64+3*16+2*4+1
pshufd xmm5, xmm5, 0*64+3*16+2*4+1
pshufd xmm4, xmm4, 0*64+3*16+2*4+1
xor eax, 1
movd ecx, xmm0
xor eax, ecx
movd ecx, xmm3
xor eax, ecx
pxor xmm3, xmm2
pxor xmm2, xmm1
pxor xmm1, xmm0
pxor xmm0, xmm7
pxor xmm3, xmm7
pxor xmm2, xmm6
pxor xmm1, xmm5
pxor xmm0, xmm4
lea rcx, [r10 + (32-4)*32]
and rcx, 31*32
lea r11, [r10 + 16*32]
and r11, 31*32
movdqa xmm4, xmmword ptr [rdx+20*4+rcx+0*16]
movdqa xmm5, xmmword ptr [rdx+20*4+r11+0*16]
movdqa xmm6, xmm4
punpcklqdq xmm4, xmm5
punpckhqdq xmm6, xmm5
pxor xmm3, xmm4
pxor xmm2, xmm6
movdqa xmm4, xmmword ptr [rdx+20*4+rcx+1*16]
movdqa xmm5, xmmword ptr [rdx+20*4+r11+1*16]
movdqa xmm6, xmm4
punpcklqdq xmm4, xmm5
punpckhqdq xmm6, xmm5
pxor xmm1, xmm4
pxor xmm0, xmm6
add r10, 32
cmp r10, rdi
jne label4
mov [rdx+4*16], eax
movdqa xmmword ptr [rdx+3*16], xmm3
movdqa xmmword ptr [rdx+2*16], xmm2
movdqa xmmword ptr [rdx+1*16], xmm1
movdqa xmmword ptr [rdx+0*16], xmm0
label5:
movdqa xmm6, [rsp + 0h]
movdqa xmm7, [rsp + 10h]
add rsp, 2*16
pop	rdi
ret
Panama_SSE2_Pull ENDP

_TEXT ENDS
END
