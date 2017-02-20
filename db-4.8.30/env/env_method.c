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
#include "dbinc/hmac.h"
#include "dbinc/db_page.h"
#include "dbinc/db_am.h"
#include "dbinc/lock.h"
#include "dbinc/log.h"
#include "dbinc/mp.h"
#include "dbinc/txn.h"

#ifdef HAVE_RPC
#ifdef HAVE_SYSTEM_INCLUDE_FILES
#include <rpc/rpc.h>
#endif
#include "db_server.h"
#include "dbinc_auto/rpc_client_ext.h"
#endif

static int  __db_env_init __P((DB_ENV *));
static void __env_err __P((const DB_ENV *, int, const char *, ...));
static void __env_errx __P((const DB_ENV *, const char *, ...));
static int  __env_get_create_dir __P((DB_ENV *, const char **));
static int  __env_get_data_dirs __P((DB_ENV *, const char ***));
static int  __env_get_flags __P((DB_ENV *, u_int32_t *));
static int  __env_get_home __P((DB_ENV *, const char **));
static int  __env_get_intermediate_dir_mode __P((DB_ENV *, const char **));
static int  __env_get_shm_key __P((DB_ENV *, long *));
static int  __env_get_thread_count __P((DB_ENV *, u_int32_t *));
static int  __env_get_thread_id_fn __P((DB_ENV *,
		void (**)(DB_ENV *, pid_t *, db_threadid_t *)));
static int  __env_get_thread_id_string_fn __P((DB_ENV *,
		char * (**)(DB_ENV *, pid_t, db_threadid_t, char *)));
static int  __env_get_timeout __P((DB_ENV *, db_timeout_t *, u_int32_t));
static int  __env_get_tmp_dir __P((DB_ENV *, const char **));
static int  __env_get_verbose __P((DB_ENV *, u_int32_t, int *));
static int  __env_get_app_dispatch
		__P((DB_ENV *, int (**)(DB_ENV *, DBT *, DB_LSN *, db_recops)));
static int  __env_set_app_dispatch
		__P((DB_ENV *, int (*)(DB_ENV *, DBT *, DB_LSN *, db_recops)));
static int __env_set_event_notify
		__P((DB_ENV *, void (*)(DB_ENV *, u_int32_t, void *)));
static int  __env_get_feedback __P((DB_ENV *, void (**)(DB_ENV *, int, int)));
static int  __env_set_feedback __P((DB_ENV *, void (*)(DB_ENV *, int, int)));
static int  __env_get_isalive __P((DB_ENV *,
		int (**)(DB_ENV *, pid_t, db_threadid_t, u_int32_t)));
static int  __env_set_isalive __P((DB_ENV *,
		int (*)(DB_ENV *, pid_t, db_threadid_t, u_int32_t)));
static int  __env_set_thread_id __P((DB_ENV *, void (*)(DB_ENV *,
		pid_t *, db_threadid_t *)));
static int  __env_set_thread_id_string __P((DB_ENV *,
		char * (*)(DB_ENV *, pid_t, db_threadid_t, char *)));
static int  __env_set_rpc_server
		__P((DB_ENV *, void *, const char *, long, long, u_int32_t));

/*
 * db_env_create --
 *	DB_ENV constructor.
 *
 * EXTERN: int db_env_create __P((DB_ENV **, u_int32_t));
 */
int
db_env_create(dbenvpp, flags)
	DB_ENV **dbenvpp;
	u_int32_t flags;
{
	DB_ENV *dbenv;
	ENV *env;
	int ret;

	/*
	 * !!!
	 * Our caller has not yet had the opportunity to reset the panic
	 * state or turn off mutex locking, and so we can neither check
	 * the panic state or acquire a mutex in the DB_ENV create path.
	 *
	 * !!!
	 * We can't call the flags-checking routines, we don't have an
	 * environment yet.
	 */
	if (flags != 0 && !LF_ISSET(DB_RPCCLIENT))
		return (EINVAL);

	/* Allocate the DB_ENV and ENV structures -- we always have both. */
	if ((ret = __os_calloc(NULL, 1, sizeof(DB_ENV), &dbenv)) != 0)
		return (ret);
	if ((ret = __os_calloc(NULL, 1, sizeof(ENV), &env)) != 0)
		goto err;
	dbenv->env = env;
	env->dbenv = dbenv;

#ifdef HAVE_RPC
	if (LF_ISSET(DB_RPCCLIENT))
		F_SET(dbenv, DB_ENV_RPCCLIENT);
#endif
	if ((ret = __db_env_init(dbenv)) != 0 ||
	    (ret = __lock_env_create(dbenv)) != 0 ||
	    (ret = __log_env_create(dbenv)) != 0 ||
	    (ret = __memp_env_create(dbenv)) != 0 ||
#ifdef HAVE_REPLICATION
	    (ret = __rep_env_create(dbenv)) != 0 ||
#endif
	    (ret = __txn_env_create(dbenv)))
		goto err;

#ifdef HAVE_RPC
	/*
	 * RPC specific: must be last, as we replace methods set by the
	 * access methods.
	 */
	if (F_ISSET(dbenv, DB_ENV_RPCCLIENT)) {
		__dbcl_dbenv_init(dbenv);
		/*
		 * !!!
		 * We wrap the DB_ENV->open and close methods for RPC, and
		 * the rpc.src file can't handle that.
		 */
		dbenv->open = __dbcl_env_open_wrap;
		dbenv->close = __dbcl_env_close_wrap;
	}
#endif

	*dbenvpp = dbenv;
	return (0);

err:	__db_env_destroy(dbenv);
	return (ret);
}

/*
 * __db_env_destroy --
 *	DB_ENV destructor.
 *
 * PUBLIC: void __db_env_destroy __P((DB_ENV *));
 */
void
__db_env_destroy(dbenv)
	DB_ENV *dbenv;
{
	__lock_env_destroy(dbenv);
	__log_env_destroy(dbenv);
	__memp_env_destroy(dbenv);
#ifdef HAVE_REPLICATION
	__rep_env_destroy(dbenv);
#endif
	__txn_env_destroy(dbenv);

	/*
	 * Discard the underlying ENV structure.
	 *
	 * XXX
	 * This is wrong, but can't be fixed until we finish the work of
	 * splitting up the DB_ENV and ENV structures so that we don't
	 * touch anything in the ENV as part of the above calls to subsystem
	 * DB_ENV cleanup routines.
	 */
	memset(dbenv->env, CLEAR_BYTE, sizeof(ENV));
	__os_free(NULL, dbenv->env);

	memset(dbenv, CLEAR_BYTE, sizeof(DB_ENV));
	__os_free(NULL, dbenv);
}

/*
 * __db_env_init --
 *	Initialize a DB_ENV structure.
 */
