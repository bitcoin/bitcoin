/*-
 * See the file LICENSE for redistribution information.
 *
 * Copyright (c) 2004-2009 Oracle.  All rights reserved.
 *
 * $Id$
 */

#include "db_config.h"

#include "db_int.h"

#define	REGISTER_FILE	"__db.register"

#define	PID_EMPTY	"X                      0\n"	/* Unused PID entry */
#define	PID_FMT		"%24lu\n"			/* PID entry format */
							/* Unused PID test */
#define	PID_ISEMPTY(p)	(memcmp(p, PID_EMPTY, PID_LEN) == 0)
#define	PID_LEN		(25)				/* PID entry length */

#define	REGISTRY_LOCK(env, pos, nowait)					\
	__os_fdlock(env, (env)->dbenv->registry, (off_t)(pos), 1, nowait)
#define	REGISTRY_UNLOCK(env, pos)					\
	__os_fdlock(env, (env)->dbenv->registry, (off_t)(pos), 0, 0)
#define	REGISTRY_EXCL_LOCK(env, nowait)					\
	REGISTRY_LOCK(env, 1, nowait)
#define	REGISTRY_EXCL_UNLOCK(env)					\
	REGISTRY_UNLOCK(env, 1)

static  int __envreg_add __P((ENV *, int *, u_int32_t));

/*
 * Support for portable, multi-process database environment locking, based on
 * the Subversion SR (#11511).
 *
 * The registry feature is configured by specifying the DB_REGISTER flag to the
 * DbEnv.open method.  If DB_REGISTER is specified, DB opens the registry file
 * in the database environment home directory.  The registry file is formatted
 * as follows:
 *
 *	                    12345		# process ID slot 1
 *	X		# empty slot
 *	                    12346		# process ID slot 2
 *	X		# empty slot
 *	                    12347		# process ID slot 3
 *	                    12348		# process ID slot 4
 *	X                   12349		# empty slot
 *	X		# empty slot
 *
 * All lines are fixed-length.  All lines are process ID slots.  Empty slots
 * are marked with leading non-digit characters.
 *
 * To modify the file, you get an exclusive lock on the first byte of the file.
 *
 * While holding any DbEnv handle, each process has an exclusive lock on the
 * first byte of a process ID slot.  There is a restriction on having more
 * than one DbEnv handle open at a time, because Berkeley DB uses per-process
 * locking to implement this feature, that is, a process may never have more
 * than a single slot locked.
 *
 * This work requires that if a process dies or the system crashes, locks held
 * by the dying processes will be dropped.  (We can't use system shared
 * memory-backed or filesystem-backed locks because they're persistent when a
 * process dies.)  On POSIX systems, we use fcntl(2) locks; on Win32 we have
 * LockFileEx/UnlockFile, except for Win/9X and Win/ME which have to loop on
 * Lockfile/UnlockFile.
 *
 * We could implement the same solution with flock locking instead of fcntl,
 * but flock would require a separate file for each process of control (and
 * probably each DbEnv handle) in the database environment, which is fairly
 * ugly.
 *
 * Whenever a process opens a new DbEnv handle, it walks the registry file and
 * verifies it CANNOT acquire the lock for any non-empty slot.  If a lock for
 * a non-empty slot is available, we know a process died holding an open handle,
 * and recovery needs to be run.
 *
 * It's possible to get corruption in the registry file.  If a write system
 * call fails after partially completing, there can be corrupted entries in
 * the registry file, or a partial entry at the end of the file.  This is OK.
 * A corrupted entry will be flagged as a non-empty line during the registry
 * file walk.  Since the line was corrupted by process failure, no process will
 * hold a lock on the slot, which will lead to recovery being run.
 *
 * There can still be processes running in the environment when we recover it,
 * and, in fact, there can still be processes running in the old environment
 * after we're up and running in a new one.  This is safe because performing
 * recovery panics (and removes) the existing environment, so the window of
 * vulnerability is small.  Further, we check the panic flag in the DB API
 * methods, when waking from spinning on a mutex, and whenever we're about to
 * write to disk).  The only window of corruption is if the write check of the
 * panic were to complete, the region subsequently be recovered, and then the
 * write continues.  That's very, very unlikely to happen.  This vulnerability
 * already exists in Berkeley DB, too, the registry code doesn't make it any
 * worse than it already is.
 *
 * The only way to avoid that window entirely is to ensure that all processes
 * in the Berkeley DB environment exit before we run recovery.   Applications
 * can do that if they maintain their own process registry outside of Berkeley
 * DB, but it's a little more difficult to do here.   The obvious approach is
 * to send signals to any process using the database environment as soon as we
 * decide to run recovery, but there are problems with that approach: we might
 * not have permission to send signals to the process, the process might have
 * signal handlers installed, the cookie stored might not be the same as kill's
 * argument, we may not be able to reliably tell if the process died, and there
 * are probably other problems.  However, if we can send a signal, it reduces
 * the window, and so we include the code here.  To configure it, turn on the
 * DB_ENVREG_KILL_ALL #define.
 */
