// Copyright (c) 2009-2012 The Bitcoin developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <map>

#include <openssl/ecdsa.h>
#include <openssl/evp.h>
#include <openssl/obj_mac.h>

#include "key.h"
#include "base58.h"

// Generate a private key from just the secret parameter
int EC_KEY_regenerate_key(EC_KEY *eckey, BIGNUM *priv_key)
{
    int ok = 0;
    BN_CTX *ctx = NULL;
    EC_POINT *pub_key = NULL;

    if (!eckey) return 0;

    const EC_GROUP *group = EC_KEY_get0_group(eckey);

    if ((ctx = BN_CTX_new()) == NULL)
        goto err;

    pub_key = EC_POINT_new(group);

    if (pub_key == NULL)
        goto err;

    if (!EC_POINT_mul(group, pub_key, priv_key, NULL, NULL, ctx))
        goto err;

    EC_KEY_set_private_key(eckey,priv_key);
    EC_KEY_set_public_key(eckey,pub_key);

    ok = 1;

err:

    if (pub_key)
        EC_POINT_free(pub_key);
    if (ctx != NULL)
        BN_CTX_free(ctx);

    return(ok);
}

// Perform ECDSA key recovery (see SEC1 4.1.6) for curves over (mod p)-fields
// recid selects which key is recovered
// if check is non-zero, additional checks are performed
int ECDSA_SIG_recover_key_GFp(EC_KEY *eckey, ECDSA_SIG *ecsig, const unsigned char *msg, int msglen, int recid, int check)
{
    if (!eckey) return 0;

    int ret = 0;
    BN_CTX *ctx = NULL;

    BIGNUM *x = NULL;
    BIGNUM *e = NULL;
    BIGNUM *order = NULL;
    BIGNUM *sor = NULL;
    BIGNUM *eor = NULL;
    BIGNUM *field = NULL;
    EC_POINT *R = NULL;
    EC_POINT *O = NULL;
    EC_POINT *Q = NULL;
    BIGNUM *rr = NULL;
    BIGNUM *zero = NULL;
    int n = 0;
    int i = recid / 2;

    const EC_GROUP *group = EC_KEY_get0_group(eckey);
    if ((ctx = BN_CTX_new()) == NULL) { ret = -1; goto err; }
    BN_CTX_start(ctx);
    order = BN_CTX_get(ctx);
    if (!EC_GROUP_get_order(group, order, ctx)) { ret = -2; goto err; }
    x = BN_CTX_get(ctx);
    if (!BN_copy(x, order)) { ret=-1; goto err; }
    if (!BN_mul_word(x, i)) { ret=-1; goto err; }
    if (!BN_add(x, x, ecsig->r)) { ret=-1; goto err; }
    field = BN_CTX_get(ctx);
    if (!EC_GROUP_get_curve_GFp(group, field, NULL, NULL, ctx)) { ret=-2; goto err; }
    if (BN_cmp(x, field) >= 0) { ret=0; goto err; }
    if ((R = EC_POINT_new(group)) == NULL) { ret = -2; goto err; }
    if (!EC_POINT_set_compressed_coordinates_GFp(group, R, x, recid % 2, ctx)) { ret=0; goto err; }
    if (check)
    {
        if ((O = EC_POINT_new(group)) == NULL) { ret = -2; goto err; }
        if (!EC_POINT_mul(group, O, NULL, R, order, ctx)) { ret=-2; goto err; }
        if (!EC_POINT_is_at_infinity(group, O)) { ret = 0; goto err; }
    }
    if ((Q = EC_POINT_new(group)) == NULL) { ret = -2; goto err; }
    n = EC_GROUP_get_degree(group);
    e = BN_CTX_get(ctx);
    if (!BN_bin2bn(msg, msglen, e)) { ret=-1; goto err; }
    if (8*msglen > n) BN_rshift(e, e, 8-(n & 7));
    zero = BN_CTX_get(ctx);
    if (!BN_zero(zero)) { ret=-1; goto err; }
    if (!BN_mod_sub(e, zero, e, order, ctx)) { ret=-1; goto err; }
    rr = BN_CTX_get(ctx);
    if (!BN_mod_inverse(rr, ecsig->r, order, ctx)) { ret=-1; goto err; }
    sor = BN_CTX_get(ctx);
    if (!BN_mod_mul(sor, ecsig->s, rr, order, ctx)) { ret=-1; goto err; }
    eor = BN_CTX_get(ctx);
    if (!BN_mod_mul(eor, e, rr, order, ctx)) { ret=-1; goto err; }
    if (!EC_POINT_mul(group, Q, eor, R, sor, ctx)) { ret=-2; goto err; }
    if (!EC_KEY_set_public_key(eckey, Q)) { ret=-2; goto err; }

    ret = 1;

err:
    if (ctx) {
        BN_CTX_end(ctx);
        BN_CTX_free(ctx);
    }
    if (R != NULL) EC_POINT_free(R);
    if (O != NULL) EC_POINT_free(O);
    if (Q != NULL) EC_POINT_free(Q);
    return ret;
}

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

// Order of secp256k1's generator minus 1.
const unsigned char vchMaxModOrder[32] = {
    0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,
    0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFE,
    0xBA,0xAE,0xDC,0xE6,0xAF,0x48,0xA0,0x3B,
    0xBF,0xD2,0x5E,0x8C,0xD0,0x36,0x41,0x40
};

