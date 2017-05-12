// cast.cpp - written and placed in the public domain by Wei Dai and Leonard Janke
// based on Steve Reid's public domain cast.c

#include "pch.h"
#include "cast.h"
#include "misc.h"

NAMESPACE_BEGIN(CryptoPP)

/* Macros to access 8-bit bytes out of a 32-bit word */
#define U8a(x) GETBYTE(x,3)
#define U8b(x) GETBYTE(x,2)
#define U8c(x) GETBYTE(x,1)
#define U8d(x) GETBYTE(x,0)

/* CAST uses three different round functions */
#define f1(l, r, km, kr) \
	t = rotlVariable(km + r, kr); \
	l ^= ((S[0][U8a(t)] ^ S[1][U8b(t)]) - \
	 S[2][U8c(t)]) + S[3][U8d(t)];
#define f2(l, r, km, kr) \
	t = rotlVariable(km ^ r, kr); \
	l ^= ((S[0][U8a(t)] - S[1][U8b(t)]) + \
	 S[2][U8c(t)]) ^ S[3][U8d(t)];
#define f3(l, r, km, kr) \
	t = rotlVariable(km - r, kr); \
	l ^= ((S[0][U8a(t)] + S[1][U8b(t)]) ^ \
	 S[2][U8c(t)]) - S[3][U8d(t)];

#define F1(l, r, i, j) f1(l, r, K[i], K[i+j])
#define F2(l, r, i, j) f2(l, r, K[i], K[i+j])
#define F3(l, r, i, j) f3(l, r, K[i], K[i+j])

typedef BlockGetAndPut<word32, BigEndian> Block;

void CAST128::Enc::ProcessAndXorBlock(const byte *inBlock, const byte *xorBlock, byte *outBlock) const
{
	word32 t, l, r;

	/* Get inblock into l,r */
	Block::Get(inBlock)(l)(r);
	/* Do the work */
	F1(l, r,  0, 16);
	F2(r, l,  1, 16);
	F3(l, r,  2, 16);
	F1(r, l,  3, 16);
	F2(l, r,  4, 16);
	F3(r, l,  5, 16);
	F1(l, r,  6, 16);
	F2(r, l,  7, 16);
	F3(l, r,  8, 16);
	F1(r, l,  9, 16);
	F2(l, r, 10, 16);
	F3(r, l, 11, 16);
	/* Only do full 16 rounds if key length > 80 bits */
	if (!reduced) {
		F1(l, r, 12, 16);
		F2(r, l, 13, 16);
		F3(l, r, 14, 16);
		F1(r, l, 15, 16);
	}
	/* Put l,r into outblock */
	Block::Put(xorBlock, outBlock)(r)(l);
}

void CAST128::Dec::ProcessAndXorBlock(const byte *inBlock, const byte *xorBlock, byte *outBlock) const
{
	word32 t, l, r;

	/* Get inblock into l,r */
	Block::Get(inBlock)(r)(l);
	/* Only do full 16 rounds if key length > 80 bits */
	if (!reduced) {
		F1(r, l, 15, 16);
		F3(l, r, 14, 16);
		F2(r, l, 13, 16);
		F1(l, r, 12, 16);
	}
	F3(r, l, 11, 16);
	F2(l, r, 10, 16);
	F1(r, l,  9, 16);
	F3(l, r,  8, 16);
	F2(r, l,  7, 16);
	F1(l, r,  6, 16);
	F3(r, l,  5, 16);
	F2(l, r,  4, 16);
	F1(r, l,  3, 16);
	F3(l, r,  2, 16);
	F2(r, l,  1, 16);
	F1(l, r,  0, 16);
	/* Put l,r into outblock */
	Block::Put(xorBlock, outBlock)(l)(r);
	/* Wipe clean */
	t = l = r = 0;
}

