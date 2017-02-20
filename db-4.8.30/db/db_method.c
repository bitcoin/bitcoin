/*-
 * See the file LICENSE for redistribution information.
 *
 * Copyright (c) 1999-2009 Oracle.  All rights reserved.
 *
 * $Id$
 */

#include "db_config.h"

#include "db_int.h"
#include "dbinc/crypto.h"
#include "dbinc/db_page.h"
#include "dbinc/btree.h"
#include "dbinc/hash.h"
#include "dbinc/lock.h"
#include "dbinc/mp.h"
#include "dbinc/qam.h"
#include "dbinc/txn.h"

#ifdef HAVE_RPC
#ifdef HAVE_SYSTEM_INCLUDE_FILES
#include <rpc/rpc.h>
#endif
#include "db_server.h"
#include "dbinc_auto/rpc_client_ext.h"
#endif

static int  __db_get_byteswapped __P((DB *, int *));
static int  __db_get_dbname __P((DB *, const char **, const char **));
static DB_ENV *__db_get_env __P((DB *));
static void __db_get_msgcall
	      __P((DB *, void (**)(const DB_ENV *, const char *)));
static DB_MPOOLFILE *__db_get_mpf __P((DB *));
static int  __db_get_multiple __P((DB *));
static int  __db_get_transactional __P((DB *));
static int  __db_get_type __P((DB *, DBTYPE *dbtype));
static int  __db_init __P((DB *, u_int32_t));
static int  __db_get_alloc __P((DB *, void *(**)(size_t),
		void *(**)(void *, size_t), void (**)(void *)));
static int  __db_set_alloc __P((DB *, void *(*)(size_t),
		void *(*)(void *, size_t), void (*)(void *)));
static int  __db_get_append_recno __P((DB *,
		int (**)(DB *, DBT *, db_recno_t)));
static int  __db_set_append_recno __P((DB *, int (*)(DB *, DBT *, db_recno_t)));
static int  __db_get_cachesize __P((DB *, u_int32_t *, u_int32_t *, int *));
static int  __db_set_cachesize __P((DB *, u_int32_t, u_int32_t, int));
static int  __db_get_create_dir __P((DB *, const char **));
static int  __db_set_create_dir __P((DB *, const char *));
static int  __db_get_dup_compare
		__P((DB *, int (**)(DB *, const DBT *, const DBT *)));
static int  __db_set_dup_compare
		__P((DB *, int (*)(DB *, const DBT *, const DBT *)));
static int  __db_get_encrypt_flags __P((DB *, u_int32_t *));
static int  __db_set_encrypt __P((DB *, const char *, u_int32_t));
static int  __db_get_feedback __P((DB *, void (**)(DB *, int, int)));
static int  __db_set_feedback __P((DB *, void (*)(DB *, int, int)));
static void __db_map_flags __P((DB *, u_int32_t *, u_int32_t *));
static int  __db_get_pagesize __P((DB *, u_int32_t *));
static int  __db_set_paniccall __P((DB *, void (*)(DB_ENV *, int)));
static int  __db_set_priority __P((DB *, DB_CACHE_PRIORITY));
static int  __db_get_priority __P((DB *, DB_CACHE_PRIORITY *));
static void __db_get_errcall __P((DB *,
	      void (**)(const DB_ENV *, const char *, const char *)));
static void __db_set_errcall
	      __P((DB *, void (*)(const DB_ENV *, const char *, const char *)));
static void __db_get_errfile __P((DB *, FILE **));
static void __db_set_errfile __P((DB *, FILE *));
static void __db_get_errpfx __P((DB *, const char **));
static void __db_set_errpfx __P((DB *, const char *));
static void __db_set_msgcall
	      __P((DB *, void (*)(const DB_ENV *, const char *)));
static void __db_get_msgfile __P((DB *, FILE **));
static void __db_set_msgfile __P((DB *, FILE *));
static void __dbh_err __P((DB *, int, const char *, ...));
static void __dbh_errx __P((DB *, const char *, ...));

/*
 * db_create --
 *	DB constructor.
 *
 * EXTERN: int db_create __P((DB **, DB_ENV *, u_int32_t));
 */
