/***********************************************************************
 * Copyright (c) Pieter Wuille, Gregory Maxwell, Peter Dettman         *
 * Distributed under the MIT software license, see the accompanying    *
 * file COPYING or https://www.opensource.org/licenses/mit-license.php.*
 ***********************************************************************/

#ifndef SECP256K1_ECMULT_GEN_IMPL_H
#define SECP256K1_ECMULT_GEN_IMPL_H

#include "util.h"
#include "scalar.h"
#include "group.h"
#include "ecmult_gen.h"
#include "hash_impl.h"
#include "precomputed_ecmult_gen.h"

static void secp256k1_ecmult_gen_context_build(secp256k1_ecmult_gen_context *ctx) {
    secp256k1_ecmult_gen_blind(ctx, NULL);
    ctx->built = 1;
}

static int secp256k1_ecmult_gen_context_is_built(const secp256k1_ecmult_gen_context* ctx) {
    return ctx->built;
}

static void secp256k1_ecmult_gen_context_clear(secp256k1_ecmult_gen_context *ctx) {
    ctx->built = 0;
    secp256k1_scalar_clear(&ctx->scalar_offset);
    secp256k1_ge_clear(&ctx->ge_offset);
    secp256k1_fe_clear(&ctx->proj_blind);
}

/* Compute the scalar (2^COMB_BITS - 1) / 2, the difference between the gn argument to
 * secp256k1_ecmult_gen, and the scalar whose encoding the table lookup bits are drawn
 * from (before applying blinding). */
static void secp256k1_ecmult_gen_scalar_diff(secp256k1_scalar* diff) {
    int i;

    /* Compute scalar -1/2. */
    secp256k1_scalar neghalf;
    secp256k1_scalar_half(&neghalf, &secp256k1_scalar_one);
    secp256k1_scalar_negate(&neghalf, &neghalf);

    /* Compute offset = 2^(COMB_BITS - 1). */
    *diff = secp256k1_scalar_one;
    for (i = 0; i < COMB_BITS - 1; ++i) {
        secp256k1_scalar_add(diff, diff, diff);
    }

    /* The result is the sum 2^(COMB_BITS - 1) + (-1/2). */
    secp256k1_scalar_add(diff, diff, &neghalf);
}

