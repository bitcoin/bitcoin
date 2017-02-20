/*-
 * See the file LICENSE for redistribution information.
 *
 * Copyright (c) 2000-2009 Oracle.  All rights reserved.
 *
 * $Id$
 */

#include "db_config.h"

#include "db_int.h"
#include "dbinc/db_page.h"
#include "dbinc/db_swap.h"
#include "dbinc/db_verify.h"
#include "dbinc/btree.h"
#include "dbinc/hash.h"
#include "dbinc/lock.h"
#include "dbinc/mp.h"
#include "dbinc/qam.h"
#include "dbinc/txn.h"

/*
 * This is the code for DB->verify, the DB database consistency checker.
 * For now, it checks all subdatabases in a database, and verifies
 * everything it knows how to (i.e. it's all-or-nothing, and one can't
 * check only for a subset of possible problems).
 */

static u_int __db_guesspgsize __P((ENV *, DB_FH *));
static int   __db_is_valid_magicno __P((u_int32_t, DBTYPE *));
static int   __db_meta2pgset
		__P((DB *, VRFY_DBINFO *, db_pgno_t, u_int32_t, DB *));
static int   __db_salvage __P((DB *, VRFY_DBINFO *,
		db_pgno_t, void *, int (*)(void *, const void *), u_int32_t));
static int   __db_salvage_subdbpg __P((DB *, VRFY_DBINFO *,
		PAGE *, void *, int (*)(void *, const void *), u_int32_t));
static int   __db_salvage_all __P((DB *, VRFY_DBINFO *, void *,
		int(*)(void *, const void *), u_int32_t, int *));
static int   __db_salvage_unknowns __P((DB *, VRFY_DBINFO *, void *,
		int (*)(void *, const void *), u_int32_t));
static int   __db_verify_arg __P((DB *, const char *, void *, u_int32_t));
static int   __db_vrfy_freelist
		__P((DB *, VRFY_DBINFO *, db_pgno_t, u_int32_t));
static int   __db_vrfy_invalid
		__P((DB *, VRFY_DBINFO *, PAGE *, db_pgno_t, u_int32_t));
static int   __db_vrfy_orderchkonly __P((DB *,
		VRFY_DBINFO *, const char *, const char *, u_int32_t));
static int   __db_vrfy_pagezero __P((DB *, VRFY_DBINFO *, DB_FH *, u_int32_t));
static int   __db_vrfy_subdbs
		__P((DB *, VRFY_DBINFO *, const char *, u_int32_t));
static int   __db_vrfy_structure __P((DB *, VRFY_DBINFO *,
		const char *, db_pgno_t, void *, void *, u_int32_t));
static int   __db_vrfy_walkpages __P((DB *, VRFY_DBINFO *,
		void *, int (*)(void *, const void *), u_int32_t));

#define	VERIFY_FLAGS							\
    (DB_AGGRESSIVE |							\
     DB_NOORDERCHK | DB_ORDERCHKONLY | DB_PRINTABLE | DB_SALVAGE | DB_UNREF)

/*
 * __db_verify_pp --
 *	DB->verify public interface.
 *
 * PUBLIC: int __db_verify_pp
 * PUBLIC:     __P((DB *, const char *, const char *, FILE *, u_int32_t));
 */
int
__db_verify_pp(dbp, file, database, outfile, flags)
	DB *dbp;
	const char *file, *database;
	FILE *outfile;
	u_int32_t flags;
{
	/*
	 * __db_verify_pp is a wrapper to __db_verify_internal, which lets
	 * us pass appropriate equivalents to FILE * in from the non-C APIs.
	 * That's why the usual ENV_ENTER macros are in __db_verify_internal,
	 * not here.
	 */
	return (__db_verify_internal(dbp,
	    file, database, outfile, __db_pr_callback, flags));
}

/*
 * __db_verify_internal --
 *
 * PUBLIC: int __db_verify_internal __P((DB *, const char *,
 * PUBLIC:     const char *, void *, int (*)(void *, const void *), u_int32_t));
 */
int
__db_verify_internal(dbp, fname, dname, handle, callback, flags)
	DB *dbp;
	const char *fname, *dname;
	void *handle;
	int (*callback) __P((void *, const void *));
	u_int32_t flags;
{
	DB_THREAD_INFO *ip;
	ENV *env;
	int ret, t_ret;

	env = dbp->env;

	DB_ILLEGAL_AFTER_OPEN(dbp, "DB->verify");

	if (!LF_ISSET(DB_SALVAGE))
		LF_SET(DB_UNREF);

	ENV_ENTER(env, ip);

	if ((ret = __db_verify_arg(dbp, dname, handle, flags)) == 0)
		ret = __db_verify(dbp, ip,
		     fname, dname, handle, callback, NULL, NULL, flags);

	/* Db.verify is a DB handle destructor. */
	if ((t_ret = __db_close(dbp, NULL, 0)) != 0 && ret == 0)
		ret = t_ret;

	ENV_LEAVE(env, ip);
	return (ret);
}

/*
 * __db_verify_arg --
 *	Check DB->verify arguments.
 */
static int
__db_verify_arg(dbp, dname, handle, flags)
	DB *dbp;
	const char *dname;
	void *handle;
	u_int32_t flags;
{
	ENV *env;
	int ret;

	env = dbp->env;

	if ((ret = __db_fchk(env, "DB->verify", flags, VERIFY_FLAGS)) != 0)
		return (ret);

	/*
	 * DB_SALVAGE is mutually exclusive with the other flags except
	 * DB_AGGRESSIVE, DB_PRINTABLE.
	 *
	 * DB_AGGRESSIVE and DB_PRINTABLE are only meaningful when salvaging.
	 *
	 * DB_SALVAGE requires an output stream.
	 */
	if (LF_ISSET(DB_SALVAGE)) {
		if (LF_ISSET(~(DB_AGGRESSIVE | DB_PRINTABLE | DB_SALVAGE)))
			return (__db_ferr(env, "DB->verify", 1));
		if (handle == NULL) {
			__db_errx(env,
			    "DB_SALVAGE requires a an output handle");
			return (EINVAL);
		}
	} else
		if (LF_ISSET(DB_AGGRESSIVE | DB_PRINTABLE))
			return (__db_ferr(env, "DB->verify", 1));

	/*
	 * DB_ORDERCHKONLY is mutually exclusive with DB_SALVAGE and
	 * DB_NOORDERCHK, and requires a database name.
	 */
	if ((ret = __db_fcchk(env, "DB->verify", flags,
	    DB_ORDERCHKONLY, DB_SALVAGE | DB_NOORDERCHK)) != 0)
		return (ret);
	if (LF_ISSET(DB_ORDERCHKONLY) && dname == NULL) {
		__db_errx(env, "DB_ORDERCHKONLY requires a database name");
		return (EINVAL);
	}
	return (0);
}

/*
 * __db_verify --
 *	Walk the entire file page-by-page, either verifying with or without
 *	dumping in db_dump -d format, or DB_SALVAGE-ing whatever key/data
 *	pairs can be found and dumping them in standard (db_load-ready)
 *	dump format.
 *
 *	(Salvaging isn't really a verification operation, but we put it
 *	here anyway because it requires essentially identical top-level
 *	code.)
 *
 *	flags may be 0, DB_NOORDERCHK, DB_ORDERCHKONLY, or DB_SALVAGE
 *	(and optionally DB_AGGRESSIVE).
 * PUBLIC: int   __db_verify __P((DB *, DB_THREAD_INFO *, const char *,
 * PUBLIC:		const char *, void *, int (*)(void *, const void *),
 * PUBLIC:		void *, void *, u_int32_t));
 */
int
__db_verify(dbp, ip, name, subdb, handle, callback, lp, rp, flags)
	DB *dbp;
	DB_THREAD_INFO *ip;
	const char *name, *subdb;
	void *handle;
	int (*callback) __P((void *, const void *));
	void *lp, *rp;
	u_int32_t flags;
{
	DB_FH *fhp;
	ENV *env;
	VRFY_DBINFO *vdp;
	u_int32_t sflags;
	int has_subdbs, isbad, ret, t_ret;
	char *real_name;

	env = dbp->env;
	fhp = NULL;
	vdp = NULL;
	real_name = NULL;
	has_subdbs = isbad = ret = t_ret = 0;

	F_SET(dbp, DB_AM_VERIFYING);

	/* Initialize any feedback function. */
	if (!LF_ISSET(DB_SALVAGE) && dbp->db_feedback != NULL)
		dbp->db_feedback(dbp, DB_VERIFY, 0);

	/*
	 * We don't know how large the cache is, and if the database
	 * in question uses a small page size--which we don't know
	 * yet!--it may be uncomfortably small for the default page
	 * size [#2143].  However, the things we need temporary
	 * databases for in dbinfo are largely tiny, so using a
	 * 1024-byte pagesize is probably not going to be a big hit,
	 * and will make us fit better into small spaces.
	 */
	if ((ret = __db_vrfy_dbinfo_create(env, ip,  1024, &vdp)) != 0)
		goto err;

	/*
	 * Note whether the user has requested that we use printable
	 * chars where possible.  We won't get here with this flag if
	 * we're not salvaging.
	 */
	if (LF_ISSET(DB_PRINTABLE))
		F_SET(vdp, SALVAGE_PRINTABLE);

	/* Find the real name of the file. */
	if ((ret = __db_appname(env,
	    DB_APP_DATA, name, &dbp->dirname, &real_name)) != 0)
		goto err;

	/*
	 * Our first order of business is to verify page 0, which is
	 * the metadata page for the master database of subdatabases
	 * or of the only database in the file.  We want to do this by hand
	 * rather than just calling __db_open in case it's corrupt--various
	 * things in __db_open might act funny.
	 *
	 * Once we know the metadata page is healthy, I believe that it's
	 * safe to open the database normally and then use the page swapping
	 * code, which makes life easier.
	 */
	if ((ret = __os_open(env, real_name, 0, DB_OSO_RDONLY, 0, &fhp)) != 0)
		goto err;

	/* Verify the metadata page 0; set pagesize and type. */
	if ((ret = __db_vrfy_pagezero(dbp, vdp, fhp, flags)) != 0) {
		if (ret == DB_VERIFY_BAD)
			isbad = 1;
		else
			goto err;
	}

	/*
	 * We can assume at this point that dbp->pagesize and dbp->type are
	 * set correctly, or at least as well as they can be, and that
	 * locking, logging, and txns are not in use.  Thus we can trust
	 * the memp code not to look at the page, and thus to be safe
	 * enough to use.
	 *
	 * The dbp is not open, but the file is open in the fhp, and we
	 * cannot assume that __db_open is safe.  Call __env_setup,
	 * the [safe] part of __db_open that initializes the environment--
	 * and the mpool--manually.
	 */
	if ((ret = __env_setup(dbp, NULL,
	    name, subdb, TXN_INVALID, DB_ODDFILESIZE | DB_RDONLY)) != 0)
		goto err;

	/*
	 * Set our name in the Queue subsystem;  we may need it later
	 * to deal with extents.
	 */
	if (dbp->type == DB_QUEUE &&
	    (ret = __qam_set_ext_data(dbp, name)) != 0)
		goto err;

	/* Mark the dbp as opened, so that we correctly handle its close. */
	F_SET(dbp, DB_AM_OPEN_CALLED);

	/* Find out the page number of the last page in the database. */
	if ((ret = __memp_get_last_pgno(dbp->mpf, &vdp->last_pgno)) != 0)
		goto err;

	/*
	 * DB_ORDERCHKONLY is a special case;  our file consists of
	 * several subdatabases, which use different hash, bt_compare,
	 * and/or dup_compare functions.  Consequently, we couldn't verify
	 * sorting and hashing simply by calling DB->verify() on the file.
	 * DB_ORDERCHKONLY allows us to come back and check those things;  it
	 * requires a subdatabase, and assumes that everything but that
	 * database's sorting/hashing is correct.
	 */
	if (LF_ISSET(DB_ORDERCHKONLY)) {
		ret = __db_vrfy_orderchkonly(dbp, vdp, name, subdb, flags);
		goto done;
	}

	sflags = flags;
	if (dbp->p_internal != NULL)
		LF_CLR(DB_SALVAGE);

	/*
	 * When salvaging, we use a db to keep track of whether we've seen a
	 * given overflow or dup page in the course of traversing normal data.
	 * If in the end we have not, we assume its key got lost and print it
	 * with key "UNKNOWN".
	 */
	if (LF_ISSET(DB_SALVAGE)) {
		if ((ret = __db_salvage_init(vdp)) != 0)
			goto err;

		/*
		 * If we're not being aggressive, salvage by walking the tree
		 * and only printing the leaves we find.  "has_subdbs" will
		 * indicate whether we found subdatabases.
		 */
		if (!LF_ISSET(DB_AGGRESSIVE) && __db_salvage_all(
		    dbp, vdp, handle, callback, flags, &has_subdbs) != 0)
			isbad = 1;

		/*
		 * If we have subdatabases, flag if any keys are found that
		 * don't belong to a subdatabase -- they'll need to have an
		 * "__OTHER__" subdatabase header printed first.
		 */
		if (has_subdbs) {
			F_SET(vdp, SALVAGE_PRINTHEADER);
			F_SET(vdp, SALVAGE_HASSUBDBS);
		}
	}

	/* Walk all the pages, if a page cannot be read, verify structure. */
	if ((ret =
	    __db_vrfy_walkpages(dbp, vdp, handle, callback, flags)) != 0) {
		if (ret == DB_VERIFY_BAD)
			isbad = 1;
		else if (ret != DB_PAGE_NOTFOUND)
			goto err;
	}

	/* If we're verifying, verify inter-page structure. */
	if (!LF_ISSET(DB_SALVAGE) && isbad == 0)
		if ((t_ret = __db_vrfy_structure(dbp,
		    vdp, name, 0, lp, rp, flags)) != 0) {
			if (t_ret == DB_VERIFY_BAD)
				isbad = 1;
			else
				goto err;
		}

	/*
	 * If we're salvaging, output with key UNKNOWN any overflow or dup pages
	 * we haven't been able to put in context.  Then destroy the salvager's
	 * state-saving database.
	 */
	if (LF_ISSET(DB_SALVAGE)) {
		if ((ret = __db_salvage_unknowns(dbp,
		    vdp, handle, callback, flags)) != 0)
			isbad = 1;
	}

	flags = sflags;

#ifdef HAVE_PARTITION
	if (t_ret == 0 && dbp->p_internal != NULL)
		t_ret = __part_verify(dbp, vdp, name, handle, callback, flags);
#endif

	if (ret == 0)
		ret = t_ret;

	/* Don't display a footer for a database holding other databases. */
	if (LF_ISSET(DB_SALVAGE | DB_VERIFY_PARTITION) == DB_SALVAGE &&
	    (!has_subdbs || F_ISSET(vdp, SALVAGE_PRINTFOOTER)))
		(void)__db_prfooter(handle, callback);

done: err:
	/* Send feedback that we're done. */
	if (!LF_ISSET(DB_SALVAGE) && dbp->db_feedback != NULL)
		dbp->db_feedback(dbp, DB_VERIFY, 100);

	if (LF_ISSET(DB_SALVAGE) &&
	    (t_ret = __db_salvage_destroy(vdp)) != 0 && ret == 0)
		ret = t_ret;
	if (fhp != NULL &&
	    (t_ret = __os_closehandle(env, fhp)) != 0 && ret == 0)
		ret = t_ret;
	if (vdp != NULL &&
	    (t_ret = __db_vrfy_dbinfo_destroy(env, vdp)) != 0 && ret == 0)
		ret = t_ret;
	if (real_name != NULL)
		__os_free(env, real_name);

	/*
	 * DB_VERIFY_FATAL is a private error, translate to a public one.
	 *
	 * If we didn't find a page, it's probably a page number was corrupted.
	 * Return the standard corruption error.
	 *
	 * Otherwise, if we found corruption along the way, set the return.
	 */
	if (ret == DB_VERIFY_FATAL ||
	    ret == DB_PAGE_NOTFOUND || (ret == 0 && isbad == 1))
		ret = DB_VERIFY_BAD;

	/* Make sure there's a public complaint if we found corruption. */
	if (ret != 0)
		__db_err(env, ret, "%s", name);

	return (ret);
}

