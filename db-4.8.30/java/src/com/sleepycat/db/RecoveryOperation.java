/*-
 * See the file LICENSE for redistribution information.
 *
 * Copyright (c) 2002-2009 Oracle.  All rights reserved.
 *
 * $Id$
 */

package com.sleepycat.db;

import com.sleepycat.db.internal.DbConstants;

/**
The recovery operation being performed when {@link com.sleepycat.db.LogRecordHandler#handleLogRecord LogRecordHandler.handleLogRecord} is called.
*/
public final class RecoveryOperation {
    /**
    The log is being read backward to determine which transactions have
    been committed and to abort those operations that were not; undo the
    operation described by the log record.
    */
    public static final RecoveryOperation BACKWARD_ROLL =
        new RecoveryOperation("BACKWARD_ROLL", DbConstants.DB_TXN_BACKWARD_ROLL);
    /**
    The log is being played forward; redo the operation described by the log
    record.
    <p>
    The FORWARD_ROLL and APPLY operations frequently imply the same actions,
    redoing changes that appear in the log record, although if a recovery
    function is to be used on a replication client where reads may be taking
    place concurrently with the processing of incoming messages, APPLY
    operations should also perform appropriate locking.
    */
    public static final RecoveryOperation FORWARD_ROLL =
        new RecoveryOperation("FORWARD_ROLL", DbConstants.DB_TXN_FORWARD_ROLL);
    /**
    The log is being read backward during a transaction abort; undo the
    operation described by the log record.
    */
    public static final RecoveryOperation ABORT =
        new RecoveryOperation("ABORT", DbConstants.DB_TXN_ABORT);
    /**
    The log is being applied on a replica site; redo the operation
    described by the log record.
    <p>
    The FORWARD_ROLL and APPLY operations frequently imply the same actions,
    redoing changes that appear in the log record, although if a recovery
    function is to be used on a replication client where reads may be taking
    place concurrently with the processing of incoming messages, APPLY
    operations should also perform appropriate locking.
    */
    public static final RecoveryOperation APPLY =
        new RecoveryOperation("APPLY", DbConstants.DB_TXN_APPLY);
    /**
    The log is being printed for debugging purposes; print the contents of
    this log record in the desired format.
    */
    public static final RecoveryOperation PRINT =
        new RecoveryOperation("PRINT", DbConstants.DB_TXN_PRINT);

    private String operationName;
    private int flag;

    private RecoveryOperation(String operationName, int flag) {
        this.operationName = operationName;
        this.flag = flag;
    }

    /** {@inheritDoc} */
    public String toString() {
        return "RecoveryOperation." + operationName;
    }

    /* This is public only so it can be called from internal/DbEnv.java. */
    /**
    Internal: this is public only so it can be called from an internal
    package.
     *
    @param flag
    the internal flag value to be wrapped in a RecoveryException object
    */
    public static RecoveryOperation fromFlag(int flag) {
        switch (flag) {
        case DbConstants.DB_TXN_BACKWARD_ROLL:
            return BACKWARD_ROLL;
        case DbConstants.DB_TXN_FORWARD_ROLL:
            return FORWARD_ROLL;
        case DbConstants.DB_TXN_ABORT:
            return ABORT;
        case DbConstants.DB_TXN_APPLY:
            return APPLY;
        case DbConstants.DB_TXN_PRINT:
            return PRINT;
        default:
            throw new IllegalArgumentException(
                "Unknown recover operation: " + flag);
        }
    }
}
