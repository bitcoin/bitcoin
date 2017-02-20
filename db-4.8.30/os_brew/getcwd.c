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
 * getcwd --
 */
char *
getcwd(buf, size)
	char *buf;
	size_t size;
{
	IFileMgr *pIFileMgr;
	int ret;
#ifndef HAVE_BREW_SDK2
	int outlen;
#endif

	FILE_MANAGER_CREATE(NULL, pIFileMgr, ret);
	if (ret != 0) {
		__os_set_errno(ret);
		return (NULL);
	}

	buf[0] = '\0';

#ifdef AEE_SIMULATOR
	/* If AEE_SIMULATOR, we should mimic the resolvepath. */
	if (IFILEMGR_Test(pIFileMgr, "fs:/") == SUCCESS)
		/* Current directory. */
		(void)strncpy(buf, "fs:/", size - 1);
	else
		FILE_MANAGER_ERR(
		    NULL, pIFileMgr, NULL, "IFILEMGR_ResolvePath", ret);
#else
#ifndef HAVE_BREW_SDK2
	outlen = size;
	if (IFILEMGR_ResolvePath(pIFileMgr, ".", buf, &outlen) != SUCCESS)
		FILE_MANAGER_ERR(
		    NULL, pIFileMgr, NULL, "IFILEMGR_ResolvePath", ret);
#endif
#endif

	IFILEMGR_Release(pIFileMgr);

	if (ret == 0)
		return (buf);

	__os_set_errno(ret);

	COMPQUIET(size, 0);
	return (NULL);
}
