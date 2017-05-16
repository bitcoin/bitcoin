// Copyright (c) 2009-2016 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "key.h"

#include "uint256.h"
#include "crypto/common.h"
#include "pubkey.h"
#include "random.h"

#include <secp256k1.h>
#include <secp256k1_recovery.h>

static secp256k1_context* secp256k1_context_sign = NULL;

/** These functions are taken from the libsecp256k1 distribution and are very ugly. */
static int ec_privkey_import_der(const secp256k1_context* ctx, unsigned char *out32, const unsigned char *privkey, size_t privkeylen) {
    const unsigned char *end = privkey + privkeylen;
    int lenb = 0;
    int len = 0;
    memset(out32, 0, 32);
    /* sequence header */
    if (end < privkey+1 || *privkey != 0x30) {
        return 0;
    }
    privkey++;
    /* sequence length constructor */
    if (end < privkey+1 || !(*privkey & 0x80)) {
        return 0;
    }
    lenb = *privkey & ~0x80; privkey++;
    if (lenb < 1 || lenb > 2) {
        return 0;
    }
    if (end < privkey+lenb) {
        return 0;
    }
    /* sequence length */
    len = privkey[lenb-1] | (lenb > 1 ? privkey[lenb-2] << 8 : 0);
    privkey += lenb;
    if (end < privkey+len) {
        return 0;
    }
    /* sequence element 0: version number (=1) */
    if (end < privkey+3 || privkey[0] != 0x02 || privkey[1] != 0x01 || privkey[2] != 0x01) {
        return 0;
    }
    privkey += 3;
    /* sequence element 1: octet string, up to 32 bytes */
    if (end < privkey+2 || privkey[0] != 0x04 || privkey[1] > 0x20 || end < privkey+2+privkey[1]) {
        return 0;
    }
    memcpy(out32 + 32 - privkey[1], privkey + 2, privkey[1]);
    if (!secp256k1_ec_seckey_verify(ctx, out32)) {
        memset(out32, 0, 32);
        return 0;
    }
    return 1;
}

static int ec_privkey_export_der(
    const secp256k1_context*    ctx,
    unsigned char*              privkey,
    size_t*                     privkeylen,
    const unsigned char*        key32,
    int                         compressed // actually always true.
)
{
    secp256k1_pubkey pubkey;
    size_t pubkeylen = 0;
    if (!secp256k1_ec_pubkey_create(ctx, &pubkey, key32)) {
        *privkeylen = 0;
        return 0;
    }

    // ここは常に true では…？ //
    // SECP256K1_EC_COMPRESSED
    // SECP256K1_EC_UNCOMPRESSED

    // #define SECP256K1_EC_COMPRESSED   (SECP256K1_FLAGS_TYPE_COMPRESSION | SECP256K1_FLAGS_BIT_COMPRESSION)
    // #define SECP256K1_FLAGS_BIT_COMPRESSION (1 << 8)

    // #define SECP256K1_EC_UNCOMPRESSED (SECP256K1_FLAGS_TYPE_COMPRESSION)
    // #define SECP256K1_FLAGS_TYPE_COMPRESSION (1 << 1)

    if (compressed) {
        static const unsigned char begin[] = {
            0x30,0x81,0xD3,0x02,0x01,0x01,0x04,0x20
        };
        static const unsigned char middle[] = {
            0xA0,0x81,0x85,0x30,0x81,0x82,0x02,0x01,0x01,0x30,0x2C,0x06,0x07,0x2A,0x86,0x48,
            0xCE,0x3D,0x01,0x01,0x02,0x21,0x00,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,
            0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,
            0xFF,0xFF,0xFE,0xFF,0xFF,0xFC,0x2F,0x30,0x06,0x04,0x01,0x00,0x04,0x01,0x07,0x04,
            0x21,0x02,0x79,0xBE,0x66,0x7E,0xF9,0xDC,0xBB,0xAC,0x55,0xA0,0x62,0x95,0xCE,0x87,
            0x0B,0x07,0x02,0x9B,0xFC,0xDB,0x2D,0xCE,0x28,0xD9,0x59,0xF2,0x81,0x5B,0x16,0xF8,
            0x17,0x98,0x02,0x21,0x00,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,
            0xFF,0xFF,0xFF,0xFF,0xFE,0xBA,0xAE,0xDC,0xE6,0xAF,0x48,0xA0,0x3B,0xBF,0xD2,0x5E,
            0x8C,0xD0,0x36,0x41,0x41,0x02,0x01,0x01,0xA1,0x24,0x03,0x22,0x00
        };
        unsigned char *ptr = privkey;
        memcpy(ptr, begin, sizeof(begin)); ptr += sizeof(begin);
        memcpy(ptr, key32, 32); ptr += 32;
        memcpy(ptr, middle, sizeof(middle)); ptr += sizeof(middle);
        pubkeylen = 33;
        secp256k1_ec_pubkey_serialize(ctx, ptr, &pubkeylen, &pubkey, SECP256K1_EC_COMPRESSED);
        ptr += pubkeylen;
        *privkeylen = ptr - privkey;
    } else {
        // NOTE: Actually NOT USED.
        assert(0);
    }
    return 1;
}