int
db_create(dbpp, dbenv, flags)
	DB **dbpp;
	DB_ENV *dbenv;
	u_int32_t flags;
{
	DB_THREAD_INFO *ip;
	ENV *env;
	int ret;

	ip = NULL;
	env = dbenv == NULL ? NULL : dbenv->env;

	/* Check for invalid function flags. */
	if (flags != 0)
		return (__db_ferr(env, "db_create", 0));

	if (env != NULL)
		ENV_ENTER(env, ip);
	ret = __db_create_internal(dbpp, env, flags);
	if (env != NULL)
		ENV_LEAVE(env, ip);

	return (ret);
}

/*
 * __db_create_internal --
 *	DB constructor internal routine.
 *
 * PUBLIC: int __db_create_internal  __P((DB **, ENV *, u_int32_t));
 */
int
__db_create_internal(dbpp, env, flags)
	DB **dbpp;
	ENV *env;
	u_int32_t flags;
{
	DB *dbp;
	DB_ENV *dbenv;
	DB_REP *db_rep;
	int ret;

	*dbpp = NULL;

	/* If we don't have an environment yet, allocate a local one. */
	if (env == NULL) {
		if ((ret = db_env_create(&dbenv, 0)) != 0)
			return (ret);
		env = dbenv->env;
		F_SET(env, ENV_DBLOCAL);
	} else
		dbenv = env->dbenv;

	/* Allocate and initialize the DB handle. */
	if ((ret = __os_calloc(env, 1, sizeof(*dbp), &dbp)) != 0)
		goto err;

	dbp->dbenv = env->dbenv;
	dbp->env = env;
	if ((ret = __db_init(dbp, flags)) != 0)
		goto err;

	MUTEX_LOCK(env, env->mtx_dblist);
	++env->db_ref;
	MUTEX_UNLOCK(env, env->mtx_dblist);

	/*
	 * Set the replication timestamp; it's 0 if we're not in a replicated
	 * environment.  Don't acquire a lock to read the value, even though
	 * it's opaque: all we check later is value equality, nothing else.
	 */
	dbp->timestamp = REP_ON(env) ?
	    ((REGENV *)env->reginfo->primary)->rep_timestamp : 0;
	/*
	 * Set the replication generation number for fid management; valid
	 * replication generations start at 1.  Don't acquire a lock to
	 * read the value.  All we check later is value equality.
	 */
	db_rep = env->rep_handle;
	dbp->fid_gen = REP_ON(env) ? ((REP *)db_rep->region)->gen : 0;

	/* If not RPC, open a backing DB_MPOOLFILE handle in the memory pool. */
	if (!RPC_ON(dbenv) && (ret = __memp_fcreate(env, &dbp->mpf)) != 0)
		goto err;

	dbp->type = DB_UNKNOWN;

	*dbpp = dbp;
	return (0);

err:	if (dbp != NULL) {
		if (dbp->mpf != NULL)
			(void)__memp_fclose(dbp->mpf, 0);
		__os_free(env, dbp);
	}

	if (F_ISSET(env, ENV_DBLOCAL))
		(void)__env_close(dbp->dbenv, 0);

	return (ret);
}

/*
 * __db_init --
 *	Initialize a DB structure.
 */