static int
__db_env_init(dbenv)
	DB_ENV *dbenv;
{
	ENV *env;
	/*
	 * !!!
	 * Our caller has not yet had the opportunity to reset the panic
	 * state or turn off mutex locking, and so we can neither check
	 * the panic state or acquire a mutex in the DB_ENV create path.
	 *
	 * Initialize the method handles.
	 */
	/* DB_ENV PUBLIC HANDLE LIST BEGIN */
	dbenv->add_data_dir = __env_add_data_dir;
	dbenv->cdsgroup_begin = __cdsgroup_begin;
	dbenv->close = __env_close_pp;
	dbenv->dbremove = __env_dbremove_pp;
	dbenv->dbrename = __env_dbrename_pp;
	dbenv->err = __env_err;
	dbenv->errx = __env_errx;
	dbenv->failchk = __env_failchk_pp;
	dbenv->fileid_reset = __env_fileid_reset_pp;
	dbenv->get_alloc = __env_get_alloc;
	dbenv->get_app_dispatch = __env_get_app_dispatch;
	dbenv->get_cache_max = __memp_get_cache_max;
	dbenv->get_cachesize = __memp_get_cachesize;
	dbenv->get_create_dir = __env_get_create_dir;
	dbenv->get_data_dirs = __env_get_data_dirs;
	dbenv->get_encrypt_flags = __env_get_encrypt_flags;
	dbenv->get_errcall = __env_get_errcall;
	dbenv->get_errfile = __env_get_errfile;
	dbenv->get_errpfx = __env_get_errpfx;
	dbenv->get_feedback = __env_get_feedback;
	dbenv->get_flags = __env_get_flags;
	dbenv->get_home = __env_get_home;
	dbenv->get_intermediate_dir_mode = __env_get_intermediate_dir_mode;
	dbenv->get_isalive = __env_get_isalive;
	dbenv->get_lg_bsize = __log_get_lg_bsize;
	dbenv->get_lg_dir = __log_get_lg_dir;
	dbenv->get_lg_filemode = __log_get_lg_filemode;
	dbenv->get_lg_max = __log_get_lg_max;
	dbenv->get_lg_regionmax = __log_get_lg_regionmax;
	dbenv->get_lk_conflicts = __lock_get_lk_conflicts;
	dbenv->get_lk_detect = __lock_get_lk_detect;
	dbenv->get_lk_max_lockers = __lock_get_lk_max_lockers;
	dbenv->get_lk_max_locks = __lock_get_lk_max_locks;
	dbenv->get_lk_max_objects = __lock_get_lk_max_objects;
	dbenv->get_lk_partitions = __lock_get_lk_partitions;
	dbenv->get_mp_max_openfd = __memp_get_mp_max_openfd;
	dbenv->get_mp_max_write = __memp_get_mp_max_write;
	dbenv->get_mp_mmapsize = __memp_get_mp_mmapsize;
	dbenv->get_mp_pagesize = __memp_get_mp_pagesize;
	dbenv->get_mp_tablesize = __memp_get_mp_tablesize;
	dbenv->get_msgcall = __env_get_msgcall;
	dbenv->get_msgfile = __env_get_msgfile;
	dbenv->get_open_flags = __env_get_open_flags;
	dbenv->get_shm_key = __env_get_shm_key;
	dbenv->get_thread_count = __env_get_thread_count;
	dbenv->get_thread_id_fn = __env_get_thread_id_fn;
	dbenv->get_thread_id_string_fn = __env_get_thread_id_string_fn;
	dbenv->get_timeout = __env_get_timeout;
	dbenv->get_tmp_dir = __env_get_tmp_dir;
	dbenv->get_tx_max = __txn_get_tx_max;
	dbenv->get_tx_timestamp = __txn_get_tx_timestamp;
	dbenv->get_verbose = __env_get_verbose;
	dbenv->is_bigendian = __db_isbigendian;
	dbenv->lock_detect = __lock_detect_pp;
	dbenv->lock_get = __lock_get_pp;
	dbenv->lock_id = __lock_id_pp;
	dbenv->lock_id_free = __lock_id_free_pp;
	dbenv->lock_put = __lock_put_pp;
	dbenv->lock_stat = __lock_stat_pp;
	dbenv->lock_stat_print = __lock_stat_print_pp;
	dbenv->lock_vec = __lock_vec_pp;
	dbenv->log_archive = __log_archive_pp;
	dbenv->log_cursor = __log_cursor_pp;
	dbenv->log_file = __log_file_pp;
	dbenv->log_flush = __log_flush_pp;
	dbenv->log_get_config = __log_get_config;
	dbenv->log_printf = __log_printf_capi;
	dbenv->log_put = __log_put_pp;
	dbenv->log_set_config = __log_set_config;
	dbenv->log_stat = __log_stat_pp;
	dbenv->log_stat_print = __log_stat_print_pp;
	dbenv->lsn_reset = __env_lsn_reset_pp;
	dbenv->memp_fcreate = __memp_fcreate_pp;
	dbenv->memp_register = __memp_register_pp;
	dbenv->memp_stat = __memp_stat_pp;
	dbenv->memp_stat_print = __memp_stat_print_pp;
	dbenv->memp_sync = __memp_sync_pp;
	dbenv->memp_trickle = __memp_trickle_pp;
	dbenv->mutex_alloc = __mutex_alloc_pp;
	dbenv->mutex_free = __mutex_free_pp;
	dbenv->mutex_get_align = __mutex_get_align;
	dbenv->mutex_get_increment = __mutex_get_increment;
	dbenv->mutex_get_max = __mutex_get_max;
	dbenv->mutex_get_tas_spins = __mutex_get_tas_spins;
	dbenv->mutex_lock = __mutex_lock_pp;
	dbenv->mutex_set_align = __mutex_set_align;
	dbenv->mutex_set_increment = __mutex_set_increment;
	dbenv->mutex_set_max = __mutex_set_max;
	dbenv->mutex_set_tas_spins = __mutex_set_tas_spins;
	dbenv->mutex_stat = __mutex_stat_pp;
	dbenv->mutex_stat_print = __mutex_stat_print_pp;
	dbenv->mutex_unlock = __mutex_unlock_pp;
	dbenv->open = __env_open_pp;
	dbenv->remove = __env_remove;
	dbenv->rep_elect = __rep_elect_pp;
	dbenv->rep_flush = __rep_flush;
	dbenv->rep_get_clockskew = __rep_get_clockskew;
	dbenv->rep_get_config = __rep_get_config;
	dbenv->rep_get_limit = __rep_get_limit;
	dbenv->rep_get_nsites = __rep_get_nsites;
	dbenv->rep_get_priority = __rep_get_priority;
	dbenv->rep_get_request = __rep_get_request;
	dbenv->rep_get_timeout = __rep_get_timeout;
	dbenv->rep_process_message = __rep_process_message_pp;
	dbenv->rep_set_clockskew = __rep_set_clockskew;
	dbenv->rep_set_config = __rep_set_config;
	dbenv->rep_set_limit = __rep_set_limit;
	dbenv->rep_set_nsites = __rep_set_nsites;
	dbenv->rep_set_priority = __rep_set_priority;
	dbenv->rep_set_request = __rep_set_request;
	dbenv->rep_set_timeout = __rep_set_timeout;
	dbenv->rep_set_transport = __rep_set_transport_pp;
	dbenv->rep_start = __rep_start_pp;
	dbenv->rep_stat = __rep_stat_pp;
	dbenv->rep_stat_print = __rep_stat_print_pp;
	dbenv->rep_sync = __rep_sync;
	dbenv->repmgr_add_remote_site = __repmgr_add_remote_site;
	dbenv->repmgr_get_ack_policy = __repmgr_get_ack_policy;
	dbenv->repmgr_set_ack_policy = __repmgr_set_ack_policy;
	dbenv->repmgr_set_local_site = __repmgr_set_local_site;
	dbenv->repmgr_site_list = __repmgr_site_list;
	dbenv->repmgr_start = __repmgr_start;
	dbenv->repmgr_stat = __repmgr_stat_pp;
	dbenv->repmgr_stat_print = __repmgr_stat_print_pp;
	dbenv->set_alloc = __env_set_alloc;
	dbenv->set_app_dispatch = __env_set_app_dispatch;
	dbenv->set_cache_max = __memp_set_cache_max;
	dbenv->set_cachesize = __memp_set_cachesize;
	dbenv->set_create_dir = __env_set_create_dir;
	dbenv->set_data_dir = __env_set_data_dir;
	dbenv->set_encrypt = __env_set_encrypt;
	dbenv->set_errcall = __env_set_errcall;
	dbenv->set_errfile = __env_set_errfile;
	dbenv->set_errpfx = __env_set_errpfx;
	dbenv->set_event_notify = __env_set_event_notify;
	dbenv->set_feedback = __env_set_feedback;
	dbenv->set_flags = __env_set_flags;
	dbenv->set_intermediate_dir_mode = __env_set_intermediate_dir_mode;
	dbenv->set_isalive = __env_set_isalive;
	dbenv->set_lg_bsize = __log_set_lg_bsize;
	dbenv->set_lg_dir = __log_set_lg_dir;
	dbenv->set_lg_filemode = __log_set_lg_filemode;
	dbenv->set_lg_max = __log_set_lg_max;
	dbenv->set_lg_regionmax = __log_set_lg_regionmax;
	dbenv->set_lk_conflicts = __lock_set_lk_conflicts;
	dbenv->set_lk_detect = __lock_set_lk_detect;
	dbenv->set_lk_max_lockers = __lock_set_lk_max_lockers;
	dbenv->set_lk_max_locks = __lock_set_lk_max_locks;
	dbenv->set_lk_max_objects = __lock_set_lk_max_objects;
	dbenv->set_lk_partitions = __lock_set_lk_partitions;
	dbenv->set_mp_max_openfd = __memp_set_mp_max_openfd;
	dbenv->set_mp_max_write = __memp_set_mp_max_write;
	dbenv->set_mp_mmapsize = __memp_set_mp_mmapsize;
	dbenv->set_mp_pagesize = __memp_set_mp_pagesize;
	dbenv->set_mp_tablesize = __memp_set_mp_tablesize;
	dbenv->set_msgcall = __env_set_msgcall;
	dbenv->set_msgfile = __env_set_msgfile;
	dbenv->set_paniccall = __env_set_paniccall;
	dbenv->set_rpc_server = __env_set_rpc_server;
	dbenv->set_shm_key = __env_set_shm_key;
	dbenv->set_thread_count = __env_set_thread_count;
	dbenv->set_thread_id = __env_set_thread_id;
	dbenv->set_thread_id_string = __env_set_thread_id_string;
	dbenv->set_timeout = __env_set_timeout;
	dbenv->set_tmp_dir = __env_set_tmp_dir;
	dbenv->set_tx_max = __txn_set_tx_max;
	dbenv->set_tx_timestamp = __txn_set_tx_timestamp;
	dbenv->set_verbose = __env_set_verbose;
	dbenv->stat_print = __env_stat_print_pp;
	dbenv->txn_begin = __txn_begin_pp;
	dbenv->txn_checkpoint = __txn_checkpoint_pp;
	dbenv->txn_recover = __txn_recover_pp;
	dbenv->txn_stat = __txn_stat_pp;
	dbenv->txn_stat_print = __txn_stat_print_pp;
	/* DB_ENV PUBLIC HANDLE LIST END */

	/* DB_ENV PRIVATE HANDLE LIST BEGIN */
	dbenv->prdbt = __db_prdbt;
	/* DB_ENV PRIVATE HANDLE LIST END */

	dbenv->shm_key = INVALID_REGION_SEGID;
	dbenv->thread_id = __os_id;
	dbenv->thread_id_string = __env_thread_id_string;

	env = dbenv->env;
	__os_id(NULL, &env->pid_cache, NULL);

	env->db_ref = 0;
	TAILQ_INIT(&env->fdlist);

	if (!__db_isbigendian())
		F_SET(env, ENV_LITTLEENDIAN);
	F_SET(env, ENV_NO_OUTPUT_SET);

	return (0);
}

