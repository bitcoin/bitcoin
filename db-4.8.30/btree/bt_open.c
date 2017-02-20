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
 * This code is derived from software contributed to Berkeley by
 * Mike Olson.
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
#include "dbinc/crypto.h"
#include "dbinc/db_page.h"
#include "dbinc/db_swap.h"
#include "dbinc/btree.h"
#include "dbinc/lock.h"
#include "dbinc/log.h"
#include "dbinc/mp.h"
#include "dbinc/partition.h"
#include "dbinc/fop.h"

static void __bam_init_meta __P((DB *, BTMETA *, db_pgno_t, DB_LSN *));

/*
 * __bam_open --
 *	Open a btree.
 *
 * PUBLIC: int __bam_open __P((DB *, DB_THREAD_INFO *,
 * PUBLIC:      DB_TXN *, const char *, db_pgno_t, u_int32_t));
 */
int
__bam_open(dbp, ip, txn, name, base_pgno, flags)
	DB *dbp;
	DB_THREAD_INFO *ip;
	DB_TXN *txn;
	const char *name;
	db_pgno_t base_pgno;
	u_int32_t flags;
{
	BTREE *t;

	COMPQUIET(name, NULL);
	t = dbp->bt_internal;

	/*
	 * We don't permit the user to specify a prefix routine if they didn't
	 * also specify a comparison routine, they can't know enough about our
	 * comparison routine to get it right.
	 */
	if (t->bt_compare == __bam_defcmp && t->bt_prefix != __bam_defpfx) {
		__db_errx(dbp->env,
"prefix comparison may not be specified for default comparison routine");
		return (EINVAL);
	}

	/*
	 * Verify that the bt_minkey value specified won't cause the
	 * calculation of ovflsize to underflow [#2406] for this pagesize.
	 */
	if (B_MINKEY_TO_OVFLSIZE(dbp, t->bt_minkey, dbp->pgsize) >
	    B_MINKEY_TO_OVFLSIZE(dbp, DEFMINKEYPAGE, dbp->pgsize)) {
		__db_errx(dbp->env,
		    "bt_minkey value of %lu too high for page size of %lu",
		    (u_long)t->bt_minkey, (u_long)dbp->pgsize);
		return (EINVAL);
	}

	/* Start up the tree. */
	return (__bam_read_root(dbp, ip, txn, base_pgno, flags));
}

/*
 * __bam_metachk --
 *
 * PUBLIC: int __bam_metachk __P((DB *, const char *, BTMETA *));
 */
