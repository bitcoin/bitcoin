#ifndef CRYPTOPP_ITERHASH_H
#define CRYPTOPP_ITERHASH_H

#include "cryptlib.h"
#include "secblock.h"
#include "misc.h"
#include "simple.h"

NAMESPACE_BEGIN(CryptoPP)

//! \class HashInputTooLong
//! \brief Exception thrown when trying to hash more data than is allowed by a hash function
class CRYPTOPP_DLL HashInputTooLong : public InvalidDataFormat
{
public:
	explicit HashInputTooLong(const std::string &alg)
		: InvalidDataFormat("IteratedHashBase: input data exceeds maximum allowed by hash function " + alg) {}
};

//! \class IteratedHashBase
//! \brief Iterated hash base class
//! \tparam T Hash word type
//! \tparam BASE HashTransformation derived class
//! \details IteratedHashBase provides an interface for block-based iterated hashes
//! \sa HashTransformation, MessageAuthenticationCode
template <class T, class BASE>
class CRYPTOPP_NO_VTABLE IteratedHashBase : public BASE
{
public:
	typedef T HashWordType;

	//! \brief Construct an IteratedHashBase
	IteratedHashBase() : m_countLo(0), m_countHi(0) {}

	//! \brief Provides the input block size most efficient for this cipher.
	//! \return The input block size that is most efficient for the cipher
	//! \details The base class implementation returns MandatoryBlockSize().
	//! \note Optimal input length is
	//!   <tt>n * OptimalBlockSize() - GetOptimalBlockSizeUsed()</tt> for any <tt>n \> 0</tt>.
	unsigned int OptimalBlockSize() const {return this->BlockSize();}

	//! \brief Provides input and output data alignment for optimal performance.
	//! \return the input data alignment that provides optimal performance
	//! \details OptimalDataAlignment returnes the natural alignment of the hash word.
	unsigned int OptimalDataAlignment() const {return GetAlignmentOf<T>();}

	//! \brief Updates a hash with additional input
	//! \param input the additional input as a buffer
	//! \param length the size of the buffer, in bytes
	void Update(const byte *input, size_t length);

	//! \brief Requests space which can be written into by the caller
	//! \param size the requested size of the buffer
	//! \details The purpose of this method is to help avoid extra memory allocations.
	//! \details size is an \a IN and \a OUT parameter and used as a hint. When the call is made,
	//!   size is the requested size of the buffer. When the call returns, size is the size of
	//!   the array returned to the caller.
	//! \details The base class implementation sets  size to 0 and returns  NULL.
	//! \note Some objects, like ArraySink, cannot create a space because its fixed.
	byte * CreateUpdateSpace(size_t &size);

	//! \brief Restart the hash
	//! \details Discards the current state, and restart for a new message
	void Restart();

	//! \brief Computes the hash of the current message
	//! \param digest a pointer to the buffer to receive the hash
	//! \param digestSize the size of the truncated digest, in bytes
	//! \details TruncatedFinal() call Final() and then copies digestSize bytes to digest.
	//!   The hash is restarted the hash for the next message.
	void TruncatedFinal(byte *digest, size_t digestSize);

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

//! \class IteratedHash
//! \brief Iterated hash base class
//! \tparam T_HashWordType Hash word type
//! \tparam T_Endianness Endianness type of hash
//! \tparam T_BlockSize Block size of the hash
//! \tparam T_Base HashTransformation derived class
//! \details IteratedHash provides a default implementation for block-based iterated hashes
//! \sa HashTransformation, MessageAuthenticationCode
template <class T_HashWordType, class T_Endianness, unsigned int T_BlockSize, class T_Base = HashTransformation>
class CRYPTOPP_NO_VTABLE IteratedHash : public IteratedHashBase<T_HashWordType, T_Base>
{
public:
	typedef T_Endianness ByteOrderClass;
	typedef T_HashWordType HashWordType;

#ifndef CRYPTOPP_MAINTAIN_BACKWARDS_COMPATIBILITY_562
	virtual ~IteratedHash() { }
#endif

	CRYPTOPP_CONSTANT(BLOCKSIZE = T_BlockSize)
	// BCB2006 workaround: can't use BLOCKSIZE here
	CRYPTOPP_COMPILE_ASSERT((T_BlockSize & (T_BlockSize - 1)) == 0);	// blockSize is a power of 2

	//! \brief Provides the block size of the hash
	//! \return the block size of the hash, in bytes
	//! \details BlockSize() returns <tt>T_BlockSize</tt>.
	unsigned int BlockSize() const {return T_BlockSize;}

	//! \brief Provides the byte order of the hash
	//! \returns the byte order of the hash as an enumeration
	//! \details GetByteOrder() returns <tt>T_Endianness::ToEnum()</tt>.
	//! \sa ByteOrder()
	ByteOrder GetByteOrder() const {return T_Endianness::ToEnum();}

	//! \brief Adjusts the byte ordering of the hash
	//! \param out the output buffer
	//! \param in the input buffer
	//! \param byteCount the size of the buffers, in bytes
	//! \details CorrectEndianess() calls ConditionalByteReverse() using <tt>T_Endianness</tt>.
	inline void CorrectEndianess(HashWordType *out, const HashWordType *in, size_t byteCount)
	{
		ConditionalByteReverse(T_Endianness::ToEnum(), out, in, byteCount);
	}

protected:
	T_HashWordType* DataBuf() {return this->m_data;}
	FixedSizeSecBlock<T_HashWordType, T_BlockSize/sizeof(T_HashWordType)> m_data;
};

//! \class IteratedHashWithStaticTransform
//! \brief Iterated hash with a static transformation function
//! \tparam T_HashWordType Hash word type
//! \tparam T_Endianness Endianness type of hash
//! \tparam T_BlockSize Block size of the hash
//! \tparam T_StateSize Internal state size of the hash
//! \tparam T_Transform HashTransformation derived class
//! \tparam T_DigestSize Digest size of the hash
//! \tparam T_StateAligned Flag indicating if state is 16-byte aligned
//! \sa HashTransformation, MessageAuthenticationCode
template <class T_HashWordType, class T_Endianness, unsigned int T_BlockSize, unsigned int T_StateSize, class T_Transform, unsigned int T_DigestSize = 0, bool T_StateAligned = false>
class CRYPTOPP_NO_VTABLE IteratedHashWithStaticTransform
	: public ClonableImpl<T_Transform, AlgorithmImpl<IteratedHash<T_HashWordType, T_Endianness, T_BlockSize>, T_Transform> >
{
public:

#ifndef CRYPTOPP_MAINTAIN_BACKWARDS_COMPATIBILITY_562
	virtual ~IteratedHashWithStaticTransform() { }
#endif

	CRYPTOPP_CONSTANT(DIGESTSIZE = T_DigestSize ? T_DigestSize : T_StateSize)

	//! \brief Provides the digest size of the hash
	//! \return the digest size of the hash, in bytes
	//! \details DigestSize() returns <tt>DIGESTSIZE</tt>.
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
