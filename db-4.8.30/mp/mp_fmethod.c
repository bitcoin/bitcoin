/*-
 * See the file LICENSE for redistribution information.
 *
 * Copyright (c) 1996-2009 Oracle.  All rights reserved.
 *
 * $Id$
 */

#include "db_config.h"

#include "db_int.h"
#include "dbinc/log.h"
#include "dbinc/mp.h"

static int __memp_get_clear_len __P((DB_MPOOLFILE *, u_int32_t *));
static int __memp_get_lsn_offset __P((DB_MPOOLFILE *, int32_t *));
static int __memp_get_maxsize __P((DB_MPOOLFILE *, u_int32_t *, u_int32_t *));
static int __memp_set_maxsize __P((DB_MPOOLFILE *, u_int32_t, u_int32_t));
static int __memp_get_priority __P((DB_MPOOLFILE *, DB_CACHE_PRIORITY *));
static int __memp_set_priority __P((DB_MPOOLFILE *, DB_CACHE_PRIORITY));

/*
 * __memp_fcreate_pp --
 *	ENV->memp_fcreate pre/post processing.
 *
 * PUBLIC: int __memp_fcreate_pp __P((DB_ENV *, DB_MPOOLFILE **, u_int32_t));
 */
int
__memp_fcreate_pp(dbenv, retp, flags)
	DB_ENV *dbenv;
	DB_MPOOLFILE **retp;
	u_int32_t flags;
{
	DB_THREAD_INFO *ip;
	ENV *env;
	int ret;

	env = dbenv->env;

	/* Validate arguments. */
	if ((ret = __db_fchk(env, "DB_ENV->memp_fcreate", flags, 0)) != 0)
		return (ret);

	if (REP_ON(env)) {
		__db_errx(env,
  "DB_ENV->memp_fcreate: method not permitted when replication is configured");
		return (EINVAL);
	}

	ENV_ENTER(env, ip);
	ret = __memp_fcreate(env, retp);
	ENV_LEAVE(env, ip);
	return (ret);
}

/*
 * __memp_fcreate --
 *	ENV->memp_fcreate.
 *
 * PUBLIC: int __memp_fcreate __P((ENV *, DB_MPOOLFILE **));
 */
int
__memp_fcreate(env, retp)
	ENV *env;
	DB_MPOOLFILE **retp;
{
	DB_MPOOLFILE *dbmfp;
	int ret;

	/* Allocate and initialize the per-process structure. */
	if ((ret = __os_calloc(env, 1, sizeof(DB_MPOOLFILE), &dbmfp)) != 0)
		return (ret);

	dbmfp->ref = 1;
	dbmfp->lsn_offset = DB_LSN_OFF_NOTSET;
	dbmfp->env = env;
	dbmfp->mfp = INVALID_ROFF;

	dbmfp->close = __memp_fclose_pp;
	dbmfp->get = __memp_fget_pp;
	dbmfp->get_clear_len = __memp_get_clear_len;
	dbmfp->get_fileid = __memp_get_fileid;
	dbmfp->get_flags = __memp_get_flags;
	dbmfp->get_ftype = __memp_get_ftype;
	dbmfp->get_last_pgno = __memp_get_last_pgno;
	dbmfp->get_lsn_offset = __memp_get_lsn_offset;
	dbmfp->get_maxsize = __memp_get_maxsize;
	dbmfp->get_pgcookie = __memp_get_pgcookie;
	dbmfp->get_priority = __memp_get_priority;
	dbmfp->open = __memp_fopen_pp;
	dbmfp->put = __memp_fput_pp;
	dbmfp->set_clear_len = __memp_set_clear_len;
	dbmfp->set_fileid = __memp_set_fileid;
	dbmfp->set_flags = __memp_set_flags;
	dbmfp->set_ftype = __memp_set_ftype;
	dbmfp->set_lsn_offset = __memp_set_lsn_offset;
	dbmfp->set_maxsize = __memp_set_maxsize;
	dbmfp->set_pgcookie = __memp_set_pgcookie;
	dbmfp->set_priority = __memp_set_priority;
	dbmfp->sync = __memp_fsync_pp;

	*retp = dbmfp;
	return (0);
}

/*
 * __memp_get_clear_len --
 *	Get the clear length.
 */
static int
__memp_get_clear_len(dbmfp, clear_lenp)
	DB_MPOOLFILE *dbmfp;
	u_int32_t *clear_lenp;
{
	*clear_lenp = dbmfp->clear_len;
	return (0);
}

