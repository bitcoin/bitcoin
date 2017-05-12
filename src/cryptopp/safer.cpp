// safer.cpp - modified by by Wei Dai from Richard De Moliner's safer.c

#include "pch.h"
#include "safer.h"
#include "misc.h"
#include "argnames.h"

#if CRYPTOPP_MSC_VERSION
# pragma warning(disable: 4244)
#endif

NAMESPACE_BEGIN(CryptoPP)

const byte SAFER::Base::exp_tab[256] =
	{1, 45, 226, 147, 190, 69, 21, 174, 120, 3, 135, 164, 184, 56, 207, 63,
	8, 103, 9, 148, 235, 38, 168, 107, 189, 24, 52, 27, 187, 191, 114, 247,
	64, 53, 72, 156, 81, 47, 59, 85, 227, 192, 159, 216, 211, 243, 141, 177,
	255, 167, 62, 220, 134, 119, 215, 166, 17, 251, 244, 186, 146, 145, 100, 131,
	241, 51, 239, 218, 44, 181, 178, 43, 136, 209, 153, 203, 140, 132, 29, 20,
	129, 151, 113, 202, 95, 163, 139, 87, 60, 130, 196, 82, 92, 28, 232, 160,
	4, 180, 133, 74, 246, 19, 84, 182, 223, 12, 26, 142, 222, 224, 57, 252,
	32, 155, 36, 78, 169, 152, 158, 171, 242, 96, 208, 108, 234, 250, 199, 217,
	0, 212, 31, 110, 67, 188, 236, 83, 137, 254, 122, 93, 73, 201, 50, 194,
	249, 154, 248, 109, 22, 219, 89, 150, 68, 233, 205, 230, 70, 66, 143, 10,
	193, 204, 185, 101, 176, 210, 198, 172, 30, 65, 98, 41, 46, 14, 116, 80,
	2, 90, 195, 37, 123, 138, 42, 91, 240, 6, 13, 71, 111, 112, 157, 126,
	16, 206, 18, 39, 213, 76, 79, 214, 121, 48, 104, 54, 117, 125, 228, 237,
	128, 106, 144, 55, 162, 94, 118, 170, 197, 127, 61, 175, 165, 229, 25, 97,
	253, 77, 124, 183, 11, 238, 173, 75, 34, 245, 231, 115, 35, 33, 200, 5,
	225, 102, 221, 179, 88, 105, 99, 86, 15, 161, 49, 149, 23, 7, 58, 40};

const byte SAFER::Base::log_tab[256] =
	{128, 0, 176, 9, 96, 239, 185, 253, 16, 18, 159, 228, 105, 186, 173, 248,
	192, 56, 194, 101, 79, 6, 148, 252, 25, 222, 106, 27, 93, 78, 168, 130,
	112, 237, 232, 236, 114, 179, 21, 195, 255, 171, 182, 71, 68, 1, 172, 37,
	201, 250, 142, 65, 26, 33, 203, 211, 13, 110, 254, 38, 88, 218, 50, 15,
	32, 169, 157, 132, 152, 5, 156, 187, 34, 140, 99, 231, 197, 225, 115, 198,
	175, 36, 91, 135, 102, 39, 247, 87, 244, 150, 177, 183, 92, 139, 213, 84,
	121, 223, 170, 246, 62, 163, 241, 17, 202, 245, 209, 23, 123, 147, 131, 188,
	189, 82, 30, 235, 174, 204, 214, 53, 8, 200, 138, 180, 226, 205, 191, 217,
	208, 80, 89, 63, 77, 98, 52, 10, 72, 136, 181, 86, 76, 46, 107, 158,
	210, 61, 60, 3, 19, 251, 151, 81, 117, 74, 145, 113, 35, 190, 118, 42,
	95, 249, 212, 85, 11, 220, 55, 49, 22, 116, 215, 119, 167, 230, 7, 219,
	164, 47, 70, 243, 97, 69, 103, 227, 12, 162, 59, 28, 133, 24, 4, 29,
	41, 160, 143, 178, 90, 216, 166, 126, 238, 141, 83, 75, 161, 154, 193, 14,
	122, 73, 165, 44, 129, 196, 199, 54, 43, 127, 67, 149, 51, 242, 108, 104,
	109, 240, 2, 40, 206, 221, 155, 234, 94, 153, 124, 20, 134, 207, 229, 66,
	184, 64, 120, 45, 58, 233, 100, 31, 146, 144, 125, 57, 111, 224, 137, 48};

#define EXP(x)       exp_tab[(x)]
#define LOG(x)       log_tab[(x)]
#define PHT(x, y)    { y += x; x += y; }
#define IPHT(x, y)   { x -= y; y -= x; }

static const unsigned int BLOCKSIZE = 8;
static const unsigned int MAX_ROUNDS = 13;

