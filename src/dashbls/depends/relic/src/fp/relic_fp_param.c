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
 * Implementation of the prime field modulus manipulation.
 *
 * @ingroup fp
 */

#include "relic_core.h"
#include "relic_fpx.h"

/*============================================================================*/
/* Private definitions                                                        */
/*============================================================================*/

#if FP_PRIME == 255

/**
 * Primes with high 2-adicity for curves Tweedledum and Tweedledee.
 */
#define STR_H2ADC	"38AA1276C3F59B9A14064E2"

#elif FP_PRIME == 256
/**
 * Random prime modulus for the Brainpool P256r1.
 */
#define STR_P256	"A9FB57DBA1EEA9BC3E660A909D838D726E3BF623D52620282013481D1F6E5377"

#elif FP_PRIME == 544
/**
 * Random prime modulus for the Cocks-Pinch curve of embedding degree 8.
 */
#define STR_P544	"BB9DFD549299F1C803DDD5D7C05E7CC0373D9B1AC15B47AA5AA84626F33E58FE66943943049031AE4CA1D2719B3A84FA363BCD2539A5CD02C6F4B6B645A58C1085E14411"

#elif FP_PRIME == 1536
/**
 * Cofactor description of 1536-bit prime modulus.
 */
#define STR_P1536	"83093742908D4D529CEF06C72191A05D5E6073FE861E637D7747C3E52FBB92DAA5DDF3EF1C61F5F70B256802481A36CAFE995FE33CD54014B846751364C0D3B8327D9E45366EA08F1B3446AC23C9D4B656886731A8D05618CFA1A3B202A2445ABA0E77C5F4F00CA1239975A05377084F256DEAA07D21C4CF2A4279BC117603ACB7B10228C3AB8F8C1742D674395701BB02071A88683041D9C4231E8EE982B8DA"

#elif FP_PRIME == 3072

/**
 * Cofactor description of 3072-bit prime modulus.
 */
#define STR_P3072	"D2DECB54BA1C33C49BD7B174FBC6E56AD9B05D1B2B4100708AA3B6AA620B5CD167F66D33818B608FC25C9F56A0C685129B9DDAA342ADB37B2CEA7E09DEB49F3BC85B467801ED7507149CF025A021D8109926CAC3C774AD8E3C0B4D175CF1C94A6641A8E5BBBE63FB40EC52B206315D20C074B538785E63174FB8A72C912F51B6A3D132A185FD95B067B817AB9F4C01ABA6CC7BF1546D94FD79AC2D0183814FD8"

#endif

/*============================================================================*/
/* Public definitions                                                         */
/*============================================================================*/