/*
 * __memp_set_clear_len --
 *	DB_MPOOLFILE->set_clear_len.
 *
 * PUBLIC: int __memp_set_clear_len __P((DB_MPOOLFILE *, u_int32_t));
 */
int
__memp_set_clear_len(dbmfp, clear_len)
	DB_MPOOLFILE *dbmfp;
	u_int32_t clear_len;
{
	MPF_ILLEGAL_AFTER_OPEN(dbmfp, "DB_MPOOLFILE->set_clear_len");

	dbmfp->clear_len = clear_len;
	return (0);
}

/*
 * __memp_get_fileid --
 *	DB_MPOOLFILE->get_fileid.
 *
 * PUBLIC: int __memp_get_fileid __P((DB_MPOOLFILE *, u_int8_t *));
 */
int
__memp_get_fileid(dbmfp, fileid)
	DB_MPOOLFILE *dbmfp;
	u_int8_t *fileid;
{
	if (!F_ISSET(dbmfp, MP_FILEID_SET)) {
		__db_errx(dbmfp->env, "get_fileid: file ID not set");
		return (EINVAL);
	}

	memcpy(fileid, dbmfp->fileid, DB_FILE_ID_LEN);
	return (0);
}

/*
 * __memp_set_fileid --
 *	DB_MPOOLFILE->set_fileid.
 *
 * PUBLIC: int __memp_set_fileid __P((DB_MPOOLFILE *, u_int8_t *));
 */
int
__memp_set_fileid(dbmfp, fileid)
	DB_MPOOLFILE *dbmfp;
	u_int8_t *fileid;
{
	MPF_ILLEGAL_AFTER_OPEN(dbmfp, "DB_MPOOLFILE->set_fileid");

	memcpy(dbmfp->fileid, fileid, DB_FILE_ID_LEN);
	F_SET(dbmfp, MP_FILEID_SET);

	return (0);
}

/*
 * __memp_get_flags --
 *	Get the DB_MPOOLFILE flags;
 *
 * PUBLIC: int __memp_get_flags __P((DB_MPOOLFILE *, u_int32_t *));
 */
int
__memp_get_flags(dbmfp, flagsp)
	DB_MPOOLFILE *dbmfp;
	u_int32_t *flagsp;
{
	MPOOLFILE *mfp;

	mfp = dbmfp->mfp;

	*flagsp = 0;

	if (mfp == NULL)
		*flagsp = FLD_ISSET(dbmfp->config_flags,
		     DB_MPOOL_NOFILE | DB_MPOOL_UNLINK);
	else {
		if (mfp->no_backing_file)
			FLD_SET(*flagsp, DB_MPOOL_NOFILE);
		if (mfp->unlink_on_close)
			FLD_SET(*flagsp, DB_MPOOL_UNLINK);
	}
	return (0);
}

/*
 * __memp_set_flags --
 *	Set the DB_MPOOLFILE flags;
 *
 * PUBLIC: int __memp_set_flags __P((DB_MPOOLFILE *, u_int32_t, int));
 */
int
__memp_set_flags(dbmfp, flags, onoff)
	DB_MPOOLFILE *dbmfp;
	u_int32_t flags;
	int onoff;
{
	ENV *env;
	MPOOLFILE *mfp;
	int ret;

	env = dbmfp->env;
	mfp = dbmfp->mfp;

	switch (flags) {
	case DB_MPOOL_NOFILE:
		if (mfp == NULL)
			if (onoff)
				FLD_SET(dbmfp->config_flags, DB_MPOOL_NOFILE);
			else
				FLD_CLR(dbmfp->config_flags, DB_MPOOL_NOFILE);
		else
			mfp->no_backing_file = onoff;
		break;
	case DB_MPOOL_UNLINK:
		if (mfp == NULL)
			if (onoff)
				FLD_SET(dbmfp->config_flags, DB_MPOOL_UNLINK);
			else
				FLD_CLR(dbmfp->config_flags, DB_MPOOL_UNLINK);
		else
			mfp->unlink_on_close = onoff;
		break;
	default:
		if ((ret = __db_fchk(env, "DB_MPOOLFILE->set_flags",
		    flags, DB_MPOOL_NOFILE | DB_MPOOL_UNLINK)) != 0)
			return (ret);
		break;
	}
	return (0);
}

/*
 * __memp_get_ftype --
 *	Get the file type (as registered).
 *
 * PUBLIC: int __memp_get_ftype __P((DB_MPOOLFILE *, int *));
 */
int
__memp_get_ftype(dbmfp, ftypep)
	DB_MPOOLFILE *dbmfp;
	int *ftypep;
{
	*ftypep = dbmfp->ftype;
	return (0);
}

