/*-
 * See the file LICENSE for redistribution information.
 *
 * Copyright (c) 2006-2009 Oracle.  All rights reserved.
 *
 * $Id$
 */

#include "db_config.h"

#include "db_int.h"

/*
 * fwrite --
 *
 * PUBLIC: #ifndef HAVE_FWRITE
 * PUBLIC: size_t fwrite __P((const void *, size_t, size_t, FILE *));
 * PUBLIC: #endif
 */
size_t
fwrite(buf, size, count, fp)
	const void *buf;
	size_t size, count;
	FILE *fp;
{
	if (fp == stderr) {
		DBGPRINTF("%.*s", (int)count, buf);
		return (size * count);
	} else
		return ((size_t)IFILE_Write(fp, buf, size * count) / size);
}
