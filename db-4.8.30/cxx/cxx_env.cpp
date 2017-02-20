/*-
 * See the file LICENSE for redistribution information.
 *
 * Copyright (c) 1997-2009 Oracle.  All rights reserved.
 *
 * $Id$
 */

#include "db_config.h"

#include "db_int.h"

#include "db_cxx.h"
#include "dbinc/cxx_int.h"

#include "dbinc/db_page.h"
#include "dbinc/db_am.h"
#include "dbinc/log.h"
#include "dbinc_auto/common_ext.h"
#include "dbinc_auto/log_ext.h"

#ifdef HAVE_CXX_STDHEADERS
using std::cerr;
#endif

// Helper macros for simple methods that pass through to the
// underlying C method.  They may return an error or raise an exception.
// These macros expect that input _argspec is an argument
// list element (e.g., "char *arg") and that _arglist is the arguments
// that should be passed through to the C method (e.g., "(dbenv, arg)")
//
#define DBENV_METHOD_ERR(_name, _argspec, _arglist, _on_err)      \
int DbEnv::_name _argspec                     \
{                                 \
    DB_ENV *dbenv = unwrap(this);                 \
    int ret;                          \
                                  \
    if ((ret = dbenv->_name _arglist) != 0) {         \
        _on_err;                      \
    }                             \
    return (ret);                         \
}

