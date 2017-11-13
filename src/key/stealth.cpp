// Copyright (c) 2014 The ShadowCoin developers
// Copyright (c) 2017 The Particl developers
// Distributed under the MIT/X11 software license, see the accompanying
// file license.txt or http://www.opensource.org/licenses/mit-license.php.

#include "key/stealth.h"
#include "base58.h"
#include "crypto/sha256.h"
#include "key/keyutil.h"
#include "key.h"
#include "pubkey.h"
#include "random.h"

#include "support/allocators/secure.h"

#include <cmath>
#include <secp256k1.h>

secp256k1_context *secp256k1_ctx_stealth = nullptr;

bool CStealthAddress::SetEncoded(const std::string &encodedAddress)
{
    std::vector<uint8_t> raw;

    if (!DecodeBase58(encodedAddress, raw))
    {
        LogPrint(BCLog::HDWALLET, "%s: DecodeBase58 falied.\n", __func__);
        return false;
    };

    if (!VerifyChecksum(raw))
    {
        LogPrint(BCLog::HDWALLET, "%s: verify_checksum falied.\n", __func__);
        return false;
    };

    if (raw.size() < MIN_STEALTH_RAW_SIZE + 5)
    {
        LogPrint(BCLog::HDWALLET, "%s: too few bytes provided.\n", __func__);
        return false;
    };

    uint8_t *p = &raw[0];
    uint8_t version = *p++;

    if (version != Params().Base58Prefix(CChainParams::STEALTH_ADDRESS)[0])
    {
        LogPrintf("%s: version mismatch 0x%x != 0x%x.\n", __func__, version, Params().Base58Prefix(CChainParams::STEALTH_ADDRESS)[0]);
        return false;
    };

    return 0 == FromRaw(p, raw.size()-1);
};

int CStealthAddress::FromRaw(const uint8_t *p, size_t nSize)
{
    if (nSize < MIN_STEALTH_RAW_SIZE)
        return 1;
    options = *p++;

    scan_pubkey.resize(EC_COMPRESSED_SIZE);
    memcpy(&scan_pubkey[0], p, EC_COMPRESSED_SIZE);
    p += EC_COMPRESSED_SIZE;
    uint8_t spend_pubkeys = *p++;

    if (nSize < MIN_STEALTH_RAW_SIZE + EC_COMPRESSED_SIZE * (spend_pubkeys-1))
        return 1;

    spend_pubkey.resize(EC_COMPRESSED_SIZE * spend_pubkeys);
    memcpy(&spend_pubkey[0], p, EC_COMPRESSED_SIZE * spend_pubkeys);
    p += EC_COMPRESSED_SIZE * spend_pubkeys;
    number_signatures = *p++;
    prefix.number_bits = *p++;
    prefix.bitfield = 0;
    size_t nPrefixBytes = std::ceil((float)prefix.number_bits / 8.0);

    if (nSize < MIN_STEALTH_RAW_SIZE + EC_COMPRESSED_SIZE * (spend_pubkeys-1) + nPrefixBytes)
        return 1;

    if (nPrefixBytes)
        memcpy(&prefix.bitfield, p, nPrefixBytes);

    return 0;
}

int CStealthAddress::ToRaw(std::vector<uint8_t> &raw) const
{
    // https://wiki.unsystem.net/index.php/DarkWallet/Stealth#Address_format
    // [version] [options] [scan_key] [N] ... [Nsigs] [prefix_length] ...

    size_t nPrefixBytes = std::ceil((float)prefix.number_bits / 8.0);
    size_t nPkSpend = spend_pubkey.size() / EC_COMPRESSED_SIZE;
    if (scan_pubkey.size() != EC_COMPRESSED_SIZE
        || spend_pubkey.size() < EC_COMPRESSED_SIZE
        || spend_pubkey.size() % EC_COMPRESSED_SIZE != 0
        || nPkSpend > 255
        || nPrefixBytes > 4)
    {
        LogPrintf("%s: sanity checks failed.\n", __func__);
        return 1;
    };

    raw.resize(MIN_STEALTH_RAW_SIZE + EC_COMPRESSED_SIZE * (nPkSpend-1) + nPrefixBytes);

    int o = 0;
    raw[o] = options; o++;
    memcpy(&raw[o], &scan_pubkey[0], EC_COMPRESSED_SIZE); o += EC_COMPRESSED_SIZE;
    raw[o] = nPkSpend; o++;
    memcpy(&raw[o], &spend_pubkey[0], EC_COMPRESSED_SIZE * nPkSpend); o += EC_COMPRESSED_SIZE * nPkSpend;
    raw[o] = number_signatures; o++;
    raw[o] = prefix.number_bits; o++;
    if (nPrefixBytes)
    {
        memcpy(&raw[o], &prefix.bitfield, nPrefixBytes); o += nPrefixBytes;
    };

    return 0;
};

