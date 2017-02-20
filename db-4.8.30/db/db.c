/*-
 * See the file LICENSE for redistribution information.
 *
 * Copyright (c) 1996-2009 Oracle.  All rights reserved.
 */
/*
 * Copyright (c) 1990, 1993, 1994, 1995, 1996
 *	Keith Bostic.  All rights reserved.
 */
/*
 * Copyright (c) 1990, 1993, 1994, 1995
 *	The Regents of the University of California.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 * $Id$
 */

#include "db_config.h"

#include "db_int.h"
#include "dbinc/db_page.h"
#include "dbinc/db_swap.h"
#include "dbinc/btree.h"
#include "dbinc/fop.h"
#include "dbinc/hash.h"
#include "dbinc/lock.h"
#include "dbinc/log.h"
#include "dbinc/mp.h"
#include "dbinc/partition.h"
#include "dbinc/qam.h"
#include "dbinc/txn.h"

static int __db_disassociate __P((DB *));
static int __db_disassociate_foreign __P ((DB *));

#ifdef CONFIG_TEST
static int __db_makecopy __P((ENV *, const char *, const char *));
static int __qam_testdocopy __P((DB *, const char *));
#endif

/*
 * DB.C --
 *	This file contains the utility functions for the DBP layer.
 */

/*
 * __db_master_open --
 *	Open up a handle on a master database.
 *
 * PUBLIC: int __db_master_open __P((DB *, DB_THREAD_INFO *,
 * PUBLIC:     DB_TXN *, const char *, u_int32_t, int, DB **));
 */
int
__db_master_open(subdbp, ip, txn, name, flags, mode, dbpp)
	DB *subdbp;
	DB_THREAD_INFO *ip;
	DB_TXN *txn;
	const char *name;
	u_int32_t flags;
	int mode;
	DB **dbpp;
{
	DB *dbp;
	int ret;

	*dbpp = NULL;

	/* Open up a handle on the main database. */
	if ((ret = __db_create_internal(&dbp, subdbp->env, 0)) != 0)
		return (ret);

	/*
	 * It's always a btree.
	 * Run in the transaction we've created.
	 * Set the pagesize in case we're creating a new database.
	 * Flag that we're creating a database with subdatabases.
	 */
	dbp->pgsize = subdbp->pgsize;
	F_SET(dbp, DB_AM_SUBDB);
	F_SET(dbp, F_ISSET(subdbp,
	    DB_AM_RECOVER | DB_AM_SWAP |
	    DB_AM_ENCRYPT | DB_AM_CHKSUM | DB_AM_NOT_DURABLE));

	/*
	 * If there was a subdb specified, then we only want to apply
	 * DB_EXCL to the subdb, not the actual file.  We only got here
	 * because there was a subdb specified.
	 */
	LF_CLR(DB_EXCL);
	LF_SET(DB_RDWRMASTER);
	if ((ret = __db_open(dbp, ip,
	    txn, name, NULL, DB_BTREE, flags, mode, PGNO_BASE_MD)) != 0)
		goto err;

	/*
	 * The items in dbp are initialized from the master file's meta page.
	 * Other items such as checksum and encryption are checked when we
	 * read the meta-page, so we do not check those here.  However, if
	 * the meta-page caused checksumming to be turned on and it wasn't
	 * already, set it here.
	 */
	if (F_ISSET(dbp, DB_AM_CHKSUM))
		F_SET(subdbp, DB_AM_CHKSUM);

	/*
	 * The user may have specified a page size for an existing file,
	 * which we want to ignore.
	 */
	subdbp->pgsize = dbp->pgsize;
	*dbpp = dbp;

	if (0) {
err:		if (!F_ISSET(dbp, DB_AM_DISCARD))
			(void)__db_close(dbp, txn, 0);
	}

	return (ret);
}

/*
 * __db_master_update --
 *	Add/Open/Remove a subdatabase from a master database.
 *
 * PUBLIC: int __db_master_update __P((DB *, DB *, DB_THREAD_INFO *, DB_TXN *,
 * PUBLIC:      const char *, DBTYPE, mu_action, const char *, u_int32_t));
 */