// Half of the order of secp256k1's generator minus 1.
const unsigned char vchMaxModHalfOrder[32] = {
    0x7F,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,
    0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,
    0x5D,0x57,0x6E,0x73,0x57,0xA4,0x50,0x1D,
    0xDF,0xE9,0x2F,0x46,0x68,0x1B,0x20,0xA0
};

const unsigned char *vchZero = NULL;

void CKey::SetCompressedPubKey(bool fCompressed)
{
    EC_KEY_set_conv_form(pkey, fCompressed ? POINT_CONVERSION_COMPRESSED : POINT_CONVERSION_UNCOMPRESSED);
}

void CKey::Reset()
{
    fSet = false;
    if (pkey != NULL)
        EC_KEY_free(pkey);
    pkey = EC_KEY_new_by_curve_name(NID_secp256k1);
    if (pkey == NULL)
        throw key_error("CKey::CKey() : EC_KEY_new_by_curve_name failed");
}

CKey::CKey()
{
    pkey = NULL;
    Reset();
}

CKey::CKey(const CKey& b)
{
    pkey = EC_KEY_dup(b.pkey);
    if (pkey == NULL)
        throw key_error("CKey::CKey(const CKey&) : EC_KEY_dup failed");
    fSet = b.fSet;
}

CKey::CKey(const CSecret& b, bool fCompressed)
{
    pkey = EC_KEY_new_by_curve_name(NID_secp256k1);
    if (pkey == NULL)
        throw key_error("CKey::CKey(const CKey&) : EC_KEY_dup failed");
    SetSecret(b, fCompressed);
}

CKey& CKey::operator=(const CKey& b)
{
    if (!EC_KEY_copy(pkey, b.pkey))
        throw key_error("CKey::operator=(const CKey&) : EC_KEY_copy failed");
    fSet = b.fSet;
    return (*this);
}

CKey::~CKey()
{
    if (pkey != NULL)
        EC_KEY_free(pkey);
}

bool CKey::IsNull() const
{
    return !fSet;
}

bool CKey::IsCompressed() const
{
    return (EC_KEY_get_conv_form(pkey) == POINT_CONVERSION_COMPRESSED);
}

bool CKey::CheckSignatureElement(const unsigned char *vch, int len, bool half) {
    return CompareBigEndian(vch, len, vchZero, 0) > 0 &&
        CompareBigEndian(vch, len, half ? vchMaxModHalfOrder : vchMaxModOrder, 32) <= 0;
}

bool CPubKey::ReserealizeSignature(std::vector<unsigned char>& vchSig)
{
    if (vchSig.empty())
        return false;

    unsigned char *pos = &vchSig[0];
    ECDSA_SIG *sig = d2i_ECDSA_SIG(NULL, (const unsigned char **)&pos, vchSig.size());
    if (sig == NULL)
        return false;

    bool ret = false;
    int nSize = i2d_ECDSA_SIG(sig, NULL);
    if (nSize > 0) {
        vchSig.resize(nSize); // grow or shrink as needed

        pos = &vchSig[0];
        i2d_ECDSA_SIG(sig, &pos);

        ret = true;
    }

    ECDSA_SIG_free(sig);

    return ret;
}

void CKey::MakeNewKey(bool fCompressed)
{
    if (!EC_KEY_generate_key(pkey))
        throw key_error("CKey::MakeNewKey() : EC_KEY_generate_key failed");
    SetCompressedPubKey(fCompressed);
    fSet = true;
}

bool CKey::SetPrivKey(const CPrivKey& vchPrivKey)
{
    const unsigned char* pbegin = &vchPrivKey[0];
    if (d2i_ECPrivateKey(&pkey, &pbegin, vchPrivKey.size()))
    {
        // In testing, d2i_ECPrivateKey can return true
        // but fill in pkey with a key that fails
        // EC_KEY_check_key, so:
        if (EC_KEY_check_key(pkey))
        {
            fSet = true;
            return true;
        }
    }
    // If vchPrivKey data is bad d2i_ECPrivateKey() can
    // leave pkey in a state where calling EC_KEY_free()
    // crashes. To avoid that, set pkey to NULL and
    // leak the memory (a leak is better than a crash)
    pkey = NULL;
    Reset();
    return false;
}

bool CKey::SetSecret(const CSecret& vchSecret, bool fCompressed)
{
    EC_KEY_free(pkey);
    pkey = EC_KEY_new_by_curve_name(NID_secp256k1);
    if (pkey == NULL)
        throw key_error("CKey::SetSecret() : EC_KEY_new_by_curve_name failed");

    if (vchSecret.size() != 32)
        throw key_error("CKey::SetSecret() : secret must be 32 bytes");
    BIGNUM *bn = BN_bin2bn(&vchSecret[0],32,BN_new());
    if (bn == NULL)
        throw key_error("CKey::SetSecret() : BN_bin2bn failed");
    if (!EC_KEY_regenerate_key(pkey,bn))
    {
        BN_clear_free(bn);
        throw key_error("CKey::SetSecret() : EC_KEY_regenerate_key failed");
    }
    BN_clear_free(bn);
    fSet = true;
    SetCompressedPubKey(fCompressed);
    return true;
}

