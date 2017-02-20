%module libdb_csharp
%{
#include "db_config.h"
#include "db_int.h"
#include "dbinc/txn.h"
#include "db.h"
%}

typedef unsigned int u_int;
typedef unsigned long u_int32_t;
typedef u_int32_t size_t;
typedef unsigned long db_pgno_t;
typedef unsigned long db_recno_t;
typedef u_int32_t db_mutex_t;
typedef u_int32_t db_threadid_t;
typedef unsigned long db_timeout_t;
typedef int pid_t;
typedef	uintptr_t	roff_t;
typedef u_int32_t uintptr_t;
typedef int int32_t;
typedef int64_t db_seq_t;
typedef long long int int64_t;
typedef u_int32_t DB_CACHE_PRIORITY;

%csmethodmodifiers "internal";
%typemap(csclassmodifiers) SWIGTYPE "internal class"
%pragma(csharp) moduleclassmodifiers="internal class"
%typemap(csclassmodifiers) DBTYPE "internal enum"
%typemap(csclassmodifiers) db_lockmode_t "internal enum"
%typemap(csclassmodifiers) db_lockop_t "internal enum"
%typemap(csclassmodifiers) db_recops "internal enum"

typedef enum {
	DB_BTREE=1,
	DB_HASH=2,
	DB_RECNO=3,
	DB_QUEUE=4,
	DB_UNKNOWN=5			/* Figure it out on open. */
} DBTYPE;

typedef enum {
	DB_LOCK_NG=0,			/* Not granted. */
	DB_LOCK_READ=1,			/* Shared/read. */
	DB_LOCK_WRITE=2,		/* Exclusive/write. */	
	DB_LOCK_WAIT=3,			/* Wait for event */
	DB_LOCK_IWRITE=4,		/* Intent exclusive/write. */
	DB_LOCK_IREAD=5,		/* Intent to share/read. */
	DB_LOCK_IWR=6,			/* Intent to read and write. */
	DB_LOCK_READ_UNCOMMITTED=7,	/* Degree 1 isolation. */
	DB_LOCK_WWRITE=8		/* Was Written. */
} db_lockmode_t;

typedef enum {
	DB_LOCK_DUMP=0,			/* Display held locks. */
	DB_LOCK_GET=1,			/* Get the lock. */
	DB_LOCK_GET_TIMEOUT=2,		/* Get lock with a timeout. */
	DB_LOCK_INHERIT=3,		/* Pass locks to parent. */
	DB_LOCK_PUT=4,			/* Release the lock. */
	DB_LOCK_PUT_ALL=5,		/* Release locker's locks. */
	DB_LOCK_PUT_OBJ=6,		/* Release locker's locks on obj. */
	DB_LOCK_PUT_READ=7,		/* Release locker's read locks. */
	DB_LOCK_TIMEOUT=8,		/* Force a txn to timeout. */
	DB_LOCK_TRADE=9,		/* Trade locker ids on a lock. */
	DB_LOCK_UPGRADE_WRITE=10	/* Upgrade writes for dirty reads. */
} db_lockop_t;

typedef enum {
	DB_TXN_ABORT=0,			/* Public. */
	DB_TXN_APPLY=1,			/* Public. */
	DB_TXN_BACKWARD_ROLL=3,		/* Public. */
	DB_TXN_FORWARD_ROLL=4,		/* Public. */
	DB_TXN_OPENFILES=5,		/* Internal. */
	DB_TXN_POPENFILES=6,		/* Internal. */
	DB_TXN_PRINT=7			/* Public. */
} db_recops;

struct __db;			typedef struct __db DB;
struct __db_compact;	typedef struct __db_compact DB_COMPACT;
struct __db_lock_u;		typedef struct __db_lock_u DB_LOCK;
struct __db_lsn;		typedef struct __db_lsn DB_LSN;
struct __db_preplist;	typedef struct __db_preplist DB_PREPLIST;
struct __db_repmgrsite; typedef struct __db_repmgrsite DB_REPMGR_SITE;
struct __db_sequence;	typedef struct __db_sequence DB_SEQUENCE;
struct __dbc;			typedef struct __dbc DBC;
struct __dbenv;			typedef struct __dbenv DB_ENV;
struct __dbt;			typedef struct __dbt DBT;
struct __dbtxn;			typedef struct __dbtxn DB_TXN;
struct __key_range;		typedef struct __key_range DB_KEY_RANGE;

%typemap(cstype) char ** "ref string"
%typemap(imtype) char ** "ref string"
%typemap(csin) char ** "ref $csinput"
%typemap(cstype) char *** "ref string[]"
%typemap(imtype) char *** "ref string[]"
%typemap(csin) char *** "ref $csinput"
%typemap(cstype) int * "ref int"
%typemap(imtype) int * "ref int"
%typemap(csin) int * "ref $csinput"
%typemap(cstype) u_int32_t * "ref uint"
%typemap(imtype) u_int32_t * "ref uint"
%typemap(csin) u_int32_t * "ref $csinput"
%typemap(cstype) u_int * "ref uint"
%typemap(imtype) u_int * "ref uint"
%typemap(csin) u_int * "ref $csinput"
%typemap(cstype) db_pgno_t * "ref uint"
%typemap(imtype) db_pgno_t * "ref uint"
%typemap(csin) db_pgno_t * "ref $csinput"
%typemap(cstype) time_t * "ref long"
%typemap(imtype) time_t * "ref long"
%typemap(csin) time_t * "ref $csinput"
%typemap(cstype) db_timeout_t * "ref uint"
%typemap(imtype) db_timeout_t * "ref uint"
%typemap(csin) db_timeout_t * "ref $csinput"
%typemap(cstype) db_seq_t "Int64"
%typemap(imtype) db_seq_t "Int64"
%typemap(csin) db_seq_t "$csinput"
%typemap(cstype) db_seq_t * "ref Int64"
%typemap(imtype) db_seq_t * "ref Int64"
%typemap(csin) db_seq_t * "ref $csinput"
%typemap(cstype) u_int8_t [DB_GID_SIZE] "byte[]"
%typemap(imtype) u_int8_t [DB_GID_SIZE] "byte[]"
%typemap(csin) u_int8_t [DB_GID_SIZE] "$csinput"
%typemap(cstype) u_int8_t * "byte[,]"
%typemap(imtype) u_int8_t * "byte[,]"
%typemap(csin) u_int8_t * "$csinput"

%typemap(cstype) DBT * "DatabaseEntry"
%typemap(csin, post="      GC.KeepAlive($csinput);") DBT * "$csclassname.getCPtr(DatabaseEntry.getDBT($csinput))"

%typemap(csfinalize) DB, DB_ENV, DB_SEQUENCE ""
%typemap(csdestruct, methodname="Dispose", methodmodifiers="public") DB, DB_ENV, DB_SEQUENCE %{ {
    lock(this) {
      if(swigCPtr.Handle != IntPtr.Zero && swigCMemOwn) {
        swigCMemOwn = false;
      }
      swigCPtr = new HandleRef(null, IntPtr.Zero);
      GC.SuppressFinalize(this);
    }
} %}

%typemap(csout) int close(u_int32_t flags) {
		int ret = $imcall;
		if (ret == 0)
			/* Close is a db handle destructor.  Reflect that in the wrapper class. */
			swigCPtr = new HandleRef(null, IntPtr.Zero);
		else
			DatabaseException.ThrowException(ret);
		return ret;
} 

%typemap(csout) int get_transactional{
		return $imcall;
}

%typemap(csout) int get_multiple{
		return $imcall;
}

%typemap(csout) char **log_archive {
	IntPtr cPtr = $imcall;
	List<string> ret = new List<string>();
	if (cPtr == IntPtr.Zero)
		return ret;

	IntPtr[] strs = new IntPtr[cntp];
	Marshal.Copy(cPtr, strs, 0, cntp);

	for (int i =0; i < cntp; i++)
		ret.Add(Marshal.PtrToStringAnsi(strs[i]));
	libdb_csharp.__os_ufree(this, cPtr);
	return ret;
}

%typemap(csout) char **get_data_dirs {
	IntPtr cPtr = $imcall;
	List<string> ret = new List<string>();
	if (cPtr == IntPtr.Zero)
		return ret;

	IntPtr[] strs = new IntPtr[cntp];
	Marshal.Copy(cPtr, strs, 0, cntp);

	for (int i =0; i < cntp; i++)
		ret.Add(Marshal.PtrToStringAnsi(strs[i]));

	return ret;
}
		
%typemap(csout) int log_compare{
		return $imcall;
}
%typemap(csout) int log_file{
		return $imcall;
}

%typemap(csout) int open {
	int ret;
	ret = $imcall;
	if (ret != 0)
		close(0);
	DatabaseException.ThrowException(ret);
	return ret;
}

%typemap(csout) int remove {
	int ret;
	ret = $imcall;
	/* 
	 * remove is a handle destructor, regardless of whether the remove
	 * succeeds.  Reflect that in the wrapper class. 
	 */
	swigCPtr = new HandleRef(null, IntPtr.Zero);
	DatabaseException.ThrowException(ret);
	return ret;
}

%typemap(csout) int rename {
	int ret;
	ret = $imcall;
	/* 
	 * rename is a handle destructor, regardless of whether the rename
	 * succeeds.  Reflect that in the wrapper class. 
	 */
	swigCPtr = new HandleRef(null, IntPtr.Zero);
	DatabaseException.ThrowException(ret);
	return ret;
}

