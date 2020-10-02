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
 * @defgroup pp Bilinear pairings over prime elliptic curves.
 */

/**
 * @file
 *
 * Interface of the module for computing bilinear pairings over prime elliptic
 * curves.
 *
 * @ingroup pp
 */

#ifndef RELIC_PP_H
#define RELIC_PP_H

#include "relic_fpx.h"
#include "relic_epx.h"
#include "relic_types.h"

/*============================================================================*/
/* Macro definitions                                                          */
/*============================================================================*/

/**
 * Adds two points and evaluates the corresponding line function at another
 * point on an elliptic curve with embedding degree 2.
 *
 * @param[out] L			- the result of the evaluation.
 * @param[in, out] R		- the resulting point and first point to add.
 * @param[in] P				- the second point to add.
 * @param[in] Q				- the affine point to evaluate the line function.
 */
#if EP_ADD == BASIC
#define pp_add_k2(L, R, P, Q)		pp_add_k2_basic(L, R, P, Q)
#elif EP_ADD == PROJC
#define pp_add_k2(L, R, P, Q)		pp_add_k2_projc(L, R, P, Q)
#endif

/**
 * Adds two points and evaluates the corresponding line function at another
 * point on an elliptic curve with embedding degree 2 using projective
 * coordinates.
 *
 * @param[out] L			- the result of the evaluation.
 * @param[in, out] R		- the resulting point and first point to add.
 * @param[in] P				- the second point to add.
 * @param[in] Q				- the affine point to evaluate the line function.
 */
#if PP_EXT == BASIC
#define pp_add_k2_projc(L, R, P, Q)		pp_add_k2_projc_basic(L, R, P, Q)
#elif PP_EXT == LAZYR
#define pp_add_k2_projc(L, R, P, Q)		pp_add_k2_projc_lazyr(L, R, P, Q)
#endif

/**
 * Adds two points and evaluates the corresponding line function at another
 * point on an elliptic curve with embedding degree 12.
 *
 * @param[out] L			- the result of the evaluation.
 * @param[in, out] R		- the resulting point and first point to add.
 * @param[in] Q				- the second point to add.
 * @param[in] P				- the affine point to evaluate the line function.
 */
#if EP_ADD == BASIC
#define pp_add_k12(L, R, Q, P)		pp_add_k12_basic(L, R, Q, P)
#elif EP_ADD == PROJC
#define pp_add_k12(L, R, Q, P)		pp_add_k12_projc(L, R, Q, P)
#endif

/**
 * Adds two points and evaluates the corresponding line function at another
 * point on an elliptic curve with embedding degree 12 using projective
 * coordinates.
 *
 * @param[out] L			- the result of the evaluation.
 * @param[in, out] R		- the resulting point and first point to add.
 * @param[in] Q				- the second point to add.
 * @param[in] P				- the affine point to evaluate the line function.
 */
#if PP_EXT == BASIC
#define pp_add_k12_projc(L, R, Q, P)	pp_add_k12_projc_basic(L, R, Q, P)
#elif PP_EXT == LAZYR
#define pp_add_k12_projc(L, R, Q, P)	pp_add_k12_projc_lazyr(L, R, Q, P)
#endif

/**
 * Doubles a point and evaluates the corresponding line function at another
 * point on an elliptic curve with embedding degree 2.
 *
 * @param[out] L			- the result of the evaluation.
 * @param[out] R			- the resulting point.
 * @param[in] P				- the point to double.
 * @param[in] Q				- the affine point to evaluate the line function.
 */
#if EP_ADD == BASIC
#define pp_dbl_k2(L, R, P, Q)			pp_dbl_k2_basic(L, R, P, Q)
#elif EP_ADD == PROJC
#define pp_dbl_k2(L, R, P, Q)			pp_dbl_k2_projc(L, R, P, Q)
#endif

/**
 * Doubles a point and evaluates the corresponding line function at another
 * point on an elliptic curve with embedding degree 12.
 *
 * @param[out] L			- the result of the evaluation.
 * @param[out] R			- the resulting point.
 * @param[in] Q				- the point to double.
 * @param[in] P				- the affine point to evaluate the line function.
 */