/*
 * __db_vrfy_pagezero --
 *	Verify the master metadata page.  Use seek, read, and a local buffer
 *	rather than the DB paging code, for safety.
 *
 *	Must correctly (or best-guess) set dbp->type and dbp->pagesize.
 */
static int
__db_vrfy_pagezero(dbp, vdp, fhp, flags)
	DB *dbp;
	VRFY_DBINFO *vdp;
	DB_FH *fhp;
	u_int32_t flags;
{
	DBMETA *meta;
	ENV *env;
	VRFY_PAGEINFO *pip;
	db_pgno_t freelist;
	size_t nr;
	int isbad, ret, swapped;
	u_int8_t mbuf[DBMETASIZE];

	isbad = ret = swapped = 0;
	freelist = 0;
	env = dbp->env;
	meta = (DBMETA *)mbuf;
	dbp->type = DB_UNKNOWN;

	if ((ret = __db_vrfy_getpageinfo(vdp, PGNO_BASE_MD, &pip)) != 0)
		return (ret);

	/*
	 * Seek to the metadata page.
	 * Note that if we're just starting a verification, dbp->pgsize
	 * may be zero;  this is okay, as we want page zero anyway and
	 * 0*0 == 0.
	 */
	if ((ret = __os_seek(env, fhp, 0, 0, 0)) != 0 ||
	    (ret = __os_read(env, fhp, mbuf, DBMETASIZE, &nr)) != 0) {
		__db_err(env, ret,
		    "Metadata page %lu cannot be read", (u_long)PGNO_BASE_MD);
		return (ret);
	}

	if (nr != DBMETASIZE) {
		EPRINT((env,
		    "Page %lu: Incomplete metadata page",
		    (u_long)PGNO_BASE_MD));
		return (DB_VERIFY_FATAL);
	}

	if ((ret = __db_chk_meta(env, dbp, meta, 1)) != 0) {
		EPRINT((env,
		    "Page %lu: metadata page corrupted", (u_long)PGNO_BASE_MD));
		isbad = 1;
		if (ret != -1) {
			EPRINT((env,
			    "Page %lu: could not check metadata page",
			    (u_long)PGNO_BASE_MD));
			return (DB_VERIFY_FATAL);
		}
	}

	/*
	 * Check all of the fields that we can.
	 *
	 * 08-11: Current page number.  Must == pgno.
	 * Note that endianness doesn't matter--it's zero.
	 */
	if (meta->pgno != PGNO_BASE_MD) {
		isbad = 1;
		EPRINT((env, "Page %lu: pgno incorrectly set to %lu",
		    (u_long)PGNO_BASE_MD, (u_long)meta->pgno));
	}

	/* 12-15: Magic number.  Must be one of valid set. */
	if (__db_is_valid_magicno(meta->magic, &dbp->type))
		swapped = 0;
	else {
		M_32_SWAP(meta->magic);
		if (__db_is_valid_magicno(meta->magic,
		    &dbp->type))
			swapped = 1;
		else {
			isbad = 1;
			EPRINT((env,
			    "Page %lu: bad magic number %lu",
			    (u_long)PGNO_BASE_MD, (u_long)meta->magic));
		}
	}

	/*
	 * 16-19: Version.  Must be current;  for now, we
	 * don't support verification of old versions.
	 */
	if (swapped)
		M_32_SWAP(meta->version);
	if ((dbp->type == DB_BTREE &&
	    (meta->version > DB_BTREEVERSION ||
	    meta->version < DB_BTREEOLDVER)) ||
	    (dbp->type == DB_HASH &&
	    (meta->version > DB_HASHVERSION ||
	    meta->version < DB_HASHOLDVER)) ||
	    (dbp->type == DB_QUEUE &&
	    (meta->version > DB_QAMVERSION ||
	    meta->version < DB_QAMOLDVER))) {
		isbad = 1;
		EPRINT((env,
    "Page %lu: unsupported DB version %lu; extraneous errors may result",
		    (u_long)PGNO_BASE_MD, (u_long)meta->version));
	}

	/*
	 * 20-23: Pagesize.  Must be power of two,
	 * greater than 512, and less than 64K.
	 */
	if (swapped)
		M_32_SWAP(meta->pagesize);
	if (IS_VALID_PAGESIZE(meta->pagesize))
		dbp->pgsize = meta->pagesize;
	else {
		isbad = 1;
		EPRINT((env, "Page %lu: bad page size %lu",
		    (u_long)PGNO_BASE_MD, (u_long)meta->pagesize));

		/*
		 * Now try to settle on a pagesize to use.
		 * If the user-supplied one is reasonable,
		 * use it;  else, guess.
		 */
		if (!IS_VALID_PAGESIZE(dbp->pgsize))
			dbp->pgsize = __db_guesspgsize(env, fhp);
	}

	/*
	 * 25: Page type.  Must be correct for dbp->type,
	 * which is by now set as well as it can be.
	 */
	/* Needs no swapping--only one byte! */
	if ((dbp->type == DB_BTREE && meta->type != P_BTREEMETA) ||
	    (dbp->type == DB_HASH && meta->type != P_HASHMETA) ||
	    (dbp->type == DB_QUEUE && meta->type != P_QAMMETA)) {
		isbad = 1;
		EPRINT((env, "Page %lu: bad page type %lu",
		    (u_long)PGNO_BASE_MD, (u_long)meta->type));
	}

	/*
	 * 26: Meta-flags.
	 */
	if (meta->metaflags != 0) {
		if (FLD_ISSET(meta->metaflags,
		    ~(DBMETA_CHKSUM|DBMETA_PART_RANGE|DBMETA_PART_CALLBACK))) {
			isbad = 1;
			EPRINT((env,
			    "Page %lu: bad meta-data flags value %#lx",
			    (u_long)PGNO_BASE_MD, (u_long)meta->metaflags));
		}
		if (FLD_ISSET(meta->metaflags, DBMETA_CHKSUM))
			F_SET(pip, VRFY_HAS_CHKSUM);
		if (FLD_ISSET(meta->metaflags, DBMETA_PART_RANGE))
			F_SET(pip, VRFY_HAS_PART_RANGE);
		if (FLD_ISSET(meta->metaflags, DBMETA_PART_CALLBACK))
			F_SET(pip, VRFY_HAS_PART_CALLBACK);

		if (FLD_ISSET(meta->metaflags,
		    DBMETA_PART_RANGE | DBMETA_PART_CALLBACK) &&
		    (ret = __partition_init(dbp, meta->metaflags)) != 0)
			return (ret);
	}

	/*
	 * 28-31: Free list page number.
	 * 32-35: Last page in database file.
	 * We'll verify its sensibility when we do inter-page
	 * verification later;  for now, just store it.
	 */
	if (swapped)
	    M_32_SWAP(meta->free);
	freelist = meta->free;
	if (swapped)
	    M_32_SWAP(meta->last_pgno);
	vdp->meta_last_pgno = meta->last_pgno;

	/*
	 * Initialize vdp->pages to fit a single pageinfo structure for
	 * this one page.  We'll realloc later when we know how many
	 * pages there are.
	 */
	pip->pgno = PGNO_BASE_MD;
	pip->type = meta->type;

	/*
	 * Signal that we still have to check the info specific to
	 * a given type of meta page.
	 */
	F_SET(pip, VRFY_INCOMPLETE);

	pip->free = freelist;

	if ((ret = __db_vrfy_putpageinfo(env, vdp, pip)) != 0)
		return (ret);

	/* Set up the dbp's fileid.  We don't use the regular open path. */
	memcpy(dbp->fileid, meta->uid, DB_FILE_ID_LEN);

	if (swapped == 1)
		F_SET(dbp, DB_AM_SWAP);

	return (isbad ? DB_VERIFY_BAD : 0);
}

/*
 * __db_vrfy_walkpages --
 *	Main loop of the verifier/salvager.  Walks through,
 *	page by page, and verifies all pages and/or prints all data pages.
 */
