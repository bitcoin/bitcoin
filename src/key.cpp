// Copyright (c) 2009-2014 The Bitcoin developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "key.h"

#include "crypto/sha2.h"
#include "random.h"

#ifdef USE_SECP256K1
#include <secp256k1.h>
#else
#include "ecwrapper.h"
#endif

//! anonymous namespace with local implementation code (OpenSSL interaction)
namespace {

#ifdef USE_SECP256K1
#include <secp256k1.h>
class CSecp256k1Init {
public:
    CSecp256k1Init() {
        secp256k1_start();
    }
    ~CSecp256k1Init() {
        secp256k1_stop();
    }
};
static CSecp256k1Init instance_of_csecp256k1;

#endif

int CompareBigEndian(const unsigned char *c1, size_t c1len, const unsigned char *c2, size_t c2len) {
    while (c1len > c2len) {
        if (*c1)
            return 1;
        c1++;
        c1len--;
    }
    while (c2len > c1len) {
        if (*c2)
            return -1;
        c2++;
        c2len--;
    }
    while (c1len > 0) {
        if (*c1 > *c2)
            return 1;
        if (*c2 > *c1)
            return -1;
        c1++;
        c2++;
        c1len--;
    }
    return 0;
}

/** Order of secp256k1's generator minus 1. */
const unsigned char vchMaxModOrder[32] = {
    0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,
    0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFE,
    0xBA,0xAE,0xDC,0xE6,0xAF,0x48,0xA0,0x3B,
    0xBF,0xD2,0x5E,0x8C,0xD0,0x36,0x41,0x40
};

/** Half of the order of secp256k1's generator minus 1. */
const unsigned char vchMaxModHalfOrder[32] = {
    0x7F,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,
    0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,
    0x5D,0x57,0x6E,0x73,0x57,0xA4,0x50,0x1D,
    0xDF,0xE9,0x2F,0x46,0x68,0x1B,0x20,0xA0
};

const unsigned char vchZero[1] = {0};

} // anon namespace

bool CKey::Check(const unsigned char *vch) {
    return CompareBigEndian(vch, 32, vchZero, 0) > 0 &&
           CompareBigEndian(vch, 32, vchMaxModOrder, 32) <= 0;
}

bool CKey::CheckSignatureElement(const unsigned char *vch, int len, bool half) {
    return CompareBigEndian(vch, len, vchZero, 0) > 0 &&
           CompareBigEndian(vch, len, half ? vchMaxModHalfOrder : vchMaxModOrder, 32) <= 0;
}

void CKey::MakeNewKey(bool fCompressedIn) {
    do {
        GetRandBytes(vch, sizeof(vch));
    } while (!Check(vch));
    fValid = true;
    fCompressed = fCompressedIn;
}

bool CKey::SetPrivKey(const CPrivKey &privkey, bool fCompressedIn) {
#ifdef USE_SECP256K1
    if (!secp256k1_ecdsa_privkey_import((unsigned char*)begin(), &privkey[0], privkey.size()))
        return false;
#else
    CECKey key;
    if (!key.SetPrivKey(&privkey[0], privkey.size()))
        return false;
    key.GetSecretBytes(vch);
#endif
    fCompressed = fCompressedIn;
    fValid = true;
    return true;
}

CPrivKey CKey::GetPrivKey() const {
    assert(fValid);
    CPrivKey privkey;
    int privkeylen, ret;
#ifdef USE_SECP256K1
    privkey.resize(279);
    privkeylen = 279;
    ret = secp256k1_ecdsa_privkey_export(begin(), (unsigned char*)&privkey[0], &privkeylen, fCompressed);
    assert(ret);
    privkey.resize(privkeylen);
#else
    CECKey key;
    key.SetSecretBytes(vch);
    privkeylen = key.GetPrivKeySize(fCompressed);
    assert(privkeylen);
    privkey.resize(privkeylen);
    ret = key.GetPrivKey(&privkey[0], fCompressed);
    assert(ret == (int)privkey.size());
#endif
    return privkey;
}

CPubKey CKey::GetPubKey() const {
    assert(fValid);
    CPubKey result;
#ifdef USE_SECP256K1
    int clen = 65;
    int ret = secp256k1_ecdsa_pubkey_create((unsigned char*)result.begin(), &clen, begin(), fCompressed);
    assert((int)result.size() == clen);
    assert(ret);
#else
    std::vector<unsigned char> pubkey;
    CECKey key;
    key.SetSecretBytes(vch);
    key.GetPubKey(pubkey, fCompressed);
    result.Set(pubkey.begin(), pubkey.end());
#endif
    assert(result.IsValid());
    return result;
}

bool CKey::Sign(const uint256 &hash, std::vector<unsigned char>& vchSig, bool lowS) const {
    if (!fValid)
        return false;
#ifdef USE_SECP256K1
    vchSig.resize(72);
    int nSigLen = 72;
    CKey nonce;
    do {
        nonce.MakeNewKey(true);
        if (secp256k1_ecdsa_sign((const unsigned char*)&hash, 32, (unsigned char*)&vchSig[0], &nSigLen, begin(), nonce.begin()))
            break;
    } while(true);
    vchSig.resize(nSigLen);
    return true;
#else
    CECKey key;
    key.SetSecretBytes(vch);
    return key.Sign(hash, vchSig, lowS);
#endif
}

