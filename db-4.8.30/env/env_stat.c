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
#include "dbinc/db_am.h"
#include "dbinc/lock.h"
#include "dbinc/log.h"
#include "dbinc/mp.h"
#include "dbinc/txn.h"

#ifdef HAVE_STATISTICS
static int   __env_print_all __P((ENV *, u_int32_t));
static int   __env_print_dbenv_all __P((ENV *, u_int32_t));
static int   __env_print_env_all __P((ENV *, u_int32_t));
static int   __env_print_fh __P((ENV *));
static int   __env_print_stats __P((ENV *, u_int32_t));
static int   __env_print_thread __P((ENV *));
static int   __env_stat_print __P((ENV *, u_int32_t));
static char *__env_thread_state_print __P((DB_THREAD_STATE));
static const char *
	     __reg_type __P((reg_type_t));

/*
 * __env_stat_print_pp --
 *	ENV->stat_print pre/post processor.
 *
 * PUBLIC: int __env_stat_print_pp __P((DB_ENV *, u_int32_t));
 */
int
__env_stat_print_pp(dbenv, flags)
	DB_ENV *dbenv;
	u_int32_t flags;
{
	DB_THREAD_INFO *ip;
	ENV *env;
	int ret;

	env = dbenv->env;

	ENV_ILLEGAL_BEFORE_OPEN(env, "DB_ENV->stat_print");

	if ((ret = __db_fchk(env, "DB_ENV->stat_print",
	    flags, DB_STAT_ALL | DB_STAT_CLEAR | DB_STAT_SUBSYSTEM)) != 0)
		return (ret);

	ENV_ENTER(env, ip);
	REPLICATION_WRAP(env, (__env_stat_print(env, flags)), 0, ret);
	ENV_LEAVE(env, ip);
	return (ret);
}

/*
 * __env_stat_print --
 *	ENV->stat_print method.
 */
static int
__env_stat_print(env, flags)
	ENV *env;
	u_int32_t flags;
{
	time_t now;
	int ret;
	char time_buf[CTIME_BUFLEN];

	(void)time(&now);
	__db_msg(env, "%.24s\tLocal time", __os_ctime(&now, time_buf));

	if ((ret = __env_print_stats(env, flags)) != 0)
		return (ret);

	if (LF_ISSET(DB_STAT_ALL) &&
	    (ret = __env_print_all(env, flags)) != 0)
		return (ret);

	if ((ret = __env_print_thread(env)) != 0)
		return (ret);

	if ((ret = __env_print_fh(env)) != 0)
		return (ret);

	if (!LF_ISSET(DB_STAT_SUBSYSTEM))
		return (0);

	if (LOGGING_ON(env)) {
		__db_msg(env, "%s", DB_GLOBAL(db_line));
		if ((ret = __log_stat_print(env, flags)) != 0)
			return (ret);

		__db_msg(env, "%s", DB_GLOBAL(db_line));
		if ((ret = __dbreg_stat_print(env, flags)) != 0)
			return (ret);
	}

	if (LOCKING_ON(env)) {
		__db_msg(env, "%s", DB_GLOBAL(db_line));
		if ((ret = __lock_stat_print(env, flags)) != 0)
			return (ret);
	}

	if (MPOOL_ON(env)) {
		__db_msg(env, "%s", DB_GLOBAL(db_line));
		if ((ret = __memp_stat_print(env, flags)) != 0)
			return (ret);
	}

	if (REP_ON(env)) {
		__db_msg(env, "%s", DB_GLOBAL(db_line));
		if ((ret = __rep_stat_print(env, flags)) != 0)
			return (ret);
	}

	if (TXN_ON(env)) {
		__db_msg(env, "%s", DB_GLOBAL(db_line));
		if ((ret = __txn_stat_print(env, flags)) != 0)
			return (ret);
	}

#ifdef HAVE_MUTEX_SUPPORT
	/*
	 * Dump the mutexes last.  If DB_STAT_CLEAR is set this will
	 * clear out the mutex counters and we want to see them in
	 * the context of the other subsystems first.
	 */
	if (MUTEX_ON(env)) {
		__db_msg(env, "%s", DB_GLOBAL(db_line));
		if ((ret = __mutex_stat_print(env, flags)) != 0)
			return (ret);
	}
#endif

	return (0);
}

