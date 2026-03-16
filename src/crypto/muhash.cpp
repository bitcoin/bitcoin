// Copyright (c) 2017-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <crypto/muhash.h>

#include <crypto/chacha20.h>
#include <crypto/common.h>
#include <hash.h>
#include <span.h>
#include <uint256.h>
#include <util/check.h>

#include <bit>
#include <cstring>
#include <limits>

namespace {

using limb_t = Num3072::limb_t;
using signed_limb_t = Num3072::signed_limb_t;
using double_limb_t = Num3072::double_limb_t;
using signed_double_limb_t = Num3072::signed_double_limb_t;
constexpr int LIMB_SIZE = Num3072::LIMB_SIZE;
constexpr int SIGNED_LIMB_SIZE = Num3072::SIGNED_LIMB_SIZE;
constexpr int LIMBS = Num3072::LIMBS;
constexpr int SIGNED_LIMBS = Num3072::SIGNED_LIMBS;
constexpr int FINAL_LIMB_POSITION = 3072 / SIGNED_LIMB_SIZE;
constexpr int FINAL_LIMB_MODULUS_BITS = 3072 % SIGNED_LIMB_SIZE;
constexpr limb_t MAX_LIMB = (limb_t)(-1);
constexpr limb_t MAX_SIGNED_LIMB = (((limb_t)1) << SIGNED_LIMB_SIZE) - 1;
/** 2^3072 - 1103717, the largest 3072-bit safe prime number, is used as the modulus. */
constexpr limb_t MAX_PRIME_DIFF = 1103717;
/** The modular inverse of (2**3072 - MAX_PRIME_DIFF) mod (MAX_SIGNED_LIMB + 1). */
constexpr limb_t MODULUS_INVERSE = limb_t(0x70a1421da087d93);


/** Extract the lowest limb of [c0,c1,c2] into n, and left shift the number by 1 limb. */
inline void extract3(limb_t& c0, limb_t& c1, limb_t& c2, limb_t& n)
{
    n = c0;
    c0 = c1;
    c1 = c2;
    c2 = 0;
}

/** [c0,c1] = a * b */
inline void mul(limb_t& c0, limb_t& c1, const limb_t& a, const limb_t& b)
{
    double_limb_t t = (double_limb_t)a * b;
    c1 = t >> LIMB_SIZE;
    c0 = t;
}

/* [c0,c1,c2] += n * [d0,d1,d2]. c2 is 0 initially */
inline void mulnadd3(limb_t& c0, limb_t& c1, limb_t& c2, limb_t& d0, limb_t& d1, limb_t& d2, const limb_t& n)
{
    double_limb_t t = (double_limb_t)d0 * n + c0;
    c0 = t;
    t >>= LIMB_SIZE;
    t += (double_limb_t)d1 * n + c1;
    c1 = t;
    t >>= LIMB_SIZE;
    c2 = t + d2 * n;
}

/* [c0,c1] *= n */
inline void muln2(limb_t& c0, limb_t& c1, const limb_t& n)
{
    double_limb_t t = (double_limb_t)c0 * n;
    c0 = t;
    t >>= LIMB_SIZE;
    t += (double_limb_t)c1 * n;
    c1 = t;
}

/** [c0,c1,c2] += a * b */
inline void muladd3(limb_t& c0, limb_t& c1, limb_t& c2, const limb_t& a, const limb_t& b)
{
    double_limb_t t = (double_limb_t)a * b;
    limb_t th = t >> LIMB_SIZE;
    limb_t tl = t;

    c0 += tl;
    th += (c0 < tl) ? 1 : 0;
    c1 += th;
    c2 += (c1 < th) ? 1 : 0;
}

/**
 * Add limb a to [c0,c1]: [c0,c1] += a. Then extract the lowest
 * limb of [c0,c1] into n, and left shift the number by 1 limb.
 * */
inline void addnextract2(limb_t& c0, limb_t& c1, const limb_t& a, limb_t& n)
{
    limb_t c2 = 0;

    // add
    c0 += a;
    if (c0 < a) {
        c1 += 1;

        // Handle case when c1 has overflown
        if (c1 == 0) c2 = 1;
    }

    // extract
    n = c0;
    c0 = c1;
    c1 = c2;
}

} // namespace