bool CKeyUtil::Check(const unsigned char *vch) {
    return secp256k1_ec_seckey_verify(secp256k1_context_sign, vch);
}

void CKey::MakeNewKey(bool fCompressedIn) {
    do {
        GetStrongRandBytes(m_keydata.data(), m_keydata.size());
    } while (!CKeyUtil::Check(m_keydata.data()));
    m_fValid = true;
    m_fCompressed = fCompressedIn;
}

bool CKey::SetPrivKey(const CPrivKey &privkey, bool fCompressedIn) {
    if (!ec_privkey_import_der(secp256k1_context_sign, (unsigned char*)begin(), &privkey[0], privkey.size()))
        return false;
    m_fCompressed = fCompressedIn;
    m_fValid = true;
    return true;
}

CPrivKey CKey::GetPrivKey() const {
    assert(m_fValid);
    CPrivKey privkey;
    int ret;
    size_t privkeylen;
    privkey.resize(279);
    privkeylen = 279;

    // ※常に圧縮モード.
    // Ref: https://github.com/bitcoin/bitcoin/issues/10041
    ret = ec_privkey_export_der(
        secp256k1_context_sign,
        (unsigned char*)&privkey[0],
        &privkeylen,
        begin(),
        SECP256K1_EC_COMPRESSED // Compressed mode = TRUE (Actually bool-typed value)
    );

    assert(ret);
    privkey.resize(privkeylen);
    return privkey;
}

base58string CKey::GetBase58stringWithNetworkSecretKeyPrefix() const
{
    CBase58Data data;

    assert(this->IsValid());
    data.SetData(
        Params().Base58Prefix(CChainParams::SECRET_KEY),
        this->begin(),
        this->size() // ここは常に32??? まだよくわからん。m_keydata.size() が来るっぽいが. (※invalidの場合は0)
    );
    if (this->IsCompressed()) // ★.
        data.m_vchData.push_back(1);

    return base58string(data._ToString());
}

// static
CKey CKey::FromBase58string(const base58string& strPrivkey)
{
    // -- -- デコード -- -- //
    CBase58Data data;
    bool fGood = data.SetBase58string(strPrivkey);
    if (!fGood)return CKey(); // invalid key

    // -- -- 有効性チェック (旧 CBitcoinSecret::IsValid) -- -- //
    // データ内容チェック (長さおよび圧縮フラグ).
    bool fExpectedFormat =
        data.m_vchData.size() == 32 // 非圧縮版.
        || (data.m_vchData.size() == 33 && data.m_vchData[32] == 1); // 圧縮版.
    if (!fExpectedFormat)return CKey(); // invalid key

    // prefix のチェック.
    bool fCorrectVersion =
        data.m_vchVersion == Params().Base58Prefix(CChainParams::SECRET_KEY);
    if (!fCorrectVersion)return CKey(); // invalid key;

    // -- -- CKey への変換 -- -- //
    CKey ret;
    assert(data.m_vchData.size() >= 32);

    // 先頭32バイトを抽出してセット.
    // ※圧縮されている場合は [32] が 1 になっているので、それを bool 値として compressed に渡す.
    ret.SetBinary(
        data.m_vchData.data(),      // pbegin
        data.m_vchData.data() + 32, // pend
        data.m_vchData.size() > 32 && data.m_vchData[32] == 1 // compressed
    );
    return ret;
}

CPubKey CKey::GetPubKey() const {
    assert(m_fValid);
    secp256k1_pubkey pubkey;
    size_t clen = 65;
    CPubKey result;
    int ret = secp256k1_ec_pubkey_create(secp256k1_context_sign, &pubkey, begin());
    assert(ret);
    secp256k1_ec_pubkey_serialize(
        secp256k1_context_sign,
        (unsigned char*)result.begin(),
        &clen,
        &pubkey,
        m_fCompressed ? SECP256K1_EC_COMPRESSED : SECP256K1_EC_UNCOMPRESSED
    );
    assert(result.size() == clen);
    assert(result.IsValid());
    return result;
}