int
__bam_metachk(dbp, name, btm)
	DB *dbp;
	const char *name;
	BTMETA *btm;
{
	ENV *env;
	u_int32_t vers;
	int ret;

	env = dbp->env;

	/*
	 * At this point, all we know is that the magic number is for a Btree.
	 * Check the version, the database may be out of date.
	 */
	vers = btm->dbmeta.version;
	if (F_ISSET(dbp, DB_AM_SWAP))
		M_32_SWAP(vers);
	switch (vers) {
	case 6:
	case 7:
		__db_errx(env,
		    "%s: btree version %lu requires a version upgrade",
		    name, (u_long)vers);
		return (DB_OLD_VERSION);
	case 8:
	case 9:
		break;
	default:
		__db_errx(env,
		    "%s: unsupported btree version: %lu", name, (u_long)vers);
		return (EINVAL);
	}

	/* Swap the page if we need to. */
	if (F_ISSET(dbp, DB_AM_SWAP) &&
	    (ret = __bam_mswap(env, (PAGE *)btm)) != 0)
		return (ret);

	/*
	 * Check application info against metadata info, and set info, flags,
	 * and type based on metadata info.
	 */
	if ((ret =
	    __db_fchk(env, "DB->open", btm->dbmeta.flags, BTM_MASK)) != 0)
		return (ret);

	if (F_ISSET(&btm->dbmeta, BTM_RECNO)) {
		if (dbp->type == DB_BTREE)
			goto wrong_type;
		dbp->type = DB_RECNO;
		DB_ILLEGAL_METHOD(dbp, DB_OK_RECNO);
	} else {
		if (dbp->type == DB_RECNO)
			goto wrong_type;
		dbp->type = DB_BTREE;
		DB_ILLEGAL_METHOD(dbp, DB_OK_BTREE);
	}

	if (F_ISSET(&btm->dbmeta, BTM_DUP))
		F_SET(dbp, DB_AM_DUP);
	else
		if (F_ISSET(dbp, DB_AM_DUP)) {
			__db_errx(env,
		"%s: DB_DUP specified to open method but not set in database",
			    name);
			return (EINVAL);
		}

	if (F_ISSET(&btm->dbmeta, BTM_RECNUM)) {
		if (dbp->type != DB_BTREE)
			goto wrong_type;
		F_SET(dbp, DB_AM_RECNUM);

		if ((ret = __db_fcchk(env,
		    "DB->open", dbp->flags, DB_AM_DUP, DB_AM_RECNUM)) != 0)
			return (ret);
	} else
		if (F_ISSET(dbp, DB_AM_RECNUM)) {
			__db_errx(env,
	    "%s: DB_RECNUM specified to open method but not set in database",
			    name);
			return (EINVAL);
		}

	if (F_ISSET(&btm->dbmeta, BTM_FIXEDLEN)) {
		if (dbp->type != DB_RECNO)
			goto wrong_type;
		F_SET(dbp, DB_AM_FIXEDLEN);
	} else
		if (F_ISSET(dbp, DB_AM_FIXEDLEN)) {
			__db_errx(env,
	"%s: DB_FIXEDLEN specified to open method but not set in database",
			    name);
			return (EINVAL);
		}

	if (F_ISSET(&btm->dbmeta, BTM_RENUMBER)) {
		if (dbp->type != DB_RECNO)
			goto wrong_type;
		F_SET(dbp, DB_AM_RENUMBER);
	} else
		if (F_ISSET(dbp, DB_AM_RENUMBER)) {
			__db_errx(env,
	    "%s: DB_RENUMBER specified to open method but not set in database",
			    name);
			return (EINVAL);
		}

	if (F_ISSET(&btm->dbmeta, BTM_SUBDB))
		F_SET(dbp, DB_AM_SUBDB);
	else
		if (F_ISSET(dbp, DB_AM_SUBDB)) {
			__db_errx(env,
	    "%s: multiple databases specified but not supported by file",
			    name);
			return (EINVAL);
		}

	if (F_ISSET(&btm->dbmeta, BTM_DUPSORT)) {
		if (dbp->dup_compare == NULL)
			dbp->dup_compare = __bam_defcmp;
		F_SET(dbp, DB_AM_DUPSORT);
	} else
		if (dbp->dup_compare != NULL) {
			__db_errx(env,
		"%s: duplicate sort specified but not supported in database",
			    name);
			return (EINVAL);
		}

#ifdef HAVE_COMPRESSION
	if (F_ISSET(&btm->dbmeta, BTM_COMPRESS)) {
		F_SET(dbp, DB_AM_COMPRESS);
		if ((BTREE *)dbp->bt_internal != NULL &&
		    !DB_IS_COMPRESSED(dbp) &&
		    (ret = __bam_set_bt_compress(dbp, NULL, NULL)) != 0)
			return (ret);
	} else {
		if ((BTREE *)dbp->bt_internal != NULL &&
		    DB_IS_COMPRESSED(dbp)) {
			__db_errx(env,
	"%s: compresssion specified to open method but not set in database",
			    name);
			return (EINVAL);
		}
	}
#else
	if (F_ISSET(&btm->dbmeta, BTM_COMPRESS)) {
		__db_errx(env,
			"%s: compression support has not been compiled in",
			name);
		return (EINVAL);
	}
#endif

	/* Set the page size. */
	dbp->pgsize = btm->dbmeta.pagesize;

	/* Copy the file's ID. */
	memcpy(dbp->fileid, btm->dbmeta.uid, DB_FILE_ID_LEN);

	return (0);

wrong_type:
	if (dbp->type == DB_BTREE)
		__db_errx(env,
		    "open method type is Btree, database type is Recno");
	else
		__db_errx(env,
		    "open method type is Recno, database type is Btree");
	return (EINVAL);
}

