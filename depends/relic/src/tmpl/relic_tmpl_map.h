/*
 * RELIC is an Efficient LIbrary for Cryptography
 * Copyright (c) 2020 RELIC Authors
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
 * Templates for hashing to elliptic curves.
 *
 * @ingroup tmpl
 */

/*============================================================================*/
/* Private definitions                                                        */
/*============================================================================*/

/**
 * Evaluate a polynomial represented by its coefficients over a using Horner's
 * rule. Might promove to an API if needed elsewhere in the future.
 */
#define TMPL_MAP_HORNER(PFX, IN)											\
	static void PFX##_eval(PFX##_t c, PFX##_t a, IN *coeffs, int deg) {		\
		PFX##_copy(c, coeffs[deg]);											\
		for (int i = deg; i > 0; --i) {										\
			PFX##_mul(c, c, a);												\
			PFX##_add(c, c, coeffs[i - 1]);									\
		}																	\
	}

/* TODO: remove the ugly hack due to lack of support for JACOB in ep2. */
/* conditionally normalize result of isogeny map when not using projective coords */
#if EP_ADD == JACOB
#define TMPL_MAP_ISOMAP_NORM(PFX)											\
	do {																	\
		/* Y = Ny * Dx * Z^2. */											\
		PFX##_mul(q->y, p->y, t1);											\
		PFX##_mul(q->y, q->y, t3);											\
		/* Z = Dx * Dy, t1 = Z^2. */										\
		PFX##_mul(q->z, t2, t3);											\
		PFX##_sqr(t1, q->z);												\
		PFX##_mul(q->y, q->y, t1);											\
		/* X = Nx * Dy * Z. */												\
		PFX##_mul(q->x, t0, t2);											\
		PFX##_mul(q->x, q->x, q->z);										\
		q->coord = JACOB;													\
	} while (0)
#elif EP_ADD == PROJC
#define TMPL_MAP_ISOMAP_NORM(PFX)											\
	if (#PFX[2] == '2') {													\
		/* Y = Ny * Dx * Z^2. */											\
		PFX##_mul(q->y, p->y, t1);											\
		PFX##_mul(q->y, q->y, t3);											\
		/* Z = Dx * Dy, t1 = Z^2. */										\
		PFX##_mul(q->z, t2, t3);											\
		PFX##_sqr(t1, q->z);												\
		PFX##_mul(q->y, q->y, t1);											\
		/* X = Nx * Dy * Z. */												\
		PFX##_mul(q->x, t0, t2);											\
		PFX##_mul(q->x, q->x, q->z);										\
		q->coord = PROJC;													\
	} else {																\
		/* Z = Dx * Dy. */													\
		PFX##_mul(q->z, t2, t3);											\
		/* X = Nx * Dy. */													\
		PFX##_mul(q->x, t0, t2);											\
		/* Y = Ny * Dx. */													\
		PFX##_mul(q->y, p->y, t1);											\
		PFX##_mul(q->y, q->y, t3);											\
		q->coord = PROJC;													\
	}
#else
#define TMPL_MAP_ISOMAP_NORM(PFX)											\
	do {																	\
		/* when working with affine coordinates, clear denominator */		\
		PFX##_mul(q->z, t2, t3);											\
		PFX##_inv(q->z, q->z);												\
		/* y coord */														\
		PFX##_mul(q->y, p->y, q->z);										\
		PFX##_mul(q->y, q->y, t3);											\
		PFX##_mul(q->y, q->y, t1);											\
		/* x coord */														\
		PFX##_mul(q->x, t2, q->z);											\
		PFX##_mul(q->x, q->x, t0);											\
		/* z coord == 1 */													\
		PFX##_set_dig(q->z, 1);												\
		q->coord = BASIC;													\
	} while (0)
#endif

/**
 * Generic isogeny map evaluation for use with SSWU map.
 */