#if EP_ADD == BASIC
#define pp_dbl_k12(L, R, Q, P)			pp_dbl_k12_basic(L, R, Q, P)
#elif EP_ADD == PROJC
#define pp_dbl_k12(L, R, Q, P)			pp_dbl_k12_projc(L, R, Q, P)
#endif

/**
 * Doubles a point and evaluates the corresponding line function at another
 * point on an elliptic curve with embedding degree 2 using projective
 * coordinates.
 *
 * @param[out] L			- the result of the evaluation.
 * @param[in, out] R		- the resulting point.
 * @param[in] Q				- the point to double.
 * @param[in] P				- the affine point to evaluate the line function.
 */
#if PP_EXT == BASIC
#define pp_dbl_k2_projc(L, R, P, Q)		pp_dbl_k2_projc_basic(L, R, P, Q)
#elif PP_EXT == LAZYR
#define pp_dbl_k2_projc(L, R, P, Q)		pp_dbl_k2_projc_lazyr(L, R, P, Q)
#endif

/**
 * Doubles a point and evaluates the corresponding line function at another
 * point on an elliptic curve with embedding degree 12 using projective
 * coordinates.
 *
 * @param[out] L			- the result of the evaluation.
 * @param[in, out] R		- the resulting point.
 * @param[in] Q				- the point to double.
 * @param[in] P				- the affine point to evaluate the line function.
 */
#if PP_EXT == BASIC
#define pp_dbl_k12_projc(L, R, Q, P)	pp_dbl_k12_projc_basic(L, R, Q, P)
#elif PP_EXT == LAZYR
#define pp_dbl_k12_projc(L, R, Q, P)	pp_dbl_k12_projc_lazyr(L, R, Q, P)
#endif

/**
 * Computes a pairing of two prime elliptic curve points defined on an elliptic
 * curves of embedding degree 2. Computes e(P, Q).
 *
 * @param[out] R			- the result.
 * @param[in] P				- the first elliptic curve point.
 * @param[in] Q				- the second elliptic curve point.
 */
#if PP_MAP == TATEP
#define pp_map_k2(R, P, Q)				pp_map_tatep_k2(R, P, Q)
#elif PP_MAP == WEILP
#define pp_map_k2(R, P, Q)				pp_map_weilp_k2(R, P, Q)
#elif PP_MAP == OATEP
#define pp_map_k2(R, P, Q)				pp_map_tatep_k2(R, P, Q)
#endif

/**
 * Computes a pairing of two prime elliptic curve points defined on an elliptic
 * curve of embedding degree 12. Computes e(P, Q).
 *
 * @param[out] R			- the result.
 * @param[in] P				- the first elliptic curve point.
 * @param[in] Q				- the second elliptic curve point.
 */
#if PP_MAP == TATEP
#define pp_map_k12(R, P, Q)				pp_map_tatep_k12(R, P, Q)
#elif PP_MAP == WEILP
#define pp_map_k12(R, P, Q)				pp_map_weilp_k12(R, P, Q)
#elif PP_MAP == OATEP
#define pp_map_k12(R, P, Q)				pp_map_oatep_k12(R, P, Q)
#endif

/**
 * Computes a multi-pairing of elliptic curve points defined on an elliptic
 * curve of embedding degree 2. Computes \prod e(P_i, Q_i).
 *
 * @param[out] R			- the result.
 * @param[in] P				- the first pairing arguments.
 * @param[in] Q				- the second pairing arguments.
 * @param[in] M 			- the number of pairings to evaluate.
 */
#if PP_MAP == WEILP
#define pp_map_sim_k2(R, P, Q, M)		pp_map_sim_weilp_k2(R, P, Q, M)
#elif PP_MAP == TATEP || PP_MAP == OATEP
#define pp_map_sim_k2(R, P, Q, M)		pp_map_sim_tatep_k2(R, P, Q, M)
#endif


/**
 * Computes a multi-pairing of elliptic curve points defined on an elliptic
 * curve of embedding degree 12. Computes \prod e(P_i, Q_i).
 *
 * @param[out] R			- the result.
 * @param[in] P				- the first pairing arguments.
 * @param[in] Q				- the second pairing arguments.
 * @param[in] M 			- the number of pairings to evaluate.
 */
