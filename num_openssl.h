#ifndef _SECP256K1_NUM_OPENSSL_
#define _SECP256K1_NUM_OPENSSL_

#include <assert.h>
#include <string>
#include <string.h>
#include <openssl/bn.h>
#include <openssl/crypto.h>

#include <stdint.h>

namespace secp256k1 {

class Context {
private:
    BN_CTX *ctx;

    operator BN_CTX*() {
        return ctx;
    }

    friend class Number;
public:
    Context() {
        ctx = BN_CTX_new();
    }

    ~Context() {
        BN_CTX_free(ctx);
    }
};

class Number {
private:
    BIGNUM b;
    Number(const Number &x) {}

    operator const BIGNUM*() const {
        return &b;
    }

    operator BIGNUM*() {
        return &b;
    }
public:
    Number() {
        BN_init(*this);
    }

    ~Number() {
        BN_free(*this);
    }

    Number(const unsigned char *bin, int len) {
        BN_init(*this);
        SetBytes(bin,len);
    }
    void SetNumber(const Number &x) {
        BN_copy(*this, x);
    }
    Number &operator=(const Number &x) {
        BN_copy(*this, x);
        return *this;
    }
    void SetBytes(const unsigned char *bin, int len) {
        BN_bin2bn(bin, len, *this);
    }
    void GetBytes(unsigned char *bin, int len) {
        int size = BN_num_bytes(*this);
        assert(size <= len);
        memset(bin,0,len);
        BN_bn2bin(*this, bin + len - size);
    }
    void SetInt(int x) {
        if (x >= 0) {
            BN_set_word(*this, x);
        } else {
            BN_set_word(*this, -x);
            BN_set_negative(*this, 1);
        }
    }
    void SetModInverse(const Number &x, const Number &m) {
        Context ctx;
        BN_mod_inverse(*this, x, m, ctx);
    }
    void SetModMul(const Number &a, const Number &b, const Number &m) {
        Context ctx;
        BN_mod_mul(*this, a, b, m, ctx);
    }
    void SetAdd(const Number &a1, const Number &a2) {
        BN_add(*this, a1, a2);
    }
    void SetSub(const Number &a1, const Number &a2) {
        BN_sub(*this, a1, a2);
    }
    void SetMult(const Number &a1, const Number &a2) {
        Context ctx;
        BN_mul(*this, a1, a2, ctx);
    }
    void SetDiv(const Number &a1, const Number &a2) {
        Context ctx;
        BN_div(*this, NULL, a1, a2, ctx);
    }
    void SetMod(const Number &a, const Number &m) {
        Context ctx;
        BN_nnmod(*this, a, m, ctx);
    }
    int Compare(const Number &a) const {
        return BN_cmp(*this, a);
    }
    int GetBits() const {
        return BN_num_bits(*this);
    }
    // return the lowest (rightmost) bits bits, and rshift them away
    int ShiftLowBits(int bits) {
        BIGNUM *bn = *this;
        int ret = BN_is_zero(bn) ? 0 : bn->d[0] & ((1 << bits) - 1);
        BN_rshift(*this, *this, bits);
        return ret;
    }
    // check whether number is 0,
    bool IsZero() const {
        return BN_is_zero((const BIGNUM*)*this);
    }
    bool IsOdd() const {
        return BN_is_odd((const BIGNUM*)*this);
    }
    bool IsNeg() const {
        return BN_is_negative((const BIGNUM*)*this);
    }
    void Negate() {
        BN_set_negative(*this, !IsNeg());
    }
    void Shift1() {
        BN_rshift1(*this,*this);
    }
    void Inc() {
        BN_add_word(*this,1);
    }
    void SetHex(const std::string &str) {
        BIGNUM *bn = *this;
        BN_hex2bn(&bn, str.c_str());
    }
    void SetPseudoRand(const Number &max) {
        BN_pseudo_rand_range(*this, max);
    }
    void SplitInto(int bits, Number &low, Number &high) const {
        BN_copy(low, *this);
        BN_mask_bits(low, bits);
        BN_rshift(high, *this, bits);
    }

    std::string ToString() const {
        char *str = BN_bn2hex(*this);
        std::string ret(str);
        OPENSSL_free(str);
        return ret;
    }
};

}

#endif
