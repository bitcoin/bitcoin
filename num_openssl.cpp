#include <assert.h>
#include <string>
#include <string.h>
#include <openssl/bn.h>
#include <openssl/crypto.h>

#include "num_openssl.h"

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

Number::operator const BIGNUM*() const {
    return &b;
}

Number::operator BIGNUM*() {
    return &b;
}

Number::Number() {
    BN_init(*this);
}

Number::~Number() {
    BN_free(*this);
}

Number::Number(const unsigned char *bin, int len) {
    BN_init(*this);
    SetBytes(bin,len);
}

void Number::SetNumber(const Number &x) {
    BN_copy(*this, x);
}

Number::Number(const Number &x) {
    BN_init(*this);
    BN_copy(*this, x);
}

Number &Number::operator=(const Number &x) {
    BN_copy(*this, x);
    return *this;
}

void Number::SetBytes(const unsigned char *bin, int len) {
    BN_bin2bn(bin, len, *this);
}

void Number::GetBytes(unsigned char *bin, int len) {
    int size = BN_num_bytes(*this);
    assert(size <= len);
    memset(bin,0,len);
    BN_bn2bin(*this, bin + len - size);
}

void Number::SetInt(int x) {
    if (x >= 0) {
        BN_set_word(*this, x);
    } else {
        BN_set_word(*this, -x);
        BN_set_negative(*this, 1);
    }
}

void Number::SetModInverse(const Number &x, const Number &m) {
    Context ctx;
    BN_mod_inverse(*this, x, m, ctx);
}

void Number::SetModMul(const Number &a, const Number &b, const Number &m) {
    Context ctx;
    BN_mod_mul(*this, a, b, m, ctx);
}

void Number::SetAdd(const Number &a1, const Number &a2) {
    BN_add(*this, a1, a2);
}

void Number::SetSub(const Number &a1, const Number &a2) {
    BN_sub(*this, a1, a2);
}

void Number::SetMult(const Number &a1, const Number &a2) {
    Context ctx;
    BN_mul(*this, a1, a2, ctx);
}

void Number::SetDiv(const Number &a1, const Number &a2) {
    Context ctx;
    BN_div(*this, NULL, a1, a2, ctx);
}

void Number::SetMod(const Number &a, const Number &m) {
    Context ctx;
    BN_nnmod(*this, a, m, ctx);
}

int Number::Compare(const Number &a) const {
    return BN_cmp(*this, a);
}

int Number::GetBits() const {
    return BN_num_bits(*this);
}

int Number::ShiftLowBits(int bits) {
    BIGNUM *bn = *this;
    int ret = BN_is_zero(bn) ? 0 : bn->d[0] & ((1 << bits) - 1);
    BN_rshift(*this, *this, bits);
    return ret;
}

bool Number::IsZero() const {
    return BN_is_zero((const BIGNUM*)*this);
}

bool Number::IsOdd() const {
    return BN_is_odd((const BIGNUM*)*this);
}

bool Number::CheckBit(int pos) const {
    return BN_is_bit_set((const BIGNUM*)*this, pos);
}

bool Number::IsNeg() const {
    return BN_is_negative((const BIGNUM*)*this);
}

void Number::Negate() {
    BN_set_negative(*this, !IsNeg());
}

void Number::Shift1() {
    BN_rshift1(*this,*this);
}

void Number::Inc() {
    BN_add_word(*this,1);
}

void Number::SetHex(const std::string &str) {
    BIGNUM *bn = *this;
    BN_hex2bn(&bn, str.c_str());
}

void Number::SetPseudoRand(const Number &max) {
    BN_pseudo_rand_range(*this, max);
}

void Number::SplitInto(int bits, Number &low, Number &high) const {
    BN_copy(low, *this);
    BN_mask_bits(low, bits);
    BN_rshift(high, *this, bits);
}

std::string Number::ToString() const {
    char *str = BN_bn2hex(*this);
    std::string ret(str);
    OPENSSL_free(str);
    return ret;
}

}
