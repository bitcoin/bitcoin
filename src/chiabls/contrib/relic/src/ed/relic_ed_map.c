/*
 * RELIC is an Efficient LIbrary for Cryptography
 * Copyright (C) 2007-2017 RELIC Authors
 *
 * This file is part of RELIC. RELIC is legal property of its developers,
 * whose names are not listed here. Please refer to the COPYRIGHT file
 * for contact information.
 *
 * RELIC is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * RELIC is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with RELIC. If not, see <http://www.gnu.org/licenses/>.
 */

/**
 * @file
 *
 * Implementation of hashing to a prime elliptic curve.
 *
 * @version $Id$
 * @ingroup ed
 */

#include "relic_core.h"
#include "relic_md.h"

/*============================================================================*/
/* Public definitions                                                         */
/*============================================================================*/

/* Square root of -1 mod p. */
#define SRTM1 "2B8324804FC1DF0B2B4D00993DFBD7A72F431806AD2FE478C4EE1B274A0EA0B0"

void ed_map(ed_t p, const uint8_t *msg, int len) {
	bn_t h;
	fp_t t, u, v;
	uint8_t digest[MD_LEN];

	bn_null(h);
	fp_null(t);
	fp_null(u);
	fp_null(v);

	TRY {
		bn_new(h);
		fp_new(t);
		fp_new(u);
		fp_new(v);

		md_map(digest, msg, len);
		bn_read_bin(h, digest, MIN(FP_BYTES, MD_LEN));

		fp_prime_conv(p->y, h);
		fp_set_dig(p->z, 1);

		/* Make e = p. */
		h->used = FP_DIGS;
		dv_copy(h->dp, fp_prime_get(), FP_DIGS);

		/* Compute a^((p - 5)/8). */
		bn_sub_dig(h, h, 5);
		bn_rsh(h, h, 3);

		/* Decode using Elligator 2. */
		while (1) {
			/* u = y^2 - 1, v = d * y^2 + 1. */
			fp_sqr(u, p->y);
			fp_mul(v, u, core_get()->ed_d);
			fp_sub_dig(u, u, 1);
			fp_add_dig(v, v, 1);

			/* t = v^3, x = uv^7. */
			fp_sqr(t, v);
			fp_mul(t, t, v);
			fp_sqr(p->x, t);
			fp_mul(p->x, p->x, v);
			fp_mul(p->x, p->x, u);

			/* x = uv^3 * (uv^7)^((p - 5)/8). */
			fp_exp(p->x, p->x, h);
			fp_mul(p->x, p->x, t);
			fp_mul(p->x, p->x, u);

			/* Check if vx^2 == u. */
			fp_sqr(t, p->x);
			fp_mul(t, t, v);

			if (fp_cmp(t, u) != CMP_EQ) {
				fp_neg(u, u);
				/* Check if vx^2 == -u. */
				if (fp_cmp(t, u) != CMP_EQ) {
					fp_add_dig(p->y, p->y, 1);
				} else {
					fp_read_str(t, SRTM1, strlen(SRTM1), 16);
					fp_mul(p->x, p->x, t);
					break;
				}
			} else {
				break;
			}
		}

		/* By Elligator convention. */
		if (p->x[FP_DIGS - 1] >> (FP_DIGIT - 1) == 1) {
			fp_neg(p->x, p->x);
		}

		/* Multiply by cofactor. */
		ed_dbl(p, p);
		ed_dbl(p, p);
		ed_dbl(p, p);
		ed_norm(p, p);

#if ED_ADD == EXTND
		fp_mul(p->t, p->x, p->y);
#endif
		p->norm = 1;
	}
	CATCH_ANY {
		THROW(ERR_CAUGHT);
	}
	FINALLY {
		bn_free(h);
		fp_free(t);
		fp_free(u);
		fp_free(v);
	}
}