static int
__db_vrfy_walkpages(dbp, vdp, handle, callback, flags)
	DB *dbp;
	VRFY_DBINFO *vdp;
	void *handle;
	int (*callback) __P((void *, const void *));
	u_int32_t flags;
{
	DB_MPOOLFILE *mpf;
	ENV *env;
	PAGE *h;
	VRFY_PAGEINFO *pip;
	db_pgno_t i;
	int ret, t_ret, isbad;

	env = dbp->env;
	mpf = dbp->mpf;
	h = NULL;
	ret = isbad = t_ret = 0;

	for (i = 0; i <= vdp->last_pgno; i++) {
		/*
		 * If DB_SALVAGE is set, we inspect our database of completed
		 * pages, and skip any we've already printed in the subdb pass.
		 */
		if (LF_ISSET(DB_SALVAGE) && (__db_salvage_isdone(vdp, i) != 0))
			continue;

		/*
		 * An individual page get can fail if:
		 *  * This is a hash database, it is expected to find
		 *    empty buckets, which don't have allocated pages. Create
		 *    a dummy page so the verification can proceed.
		 *  * We are salvaging, flag the error and continue.
		 */
		if ((t_ret = __memp_fget(mpf, &i,
		    vdp->thread_info, NULL, 0, &h)) != 0) {
			if (dbp->type == DB_HASH) {
				if ((t_ret =
				    __db_vrfy_getpageinfo(vdp, i, &pip)) != 0)
					goto err1;
				pip->type = P_INVALID;
				pip->pgno = i;
				F_CLR(pip, VRFY_IS_ALLZEROES);
				if ((t_ret = __db_vrfy_putpageinfo(
				    env, vdp, pip)) != 0)
					goto err1;
				continue;
			}
			if (t_ret == DB_PAGE_NOTFOUND) {
				EPRINT((env,
    "Page %lu: beyond the end of the file, metadata page has last page as %lu",
				    (u_long)i, (u_long)vdp->last_pgno));
				if (ret == 0)
					return (t_ret);
			}

err1:			if (ret == 0)
				ret = t_ret;
			if (LF_ISSET(DB_SALVAGE))
				continue;
			return (ret);
		}

		if (LF_ISSET(DB_SALVAGE)) {
			/*
			 * We pretty much don't want to quit unless a
			 * bomb hits.  May as well return that something
			 * was screwy, however.
			 */
			if ((t_ret = __db_salvage_pg(dbp,
			    vdp, i, h, handle, callback, flags)) != 0) {
				if (ret == 0)
					ret = t_ret;
				isbad = 1;
			}
		} else {
			/*
			 * If we are not salvaging, and we get any error
			 * other than DB_VERIFY_BAD, return immediately;
			 * it may not be safe to proceed.  If we get
			 * DB_VERIFY_BAD, keep going;  listing more errors
			 * may make it easier to diagnose problems and
			 * determine the magnitude of the corruption.
			 *
			 * Verify info common to all page types.
			 */
			if (i != PGNO_BASE_MD) {
				ret = __db_vrfy_common(dbp, vdp, h, i, flags);
				if (ret == DB_VERIFY_BAD)
					isbad = 1;
				else if (ret != 0)
					goto err;
			}

			switch (TYPE(h)) {
			case P_INVALID:
				ret = __db_vrfy_invalid(dbp, vdp, h, i, flags);
				break;
			case __P_DUPLICATE:
				isbad = 1;
				EPRINT((env,
				    "Page %lu: old-style duplicate page",
				    (u_long)i));
				break;
			case P_HASH_UNSORTED:
			case P_HASH:
				ret = __ham_vrfy(dbp, vdp, h, i, flags);
				break;
			case P_IBTREE:
			case P_IRECNO:
			case P_LBTREE:
			case P_LDUP:
				ret = __bam_vrfy(dbp, vdp, h, i, flags);
				break;
			case P_LRECNO:
				ret = __ram_vrfy_leaf(dbp, vdp, h, i, flags);
				break;
			case P_OVERFLOW:
				ret = __db_vrfy_overflow(dbp, vdp, h, i, flags);
				break;
			case P_HASHMETA:
				ret = __ham_vrfy_meta(dbp,
				    vdp, (HMETA *)h, i, flags);
				break;
			case P_BTREEMETA:
				ret = __bam_vrfy_meta(dbp,
				    vdp, (BTMETA *)h, i, flags);
				break;
			case P_QAMMETA:
				ret = __qam_vrfy_meta(dbp,
				    vdp, (QMETA *)h, i, flags);
				break;
			case P_QAMDATA:
				ret = __qam_vrfy_data(dbp,
				    vdp, (QPAGE *)h, i, flags);
				break;
			default:
				EPRINT((env,
				    "Page %lu: unknown page type %lu",
				    (u_long)i, (u_long)TYPE(h)));
				isbad = 1;
				break;
			}

			/*
			 * Set up error return.
			 */
			if (ret == DB_VERIFY_BAD)
				isbad = 1;
			else if (ret != 0)
				goto err;

			/*
			 * Provide feedback to the application about our
			 * progress.  The range 0-50% comes from the fact
			 * that this is the first of two passes through the
			 * database (front-to-back, then top-to-bottom).
			 */
			if (dbp->db_feedback != NULL)
				dbp->db_feedback(dbp, DB_VERIFY,
				    (int)((i + 1) * 50 / (vdp->last_pgno + 1)));
		}

		/*
		 * Just as with the page get, bail if and only if we're
		 * not salvaging.
		 */
		if ((t_ret = __memp_fput(mpf,
		    vdp->thread_info, h, dbp->priority)) != 0) {
			if (ret == 0)
				ret = t_ret;
			if (!LF_ISSET(DB_SALVAGE))
				return (ret);
		}
	}

	/*
	 * If we've seen a Queue metadata page, we may need to walk Queue
	 * extent pages that won't show up between 0 and vdp->last_pgno.
	 */
	if (F_ISSET(vdp, VRFY_QMETA_SET) && (t_ret =
	    __qam_vrfy_walkqueue(dbp, vdp, handle, callback, flags)) != 0) {
		if (ret == 0)
			ret = t_ret;
		if (t_ret == DB_VERIFY_BAD)
			isbad = 1;
		else if (!LF_ISSET(DB_SALVAGE))
			return (ret);
	}

	if (0) {
err:		if (h != NULL && (t_ret = __memp_fput(mpf,
		    vdp->thread_info, h, dbp->priority)) != 0)
			return (ret == 0 ? t_ret : ret);
	}

	return ((isbad == 1 && ret == 0) ? DB_VERIFY_BAD : ret);
}

/*
 * __db_vrfy_structure--
 *	After a beginning-to-end walk through the database has been
 *	completed, put together the information that has been collected
 *	to verify the overall database structure.
 *
 *	Should only be called if we want to do a database verification,
 *	i.e. if DB_SALVAGE is not set.
 */
static int
__db_vrfy_structure(dbp, vdp, dbname, meta_pgno, lp, rp, flags)
	DB *dbp;
	VRFY_DBINFO *vdp;
	const char *dbname;
	db_pgno_t meta_pgno;
	void *lp, *rp;
	u_int32_t flags;
{
	DB *pgset;
	ENV *env;
	VRFY_PAGEINFO *pip;
	db_pgno_t i;
	int ret, isbad, hassubs, p;

	isbad = 0;
	pip = NULL;
	env = dbp->env;
	pgset = vdp->pgset;

	/*
	 * Providing feedback here is tricky;  in most situations,
	 * we fetch each page one more time, but we do so in a top-down
	 * order that depends on the access method.  Worse, we do this
	 * recursively in btree, such that on any call where we're traversing
	 * a subtree we don't know where that subtree is in the whole database;
	 * worse still, any given database may be one of several subdbs.
	 *
	 * The solution is to decrement a counter vdp->pgs_remaining each time
	 * we verify (and call feedback on) a page.  We may over- or
	 * under-count, but the structure feedback function will ensure that we
	 * never give a percentage under 50 or over 100.  (The first pass
	 * covered the range 0-50%.)
	 */
	if (dbp->db_feedback != NULL)
		vdp->pgs_remaining = vdp->last_pgno + 1;

	/*
	 * Call the appropriate function to downwards-traverse the db type.
	 */
	switch (dbp->type) {
	case DB_BTREE:
	case DB_RECNO:
		if ((ret =
		    __bam_vrfy_structure(dbp, vdp, 0, lp, rp, flags)) != 0) {
			if (ret == DB_VERIFY_BAD)
				isbad = 1;
			else
				goto err;
		}

		/*
		 * If we have subdatabases and we know that the database is,
		 * thus far, sound, it's safe to walk the tree of subdatabases.
		 * Do so, and verify the structure of the databases within.
		 */
		if ((ret = __db_vrfy_getpageinfo(vdp, 0, &pip)) != 0)
			goto err;
		hassubs = F_ISSET(pip, VRFY_HAS_SUBDBS) ? 1 : 0;
		if ((ret = __db_vrfy_putpageinfo(env, vdp, pip)) != 0)
			goto err;
		pip = NULL;

		if (isbad == 0 && hassubs)
			if ((ret =
			    __db_vrfy_subdbs(dbp, vdp, dbname, flags)) != 0) {
				if (ret == DB_VERIFY_BAD)
					isbad = 1;
				else
					goto err;
			}
		break;
	case DB_HASH:
		if ((ret = __ham_vrfy_structure(dbp, vdp, 0, flags)) != 0) {
			if (ret == DB_VERIFY_BAD)
				isbad = 1;
			else
				goto err;
		}
		break;
	case DB_QUEUE:
		if ((ret = __qam_vrfy_structure(dbp, vdp, flags)) != 0) {
			if (ret == DB_VERIFY_BAD)
				isbad = 1;
		}

		/*
		 * Queue pages may be unreferenced and totally zeroed, if
		 * they're empty;  queue doesn't have much structure, so
		 * this is unlikely to be wrong in any troublesome sense.
		 * Skip to "err".
		 */
		goto err;
	case DB_UNKNOWN:
	default:
		ret = __db_unknown_path(env, "__db_vrfy_structure");
		goto err;
	}

	/* Walk free list. */
	if ((ret =
	    __db_vrfy_freelist(dbp, vdp, meta_pgno, flags)) == DB_VERIFY_BAD)
		isbad = 1;

	/*
	 * If structure checks up until now have failed, it's likely that
	 * checking what pages have been missed will result in oodles of
	 * extraneous error messages being EPRINTed.  Skip to the end
	 * if this is the case;  we're going to be printing at least one
	 * error anyway, and probably all the more salient ones.
	 */
	if (ret != 0 || isbad == 1)
		goto err;

	/*
	 * Make sure no page has been missed and that no page is still marked
	 * "all zeroes" (only certain hash pages can be, and they're unmarked
	 * in __ham_vrfy_structure).
	 */
	for (i = 0; i < vdp->last_pgno + 1; i++) {
		if ((ret = __db_vrfy_getpageinfo(vdp, i, &pip)) != 0)
			goto err;
		if ((ret = __db_vrfy_pgset_get(pgset,
		    vdp->thread_info, i, &p)) != 0)
			goto err;
		if (pip->type == P_OVERFLOW) {
			if ((u_int32_t)p != pip->refcount) {
				EPRINT((env,
		    "Page %lu: overflow refcount %lu, referenced %lu times",
				    (u_long)i,
				    (u_long)pip->refcount, (u_long)p));
				isbad = 1;
			}
		} else if (p == 0 &&
#ifndef HAVE_FTRUNCATE
		    !(i > vdp->meta_last_pgno &&
		    (F_ISSET(pip, VRFY_IS_ALLZEROES) || pip->type == P_HASH)) &&
#endif
		    !(dbp->type == DB_HASH && pip->type == P_INVALID)) {
			/*
			 * It is OK for unreferenced hash buckets to be
			 * marked invalid and unreferenced.
			 */
			EPRINT((env,
			    "Page %lu: unreferenced page", (u_long)i));
			isbad = 1;
		}

		if (F_ISSET(pip, VRFY_IS_ALLZEROES)
#ifndef HAVE_FTRUNCATE
		    && i <= vdp->meta_last_pgno
#endif
		    ) {
			EPRINT((env,
			    "Page %lu: totally zeroed page", (u_long)i));
			isbad = 1;
		}
		if ((ret = __db_vrfy_putpageinfo(env, vdp, pip)) != 0)
			goto err;
		pip = NULL;
	}

err:	if (pip != NULL)
		(void)__db_vrfy_putpageinfo(env, vdp, pip);

	return ((isbad == 1 && ret == 0) ? DB_VERIFY_BAD : ret);
}

/*
 * __db_is_valid_magicno
 */
static int
__db_is_valid_magicno(magic, typep)
	u_int32_t magic;
	DBTYPE *typep;
{
	switch (magic) {
	case DB_BTREEMAGIC:
		*typep = DB_BTREE;
		return (1);
	case DB_HASHMAGIC:
		*typep = DB_HASH;
		return (1);
	case DB_QAMMAGIC:
		*typep = DB_QUEUE;
		return (1);
	default:
		break;
	}
	*typep = DB_UNKNOWN;
	return (0);
}

/*
 * __db_vrfy_common --
 *	Verify info common to all page types.
 *
 * PUBLIC: int  __db_vrfy_common
 * PUBLIC:     __P((DB *, VRFY_DBINFO *, PAGE *, db_pgno_t, u_int32_t));
 */