int
__db_master_update(mdbp, sdbp, ip, txn, subdb, type, action, newname, flags)
	DB *mdbp, *sdbp;
	DB_TXN *txn;
	DB_THREAD_INFO *ip;
	const char *subdb;
	DBTYPE type;
	mu_action action;
	const char *newname;
	u_int32_t flags;
{
	DBC *dbc, *ndbc;
	DBT key, data, ndata;
	ENV *env;
	PAGE *p, *r;
	db_pgno_t t_pgno;
	int modify, ret, t_ret;

	env = mdbp->env;
	dbc = ndbc = NULL;
	p = NULL;

	/*
	 * Open up a cursor.  If this is CDB and we're creating the database,
	 * make it an update cursor.
	 *
	 * Might we modify the master database?  If so, we'll need to lock.
	 */
	modify = (action != MU_OPEN || LF_ISSET(DB_CREATE)) ? 1 : 0;

	if ((ret = __db_cursor(mdbp, ip, txn, &dbc,
	    (CDB_LOCKING(env) && modify) ? DB_WRITECURSOR : 0)) != 0)
		return (ret);

	/*
	 * Point the cursor at the record.
	 *
	 * If we're removing or potentially creating an entry, lock the page
	 * with DB_RMW.
	 *
	 * We do multiple cursor operations with the cursor in some cases and
	 * subsequently access the data DBT information.  Set DB_DBT_MALLOC so
	 * we don't risk modification of the data between our uses of it.
	 *
	 * !!!
	 * We don't include the name's nul termination in the database.
	 */
	DB_INIT_DBT(key, subdb, strlen(subdb));
	memset(&data, 0, sizeof(data));
	F_SET(&data, DB_DBT_MALLOC);

	ret = __dbc_get(dbc, &key, &data,
	    DB_SET | ((STD_LOCKING(dbc) && modify) ? DB_RMW : 0));

	/*
	 * What we do next--whether or not we found a record for the
	 * specified subdatabase--depends on what the specified action is.
	 * Handle ret appropriately as the first statement of each case.
	 */
	switch (action) {
	case MU_REMOVE:
		/*
		 * We should have found something if we're removing it.  Note
		 * that in the common case where the DB we're asking to remove
		 * doesn't exist, we won't get this far;  __db_subdb_remove
		 * will already have returned an error from __db_open.
		 */
		if (ret != 0)
			goto err;

		/*
		 * Delete the subdatabase entry first;  if this fails,
		 * we don't want to touch the actual subdb pages.
		 */
		if ((ret = __dbc_del(dbc, 0)) != 0)
			goto err;

		/*
		 * We're handling actual data, not on-page meta-data,
		 * so it hasn't been converted to/from opposite
		 * endian architectures.  Do it explicitly, now.
		 */
		memcpy(&sdbp->meta_pgno, data.data, sizeof(db_pgno_t));
		DB_NTOHL_SWAP(env, &sdbp->meta_pgno);
		if ((ret = __memp_fget(mdbp->mpf, &sdbp->meta_pgno,
		    ip, dbc->txn, DB_MPOOL_DIRTY, &p)) != 0)
			goto err;

		/* Free the root on the master db if it was created. */
		if (TYPE(p) == P_BTREEMETA &&
		    ((BTMETA *)p)->root != PGNO_INVALID) {
			if ((ret = __memp_fget(mdbp->mpf,
			    &((BTMETA *)p)->root, ip, dbc->txn,
			    DB_MPOOL_DIRTY, &r)) != 0)
				goto err;

			/* Free and put the page. */
			if ((ret = __db_free(dbc, r)) != 0) {
				r = NULL;
				goto err;
			}
		}
		/* Free and put the page. */
		if ((ret = __db_free(dbc, p)) != 0) {
			p = NULL;
			goto err;
		}
		p = NULL;
		break;
	case MU_RENAME:
		/* We should have found something if we're renaming it. */
		if (ret != 0)
			goto err;

		/*
		 * Before we rename, we need to make sure we're not
		 * overwriting another subdatabase, or else this operation
		 * won't be undoable.  Open a second cursor and check
		 * for the existence of newname;  it shouldn't appear under
		 * us since we hold the metadata lock.
		 */
		if ((ret = __db_cursor(mdbp, ip, txn, &ndbc,
		    CDB_LOCKING(env) ? DB_WRITECURSOR : 0)) != 0)
			goto err;
		DB_SET_DBT(key, newname, strlen(newname));

		/*
		 * We don't actually care what the meta page of the potentially-
		 * overwritten DB is; we just care about existence.
		 */
		memset(&ndata, 0, sizeof(ndata));
		F_SET(&ndata, DB_DBT_USERMEM | DB_DBT_PARTIAL);

		if ((ret = __dbc_get(ndbc, &key, &ndata, DB_SET)) == 0) {
			/* A subdb called newname exists.  Bail. */
			ret = EEXIST;
			__db_errx(env, "rename: database %s exists", newname);
			goto err;
		} else if (ret != DB_NOTFOUND)
			goto err;

		/*
		 * Now do the put first; we don't want to lose our only
		 * reference to the subdb.  Use the second cursor so the
		 * first one continues to point to the old record.
		 */
		if ((ret = __dbc_put(ndbc, &key, &data, DB_KEYFIRST)) != 0)
			goto err;
		if ((ret = __dbc_del(dbc, 0)) != 0) {
			/*
			 * If the delete fails, try to delete the record
			 * we just put, in case we're not txn-protected.
			 */
			(void)__dbc_del(ndbc, 0);
			goto err;
		}

		break;
	case MU_OPEN:
		/*
		 * Get the subdatabase information.  If it already exists,
		 * copy out the page number and we're done.
		 */
		switch (ret) {
		case 0:
			if (LF_ISSET(DB_CREATE) && LF_ISSET(DB_EXCL)) {
				ret = EEXIST;
				goto err;
			}
			memcpy(&sdbp->meta_pgno, data.data, sizeof(db_pgno_t));
			DB_NTOHL_SWAP(env, &sdbp->meta_pgno);
			goto done;
		case DB_NOTFOUND:
			if (LF_ISSET(DB_CREATE))
				break;
			/*
			 * No db_err, it is reasonable to remove a
			 * nonexistent db.
			 */
			ret = ENOENT;
			goto err;
		default:
			goto err;
		}

		/* Create a subdatabase. */
		if ((ret = __db_new(dbc,
		    type == DB_HASH ? P_HASHMETA : P_BTREEMETA, NULL, &p)) != 0)
			goto err;
		sdbp->meta_pgno = PGNO(p);

		/*
		 * XXX
		 * We're handling actual data, not on-page meta-data, so it
		 * hasn't been converted to/from opposite endian architectures.
		 * Do it explicitly, now.
		 */
		t_pgno = PGNO(p);
		DB_HTONL_SWAP(env, &t_pgno);
		memset(&ndata, 0, sizeof(ndata));
		ndata.data = &t_pgno;
		ndata.size = sizeof(db_pgno_t);
		if ((ret = __dbc_put(dbc, &key, &ndata, 0)) != 0)
			goto err;
		F_SET(sdbp, DB_AM_CREATED);
		break;
	}

err:
done:	/*
	 * If we allocated a page: if we're successful, mark the page dirty
	 * and return it to the cache, otherwise, discard/free it.
	 */
	if (p != NULL && (t_ret = __memp_fput(mdbp->mpf,
	     dbc->thread_info, p, dbc->priority)) != 0 && ret == 0)
		ret = t_ret;

	/* Discard the cursor(s) and data. */
	if (data.data != NULL)
		__os_ufree(env, data.data);
	if (dbc != NULL && (t_ret = __dbc_close(dbc)) != 0 && ret == 0)
		ret = t_ret;
	if (ndbc != NULL && (t_ret = __dbc_close(ndbc)) != 0 && ret == 0)
		ret = t_ret;

	return (ret);
}

/*
 * __env_setup --
 *	Set up the underlying environment during a db_open.
 *
 * PUBLIC: int __env_setup __P((DB *,
 * PUBLIC:     DB_TXN *, const char *, const char *, u_int32_t, u_int32_t));
 */
