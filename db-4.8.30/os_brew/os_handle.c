/*-
 * See the file LICENSE for redistribution information.
 *
 * Copyright (c) 1998-2009 Oracle.  All rights reserved.
 *
 * $Id$
 */

#include "db_config.h"

#include "db_int.h"

/*
 * __os_openhandle --
 *      Open a file, using BREW open flags.
 */
int
__os_openhandle(env, name, flags, mode, fhpp)
	ENV *env;
	const char *name;
	int flags, mode;
	DB_FH **fhpp;
{
	DB_FH *fhp;
	IFile *pIFile;
	IFileMgr *pIFileMgr;
	int f_exists, ret;

	COMPQUIET(mode, 0);

	FILE_MANAGER_CREATE(env, pIFileMgr, ret);
	if (ret != 0)
		return (ret);

	/*
	 * Allocate the file handle and copy the file name.  We generally only
	 * use the name for verbose or error messages, but on systems where we
	 * can't unlink temporary files immediately, we use the name to unlink
	 * the temporary file when the file handle is closed.
	 */
	if ((ret = __os_calloc(env, 1, sizeof(DB_FH), &fhp)) != 0)
		return (ret);
	if ((ret = __os_strdup(env, name, &fhp->name)) != 0)
		goto err;

	/*
	 * Test the file before opening.  BREW doesn't want to see the
	 * _OFM_CREATE flag if the file already exists, and it doesn't
	 * want to see any other flag if the file doesn't exist.
	 */
	f_exists = IFILEMGR_Test(pIFileMgr, name) == SUCCESS ? 1 : 0;
	if (f_exists)
		LF_CLR(_OFM_CREATE);		/* Clear _OFM_CREATE. */
	else
		LF_CLR(~_OFM_CREATE);		/* Leave only _OFM_CREATE. */

	if ((pIFile =
	    IFILEMGR_OpenFile(pIFileMgr, name, (OpenFileMode)flags)) == NULL) {
		FILE_MANAGER_ERR(env,
		    pIFileMgr, name, "IFILEMGR_OpenFile", ret);
		goto err;
	}

	fhp->ifp = pIFile;
	IFILEMGR_Release(pIFileMgr);

	F_SET(fhp, DB_FH_OPENED);
	*fhpp = fhp;
	return (0);

err:	if (pIFile != NULL)
		IFILE_Release(pIFile);
	IFILEMGR_Release(pIFileMgr);

	if (fhp != NULL)
		(void)__os_closehandle(env, fhp);
	return (ret);
}

/*
 * __os_closehandle --
 *      Close a file.
 */
int
__os_closehandle(env, fhp)
	ENV *env;
	DB_FH *fhp;
{
	/* Discard any underlying system file reference. */
	if (F_ISSET(fhp, DB_FH_OPENED))
		(void)IFILE_Release(fhp->ifp);

	/* Unlink the file if we haven't already done so. */
	if (F_ISSET(fhp, DB_FH_UNLINK))
		(void)__os_unlink(env, fhp->name, 0);

	if (fhp->name != NULL)
		__os_free(env, fhp->name);
	__os_free(env, fhp);

	return (0);
}