/*
 * __env_print_stats --
 *	Display the default environment statistics.
 *
 */
static int
__env_print_stats(env, flags)
	ENV *env;
	u_int32_t flags;
{
	REGENV *renv;
	REGINFO *infop;
	char time_buf[CTIME_BUFLEN];

	infop = env->reginfo;
	renv = infop->primary;

	if (LF_ISSET(DB_STAT_ALL)) {
		__db_msg(env, "%s", DB_GLOBAL(db_line));
		__db_msg(env, "Default database environment information:");
	}
	STAT_HEX("Magic number", renv->magic);
	STAT_LONG("Panic value", renv->panic);
	__db_msg(env, "%d.%d.%d\tEnvironment version",
	    renv->majver, renv->minver, renv->patchver);
	STAT_LONG("Btree version", DB_BTREEVERSION);
	STAT_LONG("Hash version", DB_HASHVERSION);
	STAT_LONG("Lock version", DB_LOCKVERSION);
	STAT_LONG("Log version", DB_LOGVERSION);
	STAT_LONG("Queue version", DB_QAMVERSION);
	STAT_LONG("Sequence version", DB_SEQUENCE_VERSION);
	STAT_LONG("Txn version", DB_TXNVERSION);
	__db_msg(env,
	    "%.24s\tCreation time", __os_ctime(&renv->timestamp, time_buf));
	STAT_HEX("Environment ID", renv->envid);
	__mutex_print_debug_single(env,
	    "Primary region allocation and reference count mutex",
	    renv->mtx_regenv, flags);
	STAT_LONG("References", renv->refcnt);

	return (0);
}

/*
 * __env_print_all --
 *	Display the debugging environment statistics.
 */
static int
__env_print_all(env, flags)
	ENV *env;
	u_int32_t flags;
{
	int ret, t_ret;

	/*
	 * There are two structures -- DB_ENV and ENV.
	 */
	ret = __env_print_dbenv_all(env, flags);
	if ((t_ret = __env_print_env_all(env, flags)) != 0 && ret == 0)
		ret = t_ret;

	return (ret);
}

/*
 * __env_print_dbenv_all --
 *	Display the debugging environment statistics.
 */
