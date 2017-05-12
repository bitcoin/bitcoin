// blake2.h - written and placed in the public domain by Jeffrey Walton and Zooko
//            Wilcox-O'Hearn. Copyright assigned to the Crypto++ project.
//            Based on Aumasson, Neves, Wilcox-O'Hearn and Winnerlein's reference BLAKE2
//            implementation at http://github.com/BLAKE2/BLAKE2.

//! \file blake2.h
//! \brief Classes for BLAKE2b and BLAKE2s message digests and keyed message digests
//! \details This implmentation follows Aumasson, Neves, Wilcox-O'Hearn and Winnerlein's
//!   <A HREF="http://blake2.net/blake2.pdf">BLAKE2: simpler, smaller, fast as MD5</A> (2013.01.29).
//!   Static algorithm name return either "BLAKE2b" or "BLAKE2s". An object algorithm name follows
//!   the naming described in <A HREF="http://tools.ietf.org/html/rfc7693#section-4">RFC 7693, The
//!   BLAKE2 Cryptographic Hash and Message Authentication Code (MAC)</A>.
//! \details The library provides specialized SSE2, SSE4 and NEON version of the BLAKE2 compression
//!   function. For best results under ARM NEON, specify both an architecture and cpu. For example:
//!   <pre>CXXFLAGS="-DNDEBUG -march=armv8-a+crc -mcpu=cortex-a53 ..."</pre>
//! \since Crypto++ 5.6.4

#ifndef CRYPTOPP_BLAKE2_H
#define CRYPTOPP_BLAKE2_H

#include "cryptlib.h"
#include "secblock.h"
#include "seckey.h"

NAMESPACE_BEGIN(CryptoPP)

//! \class BLAKE2_Info
//! \brief BLAKE2 hash information
//! \tparam T_64bit flag indicating 64-bit
//! \since Crypto++ 5.6.4
template <bool T_64bit>
struct BLAKE2_Info : public VariableKeyLength<(T_64bit ? 64 : 32),0,(T_64bit ? 64 : 32),1,SimpleKeyingInterface::NOT_RESYNCHRONIZABLE>
{
	typedef VariableKeyLength<(T_64bit ? 64 : 32),0,(T_64bit ? 64 : 32),1,SimpleKeyingInterface::NOT_RESYNCHRONIZABLE> KeyBase;
	CRYPTOPP_CONSTANT(MIN_KEYLENGTH = KeyBase::MIN_KEYLENGTH)
	CRYPTOPP_CONSTANT(MAX_KEYLENGTH = KeyBase::MAX_KEYLENGTH)
	CRYPTOPP_CONSTANT(DEFAULT_KEYLENGTH = KeyBase::DEFAULT_KEYLENGTH)

	CRYPTOPP_CONSTANT(BLOCKSIZE = (T_64bit ? 128 : 64))
	CRYPTOPP_CONSTANT(DIGESTSIZE = (T_64bit ? 64 : 32))
	CRYPTOPP_CONSTANT(SALTSIZE = (T_64bit ? 16 : 8))
	CRYPTOPP_CONSTANT(PERSONALIZATIONSIZE = (T_64bit ? 16 : 8))

	CRYPTOPP_CONSTEXPR static const	char *StaticAlgorithmName() {return (T_64bit ? "BLAKE2b" : "BLAKE2s");}
};

//! \class BLAKE2_ParameterBlock
//! \brief BLAKE2 parameter block
//! \tparam T_64bit flag indicating 64-bit
//! \details BLAKE2b uses BLAKE2_ParameterBlock<true>, while BLAKE2s
//!   uses BLAKE2_ParameterBlock<false>.
//! \since Crypto++ 5.6.4
template <bool T_64bit>
struct CRYPTOPP_NO_VTABLE BLAKE2_ParameterBlock
{
};

//! \brief BLAKE2b parameter block specialization
template<>
struct CRYPTOPP_NO_VTABLE BLAKE2_ParameterBlock<true>
{
	CRYPTOPP_CONSTANT(SALTSIZE = BLAKE2_Info<true>::SALTSIZE)
	CRYPTOPP_CONSTANT(DIGESTSIZE = BLAKE2_Info<true>::DIGESTSIZE)
	CRYPTOPP_CONSTANT(PERSONALIZATIONSIZE = BLAKE2_Info<true>::PERSONALIZATIONSIZE)