/*
 * __env_err --
 *	DbEnv.err method.
 */
static void
#ifdef STDC_HEADERS
__env_err(const DB_ENV *dbenv, int error, const char *fmt, ...)
#else
__env_err(dbenv, error, fmt, va_alist)
	const DB_ENV *dbenv;
	int error;
	const char *fmt;
	va_dcl
#endif
{
	/* Message with error string, to stderr by default. */
	DB_REAL_ERR(dbenv, error, DB_ERROR_SET, 1, fmt);
}

/*
 * __env_errx --
 *	DbEnv.errx method.
 */
static void
#ifdef STDC_HEADERS
__env_errx(const DB_ENV *dbenv, const char *fmt, ...)
#else
__env_errx(dbenv, fmt, va_alist)
	const DB_ENV *dbenv;
	const char *fmt;
	va_dcl
#endif
{
	/* Message without error string, to stderr by default. */
	DB_REAL_ERR(dbenv, 0, DB_ERROR_NOT_SET, 1, fmt);
}

static int
__env_get_home(dbenv, homep)
	DB_ENV *dbenv;
	const char **homep;
{
	ENV *env;

	env = dbenv->env;

	ENV_ILLEGAL_BEFORE_OPEN(env, "DB_ENV->get_home");
	*homep = env->db_home;

	return (0);
}

/*
 * __env_get_alloc --
 *	{DB_ENV,DB}->get_alloc.
 *
 * PUBLIC: int  __env_get_alloc __P((DB_ENV *, void *(**)(size_t),
 * PUBLIC:          void *(**)(void *, size_t), void (**)(void *)));
 */
int
__env_get_alloc(dbenv, mal_funcp, real_funcp, free_funcp)
	DB_ENV *dbenv;
	void *(**mal_funcp) __P((size_t));
	void *(**real_funcp) __P((void *, size_t));
	void (**free_funcp) __P((void *));
{

	if (mal_funcp != NULL)
		*mal_funcp = dbenv->db_malloc;
	if (real_funcp != NULL)
		*real_funcp = dbenv->db_realloc;
	if (free_funcp != NULL)
		*free_funcp = dbenv->db_free;
	return (0);
}