%typemap(csout) DB_REPMGR_SITE *repmgr_site_list(u_int *countp, u_int *sizep, int *err) {
	IntPtr cPtr = $imcall;
	if (cPtr == IntPtr.Zero)
		return new RepMgrSite[] { null };
	/*
	 * This is a big kludgy, but we need to free the memory that
	 * repmgr_site_list mallocs.  The RepMgrSite constructors will copy all
	 * the data out of that malloc'd area and we can free it right away.
	 * This is easier than trying to construct a SWIG generated object that
	 * will copy everything in it's constructor, because SWIG generated
	 * classes come with a lot of baggage.
	 */	
	RepMgrSite[] ret = new RepMgrSite[countp];
	for (int i = 0; i < countp; i++) {
		/*
		 * We're copying data out of an array of countp DB_REPMGR_SITE
		 * structures, whose size varies between 32- and 64-bit
		 * platforms.  
		 */
		IntPtr val = new IntPtr((IntPtr.Size == 4 ? cPtr.ToInt32() : cPtr.ToInt64()) + i * sizep);
		ret[i] = new RepMgrSite(new DB_REPMGR_SITE(val, false));
	}
	libdb_csharp.__os_ufree(this, cPtr);
	return ret;
}


%typemap(csout) void *stat {
	return $imcall;
}
%typemap(csout) void *lock_stat(u_int32_t flags, int *err) {
	return $imcall;
}
%typemap(csout) void *log_stat(u_int32_t flags, int *err) {
	return $imcall;
}
%typemap(csout) void *memp_stat(u_int32_t flags, int *err) {
	return $imcall;
}
%typemap(csout) void *mutex_stat(u_int32_t flags, int *err) {
	return $imcall;
}
%typemap(csout) void *repmgr_stat(u_int32_t flags, int *err) {
	return $imcall;
}
%typemap(csout) void *rep_stat(u_int32_t flags, int *err) {
	return $imcall;
}
%typemap(csout) void *txn_stat(u_int32_t flags, uint *size, int *err) {
	return $imcall;
}

%typemap(csout) int verify(const char *file, const char *database, FILE *handle, int (*callback)(void *handle, const void *str), u_int32_t flags) {
		int ret;
		ret = $imcall;
		/* Verify is a db handle destructor.  Reflect that in the wrapper class. */
		swigCMemOwn = false;
		swigCPtr = new HandleRef(null, IntPtr.Zero);
		DatabaseException.ThrowException(ret);
		return ret;
}

%typemap(csout) int {
		int ret;
		ret = $imcall;
		DatabaseException.ThrowException(ret);
		return ret;
}

typedef struct __db_compact {
	u_int32_t	compact_fillpercent;	/* Desired fillfactor: 1-100 */
	db_timeout_t	compact_timeout;	/* Lock timeout. */
	u_int32_t	compact_pages;		/* Max pages to process. */
	/* Output Stats. */
	u_int32_t	compact_pages_free;	/* Number of pages freed. */
	u_int32_t	compact_pages_examine;	/* Number of pages examine. */
	u_int32_t	compact_levels;		/* Number of levels removed. */
	u_int32_t	compact_deadlock;	/* Number of deadlocks. */
	db_pgno_t	compact_pages_truncated; /* Pages truncated to OS. */
	/* Internal. */
	db_pgno_t	compact_truncate;	/* Page number for truncation */
} DB_COMPACT;

typedef struct __db_lsn {
	u_int32_t file;
	u_int32_t offset;
} DB_LSN;

