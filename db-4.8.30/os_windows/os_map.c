/*-
 * See the file LICENSE for redistribution information.
 *
 * Copyright (c) 1996-2009 Oracle.  All rights reserved.
 *
 * $Id$
 */

#include "db_config.h"

#include "db_int.h"

static int __os_map
  __P((ENV *, char *, REGINFO *, DB_FH *, size_t, int, int, int, void **));
static int __os_unique_name __P((_TCHAR *, HANDLE, _TCHAR *, size_t));

/*
 * __os_attach --
 *	Create/join a shared memory region.
 */
int
__os_attach(env, infop, rp)
	ENV *env;
	REGINFO *infop;
	REGION *rp;
{
	DB_FH *fhp;
	int ret;

	/*
	 * On Windows/9X, files that are opened by multiple processes do not
	 * share data correctly.  For this reason, we require that DB_PRIVATE
	 * be specified on that platform.
	 */
	if (!F_ISSET(env, ENV_PRIVATE) && __os_is_winnt() == 0) {
		__db_err(env,
		    EINVAL, "Windows 9X systems must specify DB_PRIVATE");
		return (EINVAL);
	}

	/*
	 * Try to open/create the file.  We DO NOT need to ensure that multiple
	 * threads/processes attempting to simultaneously create the region are
	 * properly ordered, our caller has already taken care of that.
	 */
	if ((ret = __os_open(env, infop->name, 0, DB_OSO_REGION |
	    (F_ISSET(infop, REGION_CREATE_OK) ? DB_OSO_CREATE : 0),
	    env->db_mode, &fhp)) != 0) {
		__db_err(env, ret, "%s", infop->name);
		return (ret);
	}

	/*
	 * Map the file in.  If we're creating an in-system-memory region,
	 * specify a segment ID (which is never used again) so that the
	 * calling code writes out the REGENV_REF structure to the primary
	 * environment file.
	 */
	ret = __os_map(env, infop->name, infop, fhp, rp->size,
	   1, F_ISSET(env, ENV_SYSTEM_MEM), 0, &infop->addr);
	if (ret == 0 && F_ISSET(env, ENV_SYSTEM_MEM))
		rp->segid = 1;

	(void)__os_closehandle(env, fhp);

	return (ret);
}

/*
 * __os_detach --
 *	Detach from a shared memory region.
 */
int
__os_detach(env, infop, destroy)
	ENV *env;
	REGINFO *infop;
	int destroy;
{
	DB_ENV *dbenv;
	int ret, t_ret;

	dbenv = env->dbenv;

	if (infop->wnt_handle != NULL) {
		(void)CloseHandle(infop->wnt_handle);
		infop->wnt_handle = NULL;
	}

	ret = !UnmapViewOfFile(infop->addr) ? __os_get_syserr() : 0;
	if (ret != 0) {
		__db_syserr(env, ret, "UnmapViewOfFile");
		ret = __os_posix_err(ret);
	}

	if (!F_ISSET(env, ENV_SYSTEM_MEM) && destroy &&
	    (t_ret = __os_unlink(env, infop->name, 1)) != 0 && ret == 0)
		ret = t_ret;

	return (ret);
}

/*
 * __os_mapfile --
 *	Map in a shared memory file.
 */
int
__os_mapfile(env, path, fhp, len, is_rdonly, addr)
	ENV *env;
	char *path;
	DB_FH *fhp;
	int is_rdonly;
	size_t len;
	void **addr;
{
#ifdef DB_WINCE
	/*
	 * Windows CE has special requirements for file mapping to work.
	 * * The input handle needs to be opened using CreateFileForMapping
	 * * Concurrent access via a non mapped file is not supported.
	 * So we disable support for memory mapping files on Windows CE. It is
	 * currently only used as an optimization in mpool for small read only
	 * databases.
	 */
	return (EFAULT);
#else
	DB_ENV *dbenv;

	dbenv = env == NULL ? NULL : env->dbenv;

	if (dbenv != NULL &&
	    FLD_ISSET(dbenv->verbose, DB_VERB_FILEOPS | DB_VERB_FILEOPS_ALL))
		__db_msg(env, "fileops: mmap %s", path);
	return (__os_map(env, path, NULL, fhp, len, 0, 0, is_rdonly, addr));
#endif
}

