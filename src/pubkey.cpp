// Copyright (c) 2009-2020 The Bitcoin Core developers
// Copyright (c) 2017 The Zcash developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <pubkey.h>

#include <hash.h>
#include <secp256k1.h>
#include <secp256k1_extrakeys.h>
#include <secp256k1_recovery.h>
#include <secp256k1_schnorrsig.h>
#include <span.h>
#include <uint256.h>

#include <algorithm>
#include <cassert>

namespace
{
/* Global secp256k1_context object used for verification. */
secp256k1_context* secp256k1_context_verify = nullptr;
} // namespace

/** This function is taken from the libsecp256k1 distribution and implements
 *  DER parsing for ECDSA signatures, while supporting an arbitrary subset of
 *  format violations.
 *
 *  Supported violations include negative integers, excessive padding, garbage
 *  at the end, and overly long length descriptors. This is safe to use in
 *  Bitcoin because since the activation of BIP66, signatures are verified to be
 *  strict DER before being passed to this module, and we know it supports all
 *  violations present in the blockchain before that point.
 */
int ecdsa_signature_parse_der_lax(const secp256k1_context* ctx, secp256k1_ecdsa_signature* sig, const unsigned char *input, size_t inputlen) {
    size_t rpos, rlen, spos, slen;
    size_t pos = 0;
    size_t lenbyte;
    unsigned char tmpsig[64] = {0};
    int overflow = 0;

    /* Hack to initialize sig with a correctly-parsed but invalid signature. */
    secp256k1_ecdsa_signature_parse_compact(ctx, sig, tmpsig);

    /* Sequence tag byte */
    if (pos == inputlen || input[pos] != 0x30) {
        return 0;
    }
    pos++;

    /* Sequence length bytes */
    if (pos == inputlen) {
        return 0;
    }
    lenbyte = input[pos++];
    if (lenbyte & 0x80) {
        lenbyte -= 0x80;
        if (lenbyte > inputlen - pos) {
            return 0;
        }
        pos += lenbyte;
    }

    /* Integer tag byte for R */
    if (pos == inputlen || input[pos] != 0x02) {
        return 0;
    }
    pos++;

    /* Integer length for R */
    if (pos == inputlen) {
        return 0;
    }
    lenbyte = input[pos++];
    if (lenbyte & 0x80) {
        lenbyte -= 0x80;
        if (lenbyte > inputlen - pos) {
            return 0;
        }
        while (lenbyte > 0 && input[pos] == 0) {
            pos++;
            lenbyte--;
        }
        static_assert(sizeof(size_t) >= 4, "size_t too small");
        if (lenbyte >= 4) {
            return 0;
        }
        rlen = 0;
        while (lenbyte > 0) {
            rlen = (rlen << 8) + input[pos];
            pos++;
            lenbyte--;
        }
    } else {
        rlen = lenbyte;
    }
    if (rlen > inputlen - pos) {
        return 0;
    }
    rpos = pos;
    pos += rlen;

    /* Integer tag byte for S */
    if (pos == inputlen || input[pos] != 0x02) {
        return 0;
    }
    pos++;

    /* Integer length for S */
    if (pos == inputlen) {
        return 0;
    }
    lenbyte = input[pos++];
    if (lenbyte & 0x80) {
        lenbyte -= 0x80;
        if (lenbyte > inputlen - pos) {
            return 0;
        }
        while (lenbyte > 0 && input[pos] == 0) {
            pos++;
            lenbyte--;
        }
        static_assert(sizeof(size_t) >= 4, "size_t too small");
        if (lenbyte >= 4) {
            return 0;
        }
        slen = 0;
        while (lenbyte > 0) {
            slen = (slen << 8) + input[pos];
            pos++;
            lenbyte--;
        }
    } else {
        slen = lenbyte;
    }
    if (slen > inputlen - pos) {
        return 0;
    }
    spos = pos;

    /* Ignore leading zeroes in R */
    while (rlen > 0 && input[rpos] == 0) {
        rlen--;
        rpos++;
    }
    /* Copy R value */
    if (rlen > 32) {
        overflow = 1;
    } else {
        memcpy(tmpsig + 32 - rlen, input + rpos, rlen);
    }

    /* Ignore leading zeroes in S */
    while (slen > 0 && input[spos] == 0) {
        slen--;
        spos++;
    }
    /* Copy S value */
    if (slen > 32) {
        overflow = 1;
    } else {
        memcpy(tmpsig + 64 - slen, input + spos, slen);
    }

    if (!overflow) {
        overflow = !secp256k1_ecdsa_signature_parse_compact(ctx, sig, tmpsig);
    }
    if (overflow) {
        /* Overwrite the result again with a correctly-parsed but invalid
           signature if parsing failed. */
        memset(tmpsig, 0, 64);
        secp256k1_ecdsa_signature_parse_compact(ctx, sig, tmpsig);
    }
    return 1;
}

XOnlyPubKey::XOnlyPubKey(Span<const unsigned char> bytes)
{
    assert(bytes.size() == 32);
    std::copy(bytes.begin(), bytes.end(), m_keydata.begin());
}

