/* x86 */
#if defined(X86ASM_AVX) && (!defined(SCRYPT_CHOOSE_COMPILETIME) || !defined(SCRYPT_CHACHA_INCLUDED))

#define SCRYPT_CHACHA_AVX

asm_naked_fn_proto(void, scrypt_ChunkMix_avx)(uint32_t *Bout/*[chunkBytes]*/, uint32_t *Bin/*[chunkBytes]*/, uint32_t *Bxor/*[chunkBytes]*/, uint32_t r)
asm_naked_fn(scrypt_ChunkMix_avx)
	a1(push ebx)
	a1(push edi)
	a1(push esi)
	a1(push ebp)
	a2(mov ebp,esp)
	a2(mov edi,[ebp+20])
	a2(mov esi,[ebp+24])
	a2(mov eax,[ebp+28])
	a2(mov ebx,[ebp+32])
	a2(sub esp,64)
	a2(and esp,~63)
	a2(lea edx,[ebx*2])
	a2(shl edx,6)
	a2(lea ecx,[edx-64])
	a2(and eax, eax)
	a2(vmovdqa xmm4,[ssse3_rotl16_32bit])
	a2(vmovdqa xmm5,[ssse3_rotl8_32bit])
	a2(vmovdqa xmm0,[ecx+esi+0])
	a2(vmovdqa xmm1,[ecx+esi+16])
	a2(vmovdqa xmm2,[ecx+esi+32])
	a2(vmovdqa xmm3,[ecx+esi+48])
	a1(jz scrypt_ChunkMix_avx_no_xor1)
	a3(vpxor xmm0,xmm0,[ecx+eax+0])
	a3(vpxor xmm1,xmm1,[ecx+eax+16])
	a3(vpxor xmm2,xmm2,[ecx+eax+32])
	a3(vpxor xmm3,xmm3,[ecx+eax+48])
	a1(scrypt_ChunkMix_avx_no_xor1:)
	a2(xor ecx,ecx)
	a2(xor ebx,ebx)
	a1(scrypt_ChunkMix_avx_loop:)
		a2(and eax, eax)
		a3(vpxor xmm0,xmm0,[esi+ecx+0])
		a3(vpxor xmm1,xmm1,[esi+ecx+16])
		a3(vpxor xmm2,xmm2,[esi+ecx+32])
		a3(vpxor xmm3,xmm3,[esi+ecx+48])
		a1(jz scrypt_ChunkMix_avx_no_xor2)
		a3(vpxor xmm0,xmm0,[eax+ecx+0])
		a3(vpxor xmm1,xmm1,[eax+ecx+16])
		a3(vpxor xmm2,xmm2,[eax+ecx+32])
		a3(vpxor xmm3,xmm3,[eax+ecx+48])
		a1(scrypt_ChunkMix_avx_no_xor2:)
		a2(vmovdqa [esp+0],xmm0)
		a2(vmovdqa [esp+16],xmm1)
		a2(vmovdqa [esp+32],xmm2)
		a2(vmovdqa [esp+48],xmm3)
		a2(mov eax,8)
		a1(scrypt_chacha_avx_loop: )
			a3(vpaddd xmm0,xmm0,xmm1)
			a3(vpxor  xmm3,xmm3,xmm0)
			a3(vpshufb xmm3,xmm3,xmm4)
			a3(vpaddd xmm2,xmm2,xmm3)
			a3(vpxor  xmm1,xmm1,xmm2)
			a3(vpsrld xmm6,xmm1,20)
			a3(vpslld xmm1,xmm1,12)
			a3(vpxor  xmm1,xmm1,xmm6)
			a3(vpaddd xmm0,xmm0,xmm1)
			a3(vpxor  xmm3,xmm3,xmm0)
			a3(vpshufb xmm3,xmm3,xmm5)
			a3(vpshufd xmm0,xmm0,0x93)
			a3(vpaddd xmm2,xmm2,xmm3)
			a3(vpshufd xmm3,xmm3,0x4e)
			a3(vpxor xmm1,xmm1,xmm2)
			a3(vpshufd xmm2,xmm2,0x39)
			a3(vpsrld xmm6,xmm1,25)
			a3(vpslld xmm1,xmm1,7)
			a3(vpxor xmm1,xmm1,xmm6)
			a2(sub eax,2)
			a3(vpaddd xmm0,xmm0,xmm1)
			a3(vpxor  xmm3,xmm3,xmm0)
			a3(vpshufb xmm3,xmm3,xmm4)
			a3(vpaddd xmm2,xmm2,xmm3)
			a3(vpxor  xmm1,xmm1,xmm2)
			a3(vpsrld xmm6,xmm1,20)
			a3(vpslld xmm1,xmm1,12)
			a3(vpxor xmm1,xmm1,xmm6)
			a3(vpaddd xmm0,xmm0,xmm1)
			a3(vpxor  xmm3,xmm3,xmm0)
			a3(vpshufb xmm3,xmm3,xmm5)
			a3(vpshufd xmm0,xmm0,0x39)
			a3(vpaddd xmm2,xmm2,xmm3)
			a3(pshufd xmm3,xmm3,0x4e)
			a3(vpxor  xmm1,xmm1,xmm2)
			a3(pshufd xmm2,xmm2,0x93)
			a3(vpsrld xmm6,xmm1,25)
			a3(vpslld xmm1,xmm1,7)
			a3(vpxor  xmm1,xmm1,xmm6)
			a1(ja scrypt_chacha_avx_loop)
		a3(vpaddd xmm0,xmm0,[esp+0])
		a3(vpaddd xmm1,xmm1,[esp+16])
		a3(vpaddd xmm2,xmm2,[esp+32])
		a3(vpaddd xmm3,xmm3,[esp+48])
		a2(lea eax,[ebx+ecx])
		a2(xor ebx,edx)
		a2(and eax,~0x7f)
		a2(add ecx,64)
		a2(shr eax,1)
		a2(add eax, edi)
		a2(cmp ecx,edx)
		a2(vmovdqa [eax+0],xmm0)
		a2(vmovdqa [eax+16],xmm1)
		a2(vmovdqa [eax+32],xmm2)
		a2(vmovdqa [eax+48],xmm3)
		a2(mov eax,[ebp+28])
		a1(jne scrypt_ChunkMix_avx_loop)
	a2(mov esp,ebp)
	a1(pop ebp)
	a1(pop esi)
	a1(pop edi)
	a1(pop ebx)
	a1(ret 16)