static int
__env_print_dbenv_all(env, flags)
	ENV *env;
	u_int32_t flags;
{
	static const FN db_env_fn[] = {
		{ DB_ENV_AUTO_COMMIT,		"DB_ENV_AUTO_COMMIT" },
		{ DB_ENV_CDB_ALLDB,		"DB_ENV_CDB_ALLDB" },
		{ DB_ENV_DIRECT_DB,		"DB_ENV_DIRECT_DB" },
		{ DB_ENV_DSYNC_DB,		"DB_ENV_DSYNC_DB" },
		{ DB_ENV_MULTIVERSION,		"DB_ENV_MULTIVERSION" },
		{ DB_ENV_NOLOCKING,		"DB_ENV_NOLOCKING" },
		{ DB_ENV_NOMMAP,		"DB_ENV_NOMMAP" },
		{ DB_ENV_NOPANIC,		"DB_ENV_NOPANIC" },
		{ DB_ENV_OVERWRITE,		"DB_ENV_OVERWRITE" },
		{ DB_ENV_REGION_INIT,		"DB_ENV_REGION_INIT" },
		{ DB_ENV_RPCCLIENT,		"DB_ENV_RPCCLIENT" },
		{ DB_ENV_RPCCLIENT_GIVEN,	"DB_ENV_RPCCLIENT_GIVEN" },
		{ DB_ENV_TIME_NOTGRANTED,	"DB_ENV_TIME_NOTGRANTED" },
		{ DB_ENV_TXN_NOSYNC,		"DB_ENV_TXN_NOSYNC" },
		{ DB_ENV_TXN_NOWAIT,		"DB_ENV_TXN_NOWAIT" },
		{ DB_ENV_TXN_SNAPSHOT,		"DB_ENV_TXN_SNAPSHOT" },
		{ DB_ENV_TXN_WRITE_NOSYNC,	"DB_ENV_TXN_WRITE_NOSYNC" },
		{ DB_ENV_YIELDCPU,		"DB_ENV_YIELDCPU" },
		{ 0,				NULL }
	};
	static const FN vfn[] = {
		{ DB_VERB_DEADLOCK,		"DB_VERB_DEADLOCK" },
		{ DB_VERB_FILEOPS,		"DB_VERB_FILEOPS" },
		{ DB_VERB_FILEOPS_ALL,		"DB_VERB_FILEOPS_ALL" },
		{ DB_VERB_RECOVERY,		"DB_VERB_RECOVERY" },
		{ DB_VERB_REGISTER,		"DB_VERB_REGISTER" },
		{ DB_VERB_REPLICATION,		"DB_VERB_REPLICATION" },
		{ DB_VERB_REP_ELECT,		"DB_VERB_REP_ELECT" },
		{ DB_VERB_REP_LEASE,		"DB_VERB_REP_LEASE" },
		{ DB_VERB_REP_MISC,		"DB_VERB_REP_MISC" },
		{ DB_VERB_REP_MSGS,		"DB_VERB_REP_MSGS" },
		{ DB_VERB_REP_SYNC,		"DB_VERB_REP_SYNC" },
		{ DB_VERB_REP_TEST,		"DB_VERB_REP_TEST" },
		{ DB_VERB_REPMGR_CONNFAIL,	"DB_VERB_REPMGR_CONNFAIL" },
		{ DB_VERB_REPMGR_MISC,		"DB_VERB_REPMGR_MISC" },
		{ DB_VERB_WAITSFOR,		"DB_VERB_WAITSFOR" },
		{ 0,				NULL }
	};
	DB_ENV *dbenv;
	DB_MSGBUF mb;
	char **p;

	dbenv = env->dbenv;
	DB_MSGBUF_INIT(&mb);

	__db_msg(env, "%s", DB_GLOBAL(db_line));
	STAT_POINTER("ENV", dbenv->env);
	__mutex_print_debug_single(
	    env, "DB_ENV handle mutex", dbenv->mtx_db_env, flags);
	STAT_ISSET("Errcall", dbenv->db_errcall);
	STAT_ISSET("Errfile", dbenv->db_errfile);
	STAT_STRING("Errpfx", dbenv->db_errpfx);
	STAT_ISSET("Msgfile", dbenv->db_msgfile);
	STAT_ISSET("Msgcall", dbenv->db_msgcall);

	STAT_ISSET("AppDispatch", dbenv->app_dispatch);
	STAT_ISSET("Event", dbenv->db_event_func);
	STAT_ISSET("Feedback", dbenv->db_feedback);
	STAT_ISSET("Free", dbenv->db_free);
	STAT_ISSET("Panic", dbenv->db_paniccall);
	STAT_ISSET("Malloc", dbenv->db_malloc);
	STAT_ISSET("Realloc", dbenv->db_realloc);
	STAT_ISSET("IsAlive", dbenv->is_alive);
	STAT_ISSET("ThreadId", dbenv->thread_id);
	STAT_ISSET("ThreadIdString", dbenv->thread_id_string);

	STAT_STRING("Log dir", dbenv->db_log_dir);
	STAT_STRING("Tmp dir", dbenv->db_tmp_dir);
	if (dbenv->db_data_dir == NULL)
		STAT_ISSET("Data dir", dbenv->db_data_dir);
	else {
		for (p = dbenv->db_data_dir; *p != NULL; ++p)
			__db_msgadd(env, &mb, "%s\tData dir", *p);
		DB_MSGBUF_FLUSH(env, &mb);
	}

	STAT_STRING(
	    "Intermediate directory mode", dbenv->intermediate_dir_mode);

	STAT_LONG("Shared memory key", dbenv->shm_key);

	STAT_ISSET("Password", dbenv->passwd);

	STAT_ISSET("RPC client", dbenv->cl_handle);
	STAT_ULONG("RPC client ID", dbenv->cl_id);

	STAT_ISSET("App private", dbenv->app_private);
	STAT_ISSET("Api1 internal", dbenv->api1_internal);
	STAT_ISSET("Api2 internal", dbenv->api2_internal);

	__db_prflags(env, NULL, dbenv->verbose, vfn, NULL, "\tVerbose flags");

	STAT_ULONG("Mutex align", dbenv->mutex_align);
	STAT_ULONG("Mutex cnt", dbenv->mutex_cnt);
	STAT_ULONG("Mutex inc", dbenv->mutex_inc);
	STAT_ULONG("Mutex tas spins", dbenv->mutex_tas_spins);

	STAT_ISSET("Lock conflicts", dbenv->lk_conflicts);
	STAT_LONG("Lock modes", dbenv->lk_modes);
	STAT_ULONG("Lock detect", dbenv->lk_detect);
	STAT_ULONG("Lock max", dbenv->lk_max);
	STAT_ULONG("Lock max lockers", dbenv->lk_max_lockers);
	STAT_ULONG("Lock max objects", dbenv->lk_max_objects);
	STAT_ULONG("Lock partitions", dbenv->lk_partitions);
	STAT_ULONG("Lock timeout", dbenv->lk_timeout);

	STAT_ULONG("Log bsize", dbenv->lg_bsize);
	STAT_FMT("Log file mode", "%#o", int, dbenv->lg_filemode);
	STAT_ULONG("Log region max", dbenv->lg_regionmax);
	STAT_ULONG("Log size", dbenv->lg_size);

	STAT_ULONG("Cache GB", dbenv->mp_gbytes);
	STAT_ULONG("Cache B", dbenv->mp_bytes);
	STAT_ULONG("Cache max GB", dbenv->mp_max_gbytes);
	STAT_ULONG("Cache max B", dbenv->mp_max_bytes);
	STAT_ULONG("Cache mmap size", dbenv->mp_mmapsize);
	STAT_ULONG("Cache max open fd", dbenv->mp_maxopenfd);
	STAT_ULONG("Cache max write", dbenv->mp_maxwrite);
	STAT_ULONG("Cache number", dbenv->mp_ncache);
	STAT_ULONG("Cache max write sleep", dbenv->mp_maxwrite_sleep);

	STAT_ULONG("Txn max", dbenv->tx_max);
	STAT_ULONG("Txn timestamp", dbenv->tx_timestamp);
	STAT_ULONG("Txn timeout", dbenv->tx_timeout);

	STAT_ULONG("Thread count", dbenv->thr_max);

	STAT_ISSET("Registry", dbenv->registry);
	STAT_ULONG("Registry offset", dbenv->registry_off);

	__db_prflags(env,
	    NULL, dbenv->flags, db_env_fn, NULL, "\tPublic environment flags");

	return (0);
}