	BLAKE2_ParameterBlock()
	{
		memset(this, 0x00, sizeof(*this));
		digestLength = DIGESTSIZE;
		fanout = depth = 1;
	}

	BLAKE2_ParameterBlock(size_t digestSize)
	{
		CRYPTOPP_ASSERT(digestSize <= DIGESTSIZE);
		memset(this, 0x00, sizeof(*this));
		digestLength = (byte)digestSize;
		fanout = depth = 1;
	}

	BLAKE2_ParameterBlock(size_t digestSize, size_t keyLength, const byte* salt, size_t saltLength,
		const byte* personalization, size_t personalizationLength);

	byte digestLength;
	byte keyLength, fanout, depth;
	byte leafLength[4];
	byte nodeOffset[8];
	byte nodeDepth, innerLength, rfu[14];
	byte salt[SALTSIZE];
	byte personalization[PERSONALIZATIONSIZE];
};

//! \brief BLAKE2s parameter block specialization
template<>
struct CRYPTOPP_NO_VTABLE BLAKE2_ParameterBlock<false>
{
	CRYPTOPP_CONSTANT(SALTSIZE = BLAKE2_Info<false>::SALTSIZE)
	CRYPTOPP_CONSTANT(DIGESTSIZE = BLAKE2_Info<false>::DIGESTSIZE)
	CRYPTOPP_CONSTANT(PERSONALIZATIONSIZE = BLAKE2_Info<false>::PERSONALIZATIONSIZE)

	BLAKE2_ParameterBlock()
	{
		memset(this, 0x00, sizeof(*this));
		digestLength = DIGESTSIZE;
		fanout = depth = 1;
	}

	BLAKE2_ParameterBlock(size_t digestSize)
	{
		CRYPTOPP_ASSERT(digestSize <= DIGESTSIZE);
		memset(this, 0x00, sizeof(*this));
		digestLength = (byte)digestSize;
		fanout = depth = 1;
	}

	BLAKE2_ParameterBlock(size_t digestSize, size_t keyLength, const byte* salt, size_t saltLength,
		const byte* personalization, size_t personalizationLength);

	byte digestLength;
	byte keyLength, fanout, depth;
	byte leafLength[4];
	byte nodeOffset[6];
	byte nodeDepth, innerLength;
	byte salt[SALTSIZE];
	byte personalization[PERSONALIZATIONSIZE];
};

//! \class BLAKE2_State
//! \brief BLAKE2 state information
//! \tparam W word type
//! \tparam T_64bit flag indicating 64-bit
//! \details BLAKE2b uses BLAKE2_State<word64, true>, while BLAKE2s
//!   uses BLAKE2_State<word32, false>.
//! \since Crypto++ 5.6.4
template <class W, bool T_64bit>
struct CRYPTOPP_NO_VTABLE BLAKE2_State
{
	CRYPTOPP_CONSTANT(BLOCKSIZE = BLAKE2_Info<T_64bit>::BLOCKSIZE)

	BLAKE2_State()
	{
		// Set all members except scratch buffer[]
		h[0]=h[1]=h[2]=h[3]=h[4]=h[5]=h[6]=h[7] = 0;
		t[0]=t[1]=f[0]=f[1] = 0;
		length = 0;
	}

	// SSE2, SSE4 and NEON depend upon t[] and f[] being side-by-side
	W h[8], t[2], f[2];
	byte  buffer[BLOCKSIZE];
	size_t length;
};

//! \class BLAKE2_Base
//! \brief BLAKE2 hash implementation
//! \tparam W word type
//! \tparam T_64bit flag indicating 64-bit
//! \details BLAKE2b uses BLAKE2_Base<word64, true>, while BLAKE2s
//!   uses BLAKE2_Base<word32, false>.
//! \since Crypto++ 5.6.4
template <class W, bool T_64bit>
class BLAKE2_Base : public SimpleKeyingInterfaceImpl<MessageAuthenticationCode, BLAKE2_Info<T_64bit> >
{
public:
	CRYPTOPP_CONSTANT(DEFAULT_KEYLENGTH = BLAKE2_Info<T_64bit>::DEFAULT_KEYLENGTH)
	CRYPTOPP_CONSTANT(MIN_KEYLENGTH = BLAKE2_Info<T_64bit>::MIN_KEYLENGTH)
	CRYPTOPP_CONSTANT(MAX_KEYLENGTH = BLAKE2_Info<T_64bit>::MAX_KEYLENGTH)

