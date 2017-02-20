/*-
 * See the file LICENSE for redistribution information.
 *
 * Copyright (c) 1997-2009 Oracle.  All rights reserved.
 *
 * $Id$
 */

#include "db_config.h"

#define	__INCLUDE_DIRECTORY	1
#include "db_int.h"

/*
 * __os_dirlist --
 *	Return a list of the files in a directory.
 *
 * PUBLIC: int __os_dirlist __P((ENV *, const char *, int, char ***, int *));
 */
int
__os_dirlist(env, dir, returndir, namesp, cntp)
	ENV *env;
	const char *dir;
	int returndir, *cntp;
	char ***namesp;
{
	DB_ENV *dbenv;
	struct dirent *dp;
	DIR *dirp;
	struct stat sb;
	int arraysz, cnt, ret;
	char **names, buf[DB_MAXPATHLEN];

	*namesp = NULL;
	*cntp = 0;

	dbenv = env == NULL ? NULL : env->dbenv;

	if (dbenv != NULL &&
	    FLD_ISSET(dbenv->verbose, DB_VERB_FILEOPS | DB_VERB_FILEOPS_ALL))
		__db_msg(env, "fileops: directory list %s", dir);

	if (DB_GLOBAL(j_dirlist) != NULL)
		return (DB_GLOBAL(j_dirlist)(dir, namesp, cntp));

	if ((dirp = opendir(CHAR_STAR_CAST dir)) == NULL)
		return (__os_get_errno());
	names = NULL;
	for (arraysz = cnt = 0; (dp = readdir(dirp)) != NULL;) {
		snprintf(buf, sizeof(buf), "%s/%s", dir, dp->d_name);

		RETRY_CHK(stat(buf, &sb), ret);
		if (ret != 0) {
			ret = __os_posix_err(ret);
			goto err;
		}

		/*
		 * We return regular files, and optionally return directories
		 * (except for dot and dot-dot).
		 *
		 * Shared memory files are of a different type on QNX, and we
		 * return those as well.
		 */
#ifdef HAVE_QNX
		if (!S_ISREG(sb.st_mode) && !S_TYPEISSHM(&sb)) {
#else
		if (!S_ISREG(sb.st_mode)) {
#endif
			if (!returndir || !S_ISDIR(sb.st_mode))
				continue;
			if (dp->d_name[0] == '.' && (dp->d_name[1] == '\0' ||
			    (dp->d_name[1] == '.' && dp->d_name[2] == '\0')))
				continue;
		}

		if (cnt >= arraysz) {
			arraysz += 100;
			if ((ret = __os_realloc(env,
			    (u_int)arraysz * sizeof(names[0]), &names)) != 0)
				goto err;
		}
		if ((ret = __os_strdup(env, dp->d_name, &names[cnt])) != 0)
			goto err;
		cnt++;
	}
	(void)closedir(dirp);

	*namesp = names;
	*cntp = cnt;
	return (0);

err:	if (names != NULL)
		__os_dirfree(env, names, cnt);
	if (dirp != NULL)
		(void)closedir(dirp);
	return (ret);
}

/*
 * __os_dirfree --
 *	Free the list of files.
 *
 * PUBLIC: void __os_dirfree __P((ENV *, char **, int));
 */
void
__os_dirfree(env, names, cnt)
	ENV *env;
	char **names;
	int cnt;
{
	if (DB_GLOBAL(j_dirfree) != NULL)
		DB_GLOBAL(j_dirfree)(names, cnt);
	else {
		while (cnt > 0)
			__os_free(env, names[--cnt]);
		__os_free(env, names);
	}
}