/*
 * __env_print_env_all --
 *	Display the debugging environment statistics.
 */
static int
__env_print_env_all(env, flags)
	ENV *env;
	u_int32_t flags;
{
	static const FN env_fn[] = {
		{ ENV_CDB,			"ENV_CDB" },
		{ ENV_DBLOCAL,			"ENV_DBLOCAL" },
		{ ENV_LOCKDOWN,			"ENV_LOCKDOWN" },
		{ ENV_NO_OUTPUT_SET,		"ENV_NO_OUTPUT_SET" },
		{ ENV_OPEN_CALLED,		"ENV_OPEN_CALLED" },
		{ ENV_PRIVATE,			"ENV_PRIVATE" },
		{ ENV_RECOVER_FATAL,		"ENV_RECOVER_FATAL" },
		{ ENV_REF_COUNTED,		"ENV_REF_COUNTED" },
		{ ENV_SYSTEM_MEM,		"ENV_SYSTEM_MEM" },
		{ ENV_THREAD,			"ENV_THREAD" },
		{ 0,				NULL }
	};
	static const FN ofn[] = {
		{ DB_CREATE,			"DB_CREATE" },
		{ DB_FORCE,			"DB_FORCE" },
		{ DB_INIT_CDB,			"DB_INIT_CDB" },
		{ DB_INIT_LOCK,			"DB_INIT_LOCK" },
		{ DB_INIT_LOG,			"DB_INIT_LOG" },
		{ DB_INIT_MPOOL,		"DB_INIT_MPOOL" },
		{ DB_INIT_REP,			"DB_INIT_REP" },
		{ DB_INIT_TXN,			"DB_INIT_TXN" },
		{ DB_LOCKDOWN,			"DB_LOCKDOWN" },
		{ DB_NOMMAP,			"DB_NOMMAP" },
		{ DB_PRIVATE,			"DB_PRIVATE" },
		{ DB_RDONLY,			"DB_RDONLY" },
		{ DB_RECOVER,			"DB_RECOVER" },
		{ DB_RECOVER_FATAL,		"DB_RECOVER_FATAL" },
		{ DB_SYSTEM_MEM,		"DB_SYSTEM_MEM" },
		{ DB_THREAD,			"DB_THREAD" },
		{ DB_TRUNCATE,			"DB_TRUNCATE" },
		{ DB_TXN_NOSYNC,		"DB_TXN_NOSYNC" },
		{ DB_USE_ENVIRON,		"DB_USE_ENVIRON" },
		{ DB_USE_ENVIRON_ROOT,		"DB_USE_ENVIRON_ROOT" },
		{ 0,				NULL }
	};
	static const FN regenvfn[] = {
		{ DB_REGENV_REPLOCKED,		"DB_REGENV_REPLOCKED" },
		{ 0,				NULL }
	};
	REGENV *renv;
	REGINFO *infop;
	REGION *rp;
	u_int32_t i;
	char time_buf[CTIME_BUFLEN];

	infop = env->reginfo;
	renv = infop->primary;

	__db_msg(env, "%s", DB_GLOBAL(db_line));
	STAT_POINTER("DB_ENV", env->dbenv);
	__mutex_print_debug_single(
	    env, "ENV handle mutex", env->mtx_env, flags);

	STAT_STRING("Home", env->db_home);
	__db_prflags(env, NULL, env->open_flags, ofn, NULL, "\tOpen flags");
	STAT_FMT("Mode", "%#o", int, env->db_mode);

	STAT_ULONG("Pid cache", env->pid_cache);

	STAT_ISSET("Lockfhp", env->lockfhp);

	STAT_ISSET("Locker", env->env_lref);

	STAT_ISSET("Internal recovery table", env->recover_dtab.int_dispatch);
	STAT_ULONG("Number of recovery table slots",
	    env->recover_dtab.int_size);
	STAT_ISSET("External recovery table", env->recover_dtab.ext_dispatch);
	STAT_ULONG("Number of recovery table slots",
	    env->recover_dtab.ext_size);

	STAT_ULONG("Thread hash buckets", env->thr_nbucket);
	STAT_ISSET("Thread hash table", env->thr_hashtab);

	STAT_ULONG("Mutex initial count", env->mutex_iq_next);
	STAT_ULONG("Mutex initial max", env->mutex_iq_max);

	__mutex_print_debug_single(
	    env, "ENV list of DB handles mutex", env->mtx_dblist, flags);
	STAT_LONG("DB reference count", env->db_ref);

	__mutex_print_debug_single(env, "MT mutex", env->mtx_mt, flags);

	STAT_ISSET("Crypto handle", env->crypto_handle);
	STAT_ISSET("Lock handle", env->lk_handle);
	STAT_ISSET("Log handle", env->lg_handle);
	STAT_ISSET("Cache handle", env->mp_handle);
	STAT_ISSET("Mutex handle", env->mutex_handle);
	STAT_ISSET("Replication handle", env->rep_handle);
	STAT_ISSET("Txn handle", env->tx_handle);

	STAT_ISSET("User copy", env->dbt_usercopy);

	STAT_LONG("Test abort", env->test_abort);
	STAT_LONG("Test check", env->test_check);
	STAT_LONG("Test copy", env->test_copy);

	__db_prflags(env,
	    NULL, env->flags, env_fn, NULL, "\tPrivate environment flags");

	__db_print_reginfo(env, infop, "Primary", flags);
	__db_msg(env, "%s", DB_GLOBAL(db_line));
	__db_msg(env, "Per region database environment information:");
	for (rp = R_ADDR(infop, renv->region_off),
	    i = 0; i < renv->region_cnt; ++i, ++rp) {
		if (rp->id == INVALID_REGION_ID)
			continue;
		__db_msg(env, "%s Region:", __reg_type(rp->type));
		STAT_LONG("Region ID", rp->id);
		STAT_LONG("Segment ID", rp->segid);
		__db_dlbytes(env,
		    "Size", (u_long)0, (u_long)0, (u_long)rp->size);
	}
	__db_prflags(env,
	    NULL, renv->init_flags, ofn, NULL, "\tInitialization flags");
	STAT_ULONG("Region slots", renv->region_cnt);
	__db_prflags(env,
	    NULL, renv->flags, regenvfn, NULL, "\tReplication flags");
	__db_msg(env, "%.24s\tOperation timestamp",
	    renv->op_timestamp == 0 ?
	    "!Set" : __os_ctime(&renv->op_timestamp, time_buf));
	__db_msg(env, "%.24s\tReplication timestamp",
	    renv->rep_timestamp == 0 ?
	    "!Set" : __os_ctime(&renv->rep_timestamp, time_buf));

	return (0);
}

