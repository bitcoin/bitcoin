/*-
 * See the file LICENSE for redistribution information.
 *
 * Copyright (c) 1998-2009 Oracle.  All rights reserved.
 *
 * $Id$
 */

#include "db_config.h"

#include "db_int.h"
#include "dbinc/db_page.h"
#include "dbinc/btree.h"
#include "dbinc/hash.h"
#include "dbinc/lock.h"
#include "dbinc/log.h"
#include "dbinc/mp.h"
#include "dbinc/partition.h"
#include "dbinc/qam.h"
#include "dbinc/txn.h"

static int __db_secondary_get __P((DB *, DB_TXN *, DBT *, DBT *, u_int32_t));
static int __dbc_set_priority __P((DBC *, DB_CACHE_PRIORITY));
static int __dbc_get_priority __P((DBC *, DB_CACHE_PRIORITY* ));

/*
 * __db_cursor_int --
 *	Internal routine to create a cursor.
 *
 * PUBLIC: int __db_cursor_int __P((DB *, DB_THREAD_INFO *,
 * PUBLIC:     DB_TXN *, DBTYPE, db_pgno_t, int, DB_LOCKER *, DBC **));
 */
int
__db_cursor_int(dbp, ip, txn, dbtype, root, flags, locker, dbcp)
	DB *dbp;
	DB_THREAD_INFO *ip;
	DB_TXN *txn;
	DBTYPE dbtype;
	db_pgno_t root;
	int flags;
	DB_LOCKER *locker;
	DBC **dbcp;
{
	DBC *dbc;
	DBC_INTERNAL *cp;
	ENV *env;
	db_threadid_t tid;
	int allocated, ret;
	pid_t pid;

	env = dbp->env;
	allocated = 0;

	/*
	 * If dbcp is non-NULL it is assumed to point to an area to initialize
	 * as a cursor.
	 *
	 * Take one from the free list if it's available.  Take only the
	 * right type.  With off page dups we may have different kinds
	 * of cursors on the queue for a single database.
	 */
	MUTEX_LOCK(env, dbp->mutex);

#ifndef HAVE_NO_DB_REFCOUNT
	/*
	 * If this DBP is being logged then refcount the log filename
	 * relative to this transaction. We do this here because we have
	 * the dbp->mutex which protects the refcount.  We want to avoid
	 * calling the function if we are duplicating a cursor.  This includes
	 * the case of creating an off page duplicate cursor. If we know this
	 * cursor will not be used in an update, we could avoid this,
	 * but we don't have that information.
	 */
	if (txn != NULL && !LF_ISSET(DBC_OPD | DBC_DUPLICATE)
	    && !F_ISSET(dbp, DB_AM_RECOVER) &&
	    dbp->log_filename != NULL && !IS_REP_CLIENT(env) &&
	    (ret = __txn_record_fname(env, txn, dbp->log_filename)) != 0) {
		MUTEX_UNLOCK(env, dbp->mutex);
		return (ret);
	}

#endif

	TAILQ_FOREACH(dbc, &dbp->free_queue, links)
		if (dbtype == dbc->dbtype) {
			TAILQ_REMOVE(&dbp->free_queue, dbc, links);
			F_CLR(dbc, ~DBC_OWN_LID);
			break;
		}
	MUTEX_UNLOCK(env, dbp->mutex);

	if (dbc == NULL) {
		if ((ret = __os_calloc(env, 1, sizeof(DBC), &dbc)) != 0)
			return (ret);
		allocated = 1;
		dbc->flags = 0;

		dbc->dbp = dbp;
		dbc->dbenv = dbp->dbenv;
		dbc->env = dbp->env;

		/* Set up locking information. */
		if (LOCKING_ON(env)) {
			/*
			 * If we are not threaded, we share a locker ID among
			 * all cursors opened in the environment handle,
			 * allocating one if this is the first cursor.
			 *
			 * This relies on the fact that non-threaded DB handles
			 * always have non-threaded environment handles, since
			 * we set DB_THREAD on DB handles created with threaded
			 * environment handles.
			 */
			if (!DB_IS_THREADED(dbp)) {
				if (env->env_lref == NULL && (ret =
				    __lock_id(env, NULL, &env->env_lref)) != 0)
					goto err;
				dbc->lref = env->env_lref;
			} else {
				if ((ret =
				    __lock_id(env, NULL, &dbc->lref)) != 0)
					goto err;
				F_SET(dbc, DBC_OWN_LID);
			}

			/*
			 * In CDB, secondary indices should share a lock file
			 * ID with the primary;  otherwise we're susceptible
			 * to deadlocks.  We also use __db_cursor_int rather
			 * than __db_cursor to create secondary update cursors
			 * in c_put and c_del; these won't acquire a new lock.
			 *
			 * !!!
			 * Since this is in the one-time cursor allocation
			 * code, we need to be sure to destroy, not just
			 * close, all cursors in the secondary when we
			 * associate.
			 */
			if (CDB_LOCKING(env) &&
			    F_ISSET(dbp, DB_AM_SECONDARY))
				memcpy(dbc->lock.fileid,
				    dbp->s_primary->fileid, DB_FILE_ID_LEN);
			else
				memcpy(dbc->lock.fileid,
				    dbp->fileid, DB_FILE_ID_LEN);

			if (CDB_LOCKING(env)) {
				if (F_ISSET(env->dbenv, DB_ENV_CDB_ALLDB)) {
					/*
					 * If we are doing a single lock per
					 * environment, set up the global
					 * lock object just like we do to
					 * single thread creates.
					 */
					DB_ASSERT(env, sizeof(db_pgno_t) ==
					    sizeof(u_int32_t));
					dbc->lock_dbt.size = sizeof(u_int32_t);
					dbc->lock_dbt.data = &dbc->lock.pgno;
					dbc->lock.pgno = 0;
				} else {
					dbc->lock_dbt.size = DB_FILE_ID_LEN;
					dbc->lock_dbt.data = dbc->lock.fileid;
				}
			} else {
				dbc->lock.type = DB_PAGE_LOCK;
				dbc->lock_dbt.size = sizeof(dbc->lock);
				dbc->lock_dbt.data = &dbc->lock;
			}
		}
		/* Init the DBC internal structure. */
#ifdef HAVE_PARTITION
		if (DB_IS_PARTITIONED(dbp)) {
			if ((ret = __partc_init(dbc)) != 0)
				goto err;
		} else
#endif
		switch (dbtype) {
		case DB_BTREE:
		case DB_RECNO:
			if ((ret = __bamc_init(dbc, dbtype)) != 0)
				goto err;
			break;
		case DB_HASH:
			if ((ret = __hamc_init(dbc)) != 0)
				goto err;
			break;
		case DB_QUEUE:
			if ((ret = __qamc_init(dbc)) != 0)
				goto err;
			break;
		case DB_UNKNOWN:
		default:
			ret = __db_unknown_type(env, "DB->cursor", dbtype);
			goto err;
		}

		cp = dbc->internal;
	}

	/* Refresh the DBC structure. */
	dbc->dbtype = dbtype;
	RESET_RET_MEM(dbc);
	dbc->set_priority = __dbc_set_priority;
	dbc->get_priority = __dbc_get_priority;
	dbc->priority = dbp->priority;

	if ((dbc->txn = txn) != NULL)
		dbc->locker = txn->locker;
	else if (LOCKING_ON(env)) {
		/*
		 * There are certain cases in which we want to create a
		 * new cursor with a particular locker ID that is known
		 * to be the same as (and thus not conflict with) an
		 * open cursor.
		 *
		 * The most obvious case is cursor duplication;  when we
		 * call DBC->dup or __dbc_idup, we want to use the original
		 * cursor's locker ID.
		 *
		 * Another case is when updating secondary indices.  Standard
		 * CDB locking would mean that we might block ourself:  we need
		 * to open an update cursor in the secondary while an update
		 * cursor in the primary is open, and when the secondary and
		 * primary are subdatabases or we're using env-wide locking,
		 * this is disastrous.
		 *
		 * In these cases, our caller will pass a nonzero locker
		 * ID into this function.  Use this locker ID instead of
		 * the default as the locker ID for our new cursor.
		 */
		if (locker != NULL)
			dbc->locker = locker;
		else {
			/*
			 * If we are threaded then we need to set the
			 * proper thread id into the locker.
			 */
			if (DB_IS_THREADED(dbp)) {
				env->dbenv->thread_id(env->dbenv, &pid, &tid);
				__lock_set_thread_id(dbc->lref, pid, tid);
			}
			dbc->locker = dbc->lref;
		}
	}

	/*
	 * These fields change when we are used as a secondary index, so
	 * if the DB is a secondary, make sure they're set properly just
	 * in case we opened some cursors before we were associated.
	 *
	 * __dbc_get is used by all access methods, so this should be safe.
	 */
	if (F_ISSET(dbp, DB_AM_SECONDARY))
		dbc->get = dbc->c_get = __dbc_secondary_get_pp;

	if (LF_ISSET(DB_CURSOR_BULK) && dbtype == DB_BTREE)
		F_SET(dbc, DBC_BULK);
	if (LF_ISSET(DB_CURSOR_TRANSIENT))
		F_SET(dbc, DBC_TRANSIENT);
	if (LF_ISSET(DBC_OPD))
		F_SET(dbc, DBC_OPD);
	if (F_ISSET(dbp, DB_AM_RECOVER))
		F_SET(dbc, DBC_RECOVER);
	if (F_ISSET(dbp, DB_AM_COMPENSATE))
		F_SET(dbc, DBC_DONTLOCK);
#ifdef HAVE_REPLICATION
	/*
	 * If we are replicating from a down rev version then we must
	 * use old locking protocols.
	 */
	if (LOGGING_ON(env) &&
	     ((LOG *)env->lg_handle->
	     reginfo.primary)->persist.version < DB_LOGVERSION_LATCHING)
		F_SET(dbc, DBC_DOWNREV);
#endif

	/* Refresh the DBC internal structure. */
	cp = dbc->internal;
	cp->opd = NULL;
	cp->pdbc = NULL;

	cp->indx = 0;
	cp->page = NULL;
	cp->pgno = PGNO_INVALID;
	cp->root = root;
	cp->stream_start_pgno = cp->stream_curr_pgno = PGNO_INVALID;
	cp->stream_off = 0;

	if (DB_IS_PARTITIONED(dbp)) {
		DBC_PART_REFRESH(dbc);
	} else switch (dbtype) {
	case DB_BTREE:
	case DB_RECNO:
		if ((ret = __bamc_refresh(dbc)) != 0)
			goto err;
		break;
	case DB_HASH:
	case DB_QUEUE:
		break;
	case DB_UNKNOWN:
	default:
		ret = __db_unknown_type(env, "DB->cursor", dbp->type);
		goto err;
	}

	/*
	 * The transaction keeps track of how many cursors were opened within
	 * it to catch application errors where the cursor isn't closed when
	 * the transaction is resolved.
	 */
	if (txn != NULL)
		++txn->cursors;
	if (ip != NULL)
		dbc->thread_info = ip;
	else if (txn != NULL)
		dbc->thread_info = txn->thread_info;
	else
		ENV_GET_THREAD_INFO(env, dbc->thread_info);

	MUTEX_LOCK(env, dbp->mutex);
	TAILQ_INSERT_TAIL(&dbp->active_queue, dbc, links);
	F_SET(dbc, DBC_ACTIVE);
	MUTEX_UNLOCK(env, dbp->mutex);

	*dbcp = dbc;
	return (0);

err:	if (allocated)
		__os_free(env, dbc);
	return (ret);
}

