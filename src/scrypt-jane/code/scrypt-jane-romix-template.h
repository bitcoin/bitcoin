#if !defined(SCRYPT_CHOOSE_COMPILETIME) || !defined(SCRYPT_HAVE_ROMIX)

#if defined(SCRYPT_CHOOSE_COMPILETIME)
#undef SCRYPT_ROMIX_FN
#define SCRYPT_ROMIX_FN scrypt_ROMix
#endif

#undef SCRYPT_HAVE_ROMIX
#define SCRYPT_HAVE_ROMIX

#if !defined(SCRYPT_CHUNKMIX_FN)

#define SCRYPT_CHUNKMIX_FN scrypt_ChunkMix_basic

/*
	Bout = ChunkMix(Bin)

	2*r: number of blocks in the chunk
*/
static void STDCALL
SCRYPT_CHUNKMIX_FN(scrypt_mix_word_t *Bout/*[chunkWords]*/, scrypt_mix_word_t *Bin/*[chunkWords]*/, scrypt_mix_word_t *Bxor/*[chunkWords]*/, uint32_t r) {
	scrypt_mix_word_t MM16 X[SCRYPT_BLOCK_WORDS], *block;
	uint32_t i, j, blocksPerChunk = r * 2, half = 0;

	/* 1: X = B_{2r - 1} */
	block = scrypt_block(Bin, blocksPerChunk - 1);
	for (i = 0; i < SCRYPT_BLOCK_WORDS; i++)
		X[i] = block[i];

	if (Bxor) {
		block = scrypt_block(Bxor, blocksPerChunk - 1);
		for (i = 0; i < SCRYPT_BLOCK_WORDS; i++)
			X[i] ^= block[i];
	}

	/* 2: for i = 0 to 2r - 1 do */
	for (i = 0; i < blocksPerChunk; i++, half ^= r) {
		/* 3: X = H(X ^ B_i) */
		block = scrypt_block(Bin, i);
		for (j = 0; j < SCRYPT_BLOCK_WORDS; j++)
			X[j] ^= block[j];

		if (Bxor) {
			block = scrypt_block(Bxor, i);
			for (j = 0; j < SCRYPT_BLOCK_WORDS; j++)
				X[j] ^= block[j];
		}
		SCRYPT_MIX_FN(X);

		/* 4: Y_i = X */
		/* 6: B'[0..r-1] = Y_even */
		/* 6: B'[r..2r-1] = Y_odd */
		block = scrypt_block(Bout, (i / 2) + half);
		for (j = 0; j < SCRYPT_BLOCK_WORDS; j++)
			block[j] = X[j];
	}
}
#endif

/*
	X = ROMix(X)

	X: chunk to mix
	Y: scratch chunk
	N: number of rounds
	V[N]: array of chunks to randomly index in to
	2*r: number of blocks in a chunk
*/

static void NOINLINE FASTCALL
SCRYPT_ROMIX_FN(scrypt_mix_word_t *X/*[chunkWords]*/, scrypt_mix_word_t *Y/*[chunkWords]*/, scrypt_mix_word_t *V/*[N * chunkWords]*/, uint32_t N, uint32_t r) {
	uint32_t i, j, chunkWords = SCRYPT_BLOCK_WORDS * r * 2;
	scrypt_mix_word_t *block = V;

	SCRYPT_ROMIX_TANGLE_FN(X, r * 2);

	/* 1: X = B */
	/* implicit */

	/* 2: for i = 0 to N - 1 do */
	memcpy(block, X, chunkWords * sizeof(scrypt_mix_word_t));
	for (i = 0; i < N - 1; i++, block += chunkWords) {
		/* 3: V_i = X */
		/* 4: X = H(X) */
		SCRYPT_CHUNKMIX_FN(block + chunkWords, block, NULL, r);
	}
	SCRYPT_CHUNKMIX_FN(X, block, NULL, r);

	/* 6: for i = 0 to N - 1 do */
	for (i = 0; i < N; i += 2) {
		/* 7: j = Integerify(X) % N */
		j = X[chunkWords - SCRYPT_BLOCK_WORDS] & (N - 1);

		/* 8: X = H(Y ^ V_j) */
		SCRYPT_CHUNKMIX_FN(Y, X, scrypt_item(V, j, chunkWords), r);

		/* 7: j = Integerify(Y) % N */
		j = Y[chunkWords - SCRYPT_BLOCK_WORDS] & (N - 1);

		/* 8: X = H(Y ^ V_j) */
		SCRYPT_CHUNKMIX_FN(X, Y, scrypt_item(V, j, chunkWords), r);
	}

	/* 10: B' = X */
	/* implicit */

	SCRYPT_ROMIX_UNTANGLE_FN(X, r * 2);
}

#endif /* !defined(SCRYPT_CHOOSE_COMPILETIME) || !defined(SCRYPT_HAVE_ROMIX) */


#undef SCRYPT_CHUNKMIX_FN
#undef SCRYPT_ROMIX_FN
#undef SCRYPT_MIX_FN
#undef SCRYPT_ROMIX_TANGLE_FN
#undef SCRYPT_ROMIX_UNTANGLE_FN