/** Indicates whether d is larger than the modulus. */
bool Num3072::IsOverflow() const
{
    if (this->limbs[0] <= std::numeric_limits<limb_t>::max() - MAX_PRIME_DIFF) return false;
    for (int i = 1; i < LIMBS; ++i) {
        if (this->limbs[i] != std::numeric_limits<limb_t>::max()) return false;
    }
    return true;
}

void Num3072::FullReduce()
{
    limb_t c0 = MAX_PRIME_DIFF;
    limb_t c1 = 0;
    for (int i = 0; i < LIMBS; ++i) {
        addnextract2(c0, c1, this->limbs[i], this->limbs[i]);
    }
}

namespace {
/** A type representing a number in signed limb representation. */
struct Num3072Signed
{
    /** The represented value is sum(limbs[i]*2^(SIGNED_LIMB_SIZE*i), i=0..SIGNED_LIMBS-1).
     *  Note that limbs may be negative, or exceed 2^SIGNED_LIMB_SIZE-1. */
    signed_limb_t limbs[SIGNED_LIMBS];

    /** Construct a Num3072Signed with value 0. */
    Num3072Signed()
    {
        memset(limbs, 0, sizeof(limbs));
    }

    /** Convert a Num3072 to a Num3072Signed. Output will be normalized and in
     *  range 0..2^3072-1. */
    void FromNum3072(const Num3072& in)
    {
        double_limb_t c = 0;
        int b = 0, outpos = 0;
        for (int i = 0; i < LIMBS; ++i) {
            c += double_limb_t{in.limbs[i]} << b;
            b += LIMB_SIZE;
            while (b >= SIGNED_LIMB_SIZE) {
                limbs[outpos++] = limb_t(c) & MAX_SIGNED_LIMB;
                c >>= SIGNED_LIMB_SIZE;
                b -= SIGNED_LIMB_SIZE;
            }
        }
        Assume(outpos == SIGNED_LIMBS - 1);
        limbs[SIGNED_LIMBS - 1] = c;
        c >>= SIGNED_LIMB_SIZE;
        Assume(c == 0);
    }

    /** Convert a Num3072Signed to a Num3072. Input must be in range 0..modulus-1. */
    void ToNum3072(Num3072& out) const
    {
        double_limb_t c = 0;
        int b = 0, outpos = 0;
        for (int i = 0; i < SIGNED_LIMBS; ++i) {
            c += double_limb_t(limbs[i]) << b;
            b += SIGNED_LIMB_SIZE;
            if (b >= LIMB_SIZE) {
                out.limbs[outpos++] = c;
                c >>= LIMB_SIZE;
                b -= LIMB_SIZE;
            }
        }
        Assume(outpos == LIMBS);
        Assume(c == 0);
    }

