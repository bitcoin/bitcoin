#ifndef CRYPTOPP_ITERHASH_H
#define CRYPTOPP_ITERHASH_H

#include "cryptlib.h"
#include "secblock.h"
#include "misc.h"
#include "simple.h"

NAMESPACE_BEGIN(CryptoPP)

//! exception thrown when trying to hash more data than is allowed by a hash function
class CRYPTOPP_DLL HashInputTooLong : public InvalidDataFormat
{
public:
	explicit HashInputTooLong(const std::string &alg)
		: InvalidDataFormat("IteratedHashBase: input data exceeds maximum allowed by hash function " + alg) {}
};

//! _
template <class T, class BASE>
class CRYPTOPP_NO_VTABLE IteratedHashBase : public BASE
{
public:
	typedef T HashWordType;

	IteratedHashBase() : m_countLo(0), m_countHi(0) {}
	unsigned int OptimalBlockSize() const {return this->BlockSize();}
	unsigned int OptimalDataAlignment() const {return GetAlignmentOf<T>();}
	void Update(const byte *input, size_t length);
	byte * CreateUpdateSpace(size_t &size);
	void Restart();
	void TruncatedFinal(byte *digest, size_t size);

protected:
	inline T GetBitCountHi() const {return (m_countLo >> (8*sizeof(T)-3)) + (m_countHi << 3);}
	inline T GetBitCountLo() const {return m_countLo << 3;}

	void PadLastBlock(unsigned int lastBlockSize, byte padFirst=0x80);
	virtual void Init() =0;

	virtual ByteOrder GetByteOrder() const =0;
	virtual void HashEndianCorrectedBlock(const HashWordType *data) =0;
	virtual size_t HashMultipleBlocks(const T *input, size_t length);
	void HashBlock(const HashWordType *input) {HashMultipleBlocks(input, this->BlockSize());}

	virtual T* DataBuf() =0;
	virtual T* StateBuf() =0;

private:
	T m_countLo, m_countHi;
};

//! _
template <class T_HashWordType, class T_Endianness, unsigned int T_BlockSize, class T_Base = HashTransformation>
class CRYPTOPP_NO_VTABLE IteratedHash : public IteratedHashBase<T_HashWordType, T_Base>
{
public:
	typedef T_Endianness ByteOrderClass;
	typedef T_HashWordType HashWordType;

	CRYPTOPP_CONSTANT(BLOCKSIZE = T_BlockSize)
	// BCB2006 workaround: can't use BLOCKSIZE here
	CRYPTOPP_COMPILE_ASSERT((T_BlockSize & (T_BlockSize - 1)) == 0);	// blockSize is a power of 2
	unsigned int BlockSize() const {return T_BlockSize;}

	ByteOrder GetByteOrder() const {return T_Endianness::ToEnum();}

	inline static void CorrectEndianess(HashWordType *out, const HashWordType *in, size_t byteCount)
	{
		ConditionalByteReverse(T_Endianness::ToEnum(), out, in, byteCount);
	}

protected:
	T_HashWordType* DataBuf() {return this->m_data;}
	FixedSizeSecBlock<T_HashWordType, T_BlockSize/sizeof(T_HashWordType)> m_data;
};

//! _
template <class T_HashWordType, class T_Endianness, unsigned int T_BlockSize, unsigned int T_StateSize, class T_Transform, unsigned int T_DigestSize = 0, bool T_StateAligned = false>
class CRYPTOPP_NO_VTABLE IteratedHashWithStaticTransform
	: public ClonableImpl<T_Transform, AlgorithmImpl<IteratedHash<T_HashWordType, T_Endianness, T_BlockSize>, T_Transform> >
{
public:
	CRYPTOPP_CONSTANT(DIGESTSIZE = T_DigestSize ? T_DigestSize : T_StateSize)
	unsigned int DigestSize() const {return DIGESTSIZE;};

protected:
	IteratedHashWithStaticTransform() {this->Init();}
	void HashEndianCorrectedBlock(const T_HashWordType *data) {T_Transform::Transform(this->m_state, data);}
	void Init() {T_Transform::InitState(this->m_state);}

	T_HashWordType* StateBuf() {return this->m_state;}
	FixedSizeAlignedSecBlock<T_HashWordType, T_BlockSize/sizeof(T_HashWordType), T_StateAligned> m_state;
};

#ifndef __GNUC__
	CRYPTOPP_DLL_TEMPLATE_CLASS IteratedHashBase<word64, HashTransformation>;
	CRYPTOPP_STATIC_TEMPLATE_CLASS IteratedHashBase<word64, MessageAuthenticationCode>;

	CRYPTOPP_DLL_TEMPLATE_CLASS IteratedHashBase<word32, HashTransformation>;
	CRYPTOPP_STATIC_TEMPLATE_CLASS IteratedHashBase<word32, MessageAuthenticationCode>;
#endif

NAMESPACE_END

#endif
