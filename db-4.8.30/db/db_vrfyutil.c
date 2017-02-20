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
#include "dbinc/db_verify.h"
#include "dbinc/db_am.h"

static int __db_vrfy_childinc __P((DBC *, VRFY_CHILDINFO *));
static int __db_vrfy_pageinfo_create __P((ENV *, VRFY_PAGEINFO **));

/*
 * __db_vrfy_dbinfo_create --
 *	Allocate and initialize a VRFY_DBINFO structure.
 *
 * PUBLIC: int __db_vrfy_dbinfo_create
 * PUBLIC:     __P((ENV *, DB_THREAD_INFO *, u_int32_t, VRFY_DBINFO **));
 */
int
__db_vrfy_dbinfo_create(env, ip, pgsize, vdpp)
	ENV *env;
	DB_THREAD_INFO *ip;
	u_int32_t pgsize;
	VRFY_DBINFO **vdpp;
{
	DB *cdbp, *pgdbp, *pgset;
	VRFY_DBINFO *vdp;
	int ret;

	vdp = NULL;
	cdbp = pgdbp = pgset = NULL;

	if ((ret = __os_calloc(NULL, 1, sizeof(VRFY_DBINFO), &vdp)) != 0)
		goto err;

	if ((ret = __db_create_internal(&cdbp, env, 0)) != 0)
		goto err;

	if ((ret = __db_set_flags(cdbp, DB_DUP)) != 0)
		goto err;

	if ((ret = __db_set_pagesize(cdbp, pgsize)) != 0)
		goto err;

	/* If transactional, make sure we don't log. */
	if (TXN_ON(env) &&
	    (ret = __db_set_flags(cdbp, DB_TXN_NOT_DURABLE)) != 0)
		goto err;
	if ((ret = __db_open(cdbp, ip,
	    NULL, NULL, NULL, DB_BTREE, DB_CREATE, 0600, PGNO_BASE_MD)) != 0)
		goto err;

	if ((ret = __db_create_internal(&pgdbp, env, 0)) != 0)
		goto err;

	if ((ret = __db_set_pagesize(pgdbp, pgsize)) != 0)
		goto err;

	/* If transactional, make sure we don't log. */
	if (TXN_ON(env) &&
	    (ret = __db_set_flags(pgdbp, DB_TXN_NOT_DURABLE)) != 0)
		goto err;

	if ((ret = __db_open(pgdbp, ip,
	    NULL, NULL, NULL, DB_BTREE, DB_CREATE, 0600, PGNO_BASE_MD)) != 0)
		goto err;

	if ((ret = __db_vrfy_pgset(env, ip, pgsize, &pgset)) != 0)
		goto err;

	LIST_INIT(&vdp->subdbs);
	LIST_INIT(&vdp->activepips);

	vdp->cdbp = cdbp;
	vdp->pgdbp = pgdbp;
	vdp->pgset = pgset;
	vdp->thread_info = ip;
	*vdpp = vdp;
	return (0);

err:	if (cdbp != NULL)
		(void)__db_close(cdbp, NULL, 0);
	if (pgdbp != NULL)
		(void)__db_close(pgdbp, NULL, 0);
	if (vdp != NULL)
		__os_free(env, vdp);
	return (ret);
}

/*
 * __db_vrfy_dbinfo_destroy --
 *	Destructor for VRFY_DBINFO.  Destroys VRFY_PAGEINFOs and deallocates
 *	structure.
 *
 * PUBLIC: int __db_vrfy_dbinfo_destroy __P((ENV *, VRFY_DBINFO *));
 */
int
__db_vrfy_dbinfo_destroy(env, vdp)
	ENV *env;
	VRFY_DBINFO *vdp;
{
	VRFY_CHILDINFO *c;
	int t_ret, ret;

	ret = 0;

	/*
	 * Discard active page structures.  Ideally there wouldn't be any,
	 * but in some error cases we may not have cleared them all out.
	 */
	while (LIST_FIRST(&vdp->activepips) != NULL)
		if ((t_ret = __db_vrfy_putpageinfo(
		    env, vdp, LIST_FIRST(&vdp->activepips))) != 0) {
			if (ret == 0)
				ret = t_ret;
			break;
		}

	/* Discard subdatabase list structures. */
	while ((c = LIST_FIRST(&vdp->subdbs)) != NULL) {
		LIST_REMOVE(c, links);
		__os_free(NULL, c);
	}

	if ((t_ret = __db_close(vdp->pgdbp, NULL, 0)) != 0)
		ret = t_ret;

	if ((t_ret = __db_close(vdp->cdbp, NULL, 0)) != 0 && ret == 0)
		ret = t_ret;

	if ((t_ret = __db_close(vdp->pgset, NULL, 0)) != 0 && ret == 0)
		ret = t_ret;

	if (vdp->extents != NULL)
		__os_free(env, vdp->extents);
	__os_free(env, vdp);
	return (ret);
}