    /** Take a Num3072Signed in range 1-2*2^3072..2^3072-1, and:
     *  - optionally negate it (if negate is true)
     *  - reduce it modulo the modulus (2^3072 - MAX_PRIME_DIFF)
     *  - produce output with all limbs in range 0..2^SIGNED_LIMB_SIZE-1
     */
    void Normalize(bool negate)
    {
        // Add modulus if this was negative. This brings the range of *this to 1-2^3072..2^3072-1.
        signed_limb_t cond_add = limbs[SIGNED_LIMBS-1] >> (LIMB_SIZE-1); // -1 if this is negative; 0 otherwise
        limbs[0] += signed_limb_t(-MAX_PRIME_DIFF) & cond_add;
        limbs[FINAL_LIMB_POSITION] += (signed_limb_t(1) << FINAL_LIMB_MODULUS_BITS) & cond_add;
        // Next negate all limbs if negate was set. This does not change the range of *this.
        signed_limb_t cond_negate = -signed_limb_t(negate); // -1 if this negate is true; 0 otherwise
        for (int i = 0; i < SIGNED_LIMBS; ++i) {
            limbs[i] = (limbs[i] ^ cond_negate) - cond_negate;
        }
        // Perform carry (make all limbs except the top one be in range 0..2^SIGNED_LIMB_SIZE-1).
        for (int i = 0; i < SIGNED_LIMBS - 1; ++i) {
            limbs[i + 1] += limbs[i] >> SIGNED_LIMB_SIZE;
            limbs[i] &= MAX_SIGNED_LIMB;
        }
        // Again add modulus if *this was negative. This brings the range of *this to 0..2^3072-1.
        cond_add = limbs[SIGNED_LIMBS-1] >> (LIMB_SIZE-1); // -1 if this is negative; 0 otherwise
        limbs[0] += signed_limb_t(-MAX_PRIME_DIFF) & cond_add;
        limbs[FINAL_LIMB_POSITION] += (signed_limb_t(1) << FINAL_LIMB_MODULUS_BITS) & cond_add;
        // Perform another carry. Now all limbs are in range 0..2^SIGNED_LIMB_SIZE-1.
        for (int i = 0; i < SIGNED_LIMBS - 1; ++i) {
            limbs[i + 1] += limbs[i] >> SIGNED_LIMB_SIZE;
            limbs[i] &= MAX_SIGNED_LIMB;
        }
    }
};

/** 2x2 transformation matrix with signed_limb_t elements. */
struct SignedMatrix
{
    signed_limb_t u, v, q, r;
};

/** Compute the transformation matrix for SIGNED_LIMB_SIZE divsteps.
 *
 * eta: initial eta value
 * f:   bottom SIGNED_LIMB_SIZE bits of initial f value
 * g:   bottom SIGNED_LIMB_SIZE bits of initial g value
 * out: resulting transformation matrix, scaled by 2^SIGNED_LIMB_SIZE
 * return: eta value after SIGNED_LIMB_SIZE divsteps
 */
inline limb_t ComputeDivstepMatrix(signed_limb_t eta, limb_t f, limb_t g, SignedMatrix& out)
{
    /** inv256[i] = -1/(2*i+1) (mod 256) */
    static const uint8_t NEGINV256[128] = {
        0xFF, 0x55, 0x33, 0x49, 0xC7, 0x5D, 0x3B, 0x11, 0x0F, 0xE5, 0xC3, 0x59,
        0xD7, 0xED, 0xCB, 0x21, 0x1F, 0x75, 0x53, 0x69, 0xE7, 0x7D, 0x5B, 0x31,
        0x2F, 0x05, 0xE3, 0x79, 0xF7, 0x0D, 0xEB, 0x41, 0x3F, 0x95, 0x73, 0x89,
        0x07, 0x9D, 0x7B, 0x51, 0x4F, 0x25, 0x03, 0x99, 0x17, 0x2D, 0x0B, 0x61,
        0x5F, 0xB5, 0x93, 0xA9, 0x27, 0xBD, 0x9B, 0x71, 0x6F, 0x45, 0x23, 0xB9,
        0x37, 0x4D, 0x2B, 0x81, 0x7F, 0xD5, 0xB3, 0xC9, 0x47, 0xDD, 0xBB, 0x91,
        0x8F, 0x65, 0x43, 0xD9, 0x57, 0x6D, 0x4B, 0xA1, 0x9F, 0xF5, 0xD3, 0xE9,
        0x67, 0xFD, 0xDB, 0xB1, 0xAF, 0x85, 0x63, 0xF9, 0x77, 0x8D, 0x6B, 0xC1,
        0xBF, 0x15, 0xF3, 0x09, 0x87, 0x1D, 0xFB, 0xD1, 0xCF, 0xA5, 0x83, 0x19,
        0x97, 0xAD, 0x8B, 0xE1, 0xDF, 0x35, 0x13, 0x29, 0xA7, 0x3D, 0x1B, 0xF1,
        0xEF, 0xC5, 0xA3, 0x39, 0xB7, 0xCD, 0xAB, 0x01
    };
    // Coefficients of returned SignedMatrix; starts off as identity matrix. */
    limb_t u = 1, v = 0, q = 0, r = 1;
    // The number of divsteps still left.
    int i = SIGNED_LIMB_SIZE;
    while (true) {
        /* Use a sentinel bit to count zeros only up to i. */
        int zeros = std::countr_zero(g | (MAX_LIMB << i));
        /* Perform zeros divsteps at once; they all just divide g by two. */
        g >>= zeros;
        u <<= zeros;
        v <<= zeros;
        eta -= zeros;
        i -= zeros;
         /* We're done once we've performed SIGNED_LIMB_SIZE divsteps. */
        if (i == 0) break;
        /* If eta is negative, negate it and replace f,g with g,-f. */
        if (eta < 0) {
            limb_t tmp;
            eta = -eta;
            tmp = f; f = g; g = -tmp;
            tmp = u; u = q; q = -tmp;
            tmp = v; v = r; r = -tmp;
        }
        /* eta is now >= 0. In what follows we're going to cancel out the bottom bits of g. No more
         * than i can be cancelled out (as we'd be done before that point), and no more than eta+1
         * can be done as its sign will flip once that happens. */
        int limit = ((int)eta + 1) > i ? i : ((int)eta + 1);
        /* m is a mask for the bottom min(limit, 8) bits (our table only supports 8 bits). */
        limb_t m = (MAX_LIMB >> (LIMB_SIZE - limit)) & 255U;
        /* Find what multiple of f must be added to g to cancel its bottom min(limit, 8) bits. */
        limb_t w = (g * NEGINV256[(f >> 1) & 127]) & m;
        /* Do so. */
        g += f * w;
        q += u * w;
        r += v * w;
    }
    out.u = (signed_limb_t)u;
    out.v = (signed_limb_t)v;
    out.q = (signed_limb_t)q;
    out.r = (signed_limb_t)r;
    return eta;
}

/** Apply matrix t/2^SIGNED_LIMB_SIZE to vector [d,e], modulo modulus.
 *
 * On input and output, d and e are in range 1-2*modulus..modulus-1.
 */
inline void UpdateDE(Num3072Signed& d, Num3072Signed& e, const SignedMatrix& t)
{
    const signed_limb_t u = t.u, v=t.v, q=t.q, r=t.r;

    /* [md,me] start as zero; plus [u,q] if d is negative; plus [v,r] if e is negative. */
    signed_limb_t sd = d.limbs[SIGNED_LIMBS - 1] >> (LIMB_SIZE - 1);
    signed_limb_t se = e.limbs[SIGNED_LIMBS - 1] >> (LIMB_SIZE - 1);
    signed_limb_t md = (u & sd) + (v & se);
    signed_limb_t me = (q & sd) + (r & se);
    /* Begin computing t*[d,e]. */
    signed_limb_t di = d.limbs[0], ei = e.limbs[0];
    signed_double_limb_t cd = (signed_double_limb_t)u * di + (signed_double_limb_t)v * ei;
    signed_double_limb_t ce = (signed_double_limb_t)q * di + (signed_double_limb_t)r * ei;
    /* Correct md,me so that t*[d,e]+modulus*[md,me] has SIGNED_LIMB_SIZE zero bottom bits. */
    md -= (MODULUS_INVERSE * limb_t(cd) + md) & MAX_SIGNED_LIMB;
    me -= (MODULUS_INVERSE * limb_t(ce) + me) & MAX_SIGNED_LIMB;
    /* Update the beginning of computation for t*[d,e]+modulus*[md,me] now md,me are known. */
    cd -= (signed_double_limb_t)1103717 * md;
    ce -= (signed_double_limb_t)1103717 * me;
    /* Verify that the low SIGNED_LIMB_SIZE bits of the computation are indeed zero, and then throw them away. */
    Assume((cd & MAX_SIGNED_LIMB) == 0);
    Assume((ce & MAX_SIGNED_LIMB) == 0);
    cd >>= SIGNED_LIMB_SIZE;
    ce >>= SIGNED_LIMB_SIZE;
    /* Now iteratively compute limb i=1..SIGNED_LIMBS-2 of t*[d,e]+modulus*[md,me], and store them in output
     * limb i-1 (shifting down by SIGNED_LIMB_SIZE bits). The corresponding limbs in modulus are all zero,
     * so modulus/md/me are not actually involved here. */
    for (int i = 1; i < SIGNED_LIMBS - 1; ++i) {
        di = d.limbs[i];
        ei = e.limbs[i];
        cd += (signed_double_limb_t)u * di + (signed_double_limb_t)v * ei;
        ce += (signed_double_limb_t)q * di + (signed_double_limb_t)r * ei;
        d.limbs[i - 1] = (signed_limb_t)cd & MAX_SIGNED_LIMB; cd >>= SIGNED_LIMB_SIZE;
        e.limbs[i - 1] = (signed_limb_t)ce & MAX_SIGNED_LIMB; ce >>= SIGNED_LIMB_SIZE;
    }
    /* Compute limb SIGNED_LIMBS-1 of t*[d,e]+modulus*[md,me], and store it in output limb SIGNED_LIMBS-2. */
    di = d.limbs[SIGNED_LIMBS - 1];
    ei = e.limbs[SIGNED_LIMBS - 1];
    cd += (signed_double_limb_t)u * di + (signed_double_limb_t)v * ei;
    ce += (signed_double_limb_t)q * di + (signed_double_limb_t)r * ei;
    cd += (signed_double_limb_t)md << FINAL_LIMB_MODULUS_BITS;
    ce += (signed_double_limb_t)me << FINAL_LIMB_MODULUS_BITS;
    d.limbs[SIGNED_LIMBS - 2] = (signed_limb_t)cd & MAX_SIGNED_LIMB; cd >>= SIGNED_LIMB_SIZE;
    e.limbs[SIGNED_LIMBS - 2] = (signed_limb_t)ce & MAX_SIGNED_LIMB; ce >>= SIGNED_LIMB_SIZE;
    /* What remains goes into output limb SINGED_LIMBS-1 */
    d.limbs[SIGNED_LIMBS - 1] = (signed_limb_t)cd;
    e.limbs[SIGNED_LIMBS - 1] = (signed_limb_t)ce;
}

/** Apply matrix t/2^SIGNED_LIMB_SIZE to vector (f,g).
 *
 * The matrix t must be chosen such that t*(f,g) results in multiples of 2^SIGNED_LIMB_SIZE.
 * This is the case for matrices computed by ComputeDivstepMatrix().
 */
inline void UpdateFG(Num3072Signed& f, Num3072Signed& g, const SignedMatrix& t, int len)
{
    const signed_limb_t u = t.u, v=t.v, q=t.q, r=t.r;

    signed_limb_t fi, gi;
    signed_double_limb_t cf, cg;
    /* Start computing t*[f,g]. */
    fi = f.limbs[0];
    gi = g.limbs[0];
    cf = (signed_double_limb_t)u * fi + (signed_double_limb_t)v * gi;
    cg = (signed_double_limb_t)q * fi + (signed_double_limb_t)r * gi;
    /* Verify that the bottom SIGNED_LIMB_BITS bits of the result are zero, and then throw them away. */
    Assume((cf & MAX_SIGNED_LIMB) == 0);
    Assume((cg & MAX_SIGNED_LIMB) == 0);
    cf >>= SIGNED_LIMB_SIZE;
    cg >>= SIGNED_LIMB_SIZE;
    /* Now iteratively compute limb i=1..SIGNED_LIMBS-1 of t*[f,g], and store them in output limb i-1 (shifting
     * down by SIGNED_LIMB_BITS bits). */
    for (int i = 1; i < len; ++i) {
        fi = f.limbs[i];
        gi = g.limbs[i];
        cf += (signed_double_limb_t)u * fi + (signed_double_limb_t)v * gi;
        cg += (signed_double_limb_t)q * fi + (signed_double_limb_t)r * gi;
        f.limbs[i - 1] = (signed_limb_t)cf & MAX_SIGNED_LIMB; cf >>= SIGNED_LIMB_SIZE;
        g.limbs[i - 1] = (signed_limb_t)cg & MAX_SIGNED_LIMB; cg >>= SIGNED_LIMB_SIZE;
    }
    /* What remains is limb SIGNED_LIMBS of t*[f,g]; store it as output limb SIGNED_LIMBS-1. */
    f.limbs[len - 1] = (signed_limb_t)cf;
    g.limbs[len - 1] = (signed_limb_t)cg;

}
} // namespace

