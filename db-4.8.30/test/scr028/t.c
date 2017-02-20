/*-
 * See the file LICENSE for redistribution information.
 *
 * Copyright (c) 2002-2009 Oracle.  All rights reserved.
 */

#include <sys/types.h>

#include <ctype.h>
#include <errno.h>
#include <fcntl.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <strings.h>
/* added to get clean compile on linux blade server else pread undefined */
#ifdef __linux__
#define __USE_UNIX98
#endif
#include <unistd.h>

#include "db.h"

#define	E(api, func, name) {						\
	if ((ret = api(func)) != 0) {					\
		fprintf(stderr, "%s: %s", name, db_strerror(ret));	\
		return (1);						\
	}								\
}

#define	F(api, func1, func2, name) {						\
	if ((ret = api(func1, func2)) != 0) {					\
		fprintf(stderr, "%s: %s", name, db_strerror(ret));	\
		return (1);						\
	}								\
}

void
dirfree(char **namesp, int cnt)
{ return; }
int
dirlist(const char *dir, char ***namesp, int *cntp)
{ return (0); }
int
exists(const char *path, int *isdirp)
{ return (0); }
int
ioinfo(const char *path,
    int fd, u_int32_t *mbytesp, u_int32_t *bytesp, u_int32_t *iosizep)
{ return (0); }
int
file_map(DB_ENV *dbenv, char *path, size_t len, int is_readonly, void **addr)
{ return (0); }
int
region_map(DB_ENV *dbenv, char *path, size_t len, int *is_create, void **addr)
{ return (0); }
int
seek(int fd, off_t offset, int whence)
{ return (0); }
int
local_sleep(u_long seconds, u_long microseconds)
{ return (0); }
int
unmap(DB_ENV *dbenv, void *addr)
{ return (0); }

int
main(int argc, char *argv[])
{
	int ret;

	E(db_env_set_func_close, close, "close");
	E(db_env_set_func_dirfree, dirfree, "dirfree");
	E(db_env_set_func_dirlist, dirlist, "dirlist");
	E(db_env_set_func_exists, exists, "exists");
	F(db_env_set_func_file_map, file_map, unmap, "file map");
	E(db_env_set_func_free, free, "free");
	E(db_env_set_func_fsync, fsync, "fsync");
	E(db_env_set_func_ftruncate, ftruncate, "ftruncate");
	E(db_env_set_func_ioinfo, ioinfo, "ioinfo");
	E(db_env_set_func_malloc, malloc, "malloc");
	E(db_env_set_func_open, open, "open");
	E(db_env_set_func_pread, pread, "pread");
	E(db_env_set_func_pwrite, pwrite, "pwrite");
	E(db_env_set_func_read, read, "read");
	E(db_env_set_func_realloc, realloc, "realloc");
	F(db_env_set_func_region_map, region_map, unmap, "region map");
	E(db_env_set_func_rename, rename, "rename");
	E(db_env_set_func_seek, seek, "seek");
	E(db_env_set_func_unlink, unlink, "unlink");
	E(db_env_set_func_write, write, "write");
	E(db_env_set_func_yield, local_sleep, "sleep/yield");

	return (0);
}
