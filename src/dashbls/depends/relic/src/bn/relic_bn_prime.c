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
 * Implementation of the prime number generation and testing functions.
 *
 * Strong prime generation is based on Gordon's Algorithm, taken from Handbook
 * of Applied Cryptography.
 *
 * @ingroup bn
 */

#include "relic_core.h"

/*============================================================================*/
/* Private definitions                                                        */
/*============================================================================*/

/**
 * Number of trial division tests.
 */
#define BASIC_TESTS	((int)(sizeof(primes)/sizeof(dig_t)))

/**
 * Small prime numbers table.
 */
static const dig_t primes[] = {
	0x0002, 0x0003, 0x0005, 0x0007, 0x000B, 0x000D, 0x0011, 0x0013,
	0x0017, 0x001D, 0x001F, 0x0025, 0x0029, 0x002B, 0x002F, 0x0035,
	0x003B, 0x003D, 0x0043, 0x0047, 0x0049, 0x004F, 0x0053, 0x0059,
	0x0061, 0x0065, 0x0067, 0x006B, 0x006D, 0x0071, 0x007F, 0x0083,
	0x0089, 0x008B, 0x0095, 0x0097, 0x009D, 0x00A3, 0x00A7, 0x00AD,
	0x00B3, 0x00B5, 0x00BF, 0x00C1, 0x00C5, 0x00C7, 0x00D3, 0x00DF,
#if WSIZE > 8
	0x00E3, 0x00E5, 0x00E9, 0x00EF, 0x00F1, 0x00FB, 0x0101, 0x0107,
	0x010D, 0x010F, 0x0115, 0x0119, 0x011B, 0x0125, 0x0133, 0x0137,

	0x0139, 0x013D, 0x014B, 0x0151, 0x015B, 0x015D, 0x0161, 0x0167,
	0x016F, 0x0175, 0x017B, 0x017F, 0x0185, 0x018D, 0x0191, 0x0199,
	0x01A3, 0x01A5, 0x01AF, 0x01B1, 0x01B7, 0x01BB, 0x01C1, 0x01C9,
	0x01CD, 0x01CF, 0x01D3, 0x01DF, 0x01E7, 0x01EB, 0x01F3, 0x01F7,
	0x01FD, 0x0209, 0x020B, 0x021D, 0x0223, 0x022D, 0x0233, 0x0239,
	0x023B, 0x0241, 0x024B, 0x0251, 0x0257, 0x0259, 0x025F, 0x0265,
	0x0269, 0x026B, 0x0277, 0x0281, 0x0283, 0x0287, 0x028D, 0x0293,
	0x0295, 0x02A1, 0x02A5, 0x02AB, 0x02B3, 0x02BD, 0x02C5, 0x02CF,

	0x02D7, 0x02DD, 0x02E3, 0x02E7, 0x02EF, 0x02F5, 0x02F9, 0x0301,
	0x0305, 0x0313, 0x031D, 0x0329, 0x032B, 0x0335, 0x0337, 0x033B,
	0x033D, 0x0347, 0x0355, 0x0359, 0x035B, 0x035F, 0x036D, 0x0371,
	0x0373, 0x0377, 0x038B, 0x038F, 0x0397, 0x03A1, 0x03A9, 0x03AD,
	0x03B3, 0x03B9, 0x03C7, 0x03CB, 0x03D1, 0x03D7, 0x03DF, 0x03E5,
	0x03F1, 0x03F5, 0x03FB, 0x03FD, 0x0407, 0x0409, 0x040F, 0x0419,
	0x041B, 0x0425, 0x0427, 0x042D, 0x043F, 0x0443, 0x0445, 0x0449,
	0x044F, 0x0455, 0x045D, 0x0463, 0x0469, 0x047F, 0x0481, 0x048B,

	0x0493, 0x049D, 0x04A3, 0x04A9, 0x04B1, 0x04BD, 0x04C1, 0x04C7,
	0x04CD, 0x04CF, 0x04D5, 0x04E1, 0x04EB, 0x04FD, 0x04FF, 0x0503,
	0x0509, 0x050B, 0x0511, 0x0515, 0x0517, 0x051B, 0x0527, 0x0529,
	0x052F, 0x0551, 0x0557, 0x055D, 0x0565, 0x0577, 0x0581, 0x058F,
	0x0593, 0x0595, 0x0599, 0x059F, 0x05A7, 0x05AB, 0x05AD, 0x05B3,
	0x05BF, 0x05C9, 0x05CB, 0x05CF, 0x05D1, 0x05D5, 0x05DB, 0x05E7,
	0x05F3, 0x05FB, 0x0607, 0x060D, 0x0611, 0x0617, 0x061F, 0x0623,
	0x062B, 0x062F, 0x063D, 0x0641, 0x0647, 0x0649, 0x064D, 0x0653,

	0x0655, 0x065B, 0x0665, 0x0679, 0x067F, 0x0683, 0x0685, 0x069D,
	0x06A1, 0x06A3, 0x06AD, 0x06B9, 0x06BB, 0x06C5, 0x06CD, 0x06D3,
	0x06D9, 0x06DF, 0x06F1, 0x06F7, 0x06FB, 0x06FD, 0x0709, 0x0713,
	0x071F, 0x0727, 0x0737, 0x0745, 0x074B, 0x074F, 0x0751, 0x0755,
	0x0757, 0x0761, 0x076D, 0x0773, 0x0779, 0x078B, 0x078D, 0x079D,
	0x079F, 0x07B5, 0x07BB, 0x07C3, 0x07C9, 0x07CD, 0x07CF, 0x07D3,
	0x07DB, 0x07E1, 0x07EB, 0x07ED, 0x07F7, 0x0805, 0x080F, 0x0815,
	0x0821, 0x0823, 0x0827, 0x0829, 0x0833, 0x083F, 0x0841, 0x0851,

	0x0853, 0x0859, 0x085D, 0x085F, 0x0869, 0x0871, 0x0883, 0x089B,
	0x089F, 0x08A5, 0x08AD, 0x08BD, 0x08BF, 0x08C3, 0x08CB, 0x08DB,
	0x08DD, 0x08E1, 0x08E9, 0x08EF, 0x08F5, 0x08F9, 0x0905, 0x0907,
	0x091D, 0x0923, 0x0925, 0x092B, 0x092F, 0x0935, 0x0943, 0x0949,
	0x094D, 0x094F, 0x0955, 0x0959, 0x095F, 0x096B, 0x0971, 0x0977,
	0x0985, 0x0989, 0x098F, 0x099B, 0x09A3, 0x09A9, 0x09AD, 0x09C7,
	0x09D9, 0x09E3, 0x09EB, 0x09EF, 0x09F5, 0x09F7, 0x09FD, 0x0A13,
	0x0A1F, 0x0A21, 0x0A31, 0x0A39, 0x0A3D, 0x0A49, 0x0A57, 0x0A61,

	0x0A63, 0x0A67, 0x0A6F, 0x0A75, 0x0A7B, 0x0A7F, 0x0A81, 0x0A85,
	0x0A8B, 0x0A93, 0x0A97, 0x0A99, 0x0A9F, 0x0AA9, 0x0AAB, 0x0AB5,
	0x0ABD, 0x0AC1, 0x0ACF, 0x0AD9, 0x0AE5, 0x0AE7, 0x0AED, 0x0AF1,
	0x0AF3, 0x0B03, 0x0B11, 0x0B15, 0x0B1B, 0x0B23, 0x0B29, 0x0B2D,
	0x0B3F, 0x0B47, 0x0B51, 0x0B57, 0x0B5D, 0x0B65, 0x0B6F, 0x0B7B,
	0x0B89, 0x0B8D, 0x0B93, 0x0B99, 0x0B9B, 0x0BB7, 0x0BB9, 0x0BC3,
	0x0BCB, 0x0BCF, 0x0BDD, 0x0BE1, 0x0BE9, 0x0BF5, 0x0BFB, 0x0C07,
	0x0C0B, 0x0C11, 0x0C25, 0x0C2F, 0x0C31, 0x0C41, 0x0C5B, 0x0C5F,

	0x0C61, 0x0C6D, 0x0C73, 0x0C77, 0x0C83, 0x0C89, 0x0C91, 0x0C95,
	0x0C9D, 0x0CB3, 0x0CB5, 0x0CB9, 0x0CBB, 0x0CC7, 0x0CE3, 0x0CE5,
	0x0CEB, 0x0CF1, 0x0CF7, 0x0CFB, 0x0D01, 0x0D03, 0x0D0F, 0x0D13,
	0x0D1F, 0x0D21, 0x0D2B, 0x0D2D, 0x0D3D, 0x0D3F, 0x0D4F, 0x0D55,
	0x0D69, 0x0D79, 0x0D81, 0x0D85, 0x0D87, 0x0D8B, 0x0D8D, 0x0DA3,
	0x0DAB, 0x0DB7, 0x0DBD, 0x0DC7, 0x0DC9, 0x0DCD, 0x0DD3, 0x0DD5,
	0x0DDB, 0x0DE5, 0x0DE7, 0x0DF3, 0x0DFD, 0x0DFF, 0x0E09, 0x0E17,
	0x0E1D, 0x0E21, 0x0E27, 0x0E2F, 0x0E35, 0x0E3B, 0x0E4B, 0x0E57,
#endif
};