void CAST128::Base::UncheckedSetKey(const byte *userKey, unsigned int keylength, const NameValuePairs &)
{
	AssertValidKeyLength(keylength);

	reduced = (keylength <= 10);

	word32 X[4], Z[4];
	GetUserKey(BIG_ENDIAN_ORDER, X, 4, userKey, keylength);

#define x(i) GETBYTE(X[i/4], 3-i%4)
#define z(i) GETBYTE(Z[i/4], 3-i%4)

	unsigned int i;
	for (i=0; i<=16; i+=16)
	{
		// this part is copied directly from RFC 2144 (with some search and replace) by Wei Dai
		Z[0] = X[0] ^ S[4][x(0xD)] ^ S[5][x(0xF)] ^ S[6][x(0xC)] ^ S[7][x(0xE)] ^ S[6][x(0x8)];
		Z[1] = X[2] ^ S[4][z(0x0)] ^ S[5][z(0x2)] ^ S[6][z(0x1)] ^ S[7][z(0x3)] ^ S[7][x(0xA)];
		Z[2] = X[3] ^ S[4][z(0x7)] ^ S[5][z(0x6)] ^ S[6][z(0x5)] ^ S[7][z(0x4)] ^ S[4][x(0x9)];
		Z[3] = X[1] ^ S[4][z(0xA)] ^ S[5][z(0x9)] ^ S[6][z(0xB)] ^ S[7][z(0x8)] ^ S[5][x(0xB)];
		K[i+0] = S[4][z(0x8)] ^ S[5][z(0x9)] ^ S[6][z(0x7)] ^ S[7][z(0x6)] ^ S[4][z(0x2)];
		K[i+1] = S[4][z(0xA)] ^ S[5][z(0xB)] ^ S[6][z(0x5)] ^ S[7][z(0x4)] ^ S[5][z(0x6)];
		K[i+2] = S[4][z(0xC)] ^ S[5][z(0xD)] ^ S[6][z(0x3)] ^ S[7][z(0x2)] ^ S[6][z(0x9)];
		K[i+3] = S[4][z(0xE)] ^ S[5][z(0xF)] ^ S[6][z(0x1)] ^ S[7][z(0x0)] ^ S[7][z(0xC)];
		X[0] = Z[2] ^ S[4][z(0x5)] ^ S[5][z(0x7)] ^ S[6][z(0x4)] ^ S[7][z(0x6)] ^ S[6][z(0x0)];
		X[1] = Z[0] ^ S[4][x(0x0)] ^ S[5][x(0x2)] ^ S[6][x(0x1)] ^ S[7][x(0x3)] ^ S[7][z(0x2)];
		X[2] = Z[1] ^ S[4][x(0x7)] ^ S[5][x(0x6)] ^ S[6][x(0x5)] ^ S[7][x(0x4)] ^ S[4][z(0x1)];
		X[3] = Z[3] ^ S[4][x(0xA)] ^ S[5][x(0x9)] ^ S[6][x(0xB)] ^ S[7][x(0x8)] ^ S[5][z(0x3)];
		K[i+4] = S[4][x(0x3)] ^ S[5][x(0x2)] ^ S[6][x(0xC)] ^ S[7][x(0xD)] ^ S[4][x(0x8)];
		K[i+5] = S[4][x(0x1)] ^ S[5][x(0x0)] ^ S[6][x(0xE)] ^ S[7][x(0xF)] ^ S[5][x(0xD)];
		K[i+6] = S[4][x(0x7)] ^ S[5][x(0x6)] ^ S[6][x(0x8)] ^ S[7][x(0x9)] ^ S[6][x(0x3)];
		K[i+7] = S[4][x(0x5)] ^ S[5][x(0x4)] ^ S[6][x(0xA)] ^ S[7][x(0xB)] ^ S[7][x(0x7)];
		Z[0] = X[0] ^ S[4][x(0xD)] ^ S[5][x(0xF)] ^ S[6][x(0xC)] ^ S[7][x(0xE)] ^ S[6][x(0x8)];
		Z[1] = X[2] ^ S[4][z(0x0)] ^ S[5][z(0x2)] ^ S[6][z(0x1)] ^ S[7][z(0x3)] ^ S[7][x(0xA)];
		Z[2] = X[3] ^ S[4][z(0x7)] ^ S[5][z(0x6)] ^ S[6][z(0x5)] ^ S[7][z(0x4)] ^ S[4][x(0x9)];
		Z[3] = X[1] ^ S[4][z(0xA)] ^ S[5][z(0x9)] ^ S[6][z(0xB)] ^ S[7][z(0x8)] ^ S[5][x(0xB)];
		K[i+8] = S[4][z(0x3)] ^ S[5][z(0x2)] ^ S[6][z(0xC)] ^ S[7][z(0xD)] ^ S[4][z(0x9)];
		K[i+9] = S[4][z(0x1)] ^ S[5][z(0x0)] ^ S[6][z(0xE)] ^ S[7][z(0xF)] ^ S[5][z(0xC)];
		K[i+10] = S[4][z(0x7)] ^ S[5][z(0x6)] ^ S[6][z(0x8)] ^ S[7][z(0x9)] ^ S[6][z(0x2)];
		K[i+11] = S[4][z(0x5)] ^ S[5][z(0x4)] ^ S[6][z(0xA)] ^ S[7][z(0xB)] ^ S[7][z(0x6)];
		X[0] = Z[2] ^ S[4][z(0x5)] ^ S[5][z(0x7)] ^ S[6][z(0x4)] ^ S[7][z(0x6)] ^ S[6][z(0x0)];
		X[1] = Z[0] ^ S[4][x(0x0)] ^ S[5][x(0x2)] ^ S[6][x(0x1)] ^ S[7][x(0x3)] ^ S[7][z(0x2)];
		X[2] = Z[1] ^ S[4][x(0x7)] ^ S[5][x(0x6)] ^ S[6][x(0x5)] ^ S[7][x(0x4)] ^ S[4][z(0x1)];
		X[3] = Z[3] ^ S[4][x(0xA)] ^ S[5][x(0x9)] ^ S[6][x(0xB)] ^ S[7][x(0x8)] ^ S[5][z(0x3)];
		K[i+12] = S[4][x(0x8)] ^ S[5][x(0x9)] ^ S[6][x(0x7)] ^ S[7][x(0x6)] ^ S[4][x(0x3)];
		K[i+13] = S[4][x(0xA)] ^ S[5][x(0xB)] ^ S[6][x(0x5)] ^ S[7][x(0x4)] ^ S[5][x(0x7)];
		K[i+14] = S[4][x(0xC)] ^ S[5][x(0xD)] ^ S[6][x(0x3)] ^ S[7][x(0x2)] ^ S[6][x(0x8)];
		K[i+15] = S[4][x(0xE)] ^ S[5][x(0xF)] ^ S[6][x(0x1)] ^ S[7][x(0x0)] ^ S[7][x(0xD)];
	}

	for (i=16; i<32; i++)
		K[i] &= 0x1f;
}