Num3072 Num3072::GetInverse() const
{
    // Compute a modular inverse based on a variant of the safegcd algorithm:
    // - Paper: https://gcd.cr.yp.to/papers.html
    // - Inspired by this code in libsecp256k1:
    //   https://github.com/bitcoin-core/secp256k1/blob/master/src/modinv32_impl.h
    // - Explanation of the algorithm:
    //   https://github.com/bitcoin-core/secp256k1/blob/master/doc/safegcd_implementation.md

    // Local variables d, e, f, g:
    // - f and g are the variables whose gcd we compute (despite knowing the answer is 1):
    //   - f is always odd, and initialized as modulus
    //   - g is initialized as *this (called x in what follows)
    // - d and e are the numbers for which at every step it is the case that:
    //   - f = d * x mod modulus; d is initialized as 0
    //   - g = e * x mod modulus; e is initialized as 1
    Num3072Signed d, e, f, g;
    e.limbs[0] = 1;
    // F is initialized as modulus, which in signed limb representation can be expressed
    // simply as 2^3072 + -MAX_PRIME_DIFF.
    f.limbs[0] = -MAX_PRIME_DIFF;
    f.limbs[FINAL_LIMB_POSITION] = ((limb_t)1) << FINAL_LIMB_MODULUS_BITS;
    g.FromNum3072(*this);
    int len = SIGNED_LIMBS; //!< The number of significant limbs in f and g
    signed_limb_t eta = -1; //!< State to track knowledge about ratio of f and g
    // Perform divsteps on [f,g] until g=0 is reached, keeping (d,e) synchronized with them.
    while (true) {
        // Compute transformation matrix t that represents the next SIGNED_LIMB_SIZE divsteps
        // to apply. This can be computed from just the bottom limb of f and g, and eta.
        SignedMatrix t;
        eta = ComputeDivstepMatrix(eta, f.limbs[0], g.limbs[0], t);
        // Apply that transformation matrix to the full [f,g] vector.
        UpdateFG(f, g, t, len);
        // Apply that transformation matrix to the full [d,e] vector (mod modulus).
        UpdateDE(d, e, t);

        // Check if g is zero.
        if (g.limbs[0] == 0) {
            signed_limb_t cond = 0;
            for (int j = 1; j < len; ++j) {
                cond |= g.limbs[j];
            }
            // If so, we're done.
            if (cond == 0) break;
        }

        // Check if the top limbs of both f and g are both 0 or -1.
        signed_limb_t fn = f.limbs[len - 1], gn = g.limbs[len - 1];
        signed_limb_t cond = ((signed_limb_t)len - 2) >> (LIMB_SIZE - 1);
        cond |= fn ^ (fn >> (LIMB_SIZE - 1));
        cond |= gn ^ (gn >> (LIMB_SIZE - 1));
        if (cond == 0) {
            // If so, drop the top limb, shrinking the size of f and g, by
            // propagating the sign to the previous limb.
            f.limbs[len - 2] |= (limb_t)f.limbs[len - 1] << SIGNED_LIMB_SIZE;
            g.limbs[len - 2] |= (limb_t)g.limbs[len - 1] << SIGNED_LIMB_SIZE;
            --len;
        }
    }
    // At some point, [f,g] will have been rewritten into [f',0], such that gcd(f,g) = gcd(f',0).
    // This is proven in the paper. As f started out being modulus, a prime number, we know that
    // gcd is 1, and thus f' is 1 or -1.
    Assume((f.limbs[0] & MAX_SIGNED_LIMB) == 1 || (f.limbs[0] & MAX_SIGNED_LIMB) == MAX_SIGNED_LIMB);
    // As we've maintained the invariant that f = d * x mod modulus, we get d/f mod modulus is the
    // modular inverse of x we're looking for. As f is 1 or -1, it is also true that d/f = d*f.
    // Normalize d to prepare it for output, while negating it if f is negative.
    d.Normalize(f.limbs[len - 1] >> (LIMB_SIZE  - 1));
    Num3072 ret;
    d.ToNum3072(ret);
    return ret;
}

