/*-
 * See the file LICENSE for redistribution information.
 *
 * Copyright (c) 1996-2009 Oracle.  All rights reserved.
 *
 * $Id$
 */

#ifndef HAVE_HASH
#include "db_config.h"

#include "db_int.h"
#include "dbinc/db_page.h"
#include "dbinc/hash.h"

/*
 * If the library wasn't compiled with the Hash access method, various
 * routines aren't available.  Stub them here, returning an appropriate
 * error.
 */

/*
 * __db_nohasham --
 *	Error when a Berkeley DB build doesn't include the access method.
 *
 * PUBLIC: int __db_no_hash_am __P((ENV *));
 */
int
__db_no_hash_am(env)
	ENV *env;
{
	__db_errx(env,
	    "library build did not include support for the Hash access method");
	return (DB_OPNOTSUP);
}

int
__ham_30_hashmeta(dbp, real_name, obuf)
	DB *dbp;
	char *real_name;
	u_int8_t *obuf;
{
	COMPQUIET(real_name, NULL);
	COMPQUIET(obuf, NULL);
	return (__db_no_hash_am(dbp->env));
}

int
__ham_30_sizefix(dbp, fhp, realname, metabuf)
	DB *dbp;
	DB_FH *fhp;
	char *realname;
	u_int8_t *metabuf;
{
	COMPQUIET(fhp, NULL);
	COMPQUIET(realname, NULL);
	COMPQUIET(metabuf, NULL);
	return (__db_no_hash_am(dbp->env));
}

int
__ham_31_hash(dbp, real_name, flags, fhp, h, dirtyp)
	DB *dbp;
	char *real_name;
	u_int32_t flags;
	DB_FH *fhp;
	PAGE *h;
	int *dirtyp;
{
	COMPQUIET(real_name, NULL);
	COMPQUIET(flags, 0);
	COMPQUIET(fhp, NULL);
	COMPQUIET(h, NULL);
	COMPQUIET(dirtyp, NULL);
	return (__db_no_hash_am(dbp->env));
}

int
__ham_31_hashmeta(dbp, real_name, flags, fhp, h, dirtyp)
	DB *dbp;
	char *real_name;
	u_int32_t flags;
	DB_FH *fhp;
	PAGE *h;
	int *dirtyp;
{
	COMPQUIET(real_name, NULL);
	COMPQUIET(flags, 0);
	COMPQUIET(fhp, NULL);
	COMPQUIET(h, NULL);
	COMPQUIET(dirtyp, NULL);
	return (__db_no_hash_am(dbp->env));
}

int
__ham_46_hash(dbp, real_name, flags, fhp, h, dirtyp)
	DB *dbp;
	char *real_name;
	u_int32_t flags;
	DB_FH *fhp;
	PAGE *h;
	int *dirtyp;
{
	COMPQUIET(real_name, NULL);
	COMPQUIET(flags, 0);
	COMPQUIET(fhp, NULL);
	COMPQUIET(h, NULL);
	COMPQUIET(dirtyp, NULL);
	return (__db_no_hash_am(dbp->env));
}

int
__ham_46_hashmeta(dbp, real_name, flags, fhp, h, dirtyp)
	DB *dbp;
	char *real_name;
	u_int32_t flags;
	DB_FH *fhp;
	PAGE *h;
	int *dirtyp;
{
	COMPQUIET(real_name, NULL);
	COMPQUIET(flags, 0);
	COMPQUIET(fhp, NULL);
	COMPQUIET(h, NULL);
	COMPQUIET(dirtyp, NULL);
	return (__db_no_hash_am(dbp->env));
}

int
__hamc_cmp(dbc, other_dbc, result)
	DBC *dbc, *other_dbc;
	int *result;
{
	COMPQUIET(other_dbc, NULL);
	COMPQUIET(result, NULL);
	return (__db_no_hash_am(dbc->env));
}

int
__hamc_count(dbc, recnop)
	DBC *dbc;
	db_recno_t *recnop;
{
	COMPQUIET(recnop, NULL);
	return (__db_no_hash_am(dbc->env));
}

int
__hamc_dup(orig_dbc, new_dbc)
	DBC *orig_dbc, *new_dbc;
{
	COMPQUIET(new_dbc, NULL);
	return (__db_no_hash_am(orig_dbc->env));
}