// The following CAST-256 implementation was contributed by Leonard Janke

const word32 CAST256::Base::t_m[8][24]={
{	0x5a827999, 0xd151d6a1, 0x482133a9, 0xbef090b1, 0x35bfedb9, 0xac8f4ac1,
	0x235ea7c9, 0x9a2e04d1, 0x10fd61d9, 0x87ccbee1, 0xfe9c1be9, 0x756b78f1,
	0xec3ad5f9, 0x630a3301, 0xd9d99009, 0x50a8ed11, 0xc7784a19, 0x3e47a721,
	0xb5170429, 0x2be66131, 0xa2b5be39, 0x19851b41, 0x90547849, 0x0723d551},
{	0xc95c653a, 0x402bc242, 0xb6fb1f4a, 0x2dca7c52, 0xa499d95a, 0x1b693662,
	0x9238936a, 0x0907f072, 0x7fd74d7a, 0xf6a6aa82, 0x6d76078a, 0xe4456492,
	0x5b14c19a, 0xd1e41ea2, 0x48b37baa, 0xbf82d8b2, 0x365235ba, 0xad2192c2,
	0x23f0efca, 0x9ac04cd2, 0x118fa9da, 0x885f06e2, 0xff2e63ea, 0x75fdc0f2},
{	0x383650db, 0xaf05ade3, 0x25d50aeb, 0x9ca467f3, 0x1373c4fb, 0x8a432203,
	0x01127f0b, 0x77e1dc13, 0xeeb1391b, 0x65809623, 0xdc4ff32b, 0x531f5033,
	0xc9eead3b, 0x40be0a43, 0xb78d674b, 0x2e5cc453, 0xa52c215b, 0x1bfb7e63,
	0x92cadb6b, 0x099a3873, 0x8069957b, 0xf738f283, 0x6e084f8b, 0xe4d7ac93},
{	0xa7103c7c, 0x1ddf9984, 0x94aef68c, 0x0b7e5394, 0x824db09c, 0xf91d0da4,
	0x6fec6aac, 0xe6bbc7b4, 0x5d8b24bc, 0xd45a81c4, 0x4b29decc, 0xc1f93bd4,
	0x38c898dc, 0xaf97f5e4, 0x266752ec, 0x9d36aff4, 0x14060cfc, 0x8ad56a04,
	0x01a4c70c, 0x78742414, 0xef43811c, 0x6612de24, 0xdce23b2c, 0x53b19834},
{	0x15ea281d, 0x8cb98525, 0x0388e22d, 0x7a583f35, 0xf1279c3d, 0x67f6f945,
	0xdec6564d, 0x5595b355, 0xcc65105d, 0x43346d65, 0xba03ca6d, 0x30d32775,
	0xa7a2847d, 0x1e71e185, 0x95413e8d, 0x0c109b95, 0x82dff89d, 0xf9af55a5,
	0x707eb2ad, 0xe74e0fb5, 0x5e1d6cbd, 0xd4ecc9c5, 0x4bbc26cd, 0xc28b83d5},
{	0x84c413be, 0xfb9370c6, 0x7262cdce, 0xe9322ad6, 0x600187de, 0xd6d0e4e6,
	0x4da041ee, 0xc46f9ef6, 0x3b3efbfe, 0xb20e5906, 0x28ddb60e, 0x9fad1316,
	0x167c701e, 0x8d4bcd26, 0x041b2a2e, 0x7aea8736, 0xf1b9e43e, 0x68894146,
	0xdf589e4e, 0x5627fb56, 0xccf7585e, 0x43c6b566, 0xba96126e, 0x31656f76},
{	0xf39dff5f, 0x6a6d5c67, 0xe13cb96f, 0x580c1677, 0xcedb737f, 0x45aad087,
	0xbc7a2d8f, 0x33498a97, 0xaa18e79f, 0x20e844a7, 0x97b7a1af, 0x0e86feb7,
	0x85565bbf, 0xfc25b8c7, 0x72f515cf, 0xe9c472d7, 0x6093cfdf, 0xd7632ce7,
	0x4e3289ef, 0xc501e6f7, 0x3bd143ff, 0xb2a0a107, 0x296ffe0f, 0xa03f5b17},
{	0x6277eb00, 0xd9474808, 0x5016a510, 0xc6e60218, 0x3db55f20, 0xb484bc28,
	0x2b541930, 0xa2237638, 0x18f2d340, 0x8fc23048, 0x06918d50, 0x7d60ea58,
	0xf4304760, 0x6affa468, 0xe1cf0170, 0x589e5e78, 0xcf6dbb80, 0x463d1888,
	0xbd0c7590, 0x33dbd298, 0xaaab2fa0, 0x217a8ca8, 0x9849e9b0, 0x0f1946b8}
};