static int
__db_init(dbp, flags)
	DB *dbp;
	u_int32_t flags;
{
	int ret;

	dbp->locker = NULL;
	LOCK_INIT(dbp->handle_lock);

	TAILQ_INIT(&dbp->free_queue);
	TAILQ_INIT(&dbp->active_queue);
	TAILQ_INIT(&dbp->join_queue);
	LIST_INIT(&dbp->s_secondaries);

	FLD_SET(dbp->am_ok,
	    DB_OK_BTREE | DB_OK_HASH | DB_OK_QUEUE | DB_OK_RECNO);

	/* DB PUBLIC HANDLE LIST BEGIN */
	dbp->associate = __db_associate_pp;
	dbp->associate_foreign = __db_associate_foreign_pp;
	dbp->close = __db_close_pp;
	dbp->compact = __db_compact_pp;
	dbp->cursor = __db_cursor_pp;
	dbp->del = __db_del_pp;
	dbp->dump = __db_dump_pp;
	dbp->err = __dbh_err;
	dbp->errx = __dbh_errx;
	dbp->exists = __db_exists;
	dbp->fd = __db_fd_pp;
	dbp->get = __db_get_pp;
	dbp->get_alloc = __db_get_alloc;
	dbp->get_append_recno = __db_get_append_recno;
	dbp->get_byteswapped = __db_get_byteswapped;
	dbp->get_cachesize = __db_get_cachesize;
	dbp->get_create_dir = __db_get_create_dir;
	dbp->get_dbname = __db_get_dbname;
	dbp->get_dup_compare = __db_get_dup_compare;
	dbp->get_encrypt_flags = __db_get_encrypt_flags;
	dbp->get_env = __db_get_env;
	dbp->get_errcall = __db_get_errcall;
	dbp->get_errfile = __db_get_errfile;
	dbp->get_errpfx = __db_get_errpfx;
	dbp->get_feedback = __db_get_feedback;
	dbp->get_flags = __db_get_flags;
	dbp->get_lorder = __db_get_lorder;
	dbp->get_mpf = __db_get_mpf;
	dbp->get_msgcall = __db_get_msgcall;
	dbp->get_msgfile = __db_get_msgfile;
	dbp->get_multiple = __db_get_multiple;
	dbp->get_open_flags = __db_get_open_flags;
	dbp->get_partition_dirs = __partition_get_dirs;
	dbp->get_partition_callback = __partition_get_callback;
	dbp->get_partition_keys = __partition_get_keys;
	dbp->get_pagesize = __db_get_pagesize;
	dbp->get_priority = __db_get_priority;
	dbp->get_transactional = __db_get_transactional;
	dbp->get_type = __db_get_type;
	dbp->join = __db_join_pp;
	dbp->key_range = __db_key_range_pp;
	dbp->open = __db_open_pp;
	dbp->pget = __db_pget_pp;
	dbp->put = __db_put_pp;
	dbp->remove = __db_remove_pp;
	dbp->rename = __db_rename_pp;
	dbp->set_alloc = __db_set_alloc;
	dbp->set_append_recno = __db_set_append_recno;
	dbp->set_cachesize = __db_set_cachesize;
	dbp->set_create_dir = __db_set_create_dir;
	dbp->set_dup_compare = __db_set_dup_compare;
	dbp->set_encrypt = __db_set_encrypt;
	dbp->set_errcall = __db_set_errcall;
	dbp->set_errfile = __db_set_errfile;
	dbp->set_errpfx = __db_set_errpfx;
	dbp->set_feedback = __db_set_feedback;
	dbp->set_flags = __db_set_flags;
	dbp->set_lorder = __db_set_lorder;
	dbp->set_msgcall = __db_set_msgcall;
	dbp->set_msgfile = __db_set_msgfile;
	dbp->set_pagesize = __db_set_pagesize;
	dbp->set_paniccall = __db_set_paniccall;
	dbp->set_partition = __partition_set;
	dbp->set_partition_dirs = __partition_set_dirs;
	dbp->set_priority = __db_set_priority;
	dbp->sort_multiple = __db_sort_multiple;
	dbp->stat = __db_stat_pp;
	dbp->stat_print = __db_stat_print_pp;
	dbp->sync = __db_sync_pp;
	dbp->truncate = __db_truncate_pp;
	dbp->upgrade = __db_upgrade_pp;
	dbp->verify = __db_verify_pp;
	/* DB PUBLIC HANDLE LIST END */

					/* Access method specific. */
	if ((ret = __bam_db_create(dbp)) != 0)
		return (ret);
	if ((ret = __ham_db_create(dbp)) != 0)
		return (ret);
	if ((ret = __qam_db_create(dbp)) != 0)
		return (ret);

#ifdef HAVE_RPC
	/*
	 * RPC specific: must be last, as we replace methods set by the
	 * access methods.
	 */
	if (RPC_ON(dbp->dbenv)) {
		__dbcl_dbp_init(dbp);
		/*
		 * !!!
		 * We wrap the DB->open method for RPC, and the rpc.src file
		 * can't handle that.
		 */
		dbp->open = __dbcl_db_open_wrap;
		if ((ret = __dbcl_db_create(dbp, dbp->dbenv, flags)) != 0)
			return (ret);
	}
#else
	COMPQUIET(flags, 0);
#endif

	return (0);
}

/*
 * __dbh_am_chk --
 *	Error if an unreasonable method is called.
 *
 * PUBLIC: int __dbh_am_chk __P((DB *, u_int32_t));
 */