#if BN_MOD == PMERS

/**
 * Computes c = a ^ b mod m.
 *
 * @param c				- the result.
 * @param a				- the basis.
 * @param b				- the exponent.
 * @param m				- the modulus.
 */
static void bn_exp(bn_t c, const bn_t a, const bn_t b, const bn_t m) {
	int i, l;
	bn_t t;

	bn_null(t);

	RLC_TRY {
		bn_new(t);

		l = bn_bits(b);

		bn_copy(t, a);

		for (i = l - 2; i >= 0; i--) {
			bn_sqr(t, t);
			bn_mod(t, t, m);
			if (bn_get_bit(b, i)) {
				bn_mul(t, t, a);
				bn_mod(t, t, m);
			}
		}

		bn_copy(c, t);
	}
	RLC_CATCH_ANY {
		RLC_THROW(ERR_CAUGHT);
	}
	RLC_FINALLY {
		bn_free(t);
	}
}

#endif

/*============================================================================*/
/* Public definitions                                                         */
/*============================================================================*/

dig_t bn_get_prime(int pos) {
	if (pos >= BASIC_TESTS) {
		return 0;
	}
	return primes[pos];
}

int bn_is_prime(const bn_t a) {
	int result;

	result = 0;
	if (!bn_is_prime_basic(a)) {
		goto end;
	}

	if (!bn_is_prime_rabin(a)) {
		goto end;
	}

	result = 1;
  end:
	return result;
}