%typemap(cscode) DB %{
	internal DBC cursor(DB_TXN txn, uint flags) {
		int err = 0;
		DBC ret = cursor(txn, flags, ref err);
		DatabaseException.ThrowException(err);
		return ret;
	}
	
	internal DBC join(IntPtr[] curslist, uint flags) {
		int err = 0;
		DBC ret = join(curslist, flags, ref err);
		DatabaseException.ThrowException(err);
		return ret;
	}

	internal BTreeStatStruct stat_bt(DB_TXN txn, uint flags) {
		int err = 0;
		IntPtr ptr = stat(txn, flags, ref err);
		DatabaseException.ThrowException(err);
		BTreeStatStruct ret = (BTreeStatStruct)Marshal.PtrToStructure(ptr, typeof(BTreeStatStruct));
		libdb_csharp.__os_ufree(null, ptr);
		return ret;
	}
	internal HashStatStruct stat_hash(DB_TXN txn, uint flags) {
		int err = 0;
		IntPtr ptr = stat(txn, flags, ref err);
		DatabaseException.ThrowException(err);
		HashStatStruct ret = (HashStatStruct)Marshal.PtrToStructure(ptr, typeof(HashStatStruct));
		libdb_csharp.__os_ufree(null, ptr);
		return ret;
	}
	internal QueueStatStruct stat_qam(DB_TXN txn, uint flags) {
		int err = 0;
		IntPtr ptr = stat(txn, flags, ref err);
		DatabaseException.ThrowException(err);
		QueueStatStruct ret = (QueueStatStruct)Marshal.PtrToStructure(ptr, typeof(QueueStatStruct));
		libdb_csharp.__os_ufree(null, ptr);
		return ret;
	}
%}
typedef struct __db
{
	%typemap(cstype) void *api_internal "BaseDatabase"
	%typemap(imtype) void *api_internal "BaseDatabase"
	%typemap(csin) void *api_internal "value"
	%typemap(csvarout) void *api_internal %{
		get { return $imcall; }
	%}
	void *api_internal;
	
%extend {
	%typemap(cstype) int (*dbt_usercopy)(DBT*, u_int32_t, void *, u_int32_t, u_int32_t) "DBTCopyDelegate"
	%typemap(imtype) int (*dbt_usercopy)(DBT*, u_int32_t, void *, u_int32_t, u_int32_t) "DBTCopyDelegate"
	%typemap(csin) int (*dbt_usercopy)(DBT*, u_int32_t, void *, u_int32_t, u_int32_t) "dbt_usercopy"
	int set_usercopy(int (*dbt_usercopy)(DBT *, u_int32_t, void *, u_int32_t, u_int32_t)) {
		self->env->dbt_usercopy = dbt_usercopy;
		return 0;
	}
	
	DB(DB_ENV *env, u_int32_t flags) {
		DB *self = NULL;
		errno = db_create(&self, env, flags);
		return self;
	}

	~DB() { }

	%typemap(cstype) int (*)(DB*, const DBT*, const DBT*, DBT*) "BDB_AssociateDelegate"
	%typemap(imtype) int (*)(DB*, const DBT*, const DBT*, DBT*) "BDB_AssociateDelegate"
	%typemap(csin) int (*callback)(DB *secondary, const DBT *key, const DBT *data, DBT *result) "callback"
	int associate(DB_TXN *txn, DB *sec, int (*callback)(DB *secondary, const DBT *key, const DBT *data, DBT *result), u_int32_t flags) {
		return self->associate(self, txn, sec, callback, flags);
	}
		
	%typemap(cstype) int (*)(DB*, const DBT*, DBT*, const DBT*, int *) "BDB_AssociateForeignDelegate"
	%typemap(imtype) int (*)(DB*, const DBT*, DBT*, const DBT*, int *) "BDB_AssociateForeignDelegate"
	%typemap(csin) int (*callback)(DB *foreign, const DBT *key, DBT *data, const DBT *result, int *changed) "callback"
	int associate_foreign(DB *dbp, int (*callback)(DB *secondary, const DBT *key, DBT *data, const DBT *foreign, int *changed), u_int32_t flags) {
		return self->associate_foreign(self, dbp, callback, flags);
	}
		
	int close(u_int32_t flags) {
		return self->close(self, flags);
	}
	
	int compact(DB_TXN *txn, DBT *start, DBT *stop, DB_COMPACT *cdata, u_int32_t flags, DBT *end) {
		return self->compact(self, txn, start, stop, cdata, flags, end);
	}
	
	%csmethodmodifiers cursor "private"
	DBC *cursor(DB_TXN *txn, u_int32_t flags, int *err) {
		DBC *cursor = NULL;
		
		*err = self->cursor(self, txn, &cursor, flags);
		return cursor;
	}
	
	int del(DB_TXN *txn, DBT *key, u_int32_t flags) {
		return self->del(self, txn, key, flags);
	}
	
	DB_ENV *env() {
		return self->dbenv;
	}

	int exists(DB_TXN *txn, DBT *key, u_int32_t flags) {
		return self->exists(self, txn, key, flags);
	}
		
	int get(DB_TXN *txn, DBT *key, DBT *data, u_int32_t flags) {
		return self->get(self, txn, key, data, flags);
	}
	
	int get_byteswapped(int *isswapped) {
		return self->get_byteswapped(self, isswapped);
	}
	
	int get_dbname(const char **filenamep, const char **dbnamep) {
		return self->get_dbname(self, filenamep, dbnamep);
	}
	
	int get_multiple() {
		return self->get_multiple(self);
	}

	int get_open_flags(u_int32_t *flags) {
		return self->get_open_flags(self, flags);
	}
	
	int get_transactional() {
		return self->get_transactional(self);
	}
	
	%typemap(cstype) DBTYPE * "ref DBTYPE"
	%typemap(imtype) DBTYPE * "ref DBTYPE"
	%typemap(csin) DBTYPE * "ref $csinput"
	int get_type(DBTYPE *type) {
		return self->get_type(self, type);
	}
	
	%csmethodmodifiers join "private"
	%typemap(cstype) DBC ** "IntPtr[]"
	%typemap(imtype) DBC ** "IntPtr[]"
	%typemap(csin) DBC ** "$csinput"
	DBC *join(DBC **curslist, u_int32_t flags, int *err) {
		DBC *dbc = NULL;
		
		*err = self->join(self, curslist, &dbc, flags);
		return dbc;
	}
	
	int key_range(DB_TXN *txn, DBT *key, DB_KEY_RANGE *range, u_int32_t flags) {
		return self->key_range(self, txn, key, range, flags);
	}
		
	int open(DB_TXN *txn, const char *file, const char *database, DBTYPE type, u_int32_t flags, int mode) {
		return self->open(self, txn, file, database, type, flags, mode);
	}
	
	int pget(DB_TXN *txn, DBT *key, DBT *pkey, DBT *data, u_int32_t flags) {
		return self->pget(self, txn, key, pkey, data, flags);
	}

	int put(DB_TXN *txn, DBT *key, DBT *data, u_int32_t flags) {
		return self->put(self, txn, key, data, flags);
	}
	
	int remove(const char *file, const char *database, u_int32_t flags) {
		return self->remove(self, file, database, flags);
	}
	
	int rename(const char *file, const char *database, const char *newname, u_int32_t flags) {
		return self->rename(self, file, database, newname, flags);
	}
	
	%typemap(cstype) int (*)(DB*, DBT*, db_recno_t) "BDB_AppendRecnoDelegate"
	%typemap(imtype) int (*)(DB*, DBT*, db_recno_t) "BDB_AppendRecnoDelegate"
	%typemap(csin) int (*callback)(DB *dbp, DBT*, db_recno_t) "callback"
	int set_append_recno(int (*callback)(DB *dbp, DBT*, db_recno_t)) {
		return self->set_append_recno(self, callback);
	}
	
	%typemap(cstype) int (*)(DB*, const DBT*, const DBT*) "BDB_CompareDelegate"
	%typemap(imtype) int (*)(DB*, const DBT*, const DBT*) "BDB_CompareDelegate"
	%typemap(csin) int (*callback)(DB *dbp, const DBT *dbt1, const DBT *dbt2) "callback"
	int set_bt_compare(int (*callback)(DB *dbp, const DBT *dbt1, const DBT *dbt2)) {
		return self->set_bt_compare(self, callback);
	}
	
	%typemap(cstype) int (*)(DB *dbp, const DBT *prevKey, const DBT *prevData, const DBT *key, const DBT *data, DBT *dest) "BDB_CompressDelegate"
	%typemap(imtype) int (*)(DB *dbp, const DBT *prevKey, const DBT *prevData, const DBT *key, const DBT *data, DBT *dest) "BDB_CompressDelegate"
	%typemap(csin) int (*compress)(DB *dbp, const DBT *prevKey, const DBT *prevData, const DBT *key, const DBT *data, DBT *dest) "compress"
	%typemap(cstype) int (*)(DB *dbp, const DBT *prevKey, const DBT *prevData, DBT *compressed, DBT *destKey, DBT *destData) "BDB_DecompressDelegate"
	%typemap(imtype) int (*)(DB *dbp, const DBT *prevKey, const DBT *prevData, DBT *compressed, DBT *destKey, DBT *destData) "BDB_DecompressDelegate"
	%typemap(csin) int (*decompress)(DB *dbp, const DBT *prevKey, const DBT *prevData, DBT *compressed, DBT *destKey, DBT *destData) "decompress"
	int set_bt_compress(int (*compress)(DB *dbp, const DBT *prevKey, const DBT *prevData, const DBT *key, const DBT *data, DBT *dest), int (*decompress)(DB *dbp, const DBT *prevKey, const DBT *prevData, DBT *compressed, DBT *destKey, DBT *destData)) {
		return self->set_bt_compress(self, compress, decompress);
	}

	int get_bt_minkey(u_int32_t *bt_minkey) {
		return self->get_bt_minkey(self, bt_minkey);
	}
	int set_bt_minkey(u_int32_t bt_minkey) {
		return self->set_bt_minkey(self, bt_minkey);
	}
	
	int set_bt_prefix(int (*callback)(DB *dbp, const DBT *dbt1, const DBT *dbt2)) {
		return self->set_bt_prefix(self, callback);
	}
	
	int get_cachesize(u_int32_t *gbytes, u_int32_t *bytes, int *ncache) {
		return self->get_cachesize(self, gbytes, bytes, ncache);
	}
	int set_cachesize(u_int32_t gbytes, u_int32_t bytes, int ncache) {
		return self->set_cachesize(self, gbytes, bytes, ncache);
	}
	
	int set_dup_compare(int (*callback)(DB *dbp, const DBT *dbt1, const DBT *dbt2)) {
		return self->set_dup_compare(self, callback);
	}
	
	int get_encrypt_flags(u_int32_t *flags) {
		return self->get_encrypt_flags(self, flags);
	}
	int set_encrypt(const char *pwd, u_int32_t flags) {
		return self->set_encrypt(self, pwd, flags);
	}
	
	%typemap(cstype) void (*)(const DB_ENV *, const char *, const char *) "BDB_ErrcallDelegate"
	%typemap(imtype) void (*)(const DB_ENV *, const char *, const char *) "BDB_ErrcallDelegate"
	%typemap(csin) void (*db_errcall_fcn)(const DB_ENV *dbenv, const char *errpfx, const char *errmsg) "db_errcall_fcn"
	void set_errcall(void (*db_errcall_fcn)(const DB_ENV *dbenv, const char *errpfx, const char *errmsg)) {
		self->set_errcall(self, db_errcall_fcn);
	}
	
	%typemap(cstype) int (*)(DB*, int, int) "BDB_DbFeedbackDelegate"
	%typemap(imtype) int (*)(DB*, int, int) "BDB_DbFeedbackDelegate"
	%typemap(csin) int (*callback)(DB *dbp, int opcode, int percent) "callback"
	int set_feedback(int (*callback)(DB *dbp, int opcode, int percent)) {
		return self->set_feedback(self, callback);
	}
	
	int get_flags(u_int32_t *flags) {
		return self->get_flags(self, flags);
	}
	int set_flags(u_int32_t flags) {
		return self->set_flags(self, flags);
	}
	
	int set_h_compare(int (*callback)(DB *dbp, const DBT *dbt1, const DBT *dbt2)) {
		return self->set_h_compare(self, callback);
	}
	
	int get_h_ffactor(u_int32_t *ffactor) {
		return self->get_h_ffactor(self, ffactor);
	}
	int set_h_ffactor(u_int32_t ffactor) {
		return self->set_h_ffactor(self, ffactor);
	}
	
	%typemap(cstype) u_int32_t (*)(DB*, const void*, u_int32_t) "BDB_HashDelegate"
	%typemap(imtype) u_int32_t (*)(DB*, const void*, u_int32_t) "BDB_HashDelegate"
	%typemap(csin) u_int32_t (*callback)(DB *dbp, const void *bytes, u_int32_t length) "callback"
	int set_h_hash(u_int32_t (*callback)(DB *dbp, const void *bytes, u_int32_t length)) {
		return self->set_h_hash(self, callback);
	}
	
	int get_h_nelem(u_int32_t *nelem) {
		return self->get_h_nelem(self, nelem);
	}
	int set_h_nelem(u_int32_t nelem) {
		return self->set_h_nelem(self, nelem);
	}
	
	int get_lorder(int *lorder) {
		return self->get_lorder(self, lorder);
	}
	int set_lorder(int lorder) {
		return self->set_lorder(self, lorder);
	}
	
	int get_pagesize(u_int32_t *pgsz) {
		return self->get_pagesize(self, pgsz);
	}
	int set_pagesize(u_int32_t pgsz) {
		return self->set_pagesize(self, pgsz);
	}
	
	int get_priority(DB_CACHE_PRIORITY *flags) {
		return self->get_priority(self, flags);
	}
	int set_priority(DB_CACHE_PRIORITY flags) {
		return self->set_priority(self, flags);
	}
	
	int get_q_extentsize(u_int32_t *extentsz) {
		return self->get_q_extentsize(self, extentsz);
	}
	int set_q_extentsize(u_int32_t extentsz) {
		return self->set_q_extentsize(self, extentsz);
	}
	
	int get_re_delim(int *delim) {
		return self->get_re_delim(self, delim);
	}
	int set_re_delim(int delim) {
		return self->set_re_delim(self, delim);
	}
	
	int get_re_len(u_int32_t *len) {
		return self->get_re_len(self, len);
	}
	int set_re_len(u_int32_t len) {
		return self->set_re_len(self, len);
	}
	
	int get_re_pad(int *pad) {
		return self->get_re_pad(self, pad);
	}
	int set_re_pad(int pad) {
		return self->set_re_pad(self, pad);
	}
	
	int get_re_source(const char **source) {
		return self->get_re_source(self, source);
	}
	int set_re_source(char *source) {
		return self->set_re_source(self, source);
	}
	
	%csmethodmodifiers stat "private"
	%typemap(cstype, out="IntPtr") void * "IntPtr"
	void *stat(DB_TXN *txnid, u_int32_t flags, int *err) {
		void *ret = NULL;

		*err = self->stat(self, txnid, &ret, flags);
		return ret;
	}

	int stat_print(u_int32_t flags) {
		return self->stat_print(self, flags);
	}

	int sync(u_int32_t flags) {
		return self->sync(self, flags);
	}
	
	int truncate(DB_TXN *txn, u_int32_t *countp, u_int32_t flags) {
		return self->truncate(self, txn, countp, flags);
	}
	
	int upgrade(const char *file, u_int32_t flags) {
		return self->upgrade(self, file, flags);
	}
	
	%typemap(cstype) FILE * "System.IO.TextWriter"
	%typemap(imtype) FILE * "System.IO.TextWriter"
	%typemap(csin) FILE * "$csinput"
	%typemap(cstype) int (*)(void*, const void*) "BDB_FileWriteDelegate"
	%typemap(imtype) int (*)(void*, const void*) "BDB_FileWriteDelegate"
	%typemap(csin) int (*callback)(void *handle, const void *str) "callback"
	int verify(const char *file, const char *database, FILE *handle, int (*callback)(void *handle, const void *str), u_int32_t flags) {
		/*
		 * We can't easily #include "dbinc/db_ext.h" because of name
		 * clashes, so we declare this explicitly.
		 */
		extern int __db_verify_internal __P((DB *, const char *, const
		    char *, void *, int (*)(void *, const void *), u_int32_t));
		return __db_verify_internal(self, file, database, (void *)handle, callback, flags);
	}
}
} DB;

