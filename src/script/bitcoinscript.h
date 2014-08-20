#ifndef H_BITCOIN_BITCOINSCRIPT
#define H_BITCOIN_BITCOINSCRIPT

#ifdef __cplusplus
extern "C" {
#else
#include <stdbool.h>
#endif

/** Script verification flags */
enum
{
    SCRIPT_VERIFY_NONE      = 0,
    SCRIPT_VERIFY_P2SH      = (1U << 0), // evaluate P2SH (BIP16) subscripts
    SCRIPT_VERIFY_STRICTENC = (1U << 1), // enforce strict conformance to DER and SEC2 for signatures and pubkeys
    SCRIPT_VERIFY_LOW_S     = (1U << 2), // enforce low S values (<n/2) in signatures (depends on STRICTENC)
    SCRIPT_VERIFY_NULLDUMMY = (1U << 4), // verify dummy stack item consumed by CHECKMULTISIG is of zero-length

    // Mandatory script verification flags that all new blocks must comply with for
    // them to be valid. (but old blocks may not comply with) Currently just P2SH,
    // but in the future other flags may be added, such as a soft-fork to enforce
    // strict DER encoding.
    //
    // Failing one of these tests may trigger a DoS ban - see CheckInputs() for
    // details.
    MANDATORY_SCRIPT_VERIFY_FLAGS = SCRIPT_VERIFY_P2SH,

    // Standard script verification flags that standard transactions will comply
    // with. However scripts violating these flags may still be present in valid
    // blocks and we must accept those blocks.
    STANDARD_SCRIPT_VERIFY_FLAGS = MANDATORY_SCRIPT_VERIFY_FLAGS |
                                   SCRIPT_VERIFY_STRICTENC |
                                   SCRIPT_VERIFY_NULLDUMMY,
};

bool VerifyScript(const unsigned char *scriptPubKey, const unsigned int scriptPubKeyLen,
                  const unsigned char *txTo,         const unsigned int txToLen,
                  const unsigned int nIn, const unsigned int flags);

#ifdef __cplusplus
}
#endif

#endif