/*
 * __db_vrfy_getpageinfo --
 *	Get a PAGEINFO structure for a given page, creating it if necessary.
 *
 * PUBLIC: int __db_vrfy_getpageinfo
 * PUBLIC:     __P((VRFY_DBINFO *, db_pgno_t, VRFY_PAGEINFO **));
 */
int
__db_vrfy_getpageinfo(vdp, pgno, pipp)
	VRFY_DBINFO *vdp;
	db_pgno_t pgno;
	VRFY_PAGEINFO **pipp;
{
	DB *pgdbp;
	DBT key, data;
	ENV *env;
	VRFY_PAGEINFO *pip;
	int ret;

	/*
	 * We want a page info struct.  There are three places to get it from,
	 * in decreasing order of preference:
	 *
	 * 1. vdp->activepips.  If it's already "checked out", we're
	 *	already using it, we return the same exact structure with a
	 *	bumped refcount.  This is necessary because this code is
	 *	replacing array accesses, and it's common for f() to make some
	 *	changes to a pip, and then call g() and h() which each make
	 *	changes to the same pip.  vdps are never shared between threads
	 *	(they're never returned to the application), so this is safe.
	 * 2. The pgdbp.  It's not in memory, but it's in the database, so
	 *	get it, give it a refcount of 1, and stick it on activepips.
	 * 3. malloc.  It doesn't exist yet;  create it, then stick it on
	 *	activepips.  We'll put it in the database when we putpageinfo
	 *	later.
	 */

	/* Case 1. */
	LIST_FOREACH(pip, &vdp->activepips, links)
		if (pip->pgno == pgno)
			goto found;

	/* Case 2. */
	pgdbp = vdp->pgdbp;
	env = pgdbp->env;
	memset(&key, 0, sizeof(DBT));
	memset(&data, 0, sizeof(DBT));
	F_SET(&data, DB_DBT_MALLOC);
	key.data = &pgno;
	key.size = sizeof(db_pgno_t);

	if ((ret = __db_get(pgdbp,
	    vdp->thread_info, NULL, &key, &data, 0)) == 0) {
		/* Found it. */
		DB_ASSERT(env, data.size == sizeof(VRFY_PAGEINFO));
		pip = data.data;
		LIST_INSERT_HEAD(&vdp->activepips, pip, links);
		goto found;
	} else if (ret != DB_NOTFOUND)	/* Something nasty happened. */
		return (ret);

	/* Case 3 */
	if ((ret = __db_vrfy_pageinfo_create(env, &pip)) != 0)
		return (ret);

	LIST_INSERT_HEAD(&vdp->activepips, pip, links);
found:	pip->pi_refcount++;

	*pipp = pip;
	return (0);
}

/*
 * __db_vrfy_putpageinfo --
 *	Put back a VRFY_PAGEINFO that we're done with.
 *
 * PUBLIC: int __db_vrfy_putpageinfo __P((ENV *,
 * PUBLIC:     VRFY_DBINFO *, VRFY_PAGEINFO *));
 */
int
__db_vrfy_putpageinfo(env, vdp, pip)
	ENV *env;
	VRFY_DBINFO *vdp;
	VRFY_PAGEINFO *pip;
{
	DB *pgdbp;
	DBT key, data;
	VRFY_PAGEINFO *p;
	int ret;

	if (--pip->pi_refcount > 0)
		return (0);

	pgdbp = vdp->pgdbp;
	memset(&key, 0, sizeof(DBT));
	memset(&data, 0, sizeof(DBT));

	key.data = &pip->pgno;
	key.size = sizeof(db_pgno_t);
	data.data = pip;
	data.size = sizeof(VRFY_PAGEINFO);

	if ((ret = __db_put(pgdbp,
	     vdp->thread_info, NULL, &key, &data, 0)) != 0)
		return (ret);

	LIST_FOREACH(p, &vdp->activepips, links)
		if (p == pip)
			break;
	if (p != NULL)
		LIST_REMOVE(p, links);

	__os_ufree(env, p);
	return (0);
}