std::string CStealthAddress::Encoded(bool fBech32) const
{
    return CBitcoinAddress(*this, fBech32).ToString();

};

int CStealthAddress::SetScanPubKey(CPubKey pk)
{
    scan_pubkey.resize(pk.size());
    memcpy(&scan_pubkey[0], pk.begin(), pk.size());
    return 0;
};

CKeyID CStealthAddress::GetSpendKeyID() const
{
    return CKeyID(Hash160(spend_pubkey.begin(), spend_pubkey.end()));
};

int SecretToPublicKey(const CKey &secret, ec_point &out)
{
    // Public key = private * G

    CPubKey pkTemp = secret.GetPubKey();
    out.resize(EC_COMPRESSED_SIZE);
    memcpy(&out[0], pkTemp.begin(), EC_COMPRESSED_SIZE);

    /*
    // Need a secp256k1_ctx that satisfies: secp256k1_ecmult_gen_context_is_built(&ctx->ecmult_gen_ctx)
    secp256k1_pubkey P;

    if (!secp256k1_ec_pubkey_create(secp256k1_ctx_stealth, &P, secret.begin()))
        return errorN(1, "%s: secp256k1_ec_pubkey_create failed.", __func__); // Start again with a new ephemeral key

    try {
        out.resize(EC_COMPRESSED_SIZE);
    } catch (std::exception &e) {
        return errorN(8, "%s: out.resize %u threw: %s.", __func__, EC_COMPRESSED_SIZE);
    };

    if (!secp256k1_ec_pubkey_parse(secp256k1_ctx_stealth, &P, &out[0], EC_COMPRESSED_SIZE))
        return errorN(1, "%s: secp256k1_ec_pubkey_parse R failed.", __func__);
    */
    return 0;
};

int StealthShared(const CKey &secret, const ec_point &pubkey, CKey &sharedSOut)
{
    if (pubkey.size() != EC_COMPRESSED_SIZE)
        return errorN(1, "%s: sanity checks failed.", __func__);

    secp256k1_pubkey Q;
    if (!secp256k1_ec_pubkey_parse(secp256k1_ctx_stealth, &Q, &pubkey[0], EC_COMPRESSED_SIZE))
        return errorN(1, "%s: secp256k1_ec_pubkey_parse Q failed.", __func__);
    if (!secp256k1_ec_pubkey_tweak_mul(secp256k1_ctx_stealth, &Q, secret.begin())) // eQ
        return errorN(1, "%s: secp256k1_ec_pubkey_tweak_mul failed.", __func__);

    size_t len = 33;
    uint8_t tmp33[33];
    secp256k1_ec_pubkey_serialize(secp256k1_ctx_stealth, tmp33, &len, &Q, SECP256K1_EC_COMPRESSED); // Returns: 1 always.

    CSHA256().Write(tmp33, 33).Finalize(sharedSOut.begin_nc());
    return 0;
};

