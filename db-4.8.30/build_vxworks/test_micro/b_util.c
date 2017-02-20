/*
 * See the file LICENSE for redistribution information.
 *
 * Copyright (c) 2005-2009 Oracle.  All rights reserved.
 *
 * $Id$
 */

#include "bench.h"

static int b_util_testdir_remove __P((char *));

int
b_util_have_hash()
{
#if defined(HAVE_HASH) ||\
    DB_VERSION_MAJOR < 4 || DB_VERSION_MAJOR == 4 && DB_VERSION_MINOR < 2
	return (0);
#else
	fprintf(stderr,
    "library build did not include support for the Hash access method\n");
	return (1);
#endif
}

int
b_util_have_queue()
{
#if defined(HAVE_QUEUE) ||\
    DB_VERSION_MAJOR < 4 || DB_VERSION_MAJOR == 4 && DB_VERSION_MINOR < 2
	return (0);
#else
	fprintf(stderr,
    "library build did not include support for the Queue access method\n");
	return (1);
#endif
}

/*
 * b_util_dir_setup --
 *	Create the test directory.
 */
int
b_util_dir_setup()
{
	int ret;

#if DB_VERSION_MAJOR > 4 || DB_VERSION_MAJOR == 4 && DB_VERSION_MINOR > 3
	if ((ret = __os_mkdir(NULL, TESTDIR, 0755)) != 0) {
#else
	if ((ret = mkdir(TESTDIR, 0755)) != 0) {
#endif
		fprintf(stderr,
		    "%s: %s: %s\n", progname, TESTDIR, db_strerror(ret));
		return (1);
	}
	return (0);
}

#if DB_VERSION_MAJOR > 4 || DB_VERSION_MAJOR == 4 && DB_VERSION_MINOR > 4
#define	OS_EXISTS(a, b, c)	__os_exists(a, b, c)
#else
#define	OS_EXISTS(a, b, c)	__os_exists(b, c)
#endif

/*
 * b_util_dir_teardown
 *	Clean up the test directory.
 */
int
b_util_dir_teardown()
{
	int ret;

	if (OS_EXISTS(NULL, TESTFILE, NULL) == 0 &&
	    (ret = b_util_unlink(TESTFILE)) != 0) {
		fprintf(stderr,
		    "%s: %s: %s\n", progname, TESTFILE, db_strerror(ret));
		return (1);
	}
	return (b_util_testdir_remove(TESTDIR) ? 1 : 0);
}

/*
 * testdir_remove --
 *	Remove a directory and all its contents, the "dir" must contain no
 *	subdirectories, because testdir_remove will not recursively delete
 *	all subdirectories.
 */
static int
b_util_testdir_remove(dir)
	char *dir;
{
	int cnt, i, isdir, ret;
	char buf[1024], **names;

	ret = 0;

	/* If the directory doesn't exist, we're done. */
	if (OS_EXISTS(NULL, dir, &isdir) != 0)
		return (0);

	/* Get a list of the directory contents. */
#if DB_VERSION_MAJOR > 4 || DB_VERSION_MAJOR == 4 && DB_VERSION_MINOR > 6
	if ((ret = __os_dirlist(NULL, dir, 0, &names, &cnt)) != 0)
		return (ret);
#else
	if ((ret = __os_dirlist(NULL, dir, &names, &cnt)) != 0)
		return (ret);
#endif
	/* Go through the file name list, remove each file in the list */
	for (i = 0; i < cnt; ++i) {
		(void)snprintf(buf, sizeof(buf),
		    "%s%c%s", dir, PATH_SEPARATOR[0], names[i]);
		if ((ret = OS_EXISTS(NULL, buf, &isdir)) != 0)
			goto file_err;
		if (!isdir && (ret = b_util_unlink(buf)) != 0) {
file_err:		fprintf(stderr, "%s: %s: %s\n",
			    progname, buf, db_strerror(ret));
			break;
		}
	}

	__os_dirfree(NULL, names, cnt);

	/*
	 * If we removed the contents of the directory, remove the directory
	 * itself.
	 */
	if (i == cnt && (ret = rmdir(dir)) != 0)
		fprintf(stderr,
		    "%s: %s: %s\n", progname, dir, db_strerror(errno));
	return (ret);
}

void
b_util_abort()
{
#if DB_VERSION_MAJOR < 4 || DB_VERSION_MAJOR == 4 && DB_VERSION_MINOR < 6
	abort();
#elif DB_VERSION_MAJOR == 4 && DB_VERSION_MINOR == 6
	__os_abort();
#else
	__os_abort(NULL);
#endif
}

int
b_util_unlink(path)
	char *path;
{
#if DB_VERSION_MAJOR < 4 || DB_VERSION_MAJOR == 4 && DB_VERSION_MINOR < 7
	return (__os_unlink(NULL, path));
#else
	return (__os_unlink(NULL, path, 0));
#endif
}
