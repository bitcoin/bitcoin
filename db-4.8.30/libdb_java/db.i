%{
#include "db_config.h"
#include "db_int.h"
#include "dbinc/txn.h"

#ifdef HAVE_CRYPTO
#define	CRYPTO_ONLY(x) (x);
#else
#define	CRYPTO_ONLY(x)
#endif
%}

#if defined(SWIGJAVA)
%include "db_java.i"
#elif defined(SWIGCSHARP)
%include "db_csharp.i"
#endif

typedef	unsigned char u_int8_t;
typedef	long int32_t;
typedef	long long db_seq_t;
typedef	long long pid_t;
#ifndef SWIGJAVA
typedef	long long db_threadid_t;
#endif
typedef	unsigned long u_int32_t;
typedef	u_int32_t	db_recno_t;	/* Record number type. */
typedef	u_int32_t	db_timeout_t;	/* Type of a timeout. */

typedef	int db_recops;
typedef	int db_lockmode_t;
typedef	int DBTYPE;
typedef	int DB_CACHE_PRIORITY;

/* Fake typedefs for SWIG */
typedef	int db_ret_t;    /* An int that is mapped to a void */
typedef	int int_bool;    /* An int that is mapped to a boolean */

%{
typedef int db_ret_t;
typedef int int_bool;

struct __db_lk_conflicts {
	u_int8_t *lk_conflicts;
	int lk_modes;
};

struct __db_out_stream {
	void *handle;
	int (*callback) __P((void *, const void *));
};

struct __db_repmgr_sites {
	DB_REPMGR_SITE *sites;
	u_int32_t nsites;
};

#define	Db __db
#define	Dbc __dbc
#define	Dbt __db_dbt
#define	DbEnv __db_env
#define	DbLock __db_lock_u
#define	DbLogc __db_log_cursor
#define	DbLsn __db_lsn
#define	DbMpoolFile __db_mpoolfile
#define	DbSequence __db_sequence
#define	DbTxn __db_txn

/* Suppress a compilation warning for an unused symbol */
void *unused = (void *)SWIG_JavaThrowException;
%}

struct Db;		typedef struct Db DB;
struct Dbc;		typedef struct Dbc DBC;
struct Dbt;	typedef struct Dbt DBT;
struct DbEnv;	typedef struct DbEnv DB_ENV;
struct DbLock;	typedef struct DbLock DB_LOCK;
struct DbLogc;	typedef struct DbLogc DB_LOGC;
struct DbLsn;	typedef struct DbLsn DB_LSN;
struct DbMpoolFile;	typedef struct DbMpoolFile DB_MPOOLFILE;
struct DbSequence;		typedef struct Db DB_SEQUENCE;
struct DbTxn;	typedef struct DbTxn DB_TXN;

/* Methods that allocate new objects */
%newobject Db::join(DBC **curslist, u_int32_t flags);
%newobject Db::dup(u_int32_t flags);
%newobject DbEnv::lock_get(u_int32_t locker,
	u_int32_t flags, DBT *object, db_lockmode_t lock_mode);
%newobject DbEnv::log_cursor(u_int32_t flags);