CSecret CKey::GetSecret(bool &fCompressed) const
{
    CSecret vchRet;
    vchRet.resize(32);
    const BIGNUM *bn = EC_KEY_get0_private_key(pkey);
    int nBytes = BN_num_bytes(bn);
    if (bn == NULL)
        throw key_error("CKey::GetSecret() : EC_KEY_get0_private_key failed");
    int n=BN_bn2bin(bn,&vchRet[32 - nBytes]);
    if (n != nBytes)
        throw key_error("CKey::GetSecret(): BN_bn2bin failed");
    fCompressed = IsCompressed();
    return vchRet;
}

bool CKey::WritePEM(BIO *streamObj, const SecureString &strPassKey) const // dumppem 4KJLA99FyqMMhjjDe7KnRXK4sjtv9cCtNS /tmp/test.pem 123
{
    EVP_PKEY *evpKey = EVP_PKEY_new();
    if (!EVP_PKEY_assign_EC_KEY(evpKey, pkey))
        return error("CKey::WritePEM() : Error initializing EVP_PKEY instance.");
    if(!PEM_write_bio_PKCS8PrivateKey(streamObj, evpKey, EVP_aes_256_cbc(), (char *)&strPassKey[0], strPassKey.size(), NULL, NULL))
        return error("CKey::WritePEM() : Error writing private key data to stream object");

    return true;
}

CSecret CKey::GetSecret() const
{
    bool fCompressed;
    return GetSecret(fCompressed);
}

CPrivKey CKey::GetPrivKey() const
{
    int nSize = i2d_ECPrivateKey(pkey, NULL);
    if (!nSize)
        throw key_error("CKey::GetPrivKey() : i2d_ECPrivateKey failed");
    CPrivKey vchPrivKey(nSize, 0);
    unsigned char* pbegin = &vchPrivKey[0];
    if (i2d_ECPrivateKey(pkey, &pbegin) != nSize)
        throw key_error("CKey::GetPrivKey() : i2d_ECPrivateKey returned unexpected size");
    return vchPrivKey;
}

CPubKey CKey::GetPubKey() const
{
    int nSize = i2o_ECPublicKey(pkey, NULL);
    if (!nSize)
        throw key_error("CKey::GetPubKey() : i2o_ECPublicKey failed");
    std::vector<unsigned char> vchPubKey(nSize, 0);
    unsigned char* pbegin = &vchPubKey[0];
    if (i2o_ECPublicKey(pkey, &pbegin) != nSize)
        throw key_error("CKey::GetPubKey() : i2o_ECPublicKey returned unexpected size");
    return CPubKey(vchPubKey);
}

bool CKey::Sign(uint256 hash, std::vector<unsigned char>& vchSig)
{
    vchSig.clear();
    ECDSA_SIG *sig = ECDSA_do_sign((unsigned char*)&hash, sizeof(hash), pkey);
    if (sig==NULL)
        return false;
    const EC_GROUP *group = EC_KEY_get0_group(pkey);
    CBigNum order, halforder;
    EC_GROUP_get_order(group, &order, NULL);
    BN_rshift1(&halforder, &order);
    // enforce low S values, by negating the value (modulo the order) if above order/2.
    if (BN_cmp(sig->s, &halforder) > 0) {
        BN_sub(sig->s, &order, sig->s);
    }
    unsigned int nSize = ECDSA_size(pkey);
    vchSig.resize(nSize); // Make sure it is big enough
    unsigned char *pos = &vchSig[0];
    nSize = i2d_ECDSA_SIG(sig, &pos);
    ECDSA_SIG_free(sig);
    vchSig.resize(nSize); // Shrink to fit actual size
    // Testing our new signature
    if (ECDSA_verify(0, (unsigned char*)&hash, sizeof(hash), &vchSig[0], vchSig.size(), pkey) != 1) {
        vchSig.clear();
        return false;
    }
    return true;
}

// create a compact signature (65 bytes), which allows reconstructing the used public key
// The format is one header byte, followed by two times 32 bytes for the serialized r and s values.
// The header byte: 0x1B = first key with even y, 0x1C = first key with odd y,
//                  0x1D = second key with even y, 0x1E = second key with odd y
bool CKey::SignCompact(uint256 hash, std::vector<unsigned char>& vchSig)
{
    bool fOk = false;
    ECDSA_SIG *sig = ECDSA_do_sign((unsigned char*)&hash, sizeof(hash), pkey);
    if (sig==NULL)
        return false;
    const EC_GROUP *group = EC_KEY_get0_group(pkey);
    CBigNum order, halforder;
    EC_GROUP_get_order(group, &order, NULL);
    BN_rshift1(&halforder, &order);
    // enforce low S values, by negating the value (modulo the order) if above order/2.
    if (BN_cmp(sig->s, &halforder) > 0) {
        BN_sub(sig->s, &order, sig->s);
    }
    vchSig.clear();
    vchSig.resize(65,0);
    int nBitsR = BN_num_bits(sig->r);
    int nBitsS = BN_num_bits(sig->s);
    bool fCompressedPubKey = IsCompressed();
    if (nBitsR <= 256 && nBitsS <= 256)
    {
        int8_t nRecId = -1;
        for (int8_t i=0; i<4; i++)
        {
            CKey keyRec;
            keyRec.fSet = true;
            keyRec.SetCompressedPubKey(fCompressedPubKey);
            if (ECDSA_SIG_recover_key_GFp(keyRec.pkey, sig, (unsigned char*)&hash, sizeof(hash), i, 1) == 1)
                if (keyRec.GetPubKey() == this->GetPubKey())
                {
                    nRecId = i;
                    break;
                }
        }

        if (nRecId == -1)
        {
            ECDSA_SIG_free(sig);
            throw key_error("CKey::SignCompact() : unable to construct recoverable key");
        }

        vchSig[0] = nRecId+27+(fCompressedPubKey ? 4 : 0);
        BN_bn2bin(sig->r,&vchSig[33-(nBitsR+7)/8]);
        BN_bn2bin(sig->s,&vchSig[65-(nBitsS+7)/8]);
        fOk = true;
    }
    ECDSA_SIG_free(sig);
    return fOk;
}

