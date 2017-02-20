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
#include "dbinc/db_am.h"
#include "dbinc/mp.h"

/*
 * __env_fileid_reset_pp --
 *	ENV->fileid_reset pre/post processing.
 *
 * PUBLIC: int __env_fileid_reset_pp __P((DB_ENV *, const char *, u_int32_t));
 */
int
__env_fileid_reset_pp(dbenv, name, flags)
	DB_ENV *dbenv;
	const char *name;
	u_int32_t flags;
{
	DB_THREAD_INFO *ip;
	ENV *env;
	int ret;

	env = dbenv->env;

	ENV_ILLEGAL_BEFORE_OPEN(env, "DB_ENV->fileid_reset");

	/*
	 * !!!
	 * The actual argument checking is simple, do it inline, outside of
	 * the replication block.
	 */
	if (flags != 0 && flags != DB_ENCRYPT)
		return (__db_ferr(env, "DB_ENV->fileid_reset", 0));

	ENV_ENTER(env, ip);
	REPLICATION_WRAP(env,
	    (__env_fileid_reset(env, ip, name, LF_ISSET(DB_ENCRYPT) ? 1 : 0)),
	    1, ret);
	ENV_LEAVE(env, ip);
	return (ret);
}

/*
 * __env_fileid_reset --
 *	Reset the file IDs for every database in the file.
 * PUBLIC: int __env_fileid_reset
 * PUBLIC:	 __P((ENV *, DB_THREAD_INFO *, const char *, int));
 */
int
__env_fileid_reset(env, ip, name, encrypted)
	ENV *env;
	DB_THREAD_INFO *ip;
	const char *name;
	int encrypted;
{
	DB *dbp;
	DBC *dbcp;
	DBMETA *meta;
	DBT key, data;
	DB_FH *fhp;
	DB_MPOOLFILE *mpf;
	DB_PGINFO cookie;
	db_pgno_t pgno;
	int t_ret, ret;
	size_t n;
	char *real_name;
	u_int8_t fileid[DB_FILE_ID_LEN], mbuf[DBMETASIZE];
	void *pagep;

	dbp = NULL;
	dbcp = NULL;
	fhp = NULL;
	real_name = NULL;

	/* Get the real backing file name. */
	if ((ret = __db_appname(env,
	    DB_APP_DATA, name, NULL, &real_name)) != 0)
		return (ret);

	/* Get a new file ID. */
	if ((ret = __os_fileid(env, real_name, 1, fileid)) != 0)
		goto err;

	/*
	 * The user may have physically copied a file currently open in the
	 * cache, which means if we open this file through the cache before
	 * updating the file ID on page 0, we might connect to the file from
	 * which the copy was made.
	 */
	if ((ret = __os_open(env, real_name, 0, 0, 0, &fhp)) != 0) {
		__db_err(env, ret, "%s", real_name);
		goto err;
	}
	if ((ret = __os_read(env, fhp, mbuf, sizeof(mbuf), &n)) != 0)
		goto err;

	if (n != sizeof(mbuf)) {
		ret = EINVAL;
		__db_errx(env,
		    "__env_fileid_reset: %s: unexpected file type or format",
		    real_name);
		goto err;
	}

	/*
	 * Create the DB object.
	 */
	if ((ret = __db_create_internal(&dbp, env, 0)) != 0)
		goto err;

	/* If configured with a password, the databases are encrypted. */
	if (encrypted && (ret = __db_set_flags(dbp, DB_ENCRYPT)) != 0)
		goto err;

	if ((ret = __db_meta_setup(env,
	    dbp, real_name, (DBMETA *)mbuf, 0, DB_CHK_META)) != 0)
		goto err;

	meta = (DBMETA *)mbuf;
	if (FLD_ISSET(meta->metaflags,
	    DBMETA_PART_RANGE | DBMETA_PART_CALLBACK) && (ret =
	    __part_fileid_reset(env, ip, name, meta->nparts, encrypted)) != 0)
		goto err;

	memcpy(meta->uid, fileid, DB_FILE_ID_LEN);
	cookie.db_pagesize = sizeof(mbuf);
	cookie.flags = dbp->flags;
	cookie.type = dbp->type;
	key.data = &cookie;

	if ((ret = __db_pgout(env->dbenv, 0, mbuf, &key)) != 0)
		goto err;
	if ((ret = __os_seek(env, fhp, 0, 0, 0)) != 0)
		goto err;
	if ((ret = __os_write(env, fhp, mbuf, sizeof(mbuf), &n)) != 0)
		goto err;
	if ((ret = __os_fsync(env, fhp)) != 0)
		goto err;

	/*
	 * Page 0 of the file has an updated file ID, and we can open it in
	 * the cache without connecting to a different, existing file.  Open
	 * the file in the cache, and update the file IDs for subdatabases.
	 * (No existing code, as far as I know, actually uses the file ID of
	 * a subdatabase, but it's cleaner to get them all.)
	 */

	/*
	 * If the database file doesn't support subdatabases, we only have
	 * to update a single metadata page.  Otherwise, we have to open a
	 * cursor and step through the master database, and update all of
	 * the subdatabases' metadata pages.
	 */
	if (meta->type != P_BTREEMETA || !F_ISSET(meta, BTM_SUBDB))
		goto err;

	/*
	 * Open the DB file.
	 *
	 * !!!
	 * Note DB_RDWRMASTER flag, we need to open the master database file
	 * for writing in this case.
	 */
	if ((ret = __db_open(dbp, ip, NULL,
	    name, NULL, DB_UNKNOWN, DB_RDWRMASTER, 0, PGNO_BASE_MD)) != 0)
		goto err;

	mpf = dbp->mpf;
	memset(&key, 0, sizeof(key));
	memset(&data, 0, sizeof(data));
	if ((ret = __db_cursor(dbp, ip, NULL, &dbcp, 0)) != 0)
		goto err;
	while ((ret = __dbc_get(dbcp, &key, &data, DB_NEXT)) == 0) {
		/*
		 * XXX
		 * We're handling actual data, not on-page meta-data, so it
		 * hasn't been converted to/from opposite endian architectures.
		 * Do it explicitly, now.
		 */
		memcpy(&pgno, data.data, sizeof(db_pgno_t));
		DB_NTOHL_SWAP(env, &pgno);
		if ((ret = __memp_fget(mpf, &pgno, ip, NULL,
		    DB_MPOOL_DIRTY, &pagep)) != 0)
			goto err;
		memcpy(((DBMETA *)pagep)->uid, fileid, DB_FILE_ID_LEN);
		if ((ret = __memp_fput(mpf, ip, pagep, dbcp->priority)) != 0)
			goto err;
	}
	if (ret == DB_NOTFOUND)
		ret = 0;

err:	if (dbcp != NULL && (t_ret = __dbc_close(dbcp)) != 0 && ret == 0)
		ret = t_ret;
	if (dbp != NULL && (t_ret = __db_close(dbp, NULL, 0)) != 0 && ret == 0)
		ret = t_ret;
	if (fhp != NULL &&
	    (t_ret = __os_closehandle(env, fhp)) != 0 && ret == 0)
		ret = t_ret;
	if (real_name != NULL)
		__os_free(env, real_name);

	return (ret);
}
