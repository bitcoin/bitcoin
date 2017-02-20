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
#include "dbinc/hash.h"
#include "dbinc/qam.h"

/*
 * __db_upgrade_pp --
 *	DB->upgrade pre/post processing.
 *
 * PUBLIC: int __db_upgrade_pp __P((DB *, const char *, u_int32_t));
 */
int
__db_upgrade_pp(dbp, fname, flags)
	DB *dbp;
	const char *fname;
	u_int32_t flags;
{
#ifdef HAVE_UPGRADE_SUPPORT
	DB_THREAD_INFO *ip;
	ENV *env;
	int ret;

	env = dbp->env;

	/*
	 * !!!
	 * The actual argument checking is simple, do it inline.
	 */
	if ((ret = __db_fchk(env, "DB->upgrade", flags, DB_DUPSORT)) != 0)
		return (ret);

	ENV_ENTER(env, ip);
	ret = __db_upgrade(dbp, fname, flags);
	ENV_LEAVE(env, ip);
	return (ret);
#else
	COMPQUIET(dbp, NULL);
	COMPQUIET(fname, NULL);
	COMPQUIET(flags, 0);

	__db_errx(dbp->env, "upgrade not supported");
	return (EINVAL);
#endif
}

#ifdef HAVE_UPGRADE_SUPPORT
static int (* const func_31_list[P_PAGETYPE_MAX])
    __P((DB *, char *, u_int32_t, DB_FH *, PAGE *, int *)) = {
	NULL,			/* P_INVALID */
	NULL,			/* __P_DUPLICATE */
	__ham_31_hash,		/* P_HASH_UNSORTED */
	NULL,			/* P_IBTREE */
	NULL,			/* P_IRECNO */
	__bam_31_lbtree,	/* P_LBTREE */
	NULL,			/* P_LRECNO */
	NULL,			/* P_OVERFLOW */
	__ham_31_hashmeta,	/* P_HASHMETA */
	__bam_31_btreemeta,	/* P_BTREEMETA */
	NULL,			/* P_QAMMETA */
	NULL,			/* P_QAMDATA */
	NULL,			/* P_LDUP */
	NULL,			/* P_HASH */
};

static int (* const func_46_list[P_PAGETYPE_MAX])
    __P((DB *, char *, u_int32_t, DB_FH *, PAGE *, int *)) = {
	NULL,			/* P_INVALID */
	NULL,			/* __P_DUPLICATE */
	__ham_46_hash,		/* P_HASH_UNSORTED */
	NULL,			/* P_IBTREE */
	NULL,			/* P_IRECNO */
	NULL,			/* P_LBTREE */
	NULL,			/* P_LRECNO */
	NULL,			/* P_OVERFLOW */
	__ham_46_hashmeta,	/* P_HASHMETA */
	NULL,			/* P_BTREEMETA */
	NULL,			/* P_QAMMETA */
	NULL,			/* P_QAMDATA */
	NULL,			/* P_LDUP */
	NULL,			/* P_HASH */
};

static int __db_page_pass __P((DB *, char *, u_int32_t, int (* const [])
	       (DB *, char *, u_int32_t, DB_FH *, PAGE *, int *), DB_FH *));
static int __db_set_lastpgno __P((DB *, char *, DB_FH *));

/*
 * __db_upgrade --
 *	Upgrade an existing database.
 *
 * PUBLIC: int __db_upgrade __P((DB *, const char *, u_int32_t));
 */
