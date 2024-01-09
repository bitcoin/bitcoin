// Copyright (c) 2017-2022 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <crypto/muhash.h>

#include <crypto/chacha20.h>
#include <crypto/common.h>
#include <hash.h>

#include <cassert>
#include <cstdio>
#include <limits>

namespace {

using limb_t = Num3072::limb_t;
using double_limb_t = Num3072::double_limb_t;
constexpr int LIMB_SIZE = Num3072::LIMB_SIZE;
/** 2^3072 - 1103717, the largest 3072-bit safe prime number, is used as the modulus. */
constexpr limb_t MAX_PRIME_DIFF = 1103717;

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

/** [c0,c1,c2] += 2 * a * b */
inline void muldbladd3(limb_t& c0, limb_t& c1, limb_t& c2, const limb_t& a, const limb_t& b)
{
    double_limb_t t = (double_limb_t)a * b;
    limb_t th = t >> LIMB_SIZE;
    limb_t tl = t;

    c0 += tl;
    limb_t tt = th + ((c0 < tl) ? 1 : 0);
    c1 += tt;
    c2 += (c1 < tt) ? 1 : 0;
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
        if (c1 == 0)
            c2 = 1;
    }

    // extract
    n = c0;
    c0 = c1;
    c1 = c2;
}

/** in_out = in_out^(2^sq) * mul */
inline void square_n_mul(Num3072& in_out, const int sq, const Num3072& mul)
{
    for (int j = 0; j < sq; ++j) in_out.Square();
    in_out.Multiply(mul);
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

Num3072 Num3072::GetInverse() const
{
    // For fast exponentiation a sliding window exponentiation with repunit
    // precomputation is utilized. See "Fast Point Decompression for Standard
    // Elliptic Curves" (Brumley, Järvinen, 2008).

    Num3072 p[12]; // p[i] = a^(2^(2^i)-1)
    Num3072 out;

    p[0] = *this;

    for (int i = 0; i < 11; ++i) {
        p[i + 1] = p[i];
        for (int j = 0; j < (1 << i); ++j) p[i + 1].Square();
        p[i + 1].Multiply(p[i]);
    }

    out = p[11];

    square_n_mul(out, 512, p[9]);
    square_n_mul(out, 256, p[8]);
    square_n_mul(out, 128, p[7]);
    square_n_mul(out, 64, p[6]);
    square_n_mul(out, 32, p[5]);
    square_n_mul(out, 8, p[3]);
    square_n_mul(out, 2, p[1]);
    square_n_mul(out, 1, p[0]);
    square_n_mul(out, 5, p[2]);
    square_n_mul(out, 3, p[0]);
    square_n_mul(out, 2, p[0]);
    square_n_mul(out, 4, p[0]);
    square_n_mul(out, 4, p[1]);
    square_n_mul(out, 3, p[0]);

    return out;
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

void Num3072::Square()
{
    limb_t c0 = 0, c1 = 0, c2 = 0;
    Num3072 tmp;

    /* Compute limbs 0..N-2 of this*this into tmp, including one reduction. */
    for (int j = 0; j < LIMBS - 1; ++j) {
        limb_t d0 = 0, d1 = 0, d2 = 0;
        for (int i = 0; i < (LIMBS - 1 - j) / 2; ++i) muldbladd3(d0, d1, d2, this->limbs[i + j + 1], this->limbs[LIMBS - 1 - i]);
        if ((j + 1) & 1) muladd3(d0, d1, d2, this->limbs[(LIMBS - 1 - j) / 2 + j + 1], this->limbs[LIMBS - 1 - (LIMBS - 1 - j) / 2]);
        mulnadd3(c0, c1, c2, d0, d1, d2, MAX_PRIME_DIFF);
        for (int i = 0; i < (j + 1) / 2; ++i) muldbladd3(c0, c1, c2, this->limbs[i], this->limbs[j - i]);
        if ((j + 1) & 1) muladd3(c0, c1, c2, this->limbs[(j + 1) / 2], this->limbs[j - (j + 1) / 2]);
        extract3(c0, c1, c2, tmp.limbs[j]);
    }

    assert(c2 == 0);
    for (int i = 0; i < LIMBS / 2; ++i) muldbladd3(c0, c1, c2, this->limbs[i], this->limbs[LIMBS - 1 - i]);
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

Num3072 MuHash3072::ToNum3072(Span<const unsigned char> in) {
    unsigned char tmp[Num3072::BYTE_SIZE];

    uint256 hashed_in{(HashWriter{} << in).GetSHA256()};
    static_assert(sizeof(tmp) % ChaCha20Aligned::BLOCKLEN == 0);
    ChaCha20Aligned{MakeByteSpan(hashed_in)}.Keystream(MakeWritableByteSpan(tmp));
    Num3072 out{tmp};

    return out;
}

MuHash3072::MuHash3072(Span<const unsigned char> in) noexcept
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

MuHash3072& MuHash3072::Insert(Span<const unsigned char> in) noexcept {
    m_numerator.Multiply(ToNum3072(in));
    return *this;
}

MuHash3072& MuHash3072::Remove(Span<const unsigned char> in) noexcept {
    m_denominator.Multiply(ToNum3072(in));
    return *this;
}