#if PP_MAP == TATEP
#define pp_map_sim_k12(R, P, Q, M)		pp_map_sim_tatep_k12(R, P, Q, M)
#elif PP_MAP == WEILP
#define pp_map_sim_k12(R, P, Q, M)		pp_map_sim_weilp_k12(R, P, Q, M)
#elif PP_MAP == OATEP
#define pp_map_sim_k12(R, P, Q, M)		pp_map_sim_oatep_k12(R, P, Q, M)
#endif

/*============================================================================*/
/* Function prototypes                                                        */
/*============================================================================*/

/**
 * Initializes the pairing over prime fields.
 */
void pp_map_init(void);

/**
 * Finalizes the pairing over prime fields.
 */
void pp_map_clean(void);

/**
 * Adds two points and evaluates the corresponding line function at another
 * point on an elliptic curve with embedding degree 2 using affine coordinates.
 *
 * @param[out] l			- the result of the evaluation.
 * @param[in, out] r		- the resulting point and first point to add.
 * @param[in] p				- the second point to add.
 * @param[in] q				- the affine point to evaluate the line function.
 */
void pp_add_k2_basic(fp2_t l, ep_t r, ep_t p, ep_t q);

/**
 * Adds two points and evaluates the corresponding line function at another
 * point on an elliptic curve with embedding degree 12 using projective
 * coordinates.
 *
 * @param[out] l			- the result of the evaluation.
 * @param[in, out] r		- the resulting point and first point to add.
 * @param[in] p				- the second point to add.
 * @param[in] q				- the affine point to evaluate the line function.
 */
void pp_add_k2_projc_basic(fp2_t l, ep_t r, ep_t p, ep_t q);

/**
 * Adds two points and evaluates the corresponding line function at another
 * point on an elliptic curve with embedding degree 12 using projective
 * coordinates and lazy reduction.
 *
 * @param[out] l			- the result of the evaluation.
 * @param[in, out] r		- the resulting point and first point to add.
 * @param[in] p				- the second point to add.
 * @param[in] q				- the affine point to evaluate the line function.
 */
void pp_add_k2_projc_lazyr(fp2_t l, ep_t r, ep_t p, ep_t q);

/**
 * Adds two points and evaluates the corresponding line function at another
 * point on an elliptic curve with embedding degree 12 using affine coordinates.
 *
 * @param[out] l			- the result of the evaluation.
 * @param[in, out] r		- the resulting point and first point to add.
 * @param[in] q				- the second point to add.
 * @param[in] p				- the affine point to evaluate the line function.
 */
void pp_add_k12_basic(fp12_t l, ep2_t r, ep2_t q, ep_t p);

/**
 * Adds two points and evaluates the corresponding line function at another
 * point on an elliptic curve with embedding degree 12 using projective
 * coordinates.
 *
 * @param[out] l			- the result of the evaluation.
 * @param[in, out] r		- the resulting point and first point to add.
 * @param[in] q				- the second point to add.
 * @param[in] p				- the affine point to evaluate the line function.
 */
void pp_add_k12_projc_basic(fp12_t l, ep2_t r, ep2_t q, ep_t p);

/**
 * Adds two points and evaluates the corresponding line function at another
 * point on an elliptic curve with embedding degree 12 using projective
 * coordinates and lazy reduction.
 *
 * @param[out] l			- the result of the evaluation.
 * @param[in, out] r		- the resulting point and first point to add.
 * @param[in] q				- the second point to add.
 * @param[in] p				- the affine point to evaluate the line function.
 */
void pp_add_k12_projc_lazyr(fp12_t l, ep2_t r, ep2_t q, ep_t p);

/**
 * Adds two points and evaluates the corresponding line function at another
 * point on an elliptic curve twist with embedding degree 12 using projective
 * coordinates.
 *
 * @param[out] l			- the result of the evaluation.
 * @param[in, out] r		- the resulting point and first point to add.
 * @param[in] p				- the second point to add.
 * @param[in] q				- the affine point to evaluate the line function.
 */
void pp_add_lit_k12(fp12_t l, ep_t r, ep_t p, ep2_t q);