/*
 * __env_set_alloc --
 *	{DB_ENV,DB}->set_alloc.
 *
 * PUBLIC: int  __env_set_alloc __P((DB_ENV *, void *(*)(size_t),
 * PUBLIC:          void *(*)(void *, size_t), void (*)(void *)));
 */
int
__env_set_alloc(dbenv, mal_func, real_func, free_func)
	DB_ENV *dbenv;
	void *(*mal_func) __P((size_t));
	void *(*real_func) __P((void *, size_t));
	void (*free_func) __P((void *));
{
	ENV *env;

	env = dbenv->env;

	ENV_ILLEGAL_AFTER_OPEN(env, "DB_ENV->set_alloc");

	dbenv->db_malloc = mal_func;
	dbenv->db_realloc = real_func;
	dbenv->db_free = free_func;
	return (0);
}

/*
 * __env_get_app_dispatch --
 *	Get the transaction abort recover function.
 */
static int
__env_get_app_dispatch(dbenv, app_dispatchp)
	DB_ENV *dbenv;
	int (**app_dispatchp) __P((DB_ENV *, DBT *, DB_LSN *, db_recops));
{

	if (app_dispatchp != NULL)
		*app_dispatchp = dbenv->app_dispatch;
	return (0);
}

/*
 * __env_set_app_dispatch --
 *	Set the transaction abort recover function.
 */
static int
__env_set_app_dispatch(dbenv, app_dispatch)
	DB_ENV *dbenv;
	int (*app_dispatch) __P((DB_ENV *, DBT *, DB_LSN *, db_recops));
{
	ENV *env;

	env = dbenv->env;

	ENV_ILLEGAL_AFTER_OPEN(env, "DB_ENV->set_app_dispatch");

	dbenv->app_dispatch = app_dispatch;
	return (0);
}

/*
 * __env_get_encrypt_flags --
 *	{DB_ENV,DB}->get_encrypt_flags.
 *
 * PUBLIC: int __env_get_encrypt_flags __P((DB_ENV *, u_int32_t *));
 */
int
__env_get_encrypt_flags(dbenv, flagsp)
	DB_ENV *dbenv;
	u_int32_t *flagsp;
{
#ifdef HAVE_CRYPTO
	DB_CIPHER *db_cipher;
#endif
	ENV *env;

	env = dbenv->env;

#ifdef HAVE_CRYPTO
	db_cipher = env->crypto_handle;
	if (db_cipher != NULL && db_cipher->alg == CIPHER_AES)
		*flagsp = DB_ENCRYPT_AES;
	else
		*flagsp = 0;
	return (0);
#else
	COMPQUIET(flagsp, 0);
	__db_errx(env,
	    "library build did not include support for cryptography");
	return (DB_OPNOTSUP);
#endif
}

/*
 * __env_set_encrypt --
 *	DB_ENV->set_encrypt.
 *
 * PUBLIC: int __env_set_encrypt __P((DB_ENV *, const char *, u_int32_t));
 */
int
__env_set_encrypt(dbenv, passwd, flags)
	DB_ENV *dbenv;
	const char *passwd;
	u_int32_t flags;
{
#ifdef HAVE_CRYPTO
	DB_CIPHER *db_cipher;
	ENV *env;
	int ret;

	env = dbenv->env;

	ENV_ILLEGAL_AFTER_OPEN(env, "DB_ENV->set_encrypt");
#define	OK_CRYPTO_FLAGS	(DB_ENCRYPT_AES)

	if (flags != 0 && LF_ISSET(~OK_CRYPTO_FLAGS))
		return (__db_ferr(env, "DB_ENV->set_encrypt", 0));

	if (passwd == NULL || strlen(passwd) == 0) {
		__db_errx(env, "Empty password specified to set_encrypt");
		return (EINVAL);
	}
	if (!CRYPTO_ON(env)) {
		if ((ret = __os_calloc(env, 1, sizeof(DB_CIPHER), &db_cipher))
		    != 0)
			goto err;
		env->crypto_handle = db_cipher;
	} else
		db_cipher = env->crypto_handle;

	if (dbenv->passwd != NULL)
		__os_free(env, dbenv->passwd);
	if ((ret = __os_strdup(env, passwd, &dbenv->passwd)) != 0) {
		__os_free(env, db_cipher);
		goto err;
	}
	/*
	 * We're going to need this often enough to keep around
	 */
	dbenv->passwd_len = strlen(dbenv->passwd) + 1;
	/*
	 * The MAC key is for checksumming, and is separate from
	 * the algorithm.  So initialize it here, even if they
	 * are using CIPHER_ANY.
	 */
	__db_derive_mac(
	    (u_int8_t *)dbenv->passwd, dbenv->passwd_len, db_cipher->mac_key);
	switch (flags) {
	case 0:
		F_SET(db_cipher, CIPHER_ANY);
		break;
	case DB_ENCRYPT_AES:
		if ((ret =
		    __crypto_algsetup(env, db_cipher, CIPHER_AES, 0)) != 0)
			goto err1;
		break;
	default:				/* Impossible. */
		break;
	}
	return (0);

err1:
	__os_free(env, dbenv->passwd);
	__os_free(env, db_cipher);
	env->crypto_handle = NULL;
err:
	return (ret);
#else
	COMPQUIET(passwd, NULL);
	COMPQUIET(flags, 0);

	__db_errx(dbenv->env,
	    "library build did not include support for cryptography");
	return (DB_OPNOTSUP);
#endif
}
#ifndef HAVE_BREW
static
#endif
const FLAG_MAP EnvMap[] = {
	{ DB_AUTO_COMMIT,	DB_ENV_AUTO_COMMIT },
	{ DB_CDB_ALLDB,		DB_ENV_CDB_ALLDB },
	{ DB_DIRECT_DB,		DB_ENV_DIRECT_DB },
	{ DB_DSYNC_DB,		DB_ENV_DSYNC_DB },
	{ DB_MULTIVERSION,	DB_ENV_MULTIVERSION },
	{ DB_NOLOCKING,		DB_ENV_NOLOCKING },
	{ DB_NOMMAP,		DB_ENV_NOMMAP },
	{ DB_NOPANIC,		DB_ENV_NOPANIC },
	{ DB_OVERWRITE,		DB_ENV_OVERWRITE },
	{ DB_REGION_INIT,	DB_ENV_REGION_INIT },
	{ DB_TIME_NOTGRANTED,	DB_ENV_TIME_NOTGRANTED },
	{ DB_TXN_NOSYNC,	DB_ENV_TXN_NOSYNC },
	{ DB_TXN_NOWAIT,	DB_ENV_TXN_NOWAIT },
	{ DB_TXN_SNAPSHOT,	DB_ENV_TXN_SNAPSHOT },
	{ DB_TXN_WRITE_NOSYNC,	DB_ENV_TXN_WRITE_NOSYNC },
	{ DB_YIELDCPU,		DB_ENV_YIELDCPU }
};