/*
 * __bam_read_root --
 *	Read the root page and check a tree.
 *
 * PUBLIC: int __bam_read_root __P((DB *,
 * PUBLIC:      DB_THREAD_INFO *, DB_TXN *, db_pgno_t, u_int32_t));
 */
int
__bam_read_root(dbp, ip, txn, base_pgno, flags)
	DB *dbp;
	DB_THREAD_INFO *ip;
	DB_TXN *txn;
	db_pgno_t base_pgno;
	u_int32_t flags;
{
	BTMETA *meta;
	BTREE *t;
	DBC *dbc;
	DB_LOCK metalock;
	DB_MPOOLFILE *mpf;
	int ret, t_ret;

	COMPQUIET(flags, 0);

	meta = NULL;
	t = dbp->bt_internal;
	LOCK_INIT(metalock);
	mpf = dbp->mpf;
	ret = 0;

	/* Get a cursor.  */
	if ((ret = __db_cursor(dbp, ip, txn, &dbc, 0)) != 0)
		return (ret);

	/* Get the metadata page. */
	if ((ret =
	    __db_lget(dbc, 0, base_pgno, DB_LOCK_READ, 0, &metalock)) != 0)
		goto err;
	if ((ret = __memp_fget(mpf, &base_pgno, ip, dbc->txn, 0, &meta)) != 0)
		goto err;

	/*
	 * If the magic number is set, the tree has been created.  Correct
	 * any fields that may not be right.  Note, all of the local flags
	 * were set by DB->open.
	 *
	 * Otherwise, we'd better be in recovery or abort, in which case the
	 * metadata page will be created/initialized elsewhere.
	 */
	if (meta->dbmeta.magic == DB_BTREEMAGIC) {
		t->bt_minkey = meta->minkey;
		t->re_pad = (int)meta->re_pad;
		t->re_len = meta->re_len;

		t->bt_meta = base_pgno;
		t->bt_root = meta->root;
#ifndef HAVE_FTRUNCATE
		if (PGNO(meta) == PGNO_BASE_MD &&
		    !F_ISSET(dbp, DB_AM_RECOVER) && !IS_VERSION(dbp, meta))
			__memp_set_last_pgno(mpf, meta->dbmeta.last_pgno);
#endif
	} else {
		DB_ASSERT(dbp->env,
		    IS_RECOVERING(dbp->env) || F_ISSET(dbp, DB_AM_RECOVER));
	}

	/*
	 * !!!
	 * If creating a subdatabase, we've already done an insert when
	 * we put the subdatabase's entry into the master database, so
	 * our last-page-inserted value is wrongly initialized for the
	 * master database, not the subdatabase we're creating.  I'm not
	 * sure where the *right* place to clear this value is, it's not
	 * intuitively obvious that it belongs here.
	 */
	t->bt_lpgno = PGNO_INVALID;

err:	/* Put the metadata page back. */
	if (meta != NULL && (t_ret = __memp_fput(mpf,
	    ip, meta, dbc->priority)) != 0 && ret == 0)
		ret = t_ret;
	if ((t_ret = __LPUT(dbc, metalock)) != 0 && ret == 0)
		ret = t_ret;

	if ((t_ret = __dbc_close(dbc)) != 0 && ret == 0)
		ret = t_ret;
	return (ret);
}

/*
 * __bam_init_meta --
 *
 * Initialize a btree meta-data page.  The following fields may need
 * to be updated later: last_pgno, root.
 */