int
__hamc_init(dbc)
	DBC *dbc;
{
	return (__db_no_hash_am(dbc->env));
}

int
__ham_db_close(dbp)
	DB *dbp;
{
	COMPQUIET(dbp, NULL);
	return (0);
}

int
__ham_db_create(dbp)
	DB *dbp;
{
	COMPQUIET(dbp, NULL);
	return (0);
}

int
__ham_init_print(env, dtabp)
	ENV *env;
	DB_DISTAB *dtabp;
{
	COMPQUIET(env, NULL);
	COMPQUIET(dtabp, NULL);
	return (0);
}

int
__ham_init_recover(env, dtabp)
	ENV *env;
	DB_DISTAB *dtabp;
{
	COMPQUIET(env, NULL);
	COMPQUIET(dtabp, NULL);
	return (0);
}

int
__ham_meta2pgset(dbp, vdp, hmeta, flags, pgset)
	DB *dbp;
	VRFY_DBINFO *vdp;
	HMETA *hmeta;
	u_int32_t flags;
	DB *pgset;
{
	COMPQUIET(vdp, NULL);
	COMPQUIET(hmeta, NULL);
	COMPQUIET(flags, 0);
	COMPQUIET(pgset, NULL);
	return (__db_no_hash_am(dbp->env));
}

int
__ham_metachk(dbp, name, hashm)
	DB *dbp;
	const char *name;
	HMETA *hashm;
{
	COMPQUIET(name, NULL);
	COMPQUIET(hashm, NULL);
	return (__db_no_hash_am(dbp->env));
}

int
__ham_metagroup_42_recover(env, dbtp, lsnp, op, info)
	ENV *env;
	DBT *dbtp;
	DB_LSN *lsnp;
	db_recops op;
	void *info;
{
	COMPQUIET(dbtp, NULL);
	COMPQUIET(lsnp, NULL);
	COMPQUIET(op, (db_recops)0);
	COMPQUIET(info, NULL);
	return (__db_no_hash_am(env));
}

int
__ham_mswap(env, pg)
	ENV *env;
	void *pg;
{
	COMPQUIET(pg, NULL);
	return (__db_no_hash_am(env));
}

int
__ham_groupalloc_42_recover(env, dbtp, lsnp, op, info)
	ENV *env;
	DBT *dbtp;
	DB_LSN *lsnp;
	db_recops op;
	void *info;
{
	COMPQUIET(dbtp, NULL);
	COMPQUIET(lsnp, NULL);
	COMPQUIET(op, (db_recops)0);
	COMPQUIET(info, NULL);
	return (__db_no_hash_am(env));
}

int
__ham_new_file(dbp, ip, txn, fhp, name)
	DB *dbp;
	DB_THREAD_INFO *ip;
	DB_TXN *txn;
	DB_FH *fhp;
	const char *name;
{
	COMPQUIET(ip, NULL);
	COMPQUIET(txn, NULL);
	COMPQUIET(fhp, NULL);
	COMPQUIET(name, NULL);
	return (__db_no_hash_am(dbp->env));
}

int
__ham_new_subdb(mdbp, dbp, ip, txn)
	DB *mdbp, *dbp;
	DB_THREAD_INFO *ip;
	DB_TXN *txn;
{
	COMPQUIET(dbp, NULL);
	COMPQUIET(txn, NULL);
	COMPQUIET(ip, NULL);
	return (__db_no_hash_am(mdbp->env));
}

int
__ham_open(dbp, ip, txn, name, base_pgno, flags)
	DB *dbp;
	DB_THREAD_INFO *ip;
	DB_TXN *txn;
	const char *name;
	db_pgno_t base_pgno;
	u_int32_t flags;
{
	COMPQUIET(ip, NULL);
	COMPQUIET(txn, NULL);
	COMPQUIET(name, NULL);
	COMPQUIET(base_pgno, 0);
	COMPQUIET(flags, 0);
	return (__db_no_hash_am(dbp->env));
}

int
__ham_pgin(dbp, pg, pp, cookie)
	DB *dbp;
	db_pgno_t pg;
	void *pp;
	DBT *cookie;
{
	COMPQUIET(pg, 0);
	COMPQUIET(pp, NULL);
	COMPQUIET(cookie, NULL);
	return (__db_no_hash_am(dbp->env));
}