int
__dbh_am_chk(dbp, flags)
	DB *dbp;
	u_int32_t flags;
{
	/*
	 * We start out allowing any access methods to be called, and as the
	 * application calls the methods the options become restricted.  The
	 * idea is to quit as soon as an illegal method combination is called.
	 */
	if ((LF_ISSET(DB_OK_BTREE) && FLD_ISSET(dbp->am_ok, DB_OK_BTREE)) ||
	    (LF_ISSET(DB_OK_HASH) && FLD_ISSET(dbp->am_ok, DB_OK_HASH)) ||
	    (LF_ISSET(DB_OK_QUEUE) && FLD_ISSET(dbp->am_ok, DB_OK_QUEUE)) ||
	    (LF_ISSET(DB_OK_RECNO) && FLD_ISSET(dbp->am_ok, DB_OK_RECNO))) {
		FLD_CLR(dbp->am_ok, ~flags);
		return (0);
	}

	__db_errx(dbp->env,
    "call implies an access method which is inconsistent with previous calls");
	return (EINVAL);
}

/*
 * __dbh_err --
 *	Db.err method.
 */
static void
#ifdef STDC_HEADERS
__dbh_err(DB *dbp, int error, const char *fmt, ...)
#else
__dbh_err(dbp, error, fmt, va_alist)
	DB *dbp;
	int error;
	const char *fmt;
	va_dcl
#endif
{
	/* Message with error string, to stderr by default. */
	DB_REAL_ERR(dbp->dbenv, error, DB_ERROR_SET, 1, fmt);
}

/*
 * __dbh_errx --
 *	Db.errx method.
 */
static void
#ifdef STDC_HEADERS
__dbh_errx(DB *dbp, const char *fmt, ...)
#else
__dbh_errx(dbp, fmt, va_alist)
	DB *dbp;
	const char *fmt;
	va_dcl
#endif
{
	/* Message without error string, to stderr by default. */
	DB_REAL_ERR(dbp->dbenv, 0, DB_ERROR_NOT_SET, 1, fmt);
}

/*
 * __db_get_byteswapped --
 *	Return if database requires byte swapping.
 */
static int
__db_get_byteswapped(dbp, isswapped)
	DB *dbp;
	int *isswapped;
{
	DB_ILLEGAL_BEFORE_OPEN(dbp, "DB->get_byteswapped");

	*isswapped = F_ISSET(dbp, DB_AM_SWAP) ? 1 : 0;
	return (0);
}

/*
 * __db_get_dbname --
 *	Get the name of the database as passed to DB->open.
 */
static int
__db_get_dbname(dbp, fnamep, dnamep)
	DB *dbp;
	const char **fnamep, **dnamep;
{
	DB_ILLEGAL_BEFORE_OPEN(dbp, "DB->get_dbname");

	if (fnamep != NULL)
		*fnamep = dbp->fname;
	if (dnamep != NULL)
		*dnamep = dbp->dname;
	return (0);
}

/*
 * __db_get_env --
 *	Get the DB_ENV handle that was passed to db_create.
 */
static DB_ENV *
__db_get_env(dbp)
	DB *dbp;
{
	return (dbp->dbenv);
}

/*
 * __db_get_mpf --
 *	Get the underlying DB_MPOOLFILE handle.
 */
static DB_MPOOLFILE *
__db_get_mpf(dbp)
	DB *dbp;
{
	return (dbp->mpf);
}

/*
 * get_multiple --
 *	Return whether this DB handle references a physical file with multiple
 *	databases.
 */
static int
__db_get_multiple(dbp)
	DB *dbp;
{
	DB_ILLEGAL_BEFORE_OPEN(dbp, "DB->get_multiple");

	/*
	 * Only return TRUE if the handle is for the master database, not for
	 * any subdatabase in the physical file.  If it's a Btree, with the
	 * subdatabases flag set, and the meta-data page has the right value,
	 * return TRUE.  (We don't need to check it's a Btree, I suppose, but
	 * it doesn't hurt.)
	 */
	return (dbp->type == DB_BTREE &&
	    F_ISSET(dbp, DB_AM_SUBDB) &&
	    dbp->meta_pgno == PGNO_BASE_MD ? 1 : 0);
}

