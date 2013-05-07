#if !defined(SCRYPT_CHOOSE_COMPILETIME) || !defined(SCRYPT_CHACHA_INCLUDED)

#undef SCRYPT_MIX
#define SCRYPT_MIX "ChaCha20/8 Ref"

#undef SCRYPT_CHACHA_INCLUDED
#define SCRYPT_CHACHA_INCLUDED
#define SCRYPT_CHACHA_BASIC

static void
chacha_core_basic(uint32_t state[16]) {
	size_t rounds = 8;
	uint32_t x0,x1,x2,x3,x4,x5,x6,x7,x8,x9,x10,x11,x12,x13,x14,x15,t;

	x0 = state[0];
	x1 = state[1];
	x2 = state[2];
	x3 = state[3];
	x4 = state[4];
	x5 = state[5];
	x6 = state[6];
	x7 = state[7];
	x8 = state[8];
	x9 = state[9];
	x10 = state[10];
	x11 = state[11];
	x12 = state[12];
	x13 = state[13];
	x14 = state[14];
	x15 = state[15];

	#define quarter(a,b,c,d) \
		a += b; t = d^a; d = ROTL32(t,16); \
		c += d; t = b^c; b = ROTL32(t,12); \
		a += b; t = d^a; d = ROTL32(t, 8); \
		c += d; t = b^c; b = ROTL32(t, 7);

	for (; rounds; rounds -= 2) {
		quarter( x0, x4, x8,x12)
		quarter( x1, x5, x9,x13)
		quarter( x2, x6,x10,x14)
		quarter( x3, x7,x11,x15)
		quarter( x0, x5,x10,x15)
		quarter( x1, x6,x11,x12)
		quarter( x2, x7, x8,x13)
		quarter( x3, x4, x9,x14)
	}

	state[0] += x0;
	state[1] += x1;
	state[2] += x2;
	state[3] += x3;
	state[4] += x4;
	state[5] += x5;
	state[6] += x6;
	state[7] += x7;
	state[8] += x8;
	state[9] += x9;
	state[10] += x10;
	state[11] += x11;
	state[12] += x12;
	state[13] += x13;
	state[14] += x14;
	state[15] += x15;

	#undef quarter
}

#endif