const unsigned int CAST256::Base::t_r[8][24]={
	{19, 27, 3, 11, 19, 27, 3, 11, 19, 27, 3, 11, 19, 27, 3, 11, 19, 27, 3, 11, 19, 27, 3, 11},
	{4, 12, 20, 28, 4, 12, 20, 28, 4, 12, 20, 28, 4, 12, 20, 28, 4, 12, 20, 28, 4, 12, 20, 28},
	{21, 29, 5, 13, 21, 29, 5, 13, 21, 29, 5, 13, 21, 29, 5, 13, 21, 29, 5, 13, 21, 29, 5, 13},
	{6, 14, 22, 30, 6, 14, 22, 30, 6, 14, 22, 30, 6, 14, 22, 30, 6, 14, 22, 30, 6, 14, 22, 30},
	{23, 31, 7, 15, 23, 31, 7, 15, 23, 31, 7, 15, 23, 31, 7, 15, 23, 31, 7, 15, 23, 31, 7, 15},
	{8, 16, 24, 0, 8, 16, 24, 0, 8, 16, 24, 0, 8, 16, 24, 0, 8, 16, 24, 0, 8, 16, 24, 0},
	{25, 1, 9, 17, 25, 1, 9, 17, 25, 1, 9, 17, 25, 1, 9, 17, 25, 1, 9, 17, 25, 1, 9, 17},
	{10, 18, 26, 2, 10, 18, 26, 2, 10, 18, 26, 2, 10, 18, 26, 2, 10, 18, 26, 2, 10, 18, 26, 2}
};

