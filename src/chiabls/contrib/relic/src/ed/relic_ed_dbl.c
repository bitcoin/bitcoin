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
* Implementation of the point doubling on prime elliptic twisted Edwards curves.
*
* @version $Id$
* @ingroup ed
*/

#include "relic_core.h"
#include "relic_label.h"

#if ED_ADD == PROJC

static void ed_dbl_projc(ed_t r, const ed_t p) {
	fp_t B;
	fp_t C;
	fp_t D;
	fp_t E;
	fp_t F;
	fp_t H;
	fp_t J;

	fp_new(B);
	fp_new(C);
	fp_new(D);
	fp_new(E);
	fp_new(F);
	fp_new(H);
	fp_new(J);

	// B = (X_1 + Y_1)^2
	fp_add(B, p->x, p->y);
	fp_sqr(B, B);

	// C = X_1^2
	fp_sqr(C, p->x);

	// D = Y_1^2
	fp_sqr(D, p->y);

	// E = aC
	fp_mul(E, core_get()->ed_a, C);

	// F = E + D
	fp_add(F, E, D);

	// H = Z^2
	fp_sqr(H, p->z);

	// J = F - 2H
	fp_dbl(J, H);
	fp_sub(J, F, J);

	// X_3 = (B - C - D) * J
	fp_sub(r->x, B, C);
	fp_sub(r->x, r->x, D);
	fp_mul(r->x, r->x, J);

	// Y_3 = F * (E - D)
	fp_sub(r->y, E, D);
	fp_mul(r->y, F, r->y);

	// Z_3 = F * J
	fp_mul(r->z, F, J);

	// 3M + 4S + 1D + 7add

	fp_free(B);
	fp_free(C);
	fp_free(D);
	fp_free(E);
	fp_free(F);
	fp_free(H);
	fp_free(J);
}

#endif

#if ED_ADD == EXTND

static void ed_dbl_extnd(ed_t r, const ed_t p) {
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

	// A = X^2
	fp_sqr(A, p->x);

	// B = Y^2
	fp_sqr(B, p->y);

	// C = 2 * Z^2
#define C (r->z)
	fp_sqr(C, p->z);
	fp_dbl(C, C);

	// D = a * A
#define D (r->t)
	fp_mul(D, core_get()->ed_a, A);

	// E = (X + Y) ^ 2 - A - B
	fp_add(E, p->x, p->y);
	fp_sqr(E, E);
	fp_sub(E, E, A);
	fp_sub(E, E, B);

	// G = D + B
	fp_add(G, D, B);

	// F = G - C
	fp_sub(F, G, C);
#undef C

	// H = D - B
#define H (r->z)
	fp_sub(H, D, B);
#undef D

	// X = E * F
	fp_mul(r->x, E, F);

	// Y = G * H
	fp_mul(r->y, G, H);

	// T = E * H
	fp_mul(r->t, E, H);
#undef H

	// Z = F * G
	fp_mul(r->z, F, G);

	// 4M + 4S + 1D + 7add

	fp_free(A);
	fp_free(B);
	fp_free(E);
	fp_free(F);
	fp_free(G);
}

static void ed_dbl_extnd_short(ed_t r, const ed_t p) {
	fp_t A;
	fp_t B;
	fp_t F;
	fp_t G;


	fp_new(A);
	fp_new(B);
	fp_new(F);
	fp_new(G);

	// A = X^2
	fp_sqr(A, p->x);

	// B = Y^2
	fp_sqr(B, p->y);

	// C = 2 * Z^2
#define C (r->z)
	fp_sqr(C, p->z);
	fp_dbl(C, C);

	// D = a * A
#define D (r->t)
	fp_mul(D, core_get()->ed_a, A);

	// E = (X + Y) ^ 2 - A - B
#define E (r->y)
	fp_add(E, p->x, p->y);
	fp_sqr(E, E);
	fp_sub(E, E, A);
	fp_sub(E, E, B);

	// G = D + B
	fp_add(G, D, B);

	// F = G - C
	fp_sub(F, G, C);
#undef C

	// H = D - B
#define H (r->z)
	fp_sub(H, D, B);
#undef D

	// X = E * F
	fp_mul(r->x, E, F);
#undef E

	// Y = G * H
	fp_mul(r->y, G, H);
#undef H

	// Z = F * G
	fp_mul(r->z, F, G);

	// 4M + 4S + 1D + 7add

	fp_free(A);
	fp_free(B);
	fp_free(F);
	fp_free(G);
}

#endif

void ed_dbl(ed_t r, const ed_t p) {
#if ED_ADD == PROJC
	ed_dbl_projc(r, p);
#elif ED_ADD == EXTND
	ed_dbl_extnd(r, p);
#endif
}

void ed_dbl_short(ed_t r, const ed_t p) {
#if ED_ADD == PROJC
	ed_dbl(r, p);
#elif ED_ADD == EXTND
	ed_dbl_extnd_short(r, p);
#endif
}