/**
 * Doubles a point and evaluates the corresponding line function at another
 * point on an elliptic curve with embedding degree 2 using affine
 * coordinates.
 *
 * @param[out] l			- the result of the evaluation.
 * @param[in, out] r		- the resulting point.
 * @param[in] p				- the point to double.
 * @param[in] q				- the affine point to evaluate the line function.
 */
void pp_dbl_k2_basic(fp2_t l, ep_t r, ep_t p, ep_t q);

/**
 * Doubles a point and evaluates the corresponding line function at another
 * point on an elliptic curve with embedding degree 12 using projective
 * coordinates.
 *
 * @param[out] l			- the result of the evaluation.
 * @param[in, out] r		- the resulting point.
 * @param[in] p				- the point to double.
 * @param[in] q				- the affine point to evaluate the line function.
 */
void pp_dbl_k2_projc_basic(fp2_t l, ep_t r, ep_t p, ep_t q);

/**
 * Doubles a point and evaluates the corresponding line function at another
 * point on an elliptic curve with embedding degree 12 using projective
 * coordinates and lazy reduction.
 *
 * @param[out] l			- the result of the evaluation.
 * @param[in, out] r		- the resulting point.
 * @param[in] p				- the point to double.
 * @param[in] q				- the affine point to evaluate the line function.
 */
void pp_dbl_k2_projc_lazyr(fp2_t l, ep_t r, ep_t p, ep_t q);

/**
 * Doubles a point and evaluates the corresponding line function at another
 * point on an elliptic curve with embedding degree 12 using affine
 * coordinates.
 *
 * @param[out] l			- the result of the evaluation.
 * @param[in, out] r		- the resulting point.
 * @param[in] q				- the point to double.
 * @param[in] p				- the affine point to evaluate the line function.
 */
void pp_dbl_k12_basic(fp12_t l, ep2_t r, ep2_t q, ep_t p);

/**
 * Doubles a point and evaluates the corresponding line function at another
 * point on an elliptic curve with embedding degree 12 using projective
 * coordinates.
 *
 * @param[out] l			- the result of the evaluation.
 * @param[in, out] r		- the resulting point.
 * @param[in] q				- the point to double.
 * @param[in] p				- the affine point to evaluate the line function.
 */
void pp_dbl_k12_projc_basic(fp12_t l, ep2_t r, ep2_t q, ep_t p);

/**
 * Doubles a point and evaluates the corresponding line function at another
 * point on an elliptic curve with embedding degree 12 using projective
 * coordinates and lazy reduction.
 *
 * @param[out] l			- the result of the evaluation.
 * @param[in, out] r		- the resulting point.
 * @param[in] q				- the point to double.
 * @param[in] p				- the affine point to evaluate the line function.
 */
void pp_dbl_k12_projc_lazyr(fp12_t l, ep2_t r, ep2_t q, ep_t p);

/**
 * Doubles a point and evaluates the corresponding line function at another
 * point on an elliptic curve twist with embedding degree 12 using projective
 * coordinates.
 *
 * @param[out] l			- the result of the evaluation.
 * @param[in, out] r		- the resulting point.
 * @param[in] p				- the point to double.
 * @param[in] q				- the affine point to evaluate the line function.
 */
void pp_dbl_lit_k12(fp12_t l, ep_t r, ep_t p, ep2_t q);

/**
 * Computes the final exponentiation for a pairing defined over curves of
 * embedding degree 2. Computes c = a^(p^2 - 1)/r.
 *
 * @param[out] c			- the result.
 * @param[in] a				- the extension field element to exponentiate.
 */
void pp_exp_k2(fp2_t c, fp2_t a);

/**
 * Computes the final exponentiation for a pairing defined over curves of
 * embedding degree 12. Computes c = a^(p^12 - 1)/r.
 *
 * @param[out] c			- the result.
 * @param[in] a				- the extension field element to exponentiate.
 */
void pp_exp_k12(fp12_t c, fp12_t a);

/**
 * Normalizes the accumulator point used inside pairing computation defined
 * over curves of embedding degree 2.
 *
 * @param[out] r			- the resulting point.
 * @param[in] p				- the point to normalize.
 */
void pp_norm_k2(ep_t c, ep_t a);