int
__env_setup(dbp, txn, fname, dname, id, flags)
	DB *dbp;
	DB_TXN *txn;
	const char *fname, *dname;
	u_int32_t id, flags;
{
	DB *ldbp;
	DB_ENV *dbenv;
	ENV *env;
	u_int32_t maxid;
	int ret;

	env = dbp->env;
	dbenv = env->dbenv;

	/* If we don't yet have an environment, it's time to create it. */
	if (!F_ISSET(env, ENV_OPEN_CALLED)) {
		/* Make sure we have at least DB_MINCACHE pages in our cache. */
		if (dbenv->mp_gbytes == 0 &&
		    dbenv->mp_bytes < dbp->pgsize * DB_MINPAGECACHE &&
		    (ret = __memp_set_cachesize(
		    dbenv, 0, dbp->pgsize * DB_MINPAGECACHE, 0)) != 0)
			return (ret);

		if ((ret = __env_open(dbenv, NULL, DB_CREATE |
		    DB_INIT_MPOOL | DB_PRIVATE | LF_ISSET(DB_THREAD), 0)) != 0)
			return (ret);
	}

	/* Join the underlying cache. */
	if ((!F_ISSET(dbp, DB_AM_INMEM) || dname == NULL) &&
	    (ret = __env_mpool(dbp, fname, flags)) != 0)
		return (ret);

	/* We may need a per-thread mutex. */
	if (LF_ISSET(DB_THREAD) && (ret = __mutex_alloc(
	    env, MTX_DB_HANDLE, DB_MUTEX_PROCESS_ONLY, &dbp->mutex)) != 0)
		return (ret);

	/*
	 * Set up a bookkeeping entry for this database in the log region,
	 * if such a region exists.  Note that even if we're in recovery
	 * or a replication client, where we won't log registries, we'll
	 * still need an FNAME struct, so LOGGING_ON is the correct macro.
	 */
	if (LOGGING_ON(env) && dbp->log_filename == NULL
#if !defined(DEBUG_ROP) && !defined(DEBUG_WOP) && !defined(DIAGNOSTIC)
	    && (txn != NULL || F_ISSET(dbp, DB_AM_RECOVER))
#endif
#if !defined(DEBUG_ROP)
	    && !F_ISSET(dbp, DB_AM_RDONLY)
#endif
	    ) {
		if ((ret = __dbreg_setup(dbp,
		    F_ISSET(dbp, DB_AM_INMEM) ? dname : fname,
		    F_ISSET(dbp, DB_AM_INMEM) ? NULL : dname, id)) != 0)
			return (ret);

		/*
		 * If we're actively logging and our caller isn't a
		 * recovery function that already did so, then assign
		 * this dbp a log fileid.
		 */
		if (DBENV_LOGGING(env) && !F_ISSET(dbp, DB_AM_RECOVER) &&
		    (ret = __dbreg_new_id(dbp, txn)) != 0)
			return (ret);
	}

	/*
	 * Insert ourselves into the ENV's dblist.  We allocate a
	 * unique ID to each {fileid, meta page number} pair, and to
	 * each temporary file (since they all have a zero fileid).
	 * This ID gives us something to use to tell which DB handles
	 * go with which databases in all the cursor adjustment
	 * routines, where we don't want to do a lot of ugly and
	 * expensive memcmps.
	 */
	MUTEX_LOCK(env, env->mtx_dblist);
	maxid = 0;
	TAILQ_FOREACH(ldbp, &env->dblist, dblistlinks) {
		/*
		 * There are three cases: on-disk database (first clause),
		 * named in-memory database (second clause), temporary database
		 * (never matches; no clause).
		 */
		if (!F_ISSET(dbp, DB_AM_INMEM)) {
			if (memcmp(ldbp->fileid, dbp->fileid, DB_FILE_ID_LEN)
			    == 0 && ldbp->meta_pgno == dbp->meta_pgno)
				break;
		} else if (dname != NULL) {
			if (F_ISSET(ldbp, DB_AM_INMEM) &&
			    ldbp->dname != NULL &&
			    strcmp(ldbp->dname, dname) == 0)
				break;
		}
		if (ldbp->adj_fileid > maxid)
			maxid = ldbp->adj_fileid;
	}

	/*
	 * If ldbp is NULL, we didn't find a match. Assign the dbp an
	 * adj_fileid one higher than the largest we found, and
	 * insert it at the head of the master dbp list.
	 *
	 * If ldbp is not NULL, it is a match for our dbp.  Give dbp
	 * the same ID that ldbp has, and add it after ldbp so they're
	 * together in the list.
	 */
	if (ldbp == NULL) {
		dbp->adj_fileid = maxid + 1;
		TAILQ_INSERT_HEAD(&env->dblist, dbp, dblistlinks);
	} else {
		dbp->adj_fileid = ldbp->adj_fileid;
		TAILQ_INSERT_AFTER(&env->dblist, ldbp, dbp, dblistlinks);
	}
	MUTEX_UNLOCK(env, env->mtx_dblist);

	return (0);
}

/*
 * __env_mpool --
 *	Set up the underlying environment cache during a db_open.
 *
 * PUBLIC: int __env_mpool __P((DB *, const char *, u_int32_t));
 */
