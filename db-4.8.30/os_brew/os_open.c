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
 * __os_open --
 *	Open a file descriptor (including page size and log size information).
 */
int
__os_open(env, name, page_size, flags, mode, fhpp)
	ENV *env;
	const char *name;
	u_int32_t page_size, flags;
	int mode;
	DB_FH **fhpp;
{
	OpenFileMode oflags;
	int ret;

	COMPQUIET(page_size, 0);

#define	OKFLAGS								\
	(DB_OSO_ABSMODE | DB_OSO_CREATE | DB_OSO_DIRECT | DB_OSO_DSYNC |\
	DB_OSO_EXCL | DB_OSO_RDONLY | DB_OSO_REGION |	DB_OSO_SEQ |	\
	DB_OSO_TEMP | DB_OSO_TRUNC)
	if ((ret = __db_fchk(env, "__os_open", flags, OKFLAGS)) != 0)
		return (ret);

	oflags = 0;
	if (LF_ISSET(DB_OSO_CREATE))
		oflags |= _OFM_CREATE;

	if (LF_ISSET(DB_OSO_RDONLY))
		oflags |= _OFM_READ;
	else
		oflags |= _OFM_READWRITE;

	if ((ret = __os_openhandle(env, name, oflags, mode, fhpp)) != 0)
		return (ret);
	if (LF_ISSET(DB_OSO_REGION))
		F_SET(fhp, DB_FH_REGION);
	return (ret);
}