int
__db_vrfy_common(dbp, vdp, h, pgno, flags)
	DB *dbp;
	VRFY_DBINFO *vdp;
	PAGE *h;
	db_pgno_t pgno;
	u_int32_t flags;
{
	ENV *env;
	VRFY_PAGEINFO *pip;
	int ret, t_ret;
	u_int8_t *p;

	env = dbp->env;

	if ((ret = __db_vrfy_getpageinfo(vdp, pgno, &pip)) != 0)
		return (ret);

	pip->pgno = pgno;
	F_CLR(pip, VRFY_IS_ALLZEROES);

	/*
	 * Hash expands the table by leaving some pages between the
	 * old last and the new last totally zeroed.  These pages may
	 * not be all zero if they were used, freed and then reallocated.
	 *
	 * Queue will create sparse files if sparse record numbers are used.
	 */
	if (pgno != 0 && PGNO(h) == 0) {
		F_SET(pip, VRFY_IS_ALLZEROES);
		for (p = (u_int8_t *)h; p < (u_int8_t *)h + dbp->pgsize; p++)
			if (*p != 0) {
				F_CLR(pip, VRFY_IS_ALLZEROES);
				break;
			}
		/*
		 * Mark it as a hash, and we'll
		 * check that that makes sense structurally later.
		 * (The queue verification doesn't care, since queues
		 * don't really have much in the way of structure.)
		 */
		pip->type = P_HASH;
		ret = 0;
		goto err;	/* well, not really an err. */
	}

	if (PGNO(h) != pgno) {
		EPRINT((env, "Page %lu: bad page number %lu",
		    (u_long)pgno, (u_long)h->pgno));
		ret = DB_VERIFY_BAD;
	}

	switch (h->type) {
	case P_INVALID:			/* Order matches ordinal value. */
	case P_HASH_UNSORTED:
	case P_IBTREE:
	case P_IRECNO:
	case P_LBTREE:
	case P_LRECNO:
	case P_OVERFLOW:
	case P_HASHMETA:
	case P_BTREEMETA:
	case P_QAMMETA:
	case P_QAMDATA:
	case P_LDUP:
	case P_HASH:
		break;
	default:
		EPRINT((env, "Page %lu: bad page type %lu",
		    (u_long)pgno, (u_long)h->type));
		ret = DB_VERIFY_BAD;
	}
	pip->type = h->type;

err:	if ((t_ret = __db_vrfy_putpageinfo(env, vdp, pip)) != 0 && ret == 0)
		ret = t_ret;

	return (ret);
}

/*
 * __db_vrfy_invalid --
 *	Verify P_INVALID page.
 *	(Yes, there's not much to do here.)
 */
static int
__db_vrfy_invalid(dbp, vdp, h, pgno, flags)
	DB *dbp;
	VRFY_DBINFO *vdp;
	PAGE *h;
	db_pgno_t pgno;
	u_int32_t flags;
{
	ENV *env;
	VRFY_PAGEINFO *pip;
	int ret, t_ret;

	env = dbp->env;

	if ((ret = __db_vrfy_getpageinfo(vdp, pgno, &pip)) != 0)
		return (ret);
	pip->next_pgno = pip->prev_pgno = 0;

	if (!IS_VALID_PGNO(NEXT_PGNO(h))) {
		EPRINT((env, "Page %lu: invalid next_pgno %lu",
		    (u_long)pgno, (u_long)NEXT_PGNO(h)));
		ret = DB_VERIFY_BAD;
	} else
		pip->next_pgno = NEXT_PGNO(h);

	if ((t_ret = __db_vrfy_putpageinfo(env, vdp, pip)) != 0 && ret == 0)
		ret = t_ret;
	return (ret);
}

/*
 * __db_vrfy_datapage --
 *	Verify elements common to data pages (P_HASH, P_LBTREE,
 *	P_IBTREE, P_IRECNO, P_LRECNO, P_OVERFLOW, P_DUPLICATE)--i.e.,
 *	those defined in the PAGE structure.
 *
 *	Called from each of the per-page routines, after the
 *	all-page-type-common elements of pip have been verified and filled
 *	in.
 *
 * PUBLIC: int __db_vrfy_datapage
 * PUBLIC:     __P((DB *, VRFY_DBINFO *, PAGE *, db_pgno_t, u_int32_t));
 */
int
__db_vrfy_datapage(dbp, vdp, h, pgno, flags)
	DB *dbp;
	VRFY_DBINFO *vdp;
	PAGE *h;
	db_pgno_t pgno;
	u_int32_t flags;
{
	ENV *env;
	VRFY_PAGEINFO *pip;
	u_int32_t smallest_entry;
	int isbad, ret, t_ret;

	env = dbp->env;

	if ((ret = __db_vrfy_getpageinfo(vdp, pgno, &pip)) != 0)
		return (ret);
	isbad = 0;

	/*
	 * prev_pgno and next_pgno:  store for inter-page checks,
	 * verify that they point to actual pages and not to self.
	 *
	 * !!!
	 * Internal btree pages do not maintain these fields (indeed,
	 * they overload them).  Skip.
	 */
	if (TYPE(h) != P_IBTREE && TYPE(h) != P_IRECNO) {
		if (!IS_VALID_PGNO(PREV_PGNO(h)) || PREV_PGNO(h) == pip->pgno) {
			isbad = 1;
			EPRINT((env, "Page %lu: invalid prev_pgno %lu",
			    (u_long)pip->pgno, (u_long)PREV_PGNO(h)));
		}
		if (!IS_VALID_PGNO(NEXT_PGNO(h)) || NEXT_PGNO(h) == pip->pgno) {
			isbad = 1;
			EPRINT((env, "Page %lu: invalid next_pgno %lu",
			    (u_long)pip->pgno, (u_long)NEXT_PGNO(h)));
		}
		pip->prev_pgno = PREV_PGNO(h);
		pip->next_pgno = NEXT_PGNO(h);
	}

	/*
	 * Verify the number of entries on the page: there's no good way to
	 * determine if this is accurate.  The best we can do is verify that
	 * it's not more than can, in theory, fit on the page.  Then, we make
	 * sure there are at least this many valid elements in inp[], and
	 * hope the test catches most cases.
	 */
	switch (TYPE(h)) {
	case P_HASH_UNSORTED:
	case P_HASH:
		smallest_entry = HKEYDATA_PSIZE(0);
		break;
	case P_IBTREE:
		smallest_entry = BINTERNAL_PSIZE(0);
		break;
	case P_IRECNO:
		smallest_entry = RINTERNAL_PSIZE;
		break;
	case P_LBTREE:
	case P_LDUP:
	case P_LRECNO:
		smallest_entry = BKEYDATA_PSIZE(0);
		break;
	default:
		smallest_entry = 0;
		break;
	}
	if (smallest_entry * NUM_ENT(h) / 2 > dbp->pgsize) {
		isbad = 1;
		EPRINT((env, "Page %lu: too many entries: %lu",
		    (u_long)pgno, (u_long)NUM_ENT(h)));
	}

	if (TYPE(h) != P_OVERFLOW)
		pip->entries = NUM_ENT(h);

	/*
	 * btree level.  Should be zero unless we're a btree;
	 * if we are a btree, should be between LEAFLEVEL and MAXBTREELEVEL,
	 * and we need to save it off.
	 */
	switch (TYPE(h)) {
	case P_IBTREE:
	case P_IRECNO:
		if (LEVEL(h) < LEAFLEVEL + 1) {
			isbad = 1;
			EPRINT((env, "Page %lu: bad btree level %lu",
			    (u_long)pgno, (u_long)LEVEL(h)));
		}
		pip->bt_level = LEVEL(h);
		break;
	case P_LBTREE:
	case P_LDUP:
	case P_LRECNO:
		if (LEVEL(h) != LEAFLEVEL) {
			isbad = 1;
			EPRINT((env,
			    "Page %lu: btree leaf page has incorrect level %lu",
			    (u_long)pgno, (u_long)LEVEL(h)));
		}
		break;
	default:
		if (LEVEL(h) != 0) {
			isbad = 1;
			EPRINT((env,
			    "Page %lu: nonzero level %lu in non-btree database",
			    (u_long)pgno, (u_long)LEVEL(h)));
		}
		break;
	}

	/*
	 * Even though inp[] occurs in all PAGEs, we look at it in the
	 * access-method-specific code, since btree and hash treat
	 * item lengths very differently, and one of the most important
	 * things we want to verify is that the data--as specified
	 * by offset and length--cover the right part of the page
	 * without overlaps, gaps, or violations of the page boundary.
	 */
	if ((t_ret = __db_vrfy_putpageinfo(env, vdp, pip)) != 0 && ret == 0)
		ret = t_ret;

	return ((ret == 0 && isbad == 1) ? DB_VERIFY_BAD : ret);
}

/*
 * __db_vrfy_meta--
 *	Verify the access-method common parts of a meta page, using
 *	normal mpool routines.
 *
 * PUBLIC: int __db_vrfy_meta
 * PUBLIC:     __P((DB *, VRFY_DBINFO *, DBMETA *, db_pgno_t, u_int32_t));
 */
int
__db_vrfy_meta(dbp, vdp, meta, pgno, flags)
	DB *dbp;
	VRFY_DBINFO *vdp;
	DBMETA *meta;
	db_pgno_t pgno;
	u_int32_t flags;
{
	DBTYPE dbtype, magtype;
	ENV *env;
	VRFY_PAGEINFO *pip;
	int isbad, ret, t_ret;

	isbad = 0;
	env = dbp->env;

	if ((ret = __db_vrfy_getpageinfo(vdp, pgno, &pip)) != 0)
		return (ret);

	/* type plausible for a meta page */
	switch (meta->type) {
	case P_BTREEMETA:
		dbtype = DB_BTREE;
		break;
	case P_HASHMETA:
		dbtype = DB_HASH;
		break;
	case P_QAMMETA:
		dbtype = DB_QUEUE;
		break;
	default:
		ret = __db_unknown_path(env, "__db_vrfy_meta");
		goto err;
	}

	/* magic number valid */
	if (!__db_is_valid_magicno(meta->magic, &magtype)) {
		isbad = 1;
		EPRINT((env,
		    "Page %lu: invalid magic number", (u_long)pgno));
	}
	if (magtype != dbtype) {
		isbad = 1;
		EPRINT((env,
		    "Page %lu: magic number does not match database type",
		    (u_long)pgno));
	}

	/* version */
	if ((dbtype == DB_BTREE &&
	    (meta->version > DB_BTREEVERSION ||
	    meta->version < DB_BTREEOLDVER)) ||
	    (dbtype == DB_HASH &&
	    (meta->version > DB_HASHVERSION ||
	    meta->version < DB_HASHOLDVER)) ||
	    (dbtype == DB_QUEUE &&
	    (meta->version > DB_QAMVERSION ||
	    meta->version < DB_QAMOLDVER))) {
		isbad = 1;
		EPRINT((env,
    "Page %lu: unsupported database version %lu; extraneous errors may result",
		    (u_long)pgno, (u_long)meta->version));
	}

	/* pagesize */
	if (meta->pagesize != dbp->pgsize) {
		isbad = 1;
		EPRINT((env, "Page %lu: invalid pagesize %lu",
		    (u_long)pgno, (u_long)meta->pagesize));
	}

	/* Flags */
	if (meta->metaflags != 0) {
		if (FLD_ISSET(meta->metaflags,
		    ~(DBMETA_CHKSUM|DBMETA_PART_RANGE|DBMETA_PART_CALLBACK))) {
			isbad = 1;
			EPRINT((env,
			    "Page %lu: bad meta-data flags value %#lx",
			    (u_long)PGNO_BASE_MD, (u_long)meta->metaflags));
		}
		if (FLD_ISSET(meta->metaflags, DBMETA_CHKSUM))
			F_SET(pip, VRFY_HAS_CHKSUM);
		if (FLD_ISSET(meta->metaflags, DBMETA_PART_RANGE))
			F_SET(pip, VRFY_HAS_PART_RANGE);
		if (FLD_ISSET(meta->metaflags, DBMETA_PART_CALLBACK))
			F_SET(pip, VRFY_HAS_PART_CALLBACK);
	}

	/*
	 * Free list.
	 *
	 * If this is not the main, master-database meta page, it
	 * should not have a free list.
	 */
	if (pgno != PGNO_BASE_MD && meta->free != PGNO_INVALID) {
		isbad = 1;
		EPRINT((env,
		    "Page %lu: nonempty free list on subdatabase metadata page",
		    (u_long)pgno));
	}

	/* Can correctly be PGNO_INVALID--that's just the end of the list. */
	if (meta->free != PGNO_INVALID && IS_VALID_PGNO(meta->free))
		pip->free = meta->free;
	else if (!IS_VALID_PGNO(meta->free)) {
		isbad = 1;
		EPRINT((env,
		    "Page %lu: nonsensical free list pgno %lu",
		    (u_long)pgno, (u_long)meta->free));
	}

	/*
	 * Check that the meta page agrees with what we got from mpool.
	 * If we don't have FTRUNCATE then mpool could include some
	 * zeroed pages at the end of the file, we assume the meta page
	 * is correct.
	 */
	if (pgno == PGNO_BASE_MD && meta->last_pgno != vdp->last_pgno) {
#ifdef HAVE_FTRUNCATE
		isbad = 1;
		EPRINT((env,
		    "Page %lu: last_pgno is not correct: %lu != %lu",
		    (u_long)pgno,
		    (u_long)meta->last_pgno, (u_long)vdp->last_pgno));
#endif
		vdp->meta_last_pgno = meta->last_pgno;
	}

	/*
	 * We have now verified the common fields of the metadata page.
	 * Clear the flag that told us they had been incompletely checked.
	 */
	F_CLR(pip, VRFY_INCOMPLETE);

err:	if ((t_ret = __db_vrfy_putpageinfo(env, vdp, pip)) != 0 && ret == 0)
		ret = t_ret;

	return ((ret == 0 && isbad == 1) ? DB_VERIFY_BAD : ret);
}