int
__env_mpool(dbp, fname, flags)
	DB *dbp;
	const char *fname;
	u_int32_t flags;
{
	DBT pgcookie;
	DB_MPOOLFILE *mpf;
	DB_PGINFO pginfo;
	ENV *env;
	int fidset, ftype, ret;
	int32_t lsn_off;
	u_int8_t nullfid[DB_FILE_ID_LEN];
	u_int32_t clear_len;

	env = dbp->env;

	/* The LSN is the first entry on a DB page, byte offset 0. */
	lsn_off = F_ISSET(dbp, DB_AM_NOT_DURABLE) ? DB_LSN_OFF_NOTSET : 0;

	/* It's possible that this database is already open. */
	if (F_ISSET(dbp, DB_AM_OPEN_CALLED))
		return (0);

	/*
	 * If we need to pre- or post-process a file's pages on I/O, set the
	 * file type.  If it's a hash file, always call the pgin and pgout
	 * routines.  This means that hash files can never be mapped into
	 * process memory.  If it's a btree file and requires swapping, we
	 * need to page the file in and out.  This has to be right -- we can't
	 * mmap files that are being paged in and out.
	 */
	switch (dbp->type) {
	case DB_BTREE:
	case DB_RECNO:
		ftype = F_ISSET(dbp, DB_AM_SWAP | DB_AM_ENCRYPT | DB_AM_CHKSUM)
		    ? DB_FTYPE_SET : DB_FTYPE_NOTSET;
		clear_len = CRYPTO_ON(env) ?
		    (dbp->pgsize != 0 ? dbp->pgsize : DB_CLEARLEN_NOTSET) :
		    DB_PAGE_DB_LEN;
		break;
	case DB_HASH:
		ftype = DB_FTYPE_SET;
		clear_len = CRYPTO_ON(env) ?
		    (dbp->pgsize != 0 ? dbp->pgsize : DB_CLEARLEN_NOTSET) :
		    DB_PAGE_DB_LEN;
		break;
	case DB_QUEUE:
		ftype = F_ISSET(dbp,
		    DB_AM_SWAP | DB_AM_ENCRYPT | DB_AM_CHKSUM) ?
		    DB_FTYPE_SET : DB_FTYPE_NOTSET;

		/*
		 * If we came in here without a pagesize set, then we need
		 * to mark the in-memory handle as having clear_len not
		 * set, because we don't really know the clear length or
		 * the page size yet (since the file doesn't yet exist).
		 */
		clear_len = dbp->pgsize != 0 ? dbp->pgsize : DB_CLEARLEN_NOTSET;
		break;
	case DB_UNKNOWN:
		/*
		 * If we're running in the verifier, our database might
		 * be corrupt and we might not know its type--but we may
		 * still want to be able to verify and salvage.
		 *
		 * If we can't identify the type, it's not going to be safe
		 * to call __db_pgin--we pretty much have to give up all
		 * hope of salvaging cross-endianness.  Proceed anyway;
		 * at worst, the database will just appear more corrupt
		 * than it actually is, but at best, we may be able
		 * to salvage some data even with no metadata page.
		 */
		if (F_ISSET(dbp, DB_AM_VERIFYING)) {
			ftype = DB_FTYPE_NOTSET;
			clear_len = DB_PAGE_DB_LEN;
			break;
		}

		/*
		 * This might be an in-memory file and we won't know its
		 * file type until after we open it and read the meta-data
		 * page.
		 */
		if (F_ISSET(dbp, DB_AM_INMEM)) {
			clear_len = DB_CLEARLEN_NOTSET;
			ftype = DB_FTYPE_NOTSET;
			lsn_off = DB_LSN_OFF_NOTSET;
			break;
		}
		/* FALLTHROUGH */
	default:
		return (__db_unknown_type(env, "DB->open", dbp->type));
	}

	mpf = dbp->mpf;

	memset(nullfid, 0, DB_FILE_ID_LEN);
	fidset = memcmp(nullfid, dbp->fileid, DB_FILE_ID_LEN);
	if (fidset)
		(void)__memp_set_fileid(mpf, dbp->fileid);

	(void)__memp_set_clear_len(mpf, clear_len);
	(void)__memp_set_ftype(mpf, ftype);
	(void)__memp_set_lsn_offset(mpf, lsn_off);

	pginfo.db_pagesize = dbp->pgsize;
	pginfo.flags =
	    F_ISSET(dbp, (DB_AM_CHKSUM | DB_AM_ENCRYPT | DB_AM_SWAP));
	pginfo.type = dbp->type;
	pgcookie.data = &pginfo;
	pgcookie.size = sizeof(DB_PGINFO);
	(void)__memp_set_pgcookie(mpf, &pgcookie);

#ifndef DIAG_MVCC
	if (F_ISSET(env->dbenv, DB_ENV_MULTIVERSION))
#endif
		if (F_ISSET(dbp, DB_AM_TXN) &&
		    dbp->type != DB_QUEUE && dbp->type != DB_UNKNOWN)
			LF_SET(DB_MULTIVERSION);

	if ((ret = __memp_fopen(mpf, NULL, fname, &dbp->dirname,
	    LF_ISSET(DB_CREATE | DB_DURABLE_UNKNOWN | DB_MULTIVERSION |
		DB_NOMMAP | DB_ODDFILESIZE | DB_RDONLY | DB_TRUNCATE) |
	    (F_ISSET(env->dbenv, DB_ENV_DIRECT_DB) ? DB_DIRECT : 0) |
	    (F_ISSET(dbp, DB_AM_NOT_DURABLE) ? DB_TXN_NOT_DURABLE : 0),
	    0, dbp->pgsize)) != 0) {
		/*
		 * The open didn't work; we need to reset the mpf,
		 * retaining the in-memory semantics (if any).
		 */
		(void)__memp_fclose(dbp->mpf, 0);
		(void)__memp_fcreate(env, &dbp->mpf);
		if (F_ISSET(dbp, DB_AM_INMEM))
			MAKE_INMEM(dbp);
		return (ret);
	}

	/*
	 * Set the open flag.  We use it to mean that the dbp has gone
	 * through mpf setup, including dbreg_register.  Also, below,
	 * the underlying access method open functions may want to do
	 * things like acquire cursors, so the open flag has to be set
	 * before calling them.
	 */
	F_SET(dbp, DB_AM_OPEN_CALLED);
	if (!fidset && fname != NULL) {
		(void)__memp_get_fileid(dbp->mpf, dbp->fileid);
		dbp->preserve_fid = 1;
	}

	return (0);
}

/*
 * __db_close --
 *	DB->close method.
 *
 * PUBLIC: int __db_close __P((DB *, DB_TXN *, u_int32_t));
 */
int
__db_close(dbp, txn, flags)
	DB *dbp;
	DB_TXN *txn;
	u_int32_t flags;
{
	ENV *env;
	int db_ref, deferred_close, ret, t_ret;

	env = dbp->env;
	deferred_close = ret = 0;

	/*
	 * Validate arguments, but as a DB handle destructor, we can't fail.
	 *
	 * Check for consistent transaction usage -- ignore errors.  Only
	 * internal callers specify transactions, so it's a serious problem
	 * if we get error messages.
	 */
	if (txn != NULL)
		(void)__db_check_txn(dbp, txn, DB_LOCK_INVALIDID, 0);

	/* Refresh the structure and close any underlying resources. */
	ret = __db_refresh(dbp, txn, flags, &deferred_close, 0);

	/*
	 * If we've deferred the close because the logging of the close failed,
	 * return our failure right away without destroying the handle.
	 */
	if (deferred_close)
		return (ret);

	/* !!!
	 * This code has an apparent race between the moment we read and
	 * decrement env->db_ref and the moment we check whether it's 0.
	 * However, if the environment is DBLOCAL, the user shouldn't have a
	 * reference to the env handle anyway;  the only way we can get
	 * multiple dbps sharing a local env is if we open them internally
	 * during something like a subdatabase open.  If any such thing is
	 * going on while the user is closing the original dbp with a local
	 * env, someone's already badly screwed up, so there's no reason
	 * to bother engineering around this possibility.
	 */
	MUTEX_LOCK(env, env->mtx_dblist);
	db_ref = --env->db_ref;
	MUTEX_UNLOCK(env, env->mtx_dblist);
	if (F_ISSET(env, ENV_DBLOCAL) && db_ref == 0 &&
	    (t_ret = __env_close(env->dbenv, 0)) != 0 && ret == 0)
		ret = t_ret;

	/* Free the database handle. */
	memset(dbp, CLEAR_BYTE, sizeof(*dbp));
	__os_free(env, dbp);

	return (ret);
}

/*
 * __db_refresh --
 *	Refresh the DB structure, releasing any allocated resources.
 * This does most of the work of closing files now because refresh
 * is what is used during abort processing (since we can't destroy
 * the actual handle) and during abort processing, we may have a
 * fully opened handle.
 *
 * PUBLIC: int __db_refresh __P((DB *, DB_TXN *, u_int32_t, int *, int));
 */