bool CKey::Sign(const uint256 &hash, std::vector<unsigned char>& vchSig, uint32_t test_case) const {
    if (!m_fValid)
        return false;
    vchSig.resize(72);
    size_t nSigLen = 72;
    unsigned char extra_entropy[32] = {0};
    WriteLE32(extra_entropy, test_case);
    secp256k1_ecdsa_signature sig;
    int ret = secp256k1_ecdsa_sign(secp256k1_context_sign, &sig, hash.begin(), begin(), secp256k1_nonce_function_rfc6979, test_case ? extra_entropy : NULL);
    assert(ret);
    secp256k1_ecdsa_signature_serialize_der(secp256k1_context_sign, (unsigned char*)&vchSig[0], &nSigLen, &sig);
    vchSig.resize(nSigLen);
    return true;
}

bool CKey::VerifyPubKey(const CPubKey& pubkey) const {
    if (pubkey.IsCompressed() != m_fCompressed) {
        return false;
    }
    unsigned char rnd[8];
    std::string str = "Bitcoin key verification\n";
    GetRandBytes(rnd, sizeof(rnd));
    uint256 hash;
    CHash256().Write((unsigned char*)str.data(), str.size()).Write(rnd, sizeof(rnd)).Finalize(hash.begin());
    std::vector<unsigned char> vchSig;
    Sign(hash, vchSig);
    return pubkey.Verify(hash, vchSig);
}

bool CKey::SignCompact(const uint256 &hash, std::vector<unsigned char>& vchSig) const {
    if (!m_fValid)
        return false;
    vchSig.resize(65);
    int rec = -1;
    secp256k1_ecdsa_recoverable_signature sig;
    int ret = secp256k1_ecdsa_sign_recoverable(secp256k1_context_sign, &sig, hash.begin(), begin(), secp256k1_nonce_function_rfc6979, NULL);
    assert(ret);
    secp256k1_ecdsa_recoverable_signature_serialize_compact(secp256k1_context_sign, (unsigned char*)&vchSig[1], &rec, &sig);
    assert(ret);
    assert(rec != -1);
    vchSig[0] = 27 + rec + (m_fCompressed ? 4 : 0);
    return true;
}

bool CKey::Load(CPrivKey &privkey, CPubKey &vchPubKey, bool fSkipCheck=false) {
    if (!ec_privkey_import_der(secp256k1_context_sign, (unsigned char*)begin(), &privkey[0], privkey.size()))
        return false;
    m_fCompressed = vchPubKey.IsCompressed();
    m_fValid = true;

    if (fSkipCheck)
        return true;

    return VerifyPubKey(vchPubKey);
}

bool CKey::Derive(CKey& keyChild, ChainCode &ccChild, unsigned int nChild, const ChainCode& cc) const {
    assert(IsValid());
    assert(IsCompressed());
    std::vector<unsigned char, secure_allocator<unsigned char>> vout(64);
    if ((nChild >> 31) == 0) {
        CPubKey pubkey = GetPubKey();
        assert(pubkey.begin() + 33 == pubkey.end());
        BIP32Hash(cc, nChild, *pubkey.begin(), pubkey.begin()+1, vout.data());
    } else {
        assert(begin() + 32 == end());
        BIP32Hash(cc, nChild, 0, begin(), vout.data());
    }
    memcpy(ccChild.begin(), vout.data()+32, 32);
    memcpy((unsigned char*)keyChild.begin(), begin(), 32);
    bool ret = secp256k1_ec_privkey_tweak_add(secp256k1_context_sign, (unsigned char*)keyChild.begin(), vout.data());
    keyChild.m_fCompressed = true;
    keyChild.m_fValid = ret;
    return ret;
}

/*
base58string CKey::ToBase58string() const
{
}
*/

bool ECC_InitSanityCheck() {
    CKey key;
    key.MakeNewKey(true);
    CPubKey pubkey = key.GetPubKey();
    return key.VerifyPubKey(pubkey);
}

void ECC_Start() {
    assert(secp256k1_context_sign == NULL);

    secp256k1_context *ctx = secp256k1_context_create(SECP256K1_CONTEXT_SIGN);
    assert(ctx != NULL);

    {
        // Pass in a random blinding seed to the secp256k1 context.
        std::vector<unsigned char, secure_allocator<unsigned char>> vseed(32);
        GetRandBytes(vseed.data(), 32);
        bool ret = secp256k1_context_randomize(ctx, vseed.data());
        assert(ret);
    }

    secp256k1_context_sign = ctx;

    CChainParams::Initialize(); // 仮.
}

void ECC_Stop() {
    secp256k1_context *ctx = secp256k1_context_sign;
    secp256k1_context_sign = NULL;

    if (ctx) {
        secp256k1_context_destroy(ctx);
    }
}
