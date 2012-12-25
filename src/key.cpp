// Copyright (c) 2009-2012 The Bitcoin developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <map>

#include <openssl/ecdsa.h>
#include <openssl/obj_mac.h>

#include "key.h"

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


class CSecp256k1Math {
private:
    BIGNUM* order;
    BIGNUM* bnp;
    BIGNUM* bna1b2;
    BIGNUM* bnb1m;
    BIGNUM* bna2;
    BIGNUM* bnbeta;
    BIGNUM* bnlambda;
    EC_POINT* G128; // G * 2^128

    // Split a secp256k1 exponent k into two smaller ones k1 and k2 such that for any point Y,
    // k*Y = k1*Y + k2*Y', where Y' = lambda*Y is very fast
    void splitk(BIGNUM *bnk1, BIGNUM *bnk2, const BIGNUM *bnk, BN_CTX *ctx) {
        BN_CTX_start(ctx);
        BIGNUM *bnc1 = BN_CTX_get(ctx);
        BIGNUM *bnc2 = BN_CTX_get(ctx);
        BIGNUM *bnt1 = BN_CTX_get(ctx);
        BIGNUM *bnt2 = BN_CTX_get(ctx);
        BIGNUM *bnn2 = BN_CTX_get(ctx);

        BN_rshift1(bnn2, order);
        BN_mul(bnc1, bnk,  bna1b2, ctx);
        BN_add(bnc1, bnc1, bnn2);
        BN_div(bnc1, NULL, bnc1, order, ctx);
        BN_mul(bnc2, bnk,  bnb1m, ctx);
        BN_add(bnc2, bnc2, bnn2);
        BN_div(bnc2, NULL, bnc2, order, ctx);

        BN_mul(bnt1, bnc1, bna1b2, ctx);
        BN_mul(bnt2, bnc2, bna2, ctx);
        BN_add(bnt1, bnt1, bnt2);
        BN_sub(bnk1, bnk,  bnt1);
        BN_mul(bnt1, bnc1, bnb1m, ctx);
        BN_mul(bnt2, bnc2, bna1b2, ctx);
        BN_sub(bnk2, bnt1, bnt2);

#ifdef VERIFY_OPTIMIZED_SECP256K1
        // verify k == k1 + lambda*k2
        BIGNUM *bnt = BN_new();
        BN_mul(bnt, bnk2, bnlambda, ctx);
        BN_add(bnt, bnt, bnk1);
        BN_nnmod(bnt, bnt, order, ctx);
        assert(BN_cmp(bnk, bnt) == 0);
        BN_free(bnt);
#endif

        BN_CTX_end(ctx);
    }

