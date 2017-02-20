/*-
 * See the file LICENSE for redistribution information.
 *
 * Copyright (c) 2001, 2010 Oracle and/or its affiliates.  All rights reserved.
 *
 * $Id$
 */

#include "db_config.h"

#include "db_int.h"
#include "dbinc/db_page.h"
#include "dbinc/db_am.h"
#include "dbinc/hash.h"
#include "dbinc/fop.h"
#include "dbinc/lock.h"
#include "dbinc/mp.h"
#include "dbinc/log.h"
#include "dbinc/txn.h"

static int __fop_set_pgsize __P((DB *, DB_FH *, const char *));
static int __fop_inmem_create __P((DB *, const char *, DB_TXN *, u_int32_t));
static int __fop_inmem_dummy __P((DB *, DB_TXN *, const char *, u_int8_t *));
static int __fop_inmem_read_meta __P((DB *, DB_TXN *, const char *, u_int32_t));
static int __fop_inmem_swap __P((DB *, DB *, DB_TXN *,
	       const char *, const char *, const char *, DB_LOCKER *));
static int __fop_ondisk_dummy __P((DB *, DB_TXN *, const char *, u_int8_t *));
static int __fop_ondisk_swap __P((DB *, DB *, DB_TXN *,
	     const char *, const char *, const char *, DB_LOCKER *));

/*
 * Acquire the environment meta-data lock.  The parameters are the
 * environment (ENV), the locker id to use in acquiring the lock (ID)
 * and a pointer to a DB_LOCK.
 *
 * !!!
 * Turn off locking for Critical Path.  The application must do its own
 * synchronization of open/create.  Two threads creating and opening a
 * file at the same time may have unpredictable results.
 */
#ifdef CRITICALPATH_10266
#define	GET_ENVLOCK(ENV, ID, L) (0)
#else
#define	GET_ENVLOCK(ENV, ID, L) do {					\
	DBT __dbt;							\
	u_int32_t __lockval;						\
									\
	if (LOCKING_ON((ENV))) {					\
		__lockval = 1;						\
		__dbt.data = &__lockval;				\
		__dbt.size = sizeof(__lockval);				\
		if ((ret = __lock_get((ENV), (ID),			\
		    0, &__dbt, DB_LOCK_WRITE, (L))) != 0)		\
			goto err;					\
	}								\
} while (0)
#endif

#define	RESET_MPF(D, F) do {						\
	(void)__memp_fclose((D)->mpf, (F));				\
	(D)->mpf = NULL;						\
	F_CLR((D), DB_AM_OPEN_CALLED);					\
	if ((ret = __memp_fcreate((D)->env, &(D)->mpf)) != 0)		\
		goto err;						\
} while (0)

/*
 * If we open a file handle and our caller is doing fcntl(2) locking,
 * we can't close the handle because that would discard the caller's
 * lock. Save it until we close or refresh the DB handle.
 */
#define	CLOSE_HANDLE(D, F) {						\
	if ((F) != NULL) {						\
		if (LF_ISSET(DB_FCNTL_LOCKING))				\
			(D)->saved_open_fhp = (F);			\
		else if ((t_ret =					\
		    __os_closehandle((D)->env, (F))) != 0) {		\
			if (ret == 0)					\
				ret = t_ret;				\
			goto err;					\
		}							\
		(F) = NULL;						\
	}								\
}

/*
 * __fop_lock_handle --
 *
 * Get the handle lock for a database.  If the envlock is specified, do this
 * as a lock_vec call that releases the environment lock before acquiring the
 * handle lock.
 *
 * PUBLIC: int __fop_lock_handle __P((ENV *,
 * PUBLIC:     DB *, DB_LOCKER *, db_lockmode_t, DB_LOCK *, u_int32_t));
 *
 */
int
__fop_lock_handle(env, dbp, locker, mode, elockp, flags)
	ENV *env;
	DB *dbp;
	DB_LOCKER *locker;
	db_lockmode_t mode;
	DB_LOCK *elockp;
	u_int32_t flags;
{
	DBT fileobj;
	DB_LOCKREQ reqs[2], *ereq;
	DB_LOCK_ILOCK lock_desc;
	int ret;

	if (!LOCKING_ON(env) ||
	    F_ISSET(dbp, DB_AM_COMPENSATE | DB_AM_RECOVER))
		return (0);

	/*
	 * If we are in recovery, the only locking we should be
	 * doing is on the global environment.
	 */
	if (IS_RECOVERING(env))
		return (elockp == NULL ? 0 : __ENV_LPUT(env, *elockp));

	memcpy(lock_desc.fileid, dbp->fileid, DB_FILE_ID_LEN);
	lock_desc.pgno = dbp->meta_pgno;
	lock_desc.type = DB_HANDLE_LOCK;

	memset(&fileobj, 0, sizeof(fileobj));
	fileobj.data = &lock_desc;
	fileobj.size = sizeof(lock_desc);
	DB_TEST_SUBLOCKS(env, flags);
	if (elockp == NULL)
		ret = __lock_get(env, locker,
		    flags, &fileobj, mode, &dbp->handle_lock);
	else {
		reqs[0].op = DB_LOCK_PUT;
		reqs[0].lock = *elockp;
		reqs[1].op = DB_LOCK_GET;
		reqs[1].mode = mode;
		reqs[1].obj = &fileobj;
		reqs[1].timeout = 0;
		if ((ret = __lock_vec(env,
		    locker, flags, reqs, 2, &ereq)) == 0) {
			dbp->handle_lock = reqs[1].lock;
			LOCK_INIT(*elockp);
		} else if (ereq != reqs)
			LOCK_INIT(*elockp);
	}

	dbp->cur_locker = locker;
	return (ret);
}

/*
 * __fop_file_setup --
 *
 * Perform all the needed checking and locking to open up or create a
 * file.
 *
 * There's a reason we don't push this code down into the buffer cache.
 * The problem is that there's no information external to the file that
 * we can use as a unique ID.  UNIX has dev/inode pairs, but they are
 * not necessarily unique after reboot, if the file was mounted via NFS.
 * Windows has similar problems, as the FAT filesystem doesn't maintain
 * dev/inode numbers across reboot.  So, we must get something from the
 * file we can use to ensure that, even after a reboot, the file we're
 * joining in the cache is the right file for us to join.  The solution
 * we use is to maintain a file ID that's stored in the database, and
 * that's why we have to open and read the file before calling into the
 * buffer cache or obtaining a lock (we use this unique fileid to lock
 * as well as to identify like files in the cache).
 *
 * There are a couple of idiosyncrasies that this code must support, in
 * particular, DB_TRUNCATE and DB_FCNTL_LOCKING.  First, we disallow
 * DB_TRUNCATE in the presence of transactions, since opening a file with
 * O_TRUNC will result in data being lost in an unrecoverable fashion.
 * We also disallow DB_TRUNCATE if locking is enabled, because even in
 * the presence of locking, we cannot avoid race conditions, so allowing
 * DB_TRUNCATE with locking would be misleading.  See SR [#7345] for more
 * details.
 *
 * However, if you are running with neither locking nor transactions, then
 * you can specify DB_TRUNCATE, and if you do so, we will truncate the file
 * regardless of its contents.
 *
 * FCNTL locking introduces another set of complications.  First, the only
 * reason we support the DB_FCNTL_LOCKING flag is for historic compatibility
 * with programs like Sendmail and Postfix.  In these cases, the caller may
 * already have a lock on the file; we need to make sure that any file handles
 * we open remain open, because if we were to close them, the lock held by the
 * caller would go away.  Furthermore, Sendmail and/or Postfix need the ability
 * to create databases in empty files.  So, when you're doing FCNTL locking,
 * it's reasonable that you are trying to create a database into a 0-length
 * file and we allow it, while under normal conditions, we do not create
 * databases if the files already exist and are not Berkeley DB files.
 *
 * PUBLIC: int __fop_file_setup __P((DB *, DB_THREAD_INFO *ip,
 * PUBLIC:     DB_TXN *, const char *, int, u_int32_t, u_int32_t *));
 */
