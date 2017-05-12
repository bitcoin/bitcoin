#ifndef CRYPTOPP_VALIDATE_H
#define CRYPTOPP_VALIDATE_H

#include "cryptlib.h"
#include <iostream>
#include <iomanip>

bool ValidateAll(bool thorough);
bool TestSettings();
bool TestOS_RNG();
bool TestAutoSeeded();
bool TestAutoSeededX917();

#if (CRYPTOPP_BOOL_X86 || CRYPTOPP_BOOL_X32 || CRYPTOPP_BOOL_X64)
bool TestRDRAND();
bool TestRDSEED();
#endif

bool ValidateBaseCode();
bool ValidateCRC32();
bool ValidateCRC32C();
bool ValidateAdler32();
bool ValidateMD2();
bool ValidateMD4();
bool ValidateMD5();
bool ValidateSHA();
bool ValidateSHA2();
bool ValidateTiger();
bool ValidateRIPEMD();
bool ValidatePanama();
bool ValidateWhirlpool();
bool ValidateBLAKE2s();
bool ValidateBLAKE2b();

bool ValidateHMAC();
bool ValidateTTMAC();

bool ValidateCipherModes();
bool ValidatePBKDF();
bool ValidateHKDF();

bool ValidateDES();
bool ValidateIDEA();
bool ValidateSAFER();
bool ValidateRC2();
bool ValidateARC4();

bool ValidateRC5();
bool ValidateBlowfish();
bool ValidateThreeWay();
bool ValidateGOST();
bool ValidateSHARK();
bool ValidateSEAL();
bool ValidateCAST();
bool ValidateSquare();
bool ValidateSKIPJACK();
bool ValidateRC6();
bool ValidateMARS();
bool ValidateRijndael();
bool ValidateTwofish();
bool ValidateSerpent();
bool ValidateSHACAL2();
bool ValidateCamellia();
bool ValidateSalsa();
bool ValidateSosemanuk();
bool ValidateVMAC();
bool ValidateCCM();
bool ValidateGCM();
bool ValidateCMAC();

bool ValidateBBS();
bool ValidateDH();
bool ValidateMQV();
bool ValidateHMQV();
bool ValidateFHMQV();
bool ValidateRSA();
bool ValidateElGamal();
bool ValidateDLIES();
bool ValidateNR();
bool ValidateDSA(bool thorough);
bool ValidateLUC();
bool ValidateLUC_DL();
bool ValidateLUC_DH();
bool ValidateXTR_DH();
bool ValidateRabin();
bool ValidateRW();
//bool ValidateBlumGoldwasser();
bool ValidateECP();
bool ValidateEC2N();
bool ValidateECDSA();
bool ValidateESIGN();

#if CRYPTOPP_DEBUG
bool TestSecBlock();
bool TestPolynomialMod2();
bool TestHuffmanCodes();
#endif

// Coverity finding
template <class T, bool NON_NEGATIVE>
T StringToValue(const std::string& str);

// Coverity finding
template<>
int StringToValue<int, true>(const std::string& str);

// Coverity finding
class StreamState
{
public:
	StreamState(std::ostream& out)
		: m_out(out), m_fmt(out.flags()), m_prec(out.precision())
	{
	}

	~StreamState()
	{
		m_out.precision(m_prec);
		m_out.flags(m_fmt);
	}

private:
	std::ostream& m_out;
	std::ios_base::fmtflags m_fmt;
	std::streamsize m_prec;
};

// Functions that need a RNG; uses AES inf CFB mode with Seed.
CryptoPP::RandomNumberGenerator & GlobalRNG();

bool RunTestDataFile(const char *filename, const CryptoPP::NameValuePairs &overrideParameters=CryptoPP::g_nullNameValuePairs, bool thorough=true);

#endif
