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
 * Implementation of the prime field modulus manipulation.
 *
 * @ingroup fp
 */

#include "relic_core.h"
#include "relic_fpx.h"

/*============================================================================*/
/* Private definitions                                                        */
/*============================================================================*/

#if FP_PRIME == 1536
/**
 * Cofactor description of 1536-bit prime modulus.
 */
#define SS_P1536	"83093742908D4D529CEF06C72191A05D5E6073FE861E637D7747C3E52FBB92DAA5DDF3EF1C61F5F70B256802481A36CAFE995FE33CD54014B846751364C0D3B8327D9E45366EA08F1B3446AC23C9D4B656886731A8D05618CFA1A3B202A2445ABA0E77C5F4F00CA1239975A05377084F256DEAA07D21C4CF2A4279BC117603ACB7B10228C3AB8F8C1742D674395701BB02071A88683041D9C4231E8EE982B8DA"
#endif

#if FP_PRIME == 256

/**
 * Random prime modulus for the Brainpool P256r1.
 */
#define BSI_P256		"A9FB57DBA1EEA9BC3E660A909D838D726E3BF623D52620282013481D1F6E5377"

#endif

/*============================================================================*/
/* Public definitions                                                         */
/*============================================================================*/

int fp_param_get(void) {
	return core_get()->fp_id;
}

void fp_param_get_var(bn_t x) {
	bn_t a;

	bn_null(a);

	TRY {
		bn_new(a);

		switch (fp_param_get()) {
			case BN_158:
				/* x = 0x4000000031. */
				bn_set_2b(x, 38);
				bn_add_dig(x, x, 0x31);
				break;
			case BN_254:
				/* x = -(2^62 + 2^55 + 1). */
				bn_set_2b(x, 62);
				bn_set_2b(a, 55);
				bn_add(x, x, a);
				bn_add_dig(x, x, 1);
				bn_neg(x, x);
				break;
			case BN_256:
				/* x = -0x600000000000219B. */
				bn_set_2b(x, 62);
				bn_set_2b(a, 61);
				bn_add(x, x, a);
				bn_set_dig(a, 0x21);
				bn_lsh(a, a, 8);
				bn_add(x, x, a);
				bn_add_dig(x, x, 0x9B);
				bn_neg(x, x);
				break;
			case B12_381:
				/* x = -(2^63 + 2^62 + 2^60 + 2^57 + 2^48 + 2^16). */
				bn_set_2b(x, 63);
				bn_set_2b(a, 62);
				bn_add(x, x, a);
				bn_set_2b(a, 60);
				bn_add(x, x, a);
				bn_set_2b(a, 57);
				bn_add(x, x, a);
				bn_set_2b(a, 48);
				bn_add(x, x, a);
				bn_set_2b(a, 16);
				bn_add(x, x, a);
				bn_neg(x, x);
				break;
			case BN_382:
				/* x = -(2^94 + 2^78 + 2^67 + 2^64 + 2^48 + 1). */
				bn_set_2b(x, 94);
				bn_set_2b(a, 78);
				bn_add(x, x, a);
				bn_set_2b(a, 67);
				bn_add(x, x, a);
				bn_set_2b(a, 64);
				bn_add(x, x, a);
				bn_set_2b(a, 48);
				bn_add(x, x, a);
				bn_add_dig(x, x, 1);
				bn_neg(x, x);
				break;
			case B12_455:
				/* x = 2^76 + 2^53 + 2^31 + 2^11. */
				bn_set_2b(x, 76);
				bn_set_2b(a, 53);
				bn_add(x, x, a);
				bn_set_2b(a, 31);
				bn_add(x, x, a);
				bn_set_2b(a, 11);
				bn_add(x, x, a);
				break;
			case B24_477:
				/* x = -2^48 + 2^45 + 2^31 - 2^7. */
				bn_set_2b(x, 48);
				bn_set_2b(a, 45);
				bn_sub(x, x, a);
				bn_set_2b(a, 31);
				bn_sub(x, x, a);
				bn_set_2b(a, 7);
				bn_add(x, x, a);
				bn_neg(x, x);
				break;
			case KSS_508:
				/* x = -(2^64 + 2^51 - 2^46 - 2^12). */
				bn_set_2b(x, 64);
				bn_set_2b(a, 51);
				bn_add(x, x, a);
				bn_set_2b(a, 46);
				bn_sub(x, x, a);
				bn_set_2b(a, 12);
				bn_sub(x, x, a);
				bn_neg(x, x);
				break;
			case BN_638:
				/* x = 2^158 - 2^128 - 2^68 + 1. */
				bn_set_2b(x, 158);
				bn_set_2b(a, 128);
				bn_sub(x, x, a);
				bn_set_2b(a, 68);
				bn_sub(x, x, a);
				bn_add_dig(x, x, 1);
				break;
			case B12_638:
				/* x = -2^107 + 2^105 + 2^93 + 2^5. */
				bn_set_2b(x, 107);
				bn_set_2b(a, 105);
				bn_sub(x, x, a);
				bn_set_2b(a, 93);
				bn_sub(x, x, a);
				bn_set_2b(a, 5);
				bn_sub(x, x, a);
				bn_neg(x, x);
				break;
			case SS_1536:
				/* x = 2^255 + 2^41 + 1. */
				bn_set_2b(x, 255);
				bn_set_2b(a, 41);
				bn_add(x, x, a);
				bn_add_dig(x, x, 1);
				break;
			default:
				THROW(ERR_NO_VALID);
				break;
		}
	}
	CATCH_ANY {
		THROW(ERR_CAUGHT);
	}
	FINALLY {
		bn_free(a);
	}
}

