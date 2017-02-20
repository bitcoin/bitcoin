/*-
 * See the file LICENSE for redistribution information.
 *
 * Copyright (c) 1996-2009 Oracle.  All rights reserved.
 *
 * $Id$
 */

#include "db_config.h"

#include "db_int.h"
#include "dbinc/db_page.h"
#include "dbinc/db_swap.h"
#include "dbinc/btree.h"
#include "dbinc/crypto.h"
#include "dbinc/hmac.h"
#include "dbinc/fop.h"
#include "dbinc/hash.h"
#include "dbinc/lock.h"
#include "dbinc/log.h"
#include "dbinc/mp.h"
#include "dbinc/qam.h"
#include "dbinc/txn.h"

/*
 * __db_open --
 *	DB->open method.
 *
 * This routine gets called in three different ways:
 *
 * 1. It can be called to open a file/database.  In this case, subdb will
 *    be NULL and meta_pgno will be PGNO_BASE_MD.
 * 2. It can be called to open a subdatabase during normal operation.  In
 *    this case, name and subname will both be non-NULL and meta_pgno will
 *    be PGNO_BASE_MD (also PGNO_INVALID).
 * 3. It can be called to open an in-memory database (name == NULL;
 *    subname = name).
 * 4. It can be called during recovery to open a file/database, in which case
 *    name will be non-NULL, subname will be NULL, and meta-pgno will be
 *    PGNO_BASE_MD.
 * 5. It can be called during recovery to open a subdatabase, in which case
 *    name will be non-NULL, subname may be NULL and meta-pgno will be
 *    a valid pgno (i.e., not PGNO_BASE_MD).
 * 6. It can be called during recovery to open an in-memory database.
 *
 * PUBLIC: int __db_open __P((DB *, DB_THREAD_INFO *, DB_TXN *,
 * PUBLIC:     const char *, const char *, DBTYPE, u_int32_t, int, db_pgno_t));
 */