/*
 * __env_map_flags -- map from external to internal flags.
 * PUBLIC: void __env_map_flags __P((const FLAG_MAP *,
 * PUBLIC:       u_int, u_int32_t *, u_int32_t *));
 */
void
__env_map_flags(flagmap, mapsize, inflagsp, outflagsp)
	const FLAG_MAP *flagmap;
	u_int mapsize;
	u_int32_t *inflagsp, *outflagsp;
{

	const FLAG_MAP *fmp;
	u_int i;

	for (i = 0, fmp = flagmap;
	    i < mapsize / sizeof(flagmap[0]); ++i, ++fmp)
		if (FLD_ISSET(*inflagsp, fmp->inflag)) {
			FLD_SET(*outflagsp, fmp->outflag);
			FLD_CLR(*inflagsp, fmp->inflag);
			if (*inflagsp == 0)
				break;
		}
}

/*
 * __env_fetch_flags -- map from internal to external flags.
 * PUBLIC: void __env_fetch_flags __P((const FLAG_MAP *,
 * PUBLIC:       u_int, u_int32_t *, u_int32_t *));
 */
void
__env_fetch_flags(flagmap, mapsize, inflagsp, outflagsp)
	const FLAG_MAP *flagmap;
	u_int mapsize;
	u_int32_t *inflagsp, *outflagsp;
{
	const FLAG_MAP *fmp;
	u_int32_t i;

	*outflagsp = 0;
	for (i = 0, fmp = flagmap;
	    i < mapsize / sizeof(flagmap[0]); ++i, ++fmp)
		if (FLD_ISSET(*inflagsp, fmp->outflag))
			FLD_SET(*outflagsp, fmp->inflag);
}

static int
__env_get_flags(dbenv, flagsp)
	DB_ENV *dbenv;
	u_int32_t *flagsp;
{
	ENV *env;

	__env_fetch_flags(EnvMap, sizeof(EnvMap), &dbenv->flags, flagsp);

	env = dbenv->env;
	/* Some flags are persisted in the regions. */
	if (env->reginfo != NULL &&
	    ((REGENV *)env->reginfo->primary)->panic != 0)
		FLD_SET(*flagsp, DB_PANIC_ENVIRONMENT);

	return (0);
}

/*
 * __env_set_flags --
 *	DB_ENV->set_flags.
 *
 * PUBLIC: int  __env_set_flags __P((DB_ENV *, u_int32_t, int));
 */
int
__env_set_flags(dbenv, flags, on)
	DB_ENV *dbenv;
	u_int32_t flags;
	int on;
{
	ENV *env;
	u_int32_t mapped_flags;
	int mem_on, ret;

	env = dbenv->env;

#define	OK_FLAGS							\
	(DB_AUTO_COMMIT | DB_CDB_ALLDB | DB_DIRECT_DB |			\
	    DB_DSYNC_DB |  DB_MULTIVERSION | DB_NOLOCKING |		\
	    DB_NOMMAP | DB_NOPANIC | DB_OVERWRITE |			\
	    DB_PANIC_ENVIRONMENT | DB_REGION_INIT |			\
	    DB_TIME_NOTGRANTED | DB_TXN_NOSYNC | DB_TXN_NOWAIT |	\
	    DB_TXN_SNAPSHOT |	DB_TXN_WRITE_NOSYNC | DB_YIELDCPU)

	if (LF_ISSET(~OK_FLAGS))
		return (__db_ferr(env, "DB_ENV->set_flags", 0));
	if (on) {
		if ((ret = __db_fcchk(env, "DB_ENV->set_flags",
		    flags, DB_TXN_NOSYNC, DB_TXN_WRITE_NOSYNC)) != 0)
			return (ret);
		if (LF_ISSET(DB_DIRECT_DB) && __os_support_direct_io() == 0) {
			__db_errx(env,
	"DB_ENV->set_flags: direct I/O either not configured or not supported");
			return (EINVAL);
		}
	}

	if (LF_ISSET(DB_CDB_ALLDB))
		ENV_ILLEGAL_AFTER_OPEN(env,
		    "DB_ENV->set_flags: DB_CDB_ALLDB");
	if (LF_ISSET(DB_PANIC_ENVIRONMENT)) {
		ENV_ILLEGAL_BEFORE_OPEN(env,
		    "DB_ENV->set_flags: DB_PANIC_ENVIRONMENT");
		if (on) {
			__db_errx(env, "Environment panic set");
			(void)__env_panic(env, DB_RUNRECOVERY);
		} else
			__env_panic_set(env, 0);
	}
	if (LF_ISSET(DB_REGION_INIT))
		ENV_ILLEGAL_AFTER_OPEN(env,
		    "DB_ENV->set_flags: DB_REGION_INIT");

	/*
	 * DB_LOG_IN_MEMORY, DB_TXN_NOSYNC and DB_TXN_WRITE_NOSYNC are
	 * mutually incompatible.  If we're setting one of them, clear all
	 * current settings.  If the environment is open, check to see that
	 * logging is not in memory.
	 */
	if (LF_ISSET(DB_TXN_NOSYNC | DB_TXN_WRITE_NOSYNC)) {
		F_CLR(dbenv, DB_ENV_TXN_NOSYNC | DB_ENV_TXN_WRITE_NOSYNC);
		if (!F_ISSET(env, ENV_OPEN_CALLED)) {
		    if ((ret =
			__log_set_config(dbenv, DB_LOG_IN_MEMORY, 0)) != 0)
				return (ret);
		} else if (LOGGING_ON(env)) {
			if ((ret = __log_get_config(dbenv,
			    DB_LOG_IN_MEMORY, &mem_on)) != 0)
				return (ret);
			if (mem_on == 1) {
				__db_errx(env,
"DB_TXN_NOSYNC and DB_TXN_WRITE_NOSYNC may not be used with DB_LOG_IN_MEMORY");
				return (EINVAL);
			}
		}
	}

	mapped_flags = 0;
	__env_map_flags(EnvMap, sizeof(EnvMap), &flags, &mapped_flags);
	if (on)
		F_SET(dbenv, mapped_flags);
	else
		F_CLR(dbenv, mapped_flags);

	return (0);
}

static int
__env_get_data_dirs(dbenv, dirpp)
	DB_ENV *dbenv;
	const char ***dirpp;
{
	*dirpp = (const char **)dbenv->db_data_dir;
	return (0);
}

/*
 * __env_set_data_dir --
 *	DB_ENV->set_data_dir.
 *
 * PUBLIC: int  __env_set_data_dir __P((DB_ENV *, const char *));
 */
int
__env_set_data_dir(dbenv, dir)
	DB_ENV *dbenv;
	const char *dir;
{
	int ret;

	if ((ret = __env_add_data_dir(dbenv, dir)) != 0)
		return (ret);

	if (dbenv->data_next == 1)
		return (__env_set_create_dir(dbenv, dir));

	return (0);
}

/*
 * __env_add_data_dir --
 *	DB_ENV->add_data_dir.
 *
 * PUBLIC: int  __env_add_data_dir __P((DB_ENV *, const char *));
 */