static char *
__env_thread_state_print(state)
	DB_THREAD_STATE state;
{
	switch (state) {
	case THREAD_ACTIVE:
		return ("active");
	case THREAD_BLOCKED:
		return ("blocked");
	case THREAD_BLOCKED_DEAD:
		return ("blocked and dead");
	case THREAD_OUT:
		return ("out");
	default:
		return ("unknown");
	}
	/* NOTREACHED */
}

/*
 * __env_print_thread --
 *	Display the thread block state.
 */
static int
__env_print_thread(env)
	ENV *env;
{
	BH *bhp;
	DB_ENV *dbenv;
	DB_HASHTAB *htab;
	DB_MPOOL *dbmp;
	DB_THREAD_INFO *ip;
	PIN_LIST *list, *lp;
	REGENV *renv;
	REGINFO *infop;
	THREAD_INFO *thread;
	u_int32_t i;
	char buf[DB_THREADID_STRLEN];

	dbenv = env->dbenv;

	/* The thread table may not be configured. */
	if ((htab = env->thr_hashtab) == NULL)
		return (0);

	dbmp = env->mp_handle;
	__db_msg(env, "%s", DB_GLOBAL(db_line));
	__db_msg(env, "Thread tracking information");

	/* Dump out the info we have on thread tracking. */
	infop = env->reginfo;
	renv = infop->primary;
	thread = R_ADDR(infop, renv->thread_off);
	STAT_ULONG("Thread blocks allocated", thread->thr_count);
	STAT_ULONG("Thread allocation threshold", thread->thr_max);
	STAT_ULONG("Thread hash buckets", thread->thr_nbucket);

	/* Dump out the info we have on active threads. */
	__db_msg(env, "Thread status blocks:");
	for (i = 0; i < env->thr_nbucket; i++)
		SH_TAILQ_FOREACH(ip, &htab[i], dbth_links, __db_thread_info) {
			if (ip->dbth_state == THREAD_SLOT_NOT_IN_USE)
				continue;
			__db_msg(env, "\tprocess/thread %s: %s",
			    dbenv->thread_id_string(
			    dbenv, ip->dbth_pid, ip->dbth_tid, buf),
			    __env_thread_state_print(ip->dbth_state));
			list = R_ADDR(env->reginfo, ip->dbth_pinlist);
			for (lp = list; lp < &list[ip->dbth_pinmax]; lp++) {
				if (lp->b_ref == INVALID_ROFF)
					continue;
				bhp = R_ADDR(
				    &dbmp->reginfo[lp->region], lp->b_ref);
				__db_msg(env,
				     "\t\tpins: %lu", (u_long)bhp->pgno);
			}
		}
	return (0);
}