/*
 * __db_vrfy_freelist --
 *	Walk free list, checking off pages and verifying absence of
 *	loops.
 */
static int
__db_vrfy_freelist(dbp, vdp, meta, flags)
	DB *dbp;
	VRFY_DBINFO *vdp;
	db_pgno_t meta;
	u_int32_t flags;
{
	DB *pgset;
	ENV *env;
	VRFY_PAGEINFO *pip;
	db_pgno_t cur_pgno, next_pgno;
	int p, ret, t_ret;

	env = dbp->env;
	pgset = vdp->pgset;
	DB_ASSERT(env, pgset != NULL);

	if ((ret = __db_vrfy_getpageinfo(vdp, meta, &pip)) != 0)
		return (ret);
	for (next_pgno = pip->free;
	    next_pgno != PGNO_INVALID; next_pgno = pip->next_pgno) {
		cur_pgno = pip->pgno;
		if ((ret = __db_vrfy_putpageinfo(env, vdp, pip)) != 0)
			return (ret);

		/* This shouldn't happen, but just in case. */
		if (!IS_VALID_PGNO(next_pgno)) {
			EPRINT((env,
			    "Page %lu: invalid next_pgno %lu on free list page",
			    (u_long)cur_pgno, (u_long)next_pgno));
			return (DB_VERIFY_BAD);
		}

		/* Detect cycles. */
		if ((ret = __db_vrfy_pgset_get(pgset,
		    vdp->thread_info, next_pgno, &p)) != 0)
			return (ret);
		if (p != 0) {
			EPRINT((env,
		    "Page %lu: page %lu encountered a second time on free list",
			    (u_long)cur_pgno, (u_long)next_pgno));
			return (DB_VERIFY_BAD);
		}
		if ((ret = __db_vrfy_pgset_inc(pgset,
		    vdp->thread_info, next_pgno)) != 0)
			return (ret);

		if ((ret = __db_vrfy_getpageinfo(vdp, next_pgno, &pip)) != 0)
			return (ret);

		if (pip->type != P_INVALID) {
			EPRINT((env,
			    "Page %lu: non-invalid page %lu on free list",
			    (u_long)cur_pgno, (u_long)next_pgno));
			ret = DB_VERIFY_BAD;	  /* unsafe to continue */
			break;
		}
	}

	if ((t_ret = __db_vrfy_putpageinfo(env, vdp, pip)) != 0)
		ret = t_ret;
	return (ret);
}

/*
 * __db_vrfy_subdbs --
 *	Walk the known-safe master database of subdbs with a cursor,
 *	verifying the structure of each subdatabase we encounter.
 */
static int
__db_vrfy_subdbs(dbp, vdp, dbname, flags)
	DB *dbp;
	VRFY_DBINFO *vdp;
	const char *dbname;
	u_int32_t flags;
{
	DB *mdbp;
	DBC *dbc;
	DBT key, data;
	ENV *env;
	VRFY_PAGEINFO *pip;
	db_pgno_t meta_pgno;
	int ret, t_ret, isbad;
	u_int8_t type;

	isbad = 0;
	dbc = NULL;
	env = dbp->env;

	if ((ret = __db_master_open(dbp,
	    vdp->thread_info, NULL, dbname, DB_RDONLY, 0, &mdbp)) != 0)
		return (ret);

	if ((ret = __db_cursor_int(mdbp, NULL,
	    NULL, DB_BTREE, PGNO_INVALID, 0, DB_LOCK_INVALIDID, &dbc)) != 0)
		goto err;

	memset(&key, 0, sizeof(key));
	memset(&data, 0, sizeof(data));
	while ((ret = __dbc_get(dbc, &key, &data, DB_NEXT)) == 0) {
		if (data.size != sizeof(db_pgno_t)) {
			EPRINT((env,
			    "Subdatabase entry not page-number size"));
			isbad = 1;
			goto err;
		}
		memcpy(&meta_pgno, data.data, data.size);
		/*
		 * Subdatabase meta pgnos are stored in network byte
		 * order for cross-endian compatibility.  Swap if appropriate.
		 */
		DB_NTOHL_SWAP(env, &meta_pgno);
		if (meta_pgno == PGNO_INVALID || meta_pgno > vdp->last_pgno) {
			EPRINT((env,
		    "Subdatabase entry references invalid page %lu",
			    (u_long)meta_pgno));
			isbad = 1;
			goto err;
		}
		if ((ret = __db_vrfy_getpageinfo(vdp, meta_pgno, &pip)) != 0)
			goto err;
		type = pip->type;
		if ((ret = __db_vrfy_putpageinfo(env, vdp, pip)) != 0)
			goto err;
		switch (type) {
		case P_BTREEMETA:
			if ((ret = __bam_vrfy_structure(
			    dbp, vdp, meta_pgno, NULL, NULL, flags)) != 0) {
				if (ret == DB_VERIFY_BAD)
					isbad = 1;
				else
					goto err;
			}
			break;
		case P_HASHMETA:
			if ((ret = __ham_vrfy_structure(
			    dbp, vdp, meta_pgno, flags)) != 0) {
				if (ret == DB_VERIFY_BAD)
					isbad = 1;
				else
					goto err;
			}
			break;
		case P_QAMMETA:
		default:
			EPRINT((env,
		    "Subdatabase entry references page %lu of invalid type %lu",
			    (u_long)meta_pgno, (u_long)type));
			ret = DB_VERIFY_BAD;
			goto err;
		}
	}

	if (ret == DB_NOTFOUND)
		ret = 0;

err:	if (dbc != NULL && (t_ret = __dbc_close(dbc)) != 0 && ret == 0)
		ret = t_ret;

	if ((t_ret = __db_close(mdbp, NULL, 0)) != 0 && ret == 0)
		ret = t_ret;

	return ((ret == 0 && isbad == 1) ? DB_VERIFY_BAD : ret);
}

/*
 * __db_vrfy_struct_feedback --
 *	Provide feedback during top-down database structure traversal.
 *	(See comment at the beginning of __db_vrfy_structure.)
 *
 * PUBLIC: void __db_vrfy_struct_feedback __P((DB *, VRFY_DBINFO *));
 */
void
__db_vrfy_struct_feedback(dbp, vdp)
	DB *dbp;
	VRFY_DBINFO *vdp;
{
	int progress;

	if (dbp->db_feedback == NULL)
		return;

	if (vdp->pgs_remaining > 0)
		vdp->pgs_remaining--;

	/* Don't allow a feedback call of 100 until we're really done. */
	progress = 100 - (int)(vdp->pgs_remaining * 50 / (vdp->last_pgno + 1));
	dbp->db_feedback(dbp, DB_VERIFY, progress == 100 ? 99 : progress);
}

/*
 * __db_vrfy_orderchkonly --
 *	Do an sort-order/hashing check on a known-otherwise-good subdb.
 */
static int
__db_vrfy_orderchkonly(dbp, vdp, name, subdb, flags)
	DB *dbp;
	VRFY_DBINFO *vdp;
	const char *name, *subdb;
	u_int32_t flags;
{
	BTMETA *btmeta;
	DB *mdbp, *pgset;
	DBC *pgsc;
	DBT key, data;
	DB_MPOOLFILE *mpf;
	ENV *env;
	HASH *h_internal;
	HMETA *hmeta;
	PAGE *h, *currpg;
	db_pgno_t meta_pgno, p, pgno;
	u_int32_t bucket;
	int t_ret, ret;

	pgset = NULL;
	pgsc = NULL;
	env = dbp->env;
	mpf = dbp->mpf;
	currpg = h = NULL;

	LF_CLR(DB_NOORDERCHK);

	/* Open the master database and get the meta_pgno for the subdb. */
	if ((ret = __db_master_open(dbp,
	    vdp->thread_info, NULL, name, DB_RDONLY, 0, &mdbp)) != 0)
		goto err;

	DB_INIT_DBT(key, subdb, strlen(subdb));
	memset(&data, 0, sizeof(data));
	if ((ret = __db_get(mdbp,
	    vdp->thread_info, NULL, &key, &data, 0)) != 0) {
		if (ret == DB_NOTFOUND)
			ret = ENOENT;
		goto err;
	}

	if (data.size != sizeof(db_pgno_t)) {
		EPRINT((env, "Subdatabase entry of invalid size"));
		ret = DB_VERIFY_BAD;
		goto err;
	}

	memcpy(&meta_pgno, data.data, data.size);

	/*
	 * Subdatabase meta pgnos are stored in network byte
	 * order for cross-endian compatibility.  Swap if appropriate.
	 */
	DB_NTOHL_SWAP(env, &meta_pgno);

	if ((ret = __memp_fget(mpf,
	     &meta_pgno, vdp->thread_info, NULL, 0, &h)) != 0)
		goto err;

	if ((ret = __db_vrfy_pgset(env,
	    vdp->thread_info, dbp->pgsize, &pgset)) != 0)
		goto err;

	switch (TYPE(h)) {
	case P_BTREEMETA:
		btmeta = (BTMETA *)h;
		if (F_ISSET(&btmeta->dbmeta, BTM_RECNO)) {
			/* Recnos have no order to check. */
			ret = 0;
			goto err;
		}
		if ((ret =
		    __db_meta2pgset(dbp, vdp, meta_pgno, flags, pgset)) != 0)
			goto err;
		if ((ret = __db_cursor_int(pgset, NULL, NULL, dbp->type,
		    PGNO_INVALID, 0, DB_LOCK_INVALIDID, &pgsc)) != 0)
			goto err;
		while ((ret = __db_vrfy_pgset_next(pgsc, &p)) == 0) {
			if ((ret = __memp_fget(mpf, &p,
			     vdp->thread_info, NULL, 0, &currpg)) != 0)
				goto err;
			if ((ret = __bam_vrfy_itemorder(dbp, NULL,
			    vdp->thread_info, currpg, p, NUM_ENT(currpg), 1,
			    F_ISSET(&btmeta->dbmeta, BTM_DUP), flags)) != 0)
				goto err;
			if ((ret = __memp_fput(mpf,
			    vdp->thread_info, currpg, dbp->priority)) != 0)
				goto err;
			currpg = NULL;
		}

		/*
		 * The normal exit condition for the loop above is DB_NOTFOUND.
		 * If we see that, zero it and continue on to cleanup.
		 * Otherwise, it's a real error and will be returned.
		 */
		if (ret == DB_NOTFOUND)
			ret = 0;
		break;
	case P_HASHMETA:
		hmeta = (HMETA *)h;
		h_internal = (HASH *)dbp->h_internal;
		/*
		 * Make sure h_charkey is right.
		 */
		if (h_internal == NULL) {
			EPRINT((env,
			    "Page %lu: DB->h_internal field is NULL",
			    (u_long)meta_pgno));
			ret = DB_VERIFY_BAD;
			goto err;
		}
		if (h_internal->h_hash == NULL)
			h_internal->h_hash = hmeta->dbmeta.version < 5
			? __ham_func4 : __ham_func5;
		if (hmeta->h_charkey !=
		    h_internal->h_hash(dbp, CHARKEY, sizeof(CHARKEY))) {
			EPRINT((env,
			    "Page %lu: incorrect hash function for database",
			    (u_long)meta_pgno));
			ret = DB_VERIFY_BAD;
			goto err;
		}

		/*
		 * Foreach bucket, verify hashing on each page in the
		 * corresponding chain of pages.
		 */
		if ((ret = __db_cursor_int(dbp, NULL, NULL, dbp->type,
		    PGNO_INVALID, 0, DB_LOCK_INVALIDID, &pgsc)) != 0)
			goto err;
		for (bucket = 0; bucket <= hmeta->max_bucket; bucket++) {
			pgno = BS_TO_PAGE(bucket, hmeta->spares);
			while (pgno != PGNO_INVALID) {
				if ((ret = __memp_fget(mpf, &pgno,
				    vdp->thread_info, NULL, 0, &currpg)) != 0)
					goto err;
				if ((ret = __ham_vrfy_hashing(pgsc,
				    NUM_ENT(currpg), hmeta, bucket, pgno,
				    flags, h_internal->h_hash)) != 0)
					goto err;
				pgno = NEXT_PGNO(currpg);
				if ((ret = __memp_fput(mpf, vdp->thread_info,
				    currpg, dbp->priority)) != 0)
					goto err;
				currpg = NULL;
			}
		}
		break;
	default:
		EPRINT((env, "Page %lu: database metapage of bad type %lu",
		    (u_long)meta_pgno, (u_long)TYPE(h)));
		ret = DB_VERIFY_BAD;
		break;
	}

err:	if (pgsc != NULL && (t_ret = __dbc_close(pgsc)) != 0 && ret == 0)
		ret = t_ret;
	if (pgset != NULL &&
	    (t_ret = __db_close(pgset, NULL, 0)) != 0 && ret == 0)
		ret = t_ret;
	if (h != NULL && (t_ret = __memp_fput(mpf,
	    vdp->thread_info, h, dbp->priority)) != 0)
		ret = t_ret;
	if (currpg != NULL &&
	    (t_ret = __memp_fput(mpf,
		vdp->thread_info, currpg, dbp->priority)) != 0)
		ret = t_ret;
	if ((t_ret = __db_close(mdbp, NULL, 0)) != 0)
		ret = t_ret;
	return (ret);
}

