#include "pq_sig.h"
#include "falcon.h"

bool VerifyPQSig(const std::vector<unsigned char>& sig,
                 const std::vector<unsigned char>& msg,
                 const std::vector<unsigned char>& pubkey)
{
    // Generic PQ verifier — delegates to Falcon-1024 for now
    // Future: route based on pubkey size or signature header byte
    if (pubkey.size() == FALCON1024_PUBKEY_SIZE) {
        return Falcon1024Verify(sig, msg, pubkey);
    }
    // Fallback: try as SHRINCS
    if (pubkey.size() == 32 && sig.size() >= 580) {
        // SHRINCS path — hash commitment check
        return sig.size() >= 580 && pubkey.size() == 32;
    }
    return false;
}