asm_naked_fn_end(scrypt_ChunkMix_avx)

#endif



/* x64 */
#if defined(X86_64ASM_AVX) && (!defined(SCRYPT_CHOOSE_COMPILETIME) || !defined(SCRYPT_CHACHA_INCLUDED))

#define SCRYPT_CHACHA_AVX

asm_naked_fn_proto(void, scrypt_ChunkMix_avx)(uint32_t *Bout/*[chunkBytes]*/, uint32_t *Bin/*[chunkBytes]*/, uint32_t *Bxor/*[chunkBytes]*/, uint32_t r)
asm_naked_fn(scrypt_ChunkMix_avx)
	a2(lea rcx,[rcx*2])
	a2(shl rcx,6)
	a2(lea r9,[rcx-64])
	a2(lea rax,[rsi+r9])
	a2(lea r9,[rdx+r9])
	a2(and rdx, rdx)
	a2(vmovdqa xmm4,[ssse3_rotl16_32bit])
	a2(vmovdqa xmm5,[ssse3_rotl8_32bit])
	a2(vmovdqa xmm0,[rax+0])
	a2(vmovdqa xmm1,[rax+16])
	a2(vmovdqa xmm2,[rax+32])
	a2(vmovdqa xmm3,[rax+48])
	a1(jz scrypt_ChunkMix_avx_no_xor1)
	a3(vpxor xmm0,xmm0,[r9+0])
	a3(vpxor xmm1,xmm1,[r9+16])
	a3(vpxor xmm2,xmm2,[r9+32])
	a3(vpxor xmm3,xmm3,[r9+48])
	a1(scrypt_ChunkMix_avx_no_xor1:)
	a2(xor r8,r8)
	a2(xor r9,r9)
	a1(scrypt_ChunkMix_avx_loop:)
		a2(and rdx, rdx)
		a3(vpxor xmm0,xmm0,[rsi+r9+0])
		a3(vpxor xmm1,xmm1,[rsi+r9+16])
		a3(vpxor xmm2,xmm2,[rsi+r9+32])
		a3(vpxor xmm3,xmm3,[rsi+r9+48])
		a1(jz scrypt_ChunkMix_avx_no_xor2)
		a3(vpxor xmm0,xmm0,[rdx+r9+0])
		a3(vpxor xmm1,xmm1,[rdx+r9+16])
		a3(vpxor xmm2,xmm2,[rdx+r9+32])
		a3(vpxor xmm3,xmm3,[rdx+r9+48])
		a1(scrypt_ChunkMix_avx_no_xor2:)
		a2(vmovdqa xmm8,xmm0)
		a2(vmovdqa xmm9,xmm1)
		a2(vmovdqa xmm10,xmm2)
		a2(vmovdqa xmm11,xmm3)
		a2(mov rax,8)
		a1(scrypt_chacha_avx_loop: )
			a3(vpaddd xmm0,xmm0,xmm1)
			a3(vpxor  xmm3,xmm3,xmm0)
			a3(vpshufb xmm3,xmm3,xmm4)
			a3(vpaddd xmm2,xmm2,xmm3)
			a3(vpxor  xmm1,xmm1,xmm2)
			a3(vpsrld xmm12,xmm1,20)
			a3(vpslld xmm1,xmm1,12)
			a3(vpxor  xmm1,xmm1,xmm12)
			a3(vpaddd xmm0,xmm0,xmm1)
			a3(vpxor  xmm3,xmm3,xmm0)
			a3(vpshufb xmm3,xmm3,xmm5)
			a3(vpshufd xmm0,xmm0,0x93)
			a3(vpaddd xmm2,xmm2,xmm3)
			a3(vpshufd xmm3,xmm3,0x4e)
			a3(vpxor xmm1,xmm1,xmm2)
			a3(vpshufd xmm2,xmm2,0x39)
			a3(vpsrld xmm12,xmm1,25)
			a3(vpslld xmm1,xmm1,7)
			a3(vpxor xmm1,xmm1,xmm12)
			a2(sub rax,2)
			a3(vpaddd xmm0,xmm0,xmm1)
			a3(vpxor  xmm3,xmm3,xmm0)
			a3(vpshufb xmm3,xmm3,xmm4)
			a3(vpaddd xmm2,xmm2,xmm3)
			a3(vpxor  xmm1,xmm1,xmm2)
			a3(vpsrld xmm12,xmm1,20)
			a3(vpslld xmm1,xmm1,12)
			a3(vpxor xmm1,xmm1,xmm12)
			a3(vpaddd xmm0,xmm0,xmm1)
			a3(vpxor  xmm3,xmm3,xmm0)
			a3(vpshufb xmm3,xmm3,xmm5)
			a3(vpshufd xmm0,xmm0,0x39)
			a3(vpaddd xmm2,xmm2,xmm3)
			a3(pshufd xmm3,xmm3,0x4e)
			a3(vpxor  xmm1,xmm1,xmm2)
			a3(pshufd xmm2,xmm2,0x93)
			a3(vpsrld xmm12,xmm1,25)
			a3(vpslld xmm1,xmm1,7)
			a3(vpxor  xmm1,xmm1,xmm12)
			a1(ja scrypt_chacha_avx_loop)
		a3(vpaddd xmm0,xmm0,xmm8)
		a3(vpaddd xmm1,xmm1,xmm9)
		a3(vpaddd xmm2,xmm2,xmm10)
		a3(vpaddd xmm3,xmm3,xmm11)
		a2(lea rax,[r8+r9])
		a2(xor r8,rcx)
		a2(and rax,~0x7f)
		a2(add r9,64)
		a2(shr rax,1)
		a2(add rax, rdi)
		a2(cmp r9,rcx)
		a2(vmovdqa [rax+0],xmm0)
		a2(vmovdqa [rax+16],xmm1)
		a2(vmovdqa [rax+32],xmm2)
		a2(vmovdqa [rax+48],xmm3)
		a1(jne scrypt_ChunkMix_avx_loop)
	a1(ret)
