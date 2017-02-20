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
Operations that can be performed on locks.
*/
public final class LockOperation {
    /**
    Get the lock defined by the values of the mode and obj fields, for
    the specified locker.  Upon return from {@link com.sleepycat.db.Environment#lockVector Environment.lockVector}, if the lock field is non-null, a reference to the
    acquired lock is stored there.  (This reference is invalidated by
    any call to {@link com.sleepycat.db.Environment#lockVector Environment.lockVector} or {@link com.sleepycat.db.Environment#putLock Environment.putLock} that releases the lock.)
    */
    public static final LockOperation GET =
        new LockOperation("GET", DbConstants.DB_LOCK_GET);
    /**
    Identical to LockOperation GET except that the value in the timeout
    field overrides any previously specified timeout value for this
    lock.  A value of 0 turns off any previously specified timeout.
    */
    public static final LockOperation GET_TIMEOUT =
        new LockOperation("GET_TIMEOUT", DbConstants.DB_LOCK_GET_TIMEOUT);
    /**
    The lock to which the lock field refers is released.  The locker,
    mode and obj fields are ignored.
    */
    public static final LockOperation PUT =
        new LockOperation("PUT", DbConstants.DB_LOCK_PUT);
    /**
    All locks held by the specified locker are released.  The lock,
    mode, and obj fields are ignored.  Locks acquired in operations
    performed by the current call to {@link com.sleepycat.db.Environment#lockVector Environment.lockVector}
    which appear before the PUT_ALL operation are released; those
    acquired in operations appearing after the PUT_ALL operation are not
    released.
    */
    public static final LockOperation PUT_ALL =
        new LockOperation("PUT_ALL", DbConstants.DB_LOCK_PUT_ALL);
    /**
    All locks held on obj are released.  The locker parameter and the
    lock and mode fields are ignored.  Locks acquired in operations
    performed by the current call to {@link com.sleepycat.db.Environment#lockVector Environment.lockVector}
    that appear before the PUT_OBJ operation operation are released;
    those acquired in operations appearing after the PUT_OBJ operation
    are not released.
    */
    public static final LockOperation PUT_OBJ =
        new LockOperation("PUT_OBJ", DbConstants.DB_LOCK_PUT_OBJ);
    /**
    Cause the specified locker to timeout immediately.  If the database
    environment has not configured automatic deadlock detection, the
    transaction will timeout the next time deadlock detection is
    performed.  As transactions acquire locks on behalf of a single
    locker ID, timing out the locker ID associated with a transaction
    will time out the transaction itself.
    */
    public static final LockOperation TIMEOUT =
        new LockOperation("TIMEOUT", DbConstants.DB_LOCK_TIMEOUT);

    /* package */
    static LockOperation fromFlag(int flag) {
        switch (flag) {
        case DbConstants.DB_LOCK_GET:
            return GET;
        case DbConstants.DB_LOCK_GET_TIMEOUT:
            return GET_TIMEOUT;
        case DbConstants.DB_LOCK_PUT:
            return PUT;
        case DbConstants.DB_LOCK_PUT_ALL:
            return PUT_ALL;
        case DbConstants.DB_LOCK_PUT_OBJ:
            return PUT_OBJ;
        case DbConstants.DB_LOCK_TIMEOUT:
            return TIMEOUT;
        default:
            throw new IllegalArgumentException(
                "Unknown lock operation: " + flag);
        }
    }

    private final String operationName;
    private final int flag;

    private LockOperation(final String operationName, final int flag) {
        this.operationName = operationName;
        this.flag = flag;
    }

    /** {@inheritDoc} */
    public String toString() {
        return "LockOperation." + operationName;
    }

    /* package */
    int getFlag() {
        return flag;
    }
}