int
__db_upgrade(dbp, fname, flags)
	DB *dbp;
	const char *fname;
	u_int32_t flags;
{
	DBMETA *meta;
	DB_FH *fhp;
	ENV *env;
	size_t n;
	int ret, t_ret, use_mp_open;
	u_int8_t mbuf[256], tmpflags;
	char *real_name;

	use_mp_open = 0;
	env = dbp->env;
	fhp = NULL;

	/* Get the real backing file name. */
	if ((ret = __db_appname(env,
	    DB_APP_DATA, fname, NULL, &real_name)) != 0)
		return (ret);

	/* Open the file. */
	if ((ret = __os_open(env, real_name, 0, 0, 0, &fhp)) != 0) {
		__db_err(env, ret, "%s", real_name);
		return (ret);
	}

	/* Initialize the feedback. */
	if (dbp->db_feedback != NULL)
		dbp->db_feedback(dbp, DB_UPGRADE, 0);

	/*
	 * Read the metadata page.  We read 256 bytes, which is larger than
	 * any access method's metadata page and smaller than any disk sector.
	 */
	if ((ret = __os_read(env, fhp, mbuf, sizeof(mbuf), &n)) != 0)
		goto err;

	switch (((DBMETA *)mbuf)->magic) {
	case DB_BTREEMAGIC:
		switch (((DBMETA *)mbuf)->version) {
		case 6:
			/*
			 * Before V7 not all pages had page types, so we do the
			 * single meta-data page by hand.
			 */
			if ((ret =
			    __bam_30_btreemeta(dbp, real_name, mbuf)) != 0)
				goto err;
			if ((ret = __os_seek(env, fhp, 0, 0, 0)) != 0)
				goto err;
			if ((ret = __os_write(env, fhp, mbuf, 256, &n)) != 0)
				goto err;
			/* FALLTHROUGH */
		case 7:
			/*
			 * We need the page size to do more.  Rip it out of
			 * the meta-data page.
			 */
			memcpy(&dbp->pgsize, mbuf + 20, sizeof(u_int32_t));

			if ((ret = __db_page_pass(
			    dbp, real_name, flags, func_31_list, fhp)) != 0)
				goto err;
			/* FALLTHROUGH */
		case 8:
			if ((ret =
			     __db_set_lastpgno(dbp, real_name, fhp)) != 0)
				goto err;
			/* FALLTHROUGH */
		case 9:
			break;
		default:
			__db_errx(env, "%s: unsupported btree version: %lu",
			    real_name, (u_long)((DBMETA *)mbuf)->version);
			ret = DB_OLD_VERSION;
			goto err;
		}
		break;
	case DB_HASHMAGIC:
		switch (((DBMETA *)mbuf)->version) {
		case 4:
		case 5:
			/*
			 * Before V6 not all pages had page types, so we do the
			 * single meta-data page by hand.
			 */
			if ((ret =
			    __ham_30_hashmeta(dbp, real_name, mbuf)) != 0)
				goto err;
			if ((ret = __os_seek(env, fhp, 0, 0, 0)) != 0)
				goto err;
			if ((ret = __os_write(env, fhp, mbuf, 256, &n)) != 0)
				goto err;

			/*
			 * Before V6, we created hash pages one by one as they
			 * were needed, using hashhdr.ovfl_point to reserve
			 * a block of page numbers for them.  A consequence
			 * of this was that, if no overflow pages had been
			 * created, the current doubling might extend past
			 * the end of the database file.
			 *
			 * In DB 3.X, we now create all the hash pages
			 * belonging to a doubling atomically; it's not
			 * safe to just save them for later, because when
			 * we create an overflow page we'll just create
			 * a new last page (whatever that may be).  Grow
			 * the database to the end of the current doubling.
			 */
			if ((ret =
			    __ham_30_sizefix(dbp, fhp, real_name, mbuf)) != 0)
				goto err;
			/* FALLTHROUGH */
		case 6:
			/*
			 * We need the page size to do more.  Rip it out of
			 * the meta-data page.
			 */
			memcpy(&dbp->pgsize, mbuf + 20, sizeof(u_int32_t));

			if ((ret = __db_page_pass(
			    dbp, real_name, flags, func_31_list, fhp)) != 0)
				goto err;
			/* FALLTHROUGH */
		case 7:
			if ((ret =
			     __db_set_lastpgno(dbp, real_name, fhp)) != 0)
				goto err;
			/* FALLTHROUGH */
		case 8:
			/*
			 * Any upgrade that has proceeded this far has metadata
			 * pages compatible with hash version 8 metadata pages,
			 * so casting mbuf to a dbmeta is safe.
			 * If a newer revision moves the pagesize, checksum or
			 * encrypt_alg flags in the metadata, then the
			 * extraction of the fields will need to use hard coded
			 * offsets.
			 */
			meta = (DBMETA*)mbuf;
			/*
			 * We need the page size to do more.  Extract it from
			 * the meta-data page.
			 */
			memcpy(&dbp->pgsize, &meta->pagesize,
			    sizeof(u_int32_t));
			/*
			 * Rip out metadata and encrypt_alg fields from the
			 * metadata page. So the upgrade can know how big
			 * the page metadata pre-amble is. Any upgrade that has
			 * proceeded this far has metadata pages compatible
			 * with hash version 8 metadata pages, so extracting
			 * the fields is safe.
			 */
			memcpy(&tmpflags, &meta->metaflags, sizeof(u_int8_t));
			if (FLD_ISSET(tmpflags, DBMETA_CHKSUM))
				F_SET(dbp, DB_AM_CHKSUM);
			memcpy(&tmpflags, &meta->encrypt_alg, sizeof(u_int8_t));
			if (tmpflags != 0) {
				if (!CRYPTO_ON(dbp->env)) {
					__db_errx(env,
"Attempt to upgrade an encrypted database without providing a password.");
					ret = EINVAL;
					goto err;
				}
				F_SET(dbp, DB_AM_ENCRYPT);
			}

			/*
			 * This is ugly. It is necessary to have a usable
			 * mpool in the dbp to upgrade from an unsorted
			 * to a sorted hash database. The mpool file is used
			 * to resolve offpage key items, which are needed to
			 * determine sort order. Having mpool open and access
			 * the file does not affect the page pass, since the
			 * page pass only updates DB_HASH_UNSORTED pages
			 * in-place, and the mpool file is only used to read
			 * OFFPAGE items.
			 */
			use_mp_open = 1;
			if ((ret = __os_closehandle(env, fhp)) != 0)
				return (ret);
			dbp->type = DB_HASH;
			if ((ret = __env_mpool(dbp, fname,
			    DB_AM_NOT_DURABLE | DB_AM_VERIFYING)) != 0)
				return (ret);
			fhp = dbp->mpf->fhp;

			/* Do the actual conversion pass. */
			if ((ret = __db_page_pass(
			    dbp, real_name, flags, func_46_list, fhp)) != 0)
				goto err;

			/* FALLTHROUGH */
		case 9:
			break;
		default:
			__db_errx(env, "%s: unsupported hash version: %lu",
			    real_name, (u_long)((DBMETA *)mbuf)->version);
			ret = DB_OLD_VERSION;
			goto err;
		}
		break;
	case DB_QAMMAGIC:
		switch (((DBMETA *)mbuf)->version) {
		case 1:
			/*
			 * If we're in a Queue database, the only page that
			 * needs upgrading is the meta-database page, don't
			 * bother with a full pass.
			 */
			if ((ret = __qam_31_qammeta(dbp, real_name, mbuf)) != 0)
				return (ret);
			/* FALLTHROUGH */
		case 2:
			if ((ret = __qam_32_qammeta(dbp, real_name, mbuf)) != 0)
				return (ret);
			if ((ret = __os_seek(env, fhp, 0, 0, 0)) != 0)
				goto err;
			if ((ret = __os_write(env, fhp, mbuf, 256, &n)) != 0)
				goto err;
			/* FALLTHROUGH */
		case 3:
		case 4:
			break;
		default:
			__db_errx(env, "%s: unsupported queue version: %lu",
			    real_name, (u_long)((DBMETA *)mbuf)->version);
			ret = DB_OLD_VERSION;
			goto err;
		}
		break;
	default:
		M_32_SWAP(((DBMETA *)mbuf)->magic);
		switch (((DBMETA *)mbuf)->magic) {
		case DB_BTREEMAGIC:
		case DB_HASHMAGIC:
		case DB_QAMMAGIC:
			__db_errx(env,
		"%s: DB->upgrade only supported on native byte-order systems",
			    real_name);
			break;
		default:
			__db_errx(env,
			    "%s: unrecognized file type", real_name);
			break;
		}
		ret = EINVAL;
		goto err;
	}

	ret = __os_fsync(env, fhp);

	/*
	 * If mp_open was used, then rely on the database close to clean up
	 * any file handles.
	 */
err:	if (use_mp_open == 0 && fhp != NULL &&
	    (t_ret = __os_closehandle(env, fhp)) != 0 && ret == 0)
		ret = t_ret;
	__os_free(env, real_name);

	/* We're done. */
	if (dbp->db_feedback != NULL)
		dbp->db_feedback(dbp, DB_UPGRADE, 100);

	return (ret);
}