%typemap(cscode) DBC %{
	internal DBC dup(uint flags) {
		int err = 0;
		DBC ret = dup(flags, ref err);
		DatabaseException.ThrowException(err);
		return ret;
	}
%}
typedef struct __dbc
{
%extend {
	int close() {
		return self->close(self);
	}
	
	%typemap(cstype) db_recno_t * "ref uint"
	%typemap(imtype) db_recno_t * "ref uint"
	%typemap(csin) db_recno_t * "ref $csinput"
	
	int cmp(DBC *other_dbc, int *result, u_int32_t flags) {
		return self->cmp(self, other_dbc, result, flags);
	}

	int count(db_recno_t *cnt, u_int32_t flags) {
		return self->count(self, cnt, flags);
	}

	int del(u_int32_t flags) {
		return self->del(self, flags);
	}
	
	%csmethodmodifiers dup "private"
	DBC *dup(u_int32_t flags, int *err) {
		DBC *cursor = NULL;
		
		*err = self->dup(self, &cursor, flags);
		return cursor;
	}
	
	int get(DBT *key, DBT *data, u_int32_t flags) {
		return self->get(self, key, data, flags);
	}
	
	int pget(DBT *key, DBT *pkey, DBT *data, u_int32_t flags) {
		return self->pget(self, key, pkey, data, flags);
	}
	
	int put(DBT *key, DBT *data, u_int32_t flags) {
		return self->put(self, key, data, flags);
	}
	
	int set_priority(u_int32_t priority) {
		return self->set_priority(self, priority);
	}
		
}
} DBC;

%typemap(cscode) DBT %{
	internal IntPtr dataPtr {
		get {
			return $imclassname.DBT_data_get(swigCPtr);
		}
	}
%}
typedef struct __dbt
{
	u_int32_t dlen;
	u_int32_t doff;
	u_int32_t flags;
	u_int32_t size;
	u_int32_t ulen;
	
%typemap(cstype) void * "byte[]"
%typemap(imtype) void * "IntPtr"
%typemap(csvarin) void *data %{
	set {
		IntPtr _data = Marshal.AllocHGlobal(value.Length);
        Marshal.Copy(value, 0, _data, value.Length);
        $imclassname.DBT_data_set(swigCPtr, _data);
        size = (uint)value.Length;
	}
%}
%typemap(csvarout) void *data %{
	get {
		IntPtr datap = $imcall;
		int sz = (int)size;
		byte[] ret = new byte[sz];
		Marshal.Copy(datap, ret, 0, sz);
		return ret;
	}
%}
	void *data;
	
	%typemap(cstype) void *app_data "DatabaseEntry"
	%typemap(imtype) void *app_data "DatabaseEntry"
	%typemap(csin) void *app_data "value"
	%typemap(csvarout) void *app_data %{
		get { return $imcall; }
	%}
	void *app_data;
	
%extend {
	%typemap(csconstruct) DBT %{: this($imcall, true) { 
		flags = DbConstants.DB_DBT_USERCOPY;
	} %}
	
	DBT() {
		DBT *self = NULL;
		self = malloc(sizeof(DBT));
		memset(self, 0, sizeof(DBT));
		return self;
	}
}
} DBT;

typedef struct __db_repmgrsite
{
	int eid;
	char host[];
	u_int port;
	u_int32_t status;
} DB_REPMGR_SITE;

typedef struct __dbtxn
{
%extend {
	int abort() {
		return self->abort(self);
	}
	
	int commit(u_int32_t flags) {
		return self->commit(self, flags);
	}
	
	int discard(u_int32_t flags) {
		return self->discard(self, flags);
	}
	
	u_int32_t id() {
		return self->id(self);
	}
	
	int prepare(u_int8_t globalID[DB_GID_SIZE]) {
		return self->prepare(self, globalID);
	}
	
	int get_name(const char **name) {
		return self->get_name(self, name);
	}
	int set_name(const char *name) {
		return self->set_name(self, name);
	}

	int set_timeout(db_timeout_t timeout, u_int32_t flags) {
		return self->set_timeout(self, timeout, flags);
	}
}
} DB_TXN;