struct Db
{
%extend {
	JAVA_EXCEPT_ERRNO(DB_RETOK_STD, JDBENV)
	Db(DB_ENV *dbenv, u_int32_t flags) {
		DB *self = NULL;
		errno = db_create(&self, dbenv, flags);
		if (errno == 0 && dbenv == NULL)
			self->env->dbt_usercopy = __dbj_dbt_memcopy;
		return self;
	}

	JAVA_EXCEPT(DB_RETOK_STD, DB2JDBENV)
	db_ret_t associate(DB_TXN *txnid, DB *secondary,
	    int (*callback)(DB *, const DBT *, const DBT *, DBT *),
	    u_int32_t flags) {
		return self->associate(self, txnid, secondary, callback, flags);
	}

	db_ret_t associate_foreign(DB *primary, 
	    int (*callback)(DB *, const DBT *, DBT *, const DBT *, int *), u_int32_t flags) {
		return self->associate_foreign(self, primary, callback, flags);
	}

	db_ret_t compact(DB_TXN *txnid, DBT *start, DBT *stop,
	    DB_COMPACT *c_data, u_int32_t flags, DBT *end) {
		return self->compact(self, txnid, start, stop, c_data, flags,
		    end);
	}

	/*
	 * Should probably be db_ret_t, but maintaining backwards compatibility
	 * for now.
	 */
	JAVA_EXCEPT_ERRNO(DB_RETOK_STD, NULL)
	int close(u_int32_t flags) {
		errno = self->close(self, flags);
		return errno;
	}

	JAVA_EXCEPT_ERRNO(DB_RETOK_STD, DB2JDBENV)
	DBC *cursor(DB_TXN *txnid, u_int32_t flags) {
		DBC *cursorp = NULL;
		errno = self->cursor(self, txnid, &cursorp, flags);
		return cursorp;
	}

	JAVA_EXCEPT(DB_RETOK_DBDEL, DB2JDBENV)
	int del(DB_TXN *txnid, DBT *key, u_int32_t flags) {
		return self->del(self, txnid, key, flags);
	}

	JAVA_EXCEPT_NONE
	void err(int error, const char *message) {
		self->err(self, error, message);
	}

	void errx(const char *message) {
		self->errx(self, message);
	}

	JAVA_EXCEPT(DB_RETOK_EXISTS, DB2JDBENV)
	int exists(DB_TXN *txnid, DBT *key, u_int32_t flags) {
		return self->exists(self, txnid, key, flags);
	}

#ifndef SWIGJAVA
	int fd() {
		int ret = 0;
		errno = self->fd(self, &ret);
		return ret;
	}
#endif

	JAVA_EXCEPT(DB_RETOK_DBGET, DB2JDBENV)
	int get(DB_TXN *txnid, DBT *key, DBT *data, u_int32_t flags) {
		return self->get(self, txnid, key, data, flags);
	}

	JAVA_EXCEPT_ERRNO(DB_RETOK_STD, DB2JDBENV)
	int_bool get_byteswapped() {
		int ret = 0;
		errno = self->get_byteswapped(self, &ret);
		return ret;
	}

	jlong get_cachesize() {
		u_int32_t gbytes = 0, bytes = 0;
		errno = self->get_cachesize(self, &gbytes, &bytes, NULL);
		return (jlong)gbytes * GIGABYTE + bytes;
	}

	u_int32_t get_cachesize_ncache() {
		int ret = 0;
		errno = self->get_cachesize(self, NULL, NULL, &ret);
		return ret;
	}

	const char *get_create_dir() {
		const char *ret;
		errno = self->get_create_dir(self, &ret);
		return ret;
	}

	const char *get_filename() {
		const char *ret = NULL;
		errno = self->get_dbname(self, &ret, NULL);
		return ret;
	}

	const char *get_dbname() {
		const char *ret = NULL;
		errno = self->get_dbname(self, NULL, &ret);
		return ret;
	}

	u_int32_t get_encrypt_flags() {
		u_int32_t ret = 0;
		CRYPTO_ONLY(errno = self->get_encrypt_flags(self, &ret))
		return ret;
	}

	/*
	 * This method is implemented in Java to avoid wrapping the object on
	 * every call.
	 */
#ifndef SWIGJAVA
	DB_ENV *get_env() {
		DB_ENV *env = NULL;
		errno = self->get_env(self, &env);
		return env;
	}

	const char *get_errpfx() {
		const char *ret = NULL;
		errno = 0;
		self->get_errpfx(self, &ret);
		return ret;
	}
#endif

	u_int32_t get_flags() {
		u_int32_t ret = 0;
		errno = self->get_flags(self, &ret);
		return ret;
	}

	int get_lorder() {
		int ret = 0;
		errno = self->get_lorder(self, &ret);
		return ret;
	}

	DB_MPOOLFILE *get_mpf() {
		errno = 0;
		return self->get_mpf(self);
	}

	u_int32_t get_open_flags() {
		u_int32_t ret = 0;
		errno = self->get_open_flags(self, &ret);
		return ret;
	}

	u_int32_t get_pagesize() {
		u_int32_t ret = 0;
		errno = self->get_pagesize(self, &ret);
		return ret;
	}

	u_int32_t get_bt_minkey() {
		u_int32_t ret = 0;
		errno = self->get_bt_minkey(self, &ret);
		return ret;
	}

	u_int32_t get_h_ffactor() {
		u_int32_t ret = 0;
		errno = self->get_h_ffactor(self, &ret);
		return ret;
	}

	u_int32_t get_h_nelem() {
		u_int32_t ret = 0;
		errno = self->get_h_nelem(self, &ret);
		return ret;
	}

	int get_re_delim() {
		int ret = 0;
		errno = self->get_re_delim(self, &ret);
		return ret;
	}

	DB_CACHE_PRIORITY get_priority() {
		DB_CACHE_PRIORITY ret;
		errno = self->get_priority(self, &ret);
		return ret;
	}

	const char **get_partition_dirs() {
		const char **ret;
		errno = self->get_partition_dirs(self, &ret);
		return ret;
	}

	DBT *get_partition_keys() {
		DBT *ret = NULL;
		errno = self->get_partition_keys(self, NULL, &ret);
		return ret;
	}

	int get_partition_parts() {
		int ret = 0;
		errno = self->get_partition_keys(self, &ret, NULL);
                /* If not partitioned by range, check by callback. */
		if (ret == 0)
			errno = self->get_partition_callback(self, &ret, NULL);
		return ret;
	}

	u_int32_t get_re_len() {
		u_int32_t ret = 0;
		errno = self->get_re_len(self, &ret);
		return ret;
	}

	int get_re_pad() {
		int ret = 0;
		errno = self->get_re_pad(self, &ret);
		return ret;
	}

	const char *get_re_source() {
		const char *ret = NULL;
		errno = self->get_re_source(self, &ret);
		return ret;
	}

	u_int32_t get_q_extentsize() {
		u_int32_t ret = 0;
		errno = self->get_q_extentsize(self, &ret);
		return ret;
	}

	JAVA_EXCEPT_NONE
	int_bool get_multiple() {
		return self->get_multiple(self);
	}

	int_bool get_transactional() {
		return self->get_transactional(self);
	}

	JAVA_EXCEPT_ERRNO(DB_RETOK_STD, DB2JDBENV)
	DBTYPE get_type() {
		DBTYPE type = (DBTYPE)0;
		errno = self->get_type(self, &type);
		return type;
	}

	DBC *join(DBC **curslist, u_int32_t flags) {
		DBC *dbcp = NULL;
		errno = self->join(self, curslist, &dbcp, flags);
		return dbcp;
	}

	JAVA_EXCEPT(DB_RETOK_STD, DB2JDBENV)
	db_ret_t key_range(DB_TXN *txnid, DBT *key, DB_KEY_RANGE *key_range,
	    u_int32_t flags) {
		return self->key_range(self, txnid, key, key_range, flags);
	}

	db_ret_t open(DB_TXN *txnid, const char *file, const char *database,
	    DBTYPE type, u_int32_t flags, int mode) {
		return self->open(self, txnid, file, database,
		    type, flags, mode);
	}

	JAVA_EXCEPT(DB_RETOK_DBGET, DB2JDBENV)
	int pget(DB_TXN *txnid, DBT *key, DBT *pkey, DBT *data,
	    u_int32_t flags) {
		return self->pget(self, txnid, key, pkey, data, flags);
	}

	JAVA_EXCEPT(DB_RETOK_DBPUT, DB2JDBENV)
	int put(DB_TXN *txnid, DBT *key, DBT *db_put_data, u_int32_t flags) {
		return self->put(self, txnid, key, db_put_data, flags);
	}

	JAVA_EXCEPT(DB_RETOK_STD, NULL)
	db_ret_t remove(const char *file, const char *database,
	    u_int32_t flags) {
		return self->remove(self, file, database, flags);
	}

	db_ret_t rename(const char *file, const char *database,
	    const char *newname, u_int32_t flags) {
		return self->rename(self, file, database, newname, flags);
	}

	JAVA_EXCEPT(DB_RETOK_STD, DB2JDBENV)
	db_ret_t set_append_recno(
	    int (*db_append_recno_fcn)(DB *, DBT *, db_recno_t)) {
		return self->set_append_recno(self, db_append_recno_fcn);
	}

	db_ret_t set_bt_compare(
	    int (*bt_compare_fcn)(DB *, const DBT *, const DBT *)) {
		return self->set_bt_compare(self, bt_compare_fcn);
	}

	db_ret_t set_bt_minkey(u_int32_t bt_minkey) {
		return self->set_bt_minkey(self, bt_minkey);
	}

	db_ret_t set_bt_compress(
	    int (*bt_compress_fcn)(DB *, const DBT *, const DBT *,
	    const DBT *, const DBT *, DBT *),
	    int (*bt_decompress_fcn)(DB *, const DBT *, const DBT *,
	    DBT *, DBT *, DBT *)) {
		return self->set_bt_compress(
		    self, bt_compress_fcn, bt_decompress_fcn);
	}

	db_ret_t set_bt_prefix(
	    size_t (*bt_prefix_fcn)(DB *, const DBT *, const DBT *)) {
		return self->set_bt_prefix(self, bt_prefix_fcn);
	}

	db_ret_t set_cachesize(jlong bytes, int ncache) {
		return self->set_cachesize(self,
		    (u_int32_t)(bytes / GIGABYTE),
		    (u_int32_t)(bytes % GIGABYTE), ncache);
	}

	db_ret_t set_create_dir(const char *dir) {
		return self->set_create_dir(self, dir);
	}

	db_ret_t set_dup_compare(
	    int (*dup_compare_fcn)(DB *, const DBT *, const DBT *)) {
		return self->set_dup_compare(self, dup_compare_fcn);
	}

	db_ret_t set_encrypt(const char *passwd, u_int32_t flags) {
		return self->set_encrypt(self, passwd, flags);
	}

	JAVA_EXCEPT_NONE
#ifndef SWIGJAVA
	void set_errcall(
	   void (*db_errcall_fcn)(const DB_ENV *, const char *, const char *)) {
		self->set_errcall(self, db_errcall_fcn);
	}

	void set_errpfx(const char *errpfx) {
		self->set_errpfx(self, errpfx);
	}
#endif /* SWIGJAVA */

	JAVA_EXCEPT(DB_RETOK_STD, DB2JDBENV)
	db_ret_t set_feedback(void (*db_feedback_fcn)(DB *, int, int)) {
		return self->set_feedback(self, db_feedback_fcn);
	}

	db_ret_t set_flags(u_int32_t flags) {
		return self->set_flags(self, flags);
	}

	db_ret_t set_h_compare(
	    int (*h_compare_fcn)(DB *, const DBT *, const DBT *)) {
		return self->set_h_compare(self, h_compare_fcn);
	}

	db_ret_t set_h_ffactor(u_int32_t h_ffactor) {
		return self->set_h_ffactor(self, h_ffactor);
	}

	db_ret_t set_h_hash(
	    u_int32_t (*h_hash_fcn)(DB *, const void *, u_int32_t)) {
		return self->set_h_hash(self, h_hash_fcn);
	}

	db_ret_t set_h_nelem(u_int32_t h_nelem) {
		return self->set_h_nelem(self, h_nelem);
	}

	db_ret_t set_lorder(int lorder) {
		return self->set_lorder(self, lorder);
	}

#ifndef SWIGJAVA
	void set_msgcall(void (*db_msgcall_fcn)(const DB_ENV *, const char *)) {
		self->set_msgcall(self, db_msgcall_fcn);
	}
#endif /* SWIGJAVA */

	db_ret_t set_pagesize(u_int32_t pagesize) {
		return self->set_pagesize(self, pagesize);
	}

#ifndef SWIGJAVA
	db_ret_t set_paniccall(void (* db_panic_fcn)(DB_ENV *, int)) {
		return self->set_paniccall(self, db_panic_fcn);
	}
#endif /* SWIGJAVA */

	db_ret_t set_partition(u_int32_t parts, DBT *keys, 
	    u_int32_t (*db_partition_fcn)(DB *, DBT *)) {
		return self->set_partition(self, parts, keys, db_partition_fcn);
	}

	db_ret_t set_partition_dirs(const char **dirp) {
		return self->set_partition_dirs(self, dirp);
	}

	db_ret_t set_priority(DB_CACHE_PRIORITY priority) {
		return self->set_priority(self, priority);
	}

	db_ret_t set_re_delim(int re_delim) {
		return self->set_re_delim(self, re_delim);
	}

	db_ret_t set_re_len(u_int32_t re_len) {
		return self->set_re_len(self, re_len);
	}

	db_ret_t set_re_pad(int re_pad) {
		return self->set_re_pad(self, re_pad);
	}

	db_ret_t set_re_source(char *source) {
		return self->set_re_source(self, source);
	}

	db_ret_t set_q_extentsize(u_int32_t extentsize) {
		return self->set_q_extentsize(self, extentsize);
	}

	db_ret_t sort_multiple(DBT *key, DBT *data) {
		return self->sort_multiple(self, key, data, 0);
	}

	JAVA_EXCEPT_ERRNO(DB_RETOK_STD, DB2JDBENV)
	void *stat(DB_TXN *txnid, u_int32_t flags) {
		void *statp = NULL;
		errno = self->stat(self, txnid, &statp, flags);
		return statp;
	}

	JAVA_EXCEPT(DB_RETOK_STD, DB2JDBENV)
	db_ret_t sync(u_int32_t flags) {
		return self->sync(self, flags);
	}

	JAVA_EXCEPT_ERRNO(DB_RETOK_STD, DB2JDBENV)
	int truncate(DB_TXN *txnid, u_int32_t flags) {
		u_int32_t count = 0;
		errno = self->truncate(self, txnid, &count, flags);
		return count;
	}

	JAVA_EXCEPT(DB_RETOK_STD, DB2JDBENV)
	db_ret_t upgrade(const char *file, u_int32_t flags) {
		return self->upgrade(self, file, flags);
	}

	JAVA_EXCEPT_ERRNO(DB_RETOK_STD, NULL)
	int_bool verify(const char *file, const char *database,
	    struct __db_out_stream outfile, u_int32_t flags) {
		/*
		 * We can't easily #include "dbinc/db_ext.h" because of name
		 * clashes, so we declare this explicitly.
		 */
		extern int __db_verify_internal __P((DB *, const char *, const
		    char *, void *, int (*)(void *, const void *), u_int32_t));
		errno = __db_verify_internal(self, file, database,
		    outfile.handle, outfile.callback, flags);
		if (errno == DB_VERIFY_BAD) {
			errno = 0;
			return 0;
		} else
			return 1;
	}
}
};

