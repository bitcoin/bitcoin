#ifndef _SECP256K1_NUM_
#define _SECP256K1_NUM_

#include <assert.h>
#include <string>
#include <string.h>
#include <openssl/bn.h>
#include <openssl/crypto.h>

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
        memset(bin,0,len);
        BN_bn2bin(bn, bin + size - len);
    }
    void SetModInverse(Context &ctx, const Number &x, const Number &m) {
        BN_mod_inverse(bn, x.bn, m.bn, ctx);
    }
    int GetBits() const {
        return BN_num_bits(bn);
    }
    // return the lowest (rightmost) bits bits, and rshift them away
    int ShiftLowBits(Context &ctx, int bits) {
        Context ct(ctx);
        BIGNUM *tmp = ct.Get();
        BN_copy(tmp, bn);
        BN_mask_bits(tmp, bits);
        int ret = BN_get_word(tmp);
        BN_rshift(bn, bn, bits);
        return ret;
    }
    // check whether number is 0
    bool IsZero() {
        return BN_is_zero(bn);
    }
    // right-shift as many zeroes as possible
    int IsOdd() {
        return BN_is_odd(bn);
    }
    void Shift1() {
        BN_rshift1(bn,bn);
    }
    void Inc() {
        BN_add_word(bn,1);
    }
    void SetHex(const std::string &str) {
        BN_hex2bn(&bn, str.c_str());
    }
    std::string ToString() {
        char *str = BN_bn2hex(bn);
        std::string ret(str);
        OPENSSL_free(str);
        return ret;
    }
};

}

#endif
