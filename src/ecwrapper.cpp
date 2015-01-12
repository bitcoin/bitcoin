// Copyright (c) 2009-2014 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "ecwrapper.h"

#include "serialize.h"
#include "uint256.h"

#include <openssl/bn.h>
#include <openssl/ecdsa.h>
#include <openssl/obj_mac.h>

namespace {

/**
 * Perform ECDSA key recovery (see SEC1 4.1.6) for curves over (mod p)-fields
 * recid selects which key is recovered
 * if check is non-zero, additional checks are performed
 */
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

bool CECKey::Verify(const uint256 &hash, const std::vector<unsigned char>& vchSig) {
    if (vchSig.empty())
        return false;

    // New versions of OpenSSL will reject non-canonical DER signatures. de/re-serialize first.
    unsigned char *norm_der = NULL;
    ECDSA_SIG *norm_sig = ECDSA_SIG_new();
    const unsigned char* sigptr = &vchSig[0];
    assert(norm_sig);
    if (d2i_ECDSA_SIG(&norm_sig, &sigptr, vchSig.size()) == NULL)
    {
        /* As of OpenSSL 1.0.0p d2i_ECDSA_SIG frees and nulls the pointer on
         * error. But OpenSSL's own use of this function redundantly frees the
         * result. As ECDSA_SIG_free(NULL) is a no-op, and in the absence of a
         * clear contract for the function behaving the same way is more
         * conservative.
         */
        ECDSA_SIG_free(norm_sig);
        return false;
    }
    int derlen = i2d_ECDSA_SIG(norm_sig, &norm_der);
    ECDSA_SIG_free(norm_sig);
    if (derlen <= 0)
        return false;

    // -1 = error, 0 = bad sig, 1 = good
    bool ret = ECDSA_verify(0, (unsigned char*)&hash, sizeof(hash), norm_der, derlen, pkey) == 1;
    OPENSSL_free(norm_der);
    return ret;
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
