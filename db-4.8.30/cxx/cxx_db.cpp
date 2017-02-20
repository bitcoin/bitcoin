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
#include "dbinc_auto/db_auto.h"
#include "dbinc_auto/crdel_auto.h"
#include "dbinc/db_dispatch.h"
#include "dbinc_auto/db_ext.h"
#include "dbinc_auto/common_ext.h"

// Helper macros for simple methods that pass through to the
// underlying C method. It may return an error or raise an exception.
// Note this macro expects that input _argspec is an argument
// list element (e.g., "char *arg") and that _arglist is the arguments
// that should be passed through to the C method (e.g., "(db, arg)")
//
#define DB_METHOD(_name, _argspec, _arglist, _retok)            \
int Db::_name _argspec                          \
{                                   \
    int ret;                            \
    DB *db = unwrap(this);                      \
                                    \
    ret = db->_name _arglist;                   \
    if (!_retok(ret))                       \
        DB_ERROR(dbenv_, "Db::" # _name, ret, error_policy());  \
    return (ret);                           \
}

#define DB_DESTRUCTOR(_name, _argspec, _arglist, _retok)        \
int Db::_name _argspec                          \
{                                   \
    int ret;                            \
    DB *db = unwrap(this);                      \
                                    \
    if (!db) {                          \
        DB_ERROR(dbenv_, "Db::" # _name, EINVAL, error_policy());   \
        return (EINVAL);                    \
    }                               \
    ret = db->_name _arglist;                   \
    cleanup();                          \
    if (!_retok(ret))                       \
        DB_ERROR(dbenv_, "Db::" # _name, ret, error_policy());  \
    return (ret);                           \
}

#define DB_METHOD_QUIET(_name, _argspec, _arglist)          \
int Db::_name _argspec                          \
{                                   \
    DB *db = unwrap(this);                      \
                                    \
    return (db->_name _arglist);                    \
}

#define DB_METHOD_VOID(_name, _argspec, _arglist)           \
void Db::_name _argspec                         \
{                                   \
    DB *db = unwrap(this);                      \
                                    \
    db->_name _arglist;                     \
}

// A truism for the Db object is that there is a valid
// DB handle from the constructor until close().
// After the close, the DB handle is invalid and
// no operations are permitted on the Db (other than
// destructor).  Leaving the Db handle open and not
// doing a close is generally considered an error.
//
// We used to allow Db objects to be closed and reopened.
// This implied always keeping a valid DB object, and
// coordinating the open objects between Db/DbEnv turned
// out to be overly complicated.  Now we do not allow this.

Db::Db(DbEnv *dbenv, u_int32_t flags)
:   imp_(0)
,   dbenv_(dbenv)
,   mpf_(0)
,   construct_error_(0)
,   flags_(0)
,   construct_flags_(flags)
,   append_recno_callback_(0)
,   associate_callback_(0)
,       associate_foreign_callback_(0)
,   bt_compare_callback_(0)
,   bt_compress_callback_(0)
,   bt_decompress_callback_(0)
,   bt_prefix_callback_(0)
,   db_partition_callback_(0)
,   dup_compare_callback_(0)
,   feedback_callback_(0)
,   h_compare_callback_(0)
,   h_hash_callback_(0)
{
    if (dbenv_ == 0)
        flags_ |= DB_CXX_PRIVATE_ENV;

    if ((construct_error_ = initialize()) != 0)
        DB_ERROR(dbenv_, "Db::Db", construct_error_, error_policy());
}

// If the DB handle is still open, we close it.  This is to make stack
// allocation of Db objects easier so that they are cleaned up in the error
// path.  If the environment was closed prior to this, it may cause a trap, but
// an error message is generated during the environment close.  Applications
// should call close explicitly in normal (non-exceptional) cases to check the
// return value.
//
Db::~Db()
{
    DB *db;

    db = unwrap(this);
    if (db != NULL) {
        (void)db->close(db, 0);
        cleanup();
    }
}

// private method to initialize during constructor.
// initialize must create a backing DB object,
// and if that creates a new DB_ENV, it must be tied to a new DbEnv.
//
int Db::initialize()
{
    DB *db;
    DB_ENV *cenv = unwrap(dbenv_);
    int ret;
    u_int32_t cxx_flags;

    cxx_flags = construct_flags_ & DB_CXX_NO_EXCEPTIONS;

    // Create a new underlying DB object.
    // We rely on the fact that if a NULL DB_ENV* is given,
    // one is allocated by DB.
    //
    if ((ret = db_create(&db, cenv,
                 construct_flags_ & ~cxx_flags)) != 0)
        return (ret);

    // Associate the DB with this object
    imp_ = db;
    db->api_internal = this;

    // Create a new DbEnv from a DB_ENV* if it was created locally.
    // It is deleted in Db::close().
    //
    if ((flags_ & DB_CXX_PRIVATE_ENV) != 0)
        dbenv_ = new DbEnv(db->dbenv, cxx_flags);

    // Create a DbMpoolFile from the DB_MPOOLFILE* in the DB handle.
    mpf_ = new DbMpoolFile();
    mpf_->imp_ = db->mpf;

    return (0);
}

// private method to cleanup after destructor or during close.
// If the environment was created by this Db object, we need to delete it.
//
void Db::cleanup()
{
    if (imp_ != 0) {
        imp_ = 0;

        // we must dispose of the DbEnv object if
        // we created it.  This will be the case
        // if a NULL DbEnv was passed into the constructor.
        // The underlying DB_ENV object will be inaccessible
        // after the close, so we must clean it up now.
        //
        if ((flags_ & DB_CXX_PRIVATE_ENV) != 0) {
            dbenv_->cleanup();
            delete dbenv_;
            dbenv_ = 0;
        }

        delete mpf_;
    }
}

// Return a tristate value corresponding to whether we should
// throw exceptions on errors:
//   ON_ERROR_RETURN
//   ON_ERROR_THROW
//   ON_ERROR_UNKNOWN
//
int Db::error_policy()
{
    if (dbenv_ != NULL)
        return (dbenv_->error_policy());
    else {
        // If the dbenv_ is null, that means that the user
        // did not attach an environment, so the correct error
        // policy can be deduced from constructor flags
        // for this Db.
        //
        if ((construct_flags_ & DB_CXX_NO_EXCEPTIONS) != 0) {
            return (ON_ERROR_RETURN);
        }
        else {
            return (ON_ERROR_THROW);
        }
    }
}

DB_DESTRUCTOR(close, (u_int32_t flags), (db, flags), DB_RETOK_STD)
DB_METHOD(compact, (DbTxn *txnid, Dbt *start, Dbt *stop,
    DB_COMPACT *c_data, u_int32_t flags, Dbt *end),
    (db, unwrap(txnid), start, stop, c_data, flags, end), DB_RETOK_STD)

// The following cast implies that Dbc can be no larger than DBC
DB_METHOD(cursor, (DbTxn *txnid, Dbc **cursorp, u_int32_t flags),
    (db, unwrap(txnid), (DBC **)cursorp, flags),
    DB_RETOK_STD)

DB_METHOD(del, (DbTxn *txnid, Dbt *key, u_int32_t flags),
    (db, unwrap(txnid), key, flags),
    DB_RETOK_DBDEL)

void Db::err(int error, const char *format, ...)
{
    DB *db = unwrap(this);

    DB_REAL_ERR(db->dbenv, error, DB_ERROR_SET, 1, format);
}

void Db::errx(const char *format, ...)
{
    DB *db = unwrap(this);

    DB_REAL_ERR(db->dbenv, 0, DB_ERROR_NOT_SET, 1, format);
}

DB_METHOD(exists, (DbTxn *txnid, Dbt *key, u_int32_t flags),
    (db, unwrap(txnid), key, flags), DB_RETOK_EXISTS)

DB_METHOD(fd, (int *fdp), (db, fdp), DB_RETOK_STD)

int Db::get(DbTxn *txnid, Dbt *key, Dbt *value, u_int32_t flags)
{
    DB *db = unwrap(this);
    int ret;

    ret = db->get(db, unwrap(txnid), key, value, flags);

    if (!DB_RETOK_DBGET(ret)) {
        if (ret == DB_BUFFER_SMALL)
            DB_ERROR_DBT(dbenv_, "Db::get", value, error_policy());
        else
            DB_ERROR(dbenv_, "Db::get", ret, error_policy());
    }

    return (ret);
}

int Db::get_byteswapped(int *isswapped)
{
    DB *db = (DB *)unwrapConst(this);
    return (db->get_byteswapped(db, isswapped));
}

DbEnv *Db::get_env()
{
    DB *db = (DB *)unwrapConst(this);
    DB_ENV *dbenv = db->get_env(db);
    return (dbenv != NULL ? DbEnv::get_DbEnv(dbenv) : NULL);
}

DbMpoolFile *Db::get_mpf()
{
    return (mpf_);
}

DB_METHOD(get_dbname, (const char **filenamep, const char **dbnamep),
    (db, filenamep, dbnamep), DB_RETOK_STD)

DB_METHOD(get_open_flags, (u_int32_t *flagsp), (db, flagsp), DB_RETOK_STD)

int Db::get_type(DBTYPE *dbtype)
{
    DB *db = (DB *)unwrapConst(this);
    return (db->get_type(db, dbtype));
}

// Dbc is a "compatible" subclass of DBC - that is, no virtual functions
// or even extra data members, so these casts, although technically
// non-portable, "should" always be okay.
DB_METHOD(join, (Dbc **curslist, Dbc **cursorp, u_int32_t flags),
    (db, (DBC **)curslist, (DBC **)cursorp, flags), DB_RETOK_STD)

DB_METHOD(key_range,
    (DbTxn *txnid, Dbt *key, DB_KEY_RANGE *results, u_int32_t flags),
    (db, unwrap(txnid), key, results, flags), DB_RETOK_STD)

// If an error occurred during the constructor, report it now.
// Otherwise, call the underlying DB->open method.
//
int Db::open(DbTxn *txnid, const char *file, const char *database,
         DBTYPE type, u_int32_t flags, int mode)
{
    int ret;
    DB *db = unwrap(this);

    if (construct_error_ != 0)
        ret = construct_error_;
    else
        ret = db->open(db, unwrap(txnid), file, database, type, flags,
            mode);

    if (!DB_RETOK_STD(ret))
        DB_ERROR(dbenv_, "Db::open", ret, error_policy());

    return (ret);
}

int Db::pget(DbTxn *txnid, Dbt *key, Dbt *pkey, Dbt *value, u_int32_t flags)
{
    DB *db = unwrap(this);
    int ret;

    ret = db->pget(db, unwrap(txnid), key, pkey, value, flags);

    /* The logic here is identical to Db::get - reuse the macro. */
    if (!DB_RETOK_DBGET(ret)) {
        if (ret == DB_BUFFER_SMALL && DB_OVERFLOWED_DBT(value))
            DB_ERROR_DBT(dbenv_, "Db::pget", value, error_policy());
        else
            DB_ERROR(dbenv_, "Db::pget", ret, error_policy());
    }

    return (ret);
}

DB_METHOD(put, (DbTxn *txnid, Dbt *key, Dbt *value, u_int32_t flags),
    (db, unwrap(txnid), key, value, flags), DB_RETOK_DBPUT)

DB_DESTRUCTOR(rename,
    (const char *file, const char *database, const char *newname,
    u_int32_t flags),
    (db, file, database, newname, flags), DB_RETOK_STD)

DB_DESTRUCTOR(remove, (const char *file, const char *database, u_int32_t flags),
    (db, file, database, flags), DB_RETOK_STD)

DB_METHOD(truncate, (DbTxn *txnid, u_int32_t *countp, u_int32_t flags),
    (db, unwrap(txnid), countp, flags), DB_RETOK_STD)

DB_METHOD(stat, (DbTxn *txnid, void *sp, u_int32_t flags),
    (db, unwrap(txnid), sp, flags), DB_RETOK_STD)

DB_METHOD(stat_print, (u_int32_t flags), (db, flags), DB_RETOK_STD)

DB_METHOD(sync, (u_int32_t flags), (db, flags), DB_RETOK_STD)

DB_METHOD(upgrade,
    (const char *name, u_int32_t flags), (db, name, flags), DB_RETOK_STD)

////////////////////////////////////////////////////////////////////////
//
// callbacks
//
// *_intercept_c are 'glue' functions that must be declared
// as extern "C" so to be typesafe.  Using a C++ method, even
// a static class method with 'correct' arguments, will not pass
// the test; some picky compilers do not allow mixing of function
// pointers to 'C' functions with function pointers to C++ functions.
//
// One wart with this scheme is that the *_callback_ method pointer
// must be declared public to be accessible by the C intercept.
// It's possible to accomplish the goal without this, and with
// another public transfer method, but it's just too much overhead.
// These callbacks are supposed to be *fast*.
//
// The DBTs we receive in these callbacks from the C layer may be
// manufactured there, but we want to treat them as a Dbts.
// Technically speaking, these DBTs were not constructed as a Dbts,
// but it should be safe to cast them as such given that Dbt is a
// *very* thin extension of the DBT.  That is, Dbt has no additional
// data elements, does not use virtual functions, virtual inheritance,
// multiple inheritance, RTI, or any other language feature that
// causes the structure to grow or be displaced.  Although this may
// sound risky, a design goal of C++ is complete structure
// compatibility with C, and has the philosophy 'if you don't use it,
// you shouldn't incur the overhead'.  If the C/C++ compilers you're
// using on a given machine do not have matching struct layouts, then
// a lot more things will be broken than just this.
//
// The alternative, creating a Dbt here in the callback, and populating
// it from the DBT, is just too slow and cumbersome to be very useful.

// These macros avoid a lot of boilerplate code for callbacks

#define DB_CALLBACK_C_INTERCEPT(_name, _rettype, _cargspec,     \
    _return, _cxxargs)                          \
extern "C" _rettype _db_##_name##_intercept_c _cargspec         \
{                                   \
    Db *cxxthis;                            \
                                    \
    /* We don't have a dbenv handle at this point. */       \
    DB_ASSERT(NULL, cthis != NULL);                 \
    cxxthis = Db::get_Db(cthis);                    \
    DB_ASSERT(cthis->dbenv->env, cxxthis != NULL);          \
    DB_ASSERT(cthis->dbenv->env, cxxthis->_name##_callback_ != 0);  \
                                    \
    _return (*cxxthis->_name##_callback_) _cxxargs;         \
}

#define DB_SET_CALLBACK(_cxxname, _name, _cxxargspec, _cb)      \
int Db::_cxxname _cxxargspec                        \
{                                   \
    DB *cthis = unwrap(this);                   \
                                    \
    _name##_callback_ = _cb;                    \
    return ((*(cthis->_cxxname))(cthis,             \
        (_cb) ? _db_##_name##_intercept_c : NULL));         \
}

#define DB_GET_CALLBACK(_cxxname, _name, _cxxargspec, _cbp)     \
int Db::_cxxname _cxxargspec                        \
{                                   \
    if (_cbp != NULL)                       \
        *(_cbp) = _name##_callback_;                \
    return 0;                           \
}

/* associate callback - doesn't quite fit the pattern because of the flags */
DB_CALLBACK_C_INTERCEPT(associate,
    int, (DB *cthis, const DBT *key, const DBT *data, DBT *retval),
    return, (cxxthis, Dbt::get_const_Dbt(key), Dbt::get_const_Dbt(data),
    Dbt::get_Dbt(retval)))

int Db::associate(DbTxn *txn, Db *secondary, int (*callback)(Db *, const Dbt *,
    const Dbt *, Dbt *), u_int32_t flags)
{
    DB *cthis = unwrap(this);

    /* Since the secondary Db is used as the first argument
     * to the callback, we store the C++ callback on it
     * rather than on 'this'.
     */
    secondary->associate_callback_ = callback;
    return ((*(cthis->associate))(cthis, unwrap(txn), unwrap(secondary),
        (callback) ? _db_associate_intercept_c : NULL, flags));
}

/* associate callback - doesn't quite fit the pattern because of the flags */
DB_CALLBACK_C_INTERCEPT(associate_foreign, int,
    (DB *cthis, const DBT *key, DBT *data, const DBT *fkey, int *changed),
    return, (cxxthis, Dbt::get_const_Dbt(key),
    Dbt::get_Dbt(data), Dbt::get_const_Dbt(fkey), changed))

int Db::associate_foreign(Db *secondary, int (*callback)(Db *,
        const Dbt *, Dbt *, const Dbt *, int *), u_int32_t flags)
{
    DB *cthis = unwrap(this);
    
    secondary->associate_foreign_callback_ = callback;
    return ((*(cthis->associate_foreign))(cthis, unwrap(secondary),
        (callback) ? _db_associate_foreign_intercept_c : NULL, flags));
}

DB_CALLBACK_C_INTERCEPT(feedback,
    void, (DB *cthis, int opcode, int pct),
    /* no return */ (void), (cxxthis, opcode, pct))

DB_GET_CALLBACK(get_feedback, feedback,
    (void (**argp)(Db *cxxthis, int opcode, int pct)), argp)

DB_SET_CALLBACK(set_feedback, feedback,
    (void (*arg)(Db *cxxthis, int opcode, int pct)), arg)

DB_CALLBACK_C_INTERCEPT(append_recno,
    int, (DB *cthis, DBT *data, db_recno_t recno),
    return, (cxxthis, Dbt::get_Dbt(data), recno))

DB_GET_CALLBACK(get_append_recno, append_recno,
    (int (**argp)(Db *cxxthis, Dbt *data, db_recno_t recno)), argp)

DB_SET_CALLBACK(set_append_recno, append_recno,
    (int (*arg)(Db *cxxthis, Dbt *data, db_recno_t recno)), arg)

DB_CALLBACK_C_INTERCEPT(bt_compare,
    int, (DB *cthis, const DBT *data1, const DBT *data2),
    return,
    (cxxthis, Dbt::get_const_Dbt(data1), Dbt::get_const_Dbt(data2)))

DB_GET_CALLBACK(get_bt_compare, bt_compare,
    (int (**argp)(Db *cxxthis, const Dbt *data1, const Dbt *data2)), argp)

DB_SET_CALLBACK(set_bt_compare, bt_compare,
    (int (*arg)(Db *cxxthis, const Dbt *data1, const Dbt *data2)), arg)

DB_CALLBACK_C_INTERCEPT(bt_compress,
    int, (DB *cthis, const DBT *data1, const DBT *data2, const DBT *data3,
    const DBT *data4, DBT *data5), return,
    (cxxthis, Dbt::get_const_Dbt(data1), Dbt::get_const_Dbt(data2),
    Dbt::get_const_Dbt(data3), Dbt::get_const_Dbt(data4), Dbt::get_Dbt(data5)))

DB_CALLBACK_C_INTERCEPT(bt_decompress,
    int, (DB *cthis, const DBT *data1, const DBT *data2, DBT *data3,
    DBT *data4, DBT *data5), return,
    (cxxthis, Dbt::get_const_Dbt(data1), Dbt::get_const_Dbt(data2),
    Dbt::get_Dbt(data3), Dbt::get_Dbt(data4), Dbt::get_Dbt(data5)))

// The {g|s}et_bt_compress methods don't fit into the standard macro templates
// since they take two callback functions.
int Db::get_bt_compress(
    int (**bt_compress)
    (Db *, const Dbt *, const Dbt *, const Dbt *, const Dbt *, Dbt *),
    int (**bt_decompress)
    (Db *, const Dbt *, const Dbt *, Dbt *, Dbt *, Dbt *))
{
    if (bt_compress != NULL)
        *(bt_compress) = bt_compress_callback_;
    if (bt_decompress != NULL)
        *(bt_decompress) = bt_decompress_callback_;
    return 0;
}

int Db::set_bt_compress(
    int (*bt_compress)
    (Db *, const Dbt *, const Dbt *, const Dbt *, const Dbt *, Dbt *),
    int (*bt_decompress)(Db *, const Dbt *, const Dbt *, Dbt *, Dbt *, Dbt *))
{
    DB *cthis = unwrap(this);

    bt_compress_callback_ = bt_compress;
    bt_decompress_callback_ = bt_decompress;
    return ((*(cthis->set_bt_compress))(cthis, 
        (bt_compress ? _db_bt_compress_intercept_c : NULL),
        (bt_decompress ? _db_bt_decompress_intercept_c : NULL)));

}

DB_CALLBACK_C_INTERCEPT(bt_prefix,
    size_t, (DB *cthis, const DBT *data1, const DBT *data2),
    return,
    (cxxthis, Dbt::get_const_Dbt(data1), Dbt::get_const_Dbt(data2)))

DB_GET_CALLBACK(get_bt_prefix, bt_prefix,
    (size_t (**argp)(Db *cxxthis, const Dbt *data1, const Dbt *data2)), argp)

DB_SET_CALLBACK(set_bt_prefix, bt_prefix,
    (size_t (*arg)(Db *cxxthis, const Dbt *data1, const Dbt *data2)), arg)

DB_CALLBACK_C_INTERCEPT(dup_compare,
    int, (DB *cthis, const DBT *data1, const DBT *data2),
    return,
    (cxxthis, Dbt::get_const_Dbt(data1), Dbt::get_const_Dbt(data2)))

DB_GET_CALLBACK(get_dup_compare, dup_compare,
    (int (**argp)(Db *cxxthis, const Dbt *data1, const Dbt *data2)), argp)

DB_SET_CALLBACK(set_dup_compare, dup_compare,
    (int (*arg)(Db *cxxthis, const Dbt *data1, const Dbt *data2)), arg)

DB_CALLBACK_C_INTERCEPT(h_compare,
    int, (DB *cthis, const DBT *data1, const DBT *data2),
    return,
    (cxxthis, Dbt::get_const_Dbt(data1), Dbt::get_const_Dbt(data2)))

DB_GET_CALLBACK(get_h_compare, h_compare,
    (int (**argp)(Db *cxxthis, const Dbt *data1, const Dbt *data2)), argp)

DB_SET_CALLBACK(set_h_compare, h_compare,
    (int (*arg)(Db *cxxthis, const Dbt *data1, const Dbt *data2)), arg)

DB_CALLBACK_C_INTERCEPT(h_hash,
    u_int32_t, (DB *cthis, const void *data, u_int32_t len),
    return, (cxxthis, data, len))

DB_GET_CALLBACK(get_h_hash, h_hash,
    (u_int32_t (**argp)(Db *cxxthis, const void *data, u_int32_t len)), argp)

DB_SET_CALLBACK(set_h_hash, h_hash,
    (u_int32_t (*arg)(Db *cxxthis, const void *data, u_int32_t len)), arg)

// This is a 'glue' function declared as extern "C" so it will
// be compatible with picky compilers that do not allow mixing
// of function pointers to 'C' functions with function pointers
// to C++ functions.
//
extern "C"
int _verify_callback_c(void *handle, const void *str_arg)
{
    char *str;
    __DB_STD(ostream) *out;

    str = (char *)str_arg;
    out = (__DB_STD(ostream) *)handle;

    (*out) << str;
    if (out->fail())
        return (EIO);

    return (0);
}

int Db::verify(const char *name, const char *subdb,
           __DB_STD(ostream) *ostr, u_int32_t flags)
{
    DB *db = unwrap(this);
    int ret;

    if (!db)
        ret = EINVAL;
    else {
        ret = __db_verify_internal(db, name, subdb, ostr,
            _verify_callback_c, flags);

        // After a DB->verify (no matter if success or failure),
        // the underlying DB object must not be accessed.
        //
        cleanup();
    }

    if (!DB_RETOK_STD(ret))
        DB_ERROR(dbenv_, "Db::verify", ret, error_policy());

    return (ret);
}

DB_METHOD(set_bt_compare, (bt_compare_fcn_type func),
    (db, func), DB_RETOK_STD)
DB_METHOD(get_bt_minkey, (u_int32_t *bt_minkeyp),
    (db, bt_minkeyp), DB_RETOK_STD)
DB_METHOD(set_bt_minkey, (u_int32_t bt_minkey),
    (db, bt_minkey), DB_RETOK_STD)
DB_METHOD(set_bt_prefix, (bt_prefix_fcn_type func),
    (db, func), DB_RETOK_STD)
DB_METHOD(set_dup_compare, (dup_compare_fcn_type func),
    (db, func), DB_RETOK_STD)
DB_METHOD(get_encrypt_flags, (u_int32_t *flagsp),
    (db, flagsp), DB_RETOK_STD)
DB_METHOD(set_encrypt, (const char *passwd, u_int32_t flags),
    (db, passwd, flags), DB_RETOK_STD)
DB_METHOD_VOID(get_errfile, (FILE **errfilep), (db, errfilep))
DB_METHOD_VOID(set_errfile, (FILE *errfile), (db, errfile))
DB_METHOD_VOID(get_errpfx, (const char **errpfx), (db, errpfx))
DB_METHOD_VOID(set_errpfx, (const char *errpfx), (db, errpfx))
DB_METHOD(get_flags, (u_int32_t *flagsp), (db, flagsp),
    DB_RETOK_STD)
DB_METHOD(set_flags, (u_int32_t flags), (db, flags),
    DB_RETOK_STD)
DB_METHOD(set_h_compare, (h_compare_fcn_type func),
    (db, func), DB_RETOK_STD)
DB_METHOD(get_h_ffactor, (u_int32_t *h_ffactorp),
    (db, h_ffactorp), DB_RETOK_STD)
DB_METHOD(set_h_ffactor, (u_int32_t h_ffactor),
    (db, h_ffactor), DB_RETOK_STD)
DB_METHOD(set_h_hash, (h_hash_fcn_type func),
    (db, func), DB_RETOK_STD)
DB_METHOD(get_h_nelem, (u_int32_t *h_nelemp),
    (db, h_nelemp), DB_RETOK_STD)
DB_METHOD(set_h_nelem, (u_int32_t h_nelem),
    (db, h_nelem), DB_RETOK_STD)
DB_METHOD(get_lorder, (int *db_lorderp), (db, db_lorderp),
    DB_RETOK_STD)
DB_METHOD(set_lorder, (int db_lorder), (db, db_lorder),
    DB_RETOK_STD)
DB_METHOD_VOID(get_msgfile, (FILE **msgfilep), (db, msgfilep))
DB_METHOD_VOID(set_msgfile, (FILE *msgfile), (db, msgfile))
DB_METHOD_QUIET(get_multiple, (), (db))
DB_METHOD(get_pagesize, (u_int32_t *db_pagesizep),
    (db, db_pagesizep), DB_RETOK_STD)
DB_METHOD(set_pagesize, (u_int32_t db_pagesize),
    (db, db_pagesize), DB_RETOK_STD)

DB_CALLBACK_C_INTERCEPT(db_partition,
    u_int32_t, (DB *cthis, DBT *key),
    return, (cxxthis, Dbt::get_Dbt(key)))

// set_partition and get_partition_callback do not fit into the macro 
// templates, since there is an additional argument in the API calls.
int Db::set_partition(u_int32_t parts, Dbt *keys, 
    u_int32_t (*arg)(Db *cxxthis, Dbt *key))
{
    DB *cthis = unwrap(this);

    db_partition_callback_ = arg;
    return ((*(cthis->set_partition))(cthis, parts, keys,
        arg ? _db_db_partition_intercept_c : NULL));
}

int Db::get_partition_callback(u_int32_t *parts, 
    u_int32_t (**argp)(Db *cxxthis, Dbt *key))
{
    DB *cthis = unwrap(this);
    if (argp != NULL)
        *(argp) = db_partition_callback_;
    if (parts != NULL)
        (cthis->get_partition_callback)(cthis, parts, NULL);
    return 0;
}

DB_METHOD(set_partition_dirs, (const char **dirp), (db, dirp), DB_RETOK_STD)
DB_METHOD(get_partition_dirs, (const char ***dirpp), (db, dirpp), DB_RETOK_STD)
DB_METHOD(get_partition_keys, (u_int32_t *parts, Dbt **keys),
    (db, parts, (DBT **)keys), DB_RETOK_STD)
DB_METHOD(get_priority, (DB_CACHE_PRIORITY *priorityp),
    (db, priorityp), DB_RETOK_STD)
DB_METHOD(set_priority, (DB_CACHE_PRIORITY priority),
    (db, priority), DB_RETOK_STD)
DB_METHOD(get_re_delim, (int *re_delimp),
    (db, re_delimp), DB_RETOK_STD)
DB_METHOD(set_re_delim, (int re_delim),
    (db, re_delim), DB_RETOK_STD)
DB_METHOD(get_re_len, (u_int32_t *re_lenp),
    (db, re_lenp), DB_RETOK_STD)
DB_METHOD(set_re_len, (u_int32_t re_len),
    (db, re_len), DB_RETOK_STD)
DB_METHOD(get_re_pad, (int *re_padp),
    (db, re_padp), DB_RETOK_STD)
DB_METHOD(set_re_pad, (int re_pad),
    (db, re_pad), DB_RETOK_STD)
DB_METHOD(get_re_source, (const char **re_source),
    (db, re_source), DB_RETOK_STD)
DB_METHOD(set_re_source, (const char *re_source),
    (db, re_source), DB_RETOK_STD)
DB_METHOD(sort_multiple, (Dbt *key, Dbt *data, u_int32_t flags),
    (db, key, data, flags), DB_RETOK_STD)
DB_METHOD(get_q_extentsize, (u_int32_t *extentsizep),
    (db, extentsizep), DB_RETOK_STD)
DB_METHOD(set_q_extentsize, (u_int32_t extentsize),
    (db, extentsize), DB_RETOK_STD)

DB_METHOD_QUIET(get_alloc, (db_malloc_fcn_type *malloc_fcnp,
    db_realloc_fcn_type *realloc_fcnp, db_free_fcn_type *free_fcnp),
    (db, malloc_fcnp, realloc_fcnp, free_fcnp))

DB_METHOD_QUIET(set_alloc, (db_malloc_fcn_type malloc_fcn,
    db_realloc_fcn_type realloc_fcn, db_free_fcn_type free_fcn),
    (db, malloc_fcn, realloc_fcn, free_fcn))

void Db::get_errcall(void (**argp)(const DbEnv *, const char *, const char *))
{
    dbenv_->get_errcall(argp);
}

void Db::set_errcall(void (*arg)(const DbEnv *, const char *, const char *))
{
    dbenv_->set_errcall(arg);
}

void Db::get_msgcall(void (**argp)(const DbEnv *, const char *))
{
    dbenv_->get_msgcall(argp);
}

void Db::set_msgcall(void (*arg)(const DbEnv *, const char *))
{
    dbenv_->set_msgcall(arg);
}

void *Db::get_app_private() const
{
    return unwrapConst(this)->app_private;
}

void Db::set_app_private(void *value)
{
    unwrap(this)->app_private = value;
}

DB_METHOD(get_cachesize, (u_int32_t *gbytesp, u_int32_t *bytesp, int *ncachep),
    (db, gbytesp, bytesp, ncachep), DB_RETOK_STD)
DB_METHOD(set_cachesize, (u_int32_t gbytes, u_int32_t bytes, int ncache),
    (db, gbytes, bytes, ncache), DB_RETOK_STD)

DB_METHOD(get_create_dir, (const char **dirp), (db, dirp), DB_RETOK_STD)
DB_METHOD(set_create_dir, (const char *dir), (db, dir), DB_RETOK_STD)

int Db::set_paniccall(void (*callback)(DbEnv *, int))
{
    return (dbenv_->set_paniccall(callback));
}

__DB_STD(ostream) *Db::get_error_stream()
{
    return dbenv_->get_error_stream();
}

void Db::set_error_stream(__DB_STD(ostream) *error_stream)
{
    dbenv_->set_error_stream(error_stream);
}

__DB_STD(ostream) *Db::get_message_stream()
{
    return dbenv_->get_message_stream();
}

void Db::set_message_stream(__DB_STD(ostream) *message_stream)
{
    dbenv_->set_message_stream(message_stream);
}

DB_METHOD_QUIET(get_transactional, (), (db))
