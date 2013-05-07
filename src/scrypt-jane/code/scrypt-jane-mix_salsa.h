#if !defined(SCRYPT_CHOOSE_COMPILETIME) || !defined(SCRYPT_SALSA_INCLUDED)

#undef SCRYPT_MIX
#define SCRYPT_MIX "Salsa20/8 Ref"

#undef SCRYPT_SALSA_INCLUDED
#define SCRYPT_SALSA_INCLUDED
#define SCRYPT_SALSA_BASIC

static void
salsa_core_basic(uint32_t state[16]) {
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
		t = a+d; t = ROTL32(t,  7); b ^= t; \
		t = b+a; t = ROTL32(t,  9); c ^= t; \
		t = c+b; t = ROTL32(t, 13); d ^= t; \
		t = d+c; t = ROTL32(t, 18); a ^= t; \

	for (; rounds; rounds -= 2) {
		quarter( x0, x4, x8,x12)
		quarter( x5, x9,x13, x1)
		quarter(x10,x14, x2, x6)
		quarter(x15, x3, x7,x11)
		quarter( x0, x1, x2, x3)
		quarter( x5, x6, x7, x4)
		quarter(x10,x11, x8, x9)
		quarter(x15,x12,x13,x14)
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

