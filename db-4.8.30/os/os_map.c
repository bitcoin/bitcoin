/*-
 * See the file LICENSE for redistribution information.
 *
 * Copyright (c) 1996-2009 Oracle.  All rights reserved.
 *
 * $Id$
 */

#include "db_config.h"

#include "db_int.h"

#ifdef HAVE_SYSTEM_INCLUDE_FILES
#ifdef HAVE_MMAP
#include <sys/mman.h>
#endif

#ifdef HAVE_SHMGET
#include <sys/ipc.h>
#include <sys/shm.h>
#endif
#endif

#ifdef HAVE_MMAP
static int __os_map __P((ENV *, char *, DB_FH *, size_t, int, int, void **));
#endif
#ifdef HAVE_SHMGET
static int __shm_mode __P((ENV *));
#else
static int __no_system_mem __P((ENV *));
#endif

/*
 * __os_attach --
 *	Create/join a shared memory region.
 *
 * PUBLIC: int __os_attach __P((ENV *, REGINFO *, REGION *));
 */
int
__os_attach(env, infop, rp)
	ENV *env;
	REGINFO *infop;
	REGION *rp;
{
	DB_ENV *dbenv;
	int create_ok, ret;

	/*
	 * We pass a DB_ENV handle to the user's replacement map function,
	 * so there must be a valid handle.
	 */
	DB_ASSERT(env, env != NULL && env->dbenv != NULL);
	dbenv = env->dbenv;

	if (DB_GLOBAL(j_region_map) != NULL) {
		/*
		 * We have to find out if the region is being created.  Ask
		 * the underlying map function, and use the REGINFO structure
		 * to pass that information back to our caller.
		 */
		create_ok = F_ISSET(infop, REGION_CREATE) ? 1 : 0;
		ret = DB_GLOBAL(j_region_map)
		    (dbenv, infop->name, rp->size, &create_ok, &infop->addr);
		if (create_ok)
			F_SET(infop, REGION_CREATE);
		else
			F_CLR(infop, REGION_CREATE);
		return (ret);
	}

	if (F_ISSET(env, ENV_SYSTEM_MEM)) {
		/*
		 * If the region is in system memory on UNIX, we use shmget(2).
		 *
		 * !!!
		 * There exist spinlocks that don't work in shmget memory, e.g.,
		 * the HP/UX msemaphore interface.  If we don't have locks that
		 * will work in shmget memory, we better be private and not be
		 * threaded.  If we reach this point, we know we're public, so
		 * it's an error.
		 */
#if defined(HAVE_MUTEX_HPPA_MSEM_INIT)
		__db_errx(env,
	    "architecture does not support locks inside system shared memory");
		return (EINVAL);
#endif
#if defined(HAVE_SHMGET)
		{
		key_t segid;
		int id, mode;

		/*
		 * We could potentially create based on REGION_CREATE_OK, but
		 * that's dangerous -- we might get crammed in sideways if
		 * some of the expected regions exist but others do not.  Also,
		 * if the requested size differs from an existing region's
		 * actual size, then all sorts of nasty things can happen.
		 * Basing create solely on REGION_CREATE is much safer -- a
		 * recovery will get us straightened out.
		 */
		if (F_ISSET(infop, REGION_CREATE)) {
			/*
			 * The application must give us a base System V IPC key
			 * value.  Adjust that value based on the region's ID,
			 * and correct so the user's original value appears in
			 * the ipcs output.
			 */
			if (dbenv->shm_key == INVALID_REGION_SEGID) {
				__db_errx(env,
			    "no base system shared memory ID specified");
				return (EINVAL);
			}

			/*
			 * !!!
			 * The BDB API takes a "long" as the base segment ID,
			 * then adds an unsigned 32-bit value and stores it
			 * in a key_t.  Wrong, admittedly, but not worth an
			 * API change to fix.
			 */
			segid = (key_t)
			    ((u_long)dbenv->shm_key + (infop->id - 1));

			/*
			 * If map to an existing region, assume the application
			 * crashed and we're restarting.  Delete the old region
			 * and re-try.  If that fails, return an error, the
			 * application will have to select a different segment
			 * ID or clean up some other way.
			 */
			if ((id = shmget(segid, 0, 0)) != -1) {
				(void)shmctl(id, IPC_RMID, NULL);
				if ((id = shmget(segid, 0, 0)) != -1) {
					__db_errx(env,
		"shmget: key: %ld: shared system memory region already exists",
					    (long)segid);
					return (EAGAIN);
				}
			}

			/*
			 * Map the DbEnv::open method file mode permissions to
			 * shmget call permissions.
			 */
			mode = IPC_CREAT | __shm_mode(env);
			if ((id = shmget(segid, rp->size, mode)) == -1) {
				ret = __os_get_syserr();
				__db_syserr(env, ret,
	"shmget: key: %ld: unable to create shared system memory region",
				    (long)segid);
				return (__os_posix_err(ret));
			}
			rp->segid = id;
		} else
			id = rp->segid;

		if ((infop->addr = shmat(id, NULL, 0)) == (void *)-1) {
			infop->addr = NULL;
			ret = __os_get_syserr();
			__db_syserr(env, ret,
	"shmat: id %d: unable to attach to shared system memory region", id);
			return (__os_posix_err(ret));
		}

		/* Optionally lock the memory down. */
		if (F_ISSET(env, ENV_LOCKDOWN)) {
#ifdef HAVE_SHMCTL_SHM_LOCK
			ret = shmctl(
			    id, SHM_LOCK, NULL) == 0 ? 0 : __os_get_syserr();
#else
			ret = DB_OPNOTSUP;
#endif
			if (ret != 0) {
				__db_syserr(env, ret,
	"shmctl/SHM_LOCK: id %d: unable to lock down shared memory region", id);
				return (__os_posix_err(ret));
			}
		}

		return (0);
		}
#else
		return (__no_system_mem(env));
#endif
	}

#ifdef HAVE_MMAP
	{
	DB_FH *fhp;

	fhp = NULL;

	/*
	 * Try to open/create the shared region file.  We DO NOT need to ensure
	 * that multiple threads/processes attempting to simultaneously create
	 * the region are properly ordered, our caller has already taken care
	 * of that.
	 */
	if ((ret = __os_open(env, infop->name, 0,
	    DB_OSO_REGION |
	    (F_ISSET(infop, REGION_CREATE_OK) ? DB_OSO_CREATE : 0),
	    env->db_mode, &fhp)) != 0)
		__db_err(env, ret, "%s", infop->name);

	/*
	 * If we created the file, grow it to its full size before mapping
	 * it in.  We really want to avoid touching the buffer cache after
	 * mmap(2) is called, doing anything else confuses the hell out of
	 * systems without merged VM/buffer cache systems, or, more to the
	 * point, *badly* merged VM/buffer cache systems.
	 */
	if (ret == 0 && F_ISSET(infop, REGION_CREATE)) {
		if (F_ISSET(dbenv, DB_ENV_REGION_INIT))
			ret = __db_file_write(env, fhp,
			    rp->size / MEGABYTE, rp->size % MEGABYTE, 0x00);
		else
			ret = __db_file_extend(env, fhp, rp->size);
	}

	/* Map the file in. */
	if (ret == 0)
		ret = __os_map(env,
		    infop->name, fhp, rp->size, 1, 0, &infop->addr);

	if (fhp != NULL)
		(void)__os_closehandle(env, fhp);

	return (ret);
	}
#else
	COMPQUIET(infop, NULL);
	COMPQUIET(rp, NULL);
	__db_errx(env,
	    "architecture lacks mmap(2), shared environments not possible");
	return (DB_OPNOTSUP);
#endif
}