int
__fop_file_setup(dbp, ip, txn, name, mode, flags, retidp)
	DB *dbp;
	DB_THREAD_INFO *ip;
	DB_TXN *txn;
	const char *name;
	int mode;
	u_int32_t flags, *retidp;
{
	DBTYPE save_type;
	DB_FH *fhp;
	DB_LOCK elock;
	DB_LOCKER *locker;
	DB_TXN *stxn;
	ENV *env;
	size_t len;
	u_int32_t dflags, oflags;
	u_int8_t mbuf[DBMETASIZE];
	int created_locker, create_ok, ret, retries, t_ret, tmp_created;
	int truncating, was_inval;
	char *real_name, *real_tmpname, *tmpname;

	*retidp = TXN_INVALID;

	env = dbp->env;
	fhp = NULL;
	LOCK_INIT(elock);
	stxn = NULL;
	created_locker = tmp_created = truncating = was_inval = 0;
	real_name = real_tmpname = tmpname = NULL;
	dflags = F_ISSET(dbp, DB_AM_NOT_DURABLE) ? DB_LOG_NOT_DURABLE : 0;

	ret = 0;
	retries = 0;
	save_type = dbp->type;

	/*
	 * Get a lockerid for this handle.  There are paths through queue
	 * rename and remove where this dbp already has a locker, so make
	 * sure we don't clobber it and conflict.
	 */
	if (LOCKING_ON(env) &&
	    !F_ISSET(dbp, DB_AM_COMPENSATE) &&
	    !F_ISSET(dbp, DB_AM_RECOVER) &&
	    dbp->locker == DB_LOCK_INVALIDID) {
		if ((ret = __lock_id(env, NULL, &dbp->locker)) != 0)
			goto err;
		created_locker = 1;
	}
	LOCK_INIT(dbp->handle_lock);

	locker = txn == NULL ? dbp->locker : txn->locker;

	oflags = 0;
	if (F_ISSET(dbp, DB_AM_INMEM))
		real_name = (char *)name;
	else {
		/* Get the real backing file name. */
		if ((ret = __db_appname(env,
		    DB_APP_DATA, name, &dbp->dirname, &real_name)) != 0)
			goto err;

		/* Fill in the default file mode. */
		if (mode == 0)
			mode = DB_MODE_660;

		if (LF_ISSET(DB_RDONLY))
			oflags |= DB_OSO_RDONLY;
		if (LF_ISSET(DB_TRUNCATE))
			oflags |= DB_OSO_TRUNC;
	}

	retries = 0;
	create_ok = LF_ISSET(DB_CREATE);
	LF_CLR(DB_CREATE);

retry:
	/*
	 * If we cannot create the file, only retry a few times.  We
	 * think we might be in a race with another create, but it could
	 * be that the backup filename exists (that is, is left over from
	 * a previous crash).
	 */
	if (++retries > DB_RETRY) {
		__db_errx(env, "__fop_file_setup:  Retry limit (%d) exceeded",
		    DB_RETRY);
		goto err;
	}
	if (!F_ISSET(dbp, DB_AM_COMPENSATE) && !F_ISSET(dbp, DB_AM_RECOVER))
		GET_ENVLOCK(env, locker, &elock);
	if (name == NULL)
		ret = ENOENT;
	else if (F_ISSET(dbp, DB_AM_INMEM)) {
		ret = __env_mpool(dbp, name, flags);
		/*
		 * We are using __env_open as a check for existence.
		 * However, __env_mpool does an actual open and there
		 * are scenarios where the object exists, but cannot be
		 * opened, because our settings don't match those internally.
		 * We need to check for that explicitly.  We'll need the
		 * mpool open to read the meta-data page, so we're going to
		 * have to temporarily turn this dbp into an UNKNOWN one.
		 */
		if (ret == EINVAL) {
			was_inval = 1;
			save_type = dbp->type;
			dbp->type = DB_UNKNOWN;
			ret = __env_mpool(dbp, name, flags);
			dbp->type = save_type;
		}
	} else
		ret = __os_exists(env, real_name, NULL);

	if (ret == 0) {
		/*
		 * If the file exists, there are 5 possible cases:
		 * 1. DB_EXCL was specified so this is an error, unless
		 *	this is a file left around after a rename and we
		 *	are in the same transaction.  This gets decomposed
		 *	into several subcases, because we check for various
		 *	errors before we know we're in rename.
		 * 2. We are truncating, and it doesn't matter what kind
		 *	of file it is, we should open/create it.
		 * 3. It is 0-length, we are not doing transactions (i.e.,
		 *      we are sendmail), we should open/create into it.
		 *	-- on-disk files only!
		 * 4. Is it a Berkeley DB file and we should simply open it.
		 * 5. It is not a BDB file and we should return an error.
		 */

		/* Open file (if there is one). */
reopen:		if (!F_ISSET(dbp, DB_AM_INMEM) && (ret =
		    __os_open(env, real_name, 0, oflags, 0, &fhp)) != 0)
			goto err;

		/* Case 2: DB_TRUNCATE: we must do the creation in place. */
		if (LF_ISSET(DB_TRUNCATE)) {
			if (LF_ISSET(DB_EXCL)) {
				/* Case 1a: DB_EXCL and DB_TRUNCATE. */
				ret = EEXIST;
				goto err;
			}
			tmpname = (char *)name;
			goto creat2;
		}

		/* Cases 1,3-5: we need to read the meta-data page. */
		if (F_ISSET(dbp, DB_AM_INMEM))
			ret = __fop_inmem_read_meta(dbp, txn, name, flags);
		else {
			ret = __fop_read_meta(env, real_name, mbuf,
			    sizeof(mbuf), fhp,
			    LF_ISSET(DB_NOERROR) || 
			    (LF_ISSET(DB_FCNTL_LOCKING) && txn == NULL) ? 1 : 0,
			    &len);

			/* Case 3: 0-length, no txns. */
			if (ret != 0 && len == 0 && txn == NULL) {
				if (LF_ISSET(DB_EXCL)) {
					/*
					 * Case 1b: DB_EXCL and
					 * 0-length file exists.
					 */
					ret = EEXIST;
					goto err;
				}
				tmpname = (char *)name;
				if (create_ok)
					goto creat2;
				goto done;
			}

			/* Case 4: This is a valid file. */
			if (ret == 0)
				ret = __db_meta_setup(env, dbp, real_name,
				    (DBMETA *)mbuf, flags, DB_CHK_META);

		}

		/* Case 5: Invalid file. */
		if (ret != 0)
			goto err;

		/* Now, get our handle lock. */
		if ((ret = __fop_lock_handle(env,
		    dbp, locker, DB_LOCK_READ, NULL, DB_LOCK_NOWAIT)) == 0) {
			if ((ret = __ENV_LPUT(env, elock)) != 0)
				goto err;
		} else if (ret != DB_LOCK_NOTGRANTED ||
		    (txn != NULL && F_ISSET(txn, TXN_NOWAIT)))
			goto err;
		else {
			/*
			 * We were unable to acquire the handle lock without
			 * blocking.  The fact that we are blocking might mean
			 * that someone else is trying to delete the file.
			 * Since some platforms cannot delete files while they
			 * are open (Windows), we are going to have to close
			 * the file.  This would be a problem if we were doing
			 * FCNTL locking, because our closing the handle would
			 * release the FCNTL locks.  Fortunately, if we are
			 * doing FCNTL locking, then we should never fail to
			 * acquire our handle lock, so we should never get here.
			 * We assert it here to make sure we aren't destroying
			 * any application level FCNTL semantics.
			 */
			DB_ASSERT(env, !LF_ISSET(DB_FCNTL_LOCKING));
			if (!F_ISSET(dbp, DB_AM_INMEM)) {
				if ((ret = __os_closehandle(env, fhp)) != 0)
					goto err;
				fhp = NULL;
			}
			if ((ret = __fop_lock_handle(env,
			    dbp, locker, DB_LOCK_READ, &elock, 0)) != 0) {
				if (F_ISSET(dbp, DB_AM_INMEM))
					RESET_MPF(dbp, 0);
				goto err;
			}

			/*
			 * If we had to wait, we might be waiting on a
			 * dummy file used in create/destroy of a database.
			 * To be sure we have the correct information we
			 * try again.
			 */
			if ((ret = __db_refresh(dbp,
			    txn, DB_NOSYNC, NULL, 1)) != 0)
				goto err;
			if ((ret =
			    __ENV_LPUT(env, dbp->handle_lock)) != 0) {
				LOCK_INIT(dbp->handle_lock);
				goto err;
			}
			goto retry;

		}

		/* If we got here, then we have the handle lock. */

		/*
		 * Check for a file in the midst of a rename.  If we find that
		 * the file is in the midst of a rename, it must be the case
		 * that it is in our current transaction (else we would still
		 * be blocking), so we can continue along and create a new file
		 * with the same name.  In that case, we have to close the file
		 * handle because we reuse it below.  This is a case where
		 * a 'was_inval' above is OK.
		 */
		if (F_ISSET(dbp, DB_AM_IN_RENAME)) {
			was_inval = 0;
			if (create_ok) {
				if (F_ISSET(dbp, DB_AM_INMEM)) {
					RESET_MPF(dbp, DB_MPOOL_DISCARD);
				} else if ((ret =
				    __os_closehandle(env, fhp)) != 0)
					goto err;
				LF_SET(DB_CREATE);
				goto create;
			} else {
				ret = ENOENT;
				goto err;
			}
		}

		/* If we get here, a was_inval is bad. */
		if (was_inval) {
			ret = EINVAL;
			goto err;
		}

		/*
		 * Now, case 1: check for DB_EXCL, because the file that exists
		 * is not in the middle of a rename, so we have an error.  This
		 * is a weird case, but we need to make sure that we don't
		 * continue to hold the handle lock, since technically, we
		 * should not have been allowed to open it.
		 */
		if (LF_ISSET(DB_EXCL)) {
			ret = __ENV_LPUT(env, dbp->handle_lock);
			LOCK_INIT(dbp->handle_lock);
			if (ret == 0)
				ret = EEXIST;
			goto err;
		}
		goto done;
	}

	/* File does not exist. */
#ifdef	HAVE_VXWORKS
	/*
	 * VxWorks can return file-system specific error codes if the
	 * file does not exist, not ENOENT.
	 */
	if (!create_ok)
#else
	if (!create_ok || ret != ENOENT)
#endif
		goto err;
	LF_SET(DB_CREATE);
	ret = 0;

	/*
	 * We need to create file, which means that we need to set up the file,
	 * the fileid and the locks.  Then we need to call the appropriate
	 * routines to create meta-data pages.  For in-memory files, we retain
	 * the environment lock, while for on-disk files, we drop the env lock
	 * and create into a temporary.
	 */
	if (!F_ISSET(dbp, DB_AM_INMEM) &&
	    (ret = __ENV_LPUT(env, elock)) != 0)
		goto err;

create:	if (txn != NULL && IS_REP_CLIENT(env) &&
	    !F_ISSET(dbp, DB_AM_NOT_DURABLE)) {
		__db_errx(env,
		    "Transactional create on replication client disallowed");
		ret = EINVAL;
		goto err;
	}

	if (F_ISSET(dbp, DB_AM_INMEM))
		ret = __fop_inmem_create(dbp, name, txn, flags);
	else {
		if ((ret = __db_backup_name(env, name, txn, &tmpname)) != 0)
			goto err;
		if (TXN_ON(env) && txn != NULL &&
		    (ret = __txn_begin(env, NULL, txn, &stxn, 0)) != 0)
			goto err;
		if ((ret = __fop_create(env, stxn, &fhp,
		    tmpname, &dbp->dirname, DB_APP_DATA, mode, dflags)) != 0) {
			/*
			 * If no transactions, there is a race on creating the
			 * backup file, as the backup file name is the same for
			 * all processes.  Wait for the other process to finish
			 * with the name.
			 */
			if (!TXN_ON(env) && ret == EEXIST) {
				__os_free(env, tmpname);
				tmpname = NULL;
				__os_yield(env, 1, 0);
				goto retry;
			}
			goto err;
		}
		tmp_created = 1;
	}

creat2:	if (!F_ISSET(dbp, DB_AM_INMEM)) {
		if ((ret = __db_appname(env,
		    DB_APP_DATA, tmpname, &dbp->dirname, &real_tmpname)) != 0)
			goto err;

		/* Set the pagesize if it isn't yet set. */
		if (dbp->pgsize == 0 &&
		    (ret = __fop_set_pgsize(dbp, fhp, real_tmpname)) != 0)
			goto errmsg;

		/* Construct a file_id. */
		if ((ret =
		    __os_fileid(env, real_tmpname, 1, dbp->fileid)) != 0)
			goto errmsg;
	}

	if ((ret = __db_new_file(dbp, ip,
	    F_ISSET(dbp, DB_AM_INMEM) ? txn : stxn, fhp, tmpname)) != 0)
		goto err;

	/*
	 * We need to close the handle here on platforms where remove and
	 * rename fail if a handle is open (including Windows).
	 */
	CLOSE_HANDLE(dbp, fhp);

	/*
	 * Now move the file into place unless we are creating in place (because
	 * we created a database in a file that started out 0-length).  If
	 * this is an in-memory file, we may or may not hold the environment
	 * lock depending on how we got here.
	 */
	if (!F_ISSET(dbp, DB_AM_COMPENSATE) &&
	    !F_ISSET(dbp, DB_AM_RECOVER) && !LOCK_ISSET(elock))
		GET_ENVLOCK(env, locker, &elock);

	if (F_ISSET(dbp, DB_AM_IN_RENAME)) {
		F_CLR(dbp, DB_AM_IN_RENAME);
		__txn_remrem(env, txn, real_name);
	} else if (name == tmpname) {
		/* We created it in place. */
	} else if (!F_ISSET(dbp, DB_AM_INMEM) &&
	    __os_exists(env, real_name, NULL) == 0) {
		/*
		 * Someone managed to create the file; remove our temp
		 * and try to open the file that now exists.
		 */
		(void)__fop_remove(env, NULL,
		    dbp->fileid, tmpname, &dbp->dirname, DB_APP_DATA, dflags);
		(void)__ENV_LPUT(env, dbp->handle_lock);
		LOCK_INIT(dbp->handle_lock);

		if (stxn != NULL) {
			ret = __txn_abort(stxn);
			stxn = NULL;
		}
		if (ret != 0)
			goto err;
		goto reopen;
	}

	if (name != NULL && (ret = __fop_lock_handle(env,
	    dbp, locker, DB_LOCK_WRITE, NULL, NOWAIT_FLAG(txn))) != 0)
		goto err;
	if (tmpname != NULL &&
	    tmpname != name && (ret = __fop_rename(env, stxn, tmpname,
	    name, &dbp->dirname, dbp->fileid, DB_APP_DATA, 1, dflags)) != 0)
		goto err;
	if ((ret = __ENV_LPUT(env, elock)) != 0)
		goto err;

	if (stxn != NULL) {
		*retidp = stxn->txnid;
		ret = __txn_commit(stxn, 0);
		stxn = NULL;
	} else
		*retidp = TXN_INVALID;

	if (ret != 0)
		goto err;

	F_SET(dbp, DB_AM_CREATED);

	if (0) {
errmsg:		__db_err(env, ret, "%s", name);

err:		CLOSE_HANDLE(dbp, fhp);
		if (stxn != NULL)
			(void)__txn_abort(stxn);
		if (tmp_created && txn == NULL)
			(void)__fop_remove(env,
			    NULL, NULL, tmpname, NULL, DB_APP_DATA, dflags);
		if (txn == NULL)
			(void)__ENV_LPUT(env, dbp->handle_lock);
		(void)__ENV_LPUT(env, elock);
		if (created_locker) {
			(void)__lock_id_free(env, dbp->locker);
			dbp->locker = NULL;
		}
	}

done:	/*
	 * There are cases where real_name and tmpname take on the
	 * exact same string, so we need to make sure that we do not
	 * free twice.
	 */
	if (!truncating && tmpname != NULL && tmpname != name)
		__os_free(env, tmpname);
	if (real_name != name && real_name != NULL)
		__os_free(env, real_name);
	if (real_tmpname != NULL)
		__os_free(env, real_tmpname);
	CLOSE_HANDLE(dbp, fhp);

	return (ret);
}

