// Copyright (c) 2009-2014 The Bitcoin developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "ecwrapper.h"

#include "serialize.h"
#include "uint256.h"

#include <openssl/bn.h>
#include <openssl/ecdsa.h>
#include <openssl/obj_mac.h>

namespace {

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

} // anon namespace

CECKey::CECKey() {
    pkey = EC_KEY_new_by_curve_name(NID_secp256k1);
    assert(pkey != NULL);
}

CECKey::~CECKey() {
    EC_KEY_free(pkey);
}

void CECKey::GetSecretBytes(unsigned char vch[32]) const {
    const BIGNUM *bn = EC_KEY_get0_private_key(pkey);
    assert(bn);
    int nBytes = BN_num_bytes(bn);
    int n=BN_bn2bin(bn,&vch[32 - nBytes]);
    assert(n == nBytes);
    memset(vch, 0, 32 - nBytes);
}

void CECKey::SetSecretBytes(const unsigned char vch[32]) {
    bool ret;
    BIGNUM bn;
    BN_init(&bn);
    ret = BN_bin2bn(vch, 32, &bn) != NULL;
    assert(ret);
    ret = EC_KEY_regenerate_key(pkey, &bn) != 0;
    assert(ret);
    BN_clear_free(&bn);
}

int CECKey::GetPrivKeySize(bool fCompressed) {
    EC_KEY_set_conv_form(pkey, fCompressed ? POINT_CONVERSION_COMPRESSED : POINT_CONVERSION_UNCOMPRESSED);
    return i2d_ECPrivateKey(pkey, NULL);
}
int CECKey::GetPrivKey(unsigned char* privkey, bool fCompressed) {
    EC_KEY_set_conv_form(pkey, fCompressed ? POINT_CONVERSION_COMPRESSED : POINT_CONVERSION_UNCOMPRESSED);
    return i2d_ECPrivateKey(pkey, &privkey);
}

bool CECKey::SetPrivKey(const unsigned char* privkey, size_t size, bool fSkipCheck) {
    if (d2i_ECPrivateKey(&pkey, &privkey, size)) {
        if(fSkipCheck)
            return true;

        // d2i_ECPrivateKey returns true if parsing succeeds.
        // This doesn't necessarily mean the key is valid.
        if (EC_KEY_check_key(pkey))
            return true;
    }
    return false;
}

void CECKey::GetPubKey(std::vector<unsigned char> &pubkey, bool fCompressed) {
    EC_KEY_set_conv_form(pkey, fCompressed ? POINT_CONVERSION_COMPRESSED : POINT_CONVERSION_UNCOMPRESSED);
    int nSize = i2o_ECPublicKey(pkey, NULL);
    assert(nSize);
    assert(nSize <= 65);
    pubkey.clear();
    pubkey.resize(nSize);
    unsigned char *pbegin(begin_ptr(pubkey));
    int nSize2 = i2o_ECPublicKey(pkey, &pbegin);
    assert(nSize == nSize2);
}

bool CECKey::SetPubKey(const unsigned char* pubkey, size_t size) {
    return o2i_ECPublicKey(&pkey, &pubkey, size) != NULL;
}

bool CECKey::Sign(const uint256 &hash, std::vector<unsigned char>& vchSig, bool lowS) {
    vchSig.clear();
    ECDSA_SIG *sig = ECDSA_do_sign((unsigned char*)&hash, sizeof(hash), pkey);
    if (sig == NULL)
        return false;
    BN_CTX *ctx = BN_CTX_new();
    BN_CTX_start(ctx);
    const EC_GROUP *group = EC_KEY_get0_group(pkey);
    BIGNUM *order = BN_CTX_get(ctx);
    BIGNUM *halforder = BN_CTX_get(ctx);
    EC_GROUP_get_order(group, order, ctx);
    BN_rshift1(halforder, order);
    if (lowS && BN_cmp(sig->s, halforder) > 0) {
        // enforce low S values, by negating the value (modulo the order) if above order/2.
        BN_sub(sig->s, order, sig->s);
    }
    BN_CTX_end(ctx);
    BN_CTX_free(ctx);
    unsigned int nSize = ECDSA_size(pkey);
    vchSig.resize(nSize); // Make sure it is big enough
    unsigned char *pos = &vchSig[0];
    nSize = i2d_ECDSA_SIG(sig, &pos);
    ECDSA_SIG_free(sig);
    vchSig.resize(nSize); // Shrink to fit actual size
    return true;
}

bool CECKey::Verify(const uint256 &hash, const std::vector<unsigned char>& vchSig) {
    // -1 = error, 0 = bad sig, 1 = good
    if (ECDSA_verify(0, (unsigned char*)&hash, sizeof(hash), &vchSig[0], vchSig.size(), pkey) != 1)
        return false;
    return true;
}

