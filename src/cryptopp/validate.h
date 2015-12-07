#ifndef CRYPTOPP_VALIDATE_H
#define CRYPTOPP_VALIDATE_H

#include "cryptlib.h"

bool ValidateAll(bool thorough);
bool TestSettings();
bool TestOS_RNG();
bool TestAutoSeeded();

#if (CRYPTOPP_BOOL_X86 || CRYPTOPP_BOOL_X32 || CRYPTOPP_BOOL_X64)
bool TestRDRAND();
bool TestRDSEED();
#endif

bool ValidateBaseCode();
bool ValidateCRC32();
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

#if !defined(NDEBUG)
bool TestPolynomialMod2();
#endif

// Coverity findings
template <class T, bool NON_NEGATIVE>
T StringToValue(const std::string& str);
template<>
int StringToValue<int, true>(const std::string& str);

// Functions that need a RNG; uses AES inf CFB mode with Seed.
CryptoPP::RandomNumberGenerator & GlobalRNG();

bool RunTestDataFile(const char *filename, const CryptoPP::NameValuePairs &overrideParameters=CryptoPP::g_nullNameValuePairs, bool thorough=true);

#endif