struct Dbc
{
%extend {
	JAVA_EXCEPT(DB_RETOK_STD, NULL)
	db_ret_t close() {
		return self->close(self);
	}

	JAVA_EXCEPT_ERRNO(DB_RETOK_STD, DBC2JDBENV)
	int cmp(DBC *odbc, u_int32_t flags) {
		int result = 0;
		errno = self->cmp(self, odbc, &result, flags);
		return result;
	}

	JAVA_EXCEPT_ERRNO(DB_RETOK_STD, DBC2JDBENV)
	db_recno_t count(u_int32_t flags) {
		db_recno_t count = 0;
		errno = self->count(self, &count, flags);
		return count;
	}

	JAVA_EXCEPT(DB_RETOK_DBCDEL, DBC2JDBENV)
	int del(u_int32_t flags) {
		return self->del(self, flags);
	}

	JAVA_EXCEPT_ERRNO(DB_RETOK_STD, DBC2JDBENV)
	DBC *dup(u_int32_t flags) {
		DBC *newcurs = NULL;
		errno = self->dup(self, &newcurs, flags);
		return newcurs;
	}

	JAVA_EXCEPT(DB_RETOK_DBCGET, DBC2JDBENV)
	int get(DBT* key, DBT *data, u_int32_t flags) {
		return self->get(self, key, data, flags);
	}

	JAVA_EXCEPT_ERRNO(DB_RETOK_STD, DBC2JDBENV)
	DB_CACHE_PRIORITY get_priority() {
		DB_CACHE_PRIORITY ret;
		errno = self->get_priority(self, &ret);
		return ret;
	}

	JAVA_EXCEPT(DB_RETOK_DBCGET, DBC2JDBENV)
	int pget(DBT* key, DBT* pkey, DBT *data, u_int32_t flags) {
		return self->pget(self, key, pkey, data, flags);
	}

	JAVA_EXCEPT(DB_RETOK_DBCPUT, DBC2JDBENV)
	int put(DBT* key, DBT *db_put_data, u_int32_t flags) {
		return self->put(self, key, db_put_data, flags);
	}

	JAVA_EXCEPT_ERRNO(DB_RETOK_STD, DBC2JDBENV)
	db_ret_t set_priority(DB_CACHE_PRIORITY priority) {
		return self->set_priority(self, priority);
	}
}
};