void fp_param_get_sps(int *s, int *len) {
	bn_t a;

	bn_null(a);

	if (*len < MAX_TERMS) {
		THROW(ERR_NO_BUFFER);
	}

	TRY {
		bn_new(a);

		*len = 0;

		switch (fp_param_get()) {
			case BN_158:
			case BN_254:
			case BN_256:
			case BN_382:
				fp_param_get_var(a);
				if (bn_sign(a) == BN_NEG) {
					bn_neg(a, a);
				}
				*len = bn_ham(a);
				for (int i = 0, j = 0; j < bn_bits(a); j++) {
					if (bn_get_bit(a, j)) {
						s[i++] = j;
					}
				}
				break;
			case B12_381:
				s[0] = 16;
				s[1] = 48;
				s[2] = 57;
				s[3] = 60;
				s[4] = 62;
				s[5] = 63;
				*len = 6;
				break;
			case B12_455:
				s[0] = 11;
				s[1] = 31;
				s[2] = 53;
				s[3] = 76;
				*len = 4;
				break;
			case B24_477:
				s[0] = 7;
				s[1] = -31;
				s[2] = -45;
				s[3] = 48;
				*len = 4;
				break;
			case KSS_508:
				s[0] = -12;
				s[1] = -46;
				s[2] = 51;
				s[3] = 64;
				*len = 4;
				break;
			case BN_638:
				s[0] = 0;
				s[1] = -68;
				s[2] = -128;
				s[3] = 158;
				*len = 4;
				break;
			case B12_638:
				s[0] = -5;
				s[1] = -93;
				s[2] = -105;
				s[3] = 107;
				*len = 4;
				break;
			case SS_1536:
				s[0] = 0;
				s[1] = 41;
				s[2] = 255;
				*len = 3;
				break;
			default:
				THROW(ERR_NO_VALID);
				break;
		}
	}
	CATCH_ANY {
		THROW(ERR_CAUGHT);
	}
	FINALLY {
		bn_free(a);
	}
}