int
__env_add_data_dir(dbenv, dir)
	DB_ENV *dbenv;
	const char *dir;
{
	ENV *env;
	int ret;

	env = dbenv->env;
	ENV_ILLEGAL_AFTER_OPEN(env, "DB_ENV->add_data_dir");

	/*
	 * The array is NULL-terminated so it can be returned by get_data_dirs
	 * without a length.
	 */

#define	DATA_INIT_CNT	20			/* Start with 20 data slots. */
	if (dbenv->db_data_dir == NULL) {
		if ((ret = __os_calloc(env, DATA_INIT_CNT,
		    sizeof(char **), &dbenv->db_data_dir)) != 0)
			return (ret);
		dbenv->data_cnt = DATA_INIT_CNT;
	} else if (dbenv->data_next == dbenv->data_cnt - 2) {
		dbenv->data_cnt *= 2;
		if ((ret = __os_realloc(env,
		    (u_int)dbenv->data_cnt * sizeof(char **),
		    &dbenv->db_data_dir)) != 0)
			return (ret);
	}

	ret = __os_strdup(env,
	    dir, &dbenv->db_data_dir[dbenv->data_next++]);
	dbenv->db_data_dir[dbenv->data_next] = NULL;
	return (ret);
}

/*
 * __env_set_create_dir --
 *	DB_ENV->set_create_dir.
 * The list of directories cannot change after opening the env and setting
 * a pointer must be atomic so we do not need to mutex here even if multiple
 * threads are using the DB_ENV handle.
 *
 * PUBLIC: int  __env_set_create_dir __P((DB_ENV *, const char *));
 */
int
__env_set_create_dir(dbenv, dir)
	DB_ENV *dbenv;
	const char *dir;
{
	ENV *env;
	int i;

	env = dbenv->env;

	for (i = 0; i < dbenv->data_next; i++)
		if (strcmp(dir, dbenv->db_data_dir[i]) == 0)
			break;

	if (i == dbenv->data_next) {
		__db_errx(env, "Directory %s not in environment list.", dir);
		return (EINVAL);
	}

	dbenv->db_create_dir = dbenv->db_data_dir[i];
	return (0);
}

static int
__env_get_create_dir(dbenv, dirp)
	DB_ENV *dbenv;
	const char **dirp;
{
	*dirp = dbenv->db_create_dir;
	return (0);
}

static int
__env_get_intermediate_dir_mode(dbenv, modep)
	DB_ENV *dbenv;
	const char **modep;
{
	*modep = dbenv->intermediate_dir_mode;
	return (0);
}

/*
 * __env_set_intermediate_dir_mode --
 *	DB_ENV->set_intermediate_dir_mode.
 *
 * PUBLIC: int  __env_set_intermediate_dir_mode __P((DB_ENV *, const char *));
 */
int
__env_set_intermediate_dir_mode(dbenv, mode)
	DB_ENV *dbenv;
	const char *mode;
{
	ENV *env;
	u_int t;
	int ret;

	env = dbenv->env;

	ENV_ILLEGAL_AFTER_OPEN(env, "DB_ENV->set_intermediate_dir_mode");

#define	__SETMODE(offset, valid_ch, mask) {				\
	if (mode[offset] == (valid_ch))					\
		t |= (mask);						\
	else if (mode[offset] != '-')					\
		goto format_err;					\
}
	t = 0;
	__SETMODE(0, 'r', S_IRUSR);
	__SETMODE(1, 'w', S_IWUSR);
	__SETMODE(2, 'x', S_IXUSR);
	__SETMODE(3, 'r', S_IRGRP);
	__SETMODE(4, 'w', S_IWGRP);
	__SETMODE(5, 'x', S_IXGRP);
	__SETMODE(6, 'r', S_IROTH);
	__SETMODE(7, 'w', S_IWOTH);
	__SETMODE(8, 'x', S_IXOTH);
	if (mode[9] != '\0' || t == 0) {
		/*
		 * We disallow modes of 0 -- we use 0 to decide the application
		 * never configured intermediate directory permissions, and we
		 * shouldn't create intermediate directories.  Besides, setting
		 * the permissions to 0 makes no sense.
		 */
format_err:	__db_errx(env,
	    "DB_ENV->set_intermediate_dir_mode: illegal mode \"%s\"", mode);
		return (EINVAL);
	}

	if (dbenv->intermediate_dir_mode != NULL)
		__os_free(env, dbenv->intermediate_dir_mode);
	if ((ret = __os_strdup(env, mode, &dbenv->intermediate_dir_mode)) != 0)
		return (ret);

	env->dir_mode = (int)t;
	return (0);
}

/*
 * __env_get_errcall --
 *	{DB_ENV,DB}->get_errcall.
 *
 * PUBLIC: void __env_get_errcall __P((DB_ENV *,
 * PUBLIC:		void (**)(const DB_ENV *, const char *, const char *)));
 */
void
__env_get_errcall(dbenv, errcallp)
	DB_ENV *dbenv;
	void (**errcallp) __P((const DB_ENV *, const char *, const char *));
{
	*errcallp = dbenv->db_errcall;
}

/*
 * __env_set_errcall --
 *	{DB_ENV,DB}->set_errcall.
 *
 * PUBLIC: void __env_set_errcall __P((DB_ENV *,
 * PUBLIC:		void (*)(const DB_ENV *, const char *, const char *)));
 */
void
__env_set_errcall(dbenv, errcall)
	DB_ENV *dbenv;
	void (*errcall) __P((const DB_ENV *, const char *, const char *));
{
	ENV *env;

	env = dbenv->env;

	F_CLR(env, ENV_NO_OUTPUT_SET);
	dbenv->db_errcall = errcall;
}

/*
 * __env_get_errfile --
 *	{DB_ENV,DB}->get_errfile.
 *
 * PUBLIC: void __env_get_errfile __P((DB_ENV *, FILE **));
 */
void
__env_get_errfile(dbenv, errfilep)
	DB_ENV *dbenv;
	FILE **errfilep;
{
	*errfilep = dbenv->db_errfile;
}

/*
 * __env_set_errfile --
 *	{DB_ENV,DB}->set_errfile.
 *
 * PUBLIC: void __env_set_errfile __P((DB_ENV *, FILE *));
 */
void
__env_set_errfile(dbenv, errfile)
	DB_ENV *dbenv;
	FILE *errfile;
{
	ENV *env;

	env = dbenv->env;

	F_CLR(env, ENV_NO_OUTPUT_SET);
	dbenv->db_errfile = errfile;
}

/*
 * __env_get_errpfx --
 *	{DB_ENV,DB}->get_errpfx.
 *
 * PUBLIC: void __env_get_errpfx __P((DB_ENV *, const char **));
 */
void
__env_get_errpfx(dbenv, errpfxp)
	DB_ENV *dbenv;
	const char **errpfxp;
{
	*errpfxp = dbenv->db_errpfx;
}