/*
 * __fop_set_pgsize --
 *	Set the page size based on file information.
 */
static int
__fop_set_pgsize(dbp, fhp, name)
	DB *dbp;
	DB_FH *fhp;
	const char *name;
{
	ENV *env;
	u_int32_t iopsize;
	int ret;

	env = dbp->env;

	/*
	 * Use the filesystem's optimum I/O size as the pagesize if a pagesize
	 * not specified.  Some filesystems have 64K as their optimum I/O size,
	 * but as that results in fairly large default caches, we limit the
	 * default pagesize to 16K.
	 */
	if ((ret = __os_ioinfo(env, name, fhp, NULL, NULL, &iopsize)) != 0) {
		__db_err(env, ret, "%s", name);
		return (ret);
	}
	if (iopsize < 512)
		iopsize = 512;
	if (iopsize > 16 * 1024)
		iopsize = 16 * 1024;

	/*
	 * Sheer paranoia, but we don't want anything that's not a power-of-2
	 * (we rely on that for alignment of various types on the pages), and
	 * we want a multiple of the sector size as well.  If the value
	 * we got out of __os_ioinfo looks bad, use a default instead.
	 */
	if (!IS_VALID_PAGESIZE(iopsize))
		iopsize = DB_DEF_IOSIZE;

	dbp->pgsize = iopsize;
	F_SET(dbp, DB_AM_PGDEF);

	return (0);
}