bool XOnlyPubKey::IsFullyValid() const
{
    secp256k1_xonly_pubkey pubkey;
    return secp256k1_xonly_pubkey_parse(secp256k1_context_verify, &pubkey, m_keydata.data());
}

bool XOnlyPubKey::VerifySchnorr(const uint256& msg, Span<const unsigned char> sigbytes) const
{
    assert(sigbytes.size() == 64);
    secp256k1_xonly_pubkey pubkey;
    if (!secp256k1_xonly_pubkey_parse(secp256k1_context_verify, &pubkey, m_keydata.data())) return false;
    return secp256k1_schnorrsig_verify(secp256k1_context_verify, sigbytes.data(), msg.begin(), &pubkey);
}

static const CHashWriter HASHER_TAPTWEAK = TaggedHash("TapTweak");

uint256 XOnlyPubKey::ComputeTapTweakHash(const uint256* merkle_root) const
{
    if (merkle_root == nullptr) {
        // We have no scripts. The actual tweak does not matter, but follow BIP341 here to
        // allow for reproducible tweaking.
        return (CHashWriter(HASHER_TAPTWEAK) << m_keydata).GetSHA256();
    } else {
        return (CHashWriter(HASHER_TAPTWEAK) << m_keydata << *merkle_root).GetSHA256();
    }
}

bool XOnlyPubKey::CheckTapTweak(const XOnlyPubKey& internal, const uint256& merkle_root, bool parity) const
{
    secp256k1_xonly_pubkey internal_key;
    if (!secp256k1_xonly_pubkey_parse(secp256k1_context_verify, &internal_key, internal.data())) return false;
    uint256 tweak = internal.ComputeTapTweakHash(&merkle_root);
    return secp256k1_xonly_pubkey_tweak_add_check(secp256k1_context_verify, m_keydata.begin(), parity, &internal_key, tweak.begin());
}

std::optional<std::pair<XOnlyPubKey, bool>> XOnlyPubKey::CreateTapTweak(const uint256* merkle_root) const
{
    secp256k1_xonly_pubkey base_point;
    if (!secp256k1_xonly_pubkey_parse(secp256k1_context_verify, &base_point, data())) return std::nullopt;
    secp256k1_pubkey out;
    uint256 tweak = ComputeTapTweakHash(merkle_root);
    if (!secp256k1_xonly_pubkey_tweak_add(secp256k1_context_verify, &out, &base_point, tweak.data())) return std::nullopt;
    int parity = -1;
    std::pair<XOnlyPubKey, bool> ret;
    secp256k1_xonly_pubkey out_xonly;
    if (!secp256k1_xonly_pubkey_from_pubkey(secp256k1_context_verify, &out_xonly, &parity, &out)) return std::nullopt;
    secp256k1_xonly_pubkey_serialize(secp256k1_context_verify, ret.first.begin(), &out_xonly);
    assert(parity == 0 || parity == 1);
    ret.second = parity;
    return ret;
}


bool CPubKey::Verify(const uint256 &hash, const std::vector<unsigned char>& vchSig) const {
    if (!IsValid())
        return false;
    secp256k1_pubkey pubkey;
    secp256k1_ecdsa_signature sig;
    assert(secp256k1_context_verify && "secp256k1_context_verify must be initialized to use CPubKey.");
    if (!secp256k1_ec_pubkey_parse(secp256k1_context_verify, &pubkey, vch, size())) {
        return false;
    }
    if (!ecdsa_signature_parse_der_lax(secp256k1_context_verify, &sig, vchSig.data(), vchSig.size())) {
        return false;
    }
    /* libsecp256k1's ECDSA verification requires lower-S signatures, which have
     * not historically been enforced in Bitcoin, so normalize them first. */
    secp256k1_ecdsa_signature_normalize(secp256k1_context_verify, &sig, &sig);
    return secp256k1_ecdsa_verify(secp256k1_context_verify, &sig, hash.begin(), &pubkey);
}

bool CPubKey::RecoverCompact(const uint256 &hash, const std::vector<unsigned char>& vchSig) {
    if (vchSig.size() != COMPACT_SIGNATURE_SIZE)
        return false;
    int recid = (vchSig[0] - 27) & 3;
    bool fComp = ((vchSig[0] - 27) & 4) != 0;
    secp256k1_pubkey pubkey;
    secp256k1_ecdsa_recoverable_signature sig;
    assert(secp256k1_context_verify && "secp256k1_context_verify must be initialized to use CPubKey.");
    if (!secp256k1_ecdsa_recoverable_signature_parse_compact(secp256k1_context_verify, &sig, &vchSig[1], recid)) {
        return false;
    }
    if (!secp256k1_ecdsa_recover(secp256k1_context_verify, &pubkey, &sig, hash.begin())) {
        return false;
    }
    unsigned char pub[SIZE];
    size_t publen = SIZE;
    secp256k1_ec_pubkey_serialize(secp256k1_context_verify, pub, &publen, &pubkey, fComp ? SECP256K1_EC_COMPRESSED : SECP256K1_EC_UNCOMPRESSED);
    Set(pub, pub + publen);
    return true;
}