/*
 * get_transactional --
 *	Return whether this database was created in a transaction.
 */
static int
__db_get_transactional(dbp)
	DB *dbp;
{
	return (F_ISSET(dbp, DB_AM_TXN) ? 1 : 0);
}

/*
 * __db_get_type --
 *	Return type of underlying database.
 */
static int
__db_get_type(dbp, dbtype)
	DB *dbp;
	DBTYPE *dbtype;
{
	DB_ILLEGAL_BEFORE_OPEN(dbp, "DB->get_type");

	*dbtype = dbp->type;
	return (0);
}

/*
 * __db_get_append_recno --
 *	Get record number append routine.
 */
static int
__db_get_append_recno(dbp, funcp)
	DB *dbp;
	int (**funcp) __P((DB *, DBT *, db_recno_t));
{
	DB_ILLEGAL_METHOD(dbp, DB_OK_QUEUE | DB_OK_RECNO);
	if (funcp)
		*funcp = dbp->db_append_recno;

	return (0);
}
/*
 * __db_set_append_recno --
 *	Set record number append routine.
 */
static int
__db_set_append_recno(dbp, func)
	DB *dbp;
	int (*func) __P((DB *, DBT *, db_recno_t));
{
	DB_ILLEGAL_AFTER_OPEN(dbp, "DB->set_append_recno");
	DB_ILLEGAL_METHOD(dbp, DB_OK_QUEUE | DB_OK_RECNO);

	dbp->db_append_recno = func;

	return (0);
}

/*
 * __db_get_cachesize --
 *	Get underlying cache size.
 */
static int
__db_get_cachesize(dbp, cache_gbytesp, cache_bytesp, ncachep)
	DB *dbp;
	u_int32_t *cache_gbytesp, *cache_bytesp;
	int *ncachep;
{
	DB_ILLEGAL_IN_ENV(dbp, "DB->get_cachesize");

	return (__memp_get_cachesize(dbp->dbenv,
	    cache_gbytesp, cache_bytesp, ncachep));
}

/*
 * __db_set_cachesize --
 *	Set underlying cache size.
 */
static int
__db_set_cachesize(dbp, cache_gbytes, cache_bytes, ncache)
	DB *dbp;
	u_int32_t cache_gbytes, cache_bytes;
	int ncache;
{
	DB_ILLEGAL_IN_ENV(dbp, "DB->set_cachesize");
	DB_ILLEGAL_AFTER_OPEN(dbp, "DB->set_cachesize");

	return (__memp_set_cachesize(
	    dbp->dbenv, cache_gbytes, cache_bytes, ncache));
}

static int
__db_set_create_dir(dbp, dir)
	DB *dbp;
	const char *dir;
{
	DB_ENV *dbenv;
	int i;

	dbenv = dbp->dbenv;

	for (i = 0; i < dbenv->data_next; i++)
		if (strcmp(dir, dbenv->db_data_dir[i]) == 0)
			break;

	if (i == dbenv->data_next) {
		__db_errx(dbp->env,
		     "Directory %s not in environment list.", dir);
		return (EINVAL);
	}

	dbp->dirname = dbenv->db_data_dir[i];
	return (0);
}

static int
__db_get_create_dir(dbp, dirp)
	DB *dbp;
	const char **dirp;
{
	*dirp = dbp->dirname;
	return (0);
}

/*
 * __db_get_dup_compare --
 *	Get duplicate comparison routine.
 */
static int
__db_get_dup_compare(dbp, funcp)
	DB *dbp;
	int (**funcp) __P((DB *, const DBT *, const DBT *));
{

	DB_ILLEGAL_METHOD(dbp, DB_OK_BTREE | DB_OK_HASH);

	if (funcp != NULL) {
#ifdef HAVE_COMPRESSION
		if (DB_IS_COMPRESSED(dbp)) {
			*funcp =
			     ((BTREE *)dbp->bt_internal)->compress_dup_compare;
		} else
#endif
			*funcp = dbp->dup_compare;
	}

	return (0);
}

/*
 * __db_set_dup_compare --
 *	Set duplicate comparison routine.
 */