/*
 * __fop_subdb_setup --
 *
 * Subdb setup is significantly simpler than file setup.  In terms of
 * locking, for the duration of the operation/transaction, the locks on
 * the meta-data page will suffice to protect us from simultaneous operations
 * on the sub-database.  Before we complete the operation though, we'll get a
 * handle lock on the subdatabase so that on one else can try to remove it
 * while we've got it open.  We use an object that looks like the meta-data
 * page lock with a different type (DB_HANDLE_LOCK) for the long-term handle.
 * locks.
 *
 * PUBLIC: int __fop_subdb_setup __P((DB *, DB_THREAD_INFO *, DB_TXN *,
 * PUBLIC:     const char *, const char *, int, u_int32_t));
 */
int
__fop_subdb_setup(dbp, ip, txn, mname, name, mode, flags)
	DB *dbp;
	DB_THREAD_INFO *ip;
	DB_TXN *txn;
	const char *mname, *name;
	int mode;
	u_int32_t flags;
{
	DB *mdbp;
	ENV *env;
	db_lockmode_t lkmode;
	int ret, t_ret;

	mdbp = NULL;
	env = dbp->env;

	if ((ret = __db_master_open(dbp,
	    ip, txn, mname, flags, mode, &mdbp)) != 0)
		return (ret);
	/*
	 * If we created this file, then we need to set the DISCARD flag so
	 * that if we fail in the middle of this routine, we discard from the
	 * mpool any pages that we just created.
	 */
	if (F_ISSET(mdbp, DB_AM_CREATED))
		F_SET(mdbp, DB_AM_DISCARD);

	/*
	 * We are going to close this instance of the master, so we can
	 * steal its handle instead of reopening a handle on the database.
	 */
	if (LF_ISSET(DB_FCNTL_LOCKING)) {
		dbp->saved_open_fhp = mdbp->saved_open_fhp;
		mdbp->saved_open_fhp = NULL;
	}

	/* Copy the pagesize and set the sub-database flag. */
	dbp->pgsize = mdbp->pgsize;
	F_SET(dbp, DB_AM_SUBDB);

	if (name != NULL && (ret = __db_master_update(mdbp, dbp,
	    ip, txn, name, dbp->type, MU_OPEN, NULL, flags)) != 0)
		goto err;

	/*
	 * Hijack the master's locker ID as well, so that our locks don't
	 * conflict with the master's.  Since we're closing the master,
	 * that locker would just have been freed anyway.  Once we've gotten
	 * the locker id, we need to acquire the handle lock for this
	 * subdatabase.
	 */
	dbp->locker = mdbp->locker;
	mdbp->locker = NULL;

	DB_TEST_RECOVERY(dbp, DB_TEST_POSTLOG, ret, mname);

	/*
	 * We copy our fileid from our master so that we all open
	 * the same file in mpool.  We'll use the meta-pgno to lock
	 * so that we end up with different handle locks.
	 */

	memcpy(dbp->fileid, mdbp->fileid, DB_FILE_ID_LEN);
	lkmode = F_ISSET(dbp, DB_AM_CREATED) || LF_ISSET(DB_WRITEOPEN) ?
	    DB_LOCK_WRITE : DB_LOCK_READ;
	if ((ret = __fop_lock_handle(env, dbp,
	    txn == NULL ? dbp->locker : txn->locker, lkmode, NULL,
	    NOWAIT_FLAG(txn))) != 0)
		goto err;

	if ((ret = __db_init_subdb(mdbp, dbp, name, ip, txn)) != 0) {
		/*
		 * If there was no transaction and we created this database,
		 * then we need to undo the update of the master database.
		 */
		if (F_ISSET(dbp, DB_AM_CREATED) && txn == NULL)
			(void)__db_master_update(mdbp, dbp,
			    ip, txn, name, dbp->type, MU_REMOVE, NULL, 0);
		F_CLR(dbp, DB_AM_CREATED);
		goto err;
	}

	/*
	 * XXX
	 * This should have been done at the top of this routine.  The problem
	 * is that __db_init_subdb() uses "standard" routines to process the
	 * meta-data page and set information in the DB handle based on it.
	 * Those routines have to deal with swapped pages and will normally set
	 * the DB_AM_SWAP flag.  However, we use the master's metadata page and
	 * that has already been swapped, so they get the is-swapped test wrong.
	 */
	F_CLR(dbp, DB_AM_SWAP);
	F_SET(dbp, F_ISSET(mdbp, DB_AM_SWAP));

	/*
	 * In the file create case, these happen in separate places so we have
	 * two different tests.  They end up in the same place for subdbs, but
	 * for compatibility with file testing, we put them both here anyway.
	 */
	DB_TEST_RECOVERY(dbp, DB_TEST_POSTLOGMETA, ret, mname);
	DB_TEST_RECOVERY(dbp, DB_TEST_POSTSYNC, ret, mname);

	/*
	 * File exists and we have the appropriate locks; we should now
	 * process a normal open.
	 */
	if (F_ISSET(mdbp, DB_AM_CREATED)) {
		F_SET(dbp, DB_AM_CREATED_MSTR);
		F_CLR(mdbp, DB_AM_DISCARD);
	}

	if (0) {
err:
DB_TEST_RECOVERY_LABEL
		if (txn == NULL)
			(void)__ENV_LPUT(env, dbp->handle_lock);
	}

	/*
	 * The master's handle lock is under the control of the
	 * subdb (it acquired the master's locker).  We want to
	 * keep the master's handle lock so that no one can remove
	 * the file while the subdb is open.  If we register the
	 * trade event and then invalidate the copy of the lock
	 * in the master's handle, that will accomplish this.  However,
	 * before we register this event, we'd better remove any
	 * events that we've already registered for the master.
	 */
	if (!F_ISSET(dbp, DB_AM_RECOVER) && IS_REAL_TXN(txn)) {
		/* Unregister old master events. */
		 __txn_remlock(env,
		    txn, &mdbp->handle_lock, DB_LOCK_INVALIDID);

		/* Now register the new event. */
		if ((t_ret = __txn_lockevent(env, txn, dbp,
		    &mdbp->handle_lock, dbp->locker == NULL ?
		    mdbp->locker : dbp->locker)) != 0 && ret == 0)
			ret = t_ret;
	}
	LOCK_INIT(mdbp->handle_lock);

	/*
	 * If the master was created, we need to sync so that the metadata
	 * page is correct on disk for recovery, since it isn't read through
	 * mpool.  If we're opening a subdb in an existing file, we can skip
	 * the sync.
	 */
	if ((t_ret = __db_close(mdbp, txn,
	    F_ISSET(dbp, DB_AM_CREATED_MSTR) ? 0 : DB_NOSYNC)) != 0 && ret == 0)
		ret = t_ret;

	return (ret);
}

