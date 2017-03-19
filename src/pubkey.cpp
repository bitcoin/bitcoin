// Copyright (c) 2009-2016 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "pubkey.h"
#include "BitcoinAddress.h"
#include "PubKeyUtil.h"
#include <secp256k1.h>
#include <secp256k1_recovery.h>

base58string CKeyID::GetBase58addressWithNetworkPubkeyPrefix() const
{
    base58string str;
    str.SetData(Params().Base58Prefix(CChainParams::PUBKEY_ADDRESS), this, 20);
    return str;
}

/* Global secp256k1_context object used for verification. */
secp256k1_context* secp256k1_context_verify = NULL;

bool CPubKey::Verify(const uint256 &hash, const std::vector<unsigned char>& vchSig) const {
    if (!IsValid())
        return false;
    secp256k1_pubkey pubkey;
    secp256k1_ecdsa_signature sig;
    if (!secp256k1_ec_pubkey_parse(secp256k1_context_verify, &pubkey, &(*this)[0], size())) {
        return false;
    }
    if (vchSig.size() == 0) {
        return false;
    }
    if (!CPubKeyUtil::ecdsa_signature_parse_der_lax(secp256k1_context_verify, &sig, &vchSig[0], vchSig.size())) {
        return false;
    }
    /* libsecp256k1's ECDSA verification requires lower-S signatures, which have
     * not historically been enforced in Bitcoin, so normalize them first. */
    secp256k1_ecdsa_signature_normalize(secp256k1_context_verify, &sig, &sig);
    return secp256k1_ecdsa_verify(secp256k1_context_verify, &sig, hash.begin(), &pubkey);
}

bool CPubKey::RecoverCompact(const uint256 &hash, const std::vector<unsigned char>& vchSig) {
    if (vchSig.size() != 65)
        return false;
    int recid = (vchSig[0] - 27) & 3;
    bool fComp = ((vchSig[0] - 27) & 4) != 0;
    secp256k1_pubkey pubkey;
    secp256k1_ecdsa_recoverable_signature sig;
    if (!secp256k1_ecdsa_recoverable_signature_parse_compact(secp256k1_context_verify, &sig, &vchSig[1], recid)) {
        return false;
    }
    if (!secp256k1_ecdsa_recover(secp256k1_context_verify, &pubkey, &sig, hash.begin())) {
        return false;
    }
    unsigned char pub[65];
    size_t publen = 65;
    secp256k1_ec_pubkey_serialize(secp256k1_context_verify, pub, &publen, &pubkey, fComp ? SECP256K1_EC_COMPRESSED : SECP256K1_EC_UNCOMPRESSED);
    Set(pub, pub + publen);
    return true;
}

bool CPubKey::IsFullyValid() const {
    if (!IsValid())
        return false;
    secp256k1_pubkey pubkey;
    return secp256k1_ec_pubkey_parse(secp256k1_context_verify, &pubkey, &(*this)[0], size());
}

bool CPubKey::Decompress() {
    if (!IsValid())
        return false;
    secp256k1_pubkey pubkey;
    if (!secp256k1_ec_pubkey_parse(secp256k1_context_verify, &pubkey, &(*this)[0], size())) {
        return false;
    }
    unsigned char pub[65];
    size_t publen = 65;
    secp256k1_ec_pubkey_serialize(secp256k1_context_verify, pub, &publen, &pubkey, SECP256K1_EC_UNCOMPRESSED);
    Set(pub, pub + publen);
    return true;
}

bool CPubKey::Derive(CPubKey& pubkeyChild, ChainCode &ccChild, unsigned int nChild, const ChainCode& cc) const {
    assert(IsValid());
    assert((nChild >> 31) == 0);
    assert(begin() + 33 == end());
    unsigned char out[64];
    BIP32Hash(cc, nChild, *begin(), begin()+1, out);
    memcpy(ccChild.begin(), out+32, 32);
    secp256k1_pubkey pubkey;
    if (!secp256k1_ec_pubkey_parse(secp256k1_context_verify, &pubkey, &(*this)[0], size())) {
        return false;
    }
    if (!secp256k1_ec_pubkey_tweak_add(secp256k1_context_verify, &pubkey, out)) {
        return false;
    }
    unsigned char pub[33];
    size_t publen = 33;
    secp256k1_ec_pubkey_serialize(secp256k1_context_verify, pub, &publen, &pubkey, SECP256K1_EC_COMPRESSED);
    pubkeyChild.Set(pub, pub + publen);
    return true;
}

/* static */ int ECCVerifyHandle::refcount = 0;

ECCVerifyHandle::ECCVerifyHandle()
{
    if (refcount == 0) {
        assert(secp256k1_context_verify == NULL);
        secp256k1_context_verify = secp256k1_context_create(SECP256K1_CONTEXT_VERIFY);
        assert(secp256k1_context_verify != NULL);
    }
    refcount++;
}

ECCVerifyHandle::~ECCVerifyHandle()
{
    refcount--;
    if (refcount == 0) {
        assert(secp256k1_context_verify != NULL);
        secp256k1_context_destroy(secp256k1_context_verify);
        secp256k1_context_verify = NULL;
    }
}
