/*-
 * See the file LICENSE for redistribution information.
 *
 * Copyright (c) 1996-2009 Oracle.  All rights reserved.
 *
 * $Id$
 */

#include "db_config.h"

#include "db_int.h"

static int __db_fullpath
    __P((ENV *, const char *, const char *, int, int, char **));

#define	DB_ADDSTR(add) {						\
	/*								\
	 * The string might be NULL or zero-length, and the p[-1]	\
	 * might indirect to before the beginning of our buffer.	\
	 */								\
	if ((add) != NULL && (add)[0] != '\0') {			\
		/* If leading slash, start over. */			\
		if (__os_abspath(add)) {				\
			p = str;					\
			slash = 0;					\
		}							\
		/* Append to the current string. */			\
		len = strlen(add);					\
		if (slash)						\
			*p++ = PATH_SEPARATOR[0];			\
		memcpy(p, add, len);					\
		p += len;						\
		slash = strchr(PATH_SEPARATOR, p[-1]) == NULL;		\
	}								\
}

/*
 * __db_fullpath --
 *	Constructs a path name relative to the environment home, and optionally
 *	checks whether the file or directory exist.
 */
static int
__db_fullpath(env, dir, file, check_file, check_dir, namep)
	ENV *env;
	const char *dir;
	const char *file;
	int check_file;
	int check_dir;
	char **namep;
{
	size_t len;
	const char *home;
	char *p, *str;
	int isdir, ret, slash;

	/* All paths are relative to the environment home. */
	home = (env == NULL) ? NULL : env->db_home;

	len =
	    (home == NULL ? 0 : strlen(home) + 1) +
	    (dir == NULL ? 0 : strlen(dir) + 1) +
	    (file == NULL ? 0 : strlen(file) + 1);

	if ((ret = __os_malloc(env, len, &str)) != 0)
		return (ret);

	slash = 0;
	p = str;
	DB_ADDSTR(home);
	DB_ADDSTR(dir);
	*p = '\0';
	if (check_dir && (__os_exists(env, str, &isdir) != 0 || !isdir)) {
		__os_free(env, str);
		return (ENOENT);
	}
	DB_ADDSTR(file);
	*p = '\0';

	/*
	 * If we're opening a data file, see if it exists.  If not, keep
	 * trying.
	 */
	if (check_file && __os_exists(env, str, NULL) != 0) {
		__os_free(env, str);
		return (ENOENT);
	}

	if (namep == NULL)
		__os_free(env, str);
	else
		*namep = str;
	return (0);
}

#define	DB_CHECKFILE(file, dir, check_file, check_dir, namep, ret_dir) do { \
	ret = __db_fullpath(env, dir, file,				\
			check_file, check_dir, namep);			\
	if (ret == 0 && (ret_dir) != NULL)				\
		*(ret_dir) = (dir);					\
	if (ret != ENOENT)						\
		return (ret);						\
} while (0)

/*
 * __db_appname --
 *	Given an optional DB environment, directory and file name and type
 *	of call, build a path based on the ENV->open rules, and return
 *	it in allocated space.  Dirp can be used to specify a data directory
 *	to use.  If not and one is used then drip will contain a pointer
 *	to the directory name.
 *
 * PUBLIC: int __db_appname __P((ENV *, APPNAME,
 * PUBLIC:    const char *, const char **, char **));
 */
