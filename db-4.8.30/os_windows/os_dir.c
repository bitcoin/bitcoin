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
 * __os_dirlist --
 *	Return a list of the files in a directory.
 */
int
__os_dirlist(env, dir, returndir, namesp, cntp)
	ENV *env;
	const char *dir;
	int returndir, *cntp;
	char ***namesp;
{
	HANDLE dirhandle;
	WIN32_FIND_DATA fdata;
	int arraysz, cnt, ret;
	char **names, *onename;
	_TCHAR tfilespec[DB_MAXPATHLEN + 1];
	_TCHAR *tdir;

	*namesp = NULL;
	*cntp = 0;

	TO_TSTRING(env, dir, tdir, ret);
	if (ret != 0)
		return (ret);

	(void)_sntprintf(tfilespec, DB_MAXPATHLEN,
	    _T("%s%hc*"), tdir, PATH_SEPARATOR[0]);

	/*
	 * On WinCE, FindFirstFile will return INVALID_HANDLE_VALUE when
	 * the searched directory is empty, and set last error to
	 * ERROR_NO_MORE_FILES, on Windows it will return "." instead.
	 */
	if ((dirhandle =
	    FindFirstFile(tfilespec, &fdata)) == INVALID_HANDLE_VALUE) {
		if (GetLastError() == ERROR_NO_MORE_FILES)
			return (0);
		return (__os_posix_err(__os_get_syserr()));
	}

	names = NULL;
	arraysz = cnt = ret = 0;
	for (;;) {
		if (returndir ||
		    (fdata.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) == 0) {
			if (fdata.cFileName[0] == _T('.') &&
			    (fdata.cFileName[1] == _T('\0') ||
			    (fdata.cFileName[1] == _T('.') &&
			    fdata.cFileName[2] == _T('\0'))))
				goto next;
			if (cnt >= arraysz) {
				arraysz += 100;
				if ((ret = __os_realloc(env,
				    arraysz * sizeof(names[0]), &names)) != 0)
					goto err;
			}
			/*
			 * FROM_TSTRING doesn't necessarily allocate new
			 * memory, so we must do that explicitly.
			 * Unfortunately, when compiled with UNICODE, we'll
			 * copy twice.
			 */
			FROM_TSTRING(env, fdata.cFileName, onename, ret);
			if (ret != 0)
				goto err;
			ret = __os_strdup(env, onename, &names[cnt]);
			FREE_STRING(env, onename);
			if (ret != 0)
				goto err;
			cnt++;
		}
next:
		if (!FindNextFile(dirhandle, &fdata)) {
			if (GetLastError() == ERROR_NO_MORE_FILES)
				break;
			else {
				ret = __os_posix_err(__os_get_syserr());
				goto err;
			}
		}
	}

err:	if (!FindClose(dirhandle) && ret == 0)
		ret = __os_posix_err(__os_get_syserr());

	if (ret == 0) {
		*namesp = names;
		*cntp = cnt;
	} else if (names != NULL)
		__os_dirfree(env, names, cnt);

	FREE_STRING(env, tdir);

	return (ret);
}

/*
 * __os_dirfree --
 *	Free the list of files.
 */
void
__os_dirfree(env, names, cnt)
	ENV *env;
	char **names;
	int cnt;
{
	while (cnt > 0)
		__os_free(env, names[--cnt]);
	__os_free(env, names);
}