void fp_param_get_map(int *s, int *len) {
	if (*len < FP_BITS) {
		THROW(ERR_NO_BUFFER);
	}

	for (int i = 0; i < FP_BITS; i++) {
		s[i] = 0;
	}

	switch (fp_param_get()) {
		case BN_158:
			s[3] = s[5] = s[8] = s[39] = s[40] = 1;
			*len = 41;
			break;
		case BN_254:
			s[2] = s[56] = s[57] = s[63] = s[64] = 1;
			*len = 65;
			break;
		case BN_256:
			s[5] = s[7] = s[8] = s[11] = s[14] = s[15] = s[62] = s[65] = 1;
			*len = 66;
			break;
		case BN_382:
			s[2] = s[49] = s[50] = s[65] = s[66] = s[68] = 1;
			s[69] = s[79] = s[80] = s[95] = s[96] = 1;
			*len = 97;
			break;
		case B12_381:
			s[16] = s[48] = s[57] = s[60] = s[62] = s[63] = 1;
			*len = 64;
			break;
		case B12_455:
			s[11] = s[31] = s[53] = s[76] = 1;
			*len = 77;
			break;
		case B24_477:
			s[7] = s[48] = 1;
			s[31] = s[45] = -1;
			*len = 49;
			break;
		case KSS_508:
			s[64] = s[51] = 1;
			s[12] = s[46] = -1;
			*len = 65;
			break;
		case BN_638:
			s[3] = s[159] = s[160] = 1;
			s[69] = s[70] = s[129] = s[130] = -1;
			*len = 161;
			break;
		case B12_638:
			s[5] = s[93] = s[105] = -1;
			s[107] = 1;
			*len = 108;
			break;
		case SS_1536:
			s[0] = s[41] = s[255] = 1;
			*len = 256;
			break;
		default:
			THROW(ERR_NO_VALID);
			break;
	}
}