// reconstruct public key from a compact signature
// This is only slightly more CPU intensive than just verifying it.
// If this function succeeds, the recovered public key is guaranteed to be valid
// (the signature is a valid signature of the given data for that key)
bool CPubKey::SetCompactSignature(uint256 hash, const std::vector<unsigned char>& vchSig)
{
    if (vchSig.size() != 65)
        return false;
    int nV = vchSig[0];
    if (nV<27 || nV>=35)
        return false;
    ECDSA_SIG *sig = ECDSA_SIG_new();
    BN_bin2bn(&vchSig[1],32,sig->r);
    BN_bin2bn(&vchSig[33],32,sig->s);
    bool fSuccessful = false;
    EC_KEY* pkey = EC_KEY_new_by_curve_name(NID_secp256k1);
    if (nV >= 31)
    {
        nV -= 4;
        EC_KEY_set_conv_form(pkey, POINT_CONVERSION_COMPRESSED);
    }
    do
    {
        if (ECDSA_SIG_recover_key_GFp(pkey, sig, (unsigned char*)&hash, sizeof(hash), nV - 27, 0) != 1)
            break;
        int nSize = i2o_ECPublicKey(pkey, NULL);
        if (!nSize)
            break;
        std::vector<unsigned char> vchPubKey(nSize, 0);
        unsigned char* pbegin = &vchPubKey[0];
        if (i2o_ECPublicKey(pkey, &pbegin) != nSize)
            break;
        Set(vchPubKey.begin(), vchPubKey.end());
        fSuccessful = IsValid();

    } while (false);
    ECDSA_SIG_free(sig);
    EC_KEY_free(pkey);
    if (!fSuccessful)
        Invalidate();
    return fSuccessful;
}

bool CPubKey::Verify(const uint256 &hash, const std::vector<unsigned char>& vchSig) const
{
    if (vchSig.empty() || !IsValid())
        return false;

    EC_KEY *pkey = EC_KEY_new_by_curve_name(NID_secp256k1);
    ECDSA_SIG *norm_sig = ECDSA_SIG_new();

    assert(norm_sig);
    assert(pkey);

    bool ret = false;
    do
    {
        int derlen;
        uint8_t *norm_der = NULL;
        const uint8_t* pbegin = &vbytes[0];
        const uint8_t* sigptr = &vchSig[0];

        // Trying to parse public key
        if (!o2i_ECPublicKey(&pkey, &pbegin, size()))
            break;
        // New versions of OpenSSL are rejecting a non-canonical DER signatures, de/re-serialize first.
        if (d2i_ECDSA_SIG(&norm_sig, &sigptr, vchSig.size()) == NULL)
            break;
        if ((derlen = i2d_ECDSA_SIG(norm_sig, &norm_der)) <= 0)
            break;

        // -1 = error, 0 = bad sig, 1 = good
        ret = ECDSA_verify(0, (const unsigned char*)&hash, sizeof(hash), norm_der, derlen, pkey) == 1;
        OPENSSL_free(norm_der);
    } while(false);

    ECDSA_SIG_free(norm_sig);
    EC_KEY_free(pkey);

    return ret;
}

bool CPubKey::VerifyCompact(uint256 hash, const std::vector<unsigned char>& vchSig)
{
    CPubKey key;
    if (!key.SetCompactSignature(hash, vchSig))
        return false;
    return true;
}

bool CKey::IsValid()
{
    if (!fSet)
        return false;

    if (!EC_KEY_check_key(pkey))
        return false;

    bool fCompr;
    CSecret secret = GetSecret(fCompr);
    CKey key2;
    key2.SetSecret(secret, fCompr);

    return GetPubKey() == key2.GetPubKey();
}

CPoint::CPoint()
{
    std::string err;
    group = NULL;
    point = NULL;
    ctx   = NULL;

    group = EC_GROUP_new_by_curve_name(NID_secp256k1);
    if (!group) {
        err = "EC_KEY_new_by_curve_name failed.";
        goto finish;
    }

    point = EC_POINT_new(group);
    if (!point) {
        err = "EC_POINT_new failed.";
        goto finish;
    }

    ctx = BN_CTX_new();
    if (!ctx) {
        err = "BN_CTX_new failed.";
        goto finish;
    }

    return;

finish:
    if (group) EC_GROUP_free(group);
    if (point) EC_POINT_free(point);
    throw std::runtime_error(std::string("CPoint::CPoint() :  - ") + err);
}