%typemap(cscode) DB_ENV %{
	internal DB_TXN cdsgroup_begin() {
		int err = 0;
		DB_TXN ret = cdsgroup_begin(ref err);
		DatabaseException.ThrowException(err);
		return ret;
	}
	internal List<string> get_data_dirs() {
		int err = 0;
		int cnt = 0;
		List<string> ret = get_data_dirs(ref err, ref cnt);
		DatabaseException.ThrowException(err);
		return ret;
	}
	internal DB_LOCK lock_get(uint locker, uint flags, DBT arg2, db_lockmode_t mode) {
		int err = 0;
		DB_LOCK ret = lock_get(locker, flags, arg2, mode, ref err);
		DatabaseException.ThrowException(err);
		return ret;
	}
	internal LockStatStruct lock_stat(uint flags) {
		int err = 0;
		IntPtr ptr = lock_stat(flags, ref err);
		DatabaseException.ThrowException(err);
		LockStatStruct ret = (LockStatStruct)Marshal.PtrToStructure(ptr, typeof(LockStatStruct));
		libdb_csharp.__os_ufree(null, ptr);
		return ret;
	}
	internal List<string> log_archive(uint flags) {
		int err = 0;
		int cnt = 0;
		List<string> ret = log_archive(flags, ref err, ref cnt);
		DatabaseException.ThrowException(err);
		return ret;
	}
	internal string log_file(DB_LSN dblsn) {
		int err = 0;
		int len = 100;
		IntPtr namep;
		while (true) {
			namep = Marshal.AllocHGlobal(len);
			err = log_file(dblsn, namep, (uint)len);
			if (err != DbConstants.DB_BUFFER_SMALL)
				break;
			Marshal.FreeHGlobal(namep);
			len *= 2;
		}
		DatabaseException.ThrowException(err);
		string ret = Marshal.PtrToStringAnsi(namep);
		Marshal.FreeHGlobal(namep);
		return ret;
	}
	internal LogStatStruct log_stat(uint flags) {
		int err = 0;
		IntPtr ptr = log_stat(flags, ref err);
		DatabaseException.ThrowException(err);
		LogStatStruct ret = (LogStatStruct)Marshal.PtrToStructure(ptr, typeof(LogStatStruct));
		libdb_csharp.__os_ufree(null, ptr);
		return ret;
	}
	internal MempStatStruct memp_stat(uint flags) {
		int err = 0;
	        int cnt = 0;
        	IntPtr mpf = new IntPtr();
	        IntPtr ptr = memp_stat(ref mpf, flags, ref err, ref cnt);
		DatabaseException.ThrowException(err);
        	IntPtr[] files = new IntPtr[cnt];
		if (cnt > 0)
	        	Marshal.Copy(mpf, files, 0, cnt);

	        MempStatStruct ret = new MempStatStruct();
		ret.st = (MPoolStatStruct)Marshal.PtrToStructure(ptr, typeof(MPoolStatStruct));
        	ret.files = new MPoolFileStatStruct[cnt];
	        for (int i = 0; i < cnt; i++)
        	    ret.files[i] = (MPoolFileStatStruct)Marshal.PtrToStructure(files[i], typeof(MPoolFileStatStruct));

		libdb_csharp.__os_ufree(null, ptr);
		libdb_csharp.__os_ufree(null, mpf);
		return ret;
	}
	internal MutexStatStruct mutex_stat(uint flags) {
		int err = 0;
		IntPtr ptr = mutex_stat(flags, ref err);
		DatabaseException.ThrowException(err);
		MutexStatStruct ret = (MutexStatStruct)Marshal.PtrToStructure(ptr, typeof(MutexStatStruct));
		libdb_csharp.__os_ufree(null, ptr);
		return ret;
	}
	internal RepMgrSite[] repmgr_site_list() {
		uint count = 0;
		int err = 0;
		uint size = 0;
		RepMgrSite[] ret = repmgr_site_list(ref count, ref size, ref err);
		DatabaseException.ThrowException(err);
		return ret;
	}
	internal RepMgrStatStruct repmgr_stat(uint flags) {
		int err = 0;
		IntPtr ptr = repmgr_stat(flags, ref err);
		DatabaseException.ThrowException(err);
		RepMgrStatStruct ret = (RepMgrStatStruct)Marshal.PtrToStructure(ptr, typeof(RepMgrStatStruct));
		libdb_csharp.__os_ufree(null, ptr);
		return ret;
	}
	internal ReplicationStatStruct rep_stat(uint flags) {
		int err = 0;
		IntPtr ptr = rep_stat(flags, ref err);
		DatabaseException.ThrowException(err);
		ReplicationStatStruct ret = (ReplicationStatStruct)Marshal.PtrToStructure(ptr, typeof(ReplicationStatStruct));
		libdb_csharp.__os_ufree(null, ptr);
		return ret;
	}
	internal DB_TXN txn_begin(DB_TXN parent, uint flags) {
		int err = 0;
		DB_TXN ret = txn_begin(parent, flags, ref err);
		DatabaseException.ThrowException(err);
		return ret;
	}
	internal PreparedTransaction[] txn_recover(uint count, uint flags) {
		int err = 0;
		IntPtr prepp = Marshal.AllocHGlobal((int)(count * (IntPtr.Size + DbConstants.DB_GID_SIZE)));
		uint sz = 0;
		err = txn_recover(prepp, count, ref sz, flags);
		DatabaseException.ThrowException(err);
		PreparedTransaction[] ret = new PreparedTransaction[sz];
		for (int i = 0; i < sz; i++) {
			IntPtr cPtr = new IntPtr((IntPtr.Size == 4 ? prepp.ToInt32() : prepp.ToInt64()) + i * (IntPtr.Size + DbConstants.DB_GID_SIZE));
			DB_PREPLIST prep = new DB_PREPLIST(cPtr, false);
			ret[i] = new PreparedTransaction(prep);
		}
		Marshal.FreeHGlobal(prepp);
		return ret;
	}
	internal TxnStatStruct txn_stat(uint flags) {
		int err = 0;
		uint size = 0;
		int offset = Marshal.SizeOf(typeof(DB_TXN_ACTIVE));
		IntPtr ptr = txn_stat(flags, ref size, ref err);
		DatabaseException.ThrowException(err);
	        TxnStatStruct ret = new TxnStatStruct();
		ret.st = (TransactionStatStruct)Marshal.PtrToStructure(ptr, typeof(TransactionStatStruct));
        	ret.st_txnarray = new DB_TXN_ACTIVE[ret.st.st_nactive];
	        ret.st_txngids = new byte[ret.st.st_nactive][];
        	ret.st_txnnames = new string[ret.st.st_nactive];

	        for (int i = 0; i < ret.st.st_nactive; i++) {
        	    IntPtr activep = new IntPtr((IntPtr.Size == 4 ? ret.st.st_txnarray.ToInt32() : ret.st.st_txnarray.ToInt64()) + i * size);
	            ret.st_txnarray[i] = (DB_TXN_ACTIVE)Marshal.PtrToStructure(activep, typeof(DB_TXN_ACTIVE));
        	    ret.st_txngids[i] = new byte[DbConstants.DB_GID_SIZE];
	            IntPtr gidp = new IntPtr((IntPtr.Size == 4 ? activep.ToInt32() : activep.ToInt64()) + offset);
        	    Marshal.Copy(gidp, ret.st_txngids[i], 0, (int)DbConstants.DB_GID_SIZE);
	            IntPtr namep = new IntPtr((IntPtr.Size == 4 ? gidp.ToInt32() : gidp.ToInt64()) + DbConstants.DB_GID_SIZE);
        	    ret.st_txnnames[i] = Marshal.PtrToStringAnsi(namep);
	        }
        	libdb_csharp.__os_ufree(null, ptr);
        
		return ret;
	}
	
%}
%typemap(csimports) DB_ENV "using System;
using System.Runtime.InteropServices;
using System.Collections.Generic;"		
typedef struct __dbenv
{

%typemap(cstype) void *api2_internal "DatabaseEnvironment"
%typemap(imtype) void *api2_internal "DatabaseEnvironment"
%typemap(csin) void *api2_internal "value"
%typemap(csvarout) void *api2_internal %{
		get { return $imcall; }
%}
	void *api2_internal;
	
%extend {
	%typemap(cstype) int (*dbt_usercopy)(DBT*, u_int32_t, void *, u_int32_t, u_int32_t) "DBTCopyDelegate"
	%typemap(imtype) int (*dbt_usercopy)(DBT*, u_int32_t, void *, u_int32_t, u_int32_t) "DBTCopyDelegate"
	%typemap(csin) int (*dbt_usercopy)(DBT*, u_int32_t, void *, u_int32_t, u_int32_t) "dbt_usercopy"
	int set_usercopy(int (*dbt_usercopy)(DBT *, u_int32_t, void *, u_int32_t, u_int32_t)) {
		self->env->dbt_usercopy = dbt_usercopy;
		return 0;
	}
	
	DB_ENV(u_int32_t flags) {
		DB_ENV *self = NULL;
		errno = db_env_create(&self, flags);
		return self;
	}
	
	~DB_ENV() { }

	%csmethodmodifiers cdsgroup_begin "private"
	DB_TXN *cdsgroup_begin(int *err) {
		DB_TXN *group;
		
		*err = self->cdsgroup_begin(self, &group);
		return group;
	}
	 
	int close(u_int32_t flags) {
		return self->close(self, flags);
	}
	
	int dbremove(DB_TXN *txn, const char *file, const char *database, u_int32_t flags) {
		return self->dbremove(self, txn, file, database, flags);
	}
	
	int dbrename(DB_TXN *txn, const char *file, const char *database, const char *newname, u_int32_t flags) {
		return self->dbrename(self, txn, file, database, newname, flags);
	}
	
	int failchk(u_int32_t flags) {
		return self->failchk(self, flags);
	}
	
	int fileid_reset(const char *file, u_int32_t flags) {
		return self->fileid_reset(self, file, flags);
	}
	
	int get_home(const char **file) {
		return self->get_home(self, file);
	}
	
	int lock_detect(u_int32_t flags, u_int32_t atype, u_int32_t *rejected) {
		return self->lock_detect(self, flags, atype, rejected);
	}
	
	%csmethodmodifiers lock_get "private"
	DB_LOCK lock_get(u_int32_t locker, u_int32_t flags, DBT *object, const db_lockmode_t mode, int *err) {
		DB_LOCK lock;
		
		*err = self->lock_get(self, locker, flags, object, mode, &lock);
		return lock;
	}
	
	int lock_id(u_int32_t *id) {
		return self->lock_id(self, id);
	}
		
	int lock_id_free(u_int32_t id) {
		return self->lock_id_free(self, id);
	}
	
	int lock_put(DB_LOCK *lck) {
		return self->lock_put(self, lck);
	}

	%typemap(cstype, out="IntPtr") void * "IntPtr"
	%csmethodmodifiers lock_stat "private"		
	void *lock_stat(u_int32_t flags, int *err) {
		DB_LOCK_STAT *ret;
		*err = self->lock_stat(self, &ret, flags);
		return (void *)ret;
	}

	int lock_stat_print(u_int32_t flags) {
		return self->lock_stat_print(self, flags);
	}


	%typemap(cstype) DB_LOCKREQ ** "IntPtr[]"
	%typemap(imtype) DB_LOCKREQ ** "IntPtr[]"
	%typemap(csin) DB_LOCKREQ ** "$csinput"
	int lock_vec(u_int32_t locker, u_int32_t flags, DB_LOCKREQ **list, int nlist, DB_LOCKREQ *elistp){
		int i, ret;
		DB_LOCKREQ *vec;
		
		ret = __os_malloc(self->env, sizeof(DB_LOCKREQ) * nlist, &vec);
		if (ret != 0)
			return ENOMEM;
		for (i = 0; i < nlist; i++)
			vec[i] = *list[i];
			
		if (elistp == NULL)
			ret = self->lock_vec(self, locker, flags, vec, nlist, NULL);
		else
			ret = self->lock_vec(self, locker, flags, vec, nlist, &elistp);
			
		for (i = 0; i < nlist; i++)
			*list[i] = vec[i];
		
		__os_free(self->env , vec);
		
		return ret;
	}

	%typemap(cstype) char ** "List<string>"
	%typemap(imtype) char ** "IntPtr"
	%typemap(csin) char ** "$csinput"
	char **log_archive(u_int32_t flags, int *err, int *cntp) {
		char **list = NULL;
		int cnt = 0;
		*err = self->log_archive(self, &list, flags);

		if (list != NULL)
			while (list[cnt] != NULL) {
				cnt++;
			}
		*cntp = cnt;
		return list;
	}
	%typemap(cstype) char ** "ref string"
	%typemap(imtype) char ** "ref string"
	%typemap(csin) char ** "ref $csinput"

	%typemap(cstype) char *namep "IntPtr"
	%typemap(imtype) char *namep "IntPtr"
	%csmethodmodifiers log_file "private"
	int log_file(const DB_LSN *lsn, char *namep, size_t len) {
		int ret = self->log_file(self, lsn, namep, len);
		if (ret == EINVAL)
			return DB_BUFFER_SMALL;
		return ret;
	}
	
	int log_flush(const DB_LSN *lsn) {
		return self->log_flush(self, lsn);
	}
	
	int log_put(DB_LSN *lsn, DBT *data, u_int32_t flags) {
		return self->log_put(self, lsn, data, flags);
	}
	
	int log_get_config(u_int32_t which, int *onoff) {
		return self->log_get_config(self, which, onoff);
	}
	int log_set_config(u_int32_t which, int onoff) {
		return self->log_set_config(self, which, onoff);
	}
	
	int log_printf(DB_TXN *txn, const char *str) {
		return self->log_printf(self, txn, "%s", str);
	}
	
	%typemap(cstype, out="IntPtr") void * "IntPtr"
	%csmethodmodifiers log_stat "private"		
	void *log_stat(u_int32_t flags, int *err) {
		DB_LOG_STAT *ret;
		*err = self->log_stat(self, &ret, flags);
		return (void *)ret;
	}

	int log_stat_print(u_int32_t flags) {
		return self->log_stat_print(self, flags);
	}


	int lsn_reset(const char *file, u_int32_t flags) {
		return self->lsn_reset(self, file, flags);
	}
	
	%typemap(cstype, out="IntPtr") void * "IntPtr"
	%typemap(cstype, out="ref IntPtr") void *** "ref IntPtr"
	%typemap(csin) void *** "ref $csinput"
	%typemap(imtype, out="ref IntPtr") void *** "ref IntPtr"
	%csmethodmodifiers memp_stat "private"		
	void *memp_stat(void ***fstatp, u_int32_t flags, int *err, int *cntp) {
		DB_MPOOL_STAT *ret;
		DB_MPOOL_FSTAT **fptr;
		int cnt;

		*err = self->memp_stat(self, &ret, &fptr, flags);
		cnt = 0;
		if (fptr != NULL)
			while (fptr[cnt] != NULL) {
				cnt++;
			}

		*cntp = cnt;
		*fstatp = (void **)fptr;
		return (void *)ret;
	}

	int memp_stat_print(u_int32_t flags) {
		return self->memp_stat_print(self, flags);
	}

	int memp_sync(DB_LSN *lsn) {
		return self->memp_sync(self, lsn);
	}
	
	int memp_trickle(int percent, int *nwrotep) {
		return self->memp_trickle(self, percent, nwrotep);
	}
	
	int mutex_alloc(u_int32_t flags, db_mutex_t *mutexp) {
		return self->mutex_alloc(self, flags, mutexp);
	}
	
	int mutex_free(db_mutex_t mutex) {
		return self->mutex_free(self, mutex);
	}
	
	int mutex_lock(db_mutex_t mutex) {
		return self->mutex_lock(self, mutex);
	}
	
	%typemap(cstype, out="IntPtr") void * "IntPtr"
	%csmethodmodifiers mutex_stat "private"		
	void *mutex_stat(u_int32_t flags, int *err) {
		DB_MUTEX_STAT *ret;
		*err = self->mutex_stat(self, &ret, flags);
		return (void *)ret;
	}

	int mutex_stat_print(u_int32_t flags) {
		return self->mutex_stat_print(self, flags);
	}

	int mutex_unlock(db_mutex_t mutex) {
		return self->mutex_unlock(self, mutex);
	}
	
	int mutex_get_align(u_int32_t *align) {
		return self->mutex_get_align(self, align);
	}
	int mutex_set_align(u_int32_t align) {
		return self->mutex_set_align(self, align);
	}
	
	int mutex_get_increment(u_int32_t *increment) {
		return self->mutex_get_increment(self, increment);
	}
	int mutex_set_increment(u_int32_t increment) {
		return self->mutex_set_increment(self, increment);
	}
	
	int mutex_get_max(u_int32_t *max) {
		return self->mutex_get_max(self, max);
	}
	int mutex_set_max(u_int32_t max) {
		return self->mutex_set_max(self, max);
	}
	
	int mutex_get_tas_spins(u_int32_t *tas_spins) {
		return self->mutex_get_tas_spins(self, tas_spins);
	}
	int mutex_set_tas_spins(u_int32_t tas_spins) {
		return self->mutex_set_tas_spins(self, tas_spins);
	}
	
	int open(const char *home, u_int32_t flags, int mode) {
		return self->open(self, home, flags, mode);
	}

	int get_open_flags(u_int32_t *flags) {
		return self->get_open_flags(self, flags);
	}
	
	int remove(char *db_home, u_int32_t flags) {
		return self->remove(self, db_home, flags);
	}
	
	int repmgr_add_remote_site(const char *host, u_int port, int *eidp, u_int32_t flags) {
		return self->repmgr_add_remote_site(self, host, port, eidp, flags);
	}

	int repmgr_set_ack_policy(int ack_policy) {
		return self->repmgr_set_ack_policy(self, ack_policy);
	}
	int repmgr_get_ack_policy(int *ack_policy) {
		return self->repmgr_get_ack_policy(self, ack_policy);
	}

	int repmgr_set_local_site(const char *host, u_int port, u_int32_t flags) {
		return self->repmgr_set_local_site(self, host, port, flags);
	}

	%typemap(cstype) DB_REPMGR_SITE * "RepMgrSite[]"
	%typemap(imtype) DB_REPMGR_SITE * "IntPtr"
	%csmethodmodifiers repmgr_site_list "private"
	DB_REPMGR_SITE *repmgr_site_list(u_int *countp, u_int32_t *sizep, int *err) {
		DB_REPMGR_SITE *listp = NULL;
	
		*err = self->repmgr_site_list(self, countp, &listp);
		*sizep = sizeof(DB_REPMGR_SITE);
		return listp;
	}

	int repmgr_start(int nthreads, u_int32_t flags) {
		return self->repmgr_start(self, nthreads, flags);
	}

	%csmethodmodifiers repmgr_stat "private"		
	%typemap(cstype, out="IntPtr") void * "IntPtr"
	void *repmgr_stat(u_int32_t flags, int *err) {
		DB_REPMGR_STAT *ret;
		*err = self->repmgr_stat(self, &ret, flags);
		return (void *)ret;
	}

	int repmgr_stat_print(u_int32_t flags) {
		return self->repmgr_stat_print(self, flags);
	}

	int rep_elect(u_int32_t nsites, u_int32_t nvotes, u_int32_t flags) {
		return self->rep_elect(self, nsites, nvotes, flags);
	}

	int rep_process_message(DBT *control, DBT *rec, int envid, DB_LSN *ret_lsnp) {
		return self->rep_process_message(self, control, rec, envid, ret_lsnp);
	}

	int rep_start(DBT *cdata, u_int32_t flags) {
		return self->rep_start(self, cdata, flags);
	}

	%csmethodmodifiers rep_stat "private"		
	%typemap(cstype, out="IntPtr") void * "IntPtr"
	void *rep_stat(u_int32_t flags, int *err) {
		DB_REP_STAT *ret;
		*err = self->rep_stat(self, &ret, flags);
		return (void *)ret;
	}

	int rep_stat_print(u_int32_t flags) {
		return self->rep_stat_print(self, flags);
	}

	int rep_sync(u_int32_t flags) {
		return self->rep_sync(self, flags);
	}

	int rep_set_config(u_int32_t which, int onoff) {
		return self->rep_set_config(self, which, onoff);
	}
	int rep_get_config(u_int32_t which, int *onoffp) {
		return self->rep_get_config(self, which, onoffp);
	}

	int rep_set_clockskew(u_int32_t fast_clock, u_int32_t slow_clock) {
		return self->rep_set_clockskew(self, fast_clock, slow_clock);
	}
	int rep_get_clockskew(u_int32_t *fast_clockp, u_int32_t *slow_clockp) {
		return self->rep_get_clockskew(self, fast_clockp, slow_clockp);
	}

	int rep_set_limit(u_int32_t gbytes, u_int32_t bytes) {
		return self->rep_set_limit(self, gbytes, bytes);
	}
	int rep_get_limit(u_int32_t *gbytesp, u_int32_t *bytesp) {
		return self->rep_get_limit(self, gbytesp, bytesp);
	}

	int rep_set_nsites(u_int32_t nsites) {
		return self->rep_set_nsites(self, nsites);
	}
	int rep_get_nsites(u_int32_t *nsitesp) {
		return self->rep_get_nsites(self, nsitesp);
	}

	int rep_set_priority(u_int32_t priority) {
		return self->rep_set_priority(self, priority);
	}
	int rep_get_priority(u_int32_t *priorityp) {
		return self->rep_get_priority(self, priorityp);
	}

	int rep_set_request(u_int32_t min, u_int32_t max) {
		return self->rep_set_request(self, min, max);
	}
	int rep_get_request(u_int32_t *minp, u_int32_t *maxp) {
		return self->rep_get_request(self, minp, maxp);
	}

	int rep_set_timeout(int which, u_int32_t timeout) {
		return self->rep_set_timeout(self, which, timeout);
	}
	int rep_get_timeout(int which, u_int32_t *timeoutp) {
		return self->rep_get_timeout(self, which, timeoutp);
	}

	%typemap(cstype) int (*)(DB_ENV*, const DBT*, const DBT*, const DB_LSN *, int envid, u_int32_t) "BDB_RepTransportDelegate"
	%typemap(imtype) int (*)(DB_ENV*, const DBT*, const DBT*, const DB_LSN *, int envid, u_int32_t) "BDB_RepTransportDelegate"
	%typemap(csin) int (*send)(DB_ENV *, const DBT*, const DBT*, const DB_LSN*, int, u_int32_t) "send"
	int rep_set_transport(int envid, int (*send)(DB_ENV *dbenv, const DBT *control, const DBT *rec, const DB_LSN *lsnp, int envid, u_int32_t flags)) {
		return self->rep_set_transport(self, envid, send);
	}
	
	int get_cachesize(u_int32_t *gbytes, u_int32_t *bytes, int *ncache) {
		return self->get_cachesize(self, gbytes, bytes, ncache);
	}
	int set_cachesize(u_int32_t gbytes, u_int32_t bytes, int ncache) {
		return self->set_cachesize(self, gbytes, bytes, ncache);
	}
	
	int get_cache_max(u_int32_t *gbytes, u_int32_t *bytes) {
		return self->get_cache_max(self, gbytes, bytes);
	}
	int set_cache_max(u_int32_t gbytes, u_int32_t bytes) {
		return self->set_cache_max(self, gbytes, bytes);
	}
	
	%typemap(cstype) char ** "List<string>"
	%typemap(imtype) char ** "IntPtr"
	%typemap(csin) char ** "$csinput"
	%csmethodmodifiers get_data_dirs "private"
	char **get_data_dirs(int *err, int *cntp) {
		char **list = NULL;
		int cnt = 0;
		*err = self->get_data_dirs(self, &list);

		if (list != NULL)
			while (list[cnt] != NULL) {
				cnt++;
			}
		*cntp = cnt;
		return list;
	}
	
	int add_data_dir(const char *dir) {
		return self->add_data_dir(self, dir);
	}
	
	int set_create_dir(const char *dir) {
		return self->set_create_dir(self, dir);
	}

	int get_encrypt_flags(u_int32_t *flags) {
		return self->get_encrypt_flags(self, flags);
	}
	int set_encrypt(const char *passwd, u_int32_t flags) {
		return self->set_encrypt(self, passwd, flags);
	}
	
	%typemap(cstype) void (*)(const DB_ENV *, const char *, const char *) "BDB_ErrcallDelegate"
	%typemap(imtype) void (*)(const DB_ENV *, const char *, const char *) "BDB_ErrcallDelegate"
	%typemap(csin) void (*db_errcall_fcn)(const DB_ENV *dbenv, const char *errpfx, const char *errmsg) "db_errcall_fcn"
	void set_errcall(void (*db_errcall_fcn)(const DB_ENV *dbenv, const char *errpfx, const char *errmsg)) {
		self->set_errcall(self, db_errcall_fcn);
	}

	%typemap(cstype) void (*)(DB_ENV*, u_int32_t, void*) "BDB_EventNotifyDelegate"
	%typemap(imtype) void (*)(DB_ENV*, u_int32_t, void*) "BDB_EventNotifyDelegate"
	%typemap(csin) void (*callback)(DB_ENV *dbenv, u_int32_t, void*) "callback"
	int set_event_notify(void (*callback)(DB_ENV *env, u_int32_t event, void *event_info)) {
		return self->set_event_notify(self, callback);
	}
	
	%typemap(cstype) void (*)(DB_ENV*, int, int) "BDB_EnvFeedbackDelegate"
	%typemap(imtype) void (*)(DB_ENV*, int, int) "BDB_EnvFeedbackDelegate"
	%typemap(csin) void (*callback)(DB_ENV *dbenv, int opcode, int percent) "callback"
	int set_feedback(void (*callback)(DB_ENV *dbenv, int opcode, int percent)) {
		return self->set_feedback(self, callback);
	}
	
	int get_flags(u_int32_t *flags) {
		return self->get_flags(self, flags);
	}
	int set_flags(u_int32_t flags, int onoff) {
		return self->set_flags(self, flags, onoff);
	}
	
	%typemap(cstype) char ** "ref string"
	%typemap(imtype) char ** "ref string"
	%typemap(csin) char ** "ref $csinput"
	%csmethodmodifiers get_data_dirs "private"
	int get_intermediate_dir_mode(const char **mode) {
		return self->get_intermediate_dir_mode(self, mode);
	}
	int set_intermediate_dir_mode(const char *mode) {
		return self->set_intermediate_dir_mode(self, mode);
	}
	
	%typemap(cstype) int (*)(DB_ENV*, pid_t, db_threadid_t, u_int32_t) "BDB_IsAliveDelegate"
	%typemap(imtype) int (*)(DB_ENV*, pid_t, db_threadid_t, u_int32_t) "BDB_IsAliveDelegate"
	%typemap(csin) int (*callback)(DB_ENV *dbenv, pid_t, db_threadid_t, u_int32_t) "callback"
	int set_isalive(int (*callback)(DB_ENV *dbenv, pid_t pid, db_threadid_t tid, u_int32_t flags)) {
		return self->set_isalive(self, callback);
	}
	
	int get_lg_bsize(u_int32_t *bsize){
		return self->get_lg_bsize(self, bsize);
	}
	int set_lg_bsize(u_int32_t bsize){
		return self->set_lg_bsize(self, bsize);
	}
	int get_lg_dir(const char **dir){
		return self->get_lg_dir(self, dir);
	}
	int set_lg_dir(const char *dir){
		return self->set_lg_dir(self, dir);
	}
	int get_lg_filemode(int *mode){
		return self->get_lg_filemode(self, mode);
	}
	int set_lg_filemode(int mode){
		return self->set_lg_filemode(self, mode);
	}
	int get_lg_max(u_int32_t *max){
		return self->get_lg_max(self, max);
	}
	int set_lg_max(u_int32_t max){
		return self->set_lg_max(self, max);
	}
	int get_lg_regionmax(u_int32_t *max){
		return self->get_lg_regionmax(self, max);
	}
	int set_lg_regionmax(u_int32_t max){
		return self->set_lg_regionmax(self, max);
	}
	
	int get_lk_conflicts_nmodes(int *nmodes) {
		return self->get_lk_conflicts(self, NULL, nmodes);
	}
	
	int get_lk_conflicts(u_int8_t *conflicts) {
		int i, nmodes, ret;
		u_int8_t *mtrx = NULL;
		
		ret = self->get_lk_conflicts(self, &mtrx, &nmodes);
		for (i = 0; i < nmodes * nmodes; i++)
			conflicts[i] = mtrx[i];
		
		return ret;
	}
	int set_lk_conflicts(u_int8_t *conflicts, int nmodes) {
		return self->set_lk_conflicts(self, conflicts, nmodes);
	}
	
	int get_lk_detect(u_int32_t *mode) {
		return self->get_lk_detect(self, mode);
	}
	int set_lk_detect(u_int32_t mode) {
		return self->set_lk_detect(self, mode);
	}
	
	int get_lk_max_locks(u_int32_t *max) {
		return self->get_lk_max_locks(self, max);
	}
	int set_lk_max_locks(u_int32_t max) {
		return self->set_lk_max_locks(self, max);
	}
	
	int get_lk_max_lockers(u_int32_t *max) {
		return self->get_lk_max_lockers(self, max);
	}
	int set_lk_max_lockers(u_int32_t max) {
		return self->set_lk_max_lockers(self, max);
	}
	
	int get_lk_max_objects(u_int32_t *max) {
		return self->get_lk_max_objects(self, max);
	}
	int set_lk_max_objects(u_int32_t max) {
		return self->set_lk_max_objects(self, max);
	}
	
	int get_lk_partitions(u_int32_t *max) {
		return self->get_lk_partitions(self, max);
	}
	int set_lk_partitions(u_int32_t max) {
		return self->set_lk_partitions(self, max);
	}
	
	int get_mp_max_openfd(int *maxopenfd) {
		return self->get_mp_max_openfd(self, maxopenfd);
	}
	int set_mp_max_openfd(int maxopenfd) {
		return self->set_mp_max_openfd(self, maxopenfd);
	}
	
	int get_mp_max_write(int *maxwrite, db_timeout_t *maxwrite_sleep) {
		return self->get_mp_max_write(self, maxwrite, maxwrite_sleep);
	}
	int set_mp_max_write(int maxwrite, db_timeout_t maxwrite_sleep) {
		return self->set_mp_max_write(self, maxwrite, maxwrite_sleep);
	}
	
	int get_mp_mmapsize(size_t *mp_mmapsize) {
		return self->get_mp_mmapsize(self, mp_mmapsize);
	}
	int set_mp_mmapsize(size_t mp_mmapsize) {
		return self->set_mp_mmapsize(self, mp_mmapsize);
	}
	
	int get_thread_count(u_int32_t *count) {
		return self->get_thread_count(self, count);
	}
	int set_thread_count(u_int32_t count) {
		return self->set_thread_count(self, count);
	}
	
	%typemap(cstype) void (*)(DB_ENV*, pid_t*, db_threadid_t*) "BDB_ThreadIDDelegate"
	%typemap(imtype) void (*)(DB_ENV*, pid_t*, db_threadid_t*) "BDB_ThreadIDDelegate"
	%typemap(csin) void (*callback)(DB_ENV *dbenv, pid_t*, db_threadid_t*) "callback"
	int set_thread_id(void (*callback)(DB_ENV *dbenv, pid_t *pid, db_threadid_t *tid)) {
		return self->set_thread_id(self, callback);
	}
	
	%typemap(cstype) char* (*)(DB_ENV*, pid_t, db_threadid_t, char *) "BDB_ThreadNameDelegate"
	%typemap(imtype) char* (*)(DB_ENV*, pid_t, db_threadid_t, char *) "BDB_ThreadNameDelegate"
	%typemap(csin) char *(*callback)(DB_ENV *dbenv, pid_t, db_threadid_t, char *) "callback"
	int set_thread_id_string(char *(*callback)(DB_ENV *dbenv, pid_t pid, db_threadid_t tid, char *buf)) {
		return self->set_thread_id_string(self, callback);
	}

	int get_timeout(db_timeout_t *timeout, u_int32_t flags) {
		return self->get_timeout(self, timeout, flags);
	}
	int set_timeout(db_timeout_t timeout, u_int32_t flags) {
		return self->set_timeout(self, timeout, flags);
	}
	
	int get_tmp_dir(const char **dir) {
		return self->get_tmp_dir(self, dir);
	}
	int set_tmp_dir(const char *dir) {
		return self->set_tmp_dir(self, dir);
	}
	
	int get_tx_max(u_int32_t *max) {
		return self->get_tx_max(self, max);
	}
	int set_tx_max(u_int32_t max) {
		return self->set_tx_max(self, max);
	}
	
	int get_tx_timestamp(time_t *timestamp) {
		return self->get_tx_timestamp(self, timestamp);
	}
	int set_tx_timestamp(time_t *timestamp) {
		return self->set_tx_timestamp(self, timestamp);
	}
	
	int get_verbose(u_int32_t *msgs) {
		int onoff, ret;
		
		*msgs = 0;
		if ((ret = self->get_verbose(self, DB_VERB_DEADLOCK, &onoff)) != 0)
			return ret;
		if (onoff)
			*msgs |= DB_VERB_DEADLOCK;
		if ((ret = self->get_verbose(self, DB_VERB_FILEOPS, &onoff)) != 0)
			return ret;
		if (onoff)
			*msgs |= DB_VERB_FILEOPS;
		if ((ret = self->get_verbose(self, DB_VERB_FILEOPS_ALL, &onoff)) != 0)
			return ret;
		if (onoff)
			*msgs |= DB_VERB_FILEOPS_ALL;
		if ((ret = self->get_verbose(self, DB_VERB_RECOVERY, &onoff)) != 0)
			return ret;
		if (onoff)
			*msgs |= DB_VERB_RECOVERY;
		if ((ret = self->get_verbose(self, DB_VERB_REGISTER, &onoff)) != 0)
			return ret;
		if (onoff)
			*msgs |= DB_VERB_REGISTER;
		if ((ret = self->get_verbose(self, DB_VERB_REPLICATION, &onoff)) != 0)
			return ret;
		if (onoff)
			*msgs |= DB_VERB_REPLICATION;
		if ((ret = self->get_verbose(self, DB_VERB_REP_ELECT, &onoff)) != 0)
			return ret;
		if (onoff)
			*msgs |= DB_VERB_REP_ELECT;
		if ((ret = self->get_verbose(self, DB_VERB_REP_LEASE, &onoff)) != 0)
			return ret;
		if (onoff)
			*msgs |= DB_VERB_REP_LEASE;
		if ((ret = self->get_verbose(self, DB_VERB_REP_MISC, &onoff)) != 0)
			return ret;
		if (onoff)
			*msgs |= DB_VERB_REP_MISC;
		if ((ret = self->get_verbose(self, DB_VERB_REP_MSGS, &onoff)) != 0)
			return ret;
		if (onoff)
			*msgs |= DB_VERB_REP_MSGS;
		if ((ret = self->get_verbose(self, DB_VERB_REP_SYNC, &onoff)) != 0)
			return ret;
		if (onoff)
			*msgs |= DB_VERB_REP_SYNC;
		if ((ret = self->get_verbose(self, DB_VERB_REPMGR_CONNFAIL, &onoff)) != 0)
			return ret;
		if (onoff)
			*msgs |= DB_VERB_REPMGR_CONNFAIL;
		if ((ret = self->get_verbose(self, DB_VERB_REPMGR_MISC, &onoff)) != 0)
			return ret;
		if (onoff)
			*msgs |= DB_VERB_REPMGR_MISC;
		if ((ret = self->get_verbose(self, DB_VERB_WAITSFOR, &onoff)) != 0)
			return ret;
		if (onoff)
			*msgs |= DB_VERB_WAITSFOR;
		
		return 0;
	}
	int set_verbose(u_int32_t which, int onoff) {
		int ret;

		if ((which & DB_VERB_DEADLOCK) && 
			(ret = self->set_verbose(self, DB_VERB_DEADLOCK, onoff)) != 0)
			return ret;
		if ((which & DB_VERB_FILEOPS) && 
			(ret = self->set_verbose(self, DB_VERB_FILEOPS, onoff)) != 0)
			return ret;
		if ((which & DB_VERB_FILEOPS_ALL) && 
			(ret = self->set_verbose(self, DB_VERB_FILEOPS_ALL, onoff)) != 0)
			return ret;
		if ((which & DB_VERB_RECOVERY) && 
			(ret = self->set_verbose(self, DB_VERB_RECOVERY, onoff)) != 0)
			return ret;
		if ((which & DB_VERB_REGISTER) && 
			(ret = self->set_verbose(self, DB_VERB_REGISTER, onoff)) != 0)
			return ret;
		if ((which & DB_VERB_REPLICATION) && 
			(ret = self->set_verbose(self, DB_VERB_REPLICATION, onoff)) != 0)
			return ret;
		if ((which & DB_VERB_REP_ELECT) && 
			(ret = self->set_verbose(self, DB_VERB_REP_ELECT, onoff)) != 0)
			return ret;
		if ((which & DB_VERB_REP_LEASE) && 
			(ret = self->set_verbose(self, DB_VERB_REP_LEASE, onoff)) != 0)
			return ret;
		if ((which & DB_VERB_REP_MISC) && 
			(ret = self->set_verbose(self, DB_VERB_REP_MISC, onoff)) != 0)
			return ret;
		if ((which & DB_VERB_REP_MSGS) && 
			(ret = self->set_verbose(self, DB_VERB_REP_MSGS, onoff)) != 0)
			return ret;
		if ((which & DB_VERB_REP_SYNC) && 
			(ret = self->set_verbose(self, DB_VERB_REP_SYNC, onoff)) != 0)
			return ret;
		if ((which & DB_VERB_REPMGR_CONNFAIL) && 
			(ret = self->set_verbose(self, DB_VERB_REPMGR_CONNFAIL, onoff)) != 0)
			return ret;
		if ((which & DB_VERB_REPMGR_MISC) &&
			(ret = self->set_verbose(self, DB_VERB_REPMGR_MISC, onoff)) != 0)
			return ret;
		if ((which & DB_VERB_WAITSFOR) &&
			(ret = self->set_verbose(self, DB_VERB_WAITSFOR, onoff)) != 0)
			return ret;
		return 0;
	}
	
	int stat_print(u_int32_t flags) {
		return self->stat_print(self, flags);
	}

	%csmethodmodifiers txn_begin "private"
	DB_TXN *txn_begin(DB_TXN *parent, u_int32_t flags, int *err) {
		DB_TXN *txnid = NULL;
		
		*err = self->txn_begin(self, parent, &txnid, flags);
		return txnid;
	}
	
	int txn_checkpoint(u_int32_t kbyte, u_int32_t min, u_int32_t flags) {
		return self->txn_checkpoint(self, kbyte, min, flags);
	}
	
	%csmethodmodifiers txn_recover "private"
	%typemap(cstype) DB_PREPLIST [] "IntPtr"
	%typemap(imtype) DB_PREPLIST [] "IntPtr"
	%typemap(csin) DB_PREPLIST [] "$csinput"
	int txn_recover(DB_PREPLIST preplist[], u_int32_t count, u_int32_t *retp, u_int32_t flags) {
		return self->txn_recover(self, preplist, count, retp, flags);
	}

	%csmethodmodifiers txn_stat "private"		
	%typemap(cstype, out="IntPtr") void * "IntPtr"
	void *txn_stat(u_int32_t flags, u_int32_t *size, int *err) {
		DB_TXN_STAT *ret;
		*err = self->txn_stat(self, &ret, flags);
		*size = sizeof(DB_TXN_ACTIVE);
		return (void *)ret;
	}

	int txn_stat_print(u_int32_t flags) {
		return self->txn_stat_print(self, flags);
	}

}
} DB_ENV;

