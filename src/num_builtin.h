#ifndef _SECP256K1_NUM_GMP_
#define _SECP256K1_NUM_GMP_

#include <assert.h>
#include <string>
#include <string.h>
#include <stdlib.h>

namespace secp256k1 {

class Number {
private:
    uint64_t n[8]; // 512 bits ought to be enough for everyone
    int top; // number of used entries in n

    void FixTop() {
        while (top > 0 && n[top-1] == 0)
            top--;
    }

    int GetNumBytes() {
        if (top==0)
            return 0;
        int ret = 8*(top-1);
        uint64_t h = n[top-1];
        while (h>0) {
            ret++;
            h >>= 8;
        }
        return ret;
    }

    Number(const Number &c) {}
public:
    Number() {
        top = 0;
    }
    Number(const unsigned char *bin, int len) {
        top = 0;
        SetBytes(bin, len);
    }
    Number &operator=(const Number &x) {
        for (int pos = 0; pos < x.top; pos++)
            n[pos] = x.n[pos];
        top = x.top;
    }
    void SetNumber(const Number &x) {
        *this = x;
    }
    void SetBytes(const unsigned char *bin, unsigned int len) {
        n[0] = n[1] = n[2] = n[3] = n[4] = n[5] = n[6] = n[7] = 0;
        assert(len<=64);
        for (int pos = 0; pos < len; pos++)
            n[(len-1-pos)/8] |= ((uint64_t)(bin[pos])) << (((len-1-pos)%8)*8);
        top = 8;
        FixTop();
    }

    void GetBytes(unsigned char *bin, unsigned int len) {
        unsigned int size = GetNumBytes();
        assert(size <= len);
        memset(bin,0,len);
        for (int i=0; i<size; i++)
            bin[i] = (n[(size-i-1)/8] >> (((size-1-i)%8)*8)) & 0xFF;
    }

    void SetInt(int x) {
        n[0] = x;
        top = 1;
        FixTop();
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