bool CPoint::operator!=(const CPoint &a)
{
    if (EC_POINT_cmp(group, point, a.point, ctx) != 0)
        return true;
    return false;
}
CPoint::~CPoint()
{
    if (point) EC_POINT_free(point);
    if (group) EC_GROUP_free(group);
    if (ctx)   BN_CTX_free(ctx);
}

// Initialize from octets stream
bool CPoint::setBytes(const std::vector<unsigned char> &vchBytes)
{
    if (!EC_POINT_oct2point(group, point, &vchBytes[0], vchBytes.size(), ctx)) {
        return false;
    }
    return true;
}

// Initialize from octets stream
bool CPoint::setPubKey(const CPubKey &key)
{
    std::vector<uint8_t> vchPubKey(key.begin(), key.end());
    return setBytes(vchPubKey);
}

// Serialize to octets stream
bool CPoint::getBytes(std::vector<unsigned char> &vchBytes)
{
    size_t nSize = EC_POINT_point2oct(group, point, POINT_CONVERSION_COMPRESSED, NULL, 0, ctx);
    vchBytes.resize(nSize);
    if (!(nSize == EC_POINT_point2oct(group, point, POINT_CONVERSION_COMPRESSED, &vchBytes[0], nSize, ctx))) {
        return false;
    }
    return true;
}

// ECC multiplication by specified multiplier
bool CPoint::ECMUL(const CBigNum &bnMultiplier)
{
    if (!EC_POINT_mul(group, point, NULL, point, &bnMultiplier, NULL)) {
        printf("CPoint::ECMUL() : EC_POINT_mul failed");
        return false;
    }

    return true;
}

// Calculate G*m + q
bool CPoint::ECMULGEN(const CBigNum &bnMultiplier, const CPoint &qPoint)
{
    if (!EC_POINT_mul(group, point, &bnMultiplier, qPoint.point, BN_value_one(), NULL)) {
        printf("CPoint::ECMULGEN() : EC_POINT_mul failed.");
        return false;
    }

    return true;
}

// CMalleablePubKey

void CMalleablePubKey::GetVariant(CPubKey &R, CPubKey &vchPubKeyVariant)
{
    EC_KEY *eckey = NULL;
    eckey = EC_KEY_new_by_curve_name(NID_secp256k1);
    if (eckey == NULL) {
        throw key_error("CMalleablePubKey::GetVariant() : EC_KEY_new_by_curve_name failed");
    }

    // Use standard key generation function to get r and R values.
    //
    // r will be presented by private key;
    // R is ECDSA public key which calculated as G*r
    if (!EC_KEY_generate_key(eckey)) {
        throw key_error("CMalleablePubKey::GetVariant() : EC_KEY_generate_key failed");
    }

    EC_KEY_set_conv_form(eckey, POINT_CONVERSION_COMPRESSED);

    int nSize = i2o_ECPublicKey(eckey, NULL);
    if (!nSize) {
        throw key_error("CMalleablePubKey::GetVariant() : i2o_ECPublicKey failed");
    }

    std::vector<unsigned char> vchPubKey(nSize, 0);
    unsigned char* pbegin_R = &vchPubKey[0];

    if (i2o_ECPublicKey(eckey, &pbegin_R) != nSize) {
        throw key_error("CMalleablePubKey::GetVariant() : i2o_ECPublicKey returned unexpected size");
    }

    // R = G*r
    R = CPubKey(vchPubKey);

    // OpenSSL BIGNUM representation of r value
    CBigNum bnr;
    bnr = *(CBigNum*) EC_KEY_get0_private_key(eckey);
    EC_KEY_free(eckey);

    CPoint point;
    if (!point.setPubKey(pubKeyL)) {
        throw key_error("CMalleablePubKey::GetVariant() : Unable to decode L value");
    }

    // Calculate L*r
    point.ECMUL(bnr);

    std::vector<unsigned char> vchLr;
    if (!point.getBytes(vchLr)) {
        throw key_error("CMalleablePubKey::GetVariant() : Unable to convert Lr value");
    }

    // Calculate Hash(L*r) and then get a BIGNUM representation of hash value.
    CBigNum bnHash;
    bnHash.setuint160(Hash160(vchLr));

    CPoint pointH;
    pointH.setPubKey(pubKeyH);

    CPoint P;
    // Calculate P = Hash(L*r)*G + H
    P.ECMULGEN(bnHash, pointH);

    if (P.IsInfinity()) {
        throw key_error("CMalleablePubKey::GetVariant() : P is infinity");
    }

    std::vector<unsigned char> vchResult;
    P.getBytes(vchResult);

    vchPubKeyVariant = CPubKey(vchResult);
}

std::string CMalleablePubKey::ToString() const
{
    CDataStream ssKey(SER_NETWORK, PROTOCOL_VERSION);
    ssKey << *this;
    std::vector<unsigned char> vch(ssKey.begin(), ssKey.end());

    return EncodeBase58Check(vch);
}

bool CMalleablePubKey::setvch(const std::vector<unsigned char> &vchPubKeyPair)
{
    CDataStream ssKey(vchPubKeyPair, SER_NETWORK, PROTOCOL_VERSION);
    ssKey >> *this;

    return IsValid();
}