/*
 * __db_put --
 *	Store a key/data pair.
 *
 * PUBLIC: int __db_put __P((DB *,
 * PUBLIC:      DB_THREAD_INFO *, DB_TXN *, DBT *, DBT *, u_int32_t));
 */
int
__db_put(dbp, ip, txn, key, data, flags)
	DB *dbp;
	DB_THREAD_INFO *ip;
	DB_TXN *txn;
	DBT *key, *data;
	u_int32_t flags;
{
	DBC *dbc;
	DBT tdata, tkey;
	ENV *env;
	void *bulk_kptr, *bulk_ptr;
	db_recno_t recno;
	u_int32_t cursor_flags;
	int ret, t_ret;

	env = dbp->env;

	/*
	 * See the comment in __db_get() regarding DB_CURSOR_TRANSIENT.
	 *
	 * Note that the get in the DB_NOOVERWRITE case is safe to do with this
	 * flag set;  if it errors in any way other than DB_NOTFOUND, we're
	 * going to close the cursor without doing anything else, and if it
	 * returns DB_NOTFOUND then it's safe to do a c_put(DB_KEYLAST) even if
	 * an access method moved the cursor, since that's not
	 * position-dependent.
	 */
	cursor_flags = DB_WRITELOCK;
	if (LF_ISSET(DB_MULTIPLE | DB_MULTIPLE_KEY))
		cursor_flags |= DB_CURSOR_BULK;
	else
		cursor_flags |= DB_CURSOR_TRANSIENT;
	if ((ret = __db_cursor(dbp, ip, txn, &dbc, cursor_flags)) != 0)
		return (ret);

	DEBUG_LWRITE(dbc, txn, "DB->put", key, data, flags);

	SET_RET_MEM(dbc, dbp);

	if (flags == DB_APPEND && !DB_IS_PRIMARY(dbp)) {
		/*
		 * If there is an append callback, the value stored in
		 * data->data may be replaced and then freed.  To avoid
		 * passing a freed pointer back to the user, just operate
		 * on a copy of the data DBT.
		 */
		tdata = *data;

		/*
		 * Append isn't a normal put operation;  call the appropriate
		 * access method's append function.
		 */
		switch (dbp->type) {
		case DB_QUEUE:
			if ((ret = __qam_append(dbc, key, &tdata)) != 0)
				goto err;
			break;
		case DB_RECNO:
			if ((ret = __ram_append(dbc, key, &tdata)) != 0)
				goto err;
			break;
		case DB_BTREE:
		case DB_HASH:
		case DB_UNKNOWN:
		default:
			/* The interface should prevent this. */
			DB_ASSERT(env,
			    dbp->type == DB_QUEUE || dbp->type == DB_RECNO);

			ret = __db_ferr(env, "DB->put", 0);
			goto err;
		}

		/*
		 * The append callback, if one exists, may have allocated
		 * a new tdata.data buffer.  If so, free it.
		 */
		FREE_IF_NEEDED(env, &tdata);

		/* No need for a cursor put;  we're done. */
#ifdef HAVE_COMPRESSION
	} else if (DB_IS_COMPRESSED(dbp) && !F_ISSET(dbp, DB_AM_SECONDARY) &&
	    !DB_IS_PRIMARY(dbp) && LIST_FIRST(&dbp->f_primaries) == NULL) {
		ret = __dbc_put(dbc, key, data, flags);
#endif
	} else if (LF_ISSET(DB_MULTIPLE)) {
		ret = 0;
		memset(&tkey, 0, sizeof(tkey));
		if (dbp->type == DB_QUEUE || dbp->type == DB_RECNO) {
			tkey.data = &recno;
			tkey.size = sizeof(recno);
		}
		memset(&tdata, 0, sizeof(tdata));
		DB_MULTIPLE_INIT(bulk_kptr, key);
		DB_MULTIPLE_INIT(bulk_ptr, data);
		key->doff = 0;
		while (ret == 0) {
			if (dbp->type == DB_QUEUE || dbp->type == DB_RECNO)
				DB_MULTIPLE_RECNO_NEXT(bulk_kptr, key,
				    recno, tdata.data, tdata.size);
			else
				DB_MULTIPLE_NEXT(bulk_kptr, key,
				    tkey.data, tkey.size);
			DB_MULTIPLE_NEXT(bulk_ptr, data,
			    tdata.data, tdata.size);
			if (bulk_kptr == NULL || bulk_ptr == NULL)
				break;
			ret = __dbc_put(dbc, &tkey, &tdata,
			    LF_ISSET(DB_OPFLAGS_MASK));
			if (ret == 0)
				++key->doff;
		}
	} else if (LF_ISSET(DB_MULTIPLE_KEY)) {
		ret = 0;
		memset(&tkey, 0, sizeof(tkey));
		if (dbp->type == DB_QUEUE || dbp->type == DB_RECNO) {
			tkey.data = &recno;
			tkey.size = sizeof(recno);
		}
		memset(&tdata, 0, sizeof(tdata));
		DB_MULTIPLE_INIT(bulk_ptr, key);
		while (ret == 0) {
			if (dbp->type == DB_QUEUE || dbp->type == DB_RECNO)
				DB_MULTIPLE_RECNO_NEXT(bulk_ptr, key, recno,
				    tdata.data, tdata.size);
			else
				DB_MULTIPLE_KEY_NEXT(bulk_ptr, key, tkey.data,
				    tkey.size, tdata.data, tdata.size);
			if (bulk_ptr == NULL)
				break;
			ret = __dbc_put(dbc, &tkey, &tdata,
			    LF_ISSET(DB_OPFLAGS_MASK));
			if (ret == 0)
				++key->doff;
		}
	} else
		ret = __dbc_put(dbc, key, data, flags);

err:	/* Close the cursor. */
	if ((t_ret = __dbc_close(dbc)) != 0 && ret == 0)
		ret = t_ret;

	return (ret);
}