int
__db_open(dbp, ip, txn, fname, dname, type, flags, mode, meta_pgno)
	DB *dbp;
	DB_THREAD_INFO *ip;
	DB_TXN *txn;
	const char *fname, *dname;
	DBTYPE type;
	u_int32_t flags;
	int mode;
	db_pgno_t meta_pgno;
{
	DB *tdbp;
	ENV *env;
	int ret;
	u_int32_t id;

	env = dbp->env;
	id = TXN_INVALID;

	/*
	 * We must flush any existing pages before truncating the file
	 * since they could age out of mpool and overwrite new pages.
	 */
	if (LF_ISSET(DB_TRUNCATE)) {
		if ((ret = __db_create_internal(&tdbp, dbp->env, 0)) != 0)
			goto err;
		ret = __db_open(tdbp, ip, txn, fname, dname, DB_UNKNOWN,
		     DB_NOERROR | (flags &  ~(DB_TRUNCATE|DB_CREATE)),
		     mode, meta_pgno);
		if (ret == 0)
			ret = __memp_ftruncate(tdbp->mpf, txn, ip, 0, 0);
		(void)__db_close(tdbp, txn, DB_NOSYNC);
		if (ret != 0 && ret != ENOENT && ret != EINVAL)
			goto err;
		ret = 0;
	}

	DB_TEST_RECOVERY(dbp, DB_TEST_PREOPEN, ret, fname);

	/*
	 * If the environment was configured with threads, the DB handle
	 * must also be free-threaded, so we force the DB_THREAD flag on.
	 * (See SR #2033 for why this is a requirement--recovery needs
	 * to be able to grab a dbp using __db_fileid_to_dbp, and it has
	 * no way of knowing which dbp goes with which thread, so whichever
	 * one it finds has to be usable in any of them.)
	 */
	if (F_ISSET(env, ENV_THREAD))
		LF_SET(DB_THREAD);

	/* Convert any DB->open flags. */
	if (LF_ISSET(DB_RDONLY))
		F_SET(dbp, DB_AM_RDONLY);
	if (LF_ISSET(DB_READ_UNCOMMITTED))
		F_SET(dbp, DB_AM_READ_UNCOMMITTED);

	if (IS_REAL_TXN(txn))
		F_SET(dbp, DB_AM_TXN);

	/* Fill in the type. */
	dbp->type = type;

	/*
	 * If both fname and subname are NULL, it's always a create, so make
	 * sure that we have both DB_CREATE and a type specified.  It would
	 * be nice if this checking were done in __db_open where most of the
	 * interface checking is done, but this interface (__db_dbopen) is
	 * used by the recovery and limbo system, so we need to safeguard
	 * this interface as well.
	 */
	if (fname == NULL) {
		if (dbp->p_internal != NULL) {
			__db_errx(env,
		    "Partitioned databases may not be in memory.");
			return (ENOENT);
		}
		if (dname == NULL) {
			if (!LF_ISSET(DB_CREATE)) {
				__db_errx(env,
			    "DB_CREATE must be specified to create databases.");
				return (ENOENT);
			}

			F_SET(dbp, DB_AM_INMEM);
			F_SET(dbp, DB_AM_CREATED);

			if (dbp->type == DB_UNKNOWN) {
				__db_errx(env,
				    "DBTYPE of unknown without existing file");
				return (EINVAL);
			}

			if (dbp->pgsize == 0)
				dbp->pgsize = DB_DEF_IOSIZE;

			/*
			 * If the file is a temporary file and we're
			 * doing locking, then we have to create a
			 * unique file ID.  We can't use our normal
			 * dev/inode pair (or whatever this OS uses
			 * in place of dev/inode pairs) because no
			 * backing file will be created until the
			 * mpool cache is filled forcing the buffers
			 * to disk.  Grab a random locker ID to use
			 * as a file ID.  The created ID must never
			 * match a potential real file ID -- we know
			 * it won't because real file IDs contain a
			 * time stamp after the dev/inode pair, and
			 * we're simply storing a 4-byte value.

			 * !!!
			 * Store the locker in the file id structure
			 * -- we can get it from there as necessary,
			 * and it saves having two copies.
			*/
			if (LOCKING_ON(env) && (ret = __lock_id(env,
			    (u_int32_t *)dbp->fileid, NULL)) != 0)
				return (ret);
		} else
			MAKE_INMEM(dbp);

		/*
		 * Normally we would do handle locking here, however, with
		 * in-memory files, we cannot do any database manipulation
		 * until the mpool is open, so it happens later.
		 */
	} else if (dname == NULL && meta_pgno == PGNO_BASE_MD) {
		/* Open/create the underlying file.  Acquire locks. */
		if ((ret = __fop_file_setup(dbp, ip,
		    txn, fname, mode, flags, &id)) != 0)
			return (ret);
	} else {
		if (dbp->p_internal != NULL) {
			__db_errx(env,
    "Partitioned databases may not be included with multiple databases.");
			return (ENOENT);
		}
		if ((ret = __fop_subdb_setup(dbp, ip,
		    txn, fname, dname, mode, flags)) != 0)
			return (ret);
		meta_pgno = dbp->meta_pgno;
	}

	/* Set up the underlying environment. */
	if ((ret = __env_setup(dbp, txn, fname, dname, id, flags)) != 0)
		return (ret);

	/* For in-memory databases, we now need to open/create the database. */
	if (F_ISSET(dbp, DB_AM_INMEM)) {
		if (dname == NULL)
			ret = __db_new_file(dbp, ip, txn, NULL, NULL);
		else {
			id = TXN_INVALID;
			if ((ret = __fop_file_setup(dbp, ip,
			    txn, dname, mode, flags, &id)) == 0 &&
			    DBENV_LOGGING(env) && !F_ISSET(dbp, DB_AM_RECOVER)
#if !defined(DEBUG_ROP) && !defined(DEBUG_WOP) && !defined(DIAGNOSTIC)
			    && txn != NULL
#endif
#if !defined(DEBUG_ROP)
			    && !F_ISSET(dbp, DB_AM_RDONLY)
#endif
			)
				ret = __dbreg_log_id(dbp,
				    txn, dbp->log_filename->id, 1);
		}
		if (ret != 0)
			goto err;
	}

	switch (dbp->type) {
		case DB_BTREE:
			ret = __bam_open(dbp, ip, txn, fname, meta_pgno, flags);
			break;
		case DB_HASH:
			ret = __ham_open(dbp, ip, txn, fname, meta_pgno, flags);
			break;
		case DB_RECNO:
			ret = __ram_open(dbp, ip, txn, fname, meta_pgno, flags);
			break;
		case DB_QUEUE:
			ret = __qam_open(
			    dbp, ip, txn, fname, meta_pgno, mode, flags);
			break;
		case DB_UNKNOWN:
			return (
			    __db_unknown_type(env, "__db_dbopen", dbp->type));
	}
	if (ret != 0)
		goto err;

#ifdef HAVE_PARTITION
	if (dbp->p_internal != NULL && (ret =
	    __partition_open(dbp, ip, txn, fname, type, flags, mode, 1)) != 0)
		goto err;
#endif
	DB_TEST_RECOVERY(dbp, DB_TEST_POSTOPEN, ret, fname);

	/*
	 * Temporary files don't need handle locks, so we only have to check
	 * for a handle lock downgrade or lockevent in the case of named
	 * files.
	 */
	if (!F_ISSET(dbp, DB_AM_RECOVER) && (fname != NULL || dname != NULL) &&
	    LOCK_ISSET(dbp->handle_lock)) {
		if (IS_REAL_TXN(txn))
			ret = __txn_lockevent(env,
			    txn, dbp, &dbp->handle_lock, dbp->locker);
		else if (LOCKING_ON(env))
			/* Trade write handle lock for read handle lock. */
			ret = __lock_downgrade(env,
			    &dbp->handle_lock, DB_LOCK_READ, 0);
	}
DB_TEST_RECOVERY_LABEL
err:
	return (ret);
}

