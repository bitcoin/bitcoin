// wake.cpp - written and placed in the public domain by Wei Dai

#include "pch.h"

#include "wake.h"
#include "smartptr.h"

NAMESPACE_BEGIN(CryptoPP)

#if !defined(NDEBUG) && !defined(CRYPTOPP_DOXYGEN_PROCESSING)
void WAKE_TestInstantiations()
{
	WAKE_OFB<>::Encryption x2;
	WAKE_OFB<>::Decryption x4;
}
#endif

inline word32 WAKE_Base::M(word32 x, word32 y)
{
	word32 w = x+y;
	return (w>>8) ^ t[w & 0xff];
}

void WAKE_Base::GenKey(word32 k0, word32 k1, word32 k2, word32 k3)
{
	// this code is mostly copied from David Wheeler's paper "A Bulk Data Encryption Algorithm"
	signed int x, z, p;	
	// x and z were declared as "long" in Wheeler's paper, which is a signed type. I don't know if that was intentional, but it's too late to change it now. -- Wei 7/4/2010
	CRYPTOPP_COMPILE_ASSERT(sizeof(x) == 4);
	static unsigned int tt[10]= {
		0x726a8f3b,								 // table
		0xe69a3b5c,
		0xd3c71fe5,
		0xab3c73d2,
		0x4d3a8eb3,
		0x0396d6e8,
		0x3d4c2f7a,
		0x9ee27cf3, } ;
	t[0] = k0;
	t[1] = k1;
	t[2] = k2;
	t[3] = k3;
	for (p=4 ; p<256 ; p++)
	{
	  x=t[p-4]+t[p-1] ; 					   // fill t
	  t[p]= (x>>3) ^ tt[x&7] ;
	}

	for (p=0 ; p<23 ; p++)
		t[p]+=t[p+89] ; 		  // mix first entries
	x=t[33] ; z=t[59] | 0x01000001 ;
	z=z&0xff7fffff ;
	for (p=0 ; p<256 ; p++) {		//change top byte to
	  x=(x&0xff7fffff)+z ; 		 // a permutation etc
	  t[p]=(t[p] & 0x00ffffff) ^ x ; }

	t[256]=t[0] ;
	byte y=byte(x);
	for (p=0 ; p<256 ; p++) {	  // further change perm.
	  t[p]=t[y=byte(t[p^y]^y)] ;  // and other digits
	  t[y]=t[p+1] ;  }
}

template <class B>
void WAKE_Policy<B>::CipherSetKey(const NameValuePairs &params, const byte *key, size_t length)
{
	CRYPTOPP_UNUSED(params); CRYPTOPP_UNUSED(key); CRYPTOPP_UNUSED(length);
	word32 k0, k1, k2, k3;
	BlockGetAndPut<word32, BigEndian>::Get(key)(r3)(r4)(r5)(r6)(k0)(k1)(k2)(k3);
	GenKey(k0, k1, k2, k3);
}

// OFB
template <class B>
void WAKE_Policy<B>::OperateKeystream(KeystreamOperation operation, byte *output, const byte *input, size_t iterationCount)
{
#define WAKE_OUTPUT(x)\
	while (iterationCount--)\
	{\
		CRYPTOPP_KEYSTREAM_OUTPUT_WORD(x, B::ToEnum(), 0, r6);\
		r3 = M(r3, r6);\
		r4 = M(r4, r3);\
		r5 = M(r5, r4);\
		r6 = M(r6, r5);\
		output += 4;\
		if (!(x & INPUT_NULL))\
			input += 4;\
	}

	typedef word32 WordType;
	CRYPTOPP_KEYSTREAM_OUTPUT_SWITCH(WAKE_OUTPUT, 0);
}
/*
template <class B>
void WAKE_ROFB_Policy<B>::Iterate(KeystreamOperation operation, byte *output, const byte *input, unsigned int iterationCount)
{
	KeystreamOutput<B> keystreamOperation(operation, output, input);

	while (iterationCount--)
	{
		keystreamOperation(r6);
		r3 = M(r3, r6);
		r4 = M(r4, r3);
		r5 = M(r5, r4);
		r6 = M(r6, r5);
	}
}
*/
template class WAKE_Policy<BigEndian>;
template class WAKE_Policy<LittleEndian>;
//template class WAKE_ROFB_Policy<BigEndian>;
//template class WAKE_ROFB_Policy<LittleEndian>;

NAMESPACE_END
