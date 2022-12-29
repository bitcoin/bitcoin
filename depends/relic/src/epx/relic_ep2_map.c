/*
 * RELIC is an Efficient LIbrary for Cryptography
 * Copyright (c) 2012 RELIC Authors
 *
 * This file is part of RELIC. RELIC is legal property of its developers,
 * whose names are not listed here. Please refer to the COPYRIGHT file
 * for contact information.
 *
 * RELIC is free software; you can redistribute it and/or modify it under the
 * terms of the version 2.1 (or later) of the GNU Lesser General Public License
 * as published by the Free Software Foundation; or version 2.0 of the Apache
 * License as published by the Apache Software Foundation. See the LICENSE files
 * for more details.
 *
 * RELIC is distributed in the hope that it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR
 * A PARTICULAR PURPOSE. See the LICENSE files for more details.
 *
 * You should have received a copy of the GNU Lesser General Public or the
 * Apache License along with RELIC. If not, see <https://www.gnu.org/licenses/>
 * or <https://www.apache.org/licenses/>.
 */

/**
 * @file
 *
 * Implementation of hashing to a prime elliptic curve over a quadratic
 * extension.
 *
 * @ingroup epx
 */

#include "relic_core.h"
#include "relic_md.h"
#include "relic_tmpl_map.h"

/*============================================================================*/
/* Private definitions                                                        */
/*============================================================================*/

#ifdef EP_CTMAP
/**
 * Evaluate a polynomial represented by its coefficients using Horner's rule.
 *
 * @param[out] c			- the result.
 * @param[in] a				- the input value.
 * @param[in] coeffs		- the vector of coefficients in the polynomial.
 * @param[in] len			- the degree of the polynomial.
 */
TMPL_MAP_HORNER(fp2, fp2_t)

/**
 * Generic isogeny map evaluation for use with SSWU map.
 */
TMPL_MAP_ISOGENY_MAP(ep2, fp2, iso2)
#endif /* EP_CTMAP */

/**
 * Simplified SWU mapping.
 */
#define EP2_MAP_COPY_COND(O, I, C)                                                       \
	do {                                                                                 \
		dv_copy_cond(O[0], I[0], RLC_FP_DIGS, C);                                        \
		dv_copy_cond(O[1], I[1], RLC_FP_DIGS, C);                                        \
	} while (0)
TMPL_MAP_SSWU(ep2, fp2, fp_t, EP2_MAP_COPY_COND)

/**
 * Shallue--van de Woestijne map.
 */
TMPL_MAP_SVDW(ep2, fp2, fp_t, EP2_MAP_COPY_COND)
#undef EP2_MAP_COPY_COND

/* caution: this function overwrites k, which it uses as an auxiliary variable */
static inline int fp2_sgn0(const fp2_t t, bn_t k) {
	const int t_0_zero = fp_is_zero(t[0]);

	fp_prime_back(k, t[0]);
	const int t_0_neg = bn_get_bit(k, 0);

	fp_prime_back(k, t[1]);
	const int t_1_neg = bn_get_bit(k, 0);

	/* t[0] == 0 ? sgn0(t[1]) : sgn0(t[0]) */
	return t_0_neg | (t_0_zero & t_1_neg);
}

/*============================================================================*/
/* Public definitions                                                         */
/*============================================================================*/

void ep2_map_from_field(ep2_t p, const uint8_t *uniform_bytes, int len) {
        bn_t k;
        fp2_t t;
        ep2_t q;
        int neg;
        /* enough space for two extension field elements plus extra bytes for uniformity */
        const int len_per_elm = (FP_PRIME + ep_param_level() + 7) / 8;

        bn_null(k);
        fp2_null(t);
        ep2_null(q);

        RLC_TRY {
                if (len != 2* len_per_elm) {
                  RLC_THROW(ERR_NO_VALID);
                }

                bn_new(k);
                fp2_new(t);
                ep2_new(q);

                /* which hash function should we use? */
                const int abNeq0 = (ep2_curve_opt_a() != RLC_ZERO) && (ep2_curve_opt_b() != RLC_ZERO);
                void (*const map_fn)(ep2_t, fp2_t) = (ep2_curve_is_ctmap() || abNeq0) ? ep2_map_sswu : ep2_map_svdw;

#define EP2_MAP_CONVERT_BYTES(IDX)                                                       \
        do {                                                                                 \
                bn_read_bin(k, uniform_bytes + 2 * IDX * len_per_elm, len_per_elm);        \
                fp_prime_conv(t[0], k);                                                          \
                bn_read_bin(k, uniform_bytes + (2 * IDX + 1) * len_per_elm, len_per_elm);  \
                fp_prime_conv(t[1], k);                                                          \
        } while (0)

#define EP2_MAP_APPLY_MAP(PT)                                                            \
        do {                                                                                 \
                /* sign of t */                                                                  \
                neg = fp2_sgn0(t, k);                                                            \
                /* convert */                                                                    \
                map_fn(PT, t);                                                                   \
                /* compare sign of y to sign of t; fix if necessary */                           \
                neg = neg != fp2_sgn0(PT->y, k);                                                 \
                fp2_neg(t, PT->y);                                                               \
                dv_copy_cond(PT->y[0], t[0], RLC_FP_DIGS, neg);                                  \
                dv_copy_cond(PT->y[1], t[1], RLC_FP_DIGS, neg);                                  \
        } while (0)

                /* first map invocation */
                EP2_MAP_CONVERT_BYTES(0);
                EP2_MAP_APPLY_MAP(p);
                TMPL_MAP_CALL_ISOMAP(ep2, p);

                /* second map invocation */
                EP2_MAP_CONVERT_BYTES(1);
                EP2_MAP_APPLY_MAP(q);
                TMPL_MAP_CALL_ISOMAP(ep2, q);

                /* XXX(rsw) could add p and q and then apply isomap,
                 * but need ep_add to support addition on isogeny curves */

#undef EP2_MAP_CONVERT_BYTES
#undef EP2_MAP_APPLY_MAP

                /* sum the result */
                ep2_add(p, p, q);
                ep2_norm(p, p);
                ep2_mul_cof(p, p);
        }
        RLC_CATCH_ANY {
                RLC_THROW(ERR_CAUGHT);
        }
        RLC_FINALLY {
                bn_free(k);
                fp2_free(t);
                ep2_free(q);
        }
}


void ep2_map_dst(ep2_t p, const uint8_t *msg, int len, const uint8_t *dst, int dst_len) {

        /* enough space for two field elements plus extra bytes for uniformity */
        const int len_per_elm = (FP_PRIME + ep_param_level() + 7) / 8;
        uint8_t *pseudo_random_bytes = RLC_ALLOCA(uint8_t, 4 * len_per_elm);

        RLC_TRY {

                /* XXX(rsw) See note in ep/relic_ep_map.c about using MD_MAP. */
                /* hash to a pseudorandom string using md_xmd */
                md_xmd(pseudo_random_bytes, 4 * len_per_elm, msg, len, dst, dst_len);
                ep2_map_from_field(p, pseudo_random_bytes, 2 * len_per_elm);
        }
        RLC_CATCH_ANY {
                RLC_THROW(ERR_CAUGHT);
        }
        RLC_FINALLY {
                RLC_FREE(pseudo_random_bytes);
        }
}

void ep2_map(ep2_t p, const uint8_t *msg, int len) {
	ep2_map_dst(p, msg, len, (const uint8_t *)"RELIC", 5);
}