void fp_param_set(int param) {
	bn_t t0, t1, t2, p;
	int f[10] = { 0 };

	bn_null(t0);
	bn_null(t1);
	bn_null(t2);
	bn_null(p);

	/* Suppress possible unused parameter warning. */
	(void) f;

	TRY {
		bn_new(t0);
		bn_new(t1);
		bn_new(t2);
		bn_new(p);

		core_get()->fp_id = param;

		switch (param) {
#if FP_PRIME == 158
			case BN_158:
				/* x = 4000000031. */
				fp_param_get_var(t0);
				/* p = 36 * x^4 + 36 * x^3 + 24 * x^2 + 6 * x + 1. */
				bn_set_dig(p, 1);
				bn_mul_dig(t1, t0, 6);
				bn_add(p, p, t1);
				bn_mul(t1, t0, t0);
				bn_mul_dig(t1, t1, 24);
				bn_add(p, p, t1);
				bn_mul(t1, t0, t0);
				bn_mul(t1, t1, t0);
				bn_mul_dig(t1, t1, 36);
				bn_add(p, p, t1);
				bn_mul(t0, t0, t0);
				bn_mul(t1, t0, t0);
				bn_mul_dig(t1, t1, 36);
				bn_add(p, p, t1);
				fp_prime_set_dense(p);
				break;
#elif FP_PRIME == 160
			case SECG_160:
				/* p = 2^160 - 2^31 + 1. */
				f[0] = -1;
				f[1] = -31;
				f[2] = 160;
				fp_prime_set_pmers(f, 3);
				break;
			case SECG_160D:
				/* p = 2^160 - 2^32 - 2^14 - 2^12 - 2^9 - 2^8 - 2^7 - 2^3 - 2^2 - 1.*/
				f[0] = -1;
				f[1] = -2;
				f[2] = -3;
				f[3] = -7;
				f[4] = -8;
				f[5] = -9;
				f[6] = -12;
				f[7] = -14;
				f[8] = -32;
				f[9] = 160;
				fp_prime_set_pmers(f, 10);
				break;
#elif FP_PRIME == 192
			case NIST_192:
				/* p = 2^192 - 2^64 - 1. */
				f[0] = -1;
				f[1] = -64;
				f[2] = 192;
				fp_prime_set_pmers(f, 3);
				break;
			case SECG_192:
				/* p = 2^192 - 2^32 - 2^12 - 2^8 - 2^7 - 2^6 - 2^3 - 1.*/
				f[0] = -1;
				f[1] = -3;
				f[2] = -6;
				f[3] = -7;
				f[4] = -8;
				f[5] = -12;
				f[6] = -32;
				f[7] = 192;
				fp_prime_set_pmers(f, 8);
				break;
#elif FP_PRIME == 221
			case PRIME_22103:
				bn_set_2b(p, 221);
				bn_sub_dig(p, p, 3);
				fp_prime_set_dense(p);
				break;
#elif FP_PRIME == 224
			case NIST_224:
				/* p = 2^224 - 2^96 + 1. */
				f[0] = 1;
				f[1] = -96;
				f[2] = 224;
				fp_prime_set_pmers(f, 3);
				break;
			case SECG_224:
				/* p = 2^224 - 2^32 - 2^12 - 2^11 - 2^9 - 2^7 - 2^4 - 2 - 1.*/
				f[0] = -1;
				f[1] = -1;
				f[2] = -4;
				f[3] = -7;
				f[4] = -9;
				f[5] = -11;
				f[6] = -12;
				f[7] = -32;
				f[8] = 224;
				fp_prime_set_pmers(f, 9);
				break;
#elif FP_PRIME == 226
			case PRIME_22605:
				bn_set_2b(p, 226);
				bn_sub_dig(p, p, 5);
				fp_prime_set_dense(p);
				break;
#elif FP_PRIME == 251
			case PRIME_25109:
				bn_set_2b(p, 251);
				bn_sub_dig(p, p, 9);
				fp_prime_set_dense(p);
				break;
#elif FP_PRIME == 254
			case BN_254:
				/* x = -4080000000000001. */
				fp_param_get_var(t0);
				/* p = 36 * x^4 + 36 * x^3 + 24 * x^2 + 6 * x + 1. */
				bn_set_dig(p, 1);
				bn_mul_dig(t1, t0, 6);
				bn_add(p, p, t1);
				bn_mul(t1, t0, t0);
				bn_mul_dig(t1, t1, 24);
				bn_add(p, p, t1);
				bn_mul(t1, t0, t0);
				bn_mul(t1, t1, t0);
				bn_mul_dig(t1, t1, 36);
				bn_add(p, p, t1);
				bn_mul(t0, t0, t0);
				bn_mul(t1, t0, t0);
				bn_mul_dig(t1, t1, 36);
				bn_add(p, p, t1);
				fp_prime_set_dense(p);
				break;
#elif FP_PRIME == 255
			case PRIME_25519:
				bn_set_2b(p, 255);
				bn_sub_dig(p, p, 19);
				fp_prime_set_dense(p);
				break;
#elif FP_PRIME == 256
			case NIST_256:
				/* p = 2^256 - 2^224 + 2^192 + 2^96 - 1. */
				f[0] = -1;
				f[1] = 96;
				f[2] = 192;
				f[3] = -224;
				f[4] = 256;
				fp_prime_set_pmers(f, 5);
				break;
			case BSI_256:
				bn_read_str(p, BSI_P256, strlen(BSI_P256), 16);
				fp_prime_set_dense(p);
				break;
			case SECG_256:
				/* p = 2^256 - 2^32 - 2^9 - 2^8 - 2^7 - 2^6 - 2^4 - 1. */
				f[0] = -1;
				f[1] = -4;
				f[2] = -6;
				f[3] = -7;
				f[4] = -8;
				f[5] = -9;
				f[6] = -32;
				f[7] = 256;
				fp_prime_set_pmers(f, 8);
				break;
			case BN_256:
				/* x = -0x600000000000219B. */
				fp_param_get_var(t0);
				/* p = 36 * x^4 + 36 * x^3 + 24 * x^2 + 6 * x + 1. */
				bn_set_dig(p, 1);
				bn_mul_dig(t1, t0, 6);
				bn_add(p, p, t1);
				bn_mul(t1, t0, t0);
				bn_mul_dig(t1, t1, 24);
				bn_add(p, p, t1);
				bn_mul(t1, t0, t0);
				bn_mul(t1, t1, t0);
				bn_mul_dig(t1, t1, 36);
				bn_add(p, p, t1);
				bn_mul(t0, t0, t0);
				bn_mul(t1, t0, t0);
				bn_mul_dig(t1, t1, 36);
				bn_add(p, p, t1);
				fp_prime_set_dense(p);
				break;
#elif FP_PRIME == 381
			case B12_381:
				fp_param_get_var(t0);
				/* p = (x^2 - 2x + 1) * (x^4 - x^2 + 1)/3 + x. */
				bn_sqr(t1, t0);
				bn_sqr(p, t1);
				bn_sub(p, p, t1);
				bn_add_dig(p, p, 1);
				bn_sub(t1, t1, t0);
				bn_sub(t1, t1, t0);
				bn_add_dig(t1, t1, 1);
				bn_mul(p, p, t1);
				bn_div_dig(p, p, 3);
				bn_add(p, p, t0);
				fp_prime_set_dense(p);
				break;
#elif FP_PRIME == 382
			case PRIME_382105:
				bn_set_2b(p, 382);
				bn_sub_dig(p, p, 105);
				fp_prime_set_dense(p);
				break;
			case BN_382:
				/* x = -400040090001000000000001. */
				fp_param_get_var(t0);
				/* p = 36 * x^4 + 36 * x^3 + 24 * x^2 + 6 * x + 1. */
				bn_set_dig(p, 1);
				bn_mul_dig(t1, t0, 6);
				bn_add(p, p, t1);
				bn_mul(t1, t0, t0);
				bn_mul_dig(t1, t1, 24);
				bn_add(p, p, t1);
				bn_mul(t1, t0, t0);
				bn_mul(t1, t1, t0);
				bn_mul_dig(t1, t1, 36);
				bn_add(p, p, t1);
				bn_mul(t0, t0, t0);
				bn_mul(t1, t0, t0);
				bn_mul_dig(t1, t1, 36);
				bn_add(p, p, t1);
				fp_prime_set_dense(p);
				break;
#elif FP_PRIME == 383
			case PRIME_383187:
				bn_set_2b(p, 383);
				bn_sub_dig(p, p, 187);
				fp_prime_set_dense(p);
				break;
#elif FP_PRIME == 384
			case NIST_384:
				/* p = 2^384 - 2^128 - 2^96 + 2^32 - 1. */
				f[0] = -1;
				f[1] = 32;
				f[2] = -96;
				f[3] = -128;
				f[4] = 384;
				fp_prime_set_pmers(f, 5);
				break;
#elif FP_PRIME == 455
			case B12_455:
				fp_param_get_var(t0);
				/* p = (x^2 - 2x + 1) * (x^4 - x^2 + 1)/3 + x. */
				bn_sqr(t1, t0);
				bn_sqr(p, t1);
				bn_sub(p, p, t1);
				bn_add_dig(p, p, 1);
				bn_sub(t1, t1, t0);
				bn_sub(t1, t1, t0);
				bn_add_dig(t1, t1, 1);
				bn_mul(p, p, t1);
				bn_div_dig(p, p, 3);
				bn_add(p, p, t0);
				fp_prime_set_dense(p);
				break;
#elif FP_PRIME == 477
			case B24_477:
				fp_param_get_var(t0);
				/* p = (u - 1)^2 * (u^8 - u^4 + 1) div 3 + u. */
				bn_sub_dig(p, t0, 1);
				bn_sqr(p, p);
				bn_sqr(t1, t0);
				bn_sqr(t1, t1);
				bn_sqr(t2, t1);
				bn_sub(t2, t2, t1);
				bn_add_dig(t2, t2, 1);
				bn_mul(p, p, t2);
				bn_div_dig(p, p, 3);
				bn_add(p, p, t0);
				fp_prime_set_dense(p);
				break;
#elif FP_PRIME == 508
			case KSS_508:
				fp_param_get_var(t0);
				/* h = (49*u^2 + 245 * u + 343)/3 */
				bn_mul_dig(p, t0, 245);
				bn_add_dig(p, p, 200);
				bn_add_dig(p, p, 143);
				bn_sqr(t1, t0);
				bn_mul_dig(t2, t1, 49);
				bn_add(p, p, t2);
				bn_div_dig(p, p, 3);
				/* n = (u^6 + 37 * u^3 + 343)/343. */
				bn_mul(t1, t1, t0);
				bn_mul_dig(t2, t1, 37);
				bn_sqr(t1, t1);
				bn_add(t2, t2, t1);
				bn_add_dig(t2, t2, 200);
				bn_add_dig(t2, t2, 143);
				bn_div_dig(t2, t2, 49);
				bn_div_dig(t2, t2, 7);
				bn_mul(p, p, t2);
				/* t = (u^4 + 16 * u + 7)/7. */
				bn_mul_dig(t1, t0, 16);
				bn_add_dig(t1, t1, 7);
				bn_sqr(t2, t0);
				bn_sqr(t2, t2);
				bn_add(t2, t2, t1);
				bn_div_dig(t2, t2, 7);
				bn_add(p, p, t2);
				bn_sub_dig(p, p, 1);
				fp_prime_set_dense(p);
				break;
#elif FP_PRIME == 511
			case PRIME_511187:
				bn_set_2b(p, 511);
				bn_sub_dig(p, p, 187);
				fp_prime_set_dense(p);
				break;
#elif FP_PRIME == 521
			case NIST_521:
				/* p = 2^521 - 1. */
				f[0] = -1;
				f[1] = 521;
				fp_prime_set_pmers(f, 2);
				break;
#elif FP_PRIME == 638
			case BN_638:
				fp_param_get_var(t0);
				/* p = 36 * x^4 + 36 * x^3 + 24 * x^2 + 6 * x + 1. */
				bn_set_dig(p, 1);
				bn_mul_dig(t1, t0, 6);
				bn_add(p, p, t1);
				bn_mul(t1, t0, t0);
				bn_mul_dig(t1, t1, 24);
				bn_add(p, p, t1);
				bn_mul(t1, t0, t0);
				bn_mul(t1, t1, t0);
				bn_mul_dig(t1, t1, 36);
				bn_add(p, p, t1);
				bn_mul(t0, t0, t0);
				bn_mul(t1, t0, t0);
				bn_mul_dig(t1, t1, 36);
				bn_add(p, p, t1);
				fp_prime_set_dense(p);
				break;
			case B12_638:
				fp_param_get_var(t0);
				/* p = (x^2 - 2x + 1) * (x^4 - x^2 + 1)/3 + x. */
				bn_sqr(t1, t0);
				bn_sqr(p, t1);
				bn_sub(p, p, t1);
				bn_add_dig(p, p, 1);
				bn_sub(t1, t1, t0);
				bn_sub(t1, t1, t0);
				bn_add_dig(t1, t1, 1);
				bn_mul(p, p, t1);
				bn_div_dig(p, p, 3);
				bn_add(p, p, t0);
				fp_prime_set_dense(p);
				break;
#elif FP_PRIME == 1536
			case SS_1536:
				fp_param_get_var(t0);
				bn_read_str(p, SS_P1536, strlen(SS_P1536), 16);
				bn_mul(p, p, t0);
				bn_dbl(p, p);
				bn_sub_dig(p, p, 1);
				fp_prime_set_dense(p);
				break;
#else
			default:
				fp_param_set_any_dense();
				core_get()->fp_id = 0;
				break;
#endif
		}
	}
	CATCH_ANY {
		THROW(ERR_CAUGHT);
	}
	FINALLY {
		bn_free(t0);
		bn_free(t1);
		bn_free(t2);
		bn_free(p);
	}
}