static void
__bam_init_meta(dbp, meta, pgno, lsnp)
	DB *dbp;
	BTMETA *meta;
	db_pgno_t pgno;
	DB_LSN *lsnp;
{
	BTREE *t;
#ifdef HAVE_PARTITION
	DB_PARTITION *part;
#endif
	ENV *env;

	env = dbp->env;
	t = dbp->bt_internal;

	memset(meta, 0, sizeof(BTMETA));
	meta->dbmeta.lsn = *lsnp;
	meta->dbmeta.pgno = pgno;
	meta->dbmeta.magic = DB_BTREEMAGIC;
	meta->dbmeta.version = DB_BTREEVERSION;
	meta->dbmeta.pagesize = dbp->pgsize;
	if (F_ISSET(dbp, DB_AM_CHKSUM))
		FLD_SET(meta->dbmeta.metaflags, DBMETA_CHKSUM);
	if (F_ISSET(dbp, DB_AM_ENCRYPT)) {
		meta->dbmeta.encrypt_alg = env->crypto_handle->alg;
		DB_ASSERT(env, meta->dbmeta.encrypt_alg != 0);
		meta->crypto_magic = meta->dbmeta.magic;
	}
	meta->dbmeta.type = P_BTREEMETA;
	meta->dbmeta.free = PGNO_INVALID;
	meta->dbmeta.last_pgno = pgno;
	if (F_ISSET(dbp, DB_AM_DUP))
		F_SET(&meta->dbmeta, BTM_DUP);
	if (F_ISSET(dbp, DB_AM_FIXEDLEN))
		F_SET(&meta->dbmeta, BTM_FIXEDLEN);
	if (F_ISSET(dbp, DB_AM_RECNUM))
		F_SET(&meta->dbmeta, BTM_RECNUM);
	if (F_ISSET(dbp, DB_AM_RENUMBER))
		F_SET(&meta->dbmeta, BTM_RENUMBER);
	if (F_ISSET(dbp, DB_AM_SUBDB))
		F_SET(&meta->dbmeta, BTM_SUBDB);
	if (dbp->dup_compare != NULL)
		F_SET(&meta->dbmeta, BTM_DUPSORT);
#ifdef HAVE_COMPRESSION
	if (DB_IS_COMPRESSED(dbp))
		F_SET(&meta->dbmeta, BTM_COMPRESS);
#endif
	if (dbp->type == DB_RECNO)
		F_SET(&meta->dbmeta, BTM_RECNO);
	memcpy(meta->dbmeta.uid, dbp->fileid, DB_FILE_ID_LEN);

	meta->minkey = t->bt_minkey;
	meta->re_len = t->re_len;
	meta->re_pad = (u_int32_t)t->re_pad;

#ifdef HAVE_PARTITION
	if ((part = dbp->p_internal) != NULL) {
		meta->dbmeta.nparts = part->nparts;
		if (F_ISSET(part, PART_CALLBACK))
			FLD_SET(meta->dbmeta.metaflags, DBMETA_PART_CALLBACK);
		if (F_ISSET(part, PART_RANGE))
			FLD_SET(meta->dbmeta.metaflags, DBMETA_PART_RANGE);
	}
#endif
}

/*
 * __bam_new_file --
 * Create the necessary pages to begin a new database file.
 *
 * This code appears more complex than it is because of the two cases (named
 * and unnamed).  The way to read the code is that for each page being created,
 * there are three parts: 1) a "get page" chunk (which either uses malloc'd
 * memory or calls __memp_fget), 2) the initialization, and 3) the "put page"
 * chunk which either does a fop write or an __memp_fput.
 *
 * PUBLIC: int __bam_new_file __P((DB *,
 * PUBLIC:      DB_THREAD_INFO *, DB_TXN *, DB_FH *, const char *));
 */