/*
 * __db_vrfy_pgset --
 *	Create a temporary database for the storing of sets of page numbers.
 *	(A mapping from page number to int, used by the *_meta2pgset functions,
 *	as well as for keeping track of which pages the verifier has seen.)
 *
 * PUBLIC: int __db_vrfy_pgset __P((ENV *,
 * PUBLIC:     DB_THREAD_INFO *, u_int32_t, DB **));
 */
int
__db_vrfy_pgset(env, ip, pgsize, dbpp)
	ENV *env;
	DB_THREAD_INFO *ip;
	u_int32_t pgsize;
	DB **dbpp;
{
	DB *dbp;
	int ret;

	if ((ret = __db_create_internal(&dbp, env, 0)) != 0)
		return (ret);
	if ((ret = __db_set_pagesize(dbp, pgsize)) != 0)
		goto err;

	/* If transactional, make sure we don't log. */
	if (TXN_ON(env) &&
	    (ret = __db_set_flags(dbp, DB_TXN_NOT_DURABLE)) != 0)
		goto err;
	if ((ret = __db_open(dbp, ip,
	    NULL, NULL, NULL, DB_BTREE, DB_CREATE, 0600, PGNO_BASE_MD)) == 0)
		*dbpp = dbp;
	else
err:		(void)__db_close(dbp, NULL, 0);

	return (ret);
}

/*
 * __db_vrfy_pgset_get --
 *	Get the value associated in a page set with a given pgno.  Return
 *	a 0 value (and succeed) if we've never heard of this page.
 *
 * PUBLIC: int __db_vrfy_pgset_get __P((DB *, DB_THREAD_INFO *, db_pgno_t,
 * PUBLIC:     int *));
 */
int
__db_vrfy_pgset_get(dbp, ip, pgno, valp)
	DB *dbp;
	DB_THREAD_INFO *ip;
	db_pgno_t pgno;
	int *valp;
{
	DBT key, data;
	int ret, val;

	memset(&key, 0, sizeof(DBT));
	memset(&data, 0, sizeof(DBT));

	key.data = &pgno;
	key.size = sizeof(db_pgno_t);
	data.data = &val;
	data.ulen = sizeof(int);
	F_SET(&data, DB_DBT_USERMEM);

	if ((ret = __db_get(dbp, ip, NULL, &key, &data, 0)) == 0) {
		DB_ASSERT(dbp->env, data.size == sizeof(int));
	} else if (ret == DB_NOTFOUND)
		val = 0;
	else
		return (ret);

	*valp = val;
	return (0);
}

/*
 * __db_vrfy_pgset_inc --
 *	Increment the value associated with a pgno by 1.
 *
 * PUBLIC: int __db_vrfy_pgset_inc __P((DB *, DB_THREAD_INFO *, db_pgno_t));
 */
int
__db_vrfy_pgset_inc(dbp, ip, pgno)
	DB *dbp;
	DB_THREAD_INFO *ip;
	db_pgno_t pgno;
{
	DBT key, data;
	int ret;
	int val;

	memset(&key, 0, sizeof(DBT));
	memset(&data, 0, sizeof(DBT));

	val = 0;

	key.data = &pgno;
	key.size = sizeof(db_pgno_t);
	data.data = &val;
	data.ulen = sizeof(int);
	F_SET(&data, DB_DBT_USERMEM);

	if ((ret = __db_get(dbp, ip, NULL, &key, &data, 0)) == 0) {
		DB_ASSERT(dbp->env, data.size == sizeof(int));
	} else if (ret != DB_NOTFOUND)
		return (ret);

	data.size = sizeof(int);
	++val;

	return (__db_put(dbp, ip, NULL, &key, &data, 0));
}

/*
 * __db_vrfy_pgset_next --
 *	Given a cursor open in a pgset database, get the next page in the
 *	set.
 *
 * PUBLIC: int __db_vrfy_pgset_next __P((DBC *, db_pgno_t *));
 */