bool CKey::SignCompact(const uint256 &hash, std::vector<unsigned char>& vchSig) const {
    if (!fValid)
        return false;
    vchSig.resize(65);
    int rec = -1;
#ifdef USE_SECP256K1
    CKey nonce;
    do {
        nonce.MakeNewKey(true);
        if (secp256k1_ecdsa_sign_compact((const unsigned char*)&hash, 32, &vchSig[1], begin(), nonce.begin(), &rec))
            break;
    } while(true);
#else
    CECKey key;
    key.SetSecretBytes(vch);
    if (!key.SignCompact(hash, &vchSig[1], rec))
        return false;
#endif
    assert(rec != -1);
    vchSig[0] = 27 + rec + (fCompressed ? 4 : 0);
    return true;
}

bool CKey::Load(CPrivKey &privkey, CPubKey &vchPubKey, bool fSkipCheck=false) {
#ifdef USE_SECP256K1
    if (!secp256k1_ecdsa_privkey_import((unsigned char*)begin(), &privkey[0], privkey.size()))
        return false;
#else
    CECKey key;
    if (!key.SetPrivKey(&privkey[0], privkey.size(), fSkipCheck))
        return false;
    key.GetSecretBytes(vch);
#endif
    fCompressed = vchPubKey.IsCompressed();
    fValid = true;

    if (fSkipCheck)
        return true;

    if (GetPubKey() != vchPubKey)
        return false;

    return true;
}

bool CPubKey::Verify(const uint256 &hash, const std::vector<unsigned char>& vchSig) const {
    if (!IsValid())
        return false;
#ifdef USE_SECP256K1
    if (secp256k1_ecdsa_verify((const unsigned char*)&hash, 32, &vchSig[0], vchSig.size(), begin(), size()) != 1)
        return false;
#else
    CECKey key;
    if (!key.SetPubKey(begin(), size()))
        return false;
    if (!key.Verify(hash, vchSig))
        return false;
#endif
    return true;
}

bool CPubKey::RecoverCompact(const uint256 &hash, const std::vector<unsigned char>& vchSig) {
    if (vchSig.size() != 65)
        return false;
    int recid = (vchSig[0] - 27) & 3;
    bool fComp = ((vchSig[0] - 27) & 4) != 0;
#ifdef USE_SECP256K1
    int pubkeylen = 65;
    if (!secp256k1_ecdsa_recover_compact((const unsigned char*)&hash, 32, &vchSig[1], (unsigned char*)begin(), &pubkeylen, fComp, recid))
        return false;
    assert((int)size() == pubkeylen);
#else
    CECKey key;
    if (!key.Recover(hash, &vchSig[1], recid))
        return false;
    std::vector<unsigned char> pubkey;
    key.GetPubKey(pubkey, fComp);
    Set(pubkey.begin(), pubkey.end());
#endif
    return true;
}

bool CPubKey::IsFullyValid() const {
    if (!IsValid())
        return false;
#ifdef USE_SECP256K1
    if (!secp256k1_ecdsa_pubkey_verify(begin(), size()))
        return false;
#else
    CECKey key;
    if (!key.SetPubKey(begin(), size()))
        return false;
#endif
    return true;
}

bool CPubKey::Decompress() {
    if (!IsValid())
        return false;
#ifdef USE_SECP256K1
    int clen = size();
    int ret = secp256k1_ecdsa_pubkey_decompress((unsigned char*)begin(), &clen);
    assert(ret);
    assert(clen == (int)size());
#else
    CECKey key;
    if (!key.SetPubKey(begin(), size()))
        return false;
    std::vector<unsigned char> pubkey;
    key.GetPubKey(pubkey, false);
    Set(pubkey.begin(), pubkey.end());
#endif
    return true;
}

void static BIP32Hash(const unsigned char chainCode[32], unsigned int nChild, unsigned char header, const unsigned char data[32], unsigned char output[64]) {
    unsigned char num[4];
    num[0] = (nChild >> 24) & 0xFF;
    num[1] = (nChild >> 16) & 0xFF;
    num[2] = (nChild >>  8) & 0xFF;
    num[3] = (nChild >>  0) & 0xFF;
    CHMAC_SHA512(chainCode, 32).Write(&header, 1)
                               .Write(data, 32)
                               .Write(num, 4)
                               .Finalize(output);
}

bool CKey::Derive(CKey& keyChild, unsigned char ccChild[32], unsigned int nChild, const unsigned char cc[32]) const {
    assert(IsValid());
    assert(IsCompressed());
    unsigned char out[64];
    LockObject(out);
    if ((nChild >> 31) == 0) {
        CPubKey pubkey = GetPubKey();
        assert(pubkey.begin() + 33 == pubkey.end());
        BIP32Hash(cc, nChild, *pubkey.begin(), pubkey.begin()+1, out);
    } else {
        assert(begin() + 32 == end());
        BIP32Hash(cc, nChild, 0, begin(), out);
    }
    memcpy(ccChild, out+32, 32);
#ifdef USE_SECP256K1
    memcpy((unsigned char*)keyChild.begin(), begin(), 32);
    bool ret = secp256k1_ecdsa_privkey_tweak_add((unsigned char*)keyChild.begin(), out);
#else
    bool ret = CECKey::TweakSecret((unsigned char*)keyChild.begin(), begin(), out);
#endif
    UnlockObject(out);
    keyChild.fCompressed = true;
    keyChild.fValid = ret;
    return ret;
}

