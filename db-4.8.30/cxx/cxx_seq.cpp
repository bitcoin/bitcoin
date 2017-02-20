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

// Helper macro for simple methods that pass through to the
// underlying C method. It may return an error or raise an exception.
// Note this macro expects that input _argspec is an argument
// list element (e.g., "char *arg") and that _arglist is the arguments
// that should be passed through to the C method (e.g., "(db, arg)")
//
#define DBSEQ_METHOD(_name, _argspec, _arglist, _destructor)        \
int DbSequence::_name _argspec                      \
{                                   \
    int ret;                            \
    DB_SEQUENCE *seq = unwrap(this);                \
    DbEnv *dbenv = DbEnv::get_DbEnv(seq->seq_dbp->dbenv);       \
                                    \
    ret = seq->_name _arglist;                  \
    if (_destructor)                        \
        imp_ = 0;                       \
    if (!DB_RETOK_STD(ret))                     \
        DB_ERROR(dbenv,                     \
            "DbSequence::" # _name, ret, ON_ERROR_UNKNOWN); \
    return (ret);                           \
}

DbSequence::DbSequence(Db *db, u_int32_t flags)
:   imp_(0)
{
    DB_SEQUENCE *seq;
    int ret;

    if ((ret = db_sequence_create(&seq, unwrap(db), flags)) != 0)
        DB_ERROR(db->get_env(), "DbSequence::DbSequence", ret,
            ON_ERROR_UNKNOWN);
    else {
        imp_ = seq;
        seq->api_internal = this;
    }
}

DbSequence::DbSequence(DB_SEQUENCE *seq)
:   imp_(seq)
{
    seq->api_internal = this;
}

DbSequence::~DbSequence()
{
    DB_SEQUENCE *seq;

    seq = unwrap(this);
    if (seq != NULL)
        (void)seq->close(seq, 0);
}

DBSEQ_METHOD(open, (DbTxn *txnid, Dbt *key, u_int32_t flags),
    (seq, unwrap(txnid), key, flags), 0)
DBSEQ_METHOD(initial_value, (db_seq_t value), (seq, value), 0)
DBSEQ_METHOD(close, (u_int32_t flags), (seq, flags), 1)
DBSEQ_METHOD(remove, (DbTxn *txnid, u_int32_t flags),
    (seq, unwrap(txnid), flags), 1)
DBSEQ_METHOD(stat, (DB_SEQUENCE_STAT **sp, u_int32_t flags),
    (seq, sp, flags), 0)
DBSEQ_METHOD(stat_print, (u_int32_t flags), (seq, flags), 0)

DBSEQ_METHOD(get,
    (DbTxn  *txnid, int32_t delta, db_seq_t *retp, u_int32_t flags),
    (seq, unwrap(txnid), delta, retp, flags), 0)
DBSEQ_METHOD(get_cachesize, (int32_t *sizep), (seq, sizep), 0)
DBSEQ_METHOD(set_cachesize, (int32_t size), (seq, size), 0)
DBSEQ_METHOD(get_flags, (u_int32_t *flagsp), (seq, flagsp), 0)
DBSEQ_METHOD(set_flags, (u_int32_t flags), (seq, flags), 0)
DBSEQ_METHOD(get_range, (db_seq_t *minp, db_seq_t *maxp), (seq, minp, maxp), 0)
DBSEQ_METHOD(set_range, (db_seq_t min, db_seq_t max), (seq, min, max), 0)

Db *DbSequence::get_db()
{
    DB_SEQUENCE *seq = unwrap(this);
    DB *db;
    (void)seq->get_db(seq, &db);
    return Db::get_Db(db);
}

Dbt *DbSequence::get_key()
{
    DB_SEQUENCE *seq = unwrap(this);
    memset(&key_, 0, sizeof(DBT));
    (void)seq->get_key(seq, &key_);
    return Dbt::get_Dbt(&key_);
}

// static method
DbSequence *DbSequence::wrap_DB_SEQUENCE(DB_SEQUENCE *seq)
{
    DbSequence *wrapped_seq = get_DbSequence(seq);
    return (wrapped_seq != NULL) ? wrapped_seq : new DbSequence(seq);
}