/*
 * __fop_remove_setup --
 *	Open handle appropriately and lock for removal of a database file.
 *
 * PUBLIC: int __fop_remove_setup __P((DB *,
 * PUBLIC:      DB_TXN *, const char *, u_int32_t));
 */
int
__fop_remove_setup(dbp, txn, name, flags)
	DB *dbp;
	DB_TXN *txn;
	const char *name;
	u_int32_t flags;
{
	DB_FH *fhp;
	DB_LOCK elock;
	ENV *env;
	u_int8_t mbuf[DBMETASIZE];
	int ret;

	COMPQUIET(flags, 0);

	env = dbp->env;

	LOCK_INIT(elock);
	fhp = NULL;
	ret = 0;

	/* Create locker if necessary. */
retry:	if (LOCKING_ON(env)) {
		if (txn != NULL)
			dbp->locker = txn->locker;
		else if (dbp->locker == DB_LOCK_INVALIDID) {
			if ((ret = __lock_id(env, NULL, &dbp->locker)) != 0)
				goto err;
		}
	}

	/*
	 * We are about to open a file handle and then possibly close it.
	 * We cannot close handles if we are doing FCNTL locking.  However,
	 * there is no way to pass the FCNTL flag into this routine via the
	 * user API.  The only way we can get in here and be doing FCNTL
	 * locking is if we are trying to clean up an open that was called
	 * with FCNTL locking.  In that case, the save_fhp should already be
	 * set.  So, we use that field to tell us if we need to make sure
	 * that we shouldn't close the handle.
	 */
	fhp = dbp->saved_open_fhp;
	DB_ASSERT(env, LF_ISSET(DB_FCNTL_LOCKING) || fhp == NULL);

	/*
	 * Lock environment to protect file open.  That will enable us to
	 * read the meta-data page and get the fileid so that we can lock
	 * the handle.
	 */
	GET_ENVLOCK(env, dbp->locker, &elock);

	/* Open database. */
	if (F_ISSET(dbp, DB_AM_INMEM)) {
		if ((ret = __env_mpool(dbp, name, flags)) == 0)
			ret = __os_strdup(env, name, &dbp->dname);
	} else if (fhp == NULL)
		ret = __os_open(env, name, 0, DB_OSO_RDONLY, 0, &fhp);
	if (ret != 0)
		goto err;

	/* Get meta-data */
	if (F_ISSET(dbp, DB_AM_INMEM))
		ret = __fop_inmem_read_meta(dbp, txn, name, flags);
	else if ((ret = __fop_read_meta(env,
	    name, mbuf, sizeof(mbuf), fhp, 0, NULL)) == 0)
		ret = __db_meta_setup(env, dbp,
		    name, (DBMETA *)mbuf, flags, DB_CHK_META | DB_CHK_NOLSN);
	if (ret != 0)
		goto err;

	/*
	 * Now, get the handle lock.  We first try with NOWAIT, because if
	 * we have to wait, we're going to have to close the file and reopen
	 * it, so that if there is someone else removing it, our open doesn't
	 * prevent that.
	 */
	if ((ret = __fop_lock_handle(env,
	    dbp, dbp->locker, DB_LOCK_WRITE, NULL, DB_LOCK_NOWAIT)) != 0) {
		/*
		 * Close the file, block on the lock, clean up the dbp, and
		 * then start all over again.
		 */
		if (!F_ISSET(dbp, DB_AM_INMEM) && !LF_ISSET(DB_FCNTL_LOCKING)) {
			(void)__os_closehandle(env, fhp);
			fhp = NULL;
		}
		if (ret != DB_LOCK_NOTGRANTED ||
		    (txn != NULL && F_ISSET(txn, TXN_NOWAIT)))
			goto err;
		else if ((ret = __fop_lock_handle(env,
		    dbp, dbp->locker, DB_LOCK_WRITE, &elock, 0)) != 0)
			goto err;

		if (F_ISSET(dbp, DB_AM_INMEM)) {
			(void)__lock_put(env, &dbp->handle_lock);
			(void)__db_refresh(dbp, txn, DB_NOSYNC, NULL, 1);
		} else {
			if (txn != NULL)
				dbp->locker = NULL;
			(void)__db_refresh(dbp, txn, DB_NOSYNC, NULL, 0);
		}
		goto retry;
	} else if ((ret = __ENV_LPUT(env, elock)) != 0)
		goto err;
	else if (F_ISSET(dbp, DB_AM_IN_RENAME))
		ret = ENOENT;

	if (0) {
err:		(void)__ENV_LPUT(env, elock);
	}
	if (fhp != NULL && !LF_ISSET(DB_FCNTL_LOCKING))
		(void)__os_closehandle(env, fhp);
	/*
	 * If this is a real file and we are going to proceed with the removal,
	 * then we need to make sure that we don't leave any pages around in the
	 * mpool since the file is closed and will be reopened again before
	 * access.  However, this might be an in-memory file, in which case
	 * we will handle the discard from the mpool later as it's the "real"
	 * removal of the database.
	 */
	if (ret == 0 && !F_ISSET(dbp, DB_AM_INMEM))
		F_SET(dbp, DB_AM_DISCARD);
	return (ret);
}

/*
 * __fop_read_meta --
 *	Read the meta-data page from a file and return it in buf.
 *
 * PUBLIC: int __fop_read_meta __P((ENV *, const char *,
 * PUBLIC:     u_int8_t *, size_t, DB_FH *, int, size_t *));
 */
int
__fop_read_meta(env, name, buf, size, fhp, errok, nbytesp)
	ENV *env;
	const char *name;
	u_int8_t *buf;
	size_t size;
	DB_FH *fhp;
	int errok;
	size_t *nbytesp;
{
	size_t nr;
	int ret;

	/*
	 * Our caller wants to know the number of bytes read, even if we
	 * return an error.
	 */
	if (nbytesp != NULL)
		*nbytesp = 0;

	nr = 0;
	ret = __os_read(env, fhp, buf, size, &nr);
	if (nbytesp != NULL)
		*nbytesp = nr;

	if (ret != 0) {
		if (!errok)
			__db_err(env, ret, "%s", name);
		goto err;
	}

	if (nr != size) {
		if (!errok)
			__db_errx(env,
			    "fop_read_meta: %s: unexpected file type or format",
			    name);
		ret = EINVAL;
	}

err:
	return (ret);
}

/*
 * __fop_dummy --
 *	This implements the creation and name swapping of dummy files that
 * we use for remove and rename (remove is simply a rename with a delayed
 * remove).
 *
 * PUBLIC: int __fop_dummy __P((DB *,
 * PUBLIC:     DB_TXN *, const char *, const char *));
 */