void Num3072::Multiply(const Num3072& a)
{
    limb_t c0 = 0, c1 = 0, c2 = 0;
    Num3072 tmp;

    /* Compute limbs 0..N-2 of this*a into tmp, including one reduction. */
    for (int j = 0; j < LIMBS - 1; ++j) {
        limb_t d0 = 0, d1 = 0, d2 = 0;
        mul(d0, d1, this->limbs[1 + j], a.limbs[LIMBS + j - (1 + j)]);
        for (int i = 2 + j; i < LIMBS; ++i) muladd3(d0, d1, d2, this->limbs[i], a.limbs[LIMBS + j - i]);
        mulnadd3(c0, c1, c2, d0, d1, d2, MAX_PRIME_DIFF);
        for (int i = 0; i < j + 1; ++i) muladd3(c0, c1, c2, this->limbs[i], a.limbs[j - i]);
        extract3(c0, c1, c2, tmp.limbs[j]);
    }

    /* Compute limb N-1 of a*b into tmp. */
    assert(c2 == 0);
    for (int i = 0; i < LIMBS; ++i) muladd3(c0, c1, c2, this->limbs[i], a.limbs[LIMBS - 1 - i]);
    extract3(c0, c1, c2, tmp.limbs[LIMBS - 1]);

    /* Perform a second reduction. */
    muln2(c0, c1, MAX_PRIME_DIFF);
    for (int j = 0; j < LIMBS; ++j) {
        addnextract2(c0, c1, tmp.limbs[j], this->limbs[j]);
    }

    assert(c1 == 0);
    assert(c0 == 0 || c0 == 1);

    /* Perform up to two more reductions if the internal state has already
     * overflown the MAX of Num3072 or if it is larger than the modulus or
     * if both are the case.
     * */
    if (this->IsOverflow()) this->FullReduce();
    if (c0) this->FullReduce();
}