#define DBENV_METHOD(_name, _argspec, _arglist)           \
    DBENV_METHOD_ERR(_name, _argspec, _arglist,       \
    DB_ERROR(this, "DbEnv::" # _name, ret, error_policy()))

#define DBENV_METHOD_QUIET(_name, _argspec, _arglist)         \
int DbEnv::_name _argspec                     \
{                                 \
    DB_ENV *dbenv = unwrap(this);                 \
                                  \
    return (dbenv->_name _arglist);               \
}

#define DBENV_METHOD_VOID(_name, _argspec, _arglist)          \
void DbEnv::_name _argspec                    \
{                                 \
    DB_ENV *dbenv = unwrap(this);                 \
                                  \
    dbenv->_name _arglist;                    \
}

// The reason for a static variable is that some structures
// (like Dbts) have no connection to any Db or DbEnv, so when
// errors occur in their methods, we must have some reasonable
// way to determine whether to throw or return errors.
//
// This variable is taken from flags whenever a DbEnv is constructed.
// Normally there is only one DbEnv per program, and even if not,
// there is typically a single policy of throwing or returning.
//
static int last_known_error_policy = ON_ERROR_UNKNOWN;

// These 'glue' function are declared as extern "C" so they will
// be compatible with picky compilers that do not allow mixing
// of function pointers to 'C' functions with function pointers
// to C++ functions.
//
extern "C"
void _feedback_intercept_c(DB_ENV *dbenv, int opcode, int pct)
{
    DbEnv::_feedback_intercept(dbenv, opcode, pct);
}

extern "C"
void _paniccall_intercept_c(DB_ENV *dbenv, int errval)
{
    DbEnv::_paniccall_intercept(dbenv, errval);
}

extern "C"
void _event_func_intercept_c(DB_ENV *dbenv, u_int32_t event, void *event_info)
{
    DbEnv::_event_func_intercept(dbenv, event, event_info);
}

extern "C"
void _stream_error_function_c(const DB_ENV *dbenv,
    const char *prefix, const char *message)
{
    DbEnv::_stream_error_function(dbenv, prefix, message);
}

extern "C"
void _stream_message_function_c(const DB_ENV *dbenv, const char *message)
{
    DbEnv::_stream_message_function(dbenv, message);
}

extern "C"
int _app_dispatch_intercept_c(DB_ENV *dbenv, DBT *dbt, DB_LSN *lsn, db_recops op)
{
    return (DbEnv::_app_dispatch_intercept(dbenv, dbt, lsn, op));
}

extern "C"
int _rep_send_intercept_c(DB_ENV *dbenv, const DBT *cntrl, const DBT *data,
    const DB_LSN *lsn, int id, u_int32_t flags)
{
    return (DbEnv::_rep_send_intercept(dbenv,
        cntrl, data, lsn, id, flags));
}

extern "C"
int _isalive_intercept_c(
    DB_ENV *dbenv, pid_t pid, db_threadid_t thrid, u_int32_t flags)
{
    return (DbEnv::_isalive_intercept(dbenv, pid, thrid, flags));
}

extern "C"
void _thread_id_intercept_c(DB_ENV *dbenv, pid_t *pidp, db_threadid_t *thridp)
{
    DbEnv::_thread_id_intercept(dbenv, pidp, thridp);
}

extern "C"
char *_thread_id_string_intercept_c(DB_ENV *dbenv, pid_t pid,
    db_threadid_t thrid, char *buf)
{
    return (DbEnv::_thread_id_string_intercept(dbenv, pid, thrid, buf));
}

void DbEnv::_feedback_intercept(DB_ENV *dbenv, int opcode, int pct)
{
    DbEnv *cxxenv = DbEnv::get_DbEnv(dbenv);
    if (cxxenv == 0) {
        DB_ERROR(0,
            "DbEnv::feedback_callback", EINVAL, ON_ERROR_UNKNOWN);
        return;
    }
    if (cxxenv->feedback_callback_ == 0) {
        DB_ERROR(DbEnv::get_DbEnv(dbenv),
            "DbEnv::feedback_callback", EINVAL, cxxenv->error_policy());
        return;
    }
    (*cxxenv->feedback_callback_)(cxxenv, opcode, pct);
}

void DbEnv::_paniccall_intercept(DB_ENV *dbenv, int errval)
{
    DbEnv *cxxenv = DbEnv::get_DbEnv(dbenv);
    if (cxxenv == 0) {
        DB_ERROR(0,
            "DbEnv::paniccall_callback", EINVAL, ON_ERROR_UNKNOWN);
        return;
    }
    if (cxxenv->paniccall_callback_ == 0) {
        DB_ERROR(cxxenv, "DbEnv::paniccall_callback", EINVAL,
            cxxenv->error_policy());
        return;
    }
    (*cxxenv->paniccall_callback_)(cxxenv, errval);
}

void DbEnv::_event_func_intercept(
    DB_ENV *dbenv, u_int32_t event, void *event_info)
{
    DbEnv *cxxenv = DbEnv::get_DbEnv(dbenv);
    if (cxxenv == 0) {
        DB_ERROR(0,
            "DbEnv::event_func_callback", EINVAL, ON_ERROR_UNKNOWN);
        return;
    }
    if (cxxenv->event_func_callback_ == 0) {
        DB_ERROR(cxxenv, "DbEnv::event_func_callback", EINVAL,
            cxxenv->error_policy());
        return;
    }
    (*cxxenv->event_func_callback_)(cxxenv, event, event_info);
}

int DbEnv::_app_dispatch_intercept(DB_ENV *dbenv, DBT *dbt, DB_LSN *lsn,
    db_recops op)
{
    DbEnv *cxxenv = DbEnv::get_DbEnv(dbenv);
    if (cxxenv == 0) {
        DB_ERROR(DbEnv::get_DbEnv(dbenv),
            "DbEnv::app_dispatch_callback", EINVAL, ON_ERROR_UNKNOWN);
        return (EINVAL);
    }
    if (cxxenv->app_dispatch_callback_ == 0) {
        DB_ERROR(DbEnv::get_DbEnv(dbenv),
            "DbEnv::app_dispatch_callback", EINVAL,
            cxxenv->error_policy());
        return (EINVAL);
    }
    Dbt *cxxdbt = (Dbt *)dbt;
    DbLsn *cxxlsn = (DbLsn *)lsn;
    return ((*cxxenv->app_dispatch_callback_)(cxxenv, cxxdbt, cxxlsn, op));
}

int DbEnv::_isalive_intercept(
    DB_ENV *dbenv, pid_t pid, db_threadid_t thrid, u_int32_t flags)
{
    DbEnv *cxxenv = DbEnv::get_DbEnv(dbenv);
    if (cxxenv == 0) {
        DB_ERROR(DbEnv::get_DbEnv(dbenv),
            "DbEnv::isalive_callback", EINVAL, ON_ERROR_UNKNOWN);
        return (0);
    }
    return ((*cxxenv->isalive_callback_)(cxxenv, pid, thrid, flags));
}

int DbEnv::_rep_send_intercept(DB_ENV *dbenv, const DBT *cntrl, const DBT *data,
    const DB_LSN *lsn, int id, u_int32_t flags)
{
    DbEnv *cxxenv = DbEnv::get_DbEnv(dbenv);
    if (cxxenv == 0) {
        DB_ERROR(DbEnv::get_DbEnv(dbenv),
            "DbEnv::rep_send_callback", EINVAL, ON_ERROR_UNKNOWN);
        return (EINVAL);
    }
    const Dbt *cxxcntrl = (const Dbt *)cntrl;
    const DbLsn *cxxlsn = (const DbLsn *)lsn;
    Dbt *cxxdata = (Dbt *)data;
    return ((*cxxenv->rep_send_callback_)(cxxenv,
        cxxcntrl, cxxdata, cxxlsn, id, flags));
}

void DbEnv::_thread_id_intercept(DB_ENV *dbenv,
    pid_t *pidp, db_threadid_t *thridp)
{
    DbEnv *cxxenv = DbEnv::get_DbEnv(dbenv);
    if (cxxenv == 0) {
        DB_ERROR(DbEnv::get_DbEnv(dbenv),
            "DbEnv::thread_id_callback", EINVAL, ON_ERROR_UNKNOWN);
    } else
        cxxenv->thread_id_callback_(cxxenv, pidp, thridp);
}

char *DbEnv::_thread_id_string_intercept(DB_ENV *dbenv,
    pid_t pid, db_threadid_t thrid, char *buf)
{
    DbEnv *cxxenv = DbEnv::get_DbEnv(dbenv);
    if (cxxenv == 0) {
        DB_ERROR(DbEnv::get_DbEnv(dbenv),
            "DbEnv::thread_id_string_callback", EINVAL,
            ON_ERROR_UNKNOWN);
        return (NULL);
    }
    return (cxxenv->thread_id_string_callback_(cxxenv, pid, thrid, buf));
}

// A truism for the DbEnv object is that there is a valid
// DB_ENV handle from the constructor until close().
// After the close, the DB_ENV handle is invalid and
// no operations are permitted on the DbEnv (other than
// destructor).  Leaving the DbEnv handle open and not
// doing a close is generally considered an error.
//
// We used to allow DbEnv objects to be closed and reopened.
// This implied always keeping a valid DB_ENV object, and
// coordinating the open objects between Db/DbEnv turned
// out to be overly complicated.  Now we do not allow this.

DbEnv::DbEnv(u_int32_t flags)
:   imp_(0)
,   construct_error_(0)
,   construct_flags_(flags)
,   error_stream_(0)
,   message_stream_(0)
,   app_dispatch_callback_(0)
,   feedback_callback_(0)
,   paniccall_callback_(0)
,   event_func_callback_(0)
,   rep_send_callback_(0)
{
    if ((construct_error_ = initialize(0)) != 0)
        DB_ERROR(this, "DbEnv::DbEnv", construct_error_,
            error_policy());
}

DbEnv::DbEnv(DB_ENV *dbenv, u_int32_t flags)
:   imp_(0)
,   construct_error_(0)
,   construct_flags_(flags)
,   error_stream_(0)
,   message_stream_(0)
,   app_dispatch_callback_(0)
,   feedback_callback_(0)
,   paniccall_callback_(0)
,   event_func_callback_(0)
,   rep_send_callback_(0)
{
    if ((construct_error_ = initialize(dbenv)) != 0)
        DB_ERROR(this, "DbEnv::DbEnv", construct_error_,
            error_policy());
}

// If the DB_ENV handle is still open, we close it.  This is to make stack
// allocation of DbEnv objects easier so that they are cleaned up in the error
// path.  Note that the C layer catches cases where handles are open in the
// environment at close time and reports an error.  Applications should call
// close explicitly in normal (non-exceptional) cases to check the return
// value.
//
DbEnv::~DbEnv()
{
    DB_ENV *dbenv = unwrap(this);

    if (dbenv != NULL) {
        (void)dbenv->close(dbenv, 0);
        cleanup();
    }
}

// called by destructors before the DB_ENV is destroyed.
void DbEnv::cleanup()
{
    imp_ = 0;
}

int DbEnv::close(u_int32_t flags)
{
    int ret;
    DB_ENV *dbenv = unwrap(this);

    ret = dbenv->close(dbenv, flags);

    // after a close (no matter if success or failure),
    // the underlying DB_ENV object must not be accessed.
    cleanup();

    // It's safe to throw an error after the close,
    // since our error mechanism does not peer into
    // the DB* structures.
    //
    if (ret != 0)
        DB_ERROR(this, "DbEnv::close", ret, error_policy());

    return (ret);
}

DBENV_METHOD(dbremove,
    (DbTxn *txn, const char *name, const char *subdb, u_int32_t flags),
    (dbenv, unwrap(txn), name, subdb, flags))
DBENV_METHOD(dbrename, (DbTxn *txn, const char *name, const char *subdb,
    const char *newname, u_int32_t flags),
    (dbenv, unwrap(txn), name, subdb, newname, flags))

void DbEnv::err(int error, const char *format, ...)
{
    DB_ENV *dbenv = unwrap(this);

    DB_REAL_ERR(dbenv, error, DB_ERROR_SET, 1, format);
}

// Return a tristate value corresponding to whether we should
// throw exceptions on errors:
//   ON_ERROR_RETURN
//   ON_ERROR_THROW
//   ON_ERROR_UNKNOWN
//
int DbEnv::error_policy()
{
    if ((construct_flags_ & DB_CXX_NO_EXCEPTIONS) != 0) {
        return (ON_ERROR_RETURN);
    }
    else {
        return (ON_ERROR_THROW);
    }
}

void DbEnv::errx(const char *format, ...)
{
    DB_ENV *dbenv = unwrap(this);

    DB_REAL_ERR(dbenv, 0, DB_ERROR_NOT_SET, 1, format);
}

void *DbEnv::get_app_private() const
{
    return unwrapConst(this)->app_private;
}

DBENV_METHOD(failchk, (u_int32_t flags), (dbenv, flags))
DBENV_METHOD(fileid_reset, (const char *file, u_int32_t flags),
    (dbenv, file, flags))
DBENV_METHOD(get_home, (const char **homep), (dbenv, homep))
DBENV_METHOD(get_open_flags, (u_int32_t *flagsp), (dbenv, flagsp))
DBENV_METHOD(get_data_dirs, (const char ***dirspp), (dbenv, dirspp))

bool DbEnv::is_bigendian()
{
    return unwrap(this)->is_bigendian() ? true : false;
}

DBENV_METHOD(get_thread_count, (u_int32_t *count), (dbenv, count))
DBENV_METHOD(set_thread_count, (u_int32_t count), (dbenv, count))

// used internally during constructor
// to associate an existing DB_ENV with this DbEnv,
// or create a new one.
//
int DbEnv::initialize(DB_ENV *dbenv)
{
    int ret;

    last_known_error_policy = error_policy();

    if (dbenv == 0) {
        // Create a new DB_ENV environment.
        if ((ret = ::db_env_create(&dbenv,
            construct_flags_ & ~DB_CXX_NO_EXCEPTIONS)) != 0)
            return (ret);
    }
    imp_ = dbenv;
    dbenv->api1_internal = this;    // for DB_ENV* to DbEnv* conversion
    return (0);
}

// lock methods
DBENV_METHOD(lock_detect, (u_int32_t flags, u_int32_t atype, int *aborted),
    (dbenv, flags, atype, aborted))
DBENV_METHOD_ERR(lock_get,
    (u_int32_t locker, u_int32_t flags, Dbt *obj,
    db_lockmode_t lock_mode, DbLock *lock),
    (dbenv, locker, flags, obj, lock_mode, &lock->lock_),
    DbEnv::runtime_error_lock_get(this, "DbEnv::lock_get", ret,
                  DB_LOCK_GET, lock_mode, obj, *lock,
                  -1, error_policy()))
DBENV_METHOD(lock_id, (u_int32_t *idp), (dbenv, idp))
DBENV_METHOD(lock_id_free, (u_int32_t id), (dbenv, id))
DBENV_METHOD(lock_put, (DbLock *lock), (dbenv, &lock->lock_))
DBENV_METHOD(lock_stat, (DB_LOCK_STAT **statp, u_int32_t flags),
    (dbenv, statp, flags))
DBENV_METHOD(lock_stat_print, (u_int32_t flags), (dbenv, flags))
DBENV_METHOD_ERR(lock_vec,
    (u_int32_t locker, u_int32_t flags, DB_LOCKREQ list[],
    int nlist, DB_LOCKREQ **elist_returned),
    (dbenv, locker, flags, list, nlist, elist_returned),
    DbEnv::runtime_error_lock_get(this, "DbEnv::lock_vec", ret,
    (*elist_returned)->op, (*elist_returned)->mode,
    Dbt::get_Dbt((*elist_returned)->obj), DbLock((*elist_returned)->lock),
    (int)((*elist_returned) - list), error_policy()))
// log methods
DBENV_METHOD(log_archive, (char **list[], u_int32_t flags),
    (dbenv, list, flags))

int DbEnv::log_compare(const DbLsn *lsn0, const DbLsn *lsn1)
{
    return (::log_compare(lsn0, lsn1));
}

// The following cast implies that DbLogc can be no larger than DB_LOGC
DBENV_METHOD(log_cursor, (DbLogc **cursorp, u_int32_t flags),
    (dbenv, (DB_LOGC **)cursorp, flags))
DBENV_METHOD(log_file, (DbLsn *lsn, char *namep, size_t len),
    (dbenv, lsn, namep, len))
DBENV_METHOD(log_flush, (const DbLsn *lsn), (dbenv, lsn))
DBENV_METHOD(log_get_config, (u_int32_t which, int *onoffp),
    (dbenv, which, onoffp))
DBENV_METHOD(log_put, (DbLsn *lsn, const Dbt *data, u_int32_t flags),
    (dbenv, lsn, data, flags))

int DbEnv::log_printf(DbTxn *txn, const char *fmt, ...)
{
    DB_ENV *dbenv = unwrap(this);
    va_list ap;
    int ret;

    va_start(ap, fmt);
    ret = __log_printf_pp(dbenv, unwrap(txn), fmt, ap);
    va_end(ap);

    return (ret);
}

DBENV_METHOD(log_set_config, (u_int32_t which, int onoff),
    (dbenv, which, onoff))
DBENV_METHOD(log_stat, (DB_LOG_STAT **spp, u_int32_t flags),
    (dbenv, spp, flags))
DBENV_METHOD(log_stat_print, (u_int32_t flags), (dbenv, flags))

DBENV_METHOD(lsn_reset, (const char *file, u_int32_t flags),
    (dbenv, file, flags))

int DbEnv::memp_fcreate(DbMpoolFile **dbmfp, u_int32_t flags)
{
    DB_ENV *dbenv = unwrap(this);
    int ret;
    DB_MPOOLFILE *mpf;

    if (dbenv == NULL)
        ret = EINVAL;
    else
        ret = dbenv->memp_fcreate(dbenv, &mpf, flags);

    if (DB_RETOK_STD(ret)) {
        *dbmfp = new DbMpoolFile();
        (*dbmfp)->imp_ = mpf;
    } else
        DB_ERROR(this, "DbMpoolFile::f_create", ret, ON_ERROR_UNKNOWN);

    return (ret);
}

DBENV_METHOD(memp_register,
    (int ftype, pgin_fcn_type pgin_fcn, pgout_fcn_type pgout_fcn),
    (dbenv, ftype, pgin_fcn, pgout_fcn))

// memory pool methods
DBENV_METHOD(memp_stat,
    (DB_MPOOL_STAT **gsp, DB_MPOOL_FSTAT ***fsp, u_int32_t flags),
    (dbenv, gsp, fsp, flags))
DBENV_METHOD(memp_stat_print, (u_int32_t flags), (dbenv, flags))
DBENV_METHOD(memp_sync, (DbLsn *sn), (dbenv, sn))
DBENV_METHOD(memp_trickle, (int pct, int *nwrotep), (dbenv, pct, nwrotep))

// If an error occurred during the constructor, report it now.
// Otherwise, call the underlying DB->open method.
//
int DbEnv::open(const char *db_home, u_int32_t flags, int mode)
{
    int ret;
    DB_ENV *dbenv = unwrap(this);

    if (construct_error_ != 0)
        ret = construct_error_;
    else
        ret = dbenv->open(dbenv, db_home, flags, mode);

    if (!DB_RETOK_STD(ret))
        DB_ERROR(this, "DbEnv::open", ret, error_policy());

    return (ret);
}

int DbEnv::remove(const char *db_home, u_int32_t flags)
{
    int ret;
    DB_ENV *dbenv = unwrap(this);

    ret = dbenv->remove(dbenv, db_home, flags);

    // after a remove (no matter if success or failure),
    // the underlying DB_ENV object must not be accessed,
    // so we clean up in advance.
    //
    cleanup();

    if (ret != 0)
        DB_ERROR(this, "DbEnv::remove", ret, error_policy());

    return (ret);
}

// Report an error associated with the DbEnv.
// error_policy is one of:
//   ON_ERROR_THROW     throw an error
//   ON_ERROR_RETURN    do nothing here, the caller will return an error
//   ON_ERROR_UNKNOWN   defer the policy to policy saved in DbEnv::DbEnv
//
void DbEnv::runtime_error(DbEnv *dbenv,
    const char *caller, int error, int error_policy)
{
    if (error_policy == ON_ERROR_UNKNOWN)
        error_policy = last_known_error_policy;
    if (error_policy == ON_ERROR_THROW) {
        // Creating and throwing the object in two separate
        // statements seems to be necessary for HP compilers.
        switch (error) {
        case DB_LOCK_DEADLOCK:
            {
                DbDeadlockException dl_except(caller);
                dl_except.set_env(dbenv);
                throw dl_except;
            }
        case DB_LOCK_NOTGRANTED:
            {
                DbLockNotGrantedException lng_except(caller);
                lng_except.set_env(dbenv);
                throw lng_except;
            }
        case DB_REP_HANDLE_DEAD:
            {
                DbRepHandleDeadException hd_except(caller);
                hd_except.set_env(dbenv);
                throw hd_except;
            }
        case DB_RUNRECOVERY:
            {
                DbRunRecoveryException rr_except(caller);
                rr_except.set_env(dbenv);
                throw rr_except;
            }
        default:
            {
                DbException except(caller, error);
                except.set_env(dbenv);
                throw except;
            }
        }
    }
}

// Like DbEnv::runtime_error, but issue a DbMemoryException
// based on the fact that this Dbt is not large enough.
void DbEnv::runtime_error_dbt(DbEnv *dbenv,
    const char *caller, Dbt *dbt, int error_policy)
{
    if (error_policy == ON_ERROR_UNKNOWN)
        error_policy = last_known_error_policy;
    if (error_policy == ON_ERROR_THROW) {
        // Creating and throwing the object in two separate
        // statements seems to be necessary for HP compilers.
        DbMemoryException except(caller, dbt);
        except.set_env(dbenv);
        throw except;
    }
}

// Like DbEnv::runtime_error, but issue a DbLockNotGrantedException,
// or a regular runtime error.
// call regular runtime_error if it
void DbEnv::runtime_error_lock_get(DbEnv *dbenv,
    const char *caller, int error,
    db_lockop_t op, db_lockmode_t mode, Dbt *obj,
    DbLock lock, int index, int error_policy)
{
    if (error != DB_LOCK_NOTGRANTED) {
        runtime_error(dbenv, caller, error, error_policy);
        return;
    }

    if (error_policy == ON_ERROR_UNKNOWN)
        error_policy = last_known_error_policy;
    if (error_policy == ON_ERROR_THROW) {
        // Creating and throwing the object in two separate
        // statements seems to be necessary for HP compilers.
        DbLockNotGrantedException except(caller, op, mode,
            obj, lock, index);
        except.set_env(dbenv);
        throw except;
    }
}

void DbEnv::_stream_error_function(
    const DB_ENV *dbenv, const char *prefix, const char *message)
{
    const DbEnv *cxxenv = DbEnv::get_const_DbEnv(dbenv);
    if (cxxenv == 0) {
        DB_ERROR(0,
            "DbEnv::stream_error", EINVAL, ON_ERROR_UNKNOWN);
        return;
    }

    if (cxxenv->error_callback_)
        cxxenv->error_callback_(cxxenv, prefix, message);
    else if (cxxenv->error_stream_) {
        // HP compilers need the extra casts, we don't know why.
        if (prefix) {
            (*cxxenv->error_stream_) << prefix;
            (*cxxenv->error_stream_) << (const char *)": ";
        }
        if (message)
            (*cxxenv->error_stream_) << (const char *)message;
        (*cxxenv->error_stream_) << (const char *)"\n";
    }
}

void DbEnv::_stream_message_function(const DB_ENV *dbenv, const char *message)
{
    const DbEnv *cxxenv = DbEnv::get_const_DbEnv(dbenv);
    if (cxxenv == 0) {
        DB_ERROR(0,
            "DbEnv::stream_message", EINVAL, ON_ERROR_UNKNOWN);
        return;
    }

    if (cxxenv->message_callback_)
        cxxenv->message_callback_(cxxenv, message);
    else if (cxxenv->message_stream_) {
        // HP compilers need the extra casts, we don't know why.
        (*cxxenv->message_stream_) << (const char *)message;
        (*cxxenv->message_stream_) << (const char *)"\n";
    }
}

// static method
char *DbEnv::strerror(int error)
{
    return (db_strerror(error));
}

// We keep these alphabetical by field name,
// for comparison with Java's list.
//
DBENV_METHOD(set_data_dir, (const char *dir), (dbenv, dir))
DBENV_METHOD(get_encrypt_flags, (u_int32_t *flagsp),
    (dbenv, flagsp))
DBENV_METHOD(set_encrypt, (const char *passwd, u_int32_t flags),
    (dbenv, passwd, flags))
DBENV_METHOD_VOID(get_errfile, (FILE **errfilep), (dbenv, errfilep))
DBENV_METHOD_VOID(set_errfile, (FILE *errfile), (dbenv, errfile))
DBENV_METHOD_VOID(get_errpfx, (const char **errpfxp), (dbenv, errpfxp))
DBENV_METHOD_VOID(set_errpfx, (const char *errpfx), (dbenv, errpfx))
DBENV_METHOD(get_intermediate_dir_mode, (const char **modep), (dbenv, modep))
DBENV_METHOD(set_intermediate_dir_mode, (const char *mode), (dbenv, mode))
DBENV_METHOD(get_lg_bsize, (u_int32_t *bsizep), (dbenv, bsizep))
DBENV_METHOD(set_lg_bsize, (u_int32_t bsize), (dbenv, bsize))
DBENV_METHOD(get_lg_dir, (const char **dirp), (dbenv, dirp))
DBENV_METHOD(set_lg_dir, (const char *dir), (dbenv, dir))
DBENV_METHOD(get_lg_filemode, (int *modep), (dbenv, modep))
DBENV_METHOD(set_lg_filemode, (int mode), (dbenv, mode))
DBENV_METHOD(get_lg_max, (u_int32_t *maxp), (dbenv, maxp))
DBENV_METHOD(set_lg_max, (u_int32_t max), (dbenv, max))
DBENV_METHOD(get_lg_regionmax, (u_int32_t *regionmaxp), (dbenv, regionmaxp))
DBENV_METHOD(set_lg_regionmax, (u_int32_t regionmax), (dbenv, regionmax))
DBENV_METHOD(get_lk_conflicts, (const u_int8_t **lk_conflictsp, int *lk_maxp),
    (dbenv, lk_conflictsp, lk_maxp))
DBENV_METHOD(set_lk_conflicts, (u_int8_t *lk_conflicts, int lk_max),
    (dbenv, lk_conflicts, lk_max))
DBENV_METHOD(get_lk_detect, (u_int32_t *detectp), (dbenv, detectp))
DBENV_METHOD(set_lk_detect, (u_int32_t detect), (dbenv, detect))
DBENV_METHOD(get_lk_max_lockers, (u_int32_t *max_lockersp),
    (dbenv, max_lockersp))
DBENV_METHOD(set_lk_max_lockers, (u_int32_t max_lockers), (dbenv, max_lockers))
DBENV_METHOD(get_lk_max_locks, (u_int32_t *max_locksp), (dbenv, max_locksp))
DBENV_METHOD(set_lk_max_locks, (u_int32_t max_locks), (dbenv, max_locks))
DBENV_METHOD(get_lk_max_objects, (u_int32_t *max_objectsp),
    (dbenv, max_objectsp))
DBENV_METHOD(set_lk_max_objects, (u_int32_t max_objects), (dbenv, max_objects))
DBENV_METHOD(get_lk_partitions, (u_int32_t *partitionsp), (dbenv, partitionsp))
DBENV_METHOD(set_lk_partitions, (u_int32_t partitions), (dbenv, partitions))
DBENV_METHOD(get_mp_max_openfd, (int *maxopenfdp), (dbenv, maxopenfdp))
DBENV_METHOD(set_mp_max_openfd, (int maxopenfd), (dbenv, maxopenfd))
DBENV_METHOD(get_mp_max_write, (int *maxwritep, db_timeout_t *maxwrite_sleepp),
    (dbenv, maxwritep, maxwrite_sleepp))
DBENV_METHOD(set_mp_max_write, (int maxwrite, db_timeout_t maxwrite_sleep),
    (dbenv, maxwrite, maxwrite_sleep))
DBENV_METHOD(get_mp_mmapsize, (size_t *mmapsizep), (dbenv, mmapsizep))
DBENV_METHOD(set_mp_mmapsize, (size_t mmapsize), (dbenv, mmapsize))
DBENV_METHOD(get_mp_pagesize, (u_int32_t *pagesizep), (dbenv, pagesizep))
DBENV_METHOD(set_mp_pagesize, (u_int32_t pagesize), (dbenv, pagesize))
DBENV_METHOD(get_mp_tablesize, (u_int32_t *tablesizep), (dbenv, tablesizep))
DBENV_METHOD(set_mp_tablesize, (u_int32_t tablesize), (dbenv, tablesize))
DBENV_METHOD_VOID(get_msgfile, (FILE **msgfilep), (dbenv, msgfilep))
DBENV_METHOD_VOID(set_msgfile, (FILE *msgfile), (dbenv, msgfile))
DBENV_METHOD(get_tmp_dir, (const char **tmp_dirp), (dbenv, tmp_dirp))
DBENV_METHOD(set_tmp_dir, (const char *tmp_dir), (dbenv, tmp_dir))
DBENV_METHOD(get_tx_max, (u_int32_t *tx_maxp), (dbenv, tx_maxp))
DBENV_METHOD(set_tx_max, (u_int32_t tx_max), (dbenv, tx_max))

DBENV_METHOD(stat_print, (u_int32_t flags), (dbenv, flags))

DBENV_METHOD_QUIET(get_alloc,
    (db_malloc_fcn_type *malloc_fcnp, db_realloc_fcn_type *realloc_fcnp,
    db_free_fcn_type *free_fcnp),
    (dbenv, malloc_fcnp, realloc_fcnp, free_fcnp))

DBENV_METHOD_QUIET(set_alloc,
    (db_malloc_fcn_type malloc_fcn, db_realloc_fcn_type realloc_fcn,
    db_free_fcn_type free_fcn),
    (dbenv, malloc_fcn, realloc_fcn, free_fcn))

void DbEnv::set_app_private(void *value)
{
    unwrap(this)->app_private = value;
}

DBENV_METHOD(get_cachesize,
    (u_int32_t *gbytesp, u_int32_t *bytesp, int *ncachep),
    (dbenv, gbytesp, bytesp, ncachep))
DBENV_METHOD(set_cachesize,
    (u_int32_t gbytes, u_int32_t bytes, int ncache),
    (dbenv, gbytes, bytes, ncache))
DBENV_METHOD(get_cache_max, (u_int32_t *gbytesp, u_int32_t *bytesp),
    (dbenv, gbytesp, bytesp))
DBENV_METHOD(set_cache_max, (u_int32_t gbytes, u_int32_t bytes),
    (dbenv, gbytes, bytes))
DBENV_METHOD(get_create_dir, (const char **dirp), (dbenv, dirp))
DBENV_METHOD(set_create_dir, (const char *dir), (dbenv, dir))

void DbEnv::get_errcall(void (**argp)(const DbEnv *, const char *, const char *))
{
    if (argp != NULL)
        *argp = error_callback_;
    return;
}

void DbEnv::set_errcall(void (*arg)(const DbEnv *, const char *, const char *))
{
    DB_ENV *dbenv = unwrap(this);

    error_callback_ = arg;
    error_stream_ = 0;

    dbenv->set_errcall(dbenv, (arg == 0) ? 0 :
               _stream_error_function_c);
}

__DB_STD(ostream) *DbEnv::get_error_stream()
{
    return (error_stream_);
}

void DbEnv::set_error_stream(__DB_STD(ostream) *stream)
{
    DB_ENV *dbenv = unwrap(this);

    error_stream_ = stream;
    error_callback_ = 0;

    dbenv->set_errcall(dbenv, (stream == 0) ? 0 :
               _stream_error_function_c);
}

int DbEnv::get_feedback(void (**argp)(DbEnv *, int, int))
{
    if (argp != NULL)
        *argp = feedback_callback_;
    return 0;
}

int DbEnv::set_feedback(void (*arg)(DbEnv *, int, int))
{
    DB_ENV *dbenv = unwrap(this);

    feedback_callback_ = arg;

    return (dbenv->set_feedback(dbenv,
        arg == 0 ? 0 : _feedback_intercept_c));
}

DBENV_METHOD(get_flags, (u_int32_t *flagsp), (dbenv, flagsp))
DBENV_METHOD(set_flags, (u_int32_t flags, int onoff), (dbenv, flags, onoff))

void DbEnv::get_msgcall(void (**argp)(const DbEnv *, const char *))
{
    if (argp != NULL)
        *argp = message_callback_;
}

void DbEnv::set_msgcall(void (*arg)(const DbEnv *, const char *))
{
    DB_ENV *dbenv = unwrap(this);

    message_callback_ = arg;
    message_stream_ = 0;

    dbenv->set_msgcall(dbenv, (arg == 0) ? 0 :
        _stream_message_function_c);
}

__DB_STD(ostream) *DbEnv::get_message_stream()
{
    return (message_stream_);
}

void DbEnv::set_message_stream(__DB_STD(ostream) *stream)
{
    DB_ENV *dbenv = unwrap(this);

    message_stream_ = stream;
    message_callback_ = 0;

    dbenv->set_msgcall(dbenv, (stream == 0) ? 0 :
               _stream_message_function_c);
}

int DbEnv::set_paniccall(void (*arg)(DbEnv *, int))
{
    DB_ENV *dbenv = unwrap(this);

    paniccall_callback_ = arg;

    return (dbenv->set_paniccall(dbenv,
        arg == 0 ? 0 : _paniccall_intercept_c));
}

int DbEnv::set_event_notify(void (*arg)(DbEnv *, u_int32_t, void *))
{
    DB_ENV *dbenv = unwrap(this);

    event_func_callback_ = arg;

    return (dbenv->set_event_notify(dbenv,
        arg == 0 ? 0 : _event_func_intercept_c));
}

DBENV_METHOD(set_rpc_server,
    (void *cl, char *host, long tsec, long ssec, u_int32_t flags),
    (dbenv, cl, host, tsec, ssec, flags))
DBENV_METHOD(get_shm_key, (long *shm_keyp), (dbenv, shm_keyp))
DBENV_METHOD(set_shm_key, (long shm_key), (dbenv, shm_key))

int DbEnv::get_app_dispatch
    (int (**argp)(DbEnv *, Dbt *, DbLsn *, db_recops))
{
    if (argp != NULL)
        *argp = app_dispatch_callback_;
    return 0;
}

int DbEnv::set_app_dispatch
    (int (*arg)(DbEnv *, Dbt *, DbLsn *, db_recops))
{
    DB_ENV *dbenv = unwrap(this);
    int ret;

    app_dispatch_callback_ = arg;
    if ((ret = dbenv->set_app_dispatch(dbenv,
        arg == 0 ? 0 : _app_dispatch_intercept_c)) != 0)
        DB_ERROR(this, "DbEnv::set_app_dispatch", ret, error_policy());

    return (ret);
}

int DbEnv::get_isalive
    (int (**argp)(DbEnv *, pid_t, db_threadid_t, u_int32_t))
{
    if (argp != NULL)
        *argp = isalive_callback_;
    return 0;
}

int DbEnv::set_isalive
    (int (*arg)(DbEnv *, pid_t, db_threadid_t, u_int32_t))
{
    DB_ENV *dbenv = unwrap(this);
    int ret;

    isalive_callback_ = arg;
    if ((ret = dbenv->set_isalive(dbenv,
        arg == 0 ? 0 : _isalive_intercept_c)) != 0)
        DB_ERROR(this, "DbEnv::set_isalive", ret, error_policy());

    return (ret);
}

DBENV_METHOD(get_tx_timestamp, (time_t *timestamp), (dbenv, timestamp))
DBENV_METHOD(set_tx_timestamp, (time_t *timestamp), (dbenv, timestamp))
DBENV_METHOD(get_verbose, (u_int32_t which, int *onoffp),
    (dbenv, which, onoffp))
DBENV_METHOD(set_verbose, (u_int32_t which, int onoff), (dbenv, which, onoff))

DBENV_METHOD(mutex_alloc,
    (u_int32_t flags, db_mutex_t *mutexp), (dbenv, flags, mutexp))
DBENV_METHOD(mutex_free, (db_mutex_t mutex), (dbenv, mutex))
DBENV_METHOD(mutex_get_align, (u_int32_t *argp), (dbenv, argp))
DBENV_METHOD(mutex_get_increment, (u_int32_t *argp), (dbenv, argp))
DBENV_METHOD(mutex_get_max, (u_int32_t *argp), (dbenv, argp))
DBENV_METHOD(mutex_get_tas_spins, (u_int32_t *argp), (dbenv, argp))
DBENV_METHOD(mutex_lock, (db_mutex_t mutex), (dbenv, mutex))
DBENV_METHOD(mutex_set_align, (u_int32_t arg), (dbenv, arg))
DBENV_METHOD(mutex_set_increment, (u_int32_t arg), (dbenv, arg))
DBENV_METHOD(mutex_set_max, (u_int32_t arg), (dbenv, arg))
DBENV_METHOD(mutex_set_tas_spins, (u_int32_t arg), (dbenv, arg))
DBENV_METHOD(mutex_stat,
    (DB_MUTEX_STAT **statp, u_int32_t flags), (dbenv, statp, flags))
DBENV_METHOD(mutex_stat_print, (u_int32_t flags), (dbenv, flags))
DBENV_METHOD(mutex_unlock, (db_mutex_t mutex), (dbenv, mutex))

int DbEnv::get_thread_id_fn(void (**argp)(DbEnv *, pid_t *, db_threadid_t *))
{
    if (argp != NULL)
        *argp = thread_id_callback_;
    return 0;
}

int DbEnv::set_thread_id(void (*arg)(DbEnv *, pid_t *, db_threadid_t *))
{
    DB_ENV *dbenv = unwrap(this);
    int ret;

    thread_id_callback_ = arg;
    if ((ret = dbenv->set_thread_id(dbenv,
        arg == 0 ? 0 : _thread_id_intercept_c)) != 0)
        DB_ERROR(this, "DbEnv::set_thread_id", ret, error_policy());

    return (ret);
}

int DbEnv::get_thread_id_string_fn(
    char *(**argp)(DbEnv *, pid_t, db_threadid_t, char *))
{
    if (argp != NULL)
        *argp = thread_id_string_callback_;
    return 0;
}

int DbEnv::set_thread_id_string(
    char *(*arg)(DbEnv *, pid_t, db_threadid_t, char *))
{
    DB_ENV *dbenv = unwrap(this);
    int ret;

    thread_id_string_callback_ = arg;
    if ((ret = dbenv->set_thread_id_string(dbenv,
        arg == 0 ? 0 : _thread_id_string_intercept_c)) != 0)
        DB_ERROR(this, "DbEnv::set_thread_id_string", ret,
            error_policy());

    return (ret);
}

DBENV_METHOD(add_data_dir, (const char *dir), (dbenv, dir))

int DbEnv::cdsgroup_begin(DbTxn **tid)
{
    DB_ENV *dbenv = unwrap(this);
    DB_TXN *txn;
    int ret;

    ret = dbenv->cdsgroup_begin(dbenv, &txn);
    if (DB_RETOK_STD(ret))
        *tid = new DbTxn(txn, NULL);
    else
        DB_ERROR(this, "DbEnv::cdsgroup_begin", ret, error_policy());

    return (ret);
}

int DbEnv::txn_begin(DbTxn *pid, DbTxn **tid, u_int32_t flags)
{
    DB_ENV *dbenv = unwrap(this);
    DB_TXN *txn;
    int ret;

    ret = dbenv->txn_begin(dbenv, unwrap(pid), &txn, flags);
    if (DB_RETOK_STD(ret))
        *tid = new DbTxn(txn, pid);
    else
        DB_ERROR(this, "DbEnv::txn_begin", ret, error_policy());

    return (ret);
}

DBENV_METHOD(txn_checkpoint, (u_int32_t kbyte, u_int32_t min, u_int32_t flags),
    (dbenv, kbyte, min, flags))

int DbEnv::txn_recover(DbPreplist *preplist, u_int32_t count,
    u_int32_t *retp, u_int32_t flags)
{
    DB_ENV *dbenv = unwrap(this);
    DB_PREPLIST *c_preplist;
    u_int32_t i;
    int ret;

    /*
     * We need to allocate some local storage for the
     * returned preplist, and that requires us to do
     * our own argument validation.
     */
    if (count <= 0)
        ret = EINVAL;
    else
        ret = __os_malloc(dbenv->env, sizeof(DB_PREPLIST) * count,
            &c_preplist);

    if (ret != 0) {
        DB_ERROR(this, "DbEnv::txn_recover", ret, error_policy());
        return (ret);
    }

    if ((ret =
        dbenv->txn_recover(dbenv, c_preplist, count, retp, flags)) != 0) {
        __os_free(dbenv->env, c_preplist);
        DB_ERROR(this, "DbEnv::txn_recover", ret, error_policy());
        return (ret);
    }

    for (i = 0; i < *retp; i++) {
        preplist[i].txn = new DbTxn(NULL);
        preplist[i].txn->imp_ = c_preplist[i].txn;
        memcpy(preplist[i].gid, c_preplist[i].gid,
            sizeof(preplist[i].gid));
    }

    __os_free(dbenv->env, c_preplist);

    return (0);
}

DBENV_METHOD(txn_stat, (DB_TXN_STAT **statp, u_int32_t flags),
    (dbenv, statp, flags))
DBENV_METHOD(txn_stat_print, (u_int32_t flags), (dbenv, flags))

int DbEnv::rep_set_transport(int myid, int (*arg)(DbEnv *,
    const Dbt *, const Dbt *, const DbLsn *, int, u_int32_t))
{
    DB_ENV *dbenv = unwrap(this);
    int ret;

    rep_send_callback_ = arg;
    if ((ret = dbenv->rep_set_transport(dbenv, myid,
        arg == 0 ? 0 : _rep_send_intercept_c)) != 0)
        DB_ERROR(this, "DbEnv::rep_set_transport", ret, error_policy());

    return (ret);
}

DBENV_METHOD(rep_elect, (u_int32_t nsites, u_int32_t nvotes, u_int32_t flags),
    (dbenv, nsites, nvotes, flags))
DBENV_METHOD(rep_flush, (), (dbenv))
DBENV_METHOD(rep_get_config, (u_int32_t which, int *onoffp),
    (dbenv, which, onoffp))
DBENV_METHOD(rep_get_request, (u_int32_t *min, u_int32_t *max),
    (dbenv, min, max))
DBENV_METHOD(rep_set_request, (u_int32_t min, u_int32_t max), (dbenv, min, max))

int DbEnv::rep_process_message(Dbt *control,
    Dbt *rec, int id, DbLsn *ret_lsnp)
{
    DB_ENV *dbenv = unwrap(this);
    int ret;

    ret = dbenv->rep_process_message(dbenv, control, rec, id, ret_lsnp);
    if (!DB_RETOK_REPPMSG(ret))
        DB_ERROR(this, "DbEnv::rep_process_message", ret,
            error_policy());

    return (ret);
}

DBENV_METHOD(rep_set_config,
    (u_int32_t which, int onoff), (dbenv, which, onoff))
DBENV_METHOD(rep_start,
    (Dbt *cookie, u_int32_t flags),
    (dbenv, (DBT *)cookie, flags))

DBENV_METHOD(rep_stat, (DB_REP_STAT **statp, u_int32_t flags),
    (dbenv, statp, flags))
DBENV_METHOD(rep_stat_print, (u_int32_t flags), (dbenv, flags))
DBENV_METHOD(rep_sync, (u_int32_t flags), (dbenv, flags))

DBENV_METHOD(rep_get_clockskew, (u_int32_t *fast_clockp, u_int32_t *slow_clockp),
    (dbenv, fast_clockp, slow_clockp))
DBENV_METHOD(rep_set_clockskew, (u_int32_t fast_clock, u_int32_t slow_clock),
    (dbenv, fast_clock, slow_clock))
DBENV_METHOD(rep_get_limit, (u_int32_t *gbytesp, u_int32_t *bytesp),
    (dbenv, gbytesp, bytesp))
DBENV_METHOD(rep_set_limit, (u_int32_t gbytes, u_int32_t bytes),
    (dbenv, gbytes, bytes))

//
// Begin advanced replication API method implementations
DBENV_METHOD(rep_get_nsites, (u_int32_t *n), (dbenv, n))
DBENV_METHOD(rep_set_nsites, (u_int32_t n), (dbenv, n))
DBENV_METHOD(rep_get_priority, (u_int32_t *priority),
    (dbenv, priority))
DBENV_METHOD(rep_set_priority, (u_int32_t priority),
    (dbenv, priority))
DBENV_METHOD(rep_get_timeout, (int which, db_timeout_t * timeout),
    (dbenv, which, timeout))
DBENV_METHOD(rep_set_timeout, (int which, db_timeout_t timeout),
    (dbenv, which, timeout))
DBENV_METHOD(repmgr_add_remote_site, (const char* host, u_int16_t port,
    int * eidp, u_int32_t flags), (dbenv, host, port, eidp, flags))
DBENV_METHOD(repmgr_get_ack_policy, (int *policy), (dbenv, policy))
DBENV_METHOD(repmgr_set_ack_policy, (int policy), (dbenv, policy))
DBENV_METHOD(repmgr_set_local_site, (const char* host, u_int16_t port,
    u_int32_t flags), (dbenv, host, port, flags))
DBENV_METHOD(repmgr_site_list, (u_int *countp, DB_REPMGR_SITE **listp),
    (dbenv, countp, listp))

int DbEnv::repmgr_start(int nthreads, u_int32_t flags)
{
    DB_ENV *dbenv = unwrap(this);
    int ret;

    ret = dbenv->repmgr_start(dbenv, nthreads, flags);
    if (!DB_RETOK_REPMGR_START(ret))
        DB_ERROR(this, "DbEnv::repmgr_start", ret,
            error_policy());

    return (ret);
}

DBENV_METHOD(repmgr_stat, (DB_REPMGR_STAT **statp, u_int32_t flags),
    (dbenv, statp, flags))
DBENV_METHOD(repmgr_stat_print, (u_int32_t flags), (dbenv, flags))

// End advanced replication API method implementations.

DBENV_METHOD(get_timeout,
    (db_timeout_t *timeoutp, u_int32_t flags),
    (dbenv, timeoutp, flags))
DBENV_METHOD(set_timeout,
    (db_timeout_t timeout, u_int32_t flags),
    (dbenv, timeout, flags))

// static method
char *DbEnv::version(int *major, int *minor, int *patch)
{
    return (db_version(major, minor, patch));
}

// static method
DbEnv *DbEnv::wrap_DB_ENV(DB_ENV *dbenv)
{
    DbEnv *wrapped_env = get_DbEnv(dbenv);
    return (wrapped_env != NULL) ? wrapped_env : new DbEnv(dbenv, 0);
}