	CRYPTOPP_CONSTANT(DIGESTSIZE = BLAKE2_Info<T_64bit>::DIGESTSIZE)
	CRYPTOPP_CONSTANT(BLOCKSIZE = BLAKE2_Info<T_64bit>::BLOCKSIZE)
	CRYPTOPP_CONSTANT(SALTSIZE = BLAKE2_Info<T_64bit>::SALTSIZE)
	CRYPTOPP_CONSTANT(PERSONALIZATIONSIZE = BLAKE2_Info<T_64bit>::PERSONALIZATIONSIZE)

	typedef BLAKE2_State<W, T_64bit> State;
	typedef BLAKE2_ParameterBlock<T_64bit> ParameterBlock;
	typedef SecBlock<State, AllocatorWithCleanup<State, true> > AlignedState;
	typedef SecBlock<ParameterBlock, AllocatorWithCleanup<ParameterBlock, true> > AlignedParameterBlock;

	virtual ~BLAKE2_Base() {}

	//! \brief Retrieve the static algorithm name
	//! \returns the algorithm name (BLAKE2s or BLAKE2b)
	CRYPTOPP_CONSTEXPR static const char *StaticAlgorithmName() {return BLAKE2_Info<T_64bit>::StaticAlgorithmName();}

	//! \brief Retrieve the object's name
	//! \returns the object's algorithm name following RFC 7693
	//! \details Object algorithm name follows the naming described in
	//!   <A HREF="http://tools.ietf.org/html/rfc7693#section-4">RFC 7693, The BLAKE2 Cryptographic Hash and
	//! Message Authentication Code (MAC)</A>. For example, "BLAKE2b-512" and "BLAKE2s-256".
	std::string AlgorithmName() const {return std::string(StaticAlgorithmName()) + "-" + IntToString(this->DigestSize()*8);}

	unsigned int DigestSize() const {return m_digestSize;}
	unsigned int OptimalDataAlignment() const {return (CRYPTOPP_BOOL_ALIGN16 ? 16 : GetAlignmentOf<W>());}

	void Update(const byte *input, size_t length);
	void Restart();

	//! \brief Restart a hash with parameter block and counter
	//! \param block paramter block
	//! \param counter counter array
	//! \details Parameter block is persisted across calls to Restart().
	void Restart(const BLAKE2_ParameterBlock<T_64bit>& block, const W counter[2]);

	//! \brief Set tree mode
	//! \param mode the new tree mode
	//! \details BLAKE2 has two finalization flags, called State::f[0] and State::f[1].
	//!   If <tt>treeMode=false</tt> (default), then State::f[1] is never set. If
	//!   <tt>treeMode=true</tt>, then State::f[1] is set when State::f[0] is set.
	//!   Tree mode is persisted across calls to Restart().
	void SetTreeMode(bool mode) {m_treeMode=mode;}

	//! \brief Get tree mode
	//! \returns the current tree mode
	//! \details Tree mode is persisted across calls to Restart().
	bool GetTreeMode() const {return m_treeMode;}

	void TruncatedFinal(byte *hash, size_t size);

protected:
	BLAKE2_Base();
	BLAKE2_Base(bool treeMode, unsigned int digestSize);
	BLAKE2_Base(const byte *key, size_t keyLength, const byte* salt, size_t saltLength,
		const byte* personalization, size_t personalizationLength,
		bool treeMode, unsigned int digestSize);

	// Operates on state buffer and/or input. Must be BLOCKSIZE, final block will pad with 0's.
	void Compress(const byte *input);
	inline void IncrementCounter(size_t count=BLOCKSIZE);

	void UncheckedSetKey(const byte* key, unsigned int length, const CryptoPP::NameValuePairs& params);

private:
	AlignedState m_state;
	AlignedParameterBlock m_block;
	AlignedSecByteBlock m_key;
	word32 m_digestSize;
	bool m_treeMode;
};