/*
 * __db_del --
 *	Delete the items referenced by a key.
 *
 * PUBLIC: int __db_del __P((DB *,
 * PUBLIC:      DB_THREAD_INFO *, DB_TXN *, DBT *, u_int32_t));
 */
int
__db_del(dbp, ip, txn, key, flags)
	DB *dbp;
	DB_THREAD_INFO *ip;
	DB_TXN *txn;
	DBT *key;
	u_int32_t flags;
{
	DBC *dbc;
	DBT data, tkey;
	void *bulk_ptr;
	db_recno_t recno;
	u_int32_t cursor_flags, f_init, f_next;
	int ret, t_ret;

	COMPQUIET(bulk_ptr, NULL);
	/* Allocate a cursor. */
	cursor_flags = DB_WRITELOCK;
	if (LF_ISSET(DB_MULTIPLE | DB_MULTIPLE_KEY))
		cursor_flags |= DB_CURSOR_BULK;
	if ((ret = __db_cursor(dbp, ip, txn, &dbc, cursor_flags)) != 0)
		goto err;

	DEBUG_LWRITE(dbc, txn, "DB->del", key, NULL, flags);

#ifdef HAVE_COMPRESSION
	if (DB_IS_COMPRESSED(dbp) && !F_ISSET(dbp, DB_AM_SECONDARY) &&
	    !DB_IS_PRIMARY(dbp) && LIST_FIRST(&dbp->f_primaries) == NULL) {
		F_SET(dbc, DBC_TRANSIENT);
		ret = __dbc_bulk_del(dbc, key, flags);
		goto err;
	}
#endif

	/*
	 * Walk a cursor through the key/data pairs, deleting as we go.  Set
	 * the DB_DBT_USERMEM flag, as this might be a threaded application
	 * and the flags checking will catch us.  We don't actually want the
	 * keys or data, set DB_DBT_ISSET.  We rely on __dbc_get to clear
	 * this.
	 */
	memset(&data, 0, sizeof(data));
	F_SET(&data, DB_DBT_USERMEM);
	tkey = *key;

	f_init = LF_ISSET(DB_MULTIPLE_KEY) ? DB_GET_BOTH : DB_SET;
	f_next = DB_NEXT_DUP;

	/*
	 * If locking (and we haven't already acquired CDB locks), set the
	 * read-modify-write flag.
	 */
	if (STD_LOCKING(dbc)) {
		f_init |= DB_RMW;
		f_next |= DB_RMW;
	}

	if (LF_ISSET(DB_MULTIPLE | DB_MULTIPLE_KEY)) {
		if (dbp->type == DB_QUEUE || dbp->type == DB_RECNO) {
			memset(&tkey, 0, sizeof(tkey));
			tkey.data = &recno;
			tkey.size = sizeof(recno);
		}
		DB_MULTIPLE_INIT(bulk_ptr, key);
		/* We return the number of keys deleted in doff. */
		key->doff = 0;
bulk_next:	if (dbp->type == DB_QUEUE || dbp->type == DB_RECNO)
			DB_MULTIPLE_RECNO_NEXT(bulk_ptr, key,
			    recno, data.data, data.size);
		else if (LF_ISSET(DB_MULTIPLE))
			DB_MULTIPLE_NEXT(bulk_ptr, key, tkey.data, tkey.size);
		else
			DB_MULTIPLE_KEY_NEXT(bulk_ptr, key,
			    tkey.data, tkey.size, data.data, data.size);
		if (bulk_ptr == NULL)
			goto err;
	}

	/* We're not interested in the data -- do not return it. */
	F_SET(&tkey, DB_DBT_ISSET);
	F_SET(&data, DB_DBT_ISSET);

	/*
	 * Optimize the simple cases.  For all AMs if we don't have secondaries
	 * and are not a secondary and we aren't a foreign database and there
	 * are no dups then we can avoid a bunch of overhead.  For queue we
	 * don't need to fetch the record since we delete by direct calculation
	 * from the record number.
	 *
	 * Hash permits an optimization in DB->del: since on-page duplicates are
	 * stored in a single HKEYDATA structure, it's possible to delete an
	 * entire set of them at once, and as the HKEYDATA has to be rebuilt
	 * and re-put each time it changes, this is much faster than deleting
	 * the duplicates one by one.  Thus, if not pointing at an off-page
	 * duplicate set, and we're not using secondary indices (in which case
	 * we'd have to examine the items one by one anyway), let hash do this
	 * "quick delete".
	 *
	 * !!!
	 * Note that this is the only application-executed delete call in
	 * Berkeley DB that does not go through the __dbc_del function.
	 * If anything other than the delete itself (like a secondary index
	 * update) has to happen there in a particular situation, the
	 * conditions here should be modified not to use these optimizations.
	 * The ordinary AM-independent alternative will work just fine;
	 * it'll just be slower.
	 */
	if (!F_ISSET(dbp, DB_AM_SECONDARY) && !DB_IS_PRIMARY(dbp) &&
	    LIST_FIRST(&dbp->f_primaries) == NULL) {
#ifdef HAVE_QUEUE
		if (dbp->type == DB_QUEUE) {
			ret = __qam_delete(dbc, &tkey, flags);
			goto next;
		}
#endif

		/* Fetch the first record. */
		if ((ret = __dbc_get(dbc, &tkey, &data, f_init)) != 0)
			goto err;

#ifdef HAVE_HASH
		/*
		 * Hash "quick delete" removes all on-page duplicates.  We
		 * can't do that if deleting specific key/data pairs.
		 */
		if (dbp->type == DB_HASH && !LF_ISSET(DB_MULTIPLE_KEY)) {
			DBC *sdbc;
			sdbc = dbc;
#ifdef HAVE_PARTITION
			if (F_ISSET(dbc, DBC_PARTITIONED))
				sdbc =
				    ((PART_CURSOR*)dbc->internal)->sub_cursor;
#endif
			if (sdbc->internal->opd == NULL) {
				ret = __ham_quick_delete(sdbc);
				goto next;
			}
		}
#endif

		if (!F_ISSET(dbp, DB_AM_DUP)) {
			ret = dbc->am_del(dbc, 0);
			goto next;
		}
	} else if ((ret = __dbc_get(dbc, &tkey, &data, f_init)) != 0)
		goto err;

	/* Walk through the set of key/data pairs, deleting as we go. */
	for (;;) {
		if ((ret = __dbc_del(dbc, flags)) != 0)
			break;
		/*
		 * With DB_MULTIPLE_KEY, the application has specified the
		 * exact records they want deleted.  We don't need to walk
		 * through a set of duplicates.
		 */
		if (LF_ISSET(DB_MULTIPLE_KEY))
			break;

		F_SET(&tkey, DB_DBT_ISSET);
		F_SET(&data, DB_DBT_ISSET);
		if ((ret = __dbc_get(dbc, &tkey, &data, f_next)) != 0) {
			if (ret == DB_NOTFOUND)
				ret = 0;
			break;
		}
	}

next:	if (ret == 0 && LF_ISSET(DB_MULTIPLE | DB_MULTIPLE_KEY)) {
		++key->doff;
		goto bulk_next;
	}
err:	/* Discard the cursor. */
	if ((t_ret = __dbc_close(dbc)) != 0 && ret == 0)
		ret = t_ret;

	return (ret);
}

