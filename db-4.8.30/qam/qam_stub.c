/*-
 * See the file LICENSE for redistribution information.
 *
 * Copyright (c) 1996-2009 Oracle.  All rights reserved.
 *
 * $Id$
 */

#ifndef	HAVE_QUEUE
#include "db_config.h"

#include "db_int.h"
#include "dbinc/db_page.h"
#include "dbinc/qam.h"

/*
 * If the library wasn't compiled with the Queue access method, various
 * routines aren't available.  Stub them here, returning an appropriate
 * error.
 */

/*
 * __db_no_queue_am --
 *	Error when a Berkeley DB build doesn't include the access method.
 *
 * PUBLIC: int __db_no_queue_am __P((ENV *));
 */
int
__db_no_queue_am(env)
	ENV *env;
{
	__db_errx(env,
    "library build did not include support for the Queue access method");
	return (DB_OPNOTSUP);
}

int
__db_prqueue(dbp, flags)
	DB *dbp;
	u_int32_t flags;
{
	COMPQUIET(flags, 0);
	return (__db_no_queue_am(dbp->env));
}

int
__qam_31_qammeta(dbp, real_name, buf)
	DB *dbp;
	char *real_name;
	u_int8_t *buf;
{
	COMPQUIET(real_name, NULL);
	COMPQUIET(buf, NULL);
	return (__db_no_queue_am(dbp->env));
}

int
__qam_32_qammeta(dbp, real_name, buf)
	DB *dbp;
	char *real_name;
	u_int8_t *buf;
{
	COMPQUIET(real_name, NULL);
	COMPQUIET(buf, NULL);
	return (__db_no_queue_am(dbp->env));
}

int
__qam_append(dbc, key, data)
	DBC *dbc;
	DBT *key, *data;
{
	COMPQUIET(key, NULL);
	COMPQUIET(data, NULL);
	return (__db_no_queue_am(dbc->env));
}

int
__qamc_dup(orig_dbc, new_dbc)
	DBC *orig_dbc, *new_dbc;
{
	COMPQUIET(new_dbc, NULL);
	return (__db_no_queue_am(orig_dbc->env));
}

int
__qamc_init(dbc)
	DBC *dbc;
{
	return (__db_no_queue_am(dbc->env));
}

int
__qam_db_close(dbp, flags)
	DB *dbp;
	u_int32_t flags;
{
	COMPQUIET(dbp, NULL);
	COMPQUIET(flags, 0);
	return (0);
}

int
__qam_db_create(dbp)
	DB *dbp;
{
	COMPQUIET(dbp, NULL);
	return (0);
}

int
__qam_extent_names(env, name, namelistp)
	ENV *env;
	char *name;
	char ***namelistp;
{
	COMPQUIET(name, NULL);
	COMPQUIET(namelistp, NULL);
	return (__db_no_queue_am(env));
}

int
__qam_gen_filelist(dbp, ip, filelistp)
	DB *dbp;
	DB_THREAD_INFO *ip;
	QUEUE_FILELIST **filelistp;
{
	COMPQUIET(ip, NULL);
	COMPQUIET(filelistp, NULL);
	return (__db_no_queue_am(dbp->env));
}

int
__qam_init_print(env, dtabp)
	ENV *env;
	DB_DISTAB *dtabp;
{
	COMPQUIET(env, NULL);
	COMPQUIET(dtabp, NULL);
	return (0);
}

int
__qam_init_recover(env, dtabp)
	ENV *env;
	DB_DISTAB *dtabp;
{
	COMPQUIET(env, NULL);
	COMPQUIET(dtabp, NULL);
	return (0);
}

int
__qam_metachk(dbp, name, qmeta)
	DB *dbp;
	const char *name;
	QMETA *qmeta;
{
	COMPQUIET(name, NULL);
	COMPQUIET(qmeta, NULL);
	return (__db_no_queue_am(dbp->env));
}

int
__qam_mswap(env, pg)
	ENV *env;
	PAGE *pg;
{
	COMPQUIET(pg, NULL);
	return (__db_no_queue_am(env));
}

