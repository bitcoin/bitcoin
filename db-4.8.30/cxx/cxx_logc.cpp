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

// It's private, and should never be called,
// but some compilers need it resolved
//
DbLogc::~DbLogc()
{
}

// The name _flags prevents a name clash with __db_log_cursor::flags
int DbLogc::close(u_int32_t _flags)
{
    DB_LOGC *logc = this;
    int ret;
    DbEnv *dbenv2 = DbEnv::get_DbEnv(logc->env->dbenv);

    ret = logc->close(logc, _flags);

    if (!DB_RETOK_STD(ret))
        DB_ERROR(dbenv2, "DbLogc::close", ret, ON_ERROR_UNKNOWN);

    return (ret);
}

// The name _flags prevents a name clash with __db_log_cursor::flags
int DbLogc::get(DbLsn *get_lsn, Dbt *data, u_int32_t _flags)
{
    DB_LOGC *logc = this;
    int ret;

    ret = logc->get(logc, get_lsn, data, _flags);

    if (!DB_RETOK_LGGET(ret)) {
        if (ret == DB_BUFFER_SMALL)
            DB_ERROR_DBT(DbEnv::get_DbEnv(logc->env->dbenv),
                "DbLogc::get", data, ON_ERROR_UNKNOWN);
        else
            DB_ERROR(DbEnv::get_DbEnv(logc->env->dbenv),
                "DbLogc::get", ret, ON_ERROR_UNKNOWN);
    }

    return (ret);
}

// The name _flags prevents a name clash with __db_log_cursor::flags
int DbLogc::version(u_int32_t *versionp, u_int32_t _flags)
{
    DB_LOGC *logc = this;
    int ret;

    ret = logc->version(logc, versionp, _flags);

    if (!DB_RETOK_LGGET(ret))
        DB_ERROR(DbEnv::get_DbEnv(logc->env->dbenv),
            "DbLogc::version", ret, ON_ERROR_UNKNOWN);

    return (ret);
}