int fp_param_get(void) {
	return core_get()->fp_id;
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

	RLC_TRY {
		bn_new(t0);
		bn_new(t1);
		bn_new(t2);
		bn_new(p);

		core_get()->fp_id = param;

		switch (param) {
#if FP_PRIME == 158
			case BN_158:
				/* x = 0x4000000031. */
				bn_set_2b(t0, 38);
				bn_add_dig(t0, t0, 0x31);
				fp_prime_set_pairf(t0, EP_BN);
				break;
#elif FP_PRIME == 160
			case SECG_160:
				/* p = 2^160 - 2^31 - 1. */
				f[0] = -1;
				f[1] = -31;
				f[2] = 160;
				fp_prime_set_pmers(f, 3);
				break;
			case SECG_160D:
				/* p = 2^160 - 2^32 - 2^14 - 2^12 - 2^9 - 2^8 - 2^7 - 2^3 - 2^2 - 1. */
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
				/* p = 2^192 - 2^32 - 2^12 - 2^8 - 2^7 - 2^6 - 2^3 - 1. */
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
				/* x = -(2^62 + 2^55 + 1). */
				bn_set_2b(t0, 62);
				bn_set_bit(t0, 55, 1);
				bn_add_dig(t0, t0, 1);
				bn_neg(t0, t0);
				fp_prime_set_pairf(t0, EP_BN);
				break;
#elif FP_PRIME == 255
			case PRIME_25519:
				bn_set_2b(p, 255);
				bn_sub_dig(p, p, 19);
				fp_prime_set_dense(p);
				break;
			case PRIME_H2ADC:
				bn_set_2b(p, 222);
				bn_read_str(t0, STR_H2ADC, strlen(STR_H2ADC), 16);
				bn_add(p, p, t0);
				bn_lsh(p, p, 32);
				bn_add_dig(p, p, 1);
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
				bn_read_str(p, STR_P256, strlen(STR_P256), 16);
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
				bn_set_2b(t0, 62);
				bn_set_bit(t0, 61, 1);
				bn_set_dig(t1, 0x21);
				bn_lsh(t1, t1, 8);
				bn_add(t0, t0, t1);
				bn_add_dig(t0, t0, 0x9B);
				bn_neg(t0, t0);
				fp_prime_set_pairf(t0, EP_BN);
				break;
			case SM9_256:
				/* x = 0x600000000058F98A */
				bn_set_2b(t0, 62);
				bn_set_bit(t0, 61, 1);
				bn_set_dig(t1, 0x58);
				bn_lsh(t1, t1, 8);
				bn_add_dig(t1, t1, 0xF9);
				bn_lsh(t1, t1, 8);
				bn_add(t0, t0, t1);
				bn_add_dig(t0, t0, 0x8A);
				fp_prime_set_pairf(t0, EP_BN);
				break;
#elif FP_PRIME == 381
			case B12_381:
				/* x = -(2^63 + 2^62 + 2^60 + 2^57 + 2^48 + 2^16). */
				bn_set_2b(t0, 63);
				bn_set_bit(t0, 62, 1);
				bn_set_bit(t0, 60, 1);
				bn_set_bit(t0, 57, 1);
				bn_set_bit(t0, 48, 1);
				bn_set_bit(t0, 16, 1);
				bn_neg(t0, t0);
				fp_prime_set_pairf(t0, EP_B12);
				break;
#elif FP_PRIME == 382
			case PRIME_382105:
				bn_set_2b(p, 382);
				bn_sub_dig(p, p, 105);
				fp_prime_set_dense(p);
				break;
			case BN_382:
				/* x = -(2^94 + 2^78 + 2^67 + 2^64 + 2^48 + 1). */
				bn_set_2b(t0, 94);
				bn_set_bit(t0, 78, 1);
				bn_set_bit(t0, 67, 1);
				bn_set_bit(t0, 64, 1);
				bn_set_bit(t0, 48, 1);
				bn_add_dig(t0, t0, 1);
				bn_neg(t0, t0);
				fp_prime_set_pairf(t0, EP_BN);
				break;
#elif FP_PRIME == 383
			case PRIME_383187:
				bn_set_2b(p, 383);
				bn_sub_dig(p, p, 187);
				fp_prime_set_dense(p);
				break;
			case B12_383:
				/* x = 2^64 + 2^51 + 2^24 + 2^12 + 2^9 */
				bn_set_2b(t0, 64);
				bn_set_bit(t0, 51, 1);
				bn_set_bit(t0, 24, 1);
				bn_set_bit(t0, 12, 1);
				bn_set_bit(t0, 9, 1);
				fp_prime_set_pairf(t0, EP_B12);
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
#elif FP_PRIME == 446
			case BN_446:
				/* x = 2^110 + 2^36 + 1. */
				bn_set_2b(t0, 110);
				bn_set_bit(t0, 36, 1);
				bn_add_dig(t0, t0, 1);
				fp_prime_set_pairf(t0, EP_BN);
				break;
			case B12_446:
				/* x = -(2^75 - 2^73 + 2^63 + 2^57 + 2^50 + 2^17 + 1). */
				bn_set_2b(t0, 75);
				bn_set_bit(t0, 63, 1);
				bn_set_bit(t0, 57, 1);
				bn_set_bit(t0, 50, 1);
				bn_set_bit(t0, 17, 1);
				bn_add_dig(t0, t0, 1);
				bn_set_2b(t1, 73);
				bn_sub(t0, t0, t1);
				bn_neg(t0, t0);
				fp_prime_set_pairf(t0, EP_B12);
				break;
#elif FP_PRIME == 448
			case PRIME_448:
				/* p = 2^448 - 2^224 + 1. */
				f[0] = -1;
				f[1] = -224;
				f[2] = 448;
				fp_prime_set_pmers(f, 3);
				break;
#elif FP_PRIME == 455
			case B12_455:
				/* x = 2^76 + 2^53 + 2^31 + 2^11. */
				bn_set_2b(t0, 76);
				bn_set_bit(t0, 53, 1);
				bn_set_bit(t0, 31, 1);
				bn_set_bit(t0, 11, 1);
				fp_prime_set_pairf(t0, EP_B12);
				break;
#elif FP_PRIME == 508
			case KSS_508:
				/* x = -(2^64 + 2^51 - 2^46 - 2^12). */
				bn_set_2b(t0, 64);
				bn_set_2b(t1, 51);
				bn_add(t0, t0, t1);
				bn_set_2b(t1, 46);
				bn_sub(t0, t0, t1);
				bn_set_2b(t1, 12);
				bn_sub(t0, t0, t1);
				bn_neg(t0, t0);
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
#elif FP_PRIME == 509
			case B24_509:
				/* x = -2^51 - 2^28 + 2^11 - 1. */
				bn_set_2b(t0, 51);
				bn_set_2b(t1, 28);
				bn_add(t0, t0, t1);
				bn_set_2b(t1, 11);
				bn_sub(t0, t0, t1);
				bn_add_dig(t0, t0, 1);
				bn_neg(t0, t0);
				fp_prime_set_pairf(t0, EP_B24);
				break;
#elif FP_PRIME == 511
			case OT_511:
				bn_set_2b(t0, 52);
				bn_add_dig(t0, t0, 0xAB);
				bn_lsh(t0, t0, 12);
				fp_prime_set_pairf(t0, EP_OT8);
				break;
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
#elif FP_PRIME == 544
			case GMT8_544:
				bn_read_str(p, STR_P544, strlen(STR_P544), 16);
				/* T = 2^64 - 2^54 + 2^37 + 2^32 - 4 */
				bn_set_2b(t0, 64);
				bn_set_2b(t1, 54);
				bn_sub(t0, t0, t1);
				bn_set_2b(t1, 37);
				bn_add(t0, t0, t1);
				bn_set_2b(t1, 32);
				bn_add(t0, t0, t1);
				bn_sub_dig(t0, t0, 4);
				fp_prime_set_pairf(t0, EP_GMT8);
				fp_prime_set_dense(p);
				break;
#elif FP_PRIME == 569
			case K54_569:
				/* 2^27 + 2^26 + 2^22 + 2^14 + 2^6 + 2 */
				bn_set_2b(t0, 27);
				bn_set_2b(t1, 26);
				bn_add(t0, t0, t1);
				bn_set_2b(t1, 22);
				bn_add(t0, t0, t1);
				bn_set_2b(t1, 14);
				bn_add(t0, t0, t1);
				bn_add_dig(t0, t0, 66);
				fp_prime_set_pairf(t0, EP_K54);
				break;
#elif FP_PRIME == 575
			case B48_575:
				/* x = 2^32 - 2^18 - 2^10 - 2^4. */
				bn_set_2b(t0, 32);
				bn_set_2b(t1, 18);
				bn_sub(t0, t0, t1);
				bn_set_2b(t1, 10);
				bn_sub(t0, t0, t1);
				bn_sub_dig(t0, t0, 16);
				fp_prime_set_pairf(t0, EP_B48);
				break;
#elif FP_PRIME == 638
			case BN_638:
				/* x = 2^158 - 2^128 - 2^68 + 1. */
				bn_set_2b(t0, 158);
				bn_set_2b(t1, 128);
				bn_sub(t0, t0, t1);
				bn_set_2b(t1, 68);
				bn_sub(t0, t0, t1);
				bn_add_dig(t0, t0, 1);
				fp_prime_set_pairf(t0, EP_BN);
				break;
			case B12_638:
				/* x = -2^107 + 2^105 + 2^93 + 2^5. */
				bn_set_2b(t0, 107);
				bn_set_2b(t1, 105);
				bn_sub(t0, t0, t1);
				bn_set_2b(t1, 93);
				bn_sub(t0, t0, t1);
				bn_set_2b(t1, 5);
				bn_sub(t0, t0, t1);
				bn_neg(t0, t0);
				fp_prime_set_pairf(t0, EP_B12);
				break;
#elif FP_PRIME == 1536
			case SS_1536:
				/* x = 2^255 + 2^41 + 1. */
				bn_set_2b(t0, 255);
				bn_set_bit(t0, 41, 1);
				bn_add_dig(t0, t0, 1);
				bn_read_str(p, STR_P1536, strlen(STR_P1536), 16);
				bn_mul(p, p, t0);
				bn_dbl(p, p);
				bn_sub_dig(p, p, 1);
				fp_prime_set_dense(p);
				fp_prime_set_pairf(t0, EP_SS2);
				break;
#elif FP_PRIME == 3072
			case SS_3072:
				/* x = 2^256 + 2^96 - 1. */
				bn_set_2b(t0, 256);
				bn_set_bit(t0, 96, 1);
				bn_sub_dig(t0, t0, 1);
				bn_read_str(p, STR_P3072, strlen(STR_P3072), 16);
				bn_mul(p, p, t0);
				bn_sqr(p, p);
				bn_add_dig(p, p, 1);
				fp_prime_set_dense(p);
				fp_prime_set_pairf(t0, EP_SS1);
				break;
#else
			default:
				fp_param_set_any_dense();
				core_get()->fp_id = 0;
				break;
#endif
		}
	}
	RLC_CATCH_ANY {
		RLC_THROW(ERR_CAUGHT);
	}
	RLC_FINALLY {
		bn_free(t0);
		bn_free(t1);
		bn_free(t2);
		bn_free(p);
	}
}