typedef struct __key_range {
	double less;
	double equal;
	double greater;
} DB_KEY_RANGE;

typedef struct __db_lock_u {
	roff_t		off;		/* Offset of the lock in the region */
	u_int32_t	ndx;		/* Index of the object referenced by
					 * this lock; used for locking. */
	u_int32_t	gen;		/* Generation number of this lock. */
	db_lockmode_t	mode;		/* mode of this lock. */
} DB_LOCK;

typedef struct __db_lockreq {
	db_lockop_t	 op;		/* Operation. */
	db_lockmode_t	 mode;		/* Requested mode. */
	db_timeout_t	 timeout;	/* Time to expire lock. */
	DBT		*obj;		/* Object being locked. */
	%rename(lck) lock;
	DB_LOCK		 lock;		/* Lock returned. */
} DB_LOCKREQ;

char *db_strerror(int errno);
int log_compare(DB_LSN *lsn0, DB_LSN *lsn1);
%typemap(cstype) void *ptr "IntPtr"
%typemap(imtype) void *ptr "IntPtr"
%typemap(csin) void *ptr "$csinput"
%rename(__os_ufree) wrap_ufree;
%inline %{
void wrap_ufree(DB_ENV *dbenv, void *ptr) {
	if (dbenv == NULL)
		__os_ufree(NULL, ptr);
	else
		__os_ufree(dbenv->env, ptr);
}
%}