int bn_is_prime_basic(const bn_t a) {
	dig_t t;
	int i, result;

	result = 1;

	if (bn_cmp_dig(a, 1) == RLC_EQ) {
		return 0;
	}

	/* Trial division. */
	for (i = 0; i < BASIC_TESTS; i++) {
		bn_mod_dig(&t, a, primes[i]);
		if (t == 0 && bn_cmp_dig(a, primes[i]) != RLC_EQ) {
			result = 0;
			break;
		}
	}
	return result;
}

int bn_is_prime_rabin(const bn_t a) {
	bn_t t, n1, y, r;
	int i, s, j, result, b, tests = 0, cmp2;

	tests = 0;
	result = 1;

	bn_null(t);
	bn_null(n1);
	bn_null(y);
	bn_null(r);

	cmp2 = bn_cmp_dig(a, 2);
	if (cmp2 == RLC_LT) {
		/* Numbers 1 or smaller are not prime */
		return 0;
	}
	if (cmp2 == RLC_EQ) {
		/* The number 2 is prime */
		return 1;
	}

	if (bn_is_even(a) == 1) {
		/* Even numbers > 2 are not prime */
		return 0;
	}

	RLC_TRY {
		/*
		 * These values are taken from Table 4.4 inside Handbook of Applied
		 * Cryptography.
		 */
		b = bn_bits(a);
		if (b >= 1300) {
			tests = 2;
		} else if (b >= 850) {
			tests = 3;
		} else if (b >= 650) {
			tests = 4;
		} else if (b >= 550) {
			tests = 5;
		} else if (b >= 450) {
			tests = 6;
		} else if (b >= 400) {
			tests = 7;
		} else if (b >= 350) {
			tests = 8;
		} else if (b >= 300) {
			tests = 9;
		} else if (b >= 250) {
			tests = 12;
		} else if (b >= 200) {
			tests = 15;
		} else if (b >= 150) {
			tests = 18;
		} else {
			tests = 27;
		}

		bn_new(t);
		bn_new(n1);
		bn_new(y);
		bn_new(r);

		/* r = (n - 1)/2^s. */
		bn_sub_dig(n1, a, 1);
		bn_copy(r, n1);
		s = 0;
		while (bn_is_even(r)) {
			s++;
			bn_rsh(r, r, 1);
		}

		for (i = 0; i < tests; i++) {
			/* Fix the basis as the first few primes. */
			bn_set_dig(t, primes[i]);

			/* Ensure t <= n - 2 as per HAC */
			if( bn_cmp(t, n1) != RLC_LT ) {
				result = 1;
				break;
			}

			/* y = b^r mod a. */
#if BN_MOD != PMERS
			bn_mxp(y, t, r, a);
#else
			bn_exp(y, t, r, a);
#endif

			if (bn_cmp_dig(y, 1) != RLC_EQ && bn_cmp(y, n1) != RLC_EQ) {
				j = 1;
				while ((j <= (s - 1)) && bn_cmp(y, n1) != RLC_EQ) {
					bn_sqr(y, y);
					bn_mod(y, y, a);

					/* If y == 1 then composite. */
					if (bn_cmp_dig(y, 1) == RLC_EQ) {
						result = 0;
						break;
					}
					++j;
				}

				/* If y != n1 then composite. */
				if (bn_cmp(y, n1) != RLC_EQ) {
					result = 0;
					break;
				}
			}
		}
	}
	RLC_CATCH_ANY {
		result = 0;
		RLC_THROW(ERR_CAUGHT);
	}
	RLC_FINALLY {
		bn_free(r);
		bn_free(y);
		bn_free(n1);
		bn_free(t);
	}
	return result;
}