int
__bam_new_file(dbp, ip, txn, fhp, name)
	DB *dbp;
	DB_THREAD_INFO *ip;
	DB_TXN *txn;
	DB_FH *fhp;
	const char *name;
{
	BTMETA *meta;
	DBT pdbt;
	DB_LSN lsn;
	DB_MPOOLFILE *mpf;
	DB_PGINFO pginfo;
	ENV *env;
	PAGE *root;
	db_pgno_t pgno;
	int ret, t_ret;
	void *buf;

	env = dbp->env;
	mpf = dbp->mpf;
	root = NULL;
	meta = NULL;
	buf = NULL;

	if (F_ISSET(dbp, DB_AM_INMEM)) {
		/* Build the meta-data page. */
		pgno = PGNO_BASE_MD;
		if ((ret = __memp_fget(mpf, &pgno, ip, txn,
		    DB_MPOOL_CREATE | DB_MPOOL_DIRTY, &meta)) != 0)
			return (ret);
		LSN_NOT_LOGGED(lsn);
		__bam_init_meta(dbp, meta, PGNO_BASE_MD, &lsn);
		meta->root = 1;
		meta->dbmeta.last_pgno = 1;
		if ((ret =
		    __db_log_page(dbp, txn, &lsn, pgno, (PAGE *)meta)) != 0)
			goto err;
		ret = __memp_fput(mpf, ip, meta, dbp->priority);
		meta = NULL;
		if (ret != 0)
			goto err;

		/* Build the root page. */
		pgno = 1;
		if ((ret = __memp_fget(mpf, &pgno,
		    ip, txn, DB_MPOOL_CREATE, &root)) != 0)
			goto err;
		P_INIT(root, dbp->pgsize, 1, PGNO_INVALID, PGNO_INVALID,
		    LEAFLEVEL, dbp->type == DB_RECNO ? P_LRECNO : P_LBTREE);
		LSN_NOT_LOGGED(root->lsn);
		if ((ret =
		    __db_log_page(dbp, txn, &root->lsn, pgno, root)) != 0)
			goto err;
		ret = __memp_fput(mpf, ip, root, dbp->priority);
		root = NULL;
		if (ret != 0)
			goto err;
	} else {
		memset(&pdbt, 0, sizeof(pdbt));

		/* Build the meta-data page. */
		pginfo.db_pagesize = dbp->pgsize;
		pginfo.flags =
		    F_ISSET(dbp, (DB_AM_CHKSUM | DB_AM_ENCRYPT | DB_AM_SWAP));
		pginfo.type = dbp->type;
		pdbt.data = &pginfo;
		pdbt.size = sizeof(pginfo);
		if ((ret = __os_calloc(env, 1, dbp->pgsize, &buf)) != 0)
			return (ret);
		meta = (BTMETA *)buf;
		LSN_NOT_LOGGED(lsn);
		__bam_init_meta(dbp, meta, PGNO_BASE_MD, &lsn);
		meta->root = 1;
		meta->dbmeta.last_pgno = 1;
		if ((ret = __db_pgout(
		    dbp->dbenv, PGNO_BASE_MD, meta, &pdbt)) != 0)
			goto err;
		if ((ret = __fop_write(env, txn, name, dbp->dirname,
		    DB_APP_DATA, fhp,
		    dbp->pgsize, 0, 0, buf, dbp->pgsize, 1, F_ISSET(
		    dbp, DB_AM_NOT_DURABLE) ? DB_LOG_NOT_DURABLE : 0)) != 0)
			goto err;
		meta = NULL;

		/* Build the root page. */
#ifdef DIAGNOSTIC
		memset(buf, CLEAR_BYTE, dbp->pgsize);
#endif
		root = (PAGE *)buf;
		P_INIT(root, dbp->pgsize, 1, PGNO_INVALID, PGNO_INVALID,
		    LEAFLEVEL, dbp->type == DB_RECNO ? P_LRECNO : P_LBTREE);
		LSN_NOT_LOGGED(root->lsn);
		if ((ret =
		    __db_pgout(dbp->dbenv, root->pgno, root, &pdbt)) != 0)
			goto err;
		if ((ret =
		    __fop_write(env, txn, name, dbp->dirname, DB_APP_DATA,
		    fhp, dbp->pgsize, 1, 0, buf, dbp->pgsize, 1, F_ISSET(
		    dbp, DB_AM_NOT_DURABLE) ? DB_LOG_NOT_DURABLE : 0)) != 0)
			goto err;
		root = NULL;
	}

err:	if (buf != NULL)
		__os_free(env, buf);
	else {
		if (meta != NULL &&
		    (t_ret = __memp_fput(mpf, ip,
		    meta, dbp->priority)) != 0 && ret == 0)
			ret = t_ret;
		if (root != NULL &&
		    (t_ret = __memp_fput(mpf, ip,
		    root, dbp->priority)) != 0 && ret == 0)
			ret = t_ret;
	}
	return (ret);
}

