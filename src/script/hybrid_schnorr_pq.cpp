#include "hybrid_schnorr_pq.h"
#include "falcon.h"
#include <cstring>

// Forward declare Schnorr verify (will use secp256k1 library later)
extern "C" int secp256k1_schnorrsig_verify(const unsigned char *sig, size_t siglen,
    const unsigned char *msg32, size_t msglen, const unsigned char *pubkey32);

bool VerifyHybridSchnorrPQ(const std::vector<unsigned char>& sig,
                           const std::vector<unsigned char>& msg,
                           const std::vector<unsigned char>& pubkey)
{
    // Composite format: schnorr_sig(64) || falcon_sig(rest)
    size_t schnorr_len = 64;
    if (sig.size() < schnorr_len + 1 || pubkey.size() < 32) return false;
    
    // Extract keys: first 32 bytes = Schnorr pubkey, rest = Falcon pubkey
    std::vector<unsigned char> schnorr_pk(pubkey.begin(), pubkey.begin() + 32);
    std::vector<unsigned char> falcon_pk(pubkey.begin() + 32, pubkey.end());
    
    // Extract signatures
    std::vector<unsigned char> schnorr_sig(sig.begin(), sig.begin() + schnorr_len);
    std::vector<unsigned char> falcon_sig(sig.begin() + schnorr_len, sig.end());
    
    // Verify Schnorr (BIP 340 style)
    if (!secp256k1_schnorrsig_verify(schnorr_sig.data(), schnorr_sig.size(),
                                       msg.data(), msg.size(),
                                       schnorr_pk.data()))
        return false;
    
    // Verify Falcon-1024
    return Falcon1024Verify(falcon_sig, msg, falcon_pk);
}
