// Copyright (c) 2017 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "muhash.h"

#include <limits>
#include "common.h"
#include "chacha20.h"

#include <assert.h>
#include <stdio.h>

namespace {

/** Extract the lowest limb of [c0,c1,c2] into n, and left shift the number by 1 limb. */
#define extract3(c0,c1,c2,n) { \
    (n) = c0; \
    c0 = c1; \
    c1 = c2; \
    c2 = 0; \
}

/** Extract the lowest limb of [c0,c1] into n, and left shift the number by 1 limb. */
#define extract2(c0,c1,n) { \
    (n) = c0; \
    c0 = c1; \
    c1 = 0; \
}

/** [c0,c1] = a * b */
#define mul(c0,c1,a,b) { \
    Num3072::double_limb_type t = (Num3072::double_limb_type)a * b; \
    c2 = 0; \
    c1 = t >> Num3072::LIMB_SIZE; \
    c0 = t; \
}

/* [c0,c1,c2] += n * [d0,d1,d2]. c2 is 0 initially */
#define mulnadd3(c0,c1,c2,d0,d1,d2,n) { \
    Num3072::double_limb_type t = (Num3072::double_limb_type)d0 * n + c0; \
    c0 = t; \
    t >>= Num3072::LIMB_SIZE; \
    t += (Num3072::double_limb_type)d1 * n + c1; \
    c1 = t; \
    t >>= Num3072::LIMB_SIZE; \
    c2 = t + d2 * n; \
}

/* [c0,c1] *= n */
#define muln2(c0,c1,n) { \
    Num3072::double_limb_type t = (Num3072::double_limb_type)c0 * n; \
    c0 = t; \
    t >>= Num3072::LIMB_SIZE; \
    t += (Num3072::double_limb_type)c1 * n; \
    c1 = t; \
    t >>= Num3072::LIMB_SIZE; \
}

/** [c0,c1,c2] += a * b */
#define muladd3(c0,c1,c2,a,b) { \
    Num3072::limb_type tl, th; \
    { \
        Num3072::double_limb_type t = (Num3072::double_limb_type)a * b; \
        th = t >> Num3072::LIMB_SIZE; \
        tl = t; \
    } \
    c0 += tl; \
    th += (c0 < tl) ? 1 : 0; \
    c1 += th; \
    c2 += (c1 < th) ? 1 : 0; \
}

/** [c0,c1,c2] += 2 * a * b */
#define muldbladd3(c0,c1,c2,a,b) { \
    Num3072::limb_type tl, th; \
    { \
        Num3072::double_limb_type t = (Num3072::double_limb_type)a * b; \
        th = t >> Num3072::LIMB_SIZE; \
        tl = t; \
    } \
    c0 += tl; \
    Num3072::limb_type tt = th + ((c0 < tl) ? 1 : 0); \
    c1 += tt; \
    c2 += (c1 < tt) ? 1 : 0; \
    c0 += tl; \
    th += (c0 < tl) ? 1 : 0; \
    c1 += th; \
    c2 += (c1 < th) ? 1 : 0; \
}

/** [c0,c1] += a */
#define add2(c0,c1,a) { \
    c0 += (a); \
    c1 += (c0 < (a)) ? 1 : 0; \
}

bool IsOverflow(const Num3072* d)
{
    for (int i = 1; i < Num3072::LIMBS - 1; ++i) {
        if (d->limbs[i] != std::numeric_limits<Num3072::limb_type>::max()) return false;
    }
    if (d->limbs[0] <= std::numeric_limits<Num3072::limb_type>::max() - 1103717) return false;
    return true;
}

void FullReduce(Num3072* d)
{
    Num3072::limb_type c0 = 1103717;
    for (int i = 0; i < Num3072::LIMBS; ++i) {
        Num3072::limb_type c1 = 0;
        add2(c0, c1, d->limbs[i]);
        extract2(c0, c1, d->limbs[i]);
    }
}