int bn_is_prime_solov(const bn_t a) {
	bn_t t0, t1, t2;
	int i, result;

	bn_null(t0);
	bn_null(t1);
	bn_null(t2);

	result = 1;

	RLC_TRY {
		bn_new(t0);
		bn_new(t1);
		bn_new(t2);

		for (i = 0; i < 100; i++) {
			/* Generate t0, 2 <= t0, <= a - 2. */
			do {
				bn_rand(t0, RLC_POS, bn_bits(a));
				bn_mod(t0, t0, a);
			} while (bn_cmp_dig(t0, 2) == RLC_LT);
			/* t2 = a - 1. */
			bn_copy(t2, a);
			bn_sub_dig(t2, t2, 1);
			/* t1 = (a - 1)/2. */
			bn_rsh(t1, t2, 1);
			/* t1 = t0^(a - 1)/2 mod a. */
#if BN_MOD != PMERS
			bn_mxp(t1, t0, t1, a);
#else
			bn_exp(t1, t0, t1, a);
#endif
			/* If t1 != 1 and t1 != n - 1 return 0 */
			if (bn_cmp_dig(t1, 1) != RLC_EQ && bn_cmp(t1, t2) != RLC_EQ) {
				result = 0;
				break;
			}

			/* t2 = (t0|a). */
			bn_smb_jac(t2, t0, a);
			if (bn_sign(t2) == RLC_NEG) {
				bn_add(t2, t2, a);
			}
			/* If t1 != t2 (mod a) return 0. */
			bn_mod(t1, t1, a);
			bn_mod(t2, t2, a);
			if (bn_cmp(t1, t2) != RLC_EQ) {
				result = 0;
				break;
			}
		}
	}
	RLC_CATCH_ANY {
		result = 0;
		RLC_THROW(ERR_CAUGHT);
	}
	RLC_FINALLY {
		bn_free(t0);
		bn_free(t1);
		bn_free(t2);
	}
	return result;
}