/*
 * __os_unmapfile --
 *	Unmap the shared memory file.
 */
int
__os_unmapfile(env, addr, len)
	ENV *env;
	void *addr;
	size_t len;
{
	DB_ENV *dbenv;

	dbenv = env == NULL ? NULL : env->dbenv;

	if (dbenv != NULL &&
	    FLD_ISSET(dbenv->verbose, DB_VERB_FILEOPS | DB_VERB_FILEOPS_ALL))
		__db_msg(env, "fileops: munmap");

	return (!UnmapViewOfFile(addr) ? __os_posix_err(__os_get_syserr()) : 0);
}

/*
 * __os_unique_name --
 *	Create a unique identifying name from a pathname (may be absolute or
 *	relative) and/or a file descriptor.
 *
 *	The name returned must be unique (different files map to different
 *	names), and repeatable (same files, map to same names).  It's not
 *	so easy to do by name.  Should handle not only:
 *
 *		foo.bar == ./foo.bar == c:/whatever_path/foo.bar
 *
 *	but also understand that:
 *
 *		foo.bar == Foo.Bar	(FAT file system)
 *		foo.bar != Foo.Bar	(NTFS)
 *
 *	The best solution is to use the file index, found in the file
 *	information structure (similar to UNIX inode #).
 *
 *	When a file is deleted, its file index may be reused,
 *	but if the unique name has not gone from its namespace,
 *	we may get a conflict.  So to ensure some tie in to the
 *	original pathname, we also use the creation time and the
 *	file basename.  This is not a perfect system, but it
 *	should work for all but anamolous test cases.
 *
 */
static int
__os_unique_name(orig_path, hfile, result_path, result_path_len)
	_TCHAR *orig_path, *result_path;
	HANDLE hfile;
	size_t result_path_len;
{
	BY_HANDLE_FILE_INFORMATION fileinfo;
	_TCHAR *basename, *p;

	/*
	 * In Windows, pathname components are delimited by '/' or '\', and
	 * if neither is present, we need to strip off leading drive letter
	 * (e.g. c:foo.txt).
	 */
	basename = _tcsrchr(orig_path, '/');
	p = _tcsrchr(orig_path, '\\');
	if (basename == NULL || (p != NULL && p > basename))
		basename = p;
	if (basename == NULL)
		basename = _tcsrchr(orig_path, ':');

	if (basename == NULL)
		basename = orig_path;
	else
		basename++;

	if (!GetFileInformationByHandle(hfile, &fileinfo))
		return (__os_posix_err(__os_get_syserr()));

	(void)_sntprintf(result_path, result_path_len,
	    _T("__db_shmem.%8.8lx.%8.8lx.%8.8lx.%8.8lx.%8.8lx.%s"),
	    fileinfo.dwVolumeSerialNumber,
	    fileinfo.nFileIndexHigh,
	    fileinfo.nFileIndexLow,
	    fileinfo.ftCreationTime.dwHighDateTime,
	    fileinfo.ftCreationTime.dwHighDateTime,
	    basename);

	return (0);
}

/*
 * __os_map --
 *	The mmap(2) function for Windows.
 */
