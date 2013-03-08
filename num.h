#ifndef _SECP256K1_NUM_
#define _SECP256K1_NUM_

#include <assert.h>
#include <string.h>
#include <openssl/bn.h>

namespace secp256k1 {

class Context {
private:
    BN_CTX *bn_ctx;
    bool root;
    bool offspring;

public:
    operator BN_CTX*() {
        return bn_ctx;
    }

    Context() {
        bn_ctx = BN_CTX_new();
        BN_CTX_start(bn_ctx);
        root = true;
        offspring = false;
    }

    Context(Context &par) {
        bn_ctx = par.bn_ctx;
        root = false;
        offspring = false;
        par.offspring = true;
        BN_CTX_start(bn_ctx);
    }

    ~Context() {
        BN_CTX_end(bn_ctx);
        if (root)
            BN_CTX_free(bn_ctx);
    }

    BIGNUM *Get() {
        assert(offspring == false);
        return BN_CTX_get(bn_ctx);
    }
};


class Number {
private:
    BIGNUM *bn;
public:
    Number(Context &ctx) : bn(ctx.Get()) {}
    Number(Context &ctx, const unsigned char *bin, int len) : bn(ctx.Get()) {
        SetBytes(bin,len);
    }
    void SetBytes(const unsigned char *bin, int len) {
        BN_bin2bn(bin, len, bn);
    }
    void GetBytes(unsigned char *bin, int len) {
        int size = BN_num_bytes(bn);
        assert(size <= len);
        ::memset(bin,0,len);
        BN_bn2bin(bn, bin + size - len);
    }
    void SetModInverse(Context &ctx, const Number &x, const Number &m) {
        BN_mod_inverse(bn, x.bn, m.bn, ctx);
    }
};

}

#endif

