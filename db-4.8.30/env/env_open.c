/*-
 * See the file LICENSE for redistribution information.
 *
 * Copyright (c) 1996-2009 Oracle.  All rights reserved.
 *
 * $Id$
 */

#include "db_config.h"

#include "db_int.h"
#include "dbinc/crypto.h"
#include "dbinc/db_page.h"
#include "dbinc/btree.h"
#include "dbinc/lock.h"
#include "dbinc/log.h"
#include "dbinc/mp.h"
#include "dbinc/txn.h"

static int __file_handle_cleanup __P((ENV *));

/*
 * db_version --
 *	Return version information.
 *
 * EXTERN: char *db_version __P((int *, int *, int *));
 */
char *
db_version(majverp, minverp, patchp)
	int *majverp, *minverp, *patchp;
{
	if (majverp != NULL)
		*majverp = DB_VERSION_MAJOR;
	if (minverp != NULL)
		*minverp = DB_VERSION_MINOR;
	if (patchp != NULL)
		*patchp = DB_VERSION_PATCH;
	return ((char *)DB_VERSION_STRING);
}

/*
 * __env_open_pp --
 *	DB_ENV->open pre/post processing.
 *
 * PUBLIC: int __env_open_pp __P((DB_ENV *, const char *, u_int32_t, int));
 */
int
__env_open_pp(dbenv, db_home, flags, mode)
	DB_ENV *dbenv;
	const char *db_home;
	u_int32_t flags;
	int mode;
{
	ENV *env;
	int ret;

	env = dbenv->env;

#undef	OKFLAGS
#define	OKFLAGS								\
	(DB_CREATE | DB_INIT_CDB | DB_INIT_LOCK | DB_INIT_LOG |		\
	DB_INIT_MPOOL | DB_INIT_REP | DB_INIT_TXN | DB_LOCKDOWN |	\
	DB_PRIVATE | DB_RECOVER | DB_RECOVER_FATAL | DB_REGISTER |	\
	DB_SYSTEM_MEM | DB_THREAD | DB_USE_ENVIRON | DB_FAILCHK |	\
	DB_USE_ENVIRON_ROOT)
#undef	OKFLAGS_CDB
#define	OKFLAGS_CDB							\
	(DB_CREATE | DB_INIT_CDB | DB_INIT_MPOOL | DB_LOCKDOWN |	\
	DB_PRIVATE | DB_SYSTEM_MEM | DB_THREAD |			\
	DB_USE_ENVIRON | DB_USE_ENVIRON_ROOT)

	if ((ret = __db_fchk(env, "DB_ENV->open", flags, OKFLAGS)) != 0)
		return (ret);
	if ((ret = __db_fcchk(
	    env, "DB_ENV->open", flags, DB_INIT_CDB, ~OKFLAGS_CDB)) != 0)
		return (ret);
	if (LF_ISSET(DB_REGISTER)) {
		if (!__os_support_db_register()) {
			__db_errx(env,
	    "Berkeley DB library does not support DB_REGISTER on this system");
			return (EINVAL);
		}
		if ((ret = __db_fcchk(env, "DB_ENV->open", flags,
		    DB_PRIVATE, DB_REGISTER | DB_SYSTEM_MEM)) != 0)
			return (ret);
		if (!LF_ISSET(DB_INIT_TXN)) {
			__db_errx(
			    env, "registration requires transaction support");
			return (EINVAL);
		}
	}
	if (LF_ISSET(DB_INIT_REP)) {
		if (!__os_support_replication()) {
			__db_errx(env,
	    "Berkeley DB library does not support replication on this system");
			return (EINVAL);
		}
		if (!LF_ISSET(DB_INIT_LOCK)) {
			__db_errx(env,
			    "replication requires locking support");
			return (EINVAL);
		}
		if (!LF_ISSET(DB_INIT_TXN)) {
			__db_errx(
			    env, "replication requires transaction support");
			return (EINVAL);
		}
	}
	if (LF_ISSET(DB_RECOVER | DB_RECOVER_FATAL)) {
		if ((ret = __db_fcchk(env,
		    "DB_ENV->open", flags, DB_RECOVER, DB_RECOVER_FATAL)) != 0)
			return (ret);
		if ((ret = __db_fcchk(env,
		    "DB_ENV->open", flags, DB_REGISTER, DB_RECOVER_FATAL)) != 0)
			return (ret);
		if (!LF_ISSET(DB_CREATE)) {
			__db_errx(env, "recovery requires the create flag");
			return (EINVAL);
		}
		if (!LF_ISSET(DB_INIT_TXN)) {
			__db_errx(
			    env, "recovery requires transaction support");
			return (EINVAL);
		}
	}
	if (LF_ISSET(DB_FAILCHK)) {
		if (!ALIVE_ON(env)) {
			__db_errx(env,
			 "DB_FAILCHK requires DB_ENV->is_alive be configured");
			return (EINVAL);
		}
		if (dbenv->thr_max == 0) {
			__db_errx(env,
		 "DB_FAILCHK requires DB_ENV->set_thread_count be configured");
			return (EINVAL);
		}
	}

#ifdef HAVE_MUTEX_THREAD_ONLY
	/*
	 * Currently we support one kind of mutex that is intra-process only,
	 * POSIX 1003.1 pthreads, because a variety of systems don't support
	 * the full pthreads API, and our only alternative is test-and-set.
	 */
	if (!LF_ISSET(DB_PRIVATE)) {
		__db_errx(env,
	 "Berkeley DB library configured to support only private environments");
		return (EINVAL);
	}
#endif

#ifdef HAVE_MUTEX_FCNTL
	/*
	 * !!!
	 * We need a file descriptor for fcntl(2) locking.  We use the file
	 * handle from the REGENV file for this purpose.
	 *
	 * Since we may be using shared memory regions, e.g., shmget(2), and
	 * not a mapped-in regular file, the backing file may be only a few
	 * bytes in length.  So, this depends on the ability to call fcntl to
	 * lock file offsets much larger than the actual physical file.  I
	 * think that's safe -- besides, very few systems actually need this
	 * kind of support, SunOS is the only one still in wide use of which
	 * I'm aware.
	 *
	 * The error case is if an application lacks spinlocks and wants to be
	 * threaded.  That doesn't work because fcntl will lock the underlying
	 * process, including all its threads.
	 */
	if (F_ISSET(env, ENV_THREAD)) {
		__db_errx(env,
	    "architecture lacks fast mutexes: applications cannot be threaded");
		return (EINVAL);
	}
#endif

	return (__env_open(dbenv, db_home, flags, mode));
}