#define	DB_ENVREG_KILL_ALL	0

/*
 * __envreg_register --
 *	Register a ENV handle.
 *
 * PUBLIC: int __envreg_register __P((ENV *, int *, u_int32_t));
 */
int
__envreg_register(env, need_recoveryp, flags)
	ENV *env;
	int *need_recoveryp;
	u_int32_t flags;
{
	DB_ENV *dbenv;
	pid_t pid;
	u_int32_t bytes, mbytes;
	int ret;
	char *pp;

	*need_recoveryp = 0;

	dbenv = env->dbenv;
	dbenv->thread_id(dbenv, &pid, NULL);
	pp = NULL;

	if (FLD_ISSET(dbenv->verbose, DB_VERB_REGISTER))
		__db_msg(env, "%lu: register environment", (u_long)pid);

	/* Build the path name and open the registry file. */
	if ((ret = __db_appname(env,
	    DB_APP_NONE, REGISTER_FILE, NULL, &pp)) != 0)
		goto err;
	if ((ret = __os_open(env, pp, 0,
	    DB_OSO_CREATE, DB_MODE_660, &dbenv->registry)) != 0)
		goto err;

	/*
	 * Wait for an exclusive lock on the file.
	 *
	 * !!!
	 * We're locking bytes that don't yet exist, but that's OK as far as
	 * I know.
	 */
	if ((ret = REGISTRY_EXCL_LOCK(env, 0)) != 0)
		goto err;

	/*
	 * If the file size is 0, initialize the file.
	 *
	 * Run recovery if we create the file, that means we can clean up the
	 * system by removing the registry file and restarting the application.
	 */
	if ((ret = __os_ioinfo(
	    env, pp, dbenv->registry, &mbytes, &bytes, NULL)) != 0)
		goto err;
	if (mbytes == 0 && bytes == 0) {
		if (FLD_ISSET(dbenv->verbose, DB_VERB_REGISTER))
			__db_msg(env, "%lu: creating %s", (u_long)pid, pp);
		*need_recoveryp = 1;
	}

	/* Register this process. */
	if ((ret = __envreg_add(env, need_recoveryp, flags) != 0))
		goto err;

	/*
	 * Release our exclusive lock if we don't need to run recovery.  If
	 * we need to run recovery, ENV->open will call back into register
	 * code once recovery has completed.
	 */
	if (*need_recoveryp == 0 && (ret = REGISTRY_EXCL_UNLOCK(env)) != 0)
		goto err;

	if (0) {
err:		*need_recoveryp = 0;

		/*
		 * !!!
		 * Closing the file handle must release all of our locks.
		 */
		if (dbenv->registry != NULL)
			(void)__os_closehandle(env, dbenv->registry);
		dbenv->registry = NULL;
	}

	if (pp != NULL)
		__os_free(env, pp);

	return (ret);
}

/*
 * __envreg_add --
 *	Add the process' pid to the register.
 */