#define Q(i) \
	F1(block[2],block[3],8*i+4,-4); \
	F2(block[1],block[2],8*i+5,-4); \
	F3(block[0],block[1],8*i+6,-4); \
	F1(block[3],block[0],8*i+7,-4);

#define QBar(i) \
	F1(block[3],block[0],8*i+7,-4); \
	F3(block[0],block[1],8*i+6,-4); \
	F2(block[1],block[2],8*i+5,-4); \
	F1(block[2],block[3],8*i+4,-4);

/* CAST256's encrypt/decrypt functions  are identical except for the order that
the keys are used */

void CAST256::Base::ProcessAndXorBlock(const byte *inBlock, const byte *xorBlock, byte *outBlock) const
{
	word32 t, block[4];
	Block::Get(inBlock)(block[0])(block[1])(block[2])(block[3]);

	// Perform 6 forward quad rounds
	Q(0);
	Q(1);
	Q(2);
	Q(3);
	Q(4);
	Q(5);

	// Perform 6 reverse quad rounds
	QBar(6);
	QBar(7);
	QBar(8);
	QBar(9);
	QBar(10);
	QBar(11);

	Block::Put(xorBlock, outBlock)(block[0])(block[1])(block[2])(block[3]);
}

/* Set up a CAST-256 key */

void CAST256::Base::Omega(int i, word32 kappa[8])
{
	word32 t;

	f1(kappa[6],kappa[7],t_m[0][i],t_r[0][i]);
	f2(kappa[5],kappa[6],t_m[1][i],t_r[1][i]);
	f3(kappa[4],kappa[5],t_m[2][i],t_r[2][i]);
	f1(kappa[3],kappa[4],t_m[3][i],t_r[3][i]);
	f2(kappa[2],kappa[3],t_m[4][i],t_r[4][i]);
	f3(kappa[1],kappa[2],t_m[5][i],t_r[5][i]);
	f1(kappa[0],kappa[1],t_m[6][i],t_r[6][i]);
	f2(kappa[7],kappa[0],t_m[7][i],t_r[7][i]);
}

void CAST256::Base::UncheckedSetKey(const byte *userKey, unsigned int keylength, const NameValuePairs &)
{
	AssertValidKeyLength(keylength);

	word32 kappa[8];
	GetUserKey(BIG_ENDIAN_ORDER, kappa, 8, userKey, keylength);

	for(int i=0; i<12; ++i)
	{
		Omega(2*i,kappa);
		Omega(2*i+1,kappa);

		K[8*i]=kappa[0] & 31;
		K[8*i+1]=kappa[2] & 31;
		K[8*i+2]=kappa[4] & 31;
		K[8*i+3]=kappa[6] & 31;
		K[8*i+4]=kappa[7];
		K[8*i+5]=kappa[5];
		K[8*i+6]=kappa[3];
		K[8*i+7]=kappa[1];
	}

	if (!IsForwardTransformation())
	{
		for(int j=0; j<6; ++j)
		{
			for(int i=0; i<4; ++i)
			{
				int i1=8*j+i;
				int i2=8*(11-j)+i;

				CRYPTOPP_ASSERT(i1<i2);

				std::swap(K[i1],K[i2]);
				std::swap(K[i1+4],K[i2+4]);
			}
		}
	}

	memset(kappa, 0, sizeof(kappa));
}

NAMESPACE_END