/*
 * __db_sync --
 *	Flush the database cache.
 *
 * PUBLIC: int __db_sync __P((DB *));
 */
int
__db_sync(dbp)
	DB *dbp;
{
	int ret, t_ret;

	ret = 0;

	/* If the database was read-only, we're done. */
	if (F_ISSET(dbp, DB_AM_RDONLY))
		return (0);

	/* If it's a Recno tree, write the backing source text file. */
	if (dbp->type == DB_RECNO)
		ret = __ram_writeback(dbp);

	/* If the database was never backed by a database file, we're done. */
	if (F_ISSET(dbp, DB_AM_INMEM))
		return (ret);
#ifdef HAVE_PARTITION
	if (DB_IS_PARTITIONED(dbp))
		ret = __partition_sync(dbp);
	else
#endif
	if (dbp->type == DB_QUEUE)
		ret = __qam_sync(dbp);
	else
		/* Flush any dirty pages from the cache to the backing file. */
		if ((t_ret = __memp_fsync(dbp->mpf)) != 0 && ret == 0)
			ret = t_ret;

	return (ret);
}

/*
 * __db_associate --
 *	Associate another database as a secondary index to this one.
 *
 * PUBLIC: int __db_associate __P((DB *, DB_THREAD_INFO *, DB_TXN *, DB *,
 * PUBLIC:     int (*)(DB *, const DBT *, const DBT *, DBT *), u_int32_t));
 */
