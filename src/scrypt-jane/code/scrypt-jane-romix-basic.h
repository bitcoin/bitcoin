#if !defined(SCRYPT_CHOOSE_COMPILETIME)
/* function type returned by scrypt_getROMix, used with cpu detection */
typedef void (FASTCALL *scrypt_ROMixfn)(scrypt_mix_word_t *X/*[chunkWords]*/, scrypt_mix_word_t *Y/*[chunkWords]*/, scrypt_mix_word_t *V/*[chunkWords * N]*/, uint32_t N, uint32_t r);
#endif

/* romix pre/post nop function */
static void STDCALL
scrypt_romix_nop(scrypt_mix_word_t *blocks, size_t nblocks) {
}

/* romix pre/post endian conversion function */
static void STDCALL
scrypt_romix_convert_endian(scrypt_mix_word_t *blocks, size_t nblocks) {
#if !defined(CPU_LE)
	static const union { uint8_t b[2]; uint16_t w; } endian_test = {{1,0}};
	size_t i;
	if (endian_test.w == 0x100) {
		nblocks *= SCRYPT_BLOCK_WORDS;
		for (i = 0; i < nblocks; i++) {
			SCRYPT_WORD_ENDIAN_SWAP(blocks[i]);
		}
	}
#endif
}

/* chunkmix test function */
typedef void (STDCALL *chunkmixfn)(scrypt_mix_word_t *Bout/*[chunkWords]*/, scrypt_mix_word_t *Bin/*[chunkWords]*/, scrypt_mix_word_t *Bxor/*[chunkWords]*/, uint32_t r);
typedef void (STDCALL *blockfixfn)(scrypt_mix_word_t *blocks, size_t nblocks);

static int
scrypt_test_mix_instance(chunkmixfn mixfn, blockfixfn prefn, blockfixfn postfn, const uint8_t expected[16]) {
	/* r = 2, (2 * r) = 4 blocks in a chunk, 4 * SCRYPT_BLOCK_WORDS total */
	const uint32_t r = 2, blocks = 2 * r, words = blocks * SCRYPT_BLOCK_WORDS;
	scrypt_mix_word_t MM16 chunk[2][4 * SCRYPT_BLOCK_WORDS], v;
	uint8_t final[16];
	size_t i;

	for (i = 0; i < words; i++) {
		v = (scrypt_mix_word_t)i;
		v = (v << 8) | v;
		v = (v << 16) | v;
		chunk[0][i] = v;
	}

	prefn(chunk[0], blocks);
	mixfn(chunk[1], chunk[0], NULL, r);
	postfn(chunk[1], blocks);

	/* grab the last 16 bytes of the final block */
	for (i = 0; i < 16; i += sizeof(scrypt_mix_word_t)) {
		SCRYPT_WORDTO8_LE(final + i, chunk[1][words - (16 / sizeof(scrypt_mix_word_t)) + (i / sizeof(scrypt_mix_word_t))]);
	}

	return scrypt_verify(expected, final, 16);
}

/* returns a pointer to item i, where item is len scrypt_mix_word_t's long */
static scrypt_mix_word_t *
scrypt_item(scrypt_mix_word_t *base, scrypt_mix_word_t i, scrypt_mix_word_t len) {
	return base + (i * len);
}

/* returns a pointer to block i */
static scrypt_mix_word_t *
scrypt_block(scrypt_mix_word_t *base, scrypt_mix_word_t i) {
	return base + (i * SCRYPT_BLOCK_WORDS);
}
