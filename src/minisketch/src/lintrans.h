/**********************************************************************
 * Copyright (c) 2018 Pieter Wuille, Greg Maxwell, Gleb Naumenko      *
 * Distributed under the MIT software license, see the accompanying   *
 * file LICENSE or http://www.opensource.org/licenses/mit-license.php.*
 **********************************************************************/

#ifndef _MINISKETCH_LINTRANS_H_
#define _MINISKETCH_LINTRANS_H_

#include "int_utils.h"

/** A type to represent integers in the type system. */
template<int N> struct Num {};

/** A Linear N-bit transformation over the field I. */
template<typename I, int N> class LinTrans {
private:
    I table[1 << N];
public:
    LinTrans() = default;

    /* Construct a transformation over 3 to 8 bits, using the images of each bit. */
    constexpr LinTrans(I a, I b) : table{I(0), I(a), I(b), I(a ^ b)} {}
    constexpr LinTrans(I a, I b, I c) : table{I(0), I(a), I(b), I(a ^ b), I(c), I(a ^ c), I(b ^ c), I(a ^ b ^ c)} {}
    constexpr LinTrans(I a, I b, I c, I d) : table{I(0), I(a), I(b), I(a ^ b), I(c), I(a ^ c), I(b ^ c), I(a ^ b ^ c), I(d), I(a ^ d), I(b ^ d), I(a ^ b ^ d), I(c ^ d), I(a ^ c ^ d), I(b ^ c ^ d), I(a ^ b ^ c ^ d)} {}
    constexpr LinTrans(I a, I b, I c, I d, I e) : table{I(0), I(a), I(b), I(a ^ b), I(c), I(a ^ c), I(b ^ c), I(a ^ b ^ c), I(d), I(a ^ d), I(b ^ d), I(a ^ b ^ d), I(c ^ d), I(a ^ c ^ d), I(b ^ c ^ d), I(a ^ b ^ c ^ d), I(e), I(a ^ e), I(b ^ e), I(a ^ b ^ e), I(c ^ e), I(a ^ c ^ e), I(b ^ c ^ e), I(a ^ b ^ c ^ e), I(d ^ e), I(a ^ d ^ e), I(b ^ d ^ e), I(a ^ b ^ d ^ e), I(c ^ d ^ e), I(a ^ c ^ d ^ e), I(b ^ c ^ d ^ e), I(a ^ b ^ c ^ d ^ e)} {}
    constexpr LinTrans(I a, I b, I c, I d, I e, I f) : table{I(0), I(a), I(b), I(a ^ b), I(c), I(a ^ c), I(b ^ c), I(a ^ b ^ c), I(d), I(a ^ d), I(b ^ d), I(a ^ b ^ d), I(c ^ d), I(a ^ c ^ d), I(b ^ c ^ d), I(a ^ b ^ c ^ d), I(e), I(a ^ e), I(b ^ e), I(a ^ b ^ e), I(c ^ e), I(a ^ c ^ e), I(b ^ c ^ e), I(a ^ b ^ c ^ e), I(d ^ e), I(a ^ d ^ e), I(b ^ d ^ e), I(a ^ b ^ d ^ e), I(c ^ d ^ e), I(a ^ c ^ d ^ e), I(b ^ c ^ d ^ e), I(a ^ b ^ c ^ d ^ e), I(f), I(a ^ f), I(b^ f), I(a ^ b ^ f), I(c^ f), I(a ^ c ^ f), I(b ^ c ^ f), I(a ^ b ^ c ^ f), I(d ^ f), I(a ^ d ^ f), I(b ^ d ^ f), I(a ^ b ^ d ^ f), I(c ^ d ^ f), I(a ^ c ^ d ^ f), I(b ^ c ^ d ^ f), I(a ^ b ^ c ^ d ^ f), I(e ^ f), I(a ^ e ^ f), I(b ^ e ^ f), I(a ^ b ^ e ^ f), I(c ^ e ^ f), I(a ^ c ^ e ^ f), I(b ^ c ^ e ^ f), I(a ^ b ^ c ^ e ^ f), I(d ^ e ^ f), I(a ^ d ^ e ^ f), I(b ^ d ^ e ^ f), I(a ^ b ^ d ^ e ^ f), I(c ^ d ^ e ^ f), I(a ^ c ^ d ^ e ^ f), I(b ^ c ^ d ^ e ^ f), I(a ^ b ^ c ^ d ^ e ^ f)} {}
    constexpr LinTrans(I a, I b, I c, I d, I e, I f, I g) : table{I(0), I(a), I(b), I(a ^ b), I(c), I(a ^ c), I(b ^ c), I(a ^ b ^ c), I(d), I(a ^ d), I(b ^ d), I(a ^ b ^ d), I(c ^ d), I(a ^ c ^ d), I(b ^ c ^ d), I(a ^ b ^ c ^ d), I(e), I(a ^ e), I(b ^ e), I(a ^ b ^ e), I(c ^ e), I(a ^ c ^ e), I(b ^ c ^ e), I(a ^ b ^ c ^ e), I(d ^ e), I(a ^ d ^ e), I(b ^ d ^ e), I(a ^ b ^ d ^ e), I(c ^ d ^ e), I(a ^ c ^ d ^ e), I(b ^ c ^ d ^ e), I(a ^ b ^ c ^ d ^ e), I(f), I(a ^ f), I(b^ f), I(a ^ b ^ f), I(c^ f), I(a ^ c ^ f), I(b ^ c ^ f), I(a ^ b ^ c ^ f), I(d ^ f), I(a ^ d ^ f), I(b ^ d ^ f), I(a ^ b ^ d ^ f), I(c ^ d ^ f), I(a ^ c ^ d ^ f), I(b ^ c ^ d ^ f), I(a ^ b ^ c ^ d ^ f), I(e ^ f), I(a ^ e ^ f), I(b ^ e ^ f), I(a ^ b ^ e ^ f), I(c ^ e ^ f), I(a ^ c ^ e ^ f), I(b ^ c ^ e ^ f), I(a ^ b ^ c ^ e ^ f), I(d ^ e ^ f), I(a ^ d ^ e ^ f), I(b ^ d ^ e ^ f), I(a ^ b ^ d ^ e ^ f), I(c ^ d ^ e ^ f), I(a ^ c ^ d ^ e ^ f), I(b ^ c ^ d ^ e ^ f), I(a ^ b ^ c ^ d ^ e ^ f), I(g), I(a ^ g), I(b ^ g), I(a ^ b ^ g), I(c ^ g), I(a ^ c ^ g), I(b ^ c ^ g), I(a ^ b ^ c ^ g), I(d ^ g), I(a ^ d ^ g), I(b ^ d ^ g), I(a ^ b ^ d ^ g), I(c ^ d ^ g), I(a ^ c ^ d ^ g), I(b ^ c ^ d ^ g), I(a ^ b ^ c ^ d ^ g), I(e ^ g), I(a ^ e ^ g), I(b ^ e ^ g), I(a ^ b ^ e ^ g), I(c ^ e ^ g), I(a ^ c ^ e ^ g), I(b ^ c ^ e ^ g), I(a ^ b ^ c ^ e ^ g), I(d ^ e ^ g), I(a ^ d ^ e ^ g), I(b ^ d ^ e ^ g), I(a ^ b ^ d ^ e ^ g), I(c ^ d ^ e ^ g), I(a ^ c ^ d ^ e ^ g), I(b ^ c ^ d ^ e ^ g), I(a ^ b ^ c ^ d ^ e ^ g), I(f ^ g), I(a ^ f ^ g), I(b^ f ^ g), I(a ^ b ^ f ^ g), I(c^ f ^ g), I(a ^ c ^ f ^ g), I(b ^ c ^ f ^ g), I(a ^ b ^ c ^ f ^ g), I(d ^ f ^ g), I(a ^ d ^ f ^ g), I(b ^ d ^ f ^ g), I(a ^ b ^ d ^ f ^ g), I(c ^ d ^ f ^ g), I(a ^ c ^ d ^ f ^ g), I(b ^ c ^ d ^ f ^ g), I(a ^ b ^ c ^ d ^ f ^ g), I(e ^ f ^ g), I(a ^ e ^ f ^ g), I(b ^ e ^ f ^ g), I(a ^ b ^ e ^ f ^ g), I(c ^ e ^ f ^ g), I(a ^ c ^ e ^ f ^ g), I(b ^ c ^ e ^ f ^ g), I(a ^ b ^ c ^ e ^ f ^ g), I(d ^ e ^ f ^ g), I(a ^ d ^ e ^ f ^ g), I(b ^ d ^ e ^ f ^ g), I(a ^ b ^ d ^ e ^ f ^ g), I(c ^ d ^ e ^ f ^ g), I(a ^ c ^ d ^ e ^ f ^ g), I(b ^ c ^ d ^ e ^ f ^ g), I(a ^ b ^ c ^ d ^ e ^ f ^ g)} {}
    constexpr LinTrans(I a, I b, I c, I d, I e, I f, I g, I h) : table{I(0), I(a), I(b), I(a ^ b), I(c), I(a ^ c), I(b ^ c), I(a ^ b ^ c), I(d), I(a ^ d), I(b ^ d), I(a ^ b ^ d), I(c ^ d), I(a ^ c ^ d), I(b ^ c ^ d), I(a ^ b ^ c ^ d), I(e), I(a ^ e), I(b ^ e), I(a ^ b ^ e), I(c ^ e), I(a ^ c ^ e), I(b ^ c ^ e), I(a ^ b ^ c ^ e), I(d ^ e), I(a ^ d ^ e), I(b ^ d ^ e), I(a ^ b ^ d ^ e), I(c ^ d ^ e), I(a ^ c ^ d ^ e), I(b ^ c ^ d ^ e), I(a ^ b ^ c ^ d ^ e), I(f), I(a ^ f), I(b^ f), I(a ^ b ^ f), I(c^ f), I(a ^ c ^ f), I(b ^ c ^ f), I(a ^ b ^ c ^ f), I(d ^ f), I(a ^ d ^ f), I(b ^ d ^ f), I(a ^ b ^ d ^ f), I(c ^ d ^ f), I(a ^ c ^ d ^ f), I(b ^ c ^ d ^ f), I(a ^ b ^ c ^ d ^ f), I(e ^ f), I(a ^ e ^ f), I(b ^ e ^ f), I(a ^ b ^ e ^ f), I(c ^ e ^ f), I(a ^ c ^ e ^ f), I(b ^ c ^ e ^ f), I(a ^ b ^ c ^ e ^ f), I(d ^ e ^ f), I(a ^ d ^ e ^ f), I(b ^ d ^ e ^ f), I(a ^ b ^ d ^ e ^ f), I(c ^ d ^ e ^ f), I(a ^ c ^ d ^ e ^ f), I(b ^ c ^ d ^ e ^ f), I(a ^ b ^ c ^ d ^ e ^ f), I(g), I(a ^ g), I(b ^ g), I(a ^ b ^ g), I(c ^ g), I(a ^ c ^ g), I(b ^ c ^ g), I(a ^ b ^ c ^ g), I(d ^ g), I(a ^ d ^ g), I(b ^ d ^ g), I(a ^ b ^ d ^ g), I(c ^ d ^ g), I(a ^ c ^ d ^ g), I(b ^ c ^ d ^ g), I(a ^ b ^ c ^ d ^ g), I(e ^ g), I(a ^ e ^ g), I(b ^ e ^ g), I(a ^ b ^ e ^ g), I(c ^ e ^ g), I(a ^ c ^ e ^ g), I(b ^ c ^ e ^ g), I(a ^ b ^ c ^ e ^ g), I(d ^ e ^ g), I(a ^ d ^ e ^ g), I(b ^ d ^ e ^ g), I(a ^ b ^ d ^ e ^ g), I(c ^ d ^ e ^ g), I(a ^ c ^ d ^ e ^ g), I(b ^ c ^ d ^ e ^ g), I(a ^ b ^ c ^ d ^ e ^ g), I(f ^ g), I(a ^ f ^ g), I(b^ f ^ g), I(a ^ b ^ f ^ g), I(c^ f ^ g), I(a ^ c ^ f ^ g), I(b ^ c ^ f ^ g), I(a ^ b ^ c ^ f ^ g), I(d ^ f ^ g), I(a ^ d ^ f ^ g), I(b ^ d ^ f ^ g), I(a ^ b ^ d ^ f ^ g), I(c ^ d ^ f ^ g), I(a ^ c ^ d ^ f ^ g), I(b ^ c ^ d ^ f ^ g), I(a ^ b ^ c ^ d ^ f ^ g), I(e ^ f ^ g), I(a ^ e ^ f ^ g), I(b ^ e ^ f ^ g), I(a ^ b ^ e ^ f ^ g), I(c ^ e ^ f ^ g), I(a ^ c ^ e ^ f ^ g), I(b ^ c ^ e ^ f ^ g), I(a ^ b ^ c ^ e ^ f ^ g), I(d ^ e ^ f ^ g), I(a ^ d ^ e ^ f ^ g), I(b ^ d ^ e ^ f ^ g), I(a ^ b ^ d ^ e ^ f ^ g), I(c ^ d ^ e ^ f ^ g), I(a ^ c ^ d ^ e ^ f ^ g), I(b ^ c ^ d ^ e ^ f ^ g), I(a ^ b ^ c ^ d ^ e ^ f ^ g), I(h), I(a ^ h), I(b ^ h), I(a ^ b ^ h), I(c ^ h), I(a ^ c ^ h), I(b ^ c ^ h), I(a ^ b ^ c ^ h), I(d ^ h), I(a ^ d ^ h), I(b ^ d ^ h), I(a ^ b ^ d ^ h), I(c ^ d ^ h), I(a ^ c ^ d ^ h), I(b ^ c ^ d ^ h), I(a ^ b ^ c ^ d ^ h), I(e ^ h), I(a ^ e ^ h), I(b ^ e ^ h), I(a ^ b ^ e ^ h), I(c ^ e ^ h), I(a ^ c ^ e ^ h), I(b ^ c ^ e ^ h), I(a ^ b ^ c ^ e ^ h), I(d ^ e ^ h), I(a ^ d ^ e ^ h), I(b ^ d ^ e ^ h), I(a ^ b ^ d ^ e ^ h), I(c ^ d ^ e ^ h), I(a ^ c ^ d ^ e ^ h), I(b ^ c ^ d ^ e ^ h), I(a ^ b ^ c ^ d ^ e ^ h), I(f ^ h), I(a ^ f ^ h), I(b^ f ^ h), I(a ^ b ^ f ^ h), I(c^ f ^ h), I(a ^ c ^ f ^ h), I(b ^ c ^ f ^ h), I(a ^ b ^ c ^ f ^ h), I(d ^ f ^ h), I(a ^ d ^ f ^ h), I(b ^ d ^ f ^ h), I(a ^ b ^ d ^ f ^ h), I(c ^ d ^ f ^ h), I(a ^ c ^ d ^ f ^ h), I(b ^ c ^ d ^ f ^ h), I(a ^ b ^ c ^ d ^ f ^ h), I(e ^ f ^ h), I(a ^ e ^ f ^ h), I(b ^ e ^ f ^ h), I(a ^ b ^ e ^ f ^ h), I(c ^ e ^ f ^ h), I(a ^ c ^ e ^ f ^ h), I(b ^ c ^ e ^ f ^ h), I(a ^ b ^ c ^ e ^ f ^ h), I(d ^ e ^ f ^ h), I(a ^ d ^ e ^ f ^ h), I(b ^ d ^ e ^ f ^ h), I(a ^ b ^ d ^ e ^ f ^ h), I(c ^ d ^ e ^ f ^ h), I(a ^ c ^ d ^ e ^ f ^ h), I(b ^ c ^ d ^ e ^ f ^ h), I(a ^ b ^ c ^ d ^ e ^ f ^ h), I(g ^ h), I(a ^ g ^ h), I(b ^ g ^ h), I(a ^ b ^ g ^ h), I(c ^ g ^ h), I(a ^ c ^ g ^ h), I(b ^ c ^ g ^ h), I(a ^ b ^ c ^ g ^ h), I(d ^ g ^ h), I(a ^ d ^ g ^ h), I(b ^ d ^ g ^ h), I(a ^ b ^ d ^ g ^ h), I(c ^ d ^ g ^ h), I(a ^ c ^ d ^ g ^ h), I(b ^ c ^ d ^ g ^ h), I(a ^ b ^ c ^ d ^ g ^ h), I(e ^ g ^ h), I(a ^ e ^ g ^ h), I(b ^ e ^ g ^ h), I(a ^ b ^ e ^ g ^ h), I(c ^ e ^ g ^ h), I(a ^ c ^ e ^ g ^ h), I(b ^ c ^ e ^ g ^ h), I(a ^ b ^ c ^ e ^ g ^ h), I(d ^ e ^ g ^ h), I(a ^ d ^ e ^ g ^ h), I(b ^ d ^ e ^ g ^ h), I(a ^ b ^ d ^ e ^ g ^ h), I(c ^ d ^ e ^ g ^ h), I(a ^ c ^ d ^ e ^ g ^ h), I(b ^ c ^ d ^ e ^ g ^ h), I(a ^ b ^ c ^ d ^ e ^ g ^ h), I(f ^ g ^ h), I(a ^ f ^ g ^ h), I(b^ f ^ g ^ h), I(a ^ b ^ f ^ g ^ h), I(c^ f ^ g ^ h), I(a ^ c ^ f ^ g ^ h), I(b ^ c ^ f ^ g ^ h), I(a ^ b ^ c ^ f ^ g ^ h), I(d ^ f ^ g ^ h), I(a ^ d ^ f ^ g ^ h), I(b ^ d ^ f ^ g ^ h), I(a ^ b ^ d ^ f ^ g ^ h), I(c ^ d ^ f ^ g ^ h), I(a ^ c ^ d ^ f ^ g ^ h), I(b ^ c ^ d ^ f ^ g ^ h), I(a ^ b ^ c ^ d ^ f ^ g ^ h), I(e ^ f ^ g ^ h), I(a ^ e ^ f ^ g ^ h), I(b ^ e ^ f ^ g ^ h), I(a ^ b ^ e ^ f ^ g ^ h), I(c ^ e ^ f ^ g ^ h), I(a ^ c ^ e ^ f ^ g ^ h), I(b ^ c ^ e ^ f ^ g ^ h), I(a ^ b ^ c ^ e ^ f ^ g ^ h), I(d ^ e ^ f ^ g ^ h), I(a ^ d ^ e ^ f ^ g ^ h), I(b ^ d ^ e ^ f ^ g ^ h), I(a ^ b ^ d ^ e ^ f ^ g ^ h), I(c ^ d ^ e ^ f ^ g ^ h), I(a ^ c ^ d ^ e ^ f ^ g ^ h), I(b ^ c ^ d ^ e ^ f ^ g ^ h), I(a ^ b ^ c ^ d ^ e ^ f ^ g ^ h)} {}

    /* Construct a transformation over 3 to 8 bits, using a pointer to the bit's images. */
    constexpr LinTrans(const I* p, Num<2>) : LinTrans(I(p[0]), I(p[1])) {}
    constexpr LinTrans(const I* p, Num<3>) : LinTrans(I(p[0]), I(p[1]), I(p[2])) {}
    constexpr LinTrans(const I* p, Num<4>) : LinTrans(I(p[0]), I(p[1]), I(p[2]), I(p[3])) {}
    constexpr LinTrans(const I* p, Num<5>) : LinTrans(I(p[0]), I(p[1]), I(p[2]), I(p[3]), I(p[4])) {}
    constexpr LinTrans(const I* p, Num<6>) : LinTrans(I(p[0]), I(p[1]), I(p[2]), I(p[3]), I(p[4]), I(p[5])) {}
    constexpr LinTrans(const I* p, Num<7>) : LinTrans(I(p[0]), I(p[1]), I(p[2]), I(p[3]), I(p[4]), I(p[5]), I(p[6])) {}
    constexpr LinTrans(const I* p, Num<8>) : LinTrans(I(p[0]), I(p[1]), I(p[2]), I(p[3]), I(p[4]), I(p[5]), I(p[6]), I(p[7])) {}

    template<I (*F)(const I&)>
    inline I Build(Num<1>, I a)
    {
        table[0] = I(); table[1] = a;
        return a;
    }

    template<I (*F)(const I&)>
    inline I Build(Num<2>, I a)
    {
        I b = F(a);
        table[0] = I(); table[1] = a; table[2] = b; table[3] = a ^ b;
        return b;
    }

    template<I (*F)(const I&)>
    inline I Build(Num<3>, I a)
    {
        I b = F(a), c = F(b);
        table[0] = I(); table[1] = a; table[2] = b; table[3] = a ^ b; table[4] = c; table[5] = a ^ c; table[6] = b ^ c; table[7] = a ^ b ^ c;
        return c;
    }

    template<I (*F)(const I&)>
    inline I Build(Num<4>, I a)
    {
        I b = F(a), c = F(b), d = F(c);
        table[0] = I(); table[1] = a; table[2] = b; table[3] = a ^ b; table[4] = c; table[5] = a ^ c; table[6] = b ^ c; table[7] = a ^ b ^ c;
        table[8] = d; table[9] = a ^ d; table[10] = b ^ d; table[11] = a ^ b ^ d; table[12] = c ^ d; table[13] = a ^ c ^ d; table[14] = b ^ c ^ d; table[15] = a ^ b ^ c ^ d;
        return d;
    }

    template<I (*F)(const I&)>
    inline I Build(Num<5>, I a)
    {
        I b = F(a), c = F(b), d = F(c), e = F(d);
        table[0] = I(); table[1] = a; table[2] = b; table[3] = a ^ b; table[4] = c; table[5] = a ^ c; table[6] = b ^ c; table[7] = a ^ b ^ c;
        table[8] = d; table[9] = a ^ d; table[10] = b ^ d; table[11] = a ^ b ^ d; table[12] = c ^ d; table[13] = a ^ c ^ d; table[14] = b ^ c ^ d; table[15] = a ^ b ^ c ^ d;
        table[16] = e; table[17] = a ^ e; table[18] = b ^ e; table[19] = a ^ b ^ e; table[20] = c ^ e; table[21] = a ^ c ^ e; table[22] = b ^ c ^ e; table[23] = a ^ b ^ c ^ e;
        table[24] = d ^ e; table[25] = a ^ d ^ e; table[26] = b ^ d ^ e; table[27] = a ^ b ^ d ^ e; table[28] = c ^ d ^ e; table[29] = a ^ c ^ d ^ e; table[30] = b ^ c ^ d ^ e; table[31] = a ^ b ^ c ^ d ^ e;
        return e;
    }

    template<I (*F)(const I&)>
    inline I Build(Num<6>, I a)
    {
        I b = F(a), c = F(b), d = F(c), e = F(d), f = F(e);
        table[0] = I(); table[1] = a; table[2] = b; table[3] = a ^ b; table[4] = c; table[5] = a ^ c; table[6] = b ^ c; table[7] = a ^ b ^ c;
        table[8] = d; table[9] = a ^ d; table[10] = b ^ d; table[11] = a ^ b ^ d; table[12] = c ^ d; table[13] = a ^ c ^ d; table[14] = b ^ c ^ d; table[15] = a ^ b ^ c ^ d;
        table[16] = e; table[17] = a ^ e; table[18] = b ^ e; table[19] = a ^ b ^ e; table[20] = c ^ e; table[21] = a ^ c ^ e; table[22] = b ^ c ^ e; table[23] = a ^ b ^ c ^ e;
        table[24] = d ^ e; table[25] = a ^ d ^ e; table[26] = b ^ d ^ e; table[27] = a ^ b ^ d ^ e; table[28] = c ^ d ^ e; table[29] = a ^ c ^ d ^ e; table[30] = b ^ c ^ d ^ e; table[31] = a ^ b ^ c ^ d ^ e;
        table[32] = f; table[33] = a ^ f; table[34] = b ^ f; table[35] = a ^ b ^ f; table[36] = c ^ f; table[37] = a ^ c ^ f; table[38] = b ^ c ^ f; table[39] = a ^ b ^ c ^ f;
        table[40] = d ^ f; table[41] = a ^ d ^ f; table[42] = b ^ d ^ f; table[43] = a ^ b ^ d ^ f; table[44] = c ^ d ^ f; table[45] = a ^ c ^ d ^ f; table[46] = b ^ c ^ d ^ f; table[47] = a ^ b ^ c ^ d ^ f;
        table[48] = e ^ f; table[49] = a ^ e ^ f; table[50] = b ^ e ^ f; table[51] = a ^ b ^ e ^ f; table[52] = c ^ e ^ f; table[53] = a ^ c ^ e ^ f; table[54] = b ^ c ^ e ^ f; table[55] = a ^ b ^ c ^ e ^ f;
        table[56] = d ^ e ^ f; table[57] = a ^ d ^ e ^ f; table[58] = b ^ d ^ e ^ f; table[59] = a ^ b ^ d ^ e ^ f; table[60] = c ^ d ^ e ^ f; table[61] = a ^ c ^ d ^ e ^ f; table[62] = b ^ c ^ d ^ e ^ f; table[63] = a ^ b ^ c ^ d ^ e ^ f;
        return f;
    }

    template<typename O, int P>
    inline I constexpr Map(I a) const { return table[O::template MidBits<P, N>(a)]; }

    template<typename O, int P>
    inline I constexpr TopMap(I a) const { static_assert(P + N == O::SIZE, "TopMap inconsistency"); return table[O::template TopBits<N>(a)]; }
};


