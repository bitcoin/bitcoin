#ifndef _SECP256K1_NUM_GMP_
#define _SECP256K1_NUM_GMP_

#include <assert.h>
#include <string>
#include <string.h>
#include <stdlib.h>
#include <gmp.h>

namespace secp256k1 {

class NumberState {
private:
    gmp_randstate_t rng;

public:
    NumberState() {
        gmp_randinit_default(rng);
    }

    ~NumberState() {
        gmp_randclear(rng);
    }

    void gen(mpz_t out, mpz_t size) {
        mpz_urandomm(out, rng, size);
    }
};

static NumberState number_state;

class Number {
private:
    mutable mpz_t bn;
    Number(const Number &x) {
    }
public:
    Number() {
        mpz_init(bn);
    }
    ~Number() {
        mpz_clear(bn);
    }
    Number(const unsigned char *bin, int len) {
        mpz_init(bn);
        SetBytes(bin,len);
    }
    Number &operator=(const Number &x) {
        mpz_set(bn, x.bn);
        return *this;
    }
    void SetNumber(const Number &x) {
        mpz_set(bn, x.bn);
    }
    void SetBytes(const unsigned char *bin, unsigned int len) {
        mpz_import(bn, len, 1, 1, 1, 0, bin);
    }
    void GetBytes(unsigned char *bin, unsigned int len) {
        unsigned int size = (mpz_sizeinbase(bn,2)+7)/8;
        assert(size <= len);
        memset(bin,0,len);
        size_t count = 0;
        mpz_export(bin + len - size, &count, 1, 1, 1, 0, bn);
        assert(count == 0 || size == count);
    }
    void SetInt(int x) {
        mpz_set_si(bn, x);
    }
    void SetModInverse(const Number &x, const Number &m) {
        mpz_invert(bn, x.bn, m.bn);
    }
    void SetModMul(const Number &a, const Number &b, const Number &m) {
        mpz_mul(bn, a.bn, b.bn);
        mpz_mod(bn, bn, m.bn);
    }
    void SetAdd(const Number &a1, const Number &a2) {
        mpz_add(bn, a1.bn, a2.bn);
    }
    void SetSub(const Number &a1, const Number &a2) {
        mpz_sub(bn, a1.bn, a2.bn);
    }
    void SetMult(const Number &a1, const Number &a2) {
        mpz_mul(bn, a1.bn, a2.bn);
    }
    void SetDiv(const Number &a1, const Number &a2) {
        mpz_tdiv_q(bn, a1.bn, a2.bn);
    }
    void SetMod(const Number &a, const Number &m) {
        mpz_mod(bn, a.bn, m.bn);
    }
    int Compare(const Number &a) const {
        return mpz_cmp(bn, a.bn);
    }
    int GetBits() const {
        return mpz_sizeinbase(bn,2);
    }
    // return the lowest (rightmost) bits bits, and rshift them away
    int ShiftLowBits(int bits) {
        int ret = mpz_get_ui(bn) & ((1 << bits) - 1);
        mpz_fdiv_q_2exp(bn, bn, bits);
        return ret;
    }
    // check whether number is 0,
    bool IsZero() const {
        return mpz_size(bn) == 0;
    }
    bool IsOdd() const {
        return mpz_get_ui(bn) & 1;
    }
    bool IsNeg() const {
        return mpz_sgn(bn) < 0;
    }
    void Negate() {
        mpz_neg(bn, bn);
    }
    void Shift1() {
        mpz_fdiv_q_2exp(bn, bn, 1);
    }
    void Inc() {
        mpz_add_ui(bn, bn, 1);
    }
    void SetHex(const std::string &str) {
        mpz_set_str(bn, str.c_str(), 16);
    }
    void SetPseudoRand(const Number &max) {
        number_state.gen(bn, max.bn);
    }
    void SplitInto(int bits, Number &low, Number &high) const {
        mpz_t tmp;
        mpz_init_set_ui(tmp,1);
        mpz_mul_2exp(tmp,tmp,bits);
        mpz_sub_ui(tmp,tmp,1);
        mpz_and(low.bn, bn, tmp);
        mpz_clear(tmp);
        mpz_fdiv_q_2exp(high.bn, bn, bits);
    }

    std::string ToString() const {
        char *str = (char*)malloc(mpz_sizeinbase(bn,16) + 2);
        mpz_get_str(str, 16, bn);
        std::string ret(str);
        free(str);
        return ret;
    }
};

}

#endif