static int
__db_set_dup_compare(dbp, func)
	DB *dbp;
	int (*func) __P((DB *, const DBT *, const DBT *));
{
	int ret;

	DB_ILLEGAL_AFTER_OPEN(dbp, "DB->set_dup_compare");
	DB_ILLEGAL_METHOD(dbp, DB_OK_BTREE | DB_OK_HASH);

	if ((ret = __db_set_flags(dbp, DB_DUPSORT)) != 0)
		return (ret);

#ifdef HAVE_COMPRESSION
	if (DB_IS_COMPRESSED(dbp)) {
		dbp->dup_compare = __bam_compress_dupcmp;
		((BTREE *)dbp->bt_internal)->compress_dup_compare = func;
	} else
#endif
		dbp->dup_compare = func;

	return (0);
}

/*
 * __db_get_encrypt_flags --
 */
static int
__db_get_encrypt_flags(dbp, flagsp)
	DB *dbp;
	u_int32_t *flagsp;
{
	DB_ILLEGAL_IN_ENV(dbp, "DB->get_encrypt_flags");

	return (__env_get_encrypt_flags(dbp->dbenv, flagsp));
}

/*
 * __db_set_encrypt --
 *	Set database passwd.
 */
static int
__db_set_encrypt(dbp, passwd, flags)
	DB *dbp;
	const char *passwd;
	u_int32_t flags;
{
	DB_CIPHER *db_cipher;
	int ret;

	DB_ILLEGAL_IN_ENV(dbp, "DB->set_encrypt");
	DB_ILLEGAL_AFTER_OPEN(dbp, "DB->set_encrypt");

	if ((ret = __env_set_encrypt(dbp->dbenv, passwd, flags)) != 0)
		return (ret);

	/*
	 * In a real env, this gets initialized with the region.  In a local
	 * env, we must do it here.
	 */
	db_cipher = dbp->env->crypto_handle;
	if (!F_ISSET(db_cipher, CIPHER_ANY) &&
	    (ret = db_cipher->init(dbp->env, db_cipher)) != 0)
		return (ret);

	return (__db_set_flags(dbp, DB_ENCRYPT));
}

static void
__db_get_errcall(dbp, errcallp)
	DB *dbp;
	void (**errcallp) __P((const DB_ENV *, const char *, const char *));
{
	__env_get_errcall(dbp->dbenv, errcallp);
}

static void
__db_set_errcall(dbp, errcall)
	DB *dbp;
	void (*errcall) __P((const DB_ENV *, const char *, const char *));
{
	__env_set_errcall(dbp->dbenv, errcall);
}

static void
__db_get_errfile(dbp, errfilep)
	DB *dbp;
	FILE **errfilep;
{
	__env_get_errfile(dbp->dbenv, errfilep);
}

static void
__db_set_errfile(dbp, errfile)
	DB *dbp;
	FILE *errfile;
{
	__env_set_errfile(dbp->dbenv, errfile);
}

static void
__db_get_errpfx(dbp, errpfxp)
	DB *dbp;
	const char **errpfxp;
{
	__env_get_errpfx(dbp->dbenv, errpfxp);
}

static void
__db_set_errpfx(dbp, errpfx)
	DB *dbp;
	const char *errpfx;
{
	__env_set_errpfx(dbp->dbenv, errpfx);
}

static int
__db_get_feedback(dbp, feedbackp)
	DB *dbp;
	void (**feedbackp) __P((DB *, int, int));
{
	if (feedbackp != NULL)
		*feedbackp = dbp->db_feedback;
	return (0);
}

static int
__db_set_feedback(dbp, feedback)
	DB *dbp;
	void (*feedback) __P((DB *, int, int));
{
	dbp->db_feedback = feedback;
	return (0);
}

/*
 * __db_map_flags --
 *	Maps between public and internal flag values.
 *      This function doesn't check for validity, so it can't fail.
 */
static void
__db_map_flags(dbp, inflagsp, outflagsp)
	DB *dbp;
	u_int32_t *inflagsp, *outflagsp;
{
	COMPQUIET(dbp, NULL);

	if (FLD_ISSET(*inflagsp, DB_CHKSUM)) {
		FLD_SET(*outflagsp, DB_AM_CHKSUM);
		FLD_CLR(*inflagsp, DB_CHKSUM);
	}
	if (FLD_ISSET(*inflagsp, DB_ENCRYPT)) {
		FLD_SET(*outflagsp, DB_AM_ENCRYPT | DB_AM_CHKSUM);
		FLD_CLR(*inflagsp, DB_ENCRYPT);
	}
	if (FLD_ISSET(*inflagsp, DB_TXN_NOT_DURABLE)) {
		FLD_SET(*outflagsp, DB_AM_NOT_DURABLE);
		FLD_CLR(*inflagsp, DB_TXN_NOT_DURABLE);
	}
}