/*
 * __db_salvage_pg --
 *	Walk through a page, salvaging all likely or plausible (w/
 *	DB_AGGRESSIVE) key/data pairs and marking seen pages in vdp.
 *
 * PUBLIC: int __db_salvage_pg __P((DB *, VRFY_DBINFO *, db_pgno_t,
 * PUBLIC:     PAGE *, void *, int (*)(void *, const void *), u_int32_t));
 */
int
__db_salvage_pg(dbp, vdp, pgno, h, handle, callback, flags)
	DB *dbp;
	VRFY_DBINFO *vdp;
	db_pgno_t pgno;
	PAGE *h;
	void *handle;
	int (*callback) __P((void *, const void *));
	u_int32_t flags;
{
	ENV *env;
	VRFY_PAGEINFO *pip;
	int keyflag, ret, t_ret;

	env = dbp->env;
	DB_ASSERT(env, LF_ISSET(DB_SALVAGE));

	/*
	 * !!!
	 * We dump record numbers when salvaging Queue databases, but not for
	 * immutable Recno databases.  The problem is we can't figure out the
	 * record number from the database page in the Recno case, while the
	 * offset in the file is sufficient for Queue.
	 */
	keyflag = 0;

	/* If we got this page in the subdb pass, we can safely skip it. */
	if (__db_salvage_isdone(vdp, pgno))
		return (0);

	switch (TYPE(h)) {
	case P_BTREEMETA:
		ret = __bam_vrfy_meta(dbp, vdp, (BTMETA *)h, pgno, flags);
		break;
	case P_HASH:
	case P_HASH_UNSORTED:
	case P_LBTREE:
	case P_QAMDATA:
		return (__db_salvage_leaf(dbp,
		    vdp, pgno, h, handle, callback, flags));
	case P_HASHMETA:
		ret = __ham_vrfy_meta(dbp, vdp, (HMETA *)h, pgno, flags);
		break;
	case P_IBTREE:
		/*
		 * We need to mark any overflow keys on internal pages as seen,
		 * so we don't print them out in __db_salvage_unknowns.  But if
		 * we're an upgraded database, a P_LBTREE page may very well
		 * have a reference to the same overflow pages (this practice
		 * stopped somewhere around db4.5).  To give P_LBTREEs a chance
		 * to print out any keys on shared pages, mark the page now and
		 * deal with it at the end.
		 */
		return (__db_salvage_markneeded(vdp, pgno, SALVAGE_IBTREE));
	case P_LDUP:
		return (__db_salvage_markneeded(vdp, pgno, SALVAGE_LDUP));
	case P_LRECNO:
		/*
		 * Recno leaves are tough, because the leaf could be (1) a dup
		 * page, or it could be (2) a regular database leaf page.
		 * Fortunately, RECNO databases are not allowed to have
		 * duplicates.
		 *
		 * If there are no subdatabases, dump the page immediately if
		 * it's a leaf in a RECNO database, otherwise wait and hopefully
		 * it will be dumped by the leaf page that refers to it,
		 * otherwise we'll get it with the unknowns.
		 *
		 * If there are subdatabases, there might be mixed types and
		 * dbp->type can't be trusted.  We'll only get here after
		 * salvaging each database, though, so salvaging this page
		 * immediately isn't important.  If this page is a dup, it might
		 * get salvaged later on, otherwise the unknowns pass will pick
		 * it up.  Note that SALVAGE_HASSUBDBS won't get set if we're
		 * salvaging aggressively.
		 *
		 * If we're salvaging aggressively, we don't know whether or not
		 * there's subdatabases, so we wait on all recno pages.
		 */
		if (!LF_ISSET(DB_AGGRESSIVE) &&
		    !F_ISSET(vdp, SALVAGE_HASSUBDBS) && dbp->type == DB_RECNO)
			return (__db_salvage_leaf(dbp,
			    vdp, pgno, h, handle, callback, flags));
		return (__db_salvage_markneeded(vdp, pgno, SALVAGE_LRECNODUP));
	case P_OVERFLOW:
		return (__db_salvage_markneeded(vdp, pgno, SALVAGE_OVERFLOW));
	case P_QAMMETA:
		keyflag = 1;
		ret = __qam_vrfy_meta(dbp, vdp, (QMETA *)h, pgno, flags);
		break;
	case P_INVALID:
	case P_IRECNO:
	case __P_DUPLICATE:
	default:
		/*
		 * There's no need to display an error, the page type was
		 * already checked and reported on.
		 */
		return (0);
	}
	if (ret != 0)
		return (ret);

	/*
	 * We have to display the dump header if it's a metadata page.  It's
	 * our last chance as the page was marked "seen" in the vrfy routine,
	 * and  we won't see the page again.  We don't display headers for
	 * the first database in a multi-database file, that database simply
	 * contains a list of subdatabases.
	 */
	if ((ret = __db_vrfy_getpageinfo(vdp, pgno, &pip)) != 0)
		return (ret);
	if (!F_ISSET(pip, VRFY_HAS_SUBDBS) && !LF_ISSET(DB_VERIFY_PARTITION))
		ret = __db_prheader(
		    dbp, NULL, 0, keyflag, handle, callback, vdp, pgno);
	if ((t_ret = __db_vrfy_putpageinfo(env, vdp, pip)) != 0 && ret == 0)
		ret = t_ret;
	return (ret);
}

/*
 * __db_salvage_leaf --
 *	Walk through a leaf, salvaging all likely key/data pairs and marking
 *	seen pages in vdp.
 *
 * PUBLIC: int __db_salvage_leaf __P((DB *, VRFY_DBINFO *, db_pgno_t,
 * PUBLIC:     PAGE *, void *, int (*)(void *, const void *), u_int32_t));
 */
int
__db_salvage_leaf(dbp, vdp, pgno, h, handle, callback, flags)
	DB *dbp;
	VRFY_DBINFO *vdp;
	db_pgno_t pgno;
	PAGE *h;
	void *handle;
	int (*callback) __P((void *, const void *));
	u_int32_t flags;
{
	ENV *env;

	env = dbp->env;
	DB_ASSERT(env, LF_ISSET(DB_SALVAGE));

	/* If we got this page in the subdb pass, we can safely skip it. */
	if (__db_salvage_isdone(vdp, pgno))
		return (0);

	switch (TYPE(h)) {
	case P_HASH_UNSORTED:
	case P_HASH:
		return (__ham_salvage(dbp, vdp,
		    pgno, h, handle, callback, flags));
	case P_LBTREE:
	case P_LRECNO:
		return (__bam_salvage(dbp, vdp,
		    pgno, TYPE(h), h, handle, callback, NULL, flags));
	case P_QAMDATA:
		return (__qam_salvage(dbp, vdp,
		    pgno, h, handle, callback, flags));
	default:
		/*
		 * There's no need to display an error, the page type was
		 * already checked and reported on.
		 */
		return (0);
	}
}

/*
 * __db_salvage_unknowns --
 *	Walk through the salvager database, printing with key "UNKNOWN"
 *	any pages we haven't dealt with.
 */
static int
__db_salvage_unknowns(dbp, vdp, handle, callback, flags)
	DB *dbp;
	VRFY_DBINFO *vdp;
	void *handle;
	int (*callback) __P((void *, const void *));
	u_int32_t flags;
{
	DBC *dbc;
	DBT unkdbt, key, *dbt;
	DB_MPOOLFILE *mpf;
	ENV *env;
	PAGE *h;
	db_pgno_t pgno;
	u_int32_t pgtype, ovfl_bufsz, tmp_flags;
	int ret, t_ret;
	void *ovflbuf;

	dbc = NULL;
	env = dbp->env;
	mpf = dbp->mpf;

	DB_INIT_DBT(unkdbt, "UNKNOWN", sizeof("UNKNOWN") - 1);

	if ((ret = __os_malloc(env, dbp->pgsize, &ovflbuf)) != 0)
		return (ret);
	ovfl_bufsz = dbp->pgsize;

	/*
	 * We make two passes -- in the first pass, skip SALVAGE_OVERFLOW
	 * pages, because they may be referenced by the standard database
	 * pages that we're resolving.
	 */
	while ((t_ret =
	    __db_salvage_getnext(vdp, &dbc, &pgno, &pgtype, 1)) == 0) {
		if ((t_ret = __memp_fget(mpf,
		    &pgno, vdp->thread_info, NULL, 0, &h)) != 0) {
			if (ret == 0)
				ret = t_ret;
			continue;
		}

		dbt = NULL;
		tmp_flags = 0;
		switch (pgtype) {
		case SALVAGE_LDUP:
		case SALVAGE_LRECNODUP:
			dbt = &unkdbt;
			tmp_flags = DB_SA_UNKNOWNKEY;
			/* FALLTHROUGH */
		case SALVAGE_IBTREE:
		case SALVAGE_LBTREE:
		case SALVAGE_LRECNO:
			if ((t_ret = __bam_salvage(
			    dbp, vdp, pgno, pgtype, h, handle,
			    callback, dbt, tmp_flags | flags)) != 0 && ret == 0)
				ret = t_ret;
			break;
		case SALVAGE_OVERFLOW:
			DB_ASSERT(env, 0);	/* Shouldn't ever happen. */
			break;
		case SALVAGE_HASH:
			if ((t_ret = __ham_salvage(dbp, vdp,
			    pgno, h, handle, callback, flags)) != 0 && ret == 0)
				ret = t_ret;
			break;
		case SALVAGE_INVALID:
		case SALVAGE_IGNORE:
		default:
			/*
			 * Shouldn't happen, but if it does, just do what the
			 * nice man says.
			 */
			DB_ASSERT(env, 0);
			break;
		}
		if ((t_ret = __memp_fput(mpf,
		    vdp->thread_info, h, dbp->priority)) != 0 && ret == 0)
			ret = t_ret;
	}

	/* We should have reached the end of the database. */
	if (t_ret == DB_NOTFOUND)
		t_ret = 0;
	if (t_ret != 0 && ret == 0)
		ret = t_ret;

	/* Re-open the cursor so we traverse the database again. */
	if ((t_ret = __dbc_close(dbc)) != 0 && ret == 0)
		ret = t_ret;
	dbc = NULL;

	/* Now, deal with any remaining overflow pages. */
	while ((t_ret =
	    __db_salvage_getnext(vdp, &dbc, &pgno, &pgtype, 0)) == 0) {
		if ((t_ret = __memp_fget(mpf,
		    &pgno, vdp->thread_info, NULL, 0, &h)) != 0) {
			if (ret == 0)
				ret = t_ret;
			continue;
		}

		switch (pgtype) {
		case SALVAGE_OVERFLOW:
			/*
			 * XXX:
			 * This may generate multiple "UNKNOWN" keys in
			 * a database with no dups.  What to do?
			 */
			if ((t_ret = __db_safe_goff(dbp, vdp,
			    pgno, &key, &ovflbuf, &ovfl_bufsz, flags)) != 0 ||
			    ((vdp->type == DB_BTREE || vdp->type == DB_HASH) &&
			    (t_ret = __db_vrfy_prdbt(&unkdbt,
			    0, " ", handle, callback, 0, vdp)) != 0) ||
			    (t_ret = __db_vrfy_prdbt(
			    &key, 0, " ", handle, callback, 0, vdp)) != 0)
				if (ret == 0)
					ret = t_ret;
			break;
		default:
			DB_ASSERT(env, 0);	/* Shouldn't ever happen. */
			break;
		}
		if ((t_ret = __memp_fput(mpf,
		    vdp->thread_info, h, dbp->priority)) != 0 && ret == 0)
			ret = t_ret;
	}

	/* We should have reached the end of the database. */
	if (t_ret == DB_NOTFOUND)
		t_ret = 0;
	if (t_ret != 0 && ret == 0)
		ret = t_ret;

	if ((t_ret = __dbc_close(dbc)) != 0 && ret == 0)
		ret = t_ret;

	__os_free(env, ovflbuf);

	return (ret);
}

