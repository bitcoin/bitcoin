// salsa.h - written and placed in the public domain by Wei Dai

//! \file salsa.h
//! \brief Classes for Salsa and Salsa20 stream ciphers

#ifndef CRYPTOPP_SALSA_H
#define CRYPTOPP_SALSA_H

#include "strciphr.h"
#include "secblock.h"

// TODO: work around GCC 4.8+ issue with SSE2 ASM until the exact details are known
//   and fix is released. Duplicate with "valgrind ./cryptest.exe tv salsa"
// "Inline assembly operands don't work with .intel_syntax", http://llvm.org/bugs/show_bug.cgi?id=24232
#if CRYPTOPP_BOOL_X32 || defined(CRYPTOPP_DISABLE_INTEL_ASM) || (CRYPTOPP_GCC_VERSION >= 40800)
# define CRYPTOPP_DISABLE_SALSA_ASM
#endif

NAMESPACE_BEGIN(CryptoPP)

//! \class Salsa20_Info
//! \brief Salsa20 stream cipher information
struct Salsa20_Info : public VariableKeyLength<32, 16, 32, 16, SimpleKeyingInterface::UNIQUE_IV, 8>
{
	CRYPTOPP_CONSTEXPR static const char *StaticAlgorithmName() {return "Salsa20";}
};

//! \class Salsa20_Policy
//! \brief Salsa20 stream cipher operation
class CRYPTOPP_NO_VTABLE Salsa20_Policy : public AdditiveCipherConcretePolicy<word32, 16>
{
protected:
	void CipherSetKey(const NameValuePairs &params, const byte *key, size_t length);
	void OperateKeystream(KeystreamOperation operation, byte *output, const byte *input, size_t iterationCount);
	void CipherResynchronize(byte *keystreamBuffer, const byte *IV, size_t length);
	bool CipherIsRandomAccess() const {return true;}
	void SeekToIteration(lword iterationCount);
#if (CRYPTOPP_BOOL_X86 || CRYPTOPP_BOOL_X32 || CRYPTOPP_BOOL_X64) && !defined(CRYPTOPP_DISABLE_SALSA_ASM)
	unsigned int GetAlignment() const;
	unsigned int GetOptimalBlockSize() const;
#endif

	FixedSizeAlignedSecBlock<word32, 16> m_state;
	int m_rounds;
};

//! \class Salsa20
//! \brief Salsa20 stream cipher
//! \details Salsa20 provides a variable number of rounds: 8, 12 or 20. The default number of rounds is 20.
//! \sa <a href="http://www.cryptolounge.org/wiki/XSalsa20">XSalsa20</a>
struct Salsa20 : public Salsa20_Info, public SymmetricCipherDocumentation
{
	typedef SymmetricCipherFinal<ConcretePolicyHolder<Salsa20_Policy, AdditiveCipherTemplate<> >, Salsa20_Info> Encryption;
	typedef Encryption Decryption;
};

//! \class XSalsa20_Info
//! \brief XSalsa20 stream cipher information
struct XSalsa20_Info : public FixedKeyLength<32, SimpleKeyingInterface::UNIQUE_IV, 24>
{
	CRYPTOPP_CONSTEXPR static const char *StaticAlgorithmName() {return "XSalsa20";}
};

//! \class XSalsa20_Policy
//! \brief XSalsa20 stream cipher operation
class CRYPTOPP_NO_VTABLE XSalsa20_Policy : public Salsa20_Policy
{
public:
	void CipherSetKey(const NameValuePairs &params, const byte *key, size_t length);
	void CipherResynchronize(byte *keystreamBuffer, const byte *IV, size_t length);

protected:
	FixedSizeSecBlock<word32, 8> m_key;
};

//! \class XSalsa20
//! \brief XSalsa20 stream cipher
//! \details XSalsa20 provides a variable number of rounds: 8, 12 or 20. The default number of rounds is 20.
//! \sa <a href="http://www.cryptolounge.org/wiki/XSalsa20">XSalsa20</a>
struct XSalsa20 : public XSalsa20_Info, public SymmetricCipherDocumentation
{
	typedef SymmetricCipherFinal<ConcretePolicyHolder<XSalsa20_Policy, AdditiveCipherTemplate<> >, XSalsa20_Info> Encryption;
	typedef Encryption Decryption;
};

NAMESPACE_END

#endif