/*
 * __env_set_errpfx --
 *	{DB_ENV,DB}->set_errpfx.
 *
 * PUBLIC: void __env_set_errpfx __P((DB_ENV *, const char *));
 */
void
__env_set_errpfx(dbenv, errpfx)
	DB_ENV *dbenv;
	const char *errpfx;
{
	dbenv->db_errpfx = errpfx;
}

static int
__env_get_feedback(dbenv, feedbackp)
	DB_ENV *dbenv;
	void (**feedbackp) __P((DB_ENV *, int, int));
{
	if (feedbackp != NULL)
		*feedbackp = dbenv->db_feedback;
	return (0);
}

static int
__env_set_feedback(dbenv, feedback)
	DB_ENV *dbenv;
	void (*feedback) __P((DB_ENV *, int, int));
{
	dbenv->db_feedback = feedback;
	return (0);
}

/*
 * __env_get_thread_id_fn --
 *	DB_ENV->get_thread_id_fn
 */
static int
__env_get_thread_id_fn(dbenv, idp)
	DB_ENV *dbenv;
	void (**idp) __P((DB_ENV *, pid_t *, db_threadid_t *));
{
	if (idp != NULL)
		*idp = dbenv->thread_id;
	return (0);
}

/*
 * __env_set_thread_id --
 *	DB_ENV->set_thread_id
 */
static int
__env_set_thread_id(dbenv, id)
	DB_ENV *dbenv;
	void (*id) __P((DB_ENV *, pid_t *, db_threadid_t *));
{
	dbenv->thread_id = id;
	return (0);
}

/*
 * __env_get_threadid_string_fn --
 *	DB_ENV->get_threadid_string_fn
 */
static int
__env_get_thread_id_string_fn(dbenv, thread_id_stringp)
	DB_ENV *dbenv;
	char *(**thread_id_stringp)
	    __P((DB_ENV *, pid_t, db_threadid_t, char *));
{
	if (thread_id_stringp != NULL)
		*thread_id_stringp = dbenv->thread_id_string;
	return (0);
}

/*
 * __env_set_threadid_string --
 *	DB_ENV->set_threadid_string
 */
static int
__env_set_thread_id_string(dbenv, thread_id_string)
	DB_ENV *dbenv;
	char *(*thread_id_string) __P((DB_ENV *, pid_t, db_threadid_t, char *));
{
	dbenv->thread_id_string = thread_id_string;
	return (0);
}

/*
 * __env_get_isalive --
 *	DB_ENV->get_isalive
 */
static int
__env_get_isalive(dbenv, is_alivep)
	DB_ENV *dbenv;
	int (**is_alivep) __P((DB_ENV *, pid_t, db_threadid_t, u_int32_t));
{
	ENV *env;

	env = dbenv->env;

	if (F_ISSET(env, ENV_OPEN_CALLED) && env->thr_nbucket == 0) {
		__db_errx(env,
		    "is_alive method specified but no thread region allocated");
		return (EINVAL);
	}
	if (is_alivep != NULL)
		*is_alivep = dbenv->is_alive;
	return (0);
}

/*
 * __env_set_isalive --
 *	DB_ENV->set_isalive
 */
static int
__env_set_isalive(dbenv, is_alive)
	DB_ENV *dbenv;
	int (*is_alive) __P((DB_ENV *, pid_t, db_threadid_t, u_int32_t));
{
	ENV *env;

	env = dbenv->env;

	if (F_ISSET(env, ENV_OPEN_CALLED) && env->thr_nbucket == 0) {
		__db_errx(env,
		    "is_alive method specified but no thread region allocated");
		return (EINVAL);
	}
	dbenv->is_alive = is_alive;
	return (0);
}

/*
 * __env_get_thread_count --
 *	DB_ENV->get_thread_count
 */
static int
__env_get_thread_count(dbenv, countp)
	DB_ENV *dbenv;
	u_int32_t *countp;
{
	*countp = dbenv->thr_max;
	return (0);
}

/*
 * __env_set_thread_count --
 *	DB_ENV->set_thread_count
 *
 * PUBLIC: int  __env_set_thread_count __P((DB_ENV *, u_int32_t));
 */
int
__env_set_thread_count(dbenv, count)
	DB_ENV *dbenv;
	u_int32_t count;
{
	ENV *env;

	env = dbenv->env;

	ENV_ILLEGAL_AFTER_OPEN(env, "DB_ENV->set_thread_count");
	dbenv->thr_max = count;

	return (0);
}

/*
 * __env_get_msgcall --
 *	{DB_ENV,DB}->get_msgcall.
 *
 * PUBLIC: void __env_get_msgcall
 * PUBLIC:     __P((DB_ENV *, void (**)(const DB_ENV *, const char *)));
 */
void
__env_get_msgcall(dbenv, msgcallp)
	DB_ENV *dbenv;
	void (**msgcallp) __P((const DB_ENV *, const char *));
{
	if (msgcallp != NULL)
		*msgcallp = dbenv->db_msgcall;
}

/*
 * __env_set_msgcall --
 *	{DB_ENV,DB}->set_msgcall.
 *
 * PUBLIC: void __env_set_msgcall
 * PUBLIC:     __P((DB_ENV *, void (*)(const DB_ENV *, const char *)));
 */
void
__env_set_msgcall(dbenv, msgcall)
	DB_ENV *dbenv;
	void (*msgcall) __P((const DB_ENV *, const char *));
{
	dbenv->db_msgcall = msgcall;
}

/*
 * __env_get_msgfile --
 *	{DB_ENV,DB}->get_msgfile.
 *
 * PUBLIC: void __env_get_msgfile __P((DB_ENV *, FILE **));
 */
void
__env_get_msgfile(dbenv, msgfilep)
	DB_ENV *dbenv;
	FILE **msgfilep;
{
	*msgfilep = dbenv->db_msgfile;
}

/*
 * __env_set_msgfile --
 *	{DB_ENV,DB}->set_msgfile.
 *
 * PUBLIC: void __env_set_msgfile __P((DB_ENV *, FILE *));
 */
void
__env_set_msgfile(dbenv, msgfile)
	DB_ENV *dbenv;
	FILE *msgfile;
{
	dbenv->db_msgfile = msgfile;
}

/*
 * __env_set_paniccall --
 *	{DB_ENV,DB}->set_paniccall.
 *
 * PUBLIC: int  __env_set_paniccall __P((DB_ENV *, void (*)(DB_ENV *, int)));
 */
int
__env_set_paniccall(dbenv, paniccall)
	DB_ENV *dbenv;
	void (*paniccall) __P((DB_ENV *, int));
{
	dbenv->db_paniccall = paniccall;
	return (0);
}

/*
 * __env_set_event_notify --
 *	DB_ENV->set_event_notify.
 */
static int
__env_set_event_notify(dbenv, event_func)
	DB_ENV *dbenv;
	void (*event_func) __P((DB_ENV *, u_int32_t, void *));
{
	dbenv->db_event_func = event_func;
	return (0);
}