bool CECKey::SignCompact(const uint256 &hash, unsigned char *p64, int &rec) {
    bool fOk = false;
    ECDSA_SIG *sig = ECDSA_do_sign((unsigned char*)&hash, sizeof(hash), pkey);
    if (sig==NULL)
        return false;
    memset(p64, 0, 64);
    int nBitsR = BN_num_bits(sig->r);
    int nBitsS = BN_num_bits(sig->s);
    if (nBitsR <= 256 && nBitsS <= 256) {
        std::vector<unsigned char> pubkey;
        GetPubKey(pubkey, true);
        for (int i=0; i<4; i++) {
            CECKey keyRec;
            if (ECDSA_SIG_recover_key_GFp(keyRec.pkey, sig, (unsigned char*)&hash, sizeof(hash), i, 1) == 1) {
                std::vector<unsigned char> pubkeyRec;
                keyRec.GetPubKey(pubkeyRec, true);
                if (pubkeyRec == pubkey) {
                    rec = i;
                    fOk = true;
                    break;
                }
            }
        }
        assert(fOk);
        BN_bn2bin(sig->r,&p64[32-(nBitsR+7)/8]);
        BN_bn2bin(sig->s,&p64[64-(nBitsS+7)/8]);
    }
    ECDSA_SIG_free(sig);
    return fOk;
}

bool CECKey::Recover(const uint256 &hash, const unsigned char *p64, int rec)
{
    if (rec<0 || rec>=3)
        return false;
    ECDSA_SIG *sig = ECDSA_SIG_new();
    BN_bin2bn(&p64[0],  32, sig->r);
    BN_bin2bn(&p64[32], 32, sig->s);
    bool ret = ECDSA_SIG_recover_key_GFp(pkey, sig, (unsigned char*)&hash, sizeof(hash), rec, 0) == 1;
    ECDSA_SIG_free(sig);
    return ret;
}

bool CECKey::TweakSecret(unsigned char vchSecretOut[32], const unsigned char vchSecretIn[32], const unsigned char vchTweak[32])
{
    bool ret = true;
    BN_CTX *ctx = BN_CTX_new();
    BN_CTX_start(ctx);
    BIGNUM *bnSecret = BN_CTX_get(ctx);
    BIGNUM *bnTweak = BN_CTX_get(ctx);
    BIGNUM *bnOrder = BN_CTX_get(ctx);
    EC_GROUP *group = EC_GROUP_new_by_curve_name(NID_secp256k1);
    EC_GROUP_get_order(group, bnOrder, ctx); // what a grossly inefficient way to get the (constant) group order...
    BN_bin2bn(vchTweak, 32, bnTweak);
    if (BN_cmp(bnTweak, bnOrder) >= 0)
        ret = false; // extremely unlikely
    BN_bin2bn(vchSecretIn, 32, bnSecret);
    BN_add(bnSecret, bnSecret, bnTweak);
    BN_nnmod(bnSecret, bnSecret, bnOrder, ctx);
    if (BN_is_zero(bnSecret))
        ret = false; // ridiculously unlikely
    int nBits = BN_num_bits(bnSecret);
    memset(vchSecretOut, 0, 32);
    BN_bn2bin(bnSecret, &vchSecretOut[32-(nBits+7)/8]);
    EC_GROUP_free(group);
    BN_CTX_end(ctx);
    BN_CTX_free(ctx);
    return ret;
}

bool CECKey::TweakPublic(const unsigned char vchTweak[32]) {
    bool ret = true;
    BN_CTX *ctx = BN_CTX_new();
    BN_CTX_start(ctx);
    BIGNUM *bnTweak = BN_CTX_get(ctx);
    BIGNUM *bnOrder = BN_CTX_get(ctx);
    BIGNUM *bnOne = BN_CTX_get(ctx);
    const EC_GROUP *group = EC_KEY_get0_group(pkey);
    EC_GROUP_get_order(group, bnOrder, ctx); // what a grossly inefficient way to get the (constant) group order...
    BN_bin2bn(vchTweak, 32, bnTweak);
    if (BN_cmp(bnTweak, bnOrder) >= 0)
        ret = false; // extremely unlikely
    EC_POINT *point = EC_POINT_dup(EC_KEY_get0_public_key(pkey), group);
    BN_one(bnOne);
    EC_POINT_mul(group, point, bnTweak, point, bnOne, ctx);
    if (EC_POINT_is_at_infinity(group, point))
        ret = false; // ridiculously unlikely
    EC_KEY_set_public_key(pkey, point);
    EC_POINT_free(point);
    BN_CTX_end(ctx);
    BN_CTX_free(ctx);
    return ret;
}

bool CECKey::SanityCheck()
{
    EC_KEY *pkey = EC_KEY_new_by_curve_name(NID_secp256k1);
    if(pkey == NULL)
        return false;
    EC_KEY_free(pkey);

    // TODO Is there more EC functionality that could be missing?
    return true;
}