/*
 * __os_detach --
 *	Detach from a shared memory region.
 *
 * PUBLIC: int __os_detach __P((ENV *, REGINFO *, int));
 */
int
__os_detach(env, infop, destroy)
	ENV *env;
	REGINFO *infop;
	int destroy;
{
	DB_ENV *dbenv;
	REGION *rp;
	int ret;

	/*
	 * We pass a DB_ENV handle to the user's replacement unmap function,
	 * so there must be a valid handle.
	 */
	DB_ASSERT(env, env != NULL && env->dbenv != NULL);
	dbenv = env->dbenv;

	rp = infop->rp;

	/* If the user replaced the unmap call, call through their interface. */
	if (DB_GLOBAL(j_region_unmap) != NULL)
		return (DB_GLOBAL(j_region_unmap)(dbenv, infop->addr));

	if (F_ISSET(env, ENV_SYSTEM_MEM)) {
#ifdef HAVE_SHMGET
		int segid;

		/*
		 * We may be about to remove the memory referenced by rp,
		 * save the segment ID, and (optionally) wipe the original.
		 */
		segid = rp->segid;
		if (destroy)
			rp->segid = INVALID_REGION_SEGID;

		if (shmdt(infop->addr) != 0) {
			ret = __os_get_syserr();
			__db_syserr(env, ret, "shmdt");
			return (__os_posix_err(ret));
		}

		if (destroy && shmctl(segid, IPC_RMID,
		    NULL) != 0 && (ret = __os_get_syserr()) != EINVAL) {
			__db_syserr(env, ret,
	    "shmctl: id %d: unable to delete system shared memory region",
			    segid);
			return (__os_posix_err(ret));
		}

		return (0);
#else
		return (__no_system_mem(env));
#endif
	}

#ifdef HAVE_MMAP
#ifdef HAVE_MUNLOCK
	if (F_ISSET(env, ENV_LOCKDOWN))
		(void)munlock(infop->addr, rp->size);
#endif
	if (munmap(infop->addr, rp->size) != 0) {
		ret = __os_get_syserr();
		__db_syserr(env, ret, "munmap");
		return (__os_posix_err(ret));
	}

	if (destroy && (ret = __os_unlink(env, infop->name, 1)) != 0)
		return (ret);

	return (0);
#else
	COMPQUIET(destroy, 0);
	COMPQUIET(ret, 0);
	return (EINVAL);
#endif
}