/*
 * __db_get_open_flags --
 *	Accessor for flags passed into DB->open call
 *
 * PUBLIC: int __db_get_open_flags __P((DB *, u_int32_t *));
 */
int
__db_get_open_flags(dbp, flagsp)
	DB *dbp;
	u_int32_t *flagsp;
{
	DB_ILLEGAL_BEFORE_OPEN(dbp, "DB->get_open_flags");

	*flagsp = dbp->open_flags;
	return (0);
}

/*
 * __db_new_file --
 *	Create a new database file.
 *
 * PUBLIC: int __db_new_file __P((DB *,
 * PUBLIC:      DB_THREAD_INFO *, DB_TXN *, DB_FH *, const char *));
 */
int
__db_new_file(dbp, ip, txn, fhp, name)
	DB *dbp;
	DB_THREAD_INFO *ip;
	DB_TXN *txn;
	DB_FH *fhp;
	const char *name;
{
	int ret;

	switch (dbp->type) {
	case DB_BTREE:
	case DB_RECNO:
		ret = __bam_new_file(dbp, ip, txn, fhp, name);
		break;
	case DB_HASH:
		ret = __ham_new_file(dbp, ip, txn, fhp, name);
		break;
	case DB_QUEUE:
		ret = __qam_new_file(dbp, ip, txn, fhp, name);
		break;
	case DB_UNKNOWN:
	default:
		__db_errx(dbp->env,
		    "%s: Invalid type %d specified", name, dbp->type);
		ret = EINVAL;
		break;
	}

	DB_TEST_RECOVERY(dbp, DB_TEST_POSTLOGMETA, ret, name);
	/* Sync the file in preparation for moving it into place. */
	if (ret == 0 && fhp != NULL)
		ret = __os_fsync(dbp->env, fhp);

	DB_TEST_RECOVERY(dbp, DB_TEST_POSTSYNC, ret, name);

DB_TEST_RECOVERY_LABEL
	return (ret);
}

/*
 * __db_init_subdb --
 *	Initialize the dbp for a subdb.
 *
 * PUBLIC: int __db_init_subdb __P((DB *,
 * PUBLIC:       DB *, const char *, DB_THREAD_INFO *, DB_TXN *));
 */
int
__db_init_subdb(mdbp, dbp, name, ip, txn)
	DB *mdbp, *dbp;
	const char *name;
	DB_THREAD_INFO *ip;
	DB_TXN *txn;
{
	DBMETA *meta;
	DB_MPOOLFILE *mpf;
	int ret, t_ret;

	ret = 0;
	if (!F_ISSET(dbp, DB_AM_CREATED)) {
		/* Subdb exists; read meta-data page and initialize. */
		mpf = mdbp->mpf;
		if  ((ret = __memp_fget(mpf, &dbp->meta_pgno,
		    ip, txn, 0, &meta)) != 0)
			goto err;
		ret = __db_meta_setup(mdbp->env, dbp, name, meta, 0, 0);
		if ((t_ret = __memp_fput(mpf,
		    ip, meta, dbp->priority)) != 0 && ret == 0)
			ret = t_ret;
		/*
		 * If __db_meta_setup found that the meta-page hadn't
		 * been written out during recovery, we can just return.
		 */
		if (ret == ENOENT)
			ret = 0;
		goto err;
	}

	/* Handle the create case here. */
	switch (dbp->type) {
	case DB_BTREE:
	case DB_RECNO:
		ret = __bam_new_subdb(mdbp, dbp, ip, txn);
		break;
	case DB_HASH:
		ret = __ham_new_subdb(mdbp, dbp, ip, txn);
		break;
	case DB_QUEUE:
		ret = EINVAL;
		break;
	case DB_UNKNOWN:
	default:
		__db_errx(dbp->env,
		    "Invalid subdatabase type %d specified", dbp->type);
		return (EINVAL);
	}

err:	return (ret);
}