int
__db_refresh(dbp, txn, flags, deferred_closep, reuse)
	DB *dbp;
	DB_TXN *txn;
	u_int32_t flags;
	int *deferred_closep, reuse;
{
	DB *sdbp;
	DBC *dbc;
	DB_FOREIGN_INFO *f_info, *tmp;
	DB_LOCKER *locker;
	DB_LOCKREQ lreq;
	ENV *env;
	REGENV *renv;
	REGINFO *infop;
	u_int32_t save_flags;
	int resync, ret, t_ret;

	ret = 0;

	env = dbp->env;
	infop = env->reginfo;
	if (infop != NULL)
		renv = infop->primary;
	else
		renv = NULL;

	/*
	 * If this dbp is not completely open, avoid trapping by trying to
	 * sync without an mpool file.
	 */
	if (dbp->mpf == NULL)
		LF_SET(DB_NOSYNC);

	/* If never opened, or not currently open, it's easy. */
	if (!F_ISSET(dbp, DB_AM_OPEN_CALLED))
		goto never_opened;

	/*
	 * If we have any secondary indices, disassociate them from us.
	 * We don't bother with the mutex here;  it only protects some
	 * of the ops that will make us core-dump mid-close anyway, and
	 * if you're trying to do something with a secondary *while* you're
	 * closing the primary, you deserve what you get.  The disassociation
	 * is mostly done just so we can close primaries and secondaries in
	 * any order--but within one thread of control.
	 */
	LIST_FOREACH(sdbp, &dbp->s_secondaries, s_links) {
		LIST_REMOVE(sdbp, s_links);
		if ((t_ret = __db_disassociate(sdbp)) != 0 && ret == 0)
			ret = t_ret;
	}

	/*
	 * Disassociate ourself from any databases using us as a foreign key
	 * database by clearing the referring db's pointer.  Reclaim memory.
	 */
	f_info = LIST_FIRST(&dbp->f_primaries);
	while (f_info != NULL) {
		tmp = LIST_NEXT(f_info, f_links);
		LIST_REMOVE(f_info, f_links);
		f_info->dbp->s_foreign = NULL;
		__os_free(env, f_info);
		f_info = tmp;
	}

	if (dbp->s_foreign != NULL &&
	    (t_ret = __db_disassociate_foreign(dbp)) != 0 && ret == 0)
		ret = t_ret;

	/*
	 * Sync the underlying access method.  Do before closing the cursors
	 * because DB->sync allocates cursors in order to write Recno backing
	 * source text files.
	 *
	 * Sync is slow on some systems, notably Solaris filesystems where the
	 * entire buffer cache is searched.  If we're in recovery, don't flush
	 * the file, it's not necessary.
	 */
	if (!LF_ISSET(DB_NOSYNC) &&
	    !F_ISSET(dbp, DB_AM_DISCARD | DB_AM_RECOVER) &&
	    (t_ret = __db_sync(dbp)) != 0 && ret == 0)
		ret = t_ret;

	/*
	 * Go through the active cursors and call the cursor recycle routine,
	 * which resolves pending operations and moves the cursors onto the
	 * free list.  Then, walk the free list and call the cursor destroy
	 * routine.  Note that any failure on a close is considered "really
	 * bad" and we just break out of the loop and force forward.
	 */
	resync = TAILQ_FIRST(&dbp->active_queue) == NULL ? 0 : 1;
	while ((dbc = TAILQ_FIRST(&dbp->active_queue)) != NULL)
		if ((t_ret = __dbc_close(dbc)) != 0) {
			if (ret == 0)
				ret = t_ret;
			break;
		}

	while ((dbc = TAILQ_FIRST(&dbp->free_queue)) != NULL)
		if ((t_ret = __dbc_destroy(dbc)) != 0) {
			if (ret == 0)
				ret = t_ret;
			break;
		}

	/*
	 * Close any outstanding join cursors.  Join cursors destroy themselves
	 * on close and have no separate destroy routine.  We don't have to set
	 * the resync flag here, because join cursors aren't write cursors.
	 */
	while ((dbc = TAILQ_FIRST(&dbp->join_queue)) != NULL)
		if ((t_ret = __db_join_close(dbc)) != 0) {
			if (ret == 0)
				ret = t_ret;
			break;
		}

	/*
	 * Sync the memory pool, even though we've already called DB->sync,
	 * because closing cursors can dirty pages by deleting items they
	 * referenced.
	 *
	 * Sync is slow on some systems, notably Solaris filesystems where the
	 * entire buffer cache is searched.  If we're in recovery, don't flush
	 * the file, it's not necessary.
	 */
	if (resync && !LF_ISSET(DB_NOSYNC) &&
	    !F_ISSET(dbp, DB_AM_DISCARD | DB_AM_RECOVER) &&
	    (t_ret = __memp_fsync(dbp->mpf)) != 0 && ret == 0)
		ret = t_ret;

never_opened:
	/*
	 * At this point, we haven't done anything to render the DB handle
	 * unusable, at least by a transaction abort.  Take the opportunity
	 * now to log the file close if we have initialized the logging
	 * information.  If this log fails and we're in a transaction,
	 * we have to bail out of the attempted close; we'll need a dbp in
	 * order to successfully abort the transaction, and we can't conjure
	 * a new one up because we haven't gotten out the dbreg_register
	 * record that represents the close.  In this case, we put off
	 * actually closing the dbp until we've performed the abort.
	 */
	if (!reuse && LOGGING_ON(dbp->env) && dbp->log_filename != NULL) {
		/*
		 * Discard the log file id, if any.  We want to log the close
		 * if and only if this is not a recovery dbp or a client dbp,
		 * or a dead dbp handle.
		 */
		DB_ASSERT(env, renv != NULL);
		if (F_ISSET(dbp, DB_AM_RECOVER) || IS_REP_CLIENT(env) ||
		    dbp->timestamp != renv->rep_timestamp) {
			if ((t_ret = __dbreg_revoke_id(dbp,
			    0, DB_LOGFILEID_INVALID)) == 0 && ret == 0)
				ret = t_ret;
			if ((t_ret = __dbreg_teardown(dbp)) != 0 && ret == 0)
				ret = t_ret;
		} else {
			if ((t_ret = __dbreg_close_id(dbp,
			    txn, DBREG_CLOSE)) != 0 && txn != NULL) {
				/*
				 * We're in a txn and the attempt to log the
				 * close failed;  let the txn subsystem know
				 * that we need to destroy this dbp once we're
				 * done with the abort, then bail from the
				 * close.
				 *
				 * Note that if the attempt to put off the
				 * close -also- fails--which it won't unless
				 * we're out of heap memory--we're really
				 * screwed.  Panic.
				 */
				if ((ret =
				    __txn_closeevent(env, txn, dbp)) != 0)
					return (__env_panic(env, ret));
				if (deferred_closep != NULL)
					*deferred_closep = 1;
				return (t_ret);
			}
			/*
			 * If dbreg_close_id failed and we were not in a
			 * transaction, then we need to finish this close
			 * because the caller can't do anything with the
			 * handle after we return an error.  We rely on
			 * dbreg_close_id to mark the entry in some manner
			 * so that we do not do a clean shutdown of this
			 * environment.  If shutdown isn't clean, then the
			 * application *must* run recovery and that will
			 * generate the RCLOSE record.
			 */
		}

	}

	/* Close any handle we've been holding since the open.  */
	if (dbp->saved_open_fhp != NULL &&
	    (t_ret = __os_closehandle(env, dbp->saved_open_fhp)) != 0 &&
	    ret == 0)
		ret = t_ret;

	/*
	 * Remove this DB handle from the ENV's dblist, if it's been added.
	 *
	 * Close our reference to the underlying cache while locked, we don't
	 * want to race with a thread searching for our underlying cache link
	 * while opening a DB handle.
	 *
	 * The DB handle may not yet have been added to the ENV list, don't
	 * blindly call the underlying TAILQ_REMOVE macro.  Explicitly reset
	 * the field values to NULL so that we can't call TAILQ_REMOVE twice.
	 */
	MUTEX_LOCK(env, env->mtx_dblist);
	if (!reuse &&
	    (dbp->dblistlinks.tqe_next != NULL ||
	    dbp->dblistlinks.tqe_prev != NULL)) {
		TAILQ_REMOVE(&env->dblist, dbp, dblistlinks);
		dbp->dblistlinks.tqe_next = NULL;
		dbp->dblistlinks.tqe_prev = NULL;
	}

	/* Close the memory pool file handle. */
	if (dbp->mpf != NULL) {
		if ((t_ret = __memp_fclose(dbp->mpf,
		    F_ISSET(dbp, DB_AM_DISCARD) ? DB_MPOOL_DISCARD : 0)) != 0 &&
		    ret == 0)
			ret = t_ret;
		dbp->mpf = NULL;
		if (reuse &&
		    (t_ret = __memp_fcreate(env, &dbp->mpf)) != 0 &&
		    ret == 0)
			ret = t_ret;
	}

	MUTEX_UNLOCK(env, env->mtx_dblist);

	/*
	 * Call the access specific close function.
	 *
	 * We do this here rather than in __db_close as we need to do this when
	 * aborting an open so that file descriptors are closed and abort of
	 * renames can succeed on platforms that lock open files (such as
	 * Windows).  In particular, we need to ensure that all the extents
	 * associated with a queue are closed so that queue renames can be
	 * aborted.
	 *
	 * It is also important that we do this before releasing the handle
	 * lock, because dbremove and dbrename assume that once they have the
	 * handle lock, it is safe to modify the underlying file(s).
	 *
	 * !!!
	 * Because of where these functions are called in the DB handle close
	 * process, these routines can't do anything that would dirty pages or
	 * otherwise affect closing down the database.  Specifically, we can't
	 * abort and recover any of the information they control.
	 */
#ifdef HAVE_PARTITION
	if (dbp->p_internal != NULL &&
	    (t_ret = __partition_close(dbp, txn, flags)) != 0 && ret == 0)
		ret = t_ret;
#endif
	if ((t_ret = __bam_db_close(dbp)) != 0 && ret == 0)
		ret = t_ret;
	if ((t_ret = __ham_db_close(dbp)) != 0 && ret == 0)
		ret = t_ret;
	if ((t_ret = __qam_db_close(dbp, dbp->flags)) != 0 && ret == 0)
		ret = t_ret;

	/*
	 * !!!
	 * At this point, the access-method specific information has been
	 * freed.  From now on, we can use the dbp, but not touch any
	 * access-method specific data.
	 */

	if (!reuse && dbp->locker != NULL) {
		/* We may have pending trade operations on this dbp. */
		if (txn == NULL)
			txn = dbp->cur_txn;
		if (IS_REAL_TXN(txn))
			__txn_remlock(env,
			     txn, &dbp->handle_lock, dbp->locker);

		/* We may be holding the handle lock; release it. */
		lreq.op = DB_LOCK_PUT_ALL;
		lreq.obj = NULL;
		if ((t_ret = __lock_vec(env,
		    dbp->locker, 0, &lreq, 1, NULL)) != 0 && ret == 0)
			ret = t_ret;

		if ((t_ret =
		     __lock_id_free(env, dbp->locker)) != 0 && ret == 0)
			ret = t_ret;
		dbp->locker = NULL;
		LOCK_INIT(dbp->handle_lock);
	}

	/*
	 * If this is a temporary file (un-named in-memory file), then
	 * discard the locker ID allocated as the fileid.
	 */
	if (LOCKING_ON(env) &&
	    F_ISSET(dbp, DB_AM_INMEM) && !dbp->preserve_fid &&
	    *(u_int32_t *)dbp->fileid != DB_LOCK_INVALIDID) {
		if ((t_ret = __lock_getlocker(env->lk_handle,
		     *(u_int32_t *)dbp->fileid, 0, &locker)) == 0)
			t_ret = __lock_id_free(env, locker);
		if (ret == 0)
			ret = t_ret;
	}

	if (reuse) {
		/*
		 * If we are reusing this dbp, then we're done now. Re-init
		 * the handle, preserving important flags, and then return.
		 * This code is borrowed from __db_init, which does more
		 * than we can do here.
		 */
		save_flags = F_ISSET(dbp, DB_AM_INMEM | DB_AM_TXN);

		if ((ret = __bam_db_create(dbp)) != 0)
			return (ret);
		if ((ret = __ham_db_create(dbp)) != 0)
			return (ret);
		if ((ret = __qam_db_create(dbp)) != 0)
			return (ret);

		/* Restore flags */
		dbp->flags = dbp->orig_flags | save_flags;

		if (FLD_ISSET(save_flags, DB_AM_INMEM)) {
			/*
			 * If this is inmem, then it may have a fileid
			 * even if it was never opened, and we need to
			 * clear out that fileid.
			 */
			memset(dbp->fileid, 0, sizeof(dbp->fileid));
			MAKE_INMEM(dbp);
		}
		return (ret);
	}

	dbp->type = DB_UNKNOWN;

	/*
	 * The thread mutex may have been invalidated in __dbreg_close_id if the
	 * fname refcount did not go to 0. If not, discard the thread mutex.
	 */
	if ((t_ret = __mutex_free(env, &dbp->mutex)) != 0 && ret == 0)
		ret = t_ret;

	/* Discard any memory allocated for the file and database names. */
	if (dbp->fname != NULL) {
		__os_free(dbp->env, dbp->fname);
		dbp->fname = NULL;
	}
	if (dbp->dname != NULL) {
		__os_free(dbp->env, dbp->dname);
		dbp->dname = NULL;
	}

	/* Discard any memory used to store returned data. */
	if (dbp->my_rskey.data != NULL)
		__os_free(dbp->env, dbp->my_rskey.data);
	if (dbp->my_rkey.data != NULL)
		__os_free(dbp->env, dbp->my_rkey.data);
	if (dbp->my_rdata.data != NULL)
		__os_free(dbp->env, dbp->my_rdata.data);

	/* For safety's sake;  we may refresh twice. */
	memset(&dbp->my_rskey, 0, sizeof(DBT));
	memset(&dbp->my_rkey, 0, sizeof(DBT));
	memset(&dbp->my_rdata, 0, sizeof(DBT));

	/* Clear out fields that normally get set during open. */
	memset(dbp->fileid, 0, sizeof(dbp->fileid));
	dbp->adj_fileid = 0;
	dbp->meta_pgno = 0;
	dbp->cur_locker = NULL;
	dbp->cur_txn = NULL;
	dbp->associate_locker = NULL;
	dbp->cl_id = 0;
	dbp->open_flags = 0;

	/*
	 * If we are being refreshed with a txn specified, then we need
	 * to make sure that we clear out the lock handle field, because
	 * releasing all the locks for this transaction will release this
	 * lock and we don't want close to stumble upon this handle and
	 * try to close it.
	 */
	if (txn != NULL)
		LOCK_INIT(dbp->handle_lock);

	/* Reset flags to whatever the user configured. */
	dbp->flags = dbp->orig_flags;

	return (ret);
}