int
__db_appname(env, appname, file, dirp, namep)
	ENV *env;
	APPNAME appname;
	const char *file;
	const char **dirp;
	char **namep;
{
	DB_ENV *dbenv;
	char **ddp;
	const char *dir;
	int ret;

	dbenv = env->dbenv;
	dir = NULL;

	if (namep != NULL)
		*namep = NULL;

	/*
	 * Absolute path names are never modified.  If the file is an absolute
	 * path, we're done.
	 */
	if (file != NULL && __os_abspath(file))
		return (__os_strdup(env, file, namep));

	/*
	 * DB_APP_NONE:
	 *      DB_HOME/file
	 * DB_APP_DATA:
	 *      DB_HOME/DB_DATA_DIR/file
	 * DB_APP_LOG:
	 *      DB_HOME/DB_LOG_DIR/file
	 * DB_APP_TMP:
	 *      DB_HOME/DB_TMP_DIR/<create>
	 */
	switch (appname) {
	case DB_APP_NONE:
		break;
	case DB_APP_RECOVER:
	case DB_APP_DATA:
		/*
		 * First, step through the data_dir entries, if any, looking
		 * for the file.
		 */
		if (dbenv != NULL && dbenv->db_data_dir != NULL)
			for (ddp = dbenv->db_data_dir; *ddp != NULL; ddp++)
				DB_CHECKFILE(file, *ddp, 1, 0, namep, dirp);

		/* Second, look in the environment home directory. */
		DB_CHECKFILE(file, NULL, 1, 0, namep, dirp);

		/*
		 * Otherwise, we're going to create.  Use the specified
		 * directory unless we're in recovery and it doesn't exist.
		 */
		if (dirp != NULL && *dirp != NULL)
			DB_CHECKFILE(file, *dirp, 0,
			    appname == DB_APP_RECOVER, namep, dirp);

		/* Finally, use the create directory, if set. */
		if (dbenv != NULL && dbenv->db_create_dir != NULL)
			dir = dbenv->db_create_dir;
		break;
	case DB_APP_LOG:
		if (dbenv != NULL)
			dir = dbenv->db_log_dir;
		break;
	case DB_APP_TMP:
		if (dbenv != NULL)
			dir = dbenv->db_tmp_dir;
		break;
	}

	/*
	 * Construct the full path.  For temporary files, it is an error if the
	 * directory does not exist: if it doesn't, checking whether millions
	 * of temporary files exist inside it takes a *very* long time.
	 */
	DB_CHECKFILE(file, dir, 0, appname == DB_APP_TMP, namep, dirp);

	return (ret);
}

/*
 * __db_tmp_open --
 *	Create a temporary file.
 *
 * PUBLIC: int __db_tmp_open __P((ENV *, u_int32_t, DB_FH **));
 */
int
__db_tmp_open(env, oflags, fhpp)
	ENV *env;
	u_int32_t oflags;
	DB_FH **fhpp;
{
	pid_t pid;
	int filenum, i, ret;
	char *path;
	char *firstx, *trv;

	DB_ASSERT(env, fhpp != NULL);
	*fhpp = NULL;

#define	DB_TRAIL	"BDBXXXXX"
	if ((ret = __db_appname(env, DB_APP_TMP, DB_TRAIL, NULL, &path)) != 0)
		goto done;

	/* Replace the X's with the process ID (in decimal). */
	__os_id(env->dbenv, &pid, NULL);
	for (trv = path + strlen(path); *--trv == 'X'; pid /= 10)
		*trv = '0' + (u_char)(pid % 10);
	firstx = trv + 1;

	/* Loop, trying to open a file. */
	for (filenum = 1;; filenum++) {
		if ((ret = __os_open(env, path, 0,
		    oflags | DB_OSO_CREATE | DB_OSO_EXCL | DB_OSO_TEMP,
		    DB_MODE_600, fhpp)) == 0) {
			ret = 0;
			goto done;
		}

		/*
		 * !!!:
		 * If we don't get an EEXIST error, then there's something
		 * seriously wrong.  Unfortunately, if the implementation
		 * doesn't return EEXIST for O_CREAT and O_EXCL regardless
		 * of other possible errors, we've lost.
		 */
		if (ret != EEXIST) {
			__db_err(env, ret, "temporary open: %s", path);
			goto done;
		}

		/*
		 * Generate temporary file names in a backwards-compatible way.
		 * If pid == 12345, the result is:
		 *   <path>/DB12345 (tried above, the first time through).
		 *   <path>/DBa2345 ...  <path>/DBz2345
		 *   <path>/DBaa345 ...  <path>/DBaz345
		 *   <path>/DBba345, and so on.
		 *
		 * XXX
		 * This algorithm is O(n**2) -- that is, creating 100 temporary
		 * files requires 5,000 opens, creating 1000 files requires
		 * 500,000.  If applications open a lot of temporary files, we
		 * could improve performance by switching to timestamp-based
		 * file names.
		 */
		for (i = filenum, trv = firstx; i > 0; i = (i - 1) / 26)
			if (*trv++ == '\0') {
				ret = EINVAL;
				goto done;
			}

		for (i = filenum; i > 0; i = (i - 1) / 26)
			*--trv = 'a' + ((i - 1) % 26);
	}
done:
	__os_free(env, path);
	return (ret);
}
