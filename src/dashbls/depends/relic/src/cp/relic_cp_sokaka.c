/*
 * RELIC is an Efficient LIbrary for Cryptography
 * Copyright (c) 2009 RELIC Authors
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
 * Implementation of the Sakai-Ohgishi-Kasahara Identity-Based Non-Interactive
 * Authenticated Key Agreement scheme.
 *
 * @ingroup test
 */

#include "relic.h"

/*============================================================================*/
/* Public definitions                                                         */
/*============================================================================*/

int cp_sokaka_gen(bn_t master) {
	bn_t n;
	int result = RLC_OK;

	bn_null(n);

	RLC_TRY {
		bn_new(n);

		pc_get_ord(n);
		bn_rand_mod(master, n);
	}
	RLC_CATCH_ANY {
		result = RLC_ERR;
	}
	RLC_FINALLY {
		bn_free(n);
	}
	return result;
}

int cp_sokaka_gen_prv(sokaka_t k, char *id, bn_t master) {
	if (pc_map_is_type1()) {
		g1_map(k->s1, (uint8_t *)id, strlen(id));
		g1_mul(k->s1, k->s1, master);
	} else {
		g1_map(k->s1, (uint8_t *)id, strlen(id));
		g1_mul(k->s1, k->s1, master);
		g2_map(k->s2, (uint8_t *)id, strlen(id));
		g2_mul(k->s2, k->s2, master);
	}
	return RLC_OK;
}

int cp_sokaka_key(uint8_t *key, unsigned int key_len, char *id1,
		sokaka_t k, char *id2) {
	int len1 = strlen(id1), len2 = strlen(id2);
	int size, first = 0, result = RLC_OK;
	uint8_t *buf;
	g1_t p;
	g2_t q;
	gt_t e;

	g1_null(p);
	g2_null(q);
	gt_null(e);

	RLC_TRY {
		g1_new(p);
		g2_new(q);
		gt_new(e);
		size = gt_size_bin(e, 0);
		buf = RLC_ALLOCA(uint8_t, size);
		if (buf == NULL) {
			RLC_THROW(ERR_NO_MEMORY);
		}

		if (len1 == len2) {
			if (strncmp(id1, id2, len1) == 0) {
				RLC_THROW(ERR_NO_VALID);
			}
			first = (strncmp(id1, id2, len1) < 0 ? 1 : 2);
		} else {
			if (len1 < len2) {
				if (strncmp(id1, id2, len1) == 0) {
					first = 1;
				} else {
					first = (strncmp(id1, id2, len1) < 0 ? 1 : 2);
				}
			} else {
				if (strncmp(id1, id2, len2) == 0) {
					first = 2;
				} else {
					first = (strncmp(id1, id2, len2) < 0 ? 1 : 2);
				}
			}
		}

		if (pc_map_is_type1()) {
			g2_map(q, (uint8_t *)id2, len2);
			pc_map(e, k->s1, q);
		} else {
			if (first == 1) {
				g2_map(q, (uint8_t *)id2, len2);
				pc_map(e, k->s1, q);
			} else {
				g1_map(p, (uint8_t *)id2, len2);
				pc_map(e, p, k->s2);
			}
		}

		/* Allocate size for storing the output. */
		gt_write_bin(buf, size, e, 0);
		md_kdf(key, key_len, buf, size);
	}
	RLC_CATCH_ANY {
		result = RLC_ERR;
	}
	RLC_FINALLY {
		g1_free(p);
		g2_free(q);
		gt_free(e);
		RLC_FREE(buf);
	}
	return result;
}
