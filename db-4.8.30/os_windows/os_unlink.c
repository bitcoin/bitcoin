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
 * __os_unlink --
 *	Remove a file.
 */
int
__os_unlink(env, path, overwrite_test)
	ENV *env;
	const char *path;
	int overwrite_test;
{
	DB_ENV *dbenv;
	HANDLE h;
	_TCHAR *tpath, *orig_tpath, buf[DB_MAXPATHLEN];
	u_int32_t id;
	int ret, t_ret;

	dbenv = env == NULL ? NULL : env->dbenv;

	if (dbenv != NULL &&
	    FLD_ISSET(dbenv->verbose, DB_VERB_FILEOPS | DB_VERB_FILEOPS_ALL))
		__db_msg(env, "fileops: unlink %s", path);

	/* Optionally overwrite the contents of the file to enhance security. */
	if (dbenv != NULL && overwrite_test && F_ISSET(dbenv, DB_ENV_OVERWRITE))
		(void)__db_file_multi_write(env, path);

	TO_TSTRING(env, path, tpath, ret);
	if (ret != 0)
		return (ret);
	orig_tpath = tpath;

	LAST_PANIC_CHECK_BEFORE_IO(env);

	/*
	 * Windows NT and its descendants allow removal of open files, but the
	 * DeleteFile Win32 system call isn't equivalent to a POSIX unlink.
	 * Firstly, it only succeeds if FILE_SHARE_DELETE is set when the file
	 * is opened.  Secondly, it leaves the file in a "zombie" state, where
	 * it can't be opened again, but a new file with the same name can't be
	 * created either.
	 *
	 * Since we depend on being able to recreate files (during recovery,
	 * say), we have to first rename the file, and then delete it.  It
	 * still hangs around, but with a name we don't care about.  The rename
	 * will fail if the file doesn't exist, which isn't a problem, but if
	 * it fails for some other reason, we need to know about it or a
	 * subsequent open may fail for no apparent reason.
	 */
	if (__os_is_winnt()) {
		__os_unique_id(env, &id);
		_sntprintf(buf, DB_MAXPATHLEN, _T("%s.del.%010u"), tpath, id);
		if (MoveFile(tpath, buf))
			tpath = buf;
		else {
			ret = __os_get_syserr();
			if (__os_posix_err(ret) != ENOENT)
				__db_err(env, ret,
				    "MoveFile: rename %s to temporary file",
				    path);
		}

		/*
		 * Try removing the file using the delete-on-close flag.  This
		 * plays nicer with files that are still open than DeleteFile.
		 */
		h = CreateFile(tpath, 0,
		    FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
		    NULL, OPEN_EXISTING, FILE_FLAG_DELETE_ON_CLOSE, 0);
		if (h != INVALID_HANDLE_VALUE) {
			(void)CloseHandle (h);
			if (GetFileAttributes(tpath) == INVALID_FILE_ATTRIBUTES)
				goto skipdel;
		}
	}

	RETRY_CHK((!DeleteFile(tpath)), ret);

skipdel:
	FREE_STRING(env, orig_tpath);

	/*
	 * XXX
	 * We shouldn't be testing for an errno of ENOENT here, but ENOENT
	 * signals that a file is missing, and we attempt to unlink things
	 * (such as v. 2.x environment regions, in ENV->remove) that we
	 * are expecting not to be there.  Reporting errors in these cases
	 * is annoying.
	 */
	if (ret != 0) {
		if ((t_ret = __os_posix_err(ret)) != ENOENT)
			__db_syserr(env, ret, "DeleteFile: %s", path);
		ret = t_ret;
	}

	return (ret);
}