/*
 * __db_disassociate --
 *	Destroy the association between a given secondary and its primary.
 */
static int
__db_disassociate(sdbp)
	DB *sdbp;
{
	DBC *dbc;
	int ret, t_ret;

	ret = 0;

	sdbp->s_callback = NULL;
	sdbp->s_primary = NULL;
	sdbp->get = sdbp->stored_get;
	sdbp->close = sdbp->stored_close;

	/*
	 * Complain, but proceed, if we have any active cursors.  (We're in
	 * the middle of a close, so there's really no turning back.)
	 */
	if (sdbp->s_refcnt != 1 ||
	    TAILQ_FIRST(&sdbp->active_queue) != NULL ||
	    TAILQ_FIRST(&sdbp->join_queue) != NULL) {
		__db_errx(sdbp->env,
    "Closing a primary DB while a secondary DB has active cursors is unsafe");
		ret = EINVAL;
	}
	sdbp->s_refcnt = 0;

	while ((dbc = TAILQ_FIRST(&sdbp->free_queue)) != NULL)
		if ((t_ret = __dbc_destroy(dbc)) != 0 && ret == 0)
			ret = t_ret;

	F_CLR(sdbp, DB_AM_SECONDARY);
	return (ret);
}

/*
 * __db_disassociate_foreign --
 *     Destroy the association between a given secondary and its foreign.
 */