int
__db_vrfy_pgset_next(dbc, pgnop)
	DBC *dbc;
	db_pgno_t *pgnop;
{
	DBT key, data;
	db_pgno_t pgno;
	int ret;

	memset(&key, 0, sizeof(DBT));
	memset(&data, 0, sizeof(DBT));
	/* We don't care about the data, just the keys. */
	F_SET(&data, DB_DBT_USERMEM | DB_DBT_PARTIAL);
	F_SET(&key, DB_DBT_USERMEM);
	key.data = &pgno;
	key.ulen = sizeof(db_pgno_t);

	if ((ret = __dbc_get(dbc, &key, &data, DB_NEXT)) != 0)
		return (ret);

	DB_ASSERT(dbc->env, key.size == sizeof(db_pgno_t));
	*pgnop = pgno;

	return (0);
}

/*
 * __db_vrfy_childcursor --
 *	Create a cursor to walk the child list with.  Returns with a nonzero
 *	final argument if the specified page has no children.
 *
 * PUBLIC: int __db_vrfy_childcursor __P((VRFY_DBINFO *, DBC **));
 */
int
__db_vrfy_childcursor(vdp, dbcp)
	VRFY_DBINFO *vdp;
	DBC **dbcp;
{
	DB *cdbp;
	DBC *dbc;
	int ret;

	cdbp = vdp->cdbp;

	if ((ret = __db_cursor(cdbp, vdp->thread_info, NULL, &dbc, 0)) == 0)
		*dbcp = dbc;

	return (ret);
}

/*
 * __db_vrfy_childput --
 *	Add a child structure to the set for a given page.
 *
 * PUBLIC: int __db_vrfy_childput
 * PUBLIC:     __P((VRFY_DBINFO *, db_pgno_t, VRFY_CHILDINFO *));
 */
int
__db_vrfy_childput(vdp, pgno, cip)
	VRFY_DBINFO *vdp;
	db_pgno_t pgno;
	VRFY_CHILDINFO *cip;
{
	DB *cdbp;
	DBC *cc;
	DBT key, data;
	VRFY_CHILDINFO *oldcip;
	int ret;

	cdbp = vdp->cdbp;
	memset(&key, 0, sizeof(DBT));
	memset(&data, 0, sizeof(DBT));

	key.data = &pgno;
	key.size = sizeof(db_pgno_t);

	/*
	 * We want to avoid adding multiple entries for a single child page;
	 * we only need to verify each child once, even if a child (such
	 * as an overflow key) is multiply referenced.
	 *
	 * However, we also need to make sure that when walking the list
	 * of children, we encounter them in the order they're referenced
	 * on a page.  (This permits us, for example, to verify the
	 * prev_pgno/next_pgno chain of Btree leaf pages.)
	 *
	 * Check the child database to make sure that this page isn't
	 * already a child of the specified page number.  If it's not,
	 * put it at the end of the duplicate set.
	 */
	if ((ret = __db_vrfy_childcursor(vdp, &cc)) != 0)
		return (ret);
	for (ret = __db_vrfy_ccset(cc, pgno, &oldcip); ret == 0;
	    ret = __db_vrfy_ccnext(cc, &oldcip))
		if (oldcip->pgno == cip->pgno) {
			/*
			 * Found a matching child.  Increment its reference
			 * count--we've run into it again--but don't put it
			 * again.
			 */
			if ((ret = __db_vrfy_childinc(cc, oldcip)) != 0 ||
			    (ret = __db_vrfy_ccclose(cc)) != 0)
				return (ret);
			return (0);
		}
	if (ret != DB_NOTFOUND) {
		(void)__db_vrfy_ccclose(cc);
		return (ret);
	}
	if ((ret = __db_vrfy_ccclose(cc)) != 0)
		return (ret);

	cip->refcnt = 1;
	data.data = cip;
	data.size = sizeof(VRFY_CHILDINFO);

	return (__db_put(cdbp, vdp->thread_info, NULL, &key, &data, 0));
}

/*
 * __db_vrfy_childinc --
 *	Increment the refcount of the VRFY_CHILDINFO struct that the child
 * cursor is pointing to.  (The caller has just retrieved this struct, and
 * passes it in as cip to save us a get.)
 */
static int
__db_vrfy_childinc(dbc, cip)
	DBC *dbc;
	VRFY_CHILDINFO *cip;
{
	DBT key, data;

	memset(&key, 0, sizeof(DBT));
	memset(&data, 0, sizeof(DBT));

	cip->refcnt++;
	data.data = cip;
	data.size = sizeof(VRFY_CHILDINFO);

	return (__dbc_put(dbc, &key, &data, DB_CURRENT));
}

