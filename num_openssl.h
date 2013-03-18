#ifndef _SECP256K1_NUM_OPENSSL_
#define _SECP256K1_NUM_OPENSSL_

#include <string>
#include <openssl/bn.h>

namespace secp256k1 {

class Number {
private:
    BIGNUM b;
    Number(const Number &x);

    operator const BIGNUM*() const;
    operator BIGNUM*();
public:
    Number();
    ~Number();
    Number(const unsigned char *bin, int len);
    void SetNumber(const Number &x);
    Number &operator=(const Number &x);
    void SetBytes(const unsigned char *bin, int len);
    void GetBytes(unsigned char *bin, int len);
    void SetInt(int x);
    void SetModInverse(const Number &x, const Number &m);
    void SetModMul(const Number &a, const Number &b, const Number &m);
    void SetAdd(const Number &a1, const Number &a2);
    void SetSub(const Number &a1, const Number &a2);
    void SetMult(const Number &a1, const Number &a2);
    void SetDiv(const Number &a1, const Number &a2);
    void SetMod(const Number &a, const Number &m);
    int Compare(const Number &a) const;
    int GetBits() const;
    int ShiftLowBits(int bits);
    bool IsZero() const;
    bool IsOdd() const;
    bool IsNeg() const;
    bool CheckBit(int pos) const;
    void Negate();
    void Shift1();
    void Inc();
    void SetHex(const std::string &str);
    void SetPseudoRand(const Number &max);
    void SplitInto(int bits, Number &low, Number &high) const;
    std::string ToString() const;
};

}

#endif