/*
 * Offset of the ith inp array entry, which we can compare to the offset
 * the entry stores.
 */
#define	INP_OFFSET(dbp, h, i)	\
    ((db_indx_t)((u_int8_t *)((P_INP(dbp,(h))) + (i)) - (u_int8_t *)(h)))

/*
 * __db_vrfy_inpitem --
 *	Verify that a single entry in the inp array is sane, and update
 *	the high water mark and current item offset.  (The former of these is
 *	used for state information between calls, and is required;  it must
 *	be initialized to the pagesize before the first call.)
 *
 *	Returns DB_VERIFY_FATAL if inp has collided with the data,
 *	since verification can't continue from there;  returns DB_VERIFY_BAD
 *	if anything else is wrong.
 *
 * PUBLIC: int __db_vrfy_inpitem __P((DB *, PAGE *,
 * PUBLIC:     db_pgno_t, u_int32_t, int, u_int32_t, u_int32_t *, u_int32_t *));
 */
int
__db_vrfy_inpitem(dbp, h, pgno, i, is_btree, flags, himarkp, offsetp)
	DB *dbp;
	PAGE *h;
	db_pgno_t pgno;
	u_int32_t i;
	int is_btree;
	u_int32_t flags, *himarkp, *offsetp;
{
	BKEYDATA *bk;
	ENV *env;
	db_indx_t *inp, offset, len;

	env = dbp->env;

	DB_ASSERT(env, himarkp != NULL);
	inp = P_INP(dbp, h);

	/*
	 * Check that the inp array, which grows from the beginning of the
	 * page forward, has not collided with the data, which grow from the
	 * end of the page backward.
	 */
	if (inp + i >= (db_indx_t *)((u_int8_t *)h + *himarkp)) {
		/* We've collided with the data.  We need to bail. */
		EPRINT((env, "Page %lu: entries listing %lu overlaps data",
		    (u_long)pgno, (u_long)i));
		return (DB_VERIFY_FATAL);
	}

	offset = inp[i];

	/*
	 * Check that the item offset is reasonable:  it points somewhere
	 * after the inp array and before the end of the page.
	 */
	if (offset <= INP_OFFSET(dbp, h, i) || offset >= dbp->pgsize) {
		EPRINT((env, "Page %lu: bad offset %lu at page index %lu",
		    (u_long)pgno, (u_long)offset, (u_long)i));
		return (DB_VERIFY_BAD);
	}

	/* Update the high-water mark (what HOFFSET should be) */
	if (offset < *himarkp)
		*himarkp = offset;

	if (is_btree) {
		/*
		 * Check alignment;  if it's unaligned, it's unsafe to
		 * manipulate this item.
		 */
		if (offset != DB_ALIGN(offset, sizeof(u_int32_t))) {
			EPRINT((env,
			    "Page %lu: unaligned offset %lu at page index %lu",
			    (u_long)pgno, (u_long)offset, (u_long)i));
			return (DB_VERIFY_BAD);
		}

		/*
		 * Check that the item length remains on-page.
		 */
		bk = GET_BKEYDATA(dbp, h, i);

		/*
		 * We need to verify the type of the item here;
		 * we can't simply assume that it will be one of the
		 * expected three.  If it's not a recognizable type,
		 * it can't be considered to have a verifiable
		 * length, so it's not possible to certify it as safe.
		 */
		switch (B_TYPE(bk->type)) {
		case B_KEYDATA:
			len = bk->len;
			break;
		case B_DUPLICATE:
		case B_OVERFLOW:
			len = BOVERFLOW_SIZE;
			break;
		default:
			EPRINT((env,
			    "Page %lu: item %lu of unrecognizable type",
			    (u_long)pgno, (u_long)i));
			return (DB_VERIFY_BAD);
		}

		if ((size_t)(offset + len) > dbp->pgsize) {
			EPRINT((env,
			    "Page %lu: item %lu extends past page boundary",
			    (u_long)pgno, (u_long)i));
			return (DB_VERIFY_BAD);
		}
	}

	if (offsetp != NULL)
		*offsetp = offset;
	return (0);
}

/*
 * __db_vrfy_duptype--
 *	Given a page number and a set of flags to __bam_vrfy_subtree,
 *	verify that the dup tree type is correct--i.e., it's a recno
 *	if DUPSORT is not set and a btree if it is.
 *
 * PUBLIC: int __db_vrfy_duptype
 * PUBLIC:     __P((DB *, VRFY_DBINFO *, db_pgno_t, u_int32_t));
 */
int
__db_vrfy_duptype(dbp, vdp, pgno, flags)
	DB *dbp;
	VRFY_DBINFO *vdp;
	db_pgno_t pgno;
	u_int32_t flags;
{
	ENV *env;
	VRFY_PAGEINFO *pip;
	int ret, isbad;

	env = dbp->env;
	isbad = 0;

	if ((ret = __db_vrfy_getpageinfo(vdp, pgno, &pip)) != 0)
		return (ret);

	switch (pip->type) {
	case P_IBTREE:
	case P_LDUP:
		if (!LF_ISSET(DB_ST_DUPSORT)) {
			EPRINT((env,
	    "Page %lu: sorted duplicate set in unsorted-dup database",
			    (u_long)pgno));
			isbad = 1;
		}
		break;
	case P_IRECNO:
	case P_LRECNO:
		if (LF_ISSET(DB_ST_DUPSORT)) {
			EPRINT((env,
	    "Page %lu: unsorted duplicate set in sorted-dup database",
			    (u_long)pgno));
			isbad = 1;
		}
		break;
	default:
		/*
		 * If the page is entirely zeroed, its pip->type will be a lie
		 * (we assumed it was a hash page, as they're allowed to be
		 * zeroed);  handle this case specially.
		 */
		if (F_ISSET(pip, VRFY_IS_ALLZEROES))
			ZEROPG_ERR_PRINT(env, pgno, "duplicate page");
		else
			EPRINT((env,
		    "Page %lu: duplicate page of inappropriate type %lu",
			    (u_long)pgno, (u_long)pip->type));
		isbad = 1;
		break;
	}

	if ((ret = __db_vrfy_putpageinfo(env, vdp, pip)) != 0)
		return (ret);
	return (isbad == 1 ? DB_VERIFY_BAD : 0);
}

/*
 * __db_salvage_duptree --
 *	Attempt to salvage a given duplicate tree, given its alleged root.
 *
 *	The key that corresponds to this dup set has been passed to us
 *	in DBT *key.  Because data items follow keys, though, it has been
 *	printed once already.
 *
 *	The basic idea here is that pgno ought to be a P_LDUP, a P_LRECNO, a
 *	P_IBTREE, or a P_IRECNO.  If it's an internal page, use the verifier
 *	functions to make sure it's safe;  if it's not, we simply bail and the
 *	data will have to be printed with no key later on.  if it is safe,
 *	recurse on each of its children.
 *
 *	Whether or not it's safe, if it's a leaf page, __bam_salvage it.
 *
 *	At all times, use the DB hanging off vdp to mark and check what we've
 *	done, so each page gets printed exactly once and we don't get caught
 *	in any cycles.
 *
 * PUBLIC: int __db_salvage_duptree __P((DB *, VRFY_DBINFO *, db_pgno_t,
 * PUBLIC:     DBT *, void *, int (*)(void *, const void *), u_int32_t));
 */
int
__db_salvage_duptree(dbp, vdp, pgno, key, handle, callback, flags)
	DB *dbp;
	VRFY_DBINFO *vdp;
	db_pgno_t pgno;
	DBT *key;
	void *handle;
	int (*callback) __P((void *, const void *));
	u_int32_t flags;
{
	DB_MPOOLFILE *mpf;
	PAGE *h;
	int ret, t_ret;

	mpf = dbp->mpf;

	if (pgno == PGNO_INVALID || !IS_VALID_PGNO(pgno))
		return (DB_VERIFY_BAD);

	/* We have a plausible page.  Try it. */
	if ((ret = __memp_fget(mpf, &pgno, vdp->thread_info, NULL, 0, &h)) != 0)
		return (ret);

	switch (TYPE(h)) {
	case P_IBTREE:
	case P_IRECNO:
		if ((ret = __db_vrfy_common(dbp, vdp, h, pgno, flags)) != 0)
			goto err;
		if ((ret = __bam_vrfy(dbp,
		    vdp, h, pgno, flags | DB_NOORDERCHK)) != 0 ||
		    (ret = __db_salvage_markdone(vdp, pgno)) != 0)
			goto err;
		/*
		 * We have a known-healthy internal page.  Walk it.
		 */
		if ((ret = __bam_salvage_walkdupint(dbp, vdp, h, key,
		    handle, callback, flags)) != 0)
			goto err;
		break;
	case P_LRECNO:
	case P_LDUP:
		if ((ret = __bam_salvage(dbp,
		    vdp, pgno, TYPE(h), h, handle, callback, key, flags)) != 0)
			goto err;
		break;
	default:
		ret = DB_VERIFY_BAD;
		goto err;
	}

err:	if ((t_ret = __memp_fput(mpf,
	     vdp->thread_info, h, dbp->priority)) != 0 && ret == 0)
		ret = t_ret;
	return (ret);
}

/*
 * __db_salvage_all --
 *	Salvage only the leaves we find by walking the tree.  If we have subdbs,
 *	salvage each of them individually.
 */
static int
__db_salvage_all(dbp, vdp, handle, callback, flags, hassubsp)
	DB *dbp;
	VRFY_DBINFO *vdp;
	void *handle;
	int (*callback) __P((void *, const void *));
	u_int32_t flags;
	int *hassubsp;
{
	DB *pgset;
	DBC *pgsc;
	DB_MPOOLFILE *mpf;
	ENV *env;
	PAGE *h;
	VRFY_PAGEINFO *pip;
	db_pgno_t p, meta_pgno;
	int ret, t_ret;

	*hassubsp = 0;

	env = dbp->env;
	pgset = NULL;
	pgsc = NULL;
	mpf = dbp->mpf;
	h = NULL;
	pip = NULL;
	ret = 0;

	/*
	 * Check to make sure the page is OK and find out if it contains
	 * subdatabases.
	 */
	meta_pgno = PGNO_BASE_MD;
	if ((t_ret = __memp_fget(mpf,
	    &meta_pgno, vdp->thread_info, NULL, 0, &h)) == 0 &&
	    (t_ret = __db_vrfy_common(dbp, vdp, h, PGNO_BASE_MD, flags)) == 0 &&
	    (t_ret = __db_salvage_pg(
		dbp, vdp, PGNO_BASE_MD, h, handle, callback, flags)) == 0 &&
	    (t_ret = __db_vrfy_getpageinfo(vdp, 0, &pip)) == 0)
		if (F_ISSET(pip, VRFY_HAS_SUBDBS))
			*hassubsp = 1;
	if (pip != NULL &&
	    (t_ret = __db_vrfy_putpageinfo(env, vdp, pip)) != 0 && ret == 0)
		ret = t_ret;
	if (h != NULL) {
		if ((t_ret = __memp_fput(mpf,
		     vdp->thread_info, h, dbp->priority)) != 0 && ret == 0)
			ret = t_ret;
		h = NULL;
	}
	if (ret != 0)
		return (ret);

	/* Without subdatabases, we can just dump from the meta pgno. */
	if (*hassubsp == 0)
		return (__db_salvage(dbp,
		    vdp, PGNO_BASE_MD, handle, callback, flags));

	/*
	 * We have subdbs.  Try to crack them.
	 *
	 * To do so, get a set of leaf pages in the master database, and then
	 * walk each of the valid ones, salvaging subdbs as we go.  If any
	 * prove invalid, just drop them;  we'll pick them up on a later pass.
	 */
	if ((ret = __db_vrfy_pgset(env,
	    vdp->thread_info, dbp->pgsize, &pgset)) != 0)
		goto err;
	if ((ret = __db_meta2pgset(dbp, vdp, PGNO_BASE_MD, flags, pgset)) != 0)
		goto err;
	if ((ret = __db_cursor(pgset, vdp->thread_info, NULL, &pgsc, 0)) != 0)
		goto err;
	while ((t_ret = __db_vrfy_pgset_next(pgsc, &p)) == 0) {
		if ((t_ret = __memp_fget(mpf,
		    &p, vdp->thread_info, NULL, 0, &h)) == 0 &&
		    (t_ret = __db_vrfy_common(dbp, vdp, h, p, flags)) == 0 &&
		    (t_ret =
		    __bam_vrfy(dbp, vdp, h, p, flags | DB_NOORDERCHK)) == 0)
			t_ret = __db_salvage_subdbpg(
			    dbp, vdp, h, handle, callback, flags);
		if (t_ret != 0 && ret == 0)
			ret = t_ret;
		if (h != NULL) {
			if ((t_ret = __memp_fput(mpf, vdp->thread_info,
			    h, dbp->priority)) != 0 && ret == 0)
				ret = t_ret;
			h = NULL;
		}
	}

	if (t_ret != DB_NOTFOUND && ret == 0)
		ret = t_ret;

err:	if (pgsc != NULL && (t_ret = __dbc_close(pgsc)) != 0 && ret == 0)
		ret = t_ret;
	if (pgset != NULL &&
	    (t_ret = __db_close(pgset, NULL, 0)) != 0 && ret ==0)
		ret = t_ret;
	if (h != NULL &&
	    (t_ret = __memp_fput(mpf,
		vdp->thread_info, h, dbp->priority)) != 0 && ret == 0)
		ret = t_ret;
	return (ret);
}