#define TMPL_MAP_ISOGENY_MAP(CUR, PFX, ISO)									\
	/* declaring this function inline suppresses unused warnings */			\
	static inline void CUR##_iso(CUR##_t q, CUR##_t p) {					\
		PFX##_t t0, t1, t2, t3;												\
																			\
		if (!CUR##_curve_is_ctmap()) {										\
			CUR##_copy(q, p);												\
			return;															\
		}																	\
		/* XXX need to add real support for input projective points */		\
		if (p->coord != BASIC) {											\
			CUR##_norm(p, p);												\
		}																	\
																			\
		PFX##_null(t0);														\
		PFX##_null(t1);														\
		PFX##_null(t2);														\
		PFX##_null(t3);														\
                                                                    		\
		RLC_TRY {															\
			PFX##_new(t0);													\
			PFX##_new(t1);													\
			PFX##_new(t2);													\
			PFX##_new(t3);													\
																			\
			ISO##_t coeffs = CUR##_curve_get_iso();							\
																			\
			/* numerators */												\
			PFX##_eval(t0, p->x, coeffs->xn, coeffs->deg_xn);				\
			PFX##_eval(t1, p->x, coeffs->yn, coeffs->deg_yn);				\
			/* denominators */												\
			PFX##_eval(t2, p->x, coeffs->yd, coeffs->deg_yd);				\
			PFX##_eval(t3, p->x, coeffs->xd, coeffs->deg_xd);				\
																			\
			/* normalize if necessary */									\
			TMPL_MAP_ISOMAP_NORM(PFX);										\
		}																	\
		RLC_CATCH_ANY { RLC_THROW(ERR_CAUGHT); }							\
		RLC_FINALLY {														\
			PFX##_free(t0);													\
			PFX##_free(t1);													\
			PFX##_free(t2);													\
			PFX##_free(t3);													\
		}																	\
	}

/* Conditionally call isogeny mapping function depending on whether EP_CTMAP is defined */
#ifdef EP_CTMAP
#define TMPL_MAP_CALL_ISOMAP(CUR, PT)										\
	do {																	\
		if (CUR##_curve_is_ctmap()) {										\
			CUR##_iso(PT, PT);												\
		}																	\
	} while (0)
#else
#define TMPL_MAP_CALL_ISOMAP(CUR, PT) /* No isogeny map call in this case. */
#endif

/**
 * Simplified SWU mapping from Section 4 of
 * "Fast and simple constant-time hashing to the BLS12-381 Elliptic Curve"
 */
#define TMPL_MAP_SSWU(CUR, PFX, PTR_TY, COPY_COND)							\
	static void CUR##_map_sswu(CUR##_t p, PFX##_t t) {						\
		PFX##_t t0, t1, t2, t3;												\
		ctx_t *ctx = core_get();											\
		PTR_TY *mBoverA = ctx->CUR##_map_c[0];								\
		PTR_TY *a = ctx->CUR##_map_c[2];									\
		PTR_TY *b = ctx->CUR##_map_c[3];									\
		PTR_TY *u = ctx->CUR##_map_u;										\
                                                                            \
		PFX##_null(t0);														\
		PFX##_null(t1);														\
		PFX##_null(t2);														\
		PFX##_null(t3);														\
																			\
		RLC_TRY {															\
			PFX##_new(t0);													\
			PFX##_new(t1);													\
			PFX##_new(t2);													\
			PFX##_new(t3);													\
																			\
			/* start computing the map */									\
			PFX##_sqr(t0, t);												\
			PFX##_mul(t0, t0, u);  /* t0 = u * t^2 */						\
			PFX##_sqr(t1, t0);     /* t1 = u^2 * t^4 */						\
			PFX##_add(t2, t1, t0); /* t2 = u^2 * t^4 + u * t^2 */			\
																			\
			/* handle the exceptional cases */												\
			/* XXX(rsw) should be done projectively */										\
			{																				\
				const int e1 = PFX##_is_zero(t2);											\
				PFX##_neg(t3, u);         /* t3 = -u */										\
				COPY_COND(t2, t3, e1);        /* exception: -u instead of u^2t^4 + ut^2 */	\
				PFX##_inv(t2, t2);        /* t2 = -1/u or 1/(u^2 * t^4 + u*t^2) */			\
				PFX##_add_dig(t3, t2, 1); /* t3 = 1 + t2 */									\
				COPY_COND(t2, t3, e1 == 0);      /* only add 1 if t2 != -1/u */				\
			}																				\
			/* e1 goes out of scope */														\
                                                                                			\
			/* compute x1, g(x1) */															\
			PFX##_mul(p->x, t2, mBoverA); /* -B / A * (1 + 1 / (u^2 * t^4 + u * t^2)) */	\
			PFX##_sqr(p->y, p->x);        /* x^2 */											\
			PFX##_add(p->y, p->y, a);     /* x^2 + a */										\
			PFX##_mul(p->y, p->y, p->x);  /* x^3 + a x */									\
			PFX##_add(p->y, p->y, b);     /* x^3 + a x + b */								\
                                                                                    		\
			/* compute x2, g(x2) */															\
			PFX##_mul(t2, t0, p->x); /* t2 = u * t^2 * x1 */								\
			PFX##_mul(t1, t0, t1);   /* t1 = u^3 * t^6 */									\
			PFX##_mul(t3, t1, p->y); /* t5 = g(t2) = u^3 * t^6 * g(p->x) */					\
																							\
			/* XXX(rsw)                                                               */	\
			/* This should be done in constant time and without computing 2 sqrts.    */	\
			/* Avoiding a second sqrt relies on knowing the 2-adicity of the modulus. */	\
			if (!PFX##_srt(p->y, p->y)) {													\
				/* try x2, g(x2) */															\
				PFX##_copy(p->x, t2);														\
				if (!PFX##_srt(p->y, t3)) {													\
					RLC_THROW(ERR_NO_VALID);												\
				}																			\
			}																				\
			PFX##_set_dig(p->z, 1);											\
			p->coord = BASIC;												\
		}																	\
		RLC_CATCH_ANY { RLC_THROW(ERR_CAUGHT); }							\
		RLC_FINALLY {														\
			PFX##_free(t0);													\
			PFX##_free(t1);													\
			PFX##_free(t2);													\
			PFX##_free(t3);													\
		}																	\
	}