struct DbEnv
{
%extend {
	JAVA_EXCEPT_ERRNO(DB_RETOK_STD, NULL)
	DbEnv(u_int32_t flags) {
		DB_ENV *self = NULL;
		errno = db_env_create(&self, flags);
		if (errno == 0)
			self->env->dbt_usercopy = __dbj_dbt_memcopy;
		return self;
	}

	JAVA_EXCEPT(DB_RETOK_STD, NULL)
	db_ret_t close(u_int32_t flags) {
		return self->close(self, flags);
	}

	JAVA_EXCEPT(DB_RETOK_STD, JDBENV)
	db_ret_t dbremove(DB_TXN *txnid, const char *file, const char *database,
	    u_int32_t flags) {
		return self->dbremove(self, txnid, file, database, flags);
	}

	db_ret_t dbrename(DB_TXN *txnid, const char *file, const char *database,
	    const char *newname, u_int32_t flags) {
		return self->dbrename(self,
		    txnid, file, database, newname, flags);
	}

	JAVA_EXCEPT_NONE
	void err(int error, const char *message) {
		self->err(self, error, message);
	}

	void errx(const char *message) {
		self->errx(self, message);
	}

#ifndef SWIGJAVA
	u_int32_t get_thread_count() {
		u_int32_t ret;
		errno = self->get_thread_count(self, &ret);
		return ret;
	}

	pid_t getpid() {
		pid_t ret;
		db_threadid_t junk;
		__os_id(self, &ret, &junk);
		return ret;
	}

	db_threadid_t get_threadid() {
		pid_t junk;
		db_threadid_t ret;
		__os_id(self, &junk, &ret);
		return ret;
	}

	JAVA_EXCEPT(DB_RETOK_STD, JDBENV)
	db_ret_t failchk(u_int32_t flags) {
		return self->failchk(self, flags);
	}

	db_ret_t set_isalive(
	    int (*isalive_fcn)(DB_ENV *, pid_t, db_threadid_t)) {
		return self->set_isalive(self, isalive_fcn);
	}

	db_ret_t set_thread_count(u_int32_t count) {
		return self->set_thread_count(self, count);
	}

	db_ret_t set_thread_id(void (*thread_id_fcn)(DB_ENV *, pid_t *,
	    db_threadid_t *)) {
		return self->set_thread_id(self, thread_id_fcn);
	}

	db_ret_t set_thread_id_string(char *(*thread_id_string_fcn)(DB_ENV *,
	    pid_t, db_threadid_t, char *)) {
		return self->set_thread_id_string(self, thread_id_string_fcn);
	}
#endif

	DB_TXN *cdsgroup_begin() {
		DB_TXN *tid = NULL;
		errno = self->cdsgroup_begin(self, &tid);
		return tid;
	}

	db_ret_t fileid_reset(const char *file, u_int32_t flags) {
		return self->fileid_reset(self, file, flags);
	}

	JAVA_EXCEPT_ERRNO(DB_RETOK_STD, JDBENV)
	const char **get_data_dirs() {
		const char **ret;
		errno = self->get_data_dirs(self, &ret);
		return ret;
	}

	u_int32_t get_encrypt_flags() {
		u_int32_t ret = 0;
		CRYPTO_ONLY(errno = self->get_encrypt_flags(self, &ret))
		return ret;
	}

#ifndef SWIGJAVA
	const char *get_errpfx() {
		const char *ret;
		errno = 0;
		self->get_errpfx(self, &ret);
		return ret;
	}
#endif /* SWIGJAVA */

	u_int32_t get_flags() {
		u_int32_t ret;
		errno = self->get_flags(self, &ret);
		return ret;
	}

	const char *get_home() {
		const char *ret;
		errno = self->get_home(self, &ret);
		return ret;
	}

	const char *get_intermediate_dir_mode() {
		const char *ret;
		errno = self->get_intermediate_dir_mode(self, &ret);
		return ret;
	}

	u_int32_t get_open_flags() {
		u_int32_t ret;
		errno = self->get_open_flags(self, &ret);
		return ret;
	}

	long get_shm_key() {
		long ret;
		errno = self->get_shm_key(self, &ret);
		return ret;
	}

	const char *get_tmp_dir() {
		const char *ret;
		errno = self->get_tmp_dir(self, &ret);
		return ret;
	}

	int_bool get_verbose(u_int32_t which) {
		int ret;
		errno = self->get_verbose(self, which, &ret);
		return ret;
	}

	JAVA_EXCEPT_NONE
	int_bool is_bigendian() {
		return self->is_bigendian();
	}

	JAVA_EXCEPT(DB_RETOK_STD, JDBENV)
	db_ret_t lsn_reset(const char *file, u_int32_t flags) {
		return self->lsn_reset(self, file, flags);
	}

	db_ret_t open(const char *db_home, u_int32_t flags, int mode) {
		return self->open(self, db_home, flags, mode);
	}

	JAVA_EXCEPT(DB_RETOK_STD, NULL)
	db_ret_t remove(const char *db_home, u_int32_t flags) {
		return self->remove(self, db_home, flags);
	}

	JAVA_EXCEPT(DB_RETOK_STD, JDBENV)
	db_ret_t set_cachesize(jlong bytes, int ncache) {
		return self->set_cachesize(self,
		    (u_int32_t)(bytes / GIGABYTE),
		    (u_int32_t)(bytes % GIGABYTE), ncache);
	}

	db_ret_t set_cache_max(jlong bytes) {
		return self->set_cache_max(self,
		    (u_int32_t)(bytes / GIGABYTE),
		    (u_int32_t)(bytes % GIGABYTE));
	}

	db_ret_t set_create_dir(const char *dir) {
		return self->set_create_dir(self, dir);
	}

	db_ret_t set_data_dir(const char *dir) {
		return self->set_data_dir(self, dir);
	}

	db_ret_t set_intermediate_dir_mode(const char *mode) {
		return self->set_intermediate_dir_mode(self, mode);
	}

	db_ret_t set_encrypt(const char *passwd, u_int32_t flags) {
		return self->set_encrypt(self, passwd, flags);
	}

	JAVA_EXCEPT_NONE
	void set_errcall(void (*db_errcall_fcn)(const DB_ENV *, const char *,
	    const char *)) {
		self->set_errcall(self, db_errcall_fcn);
	}

#ifndef SWIGJAVA
	void set_errpfx(const char *errpfx) {
		self->set_errpfx(self, errpfx);
	}
#endif

	JAVA_EXCEPT(DB_RETOK_STD, JDBENV)
	db_ret_t set_flags(u_int32_t flags, int_bool onoff) {
		return self->set_flags(self, flags, onoff);
	}

	db_ret_t set_feedback(void (*env_feedback_fcn)(DB_ENV *, int, int)) {
		return self->set_feedback(self, env_feedback_fcn);
	}

	db_ret_t set_mp_max_openfd(int maxopenfd) {
		return self->set_mp_max_openfd(self, maxopenfd);
	}

	db_ret_t set_mp_max_write(int maxwrite, db_timeout_t maxwrite_sleep) {
		return self->set_mp_max_write(self, maxwrite, maxwrite_sleep);
	}

	db_ret_t set_mp_mmapsize(size_t mp_mmapsize) {
		return self->set_mp_mmapsize(self, mp_mmapsize);
	}

	db_ret_t set_mp_pagesize(size_t mp_pagesize) {
		return self->set_mp_pagesize(self, mp_pagesize);
	}

	db_ret_t set_mp_tablesize(size_t mp_tablesize) {
		return self->set_mp_tablesize(self, mp_tablesize);
	}

	JAVA_EXCEPT_NONE
	void set_msgcall(void (*db_msgcall_fcn)(const DB_ENV *, const char *)) {
		self->set_msgcall(self, db_msgcall_fcn);
	}

	JAVA_EXCEPT(DB_RETOK_STD, JDBENV)
	db_ret_t set_paniccall(void (*db_panic_fcn)(DB_ENV *, int)) {
		return self->set_paniccall(self, db_panic_fcn);
	}

	db_ret_t set_rpc_server(char *host,
	    long cl_timeout, long sv_timeout, u_int32_t flags) {
		return self->set_rpc_server(self, NULL, host,
		    cl_timeout, sv_timeout, flags);
	}

	db_ret_t set_shm_key(long shm_key) {
		return self->set_shm_key(self, shm_key);
	}

	db_ret_t set_timeout(db_timeout_t timeout, u_int32_t flags) {
		return self->set_timeout(self, timeout, flags);
	}

	db_ret_t set_tmp_dir(const char *dir) {
		return self->set_tmp_dir(self, dir);
	}

	db_ret_t set_tx_max(u_int32_t max) {
		return self->set_tx_max(self, max);
	}

	db_ret_t set_app_dispatch(
	    int (*tx_recover)(DB_ENV *, DBT *, DB_LSN *, db_recops)) {
		return self->set_app_dispatch(self, tx_recover);
	}

	db_ret_t set_event_notify(
	    void (*event_notify)(DB_ENV *, u_int32_t, void *)) {
		return self->set_event_notify(self, event_notify);
	}

	db_ret_t set_tx_timestamp(time_t *timestamp) {
		return self->set_tx_timestamp(self, timestamp);
	}

	db_ret_t set_verbose(u_int32_t which, int_bool onoff) {
		return self->set_verbose(self, which, onoff);
	}

	/* Lock functions */
	JAVA_EXCEPT_ERRNO(DB_RETOK_STD, JDBENV)
	struct __db_lk_conflicts get_lk_conflicts() {
		struct __db_lk_conflicts ret;
		errno = self->get_lk_conflicts(self,
		    (const u_int8_t **)&ret.lk_conflicts, &ret.lk_modes);
		return ret;
	}

	u_int32_t get_lk_detect() {
		u_int32_t ret;
		errno = self->get_lk_detect(self, &ret);
		return ret;
	}

	u_int32_t get_lk_max_locks() {
		u_int32_t ret;
		errno = self->get_lk_max_locks(self, &ret);
		return ret;
	}

	u_int32_t get_lk_max_lockers() {
		u_int32_t ret;
		errno = self->get_lk_max_lockers(self, &ret);
		return ret;
	}

	u_int32_t get_lk_max_objects() {
		u_int32_t ret;
		errno = self->get_lk_max_objects(self, &ret);
		return ret;
	}

	u_int32_t get_lk_partitions() {
		u_int32_t ret;
		errno = self->get_lk_partitions(self, &ret);
		return ret;
	}

	int lock_detect(u_int32_t flags, u_int32_t atype) {
		int aborted;
		errno = self->lock_detect(self, flags, atype, &aborted);
		return aborted;
	}

	DB_LOCK *lock_get(u_int32_t locker,
	    u_int32_t flags, DBT *object, db_lockmode_t lock_mode) {
		DB_LOCK *lock = NULL;
		if ((errno = __os_malloc(self->env, sizeof (DB_LOCK), &lock)) == 0)
			errno = self->lock_get(self, locker, flags, object,
			    lock_mode, lock);
		return lock;
	}

	u_int32_t lock_id() {
		u_int32_t id;
		errno = self->lock_id(self, &id);
		return id;
	}

	JAVA_EXCEPT(DB_RETOK_STD, JDBENV)
	db_ret_t lock_id_free(u_int32_t id) {
		return self->lock_id_free(self, id);
	}

	db_ret_t lock_put(DB_LOCK *lock) {
		return self->lock_put(self, lock);
	}

	JAVA_EXCEPT_ERRNO(DB_RETOK_STD, JDBENV)
	DB_LOCK_STAT *lock_stat(u_int32_t flags) {
		DB_LOCK_STAT *statp = NULL;
		errno = self->lock_stat(self, &statp, flags);
		return statp;
	}

#ifndef SWIGJAVA
	/* For Java, this is defined in native code */
	db_ret_t lock_vec(u_int32_t locker, u_int32_t flags, DB_LOCKREQ *list,
	    int offset, int nlist)
	{
		DB_LOCKREQ *elistp;
		return self->lock_vec(self, locker, flags, list + offset,
		    nlist, &elistp);
	}
#endif

	JAVA_EXCEPT(DB_RETOK_STD, JDBENV)
	db_ret_t set_lk_conflicts(struct __db_lk_conflicts conflicts) {
		return self->set_lk_conflicts(self,
		    conflicts.lk_conflicts, conflicts.lk_modes);
	}

	db_ret_t set_lk_detect(u_int32_t detect) {
		return self->set_lk_detect(self, detect);
	}

	db_ret_t set_lk_max_lockers(u_int32_t max) {
		return self->set_lk_max_lockers(self, max);
	}

	db_ret_t set_lk_max_locks(u_int32_t max) {
		return self->set_lk_max_locks(self, max);
	}

	db_ret_t set_lk_max_objects(u_int32_t max) {
		return self->set_lk_max_objects(self, max);
	}

	db_ret_t set_lk_partitions(u_int32_t partitions) {
		return self->set_lk_partitions(self, partitions);
	}

	/* Log functions */
	JAVA_EXCEPT_ERRNO(DB_RETOK_STD, JDBENV)
	u_int32_t get_lg_bsize() {
		u_int32_t ret;
		errno = self->get_lg_bsize(self, &ret);
		return ret;
	}

	const char *get_lg_dir() {
		const char *ret;
		errno = self->get_lg_dir(self, &ret);
		return ret;
	}

	int get_lg_filemode() {
		int ret;
		errno = self->get_lg_filemode(self, &ret);
		return ret;
	}

	u_int32_t get_lg_max() {
		u_int32_t ret;
		errno = self->get_lg_max(self, &ret);
		return ret;
	}

	u_int32_t get_lg_regionmax() {
		u_int32_t ret;
		errno = self->get_lg_regionmax(self, &ret);
		return ret;
	}

	char **log_archive(u_int32_t flags) {
		char **list = NULL;
		errno = self->log_archive(self, &list, flags);
		return list;
	}

	JAVA_EXCEPT_NONE
	static int log_compare(const DB_LSN *lsn0, const DB_LSN *lsn1) {
		return log_compare(lsn0, lsn1);
	}

	JAVA_EXCEPT_ERRNO(DB_RETOK_STD, JDBENV)
	DB_LOGC *log_cursor(u_int32_t flags) {
		DB_LOGC *cursor = NULL;
		errno = self->log_cursor(self, &cursor, flags);
		return cursor;
	}

	char *log_file(DB_LSN *lsn) {
		char namebuf[DB_MAXPATHLEN];
		errno = self->log_file(self, lsn, namebuf, sizeof namebuf);
		return (errno == 0) ? strdup(namebuf) : NULL;
	}

	JAVA_EXCEPT(DB_RETOK_STD, JDBENV)
	db_ret_t log_flush(const DB_LSN *lsn_or_null) {
		return self->log_flush(self, lsn_or_null);
	}

	JAVA_EXCEPT_ERRNO(DB_RETOK_STD, JDBENV)
	int_bool log_get_config(u_int32_t which) {
		int ret;
		errno = self->log_get_config(self, which, &ret);
		return ret;
	}

	JAVA_EXCEPT(DB_RETOK_STD, JDBENV)
	db_ret_t log_put(DB_LSN *lsn, const DBT *data, u_int32_t flags) {
		return self->log_put(self, lsn, data, flags);
	}

	db_ret_t log_print(DB_TXN *txn, const char *msg) {
		return self->log_printf(self, txn, "%s", msg);
	}

	db_ret_t log_set_config(u_int32_t which, int_bool onoff) {
		return self->log_set_config(self, which, onoff);
	}

	JAVA_EXCEPT_ERRNO(DB_RETOK_STD, JDBENV)
	DB_LOG_STAT *log_stat(u_int32_t flags) {
		DB_LOG_STAT *sp = NULL;
		errno = self->log_stat(self, &sp, flags);
		return sp;
	}

	JAVA_EXCEPT(DB_RETOK_STD, JDBENV)
	db_ret_t set_lg_bsize(u_int32_t lg_bsize) {
		return self->set_lg_bsize(self, lg_bsize);
	}

	db_ret_t set_lg_dir(const char *dir) {
		return self->set_lg_dir(self, dir);
	}

	db_ret_t set_lg_filemode(int mode) {
		return self->set_lg_filemode(self, mode);
	}

	db_ret_t set_lg_max(u_int32_t lg_max) {
		return self->set_lg_max(self, lg_max);
	}

	db_ret_t set_lg_regionmax(u_int32_t lg_regionmax) {
		return self->set_lg_regionmax(self, lg_regionmax);
	}

	/* Memory pool functions */
	JAVA_EXCEPT_ERRNO(DB_RETOK_STD, JDBENV)
	jlong get_cachesize() {
		u_int32_t gbytes, bytes;
		errno = self->get_cachesize(self, &gbytes, &bytes, NULL);
		return (jlong)gbytes * GIGABYTE + bytes;
	}

	int get_cachesize_ncache() {
		int ret;
		errno = self->get_cachesize(self, NULL, NULL, &ret);
		return ret;
	}

	jlong get_cache_max() {
		u_int32_t gbytes, bytes;
		errno = self->get_cache_max(self, &gbytes, &bytes);
		return (jlong)gbytes * GIGABYTE + bytes;
	}

	const char *get_create_dir() {
		const char *ret;
		errno = self->get_create_dir(self, &ret);
		return ret;
	}

	int get_mp_max_openfd() {
		int ret;
		errno = self->get_mp_max_openfd(self, &ret);
		return ret;
	}

	int get_mp_max_write() {
		int maxwrite;
		db_timeout_t sleep;
		errno = self->get_mp_max_write(self, &maxwrite, &sleep);
		return maxwrite;
	}

	db_timeout_t get_mp_max_write_sleep() {
		int maxwrite;
		db_timeout_t sleep;
		errno = self->get_mp_max_write(self, &maxwrite, &sleep);
		return sleep;
	}

	size_t get_mp_mmapsize() {
		size_t ret;
		errno = self->get_mp_mmapsize(self, &ret);
		return ret;
	}

	int get_mp_pagesize() {
		int ret;
		errno = self->get_mp_pagesize(self, &ret);
		return ret;
	}

	int get_mp_tablesize() {
		int ret;
		errno = self->get_mp_tablesize(self, &ret);
		return ret;
	}

	DB_MPOOL_STAT *memp_stat(u_int32_t flags) {
		DB_MPOOL_STAT *mp_stat = NULL;
		errno = self->memp_stat(self, &mp_stat, NULL, flags);
		return mp_stat;
	}

	DB_MPOOL_FSTAT **memp_fstat(u_int32_t flags) {
		DB_MPOOL_FSTAT **mp_fstat = NULL;
		errno = self->memp_stat(self, NULL, &mp_fstat, flags);
		return mp_fstat;
	}

	JAVA_EXCEPT_ERRNO(DB_RETOK_STD, JDBENV)
	db_ret_t memp_sync(DB_LSN *lsn) {
		return self->memp_sync(self, lsn);
	}

	int memp_trickle(int percent) {
		int ret;
		errno = self->memp_trickle(self, percent, &ret);
		return ret;
	}

	/* Mutex functions */
	JAVA_EXCEPT_ERRNO(DB_RETOK_STD, JDBENV)
	u_int32_t mutex_get_align() {
		u_int32_t ret;
		errno = self->mutex_get_align(self, &ret);
		return ret;
	}

	u_int32_t mutex_get_increment() {
		u_int32_t ret;
		errno = self->mutex_get_increment(self, &ret);
		return ret;
	}

	u_int32_t mutex_get_max() {
		u_int32_t ret;
		errno = self->mutex_get_max(self, &ret);
		return ret;
	}

	u_int32_t mutex_get_tas_spins() {
		u_int32_t ret;
		errno = self->mutex_get_tas_spins(self, &ret);
		return ret;
	}

	JAVA_EXCEPT(DB_RETOK_STD, JDBENV)
	db_ret_t mutex_set_align(u_int32_t align) {
		return self->mutex_set_align(self, align);
	}

	db_ret_t mutex_set_increment(u_int32_t increment) {
		return self->mutex_set_increment(self, increment);
	}

	db_ret_t mutex_set_max(u_int32_t mutex_max) {
		return self->mutex_set_max(self, mutex_max);
	}

	db_ret_t mutex_set_tas_spins(u_int32_t tas_spins) {
		return self->mutex_set_tas_spins(self, tas_spins);
	}

	JAVA_EXCEPT_ERRNO(DB_RETOK_STD, JDBENV)
	DB_MUTEX_STAT *mutex_stat(u_int32_t flags) {
		DB_MUTEX_STAT *statp = NULL;
		errno = self->mutex_stat(self, &statp, flags);
		return statp;
	}

	/* Transaction functions */
	u_int32_t get_tx_max() {
		u_int32_t ret;
		errno = self->get_tx_max(self, &ret);
		return ret;
	}

	time_t get_tx_timestamp() {
		time_t ret;
		errno = self->get_tx_timestamp(self, &ret);
		return ret;
	}

	db_timeout_t get_timeout(u_int32_t flag) {
		db_timeout_t ret;
		errno = self->get_timeout(self, &ret, flag);
		return ret;
	}

	DB_TXN *txn_begin(DB_TXN *parent, u_int32_t flags) {
		DB_TXN *tid = NULL;
		errno = self->txn_begin(self, parent, &tid, flags);
		return tid;
	}

	JAVA_EXCEPT(DB_RETOK_STD, JDBENV)
	db_ret_t txn_checkpoint(u_int32_t kbyte, u_int32_t min,
	    u_int32_t flags) {
		return self->txn_checkpoint(self, kbyte, min, flags);
	}

	JAVA_EXCEPT_ERRNO(DB_RETOK_STD, JDBENV)
	DB_PREPLIST *txn_recover(int count, u_int32_t flags) {
		DB_PREPLIST *preplist;
		u_int32_t retcount;

		/* Add a NULL element to terminate the array. */
		if ((errno = __os_malloc(self->env,
		    (count + 1) * sizeof(DB_PREPLIST), &preplist)) != 0)
			return NULL;

		if ((errno = self->txn_recover(self, preplist, count,
		    &retcount, flags)) != 0) {
			__os_free(self->env, preplist);
			return NULL;
		}

		preplist[retcount].txn = NULL;
		return preplist;
	}

	DB_TXN_STAT *txn_stat(u_int32_t flags) {
		DB_TXN_STAT *statp = NULL;
		errno = self->txn_stat(self, &statp, flags);
		return statp;
	}

	/* Replication functions */
	jlong rep_get_limit() {
		u_int32_t gbytes, bytes;
		errno = self->rep_get_limit(self, &gbytes, &bytes);
		return (jlong)gbytes * GIGABYTE + bytes;
	}

	JAVA_EXCEPT(DB_RETOK_STD, JDBENV)
	db_ret_t rep_elect(u_int32_t nsites, int nvotes, u_int32_t flags) {
		return self->rep_elect(self, nsites, nvotes, flags);
	}
 
	JAVA_EXCEPT(DB_RETOK_REPPMSG, JDBENV)
	int rep_process_message(DBT *control, DBT *rec, int envid,
	    DB_LSN *ret_lsn) {
		return self->rep_process_message(self, control, rec, envid,
		    ret_lsn);
	}

	JAVA_EXCEPT(DB_RETOK_STD, JDBENV)
	db_ret_t rep_flush() {
		return self->rep_flush(self);
	}

	db_ret_t rep_set_config(u_int32_t which, int_bool onoff) {
		return self->rep_set_config(self, which, onoff);
	}

	db_ret_t rep_set_clockskew(u_int32_t fast_clock, u_int32_t slow_clock) {
		return self->rep_set_clockskew(self, fast_clock, slow_clock);
	}

	JAVA_EXCEPT_ERRNO(DB_RETOK_STD, JDBENV)
	u_int32_t rep_get_clockskew_fast() {
		u_int32_t fast_clock, slow_clock;
		errno = self->rep_get_clockskew(self, &fast_clock, &slow_clock);
		return fast_clock;
	}

	u_int32_t rep_get_clockskew_slow() {
		u_int32_t fast_clock, slow_clock;
		errno = self->rep_get_clockskew(self, &fast_clock, &slow_clock);
		return slow_clock;
	}

	JAVA_EXCEPT(DB_RETOK_STD, JDBENV)
	db_ret_t rep_start(DBT *cdata, u_int32_t flags) {
		return self->rep_start(self, cdata, flags);
	}

	db_ret_t rep_sync(u_int32_t flags) {
		return self->rep_sync(self, flags);
	}

	JAVA_EXCEPT_ERRNO(DB_RETOK_STD, JDBENV)
	int_bool rep_get_config(u_int32_t which) {
		int ret;
		errno = self->rep_get_config(self, which, &ret);
		return ret;
	}

	DB_REP_STAT *rep_stat(u_int32_t flags) {
		DB_REP_STAT *statp = NULL;
		errno = self->rep_stat(self, &statp, flags);
		return statp;
	}

	JAVA_EXCEPT(DB_RETOK_STD, JDBENV)
	db_ret_t rep_set_limit(jlong bytes) {
		return self->rep_set_limit(self,
		    (u_int32_t)(bytes / GIGABYTE),
		    (u_int32_t)(bytes % GIGABYTE));
	}

	JAVA_EXCEPT_ERRNO(DB_RETOK_STD, JDBENV)
	u_int32_t rep_get_request_min(){
		u_int32_t min, max;
		errno = self->rep_get_request(self, &min, &max);
		return min;
	}

	u_int32_t rep_get_request_max(){
		u_int32_t min, max;
		errno = self->rep_get_request(self, &min, &max);
		return max;
	}

	JAVA_EXCEPT(DB_RETOK_STD, JDBENV)
	db_ret_t rep_set_request(u_int32_t min, u_int32_t max) {
		return self->rep_set_request(self, min, max);
	}

	db_ret_t rep_set_transport(int envid,
	    int (*send)(DB_ENV *, const DBT *, const DBT *,
	    const DB_LSN *, int, u_int32_t)) {
		return self->rep_set_transport(self, envid, send);
	}

	/* Advanced replication functions. */
	JAVA_EXCEPT_ERRNO(DB_RETOK_STD, JDBENV)
	u_int32_t rep_get_nsites() {
		u_int32_t ret;
		errno = self->rep_get_nsites(self, &ret);
		return ret;
	}

	u_int32_t rep_get_priority() {
		u_int32_t ret;
		errno = self->rep_get_priority(self, &ret);
		return ret;
	}

	u_int32_t rep_get_timeout(int which) {
		u_int32_t ret;
		errno = self->rep_get_timeout(self, which, &ret);
		return ret;
	}

	JAVA_EXCEPT(DB_RETOK_STD, JDBENV)
	db_ret_t rep_set_nsites(u_int32_t number) {
		return self->rep_set_nsites(self, number);
	}

	db_ret_t rep_set_priority(u_int32_t priority) {
		return self->rep_set_priority(self, priority);
	}

	db_ret_t rep_set_timeout(int which, db_timeout_t timeout) {
		return self->rep_set_timeout(self, which, timeout);
	}

	JAVA_EXCEPT_ERRNO(DB_RETOK_STD, JDBENV)
	int repmgr_add_remote_site(const char * host, u_int32_t port,
	    u_int32_t flags) {
		int eid;
		errno = self->repmgr_add_remote_site(self, host, port, &eid, flags);
		return eid;
	}

	JAVA_EXCEPT(DB_RETOK_STD, JDBENV)
	db_ret_t repmgr_get_ack_policy() {
		int ret;
		errno = self->repmgr_get_ack_policy(self, &ret);
		return ret;
	}

	db_ret_t repmgr_set_ack_policy(int policy) {
		return self->repmgr_set_ack_policy(self, policy);
	}

	db_ret_t repmgr_set_local_site(const char * host, u_int32_t port, u_int32_t flags) {
		return self->repmgr_set_local_site(self, host, port, flags);
	}

	JAVA_EXCEPT_ERRNO(DB_RETOK_STD, JDBENV)
	struct __db_repmgr_sites repmgr_site_list() {
		struct __db_repmgr_sites sites;
		errno = self->repmgr_site_list(self,
		    &sites.nsites, &sites.sites);
		return sites;
	}

	JAVA_EXCEPT(DB_RETOK_REPMGR_START, JDBENV)
	db_ret_t repmgr_start(int nthreads, u_int32_t flags) {
		return self->repmgr_start(self, nthreads, flags);
	}

	JAVA_EXCEPT_ERRNO(DB_RETOK_STD, JDBENV)
	DB_REPMGR_STAT *repmgr_stat(u_int32_t flags) {
		DB_REPMGR_STAT *statp = NULL;
		errno = self->repmgr_stat(self, &statp, flags);
		return statp;
	}

	/* Convert DB errors to strings */
	JAVA_EXCEPT_NONE
	static const char *strerror(int error) {
		return db_strerror(error);
	}

	/* Versioning information */
	static int get_version_major() {
		return DB_VERSION_MAJOR;
	}

	static int get_version_minor() {
		return DB_VERSION_MINOR;
	}

	static int get_version_patch() {
		return DB_VERSION_PATCH;
	}

	static const char *get_version_string() {
		return DB_VERSION_STRING;
	}
}
};

