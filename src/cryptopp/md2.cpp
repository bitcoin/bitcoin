// md2.cpp - modified by Wei Dai from Andrew M. Kuchling's md2.c
// The original code and all modifications are in the public domain.

// This is the original introductory comment:

/*
 *  md2.c : MD2 hash algorithm.
 *
 * Part of the Python Cryptography Toolkit, version 1.1
 *
 * Distribute and use freely; there are no restrictions on further 
 * dissemination and usage except those imposed by the laws of your 
 * country of residence.
 *
 */

#include "pch.h"
#define CRYPTOPP_ENABLE_NAMESPACE_WEAK 1
#include "md2.h"

NAMESPACE_BEGIN(CryptoPP)
namespace Weak1 {
	
MD2::MD2()
	: m_X(48), m_C(16), m_buf(16)
{
	Init();
}

void MD2::Init()
{
	memset(m_X, 0, 48);
	memset(m_C, 0, 16);
	memset(m_buf, 0, 16);
	m_count = 0;
}

void MD2::Update(const byte *buf, size_t len)
{
	static const byte S[256] = {
		41, 46, 67, 201, 162, 216, 124, 1, 61, 54, 84, 161, 236, 240, 6,
		19, 98, 167, 5, 243, 192, 199, 115, 140, 152, 147, 43, 217, 188,
		76, 130, 202, 30, 155, 87, 60, 253, 212, 224, 22, 103, 66, 111, 24,
		138, 23, 229, 18, 190, 78, 196, 214, 218, 158, 222, 73, 160, 251,
		245, 142, 187, 47, 238, 122, 169, 104, 121, 145, 21, 178, 7, 63,
		148, 194, 16, 137, 11, 34, 95, 33, 128, 127, 93, 154, 90, 144, 50,
		39, 53, 62, 204, 231, 191, 247, 151, 3, 255, 25, 48, 179, 72, 165,
		181, 209, 215, 94, 146, 42, 172, 86, 170, 198, 79, 184, 56, 210,
		150, 164, 125, 182, 118, 252, 107, 226, 156, 116, 4, 241, 69, 157,
		112, 89, 100, 113, 135, 32, 134, 91, 207, 101, 230, 45, 168, 2, 27,
		96, 37, 173, 174, 176, 185, 246, 28, 70, 97, 105, 52, 64, 126, 15,
		85, 71, 163, 35, 221, 81, 175, 58, 195, 92, 249, 206, 186, 197,
		234, 38, 44, 83, 13, 110, 133, 40, 132, 9, 211, 223, 205, 244, 65,
		129, 77, 82, 106, 220, 55, 200, 108, 193, 171, 250, 36, 225, 123,
		8, 12, 189, 177, 74, 120, 136, 149, 139, 227, 99, 232, 109, 233,
		203, 213, 254, 59, 0, 29, 57, 242, 239, 183, 14, 102, 88, 208, 228,
		166, 119, 114, 248, 235, 117, 75, 10, 49, 68, 80, 180, 143, 237,
		31, 26, 219, 153, 141, 51, 159, 17, 131, 20
	};

	while (len) 
    {
		unsigned int L = UnsignedMin(16U-m_count, len);
		memcpy(m_buf+m_count, buf, L);
		m_count+=L;
		buf+=L;
		len-=L;
		if (m_count==16) 
		{
			byte t;
			int i,j;
			
			m_count=0;
			memcpy(m_X+16, m_buf, 16);
			t=m_C[15];
			for(i=0; i<16; i++)
			{
				m_X[32+i]=m_X[16+i]^m_X[i];
				t=m_C[i]^=S[m_buf[i]^t];
			}
			
			t=0;
			for(i=0; i<18; i++)
			{
				for(j=0; j<48; j+=8)
				{
					t=m_X[j+0]^=S[t];
					t=m_X[j+1]^=S[t];
					t=m_X[j+2]^=S[t];
					t=m_X[j+3]^=S[t];
					t=m_X[j+4]^=S[t];
					t=m_X[j+5]^=S[t];
					t=m_X[j+6]^=S[t];
					t=m_X[j+7]^=S[t];
				}
				t = byte((t+i) & 0xFF);
			}
		}
    }
}

void MD2::TruncatedFinal(byte *hash, size_t size)
{
	ThrowIfInvalidTruncatedSize(size);

	byte padding[16];
	word32 padlen;
	unsigned int i;

	padlen= 16-m_count;
	for(i=0; i<padlen; i++) padding[i]=(byte)padlen;
	Update(padding, padlen);
	Update(m_C, 16);
	memcpy(hash, m_X, size);

	Init();
}

}
NAMESPACE_END
