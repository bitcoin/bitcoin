/*-
 * See the file LICENSE for redistribution information.
 *
 * Copyright (c) 1997-2009 Oracle.  All rights reserved.
 *
 * $Id$
 */

#include "db_config.h"

#include "db_int.h"

/*
 * __db_openflags --
 *	Convert open(2) flags to DB flags.
 *
 * PUBLIC: u_int32_t __db_openflags __P((int));
 */
u_int32_t
__db_openflags(oflags)
	int oflags;
{
	u_int32_t dbflags;

	dbflags = 0;

	if (oflags & O_CREAT)
		dbflags |= DB_CREATE;

	if (oflags & O_TRUNC)
		dbflags |= DB_TRUNCATE;

	/*
	 * !!!
	 * Convert POSIX 1003.1 open(2) mode flags to DB flags.  This isn't
	 * an exact science as few POSIX implementations have a flag value
	 * for O_RDONLY, it's simply the lack of a write flag.
	 */
#ifndef	O_ACCMODE
#define	O_ACCMODE	(O_RDONLY | O_RDWR | O_WRONLY)
#endif
	switch (oflags & O_ACCMODE) {
	case O_RDWR:
	case O_WRONLY:
		break;
	default:
		dbflags |= DB_RDONLY;
		break;
	}
	return (dbflags);
}