static int
__env_get_shm_key(dbenv, shm_keyp)
	DB_ENV *dbenv;
	long *shm_keyp;			/* !!!: really a key_t *. */
{
	*shm_keyp = dbenv->shm_key;
	return (0);
}

/*
 * __env_set_shm_key --
 *	DB_ENV->set_shm_key.
 *
 * PUBLIC: int  __env_set_shm_key __P((DB_ENV *, long));
 */
int
__env_set_shm_key(dbenv, shm_key)
	DB_ENV *dbenv;
	long shm_key;			/* !!!: really a key_t. */
{
	ENV *env;

	env = dbenv->env;

	ENV_ILLEGAL_AFTER_OPEN(env, "DB_ENV->set_shm_key");

	dbenv->shm_key = shm_key;
	return (0);
}

static int
__env_get_tmp_dir(dbenv, dirp)
	DB_ENV *dbenv;
	const char **dirp;
{
	*dirp = dbenv->db_tmp_dir;
	return (0);
}

/*
 * __env_set_tmp_dir --
 *	DB_ENV->set_tmp_dir.
 *
 * PUBLIC: int  __env_set_tmp_dir __P((DB_ENV *, const char *));
 */
int
__env_set_tmp_dir(dbenv, dir)
	DB_ENV *dbenv;
	const char *dir;
{
	ENV *env;

	env = dbenv->env;

	if (dbenv->db_tmp_dir != NULL)
		__os_free(env, dbenv->db_tmp_dir);
	return (__os_strdup(env, dir, &dbenv->db_tmp_dir));
}

static int
__env_get_verbose(dbenv, which, onoffp)
	DB_ENV *dbenv;
	u_int32_t which;
	int *onoffp;
{
	switch (which) {
	case DB_VERB_DEADLOCK:
	case DB_VERB_FILEOPS:
	case DB_VERB_FILEOPS_ALL:
	case DB_VERB_RECOVERY:
	case DB_VERB_REGISTER:
	case DB_VERB_REPLICATION:
	case DB_VERB_REP_ELECT:
	case DB_VERB_REP_LEASE:
	case DB_VERB_REP_MISC:
	case DB_VERB_REP_MSGS:
	case DB_VERB_REP_SYNC:
	case DB_VERB_REP_TEST:
	case DB_VERB_REPMGR_CONNFAIL:
	case DB_VERB_REPMGR_MISC:
	case DB_VERB_WAITSFOR:
		*onoffp = FLD_ISSET(dbenv->verbose, which) ? 1 : 0;
		break;
	default:
		return (EINVAL);
	}
	return (0);
}

/*
 * __env_set_verbose --
 *	DB_ENV->set_verbose.
 *
 * PUBLIC: int  __env_set_verbose __P((DB_ENV *, u_int32_t, int));
 */
int
__env_set_verbose(dbenv, which, on)
	DB_ENV *dbenv;
	u_int32_t which;
	int on;
{
	switch (which) {
	case DB_VERB_DEADLOCK:
	case DB_VERB_FILEOPS:
	case DB_VERB_FILEOPS_ALL:
	case DB_VERB_RECOVERY:
	case DB_VERB_REGISTER:
	case DB_VERB_REPLICATION:
	case DB_VERB_REP_ELECT:
	case DB_VERB_REP_LEASE:
	case DB_VERB_REP_MISC:
	case DB_VERB_REP_MSGS:
	case DB_VERB_REP_SYNC:
	case DB_VERB_REP_TEST:
	case DB_VERB_REPMGR_CONNFAIL:
	case DB_VERB_REPMGR_MISC:
	case DB_VERB_WAITSFOR:
		if (on)
			FLD_SET(dbenv->verbose, which);
		else
			FLD_CLR(dbenv->verbose, which);
		break;
	default:
		return (EINVAL);
	}
	return (0);
}

/*
 * __db_mi_env --
 *	Method illegally called with public environment.
 *
 * PUBLIC: int __db_mi_env __P((ENV *, const char *));
 */
int
__db_mi_env(env, name)
	ENV *env;
	const char *name;
{
	__db_errx(env,
	    "%s: method not permitted when environment specified", name);
	return (EINVAL);
}

/*
 * __db_mi_open --
 *	Method illegally called after open.
 *
 * PUBLIC: int __db_mi_open __P((ENV *, const char *, int));
 */
int
__db_mi_open(env, name, after)
	ENV *env;
	const char *name;
	int after;
{
	__db_errx(env, "%s: method not permitted %s handle's open method",
	    name, after ? "after" : "before");
	return (EINVAL);
}

/*
 * __env_not_config --
 *	Method or function called without required configuration.
 *
 * PUBLIC: int __env_not_config __P((ENV *, char *, u_int32_t));
 */
int
__env_not_config(env, i, flags)
	ENV *env;
	char *i;
	u_int32_t flags;
{
	char *sub;

	switch (flags) {
	case DB_INIT_LOCK:
		sub = "locking";
		break;
	case DB_INIT_LOG:
		sub = "logging";
		break;
	case DB_INIT_MPOOL:
		sub = "memory pool";
		break;
	case DB_INIT_REP:
		sub = "replication";
		break;
	case DB_INIT_TXN:
		sub = "transaction";
		break;
	default:
		sub = "<unspecified>";
		break;
	}
	__db_errx(env,
    "%s interface requires an environment configured for the %s subsystem",
	    i, sub);
	return (EINVAL);
}

static int
__env_set_rpc_server(dbenv, cl, host, tsec, ssec, flags)
	DB_ENV *dbenv;
	void *cl;
	const char *host;
	long tsec, ssec;
	u_int32_t flags;
{
	ENV *env;

	env = dbenv->env;

	COMPQUIET(host, NULL);
	COMPQUIET(cl, NULL);
	COMPQUIET(tsec, 0);
	COMPQUIET(ssec, 0);
	COMPQUIET(flags, 0);

	__db_errx(env, "Berkeley DB was not configured for RPC support");
	return (DB_OPNOTSUP);
}

/*
 * __env_get_timeout --
 *	DB_ENV->get_timeout
 */
static int
__env_get_timeout(dbenv, timeoutp, flags)
	DB_ENV *dbenv;
	db_timeout_t *timeoutp;
	u_int32_t flags;
{
	int ret;

	ret = 0;
	if (flags == DB_SET_REG_TIMEOUT) {
		*timeoutp = dbenv->envreg_timeout;
	} else
		ret = __lock_get_env_timeout(dbenv, timeoutp, flags);
	return (ret);
}

/*
 * __env_set_timeout --
 *	DB_ENV->set_timeout
 *
 * PUBLIC: int __env_set_timeout __P((DB_ENV *, db_timeout_t, u_int32_t));
 */
int
__env_set_timeout(dbenv, timeout, flags)
	DB_ENV *dbenv;
	db_timeout_t timeout;
	u_int32_t flags;
{
	int ret;

	ret = 0;
	if (flags == DB_SET_REG_TIMEOUT)
		dbenv->envreg_timeout = timeout;
	else
		ret = __lock_set_env_timeout(dbenv, timeout, flags);
	return (ret);
}