/** A linear transformation constructed using LinTrans tables for sections of bits. */
template<typename I, int... N> class RecLinTrans;

template<typename I, int N> class RecLinTrans<I, N> {
    LinTrans<I, N> trans;
public:
    static constexpr int BITS = N;
    constexpr RecLinTrans(const I* p, Num<BITS>) : trans(p, Num<N>()) {}
    constexpr RecLinTrans() = default;
    constexpr RecLinTrans(const I (&init)[BITS]) : RecLinTrans(init, Num<BITS>()) {}

    template<typename O, int P = 0>
    inline I constexpr Map(I a) const { return trans.template TopMap<O, P>(a); }

    template<I (*F)(const I&)>
    inline void Build(I a) { trans.template Build<F>(Num<N>(), a); }
};

template<typename I, int N, int... X> class RecLinTrans<I, N, X...> {
    LinTrans<I, N> trans;
    RecLinTrans<I, X...> rec;
public:
    static constexpr int BITS = RecLinTrans<I, X...>::BITS + N;
    constexpr RecLinTrans(const I* p, Num<BITS>) : trans(p, Num<N>()), rec(p + N, Num<BITS - N>()) {}
    constexpr RecLinTrans() = default;
    constexpr RecLinTrans(const I (&init)[BITS]) : RecLinTrans(init, Num<BITS>()) {}

    template<typename O, int P = 0>
    inline I constexpr Map(I a) const { return trans.template Map<O, P>(a) ^ rec.template Map<O, P + N>(a); }

    template<I (*F)(const I&)>
    inline void Build(I a) { I n = trans.template Build<F>(Num<N>(), a); rec.template Build<F>(F(n)); }
};

/** The identity transformation. */
class IdTrans {
public:
    template<typename O, typename I>
    inline I constexpr Map(I a) const { return a; }
};

/** A singleton for the identity transformation. */
constexpr IdTrans ID_TRANS{};

#endif