#if BN_GEN == BASIC || !defined(STRIP)

void bn_gen_prime_basic(bn_t a, int bits) {
	while (1) {
		do {
			bn_rand(a, RLC_POS, bits);
		} while (bn_bits(a) != bits);
		if (bn_is_prime(a)) {
			return;
		}
	}
}

#endif

#if BN_GEN == SAFEP || !defined(STRIP)

void bn_gen_prime_safep(bn_t a, int bits) {
	while (1) {
		do {
			bn_rand(a, RLC_POS, bits);
		} while (bn_bits(a) != bits);
		/* Check if (a - 1)/2 is prime. */
		bn_sub_dig(a, a, 1);
		bn_rsh(a, a, 1);
		if (bn_is_prime(a)) {
			/* Restore a. */
			bn_lsh(a, a, 1);
			bn_add_dig(a, a, 1);
			if (bn_is_prime(a)) {
				/* Should be prime now. */
				return;
			}
		}
	}
}

#endif

#if BN_GEN == STRON || !defined(STRIP)

void bn_gen_prime_stron(bn_t a, int bits) {
	dig_t i, j;
	int found, k;
	bn_t r, s, t;

	bn_null(r);
	bn_null(s);
	bn_null(t);

	RLC_TRY {
		bn_new(r);
		bn_new(s);
		bn_new(t);

		do {
			do {
				/* Generate two large primes r and s. */
				bn_rand(s, RLC_POS, bits / 2 - RLC_DIG / 2);
				bn_rand(t, RLC_POS, bits / 2 - RLC_DIG / 2);
			} while (!bn_is_prime(s) || !bn_is_prime(t));
			found = 1;
			bn_rand(a, RLC_POS, bits / 2 - bn_bits(t) - 1);
			i = a->dp[0];
			bn_dbl(t, t);
			do {
				/* Find first prime r = 2 * i * t + 1. */
				bn_mul_dig(r, t, i);
				bn_add_dig(r, r, 1);
				i++;
				if (bn_bits(r) > bits / 2 - 1) {
					found = 0;
					break;
				}
			} while (!bn_is_prime(r));
			if (found == 0) {
				continue;
			}
			/* Compute t = 2 * (s^(r-2) mod r) * s - 1. */
			bn_sub_dig(t, r, 2);
#if BN_MOD != PMERS
			bn_mxp(t, s, t, r);
#else
			bn_exp(t, s, t, r);
#endif

			bn_mul(t, t, s);
			bn_dbl(t, t);
			bn_sub_dig(t, t, 1);

			k = bits - bn_bits(r);
			k -= bn_bits(s);
			bn_rand(a, RLC_POS, k);
			j = a->dp[0];
			do {
				/* Find first prime a = t + 2 * j * r * s. */
				bn_mul(a, r, s);
				bn_mul_dig(a, a, j);
				bn_dbl(a, a);
				bn_add(a, a, t);
				j++;
				if (bn_bits(a) > bits) {
					found = 0;
					break;
				}
			} while (!bn_is_prime(a));
		} while (found == 0 && bn_bits(a) != bits);
	}
	RLC_CATCH_ANY {
		RLC_THROW(ERR_CAUGHT);
	}
	RLC_FINALLY {
		bn_free(r);
		bn_free(s);
		bn_free(t);
	}
}

#endif