static void secp256k1_ecmult_gen(const secp256k1_ecmult_gen_context *ctx, secp256k1_gej *r, const secp256k1_scalar *gn) {
    uint32_t comb_off;
    secp256k1_ge add;
    secp256k1_fe neg;
    secp256k1_ge_storage adds;
    secp256k1_scalar d;
    /* Array of uint32_t values large enough to store COMB_BITS bits. Only the bottom
     * 8 are ever nonzero, but having the zero padding at the end if COMB_BITS>256
     * avoids the need to deal with out-of-bounds reads from a scalar. */
    uint32_t recoded[(COMB_BITS + 31) >> 5] = {0};
    int first = 1, i;

    memset(&adds, 0, sizeof(adds));

    /* We want to compute R = gn*G.
     *
     * To blind the scalar used in the computation, we rewrite this to be
     * R = (gn - b)*G + b*G, with a blinding value b determined by the context.
     *
     * The multiplication (gn-b)*G will be performed using a signed-digit multi-comb (see Section
     * 3.3 of "Fast and compact elliptic-curve cryptography" by Mike Hamburg,
     * https://eprint.iacr.org/2012/309).
     *
     * Let comb(s, P) = sum((2*s[i]-1)*2^i*P for i=0..COMB_BITS-1), where s[i] is the i'th bit of
     * the binary representation of scalar s. So the s[i] values determine whether -2^i*P (s[i]=0)
     * or +2^i*P (s[i]=1) are added together. COMB_BITS is at least 256, so all bits of s are
     * covered. By manipulating:
     *
     *     comb(s, P) = sum((2*s[i]-1)*2^i*P for i=0..COMB_BITS-1)
     * <=> comb(s, P) = sum((2*s[i]-1)*2^i for i=0..COMB_BITS-1) * P
     * <=> comb(s, P) = (2*sum(s[i]*2^i for i=0..COMB_BITS-1) - sum(2^i for i=0..COMB_BITS-1)) * P
     * <=> comb(s, P) = (2*s - (2^COMB_BITS - 1)) * P
     *
     * If we wanted to compute (gn-b)*G as comb(s, G), it would need to hold that
     *
     *     (gn - b) * G = (2*s - (2^COMB_BITS - 1)) * G
     * <=> s = (gn - b + (2^COMB_BITS - 1))/2 (mod order)
     *
     * We use an alternative here that avoids the modular division by two: instead we compute
     * (gn-b)*G as comb(d, G/2). For that to hold it must be the case that
     *
     *     (gn - b) * G = (2*d - (2^COMB_BITS - 1)) * (G/2)
     * <=> d = gn - b + (2^COMB_BITS - 1)/2 (mod order)
     *
     * Adding precomputation, our final equations become:
     *
     *     ctx->scalar_offset = (2^COMB_BITS - 1)/2 - b (mod order)
     *     ctx->ge_offset = b*G
     *     d = gn + ctx->scalar_offset (mod order)
     *     R = comb(d, G/2) + ctx->ge_offset
     *
     * comb(d, G/2) function is then computed by summing + or - 2^(i-1)*G, for i=0..COMB_BITS-1,
     * depending on the value of the bits d[i] of the binary representation of scalar d.
     */

    /* Compute the scalar d = (gn + ctx->scalar_offset). */
    secp256k1_scalar_add(&d, &ctx->scalar_offset, gn);
    /* Convert to recoded array. */
    for (i = 0; i < 8 && i < ((COMB_BITS + 31) >> 5); ++i) {
        recoded[i] = secp256k1_scalar_get_bits_limb32(&d, 32 * i, 32);
    }
    secp256k1_scalar_clear(&d);

    /* In secp256k1_ecmult_gen_prec_table we have precomputed sums of the
     * (2*d[i]-1) * 2^(i-1) * G points, for various combinations of i positions.
     * We rewrite our equation in terms of these table entries.
     *
     * Let mask(b) = sum(2^((b*COMB_TEETH + t)*COMB_SPACING) for t=0..COMB_TEETH-1),
     * with b ranging from 0 to COMB_BLOCKS-1. So for example with COMB_BLOCKS=11,
     * COMB_TEETH=6, COMB_SPACING=4, we would have:
     *   mask(0)  = 2^0   + 2^4   + 2^8   + 2^12  + 2^16  + 2^20,
     *   mask(1)  = 2^24  + 2^28  + 2^32  + 2^36  + 2^40  + 2^44,
     *   mask(2)  = 2^48  + 2^52  + 2^56  + 2^60  + 2^64  + 2^68,
     *   ...
     *   mask(10) = 2^240 + 2^244 + 2^248 + 2^252 + 2^256 + 2^260
     *
     * We will split up the bits d[i] using these masks. Specifically, each mask is
     * used COMB_SPACING times, with different shifts:
     *
     * d = (d & mask(0)<<0) + (d & mask(1)<<0) + ... + (d & mask(COMB_BLOCKS-1)<<0) +
     *     (d & mask(0)<<1) + (d & mask(1)<<1) + ... + (d & mask(COMB_BLOCKS-1)<<1) +
     *     ...
     *     (d & mask(0)<<(COMB_SPACING-1)) + ...
     *
     * Now define table(b, m) = (m - mask(b)/2) * G, and we will precompute these values for
     * b=0..COMB_BLOCKS-1, and for all values m which (d & mask(b)) can take (so m can take on
     * 2^COMB_TEETH distinct values).
     *
     * If m=(d & mask(b)), then table(b, m) is the sum of 2^i * (2*d[i]-1) * G/2, with i
     * iterating over the set bits in mask(b). In our example, table(2, 2^48 + 2^56 + 2^68)
     * would equal (2^48 - 2^52 + 2^56 - 2^60 - 2^64 + 2^68) * G/2.
     *
     * With that, we can rewrite comb(d, G/2) as:
     *
     *     2^0 * (table(0, d>>0 & mask(0)) + ... + table(COMB_BLOCKS-1, d>>0 & mask(COMP_BLOCKS-1)))
     *   + 2^1 * (table(0, d>>1 & mask(0)) + ... + table(COMB_BLOCKS-1, d>>1 & mask(COMP_BLOCKS-1)))
     *   + 2^2 * (table(0, d>>2 & mask(0)) + ... + table(COMB_BLOCKS-1, d>>2 & mask(COMP_BLOCKS-1)))
     *   + ...
     *   + 2^(COMB_SPACING-1) * (table(0, d>>(COMB_SPACING-1) & mask(0)) + ...)
     *
     * Or more generically as
     *
     *   sum(2^i * sum(table(b, d>>i & mask(b)), b=0..COMB_BLOCKS-1), i=0..COMB_SPACING-1)
     *
     * This is implemented using an outer loop that runs in reverse order over the lines of this
     * equation, which in each iteration runs an inner loop that adds the terms of that line and
     * then doubles the result before proceeding to the next line.
     *
     * In pseudocode:
     *   c = infinity
     *   for comb_off in range(COMB_SPACING - 1, -1, -1):
     *     for block in range(COMB_BLOCKS):
     *       c += table(block, (d >> comb_off) & mask(block))
     *     if comb_off > 0:
     *       c = 2*c
     *   return c
     *
     * This computes c = comb(d, G/2), and thus finally R = c + ctx->ge_offset. Note that it would
     * be possible to apply an initial offset instead of a final offset (moving ge_offset to take
     * the place of infinity above), but the chosen approach allows using (in a future improvement)
     * an incomplete addition formula for most of the multiplication.
     *
     * The last question is how to implement the table(b, m) function. For any value of b,
     * m=(d & mask(b)) can only take on at most 2^COMB_TEETH possible values (the last one may have
     * fewer as there mask(b) may exceed the curve order). So we could create COMB_BLOCK tables
     * which contain a value for each such m value.
     *
     * Now note that if m=(d & mask(b)), then flipping the relevant bits of m results in negating
     * the result of table(b, m). This is because table(b,m XOR mask(b)) = table(b, mask(b) - m) =
     * (mask(b) - m - mask(b)/2)*G = (-m + mask(b)/2)*G = -(m - mask(b)/2)*G = -table(b, m).
     * Because of this it suffices to only store the first half of the m values for every b. If an
     * entry from the second half is needed, we look up its bit-flipped version instead, and negate
     * it.
     *
     * secp256k1_ecmult_gen_prec_table[b][index] stores the table(b, m) entries. Index
     * is the relevant mask(b) bits of m packed together without gaps. */

    /* Outer loop: iterate over comb_off from COMB_SPACING - 1 down to 0. */
    comb_off = COMB_SPACING - 1;
    while (1) {
        uint32_t block;
        uint32_t bit_pos = comb_off;
        /* Inner loop: for each block, add table entries to the result. */
        for (block = 0; block < COMB_BLOCKS; ++block) {
            /* Gather the mask(block)-selected bits of d into bits. They're packed:
             * bits[tooth] = d[(block*COMB_TEETH + tooth)*COMB_SPACING + comb_off]. */
            uint32_t bits = 0, sign, abs, index, tooth;
            /* Instead of reading individual bits here to construct the bits variable,
             * build up the result by xoring rotated reads together. In every iteration,
             * one additional bit is made correct, starting at the bottom. The bits
             * above that contain junk. This reduces leakage by avoiding computations
             * on variables that can have only a low number of possible values (e.g.,
             * just two values when reading a single bit into a variable.) See:
             * https://www.usenix.org/system/files/conference/usenixsecurity18/sec18-alam.pdf
             */
            for (tooth = 0; tooth < COMB_TEETH; ++tooth) {
                /* Construct bitdata s.t. the bottom bit is the bit we'd like to read.
                 *
                 * We could just set bitdata = recoded[bit_pos >> 5] >> (bit_pos & 0x1f)
                 * but this would simply discard the bits that fall off at the bottom,
                 * and thus, for example, bitdata could still have only two values if we
                 * happen to shift by exactly 31 positions. We use a rotation instead,
                 * which ensures that bitdata doesn't lose entropy. This relies on the
                 * rotation being atomic, i.e., the compiler emitting an actual rot
                 * instruction. */
                uint32_t bitdata = secp256k1_rotr32(recoded[bit_pos >> 5], bit_pos & 0x1f);

                /* Clear the bit at position tooth, but sssh, don't tell clang. */
                uint32_t volatile vmask = ~(1 << tooth);
                bits &= vmask;

                /* Write the bit into position tooth (and junk into higher bits). */
                bits ^= bitdata << tooth;
                bit_pos += COMB_SPACING;
            }

            /* If the top bit of bits is 1, flip them all (corresponding to looking up
             * the negated table value), and remember to negate the result in sign. */
            sign = (bits >> (COMB_TEETH - 1)) & 1;
            abs = (bits ^ -sign) & (COMB_POINTS - 1);
            VERIFY_CHECK(sign == 0 || sign == 1);
            VERIFY_CHECK(abs < COMB_POINTS);

            /** This uses a conditional move to avoid any secret data in array indexes.
             *   _Any_ use of secret indexes has been demonstrated to result in timing
             *   sidechannels, even when the cache-line access patterns are uniform.
             *  See also:
             *   "A word of warning", CHES 2013 Rump Session, by Daniel J. Bernstein and Peter Schwabe
             *    (https://cryptojedi.org/peter/data/chesrump-20130822.pdf) and
             *   "Cache Attacks and Countermeasures: the Case of AES", RSA 2006,
             *    by Dag Arne Osvik, Adi Shamir, and Eran Tromer
             *    (https://eprint.iacr.org/2005/271.pdf)
             */
            for (index = 0; index < COMB_POINTS; ++index) {
                secp256k1_ge_storage_cmov(&adds, &secp256k1_ecmult_gen_prec_table[block][index], index == abs);
            }

            /* Set add=adds or add=-adds, in constant time, based on sign. */
            secp256k1_ge_from_storage(&add, &adds);
            secp256k1_fe_negate(&neg, &add.y, 1);
            secp256k1_fe_cmov(&add.y, &neg, sign);

            /* Add the looked up and conditionally negated value to r. */
            if (EXPECT(first, 0)) {
                /* If this is the first table lookup, we can skip addition. */
                secp256k1_gej_set_ge(r, &add);
                /* Give the entry a random Z coordinate to blind intermediary results. */
                secp256k1_gej_rescale(r, &ctx->proj_blind);
                first = 0;
            } else {
                secp256k1_gej_add_ge(r, r, &add);
            }
        }

        /* Double the result, except in the last iteration. */
        if (comb_off-- == 0) break;
        secp256k1_gej_double(r, r);
    }

    /* Correct for the scalar_offset added at the start (ge_offset = b*G, while b was
     * subtracted from the input scalar gn). */
    secp256k1_gej_add_ge(r, r, &ctx->ge_offset);

    /* Cleanup. */
    secp256k1_fe_clear(&neg);
    secp256k1_ge_clear(&add);
    secp256k1_memclear_explicit(&adds, sizeof(adds));
    secp256k1_memclear_explicit(&recoded, sizeof(recoded));
}

