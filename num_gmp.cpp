#include <assert.h>
#include <string>
#include <string.h>
#include <stdlib.h>
#include <gmp.h>

#include "num_gmp.h"

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

Number::Number(const Number &x) {
    mpz_init_set(bn, x.bn);
}

Number::Number() {
    mpz_init(bn);
}

Number::~Number() {
    mpz_clear(bn);
}

Number &Number::operator=(const Number &x) {
    mpz_set(bn, x.bn);
    return *this;
}

void Number::SetNumber(const Number &x) {
    mpz_set(bn, x.bn);
}

Number::Number(const unsigned char *bin, int len) {
    mpz_init(bn);
    SetBytes(bin,len);
}

void Number::SetBytes(const unsigned char *bin, unsigned int len) {
    mpz_import(bn, len, 1, 1, 1, 0, bin);
}

void Number::GetBytes(unsigned char *bin, unsigned int len) {
    unsigned int size = (mpz_sizeinbase(bn,2)+7)/8;
    assert(size <= len);
    memset(bin,0,len);
    size_t count = 0;
    mpz_export(bin + len - size, &count, 1, 1, 1, 0, bn);
    assert(count == 0 || size == count);
}

void Number::SetInt(int x) {
    mpz_set_si(bn, x);
}

void Number::SetModInverse(const Number &x, const Number &m) {
    mpz_invert(bn, x.bn, m.bn);
}

void Number::SetModMul(const Number &a, const Number &b, const Number &m) {
    mpz_mul(bn, a.bn, b.bn);
    mpz_mod(bn, bn, m.bn);
}

void Number::SetAdd(const Number &a1, const Number &a2) {
    mpz_add(bn, a1.bn, a2.bn);
}

void Number::SetSub(const Number &a1, const Number &a2) {
    mpz_sub(bn, a1.bn, a2.bn);
}

void Number::SetMult(const Number &a1, const Number &a2) {
    mpz_mul(bn, a1.bn, a2.bn);
}

void Number::SetDiv(const Number &a1, const Number &a2) {
    mpz_tdiv_q(bn, a1.bn, a2.bn);
}

void Number::SetMod(const Number &a, const Number &m) {
    mpz_mod(bn, a.bn, m.bn);
}

int Number::Compare(const Number &a) const {
    return mpz_cmp(bn, a.bn);
}

int Number::GetBits() const {
    return mpz_sizeinbase(bn,2);
}

int Number::ShiftLowBits(int bits) {
    int ret = mpz_get_ui(bn) & ((1 << bits) - 1);
    mpz_fdiv_q_2exp(bn, bn, bits);
    return ret;
}

bool Number::IsZero() const {
    return mpz_size(bn) == 0;
}

bool Number::IsOdd() const {
    return mpz_get_ui(bn) & 1;
}

bool Number::IsNeg() const {
    return mpz_sgn(bn) < 0;
}

void Number::Negate() {
    mpz_neg(bn, bn);
}

void Number::Shift1() {
    mpz_fdiv_q_2exp(bn, bn, 1);
}

void Number::Inc() {
    mpz_add_ui(bn, bn, 1);
}

void Number::SetHex(const std::string &str) {
    mpz_set_str(bn, str.c_str(), 16);
}

void Number::SetPseudoRand(const Number &max) {
    number_state.gen(bn, max.bn);
}

void Number::SplitInto(int bits, Number &low, Number &high) const {
    mpz_t tmp;
    mpz_init_set_ui(tmp,1);
    mpz_mul_2exp(tmp,tmp,bits);
    mpz_sub_ui(tmp,tmp,1);
    mpz_and(low.bn, bn, tmp);
    mpz_clear(tmp);
    mpz_fdiv_q_2exp(high.bn, bn, bits);
}

std::string Number::ToString() const {
    char *str = (char*)malloc(mpz_sizeinbase(bn,16) + 2);
    mpz_get_str(str, 16, bn);
    std::string ret(str);
    free(str);
    return ret;
}

}