int
__ham_pgout(dbp, pg, pp, cookie)
	DB *dbp;
	db_pgno_t pg;
	void *pp;
	DBT *cookie;
{
	COMPQUIET(pg, 0);
	COMPQUIET(pp, NULL);
	COMPQUIET(cookie, NULL);
	return (__db_no_hash_am(dbp->env));
}

void
__ham_print_cursor(dbc)
	DBC *dbc;
{
	(void)__db_no_hash_am(dbc->env);
}

int
__ham_quick_delete(dbc)
	DBC *dbc;
{
	return (__db_no_hash_am(dbc->env));
}

int
__ham_reclaim(dbp, ip, txn)
	DB *dbp;
	DB_THREAD_INFO *ip;
	DB_TXN *txn;
{
	COMPQUIET(txn, NULL);
	COMPQUIET(ip, NULL);
	return (__db_no_hash_am(dbp->env));
}

int
__ham_salvage(dbp, vdp, pgno, h, handle, callback, flags)
	DB *dbp;
	VRFY_DBINFO *vdp;
	db_pgno_t pgno;
	PAGE *h;
	void *handle;
	int (*callback) __P((void *, const void *));
	u_int32_t flags;
{
	COMPQUIET(vdp, NULL);
	COMPQUIET(pgno, 0);
	COMPQUIET(h, NULL);
	COMPQUIET(handle, NULL);
	COMPQUIET(callback, NULL);
	COMPQUIET(flags, 0);
	return (__db_no_hash_am(dbp->env));
}

int
__ham_stat(dbc, spp, flags)
	DBC *dbc;
	void *spp;
	u_int32_t flags;
{
	COMPQUIET(spp, NULL);
	COMPQUIET(flags, 0);
	return (__db_no_hash_am(dbc->env));
}

int
__ham_stat_print(dbc, flags)
	DBC *dbc;
	u_int32_t flags;
{
	COMPQUIET(flags, 0);
	return (__db_no_hash_am(dbc->env));
}

int
__ham_truncate(dbc, countp)
	DBC *dbc;
	u_int32_t *countp;
{
	COMPQUIET(dbc, NULL);
	COMPQUIET(countp, NULL);
	return (__db_no_hash_am(dbc->env));
}

int
__ham_vrfy(dbp, vdp, h, pgno, flags)
	DB *dbp;
	VRFY_DBINFO *vdp;
	PAGE *h;
	db_pgno_t pgno;
	u_int32_t flags;
{
	COMPQUIET(vdp, NULL);
	COMPQUIET(h, NULL);
	COMPQUIET(pgno, 0);
	COMPQUIET(flags, 0);
	return (__db_no_hash_am(dbp->env));
}

int
__ham_vrfy_hashing(dbc, nentries, m, thisbucket, pgno, flags, hfunc)
	DBC *dbc;
	u_int32_t nentries;
	HMETA *m;
	u_int32_t thisbucket;
	db_pgno_t pgno;
	u_int32_t flags;
	u_int32_t (*hfunc) __P((DB *, const void *, u_int32_t));
{
	COMPQUIET(nentries, 0);
	COMPQUIET(m, NULL);
	COMPQUIET(thisbucket, 0);
	COMPQUIET(pgno, 0);
	COMPQUIET(flags, 0);
	COMPQUIET(hfunc, NULL);
	return (__db_no_hash_am(dbc->dbp->env));
}

int
__ham_vrfy_meta(dbp, vdp, m, pgno, flags)
	DB *dbp;
	VRFY_DBINFO *vdp;
	HMETA *m;
	db_pgno_t pgno;
	u_int32_t flags;
{
	COMPQUIET(vdp, NULL);
	COMPQUIET(m, NULL);
	COMPQUIET(pgno, 0);
	COMPQUIET(flags, 0);
	return (__db_no_hash_am(dbp->env));
}

int
__ham_vrfy_structure(dbp, vdp, meta_pgno, flags)
	DB *dbp;
	VRFY_DBINFO *vdp;
	db_pgno_t meta_pgno;
	u_int32_t flags;
{
	COMPQUIET(vdp, NULL);
	COMPQUIET(meta_pgno, 0);
	COMPQUIET(flags, 0);
	return (__db_no_hash_am(dbp->env));
}
#endif /* !HAVE_HASH */