/*
 * __bam_new_subdb --
 *	Create a metadata page and a root page for a new btree.
 *
 * PUBLIC: int __bam_new_subdb __P((DB *, DB *, DB_THREAD_INFO *, DB_TXN *));
 */
int
__bam_new_subdb(mdbp, dbp, ip, txn)
	DB *mdbp, *dbp;
	DB_THREAD_INFO *ip;
	DB_TXN *txn;
{
	BTMETA *meta;
	DBC *dbc;
	DB_LOCK metalock;
	DB_LSN lsn;
	DB_MPOOLFILE *mpf;
	ENV *env;
	PAGE *root;
	int ret, t_ret;

	env = mdbp->env;
	mpf = mdbp->mpf;
	dbc = NULL;
	meta = NULL;
	root = NULL;

	if ((ret = __db_cursor(mdbp, ip, txn,
	    &dbc, CDB_LOCKING(env) ?  DB_WRITECURSOR : 0)) != 0)
		return (ret);

	/* Get, and optionally create the metadata page. */
	if ((ret = __db_lget(dbc,
	    0, dbp->meta_pgno, DB_LOCK_WRITE, 0, &metalock)) != 0)
		goto err;
	if ((ret = __memp_fget(mpf, &dbp->meta_pgno,
	    ip, txn, DB_MPOOL_CREATE, &meta)) != 0)
		goto err;

	/* Build meta-data page. */
	lsn = meta->dbmeta.lsn;
	__bam_init_meta(dbp, meta, dbp->meta_pgno, &lsn);
	if ((ret = __db_log_page(mdbp,
	    txn, &meta->dbmeta.lsn, dbp->meta_pgno, (PAGE *)meta)) != 0)
		goto err;

	/* Create and initialize a root page. */
	if ((ret = __db_new(dbc,
	    dbp->type == DB_RECNO ? P_LRECNO : P_LBTREE, NULL, &root)) != 0)
		goto err;
	root->level = LEAFLEVEL;

	if (DBENV_LOGGING(env) &&
#if !defined(DEBUG_WOP)
	    txn != NULL &&
#endif

	    (ret = __bam_root_log(mdbp, txn, &meta->dbmeta.lsn, 0,
	    meta->dbmeta.pgno, root->pgno, &meta->dbmeta.lsn)) != 0)
		goto err;

	meta->root = root->pgno;
	if ((ret =
	    __db_log_page(mdbp, txn, &root->lsn, root->pgno, root)) != 0)
		goto err;

	/* Release the metadata and root pages. */
	if ((ret = __memp_fput(mpf, ip, meta, dbc->priority)) != 0)
		goto err;
	meta = NULL;
	if ((ret = __memp_fput(mpf, ip, root, dbc->priority)) != 0)
		goto err;
	root = NULL;
err:
	if (meta != NULL)
		if ((t_ret = __memp_fput(mpf, ip,
		meta, dbc->priority)) != 0 && ret == 0)
			ret = t_ret;
	if (root != NULL)
		if ((t_ret = __memp_fput(mpf, ip,
		root, dbc->priority)) != 0 && ret == 0)
			ret = t_ret;
	if ((t_ret = __LPUT(dbc, metalock)) != 0 && ret == 0)
		ret = t_ret;
	if (dbc != NULL)
		if ((t_ret = __dbc_close(dbc)) != 0 && ret == 0)
			ret = t_ret;
	return (ret);
}