static int
__envreg_add(env, need_recoveryp, flags)
	ENV *env;
	int *need_recoveryp;
	u_int32_t flags;
{
	DB_ENV *dbenv;
	DB_THREAD_INFO *ip;
	REGENV * renv;
	REGINFO *infop;
	pid_t pid;
	off_t end, pos, dead;
	size_t nr, nw;
	u_int lcnt;
	u_int32_t bytes, mbytes, orig_flags;
	int need_recovery, ret, t_ret;
	char *p, buf[PID_LEN + 10], pid_buf[PID_LEN + 10];

	dbenv = env->dbenv;
	need_recovery = 0;
	COMPQUIET(dead, 0);
	COMPQUIET(p, NULL);
	ip = NULL;

	/* Get a copy of our process ID. */
	dbenv->thread_id(dbenv, &pid, NULL);
	snprintf(pid_buf, sizeof(pid_buf), PID_FMT, (u_long)pid);

	if (FLD_ISSET(dbenv->verbose, DB_VERB_REGISTER))
		__db_msg(env, "%lu: adding self to registry", (u_long)pid);

#if DB_ENVREG_KILL_ALL
	if (0) {
kill_all:	/*
		 * A second pass through the file, this time killing any
		 * processes still running.
		 */
		if ((ret = __os_seek(env, dbenv->registry, 0, 0, 0)) != 0)
			return (ret);
	}
#endif

	/*
	 * Read the file.  Skip empty slots, and check that a lock is held
	 * for any allocated slots.  An allocated slot which we can lock
	 * indicates a process died holding a handle and recovery needs to
	 * be run.
	 */
	for (lcnt = 0;; ++lcnt) {
		if ((ret = __os_read(
		    env, dbenv->registry, buf, PID_LEN, &nr)) != 0)
			return (ret);
		if (nr == 0)
			break;

		/*
		 * A partial record at the end of the file is possible if a
		 * previously un-registered process was interrupted while
		 * registering.
		 */
		if (nr != PID_LEN) {
			need_recovery = 1;
			break;
		}

		if (PID_ISEMPTY(buf)) {
			if (FLD_ISSET(dbenv->verbose, DB_VERB_REGISTER))
				__db_msg(env, "%02u: EMPTY", lcnt);
			continue;
		}

		/*
		 * !!!
		 * DB_REGISTER is implemented using per-process locking, only
		 * a single ENV handle may be open per process.  Enforce
		 * that restriction.
		 */
		if (memcmp(buf, pid_buf, PID_LEN) == 0) {
			__db_errx(env,
    "DB_REGISTER limits processes to one open DB_ENV handle per environment");
			return (EINVAL);
		}

		if (FLD_ISSET(dbenv->verbose, DB_VERB_REGISTER)) {
			for (p = buf; *p == ' ';)
				++p;
			buf[nr - 1] = '\0';
		}

#if DB_ENVREG_KILL_ALL
		if (need_recovery) {
			pid = (pid_t)strtoul(buf, NULL, 10);
			(void)kill(pid, SIGKILL);

			if (FLD_ISSET(dbenv->verbose, DB_VERB_REGISTER))
				__db_msg(env, "%02u: %s: KILLED", lcnt, p);
			continue;
		}
#endif
		pos = (off_t)lcnt * PID_LEN;
		if (REGISTRY_LOCK(env, pos, 1) == 0) {
			if ((ret = REGISTRY_UNLOCK(env, pos)) != 0)
				return (ret);

			if (FLD_ISSET(dbenv->verbose, DB_VERB_REGISTER))
				__db_msg(env, "%02u: %s: FAILED", lcnt, p);

			need_recovery = 1;
			dead = pos;
#if DB_ENVREG_KILL_ALL
			goto kill_all;
#else
			break;
#endif
		} else
			if (FLD_ISSET(dbenv->verbose, DB_VERB_REGISTER))
				__db_msg(env, "%02u: %s: LOCKED", lcnt, p);
	}

	/*
	 * If we have to perform recovery...
	 *
	 * Mark all slots empty.  Registry ignores empty slots we can't lock,
	 * so it doesn't matter if any of the processes are in the middle of
	 * exiting Berkeley DB -- they'll discard their lock when they exit.
	 */
	if (need_recovery) {
		if (FLD_ISSET(dbenv->verbose, DB_VERB_REGISTER))
			__db_msg(env, "%lu: recovery required", (u_long)pid);

		if (LF_ISSET(DB_FAILCHK)) {
			if (FLD_ISSET(dbenv->verbose, DB_VERB_REGISTER))
				__db_msg(env,
				    "%lu: performing failchk", (u_long)pid);
			/* The environment will already exist, so we do not
			 * want DB_CREATE set, nor do we want any recovery at
			 * this point.  No need to put values back as flags is
			 * passed in by value.  Save original dbenv flags in
			 * case we need to recover/remove existing environment.
			 * Set DB_ENV_FAILCHK before attach to help ensure we
			 * dont block on a mutex held by the dead process.
			 */
			LF_CLR(DB_CREATE | DB_RECOVER | DB_RECOVER_FATAL);
			orig_flags = dbenv->flags;
			F_SET(dbenv, DB_ENV_FAILCHK);
			/* Attach to environment and subsystems. */
			if ((ret = __env_attach_regions(
			    dbenv, flags, orig_flags, 0)) != 0)
				goto sig_proc;
			if ((t_ret =
			    __env_set_state(env, &ip, THREAD_FAILCHK)) != 0 &&
			    ret == 0)
				ret = t_ret;
			if ((t_ret =
			    __env_failchk_int(dbenv)) != 0 && ret == 0)
				ret = t_ret;
			/* Detach from environment and deregister thread. */
			if ((t_ret =
			    __env_refresh(dbenv, orig_flags, 0)) != 0 &&
			    ret == 0)
				ret = t_ret;
			if (ret == 0) {
				if ((ret = __os_seek(env, dbenv->registry,
				    0, 0,(u_int32_t)dead)) != 0 ||
				    (ret = __os_write(env, dbenv->registry,
				    PID_EMPTY, PID_LEN, &nw)) != 0)
					return (ret);
				need_recovery = 0;
				goto add;
			}

		}
		/* If we can't attach, then we cannot set DB_REGISTER panic. */
sig_proc:	if (__env_attach(env, NULL, 0, 0) == 0) {
			infop = env->reginfo;
			renv = infop->primary;
			/* Indicate DB_REGSITER panic.  Also, set environment
			 * panic as this is the panic trigger mechanism in
			 * the code that everything looks for.
			 */
			renv->reg_panic = 1;
			renv->panic = 1;
			(void)__env_detach(env, 0);
		}

		/* Wait for processes to see the panic and leave. */
		__os_yield(env, 0, dbenv->envreg_timeout);

		/* FIGURE out how big the file is. */
		if ((ret = __os_ioinfo(
		    env, NULL, dbenv->registry, &mbytes, &bytes, NULL)) != 0)
			return (ret);
		end = (off_t)mbytes * MEGABYTE + bytes;

		/*
		 * Seek to the beginning of the file and overwrite slots to
		 * the end of the file.
		 *
		 * It's possible for there to be a partial entry at the end of
		 * the file if a process died when trying to register.  If so,
		 * correct for it and overwrite it as well.
		 */
		if ((ret = __os_seek(env, dbenv->registry, 0, 0, 0)) != 0)
			return (ret);
		for (lcnt = 0; lcnt < ((u_int)end / PID_LEN +
		    ((u_int)end % PID_LEN == 0 ? 0 : 1)); ++lcnt) {

			if ((ret = __os_read(
			    env, dbenv->registry, buf, PID_LEN, &nr)) != 0)
				return (ret);

			pos = (off_t)lcnt * PID_LEN;
			/* do not notify on dead process */
			if (pos != dead) {
				pid = (pid_t)strtoul(buf, NULL, 10);
				DB_EVENT(env, DB_EVENT_REG_ALIVE, &pid);
			}

			if ((ret = __os_seek(env,
			    dbenv->registry, 0, 0, (u_int32_t)pos)) != 0 ||
			    (ret = __os_write(env,
			    dbenv->registry, PID_EMPTY, PID_LEN, &nw)) != 0)
				return (ret);
		}
		/* wait one last time to get everyone out */
		__os_yield(env, 0, dbenv->envreg_timeout);
	}

	/*
	 * Seek to the first process slot and add ourselves to the first empty
	 * slot we can lock.
	 */
add:	if ((ret = __os_seek(env, dbenv->registry, 0, 0, 0)) != 0)
		return (ret);
	for (lcnt = 0;; ++lcnt) {
		if ((ret = __os_read(
		    env, dbenv->registry, buf, PID_LEN, &nr)) != 0)
			return (ret);
		if (nr == PID_LEN && !PID_ISEMPTY(buf))
			continue;
		pos = (off_t)lcnt * PID_LEN;
		if (REGISTRY_LOCK(env, pos, 1) == 0) {
			if (FLD_ISSET(dbenv->verbose, DB_VERB_REGISTER))
				__db_msg(env,
				    "%lu: locking slot %02u at offset %lu",
				    (u_long)pid, lcnt, (u_long)pos);

			if ((ret = __os_seek(env,
			    dbenv->registry, 0, 0, (u_int32_t)pos)) != 0 ||
			    (ret = __os_write(env,
			    dbenv->registry, pid_buf, PID_LEN, &nw)) != 0)
				return (ret);
			dbenv->registry_off = (u_int32_t)pos;
			break;
		}
	}

	if (need_recovery)
		*need_recoveryp = 1;

	return (ret);
}