int
__fop_dummy(dbp, txn, old, new)
	DB *dbp;
	DB_TXN *txn;
	const char *old, *new;
{
	DB *tmpdbp;
	DB_TXN *stxn;
	ENV *env;
	char *back;
	int ret, t_ret;
	u_int8_t mbuf[DBMETASIZE];

	env = dbp->env;
	back = NULL;
	stxn = NULL;
	tmpdbp = NULL;

	DB_ASSERT(env, txn != NULL);

	/*
	 * Begin sub transaction to encapsulate the rename.  Note that we
	 * expect the inmem_swap calls to complete the sub-transaction,
	 * aborting on error and committing on success.
	 */
	if (TXN_ON(env) &&
	    (ret = __txn_begin(env, NULL, txn, &stxn, 0)) != 0)
		goto err;

	/* We need to create a dummy file as a place holder. */
	if ((ret = __db_backup_name(env, new, stxn, &back)) != 0)
		goto err;
	/* Create a dummy dbp handle. */
	if ((ret = __db_create_internal(&tmpdbp, env, 0)) != 0)
		goto err;
	if (F_ISSET(dbp, DB_AM_NOT_DURABLE) &&
		(ret = __db_set_flags(tmpdbp, DB_TXN_NOT_DURABLE)) != 0)
		goto err;
	memset(mbuf, 0, sizeof(mbuf));
	ret = F_ISSET(dbp, DB_AM_INMEM) ?
	    __fop_inmem_dummy(tmpdbp, stxn, back, mbuf) :
	    __fop_ondisk_dummy(tmpdbp, stxn, back, mbuf);

	if (ret != 0)
		goto err;

	ret = F_ISSET(dbp, DB_AM_INMEM) ?
	    __fop_inmem_swap(dbp, tmpdbp, stxn, old, new, back, txn->locker) :
	    __fop_ondisk_swap(dbp, tmpdbp, stxn, old, new, back, txn->locker);
	stxn = NULL;
	if (ret != 0)
		goto err;

err:	if (stxn != NULL)
		(void)__txn_abort(stxn);
	if (tmpdbp != NULL &&
	    (t_ret = __db_close(tmpdbp, NULL, 0)) != 0 && ret == 0)
		ret = t_ret;
	if (back != NULL)
		__os_free(env, back);
	return (ret);
}

/*
 * __fop_dbrename --
 *	Do the appropriate file locking and file system operations
 * to effect a dbrename in the absence of transactions (__fop_dummy
 * and the subsequent calls in __db_rename do the work for the
 * transactional case).
 *
 * PUBLIC: int __fop_dbrename __P((DB *, const char *, const char *));
 */
int
__fop_dbrename(dbp, old, new)
	DB *dbp;
	const char *old, *new;
{
	DB_LOCK elock;
	ENV *env;
	char *real_new, *real_old;
	int ret, t_ret;

	env = dbp->env;
	real_new = NULL;
	real_old = NULL;
	LOCK_INIT(elock);

	if (F_ISSET(dbp, DB_AM_INMEM)) {
		real_new = (char *)new;
		real_old = (char *)old;
	} else {
		/* Get full names. */
		if ((ret = __db_appname(env,
		    DB_APP_DATA, old, &dbp->dirname, &real_old)) != 0)
			goto err;

		if ((ret = __db_appname(env,
		    DB_APP_DATA, new, &dbp->dirname, &real_new)) != 0)
			goto err;
	}

	/*
	 * It is an error to rename a file over one that already exists,
	 * as that wouldn't be transaction-safe.  We check explicitly
	 * for ondisk files, but it's done memp_nameop for in-memory ones.
	 */
	GET_ENVLOCK(env, dbp->locker, &elock);
	ret = F_ISSET(dbp, DB_AM_INMEM) ? ENOENT :
	    __os_exists(env, real_new, NULL);

	if (ret == 0) {
		ret = EEXIST;
		__db_errx(env, "rename: file %s exists", real_new);
		goto err;
	}

	ret = __memp_nameop(env,
	    dbp->fileid, new, real_old, real_new, F_ISSET(dbp, DB_AM_INMEM));

err:	if ((t_ret = __ENV_LPUT(env, elock)) != 0 && ret == 0)
		ret = t_ret;
	if (!F_ISSET(dbp, DB_AM_INMEM) && real_old != NULL)
		__os_free(env, real_old);
	if (!F_ISSET(dbp, DB_AM_INMEM) && real_new != NULL)
		__os_free(env, real_new);
	return (ret);
}

static int
__fop_inmem_create(dbp, name, txn, flags)
	DB *dbp;
	const char *name;
	DB_TXN *txn;
	u_int32_t flags;
{
	DBT fid_dbt, name_dbt;
	DB_LSN lsn;
	ENV *env;
	int ret;
	int32_t lfid;
	u_int32_t *p32;

	env = dbp->env;

	MAKE_INMEM(dbp);

	/* Set the pagesize if it isn't yet set. */
	if (dbp->pgsize == 0)
		dbp->pgsize = DB_DEF_IOSIZE;

	/*
	 * Construct a file_id.
	 *
	 * If this file has no name, then we only need a fileid for locking.
	 * If this file has a name, we need the fileid both for locking and
	 * matching in the memory pool.  So, with unnamed in-memory databases,
	 * use a lock_id.  For named in-memory files, we need to find a value
	 * that we can use to uniquely identify a name/fid pair.  We use a
	 * combination of a unique id (__os_unique_id) and a hash of the
	 * original name.
	 */
	if (name == NULL) {
		if (LOCKING_ON(env) && (ret =
		    __lock_id(env, (u_int32_t *)dbp->fileid, NULL)) != 0)
			goto err;
	}  else {
		p32 = (u_int32_t *)(&dbp->fileid[0]);
		__os_unique_id(env, p32);
		p32++;
		(void)strncpy(
		    (char *)p32, name, DB_FILE_ID_LEN - sizeof(u_int32_t));
		dbp->preserve_fid = 1;

		if (DBENV_LOGGING(env) &&
#if !defined(DEBUG_WOP) && !defined(DIAGNOSTIC)
		    txn != NULL &&
#endif
		    dbp->log_filename != NULL)
			memcpy(dbp->log_filename->ufid,
			    dbp->fileid, DB_FILE_ID_LEN);
	}

	/* Now, set the fileid. */
	if ((ret = __memp_set_fileid(dbp->mpf, dbp->fileid)) != 0)
		goto err;

	if ((ret = __env_mpool(dbp, name, flags)) != 0)
		goto err;

	if (DBENV_LOGGING(env) &&
#if !defined(DEBUG_WOP)
	    txn != NULL &&
#endif
	    name != NULL) {
		DB_INIT_DBT(name_dbt, name, strlen(name) + 1);
		memset(&fid_dbt, 0, sizeof(fid_dbt));
		fid_dbt.data = dbp->fileid;
		fid_dbt.size = DB_FILE_ID_LEN;
		lfid = dbp->log_filename == NULL ?
		    DB_LOGFILEID_INVALID : dbp->log_filename->id;
		if ((ret = __crdel_inmem_create_log(env, txn,
		    &lsn, 0, lfid, &name_dbt, &fid_dbt, dbp->pgsize)) != 0)
			goto err;
	}

	F_SET(dbp, DB_AM_CREATED);

err:
	return (ret);
}

static int
__fop_inmem_read_meta(dbp, txn, name, flags)
	DB *dbp;
	DB_TXN *txn;
	const char *name;
	u_int32_t flags;
{
	DBMETA *metap;
	DB_THREAD_INFO *ip;
	db_pgno_t pgno;
	int ret, t_ret;

	if (txn == NULL)
		ENV_GET_THREAD_INFO(dbp->env, ip);
	else
		ip = txn->thread_info;

	pgno  = PGNO_BASE_MD;
	if ((ret = __memp_fget(dbp->mpf, &pgno, ip, txn, 0, &metap)) != 0)
		return (ret);
	ret = __db_meta_setup(dbp->env, dbp, name, metap, flags, DB_CHK_META);

	if ((t_ret =
	    __memp_fput(dbp->mpf, ip, metap, dbp->priority)) && ret == 0)
		ret = t_ret;

	return (ret);
}