int
__db_associate(dbp, ip, txn, sdbp, callback, flags)
	DB *dbp, *sdbp;
	DB_THREAD_INFO *ip;
	DB_TXN *txn;
	int (*callback) __P((DB *, const DBT *, const DBT *, DBT *));
	u_int32_t flags;
{
	DBC *pdbc, *sdbc;
	DBT key, data, skey, *tskeyp;
	ENV *env;
	int build, ret, t_ret;
	u_int32_t nskey;

	env = dbp->env;
	pdbc = sdbc = NULL;
	ret = 0;

	memset(&skey, 0, sizeof(DBT));
	nskey = 0;
	tskeyp = NULL;

	/*
	 * Check to see if the secondary is empty -- and thus if we should
	 * build it -- before we link it in and risk making it show up in other
	 * threads.  Do this first so that the databases remain unassociated on
	 * error.
	 */
	build = 0;
	if (LF_ISSET(DB_CREATE)) {
		if ((ret = __db_cursor(sdbp, ip, txn, &sdbc, 0)) != 0)
			goto err;

		/*
		 * We don't care about key or data;  we're just doing
		 * an existence check.
		 */
		memset(&key, 0, sizeof(DBT));
		memset(&data, 0, sizeof(DBT));
		F_SET(&key, DB_DBT_PARTIAL | DB_DBT_USERMEM);
		F_SET(&data, DB_DBT_PARTIAL | DB_DBT_USERMEM);
		if ((ret = __dbc_get(sdbc, &key, &data,
		    (STD_LOCKING(sdbc) ? DB_RMW : 0) |
		    DB_FIRST)) == DB_NOTFOUND) {
			build = 1;
			ret = 0;
		}

		if ((t_ret = __dbc_close(sdbc)) != 0 && ret == 0)
			ret = t_ret;

		/* Reset for later error check. */
		sdbc = NULL;

		if (ret != 0)
			goto err;
	}

	/*
	 * Set up the database handle as a secondary.
	 */
	sdbp->s_callback = callback;
	sdbp->s_primary = dbp;

	sdbp->stored_get = sdbp->get;
	sdbp->get = __db_secondary_get;

	sdbp->stored_close = sdbp->close;
	sdbp->close = __db_secondary_close_pp;

	F_SET(sdbp, DB_AM_SECONDARY);

	if (LF_ISSET(DB_IMMUTABLE_KEY))
		FLD_SET(sdbp->s_assoc_flags, DB_ASSOC_IMMUTABLE_KEY);

	/*
	 * Add the secondary to the list on the primary.  Do it here
	 * so that we see any updates that occur while we're walking
	 * the primary.
	 */
	MUTEX_LOCK(env, dbp->mutex);

	/* See __db_s_next for an explanation of secondary refcounting. */
	DB_ASSERT(env, sdbp->s_refcnt == 0);
	sdbp->s_refcnt = 1;
	LIST_INSERT_HEAD(&dbp->s_secondaries, sdbp, s_links);
	MUTEX_UNLOCK(env, dbp->mutex);

	if (build) {
		/*
		 * We loop through the primary, putting each item we
		 * find into the new secondary.
		 *
		 * If we're using CDB, opening these two cursors puts us
		 * in a bit of a locking tangle:  CDB locks are done on the
		 * primary, so that we stay deadlock-free, but that means
		 * that updating the secondary while we have a read cursor
		 * open on the primary will self-block.  To get around this,
		 * we force the primary cursor to use the same locker ID
		 * as the secondary, so they won't conflict.  This should
		 * be harmless even if we're not using CDB.
		 */
		if ((ret = __db_cursor(sdbp, ip, txn, &sdbc,
		    CDB_LOCKING(sdbp->env) ? DB_WRITECURSOR : 0)) != 0)
			goto err;
		if ((ret = __db_cursor_int(dbp, ip,
		    txn, dbp->type, PGNO_INVALID, 0, sdbc->locker, &pdbc)) != 0)
			goto err;

		/* Lock out other threads, now that we have a locker. */
		dbp->associate_locker = sdbc->locker;

		memset(&key, 0, sizeof(DBT));
		memset(&data, 0, sizeof(DBT));
		while ((ret = __dbc_get(pdbc, &key, &data, DB_NEXT)) == 0) {
			if ((ret = callback(sdbp, &key, &data, &skey)) != 0) {
				if (ret == DB_DONOTINDEX)
					continue;
				goto err;
			}
			if (F_ISSET(&skey, DB_DBT_MULTIPLE)) {
#ifdef DIAGNOSTIC
				__db_check_skeyset(sdbp, &skey);
#endif
				nskey = skey.size;
				tskeyp = (DBT *)skey.data;
			} else {
				nskey = 1;
				tskeyp = &skey;
			}
			SWAP_IF_NEEDED(sdbp, &key);
			for (; nskey > 0; nskey--, tskeyp++) {
				if ((ret = __dbc_put(sdbc,
				    tskeyp, &key, DB_UPDATE_SECONDARY)) != 0)
					goto err;
				FREE_IF_NEEDED(env, tskeyp);
			}
			SWAP_IF_NEEDED(sdbp, &key);
			FREE_IF_NEEDED(env, &skey);
		}
		if (ret == DB_NOTFOUND)
			ret = 0;
	}

err:	if (sdbc != NULL && (t_ret = __dbc_close(sdbc)) != 0 && ret == 0)
		ret = t_ret;

	if (pdbc != NULL && (t_ret = __dbc_close(pdbc)) != 0 && ret == 0)
		ret = t_ret;

	dbp->associate_locker = NULL;

	for (; nskey > 0; nskey--, tskeyp++)
		FREE_IF_NEEDED(env, tskeyp);
	FREE_IF_NEEDED(env, &skey);

	return (ret);
}