    // p2 = lambda*p, where lambda is chosen such that this operation is very fast
    void mullambda(const EC_GROUP *group, EC_POINT *p2, const EC_POINT *p, BN_CTX *ctx) {
        BN_CTX_start(ctx);
        BIGNUM *x = BN_CTX_get(ctx);
        BIGNUM *y = BN_CTX_get(ctx);

        // deconstruct p as (x,y)
        EC_POINT_get_affine_coordinates_GFp(group, p, x, y, ctx);
        // x' = x*beta
        BN_mod_mul(x, x, bnbeta, bnp, ctx);
        // construct p2 as (x',y)
        EC_POINT_set_affine_coordinates_GFp(group, p2, x, y, ctx);

#ifdef VERIFY_OPTIMIZED_SECP256K1
        // verify p2 == lambda*p
        EC_POINT *t = EC_POINT_new(group);
        ::EC_POINT_mul(group, t, NULL, p, bnlambda, ctx);
        assert(EC_POINT_cmp(group, t, p2, ctx) == 0);
        EC_POINT_free(t);
#endif

        BN_CTX_end(ctx);
    }

public:
    CSecp256k1Math() {
        static const unsigned char a1b2[] =   {       0x30, 0x86, 0xd2, 0x21, 0xa7, 0xd4, 0x6b, 0xcd, 0xe8, 0x6c, 0x90, 0xe4, 0x92, 0x84, 0xeb, 0x15 };
        static const unsigned char b1m[] =    {       0xe4, 0x43, 0x7e, 0xd6, 0x01, 0x0e, 0x88, 0x28, 0x6f, 0x54, 0x7f, 0xa9, 0x0a, 0xbf, 0xe4, 0xc3 };
        static const unsigned char a2[] =     { 0x01, 0x14, 0xca, 0x50, 0xf7, 0xa8, 0xe2, 0xf3, 0xf6, 0x57, 0xc1, 0x10, 0x8d, 0x9d, 0x44, 0xcf, 0xd8 };
        static const unsigned char beta[] =   { 0x7a, 0xe9, 0x6a, 0x2b, 0x65, 0x7c, 0x07, 0x10, 0x6e, 0x64, 0x47, 0x9e, 0xac, 0x34, 0x34, 0xe9, 0x9c, 0xf0, 0x49, 0x75, 0x12, 0xf5, 0x89, 0x95, 0xc1, 0x39, 0x6c, 0x28, 0x71, 0x95, 0x01, 0xee };
        static const unsigned char lambda[] = { 0x53, 0x63, 0xad, 0x4c, 0xc0, 0x5c, 0x30, 0xe0, 0xa5, 0x26, 0x1c, 0x02, 0x88, 0x12, 0x64, 0x5a, 0x12, 0x2e, 0x22, 0xea, 0x20, 0x81, 0x66, 0x78, 0xdf, 0x02, 0x96, 0x7c, 0x1b, 0x23, 0xbd, 0x72 };

        EC_KEY *pkey = EC_KEY_new_by_curve_name(NID_secp256k1);
        const EC_GROUP *group = EC_KEY_get0_group(pkey);
        BN_CTX *ctx = BN_CTX_new();
        BN_CTX_start(ctx);
        BIGNUM *bn128 = BN_CTX_get(ctx);

        bnp = BN_new();
        order = BN_new();
        EC_GROUP_get_curve_GFp(group, bnp, NULL, NULL, ctx);
        EC_GROUP_get_order(group, order, ctx);
        bna1b2   = BN_bin2bn(a1b2,   sizeof(a1b2),   NULL);
        bnb1m    = BN_bin2bn(b1m,    sizeof(b1m),    NULL);
        bna2     = BN_bin2bn(a2,     sizeof(a2),     NULL);
        bnbeta   = BN_bin2bn(beta,   sizeof(beta),   NULL);
        bnlambda = BN_bin2bn(lambda, sizeof(lambda), NULL);

        BN_one(bn128);
        BN_lshift(bn128, bn128, 128);
        G128 = EC_POINT_new(group);
        ::EC_POINT_mul(group, G128, bn128, NULL, NULL, ctx);

        BN_CTX_end(ctx);
        BN_CTX_free(ctx);
        EC_KEY_free(pkey);
    }

    ~CSecp256k1Math() {
        EC_POINT_free(G128);
        BN_free(order);
        BN_free(bnp);
        BN_free(bna1b2);
        BN_free(bnb1m);
        BN_free(bna2);
        BN_free(bnbeta);
        BN_free(bnlambda);
    }

    // calculate r = n*G + m*q
    int EC_POINT_mul(const EC_GROUP *group, EC_POINT *r, const BIGNUM *n, const EC_POINT *q, const BIGNUM *m, BN_CTX *ctx) {
        BN_CTX_start(ctx);
        BIGNUM *na = BN_CTX_get(ctx);
        BIGNUM *nb = BN_CTX_get(ctx);
        BIGNUM *ma = BN_CTX_get(ctx);
        BIGNUM *mb = BN_CTX_get(ctx);
        EC_POINT *qlam = EC_POINT_new(group);

        // rewrite n*G as na*G + nb*G128, where na and nb are small, and G128 = G*2^128 is precomputed
        BN_copy(na, n);
        BN_mask_bits(na, 128);
        BN_rshift(nb, n, 128);

        // rewrite m*q as ma*q + mb*qlam, where ma and mb are small, and qlam = lambda*q is efficiently computable
        splitk(ma, mb, m, ctx); // split m
        mullambda(group, qlam, q, ctx); // calculate qlam = lamda*Q

        // the actual calculation now becomes: r = nb*G128 + ma*q + mb*qlam + na*G, where [na,nb,ma,mb] are small
        const EC_POINT *points[3] = {G128, q,  qlam};
        const BIGNUM   *exps[3]   = {nb,   ma, mb};
        int ret = EC_POINTs_mul(group, r, na, 3, points, exps, ctx); // the exponent na to G is passed separately

        EC_POINT_free(qlam);
        BN_CTX_end(ctx);
        return ret;
    }

    const BIGNUM *get_order() {
        return order;
    }
};