bool CPubKey::Derive(CPubKey& pubkeyChild, unsigned char ccChild[32], unsigned int nChild, const unsigned char cc[32]) const {
    assert(IsValid());
    assert((nChild >> 31) == 0);
    assert(begin() + 33 == end());
    unsigned char out[64];
    BIP32Hash(cc, nChild, *begin(), begin()+1, out);
    memcpy(ccChild, out+32, 32);
#ifdef USE_SECP256K1
    pubkeyChild = *this;
    bool ret = secp256k1_ecdsa_pubkey_tweak_add((unsigned char*)pubkeyChild.begin(), pubkeyChild.size(), out);
#else
    CECKey key;
    bool ret = key.SetPubKey(begin(), size());
    ret &= key.TweakPublic(out);
    std::vector<unsigned char> pubkey;
    key.GetPubKey(pubkey, true);
    pubkeyChild.Set(pubkey.begin(), pubkey.end());
#endif
    return ret;
}

bool CExtKey::Derive(CExtKey &out, unsigned int nChild) const {
    out.nDepth = nDepth + 1;
    CKeyID id = key.GetPubKey().GetID();
    memcpy(&out.vchFingerprint[0], &id, 4);
    out.nChild = nChild;
    return key.Derive(out.key, out.vchChainCode, nChild, vchChainCode);
}

void CExtKey::SetMaster(const unsigned char *seed, unsigned int nSeedLen) {
    static const unsigned char hashkey[] = {'B','i','t','c','o','i','n',' ','s','e','e','d'};
    unsigned char out[64];
    LockObject(out);
    CHMAC_SHA512(hashkey, sizeof(hashkey)).Write(seed, nSeedLen).Finalize(out);
    key.Set(&out[0], &out[32], true);
    memcpy(vchChainCode, &out[32], 32);
    UnlockObject(out);
    nDepth = 0;
    nChild = 0;
    memset(vchFingerprint, 0, sizeof(vchFingerprint));
}

CExtPubKey CExtKey::Neuter() const {
    CExtPubKey ret;
    ret.nDepth = nDepth;
    memcpy(&ret.vchFingerprint[0], &vchFingerprint[0], 4);
    ret.nChild = nChild;
    ret.pubkey = key.GetPubKey();
    memcpy(&ret.vchChainCode[0], &vchChainCode[0], 32);
    return ret;
}

void CExtKey::Encode(unsigned char code[74]) const {
    code[0] = nDepth;
    memcpy(code+1, vchFingerprint, 4);
    code[5] = (nChild >> 24) & 0xFF; code[6] = (nChild >> 16) & 0xFF;
    code[7] = (nChild >>  8) & 0xFF; code[8] = (nChild >>  0) & 0xFF;
    memcpy(code+9, vchChainCode, 32);
    code[41] = 0;
    assert(key.size() == 32);
    memcpy(code+42, key.begin(), 32);
}

void CExtKey::Decode(const unsigned char code[74]) {
    nDepth = code[0];
    memcpy(vchFingerprint, code+1, 4);
    nChild = (code[5] << 24) | (code[6] << 16) | (code[7] << 8) | code[8];
    memcpy(vchChainCode, code+9, 32);
    key.Set(code+42, code+74, true);
}

void CExtPubKey::Encode(unsigned char code[74]) const {
    code[0] = nDepth;
    memcpy(code+1, vchFingerprint, 4);
    code[5] = (nChild >> 24) & 0xFF; code[6] = (nChild >> 16) & 0xFF;
    code[7] = (nChild >>  8) & 0xFF; code[8] = (nChild >>  0) & 0xFF;
    memcpy(code+9, vchChainCode, 32);
    assert(pubkey.size() == 33);
    memcpy(code+41, pubkey.begin(), 33);
}

void CExtPubKey::Decode(const unsigned char code[74]) {
    nDepth = code[0];
    memcpy(vchFingerprint, code+1, 4);
    nChild = (code[5] << 24) | (code[6] << 16) | (code[7] << 8) | code[8];
    memcpy(vchChainCode, code+9, 32);
    pubkey.Set(code+41, code+74);
}

bool CExtPubKey::Derive(CExtPubKey &out, unsigned int nChild) const {
    out.nDepth = nDepth + 1;
    CKeyID id = pubkey.GetID();
    memcpy(&out.vchFingerprint[0], &id, 4);
    out.nChild = nChild;
    return pubkey.Derive(out.pubkey, out.vchChainCode, nChild, vchChainCode);
}

bool ECC_InitSanityCheck() {
#ifdef USE_SECP256K1
    return true;
#else
    return CECKey::SanityCheck();
#endif
}