void Multiply(Num3072* out, const Num3072* a, const Num3072* b)
{
    Num3072::limb_type c0 = 0, c1 = 0;
    Num3072 tmp;

    /* Compute limbs 0..N-2 of a*b into tmp, including one reduction. */
    for (int j = 0; j < Num3072::LIMBS - 1; ++j) {
        Num3072::limb_type d0 = 0, d1 = 0, d2 = 0, c2 = 0;
        mul(d0, d1, a->limbs[1 + j], b->limbs[Num3072::LIMBS + j - (1 + j)]);
        for (int i = 2 + j; i < Num3072::LIMBS; ++i) muladd3(d0, d1, d2, a->limbs[i], b->limbs[Num3072::LIMBS + j - i]);
        mulnadd3(c0, c1, c2, d0, d1, d2, 1103717);
        for (int i = 0; i < j + 1; ++i) muladd3(c0, c1, c2, a->limbs[i], b->limbs[j - i]);
        extract3(c0, c1, c2, tmp.limbs[j]);
    }
    /* Compute limb N-1 of a*b into tmp. */
    {
        Num3072::limb_type c2 = 0;
        for (int i = 0; i < Num3072::LIMBS; ++i) muladd3(c0, c1, c2, a->limbs[i], b->limbs[Num3072::LIMBS - 1 - i]);
        extract3(c0, c1, c2, tmp.limbs[Num3072::LIMBS - 1]);
    }
    /* Perform a second reduction. */
    muln2(c0, c1, 1103717);
    for (int j = 0; j < Num3072::LIMBS; ++j) {
        add2(c0, c1, tmp.limbs[j]);
        extract2(c0, c1, out->limbs[j]);
    }
    assert(c1 == 0);
    assert(c0 == 0 || c0 == 1);
    /* Perform a potential third reduction. */
    if (c0) FullReduce(out);
}

void Square(Num3072* out, const Num3072* a)
{
    Num3072::limb_type c0 = 0, c1 = 0;
    Num3072 tmp;

    /* Compute limbs 0..N-2 of a*a into tmp, including one reduction. */
    for (int j = 0; j < Num3072::LIMBS - 1; ++j) {
        Num3072::limb_type d0 = 0, d1 = 0, d2 = 0, c2 = 0;
        for (int i = 0; i < (Num3072::LIMBS - 1 - j) / 2; ++i) muldbladd3(d0, d1, d2, a->limbs[i + j + 1], a->limbs[Num3072::LIMBS - 1 - i]);
        if ((j + 1) & 1) muladd3(d0, d1, d2, a->limbs[(Num3072::LIMBS - 1 - j) / 2 + j + 1], a->limbs[Num3072::LIMBS - 1 - (Num3072::LIMBS - 1 - j) / 2]);
        mulnadd3(c0, c1, c2, d0, d1, d2, 1103717);
        for (int i = 0; i < (j + 1) / 2; ++i) muldbladd3(c0, c1, c2, a->limbs[i], a->limbs[j - i]);
        if ((j + 1) & 1) muladd3(c0, c1, c2, a->limbs[(j + 1) / 2], a->limbs[j - (j + 1) / 2]);
        extract3(c0, c1, c2, tmp.limbs[j]);
    }
    {
        Num3072::limb_type c2 = 0;
        for (int i = 0; i < Num3072::LIMBS / 2; ++i) muldbladd3(c0, c1, c2, a->limbs[i], a->limbs[Num3072::LIMBS - 1 - i]);
        extract3(c0, c1, c2, tmp.limbs[Num3072::LIMBS - 1]);
    }
    /* Perform a second reduction. */
    muln2(c0, c1, 1103717);
    for (int j = 0; j < Num3072::LIMBS; ++j) {
        add2(c0, c1, tmp.limbs[j]);
        extract2(c0, c1, out->limbs[j]);
    }
    assert(c1 == 0);
    assert(c0 == 0 || c0 == 1);
    /* Perform a potential third reduction. */
    if (c0) FullReduce(out);
}