static CSecp256k1Math secp256k1math;

/** this is an almost exact copy of OpenSSL's ecdsa_do_verify, except:
 *  - it takes fixed size input
 *  - it uses a static order (which is a constant, assuming secp256k1)
 *  - it uses the optimized EC_POINT_mul operation from CSecp256k1Math
 */
int static secp256k1_ecdsa_do_verify(const unsigned char hash[32], const ECDSA_SIG *sig, const EC_KEY *eckey)
{
    int ret = -1;

    const EC_GROUP *group;
    const EC_POINT *pub_key;
    EC_POINT *point = NULL;
#ifdef VERIFY_OPTIMIZED_SECP256K1
    EC_POINT *point2 = NULL;
    BIGNUM *order2 = NULL;
#endif
    BIGNUM *m, *u2, *u1, *X;
    const BIGNUM *order;
    BN_CTX *ctx = NULL;

    // check input values
    if (eckey == NULL || (group = EC_KEY_get0_group(eckey)) == NULL ||
        (pub_key = EC_KEY_get0_public_key(eckey)) == NULL || sig == NULL)
        goto err;

    ctx = BN_CTX_new();
    BN_CTX_start(ctx);

    m = BN_CTX_get(ctx);
    u2 = BN_CTX_get(ctx);
    u1 = BN_CTX_get(ctx);
    X = BN_CTX_get(ctx);
    if (!X)
        goto err;

    order = secp256k1math.get_order();

#ifdef VERIFY_OPTIMIZED_SECP256K1
    // verify order == group.order
    order2 = BN_CTX_get(ctx);
    assert(EC_GROUP_get_order(group, order2, ctx));
    assert(BN_cmp(order, order2) == 0);
#endif

    // sanity checks
    if (BN_is_zero(sig->r) || BN_is_negative(sig->r) || BN_ucmp(sig->r, order) >= 0
        || BN_is_zero(sig->s) || BN_is_negative(sig->s) || BN_ucmp(sig->s, order) >= 0)
        goto err;

    // calculate tmp = inv(S) mod order
    if (!BN_mod_inverse(u2, sig->s, order, ctx))
        goto err;

    // turn message into number
    if (!BN_bin2bn(hash, 32, m))
        goto err;

    // u1 = m * tmp mod order
    if (!BN_mod_mul(u1, m, u2, order, ctx))
        goto err;

    // u2 = r * tmp mod order
    if (!BN_mod_mul(u2, sig->r, u2, order, ctx))
        goto err;

    // allocate result point
    if ((point = EC_POINT_new(group)) == NULL)
        goto err;

    // calculate point = u1*G + u2*pub_key
    if (!secp256k1math.EC_POINT_mul(group, point, u1, pub_key, u2, ctx))
        goto err;

#ifdef VERIFY_OPTIMIZED_SECP256K1
    // verify point == u1*G + u2*pub_key
    assert((point2 = EC_POINT_new(group)));
    assert(EC_POINT_mul(group, point2, u1, pub_key, u2, ctx));
    assert(EC_POINT_cmp(group, point, point2, ctx) == 0);
    EC_POINT_free(point2);
#endif

    // extract X coordinate from result point
    if (!EC_POINT_get_affine_coordinates_GFp(group, point, X, NULL, ctx))
        goto err;

    // u1 = X mod order
    if (!BN_nnmod(u1, X, order, ctx))
        goto err;

    // compare u1 to r
    ret = (BN_ucmp(u1, sig->r) == 0);

err:
    if (point)
        EC_POINT_free(point);
    if (ctx) {
        BN_CTX_end(ctx);
        BN_CTX_free(ctx);
    }

    return ret;
}

void CKey::SetCompressedPubKey(bool fCompressed)
{
    EC_KEY_set_conv_form(pkey, fCompressed ? POINT_CONVERSION_COMPRESSED : POINT_CONVERSION_UNCOMPRESSED);
    fCompressedPubKey = true;
}