/*
 * __env_open --
 *	DB_ENV->open.
 *
 * PUBLIC: int __env_open __P((DB_ENV *, const char *, u_int32_t, int));
 */
int
__env_open(dbenv, db_home, flags, mode)
	DB_ENV *dbenv;
	const char *db_home;
	u_int32_t flags;
	int mode;
{
	DB_THREAD_INFO *ip;
	ENV *env;
	u_int32_t orig_flags;
	int register_recovery, ret, t_ret;

	ip = NULL;
	env = dbenv->env;
	register_recovery = 0;

	/* Initial configuration. */
	if ((ret = __env_config(dbenv, db_home, flags, mode)) != 0)
		return (ret);

	/*
	 * Save the DB_ENV handle's configuration flags as set by user-called
	 * configuration methods and the environment directory's DB_CONFIG
	 * file.  If we use this DB_ENV structure to recover the existing
	 * environment or to remove an environment we created after failure,
	 * we'll restore the DB_ENV flags to these values.
	 */
	orig_flags = dbenv->flags;

	/*
	 * If we're going to register with the environment, that's the first
	 * thing we do.
	 */
	if (LF_ISSET(DB_REGISTER)) {
		if ((ret =
		    __envreg_register(env, &register_recovery, flags)) != 0)
			goto err;
		if (register_recovery) {
			if (!LF_ISSET(DB_RECOVER)) {
				__db_errx(env,
	    "The DB_RECOVER flag was not specified, and recovery is needed");
				ret = DB_RUNRECOVERY;
				goto err;
			}
		} else
			LF_CLR(DB_RECOVER);
	}

	/*
	 * If we're doing recovery, destroy the environment so that we create
	 * all the regions from scratch.  The major concern I have is if the
	 * application stomps the environment with a rogue pointer.  We have
	 * no way of detecting that, and we could be forced into a situation
	 * where we start up and then crash, repeatedly.
	 *
	 * We do not check any flags like DB_PRIVATE before calling remove.
	 * We don't care if the current environment was private or not, we
	 * want to remove files left over for any reason, from any session.
	 */
	if (LF_ISSET(DB_RECOVER | DB_RECOVER_FATAL))
#ifdef HAVE_REPLICATION
		if ((ret = __rep_reset_init(env)) != 0 ||
		    (ret = __env_remove_env(env)) != 0 ||
#else
		if ((ret = __env_remove_env(env)) != 0 ||
#endif
		    (ret = __env_refresh(dbenv, orig_flags, 0)) != 0)
			goto err;

	if ((ret = __env_attach_regions(dbenv, flags, orig_flags, 1)) != 0)
		goto err;

	/* after attached to env, run failchk if not doing register recovery */
	if (LF_ISSET(DB_FAILCHK) && !register_recovery) {
		ENV_ENTER(env, ip);
		if ((ret = __env_failchk_int(dbenv)) != 0)
			goto err;
		ENV_LEAVE(env, ip);
	}

err:	if (ret != 0)
		(void)__env_refresh(dbenv, orig_flags, 0);

	if (register_recovery) {
		/*
		 * If recovery succeeded, release our exclusive lock, other
		 * processes can now proceed.
		 *
		 * If recovery failed, unregister now and let another process
		 * clean up.
		 */
		if (ret == 0 && (t_ret = __envreg_xunlock(env)) != 0)
			ret = t_ret;
		if (ret != 0)
			(void)__envreg_unregister(env, 1);
	}

	return (ret);
}

/*
 * __env_remove --
 *	DB_ENV->remove.
 *
 * PUBLIC: int __env_remove __P((DB_ENV *, const char *, u_int32_t));
 */
int
__env_remove(dbenv, db_home, flags)
	DB_ENV *dbenv;
	const char *db_home;
	u_int32_t flags;
{
	ENV *env;
	int ret, t_ret;

	env = dbenv->env;

#undef	OKFLAGS
#define	OKFLAGS								\
	(DB_FORCE | DB_USE_ENVIRON | DB_USE_ENVIRON_ROOT)

	/* Validate arguments. */
	if ((ret = __db_fchk(env, "DB_ENV->remove", flags, OKFLAGS)) != 0)
		return (ret);

	ENV_ILLEGAL_AFTER_OPEN(env, "DB_ENV->remove");

	if ((ret = __env_config(dbenv, db_home, flags, 0)) != 0)
		return (ret);

	/*
	 * Turn the environment off -- if the environment is corrupted, this
	 * could fail.  Ignore any error if we're forcing the question.
	 */
	if ((ret = __env_turn_off(env, flags)) == 0 || LF_ISSET(DB_FORCE))
		ret = __env_remove_env(env);

	if ((t_ret = __env_close(dbenv, 0)) != 0 && ret == 0)
		ret = t_ret;

	return (ret);
}

/*
 * __env_config --
 *	Argument-based initialization.
 *
 * PUBLIC: int __env_config __P((DB_ENV *, const char *, u_int32_t, int));
 */
int
__env_config(dbenv, db_home, flags, mode)
	DB_ENV *dbenv;
	const char *db_home;
	u_int32_t flags;
	int mode;
{
	ENV *env;
	int ret;
	char *home, home_buf[DB_MAXPATHLEN];

	env = dbenv->env;

	/*
	 * Set the database home.
	 *
	 * Use db_home by default, this allows utilities to reasonably
	 * override the environment either explicitly or by using a -h
	 * option.  Otherwise, use the environment if it's permitted
	 * and initialized.
	 */
	home = (char *)db_home;
	if (home == NULL && (LF_ISSET(DB_USE_ENVIRON) ||
	    (LF_ISSET(DB_USE_ENVIRON_ROOT) && __os_isroot()))) {
		home = home_buf;
		if ((ret = __os_getenv(
		    env, "DB_HOME", &home, sizeof(home_buf))) != 0)
			return (ret);
		/*
		 * home set to NULL if __os_getenv failed to find DB_HOME.
		 */
	}
	if (home != NULL && (ret = __os_strdup(env, home, &env->db_home)) != 0)
		return (ret);

	/* Save a copy of the DB_ENV->open method flags. */
	env->open_flags = flags;

	/* Default permissions are read-write for both owner and group. */
	env->db_mode = mode == 0 ? DB_MODE_660 : mode;

	/* Read the DB_CONFIG file. */
	if ((ret = __env_read_db_config(env)) != 0)
		return (ret);

	/*
	 * If no temporary directory path was specified in the config file,
	 * choose one.
	 */
	if (dbenv->db_tmp_dir == NULL && (ret = __os_tmpdir(env, flags)) != 0)
		return (ret);

	return (0);
}

/*
 * __env_close_pp --
 *	DB_ENV->close pre/post processor.
 *
 * PUBLIC: int __env_close_pp __P((DB_ENV *, u_int32_t));
 */
int
__env_close_pp(dbenv, flags)
	DB_ENV *dbenv;
	u_int32_t flags;
{
	DB_THREAD_INFO *ip;
	ENV *env;
	int rep_check, ret, t_ret;
	u_int32_t flags_orig;

	env = dbenv->env;
	ret = flags_orig = 0;

	/*
	 * Validate arguments, but as a DB_ENV handle destructor, we can't
	 * fail.
	 */
	if (flags != 0 &&
	    (t_ret = __db_ferr(env, "DB_ENV->close", 0)) != 0 && ret == 0)
		ret = t_ret;

	/*
	 * If the environment has panic'd, all we do is try and discard
	 * the important resources.
	 */
	if (PANIC_ISSET(env)) {
		/* clean up from registry file */
		if (dbenv->registry != NULL) {
			/* 
			 * Temporarily set no panic so we do not trigger the 
			 * LAST_PANIC_CHECK_BEFORE_IO check in __os_physwrite
			 * thus allowing the unregister to happen correctly.
			 */
			flags_orig = F_ISSET(dbenv, DB_ENV_NOPANIC);
			F_SET(dbenv, DB_ENV_NOPANIC);
			(void)__envreg_unregister(env, 0);
			dbenv->registry = NULL;
			if (!flags_orig)
				F_CLR(dbenv, DB_ENV_NOPANIC);
		}

		/* Close all underlying file handles. */
		(void)__file_handle_cleanup(env);

		/* Close all underlying threads and sockets. */
		if (IS_ENV_REPLICATED(env))
			(void)__repmgr_close(env);

		PANIC_CHECK(env);
	}

	ENV_ENTER(env, ip);

	rep_check = IS_ENV_REPLICATED(env) ? 1 : 0;
	if (rep_check) {
#ifdef HAVE_REPLICATION_THREADS
		/*
		 * Shut down Replication Manager threads first of all.  This
		 * must be done before __env_rep_enter to avoid a deadlock that
		 * could occur if repmgr's background threads try to do a rep
		 * operation that needs __rep_lockout.
		 */
		if ((t_ret = __repmgr_close(env)) != 0 && ret == 0)
			ret = t_ret;
#endif
		if ((t_ret = __env_rep_enter(env, 0)) != 0 && ret == 0)
			ret = t_ret;
	}

	if ((t_ret = __env_close(dbenv, rep_check)) != 0 && ret == 0)
		ret = t_ret;

	/* Don't ENV_LEAVE as we have already detached from the region. */
	return (ret);
}

/*
 * __env_close --
 *	DB_ENV->close.
 *
 * PUBLIC: int __env_close __P((DB_ENV *, int));
 */
int
__env_close(dbenv, rep_check)
	DB_ENV *dbenv;
	int rep_check;
{
	ENV *env;
	int ret, t_ret;
	char **p;

	env = dbenv->env;
	ret = 0;

	/*
	 * Check to see if we were in the middle of restoring transactions and
	 * need to close the open files.
	 */
	if (TXN_ON(env) && (t_ret = __txn_preclose(env)) != 0 && ret == 0)
		ret = t_ret;

#ifdef HAVE_REPLICATION
	if ((t_ret = __rep_env_close(env)) != 0 && ret == 0)
		ret = t_ret;
#endif

	/*
	 * Detach from the regions and undo the allocations done by
	 * DB_ENV->open.
	 */
	if ((t_ret = __env_refresh(dbenv, 0, rep_check)) != 0 && ret == 0)
		ret = t_ret;

#ifdef HAVE_CRYPTO
	/*
	 * Crypto comes last, because higher level close functions need
	 * cryptography.
	 */
	if ((t_ret = __crypto_env_close(env)) != 0 && ret == 0)
		ret = t_ret;
#endif

	/* If we're registered, clean up. */
	if (dbenv->registry != NULL) {
		(void)__envreg_unregister(env, 0);
		dbenv->registry = NULL;
	}

	/* Check we've closed all underlying file handles. */
	if ((t_ret = __file_handle_cleanup(env)) != 0 && ret == 0)
		ret = t_ret;

	/* Release any string-based configuration parameters we've copied. */
	if (dbenv->db_log_dir != NULL)
		__os_free(env, dbenv->db_log_dir);
	dbenv->db_log_dir = NULL;
	if (dbenv->db_tmp_dir != NULL)
		__os_free(env, dbenv->db_tmp_dir);
	dbenv->db_tmp_dir = NULL;
	if (dbenv->db_data_dir != NULL) {
		for (p = dbenv->db_data_dir; *p != NULL; ++p)
			__os_free(env, *p);
		__os_free(env, dbenv->db_data_dir);
		dbenv->db_data_dir = NULL;
		dbenv->data_next = 0;
	}
	if (dbenv->intermediate_dir_mode != NULL)
		__os_free(env, dbenv->intermediate_dir_mode);
	if (env->db_home != NULL) {
		__os_free(env, env->db_home);
		env->db_home = NULL;
	}

	/* Discard the structure. */
	__db_env_destroy(dbenv);

	return (ret);
}

/*
 * __env_refresh --
 *	Refresh the DB_ENV structure.
 * PUBLIC: int __env_refresh __P((DB_ENV *, u_int32_t, int));
 */
int
__env_refresh(dbenv, orig_flags, rep_check)
	DB_ENV *dbenv;
	u_int32_t orig_flags;
	int rep_check;
{
	DB *ldbp;
	DB_THREAD_INFO *ip;
	ENV *env;
	int ret, t_ret;

	env = dbenv->env;
	ret = 0;

	/*
	 * Release resources allocated by DB_ENV->open, and return it to the
	 * state it was in just before __env_open was called.  (This means
	 * state set by pre-open configuration functions must be preserved.)
	 *
	 * Refresh subsystems, in the reverse order they were opened (txn
	 * must be first, it may want to discard locks and flush the log).
	 *
	 * !!!
	 * Note that these functions, like all of __env_refresh, only undo
	 * the effects of __env_open.  Functions that undo work done by
	 * db_env_create or by a configuration function should go in
	 * __env_close.
	 */
	if (TXN_ON(env) &&
	    (t_ret = __txn_env_refresh(env)) != 0 && ret == 0)
		ret = t_ret;

	if (LOGGING_ON(env) &&
	    (t_ret = __log_env_refresh(env)) != 0 && ret == 0)
		ret = t_ret;

	/*
	 * Locking should come after logging, because closing log results
	 * in files closing which may require locks being released.
	 */
	if (LOCKING_ON(env)) {
		if (!F_ISSET(env, ENV_THREAD) &&
		    env->env_lref != NULL && (t_ret =
		    __lock_id_free(env, env->env_lref)) != 0 && ret == 0)
			ret = t_ret;
		env->env_lref = NULL;

		if ((t_ret = __lock_env_refresh(env)) != 0 && ret == 0)
			ret = t_ret;
	}

	/* Discard the DB_ENV, ENV handle mutexes. */
	if ((t_ret = __mutex_free(env, &dbenv->mtx_db_env)) != 0 && ret == 0)
		ret = t_ret;
	if ((t_ret = __mutex_free(env, &env->mtx_env)) != 0 && ret == 0)
		ret = t_ret;

	/*
	 * Discard DB list and its mutex.
	 * Discard the MT mutex.
	 *
	 * !!!
	 * This must be done after we close the log region, because we close
	 * database handles and so acquire this mutex when we close log file
	 * handles.
	 */
	if (env->db_ref != 0) {
		__db_errx(env,
		    "Database handles still open at environment close");
		TAILQ_FOREACH(ldbp, &env->dblist, dblistlinks)
			__db_errx(env, "Open database handle: %s%s%s",
			    ldbp->fname == NULL ? "unnamed" : ldbp->fname,
			    ldbp->dname == NULL ? "" : "/",
			    ldbp->dname == NULL ? "" : ldbp->dname);
		if (ret == 0)
			ret = EINVAL;
	}
	TAILQ_INIT(&env->dblist);
	if ((t_ret = __mutex_free(env, &env->mtx_dblist)) != 0 && ret == 0)
		ret = t_ret;
	if ((t_ret = __mutex_free(env, &env->mtx_mt)) != 0 && ret == 0)
		ret = t_ret;

	if (env->mt != NULL) {
		__os_free(env, env->mt);
		env->mt = NULL;
	}

	if (MPOOL_ON(env)) {
		/*
		 * If it's a private environment, flush the contents to disk.
		 * Recovery would have put everything back together, but it's
		 * faster and cleaner to flush instead.
		 *
		 * Ignore application max-write configuration, we're shutting
		 * down.
		 */
		if (F_ISSET(env, ENV_PRIVATE) &&
		    (t_ret = __memp_sync_int(env, NULL, 0,
		    DB_SYNC_CACHE | DB_SYNC_SUPPRESS_WRITE, NULL, NULL)) != 0 &&
		    ret == 0)
			ret = t_ret;

		if ((t_ret = __memp_env_refresh(env)) != 0 && ret == 0)
			ret = t_ret;
	}

	/*
	 * If we're included in a shared replication handle count, this
	 * is our last chance to decrement that count.
	 *
	 * !!!
	 * We can't afford to do anything dangerous after we decrement the
	 * handle count, of course, as replication may be proceeding with
	 * client recovery.  However, since we're discarding the regions
	 * as soon as we drop the handle count, there's little opportunity
	 * to do harm.
	 */
	if (rep_check && (t_ret = __env_db_rep_exit(env)) != 0 && ret == 0)
		ret = t_ret;

	/*
	 * Refresh the replication region.
	 *
	 * Must come after we call __env_db_rep_exit above.
	 */
	if (REP_ON(env) && (t_ret = __rep_env_refresh(env)) != 0 && ret == 0)
		ret = t_ret;

#ifdef HAVE_CRYPTO
	/*
	 * Crypto comes last, because higher level close functions need
	 * cryptography.
	 */
	if (env->reginfo != NULL &&
	    (t_ret = __crypto_env_refresh(env)) != 0 && ret == 0)
		ret = t_ret;
#endif

	/*
	 * Mark the thread as out of the env before we get rid of the handles
	 * needed to do so.
	 */
	if (env->thr_hashtab != NULL &&
	    (t_ret = __env_set_state(env, &ip, THREAD_OUT)) != 0 && ret == 0)
		ret = t_ret;

	/*
	 * We are about to detach from the mutex region.  This is the last
	 * chance we have to acquire/destroy a mutex -- acquire/destroy the
	 * mutex and release our reference.
	 *
	 * !!!
	 * There are two DbEnv methods that care about environment reference
	 * counts: DbEnv.close and DbEnv.remove.  The DbEnv.close method is
	 * not a problem because it only decrements the reference count and
	 * no actual resources are discarded -- lots of threads of control
	 * can call DbEnv.close at the same time, and regardless of racing
	 * on the reference count mutex, we wouldn't have a problem.  Since
	 * the DbEnv.remove method actually discards resources, we can have
	 * a problem.
	 *
	 * If we decrement the reference count to 0 here, go to sleep, and
	 * the DbEnv.remove method is called, by the time we run again, the
	 * underlying shared regions could have been removed.  That's fine,
	 * except we might actually need the regions to resolve outstanding
	 * operations in the various subsystems, and if we don't have hard
	 * OS references to the regions, we could get screwed.  Of course,
	 * we should have hard OS references to everything we need, but just
	 * in case, we put off decrementing the reference count as long as
	 * possible.
	 */
	if ((t_ret = __env_ref_decrement(env)) != 0 && ret == 0)
		ret = t_ret;

#ifdef HAVE_MUTEX_SUPPORT
	if (MUTEX_ON(env) &&
	    (t_ret = __mutex_env_refresh(env)) != 0 && ret == 0)
		ret = t_ret;
#endif
	/* Free memory for thread tracking. */
	if (env->reginfo != NULL) {
		if (F_ISSET(env, ENV_PRIVATE)) {
			__env_thread_destroy(env);
			t_ret = __env_detach(env, 1);
		} else
			t_ret = __env_detach(env, 0);

		if (t_ret != 0 && ret == 0)
			ret = t_ret;

		/*
		 * !!!
		 * Don't free env->reginfo or set the reference to NULL,
		 * that was done by __env_detach().
		 */
	}

	if (env->mutex_iq != NULL) {
		__os_free(env, env->mutex_iq);
		env->mutex_iq = NULL;
	}

	if (env->recover_dtab.int_dispatch != NULL) {
		__os_free(env, env->recover_dtab.int_dispatch);
		env->recover_dtab.int_size = 0;
		env->recover_dtab.int_dispatch = NULL;
	}
	if (env->recover_dtab.ext_dispatch != NULL) {
		__os_free(env, env->recover_dtab.ext_dispatch);
		env->recover_dtab.ext_size = 0;
		env->recover_dtab.ext_dispatch = NULL;
	}

	dbenv->flags = orig_flags;

	return (ret);
}

/*
 * __file_handle_cleanup --
 *	Close any underlying open file handles so we don't leak system
 *	resources.
 */
static int
__file_handle_cleanup(env)
	ENV *env;
{
	DB_FH *fhp;

	if (TAILQ_FIRST(&env->fdlist) == NULL)
		return (0);

	__db_errx(env, "File handles still open at environment close");
	while ((fhp = TAILQ_FIRST(&env->fdlist)) != NULL) {
		__db_errx(env, "Open file handle: %s", fhp->name);
		(void)__os_closehandle(env, fhp);
	}
	return (EINVAL);
}

/*
 * __env_get_open_flags
 *	DbEnv.get_open_flags method.
 *
 * PUBLIC: int __env_get_open_flags __P((DB_ENV *, u_int32_t *));
 */
int
__env_get_open_flags(dbenv, flagsp)
	DB_ENV *dbenv;
	u_int32_t *flagsp;
{
	ENV *env;

	env = dbenv->env;

	ENV_ILLEGAL_BEFORE_OPEN(env, "DB_ENV->get_open_flags");

	*flagsp = env->open_flags;
	return (0);
}
/*
 * __env_attach_regions --
 *	Perform attaches to env and required regions (subsystems)
 *
 * PUBLIC: int __env_attach_regions __P((DB_ENV *,  u_int32_t, u_int32_t, int));
 */
int
__env_attach_regions(dbenv, flags, orig_flags, retry_ok)
	DB_ENV *dbenv;
	u_int32_t flags;
	u_int32_t orig_flags;
	int retry_ok;
{
	DB_THREAD_INFO *ip;
	ENV *env;
	REGINFO *infop;
	u_int32_t init_flags;
	int create_ok, rep_check, ret;

	ip = NULL;
	env = dbenv->env;
	rep_check = 0;

	/* Convert the DB_ENV->open flags to internal flags. */
	create_ok = LF_ISSET(DB_CREATE) ? 1 : 0;
	if (LF_ISSET(DB_LOCKDOWN))
		F_SET(env, ENV_LOCKDOWN);
	if (LF_ISSET(DB_PRIVATE))
		F_SET(env, ENV_PRIVATE);
	if (LF_ISSET(DB_RECOVER_FATAL))
		F_SET(env, ENV_RECOVER_FATAL);
	if (LF_ISSET(DB_SYSTEM_MEM))
		F_SET(env, ENV_SYSTEM_MEM);
	if (LF_ISSET(DB_THREAD))
		F_SET(env, ENV_THREAD);

	/*
	 * Flags saved in the init_flags field of the environment, representing
	 * flags to DB_ENV->set_flags and DB_ENV->open that need to be set.
	 */
#define	DB_INITENV_CDB		0x0001	/* DB_INIT_CDB */
#define	DB_INITENV_CDB_ALLDB	0x0002	/* DB_INIT_CDB_ALLDB */
#define	DB_INITENV_LOCK		0x0004	/* DB_INIT_LOCK */
#define	DB_INITENV_LOG		0x0008	/* DB_INIT_LOG */
#define	DB_INITENV_MPOOL	0x0010	/* DB_INIT_MPOOL */
#define	DB_INITENV_REP		0x0020	/* DB_INIT_REP */
#define	DB_INITENV_TXN		0x0040	/* DB_INIT_TXN */

	/*
	 * Create/join the environment.  We pass in the flags of interest to
	 * a thread subsequently joining an environment we create.  If we're
	 * not the ones to create the environment, our flags will be updated
	 * to match the existing environment.
	 */
	init_flags = 0;
	if (LF_ISSET(DB_INIT_CDB))
		FLD_SET(init_flags, DB_INITENV_CDB);
	if (F_ISSET(dbenv, DB_ENV_CDB_ALLDB))
		FLD_SET(init_flags, DB_INITENV_CDB_ALLDB);
	if (LF_ISSET(DB_INIT_LOCK))
		FLD_SET(init_flags, DB_INITENV_LOCK);
	if (LF_ISSET(DB_INIT_LOG))
		FLD_SET(init_flags, DB_INITENV_LOG);
	if (LF_ISSET(DB_INIT_MPOOL))
		FLD_SET(init_flags, DB_INITENV_MPOOL);
	if (LF_ISSET(DB_INIT_REP))
		FLD_SET(init_flags, DB_INITENV_REP);
	if (LF_ISSET(DB_INIT_TXN))
		FLD_SET(init_flags, DB_INITENV_TXN);
	if ((ret = __env_attach(env, &init_flags, create_ok, retry_ok)) != 0)
		goto err;

	/*
	 * __env_attach will return the saved init_flags field, which contains
	 * the DB_INIT_* flags used when the environment was created.
	 *
	 * We may be joining an environment -- reset our flags to match the
	 * ones in the environment.
	 */
	if (FLD_ISSET(init_flags, DB_INITENV_CDB))
		LF_SET(DB_INIT_CDB);
	if (FLD_ISSET(init_flags, DB_INITENV_LOCK))
		LF_SET(DB_INIT_LOCK);
	if (FLD_ISSET(init_flags, DB_INITENV_LOG))
		LF_SET(DB_INIT_LOG);
	if (FLD_ISSET(init_flags, DB_INITENV_MPOOL))
		LF_SET(DB_INIT_MPOOL);
	if (FLD_ISSET(init_flags, DB_INITENV_REP))
		LF_SET(DB_INIT_REP);
	if (FLD_ISSET(init_flags, DB_INITENV_TXN))
		LF_SET(DB_INIT_TXN);
	if (FLD_ISSET(init_flags, DB_INITENV_CDB_ALLDB) &&
	    (ret = __env_set_flags(dbenv, DB_CDB_ALLDB, 1)) != 0)
		goto err;

	/* Initialize for CDB product. */
	if (LF_ISSET(DB_INIT_CDB)) {
		LF_SET(DB_INIT_LOCK);
		F_SET(env, ENV_CDB);
	}

	/*
	 * Update the flags to match the database environment.  The application
	 * may have specified flags of 0 to join the environment, and this line
	 * replaces that value with the flags corresponding to the existing,
	 * underlying set of subsystems.  This means the DbEnv.get_open_flags
	 * method returns the flags to open the existing environment instead of
	 * the specific flags passed to the DbEnv.open method.
	 */
	env->open_flags = flags;

	/*
	 * The DB_ENV structure has now been initialized.  Turn off further
	 * use of the DB_ENV structure and most initialization methods, we're
	 * about to act on the values we currently have.
	 */
	F_SET(env, ENV_OPEN_CALLED);

	infop = env->reginfo;

#ifdef HAVE_MUTEX_SUPPORT
	/*
	 * Initialize the mutex regions first before ENV_ENTER().
	 * Mutexes need to be 'on' when attaching to an existing env
	 * in order to safely allocate the thread tracking info.
	 */
	if ((ret = __mutex_open(env, create_ok)) != 0)
		goto err;
	/* The MUTEX_REQUIRED() in __env_alloc() expectes this to be set. */
	infop->mtx_alloc = ((REGENV *)infop->primary)->mtx_regenv;
#endif
	/*
	 * Initialize thread tracking and enter the API.
	 */
	if ((ret =
	    __env_thread_init(env, F_ISSET(infop, REGION_CREATE) ? 1 : 0)) != 0)
		goto err;

	ENV_ENTER(env, ip);

	/*
	 * Initialize the subsystems.
	 */
	/*
	 * We can now acquire/create mutexes: increment the region's reference
	 * count.
	 */
	if ((ret = __env_ref_increment(env)) != 0)
		goto err;

	/*
	 * Initialize the handle mutexes.
	 */
	if ((ret = __mutex_alloc(env,
	    MTX_ENV_HANDLE, DB_MUTEX_PROCESS_ONLY, &dbenv->mtx_db_env)) != 0 ||
	    (ret = __mutex_alloc(env,
	    MTX_ENV_HANDLE, DB_MUTEX_PROCESS_ONLY, &env->mtx_env)) != 0)
		goto err;

	/*
	 * Initialize the replication area next, so that we can lock out this
	 * call if we're currently running recovery for replication.
	 */
	if (LF_ISSET(DB_INIT_REP) && (ret = __rep_open(env)) != 0)
		goto err;
	infop->mtx_alloc = ((REGENV *)infop->primary)->mtx_regenv;

	rep_check = IS_ENV_REPLICATED(env) ? 1 : 0;
	if (rep_check && (ret = __env_rep_enter(env, 0)) != 0)
		goto err;

	if (LF_ISSET(DB_INIT_MPOOL)) {
		if ((ret = __memp_open(env, create_ok)) != 0)
			goto err;

		/*
		 * BDB does do cache I/O during recovery and when starting up
		 * replication.  If creating a new environment, then suppress
		 * any application max-write configuration.
		 */
		if (create_ok)
			(void)__memp_set_config(
			    dbenv, DB_MEMP_SUPPRESS_WRITE, 1);

		/*
		 * Initialize the DB list and its mutex.  If the mpool is
		 * not initialized, we can't ever open a DB handle, which
		 * is why this code lives here.
		 */
		TAILQ_INIT(&env->dblist);
		if ((ret = __mutex_alloc(env, MTX_ENV_DBLIST,
		    DB_MUTEX_PROCESS_ONLY, &env->mtx_dblist)) != 0)
			goto err;

		/* Register DB's pgin/pgout functions.  */
		if ((ret = __memp_register(
		    env, DB_FTYPE_SET, __db_pgin, __db_pgout)) != 0)
			goto err;
	}

	/*
	 * Initialize the ciphering area prior to any running of recovery so
	 * that we can initialize the keys, etc. before recovery, including
	 * the MT mutex.
	 *
	 * !!!
	 * This must be after the mpool init, but before the log initialization
	 * because log_open may attempt to run log_recover during its open.
	 */
	if (LF_ISSET(DB_INIT_MPOOL | DB_INIT_LOG | DB_INIT_TXN) &&
	    (ret = __crypto_region_init(env)) != 0)
		goto err;
	if ((ret = __mutex_alloc(
	    env, MTX_TWISTER, DB_MUTEX_PROCESS_ONLY, &env->mtx_mt)) != 0)
		goto err;

	/*
	 * Transactions imply logging but do not imply locking.  While almost
	 * all applications want both locking and logging, it would not be
	 * unreasonable for a single threaded process to want transactions for
	 * atomicity guarantees, but not necessarily need concurrency.
	 */
	if (LF_ISSET(DB_INIT_LOG | DB_INIT_TXN))
		if ((ret = __log_open(env, create_ok)) != 0)
			goto err;
	if (LF_ISSET(DB_INIT_LOCK))
		if ((ret = __lock_open(env, create_ok)) != 0)
			goto err;

	if (LF_ISSET(DB_INIT_TXN)) {
		if ((ret = __txn_open(env, create_ok)) != 0)
			goto err;

		/*
		 * If the application is running with transactions, initialize
		 * the function tables.
		 */
		if ((ret = __env_init_rec(env, DB_LOGVERSION)) != 0)
			goto err;
	}

	/* Perform recovery for any previous run. */
	if (LF_ISSET(DB_RECOVER | DB_RECOVER_FATAL) &&
	    (ret = __db_apprec(env, ip, NULL, NULL, 1,
	    LF_ISSET(DB_RECOVER | DB_RECOVER_FATAL))) != 0)
		goto err;

	/*
	 * If we've created the regions, are running with transactions, and did
	 * not just run recovery, we need to log the fact that the transaction
	 * IDs got reset.
	 *
	 * If we ran recovery, there may be prepared-but-not-yet-committed
	 * transactions that need to be resolved.  Recovery resets the minimum
	 * transaction ID and logs the reset if that's appropriate, so we
	 * don't need to do anything here in the recover case.
	 */
	if (TXN_ON(env) &&
	    !FLD_ISSET(dbenv->lg_flags, DB_LOG_IN_MEMORY) &&
	    F_ISSET(infop, REGION_CREATE) &&
	    !LF_ISSET(DB_RECOVER | DB_RECOVER_FATAL) &&
	    (ret = __txn_reset(env)) != 0)
		goto err;

	/* The database environment is ready for business. */
	if ((ret = __env_turn_on(env)) != 0)
		goto err;

	if (rep_check)
		ret = __env_db_rep_exit(env);

	/* Turn any application-specific max-write configuration back on. */
	if (LF_ISSET(DB_INIT_MPOOL))
		(void)__memp_set_config(dbenv, DB_MEMP_SUPPRESS_WRITE, 0);

err:	if (ret == 0)
		ENV_LEAVE(env, ip);
	else {
		/*
		 * If we fail after creating regions, panic and remove them.
		 *
		 * !!!
		 * No need to call __env_db_rep_exit, that work is done by the
		 * calls to __env_refresh.
		 */
		infop = env->reginfo;
		if (infop != NULL && F_ISSET(infop, REGION_CREATE)) {
			ret = __env_panic(env, ret);

			/* Refresh the DB_ENV so can use it to call remove. */
			(void)__env_refresh(dbenv, orig_flags, rep_check);
			(void)__env_remove_env(env);
			(void)__env_refresh(dbenv, orig_flags, 0);
		} else
			(void)__env_refresh(dbenv, orig_flags, rep_check);
		/* clear the fact that the region had been opened */
		F_CLR(env, ENV_OPEN_CALLED);
	}

	return (ret);
}