static int
__os_map(env, path, infop, fhp, len, is_region, is_system, is_rdonly, addr)
	ENV *env;
	REGINFO *infop;
	char *path;
	DB_FH *fhp;
	int is_region, is_system, is_rdonly;
	size_t len;
	void **addr;
{
	HANDLE hMemory;
	int ret, use_pagefile;
	_TCHAR *tpath, shmem_name[DB_MAXPATHLEN];
	void *pMemory;
	unsigned __int64 len64;

	ret = 0;
	if (infop != NULL)
		infop->wnt_handle = NULL;

	/*
	 * On 64 bit systems, len is already a 64 bit value.
	 * On 32 bit systems len is a 32 bit value.
	 * Always convert to a 64 bit value, so that the high order
	 * DWORD can be simply extracted on 64 bit platforms.
	 */
	len64 = len;

	use_pagefile = is_region && is_system;

	/*
	 * If creating a region in system space, get a matching name in the
	 * paging file namespace.
	 */
	if (use_pagefile) {
#ifdef DB_WINCE
		__db_errx(env, "Unable to memory map regions using system "
		    "memory on WinCE.");
		return (EFAULT);
#endif
		TO_TSTRING(env, path, tpath, ret);
		if (ret != 0)
			return (ret);
		ret = __os_unique_name(tpath, fhp->handle,
		    shmem_name, sizeof(shmem_name));
		FREE_STRING(env, tpath);
		if (ret != 0)
			return (ret);
	}

	/*
	 * XXX
	 * DB: We have not implemented copy-on-write here.
	 *
	 * If this is an region in system memory, we try to open it using the
	 * OpenFileMapping() first, and only call CreateFileMapping() if we're
	 * really creating the section.  There are two reasons:
	 *
	 * 1) We only create the mapping if we have newly created the region.
	 *    This avoids a long-running problem caused by Windows reference
	 *    counting, where regions that are closed by all processes are
	 *    deleted.  It turns out that just checking for a zeroed region
	 *    is not good enough. See [#4882] and [#7127] for the details.
	 *
	 * 2) CreateFileMapping seems to mess up making the commit charge to
	 *    the process. It thinks, incorrectly, that when we want to join a
	 *    previously existing section, that it should make a commit charge
	 *    for the whole section.  In fact, there is no new committed memory
	 *    whatever.  The call can fail if there is insufficient memory free
	 *    to handle the erroneous commit charge.  So, we find that the
	 *    bogus commit is not made if we call OpenFileMapping.
	 */
	hMemory = NULL;
	if (use_pagefile) {
#ifndef DB_WINCE
		hMemory = OpenFileMapping(
		    is_rdonly ? FILE_MAP_READ : FILE_MAP_ALL_ACCESS,
		    0, shmem_name);

		if (hMemory == NULL && F_ISSET(infop, REGION_CREATE_OK))
			hMemory = CreateFileMapping((HANDLE)-1, 0,
			    is_rdonly ? PAGE_READONLY : PAGE_READWRITE,
			    (DWORD)(len64 >> 32), (DWORD)len64, shmem_name);
#endif
	} else {
		hMemory = CreateFileMapping(fhp->handle, 0,
		    is_rdonly ? PAGE_READONLY : PAGE_READWRITE,
		    (DWORD)(len64 >> 32), (DWORD)len64, NULL);
#ifdef DB_WINCE
		/*
		 * WinCE automatically closes the handle passed in.
		 * Ensure DB does not attempt to close the handle again.
		 */
		fhp->handle = INVALID_HANDLE_VALUE;
		F_CLR(fhp, DB_FH_OPENED);
#endif
	}

	if (hMemory == NULL) {
		ret = __os_get_syserr();
		__db_syserr(env, ret, "OpenFileMapping");
		return (__env_panic(env, __os_posix_err(ret)));
	}

	pMemory = MapViewOfFile(hMemory,
	    (is_rdonly ? FILE_MAP_READ : FILE_MAP_ALL_ACCESS), 0, 0, len);
	if (pMemory == NULL) {
		ret = __os_get_syserr();
		__db_syserr(env, ret, "MapViewOfFile");
		return (__env_panic(env, __os_posix_err(ret)));
	}

	/*
	 * XXX
	 * It turns out that the kernel object underlying the named section
	 * is reference counted, but that the call to MapViewOfFile() above
	 * does NOT increment the reference count! So, if we close the handle
	 * here, the kernel deletes the object from the kernel namespace.
	 * When a second process comes along to join the region, the kernel
	 * happily creates a new object with the same name, but completely
	 * different identity. The two processes then have distinct isolated
	 * mapped sections, not at all what was wanted. Not closing the handle
	 * here fixes this problem.  We carry the handle around in the region
	 * structure so we can close it when unmap is called.
	 */
	if (use_pagefile && infop != NULL)
		infop->wnt_handle = hMemory;
	else
		CloseHandle(hMemory);

	*addr = pMemory;
	return (ret);
}