typedef struct __db_preplist {
	%typemap(csvarout) u_int8_t gid[DB_GID_SIZE] %{
        get {
          byte[] ret = new byte[DbConstants.DB_GID_SIZE];
	  IntPtr cPtr = new IntPtr(swigCPtr.Handle.ToInt32() + IntPtr.Size);
	  Marshal.Copy(cPtr, ret, 0, ret.Length);
	  return ret;
        }
	%}

	DB_TXN	*txn;
	u_int8_t gid[DB_GID_SIZE];
} DB_PREPLIST;

%typemap(cscode) DB_SEQUENCE %{
	internal SequenceStatStruct stat(uint flags) {
		int err = 0;
		IntPtr ptr = stat(flags, ref err);
		DatabaseException.ThrowException(err);
		SequenceStatStruct ret = (SequenceStatStruct)Marshal.PtrToStructure(ptr, typeof(SequenceStatStruct));
		libdb_csharp.__os_ufree(null, ptr);
		return ret;
	}
%}
typedef struct __db_sequence {
%extend {
	DB_SEQUENCE(DB *dbp, u_int32_t flags) {
		DB_SEQUENCE *seq = NULL;
		int ret;
		
		ret = db_sequence_create(&seq, dbp, flags);
		if (ret == 0)
			return seq;
		else
			return NULL;
	}
	
	~DB_SEQUENCE() { }

	int close(u_int32_t flags) {
		return self->close(self, flags);
	}
	
	int get(DB_TXN *txn, int32_t delta, db_seq_t *retp, u_int32_t flags) {
		return self->get(self, txn, delta, retp, flags);
	}
	
	DB *get_db() {
		DB *dbp = NULL;
		int err = 0;
		err = self->get_db(self, &dbp);
		return dbp;
	}
	
	int get_key(DBT *key) {
		return self->get_key(self, key);
	}
	
	int initial_value(db_seq_t value) {
		return self->initial_value(self, value);
	}
	
	int open(DB_TXN *txn, DBT *key, u_int32_t flags) {
		return self->open(self, txn, key, flags);
	}
	
	int remove(DB_TXN *txn, u_int32_t flags) {
		return self->remove(self, txn, flags);
	}
	
	int get_cachesize(int32_t *size) {
		return self->get_cachesize(self, size);
	}
	int set_cachesize(int32_t size) {
		return self->set_cachesize(self, size);
	}
	
	int get_flags(u_int32_t *flags) {
		return self->get_flags(self, flags);
	}
	int set_flags(u_int32_t flags) {
		return self->set_flags(self, flags);
	}
	
	int get_range(db_seq_t *min, db_seq_t *max) {
		return self->get_range(self, min, max);
	}
	int set_range(db_seq_t min, db_seq_t max) {
		return self->set_range(self, min, max);
	}

	%csmethodmodifiers stat "private"		
	%typemap(cstype, out="IntPtr") void * "IntPtr"
	void *stat(u_int32_t flags, int *err) {
		DB_SEQUENCE_STAT *ret = NULL;

		*err = self->stat(self, &ret, flags);
		return (void *)ret;
	}

	int stat_print(u_int32_t flags) {
		return self->stat_print(self, flags);
	}
}
} DB_SEQUENCE;

%pragma(csharp) imclasscode=%{
#if DEBUG
	private const string libname = "libdb_csharp" + DbConstants.DB_VERSION_MAJOR_STR + DbConstants.DB_VERSION_MINOR_STR + "d";
#else
	private const string libname = "libdb_csharp" + DbConstants.DB_VERSION_MAJOR_STR + DbConstants.DB_VERSION_MINOR_STR;
#endif
%}