/*
 * __memp_set_ftype --
 *	DB_MPOOLFILE->set_ftype.
 *
 * PUBLIC: int __memp_set_ftype __P((DB_MPOOLFILE *, int));
 */
int
__memp_set_ftype(dbmfp, ftype)
	DB_MPOOLFILE *dbmfp;
	int ftype;
{
	MPF_ILLEGAL_AFTER_OPEN(dbmfp, "DB_MPOOLFILE->set_ftype");

	dbmfp->ftype = ftype;
	return (0);
}

/*
 * __memp_get_lsn_offset --
 *	Get the page's LSN offset.
 */
static int
__memp_get_lsn_offset(dbmfp, lsn_offsetp)
	DB_MPOOLFILE *dbmfp;
	int32_t *lsn_offsetp;
{
	*lsn_offsetp = dbmfp->lsn_offset;
	return (0);
}

/*
 * __memp_set_lsn_offset --
 *	Set the page's LSN offset.
 *
 * PUBLIC: int __memp_set_lsn_offset __P((DB_MPOOLFILE *, int32_t));
 */
int
__memp_set_lsn_offset(dbmfp, lsn_offset)
	DB_MPOOLFILE *dbmfp;
	int32_t lsn_offset;
{
	MPF_ILLEGAL_AFTER_OPEN(dbmfp, "DB_MPOOLFILE->set_lsn_offset");

	dbmfp->lsn_offset = lsn_offset;
	return (0);
}

/*
 * __memp_get_maxsize --
 *	Get the file's maximum size.
 */
static int
__memp_get_maxsize(dbmfp, gbytesp, bytesp)
	DB_MPOOLFILE *dbmfp;
	u_int32_t *gbytesp, *bytesp;
{
	ENV *env;
	MPOOLFILE *mfp;

	if ((mfp = dbmfp->mfp) == NULL) {
		*gbytesp = dbmfp->gbytes;
		*bytesp = dbmfp->bytes;
	} else {
		env = dbmfp->env;

		MUTEX_LOCK(env, mfp->mutex);
		*gbytesp = (u_int32_t)
		    (mfp->maxpgno / (GIGABYTE / mfp->stat.st_pagesize));
		*bytesp = (u_int32_t)
		    ((mfp->maxpgno % (GIGABYTE / mfp->stat.st_pagesize)) *
		    mfp->stat.st_pagesize);
		MUTEX_UNLOCK(env, mfp->mutex);
	}

	return (0);
}

/*
 * __memp_set_maxsize --
 *	Set the file's maximum size.
 */
static int
__memp_set_maxsize(dbmfp, gbytes, bytes)
	DB_MPOOLFILE *dbmfp;
	u_int32_t gbytes, bytes;
{
	ENV *env;
	MPOOLFILE *mfp;

	if ((mfp = dbmfp->mfp) == NULL) {
		dbmfp->gbytes = gbytes;
		dbmfp->bytes = bytes;
	} else {
		env = dbmfp->env;

		MUTEX_LOCK(env, mfp->mutex);
		mfp->maxpgno = (db_pgno_t)
		    (gbytes * (GIGABYTE / mfp->stat.st_pagesize));
		mfp->maxpgno += (db_pgno_t)
		    ((bytes + mfp->stat.st_pagesize - 1) /
		    mfp->stat.st_pagesize);
		MUTEX_UNLOCK(env, mfp->mutex);
	}

	return (0);
}

/*
 * __memp_get_pgcookie --
 *	Get the pgin/pgout cookie.
 *
 * PUBLIC: int __memp_get_pgcookie __P((DB_MPOOLFILE *, DBT *));
 */
int
__memp_get_pgcookie(dbmfp, pgcookie)
	DB_MPOOLFILE *dbmfp;
	DBT *pgcookie;
{
	if (dbmfp->pgcookie == NULL) {
		pgcookie->size = 0;
		pgcookie->data = "";
	} else
		memcpy(pgcookie, dbmfp->pgcookie, sizeof(DBT));
	return (0);
}

/*
 * __memp_set_pgcookie --
 *	Set the pgin/pgout cookie.
 *
 * PUBLIC: int __memp_set_pgcookie __P((DB_MPOOLFILE *, DBT *));
 */