int fp_param_set_any(void) {
	int r = fp_param_set_any_pmers();
	if (r == RLC_ERR) {
		r = fp_param_set_any_h2adc();
		if (r == RLC_ERR) {
			r = fp_param_set_any_tower();
			if (r == RLC_ERR) {
				r = fp_param_set_any_dense();
				if (r == RLC_ERR) {
					return RLC_ERR;
				}
			}
		}
	}
	return RLC_OK;
}

int fp_param_set_any_dense(void) {
	bn_t p;
	int result = RLC_OK;

	bn_null(p);

	RLC_TRY {
		bn_new(p);
#ifdef FP_QNRES
		do {
			bn_gen_prime(p, RLC_FP_BITS);
		} while ((p->dp[0] & 0x7) != 3);
#else
		bn_gen_prime(p, RLC_FP_BITS);
#endif
		if (!bn_is_prime(p)) {
			result = RLC_ERR;
		} else {
			fp_prime_set_dense(p);
		}
	}
	RLC_CATCH_ANY {
		RLC_THROW(ERR_CAUGHT);
	}
	RLC_FINALLY {
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
#elif FP_PRIME == 255
	fp_param_set(PRIME_25519);
#elif FP_PRIME == 256
	fp_param_set(NIST_256);
#elif FP_PRIME == 384
	fp_param_set(NIST_384);
#elif FP_PRIME == 448
	fp_param_set(PRIME_448);
#elif FP_PRIME == 521
	fp_param_set(NIST_521);
#else
	return RLC_ERR;
#endif
	return RLC_OK;
}

int fp_param_set_any_h2adc(void) {
#if FP_PRIME == 255
	fp_param_set(PRIME_H2ADC);
#else
	return RLC_ERR;
#endif
	return RLC_OK;
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
#elif FP_PRIME == 383
	fp_param_set(B12_383);
#elif FP_PRIME == 446
#ifdef FP_QNRES
	fp_param_set(B12_446);
#else
	fp_param_set(BN_446);
#endif
#elif FP_PRIME == 455
	fp_param_set(B12_455);
#elif FP_PRIME == 508
	fp_param_set(KSS_508);
#elif FP_PRIME == 509
	fp_param_set(B24_509);
#elif FP_PRIME == 511
	fp_param_set(OT_511);
#elif FP_PRIME == 544
	fp_param_set(GMT8_544);
#elif FP_PRIME == 569
	fp_param_set(K54_569);
#elif FP_PRIME == 575
	fp_param_set(B48_575);
#elif FP_PRIME == 638
#ifdef FP_QNRES
	fp_param_set(B12_638);
#else
	fp_param_set(BN_638);
#endif
#elif FP_PRIME == 1536
	fp_param_set(SS_1536);
#else
	do {
		/* Since we have to generate a prime number, pick a nice towering. */
		fp_param_set_any_dense();
	} while (fp_prime_get_mod8() == 1 || fp_prime_get_mod8() == 5);
#endif

	return RLC_OK;
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