/*
 * __db_get_flags --
 *	The DB->get_flags method.
 *
 * PUBLIC: int __db_get_flags __P((DB *, u_int32_t *));
 */
int
__db_get_flags(dbp, flagsp)
	DB *dbp;
	u_int32_t *flagsp;
{
	static const u_int32_t db_flags[] = {
		DB_CHKSUM,
		DB_DUP,
		DB_DUPSORT,
		DB_ENCRYPT,
#ifdef HAVE_QUEUE
		DB_INORDER,
#endif
		DB_RECNUM,
		DB_RENUMBER,
		DB_REVSPLITOFF,
		DB_SNAPSHOT,
		DB_TXN_NOT_DURABLE,
		0
	};
	u_int32_t f, flags, mapped_flag;
	int i;

	flags = 0;
	for (i = 0; (f = db_flags[i]) != 0; i++) {
		mapped_flag = 0;
		__db_map_flags(dbp, &f, &mapped_flag);
		__bam_map_flags(dbp, &f, &mapped_flag);
		__ram_map_flags(dbp, &f, &mapped_flag);
#ifdef HAVE_QUEUE
		__qam_map_flags(dbp, &f, &mapped_flag);
#endif
		DB_ASSERT(dbp->env, f == 0);
		if (F_ISSET(dbp, mapped_flag) == mapped_flag)
			LF_SET(db_flags[i]);
	}

	*flagsp = flags;
	return (0);
}

/*
 * __db_set_flags --
 *	DB->set_flags.
 *
 * PUBLIC: int  __db_set_flags __P((DB *, u_int32_t));
 */
int
__db_set_flags(dbp, flags)
	DB *dbp;
	u_int32_t flags;
{
	ENV *env;
	int ret;

	env = dbp->env;

	if (LF_ISSET(DB_ENCRYPT) && !CRYPTO_ON(env)) {
		__db_errx(env,
		    "Database environment not configured for encryption");
		return (EINVAL);
	}
	if (LF_ISSET(DB_TXN_NOT_DURABLE))
		ENV_REQUIRES_CONFIG(env,
		    env->tx_handle, "DB_NOT_DURABLE", DB_INIT_TXN);

	__db_map_flags(dbp, &flags, &dbp->flags);

	if ((ret = __bam_set_flags(dbp, &flags)) != 0)
		return (ret);
	if ((ret = __ram_set_flags(dbp, &flags)) != 0)
		return (ret);
#ifdef HAVE_QUEUE
	if ((ret = __qam_set_flags(dbp, &flags)) != 0)
		return (ret);
#endif

	return (flags == 0 ? 0 : __db_ferr(env, "DB->set_flags", 0));
}

/*
 * __db_get_lorder --
 *	Get whether lorder is swapped or not.
 *
 * PUBLIC: int  __db_get_lorder __P((DB *, int *));
 */
int
__db_get_lorder(dbp, db_lorderp)
	DB *dbp;
	int *db_lorderp;
{
	int ret;

	/* Flag if the specified byte order requires swapping. */
	switch (ret = __db_byteorder(dbp->env, 1234)) {
	case 0:
		*db_lorderp = F_ISSET(dbp, DB_AM_SWAP) ? 4321 : 1234;
		break;
	case DB_SWAPBYTES:
		*db_lorderp = F_ISSET(dbp, DB_AM_SWAP) ? 1234 : 4321;
		break;
	default:
		return (ret);
		/* NOTREACHED */
	}

	return (0);
}

/*
 * __db_set_lorder --
 *	Set whether lorder is swapped or not.
 *
 * PUBLIC: int  __db_set_lorder __P((DB *, int));
 */