/*
 * __db_vrfy_ccset --
 *	Sets a cursor created with __db_vrfy_childcursor to the first
 *	child of the given pgno, and returns it in the third arg.
 *
 * PUBLIC: int __db_vrfy_ccset __P((DBC *, db_pgno_t, VRFY_CHILDINFO **));
 */
int
__db_vrfy_ccset(dbc, pgno, cipp)
	DBC *dbc;
	db_pgno_t pgno;
	VRFY_CHILDINFO **cipp;
{
	DBT key, data;
	int ret;

	memset(&key, 0, sizeof(DBT));
	memset(&data, 0, sizeof(DBT));

	key.data = &pgno;
	key.size = sizeof(db_pgno_t);

	if ((ret = __dbc_get(dbc, &key, &data, DB_SET)) != 0)
		return (ret);

	DB_ASSERT(dbc->env, data.size == sizeof(VRFY_CHILDINFO));
	*cipp = (VRFY_CHILDINFO *)data.data;

	return (0);
}

/*
 * __db_vrfy_ccnext --
 *	Gets the next child of the given cursor created with
 *	__db_vrfy_childcursor, and returns it in the memory provided in the
 *	second arg.
 *
 * PUBLIC: int __db_vrfy_ccnext __P((DBC *, VRFY_CHILDINFO **));
 */
int
__db_vrfy_ccnext(dbc, cipp)
	DBC *dbc;
	VRFY_CHILDINFO **cipp;
{
	DBT key, data;
	int ret;

	memset(&key, 0, sizeof(DBT));
	memset(&data, 0, sizeof(DBT));

	if ((ret = __dbc_get(dbc, &key, &data, DB_NEXT_DUP)) != 0)
		return (ret);

	DB_ASSERT(dbc->env, data.size == sizeof(VRFY_CHILDINFO));
	*cipp = (VRFY_CHILDINFO *)data.data;

	return (0);
}

/*
 * __db_vrfy_ccclose --
 *	Closes the cursor created with __db_vrfy_childcursor.
 *
 *	This doesn't actually do anything interesting now, but it's
 *	not inconceivable that we might change the internal database usage
 *	and keep the interfaces the same, and a function call here or there
 *	seldom hurts anyone.
 *
 * PUBLIC: int __db_vrfy_ccclose __P((DBC *));
 */
int
__db_vrfy_ccclose(dbc)
	DBC *dbc;
{

	return (__dbc_close(dbc));
}

/*
 * __db_vrfy_pageinfo_create --
 *	Constructor for VRFY_PAGEINFO;  allocates and initializes.
 */
static int
__db_vrfy_pageinfo_create(env, pipp)
	ENV *env;
	VRFY_PAGEINFO **pipp;
{
	VRFY_PAGEINFO *pip;
	int ret;

	/*
	 * pageinfo structs are sometimes allocated here and sometimes
	 * allocated by fetching them from a database with DB_DBT_MALLOC.
	 * There's no easy way for the destructor to tell which was
	 * used, and so we always allocate with __os_umalloc so we can free
	 * with __os_ufree.
	 */
	if ((ret = __os_umalloc(env, sizeof(VRFY_PAGEINFO), &pip)) != 0)
		return (ret);
	memset(pip, 0, sizeof(VRFY_PAGEINFO));

	*pipp = pip;
	return (0);
}

/*
 * __db_salvage_init --
 *	Set up salvager database.
 *
 * PUBLIC: int  __db_salvage_init __P((VRFY_DBINFO *));
 */
int
__db_salvage_init(vdp)
	VRFY_DBINFO *vdp;
{
	DB *dbp;
	int ret;

	if ((ret = __db_create_internal(&dbp, NULL, 0)) != 0)
		return (ret);

	if ((ret = __db_set_pagesize(dbp, 1024)) != 0)
		goto err;

	if ((ret = __db_open(dbp, vdp->thread_info,
	    NULL, NULL, NULL, DB_BTREE, DB_CREATE, 0, PGNO_BASE_MD)) != 0)
		goto err;

	vdp->salvage_pages = dbp;
	return (0);

err:	(void)__db_close(dbp, NULL, 0);
	return (ret);
}

