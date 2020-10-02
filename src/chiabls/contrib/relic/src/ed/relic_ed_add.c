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

#include <assert.h>

#include "relic_core.h"
#include "relic_label.h"

static void ed_add_inver(ed_t r, const ed_t p, const ed_t q) {
	assert(ed_is_valid(p) != 0);
	assert(ed_is_valid(q) != 0);

	fp_t A;
	fp_t B;
	fp_t C;
	fp_t D;
	fp_t E;
	fp_t H;
	fp_t I;
	fp_t J;

	fp_new(A);
	fp_new(B);
	fp_new(C);
	fp_new(D);
	fp_new(E);
	fp_new(I);
	fp_new(H);
	fp_new(J);

	// A = Z_1 * Z_2;
	fp_mul(A, p->z, q->z);

	// B = d * A^2;
	fp_sqr(B, A);
	fp_mul(B, B, core_get()->ed_d);

	// C = X_1 * X_2;
	fp_mul(C, p->x, q->x);

	// D = Y_1 * Y_2;
	fp_mul(D, p->y, q->y);

	// E = C * D;
	fp_mul(E, C, D);

	// H = C - a * D;
	fp_mul(H, core_get()->ed_a, D);
	fp_sub(H, C, H);

	// I = (X_1 + Y_1) * (X_2 + Y_2) - C - D;
	fp_add(I, p->x, p->y);
	fp_add(J, q->x, q->y);
	fp_mul(I, I, J);
	fp_sub(I, I, C);
	fp_sub(I, I, D);

	// X_3 = (E + B) * H;
	fp_add(r->x, E, B);
	fp_mul(r->x, r->x, H);

	// Y_3 = (E - B) * I;
	fp_sub(r->y, E, B);
	fp_mul(r->y, r->y, I);

	// Z_3 = A * H * I
	fp_mul(r->z, A, H);
	fp_mul(r->z, r->z, I);

	assert(ed_is_valid(r) != 0);

	fp_free(A);
	fp_free(B);
	fp_free(C);
	fp_free(D);
	fp_free(E);
	fp_free(I);
	fp_free(H);
	fp_free(J);
}

#if ED_ADD == PROJC || ED_MUL == LWNAF_MIXED

static void ed_add_projc(ed_t r, const ed_t p, const ed_t q) {
	fp_t A;
	fp_t B;
	fp_t C;
	fp_t D;
	fp_t E;
	fp_t F;
	fp_t G;
	fp_t H;

	fp_new(A);
	fp_new(B);
	fp_new(C);
	fp_new(D);
	fp_new(E);
	fp_new(F);
	fp_new(G);
	fp_new(H);

	// A = Z_1 * Z_2;
	fp_mul(A, p->z, q->z);

	// B = A^2;
	fp_sqr(B, A);

	// C = X_1 * X_2;
	fp_mul(C, p->x, q->x);

	// D = Y_1 * Y_2;
	fp_mul(D, p->y, q->y);

	// E = d * C * D;
	fp_mul(E, core_get()->ed_d, C);
	fp_mul(E, E, D);

	// F = B - E;
	fp_sub(F, B, E);

	// G = B + E
	fp_add(G, B, E);

	// X_3 = A * F * ((X_1 + Y_1) * (X_2 + Y_2) - C - D)
	fp_mul(H, A, F);
	fp_add(r->z, p->x, p->y);
	fp_add(r->x, q->x, q->y);
	fp_mul(r->x, r->z, r->x);
	fp_sub(r->x, r->x, C);
	fp_sub(r->x, r->x, D);
	fp_mul(r->x, H, r->x);


	// Y_3 = A * G * (D - a * C)
	fp_mul(r->z, A, G);
	fp_mul(r->y, core_get()->ed_a, C);
	fp_sub(r->y, D, r->y);
	fp_mul(r->y, r->z, r->y);

	// Z_3 = F * G
	fp_mul(r->z, F, G);

	fp_free(A);
	fp_free(B);
	fp_free(C);
	fp_free(D);
	fp_free(E);
	fp_free(F);
	fp_free(G);
	fp_free(H);
}
#endif

#if ED_ADD == EXTND

static void ed_add_extnd(ed_t r, const ed_t p, const ed_t q) {
	fp_t A;
	fp_t B;
	fp_t E;
	fp_t F;
	fp_t G;

	fp_new(A);
	fp_new(B);
	fp_new(E);
	fp_new(F);
	fp_new(G);

	// A = X_1 * X_2
	fp_mul(A, p->x, q->x);

	// B = Y_1 * Y_2
	fp_mul(B, p->y, q->y);

	// C = d * T_1 * T_2
#define C (r->t)
	fp_mul(C, core_get()->ed_d, p->t);
	fp_mul(C, C, q->t);

	// D = Z_1 * Z_2
#define D (r->z)
	fp_mul(D, p->z, q->z);

	// E = (X_1 + Y_1) * (X_2 + Y_2) - A - B
	fp_add(E, p->x, p->y);
	fp_add(F, q->x, q->y);
	fp_mul(E, E, F);
	fp_sub(E, E, A);
	fp_sub(E, E, B);

	// F = D - C
	fp_sub(F, D, C);

	// G = D + C
	fp_add(G, D, C);
#undef C
#undef D

	// H = B - aA
	fp_mul(r->x, core_get()->ed_a, A);
#define H (r->z)
	fp_sub(H, B, r->x);

	// X_3 = E * F
	fp_mul(r->x, E, F);

	// Y_3 = G * H
	fp_mul(r->y, G, H);

	// T_3 = E * H
	fp_mul(r->t, E, H);
#undef H

	// Z_3 = F * G
	fp_mul(r->z, F, G);

	fp_free(A);
	fp_free(B);
	fp_free(E);
	fp_free(F);
	fp_free(G);
}

#endif

void ed_add(ed_t r, const ed_t p, const ed_t q) {
#if ED_ADD == PROJC
	ed_add_projc(r, p, q);
#elif ED_ADD == EXTND
	ed_add_extnd(r, p, q);
#endif
}

void ed_sub(ed_t r, const ed_t p, const ed_t q) {
	ed_t t;

	ed_null(t);

	if (p == q) {
		ed_set_infty(r);
		return;
	}

	TRY {
		ed_new(t);

		ed_neg(t, q);
		ed_add(r, p, t);
	}
	CATCH_ANY {
		THROW(ERR_CAUGHT);
	}
	FINALLY {
		ed_free(t);
	}
}