/*
 * __os_mapfile --
 *	Map in a shared memory file.
 *
 * PUBLIC: int __os_mapfile __P((ENV *, char *, DB_FH *, size_t, int, void **));
 */
int
__os_mapfile(env, path, fhp, len, is_rdonly, addrp)
	ENV *env;
	char *path;
	DB_FH *fhp;
	int is_rdonly;
	size_t len;
	void **addrp;
{
#if defined(HAVE_MMAP) && !defined(HAVE_QNX)
	DB_ENV *dbenv;

	/* If the user replaced the map call, call through their interface. */
	if (DB_GLOBAL(j_file_map) != NULL) {
		/*
		 * We pass a DB_ENV handle to the user's replacement map
		 * function, so there must be a valid handle.
		 */
		DB_ASSERT(env, env != NULL && env->dbenv != NULL);
		dbenv = env->dbenv;

		return (
		    DB_GLOBAL(j_file_map)(dbenv, path, len, is_rdonly, addrp));
	}

	return (__os_map(env, path, fhp, len, 0, is_rdonly, addrp));
#else
	COMPQUIET(env, NULL);
	COMPQUIET(path, NULL);
	COMPQUIET(fhp, NULL);
	COMPQUIET(is_rdonly, 0);
	COMPQUIET(len, 0);
	COMPQUIET(addrp, NULL);
	return (DB_OPNOTSUP);
#endif
}

/*
 * __os_unmapfile --
 *	Unmap the shared memory file.
 *
 * PUBLIC: int __os_unmapfile __P((ENV *, void *, size_t));
 */
int
__os_unmapfile(env, addr, len)
	ENV *env;
	void *addr;
	size_t len;
{
	DB_ENV *dbenv;
	int ret;

	/*
	 * We pass a DB_ENV handle to the user's replacement unmap function,
	 * so there must be a valid handle.
	 */
	DB_ASSERT(env, env != NULL && env->dbenv != NULL);
	dbenv = env->dbenv;

	if (FLD_ISSET(dbenv->verbose, DB_VERB_FILEOPS | DB_VERB_FILEOPS_ALL))
		__db_msg(env, "fileops: munmap");

	/* If the user replaced the map call, call through their interface. */
	if (DB_GLOBAL(j_file_unmap) != NULL)
		return (DB_GLOBAL(j_file_unmap)(dbenv, addr));

#ifdef HAVE_MMAP
#ifdef HAVE_MUNLOCK
	if (F_ISSET(env, ENV_LOCKDOWN))
		RETRY_CHK((munlock(addr, len)), ret);
		/*
		 * !!!
		 * The return value is ignored.
		 */
#else
	COMPQUIET(env, NULL);
#endif
	RETRY_CHK((munmap(addr, len)), ret);
	ret = __os_posix_err(ret);
#else
	COMPQUIET(env, NULL);
	ret = EINVAL;
#endif
	return (ret);
}

#ifdef HAVE_MMAP
/*
 * __os_map --
 *	Call the mmap(2) function.
 */