/*
 * __db_salvage_destroy --
 *	Close salvager database.
 * PUBLIC: int  __db_salvage_destroy __P((VRFY_DBINFO *));
 */
int
__db_salvage_destroy(vdp)
	VRFY_DBINFO *vdp;
{
	return (vdp->salvage_pages == NULL ? 0 :
	    __db_close(vdp->salvage_pages, NULL, 0));
}

/*
 * __db_salvage_getnext --
 *	Get the next (first) unprinted page in the database of pages we need to
 *	print still.  Delete entries for any already-printed pages we encounter
 *	in this search, as well as the page we're returning.
 *
 * PUBLIC: int __db_salvage_getnext
 * PUBLIC:     __P((VRFY_DBINFO *, DBC **, db_pgno_t *, u_int32_t *, int));
 */
int
__db_salvage_getnext(vdp, dbcp, pgnop, pgtypep, skip_overflow)
	VRFY_DBINFO *vdp;
	DBC **dbcp;
	db_pgno_t *pgnop;
	u_int32_t *pgtypep;
	int skip_overflow;
{
	DB *dbp;
	DBT key, data;
	int ret;
	u_int32_t pgtype;

	dbp = vdp->salvage_pages;

	memset(&key, 0, sizeof(DBT));
	memset(&data, 0, sizeof(DBT));

	if (*dbcp == NULL &&
	    (ret = __db_cursor(dbp, vdp->thread_info, NULL, dbcp, 0)) != 0)
		return (ret);

	while ((ret = __dbc_get(*dbcp, &key, &data, DB_NEXT)) == 0) {
		DB_ASSERT(dbp->env, data.size == sizeof(u_int32_t));
		memcpy(&pgtype, data.data, sizeof(pgtype));

		if (skip_overflow && pgtype == SALVAGE_OVERFLOW)
			continue;

		if ((ret = __dbc_del(*dbcp, 0)) != 0)
			return (ret);
		if (pgtype != SALVAGE_IGNORE) {
			DB_ASSERT(dbp->env, key.size == sizeof(db_pgno_t));
			DB_ASSERT(dbp->env, data.size == sizeof(u_int32_t));

			*pgnop = *(db_pgno_t *)key.data;
			*pgtypep = *(u_int32_t *)data.data;
			break;
		}
	}

	return (ret);
}

/*
 * __db_salvage_isdone --
 *	Return whether or not the given pgno is already marked
 *	SALVAGE_IGNORE (meaning that we don't need to print it again).
 *
 *	Returns DB_KEYEXIST if it is marked, 0 if not, or another error on
 *	error.
 *
 * PUBLIC: int __db_salvage_isdone __P((VRFY_DBINFO *, db_pgno_t));
 */
int
__db_salvage_isdone(vdp, pgno)
	VRFY_DBINFO *vdp;
	db_pgno_t pgno;
{
	DB *dbp;
	DBT key, data;
	int ret;
	u_int32_t currtype;

	dbp = vdp->salvage_pages;

	memset(&key, 0, sizeof(DBT));
	memset(&data, 0, sizeof(DBT));

	currtype = SALVAGE_INVALID;
	data.data = &currtype;
	data.ulen = sizeof(u_int32_t);
	data.flags = DB_DBT_USERMEM;

	key.data = &pgno;
	key.size = sizeof(db_pgno_t);

	/*
	 * Put an entry for this page, with pgno as key and type as data,
	 * unless it's already there and is marked done.
	 * If it's there and is marked anything else, that's fine--we
	 * want to mark it done.
	 */
	if ((ret = __db_get(dbp,
	    vdp->thread_info, NULL, &key, &data, 0)) == 0) {
		/*
		 * The key's already here.  Check and see if it's already
		 * marked done.  If it is, return DB_KEYEXIST.  If it's not,
		 * return 0.
		 */
		if (currtype == SALVAGE_IGNORE)
			return (DB_KEYEXIST);
		else
			return (0);
	} else if (ret != DB_NOTFOUND)
		return (ret);

	/* The pgno is not yet marked anything; return 0. */
	return (0);
}

/*
 * __db_salvage_markdone --
 *	Mark as done a given page.
 *
 * PUBLIC: int __db_salvage_markdone __P((VRFY_DBINFO *, db_pgno_t));
 */