/**
 * Shallue--van de Woestijne map, based on the definition from
 * draft-irtf-cfrg-hash-to-curve-06, Section 6.6.1
 */
#define TMPL_MAP_SVDW(CUR, PFX, PTR_TY, COPY_COND)							\
	static void CUR##_map_svdw(CUR##_t p, PFX##_t t) {						\
		PFX##_t t1, t2, t3, t4;												\
		PFX##_null(t1);														\
		PFX##_null(t2);														\
		PFX##_null(t3);														\
		PFX##_null(t4);														\
                                                                            \
		RLC_TRY {															\
			PFX##_new(t1);													\
			PFX##_new(t2);													\
			PFX##_new(t3);													\
			PFX##_new(t4);													\
																			\
			ctx_t *ctx = core_get();										\
			PTR_TY *gU = ctx->CUR##_map_c[0];								\
			PTR_TY *mUover2 = ctx->CUR##_map_c[1];							\
			PTR_TY *c3 = ctx->CUR##_map_c[2];								\
			PTR_TY *c4 = ctx->CUR##_map_c[3];								\
			PTR_TY *u = ctx->CUR##_map_u;									\
                                                                            \
			/* start computing the map */									\
			PFX##_sqr(t1, t);												\
			PFX##_mul(t1, t1, gU);											\
			PFX##_add_dig(t2, t1, 1); /* 1 + t^2 * g(u) */					\
			PFX##_sub_dig(t1, t1, 1);										\
			PFX##_neg(t1, t1);     /* 1 - t^2 * g(u) */						\
			PFX##_mul(t3, t1, t2); /* (1+t^2*g(u)) * (1-t^2*g(u)) */		\
                                                                            \
			/* handle exceptional case */									\
			{																\
				/* compute inv0(t3), i.e., 0 if t3 == 0, 1/t3 otherwise */	\
				const int e0 = PFX##_is_zero(t3);							\
				COPY_COND(t3, gU, e0); /* g(u) is nonzero */				\
				PFX##_inv(t3, t3);											\
				PFX##_zero(t4);												\
				COPY_COND(t3, t4, e0);										\
			}																\
			/* e0 goes out of scope */										\
			PFX##_mul(t4, t, t1);											\
			PFX##_mul(t4, t4, t3);											\
			PFX##_mul(t4, t4, c3);											\
																			\
			/* XXX(rsw) this should be constant time */						\
			/* compute x1 and g(x1) */										\
			PFX##_sub(p->x, mUover2, t4);									\
			CUR##_rhs(p->y, p);												\
			if (!PFX##_srt(p->y, p->y)) {									\
				/* compute x2 and g(x2) */									\
				PFX##_add(p->x, mUover2, t4);								\
				CUR##_rhs(p->y, p);											\
				if (!PFX##_srt(p->y, p->y)) {								\
					/* compute x3 and g(x3) */								\
					PFX##_sqr(p->x, t2);									\
					PFX##_mul(p->x, p->x, t3);								\
					PFX##_sqr(p->x, p->x);									\
					PFX##_mul(p->x, p->x, c4);								\
					PFX##_add(p->x, p->x, u);								\
					CUR##_rhs(p->y, p);										\
					if (!PFX##_srt(p->y, p->y)) {							\
						RLC_THROW(ERR_NO_VALID);							\
					}														\
				}															\
			}																\
			PFX##_set_dig(p->z, 1);											\
			p->coord = BASIC;												\
		}																	\
		RLC_CATCH_ANY { RLC_THROW(ERR_CAUGHT); }							\
		RLC_FINALLY {														\
			PFX##_free(t1);													\
			PFX##_free(t2);													\
			PFX##_free(t3);													\
			PFX##_free(t4);													\
		}																	\
	}
