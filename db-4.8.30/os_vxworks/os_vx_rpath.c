/*-
 * See the file LICENSE for redistribution information.
 *
 * Copyright (c) 1997-2009 Oracle.  All rights reserved.
 *
 * $Id$
 */

#include "db_config.h"

#include "db_int.h"

#include "iosLib.h"

/*
 * __db_rpath --
 *	Return the last path separator in the path or NULL if none found.
 */
char *
__db_rpath(path)
	const char *path;
{
	const char *s, *last;
	DEV_HDR *dummy;
	char *ptail;

	/*
	 * VxWorks devices can be rooted at any name.  We want to
	 * skip over the device name and not take into account any
	 * PATH_SEPARATOR characters that might be in that name.
	 *
	 * XXX [#2393]
	 * VxWorks supports having a filename directly follow a device
	 * name with no separator.  I.e. to access a file 'xxx' in
	 * the top level directory of a device mounted at "mydrive"
	 * you could say "mydrivexxx" or "mydrive/xxx" or "mydrive\xxx".
	 * We do not support the first usage here.
	 * XXX
	 */
	if ((dummy = iosDevFind(path, (const char**)&ptail)) == NULL)
		s = path;
	else
		s = ptail;

	last = NULL;
	if (PATH_SEPARATOR[1] != '\0') {
		for (; s[0] != '\0'; ++s)
			if (strchr(PATH_SEPARATOR, s[0]) != NULL)
				last = s;
	} else
		for (; s[0] != '\0'; ++s)
			if (s[0] == PATH_SEPARATOR[0])
				last = s;
	return ((char *)last);
}