/*
 * __db_secondary_get --
 *	This wrapper function for DB->pget() is the DB->get() function
 *	on a database which has been made into a secondary index.
 */
static int
__db_secondary_get(sdbp, txn, skey, data, flags)
	DB *sdbp;
	DB_TXN *txn;
	DBT *skey, *data;
	u_int32_t flags;
{
	DB_ASSERT(sdbp->env, F_ISSET(sdbp, DB_AM_SECONDARY));
	return (__db_pget_pp(sdbp, txn, skey, NULL, data, flags));
}

/*
 * __db_secondary_close --
 *	Wrapper function for DB->close() which we use on secondaries to
 *	manage refcounting and make sure we don't close them underneath
 *	a primary that is updating.
 *
 * PUBLIC: int __db_secondary_close __P((DB *, u_int32_t));
 */
int
__db_secondary_close(sdbp, flags)
	DB *sdbp;
	u_int32_t flags;
{
	DB *primary;
	ENV *env;
	int doclose;

	doclose = 0;
	primary = sdbp->s_primary;
	env = primary->env;

	MUTEX_LOCK(env, primary->mutex);
	/*
	 * Check the refcount--if it was at 1 when we were called, no
	 * thread is currently updating this secondary through the primary,
	 * so it's safe to close it for real.
	 *
	 * If it's not safe to do the close now, we do nothing;  the
	 * database will actually be closed when the refcount is decremented,
	 * which can happen in either __db_s_next or __db_s_done.
	 */
	DB_ASSERT(env, sdbp->s_refcnt != 0);
	if (--sdbp->s_refcnt == 0) {
		LIST_REMOVE(sdbp, s_links);
		/* We don't want to call close while the mutex is held. */
		doclose = 1;
	}
	MUTEX_UNLOCK(env, primary->mutex);

	/*
	 * sdbp->close is this function;  call the real one explicitly if
	 * need be.
	 */
	return (doclose ? __db_close(sdbp, NULL, flags) : 0);
}