/*
 * __db_page_pass --
 *	Walk the pages of the database, upgrading whatever needs it.
 */
static int
__db_page_pass(dbp, real_name, flags, fl, fhp)
	DB *dbp;
	char *real_name;
	u_int32_t flags;
	int (* const fl[P_PAGETYPE_MAX])
	    __P((DB *, char *, u_int32_t, DB_FH *, PAGE *, int *));
	DB_FH *fhp;
{
	ENV *env;
	PAGE *page;
	db_pgno_t i, pgno_last;
	size_t n;
	int dirty, ret;

	env = dbp->env;

	/* Determine the last page of the file. */
	if ((ret = __db_lastpgno(dbp, real_name, fhp, &pgno_last)) != 0)
		return (ret);

	/* Allocate memory for a single page. */
	if ((ret = __os_malloc(env, dbp->pgsize, &page)) != 0)
		return (ret);

	/* Walk the file, calling the underlying conversion functions. */
	for (i = 0; i < pgno_last; ++i) {
		if (dbp->db_feedback != NULL)
			dbp->db_feedback(
			    dbp, DB_UPGRADE, (int)((i * 100)/pgno_last));
		if ((ret = __os_seek(env, fhp, i, dbp->pgsize, 0)) != 0)
			break;
		if ((ret = __os_read(env, fhp, page, dbp->pgsize, &n)) != 0)
			break;
		dirty = 0;
		/* Always decrypt the page. */
		if ((ret = __db_decrypt_pg(env, dbp, page)) != 0)
			break;
		if (fl[TYPE(page)] != NULL && (ret = fl[TYPE(page)]
		    (dbp, real_name, flags, fhp, page, &dirty)) != 0)
			break;
		if (dirty) {
			if ((ret = __db_encrypt_and_checksum_pg(
			    env, dbp, page)) != 0)
				break;
			if ((ret =
			    __os_seek(env, fhp, i, dbp->pgsize, 0)) != 0)
				break;
			if ((ret = __os_write(env,
			    fhp, page, dbp->pgsize, &n)) != 0)
				break;
		}
	}

	__os_free(dbp->env, page);
	return (ret);
}