struct DbLock
{
%extend {
	JAVA_EXCEPT_NONE
	~DbLock() {
		__os_free(NULL, self);
	}
}
};

struct DbLogc
{
%extend {
	JAVA_EXCEPT(DB_RETOK_STD, NULL)
	db_ret_t close(u_int32_t flags) {
		return self->close(self, flags);
	}

	JAVA_EXCEPT(DB_RETOK_LGGET, NULL)
	int get(DB_LSN *lsn, DBT *data, u_int32_t flags) {
		return self->get(self, lsn, data, flags);
	}

	JAVA_EXCEPT_ERRNO(DB_RETOK_STD, NULL)
	u_int32_t version(u_int32_t flags) {
		u_int32_t result;
		errno = self->version(self, &result, flags);
		return result;
	}
}
};

#ifndef SWIGJAVA
struct DbLsn
{
%extend {
	JAVA_EXCEPT_ERRNO(DB_RETOK_STD, NULL)
	DbLsn(u_int32_t file, u_int32_t offset) {
		DB_LSN *self = NULL;
		if ((errno = __os_malloc(NULL, sizeof (DB_LSN), &self)) == 0) {
			self->file = file;
			self->offset = offset;
		}
		return self;
	}

	JAVA_EXCEPT_NONE
	~DbLsn() {
		__os_free(NULL, self);
	}

	u_int32_t get_file() {
		return self->file;
	}

	u_int32_t get_offset() {
		return self->offset;
	}
}
};
#endif