std::vector<unsigned char> CMalleablePubKey::Raw() const
{
    CDataStream ssKey(SER_NETWORK, PROTOCOL_VERSION);
    ssKey << *this;
    std::vector<unsigned char> vch(ssKey.begin(), ssKey.end());

    return vch;
}

bool CMalleablePubKey::SetString(const std::string& strMalleablePubKey)
{
    std::vector<unsigned char> vchTemp;
    if (!DecodeBase58Check(strMalleablePubKey, vchTemp)) {
        throw key_error("CMalleablePubKey::SetString() : Provided key data seems corrupted.");
    }
    if (vchTemp.size() != 68)
        return false;

    CDataStream ssKey(vchTemp, SER_NETWORK, PROTOCOL_VERSION);
    ssKey >> *this;

    return IsValid();
}

bool CMalleablePubKey::operator==(const CMalleablePubKey &b)
{
    return pubKeyL == b.pubKeyL && pubKeyH == b.pubKeyH;
}


// CMalleableKey

void CMalleableKey::Reset()
{
    vchSecretL.clear();
    vchSecretH.clear();
}

void CMalleableKey::MakeNewKeys()
{
    Reset();

    CKey keyL, keyH;
    keyL.MakeNewKey();
    keyH.MakeNewKey();

    vchSecretL = keyL.GetSecret();
    vchSecretH = keyH.GetSecret();
}

CMalleableKey::CMalleableKey()
{
    Reset();
}

CMalleableKey::CMalleableKey(const CMalleableKey &b)
{
    SetSecrets(b.vchSecretL, b.vchSecretH);
}

CMalleableKey::CMalleableKey(const CSecret &L, const CSecret &H)
{
    SetSecrets(L, H);
}

CMalleableKey::~CMalleableKey()
{
}

bool CMalleableKey::IsNull() const
{
    return vchSecretL.size() != 32 || vchSecretH.size() != 32;
}

bool CMalleableKey::SetSecrets(const CSecret &pvchSecretL, const CSecret &pvchSecretH)
{
    Reset();

    CKey keyL(pvchSecretL);
    CKey keyH(pvchSecretH);

    if (!keyL.IsValid() || !keyH.IsValid())
        return false;

    vchSecretL = pvchSecretL;
    vchSecretH = pvchSecretH;

    return true;
}

CMalleablePubKey CMalleableKey::GetMalleablePubKey() const
{
    CKey L(vchSecretL), H(vchSecretH);
    return CMalleablePubKey(L.GetPubKey(), H.GetPubKey());
}

// Check ownership
bool CMalleableKey::CheckKeyVariant(const CPubKey &R, const CPubKey &vchPubKeyVariant) const
{
    if (IsNull()) {
        throw key_error("CMalleableKey::CheckKeyVariant() : Attempting to run on NULL key object.");
    }

    if (!R.IsValid()) {
        printf("CMalleableKey::CheckKeyVariant() : R is invalid");
        return false;
    }

    if (!vchPubKeyVariant.IsValid()) {
        printf("CMalleableKey::CheckKeyVariant() : public key variant is invalid");
        return false;
    }

    CPoint point_R;
    if (!point_R.setPubKey(R)) {
        printf("CMalleableKey::CheckKeyVariant() : Unable to decode R value");
        return false;
    }

    CKey H(vchSecretH);
    CPubKey vchPubKeyH = H.GetPubKey();

    CPoint point_H;
    if (!point_H.setPubKey(vchPubKeyH)) {
        printf("CMalleableKey::CheckKeyVariant() : Unable to decode H value");
        return false;
    }

    CPoint point_P;
    if (!point_P.setPubKey(vchPubKeyVariant)) {
        printf("CMalleableKey::CheckKeyVariant() : Unable to decode P value");
        return false;
    }

    // Infinity points are senseless
    if (point_P.IsInfinity()) {
        printf("CMalleableKey::CheckKeyVariant() : P is infinity");
        return false;
    }

    CBigNum bnl;
    bnl.setBytes(std::vector<unsigned char>(vchSecretL.begin(), vchSecretL.end()));

    point_R.ECMUL(bnl);

    std::vector<unsigned char> vchRl;
    if (!point_R.getBytes(vchRl)) {
        printf("CMalleableKey::CheckKeyVariant() : Unable to convert Rl value");
        return false;
    }

    // Calculate Hash(R*l)
    CBigNum bnHash;
    bnHash.setuint160(Hash160(vchRl));

    CPoint point_Ps;
    // Calculate Ps = Hash(L*r)*G + H
    point_Ps.ECMULGEN(bnHash, point_H);

    // Infinity points are senseless
    if (point_Ps.IsInfinity()) {
        printf("CMalleableKey::CheckKeyVariant() : Ps is infinity");
        return false;
    }

    // Check ownership
    if (point_Ps != point_P) {
        return false;
    }

    return true;
}