static int
__db_disassociate_foreign(sdbp)
	DB *sdbp;
{
	DB *fdbp;
	DB_FOREIGN_INFO *f_info, *tmp;
	int ret;

	if (sdbp->s_foreign == NULL)
		return (0);
	if ((ret = __os_malloc(sdbp->env, sizeof(DB_FOREIGN_INFO), &tmp)) != 0)
		return (ret);

	fdbp = sdbp->s_foreign;
	ret = 0;
	f_info = LIST_FIRST(&fdbp->f_primaries);
	while (f_info != NULL) {
		tmp = LIST_NEXT(f_info, f_links);
		if (f_info ->dbp == sdbp) {
			LIST_REMOVE(f_info, f_links);
			__os_free(sdbp->env, f_info);
		}
		f_info = tmp;
	}

	return (ret);
}

/*
 * __db_log_page
 *	Log a meta-data or root page during a subdatabase create operation.
 *
 * PUBLIC: int __db_log_page __P((DB *, DB_TXN *, DB_LSN *, db_pgno_t, PAGE *));
 */
int
__db_log_page(dbp, txn, lsn, pgno, page)
	DB *dbp;
	DB_TXN *txn;
	DB_LSN *lsn;
	db_pgno_t pgno;
	PAGE *page;
{
	DBT page_dbt;
	DB_LSN new_lsn;
	int ret;

	if (!LOGGING_ON(dbp->env) || txn == NULL)
		return (0);

	memset(&page_dbt, 0, sizeof(page_dbt));
	page_dbt.size = dbp->pgsize;
	page_dbt.data = page;

	ret = __crdel_metasub_log(dbp, txn, &new_lsn, 0, pgno, &page_dbt, lsn);

	if (ret == 0)
		page->lsn = new_lsn;
	return (ret);
}

/*
 * __db_backup_name
 *	Create the backup file name for a given file.
 *
 * PUBLIC: int __db_backup_name __P((ENV *,
 * PUBLIC:     const char *, DB_TXN *, char **));
 */
#undef	BACKUP_PREFIX
#define	BACKUP_PREFIX	"__db."

#undef	MAX_INT_TO_HEX
#define	MAX_INT_TO_HEX	8

int
__db_backup_name(env, name, txn, backup)
	ENV *env;
	const char *name;
	DB_TXN *txn;
	char **backup;
{
	u_int32_t id;
	size_t len;
	int ret;
	char *p, *retp;

	*backup = NULL;

	/*
	 * Part of the name may be a full path, so we need to make sure that
	 * we allocate enough space for it, even in the case where we don't
	 * use the entire filename for the backup name.
	 */
	len = strlen(name) + strlen(BACKUP_PREFIX) + 2 * MAX_INT_TO_HEX + 1;
	if ((ret = __os_malloc(env, len, &retp)) != 0)
		return (ret);

	/*
	 * Create the name.  Backup file names are in one of 2 forms: in a
	 * transactional env "__db.TXNID.ID", where ID is a random number,
	 * and in any other env "__db.FILENAME".
	 *
	 * In addition, the name passed may contain an env-relative path.
	 * In that case, put the "__db." in the right place (in the last
	 * component of the pathname).
	 *
	 * There are four cases here:
	 *	1. simple path w/out transaction
	 *	2. simple path + transaction
	 *	3. multi-component path w/out transaction
	 *	4. multi-component path + transaction
	 */
	p = __db_rpath(name);
	if (IS_REAL_TXN(txn)) {
		__os_unique_id(env, &id);
		if (p == NULL)				/* Case 2. */
			snprintf(retp, len, "%s%x.%x",
			    BACKUP_PREFIX, txn->txnid, id);
		else					/* Case 4. */
			snprintf(retp, len, "%.*s%x.%x",
			    (int)(p - name) + 1, name, txn->txnid, id);
	} else {
		if (p == NULL)				/* Case 1. */
			snprintf(retp, len, "%s%s", BACKUP_PREFIX, name);
		else					/* Case 3. */
			snprintf(retp, len, "%.*s%s%s",
			    (int)(p - name) + 1, name, BACKUP_PREFIX, p + 1);
	}

	*backup = retp;
	return (0);
}

#ifdef CONFIG_TEST
/*
 * __db_testcopy
 *	Create a copy of all backup files and our "main" DB.
 *
 * PUBLIC: #ifdef CONFIG_TEST
 * PUBLIC: int __db_testcopy __P((ENV *, DB *, const char *));
 * PUBLIC: #endif
 */