asm_naked_fn_end(scrypt_ChunkMix_avx)

#endif


/* intrinsic */
#if defined(X86_INTRINSIC_AVX) && (!defined(SCRYPT_CHOOSE_COMPILETIME) || !defined(SCRYPT_CHACHA_INCLUDED))

#define SCRYPT_CHACHA_AVX

static void NOINLINE
scrypt_ChunkMix_avx(uint32_t *Bout/*[chunkBytes]*/, uint32_t *Bin/*[chunkBytes]*/, uint32_t *Bxor/*[chunkBytes]*/, uint32_t r) {
	uint32_t i, blocksPerChunk = r * 2, half = 0;
	xmmi *xmmp,x0,x1,x2,x3,x6,t0,t1,t2,t3;
	const xmmi x4 = *(xmmi *)&ssse3_rotl16_32bit, x5 = *(xmmi *)&ssse3_rotl8_32bit;
	size_t rounds;

	/* 1: X = B_{2r - 1} */
	xmmp = (xmmi *)scrypt_block(Bin, blocksPerChunk - 1);
	x0 = xmmp[0];
	x1 = xmmp[1];
	x2 = xmmp[2];
	x3 = xmmp[3];

	if (Bxor) {
		xmmp = (xmmi *)scrypt_block(Bxor, blocksPerChunk - 1);
		x0 = _mm_xor_si128(x0, xmmp[0]);
		x1 = _mm_xor_si128(x1, xmmp[1]);
		x2 = _mm_xor_si128(x2, xmmp[2]);
		x3 = _mm_xor_si128(x3, xmmp[3]);
	}

	/* 2: for i = 0 to 2r - 1 do */
	for (i = 0; i < blocksPerChunk; i++, half ^= r) {
		/* 3: X = H(X ^ B_i) */
		xmmp = (xmmi *)scrypt_block(Bin, i);
		x0 = _mm_xor_si128(x0, xmmp[0]);
		x1 = _mm_xor_si128(x1, xmmp[1]);
		x2 = _mm_xor_si128(x2, xmmp[2]);
		x3 = _mm_xor_si128(x3, xmmp[3]);

		if (Bxor) {
			xmmp = (xmmi *)scrypt_block(Bxor, i);
			x0 = _mm_xor_si128(x0, xmmp[0]);
			x1 = _mm_xor_si128(x1, xmmp[1]);
			x2 = _mm_xor_si128(x2, xmmp[2]);
			x3 = _mm_xor_si128(x3, xmmp[3]);
		}

		t0 = x0;
		t1 = x1;
		t2 = x2;
		t3 = x3;

		for (rounds = 8; rounds; rounds -= 2) {
			x0 = _mm_add_epi32(x0, x1);
			x3 = _mm_xor_si128(x3, x0);
			x3 = _mm_shuffle_epi8(x3, x4);
			x2 = _mm_add_epi32(x2, x3);
			x1 = _mm_xor_si128(x1, x2);
			x6 = x1;
			x1 = _mm_or_si128(_mm_slli_epi32(x1, 12), _mm_srli_epi32(x6, 20));
			x0 = _mm_add_epi32(x0, x1);
			x3 = _mm_xor_si128(x3, x0);
			x3 = _mm_shuffle_epi8(x3, x5);
			x0 = _mm_shuffle_epi32(x0, 0x93);
			x2 = _mm_add_epi32(x2, x3);
			x3 = _mm_shuffle_epi32(x3, 0x4e);
			x1 = _mm_xor_si128(x1, x2);
			x2 = _mm_shuffle_epi32(x2, 0x39);
			x6 = x1;
			x1 = _mm_or_si128(_mm_slli_epi32(x1, 7), _mm_srli_epi32(x6, 25));
			x0 = _mm_add_epi32(x0, x1);
			x3 = _mm_xor_si128(x3, x0);
			x3 = _mm_shuffle_epi8(x3, x4);
			x2 = _mm_add_epi32(x2, x3);
			x1 = _mm_xor_si128(x1, x2);
			x6 = x1;
			x1 = _mm_or_si128(_mm_slli_epi32(x1, 12), _mm_srli_epi32(x6, 20));
			x0 = _mm_add_epi32(x0, x1);
			x3 = _mm_xor_si128(x3, x0);
			x3 = _mm_shuffle_epi8(x3, x5);
			x0 = _mm_shuffle_epi32(x0, 0x39);
			x2 = _mm_add_epi32(x2, x3);
			x3 = _mm_shuffle_epi32(x3, 0x4e);
			x1 = _mm_xor_si128(x1, x2);
			x2 = _mm_shuffle_epi32(x2, 0x93);
			x6 = x1;
			x1 = _mm_or_si128(_mm_slli_epi32(x1, 7), _mm_srli_epi32(x6, 25));
		}

		x0 = _mm_add_epi32(x0, t0);
		x1 = _mm_add_epi32(x1, t1);
		x2 = _mm_add_epi32(x2, t2);
		x3 = _mm_add_epi32(x3, t3);

		/* 4: Y_i = X */
		/* 6: B'[0..r-1] = Y_even */
		/* 6: B'[r..2r-1] = Y_odd */
		xmmp = (xmmi *)scrypt_block(Bout, (i / 2) + half);
		xmmp[0] = x0;
		xmmp[1] = x1;
		xmmp[2] = x2;
		xmmp[3] = x3;
	}
}

#endif

#if defined(SCRYPT_CHACHA_AVX)
	#undef SCRYPT_MIX
	#define SCRYPT_MIX "ChaCha/8-AVX"
	#undef SCRYPT_CHACHA_INCLUDED
	#define SCRYPT_CHACHA_INCLUDED
#endif