int
__db_salvage_markdone(vdp, pgno)
	VRFY_DBINFO *vdp;
	db_pgno_t pgno;
{
	DB *dbp;
	DBT key, data;
	int pgtype, ret;
	u_int32_t currtype;

	pgtype = SALVAGE_IGNORE;
	dbp = vdp->salvage_pages;

	memset(&key, 0, sizeof(DBT));
	memset(&data, 0, sizeof(DBT));

	currtype = SALVAGE_INVALID;
	data.data = &currtype;
	data.ulen = sizeof(u_int32_t);
	data.flags = DB_DBT_USERMEM;

	key.data = &pgno;
	key.size = sizeof(db_pgno_t);

	/*
	 * Put an entry for this page, with pgno as key and type as data,
	 * unless it's already there and is marked done.
	 * If it's there and is marked anything else, that's fine--we
	 * want to mark it done, but db_salvage_isdone only lets
	 * us know if it's marked IGNORE.
	 *
	 * We don't want to return DB_KEYEXIST, though;  this will
	 * likely get passed up all the way and make no sense to the
	 * application.  Instead, use DB_VERIFY_BAD to indicate that
	 * we've seen this page already--it probably indicates a
	 * multiply-linked page.
	 */
	if ((ret = __db_salvage_isdone(vdp, pgno)) != 0)
		return (ret == DB_KEYEXIST ? DB_VERIFY_BAD : ret);

	data.size = sizeof(u_int32_t);
	data.data = &pgtype;

	return (__db_put(dbp, vdp->thread_info, NULL, &key, &data, 0));
}

/*
 * __db_salvage_markneeded --
 *	If it has not yet been printed, make note of the fact that a page
 *	must be dealt with later.
 *
 * PUBLIC: int __db_salvage_markneeded
 * PUBLIC:     __P((VRFY_DBINFO *, db_pgno_t, u_int32_t));
 */
int
__db_salvage_markneeded(vdp, pgno, pgtype)
	VRFY_DBINFO *vdp;
	db_pgno_t pgno;
	u_int32_t pgtype;
{
	DB *dbp;
	DBT key, data;
	int ret;

	dbp = vdp->salvage_pages;

	memset(&key, 0, sizeof(DBT));
	memset(&data, 0, sizeof(DBT));

	key.data = &pgno;
	key.size = sizeof(db_pgno_t);

	data.data = &pgtype;
	data.size = sizeof(u_int32_t);

	/*
	 * Put an entry for this page, with pgno as key and type as data,
	 * unless it's already there, in which case it's presumably
	 * already been marked done.
	 */
	ret = __db_put(dbp,
	     vdp->thread_info, NULL, &key, &data, DB_NOOVERWRITE);
	return (ret == DB_KEYEXIST ? 0 : ret);
}

/*
 * __db_vrfy_prdbt --
 *	Print out a DBT data element from a verification routine.
 *
 * PUBLIC: int __db_vrfy_prdbt __P((DBT *, int, const char *, void *,
 * PUBLIC:     int (*)(void *, const void *), int, VRFY_DBINFO *));
 */
int
__db_vrfy_prdbt(dbtp, checkprint, prefix, handle, callback, is_recno, vdp)
	DBT *dbtp;
	int checkprint;
	const char *prefix;
	void *handle;
	int (*callback) __P((void *, const void *));
	int is_recno;
	VRFY_DBINFO *vdp;
{
	if (vdp != NULL) {
		/*
		 * If vdp is non-NULL, we might be the first key in the
		 * "fake" subdatabase used for key/data pairs we can't
		 * associate with a known subdb.
		 *
		 * Check and clear the SALVAGE_PRINTHEADER flag;  if
		 * it was set, print a subdatabase header.
		 */
		if (F_ISSET(vdp, SALVAGE_PRINTHEADER)) {
			(void)__db_prheader(
			    NULL, "__OTHER__", 0, 0, handle, callback, vdp, 0);
			F_CLR(vdp, SALVAGE_PRINTHEADER);
			F_SET(vdp, SALVAGE_PRINTFOOTER);
		}

		/*
		 * Even if the printable flag wasn't set by our immediate
		 * caller, it may be set on a salvage-wide basis.
		 */
		if (F_ISSET(vdp, SALVAGE_PRINTABLE))
			checkprint = 1;
	}
	return (
	    __db_prdbt(dbtp, checkprint, prefix, handle, callback, is_recno));
}