/*
 * __db_chk_meta --
 *	Take a buffer containing a meta-data page and check it for a valid LSN,
 *	checksum (and verify the checksum if necessary) and possibly decrypt it.
 *
 *	Return 0 on success, >0 (errno) on error, -1 on checksum mismatch.
 *
 * PUBLIC: int __db_chk_meta __P((ENV *, DB *, DBMETA *, u_int32_t));
 */
int
__db_chk_meta(env, dbp, meta, flags)
	ENV *env;
	DB *dbp;
	DBMETA *meta;
	u_int32_t flags;
{
	DB_LSN swap_lsn;
	int is_hmac, ret, swapped;
	u_int32_t magic, orig_chk;
	u_int8_t *chksum;

	ret = 0;
	swapped = 0;

	if (FLD_ISSET(meta->metaflags, DBMETA_CHKSUM)) {
		if (dbp != NULL)
			F_SET(dbp, DB_AM_CHKSUM);

		is_hmac = meta->encrypt_alg == 0 ? 0 : 1;
		chksum = ((BTMETA *)meta)->chksum;

		/*
		 * If we need to swap, the checksum function overwrites the
		 * original checksum with 0, so we need to save a copy of the
		 * original for swapping later.
		 */
		orig_chk = *(u_int32_t *)chksum;

		/*
		 * We cannot add this to __db_metaswap because that gets done
		 * later after we've verified the checksum or decrypted.
		 */
		if (LF_ISSET(DB_CHK_META)) {
			swapped = 0;
chk_retry:		if ((ret =
			    __db_check_chksum(env, NULL, env->crypto_handle,
			    chksum, meta, DBMETASIZE, is_hmac)) != 0) {
				if (is_hmac || swapped)
					return (ret);

				M_32_SWAP(orig_chk);
				swapped = 1;
				*(u_int32_t *)chksum = orig_chk;
				goto chk_retry;
			}
		}
	} else if (dbp != NULL)
		F_CLR(dbp, DB_AM_CHKSUM);

#ifdef HAVE_CRYPTO
	ret = __crypto_decrypt_meta(env,
	     dbp, (u_int8_t *)meta, LF_ISSET(DB_CHK_META));
#endif

	/* Now that we're decrypted, we can check LSN. */
	if (LOGGING_ON(env) && !LF_ISSET(DB_CHK_NOLSN)) {
		/*
		 * This gets called both before and after swapping, so we
		 * need to check ourselves.  If we already swapped it above,
		 * we'll know that here.
		 */

		swap_lsn = meta->lsn;
		magic = meta->magic;
lsn_retry:
		if (swapped) {
			M_32_SWAP(swap_lsn.file);
			M_32_SWAP(swap_lsn.offset);
			M_32_SWAP(magic);
		}
		switch (magic) {
		case DB_BTREEMAGIC:
		case DB_HASHMAGIC:
		case DB_QAMMAGIC:
		case DB_RENAMEMAGIC:
			break;
		default:
			if (swapped)
				return (EINVAL);
			swapped = 1;
			goto lsn_retry;
		}
		if (!IS_REP_CLIENT(env) &&
		    !IS_NOT_LOGGED_LSN(swap_lsn) && !IS_ZERO_LSN(swap_lsn))
			/* Need to do check. */
			ret = __log_check_page_lsn(env, dbp, &swap_lsn);
	}
	return (ret);
}

/*
 * __db_meta_setup --
 *
 * Take a buffer containing a meta-data page and figure out if it's
 * valid, and if so, initialize the dbp from the meta-data page.
 *
 * PUBLIC: int __db_meta_setup __P((ENV *,
 * PUBLIC:     DB *, const char *, DBMETA *, u_int32_t, u_int32_t));
 */