void CKey::Reset()
{
    fCompressedPubKey = false;
    if (pkey != NULL)
        EC_KEY_free(pkey);
    pkey = EC_KEY_new_by_curve_name(NID_secp256k1);
    if (pkey == NULL)
        throw key_error("CKey::CKey() : EC_KEY_new_by_curve_name failed");
    fSet = false;
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

CKey& CKey::operator=(const CKey& b)
{
    if (!EC_KEY_copy(pkey, b.pkey))
        throw key_error("CKey::operator=(const CKey&) : EC_KEY_copy failed");
    fSet = b.fSet;
    return (*this);
}

CKey::~CKey()
{
    EC_KEY_free(pkey);
}

bool CKey::IsNull() const
{
    return !fSet;
}

bool CKey::IsCompressed() const
{
    return fCompressedPubKey;
}

void CKey::MakeNewKey(bool fCompressed)
{
    if (!EC_KEY_generate_key(pkey))
        throw key_error("CKey::MakeNewKey() : EC_KEY_generate_key failed");
    if (fCompressed)
        SetCompressedPubKey();
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
    if (fCompressed || fCompressedPubKey)
        SetCompressedPubKey();
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
    fCompressed = fCompressedPubKey;
    return vchRet;
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

bool CKey::SetPubKey(const CPubKey& vchPubKey)
{
    const unsigned char* pbegin = &vchPubKey.vchPubKey[0];
    if (o2i_ECPublicKey(&pkey, &pbegin, vchPubKey.vchPubKey.size()))
    {
        fSet = true;
        if (vchPubKey.vchPubKey.size() == 33)
            SetCompressedPubKey();
        return true;
    }
    pkey = NULL;
    Reset();
    return false;
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
    unsigned int nSize = ECDSA_size(pkey);
    vchSig.resize(nSize); // Make sure it is big enough
    if (!ECDSA_sign(0, (unsigned char*)&hash, sizeof(hash), &vchSig[0], &nSize, pkey))
    {
        vchSig.clear();
        return false;
    }
    vchSig.resize(nSize); // Shrink to fit actual size
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
    vchSig.clear();
    vchSig.resize(65,0);
    int nBitsR = BN_num_bits(sig->r);
    int nBitsS = BN_num_bits(sig->s);
    if (nBitsR <= 256 && nBitsS <= 256)
    {
        int nRecId = -1;
        for (int i=0; i<4; i++)
        {
            CKey keyRec;
            keyRec.fSet = true;
            if (fCompressedPubKey)
                keyRec.SetCompressedPubKey();
            if (ECDSA_SIG_recover_key_GFp(keyRec.pkey, sig, (unsigned char*)&hash, sizeof(hash), i, 1) == 1)
                if (keyRec.GetPubKey() == this->GetPubKey())
                {
                    nRecId = i;
                    break;
                }
        }

        if (nRecId == -1)
            throw key_error("CKey::SignCompact() : unable to construct recoverable key");

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
bool CKey::SetCompactSignature(uint256 hash, const std::vector<unsigned char>& vchSig)
{
    if (vchSig.size() != 65)
        return false;
    int nV = vchSig[0];
    if (nV<27 || nV>=35)
        return false;
    ECDSA_SIG *sig = ECDSA_SIG_new();
    BN_bin2bn(&vchSig[1],32,sig->r);
    BN_bin2bn(&vchSig[33],32,sig->s);

    EC_KEY_free(pkey);
    pkey = EC_KEY_new_by_curve_name(NID_secp256k1);
    if (nV >= 31)
    {
        SetCompressedPubKey();
        nV -= 4;
    }
    if (ECDSA_SIG_recover_key_GFp(pkey, sig, (unsigned char*)&hash, sizeof(hash), nV - 27, 0) == 1)
    {
        fSet = true;
        ECDSA_SIG_free(sig);
        return true;
    }
    return false;
}

bool CKey::Verify(uint256 hash, const std::vector<unsigned char>& vchSig)
{
    const unsigned char *ptr = &vchSig[0];
    ECDSA_SIG *sig = d2i_ECDSA_SIG(NULL, &ptr, vchSig.size());
    bool ret = (secp256k1_ecdsa_do_verify((unsigned char*)&hash, sig, pkey) == 1);
#ifdef VERIFY_OPTIMIZED_SECP256K1
    int fuzzpos = rand() % 256;
    hash += ((uint256)1) << fuzzpos;
    secp256k1_ecdsa_do_verify((unsigned char*)&hash, sig, pkey);
#endif
    ECDSA_SIG_free(sig);
    return ret;
}

bool CKey::VerifyCompact(uint256 hash, const std::vector<unsigned char>& vchSig)
{
    CKey key;
    if (!key.SetCompactSignature(hash, vchSig))
        return false;
    if (GetPubKey() != key.GetPubKey())
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