/* Setup blinding values for secp256k1_ecmult_gen. */
static void secp256k1_ecmult_gen_blind(secp256k1_ecmult_gen_context *ctx, const unsigned char *seed32) {
    secp256k1_scalar b;
    secp256k1_scalar diff;
    secp256k1_gej gb;
    secp256k1_fe f;
    unsigned char nonce32[32];
    secp256k1_rfc6979_hmac_sha256 rng;
    unsigned char keydata[64];

    /* Compute the (2^COMB_BITS - 1)/2 term once. */
    secp256k1_ecmult_gen_scalar_diff(&diff);

    if (seed32 == NULL) {
        /* When seed is NULL, reset the final point and blinding value. */
        secp256k1_ge_neg(&ctx->ge_offset, &secp256k1_ge_const_g);
        secp256k1_scalar_add(&ctx->scalar_offset, &secp256k1_scalar_one, &diff);
        ctx->proj_blind = secp256k1_fe_one;
        return;
    }
    /* The prior blinding value (if not reset) is chained forward by including it in the hash. */
    secp256k1_scalar_get_b32(keydata, &ctx->scalar_offset);
    /** Using a CSPRNG allows a failure free interface, avoids needing large amounts of random data,
     *   and guards against weak or adversarial seeds.  This is a simpler and safer interface than
     *   asking the caller for blinding values directly and expecting them to retry on failure.
     */
    VERIFY_CHECK(seed32 != NULL);
    memcpy(keydata + 32, seed32, 32);
    secp256k1_rfc6979_hmac_sha256_initialize(&rng, keydata, 64);
    secp256k1_memclear_explicit(keydata, sizeof(keydata));

    /* Compute projective blinding factor (cannot be 0). */
    secp256k1_rfc6979_hmac_sha256_generate(&rng, nonce32, 32);
    secp256k1_fe_set_b32_mod(&f, nonce32);
    secp256k1_fe_cmov(&f, &secp256k1_fe_one, secp256k1_fe_normalizes_to_zero(&f));
    ctx->proj_blind = f;

    /* For a random blinding value b, set scalar_offset=diff-b, ge_offset=bG */
    secp256k1_rfc6979_hmac_sha256_generate(&rng, nonce32, 32);
    secp256k1_scalar_set_b32(&b, nonce32, NULL);
    /* The blinding value cannot be zero, as that would mean ge_offset = infinity,
     * which secp256k1_gej_add_ge cannot handle. */
    secp256k1_scalar_cmov(&b, &secp256k1_scalar_one, secp256k1_scalar_is_zero(&b));
    secp256k1_rfc6979_hmac_sha256_finalize(&rng);
    secp256k1_ecmult_gen(ctx, &gb, &b);
    secp256k1_scalar_negate(&b, &b);
    secp256k1_scalar_add(&ctx->scalar_offset, &b, &diff);
    secp256k1_ge_set_gej(&ctx->ge_offset, &gb);

    /* Clean up. */
    secp256k1_memclear_explicit(nonce32, sizeof(nonce32));
    secp256k1_scalar_clear(&b);
    secp256k1_gej_clear(&gb);
    secp256k1_fe_clear(&f);
    secp256k1_rfc6979_hmac_sha256_clear(&rng);
}

#endif /* SECP256K1_ECMULT_GEN_IMPL_H */
