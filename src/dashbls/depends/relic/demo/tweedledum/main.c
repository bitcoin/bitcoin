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
 * Tests for the Elliptic Curve Cryptography module.
 *
 * @ingroup test
 */

#include <stdio.h>
#include <assert.h>

#include "relic.h"
#include "relic_test.h"

int main(void) {
	ec_t g, a, b;
	bn_t l, r;

	/* Initialize points. */
	ec_new(g);
	ec_new(a);
	ec_new(b);
	bn_new(l);
	bn_new(r);

	if (core_init() != RLC_OK) {
		core_clean();
		return 1;
	}

	util_banner("Tests for the EC module:", 0);

	/* Ask for curve to get Tweedledum. */
	if (ec_param_set_any() == RLC_ERR) {
		RLC_THROW(ERR_NO_CURVE);
		core_clean();
		return 0;
	}

	/* Print curve name to make sure it is the right one. */
	ec_param_print();

	/* Check that generator has the right order. */
	ec_curve_get_gen(g);
	ec_curve_get_ord(r);
	ec_mul(g, g, r);
	assert(ec_is_infty(g));

	/* Generate some random point and exponents to illustrate scalar mult. */
	ec_rand(a);
	ec_rand(b);
	bn_rand_mod(l, r);
	ec_mul(a, a, l);
	ec_print(a);

	util_banner("All tests have passed.\n", 0);

	ec_free(g);
	ec_free(a);
	ec_free(b);
	bn_free(l);
	bn_free(r);
	core_clean();
	return 0;
}
