// fips140.cpp - written and placed in the public domain by Wei Dai

#include "pch.h"

#ifndef CRYPTOPP_IMPORTS

#include "fips140.h"
#include "misc.h"
#include "trdlocal.h"	// needs to be included last for cygwin

NAMESPACE_BEGIN(CryptoPP)

// Define this to 1 to turn on FIPS 140-2 compliance features, including additional tests during 
// startup, random number generation, and key generation. These tests may affect performance.
#ifndef CRYPTOPP_ENABLE_COMPLIANCE_WITH_FIPS_140_2
#define CRYPTOPP_ENABLE_COMPLIANCE_WITH_FIPS_140_2 0
#endif

#if (CRYPTOPP_ENABLE_COMPLIANCE_WITH_FIPS_140_2 && !defined(THREADS_AVAILABLE))
#error FIPS 140-2 compliance requires the availability of thread local storage.
#endif

#if (CRYPTOPP_ENABLE_COMPLIANCE_WITH_FIPS_140_2 && !defined(OS_RNG_AVAILABLE))
#error FIPS 140-2 compliance requires the availability of OS provided RNG.
#endif

PowerUpSelfTestStatus g_powerUpSelfTestStatus = POWER_UP_SELF_TEST_NOT_DONE;

bool FIPS_140_2_ComplianceEnabled()
{
	return CRYPTOPP_ENABLE_COMPLIANCE_WITH_FIPS_140_2;
}

void SimulatePowerUpSelfTestFailure()
{
	g_powerUpSelfTestStatus = POWER_UP_SELF_TEST_FAILED;
}

PowerUpSelfTestStatus CRYPTOPP_API GetPowerUpSelfTestStatus()
{
	return g_powerUpSelfTestStatus;
}

#if CRYPTOPP_ENABLE_COMPLIANCE_WITH_FIPS_140_2
ThreadLocalStorage & AccessPowerUpSelfTestInProgress()
{
	static ThreadLocalStorage selfTestInProgress;
	return selfTestInProgress;
}
#endif

bool PowerUpSelfTestInProgressOnThisThread()
{
#if CRYPTOPP_ENABLE_COMPLIANCE_WITH_FIPS_140_2
	return AccessPowerUpSelfTestInProgress().GetValue() != NULL;
#else
	assert(false);	// should not be called
	return false;
#endif
}

void SetPowerUpSelfTestInProgressOnThisThread(bool inProgress)
{
	CRYPTOPP_UNUSED(inProgress);
#if CRYPTOPP_ENABLE_COMPLIANCE_WITH_FIPS_140_2
	AccessPowerUpSelfTestInProgress().SetValue((void *)inProgress);
#endif
}

void EncryptionPairwiseConsistencyTest_FIPS_140_Only(const PK_Encryptor &encryptor, const PK_Decryptor &decryptor)
{
	CRYPTOPP_UNUSED(encryptor), CRYPTOPP_UNUSED(decryptor);
#if CRYPTOPP_ENABLE_COMPLIANCE_WITH_FIPS_140_2
	EncryptionPairwiseConsistencyTest(encryptor, decryptor);
#endif
}

void SignaturePairwiseConsistencyTest_FIPS_140_Only(const PK_Signer &signer, const PK_Verifier &verifier)
{
	CRYPTOPP_UNUSED(signer), CRYPTOPP_UNUSED(verifier);
#if CRYPTOPP_ENABLE_COMPLIANCE_WITH_FIPS_140_2
	SignaturePairwiseConsistencyTest(signer, verifier);
#endif
}

NAMESPACE_END

#endif
