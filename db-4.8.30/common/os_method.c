/*-
 * See the file LICENSE for redistribution information.
 *
 * Copyright (c) 1999-2009 Oracle.  All rights reserved.
 *
 * $Id$
 */

#include "db_config.h"

#include "db_int.h"

/*
 * EXTERN: int db_env_set_func_close __P((int (*)(int)));
 */
int
db_env_set_func_close(func_close)
	int (*func_close) __P((int));
{
	DB_GLOBAL(j_close) = func_close;
	return (0);
}

/*
 * EXTERN: int db_env_set_func_dirfree __P((void (*)(char **, int)));
 */
int
db_env_set_func_dirfree(func_dirfree)
	void (*func_dirfree) __P((char **, int));
{
	DB_GLOBAL(j_dirfree) = func_dirfree;
	return (0);
}

/*
 * EXTERN: int db_env_set_func_dirlist
 * EXTERN:     __P((int (*)(const char *, char ***, int *)));
 */
int
db_env_set_func_dirlist(func_dirlist)
	int (*func_dirlist) __P((const char *, char ***, int *));
{
	DB_GLOBAL(j_dirlist) = func_dirlist;
	return (0);
}

/*
 * EXTERN: int db_env_set_func_exists __P((int (*)(const char *, int *)));
 */
int
db_env_set_func_exists(func_exists)
	int (*func_exists) __P((const char *, int *));
{
	DB_GLOBAL(j_exists) = func_exists;
	return (0);
}

/*
 * EXTERN: int db_env_set_func_free __P((void (*)(void *)));
 */
int
db_env_set_func_free(func_free)
	void (*func_free) __P((void *));
{
	DB_GLOBAL(j_free) = func_free;
	return (0);
}

/*
 * EXTERN: int db_env_set_func_fsync __P((int (*)(int)));
 */
int
db_env_set_func_fsync(func_fsync)
	int (*func_fsync) __P((int));
{
	DB_GLOBAL(j_fsync) = func_fsync;
	return (0);
}

/*
 * EXTERN: int db_env_set_func_ftruncate __P((int (*)(int, off_t)));
 */
int
db_env_set_func_ftruncate(func_ftruncate)
	int (*func_ftruncate) __P((int, off_t));
{
	DB_GLOBAL(j_ftruncate) = func_ftruncate;
	return (0);
}

/*
 * EXTERN: int db_env_set_func_ioinfo __P((int (*)(const char *,
 * EXTERN:     int, u_int32_t *, u_int32_t *, u_int32_t *)));
 */
int
db_env_set_func_ioinfo(func_ioinfo)
	int (*func_ioinfo)
	    __P((const char *, int, u_int32_t *, u_int32_t *, u_int32_t *));
{
	DB_GLOBAL(j_ioinfo) = func_ioinfo;
	return (0);
}

/*
 * EXTERN: int db_env_set_func_malloc __P((void *(*)(size_t)));
 */
int
db_env_set_func_malloc(func_malloc)
	void *(*func_malloc) __P((size_t));
{
	DB_GLOBAL(j_malloc) = func_malloc;
	return (0);
}

/*
 * EXTERN: int db_env_set_func_file_map
 * EXTERN:    __P((int (*)(DB_ENV *, char *, size_t, int, void **),
 * EXTERN:    int (*)(DB_ENV *, void *)));
 */
int
db_env_set_func_file_map(func_file_map, func_file_unmap)
	int (*func_file_map) __P((DB_ENV *, char *, size_t, int, void **));
	int (*func_file_unmap) __P((DB_ENV *, void *));
{
	DB_GLOBAL(j_file_map) = func_file_map;
	DB_GLOBAL(j_file_unmap) = func_file_unmap;
	return (0);
}

/*
 * EXTERN: int db_env_set_func_region_map
 * EXTERN:    __P((int (*)(DB_ENV *, char *, size_t, int *, void **),
 * EXTERN:    int (*)(DB_ENV *, void *)));
 */
int
db_env_set_func_region_map(func_region_map, func_region_unmap)
	int (*func_region_map) __P((DB_ENV *, char *, size_t, int *, void **));
	int (*func_region_unmap) __P((DB_ENV *, void *));
{
	DB_GLOBAL(j_region_map) = func_region_map;
	DB_GLOBAL(j_region_unmap) = func_region_unmap;
	return (0);
}

/*
 * EXTERN: int db_env_set_func_pread
 * EXTERN:    __P((ssize_t (*)(int, void *, size_t, off_t)));
 */
int
db_env_set_func_pread(func_pread)
	ssize_t (*func_pread) __P((int, void *, size_t, off_t));
{
	DB_GLOBAL(j_pread) = func_pread;
	return (0);
}

/*
 * EXTERN: int db_env_set_func_pwrite
 * EXTERN:    __P((ssize_t (*)(int, const void *, size_t, off_t)));
 */
int
db_env_set_func_pwrite(func_pwrite)
	ssize_t (*func_pwrite) __P((int, const void *, size_t, off_t));
{
	DB_GLOBAL(j_pwrite) = func_pwrite;
	return (0);
}

/*
 * EXTERN: int db_env_set_func_open __P((int (*)(const char *, int, ...)));
 */
int
db_env_set_func_open(func_open)
	int (*func_open) __P((const char *, int, ...));
{
	DB_GLOBAL(j_open) = func_open;
	return (0);
}

/*
 * EXTERN: int db_env_set_func_read __P((ssize_t (*)(int, void *, size_t)));
 */
int
db_env_set_func_read(func_read)
	ssize_t (*func_read) __P((int, void *, size_t));
{
	DB_GLOBAL(j_read) = func_read;
	return (0);
}

/*
 * EXTERN: int db_env_set_func_realloc __P((void *(*)(void *, size_t)));
 */
int
db_env_set_func_realloc(func_realloc)
	void *(*func_realloc) __P((void *, size_t));
{
	DB_GLOBAL(j_realloc) = func_realloc;
	return (0);
}

/*
 * EXTERN: int db_env_set_func_rename
 * EXTERN:     __P((int (*)(const char *, const char *)));
 */
int
db_env_set_func_rename(func_rename)
	int (*func_rename) __P((const char *, const char *));
{
	DB_GLOBAL(j_rename) = func_rename;
	return (0);
}

/*
 * EXTERN: int db_env_set_func_seek
 * EXTERN:     __P((int (*)(int, off_t, int)));
 */
int
db_env_set_func_seek(func_seek)
	int (*func_seek) __P((int, off_t, int));
{
	DB_GLOBAL(j_seek) = func_seek;
	return (0);
}

/*
 * EXTERN: int db_env_set_func_unlink __P((int (*)(const char *)));
 */
int
db_env_set_func_unlink(func_unlink)
	int (*func_unlink) __P((const char *));
{
	DB_GLOBAL(j_unlink) = func_unlink;
	return (0);
}

/*
 * EXTERN: int db_env_set_func_write
 * EXTERN:     __P((ssize_t (*)(int, const void *, size_t)));
 */
int
db_env_set_func_write(func_write)
	ssize_t (*func_write) __P((int, const void *, size_t));
{
	DB_GLOBAL(j_write) = func_write;
	return (0);
}

/*
 * EXTERN: int db_env_set_func_yield __P((int (*)(u_long, u_long)));
 */
int
db_env_set_func_yield(func_yield)
	int (*func_yield) __P((u_long, u_long));
{
	DB_GLOBAL(j_yield) = func_yield;
	return (0);
}