int
__memp_set_pgcookie(dbmfp, pgcookie)
	DB_MPOOLFILE *dbmfp;
	DBT *pgcookie;
{
	DBT *cookie;
	ENV *env;
	int ret;

	MPF_ILLEGAL_AFTER_OPEN(dbmfp, "DB_MPOOLFILE->set_pgcookie");
	env = dbmfp->env;

	if ((ret = __os_calloc(env, 1, sizeof(*cookie), &cookie)) != 0)
		return (ret);
	if ((ret = __os_malloc(env, pgcookie->size, &cookie->data)) != 0) {
		__os_free(env, cookie);
		return (ret);
	}

	memcpy(cookie->data, pgcookie->data, pgcookie->size);
	cookie->size = pgcookie->size;

	dbmfp->pgcookie = cookie;
	return (0);
}

/*
 * __memp_get_priority --
 *	Set the cache priority for pages from this file.
 */
static int
__memp_get_priority(dbmfp, priorityp)
	DB_MPOOLFILE *dbmfp;
	DB_CACHE_PRIORITY *priorityp;
{
	switch (dbmfp->priority) {
	case MPOOL_PRI_VERY_LOW:
		*priorityp = DB_PRIORITY_VERY_LOW;
		break;
	case MPOOL_PRI_LOW:
		*priorityp = DB_PRIORITY_LOW;
		break;
	case MPOOL_PRI_DEFAULT:
		*priorityp = DB_PRIORITY_DEFAULT;
		break;
	case MPOOL_PRI_HIGH:
		*priorityp = DB_PRIORITY_HIGH;
		break;
	case MPOOL_PRI_VERY_HIGH:
		*priorityp = DB_PRIORITY_VERY_HIGH;
		break;
	default:
		__db_errx(dbmfp->env,
		    "DB_MPOOLFILE->get_priority: unknown priority value: %d",
		    dbmfp->priority);
		return (EINVAL);
	}

	return (0);
}

/*
 * __memp_set_priority --
 *	Set the cache priority for pages from this file.
 */
static int
__memp_set_priority(dbmfp, priority)
	DB_MPOOLFILE *dbmfp;
	DB_CACHE_PRIORITY priority;
{
	switch (priority) {
	case DB_PRIORITY_VERY_LOW:
		dbmfp->priority = MPOOL_PRI_VERY_LOW;
		break;
	case DB_PRIORITY_LOW:
		dbmfp->priority = MPOOL_PRI_LOW;
		break;
	case DB_PRIORITY_DEFAULT:
		dbmfp->priority = MPOOL_PRI_DEFAULT;
		break;
	case DB_PRIORITY_HIGH:
		dbmfp->priority = MPOOL_PRI_HIGH;
		break;
	case DB_PRIORITY_VERY_HIGH:
		dbmfp->priority = MPOOL_PRI_VERY_HIGH;
		break;
	default:
		__db_errx(dbmfp->env,
		    "DB_MPOOLFILE->set_priority: unknown priority value: %d",
		    priority);
		return (EINVAL);
	}

	/* Update the underlying file if we've already opened it. */
	if (dbmfp->mfp != NULL)
		dbmfp->mfp->priority = dbmfp->priority;

	return (0);
}

/*
 * __memp_get_last_pgno --
 *	Return the page number of the last page in the file.
 *
 * !!!
 * The method is undocumented, but the handle is exported, users occasionally
 * ask for it.
 *
 * PUBLIC: int __memp_get_last_pgno __P((DB_MPOOLFILE *, db_pgno_t *));
 */
int
__memp_get_last_pgno(dbmfp, pgnoaddr)
	DB_MPOOLFILE *dbmfp;
	db_pgno_t *pgnoaddr;
{
	ENV *env;
	MPOOLFILE *mfp;

	env = dbmfp->env;
	mfp = dbmfp->mfp;

	MUTEX_LOCK(env, mfp->mutex);
	*pgnoaddr = mfp->last_pgno;
	MUTEX_UNLOCK(env, mfp->mutex);

	return (0);
}

/*
 * __memp_fn --
 *	On errors we print whatever is available as the file name.
 *
 * PUBLIC: char * __memp_fn __P((DB_MPOOLFILE *));
 */
char *
__memp_fn(dbmfp)
	DB_MPOOLFILE *dbmfp;
{
	return (__memp_fns(dbmfp->env->mp_handle, dbmfp->mfp));
}

/*
 * __memp_fns --
 *	On errors we print whatever is available as the file name.
 *
 * PUBLIC: char * __memp_fns __P((DB_MPOOL *, MPOOLFILE *));
 *
 */
char *
__memp_fns(dbmp, mfp)
	DB_MPOOL *dbmp;
	MPOOLFILE *mfp;
{
	if (mfp == NULL || mfp->path_off == 0)
		return ((char *)"unknown");

	return ((char *)R_ADDR(dbmp->reginfo, mfp->path_off));
}