void SAFER::Base::UncheckedSetKey(const byte *userkey_1, unsigned int length, const NameValuePairs &params)
{
	bool strengthened = Strengthened();
	unsigned int nof_rounds = params.GetIntValueWithDefault(Name::Rounds(), length == 8 ? (strengthened ? 8 : 6) : 10);

	const byte *userkey_2 = length == 8 ? userkey_1 : userkey_1 + 8;
	keySchedule.New(1 + BLOCKSIZE * (1 + 2 * nof_rounds));

	unsigned int i, j;
	byte *key = keySchedule;
	SecByteBlock ka(BLOCKSIZE + 1), kb(BLOCKSIZE + 1);

	if (MAX_ROUNDS < nof_rounds)
		nof_rounds = MAX_ROUNDS;
	*key++ = (unsigned char)nof_rounds;
	ka[BLOCKSIZE] = 0;
	kb[BLOCKSIZE] = 0;
	for (j = 0; j < BLOCKSIZE; j++)
	{
		ka[BLOCKSIZE] ^= ka[j] = rotlFixed(userkey_1[j], 5U);
		kb[BLOCKSIZE] ^= kb[j] = *key++ = userkey_2[j];
	}

	for (i = 1; i <= nof_rounds; i++)
	{
		for (j = 0; j < BLOCKSIZE + 1; j++)
		{
			ka[j] = rotlFixed(ka[j], 6U);
			kb[j] = rotlFixed(kb[j], 6U);
		}
		for (j = 0; j < BLOCKSIZE; j++)
			if (strengthened)
				*key++ = (ka[(j + 2 * i - 1) % (BLOCKSIZE + 1)]
								+ exp_tab[exp_tab[18 * i + j + 1]]) & 0xFF;
			else
				*key++ = (ka[j] + exp_tab[exp_tab[18 * i + j + 1]]) & 0xFF;
		for (j = 0; j < BLOCKSIZE; j++)
			if (strengthened)
				*key++ = (kb[(j + 2 * i) % (BLOCKSIZE + 1)]
								+ exp_tab[exp_tab[18 * i + j + 10]]) & 0xFF;
			else
				*key++ = (kb[j] + exp_tab[exp_tab[18 * i + j + 10]]) & 0xFF;
	}
}

typedef BlockGetAndPut<byte, BigEndian> Block;

void SAFER::Enc::ProcessAndXorBlock(const byte *inBlock, const byte *xorBlock, byte *outBlock) const
{
	byte a, b, c, d, e, f, g, h, t;
	const byte *key = keySchedule+1;
	unsigned int round = keySchedule[0];

	Block::Get(inBlock)(a)(b)(c)(d)(e)(f)(g)(h);
	while(round--)
	{
		a ^= key[0]; b += key[1]; c += key[2]; d ^= key[3];
		e ^= key[4]; f += key[5]; g += key[6]; h ^= key[7];
		a = EXP(a) + key[ 8]; b = LOG(b) ^ key[ 9];
		c = LOG(c) ^ key[10]; d = EXP(d) + key[11];
		e = EXP(e) + key[12]; f = LOG(f) ^ key[13];
		g = LOG(g) ^ key[14]; h = EXP(h) + key[15];
		key += 16;
		PHT(a, b); PHT(c, d); PHT(e, f); PHT(g, h);
		PHT(a, c); PHT(e, g); PHT(b, d); PHT(f, h);
		PHT(a, e); PHT(b, f); PHT(c, g); PHT(d, h);
		t = b; b = e; e = c; c = t; t = d; d = f; f = g; g = t;
	}
	a ^= key[0]; b += key[1]; c += key[2]; d ^= key[3];
	e ^= key[4]; f += key[5]; g += key[6]; h ^= key[7];
	Block::Put(xorBlock, outBlock)(a)(b)(c)(d)(e)(f)(g)(h);
}

void SAFER::Dec::ProcessAndXorBlock(const byte *inBlock, const byte *xorBlock, byte *outBlock) const
{
	byte a, b, c, d, e, f, g, h, t;
	unsigned int round = keySchedule[0];
	const byte *key = keySchedule + BLOCKSIZE * (1 + 2 * round) - 7;

	Block::Get(inBlock)(a)(b)(c)(d)(e)(f)(g)(h);
	h ^= key[7]; g -= key[6]; f -= key[5]; e ^= key[4];
	d ^= key[3]; c -= key[2]; b -= key[1]; a ^= key[0];
	while (round--)
	{
		key -= 16;
		t = e; e = b; b = c; c = t; t = f; f = d; d = g; g = t;
		IPHT(a, e); IPHT(b, f); IPHT(c, g); IPHT(d, h);
		IPHT(a, c); IPHT(e, g); IPHT(b, d); IPHT(f, h);
		IPHT(a, b); IPHT(c, d); IPHT(e, f); IPHT(g, h);
		h -= key[15]; g ^= key[14]; f ^= key[13]; e -= key[12];
		d -= key[11]; c ^= key[10]; b ^= key[9]; a -= key[8];
		h = LOG(h) ^ key[7]; g = EXP(g) - key[6];
		f = EXP(f) - key[5]; e = LOG(e) ^ key[4];
		d = LOG(d) ^ key[3]; c = EXP(c) - key[2];
		b = EXP(b) - key[1]; a = LOG(a) ^ key[0];
	}
	Block::Put(xorBlock, outBlock)(a)(b)(c)(d)(e)(f)(g)(h);
}

NAMESPACE_END