// Check ownership and restore private key
bool CMalleableKey::CheckKeyVariant(const CPubKey &R, const CPubKey &vchPubKeyVariant, CKey &privKeyVariant) const
{
    if (IsNull()) {
        throw key_error("CMalleableKey::CheckKeyVariant() : Attempting to run on NULL key object.");
    }

    if (!R.IsValid()) {
        printf("CMalleableKey::CheckKeyVariant() : R is invalid");
        return false;
    }

    if (!vchPubKeyVariant.IsValid()) {
        printf("CMalleableKey::CheckKeyVariant() : public key variant is invalid");
        return false;
    }

    CPoint point_R;
    if (!point_R.setPubKey(R)) {
        printf("CMalleableKey::CheckKeyVariant() : Unable to decode R value");
        return false;
    }

    CKey H(vchSecretH);
    CPubKey vchPubKeyH = H.GetPubKey();

    CPoint point_H;
    if (!point_H.setPubKey(vchPubKeyH)) {
        printf("CMalleableKey::CheckKeyVariant() : Unable to decode H value");
        return false;
    }

    CPoint point_P;
    if (!point_P.setPubKey(vchPubKeyVariant)) {
        printf("CMalleableKey::CheckKeyVariant() : Unable to decode P value");
        return false;
    }

    // Infinity points are senseless
    if (point_P.IsInfinity()) {
        printf("CMalleableKey::CheckKeyVariant() : P is infinity");
        return false;
    }

    CBigNum bnl;
    bnl.setBytes(std::vector<unsigned char>(vchSecretL.begin(), vchSecretL.end()));

    point_R.ECMUL(bnl);

    std::vector<unsigned char> vchRl;
    if (!point_R.getBytes(vchRl)) {
        printf("CMalleableKey::CheckKeyVariant() : Unable to convert Rl value");
        return false;
    }

    // Calculate Hash(R*l)
    CBigNum bnHash;
    bnHash.setuint160(Hash160(vchRl));

    CPoint point_Ps;
    // Calculate Ps = Hash(L*r)*G + H
    point_Ps.ECMULGEN(bnHash, point_H);

    // Infinity points are senseless
    if (point_Ps.IsInfinity()) {
        printf("CMalleableKey::CheckKeyVariant() : Ps is infinity");
        return false;
    }

    // Check ownership
    if (point_Ps != point_P) {
        return false;
    }

    // OpenSSL BIGNUM representation of the second private key from (l, h) pair
    CBigNum bnh;
    bnh.setBytes(std::vector<unsigned char>(vchSecretH.begin(), vchSecretH.end()));

    // Calculate p = Hash(R*l) + h
    CBigNum bnp = bnHash + bnh;

    std::vector<unsigned char> vchp = bnp.getBytes();
    privKeyVariant.SetSecret(CSecret(vchp.begin(), vchp.end()));

    return true;
}

std::string CMalleableKey::ToString() const
{
    CDataStream ssKey(SER_NETWORK, PROTOCOL_VERSION);
    ssKey << *this;
    std::vector<unsigned char> vch(ssKey.begin(), ssKey.end());

    return EncodeBase58Check(vch);
}

std::vector<unsigned char> CMalleableKey::Raw() const
{
    CDataStream ssKey(SER_NETWORK, PROTOCOL_VERSION);
    ssKey << *this;
    std::vector<unsigned char> vch(ssKey.begin(), ssKey.end());

    return vch;
}

bool CMalleableKey::SetString(const std::string& strMutableKey)
{
    std::vector<unsigned char> vchTemp;
    if (!DecodeBase58Check(strMutableKey, vchTemp)) {
        throw key_error("CMalleableKey::SetString() : Provided key data seems corrupted.");
    }
    if (vchTemp.size() != 66)
        return false;
    CDataStream ssKey(vchTemp, SER_NETWORK, PROTOCOL_VERSION);
    ssKey >> *this;

    return IsValid();
}

// CMalleableKeyView

CMalleableKeyView::CMalleableKeyView(const std::string &strMalleableKey)
{
    SetString(strMalleableKey);
}

CMalleableKeyView::CMalleableKeyView(const CMalleableKey &b)
{
    if (b.vchSecretL.size() != 32)
        throw key_error("CMalleableKeyView::CMalleableKeyView() : L size must be 32 bytes");

    if (b.vchSecretH.size() != 32)
        throw key_error("CMalleableKeyView::CMalleableKeyView() : H size must be 32 bytes");

    vchSecretL = b.vchSecretL;

    CKey H(b.vchSecretH);
    vchPubKeyH = H.GetPubKey();
}

CMalleableKeyView::CMalleableKeyView(const CMalleableKeyView &b)
{
    vchSecretL = b.vchSecretL;
    vchPubKeyH = b.vchPubKeyH;
}

CMalleableKeyView& CMalleableKeyView::operator=(const CMalleableKey &b)
{
    vchSecretL = b.vchSecretL;

    CKey H(b.vchSecretH);
    vchPubKeyH = H.GetPubKey();

    return (*this);
}

CMalleableKeyView::~CMalleableKeyView()
{
}

CMalleablePubKey CMalleableKeyView::GetMalleablePubKey() const
{
    CKey keyL(vchSecretL);
    return CMalleablePubKey(keyL.GetPubKey(), vchPubKeyH);
}

