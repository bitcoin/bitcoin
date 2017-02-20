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
 * fopen --
 *
 * PUBLIC: #ifndef HAVE_FOPEN
 * PUBLIC: FILE *fopen __P((const char *, const char *));
 * PUBLIC: #endif
 */
FILE *
fopen(filename, mode)
	const char *filename, *mode;
{
	IFile *pIFile;
	IFileMgr *pIFileMgr;
	OpenFileMode flags;
	int f_exists, ret, update_flag;

	/*
	 * Note: files are created with read/write privilege.
	 *
	 * Upon successful completion, fopen() returns a pointer to the
	 * object controlling the stream.  Otherwise, NULL is returned,
	 * and errno is set to indicate the error.
	 */
	DB_ASSERT(NULL, filename != NULL && mode != NULL);

	FILE_MANAGER_CREATE(NULL, pIFileMgr, ret);
	if (ret != 0) {
		__os_set_errno(ret);
		return (NULL);
	}

	/*
	 * The argument mode points to a string beginning with one of the
	 * following sequences:
	 * r or rb
	 *	Open file for reading.
	 * w or wb
	 *	Truncate to zero length or create file for writing.
	 * a or ab
	 *	Append; open or create file for writing at end-of-file.
	 * r+ or rb+ or r+b
	 *	Open file for update (reading and writing).
	 * w+ or wb+ or w+b
	 *	Truncate to zero length or create file for update.
	 * a+ or ab+ or a+b
	 *	Append; open or create file for update, writing at end-of-file.
	 */
	flags = 0;
	update_flag = strchr(mode, '+') ? 1 : 0;
	switch (*mode) {
	case 'a':			/* append mode */
		flags = _OFM_APPEND | _OFM_CREATE;
		break;
	case 'r':			/* read mode */
		flags = update_flag ? _OFM_READWRITE : _OFM_READ;
		break;
	case 'w':			/* write mode */
		flags = _OFM_READWRITE | _OFM_CREATE;
		break;
	}

	f_exists = IFILEMGR_Test(pIFileMgr, filename) == SUCCESS ? 1 : 0;
	if (f_exists)
		LF_CLR(_OFM_CREATE);		/* Clear _OFM_CREATE. */
	else
		LF_CLR(~_OFM_CREATE);		/* Leave only _OFM_CREATE. */

	if ((pIFile = IFILEMGR_OpenFile(
	    pIFileMgr, filename, (OpenFileMode)flags)) == NULL) {
		FILE_MANAGER_ERR(NULL,
		    pIFileMgr, filename, "IFILEMGR_OpenFile", ret);
		__os_set_errno(ret);
	}

	IFILEMGR_Release(pIFileMgr);
	return (pIFile);
}
