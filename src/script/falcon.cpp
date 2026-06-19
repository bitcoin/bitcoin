#include "falcon.h"
#include <oqs/oqs.h>

bool Falcon1024Keygen(std::vector<unsigned char>& pk, std::vector<unsigned char>& sk)
{
    pk.resize(OQS_SIG_falcon_1024_length_public_key);
    sk.resize(OQS_SIG_falcon_1024_length_secret_key);
    return OQS_SIG_falcon_1024_keypair(pk.data(), sk.data()) == OQS_SUCCESS;
}

bool Falcon1024Sign(const std::vector<unsigned char>& msg,
                     const std::vector<unsigned char>& sk,
                     std::vector<unsigned char>& sig)
{
    sig.resize(OQS_SIG_falcon_1024_length_signature);
    size_t siglen = sig.size();
    if (OQS_SIG_falcon_1024_sign(sig.data(), &siglen,
                                   msg.data(), msg.size(),
                                   sk.data()) != OQS_SUCCESS)
        return false;
    // Resize to actual signature length
    sig.resize(siglen);
    return true;
}

bool Falcon1024Verify(const std::vector<unsigned char>& sig,
                       const std::vector<unsigned char>& msg,
                       const std::vector<unsigned char>& pk)
{
    return OQS_SIG_falcon_1024_verify(msg.data(), msg.size(),
                                        sig.data(), sig.size(),
                                        pk.data()) == OQS_SUCCESS;
}