int fp_param_set_any(void) {
#if FP_PRIME == 158
	fp_param_set(BN_158);
#elif FP_PRIME == 160
	fp_param_set(SECG_160);
#elif FP_PRIME == 192
	fp_param_set(NIST_192);
#elif FP_PRIME == 221
	fp_param_set(PRIME_22103);
#elif FP_PRIME == 224
	fp_param_set(NIST_224);
#elif FP_PRIME == 226
	fp_param_set(PRIME_22605);
#elif FP_PRIME == 254
	fp_param_set(BN_254);
#elif FP_PRIME == 251
	fp_param_set(PRIME_25109);
#elif FP_PRIME == 255
	fp_param_set(PRIME_25519);
#elif FP_PRIME == 256
#ifdef FP_PMERS
	fp_param_set(SECG_256);
#else
	fp_param_set(BN_256);
#endif
#elif FP_PRIME == 381
	fp_param_set(B12_381);
#elif FP_PRIME == 382
	fp_param_set(BN_382);
#elif FP_PRIME == 383
	fp_param_set(PRIME_383187);
#elif FP_PRIME == 384
	fp_param_set(NIST_384);
#elif FP_PRIME == 455
	fp_param_set(B12_455);
#elif FP_PRIME == 477
	fp_param_set(B24_477);
#elif FP_PRIME == 508
	fp_param_set(KSS_508);
#elif FP_PRIME == 511
	fp_param_set(PRIME_511187);
#elif FP_PRIME == 521
	fp_param_set(NIST_521);
#elif FP_PRIME == 638
	fp_param_set(B12_638);
#elif FP_PRIME == 1536
	fp_param_set(SS_1536);
#else
	return fp_param_set_any_dense();
#endif
	return STS_OK;
}