int StealthSecret(const CKey &secret, const ec_point &pubkey, const ec_point &pkSpend, CKey &sharedSOut, ec_point &pkOut)
{
    /*
    send:
        secret = ephem_secret, pubkey = scan_pubkey

    receive:
        secret = scan_secret, pubkey = ephem_pubkey
        c = H(dP)

    Q = public scan key (EC point, 33 bytes)
    d = private scan key (integer, 32 bytes)
    R = public spend key
    f = private spend key

    Q = dG
    R = fG

    Sender (has Q and R, not d or f):

    P = eG

    c = H(eQ) = H(dP)
    R' = R + cG


    Recipient gets R' and P

    test 0 and infinity?
    */

    if (pubkey.size() != EC_COMPRESSED_SIZE
        || pkSpend.size() != EC_COMPRESSED_SIZE)
        return errorN(1, "%s: sanity checks failed.", __func__);

    secp256k1_pubkey Q, R;
    if (!secp256k1_ec_pubkey_parse(secp256k1_ctx_stealth, &Q, &pubkey[0], EC_COMPRESSED_SIZE))
        return errorN(1, "%s: secp256k1_ec_pubkey_parse Q failed.", __func__);

    if (!secp256k1_ec_pubkey_parse(secp256k1_ctx_stealth, &R, &pkSpend[0], EC_COMPRESSED_SIZE))
        return errorN(1, "%s: secp256k1_ec_pubkey_parse R failed.", __func__);

    // eQ
    if (!secp256k1_ec_pubkey_tweak_mul(secp256k1_ctx_stealth, &Q, secret.begin()))
        return errorN(1, "%s: secp256k1_ec_pubkey_tweak_mul failed.", __func__);

    size_t len = 33;
    uint8_t tmp33[33];
    secp256k1_ec_pubkey_serialize(secp256k1_ctx_stealth, tmp33, &len, &Q, SECP256K1_EC_COMPRESSED); // Returns: 1 always.

    CSHA256().Write(tmp33, 33).Finalize(sharedSOut.begin_nc());

    //if (!secp256k1_ec_seckey_verify(secp256k1_ctx_stealth, sharedSOut.begin()))
    //    return errorN(1, "%s: secp256k1_ec_seckey_verify failed.", __func__); // Start again with a new ephemeral key

    // secp256k1_ec_pubkey_tweak_add verifies secret is in correct range

    // C = sharedSOut * G
    // R' = R + C
    if (!secp256k1_ec_pubkey_tweak_add(secp256k1_ctx_stealth, &R, sharedSOut.begin()))
        return errorN(1, "%s: secp256k1_ec_pubkey_tweak_add failed.", __func__); // Start again with a new ephemeral key

    try {
        pkOut.resize(EC_COMPRESSED_SIZE);
    } catch (std::exception &e) {
        return errorN(8, "%s: pkOut.resize %u threw: %s.", __func__, EC_COMPRESSED_SIZE);
    };

    len = 33;
    secp256k1_ec_pubkey_serialize(secp256k1_ctx_stealth, &pkOut[0], &len, &R, SECP256K1_EC_COMPRESSED); // Returns: 1 always.

    return 0;
};


int StealthSecretSpend(const CKey &scanSecret, const ec_point &ephemPubkey, const CKey &spendSecret, CKey &secretOut)
{
    /*

    c  = H(dP)
    R' = R + cG     [without decrypting wallet]
       = (f + c)G   [after decryption of wallet]
         Remember: mod curve.order, pad with 0x00s where necessary?
    */

    if (ephemPubkey.size() != EC_COMPRESSED_SIZE)
        return errorN(1, "%s: sanity checks failed.", __func__);

    secp256k1_pubkey P;

    if (!secp256k1_ec_pubkey_parse(secp256k1_ctx_stealth, &P, &ephemPubkey[0], EC_COMPRESSED_SIZE))
        return errorN(1, "%s: secp256k1_ec_pubkey_parse P failed.", __func__);

    // dP
    if (!secp256k1_ec_pubkey_tweak_mul(secp256k1_ctx_stealth, &P, scanSecret.begin()))
        return errorN(1, "%s: secp256k1_ec_pubkey_tweak_mul failed.", __func__);

    size_t len = 33;
    uint8_t tmp33[33];
    uint8_t tmp32[32];
    secp256k1_ec_pubkey_serialize(secp256k1_ctx_stealth, tmp33, &len, &P, SECP256K1_EC_COMPRESSED); // Returns: 1 always.

    CSHA256().Write(tmp33, 33).Finalize(tmp32);

    if (!secp256k1_ec_seckey_verify(secp256k1_ctx_stealth, tmp32))
        return errorN(1, "%s: secp256k1_ec_seckey_verify failed.", __func__);

    secretOut = spendSecret;
    if (!secp256k1_ec_privkey_tweak_add(secp256k1_ctx_stealth, secretOut.begin_nc(), tmp32))
        return errorN(1, "%s: secp256k1_ec_privkey_tweak_add failed.", __func__);

    return 0;
};