/*
 * __env_print_fh --
 *	Display statistics for all handles open in this environment.
 */
static int
__env_print_fh(env)
	ENV *env;
{
	DB_FH *fhp;

	if (TAILQ_FIRST(&env->fdlist) == NULL)
		return (0);

	__db_msg(env, "%s", DB_GLOBAL(db_line));
	__db_msg(env, "Environment file handle information");

	MUTEX_LOCK(env, env->mtx_env);

	TAILQ_FOREACH(fhp, &env->fdlist, q)
		__db_print_fh(env, NULL, fhp, 0);

	MUTEX_UNLOCK(env, env->mtx_env);

	return (0);
}

/*
 * __db_print_fh --
 *	Print out a file handle.
 *
 * PUBLIC: void __db_print_fh __P((ENV *, const char *, DB_FH *, u_int32_t));
 */
void
__db_print_fh(env, tag, fh, flags)
	ENV *env;
	const char *tag;
	DB_FH *fh;
	u_int32_t flags;
{
	static const FN fn[] = {
		{ DB_FH_NOSYNC,	"DB_FH_NOSYNC" },
		{ DB_FH_OPENED,	"DB_FH_OPENED" },
		{ DB_FH_UNLINK,	"DB_FH_UNLINK" },
		{ 0,		NULL }
	};

	if (fh == NULL) {
		STAT_ISSET(tag, fh);
		return;
	}

	STAT_STRING("file-handle.file name", fh->name);

	__mutex_print_debug_single(
	    env, "file-handle.mutex", fh->mtx_fh, flags);

	STAT_LONG("file-handle.reference count", fh->ref);
	STAT_LONG("file-handle.file descriptor", fh->fd);

	STAT_ULONG("file-handle.page number", fh->pgno);
	STAT_ULONG("file-handle.page size", fh->pgsize);
	STAT_ULONG("file-handle.page offset", fh->offset);

	STAT_ULONG("file-handle.seek count", fh->seek_count);
	STAT_ULONG("file-handle.read count", fh->read_count);
	STAT_ULONG("file-handle.write count", fh->write_count);

	__db_prflags(env, NULL, fh->flags, fn, NULL, "\tfile-handle.flags");
}