struct DbMpoolFile
{
%extend {
	JAVA_EXCEPT_ERRNO(DB_RETOK_STD, NULL)
	DB_CACHE_PRIORITY get_priority() {
		DB_CACHE_PRIORITY ret;
		errno = self->get_priority(self, &ret);
		return ret;
	}

	JAVA_EXCEPT(DB_RETOK_STD, NULL)
	db_ret_t set_priority(DB_CACHE_PRIORITY priority) {
		return self->set_priority(self, priority);
	}

	JAVA_EXCEPT_ERRNO(DB_RETOK_STD, NULL)
	u_int32_t get_flags() {
		u_int32_t ret;
		errno = self->get_flags(self, &ret);
		return ret;
	}

	JAVA_EXCEPT(DB_RETOK_STD, NULL)
	db_ret_t set_flags(u_int32_t flags, int_bool onoff) {
		return self->set_flags(self, flags, onoff);
	}

	JAVA_EXCEPT_ERRNO(DB_RETOK_STD, NULL)
	jlong get_maxsize() {
		u_int32_t gbytes, bytes;
		errno = self->get_maxsize(self, &gbytes, &bytes);
		return (jlong)gbytes * GIGABYTE + bytes;
	}

	/* New method - no backwards compatibility version */
	JAVA_EXCEPT(DB_RETOK_STD, NULL)
	db_ret_t set_maxsize(jlong bytes) {
		return self->set_maxsize(self,
		    (u_int32_t)(bytes / GIGABYTE),
		    (u_int32_t)(bytes % GIGABYTE));
	}
}
};