static int
__fop_ondisk_dummy(dbp, txn, name, mbuf)
	DB *dbp;
	DB_TXN *txn;
	const char *name;
	u_int8_t *mbuf;
{
	ENV *env;
	int ret;
	char *realname;
	u_int32_t dflags;

	realname = NULL;
	env = dbp->env;
	dflags = F_ISSET(dbp, DB_AM_NOT_DURABLE) ? DB_LOG_NOT_DURABLE : 0;

	if ((ret = __db_appname(env,
	    DB_APP_DATA, name, &dbp->dirname, &realname)) != 0)
		goto err;

	if ((ret = __fop_create(env,
	    txn, NULL, name, &dbp->dirname, DB_APP_DATA, 0, dflags)) != 0)
		goto err;

	if ((ret =
	    __os_fileid(env, realname, 1, ((DBMETA *)mbuf)->uid)) != 0)
		goto err;

	((DBMETA *)mbuf)->magic = DB_RENAMEMAGIC;
	if ((ret = __fop_write(env, txn, name, dbp->dirname,
	    DB_APP_DATA, NULL, 0, 0, 0, mbuf, DBMETASIZE, 1, dflags)) != 0)
		goto err;

	memcpy(dbp->fileid, ((DBMETA *)mbuf)->uid, DB_FILE_ID_LEN);

err:	if (realname != NULL)
		__os_free(env, realname);

	return (ret);
}

static int
__fop_inmem_dummy(dbp, txn, name, mbuf)
	DB *dbp;
	DB_TXN *txn;
	const char *name;
	u_int8_t *mbuf;
{
	DBMETA *metap;
	DB_THREAD_INFO *ip;
	db_pgno_t pgno;
	int ret, t_ret;

	if ((ret = __fop_inmem_create(dbp, name, txn, DB_CREATE)) != 0)
		return (ret);
	if (txn == NULL)
		ENV_GET_THREAD_INFO(dbp->env, ip);
	else
		ip = txn->thread_info;

	pgno  = PGNO_BASE_MD;
	if ((ret = __memp_fget(dbp->mpf, &pgno, ip, txn,
	    DB_MPOOL_CREATE | DB_MPOOL_DIRTY, &metap)) != 0)
		return (ret);
	/* Check file existed. */
	if (metap->magic != 0)
		ret = EEXIST;
	else
		metap->magic = DB_RENAMEMAGIC;

	/* Copy the fileid onto the meta-data page. */
	memcpy(metap->uid, dbp->fileid, DB_FILE_ID_LEN);

	if ((t_ret = __memp_fput(dbp->mpf, ip, metap,
	    ret == 0 ? dbp->priority : DB_PRIORITY_VERY_LOW)) != 0 && ret == 0)
		ret = t_ret;

	if (ret != 0)
		goto err;

	((DBMETA *)mbuf)->magic = DB_RENAMEMAGIC;

err:	return (ret);
}

static int
__fop_ondisk_swap(dbp, tmpdbp, txn, old, new, back, locker)
	DB *dbp, *tmpdbp;
	DB_TXN *txn;
	const char *old, *new, *back;
	DB_LOCKER *locker;
{
	DBT fiddbt, namedbt, tmpdbt;
	DB_FH *fhp;
	DB_LOCK elock;
	DB_LSN lsn;
	DB_TXN *parent;
	ENV *env;
	u_int8_t mbuf[DBMETASIZE];
	u_int32_t child_txnid, dflags;
	int ret, t_ret;
	char *realold, *realnew;

	env = dbp->env;
	DB_ASSERT(env, txn != NULL);
	DB_ASSERT(env, old != NULL);

	realold = realnew = NULL;
	LOCK_INIT(elock);
	fhp = NULL;
	dflags = F_ISSET(dbp, DB_AM_NOT_DURABLE) ? DB_LOG_NOT_DURABLE : 0;

	if ((ret = __db_appname(env,
	    DB_APP_DATA, new, &dbp->dirname, &realnew)) != 0)
		goto err;

	/* Now, lock the name space while we initialize this file. */
retry:	GET_ENVLOCK(env, locker, &elock);
	if (__os_exists(env, realnew, NULL) == 0) {
		/*
		 * It is possible that the only reason this file exists is
		 * because we've done a previous rename of it and we have
		 * left a placeholder here.  We need to check for that case
		 * and allow this rename to succeed if that's the case.
		 */
		if ((ret = __os_open(env, realnew, 0, 0, 0, &fhp)) != 0)
			goto err;
		if ((ret = __fop_read_meta(env,
		    realnew, mbuf, sizeof(mbuf), fhp, 0, NULL)) != 0 ||
		    (ret = __db_meta_setup(env,
		    tmpdbp, realnew, (DBMETA *)mbuf, 0, DB_CHK_META)) != 0) {
			ret = EEXIST;
			goto err;
		}
		ret = __os_closehandle(env, fhp);
		fhp = NULL;
		if (ret != 0)
			goto err;

		/*
		 * Now, try to acquire the handle lock.  If the handle is locked
		 * by our current, transaction, then we'll get it and life is
		 * good.
		 *
		 * Alternately, it's not locked at all, we'll get the lock, but
		 * we will realize it exists and consider this an error.
		 *
		 * However, if it's held by another transaction, then there
		 * could be two different scenarios: 1) the file is in the
		 * midst of being created or deleted and when that transaction
		 * is over, we might be able to proceed. 2) the file is open
		 * and exists and we should report an error. In order to
		 * distinguish these two cases, we do the following. First, we
		 * try to acquire a READLOCK.  If the handle is in the midst of
		 * being created, then we'll block because a writelock is held.
		 * In that case, we should request a blocking write, and when we
		 * get the lock, we should then go back and check to see if the
		 * object exists and start all over again.
		 *
		 * If we got the READLOCK, then either no one is holding the
		 * lock or someone has an open handle and the fact that the file
		 * exists is problematic.  So, in this case, we request the
		 * WRITELOCK non-blocking -- if it succeeds, we're golden.  If
		 * it fails, then the file exists and we return EEXIST.
		 */
		if ((ret = __fop_lock_handle(env,
		    tmpdbp, locker, DB_LOCK_READ, NULL, DB_LOCK_NOWAIT)) != 0) {
			/*
			 * Someone holds a write-lock.  Wait for the write-lock
			 * and after we get it, release it and start over.
			 */
			if ((ret = __fop_lock_handle(env, tmpdbp,
			    locker, DB_LOCK_WRITE, &elock, 0)) != 0)
				goto err;
			if ((ret =
			    __lock_put(env, &tmpdbp->handle_lock)) != 0)
				goto err;
			if ((ret = __db_refresh(tmpdbp, NULL, 0, NULL, 0)) != 0)
				goto err;
			goto retry;
		}

		/* We got the read lock; try to upgrade it. */
		ret = __fop_lock_handle(env,
		    tmpdbp, locker, DB_LOCK_WRITE,
		    NULL, DB_LOCK_UPGRADE | DB_LOCK_NOWAIT);
		if (ret != 0) {
			/*
			 * We did not get the writelock, so someone
			 * has the handle open.  This is an error.
			 */
			(void)__lock_put(env, &tmpdbp->handle_lock);
			ret = EEXIST;
		} else  if (F_ISSET(tmpdbp, DB_AM_IN_RENAME))
			/* We got the lock and are renaming it. */
			ret = 0;
		else { /* We got the lock, but the file exists. */
			(void)__lock_put(env, &tmpdbp->handle_lock);
			ret = EEXIST;
		}
		if (ret != 0)
			goto err;
	}

	/*
	 * While we have the namespace locked, do the renames and then
	 * swap for the handle lock.
	 */
	if ((ret = __fop_rename(env, txn,
	    old, new, &dbp->dirname, dbp->fileid, DB_APP_DATA, 1, dflags)) != 0)
		goto err;
	if ((ret = __fop_rename(env, txn, back, old,
	    &dbp->dirname, tmpdbp->fileid, DB_APP_DATA, 0, dflags)) != 0)
		goto err;
	if ((ret = __fop_lock_handle(env,
	    tmpdbp, locker, DB_LOCK_WRITE, &elock, NOWAIT_FLAG(txn))) != 0)
		goto err;

	/*
	 * We just acquired a transactional lock on the tmp handle.
	 * We need to null out the tmp handle's lock so that it
	 * doesn't create problems for us in the close path.
	 */
	LOCK_INIT(tmpdbp->handle_lock);

	/* Commit the child. */
	child_txnid = txn->txnid;
	parent = txn->parent;
	ret = __txn_commit(txn, 0);
	txn = NULL;

	/*
	 * If the new name is available because it was previously renamed
	 * remove it from the remove list.
	 */
	if (F_ISSET(tmpdbp, DB_AM_IN_RENAME))
		__txn_remrem(env, parent, realnew);

	/* Now log the child information in the parent. */
	memset(&fiddbt, 0, sizeof(fiddbt));
	fiddbt.data = dbp->fileid;
	fiddbt.size = DB_FILE_ID_LEN;
	memset(&tmpdbt, 0, sizeof(fiddbt));
	tmpdbt.data = tmpdbp->fileid;
	tmpdbt.size = DB_FILE_ID_LEN;
	DB_INIT_DBT(namedbt, old, strlen(old) + 1);
	if ((t_ret = __fop_file_remove_log(env,
	    parent, &lsn, dflags, &fiddbt, &tmpdbt, &namedbt,
	    (u_int32_t)DB_APP_DATA, child_txnid)) != 0 && ret == 0)
		ret = t_ret;

	/* This is a delayed delete of the dummy file. */
	if ((ret = __db_appname(env,
	    DB_APP_DATA, old, &dbp->dirname, &realold)) != 0)
		goto err;

	if ((ret = __txn_remevent(env, parent, realold, NULL, 0)) != 0)
		goto err;

err:	if (txn != NULL)	/* Ret must already be set, so void abort. */
		(void)__txn_abort(txn);

	(void)__ENV_LPUT(env, elock);

	if (fhp != NULL &&
	    (t_ret = __os_closehandle(env, fhp)) != 0 && ret == 0)
		ret = t_ret;

	if (realnew != NULL)
		__os_free(env, realnew);
	if (realold != NULL)
		__os_free(env, realold);
	return (ret);
}