void Inverse(Num3072* out, const Num3072* a)
{
    Num3072 p[12]; // p[i] = a^(2^(2^i)-1)
    Num3072 x;

    p[0] = *a;

    for (int i = 0; i < 11; ++i) {
        p[i + 1] = p[i];
        for (int j = 0; j < (1 << i); ++j) Square(&p[i + 1], &p[i + 1]);
        Multiply(&p[i + 1], &p[i + 1], &p[i]);
    }

    x = p[11];

    for (int j = 0; j < 512; ++j) Square(&x, &x);
    Multiply(&x, &x, &p[9]);
    for (int j = 0; j < 256; ++j) Square(&x, &x);
    Multiply(&x, &x, &p[8]);
    for (int j = 0; j < 128; ++j) Square(&x, &x);
    Multiply(&x, &x, &p[7]);
    for (int j = 0; j < 64; ++j) Square(&x, &x);
    Multiply(&x, &x, &p[6]);
    for (int j = 0; j < 32; ++j) Square(&x, &x);
    Multiply(&x, &x, &p[5]);
    for (int j = 0; j < 8; ++j) Square(&x, &x);
    Multiply(&x, &x, &p[3]);
    for (int j = 0; j < 2; ++j) Square(&x, &x);
    Multiply(&x, &x, &p[1]);
    for (int j = 0; j < 1; ++j) Square(&x, &x);
    Multiply(&x, &x, &p[0]);
    for (int j = 0; j < 5; ++j) Square(&x, &x);
    Multiply(&x, &x, &p[2]);
    for (int j = 0; j < 3; ++j) Square(&x, &x);
    Multiply(&x, &x, &p[0]);
    for (int j = 0; j < 2; ++j) Square(&x, &x);
    Multiply(&x, &x, &p[0]);
    for (int j = 0; j < 4; ++j) Square(&x, &x);
    Multiply(&x, &x, &p[0]);
    for (int j = 0; j < 4; ++j) Square(&x, &x);
    Multiply(&x, &x, &p[1]);
    for (int j = 0; j < 3; ++j) Square(&x, &x);
    Multiply(&x, &x, &p[0]);

    *out = x;
}

}

MuHash3072::MuHash3072() noexcept
{
    data.limbs[0] = 1;
    for (int i = 1; i < Num3072::LIMBS; ++i) data.limbs[i] = 0;
}

MuHash3072::MuHash3072(const unsigned char* key32) noexcept
{
    unsigned char tmp[384];
    ChaCha20(key32, 32).Output(tmp, 384);
    for (int i = 0; i < Num3072::LIMBS; ++i) {
        if (sizeof(Num3072::limb_type) == 4) {
            data.limbs[i] = ReadLE32(tmp + 4 * i);
        } else if (sizeof(Num3072::limb_type) == 8) {
            data.limbs[i] = ReadLE64(tmp + 8 * i);
        }
    }
}

void MuHash3072::Finalize(unsigned char* hash384) noexcept
{
    if (IsOverflow(&data)) FullReduce(&data);
    for (int i = 0; i < Num3072::LIMBS; ++i) {
        if (sizeof(Num3072::limb_type) == 4) {
            WriteLE32(hash384 + i * 4, data.limbs[i]);
        } else if (sizeof(Num3072::limb_type) == 8) {
            WriteLE64(hash384 + i * 8, data.limbs[i]);
        }
    }
}

MuHash3072& MuHash3072::operator*=(const MuHash3072& x) noexcept
{
    Multiply(&this->data, &this->data, &x.data);
    return *this;
}

MuHash3072& MuHash3072::operator/=(const MuHash3072& x) noexcept
{
    Num3072 tmp;
    Inverse(&tmp, &x.data);
    Multiply(&this->data, &this->data, &tmp);
    return *this;
}