/*
 * __envreg_unregister --
 *	Unregister a ENV handle.
 *
 * PUBLIC: int __envreg_unregister __P((ENV *, int));
 */
int
__envreg_unregister(env, recovery_failed)
	ENV *env;
	int recovery_failed;
{
	DB_ENV *dbenv;
	size_t nw;
	int ret, t_ret;

	dbenv = env->dbenv;
	ret = 0;

	/*
	 * If recovery failed, we want to drop our locks and return, but still
	 * make sure any subsequent process doesn't decide everything is just
	 * fine and try to get into the database environment.  In the case of
	 * an error, discard our locks, but leave our slot filled-in.
	 */
	if (recovery_failed)
		goto err;

	/*
	 * Why isn't an exclusive lock necessary to discard a ENV handle?
	 *
	 * We mark our process ID slot empty before we discard the process slot
	 * lock, and threads of control reviewing the register file ignore any
	 * slots which they can't lock.
	 */
	if ((ret = __os_seek(env,
	    dbenv->registry, 0, 0, dbenv->registry_off)) != 0 ||
	    (ret = __os_write(
	    env, dbenv->registry, PID_EMPTY, PID_LEN, &nw)) != 0)
		goto err;

	/*
	 * !!!
	 * This code assumes that closing the file descriptor discards all
	 * held locks.
	 *
	 * !!!
	 * There is an ordering problem here -- in the case of a process that
	 * failed in recovery, we're unlocking both the exclusive lock and our
	 * slot lock.  If the OS unlocked the exclusive lock and then allowed
	 * another thread of control to acquire the exclusive lock before also
	 * also releasing our slot lock, we could race.  That can't happen, I
	 * don't think.
	 */
err:	if ((t_ret =
	    __os_closehandle(env, dbenv->registry)) != 0 && ret == 0)
		ret = t_ret;

	dbenv->registry = NULL;
	return (ret);
}

/*
 * __envreg_xunlock --
 *	Discard the exclusive lock held by the ENV handle.
 *
 * PUBLIC: int __envreg_xunlock __P((ENV *));
 */
int
__envreg_xunlock(env)
	ENV *env;
{
	DB_ENV *dbenv;
	pid_t pid;
	int ret;

	dbenv = env->dbenv;

	dbenv->thread_id(dbenv, &pid, NULL);

	if (FLD_ISSET(dbenv->verbose, DB_VERB_REGISTER))
		__db_msg(env,
		    "%lu: recovery completed, unlocking", (u_long)pid);

	if ((ret = REGISTRY_EXCL_UNLOCK(env)) == 0)
		return (ret);

	__db_err(env, ret, "%s: exclusive file unlock", REGISTER_FILE);
	return (__env_panic(env, ret));
}