void Num3072::SetToOne()
{
    this->limbs[0] = 1;
    for (int i = 1; i < LIMBS; ++i) this->limbs[i] = 0;
}

void Num3072::Divide(const Num3072& a)
{
    if (this->IsOverflow()) this->FullReduce();

    Num3072 inv{};
    if (a.IsOverflow()) {
        Num3072 b = a;
        b.FullReduce();
        inv = b.GetInverse();
    } else {
        inv = a.GetInverse();
    }

    this->Multiply(inv);
    if (this->IsOverflow()) this->FullReduce();
}

Num3072::Num3072(const unsigned char (&data)[BYTE_SIZE]) {
    for (int i = 0; i < LIMBS; ++i) {
        if (sizeof(limb_t) == 4) {
            this->limbs[i] = ReadLE32(data + 4 * i);
        } else if (sizeof(limb_t) == 8) {
            this->limbs[i] = ReadLE64(data + 8 * i);
        }
    }
}

void Num3072::ToBytes(unsigned char (&out)[BYTE_SIZE]) {
    for (int i = 0; i < LIMBS; ++i) {
        if (sizeof(limb_t) == 4) {
            WriteLE32(out + i * 4, this->limbs[i]);
        } else if (sizeof(limb_t) == 8) {
            WriteLE64(out + i * 8, this->limbs[i]);
        }
    }
}

