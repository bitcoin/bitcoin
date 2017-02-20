/*-
 * See the file LICENSE for redistribution information.
 *
 * Copyright (c) 1996-2009 Oracle.  All rights reserved.
 *
 * $Id$
 */

#include "db_config.h"

#include "db_int.h"

/*
 * __db_isbigendian --
 *	Return 1 if big-endian (Motorola and Sparc), not little-endian
 *	(Intel and Vax).  We do this work at run-time, rather than at
 *	configuration time so cross-compilation and general embedded
 *	system support is simpler.
 *
 * PUBLIC: int __db_isbigendian __P((void));
 */
int
__db_isbigendian()
{
	union {					/* From Harbison & Steele.  */
		long l;
		char c[sizeof(long)];
	} u;

	u.l = 1;
	return (u.c[sizeof(long) - 1] == 1);
}

/*
 * __db_byteorder --
 *	Return if we need to do byte swapping, checking for illegal
 *	values.
 *
 * PUBLIC: int __db_byteorder __P((ENV *, int));
 */
int
__db_byteorder(env, lorder)
	ENV *env;
	int lorder;
{
	switch (lorder) {
	case 0:
		break;
	case 1234:
		if (!F_ISSET(env, ENV_LITTLEENDIAN))
			return (DB_SWAPBYTES);
		break;
	case 4321:
		if (F_ISSET(env, ENV_LITTLEENDIAN))
			return (DB_SWAPBYTES);
		break;
	default:
		__db_errx(env,
	    "unsupported byte order, only big and little-endian supported");
		return (EINVAL);
	}
	return (0);
}