static int
__os_map(env, path, fhp, len, is_region, is_rdonly, addrp)
	ENV *env;
	char *path;
	DB_FH *fhp;
	int is_region, is_rdonly;
	size_t len;
	void **addrp;
{
	DB_ENV *dbenv;
	int flags, prot, ret;
	void *p;

	/*
	 * We pass a DB_ENV handle to the user's replacement map function,
	 * so there must be a valid handle.
	 */
	DB_ASSERT(env, env != NULL && env->dbenv != NULL);
	dbenv = env->dbenv;

	if (FLD_ISSET(dbenv->verbose, DB_VERB_FILEOPS | DB_VERB_FILEOPS_ALL))
		__db_msg(env, "fileops: mmap %s", path);

	DB_ASSERT(env, F_ISSET(fhp, DB_FH_OPENED) && fhp->fd != -1);

	/*
	 * If it's read-only, it's private, and if it's not, it's shared.
	 * Don't bother with an additional parameter.
	 */
	flags = is_rdonly ? MAP_PRIVATE : MAP_SHARED;

#ifdef MAP_FILE
	/*
	 * Historically, MAP_FILE was required for mapping regular files,
	 * even though it was the default.  Some systems have it, some
	 * don't, some that have it set it to 0.
	 */
	flags |= MAP_FILE;
#endif

	/*
	 * I know of no systems that implement the flag to tell the system
	 * that the region contains semaphores, but it's not an unreasonable
	 * thing to do, and has been part of the design since forever.  I
	 * don't think anyone will object, but don't set it for read-only
	 * files, it doesn't make sense.
	 */
#ifdef MAP_HASSEMAPHORE
	if (is_region && !is_rdonly)
		flags |= MAP_HASSEMAPHORE;
#else
	COMPQUIET(is_region, 0);
#endif

	/*
	 * FreeBSD:
	 * Causes data dirtied via this VM map to be flushed to physical media
	 * only when necessary (usually by the pager) rather then gratuitously.
	 * Typically this prevents the update daemons from flushing pages
	 * dirtied through such maps and thus allows efficient sharing of
	 * memory across unassociated processes using a file-backed shared
	 * memory map.
	 */
#ifdef MAP_NOSYNC
	flags |= MAP_NOSYNC;
#endif

	prot = PROT_READ | (is_rdonly ? 0 : PROT_WRITE);

	/*
	 * XXX
	 * Work around a bug in the VMS V7.1 mmap() implementation.  To map
	 * a file into memory on VMS it needs to be opened in a certain way,
	 * originally.  To get the file opened in that certain way, the VMS
	 * mmap() closes the file and re-opens it.  When it does this, it
	 * doesn't flush any caches out to disk before closing.  The problem
	 * this causes us is that when the memory cache doesn't get written
	 * out, the file isn't big enough to match the memory chunk and the
	 * mmap() call fails.  This call to fsync() fixes the problem.  DEC
	 * thinks this isn't a bug because of language in XPG5 discussing user
	 * responsibility for on-disk and in-memory synchronization.
	 */
#ifdef VMS
	if (__os_fsync(env, fhp) == -1)
		return (__os_posix_err(__os_get_syserr()));
#endif

	/* MAP_FAILED was not defined in early mmap implementations. */
#ifndef MAP_FAILED
#define	MAP_FAILED	-1
#endif
	if ((p = mmap(NULL,
	    len, prot, flags, fhp->fd, (off_t)0)) == (void *)MAP_FAILED) {
		ret = __os_get_syserr();
		__db_syserr(env, ret, "mmap");
		return (__os_posix_err(ret));
	}

	/*
	 * If it's a region, we want to make sure that the memory isn't paged.
	 * For example, Solaris will page large mpools because it thinks that
	 * I/O buffer memory is more important than we are.  The mlock system
	 * call may or may not succeed (mlock is restricted to the super-user
	 * on some systems).  Currently, the only other use of mmap in DB is
	 * to map read-only databases -- we don't want them paged, either, so
	 * the call isn't conditional.
	 */
	if (F_ISSET(env, ENV_LOCKDOWN)) {
#ifdef HAVE_MLOCK
		ret = mlock(p, len) == 0 ? 0 : __os_get_syserr();
#else
		ret = DB_OPNOTSUP;
#endif
		if (ret != 0) {
			__db_syserr(env, ret, "mlock");
			return (__os_posix_err(ret));
		}
	}

	*addrp = p;
	return (0);
}
#endif

#ifdef HAVE_SHMGET
#ifndef SHM_R
#define	SHM_R	0400
#endif
#ifndef SHM_W
#define	SHM_W	0200
#endif

/*
 * __shm_mode --
 *	Map the DbEnv::open method file mode permissions to shmget call
 *	permissions.
 */
static int
__shm_mode(env)
	ENV *env;
{
	int mode;

	/* Default to r/w owner, r/w group. */
	if (env->db_mode == 0)
		return (SHM_R | SHM_W | SHM_R >> 3 | SHM_W >> 3);

	mode = 0;
	if (env->db_mode & S_IRUSR)
		mode |= SHM_R;
	if (env->db_mode & S_IWUSR)
		mode |= SHM_W;
	if (env->db_mode & S_IRGRP)
		mode |= SHM_R >> 3;
	if (env->db_mode & S_IWGRP)
		mode |= SHM_W >> 3;
	if (env->db_mode & S_IROTH)
		mode |= SHM_R >> 6;
	if (env->db_mode & S_IWOTH)
		mode |= SHM_W >> 6;
	return (mode);
}
#else
/*
 * __no_system_mem --
 *	No system memory environments error message.
 */
static int
__no_system_mem(env)
	ENV *env;
{
	__db_errx(env,
	    "architecture doesn't support environments in system memory");
	return (DB_OPNOTSUP);
}
#endif /* HAVE_SHMGET */