int
__db_set_lorder(dbp, db_lorder)
	DB *dbp;
	int db_lorder;
{
	int ret;

	DB_ILLEGAL_AFTER_OPEN(dbp, "DB->set_lorder");

	/* Flag if the specified byte order requires swapping. */
	switch (ret = __db_byteorder(dbp->env, db_lorder)) {
	case 0:
		F_CLR(dbp, DB_AM_SWAP);
		break;
	case DB_SWAPBYTES:
		F_SET(dbp, DB_AM_SWAP);
		break;
	default:
		return (ret);
		/* NOTREACHED */
	}
	return (0);
}

static int
__db_get_alloc(dbp, mal_funcp, real_funcp, free_funcp)
	DB *dbp;
	void *(**mal_funcp) __P((size_t));
	void *(**real_funcp) __P((void *, size_t));
	void (**free_funcp) __P((void *));
{
	DB_ILLEGAL_IN_ENV(dbp, "DB->get_alloc");

	return (__env_get_alloc(dbp->dbenv, mal_funcp,
	    real_funcp, free_funcp));
}

static int
__db_set_alloc(dbp, mal_func, real_func, free_func)
	DB *dbp;
	void *(*mal_func) __P((size_t));
	void *(*real_func) __P((void *, size_t));
	void (*free_func) __P((void *));
{
	DB_ILLEGAL_IN_ENV(dbp, "DB->set_alloc");
	DB_ILLEGAL_AFTER_OPEN(dbp, "DB->set_alloc");

	return (__env_set_alloc(dbp->dbenv, mal_func, real_func, free_func));
}

static void
__db_get_msgcall(dbp, msgcallp)
	DB *dbp;
	void (**msgcallp) __P((const DB_ENV *, const char *));
{
	__env_get_msgcall(dbp->dbenv, msgcallp);
}

static void
__db_set_msgcall(dbp, msgcall)
	DB *dbp;
	void (*msgcall) __P((const DB_ENV *, const char *));
{
	__env_set_msgcall(dbp->dbenv, msgcall);
}

static void
__db_get_msgfile(dbp, msgfilep)
	DB *dbp;
	FILE **msgfilep;
{
	__env_get_msgfile(dbp->dbenv, msgfilep);
}

static void
__db_set_msgfile(dbp, msgfile)
	DB *dbp;
	FILE *msgfile;
{
	__env_set_msgfile(dbp->dbenv, msgfile);
}

static int
__db_get_pagesize(dbp, db_pagesizep)
	DB *dbp;
	u_int32_t *db_pagesizep;
{
	*db_pagesizep = dbp->pgsize;
	return (0);
}

/*
 * __db_set_pagesize --
 *	DB->set_pagesize
 *
 * PUBLIC: int  __db_set_pagesize __P((DB *, u_int32_t));
 */
int
__db_set_pagesize(dbp, db_pagesize)
	DB *dbp;
	u_int32_t db_pagesize;
{
	DB_ILLEGAL_AFTER_OPEN(dbp, "DB->set_pagesize");

	if (db_pagesize < DB_MIN_PGSIZE) {
		__db_errx(dbp->env, "page sizes may not be smaller than %lu",
		    (u_long)DB_MIN_PGSIZE);
		return (EINVAL);
	}
	if (db_pagesize > DB_MAX_PGSIZE) {
		__db_errx(dbp->env, "page sizes may not be larger than %lu",
		    (u_long)DB_MAX_PGSIZE);
		return (EINVAL);
	}

	/*
	 * We don't want anything that's not a power-of-2, as we rely on that
	 * for alignment of various types on the pages.
	 */
	if (!POWER_OF_TWO(db_pagesize)) {
		__db_errx(dbp->env, "page sizes must be a power-of-2");
		return (EINVAL);
	}

	/*
	 * XXX
	 * Should we be checking for a page size that's not a multiple of 512,
	 * so that we never try and write less than a disk sector?
	 */
	dbp->pgsize = db_pagesize;

	return (0);
}

static int
__db_set_paniccall(dbp, paniccall)
	DB *dbp;
	void (*paniccall) __P((DB_ENV *, int));
{
	return (__env_set_paniccall(dbp->dbenv, paniccall));
}

static int
__db_set_priority(dbp, priority)
	DB *dbp;
	DB_CACHE_PRIORITY priority;
{
	dbp->priority = priority;
	return (0);
}

static int
__db_get_priority(dbp, priority)
	DB *dbp;
	DB_CACHE_PRIORITY *priority;
{
	*priority = dbp->priority;
	return (0);
}