/**
 * Normalizes the accumulator point used inside pairing computation defined
 * over curves of embedding degree 12.
 *
 * @param[out] r			- the resulting point.
 * @param[in] p				- the point to normalize.
 */
void pp_norm_k12(ep2_t c, ep2_t a);

/**
 * Computes the Tate pairing of two points in a parameterized elliptic curve
 * with embedding degree 12.
 *
 * @param[out] r			- the result.
 * @param[in] q				- the first elliptic curve point.
 * @param[in] p				- the second elliptic curve point.
 */
void pp_map_tatep_k2(fp2_t r, ep_t p, ep_t q);

/**
 * Computes the Tate multi-pairing of in a parameterized elliptic curve with
 * embedding degree 2.
 *
 * @param[out] r			- the result.
 * @param[in] q				- the first pairing arguments.
 * @param[in] p				- the second pairing arguments.
 * @param[in] m 			- the number of pairings to evaluate.
 */
void pp_map_sim_tatep_k2(fp2_t r, ep_t *p, ep_t *q, int m);

/**
 * Computes the Weil pairing of two points in a parameterized elliptic curve
 * with embedding degree 2.
 *
 * @param[out] r			- the result.
 * @param[in] q				- the first elliptic curve point.
 * @param[in] p				- the second elliptic curve point.
 */
void pp_map_weilp_k2(fp2_t r, ep_t p, ep_t q);

/**
 * Computes the Weil multi-pairing of in a parameterized elliptic curve with
 * embedding degree 2.
 *
 * @param[out] r			- the result.
 * @param[in] q				- the first pairing arguments.
 * @param[in] p				- the second pairing arguments.
 * @param[in] m 			- the number of pairings to evaluate.
 */
void pp_map_sim_weilp_k2(fp2_t r, ep_t *p, ep_t *q, int m);

/**
 * Computes the Tate pairing of two points in a parameterized elliptic curve
 * with embedding degree 12.
 *
 * @param[out] r			- the result.
 * @param[in] q				- the first elliptic curve point.
 * @param[in] p				- the second elliptic curve point.
 */
void pp_map_tatep_k12(fp12_t r, ep_t p, ep2_t q);

/**
 * Computes the Tate multi-pairing of in a parameterized elliptic curve with
 * embedding degree 12.
 *
 * @param[out] r			- the result.
 * @param[in] q				- the first pairing arguments.
 * @param[in] p				- the second pairing arguments.
 * @param[in] m 			- the number of pairings to evaluate.
 */
void pp_map_sim_tatep_k12(fp12_t r, ep_t *p, ep2_t *q, int m);

/**
 * Computes the Weil pairing of two points in a parameterized elliptic curve
 * with embedding degree 12.
 *
 * @param[out] r			- the result.
 * @param[in] q				- the first elliptic curve point.
 * @param[in] p				- the second elliptic curve point.
 */
void pp_map_weilp_k12(fp12_t r, ep_t p, ep2_t q);

/**
 * Computes the Weil multi-pairing of in a parameterized elliptic curve with
 * embedding degree 12.
 *
 * @param[out] r			- the result.
 * @param[in] q				- the first pairing arguments.
 * @param[in] p				- the second pairing arguments.
 * @param[in] m 			- the number of pairings to evaluate.
 */
void pp_map_sim_weilp_k12(fp12_t r, ep_t *p, ep2_t *q, int m);

/**
 * Computes the optimal ate pairing of two points in a parameterized elliptic
 * curve with embedding degree 12.
 *
 * @param[out] r			- the result.
 * @param[in] q				- the first elliptic curve point.
 * @param[in] p				- the second elliptic curve point.
 */
void pp_map_oatep_k12(fp12_t r, ep_t p, ep2_t q);

/**
 * Computes the optimal ate multi-pairing of in a parameterized elliptic
 * curve with embedding degree 12.
 *
 * @param[out] r			- the result.
 * @param[in] q				- the first pairing arguments.
 * @param[in] p				- the second pairing arguments.
 * @param[in] m 			- the number of pairings to evaluate.
 */
void pp_map_sim_oatep_k12(fp12_t r, ep_t *p, ep2_t *q, int m);

#endif /* !RELIC_PP_H */