int
__qam_new_file(dbp, ip, txn, fhp, name)
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
	return (__db_no_queue_am(dbp->env));
}

int
__qam_open(dbp, ip, txn, name, base_pgno, mode, flags)
	DB *dbp;
	DB_THREAD_INFO *ip;
	DB_TXN *txn;
	const char *name;
	db_pgno_t base_pgno;
	int mode;
	u_int32_t flags;
{
	COMPQUIET(ip, NULL);
	COMPQUIET(txn, NULL);
	COMPQUIET(name, NULL);
	COMPQUIET(base_pgno, 0);
	COMPQUIET(mode, 0);
	COMPQUIET(flags, 0);
	return (__db_no_queue_am(dbp->env));
}

int
__qam_pgin_out(env, pg, pp, cookie)
	ENV *env;
	db_pgno_t pg;
	void *pp;
	DBT *cookie;
{
	COMPQUIET(pg, 0);
	COMPQUIET(pp, NULL);
	COMPQUIET(cookie, NULL);
	return (__db_no_queue_am(env));
}

int
__qam_salvage(dbp, vdp, pgno, h, handle, callback, flags)
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
	return (__db_no_queue_am(dbp->env));
}

int
__qam_set_ext_data(dbp, name)
	DB *dbp;
	const char *name;
{
	COMPQUIET(name, NULL);
	return (__db_no_queue_am(dbp->env));
}

int
__qam_stat(dbc, spp, flags)
	DBC *dbc;
	void *spp;
	u_int32_t flags;
{
	COMPQUIET(spp, NULL);
	COMPQUIET(flags, 0);
	return (__db_no_queue_am(dbc->env));
}

int
__qam_stat_print(dbc, flags)
	DBC *dbc;
	u_int32_t flags;
{
	COMPQUIET(flags, 0);
	return (__db_no_queue_am(dbc->env));
}

int
__qam_sync(dbp)
	DB *dbp;
{
	return (__db_no_queue_am(dbp->env));
}

int
__qam_truncate(dbc, countp)
	DBC *dbc;
	u_int32_t *countp;
{
	COMPQUIET(dbc, NULL);
	COMPQUIET(countp, NULL);
	return (__db_no_queue_am(dbc->env));
}

int
__qam_vrfy_data(dbp, vdp, h, pgno, flags)
	DB *dbp;
	VRFY_DBINFO *vdp;
	QPAGE *h;
	db_pgno_t pgno;
	u_int32_t flags;
{
	COMPQUIET(vdp, NULL);
	COMPQUIET(h, NULL);
	COMPQUIET(pgno, 0);
	COMPQUIET(flags, 0);
	return (__db_no_queue_am(dbp->env));
}

int
__qam_vrfy_meta(dbp, vdp, meta, pgno, flags)
	DB *dbp;
	VRFY_DBINFO *vdp;
	QMETA *meta;
	db_pgno_t pgno;
	u_int32_t flags;
{
	COMPQUIET(vdp, NULL);
	COMPQUIET(meta, NULL);
	COMPQUIET(pgno, 0);
	COMPQUIET(flags, 0);
	return (__db_no_queue_am(dbp->env));
}

int
__qam_vrfy_structure(dbp, vdp, flags)
	DB *dbp;
	VRFY_DBINFO *vdp;
	u_int32_t flags;
{
	COMPQUIET(vdp, NULL);
	COMPQUIET(flags, 0);
	return (__db_no_queue_am(dbp->env));
}

int
__qam_vrfy_walkqueue(dbp, vdp, handle, callback, flags)
	DB *dbp;
	VRFY_DBINFO *vdp;
	void *handle;
	int (*callback) __P((void *, const void *));
	u_int32_t flags;
{
	COMPQUIET(vdp, NULL);
	COMPQUIET(handle, NULL);
	COMPQUIET(callback, NULL);
	COMPQUIET(flags, 0);
	return (__db_no_queue_am(dbp->env));
}
#endif	/* !HAVE_QUEUE */