//! \brief The BLAKE2b cryptographic hash function
//! \details BLAKE2b can function as both a hash and keyed hash. If you want only the hash,
//!   then use the BLAKE2b constructor that accepts no parameters or digest size. If you
//!   want a keyed hash, then use the constuctor that accpts the key as a parameter.
//!   Once a key and digest size are selected, its effectively immutable. The Restart()
//!   method that accepts a ParameterBlock does not allow you to change it.
//! \sa Aumasson, Neves, Wilcox-O'Hearn and Winnerlein's
//!   <A HREF="http://blake2.net/blake2.pdf">BLAKE2: simpler, smaller, fast as MD5</A> (2013.01.29).
//! \since Crypto++ 5.6.4
class BLAKE2b : public BLAKE2_Base<word64, true>
{
public:
	typedef BLAKE2_Base<word64, true> ThisBase; // Early Visual Studio workaround
	typedef BLAKE2_ParameterBlock<true> ParameterBlock;
	CRYPTOPP_COMPILE_ASSERT(sizeof(ParameterBlock) == 64);

	//! \brief Construct a BLAKE2b hash
	//! \param digestSize the digest size, in bytes
	//! \param treeMode flag indicating tree mode
	BLAKE2b(bool treeMode=false, unsigned int digestSize = DIGESTSIZE) : ThisBase(treeMode, digestSize) {}

	//! \brief Construct a BLAKE2b hash
	//! \param key a byte array used to key the cipher
	//! \param keyLength the size of the byte array
	//! \param salt a byte array used as salt
	//! \param saltLength the size of the byte array
	//! \param personalization a byte array used as prsonalization string
	//! \param personalizationLength the size of the byte array
	//! \param treeMode flag indicating tree mode
	//! \param digestSize the digest size, in bytes
	BLAKE2b(const byte *key, size_t keyLength, const byte* salt = NULL, size_t saltLength = 0,
		const byte* personalization = NULL, size_t personalizationLength = 0,
		bool treeMode=false, unsigned int digestSize = DIGESTSIZE)
		: ThisBase(key, keyLength, salt, saltLength, personalization, personalizationLength, treeMode, digestSize) {}
};

//! \brief The BLAKE2s cryptographic hash function
//! \details BLAKE2s can function as both a hash and keyed hash. If you want only the hash,
//!   then use the BLAKE2s constructor that accepts no parameters or digest size. If you
//!   want a keyed hash, then use the constuctor that accpts the key as a parameter.
//!   Once a key and digest size are selected, its effectively immutable. The Restart()
//!   method that accepts a ParameterBlock does not allow you to change it.
//! \sa Aumasson, Neves, Wilcox-O'Hearn and Winnerlein's
//!   <A HREF="http://blake2.net/blake2.pdf">BLAKE2: simpler, smaller, fast as MD5</A> (2013.01.29).
//! \since Crypto++ 5.6.4
class BLAKE2s : public BLAKE2_Base<word32, false>
{
public:
	typedef BLAKE2_Base<word32, false> ThisBase; // Early Visual Studio workaround
	typedef BLAKE2_ParameterBlock<false> ParameterBlock;
	CRYPTOPP_COMPILE_ASSERT(sizeof(ParameterBlock) == 32);

	//! \brief Construct a BLAKE2s hash
	//! \param digestSize the digest size, in bytes
	//! \param treeMode flag indicating tree mode
	BLAKE2s(bool treeMode=false, unsigned int digestSize = DIGESTSIZE) : ThisBase(treeMode, digestSize) {}

	//! \brief Construct a BLAKE2s hash
	//! \param key a byte array used to key the cipher
	//! \param keyLength the size of the byte array
	//! \param salt a byte array used as salt
	//! \param saltLength the size of the byte array
	//! \param personalization a byte array used as prsonalization string
	//! \param personalizationLength the size of the byte array
	//! \param treeMode flag indicating tree mode
	//! \param digestSize the digest size, in bytes
	BLAKE2s(const byte *key, size_t keyLength, const byte* salt = NULL, size_t saltLength = 0,
		const byte* personalization = NULL, size_t personalizationLength = 0,
		bool treeMode=false, unsigned int digestSize = DIGESTSIZE)
		: ThisBase(key, keyLength, salt, saltLength, personalization, personalizationLength, treeMode, digestSize) {}
};

NAMESPACE_END

#endif
