// fips140.h - written and placed in the public domain by Wei Dai

//! \file fips140.h
//! \brief Classes and functions for the FIPS 140-2 validated library
//! \details The FIPS validated library is only available on Windows as a DLL. Once compiled,
//!   the library is always in FIPS mode contingent upon successful execution of
//!   DoPowerUpSelfTest() or DoDllPowerUpSelfTest().
//! \sa <A HREF="http://cryptopp.com/wiki/Visual_Studio">Visual Studio</A> and
//!   <A HREF="http://cryptopp.com/wiki/config.h">config.h</A> on the Crypto++ wiki.

#ifndef CRYPTOPP_FIPS140_H
#define CRYPTOPP_FIPS140_H

#include "cryptlib.h"
#include "secblock.h"

NAMESPACE_BEGIN(CryptoPP)

//! \class SelfTestFailure
//! Exception thrown when a crypto algorithm is used after a self test fails
//! \details The self tests for an algorithm are performed by Algortihm class
//!   when CRYPTOPP_ENABLE_COMPLIANCE_WITH_FIPS_140_2 is defined.
class CRYPTOPP_DLL SelfTestFailure : public Exception
{
public:
	explicit SelfTestFailure(const std::string &s) : Exception(OTHER_ERROR, s) {}
};

//! \brief Determines whether the library provides FIPS validated cryptography
//! \returns true if FIPS 140-2 validated features were enabled at compile time.
//! \details true if FIPS 140-2 validated features were enabled at compile time,
//!   false otherwise.
//! \note FIPS mode is enabled at compile time. A program or other module cannot
//!   arbitrarily enter or exit the mode.
CRYPTOPP_DLL bool CRYPTOPP_API FIPS_140_2_ComplianceEnabled();

//! \brief Status of the power-up self test
enum PowerUpSelfTestStatus {

	//! \brief The self tests have not been performed.
	POWER_UP_SELF_TEST_NOT_DONE,
	//! \brief The self tests were executed via DoPowerUpSelfTest() or
	//!   DoDllPowerUpSelfTest(), but the result was failure.
	POWER_UP_SELF_TEST_FAILED,
	//! \brief The self tests were executed via DoPowerUpSelfTest() or
	//!   DoDllPowerUpSelfTest(), and the result was success.
	POWER_UP_SELF_TEST_PASSED
};

//! \brief Performs the power-up self test
//! \param moduleFilename the fully qualified name of the module
//! \param expectedModuleMac the expected MAC of the components protected by the integrity check
//! \details Performs the power-up self test, and sets the self test status to
//!   POWER_UP_SELF_TEST_PASSED or POWER_UP_SELF_TEST_FAILED.
//! \details The self tests for an algorithm are performed by the Algortihm class
//!   when CRYPTOPP_ENABLE_COMPLIANCE_WITH_FIPS_140_2 is defined.
CRYPTOPP_DLL void CRYPTOPP_API DoPowerUpSelfTest(const char *moduleFilename, const byte *expectedModuleMac);

//! \brief Performs the power-up self test on the DLL
//! \details Performs the power-up self test using the filename of this DLL and the
//!   embedded module MAC, and sets the self test status to POWER_UP_SELF_TEST_PASSED or
//!   POWER_UP_SELF_TEST_FAILED.
//! \details The self tests for an algorithm are performed by the Algortihm class
//!   when CRYPTOPP_ENABLE_COMPLIANCE_WITH_FIPS_140_2 is defined.
CRYPTOPP_DLL void CRYPTOPP_API DoDllPowerUpSelfTest();

//! \brief Sets the power-up self test status to POWER_UP_SELF_TEST_FAILED
//! \details Sets the power-up self test status to POWER_UP_SELF_TEST_FAILED to simulate failure.
CRYPTOPP_DLL void CRYPTOPP_API SimulatePowerUpSelfTestFailure();

//! \brief Provides the current power-up self test status
//! \returns the current power-up self test status
CRYPTOPP_DLL PowerUpSelfTestStatus CRYPTOPP_API GetPowerUpSelfTestStatus();

#ifndef CRYPTOPP_DOXYGEN_PROCESSING
typedef PowerUpSelfTestStatus (CRYPTOPP_API * PGetPowerUpSelfTestStatus)();
#endif

//! \brief Class object that calculates the MAC on the module
//! \returns the MAC for the module
CRYPTOPP_DLL MessageAuthenticationCode * CRYPTOPP_API NewIntegrityCheckingMAC();

//! \brief Verifies the MAC on the module
//! \param moduleFilename the fully qualified name of the module
//! \param expectedModuleMac the expected MAC of the components protected by the integrity check
//! \param pActualMac the actual MAC of the components calculated by the integrity check
//! \param pMacFileLocation the offest of the MAC in the PE/PE+ module
//! \returns true if the MAC is valid, false otherwise
CRYPTOPP_DLL bool CRYPTOPP_API IntegrityCheckModule(const char *moduleFilename, const byte *expectedModuleMac, SecByteBlock *pActualMac = NULL, unsigned long *pMacFileLocation = NULL);

#ifndef CRYPTOPP_DOXYGEN_PROCESSING
// this is used by Algorithm constructor to allow Algorithm objects to be constructed for the self test
bool PowerUpSelfTestInProgressOnThisThread();

void SetPowerUpSelfTestInProgressOnThisThread(bool inProgress);

void SignaturePairwiseConsistencyTest(const PK_Signer &signer, const PK_Verifier &verifier);
void EncryptionPairwiseConsistencyTest(const PK_Encryptor &encryptor, const PK_Decryptor &decryptor);

void SignaturePairwiseConsistencyTest_FIPS_140_Only(const PK_Signer &signer, const PK_Verifier &verifier);
void EncryptionPairwiseConsistencyTest_FIPS_140_Only(const PK_Encryptor &encryptor, const PK_Decryptor &decryptor);
#endif

//! \brief The placeholder used prior to embedding the actual MAC in the module.
//! \details After the DLL is built but before it is MAC'd, the string CRYPTOPP_DUMMY_DLL_MAC
//!   is used as a placeholder for the actual MAC. A post-build step is performed which calculates
//!   the MAC of the DLL and embeds it in the module. The actual MAC is written by the
//!   <tt>cryptest.exe</tt> program using the <tt>mac_dll</tt> subcommand.
#define CRYPTOPP_DUMMY_DLL_MAC "MAC_51f34b8db820ae8"

NAMESPACE_END

#endif