/*
 * __db_print_fileid --
 *	Print out a file ID.
 *
 * PUBLIC: void __db_print_fileid __P((ENV *, u_int8_t *, const char *));
 */
void
__db_print_fileid(env, id, suffix)
	ENV *env;
	u_int8_t *id;
	const char *suffix;
{
	DB_MSGBUF mb;
	int i;

	if (id == NULL) {
		STAT_ISSET("ID", id);
		return;
	}

	DB_MSGBUF_INIT(&mb);
	for (i = 0; i < DB_FILE_ID_LEN; ++i, ++id) {
		__db_msgadd(env, &mb, "%x", (u_int)*id);
		if (i < DB_FILE_ID_LEN - 1)
			__db_msgadd(env, &mb, " ");
	}
	if (suffix != NULL)
		__db_msgadd(env, &mb, "%s", suffix);
	DB_MSGBUF_FLUSH(env, &mb);
}

/*
 * __db_dl --
 *	Display a big value.
 *
 * PUBLIC: void __db_dl __P((ENV *, const char *, u_long));
 */
void
__db_dl(env, msg, value)
	ENV *env;
	const char *msg;
	u_long value;
{
	/*
	 * Two formats: if less than 10 million, display as the number, if
	 * greater than 10 million display as ###M.
	 */
	if (value < 10000000)
		__db_msg(env, "%lu\t%s", value, msg);
	else
		__db_msg(env, "%luM\t%s (%lu)", value / 1000000, msg, value);
}

/*
 * __db_dl_pct --
 *	Display a big value, and related percentage.
 *
 * PUBLIC: void __db_dl_pct
 * PUBLIC:          __P((ENV *, const char *, u_long, int, const char *));
 */
void
__db_dl_pct(env, msg, value, pct, tag)
	ENV *env;
	const char *msg, *tag;
	u_long value;
	int pct;
{
	DB_MSGBUF mb;

	DB_MSGBUF_INIT(&mb);

	/*
	 * Two formats: if less than 10 million, display as the number, if
	 * greater than 10 million, round it off and display as ###M.
	 */
	if (value < 10000000)
		__db_msgadd(env, &mb, "%lu\t%s", value, msg);
	else
		__db_msgadd(env,
		    &mb, "%luM\t%s", (value + 500000) / 1000000, msg);
	if (tag == NULL)
		__db_msgadd(env, &mb, " (%d%%)", pct);
	else
		__db_msgadd(env, &mb, " (%d%% %s)", pct, tag);

	DB_MSGBUF_FLUSH(env, &mb);
}