static int
__fop_inmem_swap(olddbp, backdbp, txn, old, new, back, locker)
	DB *olddbp, *backdbp;
	DB_TXN *txn;
	const char *old, *new, *back;
	DB_LOCKER *locker;
{
	DB *tmpdbp;
	DBT fid_dbt, n1_dbt, n2_dbt;
	DB_LOCK elock;
	DB_LSN lsn;
	DB_TXN *parent;
	ENV *env;
	int ret, t_ret;

	env = olddbp->env;
	parent = txn->parent;
retry:	LOCK_INIT(elock);
	if ((ret = __db_create_internal(&tmpdbp, env, 0)) != 0)
		return (ret);
	MAKE_INMEM(tmpdbp);

	GET_ENVLOCK(env, locker, &elock);
	if ((ret = __env_mpool(tmpdbp, new, 0)) == 0) {
		/*
		 * It is possible that the only reason this database exists is
		 * because we've done a previous rename of it and we have
		 * left a placeholder here.  We need to check for that case
		 * and allow this rename to succeed if that's the case.
		 */

		if ((ret = __fop_inmem_read_meta(tmpdbp, txn, new, 0)) != 0) {
			ret = EEXIST;
			goto err;
		}

		/*
		 * Now, try to acquire the handle lock.  If it's from our txn,
		 * then we'll get the lock.  If it's not, then someone else has
		 * it locked.  See the comments in __fop_ondisk_swap for
		 * details.
		 */
		if ((ret = __fop_lock_handle(env,
		    tmpdbp, locker, DB_LOCK_READ, NULL, DB_LOCK_NOWAIT)) != 0) {
			/*
			 * Someone holds a writelock.  Try for the WRITELOCK
			 * and after we get it, retry.
			 */
			if ((ret = __fop_lock_handle(env, tmpdbp,
			    locker, DB_LOCK_WRITE, &elock, 0)) != 0)
				goto err;

			/* We have the write lock; release it and start over. */
			(void)__lock_put(env, &tmpdbp->handle_lock);
			(void)__db_close(tmpdbp, NULL, DB_NOSYNC);
			(void)__ENV_LPUT(env, elock);
			goto retry;
		} else {
			(void)__lock_put(env, &tmpdbp->handle_lock);
			if (!F_ISSET(tmpdbp, DB_AM_IN_RENAME))
				ret = EEXIST;
		}
		if (ret != 0)
			goto err;
	}

	/* Log the renames. */
	if (LOGGING_ON(env)
#ifndef DEBUG_WOP
	    && txn != NULL
#endif
	) {
		/* Rename old to new. */
		DB_INIT_DBT(fid_dbt, olddbp->fileid, DB_FILE_ID_LEN);
		DB_INIT_DBT(n1_dbt, old, strlen(old) + 1);
		DB_INIT_DBT(n2_dbt, new, strlen(new) + 1);
		if ((ret = __crdel_inmem_rename_log(
		    env, txn, &lsn, 0, &n1_dbt, &n2_dbt, &fid_dbt)) != 0)
			goto err;

		/* Rename back to old */
		fid_dbt.data = backdbp->fileid;
		DB_SET_DBT(n2_dbt, back, strlen(back) + 1);
		if ((ret = __crdel_inmem_rename_log(
		    env, txn, &lsn, 0, &n2_dbt, &n1_dbt, &fid_dbt)) != 0)
			goto err;
	}

	/*
	 * While we have the namespace locked, do the renames and then
	 * swap for the handle lock.   If we ran into a file in the midst
	 * of rename, then we need to delete it first, else nameop is
	 * going to consider it an error.
	 */
	if (F_ISSET(tmpdbp, DB_AM_IN_RENAME)) {
		if ((ret = __memp_nameop(env,
		    tmpdbp->fileid, NULL, new, NULL, 1)) != 0)
			goto err;
		__txn_remrem(env, parent, new);
	}

	if ((ret = __memp_nameop(
	    env, olddbp->fileid, new, old, new, 1)) != 0)
		goto err;
	if ((ret = __memp_nameop(
	    env, backdbp->fileid, old, back, old, 1)) != 0)
		goto err;

	if ((ret = __fop_lock_handle(env,
	    tmpdbp, locker, DB_LOCK_WRITE, &elock, 0)) != 0)
		goto err;

	/*
	 * We just acquired a transactional lock on the tmp handle.
	 * We need to null out the tmp handle's lock so that it
	 * doesn't create problems for us in the close path.
	 */
	LOCK_INIT(tmpdbp->handle_lock);

	DB_ASSERT(env, txn != NULL);

	/* Commit the child. */
	ret = __txn_commit(txn, 0);
	txn = NULL;

	if ((ret = __db_inmem_remove(backdbp, parent, old)) != 0)
		goto err;

err:	(void)__ENV_LPUT(env, elock);

	if (txn != NULL)
		(void)__txn_abort(txn);

	if ((t_ret = __db_close(tmpdbp, NULL, 0)) != 0 && ret == 0)
		ret = t_ret;

	return (ret);
}