struct DbSequence
{
%extend {
	JAVA_EXCEPT_ERRNO(DB_RETOK_STD, NULL)
	DbSequence(DB *db, u_int32_t flags) {
		DB_SEQUENCE *self = NULL;
		errno = db_sequence_create(&self, db, flags);
		return self;
	}

	JAVA_EXCEPT(DB_RETOK_STD, NULL)
	db_ret_t close(u_int32_t flags) {
		return self->close(self, flags);
	}

	JAVA_EXCEPT_ERRNO(DB_RETOK_STD, NULL)
	db_seq_t get(DB_TXN *txnid, int32_t delta, u_int32_t flags) {
		db_seq_t ret = 0;
		errno = self->get(self, txnid, delta, &ret, flags);
		return ret;
	}

	int32_t get_cachesize() {
		int32_t ret = 0;
		errno = self->get_cachesize(self, &ret);
		return ret;
	}

	DB *get_db() {
		DB *ret = NULL;
		errno = self->get_db(self, &ret);
		return ret;
	}

	u_int32_t get_flags() {
		u_int32_t ret = 0;
		errno = self->get_flags(self, &ret);
		return ret;
	}

	JAVA_EXCEPT(DB_RETOK_STD, NULL)
	db_ret_t get_key(DBT *key) {
		return self->get_key(self, key);
	}

	JAVA_EXCEPT_ERRNO(DB_RETOK_STD, NULL)
	db_seq_t get_range_min() {
		db_seq_t ret = 0;
		errno = self->get_range(self, &ret, NULL);
		return ret;
	}

	db_seq_t get_range_max() {
		db_seq_t ret = 0;
		errno = self->get_range(self, NULL, &ret);
		return ret;
	}

	JAVA_EXCEPT(DB_RETOK_STD, NULL)
	db_ret_t initial_value(db_seq_t val) {
		return self->initial_value(self, val);
	}

	db_ret_t open(DB_TXN *txnid, DBT *key, u_int32_t flags) {
		return self->open(self, txnid, key, flags);
	}

	db_ret_t remove(DB_TXN *txnid, u_int32_t flags) {
		return self->remove(self, txnid, flags);
	}

	db_ret_t set_cachesize(int32_t size) {
		return self->set_cachesize(self, size);
	}

	db_ret_t set_flags(u_int32_t flags) {
		return self->set_flags(self, flags);
	}

	db_ret_t set_range(db_seq_t min, db_seq_t max) {
		return self->set_range(self, min, max);
	}

	JAVA_EXCEPT_ERRNO(DB_RETOK_STD, NULL)
	DB_SEQUENCE_STAT *stat(u_int32_t flags) {
		DB_SEQUENCE_STAT *ret = NULL;
		errno = self->stat(self, &ret, flags);
		return ret;
	}
}
};

struct DbTxn
{
%extend {
	JAVA_EXCEPT(DB_RETOK_STD, NULL)
	db_ret_t abort() {
		return self->abort(self);
	}

	db_ret_t commit(u_int32_t flags) {
		return self->commit(self, flags);
	}

	db_ret_t discard(u_int32_t flags) {
		return self->discard(self, flags);
	}

	JAVA_EXCEPT_ERRNO(DB_RETOK_STD, TXN2JDBENV)
	const char *get_name() {
		const char *name = NULL;
		errno = self->get_name(self, &name);
		return name;
	}

	JAVA_EXCEPT_NONE
	u_int32_t id() {
		return self->id(self);
	}

	JAVA_EXCEPT(DB_RETOK_STD, TXN2JDBENV)
	db_ret_t prepare(u_int8_t *gid) {
		return self->prepare(self, gid);
	}

	db_ret_t set_timeout(db_timeout_t timeout, u_int32_t flags) {
		return self->set_timeout(self, timeout, flags);
	}

	db_ret_t set_name(const char *name) {
		return self->set_name(self, name);
	}
}
};