int fp_param_set_any_dense(void) {
	bn_t p;
	int result = STS_OK;

	bn_null(p);

	TRY {
		bn_new(p);
#ifdef FP_QNRES
		do {
			bn_gen_prime(p, FP_BITS);
		} while ((p->dp[0] & 0x7) != 3);
#else
		bn_gen_prime(p, FP_BITS);
#endif
		if (!bn_is_prime(p)) {
			result = STS_ERR;
		} else {
			fp_prime_set_dense(p);
		}
	}
	CATCH_ANY {
		THROW(ERR_CAUGHT);
	}
	FINALLY {
		bn_free(p);
	}
	return result;
}

int fp_param_set_any_pmers(void) {
#if FP_PRIME == 160
	fp_param_set(SECG_160);
#elif FP_PRIME == 192
	fp_param_set(NIST_192);
#elif FP_PRIME == 224
	fp_param_set(NIST_224);
#elif FP_PRIME == 256
	fp_param_set(NIST_256);
#elif FP_PRIME == 384
	fp_param_set(NIST_384);
#elif FP_PRIME == 521
	fp_param_set(NIST_521);
#else
	return STS_ERR;
#endif
	return STS_OK;
}

int fp_param_set_any_tower(void) {
#if FP_PRIME == 158
	fp_param_set(BN_158);
#elif FP_PRIME == 254
	fp_param_set(BN_254);
#elif FP_PRIME == 256
	fp_param_set(BN_256);
#elif FP_PRIME == 381
	fp_param_set(B12_381);
#elif FP_PRIME == 382
	fp_param_set(BN_382);
#elif FP_PRIME == 455
	fp_param_set(B12_455);
#elif FP_PRIME == 477
	fp_param_set(B24_477);
#elif FP_PRIME == 508
	fp_param_set(KSS_508);
#elif FP_PRIME == 638
	fp_param_set(B12_638);
#elif FP_PRIME == 1536
	fp_param_set(SS_1536);
#else
	do {
		/* Since we have to generate a prime number, pick a nice towering. */
		fp_param_set_any_dense();
	} while (fp_prime_get_mod8() == 1 || fp_prime_get_mod8() == 5);
#endif

	return STS_OK;
}

void fp_param_print(void) {
	util_banner("Prime modulus:", 0);
	util_print("   ");
#if ALLOC == AUTO
	fp_print(fp_prime_get());
#else
	fp_print((const fp_t)fp_prime_get());
#endif
}