/*
 * __db_dlbytes --
 *	Display a big number of bytes.
 *
 * PUBLIC: void __db_dlbytes
 * PUBLIC:     __P((ENV *, const char *, u_long, u_long, u_long));
 */
void
__db_dlbytes(env, msg, gbytes, mbytes, bytes)
	ENV *env;
	const char *msg;
	u_long gbytes, mbytes, bytes;
{
	DB_MSGBUF mb;
	const char *sep;

	DB_MSGBUF_INIT(&mb);

	/* Normalize the values. */
	while (bytes >= MEGABYTE) {
		++mbytes;
		bytes -= MEGABYTE;
	}
	while (mbytes >= GIGABYTE / MEGABYTE) {
		++gbytes;
		mbytes -= GIGABYTE / MEGABYTE;
	}

	if (gbytes == 0 && mbytes == 0 && bytes == 0)
		__db_msgadd(env, &mb, "0");
	else {
		sep = "";
		if (gbytes > 0) {
			__db_msgadd(env, &mb, "%luGB", gbytes);
			sep = " ";
		}
		if (mbytes > 0) {
			__db_msgadd(env, &mb, "%s%luMB", sep, mbytes);
			sep = " ";
		}
		if (bytes >= 1024) {
			__db_msgadd(env, &mb, "%s%luKB", sep, bytes / 1024);
			bytes %= 1024;
			sep = " ";
		}
		if (bytes > 0)
			__db_msgadd(env, &mb, "%s%luB", sep, bytes);
	}

	__db_msgadd(env, &mb, "\t%s", msg);

	DB_MSGBUF_FLUSH(env, &mb);
}

/*
 * __db_print_reginfo --
 *	Print out underlying shared region information.
 *
 * PUBLIC: void __db_print_reginfo
 * PUBLIC:     __P((ENV *, REGINFO *, const char *, u_int32_t));
 */
void
__db_print_reginfo(env, infop, s, flags)
	ENV *env;
	REGINFO *infop;
	const char *s;
	u_int32_t flags;
{
	static const FN fn[] = {
		{ REGION_CREATE,	"REGION_CREATE" },
		{ REGION_CREATE_OK,	"REGION_CREATE_OK" },
		{ REGION_JOIN_OK,	"REGION_JOIN_OK" },
		{ 0,			NULL }
	};

	__db_msg(env, "%s", DB_GLOBAL(db_line));
	__db_msg(env, "%s REGINFO information:",  s);
	STAT_STRING("Region type", __reg_type(infop->type));
	STAT_ULONG("Region ID", infop->id);
	STAT_STRING("Region name", infop->name);
	STAT_POINTER("Region address", infop->addr);
	STAT_POINTER("Region primary address", infop->primary);
	STAT_ULONG("Region maximum allocation", infop->max_alloc);
	STAT_ULONG("Region allocated", infop->allocated);
	__env_alloc_print(infop, flags);

	__db_prflags(env, NULL, infop->flags, fn, NULL, "\tRegion flags");
}

/*
 * __reg_type --
 *	Return the region type string.
 */
static const char *
__reg_type(t)
	reg_type_t t;
{
	switch (t) {
	case REGION_TYPE_ENV:
		return ("Environment");
	case REGION_TYPE_LOCK:
		return ("Lock");
	case REGION_TYPE_LOG:
		return ("Log");
	case REGION_TYPE_MPOOL:
		return ("Mpool");
	case REGION_TYPE_MUTEX:
		return ("Mutex");
	case REGION_TYPE_TXN:
		return ("Transaction");
	case INVALID_REGION_TYPE:
		return ("Invalid");
	}
	return ("Unknown");
}

#else /* !HAVE_STATISTICS */

/*
 * __db_stat_not_built --
 *	Common error routine when library not built with statistics.
 *
 * PUBLIC: int __db_stat_not_built __P((ENV *));
 */
int
__db_stat_not_built(env)
	ENV *env;
{
	__db_errx(env, "Library build did not include statistics support");
	return (DB_OPNOTSUP);
}

int
__env_stat_print_pp(dbenv, flags)
	DB_ENV *dbenv;
	u_int32_t flags;
{
	COMPQUIET(flags, 0);

	return (__db_stat_not_built(dbenv->env));
}
#endif