/*
 * __db_lastpgno --
 *	Return the current last page number of the file.
 *
 * PUBLIC: int __db_lastpgno __P((DB *, char *, DB_FH *, db_pgno_t *));
 */
int
__db_lastpgno(dbp, real_name, fhp, pgno_lastp)
	DB *dbp;
	char *real_name;
	DB_FH *fhp;
	db_pgno_t *pgno_lastp;
{
	ENV *env;
	db_pgno_t pgno_last;
	u_int32_t mbytes, bytes;
	int ret;

	env = dbp->env;

	if ((ret = __os_ioinfo(env,
	    real_name, fhp, &mbytes, &bytes, NULL)) != 0) {
		__db_err(env, ret, "%s", real_name);
		return (ret);
	}

	/* Page sizes have to be a power-of-two. */
	if (bytes % dbp->pgsize != 0) {
		__db_errx(env,
		    "%s: file size not a multiple of the pagesize", real_name);
		return (EINVAL);
	}
	pgno_last = mbytes * (MEGABYTE / dbp->pgsize);
	pgno_last += bytes / dbp->pgsize;

	*pgno_lastp = pgno_last;
	return (0);
}

/*
 * __db_set_lastpgno --
 *	Update the meta->last_pgno field.
 *
 * Code assumes that we do not have checksums/crypto on the page.
 */
static int
__db_set_lastpgno(dbp, real_name, fhp)
	DB *dbp;
	char *real_name;
	DB_FH *fhp;
{
	DBMETA meta;
	ENV *env;
	int ret;
	size_t n;

	env = dbp->env;
	if ((ret = __os_seek(env, fhp, 0, 0, 0)) != 0)
		return (ret);
	if ((ret = __os_read(env, fhp, &meta, sizeof(meta), &n)) != 0)
		return (ret);
	dbp->pgsize = meta.pagesize;
	if ((ret = __db_lastpgno(dbp, real_name, fhp, &meta.last_pgno)) != 0)
		return (ret);
	if ((ret = __os_seek(env, fhp, 0, 0, 0)) != 0)
		return (ret);
	if ((ret = __os_write(env, fhp, &meta, sizeof(meta), &n)) != 0)
		return (ret);

	return (0);
}
#endif /* HAVE_UPGRADE_SUPPORT */