// Check ownership
bool CMalleableKeyView::CheckKeyVariant(const CPubKey &R, const CPubKey &vchPubKeyVariant) const
{
    if (!IsValid()) {
        throw key_error("CMalleableKeyView::CheckKeyVariant() : Attempting to run on invalid view object.");
    }

    if (!R.IsValid()) {
        printf("CMalleableKeyView::CheckKeyVariant() : R is invalid");
        return false;
    }

    if (!vchPubKeyVariant.IsValid()) {
        printf("CMalleableKeyView::CheckKeyVariant() : public key variant is invalid");
        return false;
    }

    CPoint point_R;
    if (!point_R.setPubKey(R)) {
        printf("CMalleableKeyView::CheckKeyVariant() : Unable to decode R value");
        return false;
    }

    CPoint point_H;
    if (!point_H.setPubKey(vchPubKeyH)) {
        printf("CMalleableKeyView::CheckKeyVariant() : Unable to decode H value");
        return false;
    }

    CPoint point_P;
    if (!point_P.setPubKey(vchPubKeyVariant)) {
        printf("CMalleableKeyView::CheckKeyVariant() : Unable to decode P value");
        return false;
    }

    // Infinity points are senseless
    if (point_P.IsInfinity()) {
        printf("CMalleableKeyView::CheckKeyVariant() : P is infinity");
        return false;
    }

    CBigNum bnl;
    bnl.setBytes(std::vector<unsigned char>(vchSecretL.begin(), vchSecretL.end()));

    point_R.ECMUL(bnl);

    std::vector<unsigned char> vchRl;
    if (!point_R.getBytes(vchRl)) {
        printf("CMalleableKeyView::CheckKeyVariant() : Unable to convert Rl value");
        return false;
    }

    // Calculate Hash(R*l)
    CBigNum bnHash;
    bnHash.setuint160(Hash160(vchRl));

    CPoint point_Ps;
    // Calculate Ps = Hash(L*r)*G + H
    point_Ps.ECMULGEN(bnHash, point_H);

    // Infinity points are senseless
    if (point_Ps.IsInfinity()) {
        printf("CMalleableKeyView::CheckKeyVariant() : Ps is infinity");
        return false;
    }

    // Check ownership
    if (point_Ps != point_P) {
        return false;
    }

    return true;
}

std::string CMalleableKeyView::ToString() const
{
    CDataStream ssKey(SER_NETWORK, PROTOCOL_VERSION);
    ssKey << *this;
    std::vector<unsigned char> vch(ssKey.begin(), ssKey.end());

    return EncodeBase58Check(vch);
}

bool CMalleableKeyView::SetString(const std::string& strMutableKey)
{
    std::vector<unsigned char> vchTemp;
    if (!DecodeBase58Check(strMutableKey, vchTemp)) {
        throw key_error("CMalleableKeyView::SetString() : Provided key data seems corrupted.");
    }

    if (vchTemp.size() != 67)
        return false;

    CDataStream ssKey(vchTemp, SER_NETWORK, PROTOCOL_VERSION);
    ssKey >> *this;

    return IsValid();
}

std::vector<unsigned char> CMalleableKeyView::Raw() const
{
    CDataStream ssKey(SER_NETWORK, PROTOCOL_VERSION);
    ssKey << *this;
    std::vector<unsigned char> vch(ssKey.begin(), ssKey.end());

    return vch;
}


bool CMalleableKeyView::IsValid() const
{
    return vchSecretL.size() == 32 && GetMalleablePubKey().IsValid();
}

//// Asymmetric encryption

void CPubKey::EncryptData(const std::vector<unsigned char>& data, std::vector<unsigned char>& encrypted)
{
    ies_ctx_t *ctx;
    char error[1024] = "Unknown error";
    cryptogram_t *cryptogram;

    const unsigned char* pbegin = &vbytes[0];
    EC_KEY *pkey = EC_KEY_new_by_curve_name(NID_secp256k1);
    if (!o2i_ECPublicKey(&pkey, &pbegin, size()))
        throw key_error("Unable to parse EC key");

    ctx = create_context(pkey);
    if (!EC_KEY_get0_public_key(ctx->user_key))
        throw key_error("Given EC key is not public key");

    cryptogram = ecies_encrypt(ctx, (unsigned char*)&data[0], data.size(), error);
    if (cryptogram == NULL) {
        delete ctx;
        ctx = NULL;
        throw key_error(std::string("Error in encryption: %s") + error);
    }

    encrypted.resize(cryptogram_data_sum_length(cryptogram));
    unsigned char *key_data = cryptogram_key_data(cryptogram);
    memcpy(&encrypted[0], key_data, encrypted.size());
    cryptogram_free(cryptogram);
    delete ctx;
}

void CKey::DecryptData(const std::vector<unsigned char>& encrypted, std::vector<unsigned char>& data)
{
    ies_ctx_t *ctx;
    char error[1024] = "Unknown error";
    cryptogram_t *cryptogram;
    size_t length;
    unsigned char *decrypted;

    ctx = create_context(pkey);
    if (!EC_KEY_get0_private_key(ctx->user_key))
        throw key_error("Given EC key is not private key");

    size_t key_length = ctx->stored_key_length;
    size_t mac_length = EVP_MD_size(ctx->md);
    cryptogram = cryptogram_alloc(key_length, mac_length, encrypted.size() - key_length - mac_length);

    memcpy(cryptogram_key_data(cryptogram), &encrypted[0], encrypted.size());

    decrypted = ecies_decrypt(ctx, cryptogram, &length, error);
    cryptogram_free(cryptogram);
    delete ctx;

    if (decrypted == NULL) {
        throw key_error(std::string("Error in decryption: %s") + error);
    }

    data.resize(length);
    memcpy(&data[0], decrypted, length);
    free(decrypted);
}
