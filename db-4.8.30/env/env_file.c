/*-
 * See the file LICENSE for redistribution information.
 *
 * Copyright (c) 2002-2009 Oracle.  All rights reserved.
 *
 * $Id$
 */

#include "db_config.h"

#include "db_int.h"

/*
 * __db_file_extend --
 *	Initialize a regular file by writing the last page of the file.
 *
 * PUBLIC: int __db_file_extend __P((ENV *, DB_FH *, size_t));
 */
int
__db_file_extend(env, fhp, size)
	ENV *env;
	DB_FH *fhp;
	size_t size;
{
	db_pgno_t pages;
	size_t nw;
	u_int32_t relative;
	int ret;
	char *buf;

	/*
	 * Extend the file by writing the last page.  If the region is >4Gb,
	 * increment may be larger than the maximum possible seek "relative"
	 * argument, as it's an unsigned 32-bit value.  Break the offset into
	 * pages of 1MB each so we don't overflow -- (2^20 + 2^32 is bigger
	 * than any memory I expect to see for awhile).
	 */
#undef	FILE_EXTEND_IO_SIZE
#define	FILE_EXTEND_IO_SIZE	(8 * 1024)
	if ((ret = __os_calloc(env, FILE_EXTEND_IO_SIZE, 1, &buf)) != 0)
		return (ret);

	pages = (db_pgno_t)((size - FILE_EXTEND_IO_SIZE) / MEGABYTE);
	relative = (u_int32_t)((size - FILE_EXTEND_IO_SIZE) % MEGABYTE);
	if ((ret = __os_seek(env, fhp, pages, MEGABYTE, relative)) != 0)
		goto err;
	if ((ret = __os_write(env, fhp, buf, FILE_EXTEND_IO_SIZE, &nw)) != 0)
		goto err;

err:	__os_free(env, buf);

	return (0);
}

/*
 * __db_file_multi_write  --
 *	Overwrite a file with multiple passes to corrupt the data.
 *
 * PUBLIC: int __db_file_multi_write __P((ENV *, const char *));
 */
int
__db_file_multi_write(env, path)
	ENV *env;
	const char *path;
{
	DB_FH *fhp;
	u_int32_t mbytes, bytes;
	int ret;

	if ((ret = __os_open(env, path, 0, DB_OSO_REGION, 0, &fhp)) == 0 &&
	    (ret = __os_ioinfo(env, path, fhp, &mbytes, &bytes, NULL)) == 0) {
		/*
		 * !!!
		 * Overwrite a regular file with alternating 0xff, 0x00 and 0xff
		 * byte patterns.  Implies a fixed-block filesystem, journaling
		 * or logging filesystems will require operating system support.
		 */
		if ((ret =
		    __db_file_write(env, fhp, mbytes, bytes, 255)) != 0)
			goto err;
		if ((ret =
		    __db_file_write(env, fhp, mbytes, bytes, 0)) != 0)
			goto err;
		if ((ret =
		    __db_file_write(env, fhp, mbytes, bytes, 255)) != 0)
			goto err;
	} else
		__db_err(env, ret, "%s", path);

err:	if (fhp != NULL)
		(void)__os_closehandle(env, fhp);
	return (ret);
}

/*
 * __db_file_write --
 *	A single pass over the file, writing the specified byte pattern.
 *
 * PUBLIC: int __db_file_write __P((ENV *,
 * PUBLIC:     DB_FH *, u_int32_t, u_int32_t, int));
 */
int
__db_file_write(env, fhp, mbytes, bytes, pattern)
	ENV *env;
	DB_FH *fhp;
	int pattern;
	u_int32_t mbytes, bytes;
{
	size_t len, nw;
	int i, ret;
	char *buf;

#undef	FILE_WRITE_IO_SIZE
#define	FILE_WRITE_IO_SIZE	(64 * 1024)
	if ((ret = __os_malloc(env, FILE_WRITE_IO_SIZE, &buf)) != 0)
		return (ret);
	memset(buf, pattern, FILE_WRITE_IO_SIZE);

	if ((ret = __os_seek(env, fhp, 0, 0, 0)) != 0)
		goto err;
	for (; mbytes > 0; --mbytes)
		for (i = MEGABYTE / FILE_WRITE_IO_SIZE; i > 0; --i)
			if ((ret = __os_write(
			    env, fhp, buf, FILE_WRITE_IO_SIZE, &nw)) != 0)
				goto err;
	for (; bytes > 0; bytes -= (u_int32_t)len) {
		len = bytes < FILE_WRITE_IO_SIZE ? bytes : FILE_WRITE_IO_SIZE;
		if ((ret = __os_write(env, fhp, buf, len, &nw)) != 0)
			goto err;
	}

	ret = __os_fsync(env, fhp);

err:	__os_free(env, buf);
	return (ret);
}
