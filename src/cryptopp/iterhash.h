#ifndef CRYPTOPP_ITERHASH_H
#define CRYPTOPP_ITERHASH_H

#include "secblock.h"

NAMESPACE_BEGIN(CryptoPP)

// *** trimmed down dependency from iterhash.h ***
template <class T_HashWordType, class T_Endianness, unsigned int T_BlockSize, unsigned int T_StateSize, class T_Transform, unsigned int T_DigestSize = 0, bool T_StateAligned = false>
class CRYPTOPP_NO_VTABLE IteratedHashWithStaticTransform
{
public:
	CRYPTOPP_CONSTANT(DIGESTSIZE = T_DigestSize ? T_DigestSize : T_StateSize)
	unsigned int DigestSize() const {return DIGESTSIZE;};
    typedef T_HashWordType HashWordType;
    CRYPTOPP_CONSTANT(BLOCKSIZE = T_BlockSize)

protected:
	IteratedHashWithStaticTransform() {this->Init();}
	void HashEndianCorrectedBlock(const T_HashWordType *data) {T_Transform::Transform(this->m_state, data);}
	void Init() {T_Transform::InitState(this->m_state);}

	T_HashWordType* StateBuf() {return this->m_state;}
	FixedSizeAlignedSecBlock<T_HashWordType, T_BlockSize/sizeof(T_HashWordType), T_StateAligned> m_state;
};

NAMESPACE_END

#endif
