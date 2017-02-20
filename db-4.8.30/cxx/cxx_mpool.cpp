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

// Helper macros for simple methods that pass through to the
// underlying C method. It may return an error or raise an exception.
// Note this macro expects that input _argspec is an argument
// list element (e.g., "char *arg") and that _arglist is the arguments
// that should be passed through to the C method (e.g., "(mpf, arg)")
//
#define DB_MPOOLFILE_METHOD(_name, _argspec, _arglist, _retok)      \
int DbMpoolFile::_name _argspec                     \
{                                   \
    int ret;                            \
    DB_MPOOLFILE *mpf = unwrap(this);               \
                                    \
    if (mpf == NULL)                        \
        ret = EINVAL;                       \
    else                                \
        ret = mpf->_name _arglist;              \
    if (!_retok(ret))                       \
        DB_ERROR(DbEnv::get_DbEnv(mpf->env->dbenv),         \
            "DbMpoolFile::"#_name, ret, ON_ERROR_UNKNOWN);  \
    return (ret);                           \
}

#define DB_MPOOLFILE_METHOD_VOID(_name, _argspec, _arglist)     \
void DbMpoolFile::_name _argspec                    \
{                                   \
    DB_MPOOLFILE *mpf = unwrap(this);               \
                                    \
    mpf->_name _arglist;                        \
}

////////////////////////////////////////////////////////////////////////
//                                                                    //
//                            DbMpoolFile                             //
//                                                                    //
////////////////////////////////////////////////////////////////////////

DbMpoolFile::DbMpoolFile()
:   imp_(0)
{
}

DbMpoolFile::~DbMpoolFile()
{
}

int DbMpoolFile::close(u_int32_t flags)
{
    DB_MPOOLFILE *mpf = unwrap(this);
    int ret;
    DbEnv *dbenv = DbEnv::get_DbEnv(mpf->env->dbenv);

    if (mpf == NULL)
        ret = EINVAL;
    else
        ret = mpf->close(mpf, flags);

    imp_ = 0;                   // extra safety

    // This may seem weird, but is legal as long as we don't access
    // any data before returning.
    delete this;

    if (!DB_RETOK_STD(ret))
        DB_ERROR(dbenv, "DbMpoolFile::close", ret, ON_ERROR_UNKNOWN);

    return (ret);
}

DB_MPOOLFILE_METHOD(get,
    (db_pgno_t *pgnoaddr, DbTxn *txn, u_int32_t flags, void *pagep),
    (mpf, pgnoaddr, unwrap(txn), flags, pagep), DB_RETOK_MPGET)
DB_MPOOLFILE_METHOD(open,
    (const char *file, u_int32_t flags, int mode, size_t pagesize),
    (mpf, file, flags, mode, pagesize), DB_RETOK_STD)
DB_MPOOLFILE_METHOD(put,
    (void *pgaddr, DB_CACHE_PRIORITY priority, u_int32_t flags),
    (mpf, pgaddr, priority, flags), DB_RETOK_STD)
DB_MPOOLFILE_METHOD(get_clear_len, (u_int32_t *lenp),
    (mpf, lenp), DB_RETOK_STD)
DB_MPOOLFILE_METHOD(set_clear_len, (u_int32_t len),
    (mpf, len), DB_RETOK_STD)
DB_MPOOLFILE_METHOD(get_fileid, (u_int8_t *fileid),
    (mpf, fileid), DB_RETOK_STD)
DB_MPOOLFILE_METHOD(set_fileid, (u_int8_t *fileid),
    (mpf, fileid), DB_RETOK_STD)
DB_MPOOLFILE_METHOD(get_flags, (u_int32_t *flagsp),
    (mpf, flagsp), DB_RETOK_STD)
DB_MPOOLFILE_METHOD(set_flags, (u_int32_t flags, int onoff),
    (mpf, flags, onoff), DB_RETOK_STD)
DB_MPOOLFILE_METHOD(get_ftype, (int *ftypep),
    (mpf, ftypep), DB_RETOK_STD)
DB_MPOOLFILE_METHOD(set_ftype, (int ftype),
    (mpf, ftype), DB_RETOK_STD)
DB_MPOOLFILE_METHOD(get_last_pgno, (db_pgno_t *pgnop),
    (mpf, pgnop), DB_RETOK_STD)
DB_MPOOLFILE_METHOD(get_lsn_offset, (int32_t *offsetp),
    (mpf, offsetp), DB_RETOK_STD)
DB_MPOOLFILE_METHOD(set_lsn_offset, (int32_t offset),
    (mpf, offset), DB_RETOK_STD)
DB_MPOOLFILE_METHOD(get_maxsize, (u_int32_t *gbytesp, u_int32_t *bytesp),
    (mpf, gbytesp, bytesp), DB_RETOK_STD)
DB_MPOOLFILE_METHOD(set_maxsize, (u_int32_t gbytes, u_int32_t bytes),
    (mpf, gbytes, bytes), DB_RETOK_STD)
DB_MPOOLFILE_METHOD(get_pgcookie, (DBT *dbt),
    (mpf, dbt), DB_RETOK_STD)
DB_MPOOLFILE_METHOD(set_pgcookie, (DBT *dbt),
    (mpf, dbt), DB_RETOK_STD)
DB_MPOOLFILE_METHOD(get_priority, (DB_CACHE_PRIORITY *priorityp),
    (mpf, priorityp), DB_RETOK_STD)
DB_MPOOLFILE_METHOD(set_priority, (DB_CACHE_PRIORITY priority),
    (mpf, priority), DB_RETOK_STD)
DB_MPOOLFILE_METHOD(sync, (),
    (mpf), DB_RETOK_STD)
