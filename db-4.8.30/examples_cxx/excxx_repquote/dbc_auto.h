/*-
 * See the file LICENSE for redistribution information.
 *
 * Copyright (c) 2008-2009 Oracle.  All rights reserved.
 *
 * $Id$ 
 */

#include <db_cxx.h>

/*
 * Resource-acquisition-as-initialization pattern for Berkeley DB's cursors.
 *
 * Use DbcAuto instead of Berkeley DB's builtin Dbc class.  The constructor
 * allocates a new cursor, and it is freed automatically when it goes out of
 * scope.
 *
 * Note that some care is required with the order in which Berkeley DB handles
 * are closed.  In particular, the cursor handle must be closed before any
 * database or transaction handles the cursor references.  In addition, the
 * cursor close method can throw exceptions, which are masked by the destructor.
 * 
 * For these reasons, you are strongly advised to call the DbcAuto::close
 * method in the non-exceptional case.  This class exists to ensure that
 * cursors are closed if an exception occurs.
 */
class DbcAuto {
public:
    DbcAuto(Db *db, DbTxn *txn, u_int32_t flags) {
        db->cursor(txn, &dbc_, flags);
    }

    ~DbcAuto() {
        try {
            close();
        } catch(...) {
            // Ignore it, another exception is pending
        }
    }

    void close() {
        if (dbc_) {
            // Set the member to 0 before making the call in
            // case an exception is thrown.
            Dbc *tdbc = dbc_;
            dbc_ = 0;
            tdbc->close();
        }
    }

    operator Dbc *() {
        return dbc_;
    }

    operator Dbc **() {
        return &dbc_;
    }

    Dbc *operator->() {
        return dbc_;
    }

private:
    Dbc *dbc_;
};
