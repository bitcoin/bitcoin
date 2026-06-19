#include "hybrid_schnorr_pq.h"
#include "falcon.h"
#include <secp256k1_schnorrsig.h>
#include <secp256k1.h>

extern secp256k1_context* secp256k1_context_verify;

bool VerifyHybridSchnorrPQ(const std::vector<unsigned char>& sig,
                           const std::vector<unsigned char>& msg,
                           const std::vector<unsigned char>& pubkey)
{
    // Composite: schnorr_sig(64) || falcon_sig(rest)
    size_t schnorr_len = 64;
    if (sig.size() < schnorr_len + 1 || pubkey.size() < 32) return false;
    
    // Schnorr public key: first 32 bytes
    // Falcon public key: remaining bytes
    std::vector<unsigned char> schnorr_pk(pubkey.begin(), pubkey.begin() + 32);
    std::vector<unsigned char> falcon_pk(pubkey.begin() + 32, pubkey.end());
    std::vector<unsigned char> schnorr_sig(sig.begin(), sig.begin() + schnorr_len);
    std::vector<unsigned char> falcon_sig(sig.begin() + schnorr_len, sig.end());
    
    // BIP 340 Schnorr verify via secp256k1
    if (!secp256k1_schnorrsig_verify(secp256k1_context_verify,
                                       schnorr_sig.data(),
                                       msg.data(), msg.size(),
                                       schnorr_pk.data()))
        return false;
    
    // Falcon-1024 verify
    return Falcon1024Verify(falcon_sig, msg, falcon_pk);
}