/*
 * __db_salvage_subdbpg --
 *	Given a known-good leaf page in the master database, salvage all
 *	leaf pages corresponding to each subdb.
 */
static int
__db_salvage_subdbpg(dbp, vdp, master, handle, callback, flags)
	DB *dbp;
	VRFY_DBINFO *vdp;
	PAGE *master;
	void *handle;
	int (*callback) __P((void *, const void *));
	u_int32_t flags;
{
	BKEYDATA *bkkey, *bkdata;
	BOVERFLOW *bo;
	DB *pgset;
	DBC *pgsc;
	DBT key;
	DB_MPOOLFILE *mpf;
	ENV *env;
	PAGE *subpg;
	db_indx_t i;
	db_pgno_t meta_pgno;
	int ret, err_ret, t_ret;
	char *subdbname;
	u_int32_t ovfl_bufsz;

	env = dbp->env;
	mpf = dbp->mpf;
	ret = err_ret = 0;
	subdbname = NULL;
	pgsc = NULL;
	pgset = NULL;
	ovfl_bufsz = 0;

	/*
	 * For each entry, get and salvage the set of pages
	 * corresponding to that entry.
	 */
	for (i = 0; i < NUM_ENT(master); i += P_INDX) {
		bkkey = GET_BKEYDATA(dbp, master, i);
		bkdata = GET_BKEYDATA(dbp, master, i + O_INDX);

		/* Get the subdatabase name. */
		if (B_TYPE(bkkey->type) == B_OVERFLOW) {
			/*
			 * We can, in principle anyway, have a subdb
			 * name so long it overflows.  Ick.
			 */
			bo = (BOVERFLOW *)bkkey;
			if ((ret = __db_safe_goff(dbp, vdp, bo->pgno,
			    &key, &subdbname, &ovfl_bufsz, flags)) != 0) {
				err_ret = DB_VERIFY_BAD;
				continue;
			}

			/* Nul-terminate it. */
			if (ovfl_bufsz < key.size + 1) {
				if ((ret = __os_realloc(env,
				    key.size + 1, &subdbname)) != 0)
					goto err;
				ovfl_bufsz = key.size + 1;
			}
			subdbname[key.size] = '\0';
		} else if (B_TYPE(bkkey->type) == B_KEYDATA) {
			if (ovfl_bufsz < (u_int32_t)bkkey->len + 1) {
				if ((ret = __os_realloc(env,
				    bkkey->len + 1, &subdbname)) != 0)
					goto err;
				ovfl_bufsz = bkkey->len + 1;
			}
			DB_ASSERT(env, subdbname != NULL);
			memcpy(subdbname, bkkey->data, bkkey->len);
			subdbname[bkkey->len] = '\0';
		}

		/* Get the corresponding pgno. */
		if (bkdata->len != sizeof(db_pgno_t)) {
			err_ret = DB_VERIFY_BAD;
			continue;
		}
		memcpy(&meta_pgno,
		    (db_pgno_t *)bkdata->data, sizeof(db_pgno_t));

		/*
		 * Subdatabase meta pgnos are stored in network byte
		 * order for cross-endian compatibility.  Swap if appropriate.
		 */
		DB_NTOHL_SWAP(env, &meta_pgno);

		/* If we can't get the subdb meta page, just skip the subdb. */
		if (!IS_VALID_PGNO(meta_pgno) || (ret = __memp_fget(mpf,
		    &meta_pgno, vdp->thread_info, NULL, 0, &subpg)) != 0) {
			err_ret = ret;
			continue;
		}

		/*
		 * Verify the subdatabase meta page.  This has two functions.
		 * First, if it's bad, we have no choice but to skip the subdb
		 * and let the pages just get printed on a later pass.  Second,
		 * the access-method-specific meta verification routines record
		 * the various state info (such as the presence of dups)
		 * that we need for __db_prheader().
		 */
		if ((ret =
		    __db_vrfy_common(dbp, vdp, subpg, meta_pgno, flags)) != 0) {
			err_ret = ret;
			(void)__memp_fput(mpf,
			    vdp->thread_info, subpg, dbp->priority);
			continue;
		}
		switch (TYPE(subpg)) {
		case P_BTREEMETA:
			if ((ret = __bam_vrfy_meta(dbp,
			    vdp, (BTMETA *)subpg, meta_pgno, flags)) != 0) {
				err_ret = ret;
				(void)__memp_fput(mpf,
				    vdp->thread_info, subpg, dbp->priority);
				continue;
			}
			break;
		case P_HASHMETA:
			if ((ret = __ham_vrfy_meta(dbp,
			    vdp, (HMETA *)subpg, meta_pgno, flags)) != 0) {
				err_ret = ret;
				(void)__memp_fput(mpf,
				    vdp->thread_info, subpg, dbp->priority);
				continue;
			}
			break;
		default:
			/* This isn't an appropriate page;  skip this subdb. */
			err_ret = DB_VERIFY_BAD;
			continue;
		}

		if ((ret = __memp_fput(mpf,
		    vdp->thread_info, subpg, dbp->priority)) != 0) {
			err_ret = ret;
			continue;
		}

		/* Print a subdatabase header. */
		if ((ret = __db_prheader(dbp,
		    subdbname, 0, 0, handle, callback, vdp, meta_pgno)) != 0)
			goto err;

		/* Salvage meta_pgno's tree. */
		if ((ret = __db_salvage(dbp,
		    vdp, meta_pgno, handle, callback, flags)) != 0)
			err_ret = ret;

		/* Print a subdatabase footer. */
		if ((ret = __db_prfooter(handle, callback)) != 0)
			goto err;
	}

err:	if (subdbname)
		__os_free(env, subdbname);

	if (pgsc != NULL && (t_ret = __dbc_close(pgsc)) != 0)
		ret = t_ret;

	if (pgset != NULL && (t_ret = __db_close(pgset, NULL, 0)) != 0)
		ret = t_ret;

	if ((t_ret = __db_salvage_markdone(vdp, PGNO(master))) != 0)
		return (t_ret);

	return ((err_ret != 0) ? err_ret : ret);
}

/*
 * __db_salvage --
 *      Given a meta page number, salvage all data from leaf pages found by
 *      walking the meta page's tree.
 */
static int
__db_salvage(dbp, vdp, meta_pgno, handle, callback, flags)
     DB *dbp;
     VRFY_DBINFO *vdp;
     db_pgno_t meta_pgno;
     void *handle;
     int (*callback) __P((void *, const void *));
     u_int32_t flags;

{
	DB *pgset;
	DBC *dbc, *pgsc;
	DB_MPOOLFILE *mpf;
	ENV *env;
	PAGE *subpg;
	db_pgno_t p;
	int err_ret, ret, t_ret;

	env = dbp->env;
	mpf = dbp->mpf;
	err_ret = ret = t_ret = 0;
	pgsc = NULL;
	pgset = NULL;
	dbc = NULL;

	if ((ret = __db_vrfy_pgset(env,
	    vdp->thread_info, dbp->pgsize, &pgset)) != 0)
		goto err;

	/* Get all page numbers referenced from this meta page. */
	if ((ret = __db_meta2pgset(dbp, vdp, meta_pgno,
	    flags, pgset)) != 0) {
		err_ret = ret;
		goto err;
	}

	if ((ret = __db_cursor(pgset,
	    vdp->thread_info, NULL, &pgsc, 0)) != 0)
		goto err;

	if (dbp->type == DB_QUEUE &&
	    (ret = __db_cursor(dbp, vdp->thread_info, NULL, &dbc, 0)) != 0)
		goto err;

	/* Salvage every page in pgset. */
	while ((ret = __db_vrfy_pgset_next(pgsc, &p)) == 0) {
		if (dbp->type == DB_QUEUE) {
#ifdef HAVE_QUEUE
			ret = __qam_fget(dbc, &p, 0, &subpg);
#else
			ret = __db_no_queue_am(env);
#endif
			/* Don't report an error for pages not found in a queue.
			 * The pgset is a best guess, it doesn't know about
			 * deleted extents which leads to this error.
			 */
			if (ret == ENOENT || ret == DB_PAGE_NOTFOUND)
				continue;
		} else
			ret = __memp_fget(mpf,
			    &p, vdp->thread_info, NULL, 0, &subpg);
		if (ret != 0) {
			err_ret = ret;
			continue;
		}

		if ((ret = __db_salvage_pg(dbp, vdp, p, subpg,
		    handle, callback, flags)) != 0)
			err_ret = ret;

		if (dbp->type == DB_QUEUE)
#ifdef HAVE_QUEUE
			ret = __qam_fput(dbc, p, subpg, dbp->priority);
#else
			ret = __db_no_queue_am(env);
#endif
		else
			ret = __memp_fput(mpf,
			    vdp->thread_info, subpg, dbp->priority);
		if (ret != 0)
			err_ret = ret;
	}

	if (ret == DB_NOTFOUND)
		ret = 0;

err:
	if (dbc != NULL && (t_ret = __dbc_close(dbc)) != 0)
		ret = t_ret;
	if (pgsc != NULL && (t_ret = __dbc_close(pgsc)) != 0)
		ret = t_ret;
	if (pgset != NULL && (t_ret = __db_close(pgset, NULL, 0)) != 0)
		ret = t_ret;

	return ((err_ret != 0) ? err_ret : ret);
}

/*
 * __db_meta2pgset --
 *	Given a known-safe meta page number, return the set of pages
 *	corresponding to the database it represents.  Return DB_VERIFY_BAD if
 *	it's not a suitable meta page or is invalid.
 */
static int
__db_meta2pgset(dbp, vdp, pgno, flags, pgset)
	DB *dbp;
	VRFY_DBINFO *vdp;
	db_pgno_t pgno;
	u_int32_t flags;
	DB *pgset;
{
	DB_MPOOLFILE *mpf;
	PAGE *h;
	int ret, t_ret;

	mpf = dbp->mpf;

	if ((ret = __memp_fget(mpf, &pgno, vdp->thread_info, NULL, 0, &h)) != 0)
		return (ret);

	switch (TYPE(h)) {
	case P_BTREEMETA:
		ret = __bam_meta2pgset(dbp, vdp, (BTMETA *)h, flags, pgset);
		break;
	case P_HASHMETA:
		ret = __ham_meta2pgset(dbp, vdp, (HMETA *)h, flags, pgset);
		break;
	case P_QAMMETA:
#ifdef HAVE_QUEUE
		ret = __qam_meta2pgset(dbp, vdp, pgset);
		break;
#endif
	default:
		ret = DB_VERIFY_BAD;
		break;
	}

	if ((t_ret = __memp_fput(mpf, vdp->thread_info, h, dbp->priority)) != 0)
		return (t_ret);
	return (ret);
}

/*
 * __db_guesspgsize --
 *	Try to guess what the pagesize is if the one on the meta page
 *	and the one in the db are invalid.
 */
static u_int
__db_guesspgsize(env, fhp)
	ENV *env;
	DB_FH *fhp;
{
	db_pgno_t i;
	size_t nr;
	u_int32_t guess;
	u_int8_t type;

	for (guess = DB_MAX_PGSIZE; guess >= DB_MIN_PGSIZE; guess >>= 1) {
		/*
		 * We try to read three pages ahead after the first one
		 * and make sure we have plausible types for all of them.
		 * If the seeks fail, continue with a smaller size;
		 * we're probably just looking past the end of the database.
		 * If they succeed and the types are reasonable, also continue
		 * with a size smaller;  we may be looking at pages N,
		 * 2N, and 3N for some N > 1.
		 *
		 * As soon as we hit an invalid type, we stop and return
		 * our previous guess; that last one was probably the page size.
		 */
		for (i = 1; i <= 3; i++) {
			if (__os_seek(
			    env, fhp, i, guess, SSZ(DBMETA, type)) != 0)
				break;
			if (__os_read(env,
			    fhp, &type, 1, &nr) != 0 || nr == 0)
				break;
			if (type == P_INVALID || type >= P_PAGETYPE_MAX)
				return (guess << 1);
		}
	}

	/*
	 * If we're just totally confused--the corruption takes up most of the
	 * beginning pages of the database--go with the default size.
	 */
	return (DB_DEF_IOSIZE);
}
