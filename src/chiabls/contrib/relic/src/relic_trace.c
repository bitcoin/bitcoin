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
 * Implementation of tracing routines.
 *
 * @ingroup relic
 */

#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>

#include "relic_core.h"
#include "relic_conf.h"
#include "relic_trace.h"

#ifdef TRACE
#include <execinfo.h>
#include <dlfcn.h>
#endif

/*============================================================================*/
/* Private definitions                                                        */
/*============================================================================*/

#ifdef TRACE
/**
 * Prints some spaces before the stack trace.
 */
#define FPRINTF fprintf(stderr, "%*s", ctx->trace+1, " "); fprintf
#else
/**
 * Prints with fprintf if tracing is disabled.
 */
#define FPRINTF fprintf
#endif

/*============================================================================*/
/* Public definitions                                                         */
/*============================================================================*/

#ifdef TRACE

void trace_enter(void *this, void *from) {
	ctx_t *ctx;
	if (core_ctx != NULL) {
		ctx = core_ctx;
	} else {
		ctx = &first_ctx;
	}
	ctx->trace++;
#ifdef VERBS
	Dl_info info;

	dladdr(this, &info);
	if (info.dli_sname != NULL) {
		FPRINTF(stderr, "%d - running %s()\n", ctx->trace, info.dli_sname);
	}
#endif
	(void)this;
	(void)from;
}

void trace_exit(void *this, void *from) {
	ctx_t *ctx;
	if (core_ctx != NULL) {
		ctx = core_ctx;
	} else {
		ctx = &first_ctx;
	}
#ifdef VERBS
	Dl_info info;

	dladdr(this, &info);
	if (info.dli_sname != NULL) {
		FPRINTF(stderr, "%d - exiting %s()\n", ctx->trace, info.dli_sname);
	}
#endif
	if (core_ctx->trace > 0) {
		core_ctx->trace--;
	}
	(void)this;
	(void)from;
}

#endif