int StealthSharedToSecretSpend(const CKey &sharedS, const CKey &spendSecret, CKey &secretOut)
{

    secretOut = spendSecret;
    if (!secp256k1_ec_privkey_tweak_add(secp256k1_ctx_stealth, secretOut.begin_nc(), sharedS.begin()))
        return errorN(1, "%s: secp256k1_ec_privkey_tweak_add failed.", __func__);

    if (!secp256k1_ec_seckey_verify(secp256k1_ctx_stealth, secretOut.begin())) // necessary?
        return errorN(1, "%s: secp256k1_ec_seckey_verify failed.", __func__);

    return 0;
};

int StealthSharedToPublicKey(const ec_point &pkSpend, const CKey &sharedS, ec_point &pkOut)
{
    if (pkSpend.size() != EC_COMPRESSED_SIZE)
        return errorN(1, "%s: sanity checks failed.", __func__);

    secp256k1_pubkey R;

    if (!secp256k1_ec_pubkey_parse(secp256k1_ctx_stealth, &R, &pkSpend[0], EC_COMPRESSED_SIZE))
        return errorN(1, "%s: secp256k1_ec_pubkey_parse R failed.", __func__);

    if (!secp256k1_ec_pubkey_tweak_add(secp256k1_ctx_stealth, &R, sharedS.begin()))
        return errorN(1, "%s: secp256k1_ec_pubkey_tweak_add failed.", __func__);

    try {
        pkOut.resize(EC_COMPRESSED_SIZE);
    } catch (std::exception &e) {
        return errorN(8, "%s: pkOut.resize %u threw: %s.", __func__, EC_COMPRESSED_SIZE);
    };

    size_t len = 33;
    secp256k1_ec_pubkey_serialize(secp256k1_ctx_stealth, &pkOut[0], &len, &R, SECP256K1_EC_COMPRESSED); // Returns: 1 always.

    return 0;
};

bool IsStealthAddress(const std::string &encodedAddress)
{
    std::vector<uint8_t> raw;

    if (!DecodeBase58(encodedAddress, raw))
    {
        //LogPrintf("IsStealthAddress DecodeBase58 falied.\n");
        return false;
    };

    if (!VerifyChecksum(raw))
    {
        //LogPrintf("IsStealthAddress verify_checksum falied.\n");
        return false;
    };

    if (raw.size() < MIN_STEALTH_RAW_SIZE + 5)
    {
        //LogPrintf("IsStealthAddress too few bytes provided.\n");
        return false;
    };

    uint8_t* p = &raw[0];
    uint8_t version = *p++;

    if (version != Params().Base58Prefix(CChainParams::STEALTH_ADDRESS)[0])
    {
        //LogPrintf("IsStealthAddress version mismatch 0x%x != 0x%x.\n", version, stealth_version_byte);
        return false;
    };

    return true;
};

uint32_t FillStealthPrefix(uint8_t nBits, uint32_t nBitfield)
{
    uint32_t prefix, mask = SetStealthMask(nBits);
    GetStrongRandBytes((uint8_t*) &prefix, 4);

    prefix &= (~mask);
    prefix |= nBitfield & mask;
    return prefix;
};

bool ExtractStealthPrefix(const char *pPrefix, uint32_t &nPrefix)
{
    int base = 10;
    size_t len = strlen(pPrefix);
    if (len > 2
        && pPrefix[0] == '0')
    {
        if (pPrefix[1] == 'b')
        {
            pPrefix += 2;
            base = 2;
        } else
        if (pPrefix[1] == 'x' || pPrefix[1] == 'X')
        {
            pPrefix += 2;
            base = 16;
        };
    };

    char *pend;
    errno = 0;
    nPrefix = strtol(pPrefix, &pend, base);

    if (errno != 0 || !pend || *pend != '\0')
        return error("%s strtol failed.", __func__);
    return true;
};

void ECC_Start_Stealth()
{
    assert(secp256k1_ctx_stealth == nullptr);

    secp256k1_context *ctx = secp256k1_context_create(SECP256K1_CONTEXT_VERIFY);
    assert(ctx != nullptr);

    secp256k1_ctx_stealth = ctx;
};

void ECC_Stop_Stealth()
{
    secp256k1_context *ctx = secp256k1_ctx_stealth;
    secp256k1_ctx_stealth = nullptr;

    if (ctx)
    {
        secp256k1_context_destroy(ctx);
    };
};