bool CPubKey::IsFullyValid() const {
    if (!IsValid())
        return false;
    secp256k1_pubkey pubkey;
    assert(secp256k1_context_verify && "secp256k1_context_verify must be initialized to use CPubKey.");
    return secp256k1_ec_pubkey_parse(secp256k1_context_verify, &pubkey, vch, size());
}

bool CPubKey::Decompress() {
    if (!IsValid())
        return false;
    secp256k1_pubkey pubkey;
    assert(secp256k1_context_verify && "secp256k1_context_verify must be initialized to use CPubKey.");
    if (!secp256k1_ec_pubkey_parse(secp256k1_context_verify, &pubkey, vch, size())) {
        return false;
    }
    unsigned char pub[SIZE];
    size_t publen = SIZE;
    secp256k1_ec_pubkey_serialize(secp256k1_context_verify, pub, &publen, &pubkey, SECP256K1_EC_UNCOMPRESSED);
    Set(pub, pub + publen);
    return true;
}

bool CPubKey::Derive(CPubKey& pubkeyChild, ChainCode &ccChild, unsigned int nChild, const ChainCode& cc) const {
    assert(IsValid());
    assert((nChild >> 31) == 0);
    assert(size() == COMPRESSED_SIZE);
    unsigned char out[64];
    BIP32Hash(cc, nChild, *begin(), begin()+1, out);
    memcpy(ccChild.begin(), out+32, 32);
    secp256k1_pubkey pubkey;
    assert(secp256k1_context_verify && "secp256k1_context_verify must be initialized to use CPubKey.");
    if (!secp256k1_ec_pubkey_parse(secp256k1_context_verify, &pubkey, vch, size())) {
        return false;
    }
    if (!secp256k1_ec_pubkey_tweak_add(secp256k1_context_verify, &pubkey, out)) {
        return false;
    }
    unsigned char pub[COMPRESSED_SIZE];
    size_t publen = COMPRESSED_SIZE;
    secp256k1_ec_pubkey_serialize(secp256k1_context_verify, pub, &publen, &pubkey, SECP256K1_EC_COMPRESSED);
    pubkeyChild.Set(pub, pub + publen);
    return true;
}

void CExtPubKey::Encode(unsigned char code[BIP32_EXTKEY_SIZE]) const {
    code[0] = nDepth;
    memcpy(code+1, vchFingerprint, 4);
    code[5] = (nChild >> 24) & 0xFF; code[6] = (nChild >> 16) & 0xFF;
    code[7] = (nChild >>  8) & 0xFF; code[8] = (nChild >>  0) & 0xFF;
    memcpy(code+9, chaincode.begin(), 32);
    assert(pubkey.size() == CPubKey::COMPRESSED_SIZE);
    memcpy(code+41, pubkey.begin(), CPubKey::COMPRESSED_SIZE);
}

void CExtPubKey::Decode(const unsigned char code[BIP32_EXTKEY_SIZE]) {
    nDepth = code[0];
    memcpy(vchFingerprint, code+1, 4);
    nChild = (code[5] << 24) | (code[6] << 16) | (code[7] << 8) | code[8];
    memcpy(chaincode.begin(), code+9, 32);
    pubkey.Set(code+41, code+BIP32_EXTKEY_SIZE);
}

bool CExtPubKey::Derive(CExtPubKey &out, unsigned int _nChild) const {
    out.nDepth = nDepth + 1;
    CKeyID id = pubkey.GetID();
    memcpy(out.vchFingerprint, &id, 4);
    out.nChild = _nChild;
    return pubkey.Derive(out.pubkey, out.chaincode, _nChild, chaincode);
}

/* static */ bool CPubKey::CheckLowS(const std::vector<unsigned char>& vchSig) {
    secp256k1_ecdsa_signature sig;
    assert(secp256k1_context_verify && "secp256k1_context_verify must be initialized to use CPubKey.");
    if (!ecdsa_signature_parse_der_lax(secp256k1_context_verify, &sig, vchSig.data(), vchSig.size())) {
        return false;
    }
    return (!secp256k1_ecdsa_signature_normalize(secp256k1_context_verify, nullptr, &sig));
}

/* static */ int ECCVerifyHandle::refcount = 0;

ECCVerifyHandle::ECCVerifyHandle()
{
    if (refcount == 0) {
        assert(secp256k1_context_verify == nullptr);
        secp256k1_context_verify = secp256k1_context_create(SECP256K1_CONTEXT_VERIFY);
        assert(secp256k1_context_verify != nullptr);
    }
    refcount++;
}

ECCVerifyHandle::~ECCVerifyHandle()
{
    refcount--;
    if (refcount == 0) {
        assert(secp256k1_context_verify != nullptr);
        secp256k1_context_destroy(secp256k1_context_verify);
        secp256k1_context_verify = nullptr;
    }
}
