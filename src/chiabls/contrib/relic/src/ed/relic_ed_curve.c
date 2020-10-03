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
 * Implementation of the point addition on prime elliptic twisted Edwards curves.
 *
 * @version $Id$
 * @ingroup ed
 */

#include "relic_core.h"

void ed_curve_init(void) {
	ctx_t *ctx = core_get();
#ifdef ED_PRECO
	for (int i = 0; i < RELIC_ED_TABLE; i++) {
		ctx->ed_ptr[i] = &(ctx->ed_pre[i]);
	}
#endif
#if ALLOC == STATIC
	fp_new(ctx->ed_g.x);
	fp_new(ctx->ed_g.y);
	fp_new(ctx->ed_g.z);
#if ED_ADD == EXTND
	fp_new(ctx->ed_g.t);
#endif
#ifdef ED_PRECO
	for (int i = 0; i < RELIC_ED_TABLE; i++) {
		fp_new(ctx->ed_pre[i].x);
		fp_new(ctx->ed_pre[i].y);
		fp_new(ctx->ed_pre[i].z);
#if ED_ADD == EXTND
		fp_new(ctx->ed_pre[i].t);
#endif
	}
#endif
#endif
	ed_set_infty(&ctx->ed_g);
	bn_init(&ctx->ed_r, FP_DIGS);
	bn_init(&ctx->ed_h, FP_DIGS);
#if defined(ED_ENDOM) && (ED_MUL == LWNAF || ED_FIX == COMBS || ED_FIX == LWNAF || !defined(STRIP))
	for (int i = 0; i < 3; i++) {
		bn_init(&(ctx->ed_v1[i]), FP_DIGS);
		bn_init(&(ctx->ed_v2[i]), FP_DIGS);
	}
#endif
}

void ed_curve_clean(void) {
	ctx_t *ctx = core_get();
#if ALLOC == STATIC
	fp_free(ctx->ed_g.x);
	fp_free(ctx->ed_g.y);
	fp_free(ctx->ed_g.z);
#if ED_ADD == EXTND
	fp_free(ctx->ed_g.t);
#endif
#ifdef ED_PRECO
	for (int i = 0; i < RELIC_ED_TABLE; i++) {
		fp_free(ctx->ed_pre[i].x);
		fp_free(ctx->ed_pre[i].y);
		fp_free(ctx->ed_pre[i].z);
#if ED_ADD == EXTND
		fp_free(ctx->ed_pre[i].t);
#endif
	}
#endif
#endif
	bn_clean(&ctx->ed_r);
	bn_clean(&ctx->ed_h);
#if defined(ED_ENDOM) && (ED_MUL == LWNAF || ED_FIX == LWNAF || !defined(STRIP))
	for (int i = 0; i < 3; i++) {
		bn_clean(&(ctx->ed_v1[i]));
		bn_clean(&(ctx->ed_v2[i]));
	}
#endif
}