int
__db_testcopy(env, dbp, name)
	ENV *env;
	DB *dbp;
	const char *name;
{
	DB_MPOOL *dbmp;
	DB_MPOOLFILE *mpf;

	DB_ASSERT(env, dbp != NULL || name != NULL);

	if (name == NULL) {
		dbmp = env->mp_handle;
		mpf = dbp->mpf;
		name = R_ADDR(dbmp->reginfo, mpf->mfp->path_off);
	}

	if (dbp != NULL && dbp->type == DB_QUEUE)
		return (__qam_testdocopy(dbp, name));
	else
#ifdef HAVE_PARTITION
	if (dbp != NULL && DB_IS_PARTITIONED(dbp))
		return (__part_testdocopy(dbp, name));
	else
#endif
		return (__db_testdocopy(env, name));
}

static int
__qam_testdocopy(dbp, name)
	DB *dbp;
	const char *name;
{
	DB_THREAD_INFO *ip;
	QUEUE_FILELIST *filelist, *fp;
	int ret;
	char buf[DB_MAXPATHLEN], *dir;

	filelist = NULL;
	if ((ret = __db_testdocopy(dbp->env, name)) != 0)
		return (ret);

	/* Call ENV_GET_THREAD_INFO to get a valid DB_THREAD_INFO */
	ENV_GET_THREAD_INFO(dbp->env, ip);
	if (dbp->mpf != NULL &&
	    (ret = __qam_gen_filelist(dbp, ip, &filelist)) != 0)
		goto done;

	if (filelist == NULL)
		return (0);
	dir = ((QUEUE *)dbp->q_internal)->dir;
	for (fp = filelist; fp->mpf != NULL; fp++) {
		snprintf(buf, sizeof(buf),
		    QUEUE_EXTENT, dir, PATH_SEPARATOR[0], name, fp->id);
		if ((ret = __db_testdocopy(dbp->env, buf)) != 0)
			return (ret);
	}

done:	__os_free(dbp->env, filelist);
	return (0);
}

/*
 * __db_testdocopy
 *	Create a copy of all backup files and our "main" DB.
 * PUBLIC: int __db_testdocopy __P((ENV *, const char *));
 */
int
__db_testdocopy(env, name)
	ENV *env;
	const char *name;
{
	size_t len;
	int dircnt, i, ret;
	char *copy, **namesp, *p, *real_name;

	dircnt = 0;
	copy = NULL;
	namesp = NULL;

	/* Create the real backing file name. */
	if ((ret = __db_appname(env,
	    DB_APP_DATA, name, NULL, &real_name)) != 0)
		return (ret);

	/*
	 * !!!
	 * There are tests that attempt to copy non-existent files.  I'd guess
	 * it's a testing bug, but I don't have time to figure it out.  Block
	 * the case here.
	 */
	if (__os_exists(env, real_name, NULL) != 0) {
		__os_free(env, real_name);
		return (0);
	}

	/*
	 * Copy the file itself.
	 *
	 * Allocate space for the file name, including adding an ".afterop" and
	 * trailing nul byte.
	 */
	len = strlen(real_name) + sizeof(".afterop");
	if ((ret = __os_malloc(env, len, &copy)) != 0)
		goto err;
	snprintf(copy, len, "%s.afterop", real_name);
	if ((ret = __db_makecopy(env, real_name, copy)) != 0)
		goto err;

	/*
	 * Get the directory path to call __os_dirlist().
	 */
	if ((p = __db_rpath(real_name)) != NULL)
		*p = '\0';
	if ((ret = __os_dirlist(env, real_name, 0, &namesp, &dircnt)) != 0)
		goto err;

	/*
	 * Walk the directory looking for backup files.  Backup file names in
	 * transactional environments are of the form:
	 *
	 *	BACKUP_PREFIX.TXNID.ID
	 */
	for (i = 0; i < dircnt; i++) {
		/* Check for a related backup file name. */
		if (strncmp(
		    namesp[i], BACKUP_PREFIX, sizeof(BACKUP_PREFIX) - 1) != 0)
			continue;
		p = namesp[i] + sizeof(BACKUP_PREFIX);
		p += strspn(p, "0123456789ABCDEFabcdef");
		if (*p != '.')
			continue;
		++p;
		p += strspn(p, "0123456789ABCDEFabcdef");
		if (*p != '\0')
			continue;

		/*
		 * Copy the backup file.
		 *
		 * Allocate space for the file name, including adding a
		 * ".afterop" and trailing nul byte.
		 */
		if (real_name != NULL) {
			__os_free(env, real_name);
			real_name = NULL;
		}
		if ((ret = __db_appname(env,
		    DB_APP_DATA, namesp[i], NULL, &real_name)) != 0)
			goto err;
		if (copy != NULL) {
			__os_free(env, copy);
			copy = NULL;
		}
		len = strlen(real_name) + sizeof(".afterop");
		if ((ret = __os_malloc(env, len, &copy)) != 0)
			goto err;
		snprintf(copy, len, "%s.afterop", real_name);
		if ((ret = __db_makecopy(env, real_name, copy)) != 0)
			goto err;
	}

err:	if (namesp != NULL)
		__os_dirfree(env, namesp, dircnt);
	if (copy != NULL)
		__os_free(env, copy);
	if (real_name != NULL)
		__os_free(env, real_name);
	return (ret);
}

static int
__db_makecopy(env, src, dest)
	ENV *env;
	const char *src, *dest;
{
	DB_FH *rfhp, *wfhp;
	size_t rcnt, wcnt;
	int ret;
	char *buf;

	rfhp = wfhp = NULL;

	if ((ret = __os_malloc(env, 64 * 1024, &buf)) != 0)
		goto err;

	if ((ret = __os_open(env, src, 0,
	    DB_OSO_RDONLY, DB_MODE_600, &rfhp)) != 0)
		goto err;
	if ((ret = __os_open(env, dest, 0,
	    DB_OSO_CREATE | DB_OSO_TRUNC, DB_MODE_600, &wfhp)) != 0)
		goto err;

	for (;;) {
		if ((ret =
		    __os_read(env, rfhp, buf, sizeof(buf), &rcnt)) != 0)
			goto err;
		if (rcnt == 0)
			break;
		if ((ret =
		    __os_write(env, wfhp, buf, sizeof(buf), &wcnt)) != 0)
			goto err;
	}

	if (0) {
err:		__db_err(env, ret, "__db_makecopy: %s -> %s", src, dest);
	}

	if (buf != NULL)
		__os_free(env, buf);
	if (rfhp != NULL)
		(void)__os_closehandle(env, rfhp);
	if (wfhp != NULL)
		(void)__os_closehandle(env, wfhp);
	return (ret);
}
#endif