int
__db_meta_setup(env, dbp, name, meta, oflags, flags)
	ENV *env;
	DB *dbp;
	const char *name;
	DBMETA *meta;
	u_int32_t oflags;
	u_int32_t flags;
{
	u_int32_t magic;
	int ret;

	ret = 0;

	/*
	 * Figure out what access method we're dealing with, and then
	 * call access method specific code to check error conditions
	 * based on conflicts between the found file and application
	 * arguments.  A found file overrides some user information --
	 * we don't consider it an error, for example, if the user set
	 * an expected byte order and the found file doesn't match it.
	 */
	F_CLR(dbp, DB_AM_SWAP | DB_AM_IN_RENAME);
	magic = meta->magic;

swap_retry:
	switch (magic) {
	case DB_BTREEMAGIC:
	case DB_HASHMAGIC:
	case DB_QAMMAGIC:
	case DB_RENAMEMAGIC:
		break;
	case 0:
		/*
		 * The only time this should be 0 is if we're in the
		 * midst of opening a subdb during recovery and that
		 * subdatabase had its meta-data page allocated, but
		 * not yet initialized.
		 */
		if (F_ISSET(dbp, DB_AM_SUBDB) && ((IS_RECOVERING(env) &&
		    F_ISSET(env->lg_handle, DBLOG_FORCE_OPEN)) ||
		    meta->pgno != PGNO_INVALID))
			return (ENOENT);

		goto bad_format;
	default:
		if (F_ISSET(dbp, DB_AM_SWAP))
			goto bad_format;

		M_32_SWAP(magic);
		F_SET(dbp, DB_AM_SWAP);
		goto swap_retry;
	}

	/*
	 * We can only check the meta page if we are sure we have a meta page.
	 * If it is random data, then this check can fail.  So only now can we
	 * checksum and decrypt.  Don't distinguish between configuration and
	 * checksum match errors here, because we haven't opened the database
	 * and even a checksum error isn't a reason to panic the environment.
	 */
	if ((ret = __db_chk_meta(env, dbp, meta, flags)) != 0) {
		if (ret == -1)
			__db_errx(env,
			    "%s: metadata page checksum error", name);
		goto bad_format;
	}

	switch (magic) {
	case DB_BTREEMAGIC:
		if (dbp->type != DB_UNKNOWN &&
		    dbp->type != DB_RECNO && dbp->type != DB_BTREE)
			goto bad_format;

		flags = meta->flags;
		if (F_ISSET(dbp, DB_AM_SWAP))
			M_32_SWAP(flags);
		if (LF_ISSET(BTM_RECNO))
			dbp->type = DB_RECNO;
		else
			dbp->type = DB_BTREE;
		if ((oflags & DB_TRUNCATE) == 0 && (ret =
		    __bam_metachk(dbp, name, (BTMETA *)meta)) != 0)
			return (ret);
		break;
	case DB_HASHMAGIC:
		if (dbp->type != DB_UNKNOWN && dbp->type != DB_HASH)
			goto bad_format;

		dbp->type = DB_HASH;
		if ((oflags & DB_TRUNCATE) == 0 && (ret =
		    __ham_metachk(dbp, name, (HMETA *)meta)) != 0)
			return (ret);
		break;
	case DB_QAMMAGIC:
		if (dbp->type != DB_UNKNOWN && dbp->type != DB_QUEUE)
			goto bad_format;
		dbp->type = DB_QUEUE;
		if ((oflags & DB_TRUNCATE) == 0 && (ret =
		    __qam_metachk(dbp, name, (QMETA *)meta)) != 0)
			return (ret);
		break;
	case DB_RENAMEMAGIC:
		F_SET(dbp, DB_AM_IN_RENAME);

		/* Copy the file's ID. */
		memcpy(dbp->fileid, ((DBMETA *)meta)->uid, DB_FILE_ID_LEN);

		break;
	default:
		goto bad_format;
	}

	if (FLD_ISSET(meta->metaflags,
	    DBMETA_PART_RANGE | DBMETA_PART_CALLBACK))
		if ((ret =
		    __partition_init(dbp, meta->metaflags)) != 0)
			return (ret);
	return (0);

bad_format:
	if (F_ISSET(dbp, DB_AM_RECOVER))
		ret = ENOENT;
	else
		__db_errx(env,
		    "__db_meta_setup: %s: unexpected file type or format",
		    name);
	return (ret == 0 ? EINVAL : ret);
}