Num3072 MuHash3072::ToNum3072(std::span<const unsigned char> in) {
    unsigned char tmp[Num3072::BYTE_SIZE];

    uint256 hashed_in{(HashWriter{} << in).GetSHA256()};
    static_assert(sizeof(tmp) % ChaCha20Aligned::BLOCKLEN == 0);
    ChaCha20Aligned{MakeByteSpan(hashed_in)}.Keystream(MakeWritableByteSpan(tmp));
    Num3072 out{tmp};

    return out;
}

MuHash3072::MuHash3072(std::span<const unsigned char> in) noexcept
{
    m_numerator = ToNum3072(in);
}

void MuHash3072::Finalize(uint256& out) noexcept
{
    m_numerator.Divide(m_denominator);
    m_denominator.SetToOne();  // Needed to keep the MuHash object valid

    unsigned char data[Num3072::BYTE_SIZE];
    m_numerator.ToBytes(data);

    out = (HashWriter{} << data).GetSHA256();
}

MuHash3072& MuHash3072::operator*=(const MuHash3072& mul) noexcept
{
    m_numerator.Multiply(mul.m_numerator);
    m_denominator.Multiply(mul.m_denominator);
    return *this;
}

MuHash3072& MuHash3072::operator/=(const MuHash3072& div) noexcept
{
    m_numerator.Multiply(div.m_denominator);
    m_denominator.Multiply(div.m_numerator);
    return *this;
}

MuHash3072& MuHash3072::Insert(std::span<const unsigned char> in) noexcept {
    m_numerator.Multiply(ToNum3072(in));
    return *this;
}

MuHash3072& MuHash3072::Remove(std::span<const unsigned char> in) noexcept {
    m_denominator.Multiply(ToNum3072(in));
    return *this;
}