/*
 * __db_associate_foreign --
 *	Associate this database (fdbp) as a foreign constraint to another
 *	database (pdbp).  That is, dbp's keys appear as foreign key values in
 *	pdbp.
 *
 * PUBLIC: int __db_associate_foreign __P((DB *, DB *,
 * PUBLIC:     int (*)(DB *, const DBT *, DBT *, const DBT *, int *),
 * PUBLIC:     u_int32_t));
 */
int
__db_associate_foreign(fdbp, pdbp, callback, flags)
	DB *fdbp, *pdbp;
	int (*callback)(DB *, const DBT *, DBT *, const DBT *, int *);
	u_int32_t flags;
{
	DB_FOREIGN_INFO *f_info;
	ENV *env;
	int ret;

	env = fdbp->env;
	ret = 0;

	if ((ret = __os_malloc(env, sizeof(DB_FOREIGN_INFO), &f_info)) != 0) {
		return ret;
	}
	memset(f_info, 0, sizeof(DB_FOREIGN_INFO));

	f_info->dbp = pdbp;
	f_info->callback = callback;

	/*
	 * It might be wise to filter this, but for now the flags only
	 * set the delete action type.
	 */
	FLD_SET(f_info->flags, flags);

	/*
	 * Add f_info to the foreign database's list of primaries.  That is to
	 * say, fdbp->f_primaries lists all databases for which fdbp is a
	 * foreign constraint.
	 */
	MUTEX_LOCK(env, fdbp->mutex);
	LIST_INSERT_HEAD(&fdbp->f_primaries, f_info, f_links);
	MUTEX_UNLOCK(env, fdbp->mutex);

	/*
	* Associate fdbp as pdbp's foreign db, for referential integrity
	* checks.  We don't allow the foreign db to be changed, because we
	* currently have no way of removing pdbp from the old foreign db's list
	* of primaries.
	*/
	if (pdbp->s_foreign != NULL)
		return (EINVAL);
	pdbp->s_foreign = fdbp;

	return (ret);
}

static int
__dbc_set_priority(dbc, priority)
	DBC *dbc;
	DB_CACHE_PRIORITY priority;
{
	dbc->priority = priority;
	return (0);
}

static int
__dbc_get_priority(dbc, priority)
	DBC *dbc;
	DB_CACHE_PRIORITY *priority;
{
	*priority = dbc->priority;
	return (0);
}
