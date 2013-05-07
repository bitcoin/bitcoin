#define SCRYPT_HASH "SHA-2-256"
#define SCRYPT_HASH_BLOCK_SIZE 64
#define SCRYPT_HASH_DIGEST_SIZE 32

typedef uint8_t scrypt_hash_digest[SCRYPT_HASH_DIGEST_SIZE];

typedef struct scrypt_hash_state_t {
	uint32_t H[8];
	uint64_t T;
	uint32_t leftover;
	uint8_t buffer[SCRYPT_HASH_BLOCK_SIZE];
} scrypt_hash_state;

static const uint32_t sha256_constants[64] = {
	0x428a2f98, 0x71374491, 0xb5c0fbcf, 0xe9b5dba5, 0x3956c25b, 0x59f111f1, 0x923f82a4, 0xab1c5ed5,
	0xd807aa98, 0x12835b01, 0x243185be, 0x550c7dc3, 0x72be5d74, 0x80deb1fe, 0x9bdc06a7, 0xc19bf174,
	0xe49b69c1, 0xefbe4786, 0x0fc19dc6, 0x240ca1cc, 0x2de92c6f, 0x4a7484aa, 0x5cb0a9dc, 0x76f988da,
	0x983e5152, 0xa831c66d, 0xb00327c8, 0xbf597fc7, 0xc6e00bf3, 0xd5a79147, 0x06ca6351, 0x14292967,
	0x27b70a85, 0x2e1b2138, 0x4d2c6dfc, 0x53380d13, 0x650a7354, 0x766a0abb, 0x81c2c92e, 0x92722c85,
	0xa2bfe8a1, 0xa81a664b, 0xc24b8b70, 0xc76c51a3, 0xd192e819, 0xd6990624, 0xf40e3585, 0x106aa070,
	0x19a4c116, 0x1e376c08, 0x2748774c, 0x34b0bcb5, 0x391c0cb3, 0x4ed8aa4a, 0x5b9cca4f, 0x682e6ff3,
	0x748f82ee, 0x78a5636f, 0x84c87814, 0x8cc70208, 0x90befffa, 0xa4506ceb, 0xbef9a3f7, 0xc67178f2
};

#define Ch(x,y,z)  (z ^ (x & (y ^ z)))
#define Maj(x,y,z) (((x | y) & z) | (x & y))
#define S0(x)      (ROTR32(x,  2) ^ ROTR32(x, 13) ^ ROTR32(x, 22))
#define S1(x)      (ROTR32(x,  6) ^ ROTR32(x, 11) ^ ROTR32(x, 25))
#define G0(x)      (ROTR32(x,  7) ^ ROTR32(x, 18) ^ (x >>  3))
#define G1(x)      (ROTR32(x, 17) ^ ROTR32(x, 19) ^ (x >> 10))
#define W0(in,i)   (U8TO32_BE(&in[i * 4]))
#define W1(i)      (G1(w[i - 2]) + w[i - 7] + G0(w[i - 15]) + w[i - 16])
#define STEP(i) \
	t1 = S0(r[0]) + Maj(r[0], r[1], r[2]); \
	t0 = r[7] + S1(r[4]) + Ch(r[4], r[5], r[6]) + sha256_constants[i] + w[i]; \
	r[7] = r[6]; \
	r[6] = r[5]; \
	r[5] = r[4]; \
	r[4] = r[3] + t0; \
	r[3] = r[2]; \
	r[2] = r[1]; \
	r[1] = r[0]; \
	r[0] = t0 + t1;

static void
sha256_blocks(scrypt_hash_state *S, const uint8_t *in, size_t blocks) {
	uint32_t r[8], w[64], t0, t1;
	size_t i;

	for (i = 0; i < 8; i++) r[i] = S->H[i];

	while (blocks--) {
		for (i =  0; i < 16; i++) { w[i] = W0(in, i); }
		for (i = 16; i < 64; i++) { w[i] = W1(i); }
		for (i =  0; i < 64; i++) { STEP(i); }
		for (i =  0; i <  8; i++) { r[i] += S->H[i]; S->H[i] = r[i]; }
		S->T += SCRYPT_HASH_BLOCK_SIZE * 8;
		in += SCRYPT_HASH_BLOCK_SIZE;
	}
}

static void
scrypt_hash_init(scrypt_hash_state *S) {
	S->H[0] = 0x6a09e667;
	S->H[1] = 0xbb67ae85;
	S->H[2] = 0x3c6ef372;
	S->H[3] = 0xa54ff53a;
	S->H[4] = 0x510e527f;
	S->H[5] = 0x9b05688c;
	S->H[6] = 0x1f83d9ab;
	S->H[7] = 0x5be0cd19;
	S->T = 0;
	S->leftover = 0;
}

static void
scrypt_hash_update(scrypt_hash_state *S, const uint8_t *in, size_t inlen) {
	size_t blocks, want;

	/* handle the previous data */
	if (S->leftover) {
		want = (SCRYPT_HASH_BLOCK_SIZE - S->leftover);
		want = (want < inlen) ? want : inlen;
		memcpy(S->buffer + S->leftover, in, want);
		S->leftover += (uint32_t)want;
		if (S->leftover < SCRYPT_HASH_BLOCK_SIZE)
			return;
		in += want;
		inlen -= want;
		sha256_blocks(S, S->buffer, 1);
	}

	/* handle the current data */
	blocks = (inlen & ~(SCRYPT_HASH_BLOCK_SIZE - 1));
	S->leftover = (uint32_t)(inlen - blocks);
	if (blocks) {
		sha256_blocks(S, in, blocks / SCRYPT_HASH_BLOCK_SIZE);
		in += blocks;
	}

	/* handle leftover data */
	if (S->leftover)
		memcpy(S->buffer, in, S->leftover);
}

static void
scrypt_hash_finish(scrypt_hash_state *S, uint8_t *hash) {
	uint64_t t = S->T + (S->leftover * 8);

	S->buffer[S->leftover] = 0x80;
	if (S->leftover <= 55) {
		memset(S->buffer + S->leftover + 1, 0, 55 - S->leftover);
	} else {
		memset(S->buffer + S->leftover + 1, 0, 63 - S->leftover);
		sha256_blocks(S, S->buffer, 1);
		memset(S->buffer, 0, 56);
	}

	U64TO8_BE(S->buffer + 56, t);
	sha256_blocks(S, S->buffer, 1);

	U32TO8_BE(&hash[ 0], S->H[0]);
	U32TO8_BE(&hash[ 4], S->H[1]);
	U32TO8_BE(&hash[ 8], S->H[2]);
	U32TO8_BE(&hash[12], S->H[3]);
	U32TO8_BE(&hash[16], S->H[4]);
	U32TO8_BE(&hash[20], S->H[5]);
	U32TO8_BE(&hash[24], S->H[6]);
	U32TO8_BE(&hash[28], S->H[7]);
}

static const uint8_t scrypt_test_hash_expected[SCRYPT_HASH_DIGEST_SIZE] = {
	0xee,0x36,0xae,0xa6,0x65,0xf0,0x28,0x7d,0xc9,0xde,0xd8,0xad,0x48,0x33,0x7d,0xbf,
	0xcb,0xc0,0x48,0xfa,0x5f,0x92,0xfd,0x0a,0x95,0x6f,0x34,0x8e,0x8c,0x1e,0x73,0xad,
};
